/** ****************************************************************************
 * @brief Internal state machine events generated by SENSOR.
 */

#ifndef FKMG_SENSOR_SM_EVT_H
#define FKMG_SENSOR_SM_EVT_H

#ifdef __cplusplus
extern "C" {
#endif

/* *****************************************************************************
 * Includes
 */

#include <zephyr/kernel.h>

#include "../id.h"
#include "../instance_cfg.h"

/* *****************************************************************************
 * Events
 */

/* Signals that can be generated as part of an event. */
enum Sensor_SM_Evt_Sig{
    k_Sensor_SM_Evt_Sig_Beg,                           // Inclusive
    k_Sensor_SM_Evt_Sig_Min = k_Sensor_SM_Evt_Sig_Beg,    // Inclusive
    k_Sensor_SM_Evt_Sig_1st = k_Sensor_SM_Evt_Sig_Beg,    // Inclusive

	k_Sensor_SM_Evt_Sig_Init_Instance = k_Sensor_SM_Evt_Sig_Beg,
    #if CONFIG_FKMG_SENSOR_ALLOW_SHUTDOWN
	k_Sensor_SM_Evt_Sig_Deinit_Instance,
    #endif
	k_Sensor_SM_Evt_Sig_Read,
    k_Sensor_SM_Evt_Sig_Sensor_Setup_Complete,

    k_Sensor_SM_Evt_Sig_End,                           // Exclusive
    k_Sensor_SM_Evt_Sig_Max = k_Sensor_SM_Evt_Sig_End - 1,// Inclusive
    k_Sensor_SM_Evt_Sig_Lst = k_Sensor_SM_Evt_Sig_End - 1,// Inclusive
    k_Sensor_SM_Evt_Sig_Cnt = k_Sensor_SM_Evt_Sig_End
                         - k_Sensor_SM_Evt_Sig_Beg,	// Inclusive
	k_Sensor_SM_Evt_Sig_Ivd = k_Sensor_SM_Evt_Sig_End
};

/* Data signal k_Sensor_SM_Evt_Sig_Init_Instance can generate. */
struct Sensor_SM_Evt_Sig_Init_Instance{
    struct Sensor_Instance_Cfg cfg;
};

/* Data signal k_Sensor_SM_Evt_Sig_Read can generate. */
struct Sensor_SM_Evt_Sig_Read{
    bool read; 
};

struct Sensor_SM_Evt_Sensor_Setup_Complete{
    uint8_t setup;
};

/* Events (i.e. signal + signal's data if any) Sensor State Machine generates. */
struct Sensor_SM_Evt{
	enum Sensor_SM_Evt_Sig sig;
	union Sensor_SM_Evt_Data{
        struct Sensor_SM_Evt_Sig_Init_Instance              init_inst;
        struct Sensor_SM_Evt_Sensor_Setup_Complete          setup;
        struct Sensor_SM_Evt_Sig_Read                       read;
	}data;
};

#ifdef __cplusplus
}
#endif

#endif /* FKMG_SENSOR_SM_EVT_H */
