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
/* ![mem_config_max] */
#define ZB_TRACE_FILE_ID 40218
#include "zboss_api.h"
#include "zb_led_button.h"
#include "zb_window_covering.h"
#ifdef ZB_USE_BUTTONS
#include "bulb_hal.h"
#endif

/* Insert that include before any code or declaration. */
#ifdef ZB_CONFIGURABLE_MEM
#include "zb_mem_config_max.h"
#endif
/* Next define clusters, attributes etc. */
/* ![mem_config_max] */

#define ZB_OUTPUT_ENDPOINT          5
#define ZB_OUTPUT_MAX_CMD_PAYLOAD_SIZE 2

/* Handler for specific zcl commands */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);
zb_ieee_addr_t g_zc_addr = {0x49, 0x70, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
void test_device_interface_cb(zb_uint8_t param);
void button_press_handler(zb_uint8_t param);
#define ENDPOINT_C 5
#define ENDPOINT_ED 10

/* Basic cluster attributes data */
zb_uint8_t g_attr_basic_zcl_version = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
zb_uint8_t g_attr_basic_application_version = ZB_ZCL_BASIC_APPLICATION_VERSION_DEFAULT_VALUE;
zb_uint8_t g_attr_basic_stack_version = ZB_ZCL_BASIC_STACK_VERSION_DEFAULT_VALUE;
zb_uint8_t g_attr_basic_hw_version = ZB_ZCL_BASIC_HW_VERSION_DEFAULT_VALUE;
zb_char_t g_attr_basic_manufacturer_name[40];
zb_char_t g_attr_basic_model_identifier[40];
zb_char_t g_attr_basic_date_code[] = ZB_ZCL_BASIC_DATE_CODE_DEFAULT_VALUE;
zb_uint8_t g_attr_basic_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;
zb_char_t g_attr_basic_location_description[] = ZB_ZCL_BASIC_LOCATION_DESCRIPTION_DEFAULT_VALUE;
zb_uint8_t g_attr_basic_physical_environment = ZB_ZCL_BASIC_PHYSICAL_ENVIRONMENT_DEFAULT_VALUE;
zb_char_t g_attr_sw_build_id[] = ZB_ZCL_BASIC_SW_BUILD_ID_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST_EXT(basic_attr_list, &g_attr_basic_zcl_version, &g_attr_basic_application_version, &g_attr_basic_stack_version, &g_attr_basic_hw_version, &g_attr_basic_manufacturer_name, &g_attr_basic_model_identifier, &g_attr_basic_date_code, &g_attr_basic_power_source, &g_attr_basic_location_description, &g_attr_basic_physical_environment, &g_attr_sw_build_id);
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
zb_uint8_t g_attr_window_covering_current_position_tilt_percentage = ZB_ZCL_WINDOW_COVERING_CURRENT_POSITION_TILT_PERCENTAGE_DEFAULT_VALUE;
zb_uint16_t g_attr_window_covering_installed_open_limit_lift = ZB_ZCL_WINDOW_COVERING_INSTALLED_OPEN_LIMIT_LIFT_DEFAULT_VALUE;
zb_uint16_t g_attr_window_covering_installed_closed_limit_lift = ZB_ZCL_WINDOW_COVERING_INSTALLED_CLOSED_LIMIT_LIFT_DEFAULT_VALUE;
zb_uint16_t g_attr_window_covering_installed_open_limit_tilt = ZB_ZCL_WINDOW_COVERING_INSTALLED_OPEN_LIMIT_TILT_DEFAULT_VALUE;
zb_uint16_t g_attr_window_covering_installed_closed_limit_tilt = ZB_ZCL_WINDOW_COVERING_INSTALLED_CLOSED_LIMIT_TILT_DEFAULT_VALUE;
zb_uint8_t g_attr_window_covering_mode = ZB_ZCL_WINDOW_COVERING_MODE_DEFAULT_VALUE;
/* Shade Configuration cluster attributes data */
zb_uint8_t g_attr_shade_config_status = ZB_ZCL_SHADE_CONFIG_STATUS_DEFAULT_VALUE;
zb_uint16_t g_attr_shade_config_closed_limit = ZB_ZCL_SHADE_CONFIG_CLOSED_LIMIT_DEFAULT_VALUE;
zb_uint8_t g_attr_shade_config_mode = ZB_ZCL_SHADE_CONFIG_MODE_DEFAULT_VALUE;
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_attr_identify_identify_time);
ZB_ZCL_DECLARE_GROUPS_ATTRIB_LIST(groups_attr_list, &g_attr_groups_name_support);
ZB_ZCL_DECLARE_SCENES_ATTRIB_LIST(scenes_attr_list, &g_attr_scenes_scene_count, &g_attr_scenes_current_scene, &g_attr_scenes_current_group, &g_attr_scenes_scene_valid, &g_attr_scenes_name_support);
ZB_ZCL_DECLARE_WINDOW_COVERING_CLUSTER_ATTRIB_LIST(window_covering_attr_list, &g_attr_window_covering_window_covering_type, &g_attr_window_covering_config_status, &g_attr_window_covering_current_position_lift_percentage, &g_attr_window_covering_current_position_tilt_percentage, &g_attr_window_covering_installed_open_limit_lift, &g_attr_window_covering_installed_closed_limit_lift, &g_attr_window_covering_installed_open_limit_tilt, &g_attr_window_covering_installed_closed_limit_tilt, &g_attr_window_covering_mode);
ZB_ZCL_DECLARE_SHADE_CONFIG_ATTRIB_LIST(shade_config_attr_list, &g_attr_shade_config_status, &g_attr_shade_config_closed_limit, &g_attr_shade_config_mode);
/********************* Declare device **************************/
ZB_HA_DECLARE_WINDOW_COVERING_CLUSTER_LIST_ZED(window_covering_clusters, basic_attr_list, identify_attr_list, groups_attr_list, scenes_attr_list, window_covering_attr_list, shade_config_attr_list);
ZB_HA_DECLARE_WINDOW_COVERING_EP_ZED(window_covering_ep, ENDPOINT_C, window_covering_clusters);
ZB_HA_DECLARE_WINDOW_COVERING_CTX_ZED(device_ctx, window_covering_ep);
zb_uint16_t g_dst_addr = 0;


#ifdef ZB_USE_NVRAM
zb_uint16_t bulb_get_nvram_data_size()
{
  return sizeof(bulb_device_nvram_dataset_t);
}

void bulb_nvram_read_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length)
{
  bulb_device_nvram_dataset_t ds;
  zb_ret_t ret;
//! [trace_msg]
  TRACE_MSG(TRACE_APP1, ">> bulb_nvram_read_app_data page %hd pos %d", (FMT__H_D, page, pos));
//! [trace_msg]
  ZB_ASSERT(payload_length == sizeof(ds));

  /* If we fail, trace is given and assertion is triggered */
  ret = zb_nvram_read_data(page, pos, (zb_uint8_t*)&ds, sizeof(ds));

  if (ret == RET_OK)
  {
    zb_zcl_attr_t *attr_desc;

    attr_desc = zb_zcl_get_attr_desc_a(ZB_OUTPUT_ENDPOINT, ZB_ZCL_CLUSTER_ID_ON_OFF,
                                       ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID);

    ZB_ZCL_SET_DIRECTLY_ATTR_VAL8(attr_desc, ds.onoff_state);

    attr_desc = zb_zcl_get_attr_desc_a(ZB_OUTPUT_ENDPOINT, ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL,
                                       ZB_ZCL_CLUSTER_SERVER_ROLE,ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID);

    ZB_ZCL_SET_DIRECTLY_ATTR_VAL16(attr_desc, ds.current_level);

    if (ds.onoff_state)
    {
#ifdef ZB_USE_BUTTONS
      bulb_hal_set_level(ds.current_level);
#endif
    }
    else
    {
#ifdef ZB_USE_BUTTONS
      bulb_hal_set_on_off(ZB_FALSE);
#endif
    }
  }

  TRACE_MSG(TRACE_APP1, "<< bulb_nvram_read_app_data ret %d", (FMT__D, ret));
}

zb_ret_t bulb_nvram_write_app_data(zb_uint8_t page, zb_uint32_t pos)
{
  zb_ret_t ret;
  bulb_device_nvram_dataset_t ds;
  zb_zcl_attr_t *attr_desc = zb_zcl_get_attr_desc_a(ZB_OUTPUT_ENDPOINT,ZB_ZCL_CLUSTER_ID_ON_OFF,
                                                    ZB_ZCL_CLUSTER_SERVER_ROLE,ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID);

  TRACE_MSG(TRACE_APP1, ">> bulb_nvram_write_app_data, page %hd, pos %d", (FMT__H_D, page, pos));

  ds.onoff_state = ZB_ZCL_GET_ATTRIBUTE_VAL_8(attr_desc);

  attr_desc = zb_zcl_get_attr_desc_a(ZB_OUTPUT_ENDPOINT, ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL,
                                     ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID);

  ds.current_level = ZB_ZCL_GET_ATTRIBUTE_VAL_8(attr_desc);

  /* If we fail, trace is given and assertion is triggered */
  ret = zb_nvram_write_data(page, pos, (zb_uint8_t*)&ds, sizeof(ds));

  TRACE_MSG(TRACE_APP1, "<< bulb_nvram_write_app_data, ret %d", (FMT__D, ret));

  return ret;
}
#endif /* ZB_USE_NVRAM */

void clusters_attr_init(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> clusters_attr_init", (FMT__0));
  ZVUNUSED(param);

  ZB_ZCL_SET_STRING_VAL(
    g_attr_basic_manufacturer_name,
    "qmotion",
    ZB_ZCL_STRING_CONST_SIZE("qmotion"));

  ZB_ZCL_SET_STRING_VAL(
    g_attr_basic_model_identifier,
    "rollershade internal battery",
    ZB_ZCL_STRING_CONST_SIZE("rollershade internal battery"));

  TRACE_MSG(TRACE_APP1, "<< clusters_attr_init", (FMT__0));
}

MAIN()
{
  ARGV_UNUSED;

#ifdef ZB_USE_BUTTONS
  bulb_hal_init();
#endif

  ZB_SET_TRACE_ON();
  ZB_SET_TRAF_DUMP_OFF();

  ZB_INIT("sample_zc");

  zb_set_long_address(g_zc_addr);
  //zb_set_network_coordinator_role(1l<<24);
  zb_set_network_ed_role(1l<<25);

#ifdef ZB_USE_BUTTONS
  /* Erase NVRAM if BUTTON2 is pressed on start */
  if (bulb_hal_is_button_pressed())
  {
    zb_set_nvram_erase_at_start(ZB_TRUE);
    bulb_hal_set_power(1);
    ZB_SCHEDULE_APP_ALARM(bulb_hal_set_power, 0, ZB_TIME_ONE_SECOND * 2);
  }
#else
  /*
  Do not erase NVRAM to save the network parameters after device reboot or power-off
  NOTE: If this option is set to ZB_FALSE then do full device erase for all network
  devices before running other samples.
  */
  zb_set_nvram_erase_at_start(ZB_FALSE);
#endif

#ifdef ZB_USE_NVRAM
  zb_nvram_register_app1_read_cb(bulb_nvram_read_app_data);
  zb_nvram_register_app1_write_cb(bulb_nvram_write_app_data, bulb_get_nvram_data_size);
#endif

  clusters_attr_init(0);
  //zb_set_max_children(1);
  //zb_set_pan_id(0x029a);

  zb_set_rx_on_when_idle(ZB_FALSE);

  /* Register device list */
  ZB_AF_REGISTER_DEVICE_CTX(&device_ctx);
  ZB_AF_SET_ENDPOINT_HANDLER(ZB_OUTPUT_ENDPOINT, zcl_specific_cluster_cmd_handler);
  ZB_ZCL_REGISTER_DEVICE_CB(test_device_interface_cb);


#ifdef ZB_USE_BUTTONS
  zb_button_register_handler(0, 0, button_press_handler);
#endif

  if (zboss_start_no_autostart() != RET_OK)
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
#ifdef ZB_USE_NVRAM
        /* If we fail, trace is given and assertion is triggered */
        (void)zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);
#endif
      }
      break;

    default:
      device_cb_param->status = RET_OK;
      break;
  }

  TRACE_MSG(TRACE_APP1, "< test_device_interface_cb %hd", (FMT__H, device_cb_param->status));
}

zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
  zb_bufid_t zcl_cmd_buf = param;
  zb_zcl_parsed_hdr_t cmd_info;
  zb_uint8_t lqi = ZB_MAC_LQI_UNDEFINED;
  zb_int8_t rssi = ZB_MAC_RSSI_UNDEFINED;

  TRACE_MSG(TRACE_APP1, "> zcl_specific_cluster_cmd_handler", (FMT__0));

  ZB_ZCL_COPY_PARSED_HEADER(zcl_cmd_buf, &cmd_info);

  g_dst_addr = ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).source.u.short_addr;

  ZB_ZCL_DEBUG_DUMP_HEADER(&cmd_info);
  TRACE_MSG(TRACE_APP3, "payload size: %i", (FMT__D, zb_buf_len(zcl_cmd_buf)));

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

void button_press_handler(zb_uint8_t param)
{
  ZVUNUSED(param);
  TRACE_MSG(TRACE_APP1, "button is pressed, do nothing", (FMT__0));
}

void light_control_leave_nwk(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ERROR, ">> light_control_leave_nwk param %hd", (FMT__H, param));

  /* We are going to leave */
  if (!param)
  {
    zb_buf_get_out_delayed(light_control_leave_nwk);
  }
  else
  {
    zb_bufid_t buf = param;
    zb_zdo_mgmt_leave_param_t *req_param;

    req_param = ZB_BUF_GET_PARAM(buf, zb_zdo_mgmt_leave_param_t);
    ZB_BZERO(req_param, sizeof(zb_zdo_mgmt_leave_param_t));

    /* Set dst_addr == local address for local leave */
    req_param->dst_addr = ZB_PIBCACHE_NETWORK_ADDRESS();
    zdo_mgmt_leave_req(param, NULL);
  }

  bulb_hal_set_connect(0);

  TRACE_MSG(TRACE_ERROR, "<< light_control_leave_nwk", (FMT__0));
}

void light_control_retry_join(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ERROR, "light_control_retry_join %hd", (FMT__H, param));
  if (param == ZB_NWK_LEAVE_TYPE_RESET)
  {
    bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
  }
}

void light_control_leave_and_join(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ERROR, ">> light_control_leave_and_join param %hd", (FMT__H, param));
  if (ZB_JOINED())
  {
    light_control_leave_nwk(param);
  }
  else
  {
    light_control_retry_join(ZB_NWK_LEAVE_TYPE_RESET);
    if (param)
    {
      zb_buf_free(param);
    }
  }
  TRACE_MSG(TRACE_ERROR, "<< light_control_leave_and_join", (FMT__0));
}

void zboss_signal_handler(zb_uint8_t param)
{
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_SKIP_STARTUP:
        bdb_start_top_level_commissioning(zb_bdb_is_factory_new() ? (zb_uint8_t )ZB_BDB_NETWORK_STEERING : 0);
        break;

      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_STEERING:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
        bulb_hal_set_connect(1);
        zb_zdo_pim_permit_turbo_poll(ZB_FALSE);
        zb_zdo_pim_set_long_poll_interval(1000);
        break;

      case ZB_COMMON_SIGNAL_CAN_SLEEP:
      {
        /* zb_zdo_signal_can_sleep_params_t *can_sleep_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_can_sleep_params_t); */
        //TRACE_MSG(TRACE_ERROR, "Can sleep for %ld ms", (FMT__L, can_sleep_params->sleep_tmo));
#ifdef ZB_USE_SLEEP
        zb_sleep_now();
#endif
        break;
      }

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
    ZB_SCHEDULE_APP_ALARM(light_control_leave_and_join, 0, ZB_TIME_ONE_SECOND);
    bulb_hal_set_connect(0);
  }

  if (param)
  {
    zb_buf_free(param);
  }
}
