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
/* PURPOSE: DUT ZC
*/
#define ZB_TEST_NAME RTP_PRODCONF_02_DUT_ZC
#define ZB_TRACE_FILE_ID 40378

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"
#include "zb_mac_globals.h"

#include "rtp_prodconf_02_common.h"
#include "../common/zb_reg_test_globals.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error define ZB_USE_NVRAM
#endif

#if !defined ZB_NSNG

#ifndef ZB_PRODUCTION_CONFIG
#error define ZB_PRODUCTION_CONFIG
#endif

#ifndef ZB_MAC_CONFIGURABLE_TX_POWER
#error define ZB_MAC_CONFIGURABLE_TX_POWER
#endif

#endif  /* !ZB_NSNG */

static zb_uint8_t g_nwk_key[16] = ZB_REG_TEST_DEFAULT_NWK_KEY;
static zb_ieee_addr_t g_ieee_addr_dut_zc = IEEE_ADDR_DUT_ZC;

static void trace_production_config_tx_power(zb_uint8_t unused);

MAIN()
{
  ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
  ZB_SET_TRACE_LEVEL(4);
  ARGV_UNUSED;
  ZB_INIT("zdo_dut_zc");

  zb_set_long_address(g_ieee_addr_dut_zc);

  zb_secur_setup_nwk_key((zb_uint8_t*) g_nwk_key, 0);

  zb_reg_test_set_common_channel_settings();
  zb_set_network_coordinator_role((1l << TEST_CHANNEL));
  zb_set_nvram_erase_at_start(ZB_TRUE);

  if (zboss_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
  }
  else
  {
    zdo_main_loop();
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
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
      TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));
      break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY:
      TRACE_MSG(TRACE_APP1, "signal: ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY, status %d", (FMT__D, status));
      if (status == 0)
      {
        test_step_register(trace_production_config_tx_power, 0, RTP_PRODCONF_02_STEP_1_TIME_ZC);

        test_control_start(TEST_MODE, RTP_PRODCONF_02_STEP_1_DELAY_ZC);
      }
      break; /* ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}

#if !defined ZB_NSNG

static void trace_production_config_tx_power(zb_uint8_t unused)
{
  zb_uint8_t i;
  zb_uint8_t j;
  zb_uint8_t first_channel;
  zb_uint8_t last_channel;
  zb_int8_t out_dbm;
  zb_ret_t ret;

  ZVUNUSED(unused);

  TRACE_MSG(TRACE_APP1, "tx_power[default]: %hd", (FMT__H, MAC_CTX().default_tx_power));
  TRACE_MSG(TRACE_APP1, "tx_power[current]: %hd", (FMT__H, MAC_CTX().current_tx_power));

  for (i = ZB_CHANNEL_LIST_PAGE0_IDX; i <= ZB_CHANNEL_LIST_PAGE31_IDX; ++i)
  {
    switch (i)
    {
      case ZB_CHANNEL_LIST_PAGE0_IDX:
        first_channel = ZB_PAGE0_2_4_GHZ_CHANNEL_FROM;
        last_channel = ZB_PAGE0_2_4_GHZ_CHANNEL_TO;
        break;
      case ZB_CHANNEL_LIST_PAGE28_IDX:
        first_channel = ZB_PAGE28_SUB_GHZ_CHANNEL_FROM;
        last_channel = ZB_PAGE28_SUB_GHZ_CHANNEL_TO;
        break;
      case ZB_CHANNEL_LIST_PAGE29_IDX:
        first_channel = ZB_PAGE29_SUB_GHZ_CHANNEL_FROM;
        last_channel = ZB_PAGE29_SUB_GHZ_CHANNEL_TO + 1;
        break;
      case ZB_CHANNEL_LIST_PAGE30_IDX:
        first_channel = ZB_PAGE30_SUB_GHZ_CHANNEL_FROM;
        last_channel = ZB_PAGE30_SUB_GHZ_CHANNEL_TO;
        break;
      case ZB_CHANNEL_LIST_PAGE31_IDX:
        first_channel = ZB_PAGE31_SUB_GHZ_CHANNEL_FROM;
        last_channel = ZB_PAGE31_SUB_GHZ_CHANNEL_TO;
        break;
      default:
        break;
    }

    for (j = first_channel; j <= last_channel; ++j)
    {
      if (i == ZB_CHANNEL_LIST_PAGE29_IDX && j == last_channel)
      {
        /* special case for 62 channel from 29 page */
        j = 62;
      }

      ret = MAC_CTX().tx_power_provider(ZB_CHANNEL_PAGE_FROM_IDX(i), j, &out_dbm);

      if (ret != RET_OK)
      {
        TRACE_MSG(TRACE_ERROR, "Can not get TX power for page %hd, channel %hd", (FMT__H_H, ZB_CHANNEL_PAGE_FROM_IDX(i), j));
      }
      else
      {
        TRACE_MSG(TRACE_APP1, "tx_power[pg=%hd,ch=%hd]: %hd", (FMT__H_H_H, i, j, out_dbm));
      }
    }
  }
}

#else

static void trace_production_config_tx_power(zb_uint8_t unused)
{
  ZVUNUSED(unused);
}

#endif  /* !ZB_NSNG */
/*! @} */
