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
/* PURPOSE: FB-PRE-TC-04A: Service discovery - server side tests (TH ZR1)
*/

#define ZB_TEST_NAME TP_BDB_FB_PRE_TC_04A_THR1
#define ZB_TRACE_FILE_ID 40908

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

#include "tp_bdb_fb_pre_tc_04a_common.h"
#include "test_initiator.h"
#include "../common/zb_cert_test_globals.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif


/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_thr1 = IEEE_ADDR_THR1;
static zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;

static zb_uint8_t g_nwk_key[16] = {0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(fb_pre_tc_04a_thr1_basic_attr_list,
                                 &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(fb_pre_tc_04a_thr1_identify_attr_list, &attr_identify_time);

static zb_bool_t attr_on_off = 1;
ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(fb_pre_tc_04a_thr1_on_off_attr_list, &attr_on_off);

/********************* Declare device **************************/
DECLARE_INITIATOR_CLUSTER_LIST(fb_pre_tc_04a_thr1_initiator_device_clusters,
                               fb_pre_tc_04a_thr1_basic_attr_list,
                               fb_pre_tc_04a_thr1_identify_attr_list,
                               fb_pre_tc_04a_thr1_on_off_attr_list);

DECLARE_INITIATOR_EP(fb_pre_tc_04a_thr1_initiator_device_ep,
                     THR1_ENDPOINT, fb_pre_tc_04a_thr1_initiator_device_clusters);

DECLARE_INITIATOR_CTX(fb_pre_tc_04a_thr1_initiator_device_ctx,
                      fb_pre_tc_04a_thr1_initiator_device_ep);


/**********************General definitions for test***********************/

enum match_desc_options_e
{
  TEST_OPT_HA,
  TEST_OPT_WILDCARD,
  TEST_OPT_BROADCAST,
  TEST_OPT_COUNT
};
static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster);

static void trigger_fb_initiator(zb_uint8_t unused);

static void send_active_ep_req_delayed(zb_uint8_t unused);
static void send_active_ep_req(zb_uint8_t param);
static void active_ep_callback(zb_uint8_t param);
static void send_match_desc_loop(zb_uint8_t id);
static void send_match_desc_req(zb_uint8_t param, zb_uint16_t options);
static void match_desc_resp_cb(zb_uint8_t param);

static int s_step_idx;

MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_thr1");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif


  zb_set_long_address(g_ieee_addr_thr1);

  zb_set_network_router_role((1l << TEST_CHANNEL));
  zb_set_nvram_erase_at_start(ZB_TRUE);

  zb_aib_set_trust_center_address(g_unknown_ieee_addr);
  zb_secur_setup_nwk_key(g_nwk_key, 0);

  ZB_AF_REGISTER_DEVICE_CTX(&fb_pre_tc_04a_thr1_initiator_device_ctx);

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
  zb_bdb_finding_binding_initiator(THR1_ENDPOINT, finding_binding_cb);
}

static void send_active_ep_req_delayed(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  zb_buf_get_out_delayed(send_active_ep_req);
}

static void send_active_ep_req(zb_uint8_t param)
{
  zb_zdo_active_ep_req_t *req;

  req = zb_buf_initial_alloc(param, sizeof(zb_zdo_active_ep_req_t));
  req->nwk_addr = zb_address_short_by_ieee(g_ieee_addr_dut);

  zb_zdo_active_ep_req(param, active_ep_callback);
}

static void active_ep_callback(zb_uint8_t param)
{
  zb_uint8_t *zdp_cmd = zb_buf_begin(param);
  zb_zdo_ep_resp_t *resp = (zb_zdo_ep_resp_t*)zdp_cmd;

  TRACE_MSG(TRACE_APS1, "active_ep_callback status %hd, addr 0x%x",
            (FMT__H, resp->status, resp->nwk_addr));

  if (resp->status != ZB_ZDP_STATUS_SUCCESS)
  {
    TRACE_MSG(TRACE_APS1, "Error incorrect status", (FMT__0));
  }

  zb_buf_free(param);
  ZB_SCHEDULE_ALARM(send_match_desc_loop, 0, THR1_SHORT_DELAY);
}

static void send_match_desc_loop(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  TRACE_MSG(TRACE_ZDO1, ">>send_match_desc_loop: step = %d", (FMT__D, s_step_idx));

  if (s_step_idx < TEST_OPT_COUNT)
  {
    zb_buf_get_out_delayed_ext(send_match_desc_req, s_step_idx, 0);
  }

  TRACE_MSG(TRACE_ZDO1, "<<send_match_desc_loop", (FMT__0));
}


static void send_match_desc_req(zb_uint8_t param, zb_uint16_t options)
{
  zb_zdo_match_desc_param_t *req;
  zb_uint16_t size = sizeof(zb_zdo_match_desc_param_t);
  zb_uint16_t profile_id = ZB_AF_HA_PROFILE_ID;
  zb_uint16_t addr = zb_address_short_by_ieee(g_ieee_addr_dut);
  zb_uint8_t in_clusters = 1, out_clusters = 0;

  switch (options)
  {
    case TEST_OPT_HA:
      size += 5*sizeof(zb_uint16_t);
      out_clusters = in_clusters = 3;
      break;
    case TEST_OPT_WILDCARD:
      profile_id = 0xffff;
      break;
    case TEST_OPT_BROADCAST:
      addr = 0xfffd;
      break;
  }

  TRACE_MSG(TRACE_ZDO1, ">>send_match_desc_req: param = %d, addr = 0x%x, profile_id = 0x%x",
            (FMT__D_H_H, param, addr, profile_id));

  req = zb_buf_initial_alloc(param, size);
  req->nwk_addr = addr;
  req->addr_of_interest = addr;
  req->profile_id = profile_id;
  req->num_in_clusters = in_clusters;
  req->num_out_clusters = out_clusters;

  switch (options)
  {
    case TEST_OPT_HA:
      {
        /* in clusters */
        req->cluster_list[0] = ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT;
        req->cluster_list[1] = ZB_ZCL_CLUSTER_ID_COLOR_CONTROL;
        req->cluster_list[2] = ZB_ZCL_CLUSTER_ID_PUMP_CONFIG_CONTROL;
        /* out clusters */
        req->cluster_list[3] = ZB_ZCL_CLUSTER_ID_IAS_ZONE;
        req->cluster_list[4] = ZB_ZCL_CLUSTER_ID_ON_OFF;
        req->cluster_list[5] = ZB_ZCL_CLUSTER_ID_IAS_WD;
      }
      break;

    default:
      {
        req->cluster_list[0] = ZB_ZCL_CLUSTER_ID_IDENTIFY;
      }
  }

  zb_zdo_match_desc_req(param, match_desc_resp_cb);

  TRACE_MSG(TRACE_ZDO1, "<<send_match_desc_req", (FMT__0));
}

static void match_desc_resp_cb(zb_uint8_t param)
{

  TRACE_MSG(TRACE_ZDO1, ">>match_desc_resp_cb: param = 0x%x", (FMT__D, param));

  zb_buf_free(param);
  ++s_step_idx;
  ZB_SCHEDULE_ALARM(send_match_desc_loop, 0, THR1_SHORT_DELAY);

  TRACE_MSG(TRACE_ZDO1, "<<match_desc_resp_cb", (FMT__0));
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  switch (sig)
  {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      TRACE_MSG(TRACE_APS1, "Device started, status %d", (FMT__D, status));
      if (status == 0)
      {
        zb_ext_pan_id_t extended_pan_id;
        zb_get_extended_pan_id(extended_pan_id);

        if (ZB_IEEE_ADDR_CMP(g_ieee_addr_thr1, extended_pan_id))
        {
          bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        }
        else
        {
          ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, THR1_FB_START_DELAY);
        }
      }
      break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_STEERING:
      TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
      break; /* ZB_BDB_SIGNAL_STEERING */

    case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
      TRACE_MSG(TRACE_APS1, "signal: ZB_ZDO_SIGNAL_DEVICE_ANNCE, status %d", (FMT__D, status));
      ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, THR1_FB_START_DELAY);
      break; /* ZB_ZDO_SIGNAL_DEVICE_ANNCE */

    case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:
      TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED, status %d", (FMT__D, status));
      if (status == 0)
      {
        ZB_SCHEDULE_ALARM(send_active_ep_req_delayed, 0, THR1_DELAY);
      }
      break; /* ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}

/*! @} */
