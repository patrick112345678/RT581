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
/* PURPOSE: ZB Simple output device
*/

#define ZB_TRACE_FILE_ID 40198
#include "zboss_api.h"
#include "zb_led_button.h"
#include "zb_window_covering.h"

/* Insert that include before any code or declaration. */
#ifdef ZB_CONFIGURABLE_MEM
#include "zb_mem_config_max.h"
#endif
/* Next define clusters, attributes etc. */


#include "../common/zcl_basic_attr_list.h"

void test_device_cb(zb_uint8_t param);
zb_uint8_t test_device_scenes_get_entry(zb_uint16_t group_id, zb_uint8_t scene_id);
void test_device_scenes_remove_entries_by_group(zb_uint16_t group_id);
void test_device_scenes_table_init();

#define TEST_DEVICE_SCENES_TABLE_SIZE 3

typedef struct test_device_scenes_table_entry_s
{
  zb_zcl_scene_table_record_fixed_t common;
  zb_uint8_t lift_percentage_state;
  zb_uint8_t tilt_percentage_state;
}
test_device_scenes_table_entry_t;

test_device_scenes_table_entry_t scenes_table[TEST_DEVICE_SCENES_TABLE_SIZE];

typedef struct resp_info_s
{
  zb_zcl_parsed_hdr_t cmd_info;
  zb_zcl_scenes_view_scene_req_t view_scene_req;
  zb_zcl_scenes_get_scene_membership_req_t get_scene_membership_req;
} resp_info_t;

resp_info_t resp_info;

/*
#if ! defined ZB_COORDINATOR_ROLE
#error define ZB_COORDINATOR_ROLE to compile zc tests
#endif
*/
#if !defined ZB_ROUTER_ROLE
#error define ZB_ROUTER_ROLE to build led bulb demo
#endif

#define ZB_OUTPUT_ENDPOINT          5
#define ZB_OUTPUT_MAX_CMD_PAYLOAD_SIZE 2

/* Handler for specific zcl commands */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);
zb_ieee_addr_t g_zc_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
void test_device_interface_cb(zb_uint8_t param);
void button_press_handler(zb_uint8_t param);
#define ENDPOINT_C 5
#define ENDPOINT_ED 10

/* [COMMON_DECLARATION] */
/* Identify cluster attributes data */
zb_uint16_t g_attr_identify_identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;
/* Groups cluster attributes data */
zb_uint8_t g_attr_groups_name_support = 0;
/* Scenes cluster attributes data */
zb_uint8_t g_attr_scenes_scene_count = ZB_ZCL_SCENES_SCENE_COUNT_DEFAULT_VALUE;
zb_uint8_t g_attr_scenes_current_scene = ZB_ZCL_SCENES_CURRENT_SCENE_DEFAULT_VALUE;
zb_uint16_t g_attr_scenes_current_group = ZB_ZCL_SCENES_CURRENT_GROUP_DEFAULT_VALUE;
zb_uint8_t g_attr_scenes_scene_valid = ZB_ZCL_SCENES_SCENE_VALID_DEFAULT_VALUE;
zb_uint8_t g_attr_scenes_name_support = 0;
/* Window Covering cluster attributes data */
zb_uint8_t g_attr_window_covering_window_covering_type = ZB_ZCL_WINDOW_COVERING_WINDOW_COVERING_TYPE_DEFAULT_VALUE;
zb_uint8_t g_attr_window_covering_config_status = ZB_ZCL_WINDOW_COVERING_CONFIG_STATUS_DEFAULT_VALUE;
zb_uint8_t g_attr_window_covering_current_position_lift_percentage = ZB_ZCL_WINDOW_COVERING_CURRENT_POSITION_LIFT_PERCENTAGE_DEFAULT_VALUE;
zb_uint8_t g_attr_window_covering_current_position_tilt_percentage = 0x32;//ZB_ZCL_WINDOW_COVERING_CURRENT_POSITION_TILT_PERCENTAGE_DEFAULT_VALUE;
zb_uint16_t g_attr_window_covering_installed_open_limit_lift = ZB_ZCL_WINDOW_COVERING_INSTALLED_OPEN_LIMIT_LIFT_DEFAULT_VALUE;
zb_uint16_t g_attr_window_covering_installed_closed_limit_lift = ZB_ZCL_WINDOW_COVERING_INSTALLED_CLOSED_LIMIT_LIFT_DEFAULT_VALUE;
zb_uint16_t g_attr_window_covering_installed_open_limit_tilt = ZB_ZCL_WINDOW_COVERING_INSTALLED_OPEN_LIMIT_TILT_DEFAULT_VALUE;
zb_uint16_t g_attr_window_covering_installed_closed_limit_tilt = ZB_ZCL_WINDOW_COVERING_INSTALLED_CLOSED_LIMIT_TILT_DEFAULT_VALUE;
zb_uint8_t g_attr_window_covering_mode = ZB_ZCL_WINDOW_COVERING_MODE_DEFAULT_VALUE;
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_attr_identify_identify_time);
ZB_ZCL_DECLARE_GROUPS_ATTRIB_LIST(groups_attr_list, &g_attr_groups_name_support);
ZB_ZCL_DECLARE_SCENES_ATTRIB_LIST(scenes_attr_list, &g_attr_scenes_scene_count, &g_attr_scenes_current_scene, &g_attr_scenes_current_group, &g_attr_scenes_scene_valid, &g_attr_scenes_name_support);
ZB_ZCL_DECLARE_WINDOW_COVERING_CLUSTER_ATTRIB_LIST(window_covering_attr_list, &g_attr_window_covering_window_covering_type, &g_attr_window_covering_config_status, &g_attr_window_covering_current_position_lift_percentage, &g_attr_window_covering_current_position_tilt_percentage, &g_attr_window_covering_installed_open_limit_lift, &g_attr_window_covering_installed_closed_limit_lift, &g_attr_window_covering_installed_open_limit_tilt, &g_attr_window_covering_installed_closed_limit_tilt, &g_attr_window_covering_mode);
/********************* Declare device **************************/
ZB_HA_DECLARE_WINDOW_COVERING_CLUSTER_LIST(window_covering_clusters, basic_attr_list, identify_attr_list, groups_attr_list, scenes_attr_list, window_covering_attr_list);
ZB_HA_DECLARE_WINDOW_COVERING_EP(window_covering_ep, ENDPOINT_C, window_covering_clusters);
ZB_HA_DECLARE_WINDOW_COVERING_CTX(device_ctx, window_covering_ep);
/* [COMMON_DECLARATION] */
zb_uint16_t g_dst_addr = 0;
MAIN()
{
  ARGV_UNUSED;

  ZB_SET_TRACE_ON();
  ZB_SET_TRAF_DUMP_ON();

  ZB_INIT("sample_zc");

  zb_set_long_address(g_zc_addr);
  zb_set_network_coordinator_role(1l<<24);
  zb_set_nvram_erase_at_start(ZB_TRUE);
  zb_set_max_children(1);
  zb_set_pan_id(0x029a);

/* [REGISTER] */
  /* Register device list */
  ZB_AF_REGISTER_DEVICE_CTX(&device_ctx);
  ZB_AF_SET_ENDPOINT_HANDLER(ZB_OUTPUT_ENDPOINT, zcl_specific_cluster_cmd_handler);
  ZB_ZCL_REGISTER_DEVICE_CB(test_device_interface_cb);
/* [REGISTER] */
#ifdef ZB_USE_BUTTONS
  zb_button_register_handler(0, 0, button_press_handler);
#endif

  if (zboss_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
  }
  else
  {
    zboss_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}

/* [ZCL_COMMAND_HANDLER] */
void test_device_interface_cb(zb_uint8_t param)
{

  zb_zcl_device_callback_param_t *device_cb_param =
    ZB_BUF_GET_PARAM(param, zb_zcl_device_callback_param_t);

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
      break;
    case ZB_ZCL_WINDOW_COVERING_UP_OPEN_CB_ID:
    {
      zb_uint8_t lift_percentage_val = 0x00;
      zb_uint8_t tilt_percentage_val = 0x00;

      ZVUNUSED(zb_zcl_set_attr_val(ENDPOINT_C,
                                   ZB_ZCL_CLUSTER_ID_WINDOW_COVERING,
                                   ZB_ZCL_CLUSTER_SERVER_ROLE,
                                   ZB_ZCL_ATTR_WINDOW_COVERING_CURRENT_POSITION_LIFT_PERCENTAGE_ID,
                                   &lift_percentage_val,
                                   ZB_FALSE));

      ZVUNUSED(zb_zcl_set_attr_val(ENDPOINT_C,
                                   ZB_ZCL_CLUSTER_ID_WINDOW_COVERING,
                                   ZB_ZCL_CLUSTER_SERVER_ROLE,
                                   ZB_ZCL_ATTR_WINDOW_COVERING_CURRENT_POSITION_TILT_PERCENTAGE_ID,
                                   &tilt_percentage_val,
                                   ZB_FALSE));
      break;
    }
    case ZB_ZCL_WINDOW_COVERING_DOWN_CLOSE_CB_ID:
    {
      zb_uint8_t lift_percentage_val = 0x64;
      zb_uint8_t tilt_percentage_val = 0x64;

      ZVUNUSED(zb_zcl_set_attr_val(ENDPOINT_C,
                                   ZB_ZCL_CLUSTER_ID_WINDOW_COVERING,
                                   ZB_ZCL_CLUSTER_SERVER_ROLE,
                                   ZB_ZCL_ATTR_WINDOW_COVERING_CURRENT_POSITION_LIFT_PERCENTAGE_ID,
                                   &lift_percentage_val,
                                   ZB_FALSE));

      ZVUNUSED(zb_zcl_set_attr_val(ENDPOINT_C,
                                   ZB_ZCL_CLUSTER_ID_WINDOW_COVERING,
                                   ZB_ZCL_CLUSTER_SERVER_ROLE,
                                   ZB_ZCL_ATTR_WINDOW_COVERING_CURRENT_POSITION_TILT_PERCENTAGE_ID,
                                   &tilt_percentage_val,
                                   ZB_FALSE));
      break;
    }
    case ZB_ZCL_WINDOW_COVERING_STOP_CB_ID:
      break;
    case ZB_ZCL_WINDOW_COVERING_GO_TO_LIFT_PERCENTAGE_CB_ID:
    {
      const zb_zcl_go_to_lift_percentage_req_t *lift_percentage = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_go_to_lift_percentage_req_t);
      zb_zcl_attr_t *attr_desc = zb_zcl_get_attr_desc_a(ENDPOINT_C,
          ZB_ZCL_CLUSTER_ID_WINDOW_COVERING, ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_WINDOW_COVERING_CURRENT_POSITION_LIFT_PERCENTAGE_ID);
      if (attr_desc)
      {
        ZVUNUSED(zb_zcl_set_attr_val(ENDPOINT_C,
                                     ZB_ZCL_CLUSTER_ID_WINDOW_COVERING,
                                     ZB_ZCL_CLUSTER_SERVER_ROLE,
                                     ZB_ZCL_ATTR_WINDOW_COVERING_CURRENT_POSITION_LIFT_PERCENTAGE_ID,
                                     (zb_uint8_t *)&(lift_percentage->percentage_lift_value),
                                     ZB_FALSE));
      }
      break;
    }
    case ZB_ZCL_WINDOW_COVERING_GO_TO_TILT_PERCENTAGE_CB_ID:
    {
      const zb_zcl_go_to_tilt_percentage_req_t *tilt_percentage = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_go_to_tilt_percentage_req_t);
      zb_zcl_attr_t *attr_desc = zb_zcl_get_attr_desc_a(ENDPOINT_C,
            ZB_ZCL_CLUSTER_ID_WINDOW_COVERING, ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_WINDOW_COVERING_CURRENT_POSITION_TILT_PERCENTAGE_ID);
      if (attr_desc)
      {
        ZVUNUSED(zb_zcl_set_attr_val(ENDPOINT_C,
                                     ZB_ZCL_CLUSTER_ID_WINDOW_COVERING,
                                     ZB_ZCL_CLUSTER_SERVER_ROLE,
                                     ZB_ZCL_ATTR_WINDOW_COVERING_CURRENT_POSITION_TILT_PERCENTAGE_ID,
                                     (zb_uint8_t *)&(tilt_percentage->percentage_tilt_value),
                                     ZB_FALSE));
      }
      break;
    }
    /* >>>> Scenes */
    case ZB_ZCL_SCENES_STORE_SCENE_CB_ID:
    {
      const zb_zcl_scenes_store_scene_req_t *store_scene_req = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_scenes_store_scene_req_t);
      zb_uint8_t idx = 0xFF;
      zb_uint8_t *store_scene_status = ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(param, zb_uint8_t);
      const zb_zcl_parsed_hdr_t *in_cmd_info = ZB_ZCL_DEVICE_CMD_PARAM_CMD_INFO(param);

      TRACE_MSG(TRACE_APP1, "ZB_ZCL_SCENES_STORE_SCENE_CB_ID: group_id 0x%x scene_id 0x%hd", (FMT__D_H, store_scene_req->group_id, store_scene_req->scene_id));

      if (!zb_aps_is_endpoint_in_group(
                 store_scene_req->group_id,
                 ZB_ZCL_PARSED_HDR_SHORT_DATA(in_cmd_info).dst_endpoint))
      {
        *store_scene_status = ZB_ZCL_STATUS_INVALID_FIELD;
      }
      else
      {
        idx = test_device_scenes_get_entry(store_scene_req->group_id, store_scene_req->scene_id);

        if (idx != 0xFF)
        {
          if (scenes_table[idx].common.group_id != ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD)
          {
            /* Update existing entry with current On/Off state */
            device_cb_param->status = RET_ALREADY_EXISTS;
            scenes_table[idx].lift_percentage_state = g_attr_window_covering_current_position_lift_percentage;
            scenes_table[idx].tilt_percentage_state = g_attr_window_covering_current_position_tilt_percentage;
          }
          else
          {
            /* Create new entry with empty name and 0 transition time */
            scenes_table[idx].common.group_id = store_scene_req->group_id;
            scenes_table[idx].common.scene_id = store_scene_req->scene_id;
            scenes_table[idx].common.transition_time = 0;
            scenes_table[idx].lift_percentage_state = g_attr_window_covering_current_position_lift_percentage;
            scenes_table[idx].tilt_percentage_state = g_attr_window_covering_current_position_tilt_percentage;
          }

          *store_scene_status = ZB_ZCL_STATUS_SUCCESS;
        }
        else
        {
          *store_scene_status = ZB_ZCL_STATUS_INSUFF_SPACE;
        }
      }
    }
    break;
    case ZB_ZCL_SCENES_RECALL_SCENE_CB_ID:
    {
      const zb_zcl_scenes_recall_scene_req_t *recall_scene_req = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_scenes_recall_scene_req_t);
      zb_uint8_t idx = 0xFF;
      zb_uint8_t *recall_scene_status = ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(param, zb_uint8_t);

      TRACE_MSG(TRACE_APP1, "ZB_ZCL_SCENES_RECALL_SCENE_CB_ID: group_id 0x%x scene_id 0x%hd", (FMT__D_H, recall_scene_req->group_id, recall_scene_req->scene_id));

      idx = test_device_scenes_get_entry(recall_scene_req->group_id, recall_scene_req->scene_id);

      if (idx != 0xFF &&
          scenes_table[idx].common.group_id != ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD)
      {
        /* Recall this entry */
        ZB_ZCL_SET_ATTRIBUTE(
          ENDPOINT_C,
          ZB_ZCL_CLUSTER_ID_WINDOW_COVERING,
          ZB_ZCL_CLUSTER_SERVER_ROLE,
          ZB_ZCL_ATTR_WINDOW_COVERING_CURRENT_POSITION_LIFT_PERCENTAGE_ID,
          &scenes_table[idx].lift_percentage_state,
          ZB_FALSE);
        ZB_ZCL_SET_ATTRIBUTE(
          ENDPOINT_C,
          ZB_ZCL_CLUSTER_ID_WINDOW_COVERING,
          ZB_ZCL_CLUSTER_SERVER_ROLE,
          ZB_ZCL_ATTR_WINDOW_COVERING_CURRENT_POSITION_TILT_PERCENTAGE_ID,
          &scenes_table[idx].tilt_percentage_state,
          ZB_FALSE);
        *recall_scene_status = ZB_ZCL_STATUS_SUCCESS;
      }
      else
      {
        *recall_scene_status = ZB_ZCL_STATUS_NOT_FOUND;
      }
    }
    break;
    case ZB_ZCL_SCENES_INTERNAL_REMOVE_ALL_SCENES_ALL_ENDPOINTS_ALL_GROUPS_CB_ID:
    {
      test_device_scenes_table_init();
    }
    break;
      /* <<<< Scenes */
    default:
      device_cb_param->status = RET_OK;
      break;
  }

  TRACE_MSG(TRACE_APP1, "< test_device_interface_cb %hd", (FMT__H, device_cb_param->status));
}
/* [ZCL_COMMAND_HANDLER] */

/* [COMMAND_HANDLER] */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
  zb_zcl_parsed_hdr_t cmd_info;
  zb_uint8_t lqi = ZB_MAC_LQI_UNDEFINED;
  zb_int8_t rssi = ZB_MAC_RSSI_UNDEFINED;

  TRACE_MSG(TRACE_APP1, "> zcl_specific_cluster_cmd_handler", (FMT__0));

  ZB_ZCL_COPY_PARSED_HEADER(param, &cmd_info);

  g_dst_addr = ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).source.u.short_addr;

  ZB_ZCL_DEBUG_DUMP_HEADER(&cmd_info);
  TRACE_MSG(TRACE_APP3, "payload size: %i", (FMT__D, zb_buf_len(param)));

  zb_zdo_get_diag_data(g_dst_addr, &lqi, &rssi);
  TRACE_MSG(TRACE_APP3, "lqi %hd rssi %d", (FMT__H_H, lqi, rssi));

  if (cmd_info.cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI)
  {
    TRACE_MSG(
        TRACE_ERROR,
        "Unsupported \"from server\" command direction",
        (FMT__0));
  }

  TRACE_MSG(TRACE_APP1, "< zcl_specific_cluster_cmd_handler", (FMT__0));
  return ZB_FALSE;
}
/* [COMMAND_HANDLER] */

void button_press_handler(zb_uint8_t param)
{
  ZVUNUSED(param);
  TRACE_MSG(TRACE_APP1, "button is pressed, do nothing", (FMT__0));
}
void zboss_signal_handler(zb_uint8_t param)
{
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        break;

      case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APP1, "Successfull steering, start f&b target", (FMT__0));
        zb_bdb_finding_binding_target(ZB_OUTPUT_ENDPOINT);
        break;
      default:
        TRACE_MSG(TRACE_APP1, "Unknown signal %d", (FMT__D, (zb_uint16_t)sig));
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

  if (param)
  {
    zb_buf_free(param);
  }
}

void test_device_scenes_table_init()
{
  zb_uint8_t i = 0;
  while (i < TEST_DEVICE_SCENES_TABLE_SIZE)
  {
    scenes_table[i].common.group_id = ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD;
    ++i;
  }
}

zb_uint8_t test_device_scenes_get_entry(zb_uint16_t group_id, zb_uint8_t scene_id)
{
  zb_uint8_t i = 0;
  zb_uint8_t idx = 0xFF, free_idx = 0xFF;

  while (i < TEST_DEVICE_SCENES_TABLE_SIZE)
  {
    if (scenes_table[i].common.group_id == group_id &&
        scenes_table[i].common.group_id == scene_id)
    {
      idx = i;
      break;
    }
    else if (free_idx == 0xFF &&
             scenes_table[i].common.group_id == ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD)
    {
      /* Remember free index */
      free_idx = i;
    }
    ++i;
  }

  return ((idx != 0xFF) ? idx : free_idx);
}

void test_device_scenes_remove_entries_by_group(zb_uint16_t group_id)
{
  zb_uint8_t i = 0;

  TRACE_MSG(TRACE_APP1, ">> test_device_scenes_remove_entries_by_group: group_id 0x%x", (FMT__D, group_id));
  while (i < TEST_DEVICE_SCENES_TABLE_SIZE)
  {
    if (scenes_table[i].common.group_id == group_id)
    {
      TRACE_MSG(TRACE_APP1, "removing scene: entry idx %hd", (FMT__H, i));
      scenes_table[i].common.group_id = ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD;
    }
    ++i;
  }
  TRACE_MSG(TRACE_APP1, "<< test_device_scenes_remove_entries_by_group", (FMT__0));
}
