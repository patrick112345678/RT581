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

#define ZB_TEST_NAME TP_PRO_BV_2930_ZC1
#define ZB_TRACE_FILE_ID 40644

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur_api.h"
#include "zb_secur.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"

#ifndef ZB_CERTIFICATION_HACKS
#error Define ZB_CERTIFICATION_HACKS macro
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static const zb_ieee_addr_t g_zc_addr = IEEE_ADDR_ZC;
static const zb_ieee_addr_t g_zr2_addr = IEEE_ADDR_ZR2;

/* Key for ZC1 */
static zb_uint8_t g_nwk_key_zc1[16] = {0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("zdo_1_zc1");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	
  /* let's always be coordinator */
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zc_role();
  zb_set_max_children(1);
  zb_aib_tcpol_set_update_trust_center_link_keys_required(ZB_FALSE);

  /* assign our address */
  zb_set_long_address(g_zc_addr);
  zb_set_pan_id(0x1aaa);

  MAC_ADD_VISIBLE_LONG((zb_uint8_t*)g_zr2_addr);
  MAC_ADD_INVISIBLE_SHORT(0x0000); /* Ignore beacons from ZC2 */

  zb_secur_setup_nwk_key((zb_uint8_t*) g_nwk_key_zc1, 0);

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

static void zc_send_data(zb_bufid_t buf, zb_uint16_t addr);

static void send_buffertest1(zb_uint8_t param)
{
  zb_bufid_t buf = zb_buf_get_out();

  ZVUNUSED(param);
  /*
    5) DUT ZC sends a broadcast Buffer Test Request (secured, KEY0) to all
    members of the PAN
   */

  if (buf)
  {
    TRACE_MSG(TRACE_ERROR, "buffer test 1", (FMT__0));
    zc_send_data(buf, ZB_NWK_BROADCAST_ALL_DEVICES);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "TEST FAILED: Could not get out buf!", (FMT__0));
  }
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
        zb_enable_auto_pan_id_conflict_resolution(ZB_TRUE);
        zb_schedule_alarm(send_buffertest1, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(60000));
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


static void zc_send_data(zb_bufid_t buf, zb_uint16_t addr)
{
  zb_buffer_test_req_param_t *req_param;
  TRACE_MSG(TRACE_ERROR, "send_test_request to %d", (FMT__D, addr));
  req_param = ZB_BUF_GET_PARAM(buf, zb_buffer_test_req_param_t);
  BUFFER_TEST_REQ_SET_DEFAULT(req_param);
  req_param->len = 0x10;
  req_param->dst_addr = addr;
  req_param->src_ep = 1;

  zb_tp_buffer_test_request(buf, buffer_test_cb);
}

/*! @} */
