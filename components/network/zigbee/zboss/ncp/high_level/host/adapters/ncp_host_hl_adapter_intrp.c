/* ZBOSS Zigbee software protocol stack
 *
 * Copyright (c) 2021-2021 Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Commercial Usage
 * Licensees holding valid Commercial licenses may use
 * this file in accordance with the Commercial License
 * Agreement provided with the Software or, alternatively, in accordance
 * with the terms contained in a written agreement relevant to the usage of the file.
 */
/*  PURPOSE: NCP High level transport (adapters layer) implementation for the host side: Inter-PAN category
*/
#define ZB_TRACE_FILE_ID 17517

#include "zb_common.h"
#include "ncp_host_hl_transport_internal_api.h"
#include "ncp_host_soc_state.h"
#include "zboss_api_aps_interpan.h"

#ifdef ZB_ENABLE_INTER_PAN_NON_DEFAULT_CHANNEL


/* Context for keeping buffers for Inter-PAN confirms after sending requests. Must use
 * the same buffer for Inter-PAN request and its confirm. */
typedef struct host_intrp_adapter_ctx_s
{
    /* Buffers accordance table for APS Data Request and Confirms */
    zb_bufid_t buf;
    zb_uint8_t tsn;
    zb_callback_t cb;
    zb_af_inter_pan_handler_t af_intrp_data_cb;
} intrp_adapter_ctx_t;

static intrp_adapter_ctx_t g_adapter_ctx;


void ncp_host_intrp_adapter_init_ctx(void)
{
    ZB_BZERO(&g_adapter_ctx, sizeof(g_adapter_ctx));
}

void ncp_host_handle_intrp_data_indication(ncp_host_zb_intrp_data_ind_t *ncp_ind,
        zb_uint8_t *data_ptr,
        zb_uint8_t params_len, zb_uint16_t data_len)
{
    zb_bufid_t buf = zb_buf_get(ZB_TRUE, 0);
    zb_uint8_t *ptr = NULL;
    zb_bool_t packet_processed = ZB_FALSE;
    zb_intrp_data_ind_t ind;

    ZVUNUSED(params_len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_intrp_data_indication ", (FMT__0));

    ZB_ASSERT(buf != ZB_BUF_INVALID);

    /* Construct application parameters. */
    ind.dst_addr_mode = ncp_ind->dst_addr_mode;
    ind.dst_pan_id = ncp_ind->dst_pan_id;
    ind.dst_addr = ncp_ind->dst_addr;
    ind.src_pan_id = ncp_ind->src_pan_id;
    ZB_MEMCPY(ind.src_addr, ncp_ind->src_addr, sizeof(zb_ieee_addr_t));
    ind.profile_id = ncp_ind->profile_id;
    ind.cluster_id = ncp_ind->cluster_id;
    ind.link_quality = ncp_ind->link_quality;
    ind.rssi = ncp_ind->rssi;

    /* Construct buffer for the application. */
    ptr = (zb_uint8_t *)zb_buf_initial_alloc(buf, data_len);
    ZB_MEMCPY(ptr, data_ptr, data_len);
    ZB_MEMCPY(ZB_BUF_GET_PARAM(buf, zb_intrp_data_ind_t), &ind, sizeof(zb_intrp_data_ind_t));

    if (g_adapter_ctx.af_intrp_data_cb != NULL)
    {
        packet_processed = (zb_bool_t)(g_adapter_ctx.af_intrp_data_cb(buf, ncp_ind->page, ncp_ind->channel));
    }

    if (!packet_processed)
    {
        TRACE_MSG(TRACE_ZDO1, "can't handle Inter-PAN packet %hd, drop it", (FMT__H, buf));
        zb_buf_free(buf);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_intrp_data_indication", (FMT__0));
}


void ncp_host_handle_intrp_data_response(ncp_host_zb_intrp_data_res_t *ncp_conf, zb_uint8_t tsn)
{
    zb_mchan_intrp_data_confirm_t *conf;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_intrp_data_response, tsn %hd", (FMT__H, tsn));

    if (g_adapter_ctx.buf == ZB_BUF_INVALID)
    {
        TRACE_MSG(TRACE_ERROR, "Unsolicited Inter-PAN data response received", (FMT__0));
        TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_intrp_data_response ", (FMT__0));
        return;
    }

    conf = ZB_BUF_GET_PARAM(g_adapter_ctx.buf, zb_mchan_intrp_data_confirm_t);
    conf->channel_status_page_mask = ncp_conf->channel_status_page_mask;
    conf->asdu_handle = ncp_conf->asdu_handle;
    zb_buf_set_status(g_adapter_ctx.buf, ncp_conf->status);

    if (g_adapter_ctx.cb != NULL)
    {
        g_adapter_ctx.cb(g_adapter_ctx.buf);
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "can't handle response for Inter-PAN packet (buf: %d), drop it",
                  (FMT__D, g_adapter_ctx.buf));
        zb_buf_free(g_adapter_ctx.buf);
    }

    /* Clear the adater TX context. */
    g_adapter_ctx.buf = ZB_BUF_INVALID;
    g_adapter_ctx.cb = NULL;

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_intrp_data_response ", (FMT__0));
}

zb_ret_t zb_intrp_data_request_with_chan_change(zb_bufid_t buffer, zb_channel_page_t channel_page_mask, zb_uint32_t chan_wait_ms, zb_callback_t cb)
{
    ncp_host_zb_intrp_data_req_t ncp_req;
    zb_intrp_data_req_t *req = ZB_BUF_GET_PARAM(buffer, zb_intrp_data_req_t);
    zb_uint8_t *data_ptr = NULL;
    zb_uint8_t tsn = 0;
    zb_ret_t ret = RET_OK;

    TRACE_MSG(TRACE_TRANSPORT3, ">> zb_intrp_data_request_with_chan_change", (FMT__0));

    ZB_ASSERT(buffer);

    if (g_adapter_ctx.buf != 0)
    {
        ret = RET_BUSY;
    }

    if (ret == RET_OK)
    {
        ZB_MEMCPY(&ncp_req.dst_addr, &req->dst_addr, sizeof(zb_addr_u));

        ncp_req.dst_addr_mode = req->dst_addr_mode;
        ncp_req.dst_pan_id = req->dst_pan_id;
        ncp_req.profile_id = req->profile_id;
        ncp_req.cluster_id = req->cluster_id;
        ncp_req.asdu_handle = req->asdu_handle;
        ncp_req.channel_page_mask = channel_page_mask;
        ncp_req.chan_wait_ms = chan_wait_ms;

        data_ptr = (zb_uint8_t *)zb_buf_begin(buffer);

        ret = ncp_host_intrp_data_request(&ncp_req, sizeof(ncp_host_zb_intrp_data_req_t), zb_buf_len(buffer), data_ptr, &tsn);
    }

    if (ret == RET_OK)
    {
        g_adapter_ctx.buf = buffer;
        g_adapter_ctx.tsn = tsn;
        g_adapter_ctx.cb = cb;
    }
    else
    {
        TRACE_MSG(TRACE_TRANSPORT3, "ncp_host_apsde_data_request failed with status %d", (FMT__D, ret));
        zb_buf_free(buffer);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "Inter-PAN data request command is processed, ret %d, tsn %hd",
              (FMT__D_H, ret, tsn));

    TRACE_MSG(TRACE_TRANSPORT3, "<< zb_intrp_data_request_with_chan_change", (FMT__0));

    return ret;
}

void zb_af_interpan_set_data_indication(zb_af_inter_pan_handler_t cb)
{
    g_adapter_ctx.af_intrp_data_cb = cb;
}

#endif /* ZB_ENABLE_INTER_PAN_NON_DEFAULT_CHANNEL */
