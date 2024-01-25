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
/* PURPOSE: TP/R20/BV-13 ZC
*/

#define ZB_TEST_NAME TP_R20_BV_13_ZC
#define ZB_TRACE_FILE_ID 40732

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "../common/zb_cert_test_globals.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_CERTIFICATION_HACKS
#error Define ZB_CERTIFICATION_HACKS
#endif /* ZB_CERTIFICATION_HACKS */

#define CONNECTION_TIMEOUT 15
//#define TEST_CHANNEL (1l << 24)

/* apsUseExtendedPANID */
static const zb_ieee_addr_t g_aps_ext_pan_id = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/* For NS build first ieee addr byte should be unique */
static zb_ieee_addr_t g_ieee_addr   = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
static zb_ieee_addr_t g_ieee_addr_d = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};

static void zb_nwk_leave_req(zb_uint8_t param);

MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("zdo_zc");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	
  /* let's always be coordinator */
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zc_role();

  /* set ieee addr */
  zb_set_long_address(g_ieee_addr);
  zb_set_pan_id(0x1aaa);
  zb_set_use_extended_pan_id(g_aps_ext_pan_id);

  ZB_CERT_HACKS().enable_leave_to_router_hack = 1;

  zb_set_max_children(1);

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
        TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));

        ZB_SCHEDULE_ALARM(zb_nwk_leave_req, 0, (CONNECTION_TIMEOUT) * ZB_TIME_ONE_SECOND);
        ZB_SCHEDULE_ALARM(zb_nwk_leave_req, 1, (CONNECTION_TIMEOUT+20) * ZB_TIME_ONE_SECOND);
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

//! [zb_nlme_leave_request]
static void zb_nwk_leave_req(zb_uint8_t param)
{
  zb_nlme_leave_request_t *lr = NULL;
  zb_bufid_t buf = zb_buf_get_out();

  TRACE_MSG(TRACE_ERROR, "###zb_nwk_leave_req: step %hd", (FMT__H, param));
  if (!buf)
  {
    TRACE_MSG(TRACE_ERROR, "###zb_nwk_leave_req: unable to get data buffer", (FMT__0));
    return;
  }

  lr = ZB_BUF_GET_PARAM(buf, zb_nlme_leave_request_t);
  ZB_64BIT_ADDR_COPY(lr->device_address, g_ieee_addr_d);
  lr->remove_children = 0;
  lr->rejoin = 0;
  ZB_SCHEDULE_CALLBACK(zb_nlme_leave_request, buf);
}
//! [zb_nlme_leave_request]

/*! @} */
