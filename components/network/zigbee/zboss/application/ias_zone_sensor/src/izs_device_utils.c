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
/* PURPOSE: General IAS zone device utilities
*/

#define ZB_TRACE_FILE_ID 63296
#include "izs_device.h"

/* Perform local operation - leave network */
void izs_leave_nwk(zb_uint8_t param)
{
  zb_bufid_t buf = param;
  zb_zdo_mgmt_leave_param_t *req_param;

  req_param = ZB_BUF_GET_PARAM(buf, zb_zdo_mgmt_leave_param_t);
  ZB_BZERO(req_param, sizeof(zb_zdo_mgmt_leave_param_t));

  /* Set dst_addr == local address for local leave */
  req_param->dst_addr = ZB_PIBCACHE_NETWORK_ADDRESS();
  zdo_mgmt_leave_req(param, NULL);
}

void izs_ias_zone_queue_init(void)
{
  ZB_RING_BUFFER_INIT(&g_device_ctx.ias_zone_queue);
  g_device_ctx.current_zone_state = IZS_INIT_ZONE_STATUS;
}

void izs_ias_zone_queue_put(zb_uint16_t new_zone_status)
{
  izs_ias_zone_info_t ias_zone_info;

  TRACE_MSG(TRACE_APP1, "> izs_ias_zone_queue_put", (FMT__0));

  g_device_ctx.current_zone_state = new_zone_status;

  ias_zone_info.ias_status = new_zone_status;
  ias_zone_info.timestamp = izs_get_current_time_qsec();

  if (!ZB_RING_BUFFER_IS_FULL(&g_device_ctx.ias_zone_queue))
  {
    /* Add message to queue */
    ZB_RING_BUFFER_PUT_PTR(&g_device_ctx.ias_zone_queue, &ias_zone_info);
  }
  else
  {
    /* In case of receive pool - reuse the last used queue item
     * (the previous item will be lost). */
    ZB_RING_BUFFER_PUT_REUSE_LAST(&g_device_ctx.ias_zone_queue, &ias_zone_info);
  }

  TRACE_MSG(TRACE_APP1, "< izs_ias_zone_queue_put", (FMT__0));
}

zb_bool_t izs_ias_zone_queue_is_empty(void)
{
  return (ZB_RING_BUFFER_IS_EMPTY(&g_device_ctx.ias_zone_queue)?ZB_TRUE:ZB_FALSE);
}

/* get item from queue, but not delete it in queue. Queue item will
 * be released when command is successfully sent */
izs_ias_zone_info_t* izs_ias_zone_get_element_from_queue(void)
{
  izs_ias_zone_info_t *buf_element;

  TRACE_MSG(TRACE_APP1, "> izs_ias_zone_get_element_from_queue", (FMT__0));

  buf_element = ZB_RING_BUFFER_PEEK(&g_device_ctx.ias_zone_queue);

  TRACE_MSG(TRACE_APP1, "< izs_ias_zone_get_element_from_queue", (FMT__0));
  return buf_element;
}

/* Release queue element */
void izs_ias_zone_release_element_from_queue(void)
{
  izs_ias_zone_info_t *buf_element;

  TRACE_MSG(TRACE_APP1, "> izs_ias_zone_release_element_from_queue", (FMT__0));

  buf_element = ZB_RING_BUFFER_PEEK(&g_device_ctx.ias_zone_queue);
  if (buf_element)
  {
    ZB_RING_BUFFER_FLUSH_GET(&g_device_ctx.ias_zone_queue);
  }

  TRACE_MSG(TRACE_APP1, "< izs_ias_zone_release_element_from_queue", (FMT__0));
}

zb_uint16_t izs_calc_time_delta_qsec(zb_uint16_t t)
{
  zb_uint16_t delta = 0;

  delta = izs_get_current_time_qsec() - t;

  return delta;
}

void izs_measure_batteries(void)
{
  TRACE_MSG(TRACE_SPECIAL1, ">> izs_measure_batteries", (FMT__0));

  izs_read_unloaded_battery_voltage(0);
  izs_read_loaded_battery_voltage(0);

  TRACE_MSG(TRACE_SPECIAL1, "<< izs_measure_batteries", (FMT__0));
}


void izs_check_in_cb(zb_uint8_t param)
{
  ZVUNUSED(param);

  TRACE_MSG(TRACE_APP1, "izs_check_in_cb", (FMT__0));
#ifdef IZS_OTA
  izs_check_and_get_ota_server(0);
#endif
}

/* set default reporting configuration for the Power Configuration */
void izs_configure_power_config_default_reporting(zb_uint8_t param)
{
  zb_zcl_reporting_info_t rep_info;
  ZB_BZERO(&rep_info, sizeof(rep_info));

  ZVUNUSED(param);

  TRACE_MSG(TRACE_ZCL1, ">> izs_configure_power_config_default_reporting", (FMT__0));
  {
    rep_info.direction = ZB_ZCL_CONFIGURE_REPORTING_SEND_REPORT;
    rep_info.ep = IZS_DEVICE_ENDPOINT;
    rep_info.cluster_id = ZB_ZCL_CLUSTER_ID_POWER_CONFIG;
    rep_info.cluster_role = ZB_ZCL_CLUSTER_SERVER_ROLE;
    rep_info.attr_id = ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_ALARM_STATE_ID;
    rep_info.dst.profile_id = ZB_AF_HA_PROFILE_ID;

    rep_info.u.send_info.min_interval = 0;
    rep_info.u.send_info.max_interval = 0;
    rep_info.u.send_info.delta.u32 = 0; /* not an analog data type*/
  }
  zb_zcl_put_reporting_info(&rep_info, ZB_TRUE /* override */);

  {
    rep_info.direction = ZB_ZCL_CONFIGURE_REPORTING_SEND_REPORT;
    rep_info.ep = IZS_DEVICE_ENDPOINT;
    rep_info.cluster_id = ZB_ZCL_CLUSTER_ID_POWER_CONFIG;
    rep_info.cluster_role = ZB_ZCL_CLUSTER_SERVER_ROLE;
    rep_info.attr_id = ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_VOLTAGE_ID;
    rep_info.dst.profile_id = ZB_AF_HA_PROFILE_ID;

    rep_info.u.send_info.min_interval = 0;
    rep_info.u.send_info.max_interval = 0;
    rep_info.u.send_info.delta.u8 = 1;
  }
  zb_zcl_put_reporting_info(&rep_info, ZB_TRUE /* override */);

  TRACE_MSG(TRACE_ZCL1, "<< izs_configure_power_config_default_reporting", (FMT__0));
}

void izs_send_enroll_req(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> izs_send_enroll_req %hd, addr %d ep %hd",
            (FMT__H_D_H, param, g_device_ctx.zone_attr.cie_short_addr,
              g_device_ctx.zone_attr.cie_ep));

  ZB_ZCL_IAS_ZONE_SEND_ZONE_ENROLL_REQUEST_REQ(
    param,
    g_device_ctx.zone_attr.cie_short_addr,
    ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
    g_device_ctx.zone_attr.cie_ep,
    IZS_DEVICE_ENDPOINT, ZB_AF_HA_PROFILE_ID,
    NULL,
    g_device_ctx.zone_attr.zone_type,
    IZS_DEVICE_MANUFACTURER_CODE);

  TRACE_MSG(TRACE_APP1, "<< izs_send_enroll_req", (FMT__0));
}

void izs_identify_notification(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, "> izs_identify_notification %hd", (FMT__H, param));

  ZVUNUSED(param);

  /* TODO: Blink LEDs etc */

  TRACE_MSG(TRACE_APP1, "< izs_identify_notification %hd", (FMT__H, param));
}

void izs_update_ias_zone_status(zb_uint16_t status_bit, zb_uint8_t status_bit_value)
{
  zb_uint16_t new_zone_status;

  new_zone_status = IZS_DEVICE_GET_ZONE_STATUS();

  if (status_bit_value)
  {
    new_zone_status |= status_bit;
  }
  else
  {
    new_zone_status &= ~status_bit;
  }

  if (new_zone_status != IZS_DEVICE_GET_ZONE_STATUS())
  {
    /* if status was changed - send notification */
    izs_ias_zone_queue_put(new_zone_status);
    izs_send_notification();
  }
}

void izs_read_sensor_status(zb_uint8_t param)
{
  /* Send query detector request. Response is mapped to / handled in
   * gpIntelligentSensor_cbSensorStatusIndication(). */

  TRACE_MSG(TRACE_APP1, "> izs_read_sensor_status", (FMT__0));

  ZVUNUSED(param);

  /* TODO:HW: Query detector. */

  TRACE_MSG(TRACE_APP1, "< izs_read_sensor_status", (FMT__0));
}

#ifdef IZS_OTA
void izs_device_reset_after_upgrade(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> izs_ota_upgrade_check_fw", (FMT__0));

  ZVUNUSED(param);

  zb_reset(0);

  TRACE_MSG(TRACE_APP1, "<< izs_ota_upgrade_check_fw", (FMT__0));
}

/**
 * Initiate OTA: discover OTA server
 */
void izs_check_and_get_ota_server(zb_uint8_t param)
{
  zb_uint8_t endpoint;
  zb_zcl_attr_t *attr_desc;
  zb_uint8_t ota_ep;
  zb_uint8_t upgrade_status;

  ZVUNUSED(param);

  TRACE_MSG(TRACE_OTA1, "css_check_and_get_ota_server", (FMT__0));

  endpoint = get_endpoint_by_cluster(ZB_ZCL_CLUSTER_ID_OTA_UPGRADE, ZB_ZCL_CLUSTER_CLIENT_ROLE);
  attr_desc = zb_zcl_get_attr_desc_a(endpoint,
       ZB_ZCL_CLUSTER_ID_OTA_UPGRADE, ZB_ZCL_CLUSTER_CLIENT_ROLE, ZB_ZCL_ATTR_OTA_UPGRADE_SERVER_ENDPOINT_ID);
  ZB_ASSERT(attr_desc);
  ota_ep = ZB_ZCL_GET_ATTRIBUTE_VAL_8(attr_desc);

  attr_desc = zb_zcl_get_attr_desc_a(endpoint,
                                     ZB_ZCL_CLUSTER_ID_OTA_UPGRADE, ZB_ZCL_CLUSTER_CLIENT_ROLE, ZB_ZCL_ATTR_OTA_UPGRADE_IMAGE_STATUS_ID);
  ZB_ASSERT(attr_desc);
  upgrade_status = ZB_ZCL_GET_ATTRIBUTE_VAL_8(attr_desc);

  if (0 == ota_ep &&
      ZB_ZCL_OTA_UPGRADE_IMAGE_STATUS_DOWNLOADING != upgrade_status)
  {
    /* ota_ep is 0 if OTA server was never discovered */
    zb_buf_get_in_delayed(zb_zcl_ota_upgrade_init_client);
  }
}

void izs_process_ota_upgrade_cb(zb_uint8_t param)
{
  zb_bufid_t buffer = param;
  zb_zcl_device_callback_param_t *device_cb_param =
    ZB_BUF_GET_PARAM(buffer, zb_zcl_device_callback_param_t);
  zb_zcl_ota_upgrade_value_param_t *value = &(device_cb_param->cb_param.ota_value_param);

  TRACE_MSG(TRACE_APP1, ">> izs_process_ota_upgrade_cb param %hd",
            (FMT__H, param));

  TRACE_MSG(TRACE_APP1, "status %hd", (FMT__H, value->upgrade_status));

  switch (value->upgrade_status)
  {
    case ZB_ZCL_OTA_UPGRADE_STATUS_START:
      value->upgrade_status = izs_ota_upgrade_start(value->upgrade.start.file_length,
                                                    value->upgrade.start.file_version);
      break;

    case ZB_ZCL_OTA_UPGRADE_STATUS_RECEIVE:
      value->upgrade_status = izs_ota_upgrade_write_next_portion(value->upgrade.receive.block_data,
                                                                value->upgrade.receive.file_offset,
                                                                value->upgrade.receive.data_length);
      break;

    case ZB_ZCL_OTA_UPGRADE_STATUS_CHECK:
      value->upgrade_status = izs_ota_upgrade_check_fw(param);
      break;

    case ZB_ZCL_OTA_UPGRADE_STATUS_APPLY:
      izs_ota_upgrade_mark_fw_ok();
      value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_OK;
      break;

    case ZB_ZCL_OTA_UPGRADE_STATUS_FINISH:
      zb_zcl_ota_upgrade_file_upgraded(IZS_DEVICE_ENDPOINT);
      /* Do not reset immediately - lets finish ZCL pkts exchange etc */
      ZB_SCHEDULE_APP_ALARM(izs_device_reset_after_upgrade, 0, ZB_TIME_ONE_SECOND * 15);
      value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_OK;
      break;

    case ZB_ZCL_OTA_UPGRADE_STATUS_ABORT:
      izs_ota_upgrade_abort();
      value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_OK;
      break;

    default:
      izs_ota_upgrade_abort();
      value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_ERROR;
      break;
  }

  TRACE_MSG(TRACE_APP1, "<< izs_process_ota_upgrade_cb result_status %hd",
            (FMT__H, value->upgrade_status));
}
#endif /* IZS_OTA */

#ifdef IZS_WWAH
void izs_enable_event_retry(const zb_zcl_wwah_enable_wwah_app_event_retry_algorithm_t *params)
{
  izs_ias_zone_queue_init();

  FIRST_BACKOFF_TIME() = params->first_backoff_time_in_seconds;
  COMMON_RATIO() = params->backoff_sequence_common_ratio;
  MAX_BACKOFF_TIME() = params->max_backoff_time_in_seconds;
  MAX_REDELIVERY_ATTEMPTS() = params->max_re_delivery_attempts;
}

void izs_disable_event_retry(void)
{
  izs_ias_zone_queue_init();
}

void izs_process_wwah_debug_report_query_cb(zb_uint8_t param)
{
  const zb_uint8_t *report_id = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_uint8_t);
  ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_NOT_FOUND;
  for (zb_uint8_t i = 0; i <= DEBUG_REPORT_TABLE_SIZE; ++i)
  {
    if (g_device_ctx.debug_report_table[i].report_id == *report_id)
    {
      ZB_ZCL_DEVICE_CMD_PARAM_OUT_SET(param, &g_device_ctx.debug_report_table[i]);
      ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
      TRACE_MSG(TRACE_APP1, "debug report %d found", (FMT__D, *report_id));
      break;
    }
  }
  TRACE_MSG(TRACE_APP1, "<< dl_process_wwah_debug_report_query_cb", (FMT__0));
}
#endif

/**
 * Callback registered by ZB_ZCL_REGISTER_DEVICE_CB.
 */
void izs_device_interface_cb(zb_uint8_t param)
{
  zb_zcl_device_callback_param_t *device_cb_param =
    ZB_BUF_GET_PARAM(param, zb_zcl_device_callback_param_t);

  TRACE_MSG(TRACE_APP1, "> izs_device_interface_cb param %hd id %hd", (FMT__H_H,
      param, device_cb_param->device_cb_id));

  device_cb_param->status = RET_OK;

  switch (device_cb_param->device_cb_id)
  {
#ifdef IZS_OTA
    case ZB_ZCL_OTA_UPGRADE_VALUE_CB_ID:
      izs_process_ota_upgrade_cb(param);
      break;
#endif
#ifdef IZS_WWAH
    case ZB_ZCL_WWAH_ENABLE_APP_EVENT_RETRY_ALGORITHM_CB_ID:
    {
      const zb_zcl_wwah_enable_wwah_app_event_retry_algorithm_t *pl_in =
              ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_wwah_enable_wwah_app_event_retry_algorithm_t);
      izs_enable_event_retry(pl_in);
    }
    break;
    case ZB_ZCL_WWAH_DISABLE_APP_EVENT_RETRY_ALGORITHM_CB_ID:
      izs_disable_event_retry();
      break;
    case ZB_ZCL_WWAH_DEBUG_REPORT_QUERY_CB_ID:
      izs_process_wwah_debug_report_query_cb(param);
      break;
#endif
    case ZB_ZCL_BASIC_RESET_CB_ID:
      izs_basic_reset_to_defaults(0);
      break;
    case ZB_ZCL_IAS_ZONE_ENROLL_RESPONSE_VALUE_CB_ID:
      break;
    case ZB_ZCL_WWAH_SET_IAS_ZONE_ENROLLMENT_METHOD_CB_ID:
    {
      const zb_uint8_t *enrollment_method = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_uint8_t);
      g_device_ctx.enrollment_method = *enrollment_method;
    }
    break;

    default:
      device_cb_param->status = RET_ERROR;
      break;
  }

  TRACE_MSG(TRACE_APP1, "< izs_device_interface_cb %hd", (FMT__H, device_cb_param->status));
}

void izs_set_device_state(izs_device_state_t new_state)
{
  g_device_ctx.device_state = new_state;
  switch (g_device_ctx.device_state)
  {
    case IZS_STATE_APP_NOT_INIT:
      TRACE_MSG(TRACE_APP1, "IZS_STATE_APP_NOT_INIT", (FMT__0));
      break;
    case IZS_STATE_HW_INIT:
      TRACE_MSG(TRACE_APP1, "IZS_STATE_HW_INIT", (FMT__0));
      break;
    case IZS_STATE_IDLE:
      TRACE_MSG(TRACE_APP1, "IZS_STATE_IDLE", (FMT__0));
      break;
    case IZS_STATE_START_JOIN:
      TRACE_MSG(TRACE_APP1, "IZS_STATE_START_JOIN", (FMT__0));
      break;
    case IZS_STATE_STARTUP_COMPLETE:
      TRACE_MSG(TRACE_APP1, "IZS_STATE_STARTUP_COMPLETE", (FMT__0));
      break;
    case IZS_STATE_REJOIN_BACKOFF:
      TRACE_MSG(TRACE_APP1, "IZS_STATE_REJOIN_BACKOFF", (FMT__0));
      break;
    case IZS_STATE_RESET_TO_DEFAULT:
      TRACE_MSG(TRACE_APP1, "IZS_STATE_RESET_TO_DEFAULT", (FMT__0));
      break;
    case IZS_STATE_NO_NWK_SLEEP:
      TRACE_MSG(TRACE_APP1, "IZS_STATE_NO_NWK_SLEEP", (FMT__0));
      break;
    case IZS_STATE_SENSOR_NORMAL:
      TRACE_MSG(TRACE_APP1, "IZS_STATE_SENSOR_NORMAL", (FMT__0));
      break;
    default:
      TRACE_MSG(TRACE_ERROR, "izs_set_device_state: unknown state %hd", (FMT__H, g_device_ctx.device_state));
      break;
  }
}
