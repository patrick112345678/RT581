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

#define ZB_TEST_NAME TP_R21_BV_08_ZC
#define ZB_TRACE_FILE_ID 40812
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "../common/zb_cert_test_globals.h"


/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

static const zb_ieee_addr_t g_ext_panid = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const zb_ieee_addr_t g_ieee_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
/* [zb_secur_setup_preconfigured_key_1] */
static const zb_uint8_t g_key_nwk[16] = { 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0, 0, 0, 0, 0, 0, 0, 0};
/* [zb_secur_setup_preconfigured_key_1] */

static const zb_ieee_addr_t g_ieee_addr1 = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
/* zb_ieee_addr_t g_ieee_addr2 = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; */

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
	


  zb_set_long_address(g_ieee_addr);
  zb_set_use_extended_pan_id(g_ext_panid);


  zb_set_pan_id(0x1aaa);

  /* let's always be coordinator */
  zb_secur_setup_nwk_key((zb_uint8_t*) g_key_nwk, 0);

  /* only ZR1 is visible for ZC */
  MAC_ADD_VISIBLE_LONG((zb_uint8_t*) g_ieee_addr1);
  zb_set_max_children(1);

  zb_zdo_set_aps_unsecure_join(ZB_TRUE);

  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zc_role();

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

static void open_network(zb_uint8_t param)
{
  zb_bufid_t buf = zb_buf_get_out();
  zb_nlme_permit_joining_request_t *req;
  ZVUNUSED(param);

  if (buf)
  {
    TRACE_MSG(TRACE_APP1, ">>open_network", (FMT__0));

    TRACE_MSG(TRACE_APP1, "Opening permit join on device, buf = %d",
              (FMT__D, buf));

    req = zb_buf_get_tail(buf, sizeof(zb_nlme_permit_joining_request_t));
    req->permit_duration = -1;
    zb_nlme_permit_joining_request(buf);

    TRACE_MSG(TRACE_APP1, "<<open_network", (FMT__0));
  }
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
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_ZDO_SIGNAL_DEFAULT_START:
        TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
	open_network(0);
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

  zb_buf_free(param);
}


/*! @} */

