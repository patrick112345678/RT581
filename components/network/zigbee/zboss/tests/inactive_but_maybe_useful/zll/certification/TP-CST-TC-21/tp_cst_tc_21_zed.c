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
/* PURPOSE: Client for TP-CST-TC-15 test
*/

#define ZB_TRACE_FILE_ID 41616
#include "zll/zb_zll_common.h"
#include "test_defs.h"

#if defined ZB_ENABLE_ZLL && defined ZB_ZLL_DEFINE_DEVICE_COLOR_CONTROLLER

#if ! defined ZB_ED_ROLE
#error define ZB_ED_ROLE to compile zed tests
#endif

/** Number of the endpoint device operates on. */
#define ENDPOINT  10

/** ZCL command handler. */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);

/** Start device parameters checker. */
void test_check_start_status(zb_uint8_t param);

/** Next test step initiator. */
void test_next_step(zb_uint8_t param);

/** Test read attribute */
void read_attr_resp_handler(zb_buf_t * cmd_buf);

/** Test step enumeration. */
enum test_step_e
{
  TEST_STEP_INITIAL,            /**< Initial test pseudo-step (device startup). */
  TEST_STEP_START,              /**< Device start test. */

  TEST_STEP_SEND_ON,            /**< ZED send On command frame to ZR. */
  TEST_STEP_SEND_REMOVE_ALL_GRP,/**< ZED send Remove all groups command. */

  TEST_STEP_SEND_MOVE_TO_COLOR, /** ZED unicasts a ZCL Move to Color command frame to ZR with the
                                    ColorX and ColorY fields = appropriate values for a red
                                    color and the transition time field = 0x000a (1s). */

  TEST_STEP_1_READ_ATTR,        /**< ZED read attributes: ColorTemperature, ColorTempPhysicalMin and
                                    ColorTempPhysicalMax. Save values */
  TEST_STEP_2A_MOVE_TO_COLOR_TEMP,  /**< ZED send move to color temperature command with color temperature = 0x03e8,
                                   transition time =0x0064 */
  TEST_STEP_2B_READ_ATTR,       /**< ZED read attributes: ColorTemperature, ColorMode and EnhancedColorMode.
                                    ColorTemperature about 0x03e8. ColorMode = 0x02. EnhancedColorMode = 0x02. */
  TEST_STEP_3A_MOVE_TO_COLOR_TEMP,  /**< ZED send move to color temperature command with color temperature <
                                    ColorTempPhysicalMin (determined in step 1), transition time = 0x0064.
                                    ZR response with the status = 0x87 (INVALID_VALUE). */
  TEST_STEP_3B_READ_ATTR,       /**< ZED read attributes: ColorTemperature, ColorMode and EnhancedColorMode.
                                    ColorTemperature about 0x03e8. ColorMode = 0x02. EnhancedColorMode = 0x02. */
  TEST_STEP_4A_MOVE_TO_COLOR_TEMP,  /**< ZED send move to color temperature command with color temperature >
                                    > ColorTempPhysicalMax (determined in step 1), transition time = 0x0064.
                                    ZR response with the status = 0x87 (INVALID_VALUE). */
  TEST_STEP_4B_READ_ATTR,       /**< ZED read attributes: ColorTemperature, ColorMode and EnhancedColorMode.
                                    ColorTemperature about 0x03e8. ColorMode = 0x02. EnhancedColorMode = 0x02. */
  TEST_STEP_5A_MOVE_COLOR_TEMP, /**< ZED send move color temperature command ZR with move mode = 0x01 (move up),
                                    rate = 0x03e8 (1000 units per second), color temperature minimum = 0x0000
                                    (minimum is ColorTempPhysicalMin), color temperature maximum = 0x7530
                                    (30000 = minimum 33.3K). */
  TEST_STEP_5B_MOVE_COLOR_TEMP, /**< After 10sec ZED send move color temperature command ZR with move mode = 0x00 (stop the
                                    move), rate = 0x0000, color temperature minimum = 0x0000 and color temperature
                                    maximum = 0x0000.*/
  TEST_STEP_5C_READ_ATTR,       /**< ZED read attributes: ColorTemperature. ColorTemperature about 0x2af8 (11000). */
  TEST_STEP_6A_MOVE_COLOR_TEMP, /**< ZED send move color temperature command ZR with move mode = 0x01 (move up),
                                    rate = 0x03e8 (1000 units per second), color temperature minimum = 0x0000
                                    (minimum is ColorTempPhysicalMin), color temperature maximum = 0x3a98.
                                    Ignore result*/
  TEST_STEP_6B_READ_ATTR,       /**< After 6 sec ZED read attributes: ColorTemperature. ColorTemperature about 0x3a98. */
  TEST_STEP_7A_MOVE_COLOR_TEMP, /**< ZED send move color temperature command ZR with move mode = 0x03 (move down),
                                    rate = 0x01f4 (500 units per second), color temperature minimum = 0x01f4,
                                    color temperature maximum  = 0x0000 (maximum is ColorTempPhysicalMax). */
  TEST_STEP_7B_MOVE_COLOR_TEMP, /**< After 10sec ZED send move color temperature command ZR with move mode = 0x00 (stop the
                                    move), rate = 0x0000, color temperature minimum = 0x0000 and color temperature
                                    maximum = 0x0000.*/
  TEST_STEP_7C_READ_ATTR,       /**< ZED read attributes: ColorTemperature. ColorTemperature about 0x2710 (10000). */
  TEST_STEP_8A_MOVE_COLOR_TEMP, /**< ZED send move color temperature command ZR with move mode = 0x03 (move down),
                                    rate = 0x03e8 (1000 units per second), color temperature minimum = 0x1b58,
                                    color temperature maximum = 0x0000 (maximum is ColorTempPhysicalMax).
                                    Ignore result. */
  TEST_STEP_8B_READ_ATTR,       /**< After 5 sec ZED read attributes: ColorTemperature. ColorTemperature about 0x1b58. */
  TEST_STEP_9A_STEP_COLOR_TEMP, /**< ZED send step color temperature command with step mode = 0x01 (step up),
                                    step size = 0x2710 (10000 units per step), transition time = 0x00c8 (20s),
                                    color temperature minimum  = 0x0000 (minimum is ColorTempPhysicalMin), color
                                    temperature maximum  = 0x7530.
                                    Note: for 10 sec before next test step has no color step (10 sec < 20 sec per step).
                                     May be step size = 0x01f4 (500 unit per step), transition time = 0x000a (1s)
                                     */
  TEST_STEP_9B_STOP_MOVE_STEP,  /**< After 10 sec ZED send stop move step command. */
  TEST_STEP_9C_READ_ATTR,       /**< ZED read attributes: ColorTemperature. ColorTemperature about 0x2ee0 (12000). */
  TEST_STEP_10A_STEP_COLOR_TEMP,/**< ZED send step color temperature command with step mode = 0x01 (step up),
                                    step size =  0x1388 (5000 units per step), transition time = 0x0032 (5s), color
                                    temperature minimum = 0x0000, color temperature maximum  = 0x3a98.
                                    Ignore result. */
  TEST_STEP_10B_READ_ATTR,      /**< After 6 sec ZED read attributes: ColorTemperature. ColorTemperature about 0x3a98
                                    (15000). */
  TEST_STEP_11A_STEP_COLOR_TEMP,/**< ZED send step color temperature command with step mode = 0x03 (step down),
                                    step size = 0x4e20 (20000 units per step), transition time = 0x00c8 (20s), color
                                    temperature minimum = 0x01f4, color temperature maximum = 0x0000.
                                    Note: for 10 sec before next test step has no color step (10 sec < 20 sec per step).
                                     May be step size = 0x07d0 (1000 unit per step), transition time = 0x0014 (2s)
   */
  TEST_STEP_11B_STOP_MOVE_STEP, /**< After 10 sec ZED send stop move step command. */
  TEST_STEP_11C_READ_ATTR,      /**< ZED read attributes: ColorTemperature. ColorTemperature about 0x1388 (5000). */
  TEST_STEP_12A_STEP_COLOR_TEMP,/**< ZED send step color temperature command with step mode = 0x03 (step down),
                                    step size = 0x1388 (5000 units per step), transition time  = 0x0032 (5s), color
                                    temperature minimum = 0x07d0, color temperature maximum = 0x0000.
                                    Ignore result. */
  TEST_STEP_12B_READ_ATTR,      /**< After 6 sec ZED read attributes: ColorTemperature. ColorTemperature about 0x07d0
                                    (2000). */

  TEST_STEP_FINISHED            /**< Test finished pseudo-step. */
};

zb_uint8_t g_test_step = TEST_STEP_INITIAL;
zb_uint8_t current_seq_number;


/** Device's extended address. */
zb_ieee_addr_t g_ed_addr = {0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

zb_uint16_t color_temperature;
zb_uint16_t color_temp_physical_min;
zb_uint16_t color_temp_physical_max;

#define TEST_ATTRIBUTE(read_attr_resp, attrid)                                \
  if ( !read_attr_resp )                                                      \
  {                                                                           \
    TRACE_MSG(TRACE_ERROR, "ERROR, No info on attribute(s) read", (FMT__0));  \
    g_error_cnt++;                                                            \
  }                                                                           \
  else if (ZB_ZCL_STATUS_SUCCESS != read_attr_resp->status)                   \
  {                                                                           \
    TRACE_MSG(TRACE_ERROR,                                                    \
            "ERROR incorrect response: id 0x%04x, status %d",                 \
            (FMT__D_H, read_attr_resp->attr_id, read_attr_resp->status));     \
    g_error_cnt++;                                                            \
  }                                                                           \
  else  if ( (attrid)!= read_attr_resp->attr_id)                              \
  {                                                                           \
    g_error_cnt++;                                                            \
    TRACE_MSG(TRACE_ERROR, "ERROR id 0x%x value %hx step %d", (FMT__D_H_H,    \
      read_attr_resp->attr_id, read_attr_resp->attr_value[0], g_test_step));  \
  }

#define TEST_ATTRIBUTE_8(read_attr_resp, attrid, value)                       \
  TEST_ATTRIBUTE((read_attr_resp), (attrid))                                  \
  else  if ( read_attr_resp->attr_value[0] != (value) )                       \
  {                                                                           \
    g_error_cnt++;                                                            \
    TRACE_MSG(TRACE_ERROR, "ERROR id 0x%x value %hx expected value %hx step %d", (FMT__D_H_H_H,    \
      read_attr_resp->attr_id, read_attr_resp->attr_value[0], (value), g_test_step));  \
  }

#define TEST_ATTRIBUTE_16(read_attr_resp, attrid, value, delta)               \
  TEST_ATTRIBUTE((read_attr_resp), (attrid))                                  \
  else  if ( (value)-(delta) > *(zb_uint16_t*)(read_attr_resp->attr_value) || \
            *(zb_uint16_t*)(read_attr_resp->attr_value) > (value)+(delta) )   \
  {                                                                           \
    g_error_cnt++;                                                            \
      TRACE_MSG(TRACE_ERROR, "ERROR id 0x%x value %hx expected value %hx step %d", (FMT__D_D_D_H, \
        read_attr_resp->attr_id, *(zb_uint16_t*)(read_attr_resp->attr_value), (value), g_test_step));  \
  }

#define TEST_ATTRIBUTE_16_SHARP(read_attr_resp, attrid, value)                \
  TEST_ATTRIBUTE((read_attr_resp), (attrid))                                  \
  else  if ( (value) != *(zb_uint16_t*)(read_attr_resp->attr_value) )         \
  {                                                                           \
    g_error_cnt++;                                                            \
      TRACE_MSG(TRACE_ERROR, "ERROR id 0x%x value %hx expected value %hx step %d", (FMT__D_D_D_H,\
        read_attr_resp->attr_id, *(zb_uint16_t*)(read_attr_resp->attr_value), (value), g_test_step));  \
  }


/********************* Declare device **************************/
ZB_ZLL_DECLARE_COLOR_CONTROLLER_CLUSTER_LIST(color_controller_clusters,
    ZB_ZCL_CLUSTER_MIXED_ROLE);

ZB_ZLL_DECLARE_COLOR_CONTROLLER_EP(color_controller_ep, ENDPOINT, color_controller_clusters);

ZB_ZLL_DECLARE_COLOR_CONTROLLER_CTX(color_controller_ctx, color_controller_ep);

/******************* Declare router parameters *****************/
#define DST_ADDR        0x01
#define DST_ENDPOINT    5
#define DST_ADDR_MODE   ZB_APS_ADDR_MODE_16_ENDP_PRESENT
/******************* Declare test data & constants *************/

zb_short_t g_error_cnt = 0;

/* Send command "read attribute" to server */
#define SEND_READ_ATTR(buffer, clusterID, attributeID)                                     \
{                                                                                          \
  zb_uint8_t *cmd_ptr;                                                                     \
  ZB_ZCL_GENERAL_INIT_READ_ATTR_REQ((buffer), cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);    \
  ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, (attributeID));                             \
  ZB_ZCL_GENERAL_SEND_READ_ATTR_REQ(                                                       \
      (buffer), cmd_ptr, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ENDPOINT,                  \
      ZB_AF_ZLL_PROFILE_ID, (clusterID), NULL);                                            \
}

/* Send command "read attribute" to server */
#define SEND_READ_ATTR_2(buffer, clusterID, attributeID, attributeID_2)                    \
{                                                                                          \
  zb_uint8_t *cmd_ptr;                                                                     \
  ZB_ZCL_GENERAL_INIT_READ_ATTR_REQ((buffer), cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);    \
  ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, (attributeID));                             \
  ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, (attributeID_2));                           \
  ZB_ZCL_GENERAL_SEND_READ_ATTR_REQ(                                                       \
      (buffer), cmd_ptr, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ENDPOINT,                  \
      ZB_AF_ZLL_PROFILE_ID, (clusterID), NULL);                                            \
}

/* Send command "read attribute" to server */
#define SEND_READ_ATTR_3(buffer, clusterID, attributeID, attributeID_2, attributeID_3)     \
{                                                                                          \
  zb_uint8_t *cmd_ptr;                                                                     \
  ZB_ZCL_GENERAL_INIT_READ_ATTR_REQ((buffer), cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);    \
  ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, (attributeID));                             \
  ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, (attributeID_2));                           \
  ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, (attributeID_3));                           \
  ZB_ZCL_GENERAL_SEND_READ_ATTR_REQ(                                                       \
      (buffer), cmd_ptr, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ENDPOINT,                  \
      ZB_AF_ZLL_PROFILE_ID, (clusterID), NULL);                                            \
}

/* Send command "read attribute" to server */
#define SEND_READ_ATTR_4(buffer, clusterID,                                                \
        attributeID, attributeID_2, attributeID_3, attributeID_4)                          \
{                                                                                          \
  zb_uint8_t *cmd_ptr;                                                                     \
  ZB_ZCL_GENERAL_INIT_READ_ATTR_REQ((buffer), cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);    \
  ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, (attributeID));                             \
  ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, (attributeID_2));                           \
  ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, (attributeID_3));                           \
  ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, (attributeID_4));                           \
  ZB_ZCL_GENERAL_SEND_READ_ATTR_REQ(                                                       \
      (buffer), cmd_ptr, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ENDPOINT,                  \
      ZB_AF_ZLL_PROFILE_ID, (clusterID), NULL);                                            \
}

MAIN()
{
  ARGV_UNUSED;

#if !(defined KEIL || defined SDCC || defined ZB_IAR || defined ZB_PLATFORM_LINUX_ARM_2400)
#endif

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zed1");


  ZB_SET_NIB_SECURITY_LEVEL(0);

  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ed_addr);
  ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);

  zb_set_default_ed_descriptor_values();

  /****************** Register Device ********************************/
  /* Register device list */
  ZB_AF_REGISTER_DEVICE_CTX(&color_controller_ctx);
  ZB_AF_SET_ENDPOINT_HANDLER(ENDPOINT, zcl_specific_cluster_cmd_handler);

  ZB_AIB().aps_channel_mask = 1l << MY_CHANNEL;

  ZG->nwk.nib.security_level = 0;

  if (zb_zll_dev_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "ERROR zdo_dev_start failed", (FMT__0));
    ++g_error_cnt;
  }
  else
  {
    zcl_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}

zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
  zb_buf_t *zcl_cmd_buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
  zb_zcl_parsed_hdr_t *cmd_info = ZB_GET_BUF_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);
  zb_zcl_default_resp_payload_t* default_res;

  TRACE_MSG(TRACE_ZCL1, "> zcl_specific_cluster_cmd_handler %i", (FMT__H, param));
  ZB_ZCL_DEBUG_DUMP_HEADER(cmd_info);
  TRACE_MSG(TRACE_ZCL1, "payload size: %i", (FMT__D, ZB_BUF_LEN(zcl_cmd_buf)));

  if(current_seq_number!=cmd_info->seq_number)
  {
    TRACE_MSG(TRACE_ZCL2, "step %hd current_seq_number %hd cmd_info->seq_number %hd", (FMT__H_H_H,
          g_test_step, current_seq_number, cmd_info->seq_number));
    return ZB_FALSE;
  }

  if( cmd_info -> cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI )
  {
    switch( cmd_info -> cluster_id )
    {
    case ZB_ZCL_CLUSTER_ID_COLOR_CONTROL:
    case ZB_ZCL_CLUSTER_ID_ON_OFF:
    case ZB_ZCL_CLUSTER_ID_SCENES:
    case ZB_ZCL_CLUSTER_ID_GROUPS:
        if( cmd_info -> is_common_command )
        {
          switch( cmd_info -> cmd_id )
          {
            case ZB_ZCL_CMD_DEFAULT_RESP:
              TRACE_MSG(TRACE_ZCL3, "Got response in cluster 0x%04x",
                        ( FMT__D, cmd_info->cluster_id));
              /* Process default response */
              default_res = ZB_ZCL_READ_DEFAULT_RESP(zcl_cmd_buf);
              if( ( g_test_step==TEST_STEP_3A_MOVE_TO_COLOR_TEMP ||
                    g_test_step==TEST_STEP_4A_MOVE_TO_COLOR_TEMP ) &&
                  ( default_res->status!=ZB_ZCL_STATUS_INVALID_VALUE) )
              {
                g_error_cnt++;
                TRACE_MSG(TRACE_ERROR, "Error answer %hd", (FMT__H, default_res->status));
              }
              break;

            case ZB_ZCL_CMD_READ_ATTRIB_RESP:
              read_attr_resp_handler(zcl_cmd_buf);
              break;

            default:
              TRACE_MSG(TRACE_ZCL2, "Skip general command %hd", (FMT__H, cmd_info->cmd_id));
              break;
          }
        }
        else
        {
          switch( cmd_info -> cmd_id )
          {
            //case ZB_ZCL_CMD_DOOR_LOCK_LOCK_DOOR_RES:
            //  TRACE_MSG(TRACE_ZCL1, "Response:  ZB_ZCL_CMD_DOOR_LOCK_LOCK_DOOR_RES", (FMT__0));
            //  plpayload = ZB_ZCL_DOOR_LOCK_READ_LOCK_DOOR_RES(zcl_cmd_buf);
            //  break;
            default:
              TRACE_MSG(TRACE_ZCL2, "Cluster command %hd, skip it", (FMT__H, cmd_info->cmd_id));
              //++g_error_cnt;
              break;
          }
        }
        break;

      default:
        TRACE_MSG(TRACE_ZCL1, "ERROR CLNT role cluster 0x%d is not supported", (FMT__D, cmd_info->cluster_id));
        ++g_error_cnt;
        break;
    }
  }
  else
  {
    /* Command from client to server ZB_ZCL_FRAME_DIRECTION_TO_SRV */
    TRACE_MSG(TRACE_ZCL1, "ERROR SRV role, cluster 0x%d is not supported", (FMT__D, cmd_info->cluster_id));
    ++g_error_cnt;
  }

  if( g_test_step!=TEST_STEP_6A_MOVE_COLOR_TEMP &&
      g_test_step!=TEST_STEP_8A_MOVE_COLOR_TEMP &&
      g_test_step!=TEST_STEP_10A_STEP_COLOR_TEMP &&
      g_test_step!=TEST_STEP_12A_STEP_COLOR_TEMP)
  {
    test_next_step(param);
  }
  else
  {
    zb_free_buf(zcl_cmd_buf);
  }

  TRACE_MSG(TRACE_ZCL1, "<< zcl_specific_cluster_cmd_handler", (FMT__0));
  return ZB_TRUE;
}

void zb_zdo_startup_complete(zb_uint8_t param)
{
  zb_buf_t* buffer = ZB_BUF_FROM_REF(param);
  zb_zll_transaction_task_status_t* task_status =
      ZB_GET_BUF_PARAM(buffer, zb_zll_transaction_task_status_t);

  TRACE_MSG(TRACE_ZLL3, "> zb_zdo_startup_complete %hd status %hd", (FMT__H_H, param, task_status->task));

  switch (task_status->task)
  {
  case ZB_ZLL_DEVICE_START_TASK:
    test_check_start_status(param);
    break;

  case ZB_ZLL_TRANSACTION_NWK_START_TASK:
    TRACE_MSG(TRACE_ZLL3, "New Network status %hd", (FMT__H, task_status->status));
    if (task_status->status != ZB_ZLL_TASK_STATUS_OK)
    {
      TRACE_MSG(TRACE_ERROR, "ERROR status %hd", (FMT__H, task_status->status));
      ++g_error_cnt;
    }
    break;

  case ZB_ZLL_TRANSACTION_JOIN_ROUTER_TASK:
    TRACE_MSG(TRACE_ZLL3, "Join Router status %hd", (FMT__H, task_status->status));
    if (task_status->status != ZB_ZLL_TASK_STATUS_OK)
    {
      TRACE_MSG(TRACE_ERROR, "ERROR status %hd", (FMT__H, task_status->status));
      ++g_error_cnt;
    }
    break;

  default:
    TRACE_MSG(TRACE_ERROR, "ERROR unsupported task %hd", (FMT__H, task_status->task));
    ++g_error_cnt;
    break;
  }

  if (g_error_cnt)
  {
    g_test_step = TEST_STEP_FINISHED;
  }

  if (g_test_step == TEST_STEP_FINISHED)
  {
    zb_free_buf(ZB_BUF_FROM_REF(param));
  }
  else
  {
    test_next_step(param);
  }

  TRACE_MSG(TRACE_ZLL3, "< zb_zdo_startup_complete", (FMT__0));
}/* void zll_task_state_changed(zb_uint8_t param) */

void test_check_start_status(zb_uint8_t param)
{
  zb_buf_t* buffer = ZB_BUF_FROM_REF(param);
  zb_zll_transaction_task_status_t* task_status =
      ZB_GET_BUF_PARAM(buffer, zb_zll_transaction_task_status_t);

  TRACE_MSG(TRACE_ZLL3, "> test_check_start_status param %hd", (FMT__D, param));

  if (ZB_PIBCACHE_CURRENT_CHANNEL() != MY_CHANNEL)
  {
    TRACE_MSG(TRACE_ERROR, "ERROR wrong channel %hd (should be %hd)",
        (FMT__H_H, ZB_PIBCACHE_CURRENT_CHANNEL(), (zb_uint8_t)MY_CHANNEL));
    ++g_error_cnt;
  }

  if( ZB_ZLL_IS_FACTORY_NEW() )
  {
    if (ZB_PIBCACHE_NETWORK_ADDRESS() != 0xffff)
    {
      TRACE_MSG(TRACE_ERROR, "ERROR Network address is 0x%04x (should be 0xffff)",
          (FMT__D, ZB_PIBCACHE_NETWORK_ADDRESS()));
      ++g_error_cnt;
    }
  }

  if (! ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()))
  {
    TRACE_MSG(TRACE_ERROR, "ERROR Receiver should be turned on", (FMT__0));
    ++g_error_cnt;
  }

  if (! ZB_EXTPANID_IS_ZERO(ZB_NIB_EXT_PAN_ID()))
  {
    TRACE_MSG(TRACE_ERROR, "ERROR extended PAN Id is not zero: %s",
        (FMT__A, ZB_NIB_EXT_PAN_ID()));
    ++g_error_cnt;
  }

  if (ZB_PIBCACHE_PAN_ID() != 0xffff)
  {
    TRACE_MSG(TRACE_ERROR, "ERROR PAN Id is 0x%04x (should be 0xffff)",
        (FMT__D, ZB_PIBCACHE_PAN_ID()));
    ++g_error_cnt;
  }

  if (task_status->status == ZB_ZLL_GENERAL_STATUS_SUCCESS && ! g_error_cnt)
  {
    TRACE_MSG(TRACE_ZLL3, "Device STARTED OK", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ZLL3, "ERROR Device start FAILED (errors: %hd)", (FMT__H, g_error_cnt));
  }

  TRACE_MSG(TRACE_ZLL3, "< test_check_start_status", (FMT__0));
}/* void test_check_start_status(zb_uint8_t param) */

void test_timer_next_step(zb_uint8_t param)
{
  (void)param;

  TRACE_MSG(TRACE_ZLL3, "test_timer_next_step", (FMT__0));

  ZB_GET_OUT_BUF_DELAYED(test_next_step);
}

void test_next_step(zb_uint8_t param)
{
  zb_buf_t* buffer = ZB_BUF_FROM_REF(param);
  zb_uint8_t status;

  TRACE_MSG(TRACE_ZLL3, "> test_next_step param %hd step %hd", (FMT__H, param, g_test_step));

  if(g_test_step!=TEST_STEP_FINISHED)
  {
    ++g_test_step;
  }

  switch (g_test_step )
  {
    case TEST_STEP_START:
      TRACE_MSG(TRACE_ZLL3, "Start commissioning", (FMT__0));
      status = zb_zll_start_commissioning(param);
      if (status != RET_OK)
      {
        TRACE_MSG(TRACE_ERROR, "ERROR Could not initiate commissioning: status %hd", (FMT__H, status));
        ++g_error_cnt;
      }
      break;

    case TEST_STEP_SEND_ON:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_ON (p3)", (FMT__0));
      ZB_ZCL_ON_OFF_SEND_ON_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
                                ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL);
      break;

    case TEST_STEP_SEND_REMOVE_ALL_GRP:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_REMOVE_ALL_GRP (p4)", (FMT__0));
      ZB_ZCL_GROUPS_SEND_REMOVE_ALL_GROUPS_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
                                               ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL);
      break;

    case TEST_STEP_SEND_MOVE_TO_COLOR:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_MOVE_TO_COLOR (p5)", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_MOVE_TO_COLOR_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, ZB_ZCL_COLOR_CONTROL_COLOR_X_RED,
        ZB_ZCL_COLOR_CONTROL_COLOR_Y_RED, 0x000a);
      break;

    case TEST_STEP_1_READ_ATTR:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_1_READ_ATTR", (FMT__0));
      SEND_READ_ATTR_3(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL,
          ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_TEMPERATURE_ID,
          ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_TEMP_PHYSICAL_MIN_ZLL_ID,
          ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_TEMP_PHYSICAL_MAX_ZLL_ID);
      break;

    case TEST_STEP_2A_MOVE_TO_COLOR_TEMP:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_2A_MOVE_TO_COLOR_TEMP", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_MOVE_TO_COLOR_TEMPERATURE_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
        0x03e8, 0x0064);
      break;

    case TEST_STEP_2B_READ_ATTR:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_2B_READ_ATTR", (FMT__0));
      SEND_READ_ATTR_3(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL,
          ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_TEMPERATURE_ID,
          ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_MODE_ID,
          ZB_ZCL_ATTR_COLOR_CONTROL_ENHANCED_COLOR_MODE_ZLL_ID);
      break;

    case TEST_STEP_3A_MOVE_TO_COLOR_TEMP:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_3A_MOVE_TO_COLOR_TEMP", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_MOVE_TO_COLOR_TEMPERATURE_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
        (color_temp_physical_min-1), 0x0064);
      break;

    case TEST_STEP_3B_READ_ATTR:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_3B_READ_ATTR", (FMT__0));
      SEND_READ_ATTR_3(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL,
          ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_TEMPERATURE_ID,
          ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_MODE_ID,
          ZB_ZCL_ATTR_COLOR_CONTROL_ENHANCED_COLOR_MODE_ZLL_ID);
      break;

    case TEST_STEP_4A_MOVE_TO_COLOR_TEMP:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_4A_MOVE_TO_COLOR_TEMP", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_MOVE_TO_COLOR_TEMPERATURE_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
        (color_temp_physical_max+1), 0x0064);
      break;

    case TEST_STEP_4B_READ_ATTR:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_3B_READ_ATTR", (FMT__0));
      SEND_READ_ATTR_3(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL,
          ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_TEMPERATURE_ID,
          ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_MODE_ID,
          ZB_ZCL_ATTR_COLOR_CONTROL_ENHANCED_COLOR_MODE_ZLL_ID);
      break;

    case TEST_STEP_5A_MOVE_COLOR_TEMP:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_5A_MOVE_COLOR_TEMP", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_MOVE_COLOR_TEMP_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
        ZB_ZCL_CMD_COLOR_CONTROL_MOVE_UP, 0x03e8, 0x0000, 0x7530);
      ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 10*ZB_TIME_ONE_SECOND);
      break;

    case TEST_STEP_5B_MOVE_COLOR_TEMP:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_5B_MOVE_COLOR_TEMP", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_MOVE_COLOR_TEMP_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
        ZB_ZCL_CMD_COLOR_CONTROL_MOVE_STOP, 0x0000, 0x0000, 0x0000);
      break;

    case TEST_STEP_5C_READ_ATTR:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_5C_READ_ATTR", (FMT__0));
      SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL,
          ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_TEMPERATURE_ID);
      break;

    case TEST_STEP_6A_MOVE_COLOR_TEMP:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_6A_MOVE_COLOR_TEMP", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_MOVE_COLOR_TEMP_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
        ZB_ZCL_CMD_COLOR_CONTROL_MOVE_UP, 0x03e8, 0x0000, 0x3a98);
      ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 6*ZB_TIME_ONE_SECOND);
      break;

    case TEST_STEP_6B_READ_ATTR:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_6B_READ_ATTR", (FMT__0));
      SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL,
          ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_TEMPERATURE_ID);
      break;

    case TEST_STEP_7A_MOVE_COLOR_TEMP:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_7A_MOVE_COLOR_TEMP", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_MOVE_COLOR_TEMP_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
        ZB_ZCL_CMD_COLOR_CONTROL_MOVE_DOWN, 0x01F4, 0x01F4, 0x0000);
      ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 10*ZB_TIME_ONE_SECOND);
      break;

    case TEST_STEP_7B_MOVE_COLOR_TEMP:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_7B_MOVE_COLOR_TEMP", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_MOVE_COLOR_TEMP_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
        ZB_ZCL_CMD_COLOR_CONTROL_MOVE_STOP, 0x0000, 0x0000, 0x0000);
      break;

    case TEST_STEP_7C_READ_ATTR:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_7C_READ_ATTR", (FMT__0));
      SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL,
          ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_TEMPERATURE_ID);
      break;

    case TEST_STEP_8A_MOVE_COLOR_TEMP:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_8A_MOVE_COLOR_TEMP", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_MOVE_COLOR_TEMP_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
        ZB_ZCL_CMD_COLOR_CONTROL_MOVE_DOWN, 0x03e8, 0x1b58, 0x0000);
      ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 5*ZB_TIME_ONE_SECOND);
      break;

    case TEST_STEP_8B_READ_ATTR:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_8B_READ_ATTR", (FMT__0));
      SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL,
          ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_TEMPERATURE_ID);
      break;

    case TEST_STEP_9A_STEP_COLOR_TEMP:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_9A_STEP_COLOR_TEMP", (FMT__0));
//      ZB_ZCL_COLOR_CONTROL_SEND_STEP_COLOR_TEMP_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
//        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
//        ZB_ZCL_CMD_COLOR_CONTROL_STEP_UP, 0x2710, 0x00c8, 0x0000, 0x7530);
      ZB_ZCL_COLOR_CONTROL_SEND_STEP_COLOR_TEMP_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
        ZB_ZCL_CMD_COLOR_CONTROL_STEP_UP, 0x01f4, 0x000a, 0x0000, 0x7530);
      ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 10.1*ZB_TIME_ONE_SECOND);
      break;

    case TEST_STEP_9B_STOP_MOVE_STEP:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_9B_STOP_MOVE_STEP", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_STOP_MOVE_STEP_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL);
      break;

    case TEST_STEP_9C_READ_ATTR:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_9C_READ_ATTR", (FMT__0));
      SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL,
          ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_TEMPERATURE_ID);
      break;

    case TEST_STEP_10A_STEP_COLOR_TEMP:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_10A_STEP_COLOR_TEMP", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_STEP_COLOR_TEMP_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
        ZB_ZCL_CMD_COLOR_CONTROL_STEP_UP, 0x1388, 0x0032, 0x0000, 0x3a98);
      ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 6*ZB_TIME_ONE_SECOND);
      break;

    case TEST_STEP_10B_READ_ATTR:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_10B_READ_ATTR", (FMT__0));
      SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL,
          ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_TEMPERATURE_ID);
      break;

    case TEST_STEP_11A_STEP_COLOR_TEMP:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_11A_STEP_COLOR_TEMP", (FMT__0));
//      ZB_ZCL_COLOR_CONTROL_SEND_STEP_COLOR_TEMP_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
//        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
//        ZB_ZCL_CMD_COLOR_CONTROL_STEP_DOWN, 0x4e20, 0x00c8, 0x01f4, 0x0000);
      ZB_ZCL_COLOR_CONTROL_SEND_STEP_COLOR_TEMP_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
        ZB_ZCL_CMD_COLOR_CONTROL_STEP_DOWN, 0x07d0, 0x0014, 0x01f4, 0x0000);
      ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 10.2*ZB_TIME_ONE_SECOND);
      break;

    case TEST_STEP_11B_STOP_MOVE_STEP:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_11B_STOP_MOVE_STEP", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_STOP_MOVE_STEP_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL);
      break;

    case TEST_STEP_11C_READ_ATTR:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_11C_READ_ATTR", (FMT__0));
      SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL,
          ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_TEMPERATURE_ID);
      break;

    case TEST_STEP_12A_STEP_COLOR_TEMP:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_12A_STEP_COLOR_TEMP", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_STEP_COLOR_TEMP_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
        ZB_ZCL_CMD_COLOR_CONTROL_STEP_DOWN, 0x1388, 0x0032, 0x07d0, 0x0000);
      ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 6*ZB_TIME_ONE_SECOND);
      break;

    case TEST_STEP_12B_READ_ATTR:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_12B_READ_ATTR", (FMT__0));
      SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL,
          ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_TEMPERATURE_ID);
      break;

    case TEST_STEP_FINISHED:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_FINISHED", (FMT__0));
      break;

    default:
      TRACE_MSG(TRACE_ERROR, "ERROR step %hd can't be processed", (FMT__H, g_test_step));
      ++g_error_cnt;
      break;
  }

  current_seq_number = ZCL_CTX().seq_number -1; //previous!

  if (g_test_step == TEST_STEP_FINISHED || g_error_cnt)
  {
    if (g_error_cnt)
    {
      TRACE_MSG(TRACE_ERROR, "ERROR Test failed with %hd errors", (FMT__H, g_error_cnt));
    }
    else
    {
      TRACE_MSG(TRACE_INFO1, "Test finished. Status: OK", (FMT__0));
    }
    zb_free_buf(buffer);
  }

  TRACE_MSG(TRACE_ZLL3, "< test_next_step. Curr step %hd" , (FMT__H, g_test_step));
}/* void test_next_step(zb_uint8_t param) */

void read_attr_resp_handler(zb_buf_t * cmd_buf)
{
  zb_zcl_read_attr_res_t * read_attr_resp;

  TRACE_MSG(TRACE_ZCL1, ">> read_attr_resp_handler", (FMT__0));

  ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(cmd_buf, read_attr_resp);
  TRACE_MSG(TRACE_ZCL3, "read_attr_resp %p", (FMT__P, read_attr_resp));

  switch (g_test_step)
  {
  case TEST_STEP_1_READ_ATTR:
    TEST_ATTRIBUTE(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_TEMPERATURE_ID);
    if(g_error_cnt==0)
    {
      color_temperature = *(zb_uint16_t*)read_attr_resp->attr_value;

      ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(cmd_buf, read_attr_resp);
      TEST_ATTRIBUTE(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_TEMP_PHYSICAL_MIN_ZLL_ID);
    }
    if(g_error_cnt==0)
    {
      color_temp_physical_min = *(zb_uint16_t*)read_attr_resp->attr_value;

      ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(cmd_buf, read_attr_resp);
      TEST_ATTRIBUTE(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_TEMP_PHYSICAL_MAX_ZLL_ID);
    }
    if(g_error_cnt==0)
    {
      color_temp_physical_max = *(zb_uint16_t*)read_attr_resp->attr_value;

      TRACE_MSG(TRACE_ZCL1, "color temp %d max %d min %d", (FMT__D_D_D,
          color_temperature, color_temp_physical_min, color_temp_physical_max));
    }
    break;

  case TEST_STEP_2B_READ_ATTR:
  case TEST_STEP_3B_READ_ATTR:
  case TEST_STEP_4B_READ_ATTR:
    TEST_ATTRIBUTE_16_SHARP(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_TEMPERATURE_ID, 0x03e8);

    if(g_error_cnt==0)
    {
      ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(cmd_buf, read_attr_resp);
      TEST_ATTRIBUTE_8(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_MODE_ID, 0x02);
    }

    if(g_error_cnt==0)
    {
      ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(cmd_buf, read_attr_resp);
      TEST_ATTRIBUTE_8(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_ENHANCED_COLOR_MODE_ZLL_ID, 0x02);
    }
    break;

  case TEST_STEP_5C_READ_ATTR:
    TEST_ATTRIBUTE_16(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_TEMPERATURE_ID, 0x2af8, 256);
    break;

  case TEST_STEP_6B_READ_ATTR:
    TEST_ATTRIBUTE_16(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_TEMPERATURE_ID, 0x3a98, 256);
    break;

  case TEST_STEP_7C_READ_ATTR:
    TEST_ATTRIBUTE_16(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_TEMPERATURE_ID, 0x2710, 256);
    break;

  case TEST_STEP_8B_READ_ATTR:
    TEST_ATTRIBUTE_16(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_TEMPERATURE_ID, 0x1b58, 256);
    break;

  case TEST_STEP_9C_READ_ATTR:
    TEST_ATTRIBUTE_16(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_TEMPERATURE_ID, 0x2ee0, 256);
    break;

  case TEST_STEP_10B_READ_ATTR:
    TEST_ATTRIBUTE_16(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_TEMPERATURE_ID, 0x3a98, 256);
    break;

  case TEST_STEP_11C_READ_ATTR:
    TEST_ATTRIBUTE_16(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_TEMPERATURE_ID, 0x1388, 256);
    break;

  case TEST_STEP_12B_READ_ATTR:
    TEST_ATTRIBUTE_16(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_TEMPERATURE_ID, 0x07d0, 256);
    break;

  default:
      TRACE_MSG(
        TRACE_ERROR,
        "ERROR! Unexpected read attributes response: test step 0x%hx",
        (FMT__D, g_test_step));
      ++g_error_cnt;
      break;
  }

  TRACE_MSG(TRACE_ZCL1, "<< read_attr_resp_handler", (FMT__0));
}

#else // defined ZB_ENABLE_ZLL && defined ZB_ZLL_DEFINE_DEVICE_COLOR_CONTROLLER

#include <stdio.h>
int main()
{
  printf(" ZLL is not supported\n");
  return 0;
}

#endif // defined ZB_ENABLE_ZLL && defined ZB_ZLL_DEFINE_DEVICE_COLOR_CONTROLLER
