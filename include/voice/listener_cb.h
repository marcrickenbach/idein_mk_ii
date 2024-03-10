/** ****************************************************************************
 * @brief Voice listener definition.
 */

#ifndef FKMG_VOICE_LISTENER_CB_H
#define FKMG_VOICE_LISTENER_CB_H

#ifdef __cplusplus
extern "C" {
#endif

/* *****************************************************************************
 * Includes
 */

#include <zephyr/kernel.h>

/* *****************************************************************************
 * Listener Callback Typedef
 */

/* Forward references to prevent include interdependent items getting declared
 * out-of-order. */
struct Voice_Evt;

typedef void (*Voice_Listener_Cb)(struct Voice_Evt *p_evt);

#ifdef __cplusplus
}
#endif

#endif /* FKMG_VOICE_LISTENER_CB_H */
