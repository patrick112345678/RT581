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
/* PURPOSE: TH ZR1 (test driver)
*/


#define ZB_TEST_NAME FB_PRE_TC_03B_THR1
#define ZB_TRACE_FILE_ID 41248
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
#include "fb_pre_tc_03b_common.h"
#include "test_target.h"


#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#ifndef ZB_USE_NVRAM
#error ZB_USE_NVRAM is not compiled!
#endif


/*! \addtogroup ZB_TESTS */
/*! @{ */
/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &attr_identify_time);

static zb_bool_t attr_on_off = 1;
ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(on_off_attr_list, &attr_on_off);


/********************* Declare device **************************/
DECLARE_TARGET_CLUSTER_LIST(target_device_clusters,
                            basic_attr_list,
                            identify_attr_list,
                            on_off_attr_list);

DECLARE_TARGET_EP(target_device_ep, THR1_ENDPOINT, target_device_clusters);

DECLARE_TARGET_NO_REP_CTX(target_device_ctx, target_device_ep);


/**********************General definitions for test***********************/

enum match_desc_options_e
{
  TEST_OPT_LEGACY_0101,
  TEST_OPT_LEGACY_0102,
  TEST_OPT_LEGACY_0103,
  TEST_OPT_LEGACY_0105,
  TEST_OPT_LEGACY_0106,
  TEST_OPT_LEGACY_0107,
  TEST_OPT_LEGACY_0108,
  TEST_OPT_LEGACY_C05E,
  TEST_OPT_WILDCARD,
  TEST_OPT_ZSE,
  TEST_OPT_GW,
  TEST_OPT_MSP,
  TEST_OPT_NEGATIVE_DEVICE_NOT_FOUND, /* 12 */
  TEST_OPT_NEGATIVE_WRONG_ADDR,
  TEST_OPT_NEGATIVE_NO_MATCHING_CLUSTERS,
  TEST_OPT_NEGATIVE_NO_MATCHING_ROLE,
  TEST_OPT_NEGATIVE_EMPTY_CLUSTER_LIST,
  TEST_OPT_TWO_RESPONSES_1,
  TEST_OPT_TWO_RESPONSES_2,
  TEST_OPT_TWO_RESPONSES_3,
  TEST_OPT_COUNT
};

static void trigger_fb_target(zb_uint8_t unused);

static void start_test(zb_uint8_t unused);
static void move_test_fsm(zb_uint8_t unused);
static zb_uint8_t identify_handler(zb_uint8_t param);
static void zdo_rx_handler(zb_uint8_t param, zb_uint16_t cluster_id);

static void send_active_ep_resp(zb_uint8_t param, zb_uint16_t options);
static void send_match_desc_resp(zb_uint8_t param, zb_uint16_t options);
static void send_specific_simple_desc_resp(zb_uint8_t param, zb_uint16_t options);

extern void zdo_send_resp_by_short(zb_uint16_t command_id, zb_uint8_t param, zb_uint16_t addr);


static int s_step_idx;


MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_thr1");


  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_thr1);

  ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
  ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;
  ZB_BDB().bdb_mode = 1;
  ZB_AIB().aps_use_nvram = 1;
  
  ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ROUTER;
  ZB_IEEE_ADDR_COPY(ZB_AIB().trust_center_address, g_unknown_ieee_addr);
  zb_secur_setup_nwk_key(g_nwk_key, 0);

  ZB_AF_REGISTER_DEVICE_CTX(&target_device_ctx);

  if (zdo_dev_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
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
  ZB_BDB().bdb_commissioning_time = FB_DURATION;
  zb_bdb_finding_binding_target(THR1_ENDPOINT);
}


static void start_test(zb_uint8_t unused)
{
  ZVUNUSED(unused);
  ZB_AF_SET_ENDPOINT_HANDLER(THR1_ENDPOINT, identify_handler);
}


static void move_test_fsm(zb_uint8_t unused)
{
  ZVUNUSED(unused);
  ++s_step_idx;
  ZB_CERT_HACKS().pass_incoming_zdo_cmd_to_app = 0;
  ZB_CERT_HACKS().zdo_af_handler_cb = NULL;
  ZB_CERT_HACKS().disable_frame_retransmission = 0;
  ZB_CERT_HACKS().force_frame_indication = 0;
}


static zb_uint8_t identify_handler(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_zcl_parsed_hdr_t *cmd_info = ZB_GET_BUF_PARAM(buf, zb_zcl_parsed_hdr_t);

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
          ZB_SCHEDULE_ALARM(move_test_fsm, 0, THR1_FB_START_DELAY);
          TRACE_MSG(TRACE_ZDO2, ">>identify_handler: force_indication", (FMT__0));
          ZB_CERT_HACKS().force_frame_indication = 1;
        }
        break;
    }
  }

  TRACE_MSG(TRACE_ZCL1, "<<identify_handler", (FMT__0));

  return ZB_FALSE;
}


static void zdo_rx_handler(zb_uint8_t param, zb_uint16_t cluster_id)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  int drop_packet = 0;

  TRACE_MSG(TRACE_ZDO1, ">>zdo_rx_handler: buf_param = %d", (FMT__D, param));

  switch (cluster_id)
  {
    case ZDO_ACTIVE_EP_REQ_CLID:
      if (s_step_idx < TEST_OPT_NEGATIVE_DEVICE_NOT_FOUND)
      {
        ZB_SCHEDULE_CALLBACK2(send_active_ep_resp, param, s_step_idx);
      }
      else
      {
        drop_packet = 1;
      }
      break;

    case ZDO_ACTIVE_EP_RESP_CLID:
      if ( (s_step_idx >= TEST_OPT_NEGATIVE_DEVICE_NOT_FOUND) &&
           (s_step_idx <= TEST_OPT_NEGATIVE_EMPTY_CLUSTER_LIST) )
      {
        TRACE_MSG(TRACE_ZDO2, ">>zdo_rx_handler: disable_retransmit + force_indication", (FMT__0));
        ZB_CERT_HACKS().disable_frame_retransmission = 1;
        drop_packet = 1;
      }
      break;

    case ZDO_SIMPLE_DESC_REQ_CLID:
      if (s_step_idx < TEST_OPT_TWO_RESPONSES_2)
      {
        ZB_SCHEDULE_CALLBACK2(send_specific_simple_desc_resp, param, s_step_idx);
      }
      else
      {
        drop_packet = 1;
      }
      break;

    case ZDO_MATCH_DESC_REQ_CLID:
      ZB_SCHEDULE_CALLBACK2(send_match_desc_resp, param, s_step_idx);
      break;

    default:
      drop_packet = 1;
  }

  if (drop_packet)
  {
    zb_free_buf(buf);
  }

  TRACE_MSG(TRACE_ZDO1, "<<zdo_rx_handler", (FMT__0));
}


/*========================Additinal functions=======================================*/
static void send_active_ep_resp(zb_uint8_t param, zb_uint16_t options)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_uint8_t *aps_body, *ptr;
  zb_zdo_active_ep_req_t *req;
  zb_apsde_data_indication_t ind;
  zb_uint16_t nwk_addr;
  zb_uint8_t tsn;

  TRACE_MSG(TRACE_ZDO1, ">>send_active_ep_resp: param = %d, options = 0x%x",
            (FMT__D_H, param, options));

  aps_body = ZB_BUF_BEGIN(buf);
  tsn = *aps_body++;
  req = (zb_zdo_active_ep_req_t*) aps_body;
  ZB_HTOLE16(&nwk_addr, &req->nwk_addr);
  ZB_MEMCPY(&ind, ZB_GET_BUF_PARAM(buf, zb_apsde_data_indication_t), sizeof(ind));

  ZB_BUF_INITIAL_ALLOC(buf, 5, ptr);
  *ptr++ = ZB_ZDP_STATUS_SUCCESS;
  zb_put_next_htole16(&ptr, nwk_addr);
  if (options < TEST_OPT_NEGATIVE_DEVICE_NOT_FOUND)
  {
    *ptr++ = ZB_ZDO_SIMPLE_DESC_NUMBER();
    *ptr = THR1_ENDPOINT;
  }
  else
  {
    *ptr++ = 1;
    *ptr = THE1_ENDPOINT;
  }

  TRACE_MSG(TRACE_ZDO1, "TEST: addr = 0x%x", (FMT__H, ind.src_addr));
  
  zdo_send_resp_by_short(ZDO_ACTIVE_EP_RESP_CLID, param, ind.src_addr);

  TRACE_MSG(TRACE_ZDO1, "<<send_active_ep_resp", (FMT__0));
}


static void send_match_desc_resp(zb_uint8_t param, zb_uint16_t options)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_uint8_t *ptr, *aps_body;
  zb_uint16_t addr_of_cmd = zb_address_short_by_ieee(g_ieee_addr_the1);
  zb_apsde_data_indication_t *ind = ZB_GET_BUF_PARAM(buf, zb_apsde_data_indication_t);
  zb_uint16_t req_origin_addr;
  zb_uint8_t tsn;

  TRACE_MSG(TRACE_ZDO1, "<<send_match_desc_resp: param = %d, options = %d",
            (FMT__D_H, param, options));

  aps_body = ZB_BUF_BEGIN(buf);
  tsn = *aps_body;
  aps_body++;
  req_origin_addr = ind->src_addr;

  ZB_BUF_INITIAL_ALLOC(buf, 4, ptr);
  if (options == TEST_OPT_TWO_RESPONSES_3)
  {
    *ptr++ = ZB_ZDP_STATUS_NO_DESCRIPTOR;
  }
  else
  {
    *ptr++ = ZB_ZDP_STATUS_DEVICE_NOT_FOUND;
  }
  zb_put_next_htole16(&ptr, addr_of_cmd);
  *ptr = 0;

  zdo_send_resp_by_short(ZDO_MATCH_DESC_RESP_CLID, param, req_origin_addr);
  
  TRACE_MSG(TRACE_ZDO1, ">>send_match_desc_resp", (FMT__0));
}


static void send_specific_simple_desc_resp(zb_uint8_t param, zb_uint16_t options)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_uint8_t *ptr, *aps_body;
  zb_apsde_data_indication_t *ind = ZB_GET_BUF_PARAM(buf, zb_apsde_data_indication_t);
  zb_uint16_t req_origin_addr;
  zb_uint8_t tsn;
  zb_uint8_t len;

  TRACE_MSG(TRACE_ZDO1, "<<send_specific_simple_desc_resp: param = %d, options = %d",
            (FMT__D_H, param, options));

  aps_body = ZB_BUF_BEGIN(buf);
  tsn = *aps_body;
  aps_body++;
  req_origin_addr = ind->src_addr;

  ZB_BUF_INITIAL_ALLOC(buf, 4, ptr);

  switch (options)
  {
    case TEST_OPT_NEGATIVE_DEVICE_NOT_FOUND:
    case TEST_OPT_TWO_RESPONSES_1:
      *ptr++ = ZB_ZDP_STATUS_DEVICE_NOT_FOUND;
      break;

    default:
      *ptr++ = ZB_ZDP_STATUS_SUCCESS;
      break;
  }

  switch (options)
  {
    case TEST_OPT_LEGACY_0101:
    case TEST_OPT_LEGACY_0102:
    case TEST_OPT_LEGACY_0103:
    case TEST_OPT_LEGACY_0105:
    case TEST_OPT_LEGACY_0106:
    case TEST_OPT_LEGACY_0107:
    case TEST_OPT_LEGACY_0108:
    case TEST_OPT_LEGACY_C05E:
    case TEST_OPT_WILDCARD:
    case TEST_OPT_ZSE:
    case TEST_OPT_GW:
    case TEST_OPT_MSP:
      zb_put_next_htole16(&ptr, ZB_PIBCACHE_NETWORK_ADDRESS());
      break;
    case TEST_OPT_NEGATIVE_WRONG_ADDR:
      zb_put_next_htole16(&ptr, zb_random());
      break;
    default:
      zb_put_next_htole16(&ptr, zb_address_short_by_ieee(g_ieee_addr_the1));
  }

  switch (options)
  {
    case TEST_OPT_NEGATIVE_DEVICE_NOT_FOUND:
    case TEST_OPT_TWO_RESPONSES_1:
      len = 0;
      break;
    case TEST_OPT_NEGATIVE_EMPTY_CLUSTER_LIST:
      len = 8;
      break;
    default:
      len = 8 + 3*sizeof(zb_uint16_t);
  }
  *ptr = len;

  if (len)
  {
    zb_uint16_t profile_id;

    ZB_BUF_ALLOC_RIGHT(buf, len, ptr);
    *ptr++ = (s_step_idx < TEST_OPT_NEGATIVE_DEVICE_NOT_FOUND)? THR1_ENDPOINT:
                                                                THE1_ENDPOINT;

    switch (s_step_idx)
    {
      case TEST_OPT_LEGACY_0101:
        profile_id = 0x0101;
        break;
      case TEST_OPT_LEGACY_0102:
        profile_id = 0x0102;
        break;
      case TEST_OPT_LEGACY_0103:
        profile_id = 0x0103;
        break;
      case TEST_OPT_LEGACY_0105:
        profile_id = 0x0105;
        break;
      case TEST_OPT_LEGACY_0106:
        profile_id = 0x0106;
        break;
      case TEST_OPT_LEGACY_0107:
        profile_id = 0x0107;
        break;
      case TEST_OPT_LEGACY_0108:
        profile_id = 0x0108;
        break;
      case TEST_OPT_LEGACY_C05E:
        profile_id = 0xc05e;
        break;
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
    zb_put_next_htole16(&ptr, profile_id);
    zb_put_next_htole16(&ptr, DEVICE_ID_TARGET);
    *ptr++ = DEVICE_VER_TARGET;

    switch (options)
    {
      case TEST_OPT_NEGATIVE_NO_MATCHING_CLUSTERS:
        {
          *ptr++ = 0;
          *ptr++ = 3;
          zb_put_next_htole16(&ptr, ZB_ZCL_CLUSTER_ID_SHADE_CONFIG);
          zb_put_next_htole16(&ptr, ZB_ZCL_CLUSTER_ID_DOOR_LOCK);
          zb_put_next_htole16(&ptr, ZB_ZCL_CLUSTER_ID_WINDOW_COVERING);
        }
        break;
      case TEST_OPT_NEGATIVE_NO_MATCHING_ROLE:
        {
          *ptr++ = 0;
          *ptr++ = 3;
          zb_put_next_htole16(&ptr, ZB_ZCL_CLUSTER_ID_SHADE_CONFIG);
          zb_put_next_htole16(&ptr, ZB_ZCL_CLUSTER_ID_DOOR_LOCK);
          zb_put_next_htole16(&ptr, ZB_ZCL_CLUSTER_ID_ON_OFF);
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
          zb_put_next_htole16(&ptr, ZB_ZCL_CLUSTER_ID_BASIC);
          zb_put_next_htole16(&ptr, ZB_ZCL_CLUSTER_ID_IDENTIFY);
          zb_put_next_htole16(&ptr, ZB_ZCL_CLUSTER_ID_ON_OFF);
          *ptr = 0;
        }
    }
  }

  zdo_send_resp_by_short(ZDO_SIMPLE_DESC_RESP_CLID, param, req_origin_addr);
  
  TRACE_MSG(TRACE_ZDO1, ">>send_specific_simple_desc_resp", (FMT__0));
}


/*==============================ZDO Startup Complete===============================*/
ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
        if (ZB_IEEE_ADDR_CMP(g_ieee_addr_thr1, ZB_NIB().extended_pan_id))
        {
          bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        }
        else
        {
          ZB_SCHEDULE_ALARM(start_test, 0, 5 * ZB_TIME_ONE_SECOND);
          ZB_SCHEDULE_ALARM(trigger_fb_target, 0, THR1_FB_START_DELAY);
        }
        break;

      case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APS1, "Network steering", (FMT__0));
        ZB_SCHEDULE_ALARM(start_test, 0, 5 * ZB_TIME_ONE_SECOND);
        ZB_SCHEDULE_ALARM(trigger_fb_target, 0, THR1_FB_START_DELAY);
        break;

      case ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED:
        TRACE_MSG(TRACE_APS1, "Finding&binding done", (FMT__0));
        if (BDB_COMM_CTX().state == ZB_BDB_COMM_IDLE)
        {
          move_test_fsm(0);
          if (s_step_idx < TEST_OPT_NEGATIVE_DEVICE_NOT_FOUND)
          {
            ZB_SCHEDULE_ALARM(trigger_fb_target, 0, THR1_FB_START_DELAY);
          }
        }
        break;

      default:
        TRACE_MSG(TRACE_APS1, "Unknown signal", (FMT__0));
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
  zb_free_buf(ZB_BUF_FROM_REF(param));
}


/*! @} */
