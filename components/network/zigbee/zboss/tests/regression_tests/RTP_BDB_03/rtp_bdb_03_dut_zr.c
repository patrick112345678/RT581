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
/* PURPOSE: DUT ZR
*/

#define ZB_TEST_NAME RTP_BDB_03_DUT_ZR

#define ZB_TRACE_FILE_ID 40442
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"

#include "rtp_bdb_03_common.h"
#include "../common/zb_reg_test_globals.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */


#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error define ZB_USE_NVRAM
#endif

static zb_ieee_addr_t g_ieee_addr_dut_zr = IEEE_ADDR_DUT_ZR;

static void test_trigger_steering(zb_uint8_t unused);
static void trigger_steering(zb_uint8_t unused);
static void test_bdb_reset(zb_uint8_t div);

static zb_bool_t   g_test_steering_in_progress = ZB_FALSE;
static zb_bool_t   g_test_delayed_reset = ZB_FALSE;
static zb_uint8_t  g_test_step = 0;
static zb_uint16_t g_test_reset_timeout_ms[TEST_RESET_COUNT] =
{
  TEST_RESET_TIMEOUT_1_MS,
  TEST_RESET_TIMEOUT_2_MS,
  TEST_RESET_TIMEOUT_3_MS,
  TEST_RESET_TIMEOUT_4_MS,
  TEST_RESET_TIMEOUT_5_MS,
  TEST_RESET_TIMEOUT_6_MS,
  TEST_RESET_TIMEOUT_7_MS,
  TEST_RESET_TIMEOUT_8_MS
};

MAIN()
{

  ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
  ZB_SET_TRACE_LEVEL(4);

  ARGV_UNUSED;

  ZB_INIT("zdo_dut_zr");

  zb_set_long_address(g_ieee_addr_dut_zr);

  zb_reg_test_set_common_channel_settings();
  zb_set_network_router_role((1l << TEST_CHANNEL));
  zb_set_nvram_erase_at_start(ZB_TRUE);

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
      TRACE_MSG(TRACE_APP1, "signal: ZB_ZDO_SIGNAL_SKIP_STARTUP, status %d", (FMT__D, status));
      if (status == 0)
      {
        /* Firs start. Call steering and reset after timeout. */
        ZB_SCHEDULE_CALLBACK(test_trigger_steering, 0);
      }
      break; /* ZB_ZDO_SIGNAL_SKIP_STARTUP */

    case ZB_BDB_SIGNAL_STEERING:
      TRACE_MSG(TRACE_APP1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
      /* Do reset to factory if required. If this flag is false, reset will be called
       * later according to the test procedure. */
      if (g_test_delayed_reset)
      {
        TRACE_MSG(TRACE_APP1, "reset to factory is required", (FMT__0));
        g_test_delayed_reset = !g_test_delayed_reset;
        ZB_SCHEDULE_CALLBACK(test_bdb_reset, 0);
      }

      /* Steering is not in progress now. */
      g_test_steering_in_progress = ZB_FALSE;

      /*
       * As other beacons with association permit = False can be received during the
       * last association in the RTP_BDB_NOP_03, try to associate while status of steering
       * is not success
       */
      if (g_test_step > TEST_RESET_COUNT && status != 0)
      {
        ZB_SCHEDULE_CALLBACK(trigger_steering, 0);
      }
      break; /* ZB_BDB_SIGNAL_STEERING */

    case ZB_ZDO_SIGNAL_LEAVE:
      TRACE_MSG(TRACE_APP1, "signal: ZB_ZDO_SIGNAL_LEAVE, status %d", (FMT__D, status));
      if (status == 0)
      {
        /* Call the new test step. */
        ZB_SCHEDULE_ALARM(test_trigger_steering, 0, TEST_NEXT_STEP_TIMEOUT_BI);
      }
      break; /* ZB_ZDO_SIGNAL_LEAVE */

    default:
      TRACE_MSG(TRACE_APP1, "Unknown signal %d, status %d", (FMT__D_D, sig, status));
      break;
  }

  zb_buf_free(param);
}

/*
 * 1. Trigger bdb top level commissioning if test counter is less then TEST_RESET_COUNT
 * 2. Reset after timeout required for the test
 */
static void test_trigger_steering(zb_uint8_t unused)
{
  zb_time_t reset_tmo;

  ZVUNUSED(unused);

  TRACE_MSG(TRACE_APP1, "test_trigger_steering(), step %hd",
            (FMT__H, g_test_step));

  if (g_test_step < TEST_RESET_COUNT)
  {
    TRACE_MSG(TRACE_APP1, "test_trigger_steering(), timeout %hd",
              (FMT__D, g_test_reset_timeout_ms[g_test_step]));

    g_test_steering_in_progress = ZB_TRUE;

    reset_tmo = ZB_MILLISECONDS_TO_BEACON_INTERVAL(g_test_reset_timeout_ms[g_test_step]);

    /* Trigger steering */
    ZB_SCHEDULE_CALLBACK(trigger_steering, 0);

    /* Perform reset after steering with a timeout */
    ZB_SCHEDULE_ALARM(test_bdb_reset, 0, reset_tmo);
  }
  else
  {
    /* Trigger steering last time to confirm that we can join to PAN */
    ZB_SCHEDULE_CALLBACK(trigger_steering, 0);

    TRACE_MSG(TRACE_ERROR, "test_trigger_steering(): test finished", (FMT__0));
  }

  g_test_step++;
}

static void trigger_steering(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  TRACE_MSG(TRACE_APP1, "trigger_steering()", (FMT__0));

  bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
}

static void test_bdb_reset(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  TRACE_MSG(TRACE_APP1, "test_bdb_reset()", (FMT__0));

  if (!g_test_steering_in_progress)
  {
    zb_bdb_reset_via_local_action(0);
  }
  else
  {
    TRACE_MSG(TRACE_APP1, "test_bdb_reset(), reset is not allowed", (FMT__0));
    g_test_delayed_reset = ZB_TRUE;
  }
}

/*! @} */
