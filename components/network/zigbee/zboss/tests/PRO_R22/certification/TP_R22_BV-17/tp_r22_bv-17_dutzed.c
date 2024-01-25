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
/* PURPOSE: TP_R22_BV_17
*/

#define ZB_TEST_NAME TP_R22_BV_17_DUTZED
#define ZB_TRACE_FILE_ID 40620

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

enum test_step_e
{
  NWK_ADDR_REQ_SINGLE_RESP,
  NWK_ADDR_REQ_EXT_RESP,
  IEEE_ADDR_REQ_SINGLE_RESP,
  IEEE_ADDR_REQ_EXT_RESP,
  TEST_STEPS_COUNT
};

static const zb_ieee_addr_t g_ieee_addr_dutzed = IEEE_ADDR_DUT_ZED;
static const zb_ieee_addr_t g_ieee_addr_gzc = IEEE_ADDR_gZC;


static zb_uint8_t     g_step_idx;
static zb_uint8_t     g_payload_req_type;

static void test_logic_iteration(zb_uint8_t do_next_ts);
static void test_send_nwk_addr_req(zb_uint8_t param);
static void test_send_ieee_addr_req(zb_uint8_t param);

MAIN()
{
  ARGV_UNUSED;

  ZB_INIT("zdo_3_dutzed");
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
        ZB_SCHEDULE_ALARM(test_logic_iteration, 0, TEST_DUTZED_STARTUP_DELAY);
      }
      break; /* ZB_ZDO_SIGNAL_DEFAULT_START */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}

static void test_logic_iteration(zb_uint8_t do_next_ts)
{
  zb_callback_t call_cb = NULL;

  if (do_next_ts)
  {
    g_step_idx++;
  }

  TRACE_MSG(TRACE_ZDO1, ">>test_logic_iteration: step = %d", (FMT__D, g_step_idx));

  switch (g_step_idx)
  {
    case NWK_ADDR_REQ_SINGLE_RESP:
      g_payload_req_type = 0x00;
      call_cb = test_send_nwk_addr_req;
      break;

    case NWK_ADDR_REQ_EXT_RESP:
      g_payload_req_type = 0x01;
      call_cb = test_send_nwk_addr_req;
      break;

    case IEEE_ADDR_REQ_SINGLE_RESP:
      g_payload_req_type = 0x00;
      call_cb = test_send_ieee_addr_req;
      break;

    case IEEE_ADDR_REQ_EXT_RESP:
      g_payload_req_type = 0x01;
      call_cb = test_send_ieee_addr_req;
      break;

    default:
      if (g_step_idx == TEST_STEPS_COUNT)
      {
        TRACE_MSG(TRACE_ZDO1, "test_logic_iteration: TEST FINISHED", (FMT__0));
      }
      else
      {
        TRACE_MSG(TRACE_ZDO1, "test_logic_iteration: TEST ERROR - illegal state", (FMT__0));
      }
      break;
  }

  if (call_cb != NULL)
  {
    if (zb_buf_get_out_delayed(call_cb) != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "test_logic_iteration: zb_buf_get_out_delayed failed", (FMT__0));

      ZB_SCHEDULE_ALARM(test_logic_iteration, 0, TEST_DUTZED_NEXT_TS_DELAY);
    }
    else
    {
      ZB_SCHEDULE_ALARM(test_logic_iteration, 1, TEST_DUTZED_NEXT_TS_DELAY);
    }
  }

  TRACE_MSG(TRACE_ZDO1, "<<test_logic_iteration", (FMT__0));
}

static void test_send_nwk_addr_req(zb_uint8_t param)
{
  zb_zdo_nwk_addr_req_param_t *req = ZB_BUF_GET_PARAM(param, zb_zdo_nwk_addr_req_param_t);

  TRACE_MSG(TRACE_ZDO1, ">>send_nwk_addr_req: buf_param = %d", (FMT__D, param));

  req->dst_addr = 0xfffd;
  ZB_IEEE_ADDR_COPY(req->ieee_addr, g_ieee_addr_gzc);
  req->request_type = g_payload_req_type;
  req->start_index = 0;
  zb_zdo_nwk_addr_req(param, NULL);

  TRACE_MSG(TRACE_ZDO1, "<<send_nwk_addr_req", (FMT__0));
}


static void test_send_ieee_addr_req(zb_uint8_t param)
{
  zb_zdo_ieee_addr_req_param_t *req;

  TRACE_MSG(TRACE_ZDO1, ">>send_ieee_addr_req: buf_param = %d", (FMT__D, param));

  req = ZB_BUF_GET_PARAM(param, zb_zdo_ieee_addr_req_param_t);

  req->nwk_addr = 0x0000;
  req->dst_addr = 0x0000;
  req->request_type = g_payload_req_type;
  req->start_index = 0;
  ZB_HTOLE16_ONPLACE((zb_uint8_t*) &req->nwk_addr);
  zb_zdo_ieee_addr_req(param, NULL);

  TRACE_MSG(TRACE_ZDO1, "<<send_ieee_addr_req", (FMT__0));
}
