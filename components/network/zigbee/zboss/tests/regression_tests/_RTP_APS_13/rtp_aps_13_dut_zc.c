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

#define ZB_TEST_NAME RTP_APS_13_DUT_ZC
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
#include "rtp_aps_13_common.h"
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

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(rtp_aps_13_dut_zc_basic_attr_list_1, &attr_zcl_version_1, &attr_power_source_1);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time_1 = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(rtp_aps_13_dut_zc_identify_attr_list_1, &attr_identify_time_1);


/********************* Declare device **************************/
DECLARE_DUT_CLUSTER_LIST_1(rtp_aps_13_dut_zc_device_clusters_1,
                           rtp_aps_13_dut_zc_basic_attr_list_1,
                           rtp_aps_13_dut_zc_identify_attr_list_1);

DECLARE_DUT_EP_1(rtp_aps_13_dut_zc_device_ep_1,
                 DUT_ENDPOINT_SRV,
                 rtp_aps_13_dut_zc_device_clusters_1);

/******************* Endpoint 2 ************************/

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version_2  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source_2 = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(rtp_aps_13_dut_zc_basic_attr_list_2, &attr_zcl_version_2, &attr_power_source_2);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time_2 = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(rtp_aps_13_dut_zc_identify_attr_list_2, &attr_identify_time_2);

/* Groups cluster attributes data */
static zb_uint8_t attr_groups_name_support = ZB_ZCL_ATTR_GROUPS_NAME_NOT_SUPPORTED;

ZB_ZCL_DECLARE_GROUPS_ATTRIB_LIST(rtp_aps_13_dut_zc_groups_attr_list_2, &attr_groups_name_support);

/* On/Off cluster attributes data */
static zb_bool_t attr_on_off_2 = (zb_bool_t)ZB_ZCL_ON_OFF_IS_ON;
static zb_bool_t global_scene_ctrl_2 = ZB_ZCL_ON_OFF_GLOBAL_SCENE_CONTROL_DEFAULT_VALUE;
static zb_uint16_t on_time_2 = ZB_ZCL_ON_OFF_ON_TIME_DEFAULT_VALUE;
static zb_uint16_t off_wait_time_2 = ZB_ZCL_ON_OFF_OFF_WAIT_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST_EXT(rtp_aps_13_dut_zc_on_off_attr_list_2,
                                      &attr_on_off_2,
                                      &global_scene_ctrl_2,
                                      &on_time_2,
                                      &off_wait_time_2);

DECLARE_DUT_CLUSTER_LIST_2(rtp_aps_13_dut_zc_device_clusters_2,
                           rtp_aps_13_dut_zc_basic_attr_list_2,
                           rtp_aps_13_dut_zc_identify_attr_list_2,
                           rtp_aps_13_dut_zc_groups_attr_list_2,
                           rtp_aps_13_dut_zc_on_off_attr_list_2);

DECLARE_DUT_EP_2(rtp_aps_13_dut_zc_device_ep_2,
                 DUT_ENDPOINT_CLI_1,
                 rtp_aps_13_dut_zc_device_clusters_2);


/********************* Declare device **************************/
DECLARE_DUT_CTX(rtp_aps_13_dut_zc1_device_ctx,
                rtp_aps_13_dut_zc_device_ep_1,
                rtp_aps_13_dut_zc_device_ep_2);

/*************************************************************************/

/*******************Definitions for Test***************************/

static zb_uint8_t zcl_endpoint_cb(zb_uint8_t param);

static void send_add_group_req_delayed(zb_uint8_t unused);
static void send_add_group_req(zb_uint8_t param);

static void send_add_group_req_to_local_ep_delayed(zb_uint8_t unused);
static void send_add_group_req_to_local_ep(zb_uint8_t param);

static void send_on_off_toggle_req_delayed(zb_uint8_t unused);
static void send_on_off_toggle_req(zb_uint8_t param);

MAIN()
{
    ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP | TRACE_SUBSYSTEM_ZCL | TRACE_SUBSYSTEM_APS);
    ZB_SET_TRACE_LEVEL(4);
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("zdo_dut_zc");

    zb_set_long_address(g_ieee_addr_dut);

    zb_reg_test_set_common_channel_settings();
    zb_set_network_coordinator_role((1l << TEST_CHANNEL));
    zb_secur_setup_nwk_key(g_nwk_key, 0);

    zb_set_nvram_erase_at_start(ZB_TRUE);

    ZB_AF_REGISTER_DEVICE_CTX(&rtp_aps_13_dut_zc1_device_ctx);

    ZB_AF_SET_ENDPOINT_HANDLER(DUT_ENDPOINT_CLI_1, zcl_endpoint_cb);

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
            test_step_register(send_add_group_req_delayed, 0, RTP_APS_13_STEP_1_TIME_ZC);
            test_step_register(send_add_group_req_to_local_ep_delayed, 0, RTP_APS_13_STEP_2_TIME_ZC);
            test_step_register(send_on_off_toggle_req_delayed, 0, RTP_APS_13_STEP_3_TIME_ZC);

            test_control_start(TEST_MODE, RTP_APS_13_STEP_1_DELAY_ZC);
        }
        break; /* ZB_ZDO_SIGNAL_DEVICE_ANNCE */

    default:
        TRACE_MSG(TRACE_APP1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}

static zb_uint8_t zcl_endpoint_cb(zb_uint8_t param)
{
    zb_bufid_t zcl_cmd_buf = param;
    zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);
    zb_uint8_t cmd_processed = ZB_FALSE;

    ZVUNUSED(zcl_cmd_buf);

    TRACE_MSG(TRACE_APP1, ">> zcl_endpoint_cb", (FMT__0));

    if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_SRV)
    {
        if (cmd_info->cluster_id == ZB_ZCL_CLUSTER_ID_ON_OFF && cmd_info->cmd_id == ZB_ZCL_CMD_ON_OFF_TOGGLE_ID)
        {
            TRACE_MSG(TRACE_APP1, "On/off Toggle command is recieved", (FMT__0));
        }
        else if (cmd_info->cluster_id == ZB_ZCL_CLUSTER_ID_GROUPS
                 && cmd_info->cmd_id == ZB_ZCL_CMD_GROUPS_ADD_GROUP)
        {
            TRACE_MSG(TRACE_APP1, "Groups Add Group command is recieved", (FMT__0));
        }
        else
        {
            TRACE_MSG(TRACE_APP1, "Unexpected ZCL command is recieved: cmd_info.cluster_id = %d, cmd_info.cmd_id = %hd ", (FMT__D_H, cmd_info->cluster_id, cmd_info->cmd_id));
        }
    }

    TRACE_MSG(TRACE_APP1, "<< zcl_endpoint_cb, ret %hd", (FMT__H, cmd_processed));

    return cmd_processed;
}

static void send_add_group_req_delayed(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    zb_buf_get_out_delayed(send_add_group_req);
}

static void send_add_group_req(zb_uint8_t param)
{
    zb_ieee_addr_t addr = IEEE_ADDR_TH_ZR;

    ZB_ZCL_GROUPS_SEND_ADD_GROUP_REQ(param,
                                     addr,
                                     ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                                     TH_ENDPOINT_CLI,
                                     DUT_ENDPOINT_SRV,
                                     ZB_AF_HA_PROFILE_ID,
                                     ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                                     NULL,
                                     TEST_GROUP_ID);
}

static void send_add_group_req_to_local_ep_delayed(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    zb_buf_get_out_delayed(send_add_group_req_to_local_ep);
}

static void send_add_group_req_to_local_ep(zb_uint8_t param)
{
    zb_uint16_t addr = 0;

    ZB_ZCL_GROUPS_SEND_ADD_GROUP_REQ(param,
                                     addr,
                                     ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                     DUT_ENDPOINT_CLI_1,
                                     DUT_ENDPOINT_SRV,
                                     ZB_AF_HA_PROFILE_ID,
                                     ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                                     NULL,
                                     TEST_GROUP_ID);
}

static void send_on_off_toggle_req_delayed(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    zb_buf_get_out_delayed(send_on_off_toggle_req);
}

static void send_on_off_toggle_req(zb_uint8_t param)
{
    zb_uint16_t group_id = TEST_GROUP_ID;

    ZB_ZCL_ON_OFF_SEND_TOGGLE_REQ(param,
                                  group_id,
                                  ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT,
                                  0,
                                  DUT_ENDPOINT_SRV,
                                  ZB_AF_HA_PROFILE_ID,
                                  ZB_ZCL_DISABLE_DEFAULT_RESPONSE,
                                  NULL);
}

/*! @} */
