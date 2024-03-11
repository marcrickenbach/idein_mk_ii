/** ****************************************************************************
 * @brief Idein instance interface.
 *
 * An instance is the root instantiation of the implementation. An instance
 * contains everything required to declare and define all the threads, queues,
 * etc. Even though it might be possible to have multiple instances, typically
 * only one is required or even possible.
 */

#ifndef FKMG_IDEIN_INSTANCE_H
#define FKMG_IDEIN_INSTANCE_H

#ifdef __cplusplus
extern "C" {
#endif


/* *****************************************************************************
 * Includes
 */

#include <zephyr/kernel.h>
#include <zephyr/smf.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/gpio.h>

#include "id.h"
#include "step_id.h"
#include "err.h"
#include "evt.h"
#include "private/sm_evt.h"

/* *****************************************************************************
 * Enums
 */

/* *****************************************************************************
 * Instance
 */

struct Idein_Instance{
    #if CONFIG_FKMG_IDEIN_RUNTIME_ERROR_CHECKING
    /* Error status. */
	enum Idein_Err_Id err;
    #endif

    /* Threads used. */
    struct{
        struct{
            struct k_thread * p_thread;
        }sm;
    }task;

    /* Queues used. */
    struct{
        /* State machine event message queue. */
        struct k_msgq * p_sm_evts;
    }msgq;

    /* State machine. */
	struct smf_ctx sm;

    /* Current sm event. */
    struct Idein_SM_Evt sm_evt;

    /* Singly linked lists to keep track of things. */
    struct{
        sys_slist_t listeners[k_Idein_Evt_Sig_Cnt];
    }list;

    /* For adding this instance to singly linked lists. */
    struct{
        sys_snode_t instance;
    }node;

    /* Timers used. */
    struct{
        const struct device *t[2]; 
        bool running[2];
    }timer;

    struct {
        uint8_t last_note[2];
    }midi;

    struct{
        uint32_t threshold[4];
        uint32_t volume; 
    }idein; 

    counter_alarm_callback_t tim_cb[2];

    /* Current Seq. */
    enum Idein_Id id;

    /* GPIO Transport Interrupt Callback*/
    struct gpio_callback transport_gpio_cb;
};

#ifdef __cplusplus
}
#endif

#endif /* FKMG_IDEIN_INSTANCE_H */
