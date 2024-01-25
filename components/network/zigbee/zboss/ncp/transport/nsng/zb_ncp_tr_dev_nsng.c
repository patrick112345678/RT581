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
/* PURPOSE: NCP transport for nsng, low level. Both NCP and Host sides.
*/

#define ZB_TRACE_FILE_ID 14006
#include "zb_common.h"
#include "transport/zb_ncp_tr.h"

#ifdef ZB_NCP_TRANSPORT_TYPE_NSNG
#include "ns_api.h"


ZB_RING_BUFFER_DECLARE(ncp_tr_nsng_rx_ring, zb_uint8_t, 2048);

struct zbncp_transport_impl_s
{
  zbncp_transport_cb_t cb;
  ns_api_handle_t *ns_api_ctx;
  zb_int_t socket;
  ncp_tr_nsng_rx_ring_t rx_ring;
  zbncp_memref_t rx_mem;
  zbncp_transport_ops_t ops;
};

typedef struct zbncp_transport_impl_s ncp_tr_nsng_t;

static ncp_tr_nsng_t s_ncp_tr_nsng;

static void dump_data(const void *data, zb_uint_t len)
{
  const zb_uint8_t *p = data;
  zb_uint_t i;

  for (i = 0 ; i < len ; ++i)
  {
    TRACE_MSG(TRACE_COMMON3, "0x%hx", (FMT__H, p[i]));
  }
}


/**
   Process data received by nsng.

   This threads runs in ZBOSS main loop.
 */
void ncp_tr_nsng_handle_rx(zb_uint8_t unused)
{
  zb_uint_t i;
  zb_uint8_t *rx_mem;
  zb_uint_t rx_size;
  zb_uint_t rx_ring_used_size;
  ncp_tr_nsng_t *tr = &s_ncp_tr_nsng;

  ZVUNUSED(unused);
  rx_mem = (zb_uint8_t *)tr->rx_mem.ptr;
  rx_size = (zb_uint_t)tr->rx_mem.size;
  rx_ring_used_size = ZB_RING_BUFFER_USED_SPACE(&tr->rx_ring);

  TRACE_MSG(TRACE_COMMON3, "ncp_tr_nsng_handle_rx : rx_mem.size %d un rb %d", (FMT__D_D, rx_size, rx_ring_used_size));

  if (rx_size <= rx_ring_used_size)
  {
    for (i = 0 ; i < rx_size ; ++i)
    {
      rx_mem[i] = *ZB_RING_BUFFER_GET(&tr->rx_ring);
      ZB_RING_BUFFER_FLUSH_GET(&tr->rx_ring);
    }

    tr->rx_mem = zbncp_memref_null();
  }
  else
  {
    rx_size = 0;
  }


  if (rx_size != 0)
  {
    if (tr->cb.recv)
    {
      TRACE_MSG(TRACE_COMMON3, "call ll rx cb %p size %d", (FMT__P_D, tr->cb.recv, rx_size));
      tr->cb.recv(tr->cb.arg, rx_size);
    }
  }
  else
  {
    TRACE_MSG(TRACE_COMMON3, "No enough data received", (FMT__0));
  }
}


/**
   nsng calls that callback to handle received serial data.
   Can be called either from RX thread or from the main thread.
   Just enqueue received data.
 */
static void ncp_tr_nsng_serial_rx_cb(ns_api_handle_t *h, zb_uint_t src_device_i, zb_uint8_t *buf, zb_uint_t len)
{
  ncp_tr_nsng_t *tr = &s_ncp_tr_nsng;
  zb_uint_t i;

  ZVUNUSED(h);
  ZVUNUSED(src_device_i);       /* we are peer to peer always, so ignore */

  TRACE_MSG(TRACE_COMMON1, ">ncp_tr_nsng_serial_rx_cb src_device_i %d buf %p len %d", (FMT__D_P_D, src_device_i, buf, len));
  if (TRACE_ENABLED(TRACE_COMMON3))
  {
    dump_data(buf, len);
  }

  for (i = 0 ; i < len && !ZB_RING_BUFFER_IS_FULL(&tr->rx_ring) ; ++i)
  {
    ZB_RING_BUFFER_PUT(&tr->rx_ring, buf[i]);
  }

  if (i < len)
  {
    TRACE_MSG(TRACE_ERROR, "Oops: no enough space for %d bytes", (FMT__D, len - i));
  }

  /* Process received data */
  ZB_SCHEDULE_APP_CALLBACK(ncp_tr_nsng_handle_rx, 0);

  TRACE_MSG(TRACE_COMMON1, "<ncp_tr_nsng_serial_rx_cb", (FMT__0));

}

static void ncp_tr_nsng_init(ncp_tr_nsng_t *tr, const zbncp_transport_cb_t *cb)
{
  TRACE_MSG(TRACE_COMMON2, ">ncp_tr_nsng_init - ZBOSS FW NCP transport starting", (FMT__0));

  tr->cb = *cb;

  /* ns_api_init & reset is required for Host only. NCP already
   * initialized nsng during MAC init when ncp_tr_nsng_init is
   * called.  */
  tr->socket = 0; /* From nsng's point of view we are test, so send to TH always */
  tr->ns_api_ctx = &ZB_IOCTX().api_ctx;

  ns_api_set_serial_callback(tr->ns_api_ctx, ncp_tr_nsng_serial_rx_cb);

  if (tr->cb.init)
  {
    TRACE_MSG(TRACE_COMMON1, "calling ll init cb %p", (FMT__P, tr->cb.init));
    tr->cb.init(tr->cb.arg);
  }

  TRACE_MSG(TRACE_COMMON2, "<ncp_tr_nsng_init", (FMT__0));
}


static void ncp_tr_nsng_send(ncp_tr_nsng_t *tr, zbncp_cmemref_t mem)
{
  TRACE_MSG(TRACE_COMMON3, ">ncp_tr_nsng_send data %p len %ld ncp_socket %d", (FMT__P_L_D, mem.ptr, mem.size, tr->socket));
  if (TRACE_ENABLED(TRACE_COMMON3))
  {
    dump_data(mem.ptr, mem.size);
  }
  if (tr->socket != -1)
  {
    TRACE_MSG(TRACE_COMMON3, " ncp_tr_nsng_send data %p len %ld ncp_socket %d", (FMT__P_L_D, mem.ptr, mem.size, tr->socket));
    /* send in one nsng packet */
    ns_api_serial_tx(tr->ns_api_ctx, tr->socket, (zb_uint8_t *)mem.ptr, (zb_uint_t)mem.size);
    /* just in case - try rx with 0 timeout */
    //ns_api_wait(tr->ns_api_ctx, 0);
    if (tr->cb.send)
    {
      /* inform LL logic that data is sent */
      TRACE_MSG(TRACE_COMMON1, "calling ll sent cb %p", (FMT__P, tr->cb.send));
      tr->cb.send(tr->cb.arg, ZBNCP_TR_SEND_STATUS_SUCCESS);
    }
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Oops, no ncp socket", (FMT__0));
  }
  TRACE_MSG(TRACE_COMMON3, "<ncp_tr_nsng_send", (FMT__0));
}


/**
   Provide a buffer to receive data from the peer.

   Note that some received data may be already buffered in the ring buffer.
   ncp_tr_nsng_handle_rx transfers it into LL memory.
 */
static void ncp_tr_nsng_recv(ncp_tr_nsng_t *tr, zbncp_memref_t mem)
{
  if (!zbncp_memref_is_valid(tr->rx_mem))
  {
    TRACE_MSG(TRACE_COMMON3, "ncp_tr_nsng_recv mem %p", (FMT__P, mem));
    tr->rx_mem = mem;

    /* Check if we already have any RX data */
    ZB_SCHEDULE_APP_CALLBACK(ncp_tr_nsng_handle_rx, 0);
  }
  else
  {

    /* INTERNAL ERROR: Can't start a new read operation before
       the previous read operation completed */
    TRACE_MSG(TRACE_ERROR, "Oops, RX mem ptr is still valid from previous call", (FMT__0));
  }
}

const zbncp_transport_ops_t *ncp_dev_transport_create(void)
{
  s_ncp_tr_nsng.ops.impl = &s_ncp_tr_nsng;
  s_ncp_tr_nsng.ops.init = ncp_tr_nsng_init;
  s_ncp_tr_nsng.ops.send = ncp_tr_nsng_send;
  s_ncp_tr_nsng.ops.recv = ncp_tr_nsng_recv;
  return &s_ncp_tr_nsng.ops;
}

#endif /* ZB_NCP_TRANSPORT_TYPE_NSNG */
