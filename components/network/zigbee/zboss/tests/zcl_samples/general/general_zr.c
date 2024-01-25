/* ZBOSS Zigbee software protocol stack
 *
 * Copyright (c) 2012-2020 DSR Corporation, Denver CO, USA.
 * http://www.dsr-zboss.com
 * http://www.dsr-corporation.com
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
/* PURPOSE: Test sample with clusters On/Off, Basic, Identify, Groups,
   Scenes, On/Off Switch Config, Level Control, Power Config, Alarms, Time,
   Binary Input, Diagnostics, Poll Control, Meter Identification */

#define ZB_TRACE_FILE_ID 51359
#include "zboss_api.h"
#include "general_zr.h"

/* Insert that include before any code or declaration. */
#ifdef ZB_CONFIGURABLE_MEM
#include "zb_mem_config_max.h"
#endif

zb_uint16_t g_dst_addr = 0x00;

#define DST_ADDR g_dst_addr
#define DST_ADDR_MODE ZB_APS_ADDR_MODE_16_ENDP_PRESENT
#define DST_EP 5

/**
 * Global variables definitions
 */
zb_ieee_addr_t g_zc_addr = {0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb}; /* IEEE address of the
                                                                              * device */

/* Used endpoint */
#define ZB_SERVER_ENDPOINT          5
#define ZB_CLIENT_ENDPOINT          6

void test_loop(zb_bufid_t param);
void test_device_cb(zb_uint8_t param);

/**
 * Declaring attributes for each cluster
 */
/* On/Off cluster attributes */
zb_uint8_t g_attr_on_off = ZB_ZCL_ON_OFF_ON_OFF_DEFAULT_VALUE;
zb_bool_t g_attr_global_scene_ctrl  = ZB_TRUE;
zb_uint16_t g_attr_on_time  = 0;
zb_uint16_t g_attr_off_wait_time  = 0;
ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST_EXT(on_off_attr_list, &g_attr_on_off,
    &g_attr_global_scene_ctrl, &g_attr_on_time, &g_attr_off_wait_time);

/* Basic cluster attributes */
zb_uint8_t g_attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
zb_uint8_t g_attr_app_version = ZB_ZCL_BASIC_APPLICATION_VERSION_DEFAULT_VALUE;
zb_uint8_t g_attr_stack_version = ZB_ZCL_BASIC_STACK_VERSION_DEFAULT_VALUE;
zb_uint8_t g_attr_hardware_version = ZB_ZCL_BASIC_HW_VERSION_DEFAULT_VALUE;
zb_char_t g_attr_manufacturer_name[] = ZB_ZCL_BASIC_MANUFACTURER_NAME_DEFAULT_VALUE;
zb_char_t g_attr_model_id[] = ZB_ZCL_BASIC_MODEL_IDENTIFIER_DEFAULT_VALUE;
zb_char_t g_attr_date_code[] = ZB_ZCL_BASIC_DATE_CODE_DEFAULT_VALUE;
zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;
zb_char_t g_attr_location_id[] = ZB_ZCL_BASIC_LOCATION_DESCRIPTION_DEFAULT_VALUE;
zb_uint8_t g_attr_ph_env = ZB_ZCL_BASIC_PHYSICAL_ENVIRONMENT_DEFAULT_VALUE;
zb_char_t g_attr_sw_build_id[] = ZB_ZCL_BASIC_SW_BUILD_ID_DEFAULT_VALUE;
ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST_EXT(basic_attr_list, &g_attr_zcl_version, &g_attr_app_version, &g_attr_stack_version,
                                      &g_attr_hardware_version, g_attr_manufacturer_name, g_attr_model_id,
                                      g_attr_date_code, &g_attr_power_source, g_attr_location_id, &g_attr_ph_env,
                                      g_attr_sw_build_id);


/* Identify cluster attributes */
zb_uint16_t g_attr_identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_attr_identify_time);

/* Groups cluster attributes */
zb_uint8_t g_attr_name_support = 0;
ZB_ZCL_DECLARE_GROUPS_ATTRIB_LIST(groups_attr_list, &g_attr_name_support);

/* Scenes cluster attributes */
zb_uint8_t g_attr_scenes_scene_count = ZB_ZCL_SCENES_SCENE_COUNT_DEFAULT_VALUE;
zb_uint8_t g_attr_scenes_current_scene = ZB_ZCL_SCENES_CURRENT_SCENE_DEFAULT_VALUE;
zb_uint16_t g_attr_scenes_current_group = ZB_ZCL_SCENES_CURRENT_GROUP_DEFAULT_VALUE;
zb_uint8_t g_attr_scenes_scene_valid = ZB_ZCL_SCENES_SCENE_VALID_DEFAULT_VALUE;
zb_uint16_t g_attr_scenes_name_support = ZB_ZCL_SCENES_NAME_SUPPORT_DEFAULT_VALUE;
ZB_ZCL_DECLARE_SCENES_ATTRIB_LIST(scenes_attr_list, &g_attr_scenes_scene_count,
    &g_attr_scenes_current_scene, &g_attr_scenes_current_group,
    &g_attr_scenes_scene_valid, &g_attr_scenes_name_support);

/* On/Off Switch Configuration cluster attributes */
zb_uint8_t g_attr_on_off_switch_configuration_switch_type = ZB_ZCL_ON_OFF_SWITCH_CONFIGURATION_SWITCH_ACTIONS_TYPE1;
zb_uint8_t g_attr_on_off_switch_configuration_switch_actions = ZB_ZCL_ON_OFF_SWITCH_CONFIGURATION_SWITCH_ACTIONS_DEFAULT_VALUE;
ZB_ZCL_DECLARE_ON_OFF_SWITCH_CONFIGURATION_ATTRIB_LIST(on_off_switch_configuration_attr_list,
  &g_attr_on_off_switch_configuration_switch_type,
  &g_attr_on_off_switch_configuration_switch_actions);

/* Level Control cluster attributes */
zb_uint8_t g_attr_level_control_current_level= ZB_ZCL_LEVEL_CONTROL_CURRENT_LEVEL_DEFAULT_VALUE;
zb_uint16_t g_attr_level_control_remaining_time = ZB_ZCL_LEVEL_CONTROL_REMAINING_TIME_DEFAULT_VALUE;
zb_uint8_t g_attr_start_up_current_level= 0;
zb_uint8_t g_attr_options = ZB_ZCL_LEVEL_CONTROL_OPTIONS_DEFAULT_VALUE;
ZB_ZCL_DECLARE_LEVEL_CONTROL_ATTRIB_LIST_EXT(level_control_attr_list, &g_attr_level_control_current_level,
                                    &g_attr_level_control_remaining_time, &g_attr_start_up_current_level, &g_attr_options);

/* Power Configuration Battery Cluster attributes */
zb_uint8_t g_attr_power_config_battery_voltage = 0; /* Missing DEFAULT value */
zb_uint8_t g_attr_power_battery_size = ZB_ZCL_POWER_CONFIG_BATTERY_SIZE_DEFAULT_VALUE;
zb_uint8_t g_attr_power_battery_quantity = 0; /* Missing DEFAULT value */
zb_uint8_t g_attr_power_battery_rated_voltage = 0; /* Missing DEFAULT value; */
zb_uint8_t g_attr_power_battery_alarm_mask = ZB_ZCL_POWER_CONFIG_BATTERY_ALARM_MASK_DEFAULT_VALUE;
zb_uint8_t g_attr_power_battery_voltage_min_threshold = ZB_ZCL_POWER_CONFIG_BATTERY_VOLTAGE_MIN_THRESHOLD_DEFAULT_VALUE;
zb_uint8_t g_attr_power_battery_percentage_remaining = ZB_ZCL_POWER_CONFIG_BATTERY_REMAINING_HA_DEFAULT_VALUE;
zb_uint8_t g_attr_power_battery_voltage_threshold_1 = ZB_ZCL_POWER_CONFIG_BATTERY_VOLTAGE_THRESHOLD1_DEFAULT_VALUE;
zb_uint8_t g_attr_power_battery_voltage_threshold_2 = ZB_ZCL_POWER_CONFIG_BATTERY_VOLTAGE_THRESHOLD2_DEFAULT_VALUE;
zb_uint8_t g_attr_power_battery_voltage_threshold_3 = ZB_ZCL_POWER_CONFIG_BATTERY_VOLTAGE_THRESHOLD3_DEFAULT_VALUE;
zb_uint8_t g_attr_power_battery_percentage_min_threshold = ZB_ZCL_POWER_CONFIG_BATTERY_PERCENTAGE_MIN_THRESHOLD_DEFAULT_VALUE;
zb_uint8_t g_attr_power_battery_percentage_threshold_1 = ZB_ZCL_POWER_CONFIG_BATTERY_VOLTAGE_THRESHOLD1_DEFAULT_VALUE;
zb_uint8_t g_attr_power_battery_percentage_threshold_2 = ZB_ZCL_POWER_CONFIG_BATTERY_VOLTAGE_THRESHOLD2_DEFAULT_VALUE;
zb_uint8_t g_attr_power_battery_percentage_threshold_3 = ZB_ZCL_POWER_CONFIG_BATTERY_VOLTAGE_THRESHOLD3_DEFAULT_VALUE;
zb_uint32_t g_attr_power_battery_alarm_state = ZB_ZCL_POWER_CONFIG_BATTERY_ALARM_STATE_DEFAULT_VALUE;

/* Define 'bat_num' as empty in order to declare default battery set attributes. */
/* According to Table 3-17 of ZCL specification, defining 'bat_num' as 2 or 3 allows */
/* to declare battery set attributes for BATTERY2 and BATTERY3 */
#define bat_num

ZB_ZCL_DECLARE_POWER_CONFIG_BATTERY_ATTRIB_LIST_EXT(power_config_battery_attr_list, &g_attr_power_config_battery_voltage,
 &g_attr_power_battery_size, &g_attr_power_battery_quantity, &g_attr_power_battery_rated_voltage, &g_attr_power_battery_alarm_mask,
 &g_attr_power_battery_voltage_min_threshold, &g_attr_power_battery_percentage_remaining, &g_attr_power_battery_voltage_threshold_1,
 &g_attr_power_battery_voltage_threshold_2, &g_attr_power_battery_voltage_threshold_3, &g_attr_power_battery_percentage_min_threshold,
 &g_attr_power_battery_percentage_threshold_1, &g_attr_power_battery_percentage_threshold_2, &g_attr_power_battery_percentage_threshold_3,
 &g_attr_power_battery_alarm_state);

/* Power Configuration Mains Cluster attributes */
zb_uint16_t g_attr_power_config_mains_voltage = 0; /* Missing DEFAULT value; */
zb_uint8_t g_attr_power_config_mains_frequency = 0; /* Missing DEFAULT value; */
zb_uint8_t g_attr_power_config_mains_alarm_mask = ZB_ZCL_POWER_CONFIG_MAINS_ALARM_MASK_DEFAULT_VALUE;
zb_uint16_t g_attr_power_config_mains_voltage_min_threshold = ZB_ZCL_POWER_CONFIG_MAINS_VOLTAGE_MIN_THRESHOLD_DEFAULT_VALUE;
zb_uint16_t g_attr_power_config_mains_voltage_max_threshold = ZB_ZCL_POWER_CONFIG_MAINS_VOLTAGE_MAX_THRESHOLD_DEFAULT_VALUE;
zb_uint16_t g_attr_power_config_mains_voltage_dwell_trip_point = ZB_ZCL_POWER_CONFIG_MAINS_DWELL_TRIP_POINT_DEFAULT_VALUE;
ZB_ZCL_DECLARE_POWER_CONFIG_MAINS_ATTRIB_LIST(power_config_mains_attr_list, &g_attr_power_config_mains_voltage,
 &g_attr_power_config_mains_frequency, &g_attr_power_config_mains_alarm_mask, &g_attr_power_config_mains_voltage_min_threshold,
 &g_attr_power_config_mains_voltage_max_threshold, &g_attr_power_config_mains_voltage_dwell_trip_point);


/* Alarms cluster attributes */
zb_uint16_t g_attr_alarm_count  = 0; /* Missing default value */
ZB_ZCL_DECLARE_ALARMS_ATTRIB_LIST(alarms_attr_list, &g_attr_alarm_count);

/* Time cluster attributes */
zb_uint32_t g_attr_time = ZB_ZCL_TIME_TIME_DEFAULT_VALUE;
zb_uint8_t g_attr_time_status = ZB_ZCL_TIME_TIME_STATUS_DEFAULT_VALUE;
zb_int32_t g_attr_time_zone = ZB_ZCL_TIME_TIME_ZONE_DEFAULT_VALUE;
zb_uint32_t g_attr_dst_start = ZB_ZCL_TIME_DST_START_DEFAULT_VALUE;
zb_uint32_t g_attr_dst_end = ZB_ZCL_TIME_DST_END_DEFAULT_VALUE;
zb_int32_t g_attr_dst_shift = ZB_ZCL_TIME_DST_SHIFT_DEFAULT_VALUE;
zb_uint32_t g_attr_standard_time = ZB_ZCL_TIME_STANDARD_TIME_DEFAULT_VALUE;
zb_uint32_t g_attr_local_time = ZB_ZCL_TIME_LOCAL_TIME_DEFAULT_VALUE;
zb_uint32_t g_attr_last_set_time = ZB_ZCL_TIME_LAST_SET_TIME_DEFAULT_VALUE;
zb_uint32_t g_attr_valid_until_time = ZB_ZCL_TIME_VALID_UNTIL_TIME_DEFAULT_VALUE;
ZB_ZCL_DECLARE_TIME_ATTRIB_LIST(time_attr_list, &g_attr_time, &g_attr_time_status, &g_attr_time_zone,
                                &g_attr_dst_start, &g_attr_dst_end, &g_attr_dst_shift, &g_attr_standard_time,
                                &g_attr_local_time, &g_attr_last_set_time, &g_attr_valid_until_time);

/* Binary Input attributes: */
zb_uint8_t g_attr_out_of_service = 0;
zb_uint8_t g_attr_present_value = 0;
zb_uint8_t g_attr_status_flag = 0;
ZB_ZCL_DECLARE_BINARY_INPUT_ATTRIB_LIST(binary_input_attr_list, &g_attr_out_of_service, &g_attr_present_value, &g_attr_status_flag);

/* Diagnostics cluster attributes: */
ZB_ZCL_DECLARE_DIAGNOSTICS_ATTRIB_LIST(diagonostics_attr_list);

/* Poll Control clusber attributes: */
zb_uint32_t g_attr_checkin_interval = ZB_ZCL_POLL_CONTROL_CHECKIN_INTERVAL_DEFAULT_VALUE;
zb_uint32_t g_attr_long_poll_interval = ZB_ZCL_POLL_CONTROL_LONG_POLL_INTERVAL_DEFAULT_VALUE;
zb_uint16_t g_attr_short_poll_interval = ZB_ZCL_POLL_CONTROL_SHORT_POLL_INTERVAL_DEFAULT_VALUE;
zb_uint16_t g_attr_fast_poll_timeout = ZB_ZCL_POLL_CONTROL_FAST_POLL_TIMEOUT_DEFAULT_VALUE;
zb_uint32_t g_attr_checkin_interval_min = ZB_ZCL_POLL_CONTROL_CHECKIN_INTERVAL_MIN_DEFAULT_VALUE;
zb_uint32_t g_attr_long_poll_interval_min = ZB_ZCL_POLL_CONTROL_LONG_POLL_INTERVAL_MIN_DEFAULT_VALUE;
zb_uint16_t g_attr_fast_poll_timeout_max = ZB_ZCL_POLL_CONTROL_FAST_POLL_TIMEOUT_MAX_DEFAULT_VALUE;
ZB_ZCL_DECLARE_POLL_CONTROL_ATTRIB_LIST(poll_control_attr_list, &g_attr_checkin_interval, &g_attr_long_poll_interval,
                                      &g_attr_short_poll_interval, &g_attr_fast_poll_timeout, &g_attr_checkin_interval_min,
                                      &g_attr_long_poll_interval_min, &g_attr_fast_poll_timeout_max);

/* Meter Identification cluster attributes: */
zb_char_t g_attr_company_name[] = ZB_ZCl_ATTR_METER_IDENTIFICATION_COMPANY_NAME_DEFAULT_VALUE;
zb_uint16_t g_attr_meter_type_id = ZB_ZCl_ATTR_METER_IDENTIFICATION_METER_TYPE_ID_DEFAULT_VALUE;
zb_uint16_t g_attr_data_quality_id = ZB_ZCl_ATTR_METER_IDENTIFICATION_DATA_QUALITY_ID_DEFAULT_VALUE;
zb_char_t g_attr_pod[] = ZB_ZCl_ATTR_METER_IDENTIFICATION_POD_DEFAULT_VALUE;
zb_uint24_t g_attr_available_power = ZB_ZCl_ATTR_METER_IDENTIFICATION_AVAILABLE_POWER_DEFAULT_VALUE;
zb_uint24_t g_attr_power_threshold = ZB_ZCl_ATTR_METER_IDENTIFICATION_POWER_THRESHOLD_DEFAULT_VALUE;
ZB_ZCL_DECLARE_METER_IDENTIFICATION_ATTRIB_LIST(meter_identification_attr_list,  g_attr_company_name,  &g_attr_meter_type_id,
                                      &g_attr_data_quality_id,  &g_attr_pod,  &g_attr_available_power,  &g_attr_power_threshold);


/* Declare cluster list for the device */
ZB_DECLARE_GENERAL_SERVER_CLUSTER_LIST(general_server_clusters,
                                       on_off_attr_list,
                                       basic_attr_list,
                                       identify_attr_list,
                                       groups_attr_list,
                                       scenes_attr_list,
                                       on_off_switch_configuration_attr_list,
                                       level_control_attr_list,
                                       power_config_mains_attr_list,
                                       power_config_battery_attr_list,
                                       alarms_attr_list,
                                       time_attr_list,
                                       binary_input_attr_list,
                                       diagonostics_attr_list,
                                       poll_control_attr_list,
                                       meter_identification_attr_list);
/* Declare server endpoint */
ZB_DECLARE_GENERAL_SERVER_EP(general_server_ep, ZB_SERVER_ENDPOINT, general_server_clusters);

ZB_DECLARE_GENERAL_CLIENT_CLUSTER_LIST(general_client_clusters);
/* Declare client endpoint */
ZB_DECLARE_GENERAL_CLIENT_EP(general_client_ep, ZB_CLIENT_ENDPOINT, general_client_clusters);

/* Declare application's device context for single-endpoint device */
ZB_DECLARE_GENERAL_CTX(general_output_ctx, general_server_ep, general_client_ep);


MAIN()
{
  ARGV_UNUSED;

  /* Trace enable */
  ZB_SET_TRACE_ON();
  /* Traffic dump enable*/
  ZB_SET_TRAF_DUMP_ON();

  /* Global ZBOSS initialization */
  ZB_INIT("general_zr");

  /* Set up defaults for the commissioning */
  zb_set_long_address(g_zc_addr);
  zb_set_network_router_role(1l<<22);

  zb_set_nvram_erase_at_start(ZB_FALSE);
  zb_set_max_children(1);

  /* [af_register_device_context] */
  /* Register device ZCL context */
  ZB_AF_REGISTER_DEVICE_CTX(&general_output_ctx);
  ZB_ZCL_REGISTER_DEVICE_CB(test_device_cb);

  /* Initiate the stack start without starting the commissioning */
  if (zboss_start_no_autostart() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
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

void test_loop(zb_bufid_t param)
{
  static zb_uint8_t test_step = 0;
  zb_uint8_t *cmd_ptr;

  TRACE_MSG(TRACE_APP1, ">> test_loop test_step=%hd", (FMT__H, test_step));

  if (param == 0)
  {
    zb_buf_get_out_delayed(test_loop);
  }

  else{
    switch(test_step)
    {
      case 0:
        ZB_ZCL_BASIC_SEND_RESET_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP,
                                  ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                  ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
        break;

      case 1:
        ZB_ZCL_IDENTIFY_SEND_TRIGGER_VARIANT_REQ(param, DST_ADDR, DST_ADDR_MODE,
                                  DST_EP, ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                  ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, 0 ,0);
        break;

      case 2:
        ZB_ZCL_IDENTIFY_SEND_IDENTIFY_REQ(param, 0, DST_ADDR, DST_ADDR_MODE,
                                    DST_EP, ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                    ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
        break;

      case 3:
        ZB_ZCL_IDENTIFY_SEND_IDENTIFY_QUERY_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP,
                                          ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                          ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
        break;

      case 4:
        ZB_ZCL_GROUPS_SEND_ADD_GROUP_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP,
                                        ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                        ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, 0);
        break;

      case 5:
        ZB_ZCL_GROUPS_SEND_VIEW_GROUP_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP,
                                        ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                        ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, 0);
        break;

      case 6:
        ZB_ZCL_GROUPS_INIT_GET_GROUP_MEMBERSHIP_REQ(param, cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, 0);
        ZB_ZCL_GROUPS_SEND_GET_GROUP_MEMBERSHIP_REQ(param, cmd_ptr, DST_ADDR, DST_ADDR_MODE, DST_EP,
                                                    ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID, NULL);
        break;

      case 7:
        ZB_ZCL_GROUPS_SEND_REMOVE_GROUP_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                            ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, 0);
        break;

      case 8:
        ZB_ZCL_GROUPS_SEND_REMOVE_ALL_GROUPS_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                                ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
        break;

      case 9:
        ZB_ZCL_GROUPS_SEND_ADD_GROUP_IF_IDENT_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                                ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, 0);
        break;

      case 10:
        ZB_ZCL_SCENES_INIT_ADD_SCENE_REQ(param, cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, 0, 0, 0);
        ZB_ZCL_SCENES_SEND_ADD_SCENE_REQ(param, cmd_ptr, DST_ADDR, DST_EP, ZB_CLIENT_ENDPOINT,
                                        ZB_AF_HA_PROFILE_ID, NULL);
        break;

      case 11:
        ZB_ZCL_SCENES_SEND_VIEW_SCENE_REQ(param, DST_ADDR, DST_EP, ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                          ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, 0, 0);
        break;

      case 12:
        ZB_ZCL_SCENES_SEND_REMOVE_SCENE_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                          ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL,
                                          0, 0);
        break;

      case 13:
        ZB_ZCL_SCENES_SEND_REMOVE_ALL_SCENES_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                                ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL,
                                                0);
        break;

      case 14:
        ZB_ZCL_SCENES_SEND_STORE_SCENE_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                          ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, 0, 0);
        break;

      case 15:
        ZB_ZCL_SCENES_SEND_RECALL_SCENE_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                          ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, 0, 0);
        break;

      case 16:
        ZB_ZCL_SCENES_SEND_GET_SCENE_MEMBERSHIP_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                                ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, 0);
        break;

      case 17:
        ZB_ZCL_ON_OFF_SEND_OFF_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                              ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
        break;

      case 18:
        ZB_ZCL_ON_OFF_SEND_ON_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                              ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
        break;

      case 19:
        ZB_ZCL_ON_OFF_SEND_TOGGLE_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                    ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
        break;

      case 20:
        ZB_ZCL_ON_OFF_SEND_OFF_WITH_EFFECT_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                    ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, 0, 0, NULL);
        break;

      case 21:
        ZB_ZCL_ON_OFF_SEND_ON_WITH_RECALL_GLOBAL_SCENE_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP,
                                                        ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                                        ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
        break;

      case 22:
        ZB_ZCL_ON_OFF_SEND_ON_WITH_TIMED_OFF_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                                ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, ZB_TRUE,
                                                10, 10, NULL);
        break;

      case 23:
        ZB_ZCL_LEVEL_CONTROL_SEND_MOVE_TO_LEVEL_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                                  ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, 0, 0);
        break;

      case 24:
        ZB_ZCL_LEVEL_CONTROL_SEND_MOVE_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                          ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, 0, 0);
        break;

      case 25:
        ZB_ZCL_LEVEL_CONTROL_SEND_STEP_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                          ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, 0,
                                          0, 0);
        break;

      case 26:
        ZB_ZCL_LEVEL_CONTROL_SEND_STOP_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                          ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
        break;

      case 27:
        ZB_ZCL_LEVEL_CONTROL_SEND_MOVE_TO_LEVEL_WITH_ON_OFF_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP,
                                                              ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                                              ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL,
                                                              0, 0);
        break;

      case 28:
        ZB_ZCL_LEVEL_CONTROL_SEND_MOVE_WITH_ON_OFF_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP,
                                                    ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                                    ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL,
                                                    ZB_ZCL_LEVEL_CONTROL_MOVE_MODE_DOWN, 0);
        break;

      case 29:
        ZB_ZCL_LEVEL_CONTROL_SEND_STEP_WITH_ON_OFF_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP,
                                                      ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                                      ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL,
                                                      ZB_ZCL_LEVEL_CONTROL_STEP_MODE_DOWN, 0,
                                                      0);
        break;

      case 30:
        ZB_ZCL_LEVEL_CONTROL_SEND_STOP_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                            ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
        break;

      case 31:
        ZB_ZCL_ALARMS_SEND_RESET_ALARM_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP,
                                  ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                  ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL,0, ZB_ZCL_CLUSTER_ID_ALARMS);
        break;

      case 32:
        ZB_ZCL_ALARMS_SEND_RESET_ALL_ALARMS_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP,
                                  ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                  ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
        break;

      case 33:
        ZB_ZCL_ALARMS_SEND_GET_ALARM_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP,
                                  ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                  ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
        break;

      case 34:
        ZB_ZCL_ALARMS_SEND_RESET_ALARM_LOG_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP,
                                  ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                  ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
        break;

      case 35:
        ZB_ZCL_POLL_CONTROL_SEND_CHECK_IN_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP,
                                  ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID, NULL);
        break;
    }

    test_step++;

    if(test_step <= 35)
    {
      ZB_SCHEDULE_APP_ALARM(test_loop, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(50));
    }
  }

  TRACE_MSG(TRACE_APP1, "<< test_loop", (FMT__0));
}

/* Callback to handle the stack events */
void zboss_signal_handler(zb_uint8_t param)
{
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_SKIP_STARTUP:
        zboss_start_continue();
        break;

      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
          ZB_SCHEDULE_APP_ALARM(test_loop, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(50));
          /* Avoid freeing param */
          param = 0;
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
    TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d sig %d", (FMT__D_D, ZB_GET_APP_SIGNAL_STATUS(param), sig));
  }

  /* Free the buffer if it is not used */
  if (param)
  {
    zb_buf_free(param);
  }
}

void test_device_cb(zb_uint8_t param)
{
  zb_bufid_t buffer = param;
  zb_zcl_device_callback_param_t *device_cb_param = ZB_BUF_GET_PARAM(buffer, zb_zcl_device_callback_param_t);

  TRACE_MSG(TRACE_APP1, "> test_device_cb param %hd id %hd", (FMT__H_H, param, device_cb_param->device_cb_id));

  switch (device_cb_param->device_cb_id)
  {
    case ZB_ZCL_ALARMS_RESET_ALARM_CB_ID:
      device_cb_param->status = RET_OK;
      break;

    case ZB_ZCL_ALARMS_RESET_ALL_ALARMS_CB_ID:
      device_cb_param->status = RET_OK;
      break;

    default:
      TRACE_MSG(TRACE_APP1, "nothing to do, skip", (FMT__0));
      break;
  }
  TRACE_MSG(TRACE_APP1, "< test_device_cb %hd", (FMT__H, device_cb_param->status));
}