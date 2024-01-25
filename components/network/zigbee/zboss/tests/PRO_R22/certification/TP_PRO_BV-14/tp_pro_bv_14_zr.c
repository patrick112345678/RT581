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
/* PURPOSE: ZR
*/

#define ZB_TEST_NAME TP_PRO_BV_14_ZR
#define ZB_TRACE_FILE_ID 40639

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "test_common.h"

#include "../nwk/nwk_internal.h"
#include "../common/zb_cert_test_globals.h"

static const zb_ieee_addr_t g_ieee_addr_zr = IEEE_ADDR_ZR;
MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("zdo_2_zr");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	
  /* set ieee addr */
  zb_set_long_address(g_ieee_addr_zr);

  /* join as a router */
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zr_role();

  /* turn off security */
  /* zb_cert_test_set_security_level(0); */

  zb_set_max_children(0);

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


static void send_address_conflict(zb_uint8_t param)
{
  zb_bufid_t buf = zb_buf_get_out();

  ZVUNUSED(param);

  TRACE_MSG(TRACE_APS3, ">>Sending address conflict message", (FMT__0));

  if (buf)
  {
    zb_nlme_send_status_t *request = ZB_BUF_GET_PARAM(buf, zb_nlme_send_status_t);

    request->dest_addr           = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
    request->status.status       = ZB_NWK_COMMAND_STATUS_ADDRESS_CONFLICT;
    request->status.network_addr = 0x0000;
    request->ndsu_handle         = ZB_NWK_INTERNAL_NSDU_HANDLE;

    ZB_SCHEDULE_ALARM(zb_nlme_send_status, buf, ZB_NWK_OCTETS_TO_BI(ZB_NWKC_MAX_BROADCAST_JITTER_OCTETS));
  }
  else
  {
    TRACE_MSG(TRACE_APS3, "TEST FAILED: Could not get out buf!", (FMT__0));
  }

  TRACE_MSG(TRACE_APS3, "<<Sending address conflict message", (FMT__0));
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  TRACE_MSG(TRACE_ERROR, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  if (0 == status)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));

	ZB_SCHEDULE_ALARM(send_address_conflict, 0, 5*ZB_TIME_ONE_SECOND);
        break;

      default:
        TRACE_MSG(TRACE_ERROR, "Unknown signal %hd", (FMT__H, sig));
    }
  }
  else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
  {
    TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, status));
  }

  zb_buf_free(param);
}
