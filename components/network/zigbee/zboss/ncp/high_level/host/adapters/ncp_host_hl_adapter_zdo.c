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
/*  PURPOSE: NCP High level transport (adapters layer) implementation for the host side: ZDO category
*/
#define ZB_TRACE_FILE_ID 17511

#include "ncp_host_hl_transport_internal_api.h"
#include "ncp_host_soc_state.h"

#define ADAPTER_ZDO_TRAN_TABLE_SIZE ZDO_TRAN_TABLE_SIZE

typedef ZB_PACKED_PRE struct ncp_host_zdo_cb_ent_s
{
    void *cb;
    zb_uint8_t    zdp_tsn; /* ZDP Transaction sequence number */
    zb_uint8_t    ncp_tsn; /* NCP Transaction sequence number */
    zb_bufid_t    buffer;  /* Request buffer to use in response handler */
    zb_bool_t     wait_for_timeout; /* if ZB_TRUE, then we free a cb_ent when a response
                                     with ZB_ZDP_STATUS_TIMEOUT is received */
} ZB_PACKED_STRUCT ncp_host_zdo_cb_ent_t;

typedef ZB_PACKED_PRE struct ncp_host_zdo_stats_s
{
    zb_uint16_t packet_buffer_allocate_failures; /*Number of buffer allocation errors*/
} ZB_PACKED_STRUCT ncp_host_zdo_stats_t;


typedef ZB_PACKED_PRE struct ncp_host_zdo_adapter_ctx_s
{
    ncp_host_zdo_stats_t stats; /* ZDO stats for Diagnostic cluster */
    ncp_host_zdo_cb_ent_t zdo_cb[ADAPTER_ZDO_TRAN_TABLE_SIZE];
    zb_uint8_t zdp_tsn; /* ZDP Transaction sequence number */
} ZB_PACKED_STRUCT ncp_host_zdo_adapter_ctx_t;

ncp_host_zdo_adapter_ctx_t g_ncp_host_zdo_adapter_ctx;

static void ncp_host_zdo_cb_reset(void)
{
    zb_uint8_t i;
    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_zdo_cb_reset", (FMT__0));

    for (i = 0; i < ADAPTER_ZDO_TRAN_TABLE_SIZE; i++)
    {
        g_ncp_host_zdo_adapter_ctx.zdo_cb[i].cb = NULL;
        g_ncp_host_zdo_adapter_ctx.zdo_cb[i].zdp_tsn = ZB_ZDO_INVALID_TSN;
        g_ncp_host_zdo_adapter_ctx.zdo_cb[i].ncp_tsn = ZB_ZDO_INVALID_TSN;
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_zdo_cb_reset", (FMT__0));
}

void ncp_host_zdo_adapter_init_ctx(void)
{
    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_zdo_adapter_init_ctx", (FMT__0));

    g_ncp_host_zdo_adapter_ctx.zdp_tsn = 0;
    ncp_host_zdo_cb_reset();

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_zdo_adapter_init_ctx", (FMT__0));
}

static void increment_zdp_tsn(void)
{
    g_ncp_host_zdo_adapter_ctx.zdp_tsn++;
    if (g_ncp_host_zdo_adapter_ctx.zdp_tsn == ZB_ZDO_INVALID_TSN)
    {
        g_ncp_host_zdo_adapter_ctx.zdp_tsn = 0;
    }
}

static void decrement_zdp_tsn(void)
{
    g_ncp_host_zdo_adapter_ctx.zdp_tsn--;
    if (g_ncp_host_zdo_adapter_ctx.zdp_tsn == ZB_ZDO_INVALID_TSN)
    {
        g_ncp_host_zdo_adapter_ctx.zdp_tsn = 0xFEu;
    }
}

static void get_cb_ent_by_ncp_tsn(zb_uint8_t ncp_tsn, ncp_host_zdo_cb_ent_t **cb_ent)
{
    zb_uint8_t i;

    *cb_ent = NULL;

    for (i = 0; i < ADAPTER_ZDO_TRAN_TABLE_SIZE; i++)
    {
        if (g_ncp_host_zdo_adapter_ctx.zdo_cb[i].ncp_tsn == ncp_tsn)
        {
            *cb_ent = &g_ncp_host_zdo_adapter_ctx.zdo_cb[i];
        }
    }
}

static zb_uint8_t ncp_host_zdo_adapter_register_request(void *cb,
        zb_bool_t wait_for_timeout,
        ncp_host_zdo_cb_ent_t **cb_ent)
{
    ncp_host_zdo_cb_ent_t *curr_ent = NULL;
    zb_uint8_t zdp_tsn;
    zb_uint8_t i = 0;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_zdo_adapter_register_request, cb %p",
              (FMT__P, cb));

    zdp_tsn = g_ncp_host_zdo_adapter_ctx.zdp_tsn;
    increment_zdp_tsn();

    if (cb != NULL)
    {
        for (i = 0; i < ADAPTER_ZDO_TRAN_TABLE_SIZE; i++)
        {
            curr_ent = &g_ncp_host_zdo_adapter_ctx.zdo_cb[i];

            if (curr_ent->cb == NULL
                    && curr_ent->zdp_tsn == ZB_ZDO_INVALID_TSN
                    && curr_ent->ncp_tsn == NCP_TSN_RESERVED)
            {
                curr_ent->cb = cb;
                curr_ent->zdp_tsn = zdp_tsn;
                curr_ent->wait_for_timeout = wait_for_timeout;
                *cb_ent = curr_ent;

                TRACE_MSG(TRACE_TRANSPORT2, "set zdo_cb entry in ncp_host_zdo_adapter_ctx to %p by %hd index",
                          (FMT__P_H, cb, i));

                break;
            }
        }
        if (i == ADAPTER_ZDO_TRAN_TABLE_SIZE)
        {
            TRACE_MSG(TRACE_ERROR, "No free space for ZDO cb in ZDO adapters, zdp_tsn 0x%hx",
                      (FMT__H, zdp_tsn));
            zdp_tsn = ZB_ZDO_INVALID_TSN;
            decrement_zdp_tsn();
        }
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_zdo_adapter_register_request, zdp_tsn %hd",
              (FMT__H, zdp_tsn));
    return zdp_tsn;
}


static void ncp_host_zdo_adapter_update_cb_ent(ncp_host_zdo_cb_ent_t *cb_ent,
        zb_bufid_t buf,
        zb_uint8_t ncp_tsn)
{
    if (cb_ent != NULL)
    {
        cb_ent->ncp_tsn = ncp_tsn;
        cb_ent->buffer = buf;
    }
}

static void ncp_host_zdo_adapter_clean_cb_ent(ncp_host_zdo_cb_ent_t *cb_ent)
{
    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_zdo_adapter_clean_cb_ent", (FMT__0));

    if (cb_ent != NULL)
    {
        cb_ent->cb = NULL;
        cb_ent->zdp_tsn = ZB_ZDO_INVALID_TSN;
        cb_ent->ncp_tsn = NCP_TSN_RESERVED;
        cb_ent->buffer = ZB_BUF_INVALID;
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_zdo_adapter_clean_cb_ent", (FMT__0));
}

static void ncp_host_zdo_adapter_call_cb(zb_bufid_t buf,
        zb_zdp_status_t status,
        ncp_host_zdo_cb_ent_t *cb_ent)
{
    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_zdo_adapter_call_cb_by_for_blocking_req", (FMT__0));

    if (cb_ent != NULL)
    {
        if (cb_ent->cb)
        {
            zb_callback_t callback = cb_ent->cb;
            callback(buf);
        }

        if (!cb_ent->wait_for_timeout || status == ZB_ZDP_STATUS_TIMEOUT)
        {
            ncp_host_zdo_adapter_clean_cb_ent(cb_ent);
        }
    }
    else
    {
        zb_buf_free(buf);
    }

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_zdo_adapter_call_cb_by_for_blocking_req", (FMT__0));
}

zb_uint8_t zb_zdo_ieee_addr_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_ret_t ret;
    zb_uint8_t zdp_tsn;
    zb_uint8_t ncp_tsn;
    zb_zdo_ieee_addr_req_param_t *req_param = ZB_BUF_GET_PARAM(param, zb_zdo_ieee_addr_req_param_t);
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_zdo_ieee_addr_req", (FMT__0));

    zdp_tsn = ncp_host_zdo_adapter_register_request(cb, ZB_FALSE, &cb_ent);

    if (zdp_tsn != ZB_ZDO_INVALID_TSN)
    {
        ret = ncp_host_zdo_ieee_addr_request(&ncp_tsn, req_param);
        ZB_ASSERT(ret == RET_OK);
        ncp_host_zdo_adapter_update_cb_ent(cb_ent, ZB_BUF_INVALID, ncp_tsn);
    }

    zb_buf_free(param);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_zdo_ieee_addr_req", (FMT__0));

    return zdp_tsn;
}

zb_uint8_t zdo_mgmt_leave_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_ret_t ret = RET_BUSY;
    zb_uint8_t zdp_tsn;
    zb_uint8_t ncp_tsn;
    zb_zdo_mgmt_leave_param_t *req_param = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_leave_param_t);
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zdo_mgmt_leave_req", (FMT__0));

    zdp_tsn = ncp_host_zdo_adapter_register_request(cb, ZB_FALSE, &cb_ent);

    if (zdp_tsn != ZB_ZDO_INVALID_TSN)
    {
        ret = ncp_host_zdo_mgmt_leave_request(&ncp_tsn, req_param);
        ZB_ASSERT(ret == RET_OK);
        ncp_host_zdo_adapter_update_cb_ent(cb_ent, ZB_BUF_INVALID, ncp_tsn);
    }

    zb_buf_free(param);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zdo_mgmt_leave_req", (FMT__0));

    return zdp_tsn;
}

zb_uint8_t zb_zdo_nwk_addr_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_ret_t ret;
    zb_uint8_t zdp_tsn;
    zb_uint8_t ncp_tsn;
    zb_zdo_nwk_addr_req_param_t *req_param = ZB_BUF_GET_PARAM(param, zb_zdo_nwk_addr_req_param_t);
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_zdo_nwk_addr_request_adapter", (FMT__0));

    zdp_tsn = ncp_host_zdo_adapter_register_request(cb, ZB_FALSE, &cb_ent);

    if (zdp_tsn != ZB_ZDO_INVALID_TSN)
    {
        ret = ncp_host_zdo_nwk_addr_request(&ncp_tsn, req_param);
        ZB_ASSERT(ret == RET_OK);
        ncp_host_zdo_adapter_update_cb_ent(cb_ent, ZB_BUF_INVALID, ncp_tsn);
    }

    zb_buf_free(param);

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_zdo_nwk_addr_request_adapter", (FMT__0));

    return zdp_tsn;
}

zb_uint8_t zb_zdo_power_desc_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_ret_t ret;
    zb_uint8_t zdp_tsn;
    zb_uint8_t ncp_tsn;
    zb_zdo_power_desc_req_t *req = (zb_zdo_power_desc_req_t *)zb_buf_begin(param);
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_zdo_power_desc_req", (FMT__0));

    zdp_tsn = ncp_host_zdo_adapter_register_request(cb,
              ZB_NWK_IS_ADDRESS_BROADCAST(req->nwk_addr),
              &cb_ent);

    if (zdp_tsn != ZB_ZDO_INVALID_TSN)
    {
        ret = ncp_host_zdo_power_desc_request(&ncp_tsn, req->nwk_addr);
        ZB_ASSERT(ret == RET_OK);
        ncp_host_zdo_adapter_update_cb_ent(cb_ent, ZB_BUF_INVALID, ncp_tsn);
    }

    zb_buf_free(param);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_zdo_power_desc_req", (FMT__0));

    return zdp_tsn;
}

zb_uint8_t zb_zdo_node_desc_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_ret_t ret;
    zb_uint8_t zdp_tsn;
    zb_uint8_t ncp_tsn;
    zb_zdo_node_desc_req_t *req = (zb_zdo_node_desc_req_t *)zb_buf_begin(param);
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_zdo_node_desc_req_adapter", (FMT__0));

    zdp_tsn = ncp_host_zdo_adapter_register_request(cb,
              ZB_NWK_IS_ADDRESS_BROADCAST(req->nwk_addr),
              &cb_ent);

    if (zdp_tsn != ZB_ZDO_INVALID_TSN)
    {
        ret = ncp_host_zdo_node_desc_request(&ncp_tsn, req->nwk_addr);
        ZB_ASSERT(ret == RET_OK);
        ncp_host_zdo_adapter_update_cb_ent(cb_ent, ZB_BUF_INVALID, ncp_tsn);
    }

    zb_buf_free(param);

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_zdo_node_desc_req_adapter", (FMT__0));

    return zdp_tsn;
}

zb_uint8_t zb_zdo_simple_desc_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_ret_t ret;
    zb_uint8_t zdp_tsn;
    zb_uint8_t ncp_tsn;
    zb_zdo_simple_desc_req_t *req = (zb_zdo_simple_desc_req_t *)zb_buf_begin(param);
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_zdo_simple_desc_req", (FMT__0));

    zdp_tsn = ncp_host_zdo_adapter_register_request(cb,
              ZB_NWK_IS_ADDRESS_BROADCAST(req->nwk_addr),
              &cb_ent);

    if (zdp_tsn != ZB_ZDO_INVALID_TSN)
    {
        ret = ncp_host_zdo_simple_desc_request(&ncp_tsn, req->nwk_addr, req->endpoint);
        ZB_ASSERT(ret == RET_OK);
        ncp_host_zdo_adapter_update_cb_ent(cb_ent, ZB_BUF_INVALID, ncp_tsn);
    }

    zb_buf_free(param);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_zdo_simple_desc_req", (FMT__0));

    return zdp_tsn;
}

zb_uint8_t zb_zdo_active_ep_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_ret_t ret;
    zb_uint8_t zdp_tsn;
    zb_uint8_t ncp_tsn;
    zb_zdo_active_ep_req_t *req = (zb_zdo_active_ep_req_t *)zb_buf_begin(param);
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_zdo_active_ep_req", (FMT__0));

    zdp_tsn = ncp_host_zdo_adapter_register_request(cb,
              ZB_NWK_IS_ADDRESS_BROADCAST(req->nwk_addr),
              &cb_ent);

    if (zdp_tsn != ZB_ZDO_INVALID_TSN)
    {
        ret = ncp_host_zdo_active_ep_request(&ncp_tsn, req->nwk_addr);
        ZB_ASSERT(ret == RET_OK);
        ncp_host_zdo_adapter_update_cb_ent(cb_ent, ZB_BUF_INVALID, ncp_tsn);
    }

    zb_buf_free(param);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_zdo_active_ep_req", (FMT__0));

    return zdp_tsn;
}

zb_uint8_t zb_zdo_match_desc_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_ret_t ret;
    zb_uint8_t zdp_tsn;
    zb_uint8_t ncp_tsn;
    zb_zdo_match_desc_param_t *req = (zb_zdo_match_desc_param_t *)zb_buf_begin(param);
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_zdo_match_desc_req", (FMT__0));

    zdp_tsn = ncp_host_zdo_adapter_register_request(cb,
              ZB_NWK_IS_ADDRESS_BROADCAST(req->nwk_addr),
              &cb_ent);

    if (zdp_tsn != ZB_ZDO_INVALID_TSN)
    {
        ret = ncp_host_zdo_match_desc_request(&ncp_tsn, req);
        ZB_ASSERT(ret == RET_OK);
        ncp_host_zdo_adapter_update_cb_ent(cb_ent, ZB_BUF_INVALID, ncp_tsn);
    }

    zb_buf_free(param);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_zdo_match_desc_req", (FMT__0));

    return zdp_tsn;
}

zb_uint8_t zb_zdo_bind_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_ret_t ret;
    zb_uint8_t zdp_tsn;
    zb_uint8_t ncp_tsn;
    zb_zdo_bind_req_param_t *bind_param = ZB_BUF_GET_PARAM(param, zb_zdo_bind_req_param_t);
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_zdo_bind_req", (FMT__0));

    zdp_tsn = ncp_host_zdo_adapter_register_request(cb,
              ZB_NWK_IS_ADDRESS_BROADCAST(bind_param->req_dst_addr),
              &cb_ent);

    if (zdp_tsn != ZB_ZDO_INVALID_TSN)
    {
        ret = ncp_host_zdo_bind_unbind_request(&ncp_tsn, bind_param, ZB_TRUE);
        ZB_ASSERT(ret == RET_OK);
        ncp_host_zdo_adapter_update_cb_ent(cb_ent, ZB_BUF_INVALID, ncp_tsn);
    }

    zb_buf_free(param);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_zdo_bind_req", (FMT__0));

    return zdp_tsn;
}

zb_uint8_t zb_zdo_unbind_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_ret_t ret;
    zb_uint8_t zdp_tsn;
    zb_uint8_t ncp_tsn;
    zb_zdo_bind_req_param_t *bind_param = ZB_BUF_GET_PARAM(param, zb_zdo_bind_req_param_t);
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_zdo_unbind_req", (FMT__0));

    zdp_tsn = ncp_host_zdo_adapter_register_request(cb,
              ZB_NWK_IS_ADDRESS_BROADCAST(bind_param->req_dst_addr),
              &cb_ent);

    if (zdp_tsn != ZB_ZDO_INVALID_TSN)
    {
        ret = ncp_host_zdo_bind_unbind_request(&ncp_tsn, bind_param, ZB_FALSE);
        ZB_ASSERT(ret == RET_OK);
        ncp_host_zdo_adapter_update_cb_ent(cb_ent, ZB_BUF_INVALID, ncp_tsn);
    }

    zb_buf_free(param);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_zdo_unbind_req", (FMT__0));

    return zdp_tsn;
}

zb_uint8_t zb_zdo_mgmt_permit_joining_req(zb_bufid_t param, zb_callback_t cb)
{
    zb_ret_t ret;
    zb_uint8_t zdp_tsn;
    zb_uint8_t ncp_tsn;
    zb_zdo_mgmt_permit_joining_req_param_t *req_param
        = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_permit_joining_req_param_t);
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_zdo_mgmt_permit_joining_req", (FMT__0));

    zdp_tsn = ncp_host_zdo_adapter_register_request(cb, ZB_FALSE, &cb_ent);

    if (zdp_tsn != ZB_ZDO_INVALID_TSN)
    {
        ret = ncp_host_zdo_permit_joining_request(&ncp_tsn, req_param->dest_addr,
                req_param->permit_duration,
                req_param->tc_significance);
        ZB_ASSERT(ret == RET_OK);
        ncp_host_zdo_adapter_update_cb_ent(cb_ent, ZB_BUF_INVALID, ncp_tsn);
    }

    zb_buf_free(param);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_zdo_mgmt_permit_joining_req", (FMT__0));

    return zdp_tsn;
}

zb_uint8_t zb_zdo_system_server_discovery_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_ret_t ret;
    zb_uint8_t zdp_tsn;
    zb_uint8_t ncp_tsn;
    zb_zdo_system_server_discovery_param_t *req_param =
        ZB_BUF_GET_PARAM(param, zb_zdo_system_server_discovery_param_t);
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_zdo_system_server_discovery_req", (FMT__0));

    /* there could be multiple responses but we can't handle them for now,
       SoC will never send a response with a timeout status so we will have a cb_ent leak.
       Take a look at how zb_zdo_system_server_discovery_req is implemented internally...
       TODO: fix zb_zdo_system_server_discovery_req internal implementation */
    zdp_tsn = ncp_host_zdo_adapter_register_request(cb, ZB_FALSE, &cb_ent);

    if (zdp_tsn != ZB_ZDO_INVALID_TSN)
    {
        ret = ncp_host_zdo_system_server_discovery_request(&ncp_tsn, req_param->server_mask);
        ZB_ASSERT(ret == RET_OK);
        ncp_host_zdo_adapter_update_cb_ent(cb_ent, ZB_BUF_INVALID, ncp_tsn);
    }

    zb_buf_free(param);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_zdo_system_server_discovery_req", (FMT__0));

    return zdp_tsn;
}


zb_ret_t zdo_diagnostics_get_stats(zb_callback_t cb, zb_uint8_t pib_attr)
{
    zb_ret_t ret = RET_BUSY;
    zb_uint8_t zdp_tsn;
    zb_uint8_t ncp_tsn;
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zdo_diagnostics_get_stats", (FMT__0));

    zdp_tsn = ncp_host_zdo_adapter_register_request(cb, ZB_FALSE, &cb_ent);

    if (zdp_tsn != ZB_ZDO_INVALID_TSN)
    {
        ret = ncp_host_zdo_diagnostics_get_stats_request(&ncp_tsn, pib_attr);
        ZB_ASSERT(ret == RET_OK);
        ncp_host_zdo_adapter_update_cb_ent(cb_ent, ZB_BUF_INVALID, ncp_tsn);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< zdo_diagnostics_get_stats", (FMT__0));

    return ret;
}


void zdo_diagnostics_inc(zdo_diagnostics_counter_id_t counter_id)
{
    TRACE_MSG(TRACE_ZDO3, ">> zdo_diagnostics_inc, counter_id %hd", (FMT__H, counter_id));

    switch (counter_id)
    {
    case ZDO_DIAGNOSTICS_PACKET_BUFFER_ALLOCATE_FAILURES_ID:
        ++g_ncp_host_zdo_adapter_ctx.stats.packet_buffer_allocate_failures;
        break;

    default:
        TRACE_MSG(TRACE_ZDO3, "counter_id %hd is not available on NCP host!", (FMT__H, counter_id));
        ZB_ASSERT(ZB_FALSE);
        break;
    }

    TRACE_MSG(TRACE_ZDO3, "<< zdo_diagnostics_inc", (FMT__0));
}


void ncp_host_handle_zdo_ieee_addr_response(zb_ret_t status, zb_uint8_t ncp_tsn,
        zb_zdo_ieee_addr_resp_t *resp,
        zb_bool_t is_extended,
        zb_zdo_ieee_addr_resp_ext_t *resp_ext,
        zb_zdo_ieee_addr_resp_ext2_t *resp_ext2,
        zb_uint16_t *addr_list)
{
    zb_bufid_t buf = ZB_BUF_INVALID;
    zb_uint8_t *ptr;
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_zdo_ieee_addr_response ", (FMT__0));

    get_cb_ent_by_ncp_tsn(ncp_tsn, &cb_ent);

    if (cb_ent != NULL)
    {
        buf = zb_buf_get(ZB_TRUE, 0);
        ZB_ASSERT(buf != ZB_BUF_INVALID);

        resp->tsn = cb_ent->zdp_tsn;

        if (status != RET_OK)
        {
            /* Pass only ZDO errors to the application layer */
            if (ERROR_GET_CATEGORY(status) == ERROR_CATEGORY_ZDO)
            {
                resp->status = ERROR_GET_CODE(status);
                ptr = zb_buf_initial_alloc(buf, sizeof(zb_zdo_ieee_addr_resp_t));
                ZB_MEMCPY(ptr, resp, sizeof(zb_zdo_ieee_addr_resp_t));
            }
            else
            {
                TRACE_MSG(TRACE_ERROR, "Unexpected type of error, error category = %hd, error status = %hd ",
                          (FMT__H_H, ERROR_GET_CATEGORY(status), ERROR_GET_CODE(status)));
                ZB_ASSERT(0);
            }
        }
        else
        {
            resp->status = ZB_ZDP_STATUS_SUCCESS;
            ptr = zb_buf_initial_alloc(buf, sizeof(zb_zdo_ieee_addr_resp_t));
            ZB_MEMCPY(ptr, resp, sizeof(zb_zdo_ieee_addr_resp_t));

            if (is_extended)
            {
                ptr = zb_buf_alloc_right(buf, sizeof(zb_zdo_ieee_addr_resp_ext_t)
                                         + sizeof(zb_zdo_ieee_addr_resp_ext2_t)
                                         + sizeof(zb_uint16_t) * resp_ext->num_assoc_dev);

                ZB_MEMCPY(ptr, resp_ext, sizeof(zb_zdo_ieee_addr_resp_ext_t));
                ptr = ptr + sizeof(zb_zdo_ieee_addr_resp_ext_t);
                ZB_MEMCPY(ptr, resp_ext2, sizeof(zb_zdo_ieee_addr_resp_ext2_t));
                ptr = ptr + sizeof(zb_zdo_ieee_addr_resp_ext2_t);
                ZB_MEMCPY(ptr, addr_list, sizeof(zb_uint16_t) * resp_ext->num_assoc_dev);
            }
        }

        ncp_host_zdo_adapter_call_cb(buf, resp->status, cb_ent);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_zdo_ieee_addr_response ", (FMT__0));
}


void zb_set_node_descriptor_manufacturer_code_req(zb_uint16_t manuf_code, zb_set_manufacturer_code_cb_t cb)
{
    zb_ret_t ret = RET_ERROR;
    zb_uint8_t zdp_tsn;
    zb_uint8_t ncp_tsn;
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_set_node_descriptor_manufacturer_code_req", (FMT__0));

    zdp_tsn = ncp_host_zdo_adapter_register_request(cb, ZB_FALSE, &cb_ent);

    if (zdp_tsn != ZB_ZDO_INVALID_TSN)
    {
        ret = ncp_host_zdo_set_node_desc_manuf_code(manuf_code, &ncp_tsn);
        ZB_ASSERT(ret == RET_OK);
        ncp_host_zdo_adapter_update_cb_ent(cb_ent, ZB_BUF_INVALID, ncp_tsn);
    }

    if (ret != RET_OK)
    {
        if (cb != NULL)
        {
            cb(RET_ERROR);
        }
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_set_node_descriptor_manufacturer_code_req", (FMT__0));
}


void ncp_host_handle_zdo_nwk_addr_response(zb_ret_t status, zb_uint8_t ncp_tsn,
        zb_zdo_nwk_addr_resp_head_t *resp,
        zb_bool_t is_extended,
        zb_zdo_nwk_addr_resp_ext_t *resp_ext,
        zb_zdo_nwk_addr_resp_ext2_t *resp_ext2,
        zb_uint16_t *addr_list)
{
    zb_bufid_t buf = ZB_BUF_INVALID;
    zb_uint8_t *ptr;
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_zdo_nwk_addr_response ", (FMT__0));

    get_cb_ent_by_ncp_tsn(ncp_tsn, &cb_ent);
    if (cb_ent != NULL)
    {
        resp->tsn = cb_ent->zdp_tsn;

        buf = zb_buf_get(ZB_TRUE, 0);
        ZB_ASSERT(buf != ZB_BUF_INVALID);

        if (status != RET_OK)
        {
            /* Pass only ZDO errors to the application layer */
            if (ERROR_GET_CATEGORY(status) == ERROR_CATEGORY_ZDO)
            {
                resp->status = ERROR_GET_CODE(status);
                ptr = zb_buf_initial_alloc(buf, sizeof(zb_zdo_nwk_addr_resp_head_t));
                ZB_MEMCPY(ptr, resp, sizeof(zb_zdo_nwk_addr_resp_head_t));
            }
            else
            {
                TRACE_MSG(TRACE_ERROR, "Unexpected type of error, error category = %hd, error status = %hd ",
                          (FMT__H_H, ERROR_GET_CATEGORY(status), ERROR_GET_CODE(status)));
                ZB_ASSERT(0);
            }
        }
        else
        {
            resp->status = ZB_ZDP_STATUS_SUCCESS;
            ptr = zb_buf_initial_alloc(buf, sizeof(zb_zdo_nwk_addr_resp_head_t));
            ZB_MEMCPY(ptr, resp, sizeof(zb_zdo_nwk_addr_resp_head_t));

            if (is_extended)
            {
                ptr = zb_buf_alloc_right(buf, sizeof(zb_zdo_nwk_addr_resp_ext_t)
                                         + sizeof(zb_zdo_nwk_addr_resp_ext2_t)
                                         + sizeof(zb_uint16_t) * resp_ext->num_assoc_dev);

                ZB_MEMCPY(ptr, resp_ext, sizeof(zb_zdo_nwk_addr_resp_ext_t));
                ptr = ptr + sizeof(zb_zdo_nwk_addr_resp_ext_t);
                ZB_MEMCPY(ptr, resp_ext2, sizeof(zb_zdo_nwk_addr_resp_ext2_t));
                ptr = ptr + sizeof(zb_zdo_nwk_addr_resp_ext2_t);
                ZB_MEMCPY(ptr, addr_list, sizeof(zb_uint16_t) * resp_ext->num_assoc_dev);
            }
        }

        ncp_host_zdo_adapter_call_cb(buf, resp->status, cb_ent);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_zdo_nwk_addr_response ", (FMT__0));
}

void ncp_host_handle_zdo_power_descriptor_response(zb_ret_t status, zb_uint8_t ncp_tsn,
        zb_uint16_t nwk_addr, zb_uint16_t power_descriptor)
{
    zb_bufid_t buf = ZB_BUF_INVALID;
    zb_zdo_power_desc_resp_t *resp = NULL;
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_zdo_power_descriptor_response ", (FMT__0));

    get_cb_ent_by_ncp_tsn(ncp_tsn, &cb_ent);
    if (cb_ent != NULL)
    {

        buf = zb_buf_get(ZB_TRUE, 0);
        ZB_ASSERT(buf != ZB_BUF_INVALID);

        resp = (zb_zdo_power_desc_resp_t *)zb_buf_begin(buf);
        resp->hdr.tsn = cb_ent->zdp_tsn;

        if (status != RET_OK)
        {
            /* Pass only ZDO errors to the application layer */
            if (ERROR_GET_CATEGORY(status) == ERROR_CATEGORY_ZDO)
            {
                resp->hdr.status = ERROR_GET_CODE(status);
            }
            else
            {
                TRACE_MSG(TRACE_ERROR, "Unexpected type of error, error category = %hd, error status = %hd ",
                          (FMT__H_H, ERROR_GET_CATEGORY(status), ERROR_GET_CODE(status)));
                ZB_ASSERT(0);
            }
        }
        else
        {
            resp->hdr.status = ZB_ZDP_STATUS_SUCCESS;
            resp->hdr.nwk_addr = nwk_addr;
            resp->power_desc.power_desc_flags = power_descriptor;
        }

        ncp_host_zdo_adapter_call_cb(buf, resp->hdr.status, cb_ent);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_zdo_power_descriptor_response ", (FMT__0));
}

void ncp_host_handle_zdo_node_descriptor_response(zb_ret_t status, zb_uint8_t ncp_tsn,
        zb_uint16_t nwk_addr, zb_af_node_desc_t *node_desc)
{
    zb_bufid_t buf = ZB_BUF_INVALID;
    zb_zdo_node_desc_resp_t *resp = NULL;
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_zdo_node_descriptor_response ", (FMT__0));

    get_cb_ent_by_ncp_tsn(ncp_tsn, &cb_ent);
    if (cb_ent != NULL)
    {
        buf = zb_buf_get(ZB_TRUE, 0);
        ZB_ASSERT(buf != ZB_BUF_INVALID);

        resp = (zb_zdo_node_desc_resp_t *)zb_buf_begin(buf);
        resp->hdr.tsn = cb_ent->zdp_tsn;

        if (status != RET_OK)
        {
            /* Pass only ZDO errors to the application layer */
            if (ERROR_GET_CATEGORY(status) == ERROR_CATEGORY_ZDO)
            {
                resp->hdr.status = ERROR_GET_CODE(status);
            }
            else
            {
                TRACE_MSG(TRACE_ERROR, "Unexpected type of error, error category = %hd, error status = %hd ",
                          (FMT__H_H, ERROR_GET_CATEGORY(status), ERROR_GET_CODE(status)));
                ZB_ASSERT(0);
            }
        }
        else
        {
            resp->hdr.status = ZB_ZDP_STATUS_SUCCESS;
            resp->hdr.nwk_addr = nwk_addr;
            ZB_MEMCPY(&resp->node_desc, node_desc, sizeof(zb_af_node_desc_t));
        }

        ncp_host_zdo_adapter_call_cb(buf, resp->hdr.status, cb_ent);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_zdo_node_descriptor_response ", (FMT__0));
}

void ncp_host_handle_zdo_simple_descriptor_response(zb_ret_t status, zb_uint8_t ncp_tsn,
        zb_uint16_t nwk_addr,
        zb_af_simple_desc_1_1_t *simple_desc,
        zb_uint16_t *app_cluster_list_ptr)
{
    zb_bufid_t buf = ZB_BUF_INVALID;
    zb_zdo_simple_desc_resp_t *resp = NULL;
    zb_uint8_t i;
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_zdo_simple_descriptor_response ", (FMT__0));

    get_cb_ent_by_ncp_tsn(ncp_tsn, &cb_ent);
    if (cb_ent != NULL)
    {
        buf = zb_buf_get(ZB_TRUE, 0);
        ZB_ASSERT(buf != ZB_BUF_INVALID);

        resp = (zb_zdo_simple_desc_resp_t *)zb_buf_begin(buf);
        resp->hdr.tsn = cb_ent->zdp_tsn;

        if (status != RET_OK)
        {
            /* Pass only ZDO errors to the application layer */
            if (ERROR_GET_CATEGORY(status) == ERROR_CATEGORY_ZDO)
            {
                resp->hdr.status = ERROR_GET_CODE(status);
            }
            else
            {
                TRACE_MSG(TRACE_ERROR, "Unexpected type of error, error category = %hd, error status = %hd ",
                          (FMT__H_H, ERROR_GET_CATEGORY(status), ERROR_GET_CODE(status)));
                ZB_ASSERT(0);
            }
        }
        else
        {
            resp->hdr.status = ZB_ZDP_STATUS_SUCCESS;
            resp->hdr.length = sizeof(zb_af_simple_desc_1_1_t) + sizeof(zb_uint16_t)
                               * (resp->simple_desc.app_input_cluster_count + resp->simple_desc.app_output_cluster_count - 2);
            resp->hdr.nwk_addr = nwk_addr;

            ZB_MEMCPY(&resp->simple_desc, simple_desc, sizeof(zb_af_simple_desc_1_1_t));
            ZB_MEMCPY(&resp->simple_desc.app_cluster_list, app_cluster_list_ptr, sizeof(zb_uint16_t)
                      * (resp->simple_desc.app_input_cluster_count + resp->simple_desc.app_output_cluster_count));

            TRACE_MSG(TRACE_TRANSPORT3, "simple_desc: input_app_cluster_list:", (FMT__0));
            for (i = 0; i < resp->simple_desc.app_input_cluster_count; i++)
            {
                TRACE_MSG(TRACE_TRANSPORT3, " 0x%x", (FMT__D, resp->simple_desc.app_cluster_list[i]));
            }

            TRACE_MSG(TRACE_TRANSPORT3, "simple_desc: output_app_cluster_list:", (FMT__0));
            for (; i < resp->simple_desc.app_output_cluster_count + resp->simple_desc.app_input_cluster_count; i++)
            {
                TRACE_MSG(TRACE_TRANSPORT3, " 0x%x", (FMT__D, resp->simple_desc.app_cluster_list[i]));
            }
        }

        ncp_host_zdo_adapter_call_cb(buf, resp->hdr.status, cb_ent);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_zdo_simple_descriptor_response ", (FMT__0));
}

void ncp_host_handle_zdo_active_ep_response(zb_ret_t status, zb_uint8_t ncp_tsn,
        zb_uint16_t nwk_addr,
        zb_uint8_t ep_count,
        zb_uint8_t *ep_list)
{
    zb_bufid_t buf = ZB_BUF_INVALID;
    zb_zdo_ep_resp_t *resp = NULL;
    zb_uint8_t *ep_list_ptr = NULL;
    zb_uint8_t i;
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_zdo_active_ep_response ", (FMT__0));

    get_cb_ent_by_ncp_tsn(ncp_tsn, &cb_ent);
    if (cb_ent != NULL)
    {
        buf = zb_buf_get(ZB_TRUE, 0);
        ZB_ASSERT(buf != ZB_BUF_INVALID);

        resp = (zb_zdo_ep_resp_t *)zb_buf_begin(buf);
        resp->tsn = cb_ent->zdp_tsn;

        ep_list_ptr = (zb_uint8_t *)(resp + 1);

        if (status != RET_OK)
        {
            /* Pass only ZDO errors to the application layer */
            if (ERROR_GET_CATEGORY(status) == ERROR_CATEGORY_ZDO)
            {
                resp->status = ERROR_GET_CODE(status);
            }
            else
            {
                TRACE_MSG(TRACE_ERROR, "Unexpected type of error, error category = %hd, error status = %hd ",
                          (FMT__H_H, ERROR_GET_CATEGORY(status), ERROR_GET_CODE(status)));
                ZB_ASSERT(0);
            }
        }
        else
        {
            resp->status = ZB_ZDP_STATUS_SUCCESS;
            resp->ep_count = ep_count;
            ZB_MEMCPY(ep_list_ptr, ep_list, sizeof(zb_uint8_t) * ep_count);

            resp->nwk_addr = nwk_addr;

            TRACE_MSG(TRACE_TRANSPORT3, "endpoint list: ", (FMT__0));
            for (i = 0; i < ep_count; i++)
            {
                TRACE_MSG(TRACE_TRANSPORT3, " 0x%hx", (FMT__H, ep_list_ptr[i]));
            }
        }

        ncp_host_zdo_adapter_call_cb(buf, resp->status, cb_ent);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_zdo_active_ep_response ", (FMT__0));
}

void ncp_host_handle_zdo_match_desc_response(zb_ret_t status, zb_uint8_t ncp_tsn,
        zb_uint16_t nwk_addr,
        zb_uint8_t match_ep_count,
        zb_uint8_t *match_ep_list)
{
    zb_bufid_t buf = ZB_BUF_INVALID;
    zb_zdo_match_desc_resp_t *resp = NULL;
    zb_uint8_t *match_ep_list_ptr = NULL;
    zb_uint8_t i;
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_zdo_match_desc_response ", (FMT__0));

    get_cb_ent_by_ncp_tsn(ncp_tsn, &cb_ent);
    if (cb_ent != NULL)
    {
        buf = zb_buf_get(ZB_TRUE, 0);
        ZB_ASSERT(buf != ZB_BUF_INVALID);

        resp = (zb_zdo_match_desc_resp_t *)zb_buf_begin(buf);
        resp->tsn = cb_ent->zdp_tsn;

        match_ep_list_ptr = (zb_uint8_t *)(resp + 1);

        if (status != RET_OK)
        {
            /* Pass only ZDO errors to the application layer */
            if (ERROR_GET_CATEGORY(status) == ERROR_CATEGORY_ZDO)
            {
                resp->status = ERROR_GET_CODE(status);
            }
            else
            {
                TRACE_MSG(TRACE_ERROR, "Unexpected type of error, error category = %hd, error status = %hd ",
                          (FMT__H_H, ERROR_GET_CATEGORY(status), ERROR_GET_CODE(status)));
                ZB_ASSERT(0);
            }
        }
        else
        {
            resp->status = ZB_ZDP_STATUS_SUCCESS;
            resp->match_len = match_ep_count;
            resp->nwk_addr = nwk_addr;

            ZB_MEMCPY(match_ep_list_ptr, match_ep_list, sizeof(zb_uint8_t) * match_ep_count);

            TRACE_MSG(TRACE_TRANSPORT3, "match endpoint list: ", (FMT__0));
            for (i = 0; i < match_ep_count; i++)
            {
                TRACE_MSG(TRACE_TRANSPORT3, " 0x%hx", (FMT__H, match_ep_list_ptr[i]));
            }
        }

        ncp_host_zdo_adapter_call_cb(buf, resp->status, cb_ent);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_zdo_match_desc_response ", (FMT__0));
}

void ncp_host_handle_zdo_diagnostics_get_stats_response(zb_ret_t status, zb_uint8_t ncp_tsn,
        zdo_diagnostics_full_stats_t *full_stats)
{
    zb_bufid_t buf = ZB_BUF_INVALID;
    zdo_diagnostics_full_stats_t *buf_full_stats = NULL;
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_zdo_diagnostics_get_stats_response ", (FMT__0));

    get_cb_ent_by_ncp_tsn(ncp_tsn, &cb_ent);
    if (cb_ent == NULL)
    {
        TRACE_MSG(TRACE_ERROR, "got get_stats_response, but request is not registered", (FMT__0));
    }
    else
    {
        buf = zb_buf_get(ZB_TRUE, 0);
        ZB_ASSERT(buf != ZB_BUF_INVALID);

        buf_full_stats = zb_buf_initial_alloc(buf, sizeof(zdo_diagnostics_full_stats_t));

        if (status != RET_OK)
        {
            /* Pass only ZDO errors to the application layer */
            if (ERROR_GET_CATEGORY(status) == ERROR_CATEGORY_ZDO)
            {
                buf_full_stats->status = ERROR_GET_CODE(status);
            }
            else
            {
                TRACE_MSG(TRACE_ERROR, "Unexpected type of error, error category = %hd, error status = %hd ",
                          (FMT__H_H, ERROR_GET_CATEGORY(status), ERROR_GET_CODE(status)));
                ZB_ASSERT(0);
            }
        }
        else
        {
            full_stats->zdo_stats.packet_buffer_allocate_failures +=
                g_ncp_host_zdo_adapter_ctx.stats.packet_buffer_allocate_failures;

            ZB_MEMCPY(buf_full_stats, full_stats, sizeof(*buf_full_stats));
        }

        ncp_host_zdo_adapter_call_cb(buf, buf_full_stats->status, cb_ent);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_zdo_diagnostics_get_stats_response ", (FMT__0));
}


void ncp_host_handle_zdo_system_server_discovery_response(zb_ret_t status, zb_uint8_t ncp_tsn,
        zb_uint16_t server_mask)
{
    zb_bufid_t buf = ZB_BUF_INVALID;
    zb_zdo_system_server_discovery_resp_t *resp = NULL;
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_zdo_system_server_discovery_response ", (FMT__0));

    get_cb_ent_by_ncp_tsn(ncp_tsn, &cb_ent);
    if (cb_ent != NULL)
    {
        buf = zb_buf_get(ZB_TRUE, 0);
        ZB_ASSERT(buf != ZB_BUF_INVALID);

        resp = (zb_zdo_system_server_discovery_resp_t *)zb_buf_begin(buf);
        resp->tsn = cb_ent->zdp_tsn;

        if (status != RET_OK)
        {
            /* Pass only ZDO errors to the application layer */
            if (ERROR_GET_CATEGORY(status) == ERROR_CATEGORY_ZDO)
            {
                resp->status = ERROR_GET_CODE(status);
            }
            else
            {
                TRACE_MSG(TRACE_ERROR, "Unexpected type of error, error category = %hd, error status = %hd ",
                          (FMT__H_H, ERROR_GET_CATEGORY(status), ERROR_GET_CODE(status)));
                ZB_ASSERT(0);

            }
        }
        else
        {
            resp->status = ZB_ZDP_STATUS_SUCCESS;
            resp->server_mask = server_mask;
        }

        ncp_host_zdo_adapter_call_cb(buf, resp->status, cb_ent);

        TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_zdo_system_server_discovery_response ", (FMT__0));
    }
}


void ncp_host_handle_zdo_set_node_desc_manuf_code_response(zb_ret_t status, zb_uint8_t ncp_tsn)
{
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_zdo_set_node_desc_manuf_code_response ",
              (FMT__0));

    get_cb_ent_by_ncp_tsn(ncp_tsn, &cb_ent);

    if (cb_ent != NULL)
    {
        zb_set_manufacturer_code_cb_t set_manuf_code_cb = (zb_set_manufacturer_code_cb_t)cb_ent->cb;

        if (set_manuf_code_cb)
        {
            set_manuf_code_cb(status);
        }

        ncp_host_zdo_adapter_clean_cb_ent(cb_ent);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_zdo_set_node_desc_manuf_code_response ",
              (FMT__0));
}


void ncp_host_handle_zdo_bind_response(zb_ret_t status, zb_uint8_t ncp_tsn)
{
    zb_bufid_t buf = ZB_BUF_INVALID;
    zb_zdo_bind_resp_t *resp = NULL;
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_zdo_bind_response ", (FMT__0));

    get_cb_ent_by_ncp_tsn(ncp_tsn, &cb_ent);
    if (cb_ent != NULL)
    {
        buf = zb_buf_get(ZB_TRUE, 0);
        ZB_ASSERT(buf != ZB_BUF_INVALID);

        resp = (zb_zdo_bind_resp_t *)zb_buf_begin(buf);
        resp->tsn = cb_ent->zdp_tsn;

        if (status != RET_OK)
        {
            /* Pass only ZDO errors to the application layer */
            if (ERROR_GET_CATEGORY(status) == ERROR_CATEGORY_ZDO)
            {
                resp->status = ERROR_GET_CODE(status);
            }
            else
            {
                TRACE_MSG(TRACE_ERROR, "Unexpected type of error, error category = %hd, error status = %hd ",
                          (FMT__H_H, ERROR_GET_CATEGORY(status), ERROR_GET_CODE(status)));
                ZB_ASSERT(0);
            }
        }
        else
        {
            resp->status = ZB_ZDP_STATUS_SUCCESS;
        }

        ncp_host_zdo_adapter_call_cb(buf, resp->status, cb_ent);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_zdo_bind_response ", (FMT__0));
}

void ncp_host_handle_zdo_unbind_response(zb_ret_t status, zb_uint8_t ncp_tsn)
{
    zb_bufid_t buf = ZB_BUF_INVALID;
    zb_zdo_bind_resp_t *resp = NULL;
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_zdo_unbind_response ", (FMT__0));

    get_cb_ent_by_ncp_tsn(ncp_tsn, &cb_ent);
    if (cb_ent != NULL)
    {
        buf = zb_buf_get(ZB_TRUE, 0);
        ZB_ASSERT(buf != ZB_BUF_INVALID);

        resp = (zb_zdo_bind_resp_t *)zb_buf_begin(buf);
        resp->tsn = cb_ent->zdp_tsn;

        if (status != RET_OK)
        {
            /* Pass only ZDO errors to the application layer */
            if (ERROR_GET_CATEGORY(status) == ERROR_CATEGORY_ZDO)
            {
                resp->status = ERROR_GET_CODE(status);
            }
            else
            {
                TRACE_MSG(TRACE_ERROR, "Unexpected type of error, error category = %hd, error status = %hd ",
                          (FMT__H_H, ERROR_GET_CATEGORY(status), ERROR_GET_CODE(status)));
                ZB_ASSERT(0);
            }
        }
        else
        {
            resp->status = ZB_ZDP_STATUS_SUCCESS;
        }

        ncp_host_zdo_adapter_call_cb(buf, resp->status, cb_ent);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_zdo_unbind_response ", (FMT__0));
}

void ncp_host_handle_zdo_permit_joining_response(zb_ret_t status, zb_uint8_t ncp_tsn)
{
    zb_bufid_t buf = ZB_BUF_INVALID;
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;
    zb_zdo_mgmt_permit_joining_resp_t *resp = NULL;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_zdo_permit_joining_response ", (FMT__0));

    get_cb_ent_by_ncp_tsn(ncp_tsn, &cb_ent);
    if (cb_ent != NULL)
    {
        buf = zb_buf_get(ZB_TRUE, 0);
        ZB_ASSERT(buf != ZB_BUF_INVALID);

        resp = (zb_zdo_mgmt_permit_joining_resp_t *)zb_buf_begin(buf);
        resp->tsn = cb_ent->zdp_tsn;

        if (status != RET_OK)
        {
            /* Pass only ZDO errors to the application layer */
            if (ERROR_GET_CATEGORY(status) == ERROR_CATEGORY_ZDO)
            {
                resp->status = ERROR_GET_CODE(status);
            }
            else
            {
                TRACE_MSG(TRACE_ERROR,
                          "Unexpected type of error, error category = %hd, error status = %hd ",
                          (FMT__H_H, ERROR_GET_CATEGORY(status), ERROR_GET_CODE(status)));
                ZB_ASSERT(0);
            }
        }
        else
        {
            resp->status = ZB_ZDP_STATUS_SUCCESS;
        }

        ncp_host_zdo_adapter_call_cb(buf, resp->status, cb_ent);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_zdo_permit_joining_response ", (FMT__0));
}


void ncp_host_handle_zdo_mgmt_leave_response(zb_ret_t status, zb_uint8_t ncp_tsn)
{
    zb_bufid_t buf = ZB_BUF_INVALID;
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;
    zb_zdo_mgmt_leave_res_t *resp = NULL;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_zdo_mgmt_leave_response ", (FMT__0));

    get_cb_ent_by_ncp_tsn(ncp_tsn, &cb_ent);
    if (cb_ent != NULL)
    {
        buf = zb_buf_get(ZB_TRUE, 0);
        ZB_ASSERT(buf != ZB_BUF_INVALID);

        resp = (zb_zdo_mgmt_leave_res_t *)zb_buf_begin(buf);
        resp->tsn = cb_ent->zdp_tsn;

        if (status != RET_OK)
        {
            /* Pass only ZDO errors to the application layer */
            if (ERROR_GET_CATEGORY(status) == ERROR_CATEGORY_ZDO)
            {
                resp->status = ERROR_GET_CODE(status);
            }
            else
            {
                TRACE_MSG(TRACE_ERROR, "Unexpected type of error, error category = %hd, error status = %hd ",
                          (FMT__H_H, ERROR_GET_CATEGORY(status), ERROR_GET_CODE(status)));
                ZB_ASSERT(0);
            }
        }
        else
        {
            resp->status = ZB_ZDP_STATUS_SUCCESS;
        }

        ncp_host_zdo_adapter_call_cb(buf, resp->status, cb_ent);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_zdo_mgmt_leave_response ", (FMT__0));
}


void ncp_host_handle_zdo_device_annce_indication(zb_uint16_t nwk_addr, zb_ieee_addr_t ieee_addr,
        zb_uint8_t capability)
{
    zb_bufid_t param;
    zb_zdo_signal_device_annce_params_t *dev_annce_params;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_zdo_device_annce_indication ", (FMT__0));

    param = zb_buf_get(ZB_TRUE, 0);
    ZB_ASSERT(param != ZB_BUF_INVALID);

    dev_annce_params = (zb_zdo_signal_device_annce_params_t *)zb_app_signal_pack(param,
                       ZB_ZDO_SIGNAL_DEVICE_ANNCE,
                       RET_OK,
                       sizeof(zb_zdo_signal_device_annce_params_t));

    dev_annce_params->device_short_addr = nwk_addr;
    ZB_IEEE_ADDR_COPY(dev_annce_params->ieee_addr, ieee_addr);
    dev_annce_params->capability = capability;

    ZB_SCHEDULE_CALLBACK(zboss_signal_handler, param);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_zdo_device_annce_indication", (FMT__0));
}


void ncp_host_handle_zdo_device_authorized_indication(zb_ieee_addr_t ieee_addr,
        zb_uint16_t nwk_addr,
        zb_uint8_t authorization_type,
        zb_uint8_t authorization_status)
{
    zb_bufid_t param;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_zdo_device_authorized_indication ", (FMT__0));

    param = zb_buf_get(ZB_TRUE, 0);
    ZB_ASSERT(param != ZB_BUF_INVALID);

    zb_send_device_authorized_signal(param, ieee_addr, nwk_addr, authorization_type, authorization_status);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_zdo_device_authorized_indication", (FMT__0));
}


void ncp_host_handle_zdo_device_update_indication(zb_ieee_addr_t ieee_addr,
        zb_uint16_t nwk_addr,
        zb_uint8_t status)
{
    zb_bufid_t param;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_zdo_device_update_indication ", (FMT__0));

    param = zb_buf_get(ZB_TRUE, 0);
    ZB_ASSERT(param != ZB_BUF_INVALID);

    zb_send_device_update_signal(param, ieee_addr, nwk_addr, status);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_zdo_device_update_indication", (FMT__0));
}

#ifdef ZB_JOIN_CLIENT

void zdo_rejoin_clear_prev_join(void)
{
    TRACE_MSG(TRACE_ZDO1, ">> zdo_rejoin_clear_prev_join", (FMT__0));

    ncp_host_state_set_joined(ZB_FALSE);

    TRACE_MSG(TRACE_ZDO1, "<< zdo_rejoin_clear_prev_join", (FMT__0));
}


zb_ret_t zdo_initiate_rejoin(zb_bufid_t buf, zb_uint8_t *ext_pan_id,
                             zb_channel_page_t *channels_list, zb_bool_t secure_rejoin)
{
    zb_ret_t status = RET_ERROR;

    TRACE_MSG(TRACE_ZDO2, ">> zdo_initiate_rejoin", (FMT__0));

    status = ncp_host_zdo_rejoin_request(NULL, ext_pan_id, channels_list, secure_rejoin);
    ZB_ASSERT(status == RET_OK);

    zb_buf_free(buf);

    ncp_host_state_set_rejoin_active(ZB_TRUE);

    TRACE_MSG(TRACE_ZDO2, "<< zdo_initiate_rejoin", (FMT__0));

    return RET_OK;
}


void ncp_host_handle_zdo_rejoin_response(zb_ret_t status)
{
    zb_bufid_t buf = zb_buf_get(ZB_TRUE, 0);

    TRACE_MSG(TRACE_ZDO2, ">> ncp_host_handle_zdo_rejoin_response, status 0x%x", (FMT__D, status));

    ZB_ASSERT(buf != ZB_BUF_INVALID);

    ncp_host_state_set_rejoin_active(ZB_FALSE);

    if (buf != ZB_BUF_INVALID)
    {
        if (status == RET_OK)
        {
            ncp_host_state_set_joined(ZB_TRUE);
            zdo_commissioning_handle_dev_annce_sent_event(buf);
        }
        else if (status == ERROR_CODE(ERROR_CATEGORY_ZDO, ZB_ZDP_STATUS_NOT_AUTHORIZED))
        {
            zdo_commissioning_authentication_failed(buf);
        }
        else
        {
            zdo_commissioning_join_failed(buf);
        }
    }

    TRACE_MSG(TRACE_ZDO2, "<< ncp_host_handle_zdo_rejoin_response", (FMT__0));
}

#endif /* ZB_JOIN_CLIENT */


void zb_zdo_startup_complete_int(zb_uint8_t param)
{
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);
    TRACE_MSG(TRACE_ERROR, "> zb_zdo_startup_complete_int param %hd status %hd signal %hd",
              (FMT__H_H_H, param, zb_buf_get_status(param), sig));

    ZB_ASSERT((param) && (param != ZB_UNDEFINED_BUFFER));

    /* Send signal to the app */
    zboss_signal_handler(param);

    TRACE_MSG(TRACE_ZDO1, "< zb_zdo_startup_complete_int", (FMT__0));
}


void zb_af_register_device_ctx(zb_af_device_ctx_t *device_ctx)
{
    TRACE_MSG(TRACE_ZDO2, ">> zb_af_register_device_ctx", (FMT__0));

    ZB_ASSERT(!ncp_host_state_get_zboss_started());

    zb_zcl_register_device_ctx(device_ctx);

    ZB_ASSERT(zb_zcl_check_cluster_list());

    TRACE_MSG(TRACE_ZDO2, "<< zb_af_register_device_ctx", (FMT__0));
}


void zb_zdo_pim_get_long_poll_interval_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_ret_t ret;
    zb_uint8_t zdp_tsn;
    zb_uint8_t ncp_tsn;
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_zdo_pim_get_long_poll_interval_req", (FMT__0));

    ZB_ASSERT(param != ZB_BUF_INVALID);

    zdp_tsn = ncp_host_zdo_adapter_register_request(cb, ZB_FALSE, &cb_ent);
    /* TODO: proper error indication? */
    ZB_ASSERT(zdp_tsn != ZB_ZDO_INVALID_TSN);

    ret = ncp_host_nwk_get_long_poll_interval(&ncp_tsn);
    ZB_ASSERT(ret == RET_OK);
    ncp_host_zdo_adapter_update_cb_ent(cb_ent, ZB_BUF_INVALID, ncp_tsn);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_zdo_pim_get_long_poll_interval_req", (FMT__0));
}


void zb_zdo_pim_get_in_fast_poll_flag_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_ret_t ret;
    zb_uint8_t zdp_tsn;
    zb_uint8_t ncp_tsn;
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_zdo_pim_get_in_fast_poll_flag_req", (FMT__0));

    ZB_ASSERT(param != ZB_BUF_INVALID);

    zdp_tsn = ncp_host_zdo_adapter_register_request(cb, ZB_FALSE, &cb_ent);
    /* TODO: proper error indication */
    ZB_ASSERT(zdp_tsn != ZB_ZDO_INVALID_TSN);

    ret = ncp_host_nwk_get_in_fast_poll_flag(&ncp_tsn);
    ZB_ASSERT(ret == RET_OK);
    ncp_host_zdo_adapter_update_cb_ent(cb_ent, ZB_BUF_INVALID, ncp_tsn);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_zdo_pim_get_in_fast_poll_flag_req", (FMT__0));
}


void ncp_host_handle_nwk_get_long_poll_interval_response(zb_ret_t status, zb_uint8_t ncp_tsn, zb_uint32_t interval)
{
    zb_bufid_t buf;
    zb_zdo_pim_get_long_poll_interval_resp_t *resp = NULL;
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_get_long_poll_interval_response ", (FMT__0));

    get_cb_ent_by_ncp_tsn(ncp_tsn, &cb_ent);
    if (cb_ent == NULL)
    {
        TRACE_MSG(TRACE_ERROR, "got poll interval response, but no request is registered",
                  (FMT__0));
    }
    else
    {
        buf = cb_ent->buffer;
        ZB_ASSERT(buf != ZB_BUF_INVALID);

        resp = ZB_BUF_GET_PARAM(buf, zb_zdo_pim_get_long_poll_interval_resp_t);

        if (status != RET_OK)
        {
            TRACE_MSG(TRACE_ERROR, "Unexpected type of error, error category = %hd, error status = %hd ",
                      (FMT__H_H, ERROR_GET_CATEGORY(status), ERROR_GET_CODE(status)));
            ZB_ASSERT(0);
        }

        resp->interval = interval;

        ncp_host_zdo_adapter_call_cb(buf, RET_OK, cb_ent);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_get_long_poll_interval_response ", (FMT__0));
}


void ncp_host_handle_nwk_get_in_fast_poll_flag_response(zb_ret_t status, zb_uint8_t ncp_tsn, zb_uint8_t in_fast_poll)
{
    zb_bufid_t buf;
    zb_zdo_pim_get_in_fast_poll_flag_resp_t *resp = NULL;
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_get_in_fast_poll_flag_response ", (FMT__0));

    get_cb_ent_by_ncp_tsn(ncp_tsn, &cb_ent);
    if (cb_ent == NULL)
    {
        TRACE_MSG(TRACE_ERROR, "got get_in_fast_poll_flag response, but no request is registered",
                  (FMT__0));
    }
    else
    {
        buf = cb_ent->buffer;
        ZB_ASSERT(buf != ZB_BUF_INVALID);

        resp = ZB_BUF_GET_PARAM(buf, zb_zdo_pim_get_in_fast_poll_flag_resp_t);

        if (status != RET_OK)
        {
            TRACE_MSG(TRACE_ERROR, "Unexpected type of error, error category = %hd, error status = %hd ",
                      (FMT__H_H, ERROR_GET_CATEGORY(status), ERROR_GET_CODE(status)));
            ZB_ASSERT(0);
        }

        resp->in_fast_poll = in_fast_poll;

        ncp_host_zdo_adapter_call_cb(buf, RET_OK, cb_ent);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_get_in_fast_poll_flag_response ", (FMT__0));
}


/* Stop Fast Poll */
void zb_zdo_pim_stop_fast_poll_extended_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_ret_t ret;
    zb_uint8_t zdp_tsn;
    zb_uint8_t ncp_tsn;
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_zdo_pim_stop_fast_poll_extended_req", (FMT__0));

    ZB_ASSERT(param != ZB_BUF_INVALID);

    zdp_tsn = ncp_host_zdo_adapter_register_request(cb, ZB_FALSE, &cb_ent);
    /* TODO: proper error indication */
    ZB_ASSERT(zdp_tsn != ZB_ZDO_INVALID_TSN);

    ret = ncp_host_nwk_stop_fast_poll(&ncp_tsn);
    ZB_ASSERT(ret == RET_OK);
    ncp_host_zdo_adapter_update_cb_ent(cb_ent, ZB_BUF_INVALID, ncp_tsn);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_zdo_pim_stop_fast_poll_extended_req, ret %d", (FMT__D, ret));
}


void zb_zdo_pim_stop_fast_poll(zb_uint8_t param)
{
    zb_ret_t ret = RET_BUSY;
    zb_uint8_t ncp_tsn;

    ZVUNUSED(param);

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_zdo_pim_stop_fast_poll", (FMT__0));

    ret = ncp_host_nwk_stop_fast_poll(&ncp_tsn);
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_zdo_pim_stop_fast_poll, ret %d", (FMT__D, ret));
}


void ncp_host_handle_nwk_stop_fast_poll_response(zb_ret_t status_code, zb_uint8_t ncp_tsn,
        zb_stop_fast_poll_result_t stop_result)
{
    zb_bufid_t buf;
    zb_zdo_pim_stop_fast_poll_extended_resp_t *resp = NULL;
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_start_fast_poll_response, status_code %d, tsn %hd, stop_result %hd",
              (FMT__D_H_H, status_code, ncp_tsn, stop_result));

    get_cb_ent_by_ncp_tsn(ncp_tsn, &cb_ent);
    if (cb_ent != NULL)
    {
        if (status_code != RET_OK)
        {
            TRACE_MSG(TRACE_ERROR, "Unexpected type of error, error category = %hd, error status = %hd ",
                      (FMT__H_H, ERROR_GET_CATEGORY(status_code), ERROR_GET_CODE(status_code)));
            ZB_ASSERT(0);
        }

        buf = cb_ent->buffer;
        ZB_ASSERT(buf != ZB_BUF_INVALID);

        resp = ZB_BUF_GET_PARAM(buf, zb_zdo_pim_stop_fast_poll_extended_resp_t);

        resp->stop_result = stop_result;

        ncp_host_zdo_adapter_call_cb(buf, RET_OK, cb_ent);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_start_fast_poll_response", (FMT__0));
}



void zb_zdo_pim_set_long_poll_interval(zb_time_t ms)
{
    zb_ret_t ret = RET_BUSY;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_zdo_pim_set_long_poll_interval", (FMT__0));

    ret = ncp_host_nwk_set_long_poll_interval(ms);
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_zdo_pim_set_long_poll_interval, ret %d", (FMT__D, ret));
}


void zb_zdo_pim_set_fast_poll_interval(zb_time_t ms)
{
    zb_ret_t ret = RET_BUSY;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_zdo_pim_set_fast_poll_interval", (FMT__0));

    ret = ncp_host_nwk_set_fast_poll_interval(ms);
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_zdo_pim_set_fast_poll_interval, ret %d", (FMT__D, ret));
}


/* Start Fast Poll */
void zb_zdo_pim_start_fast_poll(zb_uint8_t param)
{
    zb_ret_t ret = RET_BUSY;

    ZVUNUSED(param);

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_zdo_pim_start_fast_poll", (FMT__0));

    ret = ncp_host_nwk_start_fast_poll();
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_zdo_pim_start_fast_poll, ret %d", (FMT__D, ret));
}


void zb_zdo_pim_start_turbo_poll_packets(zb_uint8_t n_packets)
{
    zb_ret_t ret = RET_BUSY;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_zdo_pim_start_turbo_poll_packets", (FMT__0));

    ret = ncp_host_nwk_start_turbo_poll_packets(n_packets);
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_zdo_pim_start_turbo_poll_packets, ret %d", (FMT__D, ret));
}


void zb_zdo_pim_turbo_poll_cancel_packet(void)
{
    zb_ret_t ret = RET_BUSY;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_zdo_pim_turbo_poll_cancel_packet", (FMT__0));

    ret = ncp_host_nwk_turbo_poll_cancel_packet();
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_zdo_pim_turbo_poll_cancel_packet, ret %d", (FMT__D, ret));
}


void zb_zdo_pim_start_turbo_poll_continuous(zb_time_t turbo_poll_timeout_ms)
{
    zb_ret_t ret = RET_BUSY;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_zdo_pim_start_turbo_poll_continuous", (FMT__0));

    ret = ncp_host_nwk_start_turbo_poll_continuous(turbo_poll_timeout_ms);
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_zdo_pim_start_turbo_poll_continuous, ret %d", (FMT__D, ret));
}


void zb_zdo_pim_turbo_poll_continuous_leave(zb_uint8_t param)
{
    zb_ret_t ret = RET_BUSY;

    ZVUNUSED(param);

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_zdo_pim_turbo_poll_continuous_leave", (FMT__0));

    ret = ncp_host_nwk_turbo_poll_continuous_leave();
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_zdo_pim_turbo_poll_continuous_leave, ret %d", (FMT__D, ret));
}


void zb_zdo_turbo_poll_packets_leave(zb_uint8_t param)
{
    zb_ret_t ret = RET_BUSY;

    ZVUNUSED(param);

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_zdo_turbo_poll_packets_leave", (FMT__0));

    ret = ncp_host_nwk_turbo_poll_packets_leave();
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_zdo_turbo_poll_packets_leave, ret %d", (FMT__D, ret));
}


void zb_zdo_pim_set_fast_poll_timeout(zb_time_t ms)
{
    zb_ret_t ret = RET_BUSY;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_zdo_pim_set_fast_poll_timeout", (FMT__0));

    ret = ncp_host_nwk_set_fast_poll_timeout(ms);
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_zdo_pim_set_fast_poll_timeout, ret %d", (FMT__D, ret));
}


void zb_zdo_pim_permit_turbo_poll(zb_bool_t permit)
{
    zb_ret_t ret = RET_BUSY;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_zdo_pim_permit_turbo_poll", (FMT__0));

    ret = ncp_host_nwk_permit_turbo_poll(permit);
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_zdo_pim_permit_turbo_poll, ret %d", (FMT__D, ret));
}


void zb_zdo_pim_start_poll(zb_uint8_t param)
{
    zb_ret_t ret = RET_BUSY;

    ZVUNUSED(param);

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_zdo_pim_start_poll", (FMT__0));

    ret = ncp_host_nwk_start_poll();
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_zdo_pim_start_poll, ret %d", (FMT__D, ret));
}


void zb_zdo_pim_stop_poll(zb_uint8_t param)
{
    zb_ret_t ret = RET_BUSY;

    ZVUNUSED(param);

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_zdo_pim_stop_poll", (FMT__0));

    ret = ncp_host_nwk_start_poll();
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_zdo_pim_stop_poll, ret %d", (FMT__D, ret));
}


zb_uint8_t zb_zdo_mgmt_bind_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_ret_t ret;
    zb_uint8_t zdp_tsn;
    zb_uint8_t ncp_tsn;
    zb_zdo_mgmt_bind_param_t *req_param = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_bind_param_t);
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_zdo_mgmt_bind_req", (FMT__0));

    zdp_tsn = ncp_host_zdo_adapter_register_request(cb,
              ZB_NWK_IS_ADDRESS_BROADCAST(req_param->dst_addr),
              &cb_ent);

    if (zdp_tsn != ZB_ZDO_INVALID_TSN)
    {
        ret = ncp_host_zdo_mgmt_bind_request(&ncp_tsn, req_param->dst_addr, req_param->start_index);
        ZB_ASSERT(ret == RET_OK);
        ncp_host_zdo_adapter_update_cb_ent(cb_ent, ZB_BUF_INVALID, ncp_tsn);
    }

    zb_buf_free(param);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_zdo_mgmt_bind_req", (FMT__0));

    return zdp_tsn;
}


void ncp_host_handle_zdo_mgmt_bind_response(zb_ret_t status, zb_uint8_t ncp_tsn, zb_uint8_t binding_table_entries,
        zb_uint8_t start_index, zb_uint8_t binding_table_list_count, ncp_host_zb_zdo_binding_table_record_t *binding_table)
{
    zb_bufid_t buf = ZB_BUF_INVALID;
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;
    zb_zdo_mgmt_bind_resp_t *resp = NULL;
    zb_uindex_t entry_index = 0;
    zb_zdo_binding_table_record_t *resp_binding_table;

    ncp_host_zb_zdo_binding_table_record_t *ncp_resp_binding_entry;
    zb_zdo_binding_table_record_t *resp_binding_entry;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_zdo_mgmt_bind_response ", (FMT__0));

    get_cb_ent_by_ncp_tsn(ncp_tsn, &cb_ent);

    if (cb_ent != NULL)
    {
        buf = zb_buf_get(ZB_TRUE, 0);
        ZB_ASSERT(buf != ZB_BUF_INVALID);

        resp = (zb_zdo_mgmt_bind_resp_t *)zb_buf_initial_alloc(buf, sizeof(zb_zdo_mgmt_bind_resp_t));
        resp->tsn = cb_ent->zdp_tsn;

        if (status != RET_OK)
        {
            /* Pass only ZDO errors to the application layer */
            if (ERROR_GET_CATEGORY(status) == ERROR_CATEGORY_ZDO)
            {
                resp->status = ERROR_GET_CODE(status);
            }
            else
            {
                TRACE_MSG(TRACE_ERROR, "Unexpected type of error, error category = %hd, error status = %hd ",
                          (FMT__H_H, ERROR_GET_CATEGORY(status), ERROR_GET_CODE(status)));
                ZB_ASSERT(0);
            }
        }
        else
        {
            resp->status = ZB_ZDP_STATUS_SUCCESS;

            resp->binding_table_entries = binding_table_entries;
            resp->start_index = start_index;
            resp->binding_table_list_count = binding_table_list_count;

            if (resp->binding_table_list_count > 0)
            {
                resp_binding_table = (zb_zdo_binding_table_record_t *)zb_buf_alloc_right(buf,
                                     sizeof(zb_zdo_binding_table_record_t) * resp->binding_table_list_count);

                for (entry_index = 0; entry_index < resp->binding_table_list_count; entry_index++)
                {
                    ncp_resp_binding_entry = &binding_table[entry_index];
                    resp_binding_entry = &resp_binding_table[entry_index];

                    ZB_IEEE_ADDR_COPY(resp_binding_entry->src_address, ncp_resp_binding_entry->src_address);

                    resp_binding_entry->src_endp = ncp_resp_binding_entry->src_endp;
                    resp_binding_entry->cluster_id = ncp_resp_binding_entry->cluster_id;
                    resp_binding_entry->dst_addr_mode = ncp_resp_binding_entry->dst_addr_mode;

                    ZB_MEMCPY(&resp_binding_entry->dst_address, &ncp_resp_binding_entry->dst_address, sizeof(zb_addr_u));

                    resp_binding_entry->dst_endp = ncp_resp_binding_entry->dst_endp;
                }
            }
        }

        ncp_host_zdo_adapter_call_cb(buf, resp->status, cb_ent);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_zdo_mgmt_bind_response ", (FMT__0));
}


zb_uint8_t zb_zdo_mgmt_lqi_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_ret_t ret;
    zb_uint8_t zdp_tsn;
    zb_uint8_t ncp_tsn;
    zb_zdo_mgmt_lqi_param_t *req_param = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_lqi_param_t);
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_zdo_mgmt_lqi_req", (FMT__0));

    zdp_tsn = ncp_host_zdo_adapter_register_request(cb,
              ZB_NWK_IS_ADDRESS_BROADCAST(req_param->dst_addr),
              &cb_ent);

    if (zdp_tsn != ZB_ZDO_INVALID_TSN)
    {
        ret = ncp_host_zdo_mgmt_lqi_request(&ncp_tsn, req_param->dst_addr, req_param->start_index);
        ZB_ASSERT(ret == RET_OK);
        ncp_host_zdo_adapter_update_cb_ent(cb_ent, ZB_BUF_INVALID, ncp_tsn);
    }

    zb_buf_free(param);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_zdo_mgmt_lqi_req", (FMT__0));

    return zdp_tsn;
}


void ncp_host_handle_zdo_mgmt_lqi_response(zb_ret_t status, zb_uint8_t ncp_tsn,
        zb_uint8_t neighbor_table_entries,
        zb_uint8_t start_index,
        zb_uint8_t neighbor_table_list_count,
        ncp_host_zb_zdo_neighbor_table_record_t *neighbor_table)
{
    zb_bufid_t buf = ZB_BUF_INVALID;
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;
    zb_zdo_mgmt_lqi_resp_t *resp = NULL;
    zb_uindex_t entry_index = 0;
    zb_zdo_neighbor_table_record_t *resp_neighbor_table;

    ncp_host_zb_zdo_neighbor_table_record_t *ncp_resp_neighbor_entry;
    zb_zdo_neighbor_table_record_t *resp_neighbor_entry;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_zdo_mgmt_lqi_response %hd ", (FMT__H, ncp_tsn));

    get_cb_ent_by_ncp_tsn(ncp_tsn, &cb_ent);

    if (cb_ent != NULL)
    {
        buf = zb_buf_get(ZB_TRUE, 0);
        ZB_ASSERT(buf != ZB_BUF_INVALID);

        resp = (zb_zdo_mgmt_lqi_resp_t *)zb_buf_initial_alloc(buf, sizeof(zb_zdo_mgmt_lqi_resp_t));
        resp->tsn = cb_ent->zdp_tsn;

        if (status != RET_OK)
        {
            /* Pass only ZDO errors to the application layer */
            if (ERROR_GET_CATEGORY(status) == ERROR_CATEGORY_ZDO)
            {
                resp->status = ERROR_GET_CODE(status);
            }
            else
            {
                TRACE_MSG(TRACE_ERROR, "Unexpected type of error, error category = %hd, error status = %hd ",
                          (FMT__H_H, ERROR_GET_CATEGORY(status), ERROR_GET_CODE(status)));
                ZB_ASSERT(0);
            }
        }
        else
        {
            resp->status = ZB_ZDP_STATUS_SUCCESS;

            resp->neighbor_table_entries = neighbor_table_entries;
            resp->start_index = start_index;
            resp->neighbor_table_list_count = neighbor_table_list_count;

            if (resp->neighbor_table_list_count > 0)
            {
                resp_neighbor_table = (zb_zdo_neighbor_table_record_t *)zb_buf_alloc_right(buf,
                                      sizeof(zb_zdo_neighbor_table_record_t) * resp->neighbor_table_list_count);

                for (entry_index = 0; entry_index < resp->neighbor_table_list_count; entry_index++)
                {
                    ncp_resp_neighbor_entry = &neighbor_table[entry_index];
                    resp_neighbor_entry = &resp_neighbor_table[entry_index];

                    ZB_EXTPANID_COPY(resp_neighbor_entry->ext_pan_id, ncp_resp_neighbor_entry->ext_pan_id);
                    ZB_IEEE_ADDR_COPY(resp_neighbor_entry->ext_addr, ncp_resp_neighbor_entry->ext_addr);

                    resp_neighbor_entry->network_addr = ncp_resp_neighbor_entry->network_addr;
                    resp_neighbor_entry->type_flags = ncp_resp_neighbor_entry->type_flags;
                    resp_neighbor_entry->permit_join = ncp_resp_neighbor_entry->permit_join;
                    resp_neighbor_entry->depth = ncp_resp_neighbor_entry->depth;
                    resp_neighbor_entry->lqi = ncp_resp_neighbor_entry->lqi;
                }
            }
        }

        ncp_host_zdo_adapter_call_cb(buf, resp->status, cb_ent);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_zdo_mgmt_lqi_response ", (FMT__0));
}


zb_uint8_t zb_zdo_mgmt_nwk_update_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_ret_t ret;
    zb_uint8_t zdp_tsn;
    zb_uint8_t ncp_tsn;
    zb_zdo_mgmt_nwk_update_req_t *req_param = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_nwk_update_req_t);
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_zdo_mgmt_nwk_update_req", (FMT__0));

    zdp_tsn = ncp_host_zdo_adapter_register_request(cb,
              ZB_NWK_IS_ADDRESS_BROADCAST(req_param->dst_addr),
              &cb_ent);

    if (zdp_tsn != ZB_ZDO_INVALID_TSN)
    {
        ret = ncp_host_zdo_mgmt_nwk_update_request(&ncp_tsn, req_param->hdr.scan_channels,
                req_param->hdr.scan_duration, req_param->scan_count,
                req_param->manager_addr, req_param->dst_addr);
        ZB_ASSERT(ret == RET_OK);
        ncp_host_zdo_adapter_update_cb_ent(cb_ent, ZB_BUF_INVALID, ncp_tsn);
    }

    zb_buf_free(param);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_zdo_mgmt_nwk_update_req", (FMT__0));

    return zdp_tsn;
}


void ncp_host_handle_zdo_mgmt_nwk_update_response(zb_ret_t status, zb_uint8_t ncp_tsn,
        zb_uint32_t scanned_channels,
        zb_uint16_t total_transmissions,
        zb_uint16_t transmission_failures,
        zb_uint8_t scanned_channels_list_count,
        zb_uint8_t *energy_values)
{
    zb_bufid_t buf = ZB_BUF_INVALID;
    ncp_host_zdo_cb_ent_t *cb_ent = NULL;
    zb_zdo_mgmt_nwk_update_notify_hdr_t *resp = NULL;
    zb_uint8_t *resp_energy_values;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_zdo_mgmt_nwk_update_response ", (FMT__0));

    get_cb_ent_by_ncp_tsn(ncp_tsn, &cb_ent);


    if (cb_ent != NULL)
    {
        buf = zb_buf_get(ZB_TRUE, 0);
        ZB_ASSERT(buf != ZB_BUF_INVALID);

        resp = (zb_zdo_mgmt_nwk_update_notify_hdr_t *)zb_buf_initial_alloc(buf, sizeof(zb_zdo_mgmt_nwk_update_notify_hdr_t));
        resp->tsn = cb_ent->zdp_tsn;

        if (status != RET_OK)
        {
            /* Pass only ZDO errors to the application layer */
            if (ERROR_GET_CATEGORY(status) == ERROR_CATEGORY_ZDO)
            {
                resp->status = ERROR_GET_CODE(status);
            }
            else
            {
                TRACE_MSG(TRACE_ERROR, "Unexpected type of error, error category = %hd, error status = %hd ",
                          (FMT__H_H, ERROR_GET_CATEGORY(status), ERROR_GET_CODE(status)));
                ZB_ASSERT(0);
            }
        }
        else
        {
            resp->status = ZB_ZDP_STATUS_SUCCESS;

            resp->scanned_channels = scanned_channels;
            resp->total_transmissions = total_transmissions;
            resp->transmission_failures = transmission_failures;
            resp->scanned_channels_list_count = scanned_channels_list_count;

            if (resp->scanned_channels_list_count > 0)
            {
                resp_energy_values = (zb_uint8_t *)zb_buf_alloc_right(buf,
                                     sizeof(zb_uint8_t) * resp->scanned_channels_list_count);

                ZB_MEMCPY(resp_energy_values, energy_values, sizeof(zb_uint8_t) * resp->scanned_channels_list_count);
            }
        }

        ncp_host_zdo_adapter_call_cb(buf, resp->status, cb_ent);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_zdo_mgmt_nwk_update_response ", (FMT__0));
}
