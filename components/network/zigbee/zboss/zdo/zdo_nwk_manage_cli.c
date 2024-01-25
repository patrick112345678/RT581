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
/* PURPOSE: ZDO network management functions, client side
*/

#define ZB_TRACE_FILE_ID 2099
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_hash.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zdo_common.h"
#include "zb_ncp.h"


/*! \addtogroup ZB_ZDO */
/*! @{ */

#ifdef ZB_JOIN_CLIENT
static zb_uint8_t zdo_mgmt_leave_cli(zb_uint8_t param, zb_callback_t cb);
#endif

void zdo_mgmt_permit_joining_req_cli(zb_uint8_t param, zb_callback_t cb);

zb_uint8_t zb_zdo_mgmt_nwk_update_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_zdo_mgmt_nwk_update_req_t *req_param;
    zb_zdo_mgmt_nwk_update_req_hdr_t *req;
    zb_uint8_t *payload;
    zb_uint8_t zdo_tsn;

    TRACE_MSG(TRACE_ZDO2, ">> zb_zdo_mgmt_nwk_update_req param %hd", (FMT__D, param));
    req_param = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_nwk_update_req_t);

    req = zb_buf_initial_alloc(param, sizeof(zb_zdo_mgmt_nwk_update_req_hdr_t));
    ZB_HTOLE32((zb_uint8_t *)&req->scan_channels, (zb_uint8_t *)&req_param->hdr.scan_channels);
    req->scan_duration = req_param->hdr.scan_duration;

    if (req->scan_duration == ZB_ZDO_NEW_ACTIVE_CHANNEL ||
            req->scan_duration == ZB_ZDO_NEW_CHANNEL_MASK)
    {
        payload = zb_buf_alloc_right(param, sizeof(zb_uint8_t));

        TRACE_MSG(TRACE_ZDO2, "new act channel/mask", (FMT__0));
        *payload = ZB_NIB_UPDATE_ID();
        zb_buf_flags_clr(param, ZB_BUF_ZDO_CMD_NO_RESP);
        if (req->scan_duration == ZB_ZDO_NEW_CHANNEL_MASK)
        {
            TRACE_MSG(TRACE_ZDO2, "new mask", (FMT__0));
            payload = zb_buf_alloc_right(param, sizeof(zb_uint16_t));
            ZB_HTOLE16(payload, (zb_uint8_t *)&req_param->manager_addr);
        }
    }
#ifndef ZB_LITE_NO_FULL_FUNCLIONAL_MGMT_NWK_UPDATE
    else if (req->scan_duration <= ZB_ZDO_MAX_SCAN_DURATION)
    {
        TRACE_MSG(TRACE_ZDO3, "ed scan req", (FMT__0));

        payload = zb_buf_alloc_right(param, sizeof(zb_uint8_t));
        *payload = req_param->scan_count;
    }
#endif
#ifdef ZB_CERTIFICATION_HACKS
    else if (ZB_CERT_HACKS().zdo_mgmt_nwk_update_force_scan_count)
    {
        TRACE_MSG(TRACE_ZDO3, "zdo_mgmt_nwk_update_force_scan_count cert hack", (FMT__0));

        payload = zb_buf_alloc_right(param, sizeof(zb_uint8_t));
        *payload = req_param->scan_count;
    }
#endif
#if !defined(ZB_LITE_NO_FULL_FUNCLIONAL_MGMT_NWK_UPDATE) || defined(ZB_CERTIFICATION_HACKS)
    else
    {
        TRACE_MSG(TRACE_ERROR, "Unexpected scan_duration %hd", (FMT__H, req->scan_duration));
        return ZB_ZDO_INVALID_TSN;
    }
#endif

    zdo_tsn = zdo_send_req_by_short(ZDO_MGMT_NWK_UPDATE_REQ_CLID, param, cb,
                                    req_param->dst_addr, ZB_ZDO_CB_DEFAULT_COUNTER);

    TRACE_MSG(TRACE_ZDO2, "<< zb_zdo_mgmt_nwk_update_req", (FMT__0));
    return zdo_tsn;
}


#ifdef ZB_MGMT_NWK_ENHANCED_UPDATE_ENABLED

zb_uint8_t zb_zdo_mgmt_nwk_enh_update_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_zdo_mgmt_nwk_enhanced_update_req_param_t *req_param_ptr;
    zb_zdo_mgmt_nwk_enhanced_update_req_param_t req_param;
    zb_zdo_mgmt_nwk_enhanced_update_req_hdr_t *hdr;
    zb_uint8_t zdo_tsn = ZB_ZDO_INVALID_TSN;
    zb_uint8_t *payload;
    zb_uindex_t i;

    TRACE_MSG(TRACE_ZDO1, ">> zb_zdo_mgmt_nwk_enh_update_req param %hd",
              (FMT__H, param));

    req_param_ptr = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_nwk_enhanced_update_req_param_t);
    ZB_MEMCPY(&req_param, req_param_ptr, sizeof(zb_zdo_mgmt_nwk_enhanced_update_req_param_t));

    hdr = zb_buf_initial_alloc(param, sizeof(zb_zdo_mgmt_nwk_enhanced_update_req_hdr_t));

    /* Find channel pages to be copied,
     * convert from ZBOSS internal representation to ZB spec */
    hdr->channel_page_count = 0;
    for (i = 0; i < ZB_CHANNEL_PAGES_NUM; i++)
    {
        zb_channel_page_t *channel_page = &req_param.channel_list[i];

        if (ZB_CHANNEL_PAGE_IS_SUB_GHZ(*channel_page) &&
                !ZB_CHANNEL_PAGE_IS_MASK_EMPTY(*channel_page))
        {
            payload = zb_buf_alloc_right(param, sizeof(zb_channel_page_t));
            ZB_HTOLE32(payload, channel_page);
            hdr->channel_page_count++;
        }
    }

    TRACE_MSG(TRACE_ZDO3, "Channel page count %hd",
              (FMT__H, hdr->channel_page_count));

    /* Send the frame only if we have smth to send */
    if (hdr->channel_page_count != 0U)
    {
        payload = zb_buf_alloc_right(param, sizeof(zb_uint8_t));

        /* Scan Duration */
        *payload = req_param.scan_duration;

        if (req_param.scan_duration <= ZB_ZDO_MAX_SCAN_DURATION)
        {
            /* Scan count */
            payload = zb_buf_alloc_right(param, sizeof(zb_uint8_t));
            *payload = req_param.scan_count;
        }
#ifdef ZB_CERTIFICATION_HACKS
        else if (ZB_CERT_HACKS().zdo_mgmt_nwk_update_force_scan_count)
        {
            TRACE_MSG(TRACE_ZDO3, "zdo_mgmt_nwk_update_force_scan_count cert hack", (FMT__0));

            payload = zb_buf_alloc_right(param, sizeof(zb_uint8_t));
            *payload = req_param.scan_count;
        }
#endif
        else if (req_param.scan_duration == ZB_ZDO_NEW_ACTIVE_CHANNEL)
        {
            /* nwkUpdateID */
            payload = zb_buf_alloc_right(param, sizeof(zb_uint8_t));
            *payload = ZB_NIB_UPDATE_ID();
        }
        else if (req_param.scan_duration == ZB_ZDO_NEW_CHANNEL_MASK)
        {
            /* nwkUpdateID + nwkManagerAddr */
            payload = zb_buf_alloc_right(param, sizeof(zb_uint8_t) + sizeof(zb_uint16_t));
            *payload = ZB_NIB_UPDATE_ID();
            payload += sizeof(zb_uint8_t);
            ZB_HTOLE16(payload, &req_param.manager_addr);
        }
        else
        {
            /* Unexpected, but send frame as is */
            TRACE_MSG(TRACE_ERROR, "Unexpected scan_duration %hd", (FMT__H, param));
        }

        zdo_tsn = zdo_send_req_by_short(ZDO_MGMT_NWK_ENHANCED_UPDATE_REQ_CLID, param, cb,
                                        req_param.dst_addr, ZB_ZDO_CB_DEFAULT_COUNTER);
    }

    TRACE_MSG(TRACE_ZDO1, "<< zb_zdo_mgmt_nwk_enh_update_req tsn %hd",
              (FMT__H, zdo_tsn));

    return zdo_tsn;
}

#ifdef ZB_DEPRECATED_API

zb_uint8_t zb_zdo_mgmt_nwk_enhanced_update_req(zb_uint8_t param, zb_callback_t cb)
{
    return zb_zdo_mgmt_nwk_enh_update_req(param, cb);
}

#endif /* ZB_DEPRECATED_API */

#endif /* ZB_MGMT_NWK_ENHANCED_UPDATE_ENABLED */


#ifndef ZB_LITE_NO_ZDO_SYSTEM_SERVER_DISCOVERY
zb_uint8_t zb_zdo_system_server_discovery_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_zdo_system_server_discovery_req_t *req;
    zb_zdo_system_server_discovery_param_t *req_param;

    req_param = ZB_BUF_GET_PARAM(param, zb_zdo_system_server_discovery_param_t);
    req = zb_buf_initial_alloc(param, sizeof(zb_zdo_system_server_discovery_param_t));

    ZB_HTOLE16((zb_uint8_t *)&req->server_mask, (zb_uint8_t *)&req_param->server_mask);

    TRACE_MSG(TRACE_ZDO3, "zb_zdo_system_server_discovery_req server_mask %x", (FMT__D, req->server_mask));
    TRACE_MSG(TRACE_ATM1, "Z< zb_zdo_system_server_discovery_req server_mask 0x%x", (FMT__D, req->server_mask));

    /* This request is handled with slightly different approach comapring to other ZDO requests:
     * the CB and TSN are stored in the particular variable and they are analysed separately.
     * Possibly, it is done because multiple responses are expected after broadcast request is sent.
     * TODO: handle it as all other ZDO requests (use internal ZDO context)
     */
    ZDO_CTX().system_server_discovery_cb = cb;
    ZDO_CTX().system_server_discovery_tsn =
        zdo_send_req_by_short(ZDO_SYSTEM_SERVER_DISCOVERY_REQ_CLID, param, NULL,
                              ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE, ZB_ZDO_CB_UNICAST_COUNTER);
    return ZDO_CTX().system_server_discovery_tsn;
}
#endif


zb_uint8_t zb_zdo_mgmt_lqi_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_zdo_mgmt_lqi_req_t *req;
    zb_zdo_mgmt_lqi_param_t *req_param;

    TRACE_MSG(TRACE_ZDO3, "zb_zdo_mgmt_lqi_req param %hd", (FMT__D, param));
    req_param = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_lqi_param_t);

    /* MA: Wrong struct for sizeof: zb_zdo_mgmt_nwk_update_req_hdr_t
     * update to zb_zdo_mgmt_lqi_req_t */
    req = zb_buf_initial_alloc(param, sizeof(zb_zdo_mgmt_lqi_req_t));
    req->start_index = req_param->start_index;

    return zdo_send_req_by_short(ZDO_MGMT_LQI_REQ_CLID, param, cb, req_param->dst_addr, ZB_ZDO_CB_DEFAULT_COUNTER);
}

/* GP add, start */
zb_uint8_t zb_zdo_mgmt_rtg_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_zdo_mgmt_rtg_req_t *req;
    zb_zdo_mgmt_rtg_param_t *req_param;

    TRACE_MSG(TRACE_ZDO3, "zb_zdo_mgmt_rtg_req param %hd", (FMT__D, param));
    req_param = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_rtg_param_t);

    req = zb_buf_initial_alloc(param, sizeof(zb_zdo_mgmt_rtg_req_t));
    req->start_index = req_param->start_index;

    return zdo_send_req_by_short(ZDO_MGMT_RTG_REQ_CLID, param, cb, req_param->dst_addr, ZB_ZDO_CB_DEFAULT_COUNTER);
}

/* GP add, end */

zb_uint8_t zb_zdo_mgmt_bind_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_zdo_mgmt_bind_req_t *req;
    zb_zdo_mgmt_bind_param_t *req_param;

    TRACE_MSG(TRACE_ZDO3, "zb_zdo_mgmt_bind_req param %hd", (FMT__D, param));
    req_param = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_bind_param_t);
    req = zb_buf_initial_alloc(param, sizeof(zb_zdo_mgmt_bind_req_t));
    req->start_index = req_param->start_index;

    return zdo_send_req_by_short(ZDO_MGMT_BIND_REQ_CLID, param, cb, req_param->dst_addr, ZB_ZDO_CB_DEFAULT_COUNTER);
}

zb_uint8_t zdo_mgmt_leave_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_zdo_mgmt_leave_req_t *req;
    zb_zdo_mgmt_leave_param_t req_param;
    zb_uint8_t                zdo_tsn;
    ZB_MEMCPY(&req_param, ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_leave_param_t), sizeof(req_param));
    TRACE_MSG(TRACE_ZDO3, ">> zdo_mgmt_leave_req param %hd", (FMT__D, param));

    /*
     * Is is possible to do zdo_mgmt_leave_req locally. In such case dst_addr ==
     * our address. Let's not handle it here but let's APS&NWK pass it up for us.
     * There is a problem (or not a problem?) in such case: we will got
     * response (callback call) really before our local LEAVE complete.
     * It may or may not be a problem.
     */
    req = zb_buf_initial_alloc(param, sizeof(zb_zdo_mgmt_leave_req_t));
    zb_buf_flags_clr(param, ZB_BUF_ZDO_CMD_NO_RESP);
    ZB_IEEE_ADDR_COPY(req->device_address, req_param.device_address);
    req->remove_children = req_param.remove_children;
    req->rejoin = req_param.rejoin;

    /* Check whether device should send Leave Request to itself
     * It is needed when short destination address is equal to local short address
     * or device_address is equal to local IEEE address.
     *
     * NOTE: the second condition is needed to handle case when
     * NCP host doesn't still know its short address, but should perform local leave
     * (e.g. in case of failed authorization after association)
     */
    if (req_param.dst_addr == ZB_PIBCACHE_NETWORK_ADDRESS() ||
            ZB_IEEE_ADDR_CMP(req_param.device_address, ZB_PIBCACHE_EXTENDED_ADDRESS()))
    {
#ifdef ZB_JOIN_CLIENT
        /* local leave mgmt request */
        TRACE_MSG(TRACE_ZDO3, "local leave mgmt req", (FMT__0));
        /* NK: Reset zdo_cb table. Not sure about rejoin==true, but seems like we do not
         * want to call zdo callbacks until new join/rejoin. */
        /* if mgmt_leave_req is sent to a remote, local CB should not be reset.
          Then, if leave is done by remote request, need to reset zdo CB as well => it's better to call zdo_cb_reset() on leave indication */
        /* CR:NK In the case when it is local leave, there is no leave indication. Device calls
         * zdo_mgmt_leave_req, then leave confirm is called. Also the main reason was to call it before
         * leave request - to possibly free space in zdo_cb table for leave confirm. */
        /* NK: Moved to local leave case.*/

        /*
         * 1. ZBOSS should be confident that there is enough space to
         * store new ZDO callback, otherwise, it should assert or notify application.
         * So, actually there is no need to kill ZDO callbacks here.
         *
         * 2. Much better would be cleaning coresponding internal data structures
         * (e.g, APS, ZDO, maybe ZCL layers) when performing leave (w/ or
         * w/o rejoin).
         *
         * Question to Eugene: What do you think about all above?
         * EE: not sure, need double-check of the situation when it happens.
         * I can suppose that call was inserted to prevent callback entries leak.
         * Now we have callbacks aging which was absent in r20 ZBOSS where that code
         * introduced.
         * So now comment out but might need to uncomment.
         */
        zdo_tsn = zdo_mgmt_leave_cli(param, cb);
#else
        /* self-leave operation for coordinator: not clear what to do, let's skip it. */
        /* ZB_ASSERT(0); */
        zb_buf_free(param);
        zdo_tsn = 0;
#endif  /* ZB_JOIN_CLIENT */
    }
    else
    {
        TRACE_MSG(TRACE_ZDO3, "Send by unicast", (FMT__0));
        ZB_ASSERT(!ZB_NWK_IS_ADDRESS_BROADCAST(req_param.dst_addr));
        zdo_tsn = zdo_send_req_by_short(ZDO_MGMT_LEAVE_REQ_CLID, param, cb, req_param.dst_addr, ZB_ZDO_CB_UNICAST_COUNTER);
    }
    TRACE_MSG(TRACE_ZDO3, "<< zdo_mgmt_leave_req", (FMT__0));
    return zdo_tsn;
}

#ifdef ZB_JOIN_CLIENT
static zb_uint8_t zdo_mgmt_leave_cli(zb_uint8_t param, zb_callback_t cb)
{
    zb_ushort_t i;
    zb_zdo_mgmt_leave_req_t req;
    zb_uint8_t               zdo_tsn;
    zb_uint8_t               cb_tsn = 0xFF;

    TRACE_MSG(TRACE_ZDO3, ">>zdo_mgmt_leave_cli %hd", (FMT__H, param));

    ZDO_TSN_INC();
    zdo_tsn = ZDO_CTX().tsn;

    /* add entry to the leave req table */
    for (i = 0 ;
            i < ZB_ZDO_PENDING_LEAVE_SIZE
            && ZB_U2B(ZB_IS_LEAVE_PENDING(i)) ;
            ++i)
    {
    }

    if (i == ZB_ZDO_PENDING_LEAVE_SIZE)
    {
        if (cb != NULL)
        {
            zb_zdo_callback_info_t *zdo_info_p;

            TRACE_MSG(TRACE_ERROR, "out of pending leave list send resp now.!", (FMT__0));
            /* send resp just now. */

            zdo_info_p = zb_buf_initial_alloc(param, sizeof(zb_zdo_callback_info_t));
            zdo_info_p->tsn = zdo_tsn;
            zdo_info_p->status = ZB_ZDP_STATUS_INSUFFICIENT_SPACE;
            ZB_SCHEDULE_CALLBACK(cb, param);
        }
        else
        {
            zb_buf_free(param);
        }
    }
    else
    {
        zb_uint8_t rx_on_when_idle;

        ZB_MEMCPY(&req, zb_buf_begin(param), sizeof(zb_zdo_mgmt_leave_req_t));

        rx_on_when_idle = zb_nwk_get_nbr_rx_on_idle_by_ieee(req.device_address);
        if (rx_on_when_idle == ZB_NWK_NEIGHBOR_ERROR_VALUE)
        {
            rx_on_when_idle = ZB_FALSE;
        }

        cb_tsn = zdo_tsn;
        if (cb != NULL)
        {
            if (!register_zdo_cb(zdo_tsn, cb, 1, CB_TYPE_TSN, (zb_bool_t)rx_on_when_idle))
            {
                cb_tsn = 0xFF;
            }
        }

        ZB_SET_LEAVE_PENDING(i);

        ZG->nwk.leave_context.pending_list[i].tsn = zdo_tsn;
        ZG->nwk.leave_context.pending_list[i].src_addr = ZB_PIBCACHE_NETWORK_ADDRESS();
        ZG->nwk.leave_context.pending_list[i].buf_ref = param;
        TRACE_MSG(TRACE_ZDO3, "remember mgmt_leave at i %hd, tsn %hd, addr %d, buf_ref %hd",
                  (FMT__H_H_D_H, i, ZG->nwk.leave_context.pending_list[i].tsn,
                   ZG->nwk.leave_context.pending_list[i].src_addr,
                   ZG->nwk.leave_context.pending_list[i].buf_ref));

        /* Now locally call LEAVE.request */
        {
            zb_nlme_leave_request_t *lr;

            lr = ZB_BUF_GET_PARAM(param, zb_nlme_leave_request_t);
            ZB_IEEE_ADDR_COPY(lr->device_address, req.device_address);
            lr->remove_children = req.remove_children;
            lr->rejoin = req.rejoin;
            ZB_SCHEDULE_CALLBACK(zb_nlme_leave_request, param);
        }
    }
    TRACE_MSG(TRACE_ZDO3, "<<zdo_mgmt_leave_cli tsn: %hd, cb tsn: %hd", (FMT__H_H, zdo_tsn, cb_tsn));
    return cb_tsn;
}
#endif  /* ZB_JOIN_CLIENT */

zb_uint8_t zb_zdo_mgmt_permit_joining_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_zdo_mgmt_permit_joining_req_t *req;
    zb_zdo_mgmt_permit_joining_req_param_t req_param;
    zb_uint8_t zdo_tsn = 0xFF;

    TRACE_MSG(TRACE_ZDO3, ">> zb_zdo_mgmt_permit_joining_req param %hd cb %p", (FMT__H_P, param, cb));

    ZB_MEMCPY(&req_param, ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_permit_joining_req_param_t), sizeof(req_param));

#if defined ZB_ROUTER_ROLE
    if (req_param.dest_addr == ZB_PIBCACHE_NETWORK_ADDRESS())
    {
        TRACE_MSG(TRACE_ZDO3, "req to itself: permit_duration %hd", (FMT__H, req_param.permit_duration));
        zdo_mgmt_permit_joining_req_cli(param, cb);
    }
    else
#endif
    {
        zb_uint8_t resp_counter = ZB_ZDO_CB_DEFAULT_COUNTER;

        TRACE_MSG(TRACE_ZDO3, "send permit_joining_req to 0x%x", (FMT__D, req_param.dest_addr));

        req = zb_buf_initial_alloc(param, sizeof(zb_zdo_mgmt_permit_joining_req_t));
        /* [MM]: It isn't defined explicitly if the following actions should
         * be done on command TX, but lets fill the parameters as they
         * are expected on the RX side (according to r21, 2.4.3.3.7.2 Effect
         * on Receipt)
         */
        req->permit_duration = (req_param.permit_duration == 0xFFU) ? (0xFEU) : req_param.permit_duration;
#ifdef ZB_CERTIFICATION_HACKS
        if (ZB_CERT_HACKS().override_tc_significance_flag)
        {
            req->tc_significance = req_param.tc_significance;
        }
        else
#endif
        {
            req->tc_significance = 1;
        }

        /*
          Need mgmt_permit_joining callback to be called after command is sent,
          Don't wait response because in EZ-Mode this command is broadcast and
          no response may be received
        */
        if (ZB_NWK_IS_ADDRESS_BROADCAST(req_param.dest_addr))
        {
            zb_buf_flags_or(param, ZB_BUF_ZDO_CMD_NO_RESP);
            resp_counter = ZB_ZDO_CB_UNICAST_COUNTER;
        }

        zdo_tsn = zdo_send_req_by_short(ZDO_MGMT_PERMIT_JOINING_CLID, param, cb, req_param.dest_addr, resp_counter);
    }


    TRACE_MSG(TRACE_ZDO3, "<< zb_zdo_mgmt_permit_joining_req", (FMT__0));
    return zdo_tsn;
}

#if defined ZB_ROUTER_ROLE

void zdo_mgmt_permit_joining_req_cli(zb_uint8_t param, zb_callback_t cb)
{
    zb_bool_t ret_cb;
    zb_nlme_permit_joining_request_t *req;
    zb_zdo_mgmt_permit_joining_req_param_t req_param;

    TRACE_MSG(TRACE_ZDO2, ">> zdo_mgmt_permit_joining_cli param %hd cb %p", (FMT__H_P, param, cb));

    ret_cb = register_zdo_cb(param, cb, 1, CB_TYPE_INDEX, ZB_TRUE);
    if (!ret_cb)
    {
        TRACE_MSG(TRACE_ERROR, "register_zdo_cb failed [%d]", (FMT__D, ret_cb));
    }

    ZB_MEMCPY(&req_param, ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_permit_joining_req_param_t),
              sizeof(zb_zdo_mgmt_permit_joining_req_param_t));

    req = ZB_BUF_GET_PARAM(param, zb_nlme_permit_joining_request_t);
    ZB_BZERO(req, sizeof(zb_nlme_permit_joining_request_t));
    req->permit_duration = req_param.permit_duration;

    ZB_SCHEDULE_CALLBACK(zb_nlme_permit_joining_request, param);

    TRACE_MSG(TRACE_ZDO2, "<< zdo_mgmt_permit_joining_cli", (FMT__0));
}

void zdo_mgmt_permit_joining_resp_cli(zb_uint8_t param)
{
    zb_zdo_mgmt_permit_joining_resp_t *resp = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_permit_joining_resp_t);

    /* Repack the status to parameters and call the upper layer callback */
    resp->status = (zb_uint8_t)zb_buf_get_status(param);

    TRACE_MSG(TRACE_ZDO2, ">> zdo_mgmt_permit_joining_resp_cli status %d", (FMT__D, resp->status));

    if (zdo_run_cb_by_index(param) != RET_OK)
    {
        /* DD: here was an NCP signal. But it was generated when some other device sent us
           a permit joining request. So it useless for host as we didn't tell permit join request
           parameters. */

        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_ZDO2, "<< zdo_mgmt_permit_joining_resp_cli", (FMT__0));
}

#ifdef ZB_RAF_PERMIT_JOIN_ZDO_SIGNAL
zb_ret_t zb_send_permit_join_signal(zb_uint8_t permit_duration)
{
    zb_zdo_signal_permit_join_params_t *permit_join_params;
    zb_uint8_t param;

    param = zb_buf_get_out();
    if (!param)
    {
        TRACE_MSG(TRACE_ERROR, "ERROR: could not get out buf!", (FMT__0));
        return RET_ERROR;
    }

    TRACE_MSG(TRACE_ZDO1, "zb_zdo_signal_permit_join_params_t param %hd  duration 0x%hx ", (FMT__H_D, param, permit_duration));
    permit_join_params = (zb_zdo_signal_permit_join_params_t *)zb_app_signal_pack(param, ZB_ZDO_SIGNAL_PERMIT_JOIN, RET_OK, (zb_uint8_t)sizeof(zb_zdo_signal_permit_join_params_t));
    permit_join_params->permit_duration = permit_duration;


    ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete, param);

    return RET_OK;
}
#endif
#endif

#if defined ZB_JOINING_LIST_SUPPORT

zb_uint8_t zb_zdo_mgmt_nwk_ieee_joining_list_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_zdo_mgmt_nwk_ieee_joining_list_req_t *req;
    zb_zdo_mgmt_nwk_ieee_joining_list_param_t *req_param;

    TRACE_MSG(TRACE_ZDO3, "zb_zdo_mgmt_nwk_ieee_joining_list_req param %hd", (FMT__D, param));
    req_param = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_nwk_ieee_joining_list_param_t);

    req = zb_buf_initial_alloc(param, sizeof(zb_zdo_mgmt_nwk_ieee_joining_list_req_t));
    req->start_index = req_param->start_index;

    return zdo_send_req_by_short(ZDO_MGMT_NWK_IEEE_JOINING_LIST_REQ_CLID, param, cb, req_param->dst_addr, ZB_ZDO_CB_DEFAULT_COUNTER);
}

#endif /* ZB_JOINING_LIST_SUPPORT */

/*! @} */
