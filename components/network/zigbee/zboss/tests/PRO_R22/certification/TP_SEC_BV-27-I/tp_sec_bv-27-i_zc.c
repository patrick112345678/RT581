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
/* PURPOSE: 11.27 TP/SEC/BV-27-I Security NWK Key Switch (ZR unicast)
Objective: DUT as ZR receives a new NWK Key via unicast and switches.
*/

#define ZB_TEST_NAME TP_SEC_BV_27_I_ZC
#define ZB_TRACE_FILE_ID 40852
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

#ifndef ZB_CERTIFICATION_HACKS
#error Define ZB_CERTIFICATION_HACKS
#endif

static const zb_ieee_addr_t g_ieee_addr_c = IEEE_ADDR_C;
static const zb_ieee_addr_t g_ieee_addr_r1 = IEEE_ADDR_R1;
static const zb_ieee_addr_t g_ieee_addr_r2 = IEEE_ADDR_R2;

static zb_uint16_t TEST_PAN_ID = 0x1AAA;

static void send_data_cb(zb_uint8_t param);
static void send_data_bc_delayed(zb_uint8_t param);
static void send_data_bc(zb_uint8_t param);
static void send_tk_delayed(zb_uint8_t param);
static void send_tk(zb_uint8_t param);
static void send_key_switch(zb_uint8_t param);

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

  MAC_ADD_VISIBLE_LONG((zb_uint8_t*) g_ieee_addr_r1);

  zb_set_pan_id(TEST_PAN_ID);

  zb_set_max_children(3);

  zb_secur_setup_nwk_key((zb_uint8_t*) g_key0, 0);
  zb_secur_setup_nwk_key((zb_uint8_t*) g_key1, 1);

#ifdef SECURITY_LEVEL
  zb_cert_test_set_security_level(SECURITY_LEVEL);
#endif

  zb_zdo_set_aps_unsecure_join(INSECURE_JOIN_ZC);

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
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

  TRACE_MSG(TRACE_APP1, "zboss_signal_handler: status %hd signal %hd",
            (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));

  switch (sig)
  {
    case ZB_ZDO_SIGNAL_DEFAULT_START:
      if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
      {
	TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));

#ifdef TEST_ENABLED
	/* send Transport Key */
	ZB_SCHEDULE_ALARM(send_tk, 0, TIME_ZC_TK1);
	/* send Key Switch */
	ZB_SCHEDULE_ALARM(send_key_switch, 0, TIME_ZC_KEY_SWITCH1);
	/* send Test Buffer Request by the addr 0xFFFF */
	ZB_SCHEDULE_ALARM(send_data_bc, 0, TIME_ZC_DATA2);
#endif	/* TEST_ENABLED */

      }
      else
      {
	TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d",
		  (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
      }
      break; /* ZB_ZDO_SIGNAL_DEFAULT_START */

    default:
      if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
      {
	TRACE_MSG(TRACE_APS1, "zboss_signal_handler: status OK, status %d",
		  (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
      }
      else
      {
	TRACE_MSG(TRACE_ERROR, "zboss_signal_handler: status FAILED, status %d",
		  (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
      }
      break;
  }

  zb_buf_free(param);
}

static void send_data_cb(zb_uint8_t param)
{
  if (!param)
  {
    TRACE_MSG(TRACE_INFO1, "send_data_cb: status OK", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_INFO1, "send_data_cb: status FAILED", (FMT__0));
  }
}

static void send_data_bc_delayed(zb_bufid_t buf)
{
  zb_buffer_test_req_param_t *req_param;

  TRACE_MSG(TRACE_APP3, "send_data to 0xFFFF", (FMT__0));
  if (!buf)
  {
    TRACE_MSG(TRACE_ERROR, "send_data: unable to get data buffer", (FMT__0));
    ZB_EXIT(1);
  }

  req_param = ZB_BUF_GET_PARAM(buf, zb_buffer_test_req_param_t);
  BUFFER_TEST_REQ_SET_DEFAULT(req_param);
  req_param->dst_addr = zb_address_short_by_ieee((zb_uint8_t*)g_ieee_addr_r2);

  zb_tp_buffer_test_request(buf, send_data_cb);
}

static void send_data_bc(zb_uint8_t param)
{
  ZVUNUSED(param);

  zb_buf_get_out_delayed(send_data_bc_delayed);
}

static void send_tk_delayed(zb_bufid_t buf)
{
  zb_apsme_transport_key_req_t *req = NULL;
  zb_uint8_t *key = NULL;

  TRACE_MSG(TRACE_APP3, ">>send_tk", (FMT__0));

  if (!buf)
  {
    TRACE_MSG(TRACE_ERROR, "send_tk: error - unable to get data buffer", (FMT__0));
    ZB_EXIT(1);
  }

  req = ZB_BUF_GET_PARAM(buf, zb_apsme_transport_key_req_t);
  req->addr_mode = ZB_ADDR_64BIT_DEV;
  ZB_IEEE_ADDR_COPY(req->dest_address.addr_long, g_ieee_addr_r1);
  req->key.nwk.use_parent = ZB_FALSE;
  req->key_type = ZB_STANDARD_NETWORK_KEY;
  req->key.nwk.key_seq_number = 1;
  key = secur_nwk_key_by_seq(1);
  if (!key)
  {
    TRACE_MSG(TRACE_ERROR, "send_tk: error - no nwk", (FMT__0));
    ZB_EXIT(1);
  }
  ZB_MEMCPY(req->key.nwk.key, key, ZB_CCM_KEY_SIZE);
  ZB_SCHEDULE_CALLBACK(zb_apsme_transport_key_request, buf);

  TRACE_MSG(TRACE_APP3, "<<send_tk", (FMT__0));
}

static void send_tk(zb_uint8_t param)
{
  ZVUNUSED(param);

  zb_buf_get_out_delayed(send_tk_delayed);
}

static void send_key_switch(zb_uint8_t param)
{
  ZVUNUSED(param);

  zb_buf_get_out_delayed(zb_secur_switch_nwk_key_br);
}

