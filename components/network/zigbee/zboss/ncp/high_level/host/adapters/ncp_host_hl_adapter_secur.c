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
#define ZB_TRACE_FILE_ID 17513

#include "ncp_host_hl_transport_internal_api.h"
#include "ncp_host_soc_state.h"

/* TODO: consider buffers table refactoring and
 * moving to some common part with APS buffers table */
#define SECUR_REQ_BUF_TABLE_SIZE 5

/* Buffers accordance table entry for Secur Requests and Confirms */
typedef struct secur_req_buf_table_entry_s
{
  zb_bufid_t buf;
  zb_uint8_t tsn;
} secur_req_buf_table_entry_t;

/* Context for keeping buffers for Secur confirms after sending requests. Must use
 * the same buffer for Secur request and its confirm. */
typedef struct host_secur_adapter_ctx_s
{
  /* Buffers accordance table for Secur Data Request and Confirms */
  secur_req_buf_table_entry_t secur_req_buf_table[SECUR_REQ_BUF_TABLE_SIZE];
} secur_adapter_ctx_t;

typedef ZB_PACKED_PRE struct ncp_secur_ic_add_req_s
{
  zb_secur_ic_add_cb_t response_cb; /*!< Callback that will be called on response receiving */
}
ZB_PACKED_STRUCT ncp_secur_ic_add_req_t;

static secur_adapter_ctx_t g_adapter_ctx;

void ncp_host_secur_adapter_init_ctx(void)
{
  ZB_BZERO(&g_adapter_ctx, sizeof(g_adapter_ctx));
}


static zb_ret_t find_free_secur_buf_table_entry(secur_req_buf_table_entry_t **buf)
{
  zb_ret_t ret = RET_NO_MEMORY;
  zb_uindex_t entry_index;

  *buf = NULL;
  for (entry_index = 0; entry_index < SECUR_REQ_BUF_TABLE_SIZE; entry_index++)
  {
    if (g_adapter_ctx.secur_req_buf_table[entry_index].buf == 0)
    {
      *buf = &g_adapter_ctx.secur_req_buf_table[entry_index];
      ret = RET_OK;
      break;
    }
  }

  ZB_ASSERT(ret == RET_OK);
  return ret;
}


static secur_req_buf_table_entry_t* find_secur_buf_table_entry_by_tsn(zb_uint8_t tsn)
{
  zb_uindex_t entry_index;

  for (entry_index = 0; entry_index < SECUR_REQ_BUF_TABLE_SIZE; entry_index++)
  {
    if (g_adapter_ctx.secur_req_buf_table[entry_index].tsn == tsn)
    {
      return &g_adapter_ctx.secur_req_buf_table[entry_index];
    }
  }

  return NULL;
}


static void free_secur_buf_table_entry(secur_req_buf_table_entry_t **buf)
{
  if (*buf)
  {
    ZB_BZERO(*buf, sizeof(**buf));
    *buf = NULL;
  }
}


void zb_secur_ic_add(zb_ieee_addr_t address, zb_uint8_t ic_type, zb_uint8_t *ic, zb_secur_ic_add_cb_t cb)
{
  zb_ret_t ret = RET_OK;
  secur_req_buf_table_entry_t* buf_table_entry = NULL;
  zb_bufid_t buf = zb_buf_get(ZB_TRUE, 0);
  ncp_secur_ic_add_req_t *req_param;
  zb_uint8_t tsn;

  TRACE_MSG(TRACE_SECUR1, ">> zb_secur_ic_add " TRACE_FORMAT_128, (FMT__B, TRACE_ARG_128(ic)));

  ZB_ASSERT(buf != ZB_BUF_INVALID);

  req_param = ZB_BUF_GET_PARAM(buf, ncp_secur_ic_add_req_t);
  req_param->response_cb = cb;

  TRACE_MSG(TRACE_SECUR1, "   dev_ieee = ", (FMT__A, TRACE_ARG_64(address)));
  TRACE_MSG(TRACE_SECUR1, "   type = %hd", (FMT__H, ic_type));

  ret = find_free_secur_buf_table_entry(&buf_table_entry);

  if (ret == RET_OK)
  {
    /* NP: security layer is located on SoC, so install code validation will be performed there */
    ret = ncp_host_secur_add_ic(address, ic_type, ic, &tsn);
  }

  if (ret == RET_OK)
  {
    buf_table_entry->buf = buf;
    buf_table_entry->tsn = tsn;
  }

  if (ret != RET_OK)
  {
    if (cb != NULL)
    {
      cb(ret);
    }
    else
    {
      zb_buf_free(buf);
    }

    if (buf_table_entry)
    {
      free_secur_buf_table_entry(&buf_table_entry);
    }
  }

  ZB_ASSERT(ret == RET_OK);

  TRACE_MSG(TRACE_SECUR1, "<< zb_secur_ic_add, ret %d", (FMT__D, ret));
}


void zb_secur_ic_get_list_req(zb_uint8_t param)
{
  zb_secur_ic_get_list_req_t *req_param = ZB_BUF_GET_PARAM(param, zb_secur_ic_get_list_req_t);
  secur_req_buf_table_entry_t* buf_table_entry = NULL;
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_TRANSPORT2, ">> zb_secur_ic_get_list_req, buf %d", (FMT__D, param));

  ret = find_free_secur_buf_table_entry(&buf_table_entry);

  if (ret == RET_OK)
  {
    zb_uint8_t tsn;
    ret = ncp_host_secur_get_ic_list_request(req_param->start_index, &tsn);

    ZB_ASSERT(ret == RET_OK);

    if (ret == RET_OK)
    {
      buf_table_entry->buf = param;
      buf_table_entry->tsn = tsn;
    }
  }

  if (ret != RET_OK)
  {
    zb_callback_t response_cb = req_param->response_cb;
    zb_secur_ic_get_list_resp_t *resp_param = ZB_BUF_GET_PARAM(param, zb_secur_ic_get_list_resp_t);
    ZB_BZERO(resp_param, sizeof(*resp_param));

    resp_param->status = ret;

    TRACE_MSG(TRACE_ERROR, "failed to send zb_secur_ic_get_list_req, ret %d", (FMT__D, ret));

    if (response_cb != NULL)
    {
      ZB_SCHEDULE_CALLBACK(response_cb, param);
    }
    else
    {
      zb_buf_free(param);
    }

    if (buf_table_entry)
    {
      free_secur_buf_table_entry(&buf_table_entry);
    }
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< zb_secur_ic_get_list_req", (FMT__0));
}


void zb_secur_ic_get_by_idx_req(zb_uint8_t param)
{
  zb_secur_ic_get_by_idx_req_t *req_param = ZB_BUF_GET_PARAM(param, zb_secur_ic_get_by_idx_req_t);
  secur_req_buf_table_entry_t* buf_table_entry = NULL;
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_TRANSPORT2, ">> zb_secur_ic_get_by_idx_req, buf %d", (FMT__D, param));

  ret = find_free_secur_buf_table_entry(&buf_table_entry);

  if (ret == RET_OK)
  {
    zb_uint8_t tsn;
    ret = ncp_host_secur_get_ic_by_idx_request(req_param->ic_index, &tsn);

    ZB_ASSERT(ret == RET_OK);

    if (ret == RET_OK)
    {
      buf_table_entry->buf = param;
      buf_table_entry->tsn = tsn;
    }
  }

  if (ret != RET_OK)
  {
    zb_callback_t response_cb = req_param->response_cb;
    zb_secur_ic_get_by_idx_resp_t *resp_param = ZB_BUF_GET_PARAM(param, zb_secur_ic_get_by_idx_resp_t);
    ZB_BZERO(resp_param, sizeof(*resp_param));

    resp_param->status = ret;

    TRACE_MSG(TRACE_ERROR, "failed to send zb_secur_ic_get_by_idx_req, ret %d", (FMT__D, ret));

    if (response_cb != NULL)
    {
      ZB_SCHEDULE_CALLBACK(response_cb, param);
    }
    else
    {
      zb_buf_free(param);
    }

    if (buf_table_entry)
    {
      free_secur_buf_table_entry(&buf_table_entry);
    }
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< zb_secur_ic_get_by_idx_req", (FMT__0));
}


void zb_secur_ic_remove_req(zb_uint8_t param)
{
  zb_secur_ic_remove_req_t *req_param = ZB_BUF_GET_PARAM(param, zb_secur_ic_remove_req_t);
  secur_req_buf_table_entry_t* buf_table_entry = NULL;
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_TRANSPORT2, ">> zb_secur_ic_remove_req, buf %d", (FMT__D, param));

  ret = find_free_secur_buf_table_entry(&buf_table_entry);

  if (ret == RET_OK)
  {
    zb_uint8_t tsn;
    ret = ncp_host_secur_remove_ic_request(req_param->device_address, &tsn);

    ZB_ASSERT(ret == RET_OK);

    if (ret == RET_OK)
    {
      buf_table_entry->buf = param;
      buf_table_entry->tsn = tsn;
    }
  }

  if (ret != RET_OK)
  {
    zb_callback_t response_cb = req_param->response_cb;
    zb_secur_ic_remove_resp_t *resp_param = ZB_BUF_GET_PARAM(param, zb_secur_ic_remove_resp_t);
    ZB_BZERO(resp_param, sizeof(*resp_param));

    resp_param->status = ret;

    TRACE_MSG(TRACE_ERROR, "failed to send zb_secur_ic_remove_req, ret %d", (FMT__D, ret));

    if (response_cb != NULL)
    {
      ZB_SCHEDULE_CALLBACK(response_cb, param);
    }
    else
    {
      zb_buf_free(param);
    }

    if (buf_table_entry)
    {
      free_secur_buf_table_entry(&buf_table_entry);
    }
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< zb_secur_ic_remove_req", (FMT__0));
}


void zb_secur_ic_remove_all_req(zb_uint8_t param)
{
  zb_secur_ic_remove_all_req_t *req_param = ZB_BUF_GET_PARAM(param, zb_secur_ic_remove_all_req_t);
  secur_req_buf_table_entry_t* buf_table_entry = NULL;
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_TRANSPORT2, ">> zb_secur_ic_remove_all_req, buf %d", (FMT__D, param));

  ret = find_free_secur_buf_table_entry(&buf_table_entry);

  if (ret == RET_OK)
  {
    zb_uint8_t tsn;
    ret = ncp_host_secur_remove_all_ic_request(&tsn);

    ZB_ASSERT(ret == RET_OK);

    if (ret == RET_OK)
    {
      buf_table_entry->buf = param;
      buf_table_entry->tsn = tsn;
    }
  }

  if (ret != RET_OK)
  {
    zb_callback_t response_cb = req_param->response_cb;
    zb_secur_ic_remove_all_resp_t *resp_param = ZB_BUF_GET_PARAM(param, zb_secur_ic_remove_all_resp_t);
    ZB_BZERO(resp_param, sizeof(*resp_param));

    resp_param->status = ret;

    TRACE_MSG(TRACE_ERROR, "failed to send zb_secur_ic_remove_all_req, ret %d", (FMT__D, ret));

    if (response_cb != NULL)
    {
      ZB_SCHEDULE_CALLBACK(response_cb, param);
    }
    else
    {
      zb_buf_free(param);
    }

    if (buf_table_entry)
    {
      free_secur_buf_table_entry(&buf_table_entry);
    }
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< zb_secur_ic_remove_all_req", (FMT__0));
}


void ncp_host_handle_secur_tclk_indication(zb_ret_t status,
                                           zb_ieee_addr_t trust_center_address,
                                           zb_uint8_t key_type)
{
  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_handle_secur_tclk_indication, status 0x%x",
            (FMT__D, status));

  TRACE_MSG(TRACE_TRANSPORT2, "status 0x%x", (FMT__D, status));

  ncp_host_state_set_waiting_for_tclk(ZB_FALSE);

  if (status == RET_OK)
  {
    TRACE_MSG(TRACE_TRANSPORT2, "tc address" TRACE_FORMAT_64 ", key_type 0x%x",
              (FMT__A_D, TRACE_ARG_64(trust_center_address), key_type));

    zb_aib_set_trust_center_address(trust_center_address);

    /* TODO: handle key_type properly, when we use more than one type */
    ZB_ASSERT(key_type == 0);

    ncp_host_state_set_tclk_valid(ZB_TRUE);

    zb_buf_get_out_delayed(zdo_commissioning_tclk_upd_complete);
  }
  else
  {
    zb_buf_get_out_delayed(zdo_commissioning_tclk_upd_failed);
  }


  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_handle_secur_tclk_indication", (FMT__0));
}


void ncp_host_handle_secur_add_ic_response(zb_ret_t status_code, zb_uint8_t ncp_tsn)
{
  secur_req_buf_table_entry_t *buf_table_entry = find_secur_buf_table_entry_by_tsn(ncp_tsn);

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_secur_add_ic_response, status_code %d",
    (FMT__D, status_code));

  if (buf_table_entry)
  {
    ncp_secur_ic_add_req_t *req_param = ZB_BUF_GET_PARAM(buf_table_entry->buf, ncp_secur_ic_add_req_t);

    if (req_param->response_cb)
    {
      req_param->response_cb(status_code);
    }

    zb_buf_free(buf_table_entry->buf);
    free_secur_buf_table_entry(&buf_table_entry);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "got a response, but no info about a request, skip", (FMT__0));
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_secur_add_ic_response", (FMT__0));
}


void ncp_host_handle_secur_get_ic_list_response(zb_ret_t status, zb_uint8_t ncp_tsn,
  zb_uint8_t ic_table_entries, zb_uint8_t start_index, zb_uint8_t ic_table_list_count,
  ncp_host_secur_installcode_record_t *ic_table)
{
  secur_req_buf_table_entry_t *buf_table_entry = find_secur_buf_table_entry_by_tsn(ncp_tsn);

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_secur_get_ic_list_response, status_code %d",
    (FMT__D, status));

  if (buf_table_entry)
  {
    zb_secur_ic_get_list_req_t req;
    zb_secur_ic_get_list_resp_t *resp_param = NULL;
    zb_aps_installcode_nvram_t *resp_body = NULL;
    zb_aps_installcode_nvram_t *resp_buffer_record = NULL;
    ncp_host_secur_installcode_record_t *resp_record = NULL;
    zb_uindex_t entry_index = 0;

    ZB_MEMCPY(&req, ZB_BUF_GET_PARAM(buf_table_entry->buf, zb_secur_ic_get_list_req_t), sizeof(req));

    if (req.response_cb)
    {
      resp_param = ZB_BUF_GET_PARAM(buf_table_entry->buf, zb_secur_ic_get_list_resp_t);
      resp_param->status = status;
      resp_param->ic_table_entries = ic_table_entries;
      resp_param->start_index = start_index;
      resp_param->ic_table_list_count = ic_table_list_count;

      resp_body = (zb_aps_installcode_nvram_t*)zb_buf_initial_alloc(buf_table_entry->buf,
        sizeof(zb_aps_installcode_nvram_t) * ic_table_list_count);

      for (entry_index = 0; entry_index < resp_param->ic_table_list_count; entry_index++)
      {
        resp_buffer_record = &resp_body[entry_index];
        resp_record = &ic_table[entry_index];

        ZB_IEEE_ADDR_COPY(resp_buffer_record->device_address, resp_record->device_address);

        resp_buffer_record->options = resp_record->options;

        ZB_MEMCPY(resp_buffer_record->installcode, resp_record->installcode,
          ZB_IC_SIZE_BY_TYPE(resp_buffer_record->options));
      }

      ZB_SCHEDULE_CALLBACK(req.response_cb, buf_table_entry->buf);
    }
    else
    {
      zb_buf_free(buf_table_entry->buf);
    }

    free_secur_buf_table_entry(&buf_table_entry);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "got a response, but no info about a request, skip", (FMT__0));
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_secur_get_ic_list_response", (FMT__0));
}


void ncp_host_handle_secur_get_ic_by_idx_response(zb_ret_t status, zb_uint8_t ncp_tsn,
  ncp_host_secur_installcode_record_t *ic_record)
{
  secur_req_buf_table_entry_t *buf_table_entry = find_secur_buf_table_entry_by_tsn(ncp_tsn);

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_secur_get_ic_by_idx_response, status_code %d",
    (FMT__D, status));

  if (buf_table_entry)
  {
    zb_secur_ic_get_by_idx_req_t req;
    zb_secur_ic_get_by_idx_resp_t *resp_param = NULL;

    ZB_MEMCPY(&req, ZB_BUF_GET_PARAM(buf_table_entry->buf, zb_secur_ic_get_by_idx_req_t), sizeof(req));

    if (req.response_cb)
    {
      resp_param = ZB_BUF_GET_PARAM(buf_table_entry->buf, zb_secur_ic_get_by_idx_resp_t);
      resp_param->status = status;

      ZB_IEEE_ADDR_COPY(resp_param->device_address, ic_record->device_address);

      resp_param->ic_type = ic_record->options;

      ZB_MEMCPY(resp_param->installcode, ic_record->installcode,
        ZB_IC_SIZE_BY_TYPE(resp_param->ic_type));

      ZB_SCHEDULE_CALLBACK(req.response_cb, buf_table_entry->buf);
    }
    else
    {
      zb_buf_free(buf_table_entry->buf);
    }

    free_secur_buf_table_entry(&buf_table_entry);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "got a response, but no info about a request, skip", (FMT__0));
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_secur_get_ic_by_idx_response", (FMT__0));
}


void ncp_host_handle_secur_remove_ic_response(zb_ret_t status, zb_uint8_t ncp_tsn)
{
  secur_req_buf_table_entry_t *buf_table_entry = find_secur_buf_table_entry_by_tsn(ncp_tsn);

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_secur_remove_ic_response, status_code %d",
    (FMT__D, status));

  if (buf_table_entry)
  {
    zb_secur_ic_remove_req_t req;
    zb_secur_ic_remove_resp_t *resp_param = NULL;

    ZB_MEMCPY(&req, ZB_BUF_GET_PARAM(buf_table_entry->buf, zb_secur_ic_remove_req_t), sizeof(req));

    if (req.response_cb)
    {
      resp_param = ZB_BUF_GET_PARAM(buf_table_entry->buf, zb_secur_ic_remove_resp_t);
      resp_param->status = status;

      ZB_SCHEDULE_CALLBACK(req.response_cb, buf_table_entry->buf);
    }
    else
    {
      zb_buf_free(buf_table_entry->buf);
    }

    free_secur_buf_table_entry(&buf_table_entry);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "got a response, but no info about a request, skip", (FMT__0));
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_secur_remove_ic_response", (FMT__0));
}


void ncp_host_handle_secur_remove_all_ic_response(zb_ret_t status, zb_uint8_t ncp_tsn)
{
  secur_req_buf_table_entry_t *buf_table_entry = find_secur_buf_table_entry_by_tsn(ncp_tsn);

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_secur_remove_all_ic_response, status_code %d",
    (FMT__D, status));

  if (buf_table_entry)
  {
    zb_secur_ic_remove_all_req_t req;
    zb_secur_ic_remove_all_resp_t *resp_param = NULL;

    ZB_MEMCPY(&req, ZB_BUF_GET_PARAM(buf_table_entry->buf, zb_secur_ic_remove_all_req_t), sizeof(req));

    if (req.response_cb)
    {
      resp_param = ZB_BUF_GET_PARAM(buf_table_entry->buf, zb_secur_ic_remove_all_resp_t);
      resp_param->status = status;

      ZB_SCHEDULE_CALLBACK(req.response_cb, buf_table_entry->buf);
    }
    else
    {
      zb_buf_free(buf_table_entry->buf);
    }

    free_secur_buf_table_entry(&buf_table_entry);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "got a response, but no info about a request, skip", (FMT__0));
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_secur_remove_all_ic_response", (FMT__0));
}



void ncp_host_handle_nwk_initiate_key_switch_procedure_response(zb_ret_t status)
{
  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_initiate_key_switch_procedure_response, status_code %d",
    (FMT__D, status));

  ZB_ASSERT(status == RET_OK);

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_initiate_key_switch_procedure_response", (FMT__0));
}
