/** ****************************************************************************
 * @brief Voice listener instance interface.
 */

#ifndef FKMG_VOICE_LISTENER_H
#define FKMG_VOICE_LISTENER_H

#ifdef __cplusplus
extern "C" {
#endif

/* *****************************************************************************
 * Includes
 */

#include <zephyr/kernel.h>

#include "listener_cb.h"

/* *****************************************************************************
 * Defines
 */

/* *****************************************************************************
 * Listener
 */

/* Forward references to prevent include interdependent items getting declared
 * out-of-order. */
struct Voice_Instance;

struct Voice_Listener{
    /* Listener instances are kept track in a singly-linked list. */
    struct{
        sys_snode_t listener;
    }node;

    /* Pointer to instance this listener is listening to. */
    struct Voice_Instance * p_inst;

    /* Listener callback. */
    Voice_Listener_Cb cb;
};

#ifdef __cplusplus
}
#endif

#endif /* FKMG_VOICE_LISTENER_H */
