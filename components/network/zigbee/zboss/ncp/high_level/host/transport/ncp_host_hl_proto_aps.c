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
/*  PURPOSE: NCP High level transport implementation for the host side: APS category
*/
#define ZB_TRACE_FILE_ID 17512

#include "ncp_host_hl_proto.h"

static void handle_apsde_data_indication(ncp_hl_ind_header_t* indication, zb_uint16_t len)
{
  ncp_host_hl_rx_buf_handle_t body;

  zb_uint8_t params_len;
  zb_uint16_t data_len;
  zb_apsde_data_indication_t ind;
  zb_uint8_t *data_ptr;

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
  ncp_host_hl_buf_get_u8(&body, (zb_uint8_t*)&ind.rssi);
  ncp_host_hl_buf_get_u8(&body, ((zb_uint8_t*)&ind.rssi + 1U)); /* APS key source & attr*/

  TRACE_MSG(TRACE_TRANSPORT3, "src_addr 0x%x dst_addr 0x%x",
            (FMT__D_D, ind.src_addr, ind.dst_addr));

  ncp_host_hl_buf_get_ptr(&body, &data_ptr, data_len);

  TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_apsde_data_indication", (FMT__0));

  ncp_host_handle_apsde_data_indication(&ind, data_ptr, params_len, data_len);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_apsde_data_indication", (FMT__0));
}


static void handle_apsde_data_response(ncp_hl_response_header_t* response,
                                               zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  zb_apsde_data_confirm_t conf;
  zb_ieee_addr_t temp;
  ncp_host_hl_rx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_apsde_data_response, status_code %d",
            (FMT__D, error_code));

  TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_apsde_data_response", (FMT__0));

  ncp_host_hl_init_response_body(response, len, &body);

  /* Write received address to the temp long addr in any case*/
  ncp_host_hl_buf_get_u64addr(&body, temp);
  TRACE_MSG(TRACE_TRANSPORT3, "temp address " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(temp)));

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
    ZB_LETOH16_ONPLACE(conf.dst_addr.addr_short);
    TRACE_MSG(TRACE_TRANSPORT3, "conf.dst_addr.addr_short 0x%x", (FMT__D, conf.dst_addr.addr_short));
  }
  else
  {
    ZB_MEMCPY(&conf.dst_addr.addr_long, temp, sizeof(zb_ieee_addr_t));
    TRACE_MSG(TRACE_TRANSPORT3, "conf.dst_addr.addr_long " TRACE_FORMAT_64,
              (FMT__A, TRACE_ARG_64(conf.dst_addr.addr_long)));
  }

  conf.status = error_code;
  conf.need_unlock = ZB_FALSE;

  TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_apsde_data_response", (FMT__D));

  ncp_host_handle_apsde_data_response(&conf, response->tsn);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_apsde_data_response", (FMT__0));
}

static void handle_apsme_bind_unbind_response(ncp_hl_response_header_t* response,
                                              zb_uint16_t len, zb_bool_t is_bind)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  ZVUNUSED(len);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_apsme_bind_response", (FMT__0));

  ncp_host_mark_blocking_request_finished();

  if (is_bind)
  {
    ncp_host_handle_apsme_bind_response(error_code, response->tsn);
  }
  else
  {
    ncp_host_handle_apsme_unbind_response(error_code, response->tsn);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_apsme_bind_response", (FMT__0));
}


static void handle_apsme_unbind_all_response(ncp_hl_response_header_t* response, zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  ZVUNUSED(len);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_apsme_unbind_all_response", (FMT__0));

  ncp_host_mark_blocking_request_finished();

  ncp_host_handle_apsme_unbind_all_response(error_code);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_apsme_unbind_all_response", (FMT__0));
}


static void handle_apsme_add_group_response(ncp_hl_response_header_t* response, zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  ZVUNUSED(len);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_apsme_bind_response", (FMT__0));

  ncp_host_handle_apsme_add_group_response(error_code, response->tsn);

  ncp_host_mark_blocking_request_finished();

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_apsme_bind_response", (FMT__0));
}


static void handle_apsme_remove_group_response(ncp_hl_response_header_t* response, zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  ZVUNUSED(len);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_apsme_remove_group_response", (FMT__0));

  ncp_host_mark_blocking_request_finished();

  ncp_host_handle_apsme_remove_group_response(error_code, response->tsn);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_apsme_remove_group_response", (FMT__0));
}


static void handle_apsme_remove_all_groups_response(ncp_hl_response_header_t* response, zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  ZVUNUSED(len);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_apsme_remove_all_groups_response", (FMT__0));

  ncp_host_mark_blocking_request_finished();

  ncp_host_handle_apsme_remove_all_groups_response(error_code, response->tsn);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_apsme_remove_all_groups_response", (FMT__0));
}


static void handle_aps_check_binding_response(ncp_hl_response_header_t* response, zb_uint16_t len)
{
  zb_uint8_t exists = 0;
  ncp_host_hl_rx_buf_handle_t body;
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  ZVUNUSED(len);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_aps_check_binding_response", (FMT__0));

  ncp_host_mark_blocking_request_finished();

  if (error_code == RET_OK)
  {
    ncp_host_hl_init_response_body(response, len, &body);

    ncp_host_hl_buf_get_u8(&body, &exists);
  }

  ncp_host_handle_aps_check_binding_response(error_code, (zb_bool_t)exists, response->tsn);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_aps_check_binding_response", (FMT__0));
}




static void handle_aps_get_group_table_response(ncp_hl_response_header_t* response, zb_uint16_t len)
{
  ncp_host_hl_rx_buf_handle_t body;
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  ZVUNUSED(len);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_aps_get_group_table_response, status 0x%x",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  if (error_code == RET_OK)
  {
    zb_uint8_t group_cnt;
    zb_aps_group_table_ent_t *table_entry = NULL;

    ncp_host_hl_init_response_body(response, len, &body);

    ncp_host_hl_buf_get_u8(&body, &group_cnt);
    if (group_cnt)
    {
      ncp_host_hl_buf_get_ptr(&body, (zb_uint8_t**)&table_entry, sizeof(*table_entry) * group_cnt);
    }
    ncp_host_handle_aps_get_group_table_response(error_code, table_entry, group_cnt);
  }
  else
  {
    ncp_host_handle_aps_get_group_table_response(error_code, NULL, 0);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_aps_get_group_table_response", (FMT__0));
}


void ncp_host_handle_aps_response(void* data, zb_uint16_t len)
{
  ncp_hl_response_header_t* response_header = (ncp_hl_response_header_t*)data;

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_aps_response", (FMT__0));

  switch(response_header->hdr.call_id)
  {
    case NCP_HL_APSDE_DATA_REQ:
      handle_apsde_data_response(response_header, len);
      break;

    case NCP_HL_APSME_BIND:
      handle_apsme_bind_unbind_response(response_header, len, ZB_TRUE);
      break;

    case NCP_HL_APSME_UNBIND:
      handle_apsme_bind_unbind_response(response_header, len, ZB_FALSE);
      break;

    case NCP_HL_APSME_UNBIND_ALL:
      handle_apsme_unbind_all_response(response_header, len);
      break;

    case NCP_HL_APSME_ADD_GROUP:
      handle_apsme_add_group_response(response_header, len);
      break;

    case NCP_HL_APSME_RM_GROUP:
      handle_apsme_remove_group_response(response_header, len);
      break;

    case NCP_HL_APSME_RM_ALL_GROUPS:
      handle_apsme_remove_all_groups_response(response_header, len);
      break;

    case NCP_HL_APS_CHECK_BINDING:
      handle_aps_check_binding_response(response_header, len);
      break;

    case NCP_HL_APS_GET_GROUP_TABLE:
      handle_aps_get_group_table_response(response_header, len);
      break;

    default:
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
      ZB_ASSERT(0);
    }

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_aps_indication", (FMT__0));
}

zb_ret_t ncp_host_apsde_data_request(ncp_host_zb_apsde_data_req_t *req, zb_uint8_t param_len,
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

    if (req->addr_mode == ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT || req->addr_mode == ZB_APS_ADDR_MODE_16_ENDP_PRESENT)
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

zb_ret_t ncp_host_apsme_bind_unbind_request(zb_ieee_addr_t src_addr, zb_uint8_t src_endpoint,
                                            zb_uint16_t cluster_id, zb_uint8_t addr_mode,
                                            zb_addr_u dst_addr, zb_uint8_t dst_endpoint,
                                            zb_bool_t is_bind, zb_uint8_t *tsn)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;
  zb_uint16_t call_id = (is_bind) ? NCP_HL_APSME_BIND : NCP_HL_APSME_UNBIND;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_apsme_bind_unbind_request", (FMT__0));

  ZB_ASSERT(tsn != NULL);

  if (!ncp_host_get_buf_for_blocking_request(call_id, &body, tsn))
  {
    ncp_host_hl_buf_put_u64addr(&body, src_addr);
    TRACE_MSG(TRACE_TRANSPORT2, "src_addr " TRACE_FORMAT_64,
              (FMT__A, TRACE_ARG_64(src_addr)));

    ncp_host_hl_buf_put_u8(&body, src_endpoint);
    TRACE_MSG(TRACE_TRANSPORT2, "src_endpoint %hd", (FMT__H, src_endpoint));

    ncp_host_hl_buf_put_u16(&body, cluster_id);
    TRACE_MSG(TRACE_TRANSPORT2, "cluster_id %d", (FMT__D, cluster_id));

    ncp_host_hl_buf_put_u8(&body, addr_mode);
    TRACE_MSG(TRACE_TRANSPORT2, "dst_addr_mode %hd", (FMT__H, addr_mode));

    if (addr_mode == ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT || addr_mode == ZB_APS_ADDR_MODE_16_ENDP_PRESENT)
    {
      ncp_host_hl_buf_put_u16(&body, dst_addr.addr_short);
      ncp_host_hl_buf_put_u16(&body, 0);
      ncp_host_hl_buf_put_u32(&body, 0);
      TRACE_MSG(TRACE_TRANSPORT2, "dst_addr.addr_short 0x%x", (FMT__A, dst_addr.addr_short));
    }
    else
    {
      ncp_host_hl_buf_put_u64addr(&body, dst_addr.addr_long);
      TRACE_MSG(TRACE_TRANSPORT2, "dst_addr.addr_long " TRACE_FORMAT_64,
                (FMT__A, TRACE_ARG_64(dst_addr.addr_long)));
    }

    ncp_host_hl_buf_put_u8(&body, dst_endpoint);
    TRACE_MSG(TRACE_TRANSPORT2, "dst_endpoint %hd", (FMT__H, dst_endpoint));

    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_apsme_bind_unbind_request, ret %d", (FMT__D, ret));

  return ret;
}


zb_ret_t ncp_host_apsme_unbind_all_request(void)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_apsme_unbind_all_request", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_APSME_UNBIND_ALL, &body, NULL))
  {
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_apsme_unbind_all_request, ret %d", (FMT__D, ret));

  return ret;
}


zb_ret_t ncp_host_apsme_add_group_request(zb_apsme_add_group_req_t *req, zb_uint8_t *tsn)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_apsme_add_group_request", (FMT__0));

  ZB_ASSERT(tsn != NULL);

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_APSME_ADD_GROUP, &body, tsn))
  {
    ncp_host_hl_buf_put_u16(&body, req->group_address);
    TRACE_MSG(TRACE_TRANSPORT2, "group_address %d", (FMT__D, req->group_address));

    ncp_host_hl_buf_put_u8(&body, req->endpoint);
    TRACE_MSG(TRACE_TRANSPORT2, "endpoint %hd", (FMT__H, req->endpoint));

    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_apsme_add_group_request, ret %d", (FMT__D, ret));

  return ret;
}


zb_ret_t ncp_host_apsme_remove_group_request(zb_apsme_remove_group_req_t *req, zb_uint8_t *tsn)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_apsme_add_group_request", (FMT__0));

  ZB_ASSERT(tsn != NULL);

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_APSME_RM_GROUP, &body, tsn))
  {
    ncp_host_hl_buf_put_u16(&body, req->group_address);
    TRACE_MSG(TRACE_TRANSPORT2, "group_address %d", (FMT__D, req->group_address));

    ncp_host_hl_buf_put_u8(&body, req->endpoint);
    TRACE_MSG(TRACE_TRANSPORT2, "endpoint %hd", (FMT__H, req->endpoint));

    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_apsme_add_group_request, ret %d", (FMT__D, ret));

  return ret;
}


zb_ret_t ncp_host_apsme_remove_all_groups_request(zb_apsme_remove_all_groups_req_t *req, zb_uint8_t *tsn)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_apsme_remove_all_groups_request", (FMT__0));

  ZB_ASSERT(tsn != NULL);

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_APSME_RM_ALL_GROUPS, &body, tsn))
  {
    ncp_host_hl_buf_put_u8(&body, req->endpoint);
    TRACE_MSG(TRACE_TRANSPORT2, "endpoint %hd", (FMT__H, req->endpoint));

    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_apsme_remove_all_groups_request, ret %d", (FMT__D, ret));

  return ret;
}


zb_ret_t ncp_host_aps_check_binding_request(zb_aps_check_binding_req_t *req, zb_uint8_t *tsn)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_aps_check_binding_request", (FMT__0));

  ZB_ASSERT(tsn != NULL);

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_APS_CHECK_BINDING, &body, tsn))
  {
    TRACE_MSG(TRACE_TRANSPORT2, "src ep %hd, cluster id",
              (FMT__H_D, req->src_endpoint, req->cluster_id));
    ncp_host_hl_buf_put_u8(&body, req->src_endpoint);
    ncp_host_hl_buf_put_u16(&body, req->cluster_id);

    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_aps_check_binding_request, ret %d", (FMT__D, ret));

  return ret;
}


zb_ret_t ncp_host_aps_get_group_table_request(void)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;
  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_aps_get_group_table_request", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_APS_GET_GROUP_TABLE, &body, NULL))
  {
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_aps_get_group_table_request, ret %d", (FMT__D, ret));

  return ret;
}
