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
/* PURPOSE: Dimmable light for ZLL profile
*/

#define ZB_TRACE_FILE_ID 41583
#include "zll/zb_zll_common.h"
//#include "../../../../zll/zll_commissioning_internals.h"
#include "test_defs.h"
#include "zb_nvram.h"
//#include "../../../../nwk/nwk_internal.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"

#if defined ZB_ENABLE_ZLL && defined ZB_ZLL_DEFINE_DEVICE_DIMMABLE_LIGHT

#if ! defined ZB_ED_ROLE
#error define ZB_ED_ROLE to compile zed tests
#endif

/** Number of the endpoint device operates on. */
#define ENDPOINT  10
#define DST_ADDR 0x01
#define DST_ENDPOINT 5
#define DST_ADDR_MODE ZB_APS_ADDR_MODE_16_ENDP_PRESENT

/** ZCL command handler. */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);

/** Start device parameters checker. */
void test_check_start_status(zb_uint8_t param);

/** Next test step initiator. */
void test_next_step(zb_uint8_t param);

/** Test step enumeration. */
enum test_step_e
{
  TEST_STEP_INITIAL,        /**< Initial test pseudo-step (device startup). */
  TEST_STEP_START,          /**< Device start test. */
  TEST_STEP_READ_ATTR1,
  TEST_STEP_READ_ATTR2,
  TEST_STEP_FINISHED          /**< Test finished pseudo-step. */
};

zb_uint8_t g_test_step = TEST_STEP_INITIAL;
zb_ext_pan_id_t g_zr1_ext_pan_id;
zb_uint16_t g_zr1_pan_id;
zb_uint8_t g_status;

/*#define SEND_READ_ATTR_SECURITY(buffer, clusterID, attributeID)       \
{                                                                                          \
  zb_uint8_t *cmd_ptr;                                                                     \
  ZB_ZCL_GENERAL_INIT_READ_ATTR_REQ((buffer), cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);    \
  ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, (attributeID));                             \
  ZB_ZCL_FINISH_PACKET(buffer, cmd_ptr);                                                   \
  ZB_ZCL_SEND_COMMAND_SHORT((buffer), DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ENDPOINT,     \
      ZB_AF_ZLL_PROFILE_ID, (clusterID), NULL);                                            \
}
*/
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


/********************* Declare device **************************/
ZB_ZLL_DECLARE_NON_COLOR_SCENE_CONTROLLER_CLUSTER_LIST(non_color_scene_controller_clusters,
    ZB_ZCL_CLUSTER_MIXED_ROLE);

ZB_ZLL_DECLARE_NON_COLOR_SCENE_CONTROLLER_EP(non_color_scene_controller_ep, ENDPOINT,
    non_color_scene_controller_clusters);

ZB_ZLL_DECLARE_NON_COLOR_SCENE_CONTROLLER_CTX(non_color_scene_controller_ctx,
    non_color_scene_controller_ep);

zb_short_t g_error_cnt = 0;


MAIN()
{
  ARGV_UNUSED;

#if !(defined KEIL || defined SDCC || defined ZB_IAR || defined ZB_PLATFORM_LINUX_ARM_2400)
#endif

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zed");


  ZB_SET_NIB_SECURITY_LEVEL(0x05);
/* 04/29/2013 CR:{#6619}: Major
Is NwkAllFresh implemented? See 8 . 7 . 3  Z i g B e e   S e t t i n g s

   04/30/2013 CR: I found this NIB attribute in zb_nwk_nib.h, but not anywhere
   else. In zb_nwk_globals.h written that
     all_fresh is always 0 for Standard security
*/


/* 04/29/2013 CR:{#6619}: Major
Please implement standalone tests for security processing:
1. Devkey: take sample from 8 . 7 . 4
2-3. Annex A:  ZLL security test  vectors

  04/30/2013 CR: Im not sure about transaction id and response id. Now it takes
  from scan req and scan res really. Should i predefine its values as written in spec?

  Standalone tests - you mean create 2 custom tests with devkey and AnnexA vectors using?
*/


/* 04/29/2013 CR:{#6619}: Major
Why using device context for storing keys? Not sure it's wrong, just wondering.

   04/29/2013 We discussed it with Michail. Main reason - its ZLL-specific,
   and we need to get access to it from zll_secur.c.
   Also key_info and key_index stored there.
*/
  ZB_MEMCPY(ZLL_DEVICE_INFO().master_key, g_master_key, ZB_CCM_KEY_SIZE);
  ZB_MEMCPY(ZLL_DEVICE_INFO().certification_key, g_certification_key, ZB_CCM_KEY_SIZE);
  ZB_MEMCPY(ZLL_DEVICE_INFO().development_key, g_development_key, ZB_CCM_KEY_SIZE);
  ZB_MEMCPY(ZLL_DEVICE_INFO().nwk_key, g_key, ZB_CCM_KEY_SIZE);
  ZLL_DEVICE_INFO().key_index = ZB_ZLL_MASTER_KEY_INDEX;
  ZLL_DEVICE_INFO().key_info = ZB_ZLL_MASTER_KEY | ZB_ZLL_CERTIFICATION_KEY | ZB_ZLL_DEVELOPMENT_KEY;

  zb_secur_setup_preconfigured_key(g_key, 0);
  zb_secur_update_key_pair(g_zr_addr, g_key_link, 1);
  zb_secur_update_key_pair(g_ed_addr, g_key_link, 1);
  ZB_NIB().secure_all_frames = ZB_TRUE;
  ZG->aps.authenticated = ZB_TRUE;

  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ed_addr);

  ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);

  zb_set_default_ed_descriptor_values();

  /****************** Register Device ********************************/
  ZB_AF_REGISTER_DEVICE_CTX(&non_color_scene_controller_ctx);
  ZB_AF_SET_ENDPOINT_HANDLER(ENDPOINT, zcl_specific_cluster_cmd_handler);
#if 0
  ZB_AIB().aps_channel_mask = 1l << MY_CHANNEL;
#endif
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

void read_attr_resp_handler(zb_buf_t * cmd_buf)
{
  zb_zcl_read_attr_res_t * read_attr_resp;

  TRACE_MSG(TRACE_ZCL1, ">> read_attr_resp_handler", (FMT__0));

  ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(cmd_buf, read_attr_resp);
  TRACE_MSG(TRACE_ZCL3, "read_attr_resp %p", (FMT__P, read_attr_resp));

  switch (g_test_step)
  {
    case TEST_STEP_READ_ATTR1:
      TRACE_MSG(TRACE_ZCL3, "ERROR Invalid response", (FMT__0));
      g_error_cnt++;
      break;

    case TEST_STEP_READ_ATTR2:
      ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(cmd_buf, read_attr_resp);
      TRACE_MSG(TRACE_ZCL3, "ZCL version is %i", (FMT__H, read_attr_resp->attr_value));
      break;

    default:
      g_error_cnt++;
  }
}

zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
  zb_buf_t *zcl_cmd_buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
  zb_zcl_parsed_hdr_t *cmd_info = ZB_GET_BUF_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);
  zb_bool_t cmd_is_processed = ZB_FALSE;

  TRACE_MSG(TRACE_ZCL1, "> zcl_specific_cluster_cmd_handler %i", (FMT__H, param));
  ZB_ZCL_DEBUG_DUMP_HEADER(cmd_info);
  TRACE_MSG(TRACE_ZCL1, "payload size: %i", (FMT__D, ZB_BUF_LEN(zcl_cmd_buf)));

  if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_SRV)
  {
    TRACE_MSG(TRACE_ERROR, "ERROR Unsupported direction \"to server\"", (FMT__0));
    ++g_error_cnt;
  }
  else
  {
    /* Command from server to client */
    switch (cmd_info->cluster_id)
    {
      case ZB_ZCL_CLUSTER_ID_BASIC:
        if( cmd_info -> is_common_command )
        {
          switch( cmd_info -> cmd_id )
          {
            case ZB_ZCL_CMD_READ_ATTRIB_RESP:
              read_attr_resp_handler(zcl_cmd_buf);
              cmd_is_processed = ZB_TRUE;
              break;

            default:
              TRACE_MSG(TRACE_ZCL2, "Skip general command %hd", (FMT__H, cmd_info->cmd_id));
              break;
          }
        }
        break;

      default:
        TRACE_MSG(
          TRACE_ERROR,
          "Cluster 0x%hx is not supported in the test",
          (FMT__H, cmd_info->cluster_id));
        ++g_error_cnt;
      break;
    }
  }

  if (cmd_is_processed || g_error_cnt!=0)
  {
    test_next_step(param);
  }
  TRACE_MSG(TRACE_ZCL1, "< zcl_specific_cluster_cmd_handler %i", (FMT__H, cmd_is_processed));
  return cmd_is_processed;
}

void zb_zdo_startup_complete(zb_uint8_t param)
{
  zb_buf_t* buffer = ZB_BUF_FROM_REF(param);
  zb_zll_transaction_task_status_t* task_status =
      ZB_GET_BUF_PARAM(buffer, zb_zll_transaction_task_status_t);
  zb_bool_t next_step = ZB_TRUE;

  TRACE_MSG(TRACE_ZLL3, "> zb_zdo_startup_complete %hd status %hd", (FMT__H_H, param, task_status->task));

  switch (task_status->task)
  {
    case ZB_ZLL_DEVICE_START_TASK:
      test_check_start_status(param);
      if (g_error_cnt)
      {
        g_test_step = TEST_STEP_FINISHED;
      }
      break;

    case ZB_ZLL_TRANSACTION_NWK_START_TASK:
    case ZB_ZLL_TRANSACTION_JOIN_ROUTER_TASK:
    case ZB_ZLL_TRANSACTION_JOIN_ED_TASK:
      TRACE_MSG(TRACE_ZLL3, "New Network status %hd", (FMT__H, task_status->status));
      if (task_status->status != ZB_ZLL_TASK_STATUS_OK)
      {
        TRACE_MSG(TRACE_ERROR, "ERROR status %hd", (FMT__H, task_status->status));
        ++g_error_cnt;
        g_test_step = TEST_STEP_FINISHED;
      }
      break;

    case ZB_ZLL_DEVICE_DISCOVERY_TASK:
      TRACE_MSG(TRACE_ZLL3, "Device discovery status %i", (FMT__H, task_status->status));
      if (task_status->status == ZB_ZLL_TASK_STATUS_FINISHED)
      {
        zll_add_device_to_network(param);
        next_step = ZB_FALSE;
      }
      break;

    default:
      TRACE_MSG(TRACE_ERROR, "ERROR unsupported task %hd", (FMT__H, task_status->task));
      ++g_error_cnt;
      g_test_step = TEST_STEP_FINISHED;
      break;
  }

  if (g_test_step == TEST_STEP_FINISHED)
  {
    zb_free_buf(ZB_BUF_FROM_REF(param));
  }
  else
  {
    if (next_step)
    {
      test_next_step(param);
    }
  }

  TRACE_MSG(TRACE_ZLL3, "< zb_zdo_sartup_complete", (FMT__0));
}/* void zll_task_state_changed(zb_uint8_t param) */

void test_check_start_status(zb_uint8_t param)
{
  zb_buf_t* buffer = ZB_BUF_FROM_REF(param);
  zb_zll_transaction_task_status_t* task_status =
      ZB_GET_BUF_PARAM(buffer, zb_zll_transaction_task_status_t);

  TRACE_MSG(TRACE_ZLL3, "> test_check_start_status param %hd", (FMT__D, param));

  /* Note: ZB_PIBCACHE_NETWORK_ADDRESS() may be != 0xffff -
   * NVRAM save ZB_PIBCACHE_NETWORK_ADDRESS()!
  */
  if(ZB_ZLL_IS_FACTORY_NEW())
  {
#if 0
    if (ZB_PIBCACHE_CURRENT_CHANNEL() != MY_CHANNEL)
    {
      TRACE_MSG(TRACE_ERROR, "ERROR wrong channel %hd (should be %hd)",
                (FMT__H_H, ZB_PIBCACHE_CURRENT_CHANNEL(), (zb_uint8_t)MY_CHANNEL));
      ++g_error_cnt;
    }
#endif
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

  if(ZB_ZLL_IS_FACTORY_NEW())
  {
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

void alarm_cb(zb_uint8_t param)
{
  ZVUNUSED(param);
  TRACE_MSG(TRACE_ZLL3, "> alarm_cb", (FMT__0));
  ZB_GET_OUT_BUF_DELAYED(test_next_step);
  TRACE_MSG(TRACE_ZLL3, "< alarm_cb", (FMT__0));
}

void set_security_level_cb(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZLL3, "Set security_level to %i", (FMT__H, param));
  ZB_SET_NIB_SECURITY_LEVEL(param);
}

void test_next_step(zb_uint8_t param)
{
  zb_buf_t* buffer = ZB_BUF_FROM_REF(param);
  zb_uint8_t status;

  TRACE_MSG(TRACE_ZLL3, "> test_next_step param %hd step %hd", (FMT__H, param, g_test_step));

  if (g_error_cnt)
  {
    g_test_step = TEST_STEP_FINISHED;
  }

  switch (g_test_step )
  {
    case TEST_STEP_INITIAL:
      TRACE_MSG(TRACE_ZLL3, "Start commissioning", (FMT__0));
      g_test_step = TEST_STEP_START;
      status = zb_zll_start_commissioning(param);
      if (status != RET_OK)
      {
        TRACE_MSG(TRACE_ERROR, "ERROR Could not initiate commissioning: status %hd", (FMT__H, status));
        ++g_error_cnt;
      }

      break;

    case TEST_STEP_START:
      TRACE_MSG(TRACE_ZLL3, "ZDO read attributes request (4a)", (FMT__0));
      TRACE_MSG(TRACE_ZLL3, "Set security_level to 0", (FMT__0));
      ZB_SET_NIB_SECURITY_LEVEL(0);
      SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_BASIC, ZB_ZCL_ATTR_BASIC_ZCL_VERSION_ID);
      g_test_step = TEST_STEP_READ_ATTR1;
      ZB_SCHEDULE_ALARM(alarm_cb, 0, ZB_TIME_ONE_SECOND*5);
      break;

    case TEST_STEP_READ_ATTR1:
      TRACE_MSG(TRACE_ZLL3, "ZDO read attributes request (5a)", (FMT__0));
      TRACE_MSG(TRACE_ZLL3, "Set security_level to 0x05", (FMT__0));
      ZB_SET_NIB_SECURITY_LEVEL(0x05);
      SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_BASIC, ZB_ZCL_ATTR_BASIC_ZCL_VERSION_ID);
      g_test_step = TEST_STEP_READ_ATTR2;
      break;

    case TEST_STEP_READ_ATTR2:
      g_test_step = TEST_STEP_FINISHED;
      break;

    case TEST_STEP_FINISHED:
      break;

    default:
      g_test_step = TEST_STEP_FINISHED;
      TRACE_MSG(TRACE_ERROR, "ERROR step %hd shan't be processed", (FMT__H, g_test_step));
      ++g_error_cnt;
      break;
  }

  if (g_test_step == TEST_STEP_FINISHED)
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

#else // defined ZB_ENABLE_ZLL

#include <stdio.h>
int main()
{
  printf(" ZLL is not supported\n");
  return 0;
}

#endif // defined ZB_ENABLE_ZLL
