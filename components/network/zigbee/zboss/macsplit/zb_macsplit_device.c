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
/*  PURPOSE: MAC split functions
*/

#define ZB_TRACE_FILE_ID 6665
#include "zb_common.h"

#ifdef ZB_MACSPLIT_DEVICE

#include "zb_macsplit_transport.h"

#include "zb_mac_globals.h"

#define DEFAULT_MAC_DEVICE_VERSION 0x01000000
static get_mac_device_version_cb_t dev_version_cb = 0;

void zb_mac_send_beacon_request_command(zb_uint8_t unused);
void zb_mac_send_enhanced_beacon_request_command(zb_bufid_t param);

void zb_macsplit_set_cb_dev_version(get_mac_device_version_cb_t cb)
{
    dev_version_cb = cb;
}

/**
   Check that received packet is Confirm, so Host already has a buffer for it, so no need to allocate a new buffer.
 */
zb_bool_t zb_macsplit_call_is_conf(zb_transport_call_type_t call_type)
{
    ZVUNUSED(call_type);
    return ZB_FALSE;
}


void macsplit_indicate_boot(void)
{
    zb_bufid_t bufid = zb_buf_get_out();
    macsplit_device_ver_t *version;

    TRACE_MSG(TRACE_MAC1, ">macsplit_indicate_boot", (FMT__0));
    ZB_ASSERT(bufid);

    version = ZB_BUF_GET_PARAM(bufid, macsplit_device_ver_t);
    version->val = DEFAULT_MAC_DEVICE_VERSION;

    if (dev_version_cb)
    {
        version->val = dev_version_cb();
    }

#ifdef USE_HW_LONG_ADDR
    ZB_IEEE_ADDR_COPY(version->extended_address, (zb_ieee_addr_t *)MAC_PIB().mac_extended_address);
#endif

    TRACE_MSG(TRACE_ERROR, "macsplit_indicate_boot", (FMT__0));
    TRACE_MSG(TRACE_ERROR, "radio device version: %ld", (FMT__L, version->val));

    zb_macsplit_transport_send_data_with_type(bufid, ZB_TRANSPORT_CALL_TYPE_DEVICE_BOOT);
    TRACE_MSG(TRACE_MAC1, "<macsplit_indicate_boot", (FMT__0));
}

/* Macsplit device calls implementation */

void zb_mcps_data_confirm(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_mcps_data_confirm %hd", (FMT__H, param));
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_DEVICE_MCPS_DATA_CONFIRM);
    TRACE_MSG(TRACE_MAC1, "<<zb_mcps_data_confirm", (FMT__0));
}


void zb_mcps_data_indication(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_mcps_data_indication param %hd", (FMT__H, param));
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_DEVICE_MCPS_DATA_INDICATION);
    TRACE_MSG(TRACE_MAC1, "<<zb_mcps_data_indication", (FMT__0));
}


void zb_mcps_poll_indication(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_mcps_poll_indication", (FMT__0));
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_DEVICE_MCPS_POLL_INDICATION);
    TRACE_MSG(TRACE_MAC1, "<<zb_mcps_poll_indication", (FMT__0));
}


void zb_mac_reset_at_transport_open(void)
{
    zb_bufid_t bufid = zb_buf_get_out();
    if (bufid)
    {
        zb_mlme_reset_request_t *req = ZB_BUF_GET_PARAM(bufid, zb_mlme_reset_request_t);
        /* This is an implicit reset. Do not pass its result to the Host. */
        req->set_default_pib = ZB_TRUE;
        ZB_SCHEDULE_CALLBACK(zb_mlme_reset_request, bufid);
        zb_macsplit_transport_reinit();
    }
}


void zb_mlme_reset_confirm(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_mlme_reset_confirm", (FMT__0));
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_RESET_CONFIRM);
    TRACE_MSG(TRACE_MAC1, "<<zb_mlme_reset_confirm", (FMT__0));
}


void zb_mlme_scan_confirm(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_mlme_scan_confirm", (FMT__0));
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_SCAN_CONFIRM);
    TRACE_MSG(TRACE_MAC1, "<<zb_mlme_scan_confirm", (FMT__0));
}


void zb_mlme_beacon_notify_indication(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_mlme_beacon_notify_indication", (FMT__0));
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_BEACON_NOTIFY_INDICATION);
    TRACE_MSG(TRACE_MAC1, "<<zb_mlme_beacon_notify_indication", (FMT__0));
}


void zb_mlme_associate_indication(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_mlme_associate_indication", (FMT__0));
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_ASSOCIATE_INDICATION);
    TRACE_MSG(TRACE_MAC1, "<<zb_mlme_associate_indication", (FMT__0));
}


void zb_mlme_associate_confirm(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_mlme_associate_confirm", (FMT__0));
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_ASSOCIATE_CONFIRM);
    TRACE_MSG(TRACE_MAC1, "<<zb_mlme_associate_confirm", (FMT__0));
}


void zb_mlme_poll_confirm(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_mlme_poll_confirm", (FMT__0));
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_POLL_CONFIRM);
    TRACE_MSG(TRACE_MAC1, "<<zb_mlme_poll_confirm", (FMT__0));
}


void zb_mlme_comm_status_indication(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_mlme_comm_status_indication", (FMT__0));
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_COMM_STATUS_INDICATION);
    TRACE_MSG(TRACE_MAC1, "<<zb_mlme_comm_status_indication", (FMT__0));
}


void zb_mlme_orphan_indication(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_mlme_orphan_indication", (FMT__0));
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_ORPHAN_INDICATION);
    TRACE_MSG(TRACE_MAC1, "<<zb_mlme_orphan_indication", (FMT__0));
}


void zb_mlme_start_confirm(zb_bufid_t param)
{
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_START_CONFIRM);
}


void zb_mlme_get_confirm(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_mlme_get_confirm", (FMT__0));
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_GET_CONFIRM);
    TRACE_MSG(TRACE_MAC1, "<<zb_mlme_get_confirm", (FMT__0));
}


void zb_mlme_set_confirm(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_mlme_set_confirm", (FMT__0));
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_SET_CONFIRM);
    TRACE_MSG(TRACE_MAC1, "<<zb_mlme_set_confirm", (FMT__0));
}


void zb_mcps_purge_indirect_queue_confirm(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_mcps_purge_indirect_queue_confirm", (FMT__0));
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_DEVICE_MCPS_PURGE_INDIRECT_QUEUE_CONFIRM);
    TRACE_MSG(TRACE_MAC1, "<<zb_mcps_purge_indirect_queue_confirm", (FMT__0));
}


void zb_mlme_duty_cycle_mode_indication(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_mlme_duty_cycle_mode_indication", (FMT__0));
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_DUTY_CYCLE_MODE_INDICATION);
    TRACE_MSG(TRACE_MAC1, "<<zb_mlme_duty_cycle_mode_indication", (FMT__0));
}


void zb_mlme_get_power_information_table_confirm(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_mlme_get_power_information_table_confirm", (FMT__0));
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_GET_POWER_INFO_TABLE_CONFIRM);
    TRACE_MSG(TRACE_MAC1, "<<zb_mlme_get_power_information_table_confirm", (FMT__0));
}


void zb_mlme_set_power_information_table_confirm(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_mlme_set_power_information_table_confirm", (FMT__0));
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_SET_POWER_INFO_TABLE_CONFIRM);
    TRACE_MSG(TRACE_MAC1, "<<zb_mlme_set_power_information_table_confirm", (FMT__0));
}


void zb_macsplit_get_device_version_confirm(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_macsplit_get_device_version_confirm", (FMT__0));
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_DEVICE_MACSPLIT_GET_VERSION_CONFIRM);
    TRACE_MSG(TRACE_MAC1, "<<zb_macsplit_get_device_version_confirm", (FMT__0));
}


#if defined ZB_ENABLE_ZGP_DIRECT || defined ZB_ZGPD_ROLE
void zb_gp_mcps_data_indication(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_gp_mcps_data_indication", (FMT__0));
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_DEVICE_GP_MCPS_DATA_INDICATION);
    TRACE_MSG(TRACE_MAC1, "<<zb_gp_mcps_data_indication", (FMT__0));
}
#endif /* defined ZB_ENABLE_ZGP_DIRECT || defined ZB_ZGPD_ROLE */

#ifdef ZB_LIMIT_VISIBILITY
static void zb_macsplit_add_visible_device(zb_bufid_t param)
{
    zb_ieee_addr_t long_addr;
    zb_uint8_t *v_param = zb_buf_get_tail(param, sizeof(zb_ieee_addr_t));

    ZB_IEEE_ADDR_COPY(long_addr, v_param);

    mac_add_visible_device(long_addr);

    zb_buf_free(param);
}

static void zb_macsplit_add_invisible_short(zb_bufid_t param)
{
    zb_uint16_t *v_param = ZB_BUF_GET_PARAM(param, zb_uint16_t);

    mac_add_invisible_short(*v_param);

    zb_buf_free(param);
}

static void zb_macsplit_remove_invisible_short(zb_bufid_t param)
{
    zb_uint16_t *v_param = ZB_BUF_GET_PARAM(param, zb_uint16_t);

    mac_remove_invisible_short(*v_param);

    zb_buf_free(param);
}

static void zb_macsplit_clear_mac_filters(zb_bufid_t param)
{
    mac_clear_filters();

    zb_buf_free(param);
}
#endif  /* ZB_LIMIT_VISIBILITY */

#ifdef ZB_TH_ENABLED
static void zb_macsplit_sync_address_update(zb_bufid_t param)
{
    zb_address_ieee_ref_t ref_p;
    zb_macsplit_sync_ieee_addr_update_t *v_param = (zb_macsplit_sync_ieee_addr_update_t *)zb_buf_begin(param);

    zb_address_update(v_param->ieee_address, v_param->short_address, v_param->lock, &ref_p);

    zb_buf_free(param);
}

static void zb_mac_th_configure_for_sending(zb_uint8_t ref)
{
    zb_ret_t ret;
    zb_uint8_t channel_page;
    zb_uint8_t channel_number;
    zb_uint8_t logical_channel;
    zb_uint8_t *buf_payload = zb_buf_begin(ref);
    /* Configures mac level to make it able to send packets. */

    channel_page = buf_payload[0];
    channel_number = buf_payload[1];

    ret = zb_channel_page_channel_number_to_logical(channel_page, channel_number, &logical_channel);
    ZB_ASSERT(ret == RET_OK);

    zb_mac_change_channel(channel_page, logical_channel);
    /* changing channel resets rx_on flag to rx_on_when_idle that might be false */
    ZB_TRANSCEIVER_SET_RX_ON_OFF(ZB_TRUE);

    zb_buf_free(ref);
}
#endif

void zb_macsplit_handle_call(zb_bufid_t param, zb_transport_call_type_t call_type)
{
    switch (call_type)
    {
    case ZB_TRANSPORT_CALL_TYPE_HOST_RESET:
        TRACE_MSG(TRACE_ERROR, "ZB_TRANSPORT_CALL_TYPE_HOST_RESET: soft reset MCU", (FMT__0));
        zb_reset(0);
        break;

    case ZB_TRANSPORT_CALL_TYPE_HOST_MCPS_DATA_REQUEST:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_HOST_MCPS_DATA_REQUEST", (FMT__0));
        ZB_SCHEDULE_CALLBACK(zb_mcps_data_request, param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_HOST_MCPS_DATA_REQUEST", (FMT__0));
        break;

    case ZB_TRANSPORT_CALL_TYPE_HOST_MLME_GET_REQUEST:
    {
        zb_mlme_get_request_t *get_req = (zb_mlme_get_request_t *)zb_buf_begin(param);
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_HOST_MLME_GET_REQUEST", (FMT__0));
        get_req->confirm_cb_u.cb = zb_mlme_get_confirm;
        ZB_SCHEDULE_CALLBACK(zb_mlme_get_request, param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_HOST_MLME_GET_REQUEST", (FMT__0));
        break;
    }
    case ZB_TRANSPORT_CALL_TYPE_HOST_MLME_SET_REQUEST:
    {
        zb_mlme_set_request_t *set_req = (zb_mlme_set_request_t *)zb_buf_begin(param);
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_HOST_MLME_SET_REQUEST", (FMT__0));
        set_req->confirm_cb_u.cb = zb_mlme_set_confirm;
        ZB_SCHEDULE_CALLBACK(zb_mlme_set_request, param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_HOST_MLME_SET_REQUEST", (FMT__0));
        break;
    }
    case ZB_TRANSPORT_CALL_TYPE_HOST_MLME_RESET_REQUEST:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_HOST_MLME_RESET_REQUEST", (FMT__0));
        ZB_SCHEDULE_CALLBACK(zb_mlme_reset_request, param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_HOST_MLME_RESET_REQUEST", (FMT__0));
        break;

    case ZB_TRANSPORT_CALL_TYPE_HOST_MLME_SCAN_REQUEST:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_HOST_MLME_SCAN_REQUEST", (FMT__0));
        ZB_SCHEDULE_CALLBACK(zb_mlme_scan_request, param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_HOST_MLME_SCAN_REQUEST", (FMT__0));
        break;

    case ZB_TRANSPORT_CALL_TYPE_HOST_MLME_ASSOCIATE_REQUEST:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_HOST_MLME_ASSOCIATE_REQUEST", (FMT__0));
        ZB_SCHEDULE_CALLBACK(zb_mlme_associate_request, param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_HOST_MLME_ASSOCIATE_REQUEST", (FMT__0));
        break;

    case ZB_TRANSPORT_CALL_TYPE_HOST_MLME_POLL_REQUEST:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_HOST_MLME_POLL_REQUEST", (FMT__0));
        ZB_SCHEDULE_CALLBACK(zb_mlme_poll_request, param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_HOST_MLME_POLL_REQUEST", (FMT__0));
        break;

#if defined ZB_COORDINATOR_ROLE || defined ZB_ROUTER_ROLE
    case ZB_TRANSPORT_CALL_TYPE_HOST_MLME_ASSOCIATE_RESPONSE:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_HOST_MLME_ASSOCIATE_RESPONSE", (FMT__0));
        ZB_SCHEDULE_CALLBACK(zb_mlme_associate_response, param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_HOST_MLME_ASSOCIATE_RESPONSE", (FMT__0));
        break;

    case ZB_TRANSPORT_CALL_TYPE_HOST_MLME_ORPHAN_RESPONSE:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_HOST_MLME_ORPHAN_RESPONSE", (FMT__0));
        ZB_SCHEDULE_CALLBACK(zb_mlme_orphan_response, param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_HOST_MLME_ORPHAN_RESPONSE", (FMT__0));
        break;

    case ZB_TRANSPORT_CALL_TYPE_HOST_MLME_START_REQUEST:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_HOST_MLME_START_REQUEST", (FMT__0));
        ZB_SCHEDULE_CALLBACK(zb_mlme_start_request, param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_HOST_MLME_START_REQUEST", (FMT__0));
        break;

    case ZB_TRANSPORT_CALL_TYPE_HOST_SEND_EMPTY_FRAME:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_HOST_SEND_EMPTY_FRAME", (FMT__0));
        ZB_SCHEDULE_CALLBACK(zb_mac_resp_by_empty_frame, param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_HOST_SEND_EMPTY_FRAME", (FMT__0));
        break;

    case ZB_TRANSPORT_CALL_TYPE_HOST_MCPS_PURGE_INDIRECT_QUEUE_REQUEST:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_HOST_MCPS_PURGE_INDIRECT_QUEUE_REQUEST", (FMT__0));
        ZB_SCHEDULE_CALLBACK(zb_mcps_purge_indirect_queue_request, param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_HOST_MCPS_PURGE_INDIRECT_QUEUE_REQUEST", (FMT__0));
        break;

    case ZB_TRANSPORT_CALL_TYPE_HOST_MACSPLIT_GET_VERSION_REQUEST:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_HOST_MACSPLIT_GET_VERSION_REQUEST", (FMT__0));
        ZB_SCHEDULE_CALLBACK(zb_macsplit_send_device_stack_information, param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_HOST_MACSPLIT_GET_VERSION_REQUEST", (FMT__0));
        break;
#ifdef ZB_LIMIT_VISIBILITY
    case ZB_TRANSPORT_CALL_TYPE_HOST_MAC_ADD_VISIBLE_LONG:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_HOST_MAC_ADD_VISIBLE_LONG", (FMT__0));
        zb_macsplit_add_visible_device(param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_HOST_MAC_ADD_VISIBLE_LONG", (FMT__0));
        break;

    case ZB_TRANSPORT_CALL_TYPE_HOST_MAC_ADD_INVISIBLE_SHORT:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_HOST_MAC_ADD_INVISIBLE_SHORT", (FMT__0));
        zb_macsplit_add_invisible_short(param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_HOST_MAC_ADD_INVISIBLE_SHORT", (FMT__0));
        break;

    case ZB_TRANSPORT_CALL_TYPE_HOST_MAC_REMOVE_INVISIBLE_SHORT:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_HOST_MAC_REMOVE_INVISIBLE_SHORT", (FMT__0));
        zb_macsplit_remove_invisible_short(param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_HOST_MAC_REMOVE_INVISIBLE_SHORT", (FMT__0));
        break;

    case ZB_TRANSPORT_CALL_TYPE_HOST_MAC_CLEAR_FILTERS:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_HOST_MAC_CLEAR_FILTERS", (FMT__0));
        zb_macsplit_clear_mac_filters(param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_HOST_MAC_CLEAR_FILTERS", (FMT__0));
        break;
#endif  /* ZB_LIMIT_VISIBILITY */
    case ZB_TRANSPORT_CALL_TYPE_HOST_MAC_SEND_BEACON_REQUEST:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_HOST_MAC_SEND_BEACON_REQUEST", (FMT__0));
        if (ZB_SCHEDULE_TX_CB(zb_mac_send_beacon_request_command, 0) != RET_OK)
        {
            TRACE_MSG(TRACE_ERROR, "Oops - out of MAC queue!", (FMT__0));
        }

        zb_buf_free(param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_HOST_MAC_SEND_BEACON_REQUEST", (FMT__0));
        break;

    case ZB_TRANSPORT_CALL_TYPE_HOST_MAC_SEND_ENHANCED_BEACON_REQUEST:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_HOST_MAC_SEND_ENHANCED_BEACON_REQUEST", (FMT__0));
        if (ZB_SCHEDULE_TX_CB(zb_mac_send_enhanced_beacon_request_command, param) != RET_OK)
        {
            TRACE_MSG(TRACE_ERROR, "Oops - out of MAC queue!", (FMT__0));
        }

        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_HOST_MAC_SEND_ENHANCED_BEACON_REQUEST", (FMT__0));
        break;

#ifdef ZB_TH_ENABLED
    case ZB_TRANSPORT_CALL_TYPE_HOST_MAC_SYNC_ADDRESS_UPDATE:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_HOST_MAC_SYNC_ADDRESS_UPDATE", (FMT__0));
        zb_macsplit_sync_address_update(param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_HOST_MAC_SYNC_ADDRESS_UPDATE", (FMT__0));
        break;
    case ZB_TRANSPORT_CALL_TYPE_HOST_MAC_CONFIGURE_FOR_SENDING:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_HOST_MAC_INIT_FOR_SENDING", (FMT__0));
        zb_mac_th_configure_for_sending(param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_HOST_MAC_INIT_FOR_SENDING", (FMT__0));
        break;
#endif

#endif /*  defined ZB_COORDINATOR_ROLE || defined ZB_ROUTER_ROLE */

#if defined ZB_MAC_POWER_CONTROL
    case ZB_TRANSPORT_CALL_TYPE_HOST_MLME_SET_POWER_INFORMATION_TABLE_REQUEST:
        TRACE_MSG(TRACE_MAC1, ">> ZB_TRANSPORT_CALL_TYPE_HOST_MLME_SET_POWER_INFORMATION_TABLE_REQUEST", (FMT__0));
        zb_mlme_set_power_information_table_confirm(param);
        TRACE_MSG(TRACE_MAC1, "<< ZB_TRANSPORT_CALL_TYPE_HOST_MLME_SET_POWER_INFORMATION_TABLE_REQUEST", (FMT__0));
        break;
#endif /* defined ZB_MAC_POWER_CONTROL */

    default:
        if (MACSPLIT_CTX().recv_data_cb)
        {
            TRACE_MSG(TRACE_MAC1, "Received non-call packet", (FMT__0));
            MACSPLIT_CTX().recv_data_cb(param);
        }
        else
        {
            TRACE_MSG(TRACE_MAC1, "handle_packet(): non-call packet handler is absent!", (FMT__0));
            zb_buf_free(param);
        }
        break;
    }
}

#endif /* ZB_MACSPLIT_DEVICE */
