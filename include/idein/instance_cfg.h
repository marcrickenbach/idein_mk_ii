/** ****************************************************************************
 * @brief Idein instance configuration interface.
 */

#ifndef FKMG_IDEIN_INSTANCE_CFG_H
#define FKMG_IDEIN_INSTANCE_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/* *****************************************************************************
 * Includes
 */

#include <zephyr/kernel.h>

#include "listener_cb.h"

/* *****************************************************************************
 * Defines
 */

/* *****************************************************************************
 * Instance
 */

/* Forward references to prevent include interdependent items getting declared
 * out-of-order. */
struct Idein_Instance;

/* Everything needed to configure an instance. */
struct Idein_Instance_Cfg{
    /* Required: pointer to opaque instance to config. Lifetime of instance: as
     * long as in use, which is likely lifetime of system uptime. */
    struct Idein_Instance * p_inst;
    struct Pot_Instance * p_pot_inst; 
    struct DAC_Instance * p_dac_inst;

    
    /* Required: the implementation consists of 1 thread: the main state
     * machine, which handles operational states such as disconnected/connected,
     * and events such as sending/receiving usb packets. */
    struct{
        struct{
            /* Declare k_thread instance and point to it. Lifetime: p_inst
            * lifetime. */
            struct k_thread  * p_thread;
            /* Declare k_thread_stack_t instance using K_THREAD_STACK_DEFINE()
            * and point to it. Stack size probably doesn't need to be too big;
            * try 256 bytes. Lifetime: p_thread lifetime. */
            k_thread_stack_t * p_stack;
            /* Size of the stack in bytes. Due to possible use of stack guards,
             * etc., must use K_THREAD_STACK_SIZEOF(). */
            size_t stack_sz;
            /* User discretion. Can start out low (K_LOWEST_APPLICATION_THREAD_PRIO)
            * and increase priority from there. */
            int prio;
        }sm /* state machine */;
    }task;

    /* Required: for queuing state machine events (Idein_SM_Evt). */
    struct{
        /* State machine event message queue. */
        struct k_msgq * p_sm_evts;
    }msgq;

    // struct{
    //     /* HW Timer Cfg structs. */
    //     struct counter_alarm_cfg x_cfg;
    //     struct counter_alarm_cfg y_cfg;
    // }timer;



    /* Optional: asynchronous callback to call after interface initialized. The
     * callback is sent event with signal k_Idein_Sig_Instance_Initialized. NULL
     * skips callback. */
    Idein_Listener_Cb cb;
};

#ifdef __cplusplus
}
#endif

#endif /* FKMG_IDEIN_INSTANCE_CFG_H */
