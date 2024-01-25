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
/* PURPOSE: Parent loss test - DUT ZED test harness
*/


#define ZB_TRACE_FILE_ID 63795
#include "zboss_api.h"
#include "pl_common.h"

#ifdef ZB_ROUTER_ROLE
#error End device role is not compiled!
#endif

zb_ieee_addr_t g_zed_addr = PL_ZED_IEEE_ADDR;

static void pl_dut_zed_init(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  zb_set_long_address(g_zed_addr);
  zb_set_network_ed_role_legacy(TEST_CHANNEL_MASK);

  zb_set_rx_on_when_idle(0);
}


MAIN()
{
  ARGV_UNUSED;

  ZB_SET_TRACE_ON();
  ZB_SET_TRAF_DUMP_ON();

  ZB_INIT("pl_dut_zed");

  pl_dut_zed_init(0);

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


void zboss_signal_handler(zb_uint8_t param)
{
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  TRACE_MSG(TRACE_ZCL1, ">> zboss_signal_handler %h", (FMT__H, param));

  if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
  {
    TRACE_MSG(TRACE_APP1, "Skip production config ready signal %d",
              (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));

    zb_free_buf(ZB_BUF_FROM_REF(param));
    return;
  }

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_SIGNAL_DEVICE_FIRST_START:
        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
        break;

      default:
        TRACE_MSG(TRACE_APP1, "Unknown signal", (FMT__0));
    }
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d",
              (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
  }

  if (param)
  {
    zb_free_buf(ZB_BUF_FROM_REF(param));
  }

  TRACE_MSG(TRACE_ZCL1, "<< zboss_signal_handler", (FMT__0));
}
