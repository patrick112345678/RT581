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
/* PURPOSE: NWK functionality specific to the parent: accept joins etc.
*/

#define ZB_TRACE_FILE_ID 75
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_mac.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "nwk_internal.h"
#include "zb_magic_macros.h"
#include "zdo_wwah_parent_classification.h"

/*! \addtogroup ZB_NWK */
/*! @{ */


#if defined ZB_ROUTER_ROLE
/* Note that coordinator role suppose router role. */
#if !defined ZB_COORDINATOR_ONLY
/*  zb_nlme_start_router_request never called by ZC.
 *  ZC after reboot uses zb_nwk_start_without_formation which calls mlme_start directly. */
void zb_nlme_start_router_request(zb_uint8_t param)
{
    TRACE_MSG(TRACE_NWK1, ">>start_router_req %hd", (FMT__H, param));
    ZB_TH_PUSH_PACKET(ZB_TH_NLME_NETWORK_START_ROUTER, ZB_TH_PRIMITIVE_REQUEST, param);

    /* check that we are already not a router */
    if (ZB_IS_DEVICE_ZR() )
    {
        zb_mlme_start_req_t req = {0};
        zb_nlme_start_router_request_t *request = ZB_BUF_GET_PARAM(param, zb_nlme_start_router_request_t);

        ZB_ASSERT(ZB_NWK_MAC_IFACE_TBL_SIZE == 1U);
        /* SS: TODO: loop over all enabled MAC ifaces when their count becomes more than 1. */

        TRACE_MSG(TRACE_NWK1, "b_ord %hd sf_ord %hd bat_life_ext %hd channel %hd panid 0x%x",
                  (FMT__H_H_H_H_D,
                   request->beacon_order, request->superframe_order, request->battery_life_extension,
                   ZB_PIBCACHE_CURRENT_CHANNEL(), ZB_PIBCACHE_PAN_ID()));
        req.pan_id = ZB_PIBCACHE_PAN_ID();
        req.channel_page = ZB_PIBCACHE_CURRENT_PAGE();
        req.logical_channel = ZB_PIBCACHE_CURRENT_CHANNEL();
        req.beacon_order = request->beacon_order;
        req.pan_coordinator = 0;
        req.superframe_order = request->superframe_order;
        req.battery_life_extension = request->battery_life_extension;
        req.coord_realignment = ZB_B2U(ZG->nwk.handle.router_started);
        ZG->nwk.handle.router_started = ZB_TRUE;

        ZB_MEMCPY(ZB_BUF_GET_PARAM(param, zb_mlme_start_req_t), &req, sizeof(req));
        ZB_SCHEDULE_CALLBACK(zb_mlme_start_request, param);

        ZG->nwk.handle.state = ZB_NLME_STATE_ROUTER;

        /* Internally lock NWK data indications.
           Need it to prevent passing up aps data before start router completed.
           The idea is: application must first receive "start done" then data.
         */
        nwk_internal_lock_in();
    }
    else
    {
        zb_buf_set_status(param, ZB_NWK_STATUS_INVALID_REQUEST);
        ZB_SCHEDULE_CALLBACK(zb_nlme_start_router_confirm, param);
    }

    TRACE_MSG(TRACE_NWK1, "<<start_router_req", (FMT__0));
}
#endif /* !ZB_COORDINATOR_ONLY */

void zb_nwk_update_beacon_payload_len(zb_uint8_t param);
void zb_nwk_update_beacon_payload_conf(zb_uint8_t param);
void zb_nwk_do_update_beacon_payload(zb_uint8_t param);

#if defined ZB_PARENT_CLASSIFICATION && defined ZB_ROUTER_ROLE
void nwk_set_tc_connectivity(zb_uint8_t val)
{
    if (ZB_IS_DEVICE_ZC_OR_ZR()
            && ZB_NIB().nwk_hub_connectivity != !!val)
    {
        ZB_NIB().nwk_hub_connectivity = !!val;
#ifdef ZB_ZCL_ENABLE_WWAH_SERVER
        /* Using that bits is Amazon WWAH feature. r23 uses TLVs instead */
        /* Do not interfere with ongoing commissioning */
        if (!ZG->nwk.handle.run_after_update_beacon_payload)
        {
            zb_buf_get_out_delayed(zb_nwk_update_beacon_payload);
        }
#endif
    }
}


zb_bool_t nwk_get_tc_connectivity(void)
{
    return ZB_NIB().nwk_hub_connectivity;
}
#endif  /* #if defined ZB_PARENT_CLASSIFICATION && defined ZB_ROUTER_ROLE */

/**
   Update beacon payload stored in MAC PIB and some other PIB values.

   This function must be called after MAC START confirm and after change any of
   parameter: router_capacity, end_device_capacity, nwk_update_id.
 */
void zb_nwk_update_beacon_payload(zb_uint8_t param)
{
    TRACE_MSG(TRACE_NWK1, ">>zb_nwk_update_beacon_payload %hd", (FMT__H, param));

    /*
      There was direct assigment to our PIB.
      To be able to use alien MAC, exclude direct assignment and update mac
      payload using MLME-SET call.
      Also, update macAssociationPermit and macBeaconpayloadLen using separate
      MLME-SET calls.
     */

    if (ZB_IS_DEVICE_ZED())
    {
        /* if we are ED, set zero-length beacon payload so MAC will not replay on
         * beacon payload requests. */
        zb_uint8_t len = 0;
        TRACE_MSG(TRACE_NWK1, "ZED - set empty beacon payload", (FMT__0));
        zb_nwk_pib_set(param, ZB_PIB_ATTRIBUTE_BEACON_PAYLOAD_LENGTH, &len, 1, zb_nwk_update_beacon_payload_conf);
    }
    else
    {
        /*
          Currently mac_association_permit changed when we out of joined children #.
        */

        if (!(
#ifndef ZB_PRO_STACK
                    ZB_NIB_DEPTH() < ZB_NIB_MAX_DEPTH() &&
#endif
                    ZB_NIB().max_children > ZB_NIB().router_child_num + ZB_NIB().ed_child_num))
        {
            TRACE_MSG(TRACE_NWK1, "depth %d max_ch %d >= %d + %d - reset association_permit",
                      (FMT__D_D_D_D, ZB_NIB_DEPTH(), ZB_NIB().max_children, ZB_NIB().router_child_num, ZB_NIB().ed_child_num));
            ZB_PIBCACHE_ASSOCIATION_PERMIT() = ZB_FALSE_U;
        }
        TRACE_MSG(TRACE_NWK1, "association_permit %hd", (FMT__H, ZB_PIBCACHE_ASSOCIATION_PERMIT()));
        zb_nwk_pib_set(param, ZB_PIB_ATTRIBUTE_ASSOCIATION_PERMIT, &ZB_PIBCACHE_ASSOCIATION_PERMIT(), 1, zb_nwk_update_beacon_payload_len);
    }
}


void zb_nwk_update_beacon_payload_len(zb_uint8_t param)
{
    zb_uint8_t len = (zb_uint8_t)sizeof(zb_mac_beacon_payload_t);
#if defined ZB_CERTIFICATION_HACKS
    if (ZB_CERT_HACKS().set_empty_beacon_payload)
    {
        len = 0;
        zb_nwk_pib_set(param, ZB_PIB_ATTRIBUTE_BEACON_PAYLOAD_LENGTH, &len, 1, zb_nwk_update_beacon_payload_conf);
        return;
    }
#endif
    TRACE_MSG(TRACE_NWK2, "zb_nwk_update_beacon_payload_len %hd len %hd", (FMT__H_H, param, len));
    zb_nwk_pib_set(param, ZB_PIB_ATTRIBUTE_BEACON_PAYLOAD_LENGTH, &len, 1, zb_nwk_do_update_beacon_payload);
}


void zb_nwk_do_update_beacon_payload(zb_uint8_t param)
{
    zb_mlme_set_request_t *req;
    zb_mac_beacon_payload_t *pl;

    TRACE_MSG(TRACE_NWK2, "zb_nwk_do_update_beacon_payload %hd", (FMT__H, param));
    req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_mac_beacon_payload_t));

    req->pib_attr = ZB_PIB_ATTRIBUTE_BEACON_PAYLOAD;
    req->pib_index = 0;
    req->pib_length = (zb_uint8_t)sizeof(zb_mac_beacon_payload_t);
    req->confirm_cb_u.cb = zb_nwk_update_beacon_payload_conf;

    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,24} */
    pl = (zb_mac_beacon_payload_t *)(req + 1);

    ZB_BZERO(pl, sizeof(zb_mac_beacon_payload_t));

    /* TODO: handle PRO device participate in 2007
     * network as ZE - set here 2007 stack profile. */

    pl->stack_profile = ZB_STACK_PROFILE;
    pl->protocol_version = ZB_PROTOCOL_VERSION;
    if (ZB_NIB_DEPTH() > 0x0fU)
    {
        pl->device_depth = 0x0f;
    }
    else
    {
        pl->device_depth = ZB_NIB_DEPTH();
    }
    ZB_EXTPANID_COPY(pl->extended_panid, ZB_NIB_EXT_PAN_ID());
    TRACE_MSG(TRACE_MAC3, "nib_ext_panid " TRACE_FORMAT_64 " depth %hd",
              (FMT__A_H,
               TRACE_ARG_64(ZB_NIB_EXT_PAN_ID()), pl->device_depth));

    /* TODO: fix the trace line */
    TRACE_MSG(TRACE_APS1, "Pan ID = " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(ZB_NIB_EXT_PAN_ID())));

    ZB_MEMSET(&pl->txoffset, -1, sizeof(pl->txoffset));

    TRACE_MSG(TRACE_NWK3, "max_children %hd router_child_num %hd ZB_NIB().ed_child_num %hd",
              (FMT__H_H_H, ZB_NIB().max_children, ZB_NIB().router_child_num, ZB_NIB().ed_child_num));

    if ((ZB_NIB().max_children > ZB_NIB().router_child_num + ZB_NIB().ed_child_num))
    {
        pl->router_capacity = 1;
        pl->end_device_capacity = 1;
        TRACE_MSG(TRACE_NWK3, "set capacity to 1", (FMT__0));
    }
    else
    {
        pl->router_capacity = ZB_PIBCACHE_ASSOCIATION_PERMIT();
        pl->end_device_capacity = ZB_PIBCACHE_ASSOCIATION_PERMIT();
        TRACE_MSG(TRACE_NWK3, "set capacity to %hd", (FMT__H, ZB_PIBCACHE_ASSOCIATION_PERMIT()));
    }

    TRACE_MSG(TRACE_NWK3, "update_id %hd", (FMT__H, ZB_NIB_UPDATE_ID()));
    pl->nwk_update_id = ZB_NIB_UPDATE_ID();

#ifdef ZB_ZCL_ENABLE_WWAH_SERVER
    /* Using that bits is Amazon WWAH feature. r23 uses TLVs instead */
    pl->tc_connectivity = ZB_NIB().nwk_hub_connectivity;
    pl->long_uptime = ZB_NIB().nwk_long_uptime;
#endif

    ZB_SCHEDULE_CALLBACK(zb_mlme_set_request, param);

    TRACE_MSG(TRACE_NWK1, "<<update_beacon_pl", (FMT__0));
}


void zb_nwk_update_beacon_payload_conf(zb_uint8_t param)
{
    TRACE_MSG(TRACE_NWK2, "zb_nwk_update_beacon_payload_conf %hd", (FMT__H, param));
    nwk_internal_unlock_in();
    if (ZG->nwk.handle.run_after_update_beacon_payload != NULL)
    {
        zb_buf_set_status(param, ZB_NWK_STATUS_SUCCESS);
        ZB_SCHEDULE_CALLBACK(ZG->nwk.handle.run_after_update_beacon_payload, param);
        ZG->nwk.handle.run_after_update_beacon_payload = NULL;
    }
    else
    {
        zb_buf_free(param);
    }
}

#ifdef ZB_PARENT_CLASSIFICATION
static void zdo_wwah_set_long_uptime(zb_uint8_t param)
{
    /* Moved there from zdo/zdo_wwah_parent_classification.c */
    ZVUNUSED(param);
    if (ZB_IS_DEVICE_ZC_OR_ZR()
            && ZB_JOINED())
    {
        ZB_NIB().nwk_long_uptime = 1;
#ifdef ZB_ZCL_ENABLE_WWAH_SERVER
        TRACE_MSG(TRACE_ZDO1, "schedule zb_nwk_update_beacon_payload", (FMT__0));
        zb_buf_get_out_delayed(zb_nwk_update_beacon_payload);
#endif
    }
    else
    {
        ZB_NIB().nwk_long_uptime = 0;
    }
}


void zdo_schedule_cancel_long_uptime(zb_uint8_t param)
{
    ZVUNUSED(param);
    ZB_SCHEDULE_ALARM_CANCEL(zdo_wwah_set_long_uptime, 0);
}


void zdo_schedule_set_long_uptime(zb_uint8_t param)
{
    ZVUNUSED(param);
    zdo_schedule_cancel_long_uptime(0);
    ZB_SCHEDULE_ALARM(zdo_wwah_set_long_uptime, 0 /* unused */, ZDO_WWAH_LONG_UPTIME_INTERVAL);
}
#endif  /* #ifdef ZB_PARENT_CLASSIFICATION */

#endif  /* ZB_ROUTER_ROLE */

/*! @} */
