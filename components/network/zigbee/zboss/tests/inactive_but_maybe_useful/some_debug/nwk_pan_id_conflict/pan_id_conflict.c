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
/* PURPOSE: pan_id_conflict sample (HA profile)
*/
#define ZB_TRACE_FILE_ID 47638
#include "pan_id_conflict.h"
#include "zb_g_context.h"

zb_ieee_addr_t g_zr_addr = APP_IEEE_ADDRESS;

app_ctx_t app_ctx;

#if !defined ZB_ROUTER_ROLE
#error define ZB_ROUTER_ROLE to app demo
#endif

MAIN()
{
  ARGV_UNUSED;

#ifdef LIGHT_SAMPLE_BUTTONS
  pan_id_conflict_hal_init();
#endif

  ZB_BZERO(&app_ctx, sizeof(app_ctx_t));

//! [switch_trace]
  ZB_SET_TRACE_ON();
//! [switch_trace]
  ZB_SET_TRAF_DUMP_ON();

  ZB_INIT("pan_id_conflict");

  zb_set_long_address(g_zr_addr);
  zb_set_network_router_role(APP_DEFAULT_APS_CHANNEL_MASK);
  zb_set_max_children(0);
  zb_set_nvram_erase_at_start(ZB_FALSE);
  zb_set_keepalive_timeout(ZB_MILLISECONDS_TO_BEACON_INTERVAL(3000));

  if (zboss_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "ERROR dev_start failed", (FMT__0));
  }
  else
  {
    zboss_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}

#if defined ZB_CERTIFICATION_HACKS
static void app_enable_send_incorrect_beacon(zb_uint8_t param)
{
  ZVUNUSED(param);

  app_ctx.enable_send_incorrect_beacon = !app_ctx.enable_send_incorrect_beacon;

  ZG->cert_hacks.set_empty_beacon_payload = app_ctx.enable_send_incorrect_beacon;

  ZB_GET_OUT_BUF_DELAYED(zb_nwk_update_beacon_payload);

  ZB_SCHEDULE_ALARM(app_enable_send_incorrect_beacon, 0, ZB_TIME_ONE_SECOND * 10);
}
#endif

void zboss_signal_handler(zb_uint8_t param)
{
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
#if defined ZB_CERTIFICATION_HACKS
        ZB_SCHEDULE_ALARM(app_enable_send_incorrect_beacon, 0, ZB_TIME_ONE_SECOND * 10);
#endif
        break;

      case ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY:
        TRACE_MSG(TRACE_APP1, "Loading application production config", (FMT__0));
        break;

      default:
        TRACE_MSG(TRACE_ERROR, "Unknown signal %hd", (FMT__H, sig));
    }
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
  }

  if (param)
  {
    zb_free_buf(ZB_BUF_FROM_REF(param));
  }
}
