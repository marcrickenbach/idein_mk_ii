/*******************************************************************************
 * @brief UI Thread.
 *
 * This module is the public api to interface with the UI. 
 *
 * @example
 */

#ifndef FKMG_UI_H
#define FKMG_UI_H

/* In case C++ needs to use anything here */
#ifdef __cplusplus
extern “C” {
#endif

/* *****************************************************************************
 * Includes
 */

#include <zephyr/kernel.h>

#include "ui/instance.h"
#include "ui/instance_cfg.h"
#include "ui/listener.h"
#include "ui/listener_cfg.h"

/* *****************************************************************************
 * Public
 */

/* NOTE: all callbacks must be handled in a timely manner since the callbacks
 * occur on the thread of this engine. */

/**
 * Initialize an instance. An instance contains everything required to declare
 * and define all the data/memory for threads, queues, etc., that runs the the
 * engine. Even though it might be possible to have multiple instances,
 * typically only one is required.
 * @param[in] p_cfg Pointer to the filled-in configuration struct. See the
 * struct for details.
 */
void UI_Init_Instance(struct UI_Instance_Cfg * p_cfg);


/**
 * Deinitialize an instance.
 * @param[in] p_dcfg Pointer to the filled-in deconfiguration struct. See the
 * struct for details.
 */
#if CONFIG_FKMG_UI_ALLOW_SHUTDOWN
void UI_Deinit_Instance(struct UI_Instance_Dcfg * p_dcfg);
#endif

/**
 * Add an event signal listener to an interface.
 * @param[in] p_cfg Pointer to the filled-in configuration struct. See the
 * struct for details.
 */
void UI_Add_Listener(struct UI_Listener_Cfg * p_cfg);

#if CONFIG_FKMG_UI_ALLOW_LISTENER_REMOVAL
void UI_Remove_Listener(struct UI_Listener * p_lsnr);
#endif

/* Public API functions to consider:
 *
 */
#ifdef __cplusplus
}
#endif

#endif /* FKMG_UI_H */
