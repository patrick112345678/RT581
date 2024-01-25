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
/* PURPOSE: DUT ZED
*/

#define ZB_TEST_NAME RTP_BDB_18_DUT_ZED

#define ZB_TRACE_FILE_ID 63987
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"

#include "rtp_bdb_18_common.h"
#include "device_dut.h"

#include "../common/zb_reg_test_globals.h"


/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_ED_ROLE
#error ED role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error ZB_USE_NVRAM is not compiled!
#endif

static zb_ieee_addr_t g_ieee_addr_dut_zed = IEEE_ADDR_DUT_ZED;

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(rtp_bdb_18_dut_zed_basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(rtp_bdb_18_dut_zed_identify_attr_list, &attr_identify_time);

/********************* Declare device **************************/
DECLARE_DUT_CLUSTER_LIST(rtp_bdb_18_dut_zed_device_clusters,
                        rtp_bdb_18_dut_zed_basic_attr_list,
                        rtp_bdb_18_dut_zed_identify_attr_list);

DECLARE_DUT_EP(rtp_bdb_18_dut_zed_device_ep,
              DUT_ENDPOINT,
              rtp_bdb_18_dut_zed_device_clusters);

DECLARE_DUT_CTX(rtp_bdb_18_dut_zed_device_ctx, rtp_bdb_18_dut_zed_device_ep);

static void trigger_steering(zb_uint8_t unused);
static void trigger_fb_target(zb_uint8_t unused);
static void perform_reset(zb_uint8_t unused);

MAIN()
{
  ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
  ZB_SET_TRACE_LEVEL(4);

  ARGV_UNUSED;

  ZB_INIT("zdo_dut_zed");

  zb_set_long_address(g_ieee_addr_dut_zed);

  zb_reg_test_set_common_channel_settings();
  zb_set_network_ed_role((1l << TEST_CHANNEL));
  zb_set_nvram_erase_at_start(ZB_TRUE);
  zb_set_rx_on_when_idle(ZB_FALSE);

  ZB_AF_REGISTER_DEVICE_CTX(&rtp_bdb_18_dut_zed_device_ctx);

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

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  switch (sig)
  {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));
      if (status == 0)
      {
        test_step_register(trigger_steering, 0, RTP_BDB_18_STEP_1_TIME_ZED);
        test_step_register(trigger_fb_target, 0, RTP_BDB_18_STEP_2_TIME_ZED);
        test_step_register(perform_reset, 0, RTP_BDB_18_STEP_3_TIME_ZED);
        test_step_register(trigger_steering, 0, RTP_BDB_18_STEP_4_TIME_ZED);

        test_control_start(TEST_MODE, RTP_BDB_18_STEP_1_DELAY_ZED);
      }
      break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_STEERING:
      TRACE_MSG(TRACE_APP1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
      break; /* ZB_BDB_SIGNAL_STEERING */

    case ZB_COMMON_SIGNAL_CAN_SLEEP:
      if (status == 0)
      {
        zb_sleep_now();
      }
      break; /* ZB_COMMON_SIGNAL_CAN_SLEEP */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}

static void trigger_steering(zb_uint8_t unused)
{
  ZVUNUSED(unused);
  TRACE_MSG(TRACE_APP1, "Trigger steering", (FMT__0));
  bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
}

static void trigger_fb_target(zb_uint8_t unused)
{
  ZVUNUSED(unused);
  TRACE_MSG(TRACE_APP1, "Trigger F&B target", (FMT__0));
  zb_bdb_finding_binding_target(DUT_ENDPOINT);
}

static void perform_reset(zb_uint8_t unused)
{
  ZVUNUSED(unused);
  TRACE_MSG(TRACE_APP1, "Perform reset", (FMT__0));
  zb_bdb_reset_via_local_action(0);
}

/*! @} */
