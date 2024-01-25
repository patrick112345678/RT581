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
/* PURPOSE: TP/APS/BV-13 ZC/ZR- APS TX Multiple Data Frame
*/

#define ZB_TEST_NAME TP_APS_BV_13_ZC
#define ZB_TRACE_FILE_ID 40937

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"

static const zb_ieee_addr_t g_ieee_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
static const zb_ieee_addr_t g_zr_ieee_addr = {0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};


#define HW_SLEEP_ADDITIONAL 10


MAIN()
{
    ARGV_UNUSED;

    ZB_SET_TRAF_DUMP_ON();

    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("zdo_1_zc");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    /* let's always be coordinator */
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zc_role();

    zb_set_pan_id(0x1aaa);

    /* set ieee addr */
    zb_set_long_address(g_ieee_addr);
    MAC_ADD_VISIBLE_LONG((zb_uint8_t *) g_zr_ieee_addr);

    /* accept only one child */
    zb_set_max_children(1);

    /* turn off security */
    /* zb_cert_test_set_security_level(0); */

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


static void buffer_test_cb(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APS1, "buffer_test_cb", (FMT__H, param));
    if (param == ZB_TP_BUFFER_TEST_OK)
    {
        TRACE_MSG(TRACE_APS1, "buffer_test_cb: status OK", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_APS1, "buffer_test_cb: status ERROR()", (FMT__H, param));
    }
}


//! [zb_tp_transmit_counted_packets_req]
static void send_data(zb_uint8_t param)
{
    zb_bufid_t buf = zb_buf_get_out();
    ZVUNUSED(param);

    TRACE_MSG(TRACE_APS3, ">>send_data", (FMT__0));

    if (buf)
    {
        zb_tp_transmit_counted_packets_param_t *params;

        params = ZB_BUF_GET_PARAM(buf, zb_tp_transmit_counted_packets_param_t);

        BUFFER_COUNTED_TEST_REQ_SET_DEFAULT(params);

        params->len            = 0x0A;
        params->packets_number = 3;
        params->idle_time      = 2000; /* this time is in ms */
        params->dst_addr       = 0xFFFF;
        params->addr_mode      = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
        params->src_ep         = 0x01;
        params->dst_ep         = 0xF0;
        params->radius         = 0x07;

        TRACE_MSG(TRACE_APS3, "send_data: dst addr %d", (FMT__D, params->dst_addr));
        zb_tp_transmit_counted_packets_req(buf, buffer_test_cb);
    }
    else
    {
        TRACE_MSG(TRACE_INFO3, "TEST FAILED: Could not get out buf!", (FMT__0));
    }

    TRACE_MSG(TRACE_APS3, "<<send_data", (FMT__0));
}
//! [zb_tp_transmit_counted_packets_req]


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    TRACE_MSG(TRACE_ERROR, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    if (0 == status)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));

            ZB_SCHEDULE_ALARM(send_data, 0, (25 + HW_SLEEP_ADDITIONAL) * ZB_TIME_ONE_SECOND);
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

    zb_buf_free(param);
}

