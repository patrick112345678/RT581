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

/*
  gZR
*/

#define ZB_TEST_NAME TP_PRO_BV_09_ZR1
#define ZB_TRACE_FILE_ID 40699

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "../common/zb_cert_test_globals.h"


#ifndef ZB_PRO_STACK
#error This test is not applicable for 2007 stack. ZB_PRO_STACK should be defined to enable PRO.
#endif


/* Size of test buffer */
#define TEST_BUFFER_LEN     	       0x0010

#define USE_INVISIBLE_MODE

#ifdef ZB_EMBER_TESTS
#define ZC_EMBER
#endif

static zb_ieee_addr_t g_ieee_addr = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
static zb_ieee_addr_t g_ieee_up = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
static zb_ieee_addr_t g_ieee_down_1 = {0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00};
static zb_ieee_addr_t g_ieee_down_2 = {0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00};

static zb_uint16_t g_nwk_down_1 = 0x0002;
static zb_uint16_t g_nwk_down_2 = 0x0003;

static void add_addr_ent(zb_uint16_t nwk_addr, zb_ieee_addr_t ieee_addr);

//#define TEST_CHANNEL (1l << 24)

MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("zdo_2_zr1");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	
  /* set ieee addr */
  zb_set_long_address(g_ieee_addr);

  /* join as a router */
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zr_role();
  zb_aib_tcpol_set_update_trust_center_link_keys_required(ZB_FALSE);

  /* turn off security */
  /* zb_cert_test_set_security_level(0); */

  zb_set_max_children(0);

  MAC_ADD_VISIBLE_LONG(g_ieee_up);
  MAC_ADD_VISIBLE_LONG(g_ieee_down_1);
  MAC_ADD_VISIBLE_LONG(g_ieee_down_2);

  ZB_CERT_HACKS().disable_in_out_cost_updating = 1;

  zb_set_nvram_erase_at_start(ZB_TRUE);

  add_addr_ent(g_nwk_down_1, g_ieee_down_1);
  add_addr_ent(g_nwk_down_2, g_ieee_down_2);

  if (zboss_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "#####zboss_start failed", (FMT__0));
  }
  else
  {
    zboss_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}

/*
 *  Set outgoing cost for zc larger than outgoing cost for zc into zr1
 *  to make zr3 send packets trough zr1
 */
static void change_link_status(zb_uint8_t param)
{
  zb_neighbor_tbl_ent_t *nbt;
  ZVUNUSED(param);

  if (RET_OK == zb_nwk_neighbor_get_by_ieee((zb_uint8_t*)g_ieee_up, &nbt))
  {
    nbt->u.base.age = 0;
    nbt->u.base.outgoing_cost = 1;
    nbt->lqi = NWK_COST_TO_LQI(1);
    TRACE_MSG(TRACE_ZDO1, "ZC lqi was %hd. Modify outgoing_cost to %hd, lqi %hd", (FMT__H_H_H, nbt->lqi, nbt->u.base.outgoing_cost, NWK_COST_TO_LQI(3)));
  }

  if (RET_OK == zb_nwk_neighbor_get_by_ieee((zb_uint8_t*)g_ieee_down_1, &nbt))
  {
    nbt->u.base.age = 0;
    nbt->u.base.outgoing_cost = 1;
    nbt->lqi = NWK_COST_TO_LQI(3);
    TRACE_MSG(TRACE_ZDO1, "ZC lqi was %hd. Modify outgoing_cost to %hd, lqi %hd", (FMT__H_H_H, nbt->lqi, nbt->u.base.outgoing_cost, NWK_COST_TO_LQI(1)));
  }

  if (RET_OK == zb_nwk_neighbor_get_by_ieee((zb_uint8_t*)g_ieee_down_2, &nbt))
  {
    nbt->u.base.age = 0;
    nbt->u.base.outgoing_cost = 1;
    nbt->lqi = NWK_COST_TO_LQI(1);
    TRACE_MSG(TRACE_ZDO1, "ZC lqi was %hd. Modify outgoing_cost to %hd, lqi %hd", (FMT__H_H_H, nbt->lqi, nbt->u.base.outgoing_cost, NWK_COST_TO_LQI(1)));
  }

  ZB_SCHEDULE_ALARM(change_link_status, 0, 1*ZB_TIME_ONE_SECOND);
}

static void add_addr_ent(zb_uint16_t nwk_addr, zb_ieee_addr_t ieee_addr)
{
  zb_address_ieee_ref_t addr_ref;
  zb_address_update(ieee_addr, nwk_addr, ZB_FALSE, &addr_ref);
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

  TRACE_MSG(TRACE_APP1, "zboss_signal_handler: status %hd signal %hd",
            (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
        TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
        ZB_SCHEDULE_CALLBACK(change_link_status, 0);
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

#if 0
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

  ++tr_count;

  if (tr_count != 2)
  {
    zb_bufid_t buf = zb_buf_get_out();
    ZB_SCHEDULE_CALLBACK(send_data, buf);
  }
}

static void send_data(zb_uint8_t param)
{
  zb_buffer_test_req_param_t *req_param;
  TRACE_MSG(TRACE_APS1, "send_data %hd", (FMT__H, param));
  req_param = ZB_BUF_GET_PARAM(param, zb_buffer_test_req_param_t);
  BUFFER_TEST_REQ_SET_DEFAULT(req_param);
  req_param->len = TEST_BUFFER_LEN;
  req_param->dst_addr = 0x0000; /* coordinator short address */
  req_param->src_ep = 0x01;
  zb_tp_buffer_test_request(param, buffer_test_cb);
}
#endif
