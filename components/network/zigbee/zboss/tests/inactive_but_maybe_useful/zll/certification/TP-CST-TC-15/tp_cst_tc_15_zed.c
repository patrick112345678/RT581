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

#define ZB_TRACE_FILE_ID 41604
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

  TEST_STEP_1_READ_ATTR,        /**< ZED unicasts a ZCL read attributes ColorCapabilities.
                                     ColorCapabilities = 0x001f */
  TEST_STEP_2_MOVE_TO_COLOR,    /**< ZED unicasts a ZCL move to color command with the ColorX and ColorY
                                      = red color and the transition time = 0x0032 (5s).*/
  TEST_STEP_3A_MOVE_TO_COLOR,   /**< ZED unicasts a ZCL move to color command with the ColorX and ColorY
                                      = green color and the transition time = 0x00c8 (20s). */
  TEST_STEP_3B_READ_ATTR,       /**< ZED unicasts a ZCL read attributes RemainingTime. RemainingTime
                                      approximately = 0x0064. */
  TEST_STEP_3C_READ_ATTR,       /**< ZED unicasts a ZCL read attributes RemainingTime.RemainingTime  = 0x0000. */
  TEST_STEP_4_READ_ATTR,        /**< ZED unicasts a ZCL read attributes CurrentX and CurrentY. CurrentX and CurrentY
                                   = green color. */
  TEST_STEP_5_READ_ATTR,        /**< ZED unicasts a ZCL read attributes ColorMode.
                                      ColorMode = 0x01 (CurrentX and CurrentY).*/
  TEST_STEP_6_READ_ATTR,        /**< ZED unicasts a ZCL read attributes NumberOfPrimaries.
                                      NumberOfPrimaries between 0x01 and 0x06. */
  TEST_STEP_6A_READ_ATTR,       /**< ZED unicasts a ZCL read attributes Primary1X, Primary1Y and Primary1Intensity.
                                      Primary1X, Primary1Y and Primary1Intensity attributes appropriate for primary 1.*/
  TEST_STEP_6B_READ_ATTR,       /**< ZED unicasts a ZCL read attributes Primary2X, Primary2Y and Primary1Intensity.
                                      Primary1X, Primary1Y and Primary1Intensity attributes appropriate for primary 2.*/
  TEST_STEP_6C_READ_ATTR,       /**< ZED unicasts a ZCL read attributes Primary3X, Primary3Y and Primary1Intensity.
                                      Primary1X, Primary1Y and Primary1Intensity attributes appropriate for primary 3.*/
  TEST_STEP_6D_READ_ATTR,       /**< ZED unicasts a ZCL read attributes Primary4X, Primary4Y and Primary1Intensity.
                                      Primary1X, Primary1Y and Primary1Intensity attributes appropriate for primary 4.*/
  TEST_STEP_6E_READ_ATTR,       /**< ZED unicasts a ZCL read attributes Primary5X, Primary5Y and Primary1Intensity.
                                      Primary1X, Primary1Y and Primary1Intensity attributes appropriate for primary 5.*/
  TEST_STEP_6F_READ_ATTR,       /**< ZED unicasts a ZCL read attributes Primary6X, Primary6Y and Primary1Intensity.
                                      Primary1X, Primary1Y and Primary1Intensity attributes appropriate for primary 6.*/
  TEST_STEP_7_READ_ATTR,        /**< ZED unicasts a ZCL read attributes EnhancedColorMode. EnhancedColorMode =
                                      0x01 (CurrentX and CurrentY). */
  TEST_STEP_8_MOVE_TO_HUE,      /**< ZED unicasts a ZCL move to hue command with the hue = blue hue,
                                      direction = 0x00 (shortest distance), transition time = 0x0032 (5s). */
  TEST_STEP_9_READ_ATTR,        /**< ZED unicasts a ZCL read attributes ColorMode and EnhancedColorMode.
                                      ColorMode and EnhancedColorMode attributes both = 0x00 (CurrentHue and CurrentSaturation).*/
  TEST_STEP_10_MOVE_HUE,        /**< ZED unicasts a ZCL move hue command with the move mode  = 0x01 (move up),
                                      rate = 0x0a (10). ZR changes hue in an upward direction.*/
  TEST_STEP_11_MOVE_HUE,        /**< ZED unicasts a ZCL move hue commandwith the move mode = 0x03 (move down),
                                      rate = 0x14 (20). ZR changes hue in a downward direction. */
  TEST_STEP_12_MOVE_HUE,        /**< ZED unicasts a ZCL move hue command frame to ZR with the move mode = 0x00 (stop). */
  TEST_STEP_13_STEP_HUE,        /**< ZED unicasts a ZCL step hue command frame to ZR with the step mode = 0x01 (step up), the
                                      step size = 0x40 (64) and the transition time = 0x64 (100). */
  TEST_STEP_14_MOVE_TO_SATURATION,  /**< ZED unicasts a ZCL move to saturation command with the saturation = 0x00 (white) and
                                      the transition time = 0x0032 (5s). */
  TEST_STEP_15_MOVE_SATURATION, /**< ZED unicasts a ZCL move saturation command with the move mode 0x01 (move up) and
                                      the rate = 0x0a (10). */
  TEST_STEP_16_MOVE_SATURATION, /**< ZED unicasts a ZCL move saturation command with the move mode = 0x03 (move down) and
                                      the rate = 0x14 (20). */
  TEST_STEP_17_MOVE_SATURATION, /**< ZED unicasts a ZCL move saturation command with the move mode = 0x00 (stop). */
  TEST_STEP_18_STEP_SATURATION, /**< ZED unicasts a ZCL step saturation command with the step mode = 0x01 (step up),
                                      the step size = 0x40 (64) and the transition time = 0x64 (100).  */
  TEST_STEP_19A_MOVE_TO_HUE_AND_SATURATION, /**< ZED unicasts a ZCL move to hue and saturation command with
                                      the hue = blue hue, the saturation field = 0x7f (50%) and
                                      the transition time = 0x0064 (10s).  */
  TEST_STEP_19B_READ_ATTR,      /**< ZED unicasts a ZCL read attributes CurrentHue. CurrentHue = blue hue. */
  TEST_STEP_19C_READ_ATTR,      /**< ZED unicasts a ZCL read attributes CurrentSaturation. CurrentSaturation = 0x80. */
  TEST_STEP_20A_STORE_SCENE,    /**< ZED unicasts a ZCL store scene command with the group ID = 0x0000 and
                                      the scene ID = 0x01. */
  TEST_STEP_20B_READ_ATTR,      /**< ZED unicasts a ZCL read attributes CurrentX, CurrentY, EnhancedCurrentHue and
                                      CurrentSaturation. Save this values. */
  TEST_STEP_20C_MOVE_TO_COLOR,  /**< ZED unicasts a ZCL move to color command with the ColorX and ColorY = red color and
                                     the transition time = 0x0032 (5s).  */
  TEST_STEP_20D_RECALL_SCENE,   /**< ZED unicasts a ZCL recall scene command with the group ID = 0x0000 and the
                                      scene ID = 0x01.  */
  TEST_STEP_20E_READ_ATTR,      /**< ZED unicasts a ZCL read attributes CurrentX, CurrentY, EnhancedCurrentHue and
                                      CurrentSaturation. Compare this values with step 20a. */
  TEST_STEP_21_MOVE_TO_COLOR,   /**< ZED unicasts a ZCL move to color command with the ColorX and ColorY = green color
                                      and the transition time = 0x00c8 (20s).  */
  TEST_STEP_22_MOVE_COLOR,      /**< ZED unicasts a ZCL move color command with the RateX = 0x03e8 (+1000) and
                                      the RateY = 0xd8f0 (-10000).  */
  TEST_STEP_23_MOVE_COLOR,      /**< ZED unicasts a ZCL move color command with the RateX = 0xec78 (-5000) and
                                      the RateY =to 0xff9c (-100).  */
  TEST_STEP_24_MOVE_COLOR,      /**< ZED unicasts a ZCL move color command with the RateX and RateY = 0x0000 (stop).  */
  TEST_STEP_25_MOVE_TO_COLOR,   /**< ZED unicasts a ZCL move to color command with the ColorX and ColorY =
                                      tester`s choice of color and the transition time = 0x0032 (5s). */
  TEST_STEP_26_STEP_COLOR,      /**< ZED unicasts a ZCL step color command with the StepX = 0x01f4 (+500),
                                      the StepY = 0xfc18 (-1000) and the transition time = 0x0064 (10s). */
  TEST_STEP_27A_ENHANCED_MOVE_TO_HUE, /**< ZED unicasts a ZLL enhanced move to hue command with
                                      the enhanced hue field = blue hue, the direction = 0x00 (shortest distance)
                                      and the transition time = 0x0032 (5s).  */
  TEST_STEP_27B_READ_ATTR,      /**< ZED unicasts a ZCL read attributes EnhancedCurrentHue. EnhancedCurrentHue = a blue hue. */
  TEST_STEP_28_ENHANCED_MOVE_TO_HUE,  /**< ZED unicasts a ZLL enhanced move to hue command with
                                      the enhanced hue = unobtainable hue, the direction = 0x00 (shortest distance)
                                      and the transition time = 0x0032 (5s). ZR unicasts default response command with
                                      the status = (INVALID_VALUE).
                                      NOTE unobtainable hue depended from H/W. This step is excluded. */
  TEST_STEP_29_ENHANCED_MOVE_HUE, /**< ZED unicasts a ZLL enhanced move hue command with the move mode = 0x01
                                      (move up) and the rate = 0x0a (10).  */
  TEST_STEP_30_ENHANCED_MOVE_HUE, /**< ZED unicasts a ZLL enhanced move hue command with the move mode = 0x03
                                      (move down) and the rate = 0x14 (20).  */
  TEST_STEP_31_ENHANCED_MOVE_HUE, /**< ZED unicasts a ZLL enhanced move hue command with the move mode = 0x00 (stop).  */
  TEST_STEP_32_ENHANCED_STEP_HUE, /**< ZED unicasts a ZLL enhanced step hue command with the step mode = 0x01
                                      (step up), the step size = 0x4000 (16384) and the transition time =0x0a (10).  */
  TEST_STEP_33_ENHANCED_MOVE_TO_HUE_SATURATION, /**< ZED unicasts a ZLL enhanced move to hue and saturation command
                                      with the enhanced hue = blue hue, the saturation = 0x7f (50%) and
                                      the transition time = 0x0a (10). */
  TEST_STEP_34_ENHANCED_MOVE_TO_HUE_SATURATION, /**< ZED unicasts a ZLL enhanced move to hue and saturation command
                                      with the enhanced hue = unobtainable hue, the saturation = 0xfe (maximum
                                      saturation) and the transition time = 0x0032 (5s). ZR unicasts default response
                                      command with the status = 0x87 (INVALID_VALUE).
                                      NOTE unobtainable hue depended from H/W. This step is excluded. */
  TEST_STEP_35A_MOVE_TO_COLOR,  /**< ZED unicasts a ZCL move to color command with the ColorX and ColorY = tester`s
                                      choice and the transition = 0x0064 (10s). */
  TEST_STEP_35B_STOP_MOVE_STEP, /**< ZED unicasts a ZLL stop move step command. ZR stops changing color. */

  TEST_STEP_FINISHED            /**< Test finished pseudo-step. */
};

zb_uint8_t g_test_step = TEST_STEP_INITIAL;
zb_uint8_t current_seq_number;


/** Device's extended address. */
zb_ieee_addr_t g_ed_addr = {0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

zb_uint16_t tester_color_x = 1234;
zb_uint16_t tester_color_y = 4321;

zb_uint16_t custom_color_x;
zb_uint16_t custom_color_y;
zb_uint16_t custom_ex_hue;
zb_uint8_t custom_saturation;

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

  // for TEST_STEP_3B_READ_ATTR next step only zb_scheduler_alarm!
  if (g_test_step == TEST_STEP_3B_READ_ATTR)
  {
    zb_free_buf(zcl_cmd_buf);
  }
  else
  {
    test_next_step(param);
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

  case TEST_STEP_1_READ_ATTR:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_1_READ_ATTR", (FMT__0));
    SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_CAPABILITIES_ZLL_ID);
    break;

  case TEST_STEP_2_MOVE_TO_COLOR: // step 5
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_2_MOVE_TO_COLOR", (FMT__0));
    ZB_ZCL_COLOR_CONTROL_SEND_MOVE_TO_COLOR_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, ZB_ZCL_COLOR_CONTROL_COLOR_X_GREEN,
        ZB_ZCL_COLOR_CONTROL_COLOR_Y_GREEN, 0x0032);
    break;

  case TEST_STEP_3A_MOVE_TO_COLOR:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_3A_MOVE_TO_COLOR", (FMT__0));
    ZB_ZCL_COLOR_CONTROL_SEND_MOVE_TO_COLOR_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, ZB_ZCL_COLOR_CONTROL_COLOR_X_GREEN,
        ZB_ZCL_COLOR_CONTROL_COLOR_Y_GREEN, 0x00c8);
    ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 10*ZB_TIME_ONE_SECOND);
    break;

  case TEST_STEP_3B_READ_ATTR:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_3B_READ_ATTR", (FMT__0));
    SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL, ZB_ZCL_ATTR_COLOR_CONTROL_REMAINING_TIME_ID);
    ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 15*ZB_TIME_ONE_SECOND);
    break;

  case TEST_STEP_3C_READ_ATTR:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_3C_READ_ATTR", (FMT__0));
    SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL, ZB_ZCL_ATTR_COLOR_CONTROL_REMAINING_TIME_ID);
    break;

  case TEST_STEP_4_READ_ATTR:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_4_READ_ATTR", (FMT__0));
    SEND_READ_ATTR_2(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL, ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_X_ID,
        ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_Y_ID);
    break;

  case TEST_STEP_5_READ_ATTR:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_5_READ_ATTR", (FMT__0));
    SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_MODE_ID);
    break;

  case TEST_STEP_6_READ_ATTR:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_6_READ_ATTR", (FMT__0));
    SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL, ZB_ZCL_ATTR_COLOR_CONTROL_NUMBER_OF_PRIMARIES_ID);
    break;

  case TEST_STEP_6A_READ_ATTR:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_6A_READ_ATTR", (FMT__0));
    SEND_READ_ATTR_3(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL, ZB_ZCL_ATTR_COLOR_CONTROL_PRIMARY_1_X_ID,
        ZB_ZCL_ATTR_COLOR_CONTROL_PRIMARY_1_Y_ID, ZB_ZCL_ATTR_COLOR_CONTROL_PRIMARY_1_INTENSITY_ID);
    break;

  case TEST_STEP_6B_READ_ATTR:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_6B_READ_ATTR", (FMT__0));
    SEND_READ_ATTR_3(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL, ZB_ZCL_ATTR_COLOR_CONTROL_PRIMARY_2_X_ID,
        ZB_ZCL_ATTR_COLOR_CONTROL_PRIMARY_2_Y_ID, ZB_ZCL_ATTR_COLOR_CONTROL_PRIMARY_2_INTENSITY_ID);
    break;

  case TEST_STEP_6C_READ_ATTR:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_6C_READ_ATTR", (FMT__0));
    SEND_READ_ATTR_3(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL, ZB_ZCL_ATTR_COLOR_CONTROL_PRIMARY_3_X_ID,
        ZB_ZCL_ATTR_COLOR_CONTROL_PRIMARY_3_Y_ID, ZB_ZCL_ATTR_COLOR_CONTROL_PRIMARY_3_INTENSITY_ID);
    break;

  case TEST_STEP_6D_READ_ATTR:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_6D_READ_ATTR", (FMT__0));
    SEND_READ_ATTR_3(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL, ZB_ZCL_ATTR_COLOR_CONTROL_PRIMARY_4_X_ID,
        ZB_ZCL_ATTR_COLOR_CONTROL_PRIMARY_4_Y_ID, ZB_ZCL_ATTR_COLOR_CONTROL_PRIMARY_4_INTENSITY_ID);
    break;

  case TEST_STEP_6E_READ_ATTR:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_6E_READ_ATTR", (FMT__0));
    SEND_READ_ATTR_3(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL, ZB_ZCL_ATTR_COLOR_CONTROL_PRIMARY_5_X_ID,
        ZB_ZCL_ATTR_COLOR_CONTROL_PRIMARY_5_Y_ID, ZB_ZCL_ATTR_COLOR_CONTROL_PRIMARY_5_INTENSITY_ID);
    break;

  case TEST_STEP_6F_READ_ATTR:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_6F_READ_ATTR", (FMT__0));
    SEND_READ_ATTR_3(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL, ZB_ZCL_ATTR_COLOR_CONTROL_PRIMARY_6_X_ID,
        ZB_ZCL_ATTR_COLOR_CONTROL_PRIMARY_6_Y_ID, ZB_ZCL_ATTR_COLOR_CONTROL_PRIMARY_6_INTENSITY_ID);
    break;

  case TEST_STEP_7_READ_ATTR:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_7_READ_ATTR", (FMT__0));
    SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL, ZB_ZCL_ATTR_COLOR_CONTROL_ENHANCED_COLOR_MODE_ZLL_ID);
    break;

  case TEST_STEP_8_MOVE_TO_HUE:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_8_MOVE_TO_HUE", (FMT__0));
    ZB_ZCL_COLOR_CONTROL_SEND_MOVE_TO_HUE_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, ZB_ZCL_COLOR_CONTROL_HUE_BLUE,
        ZB_ZCL_CMD_COLOR_CONTROL_MOVE_TO_HUE_SHORTEST, 0x0032);
    break;

  case TEST_STEP_9_READ_ATTR:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_9_READ_ATTR", (FMT__0));
    SEND_READ_ATTR_2(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_MODE_ID,
        ZB_ZCL_ATTR_COLOR_CONTROL_ENHANCED_COLOR_MODE_ZLL_ID);
    break;

  case TEST_STEP_10_MOVE_HUE:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_10_MOVE_HUE", (FMT__0));
    ZB_ZCL_COLOR_CONTROL_SEND_MOVE_HUE_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, ZB_ZCL_CMD_COLOR_CONTROL_MOVE_UP, 0x0a);
    ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 15*ZB_TIME_ONE_SECOND);
    break;

  case TEST_STEP_11_MOVE_HUE:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_11_MOVE_HUE", (FMT__0));
    ZB_ZCL_COLOR_CONTROL_SEND_MOVE_HUE_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, ZB_ZCL_CMD_COLOR_CONTROL_MOVE_DOWN, 0x14);
    ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 30*ZB_TIME_ONE_SECOND);
    break;

  case TEST_STEP_12_MOVE_HUE:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_12_MOVE_HUE", (FMT__0));
    ZB_ZCL_COLOR_CONTROL_SEND_MOVE_HUE_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, ZB_ZCL_CMD_COLOR_CONTROL_MOVE_STOP, 0x00);
    break;

  case TEST_STEP_13_STEP_HUE:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_13_STEP_HUE", (FMT__0));
    ZB_ZCL_COLOR_CONTROL_SEND_STEP_HUE_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, ZB_ZCL_CMD_COLOR_CONTROL_STEP_UP, 0x40, 0x0064);
    ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 5*ZB_TIME_ONE_SECOND);
    break;

  case TEST_STEP_14_MOVE_TO_SATURATION:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_14_MOVE_TO_SATURATION", (FMT__0));
    ZB_ZCL_COLOR_CONTROL_SEND_MOVE_TO_SATURATION_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, 0x00, 0x0032);
    break;

  case TEST_STEP_15_MOVE_SATURATION:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_15_MOVE_SATURATION", (FMT__0));
    ZB_ZCL_COLOR_CONTROL_SEND_MOVE_SATURATION_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, ZB_ZCL_CMD_COLOR_CONTROL_MOVE_UP, 0x0a);
    break;

  case TEST_STEP_16_MOVE_SATURATION:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_16_MOVE_SATURATION", (FMT__0));
    ZB_ZCL_COLOR_CONTROL_SEND_MOVE_SATURATION_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, ZB_ZCL_CMD_COLOR_CONTROL_MOVE_DOWN, 0x14);
    break;

  case TEST_STEP_17_MOVE_SATURATION:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_17_MOVE_SATURATION", (FMT__0));
    ZB_ZCL_COLOR_CONTROL_SEND_MOVE_SATURATION_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, ZB_ZCL_CMD_COLOR_CONTROL_MOVE_STOP, 0x00);
    break;

  case TEST_STEP_18_STEP_SATURATION:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_18_STEP_SATURATION", (FMT__0));
    ZB_ZCL_COLOR_CONTROL_SEND_STEP_SATURATION_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, ZB_ZCL_CMD_COLOR_CONTROL_STEP_UP, 0x40, 0x0064);
    break;

  case TEST_STEP_19A_MOVE_TO_HUE_AND_SATURATION:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_19A_MOVE_TO_HUE_AND_SATURATION", (FMT__0));
    ZB_ZCL_COLOR_CONTROL_SEND_MOVE_TO_HUE_SATURATION_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, ZB_ZCL_COLOR_CONTROL_HUE_BLUE, 0x7f, 0x0064);
    break;

  case TEST_STEP_19B_READ_ATTR:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_19B_READ_ATTR", (FMT__0));
    SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL, ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_HUE_ID);
    break;

  case TEST_STEP_19C_READ_ATTR:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_19C_READ_ATTR", (FMT__0));
    SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL, ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_SATURATION_ID);
    break;

  case TEST_STEP_20A_STORE_SCENE:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_20A_STORE_SCENE", (FMT__0));
    ZB_ZCL_SCENES_SEND_STORE_SCENE_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, 0x0000, 0x01);
    break;

  case TEST_STEP_20B_READ_ATTR:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_20B_READ_ATTR", (FMT__0));
    SEND_READ_ATTR_4(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL,
        ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_X_ID, ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_Y_ID,
        ZB_ZCL_ATTR_COLOR_CONTROL_ENHANCED_CURRENT_HUE_ZLL_ID, ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_SATURATION_ID);
    break;

  case TEST_STEP_20C_MOVE_TO_COLOR:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_20C_MOVE_TO_COLOR", (FMT__0));
    ZB_ZCL_COLOR_CONTROL_SEND_MOVE_TO_COLOR_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, ZB_ZCL_COLOR_CONTROL_COLOR_X_RED,
        ZB_ZCL_COLOR_CONTROL_COLOR_Y_RED, 0x0032);
    break;

  case TEST_STEP_20D_RECALL_SCENE:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_20D_RECALL_SCENE", (FMT__0));
    ZB_ZCL_SCENES_SEND_RECALL_SCENE_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, 0x0000, 0x01);
    break;

  case TEST_STEP_20E_READ_ATTR:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_20E_READ_ATTR", (FMT__0));
    SEND_READ_ATTR_4(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL,
        ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_X_ID, ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_Y_ID,
        ZB_ZCL_ATTR_COLOR_CONTROL_ENHANCED_CURRENT_HUE_ZLL_ID, ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_SATURATION_ID);
    break;

  case TEST_STEP_21_MOVE_TO_COLOR:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_21_MOVE_TO_COLOR", (FMT__0));
    ZB_ZCL_COLOR_CONTROL_SEND_MOVE_TO_COLOR_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, ZB_ZCL_COLOR_CONTROL_COLOR_X_GREEN,
        ZB_ZCL_COLOR_CONTROL_COLOR_Y_GREEN, 0x00c8);
    break;

  case TEST_STEP_22_MOVE_COLOR:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_22_MOVE_COLOR", (FMT__0));
    ZB_ZCL_COLOR_CONTROL_SEND_MOVE_COLOR_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, 0x03e8, 0xd8f0);
    break;

  case TEST_STEP_23_MOVE_COLOR:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_23_MOVE_COLOR", (FMT__0));
    ZB_ZCL_COLOR_CONTROL_SEND_MOVE_COLOR_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, 0xec78, 0xff9c);
    break;

  case TEST_STEP_24_MOVE_COLOR:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_24_MOVE_COLOR", (FMT__0));
    ZB_ZCL_COLOR_CONTROL_SEND_MOVE_COLOR_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, 0x0000, 0x0000);
    break;

  case TEST_STEP_25_MOVE_TO_COLOR:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_25_MOVE_TO_COLOR", (FMT__0));
    ZB_ZCL_COLOR_CONTROL_SEND_MOVE_TO_COLOR_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, tester_color_x, tester_color_y, 0x0032);
    break;

  case TEST_STEP_26_STEP_COLOR:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_26_STEP_COLOR", (FMT__0));
    ZB_ZCL_COLOR_CONTROL_SEND_STEP_COLOR_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, 0x01f4, 0xfc18, 0x0064);
    ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 25*ZB_TIME_ONE_SECOND);
    break;

  case TEST_STEP_27A_ENHANCED_MOVE_TO_HUE:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_27A_ENHANCED_MOVE_TO_HUE", (FMT__0));
    ZB_ZCL_COLOR_CONTROL_SEND_ENHANCED_MOVE_TO_HUE_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, ZB_ZCL_COLOR_CONTROL_ENHANCED_BLUE,
        ZB_ZCL_CMD_COLOR_CONTROL_MOVE_TO_HUE_SHORTEST, 0x0032);
    break;

  case TEST_STEP_27B_READ_ATTR:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_27B_READ_ATTR", (FMT__0));
    SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL, ZB_ZCL_ATTR_COLOR_CONTROL_ENHANCED_CURRENT_HUE_ZLL_ID);
    break;

  case TEST_STEP_28_ENHANCED_MOVE_TO_HUE:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_28_ENHANCED_MOVE_TO_HUE", (FMT__0));
    ZB_ZCL_COLOR_CONTROL_SEND_ENHANCED_MOVE_TO_HUE_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, 0xffff,
        ZB_ZCL_CMD_COLOR_CONTROL_MOVE_TO_HUE_SHORTEST, 0x0032);
    break;

  case TEST_STEP_29_ENHANCED_MOVE_HUE:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_29_ENHANCED_MOVE_HUE", (FMT__0));
    ZB_ZCL_COLOR_CONTROL_SEND_ENHANCED_MOVE_HUE_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, ZB_ZCL_CMD_COLOR_CONTROL_MOVE_UP, 0x0a);
    ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 15*ZB_TIME_ONE_SECOND);
    break;

  case TEST_STEP_30_ENHANCED_MOVE_HUE:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_30_ENHANCED_MOVE_HUE", (FMT__0));
    ZB_ZCL_COLOR_CONTROL_SEND_ENHANCED_MOVE_HUE_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, ZB_ZCL_CMD_COLOR_CONTROL_MOVE_DOWN, 0x14);
    ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 30*ZB_TIME_ONE_SECOND);
    break;

  case TEST_STEP_31_ENHANCED_MOVE_HUE:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_31_ENHANCED_MOVE_HUE", (FMT__0));
    ZB_ZCL_COLOR_CONTROL_SEND_ENHANCED_MOVE_HUE_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, ZB_ZCL_CMD_COLOR_CONTROL_MOVE_STOP, 0x00);
    break;

  case TEST_STEP_32_ENHANCED_STEP_HUE:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_32_ENHANCED_STEP_HUE", (FMT__0));
    ZB_ZCL_COLOR_CONTROL_SEND_ENHANCED_STEP_HUE_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, ZB_ZCL_CMD_COLOR_CONTROL_STEP_UP, 0x4000, 0x0a);
    ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 5*ZB_TIME_ONE_SECOND);
    break;

  case TEST_STEP_33_ENHANCED_MOVE_TO_HUE_SATURATION:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_33_ENHANCED_MOVE_TO_HUE_SATURATION", (FMT__0));
    ZB_ZCL_COLOR_CONTROL_SEND_ENHANCED_MOVE_TO_HUE_SATURATION_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, ZB_ZCL_COLOR_CONTROL_ENHANCED_BLUE, 0x7f, 0x0064);
    break;

  case TEST_STEP_34_ENHANCED_MOVE_TO_HUE_SATURATION:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_34_ENHANCED_MOVE_TO_HUE_SATURATION", (FMT__0));
    ZB_ZCL_COLOR_CONTROL_SEND_ENHANCED_MOVE_TO_HUE_SATURATION_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, 0xffff, 0x7f, 0x0032);
    break;

  case TEST_STEP_35A_MOVE_TO_COLOR:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_35A_MOVE_TO_COLOR", (FMT__0));
    ZB_ZCL_COLOR_CONTROL_SEND_MOVE_TO_COLOR_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, tester_color_x, tester_color_y, 0x0064);
    ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 5*ZB_TIME_ONE_SECOND);
    break;

  case TEST_STEP_35B_STOP_MOVE_STEP:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_35B_STOP_MOVE_STEP", (FMT__0));
    ZB_ZCL_COLOR_CONTROL_SEND_STOP_MOVE_STEP_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL);
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
  zb_uint8_t n;
  zb_uint16_t attr_id_start;

  TRACE_MSG(TRACE_ZCL1, ">> read_attr_resp_handler", (FMT__0));

  ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(cmd_buf, read_attr_resp);
  TRACE_MSG(TRACE_ZCL3, "read_attr_resp %p", (FMT__P, read_attr_resp));

  switch (g_test_step)
  {
  case TEST_STEP_1_READ_ATTR:
    TEST_ATTRIBUTE_16_SHARP(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_CAPABILITIES_ZLL_ID, 0x001f);
    break;

  case TEST_STEP_3B_READ_ATTR:
    TEST_ATTRIBUTE_16(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_REMAINING_TIME_ID, 0x0064, 2);
    break;

  case TEST_STEP_3C_READ_ATTR:
    TEST_ATTRIBUTE_16_SHARP(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_REMAINING_TIME_ID, 0x0000);
    break;

  case TEST_STEP_4_READ_ATTR:
    TEST_ATTRIBUTE_16_SHARP(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_X_ID, ZB_ZCL_COLOR_CONTROL_COLOR_X_GREEN);
    if(g_error_cnt==0)
    {
      ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(cmd_buf, read_attr_resp);
      TEST_ATTRIBUTE_16_SHARP(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_Y_ID, ZB_ZCL_COLOR_CONTROL_COLOR_Y_GREEN);
    }
    break;

  case TEST_STEP_5_READ_ATTR:
    TEST_ATTRIBUTE_8(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_MODE_ID, ZB_ZCL_COLOR_CONTROL_COLOR_MODE_CURRENT_X_Y);
    break;

  case TEST_STEP_6_READ_ATTR:
    TEST_ATTRIBUTE_8(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_NUMBER_OF_PRIMARIES_ID, 0x06);
    break;

  case TEST_STEP_6A_READ_ATTR:
  case TEST_STEP_6B_READ_ATTR:
  case TEST_STEP_6C_READ_ATTR:
  case TEST_STEP_6D_READ_ATTR:
  case TEST_STEP_6E_READ_ATTR:
  case TEST_STEP_6F_READ_ATTR:
    n = g_test_step - TEST_STEP_6A_READ_ATTR;
    attr_id_start = (n<3) ? (ZB_ZCL_ATTR_COLOR_CONTROL_PRIMARY_1_X_ID + 4*n) :
                            (ZB_ZCL_ATTR_COLOR_CONTROL_PRIMARY_4_X_ID + 4*(n-3));
    TEST_ATTRIBUTE_16_SHARP(read_attr_resp, attr_id_start, 0);
    if(g_error_cnt==0)
    {
      ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(cmd_buf, read_attr_resp);
      TEST_ATTRIBUTE_16_SHARP(read_attr_resp, (attr_id_start+1), 0);
    }
    if(g_error_cnt==0)
    {
      ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(cmd_buf, read_attr_resp);
      TEST_ATTRIBUTE_8(read_attr_resp, (attr_id_start+2), n+1);
    }
    break;

  case TEST_STEP_7_READ_ATTR:
    TEST_ATTRIBUTE_8(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_ENHANCED_COLOR_MODE_ZLL_ID, ZB_ZCL_COLOR_CONTROL_COLOR_EX_MODE_CURRENT_X_Y);
    break;

  case TEST_STEP_9_READ_ATTR:
    TEST_ATTRIBUTE_8(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_MODE_ID, ZB_ZCL_COLOR_CONTROL_COLOR_MODE_HUE_SATURATION);
    {
      ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(cmd_buf, read_attr_resp);
      TEST_ATTRIBUTE_8(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_ENHANCED_COLOR_MODE_ZLL_ID, ZB_ZCL_COLOR_CONTROL_COLOR_EX_MODE_HUE_SATURATION);
    }
    break;

  case TEST_STEP_19B_READ_ATTR:
    TEST_ATTRIBUTE_8(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_HUE_ID, ZB_ZCL_COLOR_CONTROL_HUE_BLUE);
    break;

  case TEST_STEP_19C_READ_ATTR:
    TEST_ATTRIBUTE_8(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_SATURATION_ID, 0x7f);
    break;

  case TEST_STEP_20B_READ_ATTR:
    TEST_ATTRIBUTE(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_X_ID);
    if(g_error_cnt==0)
    {
      custom_color_x = *(zb_uint16_t*)read_attr_resp->attr_value;

      ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(cmd_buf, read_attr_resp);
      TEST_ATTRIBUTE(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_Y_ID);
    }

    if(g_error_cnt==0)
    {
      custom_color_y = *(zb_uint16_t*)read_attr_resp->attr_value;

      ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(cmd_buf, read_attr_resp);
      TEST_ATTRIBUTE(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_ENHANCED_CURRENT_HUE_ZLL_ID);
    }

    if(g_error_cnt==0)
    {
      custom_ex_hue = *(zb_uint16_t*)read_attr_resp->attr_value;

      ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(cmd_buf, read_attr_resp);
      TEST_ATTRIBUTE(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_SATURATION_ID);
    }

    if(g_error_cnt==0)
    {
      custom_saturation = read_attr_resp->attr_value[0];
      TRACE_MSG(TRACE_ZLL3, "custom_color_x %d custom_color_t %d ex hue %d saturation %d", (FMT__D_D_D_H,
          custom_color_x, custom_color_y, custom_ex_hue, custom_saturation));
    }
    break;

  case TEST_STEP_20E_READ_ATTR:
    TEST_ATTRIBUTE_16_SHARP(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_X_ID, custom_color_x);
    if(g_error_cnt==0)
    {
      ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(cmd_buf, read_attr_resp);
      TEST_ATTRIBUTE_16_SHARP(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_Y_ID, custom_color_y);
    }

    if(g_error_cnt==0)
    {
      ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(cmd_buf, read_attr_resp);
      TEST_ATTRIBUTE_16_SHARP(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_ENHANCED_CURRENT_HUE_ZLL_ID, custom_ex_hue);
    }

    if(g_error_cnt==0)
    {
      ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(cmd_buf, read_attr_resp);
      TEST_ATTRIBUTE_8(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_SATURATION_ID, custom_saturation);
    }
    break;

  case TEST_STEP_27B_READ_ATTR:
    TEST_ATTRIBUTE_16(read_attr_resp, ZB_ZCL_ATTR_COLOR_CONTROL_ENHANCED_CURRENT_HUE_ZLL_ID,
          ZB_ZCL_COLOR_CONTROL_ENHANCED_BLUE /*A900*/, 256);
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
