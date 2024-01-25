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
/* PURPOSE: TP/R21/BV-02 - Router (gZR) for distributed network
*/


#define ZB_TEST_NAME TP_R21_BV_01_GZR_D
#define ZB_TRACE_FILE_ID 40526
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


/*! \addtogroup ZB_TESTS */
/*! @{ */

static const zb_ieee_addr_t g_ieee_addr_gzr = IEEE_ADDR_gZR;

static void test_send_node_desc_req(zb_uint8_t param);

static zb_uint16_t s_dut_short_addr = 0x0000;

MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("zdo_2_gzr_d");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	
  zb_set_long_address(g_ieee_addr_gzr);
  zb_set_pan_id(0x1aaa);

  zb_set_use_extended_pan_id(g_ext_pan_id);
  zb_aib_set_trust_center_address(g_unknown_ieee_addr);

  /* Not mandatory, but possible: set address */
  /* zb_cert_test_set_network_addr(0x2bbb); */

  zb_set_pan_id(0x1aaa);

  /* Set well-known nwk key set in wireshark (also optional) */
  zb_secur_setup_nwk_key((zb_uint8_t*) g_nwk_key, 0);

  zb_zdo_set_aps_unsecure_join(ZB_TRUE);

  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zr_role();

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
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);
  zb_zdo_signal_device_annce_params_t *dev_annce_params;

  TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  switch (sig)
  {
    case ZB_ZDO_SIGNAL_DEFAULT_START:
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ZB_BDB_SIGNAL_DEVICE_REBOOT:
      if (status == 0)
      {
	TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
      }
      else
      {
	TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, status));
      }
      break; /* ZB_ZDO_SIGNAL_DEFAULT_START */

    case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
      if (status == 0)
      {
        TRACE_MSG(TRACE_APS1, "signal: ZB_ZDO_SIGNAL_DEVICE_ANNCE, status OK", (FMT__0));

        dev_annce_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_annce_params_t);
        s_dut_short_addr = dev_annce_params->device_short_addr;
        ZB_SCHEDULE_CALLBACK(test_send_node_desc_req, 0);
      }
      else
      {
	TRACE_MSG(TRACE_ERROR, "signal: ZB_ZDO_SIGNAL_DEVICE_ANNCE, status %d", (FMT__D, status));
      }
      break; /* ZB_ZDO_SIGNAL_DEVICE_ANNCE */

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

static void test_send_node_desc_req(zb_uint8_t param)
{
  zb_bufid_t buf = zb_buf_get_out();
  zb_zdo_node_desc_req_t *req;

  ZVUNUSED(param);
  TRACE_MSG(TRACE_APS1, ">>test_send_node_desc_req, param = %d", (FMT__D, (int)param));

  req = zb_buf_initial_alloc(buf, sizeof(zb_zdo_node_desc_req_t));
  req->nwk_addr = s_dut_short_addr; /* send to DUT */
  TRACE_MSG(TRACE_APS1, "send node_desc_req: nwk_addr = %h.", (FMT__H, req->nwk_addr));
  zb_zdo_node_desc_req(buf, NULL);

  TRACE_MSG(TRACE_APS1, "<<test_send_node_desc_req", (FMT__0));
}


/*! @} */

