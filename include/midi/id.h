/** ****************************************************************************
 * @brief MIDI id.
 */

#ifndef FKMG_MIDI_ID_H
#define FKMG_MIDI_ID_H

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
enum MIDI_Id{
    k_MIDI_Id_Beg,                   // Inclusive
    k_MIDI_Id_Min = k_MIDI_Id_Beg,    // Inclusive
    k_MIDI_Id_1st = k_MIDI_Id_Beg,    // Inclusive

    MIDI_1 = k_MIDI_Id_Beg,      // Currently set up assuming 16 midi channels
    k_MIDI_Id_End,                   // Exclusive
    k_MIDI_Id_Max = k_MIDI_Id_End - 1,// Inclusive
    k_MIDI_Id_Lst = k_MIDI_Id_End - 1,// Inclusive
    k_MIDI_Id_Cnt = k_MIDI_Id_End
                 - k_MIDI_Id_Beg,	// Inclusive
	k_MIDI_Id_Ivd = k_MIDI_Id_End
};

#ifdef __cplusplus
}
#endif

#endif /* FKMG_MIDI_ID_H */
