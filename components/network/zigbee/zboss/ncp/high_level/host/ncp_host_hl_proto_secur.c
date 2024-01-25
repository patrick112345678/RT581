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
/*  PURPOSE: NCP High level transport implementation for the host side: SECUR category
*/

#define ZB_TRACE_FILE_ID 18

#include "ncp_host_hl_proto.h"

static void handle_secur_add_ic_response(ncp_hl_response_header_t *response,
        zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_secur_ic_add_response, status_code %d",
              (FMT__D, error_code));

    ncp_host_mark_blocking_request_finished();

    ncp_host_handle_secur_add_ic_response(error_code);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_secur_ic_add_response", (FMT__0));
}

void ncp_host_handle_secur_response(void *data, zb_uint16_t len)
{
    ncp_hl_response_header_t *response_header = (ncp_hl_response_header_t *)data;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_secur_response", (FMT__0));

    switch (response_header->hdr.call_id)
    {
    case NCP_HL_SECUR_ADD_IC:
        handle_secur_add_ic_response(response_header, len);
        break;
    default:
        /* TODO: implement handlers for other responses! */
        ZB_ASSERT(0);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_secur_response", (FMT__0));
}

void ncp_host_handle_secur_indication(void *data, zb_uint16_t len)
{
    /* TODO: implement it! */
    ZB_ASSERT(0);
}

zb_ret_t ncp_host_secur_add_ic(zb_ieee_addr_t address, zb_uint8_t ic_type, zb_uint8_t *ic)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_secur_add_ic, address " TRACE_FORMAT_64 ", ic_type %d ",
              (FMT__A_D, TRACE_ARG_64(address), ic_type));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_SECUR_ADD_IC, &body))
    {
        ncp_host_hl_buf_put_u64addr(&body, address);
        ncp_host_hl_buf_put_array(&body, ic, ZB_IC_SIZE_BY_TYPE(ic_type) + 2);
        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_secur_add_ic, ret %d", (FMT__D, ret));

    return ret;
}
