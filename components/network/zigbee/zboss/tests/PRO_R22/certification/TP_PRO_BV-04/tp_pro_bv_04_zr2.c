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
/* PURPOSE: ZR
*/

#define ZB_TEST_NAME TP_PRO_BV_04_ZR2
#define ZB_TRACE_FILE_ID 40577

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"

static const zb_ieee_addr_t g_ieee_addr_r1 = IEEE_ADDR_R1;
static const zb_ieee_addr_t g_ieee_addr_r2 = IEEE_ADDR_R2;


MAIN()
{
  ARGV_UNUSED;

  ZB_INIT("zdo_3_zr2");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	
  zb_set_long_address(g_ieee_addr_r2);
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zr_role();

  /* turn off security */
  /* zb_cert_test_set_security_level(0); */

  zb_set_max_children(1);

  /* Just to have same addresses always - easier to analyze it */

  zb_set_nvram_erase_at_start(ZB_TRUE);

  ZB_CERT_HACKS().disable_in_out_cost_updating = 1;
  ZB_CERT_HACKS().delay_pending_tx_on_rresp = 0;
  ZB_CERT_HACKS().use_route_for_neighbor = 1;

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

static void change_link_status(zb_uint8_t param)
{
  zb_neighbor_tbl_ent_t *nbt;
  ZVUNUSED(param);

  if (RET_OK == zb_nwk_neighbor_get_by_short(0, &nbt))
  {
    nbt->u.base.age = 0;
    nbt->u.base.outgoing_cost = 1;
    nbt->lqi = NWK_COST_TO_LQI(7);
  }

  if (RET_OK == zb_nwk_neighbor_get_by_ieee((zb_uint8_t*)g_ieee_addr_r1, &nbt))
  {
    nbt->u.base.age = 0;
    nbt->u.base.outgoing_cost = 1;
    nbt->lqi = NWK_COST_TO_LQI(1);
  }

  ZB_SCHEDULE_ALARM(change_link_status, 0, 1*ZB_TIME_ONE_SECOND);
}

static void zb_association_permit(zb_uint8_t param)
{
  //! [zb_get_in_buf]
#ifndef NCP_MODE_HOST
  zb_bufid_t buf = zb_buf_get(ZB_TRUE, 0);
  ZB_PIBCACHE_ASSOCIATION_PERMIT() = param;
  zb_nwk_update_beacon_payload(buf);
#else
  ZVUNUSED(param);
  ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
#endif
  //! [zb_get_in_buf]
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

  TRACE_MSG(TRACE_APP1, "zboss_signal_handler: status %hd signal %hd",
            (FMT__H_H, status, sig));

  if (0 == status)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
	//JJ zb_association_permit(0);
	ZB_SCHEDULE_CALLBACK(change_link_status, 0);
        break;
    case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
      //JJ zb_association_permit(1);
      default:
        break;
    }
  }
  else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
  {
    TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d",
              (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
  }

  if (param)
  {
    zb_buf_free(param);
  }
}

