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
/* PURPOSE: TP_R21_BV-16 DUT gZED2
*/

#define ZB_TEST_NAME TP_PED_3_ZED2
#define ZB_TRACE_FILE_ID 40564

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

static const zb_ieee_addr_t g_ieee_addr_ed1 = IEEE_ADDR_ED1;
static const zb_ieee_addr_t g_ieee_addr_ed2 = IEEE_ADDR_ED2;
static const zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;

static void send_data(zb_uint8_t param);


MAIN()
{
  ARGV_UNUSED;

  ZB_INIT("zdo_3_zed2");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

  /* set ieee addr */
  zb_set_long_address(g_ieee_addr_ed2);

  /* become an ED */
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zed_role();
  zb_set_rx_on_when_idle(ZB_FALSE);
  zdo_set_aging_timeout(ED_AGING_TIMEOUT_2MIN);
  zb_set_keepalive_timeout(ZB_MILLISECONDS_TO_BEACON_INTERVAL(2000));

  zb_zdo_set_aps_unsecure_join(ZB_TRUE);

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


static void send_data_cb(zb_uint8_t param)
{
  if (!param)
  {
    TRACE_MSG(TRACE_INFO1, "send_data_cb: status OK", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_INFO1, "send_data_cb: status FAILED", (FMT__0));
  }
}


static void get_short_addr_cb(zb_uint8_t param)
{
  zb_zdo_nwk_addr_resp_head_t *resp;
  zb_ieee_addr_t ieee_addr;
  zb_uint16_t nwk_addr;
  zb_address_ieee_ref_t addr_ref;

  TRACE_MSG(TRACE_ZDO2, ">>get_short_addr_cb param %hd", (FMT__H, param));

  resp = (zb_zdo_nwk_addr_resp_head_t*)zb_buf_begin(param);
  TRACE_MSG(TRACE_ZDO2, "resp status %hd, nwk addr %d", (FMT__H_D, resp->status, resp->nwk_addr));

  ZB_DUMP_IEEE_ADDR(resp->ieee_addr);

  if (resp->status == ZB_ZDP_STATUS_SUCCESS)
  {
    ZB_LETOH64(ieee_addr, resp->ieee_addr);
    ZB_LETOH16(&nwk_addr, &resp->nwk_addr);

    zb_address_update(ieee_addr, nwk_addr, ZB_TRUE, &addr_ref);
  }

  ZB_SCHEDULE_CALLBACK(send_data, param);

  TRACE_MSG(TRACE_ZDO2, "<<test_get_short_addr_cb", (FMT__0));
}

static void get_short_addr(zb_uint8_t param)
{
  zb_zdo_nwk_addr_req_param_t *req_param = NULL;

  req_param = ZB_BUF_GET_PARAM(param, zb_zdo_nwk_addr_req_param_t);

  ZB_IEEE_ADDR_COPY(req_param->ieee_addr, g_ieee_addr_ed1);

  req_param->dst_addr     = zb_address_short_by_ieee((zb_uint8_t*) g_ieee_addr_dut);
  req_param->start_index  = 0;
  req_param->request_type = 0x00;

  zb_zdo_nwk_addr_req(param, get_short_addr_cb);
}

static void send_data(zb_uint8_t param)
{
  zb_bufid_t buf = zb_buf_get_out();

  TRACE_MSG(TRACE_APP1, ">>send_data", (FMT__0));

  ZVUNUSED(param);

  if (buf)
  {
    if (zb_address_short_by_ieee((zb_uint8_t*) g_ieee_addr_ed1) == ZB_UNKNOWN_SHORT_ADDR)
    {
      TRACE_MSG(TRACE_APP2, "we don't know ZED short address, so try to get address", (FMT__0));
      ZB_SCHEDULE_CALLBACK(get_short_addr, buf);
    }
    else
    {
      zb_buffer_test_req_param_t *req_param;

      req_param = ZB_BUF_GET_PARAM(buf, zb_buffer_test_req_param_t);
      BUFFER_TEST_REQ_SET_DEFAULT(req_param);

      req_param->dst_addr = zb_address_short_by_ieee((zb_uint8_t*) g_ieee_addr_ed1);
      TRACE_MSG(TRACE_APP1, "addr = 0x%x;", (FMT__H, req_param->dst_addr));

      zb_tp_buffer_test_request(buf, send_data_cb);
    }
  }
  else
  {
    TRACE_MSG(TRACE_APP1, "TEST FAILED: Could not get out buf!", (FMT__0));
  }

  TRACE_MSG(TRACE_APP1, "<<send_data", (FMT__0));
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
	ZB_SCHEDULE_ALARM(send_data, 0, TEST_SEND2_DELAY);
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

