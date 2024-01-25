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
/* PURPOSE: Network creation routine
*/

#define ZB_TRACE_FILE_ID 325
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_mac.h"
#ifndef ZB_ALIEN_MAC
#include "zb_mac_globals.h"
#endif
#include "zb_nwk.h"
#include "zb_secur.h"
#include "zb_aps.h"
#include "nwk_internal.h"
#include "zb_zdo.h"
#include "zb_magic_macros.h"
#include "zb_ie.h"
#include "zb_ncp.h"

#include "zdo_wwah_parent_classification.h"
#include "zb_bufpool.h"
/*! \addtogroup ZB_NWK */
/*! @{ */

#ifdef ZB_JOIN_CLIENT

static void zb_nlme_sync_pibcache_set_short_addr(zb_uint8_t param);
static void zb_nlme_rejoin_set_page(zb_uint8_t param);
static void zb_nlme_sync_pibcache_set_channel(zb_uint8_t param);
static void zb_nlme_sync_pibcache_set_parent_short(zb_uint8_t param);
static void zb_nlme_sync_pibcache_set_parent_long(zb_uint8_t param);

static void nwk_join_failure_confirm(zb_uint8_t param, zb_ret_t s)
{
    //! [zb_nlme_join_confirm]
    zb_nlme_join_confirm_t *join_confirm = ZB_BUF_GET_PARAM(param, zb_nlme_join_confirm_t);

    TRACE_MSG(TRACE_NWK2, "nwk_join_failure_confirm", (FMT__0));

    join_confirm->status = s;
    join_confirm->network_address = (s == RET_OK) ? ZB_PIBCACHE_NETWORK_ADDRESS() : ZB_NWK_BROADCAST_ALL_DEVICES;
    ZB_EXTPANID_COPY(join_confirm->extended_pan_id, ZB_NIB_EXT_PAN_ID());
    if (s != RET_OK)
    {
        ZB_NIB().nwk_hub_connectivity = 0;
    }
    ZB_SCHEDULE_CALLBACK(zb_nlme_join_confirm, param);
    //! [zb_nlme_join_confirm]
}


static void nwk_join_confirm_ok(zb_uint8_t param)
{
    /* confirm join result */
    nwk_join_failure_confirm(param, RET_OK);
}


zb_ext_neighbor_tbl_ent_t *nwk_choose_parent(zb_address_pan_id_ref_t panid_ref, zb_mac_capability_info_t capability_information)
{
    zb_ext_neighbor_tbl_ent_t *ret = NULL;
    zb_uint_t i;

    ZVUNUSED(capability_information);
    TRACE_MSG(TRACE_NWK1, ">>nwk_choose_parent panid_ref %hd cap %hx nib_upd_id %hd",
              (FMT__H_H_H, panid_ref, capability_information, ZB_NIB_UPDATE_ID()));

    /*
      Search its neighbor table for a suitable
      parent device, i.e. a device for which following conditions are true:

      - The device belongs to the network identified by the ExtendedPANId
      parameter.

      - The device is open to join requests and is advertising capacity of the correct
      device type.

      - The link quality for frames received from this device is such that a link cost of
      at most 3 is produced when calculated as described in sub-clause 3.6.3.1.

      - If the neighbor table entry contains a potential parent field for this device, that
      field shall have a value of 1 indicating that the device is a potential
      parent.

      - The device shall have the most recent update id, where the determination of
      most recent needs to take into account that the update id will wrap back to zero.
      In particular the update id given in the beacon payload of the device should be
      greater than or equal to - again, compensating for wrap -  the nwkUpdateId
      attribute of the NIB.

      If the neighbor table has more than one device that could be
      a suitable parent, the device which is at a minimum depth from the Zigbee
      coordinator may be chosen.
    */

    for (i = 0 ; i < ZB_NEIGHBOR_TABLE_SIZE ; ++i)
    {
        zb_ext_neighbor_tbl_ent_t *ne = &ZG->nwk.neighbor.neighbor[i];

        if (!(ZB_U2B(ne->used) && ZB_U2B(ne->ext_neighbor)))
        {
            continue;
        }

        TRACE_MSG(TRACE_NWK2, "ne %p panid_ref %hd, potential_par %hd, permit_join %hd, cap r/e %hd/%hd, lqi %hu, channel %d, upd_id %hd",
                  (FMT__P_H_H_H_H_H_H_H_H, ne, ne->u.ext.panid_ref, (zb_uint8_t)ne->u.ext.potential_parent, (zb_uint8_t) ne->permit_joining, (zb_uint8_t) ne->u.ext.router_capacity, (zb_uint8_t)ne->u.ext.end_device_capacity,
                   (zb_uint8_t) ne->lqi, ne->u.ext.logical_channel, (zb_uint8_t)ne->u.ext.update_id));
        //    TRACE_MSG(TRACE_NWK2, "classification_mask 0x%hx", (FMT__H, ne->u.ext.classification_mask));

        if (
            ne->u.ext.panid_ref == panid_ref
            && ZB_U2B(ne->u.ext.potential_parent)
            && ZB_U2B(ne->permit_joining)
#ifdef ZB_ROUTER_ROLE
            && ((ZB_U2B(ZB_MAC_CAP_GET_DEVICE_TYPE(capability_information))
                 && (ne->u.ext.stack_profile == STACK_PRO)) ? ZB_U2B(ne->u.ext.router_capacity) : ZB_U2B(ne->u.ext.end_device_capacity))
#else
            && ne->u.ext.end_device_capacity
#endif
            && ZB_LINK_QUALITY_IS_OK_FOR_JOIN(ne->lqi))
        {
#if defined ZB_STACK_REGRESSION_TESTING_API
            if (ZB_REGRESSION_TESTS_API().enable_custom_best_parent &&
                    ne->u.ext.short_addr == ZB_REGRESSION_TESTS_API().set_short_custom_best_parent)
            {
                ZB_REGRESSION_TESTS_API().enable_custom_best_parent = ZB_FALSE;
                ret = ne;
                break;
            }
#endif  /* ZB_STACK_REGRESSION_TESTING_API */
            if (ret != NULL && (
                        /* DA: ignore device depth in PRO stack. Reason: R21 core stack specification */
#ifndef ZB_PRO_STACK
                        ne->depth >= ret->depth  ||
#endif

#ifndef ZB_PARENT_CLASSIFICATION
                        ZB_LINK_QUALITY_1_IS_BETTER(ret->lqi, ne->lqi)
#else
                        zdo_wwah_compare_neighbors(ret, ne)
#endif
                    ))
            {
                TRACE_MSG(TRACE_NWK1, "best_par %p ne %p dep %d / %d, lqi %d / %d - skip", (FMT__D_P_P_D_D_H_H, ret, ne, ne->depth, ret->depth, ret->lqi, ne->lqi));
                continue;
            }
            TRACE_MSG(TRACE_NWK1, "parent found: %p", (FMT__P, ne));
            ret = ne;
        }
        else
        {
            TRACE_MSG(TRACE_NWK1, "ne %p is not potential parent: panid_ok %hd potent_prnt_ok %hd permit_j_ok %hd cap_ok %hd lqi_ok %hd upd_id_ok %hd",
                      (FMT__P_H_H_H_H_H_H,
                       ne,
                       (zb_uint8_t)(ne->u.ext.panid_ref == panid_ref),
                       (zb_uint8_t)ne->u.ext.potential_parent,
                       (zb_uint8_t)ne->permit_joining,
                       (zb_uint8_t)(ZB_MAC_CAP_GET_DEVICE_TYPE(capability_information) ?
                                    (zb_uint8_t)ne->u.ext.router_capacity : (zb_uint8_t)ne->u.ext.end_device_capacity),
                       (zb_uint8_t)ZB_LINK_QUALITY_IS_OK_FOR_JOIN(ne->lqi),
                       (zb_uint8_t)ZB_NWK_UPDATE_ID1_GE_ID2(ne->u.ext.update_id, ZB_NIB_UPDATE_ID())));
            if (!ZB_LINK_QUALITY_IS_OK_FOR_JOIN(ne->lqi))
            {
                TRACE_MSG(TRACE_ERROR, "low lqi %hd", (FMT__H, ne->lqi));
            }
        }
    } /* for */

    TRACE_MSG(TRACE_NWK1, "<<nwk_choose_parent %p", (FMT__P, ret));

    return ret;
}

/**
   3.6.1.4.1  Joining a Network Through Association

   This routine can be called when upper layer called join first time or for
   attempt to join to another parent.
 */
static zb_ret_t nwk_association_join(zb_bufid_t buf, zb_nlme_join_request_t *request)
{
    zb_ext_neighbor_tbl_ent_t *best_parent = NULL;
    zb_address_pan_id_ref_t panid_ref = 0; /* shutup sdcc */
    zb_uint16_t panid;
    zb_ret_t ret;

    TRACE_MSG(TRACE_NWK1, ">>assoc_join buf %p req %p", (FMT__P_P, buf, request));

    /* upper layer logic (ZDO) has chosen PAN */

    /* see 3.6.1.4.1.1    Child Procedure */


    /*
      Only those devices that are not already joined to a network shall initiate the join
      procedure. If any other device initiates this procedure, the NLME shall terminate
      the procedure and notify the next higher layer of the illegal request by issuing the
      NLME-JOIN.confirm primitive with the Status parameter set to
      INVALID_REQUEST.
     */
    if (ZB_JOINED())
    {
        TRACE_MSG(TRACE_ERROR, "Association join req while joined", (FMT__0));
        ret = ERROR_CODE(ERROR_CATEGORY_NWK, ZB_NWK_STATUS_INVALID_REQUEST);
    }
    else if ( zb_address_get_pan_id_ref(request->extended_pan_id, &panid_ref) != RET_OK )
    {
        TRACE_MSG(TRACE_NWK1, "No dev with xpanid " TRACE_FORMAT_64,
                  (FMT__A, TRACE_ARG_64(request->extended_pan_id)));
        ret = ERROR_CODE(ERROR_CATEGORY_NWK, ZB_NWK_STATUS_NOT_PERMITTED);
    }
    else
    {
        ret = (zb_ret_t)ZB_NWK_STATUS_SUCCESS;
    }

    if (ret == (zb_ret_t)ZB_NWK_STATUS_SUCCESS)
    {
        best_parent = nwk_choose_parent(panid_ref, request->capability_information);

        if (best_parent == NULL)
        {
            /*
              If the neighbor table contains no devices that are suitable parents, the NLME shall
              respond with an NLME-JOIN.confirm with a Status parameter of
              NOT_PERMITTED.
            */

            TRACE_MSG(TRACE_ERROR, "No dev for join", (FMT__0));
            ret = ERROR_CODE(ERROR_CATEGORY_NWK, ZB_NWK_STATUS_NOT_PERMITTED);
        }
    }   /* if ok */

    if (best_parent != NULL)
    {
        /*
          Issue an MLME-ASSOCIATE.request primitive to the  MAC sub-layer
         */
        zb_address_get_short_pan_id(panid_ref, &panid);
        /* Remember my PAN id in NIB just now: don't want to store it
         * externally. Anyway, it is illegal until Join complete. */
        ZB_PIBCACHE_PAN_ID() = panid;
        ZB_PIBCACHE_CURRENT_CHANNEL() = best_parent->u.ext.logical_channel;
        ZB_PIBCACHE_CURRENT_PAGE() = best_parent->u.ext.channel_page;
        /* Do not need to sync it with MAC just now: we pass it to MAC in MLME-ASSOCIATE.REQ */

        /* save request to be able to reattempt connect */
        ZB_MEMCPY(&ZG->nwk.handle.tmp.join.saved_join_req, request, sizeof(*request));

        if (!ZB_ADDRESS_COMPRESSED_IS_UNKNOWN(best_parent->u.ext.long_addr))
        {
            zb_ieee_addr_t long_addr;
            ZB_ADDRESS_DECOMPRESS(long_addr, best_parent->u.ext.long_addr);

            TRACE_MSG(TRACE_ERROR, "Will assoc to pan 0x%x on channel %hd, dev " TRACE_FORMAT_64,
                      (FMT__D_H_A, panid, best_parent->u.ext.logical_channel, TRACE_ARG_64(long_addr)));

            ZB_MLME_BUILD_ASSOCIATE_REQUEST(buf, best_parent->u.ext.channel_page, best_parent->u.ext.logical_channel,
                                            panid,
                                            ZB_ADDR_64BIT_DEV, long_addr,
                                            ZG->nwk.handle.tmp.join.saved_join_req.capability_information);

        }
        else
        {
            TRACE_MSG(TRACE_ERROR, "Will assoc to pan 0x%x on channel %hd, dev 0x%x",
                      (FMT__D_H_D, panid, best_parent->u.ext.logical_channel, best_parent->u.ext.short_addr));

            ZB_MLME_BUILD_ASSOCIATE_REQUEST(buf, best_parent->u.ext.channel_page, best_parent->u.ext.logical_channel,
                                            panid,
                                            ZB_ADDR_16BIT_DEV_OR_BROADCAST, (zb_uint8_t *)&best_parent->u.ext.short_addr,
                                            ZG->nwk.handle.tmp.join.saved_join_req.capability_information);
        }

        /* remember parent to be able to ref it after association complete */
        ZG->nwk.handle.tmp.join.parent = best_parent;
        ZB_NIB().nwk_hub_connectivity = 0;
        ZB_SCHEDULE_CALLBACK(zb_mlme_associate_request, buf);
    }
    else
    {
        // MA 10/29/2014: run with next PanID
        // TODO need review!
        // ZB_SCHEDULE_CALLBACK(zb_nlme_network_discovery_confirm, ZB_REF_FROM_BUF(buf));
    }

    TRACE_MSG(TRACE_NWK1, "<<assoc_join %d", (FMT__D, ret));
    return ret;
}

void zb_mlme_associate_confirm(zb_uint8_t param)
{
    zb_ret_t ret;
    zb_mlme_associate_confirm_t *request = ZB_BUF_GET_PARAM(param, zb_mlme_associate_confirm_t);
    zb_address_ieee_ref_t addr_ref;

    TRACE_MSG(TRACE_NWK1, ">>zb_mlme_associate_confirm %hd", (FMT__H, param));

    CHECK_PARAM_RET_ON_ERROR(request);

    if ( request->status == MAC_SUCCESS )
    {
        /* Check if short address is correct. It can not be broadcast at least. */
        if (!zb_nwk_check_assigned_short_addr(request->assoc_short_address))
        {
            TRACE_MSG(TRACE_NWK1, "Assoc fail - Wrong short addr", (FMT__0));
            nwk_join_failure_confirm(param, ERROR_CODE(ERROR_CATEGORY_MAC, MAC_INVALID_ADDRESS));
            return;
        }

        ZB_SET_JOINED_STATUS(ZB_TRUE);

        /* short pan id is already stored in NIB - see nwk_association_join */

        ZB_EXTPANID_COPY(ZB_NIB_EXT_PAN_ID(), ZG->nwk.handle.tmp.join.saved_join_req.extended_pan_id);
        TRACE_MSG(TRACE_NWK3, "Pan ID = " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(ZB_NIB_EXT_PAN_ID())));

        /* remember my short network address. MAC already knows it. */
        ZB_PIBCACHE_NETWORK_ADDRESS() = request->assoc_short_address;

        /* update nwkUpdateId */
        ZB_NIB_UPDATE_ID() = ZG->nwk.handle.tmp.join.parent->u.ext.update_id;
        TRACE_MSG(TRACE_NWK3, "new update_id %hd", (FMT__H, ZB_NIB_UPDATE_ID()));

        /* remember this device long addr + short addr in the addr table */
        /* Lock ourselves to prevent clearing if the address table is full */
        ret = zb_address_update(ZB_PIBCACHE_EXTENDED_ADDRESS(), request->assoc_short_address, ZB_TRUE, &addr_ref);
        ZB_ASSERT(ret == RET_OK);

        /* remember potential parent address to find it in the neighbor table */
        {
            zb_neighbor_tbl_ent_t *nent = NULL; /* shutup sdcc */
            zb_uint16_t short_addr = ZG->nwk.handle.tmp.join.parent->u.ext.short_addr;
            zb_ieee_addr_t long_addr;

            if (short_addr == (zb_uint16_t)~0U)
            {
                /* If we sent request to long MAC. This is unusual case. */
                ZB_ADDRESS_DECOMPRESS(long_addr, ZG->nwk.handle.tmp.join.parent->u.ext.long_addr);
            }
            else
            {
                /* Usual case: get parent address from association confirm packet */
                ZB_IEEE_ADDR_COPY(long_addr, request->parent_address);
            }

            TRACE_MSG(TRACE_NWK1, "parent addr  0x%x = " TRACE_FORMAT_64,
                      (FMT__D_A, short_addr, TRACE_ARG_64(long_addr)));

            /* see 3.6.1.4.1.1:

               The network depth is set to one more than the parent network depth
               unless the parent network depth has a value of 0x0f, i.e. the maximum
               value for the 4-bit device depth field in the beacon payload. In this
               case, the network depth shall also be set to 0x0f.
            */
#ifdef ZB_ROUTER_ROLE
#ifdef ZB_NWK_DISTRIBUTED_ADDRESS_ASSIGN
            ZB_NIB_DEPTH() = ZG->nwk.handle.tmp.join.parent->depth + (ZB_NIB_DEPTH() != 0xf);
            /* Calculate CScip value for tree routing */
            ZB_NIB().cskip = zb_nwk_daa_calc_cskip(ZB_NIB_DEPTH());
#else
            /* Note: depth limit is about beacon payload contents. In PRO it is ok to
             * join at depth 16 (but not more) */
            if (ZB_NIB_DEPTH() != 0x0fU)
            {
                ZB_NIB_DEPTH() = ZG->nwk.handle.tmp.join.parent->depth + 1U;
            }
#endif
#endif

            if (ZB_NIB_SECURITY_LEVEL() != 0U)
            {
                zb_address_pan_id_ref_t panid_ref;

                /* If device operating in secured network do not kill extneighbors after
                 * successful association - it will be needed until authentication not done.
                 * In case of authentication failure extneighbors will be used to search another
                 * PAN to join (required by bdb steering not on network). */
                ret = zb_address_get_pan_id_ref(ZB_NIB_EXT_PAN_ID(), &panid_ref);
                ZB_ASSERT(ret == RET_OK);
                ret = zb_nwk_exneighbor_by_short(panid_ref, short_addr, &nent);
                ZB_ASSERT(ret == RET_OK);
                ret = zb_nwk_neighbor_ext_to_base(nent, ZB_TRUE);
                ZB_ASSERT(ret == RET_OK);
                /* Now we have neighbor table entry and address entry for our parent. Address is locked (because it is our neighbor), lock count 1 */
            }
#ifndef ZB_LITE_ALWAYS_SECURE
            else
            {
                /* Stop ext neighbor table - convert it to the normal neighbor.
                   When no security, sure will have no need to join to some other
                   device, so kill extneighbor.
                 */
                zb_nwk_exneighbor_stop(short_addr);
            }
#endif

            /*
             * 'parent' pointed to the ext neighbor and now is invalid.
             * Find my parent in the neighbor table and mark it using Relationship
             * field. Find either by long or short address.
             * Lock parent's address in the address translation table.
             */

            if (short_addr == (zb_uint16_t)~0U)
            {
                /* use long address */
                ret = zb_address_by_ieee(long_addr, ZB_FALSE, ZB_TRUE, &addr_ref);
            }
            else
            {
                /* Should be already locked from ext_to_base()!
                   Lock count is 1 now.
                 */
                ret = zb_address_by_short(short_addr,
                                          ZB_FALSE, ZB_FALSE, &addr_ref);
            }
            if (ret != RET_OK)
            {
                TRACE_MSG(TRACE_NWK1, "Couldn't find addr", (FMT__0));
                ZB_ASSERT(0);
            }
            else
            {
                if (!ZB_IEEE_ADDR_IS_UNKNOWN(long_addr) && short_addr != (zb_uint16_t)~0U)
                {
                    /* association request was sent to the short address, got responce
                     * from the long address - can update address translation table */
                    ret = zb_address_update(request->parent_address, short_addr, ZB_FALSE, &addr_ref);
                    ZB_ASSERT(ret == RET_OK);
                }

                ret = zb_nwk_neighbor_get(addr_ref, ZB_FALSE, &nent);
            }
            if (ret == RET_OK)
            {
                nent->relationship = ZB_NWK_RELATIONSHIP_PARENT;
                TRACE_MSG(TRACE_NWK3, "nb ent %p type %hd rel %hd",
                          (FMT__P_H_H, nent, (zb_uint8_t)nent->device_type, (zb_uint8_t)nent->relationship));

                ZG->nwk.handle.parent = addr_ref;

#ifdef ZB_USE_NVRAM
                zb_nvram_store_addr_n_nbt();
#endif
            }
        }

        /* Notify upper layer */
        {
            zb_ret_t status = (zb_ret_t)request->status;
            zb_nlme_join_confirm_t *join_confirm = ZB_BUF_GET_PARAM(param, zb_nlme_join_confirm_t);

            join_confirm->status = status;
            join_confirm->network_address = ZB_PIBCACHE_NETWORK_ADDRESS();
            ZB_EXTPANID_COPY(join_confirm->extended_pan_id, ZG->nwk.handle.tmp.join.saved_join_req.extended_pan_id);
            join_confirm->active_channel = ZG->nwk.handle.tmp.join.parent->u.ext.logical_channel;
            ZB_SCHEDULE_CALLBACK(zb_nlme_join_confirm, param);

            ZB_NIB().nwk_hub_connectivity = (ZG->nwk.handle.tmp.join.parent->u.ext.short_addr == 0U) ? 1U : 0U;
        }
    }
    else
    {
        /* Unsuccessful associate. Attempt to join to another parent or join as ZE,
         * if failed - pass error up. */
        zb_mac_status_t status = request->status;
        /* try to join to another device. */

        /*
         * Below is the hack which was made for BME. It allows to try
         * several association attempts to selected device. If the
         * association is unsuccessfull all times, nlme-join.confirm()
         * will be triggered and discard this PAN. And actually it
         * violates the spec, because the device should try associate to
         * another potential parent of the PAN.
         *
         * Put changes made for MBE under define. By default when association with potential parent failed
         * try to join to another nodes in selected network. Pass up fail confirm only
         * if join attempt failed for all nodes.
         */
#ifdef ZB_CUSTOMER_SPECIFIC_JOIN_BEHAVIOR
        /* [MM]: KLUDGE: keep trying to join the same
         * parent of the choosen parent N times (logic by ZDO joining
         * counter). Discussed with EE. */
        TRACE_MSG(TRACE_NWK1, "Assoc fail - pass MAC st %d up", (FMT__D, status));
        nwk_join_failure_confirm(param, ERROR_CODE(ERROR_CATEGORY_MAC, status));
#else
        ZG->nwk.handle.tmp.join.parent->u.ext.potential_parent = 0;

        ret = nwk_association_join(param, &ZG->nwk.handle.tmp.join.saved_join_req);
        TRACE_MSG(TRACE_NWK1, "Assoc retr ret %d", (FMT__D, ret));
        if (ret != 0)
        {
            TRACE_MSG(TRACE_NWK1, "Assoc fail - pass MAC st %d up", (FMT__D, status));
            nwk_join_failure_confirm(param, ERROR_CODE(ERROR_CATEGORY_MAC, status));
        }
        else
        {
            /* if ret is ok, let's wait for the next zb_mlme_associate_confirm() */
            TRACE_MSG(TRACE_NWK3, "wait for assoc conf", (FMT__0));
        }
#endif
    }

    TRACE_MSG(TRACE_NWK1, "<<zb_mlme_associate_confirm", (FMT__0));
}

static void zb_nlme_rejoin(zb_uint8_t param)
{
    zb_ret_t ret;
    zb_nlme_join_request_t *request = ZB_BUF_GET_PARAM(param, zb_nlme_join_request_t);
    zb_uint8_t scan_iface_idx;
    zb_uint8_t channel_page;
    zb_uint32_t channel_mask;

    TRACE_MSG(TRACE_NWK1, ">>zb_nlme_rejoin %hd", (FMT__H, param));

    if ( ZG->nwk.handle.state == ZB_NLME_STATE_IDLE )
    {
        /* Start discovery using the first supported page. */
        ret = nwk_scan_find_next_channel_mask(0, 0, request->scan_channels_list, &scan_iface_idx,
                                              &channel_page, &channel_mask);
        if (ret == RET_OK)
        {
            TRACE_MSG(TRACE_NWK1, "xpanid " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(request->extended_pan_id)));
            /* Save request params */
            ZB_EXTPANID_COPY(ZG->nwk.handle.tmp.rejoin.extended_pan_id, request->extended_pan_id);
            ZG->nwk.handle.tmp.rejoin.capability_information = request->capability_information;

            /* Make a copy to use it for further scan request sequence. */
            zb_channel_page_list_copy(ZG->nwk.handle.scan_channels_list, request->scan_channels_list);
            ZG->nwk.handle.state = ZB_NLME_STATE_REJOIN;
            ZG->nwk.handle.router_started = ZB_FALSE;
            /* start active scan */
            nlme_scan_request(param, ACTIVE_SCAN, request->scan_duration,
                              scan_iface_idx, channel_page, channel_mask);
        }
        else
        {
            TRACE_MSG(TRACE_NWK1, "Could not get channels mask to start rejoin scan!", (FMT__0));
            /* SS: TODO: Cleanup status codes!!!!!*/
            nwk_join_failure_confirm(param, ERROR_CODE(ERROR_CATEGORY_NWK, ZB_NWK_STATUS_INVALID_PARAMETER));
        }
    }
    else
    {
        TRACE_MSG(TRACE_NWK1, "nwk is busy, state %hd", (FMT__H, ZG->nwk.handle.state));
        nwk_join_failure_confirm(param, ERROR_CODE(ERROR_CATEGORY_NWK, ZB_NWK_STATUS_NOT_PERMITTED));
    }

    TRACE_MSG(TRACE_NWK1, "<<rejoin", (FMT__0));
}

void zb_nlme_rejoin_set_short_addr(zb_uint8_t param);
void zb_nlme_rejoin_set_channel(zb_uint8_t param);
void zb_nlme_rejoin_set_parent_short(zb_uint8_t param);
void zb_nlme_rejoin_set_parent_long(zb_uint8_t param);
void zb_nlme_rejoin_send_pkt(zb_uint8_t param);


void zb_nlme_rejoin_scan_confirm(zb_uint8_t param)
{
    zb_address_pan_id_ref_t panid_ref = 0;
    zb_ext_neighbor_tbl_ent_t *best_parent = NULL;
    zb_ret_t result_get_pan_id_ref;

    TRACE_MSG(TRACE_NWK1, ">>zb_nlme_rejoin_scan_confirm %hd", (FMT__H, param));

#if 0
    /* EE: keep blacklist!! We still not rejoined successfully! */
#ifdef ZB_NWK_BLACKLIST
    /* DA: if there was a rejoin attempt, reset blacklist (because it means that we found suitable network */
    zb_nwk_blacklist_reset();
#endif
#endif
    result_get_pan_id_ref = zb_address_get_pan_id_ref(ZG->nwk.handle.tmp.rejoin.extended_pan_id, &panid_ref);
    if (result_get_pan_id_ref == RET_OK)
    {
        best_parent = nwk_choose_parent(panid_ref, ZG->nwk.handle.tmp.rejoin.capability_information);
    }

#ifdef ZB_CERTIFICATION_HACKS
    if (result_get_pan_id_ref == RET_OK && ZB_CERT_HACKS().enable_rejoin_to_specified_device)
    {
        TRACE_MSG(TRACE_ERROR, "CERT HACK: rejoin to 0x%x", (FMT__D, ZB_CERT_HACKS().address_to_rejoin));
        zb_nwk_exneighbor_by_short(panid_ref, ZB_CERT_HACKS().address_to_rejoin, &best_parent);
    }
#endif /* ZB_CERTIFICATION_HACKS */

    if (result_get_pan_id_ref != RET_OK || best_parent == NULL)
    {
        TRACE_MSG(TRACE_ERROR, "Rejoin failure: no dev with xpanid " TRACE_FORMAT_64 " or can't choose best parent",
                  (FMT__A, TRACE_ARG_64(ZG->nwk.handle.tmp.rejoin.extended_pan_id)));

        ZG->nwk.handle.state = ZB_NLME_STATE_IDLE;
        zb_nwk_exneighbor_stop((zb_uint16_t) -1);
        nwk_join_failure_confirm(param, ERROR_CODE(ERROR_CATEGORY_NWK, ZB_NWK_STATUS_NOT_PERMITTED));
    }
    else
    {
        zb_ret_t ret;

        /* Assign panid to fill mac hdr by it */
        zb_address_get_short_pan_id(panid_ref, &ZB_PIBCACHE_PAN_ID());

        /* set random network address if not set */
        while ( ZB_PIBCACHE_NETWORK_ADDRESS() == 0U
                || ZB_PIBCACHE_NETWORK_ADDRESS() == ZB_MAC_SHORT_ADDR_NOT_ALLOCATED
                || ZB_NWK_IS_ADDRESS_BROADCAST(ZB_PIBCACHE_NETWORK_ADDRESS()) )
        {
            ZB_PIBCACHE_NETWORK_ADDRESS() = ZB_RANDOM_U16();
        }

        /*
          Change channel, panid, parent long and short address in MAC HW to be
          able to communicate with future parent.

          To be compatible with alien MAC, exclude usage of
          zb_mac_setup_for_associate() and set it all by changing PIB via MLME-SET
          (more then 1 call, blockable).

          ZB_PIBCACHE_PAN_ID() - ZB_PIB_ATTRIBUTE_PANID
          ZB_PIBCACHE_NETWORK_ADDRESS() - ZB_PIB_ATTRIBUTE_SHORT_ADDRESS
          best_parent->logical_channel - ZB_PHY_PIB_CURRENT_CHANNEL in PIB
          best_parent->short_addr - coordinator address - ZB_PIB_ATTRIBUTE_COORD_SHORT_ADDRESS
          best_parent->long_addr - coordinator address - ZB_PIB_ATTRIBUTE_COORD_LONG_ADDRESS
        */

        /* save parent */
        ZG->nwk.handle.tmp.rejoin.parent = best_parent;

        /* create address translation entry, add temporary ext neighbor entry to
         * base neighbor. It locks parent's address as a neighbor, lock count 1. */
        ret = zb_nwk_neighbor_ext_to_base(best_parent, ZB_TRUE);
        ZB_ASSERT(ret == RET_OK);
        if (ret != RET_OK)
        {
            TRACE_MSG(TRACE_ERROR, "zb_nwk_neighbor_ext_to_base failed [%d]", (FMT__D, ret));
        }

        zb_nwk_rejoin_sync_pibcache_with_mac(param, zb_nlme_rejoin_send_pkt);
    }
}


void zb_nwk_rejoin_sync_pibcache_with_mac(zb_uint8_t param, zb_callback_t cb)
{
    TRACE_MSG(TRACE_NWK3, "zb_nwk_rejoin_sync_pibcache_with_mac", (FMT__0));
    ZG->nwk.handle.tmp.rejoin.cb = cb;
    /* pass panid to MAC */
    zb_nwk_pib_set(param, ZB_PIB_ATTRIBUTE_PANID, &ZB_PIBCACHE_PAN_ID(), 2, zb_nlme_rejoin_set_short_addr);
}


void zb_nlme_rejoin_set_short_addr(zb_uint8_t param)
{
    TRACE_MSG(TRACE_NWK3, "zb_nlme_rejoin_set_short_addr", (FMT__0));
    zb_nwk_pib_set(param, ZB_PIB_ATTRIBUTE_SHORT_ADDRESS, &ZB_PIBCACHE_NETWORK_ADDRESS(), 2, zb_nlme_rejoin_set_page);
}

void zb_nlme_rejoin_set_page(zb_uint8_t param)
{
    TRACE_MSG(TRACE_NWK3, "zb_nlme_rejoin_set_page", (FMT__0));
    ZB_PIBCACHE_CURRENT_PAGE() = ZG->nwk.handle.tmp.rejoin.parent->u.ext.channel_page;
    zb_nwk_pib_set(param, ZB_PHY_PIB_CURRENT_PAGE, &ZB_PIBCACHE_CURRENT_PAGE(),
                   1, zb_nlme_rejoin_set_channel);
}

void zb_nlme_rejoin_set_channel(zb_uint8_t param)
{
    TRACE_MSG(TRACE_NWK3, "zb_nlme_rejoin_set_channel", (FMT__0));
    ZB_PIBCACHE_CURRENT_CHANNEL() = ZG->nwk.handle.tmp.rejoin.parent->u.ext.logical_channel;
    zb_nwk_pib_set(param, ZB_PHY_PIB_CURRENT_CHANNEL, &ZB_PIBCACHE_CURRENT_CHANNEL(),
                   1, zb_nlme_rejoin_set_parent_short);
}


void zb_nlme_rejoin_set_parent_short(zb_uint8_t param)
{
    TRACE_MSG(TRACE_NWK3, "zb_nlme_rejoin_set_parent_short", (FMT__0));
    zb_nwk_pib_set(param, ZB_PIB_ATTRIBUTE_COORD_SHORT_ADDRESS, (void *)&ZG->nwk.handle.tmp.rejoin.parent->u.ext.short_addr,
                   2, zb_nlme_rejoin_set_parent_long);
}

void zb_nlme_rejoin_set_parent_long(zb_uint8_t param)
{
    zb_ieee_addr_t long_address;

    TRACE_MSG(TRACE_NWK3, "zb_nlme_rejoin_set_parent_long", (FMT__0));
    ZB_ADDRESS_DECOMPRESS(long_address, ZG->nwk.handle.tmp.rejoin.parent->u.ext.long_addr);
    zb_nwk_pib_set(param, ZB_PIB_ATTRIBUTE_COORD_EXTEND_ADDRESS, long_address,
                   sizeof(zb_ieee_addr_t),
                   ZG->nwk.handle.tmp.rejoin.cb);
}


void zb_nwk_sync_pibcache_with_mac(zb_uint8_t param, zb_callback_t cb)
{
    ZG->nwk.handle.tmp.rejoin.cb = cb;
    /* pass panid to MAC */
    zb_nwk_pib_set(param, ZB_PIB_ATTRIBUTE_PANID, &ZB_PIBCACHE_PAN_ID(), 2, zb_nlme_sync_pibcache_set_short_addr);
}


static void zb_nlme_sync_pibcache_set_short_addr(zb_uint8_t param)
{
    zb_nwk_pib_set(param, ZB_PIB_ATTRIBUTE_SHORT_ADDRESS, &ZB_PIBCACHE_NETWORK_ADDRESS(), 2, zb_nlme_sync_pibcache_set_channel);
}


static void zb_nlme_sync_pibcache_set_channel(zb_uint8_t param)
{
    zb_nwk_pib_set(param, ZB_PHY_PIB_CURRENT_CHANNEL, &ZB_PIBCACHE_CURRENT_CHANNEL(),
                   1, zb_nlme_sync_pibcache_set_parent_short);
}


static void zb_nlme_sync_pibcache_set_parent_short(zb_uint8_t param)
{
    zb_uint16_t addr;
    zb_address_short_by_ref(&addr, ZG->nwk.handle.parent);
    zb_nwk_pib_set(param, ZB_PIB_ATTRIBUTE_COORD_SHORT_ADDRESS, &addr,
                   2, zb_nlme_sync_pibcache_set_parent_long);
}

static void zb_nlme_sync_pibcache_set_parent_long(zb_uint8_t param)
{
    zb_ieee_addr_t long_address;
    zb_address_ieee_by_ref(long_address, ZG->nwk.handle.parent);
    zb_nwk_pib_set(param, ZB_PIB_ATTRIBUTE_COORD_EXTEND_ADDRESS, long_address,
                   sizeof(zb_ieee_addr_t),
                   ZG->nwk.handle.tmp.rejoin.cb);
}

void zb_nlme_rejoin_send_pkt(zb_uint8_t param)
{
    zb_ext_neighbor_tbl_ent_t *best_parent = ZG->nwk.handle.tmp.rejoin.parent;

    zb_bool_t secure = (ZB_NIB_SECURITY_LEVEL() != 0U && !ZB_U2B(ZG->nwk.handle.tmp.rejoin.unsecured_rejoin));

    TRACE_MSG(TRACE_NWK1, ">>zb_nlme_rejoin_send_pkt %hd secure %hd", (FMT__H_H, param, secure));
    TRACE_MSG(TRACE_ATM1, "Z< send rejoin request", (FMT__0));

    {
        zb_nwk_hdr_t *nwhdr;

        nwhdr = nwk_alloc_and_fill_hdr(param,
                                       ZB_PIBCACHE_NETWORK_ADDRESS(), best_parent->u.ext.short_addr,
                                       ZB_FALSE, secure, ZB_TRUE, ZB_TRUE);
        /* AT: See: 3.4.6.2 NWK Header Fields [3.4.6 Rejoin Request Command] from specification document
         * ... The radius field shall be set to 1....
         */
        nwhdr->radius = 1;
    }
    if (secure)
    {
        nwk_mark_nwk_encr(param);
    }
    /* Reset unsecured rejoin. This flag should be set on each rejoin attempt */

    ZG->nwk.handle.tmp.rejoin.unsecured_rejoin = ZB_FALSE;

    {
        zb_nwk_rejoin_request_t *rejoin_request;

        rejoin_request = (zb_nwk_rejoin_request_t *)nwk_alloc_and_fill_cmd(param, ZB_NWK_CMD_REJOIN_REQUEST, (zb_uint8_t)sizeof(zb_nwk_rejoin_request_t));
        rejoin_request->capability_information = ZG->nwk.handle.tmp.rejoin.capability_information;

        ZB_NIB().nwk_hub_connectivity = 0;
    }

    /* send rejoin request */
    (void)zb_nwk_init_apsde_data_ind_params(param, ZB_NWK_INTERNAL_REJOIN_CMD_HANDLE);
    ZB_SCHEDULE_CALLBACK(zb_nwk_forward, param);

    /* join response timer will be started inside confirm callback */

    TRACE_MSG(TRACE_NWK1, "<<zb_nlme_rejoin_send_pkt", (FMT__0));
}


void remove_parent_from_potential_parents(zb_ext_neighbor_tbl_ent_t *parent)
{
    if ( parent != NULL )
    {
        zb_address_ieee_ref_t addr_ref;

        /* do not choose this parent again */
        parent->u.ext.potential_parent = 0;

        /* Remove tmp neighbor */
        if ( zb_address_by_short(parent->u.ext.short_addr, ZB_FALSE, ZB_FALSE, &addr_ref) == RET_OK )
        {
            zb_ret_t ret = zb_nwk_neighbor_delete(addr_ref);
            if (ret != RET_OK)
            {
                TRACE_MSG(TRACE_ERROR, "zb_nwk_neighbor_delete failed [%d]", (FMT__D, ret));
            }
        }
    }
}

void zb_nlme_rejoin_response_timeout(zb_uint8_t param)
{
    zb_bool_t stop_rejoin = (zb_bool_t)(ZG->nwk.handle.tmp.rejoin.parent != NULL);

    TRACE_MSG(TRACE_NWK1, ">>zb_nlme_rejoin_response_timeout %hd", (FMT__H, param));

    (void)param;

#ifdef ZB_ED_FUNC
    if (!ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE())
            && stop_rejoin
            && ZG->nwk.handle.tmp.rejoin.poll_attempts < ZB_NWK_REJOIN_POLL_ATTEMPTS)
    {
        nwk_next_rejoin_poll(0);
        ZB_SCHEDULE_ALARM(zb_nlme_rejoin_response_timeout, 0, ZB_NWK_REJOIN_TIMEOUT / ZB_NWK_REJOIN_POLL_ATTEMPTS);
        stop_rejoin = ZB_FALSE;
    }
#endif
    if (stop_rejoin)
    {
        ZG->nwk.handle.tmp.rejoin.poll_attempts = 0;
        ZG->nwk.handle.tmp.rejoin.poll_req = 0;

        /* If we are not authenticated, next rejoin try will be also unsecured.
         * Rationale: in some cases we do not have the key at all and can not do
         * secured rejoin!
         */
        if (!ZG->aps.authenticated)
        {
            ZG->nwk.handle.tmp.rejoin.unsecured_rejoin = ZB_TRUE;
        }

        /* do not choose this parent again */
        remove_parent_from_potential_parents(ZG->nwk.handle.tmp.rejoin.parent);

        {
            /* try to choose another parent and send join request again */
            zb_ret_t ret = zb_buf_get_out_delayed(zb_nlme_rejoin_scan_confirm);
            TRACE_MSG(TRACE_NWK1, "calling zb_nlme_rejoin_scan_confirm", (FMT__0));
            if (ret != RET_OK)
            {
                TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
                ZB_ASSERT(0);
            }
        }
    }

    TRACE_MSG(TRACE_NWK1, "<<zb_nlme_rejoin_response_timeout", (FMT__0));
}


static void zb_nwk_upd_nwk_addr_after_join(zb_uint8_t param)
{
    /* update short address in MAC using MLME-SET, then confirm join ok */
    zb_nwk_pib_set(param, ZB_PIB_ATTRIBUTE_SHORT_ADDRESS, &ZB_PIBCACHE_NETWORK_ADDRESS(), 2, nwk_join_confirm_ok);
}


void zb_nlme_rejoin_response(zb_uint8_t param)
{
    zb_nwk_hdr_t *nwhdr = (zb_nwk_hdr_t *)zb_buf_begin(param);
    zb_nwk_rejoin_response_t *rejoin_response = (zb_nwk_rejoin_response_t *)
            ZB_NWK_CMD_FRAME_GET_CMD_PAYLOAD(param, ZB_NWK_HDR_SIZE(nwhdr));
    zb_bool_t is_joined = ZB_JOINED();
    zb_address_ieee_ref_t addr_ref;
    zb_ieee_addr_t ieee_addr;

    TRACE_MSG(TRACE_NWK1, ">>rejoin_resp %hd", (FMT__H, param));

    ZB_LETOH16_ONPLACE(rejoin_response->network_addr);

    TRACE_MSG(TRACE_NWK1, "rejoin_resp nwk state %hd addr 0x%x",
              (FMT__H_D, ZG->nwk.handle.state, rejoin_response->network_addr));

    /* Response rejoin without request - Resolving Address Conflicts
     * see 3.6.1.9.3
     */
    if (ZB_IS_DEVICE_ZED()
            && ZG->nwk.handle.state == ZB_NLME_STATE_IDLE
            && is_joined
            /* unsolicited rejoin resp - change our short address */
            && rejoin_response->rejoin_status == 0U
            && !ZB_NWK_IS_ADDRESS_BROADCAST(rejoin_response->network_addr)
            /* additionally check by long that it is for us if possible */
            && (ZB_NWK_FRAMECTL_GET_DESTINATION_IEEE(nwhdr->frame_control) == 0U
                || (ZB_NWK_FRAMECTL_GET_DESTINATION_IEEE(nwhdr->frame_control) != 0U
                    && (ZB_IEEE_ADDR_CMP(ZB_PIBCACHE_EXTENDED_ADDRESS(), nwhdr->dst_ieee_addr))))
            /* additionally check that it is from our parent - only parent can send us such rejoin */

            /*cstat !MISRAC2012-Rule-13.5 */
            /* After some investigation, the following violation of the Rule 13.5 seems to be a false
             * positive. The only way this function could have side effects is if the function
             * 'zb_address_by_short()' inside 'zb_address_ieee_by_short()' is called with the
             * second and third parameters ('create' and 'lock', respectively) equal to ZB_TRUE, which is
             * not the case. */
            && ((zb_address_ieee_by_short(nwhdr->src_addr, ieee_addr) == RET_OK)
                /*cstat !MISRAC2012-Rule-13.5 */
                /* After some investigation, the following violation of the Rule 13.5 seems to be a false
                 * positive. The only way this function could have side effects is if the function
                 * 'zb_address_by_ieee()' inside 'zb_nwk_neighbor_get_by_ieee()' inside
                 * 'zb_nwk_get_nbr_rel_by_ieee()' is called with the second and
                 * third parameters ('create' and 'lock', respectively) equal to
                 * ZB_TRUE, which is not the case. */
                && ((zb_nwk_get_nbr_rel_by_ieee(ieee_addr)) == ZB_NWK_RELATIONSHIP_PARENT)))
    {
        TRACE_MSG(TRACE_NWK1,
                  "unsolicited rejoin resp - change our short addr to 0x%x (address conflict "
                  "resolution procedure)",
                  (FMT__D, rejoin_response->network_addr));
        ZB_PIBCACHE_NETWORK_ADDRESS() = rejoin_response->network_addr;
#ifdef NCP_MODE
        ncp_address_update_ind(ZB_PIBCACHE_NETWORK_ADDRESS());
#endif /* NCP_MODE */
        /* TODO: rewrite to use zb_nwk_change_my_addr_conf()? */
        {
            zb_ret_t ret = zb_address_update(ZB_PIBCACHE_EXTENDED_ADDRESS(), ZB_PIBCACHE_NETWORK_ADDRESS(),
                                             ZB_FALSE, &addr_ref);
            ZB_ASSERT(ret == RET_OK);
        }
        ZB_SCHEDULE_CALLBACK(zb_nwk_upd_nwk_addr_after_join, param);
        ZB_NIB().nwk_hub_connectivity = (nwhdr->src_addr == 0U) ? 1U : 0U;
    }
    else if (ZG->nwk.handle.state == ZB_NLME_STATE_REJOIN)
    {
        zb_ieee_addr_t parent_addr;

        ZB_ADDRESS_DECOMPRESS(parent_addr, ZG->nwk.handle.tmp.rejoin.parent->u.ext.long_addr);

        TRACE_MSG(TRACE_NWK1, "status %hd nwk_addr 0x%x",
                  (FMT__H_D, rejoin_response->rejoin_status, rejoin_response->network_addr));
        TRACE_MSG(TRACE_NWK1, "nwhdr->dst_ieee_addr " TRACE_FORMAT_64,
                  (FMT__A, TRACE_ARG_64(nwhdr->dst_ieee_addr)));
        TRACE_MSG(TRACE_NWK1, "nwhdr->src_ieee_addr " TRACE_FORMAT_64,
                  (FMT__A, TRACE_ARG_64(nwhdr->src_ieee_addr)));
        TRACE_MSG(TRACE_NWK1, "mac_extended_address " TRACE_FORMAT_64,
                  (FMT__A, TRACE_ARG_64(ZB_PIBCACHE_EXTENDED_ADDRESS())));
        TRACE_MSG(TRACE_NWK1, "parent_addr " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(parent_addr)));

        /* check if parent answer is correct */
        if (rejoin_response->rejoin_status == (zb_uint8_t)MAC_SUCCESS
                && !ZB_NWK_IS_ADDRESS_BROADCAST(rejoin_response->network_addr)
                && ZB_NWK_FRAMECTL_GET_SOURCE_IEEE(nwhdr->frame_control) != 0U
                && (ZB_NWK_FRAMECTL_GET_DESTINATION_IEEE(nwhdr->frame_control) == 0U
                    || ZB_IEEE_ADDR_CMP(nwhdr->dst_ieee_addr, ZB_PIBCACHE_EXTENDED_ADDRESS()))
                && (ZB_IEEE_ADDR_IS_UNKNOWN(parent_addr)
                    || ZB_IEEE_ADDR_CMP(nwhdr->src_ieee_addr, parent_addr)))
        {
            zb_ieee_addr_t *ieee_addr_ptr;

            /* Remember this device long addr + short addr in the addr table. */
            (void)zb_address_update(ZB_PIBCACHE_EXTENDED_ADDRESS(),
                                    rejoin_response->network_addr,
                                    ZB_TRUE,
                                    &addr_ref);

            ieee_addr_ptr = zb_nwk_get_src_long_from_hdr(nwhdr);
            (void)zb_address_update(*ieee_addr_ptr,
                                    ZG->nwk.handle.tmp.rejoin.parent->u.ext.short_addr,
                                    ZB_FALSE,
                                    &ZG->nwk.handle.parent);

            /* save assigned address */
            ZB_PIBCACHE_NETWORK_ADDRESS() = rejoin_response->network_addr;

            ZB_SET_JOINED_STATUS(ZB_TRUE);

            /* see 3.6.1.4.1.1:

               The network depth is set to one more than the parent network depth
               unless the parent network depth has a value of 0x0f, i.e. the maximum
               value for the 4-bit device depth field in the beacon payload. In this
               case, the network depth shall also be set to 0x0f.
            */
#ifdef ZB_ROUTER_ROLE
#ifdef ZB_NWK_DISTRIBUTED_ADDRESS_ASSIGN
            ZB_NIB_DEPTH() = ZG->nwk.handle.tmp.rejoin.parent->depth + (ZB_NIB_DEPTH() != 0xf);
            /* Calculate CScip value for tree routing */
            ZB_NIB().cskip = zb_nwk_daa_calc_cskip(ZB_NIB_DEPTH());
#else
            /* ZB_NIB_DEPTH() = ZG->nwk.handle.tmp.rejoin.parent->depth + 1; */
            if (ZB_NIB_DEPTH() != 0x0FU)
            {
                ZB_NIB_DEPTH() = ZG->nwk.handle.tmp.rejoin.parent->depth + 1U;
            }
#endif
#endif

            ZB_EXTPANID_COPY(ZB_NIB_EXT_PAN_ID(), ZG->nwk.handle.tmp.rejoin.extended_pan_id);

            /* extended neighbor table is useless now */
            zb_nwk_exneighbor_stop(ZG->nwk.handle.tmp.rejoin.parent->u.ext.short_addr);
            ZG->nwk.handle.state = ZB_NLME_STATE_IDLE;

            {
                zb_neighbor_tbl_ent_t *nent;

                if (zb_nwk_neighbor_get(ZG->nwk.handle.parent, ZB_FALSE, &nent) == RET_OK)
                {
                    nent->relationship = ZB_NWK_RELATIONSHIP_PARENT;
#ifdef ZB_USE_NVRAM
                    zb_nvram_store_addr_n_nbt();
#endif
                }
                else
                {
                    TRACE_MSG(TRACE_ERROR, "Oops: no our parent in the neighbor table!", (FMT__0));
                }
            }

            if (ZB_IS_DEVICE_ZED())
            {
                /* If joined as ED, make sure beacon payload length is set to 0, so MAC
                 * does not answer on beacon requests */
                zb_uint8_t len = 0;
                zb_nwk_pib_set(param, ZB_PIB_ATTRIBUTE_BEACON_PAYLOAD_LENGTH, &len, 1, zb_nwk_upd_nwk_addr_after_join);
            }
            else
            {
                ZB_SCHEDULE_CALLBACK(zb_nwk_upd_nwk_addr_after_join, param);

                ZB_NIB().nwk_hub_connectivity = (zb_address_short_by_ieee(parent_addr) == 0U) ? 1U : 0U;
            }
        }
        else
        {
            TRACE_MSG(TRACE_NWK1, "bad rejoin resp - ignore", (FMT__0));

            /* do not choose this parent again */
            remove_parent_from_potential_parents(ZG->nwk.handle.tmp.rejoin.parent);

            /* try to choose another parent and send join request again */
            ZB_SCHEDULE_CALLBACK(zb_nlme_rejoin_scan_confirm, param);
        }

        /* cancel rejoin timeout */
        ZB_SCHEDULE_ALARM_CANCEL(zb_nlme_rejoin_response_timeout, 0);
    }
    else
    {
        TRACE_MSG(TRACE_NWK1, "got rejoin resp, state differs - drop", (FMT__0));
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_NWK1, "<<rejoin_resp", (FMT__0));
}

#ifndef ZB_LITE_NO_ORPHAN_SCAN
static void zb_nlme_orphan_scan(zb_uint8_t param);
#endif  /* ZB_LITE_NO_ORPHAN_SCAN */

void zb_nlme_join_request(zb_uint8_t param)
{
    zb_uint8_t i;
    zb_nlme_join_request_t *request = ZB_BUF_GET_PARAM(param, zb_nlme_join_request_t);
    zb_ret_t ret;
    zb_uint8_t scan_iface_idx;
    zb_uint8_t channel_page;
    zb_uint32_t channel_mask;
    zb_uint8_t channels_num = 0;

    TRACE_MSG(TRACE_NWK1, ">>zb_nlme_join_request %hd", (FMT__H, param));
    CHECK_PARAM_RET_ON_ERROR(request);

    ZG->nwk.handle.router_started = ZB_FALSE;
    ZB_SET_PARENT_INFO(0);

    ZG->nwk.handle.rejoin_capability_alloc_address = ZB_U2B(ZB_MAC_CAP_GET_ALLOCATE_ADDRESS(request->capability_information));

    /* Validate the ChannelListStructure according to r22 section 3.2.2.2.2. */
    ret = nwk_scan_find_next_channel_mask(0, 0, request->scan_channels_list, &scan_iface_idx,
                                          &channel_page, &channel_mask);
    if (ret == RET_OK)
    {
        switch ( request->rejoin_network )
        {
        case ZB_NLME_REJOIN_METHOD_ASSOCIATION:
            /* On receipt of this primitive by a device that is not currently joined
               to a network and with the RejoinNetwork parameter equal to 0x00, the
               device attempts to join the network specified by the 64-bit
               ExtendedPANId parameter as described in sub-clause3.6.1.4.1.1. */

            TRACE_MSG(TRACE_NWK1, "ZB_NLME_REJOIN_METHOD_ASSOCIATION", (FMT__0));
            /* #AT: save capability info from join request */
            ZG->nwk.handle.tmp.rejoin.saved_join_req.capability_information = request->capability_information;

            /* Verify that just single channel has been selected (see r22 sub-clause 3.2.2.13.3). */
            for (i = 0; i < ZB_CHANNEL_PAGES_NUM; i++)
            {
                channels_num += zb_channel_page_list_get_channels_num(request->scan_channels_list, i);
            }

            TRACE_MSG(TRACE_NWK3, "channels_num %hd", (FMT__H, channels_num));
            if (channels_num == 1U)
            {
                ret = nwk_association_join(param, ZB_BUF_GET_PARAM(param, zb_nlme_join_request_t));
            }
            else
            {
                ret = ERROR_CODE(ERROR_CATEGORY_NWK, ZB_NWK_STATUS_INVALID_PARAMETER);
            }
            if (ret != (zb_ret_t)ZB_NWK_STATUS_SUCCESS)
            {
                nwk_join_failure_confirm(param, ret);
                ret = 0;
            }
            break;

        case ZB_NLME_REJOIN_METHOD_DIRECT:
#ifndef ZB_LITE_NO_ORPHAN_SCAN
            zb_nlme_orphan_scan(param);
#endif
            break;

        case ZB_NLME_REJOIN_METHOD_REJOIN:
            zb_nlme_rejoin(param);
            break;

        case ZB_NLME_REJOIN_METHOD_CHANGE_CHANNEL:
            TRACE_MSG(TRACE_ERROR, "Change channel not impl", (FMT__0));
            break;

        default:
            break;
        }
    }
    else
    {
        nwk_join_failure_confirm(param, ERROR_CODE(ERROR_CATEGORY_NWK, ZB_NWK_STATUS_INVALID_PARAMETER));
    }

    ZVUNUSED(ret);

    TRACE_MSG(TRACE_NWK1, "<<zb_nlme_join_request", (FMT__0));
}

#ifndef ZB_LITE_NO_ORPHAN_SCAN
static void zb_nlme_orphan_scan(zb_uint8_t param)
{
    zb_ret_t ret;
    zb_nlme_join_request_t *request = ZB_BUF_GET_PARAM(param, zb_nlme_join_request_t);
    zb_uint8_t scan_iface_idx;
    zb_uint8_t channel_page;
    zb_uint32_t channel_mask;

    TRACE_MSG(TRACE_NWK1, ">> orphan_scan prm %hd", (FMT__H, param));

    if ( ZG->nwk.handle.state == ZB_NLME_STATE_IDLE )
    {
        /* Start discovery using the first supported page. */
        ret = nwk_scan_find_next_channel_mask(0, 0, request->scan_channels_list, &scan_iface_idx,
                                              &channel_page, &channel_mask);
        if (ret == RET_OK)
        {
            /* Make a copy to use it for further scan request sequence. */
            zb_channel_page_list_copy(ZG->nwk.handle.scan_channels_list, request->scan_channels_list);
            /* perform orphan scan */
            ZG->nwk.handle.state = ZB_NLME_STATE_ORPHAN_SCAN;
            nlme_scan_request(param, ORPHAN_SCAN, request->scan_duration,
                              scan_iface_idx, channel_page, channel_mask);
        }
        else
        {
            TRACE_MSG(TRACE_NWK1, "Could not get channels mask to start orphan scan!", (FMT__0));
            /* SS: TODO: Cleanup status codes!!!!!*/
            nwk_join_failure_confirm(param, ERROR_CODE(ERROR_CATEGORY_NWK, ZB_NWK_STATUS_INVALID_PARAMETER));
        }
    }
    else
    {
        TRACE_MSG(TRACE_NWK1, "nwk is busy, state %d", (FMT__D, ZG->nwk.handle.state));
        nwk_join_failure_confirm(param, ERROR_CODE(ERROR_CATEGORY_NWK, ZB_NWK_STATUS_NOT_PERMITTED));
    }

    TRACE_MSG(TRACE_NWK1, "<< orphan_scan", (FMT__0));
}

void zb_nlme_orphan_scan_confirm(zb_uint8_t param)
{
    zb_mac_scan_confirm_t *confirm = ZB_BUF_GET_PARAM(param, zb_mac_scan_confirm_t);
    zb_mac_status_t status = confirm->status;

    TRACE_MSG(TRACE_NWK1, ">> orphan_scan_cnfrm param %hd", (FMT__H, param));

    nwk_join_failure_confirm(param, (status == MAC_SUCCESS) ?
                             (zb_ret_t)ZB_NWK_STATUS_SUCCESS :
                             ERROR_CODE(ERROR_CATEGORY_NWK, ZB_NWK_STATUS_NO_NETWORKS));

    ZG->nwk.handle.state = ZB_NLME_STATE_IDLE;

    TRACE_MSG(TRACE_NWK1, "<< orphan_scan_cnfrm", (FMT__0));
}
#endif  /* ZB_LITE_NO_ORPHAN_SCAN */

#endif  /* ZB_JOIN_CLIENT */

/*! @} */
