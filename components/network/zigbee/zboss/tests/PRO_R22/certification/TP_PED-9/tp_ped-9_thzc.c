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
/* PURPOSE: TP/PED-9 Child Table Management: Child device Leave and Rejoin when age out by Parent
*/

#define ZB_TEST_NAME TP_PED_9_THZC
#define ZB_TRACE_FILE_ID 40718
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

static const zb_ieee_addr_t g_ieee_addr_thzc = IEEE_ADDR_TH_ZC;
static const zb_ieee_addr_t g_ieee_addr_dutzed = IEEE_ADDR_DUT_ZED;

static void change_zed_device_timeout(zb_uint8_t param);

MAIN()
{
  ARGV_UNUSED;


  ZB_INIT("zdo_1_thzc");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif


  zb_set_pan_id(0x1aaa);
  zb_set_use_extended_pan_id(g_ext_pan_id);
  zb_set_long_address(g_ieee_addr_thzc);

  zb_secur_setup_nwk_key((zb_uint8_t*) g_nwk_key0, 0);
  zb_secur_setup_nwk_key((zb_uint8_t*) g_nwk_key1, 1);
  zb_set_nvram_erase_at_start(ZB_TRUE);

  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zc_role();

  zb_cert_test_set_keepalive_mode(ED_TIMEOUT_REQUEST_KEEPALIVE);

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

  TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  switch (sig)
  {
    case ZB_ZDO_SIGNAL_DEFAULT_START:
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ZB_BDB_SIGNAL_DEVICE_REBOOT:
      TRACE_MSG(TRACE_APS1, "Device started OK, status %d", (FMT__D, status));
    break; /* ZB_ZDO_SIGNAL_DEFAULT_START */

    case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
      TRACE_MSG(TRACE_APS1, "signal: ZB_ZDO_SIGNAL_DEVICE_ANNCE, status %d", (FMT__D, status));
      if (status == 0)
      {
        ZB_SCHEDULE_ALARM(change_zed_device_timeout, 0, 50 * ZB_TIME_ONE_SECOND);
      }
      break; /* ZB_ZDO_SIGNAL_DEVICE_ANNCE */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}


static void change_zed_device_timeout(zb_uint8_t param)
{
  zb_neighbor_tbl_ent_t *nbt_ent;
  zb_ret_t ret = RET_OK;

  ZVUNUSED(param);
  TRACE_MSG(TRACE_APS2, ">>change_zed_device_timeout", (FMT__0));

  ret = zb_nwk_neighbor_get_by_ieee((zb_uint8_t*) g_ieee_addr_dutzed, &nbt_ent);
  if (ret == RET_OK)
  {
    TRACE_MSG(TRACE_APS3, "neighbor with ieee = " TRACE_FORMAT_64 " found: nbt = %p",
              (FMT__A_P, TRACE_ARG_64(g_ieee_addr_dutzed), nbt_ent));
    TRACE_MSG(TRACE_APS3, "set device timeout to 10 seconds", (FMT__0));
    zb_init_ed_aging(nbt_ent, ED_AGING_TIMEOUT_10SEC, ZB_TRUE);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "entry with ieee address " TRACE_FORMAT_64 " not exist, ret = %h",
              (FMT__A_H, TRACE_ARG_64(g_ieee_addr_dutzed), ret));
  }

  TRACE_MSG(TRACE_APS2, "<<change_zed_device_timeout", (FMT__0));
}


/*! @} */
