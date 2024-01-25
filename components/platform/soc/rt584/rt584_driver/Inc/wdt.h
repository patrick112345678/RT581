/**************************************************************************//**
 * @file     wdt.h
 * @version
 * @brief    WacthDog TIMER driver header file
 *
 * @copyright
 ******************************************************************************/
#ifndef __WDT_TIMER_H__
#define __WDT_TIMER_H__

#include "RT584_cm33.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief WDT TIMER interrupt service routine callback for user application.
 *
 * @param
 *
 */
typedef void (*wdt_isr_handler_t)();


#define WDT_PRESCALE_1               0
#define WDT_PRESCALE_16              1
#define WDT_PRESCALE_32              3
#define WDT_PRESCALE_128             4
#define WDT_PRESCALE_256             2
#define WDT_PRESCALE_1024            5
#define WDT_PRESCALE_4096            6

/**@brief wdt config structure holding configuration settings for the wdt timer.
 */
typedef struct {
    uint8_t    int_enable     :1;
    uint8_t    reset_enable   :1;
    uint8_t    lock_enable    :1;
    uint8_t    prescale       :3;
    uint8_t    reserved       :2;
    uint32_t min_window;
    uint16_t clock_divider;
} wdt_config_mode_t;


/*Remark:
 *   WDT interrupt is "warning interrupt", that notify system it will reset soon.
 * so int_ticks should be less that wdt_ticks and should have enough time for
 * "reset error" handling.. int_ticks can NOT greater or equal then wdt_ticks.
 *
 *   If you set lock_enable mode, you can NOT change the watchdog setting anymore
 * before reset (after WDT lock_enable bit has been set). That is you must kick
 * the watchdog periodic to prevent reset event.
 *
 * Example:
 * Clock is 32M Hz. Set prescale is 32.. so WDT is 1M Hz.
 * If we set wdt_ticks 500000, and int_ticks 300
 * then we should kick WDT within 0.5 second, otherwise
 * the whole system will reset (if reset enable) after 0.5s
 * from the last kick and before reset. And before reset,
 * a warning reset interrupt will generate, system has 300 us
 * to save some important data or something..
 * We can ignore int_ticks and wdt_handler, if we don't set
 * int_enable mode.
 *
 */
extern void wdt_int_callback_register(wdt_isr_handler_t wdt_handler);

extern uint32_t wdt_start(
    wdt_config_mode_t wdt_mode,
    uint32_t wdt_ticks,
    uint32_t int_ticks,
    wdt_isr_handler_t wdt_isr_cb);


extern uint32_t wdt_stop(void);

__STATIC_INLINE void wdt_kick(void)
{
    WDT->WDT_KICK = WDT_KICK_VALUE;
}

__STATIC_INLINE void wdt_interrupt_clear(void)
{
    WDT->CLEAR =1;
}

__STATIC_INLINE uint32_t wdt_current_get(void)
{
    return (WDT->VALUE);
}

__STATIC_INLINE uint32_t wdt_occur_get(void)
{
    return (WDT->RST_OCCUR.bit.RESET_OCCUR);
}

__STATIC_INLINE void wdt_occur_set(uint8_t value)
{
     WDT->RST_OCCUR.bit.RESET_OCCUR = value;
}

#ifdef __cplusplus
}
#endif

#endif

