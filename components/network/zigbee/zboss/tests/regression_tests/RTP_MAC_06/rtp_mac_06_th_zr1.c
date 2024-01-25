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

#define ZB_TEST_NAME RTP_MAC_06_TH_ZR1
#define ZB_TRACE_FILE_ID 40345

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
#include "zb_mac_globals.h"
#include "zb_zcl.h"

#include "rtp_mac_06_common.h"
#include "../common/zb_reg_test_globals.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error ZB_USE_NVRAM is not compiled!
#endif


/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_th_zr1 = IEEE_ADDR_TH_ZR1;

static void test_trigger_rejoin(zb_uint8_t unused);

/************************Main*************************************/
MAIN()
{
  ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
  ZB_SET_TRACE_LEVEL(4);
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_th_zr1");


  zb_set_long_address(g_ieee_addr_th_zr1);

  zb_reg_test_set_zr_role();
  zb_reg_test_set_common_channel_settings();
  zb_set_nvram_erase_at_start(ZB_TRUE);

  zb_set_max_children(0);

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

/********************ZDO Startup*****************************/
ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);
  zb_bool_t rejoin_backoff_running = zb_zdo_rejoin_backoff_is_running();

  TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  switch (sig)
  {
    case ZB_ZDO_SIGNAL_DEFAULT_START:
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ZB_BDB_SIGNAL_DEVICE_REBOOT:
      TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));
      if (status == 0)
      {
        if (rejoin_backoff_running)
        {
          zb_zdo_rejoin_backoff_cancel();
        }
        else
        {
          /* Start rejoin and reset to defaults */
          ZB_SCHEDULE_ALARM(test_trigger_rejoin, 0, TEST_TH_ZR1_REJOIN_TIMEOUT);
        }
      }

      break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_STEERING:
      TRACE_MSG(TRACE_APP1, "signal: ZB_BDB_SIGNAL_STEERING, status %d, rejoin backoff %hd", (FMT__D_D, status, rejoin_backoff_running));
      break;

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}

static void test_trigger_rejoin(zb_uint8_t unused)
{
  zb_address_ieee_ref_t zero_addr_ref;
  ZVUNUSED(unused);

  TRACE_MSG(TRACE_APP1, "test_trigger_rejoin()", (FMT__0));

  zb_set_long_address(g_zero_addr);

  /* ZC will send Network Status command about address conflict after receiving association request,
   * so ZR will try to resolve it and change address.
   * So, addresses table should contain entry with zero address. ZR should be able to find
   * valid reference to this zero address */
  (void)zb_address_by_ieee(g_zero_addr, ZB_TRUE, ZB_FALSE, &zero_addr_ref);

  /* Trigger rejoin */
  zb_zdo_rejoin_backoff_start(ZB_FALSE);
}

/*! @} */
