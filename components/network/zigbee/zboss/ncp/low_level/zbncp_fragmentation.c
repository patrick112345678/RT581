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
/*  PURPOSE: NCP implementation of interface part of fragmentation.
*/

#define ZB_TRACE_FILE_ID 32

#include "zbncp_types.h"
#include "zbncp_mem.h"
#include "zbncp_ll_pkt.h"
#include "zbncp.h"
#include "zbncp_fragmentation.h"
#include "zbncp_frag_internal.h"


void zbncp_frag_initialize(zbncp_frag_ctx_t *frag_ctx)
{
    zbncp_res_initialize(&frag_ctx->res);
    zbncp_div_initialize(&frag_ctx->div);
}


zbncp_int32_t zbncp_frag_store_tx_pkt(zbncp_frag_ctx_t *frag_ctx, const void *buf, zbncp_size_t size)
{
    return zbncp_div_store_tx_pkt(&frag_ctx->div, buf, size);
}


void zbncp_frag_set_place_for_rx_pkt(zbncp_frag_ctx_t *frag_ctx, void *buf, zbncp_size_t size)
{
    zbncp_res_set_place_for_rx_pkt(&frag_ctx->res, buf, size);
}


void zbncp_frag_fill_request(const zbncp_frag_ctx_t *frag_ctx, zbncp_ll_quant_req_t *req)
{
    req->rxmem = zbncp_res_get_place_for_fragment(&frag_ctx->res);
    req->tx_pkt = zbncp_div_request_fragment(&frag_ctx->div);
}


zbncp_bool_t zbncp_frag_process_tx_response(zbncp_frag_ctx_t *frag_ctx, const zbncp_ll_quant_res_t *rsp)
{
    return zbncp_div_process_response(&frag_ctx->div, rsp);
}


zbncp_size_t zbncp_frag_process_rx_response(zbncp_frag_ctx_t *frag_ctx, const zbncp_ll_quant_res_t *rsp)
{
    return zbncp_res_process_response(&frag_ctx->res, &rsp->rx_info);
}
