#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cm3_mcu.h"
#include "FreeRTOS.h"
#include "task.h"

#include "hosal_rf.h"
#include "lmac15p4.h"
#include "cli.h"
#include "log.h"
#include "main.h"

void wdt_isr(void)
{
    Wdt_Kick();
}

void wdt_init(void)
{
    wdt_config_mode_t wdt_mode;
    wdt_config_tick_t wdt_cfg_ticks;

    wdt_mode.int_enable = 1;
    wdt_mode.reset_enable = 1;
    wdt_mode.lock_enable = 0;
    wdt_mode.prescale = WDT_PRESCALE_32;

    wdt_cfg_ticks.wdt_ticks = 5000 * 2000;
    wdt_cfg_ticks.int_ticks = 20 * 2000;
    wdt_cfg_ticks.wdt_min_ticks = 0;

    Wdt_Start(wdt_mode, wdt_cfg_ticks, wdt_isr);
}

int app_main(void)
{
    wdt_init();
    hosal_rf_init(HOSAL_RF_MODE_RUCI_CMD);
    lmac15p4_init(LMAC15P4_SUBG_FSK);

#if (CONFIG_PLATOFRM_ENABLE_SLEEP == 0)
    cli_init();
#endif

    app_task();
    return 0;
}
