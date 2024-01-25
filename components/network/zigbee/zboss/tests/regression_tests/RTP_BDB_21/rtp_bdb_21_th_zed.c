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

#define ZB_TEST_NAME RTP_BDB_21_TH_ZED
#define ZB_TRACE_FILE_ID 64003

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"

#include "rtp_bdb_21_common.h"
#include "../common/zb_reg_test_globals.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_ED_ROLE
#error ED role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error define ZB_USE_NVRAM
#endif

#ifndef ZB_STACK_REGRESSION_TESTING_API
#error define ZB_STACK_REGRESSION_TESTING_API
#endif

static zb_ieee_addr_t g_ieee_addr_th_zed = IEEE_ADDR_TH_ZED;

static void trigger_steering(zb_uint8_t unused);

MAIN()
{
  ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
  ZB_SET_TRACE_LEVEL(4);
  ARGV_UNUSED;

  ZB_INIT("zdo_th_zed");

  zb_set_long_address(g_ieee_addr_th_zed);

  zb_reg_test_set_common_channel_settings();
  zb_set_network_ed_role((1l << TEST_CHANNEL));

  zb_set_nvram_erase_at_start(ZB_TRUE);

  zb_set_rx_on_when_idle(ZB_FALSE);

  if (zboss_start_no_autostart() != RET_OK)
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

  TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  switch (sig)
  {
    case ZB_ZDO_SIGNAL_SKIP_STARTUP:
      TRACE_MSG(TRACE_APP1, "signal: ZB_ZDO_SIGNAL_SKIP_STARTUP, status %d", (FMT__D, zb_bdb_is_factory_new()));

      ZB_REGRESSION_TESTS_API().ignore_nwk_key = ZB_TRUE;
      zboss_start_continue();
      break; /* ZB_ZDO_SIGNAL_SKIP_STARTUP */

    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));

      ZB_REGRESSION_TESTS_API().ignore_nwk_key = ZB_FALSE;
      ZB_SCHEDULE_CALLBACK(trigger_steering, 0);
      break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_STEERING:
      TRACE_MSG(TRACE_APP1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
      break; /* ZB_BDB_SIGNAL_STEERING */

    case ZB_COMMON_SIGNAL_CAN_SLEEP:
      TRACE_MSG(TRACE_APP1, "signal: ZB_COMMON_SIGNAL_CAN_SLEEP, status %d", (FMT__D, status));
      if (status == 0)
      {
        zb_sleep_now();
      }
      break; /* ZB_COMMON_SIGNAL_CAN_SLEEP */

    default:
      TRACE_MSG(TRACE_APP1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}

static void trigger_steering(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
}

/*! @} */
