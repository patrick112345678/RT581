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
/* PURPOSE: NCP transport for OSIF SPI, low level. NCP side.
*/

#define ZB_TRACE_FILE_ID 14005
#include "zb_common.h"
#include "transport/zb_ncp_tr.h"

#ifdef ZB_NCP_TRANSPORT_TYPE_SPI

struct zbncp_transport_impl_s
{
    zbncp_transport_ops_t ops;
    zbncp_transport_cb_t cb;
};

typedef struct zbncp_transport_impl_s ncp_tr_osif_spi_t;

static ncp_tr_osif_spi_t s_ncp_tr_osif_spi;

static void zb_osif_spi_init_complete(zb_uint8_t unused)
{
    const ncp_tr_osif_spi_t *tr = &s_ncp_tr_osif_spi;

    if (tr->cb.init != NULL)
    {
        TRACE_MSG(TRACE_COMMON1, "calling ll init cb %p", (FMT__P, tr->cb.init));
        tr->cb.init(tr->cb.arg);
    }

    (void) unused;
}

static void zb_osif_spi_send_complete(zb_uint8_t spi_status)
{
    const ncp_tr_osif_spi_t *tr = &s_ncp_tr_osif_spi;
    zbncp_tr_send_status_t status = ZBNCP_TR_SEND_STATUS_ERROR;

    switch (spi_status)
    {
    case SPI_SEND_SUCCESS:
        status = ZBNCP_TR_SEND_STATUS_SUCCESS;
        break;
    case SPI_SEND_BUSY:
        status = ZBNCP_TR_SEND_STATUS_BUSY;
        break;
    case SPI_SEND_TIMEOUT_EXPIRED:
        status = ZBNCP_TR_SEND_STATUS_TIMEOUT;
        break;
    default:
        /* MISRA rule 16.4 - Mandatory default label */
        break;
    }

    if (tr->cb.send != NULL)
    {
        TRACE_MSG(TRACE_COMMON3, "calling ll send cb %p status %d", (FMT__P_D, tr->cb.send, (zb_uint_t)status));
        tr->cb.send(tr->cb.arg, status);
    }
}

static void zb_osif_spi_recv_complete(zb_uint8_t *buf, zb_ushort_t len)
{
    const ncp_tr_osif_spi_t *tr = &s_ncp_tr_osif_spi;

    if (tr->cb.recv != NULL)
    {
        TRACE_MSG(TRACE_COMMON3, "calling ll recv cb %p len %d", (FMT__P_D, tr->cb.recv, (zb_uint_t)len));
        tr->cb.recv(tr->cb.arg, len);
    }

    (void) buf;
}

static void ncp_tr_osif_spi_init(ncp_tr_osif_spi_t *tr, const zbncp_transport_cb_t *cb)
{
    TRACE_MSG(TRACE_COMMON2, ">ncp_tr_osif_spi_init - ZBOSS FW NCP SPI transport starting", (FMT__0));

    tr->cb = *cb;

    zb_osif_spi_init(zb_osif_spi_init_complete);
    zb_osif_spi_set_cb_send_data(zb_osif_spi_send_complete);
    zb_osif_spi_set_cb_recv_data(zb_osif_spi_recv_complete);

    TRACE_MSG(TRACE_COMMON2, "<ncp_tr_osif_spi_init", (FMT__0));
}


static void ncp_tr_osif_spi_send(ncp_tr_osif_spi_t *tr, zbncp_cmemref_t mem)
{
    (void)tr;
    TRACE_MSG(TRACE_COMMON3, ">ncp_tr_osif_spi_send data %p len %d", (FMT__P_D, mem.ptr, (zb_uint_t)mem.size));
    zb_osif_spi_send_data((const zb_uint8_t *)mem.ptr, (zb_ushort_t)mem.size);
    TRACE_MSG(TRACE_COMMON3, "<ncp_tr_osif_spi_send", (FMT__0));
}


static void ncp_tr_osif_spi_recv(ncp_tr_osif_spi_t *tr, zbncp_memref_t mem)
{
    (void)tr;
    TRACE_MSG(TRACE_COMMON3, ">ncp_tr_osif_spi_recv data %p len %d", (FMT__P_D, mem.ptr, (zb_uint_t)mem.size));
    zb_osif_spi_recv_data((zb_uint8_t *)mem.ptr, (zb_ushort_t)mem.size);
    TRACE_MSG(TRACE_COMMON3, "<ncp_tr_osif_spi_recv", (FMT__0));
}

const zbncp_transport_ops_t *ncp_dev_transport_create(void)
{
    s_ncp_tr_osif_spi.ops.impl = &s_ncp_tr_osif_spi;
    s_ncp_tr_osif_spi.ops.init = ncp_tr_osif_spi_init;
    s_ncp_tr_osif_spi.ops.send = ncp_tr_osif_spi_send;
    s_ncp_tr_osif_spi.ops.recv = ncp_tr_osif_spi_recv;
    return &s_ncp_tr_osif_spi.ops;
}

#endif
