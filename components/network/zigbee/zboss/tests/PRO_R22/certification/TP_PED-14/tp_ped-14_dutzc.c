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
/* PURPOSE: TP/PED-14 Child Table Management: Parent Announcement – Splitting & Randomization (DUT ZC)
*/

#define ZB_TEST_NAME TP_PED_14_DUTZC
#define ZB_TRACE_FILE_ID 40916

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_console_monitor.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

static const zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;

MAIN()
{
  ARGV_UNUSED;
	
	char command_buffer[100], *command_ptr;
  char next_cmd[40];
  zb_bool_t res;

  ZB_INIT("zdo_1_dutzc");
#if UART_CONTROL	
	test_control_init();
#endif

  zb_set_long_address(g_ieee_addr_dut);
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zc_role();
  zb_set_max_children(ZB_DEFAULT_MAX_CHILDREN);
  zb_aib_tcpol_set_update_trust_center_link_keys_required(ZB_FALSE);

  zb_set_pan_id(TEST_PAN_ID);

#ifdef SECURITY_LEVEL
  zb_cert_test_set_security_level(SECURITY_LEVEL);
#endif

#ifdef ZB_USE_NVRAM
  zb_cert_test_set_aps_use_nvram();
#endif

  zb_secur_setup_nwk_key((zb_uint8_t*) g_nwk_key, 0);

  TRACE_MSG(TRACE_APP1, "Send 'erase' for flash erase or just press enter to be continued \n", (FMT__0));
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_handler);
  zb_console_monitor_get_cmd((zb_uint8_t*)command_buffer, sizeof(command_buffer));
  command_ptr = (char *)(&command_buffer);
  res = parse_command_token(&command_ptr, next_cmd, sizeof(next_cmd));
  if (strcmp(next_cmd, "erase") == 0)
    zb_set_nvram_erase_at_start(ZB_TRUE);
  else
    zb_set_nvram_erase_at_start(ZB_FALSE);

  zb_bdb_set_legacy_device_support(ZB_TRUE);
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);

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

static void reopen_network(zb_bufid_t bufid)
{
  ZVUNUSED(bufid);

  TRACE_MSG(TRACE_APS1, "Reopen the Zigbee network", (FMT__0));
  bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
  ZB_SCHEDULE_APP_ALARM(reopen_network, 0, ZB_TIME_ONE_SECOND * 180);
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
      if (status == 0)
      {
        TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
        ZB_SCHEDULE_APP_ALARM(reopen_network, 0, ZB_TIME_ONE_SECOND * 180);
      }
      else
      {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, status));
      }
      break; /* ZB_ZDO_SIGNAL_DEFAULT_START */

    default:
      if (status == 0)
      {
        TRACE_MSG(TRACE_APS1, "Unknown signal, status OK", (FMT__0));
      }
      else
      {
        TRACE_MSG(TRACE_ERROR, "Unknown signal, status %d", (FMT__D, status));
      }
      break;
  }

  zb_buf_free(param);
}

/*! @} */
