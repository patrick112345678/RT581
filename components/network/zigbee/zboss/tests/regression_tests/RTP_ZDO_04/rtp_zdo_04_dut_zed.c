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

#define ZB_TEST_NAME RTP_ZDO_04_DUT_ZED
#define ZB_TRACE_FILE_ID 40304

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

#include "rtp_zdo_04_common.h"
#include "../common/zb_reg_test_globals.h"

#ifndef ZB_ED_ROLE
#error ED role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error ZB_USE_NVRAM is not compiled!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_dut_zed = IEEE_ADDR_DUT_ZED;

static void trigger_steering(zb_uint8_t unused);

static void test_perform_local_leave_delayed(zb_uint8_t unused);
static void test_perform_local_leave(zb_uint8_t param);
static void test_local_leave_callback(zb_uint8_t param);

MAIN()
{
  ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
  ZB_SET_TRACE_LEVEL(4);
  ARGV_UNUSED;

  ZB_INIT("zdo_dut_zed");

  zb_set_long_address(g_ieee_addr_dut_zed);

  zb_reg_test_set_common_channel_settings();
  zb_set_network_ed_role((1l << TEST_CHANNEL));
  zb_set_rx_on_when_idle(ZB_FALSE);

  zb_set_nvram_erase_at_start(ZB_TRUE);

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

  TRACE_MSG(TRACE_APS1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  switch (sig)
  {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));
      if (status == 0)
      {
        ZB_SCHEDULE_CALLBACK(trigger_steering, 0);
      }
      break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_STEERING:
      TRACE_MSG(TRACE_APP1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
      if (status == 0)
      {
        test_step_register(test_perform_local_leave_delayed, 0, RTP_ZDO_04_STEP_2_TIME_ZED);

        test_control_start(TEST_MODE, RTP_ZDO_04_STEP_1_DELAY_ZED);
      }
      break; /* ZB_BDB_SIGNAL_STEERING */

    case ZB_ZDO_SIGNAL_LEAVE:
      TRACE_MSG(TRACE_APP1, "signal: ZB_ZDO_SIGNAL_LEAVE, status %d", (FMT__D, status));
      break; /* ZB_ZDO_SIGNAL_LEAVE */

    case ZB_COMMON_SIGNAL_CAN_SLEEP:
      TRACE_MSG(TRACE_APS1, "signal: ZB_COMMON_SIGNAL_CAN_SLEEP, status %d", (FMT__D, status));
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

  bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
}

static void test_local_leave_callback(zb_uint8_t param)
{
  ZVUNUSED(param);
  TRACE_MSG(TRACE_APP1, ">>test_local_leave_callback", (FMT__0));
}

static void test_perform_local_leave_delayed(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  zb_buf_get_out_delayed(test_perform_local_leave);
}

static void test_perform_local_leave(zb_uint8_t param)
{
  zb_zdo_mgmt_leave_param_t *req_param;

  TRACE_MSG(TRACE_APP1, ">>test_perform_local_leave", (FMT__0));

  req_param = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_leave_param_t);
  ZB_BZERO(req_param, sizeof(zb_zdo_mgmt_leave_param_t));

  /* Set dst_addr == local address for local leave */
  req_param->dst_addr = ZB_PIBCACHE_NETWORK_ADDRESS();
  req_param->rejoin = ZB_TRUE;
  zdo_mgmt_leave_req(param, test_local_leave_callback);
}

/*! @} */
