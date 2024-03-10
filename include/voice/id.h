/** ****************************************************************************
 * @brief Voice id.
 */

#ifndef FKMG_VOICE_ID_H
#define FKMG_VOICE_ID_H

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
enum Voice_Id{
    k_Voice_Id_Beg,                   // Inclusive
    k_Voice_Id_Min = k_Voice_Id_Beg,    // Inclusive
    k_Voice_Id_1st = k_Voice_Id_Beg,    // Inclusive

    Voice_1 = k_Voice_Id_Beg,      // Currently set up assuming 16 Voice channels
    k_Voice_Id_End,                   // Exclusive
    k_Voice_Id_Max = k_Voice_Id_End - 1,// Inclusive
    k_Voice_Id_Lst = k_Voice_Id_End - 1,// Inclusive
    k_Voice_Id_Cnt = k_Voice_Id_End
                 - k_Voice_Id_Beg,	// Inclusive
	k_Voice_Id_Ivd = k_Voice_Id_End
};

#ifdef __cplusplus
}
#endif

#endif /* FKMG_VOICE_ID_H */
