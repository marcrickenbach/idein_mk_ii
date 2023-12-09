/** ****************************************************************************
 * @brief Idein id.
 */

#ifndef FKMG_IDEIN_ID_H
#define FKMG_IDEIN_ID_H

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
enum Idein_Id{
    k_Idein_Id_Beg,                   // Inclusive
    k_Idein_Id_Min = k_Idein_Id_Beg,    // Inclusive
    k_Idein_Id_1st = k_Idein_Id_Beg,    // Inclusive

    /* TODO: */
    k_Idein_Id_1 = k_Idein_Id_Beg,      // Suggest using pcb silkscreen name or
                                    // similar common reference
     k_Idein_Id_channel_1 = k_Idein_Id_1, // Use descriptive name, like
                                               // front panel name
    k_Idein_Id_2,
     k_Idein_Id_channel_2 = k_Idein_Id_2,

    k_Idein_Id_End,                   // Exclusive
    k_Idein_Id_Max = k_Idein_Id_End - 1,// Inclusive
    k_Idein_Id_Lst = k_Idein_Id_End - 1,// Inclusive
    k_Idein_Id_Cnt = k_Idein_Id_End
                 - k_Idein_Id_Beg,	// Inclusive
	k_Idein_Id_Ivd = k_Idein_Id_End
};

#ifdef __cplusplus
}
#endif

#endif /* FKMG_IDEIN_ID_H */
