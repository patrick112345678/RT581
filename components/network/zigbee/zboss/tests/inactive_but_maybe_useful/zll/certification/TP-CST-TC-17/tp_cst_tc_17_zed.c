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

#define ZB_TRACE_FILE_ID 41595
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

  TEST_STEP_1A_COLOR_LOOP_SET,  /** ZED unicasts a ZLL Color Loop Set command frame to ZR with the
                                    update flags field = 0x0e (update direction, time and start
                                    hue), the action field = 0x00, the direction field = 0x00,
                                    the time field = 0x000a (10s) and the start hue field = 0x0000.*/
  TEST_STEP_1B_READ_ATTR,       /** ZED unicasts a ZCL read attributes command frame for the ColorLoopActive,
                                    ColorLoopDirection, ColorLoopTime and ColorLoopStartEnhancedHue
                                    attributes to ZR. ColorLoopActive = 0x00. ColorLoopDirection =
                                    0x00. ColorLoopTime = 0x000a. ColorLoopStartEnhancedHue = 0x0000.*/
  TEST_STEP_2A_COLOR_LOOP_SET,  /** ZED unicasts a ZLL Color Loop Set command frame to ZR with the update flags field
                                    = 0x02 (update direction), the action field = 0x01,
                                    the direction field = 0x01, the time field = 0x0000
                                    and the start hue field = 0x0000. */
  TEST_STEP_2B_READ_ATTR,       /** ZED unicasts a ZCL read attributes command frame for the ColorLoopActive,
                                    ColorLoopDirection, ColorLoopTime and ColorLoopStartEnhancedHue
                                    attributes to ZR. ZR unicasts a ZCL read attributes response
                                    command frame to ZED. ColorLoopActive attribute = 0x00.
                                    ColorLoopDirection attribute = 0x01. ColorLoopTime
                                    attribute = 0x000a. ColorLoopStartEnhancedHue
                                    attribute = 0x0000. */
  TEST_STEP_3A_COLOR_LOOP_SET,  /** ZED unicasts a ZLL color loop set command frame to ZR with the
                                 * update flags field = 0x01 (update action), the action field =
                                 * 0x02 (start from enhanced current hue), the direction field =
                                 * 0x00, the time field = 0x0000 and the start hue field = 0x0000.
                                    ZR begins a color loop cycle (starting from the current enhanced hue) with a color cycle
                                    time of 10s.*/
  TEST_STEP_3B_READ_ATTR,       /** ZED unicasts a ZCL read attributes command frame for the
                                 * ColorLoopActive attribute to ZR. ZR unicasts a ZCL read
                                 * attributes response command frame to ZED. ColorLoopActive
                                 attribute = 0x01.*/
  TEST_STEP_3C_COLOR_LOOP_SET,  /** ZED unicasts a ZLL color loop set command frame to ZR with the
                                 * update flags field = 0x01 (update action), the action field =
                                 * 0x00 (deactivate), the direction field = 0x00, the time field =
                                 * 0x0000 and the start hue field = 0x0000. ZR stops the color loop
                                 * cycle and returns to its previous hue (red).*/
  TEST_STEP_3D_READ_ATTR,       /** ZED unicasts a ZCL read attributes command frame for the
                                 * ColorLoopActive attribute to ZR. ZR unicasts a ZCL read
                                 * attributes response command frame to ZED. ColorLoopActive attribute = 0x00.*/
  TEST_STEP_4A_COLOR_LOOP_SET,  /** ZED unicasts a ZLL color loop set command frame to ZR with the
                                    update flags field = 0x01 (update action), the action field =
                                    0x01 (start from ColorLoopStartEnhancedHue), the direction field
                                    = 0x00, the time field = 0x0000 and the start hue field =
                                    0x0000. ZR begins a color loop cycle (not starting from the
                                    current enhanced hue) with a color cycle time of 10s. */
  TEST_STEP_4B_READ_ATTR,       /** ZED unicasts a ZCL read attributes command frame for the
                                    ColorLoopStoredEnhancedHue attribute to ZR. ZR unicasts a ZCL
                                    read attributes response command frame to ZED. ColorLoopStoredEnhancedHue
                                    attribute = red hue.*/
  TEST_STEP_4C_COLOR_LOOP_SET,  /** ZED unicasts a ZLL color loop set command frame to ZR with the
                                    update flags field = 0x01 (update action), the action field =
                                    0x00 (deactivate), the direction field = 0x00, the time field set
                                    to 0x0000 and the start hue field = 0x0000. ZR stops the color
                                    loop cycle and returns to its previous hue (red). */
  TEST_STEP_5A_COLOR_LOOP_SET,  /** ZED unicasts a ZLL color loop set command frame to ZR with the
                                    update flags field = 0x01 (update action), the action field =
                                    0x02 (start from enhanced current hue), the direction field =
                                    0x00, the time field = 0x0000 and the start hue field = 0x0000.
                                    ZR begins a color loop cycle (starting from the current enhanced
                                    hue) with a color cycle time of 10s. */
  TEST_STEP_5B_READ_ATTR,       /** ZED unicasts a ZCL read attributes command frame for the
                                    ColorLoopStoredEnhancedHue attribute to ZR. ZR unicasts a ZCL
                                    read attributes response command frame to ZED. ColorLoopStoredEnhancedHue
                                    attribute = red hue. */
  TEST_STEP_5C_COLOR_LOOP_SET,  /** ZED unicasts a ZLL color loop set command frame to ZR with the
                                    update flags field = 0x01 (update action), the action field =
                                    0x00 (deactivate), the direction field = 0x00, the time
                                    field = 0x0000 and the start hue field = 0x0000. ZR stops the
                                    color loop cycle and returns to its previous hue (red). */
  TEST_STEP_6A_COLOR_LOOP_SET,  /** ZED unicasts a ZLL color loop set command frame to ZR with the
                                    update flags field = 0x05 (update action and time), the action
                                    field = 0x02 (start from enhanced current hue), the direction field
                                    = 0x00, the time field = 0x0064 (100s) and the start hue field =
                                    0x0000. ZR begins a Color Loop Cycle (starting from the current
                                    enhanced hue) with a color cycle time of 100s. */
  TEST_STEP_6B_COLOR_LOOP_SET,  /** ZED unicasts a ZLL color loop set command frame to ZR with the
                                    update flags field = 0x02 (update direction), the action field =
                                    0x00, the direction field = 0x00, the time field = 0x0000 and
                                    the start hue field = 0x0000. ZR reverses the direction of the
                                    color loop cycle but retains the color cycle time of 100s.*/
  TEST_STEP_6C_COLOR_LOOP_SET,  /** ZED unicasts a ZLL color loop set command frame to ZR with the
                                    update flags field = 0x01 (update action), the action field =
                                    0x00 (deactivate), the direction field = 0x00, the time field =
                                    0x0000 and the start hue field = 0x0000. ZR stops the color loop
                                    cycle and returns to its previous hue (red).*/
  TEST_STEP_7A_COLOR_LOOP_SET,  /** ZED unicasts a ZLL color loop set command frame to ZR with the
                                    update flags field = 0x07 (update action, direction and time),
                                    the action field = 0x02 (start from enhanced current hue), the
                                    direction field = 0x01 (increment), the time field = 0x0064
                                    (100s) and the start hue field = 0x0000. ZR begins a color loop
                                    cycle (starting from the current enhanced hue) with a color
                                    cycle time of 100s. */
  TEST_STEP_7B_COLOR_LOOP_SET,  /** ZED unicasts a ZLL color loop set command frame to ZR with the
                                    update flags field = 0x04 (update time), the action field =
                                    0x00, the direction field = 0x00, the time field = 0x000a (10s)
                                    and the start hue field = 0x0000. ZR adapts its color loop cycle
                                    time to 10s. */
  TEST_STEP_7C_COLOR_LOOP_SET,  /** ZED unicasts a ZLL color loop set command frame to ZR with the
                                    update flags field = 0x01 (update action), the action field =
                                    0x00 (deactivate), the direction field = 0x00, the time field set
                                    to 0x0000 and the start hue field = 0x0000. ZR stops the color
                                    loop cycle and returns to its previous hue (red). */
  TEST_STEP_8A_READ_ATTR,       /** ZED unicasts a ZCL read attributes command frame for the
                                    EnhancedCurrentHue attribute to ZR. ZR unicasts a ZCL read
                                    attributes response command frame to ZED. EnhancedCurrentHue
                                    attribute has a value commensurate with a red hue. */
  TEST_STEP_8B_COLOR_LOOP_SET,  /** ZED unicasts a ZLL color loop set command frame to ZR with the
                                    update flags field = 0x05 (update action and time), the action
                                    field = 0x02 (start from enhanced current hue), the direction field
                                    = 0x00, the time field = 0x001e (30s) and the start hue field =
                                    0x0000. ZR begins a color loop cycle (starting from the current
                                    enhanced hue) with a color cycle time of 30s.*/
  TEST_STEP_8C_READ_ATTR,       /** ZED unicasts a ZCL read attributes command frame for the
                                    ColorLoopStoredEnhancedHue attribute to ZR. ZR unicasts a ZCL
                                    read attributes response command frame to ZED. ColorLoopStoredEnhancedHue
                                    attribute = equal to the value of the EnhancedCurrentHue
                                    attribute read in step 8. */
  TEST_STEP_8D_COLOR_LOOP_SET,  /** ZED unicasts a ZLL color loop set command frame to ZR with the
                                    update flags field = 0x01 (update action), the action
                                    field = 0x00 (deactivate), the direction field = 0x00, the time
                                    field = 0x0000 and the start hue field = 0x0000. ZR stops the
                                    color loop cycle and returns to its previous hue (red).*/
  TEST_STEP_8E_READ_ATTR,       /** ZED unicasts a ZCL read attributes command frame for the
                                    EnhancedCurrentHue attribute to ZR. ZR unicasts a ZCL read
                                    attributes response command frame to ZED. EnhancedCurrentHue
                                    attribute has a value equal to the value of the
                                    ColorLoopStoredEnhancedHue attribute read in step 8b.*/
  TEST_STEP_9A_COLOR_LOOP_SET,  /** ZED unicasts a ZLL color loop set command frame to ZR with the
                                    update flags field = 0x07 (update action, direction and time),
                                    the action field = 0x02 (start from enhanced current hue), the
                                    direction field = 0x01 (increment), the time field = 0x000a
                                    (10s) and the start hue field = 0x0000. ZR begins a color loop
                                    cycle (starting from the current enhanced hue) with a color cycle
                                    time of 10s. */
  TEST_STEP_9B_ENHANCED_MOVE_TO_HUE, /** ZED unicasts a ZLL enhanced move to hue command frame to ZR
                                         with the enhanced hue field = an appropriate value for a
                                         blue hue, the direction field = 0x00 (shortest distance)
                                         and the transition time field = 0x000a (1s). ZR may change
                                         to a blue hue but the color loop continues. */
  TEST_STEP_9C_COLOR_LOOP_SET, /** ZED unicasts a ZLL color loop set command frame to ZR with the
                                   update flags field = 0x01 (update action), the action field =
                                   0x00 (deactivate), the direction field = 0x00, the time field set
                                   to 0x0000 and the start hue field = 0x0000. ZR stops the color
                                   loop cycle and returns to its previous hue (red).*/

  TEST_STEP_FINISHED    /**/  /**< Test finished pseudo-step. */
};

zb_uint8_t g_test_step = TEST_STEP_INITIAL;
zb_uint8_t current_seq_number;


/** Device's extended address. */
zb_ieee_addr_t g_ed_addr = {0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

zb_uint16_t ex_hue;

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

  test_next_step(param);

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

    case TEST_STEP_1A_COLOR_LOOP_SET:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_1A_COLOR_LOOP_SET", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_COLOR_LOOP_SET_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_UPDATE_DIRECTION |
            ZB_ZCL_CMD_COLOR_CONTROL_LOOP_UPDATE_TIME |
            ZB_ZCL_CMD_COLOR_CONTROL_LOOP_UPDATE_START_HUE,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_ACTION_DEACTIVATE,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_DIRECTION_DECREMENT, 0x000a, 0x0000);
      break;

    case TEST_STEP_1B_READ_ATTR:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_1B_READ_ATTR", (FMT__0));
      SEND_READ_ATTR_4(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL,
                       ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_LOOP_ACTIVE_ZLL_ID,
                       ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_LOOP_DIRECTION_ZLL_ID,
                       ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_LOOP_TIME_ZLL_ID,
                       ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_LOOP_START_ZLL_ID);
      break;

    case TEST_STEP_2A_COLOR_LOOP_SET:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_2A_COLOR_LOOP_SET", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_COLOR_LOOP_SET_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_UPDATE_DIRECTION,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_ACTION_START_HUE,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_DIRECTION_INCREMENT, 0x0000, 0x0000);
      break;

    case TEST_STEP_2B_READ_ATTR:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_2B_READ_ATTR", (FMT__0));
      SEND_READ_ATTR_4(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL,
                       ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_LOOP_ACTIVE_ZLL_ID,
                       ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_LOOP_DIRECTION_ZLL_ID,
                       ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_LOOP_TIME_ZLL_ID,
                       ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_LOOP_START_ZLL_ID);
      break;

    case TEST_STEP_3A_COLOR_LOOP_SET:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_3A_COLOR_LOOP_SET", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_COLOR_LOOP_SET_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_UPDATE_ACTION,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_ACTION_CURRENT_HUE,
        0x00, 0x0000, 0x0000);
      ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 1*ZB_TIME_ONE_SECOND);
      break;

    case TEST_STEP_3B_READ_ATTR:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_3B_READ_ATTR", (FMT__0));
      SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_LOOP_ACTIVE_ZLL_ID);
      //ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 3*ZB_TIME_ONE_SECOND);
      break;

    case TEST_STEP_3C_COLOR_LOOP_SET:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_3C_READ_ATTR", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_COLOR_LOOP_SET_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_UPDATE_ACTION,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_ACTION_DEACTIVATE, 0x00, 0x0000, 0x0000);
      break;

    case TEST_STEP_3D_READ_ATTR:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_3D_READ_ATTR", (FMT__0));
      SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_LOOP_ACTIVE_ZLL_ID);
      break;

    case TEST_STEP_4A_COLOR_LOOP_SET:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_4A_COLOR_LOOP_SET", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_COLOR_LOOP_SET_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_UPDATE_ACTION,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_ACTION_START_HUE, 0x00, 0x0000, 0x0000);
      ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 12*ZB_TIME_ONE_SECOND);
      break;

    case TEST_STEP_4B_READ_ATTR:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_4B_READ_ATTR", (FMT__0));
      SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_LOOP_STORED_ZLL_ID);
      break;

    case TEST_STEP_4C_COLOR_LOOP_SET:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_4C_COLOR_LOOP_SET", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_COLOR_LOOP_SET_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_UPDATE_ACTION,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_ACTION_DEACTIVATE, 0x00, 0x0000, 0x0000);
      break;

    case TEST_STEP_5A_COLOR_LOOP_SET:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_5A_COLOR_LOOP_SET", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_COLOR_LOOP_SET_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_UPDATE_ACTION,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_ACTION_CURRENT_HUE, 0x00, 0x0000, 0x0000);
      ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 12*ZB_TIME_ONE_SECOND);
      break;

    case TEST_STEP_5B_READ_ATTR:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_5B_READ_ATTR", (FMT__0));
      SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_LOOP_STORED_ZLL_ID);
      break;

    case TEST_STEP_5C_COLOR_LOOP_SET:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_5C_COLOR_LOOP_SET", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_COLOR_LOOP_SET_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_UPDATE_ACTION,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_ACTION_DEACTIVATE, 0x00, 0x0000, 0x0000);
      break;

    case TEST_STEP_6A_COLOR_LOOP_SET:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_6A_COLOR_LOOP_SET", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_COLOR_LOOP_SET_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_UPDATE_ACTION | ZB_ZCL_CMD_COLOR_CONTROL_LOOP_UPDATE_TIME,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_ACTION_CURRENT_HUE,
        0x00, 0x0064, 0x0000);
      ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 10*ZB_TIME_ONE_SECOND);
      break;

    case TEST_STEP_6B_COLOR_LOOP_SET:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_6B_COLOR_LOOP_SET", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_COLOR_LOOP_SET_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_UPDATE_DIRECTION,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_ACTION_DEACTIVATE,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_DIRECTION_DECREMENT, 0x0000, 0x0000);
      ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 10*ZB_TIME_ONE_SECOND);
      break;

    case TEST_STEP_6C_COLOR_LOOP_SET:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_6C_COLOR_LOOP_SET", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_COLOR_LOOP_SET_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_UPDATE_ACTION,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_ACTION_DEACTIVATE, 0x00, 0x0000, 0x0000);
      break;

    case TEST_STEP_7A_COLOR_LOOP_SET:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_7A_COLOR_LOOP_SET", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_COLOR_LOOP_SET_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_UPDATE_ACTION |
          ZB_ZCL_CMD_COLOR_CONTROL_LOOP_UPDATE_DIRECTION |
          ZB_ZCL_CMD_COLOR_CONTROL_LOOP_UPDATE_TIME,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_ACTION_CURRENT_HUE,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_DIRECTION_INCREMENT, 0x0064, 0x0000);
      ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 10*ZB_TIME_ONE_SECOND);
      break;

    case TEST_STEP_7B_COLOR_LOOP_SET:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_7B_COLOR_LOOP_SET", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_COLOR_LOOP_SET_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_UPDATE_TIME,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_ACTION_DEACTIVATE, 0x00, 0x000a, 0x0000);
      break;

    case TEST_STEP_7C_COLOR_LOOP_SET:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_7C_COLOR_LOOP_SET", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_COLOR_LOOP_SET_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_UPDATE_ACTION,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_ACTION_DEACTIVATE, 0x00, 0x0000, 0x0000);
      break;

    case TEST_STEP_8A_READ_ATTR:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_8A_READ_ATTR", (FMT__0));
      SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL, ZB_ZCL_ATTR_COLOR_CONTROL_ENHANCED_CURRENT_HUE_ZLL_ID);
      break;

    case TEST_STEP_8B_COLOR_LOOP_SET:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_8B_COLOR_LOOP_SET", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_COLOR_LOOP_SET_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_UPDATE_ACTION | ZB_ZCL_CMD_COLOR_CONTROL_LOOP_UPDATE_TIME,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_ACTION_CURRENT_HUE, 0x00, 0x001e, 0x0000);
      ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 10*ZB_TIME_ONE_SECOND);
      break;

    case TEST_STEP_8C_READ_ATTR:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_8C_READ_ATTR", (FMT__0));
      SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_LOOP_STORED_ZLL_ID);
      break;

    case TEST_STEP_8D_COLOR_LOOP_SET:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_8D_COLOR_LOOP_SET", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_COLOR_LOOP_SET_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_UPDATE_ACTION,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_ACTION_DEACTIVATE, 0x00, 0x0000, 0x0000);
      break;

    case TEST_STEP_8E_READ_ATTR:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_8E_READ_ATTR", (FMT__0));
      SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL, ZB_ZCL_ATTR_COLOR_CONTROL_ENHANCED_CURRENT_HUE_ZLL_ID);
      break;

    case TEST_STEP_9A_COLOR_LOOP_SET:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_9A_COLOR_LOOP_SET", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_COLOR_LOOP_SET_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_UPDATE_ACTION |
          ZB_ZCL_CMD_COLOR_CONTROL_LOOP_UPDATE_DIRECTION |
          ZB_ZCL_CMD_COLOR_CONTROL_LOOP_UPDATE_TIME,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_ACTION_CURRENT_HUE,
        ZB_ZCL_CMD_COLOR_CONTROL_LOOP_DIRECTION_INCREMENT, 0x000a, 0x0000);
      ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 20*ZB_TIME_ONE_SECOND);
      break;

    case TEST_STEP_9B_ENHANCED_MOVE_TO_HUE:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_9B_ENHANCED_MOVE_TO_HUE", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_ENHANCED_MOVE_TO_HUE_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
        ZB_ZCL_COLOR_CONTROL_ENHANCED_BLUE, ZB_ZCL_CMD_COLOR_CONTROL_MOVE_TO_HUE_SHORTEST, 0x000a);
      break;

    case TEST_STEP_9C_COLOR_LOOP_SET:
      TRACE_MSG(TRACE_ZLL3, "TEST_STEP_9C_COLOR_LOOP_SET", (FMT__0));
      ZB_ZCL_COLOR_CONTROL_SEND_COLOR_LOOP_SET_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
          ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL,
          ZB_ZCL_CMD_COLOR_CONTROL_LOOP_UPDATE_ACTION,
          ZB_ZCL_CMD_COLOR_CONTROL_LOOP_ACTION_DEACTIVATE, 0x00, 0x0000, 0x0000);
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
    case TEST_STEP_1B_READ_ATTR:
      TEST_ATTRIBUTE_8(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_LOOP_ACTIVE_ZLL_ID, 0x00);

      if(g_error_cnt==0)
      {
        ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(cmd_buf, read_attr_resp);
        TEST_ATTRIBUTE_8(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_LOOP_DIRECTION_ZLL_ID, 0x00);
      }

      if(g_error_cnt==0)
      {
        ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(cmd_buf, read_attr_resp);
        TEST_ATTRIBUTE_16_SHARP(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_LOOP_TIME_ZLL_ID, 0x000a);
      }

      if(g_error_cnt==0)
      {
        ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(cmd_buf, read_attr_resp);
        TEST_ATTRIBUTE_16_SHARP(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_LOOP_START_ZLL_ID, 0x0000);
      }
      break;

    case TEST_STEP_2B_READ_ATTR:
      TEST_ATTRIBUTE_8(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_LOOP_ACTIVE_ZLL_ID, 0x00);

      if(g_error_cnt==0)
      {
        ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(cmd_buf, read_attr_resp);
        TEST_ATTRIBUTE_8(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_LOOP_DIRECTION_ZLL_ID, 0x01);
      }

      if(g_error_cnt==0)
      {
        ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(cmd_buf, read_attr_resp);
        TEST_ATTRIBUTE_16_SHARP(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_LOOP_TIME_ZLL_ID, 0x000a);
      }

      if(g_error_cnt==0)
      {
        ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(cmd_buf, read_attr_resp);
        TEST_ATTRIBUTE_16_SHARP(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_LOOP_START_ZLL_ID, 0x0000);
      }
      break;

    case TEST_STEP_3B_READ_ATTR:
      TEST_ATTRIBUTE_8(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_LOOP_ACTIVE_ZLL_ID, 0x01);
      break;

    case TEST_STEP_3D_READ_ATTR:
      TEST_ATTRIBUTE_8(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_LOOP_ACTIVE_ZLL_ID, 0x00);
      break;

    case TEST_STEP_4B_READ_ATTR:
      TEST_ATTRIBUTE_16_SHARP(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_LOOP_STORED_ZLL_ID,
                              ZB_ZCL_COLOR_CONTROL_ENHANCED_HUE_RED);
      break;

    case TEST_STEP_5B_READ_ATTR:
      TEST_ATTRIBUTE_16_SHARP(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_LOOP_STORED_ZLL_ID,
                              ZB_ZCL_COLOR_CONTROL_ENHANCED_HUE_RED);
      break;

    case TEST_STEP_8A_READ_ATTR:
      TEST_ATTRIBUTE_16_SHARP(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_ENHANCED_CURRENT_HUE_ZLL_ID,
                              ZB_ZCL_COLOR_CONTROL_ENHANCED_HUE_RED);
      break;

    case TEST_STEP_8C_READ_ATTR:
      ex_hue = *(zb_uint16_t*)read_attr_resp->attr_value;
      TEST_ATTRIBUTE_16_SHARP(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_LOOP_STORED_ZLL_ID,
                              ZB_ZCL_COLOR_CONTROL_ENHANCED_HUE_RED);
      break;

    case TEST_STEP_8E_READ_ATTR:
      TEST_ATTRIBUTE_16_SHARP(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_ENHANCED_CURRENT_HUE_ZLL_ID,
          ex_hue);
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
