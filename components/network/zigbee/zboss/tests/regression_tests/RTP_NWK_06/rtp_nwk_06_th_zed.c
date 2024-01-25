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
/* PURPOSE: TH ZED
*/

#define ZB_TEST_NAME RTP_NWK_06_TH_ZED
#define ZB_TRACE_FILE_ID 64909

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_zcl.h"
#include "zb_bdb_internal.h"
#include "zb_zcl.h"

#include "rtp_nwk_06_common.h"
#include "../common/zb_reg_test_globals.h"

#ifndef ZB_ED_ROLE
#error ED role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error ZB_USE_NVRAM is not compiled!
#endif


/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_th_zed = IEEE_ADDR_TH_ZED;
static zb_ieee_addr_t g_ieee_addr_dut_zr1 = IEEE_ADDR_DUT_ZR1;
static zb_uint16_t g_short_addr_dut_zr1 = ZB_NWK_BROADCAST_ALL_DEVICES;

static void send_link_quality_request(zb_uint8_t param);
static void get_concentrator_addr_req_cb(zb_uint8_t param);
static void get_concentrator_addr_req(zb_uint8_t param);

/************************Main*************************************/
MAIN()
{
    ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
    ZB_SET_TRACE_LEVEL(4);
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("zdo_th_zed");

    zb_set_long_address(g_ieee_addr_th_zed);

    zb_reg_test_set_common_channel_settings();
    zb_set_network_ed_role((1l << TEST_CHANNEL));
    zb_set_nvram_erase_at_start(ZB_TRUE);

    zb_set_rx_on_when_idle(ZB_FALSE);

    if (zboss_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
    }
    else
    {
        zdo_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}


/********************ZDO Startup*****************************/
ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    TRACE_MSG(TRACE_APS1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    switch (sig)
    {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));
        if (status == 0)
        {
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        }
        break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APP1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
        if (status == 0)
        {
            test_step_register(get_concentrator_addr_req, 0, RTP_NWK_06_STEP_1_TIME_ZED);
            test_control_start(TEST_MODE, RTP_NWK_06_STEP_1_DELAY_ZED);
        }
        break; /* ZB_BDB_SIGNAL_STEERING */


    case ZB_COMMON_SIGNAL_CAN_SLEEP:
        TRACE_MSG(TRACE_APS1, "signal: ZB_COMMON_SIGNAL_CAN_SLEEP, status %d", (FMT__D, status));
        if (status == 0)
        {
            zb_sleep_now();
        }
        break; /* ZB_COMMON_SIGNAL_CAN_SLEEP */

    default:
        TRACE_MSG(TRACE_APP1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}


static void send_link_quality_request(zb_uint8_t param)
{
    zb_zdo_mgmt_lqi_param_t *req_param;

    if (param == ZB_BUF_INVALID)
    {
        zb_buf_get_out_delayed(send_link_quality_request);
        return;
    }

    TRACE_MSG(TRACE_APP1, ">> send_link_quality_request, param %hd", (FMT__H, param));

    req_param = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_lqi_param_t);

    req_param->dst_addr = g_short_addr_dut_zr1;
    req_param->start_index = 0;

    TRACE_MSG(TRACE_APP1, "send to 0x%x", (FMT__H, req_param->dst_addr));

    zb_zdo_mgmt_lqi_req(param, NULL);

    TRACE_MSG(TRACE_APP1, "<< send_link_quality_request", (FMT__0));
}


static void get_concentrator_addr_req_cb(zb_uint8_t param)
{
    zb_zdo_nwk_addr_resp_head_t *resp;
    zb_uint16_t nwk_addr;

    TRACE_MSG(TRACE_APP1, ">> get_concentrator_addr_req_cb, param %hd", (FMT__H, param));

    resp = (zb_zdo_nwk_addr_resp_head_t *)zb_buf_begin(param);
    TRACE_MSG(TRACE_APP1, "resp status %hd, nwk addr %d", (FMT__H_D, resp->status, resp->nwk_addr));

    ZB_DUMP_IEEE_ADDR(resp->ieee_addr);

    if (resp->status == ZB_ZDP_STATUS_SUCCESS)
    {
        ZB_LETOH16(&nwk_addr, &resp->nwk_addr);

        g_short_addr_dut_zr1 = nwk_addr;

        ZB_SCHEDULE_CALLBACK(send_link_quality_request, param);
    }
    else
    {
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_APP1, "<< get_concentrator_addr_req_cb", (FMT__0));
}


static void get_concentrator_addr_req(zb_uint8_t param)
{
    zb_zdo_nwk_addr_req_param_t *req_param;

    TRACE_MSG(TRACE_APP1, ">> get_concentrator_addr_req, param %hd", (FMT__H, param));

    if (param == 0)
    {
        zb_buf_get_out_delayed(get_concentrator_addr_req);
        return;
    }

    req_param = ZB_BUF_GET_PARAM(param, zb_zdo_nwk_addr_req_param_t);

    ZB_IEEE_ADDR_COPY(req_param->ieee_addr, g_ieee_addr_dut_zr1);

    req_param->dst_addr = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
    req_param->start_index = 0;
    req_param->request_type = 0x00;

    zb_zdo_nwk_addr_req(param, get_concentrator_addr_req_cb);

    TRACE_MSG(TRACE_APP2, "<< get_concentrator_addr_req", (FMT__0));
}

/*! @} */
