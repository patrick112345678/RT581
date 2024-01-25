/**
 * @file ot_uart.c
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief
 * @version 0.1
 * @date 2023-07-25
 *
 * @copyright Copyright (c) 2023
 *
 */

/**
 * @file
 *   This file implements the OpenThread platform abstraction for UART
 * communication.
 *
 */
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <openthread-core-config.h>
#include <openthread/config.h>

#include <openthread/platform/debug_uart.h>
#include <openthread/platform/logging.h>
#include <openthread_port.h>
#include <utils/uart.h>

#include <hosal_uart.h>

#include "cli.h"
#include "shell.h"

extern hosal_uart_dev_t uartstdio;

typedef struct _otUart
{
    uint16_t    start;
    uint16_t    end;
    uint32_t    recvLen;
    uint8_t     rxbuf[OT_UART_RX_BUFFSIZE];
} otUart_t;

static otUart_t otUart_var;

otError otPlatUartEnable(void)
{
    memset(&otUart_var, 0, sizeof(otUart_t));

    return OT_ERROR_NONE;
}


otError otPlatUartDisable(void)
{
    return OT_ERROR_NONE;
}

otError otPlatUartSend(const uint8_t *aBuf, uint16_t aBufLength)
{
    OT_CLI_UART_OUTPUT_LOCK();

    hosal_uart_send(&uartstdio, aBuf, aBufLength);
    otPlatUartSendDone();

    OT_CLI_UART_OUTPUT_UNLOCK();
    return OT_ERROR_NONE;
}

otError otPlatUartFlush(void)
{
    return OT_ERROR_NONE;
}

void ot_uartTask (ot_system_event_t sevent)
{
    if (!(OT_SYSETM_EVENT_UART_ALL_MASK & sevent))
    {
        return;
    }
#if !CFG_CPC_ENABLE
    if (OT_SYSTEM_EVENT_UART_RXD & sevent)
    {
        otPlatUartReceived(otUart_var.rxbuf, strlen((char *)otUart_var.rxbuf));
    }
#endif
}

static int _cli_cmd_ot(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{

    memset(otUart_var.rxbuf, 0x00, sizeof(otUart_var.rxbuf));
    if (argc > 1)
    {
        for (int i = 1; i < argc; i++)
        {
            strcat((char *)otUart_var.rxbuf, argv[i]);
            strcat((char *)otUart_var.rxbuf, " ");
        }
        strcat((char *)otUart_var.rxbuf, "\r\n");
    }
    OT_NOTIFY(OT_SYSTEM_EVENT_UART_RXD);

    return 0;
}

const sh_cmd_t g_cli_cmd_ot STATIC_CLI_CMD_ATTRIBUTE =
{
    .pCmd_name    = "ot",
    .pDescription = "Openthread command line",
    .cmd_exec     = _cli_cmd_ot,
};