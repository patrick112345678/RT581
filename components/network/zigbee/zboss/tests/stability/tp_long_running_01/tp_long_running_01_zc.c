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
/* PURPOSE: ZC
*/

#define ZB_TEST_NAME TP_LONG_RUNNING_01_ZC

#define ZB_TRACE_FILE_ID 64915
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
#include "tp_long_running_01_zc.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(tp_long_running_01_zc_basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(tp_long_running_01_zc_identify_attr_list, &attr_identify_time);

/* On/Off cluster attributes data */
static zb_bool_t attr_on_off = (zb_bool_t)ZB_ZCL_ON_OFF_IS_ON;
static zb_bool_t global_scene_ctrl = ZB_ZCL_ON_OFF_GLOBAL_SCENE_CONTROL_DEFAULT_VALUE;
static zb_uint16_t on_time = ZB_ZCL_ON_OFF_ON_TIME_DEFAULT_VALUE;
static zb_uint16_t off_wait_time = ZB_ZCL_ON_OFF_OFF_WAIT_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST_EXT(tp_long_running_01_zc_on_off_attr_list,
                                      &attr_on_off,
                                      &global_scene_ctrl,
                                      &on_time,
                                      &off_wait_time);

/********************* Declare device **************************/
DECLARE_ZC_CLUSTER_LIST(tp_long_running_01_zc_device_clusters,
                        tp_long_running_01_zc_basic_attr_list,
                        tp_long_running_01_zc_identify_attr_list,
                        tp_long_running_01_zc_on_off_attr_list);

DECLARE_ZC_EP(tp_long_running_01_zc_device_ep,
              ZC_ENDPOINT,
              tp_long_running_01_zc_device_clusters);

DECLARE_ZC_CTX(tp_long_running_01_zc_device_ctx, tp_long_running_01_zc_device_ep);
/*************************************************************************/

/*******************Definitions for Test***************************/

static void test_start_fb_target(zb_uint8_t param);
static void test_send_active_ep_cb(zb_uint8_t param);
static void test_send_active_ep_req(zb_uint8_t param, zb_uint16_t device_addr);
static void test_send_active_ep_req_delayed(zb_uint8_t param);

static zb_uint8_t g_nwk_key[16] = TEST_DEFAULT_NWK_KEY;
static zb_ieee_addr_t g_ieee_addr_zc = IEEE_ADDR_ZC;

static zb_ieee_addr_t g_ieee_addr_zr = IEEE_ADDR_ZR;
static zb_uint16_t g_short_addr_zr = ZB_NWK_BROADCAST_ALL_DEVICES;

static zb_ieee_addr_t g_ieee_addr_zed = IEEE_ADDR_ZED;
static zb_uint16_t g_short_addr_zed = ZB_NWK_BROADCAST_ALL_DEVICES;

MAIN()
{
    ZB_SET_TRACE_MASK(TP_LONG_RUNNING_01_DEVICES_TRACE_MASK);
    ZB_SET_TRACE_LEVEL(4);
    ARGV_UNUSED;

    ZB_INIT("zdo_zc");

    zb_set_long_address(g_ieee_addr_zc);

    zb_set_network_coordinator_role((1l << TEST_CHANNEL));
    zb_secur_setup_nwk_key(g_nwk_key, 0);

    zb_set_nvram_erase_at_start(ZB_TRUE);


    ZB_AF_REGISTER_DEVICE_CTX(&tp_long_running_01_zc_device_ctx);

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


/***********************************Implementation**********************************/
ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_hdr_t *sg_p = NULL;
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

    TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

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
            ZB_SCHEDULE_ALARM(test_start_fb_target, 0, TP_LONG_RUNNING_01_ZC_FB_TARGET_START_DELAY);
        }

        break; /* ZB_BDB_SIGNAL_STEERING */

    case ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED:
        TRACE_MSG(TRACE_APP1, "signal: ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED, status %d", (FMT__D, status));
        break;

    case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
        TRACE_MSG(TRACE_APP1, "signal: ZB_ZDO_SIGNAL_DEVICE_ANNCE, status %d", (FMT__D, status));
        if (status == 0)
        {
            zb_zdo_signal_device_annce_params_t *params =
                ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_annce_params_t);

            TRACE_MSG(TRACE_APP1, "  ieee_addr  " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(params->ieee_addr)));
            TRACE_MSG(TRACE_APP1, "  nwk_addr 0x%hx, caps %hd", (FMT__H_H, params->device_short_addr, params->capability));

            if (ZB_IEEE_ADDR_CMP(params->ieee_addr, g_ieee_addr_zr))
            {
                g_short_addr_zr = params->device_short_addr;
            }
            else if (ZB_IEEE_ADDR_CMP(params->ieee_addr, g_ieee_addr_zed))
            {
                g_short_addr_zed = params->device_short_addr;

                ZB_SCHEDULE_APP_ALARM(test_send_active_ep_req_delayed, 0, TP_LONG_RUNNING_01_ZC_ACTIVE_EP_REQ_DELAY);
            }
        }
        break; /* ZB_ZDO_SIGNAL_DEVICE_ANNCE */

    default:
        TRACE_MSG(TRACE_APP1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}


static void test_start_fb_target(zb_uint8_t param)
{
    ZVUNUSED(param);

    TRACE_MSG(TRACE_APP1, ">> test_start_fb_target", (FMT__0));

    zb_bdb_finding_binding_target(ZC_ENDPOINT);

    TRACE_MSG(TRACE_APP1, "<< test_start_fb_target", (FMT__0));
}


static void test_send_active_ep_cb(zb_uint8_t param)
{
    zb_uint8_t *zdp_cmd = zb_buf_begin(param);
    zb_zdo_ep_resp_t *resp = (zb_zdo_ep_resp_t *)zdp_cmd;
    zb_uint8_t *ep_list = zdp_cmd + sizeof(zb_zdo_ep_resp_t);

    TRACE_MSG(TRACE_APP1, ">> test_send_active_ep_cb, param %hd, status %hd, addr 0x%x",
              (FMT__H_H_D, param, resp->status, resp->nwk_addr));

    if (resp->status == ZB_ZDP_STATUS_SUCCESS)
    {
        zb_uindex_t ep_index;

        TRACE_MSG(TRACE_APP1, "  test_send_active_ep_cb: ep count %hd, ep numbers:", (FMT__H, resp->ep_count));

        for (ep_index = 0; ep_index < resp->ep_count; ep_index++)
        {
            TRACE_MSG(TRACE_APS1, "  test_send_active_ep_cb: ep %hd", (FMT__H_H, *(ep_list + ep_index)));
        }
    }
    else
    {
        TRACE_MSG(TRACE_APP1, "  test_send_active_ep_cb: error", (FMT__0));
    }

#ifndef TP_LONG_RUNNING_01_ZC_ACTIVE_EP_INDEPENDENT
    ZB_SCHEDULE_APP_CALLBACK(test_send_active_ep_req_delayed, param);
#else
    zb_buf_free(param);
#endif /* TP_LONG_RUNNING_01_ZC_ACTIVE_EP_INDEPENDENT */

    TRACE_MSG(TRACE_APP1, "<< test_send_active_ep_cb", (FMT__0));
}


static void test_send_active_ep_req(zb_uint8_t param, zb_uint16_t device_addr)
{
    zb_zdo_active_ep_req_t *req;

    if (!param)
    {
        zb_buf_get_out_delayed_ext(test_send_active_ep_req, device_addr, 0);
        return;
    }

    TRACE_MSG(TRACE_APP1, ">> test_send_active_ep_req, param %hd, addr 0x%x", (FMT__H_D, param, device_addr));

    req = zb_buf_initial_alloc(param, sizeof(zb_zdo_active_ep_req_t));

    req->nwk_addr = device_addr;
    zb_zdo_active_ep_req(param, test_send_active_ep_cb);

    TRACE_MSG(TRACE_APP1, "<< test_send_active_ep_req", (FMT__0));
}


static void test_send_active_ep_req_delayed(zb_uint8_t param)
{
    zb_uint16_t device_addr = g_short_addr_zed;

    TRACE_MSG(TRACE_APP1, ">> test_send_active_ep_req, param %hd, addr 0x%x", (FMT__H_D, param, device_addr));

    if (param == ZB_BUF_INVALID)
    {
        zb_buf_get_out_delayed_ext(test_send_active_ep_req, device_addr, 0);
    }
    else
    {
        ZB_SCHEDULE_CALLBACK2(test_send_active_ep_req, param, device_addr);
    }

#ifdef TP_LONG_RUNNING_01_ZC_ACTIVE_EP_INDEPENDENT
    ZB_SCHEDULE_ALARM(test_send_active_ep_req_delayed, 0, TP_LONG_RUNNING_01_ZC_ACTIVE_EP_REQ_INTERVAL);
#endif /* TP_LONG_RUNNING_01_ZC_ACTIVE_EP_INDEPENDENT */

    TRACE_MSG(TRACE_APP1, "<< test_send_active_ep_req", (FMT__0));
}

/*! @} */
