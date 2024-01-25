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
/* PURPOSE: ZED
*/

#define ZB_TEST_NAME TP_R22_CCB_2673_GZED

#define ZB_TRACE_FILE_ID 63975
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "../common/zb_cert_test_globals.h"
#include "test_common.h"

static const zb_ieee_addr_t g_ieee_addr_gzed = IEEE_ADDR_gZED;
static zb_ieee_addr_t g_ieee_addr_gzr1 = IEEE_ADDR_gZR1;
static zb_ieee_addr_t g_ieee_addr_dutzr3 = IEEE_ADDR_DUT_ZR3;

MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("zdo_5_gzed");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    zb_set_long_address(g_ieee_addr_gzed);
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zed_role();

    /* set ieee addr */
    zb_set_long_address(g_ieee_addr_gzed);

    zb_set_rx_on_when_idle(ZB_TRUE);

    MAC_ADD_INVISIBLE_SHORT(0x0000);
    MAC_ADD_VISIBLE_LONG(g_ieee_addr_dutzr3);

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


static void send_test_buffer_req(zb_uint8_t param)
{
    zb_buffer_test_req_param_t *req_param;

    TRACE_MSG(TRACE_APP1, ">> send_test_buffer_req param %d", (FMT__D, param));

    if (param == 0)
    {
        zb_buf_get_out_delayed(send_test_buffer_req);
    }
    else
    {
        req_param = ZB_BUF_GET_PARAM(param, zb_buffer_test_req_param_t);
        BUFFER_TEST_REQ_SET_DEFAULT(req_param);

        req_param->len      = 0x10;
        req_param->dst_addr = zb_address_short_by_ieee(g_ieee_addr_gzr1);

        zb_tp_buffer_test_request(param, NULL);
    }
}


static void get_dst_addr_cb(zb_uint8_t param)
{
    zb_zdo_nwk_addr_resp_head_t *resp = (zb_zdo_nwk_addr_resp_head_t *)zb_buf_begin(param);

    TRACE_MSG(TRACE_APP1, ">> get_dst_addr_cb param %d", (FMT__D, param));

    if (resp->status == ZB_ZDP_STATUS_SUCCESS)
    {
        ZB_SCHEDULE_ALARM(send_test_buffer_req, 0, 20 * ZB_TIME_ONE_SECOND);
        ZB_SCHEDULE_ALARM(send_test_buffer_req, 0, 60 * ZB_TIME_ONE_SECOND);
        ZB_SCHEDULE_ALARM(send_test_buffer_req, 0, 80 * ZB_TIME_ONE_SECOND);
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "nwk_addr_req failed", (FMT__0));
    }

    zb_buf_free(param);
}


static void get_dst_addr(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> get_dst_addr param %d", (FMT__D, param));

    if (param == 0)
    {
        zb_buf_get_out_delayed(get_dst_addr);
    }
    else
    {
        zb_zdo_nwk_addr_req_param_t *req_param = ZB_BUF_GET_PARAM(param, zb_zdo_nwk_addr_req_param_t);

        req_param->dst_addr = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
        ZB_IEEE_ADDR_COPY(req_param->ieee_addr, g_ieee_addr_gzr1);
        req_param->start_index = 0;
        req_param->request_type = 0x00;

        zb_zdo_nwk_addr_req(param, get_dst_addr_cb);
    }
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

            ZB_SCHEDULE_ALARM(get_dst_addr, 0, 20 * ZB_TIME_ONE_SECOND);
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
        TRACE_MSG(TRACE_ERROR, "Device START FAILED", (FMT__0));
    }

    zb_buf_free(param);
}

