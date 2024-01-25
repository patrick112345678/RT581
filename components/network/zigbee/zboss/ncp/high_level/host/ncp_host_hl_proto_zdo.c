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
/*  PURPOSE: NCP High level transport implementation for the host side: ZDO category
*/

#define ZB_TRACE_FILE_ID 15

#include "ncp_host_hl_proto.h"

static void handle_zdo_dev_annce_indication(ncp_hl_ind_header_t *indication, zb_uint16_t len)
{
    ncp_host_hl_rx_buf_handle_t body;
    zb_zdo_signal_device_annce_params_t da;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_zdo_dev_annce_indication ", (FMT__0));

    TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_zdo_dev_annce_indication", (FMT__0));

    ncp_host_hl_init_indication_body(indication, len, &body);
    ncp_host_hl_buf_get_u16(&body, &da.device_short_addr);
    ncp_host_hl_buf_get_u64addr(&body, da.ieee_addr);
    ncp_host_hl_buf_get_u8(&body, &da.capability);

    TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_zdo_dev_annce_indication, short_addr 0x%x, long_addr " TRACE_FORMAT_64 " mac_capabilities %hd",
              (FMT__D_A_H, da.device_short_addr, TRACE_ARG_64(da.ieee_addr), da.capability));

    ncp_host_handle_zdo_dev_annce_indication(&da);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_zdo_dev_annce_indication", (FMT__0));
}

void ncp_host_handle_zdo_response(void *data, zb_uint16_t len)
{
    /* TODO: implement it! */
    ZB_ASSERT(0);
}

void ncp_host_handle_zdo_indication(void *data, zb_uint16_t len)
{
    ncp_hl_ind_header_t *indication_header = (ncp_hl_ind_header_t *)data;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_zdo_indication", (FMT__0));

    switch (indication_header->call_id)
    {
    case NCP_HL_ZDO_DEV_ANNCE_IND:
        handle_zdo_dev_annce_indication(indication_header, len);
        break;
    default:
        /* TODO: implement handlers for other responses! */
        ZB_ASSERT(0);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_zdo_indication", (FMT__0));
}
