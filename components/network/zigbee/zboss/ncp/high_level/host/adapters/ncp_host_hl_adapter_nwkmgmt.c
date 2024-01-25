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
/*  PURPOSE: NCP High level transport (adapters layer) implementation for the host side: NWK category
*/
#define ZB_TRACE_FILE_ID 17510

#include "ncp_host_hl_transport_internal_api.h"
#include "ncp_host_soc_state.h"
#include "zb_zdo.h"

#ifdef ZB_FORMATION

void zb_nlme_network_formation_request(zb_bufid_t buf)
{
    zb_ret_t ret = RET_BUSY;
    zb_nlme_network_formation_request_t *req = ZB_BUF_GET_PARAM(buf, zb_nlme_network_formation_request_t);

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_nlme_network_formation_request", (FMT__0));

    ncp_host_state_set_joined(ZB_TRUE);
    ncp_host_state_set_authenticated(ZB_TRUE);
    ncp_host_state_set_tclk_valid(ZB_TRUE);

    if (ZB_IEEE_ADDR_IS_ZERO(req->extpanid))
    {
        ncp_host_state_get_long_addr(req->extpanid);
    }

    ncp_host_state_set_extended_pan_id(req->extpanid);

    ret = ncp_host_nwk_formation(req->scan_channels_list, req->scan_duration,
                                 req->distributed_network, req->distributed_network_address, req->extpanid);
    ZB_ASSERT(ret == RET_OK);

    zb_buf_free(buf);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_nlme_network_formation_request, ret %d", (FMT__D, ret));
}


void zb_nwk_cont_without_formation(zb_uint8_t param)
{
    zb_ret_t ret = RET_BUSY;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_nwk_cont_without_formation", (FMT__0));

    ncp_host_state_set_joined(ZB_TRUE);

    ret = ncp_host_nwk_start_without_formation();
    ZB_ASSERT(ret == RET_OK);

    zb_buf_free(param);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_nwk_cont_without_formation, ret %d", (FMT__D, ret));
}

#endif /* ZB_FORMATION */

#ifdef ZB_ROUTER_ROLE

void zb_zdo_start_router(zb_uint8_t param)
{
    zb_ret_t ret;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_zdo_start_router", (FMT__0));

    zb_buf_free(param);

    ret = ncp_host_nwk_nlme_start_router_request(ZB_TURN_OFF_ORDER, ZB_TURN_OFF_ORDER, 0);
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_zdo_start_router", (FMT__0));
}

#endif /* ZB_ROUTER_ROLE */


#ifdef ZB_JOIN_CLIENT

void zb_nlme_leave_request(zb_uint8_t param)
{
    ZVUNUSED(param);
    /* We don't use and should not use this function for NCP Host */

    ZB_ASSERT(0);
}

#endif /* ZB_JOIN_CLIENT */

void zb_nlme_network_discovery_request(zb_bufid_t buf)
{
    zb_ret_t ret = RET_BUSY;
    zb_nlme_network_discovery_request_t *req = ZB_BUF_GET_PARAM(buf, zb_nlme_network_discovery_request_t);

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_nlme_network_discovery_request", (FMT__0));

    ret = ncp_host_nwk_discovery(req->scan_channels_list, req->scan_duration);
    ZB_ASSERT(ret == RET_OK);

    zb_buf_free(buf);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_nlme_network_discovery_request, ret %d", (FMT__D, ret));
}

void zb_nlme_join_request(zb_bufid_t buf)
{
    zb_ret_t ret = RET_BUSY;
    zb_nlme_join_request_t *req = ZB_BUF_GET_PARAM(buf, zb_nlme_join_request_t);

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_nlme_join_request", (FMT__0));

    ret = ncp_host_nwk_nlme_join(req->extended_pan_id, (zb_uint8_t)req->rejoin_network,
                                 req->scan_channels_list, req->scan_duration,
                                 (zb_uint8_t)req->capability_information, req->security_enable);
    ZB_ASSERT(ret == RET_OK);

    zb_buf_free(buf);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_nlme_join_request, ret %d", (FMT__D, ret));
}


/* Set Keepalive Timeout */
void zb_set_keepalive_timeout(zb_uint_t timeout)
{
    zb_ret_t ret = RET_BUSY;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_set_keepalive_timeout", (FMT__0));

#if 0

    if (ncp_host_state_get_zboss_started())
    {
        ret = ncp_host_set_keepalive_timeout(timeout);
        ZB_ASSERT(ret == RET_OK);
    }
    else
    {
        ncp_host_state_set_keepalive_timeout(timeout);
    }

#else
    ZVUNUSED(timeout);
#endif

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_set_keepalive_timeout, ret %d", (FMT__D, ret));
}


/* Set End Device Timeout */
void zb_set_ed_timeout(zb_uint_t timeout)
{
    zb_ret_t ret = RET_BUSY;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_set_ed_timeout", (FMT__0));

    if (ncp_host_state_get_zboss_started())
    {
        ret = ncp_host_set_end_device_timeout(timeout);
        ZB_ASSERT(ret == RET_OK);
    }
    else
    {
        ncp_host_state_set_ed_timeout(timeout);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_set_ed_timeout, ret %d", (FMT__D, ret));
}


/* Set Keepalive Mode */
void zb_set_keepalive_mode(nwk_keepalive_supported_method_t mode)
{
    zb_ret_t ret = RET_BUSY;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_set_keepalive_mode", (FMT__0));

    if (ncp_host_state_get_zboss_started())
    {
        ret = ncp_host_nwk_set_keepalive_mode(mode);
        ZB_ASSERT(ret == RET_OK);
    }
    else
    {
        ncp_host_state_set_keepalive_mode(mode);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_set_keepalive_mode, ret %d", (FMT__D, ret));
}


void zb_start_concentrator_mode(zb_uint8_t radius, zb_uint32_t disc_time)
{
    zb_ret_t ret = RET_BUSY;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_start_concentrator_mode", (FMT__0));

    ret = ncp_host_nwk_start_concentrator_mode(radius, disc_time);
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_start_concentrator_mode, ret %d", (FMT__D, ret));
}


void zb_stop_concentrator_mode(void)
{
    zb_ret_t ret = RET_BUSY;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_stop_concentrator_mode", (FMT__0));

    ret = ncp_host_nwk_stop_concentrator_mode();
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_stop_concentrator_mode, ret %d", (FMT__D, ret));
}


void zb_start_pan_id_conflict_resolution(zb_uint8_t param)
{
    zb_ret_t ret;
    zb_pan_id_conflict_info_t *conflict_info = (zb_pan_id_conflict_info_t *)zb_buf_begin(param);

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_start_pan_id_conflict_resolution", (FMT__0));

    ret = ncp_host_nwk_start_pan_id_conflict_resolution(conflict_info->panid_count, conflict_info->panids);
    ZB_ASSERT(ret == RET_OK);

    zb_buf_free(param);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_start_pan_id_conflict_resolution, ret %d", (FMT__D, ret));
}


void zb_enable_auto_pan_id_conflict_resolution(zb_bool_t status)
{
    zb_ret_t ret;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_enable_auto_pan_id_conflict_resolution", (FMT__0));

    ret = ncp_host_nwk_enable_auto_pan_id_conflict_resolution(status);
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_enable_auto_pan_id_conflict_resolution, ret %d", (FMT__D, ret));
}


void zb_enable_panid_conflict_resolution(zb_bool_t status)
{
    zb_ret_t ret;

    TRACE_MSG(TRACE_TRANSPORT2, ">> zb_enable_panid_conflict_resolution", (FMT__0));

    ret = ncp_host_nwk_enable_pan_id_conflict_resolution(status);
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT2, "<< zb_enable_panid_conflict_resolution, ret %d", (FMT__D, ret));
}


void ncp_host_nwk_permit_joining_adapter(zb_bufid_t buf)
{
    zb_ret_t ret = RET_BUSY;
    zb_nlme_permit_joining_request_t *req = ZB_BUF_GET_PARAM(buf, zb_nlme_permit_joining_request_t);

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_permit_joining_adapter", (FMT__0));

    ret = ncp_host_nwk_permit_joining(req->permit_duration);
    ZB_ASSERT(ret == RET_OK);

    zb_buf_free(buf);

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_permit_joining_adapter, ret %d", (FMT__D, ret));
}


#ifdef ZB_FORMATION

void ncp_host_handle_nwk_formation_response(zb_ret_t status,
        zb_uint16_t short_address,
        zb_uint16_t pan_id,
        zb_uint8_t current_page,
        zb_uint8_t current_channel)
{
    zb_bufid_t buf = zb_buf_get(ZB_TRUE, 0);

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_formation_response", (FMT__0));

    ZB_ASSERT(buf != ZB_BUF_INVALID);

    zb_buf_set_status(buf, status);

    if (status == RET_OK)
    {
        ncp_host_state_set_short_address(short_address);
        ncp_host_state_set_pan_id(pan_id);
        ncp_host_state_set_current_page(current_page);
        ncp_host_state_set_current_channel(current_channel);

        zdo_commissioning_formation_done(buf);
    }
    else
    {
        zdo_commissioning_formation_failed(buf);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_formation_response", (FMT__0));
}


void ncp_host_handle_nwk_start_without_formation_response(zb_ret_t status)
{
    zb_bufid_t buf = zb_buf_get(ZB_TRUE, 0);

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_start_without_formation_response", (FMT__0));

    ZB_ASSERT(buf != ZB_BUF_INVALID);

    zb_buf_set_status(buf, status);

    zdo_commissioning_formation_done(buf);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_start_without_formation_response", (FMT__0));
}

#endif /* ZB_FORMATION */

void ncp_host_handle_nwk_discovery_response(zb_ret_t status, zb_uint8_t network_count,
        ncp_proto_network_descriptor_t *network_descriptors)
{
    zb_bufid_t buf = zb_buf_get(ZB_TRUE, 0);
    zb_nlme_network_discovery_confirm_t *cnf;
    zb_nlme_network_descriptor_t *dsc;
    zb_address_pan_id_ref_t pan_id_ref;
    zb_uint8_t i;
    zb_ret_t ret;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_discovery_response", (FMT__0));

    ZB_ASSERT(buf != ZB_BUF_INVALID);

    zb_buf_set_status(buf, status);

    cnf = (zb_nlme_network_discovery_confirm_t *)zb_buf_begin(buf);
    cnf->status = status;
    cnf->network_count = network_count;

    dsc = (zb_nlme_network_descriptor_t *)(cnf + 1);

    for (i = 0; i < network_count; i++)
    {
        TRACE_MSG(TRACE_TRANSPORT3, "entry number %hd", (FMT__H, i));

        ZB_BZERO(&dsc[i], sizeof(*dsc));

        TRACE_MSG(TRACE_TRANSPORT3, "pan_id = 0x%d, extended_pan_id = " TRACE_FORMAT_64,
                  (FMT__D_A, network_descriptors[i].pan_id,
                   TRACE_ARG_64(network_descriptors[i].extended_pan_id)));

        ret = zb_address_set_pan_id(network_descriptors[i].pan_id,
                                    network_descriptors[i].extended_pan_id,
                                    &pan_id_ref);

        ZB_ASSERT(ret == RET_OK || ret == RET_ALREADY_EXISTS);
        if (ret == RET_OK || ret == RET_ALREADY_EXISTS)
        {
            dsc[i].panid_ref = pan_id_ref;

            /* 07/30/2020 EE CR:MINOR Why use 9 trace lines? 1 is enough. */
            dsc[i].channel_page = network_descriptors[i].channel_page;
            TRACE_MSG(TRACE_TRANSPORT3, "channel_page = %hd", (FMT__H, dsc[i].channel_page));

            dsc[i].logical_channel = network_descriptors[i].channel;
            TRACE_MSG(TRACE_TRANSPORT3, "logical_channel = %hd", (FMT__H, dsc[i].logical_channel));

            dsc[i].permit_joining = network_descriptors[i].flags & 1;
            TRACE_MSG(TRACE_TRANSPORT3, "permit_joining = %hd", (FMT__H, dsc[i].permit_joining));

            dsc[i].router_capacity = (network_descriptors[i].flags >> 1) & 1;
            TRACE_MSG(TRACE_TRANSPORT3, "router_capacity = %hd", (FMT__H, dsc[i].router_capacity));

            dsc[i].end_device_capacity = (network_descriptors[i].flags >> 2) & 1;
            TRACE_MSG(TRACE_TRANSPORT3, "end_device_capacity = %hd", (FMT__H, dsc[i].end_device_capacity));

            dsc[i].stack_profile = (network_descriptors[i].flags >> 4) & 0x03;
            TRACE_MSG(TRACE_TRANSPORT3, "stack_profile = %hd", (FMT__H, dsc[i].stack_profile));

            dsc[i].nwk_update_id = network_descriptors[i].nwk_update_id;
            TRACE_MSG(TRACE_TRANSPORT3, "nwk_update_id = %hd", (FMT__H, dsc[i].nwk_update_id));
        }
        else
        {
            /* TODO: maybe clear PAN ID table? */
            TRACE_MSG(TRACE_ERROR, "failed to save PAN ID info, drop some discovery results", (FMT__0));
            cnf->network_count = i;
            break;
        }
    }

    zdo_handle_nlme_network_discovery_confirm(buf);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_discovery_response", (FMT__0));
}

void ncp_host_handle_nwk_nlme_join_response(zb_ret_t status, zb_uint16_t short_addr,
        zb_ext_pan_id_t extended_pan_id,
        zb_uint8_t channel_page, zb_uint8_t logical_channel,
        zb_uint8_t enhanced_beacon, zb_uint8_t mac_interface)
{
    zb_bufid_t buf = zb_buf_get(ZB_TRUE, 0);
    zb_nlme_join_confirm_t *cnf;

    ZVUNUSED(channel_page);
    ZVUNUSED(enhanced_beacon);
    ZVUNUSED(mac_interface);

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_nlme_join_response", (FMT__0));

    ZB_ASSERT(buf != ZB_BUF_INVALID);

    cnf = ZB_BUF_GET_PARAM(buf, zb_nlme_join_confirm_t);
    cnf->status = status;
    cnf->active_channel = logical_channel;
    cnf->network_address = short_addr;
    /* TODO: recheck if we need channel page in zb_nlme_join_confirm */

    ZB_EXTPANID_COPY(cnf->extended_pan_id, extended_pan_id);

    if (cnf->status == RET_OK)
    {
        ncp_host_state_set_current_page(channel_page);
        ncp_host_state_set_current_channel(cnf->active_channel);
        ncp_host_state_set_short_address(cnf->network_address);
        ncp_host_state_set_extended_pan_id(cnf->extended_pan_id);
        ncp_host_state_set_joined(ZB_TRUE);
        ncp_host_state_set_authenticated(ZB_TRUE);
        ncp_host_state_set_waiting_for_tclk(ZB_TRUE);
        zb_buf_set_status(buf, RET_OK);

        zdo_reset_scanlist(ZB_TRUE);

        if (ncp_host_state_get_device_type() == ZB_NWK_DEVICE_TYPE_ROUTER)
        {
            zdo_commissioning_start_router_confirm(buf);
        }
        else
        {
            zdo_commissioning_handle_dev_annce_sent_event(buf);
        }
    }
    else if (cnf->status == ERROR_CODE(ERROR_CATEGORY_ZDO, ZB_ZDP_STATUS_NOT_AUTHORIZED))
    {
        zdo_commissioning_authentication_failed(buf);
    }
    else
    {
        zdo_commissioning_join_failed(buf);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_nlme_join_response", (FMT__0));
}

void ncp_host_handle_nwk_permit_joining_response(zb_ret_t status)
{
    zb_bufid_t buf = zb_buf_get(ZB_TRUE, 0);

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_permit_joining_response", (FMT__0));

    ZB_ASSERT(buf != ZB_BUF_INVALID);

    zb_buf_set_status(buf, status);

    /* We should not use NWK Permit Joining Request from the NCP Host */
    ZB_ASSERT(0);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_permit_joining_response", (FMT__0));
}


void ncp_host_handle_nwk_nlme_router_start_response(zb_ret_t status)
{
    zb_bufid_t buf = zb_buf_get(ZB_TRUE, 0);

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_nlme_router_start_response", (FMT__0));

    ZB_ASSERT(buf != ZB_BUF_INVALID);

    if (status == RET_OK)
    {
        ncp_host_state_set_joined(ZB_TRUE);

        zdo_commissioning_start_router_confirm(buf);
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "failed to start router, status: 0x%x", (FMT__D, status));
        ZB_ASSERT(0);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_nlme_router_start_response", (FMT__0));
}


/* SoC sends this indication when it finishes rejoin */
void ncp_host_handle_nwk_joined_indication(zb_uint16_t nwk_addr, zb_ext_pan_id_t ext_pan_id,
        zb_uint8_t channel_page, zb_uint8_t channel,
        zb_uint8_t beacon_type, zb_uint8_t mac_interface)
{
    zb_ret_t status;
    zb_bufid_t buf = zb_buf_get(ZB_TRUE, 0);

    ZVUNUSED(beacon_type);
    ZVUNUSED(mac_interface);

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_joined_indication", (FMT__0));

    ZB_ASSERT(buf != ZB_BUF_INVALID);

    zb_buf_set_status(buf, RET_OK);

    ncp_host_state_set_current_page(channel_page);
    ncp_host_state_set_current_channel(channel);
    ncp_host_state_set_short_address(nwk_addr);
    ncp_host_state_set_extended_pan_id(ext_pan_id);
    ncp_host_state_set_joined(ZB_TRUE);
    ncp_host_state_set_authenticated(ZB_TRUE);
    ncp_host_state_set_waiting_for_tclk(ZB_FALSE);

    zdo_reset_scanlist(ZB_TRUE);

    zb_buf_set_status(buf, RET_OK);

    if (ncp_host_state_get_device_type() == ZB_NWK_DEVICE_TYPE_ROUTER)
    {
        zdo_commissioning_start_router_confirm(buf);
    }
    else
    {
        zdo_commissioning_handle_dev_annce_sent_event(buf);
    }

    /* Parent address could change after rejoin, so request it again */
    status = ncp_host_get_parent_address();
    ZB_ASSERT(status == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_joined_indication", (FMT__0));
}

/* SoC sends this indication when it fails to rejoin */
void ncp_host_handle_nwk_join_failed_indication(zb_uint8_t status_category, zb_uint8_t status_code)
{
    zb_bufid_t buf = zb_buf_get(ZB_TRUE, 0);
    zb_ret_t status = ERROR_CODE(status_category, status_code);

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_join_failed_indication", (FMT__0));

    ZB_ASSERT(buf != ZB_BUF_INVALID);

    zb_buf_set_status(buf, status);

    if (status == ERROR_CODE(ERROR_CATEGORY_ZDO, ZB_ZDP_STATUS_NOT_AUTHORIZED))
    {
        zdo_commissioning_authentication_failed(buf);
    }
    else
    {
        zdo_commissioning_join_failed(buf);
    }

    /* If rejoining has failed, set parent address as unknown */
    ncp_host_state_set_parent_short_address(ZB_UNKNOWN_SHORT_ADDR);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_join_failed_indication", (FMT__0));
}


/* SoC sends this indication when children device leaves the network */
void ncp_host_handle_nwk_leave_indication(zb_ieee_addr_t device_addr, zb_uint8_t rejoin)
{
    zb_bufid_t buf = zb_buf_get(ZB_TRUE, 0);
    zb_ieee_addr_t own_address;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_leave_indication", (FMT__0));

    ZB_ASSERT(buf != ZB_BUF_INVALID);

    zb_get_long_address(own_address);

    zb_buf_set_status(buf, RET_OK);

    if (ZB_IEEE_ADDR_CMP(device_addr, own_address))
    {
        if (!rejoin)
        {
            zb_ext_pan_id_t ext_pan_id;
            ZB_EXTPANID_ZERO(ext_pan_id);

            /* TODO: consider resetting other host state fields or moving this code into function
            * like ncp_host_state_reset */

            ncp_host_state_set_joined(ZB_FALSE);
            ncp_host_state_set_authenticated(ZB_FALSE);
            ncp_host_state_set_tclk_valid(ZB_FALSE);
            ncp_host_state_set_extended_pan_id(ext_pan_id);

            zb_buf_get_out_delayed(zdo_commissioning_leave_done);
        }
        else
        {
            /* NOTE: in case of leave with rejoin the host will wait rejoin status indication */
            zb_buf_get_out_delayed(zdo_commissioning_leave_with_rejoin);
        }
    }
    else
    {
        zb_send_leave_indication_signal(buf, device_addr, rejoin);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_leave_indication", (FMT__0));
}


void ncp_host_handle_nwk_address_update_indication(zb_uint16_t nwk_addr)
{
    zb_bufid_t buf = zb_buf_get(ZB_TRUE, 0);

    ZVUNUSED(nwk_addr);

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_address_update_indication", (FMT__0));

    ZB_ASSERT(buf != ZB_BUF_INVALID);

    ncp_host_state_set_short_address(nwk_addr);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_address_update_indication", (FMT__0));
}


void ncp_host_handle_nwk_pan_id_conflict_indication(zb_uint16_t pan_id_count, zb_uint16_t *pan_ids)
{
    zb_pan_id_conflict_info_t *conflict_info;
    zb_bufid_t buf = zb_buf_get(ZB_TRUE, 0);

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_pan_id_conflict_indication", (FMT__0));

    ZB_ASSERT(buf != ZB_BUF_INVALID);

    conflict_info = zb_buf_initial_alloc(buf, sizeof(zb_pan_id_conflict_info_t));
    conflict_info->panid_count = pan_id_count;
    ZB_MEMCPY(conflict_info->panids, pan_ids, sizeof(conflict_info->panids[0]) * pan_id_count);

    TRACE_MSG(TRACE_NWK3, "Sending a ZB_NWK_SIGNAL_PANID_CONFLICT_DETECTED signal", (FMT__0));
    zb_app_signal_pack_with_data(buf, ZB_NWK_SIGNAL_PANID_CONFLICT_DETECTED, RET_OK);
    ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete, buf);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_pan_id_conflict_indication", (FMT__0));
}


void ncp_host_handle_nwk_parent_lost_indication(void)
{
    zb_bufid_t buf = zb_buf_get(ZB_TRUE, 0);
    zb_uint8_t *rejoin_reason;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_parent_lost_indication", (FMT__0));

    ZB_ASSERT(buf != ZB_BUF_INVALID);

    rejoin_reason = ZB_BUF_GET_PARAM(buf, zb_uint8_t);

    *rejoin_reason = ZB_REJOIN_REASON_PARENT_LOST;
    ZB_SCHEDULE_CALLBACK(zdo_commissioning_initiate_rejoin, buf);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_parent_lost_indication", (FMT__0));
}


#ifdef ZB_APSDE_REQ_ROUTING_FEATURES
void ncp_host_handle_nwk_route_request_send_indication(zb_uint16_t nwk_addr)
{
    /* TODO: fill host_ctx.soc_state with new values */
    zb_bufid_t buf = zb_buf_get(ZB_TRUE, 0);

    ZVUNUSED(nwk_addr);

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_route_request_send_indication", (FMT__0));

    ZB_ASSERT(buf != ZB_BUF_INVALID);

    /* TODO: Implement adapter logic, if it is needed */

    ncp_host_handle_nwk_route_request_send_indication_adapter(buf);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_route_request_send_indication", (FMT__0));
}

void ncp_host_handle_nwk_route_reply_indication(zb_uint16_t nwk_addr)
{
    /* TODO: fill host_ctx.soc_state with new values */
    zb_bufid_t buf = zb_buf_get(ZB_TRUE, 0);

    ZVUNUSED(nwk_addr);

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_route_reply_indication", (FMT__0));

    ZB_ASSERT(buf != ZB_BUF_INVALID);

    /* TODO: Implement adapter logic, if it is needed */

    ncp_host_handle_nwk_route_reply_indication_adapter(buf);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_route_reply_indication", (FMT__0));
}

void ncp_host_handle_nwk_route_record_send_indication(zb_uint16_t nwk_addr)
{
    /* TODO: fill host_ctx.soc_state with new values */
    zb_bufid_t buf = zb_buf_get(ZB_TRUE, 0);

    ZVUNUSED(nwk_addr);

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_route_record_send_indication", (FMT__0));

    ZB_ASSERT(buf != ZB_BUF_INVALID);

    /* TODO: Implement adapter logic, if it is needed */

    ncp_host_handle_nwk_route_record_indication_adapter(buf);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_route_record_send_indication", (FMT__0));
}
#endif

void ncp_host_handle_nwk_set_fast_poll_interval_response(zb_ret_t status_code)
{
    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_set_fast_poll_interval_response, status_code %d",
              (FMT__D, status_code));

    ZB_ASSERT(status_code == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_set_fast_poll_interval_response", (FMT__0));
}


void ncp_host_handle_nwk_set_long_poll_interval_response(zb_ret_t status_code)
{
    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_set_long_poll_interval_response, status_code %d",
              (FMT__D, status_code));

    ZB_ASSERT(status_code == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_set_long_poll_interval_response", (FMT__0));
}


void ncp_host_handle_nwk_start_fast_poll_response(zb_ret_t status_code)
{
    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_start_fast_poll_response, status_code %d",
              (FMT__D, status_code));

    ZB_ASSERT(status_code == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_start_fast_poll_response", (FMT__0));
}


void ncp_host_handle_nwk_start_turbo_poll_packets_response(zb_ret_t status_code)
{
    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_start_turbo_poll_packets_response, status_code %d",
              (FMT__D, status_code));

    ZB_ASSERT(status_code == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_start_turbo_poll_packets_response", (FMT__0));
}


void ncp_host_handle_nwk_turbo_poll_cancel_packet_response(zb_ret_t status_code)
{
    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_turbo_poll_cancel_packet_response, status_code %d",
              (FMT__D, status_code));

    ZB_ASSERT(status_code == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_turbo_poll_cancel_packet_response", (FMT__0));
}


void ncp_host_handle_nwk_start_turbo_poll_continuous_response(zb_ret_t status_code)
{
    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_start_turbo_poll_continuous_response, status_code %d",
              (FMT__D, status_code));

    ZB_ASSERT(status_code == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_start_turbo_poll_continuous_response", (FMT__0));
}


void ncp_host_handle_nwk_permit_turbo_poll_response(zb_ret_t status_code)
{
    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_permit_turbo_poll_response, status_code %d",
              (FMT__D, status_code));

    ZB_ASSERT(status_code == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_permit_turbo_poll_response", (FMT__0));
}


void ncp_host_handle_nwk_set_fast_poll_timeout_response(zb_ret_t status_code)
{
    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_set_fast_poll_timeout_response, status_code %d",
              (FMT__D, status_code));

    ZB_ASSERT(status_code == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_set_fast_poll_timeout_response", (FMT__0));
}



void ncp_host_handle_nwk_start_poll_response(zb_ret_t status_code)
{
    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_start_poll_response, status_code %d",
              (FMT__D, status_code));

    ZB_ASSERT(status_code == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_start_poll_response", (FMT__0));
}


void ncp_host_handle_nwk_stop_poll_response(zb_ret_t status_code)
{
    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_stop_poll_response, status_code %d",
              (FMT__D, status_code));

    ZB_ASSERT(status_code == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_stop_poll_response", (FMT__0));
}


void ncp_host_handle_nwk_enable_turbo_poll_response(zb_ret_t status_code)
{
    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_enable_turbo_poll_response, status_code %d",
              (FMT__D, status_code));

    ZB_ASSERT(status_code == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_enable_turbo_poll_response", (FMT__0));
}


void ncp_host_handle_nwk_disable_turbo_poll_response(zb_ret_t status_code)
{
    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_disable_turbo_poll_response, status_code %d",
              (FMT__D, status_code));

    ZB_ASSERT(status_code == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_disable_turbo_poll_response", (FMT__0));
}


void ncp_host_handle_nwk_turbo_poll_continuous_leave_response(zb_ret_t status_code)
{
    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_turbo_poll_continuous_leave_response, status_code %d",
              (FMT__D, status_code));

    ZB_ASSERT(status_code == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_turbo_poll_continuous_leave_response", (FMT__0));
}


void ncp_host_handle_nwk_turbo_poll_packets_leave_response(zb_ret_t status_code)
{
    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_turbo_poll_packets_leave_response, status_code %d",
              (FMT__D, status_code));

    ZB_ASSERT(status_code == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_turbo_poll_packets_leave_response", (FMT__0));
}


void ncp_host_handle_nwk_start_concentrator_mode_response(zb_ret_t status_code)
{
    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_start_concentrator_mode_response, status_code %d",
              (FMT__D, status_code));

    ZB_ASSERT(status_code == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_start_concentrator_mode_response", (FMT__0));
}


void ncp_host_handle_nwk_stop_concentrator_mode_response(zb_ret_t status_code)
{
    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_stop_concentrator_mode_response, status_code %d",
              (FMT__D, status_code));

    ZB_ASSERT(status_code == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_stop_concentrator_mode_response", (FMT__0));
}


void ncp_host_handle_nwk_start_pan_id_conflict_resolution_response(zb_ret_t status_code)
{
    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_start_pan_id_conflict_resolution_response, status_code %d",
              (FMT__D, status_code));

    ZB_ASSERT(status_code == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_start_pan_id_conflict_resolution_response", (FMT__0));
}


void ncp_host_handle_nwk_enable_pan_id_conflict_resolution_response(zb_ret_t status_code)
{
    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_enable_pan_id_conflict_resolution_response, status_code %d",
              (FMT__D, status_code));

    ZB_ASSERT(status_code == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_enable_pan_id_conflict_resolution_response", (FMT__0));
}


void ncp_host_handle_nwk_enable_auto_pan_id_conflict_resolution_response(zb_ret_t status_code)
{
    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_enable_auto_pan_id_conflict_resolution_response, status_code %d",
              (FMT__D, status_code));

    ZB_ASSERT(status_code == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_enable_auto_pan_id_conflict_resolution_response", (FMT__0));
}
