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
/* PURPOSE: 11.12 TP/ZDO/BV-12 ZC-ZDO-Transmit Bind/Unbind_req. End
device 1
*/

#define ZB_TEST_NAME TP_ZDO_BV_12_ZED1
#define ZB_TRACE_FILE_ID 40807

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "tp_zdo_bv-12_common.h"
#include "../common/zb_cert_test_globals.h"


static const zb_ieee_addr_t g_ieee_addr_ed1 = IEEE_ADDR_ED1;
static zb_ieee_addr_t g_ieee_addr_ed2 = IEEE_ADDR_ED2;

static void test_nwk_addr_req_for_zed2_delayed(zb_uint8_t unused);
static void test_nwk_addr_req_for_zed2(zb_uint8_t param);
static void test_nwk_addr_req_for_zed2_cb(zb_uint8_t param);
static void test_send_test_request_delayed(zb_uint8_t unused);
static void test_send_test_request(zb_uint8_t param);
static void test_send_test_request_cb(zb_uint8_t param);

static zb_uint8_t g_buffer_test_resp_cnt = 0;
static zb_af_simple_desc_1_1_t g_test_simple_desc;

MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("zdo_zed1");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    zb_set_long_address(g_ieee_addr_ed1);

    /* zb_cert_test_set_security_level(0); */

    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zed_role();
    zb_set_rx_on_when_idle(ZB_FALSE);

    zb_set_simple_descriptor((zb_af_simple_desc_1_1_t *)&g_test_simple_desc,
                             TEST_ED1_EP /* endpoint */,        0x0103 /* app_profile_id */,
                             0x0 /* app_device_id */,           0x0   /* app_device_version*/,
                             0x1 /* app_input_cluster_count */, 0x1 /* app_output_cluster_count */);
    zb_set_input_cluster_id((zb_af_simple_desc_1_1_t *)&g_test_simple_desc, 0,  0x0054);
    zb_set_output_cluster_id((zb_af_simple_desc_1_1_t *)&g_test_simple_desc, 0,  0x001c);
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
#ifdef TEST_ENABLED
            /* Send data to ZC after bind request from coordinator */
            test_step_register(test_send_test_request_delayed, 0, TP_ZDO_BV_12_STEP_2_TIME_ZED1);
            /* Send data to ZC after unbind request from coordinator */
            test_step_register(test_send_test_request_delayed, 0, TP_ZDO_BV_12_STEP_5_TIME_ZED1);

            test_control_start(TEST_MODE, TP_ZDO_BV_12_STEP_2_DELAY_ZED1);
#endif
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

static void test_nwk_addr_req_for_zed2_delayed(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    if (zb_buf_get_out_delayed(test_nwk_addr_req_for_zed2) != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "test_nwk_addr_req_for_zed2_delayed: buffer allocation error", (FMT__0));
    }
}

static void test_nwk_addr_req_for_zed2(zb_uint8_t param)
{
    zb_zdo_nwk_addr_req_param_t *req_param;

    TRACE_MSG(TRACE_APS1, ">> send_nwk_addr_request %hd", (FMT__H, param));

    req_param = ZB_BUF_GET_PARAM(param, zb_zdo_nwk_addr_req_param_t);

    req_param->dst_addr = 0x0000;
    ZB_IEEE_ADDR_COPY(req_param->ieee_addr, g_ieee_addr_ed2);
    req_param->request_type = ZB_ZDO_SINGLE_DEVICE_RESP;
    req_param->start_index = 0;

    zb_zdo_nwk_addr_req(param, test_nwk_addr_req_for_zed2_cb);

    TRACE_MSG(TRACE_APS1, "<< send_nwk_addr_request", (FMT__0));
}

static void test_nwk_addr_req_for_zed2_cb(zb_uint8_t param)
{
    zb_zdo_nwk_addr_resp_head_t *resp;
    zb_ieee_addr_t ieee_addr;
    zb_uint16_t nwk_addr;
    zb_address_ieee_ref_t addr_ref;

    resp = (zb_zdo_nwk_addr_resp_head_t *)zb_buf_begin(param);

    TRACE_MSG(TRACE_APS1, ">> test_nwk_addr_req_for_zed2_cb, resp status %hd, nwk addr %d", (FMT__H_D, resp->status, resp->nwk_addr));

    if (resp->status == ZB_ZDP_STATUS_SUCCESS)
    {
        ZB_LETOH64(ieee_addr, resp->ieee_addr);
        ZB_LETOH16(&nwk_addr, &resp->nwk_addr);
        zb_address_update(ieee_addr, nwk_addr, ZB_TRUE, &addr_ref);
    }

    zb_buf_free(param);

    ZB_SCHEDULE_CALLBACK(test_send_test_request_delayed, 0);

    TRACE_MSG(TRACE_APS1, "<< test_nwk_addr_req_for_zed2_cb", (FMT__0));
}

static void test_send_test_request_delayed(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    TRACE_MSG(TRACE_APP1, ">> test_send_test_request_delayed", (FMT__0));

    if (zb_address_short_by_ieee(g_ieee_addr_ed2) == ZB_UNKNOWN_SHORT_ADDR)
    {
        ZB_SCHEDULE_CALLBACK(test_nwk_addr_req_for_zed2_delayed, 0);
    }
    else
    {
        if (zb_buf_get_out_delayed(test_send_test_request) != RET_OK)
        {
            TRACE_MSG(TRACE_ERROR, "test_send_test_request_delayed: buffer allocation error", (FMT__0));
        }
    }

    TRACE_MSG(TRACE_APP1, "<< test_send_test_request_delayed", (FMT__0));
}

static void test_send_test_request(zb_uint8_t param)
{
    zb_buffer_test_req_param_t *req_param;

    TRACE_MSG(TRACE_APS1, ">> test_send_test_request %hd", (FMT__H, param));

    req_param = ZB_BUF_GET_PARAM(param, zb_buffer_test_req_param_t);
    BUFFER_TEST_REQ_SET_DEFAULT(req_param);

    req_param->len = TEST_BUFFER_LEN;
    req_param->src_ep = TEST_ED1_EP;
    req_param->addr_mode = ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT;
    req_param->dst_addr = zb_address_short_by_ieee(g_ieee_addr_ed2);

    zb_tp_buffer_test_request(param, test_send_test_request_cb);

    TRACE_MSG(TRACE_APS1, "<< test_send_test_request", (FMT__0));
}

static void test_send_test_request_cb(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APS1, ">> test_send_test_request_cb %hd", (FMT__H, param));

    g_buffer_test_resp_cnt++;
    if (param == ZB_TP_BUFFER_TEST_OK)
    {
        TRACE_MSG(TRACE_APS1, "test_send_test_request_cb: status Ok", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_APS1, "test_send_test_request_cb: status Failed", (FMT__0));
        if (g_buffer_test_resp_cnt == 2)
        {
            TRACE_MSG(TRACE_APS1, "test_send_test_request_cb: status Failed - TEST OK (DATA after unbind request)", (FMT__0));
        }
    }

    TRACE_MSG(TRACE_APS1, "<< test_send_test_request_cb", (FMT__0));
}
