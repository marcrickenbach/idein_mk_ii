/** ****************************************************************************
 * @brief Idein Step id.
 */

#ifndef FKMG_IDEIN_STEP_ID_H
#define FKMG_IDEIN_STEP_ID_H

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
enum Idein_Step_Id{
    k_Idein_Step_Id_Beg,                   // Inclusive
    k_Idein_Step_Id_Min = k_Idein_Step_Id_Beg,    // Inclusive
    k_Idein_Step_Id_1st = k_Idein_Step_Id_Beg,    // Inclusive

    /* TODO: */
    k_Idein_Step_Id_0 = k_Idein_Step_Id_Beg,      // Suggest using pcb silkscreen name or

    k_Idein_Step_Id_End,     // Exclusive
    k_Idein_Step_Id_Max = k_Idein_Step_Id_End - 1,// Inclusive
    k_Idein_Step_Id_Lst = k_Idein_Step_Id_End - 1,// Inclusive
    k_Idein_Step_Id_Cnt = k_Idein_Step_Id_End
                 - k_Idein_Step_Id_Beg,	// Inclusive
	k_Idein_Step_Id_Ivd = k_Idein_Step_Id_End
};

#ifdef __cplusplus
}
#endif

#endif /* FKMG_IDEIN_STEP_ID_H */
