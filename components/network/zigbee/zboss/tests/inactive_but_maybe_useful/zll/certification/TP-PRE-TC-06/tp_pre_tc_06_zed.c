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

#define ZB_TRACE_FILE_ID 41607
#include "zll/zb_zll_common.h"
#include "zll/zb_zll_nwk_features.h"
#include "zb_zdo.h"
#include "test_defs.h"
#include "zb_nvram.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
//#include "../../../../zll/zll_commissioning_internals.h"
//#include "../../../../nwk/nwk_internal.h"
#include "zb_nwk.h"

#if defined ZB_ENABLE_ZLL && defined ZB_ZLL_DEFINE_DEVICE_DIMMABLE_LIGHT

#if ! defined ZB_ED_ROLE
#error define ZB_ED_ROLE to compile zed tests
#endif

/* ************************************************************************** */

/** Number of the endpoint device operates on. */
#define ENDPOINT  10

/** ZCL command handler. */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);
/** Start device parameters checker. */
zb_bool_t test_check_start_status(zb_uint8_t param, zb_bool_t is_fn);
/** Next test step initiator. */
void test_next_step(zb_uint8_t param);

/* ************************************************************************** */

/** Test step enumeration. */
enum test_step_e
{
  TEST_STEP_START,

  TEST_STEP_ZR1_TOUCHLINK,
  TEST_STEP_ZR1_SAVE_INFO,
  TEST_STEP_ZR1_CHANNEL_CHANGE,
  TEST_STEP_ZR1_REJOIN_COMPLETE,
  TEST_STEP_WAIT_1,

  TEST_STEP_ZR2_TOUCHLINK,
  TEST_STEP_ZR2_SAVE_INFO,
  TEST_STEP_ZR2_CHANNEL_CHANGE,
  TEST_STEP_ZR2_REJOIN_COMPLETE,
  TEST_STEP_WAIT_2,

  TEST_STEP_ZR1_TOUCHLINK_1,
  TEST_STEP_ZR1_TOUCHLINK_2,

  TEST_STEP_FINISHED
};

zb_uint8_t g_test_step = TEST_STEP_START;
zb_uint8_t g_status;
zb_uint8_t chan_for_nwk_up;

/* test data */
zb_ext_pan_id_t g_zr1_ext_pan_id;
zb_ext_pan_id_t g_zr2_ext_pan_id;
zb_ext_pan_id_t g_tmp_ext_pan_id;

zb_uint16_t g_zr1_pan_id;
zb_uint16_t g_zr2_pan_id;
zb_uint16_t g_tmp_pan_id;

/* ************************************************************************** */

/** Device's extended address. */
zb_ieee_addr_t g_ed_addr = {0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/** security keys. */
zb_uint8_t g_key[16]               = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
zb_uint8_t g_master_key[16]        = { 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0, 0, 0, 0, 0, 0, 0, 0xff};
zb_uint8_t g_certification_key[16] = { 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf};
zb_uint8_t g_development_key[16]   = { 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0, 0, 0, 0, 0, 0, 0xff, 0xff};
zb_uint8_t g_key_link[16]          = { 0x12, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33};

/********************* Declare device **************************/
ZB_ZLL_DECLARE_NON_COLOR_SCENE_CONTROLLER_CLUSTER_LIST(non_color_scene_controller_clusters, ZB_ZCL_CLUSTER_MIXED_ROLE);
ZB_ZLL_DECLARE_NON_COLOR_SCENE_CONTROLLER_EP(non_color_scene_controller_ep, ENDPOINT, non_color_scene_controller_clusters);
ZB_ZLL_DECLARE_NON_COLOR_SCENE_CONTROLLER_CTX(non_color_scene_controller_ctx, non_color_scene_controller_ep);

/* ************************************************************************** */

MAIN()
{
  ARGV_UNUSED;

#if !(defined KEIL || defined SDCC || defined ZB_IAR || defined ZB_PLATFORM_LINUX_ARM_2400)
#endif

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zed");


  ZB_SET_NIB_SECURITY_LEVEL(0x05);
  ZB_MEMCPY(ZLL_DEVICE_INFO().master_key, g_master_key, ZB_CCM_KEY_SIZE);
  ZB_MEMCPY(ZLL_DEVICE_INFO().certification_key, g_certification_key, ZB_CCM_KEY_SIZE);
  ZB_MEMCPY(ZLL_DEVICE_INFO().development_key, g_development_key, ZB_CCM_KEY_SIZE);
  ZB_MEMCPY(ZLL_DEVICE_INFO().nwk_key, g_key, ZB_CCM_KEY_SIZE);
  ZLL_DEVICE_INFO().key_index = ZB_ZLL_CERTIFICATION_KEY_INDEX;
  ZLL_DEVICE_INFO().key_info = ZB_ZLL_CERTIFICATION_KEY;

  zb_secur_setup_preconfigured_key(g_key, 0);
  ZB_NIB().secure_all_frames = ZB_TRUE;
  ZG->aps.authenticated = ZB_TRUE;

#if 0
  ZB_AIB().aps_use_nvram = 0;
  ZB_AIB().aps_nvram_erase_at_start = 1;
#endif

  ZLL_DEVICE_INFO().zll_info = (ZLL_DEVICE_INFO().zll_info | ZB_ZLL_INFO_TOUCHLINK_INITIATOR);

  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ed_addr);
  ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);

  zb_set_default_ed_descriptor_values();

  /****************** Register Device ********************************/
  ZB_AF_REGISTER_DEVICE_CTX(&non_color_scene_controller_ctx);
  ZB_AF_SET_ENDPOINT_HANDLER(ENDPOINT, zcl_specific_cluster_cmd_handler);

  //ZB_AIB().aps_channel_mask = 1l << MY_CHANNEL;
  ZB_AIB().aps_channel_mask = (1l<<11)|(1l<<15)|(1l<<20)|(1l<<25);

  if (zb_zll_dev_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "###ERROR zdo_dev_start failed", (FMT__0));
  }
  else
  {
    zcl_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}

/* ************************************************************************** */

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
    unknown_cmd_received = ZB_TRUE;
  }
  else
  {
    /* Command from server to client */
    switch (cmd_info->cluster_id)
    {
      default:
        TRACE_MSG(TRACE_ERROR, "Cluster 0x%hx is not supported in the test", (FMT__H, cmd_info->cluster_id));
        unknown_cmd_received = ZB_TRUE;
      break;
    }
  }

  TRACE_MSG(TRACE_ZCL2, " unknown cmd %hd", (FMT__H, unknown_cmd_received));

  TRACE_MSG(TRACE_ZCL1, "< zcl_specific_cluster_cmd_handler %i", (FMT__0));
  return !unknown_cmd_received;
}

/* ************************************************************************** */

/* Note:
... the source PAN ID field shall be set to any value in the range
0x0001 � 0xfffe, if the device is factory new,
or the PAN identifier of the device, otherwise
*/
zb_bool_t test_check_start_status(zb_uint8_t param, zb_bool_t is_fn)
{
  zb_bool_t ret = ZB_TRUE;
  zb_buf_t* buffer = ZB_BUF_FROM_REF(param);
  zb_zll_transaction_task_status_t* task_status = ZB_GET_BUF_PARAM(buffer, zb_zll_transaction_task_status_t);

  TRACE_MSG(TRACE_ZLL3, "> test_check_start_status param %hd", (FMT__D, param));

  if (is_fn != ZB_ZLL_IS_FACTORY_NEW())
  {
    TRACE_MSG(TRACE_ERROR, "ERROR Incorrect FN flag", (FMT__0));
    ret = ZB_FALSE;
  }
  if (task_status->status != ZB_ZLL_GENERAL_STATUS_SUCCESS)
  {
    TRACE_MSG(TRACE_ZLL3, "ERROR Device start FAILED (status)", (FMT__0));
    ret = ZB_FALSE;
  }
  if (!ZB_PIBCACHE_RX_ON_WHEN_IDLE())
  {
    TRACE_MSG(TRACE_ERROR, "ERROR Receiver should be turned on", (FMT__0));
    ret = ZB_FALSE;
  }
  if (ZB_PIBCACHE_PAN_ID() == 0x0000 || ZB_PIBCACHE_PAN_ID() > 0xFFFE)
  {
    TRACE_MSG(TRACE_ERROR, "ERROR PAN Id is 0x%04x (should be 0x0001 � 0xFFFE)", (FMT__D, ZB_PIBCACHE_PAN_ID()));
    ret = ZB_FALSE;
  }

  if (ZB_ZLL_IS_FACTORY_NEW())
  {
#if 0
    if (ZB_PIBCACHE_CURRENT_CHANNEL() != MY_CHANNEL)
    {
      TRACE_MSG(TRACE_ERROR, "ERROR wrong channel %hd (should be %hd)", (FMT__H_H, ZB_PIBCACHE_CURRENT_CHANNEL(), (zb_uint8_t)MY_CHANNEL));
      ret = ZB_FALSE;
    }
#endif
    if (ZB_PIBCACHE_NETWORK_ADDRESS() != 0xffff)
    {
      TRACE_MSG(TRACE_ERROR, "ERROR Network address is 0x%04x (should be 0xffff)", (FMT__D, ZB_PIBCACHE_NETWORK_ADDRESS()));
      ret = ZB_FALSE;
    }
    if (!ZB_EXTPANID_IS_ZERO(ZB_NIB_EXT_PAN_ID()))
    {
      TRACE_MSG(TRACE_ERROR, "ERROR extended PAN Id is not zero: %s", (FMT__A, ZB_NIB_EXT_PAN_ID()));
      ret = ZB_FALSE;
    }
  }
  else
  {
    if (ZB_PIBCACHE_NETWORK_ADDRESS() == 0xffff)
    {
      TRACE_MSG(TRACE_ERROR, "ERROR Network address is 0x%04x (should be 0xffff)", (FMT__D, ZB_PIBCACHE_NETWORK_ADDRESS()));
      ret = ZB_FALSE;
    }
    /* if device isn'r fn, extpanid != 0 (fixit)*/
#if 0
    if (ZB_EXTPANID_IS_ZERO(ZB_NIB_EXT_PAN_ID()))
    {
      TRACE_MSG(TRACE_ERROR, "ERROR extended PAN Id is zero: %s", (FMT__A, ZB_NIB_EXT_PAN_ID()));
      ret = ZB_FALSE;
    }
#endif
  }

  if (ret)
  {
    TRACE_MSG(TRACE_ZLL3, "Device STARTED OK", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ZLL3, "ERROR Device started with errors", (FMT__0));
  }

  TRACE_MSG(TRACE_ZLL3, "< test_check_start_status", (FMT__0));

  return ret;
}

/* ************************************************************************** */

void zb_zdo_startup_complete(zb_uint8_t param)
{
  zb_buf_t* buffer = ZB_BUF_FROM_REF(param);
  zb_zll_transaction_task_status_t *task_status = ZB_GET_BUF_PARAM(buffer, zb_zll_transaction_task_status_t);
  zb_bool_t need_free_buf = ZB_TRUE;
  zb_bool_t is_fn = (zb_bool_t)ZB_ZLL_IS_FACTORY_NEW();

  TRACE_MSG(TRACE_ZLL3, "> zb_zdo_startup_complete %hd status %hd", (FMT__H_H, param, task_status->task));

  switch (task_status->task)
  {
    case ZB_ZLL_DEVICE_START_TASK:
      TRACE_MSG(TRACE_ZLL3, "###ZB_ZLL_DEVICE_START_TASK", (FMT__0));
      if (test_check_start_status(param, is_fn))
      {
        TRACE_MSG(TRACE_ZLL3, "###ZLL startup OK", (FMT__0));

        need_free_buf = ZB_FALSE;
        ZB_SCHEDULE_CALLBACK(test_next_step, param);
      }
      else
      {
        TRACE_MSG(TRACE_ZLL3, "###ZLL startup FAILED (stop test procedure)", (FMT__0));
      }
      break;

    case ZB_ZLL_START_COMMISSIONING:
      TRACE_MSG(TRACE_ZLL3, "###ZB_ZLL_START_COMMISSIONING", (FMT__0));
      break;

    case ZB_ZLL_TRANSACTION_NWK_START_TASK:
    case ZB_ZLL_TRANSACTION_JOIN_ROUTER_TASK:
    case ZB_ZLL_TRANSACTION_JOIN_ED_TASK:
      TRACE_MSG(TRACE_ZLL3, "###START_NETWORK", (FMT__0));
      if (task_status->status == ZB_ZLL_TASK_STATUS_OK)
      {
        TRACE_MSG(TRACE_ZLL3, "status = ZB_ZLL_TASK_STATUS_OK", (FMT__0));
        need_free_buf = ZB_FALSE;
        ZB_SCHEDULE_CALLBACK(test_next_step, param);
      }
      else if (task_status->status == ZB_ZLL_TASK_STATUS_NETWORK_UPDATED)
      {
        TRACE_MSG(TRACE_ZLL3, "status = ZB_ZLL_TASK_STATUS_NETWORK_UPDATED", (FMT__0));
        need_free_buf = ZB_FALSE;
        ZB_SCHEDULE_CALLBACK(test_next_step, param);
      }
      else
      {
        TRACE_MSG(TRACE_ZLL3, "status FAILED (stop test procedure)", (FMT__0));
      }
      break;

    case ZB_ZLL_DEVICE_DISCOVERY_TASK:
      TRACE_MSG(TRACE_ZLL3, "###ZB_ZLL_DEVICE_DISCOVERY_TASK", (FMT__0));
      break;

    default:
      TRACE_MSG(TRACE_ERROR, "###ERROR unsupported task", (FMT__0));
      break;
  }

  if (need_free_buf)
  {
    zb_free_buf(ZB_BUF_FROM_REF(param));
  }

  TRACE_MSG(TRACE_ZLL3, "< zb_zdo_sartup_complete", (FMT__0));
}

/* ************************************************************************** */

void test_initiate_commissioning(zb_uint8_t param)
{
  zb_uint8_t status = zb_zll_start_commissioning(param);
  if (status != RET_OK)
  {
    TRACE_MSG(TRACE_ZLL3, "###ERROR Could not initiate commissioning: status %hd", (FMT__H, status));
  }
}

/* ************************************************************************** */

zb_uint8_t get_chan_number_for_nwk_update()
{
  zb_uint8_t ret;
  zb_uint8_t primary_chans[] = ZB_ZLL_PRIMARY_CHANNELS;
  zb_uint8_t i = 0;
  for (i = 0; i < sizeof(primary_chans)/sizeof(primary_chans[0]); ++i)
  {
    if (primary_chans[i] != ZB_PIBCACHE_CURRENT_CHANNEL())
    {
      ret = primary_chans[i];
      break;
    }
  }
  TRACE_MSG(TRACE_ZLL3, "get_chan_number_for_nwk_update: channel = %hd;", (FMT__H, ret));
  return ret;
}

/* ************************************************************************** */

void test_next_step(zb_uint8_t param)
{
  zb_buf_t* buffer = ZB_BUF_FROM_REF(param);
  TRACE_MSG(TRACE_ZLL3, "> test_next_step param %hd step %hd", (FMT__H, param, g_test_step));

  switch (g_test_step )
  {
  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  case TEST_STEP_START:
    TRACE_MSG(TRACE_ZLL3, "###TEST_STEP_START", (FMT__0));
    g_test_step = TEST_STEP_ZR1_TOUCHLINK;
    ZB_SCHEDULE_CALLBACK(test_next_step, param);
    break;

  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  case TEST_STEP_ZR1_TOUCHLINK:
    TRACE_MSG(TRACE_ZLL3, "###TEST_STEP_ZR1_TOUCHLINK", (FMT__0));
    if (ZB_ZLL_IS_FACTORY_NEW())
    {
      g_test_step = TEST_STEP_FINISHED;
    }
    else
    {
      /* 1st phase of test, when device is fnd (P1-P3) */
      g_test_step = TEST_STEP_ZR1_SAVE_INFO;
      /* NOTE: device must automatically connect to zr1
      (but in application startup callback isn't called
      after connecting to previous network, so start touchlink again)
      */
      ZB_SCHEDULE_CALLBACK(test_initiate_commissioning, param);
    }
    break;
  case TEST_STEP_ZR1_SAVE_INFO:
    TRACE_MSG(TRACE_ZLL3, "###TEST_STEP_ZR1_SAVE_INFO", (FMT__0));
    ZB_EXTPANID_COPY(g_zr1_ext_pan_id, ZB_NIB_EXT_PAN_ID());
    g_zr1_pan_id = ZB_PIBCACHE_PAN_ID();
    g_test_step = TEST_STEP_ZR1_CHANNEL_CHANGE;
    ZB_SCHEDULE_ALARM(test_next_step, param, 3*ZB_TIME_ONE_SECOND);
    break;
  case TEST_STEP_ZR1_CHANNEL_CHANGE:
    TRACE_MSG(TRACE_ZLL3, "###TEST_STEP_ZR1_CHANNEL_CHANGE", (FMT__0));
    g_test_step = TEST_STEP_ZR1_REJOIN_COMPLETE;
    chan_for_nwk_up = get_chan_number_for_nwk_update();
    ZB_ZLL_NWK_UPDATE_SEND_CHANGE_CHANNEL_REQ(buffer, 1l<<chan_for_nwk_up);
    break;
  case TEST_STEP_ZR1_REJOIN_COMPLETE:
    TRACE_MSG(TRACE_ZLL3, "###TEST_STEP_ZR1_REJOIN_COMPLETE", (FMT__0));
    /* TODO: compare zr1 data with previously saved info */
    g_tmp_pan_id = ZB_PIBCACHE_PAN_ID();
    ZB_EXTPANID_COPY(g_tmp_ext_pan_id, ZB_NIB_EXT_PAN_ID());
    g_test_step = TEST_STEP_WAIT_1;
    ZB_SCHEDULE_ALARM(test_next_step, param, 40*ZB_TIME_ONE_SECOND);
    /* Wait for turning off ZR1 and turning on ZR2 */
    break;
  case TEST_STEP_WAIT_1:
    g_test_step = TEST_STEP_ZR2_TOUCHLINK;
    ZB_SCHEDULE_CALLBACK(test_next_step, param);
    TRACE_MSG(TRACE_ZLL3, "###TEST_STEP_WAIT_1", (FMT__0));
    break;

  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  case TEST_STEP_ZR2_TOUCHLINK:
    TRACE_MSG(TRACE_ZLL3, "###TEST_STEP_ZR2_TOUCHLINK", (FMT__0));
    g_test_step = TEST_STEP_ZR2_SAVE_INFO;
    ZB_SCHEDULE_CALLBACK(test_initiate_commissioning, param);
    break;
  case TEST_STEP_ZR2_SAVE_INFO:
    TRACE_MSG(TRACE_ZLL3, "###TEST_STEP_ZR2_SAVE_INFO", (FMT__0));
    ZB_EXTPANID_COPY(g_zr2_ext_pan_id, ZB_NIB_EXT_PAN_ID());
    g_zr2_pan_id = ZB_PIBCACHE_PAN_ID();
    g_test_step = TEST_STEP_ZR2_CHANNEL_CHANGE;
    ZB_SCHEDULE_ALARM(test_next_step, param, 3*ZB_TIME_ONE_SECOND);
    break;
  case TEST_STEP_ZR2_CHANNEL_CHANGE:
    TRACE_MSG(TRACE_ZLL3, "###TEST_STEP_ZR2_CHANNEL_CHANGE", (FMT__0));
    g_test_step = TEST_STEP_ZR2_REJOIN_COMPLETE;
    ZB_ZLL_NWK_UPDATE_SEND_CHANGE_CHANNEL_REQ(buffer, 1l<<chan_for_nwk_up);
    break;
  case TEST_STEP_ZR2_REJOIN_COMPLETE:
    TRACE_MSG(TRACE_ZLL3, "###TEST_STEP_ZR2_REJOIN_COMPLETE", (FMT__0));
    /* TODO: compare zr2 data with previously saved info */
    g_tmp_pan_id = ZB_PIBCACHE_PAN_ID();
    ZB_EXTPANID_COPY(g_tmp_ext_pan_id, ZB_NIB_EXT_PAN_ID());
    g_test_step = TEST_STEP_WAIT_2;
    ZB_SCHEDULE_ALARM(test_next_step, param, 40*ZB_TIME_ONE_SECOND);
    /* Wait for turning off ZR2 and turning on ZR1 */
    break;
  case TEST_STEP_WAIT_2:
    g_test_step = TEST_STEP_ZR1_TOUCHLINK_1;
    ZB_SCHEDULE_CALLBACK(test_next_step, param);
    TRACE_MSG(TRACE_ZLL3, "###TEST_STEP_WAIT_2", (FMT__0));
    break;

  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  case TEST_STEP_ZR1_TOUCHLINK_1:
    TRACE_MSG(TRACE_ZLL3, "###TEST_STEP_ZR1_TOUCHLINK_1", (FMT__0));
    g_test_step = TEST_STEP_ZR1_TOUCHLINK_2;
    ZB_SCHEDULE_CALLBACK(test_initiate_commissioning, param);
    break;
  case TEST_STEP_ZR1_TOUCHLINK_2:
    TRACE_MSG(TRACE_ZLL3, "###TEST_STEP_ZR1_TOUCHLINK_2", (FMT__0));
    g_test_step = TEST_STEP_FINISHED;
    ZB_SCHEDULE_CALLBACK(test_initiate_commissioning, param);
    break;

  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  case TEST_STEP_FINISHED:
    TRACE_MSG(TRACE_ZLL3, "###TEST_STEP_FINISHED", (FMT__0));
    zb_free_buf(buffer);
    break;

  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
  default:
    g_test_step = TEST_STEP_FINISHED;
    TRACE_MSG(TRACE_ERROR, "###ERROR step %hd shan't be processed", (FMT__H, g_test_step));
    break;
  }

  TRACE_MSG(TRACE_ZLL3, "< test_next_step. Curr step %hd" , (FMT__H, g_test_step));
}

/* ************************************************************************** */

#else // defined ZB_ENABLE_ZLL

#include <stdio.h>
int main()
{
  printf(" ZLL is not supported\n");
  return 0;
}

#endif // defined ZB_ENABLE_ZLL
