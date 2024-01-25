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

#define ZB_TEST_NAME TP_PED_13_ZR
#define ZB_TRACE_FILE_ID 40902

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

static const zb_ieee_addr_t g_ieee_addr_zr = IEEE_ADDR_ZR;
static const zb_ieee_addr_t g_ieee_addr_dutzc = IEEE_ADDR_DUT_ZC;
static const zb_ieee_addr_t g_ieee_addr_ed = IEEE_ADDR_ED;

#if 0
static void test_close_permit_join(zb_uint8_t param);
#endif

static void send_mgmt_lqi_req_delayed(zb_uint8_t param);
static void send_mgmt_lqi_req(zb_uint8_t param);
static void mgmt_lqi_resp(zb_uint8_t param);
static void send_data_bc_delayed(zb_uint8_t param);
static void send_data_bc(zb_uint8_t param);
static void send_data_cb(zb_uint8_t param);

static zb_uint8_t s_mgmt_lqi_start_index = 0;

MAIN()
{
  ARGV_UNUSED;
	
	char command_buffer[100], *command_ptr;
  char next_cmd[40];
  zb_bool_t res;
	
  ZB_INIT("zdo_3_zr");
#if UART_CONTROL	
	test_control_init();
#endif

  /* set ieee addr */
  zb_set_long_address(g_ieee_addr_zr);
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zr_role();
  zb_zdo_set_aps_unsecure_join(ZB_TRUE);

#ifdef ZB_USE_NVRAM
  zb_cert_test_set_aps_use_nvram();
#endif

#ifdef SECURITY_LEVEL
  zb_cert_test_set_security_level(SECURITY_LEVEL);
#endif

  zb_cert_test_set_keepalive_mode(ED_TIMEOUT_REQUEST_KEEPALIVE);

  /* join as a router */
  zb_set_max_children(1);

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

  TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  switch (sig)
  {
    case ZB_ZDO_SIGNAL_DEFAULT_START:
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ZB_BDB_SIGNAL_DEVICE_REBOOT:
      if (status == 0)
      {
	TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));

	/* send a mgmt_lqi_req */
	ZB_SCHEDULE_ALARM(send_mgmt_lqi_req_delayed, 0, TEST_CHECK_DUTZC_NBT_DELAY);
	/* send Buffer Test Req */
	ZB_SCHEDULE_ALARM(send_data_bc, 0, TEST_SEND_BUFFER_TEST_REQ_DELAY);
	/* ZB_SCHEDULE_ALARM(test_close_permit_join, 0, ZR_CLOSE_PERMIT_JOIN_DELAY); */
      }
      else
      {
	TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, status));
      }
      break; /* ZB_ZDO_SIGNAL_DEFAULT_START */

    default:
      if (status == 0)
      {
	TRACE_MSG(TRACE_APS1, "Unknown signal, status OK", (FMT__0));
      }
      else
      {
	TRACE_MSG(TRACE_ERROR, "Unknown signal, status %d", (FMT__D, status));
      }
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

    TRACE_MSG(TRACE_APS1, "Closing permit join on zr, buf = %d",
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

static void send_mgmt_lqi_req_delayed(zb_uint8_t param)
{
  ZVUNUSED(param);

  zb_buf_get_out_delayed(send_mgmt_lqi_req);
}

static void send_mgmt_lqi_req(zb_uint8_t param)
{
  zb_zdo_mgmt_lqi_param_t *req_param = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_lqi_param_t);
  zb_uint16_t s_dutzc_short_addr;

  TRACE_MSG(TRACE_ZDO1, ">> mgmt_lqi_resp buf = %d", (FMT__D, param));

  s_dutzc_short_addr = zb_address_short_by_ieee((zb_uint8_t*) g_ieee_addr_dutzc);

  if (s_dutzc_short_addr != ZB_UNKNOWN_SHORT_ADDR)
  {
    req_param->dst_addr = s_dutzc_short_addr;
    req_param->start_index = s_mgmt_lqi_start_index;

    zb_zdo_mgmt_lqi_req(param, mgmt_lqi_resp);
  }
  else
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZDO1, "<<mgmt_lqi_resp buf", (FMT__0));
}

static void mgmt_lqi_resp(zb_uint8_t param)
{
  zb_zdo_mgmt_lqi_resp_t *resp = (zb_zdo_mgmt_lqi_resp_t*) zb_buf_begin(param);

  TRACE_MSG(TRACE_ZDO1, ">>mgmt_lqi_resp buf = %d", (FMT__D, param));

  if (resp->status == ZB_ZDP_STATUS_SUCCESS)
  {
    zb_uint8_t nbrs = resp->neighbor_table_list_count;

    s_mgmt_lqi_start_index += nbrs;
    if (s_mgmt_lqi_start_index < resp->neighbor_table_entries)
    {
      TRACE_MSG(TRACE_ZDO2, "mgmt_lqi_resp: retrieved = %d, total = %d",
                (FMT__D_D, nbrs, resp->neighbor_table_entries));
      zb_buf_get_out_delayed(send_mgmt_lqi_req);
    }
    else
    {
      TRACE_MSG(TRACE_ZDO2, "mgmt_lqi_resp: retrieved all entries - %d",
                (FMT__D, resp->neighbor_table_entries));
    }
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "mgmt_lqi_resp: request failure with status = %d", (FMT__D, resp->status));
  }
  zb_buf_free(param);

  TRACE_MSG(TRACE_ZDO1, "<<mgmt_lqi_resp", (FMT__0));
}

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
