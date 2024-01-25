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
/* PURPOSE: ZC
*/

#define ZB_TEST_NAME TP_PRO_BV_04_ZC
#define ZB_TRACE_FILE_ID 40580

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_nwk_neighbor.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"

static const zb_ieee_addr_t g_ieee_addr_c = IEEE_ADDR_C;
static const zb_ieee_addr_t g_ieee_addr_r1 = IEEE_ADDR_R1;
static const zb_ieee_addr_t g_ieee_addr_r2 = IEEE_ADDR_R2;
static const zb_ieee_addr_t g_ieee_addr_ed = IEEE_ADDR_ED;


static zb_uint16_t s_ed_short_addr = 0;

MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("zdo_1_zc");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	
  zb_set_long_address(g_ieee_addr_c);
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zc_role();
  zb_set_pan_id(0x1aaa);

  /* let's always be coordinator */
  zb_set_max_children(2);

  /* turn off security */
  /* zb_cert_test_set_security_level(0); */

  /* Just to have same addresses always - easier to analyze it */

  zb_set_nvram_erase_at_start(ZB_TRUE);

  MAC_ADD_VISIBLE_LONG((zb_uint8_t*)g_ieee_addr_r1);
  MAC_ADD_VISIBLE_LONG((zb_uint8_t*)g_ieee_addr_r2);

  ZB_CERT_HACKS().disable_in_out_cost_updating = 1;
  ZB_CERT_HACKS().delay_pending_tx_on_rresp = 1;
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

static void buffer_test_cb(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APS1, "buffer_test_cb %hd", (FMT__H, param));
  if (param == ZB_TP_BUFFER_TEST_OK)
  {
    TRACE_MSG(TRACE_APS1, "status OK", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_APS1, "status ERROR", (FMT__0));
  }
}

static void send_buffer_test_request(zb_uint8_t param)
{
  zb_bufid_t buf = zb_buf_get_out();
  zb_buffer_test_req_param_t *req_param;

  TRACE_MSG(TRACE_ERROR, ">>send_buffer_test_request", (FMT__0));

  ZVUNUSED(param);

  if (buf)
  {
    req_param = ZB_BUF_GET_PARAM(buf, zb_buffer_test_req_param_t);
    BUFFER_TEST_REQ_SET_DEFAULT(req_param);
    req_param->dst_addr = s_ed_short_addr;

    TRACE_MSG(TRACE_ERROR, "send to 0x%x", (FMT__H, req_param->dst_addr));

    zb_tp_buffer_test_request(buf, buffer_test_cb);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "TEST FAILED: Could not get out buf!", (FMT__0));
  }

  TRACE_MSG(TRACE_ERROR, "<<send_buffer_test_request", (FMT__0));
}

static void change_link_status(zb_uint8_t param)
{
  zb_neighbor_tbl_ent_t *nbt;
  ZVUNUSED(param);

  if (RET_OK == zb_nwk_neighbor_get_by_ieee((zb_uint8_t*)g_ieee_addr_r1, &nbt))
  {
    nbt->u.base.age = 0;
    nbt->u.base.outgoing_cost = 3;
    TRACE_MSG(TRACE_ZDO1, "ZR1 lqi was %hd. Modify outgoing_cost to %hd, lqi %hd", (FMT__H_H_H, nbt->lqi, nbt->u.base.outgoing_cost, NWK_COST_TO_LQI(3)));
    nbt->lqi = NWK_COST_TO_LQI(3);
  }
  if (RET_OK == zb_nwk_neighbor_get_by_ieee((zb_uint8_t*)g_ieee_addr_r2, &nbt))
  {
    nbt->u.base.age = 0;
    nbt->u.base.outgoing_cost = 7;
    TRACE_MSG(TRACE_ZDO1, "ZR1 lqi was %hd. Modify outgoing_cost to %hd, lqi %hd", (FMT__H_H_H, nbt->lqi, nbt->u.base.outgoing_cost, NWK_COST_TO_LQI(1)));
    nbt->lqi = NWK_COST_TO_LQI(1);
  }
  ZB_SCHEDULE_ALARM(change_link_status, 0, 1*ZB_TIME_ONE_SECOND);
}


static void device_annce_cb(zb_zdo_device_annce_t *da)
{
  TRACE_MSG(TRACE_ZDO2, ">>dev_annce_cb: short addr = %x", (FMT__H, da->nwk_addr));
  TRACE_MSG(TRACE_ZDO1, "dev_annce_cb: ieee = " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(da->ieee_addr)));

  if (ZB_IEEE_ADDR_CMP(g_ieee_addr_ed, da->ieee_addr) == ZB_TRUE)
  {
    s_ed_short_addr = da->nwk_addr;
    TRACE_MSG(TRACE_ZDO1, "s_ed_short_addr 0x%x", (FMT__D, s_ed_short_addr));
  }
  change_link_status(0);

  TRACE_MSG(TRACE_ZDO1, "<<dev_annce_cb", (FMT__0));
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

	zb_zdo_register_device_annce_cb(device_annce_cb);

        ZB_SCHEDULE_ALARM(change_link_status, 0, 1*ZB_TIME_ONE_SECOND);
        ZB_SCHEDULE_ALARM(send_buffer_test_request, 0, 60*ZB_TIME_ONE_SECOND);
#if 0
        /* Don't need second buffer test any more. Now work exactly as described in the test */
        ZB_SCHEDULE_ALARM(send_buffer_test_request, ZB_REF_FROM_BUF(zb_buf_get(ZB_TRUE, 0)), 65*ZB_TIME_ONE_SECOND);
#endif
        break;

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

