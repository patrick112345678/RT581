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
/* PURPOSE: TH ZC
*/

#define ZB_TEST_NAME RTP_BDB_NOP_13_TH_ZC
#define ZB_TRACE_FILE_ID 40259

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"

#include "rtp_bdb_nop_13_common.h"
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

static zb_uint8_t g_nwk_key[16] = ZB_REG_TEST_DEFAULT_NWK_KEY;
static zb_ieee_addr_t g_ieee_addr_th_zc = IEEE_ADDR_TH_ZC;
static zb_ieee_addr_t g_ieee_addr_dut_zr = IEEE_ADDR_DUT_ZR;

static void trigger_steering(zb_uint8_t unused);
static void test_zboss_main_loop_stop(zb_uint8_t unused);

MAIN()
{

  ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
  ZB_SET_TRACE_LEVEL(4);

  ARGV_UNUSED;

  ZB_INIT("zdo_th_zc");

  zb_set_long_address(g_ieee_addr_th_zc);

zb_set_pan_id(0x1aaa);

  zb_secur_setup_nwk_key((zb_uint8_t*) g_nwk_key, 0);

  zb_reg_test_set_common_channel_settings();
  zb_set_network_coordinator_role((1l << TEST_CHANNEL));
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

static void trigger_steering(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
  zb_zdo_app_signal_hdr_t *sg_p;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

  TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  switch (sig)
  {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      TRACE_MSG(TRACE_APP1, "signal: ZB_BDB_SIGNAL_DEVICE_FIRST_START, status %d", (FMT__D, status));
      if (status == 0)
      {
        ZB_SCHEDULE_CALLBACK(trigger_steering, 0);
      }
      break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED:
      TRACE_MSG(TRACE_APP1, "signal: ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED, status %d", (FMT__D, status));
      if (status == 0)
      {
        zb_zdo_signal_device_authorized_params_t *dev =
          ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_authorized_params_t);

        if (ZB_IEEE_ADDR_CMP(dev->long_addr, g_ieee_addr_dut_zr))
        {
          ZB_SCHEDULE_ALARM(test_zboss_main_loop_stop, 0, (ZB_TIME_ONE_SECOND));
        }
      }
      break; /* ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED */

    default:
      TRACE_MSG(TRACE_APP1, "Unknown signal %d, status %d", (FMT__D_D, sig, status));
      break;
  }

  zb_buf_free(param);
}

static void test_zboss_main_loop_stop(zb_uint8_t unused)
{
  TRACE_MSG(TRACE_APP1, ">>test_zboss_main_loop_stop", (FMT__0));

  ZVUNUSED(unused);

  ZG->sched.stop = ZB_TRUE;

  osif_sleep_using_transc_timer(TH_SLEEP_TIME);

  TRACE_MSG(TRACE_APP1, "test_zboss_main_loop_stop(): main loop started", (FMT__0));

  ZG->sched.stop = ZB_FALSE;
}

/*! @} */
