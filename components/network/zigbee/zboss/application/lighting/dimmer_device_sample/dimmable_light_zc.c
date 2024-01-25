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
/* PURPOSE: Dimmable Light
*/

#define ZB_TRACE_FILE_ID 40189
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_zcl.h"

#include "zb_ha.h"
#include "ha/custom/zb_ha_custom_dimmable_light.h"

#include "test_config.h"
#include "test_output_dev.h"

#if defined ZB_ENABLE_HA


#if ! defined ZB_COORDINATOR_ROLE
#error define ZB_COORDINATOR_ROLE to compile ze tests
#endif

#if ! (defined ZB_ZCL_SUPPORT_CLUSTER_LEVEL_CONTROL && defined ZB_ZCL_SUPPORT_CLUSTER_COLOR_CONTROL)
#error enable Level Control and Color Control to compile
#endif

#define HA_DIMMABLE_LIGHT_ENDPOINT DUT_ENDPOINT

/* Handler for specific zcl commands */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);
void test_device_cb(zb_uint8_t param) ZB_CALLBACK;

zb_ieee_addr_t g_zc_addr = ZC_IEEE_ADDR;

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
zb_uint8_t g_attr_zcl_version  = ZB_ZCL_VERSION;
zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list, &g_attr_zcl_version, &g_attr_power_source);

/* Identify cluster attributes data */
zb_uint16_t g_attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_attr_identify_time);

/* Groups cluster attributes data */
zb_uint8_t g_attr_name_support = 0;

ZB_ZCL_DECLARE_GROUPS_ATTRIB_LIST(groups_attr_list, &g_attr_name_support);

/* Scenes cluster attribute data */
zb_uint8_t g_attr_scenes_scene_count;
zb_uint8_t g_attr_scenes_current_scene;
zb_uint16_t g_attr_scenes_current_group;
zb_uint8_t g_attr_scenes_scene_valid;
zb_uint8_t g_attr_scenes_name_support;

ZB_ZCL_DECLARE_SCENES_ATTRIB_LIST(scenes_attr_list, &g_attr_scenes_scene_count,
    &g_attr_scenes_current_scene, &g_attr_scenes_current_group,
    &g_attr_scenes_scene_valid, &g_attr_scenes_name_support);


/* On/Off cluster attributes data */
zb_uint8_t g_attr_on_off  = ZB_FALSE;

ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(on_off_attr_list, &g_attr_on_off);

/* Level Control cluster attribute variables */
zb_uint8_t g_current_level = ZB_ZCL_LEVEL_CONTROL_CURRENT_LEVEL_DEFAULT_VALUE;
zb_uint16_t g_remaining_time = ZB_ZCL_LEVEL_CONTROL_REMAINING_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_LEVEL_CONTROL_ATTRIB_LIST(level_control_attr_list, &g_current_level, &g_remaining_time);

/* Color Control cluster attribute variables */
zb_uint8_t g_color_control_current_hue = ZB_ZCL_COLOR_CONTROL_CURRENT_HUE_MIN_VALUE;
zb_uint8_t g_color_control_current_saturation = ZB_ZCL_COLOR_CONTROL_CURRENT_SATURATION_MIN_VALUE;
zb_uint16_t g_color_control_remaining_time = ZB_ZCL_COLOR_CONTROL_REMAINING_TIME_MIN_VALUE;
zb_uint16_t g_color_control_current_X = ZB_ZCL_COLOR_CONTROL_CURRENT_X_DEF_VALUE;
zb_uint16_t g_color_control_current_Y = ZB_ZCL_COLOR_CONTROL_CURRENT_Y_DEF_VALUE;
zb_uint16_t g_color_control_color_temperature = ZB_ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_DEF_VALUE;
zb_uint8_t g_color_control_color_mode = ZB_ZCL_COLOR_CONTROL_COLOR_MODE_HUE_SATURATION;

zb_uint8_t g_color_control_number_primaries = ZB_ZCL_COLOR_CONTROL_NUMBER_OF_PRIMARIES_MAX_VALUE;
zb_uint16_t g_color_control_primary_1_X = 0;
zb_uint16_t g_color_control_primary_1_Y = 0;
zb_uint8_t g_color_control_primary_1_intensity = 1;
zb_uint16_t g_color_control_primary_2_X = 0;
zb_uint16_t g_color_control_primary_2_Y = 0;
zb_uint8_t g_color_control_primary_2_intensity = 2;
zb_uint16_t g_color_control_primary_3_X = 0;
zb_uint16_t g_color_control_primary_3_Y = 0;
zb_uint8_t g_color_control_primary_3_intensity = 3;
zb_uint16_t g_color_control_primary_4_X = 0;
zb_uint16_t g_color_control_primary_4_Y = 0;
zb_uint8_t g_color_control_primary_4_intensity = 4;
zb_uint16_t g_color_control_primary_5_X = 0;
zb_uint16_t g_color_control_primary_5_Y = 0;
zb_uint8_t g_color_control_primary_5_intensity = 5;
zb_uint16_t g_color_control_primary_6_X = 0;
zb_uint16_t g_color_control_primary_6_Y = 0;
zb_uint8_t g_color_control_primary_6_intensity = 6;

ZB_ZCL_DECLARE_COLOR_CONTROL_ATTRIB_LIST_EXT(color_control_attr_list,
    &g_color_control_current_hue, &g_color_control_current_saturation, &g_color_control_remaining_time,
    &g_color_control_current_X, &g_color_control_current_Y,
    &g_color_control_color_temperature, &g_color_control_color_mode,
    &g_color_control_number_primaries,
    &g_color_control_primary_1_X, &g_color_control_primary_1_Y, &g_color_control_primary_1_intensity,
    &g_color_control_primary_2_X, &g_color_control_primary_2_Y, &g_color_control_primary_2_intensity,
    &g_color_control_primary_3_X, &g_color_control_primary_3_Y, &g_color_control_primary_3_intensity,
    &g_color_control_primary_4_X, &g_color_control_primary_4_Y, &g_color_control_primary_4_intensity,
    &g_color_control_primary_5_X, &g_color_control_primary_5_Y, &g_color_control_primary_5_intensity,
    &g_color_control_primary_6_X, &g_color_control_primary_6_Y, &g_color_control_primary_6_intensity);

/********************* Declare device **************************/

ZB_HA_DECLARE_CUSTOM_DIMMABLE_LIGHT_CLUSTER_LIST(
  dimmable_light_clusters, basic_attr_list,
  identify_attr_list,
  scenes_attr_list,
  groups_attr_list,
  on_off_attr_list,
  level_control_attr_list,
  color_control_attr_list);

ZB_HA_DECLARE_CUSTOM_DIMMABLE_LIGHT_EP(
  dimmable_light_ep, HA_DIMMABLE_LIGHT_ENDPOINT,
  dimmable_light_clusters);

ZB_HA_DECLARE_CUSTOM_DIMMABLE_LIGHT_CTX(
  dimmable_light_ctx,
  dimmable_light_ep);

MAIN()
{
  ARGV_UNUSED;

#if !(defined KEIL || defined SDCC || defined ZB_IAR)
#endif

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("dimmer_switch_dut");


  zb_set_default_ed_descriptor_values();
  //ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_TRUE_U;
  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_zc_addr);

  ZB_PIBCACHE_PAN_ID() = TEST_PAN_ID;

  ZB_ZCL_REGISTER_DEVICE_CB(test_device_cb);

  ZB_AIB().aps_designated_coordinator = 1;
  ZB_AIB().aps_channel_mask = (1L << 17);
  /****************** Register Device ********************************/
  ZB_AF_REGISTER_DEVICE_CTX(&dimmable_light_ctx);
  ZB_AF_SET_ENDPOINT_HANDLER(HA_DIMMABLE_LIGHT_ENDPOINT,
                             zcl_specific_cluster_cmd_handler);

  ZB_SET_NIB_SECURITY_LEVEL(0);

  if (zdo_dev_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ZCL3, "zdo_dev_start failed", (FMT__0));
  }
  else
  {
    zcl_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}

zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
  zb_buf_t *zcl_cmd_buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
  zb_zcl_parsed_hdr_t *cmd_info = ZB_GET_BUF_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);
  zb_bool_t cmd_processed = ZB_FALSE;

  TRACE_MSG(TRACE_ZCL1, "> zcl_specific_cluster_cmd_handler %i", (FMT__H, param));
  TRACE_MSG(TRACE_ZCL3, "payload size: %i", (FMT__D, ZB_BUF_LEN(zcl_cmd_buf)));

  if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI)
  {
    TRACE_MSG(TRACE_ZCL1,
              "CLNT role cluster 0x%d is not supported",
              (FMT__D, cmd_info->cluster_id));
  }
  else
  {
    // Command from client to server ZB_ZCL_FRAME_DIRECTION_TO_SRV
    if ((cmd_info->cluster_id == ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL)
         || (cmd_info->cluster_id == ZB_ZCL_CLUSTER_ID_COLOR_CONTROL))
    {
        if (cmd_info->is_common_command)
        {
          switch (cmd_info->cmd_id)
          {
            default:
              TRACE_MSG(TRACE_ZCL2, "Skip general command %hd", (FMT__H, cmd_info->cmd_id));
              break;
          }
        }
        else
        {
          TRACE_MSG(TRACE_ZCL2, "Cluster command %hd, skip it", (FMT__H, cmd_info->cmd_id));
        }
    }
    else
    {
        TRACE_MSG(TRACE_ZCL1,
                  "SRV role, cluster 0x%d is not supported or app processing is not needed",
                  (FMT__D, cmd_info->cluster_id));
    }
  }

  TRACE_MSG(TRACE_ZCL1, "< zcl_specific_cluster_cmd_handler %hd", (FMT__H, cmd_processed));
  return cmd_processed;
}

zb_ret_t level_control_set_level(zb_uint8_t new_level)
{
  TRACE_MSG(TRACE_ZCL1, "> level_control_set_level", (FMT__0));
  g_current_level = new_level;

  TRACE_MSG(TRACE_ZCL1, "New level is %i", (FMT__H, new_level));
  TRACE_MSG(TRACE_ZCL1, "< level_control_set_level", (FMT__0));
  return RET_OK;
}

void test_device_cb(zb_uint8_t param) ZB_CALLBACK
{
  zb_buf_t* buffer = ZB_BUF_FROM_REF(param);
  zb_zcl_device_callback_param_t *device_cb_param =
    ZB_GET_BUF_PARAM(buffer, zb_zcl_device_callback_param_t);
  switch (device_cb_param->device_cb_id)
  {
    case ZB_ZCL_LEVEL_CONTROL_SET_VALUE_CB_ID:
      device_cb_param->status =
        level_control_set_level(device_cb_param->cb_param.level_control_set_value_param.new_value);
      break;

    case ZB_ZCL_SET_ATTR_VALUE_CB_ID:
      if (device_cb_param->cb_param.set_attr_value_param.cluster_id == ZB_ZCL_CLUSTER_ID_ON_OFF &&
          device_cb_param->cb_param.set_attr_value_param.attr_id == ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID)
      {
        g_attr_on_off = device_cb_param->cb_param.set_attr_value_param.values.data8;
        TRACE_MSG(TRACE_ZCL1, "on/off setting: %hd", (FMT__H, device_cb_param->cb_param.set_attr_value_param.values.data8));
        device_cb_param->status = RET_OK;
      }
      else if (device_cb_param->cb_param.set_attr_value_param.cluster_id == ZB_ZCL_CLUSTER_ID_COLOR_CONTROL)
      {
        /* Color control value update - check attr_id, send value to device h/w etc. */
        /* ZCL attr value will be updated automatically, it is not needed to do it here. */
        device_cb_param->status = RET_OK;
      }
      else
      {
        TRACE_MSG(TRACE_ZCL1, "ZB_ZCL_SET_ATTR_VALUE_CB_ID: cluster_id %d", (FMT__D, device_cb_param->cb_param.set_attr_value_param.cluster_id));
        device_cb_param->status = RET_ERROR;
      }
      break;

    default:
      device_cb_param->status = RET_ERROR;
      break;
  }
}

void zb_zdo_startup_complete(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);

  TRACE_MSG(TRACE_ZCL1, "> zb_zdo_startup_complete %hd", (FMT__H, param));

  if (buf->u.hdr.status == 0)
  {
    TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));
  }
  else
  {
    TRACE_MSG(
        TRACE_ERROR,
        "Device started FAILED status %d",
        (FMT__D, (int)buf->u.hdr.status));
    zb_free_buf(buf);
  }
  TRACE_MSG(TRACE_ZCL1, "< zb_zdo_startup_complete", (FMT__0));
}

#else // defined ZB_ENABLE_HA

#include <stdio.h>
int main()
{
  printf(" HA is not supported\n");
  return 0;
}

#endif // defined ZB_ENABLE_HA
