/** ****************************************************************************
 * @brief Events generated by UART module.
 *
 * The public events UART module generates.
 */

#ifndef FKMG_UART_EVT_H
#define FKMG_UART_EVT_H

#ifdef __cplusplus
extern "C" {
#endif

/* *****************************************************************************
 * Includes
 */

#include <zephyr/kernel.h>

#include "id.h"

/* *****************************************************************************
 * Events
 */

/* Signals that can be generated as part of an event. */
enum UART_Evt_Sig{
    k_UART_Evt_Sig_Beg,                        // Inclusive
    k_UART_Evt_Sig_Min = k_UART_Evt_Sig_Beg,    // Inclusive
    k_UART_Evt_Sig_1st = k_UART_Evt_Sig_Beg,    // Inclusive

    k_UART_Evt_Sig_Instance_Initialized = k_UART_Evt_Sig_Beg,
    #if CONFIG_FKMG_UART_SHUTDOWN_ENABLED
    k_UART_Evt_Sig_Instance_Deinitialized,
    #endif
    k_UART_Evt_Sig_Changed, // Value changed
    k_UART_Evt_Sig_Write_Ready,
    k_UART_Evt_Sig_RX_Ready,

    k_UART_Evt_Sig_End,                        // Exclusive
    k_UART_Evt_Sig_Max = k_UART_Evt_Sig_End - 1,// Inclusive
    k_UART_Evt_Sig_Lst = k_UART_Evt_Sig_End - 1,// Inclusive
    k_UART_Evt_Sig_Cnt = k_UART_Evt_Sig_End
                      - k_UART_Evt_Sig_Beg,	  // Inclusive
	k_UART_Evt_Sig_Ivd = k_UART_Evt_Sig_End
};

/* Forward references to prevent include interdependent items getting declared
 * out-of-order. */
struct UART_Instance;

/* Data signal k_UART_Evt_Sig_Instance_Initialized can generate. */
struct UART_Evt_Data_Instance_Initialized{
    struct UART_Instance * p_inst;
};

/* Data signal k_UART_Evt_Sig_Write_Ready can generate. */
struct UART_Evt_Data_Write_MIDI{
    enum UART_Id id;
    uint8_t midi_status; 
    uint8_t ctrl_byte;
    bool seq;
    uint8_t step;
    uint8_t offset;
};

/* Data signal k_UART_Evt_Sig_Changed can generate. */
struct UART_Evt_Data_Changed{
    bool seq;
    uint8_t stp; 
    uint16_t val; 
};

/* Data signal k_UART_Evt_Sig_RX_Ready can generate. */
struct UART_Evt_UART_RX_Ready{
    uint8_t bytes[3]; 
};

/* Events (i.e. signal + signal's data if any) that can be generated. */
struct UART_Evt{
	enum UART_Evt_Sig sig;
	union UART_Evt_Data{
        struct UART_Evt_Data_Instance_Initialized   initd;
        struct UART_Evt_Data_Write_MIDI             midi_write;
        struct UART_Evt_UART_RX_Ready               midi_command;
	}data;
};

#ifdef __cplusplus
}
#endif

#endif /* FKMG_UART_EVT_H */
