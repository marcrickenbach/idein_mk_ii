/** ****************************************************************************
 * @brief Idein listener configuration interface.
 */

#ifndef FKMG_IDEIN_LISTENER_CB_CFG_H
#define FKMG_IDEIN_LISTENER_CB_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/* *****************************************************************************
 * Includes
 */

#include <zephyr/kernel.h>

#include "listener_cb.h"
#include "evt.h"

/* *****************************************************************************
 * Listener Callback Typedef
 */

/* Forward references to prevent include interdependent items getting declared
 * out-of-order. */
struct Idein_Instance;
struct Idein_Listener;

struct Idein_Listener_Cfg{
    /* Required: pointer to initialized opaque instace we'll add listener to. */
    struct Idein_Instance * p_inst;

    /* Required: pointer to uninitialized/unused opaque listener to config. */
    struct Idein_Listener * p_lsnr;

    /* Required: event signal to listen for. */
    enum Idein_Evt_Sig sig;

    /* Required: function to call back and send event to when signal occurs. */
    Idein_Listener_Cb cb;
};

#ifdef __cplusplus
}
#endif

#endif /* FKMG_SEQ_LISTENER_CB_CFG_H */
