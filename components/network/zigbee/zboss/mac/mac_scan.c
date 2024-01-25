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
/* PURPOSE: Roitines specific to mlme scan
*/

#define ZB_TRACE_FILE_ID 305
#include "zb_common.h"

#if !defined ZB_ALIEN_MAC && !defined ZB_MACSPLIT_HOST

#include "zb_scheduler.h"
#include "zb_nwk.h"
#include "zb_mac.h"
#include "zb_mac_globals.h"
#include "mac_internal.h"
#include "zb_mac_transport.h"
#include "zb_ie.h"


/*! \addtogroup ZB_MAC */
/*! @{ */

void zb_mac_send_beacon_request_command(zb_uint8_t unused);
void zb_mac_send_enhanced_beacon_request_command(zb_uint8_t param);

/* 7.1.11.1 MLME-SCAN.request */
void zb_mlme_scan_request(zb_uint8_t param)
{
    zb_mac_scan_confirm_t *scan_confirm;
    zb_mlme_scan_params_t *params;
    zb_uint8_t channel_page;

    zb_buf_set_status(param, 0);  /* clearing buffer status in case someone uses it later */
    TRACE_MSG(TRACE_MAC2, ">> zb_mlme_scan_request %hd", (FMT__H, param));
    params = ZB_BUF_GET_PARAM(param, zb_mlme_scan_params_t);
    ZB_ASSERT(params);

#if defined ZB_MAC_API_TRACE_PRIMITIVES
    zb_mac_api_trace_scan_request(param);
#endif

    channel_page = params->channel_page;

    if ((params->scan_duration > ZB_MAX_SCAN_DURATION && params->scan_type != ORPHAN_SCAN)
            || params->channels == 0U
            || MAC_CTX().flags.mlme_scan_in_progress
            || !(params->scan_type == ACTIVE_SCAN
                 || params->scan_type == ED_SCAN
                 || params->scan_type == ENHANCED_ACTIVE_SCAN
#ifdef ZB_OPTIONAL_MAC_FEATURES
                 || params->scan_type == ORPHAN_SCAN
#endif
                ))
    {
        scan_confirm = ZB_BUF_GET_PARAM(param, zb_mac_scan_confirm_t);
        scan_confirm->channel_page = channel_page;
        scan_confirm->scan_type = params->scan_type;
        scan_confirm->status = MAC_INVALID_PARAMETER;
        TRACE_MSG(TRACE_MAC2, "zb_mlme_scan_request invalid parameter or state - call zb_mlme_scan_confirm", (FMT__0));

#if defined ZB_MAC_API_TRACE_PRIMITIVES
        zb_mac_api_trace_scan_confirm(param);
#endif

        ZB_SCHEDULE_CALLBACK(zb_mlme_scan_confirm, param);
    }
    else
    {
        /* remember scan parameters in pending_buf */
        TRACE_MSG(TRACE_MAC2, "scan channels mask 0x%lx", (FMT__L, params->channels));

        MAC_CTX().flags.mlme_scan_in_progress = ZB_TRUE;
        MAC_CTX().unscanned_channels = params->channels;
        MAC_CTX().save_page = MAC_PIB().phy_current_page;
        MAC_CTX().save_channel = MAC_PIB().phy_current_channel;
        MAC_CTX().scan_type = params->scan_type;
        /* Table 2.80   Fields of the Mgmt_NWK_Disc_req Command (the other scans
         requests are using the same parameters
         A value used to calculate the length
         of time to spend scanning each
         channel. The time spent scanning
         each channel is
         (aBaseSuperframeDuration * (2^n +
         1)) symbols, where n is the value of
         the ScanDuration parameter. */
#ifdef ZB_OPTIONAL_MAC_FEATURES
        if (params->scan_type == ORPHAN_SCAN)
        {
            /* So, why is 5 is being written here? According to the
             * specification, the device should perform Orphan Scan within
             * macResponseWaitTime, which is default 32 units of
             * aBaseSuperFrameDurations. The closest power of 2 is 5,
             * i.e. 33 ~= 32:
             * 2^5+1 = 32+1 = 33
             */
            params->scan_duration = 5;
        }
#endif
        MAC_CTX().scan_timeout = (1UL << params->scan_duration) + 1U;

        scan_confirm = ZB_BUF_GET_PARAM(param, zb_mac_scan_confirm_t);
        scan_confirm->status = MAC_SUCCESS;
        scan_confirm->scan_type = MAC_CTX().scan_type;
        scan_confirm->channel_page = channel_page;
        scan_confirm->unscanned_channels = ~MAC_CTX().unscanned_channels & params->channels;
        scan_confirm->result_list_size = 0;

        TRACE_MSG(TRACE_MAC1, "scan_params page %d channels 0x%lx type %hd duration %hd",
                  (FMT__H_L_H_H, channel_page, params->channels,
                   params->scan_type, params->scan_duration));

        ZB_SCHEDULE_ALARM_CANCEL(zb_mlme_scan_step, ZB_ALARM_ANY_PARAM);

        switch (MAC_CTX().scan_type)
        {
        case ACTIVE_SCAN:
            /* To receive beacons set broadcast panid.
               Note that MAC_PIB().mac_pan_id is not changed here.
               Call ZB_UPDATE_PAN_ID() to restore panid.
            */
            TRACE_MSG(TRACE_MAC1, "active scan phy_current_page %hu", (FMT__H, MAC_PIB().phy_current_page));
            MAC_CTX().flags.active_scan_beacon_found = ZB_FALSE;
            ZB_TRANSCEIVER_SET_PAN_ID(ZB_BROADCAST_PAN_ID);
#if defined ZB_MAC_TESTING_MODE
            if (MAC_PIB().mac_auto_request)
            {
                /* Reuse buf, but keep scan_confirm in the buf param.
                   TODO: check, is reuse really needed on such low layer? Isn't it
                   possible to do in in the test application?
                */
                zb_mac_scan_confirm_t scan_confirm_var;
                ZB_MEMCPY(&scan_confirm_var, scan_confirm, sizeof(zb_mac_scan_confirm_t));
                zb_buf_reuse(param);
                scan_confirm = ZB_BUF_GET_PARAM(param, zb_mac_scan_confirm_t);
                ZB_MEMCPY(scan_confirm, &scan_confirm_var, sizeof(zb_mac_scan_confirm_t));

                MAC_CTX().active_scan.buf_param = param;
            }
#endif
            break;
        case ED_SCAN:
            TRACE_MSG(TRACE_MAC1, "ed scan", (FMT__0));
            MAC_CTX().flags.ed_next_pass = ZB_FALSE;
            break;
#ifdef ZB_OPTIONAL_MAC_FEATURES
        case ORPHAN_SCAN:
            TRACE_MSG(TRACE_MAC1, "orphan scan", (FMT__0));
            MAC_CTX().flags.got_realignment = ZB_FALSE;
            break;
#endif
        case ENHANCED_ACTIVE_SCAN:
            TRACE_MSG(TRACE_MAC1, "enhanced active scan", (FMT__0));
            MAC_CTX().flags.active_scan_beacon_found = ZB_FALSE;
            ZB_TRANSCEIVER_SET_PAN_ID(ZB_BROADCAST_PAN_ID);
            break;
        default:
            /* MISRA rule 16.4 - Mandatory default label */
            break;
        }

        if (ZB_SCHEDULE_TX_CB(zb_mlme_scan_step, param) != RET_OK)
        {
            TRACE_MSG(TRACE_ERROR, "Oops! No place for zb_mlme_scan_step in tx queue", (FMT__0));
            scan_confirm->status = MAC_TRANSACTION_OVERFLOW;
            ZB_SCHEDULE_CALLBACK(zb_mlme_scan_confirm, param);
        }
    }

    TRACE_MSG(TRACE_MAC2, "<< zb_mlme_scan_request", (FMT__0));
}

/* this is a universal routine for ed/active/orphan scans */
void zb_mlme_scan_step(zb_uint8_t param)
{
    zb_ret_t ret;
    zb_uint8_t channel_page;
    zb_uint8_t channel_number;
    zb_uint8_t logical_channel;
    zb_uint8_t start_channel_number;
    zb_uint8_t max_channel_number;
    zb_mac_scan_confirm_t *scan_confirm = ZB_BUF_GET_PARAM(param, zb_mac_scan_confirm_t);

    channel_page = scan_confirm->channel_page;

    TRACE_MSG(TRACE_MAC1, ">> zb_mlme_scan_step %hd page %hd", (FMT__H_H, param, channel_page));

    ret = zb_channel_page_get_start_channel_number(channel_page, &start_channel_number);
    ZB_ASSERT(ret == RET_OK);
    ret = zb_channel_page_get_max_channel_number(channel_page, &max_channel_number);
    ZB_ASSERT(ret == RET_OK);

#ifdef ZB_OPTIONAL_MAC_FEATURES
    if (MAC_CTX().scan_type == ORPHAN_SCAN && MAC_CTX().flags.got_realignment)
    {
        /*
        7.5.2.1.4 Orphan channel scan

        The orphan scan shall terminate when the device receives a coordinator realignment command or the
        specified set of logical channels has been scanned.
         */
        channel_number = max_channel_number;
    }
    else
#endif
    {
        channel_number = start_channel_number;
    }

    for (; channel_number <= max_channel_number; channel_number++)
    {
        if (ZB_BIT_IS_SET(MAC_CTX().unscanned_channels, (1UL << channel_number)))
        {
            if (MAC_CTX().scan_type == ED_SCAN
                    && MAC_CTX().flags.ed_next_pass)
            {
                ZB_TRANSCEIVER_GET_ENERGY_LEVEL(&scan_confirm->list.energy_detect[scan_confirm->result_list_size]);
                TRACE_MSG(TRACE_MAC2, "rssi[%hd] %hd", (FMT__H_H, scan_confirm->result_list_size,
                                                        scan_confirm->list.energy_detect[scan_confirm->result_list_size]));
                scan_confirm->result_list_size++;
            }
            MAC_CTX().flags.ed_next_pass = ZB_TRUE;

            ret = zb_channel_page_channel_number_to_logical(channel_page, channel_number, &logical_channel);
            ZB_ASSERT(ret == RET_OK);

            TRACE_MSG(TRACE_MAC2, "set logical channel %hd (page %hd channel number %hd)",
                      (FMT__H_H_H, logical_channel, channel_page, channel_number));

            zb_mac_change_channel(channel_page, logical_channel);
            /* changing channel resets rx_on flag to rx_on_when_idle that might be false */
#if ZB_TRANSCEIVER_ON_BEFORE_ED
            ZB_TRANSCEIVER_SET_RX_ON_OFF(ZB_TRUE);
#endif /* ZB_TRANSCEIVER_ON_BEFORE_ED */

            if (MAC_CTX().scan_type == ED_SCAN)
            {
                ZB_TRANSCEIVER_START_GET_RSSI(MAC_CTX().scan_timeout);
            }
            MAC_CTX().unscanned_channels &= ~(1UL << channel_number);

            /* send request asynchronously (anything must be send asynchronously) */
            if (MAC_CTX().scan_type == ACTIVE_SCAN)
            {
                if (ZB_SCHEDULE_TX_CB(zb_mac_send_beacon_request_command, 0) != RET_OK)
                {
                    TRACE_MSG(TRACE_ERROR, "Oops - out of MAC queue!", (FMT__0));
                }
            }
            else if (MAC_CTX().scan_type == ENHANCED_ACTIVE_SCAN)
            {
                if (ZB_SCHEDULE_TX_CB(zb_mac_send_enhanced_beacon_request_command, param) != RET_OK)
                {
                    TRACE_MSG(TRACE_ERROR, "Oops - out of MAC queue!", (FMT__0));
                }
            }
#ifdef ZB_OPTIONAL_MAC_FEATURES
            else if (MAC_CTX().scan_type == ORPHAN_SCAN)
            {
                if (ZB_SCHEDULE_TX_CB(zb_orphan_notification_command, 0) != RET_OK)
                {
                    TRACE_MSG(TRACE_ERROR, "Oops! No place for zb_orphan_notification_command in tx queue", (FMT__0));
                }
            }
#endif
            else
            {
                /* MISRA rule 15.7 requires empty 'else' branch. */
            }
            ZB_SCHEDULE_ALARM(zb_mlme_scan_step, param, MAC_CTX().scan_timeout);
            break;
        }
    } /* for */

    if (channel_number == (max_channel_number + 1U))
    {
        /* done */

        if (MAC_CTX().scan_type == ED_SCAN)
        {
            /* Last channel */
            ZB_TRANSCEIVER_GET_ENERGY_LEVEL(&scan_confirm->list.energy_detect[scan_confirm->result_list_size]);
            TRACE_MSG(TRACE_MAC2, "rssi[%hd] %hd", (FMT__H_H, scan_confirm->result_list_size,
                                                    scan_confirm->list.energy_detect[scan_confirm->result_list_size]));
            scan_confirm->result_list_size++;
        }

#ifdef ZB_MAC_TESTING_MODE
        if (MAC_CTX().scan_type == ACTIVE_SCAN &&
                MAC_PIB().mac_auto_request &&
                MAC_CTX().active_scan.buf_param != 0)
        {
            zb_bufid_t desc_list_buf = MAC_CTX().active_scan.buf_param;
            zb_uint8_t desc_count = 0;

            desc_count = zb_buf_len(desc_list_buf) / sizeof(zb_pan_descriptor_t);
            scan_confirm->result_list_size = desc_count;

            TRACE_MSG(TRACE_MAC3, "got %hd pan desc", (FMT__H, desc_count));

            MAC_CTX().active_scan.buf_param = 0;
        }
#endif /* ZB_MAC_TESTING_MODE */

        if (MAC_CTX().save_channel != ZB_MAC_INVALID_LOGICAL_CHANNEL)
        {
            TRACE_MSG(TRACE_MAC1, "restoring original channel page %hd channel %hd",
                      (FMT__H_H, MAC_CTX().save_page, MAC_CTX().save_channel));

            zb_mac_change_channel(MAC_CTX().save_page, MAC_CTX().save_channel);
        }
        else
        {
            MAC_CTX().save_page = MAC_PIB().phy_current_page;
            MAC_CTX().save_channel = MAC_PIB().phy_current_channel;
            TRACE_MSG(TRACE_MAC1, "saving page %hd channel %hd",
                      (FMT__H_H, MAC_CTX().save_page, MAC_CTX().save_channel));
        }
        zb_mac_change_channel(MAC_PIB().phy_current_page, MAC_PIB().phy_current_channel);

        TRACE_MSG(TRACE_MAC3, "active_scan_beacon_found %hd", (FMT__H, MAC_CTX().flags.active_scan_beacon_found));

        if ((MAC_CTX().scan_type == ACTIVE_SCAN
                && !MAC_CTX().flags.active_scan_beacon_found)
                || (MAC_CTX().scan_type == ENHANCED_ACTIVE_SCAN
                    && !MAC_CTX().flags.active_scan_beacon_found)
                || (MAC_CTX().scan_type == ORPHAN_SCAN
                    && !MAC_CTX().flags.got_realignment))
        {
            scan_confirm->status = MAC_NO_BEACON;
        }

        /* We set panid to broadcast to be able to receive beacons on
         * UZ. Restore panid (PIB is still keeping right value). */
        if (MAC_CTX().scan_type == ACTIVE_SCAN
                || MAC_CTX().scan_type == ENHANCED_ACTIVE_SCAN)
        {
            ZB_TRANSCEIVER_UPDATE_PAN_ID();
        }

        MAC_CTX().flags.mlme_scan_in_progress = ZB_FALSE;
        TRACE_MSG(TRACE_MAC1, "scan confirm %hd status %hd", (FMT__H_H, param, scan_confirm->status));
#ifdef ZB_MAC_SINGLE_PACKET_IN_FIFO
        /* Set transceiver into normal mode */
        ZB_TRANSCEIVER_SET_RX_ON_OFF(ZB_PIB_RX_ON_WHEN_IDLE());
#endif
#if defined ZB_MAC_API_TRACE_PRIMITIVES
        zb_mac_api_trace_scan_confirm(param);
#endif

        ZB_SCHEDULE_ALARM_CANCEL(zb_mlme_scan_step, ZB_ALARM_ANY_PARAM);
        ZB_SCHEDULE_CALLBACK(zb_mlme_scan_confirm, param);
    }

    TRACE_MSG(TRACE_MAC1, "<< zb_mlme_scan_step", (FMT__0));
}

/*
 * Sends enhanced beacon request (D.9.1 of r22)
 *
 * Buffer content is expected to be of the following format:
 * 0 byte - total HIE bytes count
 * 1 byte - total PIE bytes count
 * HIEs and PIEs start at byte 2
 *
 * Summary of buffer content:
 *
 * Type:  HIEs size | PIEs size | HIEs            | PIEs
 * Len:   1 byte    | 1 byte    | HIEs size bytes | PIEs size bytes
 *
 * @TODO clean up commented blocks after they are relocated
 */
void zb_mac_send_enhanced_beacon_request_command(zb_uint8_t param)
{
    zb_uint8_t mhr_len, hie_len = 0, pie_len = 0;
    zb_uint8_t packet_length;
    zb_uint8_t *ptr;
    zb_uint8_t *buf_payload = zb_buf_begin(param);
    zb_mac_mhr_t mhr;

    /*
     * Frame structure:
     *
     * MHR with HIE-HT1
     * PIE (MLME Nested)
     *   NIE (EB Filter)
     * PIE (Vendor)
     *   Sub-IE (TX Power)
     * PIE (Termination)
     * Command frame ID
     */

    TRACE_MSG(TRACE_MAC2, "+zb_enhanced_beacon_request_command", (FMT__0));

    if (zb_buf_len(param) >= ZB_MLME_SCAN_REQUEST_IE_SIZES_HDR_LEN)
    {
        hie_len = buf_payload[0];
        pie_len = buf_payload[1];
        if ((zb_uint_t)ZB_MLME_SCAN_REQUEST_IE_SIZES_HDR_LEN + hie_len + pie_len > zb_buf_len(param))
        {
            TRACE_MSG(TRACE_MAC2, "invalid PIE and HIE sizes stored in the buffer", (FMT__0));
            ZB_ASSERT(0);
            hie_len = 0;
            pie_len = 0;
        }
    }

    TRACE_MSG(TRACE_MAC2, "pie_len = %d, hie_len = %d", (FMT__D_D, pie_len, hie_len));

    /* Fill Frame Controll then call zb_mac_fill_mhr() */
    /* for EBR: pan_compression=1, ie_present=1, 64 bit source*/
    mhr_len = zb_mac_calculate_mhr_length(ZB_ADDR_64BIT_DEV,
                                          ZB_ADDR_16BIT_DEV_OR_BROADCAST, ZB_TRUE);

    /* MHR, header IEs, payload IEs (incl terminations)*/
    packet_length = mhr_len + hie_len + pie_len
                    + ZB_CALCULATE_TERMINATION_IES_LENGTH(hie_len > 0U, pie_len > 0U, ZB_TRUE);

#if 0
    /* HIE termination, MLME PIE with nested IE + EB filter payload
       vendor PIE + tx power descriptor as payload + PIE termination */
    packet_length += ZB_HIE_HEADER_LENGTH + ZB_PIE_HEADER_LENGTH
                     + ZB_NIE_HEADER_LENGTH + 1 + ZB_PIE_VENDOR_HEADER_LENGTH
                     + ZB_TX_POWER_IE_DESCRIPTOR_LEN + ZB_PIE_HEADER_LENGTH;
#endif

    packet_length += 1U;           /* command id */

    ptr = zb_buf_initial_alloc(MAC_CTX().operation_buf, packet_length);
    ZB_ASSERT(ptr);

    ZB_BZERO(ptr, packet_length);

    /* Settings frame control attributes */
    ZB_BZERO2(mhr.frame_control);
    ZB_FCF_SET_FRAME_TYPE(mhr.frame_control, MAC_FRAME_COMMAND);
    /* security_enabled is 0 */
    /* frame pending is 0 */
    /* ack request is 0 */
    ZB_FCF_SET_PANID_COMPRESSION_BIT(mhr.frame_control, 1U);
    /* reserved bit & sequence number suppression are 0 */
    ZB_FCF_SET_IE_LIST_PRESENT_BIT(mhr.frame_control, 1U);
    ZB_FCF_SET_DST_ADDRESSING_MODE(mhr.frame_control, ZB_ADDR_16BIT_DEV_OR_BROADCAST);
    ZB_FCF_SET_SRC_ADDRESSING_MODE(mhr.frame_control, ZB_ADDR_64BIT_DEV);
    ZB_FCF_SET_FRAME_VERSION(mhr.frame_control, MAC_FRAME_IEEE_802_15_4_2015);

    /* 7.2.1 General MAC frame format */
    mhr.seq_number = ZB_MAC_DSN();
    ZB_INC_MAC_DSN();

    mhr.dst_pan_id = ZB_BROADCAST_PAN_ID;
    mhr.dst_addr.addr_short = ZB_MAC_SHORT_ADDR_NO_VALUE;
    ZB_MEMCPY(&mhr.src_addr, MAC_PIB().mac_extended_address, sizeof(zb_ieee_addr_t));

    zb_mac_fill_mhr(ptr, &mhr);
    ptr += mhr_len;

    /* Write all HIEs */
    if (hie_len > 0U)
    {
        ZB_MEMCPY(ptr, buf_payload + ZB_MLME_SCAN_REQUEST_IE_SIZES_HDR_LEN, hie_len);
        ptr += hie_len;
    }
    else
    {
        /* MISRA rule 15.7 requires empty 'else' branch. */
    }

    ZB_SET_NEXT_HT_IF_REQUIRED(ptr, hie_len > 0U, pie_len > 0U, ZB_TRUE);

#if defined ZB_MAC_POWER_CONTROL
    /* Patch TX power in MAC internals */
    if (pie_len > 0)
    {
        zb_uint8_t *vendor_ptr;
        zb_uint8_t *pie_ptr;
        zb_pie_header_t pie_hdr;
        zb_uint8_t *sub_ie_ptr;
        zb_zigbee_pie_header_t hdr;
        zb_bool_t tx_power_patched = ZB_FALSE;

        pie_ptr = buf_payload + ZB_MLME_SCAN_REQUEST_IE_SIZES_HDR_LEN + hie_len;
        vendor_ptr = pie_ptr;
        while (vendor_ptr - pie_ptr < pie_len &&
                !tx_power_patched)
        {
            ZB_GET_NEXT_PIE_HEADER(vendor_ptr, &pie_hdr);

            if (pie_hdr.group_id == ZB_PIE_GROUP_VENDOR_SPECIFIC &&
                    ZB_IE_CHECK_ZIGBEE_VENDOR(vendor_ptr))
            {
                /* We're inside Zigbee PIE. Find TX power sub-IE descriptor */
                sub_ie_ptr = vendor_ptr + ZB_PIE_VENDOR_OUI_LENGTH;
                while (sub_ie_ptr - vendor_ptr < pie_hdr.length &&
                        !tx_power_patched)
                {
                    ZB_GET_NEXT_ZIGBEE_PIE_HEADER(sub_ie_ptr, &hdr);
                    if (hdr.sub_id == ZB_ZIGBEE_PIE_SUB_ID_TX_POWER)
                    {
                        *(sub_ie_ptr) = MAC_CTX().default_tx_power;
                        tx_power_patched = ZB_TRUE;
                    }

                    sub_ie_ptr += hdr.length;
                }
            }

            vendor_ptr += pie_hdr.length;
        }

        TRACE_MSG(TRACE_MAC1, "TX power patched %hd", (FMT__H, tx_power_patched));
    }
#endif  /* ZB_MAC_POWER_CONTROL */

    /* Write all PIEs */
    if (pie_len > 0U)
    {
        ZB_MEMCPY(ptr, buf_payload + ZB_MLME_SCAN_REQUEST_IE_SIZES_HDR_LEN + hie_len, pie_len);
        ptr += pie_len;
    }
    ZB_SET_NEXT_PT_IF_REQUIRED(ptr, hie_len > 0U, pie_len > 0U, ZB_TRUE);

#if 0
    /* MLME nested PIE */
    ZB_SET_NEXT_PIE_HEADER(ptr, ZB_PIE_GROUP_MLME, ZB_NIE_HEADER_LENGTH + 1);
    ZB_SET_NEXT_SHORT_NIE_HEADER(ptr, ZB_NIE_SUB_ID_EB_FILTER, 1);
    *ptr = 0x01; /* simple EB Filter*/
    ptr += 1;

    /* Vendor PIE */
    ZB_SET_NEXT_PIE_ZIGBEE_VENDOR_HEADER(ptr, ZB_TX_POWER_IE_DESCRIPTOR_LEN);
    ZB_SET_NEXT_TX_POWER_IE_DESCRIPTOR(ptr, 0x7F); /* tmp tx power */

    /* PIE termination */
    ZB_SET_NEXT_PIE_HEADER(ptr, ZB_PIE_GROUP_TERMINATION, 0);
#endif
    *ptr = MAC_CMD_BEACON_REQUEST; /* command number */

    zb_mac_send_frame(MAC_CTX().operation_buf, mhr_len);
    TRACE_MSG(TRACE_MAC2, "<<zb_enhanced_beacon_request_command ret", (FMT__0));
}

/**
  sends beacon request command, mac spec 7.3.7 Beacon request command
*/
void zb_mac_send_beacon_request_command(zb_uint8_t unused)
{
    zb_uint8_t mhr_len;
    zb_uint8_t packet_length;
    zb_uint8_t *ptr;
    zb_mac_mhr_t mhr;

    (void)unused;
    /*
      7.3.7 Beacon request command
      1. Fill MHR fields
      - set dst pan id = 0xffff
      - set dst addr = 0xffff
      2. Fill FCF
      - set frame pending = 0, ack req = 0, security enabled = 0
      - set dst addr mode to ZB_ADDR_16BIT_DEV_OR_BROADCAST
      - set src addr mode to ZB_ADDR_NO_ADDR
      3. Set command frame id = 0x07 (Beacon request)
    */

    TRACE_MSG(TRACE_MAC2, "+zb_beacon_request_command", (FMT__0));

    /* Fill Frame Controll then call zb_mac_fill_mhr() */
    /*
      mac spec  7.2.1.1 Frame Control field
      | Frame Type | Security En | Frame Pending | Ack.Request | PAN ID Compres | Reserv | Dest.Addr.Mode | Frame Ver | Src.Addr.gMode |
    */
    mhr_len = zb_mac_calculate_mhr_length(ZB_ADDR_NO_ADDR, ZB_ADDR_16BIT_DEV_OR_BROADCAST, ZB_FALSE);
    packet_length = mhr_len;
    packet_length += 1U;           /* command id */

    ptr = zb_buf_initial_alloc(MAC_CTX().operation_buf, packet_length);
    ZB_ASSERT(ptr);

    ZB_BZERO(ptr, packet_length);

    ZB_BZERO2(mhr.frame_control);
    ZB_FCF_SET_FRAME_TYPE(mhr.frame_control, MAC_FRAME_COMMAND);
    /* security_enabled is 0 */
    /* frame pending is 0 */
    /* ack request is 0 */
    /* PAN id compression is 0 */
    ZB_FCF_SET_DST_ADDRESSING_MODE(mhr.frame_control, ZB_ADDR_16BIT_DEV_OR_BROADCAST);

    ZB_FCF_SET_FRAME_VERSION(mhr.frame_control, MAC_FRAME_VERSION);
    ZB_FCF_SET_SRC_ADDRESSING_MODE(mhr.frame_control, ZB_ADDR_NO_ADDR);

    /* 7.2.1 General MAC frame format */
    mhr.seq_number = ZB_MAC_DSN();
    ZB_INC_MAC_DSN();

    mhr.dst_pan_id = ZB_BROADCAST_PAN_ID;
    mhr.dst_addr.addr_short = ZB_MAC_SHORT_ADDR_NO_VALUE;
    /* src pan id and src addr are ignored */

    zb_mac_fill_mhr(ptr, &mhr);

    *(ptr + mhr_len) = MAC_CMD_BEACON_REQUEST;

    zb_mac_send_frame(MAC_CTX().operation_buf, mhr_len);
    TRACE_MSG(TRACE_MAC2, "<< zb_beacon_request_command ret", (FMT__0));
}


#ifdef ZB_OPTIONAL_MAC_FEATURES
/*
  7.3.6 sends orphan notification command
  return RET_OK, RET_ERROR
*/
void zb_orphan_notification_command(zb_uint8_t unused)
{
    zb_uint8_t mhr_len;
    zb_uint8_t *ptr = NULL;
    zb_mac_mhr_t mhr;

    /*
      Orphan notification command
      1. Fill MHR fields
      - set dst pan id = 0xffff
      - set dst addr = 0xffff
      2. Fill FCF
      - set frame pending = 0, ack req = 0, security enabled = 0
      - set dst addr mode to ZB_ADDR_16BIT_DEV_OR_BROADCAST
      - set src addr mode to ZB_ADDR_64BIT_DEV
      3. Set command frame id = 0x07 (Beacon request)
    */

    ZVUNUSED(unused);
    TRACE_MSG(TRACE_MAC2, ">>orphan_notif_cmd", (FMT__0));

    mhr_len = zb_mac_calculate_mhr_length(ZB_ADDR_64BIT_DEV, ZB_ADDR_16BIT_DEV_OR_BROADCAST, ZB_TRUE);
    {
        zb_uint8_t packet_length = mhr_len + 1;
        ptr = (zb_uint8_t *)zb_buf_initial_alloc(MAC_CTX().operation_buf, packet_length);
        ZB_ASSERT(ptr);
        ZB_BZERO(ptr, packet_length);
    }

    ZB_BZERO2(mhr.frame_control);
    ZB_FCF_SET_FRAME_TYPE(mhr.frame_control, MAC_FRAME_COMMAND);
    ZB_FCF_SET_DST_ADDRESSING_MODE(mhr.frame_control, ZB_ADDR_16BIT_DEV_OR_BROADCAST);
    ZB_FCF_SET_SRC_ADDRESSING_MODE(mhr.frame_control, ZB_ADDR_64BIT_DEV);
    ZB_FCF_SET_PANID_COMPRESSION_BIT(mhr.frame_control, 1U);
    ZB_FCF_SET_FRAME_VERSION(mhr.frame_control, MAC_FRAME_VERSION);

    /* 7.2.1 General MAC frame format */
    mhr.seq_number = ZB_MAC_DSN();
    ZB_INC_MAC_DSN();

    mhr.dst_pan_id = ZB_BROADCAST_PAN_ID;
    mhr.dst_addr.addr_short = ZB_MAC_SHORT_ADDR_NO_VALUE;
    ZB_IEEE_ADDR_COPY(mhr.src_addr.addr_long, ZB_PIB_EXTENDED_ADDRESS());

    zb_mac_fill_mhr(ptr, &mhr);
    *(ptr + mhr_len) = MAC_CMD_ORPHAN_NOTIFICATION;

    ZB_TRANS_SEND_FRAME(mhr_len, MAC_CTX().operation_buf, ZB_MAC_TX_WAIT_CSMACA);

    TRACE_MSG(TRACE_MAC2, "<<orphan_notif_cmd", (FMT__0));
}
#endif  /* ZB_OPTIONAL_MAC_FEATURES */


#ifdef ZB_MAC_TESTING_MODE
void zb_mac_store_pan_desc(zb_bufid_t beacon_buf, zb_pan_descriptor_t *pan_desc)
{
    zb_pan_descriptor_t *pan_desc_buf;
    zb_bufid_t desc_list_buf;
    zb_uint8_t desc_count;

    TRACE_MSG(TRACE_NWK1, ">>store_pan_desc %p", (FMT__P, beacon_buf));

    ZB_ASSERT(MAC_CTX().active_scan.buf_param != 0);
    desc_list_buf = MAC_CTX().active_scan.buf_param;


    /* do not calculate pan descriptors number - it can be calculated using buffer length  */
    /* in this check take into account size of scan confirm structure - descriptors will follow it */
    desc_count = zb_buf_len(desc_list_buf) / sizeof(zb_pan_descriptor_t);

    /* ZB_BUF_GET_FREE_SIZE(desc_list_buf) */
    if (((zb_buf_get_max_size(desc_list_buf) - zb_buf_len(desc_list_buf)) >= (sizeof(zb_pan_descriptor_t) + sizeof(zb_mac_scan_confirm_t))) &&
            desc_count < ZB_ACTIVE_SCAN_MAX_PAN_DESC_COUNT)
    {
        pan_desc_buf = zb_buf_alloc_right(desc_list_buf, sizeof(zb_pan_descriptor_t));
        ZB_MEMCPY(pan_desc_buf, pan_desc, sizeof(zb_pan_descriptor_t));
    }
    else
    {
        TRACE_MSG(TRACE_NWK3, "stop scan, no free space", (FMT__0));
        MAC_CTX().active_scan.stop_scan = 1;
    }

    TRACE_MSG(TRACE_NWK1, "<<store_pan_desc", (FMT__0));
}
#endif /* ZB_MAC_TESTING_MODE */


/*! @} */

#endif  /* !ZB_ALIEN_MAC && !ZB_MACSPLIT_HOST */
