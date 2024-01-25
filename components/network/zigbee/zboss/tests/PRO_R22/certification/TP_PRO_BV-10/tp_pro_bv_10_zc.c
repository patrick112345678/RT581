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
/* PURPOSE:
*/



#define ZB_TEST_NAME TP_PRO_BV_10_ZC
#define ZB_TRACE_FILE_ID 40667

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "../common/zb_cert_test_globals.h"

#ifndef ZB_PRO_STACK
#error This test is not applicable for 2007 stack. ZB_PRO_STACK should be defined to enable PRO.
#endif

static const zb_ieee_addr_t g_ieee_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
static const zb_ieee_addr_t g_zr1_addr = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
//static const zb_ieee_addr_t g_zr3_addr = {0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00};

static void device_annce_cb(zb_zdo_signal_device_annce_params_t *da);

//#define TEST_CHANNEL (1l << 24)

MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("zdo_1_zc");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	
  /* let's always be coordinator */
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zc_role();

  zb_set_pan_id(0x1aaa);

  /* set ieee addr */
  zb_set_long_address(g_ieee_addr);

  /* turn off security */
  /* zb_cert_test_set_security_level(0); */

  zb_set_max_children(1);
  MAC_ADD_VISIBLE_LONG((zb_uint8_t*)g_zr1_addr);

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

static void start_discovery(zb_uint8_t param)
{
  zb_bufid_t buf = zb_buf_get_out();
  ZVUNUSED(param);
  TRACE_MSG(TRACE_APS3, ">>start_discovery param %hd", (FMT__H, buf));

  {
    /* The procedure for initiating many-to-one route discovery is outlined in sub-clause 3.6.3.5.1. */
    //! [zb_nlme_route_discovery_request]
    zb_nlme_route_discovery_request_t *rreq = ZB_BUF_GET_PARAM(buf, zb_nlme_route_discovery_request_t);
    rreq->address_mode = ZB_ADDR_NO_ADDR;
    rreq->network_addr = 0xFFFC;
    rreq->radius = 2;
    rreq->no_route_cache = ZB_TRUE;
    ZB_SCHEDULE_CALLBACK(zb_nlme_route_discovery_request, buf);
    //! [zb_nlme_route_discovery_request]
  }

  TRACE_MSG(TRACE_APS3, "<<start_discovery", (FMT__0));
}

static void device_annce_cb(zb_zdo_signal_device_annce_params_t *da)
{
  static zb_uint32_t count = 0;

  TRACE_MSG(TRACE_APP2, ">> device_annce_cb, da %p", (FMT__P, da));

  if (++count == 2)
  {
    /* All devices are joined network. Start the discovery. */
    ZB_SCHEDULE_CALLBACK(start_discovery, 0);
  }

  TRACE_MSG(TRACE_APP2, "<< device_annce_cb", (FMT__0));
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

  TRACE_MSG(TRACE_APP1, "zboss_signal_handler: status %hd signal %hd",
            (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
        TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
        break;

      case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
      {
	zb_zdo_signal_device_annce_params_t *dev_annce_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_annce_params_t);
	device_annce_cb(dev_annce_params);
      }
      break;

      default:
        break;
    }
  }
  else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
  {
    TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d",
                        (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
  }

  if (param)
  {
    zb_buf_free(param);
  }
}

