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
/* PURPOSE: TP_PED_1 gZED
*/

#define ZB_TEST_NAME TP_PED_1_GZED
#define ZB_TRACE_FILE_ID 40904

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"


#ifndef ZB_ED_ROLE
#error define ZB_ED_ROLE to compile ze tests
#endif

static const zb_ieee_addr_t g_ieee_addr_gzed = IEEE_ADDR_gZED;

MAIN()
{
  ARGV_UNUSED;

  ZB_INIT("zdo_2_gzed");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

  /* set ieee addr */
  zb_set_long_address(g_ieee_addr_gzed);

  /* become an ED */
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zed_role();
  zb_set_rx_on_when_idle(ZB_FALSE);

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

  if (status == 0)
  {
    switch (sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
	TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
	break; /* ZB_ZDO_SIGNAL_DEFAULT_START */

      case ZB_COMMON_SIGNAL_CAN_SLEEP:
#ifdef ZB_USE_SLEEP
    	  zb_sleep_now();
#endif /* ZB_USE_SLEEP */
        break;

      default:
	TRACE_MSG(TRACE_ERROR, "Unknown signal %hd", (FMT__H, sig));
	break;
    }
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, status));
  }

  zb_buf_free(param);
}

