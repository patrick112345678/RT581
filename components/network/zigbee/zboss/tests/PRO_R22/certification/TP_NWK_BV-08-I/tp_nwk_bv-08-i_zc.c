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
/* PURPOSE: TP/NWK/BV-08-I Buffering for Sleeping Children
*/

#define ZB_TEST_NAME TP_NWK_BV_08_I_ZC
#define ZB_TRACE_FILE_ID 40730
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "../common/zb_cert_test_globals.h"

//#define TEST_CHANNEL (1l << 24)
#define HW_SLEEP_ADDITIONAL 60*ZB_TIME_ONE_SECOND

static const zb_ieee_addr_t g_ieee_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
static const zb_ieee_addr_t g_zr_addr = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
static const zb_ieee_addr_t g_ed_ieee_addr = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


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
  zb_set_long_address(g_ieee_addr);

  zb_set_max_children(1);

  /* turn off security */
  /* zb_cert_test_set_security_level(0); */

  MAC_ADD_VISIBLE_LONG((zb_uint8_t*) g_zr_addr);

  zb_set_nvram_erase_at_start(ZB_TRUE);

  if ( zboss_start() != RET_OK )
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
  ZVUNUSED(param);
  TRACE_MSG(TRACE_APS3, "packets_sent_cb", (FMT__0));
}

static void start_packet_send(zb_uint8_t param, zb_uint16_t len)
{
  TRACE_MSG(TRACE_INFO3, "###start_packet_send", (FMT__0));

/*  ZVUNUSED(param);*/

  TRACE_MSG(TRACE_APS3, "start_packet_send", (FMT__0));

  if (!param)
  {
    zb_buf_get_out_delayed_ext(start_packet_send, len, 0);
  }
  else
  {
    zb_tp_transmit_counted_packets_param_t *params;

    params = ZB_BUF_GET_PARAM(param, zb_tp_transmit_counted_packets_param_t);
    BUFFER_COUNTED_TEST_REQ_SET_DEFAULT(params);
    params->len = len;
    params->packets_number = 1;
    params->idle_time = 10;
    params->dst_addr = zb_address_short_by_ieee((zb_uint8_t*) g_ed_ieee_addr);
    params->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;

    TRACE_MSG(TRACE_APS3, "dst addr %d", (FMT__D, params->dst_addr));
    zb_tp_transmit_counted_packets_req(param, packets_sent_cb);
  }
}


static void start_packet_send_2_param(zb_uint8_t len)
{
  zb_buf_get_out_delayed_ext(start_packet_send, len, 0);
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

  TRACE_MSG(TRACE_APP1, "zboss_signal_handler: status %hd signal %hd",
            (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));

  if (status == 0)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
        break;

      case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
      {
          zb_zdo_signal_device_annce_params_t *params;

          TRACE_MSG(TRACE_APP1, "signal: ZB_ZDO_SIGNAL_DEVICE_ANNCE", (FMT__0));

          params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_annce_params_t);

          if (ZB_IEEE_ADDR_CMP(params->ieee_addr, g_ed_ieee_addr))
          {
            ZB_SCHEDULE_ALARM(start_packet_send_2_param, 0x0A, 25*ZB_TIME_ONE_SECOND);
            ZB_SCHEDULE_ALARM(start_packet_send_2_param, 0x20, 25*ZB_TIME_ONE_SECOND + ZB_ZDO_INDIRECT_POLL_TIMER + ZB_TIME_ONE_SECOND);
            ZB_SCHEDULE_ALARM(start_packet_send_2_param, 0x20, 26*ZB_TIME_ONE_SECOND + ZB_ZDO_INDIRECT_POLL_TIMER + ZB_TIME_ONE_SECOND);
          }
        }
        break; /* ZB_ZDO_SIGNAL_DEVICE_ANNCE */

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
