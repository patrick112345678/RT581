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
/*  PURPOSE: NCP High level transport implementation for the host side: ZDO category
*/

#define ZB_TRACE_FILE_ID 15

#include "ncp_host_hl_proto.h"


static void handle_zdo_dev_annce_indication(ncp_hl_ind_header_t *indication, zb_uint16_t len)
{
    ncp_host_hl_rx_buf_handle_t body;
    zb_uint16_t nwk_addr;
    zb_ieee_addr_t ieee_addr;
    zb_uint8_t capability;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_zdo_dev_annce_indication ", (FMT__0));

    TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_zdo_dev_annce_indication", (FMT__0));

    ncp_host_hl_init_indication_body(indication, len, &body);
    ncp_host_hl_buf_get_u16(&body, &nwk_addr);
    ncp_host_hl_buf_get_u64addr(&body, ieee_addr);
    ncp_host_hl_buf_get_u8(&body, &capability);

    TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_zdo_dev_annce_indication, nwk_addr 0x%x, long_addr " TRACE_FORMAT_64 " mac_capabilities %hd",
              (FMT__D_A_H, nwk_addr, TRACE_ARG_64(ieee_addr), capability));

    ncp_host_handle_zdo_device_annce_indication(nwk_addr, ieee_addr, capability);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_zdo_dev_annce_indication", (FMT__0));
}


static void handle_zdo_dev_authorized_indication(ncp_hl_ind_header_t *indication, zb_uint16_t len)
{
    ncp_host_hl_rx_buf_handle_t body;
    zb_uint16_t nwk_addr;
    zb_ieee_addr_t ieee_addr;
    zb_uint8_t authorization_type;
    zb_uint8_t authorization_status;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_zdo_dev_authorized_indication ", (FMT__0));

    TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_zdo_dev_authorized_indication", (FMT__0));

    ncp_host_hl_init_indication_body(indication, len, &body);
    ncp_host_hl_buf_get_u64addr(&body, ieee_addr);
    ncp_host_hl_buf_get_u16(&body, &nwk_addr);
    ncp_host_hl_buf_get_u8(&body, &authorization_type);
    ncp_host_hl_buf_get_u8(&body, &authorization_status);

    TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_zdo_dev_authorized_indication, long_addr " TRACE_FORMAT_64 ", nwk_addr 0x%x, authorization_type %hd, authorization_status %hd",
              (FMT__A_D_H_H, TRACE_ARG_64(ieee_addr), nwk_addr, authorization_type, authorization_status));

    ncp_host_handle_zdo_device_authorized_indication(ieee_addr, nwk_addr, authorization_type, authorization_status);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_zdo_dev_authorized_indication", (FMT__0));
}


static void handle_zdo_dev_update_indication(ncp_hl_ind_header_t *indication, zb_uint16_t len)
{
    ncp_host_hl_rx_buf_handle_t body;
    zb_uint16_t nwk_addr;
    zb_ieee_addr_t ieee_addr;
    zb_uint8_t status;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_zdo_dev_update_indication ", (FMT__0));

    TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_zdo_dev_update_indication", (FMT__0));

    ncp_host_hl_init_indication_body(indication, len, &body);
    ncp_host_hl_buf_get_u64addr(&body, ieee_addr);
    ncp_host_hl_buf_get_u16(&body, &nwk_addr);
    ncp_host_hl_buf_get_u8(&body, &status);

    TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_zdo_dev_update_indication, long_addr " TRACE_FORMAT_64 ", nwk_addr 0x%x, status %hd",
              (FMT__A_D_H, TRACE_ARG_64(ieee_addr), nwk_addr, status));

    ncp_host_handle_zdo_device_update_indication(ieee_addr, nwk_addr, status);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_zdo_dev_update_indication", (FMT__0));
}


static void handle_zdo_ieee_addr_response(ncp_hl_response_header_t *response,
        zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    zb_uint8_t ncp_tsn = response->tsn;
    ncp_host_hl_rx_buf_handle_t body;
    zb_zdo_ieee_addr_resp_t resp;
    zb_bool_t is_extended = ZB_FALSE;
    zb_zdo_ieee_addr_resp_ext_t resp_ext;
    zb_zdo_ieee_addr_resp_ext2_t resp_ext2;
    zb_uint16_t *addr_list;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_zdo_ieee_addr_response, status_code %d",
              (FMT__D, error_code));

    if (error_code == RET_OK)
    {
        TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_zdo_ieee_addr_response", (FMT__0));

        ncp_host_hl_init_response_body(response, len, &body);
        ncp_host_hl_buf_get_u64addr(&body, resp.ieee_addr_remote_dev);
        ncp_host_hl_buf_get_u16(&body, &resp.nwk_addr_remote_dev);

        TRACE_MSG(TRACE_TRANSPORT3, " ieee_addr = " TRACE_FORMAT_64 " nwk_addr = 0x%x",
                  (FMT__A_D, TRACE_ARG_64(resp.ieee_addr_remote_dev), resp.nwk_addr_remote_dev));

        if (len > sizeof(ncp_hl_response_header_t) + sizeof(zb_uint16_t) + sizeof(zb_ieee_addr_t))
        {
            is_extended = ZB_TRUE;
            ncp_host_hl_buf_get_u8(&body, &resp_ext.num_assoc_dev);

            TRACE_MSG(TRACE_TRANSPORT3, "num_assoc_dev = %h", (FMT__H, resp_ext.num_assoc_dev));

            if (resp_ext.num_assoc_dev != 0)
            {
                ncp_host_hl_buf_get_u8(&body, &resp_ext2.start_index);
                TRACE_MSG(TRACE_TRANSPORT3, "start_index = %h", (FMT__H, resp_ext2.start_index));

                ncp_host_hl_buf_get_ptr(&body, (zb_uint8_t **)&addr_list, resp_ext.num_assoc_dev);
            }
        }

        TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_zdo_ieee_addr_response", (FMT__0));
    }

    ncp_host_handle_zdo_ieee_addr_response(error_code, ncp_tsn,
                                           &resp, is_extended,
                                           &resp_ext, &resp_ext2, addr_list);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_zdo_ieee_addr_response", (FMT__0));
}

static void handle_zdo_nwk_addr_response(ncp_hl_response_header_t *response,
        zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    zb_uint8_t ncp_tsn = response->tsn;
    ncp_host_hl_rx_buf_handle_t body;
    zb_zdo_nwk_addr_resp_head_t resp;
    zb_bool_t is_extended = ZB_FALSE;
    zb_zdo_nwk_addr_resp_ext_t resp_ext;
    zb_zdo_nwk_addr_resp_ext2_t resp_ext2;
    zb_uint16_t *addr_list;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_zdo_nwk_addr_response, status_code %d",
              (FMT__D, error_code));

    if (error_code == RET_OK)
    {
        TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_zdo_nwk_addr_response", (FMT__0));

        ncp_host_hl_init_response_body(response, len, &body);
        ncp_host_hl_buf_get_u64addr(&body, resp.ieee_addr);
        ncp_host_hl_buf_get_u16(&body, &resp.nwk_addr);

        TRACE_MSG(TRACE_TRANSPORT3, " ieee_addr = " TRACE_FORMAT_64 " nwk_addr = 0x%x",
                  (FMT__A_D, TRACE_ARG_64(resp.ieee_addr), resp.nwk_addr));

        if (len > sizeof(ncp_hl_response_header_t) + sizeof(zb_uint16_t) + sizeof(zb_ieee_addr_t))
        {
            is_extended = ZB_TRUE;
            ncp_host_hl_buf_get_u8(&body, &resp_ext.num_assoc_dev);

            TRACE_MSG(TRACE_TRANSPORT3, "num_assoc_dev = %h", (FMT__H, resp_ext.num_assoc_dev));

            if (resp_ext.num_assoc_dev != 0)
            {
                ncp_host_hl_buf_get_u8(&body, &resp_ext2.start_index);
                TRACE_MSG(TRACE_TRANSPORT3, "start_index = %h", (FMT__H, resp_ext2.start_index));

                ncp_host_hl_buf_get_ptr(&body, (zb_uint8_t **)&addr_list, resp_ext.num_assoc_dev * sizeof(zb_uint16_t));
            }
        }

        TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_zdo_nwk_addr_response", (FMT__0));
    }

    ncp_host_handle_zdo_nwk_addr_response(error_code, ncp_tsn,
                                          &resp, is_extended,
                                          &resp_ext, &resp_ext2, addr_list);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_zdo_nwk_addr_response", (FMT__0));
}

static void handle_zdo_power_descriptor_response(ncp_hl_response_header_t *response,
        zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    zb_uint8_t ncp_tsn = response->tsn;
    ncp_host_hl_rx_buf_handle_t body;
    zb_uint16_t power_descriptor;
    zb_uint16_t nwk_addr;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_zdo_power_descriptor_response, status_code %d",
              (FMT__D, error_code));

    if (error_code == RET_OK)
    {
        TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_zdo_power_descriptor_response", (FMT__0));

        ncp_host_hl_init_response_body(response, len, &body);
        ncp_host_hl_buf_get_u16(&body, &power_descriptor);
        ncp_host_hl_buf_get_u16(&body, &nwk_addr);

        TRACE_MSG(TRACE_TRANSPORT3, ">> power_descriptor = 0x%x ", (FMT__D, power_descriptor));

        TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_zdo_power_descriptor_response", (FMT__0));
    }

    ncp_host_handle_zdo_power_descriptor_response(error_code, ncp_tsn, nwk_addr, power_descriptor);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_zdo_power_descriptor_response", (FMT__0));
}

static void handle_zdo_node_descriptor_response(ncp_hl_response_header_t *response,
        zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    zb_uint8_t ncp_tsn = response->tsn;
    ncp_host_hl_rx_buf_handle_t body;
    zb_af_node_desc_t node_desc;
    zb_uint16_t nwk_addr;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_zdo_node_descriptor_response, status_code %d",
              (FMT__D, error_code));

    if (error_code == RET_OK)
    {
        TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_zdo_node_descriptor_response", (FMT__0));

        ncp_host_hl_init_response_body(response, len, &body);

        ncp_host_hl_buf_get_u16(&body, &node_desc.node_desc_flags);
        TRACE_MSG(TRACE_TRANSPORT3, "node_desc.node_desc_flags 0x%x", (FMT__D, node_desc.node_desc_flags));

        ncp_host_hl_buf_get_u8(&body, &node_desc.mac_capability_flags);
        TRACE_MSG(TRACE_TRANSPORT3, "node_desc.mac_capability_flags 0x%hx",
                  (FMT__H, node_desc.mac_capability_flags));

        ncp_host_hl_buf_get_u16(&body, &node_desc.manufacturer_code);
        TRACE_MSG(TRACE_TRANSPORT3, "node_desc.manufacturer_code 0x%x", (FMT__D, node_desc.manufacturer_code));

        ncp_host_hl_buf_get_u8(&body, &node_desc.max_buf_size);
        TRACE_MSG(TRACE_TRANSPORT3, "node_desc.max_buf_size %hd", (FMT__H, node_desc.max_buf_size));

        ncp_host_hl_buf_get_u16(&body, &node_desc.max_incoming_transfer_size);
        TRACE_MSG(TRACE_TRANSPORT3, "node_desc.max_incoming_transfer_size %d",
                  (FMT__D, node_desc.max_incoming_transfer_size));

        ncp_host_hl_buf_get_u16(&body, &node_desc.server_mask);
        TRACE_MSG(TRACE_TRANSPORT3, "node_desc.server_mask 0x%x", (FMT__D, node_desc.server_mask));

        ncp_host_hl_buf_get_u16(&body, &node_desc.max_outgoing_transfer_size);
        TRACE_MSG(TRACE_TRANSPORT3, "node_desc.max_outgoing_transfer_size %d",
                  (FMT__D, node_desc.max_outgoing_transfer_size));

        ncp_host_hl_buf_get_u8(&body, &node_desc.desc_capability_field);
        TRACE_MSG(TRACE_TRANSPORT3, "node_desc.desc_capability_field 0x%hx",
                  (FMT__H, node_desc.desc_capability_field));

        ncp_host_hl_buf_get_u16(&body, &nwk_addr);
        TRACE_MSG(TRACE_TRANSPORT3, "nwk_addr 0x%x", (FMT__D, nwk_addr));

        TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_zdo_node_descriptor_response", (FMT__0));
    }

    ncp_host_handle_zdo_node_descriptor_response(error_code, ncp_tsn, nwk_addr, &node_desc);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_zdo_node_descriptor_response", (FMT__0));
}

static void handle_zdo_simple_descriptor_response(ncp_hl_response_header_t *response,
        zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    zb_uint8_t ncp_tsn = response->tsn;
    ncp_host_hl_rx_buf_handle_t body;
    zb_af_simple_desc_1_1_t simple_desc;
    zb_uint16_t *app_cluster_list_ptr;
    zb_uint16_t nwk_addr;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_zdo_simple_descriptor_response, status_code %d",
              (FMT__D, error_code));

    if (error_code == RET_OK)
    {
        TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_zdo_simple_descriptor_response", (FMT__0));

        ncp_host_hl_init_response_body(response, len, &body);

        ncp_host_hl_buf_get_u8(&body, &simple_desc.endpoint);
        TRACE_MSG(TRACE_TRANSPORT3, "simple_desc.endpoint %hd", (FMT__H, simple_desc.endpoint));

        ncp_host_hl_buf_get_u16(&body, &simple_desc.app_profile_id);
        TRACE_MSG(TRACE_TRANSPORT3, "simple_desc.app_profile_id 0x%x", (FMT__D, simple_desc.app_profile_id));

        ncp_host_hl_buf_get_u16(&body, &simple_desc.app_device_id);
        TRACE_MSG(TRACE_TRANSPORT3, "simple_desc.app_device_id 0x%x", (FMT__D, simple_desc.app_device_id));

        ncp_host_hl_buf_get_u8(&body, (zb_uint8_t *)(&simple_desc.app_device_id + 1));
        TRACE_MSG(TRACE_TRANSPORT3, "simple_desc.app_device_version 0x%hx",
                  (FMT__H, simple_desc.app_device_version));

        ncp_host_hl_buf_get_u8(&body, &simple_desc.app_input_cluster_count);
        TRACE_MSG(TRACE_TRANSPORT3, "simple_desc.app_input_cluster_count %hd",
                  (FMT__H, simple_desc.app_input_cluster_count));

        ncp_host_hl_buf_get_u8(&body, &simple_desc.app_output_cluster_count);
        TRACE_MSG(TRACE_TRANSPORT3, "simple_desc.app_output_cluster_count %hd",
                  (FMT__H, simple_desc.app_output_cluster_count));

        ncp_host_hl_buf_get_ptr(&body, (zb_uint8_t **)&app_cluster_list_ptr,
                                sizeof(zb_uint16_t) *
                                (simple_desc.app_input_cluster_count + simple_desc.app_output_cluster_count));

        ncp_host_hl_buf_get_u16(&body, &nwk_addr);
        TRACE_MSG(TRACE_TRANSPORT3, "nwk_addr 0x%x", (FMT__D, nwk_addr));

        TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_zdo_simple_descriptor_response", (FMT__0));
    }

    ncp_host_handle_zdo_simple_descriptor_response(error_code, ncp_tsn, nwk_addr,
            &simple_desc, app_cluster_list_ptr);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_zdo_simple_descriptor_response", (FMT__0));
}

static void handle_zdo_active_ep_response(ncp_hl_response_header_t *response,
        zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    zb_uint8_t ncp_tsn = response->tsn;
    ncp_host_hl_rx_buf_handle_t body;
    zb_uint8_t ep_count;
    zb_uint8_t *ep_list;
    zb_uint16_t nwk_addr;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_zdo_active_ep_response, status_code %d",
              (FMT__D, error_code));

    if (error_code == RET_OK)
    {
        TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_zdo_active_ep_response", (FMT__0));

        ncp_host_hl_init_response_body(response, len, &body);

        ncp_host_hl_buf_get_u8(&body, &ep_count);
        TRACE_MSG(TRACE_TRANSPORT3, ">> endpoint count = %hd ", (FMT__H, ep_count));

        ncp_host_hl_buf_get_ptr(&body, &ep_list, ep_count);

        ncp_host_hl_buf_get_u16(&body, &nwk_addr);
        TRACE_MSG(TRACE_TRANSPORT3, "nwk_addr 0x%x", (FMT__D, nwk_addr));

        TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_zdo_active_ep_response", (FMT__0));
    }

    ncp_host_handle_zdo_active_ep_response(error_code, ncp_tsn, nwk_addr, ep_count, ep_list);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_zdo_active_ep_response", (FMT__0));
}

static void handle_zdo_match_desc_response(ncp_hl_response_header_t *response,
        zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    zb_uint8_t ncp_tsn = response->tsn;
    ncp_host_hl_rx_buf_handle_t body;
    zb_uint8_t match_ep_count;
    zb_uint8_t *match_ep_list;
    zb_uint16_t nwk_addr;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_zdo_match_desc_response, status_code %d",
              (FMT__D, error_code));

    if (error_code == RET_OK)
    {
        TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_zdo_match_desc_response", (FMT__0));

        ncp_host_hl_init_response_body(response, len, &body);

        ncp_host_hl_buf_get_u8(&body, &match_ep_count);
        TRACE_MSG(TRACE_TRANSPORT3, ">> match endpoint count = %hd ", (FMT__H, match_ep_count));

        ncp_host_hl_buf_get_ptr(&body, &match_ep_list, match_ep_count);

        ncp_host_hl_buf_get_u16(&body, &nwk_addr);
        TRACE_MSG(TRACE_TRANSPORT3, "nwk_addr 0x%x", (FMT__D, nwk_addr));

        TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_zdo_match_desc_response", (FMT__0));
    }

    ncp_host_handle_zdo_match_desc_response(error_code, ncp_tsn, nwk_addr, match_ep_count, match_ep_list);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_zdo_match_ep_response", (FMT__0));
}

static void handle_zdo_bind_response(ncp_hl_response_header_t *response,
                                     zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    zb_uint8_t ncp_tsn = response->tsn;

    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_zdo_bind_response, status_code %d",
              (FMT__D, error_code));

    ncp_host_handle_zdo_bind_response(error_code, ncp_tsn);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_zdo_bind_response", (FMT__0));
}

static void handle_zdo_unbind_response(ncp_hl_response_header_t *response,
                                       zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    zb_uint8_t ncp_tsn = response->tsn;

    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_zdo_unbind_response, status_code %d",
              (FMT__D, error_code));

    ncp_host_handle_zdo_unbind_response(error_code, ncp_tsn);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_zdo_unbind_response", (FMT__0));
}


static void handle_zdo_permit_joining_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    zb_uint8_t ncp_tsn = response->tsn;

    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_zdo_permit_joining_response, status_code %d",
              (FMT__D, error_code));

    ncp_host_handle_zdo_permit_joining_response(error_code, ncp_tsn);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_zdo_permit_joining_response", (FMT__0));
}


static void handle_zdo_mgmt_leave_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    zb_uint8_t ncp_tsn = response->tsn;

    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_zdo_mgmt_leave_response, status_code %d",
              (FMT__D, error_code));

    ncp_host_handle_zdo_mgmt_leave_response(error_code, ncp_tsn);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_zdo_mgmt_leave_response", (FMT__0));
}


static void handle_zdo_mgmt_bind_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    zb_uint8_t ncp_tsn = response->tsn;
    ncp_host_hl_rx_buf_handle_t body;

    zb_uint8_t binding_table_entries;
    zb_uint8_t start_index;
    zb_uint8_t binding_table_list_count;

    ncp_host_zb_zdo_binding_table_record_t table_records[ZB_APS_PAYLOAD_MAX_LEN / sizeof(ncp_host_zb_zdo_binding_table_record_t)];
    ncp_host_zb_zdo_binding_table_record_t *resp_record;
    zb_uindex_t table_entry_index;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_zdo_mgmt_bind_response, status_code %d, len %d",
              (FMT__D_D, error_code, len));

    ZB_BZERO(table_records, sizeof(table_records));

    if (error_code == RET_OK)
    {
        TRACE_MSG(TRACE_TRANSPORT3, "  >> parse handle_zdo_mgmt_bind_response", (FMT__0));

        ncp_host_hl_init_response_body(response, len, &body);

        ncp_host_hl_buf_get_u8(&body, &binding_table_entries);
        TRACE_MSG(TRACE_TRANSPORT3, "    >> binding_table_entries = %hd ", (FMT__H, binding_table_entries));

        ncp_host_hl_buf_get_u8(&body, &start_index);
        TRACE_MSG(TRACE_TRANSPORT3, "    >> start_index = %hd ", (FMT__H, start_index));

        ncp_host_hl_buf_get_u8(&body, &binding_table_list_count);
        TRACE_MSG(TRACE_TRANSPORT3, "    >> binding_table_list_count = %hd ", (FMT__H, binding_table_list_count));

        for (table_entry_index = 0; table_entry_index < binding_table_list_count; table_entry_index++)
        {
            resp_record = &table_records[table_entry_index];

            ncp_host_hl_buf_get_u64addr(&body, resp_record->src_address);
            ncp_host_hl_buf_get_u8(&body, &resp_record->src_endp);
            ncp_host_hl_buf_get_u16(&body, &resp_record->cluster_id);
            ncp_host_hl_buf_get_u8(&body, &resp_record->dst_addr_mode);

            if (resp_record->dst_addr_mode == ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT
                    || resp_record->dst_addr_mode == ZB_APS_ADDR_MODE_16_ENDP_PRESENT)
            {
                zb_uint32_t unused_u32;
                zb_uint16_t unused_u16;

                ncp_host_hl_buf_get_u16(&body, &resp_record->dst_address.addr_short);
                ncp_host_hl_buf_get_u32(&body, &unused_u32);
                ncp_host_hl_buf_get_u16(&body, &unused_u16);
            }
            else
            {
                ncp_host_hl_buf_get_u64addr(&body, resp_record->dst_address.addr_long);
            }

            ncp_host_hl_buf_get_u8(&body, &resp_record->dst_endp);

            TRACE_MSG(TRACE_TRANSPORT3, "    >> binding_table_entry #%hd ", (FMT__H, table_entry_index));

            TRACE_MSG(TRACE_TRANSPORT3, "      src_address: " TRACE_FORMAT_64,
                      (FMT__A, TRACE_ARG_64(resp_record->src_address)));
            TRACE_MSG(TRACE_TRANSPORT3, "      src_endp: %hd", (FMT__H, resp_record->src_endp));
            TRACE_MSG(TRACE_TRANSPORT3, "      cluster_id: %d", (FMT__D, resp_record->cluster_id));
            TRACE_MSG(TRACE_TRANSPORT3, "      dst_addr_mode: %hd", (FMT__H, resp_record->dst_addr_mode));

            if (resp_record->dst_addr_mode == ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT
                    || resp_record->dst_addr_mode == ZB_APS_ADDR_MODE_16_ENDP_PRESENT)
            {
                TRACE_MSG(TRACE_TRANSPORT3, "      dst_address: %d", (FMT__D, resp_record->dst_address.addr_short));
            }
            else
            {
                TRACE_MSG(TRACE_TRANSPORT3, "      dst_address: " TRACE_FORMAT_64,
                          (FMT__A, TRACE_ARG_64(resp_record->dst_address.addr_long)));
            }

            TRACE_MSG(TRACE_TRANSPORT3, "      dst_endp: %hd", (FMT__H, resp_record->dst_endp));

            TRACE_MSG(TRACE_TRANSPORT3, "    << binding_table_entry", (FMT__0));
        }

        TRACE_MSG(TRACE_TRANSPORT3, "  << parse handle_zdo_mgmt_bind_response", (FMT__0));
    }

    ncp_host_handle_zdo_mgmt_bind_response(error_code, ncp_tsn, binding_table_entries, start_index,
                                           binding_table_list_count, table_records);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_zdo_mgmt_bind_response", (FMT__0));
}


static void handle_zdo_mgmt_lqi_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    zb_uint8_t ncp_tsn = response->tsn;
    ncp_host_hl_rx_buf_handle_t body;

    zb_uint8_t neighbor_table_entries;
    zb_uint8_t start_index;
    zb_uint8_t neighbor_table_list_count;

    ncp_host_zb_zdo_neighbor_table_record_t table_records[ZB_APS_PAYLOAD_MAX_LEN / sizeof(ncp_host_zb_zdo_neighbor_table_record_t)];
    ncp_host_zb_zdo_neighbor_table_record_t *resp_record;
    zb_uindex_t table_entry_index;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_zdo_mgmt_lqi_response, status_code %d",
              (FMT__D, error_code));

    if (error_code == RET_OK)
    {
        TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_zdo_mgmt_lqi_response", (FMT__0));

        ncp_host_hl_init_response_body(response, len, &body);

        ncp_host_hl_buf_get_u8(&body, &neighbor_table_entries);
        TRACE_MSG(TRACE_TRANSPORT3, ">> neighbor_table_entries = %hd ", (FMT__H, neighbor_table_entries));

        ncp_host_hl_buf_get_u8(&body, &start_index);
        TRACE_MSG(TRACE_TRANSPORT3, ">> start_index = %hd ", (FMT__H, start_index));

        ncp_host_hl_buf_get_u8(&body, &neighbor_table_list_count);
        TRACE_MSG(TRACE_TRANSPORT3, ">> neighbor_table_list_count = %hd ", (FMT__H, neighbor_table_list_count));

        for (table_entry_index = 0; table_entry_index < neighbor_table_list_count; table_entry_index++)
        {
            resp_record = &table_records[table_entry_index];

            ncp_host_hl_buf_get_u64addr(&body, resp_record->ext_pan_id);
            ncp_host_hl_buf_get_u64addr(&body, resp_record->ext_addr);
            ncp_host_hl_buf_get_u16(&body, &resp_record->network_addr);
            ncp_host_hl_buf_get_u8(&body, &resp_record->type_flags);
            ncp_host_hl_buf_get_u8(&body, &resp_record->permit_join);
            ncp_host_hl_buf_get_u8(&body, &resp_record->depth);
            ncp_host_hl_buf_get_u8(&body, &resp_record->lqi);

            TRACE_MSG(TRACE_TRANSPORT3, "    >> neighbor_table_entry #%hd ", (FMT__H, table_entry_index));
            TRACE_MSG(TRACE_TRANSPORT3, "      ext_pan_id: " TRACE_FORMAT_64,
                      (FMT__A, TRACE_ARG_64(resp_record->ext_pan_id)));
            TRACE_MSG(TRACE_TRANSPORT3, "      ext_addr: " TRACE_FORMAT_64,
                      (FMT__A, TRACE_ARG_64(resp_record->ext_addr)));
            TRACE_MSG(TRACE_TRANSPORT3, "      network_addr: %d", (FMT__D, resp_record->network_addr));
            TRACE_MSG(TRACE_TRANSPORT3, "      type_flags: 0x%hx", (FMT__H, resp_record->type_flags));
            TRACE_MSG(TRACE_TRANSPORT3, "      permit_join: %hd", (FMT__H, resp_record->permit_join));
            TRACE_MSG(TRACE_TRANSPORT3, "      depth: %hd", (FMT__H, resp_record->depth));
            TRACE_MSG(TRACE_TRANSPORT3, "      lqi: %hd", (FMT__H, resp_record->lqi));
            TRACE_MSG(TRACE_TRANSPORT3, "    << neighbor_table_entry", (FMT__0));
        }

        TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_zdo_mgmt_lqi_response", (FMT__0));
    }

    ncp_host_handle_zdo_mgmt_lqi_response(error_code, ncp_tsn, neighbor_table_entries, start_index,
                                          neighbor_table_list_count, table_records);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_zdo_mgmt_lqi_response", (FMT__0));
}


static void handle_zdo_mgmt_nwk_update_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    zb_uint8_t ncp_tsn = response->tsn;
    ncp_host_hl_rx_buf_handle_t body;

    zb_uint32_t scanned_channels;
    zb_uint16_t total_transmissions;
    zb_uint16_t transmission_failures;
    zb_uint8_t scanned_channels_list_count;
    zb_uint8_t *energy_values;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_zdo_mgmt_lqi_response, status_code %d",
              (FMT__D, error_code));

    if (error_code == RET_OK)
    {
        TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_zdo_mgmt_lqi_response", (FMT__0));

        ncp_host_hl_init_response_body(response, len, &body);

        ncp_host_hl_buf_get_u32(&body, &scanned_channels);
        TRACE_MSG(TRACE_TRANSPORT3, ">> scanned_channels = 0x%lx", (FMT__L, scanned_channels));

        ncp_host_hl_buf_get_u16(&body, &total_transmissions);
        TRACE_MSG(TRACE_TRANSPORT3, ">> total_transmissions = %d", (FMT__D, total_transmissions));

        ncp_host_hl_buf_get_u16(&body, &transmission_failures);
        TRACE_MSG(TRACE_TRANSPORT3, ">> transmission_failures = %d", (FMT__D, transmission_failures));

        ncp_host_hl_buf_get_u8(&body, &scanned_channels_list_count);
        TRACE_MSG(TRACE_TRANSPORT3, ">> scanned_channels_list_count = %hd", (FMT__H, scanned_channels_list_count));

        ncp_host_hl_buf_get_ptr(&body, &energy_values, scanned_channels_list_count * sizeof(zb_uint8_t));

        TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_zdo_mgmt_lqi_response", (FMT__0));
    }

    ncp_host_handle_zdo_mgmt_nwk_update_response(error_code, ncp_tsn, scanned_channels, total_transmissions,
            transmission_failures, scanned_channels_list_count, energy_values);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_zdo_mgmt_lqi_response", (FMT__0));
}


void handle_zdo_rejoin_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);

    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_zdo_rejoin_response, status_code %d",
              (FMT__D, error_code));

    ncp_host_handle_zdo_rejoin_response(error_code);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_zdo_rejoin_response", (FMT__0));
}

void handle_zdo_system_server_discovery_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    zb_uint8_t ncp_tsn = response->tsn;
    ncp_host_hl_rx_buf_handle_t body;
    zb_uint16_t server_mask;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_zdo_system_server_discovery_response, status_code %d",
              (FMT__D, error_code));

    if (error_code == RET_OK)
    {
        ncp_host_hl_init_response_body(response, len, &body);

        ncp_host_hl_buf_get_u16(&body, &server_mask);
        TRACE_MSG(TRACE_TRANSPORT3, "server_mask 0x%x", (FMT__D, server_mask));
    }

    ncp_host_handle_zdo_system_server_discovery_response(error_code, ncp_tsn, server_mask);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_zdo_system_server_discovery_response", (FMT__0));
}


static void read_zdo_diagnostics_get_stats_response(ncp_host_hl_rx_buf_handle_t *resp_body,
        zdo_diagnostics_full_stats_t **full_stats_ptr)
{
    zdo_diagnostics_full_stats_t *full_stats = NULL;

    ncp_host_hl_buf_get_ptr(resp_body, (zb_uint8_t **)full_stats_ptr, sizeof(*full_stats));

    full_stats = *full_stats_ptr;

    /* mac_stats */
    ZB_LETOH32_ONPLACE(full_stats->mac_stats.mac_rx_bcast);
    ZB_LETOH32_ONPLACE(full_stats->mac_stats.mac_tx_bcast);
    ZB_LETOH32_ONPLACE(full_stats->mac_stats.mac_rx_ucast);

    TRACE_MSG(TRACE_TRANSPORT3, "mac_rx_bcast %ld, mac_tx_bcast %ld, mac_rx_ucast %ld",
              (FMT__L_L_L, full_stats->mac_stats.mac_rx_bcast, full_stats->mac_stats.mac_tx_bcast,
               full_stats->mac_stats.mac_rx_ucast));

    ZB_LETOH32_ONPLACE(full_stats->mac_stats.mac_tx_ucast_total_zcl);
    ZB_LETOH16_ONPLACE(full_stats->mac_stats.mac_tx_ucast_failures_zcl);
    ZB_LETOH16_ONPLACE(full_stats->mac_stats.mac_tx_ucast_retries_zcl);

    TRACE_MSG(TRACE_TRANSPORT3, "mac_tx_ucast_total_zcl %ld, mac_tx_ucast_failures_zcl %d, mac_tx_ucast_retries_zcl %d",
              (FMT__L_D_D, full_stats->mac_stats.mac_tx_ucast_total_zcl, full_stats->mac_stats.mac_tx_ucast_failures_zcl,
               full_stats->mac_stats.mac_tx_ucast_retries_zcl));

    ZB_LETOH16_ONPLACE(full_stats->mac_stats.mac_tx_ucast_total);
    ZB_LETOH16_ONPLACE(full_stats->mac_stats.mac_tx_ucast_failures);
    ZB_LETOH16_ONPLACE(full_stats->mac_stats.mac_tx_ucast_retries);

    TRACE_MSG(TRACE_TRANSPORT3, "mac_tx_ucast_total %d, mac_tx_ucast_failures %d, mac_tx_ucast_retries %d",
              (FMT__D_D_D, full_stats->mac_stats.mac_tx_ucast_total, full_stats->mac_stats.mac_tx_ucast_failures,
               full_stats->mac_stats.mac_tx_ucast_retries));

    /* zdo_stats */
    ZB_LETOH16_ONPLACE(full_stats->zdo_stats.number_of_resets);
    ZB_LETOH16_ONPLACE(full_stats->zdo_stats.aps_tx_bcast);
    ZB_LETOH16_ONPLACE(full_stats->zdo_stats.aps_tx_ucast_success);
    ZB_LETOH16_ONPLACE(full_stats->zdo_stats.aps_tx_ucast_fail);
    ZB_LETOH16_ONPLACE(full_stats->zdo_stats.join_indication);
    ZB_LETOH16_ONPLACE(full_stats->zdo_stats.average_mac_retry_per_aps_message_sent);
    ZB_LETOH16_ONPLACE(full_stats->zdo_stats.packet_buffer_allocate_failures);
    ZB_LETOH16_ONPLACE(full_stats->zdo_stats.nwk_fc_failure);
    ZB_LETOH16_ONPLACE(full_stats->zdo_stats.aps_fc_failure);
    ZB_LETOH16_ONPLACE(full_stats->zdo_stats.nwk_retry_overflow);

    TRACE_MSG(TRACE_TRANSPORT3, "number_of_resets %d, aps_tx_bcast %d, aps_tx_ucast_success %d, aps_tx_ucast_fail %d",
              (FMT__D_D_D_D, full_stats->zdo_stats.number_of_resets, full_stats->zdo_stats.aps_tx_bcast,
               full_stats->zdo_stats.aps_tx_ucast_success, full_stats->zdo_stats.aps_tx_ucast_fail));

    TRACE_MSG(TRACE_TRANSPORT3, "join_indication %d, average_mac_retry_per_aps_message_sent %d, packet_buffer_allocate_failures %d",
              (FMT__D_D_D, full_stats->zdo_stats.join_indication,
               full_stats->zdo_stats.average_mac_retry_per_aps_message_sent,
               full_stats->zdo_stats.packet_buffer_allocate_failures));

    TRACE_MSG(TRACE_TRANSPORT3, "nwk_fc_failure %d, aps_fc_failure %d, nwk_retry_overflow %d",
              (FMT__D_D_D, full_stats->zdo_stats.nwk_fc_failure,
               full_stats->zdo_stats.aps_fc_failure,
               full_stats->zdo_stats.nwk_retry_overflow));
}


void handle_zdo_diagnostics_get_stats_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    zb_uint8_t ncp_tsn = response->tsn;
    ncp_host_hl_rx_buf_handle_t body;
    zdo_diagnostics_full_stats_t *full_stats = NULL;

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_zdo_diagnostics_get_stats_response, status_code %d",
              (FMT__D, error_code));

    if (error_code == RET_OK)
    {
        ncp_host_hl_init_response_body(response, len, &body);

        read_zdo_diagnostics_get_stats_response(&body, &full_stats);
    }

    ncp_host_handle_zdo_diagnostics_get_stats_response(error_code, ncp_tsn, full_stats);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_zdo_diagnostics_get_stats_response", (FMT__0));
}


void handle_zdo_set_node_desc_manuf_code_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
    zb_uint8_t ncp_tsn = response->tsn;

    ZVUNUSED(len);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_zdo_set_node_desc_manuf_code_response, status_code %d",
              (FMT__D, error_code));

    ncp_host_handle_zdo_set_node_desc_manuf_code_response(error_code, ncp_tsn);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_zdo_set_node_desc_manuf_code_response", (FMT__0));
}


void ncp_host_handle_zdo_response(void *data, zb_uint16_t len)
{
    ncp_hl_response_header_t *response_header = (ncp_hl_response_header_t *)data;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_zdo_response", (FMT__0));

    switch (response_header->hdr.call_id)
    {
    case NCP_HL_ZDO_IEEE_ADDR_REQ:
        handle_zdo_ieee_addr_response(response_header, len);
        break;
    case NCP_HL_ZDO_NWK_ADDR_REQ:
        handle_zdo_nwk_addr_response(response_header, len);
        break;
    case NCP_HL_ZDO_POWER_DESC_REQ:
        handle_zdo_power_descriptor_response(response_header, len);
        break;
    case NCP_HL_ZDO_NODE_DESC_REQ:
        handle_zdo_node_descriptor_response(response_header, len);
        break;
    case NCP_HL_ZDO_SIMPLE_DESC_REQ:
        handle_zdo_simple_descriptor_response(response_header, len);
        break;
    case NCP_HL_ZDO_ACTIVE_EP_REQ:
        handle_zdo_active_ep_response(response_header, len);
        break;
    case NCP_HL_ZDO_MATCH_DESC_REQ:
        handle_zdo_match_desc_response(response_header, len);
        break;
    case NCP_HL_ZDO_BIND_REQ:
        handle_zdo_bind_response(response_header, len);
        break;
    case NCP_HL_ZDO_UNBIND_REQ:
        handle_zdo_unbind_response(response_header, len);
        break;
    case NCP_HL_ZDO_PERMIT_JOINING_REQ:
        handle_zdo_permit_joining_response(response_header, len);
        break;
    case NCP_HL_ZDO_MGMT_LEAVE_REQ:
        handle_zdo_mgmt_leave_response(response_header, len);
        break;
    case NCP_HL_ZDO_MGMT_BIND_REQ:
        handle_zdo_mgmt_bind_response(response_header, len);
        break;
    case NCP_HL_ZDO_MGMT_LQI_REQ:
        handle_zdo_mgmt_lqi_response(response_header, len);
        break;
    case NCP_HL_ZDO_MGMT_NWK_UPDATE_REQ:
        handle_zdo_mgmt_nwk_update_response(response_header, len);
        break;
    case NCP_HL_ZDO_REJOIN:
        handle_zdo_rejoin_response(response_header, len);
        break;
    case NCP_HL_ZDO_SYSTEM_SRV_DISCOVERY_REQ:
        handle_zdo_system_server_discovery_response(response_header, len);
        break;
    case NCP_HL_ZDO_GET_STATS:
        handle_zdo_diagnostics_get_stats_response(response_header, len);
        break;
    case NCP_HL_ZDO_SET_NODE_DESC_MANUF_CODE_REQ:
        handle_zdo_set_node_desc_manuf_code_response(response_header, len);
        break;
    default:
        ZB_ASSERT(0);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_zdo_response", (FMT__0));
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
    case NCP_HL_ZDO_DEV_AUTHORIZED_IND:
        handle_zdo_dev_authorized_indication(indication_header, len);
        break;
    case NCP_HL_ZDO_DEV_UPDATE_IND:
        handle_zdo_dev_update_indication(indication_header, len);
        break;
    default:
        ZB_ASSERT(0);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_zdo_indication", (FMT__0));
}

zb_ret_t ncp_host_zdo_ieee_addr_request(zb_uint8_t *ncp_tsn, zb_zdo_ieee_addr_req_param_t *req_param)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_zdo_ieee_addr_request", (FMT__0));

    if (!ncp_host_get_buf_for_nonblocking_request(NCP_HL_ZDO_IEEE_ADDR_REQ, &body, ncp_tsn))
    {
        ncp_host_hl_buf_put_u16(&body, req_param->dst_addr);
        ncp_host_hl_buf_put_u16(&body, req_param->nwk_addr);
        ncp_host_hl_buf_put_u8(&body, req_param->request_type);
        ncp_host_hl_buf_put_u8(&body, req_param->start_index);
        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_zdo_ieee_addr_request, ret %d", (FMT__D, ret));

    return ret;
}

zb_ret_t ncp_host_zdo_nwk_addr_request(zb_uint8_t *ncp_tsn, zb_zdo_nwk_addr_req_param_t *req_param)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_zdo_nwk_addr_request", (FMT__0));

    if (!ncp_host_get_buf_for_nonblocking_request(NCP_HL_ZDO_NWK_ADDR_REQ, &body, ncp_tsn))
    {
        ncp_host_hl_buf_put_u16(&body, req_param->dst_addr);
        ncp_host_hl_buf_put_u64addr(&body, req_param->ieee_addr);
        ncp_host_hl_buf_put_u8(&body, req_param->request_type);
        ncp_host_hl_buf_put_u8(&body, req_param->start_index);

        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_zdo_nwk_addr_request, ret %d", (FMT__D, ret));

    return ret;
}

zb_ret_t ncp_host_zdo_power_desc_request(zb_uint8_t *ncp_tsn, zb_uint16_t nwk_addr)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_zdo_power_desc_request", (FMT__0));

    if (!ncp_host_get_buf_for_nonblocking_request(NCP_HL_ZDO_POWER_DESC_REQ, &body, ncp_tsn))
    {
        ncp_host_hl_buf_put_u16(&body, nwk_addr);
        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_zdo_power_desc_request, ret %d", (FMT__D, ret));

    return ret;
}

zb_ret_t ncp_host_zdo_node_desc_request(zb_uint8_t *ncp_tsn, zb_uint16_t nwk_addr)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_zdo_node_desc_request", (FMT__0));

    if (!ncp_host_get_buf_for_nonblocking_request(NCP_HL_ZDO_NODE_DESC_REQ, &body, ncp_tsn))
    {
        ncp_host_hl_buf_put_u16(&body, nwk_addr);
        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_zdo_node_desc_request, ret %d", (FMT__D, ret));

    return ret;
}

zb_ret_t ncp_host_zdo_simple_desc_request(zb_uint8_t *ncp_tsn, zb_uint16_t nwk_addr, zb_uint8_t endpoint)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_zdo_simple_desc_request", (FMT__0));

    if (!ncp_host_get_buf_for_nonblocking_request(NCP_HL_ZDO_SIMPLE_DESC_REQ, &body, ncp_tsn))
    {
        ncp_host_hl_buf_put_u16(&body, nwk_addr);
        ncp_host_hl_buf_put_u8(&body, endpoint);
        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_zdo_simple_desc_request, ret %d", (FMT__D, ret));

    return ret;
}

zb_ret_t ncp_host_zdo_active_ep_request(zb_uint8_t *ncp_tsn, zb_uint16_t nwk_addr)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_zdo_active_ep_request", (FMT__0));

    if (!ncp_host_get_buf_for_nonblocking_request(NCP_HL_ZDO_ACTIVE_EP_REQ, &body, ncp_tsn))
    {
        ncp_host_hl_buf_put_u16(&body, nwk_addr);
        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_zdo_active_ep_request, ret %d", (FMT__D, ret));

    return ret;
}

zb_ret_t ncp_host_zdo_permit_joining_request(zb_uint8_t *ncp_tsn, zb_uint16_t short_addr,
        zb_uint8_t permit_duration, zb_uint8_t tc_significance)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_zdo_permit_joining_request", (FMT__0));

    if (!ncp_host_get_buf_for_nonblocking_request(NCP_HL_ZDO_PERMIT_JOINING_REQ, &body, ncp_tsn))
    {
        ncp_host_hl_buf_put_u16(&body, short_addr);
        ncp_host_hl_buf_put_u8(&body, permit_duration);
        ncp_host_hl_buf_put_u8(&body, tc_significance);
        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_zdo_permit_joining_request, ret %d", (FMT__D, ret));

    return ret;
}

zb_ret_t ncp_host_zdo_match_desc_request(zb_uint8_t *ncp_tsn, zb_zdo_match_desc_param_t *req)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;
    zb_uint8_t i;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_zdo_match_desc_request", (FMT__0));

    if (!ncp_host_get_buf_for_nonblocking_request(NCP_HL_ZDO_MATCH_DESC_REQ, &body, ncp_tsn))
    {
        ncp_host_hl_buf_put_u16(&body, req->addr_of_interest);
        TRACE_MSG(TRACE_TRANSPORT3, "addr_of_interest 0x%x", (FMT__D, req->addr_of_interest));

        ncp_host_hl_buf_put_u16(&body, req->profile_id);
        TRACE_MSG(TRACE_TRANSPORT3, "app_profile_id 0x%x", (FMT__D, req->profile_id));

        ncp_host_hl_buf_put_u8(&body, req->num_in_clusters);
        TRACE_MSG(TRACE_TRANSPORT3, "num_in_clusters %hd", (FMT__H, req->num_in_clusters));

        ncp_host_hl_buf_put_u8(&body, req->num_out_clusters);
        TRACE_MSG(TRACE_TRANSPORT3, "num_out_clusters %hd", (FMT__H, req->num_out_clusters));

        TRACE_MSG(TRACE_TRANSPORT3, "in_clusters: ", (FMT__0));
        for (i = 0; i < req->num_in_clusters; i++)
        {
            ncp_host_hl_buf_put_u16(&body, req->cluster_list[i]);
            TRACE_MSG(TRACE_TRANSPORT3, " 0x%x", (FMT__D, req->cluster_list[i]));
        }

        TRACE_MSG(TRACE_TRANSPORT3, "out_clusters: ", (FMT__0));
        for (; i < req->num_in_clusters + req->num_out_clusters; i++)
        {
            ncp_host_hl_buf_put_u16(&body, req->cluster_list[i]);
            TRACE_MSG(TRACE_TRANSPORT3, " 0x%x", (FMT__D, req->cluster_list[i]));
        }

        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_zdo_match_desc_request, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_zdo_bind_unbind_request(zb_uint8_t *ncp_tsn, zb_zdo_bind_req_param_t *bind_param,
        zb_bool_t is_bind)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;
    zb_uint16_t ncp_command_id = (is_bind) ? NCP_HL_ZDO_BIND_REQ : NCP_HL_ZDO_UNBIND_REQ;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_zdo_bind_unbind_request", (FMT__0));

    if (!ncp_host_get_buf_for_nonblocking_request(ncp_command_id, &body, ncp_tsn))
    {
        ncp_host_hl_buf_put_u16(&body, bind_param->req_dst_addr);
        ncp_host_hl_buf_put_u64addr(&body, bind_param->src_address);
        ncp_host_hl_buf_put_u8(&body, bind_param->src_endp);
        ncp_host_hl_buf_put_u16(&body, bind_param->cluster_id);
        ncp_host_hl_buf_put_u8(&body, bind_param->dst_addr_mode);

        if (bind_param->dst_addr_mode == ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT)
        {
            ncp_host_hl_buf_put_u16(&body, bind_param->dst_address.addr_short);

            ncp_host_hl_buf_put_u32(&body, 0);
            ncp_host_hl_buf_put_u16(&body, 0);
            ncp_host_hl_buf_put_u8(&body, 0);
        }
        else if (bind_param->dst_addr_mode == ZB_APS_ADDR_MODE_64_ENDP_PRESENT)
        {
            ncp_host_hl_buf_put_u64addr(&body, bind_param->dst_address.addr_long);
            ncp_host_hl_buf_put_u8(&body, bind_param->dst_endp);
        }

        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_zdo_bind_unbind_request, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_zdo_mgmt_leave_request(zb_uint8_t *ncp_tsn, zb_zdo_mgmt_leave_param_t *req)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;
    zb_uint8_t flags = 0;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_zdo_mgmt_leave_request", (FMT__0));

    if (!ncp_host_get_buf_for_nonblocking_request(NCP_HL_ZDO_MGMT_LEAVE_REQ, &body, ncp_tsn))
    {
        if (req->remove_children)
        {
            ZB_SET_BIT_IN_BIT_VECTOR(&flags, NCP_HL_ZDO_LEAVE_FLAG_REMOVE_CHILDREN);
        }

        if (req->rejoin)
        {
            ZB_SET_BIT_IN_BIT_VECTOR(&flags, NCP_HL_ZDO_LEAVE_FLAG_REJOIN);
        }

        ncp_host_hl_buf_put_u16(&body, req->dst_addr);
        TRACE_MSG(TRACE_TRANSPORT3, "dst_addr 0x%x", (FMT__D, req->dst_addr));

        ncp_host_hl_buf_put_u64addr(&body, req->device_address);
        TRACE_MSG(TRACE_TRANSPORT3, "src_addr " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(req->device_address)));

        ncp_host_hl_buf_put_u8(&body, flags);
        TRACE_MSG(TRACE_TRANSPORT3, "flags 0x%hx", (FMT__H, flags));

        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_zdo_mgmt_leave_request, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_zdo_mgmt_bind_request(zb_uint8_t *ncp_tsn, zb_uint16_t dst_addr, zb_uint8_t start_index)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_zdo_mgmt_bind_request", (FMT__0));

    if (!ncp_host_get_buf_for_nonblocking_request(NCP_HL_ZDO_MGMT_BIND_REQ, &body, ncp_tsn))
    {
        ncp_host_hl_buf_put_u16(&body, dst_addr);
        TRACE_MSG(TRACE_TRANSPORT3, "dst_addr 0x%x", (FMT__D, dst_addr));

        ncp_host_hl_buf_put_u8(&body, start_index);
        TRACE_MSG(TRACE_TRANSPORT3, "start_index %hd", (FMT__H, start_index));

        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_zdo_mgmt_bind_request, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_zdo_mgmt_lqi_request(zb_uint8_t *ncp_tsn, zb_uint16_t dst_addr, zb_uint8_t start_index)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_zdo_mgmt_lqi_request", (FMT__0));

    if (!ncp_host_get_buf_for_nonblocking_request(NCP_HL_ZDO_MGMT_LQI_REQ, &body, ncp_tsn))
    {
        ncp_host_hl_buf_put_u16(&body, dst_addr);
        TRACE_MSG(TRACE_TRANSPORT3, "dst_addr 0x%x", (FMT__D, dst_addr));

        ncp_host_hl_buf_put_u8(&body, start_index);
        TRACE_MSG(TRACE_TRANSPORT3, "start_index %hd", (FMT__H, start_index));

        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_zdo_mgmt_lqi_request, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_zdo_mgmt_nwk_update_request(zb_uint8_t *ncp_tsn, zb_uint32_t scan_channels,
        zb_uint8_t scan_duration, zb_uint8_t scan_count,
        zb_uint16_t manager_addr, zb_uint16_t dst_addr)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_zdo_mgmt_nwk_update_request", (FMT__0));

    if (!ncp_host_get_buf_for_nonblocking_request(NCP_HL_ZDO_MGMT_NWK_UPDATE_REQ, &body, ncp_tsn))
    {
        ncp_host_hl_buf_put_u32(&body, scan_channels);
        TRACE_MSG(TRACE_TRANSPORT3, "scan_channels 0x%lx", (FMT__L, scan_channels));

        ncp_host_hl_buf_put_u8(&body, scan_duration);
        TRACE_MSG(TRACE_TRANSPORT3, "scan_duration %hd", (FMT__H, scan_duration));

        ncp_host_hl_buf_put_u8(&body, scan_count);
        TRACE_MSG(TRACE_TRANSPORT3, "scan_count %hd", (FMT__H, scan_count));

        ncp_host_hl_buf_put_u16(&body, manager_addr);
        TRACE_MSG(TRACE_TRANSPORT3, "manager_addr %d", (FMT__D, manager_addr));

        ncp_host_hl_buf_put_u16(&body, dst_addr);
        TRACE_MSG(TRACE_TRANSPORT3, "dst_addr %d", (FMT__D, dst_addr));

        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_zdo_mgmt_nwk_update_request, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_zdo_rejoin_request(zb_uint8_t *ncp_tsn,
                                     zb_ext_pan_id_t ext_pan_id,
                                     zb_channel_page_t *channels_list,
                                     zb_uint8_t secure_rejoin)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_zdo_rejoin_request", (FMT__0));

    if (!ncp_host_get_buf_for_nonblocking_request(NCP_HL_ZDO_REJOIN, &body, ncp_tsn))
    {
        ncp_host_hl_buf_put_u64addr(&body, ext_pan_id);
        ncp_host_hl_buf_put_u8(&body, ZB_CHANNEL_PAGES_NUM /* entries number */);
#if defined ZB_SUBGHZ_BAND_ENABLED
        {
            zb_uint8_t i;
            zb_uint8_t page;
            zb_channel_list_t ch_list = (zb_channel_list_t)channels_list;
            for (i = 0U; i < ZB_CHANNEL_PAGES_NUM; i++)
            {
                page = ZB_CHANNEL_PAGE_FROM_IDX(i);
                ncp_host_hl_buf_put_u8(&body, page);
                ncp_host_hl_buf_put_array(&body, (zb_uint8_t *) ch_list[i], sizeof(zb_channel_page_t));
            }
        }
#else
        ncp_host_hl_buf_put_u8(&body, ZB_CHANNEL_PAGE0_2_4_GHZ);
        ncp_host_hl_buf_put_array(&body, (zb_uint8_t *) channels_list, sizeof(*channels_list));
#endif /* ZB_SUBGHZ_BAND_ENABLED */
        ncp_host_hl_buf_put_u8(&body, secure_rejoin);

        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_zdo_rejoin_request", (FMT__0));

    return ret;
}

zb_ret_t ncp_host_zdo_system_server_discovery_request(zb_uint8_t *ncp_tsn, zb_uint16_t server_mask)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_zdo_system_server_discovery_request", (FMT__0));

    if (!ncp_host_get_buf_for_nonblocking_request(NCP_HL_ZDO_SYSTEM_SRV_DISCOVERY_REQ, &body, ncp_tsn))
    {
        ncp_host_hl_buf_put_u16(&body, server_mask);

        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_zdo_system_server_discovery_request", (FMT__0));

    return ret;
}


zb_ret_t ncp_host_zdo_diagnostics_get_stats_request(zb_uint8_t *ncp_tsn, zb_uint8_t pib_attr)
{
    zb_ret_t ret = RET_OK;
    ncp_host_hl_tx_buf_handle_t body;
    zb_bool_t do_cleanup = ZB_FALSE;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_zdo_diagnostics_get_stats_request", (FMT__0));

    if (!ncp_host_get_buf_for_nonblocking_request(NCP_HL_ZDO_GET_STATS, &body, ncp_tsn))
    {
        switch (pib_attr)
        {
        case ZB_PIB_ATTRIBUTE_IEEE_DIAGNOSTIC_INFO:
            do_cleanup = ZB_FALSE;
            break;
        case ZB_PIB_ATTRIBUTE_GET_AND_CLEANUP_DIAG_INFO:
            do_cleanup = ZB_TRUE;
            break;
        default:
            ret = RET_INVALID_PARAMETER;
            ZB_ASSERT(ZB_FALSE);
            break;
        }

        if (ret == RET_OK)
        {
            ncp_host_hl_buf_put_u8(&body, do_cleanup);

            ret = ncp_host_hl_send_packet(&body);
        }
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_zdo_diagnostics_get_stats_request, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_zdo_set_node_desc_manuf_code(zb_uint16_t manuf_code, zb_uint8_t *ncp_tsn)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_zdo_set_node_desc_manuf_code", (FMT__0));

    if (!ncp_host_get_buf_for_nonblocking_request(NCP_HL_ZDO_SET_NODE_DESC_MANUF_CODE_REQ, &body, ncp_tsn))
    {
        ncp_host_hl_buf_put_u16(&body, manuf_code);
        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_zdo_set_node_desc_manuf_code, ret %d", (FMT__D, ret));

    return ret;
}
