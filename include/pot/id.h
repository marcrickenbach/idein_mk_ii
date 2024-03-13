/** ****************************************************************************
 * @brief Pot id.
 */

#ifndef FKMG_POT_ID_H
#define FKMG_POT_ID_H

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
enum Pot_Id{
    k_Pot_Id_Beg,                   // Inclusive
    k_Pot_Id_Min = k_Pot_Id_Beg,    // Inclusive
    k_Pot_Id_1st = k_Pot_Id_Beg,    // Inclusive

    POT_RV_2 = k_Pot_Id_Beg,      // Suggest using pcb silkscreen name or similar common reference
     RED_POT = POT_RV_2, // Use descriptive name, like front panel name
    POT_RV_4,
    GREEN_POT = POT_RV_4,
    POT_RV_5,
    BLUE_POT = POT_RV_5,
    POT_RV_3,
    IR_POT = POT_RV_3,
    k_Pot_Id_End,                   // Exclusive
    k_Pot_Id_Max = k_Pot_Id_End - 1,// Inclusive
    k_Pot_Id_Lst = k_Pot_Id_End - 1,// Inclusive
    k_Pot_Id_Cnt = k_Pot_Id_End
                 - k_Pot_Id_Beg,	// Inclusive
	k_Pot_Id_Ivd = k_Pot_Id_End
};

#ifdef __cplusplus
}
#endif

#endif /* FKMG_POT_ID_H */
