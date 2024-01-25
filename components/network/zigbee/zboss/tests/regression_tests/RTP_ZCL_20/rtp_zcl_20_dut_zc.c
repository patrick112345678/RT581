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
/* PURPOSE: DUT ZC
*/

#define ZB_TEST_NAME RTP_ZCL_20_DUT_ZC
#define ZB_TRACE_FILE_ID 64001

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
#include "device_dut.h"
#include "rtp_zcl_20_common.h"
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

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_uint8_t g_nwk_key[16] = ZB_REG_TEST_DEFAULT_NWK_KEY;
static zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT_ZC;

/******************* Endpoint 1 ************************/

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version_1  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source_1 = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(rtp_zcl_20_dut_zc_basic_attr_list_1, &attr_zcl_version_1, &attr_power_source_1);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time_1 = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(rtp_zcl_20_dut_zc_identify_attr_list_1, &attr_identify_time_1);

/* On/Off cluster attributes data */
static zb_bool_t attr_on_off_1 = (zb_bool_t)ZB_ZCL_ON_OFF_IS_ON;
static zb_bool_t global_scene_ctrl_1 = ZB_ZCL_ON_OFF_GLOBAL_SCENE_CONTROL_DEFAULT_VALUE;
static zb_uint16_t on_time_1 = ZB_ZCL_ON_OFF_ON_TIME_DEFAULT_VALUE;
static zb_uint16_t off_wait_time_1 = ZB_ZCL_ON_OFF_OFF_WAIT_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST_EXT(rtp_zcl_20_dut_zc_on_off_attr_list_1,
                                      &attr_on_off_1,
                                      &global_scene_ctrl_1,
                                      &on_time_1,
                                      &off_wait_time_1);

/* Level control attributes data */
static zb_uint8_t attr_current_level_1 = DUT_ENDPOINT_SRV_CURRENT_LEVEL_INITIAL_VALUE;
static zb_uint16_t attr_remaining_time_1 = ZB_ZCL_LEVEL_CONTROL_REMAINING_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_LEVEL_CONTROL_ATTRIB_LIST(rtp_zcl_20_dut_zc_level_control_attr_list,
        &attr_current_level_1, &attr_remaining_time_1);

/********************* Declare device **************************/
DECLARE_DUT_CLUSTER_LIST_1(rtp_zcl_20_dut_zc_device_clusters_1,
                           rtp_zcl_20_dut_zc_basic_attr_list_1,
                           rtp_zcl_20_dut_zc_identify_attr_list_1,
                           rtp_zcl_20_dut_zc_on_off_attr_list_1,
                           rtp_zcl_20_dut_zc_level_control_attr_list);

DECLARE_DUT_EP_1(rtp_zcl_20_dut_zc_device_ep_1,
                 DUT_ENDPOINT_SRV,
                 rtp_zcl_20_dut_zc_device_clusters_1);

/******************* Endpoint 2 ************************/

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version_2  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source_2 = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(rtp_zcl_20_dut_zc_basic_attr_list_2, &attr_zcl_version_2, &attr_power_source_2);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time_2 = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(rtp_zcl_20_dut_zc_identify_attr_list_2, &attr_identify_time_2);

DECLARE_DUT_CLUSTER_LIST_2(rtp_zcl_20_dut_zc_device_clusters_2,
                           rtp_zcl_20_dut_zc_basic_attr_list_2,
                           rtp_zcl_20_dut_zc_identify_attr_list_2);

DECLARE_DUT_EP_2(rtp_zcl_20_dut_zc_device_ep_2,
                 DUT_ENDPOINT_CLI_1,
                 rtp_zcl_20_dut_zc_device_clusters_2);

/******************* Endpoint 3 ************************/

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version_3  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source_3 = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(rtp_zcl_20_dut_zc_basic_attr_list_3, &attr_zcl_version_3, &attr_power_source_3);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time_3 = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(rtp_zcl_20_dut_zc_identify_attr_list_3, &attr_identify_time_3);

DECLARE_DUT_CLUSTER_LIST_3(rtp_zcl_20_dut_zc_device_clusters_3,
                           rtp_zcl_20_dut_zc_basic_attr_list_3,
                           rtp_zcl_20_dut_zc_identify_attr_list_3);

DECLARE_DUT_EP_3(rtp_zcl_20_dut_zc_device_ep_3,
                 DUT_ENDPOINT_CLI_2,
                 rtp_zcl_20_dut_zc_device_clusters_3);

/********************* Declare device **************************/
DECLARE_DUT_CTX(rtp_zcl_20_dut_zc1_device_ctx,
                rtp_zcl_20_dut_zc_device_ep_1,
                rtp_zcl_20_dut_zc_device_ep_2,
                rtp_zcl_20_dut_zc_device_ep_3);

/*************************************************************************/

/*******************Definitions for Test***************************/

static zb_uint8_t zcl_endpoint_cb(zb_uint8_t param);


static void configure_default_reporting(zb_uint8_t param);
static void perform_binding(zb_uint8_t param);

static void trigger_reporting(zb_uint8_t param);

typedef struct test_binding_req_params_s
{
    zb_ieee_addr_t src_addr;
    zb_uint16_t src_ep_id;

    zb_ieee_addr_t dst_addr;
    zb_uint16_t dst_ep_id;

    zb_uint16_t cluster_id;
} test_binding_req_params_t;

static zb_uint8_t g_binding_req_index = 0;

static test_binding_req_params_t g_test_binding_req_params[] =
{
    {
        IEEE_ADDR_DUT_ZC,
        DUT_ENDPOINT_SRV,
        IEEE_ADDR_DUT_ZC,
        DUT_ENDPOINT_CLI_1,
        ZB_ZCL_CLUSTER_ID_ON_OFF
    },
    {
        IEEE_ADDR_DUT_ZC,
        DUT_ENDPOINT_SRV,
        IEEE_ADDR_DUT_ZC,
        DUT_ENDPOINT_CLI_1,
        ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL
    },
    {
        IEEE_ADDR_DUT_ZC,
        DUT_ENDPOINT_SRV,
        IEEE_ADDR_DUT_ZC,
        DUT_ENDPOINT_CLI_2,
        ZB_ZCL_CLUSTER_ID_ON_OFF
    },
    {
        IEEE_ADDR_DUT_ZC,
        DUT_ENDPOINT_SRV,
        IEEE_ADDR_DUT_ZC,
        DUT_ENDPOINT_CLI_2,
        ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL
    },
    {
        IEEE_ADDR_DUT_ZC,
        DUT_ENDPOINT_SRV,
        IEEE_ADDR_TH_ZR,
        TH_ENDPOINT_CLI,
        ZB_ZCL_CLUSTER_ID_ON_OFF
    },
    {
        IEEE_ADDR_DUT_ZC,
        DUT_ENDPOINT_SRV,
        IEEE_ADDR_TH_ZR,
        TH_ENDPOINT_CLI,
        ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL
    },
};

MAIN()
{
    ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
    ZB_SET_TRACE_LEVEL(4);
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_dut_zc");


    zb_set_long_address(g_ieee_addr_dut);

    zb_reg_test_set_common_channel_settings();
    zb_set_network_coordinator_role((1l << TEST_CHANNEL));
    zb_secur_setup_nwk_key(g_nwk_key, 0);

    zb_set_nvram_erase_at_start(ZB_TRUE);

    ZB_AF_REGISTER_DEVICE_CTX(&rtp_zcl_20_dut_zc1_device_ctx);

    ZB_AF_SET_ENDPOINT_HANDLER(DUT_ENDPOINT_CLI_1, zcl_endpoint_cb);
    ZB_AF_SET_ENDPOINT_HANDLER(DUT_ENDPOINT_CLI_2, zcl_endpoint_cb);

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

    case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
        TRACE_MSG(TRACE_APP1, "signal: ZB_ZDO_SIGNAL_DEVICE_ANNCE, status %d", (FMT__D, status));
        if (status == 0)
        {
            test_step_register(configure_default_reporting, 0, RTP_ZCL_20_STEP_1_TIME_ZC);
            test_step_register(perform_binding, 0, RTP_ZCL_20_STEP_2_TIME_ZC);

            test_step_register(trigger_reporting, 0, RTP_ZCL_20_STEP_3_TIME_ZC);

            test_control_start(TEST_MODE, RTP_ZCL_20_STEP_1_DELAY_ZC);
        }
        break; /* ZB_ZDO_SIGNAL_DEVICE_ANNCE */

    default:
        TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}


static void zcl_process_attribute_reporting(zb_uint8_t param)
{
    zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);
    zb_zcl_report_attr_req_t *rep_attr_req;

    zb_uint8_t src_ep = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint;
    zb_uint8_t dst_ep = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).dst_endpoint;

    TRACE_MSG(TRACE_APP1, ">> zcl_process_attribute_reporting", (FMT__0));

    do
    {
        ZB_ZCL_GENERAL_GET_NEXT_REPORT_ATTR_REQ(param, rep_attr_req);

        if (rep_attr_req)
        {
            TRACE_MSG(TRACE_APP1, "Report command is received, src_ep %hd, dst_ep %hd, cluster_id %d, attr_id %d",
                      (FMT__H_H_D_D,
                       src_ep,
                       dst_ep,
                       cmd_info->cluster_id,
                       rep_attr_req->attr_id));

            ZB_ASSERT(src_ep == DUT_ENDPOINT_SRV);
            ZB_ASSERT(dst_ep == DUT_ENDPOINT_CLI_1 || dst_ep == DUT_ENDPOINT_CLI_2);

            if (cmd_info->cluster_id == ZB_ZCL_CLUSTER_ID_ON_OFF &&
                    rep_attr_req->attr_id == ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID)
            {
                zb_uint8_t on_off_attr_value = rep_attr_req->attr_value[0];

                TRACE_MSG(TRACE_APP1, "On/Off report is received, dst_ep %hd, new_value %hd",
                          (FMT__H_H, dst_ep, on_off_attr_value));
            }
            else if (cmd_info->cluster_id == ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL &&
                     rep_attr_req->attr_id == ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID)
            {
                zb_uint8_t current_level_attr_value = rep_attr_req->attr_value[0];

                TRACE_MSG(TRACE_APP1, "Current Level report is received, dst_ep %hd, new_value %hd",
                          (FMT__H_H, dst_ep, current_level_attr_value));
            }
        }
    } while (rep_attr_req);

    TRACE_MSG(TRACE_APP1, "<< zcl_process_attribute_reporting", (FMT__0));
}


static zb_uint8_t zcl_endpoint_cb(zb_uint8_t param)
{
    zb_bufid_t zcl_cmd_buf = param;
    zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);
    zb_uint8_t cmd_processed = ZB_FALSE;

    ZVUNUSED(zcl_cmd_buf);

    TRACE_MSG(TRACE_APP1, ">> zcl_endpoint_cb", (FMT__0));

    if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI)
    {
        if (cmd_info->is_common_command)
        {
            switch (cmd_info->cmd_id)
            {
            case ZB_ZCL_CMD_REPORT_ATTRIB:
                zcl_process_attribute_reporting(param);
                break;

            default:
                break;
            }
        }
    }

    TRACE_MSG(TRACE_APP1, "<< zcl_endpoint_cb, ret %hd", (FMT__H, cmd_processed));

    return cmd_processed;
}


void configure_default_reporting(zb_uint8_t unused)
{
    zb_zcl_reporting_info_t rep_info;
    zb_ret_t status;
    ZVUNUSED(unused);

    TRACE_MSG(TRACE_APP1, ">> configure_default_reporting", (FMT__0));

    ZB_BZERO(&rep_info, sizeof(rep_info));

    /* Common settings */
    rep_info.direction = ZB_ZCL_CONFIGURE_REPORTING_SEND_REPORT;
    rep_info.ep = DUT_ENDPOINT_SRV;
    rep_info.cluster_role = ZB_ZCL_CLUSTER_SERVER_ROLE;
    rep_info.dst.profile_id = ZB_AF_HA_PROFILE_ID;

    rep_info.u.send_info.min_interval = DUT_ENDPOINT_SRV_ON_OFF_REPORTING_MIN_INTERVAL;
    rep_info.u.send_info.max_interval = DUT_ENDPOINT_SRV_ON_OFF_REPORTING_MAX_INTERVAL;
    rep_info.u.send_info.def_min_interval = DUT_ENDPOINT_SRV_ON_OFF_REPORTING_MIN_INTERVAL;
    rep_info.u.send_info.def_max_interval = DUT_ENDPOINT_SRV_ON_OFF_REPORTING_MAX_INTERVAL;

    /* On/Off attribute of On/Off cluster */

    rep_info.cluster_id = ZB_ZCL_CLUSTER_ID_ON_OFF;
    rep_info.attr_id = ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID;

    rep_info.u.send_info.delta.u8 = DUT_ENDPOINT_SRV_CURRENT_LEVEL_REPORTING_DELTA;

    status = zb_zcl_put_reporting_info(&rep_info, ZB_TRUE);
    ZB_ASSERT(status == RET_OK);

    /* Current Level attribute of Level Control cluster */

    rep_info.cluster_id = ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL;
    rep_info.attr_id = ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID;

    rep_info.u.send_info.delta.u8 = 1;

    status = zb_zcl_put_reporting_info(&rep_info, ZB_TRUE);
    ZB_ASSERT(status == RET_OK);

    TRACE_MSG(TRACE_APP1, "<< configure_default_reporting", (FMT__0));
}


static void perform_binding_cb(zb_uint8_t param)
{
    zb_zdo_bind_resp_t *bind_resp = (zb_zdo_bind_resp_t *)zb_buf_begin(param);

    TRACE_MSG(TRACE_APP1, ">> perform_binding_cb, status %hd", (FMT__H, bind_resp->status));

    ZB_ASSERT(bind_resp->status == ZB_ZDP_STATUS_SUCCESS);

    if (g_binding_req_index < ZB_ARRAY_SIZE(g_test_binding_req_params))
    {
        ZB_SCHEDULE_CALLBACK(perform_binding, param);
    }
    else
    {
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_APP1, "<< perform_binding_cb", (FMT__0));
}


static void perform_binding(zb_uint8_t param)
{
    zb_zdo_bind_req_param_t *request;
    test_binding_req_params_t *binding_params;

    if (param == 0)
    {
        zb_buf_get_out_delayed(perform_binding);
        return;
    }

    TRACE_MSG(TRACE_APP1, ">> perform_binding", (FMT__0));

    request = ZB_BUF_GET_PARAM(param, zb_zdo_bind_req_param_t);
    binding_params = &g_test_binding_req_params[g_binding_req_index];

    ZB_IEEE_ADDR_COPY(request->dst_address.addr_long, binding_params->dst_addr);
    ZB_IEEE_ADDR_COPY(request->src_address, binding_params->src_addr);

    request->req_dst_addr = zb_address_short_by_ieee(binding_params->src_addr);

    request->src_endp = binding_params->src_ep_id;
    request->cluster_id = binding_params->cluster_id;
    request->dst_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
    request->dst_endp = binding_params->dst_ep_id;

    g_binding_req_index++;

    zb_zdo_bind_req(param, perform_binding_cb);

    TRACE_MSG(TRACE_APP1, "<< perform_binding", (FMT__0));
}


static void trigger_reporting(zb_uint8_t param)
{
    zb_zcl_status_t zcl_status;

    zb_uint8_t opposite_on_off_value;
    zb_uint8_t new_current_level_value;

    ZVUNUSED(param);

    TRACE_MSG(TRACE_APP1, ">> trigger_reporting", (FMT__0));

    /* On/Off attribute of On/Off cluster */
    opposite_on_off_value = !attr_on_off_1;

    zcl_status = zb_zcl_set_attr_val(DUT_ENDPOINT_SRV,
                                     ZB_ZCL_CLUSTER_ID_ON_OFF,
                                     ZB_ZCL_CLUSTER_SERVER_ROLE,
                                     ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID,
                                     (zb_uint8_t *)&opposite_on_off_value,
                                     ZB_FALSE);

    ZB_ASSERT(zcl_status == ZB_ZCL_STATUS_SUCCESS);

    /* Current Level attribute of Level Control cluster */

    new_current_level_value = attr_current_level_1 + 1;

    zcl_status = zb_zcl_set_attr_val(DUT_ENDPOINT_SRV,
                                     ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL,
                                     ZB_ZCL_CLUSTER_SERVER_ROLE,
                                     ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID,
                                     (zb_uint8_t *)&new_current_level_value,
                                     ZB_FALSE);

    ZB_ASSERT(zcl_status == ZB_ZCL_STATUS_SUCCESS);

    TRACE_MSG(TRACE_APP1, "<< trigger_reporting", (FMT__0));
}

/*! @} */
