/*******************************************************************************
 * @brief Idein interface.
 *
 * This module is the public api to interface with the main Idein. 
 * The Idein oversees pot changes, sends dac write commands and handles timing of steps. 
 *
 * @example
 */

#ifndef FKMG_IDEIN_H
#define FKMG_IDEIN_H

/* In case C++ needs to use anything here */
#ifdef __cplusplus
extern “C” {
#endif

/* *****************************************************************************
 * Includes
 */

#include <zephyr/kernel.h>

#include "idein/instance.h"
#include "idein/instance_cfg.h"
#include "idein/listener.h"
#include "idein/listener_cfg.h"

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
void Idein_Init_Instance(struct Idein_Instance_Cfg * p_cfg);

// static void on_pot_change_ext(struct Idein_Evt *p_evt); 
/**
 * Deinitialize an instance.
 * @param[in] p_dcfg Pointer to the filled-in deconfiguration struct. See the
 * struct for details.
 */
#if CONFIG_FKMG_IDEIN_ALLOW_SHUTDOWN
void Idein_Deinit_Instance(struct Idein_Instance_Dcfg * p_dcfg);
#endif

/**
 * Add an event signal listener to an interface.
 * @param[in] p_cfg Pointer to the filled-in configuration struct. See the
 * struct for details.
 */
void Idein_Add_Listener(struct Idein_Listener_Cfg * p_cfg);

#if CONFIG_FKMG_IDEIN_ALLOW_LISTENER_REMOVAL
void Idein_Remove_Listener(struct Idein_Listener * p_lsnr);
#endif


/* Public API functions to consider:
 *
 */
#ifdef __cplusplus
}
#endif

#endif /* FKMG_IDEIN_H */
