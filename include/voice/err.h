/** ****************************************************************************
 * @brief Voice errors.
 *
 * The errors Voice module generates.
 */

#ifndef FKMG_VOICE_ERR_H
#define FKMG_VOICE_ERR_H

#ifdef __cplusplus
extern "C" {
#endif

/* *****************************************************************************
 * Enums
 */

/* Errors generated. */
enum Voice_Err_Id{
    k_Voice_Err_Id_Beg,                   // Inclusive
    k_Voice_Err_Id_Min = k_Voice_Err_Id_Beg,// Inclusive
    k_Voice_Err_Id_1st = k_Voice_Err_Id_Beg,// Inclusive

    k_Voice_Err_Id_None = k_Voice_Err_Id_Beg,
    k_Voice_Err_Id_Unknown,
    k_Voice_Err_Id_Unimplemented,
    k_Voice_Err_Id_Ptr_Invalid,
    k_Voice_Err_Id_Configuration_Invalid,

    k_Voice_Err_Id_End,                        // Exclusive
    k_Voice_Err_Id_Max = k_Voice_Err_Id_End - 1, // Inclusive
    k_Voice_Err_Id_Lst = k_Voice_Err_Id_End - 1, // Inclusive
    k_Voice_Err_Id_Cnt = k_Voice_Err_Id_End
                     - k_Voice_Err_Id_Beg,     // Inclusive
    k_Voice_Err_Id_Ivd = k_Voice_Err_Id_End
};

#ifdef __cplusplus
}
#endif

#endif /* FKMG_VOICE_ERR_H */
