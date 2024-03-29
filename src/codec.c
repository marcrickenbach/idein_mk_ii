/* *****************************************************************************
 * @brief Codec implementation.
 */

/* *****************************************************************************
 * TODO
 * Initialize codec interface -- PCM5102APWR -- over i2s
 */

/* *****************************************************************************
 * Includes
 */

#include "codec.h"

#include <zephyr/smf.h>

#include <assert.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/gpio.h>

#include "codec/private/sm_evt.h"
#include "codec/private/module_data.h"

/* *****************************************************************************
 * Constants, Defines, and Macros
 */

#define SUCCESS (0)
#define FAIL    (-1)

#define OVERRIDE    true
#define NO_OVERRIDE false

#define NUM_BLOCKS 20

#define SAMPLE_NO 64

static int16_t data[SAMPLE_NO] = {
	  3211,   6392,   9511,  12539,  15446,  18204,  20787,  23169,
	 25329,  27244,  28897,  30272,  31356,  32137,  32609,  32767,
	 32609,  32137,  31356,  30272,  28897,  27244,  25329,  23169,
	 20787,  18204,  15446,  12539,   9511,   6392,   3211,      0,
	 -3212,  -6393,  -9512, -12540, -15447, -18205, -20788, -23170,
	-25330, -27245, -28898, -30273, -31357, -32138, -32610, -32767,
	-32610, -32138, -31357, -30273, -28898, -27245, -25330, -23170,
	-20788, -18205, -15447, -12540,  -9512,  -6393,  -3212,     -1,
};

#define BLOCK_SIZE (2 * sizeof(data))

#ifdef CONFIG_NOCACHE_MEMORY
	#define MEM_SLAB_CACHE_ATTR __nocache
#else
	#define MEM_SLAB_CACHE_ATTR
#endif /* CONFIG_NOCACHE_MEMORY */

static char MEM_SLAB_CACHE_ATTR __aligned(WB_UP(32))
	_k_mem_slab_buf_tx_0_mem_slab[(NUM_BLOCKS) * WB_UP(BLOCK_SIZE)];

static STRUCT_SECTION_ITERABLE(k_mem_slab, tx_0_mem_slab) =
	Z_MEM_SLAB_INITIALIZER(tx_0_mem_slab, _k_mem_slab_buf_tx_0_mem_slab,
				WB_UP(BLOCK_SIZE), NUM_BLOCKS);

/* *****************************************************************************
 * Debugging
 */

#if CONFIG_FKMG_CODEC_NO_OPTIMIZATIONS
#pragma GCC push_options
#pragma GCC optimize ("Og")
#warning "codec.c unoptimized!"
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(codec, CONFIG_FKMG_CODEC_LOG_LEVEL);

/* *****************************************************************************
 * Structs
 */

static struct codec_module_data codec_md = {0};
/* Convenience accessor to keep name short: md - module data. */
#define md codec_md


struct i2s_config i2s_cfg;
const struct device *i2s_dev = DEVICE_DT_GET(DT_NODELABEL(i2s2));

/* *****************************************************************************
 * Private
 */

/* *****
 * Utils
 * *****/

static bool instance_contains_state_machine_ctx(
        struct Codec_Instance * p_inst,
        struct smf_ctx      * p_sm_ctx)
{
    return(&p_inst->sm == p_sm_ctx);
}

/* Find the instance that contains the state machine context. */
static struct Codec_Instance * sm_ctx_to_instance(struct smf_ctx * p_sm_ctx)
{
    /* Iterate thru the instances. */
    struct Codec_Instance * p_inst = NULL;
    SYS_SLIST_FOR_EACH_CONTAINER(&md.list.instances, p_inst, node.instance){
        if(instance_contains_state_machine_ctx(p_inst, p_sm_ctx)) return p_inst;
    }
    return(NULL);
}



/* **************
 * Listener Utils
 * **************/

#if CONFIG_FKMG_CODEC_RUNTIME_ERROR_CHECKING
static enum Codec_Err_Id check_listener_cfg_param_for_errors(
        struct Codec_Listener_Cfg * p_cfg )
{
    if(!p_cfg
    || !p_cfg->p_iface
    || !p_cfg->p_lsnr) return(k_Codec_Err_Id_Configuration_Invalid);

    /* Is signal valid? */
    if(p_cfg->sig > k_Codec_Sig_Max)
        return(k_Codec_Err_Id_Configuration_Invalid);

    /* Is callback valid? */
    if(!p_cfg->cb)
        return(k_Codec_Err_Id_Configuration_Invalid);

    return(k_Codec_Err_Id_None);
}
#endif

static void clear_listener(struct Codec_Listener * p_lsnr)
{
    memset(p_lsnr, 0, sizeof(*p_lsnr));
}

static void config_listener(
        struct Codec_Listener     * p_lsnr,
        struct Codec_Listener_Cfg * p_cfg)
{
    /* Set listner's instance it is listening to. */
    p_lsnr->p_inst = p_cfg->p_inst;

    /* Set listner's callback. */ 
    p_lsnr->cb = p_cfg->cb;
}

static void init_listener(struct Codec_Listener * p_lsnr)
{
    clear_listener(p_lsnr);
}

/* **************
 * Instance Utils
 * **************/

#if CONFIG_FKMG_CODEC_RUNTIME_ERROR_CHECKING
static enum Codec_Err_Id check_instance_cfg_param_for_errors(
        struct Codec_Instance_Cfg * p_cfg)
{
    if(!p_cfg
    || !p_cfg->p_inst
    || !p_cfg->task.sm.p_thread
    || !p_cfg->task.sm.p_stack
    || !p_cfg->msgq.p_sm_evts) return(k_Codec_Err_Id_Configuration_Invalid);

    if(p_cfg->task.sm.stack_sz == 0) return(k_Codec_Err_Id_Configuration_Invalid);

    return(k_Codec_Err_Id_None);
}
#endif

static void add_instance_to_instances(
        struct Codec_Instance  * p_inst)
{
    sys_slist_append(&md.list.instances, &p_inst->node.instance);
}

static void config_instance_queues(
        struct Codec_Instance     * p_inst,
        struct Codec_Instance_Cfg * p_cfg)
{
    p_inst->msgq.p_sm_evts = p_cfg->msgq.p_sm_evts;
}

/* Forward reference */
 


// static void config_instance_deferred(
//         struct Pot_Instance     * p_inst,
//         struct Pot_Instance_Cfg * p_cfg)
// {
// }

/* Since configuration starts on caller's thread, configure fields that require
 * immediate and/or inconsequential configuration and defer rest to be handled
 * by our own thread later. */
static void config_instance_immediate(
        struct Codec_Instance     * p_inst,
        struct Codec_Instance_Cfg * p_cfg)
{
    config_instance_queues(p_inst, p_cfg);
}

static void init_instance_lists(struct Codec_Instance * p_inst)
{
    for(enum Codec_Evt_Sig sig = k_Codec_Evt_Sig_Beg;
                         sig < k_Codec_Evt_Sig_End;
                         sig++){
        sys_slist_init(&p_inst->list.listeners[sig]);
    }
}

static void clear_instance(struct Codec_Instance * p_inst)
{
    memset(p_inst, 0, sizeof(*p_inst));
}

static void init_instance(struct Codec_Instance * p_inst)
{
    clear_instance(p_inst);
    init_instance_lists(p_inst);
    #if CONFIG_FKMG_CODEC_RUNTIME_ERROR_CHECKING
    p_inst->err = k_Codec_Err_Id_None;
    #endif
}

/* ************
 * Module Utils
 * ************/

static void init_module_lists(void)
{
    sys_slist_init(&md.list.instances);
}

static void clear_module(void)
{
    memset(&md, 0, sizeof(md));
}

static void init_module(void)
{
    if(md.initialized) return;
    clear_module();
    init_module_lists();
    md.initialized = true;
}

/* **************
 * Error Checking
 * **************/

#if CONFIG_FKMG_CODEC_RUNTIME_ERROR_CHECKING
static void set_error(
        struct Codec_Instance * p_inst,
        enum Codec_Err_Id       err,
        bool                  override)
{
    if(p_inst){
        if((override                          )
        || (p_inst->err == k_Codec_Err_Id_None )){
            p_inst->err = err;
        }
    }
}

static bool errored(
        struct Codec_Instance * p_inst,
        enum Codec_Err_Id       err )
{
    set_error(p_inst, err, NO_OVERRIDE);
    return(err != k_Codec_Err_Id_None);
}
#endif /* CONFIG_FKMG_Codec_RUNTIME_ERROR_CHECKING */

/* ************
 * Broadcasting
 * ************/

static void broadcast(
        struct Codec_Evt      * p_evt,
        struct Codec_Listener * p_lsnr)
{
    /* call the listener, passing the event */
    if(p_lsnr->cb) p_lsnr->cb(p_evt);
}

static void broadcast_event_to_listeners(
        struct Codec_Instance * p_inst,
        struct Codec_Evt      * p_evt)
{
    enum Codec_Evt_Sig sig = p_evt->sig;
    struct Codec_Listener * p_lsnr = NULL;
    SYS_SLIST_FOR_EACH_CONTAINER(&p_inst->list.listeners[sig], p_lsnr, node.listener){
        broadcast(p_evt, p_lsnr);
    }
}

/* There's only 1 listener for instance initialization, and it is provided in
 * the cfg struct. */
static void broadcast_instance_initialized(
        struct Codec_Instance * p_inst,
        Codec_Listener_Cb       cb)
{
    struct Codec_Evt evt = {
            .sig = k_Codec_Evt_Sig_Instance_Initialized,
            .data.initd.p_inst = p_inst
    };

    struct Codec_Listener lsnr = {
        .p_inst = p_inst,
        .cb = cb
    };

    broadcast(&evt, &lsnr);
}

#if CONFIG_FKMG_CODEC_ALLOW_SHUTDOWN
static void broadcast_interface_deinitialized(
        struct Codec_Instance * p_inst,
        Codec_Listener_Cb       cb)
{
    struct Codec_Evt evt = {
            .sig = k_Codec_Instance_Deinitialized,
            .data.inst_deinit.p_inst = p_inst
    };

    struct Codec_Listener lsnr = {
        .p_inst = p_inst,
        .cb = cb
    };

    broadcast(&evt, &lsnr);
}
#endif


/* **************
 * Listener Utils
 * **************/

static void add_listener_for_signal_to_listener_list(
    struct Codec_Listener_Cfg * p_cfg)
{
    /* Get pointer to interface to add listener to. */
    struct Codec_Instance * p_inst = p_cfg->p_inst;

    /* Get pointer to configured listener. */
    struct Codec_Listener * p_lsnr = p_cfg->p_lsnr;

    /* Get signal to listen for. */
    enum Codec_Evt_Sig sig = p_cfg->sig;

    /* Add listener to instance's specified signal. */
    sys_slist_t * p_list = &p_inst->list.listeners[sig];
    sys_snode_t * p_node = &p_lsnr->node.listener;
    sys_slist_append(p_list, p_node);
}

#if CONFIG_FKMG_CODEC_ALLOW_LISTENER_REMOVAL
static bool find_list_containing_listener_and_remove_listener(
    struct Codec_Instance * p_inst,
	struct Codec_Listener * p_lsnr)
{
    for(enum Codec_Evt_Sig sig = k_Codec_Evt_Sig_Beg;
                         sig < k_Codec_Evt_Sig_End;
                         sig++){
        bool found_and_removed = sys_slist_find_and_remove(
                &p_inst->list.listeners[sig], &p_lsnr->node);
        if(found_and_removed) return(true);
    }
    return( false );
}
#endif

// static bool signal_has_listeners(
//         struct Codec_Instance * p_inst,
//         enum Codec_Evt_Sig      sig)
// {
//     return(!sys_slist_is_empty(&p_inst->list.listeners[sig]));
// }

/* **************
 * Event Queueing
 * **************/

static void q_sm_event(
        struct Codec_Instance * p_inst, struct Codec_SM_Evt * p_evt)
{
    bool queued = k_msgq_put(p_inst->msgq.p_sm_evts, p_evt, K_NO_WAIT) == 0;

    if(!queued) assert(false);
}

static void q_init_instance_event(struct Codec_Instance_Cfg * p_cfg)
{
    struct Codec_SM_Evt evt = {
            .sig = k_Codec_SM_Evt_Sig_Init_Instance,
            .data.init_inst.cfg = *p_cfg
    };
    struct Codec_Instance * p_inst = p_cfg->p_inst;
    q_sm_event(p_inst, &evt);
}


/* **************
 * I2S Configs
 * **************/

static int i2s_init(void)
{
    if (!device_is_ready(i2s_dev)) {
        LOG_ERR("I2S device %s is not ready", i2s_dev->name);
        return -ENODEV;
    }

    i2s_cfg.word_size = 16U;
	i2s_cfg.channels = 2U;
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S;
	i2s_cfg.frame_clk_freq = 44100;
	i2s_cfg.block_size = BLOCK_SIZE;
	i2s_cfg.timeout = 2000;
	/* Configure the Transmit port as Master */
	i2s_cfg.options = I2S_OPT_FRAME_CLK_MASTER
			| I2S_OPT_BIT_CLK_MASTER;
	i2s_cfg.mem_slab = &tx_0_mem_slab;

    return i2s_configure(i2s_dev, I2S_DIR_TX, &i2s_cfg);
}


/* Fill buffer with sine wave on left channel, and sine wave shifted by
 * 90 degrees on right channel. "att" represents a power of two to attenuate
 * the samples by
 */
static void fill_buf(int16_t *tx_block, int att)
{
	int r_idx;

	for (int i = 0; i < SAMPLE_NO; i++) {
		/* Left channel is sine wave */
		tx_block[2 * i] = data[i] / (1 << att);
		/* Right channel is same sine wave, shifted by 90 degrees */
		r_idx = (i + (ARRAY_SIZE(data) / 4)) % ARRAY_SIZE(data);
		tx_block[2 * i + 1] = data[r_idx] / (1 << att);
	}
}

static int tx_i2s_data(struct Codec_Instance * p_inst)
{
    
    void *tx_block[NUM_BLOCKS];
    uint32_t tx_idx;
    int ret;

    for (tx_idx = 0; tx_idx < NUM_BLOCKS; tx_idx++) {
		ret = k_mem_slab_alloc(&tx_0_mem_slab, &tx_block[tx_idx],
				       K_FOREVER);
		if (ret < 0) {
			printf("Failed to allocate TX block\n");
			return ret;
		}
		fill_buf((uint16_t *)tx_block[tx_idx], tx_idx % 3);
	}

	tx_idx = 0;
	/* Send first block */
	ret = i2s_write(i2s_dev, tx_block[tx_idx++], BLOCK_SIZE);
	if (ret < 0) {
		printf("Could not write TX buffer %d\n", tx_idx);
		return ret;
	}
	/* Trigger the I2S transmission */
	ret = i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_START);
	if (ret < 0) {
		printf("Could not trigger I2S tx\n");
		return ret;
	}

	for (; tx_idx < NUM_BLOCKS; ) {
		ret = i2s_write(i2s_dev, tx_block[tx_idx++], BLOCK_SIZE);
		if (ret < 0) {
			printf("Could not write TX buffer %d\n", tx_idx);
			return ret;
		}
	}
	/* Drain TX queue */
	ret = i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
	if (ret < 0) {
		printf("Could not trigger I2S tx\n");
		return ret;
	}
	printf("All I2S blocks written\n");
}

/* **********
 * FSM States
 * **********/

/* The states are flat:
 *
 *  State init:
 *
 *  |entry          |  |run                |  |exit           |
 *  |---------------|  |-------------------|  |---------------|
 *  |not implemented|  |init instance:     |  |not implemented|
 *                     |* finish cfg.      |
 *                     |* ->run            |
 *                     |all other signals  |
 *                     |not allowed; assert|
 *
 *  State run:
 *
 *  |entry             |  |run             |  |exit           |
 *  |------------------|  |----------------|  |---------------|
 *  |not implemented   |  |convert:        |  |not implemented|
 *  |                  |  |* codec calculate |
 *                        |* quantize cv   |
 *                        |* filter        |
 *                        |deinit:         |
 *                        |* ->deinit      |
 *                        |all others:     |
 *                        |* assert        |
 *
 *  State deinit:
 *
 *  |entry             |  |run                 |  |exit           |
 *  |------------------|  |--------------------|  |---------------|
 *  |* start conversion|  |* finish dcfg.      |  |not implemented|
 *  |  timer           |  |* exit state machine|
 *                        |all others:         |
 *                        |* assert            |
 *
 *  Legend:
 *  "entry state": 1st state of state machine
 *  "xxx:"       : signal to handle
 *  "*"          : action to take on signal
 *  "-> xxx"     : state to change to
 *  "^ xxx"      : pass signal to parent, with expected parent action on signal.
 */


/* Forward declaration of state table */
static const struct smf_state states[];

enum state{
    init,   // Init instance - should only occur once, after thread start
    run,    // Run - handles all events while running
    deinit, // Deinit instance - should only occur once, after deinit event
            // (if implemented)
};



/* NOTE: in all the state functions the param o is a pointer to the state
 * machine context, which is &p_inst->sm. */

/* Init state responsibility is to complete initialization of the instance, let
 * an instance config listener know we've completed initialization (i.e. thread
 * is up and running), and then transition to next state. Init state occurs
 * immediately after thread start and is expected to only occur once. */

static void state_init_run(void * o)
{
    struct smf_ctx * p_sm = o;
    struct Codec_Instance * p_inst = sm_ctx_to_instance(p_sm);

    /* Get the event. */
    struct Codec_SM_Evt * p_evt = &p_inst->sm_evt;

    /* Expecting only an "init instance" event. Anything else is an error. */
    assert(p_evt->sig == k_Codec_SM_Evt_Sig_Init_Instance);

    /* We init'd required params on the caller's thread (i.e.
     * Codec_Init_Instance()), now finish the job. Since this is an
     * Init_Instance event the data contains the intance cfg. */
    struct Codec_SM_Evt_Sig_Init_Instance * p_ii = &p_evt->data.init_inst;

    broadcast_instance_initialized(p_inst, p_ii->cfg.cb);

    i2s_init(); 

    smf_set_state(SMF_CTX(p_sm), &states[run]);
}

/* Run state responsibility is to be the root state to handle everything else
 * other than instance initialization. */

static void state_run_entry(void * o)
{
    struct smf_ctx * p_sm = o;
    struct Codec_Instance * p_inst = sm_ctx_to_instance(p_sm);


}

static void state_run_run(void * o)
{
    struct smf_ctx * p_sm = o;
    struct Codec_Instance * p_inst = sm_ctx_to_instance(p_sm);

    /* Get the event. */
    struct Codec_SM_Evt * p_evt = &p_inst->sm_evt;

    switch(p_evt->sig){
        default: break;
        case k_Codec_SM_Evt_Sig_Init_Instance:
            /* Should never occur. */
            assert(false);
            break;
        case k_Codec_Evt_Sig_Write:
            tx_i2s_data(p_inst);
            break;
        #if CONFIG_FKMG_CODEC_SHUTDOWN_ENABLED
        case k_Codec_Evt_Sig_Instance_Deinitialized:
            assert(false);
            break;
        #endif
    }
}

/* Deinit state responsibility is to clean up before exiting thread. */
#if CONFIG_FKMG_CODEC_SHUTDOWN_ENABLED
static void state_deinit_run(void * o)
{
    struct smf_ctx * p_sm = o;
    struct Codec_Instance * p_inst = sm_ctx_to_instance(p_sm);

    /* Get the event. */
    struct Codec_SM_Evt * p_evt = &p_inst->sm_evt;

    /* TODO */
}
#endif

static const struct smf_state states[] = {
    /*                                      entry               run  exit */
    [  init] = SMF_CREATE_STATE(           NULL,   state_init_run, NULL),
    [   run] = SMF_CREATE_STATE(state_run_entry,    state_run_run, NULL),
    #if CONFIG_FKMG_CODEC_SHUTDOWN_ENABLED
    [deinit] = SMF_CREATE_STATE(           NULL, state_deinit_run, NULL),
    #endif
};

/* ******
 * Thread
 * ******/

#if CONFIG_FKMG_CODEC_ALLOW_SHUTDOWN
/* Since there is only the "join" facility to know when a thread is shut down,
 * and that isn't appropriate to use since it will put the calling thread to
 * sleep until the other thread is shut down, we set up a delayable system work
 * queue event to check that the thread is shut down and then call any callback
 * that is waiting to be notified. */
void on_thread_shutdown(struct k_work *item)
{
    struct Codec_Instance * p_inst =
            CONTAINER_OF(item, struct Codec_Instance, work);

    char * thread_state_str = "dead";
    k_thread_state_str(&p_inst->thread, thread_state_str, sizeof(thread_state_str));
    bool shut_down = strcmp( thread_state_str, "dead" ) == 0;

    if(!shut_down) k_work_reschedule(&p_inst->work, K_MSEC(1));
    else broadcast_instance_deinitialized(p_inst);
}
#endif

static void thread(void * p_1, /* struct Codec_Instance* */
        void * p_2_unused, void * p_3_unused)
{
    struct Codec_Instance * p_inst = p_1;
    /* NOTE: smf_set_initial() executes the entry state. */
    struct smf_ctx * p_sm = &p_inst->sm;
    smf_set_initial(SMF_CTX(p_sm), &states[init]);

    /* Get the state machine event queue and point to where to put the dequeued
     * event. */
    struct k_msgq * p_msgq = p_inst->msgq.p_sm_evts;
    struct Codec_SM_Evt * p_evt = &p_inst->sm_evt;

    bool run = true;

    while(run){
        /* Wait on state machine event. Cache it then run state machine. */
        k_msgq_get(p_msgq, p_evt, K_FOREVER);
        run = smf_run_state(SMF_CTX(p_sm)) == 0;
    }

    #if CONFIG_FKMG_CODEC_ALLOW_SHUTDOWN
    /* We're shutting down. Schedule a work queue event to check that the
     * thread exited and call back anything. */
    if(should_callback_on_exit(p_inst)){
        k_work_init_delayable( &p_inst->work, on_thread_shutdown);
        k_work_schedule(&p_inst->work, K_MSEC(1));
    }
    #endif
}

static void start_thread(
        struct Codec_Instance     * p_inst,
        struct Codec_Instance_Cfg * p_cfg)
{
    struct k_thread  * p_thread = p_cfg->task.sm.p_thread;
    k_thread_stack_t * p_stack  = p_cfg->task.sm.p_stack;
    size_t stack_sz_bytes = p_cfg->task.sm.stack_sz;
    int prio = p_cfg->task.sm.prio;

    /* Start the state machine thread. */
    p_inst->task.sm.p_thread = k_thread_create(
        p_thread,       // ptr to uninitialized struct k_thread
        p_stack,        // ptr to the stack space
        stack_sz_bytes, // stack size in bytes
        thread,         // thread entry function
        p_inst,         // 1st entry point parameter
        NULL,           // 2nd entry point parameter
        NULL,           // 3rd entry point parameter
        prio,           // thread priority 
        0,              // thread options
        K_NO_WAIT);     // scheduling delay
    assert(p_inst->task.sm.p_thread == p_thread);
}

/* *****************************************************************************
 * Public
 */

void Codec_Init_Instance(struct Codec_Instance_Cfg * p_cfg)
{
    /* Get pointer to instance to configure. */
    struct Codec_Instance * p_inst = p_cfg->p_inst;

    #if CONFIG_FKMG_CODEC_RUNTIME_ERROR_CHECKING
    /* Check instance configuration for errors. */
    if(errored(p_inst, check_instance_cfg_param_for_errors(p_cfg))){
        assert(false);
        return;
    }
    #endif


    init_module();
    init_instance(p_inst);
    config_instance_immediate(p_inst, p_cfg);
    add_instance_to_instances(p_inst);
    start_thread(p_inst, p_cfg);
    q_init_instance_event(p_cfg);

}

#if CONFIG_FKMG_CODEC_ALLOW_SHUTDOWN
void Codec_Deinit_Instance(struct Codec_Instance_Dcfg * p_dcfg)
{
    #error "Not implemented yet!"
}
#endif

void Codec_Add_Listener(struct Codec_Listener_Cfg * p_cfg)
{
    #if CONFIG_FKMG_CODEC_RUNTIME_ERROR_CHECKING
    /* Get pointer to instance. */
    struct Codec_Instance * p_inst = p_cfg->p_inst;

    /* Check listener instance configuration for errors. */
    if(errored(p_inst, check_listener_cfg_param_for_errors(p_cfg))){
        assert(false);
        return;
    }
    #endif

    struct Codec_Listener * p_lsnr = p_cfg->p_lsnr;
    init_listener(p_lsnr);
    config_listener(p_lsnr, p_cfg);
    add_listener_for_signal_to_listener_list(p_cfg);
}


#if CONFIG_FKMG_CODEC_ALLOW_LISTENER_REMOVAL
void CodecRemove_Listener(struct Codec_Listener * p_lsnr)
{
    #error "Not implemented yet!"
}
#endif

#if CONFIG_FKMG_CODEC_NO_OPTIMIZATIONS
#pragma GCC pop_options
#endif
