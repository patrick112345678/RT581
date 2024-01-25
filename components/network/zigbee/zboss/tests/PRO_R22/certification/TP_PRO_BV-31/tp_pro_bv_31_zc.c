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
/* PURPOSE: Test for ZC application written using ZDO.
*/

#define ZB_TEST_NAME TP_PRO_BV_31_ZC
#define ZB_TRACE_FILE_ID 40521

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "../common/zb_cert_test_globals.h"

#define TEST_BUFFER_LEN 10
//#define TEST_CHANNEL (1l << 24)

static const zb_ieee_addr_t g_ieee_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
static const zb_ieee_addr_t g_ext_pan_id = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/* ZR1 ieee addr */
static const zb_ieee_addr_t r1_ieee_addr = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};

static void zc_send_data(zb_bufid_t buf, zb_uint16_t addr);
static void send_buffertest1(zb_uint8_t param);
static void buffer_test_cb(zb_uint8_t param);


MAIN()
{
  ARGV_UNUSED;

  /* FIXME: APS secure is off inside stack, lets use NWK secure */
#if 0
  aps_secure = 1;
#endif

  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("zdo_1_zc");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	
  /* let's always be coordinator */
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zc_role();

  zb_set_long_address(g_ieee_addr);

  /* we need this for rejoin */
  zb_set_use_extended_pan_id(g_ext_pan_id);

  zb_set_pan_id(0x1aaa);

  zb_set_max_children(2);

  /* turn off security */
  /* zb_cert_test_set_security_level(0); */

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

  TRACE_MSG(TRACE_ERROR, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  if (0 == status)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
	TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));

	zb_schedule_alarm(send_buffertest1, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(30000));
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
    TRACE_MSG(TRACE_ERROR, "Device STARTE FAILED status %d", (FMT__D, status));
  }

  zb_buf_free(param);
}


static void zc_send_data(zb_bufid_t buf, zb_uint16_t addr)
{
  zb_buffer_test_req_param_t *req_param;

  TRACE_MSG(TRACE_APS1, "send_test_request to %d", (FMT__D, addr));
  req_param = ZB_BUF_GET_PARAM(buf, zb_buffer_test_req_param_t);
  BUFFER_TEST_REQ_SET_DEFAULT(req_param);
  req_param->len = TEST_BUFFER_LEN;
  req_param->dst_addr = addr;

  zb_tp_buffer_test_request(buf, buffer_test_cb);
}


static void buffer_test_cb(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APS1, "buffer_test_cb %hd", (FMT__H, param));

  if (param == ZB_TP_BUFFER_TEST_OK)
  {
    TRACE_MSG(TRACE_APS1, "status OK", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_APS1, "status ERROR", (FMT__0));
  }
}

static void send_buffertest1(zb_uint8_t param)
{
  zb_bufid_t buf = zb_buf_get_out();
  zb_uint16_t addr;

  ZVUNUSED(param);
  TRACE_MSG(TRACE_APS1, ">>send_buffertest1", (FMT__0));

  if (buf)
  {
    /* Send unicast test buffer to the r1 device */
    addr = zb_address_short_by_ieee((zb_uint8_t*) r1_ieee_addr);
    zc_send_data(buf, addr);

    TRACE_MSG(TRACE_INFO3, "send test buffer to 0x%x", (FMT__H, addr));
  }
  else
  {
    TRACE_MSG(TRACE_INFO3, "TEST FAILED: Could not get out buf!", (FMT__0));
  }

  TRACE_MSG(TRACE_APS1, "<<send_buffertest1", (FMT__0));
}

