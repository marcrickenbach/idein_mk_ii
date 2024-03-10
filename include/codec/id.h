/** ****************************************************************************
 * @brief Codec id.
 */

#ifndef FKMG_CODEC_ID_H
#define FKMG_CODEC_ID_H

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
enum Codec_Id{
    k_Codec_Id_Beg,                   // Inclusive
    k_Codec_Id_Min = k_Codec_Id_Beg,    // Inclusive
    k_Codec_Id_1st = k_Codec_Id_Beg,    // Inclusive

    /* TODO: */
    k_Codec_Id_1 = k_Codec_Id_Beg,      // Suggest using pcb silkscreen name or
                                    // similar common reference
     k_Codec_Id_descriptive_name_1 = k_Codec_Id_1, // Use descriptive name, like
                                               // front panel name
    k_Codec_Id_2,
    k_Codec_Id_descriptive_name_2 = k_Codec_Id_2,
    k_Codec_Id_3,
    k_Codec_Id_descriptive_name_3 = k_Codec_Id_3,
    /* etc */

    k_Codec_Id_End,                   // Exclusive
    k_Codec_Id_Max = k_Codec_Id_End - 1,// Inclusive
    k_Codec_Id_Lst = k_Codec_Id_End - 1,// Inclusive
    k_Codec_Id_Cnt = k_Codec_Id_End
                 - k_Codec_Id_Beg,	// Inclusive
	k_Codec_Id_Ivd = k_Codec_Id_End
};

#ifdef __cplusplus
}
#endif

#endif /* FKMG_CODEC_ID_H */
