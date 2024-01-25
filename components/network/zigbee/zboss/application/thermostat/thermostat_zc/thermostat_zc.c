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
/* PURPOSE: ZC Thermostat sample for HA profile
*/

#define ZB_TRACE_FILE_ID 60111

#include "thermostat_zc.h"

#if ! defined ZB_COORDINATOR_ROLE
#error define ZB_COORDINATOR_ROLE to compile zc tests
#endif

/**
 * Global variables definitions
 */
zb_uint8_t dst_endpoint = 0xFF;
zb_ieee_addr_t g_zc_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa}; /* IEEE address of the
                                                                              * device */

zb_uint8_t thermostat_cmd_type = ZB_ZCL_CMD_THERMOSTAT_SETPOINT_RAISE_LOWER;
/* [default_short_addr] */
zb_uint16_t thermostat_client_short_addr = 0xFFFF;
/* [default_short_addr] */

zb_zcl_thermostat_setpoint_raise_lower_req_t setpoint_raise_lower_data = { 0 };
zb_zcl_thermostat_set_weekly_schedule_req_t set_weekly_schedule_data = { 0 };
zb_zcl_thermostat_weekly_schedule_point_pair_t weekly_schedule_point_pair_data = { 0 };
zb_zcl_thermostat_get_weekly_schedule_req_t get_weekly_schedule_data = { 0 };

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

/* Declare cluster list for a device */
ZB_DECLARE_THERMOSTAT_CLUSTER_LIST(thermostat_clusters,
                                   basic_attr_list,
                                   identify_attr_list);

/* Declare endpoint */
ZB_DECLARE_THERMOSTAT_EP(thermostat_ep, SRC_ENDPOINT, thermostat_clusters);

/* Declare application's device context for single-endpoint device*/
ZB_HA_DECLARE_THERMOSTAT_CTX(thermostat_ctx, thermostat_ep);


MAIN()
{
  ARGV_UNUSED;

  /* Traffic dump enable */
  ZB_SET_TRAF_DUMP_ON();

  /* Set trace level */
  ZB_SET_TRACE_LEVEL(4);
  /* Set trace mask */
  ZB_SET_TRACE_MASK(0x0800);

  /* Global ZBOSS initialization */
  ZB_INIT("thermostat_zc");

  /* Set up defaults for the commissioning */
  zb_set_long_address(g_zc_addr);
  zb_set_network_coordinator_role(ZB_THERMOSTAT_CHANNEL_MASK);
  zb_set_max_children(1);

  /* Turn off NVRAM erase at start */
  zb_set_nvram_erase_at_start(ZB_FALSE);

  /* Set PAN ID */
  zb_set_pan_id(0x1aaa);

  /* Register device ZCL context */
  ZB_AF_REGISTER_DEVICE_CTX(&thermostat_ctx);

 /* Register cluster commands handler for a specific endpoint */
  ZB_AF_SET_ENDPOINT_HANDLER(SRC_ENDPOINT, zcl_specific_cluster_cmd_handler);

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


zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
  zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);
  zb_bool_t unknown_cmd_received = ZB_TRUE;

  TRACE_MSG(TRACE_APP1, ">> zcl_specific_cluster_cmd_handler %i", (FMT__H, param));
  TRACE_MSG(TRACE_APP3, "payload size: %i", (FMT__D, zb_buf_len(param)));

  if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI)
  {
    if (cmd_info->cmd_id == ZB_ZCL_CMD_DEFAULT_RESP)
    {
      unknown_cmd_received = ZB_FALSE;
      zb_buf_free(param);
    }
  }

  TRACE_MSG(TRACE_APP1, "<< zcl_specific_cluster_cmd_handler %i", (FMT__H, param));
  return ! unknown_cmd_received;
}

/* Callback to handle the stack events */
void zboss_signal_handler(zb_uint8_t param)
{
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_t sig = zb_get_app_signal(param, &sg_p);

  TRACE_MSG(TRACE_APP1, ">> zboss_signal_handler: status %hd signal %hd",
            (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        break;

      case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
        ZB_SCHEDULE_APP_ALARM(start_fb_initiator, 0, 3*ZB_TIME_ONE_SECOND);
        break;

      case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:
        TRACE_MSG(TRACE_APP1, "Finding&binding done", (FMT__0));
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
    TRACE_MSG(TRACE_ERROR,
              "Device started FAILED status %d",
              (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
  }

  if (param)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_APP1, "<< zboss_signal_handler", (FMT__0));
}


void start_fb_initiator(zb_uint8_t param)
{
  ZVUNUSED(param);
  zb_bdb_finding_binding_initiator(SRC_ENDPOINT, finding_binding_cb);
}


zb_bool_t finding_binding_cb(
  zb_int16_t status,
  zb_ieee_addr_t addr,
  zb_uint8_t ep,
  zb_uint16_t cluster)
{
  TRACE_MSG(TRACE_APP1, "finding_binding_cb status %d addr " TRACE_FORMAT_64 " ep %hd cluster %d",
            (FMT__D_A_H_D, status, TRACE_ARG_64(addr), ep, cluster));
/* [address_short_by_ieee] */
  if (ZB_BDB_COMM_BIND_SUCCESS == status
      && ZB_ZCL_CLUSTER_ID_THERMOSTAT == cluster)
  {
    dst_endpoint = ep;
    thermostat_client_short_addr = zb_address_short_by_ieee(addr);

    ZB_SCHEDULE_APP_CALLBACK(send_thermostat_cmd, thermostat_cmd_type);
  }
/* [address_short_by_ieee] */
  return ZB_TRUE;
}


void send_thermostat_cmd(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">>send_thermostat_cmd, buf_ref = %d", (FMT__D, param));

  if (0xFF == dst_endpoint
      || 0xFFFF == thermostat_client_short_addr)
  {
    TRACE_MSG(TRACE_ERROR, "ERROR: invalid params for sending thermostat cmd", (FMT__0));
    TRACE_MSG(TRACE_ERROR, "dst_endpoint %hd, thermostat_client_short_addr 0x%x",
              (FMT__D_D, dst_endpoint, thermostat_client_short_addr));
  }

  switch (param)
  {
    case ZB_ZCL_CMD_THERMOSTAT_SETPOINT_RAISE_LOWER:
      setpoint_raise_lower_data.mode =
        (setpoint_raise_lower_data.mode + 1) %
        ZB_ZCL_THERMOSTAT_SETPOINT_RAISE_LOWER_MODE_RESERVED;
      setpoint_raise_lower_data.amount = ZB_RANDOM_VALUE(255);

      send_setpoint_raise_lower_cmd(&setpoint_raise_lower_data);
      break;

    case ZB_ZCL_CMD_THERMOSTAT_SET_WEEKLY_SCHEDULE:
      set_weekly_schedule_data.num_of_transitions = 1;
      set_weekly_schedule_data.day_of_week =
        (set_weekly_schedule_data.day_of_week + 1) % ZB_DAYS_PER_WEEK;
      set_weekly_schedule_data.mode_for_seq =
        (set_weekly_schedule_data.mode_for_seq + 1) %
        ZB_ZCL_THERMOSTAT_SETPOINT_RAISE_LOWER_MODE_RESERVED;

      weekly_schedule_point_pair_data.transition_time = ZB_RANDOM_VALUE(MINUTES_PER_DAY);
      weekly_schedule_point_pair_data.heat_set_point = GET_RANDOM_TEMPERATURE;
      weekly_schedule_point_pair_data.cool_set_point = GET_RANDOM_TEMPERATURE;

      send_set_weekly_schedule_cmd(&set_weekly_schedule_data,
                                   &weekly_schedule_point_pair_data);
      break;

    case ZB_ZCL_CMD_THERMOSTAT_GET_WEEKLY_SCHEDULE:
      get_weekly_schedule_data.days_to_return =
        (get_weekly_schedule_data.days_to_return + 1) % ZB_DAYS_PER_WEEK;
      get_weekly_schedule_data.mode_to_return =
        (get_weekly_schedule_data.mode_to_return + 1) %
        ZB_ZCL_THERMOSTAT_SETPOINT_RAISE_LOWER_MODE_RESERVED;

      send_get_weekly_schedule_cmd(&get_weekly_schedule_data);
      break;

    case ZB_ZCL_CMD_THERMOSTAT_CLEAR_WEEKLY_SCHEDULE:
      send_clear_weekly_schedule_cmd();
      break;

    case ZB_ZCL_CMD_THERMOSTAT_GET_RELAY_STATUS_LOG:
      send_get_relay_status_log_cmd();
      break;

    default:
      break;
  }

  TRACE_MSG(TRACE_APP1, "<<send_thermostat_cmd", (FMT__0));
}


void send_thermostat_cmd_cb(zb_uint8_t param)
{
  zb_zcl_command_send_status_t *cmd_send_status =
    ZB_BUF_GET_PARAM(param, zb_zcl_command_send_status_t);

  TRACE_MSG(TRACE_APP1,
            "send_thermostat_cmd_cb, status = %d",
            (FMT__D, cmd_send_status->status));

  zb_buf_free(param);

  thermostat_cmd_type =
    (thermostat_cmd_type + 1) %
    (ZB_ZCL_CMD_THERMOSTAT_GET_RELAY_STATUS_LOG + 1);

  ZB_SCHEDULE_APP_ALARM(send_thermostat_cmd,
                    thermostat_cmd_type,
                    3*ZB_TIME_ONE_SECOND);
}


void send_setpoint_raise_lower_cmd(
  zb_zcl_thermostat_setpoint_raise_lower_req_t *setpoint_raise_lower_data)
{
  zb_bufid_t buf = 0;

  TRACE_MSG(TRACE_APP1, "send_setpoint_raise_lower_cmd", (FMT__0));

  if (!setpoint_raise_lower_data)
  {
    TRACE_MSG(TRACE_ERROR, "APP FAILED: invalid data ptr!", (FMT__0));
    return;
  }

  buf = zb_buf_get_out();

  if (!buf)
  {
    TRACE_MSG(TRACE_ERROR, "APP FAILED: could not get out buf!", (FMT__0));
    return;
  }

  ZB_ZCL_THERMOSTAT_SEND_SETPOINT_RAISE_LOWER_REQ(
    buf,
    thermostat_client_short_addr,
    DST_ADDR_MODE,
    dst_endpoint,
    SRC_ENDPOINT,
    ZB_AF_HA_PROFILE_ID,
    DISABLE_DEFAULT_RESPONSE_FLAG,
    send_thermostat_cmd_cb,
    setpoint_raise_lower_data->mode,
    setpoint_raise_lower_data->amount);
}


void send_set_weekly_schedule_cmd(
  zb_zcl_thermostat_set_weekly_schedule_req_t *set_weekly_schedule_data,
  zb_zcl_thermostat_weekly_schedule_point_pair_t *weekly_schedule_point_pair_data)
{
  zb_bufid_t buf = 0;
  zb_uint8_t *cmd_ptr = NULL;

  TRACE_MSG(TRACE_APP1, "send_set_weekly_schedule_cmd", (FMT__0));

  if (!set_weekly_schedule_data || !weekly_schedule_point_pair_data)
  {
    TRACE_MSG(TRACE_ERROR, "APP FAILED: invalid data ptr!", (FMT__0));
    return;
  }

  buf = zb_buf_get_out();

  if (!buf)
  {
    TRACE_MSG(TRACE_ERROR, "APP FAILED: could not get out buf!", (FMT__0));
    return;
  }

  ZB_ZCL_THERMOSTAT_INIT_SET_WEEKLY_SCHEDULE_REQ(
    buf,
    cmd_ptr,
    DISABLE_DEFAULT_RESPONSE_FLAG,
    set_weekly_schedule_data->num_of_transitions,
    set_weekly_schedule_data->day_of_week,
    set_weekly_schedule_data->mode_for_seq,
    weekly_schedule_point_pair_data->transition_time,
    weekly_schedule_point_pair_data->heat_set_point,
    weekly_schedule_point_pair_data->cool_set_point);

  ZB_ZCL_THERMOSTAT_SEND_SET_WEEKLY_SCHEDULE_REQ(
    buf,
    cmd_ptr,
    thermostat_client_short_addr,
    DST_ADDR_MODE,
    dst_endpoint,
    SRC_ENDPOINT,
    ZB_AF_HA_PROFILE_ID,
    send_thermostat_cmd_cb);
}


void send_get_weekly_schedule_cmd(
  zb_zcl_thermostat_get_weekly_schedule_req_t *get_weekly_schedule_data)
{
  zb_bufid_t buf = 0;

  TRACE_MSG(TRACE_APP1, "send_get_weekly_schedule_cmd", (FMT__0));

  if (!get_weekly_schedule_data)
  {
    TRACE_MSG(TRACE_ERROR, "APP FAILED: invalid data ptr!", (FMT__0));
    return;
  }

  buf = zb_buf_get_out();

  if (!buf)
  {
    TRACE_MSG(TRACE_ERROR, "APP FAILED: could not get out buf!", (FMT__0));
    return;
  }

  ZB_ZCL_THERMOSTAT_SEND_GET_WEEKLY_SCHEDULE_REQ(
    buf,
    thermostat_client_short_addr,
    DST_ADDR_MODE,
    dst_endpoint,
    SRC_ENDPOINT,
    ZB_AF_HA_PROFILE_ID,
    DISABLE_DEFAULT_RESPONSE_FLAG,
    send_thermostat_cmd_cb,
    get_weekly_schedule_data->days_to_return,
    get_weekly_schedule_data->mode_to_return);
}


void send_clear_weekly_schedule_cmd(void)
{
  zb_bufid_t buf = zb_buf_get_out();;

  TRACE_MSG(TRACE_APP1, "send_get_weekly_schedule_cmd", (FMT__0));

  if (!buf)
  {
    TRACE_MSG(TRACE_ERROR, "APP FAILED: could not get out buf!", (FMT__0));
    return;
  }

  ZB_ZCL_THERMOSTAT_SEND_CLEAR_WEEKLY_SCHEDULE_REQ(
    buf,
    thermostat_client_short_addr,
    DST_ADDR_MODE,
    dst_endpoint,
    SRC_ENDPOINT,
    ZB_AF_HA_PROFILE_ID,
    DISABLE_DEFAULT_RESPONSE_FLAG,
    send_thermostat_cmd_cb);
}


void send_get_relay_status_log_cmd(void)
{
  zb_bufid_t buf = zb_buf_get_out();;

  TRACE_MSG(TRACE_APP1, "send_get_weekly_schedule_cmd", (FMT__0));

  if (!buf)
  {
    TRACE_MSG(TRACE_ERROR, "APP FAILED: could not get out buf!", (FMT__0));
    return;
  }

  ZB_ZCL_THERMOSTAT_SEND_GET_RELAY_STATUS_LOG_REQ(
    buf,
    thermostat_client_short_addr,
    DST_ADDR_MODE,
    dst_endpoint,
    SRC_ENDPOINT,
    ZB_AF_HA_PROFILE_ID,
    DISABLE_DEFAULT_RESPONSE_FLAG,
    send_thermostat_cmd_cb);
}
