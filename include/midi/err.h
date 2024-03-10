/** ****************************************************************************
 * @brief MIDI errors.
 *
 * The errors MIDI module generates.
 */

#ifndef FKMG_MIDI_ERR_H
#define FKMG_MIDI_ERR_H

#ifdef __cplusplus
extern "C" {
#endif

/* *****************************************************************************
 * Enums
 */

/* Errors generated. */
enum MIDI_Err_Id{
    k_MIDI_Err_Id_Beg,                   // Inclusive
    k_MIDI_Err_Id_Min = k_MIDI_Err_Id_Beg,// Inclusive
    k_MIDI_Err_Id_1st = k_MIDI_Err_Id_Beg,// Inclusive

    k_MIDI_Err_Id_None = k_MIDI_Err_Id_Beg,
    k_MIDI_Err_Id_Unknown,
    k_MIDI_Err_Id_Unimplemented,
    k_MIDI_Err_Id_Ptr_Invalid,
    k_MIDI_Err_Id_Configuration_Invalid,

    k_MIDI_Err_Id_End,                        // Exclusive
    k_MIDI_Err_Id_Max = k_MIDI_Err_Id_End - 1, // Inclusive
    k_MIDI_Err_Id_Lst = k_MIDI_Err_Id_End - 1, // Inclusive
    k_MIDI_Err_Id_Cnt = k_MIDI_Err_Id_End
                     - k_MIDI_Err_Id_Beg,     // Inclusive
    k_MIDI_Err_Id_Ivd = k_MIDI_Err_Id_End
};

#ifdef __cplusplus
}
#endif

#endif /* FKMG_MIDI_ERR_H */
