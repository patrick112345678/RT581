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
/* PURPOSE: Touchlink Dimmable light device
*/

#define ZB_TRACE_FILE_ID 40190
#include "test_defs.h"
#include "zboss_api.h"
#include "zb_led_button.h"
#include "zb_tl_dimmable_light.h"

void test_check_start_status(zb_uint8_t param);
void button_press_handler(zb_uint8_t param);
void test_device_interface_cb(zb_uint8_t param);

#if ! defined ZB_ROUTER_ROLE
#error define ZB_ROUTER_ROLE to compile zr tests
#endif /* ! defined ZB_ROUTER_ROLE */

/** @brief Endpoint the device operates at. */
#define ENDPOINT  1

zb_ieee_addr_t g_zr_addr = {1, 1, 1, 1, 1, 1, 1, 1};

//security keys
zb_uint8_t g_key[16]               = { 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0, 0, 0, 0, 0, 0, 0, 0};

/******************* Declare attributes ************************/

//* On/Off cluster attributes data */
zb_uint8_t g_attr_on_off  = ZB_ZCL_ON_OFF_ON_OFF_DEFAULT_VALUE;
/* On/Off cluster attributes additions data */
zb_bool_t g_attr_global_scene_ctrl  = ZB_TRUE;
zb_uint16_t g_attr_on_time  = 0;
zb_uint16_t g_attr_off_wait_time  = 0;

ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST_EXT(on_off_attr_list, &g_attr_on_off,
    &g_attr_global_scene_ctrl, &g_attr_on_time, &g_attr_off_wait_time);

//! [BASIC_CLUSTER_DECLARE]
/* Basic cluster attributes data */
zb_uint8_t g_attr_zcl_version = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
zb_uint8_t g_attr_app_version = 0;
zb_uint8_t g_attr_stack_version = 0;
zb_uint8_t g_attr_hardware_version = 0;
zb_char_t g_attr_manufacturer_name[] = "\x15" "Test manufacture name";
zb_char_t g_attr_model_id[] = "\x03" "1.0";
zb_char_t g_attr_date_code[] = "\x0a" "20130509GP";
zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;
zb_char_t g_attr_sw_build_id[] = "\x0f" "111.111.111.111";
zb_char_t g_attr_location_id[] = "\x02" "us";
zb_uint8_t g_attr_ph_env = ZB_ZCL_BASIC_ENV_UNSPECIFIED;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST_EXT(basic_attr_list, &g_attr_zcl_version, &g_attr_app_version,
    &g_attr_stack_version, &g_attr_hardware_version, &g_attr_manufacturer_name, &g_attr_model_id,
    &g_attr_date_code, &g_attr_power_source, &g_attr_location_id, &g_attr_ph_env,
    &g_attr_sw_build_id);
//! [BASIC_CLUSTER_DECLARE] */

/* Identify cluster attributes data */
zb_uint16_t g_attr_identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_attr_identify_time);

/* Groups cluster attributes data */
zb_uint8_t g_attr_name_support = 0;

ZB_ZCL_DECLARE_GROUPS_ATTRIB_LIST(groups_attr_list, &g_attr_name_support);

/* Scenes cluster attribute data */
zb_uint8_t g_attr_scenes_scene_count = ZB_ZCL_SCENES_SCENE_COUNT_DEFAULT_VALUE;
zb_uint8_t g_attr_scenes_current_scene = ZB_ZCL_SCENES_CURRENT_SCENE_DEFAULT_VALUE;
zb_uint16_t g_attr_scenes_current_group = ZB_ZCL_SCENES_CURRENT_GROUP_DEFAULT_VALUE;
zb_uint8_t g_attr_scenes_scene_valid = ZB_ZCL_SCENES_SCENE_VALID_DEFAULT_VALUE;
zb_uint8_t g_attr_scenes_name_support = ZB_ZCL_SCENES_NAME_SUPPORT_DEFAULT_VALUE;

ZB_ZCL_DECLARE_SCENES_ATTRIB_LIST(scenes_attr_list, &g_attr_scenes_scene_count,
    &g_attr_scenes_current_scene, &g_attr_scenes_current_group,
    &g_attr_scenes_scene_valid, &g_attr_scenes_name_support);

/* Level cluster attribute data */

zb_uint8_t g_level_control_current_level = ZB_ZCL_LEVEL_CONTROL_CURRENT_LEVEL_DEFAULT_VALUE;
zb_uint16_t g_level_control_remaining_time = ZB_ZCL_LEVEL_CONTROL_REMAINING_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_LEVEL_CONTROL_ATTRIB_LIST(
    level_control_attr_list, &g_level_control_current_level, &g_level_control_remaining_time);

/********************* Declare device **************************/
ZB_TL_DECLARE_DIMMABLE_LIGHT_CLUSTER_LIST(
    dimmable_light_clusters,
    basic_attr_list, identify_attr_list, groups_attr_list,
    scenes_attr_list, on_off_attr_list, level_control_attr_list,
    ZB_FALSE);

ZB_TL_DECLARE_DIMMABLE_LIGHT_EP(dimmable_light_ep, ENDPOINT, dimmable_light_clusters);

ZB_TL_DECLARE_DIMMABLE_LIGHT_CTX(dimmable_light_ctx, dimmable_light_ep);

MAIN()
{
  zb_uint32_t primary_channel_set;
  zb_uint32_t secondary_channel_set;

  ARGV_UNUSED;

  ZB_SET_TRACE_ON();
  ZB_SET_TRAF_DUMP_ON();

  ZB_INIT("light_device_zr");

  zb_set_long_address(g_zr_addr);
  zb_set_network_router_role(1L << TEST_CHANNEL);
  zb_set_nvram_erase_at_start(ZB_FALSE);
  zb_set_bdb_commissioning_mode(ZB_BDB_TOUCHLINK_TARGET);

  /* Trace current primary and secondary channel masks */
  primary_channel_set = zb_get_bdb_primary_channel_set();
  secondary_channel_set = zb_get_bdb_secondary_channel_set();
  TRACE_MSG(TRACE_APP1, "Primary channel mask 0x%lx", (FMT__L, primary_channel_set));
  TRACE_MSG(TRACE_APP1, "Secondary channel mask 0x%lx", (FMT__L, secondary_channel_set));

  /* Set new masks (same in this example) */
  zb_set_bdb_primary_channel_set(primary_channel_set);
  zb_set_bdb_secondary_channel_set(secondary_channel_set);

  ZB_AF_REGISTER_DEVICE_CTX(&dimmable_light_ctx);
  ZB_ZCL_REGISTER_DEVICE_CB(test_device_interface_cb);

#ifdef ZB_USE_BUTTONS
  zb_button_register_handler(0, 0, button_press_handler);
#endif

  if (zboss_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "ERROR zboss_start failed", (FMT__0));
  }
  else
  {
    zboss_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}

void button_press_handler(zb_uint8_t param)
{
  ZVUNUSED(param);
  TRACE_MSG(TRACE_APP1, "button is pressed, do nothing", (FMT__0));
}

void zboss_signal_handler(zb_uint8_t param)
{
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  switch(sig)
  {
    case ZB_BDB_SIGNAL_TOUCHLINK_TARGET:
      TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
      TRACE_MSG(TRACE_APP1, "Touchlink target started", (FMT__0));
      break;
    case ZB_BDB_SIGNAL_TOUCHLINK_NWK:
      TRACE_MSG(TRACE_APP1, "Touchlink target : network started", (FMT__0));
      break;
    case ZB_BDB_SIGNAL_TOUCHLINK_TARGET_FINISHED:
      TRACE_MSG(TRACE_APP1, "Touchlink target finished", (FMT__0));
      zb_bdb_finding_binding_target(ENDPOINT);
      break;
    default:
      TRACE_MSG(TRACE_APP1, "Unknown signal %hd status %hd", (FMT__H_H, sig, ZB_GET_APP_SIGNAL_STATUS(param)));
      break;
  }
  if (param)
  {
    zb_buf_free(param);
  }
}

void test_device_interface_cb(zb_uint8_t param)
{
  zb_bufid_t buffer = param;
  zb_zcl_device_callback_param_t *device_cb_param =
    ZB_BUF_GET_PARAM(buffer, zb_zcl_device_callback_param_t);

  TRACE_MSG(TRACE_APP1, "> test_device_interface_cb param %hd id %hd", (FMT__H_H,
      param, device_cb_param->device_cb_id));

  device_cb_param->status = RET_OK;

  switch (device_cb_param->device_cb_id)
  {
    case ZB_ZCL_SET_ATTR_VALUE_CB_ID:
      if (device_cb_param->cb_param.set_attr_value_param.cluster_id == ZB_ZCL_CLUSTER_ID_ON_OFF &&
          device_cb_param->cb_param.set_attr_value_param.attr_id == ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID)
      {
        if (device_cb_param->cb_param.set_attr_value_param.values.data8)
        {
          TRACE_MSG(TRACE_APP1, "set ON", (FMT__0));
#ifdef ZB_USE_BUTTONS
          zb_osif_led_on(0);
#endif
        }
        else
        {
          TRACE_MSG(TRACE_APP1, "set OFF", (FMT__0));
#ifdef ZB_USE_BUTTONS
          zb_osif_led_off(0);
#endif
        }
      }
      else if (device_cb_param->cb_param.set_attr_value_param.cluster_id == ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL &&
               device_cb_param->cb_param.set_attr_value_param.attr_id == ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID)
      {
        TRACE_MSG(TRACE_APP1, "set current level to %hd", (FMT__H, device_cb_param->cb_param.set_attr_value_param.values.data8));
        /* TODO: Set level on HW */
      }
      break;

    default:
      device_cb_param->status = RET_ERROR;
      break;
  }

  TRACE_MSG(TRACE_APP1, "< test_device_interface_cb %hd", (FMT__H, device_cb_param->status));
}
