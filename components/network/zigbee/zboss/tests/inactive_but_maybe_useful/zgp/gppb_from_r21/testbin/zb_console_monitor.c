/* ZBOSS Zigbee software protocol stack
 *
 * Copyright (c) 2012-2020 DSR Corporation, Denver CO, USA.
 * www.dsr-zboss.com
 * www.dsr-corporation.com
 * All rights reserved.
 *
 * This is unpublished proprietary source code of DSR Corporation
 * The copyright notice does not evidence any actual or intended
 * publication of such source code.
 *
 * ZBOSS is a registered trademark of Data Storage Research LLC d/b/a DSR
 * Corporation
 *
 * Commercial Usage
 * Licensees holding valid DSR Commercial licenses may use
 * this file in accordance with the DSR Commercial License
 * Agreement provided with the Software or, alternatively, in accordance
 * with the terms contained in a written agreement between you and
 * DSR.
 */
/*  PURPOSE: Console monitor
*/

#define ZB_TRACE_FILE_ID 41391
#include "zb_common.h"
#include "zb_bufpool.h"
#include "zb_osif.h"
#include "zb_stm_serial.h"

#include "zb_console_monitor.h"

static zb_console_monitor_ctx_t gs_console_ctx;
static void zb_console_monitor_rx_handler(zb_uint8_t symbol);

void zb_console_monitor_sleep(zb_time_t timeout)
{
    /* CR: 12/16/2016: CR:MINOR Are you 146% sure timer is not started already?
       Maybe, check for it? */
    /* VP: 12/19/2016: add check to prevent timer double start */
    if (!ZB_CHECK_TIMER_IS_ON())
    {
        zb_timer_start(ZB_MILLISECONDS_TO_BEACON_INTERVAL(ZB_USECS_TO_MILLISECONDS(timeout)));
    }
    /* Block here */
    osif_sleep_using_transc_timer(timeout);
}

/* Non-blocking function */
void zb_console_monitor_init()
{
    /* Minimal ZBOSS init */
    zb_globals_init();
    ZB_PLATFORM_INIT();
    TRACE_INIT("monitor");
    ZB_ENABLE_ALL_INTER();

    zb_console_monitor_sleep(2000000);
    //SysCtrlDelay(10000000);

    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_handler);

    ZB_BZERO(&gs_console_ctx, sizeof(gs_console_ctx));
}

void zb_console_monitor_deinit()
{
    ZB_BZERO(&gs_console_ctx, sizeof(gs_console_ctx));
}

static void zb_console_monitor_send_invitation()
{
    zb_console_monitor_pkt_t monitor_pkt;
    zb_time_t time;

    /* Fill the packet header */
    monitor_pkt.hdr.sig[0] = 0xde;
    monitor_pkt.hdr.sig[1] = 0xad;
    monitor_pkt.hdr.h.len = sizeof(zb_console_monitor_pkt_t) - ZB_MAC_TRANSPORT_SIGNATURE_SIZE;
    monitor_pkt.hdr.h.type = ZB_MAC_TRANSPORT_TYPE_CONSOLE;
    time = ZB_TIMER_GET();
    ZB_HTOLE16((zb_uint8_t *)&monitor_pkt.hdr.h.time, (zb_uint8_t *)&time);

    /* Fill the payload */
    monitor_pkt.data.payload[0] = ZB_CONSOLE_MONITOR_INVITATION;

    /* OK, send data now */
    zb_osif_serial_put_bytes((zb_uint8_t *)&monitor_pkt, sizeof(zb_console_monitor_pkt_t));
}

/* Blocking function */
zb_uint8_t zb_console_monitor_get_cmd(zb_uint8_t *buffer, zb_uint8_t max_lenght)
{
    gs_console_ctx.buffer = buffer;
    gs_console_ctx.max_lenght = max_lenght;

    ZB_BZERO(gs_console_ctx.buffer, gs_console_ctx.max_lenght);
    gs_console_ctx.pos = 0;
    gs_console_ctx.cmd_recvd = ZB_FALSE;

    zb_console_monitor_send_invitation();

    /* Ready to receive  */
    gs_console_ctx.rx_ready = ZB_TRUE;

    while (!gs_console_ctx.cmd_recvd)
    {
        /* Wait until command has been received */
    }

    return gs_console_ctx.status;
}

static void zb_console_monitor_rx_handler(zb_uint8_t symbol)
{
    if (gs_console_ctx.rx_ready)
    {
        zb_ret_t status = RET_EXIT;

        if (gs_console_ctx.pos < gs_console_ctx.max_lenght)
        {
            if ('\r' == symbol)
            {
                /* Ignore this symbol */
            }
            else if ('\n' == symbol)
            {
                /* NULL-terminator for C string */
                *(zb_uint8_t *)(gs_console_ctx.buffer + gs_console_ctx.pos) = 0;
                status = RET_OK;
            }
            else
            {
                *(zb_uint8_t *)(gs_console_ctx.buffer + gs_console_ctx.pos) = symbol;
                gs_console_ctx.pos++;

                /* Check for overflow */
                if (gs_console_ctx.pos == gs_console_ctx.max_lenght)
                {
                    status = RET_OVERFLOW;
                }
            }
        }
        else
        {
            status = RET_OVERFLOW;
        }

        if (RET_EXIT != status)
        {
            gs_console_ctx.status = status;
            gs_console_ctx.rx_ready = ZB_FALSE;
            gs_console_ctx.cmd_recvd = ZB_TRUE;
        }
    }
}
