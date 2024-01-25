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
/* PURPOSE: Test sample with clusters Thermostat, Fan control, Dehumidification
 and Thermostat UI configuration


*/

#define ZB_TRACE_FILE_ID 51361
#include "zboss_api.h"
#include "hvac_zr.h"

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

/**
 * Declaring attributes for each cluster
 */

/* Thermostat cluster attributes */
/* Information set */
zb_uint16_t g_attr_local_temperature = 0;
zb_uint16_t g_attr_abs_min_heat_setpoint_limit = ZB_ZCL_THERMOSTAT_ABS_MIN_HEAT_SETPOINT_LIMIT_DEFAULT_VALUE;
zb_uint16_t g_attr_abs_max_heat_setpoint_limit = ZB_ZCL_THERMOSTAT_ABS_MAX_HEAT_SETPOINT_LIMIT_DEFAULT_VALUE;
zb_uint16_t g_attr_abs_min_cool_setpoint_limit = ZB_ZCL_THERMOSTAT_ABS_MIN_COOL_SETPOINT_LIMIT_DEFAULT_VALUE;
zb_uint16_t g_attr_abs_max_cool_setpoint_limit = ZB_ZCL_THERMOSTAT_ABS_MAX_COOL_SETPOINT_LIMIT_DEFAULT_VALUE;
zb_uint8_t g_attr_PI_cooling_demand = 0;
zb_uint8_t g_attr_PI_heating_demand = 0;
zb_uint8_t g_attr_HVAC_system_type_configuration = ZB_ZCL_THERMOSTAT_HVAC_SYSTEM_TYPE_CONFIGURATION_DEFAULT_VALUE;
/* Settings set */
zb_uint8_t g_attr_local_temperature_calibration = ZB_ZCL_THERMOSTAT_LOCAL_TEMPERATURE_CALIBRATION_DEFAULT_VALUE;
zb_uint16_t g_attr_occupied_cooling_setpoint = ZB_ZCL_THERMOSTAT_OCCUPIED_COOLING_SETPOINT_DEFAULT_VALUE;
zb_uint16_t g_attr_occupied_heating_setpoint = ZB_ZCL_THERMOSTAT_OCCUPIED_HEATING_SETPOINT_DEFAULT_VALUE;
zb_uint16_t g_attr_unoccupied_cooling_setpoint = ZB_ZCL_THERMOSTAT_UNOCCUPIED_COOLING_SETPOINT_DEFAULT_VALUE;
zb_uint16_t g_attr_unoccupied_heating_setpoint = ZB_ZCL_THERMOSTAT_UNOCCUPIED_HEATING_SETPOINT_DEFAULT_VALUE;
zb_uint16_t g_attr_min_heat_setpoint_limit = ZB_ZCL_THERMOSTAT_MIN_HEAT_SETPOINT_LIMIT_DEFAULT_VALUE;
zb_uint16_t g_attr_max_heat_setpoint_limit = ZB_ZCL_THERMOSTAT_MAX_HEAT_SETPOINT_LIMIT_DEFAULT_VALUE;
zb_uint16_t g_attr_min_cool_setpoint_limit = ZB_ZCL_THERMOSTAT_MIN_COOL_SETPOINT_LIMIT_DEFAULT_VALUE;
zb_uint16_t g_attr_max_cool_setpoint_limit = ZB_ZCL_THERMOSTAT_MAX_COOL_SETPOINT_LIMIT_DEFAULT_VALUE;
zb_uint8_t g_attr_min_setpoint_dead_band = ZB_ZCL_THERMOSTAT_MIN_SETPOINT_DEAD_BAND_DEFAULT_VALUE;
zb_uint8_t g_attr_remote_sensing = ZB_ZCL_THERMOSTAT_REMOTE_SENSING_DEFAULT_VALUE;
zb_uint8_t g_attr_control_seq_of_operation = ZB_ZCL_THERMOSTAT_CONTROL_SEQ_OF_OPERATION_DEFAULT_VALUE;
zb_uint8_t g_attr_system_mode = ZB_ZCL_THERMOSTAT_SYSTEM_MODE_DEFAULT_VALUE;
/* Schedule set */
zb_uint8_t g_attr_start_of_week = ZB_ZCL_THERMOSTAT_START_OF_WEEK_DEFAULT_VALUE;

ZB_ZCL_DECLARE_THERMOSTAT_ATTRIB_LIST_EXT(thermostat_attr_list, &g_attr_local_temperature, &g_attr_abs_min_heat_setpoint_limit,
                                          &g_attr_abs_max_heat_setpoint_limit, &g_attr_abs_min_cool_setpoint_limit,
                                          &g_attr_abs_max_cool_setpoint_limit, &g_attr_PI_cooling_demand, &g_attr_PI_heating_demand,
                                          &g_attr_HVAC_system_type_configuration, &g_attr_local_temperature_calibration,
                                          &g_attr_occupied_cooling_setpoint, &g_attr_occupied_heating_setpoint,
                                          &g_attr_unoccupied_cooling_setpoint, &g_attr_unoccupied_heating_setpoint,
                                          &g_attr_min_heat_setpoint_limit, &g_attr_max_heat_setpoint_limit,
                                          &g_attr_min_cool_setpoint_limit, &g_attr_max_cool_setpoint_limit, &g_attr_min_setpoint_dead_band,
                                          &g_attr_remote_sensing, &g_attr_control_seq_of_operation, &g_attr_system_mode, &g_attr_start_of_week);

/* Fan Control cluster attributes */
zb_uint8_t g_attr_fan_mode = ZB_ZCL_FAN_CONTROL_FAN_MODE_DEFAULT_VALUE;
zb_uint8_t g_attr_fan_mode_sequence = ZB_ZCL_FAN_CONTROL_FAN_MODE_SEQUENCE_DEFAULT_VALUE;
ZB_ZCL_DECLARE_FAN_CONTROL_ATTRIB_LIST(fan_control_attr_list, &g_attr_fan_mode, &g_attr_fan_mode_sequence);

/* Dehumidification Control cluster attributes */
zb_uint8_t g_attr_dehumid_cooling = 0;
zb_uint8_t g_attr_rh_dehumid_setpoint = ZB_ZCL_DEHUMIDIFICATION_CONTROL_RHDEHUMIDIFICATION_SETPOINT_DEFAULT_VALUE;
zb_uint8_t g_attr_dehumid_hysteresis = ZB_ZCL_DEHUMIDIFICATION_CONTROL_DEHUMIDIFICATION_HYSTERESIS_DEFAULT_VALUE;
zb_uint8_t g_attr_dehumid_max_cool = ZB_ZCL_DEHUMIDIFICATION_CONTROL_DEHUMIDIFICATION_MAX_COOL_DEFAULT_VALUE;
ZB_ZCL_DECLARE_DEHUMIDIFICATION_CONTROL_ATTRIB_LIST(dehumidification_control_attr_list, &g_attr_dehumid_cooling, &g_attr_rh_dehumid_setpoint,
                                                    &g_attr_dehumid_hysteresis, &g_attr_dehumid_max_cool);

/* Thermostat UI Configuration cluster attributes */
zb_uint8_t g_attr_temperature_display_mode = ZB_ZCL_THERMOSTAT_UI_CONFIG_TEMPERATURE_DISPLAY_MODE_DEFAULT_VALUE;
zb_uint8_t g_attr_keypad_lockout = ZB_ZCL_THERMOSTAT_UI_CONFIG_KEYPAD_LOCKOUT_DEFAULT_VALUE;
ZB_ZCL_DECLARE_THERMOSTAT_UI_CONFIG_ATTRIB_LIST(thermostat_ui_config_attr_list, &g_attr_temperature_display_mode, &g_attr_keypad_lockout);

/* Declare cluster list for the device */
ZB_DECLARE_HVAC_SERVER_CLUSTER_LIST(hvac_server_clusters, thermostat_attr_list, fan_control_attr_list,
                                        dehumidification_control_attr_list, thermostat_ui_config_attr_list);

/* Declare server endpoint */
ZB_DECLARE_HVAC_SERVER_EP(hvac_server_ep, ZB_SERVER_ENDPOINT, hvac_server_clusters);

ZB_DECLARE_HVAC_CLIENT_CLUSTER_LIST(hvac_client_clusters);
/* Declare client endpoint */
ZB_DECLARE_HVAC_CLIENT_EP(hvac_client_ep, ZB_CLIENT_ENDPOINT, hvac_client_clusters);

/* Declare application's device context for single-endpoint device */
ZB_DECLARE_HVAC_CTX(hvac_output_ctx, hvac_server_ep, hvac_client_ep);


MAIN()
{
  ARGV_UNUSED;

  /* Trace enable */
  ZB_SET_TRACE_ON();
  /* Traffic dump enable*/
  ZB_SET_TRAF_DUMP_ON();

  /* Global ZBOSS initialization */
  ZB_INIT("hvac_zr");

  /* Set up defaults for the commissioning */
  zb_set_long_address(g_zc_addr);
  zb_set_network_router_role(1l<<22);

  zb_set_nvram_erase_at_start(ZB_FALSE);
  zb_set_max_children(1);

  /* [af_register_device_context] */
  /* Register device ZCL context */
  ZB_AF_REGISTER_DEVICE_CTX(&hvac_output_ctx);

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
  zb_uint8_t *cmd_ptr = NULL;

  TRACE_MSG(TRACE_APP1, ">> test_loop test_step=%hd", (FMT__H, test_step));

  if (param == 0)
  {
    zb_buf_get_out_delayed(test_loop);
  }
  else
  {
    switch(test_step)
    {
      case 0:
        ZB_ZCL_THERMOSTAT_SEND_SETPOINT_RAISE_LOWER_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                                ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, ZB_ZCL_THERMOSTAT_SETPOINT_RAISE_LOWER_MODE_HEAT, 0);
        break;

      case 1:
        ZB_ZCL_THERMOSTAT_SEND_SET_WEEKLY_SCHEDULE_REQ(param, cmd_ptr, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                                      NULL);
        break;

      case 2:
        ZB_ZCL_THERMOSTAT_SEND_GET_WEEKLY_SCHEDULE_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                                ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, 0, ZB_ZCL_THERMOSTAT_SETPOINT_RAISE_LOWER_MODE_HEAT);
        break;

      case 3:
        ZB_ZCL_THERMOSTAT_SEND_CLEAR_WEEKLY_SCHEDULE_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                                ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
        break;

      case 4:
        ZB_ZCL_THERMOSTAT_SEND_GET_RELAY_STATUS_LOG_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                                ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
        break;

    }
    test_step++;

    if(test_step <= 4)
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
