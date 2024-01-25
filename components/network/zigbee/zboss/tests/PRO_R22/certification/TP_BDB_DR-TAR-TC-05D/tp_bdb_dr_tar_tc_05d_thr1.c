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
/* PURPOSE: TH ZR1
*/

#define ZB_TEST_NAME TP_BDB_DR_TAR_TC_05D_THR1
#define ZB_TRACE_FILE_ID 40817

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

#include "tp_bdb_dr_tar_tc_05d_common.h"
#include "../common/zb_cert_test_globals.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_CERTIFICATION_HACKS
#error ZB_CERTIFICATION_HACKS is not compiled!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_thr1 = IEEE_ADDR_THR1;
static zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;

static void send_leave_delayed(zb_uint8_t unused);
static void construct_and_send_leave(zb_uint8_t param);
static void test_start_get_peer_addr(zb_uint8_t unused);
static void test_get_peer_addr_req(zb_uint8_t param);
static void test_get_peer_addr_resp(zb_uint8_t param);

static zb_uint16_t s_dut_short_addr;

MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_thr1");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif


  zb_set_long_address(g_ieee_addr_thr1);

  zb_set_network_router_role((1l << TEST_CHANNEL));
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
      TRACE_MSG(TRACE_APS1, "Device started, status %d", (FMT__D, status));
      if (status == 0)
      {
        ZB_SCHEDULE_ALARM(test_start_get_peer_addr, 0, THR1_GET_PEER_ADDR_REQ_DELAY);
        ZB_SCHEDULE_ALARM(send_leave_delayed, 0, THR1_SEND_LEAVE_DELAY);
      }
      break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}

static void send_leave_delayed(zb_uint8_t unused)
{
  ZVUNUSED(unused);
  zb_buf_get_out_delayed(construct_and_send_leave);
}

static void construct_and_send_leave(zb_uint8_t param)
{
  zb_uint8_t *byte_p;
  zb_uint8_t aps_fc = 0;
  zb_uint8_t mac_tx_opt;

  TRACE_MSG(TRACE_APS1, ">>test_build_and_send_dev_annce: param = %d", (FMT__D, param));

  byte_p = zb_buf_initial_alloc(param, sizeof(zb_uint8_t) + sizeof(zb_zdo_mgmt_leave_req_t));

  /* ZDO Device Annce: tsn | device nwk_addr | device ieee_addr| device capabilities */
  zb_cert_test_inc_zdo_tsn();
  *byte_p++ = zb_cert_test_get_zdo_tsn();
  ZB_IEEE_ADDR_COPY(byte_p, g_ieee_addr_dut);
  byte_p += sizeof(zb_ieee_addr_t);
  *byte_p = 0;

  /* creating APS header */
  ZB_APS_FC_SET_FRAME_TYPE(aps_fc, ZB_APS_FRAME_DATA);
  ZB_APS_FC_SET_DELIVERY_MODE(aps_fc, ZB_APS_DELIVERY_UNICAST);
  ZB_APS_FC_SET_ACK_REQUEST(aps_fc, ZB_TRUE);

  /* APS Header: fc | dest_ep | cluster_id | profile_id | src_ep | aps_counter - 8 bytes */

  byte_p = zb_buf_alloc_left(param, sizeof(zb_uint8_t) * 8);
  *byte_p++ = aps_fc;
  *byte_p++ = 0; /* dest endpoint */
  byte_p = zb_put_next_htole16(byte_p, ZDO_MGMT_LEAVE_REQ_CLID);
  byte_p = zb_put_next_htole16(byte_p, ZB_AF_ZDO_PROFILE_ID);
  *byte_p++ = 0; /* src endpoint */
  *byte_p++ = zb_cert_test_get_aps_counter();
  zb_cert_test_inc_aps_counter();

  /* NWK Header: fc | dest_addr | src_addr | radius | seq_number | ext_dst | ext_src :
     24 bytes + security header */
  byte_p = zb_buf_alloc_left(param, sizeof(zb_uint8_t) * (24 + sizeof(zb_nwk_aux_frame_hdr_t)));
  byte_p[ZB_PKT_16B_ZERO_BYTE] = 0;
  byte_p[ZB_PKT_16B_FIRST_BYTE] = 0;
  ZB_NWK_FRAMECTL_SET_DISCOVER_ROUTE(byte_p, 0);
  ZB_NWK_FRAMECTL_SET_SECURITY(byte_p, ZB_TRUE);
  ZB_NWK_FRAMECTL_SET_SRC_DEST_IEEE(byte_p, ZB_TRUE, ZB_TRUE);
  ZB_NWK_FRAMECTL_SET_PROTOCOL_VERSION(byte_p, ZB_PROTOCOL_VERSION);
  byte_p += 2;
  byte_p = zb_put_next_htole16(byte_p, s_dut_short_addr);
  byte_p = zb_put_next_htole16(byte_p, ZB_PIBCACHE_NETWORK_ADDRESS());
  *byte_p++ = 30;
  *byte_p++ = zb_cert_test_get_nib_seq_number();
  zb_cert_test_inc_nib_seq_number();
  ZB_IEEE_ADDR_COPY(byte_p, g_ieee_addr_dut);
  byte_p += sizeof(zb_ieee_addr_t);

  zb_get_long_address(byte_p);

  /* building MCPS data request */
  zb_buf_flags_or(param, ZB_BUF_SECUR_NWK_ENCR);
  zb_buf_set_handle(param, param);
  mac_tx_opt = MAC_TX_OPTION_ACKNOWLEDGED_BIT;
#if 1
  zb_mcps_build_data_request(param, ZB_PIBCACHE_NETWORK_ADDRESS(),
                             s_dut_short_addr, mac_tx_opt, param);
#endif

  zb_nwk_unlock_in(param);
  ZB_SCHEDULE_CALLBACK(zb_mcps_data_request, param);
}

static void test_start_get_peer_addr(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  zb_buf_get_out_delayed(test_get_peer_addr_req);
}

static void test_get_peer_addr_req(zb_uint8_t param)
{
  zb_zdo_nwk_addr_req_param_t *req_param = ZB_BUF_GET_PARAM(param, zb_zdo_nwk_addr_req_param_t);

  TRACE_MSG(TRACE_ZDO1, ">>test_get_peer_addr_req", (FMT__0));

  req_param->dst_addr = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
  req_param->start_index = 0;
  req_param->request_type = ZB_ZDO_SINGLE_DEVICE_RESP;
  ZB_IEEE_ADDR_COPY(req_param->ieee_addr, g_ieee_addr_dut);
  zb_zdo_nwk_addr_req(param, test_get_peer_addr_resp);

  TRACE_MSG(TRACE_ZDO1, "<<test_get_peer_addr_req", (FMT__0));
}

static void test_get_peer_addr_resp(zb_uint8_t param)
{
  zb_zdo_nwk_addr_resp_head_t *resp = (zb_zdo_nwk_addr_resp_head_t*) zb_buf_begin(param);

  TRACE_MSG(TRACE_ZDO1, ">>test_get_peer_addr_resp: status = %d",
            (FMT__D_D, resp->status));

  if (resp->status == ZB_ZDP_STATUS_SUCCESS)
  {
    ZB_LETOH16(&s_dut_short_addr, &resp->nwk_addr);
  }

  zb_buf_free(param);

  TRACE_MSG(TRACE_ZDO1, "<<test_get_peer_addr_resp", (FMT__0));
}


/*! @} */
