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
/* PURPOSE: TP/ZDO/BV-11: ZED-ZDO-Transmit Service Discovery
The DUT as ZigBee end device shall request service discovery to a
ZigBee coordinator. End device side.

NOTE: Complex_Desc_req() User_Desc_req() Discovery_Register_req()
User_Desc_Set_req() are not supported.
*/

#define ZB_TEST_NAME TP_ZDO_BV_11_ZED1
#define ZB_TRACE_FILE_ID 40826

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"


/* For NS build first ieee addr byte should be unique */
static zb_ieee_addr_t g_ieee_addr = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static zb_uint_t g_error = 0;

enum test_step_e
{
  STEP_ZED1_NODE_DESCR_REQ,
  STEP_ZED1_POWER_DESCR_REQ,
  STEP_ZED1_SIMPLE_DESCR_REQ,
  STEP_ZED1_ACTIVE_EP_REQ,
  STEP_ZED1_MATCH_DESCR_REQ,
  STEP_ZED1_COMPLEX_DESCR_REQ,
  STEP_ZED1_USER_DESCR_REQ1,
  STEP_ZED1_DISCOVERY_REG_REQ,
  STEP_ZED1_USER_DESCR_SET,
  STEP_ZED1_USER_DESCR_REQ2,
  STEP_ZED1_END_DEVICE_ANNCE
};

enum test_zdo_clid_e
{
  TEST_ZDO_COMPLEX_DESCR_REQ_CLID = 0x0010,
  TEST_ZDO_USER_DESCR_REQ_CLID = 0x0011,
  TEST_ZDO_USER_DESCR_SET_CLID = 0x0014
};

static void buffer_manager(zb_uint8_t param);
static void send_aps_packet(zb_uint8_t param);
static void complex_desc_req(zb_uint8_t param);
static void user_desc_req(zb_uint8_t param);
static void user_desc_set(zb_uint8_t param);
static void end_dev_annce(zb_uint8_t param);
static void match_desc_callback(zb_uint8_t param);
static void get_match_desc(zb_uint8_t param);
static void active_ep_callback(zb_uint8_t param);
static void get_active_ep(zb_uint8_t param);
static void simple_desc_callback(zb_uint8_t param);
static void get_simple_desc(zb_uint8_t param);
static void node_power_desc_callback(zb_uint8_t param);
static void get_power_desc(zb_uint8_t param);
static void node_desc_callback(zb_uint8_t param);
static void get_node_desc(zb_uint8_t param);

MAIN()
{
ARGV_UNUSED;


  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_zed1");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif


  /* set ieee addr */
  zb_set_long_address(g_ieee_addr);


  /* turn off security */
  /* zb_cert_test_set_security_level(0); */

  /* become an ED */
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zed_role();
  zb_bdb_set_legacy_device_support(ZB_TRUE);

  zb_set_rx_on_when_idle(ZB_FALSE);

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


static void buffer_manager(zb_uint8_t param)
{
  zb_callback_t call_func = 0;

  switch (param)
  {
    case STEP_ZED1_NODE_DESCR_REQ:
      call_func = get_node_desc;
      break;
    case STEP_ZED1_POWER_DESCR_REQ:
      call_func = get_power_desc;
      break;
    case STEP_ZED1_SIMPLE_DESCR_REQ:
      call_func = get_simple_desc;
      break;
    case STEP_ZED1_ACTIVE_EP_REQ:
      call_func = get_active_ep;
      break;
    case STEP_ZED1_MATCH_DESCR_REQ:
      call_func = get_match_desc;
      break;
    case STEP_ZED1_COMPLEX_DESCR_REQ:
      call_func = complex_desc_req;
      break;
    case STEP_ZED1_USER_DESCR_REQ1:
      call_func = user_desc_req;
      break;
    case STEP_ZED1_DISCOVERY_REG_REQ:
      /* This step is omitted: Discovery Register Request is undefined */
      break;
    case STEP_ZED1_USER_DESCR_SET:
      call_func = user_desc_set;
      break;
    case STEP_ZED1_USER_DESCR_REQ2:
      call_func = user_desc_req;
      break;
    case STEP_ZED1_END_DEVICE_ANNCE:
      call_func = end_dev_annce;
      break;
  }

  if (call_func)
  {
    zb_buf_get_out_delayed(call_func);
  }
}


static void send_aps_packet(zb_bufid_t buf)
{
  zb_uint16_t req_clid = *ZB_BUF_GET_PARAM(buf, zb_uint16_t);
  zb_apsde_data_req_t *req = ZB_BUF_GET_PARAM(buf, zb_apsde_data_req_t);

  if (req_clid == ZDO_DEVICE_ANNCE_CLID)
  {
    req->dst_addr.addr_short = 0xFFFF;
  }
  else
  {
    req->dst_addr.addr_short = 0x0000;
  }
  req->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
  if (req->dst_addr.addr_short != 0xffff)
  {
    req->tx_options = ZB_APSDE_TX_OPT_ACK_TX;
  }
  else
  {
    req->tx_options = 0;
  }
  req->radius = 0;
  req->profileid = 0;
  req->clusterid = req_clid;
  req->src_endpoint = 0;
  req->dst_endpoint = 0;
  zb_buf_set_handle(buf, buf);

  TRACE_MSG(TRACE_APS3, "Sending apsde_data.request", (FMT__0));
  ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, buf);
}


static void complex_desc_req(zb_bufid_t buf)
{
  zb_uint8_t *p_byte;
  zb_uint16_t *p_word;

  TRACE_MSG(TRACE_ZDO3, "TEST: complex_desc_req - nwk_addr = %h", (FMT__H, 0x0000));

  p_byte = zb_buf_initial_alloc(buf, 3 * sizeof(zb_uint8_t));
  *p_byte++ = zb_cert_test_inc_zdo_tsn();;
  p_byte = zb_put_next_htole16(p_byte, 0x0000);

  p_word = ZB_BUF_GET_PARAM(buf, zb_uint16_t);
  *p_word = TEST_ZDO_COMPLEX_DESCR_REQ_CLID;
  send_aps_packet(buf);
}


static void user_desc_req(zb_bufid_t buf)
{
  zb_uint8_t *p_byte;
  zb_uint16_t *p_word;

  TRACE_MSG(TRACE_ZDO3, "TEST: user_desc_req - nwk_addr = %h", (FMT__H, 0x0000));

  p_byte = zb_buf_initial_alloc(buf, 3 * sizeof(zb_uint8_t));
  *p_byte++ = zb_cert_test_inc_zdo_tsn();;
  p_byte = zb_put_next_htole16(p_byte, 0x0000);

  p_word = ZB_BUF_GET_PARAM(buf, zb_uint16_t);
  *p_word = TEST_ZDO_USER_DESCR_REQ_CLID;
  send_aps_packet(buf);
}


static void user_desc_set(zb_bufid_t buf)
{
  zb_uint8_t *p_byte;
  zb_uint16_t *p_word;

  zb_uint8_t desc_payload[] = "Dummy Text";

  TRACE_MSG(TRACE_ZDO3, "TEST: user_desc_set - nwk_addr = %h, length = %d",
            (FMT__H_D, ZB_PIBCACHE_NETWORK_ADDRESS(), sizeof(desc_payload)));

  /* tsn + nwk_addr + length + sizeof("Dummy Text") - sizeof(zero-terminated byte) */
  p_byte = zb_buf_initial_alloc(buf, 3*sizeof(zb_uint8_t) + sizeof(desc_payload));
  *p_byte++ = zb_cert_test_inc_zdo_tsn();;
  p_byte = zb_put_next_htole16(p_byte, 0x0000);
  *p_byte++ = sizeof(desc_payload) - sizeof(zb_uint8_t);
  ZB_MEMCPY(p_byte, desc_payload, sizeof(desc_payload) - sizeof(zb_uint8_t));

  p_word = ZB_BUF_GET_PARAM(buf, zb_uint16_t);
  *p_word = TEST_ZDO_USER_DESCR_SET_CLID;
  send_aps_packet(buf);
}


static void end_dev_annce(zb_bufid_t buf)
{
  if (g_error < 2)
  {
    /* one error is allowed - power descriptor is different */
    TRACE_MSG(TRACE_APS1, "Test status: OK", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_APS1, "Test FAILED", (FMT__0));
  }

  TRACE_MSG(TRACE_APS1, "test is finished, error counter %d", (FMT__D, g_error));

  /* TODO: Complex_Desc_req() User_Desc_req() Discovery_Register_req()
   * User_Desc_Set_req() are not supported */

  {
    zb_uint8_t *p_byte;
    zb_uint16_t *p_word;

    TRACE_MSG(TRACE_ZDO3, "TEST: end_device_annce - nwk_addr = 0x%x", (FMT__H, 0xffff));

    p_byte = zb_buf_initial_alloc(buf, 12 * sizeof(zb_uint8_t));
    *p_byte++ = zb_cert_test_inc_zdo_tsn();;
    p_byte = zb_put_next_htole16(p_byte, ZB_PIBCACHE_NETWORK_ADDRESS());
    zb_get_long_address(p_byte);
    p_byte += sizeof(zb_ieee_addr_t);
    *p_byte = 0;
    ZB_MAC_CAP_SET_ALLOCATE_ADDRESS(*p_byte, 1);
    if (ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()))
    {
      ZB_MAC_CAP_SET_RX_ON_WHEN_IDLE(*p_byte, 1);
    }

    p_word = ZB_BUF_GET_PARAM(buf, zb_uint16_t);
    *p_word = ZDO_DEVICE_ANNCE_CLID;

    send_aps_packet(buf);
  }
}


static void match_desc_callback(zb_bufid_t buf)
{
  zb_uint8_t *zdp_cmd = zb_buf_begin(buf);
  zb_zdo_match_desc_resp_t *resp = (zb_zdo_match_desc_resp_t*)zdp_cmd;
  zb_uint8_t *match_list = (zb_uint8_t*)(resp + 1);

  TRACE_MSG(TRACE_APS1, "match_desc_callback status %hd, addr 0x%x",
            (FMT__H, resp->status, resp->nwk_addr));
  if (resp->status != ZB_ZDP_STATUS_SUCCESS || resp->nwk_addr != 0x0)
  {
    TRACE_MSG(TRACE_APS1, "Error incorrect status/addr", (FMT__0));
    g_error++;
  }
  /*
    asdu=Match_Descr_rsp(Status=0x00=Success, NWKAddrOfInterest=0x0000,
    MatchLength=0x01, MatchList=0x01)
  */
  TRACE_MSG(TRACE_APS1, "match_len %hd, list %hd ", (FMT__H_H, resp->match_len, *match_list));
  if (resp->match_len != 1 || *match_list != 1)
  {
    TRACE_MSG(TRACE_APS1, "Error incorrect match result", (FMT__0));
    g_error++;
  }

  zb_buf_free(buf);
}


static void get_match_desc(zb_bufid_t buf)
{
  zb_zdo_match_desc_param_t *req;

  TRACE_MSG(TRACE_APS1, "get_match_desc", (FMT__0));

  req = zb_buf_initial_alloc(buf, sizeof(zb_zdo_match_desc_param_t) + (2 + 3) * sizeof(zb_uint16_t));

/*
  NWKAddrOfInterest=0x0000 16-bit DUT ZC NWK address
  ProfileID=Profile of interest to match=0x0103
  NumInClusters=Number of input clusters to match=0x02,
  InClusterList=matching cluster list=0x54 0xe0
  NumOutClusters=return value=0x03
  OutClusterList=return value=0x1c 0x38 0xa8
*/

  req->nwk_addr = 0; //send to coordinator
  req->addr_of_interest = req->nwk_addr;
  req->profile_id = 0x0103;
  req->num_in_clusters = 2;
  req->num_out_clusters = 3;
  req->cluster_list[0] = 0x54;
  req->cluster_list[1] = 0xe0;

  req->cluster_list[2] = 0x1c;
  req->cluster_list[3] = 0x38;
  req->cluster_list[4] = 0xa8;

  zb_zdo_match_desc_req(buf, match_desc_callback);
}

static void active_ep_callback(zb_bufid_t buf)
{
  zb_uint8_t *zdp_cmd = zb_buf_begin(buf);
  zb_zdo_ep_resp_t *resp = (zb_zdo_ep_resp_t*)zdp_cmd;
  zb_uint8_t *ep_list = zdp_cmd + sizeof(zb_zdo_ep_resp_t);

  TRACE_MSG(TRACE_APS1, "active_ep_callback status %hd, addr 0x%x",
            (FMT__H, resp->status, resp->nwk_addr));

  if (resp->status != ZB_ZDP_STATUS_SUCCESS || resp->nwk_addr != 0x0)
  {
    TRACE_MSG(TRACE_APS1, "Error incorrect status/addr", (FMT__0));
    g_error++;
  }

  TRACE_MSG(TRACE_APS1, " ep count %hd, ep %hd", (FMT__H_H, resp->ep_count, *ep_list));
  if (resp->ep_count != 1 || *ep_list != 1)
  {
    TRACE_MSG(TRACE_APS3, "Error incorrect ep count or ep value", (FMT__0));
    g_error++;
  }

  zb_buf_free(buf);
}

static void get_active_ep(zb_bufid_t buf)
{
  zb_zdo_active_ep_req_t *req;

  TRACE_MSG(TRACE_APS1, "get_active_ep", (FMT__0));

  req = zb_buf_initial_alloc(buf, sizeof(zb_zdo_active_ep_req_t));
  req->nwk_addr = 0; //coord addr
  zb_zdo_active_ep_req(buf, active_ep_callback);
}

static void simple_desc_callback(zb_bufid_t buf)
{
  zb_zdo_simple_desc_resp_t *resp = (zb_zdo_simple_desc_resp_t*)zb_buf_begin(buf);
  zb_uint_t i;
  zb_uint8_t counter = resp->simple_desc.app_input_cluster_count +
                       resp->simple_desc.app_output_cluster_count;
  for (i = 0; i < counter; i++)
  {
    ZB_LETOH16_XOR(&resp->simple_desc.app_cluster_list[i]);
  }

  TRACE_MSG(TRACE_APS1, "simple_desc_callback status %hd, addr 0x%x",
            (FMT__H, resp->hdr.status, resp->hdr.nwk_addr));
  if (resp->hdr.status != ZB_ZDP_STATUS_SUCCESS || resp->hdr.nwk_addr != 0x0)
  {
    TRACE_MSG(TRACE_APS1, "Error incorrect status/addr", (FMT__0));
    g_error++;
  }

/*
simple descriptor for test SimpleDescriptor=
Endpoint=0x01, Application profile identifier=0x0103, Application device
identifier=0x0000, Application device version=0b0000, Application
flags=0b0000, Application input cluster count=0x0A, Application input
cluster list=0x00 0x03 0x04 0x38 0x54 0x70 0x8c 0xc4 0xe0 0xff,
Application output cluster count=0x0A, Application output cluster
list=0x00 0x01 0x02 0x1c 0x38 0x70 0x8c 0xa8 0xc4 0xff
 */
  /* we need to unpack response to fit resp structure */

  /*end of unpack*/
  TRACE_MSG(TRACE_APS1, "ep %hd, app prof %d, dev id %d, dev ver %hd, input count 0x%hx, output count 0x%hx",
            (FMT__H_D_D_H_H_H, resp->simple_desc.endpoint, resp->simple_desc.app_profile_id,
            resp->simple_desc.app_device_id, resp->simple_desc.app_device_version,
            resp->simple_desc.app_input_cluster_count, resp->simple_desc.app_output_cluster_count));

  TRACE_MSG(TRACE_APP1, "clusters:", (FMT__0));
  for(i = 0; i < counter; i++)
  {
    TRACE_MSG(TRACE_APS1, " 0x%hx", (FMT__H, *(resp->simple_desc.app_cluster_list + i)));
  }

  zb_buf_free(buf);
}

static void get_simple_desc(zb_bufid_t buf)
{
  zb_zdo_simple_desc_req_t *req;

  TRACE_MSG(TRACE_APS1, "get_simple_desc", (FMT__0));

  req = zb_buf_initial_alloc(buf, sizeof(zb_zdo_simple_desc_req_t));
  req->nwk_addr = 0; //send to coordinator
  req->endpoint = 1;
  zb_zdo_simple_desc_req(buf, simple_desc_callback);
}

static void node_power_desc_callback(zb_bufid_t buf)
{
  zb_uint8_t *zdp_cmd = zb_buf_begin(buf);
  zb_zdo_power_desc_resp_t *resp = (zb_zdo_power_desc_resp_t*)(zdp_cmd);

  TRACE_MSG(TRACE_APS1, " node_power_desc_callback status %hd, addr 0x%x",
            (FMT__H, resp->hdr.status, resp->hdr.nwk_addr));
  if (resp->hdr.status != ZB_ZDP_STATUS_SUCCESS || resp->hdr.nwk_addr != 0x0)
  {
    TRACE_MSG(TRACE_APS1, "Error incorrect status/addr", (FMT__0));
    g_error++;
  }

  TRACE_MSG(TRACE_APS1, "power mode %hd, avail power src %hd, cur power src %hd, cur power level %hd",
            (FMT__H_H_H_H, ZB_GET_POWER_DESC_CUR_POWER_MODE(&resp->power_desc),
             ZB_GET_POWER_DESC_AVAIL_POWER_SOURCES(&resp->power_desc),
             ZB_GET_POWER_DESC_CUR_POWER_SOURCE(&resp->power_desc),
             ZB_GET_POWER_DESC_CUR_POWER_SOURCE_LEVEL(&resp->power_desc)));
/* PowerDescriptor=Current power mode=0b0000, Available power mode=0b0111, Current
   power source=0b0001, Current power source level=0b1100 */
  if (ZB_GET_POWER_DESC_CUR_POWER_MODE(&resp->power_desc) != 0 ||
      ZB_GET_POWER_DESC_AVAIL_POWER_SOURCES(&resp->power_desc) != 0x7 ||
      ZB_GET_POWER_DESC_CUR_POWER_SOURCE(&resp->power_desc) != 0x3 ||
      ZB_GET_POWER_DESC_CUR_POWER_SOURCE_LEVEL(&resp->power_desc) != 0xC)
  {
    TRACE_MSG(TRACE_APS1, "Error incorrect power desc", (FMT__0));
    g_error++;
  }

  zb_buf_free(buf);
}

static void get_power_desc(zb_bufid_t buf)
{
  zb_zdo_power_desc_req_t *req;

  TRACE_MSG(TRACE_APS1, "get_power_desc", (FMT__0));

  req = zb_buf_initial_alloc(buf, sizeof(zb_zdo_power_desc_req_t));
  req->nwk_addr = 0; //send to coordinator
  zb_zdo_power_desc_req(buf, node_power_desc_callback);
}


static void node_desc_callback(zb_bufid_t buf)
{
  zb_uint8_t *zdp_cmd = zb_buf_begin(buf);
  zb_zdo_node_desc_resp_t *resp = (zb_zdo_node_desc_resp_t*)(zdp_cmd);
  zb_uint16_t tmp_desc_flags;

  TRACE_MSG(TRACE_APS1, "node_desc_callback: status %hd, addr 0x%x",
            (FMT__H_D, resp->hdr.status, resp->hdr.nwk_addr));
  if (resp->hdr.status != ZB_ZDP_STATUS_SUCCESS || resp->hdr.nwk_addr != 0x0)
  {
    TRACE_MSG(TRACE_APS1, "Error incorrect status/addr", (FMT__0));
    g_error++;
  }
  ZB_LETOH16(&tmp_desc_flags, &resp->node_desc.node_desc_flags);
  resp->node_desc.node_desc_flags = tmp_desc_flags;
  TRACE_MSG(TRACE_APS1, "logic type %hd, aps flag %hd, frequency %hd",
            (FMT__H_H_H, ZB_GET_NODE_DESC_LOGICAL_TYPE(&resp->node_desc),
             ZB_GET_NODE_DESC_APS_FLAGS(&resp->node_desc),
             ZB_GET_NODE_DESC_FREQ_BAND(&resp->node_desc)));
  if (ZB_GET_NODE_DESC_LOGICAL_TYPE(&resp->node_desc) != 0 || ZB_GET_NODE_DESC_APS_FLAGS(&resp->node_desc) != 0 || ZB_GET_NODE_DESC_FREQ_BAND(&resp->node_desc) != ZB_FREQ_BAND_2400 )
  {
    TRACE_MSG(TRACE_APS1, "Error incorrect type/flags/freq", (FMT__0));
    g_error++;
  }

  TRACE_MSG(TRACE_APS1, "mac cap 0x%hx, manufact code %hd, max buf %hd, max transfer %hd",
            (FMT__H_H_H_H, resp->node_desc.mac_capability_flags, resp->node_desc.manufacturer_code,
             resp->node_desc.max_buf_size, resp->node_desc.max_incoming_transfer_size));
  if ((resp->node_desc.mac_capability_flags & 0xB) != 0xB /*0b0X001X11*/ || (resp->node_desc.mac_capability_flags & ~0x4f) != 0 ||
      resp->node_desc.manufacturer_code != 0 ||
      resp->node_desc.max_incoming_transfer_size != 0)
  {
    TRACE_MSG(TRACE_APS1, "Error incorrect cap/manuf code/max transfer", (FMT__0));
    g_error++;
  }

  zb_buf_free(buf);
}

static void get_node_desc(zb_bufid_t buf)
{
  zb_zdo_node_desc_req_t *req;

  TRACE_MSG(TRACE_APS1, "get_node_desc", (FMT__0));

  req = zb_buf_initial_alloc(buf, sizeof(zb_zdo_node_desc_req_t));
  req->nwk_addr = 0; //send to coordinator
  zb_zdo_node_desc_req(buf, node_desc_callback);
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t i_step;
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));

        for (i_step = STEP_ZED1_NODE_DESCR_REQ; i_step <= STEP_ZED1_END_DEVICE_ANNCE; i_step++)
        {
          test_step_register(buffer_manager, i_step, TP_ZDO_BV_11_STEP_TIME_ZED1);
        }

        test_control_start(TEST_MODE, TP_ZDO_BV_11_STEP_1_DELAY_ZED1);
        break;
      case ZB_COMMON_SIGNAL_CAN_SLEEP:
#ifdef ZB_USE_SLEEP
    	  zb_sleep_now();
#endif /* ZB_USE_SLEEP */
        break;

      default:
        TRACE_MSG(TRACE_ERROR, "Unknown signal %hd", (FMT__H, sig));
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

  zb_buf_free(param);
}
