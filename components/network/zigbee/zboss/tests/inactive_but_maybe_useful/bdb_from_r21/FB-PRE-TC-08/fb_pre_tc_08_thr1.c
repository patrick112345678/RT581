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
/* PURPOSE: TH ZR1 (configure reporting, reading reporting info)
*/


#define ZB_TEST_NAME FB_PRE_TC_08_THR1
#define ZB_TRACE_FILE_ID 41218
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
#include "test_device_initiator.h"
#include "fb_pre_tc_08_common.h"


#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#ifndef ZB_USE_NVRAM
#error ZB_USE_NVRAM is not compiled!
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

/********************* Declare device **************************/
DECLARE_INITIATOR_CLUSTER_LIST(initiator_device_clusters,
                               basic_attr_list,
                               identify_attr_list);

DECLARE_INITIATOR_EP(initiator_device_ep,
                     THR1_ENDPOINT,
                     initiator_device_clusters);

DECLARE_INITIATOR_CTX(initiator_device_ctx, initiator_device_ep);


/***********************F&B***************************************/
static void device_annce_cb(zb_zdo_device_annce_t *da);
static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster);
static void trigger_fb_initiator(zb_uint8_t unused);
static void trigger_fb_target(zb_uint8_t unused);


/************************General functions************************/
typedef struct ReportInfo_s
{
    zb_uint16_t attr;
    zb_uint8_t  attr_type;
    zb_uint16_t min_int, max_int;
    zb_uint32_t delta;
} ReportInfo_t;


typedef struct ReportCmd_s
{
    zb_uint16_t  dest_addr;
    zb_uint16_t  cluster;
    int          num_of_reports;
    ReportInfo_t reports[2];
} ReportCmd_t;


enum test_steps_e
{
    TEST_STEP_IDLE,
    TEST_STEP_READY,
    TEST_STEP_READ_SINGLE_REPORTING1,
    TEST_STEP_MODIFY_MIN_INT,
    TEST_STEP_MODIFY_MAX_INT,
    TEST_STEP_SEND_BIND_REQ,
    TEST_STEP_MODIFY_DELTA,
    TEST_STEP_RESET_MAX_INT,
    TEST_STEP_BRCAST_RESET_REPORTING,
    TEST_STEP_READ_SINGLE_REPORTING2,
    TEST_STEP_CONFIG_MULTIPLE_REPORTING1,
    TEST_STEP_READ_MULTIPLE_REPORTING1,
    TEST_STEP_CONFIG_MULTIPLE_REPORTING2,
    TEST_STEP_READ_MULTIPLE_REPORTING2
};

/* returns mandatory reportable attribute of the cluster */
static zb_uint16_t cluster_to_attr(zb_uint16_t cluster);
static void initiate_reporting_test(zb_uint8_t unused);
static void buffer_manager(zb_uint8_t test_step);
static void read_reporting_config(zb_uint8_t param);
static void config_reporting(zb_uint8_t  param);
static void send_bind_req(zb_uint8_t param);
static void bind_resp_cb(zb_uint8_t param);
static zb_uint8_t zcl_resp_handler(zb_uint8_t param);
static void copy_report_info(zb_buf_t *buf);
static int move_to_next_state(zb_callback_t *cb_ptr);


/************************Static variables*************************/
static zb_uint16_t  s_match_cluster_list[MATCH_LIST_MAX_SIZE];
static int          s_match_cluster_list_size, s_match_cluster_idx;
static ReportInfo_t s_stored_reports[2];
static ReportCmd_t  s_report_cmd;
static int          s_test_step;


/************************Main*************************************/
MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_thr1");


    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_thr1);
    ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
    ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;
    ZB_BDB().bdb_mode = 1;
    ZB_AIB().aps_use_nvram = 1;

    /* Assignment required to force Distributed formation */
    ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ROUTER;
    ZB_IEEE_ADDR_COPY(ZB_AIB().trust_center_address, g_unknown_ieee_addr);
    zb_secur_setup_nwk_key(g_nwk_key, 0);

    ZB_AF_REGISTER_DEVICE_CTX(&initiator_device_ctx);
    ZB_AF_SET_ENDPOINT_HANDLER(THR1_ENDPOINT, zcl_resp_handler);

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


/***********************************Implementation**********************************/
static void device_annce_cb(zb_zdo_device_annce_t *da)
{
    TRACE_MSG(TRACE_ZDO1, ">>dev_annce_cb, ieee = " TRACE_FORMAT_64 "addr = %x",
              (FMT__A_H, TRACE_ARG_64(da->ieee_addr), da->nwk_addr));
    if (ZB_IEEE_ADDR_CMP(g_ieee_addr_dut, da->ieee_addr) == ZB_TRUE)
    {
        TRACE_MSG(TRACE_ZDO2, "dev_annce_cb: DUT ZED has joined to network!", (FMT__0));
    }
    TRACE_MSG(TRACE_ZDO1, "<<dev_annce_cb", (FMT__0));
}


static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster)
{
    TRACE_MSG(TRACE_ZCL1, "finding_binding_cb status %d addr " TRACE_FORMAT_64 " ep %hd cluster %d",
              (FMT__D_A_H_D, status, TRACE_ARG_64(addr), ep, cluster));
    if (s_match_cluster_list_size < MATCH_LIST_MAX_SIZE &&
            cluster != ZB_ZCL_CLUSTER_ID_IDENTIFY &&
            cluster != ZB_ZCL_CLUSTER_ID_BASIC &&
            !status)
    {
        s_match_cluster_list[s_match_cluster_list_size++] = cluster;
    }
    return ZB_TRUE;
}


static void trigger_fb_initiator(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    ZB_BDB().bdb_commissioning_time = DUT_FB_DURATION;
    s_test_step = TEST_STEP_READY;
    zb_bdb_finding_binding_initiator(THR1_ENDPOINT, finding_binding_cb);
}


static void trigger_fb_target(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    ZB_BDB().bdb_commissioning_time = THR1_FB_DURATION;
    zb_bdb_finding_binding_target(THR1_ENDPOINT);
}


/************************General functions************************/
static zb_uint16_t cluster_to_attr(zb_uint16_t cluster)
{
    zb_uint16_t attr = 0xffff;

    switch (cluster)
    {
    case ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT:
        attr = ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID;
        break;
    case ZB_ZCL_CLUSTER_ID_ILLUMINANCE_MEASUREMENT:
        attr = ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MEASURED_VALUE_ID;
        break;
    case ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT:
        attr = ZB_ZCL_ATTR_REL_HUMIDITY_MEASUREMENT_VALUE_ID;
        break;
    }

    return attr;
}


static zb_uint8_t get_attr_type(zb_uint16_t cluster, zb_uint16_t attr)
{
    zb_uint8_t attr_type = ZB_ZCL_ATTR_TYPE_NULL;

    if (cluster == ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT)
    {
        switch (attr)
        {
        case ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID:
            attr_type = ZB_ZCL_ATTR_TYPE_S16;
            break;
        case ZB_ZCL_ATTR_TEMP_MEASUREMENT_TOLERANCE_ID:
            attr_type = ZB_ZCL_ATTR_TYPE_U16;
            break;
        }
    }
    else if (cluster == ZB_ZCL_CLUSTER_ID_ILLUMINANCE_MEASUREMENT)
    {
        switch (attr)
        {
        case ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MEASURED_VALUE_ID:
            attr_type = ZB_ZCL_ATTR_TYPE_U16;
            break;
        }
    }
    else if (cluster == ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT)
    {
        switch (attr)
        {
        case ZB_ZCL_ATTR_REL_HUMIDITY_MEASUREMENT_VALUE_ID:
            attr_type = ZB_ZCL_ATTR_TYPE_U16;
            break;
        }
    }

    return attr_type;
}


static void initiate_reporting_test(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    TRACE_MSG(TRACE_ZCL1, ">>initiate_reporting_test, test_step = %d, matchidx = %d",
              (FMT__D_D, s_test_step, s_match_cluster_idx));

    if (s_match_cluster_idx < s_match_cluster_list_size)
    {
        s_report_cmd.dest_addr      = zb_address_short_by_ieee(g_ieee_addr_dut);
        s_report_cmd.cluster        = s_match_cluster_list[s_match_cluster_idx];
        s_report_cmd.num_of_reports = 1;
        buffer_manager(TEST_STEP_READ_SINGLE_REPORTING1);
    }
    else if (s_test_step == TEST_STEP_CONFIG_MULTIPLE_REPORTING1)
    {
        s_report_cmd.dest_addr       = zb_address_short_by_ieee(g_ieee_addr_dut);
        s_report_cmd.cluster         = ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT;
        s_report_cmd.num_of_reports  = 2;
        s_report_cmd.reports[0].attr = ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID;
        s_report_cmd.reports[0].attr_type = get_attr_type(ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
                                            ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID);
        s_report_cmd.reports[1].attr = 0x78;
        s_report_cmd.reports[1].attr_type = ZB_ZCL_ATTR_TYPE_S16;
        buffer_manager(TEST_STEP_CONFIG_MULTIPLE_REPORTING1);
    }
    else if (s_test_step == TEST_STEP_CONFIG_MULTIPLE_REPORTING2)
    {
        s_report_cmd.dest_addr       = zb_address_short_by_ieee(g_ieee_addr_dut);
        s_report_cmd.cluster         = ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT;
        s_report_cmd.num_of_reports  = 2;
        s_report_cmd.reports[0].attr = ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID;
        s_report_cmd.reports[0].attr_type = get_attr_type(ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
                                            ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID);
        s_report_cmd.reports[1].attr = ZB_ZCL_ATTR_TEMP_MEASUREMENT_TOLERANCE_ID;
        s_report_cmd.reports[1].attr_type = get_attr_type(ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
                                            ZB_ZCL_ATTR_TEMP_MEASUREMENT_TOLERANCE_ID);
        buffer_manager(TEST_STEP_CONFIG_MULTIPLE_REPORTING2);
    }

    TRACE_MSG(TRACE_ZCL1, "<<initiate_reporting_test", (FMT__0));
}


static void buffer_manager(zb_uint8_t test_step)
{
    zb_callback_t next_func = NULL;
    zb_uint16_t attr1 = cluster_to_attr(s_report_cmd.cluster);

    TRACE_MSG(TRACE_ZCL1, ">>buffer_manager: test_step = %d, cluster = 0x%x",
              (FMT__D, test_step, s_report_cmd.cluster));

    s_test_step = test_step;

    switch (test_step)
    {
    case TEST_STEP_READ_SINGLE_REPORTING1:
    {
        s_report_cmd.reports[0].attr = attr1;
        next_func = read_reporting_config;
    }
    break;

    case TEST_STEP_MODIFY_MIN_INT:
    {
        s_report_cmd.reports[0].attr = attr1;
        s_report_cmd.reports[0].attr_type = get_attr_type(s_report_cmd.cluster, attr1);
        s_report_cmd.reports[0].min_int = s_stored_reports[0].min_int / 2;
        s_report_cmd.reports[0].max_int = s_stored_reports[0].max_int;
        s_report_cmd.reports[0].delta = s_stored_reports[0].delta;
        next_func = config_reporting;
    }
    break;

    case TEST_STEP_MODIFY_MAX_INT:
    {
        zb_uint16_t new_int = s_stored_reports[0].min_int +
                              (s_stored_reports[0].max_int - s_stored_reports[0].min_int) / 10;
        s_report_cmd.reports[0].attr = attr1;
        s_report_cmd.reports[0].attr_type = get_attr_type(s_report_cmd.cluster, attr1);
        s_report_cmd.reports[0].max_int = new_int;
        s_report_cmd.reports[0].delta = s_stored_reports[0].delta;
        next_func = config_reporting;
    }
    break;

    case TEST_STEP_SEND_BIND_REQ:
    {
        next_func = send_bind_req;
    }
    break;

    case TEST_STEP_MODIFY_DELTA:
    {
        s_report_cmd.reports[0].attr = attr1;
        s_report_cmd.reports[0].attr_type = get_attr_type(s_report_cmd.cluster, attr1);
        s_report_cmd.reports[0].delta = s_stored_reports[0].delta - 1;
        next_func = config_reporting;
    }
    break;

    case TEST_STEP_RESET_MAX_INT:
    {
        s_report_cmd.reports[0].attr = attr1;
        s_report_cmd.reports[0].attr_type = get_attr_type(s_report_cmd.cluster, attr1);
        s_report_cmd.reports[0].max_int = 0xffff;
        next_func = config_reporting;
    }
    break;

    case TEST_STEP_BRCAST_RESET_REPORTING:
    {
        s_report_cmd.dest_addr = 0xffff;
        s_report_cmd.reports[0].attr = attr1;
        s_report_cmd.reports[0].attr_type = get_attr_type(s_report_cmd.cluster, attr1);
        s_report_cmd.reports[0].min_int = s_stored_reports[0].min_int;
        s_report_cmd.reports[0].max_int = s_stored_reports[0].max_int;
        s_report_cmd.reports[0].delta = s_stored_reports[0].delta;
        next_func = config_reporting;
    }
    break;

    case TEST_STEP_READ_SINGLE_REPORTING2:
    {
        s_report_cmd.dest_addr = zb_address_short_by_ieee(g_ieee_addr_dut);
        s_report_cmd.reports[0].attr = attr1;
        next_func = read_reporting_config;
    }
    break;

    case TEST_STEP_CONFIG_MULTIPLE_REPORTING1:
    case TEST_STEP_CONFIG_MULTIPLE_REPORTING2:
    {
        int i;

        for (i = 0; i < s_report_cmd.num_of_reports; ++i)
        {
            s_report_cmd.reports[i].min_int = s_stored_reports[0].min_int / 2;
            s_report_cmd.reports[i].max_int = s_stored_reports[0].max_int / 2;
            s_report_cmd.reports[i].delta = 2;
        }

        next_func = config_reporting;
    }
    break;

    case TEST_STEP_READ_MULTIPLE_REPORTING1:
    case TEST_STEP_READ_MULTIPLE_REPORTING2:
    {
        next_func = read_reporting_config;
    }
    break;

    default:
        TRACE_MSG(TRACE_ZCL1, "buffer_manager: unknown state transition", (FMT__0));
    }

    if (next_func)
    {
        ZB_GET_OUT_BUF_DELAYED(next_func);
    }

    TRACE_MSG(TRACE_ZCL1, "<<buffer_manager", (FMT__0));
}


static void read_reporting_config(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_uint8_t *cmd_ptr;
    int i;

    TRACE_MSG(TRACE_ZCL2, ">>read_reporting_config: buf_param = %d, cluster = 0x%x",
              (FMT__D_H, param, s_report_cmd.cluster));

    ZB_ZCL_GENERAL_INIT_READ_REPORTING_CONFIGURATION_SRV_REQ(
        buf, cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);

    for (i = 0; i < s_report_cmd.num_of_reports; ++i)
    {
        ZB_ZCL_GENERAL_ADD_SEND_READ_REPORTING_CONFIGURATION_REQ(
            cmd_ptr, s_report_cmd.reports[i].attr);
    }

    ZB_ZCL_GENERAL_SEND_READ_REPORTING_CONFIGURATION_REQ(
        buf, cmd_ptr, s_report_cmd.dest_addr,
        ZB_APS_ADDR_MODE_16_ENDP_PRESENT, DUT_ENDPOINT, THR1_ENDPOINT,
        ZB_AF_HA_PROFILE_ID, s_report_cmd.cluster, NULL);

    TRACE_MSG(TRACE_ZCL2, "<<read_reporting_config", (FMT__0));
}


static void config_reporting(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_uint8_t *cmd_ptr;
    int i;

    TRACE_MSG(TRACE_ZCL2, ">>config_reporting: buf_param = %d, cluster = 0x%x",
              (FMT__D_H, param, s_report_cmd.cluster));

    ZB_ZCL_GENERAL_INIT_CONFIGURE_REPORTING_SRV_REQ(
        buf, cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);

    for (i = 0; i < s_report_cmd.num_of_reports; ++i)
    {
        ZB_ZCL_GENERAL_ADD_SEND_REPORT_CONFIGURE_REPORTING_REQ(
            cmd_ptr, s_report_cmd.reports[i].attr, s_report_cmd.reports[i].attr_type,
            s_report_cmd.reports[i].min_int, s_report_cmd.reports[i].max_int,
            (zb_uint8_t *) &s_report_cmd.reports[i].delta);
    }

    ZB_ZCL_GENERAL_SEND_CONFIGURE_REPORTING_REQ(
        buf, cmd_ptr, s_report_cmd.dest_addr,
        ZB_APS_ADDR_MODE_16_ENDP_PRESENT, DUT_ENDPOINT, THR1_ENDPOINT,
        ZB_AF_HA_PROFILE_ID, s_report_cmd.cluster, NULL);

    TRACE_MSG(TRACE_ZCL2, "<<config_reporting", (FMT__0));
}


static void send_bind_req(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_zdo_bind_req_param_t *req = ZB_GET_BUF_PARAM(buf, zb_zdo_bind_req_param_t);

    TRACE_MSG(TRACE_ZDO2, ">>send_bind_req: buf_param = %d, cluster = 0x%x",
              (FMT__D_H, param, s_report_cmd.cluster));

    ZB_IEEE_ADDR_COPY(req->src_address, ZB_PIBCACHE_EXTENDED_ADDRESS());
    req->src_endp = THR1_ENDPOINT;
    req->cluster_id = s_report_cmd.cluster;
    req->dst_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
    ZB_IEEE_ADDR_COPY(req->dst_address.addr_long, g_ieee_addr_dut);
    req->dst_endp = DUT_ENDPOINT;
    req->req_dst_addr = zb_address_short_by_ieee(g_ieee_addr_dut);
    zb_zdo_bind_req(param, bind_resp_cb);

    TRACE_MSG(TRACE_ZDO2, "<<send_bind_req", (FMT__0));
}


static void bind_resp_cb(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ZDO2, ">>bind_resp_cb: buf_param = %d, cluster = 0x%x",
              (FMT__D_H, param, s_report_cmd.cluster));

    ZB_SCHEDULE_CALLBACK(buffer_manager, TEST_STEP_MODIFY_DELTA);
    zb_free_buf(ZB_BUF_FROM_REF(param));

    TRACE_MSG(TRACE_ZDO2, "<<bind_resp_cb", (FMT__0));
}


static zb_uint8_t zcl_resp_handler(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_zcl_parsed_hdr_t *cmd_info = ZB_GET_BUF_PARAM(buf, zb_zcl_parsed_hdr_t);
    zb_time_t next_delay = ZB_TIME_ONE_SECOND;
    zb_uint8_t next_param = 0;
    zb_callback_t next_func = NULL;

    TRACE_MSG(TRACE_ZCL1, ">>zcl_resp_handler: buf_param = %d, test_step = %d",
              (FMT__D_D, param, s_test_step));

    if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI
            && cmd_info->is_common_command)
    {
        switch (cmd_info->cmd_id)
        {
        case ZB_ZCL_CMD_READ_REPORT_CFG_RESP:
        {
            copy_report_info(buf);
            next_param = move_to_next_state(&next_func);
        }
        break;

        case ZB_ZCL_CMD_CONFIG_REPORT_RESP:
        {
            next_func = buffer_manager;
            switch (s_test_step)
            {
            case TEST_STEP_MODIFY_MIN_INT:
            case TEST_STEP_MODIFY_MAX_INT:
                next_param = s_test_step + 1;
                break;
            case TEST_STEP_MODIFY_DELTA:
                next_param = s_test_step + 1;
                next_delay = THR1_LONG_DELAY;
                break;
            case TEST_STEP_RESET_MAX_INT:
                next_param = s_test_step + 1;
                next_delay = 2 * THR1_LONG_DELAY;
                break;
            case TEST_STEP_BRCAST_RESET_REPORTING:
            case TEST_STEP_CONFIG_MULTIPLE_REPORTING1:
            case TEST_STEP_CONFIG_MULTIPLE_REPORTING2:
                next_param = s_test_step + 1;
                break;
            default:
                next_func = NULL;
            }
        }
        break;

        case ZB_ZCL_CMD_DEFAULT_RESP:
        {
            TRACE_MSG(TRACE_ZCL1, "zcl_resp_handler: Default Response", (FMT__0));
        }
        break;
        }
    }

    if (next_func)
    {
        ZB_SCHEDULE_ALARM(next_func, next_param, next_delay);
    }

    TRACE_MSG(TRACE_ZCL1, "<<zcl_resp_handler", (FMT__0));

    return ZB_FALSE;
}


static void copy_report_info(zb_buf_t *buf)
{
    int i, n = s_report_cmd.num_of_reports;
    zb_zcl_read_reporting_cfg_rsp_t *resp;

    for (i = 0; i < n; ++i)
    {
        ZB_ZCL_GENERAL_GET_READ_REPORTING_CONFIGURATION_RES(buf, resp);
        if (resp && resp->status == ZB_ZCL_STATUS_SUCCESS)
        {
            s_stored_reports[i].attr = resp->attr_id;
            s_stored_reports[i].attr_type = resp->u.clnt.attr_type;
            s_stored_reports[i].min_int = resp->u.clnt.min_interval;
            s_stored_reports[i].max_int = resp->u.clnt.max_interval;
            ZB_MEMCPY(&s_stored_reports[i].delta, resp->u.clnt.delta, 2);
        }
    }
}


static int move_to_next_state(zb_callback_t *cb_ptr)
{
    int next_step = 0;

    TRACE_MSG(TRACE_ZCL3, "move_to_next_state: match_idx = %d, test_step = %d",
              (FMT__D_D, s_match_cluster_idx, s_test_step));

    if (s_test_step == TEST_STEP_READ_SINGLE_REPORTING2)
    {
        *cb_ptr = initiate_reporting_test;
        if (++s_match_cluster_idx >= s_match_cluster_list_size)
        {
            ++s_test_step;
        }
    }
    else if (s_test_step == TEST_STEP_READ_MULTIPLE_REPORTING1)
    {
        *cb_ptr = initiate_reporting_test;
        s_test_step = TEST_STEP_CONFIG_MULTIPLE_REPORTING2;
    }
    else if (s_test_step == TEST_STEP_READ_MULTIPLE_REPORTING2)
    {
        *cb_ptr = NULL;
        TRACE_MSG(TRACE_ZCL1, "TEST FINISHED", (FMT__0));
    }
    else
    {
        *cb_ptr = buffer_manager;
        next_step = s_test_step + 1;
    }

    return next_step;
}


/********************ZDO Startup*****************************/
ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
            TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
            if (ZB_IEEE_ADDR_CMP(g_ieee_addr_thr1, ZB_NIB().extended_pan_id))
            {
                bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
            }
            else
            {
                ZB_SCHEDULE_ALARM(trigger_fb_target, 0, THR1_FB_TARGET_DELAY);
                ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, THR1_FB_INITIATOR_DELAY);
            }
            zb_zdo_register_device_annce_cb(device_annce_cb);
            break;

        case ZB_BDB_SIGNAL_STEERING:
            TRACE_MSG(TRACE_APS1, "Network steering", (FMT__0));
            ZB_SCHEDULE_ALARM(trigger_fb_target, 0, THR1_FB_TARGET_DELAY);
            ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, THR1_FB_INITIATOR_DELAY);
            break;

        case ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED:
        case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:
            TRACE_MSG(TRACE_APS1, "Finding&binding done", (FMT__0));
            if ((BDB_COMM_CTX().state == ZB_BDB_COMM_IDLE) && (s_test_step == TEST_STEP_READY))
            {
                s_test_step = TEST_STEP_READ_SINGLE_REPORTING1;
                ZB_SCHEDULE_ALARM(initiate_reporting_test,
                                  TEST_STEP_READ_SINGLE_REPORTING1,
                                  THR1_LONG_DELAY);
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
