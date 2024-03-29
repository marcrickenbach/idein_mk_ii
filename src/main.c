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

#include "codec.h"
#include "codec/private/sm_evt.h"

#include "pot.h"
#include "pot/id.h"
#include "pot/private/sm_evt.h"

#include "idein.h"
#include "idein/private/sm_evt.h"

#include "midi.h"
#include "midi/private/sm_evt.h"

#include "sensor.h"
#include "sensor/private/sm_evt.h"

#include "voice.h"
#include "voice/private/sm_evt.h"

#include "ui.h"
#include "ui/private/sm_evt.h"


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

/* Declare threads, queues, and other data structures for Codec instance. */
static struct k_thread codec_thread;
#define CODEC_THREAD_STACK_SZ_BYTES   512
K_THREAD_STACK_DEFINE(codec_thread_stack, CODEC_THREAD_STACK_SZ_BYTES);
#define MAX_QUEUED_CODEC_SM_EVTS  10
#define CODEC_SM_QUEUE_ALIGNMENT  4
K_MSGQ_DEFINE(codec_sm_evt_q, sizeof(struct Codec_SM_Evt),
        MAX_QUEUED_CODEC_SM_EVTS, CODEC_SM_QUEUE_ALIGNMENT);
static struct Codec_Instance codec_inst;

/* Declare threads, queues, and other data structures for Idein instance. */
static struct k_thread idein_thread;
#define IDEIN_THREAD_STACK_SZ_BYTES   512
K_THREAD_STACK_DEFINE(idein_thread_stack, IDEIN_THREAD_STACK_SZ_BYTES);
#define MAX_QUEUED_IDEIN_SM_EVTS  10
#define IDEIN_SM_QUEUE_ALIGNMENT  4
K_MSGQ_DEFINE(idein_sm_evt_q, sizeof(struct Idein_SM_Evt),
        MAX_QUEUED_IDEIN_SM_EVTS, IDEIN_SM_QUEUE_ALIGNMENT);
static struct Idein_Instance idein_inst;


/* Declare threads, queues, and other data structures for MIDI / MIDI instance. */
static struct k_thread midi_thread;
#define MIDI_THREAD_STACK_SZ_BYTES   512
K_THREAD_STACK_DEFINE(midi_thread_stack, MIDI_THREAD_STACK_SZ_BYTES);
#define MAX_QUEUED_MIDI_SM_EVTS  10
#define MIDI_SM_QUEUE_ALIGNMENT  4
K_MSGQ_DEFINE(midi_sm_evt_q, sizeof(struct MIDI_SM_Evt),
        MAX_QUEUED_MIDI_SM_EVTS, MIDI_SM_QUEUE_ALIGNMENT);
static struct MIDI_Instance midi_inst;

/* Declare threads, queues, and other data structures for Voice / Voice instance. */
static struct k_thread voice_thread;
#define VOICE_THREAD_STACK_SZ_BYTES   512
K_THREAD_STACK_DEFINE(voice_thread_stack, VOICE_THREAD_STACK_SZ_BYTES);
#define MAX_QUEUED_VOICE_SM_EVTS  10
#define VOICE_SM_QUEUE_ALIGNMENT  4
K_MSGQ_DEFINE(voice_sm_evt_q, sizeof(struct Voice_SM_Evt),
        MAX_QUEUED_VOICE_SM_EVTS, VOICE_SM_QUEUE_ALIGNMENT);
static struct Voice_Instance voice_inst;


/* Declare threads, queues, and other data structures for Sensor instance. */
static struct k_thread ui_thread;
#define UI_THREAD_STACK_SZ_BYTES   512
K_THREAD_STACK_DEFINE(ui_thread_stack, UI_THREAD_STACK_SZ_BYTES);
#define MAX_QUEUED_UI_SM_EVTS  10
#define UI_SM_QUEUE_ALIGNMENT  4
K_MSGQ_DEFINE(ui_sm_evt_q, sizeof(struct UI_SM_Evt),
        MAX_QUEUED_UI_SM_EVTS, UI_SM_QUEUE_ALIGNMENT);
static struct UI_Instance ui_inst;


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

static void on_codec_instance_initialized(struct Codec_Evt *p_evt)
{
    assert(p_evt->sig == k_Codec_Evt_Sig_Instance_Initialized);
    assert(p_evt->data.initd.p_inst == &codec_inst);
	k_event_post(&events, EVT_FLAG_INSTANCE_INITIALIZED);
}

static void on_idein_instance_initialized(struct Idein_Evt *p_evt)
{
    assert(p_evt->sig == k_Idein_Evt_Sig_Instance_Initialized);
    assert(p_evt->data.initd.p_inst == &idein_inst);
	k_event_post(&events, EVT_FLAG_INSTANCE_INITIALIZED);
}

static void on_midi_instance_initialized(struct MIDI_Evt *p_evt)
{
    assert(p_evt->sig == k_MIDI_Evt_Sig_Instance_Initialized);
    assert(p_evt->data.initd.p_inst == &midi_inst);
	k_event_post(&events, EVT_FLAG_INSTANCE_INITIALIZED);
}

static void on_sensor_instance_initialized(struct Sensor_Evt *p_evt)
{
    assert(p_evt->sig == k_Sensor_Evt_Sig_Instance_Initialized);
    assert(p_evt->data.initd.p_inst == &sensor_inst);
	k_event_post(&events, EVT_FLAG_INSTANCE_INITIALIZED);
}

static void on_voice_instance_initialized(struct Voice_Evt *p_evt)
{
    assert(p_evt->sig == k_Voice_Evt_Sig_Instance_Initialized);
    assert(p_evt->data.initd.p_inst == &voice_inst);
	k_event_post(&events, EVT_FLAG_INSTANCE_INITIALIZED);
}

static void on_ui_instance_initialized(struct UI_Evt *p_evt)
{
    assert(p_evt->sig == k_UI_Evt_Sig_Instance_Initialized);
    assert(p_evt->data.initd.p_inst == &ui_inst);
	k_event_post(&events, EVT_FLAG_INSTANCE_INITIALIZED);
}

static void wait_on_instance_initialized(void)
{
	uint32_t events_rcvd = k_event_wait(&events, EVT_FLAG_INSTANCE_INITIALIZED,
		true, K_MSEC(WAIT_MAX_MSECS_FOR_INITIALIZATION));
	assert(events_rcvd == EVT_FLAG_INSTANCE_INITIALIZED);
}


  /* ********************
 * ON BUTTON PRESS
 * ********************/


static void on_button_press(struct UI_Evt *p_evt)
{
    assert(p_evt->sig == k_UI_Evt_Sig_Button_Press);

    struct Codec_SM_Evt codec_evt = {
        .sig = k_Codec_SM_Evt_Sig_Trigger_Sine,
    }; 

    k_msgq_put(&codec_sm_evt_q, &codec_evt, K_NO_WAIT);
}


/* ********************
 * ON POT CHANGE
 * ********************/

static void on_pot_changed(struct Pot_Evt *p_evt)
{
    assert(p_evt->sig == k_Pot_Evt_Sig_Changed);

    struct Idein_SM_Evt_Sig_Pot_Value_Changed * p_changed = (struct Idein_SM_Evt_Sig_Pot_Value_Changed *)&p_evt->data.changed;

    uint32_t val[5];

    memcpy(val, p_changed->val, sizeof(p_changed->val));

    struct Idein_SM_Evt seq_evt = {
        .sig = k_Idein_SM_Evt_Sig_Pot_Value_Changed,
        .data.pot_changed = val
    }; 

    k_msgq_put(&idein_sm_evt_q, &seq_evt, K_NO_WAIT);
}


/* ********************
 * ON CLOCK EVENT
 * ********************/

static void on_clock_event(struct Idein_Evt *p_evt)
{
    assert(p_evt->sig == k_Idein_Evt_Sig_Clock_Event);

    struct Voice_SM_Evt voice_evt = {
        .sig = k_Voice_SM_Evt_Sig_New_Note_Event,
        .data.note.val = p_evt->data.note.note
    }; 

    k_msgq_put(&voice_sm_evt_q, &voice_evt, K_NO_WAIT);
}


/* ********************
 * SENSOR LISTENER CALLBACKS 
 * ********************/

static void on_sensor_read(struct Sensor_Evt *p_evt) 
{
    assert(p_evt->sig == k_Sensor_Evt_Sig_Read);

    struct Idein_SM_Evt_Sig_Read_New * p_read = (struct Idein_SM_Evt_Sig_Read_New *)&p_evt->data.read;

    uint32_t val[5];
    memcpy(val, p_read->read_values, sizeof(p_read->read_values));

    struct Idein_SM_Evt seq_evt = {
        .sig = k_Idein_SM_Evt_Sig_Sensor_Read,
        .data.sensor_read = val
    };
}

static void on_sensor_setup(struct Sensor_Evt *p_evt) 
{
    /* Read event data which gives us a bool of true or false, telling us if the sensor set up passed or failed*/
    struct Sensor_SM_Evt_Sensor_Setup_Complete * p_setup = (struct Sensor_SM_Evt_Sensor_Setup_Complete *)&p_evt->data.setup;

    /* Create an event with set up result to UI module */
    struct UI_SM_Evt evt = {
        .sig = k_UI_SM_Evt_Sig_Sensor_Setup_Complete,
        .data.pass = p_setup->setup
    };

    k_msgq_put(&ui_sm_evt_q, &evt, K_NO_WAIT);
}



/* *****************************************************************************
 * Public.
 */

   int main (void) {

    /* Instance: UI */
    struct UI_Instance_Cfg ui_inst_cfg = {
        .p_inst = &ui_inst,
        .task.sm.p_thread = &ui_thread,
        .task.sm.p_stack = ui_thread_stack,
        .task.sm.stack_sz = K_THREAD_STACK_SIZEOF(ui_thread_stack),
        .task.sm.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
        .msgq.p_sm_evts = &ui_sm_evt_q,
        .cb = on_ui_instance_initialized,
    };
    UI_Init_Instance(&ui_inst_cfg);
    wait_on_instance_initialized();

    static struct UI_Listener ui_btn_lsnr;
    struct UI_Listener_Cfg ui_btn_lsnr_cfg = {
        .p_inst = &ui_inst,
        .p_lsnr = &ui_btn_lsnr, 
        .sig     = k_UI_Evt_Sig_Button_Press,
        .cb      = on_button_press
    };
    UI_Add_Listener(&ui_btn_lsnr_cfg);


    /* Instance: Pot */
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


    /* Instance: Codec */
    struct Codec_Instance_Cfg codec_inst_cfg = {
        .p_inst = &codec_inst,
        .task.sm.p_thread = &codec_thread,
        .task.sm.p_stack = codec_thread_stack,
        .task.sm.stack_sz = K_THREAD_STACK_SIZEOF(codec_thread_stack),
        .task.sm.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
        .msgq.p_sm_evts = &codec_sm_evt_q,
        .cb = on_codec_instance_initialized,
    };
    Codec_Init_Instance(&codec_inst_cfg);
    wait_on_instance_initialized();


     /* Instance: MIDI Module */
    struct MIDI_Instance_Cfg midi_inst_cfg = {
        .p_inst = &midi_inst,
        .task.sm.p_thread = &midi_thread,
        .task.sm.p_stack = midi_thread_stack,
        .task.sm.stack_sz = K_THREAD_STACK_SIZEOF(midi_thread_stack),
        .task.sm.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
        .msgq.p_sm_evts = &midi_sm_evt_q,
        .cb = on_midi_instance_initialized,
    };
    MIDI_Init_Instance(&midi_inst_cfg);
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
    
    static struct Sensor_Listener sensor_setup_lsnr;
    struct Sensor_Listener_Cfg sensor_setup_lsnr_cfg = {
        .p_inst = &sensor_inst,
        .p_lsnr = &sensor_setup_lsnr, 
        .sig     = k_Sensor_Evt_Sig_Sensor_Setup_Complete,
        .cb      = on_sensor_setup
    };
    Sensor_Add_Listener(&sensor_setup_lsnr_cfg);

    static struct Sensor_Listener sensor_read_lsnr;
    struct Sensor_Listener_Cfg sensor_read_lsnr_cfg = {
        .p_inst = &sensor_inst,
        .p_lsnr = &sensor_read_lsnr, 
        .sig     = k_Sensor_Evt_Sig_Read,
        .cb      = on_sensor_read
    };
    Sensor_Add_Listener(&sensor_read_lsnr_cfg);

    /* Instance: Voice Engine */
    struct Voice_Instance_Cfg voice_inst_cfg = {
        .p_inst = &voice_inst,
        .task.sm.p_thread = &voice_thread,
        .task.sm.p_stack = voice_thread_stack,
        .task.sm.stack_sz = K_THREAD_STACK_SIZEOF(voice_thread_stack),
        .task.sm.prio = K_LOWEST_APPLICATION_THREAD_PRIO,
        .msgq.p_sm_evts = &voice_sm_evt_q,
        .cb = on_voice_instance_initialized,
    };
    Voice_Init_Instance(&voice_inst_cfg);
    wait_on_instance_initialized();


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
    
    static struct Idein_Listener idein_clock_lsnr;
    struct Idein_Listener_Cfg idein_clock_lsnr_cfg = {
        .p_inst = &idein_inst,
        .p_lsnr = &idein_clock_lsnr, 
        .sig     = k_Idein_Evt_Sig_Clock_Event,
        .cb      = on_clock_event
    };
    Idein_Add_Listener(&idein_clock_lsnr_cfg);

    return 0;

};
