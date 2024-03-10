/** ****************************************************************************
 * @brief MIDI listener definition.
 */

#ifndef FKMG_MIDI_LISTENER_CB_H
#define FKMG_MIDI_LISTENER_CB_H

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
struct MIDI_Evt;

typedef void (*MIDI_Listener_Cb)(struct MIDI_Evt *p_evt);

#ifdef __cplusplus
}
#endif

#endif /* FKMG_MIDI_LISTENER_CB_H */
