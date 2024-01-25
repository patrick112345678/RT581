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
/* PURPOSE: TP/R21/BV-11 - Processing of unencrypted NWK frames, gZR1 (joining to DUT ZC)
*/


#define ZB_TEST_NAME TP_R21_BV_11_GZR1_C
#define ZB_TRACE_FILE_ID 40874

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "../../../nwk/nwk_internal.h"
#include "test_common.h"
#include "../common/zb_cert_test_globals.h"


#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif


/*! \addtogroup ZB_TESTS */
/*! @{ */

/**
  Copied from nwk_internal.h
  */
#define NWK_ARRAY_GET_ENT(array, ent, cnt) do                            \
{                                                                        \
  zb_ushort_t ii;                                                        \
  for (ii = 0, ent = NULL; ii < sizeof(array) / sizeof(array[0]); ii++)  \
  {                                                                      \
    if ( !ZB_U2B(array[ii].used) )                              \
    {                                                                    \
      ent = &array[ii];                                                  \
      cnt++;                                                             \
      array[ii].used = ZB_B2U(ZB_TRUE);                         \
      break;                                                             \
    }                                                                    \
  }                                                                      \
} while (0)

static void test_send_nwk_addr_req_delayed(zb_uint8_t param);
static void test_send_secur_nwk_addr_req(zb_uint8_t param);
static void test_send_unsecur_nwk_addr_req(zb_uint8_t param);
static void test_send_brcast_cmd_delayed(zb_uint8_t param);
static void device_annce_cb(zb_zdo_device_annce_t *da);
static void test_node_desc_resp_handler(zb_uint8_t param);

static void test_send_secur_node_desc_req(zb_uint8_t param);
static void test_send_unsecur_node_desc_req(zb_uint8_t param);
static void test_send_unsecur_frame(zb_uint8_t param, zb_uint16_t command_id, zb_uint16_t addr);

static const zb_ieee_addr_t g_ieee_addr_gzr1 = IEEE_ADDR_gZR1;
static const zb_ieee_addr_t g_ieee_addr_gzr2 = IEEE_ADDR_gZR2;
static const zb_ieee_addr_t g_ieee_addr_unknown_dev = IEEE_ADDR_UNKNOWN_DEV;

static zb_uint16_t g_dest_short_addr = 0x0000;
static zb_uint16_t g_gzr2_short_addr = 0x0000;
static zb_ieee_addr_t g_dest_ieee_addr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static int iter = 0;


MAIN()
{
  ARGV_UNUSED;

  ZB_SET_TRAF_DUMP_ON();
  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("zdo_2_gzr1_c");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	
  zb_set_default_ffd_descriptor_values(ZB_ROUTER);

  zb_set_long_address(g_ieee_addr_gzr1);
  zb_set_pan_id(TEST_PAN_ID);

  zb_set_use_extended_pan_id(g_ext_pan_id);
  zb_aib_set_trust_center_address(g_addr_tc);

  zb_zdo_set_aps_unsecure_join(ZB_TRUE);
  zb_set_max_children(0);

  ZB_CERT_HACKS().use_route_for_neighbor = 1;

  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zr_role();

#ifdef SECURITY_LEVEL
  zb_cert_test_set_security_level(SECURITY_LEVEL);
#endif

  zb_zdo_register_device_annce_cb(device_annce_cb);

  zb_set_nvram_erase_at_start(ZB_TRUE);

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


static void device_annce_cb(zb_zdo_device_annce_t *da)
{
  TRACE_MSG(TRACE_ERROR, ">> device_annce_cb, da %p", (FMT__P, da));

  if (ZB_IEEE_ADDR_CMP(da->ieee_addr, g_ieee_addr_gzr2))
  {
    g_gzr2_short_addr = da->nwk_addr;
    TRACE_MSG(TRACE_ERROR, "g_gzr2_short_addr %d", (FMT__D, g_gzr2_short_addr));
  }

  TRACE_MSG(TRACE_ERROR, "<< device_annce_cb", (FMT__0));
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  TRACE_MSG(TRACE_ERROR, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  if (0 == status)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));
        ZB_SCHEDULE_ALARM(test_send_brcast_cmd_delayed, 0, ZB_TIME_ONE_SECOND * 45);
        break;

      default:
        TRACE_MSG(TRACE_ERROR, "Unknown signal %hd", (FMT__H, sig));
    }
  }
  else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
  {
    TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, status));
  }

  if (param)
  {
    zb_buf_free(param);
  }
}


static void test_send_nwk_addr_req_delayed(zb_uint8_t param)
{
  ZVUNUSED(param);

  TRACE_MSG(TRACE_ZDO1, ">>test_send_nwk_addr_req_delayed", (FMT__0));

  ZB_IEEE_ADDR_COPY(g_dest_ieee_addr, g_ieee_addr_unknown_dev);
  zb_buf_get_out_delayed(test_send_unsecur_nwk_addr_req);
  zb_buf_get_out_delayed(test_send_secur_nwk_addr_req);

  TRACE_MSG(TRACE_ZDO1, "<<test_send_nwk_addr_req_delayed", (FMT__0));
}


static void test_send_secur_nwk_addr_req(zb_uint8_t param)
{
  zb_zdo_nwk_addr_req_param_t *req_param = ZB_BUF_GET_PARAM(param, zb_zdo_nwk_addr_req_param_t);

  TRACE_MSG(TRACE_ZDO1, ">>test_send_secur_nwk_addr_req, param = %h, dest short addr = %h",
            (FMT__H_H, param, g_dest_short_addr));
  TRACE_MSG(TRACE_ZDO1, "dest ieee addr = " TRACE_FORMAT_64,
            (FMT__A, TRACE_ARG_64(g_dest_ieee_addr)));

  req_param->dst_addr = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
  req_param->start_index = 0;
  req_param->request_type = ZB_ZDO_SINGLE_DEVICE_RESP;
  ZB_IEEE_ADDR_COPY(req_param->ieee_addr, g_dest_ieee_addr);
  zb_zdo_nwk_addr_req(param, NULL);

  TRACE_MSG(TRACE_ZDO1, "<<test_send_secur_nwk_addr_req", (FMT__0));
}


static void test_send_unsecur_nwk_addr_req(zb_uint8_t param)
{
  zb_zdo_nwk_addr_req_t *req;

  TRACE_MSG(TRACE_ZDO1, ">>test_send_unsecur_nwk_addr_req, param = %d", (FMT__D, param));

  req = zb_buf_initial_alloc(param, sizeof(zb_zdo_nwk_addr_req_t));
  req->start_index = 0;
  req->request_type = ZB_ZDO_SINGLE_DEVICE_RESP;
  ZB_IEEE_ADDR_COPY(req->ieee_addr, g_dest_ieee_addr);
  test_send_unsecur_frame(param, ZDO_NWK_ADDR_REQ_CLID, ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE);

  TRACE_MSG(TRACE_ZDO1, "<<test_send_unsecur_nwk_addr_req", (FMT__0));
}


static void test_send_brcast_cmd_delayed(zb_uint8_t param)
{
  ZVUNUSED(param);

  TRACE_MSG(TRACE_ERROR, ">>test_send_brcast_cmd_delayed", (FMT__0));

  g_dest_short_addr = 0;        /* Centralized ZR - DUT is ZC. */
  zb_buf_get_out_delayed(test_send_unsecur_node_desc_req);
  zb_buf_get_out_delayed(test_send_secur_node_desc_req);

  TRACE_MSG(TRACE_ERROR, "<<test_send_brcast_cmd_delayed", (FMT__0));
}


static void test_node_desc_resp_handler(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ERROR, ">>test_node_desc_resp_handler, param = %d", (FMT__D, param));

  if (iter == 0)
  {
    zb_nwk_routing_t *routing_ent;
    /* add routing table entry for gZR2 */

    ++iter;
    g_dest_short_addr = g_gzr2_short_addr;
    routing_ent = new_routing_table_ent();

    if ( routing_ent )
    {
      zb_address_ieee_ref_t dut_addr_ref;

      TRACE_MSG(TRACE_ERROR, "test_node_desc_resp_handler: ent = %p", (FMT__P, routing_ent));
      routing_ent->dest_addr = g_dest_short_addr;
      routing_ent->status = ZB_NWK_ROUTE_STATE_ACTIVE;
#if defined ZB_PRO_STACK && !defined ZB_NO_NWK_MULTICAST
      routing_ent->group_id_flag = 0;
#endif
      /* add next hop addresss */
      zb_address_by_short(0x0000, ZB_FALSE, ZB_FALSE, &dut_addr_ref);
      routing_ent->next_hop_addr_ref = dut_addr_ref;
      routing_ent->expiry = ZB_NWK_ROUTING_TABLE_EXPIRY;
      TRACE_MSG(TRACE_ERROR, "new routing ent : dest_addr 0x%x status %hd next_hop_ref 0x%x",
                (FMT__D_H_D,
                 routing_ent->dest_addr, routing_ent->status, dut_addr_ref));
    }

    zb_buf_get_out_delayed(test_send_unsecur_node_desc_req);
    zb_buf_get_out_delayed(test_send_secur_node_desc_req);
  }
  else
  {
    ZB_SCHEDULE_ALARM(test_send_nwk_addr_req_delayed, 0, ZB_TIME_ONE_SECOND * 15);
  }

  TRACE_MSG(TRACE_ERROR, "<<test_node_desc_resp_handler", (FMT__0));

  zb_buf_free(param);
}

/*=====================================================================*/

static void test_send_secur_node_desc_req(zb_uint8_t param)
{
  zb_zdo_node_desc_req_t *req;

  TRACE_MSG(TRACE_APS1, ">>test_send_secur_node_desc_req, param = %d", (FMT__D, (int)param));

  req = zb_buf_initial_alloc(param, sizeof(zb_zdo_node_desc_req_t));
  req->nwk_addr = g_dest_short_addr;
  TRACE_MSG(TRACE_APS1, "send node_desc_req: nwk_addr = %h.", (FMT__H, req->nwk_addr));
  zb_zdo_node_desc_req(param, test_node_desc_resp_handler);

  TRACE_MSG(TRACE_APS1, "<<test_send_secur_node_desc_req", (FMT__0));
}


static void test_send_unsecur_node_desc_req(zb_uint8_t param)
{
  zb_zdo_node_desc_req_t *req;

  TRACE_MSG(TRACE_ZDO1, ">>test_send_unsecur_node_desc_req, param = %d", (FMT__D, param));

  req = zb_buf_initial_alloc(param, sizeof(zb_zdo_node_desc_req_t));
  req->nwk_addr = g_dest_short_addr;
  ZB_HTOLE16_ONPLACE(req->nwk_addr);
  test_send_unsecur_frame(param, ZDO_NODE_DESC_REQ_CLID, g_dest_short_addr);

  TRACE_MSG(TRACE_ZDO1, "<<test_send_unsecur_node_desc_req", (FMT__0));
}


static void test_send_unsecur_frame(zb_uint8_t param, zb_uint16_t command_id, zb_uint16_t addr)
{
  zb_uint8_t *byte_p;
  zb_uint16_t *word_p;
  zb_nlde_data_req_t *nldereq;
  zb_uint8_t fc = 0;

  TRACE_MSG(TRACE_APS1, ">>test_send_unsecur_frame, param = %hd, command_id = %hd, addr = %hd",
            (FMT__H_H_H, param, command_id, addr));

  byte_p = zb_buf_alloc_left(param, 1);
  zb_cert_test_inc_zdo_tsn();
  *byte_p = zb_cert_test_get_zdo_tsn();;

  /* APS */
  byte_p = zb_buf_alloc_left(param, 1);
  *byte_p = zb_cert_test_get_aps_counter();
  zb_cert_test_inc_aps_counter();

  byte_p = zb_buf_alloc_left(param, 1);
  *byte_p = 0; /* source endpoint */

  word_p = zb_buf_alloc_left(param, 2);
  *word_p = 0x0000; /* profile id */

  word_p = zb_buf_alloc_left(param, 2);
  *word_p = command_id; /* cluster id */
  ZB_HTOLE16_ONPLACE(*word_p);

  byte_p = zb_buf_alloc_left(param, 1);
  *byte_p = 0; /* destination endpoint */

  byte_p = zb_buf_alloc_left(param, 1);
  if (!ZB_NWK_IS_ADDRESS_BROADCAST(addr))
  {
    ZB_APS_FC_SET_DELIVERY_MODE(fc, ZB_APS_DELIVERY_UNICAST);
    ZB_APS_FC_SET_ACK_REQUEST(fc, 1);
  }
  else
  {
    ZB_APS_FC_SET_DELIVERY_MODE(fc, ZB_APS_DELIVERY_BROADCAST);
  }
  *byte_p = fc;

  /* NLDE request */
  nldereq = zb_buf_get_tail(param, sizeof(zb_nlde_data_req_t));
  nldereq->radius = 5;
  nldereq->addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
  nldereq->nonmember_radius = 0; /* if multicast, get it from APS IB */
  nldereq->discovery_route = 1;  /* always! see 2.2.4.1.1.3 */
  nldereq->dst_addr = addr;
  nldereq->ndsu_handle = 0;
  nldereq->security_enable = ZB_FALSE;
  nldereq->use_alias = ZB_FALSE;

  ZB_SCHEDULE_CALLBACK(zb_nlde_data_request, param);

  TRACE_MSG(TRACE_APS1, "<<test_send_unsecur_frame", (FMT__0));
}

/*! @} */
