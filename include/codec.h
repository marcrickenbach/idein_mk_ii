/*******************************************************************************
 * @brief Codec Thread.
 *
 * This module is the public api to interface with the Codec. 
 *
 * @example
 */

#ifndef FKMG_CODEC_H
#define FKMG_CODEC_H

/* In case C++ needs to use anything here */
#ifdef __cplusplus
extern “C” {
#endif

/* *****************************************************************************
 * Includes
 */

#include <zephyr/kernel.h>

#include "codec/instance.h"
#include "codec/instance_cfg.h"
#include "codec/listener.h"
#include "codec/listener_cfg.h"

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
void Codec_Init_Instance(struct Codec_Instance_Cfg * p_cfg);

void codec_write_new_value(uint8_t ch, uint32_t val); 

/**
 * Deinitialize an instance.
 * @param[in] p_dcfg Pointer to the filled-in deconfiguration struct. See the
 * struct for details.
 */
#if CONFIG_FKMG_CODEC_ALLOW_SHUTDOWN
void Codec_Deinit_Instance(struct Codec_Instance_Dcfg * p_dcfg);
#endif

/**
 * Add an event signal listener to an interface.
 * @param[in] p_cfg Pointer to the filled-in configuration struct. See the
 * struct for details.
 */
void Codec_Add_Listener(struct Codec_Listener_Cfg * p_cfg);

#if CONFIG_FKMG_CODEC_ALLOW_LISTENER_REMOVAL
void Codec_Remove_Listener(struct Codec_Listener * p_lsnr);
#endif

/* Public API functions to consider:
 *
 */
#ifdef __cplusplus
}
#endif

#endif /* FKMG_CODEC_H */
