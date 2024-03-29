/** ****************************************************************************
 * @brief Internal state machine events generated by Idein.
 */

#ifndef FKMG_IDEIN_SM_EVT_H
#define FKMG_IDEIN_SM_EVT_H

#ifdef __cplusplus
extern "C" {
#endif

/* *****************************************************************************
 * Includes
 */

#include <zephyr/kernel.h>

#include "../id.h"
#include "../instance_cfg.h"

#include "../../pot/id.h"

/* *****************************************************************************
 * Events
 */

/* Signals that can be generated as part of an event. */
enum Idein_SM_Evt_Sig{
    k_Idein_SM_Evt_Sig_Beg,                           // Inclusive
    k_Idein_SM_Evt_Sig_Min = k_Idein_SM_Evt_Sig_Beg,    // Inclusive
    k_Idein_SM_Evt_Sig_1st = k_Idein_SM_Evt_Sig_Beg,    // Inclusive

	k_Idein_SM_Evt_Sig_Init_Instance = k_Idein_SM_Evt_Sig_Beg,
    #if CONFIG_FKMG_IDEIN_ALLOW_SHUTDOWN
	k_Idein_SM_Evt_Sig_Deinit_Instance,
    #endif
    k_Idein_SM_Evt_Timer_Elapsed,
    k_Idein_SM_Evt_Sig_Pot_Value_Changed,
    k_Idein_SM_Evt_Sig_UART_RX_Received,
    k_Idein_SM_Evt_Sig_Button_Pressed,
    k_Idein_SM_Evt_Sig_Sensor_Read,

    k_Idein_SM_Evt_Sig_End,                           // Exclusive
    k_Idein_SM_Evt_Sig_Max = k_Idein_SM_Evt_Sig_End - 1,// Inclusive
    k_Idein_SM_Evt_Sig_Lst = k_Idein_SM_Evt_Sig_End - 1,// Inclusive
    k_Idein_SM_Evt_Sig_Cnt = k_Idein_SM_Evt_Sig_End
                         - k_Idein_SM_Evt_Sig_Beg,	// Inclusive
	k_Idein_SM_Evt_Sig_Ivd = k_Idein_SM_Evt_Sig_End
};

/* Data signal k_Idein_SM_Evt_Sig_Init_Instance can generate. */
struct Idein_SM_Evt_Sig_Init_Instance{
    struct Idein_Instance_Cfg cfg;
};


/* Data signal k_Idein_SM_Evt_Timer_Elapsed can generate. */
struct Idein_SM_Evt_Sig_Timer_Elapsed{
    enum Idein_Id id;
    bool edge; 
};

/* Data signal k_Idein_SM_Evt_Sig_Pot_Value_Changed can generate. */
struct Idein_SM_Evt_Sig_Pot_Value_Changed{
    uint32_t val[5]; 
};


/* Data signal k_Idein_SM_Evt_Sig_UART_RX_Received can generate. */
struct Idein_SM_Evt_Sig_UART_RX_RECEIVED{
    enum Idein_Id id;
    uint8_t bytes[3]; 
};

/* Data signal k_Idein_SM_Evt_Sig_Read_New can generate. */
struct Idein_SM_Evt_Sig_Read_New{
    uint32_t read_values[4];
};

/* Data signal k_Idein_SM_Evt_Sig_Button_Pressed can generate. */
struct Idein_SM_Evt_Sig_Button_Pressed{
    uint8_t portA_state;
    uint8_t portB_state;
    int64_t timestamp;
};


/* Events (i.e. signal + signal's data if any) Idein State Machine generates. */
struct Idein_SM_Evt{
	enum Idein_SM_Evt_Sig sig;
	union Idein_SM_Evt_Data{
        struct Idein_SM_Evt_Sig_Init_Instance        init_inst;
        struct Idein_SM_Evt_Sig_Pot_Value_Changed    pot_changed;
        struct Idein_SM_Evt_Sig_UART_RX_RECEIVED     midi_cmd; 
        struct Idein_SM_Evt_Sig_Button_Pressed       btn_pressed;
        struct Idein_SM_Evt_Sig_Read_New             sensor_read;
	}data;
};

#ifdef __cplusplus
}
#endif

#endif /* FKMG_IDEIN_SM_EVT_H */
