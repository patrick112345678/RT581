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
/* PURPOSE: TP_R21_BV-15 ZigBee Coordinator
*/

#define ZB_TEST_NAME TP_PED_2_ZC
#define ZB_TRACE_FILE_ID 40865

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


/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

static const zb_ieee_addr_t g_ieee_addr_c = IEEE_ADDR_C;
static const zb_ieee_addr_t g_ieee_addr_ed2 = IEEE_ADDR_ED2;

static void send_data(zb_uint8_t param);
static void send_data_cb(zb_uint8_t param);

MAIN()
{
  ARGV_UNUSED;

  ZB_INIT("zdo_1_zc");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

  zb_set_long_address(g_ieee_addr_c);
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zc_role();
  zb_set_max_children(2);
  zb_set_pan_id(TEST_PAN_ID);

#ifdef SECURITY_LEVEL
  zb_cert_test_set_security_level(SECURITY_LEVEL);
#endif

  zb_secur_setup_nwk_key((zb_uint8_t*) g_key0, 0);
  zb_zdo_set_aps_unsecure_join(ZB_TRUE);

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


static void send_data_cb(zb_uint8_t param)
{
  ZVUNUSED(param);

  TRACE_MSG(TRACE_INFO1, "send_data_cb: status OK", (FMT__0));
}


static void send_data(zb_uint8_t param)
{
  zb_bufid_t buf = zb_buf_get_out();
  zb_buffer_test_req_param_t *req_param;

  ZVUNUSED(param);
  TRACE_MSG(TRACE_APP1, ">>send_data", (FMT__0));

  if (buf)
  {
    req_param = ZB_BUF_GET_PARAM(buf, zb_buffer_test_req_param_t);
    BUFFER_TEST_REQ_SET_DEFAULT(req_param);

    req_param->dst_addr = zb_address_short_by_ieee((zb_uint8_t*) g_ieee_addr_ed2);
    TRACE_MSG(TRACE_APP1, "addr = 0x%x;", (FMT__H, req_param->dst_addr));

    zb_tp_buffer_test_request(buf, send_data_cb);
  }
  else
  {
    TRACE_MSG(TRACE_APP1, "TEST FAILED: Could not get out buf!", (FMT__0));
  }

  TRACE_MSG(TRACE_APP1, "<<send_data", (FMT__0));
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
        ZB_SCHEDULE_ALARM(send_data, 0, TEST_SEND1_DELAY);
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

  zb_buf_free(param);
}

/*! @} */

