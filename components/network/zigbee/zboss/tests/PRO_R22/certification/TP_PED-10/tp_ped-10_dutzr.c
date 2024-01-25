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

#define ZB_TEST_NAME TP_PED_10_DUTZR
#define ZB_TRACE_FILE_ID 40642

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_console_monitor.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"


#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

static const zb_ieee_addr_t g_ieee_addr_dutzr = IEEE_ADDR_DUT_ZR;
static const zb_ieee_addr_t g_ieee_addr_ed = IEEE_ADDR_ED;

#if 0
static void test_close_permit_join(zb_uint8_t param);
#endif

static void send_data_bc_delayed(zb_uint8_t param);
static void send_data_bc(zb_uint8_t param);
static void send_data_cb(zb_uint8_t param);

MAIN()
{
  ARGV_UNUSED;

	char command_buffer[100], *command_ptr;
  char next_cmd[40];
  zb_bool_t res;
	
  ZB_INIT("zdo_3_dutzr");
#if UART_CONTROL	
	test_control_init();
#endif

  /* set ieee addr */
  zb_set_long_address(g_ieee_addr_dutzr);
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zr_role();
  zb_zdo_set_aps_unsecure_join(ZB_TRUE);

#ifdef ZB_USE_NVRAM
  zb_cert_test_set_aps_use_nvram();
#endif

#ifdef SECURITY_LEVEL
  zb_cert_test_set_security_level(SECURITY_LEVEL);
#endif

  /* join as a router */
  zb_set_max_children(1);

  zb_bdb_set_legacy_device_support(ZB_TRUE);
  /* DON'T use ZB_TRUE parameter, because
   * gZR1 should by rejoin to gZC
   */
  TRACE_MSG(TRACE_APP1, "Send 'erase' for flash erase or just press enter to be continued \n", (FMT__0));
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_handler);
  zb_console_monitor_get_cmd((zb_uint8_t*)command_buffer, sizeof(command_buffer));
  command_ptr = (char *)(&command_buffer);
  res = parse_command_token(&command_ptr, next_cmd, sizeof(next_cmd));
  if (strcmp(next_cmd, "erase") == 0)
    zb_set_nvram_erase_at_start(ZB_TRUE);
  else
    zb_set_nvram_erase_at_start(ZB_FALSE);
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);

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

  switch (sig)
  {
    case ZB_ZDO_SIGNAL_DEFAULT_START:
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ZB_BDB_SIGNAL_DEVICE_REBOOT:
      if (status == 0)
      {
        TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));

        ZB_SCHEDULE_ALARM(send_data_bc, 0, TEST_SEND_BUFFER_TEST_REQ_DELAY);
        /* ZB_SCHEDULE_ALARM(test_close_permit_join, 0, DUTZR_CLOSE_PERMIT_JOIN_DELAY); */
      }
      else
      {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, status));
      }
      break; /* ZB_ZDO_SIGNAL_DEFAULT_START */

#ifdef ZB_USE_SLEEP
    case ZB_COMMON_SIGNAL_CAN_SLEEP:
      zb_sleep_now();
      break;
#endif /* ZB_USE_SLEEP */

    default:
      TRACE_MSG(TRACE_ERROR, "Unknown signal: status %hd signal %hd",
            (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));
      break;
  }

  zb_buf_free(param);
}

#if 0
static void test_close_permit_join(zb_uint8_t param)
{
  zb_bufid_t buf = zb_buf_get_out();

  ZVUNUSED(param);

  TRACE_MSG(TRACE_APS1, ">>test_close_permit_join", (FMT__0));

  if (buf)
  {
    zb_nlme_permit_joining_request_t *req;

    TRACE_MSG(TRACE_APS1, "Closing permit join on dutZR, buf = %d",
              (FMT__D, buf));

    req = zb_buf_get_tail(buf, sizeof(zb_nlme_permit_joining_request_t));
    ZB_BZERO(req, sizeof(zb_nlme_permit_joining_request_t));
    req->permit_duration = 0;

    zb_nlme_permit_joining_request(buf);
  }
  else
  {
    TRACE_MSG(TRACE_APS1, "TEST FAILED: Could not get out buf!", (FMT__0));
  }

  TRACE_MSG(TRACE_APS1, "<<test_close_permit_join", (FMT__0));
}
#endif


static void send_data_bc_delayed(zb_uint8_t param)
{
  zb_buffer_test_req_param_t *req_param;

  TRACE_MSG(TRACE_APP3, "send_data_bc_delayed", (FMT__0));
  if (!param)
  {
    TRACE_MSG(TRACE_ERROR, "send_data: unable to get data buffer", (FMT__0));
  }

  req_param = ZB_BUF_GET_PARAM(param, zb_buffer_test_req_param_t);
  BUFFER_TEST_REQ_SET_DEFAULT(req_param);
  req_param->dst_addr = zb_address_short_by_ieee((zb_uint8_t*)g_ieee_addr_ed);

  zb_tp_buffer_test_request(param, send_data_cb);
}

static void send_data_bc(zb_uint8_t param)
{
  ZVUNUSED(param);

  zb_buf_get_out_delayed(send_data_bc_delayed);
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
