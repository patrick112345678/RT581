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
/* PURPOSE: ZB Simple switch device
*/
#define ZB_TRACE_FILE_ID 40225
#include "zboss_api.h"
#include "zb_led_button.h"
#include "zb_ha_test_sample_1.h"

/** Test step enumeration. */
enum test_step_e
{
  TEST_STEP_READ_BASIC_ZCL_VERSION, /** Read ZCLVersion attribute from Basic cluster */
  TEST_STEP_BASIC_SEND_RESET_REQ, /** Send Reset to Factory Defaults command to Basic cluster */
  TEST_STEP_READ_BASIC_APPLICATION_VERSION, /** Read ApplicationVersion attribute from Basic cluster */
  TEST_STEP_READ_BASIC_STACK_VERSION, /** Read StackVersion attribute from Basic cluster */
  TEST_STEP_READ_BASIC_HW_VERSION, /** Read HWVersion attribute from Basic cluster */
  TEST_STEP_READ_BASIC_MANUFACTURER_NAME, /** Read ManufacturerName attribute from Basic cluster */
  TEST_STEP_READ_BASIC_MODEL_IDENTIFIER, /** Read ModelIdentifier attribute from Basic cluster */
  TEST_STEP_READ_BASIC_DATE_CODE, /** Read DateCode attribute from Basic cluster */
  TEST_STEP_READ_BASIC_POWER_SOURCE, /** Read PowerSource attribute from Basic cluster */
  TEST_STEP_READ_BASIC_LOCATION_DESCRIPTION, /** Read LocationDescription attribute from Basic cluster */
  TEST_STEP_WRITE_BASIC_LOCATION_DESCRIPTION, /** Write LocationDescription attribute to Basic cluster */
  TEST_STEP_READ_BASIC_PHYSICAL_ENVIRONMENT, /** Read PhysicalEnvironment attribute from Basic cluster */
  TEST_STEP_WRITE_BASIC_PHYSICAL_ENVIRONMENT, /** Write PhysicalEnvironment attribute to Basic cluster */
  TEST_STEP_READ_IDENTIFY_IDENTIFY_TIME, /** Read IdentifyTime attribute from Identify cluster */
  TEST_STEP_WRITE_IDENTIFY_IDENTIFY_TIME, /** Write IdentifyTime attribute to Identify cluster */
  TEST_STEP_IDENTIFY_SEND_IDENTIFY_REQ, /** Send Identify command to Identify cluster */
  TEST_STEP_IDENTIFY_SEND_IDENTIFY_QUERY_REQ, /** Send Identify Query command to Identify cluster */
  TEST_STEP_IDENTIFY_SEND_TRIGGER_VARIANT_REQ, /** Send Trigger effect command to Identify cluster */
  /* TEST_STEP_READ_ALARMS_ALARM_COUNT, /\** Read AlarmCount attribute from Alarms cluster *\/ */
  TEST_STEP_ALARMS_SEND_RESET_ALARM_REQ, /** Send Reset Alarm command to Alarms cluster */
  TEST_STEP_ALARMS_SEND_RESET_ALL_ALARMS_REQ, /** Send Reset all alarms command to Alarms cluster */
  /* TEST_STEP_ALARMS_SEND_GET_ALARM_REQ, /\** Send Get Alarm command to Alarms cluster *\/ */
  /* TEST_STEP_ALARMS_SEND_RESET_ALARM_LOG_REQ, /\** Send Reset alarm log command to Alarms cluster *\/ */
  TEST_STEP_READ_TIME_TIME, /** Read Time attribute from Time cluster */
  TEST_STEP_WRITE_TIME_TIME, /** Write Time attribute to Time cluster */
  TEST_STEP_READ_TIME_TIME_STATUS, /** Read TimeStatus attribute from Time cluster */
  TEST_STEP_WRITE_TIME_TIME_STATUS, /** Write TimeStatus attribute to Time cluster */
  TEST_STEP_READ_TIME_TIME_ZONE, /** Read TimeZone attribute from Time cluster */
  TEST_STEP_WRITE_TIME_TIME_ZONE, /** Write TimeZone attribute to Time cluster */
  TEST_STEP_READ_TIME_DST_START, /** Read DstStart attribute from Time cluster */
  TEST_STEP_WRITE_TIME_DST_START, /** Write DstStart attribute to Time cluster */
  TEST_STEP_READ_TIME_DST_END, /** Read DstEnd attribute from Time cluster */
  TEST_STEP_WRITE_TIME_DST_END, /** Write DstEnd attribute to Time cluster */
  TEST_STEP_READ_TIME_DST_SHIFT, /** Read DstShift attribute from Time cluster */
  TEST_STEP_WRITE_TIME_DST_SHIFT, /** Write DstShift attribute to Time cluster */
  TEST_STEP_READ_TIME_STANDARD_TIME, /** Read StandardTime attribute from Time cluster */
  TEST_STEP_READ_TIME_LOCAL_TIME, /** Read LocalTime attribute from Time cluster */
  TEST_STEP_READ_TIME_LAST_SET_TIME, /** Read LastSetTime attribute from Time cluster */
  TEST_STEP_READ_TIME_VALID_UNTIL_TIME, /** Read ValidUntilTime attribute from Time cluster */
  TEST_STEP_WRITE_TIME_VALID_UNTIL_TIME, /** Write ValidUntilTime attribute to Time cluster */
  TEST_STEP_READ_DIAGNOSTICS_NUMBER_OF_RESETS, /** Read number_of_resets attribute from Diagnostics cluster */
  TEST_STEP_READ_DIAGNOSTICS_MAC_RX_BCAST, /** Read MacRxBcast attribute from Diagnostics cluster */
  TEST_STEP_READ_DIAGNOSTICS_MAC_TX_BCAST, /** Read MacTxBcast attribute from Diagnostics cluster */
  TEST_STEP_READ_DIAGNOSTICS_MAC_TX_UCAST, /** Read MacTxUcast attribute from Diagnostics cluster */
  TEST_STEP_READ_DIAGNOSTICS_APS_TX_BCAST, /** Read aps_tx_bcast attribute from Diagnostics cluster */
  TEST_STEP_READ_DIAGNOSTICS_APS_TX_UCAST_SUCCESS, /** Read aps_tx_ucast_success attribute from Diagnostics cluster */
  TEST_STEP_READ_DIAGNOSTICS_APS_TX_UCAST_FAIL, /** Read aps_tx_ucast_fail attribute from Diagnostics cluster */
  TEST_STEP_READ_DIAGNOSTICS_JOIN_INDICATION, /** Read join_indication attribute from Diagnostics cluster */
  TEST_STEP_READ_DIAGNOSTICS_PACKET_BUFFER_ALLOCATE_FAILURES, /** Read packet_buffer_allocate_failures attribute from Diagnostics cluster */
  TEST_STEP_READ_DIAGNOSTICS_AVERAGE_MAC_RETRY_PER_APS, /** Read AverageMACRetryPerAPS attribute from Diagnostics cluster */
  TEST_STEP_READ_DIAGNOSTICS_LAST_LQI, /** Read LastLQI attribute from Diagnostics cluster */
  TEST_STEP_READ_DIAGNOSTICS_LAST_RSSI, /** Read LastRSSI attribute from Diagnostics cluster */
  TEST_STEP_READ_DEHUMIDIFICATION_CONTROL_DEHUMIDIFICATION_COOLING, /** Read DehumidificationCooling attribute from Dehumidification Control cluster */
  TEST_STEP_READ_DEHUMIDIFICATION_CONTROL_RHDEHUMIDIFICATION_SETPOINT, /** Read rhdehumidificationSetpoint attribute from Dehumidification Control cluster */
  TEST_STEP_WRITE_DEHUMIDIFICATION_CONTROL_RHDEHUMIDIFICATION_SETPOINT, /** Write rhdehumidificationSetpoint attribute to Dehumidification Control cluster */
  TEST_STEP_READ_DEHUMIDIFICATION_CONTROL_DEHUMIDIFICATION_HYSTERESIS, /** Read DehumidificationHysteresis attribute from Dehumidification Control cluster */
  TEST_STEP_WRITE_DEHUMIDIFICATION_CONTROL_DEHUMIDIFICATION_HYSTERESIS, /** Write DehumidificationHysteresis attribute to Dehumidification Control cluster */
  TEST_STEP_READ_DEHUMIDIFICATION_CONTROL_DEHUMIDIFICATION_MAX_COOL, /** Read DehumidificationMaxCool attribute from Dehumidification Control cluster */
  TEST_STEP_WRITE_DEHUMIDIFICATION_CONTROL_DEHUMIDIFICATION_MAX_COOL, /** Write DehumidificationMaxCool attribute to Dehumidification Control cluster */
  TEST_STEP_FINISHED    /**< Test finished pseudo-step. */
};
#define ZB_ZCL_ALARMS_ALARM_COUNT_MAX_VALUE 2
/******************* Declare router parameters *****************/
zb_uint16_t g_dst_addr = 0;

#define DST_ADDR g_dst_addr
#define DST_ADDR_MODE ZB_APS_ADDR_MODE_16_ENDP_PRESENT
#define ENDPOINT_C 5
#define ENDPOINT_ED 10

zb_uint32_t g_test_step = 0;
zb_uint8_t g_error_cnt = 0;


void button_press_handler(zb_uint8_t param);

/* #if ! defined ZB_ROUTER_ROLE */
/* #error define ZB_ROUTER_ROLE to compile ze tests */
/* #endif */

/* Handler for specific zcl commands */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);

/* Parse read attributes response */
void on_off_read_attr_resp_handler(zb_bufid_t cmd_buf);

/** Start device parameters checker. */
void test_check_start_status(zb_uint8_t param);

/** Next test step initiator. */
void test_next_step(zb_uint8_t param);

/** Send Write Attribute command . */
void send_write_attr(zb_bufid_t buffer,zb_uint16_t clusterID, zb_uint16_t attributeID, zb_uint8_t attrType, zb_uint8_t *attrVal);

/** Send Read Attribute command . */
void send_read_attr(zb_bufid_t buffer, zb_uint16_t clusterID, zb_uint16_t attributeID);

void test_restart_join_nwk(zb_uint8_t param);

zb_bool_t cmd_in_progress = ZB_FALSE;

/******************* Declare attributes ************************/
/********************* Declare device **************************/
#define ZB_SWITCH_ENDPOINT          10

ZB_HA_DECLARE_HA_TEST_SAMPLE_1_CLUSTER_LIST_ZED(ha_test_sample_1_clusters);
ZB_HA_DECLARE_HA_TEST_SAMPLE_1_EP_ZED(ha_test_sample_1_ep, ZB_SWITCH_ENDPOINT, ha_test_sample_1_clusters);
ZB_HA_DECLARE_HA_TEST_SAMPLE_1_CTX(device_ctx, ha_test_sample_1_ep);
zb_ieee_addr_t g_ed_addr = {0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22};


void send_read_attr(zb_bufid_t buffer, zb_uint16_t clusterID, zb_uint16_t attributeID)
{
  zb_uint8_t *cmd_ptr;
  ZB_ZCL_GENERAL_INIT_READ_ATTR_REQ((buffer), cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);
  ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, (attributeID));
  ZB_ZCL_GENERAL_SEND_READ_ATTR_REQ(
      (buffer), cmd_ptr, DST_ADDR, DST_ADDR_MODE, ENDPOINT_C, ENDPOINT_ED,
       ZB_AF_HA_PROFILE_ID, (clusterID), NULL);
}

void send_write_attr(zb_bufid_t buffer,zb_uint16_t clusterID, zb_uint16_t attributeID, zb_uint8_t attrType, zb_uint8_t *attrVal)
{
  zb_uint8_t *cmd_ptr;
  ZB_ZCL_GENERAL_INIT_WRITE_ATTR_REQ((buffer), cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);
  ZB_ZCL_GENERAL_ADD_VALUE_WRITE_ATTR_REQ(cmd_ptr, (attributeID), (attrType), (attrVal));
  ZB_ZCL_GENERAL_SEND_WRITE_ATTR_REQ((buffer), cmd_ptr, DST_ADDR, DST_ADDR_MODE,
    ENDPOINT_C, ENDPOINT_ED, ZB_AF_HA_PROFILE_ID, (clusterID), NULL);
}

MAIN()
{
  ARGV_UNUSED;

  ZB_SET_TRACE_ON();
  ZB_SET_TRAF_DUMP_OFF();

  ZB_INIT("sample_zed");
  zb_set_long_address(g_ed_addr);
  zb_set_network_ed_role(1l<<24);
  zb_set_nvram_erase_at_start(ZB_TRUE);

  zb_set_ed_timeout(ED_AGING_TIMEOUT_64MIN);
  zb_set_keepalive_timeout(ZB_MILLISECONDS_TO_BEACON_INTERVAL(3000));
  zb_set_rx_on_when_idle(ZB_TRUE);

  /****************** Register Device ********************************/
  /** [REGISTER] */
  ZB_AF_REGISTER_DEVICE_CTX(&device_ctx);
  ZB_AF_SET_ENDPOINT_HANDLER(ZB_SWITCH_ENDPOINT, zcl_specific_cluster_cmd_handler);
  /** [REGISTER] */

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

static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster)
{
  TRACE_MSG(TRACE_APP1, "finding_binding_cb status %d addr " TRACE_FORMAT_64 " ep %hd cluster %d",
            (FMT__D_A_H_D, status, TRACE_ARG_64(addr), ep, cluster));
  return ZB_TRUE;
}

zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
  zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);
  zb_bool_t unknown_cmd_received = ZB_TRUE;


  TRACE_MSG(TRACE_APP1, "> zcl_specific_cluster_cmd_handler %i", (FMT__H, param));
  ZB_ZCL_DEBUG_DUMP_HEADER(cmd_info);
  TRACE_MSG(TRACE_APP1, "payload size: %i", (FMT__D, zb_buf_len(param)));

  if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI)
  {
    if (cmd_info->cmd_id == ZB_ZCL_CMD_DEFAULT_RESP)
    {
      unknown_cmd_received = ZB_FALSE;

      cmd_in_progress = ZB_FALSE;

      zb_buf_free(param);
    }
  }

  TRACE_MSG(TRACE_APP1, "< zcl_specific_cluster_cmd_handler %i", (FMT__H, param));
  return ! unknown_cmd_received;
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
  case TEST_STEP_READ_BASIC_ZCL_VERSION:
    TRACE_MSG(TRACE_APP1, "Read ZCLVersion attribute from Basic cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_BASIC, ZB_ZCL_ATTR_BASIC_ZCL_VERSION_ID);
    break;
  case TEST_STEP_BASIC_SEND_RESET_REQ:
    {
      TRACE_MSG(TRACE_APP1, "Send Reset to Factory Defaults command to Basic cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_BASIC_SEND_RESET_REQ(buffer, DST_ADDR, DST_ADDR_MODE, ENDPOINT_C, ENDPOINT_ED, ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
    }
    break;
  case TEST_STEP_READ_BASIC_APPLICATION_VERSION:
    TRACE_MSG(TRACE_APP1, "Read ApplicationVersion attribute from Basic cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_BASIC, ZB_ZCL_ATTR_BASIC_APPLICATION_VERSION_ID);
    break;
  case TEST_STEP_READ_BASIC_STACK_VERSION:
    TRACE_MSG(TRACE_APP1, "Read StackVersion attribute from Basic cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_BASIC, ZB_ZCL_ATTR_BASIC_STACK_VERSION_ID);
    break;
  case TEST_STEP_READ_BASIC_HW_VERSION:
    TRACE_MSG(TRACE_APP1, "Read HWVersion attribute from Basic cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_BASIC, ZB_ZCL_ATTR_BASIC_HW_VERSION_ID);
    break;
  case TEST_STEP_READ_BASIC_MANUFACTURER_NAME:
    TRACE_MSG(TRACE_APP1, "Read ManufacturerName attribute from Basic cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_BASIC, ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID);
    break;
  case TEST_STEP_READ_BASIC_MODEL_IDENTIFIER:
    TRACE_MSG(TRACE_APP1, "Read ModelIdentifier attribute from Basic cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_BASIC, ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID);
    break;
  case TEST_STEP_READ_BASIC_DATE_CODE:
    TRACE_MSG(TRACE_APP1, "Read DateCode attribute from Basic cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_BASIC, ZB_ZCL_ATTR_BASIC_DATE_CODE_ID);
    break;
  case TEST_STEP_READ_BASIC_POWER_SOURCE:
    TRACE_MSG(TRACE_APP1, "Read PowerSource attribute from Basic cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_BASIC, ZB_ZCL_ATTR_BASIC_POWER_SOURCE_ID);
    break;
  case TEST_STEP_READ_BASIC_LOCATION_DESCRIPTION:
    TRACE_MSG(TRACE_APP1, "Read LocationDescription attribute from Basic cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_BASIC, ZB_ZCL_ATTR_BASIC_LOCATION_DESCRIPTION_ID);
    break;
  case TEST_STEP_WRITE_BASIC_LOCATION_DESCRIPTION:
    {
      zb_char_t basic_location_description_def[] = ZB_ZCL_BASIC_LOCATION_DESCRIPTION_DEFAULT_VALUE;
      TRACE_MSG(TRACE_APP1, "Write LocationDescription attribute to Basic cluster", (FMT__0));
      g_test_step++;
      send_write_attr(buffer, ZB_ZCL_CLUSTER_ID_BASIC, ZB_ZCL_ATTR_BASIC_LOCATION_DESCRIPTION_ID, ZB_ZCL_ATTR_TYPE_CHAR_STRING, (zb_uint8_t *)basic_location_description_def);
    }
    break;
  case TEST_STEP_READ_BASIC_PHYSICAL_ENVIRONMENT:
    TRACE_MSG(TRACE_APP1, "Read PhysicalEnvironment attribute from Basic cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_BASIC, ZB_ZCL_ATTR_BASIC_PHYSICAL_ENVIRONMENT_ID);
    break;
  case TEST_STEP_WRITE_BASIC_PHYSICAL_ENVIRONMENT:
    {
      zb_uint8_t basic_physical_environment_def = ZB_ZCL_BASIC_PHYSICAL_ENVIRONMENT_DEFAULT_VALUE;
      TRACE_MSG(TRACE_APP1, "Write PhysicalEnvironment attribute to Basic cluster", (FMT__0));
      g_test_step++;
      send_write_attr(buffer, ZB_ZCL_CLUSTER_ID_BASIC, ZB_ZCL_ATTR_BASIC_PHYSICAL_ENVIRONMENT_ID, ZB_ZCL_ATTR_TYPE_8BIT_ENUM, &basic_physical_environment_def);
    }
    break;
  case TEST_STEP_READ_IDENTIFY_IDENTIFY_TIME:
    TRACE_MSG(TRACE_APP1, "Read IdentifyTime attribute from Identify cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_IDENTIFY, ZB_ZCL_ATTR_IDENTIFY_IDENTIFY_TIME_ID);
    break;
  case TEST_STEP_WRITE_IDENTIFY_IDENTIFY_TIME:
    {
      zb_uint16_t identify_identify_time_def = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;
      TRACE_MSG(TRACE_APP1, "Write IdentifyTime attribute to Identify cluster", (FMT__0));
      g_test_step++;
      send_write_attr(buffer, ZB_ZCL_CLUSTER_ID_IDENTIFY, ZB_ZCL_ATTR_IDENTIFY_IDENTIFY_TIME_ID, ZB_ZCL_ATTR_TYPE_U16, (zb_uint8_t *)&identify_identify_time_def);
    }
    break;
  case TEST_STEP_IDENTIFY_SEND_IDENTIFY_REQ:
    {
      TRACE_MSG(TRACE_APP1, "Send Identify command to Identify cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_IDENTIFY_SEND_IDENTIFY_REQ(buffer, 0, DST_ADDR, DST_ADDR_MODE, ENDPOINT_C, ENDPOINT_ED, ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
    }
    break;
  case TEST_STEP_IDENTIFY_SEND_IDENTIFY_QUERY_REQ:
    {
      TRACE_MSG(TRACE_APP1, "Send Identify Query command to Identify cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_IDENTIFY_SEND_IDENTIFY_QUERY_REQ(buffer, DST_ADDR, DST_ADDR_MODE, ENDPOINT_C, ENDPOINT_ED, ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
    }
    break;
  case TEST_STEP_IDENTIFY_SEND_TRIGGER_VARIANT_REQ:
    {
      TRACE_MSG(TRACE_APP1, "Send Trigger effect command to Identify cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_IDENTIFY_SEND_TRIGGER_VARIANT_REQ(buffer, DST_ADDR, DST_ADDR_MODE, ENDPOINT_C, ENDPOINT_ED, ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, 0, 0);
    }
    break;
  /* case TEST_STEP_READ_ALARMS_ALARM_COUNT: */
  /*   TRACE_MSG(TRACE_APP1, "Read AlarmCount attribute from Alarms cluster", (FMT__0)); */
  /*   g_test_step++; */
  /*   send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_ALARMS, ZB_ZCL_ATTR_ALARMS_ALARM_COUNT_ID); */
  /*   break; */
  case TEST_STEP_ALARMS_SEND_RESET_ALARM_REQ:
    {
      TRACE_MSG(TRACE_APP1, "Send Reset Alarm command to Alarms cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_ALARMS_SEND_RESET_ALARM_REQ(buffer, DST_ADDR, DST_ADDR_MODE, ENDPOINT_C, ENDPOINT_ED, ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, 0, ZB_ZCL_CLUSTER_ID_ALARMS);
    }
    break;
  case TEST_STEP_ALARMS_SEND_RESET_ALL_ALARMS_REQ:
    {
      TRACE_MSG(TRACE_APP1, "Send Reset all alarms command to Alarms cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_ALARMS_SEND_RESET_ALL_ALARMS_REQ(buffer, DST_ADDR, DST_ADDR_MODE, ENDPOINT_C, ENDPOINT_ED, ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
    }
    break;
  /* case TEST_STEP_ALARMS_SEND_GET_ALARM_REQ: */
  /*   { */
  /*     TRACE_MSG(TRACE_APP1, "Send Get Alarm command to Alarms cluster", (FMT__0)); */
  /*     g_test_step++; */
  /*     ZB_ZCL_ALARMS_SEND_GET_ALARM_REQ(buffer, DST_ADDR, DST_ADDR_MODE, ENDPOINT_C, ENDPOINT_ED, ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL); */
  /*   } */
  /*   break; */
  /* case TEST_STEP_ALARMS_SEND_RESET_ALARM_LOG_REQ: */
  /*   { */
  /*     TRACE_MSG(TRACE_APP1, "Send Reset alarm log command to Alarms cluster", (FMT__0)); */
  /*     g_test_step++; */
  /*     ZB_ZCL_ALARMS_SEND_RESET_ALARM_LOG_REQ(buffer, DST_ADDR, DST_ADDR_MODE, ENDPOINT_C, ENDPOINT_ED, ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL); */
  /*   } */
  /*   break; */
  case TEST_STEP_READ_TIME_TIME:
    TRACE_MSG(TRACE_APP1, "Read Time attribute from Time cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_TIME_ID);
    break;
  case TEST_STEP_WRITE_TIME_TIME:
    {
      zb_time_t time_time_def = ZB_ZCL_TIME_TIME_MIN_VALUE;
      TRACE_MSG(TRACE_APP1, "Write Time attribute to Time cluster", (FMT__0));
      g_test_step++;
      send_write_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_TIME_ID, ZB_ZCL_ATTR_TYPE_UTC_TIME, (zb_uint8_t *)&time_time_def);
    }
    break;
  case TEST_STEP_READ_TIME_TIME_STATUS:
    TRACE_MSG(TRACE_APP1, "Read TimeStatus attribute from Time cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_TIME_STATUS_ID);
    break;
  case TEST_STEP_WRITE_TIME_TIME_STATUS:
    {
      zb_uint8_t time_time_status_def = ZB_ZCL_TIME_TIME_STATUS_DEFAULT_VALUE;
      TRACE_MSG(TRACE_APP1, "Write TimeStatus attribute to Time cluster", (FMT__0));
      g_test_step++;
      send_write_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_TIME_STATUS_ID, ZB_ZCL_ATTR_TYPE_8BITMAP, &time_time_status_def);
    }
    break;
  case TEST_STEP_READ_TIME_TIME_ZONE:
    TRACE_MSG(TRACE_APP1, "Read TimeZone attribute from Time cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_TIME_ZONE_ID);
    break;
  case TEST_STEP_WRITE_TIME_TIME_ZONE:
    {
      zb_int32_t time_time_zone_def = ZB_ZCL_TIME_TIME_ZONE_DEFAULT_VALUE;
      TRACE_MSG(TRACE_APP1, "Write TimeZone attribute to Time cluster", (FMT__0));
      g_test_step++;
      send_write_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_TIME_ZONE_ID, ZB_ZCL_ATTR_TYPE_S32, (zb_uint8_t *)&time_time_zone_def);
    }
    break;
  case TEST_STEP_READ_TIME_DST_START:
    TRACE_MSG(TRACE_APP1, "Read DstStart attribute from Time cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_DST_START_ID);
    break;
  case TEST_STEP_WRITE_TIME_DST_START:
    {
      zb_uint32_t time_dst_start_def = ZB_ZCL_TIME_TIME_MIN_VALUE;
      TRACE_MSG(TRACE_APP1, "Write DstStart attribute to Time cluster", (FMT__0));
      g_test_step++;
      send_write_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_DST_START_ID, ZB_ZCL_ATTR_TYPE_U32, (zb_uint8_t *)&time_dst_start_def);
    }
    break;
  case TEST_STEP_READ_TIME_DST_END:
    TRACE_MSG(TRACE_APP1, "Read DstEnd attribute from Time cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_DST_END_ID);
    break;
  case TEST_STEP_WRITE_TIME_DST_END:
    {
      zb_uint32_t time_dst_end_def = ZB_ZCL_TIME_TIME_MIN_VALUE;
      TRACE_MSG(TRACE_APP1, "Write DstEnd attribute to Time cluster", (FMT__0));
      g_test_step++;
      send_write_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_DST_END_ID, ZB_ZCL_ATTR_TYPE_U32, (zb_uint8_t *)&time_dst_end_def);
    }
    break;
  case TEST_STEP_READ_TIME_DST_SHIFT:
    TRACE_MSG(TRACE_APP1, "Read DstShift attribute from Time cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_DST_SHIFT_ID);
    break;
  case TEST_STEP_WRITE_TIME_DST_SHIFT:
    {
      zb_int32_t time_dst_shift_def = ZB_ZCL_TIME_DST_SHIFT_DEFAULT_VALUE;
      TRACE_MSG(TRACE_APP1, "Write DstShift attribute to Time cluster", (FMT__0));
      g_test_step++;
      send_write_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_DST_SHIFT_ID, ZB_ZCL_ATTR_TYPE_S32, (zb_uint8_t *)&time_dst_shift_def);
    }
    break;
  case TEST_STEP_READ_TIME_STANDARD_TIME:
    TRACE_MSG(TRACE_APP1, "Read StandardTime attribute from Time cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_STANDARD_TIME_ID);
    break;
  case TEST_STEP_READ_TIME_LOCAL_TIME:
    TRACE_MSG(TRACE_APP1, "Read LocalTime attribute from Time cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_LOCAL_TIME_ID);
    break;
  case TEST_STEP_READ_TIME_LAST_SET_TIME:
    TRACE_MSG(TRACE_APP1, "Read LastSetTime attribute from Time cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_LAST_SET_TIME_ID);
    break;
  case TEST_STEP_READ_TIME_VALID_UNTIL_TIME:
    TRACE_MSG(TRACE_APP1, "Read ValidUntilTime attribute from Time cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_VALID_UNTIL_TIME_ID);
    break;
  case TEST_STEP_WRITE_TIME_VALID_UNTIL_TIME:
    {
      zb_time_t time_valid_until_time_def = ZB_ZCL_TIME_VALID_UNTIL_TIME_DEFAULT_VALUE;
      TRACE_MSG(TRACE_APP1, "Write ValidUntilTime attribute to Time cluster", (FMT__0));
      g_test_step++;
      send_write_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_VALID_UNTIL_TIME_ID, ZB_ZCL_ATTR_TYPE_UTC_TIME, (zb_uint8_t *)&time_valid_until_time_def);
    }
    break;
  case TEST_STEP_READ_DIAGNOSTICS_NUMBER_OF_RESETS:
    TRACE_MSG(TRACE_APP1, "Read number_of_resets attribute from Diagnostics cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_DIAGNOSTICS, ZB_ZCL_ATTR_DIAGNOSTICS_NUMBER_OF_RESETS_ID);
    break;
  case TEST_STEP_READ_DIAGNOSTICS_MAC_RX_BCAST:
    TRACE_MSG(TRACE_APP1, "Read MacRxBcast attribute from Diagnostics cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_DIAGNOSTICS, ZB_ZCL_ATTR_DIAGNOSTICS_MAC_RX_BCAST_ID);
    break;
  case TEST_STEP_READ_DIAGNOSTICS_MAC_TX_BCAST:
    TRACE_MSG(TRACE_APP1, "Read MacTxBcast attribute from Diagnostics cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_DIAGNOSTICS, ZB_ZCL_ATTR_DIAGNOSTICS_MAC_TX_BCAST_ID);
    break;
  case TEST_STEP_READ_DIAGNOSTICS_MAC_TX_UCAST:
    TRACE_MSG(TRACE_APP1, "Read MacTxUcast attribute from Diagnostics cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_DIAGNOSTICS, ZB_ZCL_ATTR_DIAGNOSTICS_MAC_TX_UCAST_ID);
    break;
  case TEST_STEP_READ_DIAGNOSTICS_APS_TX_BCAST:
    TRACE_MSG(TRACE_APP1, "Read aps_tx_bcast attribute from Diagnostics cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_DIAGNOSTICS, ZB_ZCL_ATTR_DIAGNOSTICS_APS_TX_BCAST_ID);
    break;
  case TEST_STEP_READ_DIAGNOSTICS_APS_TX_UCAST_SUCCESS:
    TRACE_MSG(TRACE_APP1, "Read aps_tx_ucast_success attribute from Diagnostics cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_DIAGNOSTICS, ZB_ZCL_ATTR_DIAGNOSTICS_APS_TX_UCAST_SUCCESS_ID);
    break;
  case TEST_STEP_READ_DIAGNOSTICS_APS_TX_UCAST_FAIL:
    TRACE_MSG(TRACE_APP1, "Read aps_tx_ucast_fail attribute from Diagnostics cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_DIAGNOSTICS, ZB_ZCL_ATTR_DIAGNOSTICS_APS_TX_UCAST_FAIL_ID);
    break;
  case TEST_STEP_READ_DIAGNOSTICS_JOIN_INDICATION:
    TRACE_MSG(TRACE_APP1, "Read join_indication attribute from Diagnostics cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_DIAGNOSTICS, ZB_ZCL_ATTR_DIAGNOSTICS_JOIN_INDICATION_ID);
    break;
  case TEST_STEP_READ_DIAGNOSTICS_PACKET_BUFFER_ALLOCATE_FAILURES:
    TRACE_MSG(TRACE_APP1, "Read packet_buffer_allocate_failures attribute from Diagnostics cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_DIAGNOSTICS, ZB_ZCL_ATTR_DIAGNOSTICS_PACKET_BUFFER_ALLOCATE_FAILURES_ID);
    break;
  case TEST_STEP_READ_DIAGNOSTICS_AVERAGE_MAC_RETRY_PER_APS:
    TRACE_MSG(TRACE_APP1, "Read AverageMACRetryPerAPS attribute from Diagnostics cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_DIAGNOSTICS, ZB_ZCL_ATTR_DIAGNOSTICS_AVERAGE_MAC_RETRY_PER_APS_ID);
    break;
  case TEST_STEP_READ_DIAGNOSTICS_LAST_LQI:
    TRACE_MSG(TRACE_APP1, "Read LastLQI attribute from Diagnostics cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_DIAGNOSTICS, ZB_ZCL_ATTR_DIAGNOSTICS_LAST_LQI_ID);
    break;
  case TEST_STEP_READ_DIAGNOSTICS_LAST_RSSI:
    TRACE_MSG(TRACE_APP1, "Read LastRSSI attribute from Diagnostics cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_DIAGNOSTICS, ZB_ZCL_ATTR_DIAGNOSTICS_LAST_RSSI_ID);
    break;
  case TEST_STEP_READ_DEHUMIDIFICATION_CONTROL_DEHUMIDIFICATION_COOLING:
    TRACE_MSG(TRACE_APP1, "Read DehumidificationCooling attribute from Dehumidification Control cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_DEHUMID_CONTROL, ZB_ZCL_ATTR_DEHUMIDIFICATION_CONTROL_DEHUMIDIFICATION_COOLING_ID);
    break;
  case TEST_STEP_READ_DEHUMIDIFICATION_CONTROL_RHDEHUMIDIFICATION_SETPOINT:
    TRACE_MSG(TRACE_APP1, "Read rhdehumidificationSetpoint attribute from Dehumidification Control cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_DEHUMID_CONTROL, ZB_ZCL_ATTR_DEHUMIDIFICATION_CONTROL_RHDEHUMIDIFICATION_SETPOINT_ID);
    break;
  case TEST_STEP_WRITE_DEHUMIDIFICATION_CONTROL_RHDEHUMIDIFICATION_SETPOINT:
    {
      zb_uint8_t dehumidification_control_rhdehumidification_setpoint_def = ZB_ZCL_DEHUMIDIFICATION_CONTROL_RHDEHUMIDIFICATION_SETPOINT_DEFAULT_VALUE;
      TRACE_MSG(TRACE_APP1, "Write rhdehumidificationSetpoint attribute to Dehumidification Control cluster", (FMT__0));
      g_test_step++;
      send_write_attr(buffer, ZB_ZCL_CLUSTER_ID_DEHUMID_CONTROL, ZB_ZCL_ATTR_DEHUMIDIFICATION_CONTROL_RHDEHUMIDIFICATION_SETPOINT_ID, ZB_ZCL_ATTR_TYPE_U8, &dehumidification_control_rhdehumidification_setpoint_def);
    }
    break;
  case TEST_STEP_READ_DEHUMIDIFICATION_CONTROL_DEHUMIDIFICATION_HYSTERESIS:
    TRACE_MSG(TRACE_APP1, "Read DehumidificationHysteresis attribute from Dehumidification Control cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_DEHUMID_CONTROL, ZB_ZCL_ATTR_DEHUMIDIFICATION_CONTROL_DEHUMIDIFICATION_HYSTERESIS_ID);
    break;
  case TEST_STEP_WRITE_DEHUMIDIFICATION_CONTROL_DEHUMIDIFICATION_HYSTERESIS:
    {
      zb_uint8_t dehumidification_control_dehumidification_hysteresis_def = ZB_ZCL_DEHUMIDIFICATION_CONTROL_DEHUMIDIFICATION_HYSTERESIS_DEFAULT_VALUE;
      TRACE_MSG(TRACE_APP1, "Write DehumidificationHysteresis attribute to Dehumidification Control cluster", (FMT__0));
      g_test_step++;
      send_write_attr(buffer, ZB_ZCL_CLUSTER_ID_DEHUMID_CONTROL, ZB_ZCL_ATTR_DEHUMIDIFICATION_CONTROL_DEHUMIDIFICATION_HYSTERESIS_ID, ZB_ZCL_ATTR_TYPE_U8, &dehumidification_control_dehumidification_hysteresis_def);
    }
    break;
  case TEST_STEP_READ_DEHUMIDIFICATION_CONTROL_DEHUMIDIFICATION_MAX_COOL:
    TRACE_MSG(TRACE_APP1, "Read DehumidificationMaxCool attribute from Dehumidification Control cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_DEHUMID_CONTROL, ZB_ZCL_ATTR_DEHUMIDIFICATION_CONTROL_DEHUMIDIFICATION_MAX_COOL_ID);
    break;
  case TEST_STEP_WRITE_DEHUMIDIFICATION_CONTROL_DEHUMIDIFICATION_MAX_COOL:
    {
      zb_uint8_t dehumidification_control_dehumidification_max_cool_def = ZB_ZCL_DEHUMIDIFICATION_CONTROL_DEHUMIDIFICATION_MAX_COOL_DEFAULT_VALUE;
      TRACE_MSG(TRACE_APP1, "Write DehumidificationMaxCool attribute to Dehumidification Control cluster", (FMT__0));
      g_test_step++;
      send_write_attr(buffer, ZB_ZCL_CLUSTER_ID_DEHUMID_CONTROL, ZB_ZCL_ATTR_DEHUMIDIFICATION_CONTROL_DEHUMIDIFICATION_MAX_COOL_ID, ZB_ZCL_ATTR_TYPE_U8, &dehumidification_control_dehumidification_max_cool_def);
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


void read_attr_resp_handler(zb_bufid_t  cmd_buf)
{
  zb_zcl_read_attr_res_t * read_attr_resp;

  TRACE_MSG(TRACE_APP1, ">> read_attr_resp_handler", (FMT__0));

  ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(cmd_buf, read_attr_resp);
  TRACE_MSG(TRACE_APP3, "read_attr_resp %p", (FMT__P, read_attr_resp));
  if (read_attr_resp)
  {
    if (ZB_ZCL_STATUS_SUCCESS != read_attr_resp->status)
    {
      TRACE_MSG(TRACE_ERROR,
                "ERROR incorrect response: id 0x%04x, status %d",
                (FMT__D_H, read_attr_resp->attr_id, read_attr_resp->status));
      g_error_cnt++;
    }

  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "ERROR, No info on attribute(s) read", (FMT__0));
    g_error_cnt++;
  }

  TRACE_MSG(TRACE_APP1, "<< read_attr_resp_handler", (FMT__0));
}

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

void test_restart_join_nwk(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ERROR, "test_restart_join_nwk %hd", (FMT__H, param));
  if (param == ZB_NWK_LEAVE_TYPE_RESET)
  {
    bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
  }
}

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
//! [signal_first]
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
#ifdef TEST_APS_FRAGMENTATION
        ZB_SCHEDULE_APP_ALARM(send_frag_data, 0, ZB_TIME_ONE_SECOND / 2);
#endif
        break;
//! [signal_first]
//! [signal_reboot]
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APP1, "Device RESTARTED OK", (FMT__0));
#ifndef ZB_USE_BUTTONS
        /* Do not have buttons in simulator - just start periodic on/off sending */
        cmd_in_progress = ZB_FALSE;
        ZB_SCHEDULE_APP_ALARM_CANCEL(button_press_handler, ZB_ALARM_ANY_PARAM);
        ZB_SCHEDULE_APP_ALARM(button_press_handler, 0, 7 * ZB_TIME_ONE_SECOND);
#endif
        break;
//! [signal_reboot]
      case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APP1, "Successfull steering, start f&b initiator", (FMT__0));
        zb_bdb_finding_binding_initiator(ZB_SWITCH_ENDPOINT, finding_binding_cb);
        break;

      case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:
      {
        TRACE_MSG(TRACE_APP1, "Finding&binding done", (FMT__0));
#ifndef ZB_USE_BUTTONS
        /* Do not have buttons in simulator - just start periodic on/off sending */
        cmd_in_progress = ZB_FALSE;
        ZB_SCHEDULE_APP_ALARM_CANCEL(button_press_handler, ZB_ALARM_ANY_PARAM);
        ZB_SCHEDULE_APP_ALARM(button_press_handler, 0, 7 * ZB_TIME_ONE_SECOND);
#endif
      }
      break;

      case ZB_ZDO_SIGNAL_LEAVE:
      {
        zb_zdo_signal_leave_params_t *leave_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_leave_params_t);
        test_restart_join_nwk(leave_params->leave_type);
      }
      break;

      case ZB_COMMON_SIGNAL_CAN_SLEEP:
      {
        /* zb_zdo_signal_can_sleep_params_t *can_sleep_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_can_sleep_params_t); */
#ifdef ZB_USE_SLEEP
        zb_sleep_now();
#endif
        break;
      }

      case ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY:
      {
        TRACE_MSG(TRACE_APP1, "Production config is ready", (FMT__0));
        break;
      }

      default:
        TRACE_MSG(TRACE_ERROR, "Unknown signal %hd, do nothing", (FMT__H, sig));
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
