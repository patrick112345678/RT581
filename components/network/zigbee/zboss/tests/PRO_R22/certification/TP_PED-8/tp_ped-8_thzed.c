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
/* PURPOSE: TP/PED-8 Child Table Management: Make ZED leave by injecting NWK leave
*/

#define ZB_TEST_NAME TP_PED_8_THZED
#define ZB_TRACE_FILE_ID 40773
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_nwk_ed_aging.h"

#include "../common/zb_cert_test_globals.h"
#include "test_common.h"

#ifndef ZB_ED_ROLE
#error define ZB_ED_ROLE to compile ze tests
#endif

static const zb_ieee_addr_t g_ieee_addr_thzed = IEEE_ADDR_TH_ZED;


static void test_change_poll_interval_delayed(zb_uint8_t param);
static void start_fixed_poll(zb_time_t ms);

MAIN()
{
  ARGV_UNUSED;

  ZB_SET_TRAF_DUMP_ON();
  {

    ZB_INIT("zdo_thzed");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

   }

  /* set ieee addr */
  zb_set_long_address(g_ieee_addr_thzed);

#if 0
  zb_set_use_extended_pan_id(g_ext_pan_id);
#endif

  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zed_role();
  zb_set_rx_on_when_idle(ZB_FALSE);

  zb_zdo_set_aps_unsecure_join(ZB_TRUE);
  zdo_set_aging_timeout(ED_AGING_TIMEOUT_10SEC); /* 10 seconds */

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
    case ZB_ZDO_SIGNAL_DEFAULT_START:
      if (status == 0)
      {
        TRACE_MSG(TRACE_APS1, "Device started OK, status %d", (FMT__D, status));
        ZB_SCHEDULE_ALARM(test_change_poll_interval_delayed, 0, ZB_TIME_ONE_SECOND * 12);
        zb_zdo_set_lpd_cmd_timeout(180);
      }
      else
      {
        TRACE_MSG(TRACE_APS1, "Device started, status %d", (FMT__D, status));
      }
      break; /* ZB_ZDO_SIGNAL_DEFAULT_START */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}


static void test_change_poll_interval_delayed(zb_uint8_t param)
{
  ZVUNUSED(param);
  start_fixed_poll(120000);
}

static void start_fixed_poll(zb_time_t ms)
{
  TRACE_MSG(TRACE_ZDO1, ">>start_fixed_poll: new_poll = %ld ms", (FMT__L, ms));

#ifndef NCP_MODE_HOST
  ZDO_CTX().pim.poll_in_progress = ZB_FALSE;
  zb_zdo_pim_stop_poll(0);
  zb_zdo_pim_set_long_poll_interval(ms);
  zb_zdo_pim_permit_turbo_poll(ZB_FALSE); /* prohibit adaptive poll */
  zb_zdo_pim_start_poll(0);
#else
  ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
#endif

  TRACE_MSG(TRACE_ZDO1, "<<start_fixed_poll", (FMT__0));
}
