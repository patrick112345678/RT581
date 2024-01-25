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
/*  PURPOSE: NCP High level transport buffers implementation for the host side
*/

#define ZB_TRACE_FILE_ID 22

#include "ncp_host_hl_buffers.h"

static void init_rx_buf_handle(zb_uint8_t *ptr, zb_uint16_t len, ncp_host_hl_rx_buf_handle_t *handle)
{
  ZB_BZERO(handle, sizeof(*handle));
  handle->ptr = ptr;
  handle->len = len;
}

static void init_tx_buf_handle(zb_uint8_t buf_id,
                               zb_uint8_t *ptr,
                               zb_uint16_t max_len,
                               ncp_host_hl_tx_buf_handle_t *handle)
{
  ZB_BZERO(handle, sizeof(*handle));
  handle->ptr = ptr;
  handle->len = max_len;
  handle->buf_id = buf_id;
}

zb_ret_t ncp_host_hl_init_indication_body(ncp_hl_ind_header_t *hdr,
                                          zb_uint16_t len,
                                          ncp_host_hl_rx_buf_handle_t *handle)
{
  zb_ret_t ret = RET_OK;
  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_hl_init_indication_body, hdr %p, len %d",
            (FMT__P_D, hdr, len));

  ZB_ASSERT(hdr);
  ZB_ASSERT(handle);

  init_rx_buf_handle((zb_uint8_t*)(hdr + 1),
                     len - sizeof(*hdr),
                     handle);

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_hl_init_indication_body, ret %d", (FMT__D, ret));
  return ret;
}


zb_ret_t ncp_host_hl_init_response_body(ncp_hl_response_header_t *hdr,
                                        zb_uint16_t len,
                                        ncp_host_hl_rx_buf_handle_t *handle)
{
  zb_ret_t ret = RET_OK;
  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_hl_init_response_body, hdr %p, len %d",
            (FMT__P_D, hdr, len));

  ZB_ASSERT(hdr);
  ZB_ASSERT(handle);

  init_rx_buf_handle((zb_uint8_t*)(hdr + 1),
                     len - sizeof(*hdr),
                     handle);

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_hl_init_response_body, ret %d", (FMT__D, ret));
  return ret;
}

zb_ret_t ncp_host_hl_init_request_body(zb_uint8_t buf_id,
                                       ncp_hl_request_header_t *hdr,
                                       zb_uint16_t max_len,
                                       ncp_host_hl_tx_buf_handle_t *handle)
{
  zb_ret_t ret = RET_OK;
  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_hl_init_request_body, hdr %p", (FMT__P, hdr));

  ZB_ASSERT(hdr);
  ZB_ASSERT(handle);

  init_tx_buf_handle(buf_id,
                    (zb_uint8_t*)(hdr + 1),
                     max_len - sizeof(*hdr),
                     handle);

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_hl_init_request_body, ret %d", (FMT__D, ret));
  return ret;
}

void ncp_host_hl_buf_get_u8(ncp_host_hl_rx_buf_handle_t *buf, zb_uint8_t *val)
{
  zb_size_t vsize = sizeof(*val);

  ZB_ASSERT(buf->pos + vsize <= buf->len);

  *val = buf->ptr[buf->pos];
  buf->pos += vsize;
}

void ncp_host_hl_buf_get_u16(ncp_host_hl_rx_buf_handle_t *buf, zb_uint16_t *val)
{
  zb_size_t vsize = sizeof(*val);

  ZB_ASSERT(buf->pos + vsize <= buf->len);

  ZB_LETOH16(val, &buf->ptr[buf->pos]);
  buf->pos += vsize;
}

void ncp_host_hl_buf_get_u32(ncp_host_hl_rx_buf_handle_t *buf, zb_uint32_t *val)
{
  zb_size_t vsize = sizeof(*val);

  ZB_ASSERT(buf->pos + vsize <= buf->len);

  ZB_LETOH32(val, &buf->ptr[buf->pos]);
  buf->pos += vsize;
}

void ncp_host_hl_buf_get_u64(ncp_host_hl_rx_buf_handle_t *buf, zb_uint64_t *val)
{
  zb_size_t vsize = sizeof(*val);

  ZB_ASSERT(buf->pos + vsize <= buf->len);

  ZB_LETOH64(val, &buf->ptr[buf->pos]);
  buf->pos += vsize;
}

void ncp_host_hl_buf_get_u64addr(ncp_host_hl_rx_buf_handle_t *buf, zb_64bit_addr_t addr)
{
  zb_size_t asize = sizeof(zb_64bit_addr_t);

  ZB_ASSERT(buf->pos + asize <= buf->len);

  ZB_MEMCPY(addr, &buf->ptr[buf->pos], asize);
  buf->pos += asize;
}

void ncp_host_hl_buf_get_u128key(ncp_host_hl_rx_buf_handle_t *buf, zb_uint8_t *key)
{
  zb_size_t ksize = ZB_CCM_KEY_SIZE;

  ZB_ASSERT(buf->pos + ksize <= buf->len);

  ZB_MEMCPY(key, &buf->ptr[buf->pos], ksize);
  buf->pos += ksize;
}

void ncp_host_hl_buf_get_array(ncp_host_hl_rx_buf_handle_t *buf, zb_uint8_t *data_ptr, zb_size_t size)
{
  ZB_ASSERT(buf->pos + size <= buf->len);

  ZB_MEMCPY(data_ptr, &buf->ptr[buf->pos], size);
  buf->pos += size;
}

void ncp_host_hl_buf_get_ptr(ncp_host_hl_rx_buf_handle_t *buf, zb_uint8_t **data_ptr, zb_size_t size)
{
  ZB_ASSERT(buf->pos + size <= buf->len);

  *data_ptr = (zb_uint8_t *)&buf->ptr[buf->pos];
  buf->pos += size;
}

void ncp_host_hl_buf_put_u8(ncp_host_hl_tx_buf_handle_t *buf, zb_uint8_t val)
{
  zb_size_t vsize = sizeof(val);

  ZB_ASSERT(buf->pos + vsize <= buf->len);

  buf->ptr[buf->pos] = val;
  buf->pos += vsize;
}

void ncp_host_hl_buf_put_u16(ncp_host_hl_tx_buf_handle_t *buf, zb_uint16_t val)
{
  zb_size_t vsize = sizeof(val);

  ZB_ASSERT(buf->pos + vsize <= buf->len);

  ZB_HTOLE16(&buf->ptr[buf->pos], &val);
  buf->pos += vsize;
}

void ncp_host_hl_buf_put_u32(ncp_host_hl_tx_buf_handle_t *buf, zb_uint32_t val)
{
  zb_size_t vsize = sizeof(val);

  ZB_ASSERT(buf->pos + vsize <= buf->len);

  ZB_HTOLE32(&buf->ptr[buf->pos], &val);
  buf->pos += vsize;
}

void ncp_host_hl_buf_put_u64(ncp_host_hl_tx_buf_handle_t *buf, zb_uint64_t val)
{
  zb_size_t vsize = sizeof(val);

  ZB_ASSERT(buf->pos + vsize <= buf->len);

  ZB_HTOLE64(&buf->ptr[buf->pos], &val);
  buf->pos += vsize;
}

void ncp_host_hl_buf_put_u64addr(ncp_host_hl_tx_buf_handle_t *buf, const zb_64bit_addr_t addr)
{
  zb_size_t asize = sizeof(zb_64bit_addr_t);

  ZB_ASSERT(buf->pos + asize <= buf->len);

  ZB_MEMCPY(&buf->ptr[buf->pos], addr, asize);
  buf->pos += asize;
}

void ncp_host_hl_buf_put_u128key(ncp_host_hl_tx_buf_handle_t *buf, zb_uint8_t *key)
{
  zb_size_t ksize = ZB_CCM_KEY_SIZE;

  ZB_ASSERT(buf->pos + ksize <= buf->len);

  ZB_MEMCPY(&buf->ptr[buf->pos], key, ksize);
  buf->pos += ksize;
}

void ncp_host_hl_buf_put_array(ncp_host_hl_tx_buf_handle_t *buf, zb_uint8_t *data_ptr, zb_size_t size)
{
  ZB_ASSERT(buf->pos + size <= buf->len);

  ZB_MEMCPY(&buf->ptr[buf->pos], data_ptr, size);
  buf->pos += size;
}

zb_ret_t ncp_host_hl_buf_check_len(const ncp_host_hl_tx_buf_handle_t *buf, zb_size_t expected)
{
  zb_ret_t ret = RET_OK;

  if (buf->ptr == NULL || buf->len < expected)
  {
    ret = RET_INVALID_FORMAT;
  }
  return ret;
}

zb_uint16_t ncp_host_hl_tx_buf_get_len(ncp_host_hl_tx_buf_handle_t *handle)
{
  return handle->pos;
}
