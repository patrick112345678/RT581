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
#define ZB_TRACE_FILE_ID 40226
#include "zboss_api.h"
#include "zb_led_button.h"
#include "zb_ha_test_sample_1.h"

#ifdef ZTT_USER_INTERACTION
/* removed nordic dependent headers */
#endif

/* Insert that include before any code or declaration. */
#ifdef ZB_CONFIGURABLE_MEM
#include "zb_mem_config_max.h"
#endif
/* Next define clusters, attributes etc. */
/* ![mem_config_max] */

#include "../common/zcl_basic_attr_list.h"

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
#define BUTTON_1 ZB_BOARD_BUTTON_1

/* Handler for specific zcl commands */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);
/** Test step enumeration. */
enum test_step_e
{
  TEST_STEP_ALARMS_SEND_ALARM_RES, /** Send Alarm command to Alarms cluster */
  TEST_STEP_FINISHED    /**< Test finished pseudo-step. */
};
/** Next test step initiator. */
void test_next_step(zb_uint8_t param);
zb_ieee_addr_t g_zc_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
void test_device_interface_cb(zb_uint8_t param);
void button_press_handler(zb_uint8_t param);
#define ENDPOINT_C 5
#define ENDPOINT_ED 10

/* Identify cluster attributes data */
zb_uint16_t g_attr_identify_identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;
/* Time cluster attributes data */
zb_time_t g_attr_time_time = ZB_ZCL_TIME_TIME_MIN_VALUE;
zb_uint8_t g_attr_time_time_status = ZB_ZCL_TIME_TIME_STATUS_DEFAULT_VALUE;
zb_int32_t g_attr_time_time_zone = ZB_ZCL_TIME_TIME_ZONE_DEFAULT_VALUE;
zb_uint32_t g_attr_time_dst_start = ZB_ZCL_TIME_TIME_MIN_VALUE;
zb_uint32_t g_attr_time_dst_end = ZB_ZCL_TIME_TIME_MIN_VALUE;
zb_int32_t g_attr_time_dst_shift = ZB_ZCL_TIME_DST_SHIFT_DEFAULT_VALUE;
zb_uint32_t g_attr_time_standard_time = ZB_ZCL_TIME_STANDARD_TIME_DEFAULT_VALUE;
zb_uint32_t g_attr_time_local_time = ZB_ZCL_TIME_LOCAL_TIME_DEFAULT_VALUE;
zb_time_t g_attr_time_last_set_time = ZB_ZCL_TIME_LAST_SET_TIME_DEFAULT_VALUE;
zb_time_t g_attr_time_valid_until_time = ZB_ZCL_TIME_VALID_UNTIL_TIME_DEFAULT_VALUE;
/* Illuminance Measurement cluster attributes data */
zb_uint16_t g_attr_illuminance_measurement_measured_value = (ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MIN_MEASURED_VALUE_MIN_VALUE + ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MAX_MEASURED_VALUE_MAX_VALUE)/2;
zb_uint16_t g_attr_illuminance_measurement_min_measured_value = ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MIN_MEASURED_VALUE_MIN_VALUE;
zb_uint16_t g_attr_illuminance_measurement_max_measured_value = ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MAX_MEASURED_VALUE_MAX_VALUE;
/* Temperature Measurement cluster attributes data */
zb_int16_t g_attr_temperature_measurement_measured_value = 0x6500;
zb_int16_t g_attr_temperature_measurement_min_measured_value = 0x6000;
zb_int16_t g_attr_temperature_measurement_max_measured_value = 0x7000;
zb_uint16_t g_attr_temperature_measurement_tolerance;
/* Occupancy Sensing cluster attributes data */
zb_uint8_t g_attr_occupancy_sensing_occupancy;
enum zb_zcl_occupancy_sensing_occupancy_sensor_type_e g_attr_occupancy_sensing_occupancy_sensor_type;
zb_uint8_t g_attr_occupancy_sensing_occupancy_sensor_type_bitmap;
/* Dehumidification Control cluster attributes data */
zb_uint8_t g_attr_dehumidification_control_dehumidification_cooling = 0;
zb_uint8_t g_attr_dehumidification_control_rhdehumidification_setpoint = ZB_ZCL_DEHUMIDIFICATION_CONTROL_RHDEHUMIDIFICATION_SETPOINT_DEFAULT_VALUE;
zb_uint8_t g_attr_dehumidification_control_dehumidification_hysteresis = ZB_ZCL_DEHUMIDIFICATION_CONTROL_DEHUMIDIFICATION_HYSTERESIS_DEFAULT_VALUE;
zb_uint8_t g_attr_dehumidification_control_dehumidification_max_cool = ZB_ZCL_DEHUMIDIFICATION_CONTROL_DEHUMIDIFICATION_MAX_COOL_DEFAULT_VALUE;
/* On/Off cluster attributes data */
zb_uint8_t g_attr_on_off = ZB_ZCL_ON_OFF_ON_OFF_DEFAULT_VALUE;
ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(on_off_attr_list, &g_attr_on_off);
#define ZB_ZCL_ALARMS_ALARM_COUNT_MAX_VALUE 2
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_attr_identify_identify_time);
ZB_ZCL_DECLARE_TIME_ATTRIB_LIST(time_attr_list, &g_attr_time_time, &g_attr_time_time_status, &g_attr_time_time_zone, &g_attr_time_dst_start, &g_attr_time_dst_end, &g_attr_time_dst_shift, &g_attr_time_standard_time, &g_attr_time_local_time, &g_attr_time_last_set_time, &g_attr_time_valid_until_time);
ZB_ZCL_DECLARE_DEHUMIDIFICATION_CONTROL_ATTRIB_LIST(dehumidification_control_attr_list, &g_attr_dehumidification_control_dehumidification_cooling, &g_attr_dehumidification_control_rhdehumidification_setpoint, &g_attr_dehumidification_control_dehumidification_hysteresis, &g_attr_dehumidification_control_dehumidification_max_cool);
ZB_ZCL_DECLARE_DIAGNOSTICS_ATTRIB_LIST(diagnostics_attr_list);
ZB_ZCL_DECLARE_ILLUMINANCE_MEASUREMENT_ATTRIB_LIST(illuminance_measurement_attr_list, &g_attr_illuminance_measurement_measured_value, &g_attr_illuminance_measurement_min_measured_value, &g_attr_illuminance_measurement_max_measured_value);
ZB_ZCL_DECLARE_TEMP_MEASUREMENT_ATTRIB_LIST(temp_measurement_attr_list, &g_attr_temperature_measurement_measured_value, &g_attr_temperature_measurement_min_measured_value, &g_attr_temperature_measurement_max_measured_value, &g_attr_temperature_measurement_tolerance);
ZB_ZCL_DECLARE_OCCUPANCY_SENSING_ATTRIB_LIST(occupancy_sensing_attr_list, &g_attr_occupancy_sensing_occupancy, &g_attr_occupancy_sensing_occupancy_sensor_type, &g_attr_occupancy_sensing_occupancy_sensor_type_bitmap);
/********************* Declare device **************************/
ZB_HA_DECLARE_HA_TEST_SAMPLE_1_CLUSTER_LIST_EXT(ha_test_sample_1_clusters, basic_attr_list, identify_attr_list, time_attr_list, dehumidification_control_attr_list, diagnostics_attr_list, illuminance_measurement_attr_list, temp_measurement_attr_list, occupancy_sensing_attr_list, on_off_attr_list);
ZB_HA_DECLARE_HA_TEST_SAMPLE_1_EP_EXT(ha_test_sample_1_ep, ENDPOINT_C, ha_test_sample_1_clusters);
ZB_HA_DECLARE_HA_TEST_SAMPLE_1_CTX(device_ctx, ha_test_sample_1_ep);
zb_uint16_t g_dst_addr = 0;
#define DST_ADDR g_dst_addr
#define DST_ADDR_MODE ZB_APS_ADDR_MODE_16_ENDP_PRESENT
zb_uint32_t g_test_step = 0;
zb_uint8_t g_error_cnt = 0;
zb_uint8_t g_addr_mode;
zb_uint8_t g_endpoint;

void send_read_attr(zb_bufid_t buffer, zb_uint16_t clusterID, zb_uint16_t attributeID)
{
  zb_uint8_t *cmd_ptr;
  ZB_ZCL_GENERAL_INIT_READ_ATTR_REQ((buffer), cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);
  ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, (attributeID));
  ZB_ZCL_GENERAL_SEND_READ_ATTR_REQ(
      (buffer), cmd_ptr, DST_ADDR, DST_ADDR_MODE, ENDPOINT_ED, ENDPOINT_C,
       ZB_AF_HA_PROFILE_ID, (clusterID), NULL);
}

void send_write_attr(zb_bufid_t buffer,zb_uint16_t clusterID, zb_uint16_t attributeID, zb_uint8_t attrType, zb_uint8_t *attrVal)
{
  zb_uint8_t *cmd_ptr;
  ZB_ZCL_GENERAL_INIT_WRITE_ATTR_REQ((buffer), cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);
  ZB_ZCL_GENERAL_ADD_VALUE_WRITE_ATTR_REQ(cmd_ptr, (attributeID), (attrType), (attrVal));
  ZB_ZCL_GENERAL_SEND_WRITE_ATTR_REQ((buffer), cmd_ptr, DST_ADDR, DST_ADDR_MODE,
    ENDPOINT_ED, ENDPOINT_C, ZB_AF_HA_PROFILE_ID, (clusterID), NULL);
}

#ifdef ZTT_USER_INTERACTION
void button_2_handler(zb_uint8_t param)
{
  ZVUNUSED(param);

  g_attr_illuminance_measurement_measured_value+=0x100;
  ZVUNUSED(zb_zcl_set_attr_val(ENDPOINT_C,
                               ZB_ZCL_CLUSTER_ID_ILLUMINANCE_MEASUREMENT,
                               ZB_ZCL_CLUSTER_SERVER_ROLE,
                               ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MEASURED_VALUE_ID,
                               (zb_uint8_t *)&g_attr_illuminance_measurement_measured_value,
                               ZB_FALSE));
  g_attr_temperature_measurement_measured_value += 0x10;
  ZVUNUSED(zb_zcl_set_attr_val(ENDPOINT_C,
                               ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
                               ZB_ZCL_CLUSTER_SERVER_ROLE,
                               ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID,
                               (zb_uint8_t *)&g_attr_temperature_measurement_measured_value,
                               ZB_FALSE));
  g_attr_occupancy_sensing_occupancy ^= 0x01;
  ZVUNUSED(zb_zcl_mark_attr_for_reporting(ENDPOINT_C,
                                          ZB_ZCL_CLUSTER_ID_OCCUPANCY_SENSING,
                                          ZB_ZCL_CLUSTER_SERVER_ROLE,
                                          ZB_ZCL_ATTR_OCCUPANCY_SENSING_OCCUPANCY_ID));
}

static void gpio_init(void)
{
  zb_osif_led_button_init();
  //APP_ERROR_CHECK(err_code);
#ifdef ZB_USE_BUTTONS
  zb_button_register_handler(BUTTON_1, 0, button_2_handler);
#endif
  //APP_ERROR_CHECK(err_code);
}
#endif

MAIN()
{
  ARGV_UNUSED;

  ZB_SET_TRACE_ON();
  ZB_SET_TRAF_DUMP_ON();

  ZB_INIT("sample_zc");

#ifdef ZTT_USER_INTERACTION
  gpio_init();
#endif

  zb_set_long_address(g_zc_addr);
  zb_set_network_coordinator_role(1l<<24);
  zb_set_nvram_erase_at_start(ZB_TRUE);
  zb_set_max_children(1);
  zb_set_pan_id(0x029a);

  /* Register device list */
  ZB_AF_REGISTER_DEVICE_CTX(&device_ctx);
  ZB_AF_SET_ENDPOINT_HANDLER(ENDPOINT_C, zcl_specific_cluster_cmd_handler);
  ZB_ZCL_REGISTER_DEVICE_CB(test_device_interface_cb);

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
    /* FALLTHROUGH */
    default:
      device_cb_param->status = RET_OK;
      break;
  }

  TRACE_MSG(TRACE_APP1, "< test_device_interface_cb %hd", (FMT__H, device_cb_param->status));
}

zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
  zb_zcl_parsed_hdr_t cmd_info;
  zb_uint8_t lqi = ZB_MAC_LQI_UNDEFINED;
  zb_int8_t rssi = ZB_MAC_RSSI_UNDEFINED;

  TRACE_MSG(TRACE_APP1, "> zcl_specific_cluster_cmd_handler", (FMT__0));

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

  TRACE_MSG(TRACE_APP1, "< zcl_specific_cluster_cmd_handler", (FMT__0));
  return ZB_FALSE;
}

void button_press_handler(zb_uint8_t param)
{
  if (!param)
  {
    /* Button is pressed, get buffer for outgoing command */
    zb_buf_get_out_delayed(button_press_handler);
  }
  else
  {
    if (g_test_step == TEST_STEP_FINISHED /*|| g_error_cnt*/)
    {
      if (g_error_cnt)
      {
        TRACE_MSG(TRACE_ERROR, "ERROR Test failed with %hd errors", (FMT__H, g_error_cnt));
      }
      else
      {
        TRACE_MSG(TRACE_ERROR, "Test finished. Status: OK", (FMT__0));
      }
      zb_buf_free(param);
    }
    else
    {
      test_next_step(param);

#ifndef ZB_USE_BUTTONS
      /* Do not have buttons in simulator - just start periodic on/off sending */
      ZB_SCHEDULE_APP_ALARM(button_press_handler, 0, 5 * ZB_TIME_ONE_SECOND);
#endif
    }
  }
}
void test_timer_next_step(zb_uint8_t param)
{
  (void)param;

  //g_test_step++;
  zb_buf_get_out_delayed(test_next_step);
}

void test_next_step(zb_uint8_t param)
{
  zb_bufid_t buffer = param;

  TRACE_MSG(TRACE_APP3, "> test_next_step param %hd step %ld", (FMT__H_L, param, g_test_step));

  switch (g_test_step )
  {
  case TEST_STEP_ALARMS_SEND_ALARM_RES:
    {
      TRACE_MSG(TRACE_APP1, "Send Alarm command to Alarms cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_ALARMS_SEND_ALARM_RES(buffer, DST_ADDR, DST_ADDR_MODE, ENDPOINT_ED, ENDPOINT_C, ZB_AF_HA_PROFILE_ID, NULL, 0x01, ZB_ZCL_CLUSTER_ID_IAS_ZONE);
    }
    break;
  default:
    g_test_step = TEST_STEP_FINISHED;
    TRACE_MSG(TRACE_ERROR, "ERROR step %hd shan't be processed", (FMT__H, g_test_step));
    ++g_error_cnt;
    break;
  }

  TRACE_MSG(TRACE_APP3, "< test_next_step. Curr step %hd" , (FMT__H, g_test_step));
}/* void test_next_step(zb_uint8_t param) */
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
#ifndef ZB_USE_BUTTONS
        ZB_SCHEDULE_APP_ALARM_CANCEL(button_press_handler, ZB_ALARM_ANY_PARAM);
        ZB_SCHEDULE_APP_ALARM(button_press_handler, 0, 8.5 * ZB_TIME_ONE_SECOND);
#endif
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
