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
/*  PURPOSE: NCP High level transport implementation for the host side: NWK MGMT category
*/

#define ZB_TRACE_FILE_ID 17508
#include "ncp_host_hl_proto.h"

#ifdef ZB_FORMATION

static void handle_nwk_formation_response(ncp_hl_response_header_t *response,
        zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    ncp_host_hl_rx_buf_handle_t body;
    zb_uint16_t short_address = 0xFFFF;
    zb_uint16_t pan_id = 0xFFFF;
    zb_uint8_t page = 0;
    zb_uint8_t channel = 0;

    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_formation_response, status_code %d",
              (FMT__D, error_code));

    if (error_code == RET_OK)
    {
        ncp_host_hl_init_response_body(response, len, &body);

        ncp_host_hl_buf_get_u16(&body, &short_address);
        ncp_host_hl_buf_get_u16(&body, &pan_id);
        ncp_host_hl_buf_get_u8(&body, &page);
        ncp_host_hl_buf_get_u8(&body, &channel);

        TRACE_MSG(TRACE_TRANSPORT3, "short address 0x%x, PAN ID 0x%x, page %d, channel %d",
                  (FMT__D_D_D_D, short_address, pan_id, page, channel));
    }

    ncp_host_mark_blocking_request_finished();

    ncp_host_handle_nwk_formation_response(error_code, short_address, pan_id, page, channel);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_formation_response", (FMT__0));
}

static void handle_nwk_start_without_formation_response(ncp_hl_response_header_t *response,
        zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_start_without_formation_response, status_code %d",
              (FMT__D, error_code));

    ncp_host_mark_blocking_request_finished();

    ncp_host_handle_nwk_start_without_formation_response(error_code);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_start_without_formation_response", (FMT__0));
}

#endif /* ZB_FORMATION */

static void handle_nwk_discovery_response(ncp_hl_response_header_t *response,
        zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    ncp_host_hl_rx_buf_handle_t body;
    zb_uint8_t network_count = 0;
    ncp_proto_network_descriptor_t *network_descriptors_ptr = NULL;
    zb_uindex_t i = 0;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_discovery_response, status_code %d",
              (FMT__D, error_code));

    if (error_code == RET_OK)
    {
        TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_nwk_discovery_response", (FMT__0));

        ncp_host_hl_init_response_body(response, len, &body);

        ncp_host_hl_buf_get_u8(&body, &network_count);
        TRACE_MSG(TRACE_TRANSPORT3, " network_count = %hd", (FMT__H, network_count));

        ncp_host_hl_buf_get_ptr(&body,
                                (zb_uint8_t **)&network_descriptors_ptr,
                                sizeof(ncp_proto_network_descriptor_t) * network_count);

        for (i = 0; i < network_count; i++)
        {
            ZB_LETOH16_ONPLACE(network_descriptors_ptr[i].pan_id);
        }

        TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_nwk_discovery_response ", (FMT__0));
    }

    ncp_host_mark_blocking_request_finished();

    ncp_host_handle_nwk_discovery_response(error_code, network_count,
                                           network_descriptors_ptr);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_discovery_response", (FMT__0));
}

static void handle_nwk_nlme_join_response(ncp_hl_response_header_t *response,
        zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    ncp_host_hl_rx_buf_handle_t body;
    zb_uint16_t short_addr;
    zb_ext_pan_id_t extended_pan_id;
    zb_uint8_t channel_page;
    zb_uint8_t logical_channel;
    zb_uint8_t enhanced_beacon;
    zb_uint8_t mac_interface;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_nlme_join_response, status_code %d",
              (FMT__D, error_code));

    if (error_code == RET_OK)
    {
        TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_nwk_nlme_join_response", (FMT__0));

        ncp_host_hl_init_response_body(response, len, &body);

        ncp_host_hl_buf_get_u16(&body, &short_addr);
        TRACE_MSG(TRACE_TRANSPORT3, " short_addr = %d", (FMT__D, short_addr));

        ncp_host_hl_buf_get_u64addr(&body, extended_pan_id);
        TRACE_MSG(TRACE_TRANSPORT3, " extended_pan_id = " TRACE_FORMAT_64,
                  (FMT__A, TRACE_ARG_64(extended_pan_id)));

        ncp_host_hl_buf_get_u8(&body, &channel_page);
        TRACE_MSG(TRACE_TRANSPORT3, " channel_page = %hd", (FMT__H, channel_page));

        ncp_host_hl_buf_get_u8(&body, &logical_channel);
        TRACE_MSG(TRACE_TRANSPORT3, " logical_channel = %hd", (FMT__H, logical_channel));

        ncp_host_hl_buf_get_u8(&body, &enhanced_beacon);
        TRACE_MSG(TRACE_TRANSPORT3, " enhanced_beacon = %hd", (FMT__H, enhanced_beacon));

        ncp_host_hl_buf_get_u8(&body, &mac_interface);
        TRACE_MSG(TRACE_TRANSPORT3, " mac_interface = %hd", (FMT__H, mac_interface));

        TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_nwk_nlme_join_response ", (FMT__0));
    }

    ncp_host_mark_blocking_request_finished();

    ncp_host_handle_nwk_nlme_join_response(error_code, short_addr, extended_pan_id, channel_page,
                                           logical_channel, enhanced_beacon, mac_interface);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_nlme_join_response", (FMT__0));
}

static void handle_nwk_permit_joining_response(ncp_hl_response_header_t *response,
        zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_permit_joining_response, status_code %d",
              (FMT__D, error_code));

    ncp_host_mark_blocking_request_finished();

    ncp_host_handle_nwk_permit_joining_response(error_code);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_permit_joining_response", (FMT__0));
}

static void handle_nwk_get_ieee_by_short_response(ncp_hl_response_header_t *response,
        zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    zb_ieee_addr_t ieee_addr;
    ncp_host_hl_rx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_get_ieee_by_short_response, status_code %d",
              (FMT__D, error_code));

    if (error_code == RET_OK)
    {
        TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_nwk_get_ieee_by_short_response", (FMT__0));

        ncp_host_hl_init_response_body(response, len, &body);
        ncp_host_hl_buf_get_u64addr(&body, ieee_addr);

        TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_nwk_get_ieee_by_short_response, ieee_addr " TRACE_FORMAT_64,
                  (FMT__A, TRACE_ARG_64(ieee_addr)));
    }
    ncp_host_mark_blocking_request_finished();

    ncp_host_handle_nwk_get_ieee_by_short_response(error_code, ieee_addr);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_get_ieee_by_short_response", (FMT__0));
}


static void handle_nwk_get_neighbor_by_ieee_response(ncp_hl_response_header_t *response,
        zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    ncp_host_hl_rx_buf_handle_t body;
    ncp_hl_response_neighbor_by_ieee_t ncp_nbt;

    TRACE_MSG(TRACE_TRANSPORT3, ">>  handle_nwk_get_neighbor_by_ieee, status_code %d",
              (FMT__D, error_code));

    if (error_code == RET_OK)
    {
        TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_nwk_get_neighbor_by_ieee", (FMT__0));

        ncp_host_hl_init_response_body(response, len, &body);

        ncp_host_hl_buf_get_u64addr(&body, ncp_nbt.ieee_addr);
        TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt.ieee_addr " TRACE_FORMAT_64,
                  (FMT__A, TRACE_ARG_64(ncp_nbt.ieee_addr)));

        ncp_host_hl_buf_get_u16(&body, &ncp_nbt.short_addr);
        TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt.short_addr 0x%x", (FMT__D, ncp_nbt.short_addr));

        ncp_host_hl_buf_get_u8(&body, &ncp_nbt.device_type);
        TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt.device_type 0x%x", (FMT__D, ncp_nbt.device_type));

        ncp_host_hl_buf_get_u8(&body, &ncp_nbt.rx_on_when_idle);
        TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt.rx_on_when_idle 0x%x", (FMT__D, ncp_nbt.rx_on_when_idle));

        ncp_host_hl_buf_get_u16(&body, &ncp_nbt.ed_config);
        TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt.ed_config 0x%x", (FMT__D, ncp_nbt.ed_config));

        ncp_host_hl_buf_get_u32(&body, &ncp_nbt.timeout_counter);
        TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt.timeout_counter %d", (FMT__D, ncp_nbt.timeout_counter));

        ncp_host_hl_buf_get_u32(&body, &ncp_nbt.device_timeout);
        TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt.device_timeout %d", (FMT__D, ncp_nbt.device_timeout));

        ncp_host_hl_buf_get_u8(&body, &ncp_nbt.relationship);
        TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt.relationship 0x%x", (FMT__D, ncp_nbt.relationship));

        ncp_host_hl_buf_get_u8(&body, &ncp_nbt.transmit_failure_cnt);
        TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt.transmit_failure_cnt %d", (FMT__D, ncp_nbt.transmit_failure_cnt));

        ncp_host_hl_buf_get_u8(&body, &ncp_nbt.lqi);
        TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt.lqi %d", (FMT__D, ncp_nbt.lqi));

#ifdef ZB_ROUTER_ROLE
        ncp_host_hl_buf_get_u8(&body, &ncp_nbt.outgoing_cost);
        TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt.outgoing_cost %d", (FMT__D, ncp_nbt.outgoing_cost));

        ncp_host_hl_buf_get_u8(&body, &ncp_nbt.age);
        TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt.age %d", (FMT__D, ncp_nbt.age));
#endif /* ZB_ROUTER_ROLE */

        ncp_host_hl_buf_get_u8(&body, &ncp_nbt.keepalive_received);
        TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt.keepalive_received %d", (FMT__D, ncp_nbt.keepalive_received));

        ncp_host_hl_buf_get_u8(&body, &ncp_nbt.mac_iface_idx);
        TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt.mac_iface_idx %d", (FMT__D, ncp_nbt.mac_iface_idx));

        /* TODO: implement converting ncp_hl_response_neighbor_by_ieee_t to zb_neighbor_tbl_ent_t.
                 We don't use NWK Get Neighbor by IEEE command on NCP Host,
                 so we can leave it as is for now. */
        TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_nwk_get_neighbor_by_ieee", (FMT__0));

    }
    ncp_host_mark_blocking_request_finished();

    ncp_host_handle_nwk_get_neighbor_by_ieee_response(error_code);

    TRACE_MSG(TRACE_TRANSPORT3, "<<  handle_nwk_get_neighbor_by_ieee", (FMT__0));
}


static void handle_nwk_get_short_by_ieee_response(ncp_hl_response_header_t *response,
        zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    zb_uint16_t short_addr;
    ncp_host_hl_rx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_get_short_by_ieee_response, status_code %d",
              (FMT__D, error_code));

    if (error_code == RET_OK)
    {
        TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_nwk_get_short_by_ieee_response", (FMT__0));

        ncp_host_hl_init_response_body(response, len, &body);
        ncp_host_hl_buf_get_u16(&body, &short_addr);

        TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_nwk_get_short_by_ieee_response, short_addr 0x%x",
                  (FMT__D, short_addr));
    }
    ncp_host_mark_blocking_request_finished();

    ncp_host_handle_nwk_get_short_by_ieee_response(error_code, short_addr);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_get_short_by_ieee_response", (FMT__0));
}


static void handle_nwk_set_fast_poll_interval_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_set_fast_poll_interval_response, status_code %d",
              (FMT__D, error_code));

    ncp_host_mark_blocking_request_finished();

    ncp_host_handle_nwk_set_fast_poll_interval_response(error_code);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_set_fast_poll_interval_response", (FMT__0));
}


static void handle_nwk_set_long_poll_interval_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_set_long_poll_interval_response, status_code %d",
              (FMT__D, error_code));

    ncp_host_mark_blocking_request_finished();

    ncp_host_handle_nwk_set_long_poll_interval_response(error_code);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_set_long_poll_interval_response", (FMT__0));
}


static void handle_nwk_start_fast_poll_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_start_fast_poll_response, status_code %d",
              (FMT__D, error_code));

    ncp_host_mark_blocking_request_finished();

    ncp_host_handle_nwk_start_fast_poll_response(error_code);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_start_fast_poll_response", (FMT__0));
}


static void handle_nwk_start_poll_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_start_poll_response, status_code %d",
              (FMT__D, error_code));

    ncp_host_mark_blocking_request_finished();

    ncp_host_handle_nwk_start_poll_response(error_code);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_start_poll_response", (FMT__0));
}


static void handle_nwk_stop_fast_poll_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    zb_uint8_t ncp_tsn = response->tsn;
    zb_uint8_t stop_result;
    ncp_host_hl_rx_buf_handle_t body;

    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_stop_fast_poll_response, status_code %d",
              (FMT__D, error_code));

    if (error_code == RET_OK)
    {
        TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_nwk_stop_fast_poll_response", (FMT__0));

        ncp_host_hl_init_response_body(response, len, &body);
        ncp_host_hl_buf_get_u8(&body, &stop_result);

        TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_nwk_stop_fast_poll_response, result %hd",
                  (FMT__H, stop_result));
    }

    ncp_host_mark_blocking_request_finished();

    ncp_host_handle_nwk_stop_fast_poll_response(error_code, ncp_tsn, stop_result);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_stop_fast_poll_response", (FMT__0));
}


static void handle_nwk_stop_poll_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_stop_poll_response, status_code %d",
              (FMT__D, error_code));

    ncp_host_mark_blocking_request_finished();

    ncp_host_handle_nwk_stop_poll_response(error_code);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_stop_poll_response", (FMT__0));
}


static void handle_nwk_enable_turbo_poll_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_enable_turbo_poll_response, status_code %d",
              (FMT__D, error_code));

    ncp_host_mark_blocking_request_finished();

    ncp_host_handle_nwk_enable_turbo_poll_response(error_code);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_enable_turbo_poll_response", (FMT__0));
}


static void handle_nwk_disable_turbo_poll_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_disable_turbo_poll_response, status_code %d",
              (FMT__D, error_code));

    ncp_host_mark_blocking_request_finished();

    ncp_host_handle_nwk_disable_turbo_poll_response(error_code);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_disable_turbo_poll_response", (FMT__0));
}


static void handle_nwk_start_turbo_poll_packets_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_start_turbo_poll_packets_response, status_code %d",
              (FMT__D, error_code));

    ncp_host_mark_blocking_request_finished();

    ncp_host_handle_nwk_start_turbo_poll_packets_response(error_code);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_start_turbo_poll_packets_response", (FMT__0));
}


static void handle_nwk_turbo_poll_cancel_packet_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_turbo_poll_cancel_packet_response, status_code %d",
              (FMT__D, error_code));

    ncp_host_mark_blocking_request_finished();

    ncp_host_handle_nwk_turbo_poll_cancel_packet_response(error_code);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_turbo_poll_cancel_packet_response", (FMT__0));
}


static void handle_nwk_start_turbo_poll_continuous_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_start_turbo_poll_continuous_response, status_code %d",
              (FMT__D, error_code));

    ncp_host_mark_blocking_request_finished();

    ncp_host_handle_nwk_start_turbo_poll_continuous_response(error_code);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_start_turbo_poll_continuous_response", (FMT__0));
}


static void handle_nwk_permit_turbo_poll_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_permit_turbo_poll_response, status_code %d",
              (FMT__D, error_code));

    ncp_host_mark_blocking_request_finished();

    ncp_host_handle_nwk_permit_turbo_poll_response(error_code);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_permit_turbo_poll_response", (FMT__0));
}


static void handle_nwk_set_keepalive_mode_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_set_keepalive_mode_response, status_code %d",
              (FMT__D, error_code));

    ncp_host_mark_blocking_request_finished();

    ncp_host_handle_nwk_set_keepalive_mode_response(error_code);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_set_keepalive_mode_response", (FMT__0));
}


static void handle_nwk_start_concentrator_mode_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);

    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_start_concentrator_mode_response, status_code %d",
              (FMT__D, error_code));

    ncp_host_mark_blocking_request_finished();

    ncp_host_handle_nwk_start_concentrator_mode_response(error_code);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_start_concentrator_mode_response", (FMT__0));
}


static void handle_nwk_stop_concentrator_mode_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);

    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_stop_concentrator_mode_response, status_code %d",
              (FMT__D, error_code));

    ncp_host_mark_blocking_request_finished();

    ncp_host_handle_nwk_stop_concentrator_mode_response(error_code);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_stop_concentrator_mode_response", (FMT__0));
}


static void handle_nwk_set_fast_poll_timeout_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_set_fast_poll_timeout_response, status_code %d",
              (FMT__D, error_code));

    ncp_host_mark_blocking_request_finished();

    ncp_host_handle_nwk_set_fast_poll_timeout_response(error_code);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_set_fast_poll_timeout_response", (FMT__0));
}


static void handle_nwk_get_long_poll_interval_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    zb_uint8_t ncp_tsn = response->tsn;
    zb_uint32_t interval;
    ncp_host_hl_rx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_get_long_poll_interval_response, status_code %d",
              (FMT__D, error_code));

    if (error_code == RET_OK)
    {
        TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_nwk_get_long_poll_interval_response", (FMT__0));

        ncp_host_hl_init_response_body(response, len, &body);
        ncp_host_hl_buf_get_u32(&body, &interval);

        TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_nwk_get_long_poll_interval_response, interval %ld",
                  (FMT__L, interval));
    }

    ncp_host_mark_blocking_request_finished();

    ncp_host_handle_nwk_get_long_poll_interval_response(error_code, ncp_tsn, interval);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_get_long_poll_interval_response", (FMT__0));
}


static void handle_nwk_get_in_fast_poll_flag_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    zb_uint8_t ncp_tsn = response->tsn;
    zb_uint8_t in_fast_poll_status;
    ncp_host_hl_rx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_get_in_fast_poll_flag_response, status_code %d",
              (FMT__D, error_code));

    if (error_code == RET_OK)
    {
        TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_nwk_get_in_fast_poll_flag_response", (FMT__0));

        ncp_host_hl_init_response_body(response, len, &body);
        ncp_host_hl_buf_get_u8(&body, &in_fast_poll_status);

        TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_nwk_get_in_fast_poll_flag_response, in_fast_poll_status %hd",
                  (FMT__H, in_fast_poll_status));
    }

    ncp_host_mark_blocking_request_finished();

    ncp_host_handle_nwk_get_in_fast_poll_flag_response(error_code, ncp_tsn, in_fast_poll_status);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_get_in_fast_poll_flag_response", (FMT__0));
}


static void handle_nwk_turbo_poll_continuous_leave_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_turbo_poll_continuous_leave_response, status_code %d",
              (FMT__D, error_code));

    ncp_host_mark_blocking_request_finished();

    ncp_host_handle_nwk_turbo_poll_continuous_leave_response(error_code);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_turbo_poll_continuous_leave_response", (FMT__0));
}


static void handle_nwk_turbo_poll_packets_leave_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_turbo_poll_packets_leave_response, status_code %d",
              (FMT__D, error_code));

    ncp_host_mark_blocking_request_finished();

    ncp_host_handle_nwk_turbo_poll_packets_leave_response(error_code);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_turbo_poll_packets_leave_response", (FMT__0));
}


static void handle_nwk_nlme_router_start_response(ncp_hl_response_header_t *response,
        zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_nlme_router_start_response, status_code %d",
              (FMT__D, error_code));

    ncp_host_mark_blocking_request_finished();

    ncp_host_handle_nwk_nlme_router_start_response(error_code);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_nlme_router_start_response", (FMT__0));
}

static void handle_nwk_joined_indication(ncp_hl_ind_header_t *indication, zb_uint16_t len)
{
    ncp_host_hl_rx_buf_handle_t body;
    zb_uint16_t nwk_addr;
    zb_ext_pan_id_t ext_pan_id;
    zb_uint8_t channel_page;
    zb_uint8_t channel;
    zb_uint8_t beacon_type;
    zb_uint8_t mac_interface;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_zdo_nwk_joined_indication ", (FMT__0));

    TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_zdo_nwk_joined_indication", (FMT__0));

    ncp_host_hl_init_indication_body(indication, len, &body);

    ncp_host_hl_buf_get_u16(&body, &nwk_addr);
    TRACE_MSG(TRACE_TRANSPORT3, " nwk_addr = 0x%x", (FMT__D, nwk_addr));

    ncp_host_hl_buf_get_u64addr(&body, ext_pan_id);
    TRACE_MSG(TRACE_TRANSPORT3, " ext_pan_id = " TRACE_FORMAT_64, (FMT__A, ext_pan_id));

    ncp_host_hl_buf_get_u8(&body, &channel_page);
    TRACE_MSG(TRACE_TRANSPORT3, " channel_page = %hd", (FMT__H, channel_page));

    ncp_host_hl_buf_get_u8(&body, &channel);
    TRACE_MSG(TRACE_TRANSPORT3, " channel = %hd", (FMT__H, channel));

    ncp_host_hl_buf_get_u8(&body, &beacon_type);
    TRACE_MSG(TRACE_TRANSPORT3, " beacon_type = %hd", (FMT__H, beacon_type));

    ncp_host_hl_buf_get_u8(&body, &mac_interface);
    TRACE_MSG(TRACE_TRANSPORT3, " mac_interface = %hd", (FMT__H, mac_interface));

    TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_zdo_nwk_joined_indication", (FMT__0));

    ncp_host_handle_nwk_joined_indication(nwk_addr, ext_pan_id, channel_page,
                                          channel, beacon_type, mac_interface);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_zdo_nwk_joined_indication", (FMT__0));
}

static void handle_nwk_join_failed_indication(ncp_hl_ind_header_t *indication, zb_uint16_t len)
{
    ncp_host_hl_rx_buf_handle_t body;
    zb_uint8_t status_category;
    zb_uint8_t status_code;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_zdo_nwk_joined_indication ", (FMT__0));

    TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_nwk_join_failed_indication", (FMT__0));

    ncp_host_hl_init_indication_body(indication, len, &body);

    ncp_host_hl_buf_get_u8(&body, &status_category);
    TRACE_MSG(TRACE_TRANSPORT3, " status_category = %hd", (FMT__H, status_category));

    ncp_host_hl_buf_get_u8(&body, &status_code);
    TRACE_MSG(TRACE_TRANSPORT3, " status_code = %hd", (FMT__H, status_code));

    TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_nwk_join_failed_indication", (FMT__0));

    ncp_host_handle_nwk_join_failed_indication(status_category, status_code);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_join_failed_indication", (FMT__0));
}


static void handle_nwk_leave_indication(ncp_hl_ind_header_t *indication, zb_uint16_t len)
{
    ncp_host_hl_rx_buf_handle_t body;

    zb_ieee_addr_t device_addr;
    zb_uint8_t rejoin;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_leave_indication ", (FMT__0));

    TRACE_MSG(TRACE_TRANSPORT3, "  >> parse handle_nwk_leave_indication", (FMT__0));

    ncp_host_hl_init_indication_body(indication, len, &body);

    ncp_host_hl_buf_get_u64addr(&body, device_addr);
    TRACE_MSG(TRACE_TRANSPORT3, " device_addr = " TRACE_FORMAT_64,
              (FMT__A, TRACE_ARG_64(device_addr)));

    ncp_host_hl_buf_get_u8(&body, &rejoin);
    TRACE_MSG(TRACE_TRANSPORT3, " rejoin = %hd", (FMT__H, rejoin));

    TRACE_MSG(TRACE_TRANSPORT3, "  << parse handle_nwk_leave_indication", (FMT__0));

    ncp_host_handle_nwk_leave_indication(device_addr, rejoin);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_leave_indication", (FMT__0));
}


static void handle_nwk_address_update_indication(ncp_hl_ind_header_t *indication, zb_uint16_t len)
{
    ncp_host_hl_rx_buf_handle_t body;
    zb_uint16_t nwk_addr;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_address_update_indication ", (FMT__0));

    TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_nwk_address_update_indication", (FMT__0));

    ncp_host_hl_init_indication_body(indication, len, &body);

    ncp_host_hl_buf_get_u16(&body, &nwk_addr);
    TRACE_MSG(TRACE_TRANSPORT3, " nwk_addr =0x%x", (FMT__D, nwk_addr));

    TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_nwk_address_update_indication", (FMT__0));

    ncp_host_handle_nwk_address_update_indication(nwk_addr);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_address_update_indication", (FMT__0));
}


static void handle_nwk_pan_id_conflict_indication(ncp_hl_ind_header_t *indication, zb_uint16_t len)
{
    ncp_host_hl_rx_buf_handle_t body;
    zb_uindex_t pan_id_index;
    zb_uint16_t pan_id_count;
    zb_uint16_t pan_ids[ZB_PAN_ID_CONFLICT_INFO_MAX_PANIDS_COUNT];

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_pan_id_conflict_indication ", (FMT__0));

    TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_nwk_pan_id_conflict_indication", (FMT__0));

    ncp_host_hl_init_indication_body(indication, len, &body);

    ncp_host_hl_buf_get_u16(&body, &pan_id_count);
    TRACE_MSG(TRACE_TRANSPORT3, " panid_count 0x%x", (FMT__D, pan_id_count));

    for (pan_id_index = 0; pan_id_index < pan_id_count; pan_id_index++)
    {
        ncp_host_hl_buf_get_u16(&body, &pan_ids[pan_id_index]);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_nwk_pan_id_conflict_indication", (FMT__0));

    ncp_host_handle_nwk_pan_id_conflict_indication(pan_id_count, pan_ids);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_pan_id_conflict_indication", (FMT__0));
}


static void handle_nwk_parent_lost_indication(ncp_hl_ind_header_t *indication, zb_uint16_t len)
{
    ncp_host_hl_rx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_parent_lost_indication ", (FMT__0));

    ncp_host_hl_init_indication_body(indication, len, &body);

    ncp_host_handle_nwk_parent_lost_indication();

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_parent_lost_indication", (FMT__0));
}


#ifdef ZB_APSDE_REQ_ROUTING_FEATURES
static void handle_nwk_route_reply_indication(ncp_hl_ind_header_t *indication, zb_uint16_t len)
{
    ncp_host_hl_rx_buf_handle_t body;
    zb_uint16_t nwk_addr;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_route_reply_indication ", (FMT__0));

    TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_nwk_route_reply_indication", (FMT__0));

    ncp_host_hl_init_indication_body(indication, len, &body);

    ncp_host_hl_buf_get_u16(&body, &nwk_addr);
    TRACE_MSG(TRACE_TRANSPORT3, " nwk_addr =0x%x", (FMT__D, nwk_addr));

    TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_nwk_route_reply_indication", (FMT__0));

    ncp_host_handle_nwk_route_reply_indication(nwk_addr);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_route_reply_indication", (FMT__0));
}

static void handle_nwk_route_request_send_indication(ncp_hl_ind_header_t *indication, zb_uint16_t len)
{
    ncp_host_hl_rx_buf_handle_t body;
    zb_uint16_t nwk_addr;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_route_request_send_indication ", (FMT__0));

    TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_nwk_route_request_send_indication", (FMT__0));

    ncp_host_hl_init_indication_body(indication, len, &body);

    ncp_host_hl_buf_get_u16(&body, &nwk_addr);
    TRACE_MSG(TRACE_TRANSPORT3, " nwk_addr =0x%x", (FMT__D, nwk_addr));

    TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_nwk_route_request_send_indication", (FMT__0));

    ncp_host_handle_nwk_route_request_send_indication(nwk_addr);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_route_request_send_indication", (FMT__0));
}

static void handle_nwk_route_record_send_indication(ncp_hl_ind_header_t *indication, zb_uint16_t len)
{
    ncp_host_hl_rx_buf_handle_t body;
    zb_uint16_t nwk_addr;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_route_record_send_indication ", (FMT__0));

    TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_nwk_route_record_send_indication", (FMT__0));

    ncp_host_hl_init_indication_body(indication, len, &body);

    ncp_host_hl_buf_get_u16(&body, &nwk_addr);
    TRACE_MSG(TRACE_TRANSPORT3, " nwk_addr =0x%x", (FMT__D, nwk_addr));

    TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_nwk_route_record_send_indication", (FMT__0));

    ncp_host_handle_nwk_route_record_send_indication(nwk_addr);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_route_record_send_indication", (FMT__0));
}
#endif


static void handle_nwk_start_pan_id_conflict_resolution_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_start_pan_id_conflict_resolution_response, status_code %d",
              (FMT__D, error_code));

    ncp_host_handle_nwk_start_pan_id_conflict_resolution_response(error_code);

    ncp_host_mark_blocking_request_finished();

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_start_pan_id_conflict_resolution_response", (FMT__0));
}


static void handle_nwk_enable_pan_id_conflict_resolution_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_enable_pan_id_conflict_resolution_response, status_code %d",
              (FMT__D, error_code));

    ncp_host_handle_nwk_enable_pan_id_conflict_resolution_response(error_code);

    ncp_host_mark_blocking_request_finished();

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_enable_pan_id_conflict_resolution_response", (FMT__0));
}


static void handle_nwk_enable_auto_pan_id_conflict_resolution_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nwk_enable_auto_pan_id_conflict_resolution_response, status_code %d",
              (FMT__D, error_code));

    ncp_host_handle_nwk_enable_auto_pan_id_conflict_resolution_response(error_code);

    ncp_host_mark_blocking_request_finished();

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nwk_enable_auto_pan_id_conflict_resolution_response", (FMT__0));
}


void ncp_host_handle_nwkmgmt_response(void *data, zb_uint16_t len)
{
    ncp_hl_response_header_t *response_header = (ncp_hl_response_header_t *)data;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwkmgmt_response, call_id %d, tsn %hd",
              (FMT__D_H, response_header->hdr.call_id, response_header->tsn));

    switch (response_header->hdr.call_id)
    {
#ifdef ZB_FORMATION
    case NCP_HL_NWK_FORMATION:
        handle_nwk_formation_response(response_header, len);
        break;
    case NCP_HL_NWK_START_WITHOUT_FORMATION:
        handle_nwk_start_without_formation_response(response_header, len);
        break;
#endif /* ZB_FORMATION */
    case NCP_HL_NWK_DISCOVERY:
        handle_nwk_discovery_response(response_header, len);
        break;
    case NCP_HL_NWK_NLME_JOIN:
        handle_nwk_nlme_join_response(response_header, len);
        break;
    case NCP_HL_NWK_PERMIT_JOINING:
        handle_nwk_permit_joining_response(response_header, len);
        break;
    case NCP_HL_NWK_GET_IEEE_BY_SHORT:
        handle_nwk_get_ieee_by_short_response(response_header, len);
        break;
    case NCP_HL_NWK_GET_NEIGHBOR_BY_IEEE:
        handle_nwk_get_neighbor_by_ieee_response(response_header, len);
        break;
    case NCP_HL_NWK_GET_SHORT_BY_IEEE:
        handle_nwk_get_short_by_ieee_response(response_header, len);
        break;
    case NCP_HL_NWK_NLME_ROUTER_START:
        handle_nwk_nlme_router_start_response(response_header, len);
        break;
    case NCP_HL_PIM_SET_LONG_POLL_INTERVAL:
        handle_nwk_set_long_poll_interval_response(response_header, len);
        break;
    case NCP_HL_PIM_SET_FAST_POLL_INTERVAL:
        handle_nwk_set_fast_poll_interval_response(response_header, len);
        break;
    case NCP_HL_PIM_START_FAST_POLL:
        handle_nwk_start_fast_poll_response(response_header, len);
        break;
    case NCP_HL_PIM_START_POLL:
        handle_nwk_start_poll_response(response_header, len);
        break;
    case NCP_HL_PIM_STOP_FAST_POLL:
        handle_nwk_stop_fast_poll_response(response_header, len);
        break;
    case NCP_HL_PIM_STOP_POLL:
        handle_nwk_stop_poll_response(response_header, len);
        break;
    case NCP_HL_PIM_ENABLE_TURBO_POLL:
        handle_nwk_enable_turbo_poll_response(response_header, len);
        break;
    case NCP_HL_PIM_DISABLE_TURBO_POLL:
        handle_nwk_disable_turbo_poll_response(response_header, len);
        break;
    case NCP_HL_PIM_START_TURBO_POLL_PACKETS:
        handle_nwk_start_turbo_poll_packets_response(response_header, len);
        break;
    case NCP_HL_PIM_START_TURBO_POLL_CONTINUOUS:
        handle_nwk_start_turbo_poll_continuous_response(response_header, len);
        break;
    case NCP_HL_PIM_PERMIT_TURBO_POLL:
        handle_nwk_permit_turbo_poll_response(response_header, len);
        break;
    case NCP_HL_PIM_GET_LONG_POLL_INTERVAL:
        handle_nwk_get_long_poll_interval_response(response_header, len);
        break;
    case NCP_HL_PIM_GET_IN_FAST_POLL_FLAG:
        handle_nwk_get_in_fast_poll_flag_response(response_header, len);
        break;
    case NCP_HL_PIM_SET_FAST_POLL_TIMEOUT:
        handle_nwk_set_fast_poll_timeout_response(response_header, len);
        break;
    case NCP_HL_SET_KEEPALIVE_MODE:
        handle_nwk_set_keepalive_mode_response(response_header, len);
        break;
    case NCP_HL_START_CONCENTRATOR_MODE:
        handle_nwk_start_concentrator_mode_response(response_header, len);
        break;
    case NCP_HL_STOP_CONCENTRATOR_MODE:
        handle_nwk_stop_concentrator_mode_response(response_header, len);
        break;
    case NCP_HL_PIM_TURBO_POLL_CONTINUOUS_LEAVE:
        handle_nwk_turbo_poll_continuous_leave_response(response_header, len);
        break;
    case NCP_HL_PIM_TURBO_POLL_PACKETS_LEAVE:
        handle_nwk_turbo_poll_packets_leave_response(response_header, len);
        break;
    case NCP_HL_NWK_PAN_ID_CONFLICT_RESOLVE:
        handle_nwk_start_pan_id_conflict_resolution_response(response_header, len);
        break;
    case NCP_HL_NWK_ENABLE_PAN_ID_CONFLICT_RESOLUTION:
        handle_nwk_enable_pan_id_conflict_resolution_response(response_header, len);
        break;
    case NCP_HL_NWK_ENABLE_AUTO_PAN_ID_CONFLICT_RESOLUTION:
        handle_nwk_enable_auto_pan_id_conflict_resolution_response(response_header, len);
        break;
    case NCP_HL_PIM_TURBO_POLL_CANCEL_PACKET:
        handle_nwk_turbo_poll_cancel_packet_response(response_header, len);
        break;
    default:
        ZB_ASSERT(0);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwkmgmt_response", (FMT__0));
}

void ncp_host_handle_nwkmgmt_indication(void *data, zb_uint16_t len)
{
    ncp_hl_ind_header_t *indication_header = (ncp_hl_ind_header_t *)data;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwkmgmt_indication, call_id %d", (FMT__D, indication_header->call_id));

    switch (indication_header->call_id)
    {
    case NCP_HL_NWK_REJOINED_IND:
        handle_nwk_joined_indication(indication_header, len);
        break;
    case NCP_HL_NWK_REJOIN_FAILED_IND:
        handle_nwk_join_failed_indication(indication_header, len);
        break;
    case NCP_HL_NWK_LEAVE_IND:
        handle_nwk_leave_indication(indication_header, len);
        break;
    case NCP_HL_NWK_ADDRESS_UPDATE_IND:
        handle_nwk_address_update_indication(indication_header, len);
        break;
    case NCP_HL_NWK_PAN_ID_CONFLICT_IND:
        handle_nwk_pan_id_conflict_indication(indication_header, len);
        break;
    case NCP_HL_PARENT_LOST_IND:
        handle_nwk_parent_lost_indication(indication_header, len);
        break;
#ifdef ZB_APSDE_REQ_ROUTING_FEATURES
    case NCP_HL_NWK_ROUTE_REPLY_IND:
        handle_nwk_route_reply_indication(indication_header, len);
        break;
    case NCP_HL_NWK_ROUTE_REQUEST_SEND_IND:
        handle_nwk_route_request_send_indication(indication_header, len);
        break;
    case NCP_HL_NWK_ROUTE_RECORD_SEND_IND:
        handle_nwk_route_record_send_indication(indication_header, len);
        break;
#endif
    default:
        ZB_ASSERT(0);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwkmgmt_indication", (FMT__0));
}

#ifdef ZB_FORMATION
zb_ret_t ncp_host_nwk_formation(zb_uint32_t *scan_channels_list, zb_uint8_t scan_duration,
                                zb_uint8_t distributed_network, zb_uint16_t distributed_network_address,
                                zb_ext_pan_id_t extended_pan_id)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    zb_uint8_t channel_list_len = ZB_CHANNEL_PAGES_NUM;
    zb_uint8_t idx;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_formation", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_NWK_FORMATION, &body, NULL))
    {
        ncp_host_hl_buf_put_u8(&body, channel_list_len);
        for (idx = 0; idx < channel_list_len; idx++)
        {
            ncp_host_hl_buf_put_u8(&body, zb_channel_page_list_get_page(scan_channels_list, idx));
            ncp_host_hl_buf_put_u32(&body, scan_channels_list[idx]);
        }
        ncp_host_hl_buf_put_u8(&body, scan_duration);
        ncp_host_hl_buf_put_u8(&body, distributed_network);
        ncp_host_hl_buf_put_u16(&body, distributed_network_address);
        ncp_host_hl_buf_put_u64addr(&body, extended_pan_id);

        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_formation, ret %d", (FMT__D, ret));

    return ret;
}

zb_ret_t ncp_host_nwk_start_without_formation(void)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_start_without_formation", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_NWK_START_WITHOUT_FORMATION, &body, NULL))
    {
        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_start_without_formation, ret %d", (FMT__D, ret));

    return ret;
}
#endif /* ZB_FORMATION */

zb_ret_t ncp_host_nwk_discovery(zb_uint32_t *scan_channels_list, zb_uint8_t scan_duration)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    zb_uint8_t channel_list_len = ZB_CHANNEL_PAGES_NUM;
    zb_uint8_t idx;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_discovery", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_NWK_DISCOVERY, &body, NULL))
    {
        ncp_host_hl_buf_put_u8(&body, channel_list_len);
        for (idx = 0; idx < channel_list_len; idx++)
        {
            ncp_host_hl_buf_put_u8(&body, zb_channel_page_list_get_page(scan_channels_list, idx));
            ncp_host_hl_buf_put_u32(&body, scan_channels_list[idx]);
        }
        ncp_host_hl_buf_put_u8(&body, scan_duration);

        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_discovery, ret %d", (FMT__D, ret));

    return ret;
}

zb_ret_t ncp_host_nwk_nlme_join(zb_ext_pan_id_t extended_pan_id, zb_uint8_t rejoin_network,
                                zb_uint32_t *scan_channels_list, zb_uint8_t scan_duration,
                                zb_uint8_t mac_capabilities, zb_uint8_t security_enable)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    zb_uint8_t channel_list_len = ZB_CHANNEL_PAGES_NUM;
    zb_uint8_t idx;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_nlme_join", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_NWK_NLME_JOIN, &body, NULL))
    {
        ncp_host_hl_buf_put_u64addr(&body, extended_pan_id);
        ncp_host_hl_buf_put_u8(&body, rejoin_network);
        ncp_host_hl_buf_put_u8(&body, channel_list_len);
        for (idx = 0; idx < channel_list_len; idx++)
        {
            ncp_host_hl_buf_put_u8(&body, zb_channel_page_list_get_page(scan_channels_list, idx));
            ncp_host_hl_buf_put_u32(&body, scan_channels_list[idx]);
        }
        ncp_host_hl_buf_put_u8(&body, scan_duration);
        ncp_host_hl_buf_put_u8(&body, mac_capabilities);
        ncp_host_hl_buf_put_u8(&body, security_enable);

        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_nlme_join, ret %d", (FMT__D, ret));

    return ret;
}

zb_ret_t ncp_host_nwk_permit_joining(zb_uint8_t permit_duration)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_permit_joining", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_NWK_PERMIT_JOINING, &body, NULL))
    {
        ncp_host_hl_buf_put_u8(&body, permit_duration);
        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_permit_joining, ret %d", (FMT__D, ret));

    return ret;
}

zb_ret_t ncp_host_nwk_get_ieee_by_short(zb_uint16_t short_addr)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_get_ieee_by_short", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_NWK_GET_IEEE_BY_SHORT, &body, NULL))
    {
        ncp_host_hl_buf_put_u16(&body, short_addr);
        ret = ncp_host_hl_send_packet(&body);
    }
    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_get_ieee_by_short, ret %d", (FMT__D, ret));

    return ret;
}

zb_ret_t ncp_host_nwk_get_neighbor_by_ieee(zb_ieee_addr_t ieee_addr)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_get_neighbor_by_ieee", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_NWK_GET_NEIGHBOR_BY_IEEE, &body, NULL))
    {
        ncp_host_hl_buf_put_u64addr(&body, ieee_addr);
        ret = ncp_host_hl_send_packet(&body);
    }
    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_get_neighbor_by_ieee, ret %d", (FMT__D, ret));

    return ret;
}

zb_ret_t ncp_host_nwk_set_fast_poll_interval(zb_uint16_t fast_poll_interval)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_set_fast_poll_interval", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_PIM_SET_FAST_POLL_INTERVAL, &body, NULL))
    {
        ncp_host_hl_buf_put_u16(&body, ZB_MSEC_TO_QUARTERECONDS(fast_poll_interval));
        ret = ncp_host_hl_send_packet(&body);
    }
    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_set_fast_poll_interval, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_nwk_set_long_poll_interval(zb_uint32_t long_poll_interval)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_set_long_poll_interval", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_PIM_SET_LONG_POLL_INTERVAL, &body, NULL))
    {
        ncp_host_hl_buf_put_u32(&body, ZB_MSEC_TO_QUARTERECONDS(long_poll_interval));
        ret = ncp_host_hl_send_packet(&body);
    }
    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_set_long_poll_interval, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_nwk_start_fast_poll(void)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_start_fast_poll", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_PIM_START_FAST_POLL, &body, NULL))
    {
        ret = ncp_host_hl_send_packet(&body);
    }
    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_start_fast_poll, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_nwk_start_poll(void)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_start_poll", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_PIM_START_POLL, &body, NULL))
    {
        ret = ncp_host_hl_send_packet(&body);
    }
    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_start_poll, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_nwk_stop_fast_poll(zb_uint8_t *ncp_tsn)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_stop_fast_poll", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_PIM_STOP_FAST_POLL, &body, ncp_tsn))
    {
        ret = ncp_host_hl_send_packet(&body);
    }
    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_stop_fast_poll, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_nwk_stop_poll(void)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_stop_poll", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_PIM_STOP_POLL, &body, NULL))
    {
        ret = ncp_host_hl_send_packet(&body);
    }
    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_stop_poll, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_nwk_enable_turbo_poll(zb_uint32_t time)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_enable_turbo_poll", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_PIM_ENABLE_TURBO_POLL, &body, NULL))
    {
        ncp_host_hl_buf_put_u32(&body, time);
        ret = ncp_host_hl_send_packet(&body);
    }
    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_enable_turbo_poll, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_nwk_disable_turbo_poll(void)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_disable_turbo_poll", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_PIM_DISABLE_TURBO_POLL, &body, NULL))
    {
        ret = ncp_host_hl_send_packet(&body);
    }
    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_disable_turbo_poll, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_nwk_start_turbo_poll_packets(zb_uint8_t packets_count)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_start_turbo_poll_packets", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_PIM_START_TURBO_POLL_PACKETS, &body, NULL))
    {
        ncp_host_hl_buf_put_u8(&body, packets_count);
        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_start_turbo_poll_packets, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_nwk_turbo_poll_cancel_packet(void)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_turbo_poll_cancel_packet", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_PIM_TURBO_POLL_CANCEL_PACKET, &body, NULL))
    {
        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_turbo_poll_cancel_packet, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_nwk_start_turbo_poll_continuous(zb_time_t turbo_poll_timeout_ms)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_start_turbo_poll_continuous", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_PIM_START_TURBO_POLL_CONTINUOUS, &body, NULL))
    {
        ncp_host_hl_buf_put_u32(&body, turbo_poll_timeout_ms);
        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_start_turbo_poll_continuous, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_nwk_turbo_poll_continuous_leave(void)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_turbo_poll_continuous_leave", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_PIM_TURBO_POLL_CONTINUOUS_LEAVE, &body, NULL))
    {
        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_turbo_poll_continuous_leave, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_nwk_turbo_poll_packets_leave(void)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_turbo_poll_packets_leave", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_PIM_TURBO_POLL_PACKETS_LEAVE, &body, NULL))
    {
        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_turbo_poll_packets_leave, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_nwk_set_fast_poll_timeout(zb_time_t timeout)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_set_fast_poll_timeout", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_PIM_SET_FAST_POLL_TIMEOUT, &body, NULL))
    {
        ncp_host_hl_buf_put_u32(&body, timeout);
        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_set_fast_poll_timeout, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_nwk_start_pan_id_conflict_resolution(zb_uint16_t pan_id_count, zb_uint16_t *pan_ids)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_start_pan_id_conflict_resolution", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_NWK_PAN_ID_CONFLICT_RESOLVE, &body, NULL))
    {
        zb_uindex_t pan_id_index;

        ncp_host_hl_buf_put_u16(&body, pan_id_count);

        TRACE_MSG(TRACE_TRANSPORT2, "  pan_id_count %d", (FMT__D, pan_id_count));

        for (pan_id_index = 0; pan_id_index < pan_id_count; pan_id_index++)
        {
            TRACE_MSG(TRACE_TRANSPORT2, "  pan_id #%d: %d", (FMT__D_D, pan_id_index, pan_ids[pan_id_index]));
            ncp_host_hl_buf_put_u16(&body, pan_ids[pan_id_index]);
        }

        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_start_pan_id_conflict_resolution, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_nwk_enable_pan_id_conflict_resolution(zb_bool_t enable)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_enable_pan_id_conflict_resolution", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_NWK_ENABLE_PAN_ID_CONFLICT_RESOLUTION, &body, NULL))
    {
        ncp_host_hl_buf_put_u8(&body, enable);

        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_enable_pan_id_conflict_resolution, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_nwk_enable_auto_pan_id_conflict_resolution(zb_bool_t enable)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_enable_auto_pan_id_conflict_resolution", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_NWK_ENABLE_AUTO_PAN_ID_CONFLICT_RESOLUTION, &body, NULL))
    {
        ncp_host_hl_buf_put_u8(&body, enable);

        TRACE_MSG(TRACE_TRANSPORT2, "  enable %hd", (FMT__H, enable));

        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_enable_auto_pan_id_conflict_resolution, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_nwk_permit_turbo_poll(zb_bool_t permit_status)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_permit_turbo_poll", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_PIM_PERMIT_TURBO_POLL, &body, NULL))
    {
        ncp_host_hl_buf_put_u8(&body, permit_status);
        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_permit_turbo_poll, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_nwk_set_keepalive_mode(zb_uint8_t mode)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_set_keepalive_mode", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_SET_KEEPALIVE_MODE, &body, NULL))
    {
        ncp_host_hl_buf_put_u8(&body, mode);
        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_set_keepalive_mode, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_nwk_start_concentrator_mode(zb_uint8_t radius, zb_uint32_t disc_time)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_start_concentrator_mode", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_START_CONCENTRATOR_MODE, &body, NULL))
    {
        ncp_host_hl_buf_put_u8(&body, radius);
        ncp_host_hl_buf_put_u32(&body, disc_time);
        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_start_concentrator_mode, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_nwk_stop_concentrator_mode(void)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_stop_concentrator_mode", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_STOP_CONCENTRATOR_MODE, &body, NULL))
    {
        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_stop_concentrator_mode, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_nwk_get_long_poll_interval(zb_uint8_t *ncp_tsn)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_get_long_poll_interval", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_PIM_GET_LONG_POLL_INTERVAL, &body, ncp_tsn))
    {
        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_get_long_poll_interval, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_nwk_get_in_fast_poll_flag(zb_uint8_t *ncp_tsn)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_get_in_fast_poll_flag", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_PIM_GET_IN_FAST_POLL_FLAG, &body, ncp_tsn))
    {
        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_get_in_fast_poll_flag, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_nwk_get_short_by_ieee(zb_ieee_addr_t ieee_addr)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_get_short_by_ieee", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_NWK_GET_SHORT_BY_IEEE, &body, NULL))
    {
        ncp_host_hl_buf_put_u64addr(&body, ieee_addr);
        ret = ncp_host_hl_send_packet(&body);
    }
    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_get_short_by_ieee, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_nwk_nlme_start_router_request(zb_uint8_t beacon_order, zb_uint8_t superframe_order,
        zb_uint8_t battery_life_extension)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_nlme_start_router_request", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_NWK_NLME_ROUTER_START, &body, NULL))
    {
        ncp_host_hl_buf_put_u8(&body, beacon_order);
        ncp_host_hl_buf_put_u8(&body, superframe_order);
        ncp_host_hl_buf_put_u8(&body, battery_life_extension);

        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_nlme_start_router_request, ret %d", (FMT__D, ret));

    return ret;
}
