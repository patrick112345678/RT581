
/* ZBOSS Zigbee software protocol stack
 *
 * Copyright (c) 2012-2020 DSR Corporation, Denver CO, USA.
 * http://www.dsr-zboss.com
 * http://www.dsr-corporation.com
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
/*  PURPOSE: NCP High level transport implementation for the host side: NWKMGMT category
*/

#define ZB_TRACE_FILE_ID 17508
#include "ncp_host_hl_proto.h"


static void handle_nwk_formation_response(ncp_hl_response_header_t* response,
                                          zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  ZVUNUSED(len);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_formation_response, status_code %d",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  adaptor_handle_nwk_formation_response(error_code);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_formation_response", (FMT__0));
}

static void handle_nwk_permit_joining_response(ncp_hl_response_header_t* response,
                                               zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  ZVUNUSED(len);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_permit_joining_response, status_code %d",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  adaptor_handle_nwk_permit_joining_response(error_code);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_formation_response", (FMT__0));
}

static void handle_nwk_get_ieee_by_short_response(ncp_hl_response_header_t* response,
                                                  zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  zb_ieee_addr_t ieee_addr;
  ncp_host_hl_rx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_get_ieee_by_short_response, status_code %d",
            (FMT__D, error_code));

  if (error_code == RET_OK)
  {
    TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_nwk_get_ieee_by_short_response", (FMT__0));

    ncp_host_hl_init_response_body(response, len, &body);
    ncp_host_hl_buf_get_u64addr(&body, ieee_addr);

    TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_nwk_get_ieee_by_short_response, ieee_addr " TRACE_FORMAT_64,
              (FMT__A, TRACE_ARG_64(ieee_addr)));
  }
  ncp_host_mark_blocking_request_finished();

  ncp_host_handle_nwk_get_ieee_by_short_response(error_code, ieee_addr);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_get_ieee_by_short_response", (FMT__0));
}


static void handle_nwk_get_neighbor_by_ieee_response(ncp_hl_response_header_t* response,
                                                     zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  ncp_host_hl_rx_buf_handle_t body;
  ncp_hl_response_neighbor_by_ieee_t ncp_nbt;

  TRACE_MSG(TRACE_TRANSPORT3, ">>  handle_nwk_get_neighbor_by_ieee, status_code %d",
            (FMT__D, error_code));

  if (error_code == RET_OK)
  {
    TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_nwk_get_neighbor_by_ieee", (FMT__0));

    ncp_host_hl_init_response_body(response, len, &body);

    ncp_host_hl_buf_get_u64addr(&body, ncp_nbt.ieee_addr);
    TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt.ieee_addr " TRACE_FORMAT_64,
              (FMT__A, TRACE_ARG_64(ncp_nbt.ieee_addr)));

    ncp_host_hl_buf_get_u16(&body, &ncp_nbt.short_addr);
    TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt.short_addr 0x%x", (FMT__D, ncp_nbt.short_addr));

    ncp_host_hl_buf_get_u8(&body, &ncp_nbt.device_type);
    TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt.device_type 0x%x", (FMT__D, ncp_nbt.device_type));

    ncp_host_hl_buf_get_u8(&body, &ncp_nbt.rx_on_when_idle);
    TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt.rx_on_when_idle 0x%x", (FMT__D, ncp_nbt.rx_on_when_idle));

    ncp_host_hl_buf_get_u16(&body, &ncp_nbt.ed_config);
    TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt.ed_config 0x%x", (FMT__D, ncp_nbt.ed_config));

    ncp_host_hl_buf_get_u32(&body, &ncp_nbt.timeout_counter);
    TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt.timeout_counter %d", (FMT__D, ncp_nbt.timeout_counter));

    ncp_host_hl_buf_get_u32(&body, &ncp_nbt.device_timeout);
    TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt.device_timeout %d", (FMT__D, ncp_nbt.device_timeout));

    ncp_host_hl_buf_get_u8(&body, &ncp_nbt.relationship);
    TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt.relationship 0x%x", (FMT__D, ncp_nbt.relationship));

    ncp_host_hl_buf_get_u8(&body, &ncp_nbt.transmit_failure_cnt);
    TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt.transmit_failure_cnt %d", (FMT__D, ncp_nbt.transmit_failure_cnt));

    ncp_host_hl_buf_get_u8(&body, &ncp_nbt.lqi);
    TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt.lqi %d", (FMT__D, ncp_nbt.lqi));

#ifdef ZB_ROUTER_ROLE
    ncp_host_hl_buf_get_u8(&body, &ncp_nbt.outgoing_cost);
    TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt.outgoing_cost %d", (FMT__D, ncp_nbt.outgoing_cost));

    ncp_host_hl_buf_get_u8(&body, &ncp_nbt.age);
    TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt.age %d", (FMT__D, ncp_nbt.age));
#endif /* ZB_ROUTER_ROLE */

    ncp_host_hl_buf_get_u8(&body, &ncp_nbt.keepalive_received);
    TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt.keepalive_received %d", (FMT__D, ncp_nbt.keepalive_received));

    ncp_host_hl_buf_get_u8(&body, &ncp_nbt.mac_iface_idx);
    TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt.mac_iface_idx %d", (FMT__D, ncp_nbt.mac_iface_idx));

    /* TODO: implement converting ncp_hl_response_neighbor_by_ieee_t to zb_neighbor_tbl_ent_t */
    TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_nwk_get_neighbor_by_ieee", (FMT__0));

  }
  ncp_host_mark_blocking_request_finished();

  ncp_host_handle_nwk_get_neighbor_by_ieee_response(error_code);

  TRACE_MSG(TRACE_TRANSPORT3, "<<  handle_nwk_get_neighbor_by_ieee", (FMT__0));
}

static void handle_nwk_get_short_by_ieee_response(ncp_hl_response_header_t* response,
                                                  zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  zb_uint16_t short_addr;
  ncp_host_hl_rx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_get_short_by_ieee_response, status_code %d",
            (FMT__D, error_code));

  if (error_code == RET_OK)
  {
    TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_nwk_get_short_by_ieee_response", (FMT__0));

    ncp_host_hl_init_response_body(response, len, &body);
    ncp_host_hl_buf_get_u16(&body, &short_addr);

    TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_nwk_get_short_by_ieee_response, short_addr 0x%x",
              (FMT__D, short_addr));
  }
  ncp_host_mark_blocking_request_finished();

  ncp_host_handle_nwk_get_short_by_ieee_response(error_code, short_addr);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_get_short_by_ieee_response", (FMT__0));
}

void ncp_host_handle_nwkmgmt_response(void* data, zb_uint16_t len)
{
  ncp_hl_response_header_t* response_header = (ncp_hl_response_header_t*)data;

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwkmgmt_response", (FMT__0));

  switch(response_header->hdr.call_id)
  {
    case NCP_HL_NWK_FORMATION:
      handle_nwk_formation_response(response_header, len);
      break;
    case NCP_HL_NWK_PERMIT_JOINING:
      handle_nwk_permit_joining_response(response_header, len);
      break;
    case NCP_HL_NWK_GET_IEEE_BY_SHORT:
      handle_nwk_get_ieee_by_short_response(response_header, len);
      break;
    case NCP_HL_NWK_GET_NEIGHBOR_BY_IEEE:
      handle_nwk_get_neighbor_by_ieee_response(response_header, len);
      break;
    case NCP_HL_NWK_GET_SHORT_BY_IEEE:
      handle_nwk_get_short_by_ieee_response(response_header, len);
      break;
    default:
      /* TODO: implement handlers for other responses! */
      ZB_ASSERT(0);
    }

   TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwkmgmt_response", (FMT__0));
}

void ncp_host_handle_nwkmgmt_indication(void* data, zb_uint16_t len)
{
  ZVUNUSED(data);
  ZVUNUSED(len);
  /* TODO: implement it! */
  ZB_ASSERT(0);
}

zb_ret_t ncp_host_nwk_formation_transport(zb_nlme_network_formation_request_t *req)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  zb_uint8_t channel_list_len = ZB_CHANNEL_PAGES_NUM;
  zb_uint8_t idx;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_formation_transport", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_NWK_FORMATION, &body))
  {
    ncp_host_hl_buf_put_u8(&body, channel_list_len);
    for (idx = 0; idx < channel_list_len; idx++)
    {
      ncp_host_hl_buf_put_u8(&body, zb_channel_page_list_get_page(req->scan_channels_list, idx));
      ncp_host_hl_buf_put_u32(&body, req->scan_channels_list[idx]);
    }
    ncp_host_hl_buf_put_u8(&body, req->scan_duration);
    ncp_host_hl_buf_put_u8(&body, req->distributed_network);
    ncp_host_hl_buf_put_u16(&body, req->distributed_network_address);

    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_formation_transport, ret %d", (FMT__D, ret));

  return ret;
}

zb_ret_t ncp_host_nwk_permit_joining_transport(zb_nlme_permit_joining_request_t *req)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_permit_joining_transport", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_NWK_PERMIT_JOINING, &body))
  {
    ncp_host_hl_buf_put_u8(&body, req->permit_duration);
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_permit_joining_transport, ret %d", (FMT__D, ret));

  return ret;
}

zb_ret_t ncp_host_nwk_get_ieee_by_short(zb_uint16_t short_addr)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_get_ieee_by_short", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_NWK_GET_IEEE_BY_SHORT, &body))
  {
    ncp_host_hl_buf_put_u16(&body, short_addr);
    ret = ncp_host_hl_send_packet(&body);
  }
  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_get_ieee_by_short, ret %d", (FMT__D, ret));

  return ret;
}

zb_ret_t ncp_host_nwk_get_neighbor_by_ieee(zb_ieee_addr_t ieee_addr)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_get_neighbor_by_ieee", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_NWK_GET_NEIGHBOR_BY_IEEE, &body))
  {
    ncp_host_hl_buf_put_u64addr(&body, ieee_addr);
    ret = ncp_host_hl_send_packet(&body);
  }
  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_get_neighbor_by_ieee, ret %d", (FMT__D, ret));

  return ret;
}

zb_ret_t ncp_host_nwk_get_short_by_ieee(zb_ieee_addr_t ieee_addr)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_get_short_by_ieee", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_NWK_GET_SHORT_BY_IEEE, &body))
  {
    ncp_host_hl_buf_put_u64addr(&body, ieee_addr);
    ret = ncp_host_hl_send_packet(&body);
  }
  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_get_short_by_ieee, ret %d", (FMT__D, ret));

  return ret;
}
