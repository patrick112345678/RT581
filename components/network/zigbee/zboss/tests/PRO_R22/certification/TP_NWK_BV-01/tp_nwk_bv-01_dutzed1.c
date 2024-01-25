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
/* PURPOSE: TP/NWK/BV-01 gZED1
*/


#define ZB_TEST_NAME TP_NWK_BV_01_DUTZED1
#define ZB_TRACE_FILE_ID 40847

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur_api.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"


/* For NS build first ieee addr byte should be unique */
static zb_ieee_addr_t g_ieee_addr = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static zb_ieee_addr_t d_ieee_addr = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};

static zb_bool_t aps_secure = ZB_FALSE;

#define TEST_PACKET_LENGTH 60
#define TEST_PACKET_COUNT 200
#define TEST_PACKET_DELAY 2000 /* ms */


/*! \addtogroup ZB_TESTS */
/*! @{ */


MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("zdo_dutzed1");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zed_role();

  #if defined ZB_USE_SLEEP
  zb_set_rx_on_when_idle(ZB_FALSE);
  #else
  zb_set_rx_on_when_idle(ZB_TRUE);
  #endif

  zb_set_long_address(g_ieee_addr);

  if (aps_secure)
  {
    //ZB_NIB().secure_all_frames = 0;
  }

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


static void packets_sent_cb(zb_uint8_t param)
{

  TRACE_MSG(TRACE_APS3, "###packets_sent_cb", (FMT__0));

  zb_buf_free(param);
}


static void start_packet_send_delayed(zb_uint8_t param)
{
  zb_tp_transmit_counted_packets_param_t *params;
  zb_bufid_t asdu;

  ZVUNUSED(param);

  TRACE_MSG(TRACE_APS3, "###start_packet_send", (FMT__0));

  asdu = zb_buf_get_out();
  if (!asdu)
  {
    TRACE_MSG(TRACE_ERROR, "out buf alloc failed!", (FMT__0));
  }
  else
  {
    //! [zb_tp_transmit_counted_packets_req]
    params = ZB_BUF_GET_PARAM(asdu, zb_tp_transmit_counted_packets_param_t);
    BUFFER_COUNTED_TEST_REQ_SET_DEFAULT(params);
    params->len = TEST_PACKET_LENGTH;
    params->packets_number = TEST_PACKET_COUNT;
    params->idle_time = TEST_PACKET_DELAY;
    params->dst_addr = zb_address_short_by_ieee(d_ieee_addr);
    params->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;

    TRACE_MSG(TRACE_APS3, "dst addr %d", (FMT__D, params->dst_addr));
    zb_tp_transmit_counted_packets_req(asdu, packets_sent_cb);
    //! [zb_tp_transmit_counted_packets_req]
  }
}


static void start_packet_send(zb_uint8_t param)
{
  ZVUNUSED(param);
  ZB_SCHEDULE_ALARM(start_packet_send_delayed, 0, 5 * ZB_TIME_ONE_SECOND);
}


static void start_get_short_addr(zb_uint8_t param)
{
  zb_address_ieee_ref_t ref_p;
  TRACE_MSG(TRACE_INFO3, "###start_get_short_addr", (FMT__0));
  if (zb_address_by_ieee(d_ieee_addr, ZB_TRUE, ZB_FALSE, &ref_p) == RET_OK)
  {
    zb_start_get_peer_short_addr(ref_p, start_packet_send, param);
  }
  else
  {
    TRACE_MSG(TRACE_INFO3, "###start_get_short_addr: unable to get ref to long addr", (FMT__0));
  }
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  if (0 == status)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));
        test_step_register(start_get_short_addr, 0, TP_NWK_BV_01_STEP_4_TIME_DUTZED1);
        test_control_start(TEST_MODE, TP_NWK_BV_01_STEP_4_DELAY_DUTZED1);
        break;

      case ZB_COMMON_SIGNAL_CAN_SLEEP:
#ifdef ZB_USE_SLEEP
    	  zb_sleep_now();
#endif /* ZB_USE_SLEEP */
        break;

      default:
        TRACE_MSG(TRACE_ERROR, "Unknown signal %hd", (FMT__H, sig));
    }
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, status));
  }

  zb_buf_free(param);
}


/*! @} */
