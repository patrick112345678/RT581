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
/* PURPOSE: TH ZED1
*/

#define ZB_TEST_NAME RTP_ZDO_07_DUT_ZED
#define ZB_TRACE_FILE_ID 63992

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_zcl.h"
#include "zb_bdb_internal.h"
#include "zb_zcl.h"
#include "rtp_zdo_07_common.h"
#include "../common/zb_reg_test_globals.h"

#ifndef ZB_ED_ROLE
#error ED role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error ZB_USE_NVRAM is not compiled!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

void test_check_locks(zb_bool_t addr_present);

static zb_uint8_t g_nwk_key[16] = ZB_REG_TEST_DEFAULT_NWK_KEY;
static zb_ieee_addr_t g_ieee_addr_dut_zed = IEEE_ADDR_DUT_ZED;

/************************Main*************************************/
MAIN()
{
  ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP | TRACE_SUBSYSTEM_COMMON | TRACE_SUBSYSTEM_ZDO | TRACE_SUBSYSTEM_NWK | TRACE_SUBSYSTEM_SECUR);
  ZB_SET_TRACE_LEVEL(4);
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_dut_zed");

  zb_set_long_address(g_ieee_addr_dut_zed);

  zb_reg_test_set_common_channel_settings();
  zb_set_network_ed_role((1l << TEST_CHANNEL));
  zb_set_nvram_erase_at_start(ZB_FALSE);

  zb_secur_setup_nwk_key(g_nwk_key, 0);

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

/********************ZDO Startup*****************************/
ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  switch (sig)
  {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));
      if (status == 0)
      {
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        test_check_locks(ZB_TRUE);
      }
      break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_DEVICE_REBOOT:
      TRACE_MSG(TRACE_APP1, "signal: ZB_BDB_SIGNAL_DEVICE_REBOOT, status %d", (FMT__D, status));
      if (status == 0)
      {
        test_check_locks(ZB_TRUE);
      }
      break; /* ZB_BDB_SIGNAL_DEVICE_REBOOT */

    case ZB_ZDO_SIGNAL_LEAVE:
      TRACE_MSG(TRACE_APP1, "signal: ZB_ZDO_SIGNAL_LEAVE, status %d", (FMT__D, status));
      if (status == 0)
      {
        test_check_locks(ZB_FALSE);
      }
      break; /* ZB_ZDO_SIGNAL_LEAVE */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}

void test_check_locks(zb_bool_t addr_present)
{
  zb_uint8_t i;
  zb_uint8_t ieee_found = 0;

  TRACE_MSG(TRACE_APP1, ">> test_check_locks %d", (FMT__H, test_check_locks));

  for (i = 0; i < ZB_IEEE_ADDR_TABLE_SIZE; i++)
  {
    zb_address_map_t *ent;
    zb_ieee_addr_t ieee_addr;

    ent = &ZG->addr.addr_map[i];

    if (ZG->addr.addr_map[i].used)
    {
      if (ent->redirect_type == ZB_ADDR_REDIRECT_NONE)
      {
        zb_ieee_addr_decompress(ieee_addr, &ent->ieee_addr);

        if (ZB_IEEE_ADDR_CMP(ieee_addr, g_ieee_addr_dut_zed))
        {
          TRACE_MSG(TRACE_APP1, "test_check_locks: address found", (FMT__0));

          ZB_ASSERT(addr_present);

          /* Device's address should be locked only once during association. */
          ZB_ASSERT(ent->lock_cnt == 1);

          /* Only one entry for address */
          ieee_found++;
          ZB_ASSERT(ieee_found == 1);
        }
      }
    }
  }

  TRACE_MSG(TRACE_APP1, "<< test_check_locks", (FMT__0));
}


/*! @} */
