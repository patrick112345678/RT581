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
/* PURPOSE: TH ZR1
*/


#define ZB_TEST_NAME FB_PRE_TC_07_THR1
#define ZB_TRACE_FILE_ID 41074
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
#include "device_th.h"
#include "device_dut.h"
#include "fb_pre_tc_07_common.h"


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

/********************* Declare device **************************/
DECLARE_TH_CLUSTER_LIST(th_device_clusters,
                        basic_attr_list,
                        identify_attr_list);

DECLARE_TH_EP(th_device_ep,
              THR1_ENDPOINT,
              th_device_clusters);

DECLARE_TH_CTX(th_device_ctx, th_device_ep);


/***********************F&B***************************************/
static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster);
static void trigger_fb_initiator(zb_uint8_t unused);
static void trigger_fb_target(zb_uint8_t unused);


/************************General functions************************/
enum test_commands_e
{
  TEST_COMMAND_START_TEST,
  TEST_COMMAND_CHECK_BIND_TABLE1,
  TEST_COMMAND_BIND_CLUSTER,
  TEST_COMMAND_CHECK_BIND_TABLE2,
  TEST_COMMAND_WAIT_FOR_DUT_REBOOT,
  TEST_COMMAND_CHECK_BIND_TABLE3
};


static void buffer_manager(zb_uint8_t cmd_idx);
static void send_mgmt_bind_req(zb_uint8_t param, zb_uint16_t start_idx);
static void mgmt_bind_resp_cb(zb_uint8_t param);
static void send_bind_req(zb_uint8_t param);
static void bind_resp_cb(zb_uint8_t param);


/************************Static variables*************************/
static int N = DUT_CLUSTER_NUM;
static int M;
static int s_bind_num;
static int s_test_step;


/************************Main*************************************/
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

  /* Assignment required to force Distributed formation */
  ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ROUTER;
  ZB_IEEE_ADDR_COPY(ZB_AIB().trust_center_address, g_unknown_ieee_addr);
  zb_secur_setup_nwk_key(g_nwk_key, 0);

  ZB_AF_REGISTER_DEVICE_CTX(&th_device_ctx);

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


/***********************************Implementation**********************************/
static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster)
{
  TRACE_MSG(TRACE_ZCL1, "finding_binding_cb status %d addr " TRACE_FORMAT_64 " ep %hd cluster %d",
            (FMT__D_A_H_D, status, TRACE_ARG_64(addr), ep, cluster));
  return ZB_TRUE;
}


static void trigger_fb_initiator(zb_uint8_t unused)
{
  ZVUNUSED(unused);
  ZB_BDB().bdb_commissioning_time = DUT_FB_DURATION;
  s_test_step = TEST_COMMAND_CHECK_BIND_TABLE1;
  zb_bdb_finding_binding_initiator(THR1_ENDPOINT, finding_binding_cb);
}


static void trigger_fb_target(zb_uint8_t unused)
{
  ZVUNUSED(unused);
  ZB_BDB().bdb_commissioning_time = THR1_FB_DURATION;
  zb_bdb_finding_binding_target(THR1_ENDPOINT);
}


/************************General functions************************/
static void buffer_manager(zb_uint8_t cmd_idx)
{
  TRACE_MSG(TRACE_ZDO1, ">>buffer_manager: cmd_idx = %d", (FMT__D, cmd_idx));
  TRACE_MSG(TRACE_ZDO1, "buffer_manager: N = %d, M = %d", (FMT__D_D, N, M));

  switch (cmd_idx)
  {
    case TEST_COMMAND_CHECK_BIND_TABLE1:
      ZB_GET_OUT_BUF_DELAYED2(send_mgmt_bind_req, 0);
      break;
    case TEST_COMMAND_BIND_CLUSTER:
      ZB_GET_OUT_BUF_DELAYED(send_bind_req);
      break;
    case TEST_COMMAND_CHECK_BIND_TABLE2:
      ZB_GET_OUT_BUF_DELAYED2(send_mgmt_bind_req, 0);
      break;
    case TEST_COMMAND_WAIT_FOR_DUT_REBOOT:
      ++s_test_step;
      ZB_SCHEDULE_ALARM(buffer_manager, s_test_step, THR1_LONG_DELAY);
      break;
    case TEST_COMMAND_CHECK_BIND_TABLE3:
      ZB_GET_OUT_BUF_DELAYED2(send_mgmt_bind_req, 0);
      break;
    default:
      TRACE_MSG(TRACE_ZDO1, "buffer_manager: unknown state transition", (FMT__0));
  }

  TRACE_MSG(TRACE_ZDO1, "<<buffer_manager", (FMT__0));
}


static void send_mgmt_bind_req(zb_uint8_t param, zb_uint16_t start_idx)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_zdo_mgmt_bind_param_t *req = ZB_GET_BUF_PARAM(buf, zb_zdo_mgmt_bind_param_t);

  TRACE_MSG(TRACE_ZDO2, ">>send_mgmt_bind_req: buf_param = %d, start_idx = %d",
            (FMT__D_H, param, start_idx));

  req->start_index = start_idx;
  req->dst_addr = zb_address_short_by_ieee(g_ieee_addr_dut);

  zb_zdo_mgmt_bind_req(param, mgmt_bind_resp_cb);

  TRACE_MSG(TRACE_ZDO2, "<<send_mgmt_bind_req", (FMT__0));
}


static void mgmt_bind_resp_cb(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_zdo_mgmt_bind_resp_t *resp;
  zb_uint8_t start_idx, num_entries, table_size;
  int continue_checking = 0;
  int move_to_next_step = 0;

  resp = (zb_zdo_mgmt_bind_resp_t*) ZB_BUF_BEGIN(buf);

  TRACE_MSG(TRACE_ZDO2, ">>mgmt_bind_resp_cb: buf_param = %d, status = %d",
            (FMT__D_D, param, resp->status));

  if (resp->status == ZB_ZDP_STATUS_SUCCESS)
  {
    start_idx = resp->start_index;
    num_entries = resp->binding_table_list_count;
    table_size = resp->binding_table_entries;

    continue_checking = start_idx < table_size;
    move_to_next_step = start_idx >= table_size;
  }

  if (continue_checking)
  {
    zb_buf_reuse(buf);
    zb_switch_buf(buf, 0);
    send_mgmt_bind_req(param, start_idx + num_entries);
  }
  else
  {
    zb_free_buf(buf);
  }

  if (move_to_next_step)
  {
    if (s_test_step == TEST_COMMAND_CHECK_BIND_TABLE1)
    {
      M = table_size;
    }
    ++s_test_step;
    TRACE_MSG(TRACE_ZDO2, "mgmt_bind_resp_cb: move to next step - %d",
              (FMT__D, s_test_step));
    ZB_SCHEDULE_CALLBACK(buffer_manager, s_test_step);
  }

  TRACE_MSG(TRACE_ZDO2, "<<mgmt_bind_resp_cb", (FMT__0));
}


static void send_bind_req(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_zdo_bind_req_param_t *req = ZB_GET_BUF_PARAM(buf, zb_zdo_bind_req_param_t);
  zb_uint16_t cluster_to_bind = BINDING_CLUSTER_OFFSET + s_bind_num;

  TRACE_MSG(TRACE_ZDO2, ">>send_bind_req: buf_param = %d, cluster = 0x%x",
            (FMT__D_H, param, cluster_to_bind));

  ZB_IEEE_ADDR_COPY(req->src_address, g_ieee_addr_dut);
  req->src_endp = DUT_ENDPOINT;
  req->cluster_id = cluster_to_bind;
  req->dst_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
  ZB_IEEE_ADDR_COPY(req->dst_address.addr_long, g_ieee_addr_thr1);
  req->dst_endp = THR1_ENDPOINT;
  req->req_dst_addr = zb_address_short_by_ieee(g_ieee_addr_dut);

  zb_zdo_bind_req(param, bind_resp_cb);

  TRACE_MSG(TRACE_ZDO2, "<<send_bind_req", (FMT__0));
}


static void bind_resp_cb(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_uint8_t *aps_body = NULL;
  int make_new_bind = 0;
  int move_to_next_step = 0;

  aps_body = ZB_BUF_BEGIN(buf);

  TRACE_MSG(TRACE_ZDO2, ">>bind_resp_cb: buf_param = %d, status = %d",
            (FMT__D_D, param, *aps_body));

  if (*aps_body == ZB_ZDP_STATUS_SUCCESS)
  {
    ++s_bind_num;
    make_new_bind = s_bind_num < N - M;
    move_to_next_step = s_bind_num >= N - M;
  }
  else
  {
    move_to_next_step = (*aps_body == ZB_ZDP_STATUS_TABLE_FULL);
  }


  if (make_new_bind)
  {
    zb_buf_reuse(buf);
    zb_switch_buf(buf, 0);
    send_bind_req(param);
  }
  else
  {
    zb_free_buf(buf);
  }

  if (move_to_next_step)
  {
    ++s_test_step;
    TRACE_MSG(TRACE_ZDO2, "bind_resp_cb: move to next step - %d",
              (FMT__D, s_test_step));
    ZB_SCHEDULE_CALLBACK(buffer_manager, s_test_step);
  }

  TRACE_MSG(TRACE_ZDO2, "<<bind_resp_cb", (FMT__0));
}


/********************ZDO Startup*****************************/
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
          ZB_SCHEDULE_ALARM(trigger_fb_target, 0, THR1_FB_TARGET_DELAY);
          ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, THR1_FB_INITIATOR_DELAY);
        }
        break;

      case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APS1, "Network steering", (FMT__0));
        ZB_SCHEDULE_ALARM(trigger_fb_target, 0, THR1_FB_TARGET_DELAY);
        ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, THR1_FB_INITIATOR_DELAY);
        break;

      case ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED:
      case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:      
        TRACE_MSG(TRACE_APS1, "Finding&binding done", (FMT__0));
        if ((BDB_COMM_CTX().state == ZB_BDB_COMM_IDLE) &&
            (s_test_step == TEST_COMMAND_CHECK_BIND_TABLE1))
        {
          ZB_SCHEDULE_CALLBACK(buffer_manager, s_test_step);
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
