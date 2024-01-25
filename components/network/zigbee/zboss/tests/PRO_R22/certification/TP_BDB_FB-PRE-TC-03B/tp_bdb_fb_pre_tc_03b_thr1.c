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
/* PURPOSE: DEFERRED: FB-PRE-TC-03B: Service discovery - client side additional tests
*/

#define ZB_TEST_NAME TP_BDB_FB_PRE_TC_03B_THR1
#define ZB_TRACE_FILE_ID 40769

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_zcl.h"
#include "zb_bdb_internal.h"
#include "zb_zcl.h"

#include "../common/zb_cert_test_globals.h"
#include "tp_bdb_fb_pre_tc_03b_common.h"
#include "test_target.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error ZB_USE_NVRAM is not compiled!
#endif


/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_thr1 = IEEE_ADDR_THR1;
static zb_ieee_addr_t g_ieee_addr_the1 = IEEE_ADDR_THE1;
static zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;

static zb_uint8_t g_nwk_key[16] = {0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(fb_pre_tc_03b_thr1_basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(fb_pre_tc_03b_thr1_identify_attr_list, &attr_identify_time);

static zb_bool_t attr_on_off = 1;
ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(fb_pre_tc_03b_thr1_on_off_attr_list, &attr_on_off);


/********************* Declare device **************************/
DECLARE_TARGET_CLUSTER_LIST(fb_pre_tc_03b_the1_target_device_clusters,
                            fb_pre_tc_03b_thr1_basic_attr_list,
                            fb_pre_tc_03b_thr1_identify_attr_list,
                            fb_pre_tc_03b_thr1_on_off_attr_list);

DECLARE_TARGET_EP(fb_pre_tc_03b_thr1_target_device_ep, THR1_ENDPOINT, fb_pre_tc_03b_the1_target_device_clusters);

DECLARE_TARGET_NO_REP_CTX(fb_pre_tc_03b_thr1_target_device_ctx, fb_pre_tc_03b_thr1_target_device_ep);


/**********************General definitions for test***********************/

enum match_desc_options_e
{
  TEST_OPT_WILDCARD,
  TEST_OPT_ZSE,
  TEST_OPT_GW,
  TEST_OPT_MSP,
  TEST_OPT_NEGATIVE_DEVICE_NOT_FOUND,
  TEST_OPT_NEGATIVE_WRONG_ADDR,
  TEST_OPT_NEGATIVE_NO_MATCHING_CLUSTERS,
  TEST_OPT_NEGATIVE_NO_MATCHING_ROLE,
  TEST_OPT_NEGATIVE_EMPTY_CLUSTER_LIST,
  TEST_OPT_TWO_RESPONSES_1,
  TEST_OPT_TWO_RESPONSES_2,
  TEST_OPT_TWO_RESPONSES_3
};

static void trigger_fb_target(zb_uint8_t unused);

static void start_test(zb_uint8_t unused);
static void move_test_fsm(zb_uint8_t unused);
static zb_uint8_t identify_handler(zb_uint8_t param);
static zb_bool_t zdo_rx_handler(zb_uint8_t param, zb_uint16_t cluster_id);

static void send_match_desc_resp(zb_uint8_t param);
static void send_specific_simple_desc_resp(zb_uint8_t param);
static void send_mgmt_bind_req_delayed(zb_uint8_t unused);
static void send_mgmt_bind_req(zb_uint8_t unused);
static void send_mgmt_bind_resp_cb(zb_uint8_t param);

extern void zdo_send_resp_by_short(zb_uint16_t command_id, zb_uint8_t param, zb_uint16_t addr);

static int s_step_idx;

MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_thr1");


  zb_set_long_address(g_ieee_addr_thr1);

  zb_set_network_router_role((1l << TEST_CHANNEL));
  zb_set_nvram_erase_at_start(ZB_TRUE);

  zb_aib_set_trust_center_address(g_unknown_ieee_addr);
  zb_secur_setup_nwk_key(g_nwk_key, 0);

  ZB_AF_REGISTER_DEVICE_CTX(&fb_pre_tc_03b_thr1_target_device_ctx);

  if (zboss_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
  }
  else
  {
    zdo_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}


/******************************Implementation********************************/
static void trigger_fb_target(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  TRACE_MSG(TRACE_APP1, ">>trigger_fb_target", (FMT__0));

  ZB_BDB().bdb_commissioning_time = FB_DURATION;
  zb_bdb_finding_binding_target(THR1_ENDPOINT);

  TRACE_MSG(TRACE_APP1, "<<trigger_fb_target", (FMT__0));
}

static void start_test(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  ZB_AF_SET_ENDPOINT_HANDLER(THR1_ENDPOINT, identify_handler);
}

static void move_test_fsm(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  TRACE_MSG(TRACE_APP1, ">>move_test_fsm, test step %hd", (FMT__D, s_step_idx));

  ++s_step_idx;
  ZB_CERT_HACKS().pass_incoming_zdo_cmd_to_app = 0;
  ZB_CERT_HACKS().zdo_af_handler_cb = NULL;
  ZB_CERT_HACKS().disable_frame_retransmission = 0;
  ZB_CERT_HACKS().force_frame_indication = 0;

  TRACE_MSG(TRACE_APP1, "<<move_test_fsm", (FMT__0));
}

static zb_uint8_t identify_handler(zb_uint8_t param)
{
  zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);

  TRACE_MSG(TRACE_ZCL1, ">>identify_handler: buf_param = %d, step = %d", (FMT__D_D, param, s_step_idx));

  if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_SRV
      && !cmd_info->is_common_command && cmd_info->cluster_id == ZB_ZCL_CLUSTER_ID_IDENTIFY)
  {
    switch (cmd_info->cmd_id)
    {
      case ZB_ZCL_CMD_IDENTIFY_IDENTIFY_QUERY_ID:
        ZB_CERT_HACKS().pass_incoming_zdo_cmd_to_app = 1;
        ZB_CERT_HACKS().zdo_af_handler_cb = zdo_rx_handler;

        if (s_step_idx >= TEST_OPT_NEGATIVE_DEVICE_NOT_FOUND)
        {
          TRACE_MSG(TRACE_ZDO2, ">>identify_handler: force_indication", (FMT__0));
          ZB_CERT_HACKS().force_frame_indication = 1;

          if (s_step_idx <= TEST_OPT_TWO_RESPONSES_1)
          {
            ZB_CERT_HACKS().disable_frame_retransmission_countdown = 2;
            ZB_SCHEDULE_ALARM(move_test_fsm, 0, THR1_INCR_TS_SHORT_DELAY);
          }
          else
          {
            ZB_SCHEDULE_ALARM(move_test_fsm, 0, THR1_INCR_TS_LONG_DELAY);
          }
        }
        break;
    }
  }

  TRACE_MSG(TRACE_ZCL1, "<<identify_handler", (FMT__0));

  return ZB_FALSE;
}

static zb_bool_t zdo_rx_handler(zb_uint8_t param, zb_uint16_t cluster_id)
{
  int drop_packet = 0;

  TRACE_MSG(TRACE_ZDO1, ">>zdo_rx_handler: buf_param = %d", (FMT__D, param));

  switch (cluster_id)
  {
    case ZDO_SIMPLE_DESC_REQ_CLID:
      if (s_step_idx < TEST_OPT_TWO_RESPONSES_1)
      {
        ZB_SCHEDULE_CALLBACK(send_specific_simple_desc_resp, param);
        ZB_SCHEDULE_ALARM(send_mgmt_bind_req_delayed, 0, THR1_MGMT_BIND_REQ_DELAY_SHORT);
      }
      else if (s_step_idx == TEST_OPT_TWO_RESPONSES_1)
      {
        ZB_SCHEDULE_ALARM(send_specific_simple_desc_resp, param, THR1_RESP_DELAY);
        ZB_SCHEDULE_ALARM(send_mgmt_bind_req_delayed, 0, THR1_MGMT_BIND_REQ_DELAY_LONG);
      }
      else
      {
        drop_packet = 1;
      }
      break;

    case ZDO_MATCH_DESC_REQ_CLID:
      ZB_SCHEDULE_ALARM(send_match_desc_resp, param, THR1_RESP_DELAY);
      ZB_SCHEDULE_ALARM(send_mgmt_bind_req_delayed, 0, THR1_MGMT_BIND_REQ_DELAY_LONG);
      break;

    default:
      drop_packet = 1;
  }

  if (drop_packet)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZDO1, "<<zdo_rx_handler", (FMT__0));

  return ZB_TRUE;
}

/*========================Additinal functions=======================================*/
static void send_match_desc_resp(zb_uint8_t param)
{
  zb_uint8_t *ptr, *aps_body;
  zb_uint16_t addr_of_cmd = zb_address_short_by_ieee(g_ieee_addr_the1);
  zb_apsde_data_indication_t *ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);
  zb_uint16_t req_origin_addr;
  zb_uint8_t tsn;

  TRACE_MSG(TRACE_ZDO1, "<<send_match_desc_resp: param = %d, s_step_idx = %d",
            (FMT__D_H, param, s_step_idx));

  aps_body = zb_buf_begin(param);
  tsn = *aps_body;
  aps_body++;
  req_origin_addr = ind->src_addr;

  ptr = zb_buf_initial_alloc(param, 5);

  *ptr++ = tsn;

  if (s_step_idx == TEST_OPT_TWO_RESPONSES_3)
  {
    *ptr++ = ZB_ZDP_STATUS_NO_DESCRIPTOR;
  }
  else
  {
    *ptr++ = ZB_ZDP_STATUS_DEVICE_NOT_FOUND;
  }

  ptr = zb_put_next_htole16(ptr, addr_of_cmd);
  *ptr = 0;

  zdo_send_resp_by_short(ZDO_MATCH_DESC_RESP_CLID, param, req_origin_addr);

  TRACE_MSG(TRACE_ZDO1, ">>send_match_desc_resp", (FMT__0));
}

static void send_specific_simple_desc_resp(zb_uint8_t param)
{
  zb_uint8_t *ptr, *aps_body;
  zb_apsde_data_indication_t *ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);
  zb_uint16_t req_origin_addr;
  zb_uint8_t tsn;
  zb_uint8_t len;

  TRACE_MSG(TRACE_ZDO1, "<<send_specific_simple_desc_resp: param = %d, s_step_idx = %d",
            (FMT__D_H, param, s_step_idx));

  aps_body = zb_buf_begin(param);
  tsn = *aps_body;
  aps_body++;
  req_origin_addr = ind->src_addr;

  ptr = zb_buf_initial_alloc(param, 5);

  *ptr++ = tsn;

  switch (s_step_idx)
  {
    case TEST_OPT_NEGATIVE_DEVICE_NOT_FOUND:
    case TEST_OPT_TWO_RESPONSES_1:
      *ptr++ = ZB_ZDP_STATUS_DEVICE_NOT_FOUND;
      break;

    default:
      *ptr++ = ZB_ZDP_STATUS_SUCCESS;
      break;
  }

  switch (s_step_idx)
  {
    case TEST_OPT_WILDCARD:
    case TEST_OPT_ZSE:
    case TEST_OPT_GW:
    case TEST_OPT_MSP:
      ptr = zb_put_next_htole16(ptr, ZB_PIBCACHE_NETWORK_ADDRESS());
      break;
    case TEST_OPT_NEGATIVE_WRONG_ADDR:
      /* todo: remove it, when bug of incorrect behavior of dut will be fixed */
      ptr = zb_put_next_htole16(ptr, zb_address_short_by_ieee(g_ieee_addr_the1));

      /* zb_put_next_htole16(&ptr, zb_random()); */
      break;
    default:
      ptr = zb_put_next_htole16(ptr, zb_address_short_by_ieee(g_ieee_addr_the1));
  }

  switch (s_step_idx)
  {
    case TEST_OPT_NEGATIVE_DEVICE_NOT_FOUND:
    case TEST_OPT_TWO_RESPONSES_1:
      len = 0;
      break;
    case TEST_OPT_NEGATIVE_EMPTY_CLUSTER_LIST:
      len = 8;
      break;
    case TEST_OPT_NEGATIVE_NO_MATCHING_CLUSTERS:
    case TEST_OPT_NEGATIVE_NO_MATCHING_ROLE:
      len = 8 + 3*sizeof(zb_uint16_t);
      break;
    default:
      len = 8 + 4*sizeof(zb_uint16_t);
      break;
  }
  *ptr++ = len;

  if (len)
  {
    zb_uint16_t profile_id;

    ptr = zb_buf_alloc_right(param, len);
    *ptr++ = (s_step_idx < TEST_OPT_NEGATIVE_DEVICE_NOT_FOUND)? THR1_ENDPOINT:
                                                                THE1_ENDPOINT;

    switch (s_step_idx)
    {
      case TEST_OPT_WILDCARD:
        profile_id = 0xffff;
        break;
      case TEST_OPT_ZSE:
        profile_id = 0x0109;
        break;
      case TEST_OPT_GW:
        profile_id = 0x7f02;
        break;
      case TEST_OPT_MSP:
        profile_id = 0x8000;
        break;
      default:
        profile_id = ZB_AF_HA_PROFILE_ID;
    }
    ptr = zb_put_next_htole16(ptr, profile_id);
    ptr = zb_put_next_htole16(ptr, DEVICE_ID_TARGET);
    *ptr++ = DEVICE_VER_TARGET;

    switch (s_step_idx)
    {
      case TEST_OPT_NEGATIVE_NO_MATCHING_CLUSTERS:
        {
          *ptr++ = 0;
          *ptr++ = 3;
          ptr = zb_put_next_htole16(ptr, ZB_ZCL_CLUSTER_ID_SHADE_CONFIG);
          ptr = zb_put_next_htole16(ptr, ZB_ZCL_CLUSTER_ID_DOOR_LOCK);
          ptr = zb_put_next_htole16(ptr, ZB_ZCL_CLUSTER_ID_WINDOW_COVERING);
        }
        break;
      case TEST_OPT_NEGATIVE_NO_MATCHING_ROLE:
        {
          *ptr++ = 0;
          *ptr++ = 3;
          ptr = zb_put_next_htole16(ptr, ZB_ZCL_CLUSTER_ID_SHADE_CONFIG);
          ptr = zb_put_next_htole16(ptr, ZB_ZCL_CLUSTER_ID_DOOR_LOCK);
          ptr = zb_put_next_htole16(ptr, ZB_ZCL_CLUSTER_ID_ON_OFF);
        }
        break;
      case TEST_OPT_NEGATIVE_EMPTY_CLUSTER_LIST:
        {
          *ptr++ = 0;
          *ptr++ = 0;
        }
        break;
      default:
        {
          *ptr++ = 3;
          ptr = zb_put_next_htole16(ptr, ZB_ZCL_CLUSTER_ID_IDENTIFY);
          ptr = zb_put_next_htole16(ptr, ZB_ZCL_CLUSTER_ID_BASIC);
          ptr = zb_put_next_htole16(ptr, ZB_ZCL_CLUSTER_ID_ON_OFF);
          *ptr++ = 1;
          ptr = zb_put_next_htole16(ptr, ZB_ZCL_CLUSTER_ID_IDENTIFY);
        }
    }
  }

  zdo_send_resp_by_short(ZDO_SIMPLE_DESC_RESP_CLID, param, req_origin_addr);

  TRACE_MSG(TRACE_ZDO1, ">>send_specific_simple_desc_resp", (FMT__0));
}

static void send_mgmt_bind_req_delayed(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  zb_buf_get_out_delayed(send_mgmt_bind_req);
}

static void send_mgmt_bind_req(zb_uint8_t param)
{
  zb_zdo_mgmt_bind_param_t *req_params;

  TRACE_MSG(TRACE_ZDO3, ">>send_mgmt_bind_req", (FMT__0));

  req_params = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_bind_param_t);

  req_params->start_index = 0;
  req_params->dst_addr = zb_address_short_by_ieee(g_ieee_addr_dut);
  zb_zdo_mgmt_bind_req(param, send_mgmt_bind_resp_cb);

  TRACE_MSG(TRACE_ZDO3, "<<send_mgmt_bind_req", (FMT__0));
}

static void send_mgmt_bind_resp_cb(zb_uint8_t param)
{
  zb_zdo_mgmt_bind_resp_t *resp = (zb_zdo_mgmt_bind_resp_t*) zb_buf_begin(param);

  TRACE_MSG(TRACE_ZDO1, ">>mgmt_bind_resp_cb", (FMT__0));

  if (resp->status != ZB_ZDP_STATUS_SUCCESS)
  {
    TRACE_MSG(TRACE_ZDO1, "mgmt_bind_resp_cb: error - status failed [0x%x]", (FMT__H, resp->status));
  }

  zb_buf_free(param);

  TRACE_MSG(TRACE_ZDO1, "<<mgmt_bind_resp_cb", (FMT__0));
}

/*==============================ZDO Startup Complete===============================*/
ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  switch (sig)
  {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      if (status == 0)
      {
        zb_ext_pan_id_t extended_pan_id;

        TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));

        zb_get_extended_pan_id(extended_pan_id);

        if (ZB_IEEE_ADDR_CMP(g_ieee_addr_thr1, extended_pan_id))
        {
          bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        }
        else
        {
          ZB_SCHEDULE_CALLBACK(start_test, 0);
          ZB_SCHEDULE_ALARM(trigger_fb_target, 0, THR1_FB_FIRST_START_DELAY);
        }
      }
      else
      {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, status));
      }
      break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_STEERING:
      if (status == 0)
      {
        TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_STEERING, status OK", (FMT__0));

        ZB_SCHEDULE_CALLBACK(start_test, 0);
        ZB_SCHEDULE_ALARM(trigger_fb_target, 0, THR1_FB_FIRST_START_DELAY);
      }
      else
      {
        TRACE_MSG(TRACE_ERROR, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
      }
      break; /* ZB_BDB_SIGNAL_STEERING */

    case ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED:
      if (status == 0)
      {
        TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED, status OK", (FMT__0));
        if (BDB_COMM_CTX().state == ZB_BDB_COMM_IDLE)
        {
          move_test_fsm(0);
          if (s_step_idx < TEST_OPT_NEGATIVE_DEVICE_NOT_FOUND)
          {
            ZB_SCHEDULE_ALARM(trigger_fb_target, 0, THR1_FB_START_DELAY);
          }
        }
      }
      else
      {
        TRACE_MSG(TRACE_ERROR, "signal: ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED, status %d", (FMT__D, status));
      }
      break; /* ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED */

    default:
      if (status == 0)
      {
        TRACE_MSG(TRACE_APS1, "Unknown signal, status OK", (FMT__0));
      }
      else
      {
        TRACE_MSG(TRACE_ERROR, "Unknown signal, status %d", (FMT__D, status));
      }
      break;
  }

  zb_buf_free(param);
}

/*! @} */
