/** ****************************************************************************
 * @brief SENSOR id.
 */

#ifndef FKMG_SENSOR_ID_H
#define FKMG_SENSOR_ID_H

#ifdef __cplusplus
extern "C" {
#endif

/* *****************************************************************************
 * Includes
 */

#include <zephyr/kernel.h>

/* *****************************************************************************
 * Events
 */

/* Signals that can be generated as part of an event. */
enum Sensor_Id{
    k_Sensor_Id_Beg,                   // Inclusive
    k_Sensor_Id_Min = k_Sensor_Id_Beg,    // Inclusive
    k_Sensor_Id_1st = k_Sensor_Id_Beg,    // Inclusive

    /* TODO: */
    k_Sensor_Id_1 = k_Sensor_Id_Beg,      // Suggest using pcb silkscreen name or
                                    // similar common reference
     k_Sensor_Id_descriptive_name_1 = k_Sensor_Id_1, // Use descriptive name, like
                                               // front panel name
    k_Sensor_Id_2,
    k_Sensor_Id_descriptive_name_2 = k_Sensor_Id_2,
    k_Sensor_Id_3,
    k_Sensor_Id_descriptive_name_3 = k_Sensor_Id_3,
    /* etc */

    k_Sensor_Id_End,                   // Exclusive
    k_Sensor_Id_Max = k_Sensor_Id_End - 1,// Inclusive
    k_Sensor_Id_Lst = k_Sensor_Id_End - 1,// Inclusive
    k_Sensor_Id_Cnt = k_Sensor_Id_End
                 - k_Sensor_Id_Beg,	// Inclusive
	k_Sensor_Id_Ivd = k_Sensor_Id_End
};

#ifdef __cplusplus
}
#endif

#endif /* FKMG_SENSOR_ID_H */
