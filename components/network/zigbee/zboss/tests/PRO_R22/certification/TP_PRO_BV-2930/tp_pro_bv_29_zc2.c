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
/* PURPOSE: Test for ZC application written using ZDO.
*/


#define ZB_TEST_NAME TP_PRO_BV_2930_ZC2
#define ZB_TRACE_FILE_ID 40646

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"

#define ZB_EXIT( _p )

/*! \addtogroup ZB_TESTS */
/*! @{ */

static const zb_ieee_addr_t g_zc2_addr = IEEE_ADDR_ZC2;

/* Key for ZC2 */
static zb_uint8_t g_nwk_key_zc2[16] = {0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc,
				0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd};

/*
  ZR joins to ZC, then sends APS packet.
 */
MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("zdo_4_zc2");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	
  /* switch security off */
  /* zb_cert_test_set_security_level(0); */

  zb_set_long_address(g_zc2_addr);
  zb_set_pan_id(0x1bbb);

  /* let's always be coordinator */
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zc_role();
  zb_aib_tcpol_set_update_trust_center_link_keys_required(ZB_FALSE);

  zb_set_max_children(1);

  MAC_ADD_INVISIBLE_SHORT(0x0000);

#ifdef ZB_CERTIFICATION_HACKS
  ZB_CERT_HACKS().disable_pan_id_conflict_detection = 1;
#endif

  zb_secur_setup_nwk_key((zb_uint8_t*) g_nwk_key_zc2, 0);

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

static void change_panid(zb_uint8_t param);


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
        change_panid(0);
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

static void shutdown_plz(zb_uint8_t param);

static void shutdown_plz(zb_uint8_t param)
{
  ZVUNUSED(param);
  return;
}


static void change_panid(zb_uint8_t param)
{
  zb_mlme_start_req_t * req;

  param = zb_buf_get_out();
  //! [zb_mlme_start_request]
  req = ZB_BUF_GET_PARAM(param, zb_mlme_start_req_t);

  TRACE_MSG(TRACE_NWK1, "Change PANID", (FMT__0));

  zb_set_pan_id(0x1aaa);
  zb_cert_test_set_nib_seq_number(zb_random()%256);
  ZB_MEMSET(req, 0, sizeof(*req));
  req->pan_id = 0x1aaa;
  req->logical_channel = zb_get_current_channel();
  req->channel_page = 0;
  req->pan_coordinator = ZB_U2B(ZB_IS_DEVICE_ZC());
  req->coord_realignment = 0;
  req->beacon_order = ZB_TURN_OFF_ORDER;
  req->superframe_order = ZB_TURN_OFF_ORDER;
  zb_cert_test_set_nwk_state(ZB_NLME_STATE_PANID_CONFLICT_RESOLUTION);
  ZB_SCHEDULE_CALLBACK(zb_mlme_start_request, param);
  //! [zb_mlme_start_request]
  ZB_SCHEDULE_ALARM(shutdown_plz, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(10000));
}


/*! @} */
