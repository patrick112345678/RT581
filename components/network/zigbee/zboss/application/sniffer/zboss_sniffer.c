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
/*  PURPOSE: Application implementing ZBOSS sniffer basing on ZBOSS MAC.
 This is NOT an open-source ZBOSS sniffer.
 Open source ZBOSS sniffer works at TI CC2531 only and does not use ZBOSS MAC.
 This sniffer is to be included into ZBOSS SDK.
*/

#define ZB_TRACE_FILE_ID 40247

/* we use serial for output traffic dump */
#define ZB_HAVE_SERIAL

#include "zboss_api.h"
#include "zb_led_button.h"
#include "zb_sniffer.h"

#define ZB_SNIFFER_GET_LQI(packet) *((zb_uint8_t*)zb_buf_begin(packet) + zb_buf_len(packet) - 2)
#define ZB_SNIFFER_GET_RSSI(packet) *((zb_int8_t*)zb_buf_begin(packet) + zb_buf_len(packet) - 2 + 1)

#define GREEN_LED 1

/* Declare typedef for serial i/o ringbuffer */
ZB_RING_BUFFER_DECLARE(sniffer_io_buffer, zb_uint8_t, 4096);


static void sniffer_uart_rx_handler(zb_uint8_t symbol);
static void sniffer_process_uart_rx_cb(zb_uint8_t symbol);
static void sniffer_data_indication(zb_uint8_t param);

#define ZB_SNIFFER_PACKET_MAX_LENGTH 147
static zb_uint8_t gs_sniffer_out_pkt[ZB_SNIFFER_PACKET_MAX_LENGTH];

#ifndef ZB_USBC_SNIFFER
static sniffer_io_buffer_t gs_sniffer_io_buf;

#define PUT_BYTES zb_osif_serial_put_bytes
#define SET_IO_BUFFER(a, b) zb_osif_set_user_io_buffer(a, b)
#define SET_RX_CB zb_osif_set_uart_byte_received_cb
#define SERIAL_INIT zb_osif_serial_init

#else

#define PUT_BYTES zb_osif_userial_put_bytes
#define SET_IO_BUFFER(a, b)
#define SET_RX_CB zb_osif_set_userial_byte_received_cb
#define SERIAL_INIT zb_osif_userial_init

#endif

MAIN()
{
    ARGV_UNUSED;

    /* No need to init trace. Turn DUMP ON to init serial. */
    ZB_SET_TRACE_ON();
#ifndef ZB_USBC_SNIFFER
    ZB_SET_TRAF_DUMP_ON();
#else
    ZB_SET_TRAF_DUMP_OFF();
#endif
    ZB_INIT("zboss_sniffer");
    /* No need dump after serial has been inited */
    ZB_SET_TRACE_OFF();
    ZB_SET_TRAF_DUMP_OFF();

    zb_led_blink_on(ZB_LED_ARG_CREATE(GREEN_LED, ZB_LED_BLINK_PER_SEC));

    /* We actually do not need anything upper than MAC, but need a scheduler */
    if (zboss_start_in_sniffer_mode() == RET_OK)
    {
        /* scheduler infinity loop */
#ifndef ZB_USBC_SNIFFER
        zboss_main_loop();
#else
        while (!ZB_SCHEDULER_IS_STOP())
        {
            zb_osif_userial_poll();
            zb_sched_loop_iteration();
        }
#endif
    }

    MAIN_RETURN(0);
}


void zboss_signal_handler(zb_uint8_t param)
{
    /* Signal handler will be called only once for sure. */

    SET_RX_CB(sniffer_uart_rx_handler);
    /* Set up user buffer for serial i/o instead of stack serial buffer. */
    SET_IO_BUFFER((zb_byte_array_t *)&gs_sniffer_io_buf, 4096);
    SERIAL_INIT();
    zb_buf_free(param);
}


static void sniffer_uart_rx_handler(zb_uint8_t symbol)
{
#ifdef ZB_ALIEN_MAC
    zb_schedule_callback_from_alien(sniffer_process_uart_rx_cb, symbol);
#else
    ZB_SCHEDULE_APP_CALLBACK(sniffer_process_uart_rx_cb, symbol);
#endif
}

#define ZB_SNIFFER_PAUSE_CMD 0xAA
#define ZB_SNIFFER_STOP_CMD  0xBB

static void sniffer_process_uart_rx_cb(zb_uint8_t symbol)
{
    zb_ret_t ret = RET_BLOCKED;

    switch (symbol)
    {
    case ZB_SNIFFER_PAUSE_CMD:
        zb_led_blink_on(ZB_LED_ARG_CREATE(GREEN_LED, ZB_LED_BLINK_PER_SEC));
        zboss_sniffer_stop();
        break;
    case ZB_SNIFFER_STOP_CMD:
        zb_led_blink_on(ZB_LED_ARG_CREATE(GREEN_LED, ZB_LED_BLINK_PER_SEC));
        zboss_sniffer_stop();
        /* may implement it later      zb_osif_serial_purge(); */
        break;
    default:
        zb_led_blink_off(ZB_LED_ARG_CREATE(GREEN_LED, ZB_LED_BLINK_PER_SEC));
        zb_osif_led_on(GREEN_LED);
        ret = zboss_sniffer_set_channel_page(symbol);
        break;
    }

    if (ret == RET_OK)
    {
        zboss_sniffer_start(sniffer_data_indication);
    }
}

static void sniffer_data_indication(zb_uint8_t param)
{
    zb_uint32_t overflows;
    zb_uint16_t rest;
    zb_uint8_t  low, high;
    zb_uint8_t  lqi, rssi;
    zb_ushort_t pkt_actual_length;
    zb_ushort_t buf_length = zb_buf_len(param);
    /* Note: I put there back old code which works with all our platforms putting timestamp into the packet.
       For platform which used zb_timer_get_precise_time() and, thus, broke all others (guys, you were wrong!) need to implement in-packet timestamp.
     */
    zb_time_t   t_us = *ZB_BUF_GET_PARAM(param, zb_time_t);

    lqi = ZB_SNIFFER_GET_LQI(param);
    rssi = ZB_SNIFFER_GET_RSSI(param);

    /* Serial Protocol
       Every Packet looks like:

       magic 0x13AD - 2 bytes
       total len 1 byte
       timestamp:
         usec 4 bytes
         low 1 byte
         high 1 byte
         packet len ------ here packet in buf starts.
         packet data
         RSSI & Correlation.

      Magic | Overall length | Timestamp | Phy header 1 (current packet length) |
      Packet 1 + RSSI & Correlation | Phy header 2 | Packet 2 | ...
     */

    /* Magic Number */
    gs_sniffer_out_pkt[0] = 0xAD;
    gs_sniffer_out_pkt[1] = 0x13;

    /* total len - data length + 1 byte (for zb packet length) */
    gs_sniffer_out_pkt[2] = buf_length + 1;

    /* Timestamp */

    overflows = t_us / 2048;
    rest = t_us % 2048;
    low = rest % 0xff;
    high = rest >> 8;
    ZB_MEMCPY(gs_sniffer_out_pkt + 3, &overflows, sizeof(zb_uint32_t));
    gs_sniffer_out_pkt[7] = low;
    gs_sniffer_out_pkt[8] = high;

    /* Packet lenght and packet data */
    gs_sniffer_out_pkt[9] = buf_length;
    ZB_MEMCPY(gs_sniffer_out_pkt + 10, (zb_uint8_t *)zb_buf_begin(param), buf_length);

    /* RSSI, Correlation (LQI + FCS check)
       last byte of packet is LQI divied by two with high bit set
     */
    gs_sniffer_out_pkt[8 + buf_length] = rssi + 73;
    gs_sniffer_out_pkt[9 + buf_length] = (lqi / 2) | (1 << 7);

    pkt_actual_length = 10 + buf_length;
    PUT_BYTES(gs_sniffer_out_pkt, pkt_actual_length);
    zb_buf_free(param);
}


#ifdef ZB_USBC_SNIFFER
/**
   That function is called when USB discovered that host side is gone.
 */
void zb_mac_reset_at_transport_open(zb_uint8_t param)
{
    ZVUNUSED(param);
    zboss_sniffer_stop();
}
#endif
