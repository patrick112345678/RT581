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
/* PURPOSE: TP/NWK/BV-05 NWK Maximum Depth
Verify that DUT, in position of a router in a larger network interoperability scenario, acts
correctly after joining at maximum depth (Pro Max depth=15; OK to join at depth 0xF)
*/

#define ZB_TEST_NAME TP_NWK_BV_05_ZR9
#define ZB_TRACE_FILE_ID 40766

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"


MAIN()
{
  ARGV_UNUSED;

  ZB_INIT("zdo_zr9");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	
  /* set ieee addr */
  zb_set_long_address(g_ieee_addr_r9);
  //MAC_ADD_VISIBLE_LONG((zb_uint8_t*) g_ieee_addr_r8);
  //MAC_ADD_VISIBLE_LONG((zb_uint8_t*) g_ieee_addr_r10);

  /* join as a router */
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zr_role();

  ZB_CERT_HACKS().extended_beacon_send_jitter = ZB_TRUE;

  /* accept only one child */
  zb_set_max_children(1);

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


static zb_uint16_t addr_ass_cb(zb_ieee_addr_t ieee_addr)
{
  ZVUNUSED(ieee_addr);
  /* fix address for next ZR */
  return 0x000A;
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  TRACE_MSG(TRACE_ERROR, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  switch(sig)
  {
    case ZB_ZDO_SIGNAL_DEFAULT_START:
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ZB_BDB_SIGNAL_DEVICE_REBOOT:
      TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));
      if (status == 0)
      {
        ZB_SCHEDULE_CALLBACK(test_after_startup_action, 0);
      }
      else
      {

      }
      break;

      default:
        TRACE_MSG(TRACE_ERROR, "Unknown signal %hd", (FMT__H, sig));
  }

  zb_buf_free(param);

  zb_nwk_set_address_assignment_cb(addr_ass_cb);
}
