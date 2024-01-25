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
/* PURPOSE: DUT ZC: join devices using install codes.
*/

#define ZB_TEST_NAME CS_ICK_TC_02_DUT_USING_IC
#define ZB_TRACE_FILE_ID 41019
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"
#include "cs_ick_tc_02_common.h"


/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif


static void enter_wrong_ic(zb_uint8_t param);
static void enter_correct_ic(zb_uint8_t param);


static zb_uint8_t s_temp_ic[16+2];



MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_dut2");

  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_dut2);

  /* let's always be coordinator */
  ZB_AIB().aps_designated_coordinator = 1;
  zb_secur_setup_nwk_key(g_nwk_key, 0);

  ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
  ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;
  ZB_BDB().bdb_mode = 1;
  ZB_BDB().bdb_join_uses_install_code_key = 1;
  ZB_AIB().aps_use_nvram = 1;

  if (zdo_dev_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
  }
  else
  {
    zdo_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}

/* [wrong_ic] */
static void enter_wrong_ic(zb_uint8_t param)
{
  int i;
  zb_ret_t ret;

  ZVUNUSED(param);

  ZB_MEMCPY(s_temp_ic, g_ic1, 18);
  for (i = 0; i < 4; ++i)
  {
    s_temp_ic[i] = ~s_temp_ic[i];
  }
  ret = zb_secur_ic_add(g_ieee_addr_thr1, s_temp_ic);
/* [wrong_ic] */
  if (ret != RET_OK)
  {
    TRACE_MSG(TRACE_ZDO2, "DUT: adding wrong ic - ic rejected", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ZDO2, "DUT: adding wrong ic - ic accepted (error)", (FMT__0));
  }
  ZB_SCHEDULE_ALARM(enter_correct_ic, 0, INSTALL_INVALID_IC_CODE_DELAY);
}


static void enter_correct_ic(zb_uint8_t param)
{
  ZVUNUSED(param);
  TRACE_MSG(TRACE_ZDO2, "DUT: adding correct ic, then join thr1", (FMT__0));
  zb_secur_ic_add(g_ieee_addr_thr1, g_ic1);
  zb_secur_ic_add(g_ieee_addr_thrx, g_ic2);
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        break;

      case ZB_BDB_SIGNAL_STEERING:
        ZB_SCHEDULE_ALARM(enter_wrong_ic, 0, INSTALL_INVALID_IC_CODE_DELAY);
        break;

      default:
        TRACE_MSG(TRACE_APS1, "Unknown signal - %hd", (FMT__H, sig));
    }
  }
  else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
  {
    TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
  }

  zb_free_buf(ZB_BUF_FROM_REF(param));
}


/*! @} */
