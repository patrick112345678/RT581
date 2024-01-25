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

#define ZB_TEST_NAME TP_BDB_FB_PRE_TC_03B_THE1
#define ZB_TRACE_FILE_ID 40772

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

#ifndef ZB_ED_ROLE
#error ED role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error ZB_USE_NVRAM is not compiled!
#endif


/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_the1 = IEEE_ADDR_THE1;

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(fb_pre_tc_03b_the1_basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(fb_pre_tc_03b_the1_identify_attr_list, &attr_identify_time);

static zb_bool_t attr_on_off = 1;
ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(fb_pre_tc_03b_the1_on_off_attr_list, &attr_on_off);


/********************* Declare device **************************/
DECLARE_TARGET_CLUSTER_LIST(fb_pre_tc_03b_the1_target_device_clusters,
                            fb_pre_tc_03b_the1_basic_attr_list,
                            fb_pre_tc_03b_the1_identify_attr_list,
                            fb_pre_tc_03b_the1_on_off_attr_list);

DECLARE_TARGET_EP(fb_pre_tc_03b_the1_target_device_ep, THE1_ENDPOINT, fb_pre_tc_03b_the1_target_device_clusters);

DECLARE_TARGET_NO_REP_CTX(fb_pre_tc_03b_the1_target_device_ctx, fb_pre_tc_03b_the1_target_device_ep);


/**********************General definitions for test***********************/


enum match_desc_options_e
{
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

static int s_step_idx;

MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_the1");


  zb_set_long_address(g_ieee_addr_the1);

  zb_set_network_ed_role((1l << TEST_CHANNEL));
  zb_set_nvram_erase_at_start(ZB_TRUE);

  zb_set_rx_on_when_idle(ZB_FALSE);
  zb_set_keepalive_timeout(ZB_MILLISECONDS_TO_BEACON_INTERVAL(2000));
  /* zb_aib_set_trust_center_address(g_unknown_ieee_addr); */

  ZB_AF_REGISTER_DEVICE_CTX(&fb_pre_tc_03b_the1_target_device_ctx);

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
  zb_bdb_finding_binding_target(THE1_ENDPOINT);

  TRACE_MSG(TRACE_APP1, "<<trigger_fb_target", (FMT__0));
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
        TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));

        ZB_SCHEDULE_ALARM(trigger_fb_target, 0, THE1_FB_FIRST_START_DELAY);
      }
      else
      {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, status));
      }
      break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED:
      if (status == 0)
      {
        TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED, status OK", (FMT__0));
        if (BDB_COMM_CTX().state == ZB_BDB_COMM_IDLE)
        {
          s_step_idx++;
          TRACE_MSG(TRACE_APP1, "s_step_idx = %d", (FMT__D, s_step_idx));

          if (s_step_idx < TEST_OPT_TWO_RESPONSES_3)
          {
            ZB_SCHEDULE_ALARM(trigger_fb_target, 0, THE1_FB_START_DELAY);
          }
          else if (s_step_idx == TEST_OPT_TWO_RESPONSES_3)
          {
            ZB_SCHEDULE_ALARM(trigger_fb_target, 0, (2 * THE1_FB_START_DELAY));
          }
          else
          {
            TRACE_MSG(TRACE_APP1, "Test finished", (FMT__0));
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
