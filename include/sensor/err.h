/** ****************************************************************************
 * @brief Sensor errors.
 *
 * The errors Sensor module generates.
 */

#ifndef FKMG_SENSOR_ERR_H
#define FKMG_SENSOR_ERR_H

#ifdef __cplusplus
extern "C" {
#endif

/* *****************************************************************************
 * Enums
 */

/* Errors generated. */
enum Sensor_Err_Id{
    k_Sensor_Err_Id_Beg,                   // Inclusive
    k_Sensor_Err_Id_Min = k_Sensor_Err_Id_Beg,// Inclusive
    k_Sensor_Err_Id_1st = k_Sensor_Err_Id_Beg,// Inclusive

    k_Sensor_Err_Id_None = k_Sensor_Err_Id_Beg,
    k_Sensor_Err_Id_Unknown,
    k_Sensor_Err_Id_Unimplemented,
    k_Sensor_Err_Id_Ptr_Invalid,
    k_Sensor_Err_Id_Configuration_Invalid,

    k_Sensor_Err_Id_End,                        // Exclusive
    k_Sensor_Err_Id_Max = k_Sensor_Err_Id_End - 1, // Inclusive
    k_Sensor_Err_Id_Lst = k_Sensor_Err_Id_End - 1, // Inclusive
    k_Sensor_Err_Id_Cnt = k_Sensor_Err_Id_End
                     - k_Sensor_Err_Id_Beg,     // Inclusive
    k_Sensor_Err_Id_Ivd = k_Sensor_Err_Id_End
};

#ifdef __cplusplus
}
#endif

#endif /* FKMG_SENSOR_ERR_H */
