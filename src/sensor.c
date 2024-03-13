/* *****************************************************************************
 * @brief Sensor implementation.
 * Driver for APDS-9253 Light Sensor. Readings occur every 100 ms. 
 * Upon successful read, the sensor data is broadcast to listeners. 
 */

/* *****************************************************************************
 * TODO
    
 */

/* *****************************************************************************
 * Includes
 */

#include "sensor.h"

#include <zephyr/smf.h>

#include <zephyr/device.h>

#include <assert.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/dma.h>

#include "sensor/private/sm_evt.h"
#include "sensor/private/module_data.h"

/* *****************************************************************************
 * Constants, Defines, and Macros
 */

#define SUCCESS (0)
#define FAIL    (-1)

#define OVERRIDE    true
#define NO_OVERRIDE false

#define I2C1_NODE           DT_NODELABEL(i2c1)

#define LIGHT_RESOLUTION	0

/* *****************************************************************************
 * Debugging
 */

#if CONFIG_FKMG_SENSOR_NO_OPTIMIZATIONS
#pragma GCC push_options
#pragma GCC optimize ("Og")
#warning "sensor.c unoptimized!"
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sensor, CONFIG_FKMG_SENSOR_LOG_LEVEL);

/* *****************************************************************************
 * Structs
 */

static struct sensor_module_data sensor_md = {0};
/* Convenience accessor to keep name short: md - module data. */
#define md sensor_md

const struct device * i2c_dev = DEVICE_DT_GET(I2C1_NODE);

// CHIP ADDRESSES
const uint16_t RGB_ADDRESS = 0x52;

// CONFIG ADDRESSES
const uint16_t MAIN_CTRL = 0x00;
const uint16_t LS_MEAS_RATE = 0x04;
const uint16_t LS_GAIN = 0x05;

// COLOR REGISTERS
uint8_t RED_REGISTER = 0x13;
uint8_t GREEN_REGISTER = 0xD;
uint8_t BLUE_REGISTER = 0x10;
uint8_t IR_REGISTER = 0xA;

// COMMANDS
uint8_t RGB_CONFIG = 0x06;


// READ DATA
// raw data
uint8_t red_val[3];
uint8_t blue_val[3];
uint8_t green_val[3];
uint8_t ir_val[3];

// compiled data
uint32_t RED_TOTAL = 0;
uint32_t GREEN_TOTAL = 0;
uint32_t BLUE_TOTAL = 0;
uint32_t IR_TOTAL = 0;


/* *****************************************************************************
 * Private
 */

/* *****
 * Utils
 * *****/

static bool instance_contains_state_machine_ctx(
        struct Sensor_Instance * p_inst,
        struct smf_ctx      * p_sm_ctx)
{
    return(&p_inst->sm == p_sm_ctx);
}

/* Find the instance that contains the state machine context. */
static struct Sensor_Instance * sm_ctx_to_instance(struct smf_ctx * p_sm_ctx)
{
    /* Iterate thru the instances. */
    struct Sensor_Instance * p_inst = NULL;
    SYS_SLIST_FOR_EACH_CONTAINER(&md.list.instances, p_inst, node.instance){
        if(instance_contains_state_machine_ctx(p_inst, p_sm_ctx)) return p_inst;
    }
    return(NULL);
}



/* **************
 * Listener Utils
 * **************/

#if CONFIG_FKMG_SENSOR_RUNTIME_ERROR_CHECKING
static enum Sensor_Err_Id check_listener_cfg_param_for_errors(
        struct Sensor_Listener_Cfg * p_cfg )
{
    if(!p_cfg
    || !p_cfg->p_iface
    || !p_cfg->p_lsnr) return(k_Sensor_Err_Id_Configuration_Invalid);

    /* Is signal valid? */
    if(p_cfg->sig > k_Sensor_Sig_Max)
        return(k_Sensor_Err_Id_Configuration_Invalid);

    /* Is callback valid? */
    if(!p_cfg->cb)
        return(k_Sensor_Err_Id_Configuration_Invalid);

    return(k_Sensor_Err_Id_None);
}
#endif

static void clear_listener(struct Sensor_Listener * p_lsnr)
{
    memset(p_lsnr, 0, sizeof(*p_lsnr));
}

static void config_listener(
        struct Sensor_Listener     * p_lsnr,
        struct Sensor_Listener_Cfg * p_cfg)
{
    /* Set listner's instance it is listening to. */
    p_lsnr->p_inst = p_cfg->p_inst;

    /* Set listner's callback. */ 
    p_lsnr->cb = p_cfg->cb;
}

static void init_listener(struct Sensor_Listener * p_lsnr)
{
    clear_listener(p_lsnr);
}

/* **************
 * Instance Utils
 * **************/

#if CONFIG_FKMG_SENSOR_RUNTIME_ERROR_CHECKING
static enum Sensor_Err_Id check_instance_cfg_param_for_errors(
        struct Sensor_Instance_Cfg * p_cfg)
{
    if(!p_cfg
    || !p_cfg->p_inst
    || !p_cfg->task.sm.p_thread
    || !p_cfg->task.sm.p_stack
    || !p_cfg->msgq.p_sm_evts) return(k_Sensor_Err_Id_Configuration_Invalid);

    if(p_cfg->task.sm.stack_sz == 0) return(k_Sensor_Err_Id_Configuration_Invalid);

    return(k_Sensor_Err_Id_None);
}
#endif

static void add_instance_to_instances(
        struct Sensor_Instance  * p_inst)
{
    sys_slist_append(&md.list.instances, &p_inst->node.instance);
}

static void config_instance_queues(
        struct Sensor_Instance     * p_inst,
        struct Sensor_Instance_Cfg * p_cfg)
{
    p_inst->msgq.p_sm_evts = p_cfg->msgq.p_sm_evts;
}

/* Forward reference */
static void on_sensor_timer_expiry(struct k_timer * p_timer);
/* Cause conversion event to occur immediately and then regularly after that. */
static void init_sensor_timer(struct Sensor_Instance * p_inst)
{
    #define ON_SENSOR_TIMER_EXPIRY  on_sensor_timer_expiry
    #define ON_SENSOR_TIMER_STOPPED NULL
    k_timer_init(&p_inst->timer.sensor, ON_SENSOR_TIMER_EXPIRY,
            ON_SENSOR_TIMER_STOPPED);
}

static void config_instance_deferred(
        struct Sensor_Instance     * p_inst,
        struct Sensor_Instance_Cfg * p_cfg)
{

    init_sensor_timer(p_inst);

}

/* Since configuration starts on caller's thread, configure fields that require
 * immediate and/or inconsequential configuration and defer rest to be handled
 * by our own thread later. */
static void config_instance_immediate(
        struct Sensor_Instance     * p_inst,
        struct Sensor_Instance_Cfg * p_cfg)
{
    config_instance_queues(p_inst, p_cfg);
}

static void init_instance_lists(struct Sensor_Instance * p_inst)
{
    for(enum Sensor_Evt_Sig sig = k_Sensor_Evt_Sig_Beg;
                         sig < k_Sensor_Evt_Sig_End;
                         sig++){
        sys_slist_init(&p_inst->list.listeners[sig]);
    }
}

static void clear_instance(struct Sensor_Instance * p_inst)
{
    memset(p_inst, 0, sizeof(*p_inst));
}

static void init_instance(struct Sensor_Instance * p_inst)
{
    clear_instance(p_inst);
    init_instance_lists(p_inst);
    #if CONFIG_FKMG_SENSOR_RUNTIME_ERROR_CHECKING
    p_inst->err = k_Sensor_Err_Id_None;
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

#if CONFIG_FKMG_SENSOR_RUNTIME_ERROR_CHECKING
static void set_error(
        struct Sensor_Instance * p_inst,
        enum Sensor_Err_Id       err,
        bool                  override)
{
    if(p_inst){
        if((override                          )
        || (p_inst->err == k_Sensor_Err_Id_None )){
            p_inst->err = err;
        }
    }
}

static bool errored(
        struct Sensor_Instance * p_inst,
        enum Sensor_Err_Id       err )
{
    set_error(p_inst, err, NO_OVERRIDE);
    return(err != k_Sensor_Err_Id_None);
}
#endif /* CONFIG_FKMG_SENSOR_RUNTIME_ERROR_CHECKING */

/* ************
 * Broadcasting
 * ************/

static void broadcast(
        struct Sensor_Evt      * p_evt,
        struct Sensor_Listener * p_lsnr)
{
    /* call the listener, passing the event */
    if(p_lsnr->cb) p_lsnr->cb(p_evt);
}

static void broadcast_event_to_listeners(
        struct Sensor_Instance * p_inst,
        struct Sensor_Evt      * p_evt)
{
    enum Sensor_Evt_Sig sig = p_evt->sig;
    struct Sensor_Listener * p_lsnr = NULL;
    SYS_SLIST_FOR_EACH_CONTAINER(&p_inst->list.listeners[sig], p_lsnr, node.listener){
        broadcast(p_evt, p_lsnr);
    }
}

/* There's only 1 listener for instance initialization, and it is provided in
 * the cfg struct. */
static void broadcast_instance_initialized(
        struct Sensor_Instance * p_inst,
        Sensor_Listener_Cb       cb)
{
    struct Sensor_Evt evt = {
            .sig = k_Sensor_Evt_Sig_Instance_Initialized,
            .data.initd.p_inst = p_inst
    };

    struct Sensor_Listener lsnr = {
        .p_inst = p_inst,
        .cb = cb
    };

    broadcast(&evt, &lsnr);
}

#if CONFIG_FKMG_SENSOR_ALLOW_SHUTDOWN
static void broadcast_interface_deinitialized(
        struct Sensor_Instance * p_inst,
        Sensor_Listener_Cb       cb)
{
    struct Sensor_Evt evt = {
            .sig = k_Sensor_Instance_Deinitialized,
            .data.inst_deinit.p_inst = p_inst
    };

    struct Sensor_Listener lsnr = {
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
    struct Sensor_Listener_Cfg * p_cfg)
{
    /* Get pointer to interface to add listener to. */
    struct Sensor_Instance * p_inst = p_cfg->p_inst;

    /* Get pointer to configured listener. */
    struct Sensor_Listener * p_lsnr = p_cfg->p_lsnr;

    /* Get signal to listen for. */
    enum Sensor_Evt_Sig sig = p_cfg->sig;

    /* Add listener to instance's specified signal. */
    sys_slist_t * p_list = &p_inst->list.listeners[sig];
    sys_snode_t * p_node = &p_lsnr->node.listener;
    sys_slist_append(p_list, p_node);
}

#if CONFIG_FKMG_SENSOR_ALLOW_LISTENER_REMOVAL
static bool find_list_containing_listener_and_remove_listener(
    struct Sensor_Instance * p_inst,
	struct Sensor_Listener * p_lsnr)
{
    for(enum Sensor_Evt_Sig sig = k_Sensor_Evt_Sig_Beg;
                         sig < k_Sensor_Evt_Sig_End;
                         sig++){
        bool found_and_removed = sys_slist_find_and_remove(
                &p_inst->list.listeners[sig], &p_lsnr->node);
        if(found_and_removed) return(true);
    }
    return( false );
}
#endif

// static bool signal_has_listeners(
//         struct Sensor_Instance * p_inst,
//         enum Sensor_Evt_Sig      sig)
// {
//     return(!sys_slist_is_empty(&p_inst->list.listeners[sig]));
// }

/* **************
 * Event Queueing
 * **************/

static void q_sm_event(
        struct Sensor_Instance * p_inst, struct Sensor_SM_Evt * p_evt)
{
    bool queued = k_msgq_put(p_inst->msgq.p_sm_evts, p_evt, K_NO_WAIT) == 0;

    if(!queued) assert(false);
}

static void q_init_instance_event(struct Sensor_Instance_Cfg * p_cfg)
{
    struct Sensor_SM_Evt evt = {
            .sig = k_Sensor_SM_Evt_Sig_Init_Instance,
            .data.init_inst.cfg = *p_cfg
    };
    struct Sensor_Instance * p_inst = p_cfg->p_inst;
    q_sm_event(p_inst, &evt);
}

static void on_sensor_timer_expiry(struct k_timer * p_timer)
{
    struct Sensor_Instance * p_inst =
        CONTAINER_OF(p_timer, struct Sensor_Instance, timer.sensor);

    struct Sensor_SM_Evt evt = {
        .sig = k_Sensor_SM_Evt_Sig_Read
    };

    q_sm_event(p_inst, &evt);
}

/* Cause conversion event to occur immediately and then regularly after that. */
static void start_sensor_timer(struct Sensor_Instance * p_inst)
{
    #define CONVERSION_INITIAL_DURATION     K_NO_WAIT
    #define CONVERSION_AUTO_RELOAD_PERIOD   K_MSEC(100)
    k_timer_start(&p_inst->timer.sensor, CONVERSION_INITIAL_DURATION,
            CONVERSION_AUTO_RELOAD_PERIOD);

}

void configure_light_sensor(struct Sensor_Instance * p_inst) {

	uint8_t init_test = 0;
    int ret;

    uint8_t datatowrite_ctrl = 0x06;    // Main Control
    ret = i2c_burst_write(i2c_dev, (RGB_ADDRESS<<1), MAIN_CTRL, &datatowrite_ctrl, sizeof(datatowrite_ctrl));
    if (ret < 0) {
        LOG_ERR("Could not write to MAIN_CTRL register, %d", ret);
    } else {
        init_test |= (1U<<0);
    }

    uint8_t datatowrite_meas = 0x22;    // Measurement and Rate Parameters
    
    ret = i2c_burst_write(i2c_dev, (RGB_ADDRESS<<1), LS_MEAS_RATE, &datatowrite_ctrl, sizeof(datatowrite_ctrl));
    if (ret < 0) {
        LOG_ERR("Could not write to LS_MEAS_RATE register, %d", ret);
    } else {
        init_test |= (1U<<1);
    }

    uint8_t datatowrite_gain = 0x02;    // Gain setting
    
    ret = i2c_burst_write(i2c_dev, (RGB_ADDRESS<<1), LS_GAIN, &datatowrite_ctrl, sizeof(datatowrite_ctrl));
    if (ret < 0) {
        LOG_ERR("Could not write to LS_GAIN register, %d", ret);
    } else {
        init_test |= (1U<<2);
    }



}


void read_sensor(struct Sensor_Instance * p_inst)
{
    int ret; 
    
    // RED
    ret = i2c_write_read(i2c_dev, (RGB_ADDRESS<<1), &RED_REGISTER, 1, red_val, 3);
    if (ret < 0) {
        LOG_ERR("Could not read from RED_REGISTER, %d", ret);
    }
    RED_TOTAL = (red_val[2] | red_val[1] | red_val[0])<<LIGHT_RESOLUTION;


    // GREEN
    ret = i2c_write_read(i2c_dev, (RGB_ADDRESS<<1), &GREEN_REGISTER, 1, green_val, 3);
    if (ret < 0) {
        LOG_ERR("Could not read from GREEN_REGISTER, %d", ret);
    }
    GREEN_TOTAL = (green_val[2] | green_val[1] | green_val[0])<<LIGHT_RESOLUTION;


    // BLUE
    ret = i2c_write_read(i2c_dev, (RGB_ADDRESS<<1), &BLUE_REGISTER, 1, blue_val, 3);
    if (ret < 0) {
        LOG_ERR("Could not read from BLUE_REGISTER, %d", ret);
    }
    BLUE_TOTAL = (blue_val[2] | blue_val[1] | blue_val[0])<<LIGHT_RESOLUTION;

    // IR
    ret = i2c_write_read(i2c_dev, (RGB_ADDRESS<<1), &IR_REGISTER, 1, ir_val, 3);
    if (ret < 0) {
        LOG_ERR("Could not read from IR_REGISTER, %d", ret);
    }
    IR_TOTAL = (ir_val[2] | ir_val[1] | ir_val[0])<<LIGHT_RESOLUTION;

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
 *  |                  |  |* Sensor calculate |
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
    struct Sensor_Instance * p_inst = sm_ctx_to_instance(p_sm);

    /* Get the event. */
    struct Sensor_SM_Evt * p_evt = &p_inst->sm_evt;

    /* Expecting only an "init instance" event. Anything else is an error. */
    assert(p_evt->sig == k_Sensor_SM_Evt_Sig_Init_Instance);

    /* We init'd required params on the caller's thread (i.e.
     * Sensor_Init_Instance()), now finish the job. Since this is an
     * Init_Instance event the data contains the intance cfg. */
    struct Sensor_SM_Evt_Sig_Init_Instance * p_ii = &p_evt->data.init_inst;

    broadcast_instance_initialized(p_inst, p_ii->cfg.cb);
    config_instance_deferred(p_inst, &p_ii->cfg);
    smf_set_state(SMF_CTX(p_sm), &states[run]);
}

/* Run state responsibility is to be the root state to handle everything else
 * other than instance initialization. */

static void state_run_entry(void * o)
{
    struct smf_ctx * p_sm = o;
    struct Sensor_Instance * p_inst = sm_ctx_to_instance(p_sm);

    start_sensor_timer(p_inst);

}

static void state_run_run(void * o)
{
    struct smf_ctx * p_sm = o;
    struct Sensor_Instance * p_inst = sm_ctx_to_instance(p_sm);

    /* Get the event. */
    struct Sensor_SM_Evt * p_evt = &p_inst->sm_evt;

    switch(p_evt->sig){
        default: break;
        case k_Sensor_SM_Evt_Sig_Init_Instance:
            /* Should never occur. */
            assert(false);
            break;
        case k_Sensor_SM_Evt_Sig_Read:

            read_sensor(p_inst);
            
            /*broadcast event to listener telling it we have new sensor data*/
            uint32_t sensor_data[4] = {RED_TOTAL, GREEN_TOTAL, BLUE_TOTAL, IR_TOTAL};

            struct Sensor_SM_Evt evt = {
                .sig = k_Sensor_Evt_Sig_Read,
                .data.read.read = sensor_data
            };

            broadcast_event_to_listeners(p_inst, &evt);
    		
            break;
        case k_Sensor_SM_Evt_Sig_Sensor_Setup_Complete:
            uint8_t result = p_evt->data.setup.setup;
            
            bool pass = (result == 0x07) ? true : false;

            struct Sensor_SM_Evt read_evt = {
                .sig = k_Sensor_Evt_Sig_Sensor_Setup_Complete,
                .data.setup.setup = pass 
            };
            broadcast_event_to_listeners(p_inst, p_evt);

            break;
        #if CONFIG_FKMG_SENSOR_SHUTDOWN_ENABLED
        case k_Sensor_Evt_Sig_Instance_Deinitialized:
            assert(false);
            break;
        #endif
    }
}

/* Deinit state responsibility is to clean up before exiting thread. */
#if CONFIG_FKMG_SENSOR_SHUTDOWN_ENABLED
static void state_deinit_run(void * o)
{
    struct smf_ctx * p_sm = o;
    struct Sensor_Instance * p_inst = sm_ctx_to_instance(p_sm);

    /* Get the event. */
    struct Sensor_SM_Evt * p_evt = &p_inst->sm_evt;

    /* TODO */
}
#endif

static const struct smf_state states[] = {
    /*                                      entry               run  exit */
    [  init] = SMF_CREATE_STATE(           NULL,   state_init_run, NULL),
    [   run] = SMF_CREATE_STATE(state_run_entry,    state_run_run, NULL),
    #if CONFIG_FKMG_SENSOR_SHUTDOWN_ENABLED
    [deinit] = SMF_CREATE_STATE(           NULL, state_deinit_run, NULL),
    #endif
};

/* ******
 * Thread
 * ******/

#if CONFIG_FKMG_SENSOR_ALLOW_SHUTDOWN
/* Since there is only the "join" facility to know when a thread is shut down,
 * and that isn't appropriate to use since it will put the calling thread to
 * sleep until the other thread is shut down, we set up a delayable system work
 * queue event to check that the thread is shut down and then call any callback
 * that is waiting to be notified. */
void on_thread_shutdown(struct k_work *item)
{
    struct Sensor_Instance * p_inst =
            CONTAINER_OF(item, struct Sensor_Instance, work);

    char * thread_state_str = "dead";
    k_thread_state_str(&p_inst->thread, thread_state_str, sizeof(thread_state_str));
    bool shut_down = strcmp( thread_state_str, "dead" ) == 0;

    if(!shut_down) k_work_reschedule(&p_inst->work, K_MSEC(1));
    else broadcast_instance_deinitialized(p_inst);
}
#endif

static void thread(void * p_1, /* struct Sensor_Instance* */
        void * p_2_unused, void * p_3_unused)
{
    struct Sensor_Instance * p_inst = p_1;
    // printk("Sensor Thread Start. \n"); 
    /* NOTE: smf_set_initial() executes the entry state. */
    struct smf_ctx * p_sm = &p_inst->sm;
    smf_set_initial(SMF_CTX(p_sm), &states[init]);

    /* Get the state machine event queue and point to where to put the dequeued
     * event. */
    struct k_msgq * p_msgq = p_inst->msgq.p_sm_evts;
    struct Sensor_SM_Evt * p_evt = &p_inst->sm_evt;

    bool run = true;

    while(run){
        /* Wait on state machine event. Cache it then run state machine. */
        k_msgq_get(p_msgq, p_evt, K_FOREVER);
        run = smf_run_state(SMF_CTX(p_sm)) == 0;
    }

    #if CONFIG_FKMG_SENSOR_ALLOW_SHUTDOWN
    /* We're shutting down. Schedule a work queue event to check that the
     * thread exited and call back anything. */
    if(should_callback_on_exit(p_inst)){
        k_work_init_delayable( &p_inst->work, on_thread_shutdown);
        k_work_schedule(&p_inst->work, K_MSEC(1));
    }
    #endif
}

static void start_thread(
        struct Sensor_Instance     * p_inst,
        struct Sensor_Instance_Cfg * p_cfg)
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

void Sensor_Init_Instance(struct Sensor_Instance_Cfg * p_cfg)
{
    /* Get pointer to instance to configure. */
    struct Sensor_Instance * p_inst = p_cfg->p_inst;

    #if CONFIG_FKMG_SENSOR_RUNTIME_ERROR_CHECKING
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

#if CONFIG_FKMG_SENSOR_ALLOW_SHUTDOWN
void Sensor_Deinit_Instance(struct Sensor_Instance_Dcfg * p_dcfg)
{
    #error "Not implemented yet!"
}
#endif

void Sensor_Add_Listener(struct Sensor_Listener_Cfg * p_cfg)
{
    #if CONFIG_FKMG_SENSOR_RUNTIME_ERROR_CHECKING
    /* Get pointer to instance. */
    struct Sensor_Instance * p_inst = p_cfg->p_inst;

    /* Check listener instance configuration for errors. */
    if(errored(p_inst, check_listener_cfg_param_for_errors(p_cfg))){
        assert(false);
        return;
    }
    #endif

    struct Sensor_Listener * p_lsnr = p_cfg->p_lsnr;
    init_listener(p_lsnr);
    config_listener(p_lsnr, p_cfg);
    add_listener_for_signal_to_listener_list(p_cfg);
}


#if CONFIG_FKMG_SENSOR_ALLOW_LISTENER_REMOVAL
void Sensor_Remove_Listener(struct Sensor_Listener * p_lsnr)
{
    #error "Not implemented yet!"
}
#endif

#if CONFIG_FKMG_SENSOR_NO_OPTIMIZATIONS
#pragma GCC pop_options
#endif
