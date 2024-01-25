/* ZBOSS Zigbee software protocol stack
 *
 * Copyright (c) 2012-2021 DSR Corporation, Denver CO, USA.
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
/*  PURPOSE: NCP High level transport implementation for the host side
*/

#define ZB_TRACE_FILE_ID 20

#include "ncp_host_hl_proto.h"
#include "ncp_host_soc_state.h"
#include "ncp/zb_ncp_ll_dev.h"

#define ZB_NCP_MAX_PACKET_SIZE 0x2000u


#define TX_BUFFERS_POOL_SIZE (ZB_IOBUF_POOL_SIZE + ZDO_TRAN_TABLE_SIZE)
#define TX_BUFFER_ID_RESERVED TX_BUFFERS_POOL_SIZE
#define TX_BUFFERS_QUEUE_CAPACITY TX_BUFFERS_POOL_SIZE

typedef enum tx_buffer_status_e
{
  TX_BUFFER_STATUS_FREE                  = 0, /* buffer is free and can be used for any new request */
  TX_BUFFER_STATUS_RESERVED_BLOCKING     = 1, /* buffer is reserved for a blocking request, but the request is not prepared yet */
  TX_BUFFER_STATUS_RESERVED_NONBLOCKING  = 2, /* buffer is reserved for a nonblocking request, but the request is not prepared yet */
  TX_BUFFER_STATUS_WAIT_SENDING          = 3, /* buffer is waiting for the request sending */
  TX_BUFFER_STATUS_WAIT_RESPONSE         = 4, /* blocking request is sent, but response is not received yet */
} tx_buffer_status_t;

typedef struct tx_buffer_s
{
  tx_buffer_status_t status;
  zb_uint8_t tsn;
  zb_uint8_t data[ZB_NCP_MAX_PACKET_SIZE];
} tx_buffer_t;

ZB_RING_BUFFER_DECLARE(tx_buffer_queue, ncp_host_hl_tx_buf_handle_t, TX_BUFFERS_QUEUE_CAPACITY);

typedef struct zb_ncp_host_ctx_s
{
  tx_buffer_t tx_bufs_pool[TX_BUFFERS_POOL_SIZE];
  tx_buffer_queue_t tx_blocking_queue;
  tx_buffer_queue_t tx_nonblocking_queue;
  zb_bool_t tx_is_active;
  zb_uint8_t current_tsn;
} zb_ncp_host_ctx_t;

zb_ncp_host_ctx_t g_ncp_host_ctx;

static zb_uindex_t tx_buffer_find_free(void)
{
  zb_uint8_t buf_index;

  for (buf_index = 0; buf_index < TX_BUFFERS_POOL_SIZE; buf_index++)
  {
    if (g_ncp_host_ctx.tx_bufs_pool[buf_index].status == TX_BUFFER_STATUS_FREE)
    {
      return buf_index;
    }
  }

  return TX_BUFFER_ID_RESERVED;
}

static void tx_buffer_free(tx_buffer_t *buf)
{
  zb_uint8_t buf_index;

  for (buf_index = 0; buf_index < TX_BUFFERS_POOL_SIZE; buf_index++)
  {
    if (&g_ncp_host_ctx.tx_bufs_pool[buf_index] == buf)
    {
      TRACE_MSG(TRACE_TRANSPORT3, "TX buffer free: buf_id %hd",(FMT__H, buf_index));
    }
  }

  ZB_BZERO(buf, sizeof(buf));
}

static tx_buffer_t *tx_buffer_get_by_id(zb_uindex_t buf_id)
{
  return &g_ncp_host_ctx.tx_bufs_pool[buf_id];
}

static tx_buffer_t* tx_buffer_peek_blocking(void)
{
  ncp_host_hl_tx_buf_handle_t *buf_handle = ZB_RING_BUFFER_PEEK(&g_ncp_host_ctx.tx_blocking_queue);
  return tx_buffer_get_by_id(buf_handle->buf_id);
}

static tx_buffer_t* tx_buffer_peek_nonblocking(void)
{
  ncp_host_hl_tx_buf_handle_t *buf_handle = ZB_RING_BUFFER_PEEK(&g_ncp_host_ctx.tx_nonblocking_queue);
  return tx_buffer_get_by_id(buf_handle->buf_id);
}

static zb_bool_t blocking_request_in_progress(void)
{
  return !ZB_RING_BUFFER_IS_EMPTY(&g_ncp_host_ctx.tx_blocking_queue);
}

static zb_ret_t put_packet_in_queue(ncp_host_hl_tx_buf_handle_t *handle,
                                    zb_bool_t is_blocking_req,
                                    tx_buffer_status_t status)
{
  zb_ret_t ret = RET_OK;
  tx_buffer_queue_t *queue = NULL;
  tx_buffer_t *buf = NULL;

  buf = tx_buffer_get_by_id(handle->buf_id);

  if (is_blocking_req)
  {
    queue = &g_ncp_host_ctx.tx_blocking_queue;
  }
  else
  {
    queue = &g_ncp_host_ctx.tx_nonblocking_queue;
  }

  if (ZB_RING_BUFFER_IS_FULL(queue))
  {
    ret = RET_NO_MEMORY;
  }
  else
  {
    buf->status = status;
    ZB_RING_BUFFER_PUT_PTR(queue, handle);
  }

  return ret;
}

static void tx_ready_cb(void)
{
  ncp_host_hl_tx_buf_handle_t *buf_handle;
  tx_buffer_t *buf = NULL;
  zb_bool_t buf_ready = ZB_FALSE;
  zb_bool_t buf_blocking = ZB_FALSE;
  zb_ret_t ret = RET_ERROR;

  TRACE_MSG(TRACE_TRANSPORT3, ">> tx_ready_cb",(FMT__0));

  g_ncp_host_ctx.tx_is_active = ZB_FALSE;

  if (!ZB_RING_BUFFER_IS_EMPTY(&g_ncp_host_ctx.tx_nonblocking_queue) &&
      (ZB_RING_BUFFER_IS_EMPTY(&g_ncp_host_ctx.tx_blocking_queue)
        || tx_buffer_peek_blocking()->tsn > tx_buffer_peek_nonblocking()->tsn))
  {
    buf_handle = ZB_RING_BUFFER_PEEK(&g_ncp_host_ctx.tx_nonblocking_queue);
    buf = tx_buffer_get_by_id(buf_handle->buf_id);

    buf_ready = ZB_TRUE;
    buf_blocking = ZB_FALSE;
  }
  else if (!ZB_RING_BUFFER_IS_EMPTY(&g_ncp_host_ctx.tx_blocking_queue))
  {
    buf_handle = ZB_RING_BUFFER_PEEK(&g_ncp_host_ctx.tx_blocking_queue);
    buf = tx_buffer_get_by_id(buf_handle->buf_id);

    if (buf->status == TX_BUFFER_STATUS_WAIT_SENDING)
    {
      buf_ready = ZB_TRUE;
      buf_blocking = ZB_TRUE;
    }
  }

  if (buf_ready)
  {
    TRACE_MSG(TRACE_TRANSPORT3, "send packet from requests queue, tsn %hd, is_blocking %hd", (FMT__H_H, buf->tsn,
      buf_blocking));

    ret = ncp_ll_send_packet(buf->data,
                             ncp_host_hl_tx_buf_get_len(buf_handle) + sizeof(ncp_hl_request_header_t));

    ZB_ASSERT(ret == RET_OK);

    if (!buf_blocking)
    {
      ZB_RING_BUFFER_FLUSH_GET(&g_ncp_host_ctx.tx_nonblocking_queue);
      tx_buffer_free(buf);
    }
    else
    {
      buf->status = TX_BUFFER_STATUS_WAIT_RESPONSE;
    }

    g_ncp_host_ctx.tx_is_active = ZB_TRUE;
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< tx_ready_cb",(FMT__0));
}

void handle_response_from_device(void* data, zb_uint16_t len)
{
  ncp_hl_response_header_t* response_header = (ncp_hl_response_header_t*)data;
  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_response_from_device, data %p, len %d",
            (FMT__P_D, data, len));

  TRACE_MSG(TRACE_TRANSPORT3, "response header: tsn %d, status category: %d, status code: %d",
            (FMT__D_D_D,
             response_header->tsn,
             response_header->status_category,
             response_header->status_code));

  switch (NCP_HL_CALL_CATEGORY(response_header->hdr.call_id))
  {
    case NCP_HL_CATEGORY_CONFIGURATION:
      ncp_host_handle_configuration_response(data, len);
      break;
    case NCP_HL_CATEGORY_AF:
      ncp_host_handle_af_response(data, len);
      break;
    case NCP_HL_CATEGORY_ZDO:
      ncp_host_handle_zdo_response(data, len);
      break;
#ifdef ZB_ENABLE_INTER_PAN_NON_DEFAULT_CHANNEL
    case NCP_HL_CATEGORY_INTRP:
      ncp_host_handle_intrp_response(data, len);
      break;
#endif /* ZB_ENABLE_INTER_PAN_NON_DEFAULT_CHANNEL */
    case NCP_HL_CATEGORY_APS:
      ncp_host_handle_aps_response(data, len);
      break;
    case NCP_HL_CATEGORY_NWKMGMT:
      ncp_host_handle_nwkmgmt_response(data, len);
      break;
    case NCP_HL_CATEGORY_SECUR:
      ncp_host_handle_secur_response(data, len);
      break;
    case NCP_HL_CATEGORY_MANUF_TEST:
      ncp_host_handle_manuf_test_response(data, len);
      break;
    case NCP_HL_CATEGORY_OTA:
      ncp_host_handle_ota_response(data, len);
      break;

    default:
      TRACE_MSG(TRACE_ERROR, "unknown category: 0x%x", (FMT__D, response_header->hdr.call_id));
      break;
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_response_from_device", (FMT__0));
}


void handle_indication_from_device(void* data, zb_uint16_t len)
{
  ncp_hl_header_t* received_pkt_header = (ncp_hl_header_t*)data;

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_indication_from_device, data %p, len %d",
            (FMT__P_D, data, len));

  switch (NCP_HL_CALL_CATEGORY(received_pkt_header->call_id))
  {
    case NCP_HL_CATEGORY_CONFIGURATION:
      ncp_host_handle_configuration_indication(data, len);
      break;
    case NCP_HL_CATEGORY_AF:
      ncp_host_handle_af_indication(data, len);
      break;
    case NCP_HL_CATEGORY_ZDO:
      ncp_host_handle_zdo_indication(data, len);
      break;
#ifdef ZB_ENABLE_INTER_PAN_NON_DEFAULT_CHANNEL
    case NCP_HL_CATEGORY_INTRP:
      ncp_host_handle_intrp_indication(data, len);
      break;
#endif /* ZB_ENABLE_INTER_PAN_NON_DEFAULT_CHANNEL */
    case NCP_HL_CATEGORY_APS:
      ncp_host_handle_aps_indication(data, len);
      break;
    case NCP_HL_CATEGORY_NWKMGMT:
      ncp_host_handle_nwkmgmt_indication(data, len);
      break;
    case NCP_HL_CATEGORY_SECUR:
      ncp_host_handle_secur_indication(data, len);
      break;
    case NCP_HL_CATEGORY_MANUF_TEST:
      ncp_host_handle_manuf_test_indication(data, len);
      break;
    case NCP_HL_CATEGORY_OTA:
      ncp_host_handle_ota_indication(data, len);
      break;

    default:
      TRACE_MSG(TRACE_ERROR, "unknown category: 0x%x", (FMT__D, received_pkt_header->call_id));
      break;
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_indication_from_device", (FMT__0));
}

void handle_packet_from_device(void* data, zb_uint16_t len)
{
  ncp_hl_header_t* received_pkt_header = (ncp_hl_header_t*)data;
  zb_uint8_t packet_type;
  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_packet_from_device: data %p, len %d", (FMT__P_D, data, len));

  ZB_ASSERT(len >= sizeof(ncp_hl_header_t));
  ZB_ASSERT(received_pkt_header->version == NCP_HL_PROTO_VERSION);

  packet_type = NCP_HL_GET_PKT_TYPE(received_pkt_header->control);
  ZB_ASSERT(packet_type == NCP_HL_RESPONSE
            || packet_type == NCP_HL_INDICATION);


  ZB_LETOH16_ONPLACE(received_pkt_header->call_id);

  TRACE_MSG(TRACE_TRANSPORT3, "version: 0x%x, control: 0x%x, call_id: 0x%x",
            (FMT__D_D_D,
             received_pkt_header->version,
             received_pkt_header->control,
             received_pkt_header->call_id));


  if (packet_type == NCP_HL_RESPONSE)
  {
    handle_response_from_device(data, len);
  }
  else if (packet_type == NCP_HL_INDICATION)
  {
    handle_indication_from_device(data, len);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "unsupported packet type: 0x%x", (FMT__D, packet_type));
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_packet_from_device", (FMT__0));
}

void ncp_host_hl_proto_init(void)
{
  TRACE_MSG(TRACE_TRANSPORT1, ">> ncp_host_hl_proto_init", (FMT__0));

  ZB_RING_BUFFER_INIT(&g_ncp_host_ctx.tx_blocking_queue);
  ZB_RING_BUFFER_INIT(&g_ncp_host_ctx.tx_nonblocking_queue);

  ncp_ll_proto_init(handle_packet_from_device, tx_ready_cb);

  TRACE_MSG(TRACE_TRANSPORT1, "<< ncp_host_hl_proto_init", (FMT__0));
}

void ncp_host_mark_blocking_request_finished(void)
{
  ncp_host_hl_tx_buf_handle_t *buf_handle;

  TRACE_MSG(TRACE_TRANSPORT1, ">> ncp_host_mark_blocking_request_finished", (FMT__0));

  if (!ZB_RING_BUFFER_IS_EMPTY(&g_ncp_host_ctx.tx_blocking_queue))
  {
    buf_handle = ZB_RING_BUFFER_PEEK(&g_ncp_host_ctx.tx_blocking_queue);
    tx_buffer_free(tx_buffer_get_by_id(buf_handle->buf_id));

    ZB_RING_BUFFER_FLUSH_GET(&g_ncp_host_ctx.tx_blocking_queue);
  }

  TRACE_MSG(TRACE_TRANSPORT1, "<< ncp_host_mark_blocking_request_finished", (FMT__0));
}

static zb_uint8_t get_next_tsn(void)
{
  g_ncp_host_ctx.current_tsn++;
  if (g_ncp_host_ctx.current_tsn == NCP_TSN_RESERVED)
  {
    g_ncp_host_ctx.current_tsn++;
  }
  return g_ncp_host_ctx.current_tsn;
}

static void init_request_header(zb_uint16_t call_id, ncp_hl_request_header_t* header)
{
  header->hdr.version = NCP_HL_PROTO_VERSION;
  header->hdr.control = NCP_HL_REQUEST;
  header->hdr.call_id = call_id;
  header->tsn = get_next_tsn();
}

static zb_ret_t get_request_buffer(zb_uint16_t call_id,
                                     ncp_host_hl_tx_buf_handle_t* handle,
                                     zb_bool_t is_blocking_req,
                                     zb_uint8_t *tsn)
{
  ncp_hl_request_header_t *request_header;
  zb_ret_t ret = RET_OK;
  zb_uindex_t buf_id = tx_buffer_find_free();
  tx_buffer_t *buf = NULL;

  if (tsn == NULL)
  {
    ret = RET_INVALID_PARAMETER_4;
  }

  if ((ret == RET_OK)
      && (buf_id == TX_BUFFER_ID_RESERVED))
  {
    ret = RET_NO_MEMORY;
  }

  if (ret == RET_OK)
  {
    buf = tx_buffer_get_by_id(buf_id);

    ZB_BZERO(buf, sizeof(*buf));
    buf->status = (is_blocking_req) ? TX_BUFFER_STATUS_RESERVED_BLOCKING : TX_BUFFER_STATUS_RESERVED_NONBLOCKING;

    request_header = (ncp_hl_request_header_t*)buf->data;

    init_request_header(call_id, request_header);
    ret = ncp_host_hl_init_request_body(buf_id, request_header, ZB_NCP_MAX_PACKET_SIZE, handle);
  }

  if (ret == RET_OK)
  {
    buf->tsn = request_header->tsn;
    *tsn = request_header->tsn;
  }
  else if (buf != NULL)
  {
    tx_buffer_free(buf);
  }

  return ret;
}

zb_ret_t ncp_host_get_buf_for_blocking_request(zb_uint16_t call_id,
                                               ncp_host_hl_tx_buf_handle_t* handle,
                                               zb_uint8_t* tsn)
{
  zb_ret_t ret;
  zb_uint8_t expected_tsn = NCP_TSN_RESERVED;

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_get_buf_for_blocking_request, call_id 0x%x",
            (FMT__D, call_id));

  ret = get_request_buffer(call_id, handle, ZB_TRUE, &expected_tsn);

  if ((tsn != NULL) && (ret == RET_OK))
  {
    *tsn = expected_tsn;
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_get_buf_for_blocking_request, ret %d, tsn %hd",
    (FMT__D_H, ret, expected_tsn));
  return ret;
}


zb_ret_t ncp_host_get_buf_for_nonblocking_request(zb_uint16_t call_id,
                                                  ncp_host_hl_tx_buf_handle_t *handle,
                                                  zb_uint8_t* tsn)
{
  zb_ret_t ret;
  zb_uint8_t expected_tsn = NCP_TSN_RESERVED;

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_get_buf_for_nonblocking_request, call_id 0x%x",
            (FMT__D, call_id));

  ret = get_request_buffer(call_id, handle, ZB_FALSE, &expected_tsn);

  if ((tsn != NULL) && (ret == RET_OK))
  {
    *tsn = expected_tsn;
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_get_buf_for_nonblocking_request, ret %d, tsn %hd",
    (FMT__D_H, ret, expected_tsn));
  return ret;
}

zb_ret_t ncp_host_hl_send_packet(ncp_host_hl_tx_buf_handle_t *handle)
{
  zb_ret_t ret = RET_OK;
  tx_buffer_t *buf = NULL;

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_hl_send_packet, buf_id %hd", (FMT__H, handle->buf_id));

  ZB_ASSERT(handle->buf_id >= 0 && handle->buf_id < TX_BUFFERS_POOL_SIZE);

  buf = tx_buffer_get_by_id(handle->buf_id);

  ZB_ASSERT(buf->status == TX_BUFFER_STATUS_RESERVED_BLOCKING || buf->status == TX_BUFFER_STATUS_RESERVED_NONBLOCKING);

  if (buf->status == TX_BUFFER_STATUS_RESERVED_BLOCKING
      && (blocking_request_in_progress() || g_ncp_host_ctx.tx_is_active))
  {
    ncp_host_hl_tx_buf_handle_t *buf_handle = ZB_RING_BUFFER_GET(&g_ncp_host_ctx.tx_blocking_queue);

    TRACE_MSG(TRACE_TRANSPORT3, "    put packet in blocking requests queue, previous buf_id %hd", (FMT__H, buf_handle->buf_id));

    ret = put_packet_in_queue(handle, ZB_TRUE, TX_BUFFER_STATUS_WAIT_SENDING);
  }
  else if (buf->status == TX_BUFFER_STATUS_RESERVED_NONBLOCKING
           && g_ncp_host_ctx.tx_is_active)
  {
    ncp_host_hl_tx_buf_handle_t *buf_handle = ZB_RING_BUFFER_GET(&g_ncp_host_ctx.tx_nonblocking_queue);

    TRACE_MSG(TRACE_TRANSPORT3, "    put packet in nonblocking requests queue, previous buf_id %hd", (FMT__H, buf_handle->buf_id));

    ret = put_packet_in_queue(handle, ZB_FALSE, TX_BUFFER_STATUS_WAIT_SENDING);
  }
  else
  {
    TRACE_MSG(TRACE_TRANSPORT3, "    send packet without queueing, is_blocking %hd",
      (FMT__H, buf->status == TX_BUFFER_STATUS_RESERVED_BLOCKING));


    ret = ncp_ll_send_packet(buf->data,
                             ncp_host_hl_tx_buf_get_len(handle) + sizeof(ncp_hl_request_header_t));

    ZB_ASSERT(ret == RET_OK);
    if (ret == RET_OK)
    {
      g_ncp_host_ctx.tx_is_active = ZB_TRUE;
      if (buf->status == TX_BUFFER_STATUS_RESERVED_BLOCKING)
      {
        ret = put_packet_in_queue(handle, ZB_TRUE, TX_BUFFER_STATUS_WAIT_RESPONSE);
      }
      else
      {
        tx_buffer_free(buf);
      }
    }
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_hl_send_packet, ret %d", (FMT__D, ret));
  return ret;
}
