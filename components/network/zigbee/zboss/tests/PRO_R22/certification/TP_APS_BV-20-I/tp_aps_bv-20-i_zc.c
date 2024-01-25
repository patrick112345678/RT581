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
/* PURPOSE: TP/APS/BV-20-I Coordinator
*/

#define ZB_TEST_NAME TP_APS_BV_20_I_ZC
#define ZB_TRACE_FILE_ID 40503

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"

#define SEND_TIMEOUT ZB_MILLISECONDS_TO_BEACON_INTERVAL(20000)


static void zb_bind_callback(zb_uint8_t param);

static const zb_ieee_addr_t g_ieee_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
static const zb_ieee_addr_t g_ieee_addr_d = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
static zb_uint8_t i = 0;


MAIN()
{
  ARGV_UNUSED;

  ZB_INIT("zc");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zc_role();
  zb_set_pan_id(TEST_PAN_ID);
  zb_set_long_address(g_ieee_addr);
  zb_set_max_children(1);

  /* zb_cert_test_set_security_level(0); */

  i = 2;

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

static void zb_bind_callback(zb_uint8_t param)
{
  zb_zdo_bind_resp_t *bind_resp = (zb_zdo_bind_resp_t*)zb_buf_begin(param);

  TRACE_MSG(TRACE_APS1, ">>zb_bind_callback %hd", (FMT__H, bind_resp->status));

  if (bind_resp->status == ZB_ZDP_STATUS_SUCCESS)
  {
    zb_zdo_bind_req_param_t *req;
    req = ZB_BUF_GET_PARAM(param, zb_zdo_bind_req_param_t);
    req->src_endp       = i;
    req->cluster_id     = 0x01;
    req->dst_endp       = 0xF0;
    req->req_dst_addr   = zb_address_short_by_ieee((zb_uint8_t*) g_ieee_addr_d);
    req->dst_addr_mode  = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
    ZB_MEMCPY(&req->src_address, &g_ieee_addr_d, sizeof(zb_ieee_addr_t));
    ZB_MEMCPY(&req->dst_address.addr_long, &g_ieee_addr, sizeof(zb_ieee_addr_t));

    zb_zdo_bind_req(param, zb_bind_callback);
    i++;
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "TABLE FULL %hd", (FMT__H, bind_resp->status));
    zb_buf_free(param);
  }
  TRACE_MSG(TRACE_APS1, "<<zb_bind_callback", (FMT__0));
}

static void zb_test(zb_uint8_t param)
{
  zb_bufid_t buf = zb_buf_get_out();

  ZVUNUSED(param);
  TRACE_MSG(TRACE_APS1, ">>zb_test", (FMT__0));

  if (buf)
  {
    zb_zdo_bind_req_param_t *req;

    req = ZB_BUF_GET_PARAM(buf, zb_zdo_bind_req_param_t);

    req->src_endp       = 0x01;
    req->cluster_id     = 0x01;
    req->dst_addr_mode  = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
    req->dst_endp       = 0xF0;
    req->req_dst_addr   = zb_address_short_by_ieee((zb_uint8_t*) g_ieee_addr_d);

    ZB_MEMCPY(&req->src_address, &g_ieee_addr_d, sizeof(zb_ieee_addr_t));
    ZB_MEMCPY(&req->dst_address.addr_long, &g_ieee_addr, sizeof(zb_ieee_addr_t));

    zb_zdo_bind_req(buf, zb_bind_callback);
  }
  else
  {
    TRACE_MSG(TRACE_APS1, "TEST_FAILED: Could not get out buf!", (FMT__0));
  }

  TRACE_MSG(TRACE_APS1, "<<zb_test", (FMT__0));
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
        ZB_SCHEDULE_ALARM(zb_test, 0, SEND_TIMEOUT);
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

  if (param)
  {
    zb_buf_free(param);
  }
}

