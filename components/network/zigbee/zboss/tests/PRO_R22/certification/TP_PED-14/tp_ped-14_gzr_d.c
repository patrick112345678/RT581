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
/* PURPOSE: TP/PED-14 Child Table Management: Parent Announcement – Splitting & Randomization
(gZR distributed network)
*/

#define ZB_TEST_NAME TP_PED_14_GZR_D
#define ZB_TRACE_FILE_ID 40913

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

static const zb_ieee_addr_t g_ieee_addr_gzr = IEEE_ADDR_gZR;

MAIN()
{
  ARGV_UNUSED;

  ZB_INIT("zdo_2_gzr");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

  zb_set_long_address(g_ieee_addr_gzr);
  zb_aib_set_trust_center_address(g_unknown_ieee_addr);
  zb_zdo_set_aps_unsecure_join(ZB_TRUE);
  zb_set_max_children(1);

  zb_set_pan_id(TEST_PAN_ID);
  zb_cert_test_set_network_addr(0x2bbb);
  zb_secur_setup_nwk_key((zb_uint8_t*) g_nwk_key, 0);
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zr_role();

#ifdef SECURITY_LEVEL
  zb_cert_test_set_security_level(SECURITY_LEVEL);
#endif

  zb_set_nvram_erase_at_start(ZB_TRUE);

  if (zboss_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
  }
  else
  {
    zboss_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}

static void reopen_network(zb_bufid_t bufid)
{
  ZVUNUSED(bufid);

  TRACE_MSG(TRACE_APS1, "Reopen the Zigbee network", (FMT__0));
  bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
  ZB_SCHEDULE_APP_ALARM(reopen_network, 0, ZB_TIME_ONE_SECOND * 180);
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  switch (sig)
  {
    case ZB_ZDO_SIGNAL_DEFAULT_START:
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ZB_BDB_SIGNAL_DEVICE_REBOOT:
      if (status == 0)
      {
        TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
        ZB_SCHEDULE_APP_ALARM(reopen_network, 0, ZB_TIME_ONE_SECOND * 180);
      }
      else
      {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, status));
      }
      break; /* ZB_ZDO_SIGNAL_DEFAULT_START */

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

