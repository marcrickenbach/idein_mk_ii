/** ****************************************************************************
 * @brief Events generated by Idein module.
 *
 * The public events Idein module generates.
 */

#ifndef FKMG_IDEIN_EVT_H
#define FKMG_IDEIN_EVT_H

#ifdef __cplusplus
extern "C" {
#endif

/* *****************************************************************************
 * Includes
 */

#include <zephyr/kernel.h>

#include "id.h"
#include "uart/evt.h"


/* *****************************************************************************
 * Events
 */

/* Signals that can be generated as part of an event. */
enum Idein_Evt_Sig{
    k_Idein_Evt_Sig_Beg,                        // Inclusive
    k_Idein_Evt_Sig_Min = k_Idein_Evt_Sig_Beg,    // Inclusive
    k_Idein_Evt_Sig_1st = k_Idein_Evt_Sig_Beg,    // Inclusive

    k_Idein_Evt_Sig_Instance_Initialized = k_Idein_Evt_Sig_Beg,
    #if CONFIG_FKMG_IDEIN_SHUTDOWN_ENABLED
    k_Idein_Evt_Sig_Instance_Deinitialized,
    #endif
    k_Idein_Evt_Sig_Step_Occurred,
    k_Idein_Evt_Sig_Pot_Value_Changed,
    k_Idein_Evt_Sig_Btn_Value_Changed,
    k_Idein_Evt_Sig_LED_Write_Ready,
    k_Idein_Evt_Sig_MIDI_Write_Ready,
    k_Idein_Evt_Sig_End,                        // Exclusive
    k_Idein_Evt_Sig_Max = k_Idein_Evt_Sig_End - 1,// Inclusive
    k_Idein_Evt_Sig_Lst = k_Idein_Evt_Sig_End - 1,// Inclusive
    k_Idein_Evt_Sig_Cnt = k_Idein_Evt_Sig_End
                      - k_Idein_Evt_Sig_Beg,	  // Inclusive
	k_Idein_Evt_Sig_Ivd = k_Idein_Evt_Sig_End
};

/* Forward references to prevent include interdependent items getting declared
 * out-of-order. */
struct Idein_Instance;

/* Data signal k_Idein_Evt_Sig_Instance_Initialized can generate. */
struct Idein_Evt_Data_Instance_Initialized{
    struct Idein_Instance * p_inst;
};

/* Data signal k_Idein_Evt_Sig_Step_Occurred can generate. */
struct Idein_Evt_Step_Occurred{
    enum Idein_Id id;
};

/* Data signal k_Idein_Evt_Sig_Pot_Value_Changed can generate*/
struct Idein_Evt_Pot_Value_Changed{
    enum Idein_Id id;
    uint32_t val; 
};


/* Data signal k_Idein_Evt_Sig_Btn_Value_Changed can generate*/
struct Idein_Evt_Btn_Value_Changed{
    // enum Btn_Id id;
    bool val; 
};


/* Data signal k_Idein_Evt_Sig_LED_Write_Ready can generate*/
struct Idein_Evt_LED_Write_Ready{
    enum Idein_Id id;
    uint8_t offset;
    uint16_t val; 
};

/* Data signal k_Idein_Evt_Sig_MIDI_Write_Ready can generate*/
struct Idein_Evt_MIDI_Write_Ready{
    enum UART_Id id;
    uint8_t midi_status; 
    uint8_t ctrl_byte;
    bool Idein;
    uint8_t step;
    uint8_t offset;
};

/* Events (i.e. signal + signal's data if any) that can be generated. */
struct Idein_Evt{
	enum Idein_Evt_Sig sig;
	union Idein_Evt_Data{
        struct Idein_Evt_Data_Instance_Initialized  initd;
        struct Idein_Evt_Step_Occurred              stepped;
        struct Idein_Evt_Pot_Value_Changed          pot_changed;
        struct Idein_Evt_Btn_Value_Changed          btn_changed;
        struct Idein_Evt_LED_Write_Ready            led_write;
        struct Idein_Evt_MIDI_Write_Ready           midi_write;

	}data;
};

#ifdef __cplusplus
}
#endif

#endif /* FKMG_IDEIN_EVT_H */