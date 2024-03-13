/** ****************************************************************************
 * @brief UI listener definition.
 */

#ifndef FKMG_UI_LISTENER_CB_H
#define FKMG_UI_LISTENER_CB_H

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
struct UI_Evt;

typedef void (*UI_Listener_Cb)(struct UI_Evt *p_evt);

#ifdef __cplusplus
}
#endif

#endif /* FKMG_UI_LISTENER_CB_H */
