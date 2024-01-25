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

#define ZB_TRACE_FILE_ID 52
#include "zb_common.h"

#ifdef ZB_MACSPLIT_HOST

#include "zb_macsplit_transport.h"
#include "zb_macsplit_transport_linux.h"
#include "zb_error_indication.h"

/**
 * Initialization of UART
 *
 * @return nothing
 */
void zb_mac_init()
{
    /*MP: Macsplit transport initialization moved into zb_mlme_reset_request()
     * as it is the first moment the transport is actually necessary.
     * Rationale: runtime switch between connected transceivers by selecting
     * appropriate transport device (e. g. UART port).
     * Hope it won't hurt.
     */
    ZB_BZERO(&MACSPLIT_CTX(), sizeof(MACSPLIT_CTX()));
}

void zb_macsplit_set_device_trace_cb(zb_macsplit_device_trace_cb_t cb)
{
    TRACE_MSG(TRACE_INFO1, "set up device trace callback", (FMT__0));
    MACSPLIT_CTX().device_trace_cb = cb;
}

/**
 * Main MAC logic loop
 * Reads from UART (if data are available).
 *
 * @return if some data were read
 */
zb_ret_t zb_mac_logic_iteration()
{
    zb_ret_t ret = RET_BLOCKED;

    do
    {
        if (ZB_RING_BUFFER_IS_EMPTY(&ZB_IOCTX().in_buffer))
        {
            ZB_TRANSPORT_NONBLOCK_ITERATION();

            if (ZB_RING_BUFFER_IS_EMPTY(&ZB_IOCTX().in_buffer))
            {
                break;
            }
        }

        /* Handle incoming MAC data */
        zb_linux_transport_handle_data();
        ret = RET_OK;
    } while (0);

    return ret;
}


void zb_mac_transport_init()
{
}

void zb_mac_resp_by_empty_frame(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_mac_resp_by_empty_frame", (FMT__0));
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_HOST_SEND_EMPTY_FRAME);
    TRACE_MSG(TRACE_MAC1, "<<zb_mac_resp_by_empty_frame", (FMT__0));
}

/**
   Check that received packet is Confirm, so Host already has a buffer for it, so no need to allocate a new buffer.
 */
zb_bool_t zb_macsplit_call_is_conf(zb_transport_call_type_t call_type)
{
    TRACE_MSG(TRACE_MAC2, ">zb_macsplit_call_is_conf: %hd", (FMT__H, call_type));

    switch (call_type)
    {
    /* Currently handle only data conf, as most frequent case. Later update all other confirms.
     */
    case ZB_TRANSPORT_CALL_TYPE_DEVICE_MCPS_DATA_CONFIRM:
#if 0
    case ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_GET_CONFIRM:
    case ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_SET_CONFIRM:
    case ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_RESET_CONFIRM:
#endif
        TRACE_MSG(TRACE_MAC2, "confirm", (FMT__0));
        return ZB_TRUE;
    default:
        return ZB_FALSE;
    }
}


/* send reset to radio */
void zb_mlme_dev_reset(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_mcps_data_request", (FMT__0));
    /* We do not need a buf actually. */
    zb_buf_reuse(param);
    /* During radio reset we need to send (or resend) only reset command and postpone sending
     * any other command from host until Boot Indication will be received.
     */
    MACSPLIT_CTX().forced_device_reset = 0;
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_HOST_RESET);
    MACSPLIT_CTX().forced_device_reset = 1;
    TRACE_MSG(TRACE_MAC1, "<<zb_mcps_data_request", (FMT__0));
}


void zb_macsplit_mlme_mark_radio_reset(void)
{
    MACSPLIT_CTX().forced_device_reset = 1;
}


/* receive reset indication from radio */
void zb_mlme_dev_reset_conf(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC2, ">radio boot", (FMT__0));

    TRACE_MSG(TRACE_INFO1, "boot indication", (FMT__0));

    if (MACSPLIT_CTX().forced_device_reset)
    {
        macsplit_device_ver_t version;
        zb_zdo_signal_macsplit_dev_boot_params_t *sig_params;

        ZB_MEMCPY(&version, ZB_BUF_GET_PARAM(param, macsplit_device_ver_t), sizeof(macsplit_device_ver_t));

        sig_params = (zb_zdo_signal_macsplit_dev_boot_params_t *)zb_app_signal_pack(param,
                     ZB_MACSPLIT_DEVICE_BOOT, RET_OK, sizeof(zb_zdo_signal_macsplit_dev_boot_params_t));
        sig_params->dev_version = version.val;

#ifdef USE_HW_LONG_ADDR
        ZB_IEEE_ADDR_COPY(sig_params->extended_address, version.extended_address);
#endif

        TRACE_MSG(TRACE_INFO1, "radio device version: 0x%04x", (FMT__D, version.val));

        //ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete, param);
        zb_zdo_startup_complete(param);
        MACSPLIT_CTX().forced_device_reset = 0;
        zb_macsplit_push_tx_queue();

        if (ZG->zdo.handle.start_no_autostart == ZB_FALSE)
        {
            zboss_start_continue();
        }
#ifdef ZB_MAC_CONFIGURABLE_TX_POWER
        if (MACSPLIT_CTX().tx_power_exist)
        {
            /* send TX power array to Radio after boot */
            zb_buf_get_out_delayed(zb_mlme_dev_send_mac_tx_power);
        }
#endif /* ZB_MAC_CONFIGURABLE_TX_POWER */
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Unexpected zb_mlme_dev_reset_conf - seems, device is asserted", (FMT__0));
        ZB_ERROR_RAISE(ZB_ERROR_SEVERITY_FATAL,
                       ERROR_CODE(ERROR_CATEGORY_MACSPLIT, ZB_ERROR_MACSPLIT_RADIO_FAILURE),
                       NULL);
    }

    TRACE_MSG(TRACE_MAC2, "<radio boot", (FMT__0));
}


/* Macsplit host calls implementation */

void zb_mcps_data_request(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_mcps_data_request", (FMT__0));
    ZB_TH_PUSH_PACKET(ZB_TH_NLDE_DATA, ZB_TH_PRIMITIVE_REQUEST, param);
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_HOST_MCPS_DATA_REQUEST);
    TRACE_MSG(TRACE_MAC1, "<<zb_mcps_data_request", (FMT__0));
}


void zb_mlme_get_request(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_mlme_get_request", (FMT__0));
    ZB_TH_PUSH_PACKET(ZB_TH_MLME_GET, ZB_TH_PRIMITIVE_REQUEST, param);
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_HOST_MLME_GET_REQUEST);
    TRACE_MSG(TRACE_MAC1, "<<zb_mlme_get_request", (FMT__0));
}


void zb_mlme_set_request(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_mlme_set_request", (FMT__0));
    ZB_TH_PUSH_PACKET(ZB_TH_MLME_SET, ZB_TH_PRIMITIVE_REQUEST, param);
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_HOST_MLME_SET_REQUEST);
    TRACE_MSG(TRACE_MAC1, "<<zb_mlme_set_request", (FMT__0));
}


void zb_mlme_reset_request(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_mlme_reset_request", (FMT__0));
    ZB_TH_PUSH_PACKET(ZB_TH_MLME_RESET, ZB_TH_PRIMITIVE_REQUEST, param);
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_HOST_MLME_RESET_REQUEST);
    TRACE_MSG(TRACE_MAC1, "<<zb_mlme_reset_request", (FMT__0));
}


void zb_mlme_scan_request(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_mlme_scan_request", (FMT__0));
    ZB_TH_PUSH_PACKET(ZB_TH_MLME_SCAN, ZB_TH_PRIMITIVE_REQUEST, param);
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_HOST_MLME_SCAN_REQUEST);
    TRACE_MSG(TRACE_MAC1, "<<zb_mlme_scan_request", (FMT__0));
}


void zb_mlme_associate_request(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_mlme_associate_request", (FMT__0));
    ZB_TH_PUSH_PACKET(ZB_TH_MLME_ASSOCIATE, ZB_TH_PRIMITIVE_REQUEST, param);
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_HOST_MLME_ASSOCIATE_REQUEST);
    TRACE_MSG(TRACE_MAC1, "<<zb_mlme_associate_request", (FMT__0));
}


void zb_mlme_associate_response(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_mlme_associate_response", (FMT__0));
    ZB_TH_PUSH_PACKET(ZB_TH_MLME_ASSOCIATE, ZB_TH_PRIMITIVE_CONFIRM, param);
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_HOST_MLME_ASSOCIATE_RESPONSE);
    TRACE_MSG(TRACE_MAC1, "<<zb_mlme_associate_response", (FMT__0));
}


void zb_mlme_poll_request(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_mlme_poll_request", (FMT__0));
    ZB_TH_PUSH_PACKET(ZB_TH_MLME_POLL, ZB_TH_PRIMITIVE_REQUEST, param);
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_HOST_MLME_POLL_REQUEST);
    TRACE_MSG(TRACE_MAC1, "<<zb_mlme_poll_request", (FMT__0));
}


void zb_mlme_orphan_response(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_mlme_orphan_response", (FMT__0));
    ZB_TH_PUSH_PACKET(ZB_TH_MLME_ORPHAN, ZB_TH_PRIMITIVE_CONFIRM, param);
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_HOST_MLME_ORPHAN_RESPONSE);
    TRACE_MSG(TRACE_MAC1, "<<zb_mlme_orphan_response", (FMT__0));
}


void zb_mlme_start_request(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_mlme_start_request", (FMT__0));
    ZB_TH_PUSH_PACKET(ZB_TH_MLME_START, ZB_TH_PRIMITIVE_REQUEST, param);
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_HOST_MLME_START_REQUEST);
    TRACE_MSG(TRACE_MAC1, "<<zb_mlme_start_request", (FMT__0));
}


void zb_mcps_purge_indirect_queue_request(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_mcps_purge_indirect_queue_request", (FMT__0));
    ZB_TH_PUSH_PACKET(ZB_TH_MLME_PURGE_INDIRECT_QUEUE, ZB_TH_PRIMITIVE_REQUEST, param);
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_HOST_MCPS_PURGE_INDIRECT_QUEUE_REQUEST);
    TRACE_MSG(TRACE_MAC1, "<<zb_mcps_purge_indirect_queue_request", (FMT__0));
}


void zb_macsplit_get_device_version_request(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>zb_macsplit_get_device_version_request", (FMT__0));
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_HOST_MACSPLIT_GET_VERSION_REQUEST);
    TRACE_MSG(TRACE_MAC1, "<<zb_macsplit_get_device_version_request", (FMT__0));
}

#ifdef ZB_LIMIT_VISIBILITY
void mac_add_visible_device(zb_ieee_addr_t long_addr)
{
    zb_bufid_t bufid = zb_buf_get_any();
    zb_uint8_t *param;

    ZB_ASSERT(bufid);
    param = zb_buf_get_tail(bufid, sizeof(zb_ieee_addr_t));

    ZB_IEEE_ADDR_COPY(param, long_addr);

    TRACE_MSG(TRACE_MAC1, ">>mac_add_visible_device", (FMT__0));
    zb_macsplit_transport_send_data_with_type(bufid, ZB_TRANSPORT_CALL_TYPE_HOST_MAC_ADD_VISIBLE_LONG);
    TRACE_MSG(TRACE_MAC1, "<<mac_add_visible_device", (FMT__0));
}

void mac_add_invisible_short(zb_uint16_t addr)
{
    zb_bufid_t bufid = zb_buf_get_any();
    zb_uint16_t *param;

    ZB_ASSERT(bufid);
    param = ZB_BUF_GET_PARAM(bufid, zb_uint16_t);

    TRACE_MSG(TRACE_MAC1, ">>mac_add_invisible_short", (FMT__0));

    *param = addr;
    zb_macsplit_transport_send_data_with_type(bufid, ZB_TRANSPORT_CALL_TYPE_HOST_MAC_ADD_INVISIBLE_SHORT);

    TRACE_MSG(TRACE_MAC1, "<<mac_add_invisible_short", (FMT__0));
}
void mac_remove_invisible_short(zb_uint16_t addr)
{
    zb_bufid_t bufid = zb_buf_get_any();
    zb_uint16_t *param;

    ZB_ASSERT(bufid);
    param = ZB_BUF_GET_PARAM(bufid, zb_uint16_t);

    TRACE_MSG(TRACE_MAC1, ">>mac_remove_invisible_short", (FMT__0));

    *param = addr;
    zb_macsplit_transport_send_data_with_type(bufid, ZB_TRANSPORT_CALL_TYPE_HOST_MAC_REMOVE_INVISIBLE_SHORT);

    TRACE_MSG(TRACE_MAC1, "<<mac_remove_invisible_short", (FMT__0));
}

void mac_clear_filters()
{
    zb_bufid_t bufid = zb_buf_get_any();

    TRACE_MSG(TRACE_MAC1, ">>mac_remove_invisible_short", (FMT__0));
    zb_macsplit_transport_send_data_with_type(bufid, ZB_TRANSPORT_CALL_TYPE_HOST_MAC_CLEAR_FILTERS);
    TRACE_MSG(TRACE_MAC1, "<<mac_remove_invisible_short", (FMT__0));
}
#endif  /* ZB_LIMIT_VISIBILITY */

void mac_send_beacon_request(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>mac_send_beacon_request", (FMT__0));
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_HOST_MAC_SEND_BEACON_REQUEST);
    TRACE_MSG(TRACE_MAC1, "<<mac_send_beacon_request", (FMT__0));
}

void mac_send_enhanced_beacon_request(zb_bufid_t param)
{
    TRACE_MSG(TRACE_MAC1, ">>mac_send_enhanced_beacon_request", (FMT__0));
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_HOST_MAC_SEND_ENHANCED_BEACON_REQUEST);
    TRACE_MSG(TRACE_MAC1, "<<mac_send_enhanced_beacon_request", (FMT__0));
}

#ifdef ZB_TH_ENABLED
void mac_sync_address_update(zb_ieee_addr_t ieee_address, zb_uint16_t short_address, zb_bool_t lock)
{
    zb_bufid_t bufid = zb_buf_get_any();
    zb_macsplit_sync_ieee_addr_update_t *param;

    ZB_ASSERT(bufid);
    param = zb_buf_initial_alloc(bufid, sizeof(zb_macsplit_sync_ieee_addr_update_t));

    ZB_IEEE_ADDR_COPY(param->ieee_address, ieee_address);
    param->short_address = short_address;
    param->lock = lock;

    TRACE_MSG(TRACE_MAC1, ">>mac_sync_address_update", (FMT__0));
    zb_macsplit_transport_send_data_with_type(bufid, ZB_TRANSPORT_CALL_TYPE_HOST_MAC_SYNC_ADDRESS_UPDATE);
    TRACE_MSG(TRACE_MAC1, "<<mac_sync_address_update", (FMT__0));
}

/* TH-specific call to allow sending packets without scanning for networks.
 * Summary of buffer content:
 *
 * Type:  channel page | channel number
 * Len:   1 byte       | 1 byte
 */
void mac_th_configure_for_sending(zb_uint8_t ref)
{
    TRACE_MSG(TRACE_MAC1, ">>mac_th_configure_for_sending", (FMT__0));
    zb_macsplit_transport_send_data_with_type(ref, ZB_TRANSPORT_CALL_TYPE_HOST_MAC_CONFIGURE_FOR_SENDING);
    TRACE_MSG(TRACE_MAC1, "<<mac_th_configure_for_sending", (FMT__0));
}
#endif

/* common call handler */

void zb_macsplit_handle_call(zb_bufid_t param, zb_transport_call_type_t call_type)
{
    switch (call_type)
    {
    case ZB_TRANSPORT_CALL_TYPE_DEVICE_BOOT:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_DEVICE_BOOT", (FMT__0));
        zb_macsplit_transport_handle_device_boot_call(param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_DEVICE_BOOT", (FMT__0));
        break;

    case ZB_TRANSPORT_CALL_TYPE_DEVICE_MCPS_DATA_CONFIRM:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_DEVICE_MCPS_DATA_CONFIRM", (FMT__0));
        zb_macsplit_transport_handle_data_confirm_call(param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_DEVICE_MCPS_DATA_CONFIRM", (FMT__0));
        break;

    case ZB_TRANSPORT_CALL_TYPE_DEVICE_MCPS_DATA_INDICATION:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_DEVICE_MCPS_DATA_INDICATION", (FMT__0));
        ZB_TH_PUSH_PACKET(ZB_TH_MCPS_DATA, ZB_TH_PRIMITIVE_INDICATION, param);
        ZB_SCHEDULE_CALLBACK(zb_mcps_data_indication, param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_DEVICE_MCPS_DATA_INDICATION", (FMT__0));
        break;

    case ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_GET_CONFIRM:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_GET_CONFIRM", (FMT__0));
        ZB_TH_PUSH_PACKET(ZB_TH_MLME_GET, ZB_TH_PRIMITIVE_CONFIRM, param);
        zb_macsplit_transport_handle_set_get_confirm_call(param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_GET_CONFIRM", (FMT__0));
        break;

    case ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_SET_CONFIRM:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_SET_CONFIRM", (FMT__0));
        ZB_TH_PUSH_PACKET(ZB_TH_MLME_SET, ZB_TH_PRIMITIVE_CONFIRM, param);
        zb_macsplit_transport_handle_set_get_confirm_call(param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_SET_CONFIRM", (FMT__0));
        break;

    case ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_RESET_CONFIRM:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_RESET_CONFIRM", (FMT__0));
        ZB_TH_PUSH_PACKET(ZB_TH_MLME_RESET, ZB_TH_PRIMITIVE_CONFIRM, param);
        ZB_SCHEDULE_CALLBACK(zb_mlme_reset_confirm, param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_RESET_CONFIRM", (FMT__0));
        break;

#if !defined ZB_COORDINATOR_ONLY
    case ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_POLL_CONFIRM:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_POLL_CONFIRM", (FMT__0));
        ZB_TH_PUSH_PACKET(ZB_TH_MLME_POLL, ZB_TH_PRIMITIVE_CONFIRM, param);
        ZB_SCHEDULE_CALLBACK(zb_mlme_poll_confirm, param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_POLL_CONFIRM", (FMT__0));
        break;

    case ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_ASSOCIATE_CONFIRM:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_ASSOCIATE_CONFIRM", (FMT__0));
        ZB_TH_PUSH_PACKET_TAIL(ZB_TH_MLME_ASSOCIATE, ZB_TH_PRIMITIVE_CONFIRM, param, sizeof(zb_mlme_associate_confirm_t));
        ZB_SCHEDULE_CALLBACK(zb_mlme_associate_confirm, param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_ASSOCIATE_CONFIRM", (FMT__0));
        break;

#endif /* !defined ZB_COORDINATOR_ONLY */
    case ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_SCAN_CONFIRM:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_SCAN_CONFIRM", (FMT__0));
        ZB_TH_PUSH_PACKET(ZB_TH_MLME_SCAN, ZB_TH_PRIMITIVE_CONFIRM, param);
        ZB_SCHEDULE_CALLBACK(zb_mlme_scan_confirm, param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_SCAN_CONFIRM", (FMT__0));
        break;

    case ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_BEACON_NOTIFY_INDICATION:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_BEACON_NOTIFY_INDICATION", (FMT__0));
        ZB_TH_PUSH_PACKET(ZB_TH_MLME_BEACON_NOTIFY, ZB_TH_PRIMITIVE_INDICATION, param);
        ZB_SCHEDULE_CALLBACK(zb_mlme_beacon_notify_indication, param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_BEACON_NOTIFY_INDICATION", (FMT__0));
        break;

    case ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_COMM_STATUS_INDICATION:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_COMM_STATUS_INDICATION", (FMT__0));
        ZB_TH_PUSH_PACKET(ZB_TH_MLME_COMM_STATUS, ZB_TH_PRIMITIVE_INDICATION, param);
        ZB_SCHEDULE_CALLBACK(zb_mlme_comm_status_indication, param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_COMM_STATUS_INDICATION", (FMT__0));
        break;

#if !defined ZB_LITE_NO_ORPHAN_SCAN && defined ZB_ROUTER_ROLE
    case ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_ORPHAN_INDICATION:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_ORPHAN_INDICATION", (FMT__0));
        ZB_TH_PUSH_PACKET(ZB_TH_MLME_ORPHAN, ZB_TH_PRIMITIVE_INDICATION, param);
        ZB_SCHEDULE_CALLBACK(zb_mlme_orphan_indication, param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_ORPHAN_INDICATION", (FMT__0));
        break;
#endif /* !defined ZB_LITE_NO_ORPHAN_SCAN */

    case ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_START_CONFIRM:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_START_CONFIRM", (FMT__0));
        ZB_TH_PUSH_PACKET(ZB_TH_MLME_START, ZB_TH_PRIMITIVE_CONFIRM, param);
        ZB_SCHEDULE_CALLBACK(zb_mlme_start_confirm, param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_START_CONFIRM", (FMT__0));
        break;

#if defined ZB_ROUTER_ROLE
    case ZB_TRANSPORT_CALL_TYPE_DEVICE_MCPS_POLL_INDICATION:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_DEVICE_MCPS_POLL_INDICATION", (FMT__0));
        ZB_TH_PUSH_PACKET(ZB_TH_MLME_POLL, ZB_TH_PRIMITIVE_INDICATION, param);
        ZB_SCHEDULE_CALLBACK(zb_mcps_poll_indication, param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_DEVICE_MCPS_POLL_INDICATION", (FMT__0));
        break;

    case ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_ASSOCIATE_INDICATION:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_ASSOCIATE_INDICATION", (FMT__0));
        ZB_TH_PUSH_PACKET(ZB_TH_MLME_ASSOCIATE, ZB_TH_PRIMITIVE_INDICATION, param);
        ZB_SCHEDULE_CALLBACK(zb_mlme_associate_indication, param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_ASSOCIATE_INDICATION", (FMT__0));
        break;

    case ZB_TRANSPORT_CALL_TYPE_DEVICE_MCPS_PURGE_INDIRECT_QUEUE_CONFIRM:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_DEVICE_MCPS_PURGE_INDIRECT_QUEUE_CONFIRM", (FMT__0));
        ZB_TH_PUSH_PACKET(ZB_TH_MLME_PURGE_INDIRECT_QUEUE, ZB_TH_PRIMITIVE_CONFIRM, param);
        ZB_SCHEDULE_CALLBACK(zb_mcps_purge_indirect_queue_confirm, param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_DEVICE_MCPS_PURGE_INDIRECT_QUEUE_CONFIRM", (FMT__0));
        break;

#if defined ZB_MAC_DUTY_CYCLE_MONITORING
    case ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_DUTY_CYCLE_MODE_INDICATION:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_DUTY_CYCLE_MODE_INDICATION", (FMT__0));
        ZB_TH_PUSH_PACKET(ZB_TH_MLME_DUTY_CYCLE_MODE, ZB_TH_PRIMITIVE_INDICATION, param);
        ZB_SCHEDULE_CALLBACK(zb_mlme_duty_cycle_mode_indication, param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_DUTY_CYCLE_MODE_INDICATION", (FMT__0));
        break;
#endif  /* ZB_MAC_DUTY_CYCLE_MONITORING */

#if 0 /* Uncomment after implementation */
    case ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_GET_POWER_INFO_TABLE_CONFIRM:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_GET_POWER_INFO_TABLE_CONFIRM", (FMT__0));
        ZB_TH_PUSH_PACKET(ZB_TH_MLME_GET_POWER_INFO_TABLE, ZB_TH_PRIMITIVE_CONFIRM, param);
        ZB_SCHEDULE_CALLBACK(zb_mlme_get_power_information_table_confirm, param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_GET_POWER_INFO_TABLE_CONFIRM", (FMT__0));
        break;

    case ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_SET_POWER_INFO_TABLE_CONFIRM:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_SET_POWER_INFO_TABLE_CONFIRM", (FMT__0));
        ZB_TH_PUSH_PACKET(ZB_TH_MLME_SET_POWER_INFO_TABLE, ZB_TH_PRIMITIVE_CONFIRM, param);
        ZB_SCHEDULE_CALLBACK(zb_mlme_set_power_information_table_confirm, param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_DEVICE_MLME_SET_POWER_INFO_TABLE_CONFIRM", (FMT__0));
        break;
#endif

#endif /* defined ZB_ROUTER_ROLE */

#if defined ZB_ENABLE_ZGP_DIRECT || defined ZB_ZGPD_ROLE
    case ZB_TRANSPORT_CALL_TYPE_DEVICE_GP_MCPS_DATA_INDICATION:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_DEVICE_GP_MCPS_DATA_INDICATION", (FMT__0));
        ZB_TH_PUSH_PACKET(ZB_TH_MCPS_GP_DATA, ZB_TH_PRIMITIVE_INDICATION, param);
        ZB_SCHEDULE_CALLBACK(zb_gp_mcps_data_indication, param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_DEVICE_GP_MCPS_DATA_INDICATION", (FMT__0));
        break;
#endif /* defined ZB_ENABLE_ZGP_DIRECT || defined ZB_ZGPD_ROLE */

    case ZB_TRANSPORT_CALL_TYPE_DEVICE_MACSPLIT_GET_VERSION_CONFIRM:
        TRACE_MSG(TRACE_MAC1, ">>ZB_TRANSPORT_CALL_TYPE_DEVICE_MACSPLIT_GET_VERSION_CONFIRM", (FMT__0));
        ZB_SCHEDULE_CALLBACK(zb_macsplit_get_device_version_confirm, param);
        TRACE_MSG(TRACE_MAC1, "<<ZB_TRANSPORT_CALL_TYPE_DEVICE_MACSPLIT_GET_VERSION_CONFIRM", (FMT__0));
        break;

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

#ifdef ZB_MAC_CONFIGURABLE_TX_POWER
void zb_mac_set_tx_power_provider_function(zb_tx_power_provider_t power_provider)
{
    zb_ret_t    ret;
    zb_uint32_t i, j;
    zb_uint8_t  first_channel;
    zb_uint8_t  last_channel;
    zb_int8_t  *out_dbm;

    if (!power_provider)
    {
        TRACE_MSG(TRACE_ERROR, "TX power provider is not specified", (FMT__0));
        return;
    }

    ZB_BZERO(MACSPLIT_CTX().mac_tx_power, sizeof(MACSPLIT_CTX().mac_tx_power));
    MACSPLIT_CTX().tx_power_exist = ZB_TRUE;

    /* Read all TX power from production config */
    for (i = ZB_CHANNEL_LIST_PAGE0_IDX; i <= ZB_CHANNEL_LIST_PAGE31_IDX; ++i)
    {
        switch (i)
        {
        case ZB_CHANNEL_LIST_PAGE0_IDX:
            first_channel = ZB_PAGE0_2_4_GHZ_CHANNEL_FROM;
            last_channel = ZB_PAGE0_2_4_GHZ_CHANNEL_TO;
            break;
        case ZB_CHANNEL_LIST_PAGE28_IDX:
            first_channel = ZB_PAGE28_SUB_GHZ_CHANNEL_FROM;
            last_channel = ZB_PAGE28_SUB_GHZ_CHANNEL_TO;
            break;
        case ZB_CHANNEL_LIST_PAGE29_IDX:
            first_channel = ZB_PAGE29_SUB_GHZ_CHANNEL_FROM;
            last_channel = ZB_PAGE29_SUB_GHZ_CHANNEL_TO + 1; /* plus 62 channel */
            break;
        case ZB_CHANNEL_LIST_PAGE30_IDX:
            first_channel = ZB_PAGE30_SUB_GHZ_CHANNEL_FROM;
            last_channel = ZB_PAGE30_SUB_GHZ_CHANNEL_TO;
            break;
        case ZB_CHANNEL_LIST_PAGE31_IDX:
            first_channel = ZB_PAGE31_SUB_GHZ_CHANNEL_FROM;
            last_channel = ZB_PAGE31_SUB_GHZ_CHANNEL_TO;
            break;
        default:
            break;
        }

        out_dbm = MACSPLIT_CTX().mac_tx_power[i];

        for (j = first_channel; j <= last_channel; ++j)
        {
            if (i == ZB_CHANNEL_LIST_PAGE29_IDX &&
                    j == last_channel)
            {
                /* special case for 62 channel from 29 page */
                j = 62;
            }

            ret = power_provider(ZB_CHANNEL_PAGE_FROM_IDX(i), j, out_dbm);
            if (ret != RET_OK)
            {
                TRACE_MSG(TRACE_ERROR, "Can't get TX power for page %hd, channel %hd", (FMT__H_H, ZB_CHANNEL_PAGE_FROM_IDX(i), j));
                *out_dbm = ZB_INVALID_TX_POWER_VALUE;
            }

            out_dbm++;
        }
    }
}

void zb_mlme_dev_send_mac_tx_power(zb_bufid_t param)
{
    zb_uint8_t *ptr;

    TRACE_MSG(TRACE_MAC1, ">>zb_mlme_dev_send_mac_tx_power: param %hd", (FMT__H, param));

    ptr = zb_buf_initial_alloc(param, sizeof(MACSPLIT_CTX().mac_tx_power));

    if (ptr)
    {
        ZB_MEMCPY(ptr, MACSPLIT_CTX().mac_tx_power, sizeof(MACSPLIT_CTX().mac_tx_power));
        zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_HOST_SEND_MAC_TX_POWER);
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Can't allocate buffer for sending MAC TX power array", (FMT__0));
    }

    TRACE_MSG(TRACE_MAC1, "<<zb_mlme_dev_send_mac_tx_power", (FMT__0));
}
#endif /* ZB_MAC_CONFIGURABLE_TX_POWER */

zb_ret_t zb_macsplit_transport_secure_frame(zb_bufid_t param)
{
    zb_ret_t ret = RET_OK;
    zb_uint_t encr_flags = zb_buf_flags_get(param);
    if ((encr_flags & (ZB_BUF_SECUR_APS_ENCR | ZB_BUF_SECUR_NWK_ENCR))  /*APS & NWK  */
            == (ZB_BUF_SECUR_APS_ENCR | ZB_BUF_SECUR_NWK_ENCR))
    {
        ret = zb_aps_secure_frame(param, 0, SEC_CTX().encryption_buf2, ZB_FALSE);
        TRACE_MSG(TRACE_MAC3, "macsplit check security: APS encryption with status 0x%x", (FMT__H, ret));
        if (ret == RET_OK)
        {
            ret = zb_nwk_secure_frame(SEC_CTX().encryption_buf2, 0, SEC_CTX().encryption_buf);
            TRACE_MSG(TRACE_MAC3, "macsplit check security: NWK encryption with status 0x%x", (FMT__H, ret));
        }
    }
    else if (encr_flags & ZB_BUF_SECUR_APS_ENCR) /* APS only */
    {
        ret = zb_aps_secure_frame(param, 0, SEC_CTX().encryption_buf, ZB_FALSE);
        TRACE_MSG(TRACE_MAC3, "macsplit check security: APS-only encryption with status 0x%x", (FMT__H, ret));
    }
    else if (encr_flags & ZB_BUF_SECUR_NWK_ENCR) /* NWK only */
    {
        ret = zb_nwk_secure_frame(param, 0, SEC_CTX().encryption_buf);
        TRACE_MSG(TRACE_MAC3, "macsplit check security: NWK-only encryption with status 0x%x", (FMT__H, ret));
    }

    return ret;
}

#ifdef ZB_MACSPLIT_HANDLE_DATA_BY_APP
void zb_mac_transport_handle_data_by_app_set(zb_uint8_t flag, zb_uint8_t after_ack)
{
    MACSPLIT_CTX().handle_data_by_app = flag;
    MACSPLIT_CTX().handle_data_by_app_after_last_ack = flag;

    if (after_ack)
    {
        MACSPLIT_CTX().handle_data_by_app = 0;
    }
}

void zb_mac_transport_handle_data_by_app_set_cb(zb_macsplit_handle_data_by_app cb)
{
    MACSPLIT_CTX().handle_data_by_app_cb = cb;
}

int zb_mac_transport_get_fd()
{
    return ZB_IOCTX().uart_fd;
}
#endif

#if defined ZB_MAC_POWER_CONTROL
void zb_mlme_set_power_information_table_request(zb_uint8_t param)
{
    TRACE_MSG(TRACE_MAC1, ">> zb_mlme_set_power_information_table_confirm: param %hd", (FMT__H, param));
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_HOST_MLME_SET_POWER_INFORMATION_TABLE_REQUEST);
    TRACE_MSG(TRACE_MAC1, "<< zb_mlme_set_power_information_table_confirm", (FMT__0));
}
#endif /* defined ZB_MAC_POWER_CONTROL */

#endif /* ZB_MACSPLIT_HOST */
