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
/*  PURPOSE: NCP implementation of divider a packet to a fragments.
*/

#define ZB_TRACE_FILE_ID 27

#include "zbncp_types.h"
#include "zbncp_mem.h"
#include "zbncp_ll_pkt.h"
#include "zbncp.h"
#include "zbncp_frag_internal.h"


static zbncp_ll_tx_pkt_t zbncp_div_cut_piece(zbncp_div_ctx_t *div);


void zbncp_div_initialize(zbncp_div_ctx_t *div)
{
    div->offset = 0;
    div->pkt_size = 0;
    div->tx_pkt.mem = zbncp_make_cmemref(ZBNCP_NULL, 0);
    div->tx_pkt.flags = 0;
    zbncp_mem_fill(div->big_buf, ZBNCP_BIG_BUF_SIZE, 0);
}


zbncp_int32_t zbncp_div_store_tx_pkt(zbncp_div_ctx_t *div, const void *buf, zbncp_size_t size)
{
    zbncp_int32_t ret = 0;

    /*it's a normal situation when data for transmission is off */
    if ((buf != ZBNCP_NULL) && (size != 0u))
    {
        if (0u == div->pkt_size)
        {
            if (size <= ZBNCP_BIG_BUF_SIZE)
            {
                zbncp_mem_copy(div->big_buf, buf, size);
                div->pkt_size = size;
                div->tx_pkt = zbncp_div_cut_piece(div);
            }
            else
            {
                ret = ZBNCP_RET_ERROR;
            }
        }
        else
        {
            ret = ZBNCP_RET_BUSY;
        }
    }

    return ret;
}


zbncp_ll_tx_pkt_t zbncp_div_request_fragment(const zbncp_div_ctx_t *div)
{
    return div->tx_pkt;
}


static zbncp_ll_tx_pkt_t zbncp_div_cut_piece(zbncp_div_ctx_t *div)
{
    zbncp_cmemref_t cmem;
    zbncp_size_t fragment_size;
    zbncp_uint8_t flags = 0;

    if (div->pkt_size != 0u)
    {
        if ((div->offset + ZBNCP_LL_BODY_SIZE_MAX) < div->pkt_size)
        {
            /* Try to send _small_ packet first. It gives us a bit of economy when Host is waking up NCP by sending that packet.
               But do not create extra packets.
             */
            if (div->offset == 0u)
            {
                fragment_size = div->pkt_size % ZBNCP_LL_BODY_SIZE_MAX;
                if (fragment_size == 0u)
                {
                    fragment_size = ZBNCP_LL_BODY_SIZE_MAX;
                }
            }
            else
            {
                fragment_size = ZBNCP_LL_BODY_SIZE_MAX;
            }
        }
        else
        {
            fragment_size = div->pkt_size - div->offset;
            flags |= ZBNCP_LL_PKT_END << ZBNCP_LL_PKT_LIM_SHIFT;
        }

        if (0u == div->offset)
        {
            flags |= ZBNCP_LL_PKT_START << ZBNCP_LL_PKT_LIM_SHIFT;
        }

        cmem = zbncp_make_cmemref(div->big_buf + div->offset, fragment_size);
        div->offset += fragment_size;
    }
    else
    {
        cmem = zbncp_make_cmemref(ZBNCP_NULL, 0);
    }

    div->tx_pkt.mem = cmem;
    div->tx_pkt.flags = flags;

    return div->tx_pkt;
}


zbncp_bool_t zbncp_div_process_response(zbncp_div_ctx_t *div, const zbncp_ll_quant_res_t *rsp)
{
    if (rsp->txbytes != 0u)
    {
        if (div->offset == div->pkt_size)
        {
            zbncp_div_initialize(div);
        }
        else
        {
            div->tx_pkt = zbncp_div_cut_piece(div);
        }
    }

    return (zbncp_bool_t )(div->pkt_size == 0u);
}

