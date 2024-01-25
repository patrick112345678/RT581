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

#define ZB_TRACE_FILE_ID 41635
#include "zll/zb_zll_common.h"
#include "test_defs.h"

#if defined ZB_ENABLE_ZLL && defined ZB_ZLL_DEFINE_DEVICE_DIMMABLE_LIGHT

#if ! defined ZB_ED_ROLE
#error define ZB_ED_ROLE to compile zed tests
#endif

/** Number of the endpoint device operates on. */
#define ENDPOINT  10

/** ZCL command handler. */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);

/** ZLL task status notification handler. */
void zll_task_state_changed(zb_uint8_t param);

/** Start device parameters checker. */
void test_check_start_status(zb_uint8_t param);

/** Checks device discovery status. */
void test_check_device_discovery(zb_uint8_t param);

/** Checks new network status. */
void test_check_new_network(zb_uint8_t param);

/** Next test step initiator. */
void test_next_step(zb_uint8_t param);

/** Test step enumeration. */
enum test_step_e
{
  TEST_STEP_INITIAL,          /**< Initial test pseudo-step (device startup). */
  TEST_STEP_START_DISCOVERY,  /**< Device discovery start. */
  TEST_STEP_START_NEW_NETWORK,/**< New network start. */
  TEST_STEP_FINISHED          /**< Test finished pseudo-step. */
};

zb_uint8_t g_test_step = TEST_STEP_INITIAL;

/** Device's extended address. */
zb_ieee_addr_t g_ed_addr = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

zb_ieee_addr_t g_empty_addr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

//void identify_read_attr_resp_handler(zb_buf_t *cmd_buf);

//void send_2nd_read_attr_cb(zb_uint8_t param);

/********************* Declare device **************************/
ZB_ZLL_DECLARE_NON_COLOR_SCENE_CONTROLLER_CLUSTER_LIST(non_color_scene_controller_clusters,
    ZB_ZCL_CLUSTER_MIXED_ROLE);

ZB_ZLL_DECLARE_NON_COLOR_SCENE_CONTROLLER_EP(non_color_scene_controller_ep, ENDPOINT,
    non_color_scene_controller_clusters);

ZB_ZLL_DECLARE_NON_COLOR_SCENE_CONTROLLER_CTX(non_color_scene_controller_ctx,
    non_color_scene_controller_ep);

/*! Incorrect time value for test needs */
#define INCORRECT_TIME_515 0xFFFF
/*! Time to identify reported by server */
zb_uint16_t g_reported_time = INCORRECT_TIME_515;

/*! Identifier of the attribute to read */
zb_uint16_t g_attr2read = ZB_ZCL_ATTR_IDENTIFY_IDENTIFY_TIME_ID;

zb_short_t g_error_cnt = 0;

#define DST_ADDR 0
#define DST_ENDPOINT 5
#define DST_ADDR_MODE ZB_APS_ADDR_MODE_16_ENDP_PRESENT
/* Time to identify, see HA test spec, clause 5.15 */
#define TIME_TO_IDENTIFY 0x003c

MAIN()
{
  ARGV_UNUSED;

#ifndef KEIL
  if ( argc < 3 )
  {
    printf("%s <read pipe path> <write pipe path>\n", argv[0]);
    return 0;
  }
#endif

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zed");

  ZB_SET_NIB_SECURITY_LEVEL(0);

  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ed_addr);
  ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);

  zb_set_default_ed_descriptor_values();

  /****************** Register Device ********************************/
  ZB_AF_REGISTER_DEVICE_CTX(&non_color_scene_controller_ctx);
  ZB_AF_SET_ENDPOINT_HANDLER(ENDPOINT, zcl_specific_cluster_cmd_handler);

  ZB_ZLL_REGISTER_COMMISSIONING_CB(zll_task_state_changed);

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
  ZVUNUSED(param);
  TRACE_MSG(TRACE_ZCL1, ">< zb_zdo_startup_complete %h", (FMT__H, param));
  zb_free_buf(ZB_BUF_FROM_REF(param));
}

void zll_task_state_changed(zb_uint8_t param)
{
  zb_buf_t* buffer = ZB_BUF_FROM_REF(param);
  zb_zll_transaction_task_status_t* task_status =
      ZB_GET_BUF_PARAM(buffer, zb_zll_transaction_task_status_t);

  TRACE_MSG(TRACE_ZLL3, "> zll_task_state_changed param %hd status %hd", (FMT__H_H, param, task_status->task));

  switch (task_status->task)
  {
  case ZB_ZLL_DEVICE_START_TASK:
    test_check_start_status(param);
    if (! g_error_cnt)
    {
      test_next_step(param);
    }
    else
    {
      zb_free_buf(ZB_BUF_FROM_REF(param));
    }
    break;

  case ZB_ZLL_DEVICE_DISCOVERY_TASK:
    TRACE_MSG(TRACE_ZLL3, "Device discovery status %hd", (FMT__H, task_status->status));
    if (task_status->status == ZB_ZLL_TASK_STATUS_FINISHED)
    {
      test_check_device_discovery(param);
    }
    else
    {
      ++g_error_cnt;
      g_test_step = TEST_STEP_FINISHED;
      //test_next_step(param);
    }
    test_next_step(param);
    break;

  case ZB_ZLL_TRANSACTION_NWK_START_TASK:
    TRACE_MSG(TRACE_ZLL3, "New Network status %hd", (FMT__H, task_status->status));
    if (task_status->status == ZB_ZLL_TASK_STATUS_OK)
    {
      test_check_new_network(param);
    }
    else
    {
      ++g_error_cnt;
      g_test_step = TEST_STEP_FINISHED;
      //test_next_step(param);
    }
    test_next_step(param);
    break;

  default:
    TRACE_MSG(TRACE_ERROR, "ERROR unsupported task %hd", (FMT__H, task_status->task));
    ++g_error_cnt;
    zb_free_buf(ZB_BUF_FROM_REF(param));
    break;
  }

  TRACE_MSG(TRACE_ZLL3, "< zll_task_state_changed", (FMT__0));
}/* void zll_task_state_changed(zb_uint8_t param) */

void test_check_start_status(zb_uint8_t param)
{
  zb_buf_t* buffer = ZB_BUF_FROM_REF(param);
  zb_zll_transaction_task_status_t* task_status =
      ZB_GET_BUF_PARAM(buffer, zb_zll_transaction_task_status_t);

  TRACE_MSG(TRACE_ZLL3, "> test_check_start_status param %hd", (FMT__D, param));

  if (ZB_PIBCACHE_CURRENT_CHANNEL() != MY_CHANNEL)
  {
    TRACE_MSG(
        TRACE_ERROR,
        "ERROR wrong channel %hd (should be %hd)",
        (FMT__H_H, ZB_PIBCACHE_CURRENT_CHANNEL(), (zb_uint8_t)MY_CHANNEL));
    ++g_error_cnt;
  }

  if (ZB_PIBCACHE_NETWORK_ADDRESS() != 0xffff)
  {
    TRACE_MSG(
        TRACE_ERROR,
        "ERROR Network address is 0x%04x (should be 0xffff)",
        (FMT__D, ZB_PIBCACHE_NETWORK_ADDRESS()));
    ++g_error_cnt;
  }

  if (! ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()))
  {
    TRACE_MSG(TRACE_ERROR, "ERROR Receiver should be turned on", (FMT__0));
    ++g_error_cnt;
  }

  if (! ZB_EXTPANID_IS_ZERO(ZB_NIB_EXT_PAN_ID()))
  {
    TRACE_MSG(
        TRACE_ERROR,
        "ERROR extended PAN Id is not zero: %s",
        (FMT__A, ZB_NIB_EXT_PAN_ID()));
    ++g_error_cnt;
  }

  if (ZB_PIBCACHE_PAN_ID() != 0xffff)
  {
    TRACE_MSG(
        TRACE_ERROR,
        "ERROR PAN Id is 0x%04x (should be 0xffff)",
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

void test_next_step(zb_uint8_t param)
{
  zb_buf_t* buffer = ZB_BUF_FROM_REF(param);
  zb_uint8_t schedule_status;
  zb_int8_t router_index;

  TRACE_MSG(TRACE_ZLL3, "> test_next_step param %hd", (FMT__H, param));

  switch (g_test_step )
  {
  case TEST_STEP_INITIAL:
    TRACE_MSG(TRACE_ZLL3, "Scheduling request START_DISCOVERY", (FMT__0));
    g_test_step = TEST_STEP_START_DISCOVERY;
    ZB_ZLL_START_DEVICE_DISCOVERY(buffer, ZB_FALSE, NULL, schedule_status);
    if (schedule_status != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "ERROR Could not initiate device discovery: schedule status %hd",
          (FMT__H, schedule_status));
      ++g_error_cnt;
      g_test_step = TEST_STEP_FINISHED;
    }
    break;

  case TEST_STEP_START_DISCOVERY:
    TRACE_MSG(TRACE_ZLL3, "Scheduling request START_NEW_NETWORK", (FMT__0));
    g_test_step = TEST_STEP_START_NEW_NETWORK;
    router_index = zll_find_device_info_by_max_rssi(ZB_TRUE);
    if (router_index<0)
    {
      TRACE_MSG(TRACE_ERROR, "ERROR Router not found. Index %hd count %hd", (FMT__H_H,
          router_index, ZLL_TRAN_CTX().n_device_infos));
      ++g_error_cnt;
      g_test_step = TEST_STEP_FINISHED;
      break;
    }
    //ZB_ZLL_START_NEW_NETWORK(buffer, MY_PAN_ID, g_ed_addr, MY_CHANNEL,
    //    ZLL_TRAN_CTX().device_infos[router_index].device_addr, schedule_status);
    ZB_ZLL_START_NEW_NETWORK(buffer, 0, g_empty_addr, 0,
        ZLL_TRAN_CTX().device_infos[router_index].device_addr, schedule_status);
    if (schedule_status != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "ERROR Could not initiate start new network: schedule status %hd",
          (FMT__H, schedule_status));
      ++g_error_cnt;
      g_test_step = TEST_STEP_FINISHED;
    }
    break;

  case TEST_STEP_START_NEW_NETWORK:
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

  TRACE_MSG(TRACE_ZLL3, "< test_next_step", (FMT__0));
}/* void test_next_step(zb_uint8_t param) */

void test_check_device_discovery(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZLL3, ">test_check_device_discovery param %hd", (FMT__H, param));

  TRACE_MSG(TRACE_ZLL3, "< test_check_device_discovery", (FMT__0));
}/* void test_check_device_discovery(zb_uint8_t param) */

void test_check_new_network(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZLL3, ">test_check_new_network param %hd", (FMT__H, param));

  TRACE_MSG(TRACE_ZLL3, "< test_check_new_network", (FMT__0));
}/* void test_check_new_network(zb_uint8_t param) */

#else // defined ZB_ENABLE_HA

#include <stdio.h>
int main()
{
  printf(" HA is not supported\n");
  return 0;
}

#endif // defined ZB_ENABLE_HA
