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
/* PURPOSE: Stub APS
*/

#define ZB_TRACE_FILE_ID 2141
#include "zb_common.h"
#include "zb_types.h"
#include "zb_aps_interpan.h"
#include "zb_debug.h"
#include "zb_aps.h"
#include "zdo_wwah_stubs.h"
#if defined ZB_ENABLE_INTER_PAN_EXCHANGE

static void zb_intrp_alloc_and_fill_hdr(zb_uint8_t param);

void zb_intrp_data_confirm(zb_uint8_t param);

void zb_intrp_data_indication(zb_uint8_t param);

void zb_intrp_data_request(zb_uint8_t param)
{
    zb_intrp_data_req_t *incoming_req = ZB_BUF_GET_PARAM(param, zb_intrp_data_req_t);
    zb_mcps_data_req_params_t outgoing_req;

    TRACE_MSG(TRACE_APS1, "> zb_intrp_data_request param %hd", (FMT__H, param));

#if defined ZB_ENABLE_ZLL
#ifndef ZB_BDB_TOUCHLINK
    ZLL_TRAN_CTX().send_confirmed = ZB_FALSE;
#endif
#endif

    zb_intrp_alloc_and_fill_hdr(param);
    ZB_BZERO(&outgoing_req, sizeof(outgoing_req));

    outgoing_req.src_addr_mode = ZB_ADDR_64BIT_DEV;
    /* Seems like MAC fills in SrcPANId on its own */
    ZB_IEEE_ADDR_COPY(outgoing_req.src_addr.addr_long, ZB_PIBCACHE_EXTENDED_ADDRESS());
    outgoing_req.dst_addr_mode = (  (incoming_req->dst_addr_mode == ZB_INTRP_ADDR_IEEE)
                                    ? ZB_ADDR_64BIT_DEV
                                    : ZB_ADDR_16BIT_DEV_OR_BROADCAST);
    outgoing_req.dst_pan_id = incoming_req->dst_pan_id;
    switch (incoming_req->dst_addr_mode)
    {
    case ZB_INTRP_ADDR_IEEE:
        TRACE_MSG(TRACE_APS1, "to " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(incoming_req->dst_addr.addr_long)));
        ZB_IEEE_ADDR_COPY(outgoing_req.dst_addr.addr_long, incoming_req->dst_addr.addr_long);
        break;
    case ZB_INTRP_ADDR_GROUP:
        TRACE_MSG(TRACE_APS1, "to broadcast", (FMT__0));
        outgoing_req.dst_addr.addr_short = ZB_INTRP_BROADCAST_SHORT_ADDR;
        break;
    case ZB_INTRP_ADDR_NETWORK:
        TRACE_MSG(TRACE_APS1, "to short 0x%x", (FMT__D, incoming_req->dst_addr.addr_short));
        outgoing_req.dst_addr.addr_short = incoming_req->dst_addr.addr_short;
        break;
    default:
        TRACE_MSG(
            TRACE_ERROR,
            "Invalid INTRP-DATA.request.DstAddrMode value %hd",
            (FMT__H, incoming_req->dst_addr_mode));
        zb_buf_set_status(param, ERROR_CODE(ERROR_CATEGORY_MAC, MAC_INVALID_PARAMETER));
        ZB_SCHEDULE_CALLBACK(zb_intrp_data_frame_confirm, param);
        goto zb_intrp_data_request_finals;
        break;
    }
    outgoing_req.msdu_handle = incoming_req->asdu_handle;
    outgoing_req.tx_options = (incoming_req->dst_addr_mode == ZB_INTRP_ADDR_IEEE);
    zb_buf_flags_clr_encr(param);
    ZB_MEMCPY(
        ZB_BUF_GET_PARAM(param, zb_mcps_data_req_params_t),
        &outgoing_req,
        sizeof(zb_mcps_data_req_params_t));

    ZB_SCHEDULE_CALLBACK(zb_mcps_data_request, param);

zb_intrp_data_request_finals:
    TRACE_MSG(TRACE_APS1, "< zb_intrp_data_request", (FMT__0));
}/* void zb_intrp_data_request(zb_uint8_t param) */

#define ZB_INTRP_PUT_16BIT(dst, src) \
  ZB_HTOLE16((dst), (src));             \
  (dst) += sizeof(zb_uint16_t);

static void zb_intrp_alloc_and_fill_hdr(zb_uint8_t param)
{
    zb_intrp_data_req_t *request =
        ZB_BUF_GET_PARAM(param, zb_intrp_data_req_t);
    zb_uint8_t hdr_size = ZB_INTRP_HEADER_SIZE(request->dst_addr_mode == ZB_INTRP_ADDR_GROUP);
    zb_uint8_t *data_ptr;

    TRACE_MSG(TRACE_APS1, "> zb_interpan_alloc_and_fill_hdr param %hd %hd", (FMT__H_H, param, hdr_size));

    /* Alloc place for Stub-APS header */
    data_ptr = zb_buf_alloc_left(param, hdr_size);
    ZB_BZERO(data_ptr, hdr_size);
    /* Stub-APS frame format:
     * -----------------------------------------------------------
     * | NWK FCF | APS FCF | Group addr | Cluster id | Profile id |
     * ------------------------------------------------------------
     * |   2     |    1    |   0/2      |      2     |    2       |
     * ------------------------------------------------------------
     */
    ZB_PUT_NEXT_HTOLE16(data_ptr, ZB_NWK_INTERPAN_FCF_VALUE);
    ZB_APS_FC_SET_FRAME_TYPE(*data_ptr, ZB_APS_FRAME_INTER_PAN);
    if (request->dst_addr_mode == ZB_INTRP_ADDR_NETWORK)
    {
        ZB_APS_FC_SET_DELIVERY_MODE(*data_ptr, ZB_APS_DELIVERY_BROADCAST);
    }
    else if (request->dst_addr_mode == ZB_INTRP_ADDR_GROUP)
    {
        ZB_APS_FC_SET_DELIVERY_MODE(*data_ptr, ZB_APS_DELIVERY_GROUP);
    }
    /* No need to call ZB_APS_FC_SET_DELIVERY_MODE for ZB_APS_DELIVERY_UNICAST, because its value is 0 - just
     * increment data pointer */
    ++data_ptr;
    if (request->dst_addr_mode == ZB_INTRP_ADDR_GROUP)
    {
        ZB_INTRP_PUT_16BIT(data_ptr, &(request->dst_addr));
    }
    ZB_INTRP_PUT_16BIT(data_ptr, &(request->cluster_id));
    ZB_INTRP_PUT_16BIT(data_ptr, &(request->profile_id));

    TRACE_MSG(TRACE_APS1, "< zb_interpan_alloc_and_fill_hdr", (FMT__0));
}/* static void zb_interpan_alloc_and_fill_hdr(zb_uint8_t param) */


void zb_intrp_data_frame_indication(zb_uint8_t param, zb_mac_mhr_t *mac_hdr, zb_uint8_t lqi, zb_uint8_t rssi)
{
    zb_intrp_data_ind_t *indication = ZB_BUF_GET_PARAM(param, zb_intrp_data_ind_t);
    zb_intrp_hdr_t intrp_hdr;
    zb_uint8_t *data_ptr = zb_buf_begin(param);
    zb_uint8_t header_size;

    TRACE_MSG(TRACE_APS1, "> zb_intrp_data_frame_indication param %hd rssi %hd lqi %hd", (FMT__H_H_H, param, rssi, lqi));

    ZB_BZERO(indication, sizeof(*indication));
    indication->link_quality = lqi;
    indication->rssi = rssi;

    /* Frame type and protocol version already checked */

    /* Parse Stub-APS header */
    if (zb_buf_len(param) < (sizeof(zb_uint16_t) + sizeof(zb_uint8_t)))
    {
        TRACE_MSG(TRACE_ERROR, "Packet doesn't contain APS FCF, dropping", (FMT__0));
        goto zb_intrp_data_frame_indication_failure;
    }
    header_size =
        ZB_INTRP_HEADER_SIZE(
            ZB_APS_FC_GET_DELIVERY_MODE(*(data_ptr + sizeof(zb_uint16_t))) == ZB_APS_DELIVERY_GROUP);
    if (zb_buf_len(param) < header_size)
    {
        TRACE_MSG(TRACE_ERROR, "Packet doesn't contain valid inter-PAN packet, dropping", (FMT__0));
        goto zb_intrp_data_frame_indication_failure;
    }
    zb_parse_intrp_hdr(&intrp_hdr, param);

    /* Check group membership */
    if (ZB_APS_FC_GET_DELIVERY_MODE(intrp_hdr.aps_fcf) == ZB_APS_DELIVERY_GROUP)
    {
        if (    ! ZG->aps.group.n_groups
                ||  (intrp_hdr.group_addr != ZB_INTRP_BROADCAST_SHORT_ADDR && ! zb_aps_is_in_group(intrp_hdr.group_addr)))
        {
            TRACE_MSG(
                TRACE_ERROR,
                "Our device is not in group 0x%04x, dropping packet",
                (FMT__D, intrp_hdr.group_addr));
            goto zb_intrp_data_frame_indication_failure;
        }
    }

    /* Fill in INTRP-DATA.indication parameters */
    switch (ZB_APS_FC_GET_DELIVERY_MODE(intrp_hdr.aps_fcf))
    {
    case ZB_APS_DELIVERY_UNICAST:
        indication->dst_addr_mode = ZB_INTRP_ADDR_IEEE;
        ZB_IEEE_ADDR_COPY(indication->dst_addr.addr_long, mac_hdr->dst_addr.addr_long);
        break;
    case ZB_APS_DELIVERY_BROADCAST:
        indication->dst_addr_mode = ZB_INTRP_ADDR_NETWORK;
        indication->dst_addr.addr_short = mac_hdr->dst_addr.addr_short;
        break;
    case ZB_APS_DELIVERY_GROUP:
        indication->dst_addr_mode = ZB_INTRP_ADDR_GROUP;
        indication->dst_addr.addr_short = intrp_hdr.group_addr;
        break;
    default:
        TRACE_MSG(TRACE_ERROR, "Invalid APS delivery mode ZB_APS_DELIVERY_RESERVED", (FMT__0));
        goto zb_intrp_data_frame_indication_failure;
        break;
    }

    /* According to WWAH ZCL Cluster Definition "Disable Touchlink Interpan Message Support" Command
     * A device SHALL ignore Touchlink interpan messages.
     *
     * C-26
     * Device MUST disable support for all Zigbee non-Green Power Inter-pan messages (eg. Touchlink) when
     * instructed to do so by the TC (using TCLK). */

    if (!ZB_ZDO_CHECK_IF_INTERPAN_SUPPORTED() && intrp_hdr.profile_id == ZB_AF_ZLL_PROFILE_ID)
    {
        TRACE_MSG(TRACE_ERROR, "Processing Interpan Messages blocked by WWAH", (FMT__0));
        goto zb_intrp_data_frame_indication_failure;
    }

    indication->dst_pan_id = mac_hdr->dst_pan_id;
    indication->src_pan_id = mac_hdr->src_pan_id;
    ZB_IEEE_ADDR_COPY(indication->src_addr, mac_hdr->src_addr.addr_long);
    ZB_MEMCPY(&(indication->profile_id), &(intrp_hdr.profile_id), sizeof(intrp_hdr.profile_id));
    ZB_MEMCPY(&(indication->cluster_id), &(intrp_hdr.cluster_id), sizeof(intrp_hdr.cluster_id));

    ZB_SCHEDULE_CALLBACK(zb_intrp_data_indication, param);
    goto zb_intrp_data_indication_finals;

zb_intrp_data_frame_indication_failure:
    zb_buf_free(param);
zb_intrp_data_indication_finals:
    TRACE_MSG(TRACE_APS1, "< zb_intrp_data_frame_indication", (FMT__0));
}/* zb_intrp_data_frame_indication */

void zb_intrp_data_frame_confirm(zb_uint8_t param)
{
    zb_uint8_t *data_ptr = zb_buf_begin(param);

    TRACE_MSG(TRACE_APS1, "> zb_intrp_data_frame_confirm param %hd", (FMT__H, param));

    /* Cut  INTRP-DATA header. MAC header is already cut by MAC. */
    (void)zb_buf_cut_left(
        param,
        ZB_INTRP_HEADER_SIZE(
            ZB_APS_FC_GET_DELIVERY_MODE(*(data_ptr + sizeof(zb_uint16_t))) == ZB_APS_DELIVERY_GROUP
        )
    );
    ZB_SCHEDULE_CALLBACK(zb_intrp_data_confirm, param);

    TRACE_MSG(TRACE_APS1, "< zb_intrp_data_frame_confirm", (FMT__0));
}/* void zb_intrp_data_frame_confirm(zb_uint8_t param) */

#define ZB_INTRP_EXTRACT_16BIT(dst_ptr, src_ptr) \
  {                                              \
    ZB_LETOH16((dst_ptr), (src_ptr));            \
    (src_ptr) += sizeof(zb_uint16_t);            \
  }

void zb_parse_intrp_hdr(zb_intrp_hdr_t *header, zb_bufid_t  buffer)
{
    zb_uint8_t *data_ptr = zb_buf_begin(buffer);

    TRACE_MSG(TRACE_APS1, "> zb_parse_intrp_hdr header %p buffer %p", (FMT__P_P, header, buffer));

    ZB_INTRP_EXTRACT_16BIT(&(header->nwk_fcf), data_ptr);
    header->aps_fcf = *(data_ptr++);
    if (ZB_APS_FC_GET_DELIVERY_MODE(header->aps_fcf) == ZB_APS_DELIVERY_GROUP)
    {
        ZB_INTRP_EXTRACT_16BIT(&(header->group_addr), data_ptr);
    }
    ZB_INTRP_EXTRACT_16BIT(&(header->cluster_id), data_ptr);
    ZB_INTRP_EXTRACT_16BIT(&(header->profile_id), data_ptr);

    TRACE_MSG(TRACE_APS1, "< zb_parse_intrp_hdr", (FMT__0));
}/* static zb_parse_intrp_hdr(zb_intrp_hdr_t* header, zb_bufid_t  buffer) */

#endif /* defined ZB_ENABLE_INTER_PAN_EXCHANGE */
