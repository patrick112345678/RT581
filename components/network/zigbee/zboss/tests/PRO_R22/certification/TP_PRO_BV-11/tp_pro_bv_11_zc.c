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
#define ZB_TEST_NAME TP_PRO_BV_11_ZC
#define ZB_TRACE_FILE_ID 40583

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"

#ifndef ZB_PRO_STACK
#error This test is not applicable for 2007 stack. ZB_PRO_STACK should be defined to enable PRO.
#endif

#define TEST_MTORR_RADIUS 3
#define TEST_MTORR_DISC_TIME 30

static const zb_ieee_addr_t g_ieee_addr_c = IEEE_ADDR_C;
static const zb_ieee_addr_t g_ieee_addr_r2 = IEEE_ADDR_R2;

static void send_data(zb_uint8_t param);

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
  zb_aib_tcpol_set_update_trust_center_link_keys_required(ZB_FALSE);

  /* set ieee addr */
  zb_set_long_address(g_ieee_addr_c);

  zb_set_pan_id(0x1aaa);

  /* zb_cert_test_set_security_level(0); */

  zb_set_max_children(1);
  MAC_ADD_VISIBLE_LONG((zb_uint8_t*) g_ieee_addr_r2);

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


static zb_uint16_t addr_ass_cb(zb_ieee_addr_t ieee_addr)
{
  zb_uint16_t res = (zb_uint16_t)~0;

  TRACE_MSG(TRACE_APS3, ">>addr_assignmnet_cb " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(ieee_addr)));
  if (ZB_IEEE_ADDR_CMP(ieee_addr, g_ieee_addr_r2))
  {
    res = g_short_addr_r2;
  }

  TRACE_MSG(TRACE_APS3, "<<addr_assignmnet_cb: res = 0x%x;", (FMT__H, res));
  return res;
}


static void device_annce_cb(zb_zdo_signal_device_annce_params_t *da)
{
  static zb_uint32_t count = 0;

  TRACE_MSG(TRACE_APP2, ">> device_annce_cb, da %p", (FMT__P, da));

  zb_start_concentrator_mode(TEST_MTORR_RADIUS, TEST_MTORR_DISC_TIME);

  if (++count == 3)
  {
    /* Send test counted packets by predefined short address */
    ZB_SCHEDULE_ALARM(send_data, 0, 30 * ZB_TIME_ONE_SECOND);
  }

  TRACE_MSG(TRACE_APP2, "<< device_annce_cb", (FMT__0));
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

  TRACE_MSG(TRACE_ERROR, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  if (0 == status)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));
        zb_start_concentrator_mode(TEST_MTORR_RADIUS, TEST_MTORR_DISC_TIME);

        /* Setup address assignment callback */
        zb_nwk_set_address_assignment_cb(addr_ass_cb);
        break;

      case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
      {
	zb_zdo_signal_device_annce_params_t *dev_annce_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_annce_params_t);
	device_annce_cb(dev_annce_params);
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

static void buffer_test_cb(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APS1, "buffer_test_cb", (FMT__H, param));
  if (param == ZB_TP_BUFFER_TEST_OK)
  {
    TRACE_MSG(TRACE_APS1, "buffer_test_cb: status OK", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_APS1, "buffer_test_cb: status ERROR()", (FMT__H, param));
  }
}

static void send_data(zb_uint8_t param)
{
  zb_tp_transmit_counted_packets_param_t *params;
  zb_bufid_t buf = ZB_UNDEFINED_BUFFER;
  zb_uint16_t dst_short_addr = g_short_addr_r4;
  ZVUNUSED(param);

  TRACE_MSG(TRACE_APS3, ">>send_data", (FMT__0));

  if (dst_short_addr == (zb_uint16_t)~0)
  {
    TRACE_MSG(TRACE_INFO3, "<<send_data: error - unable to get short dst addr", (FMT__0));
    ZB_EXIT(1);
  }

  buf = zb_buf_get_out();
  if (buf)
  {
    params = ZB_BUF_GET_PARAM(buf, zb_tp_transmit_counted_packets_param_t);
    BUFFER_COUNTED_TEST_REQ_SET_DEFAULT(params);
    params->len             = TEST_BUFFER_LEN;
    params->packets_number  = 1;
    params->idle_time       = TEST_PACKET_DELAY;
    params->src_ep          = 0x01;
    params->dst_addr        = dst_short_addr;
    params->addr_mode       = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
    /* Not sure need really 10, but it is 10 in the certification log from ZB Alliance */
    params->radius          = 10;

    TRACE_MSG(TRACE_APS3, "<<send_data: dst addr %d", (FMT__D, params->dst_addr));
    zb_tp_transmit_counted_packets_req(buf, buffer_test_cb);
  }
  else
  {
    TRACE_MSG(TRACE_INFO3, "<<send_data: error - out buf alloc failed", (FMT__0));
    ZB_EXIT(1);
  }
}
