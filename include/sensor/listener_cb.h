/** ****************************************************************************
 * @brief Sensor listener definition.
 */

#ifndef FKMG_SENSOR_LISTENER_CB_H
#define FKMG_SENSOR_LISTENER_CB_H

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
struct Sensor_Evt;

typedef void (*Sensor_Listener_Cb)(struct Sensor_Evt *p_evt);

#ifdef __cplusplus
}
#endif

#endif /* FKMG_SENSOR_LISTENER_CB_H */
