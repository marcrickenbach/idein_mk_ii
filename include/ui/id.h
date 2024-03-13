/** ****************************************************************************
 * @brief UI id.
 */

#ifndef FKMG_UI_ID_H
#define FKMG_UI_ID_H

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
enum UI_Id{
    k_UI_Id_Beg,                   // Inclusive
    k_UI_Id_Min = k_UI_Id_Beg,    // Inclusive
    k_UI_Id_1st = k_UI_Id_Beg,    // Inclusive

    /* TODO: */
    k_UI_Id_1 = k_UI_Id_Beg,      // Suggest using pcb silkscreen name or
                                    // similar common reference
     k_UI_Id_descriptive_name_1 = k_UI_Id_1, // Use descriptive name, like
                                               // front panel name
    k_UI_Id_2,
    k_UI_Id_descriptive_name_2 = k_UI_Id_2,
    k_UI_Id_3,
    k_UI_Id_descriptive_name_3 = k_UI_Id_3,
    /* etc */

    k_UI_Id_End,                   // Exclusive
    k_UI_Id_Max = k_UI_Id_End - 1,// Inclusive
    k_UI_Id_Lst = k_UI_Id_End - 1,// Inclusive
    k_UI_Id_Cnt = k_UI_Id_End
                 - k_UI_Id_Beg,	// Inclusive
	k_UI_Id_Ivd = k_UI_Id_End
};

#ifdef __cplusplus
}
#endif

#endif /* FKMG_UI_ID_H */
