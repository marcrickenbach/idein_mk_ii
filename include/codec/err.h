/** ****************************************************************************
 * @brief Codec errors.
 *
 * The errors Codec module generates.
 */

#ifndef FKMG_CODEC_ERR_H
#define FKMG_CODEC_ERR_H

#ifdef __cplusplus
extern "C" {
#endif

/* *****************************************************************************
 * Enums
 */

/* Errors generated. */
enum Codec_Err_Id{
    k_Codec_Err_Id_Beg,                   // Inclusive
    k_Codec_Err_Id_Min = k_Codec_Err_Id_Beg,// Inclusive
    k_Codec_Err_Id_1st = k_Codec_Err_Id_Beg,// Inclusive

    k_Codec_Err_Id_None = k_Codec_Err_Id_Beg,
    k_Codec_Err_Id_Unknown,
    k_Codec_Err_Id_Unimplemented,
    k_Codec_Err_Id_Ptr_Invalid,
    k_Codec_Err_Id_Configuration_Invalid,

    k_Codec_Err_Id_End,                        // Exclusive
    k_Codec_Err_Id_Max = k_Codec_Err_Id_End - 1, // Inclusive
    k_Codec_Err_Id_Lst = k_Codec_Err_Id_End - 1, // Inclusive
    k_Codec_Err_Id_Cnt = k_Codec_Err_Id_End
                     - k_Codec_Err_Id_Beg,     // Inclusive
    k_Codec_Err_Id_Ivd = k_Codec_Err_Id_End
};

#ifdef __cplusplus
}
#endif

#endif /* FKMG_CODEC_ERR_H */
