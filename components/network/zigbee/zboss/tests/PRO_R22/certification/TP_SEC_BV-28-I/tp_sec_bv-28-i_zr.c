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
/* PURPOSE: 11.28 TP/SEC/BV-28-I Security Remove Device (ZR)
Objective: DUT as ZR correctly handles APS Remove device and ZDO Mgmt_Leave_req.
*/

#define ZB_TEST_NAME TP_SEC_BV_28_I_ZR
#define ZB_TRACE_FILE_ID 40557
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

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_CERTIFICATION_HACKS
#error Define ZB_CERTIFICATION_HACKS
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static const zb_ieee_addr_t g_ieee_addr_c = IEEE_ADDR_C;
static const zb_ieee_addr_t g_ieee_addr_r = IEEE_ADDR_R;
static const zb_ieee_addr_t g_ieee_addr_ed1 = IEEE_ADDR_ED1;
static const zb_ieee_addr_t g_ieee_addr_ed2 = IEEE_ADDR_ED2;

static zb_bool_t is_first_start = ZB_TRUE;

static void send_request_key(zb_uint8_t param);

MAIN()
{
  ARGV_UNUSED;


  ZB_INIT("zdo_2_zr");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif


  zb_set_long_address(g_ieee_addr_r);
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zr_role();

  MAC_ADD_VISIBLE_LONG((zb_uint8_t*) g_ieee_addr_ed1);
  MAC_ADD_VISIBLE_LONG((zb_uint8_t*) g_ieee_addr_ed2);
  MAC_ADD_VISIBLE_LONG((zb_uint8_t*) g_ieee_addr_c);

  zb_set_max_children(2);

  zb_zdo_set_aps_unsecure_join(INSECURE_JOIN_ZR);

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
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

  TRACE_MSG(TRACE_APP1, "zboss_signal_handler: status %hd signal %hd",
            (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));

  switch (sig)
  {
    case ZB_ZDO_SIGNAL_DEFAULT_START:
      if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
      {
	TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));

	if (is_first_start)
	{
          ZB_SCHEDULE_ALARM(send_request_key, 0, ZB_TIME_ONE_SECOND);
	  is_first_start = ZB_FALSE;
	}
      }
      else
      {
	TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d",
		  (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
      }
      break; /* ZB_ZDO_SIGNAL_DEFAULT_START */

    default:
      if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
      {
	TRACE_MSG(TRACE_APS1, "zboss_signal_handler: status OK, status %d",
		  (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
      }
      else
      {
	TRACE_MSG(TRACE_ERROR, "zboss_signal_handler: status FAILED, status %d",
		  (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
      }
      break;
  }

  zb_buf_free(param);
}

static void send_request_key(zb_uint8_t param)
{
  zb_bufid_t buf = zb_buf_get_out();

  ZVUNUSED(param);

  TRACE_MSG(TRACE_ERROR, ">>send_request_key", (FMT__0));

  if (buf)
  {
    zb_apsme_request_key_req_t *req_param;

    req_param = ZB_BUF_GET_PARAM(buf, zb_apsme_request_key_req_t);

    ZB_IEEE_ADDR_COPY(req_param->dest_address, &g_ieee_addr_c);
    req_param->key_type = ZB_REQUEST_TC_LINK_KEY;

    zb_secur_apsme_request_key(buf);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "TEST FAILED: Could not get out buf!", (FMT__0));
  }

  TRACE_MSG(TRACE_ERROR, "<<send_request_key", (FMT__0));
}

/*! @} */
