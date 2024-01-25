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


#define ZB_TRACE_FILE_ID 40362
#include "zb_common.h"
#include "zb_bufpool.h"
#include "zb_osif.h"
#include "zb_time.h"

#include "zb_console_monitor.h"

#define API_WAIT_TIMEOUT_MS -1

static void console_monitor_transport_init();
static void console_monitor_transport_configure();
static void console_monitor_transport_deinit();

static void console_monitor_wait_command();
static void console_monitor_put_bytes(zb_uint8_t *buf, zb_short_t len);

static zb_console_monitor_ctx_t gs_console_ctx;

/* Workaround for nRF team. To be deleted when implemented. */
/* #if defined ZB_NRF_TRACE_RX_ENABLE */
/* #error "Declare ZB_MULTITEST_CONSOLE_SLEEP_TIMEOUT in zb_vendor.h" */
/* #endif */

void zb_console_monitor_sleep(zb_time_t timeout)
{
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
    /* For platforms which experience power/UART sync troubles due to remote side behavior */
    /* zb_osif_busy_loop_delay(ZB_MULTITEST_HW_INIT_WAIT); */

    /* Minimal ZBOSS init */
    zb_globals_init();
    console_monitor_transport_init();
    ZB_PLATFORM_INIT();
    /* HACK: Disable timer stop - have sync delays below! */
    zb_timer_disable_stop();
    TRACE_INIT("monitor");

    ZB_ENABLE_ALL_INTER();

    ZB_BZERO(&gs_console_ctx, sizeof(gs_console_ctx));

    console_monitor_transport_configure();
}

void zb_console_monitor_deinit()
{
    console_monitor_transport_deinit();
    /* HACK: Enable timer stop back! */
    TRACE_DEINIT();
    /* By default - keep the timer always active for routers. */
#ifdef ZB_ED_ROLE
    zb_timer_enable_stop();
#endif /* ZB_ED_ROLE */

    ZB_BZERO(&gs_console_ctx, sizeof(gs_console_ctx));
}

void zb_console_monitor_send_data(zb_uint8_t *buffer, zb_uint8_t size)
{
    zb_console_monitor_pkt_t monitor_pkt;
    zb_time_t time;
    zb_uindex_t i;

    ZB_ASSERT (size <= ZB_CONSOLE_MONITOR_MAX_PAYLOAD_SIZE);

    /* Fill the packet header */
    monitor_pkt.hdr.sig[0] = 0xde;
    monitor_pkt.hdr.sig[1] = 0xad;
    monitor_pkt.hdr.h.len = sizeof(zb_console_monitor_pkt_t) - ZB_MAC_TRANSPORT_SIGNATURE_SIZE
                            - ZB_CONSOLE_MONITOR_MAX_PAYLOAD_SIZE + size;
    monitor_pkt.hdr.h.type = ZB_MAC_TRANSPORT_TYPE_CONSOLE;
    time = ZB_TIMER_GET();
    ZB_HTOLE16((zb_uint8_t *)&monitor_pkt.hdr.h.time, (zb_uint8_t *)&time);

    /* Fill the payload */
    for (i = 0; i < size; i++)
    {
        monitor_pkt.data.payload[i] = buffer[i];
    }

    /* OK, send data now */
    console_monitor_put_bytes((zb_uint8_t *)&monitor_pkt, monitor_pkt.hdr.h.len + ZB_MAC_TRANSPORT_SIGNATURE_SIZE);
}

static void zb_console_monitor_send_invitation()
{
    zb_console_monitor_pkt_t monitor_pkt;
    zb_time_t time;

    /* Fill the packet header */
    monitor_pkt.hdr.sig[0] = 0xde;
    monitor_pkt.hdr.sig[1] = 0xad;
    monitor_pkt.hdr.h.len = sizeof(zb_console_monitor_pkt_t) - ZB_MAC_TRANSPORT_SIGNATURE_SIZE -
                            ZB_CONSOLE_MONITOR_MAX_PAYLOAD_SIZE + ZB_CONSOLE_MONITOR_INVITATION_SIZE;
    monitor_pkt.hdr.h.type = ZB_MAC_TRANSPORT_TYPE_CONSOLE;
    time = ZB_TIMER_GET();
    ZB_HTOLE16((zb_uint8_t *)&monitor_pkt.hdr.h.time, (zb_uint8_t *)&time);

    /* Fill the payload */
    monitor_pkt.data.payload[0] = ZB_CONSOLE_MONITOR_INVITATION;

    /* OK, send data now */
    console_monitor_put_bytes((zb_uint8_t *)&monitor_pkt, monitor_pkt.hdr.h.len + ZB_MAC_TRANSPORT_SIGNATURE_SIZE);
}

#ifndef UNIX
static zb_bool_t is_tmo_expired(zb_time_t t0, zb_time_t tmo)
{
    return (ZB_TIME_GE(osif_transceiver_time_get(), ZB_TIME_ADD(t0, tmo)));
}
#endif /* UNIX */

/* Blocking function */
zb_uint8_t zb_console_monitor_get_cmd(zb_uint8_t *buffer, zb_uint8_t max_length)
{
    gs_console_ctx.buffer = buffer;
    gs_console_ctx.max_length = max_length;

    ZB_BZERO(gs_console_ctx.buffer, gs_console_ctx.max_length);
    gs_console_ctx.pos = 0;
    gs_console_ctx.cmd_recvd = ZB_FALSE;

    zb_console_monitor_sleep(ZB_MULTITEST_CONSOLE_SLEEP_TIMEOUT);

    /* Ready to receive  */
    gs_console_ctx.rx_ready = ZB_TRUE;

    while (!gs_console_ctx.cmd_recvd)
    {
        zb_console_monitor_send_invitation();

#if defined ZB_NRF_52
        ZB_OSIF_SERIAL_FLUSH();
#endif /* ZB_NRF_52 */

        /* Wait until command has been received */
        console_monitor_wait_command();
    }

    return gs_console_ctx.status;
}

static void zb_console_monitor_rx_handler(zb_uint8_t symbol)
{
    /* Echo */
    /* zb_osif_serial_put_bytes(&symbol, 1); */

    if (gs_console_ctx.rx_ready)
    {
        zb_ret_t status = RET_EXIT;

        if (gs_console_ctx.pos < gs_console_ctx.max_length)
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
                if (gs_console_ctx.pos == gs_console_ctx.max_length)
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

#if defined(ZB_MULTITEST_USE_STDIO)
static void console_monitor_transport_init()
{
}


static void console_monitor_transport_configure()
{
    gs_console_ctx.input_file = zb_osif_file_stdin();
    gs_console_ctx.output_file = zb_osif_file_stdout();
}


static void console_monitor_transport_deinit()
{
}


static void console_monitor_wait_command()
{
    zb_uint8_t buf[256];
    zb_uindex_t symbol_index = 0;
    zb_int_t bytes_to_process = 0;

    /* NOTE: this implementation supports only stdio.
     * It should be modified if it is required to support arbitrary files */
    bytes_to_process = zb_osif_stream_read(gs_console_ctx.input_file, buf, sizeof(buf));

    for (symbol_index = 0; symbol_index < bytes_to_process; symbol_index++)
    {
        zb_console_monitor_rx_handler(buf[symbol_index]);
    }
}


static void console_monitor_put_bytes(zb_uint8_t *buf, zb_short_t len)
{
    zb_osif_stream_write(gs_console_ctx.output_file, buf, len);
}

#else
void zb_osif_set_uart_byte_received_cb(zb_callback_t hnd);

static void console_monitor_transport_init()
{
#if defined UNIX
    zb_mac_transport_init();
#endif
}


static void console_monitor_transport_configure()
{
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_handler);
}


static void console_monitor_transport_deinit()
{
#if defined ZB_NSNG
    ns_api_deinit(&ZB_IOCTX().api_ctx);
#endif
}


static void console_monitor_wait_command()
{
#if defined UNIX
    ns_api_wait(&ZB_IOCTX().api_ctx, API_WAIT_TIMEOUT_MS);
#else /* defined UNIX */
    zb_time_t t0 = osif_transceiver_time_get();

    /* Block until sleep tmo expired or received cmd */
    while (!(is_tmo_expired(t0, ZB_MULTITEST_CONSOLE_SLEEP_TIMEOUT) ||
             gs_console_ctx.cmd_recvd))
    {
        /* Do nothing */
    }
#endif
}


static void console_monitor_put_bytes(zb_uint8_t *buf, zb_short_t len)
{
    zb_osif_serial_put_bytes(buf, len);
}

#endif