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

/* DUT ZR
*/

#define ZB_TEST_NAME TP_PRO_BV_11_ZR2
#define ZB_TRACE_FILE_ID 40584

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "../nwk/nwk_internal.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"

#ifndef ZB_PRO_STACK
#error This test is not applicable for 2007 stack. ZB_PRO_STACK should be defined to enable PRO.
#endif

#ifndef ZB_LIMIT_VISIBILITY
#error You must define ZB_LIMIT_VISIBILITY for use this test
#endif

static const zb_ieee_addr_t g_ieee_addr_c = IEEE_ADDR_C;
static const zb_ieee_addr_t g_ieee_addr_r2 = IEEE_ADDR_R2;
static const zb_ieee_addr_t g_ieee_addr_r3 = IEEE_ADDR_R3;

/* static void send_data(zb_uint8_t param); */
/* static void test_route_record_cmd(zb_uint8_t param); */
/* static void send_dev_annc(zb_uint8_t param); */

static zb_uint8_t dev_num = 1;
/* static zb_uint8_t recv_pckt_count = 0; */

MAIN()
{
  ARGV_UNUSED;

  ZB_INIT("zdo_2_zr2");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	
  /* ZR2 */
  zb_set_long_address(g_ieee_addr_r2);

  MAC_ADD_VISIBLE_LONG((zb_uint8_t*) g_ieee_addr_c);
  MAC_ADD_VISIBLE_LONG((zb_uint8_t*) g_ieee_addr_r3);

  /* join as a router */
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zr_role();
  zb_aib_tcpol_set_update_trust_center_link_keys_required(ZB_FALSE);

  /* turn off security */
  /* zb_cert_test_set_security_level(0); */

  zb_set_max_children(1);
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

/* 10/22/2019 EE CR:MINOR Remove all that if 0. Will never uncomment it. */
#if 0
static void test_route_record_cmd(zb_uint8_t param)
{
  zb_bufid_t buf = zb_buf_get_out();
  ZVUNUSED(param);

  TRACE_MSG(TRACE_INFO3, ">>test_route_record_cmd", (FMT__0));

  if (!buf)
  {
    TRACE_MSG(TRACE_NWK1, "test_route_record_cmd: error - unable to get data buffer", (FMT__0));
    ZB_EXIT(1);
  }
  zb_nwk_send_rrec(buf, ZB_PIBCACHE_NETWORK_ADDRESS(), 0x0000);
  TRACE_MSG(TRACE_INFO3, "<<test_route_record_cmd", (FMT__0));
}
#endif

static zb_uint16_t addr_ass_cb(zb_ieee_addr_t ieee_addr)
{
  zb_uint16_t res = -1;
  TRACE_MSG(TRACE_APS3, ">>addr_assignmnet_cb " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(ieee_addr)));

  if (ZB_IEEE_ADDR_CMP(ieee_addr, g_ieee_addr_r3))
  {
    res = g_short_addr_r3;
  }
  else
  {
    res = g_short_addr_r4;
  }

  TRACE_MSG(TRACE_APS3, "<<addr_assignmnet_cb: res = 0x%x;", (FMT__H, res));
  return res;
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

        /* Setup address assignment callback */
        zb_nwk_set_address_assignment_cb(addr_ass_cb);

        if (dev_num == 3)
        {
#if 0
          test_route_record_cmd(0);
#endif

#if ZR4_USE_DATA_INDICATION
          zb_af_set_data_indication(data_indication);
#endif
        }
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
    TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d", (FMT__D, status));
  }

  if (param)
  {
    zb_buf_free(param);
  }
}
