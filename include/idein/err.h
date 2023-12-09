/** ****************************************************************************
 * @brief Idein errors.
 *
 * The errors Idein module generates.
 */

#ifndef FKMG_IDEIN_ERR_H
#define FKMG_IDEIN_ERR_H

#ifdef __cplusplus
extern "C" {
#endif

/* *****************************************************************************
 * Enums
 */

/* Errors generated. */
enum Idein_Err_Id{
    k_Idein_Err_Id_Beg,                   // Inclusive
    k_Idein_Err_Id_Min = k_Idein_Err_Id_Beg,// Inclusive
    k_Idein_Err_Id_1st = k_Idein_Err_Id_Beg,// Inclusive

    k_Idein_Err_Id_None = k_Idein_Err_Id_Beg,
    k_Idein_Err_Id_Unknown,
    k_Idein_Err_Id_Unimplemented,
    k_Idein_Err_Id_Ptr_Invalid,
    k_Idein_Err_Id_Configuration_Invalid,

    k_Idein_Err_Id_End,                        // Exclusive
    k_Idein_Err_Id_Max = k_Idein_Err_Id_End - 1, // Inclusive
    k_Idein_Err_Id_Lst = k_Idein_Err_Id_End - 1, // Inclusive
    k_Idein_Err_Id_Cnt = k_Idein_Err_Id_End
                     - k_Idein_Err_Id_Beg,     // Inclusive
    k_Idein_Err_Id_Ivd = k_Idein_Err_Id_End
};

#ifdef __cplusplus
}
#endif

#endif /* FKMG_IDEIN_ERR_H */
