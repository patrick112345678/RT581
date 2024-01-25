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
/* PURPOSE: DUT ZR
*/

#define ZB_TEST_NAME RTP_ZCL_12_DUT_ZR

#define ZB_TRACE_FILE_ID 40429
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
#include "rtp_zcl_12_common.h"
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
static zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT_ZR;

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(rtp_zcl_12_dut_zr_basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(rtp_zcl_12_dut_zr_identify_attr_list, &attr_identify_time);

/* Groups cluster attributes data */
static zb_uint8_t attr_groups_name_support = ZB_ZCL_ATTR_GROUPS_NAME_NOT_SUPPORTED;

ZB_ZCL_DECLARE_GROUPS_ATTRIB_LIST(rtp_zcl_12_dut_zr_groups_attr_list, &attr_groups_name_support);

/* Identify cluster attributes data */
static zb_bool_t attr_on_off = (zb_bool_t)ZB_ZCL_ON_OFF_IS_ON;
static zb_bool_t global_scene_ctrl = ZB_ZCL_ON_OFF_GLOBAL_SCENE_CONTROL_DEFAULT_VALUE;
static zb_uint16_t on_time = ZB_ZCL_ON_OFF_ON_TIME_DEFAULT_VALUE;
static zb_uint16_t off_wait_time = ZB_ZCL_ON_OFF_OFF_WAIT_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST_EXT(rtp_zcl_12_dut_zr_on_off_attr_list,
                                      &attr_on_off,
                                      &global_scene_ctrl,
                                      &on_time,
                                      &off_wait_time);

/* Color Control cluster attributes data */
static zb_uint8_t g_color_control_current_hue = ZB_ZCL_COLOR_CONTROL_CURRENT_HUE_MIN_VALUE;
static zb_uint8_t g_color_control_current_saturation = ZB_ZCL_COLOR_CONTROL_CURRENT_SATURATION_MIN_VALUE;
static zb_uint16_t g_color_control_remaining_time = ZB_ZCL_COLOR_CONTROL_REMAINING_TIME_MIN_VALUE;
static zb_uint16_t g_color_control_current_X = ZB_ZCL_COLOR_CONTROL_CURRENT_X_DEF_VALUE;
static zb_uint16_t g_color_control_current_Y = ZB_ZCL_COLOR_CONTROL_CURRENT_Y_DEF_VALUE;
static zb_uint16_t g_color_control_color_temperature = ZB_ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_DEF_VALUE;
static zb_uint8_t g_color_control_color_mode = ZB_ZCL_COLOR_CONTROL_COLOR_MODE_HUE_SATURATION;

static zb_uint8_t g_color_control_options = ZB_ZCL_COLOR_CONTROL_OPTIONS_DEFAULT_VALUE;
static zb_uint8_t g_color_control_number_primaries = ZB_ZCL_COLOR_CONTROL_NUMBER_OF_PRIMARIES_MAX_VALUE;
static zb_uint16_t g_color_control_primary_1_X = 0;
static zb_uint16_t g_color_control_primary_1_Y = 0;
static zb_uint8_t g_color_control_primary_1_intensity = 1;
static zb_uint16_t g_color_control_primary_2_X = 0;
static zb_uint16_t g_color_control_primary_2_Y = 0;
static zb_uint8_t g_color_control_primary_2_intensity = 2;
static zb_uint16_t g_color_control_primary_3_X = 0;
static zb_uint16_t g_color_control_primary_3_Y = 0;
static zb_uint8_t g_color_control_primary_3_intensity = 3;
static zb_uint16_t g_color_control_primary_4_X = 0;
static zb_uint16_t g_color_control_primary_4_Y = 0;
static zb_uint8_t g_color_control_primary_4_intensity = 4;
static zb_uint16_t g_color_control_primary_5_X = 0;
static zb_uint16_t g_color_control_primary_5_Y = 0;
static zb_uint8_t g_color_control_primary_5_intensity = 5;
static zb_uint16_t g_color_control_primary_6_X = 0;
static zb_uint16_t g_color_control_primary_6_Y = 0;
static zb_uint8_t g_color_control_primary_6_intensity = 6;

static zb_uint16_t g_color_control_enhanced_current_hue = 0;
static zb_uint8_t g_color_control_enhanced_color_mode = 0;
static zb_uint8_t g_color_control_color_loop_active = ZB_ZCL_CMD_COLOR_CONTROL_LOOP_ACTION_DEACTIVATE;
static zb_uint8_t g_color_control_color_loop_direction = ZB_ZCL_CMD_COLOR_CONTROL_LOOP_DIRECTION_INCREMENT;
static zb_uint16_t g_color_control_color_loop_time = ZB_ZCL_COLOR_CONTROL_COLOR_LOOP_TIME_DEF_VALUE;
static zb_uint16_t g_color_control_color_loop_start = ZB_ZCL_COLOR_CONTROL_COLOR_LOOP_START_DEF_VALUE;
static zb_uint16_t g_color_control_color_loop_stored = 0;
static zb_uint16_t g_color_control_color_capabilities = ZB_ZCL_COLOR_CONTROL_COLOR_CAPABILITIES_MAX_VALUE;
static zb_uint16_t g_color_control_color_temp_physical_min = ZB_ZCL_COLOR_CONTROL_COLOR_TEMP_PHYSICAL_MIN_MIREDS_DEFAULT_VALUE;
static zb_uint16_t g_color_control_color_temp_physical_max = ZB_ZCL_COLOR_CONTROL_COLOR_TEMP_PHYSICAL_MAX_MIREDS_DEFAULT_VALUE;
static zb_uint16_t g_color_control_couple_color_temp_to_level_min = ZB_ZCL_COLOR_CONTROL_COLOR_TEMP_PHYSICAL_MIN_MIREDS_DEFAULT_VALUE;
static zb_uint16_t g_color_control_start_up_color_temp = ZB_ZCL_COLOR_CONTROL_COLOR_TEMP_PHYSICAL_MAX_MIREDS_DEFAULT_VALUE;

ZB_ZCL_DECLARE_COLOR_CONTROL_ATTRIB_LIST_EXT(rtp_zcl_12_dut_zr_color_control_attr_list,
    &g_color_control_current_hue, &g_color_control_current_saturation, &g_color_control_remaining_time,
    &g_color_control_current_X, &g_color_control_current_Y,
    &g_color_control_color_temperature, &g_color_control_color_mode,
    &g_color_control_options, &g_color_control_number_primaries,
    &g_color_control_primary_1_X, &g_color_control_primary_1_Y, &g_color_control_primary_1_intensity,
    &g_color_control_primary_2_X, &g_color_control_primary_2_Y, &g_color_control_primary_2_intensity,
    &g_color_control_primary_3_X, &g_color_control_primary_3_Y, &g_color_control_primary_3_intensity,
    &g_color_control_primary_4_X, &g_color_control_primary_4_Y, &g_color_control_primary_4_intensity,
    &g_color_control_primary_5_X, &g_color_control_primary_5_Y, &g_color_control_primary_5_intensity,
    &g_color_control_primary_6_X, &g_color_control_primary_6_Y, &g_color_control_primary_6_intensity,
    &g_color_control_enhanced_current_hue, &g_color_control_enhanced_color_mode,
    &g_color_control_color_loop_active, &g_color_control_color_loop_direction,
    &g_color_control_color_loop_time, &g_color_control_color_loop_start, &g_color_control_color_loop_stored,
    &g_color_control_color_capabilities,
    &g_color_control_color_temp_physical_min, &g_color_control_color_temp_physical_max,
    &g_color_control_couple_color_temp_to_level_min, &g_color_control_start_up_color_temp);

/********************* Declare device **************************/
DECLARE_DUT_CLUSTER_LIST(rtp_zcl_12_dut_zr_device_clusters,
                        rtp_zcl_12_dut_zr_basic_attr_list,
                        rtp_zcl_12_dut_zr_identify_attr_list,
                        rtp_zcl_12_dut_zr_groups_attr_list,
                        rtp_zcl_12_dut_zr_on_off_attr_list,
                        rtp_zcl_12_dut_zr_color_control_attr_list);

DECLARE_DUT_EP(rtp_zcl_12_dut_zr_device_ep,
              DUT_ENDPOINT,
              rtp_zcl_12_dut_zr_device_clusters);

DECLARE_DUT_CTX(rtp_zcl_12_dut_zr_device_ctx, rtp_zcl_12_dut_zr_device_ep);

static void trigger_fb_target(zb_uint8_t unused);

void test_zcl_device_cb(zb_uint8_t param);

/************************Main*************************************/
MAIN()
{
  ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
  ZB_SET_TRACE_LEVEL(4);
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_dut_zr");


  zb_set_long_address(g_ieee_addr_dut);

  zb_reg_test_set_common_channel_settings();
  zb_set_network_router_role((1l << TEST_CHANNEL));
  zb_set_nvram_erase_at_start(ZB_TRUE);


  zb_secur_setup_nwk_key(g_nwk_key, 0);

  ZB_AF_REGISTER_DEVICE_CTX(&rtp_zcl_12_dut_zr_device_ctx);
  ZB_ZCL_REGISTER_DEVICE_CB(test_zcl_device_cb);

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

  ZB_BDB().bdb_commissioning_time = DUT_FB_DURATION;
  zb_bdb_finding_binding_target(DUT_ENDPOINT);
}


void test_zcl_device_cb(zb_uint8_t param)
{
  zb_zcl_device_callback_param_t *device_cb_param = ZB_BUF_GET_PARAM(param, zb_zcl_device_callback_param_t);

  TRACE_MSG(TRACE_APP1, ">>test_device_cb param %hd id %hd", (FMT__H_H, param, device_cb_param->device_cb_id));

  device_cb_param->status = RET_OK;

  if (device_cb_param->device_cb_id == ZB_ZCL_SET_ATTR_VALUE_CB_ID)
  {
    zb_zcl_set_attr_value_param_t *set_attr_value = &(device_cb_param->cb_param.set_attr_value_param);

    TRACE_MSG(TRACE_APP1, "zcl_device_cb for set attr is called: clusterid=0x%x, attrid=0x%x",
      (FMT__D_D, set_attr_value->cluster_id, set_attr_value->attr_id));
  }

  TRACE_MSG(TRACE_APP1, "<<test_device_cb param", (FMT__0));
}

/*! @} */
