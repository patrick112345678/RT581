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
/* PURPOSE: TH ZR2
*/

#define ZB_TEST_NAME RTP_APS_10_TH_ZR2
#define ZB_TRACE_FILE_ID 40113

#define TH_DEVICE_ID   0xaa03

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
#include "rtp_aps_10_common.h"
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
static zb_ieee_addr_t g_ieee_addr_th_zr2  = IEEE_ADDR_TH_ZR2;

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(rtp_aps_10_th_zr2_basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(rtp_aps_10_th_zr2_identify_attr_list, &attr_identify_time);

/* Identify cluster attributes data */
static zb_bool_t attr_on_off = (zb_bool_t)ZB_ZCL_ON_OFF_IS_ON;
static zb_bool_t global_scene_ctrl = ZB_ZCL_ON_OFF_GLOBAL_SCENE_CONTROL_DEFAULT_VALUE;
static zb_uint16_t on_time = ZB_ZCL_ON_OFF_ON_TIME_DEFAULT_VALUE;
static zb_uint16_t off_wait_time = ZB_ZCL_ON_OFF_OFF_WAIT_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST_EXT(rtp_aps_10_th_zr2_on_off_attr_list,
                                      &attr_on_off,
                                      &global_scene_ctrl,
                                      &on_time,
                                      &off_wait_time);

/******** Custom cluster attributes data ********/
static zb_uint8_t g_attr_u8 = ZB_ZCL_CUSTOM_CLUSTER_ATTR_U8_DEFAULT_VALUE;
static zb_int16_t g_attr_s16 = ZB_ZCL_CUSTOM_CLUSTER_ATTR_S16_DEFAULT_VALUE;
static zb_uint24_t g_attr_24bit = ZB_ZCL_CUSTOM_CLUSTER_ATTR_24BIT_DEFAULT_VALUE;
static zb_uint32_t g_attr_32bitmap = ZB_ZCL_CUSTOM_CLUSTER_ATTR_32BITMAP_DEFAULT_VALUE;
static zb_ieee_addr_t g_attr_ieee = ZB_ZCL_CUSTOM_CLUSTER_ATTR_IEEE_DEFAULT_VALUE;
static zb_char_t g_attr_char_string[ZB_ZCL_CUSTOM_CLUSTER_ATTR_CHAR_STRING_MAX_SIZE] =
    ZB_ZCL_CUSTOM_CLUSTER_ATTR_CHAR_STRING_DEFAULT_VALUE;
static zb_time_t g_attr_utc_time = ZB_ZCL_CUSTOM_CLUSTER_ATTR_UTC_TIME_DEFAULT_VALUE;
static zb_uint8_t g_attr_byte_array[ZB_ZCL_CUSTOM_CLUSTER_ATTR_BYTE_ARRAY_MAX_SIZE] =
    ZB_ZCL_CUSTOM_CLUSTER_ATTR_BYTE_ARRAY_DEFAULT_VALUE;
static zb_bool_t g_attr_bool = ZB_ZCL_CUSTOM_CLUSTER_ATTR_BOOL_DEFAULT_VALUE;
static zb_uint8_t g_attr_128_bit_key[ZB_CCM_KEY_SIZE] = ZB_ZCL_CUSTOM_CLUSTER_ATTR_128_BIT_KEY_DEFAULT_VALUE;

ZB_ZCL_DECLARE_CUSTOM_ATTR_CLUSTER_ATTRIB_LIST(
    custom_attr_list2,
    &g_attr_u8,
    &g_attr_s16,
    &g_attr_24bit,
    &g_attr_32bitmap,
    g_attr_ieee,
    g_attr_char_string,
    &g_attr_utc_time,
    g_attr_byte_array,
    &g_attr_bool,
    g_attr_128_bit_key);

/********************* Declare device **************************/
DECLARE_TH_CLUSTER_LIST(rtp_aps_10_th_zr2_device_clusters,
                        rtp_aps_10_th_zr2_basic_attr_list,
                        rtp_aps_10_th_zr2_identify_attr_list,
                        custom_attr_list2);

DECLARE_TH_EP(rtp_aps_10_th_zr2_device_ep,
              TH_ENDPOINT,
              rtp_aps_10_th_zr2_device_clusters);

DECLARE_TH_CTX(rtp_aps_10_th_zr2_device_ctx, rtp_aps_10_th_zr2_device_ep);

static void trigger_fb_target(zb_uint8_t unused);

static zb_bool_t test_process_custom_specific_commands_srv(zb_uint8_t param);

/************************Main*************************************/
MAIN()
{
    ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
    ZB_SET_TRACE_LEVEL(4);
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_th_zr2");


    zb_set_long_address(g_ieee_addr_th_zr2);

    zb_reg_test_set_common_channel_settings();
    zb_set_network_router_role((1l << TEST_CHANNEL));
    zb_set_nvram_erase_at_start(ZB_TRUE);

    zb_secur_setup_nwk_key(g_nwk_key, 0);

    ZB_AF_REGISTER_DEVICE_CTX(&rtp_aps_10_th_zr2_device_ctx);

    zb_zcl_add_cluster_handlers(ZB_ZCL_CLUSTER_ID_CUSTOM,
                                ZB_ZCL_CLUSTER_SERVER_ROLE,
                                (zb_zcl_cluster_check_value_t)NULL,
                                (zb_zcl_cluster_write_attr_hook_t)NULL,
                                (zb_zcl_cluster_handler_t)test_process_custom_specific_commands_srv);

    zb_set_max_children(0);

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
            ZB_SCHEDULE_CALLBACK(trigger_fb_target, 0);
        }
        break; /* ZB_BDB_SIGNAL_STEERING */

    default:
        TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}

static void trigger_fb_target(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    ZB_BDB().bdb_commissioning_time = TH_FB_DURATION;
    zb_bdb_finding_binding_target(TH_ENDPOINT);
}

zb_bool_t test_process_custom_specific_commands_srv(zb_uint8_t param)
{
    zb_zcl_parsed_hdr_t cmd_info;
    zb_bool_t processed = ZB_FALSE;

    ZB_ZCL_COPY_PARSED_HEADER(param, &cmd_info);

    TRACE_MSG(TRACE_APP1, ">> test_process_custom_specific_commands_srv, "
              "param=%hd, cmd_id=%hd", (FMT__H_H, param, cmd_info.cmd_id));

    if (cmd_info.cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_SRV)
    {
        if (cmd_info.cmd_id == TEST_CUSTOM_CLUSTER_TEST_PAYLOAD_CMD_ID)
        {
            TRACE_MSG(TRACE_APP1, "Received test ZCL payload command", (FMT__0));
            processed = ZB_TRUE;
        }
    }

    if (cmd_info.disable_default_response && processed == ZB_TRUE)
    {
        TRACE_MSG(TRACE_APP1, "Default response disabled", (FMT__0));
        zb_buf_free(param);
    }
    else
    {
        ZB_ZCL_SEND_DEFAULT_RESP(param,
                                 ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).source.u.short_addr,
                                 ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                 ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).src_endpoint,
                                 ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).dst_endpoint,
                                 cmd_info.profile_id,
                                 ZB_ZCL_CLUSTER_ID_CUSTOM,
                                 cmd_info.seq_number,
                                 cmd_info.cmd_id,
                                 processed == ZB_TRUE ? ZB_ZCL_STATUS_SUCCESS : ZB_ZCL_STATUS_FAIL);
    }

    TRACE_MSG(TRACE_APP1, "<< test_process_custom_specific_commands_srv(ret=%hd)", (FMT__H, processed));

    return processed;
}

/*! @} */
