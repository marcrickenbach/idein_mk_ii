/* *****************************************************************************
 * @brief Idein implementation.
 * Oversees the operation of the entire application. 
 */

/* *****************************************************************************
 * TODO
 * 
 */

/* *****************************************************************************
 * Includes
 */

#include "idein.h"

#include <zephyr/kernel.h>
#include <zephyr/smf.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/gpio.h>

#include <assert.h>

#include "idein/private/sm_evt.h"
#include "idein/private/module_data.h"

#include "pot.h"
#include "pot/evt.h"

#include "codec.h"
#include "codec/evt.h"

#include "midi.h"
#include "midi/evt.h"

/* *****************************************************************************
 * Constants, Defines, and Macros
 */

#define SUCCESS             (0)
#define FAIL                (-1)

#define OVERRIDE            true
#define NO_OVERRIDE         false

#define TIMER_X             DT_INST(0, st_stm32_counter) 
#define TIMER_Y             DT_INST(1, st_stm32_counter)

#define RED_POT         0
#define GREEN_POT       1
#define BLUE_POT        2
#define IR_POT          3
#define VOL_POT         4

#define GPIO_PINS   DT_PATH(zephyr_user)


/* *****************************************************************************
 * Debugging
 */

#if CONFIG_FKMG_IDEIN_NO_OPTIMIZATIONS
#pragma GCC push_options
#pragma GCC optimize ("Og")
#warning "idein.c unoptimized!"
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(idein, CONFIG_FKMG_IDEIN_LOG_LEVEL);


uint8_t note_data_raw = 0; 

/* *****************************************************************************
 * Structs
 */

static struct idein_module_data idein_md = {0};
/* Convenience accessor to keep name short: md - module data. */
#define md idein_md



/* *****************************************************************************
 * Device Structs
 */


static const struct gpio_dt_spec clock_in = GPIO_DT_SPEC_GET(GPIO_PINS, ext_clk_in_gpios);
static const struct gpio_dt_spec clock_out = GPIO_DT_SPEC_GET(GPIO_PINS, int_clk_out_gpios);

static void on_clock_in(const struct device *dev, struct gpio_callback *cb, uint32_t pins);

/* *****************************************************************************
 * Private
 */

/* *****
 * Utils
 * *****/

static bool instance_contains_state_machine_ctx(
        struct Idein_Instance * p_inst,
        struct smf_ctx      * p_sm_ctx)
{
    return(&p_inst->sm == p_sm_ctx);
}

/* Find the instance that contains the state machine context. */
static struct Idein_Instance * sm_ctx_to_instance(struct smf_ctx * p_sm_ctx)
{
    /* Iterate thru the instances. */
    struct Idein_Instance * p_inst = NULL;
    SYS_SLIST_FOR_EACH_CONTAINER(&md.list.instances, p_inst, node.instance){
        if(instance_contains_state_machine_ctx(p_inst, p_sm_ctx)) return p_inst;
    }
    return(NULL);
}



/* **************
 * Listener Utils
 * **************/

#if CONFIG_FKMG_IDEIN_RUNTIME_ERROR_CHECKING
static enum Idein_Err_Id check_listener_cfg_param_for_errors(
        struct Idein_Listener_Cfg * p_cfg )
{
    if(!p_cfg
    || !p_cfg->p_iface
    || !p_cfg->p_lsnr) return(k_Idein_Err_Id_Configuration_Invalid);

    /* Is signal valid? */
    if(p_cfg->sig > k_Idein_Sig_Max)
        return(k_Idein_Err_Id_Configuration_Invalid);

    /* Is callback valid? */
    if(!p_cfg->cb)
        return(k_Idein_Err_Id_Configuration_Invalid);

    return(k_Idein_Err_Id_None);
}
#endif

static void clear_listener(struct Idein_Listener * p_lsnr)
{
    memset(p_lsnr, 0, sizeof(*p_lsnr));
}

static void config_listener(
        struct Idein_Listener     * p_lsnr,
        struct Idein_Listener_Cfg * p_cfg)
{
    /* Set listner's instance it is listening to. */
    p_lsnr->p_inst = p_cfg->p_inst;

    /* Set listner's callback. */ 
    p_lsnr->cb = p_cfg->cb;
}

static void init_listener(struct Idein_Listener * p_lsnr)
{
    clear_listener(p_lsnr);
}

/* **************
 * Instance Utils
 * **************/

#if CONFIG_FKMG_IDEIN_RUNTIME_ERROR_CHECKING
static enum Idein_Err_Id check_instance_cfg_param_for_errors(
        struct Idein_Instance_Cfg * p_cfg)
{
    if(!p_cfg
    || !p_cfg->p_inst
    || !p_cfg->task.sm.p_thread
    || !p_cfg->task.sm.p_stack
    || !p_cfg->msgq.p_sm_evts) return(k_Idein_Err_Id_Configuration_Invalid);

    if(p_cfg->task.sm.stack_sz == 0) return(k_Idein_Err_Id_Configuration_Invalid);

    return(k_Idein_Err_Id_None);
}
#endif

static void add_instance_to_instances(
        struct Idein_Instance  * p_inst)
{
    sys_slist_append(&md.list.instances, &p_inst->node.instance);
}

static void config_instance_queues(
        struct Idein_Instance     * p_inst,
        struct Idein_Instance_Cfg * p_cfg)
{
    p_inst->msgq.p_sm_evts = p_cfg->msgq.p_sm_evts;
}

/* Forward reference */

static void timer_x_callback (const struct device *timer_dev,
                              uint8_t chan_id, uint32_t ticks,
                              void *user_data); 

/* Set up hardware timers for main sequencers. */
static void init_hardware_timers(struct Idein_Instance * p_inst,
                                 struct Idein_Instance_Cfg * p_cfg)
{
    p_inst->timer.t[0] = DEVICE_DT_GET(TIMER_X);

    counter_alarm_callback_t cb_cfg[1] = {
        timer_x_callback,
    }; 

    p_inst->tim_cb[0] = cb_cfg[0];

    if (!device_is_ready(p_inst->timer.t[0])) 
    {
        printk("Timer Config: FAILED.\n");
        return;
    } else {
        printk("Timer Config: PASSED\n");
    }
    

    struct counter_alarm_cfg x_cfg = {
        .flags = 0,
        .ticks = 1000,
        .callback = p_inst->tim_cb[0],
        .user_data = p_inst
    }; 
    
    if (counter_set_channel_alarm(p_inst->timer.t[0], 0, &x_cfg) != 0) {
        // LOG_ERR("Error: Failed to set Timer X"); 
    };

    return; 

}





static void config_instance_deferred(
        struct Idein_Instance     * p_inst,
        struct Idein_Instance_Cfg * p_cfg)
{

}


/* Since configuration starts on caller's thread, configure fields that require
 * immediate and/or inconsequential configuration and defer rest to be handled
 * by our own thread later. */
static void config_instance_immediate(
        struct Idein_Instance     * p_inst,
        struct Idein_Instance_Cfg * p_cfg)
{
    config_instance_queues(p_inst, p_cfg);
}

static void init_instance_lists(struct Idein_Instance * p_inst)
{
    for(enum Idein_Evt_Sig sig = k_Idein_Evt_Sig_Beg;
                         sig < k_Idein_Evt_Sig_End;
                         sig++){
        sys_slist_init(&p_inst->list.listeners[sig]);
    }
}

static void clear_instance(struct Idein_Instance * p_inst)
{
    memset(p_inst, 0, sizeof(*p_inst));
}

static void init_instance(struct Idein_Instance * p_inst)
{
    clear_instance(p_inst);
    init_instance_lists(p_inst);
    #if CONFIG_FKMG_IDEIN_RUNTIME_ERROR_CHECKING
    p_inst->err = k_Idein_Err_Id_None;
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

#if CONFIG_FKMG_IDEIN_RUNTIME_ERROR_CHECKING
static void set_error(
        struct Idein_Instance * p_inst,
        enum Idein_Err_Id       err,
        bool                  override)
{
    if(p_inst){
        if((override                          )
        || (p_inst->err == k_Idein_Err_Id_None )){
            p_inst->err = err;
        }
    }
}

static bool errored(
        struct Idein_Instance * p_inst,
        enum Idein_Err_Id       err )
{
    set_error(p_inst, err, NO_OVERRIDE);
    return(err != k_Idein_Err_Id_None);
}
#endif /* CONFIG_FKMG_IDEIN_RUNTIME_ERROR_CHECKING */

/* ************
 * Broadcasting
 * ************/

static void broadcast(
        struct Idein_Evt      * p_evt,
        struct Idein_Listener * p_lsnr)
{
    /* call the listener, passing the event */
    if(p_lsnr->cb) p_lsnr->cb(p_evt);
}

static void broadcast_event_to_listeners(
        struct Idein_Instance * p_inst,
        struct Idein_Evt      * p_evt)
{
    enum Idein_Evt_Sig sig = p_evt->sig;
    struct Idein_Listener * p_lsnr = NULL;
    SYS_SLIST_FOR_EACH_CONTAINER(&p_inst->list.listeners[sig], p_lsnr, node.listener){
        broadcast(p_evt, p_lsnr);
    }
}

/* There's only 1 listener for instance initialization, and it is provided in
 * the cfg struct. */
static void broadcast_instance_initialized(
        struct Idein_Instance * p_inst,
        Idein_Listener_Cb       cb)
{
    struct Idein_Evt evt = {
            .sig = k_Idein_Evt_Sig_Instance_Initialized,
            .data.initd.p_inst = p_inst
    };

    struct Idein_Listener lsnr = {
        .p_inst = p_inst,
        .cb = cb
    };

    broadcast(&evt, &lsnr);
}

#if CONFIG_FKMG_IDEIN_ALLOW_SHUTDOWN
static void broadcast_interface_deinitialized(
        struct Idein_Instance * p_inst,
        Idein_Listener_Cb       cb)
{
    struct Idein_Evt evt = {
            .sig = k_Idein_Instance_Deinitialized,
            .data.inst_deinit.p_inst = p_inst
    };

    struct Idein_Listener lsnr = {
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
    struct Idein_Listener_Cfg * p_cfg)
{
    /* Get pointer to interface to add listener to. */
    struct Idein_Instance * p_inst = p_cfg->p_inst;

    /* Get pointer to configured listener. */
    struct Idein_Listener * p_lsnr = p_cfg->p_lsnr;

    /* Get signal to listen for. */
    enum Idein_Evt_Sig sig = p_cfg->sig;

    /* Add listener to instance's specified signal. */
    sys_slist_t * p_list = &p_inst->list.listeners[sig];
    sys_snode_t * p_node = &p_lsnr->node.listener;
    sys_slist_append(p_list, p_node);
}

#if CONFIG_FKMG_IDEIN_ALLOW_LISTENER_REMOVAL
static bool find_list_containing_listener_and_remove_listener(
    struct Idein_Instance * p_inst,
	struct Idein_Listener * p_lsnr)
{
    for(enum Idein_Evt_Sig sig = k_Idein_Evt_Sig_Beg;
                         sig < k_Idein_Evt_Sig_End;
                         sig++){
        bool found_and_removed = sys_slist_find_and_remove(
                &p_inst->list.listeners[sig], &p_lsnr->node);
        if(found_and_removed) return(true);
    }
    return( false );
}
#endif

// static bool signal_has_listeners(
//         struct Idein_Instance * p_inst,
//         enum Idein_Evt_Sig      sig)
// {
//     return(!sys_slist_is_empty(&p_inst->list.listeners[sig]));
// }

/* **************
 * Event Queueing
 * **************/

static void q_sm_event(
        struct Idein_Instance * p_inst, struct Idein_SM_Evt * p_evt)
{
    bool queued = k_msgq_put(p_inst->msgq.p_sm_evts, p_evt, K_NO_WAIT) == 0;

    if(!queued) assert(false);
}

static void q_init_instance_event(struct Idein_Instance_Cfg * p_cfg)
{
    struct Idein_SM_Evt evt = {
            .sig = k_Idein_SM_Evt_Sig_Init_Instance,
            .data.init_inst.cfg = *p_cfg
    };
    struct Idein_Instance * p_inst = p_cfg->p_inst;
    q_sm_event(p_inst, &evt);
}


static void q_timer_elapsed (struct Idein_Instance * p_inst, enum Idein_Id id)
{
    struct Idein_SM_Evt evt = {
        .sig = k_Idein_SM_Evt_Timer_Elapsed,
    };
    q_sm_event(p_inst, &evt);

}


static void timer_x_callback (const struct device *timer_dev,
                              uint8_t chan_id, uint32_t ticks,
                              void *user_data) {

    struct Idein_Instance *p_inst = (struct Idein_Instance *)user_data; 

    q_timer_elapsed(p_inst, k_Idein_Id_1); 

}


/* **********
 * Listener Callbacks
 * **********/


/* **********
 * SM Functions
 * **********/


static void post_updated_pot_value(struct Idein_Instance * p_inst, enum Pot_Id id, uint16_t val) 
{

}


static void clock_gpio_pin_config(void) {
    
    int ret;
    
    if (    !device_is_ready(clock_in.port)
        ||  !device_is_ready(clock_out.port)) {
        ret = 1;
    }
    ret = gpio_pin_configure(clock_in.port, clock_in.pin, GPIO_INPUT | GPIO_PULL_DOWN);
    
    if (ret < 0) {
        LOG_ERR("Error: Failed to configure Clock In pin");
    }

    ret = gpio_pin_configure_dt(&clock_out, GPIO_OUTPUT_LOW);
    
    if (ret < 0) {
        LOG_ERR("Error: Failed to configure Clock Out pin");
    }

}

static void clock_gpio_interrupt_config(void)
{
    int ret;
    
    ret = gpio_pin_interrupt_configure(clock_in.port, clock_in.pin, GPIO_INT_EDGE_TO_ACTIVE);

    if (ret < 0) {
        LOG_ERR("Error: Failed to configure interrupt on Clock In pin");
    }

}


static void clock_gpio_callback_config(struct Idein_Instance * p_inst)
{
    int ret; 

    gpio_init_callback(&p_inst->clock_gpio_cb, on_clock_in, BIT(clock_in.pin));

    ret = gpio_add_callback(clock_in.port, &p_inst->clock_gpio_cb);

    if (ret < 0) {
        LOG_ERR("Error: Failed to add callback on resetB pin");
    }
}


static void clock_gpio_init(struct Idein_Instance * p_inst) {
    
    int ret = 0; 

    if (    !device_is_ready(clock_in.port)
        ||  !device_is_ready(clock_out.port)) {
        ret = 1;
    }

    assert(ret == 0);
    clock_gpio_pin_config();
    clock_gpio_interrupt_config();
    clock_gpio_callback_config(p_inst);

}


static void on_clock_in(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    /* Broadcast clock hit to voice and usb listeners */
    
    struct Idein_Instance * p_inst =
            CONTAINER_OF(cb, struct Idein_Instance, clock_gpio_cb);
    
    struct Idein_Evt evt = {
        .sig = k_Idein_Evt_Sig_Clock_Event,
        .data.note.note = note_data_raw
    };
    broadcast_event_to_listeners(p_inst, &evt);
}


static void comparator_circuit(struct Idein_Instance *p_inst, uint32_t *read_values, size_t size) {

/* FIXME: I'd like to work on the resolution here. If we're only flipping 4 different bits, this greatly limits the possibility of which note gets hit. Come up with a scheme to make this more varied. */
    for (int i = 0; i < size; i++) {
        if (read_values[i] > p_inst->idein.threshold[i]) {
            note_data_raw ^= (1 << i);
        }
    }

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
 *                     |* ->run            
 *                     |all other signals  |
 *                     |not allowed; assert|
 *
 *  State run:
 *
 *  |entry             |  |run             |  |exit           |
 *  |------------------|  |----------------|  |---------------|
 *  |* start conversion|  |convert:        |  |not implemented|
 *  |  timer           |  |* timer elapsed |
 *                        |* pot changed   |
 *                        |* btn changed   |
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
    run,    // Run - handles all events while running (e.g. conversion, etc.)
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
    struct Idein_Instance * p_inst = sm_ctx_to_instance(p_sm);

    /* Get the event. */
    struct Idein_SM_Evt * p_evt = &p_inst->sm_evt;

    /* Expecting only an "init instance" event. Anything else is an error. */
    assert(p_evt->sig == k_Idein_SM_Evt_Sig_Init_Instance);

    /* We init'd required params on the caller's thread (i.e.
     * Idein_Init_Instance()), now finish the job. Since this is an
     * Init_Instance event the data contains the intance cfg. */
    struct Idein_SM_Evt_Sig_Init_Instance * p_ii = &p_evt->data.init_inst;
    
    config_instance_deferred(p_inst, &p_ii->cfg);
    broadcast_instance_initialized(p_inst, p_ii->cfg.cb);  
    smf_set_state(SMF_CTX(p_sm), &states[run]);
}

/* Run state responsibility is to be the root state to handle everything else
 * other than instance initialization. */

static void state_run_entry(void * o)
{
    struct smf_ctx * p_sm = o;
    struct Idein_Instance * p_inst = sm_ctx_to_instance(p_sm);
}

static void state_run_run(void * o)
{
    struct smf_ctx * p_sm = o;
    struct Idein_Instance * p_inst = sm_ctx_to_instance(p_sm);

    /* Get the event. */
    struct Idein_SM_Evt * p_evt = &p_inst->sm_evt;

    switch(p_evt->sig){
        default: break;
        case k_Idein_SM_Evt_Sig_Init_Instance:
            /* Should never occur. */
            assert(false);
            break;
        case k_Idein_SM_Evt_Timer_Elapsed:
           
            break; 

        case k_Idein_SM_Evt_Sig_Sensor_Read:
            
            uint32_t read_values[4];
            memcpy(read_values, p_evt->data.sensor_read.read_values, sizeof(p_evt->data.sensor_read.read_values));

            comparator_circuit(p_inst, read_values, (sizeof(read_values)/sizeof(read_values[0])));

            break;

        case k_Idein_SM_Evt_Sig_Pot_Value_Changed:
            
            for (int i = 0; i < 4; i++) {
                p_inst->idein.threshold[i] = p_evt->data.pot_changed.val[i];
            }

            p_inst->idein.volume = p_evt->data.pot_changed.val[VOL_POT];

            break;
        case k_Idein_SM_Evt_Sig_Button_Pressed:

            break; 
        #if CONFIG_FKMG_IDEIN_SHUTDOWN_ENABLED
        case k_Idein_Evt_Sig_Instance_Deinitialized:
            assert(false);
            break;
        #endif
    }
}

/* Deinit state responsibility is to clean up before exiting thread. */
#if CONFIG_FKMG_IDEIN_SHUTDOWN_ENABLED
static void state_deinit_run(void * o)
{
    struct smf_ctx * p_sm = o;
    struct Idein_Instance * p_inst = sm_ctx_to_instance(p_sm);

    /* Get the event. */
    struct Idein_SM_Evt * p_evt = &p_inst->sm_evt;

    /* TODO */
}
#endif

static const struct smf_state states[] = {
    /*                                      entry               run  exit */
    [  init] = SMF_CREATE_STATE(           NULL,   state_init_run, NULL),
    [   run] = SMF_CREATE_STATE(state_run_entry,    state_run_run, NULL),
    #if CONFIG_FKMG_IDEIN_SHUTDOWN_ENABLED
    [deinit] = SMF_CREATE_STATE(           NULL, state_deinit_run, NULL),
    #endif
};

/* ******
 * Thread
 * ******/

#if CONFIG_FKMG_IDEIN_ALLOW_SHUTDOWN
/* Since there is only the "join" facility to know when a thread is shut down,
 * and that isn't appropriate to use since it will put the calling thread to
 * sleep until the other thread is shut down, we set up a delayable system work
 * queue event to check that the thread is shut down and then call any callback
 * that is waiting to be notified. */
void on_thread_shutdown(struct k_work *item)
{
    struct Idein_Instance * p_inst =
            CONTAINER_OF(item, struct Idein_Instance, work);

    char * thread_state_str = "dead";
    k_thread_state_str(&p_inst->thread, thread_state_str, sizeof(thread_state_str));
    bool shut_down = strcmp( thread_state_str, "dead" ) == 0;

    if(!shut_down) k_work_reschedule(&p_inst->work, K_MSEC(1));
    else broadcast_instance_deinitialized(p_inst);
}
#endif

static void thread(void * p_1, /* struct Idein_Instance* */
        void * p_2_unused, void * p_3_unused)
{
    struct Idein_Instance * p_inst = p_1;
    /* NOTE: smf_set_initial() executes the entry state. */
    struct smf_ctx * p_sm = &p_inst->sm;
    smf_set_initial(SMF_CTX(p_sm), &states[init]);

    /* Get the state machine event queue and point to where to put the dequeued
     * event. */
    struct k_msgq * p_msgq = p_inst->msgq.p_sm_evts;
    struct Idein_SM_Evt * p_evt = &p_inst->sm_evt;

    bool run = true;

    while(run){
        /* Wait on state machine event. Cache it then run state machine. */
        k_msgq_get(p_msgq, p_evt, K_FOREVER);
        run = smf_run_state(SMF_CTX(p_sm)) == 0;
    }

    #if CONFIG_FKMG_IDEIN_ALLOW_SHUTDOWN
    /* We're shutting down. Schedule a work queue event to check that the
     * thread exited and call back anything. */
    if(should_callback_on_exit(p_inst)){
        k_work_init_delayable( &p_inst->work, on_thread_shutdown);
        k_work_schedule(&p_inst->work, K_MSEC(1));
    }
    #endif
}

static void start_thread(
        struct Idein_Instance     * p_inst,
        struct Idein_Instance_Cfg * p_cfg)
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

void Idein_Init_Instance(struct Idein_Instance_Cfg * p_cfg)
{
    /* Get pointer to instance to configure. */
    struct Idein_Instance * p_inst = p_cfg->p_inst;

    #if CONFIG_FKMG_IDEIN_RUNTIME_ERROR_CHECKING
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

#if CONFIG_FKMG_IDEIN_ALLOW_SHUTDOWN
void Idein_Deinit_Instance(struct Idein_Instance_Dcfg * p_dcfg)
{
    #error "Not implemented yet!"
}
#endif

void Idein_Add_Listener(struct Idein_Listener_Cfg * p_cfg)
{
    #if CONFIG_FKMG_IDEIN_RUNTIME_ERROR_CHECKING
    /* Get pointer to instance. */
    struct Idein_Instance * p_inst = p_cfg->p_inst;

    /* Check listener instance configuration for errors. */
    if(errored(p_inst, check_listener_cfg_param_for_errors(p_cfg))){
        assert(false);
        return;
    }
    #endif

    struct Idein_Listener * p_lsnr = p_cfg->p_lsnr;
    init_listener(p_lsnr);
    config_listener(p_lsnr, p_cfg);
    add_listener_for_signal_to_listener_list(p_cfg);
}

#if CONFIG_FKMG_IDEIN_ALLOW_LISTENER_REMOVAL
void Idein_Remove_Listener(struct Idein_Listener * p_lsnr)
{ 
    #error "Not implemented yet!"
}
#endif

#if CONFIG_FKMG_IDEIN_NO_OPTIMIZATIONS
#pragma GCC pop_options
#endif
