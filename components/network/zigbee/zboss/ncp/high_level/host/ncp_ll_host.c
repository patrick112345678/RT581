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
/*  PURPOSE: NCP Low level protocol wrapper for the host side
*/

#define ZB_TRACE_FILE_ID 14003
#include "ncp_ll_host.h"
#include "transport/zb_ncp_tr.h"
#include "zb_time.h"

typedef struct ncp_ll_ctx_s
{
    zbncp_ll_proto_t *ll;
    ncp_ll_call_me_cb_t call_me_cb;
}
ncp_ll_ctx_t;

static ncp_ll_ctx_t ncp_ll_ctx;

static zbncp_frag_ctx_t frag_ctx;

static inline zbncp_ll_time_t ncp_ll_get_time_msec(void)
{
    return (osif_transceiver_time_get() / 1000u); /* Convert to milliseconds */
}

static void ncp_host_ll_call_me_cb(zbncp_ll_cb_arg_t arg)
{
    ncp_ll_ctx.call_me_cb();
    ZVUNUSED(arg);
}

int ncp_host_ll_proto_init(ncp_ll_call_me_cb_t call_me_cb)
{
    const zbncp_transport_ops_t *tr_ops = ncp_host_transport_create();
    zbncp_ll_proto_cb_t llcb;

    TRACE_MSG(TRACE_COMMON3, ">ncp_host_ll_proto_init call_me_cb %p", (FMT__P, call_me_cb));

    ncp_ll_ctx.ll = zbncp_ll_create(tr_ops);
    ncp_ll_ctx.call_me_cb = call_me_cb;

    llcb.callme = ncp_host_ll_call_me_cb;
    llcb.arg = ZBNCP_NULL;
    zbncp_ll_init(ncp_ll_ctx.ll, &llcb, ncp_ll_get_time_msec());

    zbncp_frag_initialize(&frag_ctx);

    TRACE_MSG(TRACE_COMMON3, "<ncp_host_ll_proto_init", (FMT__0));

    return 0;
}

static int ncp_host_ll_quant_internal(
    void *tx_buf,
    zb_uint_t tx_size,
    void *rx_buf,
    zb_uint_t rx_buf_size,
    zb_uint_t *received_bytes,
    zb_bool_t *tx_ready,
    zb_uint_t *alarm_timeout_ms)
{
    zbncp_ll_quant_t llq;
    int ret;

    ret = zbncp_frag_store_tx_pkt(&frag_ctx, tx_buf, tx_size);

    zbncp_frag_set_place_for_rx_pkt(&frag_ctx, rx_buf, rx_buf_size);

    llq.req.time = ncp_ll_get_time_msec();
    zbcnp_frag_fill_request(&frag_ctx, &llq.req);

    zbncp_ll_poll(ncp_ll_ctx.ll, &llq);

    *received_bytes = zbcnp_frag_process_rx_response(&frag_ctx, &llq.res);
    *tx_ready = zbcnp_frag_process_tx_response(&frag_ctx, &llq.res);
    *alarm_timeout_ms = (zb_uint_t)llq.res.timeout;

    return ret;
}


int ncp_host_ll_quant(
    void *tx_buf,
    zb_uint_t tx_size,
    void *rx_buf,
    zb_uint_t rx_buf_size,
    zb_uint_t *received_bytes,
    zb_uint_t *alarm_timeout_ms)
{
    int ret;
    zb_bool_t tx_done;

    TRACE_MSG(TRACE_COMMON3, ">ncp_host_ll_quant rx %ld tx %ld", (FMT__D_D, rx_buf_size, tx_size));

    if (!tx_size)
    {
        tx_buf = NULL;
    }
    ret = ncp_host_ll_quant_internal(
              tx_buf, tx_size, rx_buf, rx_buf_size, received_bytes, &tx_done, alarm_timeout_ms);

    /* If tx complete now bug tx buf was busy, let's retry */
    if (tx_done
            && ret == RET_BUSY)       /* see zbncp_div_store_tx_pkt() */
    {
        zb_uint_t timeout;
        if (*received_bytes)
        {
            zb_uint_t dummy;
            TRACE_MSG(TRACE_COMMON3, "tx buf was busy and now free, retry ll iteration (already rx %d)", (FMT__D, *received_bytes));
            /* Do not receive more */
            ret = ncp_host_ll_quant_internal(
                      tx_buf, tx_size, NULL, 0, &dummy, &tx_done, &timeout);
        }
        else
        {
            ret = ncp_host_ll_quant_internal(
                      tx_buf, tx_size, rx_buf, rx_buf_size, received_bytes, &tx_done, &timeout);
        }
        if (timeout < *alarm_timeout_ms)
        {
            *alarm_timeout_ms = timeout;
        }
    }
    if (ret == RET_OK)
    {
        if (!tx_done)
        {
            ret = RET_BLOCKED;
        }
    }

    TRACE_MSG(TRACE_COMMON3, "<ncp_host_ll_quant ret %d rx %ld tx_done %d timeout %ld", (FMT__D_D_D_D, ret, *received_bytes, tx_done, *alarm_timeout_ms));

    return ret;
}
