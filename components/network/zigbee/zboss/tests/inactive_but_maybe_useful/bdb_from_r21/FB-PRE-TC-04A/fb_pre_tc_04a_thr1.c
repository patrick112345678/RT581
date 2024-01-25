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


#define ZB_TEST_NAME FB_PRE_TC_04A_THR1
#define ZB_TRACE_FILE_ID 41178
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
#include "fb_pre_tc_04a_common.h"
#include "test_initiator.h"


#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
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

static zb_uint8_t attr_name_support = 0;
ZB_ZCL_DECLARE_GROUPS_ATTRIB_LIST(group_attr_list, &attr_name_support);


/********************* Declare device **************************/
DECLARE_INITIATOR_CLUSTER_LIST(initiator_device_clusters,
                               basic_attr_list,
                               identify_attr_list);

DECLARE_INITIATOR_EP(initiator_device_ep, THR1_ENDPOINT, initiator_device_clusters);

DECLARE_INITIATOR_CTX(initiator_device_ctx, initiator_device_ep);


/**********************General definitions for test***********************/

enum match_desc_options_e
{
  TEST_OPT_HA,
  TEST_OPT_LEGACY_0101,
  TEST_OPT_LEGACY_0102,
  TEST_OPT_LEGACY_0103,
  TEST_OPT_LEGACY_0105,
  TEST_OPT_LEGACY_0106,
  TEST_OPT_LEGACY_0107,
  TEST_OPT_LEGACY_0108,
  TEST_OPT_LEGACY_C05E,
  TEST_OPT_WILDCARD,
  TEST_OPT_BROADCAST,
  TEST_OPT_2_EP_DIFFERENT_CLUSTERS,
  TEST_OPT_2_EP_SAME_CLUSTERS,
  TEST_OPT_COUNT
};
static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster);

static void trigger_fb_initiator(zb_uint8_t unused);


static void send_match_desc_loop(zb_uint8_t id);
static void send_match_desc_req(zb_uint8_t param, zb_uint16_t options);
static void match_desc_resp_cb(zb_uint8_t param);


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

#ifdef ZB_USE_NVRAM
  ZB_AIB().aps_use_nvram = 1;
#endif
  
  ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ROUTER;
  ZB_IEEE_ADDR_COPY(ZB_AIB().trust_center_address, g_unknown_ieee_addr);
  zb_secur_setup_nwk_key(g_nwk_key, 0);

  ZB_AF_REGISTER_DEVICE_CTX(&initiator_device_ctx);

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


static void send_match_desc_loop(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  TRACE_MSG(TRACE_ZDO1, ">>send_match_desc_loop: step = %d", (FMT__D, s_step_idx));

  if (s_step_idx < TEST_OPT_COUNT)
  {
    ZB_GET_OUT_BUF_DELAYED2(send_match_desc_req, s_step_idx);
  }

  TRACE_MSG(TRACE_ZDO1, "<<send_match_desc_loop", (FMT__0));  
}


static void send_match_desc_req(zb_uint8_t param, zb_uint16_t options)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
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
    case TEST_OPT_BROADCAST:
      addr = 0xfffd;
      break;
  }

  TRACE_MSG(TRACE_ZDO1, ">>send_match_desc_req: param = %d, addr = 0x%x, profile_id = 0x%x",
            (FMT__D_H_H, param, addr, profile_id));

  ZB_BUF_INITIAL_ALLOC(buf, size, req);
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
        req->cluster_list[0] = ZB_ZCL_CLUSTER_ID_BASIC;
        req->cluster_list[1] = ZB_ZCL_CLUSTER_ID_COLOR_CONTROL;
        req->cluster_list[2] = ZB_ZCL_CLUSTER_ID_PUMP_CONFIG_CONTROL;
        /* out clusters */
        req->cluster_list[3] = ZB_ZCL_CLUSTER_ID_IAS_ZONE;
        req->cluster_list[4] = ZB_ZCL_CLUSTER_ID_IAS_ACE;
        req->cluster_list[5] = ZB_ZCL_CLUSTER_ID_IAS_WD;
      }
      break;

    case TEST_OPT_2_EP_DIFFERENT_CLUSTERS:
      {
        req->cluster_list[0] = ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT;
      }
      break;

    case TEST_OPT_2_EP_SAME_CLUSTERS:
      {
        req->cluster_list[0] = ZB_ZCL_CLUSTER_ID_ON_OFF;
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
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);

  TRACE_MSG(TRACE_ZDO1, ">>match_desc_resp_cb: param = 0x%x", (FMT__D, param));

  zb_free_buf(buf);
  ++s_step_idx;
  ZB_SCHEDULE_ALARM(send_match_desc_loop, 0, THR1_SHORT_DELAY);

  TRACE_MSG(TRACE_ZDO1, "<<match_desc_resp_cb", (FMT__0));
}


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
          ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, THR1_FB_START_DELAY);
        }
        break;

      case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APS1, "Network steering", (FMT__0));
        ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, THR1_FB_START_DELAY);
        break;

      case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:
        TRACE_MSG(TRACE_APS1, "Finding&binding done", (FMT__0));
        ZB_SCHEDULE_ALARM(send_match_desc_loop, 0, THR1_DELAY);
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
