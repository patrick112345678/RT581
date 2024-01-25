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
/* PURPOSE: 12.30 TP/R21/BV-30 Parent Address Resolution of Children (ZR)
Objective: DUT as ZR answers address requests for its parent children.
*/

#define ZB_TEST_NAME TP_R21_BV_30_ZC
#define ZB_TRACE_FILE_ID 40610

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

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#ifndef ZB_CERTIFICATION_HACKS
#error Define ZB_CERTIFICATION_HACKS
#endif

enum r21_bv_30_step_e
{
  R21_BV_30_NWK_ADDR_REQ_ZED2_1,
  R21_BV_30_NWK_ADDR_REQ_NONEX,
  R21_BV_30_NWK_ADDR_REQ_ZED2_2
};

static const zb_ieee_addr_t g_ieee_addr_c = IEEE_ADDR_C;
static const zb_ieee_addr_t g_ieee_addr_r = IEEE_ADDR_R;
static const zb_ieee_addr_t g_ieee_addr_ed1 = IEEE_ADDR_ED1;
static const zb_ieee_addr_t g_ieee_addr_ed2 = IEEE_ADDR_ED2;
static const zb_ieee_addr_t g_ieee_addr_nonex = IEEE_ADDR_NONEX;

static zb_uint8_t test_counter = 0;
static zb_uint16_t TEST_PAN_ID = 0x1AAA;

static void test_nwk_addr_req_cb(zb_uint8_t param);
static void test_nwk_addr_req(zb_uint8_t unused);
static void test_ieee_addr_req_cb(zb_uint8_t param);
static void test_ieee_addr_req(zb_uint8_t param);

MAIN()
{
  ARGV_UNUSED;


  ZB_INIT("zdo_1_zc");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	

  zb_set_long_address(g_ieee_addr_c);
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zc_role();

  MAC_ADD_VISIBLE_LONG((zb_uint8_t*) g_ieee_addr_r);

  zb_set_pan_id(TEST_PAN_ID);

  zb_set_max_children(3);

  zb_secur_setup_nwk_key((zb_uint8_t*) g_key0, 0);

#ifdef SECURITY_LEVEL
  zb_cert_test_set_security_level(SECURITY_LEVEL);
#endif

  zb_zdo_set_aps_unsecure_join(INSECURE_JOIN_ZC);

  zb_bdb_set_legacy_device_support(ZB_TRUE);
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

#ifdef TEST_ENABLED
	/* NWK_addr_req for IEEEAddr = gZED2 64-bit IEEE address; */
        ZB_SCHEDULE_ALARM(test_nwk_addr_req, 0, TIME_ZC_NWK_ADDR_REQ_ZED2_1);
	/* IEEE_addr_req for NWKAddrOfInterest = gZED1 16-bit NWK address; */
        ZB_SCHEDULE_ALARM(test_ieee_addr_req, 0, TIME_ZC_IEEE_ADDR_REQ_ZED1);
	/* NWK_addr_req for IEEEAddr = non-existent device 64-bit IEEE address; */
        ZB_SCHEDULE_ALARM(test_nwk_addr_req, 0, TIME_ZC_NWK_ADDR_REQ_NONEX);
	/* NWK_addr_req for IEEEAddr = gZED2 64-bit IEEE address; */
        ZB_SCHEDULE_ALARM(test_nwk_addr_req, 0, TIME_ZC_NWK_ADDR_REQ_ZED2_2);
#endif
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

static void test_nwk_addr_req_cb(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO1, "test_nwk_addr_req_cb: param = %hd;", (FMT__H, param));

  zb_buf_free(param);
}

static void test_nwk_addr_req(zb_uint8_t unused)
{
  zb_bufid_t  buf = zb_buf_get_out();
  zb_zdo_nwk_addr_req_param_t *req_param = NULL;

  ZVUNUSED(unused);

  TRACE_MSG(TRACE_ZDO1, "test_nwk_addr_req", (FMT__0));

  if (!buf)
  {
    TRACE_MSG(TRACE_ERROR, "test_nwk_addr_req: error - unable to get data buffer", (FMT__0));
    return;
  }

  req_param = ZB_BUF_GET_PARAM(buf, zb_zdo_nwk_addr_req_param_t);
  req_param->dst_addr = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;

  switch (test_counter)
  {
    case R21_BV_30_NWK_ADDR_REQ_ZED2_1:
    case R21_BV_30_NWK_ADDR_REQ_ZED2_2:
      ZB_IEEE_ADDR_COPY(req_param->ieee_addr, g_ieee_addr_ed2);
      test_counter++;
      break;

    case R21_BV_30_NWK_ADDR_REQ_NONEX:
      ZB_IEEE_ADDR_COPY(req_param->ieee_addr, g_ieee_addr_nonex);
      test_counter++;
      break;

    default:
      TRACE_MSG(TRACE_ERROR, "test_nwk_addr_req: test_counter is out of range test_counter %hd", (FMT__D, test_counter));
      break;
  }

  req_param->start_index = 0;
  req_param->request_type = 0x00;

  zb_zdo_nwk_addr_req(buf, test_nwk_addr_req_cb);
}

static void test_ieee_addr_req_cb(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO1, "test_ieee_addr_req_cb: param = %hd;", (FMT__H, param));

  zb_buf_free(param);
}

static void test_ieee_addr_req(zb_uint8_t unused)
{
  zb_bufid_t  buf = zb_buf_get_out();
  zb_zdo_ieee_addr_req_param_t *req_param = NULL;

  ZVUNUSED(unused);

  TRACE_MSG(TRACE_ZDO1, "test_ieee_addr_req", (FMT__0));

  if (!buf)
  {
    TRACE_MSG(TRACE_ERROR, "test_ieee_addr_req: error - unable to get data buffer", (FMT__0));
    return;
  }

  req_param = ZB_BUF_GET_PARAM(buf, zb_zdo_ieee_addr_req_param_t);

  req_param->dst_addr = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
  req_param->nwk_addr = zb_address_short_by_ieee((zb_uint8_t*) g_ieee_addr_ed1);
  req_param->start_index = 0;
  req_param->request_type = 0x00;

  zb_zdo_ieee_addr_req(buf, test_ieee_addr_req_cb);
}
