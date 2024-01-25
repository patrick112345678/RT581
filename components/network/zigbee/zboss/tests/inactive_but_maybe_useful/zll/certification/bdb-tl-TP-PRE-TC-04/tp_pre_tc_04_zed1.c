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

#define ZB_TRACE_FILE_ID 41589
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

/** Test step enumeration. */
enum test_step_e
{
  TEST_STEP_INITIAL,        /**< Initial test pseudo-step (device startup). */
  TEST_STEP_START,          /**< Device start test. */
  TEST_STEP_FINISHED          /**< Test finished pseudo-step. */
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

zb_short_t g_error_cnt = 0;

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
  ZB_AF_REGISTER_DEVICE_CTX(&non_color_scene_controller_ctx);
  ZB_AF_SET_ENDPOINT_HANDLER(ENDPOINT, zcl_specific_cluster_cmd_handler);

  ZB_AIB().aps_channel_mask = 1l << MY_CHANNEL;
  ZB_AIB().aps_use_nvram = 1;
  ZB_BDB().bdb_mode = ZB_TRUE;

  ZG->nwk.nib.security_level = 0;

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

zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
  zb_buf_t *zcl_cmd_buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
  zb_zcl_parsed_hdr_t *cmd_info = ZB_GET_BUF_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);
  zb_bool_t unknown_cmd_received = ZB_FALSE;

  TRACE_MSG(TRACE_ZCL1, "> zcl_specific_cluster_cmd_handler %i", (FMT__H, param));
  ZB_ZCL_DEBUG_DUMP_HEADER(cmd_info);
  TRACE_MSG(TRACE_ZCL1, "payload size: %i", (FMT__D, ZB_BUF_LEN(zcl_cmd_buf)));

  if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_SRV)
  {
    TRACE_MSG(TRACE_ERROR, "ERROR Unsupported direction \"to server\"", (FMT__0));
    ++g_error_cnt;
    unknown_cmd_received = ZB_TRUE;
  }
  else
  {
    /* Command from server to client */
    switch (cmd_info->cluster_id)
    {
    default:
      TRACE_MSG(
          TRACE_ERROR,
          "Cluster 0x%hx is not supported in the test",
          (FMT__H, cmd_info->cluster_id));
      ++g_error_cnt;
      unknown_cmd_received = ZB_TRUE;
      break;
    }
  }

  TRACE_MSG(TRACE_ZCL2, " unknown cmd %hd", (FMT__H, unknown_cmd_received));

  TRACE_MSG(TRACE_ZCL1, "< zcl_specific_cluster_cmd_handler %i", (FMT__0));
  return ! unknown_cmd_received;
}

void zb_zdo_startup_complete(zb_uint8_t param)
{
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);
  TRACE_MSG(TRACE_ZLL3, "> zb_zdo_startup_complete %hd ev %hd", (FMT__H_H, param, sig));

  switch(sig)
  {
    case ZB_ZDO_SIGNAL_SKIP_STARTUP:
      TRACE_MSG(TRACE_APS1, "skip startup", (FMT__0));
//      bdb_start_top_level_commissioning(ZB_BDB_TOUCHLINK_COMMISSIONING);
      break;
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      TRACE_MSG(TRACE_APS1, "Device first start", (FMT__0));
      test_check_start_status(param);
      if (g_error_cnt)
      {
        g_test_step = TEST_STEP_FINISHED;
      }
      break;
    case ZB_BDB_SIGNAL_TOUCHLINK:
      TRACE_MSG(TRACE_APS1, "Touchlink commissioning as initiator done ok", (FMT__0));
//      TRACE_MSG(TRACE_ZLL3, "Device STARTED OK", (FMT__0));
      if (ZB_GET_APP_SIGNAL_STATUS(param) == ZB_ZLL_GENERAL_STATUS_SUCCESS && ! g_error_cnt)
      {
        TRACE_MSG(TRACE_ZLL3, "Device STARTED OK", (FMT__0));
      }
      else
      {
        TRACE_MSG(TRACE_ZLL3, "ERROR Device start FAILED (errors: %hd)", (FMT__H, g_error_cnt));
      }
      break;
    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal %hd status %hd", (FMT__H_H, sig, ZB_GET_APP_SIGNAL_STATUS(param)));
      break;
  }

  /* Minimize test modification - use ZLL alternations for start  */
  switch (ZLL_TRAN_CTX().transaction_task)
  {
    case ZB_ZLL_NO_TASK:
    break;
  case ZB_ZLL_DEVICE_START_TASK:
    break;
  case ZB_ZLL_TRANSACTION_NWK_START_TASK:
    TRACE_MSG(TRACE_ZLL3, "New Network status %hd", (FMT__H, ZB_GET_APP_SIGNAL_STATUS(param)));
    if (ZB_GET_APP_SIGNAL_STATUS(param) != ZB_ZLL_TASK_STATUS_OK)
    {
      TRACE_MSG(TRACE_ERROR, "ERROR status %hd", (FMT__H, ZB_GET_APP_SIGNAL_STATUS(param)));
      ++g_error_cnt;
      g_test_step = TEST_STEP_FINISHED;
    }
    break;

  case ZB_ZLL_TRANSACTION_JOIN_ROUTER_TASK:
    TRACE_MSG(TRACE_ZLL3, "Join Router status %hd", (FMT__H, ZB_GET_APP_SIGNAL_STATUS(param)));
    if (ZB_GET_APP_SIGNAL_STATUS(param) != ZB_ZLL_TASK_STATUS_OK)
    {
      TRACE_MSG(TRACE_ERROR, "ERROR status %hd", (FMT__H, ZB_GET_APP_SIGNAL_STATUS(param)));
      ++g_error_cnt;
      g_test_step = TEST_STEP_FINISHED;
    }
    break;

  default:
    TRACE_MSG(TRACE_ERROR, "ERROR unsupported task %hd", (FMT__H, ZLL_TRAN_CTX().transaction_task));
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
    test_next_step(param);
  }

  TRACE_MSG(TRACE_ZLL3, "< zb_zdo_startup_complete", (FMT__0));
}/* void zll_task_state_changed(zb_uint8_t param) */

void test_check_start_status(zb_uint8_t param)
{
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

#if 0
  if( ZB_ZLL_IS_FACTORY_NEW() )
  {
    if (ZB_PIBCACHE_PAN_ID() != 0xffff)
    {
      TRACE_MSG(TRACE_ERROR, "ERROR PAN Id is 0x%04x (should be 0xffff)",
          (FMT__D, ZB_PIBCACHE_PAN_ID()));
      ++g_error_cnt;
    }
  }
#endif

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
    TRACE_MSG(TRACE_ZLL3, "Finishing test", (FMT__0));
    g_test_step = TEST_STEP_FINISHED;
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
