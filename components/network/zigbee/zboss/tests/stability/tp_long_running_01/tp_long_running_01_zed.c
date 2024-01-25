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
/* PURPOSE: DUT ZED
*/

#define ZB_TEST_NAME TP_LONG_RUNNING_01_DUT_ZED

#define ZB_TRACE_FILE_ID 40287
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

#include "tp_long_running_01_common.h"
#include "tp_long_running_01_zed.h"

#ifndef ZB_ED_ROLE
#error ED role is not compiled!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(tp_long_running_01_zed_basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(tp_long_running_01_zed_identify_attr_list, &attr_identify_time);

/********************* Declare device **************************/
DECLARE_ZED_CLUSTER_LIST(tp_long_running_01_zed_device_clusters,
                         tp_long_running_01_zed_basic_attr_list,
                         tp_long_running_01_zed_identify_attr_list);

DECLARE_ZED_EP(tp_long_running_01_zed_device_ep,
               ZED_ENDPOINT,
               tp_long_running_01_zed_device_clusters);

DECLARE_ZED_CTX(tp_long_running_01_zed_device_ctx, tp_long_running_01_zed_device_ep);
/*************************************************************************/

/*******************Definitions for Test***************************/

static zb_ieee_addr_t g_ieee_addr_zed = IEEE_ADDR_ZED;

static zb_ieee_addr_t g_ieee_addr_zc = IEEE_ADDR_ZC;
static zb_uint16_t g_short_addr_zcl_client = ZB_NWK_BROADCAST_ALL_DEVICES;

static zb_ieee_addr_t g_ieee_addr_zr = IEEE_ADDR_ZR;
static zb_uint16_t g_short_addr_zdo_client = ZB_NWK_BROADCAST_ALL_DEVICES;


static zb_bool_t test_finding_binding_cb(zb_int16_t status, zb_ieee_addr_t addr,
        zb_uint8_t ep, zb_uint16_t cluster);
static void test_start_fb_initiator(zb_uint8_t param);

static void test_send_zcl_req_cb(zb_uint8_t param);
static void test_send_zcl_req(zb_uint8_t param);
static void test_send_zcl_req_delayed(zb_uint8_t param);

static void test_send_zdo_client_short_addr_req_cb(zb_uint8_t param);
static void test_send_zdo_client_short_addr_req(zb_uint8_t param);
static void test_send_zdo_req_cb(zb_uint8_t param);
static void test_send_zdo_req(zb_uint8_t param, zb_uint16_t short_addr);
static void test_send_zdo_req_delayed(zb_uint8_t param);

/************************Main*************************************/
MAIN()
{
    ZB_SET_TRACE_MASK(TP_LONG_RUNNING_01_DEVICES_TRACE_MASK);
    ZB_SET_TRACE_LEVEL(4);
    ARGV_UNUSED;

    ZB_INIT("zdo_zed");

    zb_set_long_address(g_ieee_addr_zed);
    MAC_ADD_INVISIBLE_SHORT(0x0000);

    zb_set_network_ed_role((1l << TEST_CHANNEL));
    zb_set_nvram_erase_at_start(ZB_TRUE);

    zb_set_rx_on_when_idle(ZB_FALSE);

    ZB_AF_REGISTER_DEVICE_CTX(&tp_long_running_01_zed_device_ctx);

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
            ZB_SCHEDULE_ALARM(test_start_fb_initiator, 0, TP_LONG_RUNNING_01_ZED_FB_INITIATOR_START_DELAY);
            ZB_SCHEDULE_ALARM(test_send_zdo_client_short_addr_req, 0, TP_LONG_RUNNING_01_ZED_ZDO_CLIENT_FINDING_DELAY);
        }

        break; /* ZB_BDB_SIGNAL_STEERING */

    case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:
        TRACE_MSG(TRACE_APP1, "signal: ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED, status %d", (FMT__D, status));
        break;

    case ZB_COMMON_SIGNAL_CAN_SLEEP:
        if (status == 0)
        {
            zb_sleep_now();
        }
        break; /* ZB_COMMON_SIGNAL_CAN_SLEEP */

    default:
        TRACE_MSG(TRACE_APP1, "Unknown signal, signal %hd, status %hd", (FMT__D_H, sig, status));
        break;
    }

    zb_buf_free(param);
}


static zb_bool_t test_finding_binding_cb(zb_int16_t status, zb_ieee_addr_t addr,
        zb_uint8_t ep, zb_uint16_t cluster)
{
    TRACE_MSG(TRACE_APP1, ">> test_finding_binding_cb, status %d, addr " TRACE_FORMAT_64 ", ep %hd, cluster %d",
              (FMT__D_A_H_D, status, TRACE_ARG_64(addr), ep, cluster));

    if (status == ZB_BDB_COMM_BIND_SUCCESS && cluster == ZB_ZCL_CLUSTER_ID_ON_OFF)
    {
        ZB_ASSERT(ZB_IEEE_ADDR_CMP(addr, g_ieee_addr_zc));
        g_short_addr_zcl_client = 0x0000;

        ZB_SCHEDULE_APP_ALARM(test_send_zcl_req_delayed, 0, TP_LONG_RUNNING_01_ZED_ZCL_REQ_DELAY);
    }

    return ZB_TRUE;
}


static void test_start_fb_initiator(zb_uint8_t param)
{
    ZVUNUSED(param);

    TRACE_MSG(TRACE_APP1, ">> test_start_fb_initiator", (FMT__0));

    zb_bdb_finding_binding_initiator(ZED_ENDPOINT, test_finding_binding_cb);

    TRACE_MSG(TRACE_APP1, "<< test_start_fb_initiator", (FMT__0));
}


static void test_send_zcl_req_cb(zb_uint8_t param)
{
    zb_zcl_command_send_status_t *cmd_send_status = ZB_BUF_GET_PARAM(param, zb_zcl_command_send_status_t);
    TRACE_MSG(TRACE_APP1, ">> test_send_zcl_req_cb, param %hd, status %hd", (FMT__H_H, param, cmd_send_status->status));

#ifndef TP_LONG_RUNNING_01_ZED_ZCL_REQ_INDEPENDENT
    ZB_SCHEDULE_APP_CALLBACK(test_send_zcl_req_delayed, param);
#else
    zb_buf_free(param);
#endif /* TP_LONG_RUNNING_01_ZED_ZCL_REQ_INDEPENDENT */

    TRACE_MSG(TRACE_APP1, "<< test_send_zcl_req_cb", (FMT__0));
}


static void test_send_zcl_req(zb_uint8_t param)
{
    zb_uint16_t addr = 0;

    if (param == ZB_BUF_INVALID)
    {
        zb_buf_get_out_delayed(test_send_zcl_req);
        return;
    }

    TRACE_MSG(TRACE_APP1, ">> test_send_zcl_req", (FMT__0));

    ZB_ZCL_ON_OFF_SEND_REQ(
        param,
        addr,
        ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT,
        0,
        ZED_ENDPOINT,
        ZB_AF_HA_PROFILE_ID,
        ZB_FALSE,
        ZB_ZCL_CMD_ON_OFF_ON_ID,
        test_send_zcl_req_cb
    );

    TRACE_MSG(TRACE_APP1, "<< test_send_zcl_req", (FMT__0));
}


static void test_send_zcl_req_delayed(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> test_send_zcl_req_delayed, param %hd, addr 0x%x", (FMT__H_D, param, g_short_addr_zcl_client));

    if (param == ZB_BUF_INVALID)
    {
        zb_buf_get_out_delayed(test_send_zcl_req);
    }
    else
    {
        ZB_SCHEDULE_APP_CALLBACK(test_send_zcl_req, param);
    }

#ifdef TP_LONG_RUNNING_01_ZED_ZCL_REQ_INDEPENDENT
    ZB_SCHEDULE_ALARM(test_send_zcl_req_delayed, 0, TP_LONG_RUNNING_01_ZED_ZCL_REQ_INTERVAL);
#endif /* TP_LONG_RUNNING_01_ZED_ZCL_REQ_INDEPENDENT */

    TRACE_MSG(TRACE_APP1, "<< test_send_zcl_req_delayed", (FMT__0));
}


static void test_send_zdo_client_short_addr_req_cb(zb_uint8_t param)
{
    zb_zdo_nwk_addr_resp_head_t *resp;
    zb_uint16_t nwk_addr;

    TRACE_MSG(TRACE_APP1, ">> test_send_zdo_client_short_addr_req_cb, param %hd", (FMT__H, param));

    resp = (zb_zdo_nwk_addr_resp_head_t *)zb_buf_begin(param);
    TRACE_MSG(TRACE_APP1, "  test_send_zdo_client_short_addr_req_cb: status %hd, nwk_addr %d",
              (FMT__H_D, resp->status, resp->nwk_addr));

    ZB_DUMP_IEEE_ADDR(resp->ieee_addr);

    if (resp->status == ZB_ZDP_STATUS_SUCCESS)
    {
        ZB_LETOH16(&nwk_addr, &resp->nwk_addr);

        g_short_addr_zdo_client = nwk_addr;

        ZB_SCHEDULE_APP_ALARM(test_send_zdo_req_delayed, param, TP_LONG_RUNNING_01_ZED_ZDO_REQ_DELAY);
    }
    else
    {
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_APP1, "<< test_send_zdo_client_short_addr_req_cb", (FMT__0));
}


static void test_send_zdo_client_short_addr_req(zb_uint8_t param)
{
    zb_zdo_nwk_addr_req_param_t *req_param;

    if (param == ZB_BUF_INVALID)
    {
        zb_buf_get_out_delayed(test_send_zdo_client_short_addr_req);
        return;
    }

    TRACE_MSG(TRACE_APP2, ">> test_send_zdo_client_short_addr_req, param %hd", (FMT__H, param));

    req_param = ZB_BUF_GET_PARAM(param, zb_zdo_nwk_addr_req_param_t);

    ZB_IEEE_ADDR_COPY(req_param->ieee_addr, g_ieee_addr_zr);

    req_param->dst_addr = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
    req_param->start_index = 0;
    req_param->request_type = 0x00;

    zb_zdo_nwk_addr_req(param, test_send_zdo_client_short_addr_req_cb);

    TRACE_MSG(TRACE_APP2, "<< test_send_zdo_client_short_addr_req", (FMT__0));
}


static void test_send_zdo_req_cb(zb_uint8_t param)
{
    zb_zdo_match_desc_resp_t *resp = (zb_zdo_match_desc_resp_t *)zb_buf_begin(param);

    TRACE_MSG(TRACE_APP1, ">> test_send_zdo_req_cb, param %hd, match_len %hd", (FMT__H_H, param, resp->match_len));

#ifndef TP_LONG_RUNNING_01_ZED_ZDO_REQ_INDEPENDENT
    ZB_SCHEDULE_APP_CALLBACK(test_send_zdo_req_delayed, param);
#else
    zb_buf_free(param);
#endif /* TP_LONG_RUNNING_01_ZED_ZDO_REQ_INDEPENDENT */

    TRACE_MSG(TRACE_APP1, "<< test_send_zdo_req_cb", (FMT__0));
}


static void test_send_zdo_req(zb_uint8_t param, zb_uint16_t short_addr)
{
    zb_bufid_t buf = param;
    zb_zdo_match_desc_param_t *req;

    TRACE_MSG(TRACE_APP1, ">> test_send_zdo_req, param %hd, dst_addr 0x%x", (FMT__H_D, param, short_addr));

    req = zb_buf_initial_alloc(buf, sizeof(zb_zdo_match_desc_param_t));

    req->nwk_addr = g_short_addr_zdo_client;
    req->addr_of_interest = g_short_addr_zdo_client;
    req->profile_id = ZB_AF_HA_PROFILE_ID;
    req->num_in_clusters = 1;
    req->num_out_clusters = 0;
    req->cluster_list[0] = ZB_ZCL_CLUSTER_ID_BASIC;

    zb_zdo_match_desc_req(param, test_send_zdo_req_cb);

    TRACE_MSG(TRACE_APP1, "<< test_send_zdo_req", (FMT__0));
}


static void test_send_zdo_req_delayed(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> test_send_zdo_req_delayed, param %hd, addr 0x%x", (FMT__H_D, param, g_short_addr_zdo_client));

    if (param == ZB_BUF_INVALID)
    {
        zb_buf_get_out_delayed_ext(test_send_zdo_req, g_short_addr_zdo_client, 0);
    }
    else
    {
        ZB_SCHEDULE_APP_CALLBACK2(test_send_zdo_req, param, g_short_addr_zdo_client);
    }

#ifdef TP_LONG_RUNNING_01_ZED_ZDO_REQ_INDEPENDENT
    ZB_SCHEDULE_ALARM(test_send_zdo_req_delayed, 0, TP_LONG_RUNNING_01_ZED_ZDO_REQ_INTERVAL);
#endif /* TP_LONG_RUNNING_01_ZED_ZDO_REQ_INDEPENDENT */

    TRACE_MSG(TRACE_APP1, "<< test_send_zdo_req_delayed", (FMT__0));
}

/*! @} */
