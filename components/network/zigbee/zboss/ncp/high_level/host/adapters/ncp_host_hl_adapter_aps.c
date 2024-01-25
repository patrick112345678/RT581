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
/*  PURPOSE: NCP High level transport (adapters layer) implementation for the host side: APS category
*/
#define ZB_TRACE_FILE_ID 17509

#include "zb_common.h"
#include "ncp_host_hl_transport_internal_api.h"
#include "ncp_host_soc_state.h"
#include "zb_zdo.h"

#define APS_REQ_BUF_TABLE_SIZE 20

/* Buffers accordance table entry for APS Requests and Confirms */
typedef struct aps_req_buf_table_entry_s
{
  zb_bufid_t buf;
  zb_uint8_t tsn;
} aps_req_buf_table_entry_t;

/* Context for keeping buffers for APS confirms after sending requests. Must use
 * the same buffer for APS request and its confirm. */
typedef struct host_aps_adapter_ctx_s
{
  /* Buffers accordance table for APS Data Request and Confirms */
  aps_req_buf_table_entry_t aps_req_buf_table[APS_REQ_BUF_TABLE_SIZE];
  zb_device_handler_t af_data_cb;
} aps_adapter_ctx_t;

static aps_adapter_ctx_t g_adapter_ctx;

void ncp_host_aps_adapter_init_ctx(void)
{
  zb_uindex_t entry_index = 0;

  ZB_BZERO(&g_adapter_ctx, sizeof(g_adapter_ctx));

  for (entry_index = 0; entry_index < APS_REQ_BUF_TABLE_SIZE; entry_index++)
  {
    g_adapter_ctx.aps_req_buf_table[entry_index].tsn = NCP_TSN_RESERVED;
  }
}


static zb_ret_t find_free_aps_buf_table_entry(aps_req_buf_table_entry_t **buf)
{
  zb_ret_t ret = RET_NO_MEMORY;
  zb_uindex_t entry_index;

  *buf = NULL;
  for (entry_index = 0; entry_index < APS_REQ_BUF_TABLE_SIZE; entry_index++)
  {
    if (g_adapter_ctx.aps_req_buf_table[entry_index].buf == 0)
    {
      *buf = &g_adapter_ctx.aps_req_buf_table[entry_index];
      ret = RET_OK;
      break;
    }
  }

  ZB_ASSERT(ret == RET_OK);
  return ret;
}


static aps_req_buf_table_entry_t* find_aps_buf_table_entry_by_tsn(zb_uint8_t tsn)
{
  zb_uindex_t entry_index;

  for (entry_index = 0; entry_index < APS_REQ_BUF_TABLE_SIZE; entry_index++)
  {
    if (g_adapter_ctx.aps_req_buf_table[entry_index].tsn == tsn)
    {
      return &g_adapter_ctx.aps_req_buf_table[entry_index];
    }
  }

  return NULL;
}


static void free_aps_buf_table_entry(aps_req_buf_table_entry_t **buf)
{
  if (*buf)
  {
    ZB_BZERO(*buf, sizeof(**buf));
    (*buf)->tsn = NCP_TSN_RESERVED;
    *buf = NULL;
  }
}

void ncp_host_handle_apsde_data_indication(zb_apsde_data_indication_t *ind,
                                           zb_uint8_t *data_ptr,
                                           zb_uint8_t params_len, zb_uint16_t data_len)
{
  zb_bufid_t buf = zb_buf_get(ZB_TRUE, 0);
  zb_uint8_t *ptr = NULL;
  zb_bool_t packet_processed = ZB_FALSE;

  ZVUNUSED(params_len);

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_apsde_data_indication ", (FMT__0));

  ZB_ASSERT(buf != ZB_BUF_INVALID);

  ptr = (zb_uint8_t *)zb_buf_initial_alloc(buf, data_len);
  ZB_MEMCPY(ptr, data_ptr, data_len);

  ZB_MEMCPY(ZB_BUF_GET_PARAM(buf, zb_apsde_data_indication_t), ind, sizeof(zb_apsde_data_indication_t));

  if (g_adapter_ctx.af_data_cb != NULL)
  {
    packet_processed = (zb_bool_t)(g_adapter_ctx.af_data_cb(buf));
  }

#if defined ZB_ENABLE_ZCL
  if (!packet_processed)
  {
    packet_processed = zb_af_handle_zcl_frame(buf);
  }
#endif

  if (!packet_processed)
  {
    TRACE_MSG(TRACE_ZDO1, "can't handle APS packet %d, drop it", (FMT__D, buf));
    zb_buf_free(buf);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_apsde_data_indication", (FMT__0));
}


static void apsde_data_acknowledged(zb_uint8_t param)
{
  TRACE_MSG(TRACE_TRANSPORT3, ">> apsde_data_acknowledged: param %hd status: %d", (FMT__H_D, param, zb_buf_get_status(param)));

#ifdef ZB_ENABLE_ZCL
  if (zb_af_is_confirm_for_zcl_frame(param))
  {
    zb_af_handle_zcl_frame_data_confirm(param);
  }
  else
#endif
  {
    TRACE_MSG(TRACE_ERROR, "can't handle response for APS packet %d, drop it",
              (FMT__D, param));
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< apsde_data_acknowledged ", (FMT__0));
}

void ncp_host_handle_apsde_data_response(zb_apsde_data_confirm_t *conf, zb_uint8_t tsn)
{
  aps_req_buf_table_entry_t* buf_table_entry = find_aps_buf_table_entry_by_tsn(tsn);
  zb_bufid_t buf;

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_apsde_data_response, tsn %hd", (FMT__H, tsn));

  ZB_ASSERT(buf_table_entry != NULL);

  buf = buf_table_entry->buf;
  free_aps_buf_table_entry(&buf_table_entry);

  ZB_ASSERT(buf != ZB_BUF_INVALID);
  ZB_MEMCPY(ZB_BUF_GET_PARAM(buf, zb_apsde_data_confirm_t), conf, sizeof(zb_apsde_data_confirm_t));

  zb_buf_set_status(buf, conf->status);

  TRACE_MSG(TRACE_TRANSPORT3, "  addr_mode %hd", (FMT__H, conf->addr_mode));
  TRACE_MSG(TRACE_TRANSPORT3, "  src_endpoint %hd", (FMT__H, conf->src_endpoint));
  TRACE_MSG(TRACE_TRANSPORT3, "  dst_endpoint %hd", (FMT__H, conf->dst_endpoint));

  apsde_data_acknowledged(buf);

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_apsde_data_response ", (FMT__0));
}


static void handle_bind_unbind_confirm_int(zb_ret_t status_code, zb_bufid_t buf)
{
  zb_apsme_binding_req_t *req_param = ZB_BUF_GET_PARAM(buf, zb_apsme_binding_req_t);
  ZB_ASSERT(req_param->confirm_cb);

  zb_buf_set_status(buf, status_code);

  if (req_param->confirm_cb)
  {
    ZB_SCHEDULE_CALLBACK(req_param->confirm_cb, buf);
  }
  else
  {
    zb_buf_free(buf);
  }
}

static void handle_bind_unbind_confirm(zb_ret_t status_code, zb_uint8_t tsn, zb_bool_t bind)
{
  aps_req_buf_table_entry_t* buf_table_entry = find_aps_buf_table_entry_by_tsn(tsn);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_bind_unbind_confirm, status_code %d, bind %d",
            (FMT__D_D, status_code, bind));

  if (buf_table_entry)
  {
    handle_bind_unbind_confirm_int(status_code, buf_table_entry->buf);
    free_aps_buf_table_entry(&buf_table_entry);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_bind_unbind_confirm", (FMT__0));
}


void ncp_host_handle_apsme_bind_response(zb_ret_t status_code, zb_uint8_t tsn)
{
  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_apsme_bind_response", (FMT__0));

  handle_bind_unbind_confirm(status_code, tsn, ZB_TRUE);

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_apsme_bind_response", (FMT__0));
}


void ncp_host_handle_apsme_unbind_response(zb_ret_t status_code, zb_uint8_t tsn)
{
  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_apsme_unbind_response", (FMT__0));

  handle_bind_unbind_confirm(status_code, tsn, ZB_FALSE);

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_apsme_unbind_response", (FMT__0));
}


void ncp_host_handle_apsme_unbind_all_response(zb_ret_t status_code)
{
  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_apsme_unbind_all_response, status_code %d",
    (FMT__D, status_code));

  ZB_ASSERT(status_code == RET_OK);

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_apsme_unbind_all_response", (FMT__0));
}


static void handle_apsme_add_group_response(zb_ret_t status_code,
                                            zb_bufid_t buf)
{
  zb_apsme_add_group_req_t req_param;
  zb_apsme_add_group_conf_t *conf_param;

  ZB_MEMCPY(&req_param,
            ZB_BUF_GET_PARAM(buf, zb_apsme_add_group_req_t),
            sizeof(zb_apsme_add_group_req_t));

  if (status_code == RET_OK)
  {
    zb_aps_group_table_add(ncp_host_state_get_group_table(),
                           req_param.group_address,
                           req_param.endpoint);
  }

  if (req_param.confirm_cb)
  {
    conf_param = ZB_BUF_GET_PARAM(buf, zb_apsme_add_group_conf_t);
    conf_param->endpoint = req_param.endpoint;
    conf_param->group_address = req_param.group_address;
    conf_param->status = status_code;

    zb_buf_set_status(buf, status_code);
    ZB_SCHEDULE_CALLBACK(req_param.confirm_cb, buf);
  }
  else
  {
    zb_buf_free(buf);
  }
}


void ncp_host_handle_apsme_add_group_response(zb_ret_t status_code, zb_uint8_t tsn)
{
  aps_req_buf_table_entry_t* buf_table_entry = find_aps_buf_table_entry_by_tsn(tsn);

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_apsme_add_group_response, status_code %d",
    (FMT__D, status_code));

  if (buf_table_entry)
  {
    handle_apsme_add_group_response(status_code, buf_table_entry->buf);
    free_aps_buf_table_entry(&buf_table_entry);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_apsme_add_group_response", (FMT__0));
}


static void handle_apsme_remove_group_response(zb_ret_t status_code, zb_bufid_t buf)
{
  zb_apsme_remove_group_req_t req_param;
  zb_apsme_remove_group_conf_t *conf_param;

  ZB_MEMCPY(&req_param,
            ZB_BUF_GET_PARAM(buf, zb_apsme_remove_group_req_t),
            sizeof(zb_apsme_remove_group_req_t));

  if (status_code == RET_OK)
  {
    zb_aps_group_table_remove(ncp_host_state_get_group_table(),
                              req_param.group_address,
                              req_param.endpoint);
  }

  if (req_param.confirm_cb)
  {
    conf_param = ZB_BUF_GET_PARAM(buf, zb_apsme_remove_group_conf_t);
    conf_param->endpoint = req_param.endpoint;
    conf_param->group_address = req_param.group_address;
    conf_param->status = status_code;

    zb_buf_set_status(buf, status_code);
    ZB_SCHEDULE_CALLBACK(req_param.confirm_cb, buf);
  }
  else
  {
    zb_buf_free(buf);
  }
}


void ncp_host_handle_apsme_remove_group_response(zb_ret_t status_code, zb_uint8_t tsn)
{
  aps_req_buf_table_entry_t* buf_table_entry = find_aps_buf_table_entry_by_tsn(tsn);

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_apsme_remove_group_response, status_code %d",
    (FMT__D, status_code));

  if (buf_table_entry)
  {
    handle_apsme_remove_group_response(status_code, buf_table_entry->buf);
    free_aps_buf_table_entry(&buf_table_entry);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_apsme_remove_group_response", (FMT__0));
}


static void handle_apsme_remove_all_groups_response(zb_ret_t status_code, zb_bufid_t buf)
{
  zb_apsme_remove_all_groups_req_t req_param;
  zb_apsme_remove_all_groups_conf_t *conf_param;

  ZB_MEMCPY(&req_param,
            ZB_BUF_GET_PARAM(buf, zb_apsme_remove_all_groups_req_t),
            sizeof(zb_apsme_remove_all_groups_req_t));

  if (status_code == RET_OK)
  {
    zb_aps_group_table_remove_all(ncp_host_state_get_group_table());
  }

  if (req_param.confirm_cb)
  {
    conf_param = ZB_BUF_GET_PARAM(buf, zb_apsme_remove_all_groups_conf_t);
    conf_param->endpoint = req_param.endpoint;
    conf_param->status = status_code;

    zb_buf_set_status(buf, status_code);
    ZB_SCHEDULE_CALLBACK(req_param.confirm_cb, buf);
  }
  else
  {
    zb_buf_free(buf);
  }
}


void ncp_host_handle_apsme_remove_all_groups_response(zb_ret_t status_code, zb_uint8_t tsn)
{
  aps_req_buf_table_entry_t* buf_table_entry = find_aps_buf_table_entry_by_tsn(tsn);

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_apsme_remove_all_groups_response, status_code %d",
    (FMT__D, status_code));

  if (buf_table_entry)
  {
    handle_apsme_remove_all_groups_response(status_code, buf_table_entry->buf);
    free_aps_buf_table_entry(&buf_table_entry);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_apsme_remove_all_groups_response", (FMT__0));
}


void ncp_host_handle_aps_check_binding_response(zb_ret_t status_code,
                                                zb_bool_t exists,
                                                zb_uint8_t tsn)
{
  aps_req_buf_table_entry_t* buf_table_entry = find_aps_buf_table_entry_by_tsn(tsn);

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_check_binding_response, status_code %d, exists %d",
            (FMT__D_D, status_code, exists));

  ZB_ASSERT(status_code == RET_OK);

  if (buf_table_entry)
  {
    zb_aps_check_binding_req_t req;
    zb_aps_check_binding_resp_t *resp_ptr = NULL;

    ZB_MEMCPY(&req, ZB_BUF_GET_PARAM(buf_table_entry->buf, zb_aps_check_binding_req_t), sizeof(req));

    if (req.response_cb)
    {
      resp_ptr = ZB_BUF_GET_PARAM(buf_table_entry->buf, zb_aps_check_binding_resp_t);
      resp_ptr->src_endpoint = req.src_endpoint;
      resp_ptr->cluster_id = req.cluster_id;
      resp_ptr->exists = exists;

      ZB_SCHEDULE_CALLBACK(req.response_cb, buf_table_entry->buf);
    }
    else
    {
      zb_buf_free(buf_table_entry->buf);
    }

    free_aps_buf_table_entry(&buf_table_entry);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "got a response, but no info about a request, skip", (FMT__0));
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_check_binding_response", (FMT__0));
}

static void aps_fail_confirm(zb_uint8_t param, zb_ret_t status)
{
  zb_apsde_data_req_t *apsreq = ZB_BUF_GET_PARAM(param, zb_apsde_data_req_t);
  zb_apsde_data_confirm_t apsde_data_conf;

  TRACE_MSG(TRACE_TRANSPORT3, ">> aps_fail_confirm: param %hd status: %d", (FMT__H_D, param, status));

  ZB_BZERO(&apsde_data_conf, sizeof(zb_apsde_data_confirm_t));
  apsde_data_conf.addr_mode = apsreq->addr_mode;
  apsde_data_conf.src_endpoint = apsreq->src_endpoint;
  apsde_data_conf.dst_endpoint = apsreq->dst_endpoint;
  apsde_data_conf.dst_addr = apsreq->dst_addr;
  apsde_data_conf.status = status;
  zb_buf_set_status(param, status);
  /* Pkt was not passed through apsde_data_req. */
  apsde_data_conf.need_unlock = ZB_FALSE;

  ZB_MEMCPY(ZB_BUF_GET_PARAM(param, zb_apsde_data_confirm_t), &apsde_data_conf, sizeof(zb_apsde_data_confirm_t));
  apsde_data_acknowledged(param);

  TRACE_MSG(TRACE_TRANSPORT3, "<< aps_fail_confirm ", (FMT__0));
}

void zb_apsde_data_request(zb_bufid_t buf)
{
  ncp_host_zb_apsde_data_req_t ncp_req;
  zb_apsde_data_req_t *req = ZB_BUF_GET_PARAM(buf, zb_apsde_data_req_t);
  zb_uint8_t *data_ptr = NULL;
  aps_req_buf_table_entry_t* buf_table_entry = NULL;
  zb_uint8_t tsn = 0;
  zb_ret_t ret = RET_BUSY;

  TRACE_MSG(TRACE_TRANSPORT2, ">> zb_apsde_data_request", (FMT__0));

  ZB_ASSERT(buf);

  /* Find table entry to save current buffer ID and TSN */
  ret = find_free_aps_buf_table_entry(&buf_table_entry);

  if (ret == RET_OK)
  {
    ZB_MEMCPY(&ncp_req.dst_addr, &req->dst_addr, sizeof(zb_addr_u));

    ncp_req.profileid = req->profileid;
    ncp_req.clusterid = req->clusterid;
    ncp_req.dst_endpoint = req->dst_endpoint;
    ncp_req.src_endpoint = req->src_endpoint;
    ncp_req.radius = req->radius;
    ncp_req.addr_mode = req->addr_mode;
    ncp_req.tx_options = req->tx_options;
    ncp_req.use_alias = req->use_alias;
    ncp_req.alias_src_addr = req->alias_src_addr;
    ncp_req.alias_seq_num = req->alias_seq_num;

    data_ptr = (zb_uint8_t *)zb_buf_begin(buf);

    ret = ncp_host_apsde_data_request(&ncp_req, sizeof(ncp_host_zb_apsde_data_req_t), zb_buf_len(buf), data_ptr, &tsn);

    if (ret != RET_OK)
    {
      TRACE_MSG(TRACE_TRANSPORT2, "ncp_host_apsde_data_request failed with status %d", (FMT__D, ret));

      free_aps_buf_table_entry(&buf_table_entry);

      aps_fail_confirm(buf, ret);
    }
    else
    {
      /* 07/30/2020 EE CR:MINOR a) Why do we need buf-tsn array in 2 places: here and in hl_proto?
       b) Am I right buf is always identified by tsn? Is so, it is enough to have zb_bufid_t[256] and index it by tsn.
       c) Buffer header has a 'handle' field. Can't it be useful there? */
      /* DD: we have other buffers, not ZBOSS ones, in hl_proto.
         Agree with b) - we can use such approach and it is beneficial speed-wise.
         TODO: implement it.
      */
      buf_table_entry->buf = buf;
      buf_table_entry->tsn = tsn;
    }
  }

  TRACE_MSG(TRACE_TRANSPORT2, "data request command is processed, ret %d, tsn %hd",
    (FMT__D_H, ret, tsn));

  TRACE_MSG(TRACE_TRANSPORT2, "<< zb_apsde_data_request", (FMT__0));
}


static void zb_apsme_bind_unbind_request(zb_bufid_t buf, zb_bool_t bind)
{
  zb_apsme_binding_req_t *req_param = ZB_BUF_GET_PARAM(buf, zb_apsme_binding_req_t);
  aps_req_buf_table_entry_t* buf_table_entry = NULL;
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_TRANSPORT2, ">> zb_apsme_bind_unbind_request, buf %d, bind %d",
            (FMT__D_D, buf, bind));

  ZB_ASSERT(buf != ZB_BUF_INVALID);

  if (req_param->confirm_cb)
  {
    ret = find_free_aps_buf_table_entry(&buf_table_entry);
  }

  if (ret == RET_OK)
  {
    zb_uint8_t tsn;

    ret = ncp_host_apsme_bind_unbind_request(req_param->src_addr, req_param->src_endpoint,
                                             req_param->clusterid, req_param->addr_mode,
                                             req_param->dst_addr, req_param->dst_endpoint,
                                             bind, &tsn);

    ZB_ASSERT(ret == RET_OK);

    if (buf_table_entry)
    {
      buf_table_entry->buf = buf;
      buf_table_entry->tsn = tsn;
    }
  }

  if (ret != RET_OK)
  {
    handle_bind_unbind_confirm_int(ret, buf);
    free_aps_buf_table_entry(&buf_table_entry);
  }
  else if (!buf_table_entry)
  {
    zb_buf_free(buf);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< zb_apsme_bind_unbind_request", (FMT__0));
}


void zb_apsme_bind_request(zb_bufid_t buf)
{
  zb_apsme_bind_unbind_request(buf, ZB_TRUE);
}


void zb_apsme_unbind_request(zb_bufid_t buf)
{
  zb_apsme_bind_unbind_request(buf, ZB_FALSE);
}


void zb_apsme_unbind_all(zb_uint8_t param)
{
  zb_ret_t ret = RET_BUSY;

  ZVUNUSED(param);

  TRACE_MSG(TRACE_TRANSPORT2, ">> zb_apsme_unbind_all", (FMT__0));

  ret = ncp_host_apsme_unbind_all_request();
  ZB_ASSERT(ret == RET_OK);

  TRACE_MSG(TRACE_TRANSPORT2, "<< zb_apsme_unbind_all, ret %d", (FMT__D, ret));
}


void zb_af_set_data_indication(zb_device_handler_t cb)
{
  g_adapter_ctx.af_data_cb = cb;
}


void zb_apsme_add_group_request(zb_bufid_t buf)
{
  zb_apsme_add_group_req_t *req_param = ZB_BUF_GET_PARAM(buf, zb_apsme_add_group_req_t);
  aps_req_buf_table_entry_t* buf_table_entry = NULL;
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_TRANSPORT2, ">> zb_apsme_add_group_request, buf %d", (FMT__D, buf));

  ZB_ASSERT(buf != ZB_BUF_INVALID);

  /* we always need to store a buf with request info to update the group table
     when a confirm is received */
  ret = find_free_aps_buf_table_entry(&buf_table_entry);

  if (ret == RET_OK)
  {
    zb_uint8_t tsn;
    ret = ncp_host_apsme_add_group_request(req_param, &tsn);

    ZB_ASSERT(ret == RET_OK);

    buf_table_entry->buf = buf;
    buf_table_entry->tsn = tsn;
  }

  if (ret != RET_OK)
  {
    handle_apsme_add_group_response(ret, buf);
    free_aps_buf_table_entry(&buf_table_entry);
  }
  else if (!buf_table_entry)
  {
    zb_buf_free(buf);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< zb_apsme_add_group_request", (FMT__0));
}


void zb_apsme_remove_group_request(zb_bufid_t buf)
{
  zb_apsme_remove_group_req_t *req_param = ZB_BUF_GET_PARAM(buf, zb_apsme_remove_group_req_t);
  aps_req_buf_table_entry_t* buf_table_entry = NULL;
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_TRANSPORT2, ">> zb_apsme_remove_group_request, buf %d", (FMT__D, buf));

  ZB_ASSERT(buf != ZB_BUF_INVALID);

  /* we always need to store a buf with request info to update the group table
     when a confirm is received */
  ret = find_free_aps_buf_table_entry(&buf_table_entry);

  if (ret == RET_OK)
  {
    zb_uint8_t tsn;
    ret = ncp_host_apsme_remove_group_request(req_param, &tsn);

    ZB_ASSERT(ret == RET_OK);

    buf_table_entry->buf = buf;
    buf_table_entry->tsn = tsn;
  }

  if (ret != RET_OK)
  {
    handle_apsme_remove_group_response(ret, buf);
    free_aps_buf_table_entry(&buf_table_entry);
  }
  else if (!buf_table_entry)
  {
    zb_buf_free(buf);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< zb_apsme_remove_group_request", (FMT__0));
}


void zb_apsme_remove_all_groups_request(zb_bufid_t buf)
{
  zb_apsme_remove_all_groups_req_t *req_param = ZB_BUF_GET_PARAM(buf, zb_apsme_remove_all_groups_req_t);
  aps_req_buf_table_entry_t* buf_table_entry = NULL;
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_TRANSPORT2, ">> zb_apsme_remove_all_groups_request, buf %d", (FMT__D, buf));

  ZB_ASSERT(buf != ZB_BUF_INVALID);

  /* we always need to store a buf with request info to update the group table
     when a confirm is received */
  ret = find_free_aps_buf_table_entry(&buf_table_entry);

  if (ret == RET_OK)
  {
    zb_uint8_t tsn;
    ret = ncp_host_apsme_remove_all_groups_request(req_param, &tsn);

    ZB_ASSERT(ret == RET_OK);

    buf_table_entry->buf = buf;
    buf_table_entry->tsn = tsn;
  }

  if (ret != RET_OK)
  {
    handle_apsme_remove_all_groups_response(ret, buf);
    free_aps_buf_table_entry(&buf_table_entry);
  }
  else if (!buf_table_entry)
  {
    zb_buf_free(buf);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< zb_apsme_remove_all_groups_request", (FMT__0));
}


void zb_aps_check_binding_request(zb_bufid_t buf)
{
  zb_aps_check_binding_req_t *req_param = ZB_BUF_GET_PARAM(buf, zb_aps_check_binding_req_t);
  aps_req_buf_table_entry_t* buf_table_entry = NULL;
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_TRANSPORT2, ">> zb_aps_check_binding_request, buf %d", (FMT__D, buf));

  ZB_ASSERT(buf != ZB_BUF_INVALID);

  ZB_ASSERT(req_param->response_cb);
  if (!req_param->response_cb)
  {
    TRACE_MSG(TRACE_ERROR, "response_cb is not specified, skip this request", (FMT__0));
    ret = RET_ERROR;
  }
  else
  {
    ret = find_free_aps_buf_table_entry(&buf_table_entry);
  }

  if (ret == RET_OK)
  {
    zb_uint8_t tsn;
    ret = ncp_host_aps_check_binding_request(req_param, &tsn);

    ZB_ASSERT(ret == RET_OK);

    if (ret == RET_OK)
    {
      buf_table_entry->buf = buf;
      buf_table_entry->tsn = tsn;
    }
  }

  if (ret != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "failed to send zb_aps_check_binding_request, ret %d", (FMT__D, ret));

    /* NOTE: callback is never called in this case.
       There is no point in setting buffer status and call a callback
       because upper layers never check a buffer status. */
    zb_buf_free(buf);

    if (buf_table_entry)
    {
      free_aps_buf_table_entry(&buf_table_entry);
    }
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< zb_aps_check_binding_request", (FMT__0));
}


zb_bool_t zb_aps_is_endpoint_in_group(zb_uint16_t group_id, zb_uint8_t endpoint)
{
  return zb_aps_group_table_is_endpoint_in_group(ncp_host_state_get_group_table(),
                                                 group_id,
                                                 endpoint);
}


void zb_apsme_get_group_membership_request(zb_bufid_t buf)
{
  TRACE_MSG(TRACE_TRANSPORT2, ">> zb_apsme_get_group_membership_request, buf %d", (FMT__D, buf));

  zb_apsme_internal_get_group_membership_request(ncp_host_state_get_group_table(), buf);

  TRACE_MSG(TRACE_TRANSPORT2, "<< zb_apsme_get_group_membership_request", (FMT__0));
}
