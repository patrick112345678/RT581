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
/*  PURPOSE: NCP High level transport implementation for the host side: Inter-PAN category
*/
#define ZB_TRACE_FILE_ID 17516

#include "ncp_host_hl_proto.h"

#ifdef ZB_ENABLE_INTER_PAN_NON_DEFAULT_CHANNEL


static void handle_intrp_data_indication(ncp_hl_ind_header_t *indication, zb_uint16_t len)
{
    ncp_host_hl_rx_buf_handle_t body;

    zb_uint8_t params_len;
    zb_uint16_t data_len;
    ncp_host_zb_intrp_data_ind_t ind;
    zb_uint8_t *data_ptr;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_intrp_data_indication ", (FMT__0));

    TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_intrp_data_indication", (FMT__0));

    ncp_host_hl_init_indication_body(indication, len, &body);

    /* Get additional NCP Serial Protocol info */
    ncp_host_hl_buf_get_u8(&body, &params_len);
    ncp_host_hl_buf_get_u16(&body, &data_len);

    TRACE_MSG(TRACE_TRANSPORT3, " params_len %hd, data_len %d",
              (FMT__H_D, params_len, data_len));

    /* Get the beginning of the ncp_host_zb_intrp_data_ind_t */
    ncp_host_hl_buf_get_u8(&body, &ind.dst_addr_mode);
    ncp_host_hl_buf_get_u16(&body, &ind.dst_pan_id);

    /* Always fetch 8 bytes, align byte order if the short address was sent. */
    ncp_host_hl_buf_get_u64addr(&body, ind.dst_addr.addr_long);
    if (ind.dst_addr_mode == ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT
            || ind.dst_addr_mode == ZB_APS_ADDR_MODE_16_ENDP_PRESENT)
    {
        ZB_LETOH16_ONPLACE(ind.dst_addr.addr_short);
    }

    /* Get the rest of the command. */
    ncp_host_hl_buf_get_u16(&body, &ind.src_pan_id);
    ncp_host_hl_buf_get_u64addr(&body, ind.src_addr);
    ncp_host_hl_buf_get_u16(&body, &ind.profile_id);
    ncp_host_hl_buf_get_u16(&body, &ind.cluster_id);
    ncp_host_hl_buf_get_u8(&body, &ind.link_quality);
    ncp_host_hl_buf_get_u8(&body, (zb_uint8_t *)&ind.rssi);
    ncp_host_hl_buf_get_u8(&body, &ind.page);
    ncp_host_hl_buf_get_u8(&body, &ind.channel);

    /* Get the pointer to the data, associated with the command. */
    ncp_host_hl_buf_get_ptr(&body, &data_ptr, data_len);

    TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_intrp_data_indication", (FMT__0));

    ncp_host_handle_intrp_data_indication(&ind, data_ptr, params_len, data_len);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_intrp_data_indication", (FMT__0));
}

static void handle_intrp_data_response(ncp_hl_response_header_t *response,
                                       zb_uint16_t len)
{
    ncp_host_zb_intrp_data_res_t conf;
    ncp_host_hl_rx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_intrp_data_response, status_code %d",
              (FMT__D, ERROR_CODE(response->status_category, response->status_code)));

    /* Get additional NCP Serial Protocol info */
    ncp_host_hl_init_response_body(response, len, &body);
    conf.status = ERROR_CODE(response->status_category, response->status_code);

    /* Get the command. */
    ncp_host_hl_buf_get_u32(&body, &conf.channel_status_page_mask);
    TRACE_MSG(TRACE_TRANSPORT3, "channel_status_page_mask %ld", (FMT__L, conf.channel_status_page_mask));
    ncp_host_hl_buf_get_u8(&body, &conf.asdu_handle);
    TRACE_MSG(TRACE_TRANSPORT3, "asdu_handle %hd", (FMT__H, conf.asdu_handle));

    /* Pass the response to the adapter layer. */
    ncp_host_handle_intrp_data_response(&conf, response->tsn);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_intrp_data_response", (FMT__0));
}

void ncp_host_handle_intrp_response(void *data, zb_uint16_t len)
{
    ncp_hl_response_header_t *response_header = (ncp_hl_response_header_t *)data;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_intrp_response", (FMT__0));

    switch (response_header->hdr.call_id)
    {
    case NCP_HL_INTRP_DATA_REQ:
        handle_intrp_data_response(response_header, len);
        break;

    default:
        ZB_ASSERT(0);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_intrp_response", (FMT__0));
}

void ncp_host_handle_intrp_indication(void *data, zb_uint16_t len)
{
    ncp_hl_ind_header_t *indication_header = (ncp_hl_ind_header_t *)data;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_intrp_indication", (FMT__0));

    switch (indication_header->call_id)
    {
    case NCP_HL_INTRP_DATA_IND:
        handle_intrp_data_indication(indication_header, len);
        break;
    default:
        ZB_ASSERT(0);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_intrp_indication", (FMT__0));
}


zb_ret_t ncp_host_intrp_data_request(ncp_host_zb_intrp_data_req_t *req, zb_uint8_t param_len,
                                     zb_uint16_t data_len, zb_uint8_t *data_ptr,
                                     zb_uint8_t *tsn)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_intrp_data_request", (FMT__0));

    /* Allocate a new NCP buffer and construct the new NCP command. */
    if (!ncp_host_get_buf_for_nonblocking_request(NCP_HL_INTRP_DATA_REQ, &body, tsn))
    {
        /* Put generic NCP Serial Protocol fields. */
        ncp_host_hl_buf_put_u8(&body, param_len);
        TRACE_MSG(TRACE_TRANSPORT3, "param_len %hd", (FMT__H, param_len));

        ncp_host_hl_buf_put_u16(&body, data_len);
        TRACE_MSG(TRACE_TRANSPORT3, "data_len %d", (FMT__H, data_len));

        /* Put all fields of the ncp_host_zb_intrp_data_req_t. */
        ncp_host_hl_buf_put_u8(&body, req->dst_addr_mode);
        TRACE_MSG(TRACE_TRANSPORT3, "req->addr_mode 0x%hx", (FMT__H, req->dst_addr_mode));

        ncp_host_hl_buf_put_u16(&body, req->dst_pan_id);
        TRACE_MSG(TRACE_TRANSPORT3, "req->dst_pan_id 0x%hx", (FMT__H, req->dst_pan_id));

        if (req->dst_addr_mode == ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT || req->dst_addr_mode == ZB_APS_ADDR_MODE_16_ENDP_PRESENT)
        {
            /* In case of short address - pad the field with two zeros, so the length is the same as with the long address. */
            ncp_host_hl_buf_put_u16(&body, req->dst_addr.addr_short);
            ncp_host_hl_buf_put_u16(&body, 0);
            ncp_host_hl_buf_put_u32(&body, 0);
            TRACE_MSG(TRACE_TRANSPORT3, "req->dst_addr.addr_short 0x%x", (FMT__A, req->dst_addr.addr_short));
        }
        else
        {
            ncp_host_hl_buf_put_u64addr(&body, req->dst_addr.addr_long);
            TRACE_MSG(TRACE_TRANSPORT3, "req->dst_addr.addr_long " TRACE_FORMAT_64,
                      (FMT__A, TRACE_ARG_64(req->dst_addr.addr_long)));
        }

        ncp_host_hl_buf_put_u16(&body, req->profile_id);
        TRACE_MSG(TRACE_TRANSPORT3, "req->profile_id %d", (FMT__D, req->profile_id));

        ncp_host_hl_buf_put_u16(&body, req->cluster_id);
        TRACE_MSG(TRACE_TRANSPORT3, "req->cluster_id %d", (FMT__D, req->cluster_id));

        ncp_host_hl_buf_put_u8(&body, req->asdu_handle);
        TRACE_MSG(TRACE_TRANSPORT3, "req->asdu_handle %hd", (FMT__H, req->asdu_handle));

        ncp_host_hl_buf_put_u32(&body, req->channel_page_mask);
        TRACE_MSG(TRACE_TRANSPORT3, "req->channel_page_mask 0x%x", (FMT__L, req->channel_page_mask));

        ncp_host_hl_buf_put_u32(&body, req->chan_wait_ms);
        TRACE_MSG(TRACE_TRANSPORT3, "req->chan_wait_ms %ld", (FMT__L, req->chan_wait_ms));

        /* Copy command data as-is. */
        ncp_host_hl_buf_put_array(&body, data_ptr, data_len);

        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_intrp_data_request, ret %d", (FMT__D, ret));

    return ret;
}

#endif /* ZB_ENABLE_INTER_PAN_NON_DEFAULT_CHANNEL */
