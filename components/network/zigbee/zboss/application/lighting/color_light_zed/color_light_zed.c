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

#define ZB_TRACE_FILE_ID 40187
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


#if ! defined ZB_ED_ROLE
#error define ZB_ED_ROLE to compile ze tests
#endif

#if ! (defined ZB_ZCL_SUPPORT_CLUSTER_LEVEL_CONTROL && defined ZB_ZCL_SUPPORT_CLUSTER_COLOR_CONTROL)
#error enable Level Control and Color Control to compile
#endif

#define HA_DIMMABLE_LIGHT_ENDPOINT DUT_ENDPOINT

/* Handler for specific zcl commands */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);
void test_device_cb(zb_uint8_t param) ZB_CALLBACK;
void zcl_reset_to_defaults_cb(zb_uint8_t param) ZB_CALLBACK;

zb_ieee_addr_t g_ed_addr = ED_IEEE_ADDR;

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
zb_uint8_t g_attr_zcl_version = ZB_ZCL_VERSION;
zb_uint8_t g_attr_app_version;
zb_uint8_t g_attr_stack_version;
zb_uint8_t g_attr_hw_version;
zb_char_t g_attr_mf_name[32];
zb_char_t g_attr_model_id[16];
zb_char_t g_attr_date_code[10];
zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;
zb_char_t g_attr_location_id[5];
zb_uint8_t g_attr_ph_env;
zb_char_t g_attr_sw_build_id[] = ZB_ZCL_BASIC_SW_BUILD_ID_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST_EXT(
  basic_attr_list,
  &g_attr_zcl_version,
  &g_attr_app_version,
  &g_attr_stack_version,
  &g_attr_hw_version,
  &g_attr_mf_name,
  &g_attr_model_id,
  &g_attr_date_code,
  &g_attr_power_source,
  &g_attr_location_id,
  &g_attr_ph_env,
  &g_attr_sw_build_id);

/* Identify cluster attributes data */
zb_uint16_t g_attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list,
  &g_attr_identify_time);

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

#ifdef ZB_ENABLE_ZLL
/* On/Off cluster attributes additions data */
zb_bool_t g_attr_global_scene_ctrl  = ZB_TRUE;
zb_uint16_t g_attr_on_time  = 0;
zb_uint16_t g_attr_off_wait_time  = 0;

ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST_ZLL(on_off_attr_list, &g_attr_on_off,
    &g_attr_global_scene_ctrl, &g_attr_on_time, &g_attr_off_wait_time);
#else
ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(on_off_attr_list, &g_attr_on_off);
#endif

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
  groups_attr_list,
  scenes_attr_list,
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

  ZB_INIT("color_light_zed");


#ifdef ZB_BDB_MODE
  ZB_BDB().bdb_mode = ZB_TRUE;
#endif

  ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ED;
  ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_TRUE_U;
  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ed_addr);

  zb_set_default_ed_descriptor_values();
  /****************** Register Device ********************************/
  ZB_AF_REGISTER_DEVICE_CTX(&dimmable_light_ctx);
  ZB_AF_SET_ENDPOINT_HANDLER(HA_DIMMABLE_LIGHT_ENDPOINT,
                             zcl_specific_cluster_cmd_handler);

  ZB_ZCL_SET_DEFAULT_VALUE_CB(zcl_reset_to_defaults_cb);

  /* set to 1 to enable nvram usage. */
  ZB_AIB().aps_use_nvram = 1;
  ZB_AIB().aps_nvram_erase_at_start = 0;

  ZB_AIB().aps_channel_mask = (1L << 11);
#ifdef ZB_BDB_MODE
  ZB_BDB().bdb_primary_channel_set = (1L << 11);
#endif

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

void zcl_reset_to_defaults_cb(zb_uint8_t param) ZB_CALLBACK
{
  zb_uint8_t canceled_param;
  (void)param;
  TRACE_MSG(TRACE_APP1, ">> zcl_reset_to_defaults_cb", (FMT__0));

  /* Reset zcl attrs to default values */
  zb_zcl_init_reporting_info();
  zb_zcl_reset_reporting_ctx();
  /* Init default reporting again */

  /* If we fail, trace is given and assertion is triggered */
  (void)zb_nvram_write_dataset(ZB_NVRAM_HA_DATA);
  (void)zb_nvram_write_dataset(ZB_NVRAM_ZCL_REPORTING_DATA);

  TRACE_MSG(TRACE_APP1, "<< zcl_reset_to_defaults_cb", (FMT__0));
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

#ifdef ZB_BDB_MODE
void zb_zdo_startup_complete(zb_uint8_t param)
{
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
        break;

      case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APP1, "Steering signal", (FMT__0));
        break;

      case ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED:
      case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:
        TRACE_MSG(TRACE_APP1, "Finding&binding signal", (FMT__0));
        break;

      default:
        TRACE_MSG(TRACE_APP1, "Unknown signal", (FMT__0));
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
#else
void zb_zdo_startup_complete(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);

  TRACE_MSG(TRACE_ZCL1, "> zb_zdo_startup_complete %h", (FMT__H, param));

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
  }

  zb_free_buf(buf);

  TRACE_MSG(TRACE_ZCL1, "< zb_zdo_startup_complete", (FMT__0));
}
#endif

#else // defined ZB_ENABLE_HA

#include <stdio.h>
int main()
{
  printf(" HA is not supported\n");
  return 0;
}

#endif // defined ZB_ENABLE_HA
