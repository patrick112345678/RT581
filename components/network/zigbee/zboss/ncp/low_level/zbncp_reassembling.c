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
/*  PURPOSE: NCP implementation of reassembling a packet from fragments.
*/

#define ZB_TRACE_FILE_ID 26

#include "zbncp_types.h"
#include "zbncp_mem.h"
#include "zbncp_ll_pkt.h"
#include "zbncp.h"
#include "zbncp_frag_internal.h"


static zbncp_memref_t zbncp_res_cut_place_for_fragment(zbncp_res_ctx_t *res);
static zbncp_size_t zbncp_res_store_fragment(zbncp_res_ctx_t *res, const zbncp_ll_rx_info_t *info);


void zbncp_res_initialize(zbncp_res_ctx_t *res)
{
    res->user_buf = ZBNCP_NULL;
    res->max_size = 0;
    res->offset = 0;
    res->mem = zbncp_res_cut_place_for_fragment(res);
    zbncp_mem_fill(res->big_buf, ZBNCP_BIG_BUF_SIZE, 0);
}


void zbncp_res_set_place_for_rx_pkt(zbncp_res_ctx_t *res, void *buf, zbncp_size_t size)
{
    res->user_buf = buf;
    res->max_size = size;
}


zbncp_memref_t zbncp_res_get_place_for_fragment(const zbncp_res_ctx_t *res)
{
    return res->mem;
}


static zbncp_memref_t zbncp_res_cut_place_for_fragment(zbncp_res_ctx_t *res)
{
    zbncp_size_t fragment_size;

    if (res->offset + ZBNCP_LL_BODY_SIZE_MAX < ZBNCP_BIG_BUF_SIZE)
    {
        fragment_size = ZBNCP_LL_BODY_SIZE_MAX;
    }
    else
    {
        fragment_size = ZBNCP_BIG_BUF_SIZE - res->offset;
    }

    return zbncp_make_memref(res->big_buf + res->offset, fragment_size);
}


static zbncp_size_t zbncp_res_store_fragment(zbncp_res_ctx_t *res, const zbncp_ll_rx_info_t *info)
{
    zbncp_size_t rx_bytes = 0;

    res->offset += info->rxbytes;
    if ((info->flags & (ZBNCP_LL_PKT_END << ZBNCP_LL_PKT_LIM_SHIFT)) != 0u)
    {
        if (res->user_buf != ZBNCP_NULL)
        {
            rx_bytes = size_min(res->offset, res->max_size);
            zbncp_mem_copy(res->user_buf, res->big_buf, rx_bytes);
            zbncp_res_initialize(res);
        }
        else
        {
            res->mem = zbncp_make_memref(ZBNCP_NULL, 0);
        }
    }
    else
    {
        res->mem = zbncp_res_cut_place_for_fragment(res);
    }

    return rx_bytes;
}


zbncp_size_t zbncp_res_process_response(zbncp_res_ctx_t *res, const zbncp_ll_rx_info_t *info)
{
    zbncp_size_t rx_bytes = 0;

    if (info->rxbytes != 0u)
    {
        if (res->offset != 0u)
        {
            if ((info->flags & (ZBNCP_LL_PKT_START << ZBNCP_LL_PKT_LIM_SHIFT)) != 0u)
            {
                /* Reset the reassembling logic.
                 * The first fragment was already received and the reassembly process was started.
                 * It is expected that a packet with the END bit or no positioning bits will be received next.
                 * If a packet with the START bit is received, it means that the other side aborted the
                 * previous transmission and started a new one.
                 * In such case, it is better to reset the logic and receive the next packet
                 * than ingore frames and stay in this state forever.
                 */
                zbncp_mem_move(res->big_buf, &res->big_buf[res->offset], info->rxbytes);
                res->offset = 0;
            }

            rx_bytes = zbncp_res_store_fragment(res, info);
        }
        else
        {
            /* waiting for the first fragment */
            if ((info->flags & (ZBNCP_LL_PKT_START << ZBNCP_LL_PKT_LIM_SHIFT)) != 0u)
            {
                rx_bytes = zbncp_res_store_fragment(res, info);
            }
            else
            {
                /* skip the fragment since we are waiting for the first fragment */
            }
        }
    }

    return rx_bytes;
}
