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
/* PURPOSE: TH ZR
*/

#define ZB_TEST_NAME RTP_BDB_09_TH_ZR

#define ZB_TRACE_FILE_ID 40486
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
#include "device_th.h"

#include "device_dut.h"
#include "rtp_bdb_09_common.h"
#include "../common/zb_reg_test_globals.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error ZB_USE_NVRAM is not compiled!
#endif

#ifndef ZB_STACK_REGRESSION_TESTING_API
#error Define ZB_STACK_REGRESSION_TESTING_API
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_uint8_t g_nwk_key[16] = ZB_REG_TEST_DEFAULT_NWK_KEY;
static zb_ieee_addr_t g_ieee_addr_th_zr1 = IEEE_ADDR_TH_ZR1;

/******************* Endpoint 1 ************************/

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version_1  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source_1 = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(rtp_bdb_09_th_zr_basic_attr_list_1, &attr_zcl_version_1, &attr_power_source_1);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time_1 = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(rtp_bdb_09_th_zr_identify_attr_list_1, &attr_identify_time_1);

/********************* Declare device **************************/
DECLARE_TH_CLUSTER_LIST_1(rtp_bdb_09_th_zr_device_clusters_1,
                          rtp_bdb_09_th_zr_basic_attr_list_1,
                          rtp_bdb_09_th_zr_identify_attr_list_1);

DECLARE_TH_EP_1(rtp_bdb_09_th_zr_device_ep_1,
                THR_ENDPOINT_1,
                rtp_bdb_09_th_zr_device_clusters_1);

/******************* Endpoint 2 ************************/

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version_2  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source_2 = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(rtp_bdb_09_th_zr_basic_attr_list_2, &attr_zcl_version_2, &attr_power_source_2);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time_2 = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(rtp_bdb_09_th_zr_identify_attr_list_2, &attr_identify_time_2);

/********************* Declare device **************************/
DECLARE_TH_CLUSTER_LIST_2(rtp_bdb_09_th_zr_device_clusters_2,
                          rtp_bdb_09_th_zr_basic_attr_list_2,
                          rtp_bdb_09_th_zr_identify_attr_list_2);

DECLARE_TH_EP_2(rtp_bdb_09_th_zr_device_ep_2,
                THR_ENDPOINT_2,
                rtp_bdb_09_th_zr_device_clusters_2);

DECLARE_TH_CTX(rtp_bdb_09_th_zr1_device_ctx, rtp_bdb_09_th_zr_device_ep_1, rtp_bdb_09_th_zr_device_ep_2);

static zb_uint16_t g_dut_short_addr = 0x0000;
static zb_uint8_t g_start_idx = 0;

static void trigger_fb_targets(zb_uint8_t unused);
static void send_mgmt_bind_req_delayed(zb_uint8_t unused);
static void send_mgmt_bind_req(zb_uint8_t param);
static void mgmt_bind_resp_cb(zb_uint8_t param);

/************************Main*************************************/
MAIN()
{
    ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
    ZB_SET_TRACE_LEVEL(4);
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_th_zr");


    zb_set_long_address(g_ieee_addr_th_zr1);

    zb_reg_test_set_common_channel_settings();
    zb_set_network_router_role((1l << TEST_CHANNEL));
    zb_set_nvram_erase_at_start(ZB_TRUE);


    zb_secur_setup_nwk_key(g_nwk_key, 0);

    ZB_AF_REGISTER_DEVICE_CTX(&rtp_bdb_09_th_zr1_device_ctx);

    ZB_REGRESSION_TESTS_API().bdb_allow_multiple_fb_targets = ZB_TRUE;

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
        TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
        if (status == 0)
        {
            test_step_register(trigger_fb_targets, 0, RTP_BDB_09_STEP_1_TIME_ZR);
            test_step_register(send_mgmt_bind_req_delayed, 0, RTP_BDB_09_STEP_2_TIME_ZR);

            test_control_start(TEST_MODE, RTP_BDB_09_STEP_1_DELAY_ZR);
        }
        break; /* ZB_BDB_SIGNAL_STEERING */

    default:
        TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}

static void trigger_fb_targets(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    zb_bdb_finding_binding_target(THR_ENDPOINT_1);
    zb_bdb_finding_binding_target(THR_ENDPOINT_2);
}


static void send_mgmt_bind_req_delayed(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    zb_buf_get_out_delayed(send_mgmt_bind_req);
}

static void send_mgmt_bind_req(zb_uint8_t param)
{
    zb_zdo_mgmt_bind_param_t *req_params;

    TRACE_MSG(TRACE_APP1, ">>send_mgmt_bind_req: param = %i", (FMT__D, param));

    req_params = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_bind_param_t);
    req_params->start_index = g_start_idx;
    req_params->dst_addr = g_dut_short_addr;
    zb_zdo_mgmt_bind_req(param, mgmt_bind_resp_cb);

    TRACE_MSG(TRACE_APP1, "<<send_mgmt_bind_req", (FMT__0));
}

static void mgmt_bind_resp_cb(zb_uint8_t param)
{
    zb_zdo_mgmt_bind_resp_t *resp;
    zb_callback_t call_cb = 0;

    TRACE_MSG(TRACE_APP1, ">>mgmt_bind_resp_cb: param = %i", (FMT__D, param));

    resp = (zb_zdo_mgmt_bind_resp_t *) zb_buf_begin(param);

    TRACE_MSG(TRACE_APP1, "mgmt_bind_resp_cb: status = %i", (FMT__D, resp->status));

    if (resp->status == ZB_ZDP_STATUS_SUCCESS)
    {
        zb_uint8_t nbrs = resp->binding_table_list_count;

        g_start_idx += nbrs;
        if (g_start_idx < resp->binding_table_entries)
        {
            TRACE_MSG(TRACE_APP1, "mgmt_bind_resp_cb: retrieved = %d, total = %d",
                      (FMT__D_D, nbrs, resp->binding_table_entries));
            call_cb = send_mgmt_bind_req;
        }
        else
        {
            TRACE_MSG(TRACE_APP1, "mgmt_bind_resp_cb: retrieved all entries - %d",
                      (FMT__D, resp->binding_table_entries));
        }
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "mgmt_bind_resp_cb: TEST_FAILED", (FMT__0));
    }

    if (call_cb)
    {
        zb_buf_reuse(param);
        ZB_SCHEDULE_CALLBACK(call_cb, param);
    }
    else
    {
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_APP1, "<<mgmt_bind_resp_cb", (FMT__0));
}

/*! @} */
