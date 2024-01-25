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
/* PURPOSE: TP_PED_1 ZigBee Router (TH): joining to Centralized Network
*/

#define ZB_TEST_NAME TP_PED_1_GZR
#define ZB_TRACE_FILE_ID 40905

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"


/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

static const zb_ieee_addr_t g_ieee_addr_gzr = IEEE_ADDR_gZR;
static const zb_ieee_addr_t g_ieee_addr_gzed = IEEE_ADDR_gZED;
static const zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;

static void test_spoof_dev_annce(zb_uint8_t param);
static void test_retrieve_nbt_table(zb_uint8_t param);
static void dev_annce_cb(zb_zdo_device_annce_t *da);
static void send_dev_annce(zb_uint8_t param);
static void send_mgmt_lqi_req(zb_uint8_t param);
static void mgmt_lqi_resp(zb_uint8_t param);


static zb_uint8_t s_mgmt_lqi_start_index = 0;
static zb_uint16_t s_dut_short_addr = 0x0000;
static zb_zdo_device_annce_t s_gzed_info;


MAIN()
{
  ARGV_UNUSED;

  ZB_INIT("zdo_3_gzr");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

  zb_set_long_address(g_ieee_addr_gzr);

  zb_set_max_children(0);
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zr_role();

#ifdef SECURITY_LEVEL
  zb_cert_test_set_security_level(SECURITY_LEVEL);
#endif

  zb_zdo_set_aps_unsecure_join(ZB_TRUE);

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


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  switch (sig)
  {
    case ZB_ZDO_SIGNAL_DEFAULT_START:
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ZB_BDB_SIGNAL_DEVICE_REBOOT:
      if (status == 0)
      {
	TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));

	ZB_SCHEDULE_ALARM(test_spoof_dev_annce, 0, TEST_GZR_SPOOF_DEV_ANNCE_DELAY);
        ZB_SCHEDULE_ALARM(test_retrieve_nbt_table, 0, TEST_GZR_RETRIEVE_NBT_TABLE_DELAY);
        zb_zdo_register_device_annce_cb(dev_annce_cb);
        s_dut_short_addr = zb_address_short_by_ieee((zb_uint8_t*) g_ieee_addr_dut);
      }
      else
      {
	TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, status));
      }
      break; /* ZB_ZDO_SIGNAL_DEFAULT_START */

    default:
      if (status == 0)
      {
	TRACE_MSG(TRACE_APS1, "Unknown signal, status OK", (FMT__0));
      }
      else
      {
	TRACE_MSG(TRACE_ERROR, "Unknown signal, status FAILED, status %d", (FMT__D, status));
      }
      break;
  }

  zb_buf_free(param);
}


static void test_spoof_dev_annce(zb_uint8_t param)
{
  ZVUNUSED(param);
  zb_buf_get_out_delayed(send_dev_annce);
}


static void test_retrieve_nbt_table(zb_uint8_t param)
{
  ZVUNUSED(param);
  zb_buf_get_out_delayed(send_mgmt_lqi_req);
}


static void dev_annce_cb(zb_zdo_device_annce_t *da)
{
  TRACE_MSG(TRACE_ZDO1, ">>dev_annce_cb, ieee = " TRACE_FORMAT_64 "addr = %h",
            (FMT__A_H, TRACE_ARG_64(da->ieee_addr), da->nwk_addr));
  if (ZB_IEEE_ADDR_CMP(g_ieee_addr_gzed, da->ieee_addr) == ZB_TRUE)
  {
    ZB_IEEE_ADDR_COPY(&s_gzed_info.ieee_addr, da->ieee_addr);
    s_gzed_info.nwk_addr = da->nwk_addr;
    s_gzed_info.capability = da->capability;
    TRACE_MSG(TRACE_ZDO2, "dev_annce_cb: gZED is found!", (FMT__0));
  }
  TRACE_MSG(TRACE_ZDO1, "<<dev_annce_cb", (FMT__0));
}


static void send_dev_annce(zb_uint8_t param)
{
  zb_uint8_t *byte_p;
  zb_uint8_t aps_fc = 0;
  zb_uint8_t mac_tx_opt;

  TRACE_MSG(TRACE_APS1, ">>send_dev_annce: param = %d", (FMT__D, param));

  byte_p = zb_buf_initial_alloc(param, sizeof(zb_zdo_device_annce_t));

  /* ZDO Device Annce: tsn | device nwk_addr | device ieee_addr| device capabilities */
  zb_cert_test_inc_zdo_tsn();
  *byte_p++ = zb_cert_test_get_zdo_tsn();;
  byte_p = zb_put_next_htole16(byte_p, s_gzed_info.nwk_addr);
  ZB_IEEE_ADDR_COPY(byte_p, s_gzed_info.ieee_addr);
  byte_p += sizeof(zb_ieee_addr_t);
  *byte_p = s_gzed_info.capability;

  /* creating APS header */
  ZB_APS_FC_SET_FRAME_TYPE(aps_fc, ZB_APS_FRAME_DATA);
  ZB_APS_FC_SET_DELIVERY_MODE(aps_fc, ZB_APS_DELIVERY_BROADCAST);

  /* APS Header: fc | dest_ep | cluster_id | profile_id | src_ep | aps_counter - 8 bytes */
  byte_p = zb_buf_alloc_left(param, sizeof(zb_uint8_t) * 8);
  *byte_p++ = aps_fc;
  *byte_p++ = 0; /* dest endpoint */
  byte_p = zb_put_next_htole16(byte_p, ZDO_DEVICE_ANNCE_CLID);
  byte_p = zb_put_next_htole16(byte_p, ZB_AF_ZDO_PROFILE_ID);
  *byte_p++ = 0; /* src endpoint */
  *byte_p++ = zb_cert_test_get_aps_counter();
  zb_cert_test_inc_aps_counter();

  /* NWK Header: fc | dest_addr | src_addr | radius | seq_number | ext_src - 16 bytes + security header */
  byte_p = zb_buf_alloc_left(param, sizeof(zb_uint8_t) * (16 + sizeof(zb_nwk_aux_frame_hdr_t)));

  ZB_BZERO(byte_p, 2);
  ZB_NWK_FRAMECTL_SET_DISCOVER_ROUTE(byte_p, ZB_FALSE);
  ZB_NWK_FRAMECTL_SET_SECURITY(byte_p, ZB_TRUE);
  ZB_NWK_FRAMECTL_SET_SOURCE_IEEE(byte_p, ZB_TRUE);
  ZB_NWK_FRAMECTL_SET_PROTOCOL_VERSION(byte_p, 2);
  byte_p += 2;
  byte_p = zb_put_next_htole16(byte_p, 0xfffd);
  byte_p = zb_put_next_htole16(byte_p, s_gzed_info.nwk_addr);
  *byte_p++ = zb_cert_test_nib_get_max_depth() * 2;

  *byte_p++ = zb_cert_test_get_nib_seq_number();
  zb_cert_test_inc_nib_seq_number();
  ZB_IEEE_ADDR_COPY(byte_p, s_gzed_info.ieee_addr);

  /* building MCPS data request */
  zb_buf_flags_or(param, ZB_BUF_SECUR_NWK_ENCR);
  zb_buf_set_handle(param, param);
  mac_tx_opt = 0; /* no ack needed, no gts/cap,  direct transmission */

#if 1
  zb_mcps_build_data_request(param, ZB_PIBCACHE_NETWORK_ADDRESS(),
                             0Xffff, mac_tx_opt, param);
#endif

  zb_nwk_unlock_in(param);
  ZB_SCHEDULE_CALLBACK(zb_mcps_data_request, param);

  TRACE_MSG(TRACE_APS1, ">>send_dev_annce", (FMT__0));
}


static void send_mgmt_lqi_req(zb_uint8_t param)
{
  zb_zdo_mgmt_lqi_param_t *req_param = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_lqi_param_t);

  TRACE_MSG(TRACE_ZDO1, ">>mgmt_lqi_resp buf = %d", (FMT__D, param));

  req_param->dst_addr = s_dut_short_addr;
  req_param->start_index = s_mgmt_lqi_start_index;
  zb_zdo_mgmt_lqi_req(param, mgmt_lqi_resp);

  TRACE_MSG(TRACE_ZDO1, "<<mgmt_lqi_resp buf", (FMT__0));
}


static void mgmt_lqi_resp(zb_uint8_t param)
{
  zb_zdo_mgmt_lqi_resp_t *resp = (zb_zdo_mgmt_lqi_resp_t*) zb_buf_begin(param);

  TRACE_MSG(TRACE_ZDO1, ">>mgmt_lqi_resp buf = %d", (FMT__D, param));

  if (resp->status == ZB_ZDP_STATUS_SUCCESS)
  {
    zb_uint8_t nbrs = resp->neighbor_table_list_count;

    s_mgmt_lqi_start_index += nbrs;
    if (s_mgmt_lqi_start_index < resp->neighbor_table_entries)
    {
      TRACE_MSG(TRACE_ZDO2, "mgmt_lqi_resp: retrieved = %d, total = %d",
                (FMT__D_D, nbrs, resp->neighbor_table_entries));
      zb_buf_get_out_delayed(send_mgmt_lqi_req);
    }
    else
    {
      TRACE_MSG(TRACE_ZDO2, "mgmt_lqi_resp: retrieved all entries - %d",
                (FMT__D, resp->neighbor_table_entries));
    }
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "mgmt_lqi_resp: request failure with status = %d", (FMT__D, resp->status));
  }
  zb_buf_free(param);

  TRACE_MSG(TRACE_ZDO1, "<<mgmt_lqi_resp", (FMT__0));
}


/*! @} */

