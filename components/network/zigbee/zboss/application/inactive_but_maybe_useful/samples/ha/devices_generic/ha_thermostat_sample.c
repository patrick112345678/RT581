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
/* PURPOSE: IAS Zone for HA profile
*/

#define ZB_TRACE_FILE_ID 40160
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_zcl.h"

#include "zb_ha.h"


#if ! defined ZB_COORDINATOR_ROLE
#error define ZB_COORDINATOR_ROLE to compile zc tests
#endif

zb_ieee_addr_t g_zc_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
#define ENDPOINT  5

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
zb_uint8_t g_attr_zcl_version  = ZB_ZCL_VERSION;
zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list, &g_attr_zcl_version, &g_attr_power_source);

/* Identify cluster attributes data */
zb_uint16_t g_attr_identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_attr_identify_time);

zb_uint16_t local_temperature = ZB_ZCL_THERMOSTAT_LOCAL_TEMPERATURE_MIN_VALUE;
zb_uint8_t local_temperature_calibration = 2;
zb_uint16_t occupied_cooling_setpoint = ZB_ZCL_THERMOSTAT_OCCUPIED_COOLING_SETPOINT_DEFAULT_VALUE;
zb_uint16_t occupied_heating_setpoint = ZB_ZCL_THERMOSTAT_OCCUPIED_HEATING_SETPOINT_DEFAULT_VALUE;
zb_uint8_t control_seq_of_operation = ZB_ZCL_THERMOSTAT_CONTROL_SEQ_OF_OPERATION_DEFAULT_VALUE;
zb_uint8_t system_mode = ZB_ZCL_THERMOSTAT_CONTROL_SYSTEM_MODE_DEFAULT_VALUE;
zb_uint8_t start_of_week = ZB_ZCL_THERMOSTAT_START_OF_WEEK_SUNDAY;

ZB_ZCL_DECLARE_THERMOSTAT_ATTRIB_LIST(thermostat_attr_list,
      &local_temperature, &local_temperature_calibration,
      &occupied_cooling_setpoint, &occupied_heating_setpoint,
      &control_seq_of_operation, &system_mode, &start_of_week);

zb_uint8_t fan_mode = ZB_ZCL_FAN_CONTROL_FAN_MODE_DEFAULT_VALUE;
zb_uint8_t fan_mode_sequence = ZB_ZCL_FAN_CONTROL_FAN_MODE_SEQUENCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_FAN_CONTROL_ATTRIB_LIST(fan_control_attr_list,
    &fan_mode, &fan_mode_sequence);

zb_uint8_t temperature_display_mode = ZB_ZCL_THERMOSTAT_UI_CONFIG_TEMPERATURE_DISPLAY_MODE_DEFAULT_VALUE;
zb_uint8_t keypad_lockout = ZB_ZCL_THERMOSTAT_UI_CONFIG_KEYPAD_LOCKOUT_DEFAULT_VALUE;

ZB_ZCL_DECLARE_THERMOSTAT_UI_CONFIG_ATTRIB_LIST(thermostat_ui_config_attr_list,
    &temperature_display_mode, &keypad_lockout);

/********************* Declare device **************************/

ZB_HA_DECLARE_THERMOSTAT_CLUSTER_LIST(thermostat_clusters,
                                      basic_attr_list,
                                      identify_attr_list,
                                      thermostat_attr_list,
                                      fan_control_attr_list,
                                      thermostat_ui_config_attr_list);

  ZB_HA_DECLARE_THERMOSTAT_EP(thermostat_ep, ENDPOINT, thermostat_clusters);

ZB_HA_DECLARE_THERMOSTAT_CTX(thermostat_ctx, thermostat_ep);

/******************* Declare server parameters *****************/
zb_uint16_t dst_addr;
zb_ieee_addr_t g_ed_addr = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#define DST_ENDPOINT    10
#define DST_ADDR_MODE   ZB_APS_ADDR_MODE_16_ENDP_PRESENT
/******************* Declare test data & constants *************/

void SpiIrqHdlr(void)
{

}

MAIN()
{
  ARGV_UNUSED;

  ZB_INIT("ha_thermostat_sample");

  /* 2018/08/10 CR:MAJOR (rev. 39308): Coordinator is always rx on when idle. */
  zb_set_rx_on_when_idle(ZB_TRUE);
  zb_set_long_address(g_zc_addr);
  zb_set_network_coordinator_role(ZB_DEFAULT_APS_CHANNEL_MASK);
  zb_set_default_ffd_descriptor_values(ZB_COORDINATOR);
  ZB_PIBCACHE_PAN_ID() = 0x1aaa;

  /****************** Register Device ********************************/
  ZB_AF_REGISTER_DEVICE_CTX(&thermostat_ctx);

  if (zboss_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
  }
  else
  {
    zboss_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}


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
    ZB_FREE_BUF(ZB_BUF_FROM_REF(param));
  }

  TRACE_MSG(TRACE_APP1, "<<zboss_signal_handler", (FMT__0));
}
