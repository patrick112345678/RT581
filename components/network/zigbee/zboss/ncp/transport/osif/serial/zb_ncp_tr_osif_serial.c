/* ZBOSS Zigbee software protocol stack
 *
 * Copyright (c) 2020-2020 Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Commercial Usage
 * Licensees holding valid Commercial licenses may use
 * this file in accordance with the Commercial License
 * Agreement provided with the Software or, alternatively, in accordance
 * with the terms contained in a written agreement relevant to the usage of the file.
 */
/* PURPOSE: NCP transport for OSIF serial, low level.
*/

#define ZB_TRACE_FILE_ID 14010
#include "zb_common.h"
#include "transport/zb_ncp_tr.h"
#include "zb_osif.h"

#ifdef ZB_NCP_TRANSPORT_TYPE_SERIAL

typedef struct zbncp_transport_impl_s
{
    zbncp_transport_ops_t ops;
    zbncp_transport_cb_t cb;
} ncp_tr_impl_t;

static ncp_tr_impl_t s_ncp_tr;


#ifndef ZB_HAVE_ASYNC_SERIAL
#define SERIAL_RX_BYTE_TIMEOUT ZB_MILLISECONDS_TO_BEACON_INTERVAL(100)

static volatile zbncp_memref_t s_rx_buf;
static volatile zbncp_size_t s_rx_offset;
static volatile zb_time_t s_rx_timestamp;

static void serial_rx_timeout(zb_bufid_t bufid)
{
    const ncp_tr_impl_t *tr = &s_ncp_tr;
    (void)(bufid);

    ZB_SCHEDULE_APP_ALARM_CANCEL(serial_rx_timeout, ZB_ALARM_ANY_PARAM);

    if ((s_rx_buf.ptr != NULL) &&
            (tr->cb.recv))
    {
        if ((s_rx_offset == s_rx_buf.size) ||
                ((ZB_TIME_SUBTRACT(ZB_TIMER_GET(), s_rx_timestamp) >= SERIAL_RX_BYTE_TIMEOUT) &&
                 (s_rx_offset > 0)))
        {
            TRACE_MSG(TRACE_COMMON3, "Serial RX timeout: recv cb %p len %d", (FMT__P_D, tr->cb.recv, (zb_uint_t)s_rx_offset));
            tr->cb.recv(tr->cb.arg, s_rx_offset);
            s_rx_offset = 0;
        }
    }

    /* Reschedule RX byte timeout alarm to handle nonsynchronized, partial transmissions. */
    ZB_SCHEDULE_APP_ALARM(serial_rx_timeout, 0, SERIAL_RX_BYTE_TIMEOUT);
}

static void serial_rx_byte_handler(zb_uint8_t byte)
{
    if (s_rx_buf.ptr != NULL)
    {
        if (s_rx_offset < s_rx_buf.size)
        {
            s_rx_timestamp = ZB_TIMER_GET();
            ((zb_uint8_t *)s_rx_buf.ptr)[s_rx_offset++] = byte;
        }
        else
        {
            ZB_SCHEDULE_APP_CALLBACK(serial_rx_timeout, 0);
        }
    }
}

#else /* ZB_HAVE_ASYNC_SERIAL */

static void ncp_tr_send_complete(zb_uint8_t tx_status)
{
    const ncp_tr_impl_t *tr = &s_ncp_tr;
    zbncp_tr_send_status_t status = ZBNCP_TR_SEND_STATUS_ERROR;

    switch (tx_status)
    {
    case SERIAL_SEND_SUCCESS:
        status = ZBNCP_TR_SEND_STATUS_SUCCESS;
        break;
    case SERIAL_SEND_BUSY:
        status = ZBNCP_TR_SEND_STATUS_BUSY;
        break;
    case SERIAL_SEND_TIMEOUT_EXPIRED:
        status = ZBNCP_TR_SEND_STATUS_TIMEOUT;
        break;
    case SERIAL_SEND_ERROR:
    /* fall-through: handle all unsupported status as errors. */
    default:
        status = ZBNCP_TR_SEND_STATUS_ERROR;
        break;
    }

    if (tr->cb.send)
    {
        TRACE_MSG(TRACE_COMMON3, "Serial TX: cb %p status %d", (FMT__P_D, tr->cb.send, (zb_uint_t)status));
        tr->cb.send(tr->cb.arg, status);
    }
}

static void ncp_tr_recv_complete(zb_uint8_t *buf, zb_ushort_t len)
{
    const ncp_tr_impl_t *tr = &s_ncp_tr;

    if (tr->cb.recv)
    {
        TRACE_MSG(TRACE_COMMON3, "Serial RX: cb %p len %d", (FMT__P_D, tr->cb.recv, (zb_uint_t)len));
        tr->cb.recv(tr->cb.arg, len);
    }

    (void) buf;
}
#endif


static void ncp_tr_init(ncp_tr_impl_t *tr, const zbncp_transport_cb_t *cb)
{
    TRACE_MSG(TRACE_COMMON2, ">ncp_tr_init - ZBOSS FW NCP transport starting", (FMT__0));

    tr->cb = *cb;
    zb_osif_serial_init();

#ifndef ZB_HAVE_ASYNC_SERIAL
    s_rx_buf.ptr = NULL;
    s_rx_offset = 0;
    zb_osif_set_uart_byte_received_cb(serial_rx_byte_handler);
#else
    zb_osif_serial_set_cb_send_data(ncp_tr_send_complete);
    zb_osif_serial_set_cb_recv_data(ncp_tr_recv_complete);
#endif

    if (tr->cb.init)
    {
        TRACE_MSG(TRACE_COMMON1, "Serial initialized: cb %p", (FMT__P, tr->cb.init));
        tr->cb.init(tr->cb.arg);
    }
    TRACE_MSG(TRACE_COMMON2, "<ncp_tr_init", (FMT__0));
}


static void ncp_tr_send(ncp_tr_impl_t *tr, zbncp_cmemref_t mem)
{
    TRACE_MSG(TRACE_COMMON3, ">ncp_tr_send data %p len %d", (FMT__P_D, mem.ptr, (zb_uint_t)mem.size));
    (void)tr;

#ifndef ZB_HAVE_ASYNC_SERIAL
    zb_osif_serial_put_bytes((zb_uint8_t *)mem.ptr, (zb_ushort_t)mem.size);
    if (tr->cb.send)
    {
        TRACE_MSG(TRACE_COMMON3, "Serial TX: cb %p status %d", (FMT__P_D, tr->cb.send, (zb_uint_t)ZBNCP_TR_SEND_STATUS_SUCCESS));
        tr->cb.send(tr->cb.arg, ZBNCP_TR_SEND_STATUS_SUCCESS);
    }
#else
    zb_osif_serial_send_data((zb_uint8_t *)mem.ptr, (zb_ushort_t)mem.size);
#endif
    TRACE_MSG(TRACE_COMMON3, "<ncp_tr_send", (FMT__0));
}


static void ncp_tr_recv(ncp_tr_impl_t *tr, zbncp_memref_t mem)
{
    (void)tr;
    TRACE_MSG(TRACE_COMMON3, ">ncp_tr_recv data %p len %d", (FMT__P_D, mem.ptr, (zb_uint_t)mem.size));
#ifndef ZB_HAVE_ASYNC_SERIAL
    /* Serial always receives data byte-by-byte without bufferring. Fill RX buffer inside byte handler. */
    ZB_SCHEDULE_APP_ALARM_CANCEL(serial_rx_timeout, ZB_ALARM_ANY_PARAM);
    s_rx_buf = mem;
    s_rx_offset = 0;
    s_rx_timestamp = ZB_TIMER_GET();
    ZB_SCHEDULE_APP_ALARM(serial_rx_timeout, 0, SERIAL_RX_BYTE_TIMEOUT);
#else
    zb_osif_serial_recv_data((zb_uint8_t *)mem.ptr, (zb_ushort_t)mem.size);
#endif
    TRACE_MSG(TRACE_COMMON3, "<ncp_tr_recv", (FMT__0));
}

#if defined NCP_MODE_HOST
const zbncp_transport_ops_t *ncp_host_transport_create(void)
#else
const zbncp_transport_ops_t *ncp_dev_transport_create(void)
#endif
{
    s_ncp_tr.ops.impl = &s_ncp_tr;
    s_ncp_tr.ops.init = ncp_tr_init;
    s_ncp_tr.ops.send = ncp_tr_send;
    s_ncp_tr.ops.recv = ncp_tr_recv;
    return &s_ncp_tr.ops;
}

#endif
