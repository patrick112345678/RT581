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
/* PURPOSE: TP_R21_BV-25 ZigBee Router (gZR): joining to centralized network.
*/

#define ZB_TEST_NAME TP_R21_BV_25_GZR_C
#define ZB_TRACE_FILE_ID 40511

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

static const zb_ieee_addr_t g_ieee_addr_gzr = IEEE_ADDR_gZR;
static const zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;

static void send_reset_packet_count_msg(zb_uint8_t param);
static void send_retrieve_packet_count_msg_delayed(zb_uint8_t param);
static void send_retrieve_packet_count_msg(zb_uint8_t param);
static void start_transmit_counted_packets_delayed(zb_uint8_t param);
static void start_transmit_counted_packets(zb_uint8_t param);


static zb_uint16_t s_dut_short_addr;
static int s_transmit_started = 0;


MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("zdo_2_gzr_c");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    zb_set_long_address(g_ieee_addr_gzr);

    zb_zdo_set_aps_unsecure_join(ZB_TRUE);

#ifdef SECURITY_LEVEL
    zb_cert_test_set_security_level(SECURITY_LEVEL);
#endif
    //  ZB_CERT_HACKS().aps_security_off = ZB_TRUE;
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zr_role();

    zb_set_nvram_erase_at_start(ZB_TRUE);
    if (zboss_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
    }
    else
    {
        zboss_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    TRACE_MSG(TRACE_ERROR, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    if (0 == status)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));
            ZB_SCHEDULE_ALARM(send_reset_packet_count_msg, 0, TEST_SEND_RESET_PCK_CNT_MSG_DELAY);
            break;

        default:
            TRACE_MSG(TRACE_ERROR, "Unknown signal %hd", (FMT__H, sig));
        }
    }
    else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
    {
        TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, status));
    }

    if (param)
    {
        zb_buf_free(param);
    }
}


static void send_reset_packet_count_msg(zb_uint8_t param)
{
    zb_bufid_t buf = zb_buf_get_out();

    ZVUNUSED(param);

    TRACE_MSG(TRACE_APS3, ">>send_reset_packet_count_msg", (FMT__0));

    if (buf)
    {
        s_dut_short_addr = zb_address_short_by_ieee((zb_uint8_t *) g_ieee_addr_dut);

        //ZB_BUF_REUSE(param);
        tp_send_req_by_short(TP_RESET_PACKET_COUNT_CLID,
                             buf,//param,
                             ZB_TEST_PROFILE_ID,
                             s_dut_short_addr,
                             ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                             ZB_TEST_PROFILE_EP,
                             ZB_TEST_PROFILE_EP,
                             ZB_APSDE_TX_OPT_ACK_TX,
                             MAX_NWK_RADIUS);

        ZB_SCHEDULE_ALARM(send_retrieve_packet_count_msg_delayed, 0, 2 * ZB_TIME_ONE_SECOND);
    }
    else
    {
        TRACE_MSG(TRACE_APS3, "TEST FAILED: Could not get out buf!", (FMT__0));
    }

    TRACE_MSG(TRACE_APS3, "<<send_reset_packet_count_msg", (FMT__0));
}


static void send_retrieve_packet_count_msg_delayed(zb_uint8_t param)
{
    ZVUNUSED(param);
    zb_buf_get_out_delayed(send_retrieve_packet_count_msg);
}


static void send_retrieve_packet_count_msg(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APS3, ">>send_retrieve_packet_count_msg param %h", (FMT__H, param));

    if (s_transmit_started)
    {
        zb_buf_reuse(param);
    }

    tp_send_req_by_short(TP_RETRIEVE_PACKET_COUNT_CLID,
                         param,
                         ZB_TEST_PROFILE_ID,
                         s_dut_short_addr,
                         ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                         ZB_TEST_PROFILE_EP,
                         ZB_TEST_PROFILE_EP,
                         ZB_APSDE_TX_OPT_ACK_TX,
                         MAX_NWK_RADIUS);

    if (!s_transmit_started)
    {
        ZB_SCHEDULE_ALARM(start_transmit_counted_packets_delayed, 0, 2 * ZB_TIME_ONE_SECOND);
    }


    TRACE_MSG(TRACE_APS3, "<<send_retrieve_packet_count_msg", (FMT__0));
}


static void start_transmit_counted_packets_delayed(zb_uint8_t param)
{
    ZVUNUSED(param);
    zb_buf_get_out_delayed(start_transmit_counted_packets);
}


static void start_transmit_counted_packets(zb_uint8_t param)
{
    zb_tp_transmit_counted_packets_param_t *params;

    TRACE_MSG(TRACE_APS3, ">>start_transmit_counted_packets param %h", (FMT__H, param));

    params = zb_buf_get_tail(param,
                             sizeof(zb_tp_transmit_counted_packets_param_t));

    BUFFER_COUNTED_TEST_REQ_SET_DEFAULT(params);
    params->packets_number = 10;
    params->len = TEST_PACKET_SIZE;
    params->dst_addr = s_dut_short_addr;
    params->idle_time = 1000;

    s_transmit_started = 1;
    zb_tp_transmit_counted_packets_req_ext(param, send_retrieve_packet_count_msg);

    TRACE_MSG(TRACE_APS3, "<<start_transmit_counted_packets", (FMT__0));
}


/*! @} */

