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
/* PURPOSE: Non color scene controller for ZLL profile
*/

#define ZB_TRACE_FILE_ID 41627
#include "zll/zb_zll_common.h"
#include "test_defs.h"
#include "zb_bdb_internal.h"

#if defined ZB_ENABLE_ZLL && defined ZB_ZLL_DEFINE_DEVICE_DIMMABLE_LIGHT

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
void on_off_read_attr_resp_handler(zb_buf_t * cmd_buf);

/** Test step enumeration. */
enum test_step_e
{
  TEST_STEP_INITIAL,            /**< Initial test pseudo-step (device startup). */
  TEST_STEP_START,              /**< Device start test. */
  TEST_STEP_SEND_ON,            /**< ZED send unicasts a ZCL "on" command to gZR. */
  TEST_STEP_SEND_OFF,           /**< ZED send unicasts a ZCL "off" command to gZR. */
  TEST_STEP_SEND_TOGGLE,        /**< ZED send unicasts a ZCL "toggle" command to gZR. */
  TEST_STEP_SEND_READ_ATTR,     /**< ZED send unicasts a ZCL "read attributes" command for the OnOff attribute to gZR.
                                     OnOff attribute has the value 0x01. */
  TEST_STEP_SEND_OFF2,          /**< ZED send unicasts a ZCL "off" or "toggle" command to gZR. */
  TEST_STEP_SEND_READ_ATTR2,    /**< ZED send unicasts a ZCL "read attributes" command for the OnOff attribute to gZR.
                                     OnOff attribute has the value 0x00. */
  TEST_STEP_SEND_OFF_EFFECT,    /**< ZED send unicasts a ZLL "off with effect" command to gZR, with the "effect identifier" and
                                     "effect variant" fields set to non-reserved values. */
  TEST_STEP_SEND_ON_SCENE,      /**< ZED send unicasts a ZLL "on with recall global scene" command to gZR. */
  TEST_STEP_SEND_ON_TIMED_OFF,  /**< ZED send unicasts a ZLL "on with timed off" command to gZR. */
  TEST_STEP_FINISHED            /**< Test finished pseudo-step. */
};

zb_uint8_t g_test_step = TEST_STEP_INITIAL;

/** Device's extended address. */
zb_ieee_addr_t g_ed_addr = {0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/********************* Declare device **************************/
ZB_ZLL_DECLARE_NON_COLOR_SCENE_CONTROLLER_CLUSTER_LIST(non_color_scene_controller_clusters,
    ZB_ZCL_CLUSTER_MIXED_ROLE);

ZB_ZLL_DECLARE_NON_COLOR_SCENE_CONTROLLER_EP(non_color_scene_controller_ep, ENDPOINT,
    non_color_scene_controller_clusters);

ZB_ZLL_DECLARE_NON_COLOR_SCENE_CONTROLLER_CTX(non_color_scene_controller_ctx,
    non_color_scene_controller_ep);

/** [COMMON_DECLARATION] */

/******************* Declare router parameters *****************/
#define DST_ADDR        0x01
#define DST_ENDPOINT    5
#define DST_ADDR_MODE   ZB_APS_ADDR_MODE_16_ENDP_PRESENT
/******************* Declare test data & constants *************/

zb_short_t g_error_cnt = 0;

#define SEND_READ_ON_OFF_ATTR(buffer)                                                      \
{                                                                                          \
  zb_uint8_t *cmd_ptr;                                                                     \
  ZB_ZCL_GENERAL_INIT_READ_ATTR_REQ(buffer, cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);      \
  ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID);              \
  ZB_ZCL_GENERAL_SEND_READ_ATTR_REQ(                                                       \
      buffer,                                                                              \
      cmd_ptr,                                                                             \
      DST_ADDR,                                                                            \
      DST_ADDR_MODE,                                                                       \
      DST_ENDPOINT,                                                                        \
      ENDPOINT,                                                                            \
      ZB_AF_ZLL_PROFILE_ID,                                                                \
      ZB_ZCL_CLUSTER_ID_ON_OFF,                                                            \
      NULL);                                                                               \
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

  /** [REGISTER] */
  /****************** Register Device ********************************/
  /* Register device list */
  ZB_AF_REGISTER_DEVICE_CTX(&non_color_scene_controller_ctx);

  ZB_AF_SET_ENDPOINT_HANDLER(ENDPOINT, zcl_specific_cluster_cmd_handler);
  /** [REGISTER] */

  ZB_AIB().aps_channel_mask = 1l << MY_CHANNEL;
//  ZB_AIB().aps_channel_mask = ~0;
  ZB_AIB().aps_use_nvram = 1;
  ZB_BDB().bdb_mode = ZB_TRUE;

  if (zb_zdo_start_no_autostart() != RET_OK)
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

/** [HANDLER] */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
  zb_buf_t *zcl_cmd_buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
  zb_zcl_parsed_hdr_t *cmd_info = ZB_GET_BUF_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);
  zb_bool_t cmd_processed           = ZB_FALSE;


  TRACE_MSG(TRACE_ZCL1, "> zcl_specific_cluster_cmd_handler %i", (FMT__H, param));
  ZB_ZCL_DEBUG_DUMP_HEADER(cmd_info);
  TRACE_MSG(TRACE_ZCL1, "payload size: %i", (FMT__D, ZB_BUF_LEN(zcl_cmd_buf)));

  if( cmd_info -> cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI )
  {
    switch( cmd_info -> cluster_id )
    {
      case ZB_ZCL_CLUSTER_ID_ON_OFF:
        if( cmd_info -> is_common_command )
        {
          switch( cmd_info -> cmd_id )
          {
            case ZB_ZCL_CMD_DEFAULT_RESP:
              TRACE_MSG(TRACE_ZCL3, "Got response in cluster 0x%04x",
                        ( FMT__D, cmd_info->cluster_id));
              /* Process default response */
              cmd_processed = ZB_TRUE;
              break;

            case ZB_ZCL_CMD_READ_ATTRIB_RESP:
              on_off_read_attr_resp_handler(zcl_cmd_buf);
              cmd_processed = ZB_TRUE;
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
            default:
              TRACE_MSG(TRACE_ZCL2, "Cluster command %hd, skip it", (FMT__H, cmd_info->cmd_id));
              break;
          }
        }
        break;

      default:
        TRACE_MSG(TRACE_ZCL1, "CLNT role cluster 0x%d is not supported", (FMT__D, cmd_info->cluster_id));
        ++g_error_cnt;
        break;
    }
  }
  else
  {
    /* Command from client to server ZB_ZCL_FRAME_DIRECTION_TO_SRV */
    TRACE_MSG(TRACE_ZCL1, "SRV role, cluster 0x%d is not supported", (FMT__D, cmd_info->cluster_id));
    ++g_error_cnt;
  }

  //if(cmd_processed)
  {
    test_next_step(param);
  }

  TRACE_MSG(TRACE_ZCL1, "<< zcl_specific_cluster_cmd_handler %hd", (FMT__H, cmd_processed));
  return cmd_processed;
}
/** [HANDLER] */

void zb_zdo_startup_complete(zb_uint8_t param)
{
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  switch(sig)
  {
    case ZB_ZDO_SIGNAL_SKIP_STARTUP:
      TRACE_MSG(TRACE_APS1, "Device start skip startup", (FMT__0));
      test_check_start_status(param);
      break;
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      TRACE_MSG(TRACE_APS1, "Device first start", (FMT__0));
      break;
    case ZB_BDB_SIGNAL_TOUCHLINK:
      TRACE_MSG(TRACE_APS1, "Touchlink commissioning as initiator done ok", (FMT__0));
      break;
    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal %hd status %hd", (FMT__H_H, sig, ZB_GET_APP_SIGNAL_STATUS(param)));
      break;
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
  TRACE_MSG(TRACE_ZLL3, "> test_check_start_status param %hd", (FMT__D, param));
/*
  if (ZB_PIBCACHE_CURRENT_CHANNEL() != MY_CHANNEL)
  {
    TRACE_MSG(TRACE_ERROR, "ERROR wrong channel %hd (should be %hd)",
        (FMT__H_H, ZB_PIBCACHE_CURRENT_CHANNEL(), (zb_uint8_t)MY_CHANNEL));
    ++g_error_cnt;
  }
*/
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
/*
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
*/
  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0 && ! g_error_cnt)
  {
    TRACE_MSG(TRACE_ZLL3, "Device STARTED OK", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ZLL3, "ERROR Device start FAILED (errors: %hd)", (FMT__H, g_error_cnt));
  }


  TRACE_MSG(TRACE_ZLL3, "< test_check_start_status", (FMT__0));
}/* void test_check_start_status(zb_uint8_t param) */

void test_next_step(zb_uint8_t param)
{
  zb_buf_t* buffer = ZB_BUF_FROM_REF(param);
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);

  TRACE_MSG(TRACE_ZLL3, "> test_next_step param %hd step %hd", (FMT__H, param, g_test_step));

  switch (g_test_step )
  {
  case TEST_STEP_INITIAL:
    TRACE_MSG(TRACE_ZLL3, "Start commissioning", (FMT__0));
    g_test_step = TEST_STEP_START;
    bdb_start_top_level_commissioning(ZB_BDB_TOUCHLINK_COMMISSIONING);
    if (status != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "ERROR Could not initiate commissioning: status %hd", (FMT__H, status));
      ++g_error_cnt;
    }
    break;

  case TEST_STEP_START:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_ON", (FMT__0));
    g_test_step = TEST_STEP_SEND_ON;
    ZB_ZCL_ON_OFF_SEND_ON_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL);
    break;

  case TEST_STEP_SEND_ON:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_OFF", (FMT__0));
    g_test_step = TEST_STEP_SEND_OFF;
    ZB_ZCL_ON_OFF_SEND_OFF_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL);
    break;

  case TEST_STEP_SEND_OFF:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_TOGGLE", (FMT__0));
    g_test_step = TEST_STEP_SEND_TOGGLE;
    /** [SEND_TOGGLE] */
    ZB_ZCL_ON_OFF_SEND_TOGGLE_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL);
    /** [SEND_TOGGLE] */
    break;

  case TEST_STEP_SEND_TOGGLE:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_READ_ATTR", (FMT__0));
    g_test_step = TEST_STEP_SEND_READ_ATTR;
    SEND_READ_ON_OFF_ATTR(buffer);
    break;

  case TEST_STEP_SEND_READ_ATTR:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_OFF", (FMT__0));
    g_test_step = TEST_STEP_SEND_OFF2;
    ZB_ZCL_ON_OFF_SEND_OFF_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL);
    break;

  case TEST_STEP_SEND_OFF2:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_READ_ATTR", (FMT__0));
    g_test_step = TEST_STEP_SEND_READ_ATTR2;
    SEND_READ_ON_OFF_ATTR(buffer);
    break;

  case TEST_STEP_SEND_READ_ATTR2:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_OFF_EFFECT", (FMT__0));
    g_test_step = TEST_STEP_SEND_OFF_EFFECT;
    ZB_ZCL_ON_OFF_SEND_OFF_WITH_EFFECT_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, 3, 4, NULL);
    break;

  case TEST_STEP_SEND_OFF_EFFECT:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_ON_SCENE", (FMT__0));
    g_test_step = TEST_STEP_SEND_ON_SCENE;
    /** [ON_WITH_RECALL_GLOBAL_SCENE] */
    ZB_ZCL_ON_OFF_SEND_ZB_ZCL_ON_OFF_ON_WITH_RECALL_GLOBAL_SCENE_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL);
    /** [ON_WITH_RECALL_GLOBAL_SCENE] */
    break;

  case TEST_STEP_SEND_ON_SCENE:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_ON_TIMED_OFF", (FMT__0));
    g_test_step = TEST_STEP_SEND_ON_TIMED_OFF;
    ZB_ZCL_ON_OFF_SEND_ON_WITH_TIMED_OFF_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
        ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, ZB_TRUE, 0x1234, 0x4321, NULL);
    break;

  case TEST_STEP_SEND_ON_TIMED_OFF:
    TRACE_MSG(TRACE_ZLL3, "TEST_STEP_FINISH", (FMT__0));
    g_test_step = TEST_STEP_FINISHED;
    break;

  default:
    g_test_step = TEST_STEP_FINISHED;
    TRACE_MSG(TRACE_ERROR, "ERROR step %hd shan't be processed", (FMT__H, g_test_step));
    ++g_error_cnt;
    break;
  }

  if (g_test_step == TEST_STEP_FINISHED /*|| g_error_cnt*/)
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

//  if (g_test_step == TEST_STEP_FINISHED)
//  {
//    zb_free_buf(buffer);
//  }

  TRACE_MSG(TRACE_ZLL3, "< test_next_step. Curr step %hd" , (FMT__H, g_test_step));
}/* void test_next_step(zb_uint8_t param) */

void on_off_read_attr_resp_handler(zb_buf_t * cmd_buf)
{
  zb_zcl_read_attr_res_t * read_attr_resp;

  TRACE_MSG(TRACE_ZCL1, ">> on_off_read_attr_resp_handler", (FMT__0));

  ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(cmd_buf, read_attr_resp);
  TRACE_MSG(TRACE_ZCL3, "read_attr_resp %p", (FMT__P, read_attr_resp));
  if (read_attr_resp)
  {
    if ((ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID != read_attr_resp->attr_id)  ||
        (ZB_ZCL_STATUS_SUCCESS != read_attr_resp->status))
    {
      TRACE_MSG(TRACE_ERROR,
                "ERROR incorrect response: id 0x%04x, status %h",
                (FMT__D_H, read_attr_resp->attr_id, read_attr_resp->status));
      g_error_cnt++;
    }

    switch (g_test_step)
    {
    case TEST_STEP_SEND_READ_ATTR:
      if (read_attr_resp->attr_value[0]!=0x01)
      {
        g_error_cnt++;
        TRACE_MSG(TRACE_ERROR, "ERROR on/off value %hx", (FMT__H, read_attr_resp->attr_value[0]));
      }
      break;

    case TEST_STEP_SEND_READ_ATTR2:
      if (read_attr_resp->attr_value[0]!=0x00)
      {
        g_error_cnt++;
        TRACE_MSG(TRACE_ERROR, "ERROR on/off value %hx", (FMT__H, read_attr_resp->attr_value[0]));
      }
      break;

      default:
        TRACE_MSG(
          TRACE_ERROR,
          "ERROR! Unexpected read attributes response: test step 0x%hx",
          (FMT__D, g_test_step));
        ++g_error_cnt;
        break;
    }
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "ERROR, No info on attribute(s) read", (FMT__0));
    g_error_cnt++;
  }

  TRACE_MSG(TRACE_ZCL1, "<< on_off_read_attr_resp_handler", (FMT__0));
}

#else // defined ZB_ENABLE_ZLL

#include <stdio.h>
int main()
{
  printf(" ZLL is not supported\n");
  return 0;
}

#endif // defined ZB_ENABLE_ZLL
