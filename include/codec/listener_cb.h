/** ****************************************************************************
 * @brief Codec listener definition.
 */

#ifndef FKMG_CODEC_LISTENER_CB_H
#define FKMG_CODEC_LISTENER_CB_H

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
struct Codec_Evt;

typedef void (*Codec_Listener_Cb)(struct Codec_Evt *p_evt);

#ifdef __cplusplus
}
#endif

#endif /* FKMG_CODEC_LISTENER_CB_H */
