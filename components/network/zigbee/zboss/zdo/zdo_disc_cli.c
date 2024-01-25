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
/* PURPOSE: ZDO Discovery services - client side.
Mandatory calls onnly. Other calls will be implemented in some other project scope.
*/


#define ZB_TRACE_FILE_ID 2096
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_hash.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zdo_common.h"
#include "zb_ncp.h"

/*! \addtogroup ZB_ZDO */
/*! @{ */

zb_uint8_t zb_zdo_nwk_addr_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_zdo_nwk_addr_req_param_t *req_param = ZB_BUF_GET_PARAM(param, zb_zdo_nwk_addr_req_param_t);
    zb_zdo_nwk_addr_req_t *req;

    TRACE_MSG(TRACE_ZDO2, "zb_zdo_nwk_addr_req param %hd", (FMT__H, param));

    /* Verify the destination IEEE address. */
    if (!ZB_IEEE_ADDR_IS_VALID(req_param->ieee_addr))
    {
        TRACE_MSG(TRACE_ZDO1, "Invalid IEEE address param %hd", (FMT__H, param));
        return ZB_ZDO_INVALID_TSN;
    }

    req = zb_buf_initial_alloc(param, sizeof(zb_zdo_nwk_addr_req_t));
    ZB_HTOLE64(req->ieee_addr, req_param->ieee_addr);
    req->request_type = req_param->request_type;
    req->start_index = req_param->start_index;

    /* Sure Unicast: we need 1 answer for addr req */
    return zdo_send_req_by_short(ZDO_NWK_ADDR_REQ_CLID, param, cb, req_param->dst_addr, ZB_ZDO_CB_UNICAST_COUNTER);
}


zb_uint8_t zb_zdo_initiate_nwk_addr_req(zb_uint8_t param, zb_ieee_addr_t ieee_addr)
{
    zb_zdo_nwk_addr_req_param_t *req_param;

#ifdef ZB_REDUCE_NWK_LOAD_ON_LOW_MEMORY
    if (zb_buf_memory_close_to_low())
    {
        TRACE_MSG(TRACE_ERROR, "memory is close to low, do not discover nwk addr", (FMT__0));
        return ZB_ZDO_INVALID_TSN;
    }
#endif
    req_param = ZB_BUF_GET_PARAM(param, zb_zdo_nwk_addr_req_param_t);

    req_param->dst_addr = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
    ZB_IEEE_ADDR_COPY(req_param->ieee_addr, ieee_addr);
    req_param->request_type = ZB_ZDO_SINGLE_DEVICE_RESP;
    req_param->start_index = 0;

    return zb_zdo_nwk_addr_req(param, ZDO_CTX().app_addr_resp_cb);
}


static void zb_zdo_nwk_addr_req_2param_cb(zb_uint8_t param)
{
    zb_zdo_callback_info_t *resp_info = (zb_zdo_callback_info_t *)zb_buf_begin(param);
    zb_uint8_t              i = 0;
    zb_bool_t               is_found = ZB_FALSE;

    while (i < ZB_IOBUF_POOL_SIZE)
    {
        /* Optimization: skip 8 bits if value is 0. */
        if (ZDO_CTX().nwk_addr_req_pending_mask[i / 8U] == 0U)
        {
            i = (i / 8U + 1U) * 8U;
        }
        else
        {
            if (ZB_U2B(ZB_CHECK_BIT_IN_BIT_VECTOR(ZDO_CTX().nwk_addr_req_pending_mask, i)) &&
                    ZDO_CTX().nwk_addr_req_pending_tsns[i] == resp_info->tsn)
            {
                /* Release the entry. Not need to actually zero
                   nwk_addr_req_pending_tsns[i] */
                ZB_CLR_BIT_IN_BIT_VECTOR(ZDO_CTX().nwk_addr_req_pending_mask, i);
#ifdef APS_FRAGMENTATION
                /* DL: call aps_send_fail_confirm() or zb_apsde_data_request() will reach the 'i' freeing.
                   If the buffer remains in node_desc_req_pending_mask, then it can be seen
                   in zb_zdo_node_desc_req_2param_cb() and will be used after free */
                ZB_CLR_BIT_IN_BIT_VECTOR(ZDO_CTX().node_desc_req_pending_mask, i);
#endif

                if (resp_info->status == (zb_uint8_t)ZB_ZDP_STATUS_SUCCESS)
                {
                    /* Now we are sure that we have short addr. Lets send the pkt again. */
                    ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, i);
                }
                else
                {
                    /* Can not find the address, pass up confirm with fail status. */
                    aps_send_fail_confirm(i, RET_DEVICE_NOT_FOUND);
                }
#ifdef SNCP_MODE
                /* Call sncp_auto_turbo_poll_aps_tx() ONCE for tsn. is_found is used for this. */
                if (!is_found)
                {
                    sncp_auto_turbo_poll_aps_tx(ZB_FALSE);
                }
#endif
                is_found = ZB_TRUE;
            }
            ++i;
        }
    }

    zb_buf_free(param);
    ZB_ASSERT(is_found);
    ZVUNUSED(is_found);
}


void zb_zdo_initiate_nwk_addr_req_2param(zb_uint8_t param, zb_uint16_t user_param)
{
    /* Get ieee from user_cmd_buf. */
    zb_apsde_data_req_t *apsreq = ZB_BUF_GET_PARAM((zb_uint8_t)user_param, zb_apsde_data_req_t);
    zb_uint8_t           i;
    zb_uint8_t           req_is_sent_tsn = ZB_ZDO_INVALID_TSN;

    /* Check if we already have sent such request - if so, silently put the buf. */
    for (i = 1; i < ZB_IOBUF_POOL_SIZE; ++i)
    {
        /* If entry is used, check ieee */
        if (ZB_CHECK_BIT_IN_BIT_VECTOR(ZDO_CTX().nwk_addr_req_pending_mask, i))
        {
            zb_apsde_data_req_t *apsreq_pending = ZB_BUF_GET_PARAM(i,
                                                  zb_apsde_data_req_t);

            if (ZB_IEEE_ADDR_CMP(apsreq_pending->dst_addr.addr_long, apsreq->dst_addr.addr_long))
            {
                req_is_sent_tsn = ZDO_CTX().nwk_addr_req_pending_tsns[i];
                break;
            }
        }
    }

    /* Put entry to pending array - user_cmd_buf to nwk_addr_req_pending_mask, tsn to nwk_addr_req_pending_tsns. */
    ZB_ASSERT(!ZB_CHECK_BIT_IN_BIT_VECTOR(ZDO_CTX().nwk_addr_req_pending_mask, user_param));
    ZB_SET_BIT_IN_BIT_VECTOR(ZDO_CTX().nwk_addr_req_pending_mask, user_param);

    if (i == ZB_IOBUF_POOL_SIZE)
    {
        zb_zdo_nwk_addr_req_param_t *req_param = ZB_BUF_GET_PARAM(param, zb_zdo_nwk_addr_req_param_t);

        req_param->dst_addr = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
        ZB_IEEE_ADDR_COPY(req_param->ieee_addr, apsreq->dst_addr.addr_long);

        req_param->request_type = ZB_ZDO_SINGLE_DEVICE_RESP;
        req_param->start_index = 0;

        {
            zb_uint8_t nwk_addr_req_zdo_tsn;

            nwk_addr_req_zdo_tsn = zb_zdo_nwk_addr_req(param, zb_zdo_nwk_addr_req_2param_cb);

            if (nwk_addr_req_zdo_tsn != ZB_ZDO_INVALID_TSN)
            {
#ifdef SNCP_MODE
                /* Call sncp_auto_turbo_poll_aps_tx() for new tsn only */
                sncp_auto_turbo_poll_aps_tx(ZB_TRUE);
#endif
                ZDO_CTX().nwk_addr_req_pending_tsns[user_param] = nwk_addr_req_zdo_tsn;
            }
            else
            {
                zb_buf_free(param);
                ZB_CLR_BIT_IN_BIT_VECTOR(ZDO_CTX().nwk_addr_req_pending_mask, user_param);
#ifdef APS_FRAGMENTATION
                ZB_CLR_BIT_IN_BIT_VECTOR(ZDO_CTX().node_desc_req_pending_mask, user_param);
#endif
                aps_send_fail_confirm((zb_uint8_t)user_param, RET_DEVICE_NOT_FOUND);
            }
        }
    }
    else
    {
        zb_buf_free(param);
        ZB_ASSERT(req_is_sent_tsn != ZB_ZDO_INVALID_TSN);
        ZDO_CTX().nwk_addr_req_pending_tsns[user_param] = req_is_sent_tsn;
    }
}


#ifdef APS_FRAGMENTATION
static void zb_zdo_node_desc_req_2param_cb(zb_uint8_t param)
{
    zb_zdo_callback_info_t *resp_info;
    zb_uint8_t i = 1;
    zb_uint8_t tsn;
#ifdef SNCP_MODE
    zb_bool_t found_at_least_one = ZB_FALSE;
#endif

    TRACE_MSG(TRACE_ZDO2, "zb_zdo_node_desc_req_2param_cb param %hd", (FMT__H, param));
    resp_info = (zb_zdo_callback_info_t *)zb_buf_begin(param);

    tsn = resp_info->tsn;

    while (i < ZB_IOBUF_POOL_SIZE)
    {
        /* Optimization: skip 8 bits if value is 0. */
        if (ZDO_CTX().node_desc_req_pending_mask[i / 8U] == 0U)
        {
            i = (i / 8U + 1U) * 8U;
        }
        else
        {
            if (ZB_CHECK_BIT_IN_BIT_VECTOR(ZDO_CTX().node_desc_req_pending_mask, i))
            {
                zb_ret_t status = RET_NOT_FOUND;

                /* Iterate all pending buffers
                   by tsn, for any status. Need not check address at all. */
                if (ZDO_CTX().nwk_addr_req_pending_tsns[i] == tsn)
                {
                    if (resp_info->status == (zb_uint8_t)ZB_ZDP_STATUS_SUCCESS)
                    {
                        status = RET_OK;
                    }
                    else
                    {
                        status = RET_ERROR;
                    }
                }

                /* Process the entry. */
                if (status == RET_OK)
                {
                    /* Now we are sure that we have short addr. Lets send the pkt again. */
                    TRACE_MSG(TRACE_ZDO2, "call zb_apsde_data_request, %hd", (FMT__H, i));
                    ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, i);
                }
                else if (status == RET_ERROR)
                {
                    /* Can not find the address, pass up confirm with fail status. */
                    /* DL: FIXME: Why is this code copy-pasted from aps_send_fail_confirm() here?
                       Maybe just call aps_send_fail_confirm(i, RET_OPERATION_FAILED)? */
                    zb_bufid_t apsdu = (zb_bufid_t )i;
                    zb_apsde_data_req_t *apsreq = ZB_BUF_GET_PARAM(apsdu, zb_apsde_data_req_t);
                    zb_apsde_data_confirm_t apsde_data_conf;
                    TRACE_MSG(TRACE_ZDO2, "Can not find the address, pass up confirm with fail status.", (FMT__0));

                    ZB_BZERO(&apsde_data_conf, sizeof(zb_apsde_data_confirm_t));
                    apsde_data_conf.addr_mode = apsreq->addr_mode;
                    apsde_data_conf.src_endpoint = apsreq->src_endpoint;
                    apsde_data_conf.dst_endpoint = apsreq->dst_endpoint;
                    apsde_data_conf.dst_addr = apsreq->dst_addr;
                    apsde_data_conf.status = RET_OPERATION_FAILED;
                    zb_buf_set_status(apsdu, RET_OPERATION_FAILED);
                    /* Pkt was not passed through apsde_data_req. */
                    apsde_data_conf.need_unlock = ZB_FALSE;

                    ZB_MEMCPY(ZB_BUF_GET_PARAM(apsdu, zb_apsde_data_confirm_t), &apsde_data_conf, sizeof(zb_apsde_data_confirm_t));

                    ZB_SCHEDULE_CALLBACK(zb_apsde_data_confirm, i);
                }

                if (status != RET_NOT_FOUND)
                {
                    /* Release the entry. Not need to actually zero
                       nwk_addr_req_pending_tsns[i] */
                    ZB_CLR_BIT_IN_BIT_VECTOR(ZDO_CTX().node_desc_req_pending_mask, i);
                    ZB_CLR_BIT_IN_BIT_VECTOR(ZDO_CTX().nwk_addr_req_pending_mask, i);

#ifdef SNCP_MODE
                    if (!found_at_least_one)
                    {
                        /* Call sncp_auto_turbo_poll_aps_tx() for tsn once */
                        sncp_auto_turbo_poll_aps_tx(ZB_FALSE);
                        found_at_least_one = ZB_TRUE;
                    }
#endif
                }
            }
            ++i;
        }
    }

    zb_buf_free(param);
}

static zb_uint8_t zb_zdo_check_for_node_desc_req_in_pool(zb_uint16_t addr1_short, zb_uint8_t *req_is_sent_tsn)
{
    zb_uint8_t i = 1;
    /* Check if we already have sent such request - if so, silently put the buf. */
    /* 06/27/2017 EE CR:MINOR Buffer #0 can't be there */
    /* NK: Agree. Did not reduced pending_mask/pending_tsns - to do not increase complexity of code */
    for (; i < ZB_IOBUF_POOL_SIZE; ++i)
    {
        /* If entry is used, check ieee */
        if (ZB_CHECK_BIT_IN_BIT_VECTOR(ZDO_CTX().node_desc_req_pending_mask, i))
        {
            zb_bufid_t buf_pending = i;
            zb_apsde_data_req_t *apsreq_pending = ZB_BUF_GET_PARAM(buf_pending, zb_apsde_data_req_t);
            /* We definitely have short here, no matter what is the mode! */
            /*zb_uint16_t addr1_short = apsreq->dst_addr.addr_short;*/
            zb_addr_u addr2 = apsreq_pending->dst_addr;

            /* Try to match by short - we must have long/short addresses now, it is faster to compare
             * short addresses. */

            if (apsreq_pending->addr_mode == ZB_APS_ADDR_MODE_64_ENDP_PRESENT)
            {
                /* violates MISRA 19.1 overlapping objects, so use temporary variable */
                zb_uint16_t temp_short_addr = zb_address_short_by_ieee(addr2.addr_long);
                addr2.addr_short = temp_short_addr;
            }

            if (addr1_short == ZB_UNKNOWN_SHORT_ADDR || addr2.addr_short == ZB_UNKNOWN_SHORT_ADDR)
            {
                /* NK: Do assert (at least until debug will be finished). */
                /* Strange... We must have long/short now */
                TRACE_MSG(TRACE_ERROR, "short1 0x%x != short2 0x%x", (FMT__H_H, addr1_short, addr2.addr_short));
                ZB_ASSERT(0);
            }

            if (addr1_short == addr2.addr_short)
            {
                *req_is_sent_tsn = ZDO_CTX().nwk_addr_req_pending_tsns[i];
                break;
            }
        }
    }
    return i;
}


void zb_zdo_init_node_desc_req_2param(zb_uint8_t param, zb_uint16_t user_param)
{
    /* Get ieee from user_cmd_buf. */
    zb_apsde_data_req_t *apsreq = ZB_BUF_GET_PARAM((zb_uint8_t)user_param, zb_apsde_data_req_t);
    zb_uint8_t i;
    zb_uint8_t req_is_sent_tsn = 0;

    TRACE_MSG(TRACE_ZDO2, ">> zb_zdo_init_node_desc_req_2param param %hd user_param %hd", (FMT__H_H, param, user_param));

    i = zb_zdo_check_for_node_desc_req_in_pool(apsreq->dst_addr.addr_short, &req_is_sent_tsn);

    /* Put entry to pending array - user_cmd_buf to nwk_addr_req_pending_mask, tsn to nwk_addr_req_pending_tsns. */
    ZB_ASSERT(!ZB_CHECK_BIT_IN_BIT_VECTOR(ZDO_CTX().node_desc_req_pending_mask, user_param));
    ZB_SET_BIT_IN_BIT_VECTOR(ZDO_CTX().node_desc_req_pending_mask, user_param);

    /* tsn 0 is valid. check for i. */
    if (i == ZB_IOBUF_POOL_SIZE)
    {
        zb_zdo_node_desc_req_t *req = zb_buf_initial_alloc(param, sizeof(zb_zdo_node_desc_req_t));

        /* Get short addr of the device */
        /* We definitely have short here, no matter what is the mode! */
        req->nwk_addr = apsreq->dst_addr.addr_short;

        ZB_ASSERT(req->nwk_addr != ZB_UNKNOWN_SHORT_ADDR &&
                  !ZB_NWK_IS_ADDRESS_BROADCAST(req->nwk_addr));

        (void)zb_zdo_node_desc_req(param, zb_zdo_node_desc_req_2param_cb);

        /* Store current tsn - it was set in sync call so it is ok. */
        /* NK: tsn is correct there. In zdo_send_req():
           ...
           ZB_BUF_ALLOC_LEFT(ZB_BUF_FROM_REF(param), 1, tsn_p);
           ZDO_CTX().tsn++;
           *tsn_p = ZDO_CTX().tsn;
           ...
         */
        /* We never send node desc req until long/short pair is not known.
         * We can use single pending tsns array for both queries. */
        ZDO_CTX().nwk_addr_req_pending_tsns[user_param] = ZDO_CTX().tsn;

#ifdef SNCP_MODE
        sncp_auto_turbo_poll_aps_tx(ZB_TRUE);
#endif
    }
    else
    {
        zb_buf_free(param);
        ZDO_CTX().nwk_addr_req_pending_tsns[user_param] = req_is_sent_tsn;
    }
    TRACE_MSG(TRACE_ZDO2, "<< zb_zdo_init_node_desc_req_2param", (FMT__0));
}


static zb_callback_t zb_zdo_nd_user_cb;
static zb_uint16_t zb_zdo_nd_dst_addr;

static void zb_zdo_node_desc_req_direct_cb(zb_uint8_t param)
{
    if (zb_zdo_nd_user_cb != NULL)
    {
        zb_zdo_nd_user_cb(param);
    }

    /* zb_buf_free(param); */
}

static void zb_zdo_init_node_desc_req_dir(zb_uint8_t param)
{
    zb_zdo_node_desc_req_t *req;

    req = zb_buf_initial_alloc(param, sizeof(zb_zdo_node_desc_req_t));

    /* Get short addr of the device */
    /* We definitely have short here, no matter what is the mode! */
    req->nwk_addr = zb_zdo_nd_dst_addr;

    /* ZB_ASSERT(req->nwk_addr != ZB_UNKNOWN_SHORT_ADDR &&
              !ZB_NWK_IS_ADDRESS_BROADCAST(req->nwk_addr)); */

    (void)zb_zdo_node_desc_req(param, zb_zdo_node_desc_req_direct_cb);

}

zb_ret_t zb_zdo_init_node_desc_req_direct(zb_uint16_t addr, zb_callback_t user_cb)
{
    zb_ret_t ret;

    ZB_ASSERT(addr != ZB_UNKNOWN_SHORT_ADDR && !ZB_NWK_IS_ADDRESS_BROADCAST(addr));

    zb_zdo_nd_user_cb = user_cb;
    zb_zdo_nd_dst_addr = addr;

    ret = zb_buf_get_out_delayed(zb_zdo_init_node_desc_req_dir);
    if (ret != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
    }

    return ret;
}
#endif  /* APS_FRAGMENTATION */

zb_uint8_t zb_zdo_ieee_addr_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_zdo_ieee_addr_req_param_t req_param = *ZB_BUF_GET_PARAM(param, zb_zdo_ieee_addr_req_param_t);
    zb_zdo_ieee_addr_req_t *req;

    TRACE_MSG(TRACE_ZDO2, "zb_zdo_ieee_addr_req param %hd", (FMT__H, param));

    req = zb_buf_initial_alloc(param, sizeof(zb_zdo_ieee_addr_req_t));

    ZB_HTOLE16((zb_uint8_t *)&req->nwk_addr, &req_param.nwk_addr);
    req->request_type = req_param.request_type;
    req->start_index = req_param.start_index;

    /* Sure Unicast: we need 1 answer for addr req */
    return zdo_send_req_by_short(ZDO_IEEE_ADDR_REQ_CLID, param, cb, req_param.dst_addr, ZB_ZDO_CB_UNICAST_COUNTER);
}

zb_uint8_t zb_zdo_initiate_ieee_addr_req(zb_uint8_t param, zb_uint16_t nwk_addr)
{
    zb_zdo_ieee_addr_req_param_t *req_param;

#ifdef ZB_REDUCE_NWK_LOAD_ON_LOW_MEMORY
    if (zb_buf_memory_close_to_low())
    {
        TRACE_MSG(TRACE_ERROR, "memory is close to low, do not discover ieee addr", (FMT__0));
        return ZB_ZDO_INVALID_TSN;
    }
#endif
    req_param = ZB_BUF_GET_PARAM(param, zb_zdo_ieee_addr_req_param_t);

    req_param->dst_addr = nwk_addr;
    req_param->nwk_addr = nwk_addr;
    req_param->request_type = ZB_ZDO_SINGLE_DEV_RESPONSE;
    req_param->start_index = 0;

    return zb_zdo_ieee_addr_req(param, ZDO_CTX().app_addr_resp_cb);
}

zb_uint8_t zb_zdo_initiate_ieee_addr_req_with_cb(zb_uint8_t param, zb_uint16_t nwk_addr, zb_callback_t cb)
{
    zb_zdo_ieee_addr_req_param_t *req_param;

#ifdef ZB_REDUCE_NWK_LOAD_ON_LOW_MEMORY
    if (zb_buf_memory_close_to_low())
    {
        TRACE_MSG(TRACE_ERROR, "memory is close to low, do not discover ieee addr", (FMT__0));
        return ZB_ZDO_INVALID_TSN;
    }
#endif
    req_param = ZB_BUF_GET_PARAM(param, zb_zdo_ieee_addr_req_param_t);

    req_param->dst_addr = nwk_addr;
    req_param->nwk_addr = nwk_addr;
    req_param->request_type = ZB_ZDO_SINGLE_DEV_RESPONSE;
    req_param->start_index = 0;

    return zb_zdo_ieee_addr_req(param, cb);
}

zb_uint8_t zb_zdo_node_desc_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_zdo_node_desc_req_t *req = (zb_zdo_node_desc_req_t *)zb_buf_begin(param);
    zb_uint16_t addr = req->nwk_addr;
    ZB_HTOLE16((zb_uint8_t *)&req->nwk_addr, &addr);

    return zdo_send_req_by_short(ZDO_NODE_DESC_REQ_CLID, param, cb, addr, ZB_ZDO_CB_DEFAULT_COUNTER /* ZB_ZDO_CB_UNICAST_COUNTER ? */);
}


zb_uint8_t zb_zdo_power_desc_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_zdo_power_desc_req_t *req = (zb_zdo_power_desc_req_t *)zb_buf_begin(param);
    zb_uint16_t addr = req->nwk_addr;
    ZB_HTOLE16((zb_uint8_t *)&req->nwk_addr, &addr);
    return zdo_send_req_by_short(ZDO_POWER_DESC_REQ_CLID, param, cb, addr, ZB_ZDO_CB_DEFAULT_COUNTER);
}


zb_uint8_t zb_zdo_simple_desc_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_zdo_simple_desc_req_t *req = (zb_zdo_simple_desc_req_t *)zb_buf_begin(param);
    zb_uint16_t addr = req->nwk_addr;
    ZB_HTOLE16((zb_uint8_t *)&req->nwk_addr, &addr);
    return zdo_send_req_by_short(ZDO_SIMPLE_DESC_REQ_CLID, param, cb, addr, ZB_ZDO_CB_DEFAULT_COUNTER);
}


zb_uint8_t zb_zdo_active_ep_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_zdo_active_ep_req_t *req = (zb_zdo_active_ep_req_t *)zb_buf_begin(param);
    zb_uint16_t addr = req->nwk_addr;
    ZB_HTOLE16((zb_uint8_t *)&req->nwk_addr, &addr);
    return zdo_send_req_by_short(ZDO_ACTIVE_EP_REQ_CLID, param, cb, addr, ZB_ZDO_CB_DEFAULT_COUNTER);
}


zb_uint8_t zb_zdo_match_desc_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_zdo_match_desc_req_head_t *req_head;
    zb_zdo_match_desc_req_tail_t *req_tail;
    zb_zdo_match_desc_param_t *match_param;
    zb_zdo_match_desc_param_t *match_param_init;
    zb_size_t num_in_out_clusters;
    zb_uint8_t *req_cluster_id;
    zb_uint8_t *cluster_id;
    zb_uint_t i;
    zb_uint8_t zdo_tsn;

    TRACE_MSG(TRACE_ZDO3, ">>zb_zdo_match_desc_req param %hd", (FMT__D, param));

    match_param_init = (zb_zdo_match_desc_param_t *)zb_buf_begin(param);
    num_in_out_clusters = sizeof(zb_zdo_match_desc_param_t);
    num_in_out_clusters += ((zb_size_t)match_param_init->num_in_clusters + (zb_size_t)match_param_init->num_out_clusters) * sizeof(zb_uint16_t);
    match_param = zb_buf_get_tail(param, num_in_out_clusters);
    ZB_MEMCPY(match_param, match_param_init, num_in_out_clusters);

    req_head = zb_buf_initial_alloc(param, sizeof(zb_zdo_match_desc_req_head_t) + match_param->num_in_clusters * sizeof(zb_uint16_t) +
                                    sizeof(zb_zdo_match_desc_req_tail_t) + match_param->num_out_clusters * sizeof(zb_uint16_t));

    ZB_HTOLE16((zb_uint8_t *)&req_head->nwk_addr, (zb_uint8_t *)&match_param->addr_of_interest);
    ZB_HTOLE16((zb_uint8_t *)&req_head->profile_id, (zb_uint8_t *)&match_param->profile_id);
    req_head->num_in_clusters = match_param->num_in_clusters;

    cluster_id = (zb_uint8_t *)match_param->cluster_list;
    /* req_cluster_id was pointer to 2-bytes integer, but need ta have it char*
     * to shutup ARM compiler. Really, there is no unaligned access. */
    req_cluster_id = (zb_uint8_t *)(req_head + 1);
    for (i = 0; i < match_param->num_in_clusters; i++)
    {
        ZB_HTOLE16(req_cluster_id, cluster_id);
        req_cluster_id += sizeof(zb_uint16_t);
        cluster_id += sizeof(zb_uint16_t);
    }
    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,73} */
    req_tail = (zb_zdo_match_desc_req_tail_t *)req_cluster_id;
    req_tail->num_out_clusters = match_param->num_out_clusters;
    req_cluster_id = (zb_uint8_t *)(req_tail + 1);
    for (i = 0; i < match_param->num_out_clusters; i++)
    {
        ZB_HTOLE16(req_cluster_id, cluster_id);
        req_cluster_id += sizeof(zb_uint16_t);
        cluster_id += sizeof(zb_uint16_t);
    }

    TRACE_MSG(TRACE_ZDO3, "addr 0x%x, profile id 0x%x, num in cl %hd, num out cl %hd",
              (FMT__H_H_H_H, req_head->nwk_addr, req_head->profile_id, req_head->num_in_clusters, req_tail->num_out_clusters));

    zdo_tsn = zdo_send_req_by_short(ZDO_MATCH_DESC_REQ_CLID, param, cb, match_param->nwk_addr,
                                    ZB_NWK_IS_ADDRESS_BROADCAST(match_param->nwk_addr) ? ZB_ZDO_CB_BROADCAST_COUNTER : ZB_ZDO_CB_UNICAST_COUNTER);

    TRACE_MSG(TRACE_ZDO3, "<<zb_zdo_match_desc_req", (FMT__0));
    return zdo_tsn;
}

/*! @} */
