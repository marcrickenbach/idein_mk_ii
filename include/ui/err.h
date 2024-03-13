/** ****************************************************************************
 * @brief UI errors.
 *
 * The errors UI module generates.
 */

#ifndef FKMG_UI_ERR_H
#define FKMG_UI_ERR_H

#ifdef __cplusplus
extern "C" {
#endif

/* *****************************************************************************
 * Enums
 */

/* Errors generated. */
enum UI_Err_Id{
    k_UI_Err_Id_Beg,                   // Inclusive
    k_UI_Err_Id_Min = k_UI_Err_Id_Beg,// Inclusive
    k_UI_Err_Id_1st = k_UI_Err_Id_Beg,// Inclusive

    k_UI_Err_Id_None = k_UI_Err_Id_Beg,
    k_UI_Err_Id_Unknown,
    k_UI_Err_Id_Unimplemented,
    k_UI_Err_Id_Ptr_Invalid,
    k_UI_Err_Id_Configuration_Invalid,

    k_UI_Err_Id_End,                        // Exclusive
    k_UI_Err_Id_Max = k_UI_Err_Id_End - 1, // Inclusive
    k_UI_Err_Id_Lst = k_UI_Err_Id_End - 1, // Inclusive
    k_UI_Err_Id_Cnt = k_UI_Err_Id_End
                     - k_UI_Err_Id_Beg,     // Inclusive
    k_UI_Err_Id_Ivd = k_UI_Err_Id_End
};

#ifdef __cplusplus
}
#endif

#endif /* FKMG_UI_ERR_H */
