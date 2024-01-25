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
/* PURPOSE: TH ZED
*/

#define ZB_TEST_NAME RTP_BDB_11_DUT_ZED

#define ZB_TRACE_FILE_ID 40325
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"

#include "rtp_bdb_11_common.h"
#include "../common/zb_reg_test_globals.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_ED_ROLE
#error ED role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error ZB_USE_NVRAM is not compiled!
#endif

#ifndef ZB_REGRESSION_TESTS_API
#error define ZB_REGRESSION_TESTS_API
#endif

static zb_ieee_addr_t g_ieee_addr_dut_zed = IEEE_ADDR_DUT_ZED;

static void trigger_steering(zb_uint8_t unused);

MAIN()
{
  ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
  ZB_SET_TRACE_LEVEL(4);
  ARGV_UNUSED;

  ZB_INIT("zdo_dut_zed");

  zb_set_long_address(g_ieee_addr_dut_zed);

  zb_reg_test_set_common_channel_settings();
  zb_set_network_ed_role(1l << TEST_CHANNEL);
  zb_set_nvram_erase_at_start(ZB_TRUE);
  zb_set_rx_on_when_idle(ZB_FALSE);

  /*
   * Regression tests api is needed here as zed tries association with zc first according to bz ticket.
   * Just ignore order of received beacons and their lqi.
   */
  ZB_REGRESSION_TESTS_API().enable_custom_best_parent = ZB_TRUE;
  ZB_REGRESSION_TESTS_API().set_short_custom_best_parent = 0x0000;

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

    case ZB_ZDO_SIGNAL_LEAVE:
      TRACE_MSG(TRACE_APS1, "signal: ZB_ZDO_SIGNAL_LEAVE, status %d", (FMT__D, status));
      break; /* ZB_ZDO_SIGNAL_LEAVE */

    case ZB_BDB_SIGNAL_STEERING:
      TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
      break; /* ZB_BDB_SIGNAL_STEERING */

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

  if (!bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING))
  {
    TRACE_MSG(TRACE_ERROR, "trigger_steering(): test FAILED", (FMT__0));
  }
}

/*! @} */
