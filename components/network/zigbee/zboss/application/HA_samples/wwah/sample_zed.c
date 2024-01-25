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
/* PURPOSE: ZB Simple WWAH device
*/
#define ZB_TRACE_FILE_ID 40237
#include "zboss_api.h"
#include "zb_wwah.h"
#include "zb_common.h"
#include "zb_zdo.h"

/** Test step enumeration. */
enum test_step_e
{
  TEST_STEP_WWAH_SEND_NEW_DEBUG_REPORT_NOTIFICATION, /** Send New Debug Report Notification command to Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_POWER_DESCRIPTOR_CHANGE, /** Send Power Descriptor Change command to Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_ENABLE_APS_LINK_KEY_AUTHORIZATION,

  TEST_STEP_FINISHED    /**< Test finished pseudo-step. */
};

zb_uint16_t g_dst_addr;
#ifdef ZB_ZCL_SUPPORT_CLUSTER_WWAH
zb_uint8_t g_dst_ep;
#define DST_ADDR g_dst_addr
#define DST_EP g_dst_ep
#else
//FIXME: Just to compile...
#define DST_ADDR g_dst_addr
#define DST_EP 1
#endif
#define DST_ADDR_MODE ZB_APS_ADDR_MODE_16_ENDP_PRESENT

zb_uint32_t g_test_step = TEST_STEP_WWAH_SEND_POWER_DESCRIPTOR_CHANGE;
void button_press_handler(zb_uint8_t param);
/* Handler for specific zcl commands */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);

/** Next test step initiator. */
void test_next_step(zb_uint8_t param);

#define OTA_UPGRADE_TEST_FILE_VERSION       0x01020101

#define OTA_UPGRADE_TEST_FILE_VERSION_NEW   0x01010101

#define OTA_UPGRADE_TEST_MANUFACTURER       123

#define OTA_UPGRADE_TEST_IMAGE_TYPE         321

#define OTA_UPGRADE_TEST_IMAGE_SIZE         0x3c

#define OTA_UPGRADE_TEST_CURRENT_TIME       0x12345678

#define OTA_UPGRADE_TEST_UPGRADE_TIME       0x12345678

#define OTA_UPGRADE_TEST_DATA_SIZE          50

/******************* Declare attributes ************************/

zb_uint8_t g_attr_basic_zcl_version = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
zb_uint8_t g_attr_basic_application_version = ZB_ZCL_BASIC_APPLICATION_VERSION_DEFAULT_VALUE;
zb_uint8_t g_attr_basic_stack_version = ZB_ZCL_BASIC_STACK_VERSION_DEFAULT_VALUE;
zb_uint8_t g_attr_basic_hw_version = ZB_ZCL_BASIC_HW_VERSION_DEFAULT_VALUE;
zb_char_t g_attr_basic_manufacturer_name[] = ZB_ZCL_BASIC_MANUFACTURER_NAME_DEFAULT_VALUE;
zb_char_t g_attr_basic_model_identifier[] = ZB_ZCL_BASIC_MODEL_IDENTIFIER_DEFAULT_VALUE;
zb_char_t g_attr_basic_date_code[] = ZB_ZCL_BASIC_DATE_CODE_DEFAULT_VALUE;
zb_uint8_t g_attr_basic_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;
zb_char_t g_attr_basic_location_description[] = ZB_ZCL_BASIC_LOCATION_DESCRIPTION_DEFAULT_VALUE;
zb_uint8_t g_attr_basic_physical_environment = ZB_ZCL_BASIC_PHYSICAL_ENVIRONMENT_DEFAULT_VALUE;
zb_char_t g_attr_sw_build_id[] = "\x04" "test";

/* OTA Upgrade client cluster attributes data */
zb_ieee_addr_t g_attr_ota_upgrade_server = ZB_ZCL_OTA_UPGRADE_SERVER_DEF_VALUE;
zb_uint32_t g_attr_ota_file_offset = ZB_ZCL_OTA_UPGRADE_FILE_OFFSET_DEF_VALUE;

zb_uint32_t g_attr_ota_file_version = OTA_UPGRADE_TEST_FILE_VERSION;   // custom data

zb_uint16_t g_attr_ota_stack_version = ZB_ZCL_OTA_UPGRADE_FILE_HEADER_STACK_PRO;
zb_uint32_t g_attr_ota_downloaded_file_ver = ZB_ZCL_OTA_UPGRADE_DOWNLOADED_FILE_VERSION_DEF_VALUE;
zb_uint16_t g_attr_ota_downloaded_stack_ver = ZB_ZCL_OTA_UPGRADE_DOWNLOADED_STACK_DEF_VALUE;
zb_uint8_t g_attr_ota_image_status = ZB_ZCL_OTA_UPGRADE_IMAGE_STATUS_DEF_VALUE;

zb_uint16_t g_attr_ota_manufacturer = OTA_UPGRADE_TEST_MANUFACTURER;   // custom data
zb_uint16_t g_attr_ota_image_type = OTA_UPGRADE_TEST_IMAGE_TYPE;       // custom data

zb_uint16_t g_attr_ota_min_block_reque = 0;
zb_uint16_t g_attr_ota_image_stamp = ZB_ZCL_OTA_UPGRADE_IMAGE_STAMP_MIN_VALUE;
zb_uint16_t g_attr_ota_server_addr;
zb_uint8_t g_attr_ota_server_ep;

/* Poll Control cluster attributes data */
zb_uint32_t checkin_interval       = 0x0168; // (360 quarterseconds)
zb_uint32_t long_poll_interval     = 0x14;   // (20 quarterseconds)
zb_uint16_t short_poll_interval    = 0x02;   // (2 quarterseconds)
zb_uint16_t fast_poll_timeout      = 0x28;   // (40 quarterseconds)
zb_uint32_t checkin_interval_min   = 0x00B4; // (180 quarterseconds)
zb_uint32_t long_poll_interval_min = 0x0C;   // (12 quarterseconds)
zb_uint16_t fast_poll_timeout_max  = 0x00F0; // (240 quarterseconds)

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST_EXT(basic_attr_list, &g_attr_basic_zcl_version, &g_attr_basic_application_version, &g_attr_basic_stack_version, &g_attr_basic_hw_version, &g_attr_basic_manufacturer_name, &g_attr_basic_model_identifier, &g_attr_basic_date_code, &g_attr_basic_power_source, &g_attr_basic_location_description, &g_attr_basic_physical_environment, &g_attr_sw_build_id);

#ifdef ZB_ZCL_ENABLE_WWAH_SERVER
ZB_ZCL_DECLARE_WWAH_ATTRIB_LIST(wwah_attr_list);
#else
zb_zcl_attr_t wwah_attr_list[] = { NULL };
#endif

/* Identify cluster attributes data */
zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &attr_identify_time);

ZB_ZCL_DECLARE_OTA_UPGRADE_ATTRIB_LIST(ota_upgrade_attr_list,
    &g_attr_ota_upgrade_server, &g_attr_ota_file_offset, &g_attr_ota_file_version, &g_attr_ota_stack_version, &g_attr_ota_downloaded_file_ver, &g_attr_ota_downloaded_stack_ver, &g_attr_ota_image_status, &g_attr_ota_manufacturer, &g_attr_ota_image_type, &g_attr_ota_min_block_reque, &g_attr_ota_image_stamp,
    &g_attr_ota_server_addr, &g_attr_ota_server_ep, 0x0101, OTA_UPGRADE_TEST_DATA_SIZE, ZB_ZCL_OTA_UPGRADE_QUERY_TIMER_COUNT_DEF);

ZB_ZCL_DECLARE_POLL_CONTROL_ATTRIB_LIST(poll_control_attr_list,
  &checkin_interval, &long_poll_interval, &short_poll_interval, &fast_poll_timeout,
  &checkin_interval_min, &long_poll_interval_min, &fast_poll_timeout_max);

/********************* Declare device **************************/

#ifndef ZB_ZCL_SUPPORT_CLUSTER_WWAH
void zb_zcl_wwah_init_server();
#endif
ZB_HA_DECLARE_WWAH_CLUSTER_LIST_ZED(wwah_ha_clusters, basic_attr_list, wwah_attr_list, poll_control_attr_list, ota_upgrade_attr_list, identify_attr_list);
ZB_HA_DECLARE_WWAH_ZED_EP(wwah_ha_ep, ZED_HA_EP, wwah_ha_clusters);
ZBOSS_DECLARE_DEVICE_CTX_1_EP(device_ctx, wwah_ha_ep);

/** @brief Size of Debug Reports Table */
#define DEBUG_REPORT_TABLE_SIZE 10
zb_zcl_wwah_debug_report_t debug_report_table[DEBUG_REPORT_TABLE_SIZE];

#ifndef ZB_ZCL_SUPPORT_CLUSTER_WWAH
void zb_zcl_wwah_init_server()
{
}
#endif
static void sample_zcl_cmd_device_cb(zb_uint8_t param) ZB_CALLBACK
{
  TRACE_MSG(TRACE_APP1, ">> sample_zcl_cmd_device_cb(param=%hd, id=%d)",
            (FMT__H_D, param, ZB_ZCL_DEVICE_CMD_PARAM_CB_ID(param)));
  switch (ZB_ZCL_DEVICE_CMD_PARAM_CB_ID(param))
  {
    case ZB_ZCL_WWAH_ENABLE_APP_EVENT_RETRY_ALGORITHM_CB_ID:
    {
      const zb_zcl_wwah_enable_wwah_app_event_retry_algorithm_t *payload =
        ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_wwah_enable_wwah_app_event_retry_algorithm_t);
      ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
      TRACE_MSG(TRACE_APP1, "Enable WWAH App Event Retry Algorithm command received", (FMT__0));
      TRACE_MSG(TRACE_APP1, "First Backoff Time In Seconds = %hd, Backoff Sequence Common Ratio = %hd",
        (FMT__H_H, payload->first_backoff_time_in_seconds, payload->backoff_sequence_common_ratio));
      TRACE_MSG(TRACE_APP1, "Max Backoff Time In Seconds = %hd, Max Re-Delivery Attempts = %hd",
        (FMT__H_H, payload->max_backoff_time_in_seconds, payload->max_re_delivery_attempts));
    }
    break;
    case ZB_ZCL_WWAH_DISABLE_APP_EVENT_RETRY_ALGORITHM_CB_ID:
      {
        ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
        TRACE_MSG(TRACE_APP1, "Disable WWAH App Event Retry Algorithm command received", (FMT__0));
      }
      break;
    default:
      TRACE_MSG(TRACE_APP1, "Undefined callback was called, id=%d",
                (FMT__D, ZB_ZCL_DEVICE_CMD_PARAM_CB_ID(param)));
      break;
  }
  TRACE_MSG(TRACE_APP1, "<< sample_zcl_cmd_device_cb", (FMT__0));
}

void setup_debug_report()
{
  zb_uindex_t i;
  zb_char_t *debug_report_message = "Issue #4";
  zb_uint8_t report_id = 4;
  ZB_ASSERT(DEBUG_REPORT_TABLE_SIZE > 0 && DEBUG_REPORT_TABLE_SIZE <= 0xFE);
  ZB_ZCL_SET_ATTRIBUTE(ZED_HA_EP, ZB_ZCL_CLUSTER_ID_WWAH, ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_WWAH_CURRENT_DEBUG_REPORT_ID_ID, &report_id, ZB_FALSE);
  debug_report_table[0] = (zb_zcl_wwah_debug_report_t){report_id, strlen(debug_report_message), debug_report_message};
  for(i = 1; i < DEBUG_REPORT_TABLE_SIZE; ++i)
  {
    debug_report_table[i] = ZB_ZCL_WWAH_DEBUG_REPORT_FREE_RECORD;
  }
}

MAIN()
{
  ARGV_UNUSED;

  ZB_SET_TRACE_ON();
  ZB_SET_TRAF_DUMP_OFF();

  ZB_INIT("sample_zed");
  zb_set_nvram_erase_at_start(ZB_TRUE);
#ifdef ZB_ZCL_ENABLE_WWAH_SERVER
  zb_zcl_wwah_set_wwah_behavior(ZB_ZCL_WWAH_BEHAVIOR_SERVER);
#endif
/*  zb_set_ed_timeout(ED_AGING_TIMEOUT_64MIN);
    zb_set_keepalive_timeout(ZB_MILLISECONDS_TO_BEACON_INTERVAL(3000));*/
/*  zb_set_rx_on_when_idle(ZB_TRUE);*/
/*  ZB_NIB().security_level = 0;*/

  /****************** Register Device ********************************/
  /** [REGISTER] */
  ZB_AF_REGISTER_DEVICE_CTX(&device_ctx);
  ZB_AF_SET_ENDPOINT_HANDLER(ZED_HA_EP, zcl_specific_cluster_cmd_handler);
  zb_set_long_address(g_ed_addr);
  ZB_ZCL_REGISTER_DEVICE_CB(sample_zcl_cmd_device_cb);

  zb_set_network_ed_role(DEV_CHANNEL_MASK );

  /* Act as Sleepy End Device */
  zb_set_rx_on_when_idle(ZB_TRUE);

#ifdef ZB_ZCL_ENABLE_WWAH_SERVER
  zb_zcl_wwah_init_server_attr();
  setup_debug_report();
#endif

  if (zboss_start_no_autostart() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zboss_start_no_autostart() failed", (FMT__0));
  }
  else
  {
    zboss_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}
/**
   Callback called when match desc req seeking for WWAH cluster is completed.
*/
static void wwah_match_desc_cb(zb_uint8_t param)
{
  zb_bufid_t buf = param;
  zb_uint8_t *zdp_cmd = zb_buf_begin(buf);
  zb_zdo_match_desc_resp_t *resp = (zb_zdo_match_desc_resp_t*)zdp_cmd;

  if (resp->status == ZB_ZDP_STATUS_SUCCESS && resp->match_len == 1)
{
#ifdef ZB_ZCL_SUPPORT_CLUSTER_WWAH
    DST_ADDR = resp->nwk_addr;
    DST_EP = *((zb_uint8_t*)(resp + 1));
#endif
    ZB_SCHEDULE_APP_ALARM_CANCEL(button_press_handler, ZB_ALARM_ANY_PARAM);
    ZB_SCHEDULE_APP_ALARM(button_press_handler, 0, 1 * ZB_TIME_ONE_SECOND);
}
  zb_buf_free(buf);
}

/**
   Seek for WWAH cluster at TC.
*/
static void send_wwah_match_desc(zb_uint8_t param)
{
  if (!param)
  {
    zb_buf_get_out_delayed(send_wwah_match_desc);
  }
  else
  {
    zb_bufid_t buf = param;
    zb_zdo_match_desc_param_t *req;

    TRACE_MSG(TRACE_APP1, "send_wwah_match_desc", (FMT__0));

    req = zb_buf_initial_alloc(buf, sizeof(zb_zdo_match_desc_param_t) + 1 * sizeof(zb_uint16_t));

    req->nwk_addr = 0;          /* coordinator */
    req->addr_of_interest = req->nwk_addr;
    req->profile_id = ZB_AF_HA_PROFILE_ID;
    req->num_in_clusters = 0;
    req->num_out_clusters = 1;
    req->cluster_list[0] = ZB_ZCL_CLUSTER_ID_WWAH;

    zb_zdo_match_desc_req(param, wwah_match_desc_cb);

  }
}

/*********************  Device-specific functions  **************************/
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
  zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);
  zb_uint8_t seq_number;
  zb_uint16_t dst_addr;
  zb_uint8_t dst_ep;
  zb_uint8_t src_ep;
  zb_zcl_parse_status_t parse_status;
  zb_zcl_status_t zcl_status;
  zb_bool_t processed = ZB_FALSE;
  zb_uindex_t i;
  zb_uint8_t report_id;

  TRACE_MSG(TRACE_APP1, "> zcl_specific_cluster_cmd_handler %i", (FMT__H, param));
  ZB_ZCL_DEBUG_DUMP_HEADER(cmd_info);
  TRACE_MSG(TRACE_APP1, "payload size: %i", (FMT__D, zb_buf_len(param)));

  if (cmd_info->cmd_id == ZB_ZCL_CMD_DEFAULT_RESP && cmd_info->is_common_command)
  {
    zb_buf_free(param);
    processed = ZB_TRUE;
  }
  else
  if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_SRV &&
      cmd_info->cluster_id == ZB_ZCL_CLUSTER_ID_WWAH &&
      cmd_info->cmd_id == ZB_ZCL_CMD_WWAH_DEBUG_REPORT_QUERY_ID)
  {
    ZB_ZCL_WWAH_GET_DEBUG_REPORT_QUERY(param, report_id, parse_status);
    seq_number = cmd_info->seq_number;
    dst_addr = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr;
    dst_ep =  ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint;
    src_ep = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).dst_endpoint;
    if (parse_status != ZB_ZCL_PARSE_STATUS_SUCCESS)
    {
      zcl_status = ZB_ZCL_STATUS_MALFORMED_CMD;
    }
    else
    {
      zcl_status = ZB_ZCL_STATUS_NOT_FOUND;
      for (i = 0; i <= DEBUG_REPORT_TABLE_SIZE; ++i)
      {
        if (debug_report_table[i].report_id == report_id)
        {
          ZB_ZCL_WWAH_SEND_DEBUG_REPORT_QUERY_RESPONSE(param,
            seq_number,
            dst_addr,
            ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
            dst_ep,
            src_ep,
            ZB_AF_HA_PROFILE_ID,
            NULL,
            report_id,
            debug_report_table[i].report,
            debug_report_table[i].report_size);
          zcl_status = ZB_ZCL_STATUS_SUCCESS;
          break;
        }
      }
    }
    if(zcl_status != ZB_ZCL_STATUS_SUCCESS)
    {
      ZB_ZCL_SEND_DEFAULT_RESP(
        param,
        dst_addr,
        ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
        dst_ep,
        src_ep,
        ZB_AF_HA_PROFILE_ID,
        ZB_ZCL_CLUSTER_ID_WWAH,
        seq_number,
        ZB_ZCL_CMD_WWAH_DEBUG_REPORT_QUERY_ID,
        zcl_status);
    }
    processed = ZB_TRUE;
  }
  TRACE_MSG(TRACE_APP1, "< zcl_specific_cluster_cmd_handler %i", (FMT__H, param));
  return processed;
}

void test_timer_next_step(zb_uint8_t param)
{
  ZVUNUSED(param);
  zb_buf_get_out_delayed(test_next_step);
}

void test_next_step(zb_uint8_t param)
{
  zb_bufid_t buffer = param;

  TRACE_MSG(TRACE_APP3, "> test_next_step param %hd step %ld", (FMT__H_L, param, g_test_step));
  switch (g_test_step)
  {
  case TEST_STEP_WWAH_SEND_POWER_DESCRIPTOR_CHANGE:
    {
      TRACE_MSG(TRACE_APP1, "Send Power Descriptor Change command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      zb_set_node_power_descriptor(ZB_POWER_MODE_COME_ON_WHEN_STIMULATED, ZB_POWER_SRC_CONSTANT | ZB_POWER_SRC_RECHARGEABLE_BATTERY, ZB_POWER_SRC_RECHARGEABLE_BATTERY, ZB_POWER_LEVEL_66);
    }
    break;
  case TEST_STEP_WWAH_SEND_NEW_DEBUG_REPORT_NOTIFICATION:
    {
      TRACE_MSG(TRACE_APP1, "Send New Debug Report Notification command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_NEW_DEBUG_REPORT_NOTIFICATION(buffer, DST_ADDR, DST_ADDR_MODE, DST_EP, ZED_HA_EP, ZB_AF_HA_PROFILE_ID, NULL, debug_report_table[0].report_id, &debug_report_table[0].report_size);
    }
    break;
  default:
    g_test_step = TEST_STEP_FINISHED;
    TRACE_MSG(TRACE_ERROR, "ERROR step %hd shan't be processed", (FMT__H, g_test_step));
    break;
  }

  TRACE_MSG(TRACE_APP3, "< test_next_step. Curr step %hd" , (FMT__H, g_test_step));
}/* void test_next_step(zb_uint8_t param) */


void button_press_handler(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ERROR, ">> button_press_handler %hd", (FMT__H, param));
  if (!param)
  {
    /* Button is pressed, get buffer for outgoing command */
    zb_buf_get_out_delayed(button_press_handler);
  }
  else
  {
    if (g_test_step == TEST_STEP_FINISHED /*|| g_error_cnt*/)
    {
      TRACE_MSG(TRACE_ERROR, "Test finished. Status: OK", (FMT__0));
      zb_buf_free(param);
    }
    else
    {
#ifndef ZB_USE_BUTTONS
      /* Do not have buttons in simulator - just start periodic on/off sending */
      ZB_SCHEDULE_APP_ALARM(button_press_handler, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(1300));
#endif
      test_next_step(param);
    }
  }
}

/** Application signal handler. Used for informing application about important ZBOSS
    events/states (device started first time/rebooted, key establishment completed etc).
    Application may ignore signals in which it is not interested.
*/
void zboss_signal_handler(zb_uint8_t param)
{
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

#ifdef ZB_USE_BUTTONS
  /* Now register handlers for buttons */
  zb_int32_t i;
  for (i = 0; i < ZB_N_BUTTONS; ++i)
  {
    zb_button_register_handler(i, 0, button_press_handler);
  }
#endif

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_SKIP_STARTUP:
        TRACE_MSG(TRACE_APP1, "ZB_ZDO_SIGNAL_SKIP_STARTUP: boot, not started yet", (FMT__0));
#ifdef TEST_INSTALLCODES
        zb_secur_ic_str_set(ed1_installcode);
#endif
        zboss_start_continue();
        break;

      case ZB_SIGNAL_DEVICE_FIRST_START:
      case ZB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APP1, "Device STARTED OK: first_start %hd",
                  (FMT__H, sig == ZB_SIGNAL_DEVICE_FIRST_START));
      {
          ZB_SCHEDULE_APP_ALARM(send_wwah_match_desc, 0, ZB_TIME_ONE_SECOND);
      }
      break;
      default:
        TRACE_MSG(TRACE_APP1, "zboss_signal_handler: skip sig %hd status %hd",
                  (FMT__H_H, sig, ZB_GET_APP_SIGNAL_STATUS(param)));
      }
  }
  else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
  {
    TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
  }

  if (param)
  {
    zb_buf_free(param);
  }
}
