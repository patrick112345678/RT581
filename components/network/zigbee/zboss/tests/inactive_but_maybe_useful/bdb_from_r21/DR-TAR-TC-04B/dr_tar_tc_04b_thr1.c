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
/* PURPOSE: TH ZCR (test driver, target)
*/


#define ZB_TEST_NAME DR_TAR_TC_04B_THR1
#define ZB_TRACE_FILE_ID 40979
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
#include "dr_tar_tc_04b_common.h"
#include "on_off_client.h"


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

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &attr_identify_time);

static zb_bool_t attr_on_off = 1;
ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(on_off_attr_list, &attr_on_off);

static zb_uint8_t attr_name_support = 0;
ZB_ZCL_DECLARE_GROUPS_ATTRIB_LIST(group_attr_list, &attr_name_support);


/********************* Declare device **************************/
DECLARE_ON_OFF_CLIENT_CLUSTER_LIST(on_off_client_clusters,
                                   basic_attr_list,
                                   identify_attr_list);

DECLARE_ON_OFF_CLIENT_EP(on_off_client_ep, TH_ZCRX_ENDPOINT, on_off_client_clusters);

DECLARE_ON_OFF_CLIENT_CTX(on_off_client_ctx, on_off_client_ep);


/**********************General definitions for test***********************/
enum top_level_test_steps_e
{
    TOP_LEVEL_CLOSE_NETWORK,
    TOP_LEVEL_MGMT_BIND,
    TOP_LEVEL_READ_REPORTING,
    TOP_LEVEL_READ_DUT_ATTR,
    TOP_LEVEL_MGMT_LQI
};


#define MAX_FUNC_PARAMS 3
typedef struct test_params_s
{
    int         top_level_state;
    zb_uint16_t func_params[MAX_FUNC_PARAMS];
    zb_uint16_t dest_addr;
    zb_uint8_t  dest_ep;
} test_params_t;


static void top_level_fsm(zb_uint8_t unused);
static zb_uint8_t th_zcl_handler(zb_uint8_t param);
static void top_level_forward_test(zb_uint8_t unused);

static void send_mgmt_bind_req(zb_uint8_t param, zb_uint16_t index);
static void mgmt_bind_resp_cb(zb_uint8_t param);
static void send_read_attr_req(zb_uint8_t param);
static void send_read_reporting_config_req(zb_uint8_t param);
static void send_mgmt_lqi_req(zb_uint8_t param, zb_uint16_t index);
static void mgmt_lqi_resp_cb(zb_uint8_t param);

static void close_network_req(zb_uint8_t param);

static void device_annce_cb(zb_zdo_device_annce_t *da);

static test_params_t s_test_desc;


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_thr1");


    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_thr1);

    ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
    ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;
    ZB_BDB().bdb_mode = 1;

    ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ROUTER;
    ZB_IEEE_ADDR_COPY(ZB_AIB().trust_center_address, g_unknown_ieee_addr);
    zb_secur_setup_nwk_key(g_nwk_key, 0);
    zb_zdo_register_device_annce_cb(device_annce_cb);

    ZB_AF_REGISTER_DEVICE_CTX(&on_off_client_ctx);
    ZB_AF_SET_ENDPOINT_HANDLER(TH_ZCRX_ENDPOINT, th_zcl_handler);

    if (zdo_dev_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
    }
    else
    {
        zdo_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}


/******************************Implementation********************************/
/* main fsm */
static void top_level_fsm(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    TRACE_MSG(TRACE_APP1, ">>top_level_fsm: top_level_state = %d",
              (FMT__D, s_test_desc.top_level_state));

    switch (s_test_desc.top_level_state)
    {
    case TOP_LEVEL_CLOSE_NETWORK:
    {
        ZB_GET_OUT_BUF_DELAYED(close_network_req);
        ZB_SCHEDULE_ALARM(top_level_forward_test, 0, THR1_START_TEST_DELAY);
    }
    break;
    case TOP_LEVEL_MGMT_BIND:
    {
        ZB_GET_OUT_BUF_DELAYED2(send_mgmt_bind_req, 0);
    }
    break;
    case TOP_LEVEL_READ_REPORTING:
    {
        ZB_GET_OUT_BUF_DELAYED(send_read_reporting_config_req);
    }
    break;
    case TOP_LEVEL_READ_DUT_ATTR:
    {
        ZB_GET_OUT_BUF_DELAYED(send_read_attr_req);
    }
    break;
    case TOP_LEVEL_MGMT_LQI:
    {
        ZB_GET_OUT_BUF_DELAYED2(send_mgmt_lqi_req, 0);
    }
    break;
    default:
    {
        TRACE_MSG(TRACE_APP1, "top_level_fsm: unknown state", (FMT__0));
    }
    }

    TRACE_MSG(TRACE_APP1, "<<top_level_fsm", (FMT__0));
}


static zb_uint8_t th_zcl_handler(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_zcl_parsed_hdr_t *cmd_info = ZB_GET_BUF_PARAM(buf, zb_zcl_parsed_hdr_t);
    int is_command_of_interest = 0;

    TRACE_MSG(TRACE_ZCL1, ">>th_zcl_handler: buf_param = %d, top_level_state = %d",
              (FMT__D_D_D, param, s_test_desc.top_level_state));

    if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI
            && cmd_info->is_common_command)
    {
        switch (cmd_info->cmd_id)
        {
        case ZB_ZCL_CMD_READ_REPORT_CFG_RESP:
        case ZB_ZCL_CMD_READ_ATTRIB_RESP:
        case ZB_ZCL_CMD_DEFAULT_RESP:
            is_command_of_interest = 1;
            break;
        }
    }

    if (is_command_of_interest)
    {
        top_level_forward_test(0);
    }

    TRACE_MSG(TRACE_ZCL1, "<<th_zcl_handler", (FMT__0));

    return ZB_FALSE;
}


static void top_level_forward_test(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    TRACE_MSG(TRACE_APP1, ">>top_level_forward_test: current top_level_state = %d",
              (FMT__D, s_test_desc.top_level_state));

    switch (s_test_desc.top_level_state)
    {
    case TOP_LEVEL_CLOSE_NETWORK:
    case TOP_LEVEL_MGMT_BIND:
    case TOP_LEVEL_READ_REPORTING:
    case TOP_LEVEL_READ_DUT_ATTR:
    {
        TRACE_MSG(TRACE_APP1, "top_level_forward_test: go to next step", (FMT__0));
        ++s_test_desc.top_level_state;
        ZB_SCHEDULE_ALARM(top_level_fsm, 0, TH_SHORT_DELAY);
    }
    break;

    case TOP_LEVEL_MGMT_LQI:
    {
        TRACE_MSG(TRACE_APP1, "top_level_forward_test: test finished", (FMT__0));
    }
    break;

    default:
    {
        TRACE_MSG(TRACE_APP1, "top_level_forward_test: unknown state", (FMT__0));
    }
    }

    TRACE_MSG(TRACE_APP1, "<<top_level_forward_test", (FMT__0));
}


static void send_mgmt_bind_req(zb_uint8_t param, zb_uint16_t index)
{
    zb_buf_t *buf;
    zb_zdo_mgmt_bind_param_t *req_params;

    TRACE_MSG(TRACE_ZDO3, ">>send_mgmt_bind_req: buf_param = %i, start_at = %d",
              (FMT__D_D, param, index));

    buf = ZB_BUF_FROM_REF(param);
    req_params = ZB_GET_BUF_PARAM(buf, zb_zdo_mgmt_bind_param_t);
    req_params->start_index = index;
    req_params->dst_addr = s_test_desc.dest_addr;
    zb_zdo_mgmt_bind_req(param, mgmt_bind_resp_cb);

    TRACE_MSG(TRACE_ZDO3, "<<send_mgmt_bind_req", (FMT__0));
}


static void mgmt_bind_resp_cb(zb_uint8_t param)
{
    zb_buf_t *buf;
    zb_zdo_mgmt_bind_resp_t *resp;
    int next_step = 0;

    buf = ZB_BUF_FROM_REF(param);
    resp = (zb_zdo_mgmt_bind_resp_t *) ZB_BUF_BEGIN(buf);

    TRACE_MSG(TRACE_ZDO1, ">>mgmt_bind_resp_cb: buf_param = %i, status = %i",
              (FMT__D_D, param, resp->status));

    if (resp->status == ZB_ZDP_STATUS_SUCCESS)
    {
        zb_uint8_t nbrs = resp->binding_table_list_count;
        zb_uint8_t start_idx = resp->start_index;

        if (start_idx + nbrs < resp->binding_table_entries)
        {
            ZB_GET_OUT_BUF_DELAYED2(send_mgmt_bind_req, start_idx + nbrs);
        }
        else
        {
            next_step = 1;
        }
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "mgmt_bind_resp_cb: failed to check", (FMT__0));
        next_step = 1;
    }

    zb_free_buf(buf);
    if (next_step)
    {
        top_level_forward_test(0);
    }

    TRACE_MSG(TRACE_ZDO1, "<<mgmt_bind_resp_cb", (FMT__0));
}


static void send_read_attr_req(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_uint8_t *ptr;
    zb_uint16_t dest_addr = s_test_desc.dest_addr;
    zb_uint8_t dest_ep = s_test_desc.dest_ep;

    TRACE_MSG(TRACE_ZCL2, ">>send_read_attr_req: buf_param = %d", (FMT__D, param));

    ZB_ZCL_GENERAL_INIT_READ_ATTR_REQ_A(buf, ptr, ZB_ZCL_FRAME_DIRECTION_TO_SRV,
                                        ZB_ZCL_ENABLE_DEFAULT_RESPONSE);
    ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(ptr, ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID);
    ZB_ZCL_GENERAL_SEND_READ_ATTR_REQ(buf, ptr, dest_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                      dest_ep, TH_ZCRX_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                      ZB_ZCL_CLUSTER_ID_ON_OFF, NULL);

    TRACE_MSG(TRACE_ZCL2, "<<send_read_attr_req", (FMT__0));
}


static void send_read_reporting_config_req(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_uint8_t *ptr;
    zb_uint16_t dest_addr = s_test_desc.dest_addr;
    zb_uint8_t dest_ep = s_test_desc.dest_ep;

    TRACE_MSG(TRACE_ZCL2, ">>send_read_reporting_config_req: buf_param = %d", (FMT__D, param));

    ZB_ZCL_GENERAL_INIT_READ_REPORTING_CONFIGURATION_SRV_REQ(
        buf, ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);

    ZB_ZCL_GENERAL_ADD_SEND_READ_REPORTING_CONFIGURATION_REQ(
        ptr, ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID);

    ZB_ZCL_GENERAL_SEND_READ_REPORTING_CONFIGURATION_REQ(
        buf, ptr, dest_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
        dest_ep, TH_ZCRX_ENDPOINT, ZB_AF_HA_PROFILE_ID,
        ZB_ZCL_CLUSTER_ID_ON_OFF, NULL);

    TRACE_MSG(TRACE_ZCL2, "<<send_read_reporting_config_req", (FMT__0));
}


static void send_mgmt_lqi_req(zb_uint8_t param, zb_uint16_t index)
{
    zb_buf_t *buf;
    zb_zdo_mgmt_lqi_param_t *req_params;

    TRACE_MSG(TRACE_ZDO3, ">>send_mgmt_lqi_req: buf_param = %i, start_at = %d",
              (FMT__D_D, param, index));

    buf = ZB_BUF_FROM_REF(param);
    req_params = ZB_GET_BUF_PARAM(buf, zb_zdo_mgmt_lqi_param_t);
    req_params->start_index = index;
    req_params->dst_addr = s_test_desc.dest_addr;
    zb_zdo_mgmt_lqi_req(param, mgmt_lqi_resp_cb);

    TRACE_MSG(TRACE_ZDO3, "<<send_mgmt_lqi_req", (FMT__0));
}


static void mgmt_lqi_resp_cb(zb_uint8_t param)
{
    zb_buf_t *buf;
    zb_zdo_mgmt_lqi_resp_t *resp;
    int next_step = 0;

    buf = ZB_BUF_FROM_REF(param);
    resp = (zb_zdo_mgmt_lqi_resp_t *) ZB_BUF_BEGIN(buf);

    TRACE_MSG(TRACE_ZDO1, ">>mgmt_lqi_resp_cb: buf_param = %i, status = %i",
              (FMT__D_D, param, resp->status));

    if (resp->status == ZB_ZDP_STATUS_SUCCESS)
    {
        zb_uint8_t nbrs = resp->neighbor_table_list_count;
        zb_uint8_t start_idx = resp->start_index;

        if (start_idx + nbrs < resp->neighbor_table_entries)
        {
            ZB_GET_OUT_BUF_DELAYED2(send_mgmt_lqi_req, start_idx + nbrs);
        }
        else
        {
            next_step = 1;
        }
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "mgmt_lqi_resp_cb: failed to check", (FMT__0));
        next_step = 1;
    }

    zb_free_buf(buf);
    if (next_step)
    {
        top_level_forward_test(0);
    }

    TRACE_MSG(TRACE_ZDO1, "<<mgmt_lqi_resp_cb", (FMT__0));
}



static void close_network_req(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_nlme_permit_joining_request_t *req;

    TRACE_MSG(TRACE_ZDO1, ">>close_network_req: buf_param = %d", (FMT__D, param));

    req = ZB_GET_BUF_TAIL(buf, sizeof(zb_nlme_permit_joining_request_t));
    req->permit_duration = 0;
    zb_nlme_permit_joining_request(param);

    TRACE_MSG(TRACE_ZDO1, "<<close_network_req", (FMT__0));
}



static void device_annce_cb(zb_zdo_device_annce_t *da)
{
    TRACE_MSG(TRACE_APP2, ">>device_annce_cb, da %p", (FMT__P, da));

    if (ZB_IEEE_ADDR_CMP(da->ieee_addr, g_ieee_addr_dut))
    {
        s_test_desc.dest_addr = zb_address_short_by_ieee(g_ieee_addr_dut);
        s_test_desc.dest_ep = DUT_ENDPOINT1;
        ZB_SCHEDULE_ALARM(top_level_fsm, 0, TH_ZCRX_CLOSE_NETWORK_DELAY);
    }

    TRACE_MSG(TRACE_APP2, "<<device_annce_cb", (FMT__0));
}



ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
            TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
            break;

        default:
            TRACE_MSG(TRACE_APS1, "Unknown signal", (FMT__0));
        }
    }
    else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
    {
        TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
    }
    zb_free_buf(ZB_BUF_FROM_REF(param));
}

/*! @} */
