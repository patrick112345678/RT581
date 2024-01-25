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
/* PURPOSE:
*/

#define ZB_TEST_NAME TP_R22_CCB_2469_DUTZED
#define ZB_TRACE_FILE_ID 40518

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

static zb_uint8_t g_is_first_start = ZB_TRUE;

static void test_add_group_request(zb_uint8_t unused);

static const zb_ieee_addr_t g_ieee_addr_dutzed = IEEE_ADDR_DUT_ZED;

MAIN()
{
  ARGV_UNUSED;

  ZB_INIT("zdo_dutzed");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	
  /* set ieee addr */
  zb_set_long_address(g_ieee_addr_dutzed);

  /* become an ED */
  zb_cert_test_set_common_channel_settings();
  zb_set_rx_on_when_idle(ZB_FALSE);
  zb_cert_test_set_zed_role();
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
        if (g_is_first_start)
        {
          g_is_first_start = ZB_FALSE;

          ZB_SCHEDULE_CALLBACK(test_add_group_request, 0);
        }
      }
      break; /* ZB_ZDO_SIGNAL_DEFAULT_START */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}

static void test_add_group_request(zb_uint8_t unused)
{
  zb_bufid_t req = zb_buf_get_out();
  zb_apsme_add_group_req_t *req_param = ZB_BUF_GET_PARAM(req, zb_apsme_add_group_req_t);
  ZB_BZERO(req_param, sizeof(*req_param));

  ZVUNUSED(unused);

  req_param->group_address = GROUP_ADDR;
  req_param->endpoint = GROUP_EP;
  /* Need to disable ZB_ENABLE_ZCL to be able add entry in group table with nonregistered endpoint id */
  zb_apsme_add_group_request(req);
}
