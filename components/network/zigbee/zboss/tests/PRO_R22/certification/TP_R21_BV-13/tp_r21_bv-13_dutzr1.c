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


#define ZB_TEST_NAME TP_R21_BV_13_DUTZR1
#define ZB_TRACE_FILE_ID 40588

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

static const zb_ieee_addr_t g_ieee_addr_dutzr1 = IEEE_ADDR_DUT_ZR1;

static void test_brcast_tbuffer_req_delayed(zb_uint8_t param);
static void change_outgoing_nwk_fr_cnt(zb_uint8_t param);
static void brcast_tbuffer_request(zb_uint8_t param);
static void test_set_permit_join(zb_uint8_t param, zb_uint16_t permit_join_duration);

MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("zdo_2_dutzr1");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zr_role();
  zb_set_long_address(g_ieee_addr_dutzr1);
  zb_zdo_set_aps_unsecure_join(ZB_TRUE);
  zb_set_max_children(0);       /* so zr2 joins zc */

#ifdef SECURITY_LEVEL
  zb_cert_test_set_security_level(SECURITY_LEVEL);
#endif

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
        ZB_SCHEDULE_ALARM(test_brcast_tbuffer_req_delayed, 0,
                          TEST_ZR1_FIRST_TBUFFER_REQUEST_DELAY);
        break;
      case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
        test_set_permit_join(0, 0xff);
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


static void test_set_permit_join(zb_uint8_t param, zb_uint16_t permit_join_duration)
{
  TRACE_MSG(TRACE_APS1, ">> test_close_permit_join param %hd", (FMT__H, param));

  if (!param)
  {
    zb_buf_get_out_delayed_ext(test_set_permit_join, permit_join_duration, 0);
  }
  else
  {
    zb_nlme_permit_joining_request_t *req;

    zb_set_max_children(1);
    req = zb_buf_get_tail(param, sizeof(zb_nlme_permit_joining_request_t));
    ZB_BZERO(req, sizeof(zb_nlme_permit_joining_request_t));
    req->permit_duration = (permit_join_duration & 0xFF); /* Safe downcast */

    zb_nlme_permit_joining_request(param);
  }

  TRACE_MSG(TRACE_APS1, "<< test_close_permit_join", (FMT__0));
}


static void test_brcast_tbuffer_req_delayed(zb_uint8_t param)
{
  static int i = 1;

  ZVUNUSED(param);
  if (i == 2)
  {
    change_outgoing_nwk_fr_cnt(0);
  }

  zb_buf_get_out_delayed(brcast_tbuffer_request);
  if (i < 3)
  {
    ZB_SCHEDULE_ALARM(test_brcast_tbuffer_req_delayed, 0,
                      TEST_ZR1_NEXT_TBUFFER_REQUEST_DELAY);
  }
  ++i;
}


static void change_outgoing_nwk_fr_cnt(zb_uint8_t param)
{
  ZVUNUSED(param);
  TRACE_MSG(TRACE_APS2, ">>change_outgoing_nwk_fr_cnt", (FMT__0));
  /* change outgoing nwk frame counter */
  zb_cert_test_nib_set_outgoing_frame_counter(0x80000001);
  TRACE_MSG(TRACE_APS2, "<<change_outgoing_nwk_fr_cnt", (FMT__0));
}


static void brcast_tbuffer_request(zb_uint8_t param)
{
  zb_buffer_test_req_param_t *req_param;

  TRACE_MSG(TRACE_APS1, ">>brcast_tbuffer_request", (FMT__0));

  req_param = ZB_BUF_GET_PARAM(param, zb_buffer_test_req_param_t);
  BUFFER_TEST_REQ_SET_DEFAULT(req_param);
  req_param->dst_addr = 0xffff;

  zb_tp_buffer_test_request(param, NULL);

  TRACE_MSG(TRACE_APS1, "<<brcast_tbuffer_request", (FMT__0));
}


/*! @} */

