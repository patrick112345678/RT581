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
/* PURPOSE: TP/NWK/BV-07-I Network Broadcast to Router only
*/

#define ZB_TEST_NAME TP_NWK_BV_07_I_ZC
#define ZB_TRACE_FILE_ID 63887

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"

#define HW_SLEEP_ADDITIONAL 10*ZB_TIME_ONE_SECOND


enum brcast_idx_e
{
  BRCAST_TO_ALL,
  BRCAST_TO_ZR_ZC,
  BRCAST_TO_RX_ON
};

static zb_ieee_addr_t gZC_ieee_addr = IEEE_ADDR_gZC;
static zb_ieee_addr_t DUT_ZR1_ieee_addr = IEEE_ADDR_DUT_ZR1;

static void schedule_test_cmd(zb_uint8_t addr_idx);


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

  /* set ieee addr */
  zb_set_long_address(gZC_ieee_addr);
  zb_set_pan_id(TEST_PAN_ID);

  zb_set_max_children(1);

  /* turn off security */
  /* zb_cert_test_set_security_level(0); */

  MAC_ADD_VISIBLE_LONG((zb_uint8_t*) DUT_ZR1_ieee_addr);

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


static void test_buffer_request(zb_uint8_t param, zb_uint16_t addr)
{
  zb_buffer_test_req_param_t *req_param;

  TRACE_MSG(TRACE_APS1, "send_test_request to %d", (FMT__D, addr));
  req_param = ZB_BUF_GET_PARAM(param, zb_buffer_test_req_param_t);
  BUFFER_TEST_REQ_SET_DEFAULT(req_param);
  req_param->len = 0x10;
  req_param->dst_addr = addr;

  zb_tp_buffer_test_request(param, buffer_test_cb);
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

        ZB_SCHEDULE_ALARM(schedule_test_cmd, BRCAST_TO_ALL,   40 * ZB_TIME_ONE_SECOND + HW_SLEEP_ADDITIONAL);
        ZB_SCHEDULE_ALARM(schedule_test_cmd, BRCAST_TO_ZR_ZC, 50 * ZB_TIME_ONE_SECOND + HW_SLEEP_ADDITIONAL);
        ZB_SCHEDULE_ALARM(schedule_test_cmd, BRCAST_TO_RX_ON, 60 * ZB_TIME_ONE_SECOND + HW_SLEEP_ADDITIONAL);
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


static void schedule_test_cmd(zb_uint8_t addr_idx)
{
  zb_uint16_t addr = 0x0000;

  switch (addr_idx)
  {
    case BRCAST_TO_ALL:
      addr = 0xffff;
      break;
    case BRCAST_TO_ZR_ZC:
      addr = 0xfffc;
      break;
    case BRCAST_TO_RX_ON:
      addr = 0xfffd;
      break;
  }

  zb_buf_get_out_delayed_ext(test_buffer_request, addr, 0);
}




