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
/* PURPOSE: 11.12 TP/ZDO/BV-12  ZC-ZDO-Transmit Bind/Unbind_req. End
device 2
*/

#define ZB_TEST_NAME TP_ZDO_BV_12_ZED2
#define ZB_TRACE_FILE_ID 40808

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "tp_zdo_bv-12_common.h"
#include "../common/zb_cert_test_globals.h"

static const zb_ieee_addr_t g_ieee_addr_ed1 = IEEE_ADDR_ED1;
static const zb_ieee_addr_t g_ieee_addr_ed2 = IEEE_ADDR_ED2;

static void test_nwk_addr_req_for_zed1_delayed(zb_uint8_t unused);
static void test_nwk_addr_req_for_zed1(zb_uint8_t param);

static zb_af_simple_desc_1_1_t g_test_simple_desc;

MAIN()
{
    ARGV_UNUSED;

#ifdef APS_SECUR
    aps_secure = 1;
#endif

    ZB_INIT("zdo_zed2");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    zb_set_long_address(g_ieee_addr_ed2);

    /* zb_cert_test_set_security_level(0); */

    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zed_role();
    zb_set_rx_on_when_idle(ZB_FALSE);

    zb_set_simple_descriptor((zb_af_simple_desc_1_1_t *)&g_test_simple_desc,
                             TEST_ED2_EP /* endpoint */,        0x0103 /* app_profile_id */,
                             0xaaaa /* app_device_id */,        0x0   /* app_device_version*/,
                             0x1 /* app_input_cluster_count */, 0x1 /* app_output_cluster_count */);
    zb_set_input_cluster_id((zb_af_simple_desc_1_1_t *)&g_test_simple_desc, 0,  0x001c);
    zb_set_output_cluster_id((zb_af_simple_desc_1_1_t *)&g_test_simple_desc, 0,  0x0054);
    zb_add_simple_descriptor((zb_af_simple_desc_1_1_t *)&g_test_simple_desc);

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

static void test_nwk_addr_req_for_zed1_delayed(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    if (zb_buf_get_out_delayed(test_nwk_addr_req_for_zed1) != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "test_nwk_addr_req_for_zed1_delayed: buffer allocation error", (FMT__0));
    }
}

static void test_nwk_addr_req_for_zed1(zb_uint8_t param)
{
    zb_zdo_nwk_addr_req_param_t *req_param;
    TRACE_MSG(TRACE_APS1, ">> send_nwk_addr_request %hd", (FMT__H, param));

    req_param = ZB_BUF_GET_PARAM(param, zb_zdo_nwk_addr_req_param_t);

    req_param->dst_addr = 0x0000;
    ZB_IEEE_ADDR_COPY(req_param->ieee_addr, g_ieee_addr_ed1);
    req_param->request_type = ZB_ZDO_SINGLE_DEVICE_RESP;
    req_param-> start_index = 0;

    zb_zdo_nwk_addr_req(param, NULL);

    TRACE_MSG(TRACE_APS1, "<< send_nwk_addr_request %hd", (FMT__H, param));
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    switch (sig)
    {
    case ZB_ZDO_SIGNAL_DEFAULT_START:
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));
        if (status == 0)
        {
            ZB_SCHEDULE_CALLBACK(test_nwk_addr_req_for_zed1_delayed, 0);
        }
        break;

    case ZB_COMMON_SIGNAL_CAN_SLEEP:
        if (status == 0)
        {
#ifdef ZB_USE_SLEEP
            zb_sleep_now();
#endif /* ZB_USE_SLEEP */
        }
        break;

    default:
        TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}
