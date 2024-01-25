#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cm3_mcu.h"
#include "FreeRTOS.h"
#include "task.h"
#include "openthread_port.h"
#include "hosal_rf.h"
#include "lmac15p4.h"
#include "cli.h"
#include "log.h"
#include "main.h"

void gpio_31_isr_handle_cb(uint32_t pin, void *isr_param)
{

}

int app_main(void)
{
    hosal_rf_init(HOSAL_RF_MODE_RUCI_CMD);
#if OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
    lmac15p4_init(LMAC15P4_SUBG_FSK);
#endif

#if (CONFIG_PLATOFRM_ENABLE_SLEEP == 0)
    cli_init();
#endif
    otrStart();
    /*Check have HUN module?*/
    gpio_cfg_input(31, 0);
    /*Check have AR8 MCU?*/
    gpio_cfg_input(30, 0);
    pin_set_pullopt(29, MODE_PULLUP_10K);

    app_uart_init();

    if (das_hex_cmd_status_check())
    {
        das_hex_cmd_command_bootup();
        das_get_meter_id_init();
    }
    else
    {
        log_info("DAS SubG Thread Init ability FTD \n");
    }

    app_task();
    return 0;
}
