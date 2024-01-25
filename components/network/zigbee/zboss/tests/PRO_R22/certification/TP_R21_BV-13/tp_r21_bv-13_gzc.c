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

#define ZB_TEST_NAME TP_R21_BV_13_GZC
#define ZB_TRACE_FILE_ID 40587

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

static const zb_ieee_addr_t g_ieee_addr_gzc = IEEE_ADDR_gZC;

static void test_transport_key(zb_uint8_t param);
static void test_switch_key(zb_uint8_t param);
/* zc functions */
static void brcast_transport_key(zb_uint8_t param);
static void brcast_switch_key(zb_uint8_t param);


MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("zdo_1_gzc");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zc_role();
  zb_set_max_children(2);

  zb_set_long_address(g_ieee_addr_gzc);
  zb_set_pan_id(g_pan_id);
  zb_set_use_extended_pan_id(g_ext_pan_id);

  zb_secur_setup_nwk_key((zb_uint8_t*) g_nwk_key0, 0);
  zb_secur_setup_nwk_key((zb_uint8_t*) g_nwk_key1, 1);
  zb_zdo_set_aps_unsecure_join(ZB_TRUE);

#ifdef ZB_USE_NVRAM
  /* ZC MUST use NVRAM to store 2 keys at least */
//  ZB_AIB().aps_use_nvram = 0;
#endif

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
        ZB_SCHEDULE_ALARM(test_transport_key, 0, TEST_ZC_TRANSPORT_KEY_DELAY);
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


static void test_transport_key(zb_uint8_t param)
{
  ZVUNUSED(param);
  zb_buf_get_out_delayed(brcast_transport_key);
  ZB_SCHEDULE_ALARM(test_switch_key, 0, TEST_ZC_SWITCH_KEY_DELAY);
}


static void test_switch_key(zb_uint8_t param)
{
  ZVUNUSED(param);
  zb_buf_get_out_delayed(brcast_switch_key);
}


/* zc functions */
static void brcast_transport_key(zb_uint8_t param)
{
  zb_uint16_t *word_p;

  TRACE_MSG(TRACE_APS2, ">>brcast_transport_key", (FMT__0));

  if (!param)
  {
    TRACE_MSG(TRACE_ERROR, "brcast_transport_key: error - unable to get data buffer", (FMT__0));
    return;
  }

  word_p = (zb_uint16_t*)  ZB_BUF_GET_PARAM(param, zb_uint16_t);
  *word_p = 0xffff;
  zb_secur_send_nwk_key_update_br(param);

  TRACE_MSG(TRACE_APS2, "<<brcast_transport_key", (FMT__0));
}


static void brcast_switch_key(zb_uint8_t param)
{
  zb_uint16_t *word_p;

  TRACE_MSG(TRACE_APS2, ">>brcast_switch_key", (FMT__0));

  if (!param)
  {
    TRACE_MSG(TRACE_ERROR, "brcast_transport_key: error - unable to get data buffer", (FMT__0));
    return;
  }

  word_p = (zb_uint16_t*)  ZB_BUF_GET_PARAM(param, zb_uint16_t);
  *word_p = 0xffff;
  zb_secur_switch_nwk_key_br(param);

  TRACE_MSG(TRACE_APS2, "<<brcast_switch_key", (FMT__0));
}


/*! @} */

