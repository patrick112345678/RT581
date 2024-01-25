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
/*  PURPOSE: NCP High level transport implementation for the host side: APS category
*/
#define ZB_TRACE_FILE_ID 17509

#include "ncp_host_hl_proto.h"
#include "zb_aps.h"

static void handle_apsde_data_indication(ncp_hl_ind_header_t* indication, zb_uint16_t len)
{
  ncp_host_hl_rx_buf_handle_t body;

  zb_uint8_t params_len;
  zb_uint16_t data_len;
  zb_apsde_data_indication_t ind;
  zb_uint8_t *data_ptr;

  zb_uint8_t i;

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_apsde_data_indication ",(FMT__0));

  TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_apsde_data_indication", (FMT__0));

  ncp_host_hl_init_indication_body(indication, len, &body);

  /* Get additional NCP Serial Protocol info */
  ncp_host_hl_buf_get_u8(&body, &params_len);
  ncp_host_hl_buf_get_u16(&body, &data_len);

  TRACE_MSG(TRACE_TRANSPORT3, " params_len %hd, data_len %d",
            (FMT__H_D, params_len, data_len));

  /* Get begin of zb_apsde_data_indication_t */
  ncp_host_hl_buf_get_u8(&body, &ind.fc);
  ncp_host_hl_buf_get_u16(&body, &ind.src_addr);
  ncp_host_hl_buf_get_u16(&body, &ind.dst_addr);
  ncp_host_hl_buf_get_u16(&body, &ind.group_addr);
  ncp_host_hl_buf_get_u8(&body, &ind.dst_endpoint);
  ncp_host_hl_buf_get_u8(&body, &ind.src_endpoint);
  ncp_host_hl_buf_get_u16(&body, &ind.clusterid);
  ncp_host_hl_buf_get_u16(&body, &ind.profileid);
  ncp_host_hl_buf_get_u8(&body, &ind.aps_counter);
  ncp_host_hl_buf_get_u16(&body, &ind.mac_src_addr);
  ncp_host_hl_buf_get_u16(&body, &ind.mac_dst_addr);
  ncp_host_hl_buf_get_u8(&body, &ind.lqi);
  ncp_host_hl_buf_get_u8(&body, &ind.rssi);
  ncp_host_hl_buf_get_u8(&body, (&ind.rssi + 1)); /* APS key source & attr*/

  TRACE_MSG(TRACE_TRANSPORT3, "src_addr 0x%x dst_addr 0x%x",
            (FMT__D_D, ind.src_addr, ind.dst_addr));

  ncp_host_hl_buf_get_ptr(&body, &data_ptr, data_len);

  TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_apsde_data_indication", (FMT__0));

  adaptor_handle_apsde_data_indication(&ind, data_ptr, params_len, data_len);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_apsde_data_indication", (FMT__0));
}


static void handle_apsde_data_request_response(ncp_hl_response_header_t* response,
                                               zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  zb_apsde_data_confirm_t conf;
  zb_ieee_addr_t temp;
  ncp_host_hl_rx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_apsde_data_request_response, status_code %d",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  if (error_code == RET_OK)
  {
    TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_apsde_data_request_response", (FMT__0));

    ncp_host_hl_init_response_body(response, len, &body);

    /* Write received address to the temp long addr in any case*/
    ncp_host_hl_buf_get_u64addr(&body, temp);
    TRACE_MSG(TRACE_TRANSPORT3, "temp address " TRACE_FORMAT_64,
              (FMT__A, TRACE_ARG_64(temp)));

    ncp_host_hl_buf_get_u8(&body, &conf.dst_endpoint);
    ncp_host_hl_buf_get_u8(&body, &conf.src_endpoint);
    TRACE_MSG(TRACE_TRANSPORT3, "conf.dst_endpoint %hd, conf.src_endpoint %hd",
              (FMT__D_D, conf.dst_endpoint, conf.src_endpoint));

    ncp_host_hl_buf_get_u32(&body, &conf.tx_time);
    TRACE_MSG(TRACE_TRANSPORT3, "conf.tx_time %ld", (FMT__L, conf.tx_time));

    ncp_host_hl_buf_get_u8(&body, &conf.addr_mode);
    TRACE_MSG(TRACE_TRANSPORT3, "addr_mode 0x%hx", (FMT__D, conf.addr_mode));

    /* After reading addr_mode, check whether we do have long address */
    if (conf.addr_mode != ZB_APS_ADDR_MODE_64_ENDP_PRESENT)
    {
      ZB_MEMCPY(&conf.dst_addr.addr_short, temp, sizeof(zb_uint16_t));
      ZB_LETOH16_ONPLACE(aps_data_conf.dst_addr.addr_short);
      TRACE_MSG(TRACE_TRANSPORT3, "conf.dst_addr.addr_short 0x%x", (FMT__D, conf.dst_addr.addr_short));
    }
    else
    {
      ZB_MEMCPY(&conf.dst_addr.addr_long, temp, sizeof(zb_ieee_addr_t));
      TRACE_MSG(TRACE_TRANSPORT3, "conf.dst_addr.addr_long " TRACE_FORMAT_64,
                (FMT__A, TRACE_ARG_64(conf.dst_addr.addr_long)));
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_apsde_data_request_response", (FMT__D));
  }

  adaptor_handle_apsde_data_request_response(&conf);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_apsde_data_request_response", (FMT__0));
}

void ncp_host_handle_aps_response(void* data, zb_uint16_t len)
{
  ncp_hl_response_header_t* response_header = (ncp_hl_response_header_t*)data;

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_aps_response", (FMT__0));

  switch(response_header->hdr.call_id)
  {
    case NCP_HL_APSDE_DATA_REQ:
      handle_apsde_data_request_response(response_header, len);
      break;
    default:
      /* TODO: implement handlers for other responses! */
      ZB_ASSERT(0);
    }

   TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_aps_response", (FMT__0));
}

void ncp_host_handle_aps_indication(void* data, zb_uint16_t len)
{
  ncp_hl_ind_header_t* indication_header = (ncp_hl_ind_header_t*)data;

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_aps_indication", (FMT__0));

  switch(indication_header->call_id)
  {
    case NCP_HL_APSDE_DATA_IND:
      handle_apsde_data_indication(indication_header, len);
      break;
    default:
      /* TODO: implement handlers for other responses! */
      ZB_ASSERT(0);
    }

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_aps_indication", (FMT__0));
}

zb_ret_t ncp_host_apsde_data_request_transport(zb_apsde_data_req_t *req, zb_uint8_t param_len,
                                               zb_uint16_t data_len, zb_uint8_t *data_ptr,
                                               zb_uint8_t *tsn)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_apsde_data_request_transport", (FMT__0));

  if (!ncp_host_get_buf_for_nonblocking_request(NCP_HL_APSDE_DATA_REQ, &body, tsn))
  {
    ncp_host_hl_buf_put_u8(&body, param_len);
    TRACE_MSG(TRACE_TRANSPORT2, "param_len %hd", (FMT__H, param_len));

    ncp_host_hl_buf_put_u16(&body, data_len);
    TRACE_MSG(TRACE_TRANSPORT2, "data_len %d", (FMT__H, data_len));

    if (req->addr_mode == ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT || ZB_APS_ADDR_MODE_16_ENDP_PRESENT)
    {
      ncp_host_hl_buf_put_u16(&body, req->dst_addr.addr_short);
      ncp_host_hl_buf_put_u16(&body, 0);
      ncp_host_hl_buf_put_u32(&body, 0);
      TRACE_MSG(TRACE_TRANSPORT2, "req->dst_addr.addr_short 0x%x", (FMT__A, req->dst_addr.addr_short));
    }
    else
    {
      ncp_host_hl_buf_put_u64addr(&body, req->dst_addr.addr_long);
      TRACE_MSG(TRACE_TRANSPORT2, "req->dst_addr.addr_long " TRACE_FORMAT_64,
                (FMT__A, TRACE_ARG_64(req->dst_addr.addr_long)));
    }

    ncp_host_hl_buf_put_u16(&body, req->profileid);
    TRACE_MSG(TRACE_TRANSPORT2, "req->profileid %d", (FMT__D, req->profileid));

    ncp_host_hl_buf_put_u16(&body, req->clusterid);
    TRACE_MSG(TRACE_TRANSPORT2, "req->clusterid %d", (FMT__D, req->clusterid));

    ncp_host_hl_buf_put_u8(&body, req->dst_endpoint);
    TRACE_MSG(TRACE_TRANSPORT2, "req->dst_endpoint %hd", (FMT__H, req->dst_endpoint));

    ncp_host_hl_buf_put_u8(&body, req->src_endpoint);
    TRACE_MSG(TRACE_TRANSPORT2, "req->src_endpoint %hd", (FMT__H, req->src_endpoint));

    ncp_host_hl_buf_put_u8(&body, req->radius);
    TRACE_MSG(TRACE_TRANSPORT2, "req->radius %hd", (FMT__H, req->radius));

    ncp_host_hl_buf_put_u8(&body, req->addr_mode);
    TRACE_MSG(TRACE_TRANSPORT2, "req->addr_mode 0x%hx", (FMT__H, req->addr_mode));

    ncp_host_hl_buf_put_u8(&body, req->tx_options);
    TRACE_MSG(TRACE_TRANSPORT2, "req->tx_options %hd", (FMT__H, req->tx_options));

    ncp_host_hl_buf_put_u8(&body, req->use_alias);
    TRACE_MSG(TRACE_TRANSPORT2, "req->use_alias %hd", (FMT__H, req->use_alias));

    ncp_host_hl_buf_put_u16(&body, req->alias_src_addr);
    TRACE_MSG(TRACE_TRANSPORT2, "req->alias_src_addr %d", (FMT__D, req->alias_src_addr));

    ncp_host_hl_buf_put_u8(&body, req->alias_seq_num);
    TRACE_MSG(TRACE_TRANSPORT2, "req->alias_seq_num %hd", (FMT__H, req->alias_seq_num));

    ncp_host_hl_buf_put_array(&body, data_ptr, data_len);

    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_apsde_data_request_transport, ret %d", (FMT__D, ret));

  return ret;
}
