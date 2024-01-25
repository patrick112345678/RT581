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

#define ZB_TEST_NAME TP_R20_BV_10_ZC
#define ZB_TRACE_FILE_ID 40918

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

//#define TEST_CHANNEL (1l << 24)

//#define TEST_WITH_EMBER_ROUTER

//ZR
#ifdef TEST_WITH_EMBER_ROUTER
static const zb_ieee_addr_t g_ieee_addr1 = {0xf0, 0x23, 0x07, 0x00, 0x00, 0xed, 0x21, 0x00};
#else
static const zb_ieee_addr_t g_ieee_addr1 = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
#endif

//ZC
static const zb_ieee_addr_t g_ieee_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
//ZED
static const zb_ieee_addr_t g_ieee_addr2 = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

//NWK keys
static const zb_uint8_t g_key_nwk[16] = { 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0, 0, 0, 0, 0, 0, 0, 0};
static const zb_uint8_t g_key_nwk2[16] = { 0x78, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99};

//APS keys
//key for ZR
static const zb_uint8_t g_key1[16] = { 0x12, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33};
//key for ZED
static const zb_uint8_t g_key2[16] = { 0x12, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33};

static const zb_ieee_addr_t g_aps_ext_pan_id = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

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

//#if 0   /* test with wrong pan_id after join */
  zb_set_pan_id(0x1aaa);
//#endif

  zb_set_use_extended_pan_id(g_aps_ext_pan_id);

  /* let's always be coordinator */
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zc_role();

  zb_secur_setup_nwk_key((zb_uint8_t*) g_key_nwk, 0);
  zb_secur_setup_nwk_key((zb_uint8_t*) g_key_nwk2, 1);
  zb_bdb_set_legacy_device_support(ZB_TRUE);

  zb_zdo_set_aps_unsecure_join(ZB_TRUE);

  zb_set_max_children(1);

#if 0
  ZB_AIB().enable_alldoors_key = ZB_TRUE;
#endif

  zb_set_nvram_erase_at_start(ZB_TRUE);
  if (zboss_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
  }
  else
  {
    ZB_CERT_HACKS().use_preconfigured_aps_link_key = 1U;

    zb_secur_update_key_pair((zb_uint8_t*) g_ieee_addr1, (zb_uint8_t*) g_key1, ZB_SECUR_GLOBAL_KEY, ZB_SECUR_VERIFIED_KEY, ZB_SECUR_KEY_SRC_UNKNOWN);
    zb_secur_update_key_pair((zb_uint8_t*) g_ieee_addr2, (zb_uint8_t*) g_key2, ZB_SECUR_GLOBAL_KEY, ZB_SECUR_VERIFIED_KEY, ZB_SECUR_KEY_SRC_UNKNOWN);
    zboss_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}


static void send_key_switch(zb_uint8_t param)
{
  ZVUNUSED(param);
  zb_buf_get_out_delayed(zb_secur_switch_nwk_key_br);
}


/* [zb_secur_send_nwk_key_update_br] */
static void send_nwk_key2(zb_uint8_t param)
{
  zb_bufid_t buf = zb_buf_get_out();
  ZVUNUSED(param);

  if (buf)
  {
    TRACE_MSG(TRACE_SECUR1, "transport key2 - broadcast", (FMT__0));

    *ZB_BUF_GET_PARAM(buf, zb_uint16_t) = ZB_NWK_BROADCAST_ALL_DEVICES;

    ZB_SCHEDULE_CALLBACK(zb_secur_send_nwk_key_update_br, buf);
    ZB_SCHEDULE_ALARM(send_key_switch, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(3000));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "TEST FAILED: Could not get buf!", (FMT__0));
  }
}
/* [zb_secur_send_nwk_key_update_br] */


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

        ZB_SCHEDULE_ALARM(send_nwk_key2, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(35*1000));
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


/*! @} */
