/**
 * @file ot_misc.c
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief
 * @version 0.1
 * @date 2023-07-25
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <openthread/platform/misc.h>
#include "stdio.h"
#include "cm3_mcu.h"

#define ms_sec(N)     (N*1000)

void otPlatReset(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    wdt_config_mode_t wdt_mode;
    wdt_config_tick_t wdt_cfg_ticks;

    wdt_mode.int_enable = 0;
    wdt_mode.reset_enable = 1;
    wdt_mode.lock_enable = 1;
    wdt_mode.prescale = WDT_PRESCALE_32;

    sys_set_retention_reg(6, 7);
    sys_set_retention_reg(7, 0);

    wdt_cfg_ticks.wdt_ticks = ms_sec(1200);
    wdt_cfg_ticks.int_ticks = ms_sec(0);
    wdt_cfg_ticks.wdt_min_ticks = ms_sec(1);

    Wdt_Start(wdt_mode, wdt_cfg_ticks, NULL);
    while (1);
    //NVIC_SystemReset();
}

otPlatResetReason otPlatGetResetReason(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    otPlatResetReason reason = OT_PLAT_RESET_REASON_UNKNOWN;

    return reason;
}

void otPlatWakeHost(void)
{
    // TODO: implement an operation to wake the host from sleep state.
}

void otPlatAssertFail(const char *aFilename, int aLineNumber)
{
    printf("otPlatAssertFail, %s @ %d\r\n", aFilename, aLineNumber);
}