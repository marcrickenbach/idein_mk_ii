/** ****************************************************************************
 * @brief Voice instance interface.
 *
 * An instance is the root instantiation of the implementation. An instance
 * contains everything required to declare and define all the threads, queues,
 * etc. Even though it might be possible to have multiple instances, typically
 * only one is required or even possible.
 */

#ifndef FKMG_VOICE_INSTANCE_H
#define FKMG_VOICE_INSTANCE_H

#ifdef __cplusplus
extern "C" {
#endif

/* *****************************************************************************
 * Includes
 */

#include <zephyr/kernel.h>
#include <zephyr/smf.h>

#include <zephyr/drivers/adc.h>

#include "err.h"
#include "id.h"
#include "evt.h"
#include "private/sm_evt.h"

/* *****************************************************************************
 * Enums
 */

/* *****************************************************************************
 * Instance
 */

struct Voice_Instance{
    #if CONFIG_FKMG_Voice_RUNTIME_ERROR_CHECKING
    /* Error status. */
	enum Voice_Err_Id err;
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
    struct Voice_SM_Evt sm_evt;

    /* Singly linked lists to keep track of things. */
    struct{
        sys_slist_t listeners[k_Voice_Evt_Sig_Cnt];
    }list;

    /* For adding this instance to singly linked lists. */
    struct{
        sys_snode_t instance;
    }node;


    /* Current pot. */
    enum Voice_Id id;
};

#ifdef __cplusplus
}
#endif

#endif /* FKMG_VOICE_INSTANCE_H */
