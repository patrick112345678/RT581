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
#define ZB_TRACE_FILE_ID 40227
#include "zboss_api.h"
#include "zb_led_button.h"
#include "zb_ha_write_attr_test.h"

#include "../common/zcl_basic_attr_list.h"

/** Test step enumeration. */
enum test_step_e
{
  TEST_STEP_SEND_READ_ATTR_1,
  TEST_STEP_SEND_READ_ATTR_2,
  TEST_STEP_SEND_READ_ATTR_3,
  TEST_STEP_SEND_READ_ATTR_4,
  TEST_STEP_SEND_READ_ATTR_5,
  TEST_STEP_SEND_READ_ATTR_6,

/* **** Default Write Attribute - 4 attrs valid **** */
  TEST_STEP_SEND_WRITE_ATTR_4_ATTRS_VALID,
  TEST_STEP_SEND_READ_ATTR_4_ATTRS_VALID_1,
  TEST_STEP_SEND_READ_ATTR_4_ATTRS_VALID_2,
  TEST_STEP_SEND_READ_ATTR_4_ATTRS_VALID_3,
  TEST_STEP_SEND_READ_ATTR_4_ATTRS_VALID_4,

/* **** Default Write Attribute - 4 attrs invalid **** */
  TEST_STEP_SEND_WRITE_ATTR_4_ATTRS_INVALID,
  TEST_STEP_SEND_READ_ATTR_4_ATTRS_INVALID_1,
  TEST_STEP_SEND_READ_ATTR_4_ATTRS_INVALID_2,
  TEST_STEP_SEND_READ_ATTR_4_ATTRS_INVALID_3,
  TEST_STEP_SEND_READ_ATTR_4_ATTRS_INVALID_4,

/* **** Default Write Attribute - 2 attrs valid, 2 attrs invalid **** */
  TEST_STEP_SEND_WRITE_ATTR_2_ATTRS_VALID_2_ATTRS_INVALID,
  TEST_STEP_SEND_READ_ATTR_2_ATTRS_VALID_2_ATTRS_INVALID_1,
  TEST_STEP_SEND_READ_ATTR_2_ATTRS_VALID_2_ATTRS_INVALID_2,
  TEST_STEP_SEND_READ_ATTR_2_ATTRS_VALID_2_ATTRS_INVALID_3,
  TEST_STEP_SEND_READ_ATTR_2_ATTRS_VALID_2_ATTRS_INVALID_4,

/* **** Undivided Write Attribute - 4 attrs valid **** */
  TEST_STEP_SEND_WRITE_ATTR_UNDIV_4_ATTRS_VALID,
  TEST_STEP_SEND_READ_ATTR_UNDIV_4_ATTRS_VALID_1,
  TEST_STEP_SEND_READ_ATTR_UNDIV_4_ATTRS_VALID_2,
  TEST_STEP_SEND_READ_ATTR_UNDIV_4_ATTRS_VALID_3,
  TEST_STEP_SEND_READ_ATTR_UNDIV_4_ATTRS_VALID_4,

/* **** Undivided Write Attribute - 4 attrs invalid **** */
  TEST_STEP_SEND_WRITE_ATTR_UNDIV_4_ATTRS_INVALID,
  TEST_STEP_SEND_READ_ATTR_UNDIV_4_ATTRS_INVALID_1,
  TEST_STEP_SEND_READ_ATTR_UNDIV_4_ATTRS_INVALID_2,
  TEST_STEP_SEND_READ_ATTR_UNDIV_4_ATTRS_INVALID_3,
  TEST_STEP_SEND_READ_ATTR_UNDIV_4_ATTRS_INVALID_4,

/* **** Undivided Write Attribute - 2 attrs valid, 2 attrs invalid **** */
  TEST_STEP_SEND_WRITE_ATTR_UNDIV_2_ATTRS_VALID_2_ATTRS_INVALID,
  TEST_STEP_SEND_READ_ATTR_UNDIV_2_ATTRS_VALID_2_ATTRS_INVALID_1,
  TEST_STEP_SEND_READ_ATTR_UNDIV_2_ATTRS_VALID_2_ATTRS_INVALID_2,
  TEST_STEP_SEND_READ_ATTR_UNDIV_2_ATTRS_VALID_2_ATTRS_INVALID_3,
  TEST_STEP_SEND_READ_ATTR_UNDIV_2_ATTRS_VALID_2_ATTRS_INVALID_4,

/* **** No Response Write Attribute - 4 attrs valid **** */
  TEST_STEP_SEND_WRITE_ATTR_NO_RESP_4_ATTRS_VALID,
  TEST_STEP_SEND_READ_ATTR_NO_RESP_4_ATTRS_VALID_1,
  TEST_STEP_SEND_READ_ATTR_NO_RESP_4_ATTRS_VALID_2,
  TEST_STEP_SEND_READ_ATTR_NO_RESP_4_ATTRS_VALID_3,
  TEST_STEP_SEND_READ_ATTR_NO_RESP_4_ATTRS_VALID_4,

/* **** No Response Write Attribute - 4 attrs invalid **** */
  TEST_STEP_SEND_WRITE_ATTR_NO_RESP_4_ATTRS_INVALID,
  TEST_STEP_SEND_READ_ATTR_NO_RESP_4_ATTRS_INVALID_1,
  TEST_STEP_SEND_READ_ATTR_NO_RESP_4_ATTRS_INVALID_2,
  TEST_STEP_SEND_READ_ATTR_NO_RESP_4_ATTRS_INVALID_3,
  TEST_STEP_SEND_READ_ATTR_NO_RESP_4_ATTRS_INVALID_4,

/* **** No Response Write Attribute - 2 attrs valid, 2 attrs invalid **** */
  TEST_STEP_SEND_WRITE_ATTR_NO_RESP_2_ATTRS_VALID_2_ATTRS_INVALID,
  TEST_STEP_SEND_READ_ATTR_NO_RESP_2_ATTRS_VALID_2_ATTRS_INVALID_1,
  TEST_STEP_SEND_READ_ATTR_NO_RESP_2_ATTRS_VALID_2_ATTRS_INVALID_2,
  TEST_STEP_SEND_READ_ATTR_NO_RESP_2_ATTRS_VALID_2_ATTRS_INVALID_3,
  TEST_STEP_SEND_READ_ATTR_NO_RESP_2_ATTRS_VALID_2_ATTRS_INVALID_4,

/* Test Global Attributes */
  TEST_STEP_READ_GLOBAL_CLUSTER_REVISION,
  TEST_STEP_WRITE_GLOBAL_CLUSTER_REVISION,
  TEST_STEP_DISCOVER_ATTR_GLOBAL_CLUSTER_REVISION,
  TEST_STEP_CONFIGURE_REPORTING_GLOBAL_CLUSTER_REVISION,

  TEST_STEP_FINISHED
};

#define ZB_ZCL_ALARMS_ALARM_COUNT_MAX_VALUE 2
/******************* Declare router parameters *****************/
zb_uint16_t g_dst_addr = 0;

#define DST_ADDR g_dst_addr
#define DST_ADDR_MODE ZB_APS_ADDR_MODE_16_ENDP_PRESENT
#define ENDPOINT_C 5
#define ENDPOINT_ED 10

zb_uint32_t g_test_step =
  TEST_STEP_SEND_READ_ATTR_1;
  /* TEST_STEP_SEND_WRITE_ATTR_4_ATTRS_VALID; */
  /* TEST_STEP_SEND_WRITE_ATTR_4_ATTRS_INVALID; */
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

ZB_HA_DECLARE_HA_WRITE_ATTR_TEST_CLUSTER_LIST_ZED(ha_test_sample_1_clusters, basic_attr_list);
ZB_HA_DECLARE_HA_WRITE_ATTR_TEST_EP_ZED(ha_test_sample_1_ep, ZB_SWITCH_ENDPOINT, ha_test_sample_1_clusters);
ZB_HA_DECLARE_HA_WRITE_ATTR_TEST_CTX(device_ctx, ha_test_sample_1_ep);
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

static zb_uint32_t time_value = 0;

void send_write_attrs_time(zb_bufid_t buf, zb_uint8_t test_step)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t *cmd_ptr;
  zb_uint32_t dst_end = 0xffffffff;

  if (test_step <= TEST_STEP_SEND_WRITE_ATTR_2_ATTRS_VALID_2_ATTRS_INVALID)
  {
    ZB_ZCL_GENERAL_INIT_WRITE_ATTR_REQ(buf, cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);
  }
  else if (test_step <= TEST_STEP_SEND_WRITE_ATTR_UNDIV_2_ATTRS_VALID_2_ATTRS_INVALID)
  {
    ZB_ZCL_GENERAL_INIT_WRITE_ATTR_REQ_UNDIV(buf, cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);
  }
  else if (test_step <= TEST_STEP_SEND_WRITE_ATTR_NO_RESP_2_ATTRS_VALID_2_ATTRS_INVALID)
  {
    ZB_ZCL_GENERAL_INIT_WRITE_ATTR_REQ_NO_RESP(buf, cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);
  }
  else
  {
    ret = RET_ERROR;
  }

  if (RET_OK == ret)
  {
    switch (test_step)
    {
      case TEST_STEP_SEND_WRITE_ATTR_4_ATTRS_VALID:
      case TEST_STEP_SEND_WRITE_ATTR_UNDIV_4_ATTRS_VALID:
      case TEST_STEP_SEND_WRITE_ATTR_NO_RESP_4_ATTRS_VALID:
        ZB_ZCL_GENERAL_ADD_VALUE_WRITE_ATTR_REQ(
          cmd_ptr, ZB_ZCL_ATTR_TIME_TIME_ID, ZB_ZCL_ATTR_TYPE_UTC_TIME, (zb_uint8_t*)&time_value);

        ZB_ZCL_GENERAL_ADD_VALUE_WRITE_ATTR_REQ(
          cmd_ptr, ZB_ZCL_ATTR_TIME_TIME_ZONE_ID, ZB_ZCL_ATTR_TYPE_S32, (zb_uint8_t*)&time_value);

        ZB_ZCL_GENERAL_ADD_VALUE_WRITE_ATTR_REQ(
          cmd_ptr, ZB_ZCL_ATTR_TIME_DST_START_ID, ZB_ZCL_ATTR_TYPE_U32, (zb_uint8_t*)&time_value);

        ZB_ZCL_GENERAL_ADD_VALUE_WRITE_ATTR_REQ(
          cmd_ptr, ZB_ZCL_ATTR_TIME_DST_END_ID, ZB_ZCL_ATTR_TYPE_U32, (zb_uint8_t*)&time_value);
      break;

      case TEST_STEP_SEND_WRITE_ATTR_4_ATTRS_INVALID:
      case TEST_STEP_SEND_WRITE_ATTR_UNDIV_4_ATTRS_INVALID:
      case TEST_STEP_SEND_WRITE_ATTR_NO_RESP_4_ATTRS_INVALID:
        /* UNSUPPORTED ATTRIBUTE error */
        ZB_ZCL_GENERAL_ADD_VALUE_WRITE_ATTR_REQ(
          cmd_ptr, ZB_ZCL_ATTR_TIME_VALID_UNTIL_TIME_ID, ZB_ZCL_ATTR_TYPE_UTC_TIME, (zb_uint8_t*)&time_value);

        /* INVALID DATA TYPE error */
        ZB_ZCL_GENERAL_ADD_VALUE_WRITE_ATTR_REQ(
          cmd_ptr, ZB_ZCL_ATTR_TIME_TIME_ZONE_ID, ZB_ZCL_ATTR_TYPE_U32, (zb_uint8_t*)&time_value);

        /* READ ONLY error */
        ZB_ZCL_GENERAL_ADD_VALUE_WRITE_ATTR_REQ(
          cmd_ptr, ZB_ZCL_ATTR_TIME_STANDARD_TIME_ID, ZB_ZCL_ATTR_TYPE_U32, (zb_uint8_t*)&time_value);

        /* INVALID VALUE error */
        ZB_ZCL_GENERAL_ADD_VALUE_WRITE_ATTR_REQ(
          cmd_ptr, ZB_ZCL_ATTR_TIME_DST_END_ID, ZB_ZCL_ATTR_TYPE_U32, (zb_uint8_t*)&dst_end);
      break;

      case TEST_STEP_SEND_WRITE_ATTR_2_ATTRS_VALID_2_ATTRS_INVALID:
      case TEST_STEP_SEND_WRITE_ATTR_UNDIV_2_ATTRS_VALID_2_ATTRS_INVALID:
      case TEST_STEP_SEND_WRITE_ATTR_NO_RESP_2_ATTRS_VALID_2_ATTRS_INVALID:
        ZB_ZCL_GENERAL_ADD_VALUE_WRITE_ATTR_REQ(
          cmd_ptr, ZB_ZCL_ATTR_TIME_TIME_ID, ZB_ZCL_ATTR_TYPE_UTC_TIME, (zb_uint8_t*)&time_value);

        ZB_ZCL_GENERAL_ADD_VALUE_WRITE_ATTR_REQ(
          cmd_ptr, ZB_ZCL_ATTR_TIME_TIME_ZONE_ID, ZB_ZCL_ATTR_TYPE_S32, (zb_uint8_t*)&time_value);

        /* READ ONLY error */
        ZB_ZCL_GENERAL_ADD_VALUE_WRITE_ATTR_REQ(
          cmd_ptr, ZB_ZCL_ATTR_TIME_STANDARD_TIME_ID, ZB_ZCL_ATTR_TYPE_U32, (zb_uint8_t*)&time_value);

        /* INVALID VALUE error */
        ZB_ZCL_GENERAL_ADD_VALUE_WRITE_ATTR_REQ(
          cmd_ptr, ZB_ZCL_ATTR_TIME_DST_END_ID, ZB_ZCL_ATTR_TYPE_U32, (zb_uint8_t*)&dst_end);
      break;

      default:
        ret = RET_ERROR;
        break;
    } /* switch (test_step) */
  } /* if (RET_OK == ret) */

  if (RET_OK == ret)
  {
    ZB_ZCL_GENERAL_SEND_WRITE_ATTR_REQ(
      buf,
      cmd_ptr,
      DST_ADDR,
      DST_ADDR_MODE,
      ENDPOINT_C,
      ENDPOINT_ED,
      ZB_AF_HA_PROFILE_ID,
      ZB_ZCL_CLUSTER_ID_TIME,
      NULL);
  }
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

  TRACE_MSG(TRACE_APP3, "> test_next_step param %hd step %hd", (FMT__H_H, param, g_test_step));

  switch (g_test_step )
  {
    case TEST_STEP_SEND_READ_ATTR_1:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_TIME_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_TIME_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_2:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_TIME_ZONE_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_TIME_ZONE_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_3:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_DST_START_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_DST_START_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_4:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_DST_END_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_DST_END_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_5:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_STANDARD_TIME_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_STANDARD_TIME_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_6:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_STANDARD_TIME_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_VALID_UNTIL_TIME_ID);
      g_test_step++;
      break;

  /* **** Default Write Attribute - 4 attrs valid **** */
    case TEST_STEP_SEND_WRITE_ATTR_4_ATTRS_VALID:
      TRACE_MSG(TRACE_APP1, "Send write attr TEST_STEP_SEND_WRITE_ATTR_4_ATTRS_VALID", (FMT__0));
      time_value = 0x01;
      send_write_attrs_time(buffer, g_test_step);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_4_ATTRS_VALID_1:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TYPE_UTC_TIME", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_TIME_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_4_ATTRS_VALID_2:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_TIME_ZONE_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_TIME_ZONE_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_4_ATTRS_VALID_3:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_DST_START_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_DST_START_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_4_ATTRS_VALID_4:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_DST_END_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_DST_END_ID);
      g_test_step++;
      break;

  /* **** Default Write Attribute - 4 attrs invalid **** */
    case TEST_STEP_SEND_WRITE_ATTR_4_ATTRS_INVALID:
      TRACE_MSG(TRACE_APP1, "Send write attr TEST_STEP_SEND_WRITE_ATTR_4_ATTRS_INVALID", (FMT__0));
      time_value = 0x02;
      send_write_attrs_time(buffer, g_test_step);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_4_ATTRS_INVALID_1:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_VALID_UNTIL_TIME_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_VALID_UNTIL_TIME_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_4_ATTRS_INVALID_2:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_TIME_ZONE_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_TIME_ZONE_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_4_ATTRS_INVALID_3:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_STANDARD_TIME_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_STANDARD_TIME_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_4_ATTRS_INVALID_4:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_DST_END_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_DST_END_ID);
      g_test_step++;
      break;

  /* **** Default Write Attribute - 2 attrs valid, 2 attrs invalid **** */
    case TEST_STEP_SEND_WRITE_ATTR_2_ATTRS_VALID_2_ATTRS_INVALID:
      TRACE_MSG(TRACE_APP1, "Send write attr TEST_STEP_SEND_WRITE_ATTR_2_ATTRS_VALID_2_ATTRS_INVALID", (FMT__0));
      time_value = 0x03;
      send_write_attrs_time(buffer, g_test_step);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_2_ATTRS_VALID_2_ATTRS_INVALID_1:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TYPE_UTC_TIME", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_TIME_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_2_ATTRS_VALID_2_ATTRS_INVALID_2:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_TIME_ZONE_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_TIME_ZONE_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_2_ATTRS_VALID_2_ATTRS_INVALID_3:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_STANDARD_TIME_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_STANDARD_TIME_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_2_ATTRS_VALID_2_ATTRS_INVALID_4:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_DST_END_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_DST_END_ID);
      g_test_step++;
      break;

  /* **** Undivided Write Attribute - 4 attrs valid **** */
    case TEST_STEP_SEND_WRITE_ATTR_UNDIV_4_ATTRS_VALID:
      TRACE_MSG(TRACE_APP1, "Send write attr TEST_STEP_SEND_WRITE_ATTR_UNDIV_4_ATTRS_VALID", (FMT__0));
      time_value = 0x04;
      send_write_attrs_time(buffer, g_test_step);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_UNDIV_4_ATTRS_VALID_1:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TYPE_UTC_TIME", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_TIME_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_UNDIV_4_ATTRS_VALID_2:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_TIME_ZONE_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_TIME_ZONE_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_UNDIV_4_ATTRS_VALID_3:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_DST_START_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_DST_START_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_UNDIV_4_ATTRS_VALID_4:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_DST_END_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_DST_END_ID);
      g_test_step++;
      break;

  /* **** Undivided Write Attribute - 4 attrs invalid **** */
    case TEST_STEP_SEND_WRITE_ATTR_UNDIV_4_ATTRS_INVALID:
      TRACE_MSG(TRACE_APP1, "Send write attr TEST_STEP_SEND_WRITE_ATTR_UNDIV_4_ATTRS_INVALID", (FMT__0));
      time_value = 0x05;
      send_write_attrs_time(buffer, g_test_step);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_UNDIV_4_ATTRS_INVALID_1:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_VALID_UNTIL_TIME_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_VALID_UNTIL_TIME_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_UNDIV_4_ATTRS_INVALID_2:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_TIME_ZONE_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_TIME_ZONE_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_UNDIV_4_ATTRS_INVALID_3:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_STANDARD_TIME_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_STANDARD_TIME_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_UNDIV_4_ATTRS_INVALID_4:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_DST_END_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_DST_END_ID);
      g_test_step++;
      break;

  /* **** Undivided Write Attribute - 2 attrs valid, 2 attrs invalid **** */
    case TEST_STEP_SEND_WRITE_ATTR_UNDIV_2_ATTRS_VALID_2_ATTRS_INVALID:
      TRACE_MSG(TRACE_APP1, "Send write attr TEST_STEP_SEND_WRITE_ATTR_UNDIV_2_ATTRS_VALID_2_ATTRS_INVALID", (FMT__0));
      time_value = 0x06;
      send_write_attrs_time(buffer, g_test_step);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_UNDIV_2_ATTRS_VALID_2_ATTRS_INVALID_1:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TYPE_UTC_TIME", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_TIME_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_UNDIV_2_ATTRS_VALID_2_ATTRS_INVALID_2:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_TIME_ZONE_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_TIME_ZONE_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_UNDIV_2_ATTRS_VALID_2_ATTRS_INVALID_3:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_STANDARD_TIME_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_STANDARD_TIME_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_UNDIV_2_ATTRS_VALID_2_ATTRS_INVALID_4:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_DST_END_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_DST_END_ID);
      g_test_step++;
      break;

  /* **** No Response Write Attribute - 4 attrs valid **** */
    case TEST_STEP_SEND_WRITE_ATTR_NO_RESP_4_ATTRS_VALID:
      TRACE_MSG(TRACE_APP1, "Send write attr TEST_STEP_SEND_WRITE_ATTR_NO_RESP_4_ATTRS_VALID", (FMT__0));
      time_value = 0x07;
      send_write_attrs_time(buffer, g_test_step);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_NO_RESP_4_ATTRS_VALID_1:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TYPE_UTC_TIME", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_TIME_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_NO_RESP_4_ATTRS_VALID_2:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_TIME_ZONE_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_TIME_ZONE_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_NO_RESP_4_ATTRS_VALID_3:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_DST_START_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_DST_START_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_NO_RESP_4_ATTRS_VALID_4:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_DST_END_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_DST_END_ID);
      g_test_step++;
      break;

  /* **** No Response Write Attribute - 4 attrs invalid **** */
    case TEST_STEP_SEND_WRITE_ATTR_NO_RESP_4_ATTRS_INVALID:
      TRACE_MSG(TRACE_APP1, "Send write attr TEST_STEP_SEND_WRITE_ATTR_NO_RESP_4_ATTRS_INVALID", (FMT__0));
      time_value = 0x08;
      send_write_attrs_time(buffer, g_test_step);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_NO_RESP_4_ATTRS_INVALID_1:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_VALID_UNTIL_TIME_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_VALID_UNTIL_TIME_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_NO_RESP_4_ATTRS_INVALID_2:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_TIME_ZONE_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_TIME_ZONE_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_NO_RESP_4_ATTRS_INVALID_3:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_STANDARD_TIME_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_STANDARD_TIME_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_NO_RESP_4_ATTRS_INVALID_4:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_DST_END_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_DST_END_ID);
      g_test_step++;
      break;

/* **** No Response Write Attribute - 2 attrs valid, 2 attrs invalid **** */
    case TEST_STEP_SEND_WRITE_ATTR_NO_RESP_2_ATTRS_VALID_2_ATTRS_INVALID:
      TRACE_MSG(TRACE_APP1, "Send write attr TEST_STEP_SEND_WRITE_ATTR_NO_RESP_2_ATTRS_VALID_2_ATTRS_INVALID", (FMT__0));
      time_value = 0x09;
      send_write_attrs_time(buffer, g_test_step);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_NO_RESP_2_ATTRS_VALID_2_ATTRS_INVALID_1:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TYPE_UTC_TIME", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_TIME_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_NO_RESP_2_ATTRS_VALID_2_ATTRS_INVALID_2:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_TIME_ZONE_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_TIME_ZONE_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_NO_RESP_2_ATTRS_VALID_2_ATTRS_INVALID_3:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_STANDARD_TIME_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_STANDARD_TIME_ID);
      g_test_step++;
      break;

    case TEST_STEP_SEND_READ_ATTR_NO_RESP_2_ATTRS_VALID_2_ATTRS_INVALID_4:
      TRACE_MSG(TRACE_APP1, "Send read attr ZB_ZCL_ATTR_TIME_DST_END_ID", (FMT__0));
      send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_TIME, ZB_ZCL_ATTR_TIME_DST_END_ID);
      g_test_step++;
      break;

    case TEST_STEP_READ_GLOBAL_CLUSTER_REVISION:
    {
/* [ZB_ZCL_PACKET] */
      zb_uint8_t *cmd_ptr;
      TRACE_MSG(TRACE_APP1, "Send read attr TEST_STEP_READ_GLOBAL_CLUSTER_REVISION", (FMT__0));
      cmd_ptr = ZB_ZCL_START_PACKET(buffer);
      ZB_ZCL_CONSTRUCT_GENERAL_COMMAND_REQ_FRAME_CONTROL_A(cmd_ptr,
                                                           ZB_ZCL_FRAME_DIRECTION_TO_CLI,
                                                           ZB_ZCL_NOT_MANUFACTURER_SPECIFIC,
                                                           ZB_ZCL_ENABLE_DEFAULT_RESPONSE);
      ZB_ZCL_CONSTRUCT_COMMAND_HEADER(cmd_ptr, ZB_ZCL_GET_SEQ_NUM(), ZB_ZCL_CMD_READ_ATTRIB);
      ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, ZB_ZCL_ATTR_GLOBAL_CLUSTER_REVISION_ID);
      ZB_ZCL_GENERAL_SEND_READ_ATTR_REQ(buffer,
                                        cmd_ptr,
                                        DST_ADDR,
                                        ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                        ENDPOINT_C,
                                        ENDPOINT_ED,
                                        ZB_AF_HA_PROFILE_ID,
                                        ZB_ZCL_CLUSTER_ID_BASIC,
                                        NULL);
/* [ZB_ZCL_PACKET] */
      g_test_step++;
    }
    break;

    case TEST_STEP_WRITE_GLOBAL_CLUSTER_REVISION:
    {
      zb_uint8_t *cmd_ptr;
      zb_uint16_t new_val = 5;

      TRACE_MSG(TRACE_APP1, "Send write attr TEST_STEP_READ_GLOBAL_CLUSTER_REVISION", (FMT__0));
      cmd_ptr = ZB_ZCL_START_PACKET(buffer);
      ZB_ZCL_CONSTRUCT_GENERAL_COMMAND_REQ_FRAME_CONTROL_A(cmd_ptr,
                                                           ZB_ZCL_FRAME_DIRECTION_TO_CLI,
                                                           ZB_ZCL_NOT_MANUFACTURER_SPECIFIC,
                                                           ZB_ZCL_ENABLE_DEFAULT_RESPONSE);
      ZB_ZCL_CONSTRUCT_COMMAND_HEADER(cmd_ptr, ZB_ZCL_GET_SEQ_NUM(), ZB_ZCL_CMD_WRITE_ATTRIB);
      ZB_ZCL_GENERAL_ADD_VALUE_WRITE_ATTR_REQ(cmd_ptr,
                                              ZB_ZCL_ATTR_GLOBAL_CLUSTER_REVISION_ID,
                                              ZB_ZCL_ATTR_TYPE_U16,
                                              (zb_uint8_t*)&new_val);
      ZB_ZCL_GENERAL_SEND_WRITE_ATTR_REQ(buffer,
                                         cmd_ptr,
                                         DST_ADDR,
                                         ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                         ENDPOINT_C,
                                         ENDPOINT_ED,
                                         ZB_AF_HA_PROFILE_ID,
                                         ZB_ZCL_CLUSTER_ID_BASIC,
                                         NULL);
      g_test_step++;
    }
    break;

    case TEST_STEP_DISCOVER_ATTR_GLOBAL_CLUSTER_REVISION:
    {
      zb_uint8_t *cmd_ptr;

      ZVUNUSED(cmd_ptr);
      ZB_ZCL_GENERAL_DISC_ATTR_REQ_A(buffer, cmd_ptr, ZB_ZCL_FRAME_DIRECTION_TO_CLI,
                                     ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                                     0, /* start attribute id */
                                     0xff, /* maximum attribute id-s */
                                     DST_ADDR,
                                     ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                     ENDPOINT_C,
                                     ENDPOINT_ED,
                                     ZB_AF_HA_PROFILE_ID,
                                     ZB_ZCL_CLUSTER_ID_BASIC,
                                     NULL);
      g_test_step++;
    }
    break;

    case TEST_STEP_CONFIGURE_REPORTING_GLOBAL_CLUSTER_REVISION:
    {
      zb_uint8_t *cmd_ptr;
      zb_uint16_t reportable_change = 1;

      ZB_ZCL_GENERAL_INIT_CONFIGURE_REPORTING_CLI_REQ(buffer,
                                                      cmd_ptr,
                                                      ZB_ZCL_ENABLE_DEFAULT_RESPONSE);

      ZB_ZCL_GENERAL_ADD_SEND_REPORT_CONFIGURE_REPORTING_REQ(
        cmd_ptr, ZB_ZCL_ATTR_GLOBAL_CLUSTER_REVISION_ID, ZB_ZCL_ATTR_TYPE_U16, 10, 30,
        (zb_uint8_t*)&reportable_change);

      ZB_ZCL_GENERAL_SEND_CONFIGURE_REPORTING_REQ(buffer,
                                                  cmd_ptr,
                                                  DST_ADDR,
                                                  ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                                  ENDPOINT_C,
                                                  ENDPOINT_ED,
                                                  ZB_AF_HA_PROFILE_ID,
                                                  ZB_ZCL_CLUSTER_ID_BASIC,
                                                  NULL);
      g_test_step++;
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
  for (int i = 0; i < 5; ++i)
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
