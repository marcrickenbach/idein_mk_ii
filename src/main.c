/* *****************************************************************************
 * Includes
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>

#include <assert.h>
#include <string.h>

#include "main.h"

#include "dac.h"
#include "dac/private/sm_evt.h"

#include "pot.h"
#include "pot/id.h"
#include "pot/private/sm_evt.h"

#include "idein.h"
#include "idein/private/sm_evt.h"

#include "uart.h"
#include "uart/private/sm_evt.h"

#include "sensor.h"
#include "sensor/private/sm_evt.h"



/* *****************************************************************************
 * Defines
 */

/* Error if took longer than this to initialize. */
#define WAIT_MAX_MSECS_FOR_INITIALIZATION 50


/* *****************************************************************************
 * Threads
 */

/* Declare threads, queues, and other data structures for pot instance. */
static struct k_thread pot_thread;
#define POT_THREAD_STACK_SZ_BYTES   512
K_THREAD_STACK_DEFINE(pot_thread_stack, POT_THREAD_STACK_SZ_BYTES);
#define MAX_QUEUED_POT_SM_EVTS  10
#define POT_SM_QUEUE_ALIGNMENT  4
K_MSGQ_DEFINE(pot_sm_evt_q, sizeof(struct Pot_SM_Evt),
        MAX_QUEUED_POT_SM_EVTS, POT_SM_QUEUE_ALIGNMENT);
static struct Pot_Instance pot_inst;

/* Declare threads, queues, and other data structures for DAC instance. */
static struct k_thread dac_thread;
#define DAC_THREAD_STACK_SZ_BYTES   512
K_THREAD_STACK_DEFINE(dac_thread_stack, DAC_THREAD_STACK_SZ_BYTES);
#define MAX_QUEUED_DAC_SM_EVTS  10
#define DAC_SM_QUEUE_ALIGNMENT  4
K_MSGQ_DEFINE(dac_sm_evt_q, sizeof(struct DAC_SM_Evt),
        MAX_QUEUED_DAC_SM_EVTS, DAC_SM_QUEUE_ALIGNMENT);
static struct DAC_Instance dac_inst;

/* Declare threads, queues, and other data structures for Idein instance. */
static struct k_thread idein_thread;
#define IDEIN_THREAD_STACK_SZ_BYTES   512
K_THREAD_STACK_DEFINE(idein_thread_stack, IDEIN_THREAD_STACK_SZ_BYTES);
#define MAX_QUEUED_IDEIN_SM_EVTS  10
#define IDEIN_SM_QUEUE_ALIGNMENT  4
K_MSGQ_DEFINE(idein_sm_evt_q, sizeof(struct Idein_SM_Evt),
        MAX_QUEUED_IDEIN_SM_EVTS, IDEIN_SM_QUEUE_ALIGNMENT);
static struct Idein_Instance idein_inst;


/* Declare threads, queues, and other data structures for UART / MIDI instance. */
static struct k_thread uart_thread;
#define UART_THREAD_STACK_SZ_BYTES   512
K_THREAD_STACK_DEFINE(uart_thread_stack, UART_THREAD_STACK_SZ_BYTES);
#define MAX_QUEUED_UART_SM_EVTS  10
#define UART_SM_QUEUE_ALIGNMENT  4
K_MSGQ_DEFINE(uart_sm_evt_q, sizeof(struct UART_SM_Evt),
        MAX_QUEUED_UART_SM_EVTS, UART_SM_QUEUE_ALIGNMENT);
static struct UART_Instance uart_inst;


/* Declare threads, queues, and other data structures for Sensor instance. */
static struct k_thread sensor_thread;
#define SENSOR_THREAD_STACK_SZ_BYTES   512
K_THREAD_STACK_DEFINE(sensor_thread_stack, SENSOR_THREAD_STACK_SZ_BYTES);
#define MAX_QUEUED_SENSOR_SM_EVTS  10
#define SENSOR_SM_QUEUE_ALIGNMENT  4
K_MSGQ_DEFINE(sensor_sm_evt_q, sizeof(struct Sensor_SM_Evt),
        MAX_QUEUED_SENSOR_SM_EVTS, SENSOR_SM_QUEUE_ALIGNMENT);
static struct Sensor_Instance sensor_inst;

/* *****************************************************************************
 * Listeners
 */

#if 0
static struct Pot_Listener midi_rcvd_listener;
#endif

/* *****************************************************************************
 * Kernel Events
 *
 * Zephyr low-overhead kernel way for threads to wait on and post asynchronous
 * events.
 */

/* For handling asynchronous callbacks. */
K_EVENT_DEFINE(events);
#define EVT_FLAG_INSTANCE_INITIALIZED   (1 << 0)



/* *****************************************************************************
 * Private Functions.
 */

/* ********************
 * ON INSTANCE INITS
 * ********************/

static void on_pot_instance_initialized(struct Pot_Evt *p_evt)
{
    assert(p_evt->sig == k_Pot_Evt_Sig_Instance_Initialized);
    assert(p_evt->data.initd.p_inst == &pot_inst);
	k_event_post(&events, EVT_FLAG_INSTANCE_INITIALIZED);
}

static void on_dac_instance_initialized(struct DAC_Evt *p_evt)
{
    assert(p_evt->sig == k_DAC_Evt_Sig_Instance_Initialized);
    assert(p_evt->data.initd.p_inst == &dac_inst);
	k_event_post(&events, EVT_FLAG_INSTANCE_INITIALIZED);
}

static void on_idein_instance_initialized(struct Idein_Evt *p_evt)
{
    assert(p_evt->sig == k_Idein_Evt_Sig_Instance_Initialized);
    assert(p_evt->data.initd.p_inst == &idein_inst);
	k_event_post(&events, EVT_FLAG_INSTANCE_INITIALIZED);
}

static void on_uart_instance_initialized(struct UART_Evt *p_evt)
{
    assert(p_evt->sig == k_UART_Evt_Sig_Instance_Initialized);
    assert(p_evt->data.initd.p_inst == &uart_inst);
	k_event_post(&events, EVT_FLAG_INSTANCE_INITIALIZED);
}

static void on_sensor_instance_initialized(struct UART_Evt *p_evt)
{
    assert(p_evt->sig == k_Sensor_Evt_Sig_Instance_Initialized);
    assert(p_evt->data.initd.p_inst == &sensor_inst);
	k_event_post(&events, EVT_FLAG_INSTANCE_INITIALIZED);
}

static void wait_on_instance_initialized(void)
{
	uint32_t events_rcvd = k_event_wait(&events, EVT_FLAG_INSTANCE_INITIALIZED,
		true, K_MSEC(WAIT_MAX_MSECS_FOR_INITIALIZATION));
	assert(events_rcvd == EVT_FLAG_INSTANCE_INITIALIZED);
}


  

/* ********************
 * ON POT CHANGE
 * ********************/

static void on_pot_changed(struct Pot_Evt *p_evt)
{
    assert(p_evt->sig == k_Pot_Evt_Sig_Changed);

    struct Idein_SM_Evt_Sig_Pot_Value_Changed * p_changed = (struct Idein_SM_Evt_Sig_Pot_Value_Changed *)&p_evt->data.changed;

    struct Idein_SM_Evt seq_evt = {
        .sig = k_Idein_SM_Evt_Sig_Pot_Value_Changed,
        .data.pot_changed = *p_changed
    }; 

    k_msgq_put(&idein_sm_evt_q, &seq_evt, K_NO_WAIT);

    /* Only need to send voltages to store in the MIDI object */
    if (p_changed->pot_id < 16) {
        struct UART_SM_Evt uart_evt = {
            .sig = k_UART_Evt_Sig_Changed,
            .data.changed.stp = (uint8_t)p_changed->pot_id,
            .data.changed.val = p_changed->val
        }; 

        k_msgq_put(&uart_sm_evt_q, &uart_evt, K_NO_WAIT); 
    }
}


/* ********************
 * ON MIDI READY TO WRITE CHANGE
 * ********************/

static void on_midi_write_ready(struct UART_Evt *p_evt) 
{
    assert(p_evt->sig == k_UART_Evt_Sig_Write_Ready);

    struct UART_SM_Evt_Sig_Write_MIDI * p_write = (struct UART_SM_Evt_Sig_Write_MIDI *) &p_evt->data.midi_write; 

    struct UART_SM_Evt evt = {
            .sig = k_UART_SM_Evt_Sig_Write_MIDI,
            .data.midi_write = *p_write
    };

    k_msgq_put(&uart_sm_evt_q, &evt, K_NO_WAIT); 
}


/* ********************
 * ON SENSOR READ 
 * ********************/

static void on_sensor_read(struct Sensor_Evt *p_evt) 
{

}


/* *****************************************************************************
 * Public.
 */

   int main (void) {
    // /* Instance: Pot */
    struct Pot_Instance_Cfg pot_inst_cfg = {
        .p_inst = &pot_inst,
        .task.sm.p_thread = &pot_thread,
        .task.sm.p_stack = pot_thread_stack,
        .task.sm.stack_sz = K_THREAD_STACK_SIZEOF(pot_thread_stack),
        .task.sm.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
        .msgq.p_sm_evts = &pot_sm_evt_q,
        .cb = on_pot_instance_initialized,
    };
    Pot_Init_Instance(&pot_inst_cfg);
    wait_on_instance_initialized();


    static struct Pot_Listener pot_changed_lsnr;
    struct Pot_Listener_Cfg pot_lsnr_cfg = {
        .p_inst = &pot_inst,
        .p_lsnr = &pot_changed_lsnr, 
        .sig     = k_Pot_Evt_Sig_Changed,
        .cb      = on_pot_changed
    };
    Pot_Add_Listener(&pot_lsnr_cfg);


    /* Instance: DAC */
    struct DAC_Instance_Cfg dac_inst_cfg = {
        .p_inst = &dac_inst,
        .task.sm.p_thread = &dac_thread,
        .task.sm.p_stack = dac_thread_stack,
        .task.sm.stack_sz = K_THREAD_STACK_SIZEOF(dac_thread_stack),
        .task.sm.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
        .msgq.p_sm_evts = &dac_sm_evt_q,
        .cb = on_dac_instance_initialized,
    };
    DAC_Init_Instance(&dac_inst_cfg);
    wait_on_instance_initialized();


     /* Instance: UART Module */
    struct UART_Instance_Cfg uart_inst_cfg = {
        .p_inst = &uart_inst,
        .task.sm.p_thread = &uart_thread,
        .task.sm.p_stack = uart_thread_stack,
        .task.sm.stack_sz = K_THREAD_STACK_SIZEOF(uart_thread_stack),
        .task.sm.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
        .msgq.p_sm_evts = &uart_sm_evt_q,
        .cb = on_uart_instance_initialized,
    };
    UART_Init_Instance(&uart_inst_cfg);
    wait_on_instance_initialized();



    /* Instance: Light Sensor */
    struct Sensor_Instance_Cfg sensor_inst_cfg = {
        .p_inst = &sensor_inst,
        .task.sm.p_thread = &sensor_thread,
        .task.sm.p_stack = sensor_thread_stack,
        .task.sm.stack_sz = K_THREAD_STACK_SIZEOF(sensor_thread_stack),
        .task.sm.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
        .msgq.p_sm_evts = &sensor_sm_evt_q,
        .cb = on_sensor_instance_initialized,
    };
    Sensor_Init_Instance(&sensor_inst_cfg);
    wait_on_instance_initialized();
    

    static struct Sensor_Listener sensor_read_lsnr;
    struct Sensor_Listener_Cfg sensor_read_lsnr_cfg = {
        .p_inst = &sensor_inst,
        .p_lsnr = &sensor_read_lsnr, 
        .sig     = k_Sensor_Evt_Sig_Read,
        .cb      = on_sensor_read
    };
    Sensor_Add_Listener(&sensor_read_lsnr_cfg);



    /* Instance: Idein */
    struct Idein_Instance_Cfg idein_inst_cfg = {
        .p_inst = &idein_inst,
        .task.sm.p_thread = &idein_thread,
        .task.sm.p_stack = idein_thread_stack,
        .task.sm.stack_sz = K_THREAD_STACK_SIZEOF(idein_thread_stack),
        .task.sm.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
        .msgq.p_sm_evts = &idein_sm_evt_q,
        .cb = on_idein_instance_initialized,
    };
    Idein_Init_Instance(&idein_inst_cfg);
    wait_on_instance_initialized();
    

    static struct Idein_Listener midi_write_lsnr;
    struct Idein_Listener_Cfg midi_write_lsnr_cfg = {
        .p_inst = &idein_inst,
        .p_lsnr = &midi_write_lsnr, 
        .sig     = k_UART_Evt_Sig_Write_Ready,
        .cb      = on_midi_write_ready
    };
    Idein_Add_Listener(&midi_write_lsnr_cfg);

    return 0;

};
