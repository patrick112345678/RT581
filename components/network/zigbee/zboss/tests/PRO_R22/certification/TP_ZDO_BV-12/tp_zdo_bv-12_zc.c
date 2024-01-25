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
/* PURPOSE: 11.12 TP/ZDO/BV-12 ZC-ZDO-Transmit
Bind/Unbind_req. Coordinator side
*/

#define ZB_TEST_NAME TP_ZDO_BV_12_ZC
#define ZB_TRACE_FILE_ID 40806

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "tp_zdo_bv-12_common.h"
#include "../common/zb_cert_test_globals.h"


static const zb_ieee_addr_t g_ieee_addr_c = IEEE_ADDR_C;
static zb_ieee_addr_t g_ieee_addr_ed1 = IEEE_ADDR_ED1;
static zb_ieee_addr_t g_ieee_addr_ed2 = IEEE_ADDR_ED2;

static void bind_req_cb(zb_uint8_t param);
static void unbind_req(zb_uint8_t param);
static void unbind_req_cb(zb_uint8_t param);
static void bind_req_common(zb_uint8_t unused);

MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("zdo_zc");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zc_role();
    zb_set_long_address(g_ieee_addr_c);
    zb_set_pan_id(0x1aaa);

    /* zb_cert_test_set_security_level(0); */

    zb_set_max_children(2);
    zb_bdb_set_legacy_device_support(ZB_TRUE);
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

    switch (sig)
    {
    case ZB_ZDO_SIGNAL_DEFAULT_START:
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));
        if (status == 0)
        {
#ifdef TEST_ENABLED
            /* Send bind request ZED1 */
            test_step_register(bind_req_common, 0, TP_ZDO_BV_12_STEP_1_TIME_ZC);
            /* Send unbind request ZED1 */
            test_step_register(unbind_req, 0, TP_ZDO_BV_12_STEP_4_TIME_ZC);

            test_control_start(TEST_MODE, TP_ZDO_BV_12_STEP_1_DELAY_ZC);
#endif
        }
        break;

    default:
        TRACE_MSG(TRACE_APP1, "Unknown signal, status %d", (FMT__D, status));
    }

    zb_buf_free(param);
}

static void unbind_req_cb(zb_uint8_t param)
{
    zb_zdo_bind_resp_t *bind_resp = (zb_zdo_bind_resp_t *)zb_buf_begin(param);

    TRACE_MSG(TRACE_ZDO1, "unbind_req_cb resp status %hd", (FMT__H, bind_resp->status));
    if (bind_resp->status != ZB_ZDP_STATUS_SUCCESS)
    {
        TRACE_MSG(TRACE_ZDO1, "unbind_req_cb: error - status failed [0x%x]", (FMT__H, bind_resp->status));
        ZB_EXIT(1);
    }
    zb_buf_free(param);
}


static void unbind_req(zb_uint8_t param)
{
    zb_bufid_t buf = zb_buf_get_out();
    zb_zdo_bind_req_param_t *bind_param;
    ZVUNUSED(param);

    TRACE_MSG(TRACE_ZDO1, "unbind_req", (FMT__0));
    if (!buf)
    {
        TRACE_MSG(TRACE_ZDO1, "unbind_req: error - unable to get data buffer", (FMT__0));
        ZB_EXIT(1);
    }

    zb_buf_initial_alloc(buf, 0);
    bind_param = ZB_BUF_GET_PARAM(buf, zb_zdo_bind_req_param_t);
    ZB_MEMCPY(bind_param->src_address, g_ieee_addr_ed1, sizeof(zb_ieee_addr_t));
    bind_param->src_endp = TEST_ED1_EP;
    bind_param->cluster_id = TP_BUFFER_TEST_REQUEST_CLID;
    bind_param->dst_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
    ZB_MEMCPY(bind_param->dst_address.addr_long, g_ieee_addr_ed2, sizeof(zb_ieee_addr_t));
    bind_param->dst_endp = TEST_ED2_EP;
    bind_param->req_dst_addr = zb_address_short_by_ieee(g_ieee_addr_ed1);
    TRACE_MSG(TRACE_ZDO1, "dst addr %d", (FMT__D, bind_param->req_dst_addr));

    zb_zdo_unbind_req(buf, unbind_req_cb);
}


static void bind_req_cb(zb_uint8_t param)
{
    zb_zdo_bind_resp_t *bind_resp = (zb_zdo_bind_resp_t *)zb_buf_begin(param);

    TRACE_MSG(TRACE_ZDO1, "bind_req_cb resp status %hd", (FMT__H, bind_resp->status));
    if (bind_resp->status != ZB_ZDP_STATUS_SUCCESS)
    {
        TRACE_MSG(TRACE_ZDO1, "bind_req_cb: error - status failed [0x%x]", (FMT__H, bind_resp->status));
        ZB_EXIT(1);
    }
    zb_buf_free(param);
}


static void bind1_req(zb_uint8_t param)
{
    zb_bufid_t buf = zb_buf_get_out();
    zb_zdo_bind_req_param_t *bind_param;
    ZVUNUSED(param);

    TRACE_MSG(TRACE_ZDO1, ">>bind1_req", (FMT__0));

    if (!buf)
    {
        TRACE_MSG(TRACE_ERROR, "bind_req: error - unbale to get data buffer", (FMT__0));
        ZB_EXIT(1);
    }

    bind_param = ZB_BUF_GET_PARAM(buf, zb_zdo_bind_req_param_t);
    ZB_MEMCPY(bind_param->src_address, g_ieee_addr_ed1, sizeof(zb_ieee_addr_t));
    bind_param->src_endp = TEST_ED1_EP;
    bind_param->cluster_id = TP_BUFFER_TEST_REQUEST_CLID;
    bind_param->dst_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
    ZB_MEMCPY(bind_param->dst_address.addr_long, g_ieee_addr_ed2, sizeof(zb_ieee_addr_t));
    bind_param->dst_endp = TEST_ED2_EP;
    bind_param->req_dst_addr = zb_address_short_by_ieee(g_ieee_addr_ed1);
    TRACE_MSG(TRACE_ZDO1, "dst addr %d", (FMT__D, bind_param->req_dst_addr));

    zb_zdo_bind_req(buf, bind_req_cb);
    TRACE_MSG(TRACE_ZDO1, "<<bind1_req", (FMT__0));
}


static void bind2_req(zb_uint8_t param)
{
    zb_bufid_t buf = zb_buf_get_out();
    zb_zdo_bind_req_param_t *bind_param;
    ZVUNUSED(param);

    TRACE_MSG(TRACE_ZDO1, ">>bind2_req", (FMT__0));

    if (!buf)
    {
        TRACE_MSG(TRACE_ERROR, "bind_req: error - unbale to get data buffer", (FMT__0));
        ZB_EXIT(1);
    }

    bind_param = ZB_BUF_GET_PARAM(buf, zb_zdo_bind_req_param_t);
    ZB_MEMCPY(bind_param->src_address, g_ieee_addr_ed2, sizeof(zb_ieee_addr_t));
    bind_param->src_endp = TEST_ED2_EP;
    bind_param->cluster_id = TP_BUFFER_TEST_REQUEST_CLID;
    bind_param->dst_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
    ZB_MEMCPY(bind_param->dst_address.addr_long, g_ieee_addr_ed1, sizeof(zb_ieee_addr_t));
    bind_param->dst_endp = TEST_ED1_EP;
    bind_param->req_dst_addr = zb_address_short_by_ieee(g_ieee_addr_ed2);
    TRACE_MSG(TRACE_ZDO1, "dst addr %d", (FMT__D, bind_param->req_dst_addr));

    zb_zdo_bind_req(buf, bind_req_cb);
    TRACE_MSG(TRACE_ZDO1, "<<bind2_req", (FMT__0));
}

static void bind_req_common(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    ZB_SCHEDULE_CALLBACK(bind1_req, 0);
    ZB_SCHEDULE_ALARM(bind2_req, 0, 2 * ZB_TIME_ONE_SECOND);
}
