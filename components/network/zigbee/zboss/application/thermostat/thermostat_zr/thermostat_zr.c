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
/* PURPOSE: ZR Thermostat sample for HA profile
*/

#define ZB_TRACE_FILE_ID 60112

#include "thermostat_zr.h"

#if ! defined ZB_ROUTER_ROLE
#error define ZB_ROUTER_ROLE to compile zc tests
#endif

/**
 * Global variables definitions
 */
zb_ieee_addr_t g_zr_addr = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; /* IEEE address of the
                                                                              * device */

zb_uint16_t g_dst_addr;
zb_uint8_t g_addr_mode;
zb_uint8_t g_endpoint;

test_device_scenes_table_entry_t scenes_table[TEST_DEVICE_SCENES_TABLE_SIZE];
resp_info_t resp_info;

/**
 * Declaring attributes for each cluster
 */

/* Basic cluster attributes */
zb_uint8_t g_attr_zcl_version  = ZB_ZCL_VERSION;
zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list, &g_attr_zcl_version, &g_attr_power_source);

/* Identify cluster attributes */
zb_uint16_t g_attr_identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_attr_identify_time);

/* Groups cluster attributes */
zb_uint8_t g_attr_groups_name_support = 0;

/* Scenes cluster attributes */
zb_uint8_t g_attr_scenes_scene_count = ZB_ZCL_SCENES_SCENE_COUNT_DEFAULT_VALUE;
zb_uint8_t g_attr_scenes_current_scene = ZB_ZCL_SCENES_CURRENT_SCENE_DEFAULT_VALUE;
zb_uint16_t g_attr_scenes_current_group = ZB_ZCL_SCENES_CURRENT_GROUP_DEFAULT_VALUE;
zb_uint8_t g_attr_scenes_scene_valid = ZB_ZCL_SCENES_SCENE_VALID_DEFAULT_VALUE;
zb_uint8_t g_attr_scenes_name_support = 0;

ZB_ZCL_DECLARE_GROUPS_ATTRIB_LIST(groups_attr_list, &g_attr_groups_name_support);
ZB_ZCL_DECLARE_SCENES_ATTRIB_LIST(scenes_attr_list, &g_attr_scenes_scene_count, &g_attr_scenes_current_scene, &g_attr_scenes_current_group, &g_attr_scenes_scene_valid, &g_attr_scenes_name_support);

/* Thermostat cluster attributes */
zb_int16_t local_temperature = 0x4000;//ZB_ZCL_THERMOSTAT_LOCAL_TEMPERATURE_MIN_VALUE;
zb_int8_t local_temperature_calibration = ZB_ZCL_THERMOSTAT_LOCAL_TEMPERATURE_CALIBRATION_DEFAULT_VALUE;
zb_int16_t occupied_cooling_setpoint = ZB_ZCL_THERMOSTAT_OCCUPIED_COOLING_SETPOINT_DEFAULT_VALUE;
zb_int16_t occupied_heating_setpoint = ZB_ZCL_THERMOSTAT_OCCUPIED_HEATING_SETPOINT_DEFAULT_VALUE;
zb_uint8_t control_seq_of_operation = ZB_ZCL_THERMOSTAT_CONTROL_SEQ_OF_OPERATION_DEFAULT_VALUE;
zb_uint8_t system_mode = ZB_ZCL_THERMOSTAT_CONTROL_SYSTEM_MODE_DEFAULT_VALUE;
zb_uint8_t start_of_week = ZB_ZCL_THERMOSTAT_START_OF_WEEK_SUNDAY;

zb_int16_t abs_min_heat_setpoint_limit = ZB_ZCL_THERMOSTAT_ABS_MIN_HEAT_SETPOINT_LIMIT_DEFAULT_VALUE;
zb_int16_t abs_max_heat_setpoint_limit = ZB_ZCL_THERMOSTAT_ABS_MAX_HEAT_SETPOINT_LIMIT_DEFAULT_VALUE;
zb_int16_t abs_min_cool_setpoint_limit = ZB_ZCL_THERMOSTAT_ABS_MIN_COOL_SETPOINT_LIMIT_DEFAULT_VALUE;
zb_int16_t abs_max_cool_setpoint_limit = ZB_ZCL_THERMOSTAT_ABS_MAX_COOL_SETPOINT_LIMIT_DEFAULT_VALUE;
zb_uint8_t PI_cooling_demand;
zb_uint8_t PI_heating_demand;
zb_uint8_t HVAC_system_type_configuration = ZB_ZCL_THERMOSTAT_HVAC_SYSTEM_TYPE_CONFIGURATION_DEFAULT_VALUE;
zb_int16_t unoccupied_cooling_setpoint = ZB_ZCL_THERMOSTAT_UNOCCUPIED_COOLING_SETPOINT_DEFAULT_VALUE;
zb_int16_t unoccupied_heating_setpoint = ZB_ZCL_THERMOSTAT_UNOCCUPIED_HEATING_SETPOINT_DEFAULT_VALUE;
zb_int16_t min_heat_setpoint_limit = ZB_ZCL_THERMOSTAT_MIN_HEAT_SETPOINT_LIMIT_DEFAULT_VALUE;
zb_int16_t max_heat_setpoint_limit = ZB_ZCL_THERMOSTAT_MAX_HEAT_SETPOINT_LIMIT_DEFAULT_VALUE;
zb_int16_t min_cool_setpoint_limit = ZB_ZCL_THERMOSTAT_MIN_COOL_SETPOINT_LIMIT_DEFAULT_VALUE;
zb_int16_t max_cool_setpoint_limit = ZB_ZCL_THERMOSTAT_MAX_COOL_SETPOINT_LIMIT_DEFAULT_VALUE;
zb_int8_t min_setpoint_dead_band = ZB_ZCL_THERMOSTAT_MIN_SETPOINT_DEAD_BAND_DEFAULT_VALUE;
zb_uint8_t remote_sensing = ZB_ZCL_THERMOSTAT_REMOTE_SENSING_DEFAULT_VALUE;
ZB_ZCL_DECLARE_THERMOSTAT_ATTRIB_LIST_EXT(thermostat_attr_list,
      &local_temperature, &abs_min_heat_setpoint_limit, &abs_max_heat_setpoint_limit, &abs_min_cool_setpoint_limit,
      &abs_max_cool_setpoint_limit, &PI_cooling_demand, &PI_heating_demand, &HVAC_system_type_configuration, &local_temperature_calibration, &occupied_cooling_setpoint,
      &occupied_heating_setpoint, &unoccupied_cooling_setpoint, &unoccupied_heating_setpoint, &min_heat_setpoint_limit,
      &max_heat_setpoint_limit, &min_cool_setpoint_limit, &max_cool_setpoint_limit, &min_setpoint_dead_band, &remote_sensing,
      &control_seq_of_operation, &system_mode, &start_of_week);

/* Fan Control cluster attributes */
zb_uint8_t fan_mode = ZB_ZCL_FAN_CONTROL_FAN_MODE_DEFAULT_VALUE;
zb_uint8_t fan_mode_sequence = ZB_ZCL_FAN_CONTROL_FAN_MODE_SEQUENCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_FAN_CONTROL_ATTRIB_LIST(fan_control_attr_list,
    &fan_mode, &fan_mode_sequence);

/* Thermostat UI Control cluster attributes */
zb_uint8_t temperature_display_mode = ZB_ZCL_THERMOSTAT_UI_CONFIG_TEMPERATURE_DISPLAY_MODE_DEFAULT_VALUE;
zb_uint8_t keypad_lockout = ZB_ZCL_THERMOSTAT_UI_CONFIG_KEYPAD_LOCKOUT_DEFAULT_VALUE;

ZB_ZCL_DECLARE_THERMOSTAT_UI_CONFIG_ATTRIB_LIST(thermostat_ui_config_attr_list,
    &temperature_display_mode, &keypad_lockout);

/* Declare cluster list for the device */
ZB_HA_DECLARE_THERMOSTAT_CLUSTER_LIST_EXT(thermostat_clusters,
                                      basic_attr_list,
                                      identify_attr_list,
                                      groups_attr_list,
                                      scenes_attr_list,
                                      thermostat_attr_list,
                                      fan_control_attr_list,
                                      thermostat_ui_config_attr_list);

/* Declare endpoint */
ZB_HA_DECLARE_THERMOSTAT_EP(thermostat_ep, SRC_ENDPOINT, thermostat_clusters);

/* Declare application's device context for single-endpoint device */
ZB_HA_DECLARE_THERMOSTAT_CTX(thermostat_ctx, thermostat_ep);


MAIN()
{
  ARGV_UNUSED;

  /* Traffic dump enable */
  ZB_SET_TRAF_DUMP_ON();

/* [set_trace] */
  /* Configure trace */
  ZB_SET_TRACE_LEVEL(4);
  ZB_SET_TRACE_MASK(0x0800);
/* [set_trace] */

  /* Global ZBOSS initialization */
  ZB_INIT("thermostat_zr");

  /* Set up defaults for the commissioning */
  zb_set_long_address(g_zr_addr);
  zb_set_network_router_role(ZB_THERMOSTAT_CHANNEL_MASK);
  zb_set_nvram_erase_at_start(ZB_FALSE);

  /* Register device ZCL context */
  ZB_AF_REGISTER_DEVICE_CTX(&thermostat_ctx);

  /* Register cluster commands handler for a specific endpoint */
  /* callback will be call BEFORE stack handle */
  ZB_AF_SET_ENDPOINT_HANDLER(SRC_ENDPOINT, zcl_specific_cluster_cmd_handler);

  /* Set Device user application callback */
  /* callback will be call AFTER stack handle */
  ZB_ZCL_REGISTER_DEVICE_CB(thermostat_device_interface_cb);

  /* Initiate the stack start with starting the commissioning */
  if (zboss_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
  }
  else
  {
    /* Call the main loop */
    zboss_main_loop();
  }

  /* Deinitialize trace */
  TRACE_DEINIT();

  MAIN_RETURN(0);
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

void send_view_scene_resp(zb_uint8_t param, zb_uint16_t idx)
{
  zb_uint8_t *payload_ptr;
  zb_uint8_t view_scene_status = ZB_ZCL_STATUS_NOT_FOUND;

  TRACE_MSG(TRACE_APP1, ">> send_view_scene_resp param %hd idx %d", (FMT__H_D, param, idx));

  if (idx != 0xFF &&
      scenes_table[idx].common.group_id != ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD)
  {
    /* Scene found */
    view_scene_status = ZB_ZCL_STATUS_SUCCESS;
  }
  else if (!zb_aps_is_endpoint_in_group(
             resp_info.view_scene_req.group_id,
             ZB_ZCL_PARSED_HDR_SHORT_DATA(&resp_info.cmd_info).dst_endpoint))
  {
    /* Not in the group */
    view_scene_status = ZB_ZCL_STATUS_INVALID_FIELD;
  }

  ZB_ZCL_SCENES_INIT_VIEW_SCENE_RES(
    param,
    payload_ptr,
    resp_info.cmd_info.seq_number,
    view_scene_status,
    resp_info.view_scene_req.group_id,
    resp_info.view_scene_req.scene_id);

  if (view_scene_status == ZB_ZCL_STATUS_SUCCESS)
  {
    ZB_ZCL_SCENES_ADD_TRANSITION_TIME_VIEW_SCENE_RES(
      payload_ptr,
      scenes_table[idx].common.transition_time);

    ZB_ZCL_SCENES_ADD_SCENE_NAME_VIEW_SCENE_RES(
      payload_ptr,
      scenes_table[idx].common.scene_name);

    /* Extention set: Cluster ID = On/Off */
    ZB_ZCL_PACKET_PUT_DATA16_VAL(payload_ptr, ZB_ZCL_CLUSTER_ID_ON_OFF);

    /* Extention set: Fieldset length = 1 */
    ZB_ZCL_PACKET_PUT_DATA8(payload_ptr, 1);
 }

  ZB_ZCL_SCENES_SEND_VIEW_SCENE_RES(
    param,
    payload_ptr,
    ZB_ZCL_PARSED_HDR_SHORT_DATA(&resp_info.cmd_info).source.u.short_addr,
    ZB_ZCL_PARSED_HDR_SHORT_DATA(&resp_info.cmd_info).src_endpoint,
    ZB_ZCL_PARSED_HDR_SHORT_DATA(&resp_info.cmd_info).dst_endpoint,
    resp_info.cmd_info.profile_id,
    NULL);

  TRACE_MSG(TRACE_APP1, "<< send_view_scene_resp", (FMT__0));
}

void send_get_scene_membership_resp(zb_uint8_t param)
{
  zb_uint8_t *payload_ptr;
  zb_uint8_t *capacity_ptr;
  zb_uint8_t *scene_count_ptr;

  TRACE_MSG(TRACE_APP1, ">> send_get_scene_membership_resp param %hd", (FMT__H, param));

  if (!zb_aps_is_endpoint_in_group(
        resp_info.get_scene_membership_req.group_id,
        ZB_ZCL_PARSED_HDR_SHORT_DATA(&resp_info.cmd_info).dst_endpoint))
  {
    /* Not in the group */
    ZB_ZCL_SCENES_INIT_GET_SCENE_MEMBERSHIP_RES(
      param,
      payload_ptr,
      resp_info.cmd_info.seq_number,
      capacity_ptr,
      ZB_ZCL_STATUS_INVALID_FIELD,
      ZB_ZCL_SCENES_CAPACITY_UNKNOWN,
      resp_info.get_scene_membership_req.group_id);
  }
  else
  {
    zb_uint8_t i = 0;

    ZB_ZCL_SCENES_INIT_GET_SCENE_MEMBERSHIP_RES(
      param,
      payload_ptr,
      resp_info.cmd_info.seq_number,
      capacity_ptr,
      ZB_ZCL_STATUS_SUCCESS,
      0,
      resp_info.get_scene_membership_req.group_id);

    scene_count_ptr = payload_ptr;
    ZB_ZCL_SCENES_ADD_SCENE_COUNT_GET_SCENE_MEMBERSHIP_RES(payload_ptr, 0);

    while (i < TEST_DEVICE_SCENES_TABLE_SIZE)
    {
      if (scenes_table[i].common.group_id == resp_info.get_scene_membership_req.group_id)
      {
        /* Add to payload */
        TRACE_MSG(TRACE_APP1, "add scene_id %hd", (FMT__H, scenes_table[i].common.scene_id));
        ++(*scene_count_ptr);
        ZB_ZCL_SCENES_ADD_SCENE_ID_GET_SCENE_MEMBERSHIP_RES(
          payload_ptr,
          scenes_table[i].common.scene_id);
      }
      else if (scenes_table[i].common.group_id == ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD)
      {
        TRACE_MSG(TRACE_APP1, "add capacity num", (FMT__0));
        ++(*capacity_ptr);
      }
      ++i;
    }
  }

  ZB_ZCL_SCENES_SEND_GET_SCENE_MEMBERSHIP_RES(
    param,
    payload_ptr,
    ZB_ZCL_PARSED_HDR_SHORT_DATA(&resp_info.cmd_info).source.u.short_addr,
    ZB_ZCL_PARSED_HDR_SHORT_DATA(&resp_info.cmd_info).src_endpoint,
    ZB_ZCL_PARSED_HDR_SHORT_DATA(&resp_info.cmd_info).dst_endpoint,
    resp_info.cmd_info.profile_id,
    NULL);

  TRACE_MSG(TRACE_APP1, "<< send_get_scene_membership_resp", (FMT__0));
}

zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
  zb_zcl_parsed_hdr_t cmd_info;
  zb_uint8_t lqi = ZB_MAC_LQI_UNDEFINED;
  zb_int8_t rssi = ZB_MAC_RSSI_UNDEFINED;

  TRACE_MSG(TRACE_APP1, ">> zcl_specific_cluster_cmd_handler", (FMT__0));

  ZB_ZCL_COPY_PARSED_HEADER(param, &cmd_info);

  g_dst_addr = ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).source.u.short_addr;
  g_endpoint = ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).src_endpoint;
  g_addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;

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

  TRACE_MSG(TRACE_APP1, "<< zcl_specific_cluster_cmd_handler", (FMT__0));
  return ZB_FALSE;
}


void thermostat_device_interface_cb(zb_uint8_t param)
{
  zb_zcl_device_callback_param_t *device_cb_param =
    ZB_BUF_GET_PARAM(param, zb_zcl_device_callback_param_t);

  TRACE_MSG(TRACE_APP1, ">>izs_device_interface_cb param %hd id %hd", (FMT__H_H,
      param, device_cb_param->device_cb_id));

  device_cb_param->status = RET_OK;

  switch (device_cb_param->device_cb_id)
  {
    case ZB_ZCL_THERMOSTAT_VALUE_CB_ID:
      TRACE_MSG(TRACE_APP1, "thermostat params values", (FMT__0));
      TRACE_MSG(TRACE_APP1, "mode = %hd",
                (FMT__D, device_cb_param->cb_param.thermostat_value_param.mode));
      TRACE_MSG(TRACE_APP1, "heat_setpoint = %hd",
                (FMT__D, device_cb_param->cb_param.thermostat_value_param.heat_setpoint));
      TRACE_MSG(TRACE_APP1, "cool_setpoint = %hd",
                (FMT__D, device_cb_param->cb_param.thermostat_value_param.cool_setpoint));
      break;
      /* >>>> Scenes */
    case ZB_ZCL_SCENES_REMOVE_ALL_SCENES_CB_ID:
    {
      const zb_zcl_scenes_remove_all_scenes_req_t *remove_all_scenes_req = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_scenes_remove_all_scenes_req_t);
      zb_uint8_t *remove_all_scenes_status = ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(param, zb_uint8_t);
      const zb_zcl_parsed_hdr_t *in_cmd_info = ZB_ZCL_DEVICE_CMD_PARAM_CMD_INFO(param);

      TRACE_MSG(TRACE_APP1, "ZB_ZCL_SCENES_REMOVE_ALL_SCENES_CB_ID: group_id 0x%x", (FMT__D, remove_all_scenes_req->group_id));

      if (!zb_aps_is_endpoint_in_group(
                 remove_all_scenes_req->group_id,
                 ZB_ZCL_PARSED_HDR_SHORT_DATA(in_cmd_info).dst_endpoint))
      {
        *remove_all_scenes_status = ZB_ZCL_STATUS_INVALID_FIELD;
      }
      else
      {
        test_device_scenes_remove_entries_by_group(remove_all_scenes_req->group_id);
        *remove_all_scenes_status = ZB_ZCL_STATUS_SUCCESS;
      }
    }
    break;
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
            scenes_table[idx].occupied_cooling_setpoint = occupied_cooling_setpoint;
            scenes_table[idx].occupied_heating_setpoint = occupied_heating_setpoint;
            scenes_table[idx].system_mode = system_mode;
          }
          else
          {
            /* Create new entry with empty name and 0 transition time */
            scenes_table[idx].common.group_id = store_scene_req->group_id;
            scenes_table[idx].common.scene_id = store_scene_req->scene_id;
            scenes_table[idx].common.transition_time = 0;
            scenes_table[idx].occupied_cooling_setpoint = occupied_cooling_setpoint;
            scenes_table[idx].occupied_heating_setpoint = occupied_heating_setpoint;
            scenes_table[idx].system_mode = system_mode;
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

      TRACE_MSG(TRACE_APP1, "ZB_ZCL_SCENES_REMOVE_SCENE_CB_ID: group_id 0x%x scene_id 0x%hd", (FMT__D_H, recall_scene_req->group_id, recall_scene_req->scene_id));

      idx = test_device_scenes_get_entry(recall_scene_req->group_id, recall_scene_req->scene_id);

      if (idx != 0xFF &&
          scenes_table[idx].common.group_id != ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD)
      {
        zb_zcl_attr_t *attr_desc;

        /* Recall this entry */
        attr_desc = zb_zcl_get_attr_desc_a(SRC_ENDPOINT, ZB_ZCL_CLUSTER_ID_THERMOSTAT,
                                       ZB_ZCL_CLUSTER_SERVER_ROLE,ZB_ZCL_ATTR_THERMOSTAT_OCCUPIED_COOLING_SETPOINT_ID);
        ZB_ZCL_SET_DIRECTLY_ATTR_VAL16(attr_desc, scenes_table[idx].occupied_cooling_setpoint);

        attr_desc = zb_zcl_get_attr_desc_a(SRC_ENDPOINT, ZB_ZCL_CLUSTER_ID_THERMOSTAT,
                                       ZB_ZCL_CLUSTER_SERVER_ROLE,ZB_ZCL_ATTR_THERMOSTAT_OCCUPIED_HEATING_SETPOINT_ID);
        ZB_ZCL_SET_DIRECTLY_ATTR_VAL16(attr_desc, scenes_table[idx].occupied_heating_setpoint);

         ZB_ZCL_SET_ATTRIBUTE(
          SRC_ENDPOINT,
          ZB_ZCL_CLUSTER_ID_THERMOSTAT,
          ZB_ZCL_CLUSTER_SERVER_ROLE,
          ZB_ZCL_ATTR_THERMOSTAT_SYSTEM_MODE_ID,
          &scenes_table[idx].system_mode,
          ZB_FALSE);

        *recall_scene_status = ZB_ZCL_STATUS_SUCCESS;
      }
      else
      {
        *recall_scene_status = ZB_ZCL_STATUS_NOT_FOUND;
      }
    }
    break;
    case ZB_ZCL_SCENES_INTERNAL_REMOVE_ALL_SCENES_ALL_ENDPOINTS_CB_ID:
    {
      const zb_zcl_scenes_remove_all_scenes_req_t *remove_all_scenes_req = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_scenes_remove_all_scenes_req_t);

      TRACE_MSG(TRACE_APP1, "ZB_ZCL_SCENES_INTERNAL_REMOVE_ALL_SCENES_ALL_ENDPOINTS_CB_ID: group_id 0x%x", (FMT__D, remove_all_scenes_req->group_id));

      /* Have only one endpoint */
      test_device_scenes_remove_entries_by_group(remove_all_scenes_req->group_id);
    }
    break;
    case ZB_ZCL_SCENES_INTERNAL_REMOVE_ALL_SCENES_ALL_ENDPOINTS_ALL_GROUPS_CB_ID:
    {
      test_device_scenes_table_init();
    }
    break;
      /* <<<< Scenes */
    default:
      device_cb_param->status = RET_ERROR;
      break;
  }

  TRACE_MSG(TRACE_APP1, "<<izs_device_interface_cb %hd", (FMT__H, device_cb_param->status));
}

/* Callback to handle the stack events */
void zboss_signal_handler(zb_uint8_t param)
{
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_t sig = zb_get_app_signal(param, &sg_p);

  TRACE_MSG(TRACE_APP1, ">>zboss_signal_handler: status %hd signal %hd",
            (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
        zb_bdb_finding_binding_target(SRC_ENDPOINT);
      break;

      case ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED:
        TRACE_MSG(TRACE_APP1, "Finding&binding TARGET done", (FMT__0));
        break;

      default:
        TRACE_MSG(TRACE_APP1, "Unknown signal %hd", (FMT__H, sig));
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

  TRACE_MSG(TRACE_APP1, "<<zboss_signal_handler", (FMT__0));
}
