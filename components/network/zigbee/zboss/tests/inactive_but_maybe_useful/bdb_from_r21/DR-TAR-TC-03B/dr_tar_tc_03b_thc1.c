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
/* PURPOSE: TH ZC1 (test driver, target)
*/


#define ZB_TEST_NAME DR_TAR_TC_03B_THC1
#define ZB_TRACE_FILE_ID 40993
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
#include "dr_tar_tc_03b_common.h"
#include "on_off_client.h"


#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#ifndef ZB_CERTIFICATION_HACKS
#error ZB_CERTIFICATION_HACKS is not compiled!
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
    TOP_LEVEL_START_FB_AS_TARGET,
    TOP_LEVEL_PREPARATORY_STEPS,
    TOP_LEVEL_SEND_NWK_LEAVE,
    TOP_LEVEL_RETRIGGER_NETWORK_STEERING
};


/* low level and preparatory steps */
enum low_level_test_steps_e
{
    LOW_LEVEL_INITIAL_MGMT_BIND,
    LOW_LEVEL_READ_ATTR_DEF_VAL,
    LOW_LEVEL_CHANGE_ATTR_VAL,
    LOW_LEVEL_READ_ATTR_NEW_VAL,
    LOW_LEVEL_READ_REPORTING_CONFIG_DEF,
    LOW_LEVEL_CHANGE_REPORTING_CONFIG,
    LOW_LEVEL_READ_REPORTING_CONFIG_NEW
};


#define MAX_FUNC_PARAMS 3
typedef struct test_params_s
{
    int         top_level_state;
    int         low_level_state;
    zb_uint16_t min_report_int;
    zb_uint16_t max_report_int;
    zb_uint16_t func_params[MAX_FUNC_PARAMS];
    zb_uint16_t dest_addr;
    zb_uint8_t  dest_ep;
} test_params_t;


static void top_level_fsm(zb_uint8_t unused);
static void low_level_fsm(zb_uint8_t unused);
static zb_uint8_t th_zcl_handler(zb_uint8_t param);
static void copy_report_info(zb_buf_t *buf);
static void top_level_forward_test(zb_uint8_t unused);
static void low_level_forward_test(zb_uint8_t unused);

static void send_mgmt_bind_req(zb_uint8_t param, zb_uint16_t index);
static void mgmt_bind_resp_cb(zb_uint8_t param);
static void send_read_attr_req(zb_uint8_t param);
static void send_toggle_on_off(zb_uint8_t param);
static void send_read_reporting_config_req(zb_uint8_t param);
static void send_config_reporting_req(zb_uint8_t param);

static void close_network_req(zb_uint8_t param);
static void send_nwk_leave_req(zb_uint8_t param);

static void device_annce_cb(zb_zdo_device_annce_t *da);
static void trigger_fb_target(zb_uint8_t unused);

static test_params_t s_test_desc;


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_thc1");


    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_thc1);

    ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
    ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;
    ZB_BDB().bdb_mode = 1;

    ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_COORDINATOR;
    ZB_AIB().aps_designated_coordinator = 1;
    zb_secur_setup_nwk_key(g_nwk_key, 0);
    zb_zdo_register_device_annce_cb(device_annce_cb);
    ZB_CERT_HACKS().enable_leave_to_router_hack = 1;

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
        ZB_SCHEDULE_ALARM(top_level_forward_test, 0, THC1_FB_START_DELAY);
    }
    break;
    case TOP_LEVEL_START_FB_AS_TARGET:
    {
        trigger_fb_target(0);
    }
    break;
    case TOP_LEVEL_PREPARATORY_STEPS:
    {
        ZB_SCHEDULE_CALLBACK(low_level_fsm, 0);
    }
    break;
    case TOP_LEVEL_SEND_NWK_LEAVE:
    {
        ZB_GET_OUT_BUF_DELAYED(send_nwk_leave_req);
        ZB_SCHEDULE_ALARM(top_level_forward_test, 0, TH_SHORT_DELAY);
    }
    break;
    case TOP_LEVEL_RETRIGGER_NETWORK_STEERING:
    {
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
    }
    break;
    default:
    {
        TRACE_MSG(TRACE_APP1, "top_level_fsm: unknown state", (FMT__0));
    }
    }

    TRACE_MSG(TRACE_APP1, "<<top_level_fsm", (FMT__0));
}


/* chain fsm */
static void low_level_fsm(zb_uint8_t unused)
{
    zb_callback_t next_func = NULL;

    ZVUNUSED(unused);
    TRACE_MSG(TRACE_APP1, ">>low_level_fsm: low_level_state = %d",
              (FMT__D, s_test_desc.low_level_state));

    switch (s_test_desc.low_level_state)
    {
    case LOW_LEVEL_INITIAL_MGMT_BIND:
    {
        ZB_GET_OUT_BUF_DELAYED2(send_mgmt_bind_req, 0);
    }
    break;

    case LOW_LEVEL_READ_ATTR_DEF_VAL:
    case LOW_LEVEL_READ_ATTR_NEW_VAL:
    {
        next_func = send_read_attr_req;
    }
    break;

    case LOW_LEVEL_CHANGE_ATTR_VAL:
    {
        next_func = send_toggle_on_off;
    }
    break;

    case LOW_LEVEL_READ_REPORTING_CONFIG_DEF:
    case LOW_LEVEL_READ_REPORTING_CONFIG_NEW:
    {
        next_func = send_read_reporting_config_req;
    }
    break;

    case LOW_LEVEL_CHANGE_REPORTING_CONFIG:
    {
        s_test_desc.func_params[0] = s_test_desc.min_report_int / 2;
        s_test_desc.func_params[1] = s_test_desc.max_report_int / 2;
        next_func = send_config_reporting_req;
    }
    break;

    default:
    {
        TRACE_MSG(TRACE_APP1, "low_level_fsm: unknown state", (FMT__0));
    }
    }

    if (next_func)
    {
        ZB_GET_OUT_BUF_DELAYED(next_func);
    }

    TRACE_MSG(TRACE_APP1, "<<low_level_fsm", (FMT__0));
}


static zb_uint8_t th_zcl_handler(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_zcl_parsed_hdr_t *cmd_info = ZB_GET_BUF_PARAM(buf, zb_zcl_parsed_hdr_t);
    int is_command_of_interest = 0;

    TRACE_MSG(TRACE_ZCL1, ">>th_zcl_handler: buf_param = %d, top_level_state = %d, lop_level_state = %d",
              (FMT__D_D_D, param, s_test_desc.top_level_state, s_test_desc.low_level_state));

    if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI
            && cmd_info->is_common_command)
    {
        switch (cmd_info->cmd_id)
        {
        case ZB_ZCL_CMD_READ_REPORT_CFG_RESP:
            copy_report_info(buf);
            is_command_of_interest = 1;
            break;

        case ZB_ZCL_CMD_READ_ATTRIB_RESP:
            is_command_of_interest = 1;
            break;

        case ZB_ZCL_CMD_CONFIG_REPORT_RESP:
        case ZB_ZCL_CMD_DEFAULT_RESP:
            is_command_of_interest = 1;
            break;
        }
    }

    if (is_command_of_interest)
    {
        low_level_forward_test(0);
    }

    TRACE_MSG(TRACE_ZCL1, "<<th_zcl_handler", (FMT__0));

    return ZB_FALSE;
}


static void copy_report_info(zb_buf_t *buf)
{
    zb_zcl_read_reporting_cfg_rsp_t *resp;

    ZB_ZCL_GENERAL_GET_READ_REPORTING_CONFIGURATION_RES(buf, resp);
    TRACE_MSG(TRACE_ZCL3, "copy_report_info: %p", (FMT__P, resp));
    TRACE_MSG(TRACE_ZCL3, "copy_report_info: len = %d, sz = %d",
              (FMT__D_D, ZB_BUF_LEN(buf), ZB_ZCL_READ_REPORTING_CFG_RES_SIZE));
    if (resp && resp->status == ZB_ZCL_STATUS_SUCCESS)
    {
        s_test_desc.min_report_int = resp->u.clnt.min_interval;
        s_test_desc.max_report_int = resp->u.clnt.max_interval;
    }
}


static void top_level_forward_test(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    TRACE_MSG(TRACE_APP1, ">>top_level_forward_test: current top_level_state = %d",
              (FMT__D, s_test_desc.top_level_state));

    switch (s_test_desc.top_level_state)
    {
    case TOP_LEVEL_CLOSE_NETWORK:
    case TOP_LEVEL_START_FB_AS_TARGET:
    {
        TRACE_MSG(TRACE_APP1, "top_level_forward_test: go to next step", (FMT__0));
        ++s_test_desc.top_level_state;
        ZB_SCHEDULE_ALARM(top_level_fsm, 0, TH_SHORT_DELAY);
    }
    break;

    case TOP_LEVEL_PREPARATORY_STEPS:
    {
        TRACE_MSG(TRACE_APP1, "top_level_forward_test: go to next step", (FMT__0));
        ++s_test_desc.top_level_state;
        ZB_SCHEDULE_ALARM(top_level_fsm, 0, THC1_SEND_NWK_LEAVE_DELAY);
    }
    break;

    case TOP_LEVEL_SEND_NWK_LEAVE:
    {
        TRACE_MSG(TRACE_APP1, "top_level_forward_test: go to next step", (FMT__0));
        ++s_test_desc.top_level_state;
        ZB_SCHEDULE_ALARM(top_level_fsm, 0, THC1_RETRIGGER_STEERING);
    }
    break;

    case TOP_LEVEL_RETRIGGER_NETWORK_STEERING:
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


static void low_level_forward_test(zb_uint8_t unused)
{
    int move_chain = 1;
    int forward_test = 0;

    ZVUNUSED(unused);
    TRACE_MSG(TRACE_APP1, ">>low_level_forward_test: current low_level_state = %d",
              (FMT__D, s_test_desc.low_level_state));

    switch (s_test_desc.low_level_state)
    {
    case LOW_LEVEL_INITIAL_MGMT_BIND:
    case LOW_LEVEL_READ_ATTR_DEF_VAL:
    case LOW_LEVEL_CHANGE_ATTR_VAL:
    case LOW_LEVEL_READ_ATTR_NEW_VAL:
    case LOW_LEVEL_READ_REPORTING_CONFIG_DEF:
    case LOW_LEVEL_CHANGE_REPORTING_CONFIG:
    {
        TRACE_MSG(TRACE_APP1, "low_level_forward_test: move to next step", (FMT__0));
        ++s_test_desc.low_level_state;
    }
    break;

    case LOW_LEVEL_READ_REPORTING_CONFIG_NEW:
    {
        TRACE_MSG(TRACE_APP1, "low_level_forward_test: chain finished", (FMT__0));
        move_chain = 0;
        forward_test = 1;
    }
    break;

    default:
    {
        TRACE_MSG(TRACE_APP1, "low_level_forward_test: unknown state", (FMT__0));
        move_chain = 0;
    }
    }

    if (move_chain)
    {
        ZB_SCHEDULE_ALARM(low_level_fsm, 0, TH_SHORT_DELAY);
    }

    if (forward_test)
    {
        ZB_SCHEDULE_ALARM(top_level_forward_test, 0, TH_SHORT_DELAY);
    }

    TRACE_MSG(TRACE_APP1, "<<low_level_forward_test", (FMT__0));
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
        TRACE_MSG(TRACE_ERROR, "check_bind_table_resp_cb: failed to check", (FMT__0));
        next_step = 1;
    }

    zb_free_buf(buf);
    if (next_step)
    {
        low_level_forward_test(0);
    }

    TRACE_MSG(TRACE_ZDO1, "<<check_bind_table_resp_cb", (FMT__0));
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


static void send_toggle_on_off(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_uint16_t dest_addr = s_test_desc.dest_addr;
    zb_uint8_t dest_ep = s_test_desc.dest_ep;

    TRACE_MSG(TRACE_ZCL2, ">>send_toggle_on_off: buf_param = %d", (FMT__D, param));

    ZB_ZCL_ON_OFF_SEND_TOGGLE_REQ(buf, dest_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                  dest_ep, TH_ZCRX_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                  ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);

    TRACE_MSG(TRACE_ZCL2, "<<send_toggle_on_off", (FMT__0));
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


static void send_config_reporting_req(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_uint8_t *ptr;
    zb_uint16_t dest_addr = s_test_desc.dest_addr;
    zb_uint8_t dest_ep = s_test_desc.dest_ep;

    TRACE_MSG(TRACE_ZCL2, ">>send_config_reporting_req: buf_param = %d", (FMT__D, param));

    ZB_ZCL_GENERAL_INIT_CONFIGURE_REPORTING_SRV_REQ(
        buf, ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);

    ZB_ZCL_GENERAL_ADD_SEND_REPORT_CONFIGURE_REPORTING_REQ(
        ptr, ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID, ZB_ZCL_ATTR_TYPE_BOOL,
        s_test_desc.func_params[0], s_test_desc.func_params[1],
        NULL);

    ZB_ZCL_GENERAL_SEND_CONFIGURE_REPORTING_REQ(
        buf, ptr, dest_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
        dest_ep, TH_ZCRX_ENDPOINT, ZB_AF_HA_PROFILE_ID,
        ZB_ZCL_CLUSTER_ID_ON_OFF, NULL);

    TRACE_MSG(TRACE_ZCL2, "<<send_config_reporting_req", (FMT__0));
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


static void send_nwk_leave_req(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_nlme_leave_request_t *req = ZB_GET_BUF_PARAM(buf, zb_nlme_leave_request_t);

    TRACE_MSG(TRACE_ZDO1, ">>send_nwk_leave_req: buf_param = %d", (FMT__D, param));

    req->remove_children = 0;
    req->rejoin = 0;
    ZB_IEEE_ADDR_COPY(req->device_address, g_ieee_addr_dut);
    zb_nlme_leave_request(param);

    TRACE_MSG(TRACE_ZDO1, "<<send_nwk_leave_req", (FMT__0));
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


static void trigger_fb_target(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    ZB_BDB().bdb_commissioning_time = FB_DURATION;
    zb_bdb_finding_binding_target(TH_ZCRX_ENDPOINT);
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

        case ZB_BDB_SIGNAL_STEERING:
            TRACE_MSG(TRACE_APS1, "Network steering", (FMT__0));
            if (s_test_desc.top_level_state == TOP_LEVEL_RETRIGGER_NETWORK_STEERING)
            {
                top_level_forward_test(0);
            }
            break;

        case ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED:
            TRACE_MSG(TRACE_APS1, "Finding&binding done", (FMT__0));
            if ( (BDB_COMM_CTX().state == ZB_BDB_COMM_IDLE) &&
                    (s_test_desc.top_level_state == TOP_LEVEL_START_FB_AS_TARGET) )
            {
                top_level_forward_test(0);
            }
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
