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

#define ZB_TRACE_FILE_ID 14007
#include "zb_common.h"
#include "ns_api.h"
#include "transport/zb_ncp_tr.h"

ZB_RING_BUFFER_DECLARE(ncp_tr_nsng_rx_ring, zb_uint8_t, 2048);

struct zbncp_transport_impl_s
{
  zbncp_transport_cb_t cb;
  ns_api_handle_t *ns_api_ctx;
  /* ns_api used for Host only. */
  ns_api_handle_t ns_api;
  zb_int_t socket;
  ncp_tr_nsng_rx_ring_t rx_ring;
  zbncp_memref_t rx_mem;
  zbncp_transport_ops_t ops;
  osif_mutex_t mutex;
  osif_mutex_t rx_mutex; /* Use separate mutex to synchronize an access to the RX buffer */
  osif_thread_t thread;
  osif_control_pipe_t thread_waker;
  zb_uint_t main_thread_active;
};

typedef struct zbncp_transport_impl_s ncp_tr_nsng_t;

static ncp_tr_nsng_t s_ncp_tr_nsng;

static void lock_nsng(ncp_tr_nsng_t *tr);
static void unlock_nsng(ncp_tr_nsng_t *tr);

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
void ncp_tr_nsng_handle_rx(ncp_tr_nsng_t *tr)
{
  zb_bool_t recv_complete = ZB_FALSE;
  zb_uint_t rx_size = 0u;

  TRACE_MSG(TRACE_COMMON3, ">ncp_tr_nsng_handle_rx", (FMT__0));

  osif_lock_mutex(&tr->rx_mutex);

  if (zbncp_memref_is_valid(tr->rx_mem))
  {
    zb_uint8_t *rx_mem = (zb_uint8_t *)tr->rx_mem.ptr;
    zb_uint_t rb_used_size = ZB_RING_BUFFER_USED_SPACE(&tr->rx_ring);
    zb_uint_t i;

    rx_size = (zb_uint_t)tr->rx_mem.size;

    TRACE_MSG(TRACE_COMMON3, " rx_mem.size %d rb used %d", (FMT__D_D, rx_size, rb_used_size));

    if (rx_size <= rb_used_size)
    {
      for (i = 0 ; i < rx_size ; ++i)
      {
        rx_mem[i] = *ZB_RING_BUFFER_GET(&tr->rx_ring);
        ZB_RING_BUFFER_FLUSH_GET(&tr->rx_ring);
      }

      tr->rx_mem = zbncp_memref_null();
      recv_complete = ZB_TRUE;
    }
    else
    {
      TRACE_MSG(TRACE_COMMON3, " not enough data received", (FMT__0));
    }
  }
  else
  {
    TRACE_MSG(TRACE_COMMON3, " ll is not wating for data now", (FMT__0));
  }

  osif_unlock_mutex(&tr->rx_mutex);

  if (recv_complete && tr->cb.recv)
  {
    TRACE_MSG(TRACE_COMMON3, " call ll rx cb %p size %d", (FMT__P_D, tr->cb.recv, rx_size));
    tr->cb.recv(tr->cb.arg, rx_size);
  }

  TRACE_MSG(TRACE_COMMON3, "<ncp_tr_nsng_handle_rx", (FMT__0));
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

  osif_lock_mutex(&tr->rx_mutex);

  for (i = 0 ; i < len && !ZB_RING_BUFFER_IS_FULL(&tr->rx_ring) ; ++i)
  {
    ZB_RING_BUFFER_PUT(&tr->rx_ring, buf[i]);
  }

  osif_unlock_mutex(&tr->rx_mutex);

  if (i < len)
  {
    TRACE_MSG(TRACE_ERROR, "Oops: no enough space for %d bytes", (FMT__D, len - i));
  }

  /* Process received data */
  ncp_tr_nsng_handle_rx(tr);

  TRACE_MSG(TRACE_COMMON1, "<ncp_tr_nsng_serial_rx_cb", (FMT__0));

}


/**
   That callback is called by nsng when another process connected
 */
static void ncp_tr_nsng_connect_cb(ns_api_handle_t *h, zb_int_t socket, zb_uint8_t is_connected)
{
  ncp_tr_nsng_t *tr = &s_ncp_tr_nsng;

  TRACE_MSG(TRACE_COMMON1, ">ncp_tr_nsng_connect_cb socket %d is_connected %d", (FMT__D_D, socket, is_connected));

  if (is_connected)
  {
    /* NCP must be first process connected to nsng. Ignore others */
    if (tr->socket == -1)
    {
      tr->socket = socket;
    }
  }
  else if (tr->socket == socket)
  {
    tr->socket = -1;
  }
  TRACE_MSG(TRACE_COMMON1, "<ncp_tr_nsng_connect_cb", (FMT__0));
  ZVUNUSED(h);
}


OSIF_DECLARE_IPC_WAIT_CONTROL_T(ncp_nsng_wait_control_t, 2);

/**
   Thread which receives data fro serial.

   Implemented as a thread to be more similar to HW setup.
   nsng logic might be implemented without using an additional thread.
 */
static osif_func_ret_t ncp_tr_nsng_thread(osif_func_arg_t arg)
{
  ncp_tr_nsng_t *tr = (ncp_tr_nsng_t *)arg;

  TRACE_MSG(TRACE_COMMON3, "ncp_tr_nsng_thread started tr %p", (FMT__P, tr));

  /* TODO: implement deinit & thread exit */
  while (1)
  {
    osif_lock_mutex(&tr->mutex);

    if (
      /* rx_done set when NS_PKT_DATA packet received in
         ns_handle_rcvd_pkt but not handled by MAC yet. Must wait
         until that packet is handled by MAC.  Since this is debug
         stuff, ok to loop there.
       */
      !tr->ns_api_ctx->rx_done
      /* main_thread_active is used to prevent looping here while
       * sending thread not locked mutex yet. Again, do some looping
       * is ok in debug. But never use such a solution in
       * production. */
      && !tr->main_thread_active)
    {
      ncp_nsng_wait_control_t wc;
      zb_int_t n;
      zb_uint_t mask;

      osif_ipc_init_wait(wc, 2);
      osif_ipc_setup_wait(wc, 0, tr->ns_api_ctx->h, OSIF_IPC_SIGNAL_RX);
      osif_ipc_setup_wait(wc, 1, osif_ctrl_pipe_get_handler(tr->thread_waker), OSIF_IPC_SIGNAL_RX);
      n = osif_ipc_wait_for_io(wc, 2, -1);
      if (n > 0)
      {
        if (osif_ipc_signaled(wc, 1, &mask))
        {
          TRACE_MSG(TRACE_COMMON3, "ncp_tr_nsng_thread started waked up", (FMT__0));
          osif_ctrl_pipe_event_flush(tr->thread_waker);
        }
        if (osif_ipc_signaled(wc, 0, &mask))
        {
          /* ns api wait w/o timeout also handles rx and calls ncp_tr_nsng_serial_rx_cb()
             Note that mutex is locked by the tread that time!
           */
          TRACE_MSG(TRACE_COMMON3, "ncp_tr_nsng_thread try handle rx", (FMT__0));
          ns_api_wait(tr->ns_api_ctx, 0);
        }
      }
    }
    osif_unlock_mutex(&tr->mutex);
  }

  return 0;
}


static void lock_nsng(ncp_tr_nsng_t *tr)
{
  tr->main_thread_active = 1;
  osif_ctrl_pipe_event_set(tr->thread_waker, '0');
  osif_lock_mutex(&tr->mutex);
}


static void unlock_nsng(ncp_tr_nsng_t *tr)
{
  tr->main_thread_active = 0;
  osif_unlock_mutex(&tr->mutex);
}


static void ncp_tr_nsng_init(ncp_tr_nsng_t *tr, const zbncp_transport_cb_t *cb)
{
  zb_ret_t ret = RET_OK;

#ifndef DISABLE_TRACE_INIT
  TRACE_INIT("ncp_host_ll_so");
#endif
  TRACE_MSG(TRACE_COMMON2, ">ncp_tr_nsng_init - ZBOSS FW Host transport starting", (FMT__0));

  tr->cb = *cb;

  tr->ns_api_ctx = &tr->ns_api;
  tr->socket = -1;

  /* ns_api_init & reset is required for Host: it has no MAC running. */
  ret = ns_api_init(tr->ns_api_ctx);
  if (ret == RET_OK)
  {
    ret = ns_api_reset(tr->ns_api_ctx);
  }
  if (ret == RET_OK)
  {
    ret = ns_api_subscribe_to_connects(tr->ns_api_ctx, ncp_tr_nsng_connect_cb);
  }
  if (ret == RET_OK)
  {
    TRACE_MSG(TRACE_COMMON2, ">ncp_tr_nsng_init - waiting for the peer...", (FMT__0));
    while (tr->socket == -1)
    {
      (void)ns_api_wait(tr->ns_api_ctx, 0);
    }
    TRACE_MSG(TRACE_COMMON2, "... wait done, socket %d", (FMT__D, tr->socket));
  }

  if (ret == RET_OK)
  {
    ns_api_set_serial_callback(tr->ns_api_ctx, ncp_tr_nsng_serial_rx_cb);
    osif_ctrl_pipe_init(&tr->thread_waker);
    osif_init_mutex(&tr->mutex);
    osif_init_mutex(&tr->rx_mutex);
    /* Start bg i/o logic in a thread. Imitate interrupt-driven i/o */
    osif_start_thread(&tr->thread, ncp_tr_nsng_thread, (osif_func_arg_t)tr);
  }

  if (tr->cb.init)
  {
    TRACE_MSG(TRACE_COMMON1, "calling ll init cb %p", (FMT__P, tr->cb.init));
    tr->cb.init(tr->cb.arg);
  }

  TRACE_MSG(TRACE_COMMON2, "<ncp_tr_nsng_init socket %d ret %d", (FMT__D_D, tr->socket, ret));
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
    lock_nsng(tr);
    /* send in one nsng packet */
    ns_api_serial_tx(tr->ns_api_ctx, tr->socket, (zb_uint8_t *)mem.ptr, (zb_uint_t)mem.size);
    /* just in case - try rx with 0 timeout */
    //ns_api_wait(tr->ns_api_ctx, 0);
    unlock_nsng(tr);
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
  /* Modification os RX-related variables should be done under RX mutex */
  osif_lock_mutex(&tr->rx_mutex);
  if (!zbncp_memref_is_valid(tr->rx_mem))
  {
    TRACE_MSG(TRACE_COMMON3, "ncp_tr_nsng_recv mem %p", (FMT__P, mem));
    tr->rx_mem = mem;
    osif_unlock_mutex(&tr->rx_mutex);

    /* Check if we already have any RX data */
    ncp_tr_nsng_handle_rx(tr);
  }
  else
  {
    osif_unlock_mutex(&tr->rx_mutex);

    /* INTERNAL ERROR: Can't start a new read operation before
       the previous read operation completed */
    TRACE_MSG(TRACE_ERROR, "Oops, RX mem ptr is still valid from previous call", (FMT__0));
  }
}


const zbncp_transport_ops_t *ncp_host_transport_create(void)
{
  s_ncp_tr_nsng.ops.impl = &s_ncp_tr_nsng;
  s_ncp_tr_nsng.ops.init = ncp_tr_nsng_init;
  s_ncp_tr_nsng.ops.send = ncp_tr_nsng_send;
  s_ncp_tr_nsng.ops.recv = ncp_tr_nsng_recv;
  return &s_ncp_tr_nsng.ops;
}
