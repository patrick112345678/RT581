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
/* PURPOSE: TP_R22_SGMB_02 DUT_ZMED
*/

#define ZB_TEST_NAME TP_R22_SGMB_02_DUTZMED
#define ZB_TRACE_FILE_ID 40493

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_nwk_ed_aging.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"

#ifndef ZB_ED_ROLE
#error define ZB_ED_ROLE to compile ze tests
#endif

#ifndef ZB_CERTIFICATION_HACKS
#error Define ZB_CERTIFICATION_HACKS
#endif

static void start_fixed_poll(zb_time_t ms);

static const zb_ieee_addr_t g_ieee_addr_dutzmed = IEEE_ADDR_DUTZMED;

MAIN()
{
  ARGV_UNUSED;

  ZB_INIT("zdo_3_dutzmed");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	
  /* set ieee addr */
  zb_set_long_address(g_ieee_addr_dutzmed);


  /* become an ED */
  zb_cert_test_set_common_channel_settings();
  zb_set_rx_on_when_idle(ZB_FALSE);
  zb_cert_test_set_zed_role();

  zb_zdo_set_aps_unsecure_join(ZB_TRUE);

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
      TRACE_MSG(TRACE_APS1, "Device started, status %d", (FMT__D, status));
      if (status == 0)
      {
	start_fixed_poll(3000);
        zb_zdo_set_lpd_cmd_timeout(6);
      }
      break; /* ZB_ZDO_SIGNAL_DEFAULT_START */

    case ZB_COMMON_SIGNAL_CAN_SLEEP:
      TRACE_MSG(TRACE_APS1, "signal: ZB_COMMON_SIGNAL_CAN_SLEEP, status %d", (FMT__D, status));
      if (status == 0)
      {
#ifdef ZB_USE_SLEEP
    	  zb_sleep_now();
#endif /* ZB_USE_SLEEP */
      }
      break; /* ZB_COMMON_SIGNAL_CAN_SLEEP */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}

static void start_fixed_poll(zb_time_t ms)
{
  TRACE_MSG(TRACE_ZDO1, ">>start_fixed_poll: new_poll = %ld ms", (FMT__P, ms));

#ifndef NCP_MODE_HOST
  zb_cert_test_set_keepalive_mode(MAC_DATA_POLL_KEEPALIVE);
  ZDO_CTX().pim.poll_in_progress = ZB_FALSE;
#else
  ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
#endif

  zb_zdo_pim_stop_poll(0);
  zb_zdo_pim_set_long_poll_interval(ms);
  zb_zdo_pim_permit_turbo_poll(ZB_FALSE); /* prohibit adaptive poll */

  TRACE_MSG(TRACE_ZDO1, "<<start_fixed_poll", (FMT__0));
}
