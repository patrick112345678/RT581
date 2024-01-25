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
/* PURPOSE: Test for simple descriptor srv and sf data recv at NCP
*/

#define ZB_TEST_NAME TP_ZDO_BV_09_ZED1
#define ZB_TRACE_FILE_ID 41686

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "test_common.h"

#define HW_CONNECTION_DELAY (30 * ZB_TIME_ONE_SECOND)

/* For NS build first ieee addr byte should be unique */
static zb_ieee_addr_t g_ieee_addr = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static zb_uint8_t g_error = 0;

static zb_uint_t s_ep_count;
static zb_uint8_t s_ep_iter;
static zb_uint8_t s_ep[64];
static zb_uint8_t s_data_ep;
static zb_uint16_t s_profile;

static void send_active_ep_req(zb_uint8_t param);
static void send_simple_desc_req(zb_uint8_t param);

MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("zdo_2_zed1");

  /* set ieee addr */
  zb_set_long_address(g_ieee_addr);

  /* become an ED */
  zb_set_network_ed_role_legacy(TEST_CHANNEL);
  zb_set_rx_on_when_idle(ZB_FALSE);

  zb_set_nvram_erase_at_start(ZB_TRUE);

  if (zboss_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
  }
  else
  {
    zboss_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}


#if 0                           /* no need match desc */
//! [zb_zdo_match_desc_req]
static void match_desc_invalid_callback(zb_uint8_t param)
{
  zb_uint8_t *zdp_cmd = zb_buf_begin(param);
  zb_zdo_match_desc_resp_t *resp = (zb_zdo_match_desc_resp_t*)zdp_cmd;

  TRACE_MSG(TRACE_APP1, "match_desc_invalid_callback status %hd, addr 0x%x",
            (FMT__H, resp->status, resp->nwk_addr));
  if (resp->status != ZB_ZDP_STATUS_SUCCESS || resp->nwk_addr != 0x0)
  {
    TRACE_MSG(TRACE_APP1, "Error incorrect status/addr", (FMT__0));
    g_error++;
  }
  /*
    asdu=Match_Descr_rsp(Status=0x00=Success, NWKAddrOfInterest=0x0000,
    MatchLength=0x00, MatchList=empty)
  */
  TRACE_MSG(TRACE_APP1, "match_len %hd ", (FMT__H, resp->match_len));
  if (resp->match_len != 0)
  {
    TRACE_MSG(TRACE_APP1, "Error incorrect match result", (FMT__0));
    g_error++;
  }

  TRACE_MSG(TRACE_APP1, "test is finished, error counter %hd", (FMT__H, g_error));
  if (g_error <= 1)
  {
    /* one error is allowed - power descriptor is different */
    TRACE_MSG(TRACE_APP1, "Test status: OK", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_APP1, "Test FAILED", (FMT__0));
  }

  zb_buf_free(param);
}


static void send_match_desc_req_invalid(zb_bufid_t buf)
{
  zb_zdo_match_desc_param_t *req;
  ZB_BUF_INITIAL_ALLOC(buf, sizeof(zb_zdo_match_desc_param_t) + (2 + 3) * sizeof(zb_uint16_t), req);

  /*
  NWKAddrOfInterest=0x0000 16-bit DUT ZC NWK address
  ProfileID=Profile of interest to match=0x0103
  NumInClusters=Number of input clusters to match=0x01,
  InClusterList=matching cluster list=0x75
  NumOutClusters=return value=0x0
  OutClusterList=empty
  */

  req->nwk_addr = 0; //send to coordinator
  req->addr_of_interest = req->nwk_addr;
  req->profile_id = 0x0103;
  req->num_in_clusters = 1;
  req->num_out_clusters = 0;
  req->cluster_list[0] = 0x75;

  zb_zdo_match_desc_req(param, match_desc_invalid_callback);
}
//! [zb_zdo_match_desc_req]


//! [zb_zdo_match_desc_req]
static void match_desc_valid_callback(zb_uint8_t param)
{
  zb_uint8_t *zdp_cmd = zb_buf_begin(param);
  zb_zdo_match_desc_resp_t *resp = (zb_zdo_match_desc_resp_t*)zdp_cmd;
  zb_uint8_t *match_list = (zb_uint8_t*)(resp + 1);

  TRACE_MSG(TRACE_APP1, "match_desc_valid_callback status %hd, addr 0x%x",
            (FMT__H, resp->status, resp->nwk_addr));
  if (resp->status != ZB_ZDP_STATUS_SUCCESS || resp->nwk_addr != 0x0)
  {
    TRACE_MSG(TRACE_APP1, "Error incorrect status/addr", (FMT__0));
    g_error++;
  }
  /*
    asdu=Match_Descr_rsp(Status=0x00=Success, NWKAddrOfInterest=0x0000,
    MatchLength=0x01, MatchList=0x01)
  */
  TRACE_MSG(TRACE_APP1, "match_len %hd, list %hd ", (FMT__H_H, resp->match_len, *match_list));
  if (resp->match_len != 1 || *match_list != 1)
  {
    TRACE_MSG(TRACE_APP1, "Error incorrect match result", (FMT__0));
    g_error++;
  }

  send_match_desc_req_invalid(buf);
}


static void send_match_desc_req_valid(zb_bufid_t buf)
{
  zb_zdo_match_desc_param_t *req;
  ZB_BUF_INITIAL_ALLOC(buf, sizeof(zb_zdo_match_desc_param_t) + (2 + 3) * sizeof(zb_uint16_t), req);

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

  zb_zdo_match_desc_req(param, match_desc_valid_callback);
}
//! [zb_zdo_match_desc_req]
#endif  /* 0 */


static void send_data_to_clu(zb_uint8_t param, zb_uint16_t clu)
{
  zb_apsde_data_req_t *req;
  zb_uint8_t *ptr = NULL;
  zb_short_t i;

  ZB_BUF_INITIAL_ALLOC(param, ZB_TEST_DATA_SIZE , ptr);
  req = ZB_GET_BUF_TAIL(param, sizeof(zb_apsde_data_req_t));
  req->dst_addr.addr_short = 0;
  req->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
  req->tx_options = ZB_APSDE_TX_OPT_ACK_TX;
  req->radius = 10;
  req->profileid = s_profile;
  req->src_endpoint = s_data_ep;
  req->dst_endpoint = s_data_ep;
  req->clusterid = clu;
  for (i = 0 ; i < ZB_TEST_DATA_SIZE ; ++i)
  {
    ptr[i] = i % 32 + '0';
  }
  TRACE_MSG(TRACE_APS3, "Sending apsde_data.request %hd", (FMT__H, param));
  ZB_SCHEDULE_APP_CALLBACK(zb_apsde_data_request, param);
}


//! [zb_zdo_simple_desc_req]
static void simple_desc_callback(zb_uint8_t param)
{
  zb_uint8_t *zdp_cmd = zb_buf_begin(param);
  zb_zdo_simple_desc_resp_t *resp = (zb_zdo_simple_desc_resp_t*)(zdp_cmd);
  zb_uint_t i;

  TRACE_MSG(TRACE_APP1, "simple_desc_callback status %hd, addr 0x%x",
            (FMT__H, resp->hdr.status, resp->hdr.nwk_addr));
  if (resp->hdr.status != ZB_ZDP_STATUS_SUCCESS || resp->hdr.nwk_addr != 0x0)
  {
    TRACE_MSG(TRACE_APP1, "Error incorrect status/addr", (FMT__0));
    g_error++;
  }

  TRACE_MSG(TRACE_APP1, "ep %hd, app prof %d, dev id %d, dev ver %hd, input count 0x%hx, output count 0x%hx",
            (FMT__H_D_D_H_H_H, resp->simple_desc.endpoint, resp->simple_desc.app_profile_id,
            resp->simple_desc.app_device_id, resp->simple_desc.app_device_version,
           resp->simple_desc.app_input_cluster_count, resp->simple_desc.app_output_cluster_count));

  TRACE_MSG(TRACE_APP1, "clusters:", (FMT__0));
  for(i = 0; i < resp->simple_desc.app_input_cluster_count + resp->simple_desc.app_output_cluster_count; i++)
  {
    TRACE_MSG(TRACE_APP1, " 0x%hx", (FMT__H, *(resp->simple_desc.app_cluster_list + i)));
  }

  /* There can be races if we have no enough buffers to send data before receiving resp to the next simple desc req. But this is just a test... */
  s_data_ep = s_ep[s_ep_iter];
  s_profile = resp->simple_desc.app_profile_id;
  for(i = 0; i < resp->simple_desc.app_input_cluster_count; i++)
  {
    ZB_GET_IN_BUF_DELAYED2(send_data_to_clu, resp->simple_desc.app_cluster_list[i]);
  }

  s_ep_iter++;
  if (s_ep_iter < s_ep_count)
  {
    ZB_SCHEDULE_APP_CALLBACK(send_simple_desc_req, param);
  }
}


static void send_simple_desc_req(zb_uint8_t param)
{
  zb_zdo_simple_desc_req_t * req;
  ZB_BUF_INITIAL_ALLOC(param, sizeof(zb_zdo_simple_desc_req_t), req);
  req->nwk_addr = 0; //send to coordinator
  req->endpoint = s_ep[s_ep_iter];
  TRACE_MSG(TRACE_APP1, "send_simple_desc_req: iter %d ep %d", (FMT__D_H, s_ep_iter, req->endpoint));
  zb_zdo_simple_desc_req(param, simple_desc_callback);
}
//! [zb_zdo_simple_desc_req]


//! [zb_zdo_power_desc_req]
static void node_power_desc_callback(zb_uint8_t param)
{
  zb_uint8_t *zdp_cmd = zb_buf_begin(param);
  zb_zdo_power_desc_resp_t *resp = (zb_zdo_power_desc_resp_t*)(zdp_cmd);

  TRACE_MSG(TRACE_APP1, " node_power_desc_callback status %hd, addr 0x%x",
            (FMT__H, resp->hdr.status, resp->hdr.nwk_addr));
  if (resp->hdr.status != ZB_ZDP_STATUS_SUCCESS || resp->hdr.nwk_addr != 0x0)
  {
    TRACE_MSG(TRACE_APP1, "Error incorrect status/addr", (FMT__0));
    g_error++;
  }

  TRACE_MSG(TRACE_APP1, "power mode %hd, avail power src %hd, cur power src %hd, cur power level %hd",
            (FMT__H_H_H_H, ZB_GET_POWER_DESC_CUR_POWER_MODE(&resp->power_desc),
             ZB_GET_POWER_DESC_AVAIL_POWER_SOURCES(&resp->power_desc),
             ZB_GET_POWER_DESC_CUR_POWER_SOURCE(&resp->power_desc),
             ZB_GET_POWER_DESC_CUR_POWER_SOURCE_LEVEL(&resp->power_desc)));
/* PowerDescriptor=Current power mode=0b0000, Available power mode=0b0111, Current
   power source=0b0001, Current power source level=0b1100 */
  if (ZB_GET_POWER_DESC_CUR_POWER_MODE(&resp->power_desc) != 0 ||
      ZB_GET_POWER_DESC_AVAIL_POWER_SOURCES(&resp->power_desc) != 0x7 ||
      ZB_GET_POWER_DESC_CUR_POWER_SOURCE(&resp->power_desc) != 0x1 ||
      ZB_GET_POWER_DESC_CUR_POWER_SOURCE_LEVEL(&resp->power_desc) != 0xC)
  {
    TRACE_MSG(TRACE_APP1, "Error incorrect power desc", (FMT__0));
    g_error++;
  }

  send_active_ep_req(param);
}


static void send_power_desc_req(zb_bufid_t buf)
{
  zb_zdo_power_desc_req_t *req;
  ZB_BUF_INITIAL_ALLOC(buf, sizeof(zb_zdo_power_desc_req_t), req);
  req->nwk_addr = 0; //send to coordinator
  zb_zdo_power_desc_req(param, node_power_desc_callback);
}
//! [zb_zdo_power_desc_req]


//! [zb_zdo_node_desc_req]
static void node_desc_callback(zb_uint8_t param)
{
  zb_uint8_t *zdp_cmd = zb_buf_begin(param);
  zb_zdo_node_desc_resp_t *resp = (zb_zdo_node_desc_resp_t*)(zdp_cmd);

  TRACE_MSG(TRACE_APP1, "node_desc_callback: status %hd, addr 0x%x",
            (FMT__H_D, resp->hdr.status, resp->hdr.nwk_addr));
  if (resp->hdr.status != ZB_ZDP_STATUS_SUCCESS || resp->hdr.nwk_addr != 0x0)
  {
    TRACE_MSG(TRACE_APP1, "Error incorrect status/addr", (FMT__0));
    g_error++;
  }
  ZB_LETOH16_XOR(resp->node_desc.node_desc_flags);
  TRACE_MSG(TRACE_APP1, "logic type %hd, aps flag %hd, frequency %hd",
            (FMT__H_H_H, ZB_GET_NODE_DESC_LOGICAL_TYPE(&resp->node_desc), ZB_GET_NODE_DESC_APS_FLAGS(&resp->node_desc),
             ZB_GET_NODE_DESC_FREQ_BAND(&resp->node_desc)));
  if (ZB_GET_NODE_DESC_LOGICAL_TYPE(&resp->node_desc) != 0 || ZB_GET_NODE_DESC_APS_FLAGS(&resp->node_desc) != 0
  ||     ZB_GET_NODE_DESC_FREQ_BAND(&resp->node_desc) != ZB_FREQ_BAND_2400 )
  {
    TRACE_MSG(TRACE_APP1, "Error incorrect type/flags/freq", (FMT__0));
    g_error++;
  }

  TRACE_MSG(TRACE_APP1, "mac cap 0x%hx, manufact code %hd, max buf %hd, max transfer %hd",
            (FMT__H_H_H_H, resp->node_desc.mac_capability_flags, resp->node_desc.manufacturer_code,
             resp->node_desc.max_buf_size, resp->node_desc.max_incoming_transfer_size));
  if ((resp->node_desc.mac_capability_flags & 0xB) != 0xB /*0b0X001X11*/ || (resp->node_desc.mac_capability_flags & ~0x4f) != 0 ||
      resp->node_desc.manufacturer_code != 0 ||
      resp->node_desc.max_incoming_transfer_size != 0)
  {
    TRACE_MSG(TRACE_APP1, "Error incorrect cap/manuf code/max transfer", (FMT__0));
    g_error++;
  }

  send_power_desc_req(buf);
}


static void get_node_desc(zb_uint8_t param)
{
  zb_bufid_t asdu;
  zb_zdo_node_desc_req_t *req;

  TRACE_MSG(TRACE_APP1, "get_node_desc", (FMT__0));
  ZVUNUSED(param);

  asdu = ZB_GET_OUT_BUF();
  if (!asdu)
  {
    TRACE_MSG(TRACE_ERROR, "out buf alloc failed!", (FMT__0));
  }
  else
  {
    ZB_BUF_INITIAL_ALLOC(asdu, sizeof(zb_zdo_node_desc_req_t), req);
    req->nwk_addr = 0; //send to coordinator
    zb_zdo_node_desc_req(ZB_REF_FROM_BUF(asdu), node_desc_callback);
  }
}
//! [zb_zdo_node_desc_req]


//! [zb_zdo_active_ep_req]
static void active_ep_callback(zb_uint8_t param)
{
  zb_uint8_t *zdp_cmd = zb_buf_begin(param);
  zb_zdo_ep_resp_t *resp = (zb_zdo_ep_resp_t*)zdp_cmd;
  zb_uint8_t *ep_list = zdp_cmd + sizeof(zb_zdo_ep_resp_t);
  zb_uint_t i;

  TRACE_MSG(TRACE_APP1, "active_ep_callback status %hd, addr 0x%x",
            (FMT__H, resp->status, resp->nwk_addr));

  if (resp->status != ZB_ZDP_STATUS_SUCCESS || resp->nwk_addr != 0x0)
  {
    TRACE_MSG(TRACE_APP1, "Error incorrect status/addr", (FMT__0));
    g_error++;
  }

  TRACE_MSG(TRACE_APP1, " ep count %hd, ep %hd", (FMT__H_H, resp->ep_count, *ep_list));

  s_ep_count = resp->ep_count;
  for (i = 0 ; i < s_ep_count ; ++i)
  {
    s_ep[i] = ep_list[i];
    TRACE_MSG(TRACE_APP3, "ep %hd", (FMT__H, s_ep[i]));
  }

  if (s_ep_count > 0)
  {
    s_ep_iter = 0;
    ZB_SCHEDULE_APP_CALLBACK(send_simple_desc_req, param);
  }
}


static void send_active_ep_req(zb_uint8_t param)
{
  zb_zdo_active_ep_req_t *req;
  ZB_BUF_INITIAL_ALLOC(param, sizeof(zb_zdo_active_ep_req_t), req);
  req->nwk_addr = 0; //coord addr
  zb_zdo_active_ep_req(param, active_ep_callback);
}
//! [zb_zdo_active_ep_req]



ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);
  /* TRACE_MSG(TRACE_ERROR, ">>zb_zdo_startup_complete status %d", (FMT__D, status)); */

  if (0 == status)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));
        ZB_SCHEDULE_APP_CALLBACK(get_node_desc, param);
        param = 0;
        break;

      case ZB_COMMON_SIGNAL_CAN_SLEEP:
    	zb_sleep_now();
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
