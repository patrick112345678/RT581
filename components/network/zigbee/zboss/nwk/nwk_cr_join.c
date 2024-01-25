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

#define ZB_TRACE_FILE_ID 318
#include "zb_common.h"
#include "zboss_api_zdo.h"
#include "zb_scheduler.h"
#include "zb_mac.h"
#include "zb_nwk.h"
#include "zb_secur.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "nwk_internal.h"
#include "zb_magic_macros.h"
#include "zb_bufpool.h"
/*! \addtogroup ZB_NWK */
/*! @{ */



#ifdef ZB_ROUTER_ROLE
static zb_mac_status_t zb_nwk_accept_child(zb_ieee_addr_t device_address, zb_mac_capability_info_t capability, zb_uint8_t lqi, zb_uint16_t *address);

static void zb_send_device_associated_signal(zb_uint8_t param);

void zb_mlme_associate_indication_cont(zb_uint8_t param);

#if !defined ZB_LITE_NO_INDIRECT_QUEUE_PURGE

static void zb_nwk_purge_indirect_queue(zb_uint8_t param, zb_uint8_t type,
                                        zb_uint16_t short_addr, zb_ieee_addr_t ieee_addr)
{
    zb_mcps_purge_indir_q_req_t *req =
        ZB_BUF_GET_PARAM(param, zb_mcps_purge_indir_q_req_t);

    TRACE_MSG(TRACE_NWK1, ">> zb_nwk_purge_indirect_queue param %hd, type %hd",
              (FMT__H_H, param, type));

    req->type = type;
    req->short_addr = short_addr;
    ZB_IEEE_ADDR_COPY(req->ieee_addr, ieee_addr);

    ZB_SCHEDULE_CALLBACK(zb_mcps_purge_indirect_queue_request, param);

    TRACE_MSG(TRACE_NWK1, "<< zb_nwk_purge_indirect_queue", (FMT__0));
}

void zb_mcps_purge_indirect_queue_confirm(zb_uint8_t param)
{
    zb_mcps_purge_indir_q_conf_t *conf =
        ZB_BUF_GET_PARAM(param, zb_mcps_purge_indir_q_conf_t);

    TRACE_MSG(TRACE_NWK1, ">> zb_mcps_purge_indirect_queue_confirm param %hd, type %hd", (FMT__H_H, param, conf->type));

    if (conf->status == MAC_SUCCESS)
    {
        switch (conf->type)
        {
        case ZB_MCPS_INDIR_Q_PURGE_TYPE_NONE:
            /* No action needed */
            break;

        case ZB_MCPS_INDIR_Q_PURGE_TYPE_ASSOCIATION:
        {
            zb_uint16_t short_addr;
            zb_mlme_associate_indication_t *indication_param =
                ZB_BUF_GET_PARAM(param, zb_mlme_associate_indication_t);
            zb_mlme_associate_indication_t *indication_body =
                (zb_mlme_associate_indication_t *)zb_buf_begin(param);

            short_addr = zb_address_short_by_ieee(indication_body->device_address);
            if (short_addr != ZB_UNKNOWN_SHORT_ADDR)
            {
                zb_aps_clear_after_leave(short_addr);
            }

            ZB_MEMCPY(indication_param, indication_body, sizeof(zb_mlme_associate_indication_t));
            ZB_SCHEDULE_CALLBACK(zb_mlme_associate_indication_cont, param);
            break;
        }

        case ZB_MCPS_INDIR_Q_PURGE_TYPE_REJOIN:
        {
            /* parameter refers to buffer with Rejoin Request - use address
             * information to purge APS retransimission table */
            zb_nwk_hdr_t *nwk_hdr = (zb_nwk_hdr_t *)zb_buf_begin(param);

            zb_aps_clear_after_leave(nwk_hdr->src_addr);

            ZB_SCHEDULE_CALLBACK(zb_nlme_rejoin_request_handler, param);
            break;
        }

        default:
            TRACE_MSG(TRACE_ERROR, "Unknown type %hd", (FMT__H, conf->type));
            ZB_ASSERT(0);
            break;
        }
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Error during MAC indirect Q purge", (FMT__0));
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_NWK1, "<< zb_mcps_purge_indirect_queue_confirm", (FMT__0));
}

#endif  /* ZB_LITE_NO_INDIRECT_QUEUE_PURGE */

/* Designed to clear all MAC Q and other APS entries, and other stuff when
 * received Rejoin Request from pending device */
void zb_nlme_rejoin_request_pre_handler(zb_uint8_t param)
{
    zb_nwk_hdr_t *nwk_hdr = (zb_nwk_hdr_t *)zb_buf_begin(param);
    zb_ieee_addr_t *ieee_addr;

    TRACE_MSG(TRACE_NWK1, ">> zb_nlme_rejoin_request_pre param %hd", (FMT__H, param));
#ifdef ZB_TH_ENABLED
    if (TH_CTX().options.block_rejoin_request)
    {
        TRACE_MSG(TRACE_ZCL1, ">>Rejoin request handler is blocked by implementation specific meaning", (FMT__0));
        zb_buf_free(param);
        return;
    }
#endif
    if ( ZG->nwk.handle.rejoin_req_table_cnt < ZB_NWK_REJOIN_REQUEST_TABLE_SIZE
            && ZB_U2B(ZB_NWK_FRAMECTL_GET_SOURCE_IEEE(nwk_hdr->frame_control)))
    {
        ieee_addr = zb_nwk_get_src_long_from_hdr(nwk_hdr);

#if !defined ZB_LITE_NO_INDIRECT_QUEUE_PURGE
        zb_nwk_purge_indirect_queue(param, ZB_MCPS_INDIR_Q_PURGE_TYPE_REJOIN,
                                    nwk_hdr->src_addr, *ieee_addr);
#else
        ZB_SCHEDULE_CALLBACK(zb_nlme_rejoin_request_handler, param);
#endif
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "rejoin req tbl is full (%hd) or bad request - drop req",
                  (FMT__H, ZG->nwk.handle.rejoin_req_table_cnt));
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_NWK1, "<< zb_nlme_rejoin_request_pre", (FMT__0));
}

void zb_nlme_rejoin_request_handler(zb_uint8_t param)
{
    zb_ret_t processing_ret = RET_OK;
    zb_nwk_hdr_t *nwhdr = (zb_nwk_hdr_t *)zb_buf_begin(param);
    /* If the end device performs unsecured rejoin. */
#ifdef ZB_TEMPORARY_UNSEC_REJOIN_FIX
    zb_address_ieee_ref_t ref_p;
#endif

    TRACE_MSG(TRACE_NWK1, ">> zb_nlme_rejoin_request %hd", (FMT__H, param));

    if ( ZG->nwk.handle.rejoin_req_table_cnt >= ZB_NWK_REJOIN_REQUEST_TABLE_SIZE
            /* || ZB_NWK_FRAMECTL_GET_DESTINATION_IEEE(nwhdr->frame_control) */
            || !ZB_U2B(ZB_NWK_FRAMECTL_GET_SOURCE_IEEE(nwhdr->frame_control)) )
    {
        TRACE_MSG(TRACE_ERROR, "rejoin req tbl is full (%hd) or bad request - drop req",
                  (FMT__H, ZG->nwk.handle.rejoin_req_table_cnt));
        processing_ret = RET_ERROR;
    }

    if (processing_ret == RET_OK)
    {
        zb_nwk_rejoin_request_t *rejoin_request = (zb_nwk_rejoin_request_t *)
                ZB_NWK_CMD_FRAME_GET_CMD_PAYLOAD(param, ZB_NWK_HDR_SIZE(nwhdr));
        zb_mac_status_t status;
        //zb_uint16_t address = (zb_uint16_t)~0;
        zb_uint16_t address = nwhdr->src_addr;
        zb_nwk_rejoin_response_t *rejoin_response;
        zb_ieee_addr_t dst_ieee_addr;
        zb_ieee_addr_t *src_ieee_addr;
        zb_uint16_t dst_addr;
        zb_bool_t rejoin_is_in_progress = ZB_FALSE;
        zb_bool_t secure = ZB_U2B(ZB_NWK_FRAMECTL_GET_SECURITY(nwhdr->frame_control));
        zb_address_ieee_ref_t addr_ref;
        zb_ret_t ret;

        /* If request was secured, response secured also - see fig. 2.108 2.5.5.4.1 */

        src_ieee_addr = zb_nwk_get_src_long_from_hdr(nwhdr);

        if (!ZB_IEEE_ADDR_IS_VALID(src_ieee_addr))
        {
            TRACE_MSG(TRACE_ERROR, "Drop rejoin request with zero or unknown source IEEE address", (FMT__0));
            processing_ret = RET_ERROR;
        }

        if (processing_ret == RET_OK)
        {
            ZB_IEEE_ADDR_COPY(dst_ieee_addr, *src_ieee_addr);
            ret = zb_address_by_ieee(*src_ieee_addr, ZB_FALSE, ZB_FALSE, &addr_ref);

            /* NK: disable rejoin procedure if another rejoin for this device is in progress */
            if (ret == RET_OK)
            {
                zb_uint8_t i;

                for (i = 0 ; i < ZG->nwk.handle.rejoin_req_table_cnt ; ++i)
                {
                    if (addr_ref == ZG->nwk.handle.rejoin_req_table[i].addr_ref)
                    {
                        TRACE_MSG(TRACE_NWK3, "device " TRACE_FORMAT_64 " already performs another rejoin, deny",
                                  (FMT__A, TRACE_ARG_64(*src_ieee_addr)));
                        rejoin_is_in_progress = ZB_TRUE;

                        break;
                    }
                }
            }

            if ( ZB_U2B(ZB_NIB().disable_rejoin) ||                   // disable rejoin or
                    /* According to TC TP_R21_BV-07 must not reject now, but do not send
                      * nwk key */
                    rejoin_is_in_progress                         // another rejoin is in progress
               )
            {
                TRACE_MSG(TRACE_NWK3, "rejoin req: access denied", (FMT__0));
                status = MAC_PAN_ACCESS_DENIED;
            }
            else
            {
                status = zb_nwk_accept_child(*src_ieee_addr, rejoin_request->capability_information, ZB_MAC_GET_LQI(param), &address);
            }
            TRACE_MSG(TRACE_NWK3, "status %d address assigned 0x%x", (FMT__D_D, status, address));
            dst_addr = nwhdr->src_addr;
            /* MZ: Update the address if the end device performs unsecured
            * rejoin. */
#ifdef ZB_TEMPORARY_UNSEC_REJOIN_FIX
            zb_address_update(*src_ieee_addr, nwhdr->src_addr, 0, &ref_p);
#endif

            TRACE_MSG(TRACE_NWK1, "zb_nlme_rejoin_request src 0x%x = " TRACE_FORMAT_64,
                      (FMT__D_A, ZB_PIBCACHE_NETWORK_ADDRESS(), TRACE_ARG_64(ZB_PIBCACHE_EXTENDED_ADDRESS())));
            TRACE_MSG(TRACE_NWK1, "zb_nlme_rejoin_request dest 0x%x = " TRACE_FORMAT_64,
                      (FMT__D_A, dst_addr, TRACE_ARG_64(dst_ieee_addr)));

            nwhdr = nwk_alloc_and_fill_hdr(param, ZB_PIBCACHE_NETWORK_ADDRESS(), dst_addr, ZB_FALSE, secure, ZB_TRUE, ZB_TRUE);
            if ( secure )
            {
                nwk_mark_nwk_encr(param);
                TRACE_MSG(TRACE_NWK1, "rejoin req was secured, reply secured also", (FMT__0));
            }
            rejoin_response = (zb_nwk_rejoin_response_t *)nwk_alloc_and_fill_cmd(param, ZB_NWK_CMD_REJOIN_RESPONSE, (zb_uint8_t)sizeof(zb_nwk_rejoin_response_t));

            rejoin_response->network_addr = address;

            ZB_NWK_ADDR_TO_LE16(rejoin_response->network_addr);
            rejoin_response->rejoin_status = (zb_uint8_t)status;

            /* transmit rejoin packet */
            TRACE_MSG(TRACE_ATM1, "Z< send rejoin response", (FMT__0));
            (void)zb_nwk_init_apsde_data_ind_params(param, (status == MAC_SUCCESS) ?
                                                    ZB_NWK_INTERNAL_REJOIN_CMD_RESPONSE :
                                                    ZB_NWK_INTERNAL_NSDU_HANDLE);
            ZB_SCHEDULE_CALLBACK(zb_nwk_forward, param);

            /* We should call zb_nlme_join_indication after successful join, save address */
            if ( status == MAC_SUCCESS )
            {
                /* Save network address and allocated address from our address
                * translation table */
                ZG->nwk.handle.rejoin_req_table[ZG->nwk.handle.rejoin_req_table_cnt].addr = address;
                ret = zb_address_by_ieee(dst_ieee_addr, ZB_FALSE, ZB_TRUE,
                                         &ZG->nwk.handle.rejoin_req_table[ZG->nwk.handle.rejoin_req_table_cnt].addr_ref);
                if (ret != RET_OK)
                {
                    TRACE_MSG(TRACE_ERROR, "Failed zb_address_by_ieee() [%d]", (FMT__D, ret));
                }

                TRACE_MSG(TRACE_NWK1, "rejoin_req_table[%hd] = 0x%x", (FMT__H_D, ZG->nwk.handle.rejoin_req_table_cnt, address));
                ZG->nwk.handle.rejoin_req_table[ZG->nwk.handle.rejoin_req_table_cnt].secure_rejoin = ZB_B2U(secure);
                ZG->nwk.handle.rejoin_req_table_cnt++;
            }
        }
    }

    if (processing_ret != RET_OK)
    {
        zb_buf_free(param);
    }

    ZVUNUSED(nwhdr);

    TRACE_MSG(TRACE_NWK1, "<< zb_nlme_rejoin_request", (FMT__0));
}

void zb_nlme_rejoin_resp_sent(zb_uint8_t param)
{
    zb_mcps_data_confirm_params_t *confirm = ZB_BUF_GET_PARAM(param, zb_mcps_data_confirm_params_t);
    zb_uint16_t dest_addr;
    zb_address_ieee_ref_t addr_ref = {0};
    zb_ushort_t i;
    zb_neighbor_tbl_ent_t *nent;

    TRACE_MSG(TRACE_NWK1, ">>zb_nlme_rejoin_resp_sent %hd", (FMT__H, param));

    /* TODO: rewrite excluding rejoin_req_table. All enough information is in
     * rejoin response packet. Parse it. */

    /* This is "old" address - we just sent to it */
    ZB_LETOH16((zb_uint8_t *)&dest_addr, (zb_uint8_t *)&confirm->dst_addr.addr_short);

    for (i = 0 ; i < ZG->nwk.handle.rejoin_req_table_cnt ; ++i)
    {
        /* address translation table still keeps "old" address - search by it */
        if (zb_address_by_short(dest_addr, ZB_FALSE, ZB_FALSE, &addr_ref) == RET_OK
                && addr_ref == ZG->nwk.handle.rejoin_req_table[i].addr_ref)
        {
            break;
        }
    }

    if (i < ZG->nwk.handle.rejoin_req_table_cnt)
    {
        zb_nlme_join_indication_t *resp = ZB_BUF_GET_PARAM(param, zb_nlme_join_indication_t);

        if (zb_buf_get_status(param) == (zb_int16_t)MAC_SUCCESS)
        {
            zb_ret_t ret;

            zb_address_ieee_by_ref(resp->extended_address, addr_ref);
            resp->network_address = ZG->nwk.handle.rejoin_req_table[i].addr;
            TRACE_MSG(TRACE_ATM1, "Z< rejoined device: 0x%x = "TRACE_FORMAT_64,
                      (FMT__D_A, ZG->nwk.handle.rejoin_req_table[i].addr, TRACE_ARG_64(resp->extended_address)));
            TRACE_MSG(TRACE_INFO1, "rejoined old 0x%x new 0x%x = "TRACE_FORMAT_64,
                      (FMT__D_D_A,
                       dest_addr, ZG->nwk.handle.rejoin_req_table[i].addr, TRACE_ARG_64(resp->extended_address)));
            ret = zb_address_update(resp->extended_address, resp->network_address,
                                    ZB_FALSE, &addr_ref);
            ZB_ASSERT(ret == RET_OK);

            resp->secure_rejoin = ZG->nwk.handle.rejoin_req_table[i].secure_rejoin;

            resp->capability_information = 0;
            if (zb_nwk_neighbor_get(addr_ref, ZB_FALSE, &nent) == RET_OK)
            {
                /* Only that 2 fields of capability information are meaningful */
                ZB_MAC_CAP_SET_DEVICE_TYPE(resp->capability_information, ZB_B2U(nent->device_type == ZB_NWK_DEVICE_TYPE_ROUTER));
                ZB_MAC_CAP_SET_RX_ON_WHEN_IDLE(resp->capability_information, nent->rx_on_when_idle);
                resp->rejoin_network = ZB_NLME_REJOIN_METHOD_REJOIN;

                zb_buf_set_status(param, (zb_int16_t)ZB_NWK_STATUS_SUCCESS);

#if defined ZB_MAC_PENDING_BIT_SOURCE_MATCHING
                if (nent->device_type == ZB_NWK_DEVICE_TYPE_ED)
                {
                    zb_uint8_t nbt_idx = (zb_uint8_t)ZB_NWK_NEIGHBOR_GET_INDEX_BY_ENTRY_ADDRESS(nent);

                    zb_ret_t ret_src_match_add = zb_buf_get_out_delayed_ext(zb_nwk_src_match_add, nbt_idx, 0);
                    if (ret_src_match_add != RET_OK)
                    {
                        TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed_ext [%d]", (FMT__D, ret_src_match_add));
                    }
                }
#endif /* ZB_MAC_PENDING_BIT_SOURCE_MATCHING */

                /* Pass indication to AF. It will free the buffer. */
                ZB_SCHEDULE_CALLBACK(zb_nlme_join_indication, param);


                ZG->nwk.handle.run_after_update_beacon_payload = NULL;

                /* Network configuration has changed - change update id. */
                TRACE_MSG(TRACE_ZDO1, "schedule zb_nwk_update_beacon_payload", (FMT__0));
                if (zb_buf_get_out_delayed(zb_nwk_update_beacon_payload) != RET_OK)
                {
                    TRACE_MSG(TRACE_ERROR, "Oops - out of memory, can't update beacon paypoad", (FMT__0));
                }
            }
            else
            {
                TRACE_MSG(TRACE_NWK1, "warning: cant get device from neighbor table, exit", (FMT__0));
                zb_buf_free(param);
            }
        }
        else
        {
            TRACE_MSG(TRACE_NWK1, "warning: rejoin resp hasn't been sent status 0x%x", (FMT__D, zb_buf_get_status(param)));
            zb_buf_free(param);
        }

        zb_address_unlock(ZG->nwk.handle.rejoin_req_table[i].addr_ref);
        ZG->nwk.handle.rejoin_req_table_cnt--;
        /* NK: Remove unused element with MEMMOVE if needed */
        if (ZG->nwk.handle.rejoin_req_table_cnt > i) /* it was not last element */
        {
            ZB_MEMMOVE(&ZG->nwk.handle.rejoin_req_table[i], &ZG->nwk.handle.rejoin_req_table[i + 1U],
                       (ZG->nwk.handle.rejoin_req_table_cnt - i) * sizeof(zb_rejoin_context_t));
        }
    }
    else
    {
        TRACE_MSG(TRACE_NWK1, "should never happen", (FMT__0));
        zb_buf_free(param);
    }


    TRACE_MSG(TRACE_NWK1, "<<zb_nlme_rejoin_resp_sent", (FMT__0));
}


static zb_mac_status_t zb_nwk_accept_child(zb_ieee_addr_t device_address, zb_mac_capability_info_t capability, zb_uint8_t lqi, zb_uint16_t *address)
{
    zb_ret_t ret;
    zb_mac_status_t status = MAC_SUCCESS;
    zb_neighbor_tbl_ent_t *ent = NULL;
    zb_uint16_t addr = ZB_MAC_SHORT_ADDR_NOT_ALLOCATED;
    zb_bool_t allocate_addr = ZB_U2B(ZB_MAC_CAP_GET_ALLOCATE_ADDRESS(capability));

    TRACE_MSG(TRACE_NWK1, ">>zb_nwk_accept_child device_address " TRACE_FORMAT_64 " short 0x%hx lqi %hd",
              (FMT__A_H_H, TRACE_ARG_64(device_address), *address, lqi));

    /* 3.6.1.4.1.2
       Search in the neighbor table. If device with this ieee found then check device
       capabilities. If they are equal then obtain the corresponding network
       address and associate it, else remove record from table and continue association. */
    if ( zb_nwk_neighbor_get_by_ieee(device_address, &ent) == RET_OK && status == MAC_SUCCESS)
    {
        if (
            /* Device type match. Don't compare for Coordinator: joining device can't be coordinator */
            (((ent->device_type == ZB_NWK_DEVICE_TYPE_ROUTER) && ZB_U2B(ZB_MAC_CAP_GET_DEVICE_TYPE(capability))) ||
             ((ent->device_type == ZB_NWK_DEVICE_TYPE_ED) && !ZB_U2B(ZB_MAC_CAP_GET_DEVICE_TYPE(capability))) ||
             (ent->device_type == ZB_NWK_DEVICE_TYPE_NONE)
            )
            /* idle match */
            && (ZB_MAC_CAP_GET_RX_ON_WHEN_IDLE(capability) == ent->rx_on_when_idle
                /* If our knowledge is incomplete, rx capability may not match */
                || ent->device_type == ZB_NWK_DEVICE_TYPE_NONE)
        )
        {
            zb_address_short_by_ref(&addr, ent->u.base.addr_ref);
            /* CR: 12/02/2015 [DT]: making sure that entry is not reused
             * the "used" field will be set is neighbor table entry for the device is created from scratch */
            ent->used = ZB_TRUE;
            /* Clear Broadcast transaction records for the re-associated device */
            ZB_NWK_CLEAR_BTR_FOR_ADDRESS(ent->u.base.addr_ref);
            TRACE_MSG(TRACE_NWK3, "found dev %hd short 0x%x in neighb tbl", (FMT__H_D, ent->u.base.addr_ref, addr));
        }
        else
        {
            TRACE_MSG(TRACE_NWK3, "rm dev %hd from neighb tbl: diff caps; relationship %hd", (FMT__H_H, ent->u.base.addr_ref, ent->relationship));
            ent = NULL;
        }
    }
#ifdef ZB_NWK_DISTRIBUTED_ADDRESS_ASSIGN
    /* Check the device capabilities and if we have room for this child type  */
    TRACE_MSG(TRACE_NWK1, "cap_inf 0x%x ed_ch_num %hd router_ch_num %hd max_ch %hd max_routers %hd",
              (FMT__D_H_H_H_H,
               (zb_uint16_t)capability,
               (zb_uint8_t)ZB_NIB().ed_child_num, (zb_uint8_t)ZB_NIB().router_child_num,
               (zb_uint8_t)ZB_NIB().max_children, (zb_uint8_t)ZB_NIB().max_routers));
#else
    /* Check the device capabilities and if we have room for this child type  */
    TRACE_MSG(TRACE_NWK1, "cap_inf 0x%x ed_ch_num %hd router_ch_num %hd max_ch %hd",
              (FMT__D_H_H_H,
               (zb_uint16_t)capability,
               (zb_uint8_t)ZB_NIB().ed_child_num, (zb_uint8_t)ZB_NIB().router_child_num,
               (zb_uint8_t)ZB_NIB().max_children));
#endif


    if ((!allocate_addr || ent != NULL) && status == MAC_SUCCESS)
    {
        /* b) for 2007 check for our addresses diapason */
#ifdef ZB_NWK_DISTRIBUTED_ADDRESS_ASSIGN
        if (ZB_MAC_CAP_GET_DEVICE_TYPE(capability))
        {
            if (!ZB_NWK_DISTRIBUTED_ROUTER_ADDRESS_VERIFY(*address))
            {
                allocate_addr = ZB_TRUE;
                ent = NULL;
            }
        }
        else if (!ZB_NWK_DISTRIBUTED_ED_ADDRESS_VERIFY(*address))
        {
            allocate_addr = ZB_TRUE;
            ent = NULL;
        }
#endif

        /*   AD: We SHOULD accept device with the same ieee. It's assumed that there could be no different */
        /*    devices with the same IEEE. Check 3.6.1.4.1.2 Parent Procedure*/
        /*    If an extended address match is   */
        /*    found, the NLME shall check that  the supplied DeviceCapabilities match the   */
        /*    device type on record in the neighbor table. If the device type also matches the  */
        /*    NLME, it shall then obtain the corresponding 16-bit network address and issue an  */
        /*    association response to the MAC sub-layer.  */

        TRACE_MSG(TRACE_NWK1, "allocate_addr %hd", (FMT__H, allocate_addr));
        /* We are here at rejoin, so may not check for child # limits */
        /* Count joiner as child only if it not our neighbor or not a child */
        if (ent == NULL || (ent->relationship != ZB_NWK_RELATIONSHIP_CHILD &&
                            ent->relationship != ZB_NWK_RELATIONSHIP_UNAUTHENTICATED_CHILD))
        {
            if (ZB_U2B(ZB_MAC_CAP_GET_DEVICE_TYPE(capability)))
            {
                ZB_NIB().router_child_num++;
            }
            else
            {
                ZB_NIB().ed_child_num++;
            }
        }
    }

    if (ent == NULL && status == MAC_SUCCESS)
    {
        status = MAC_OUT_OF_CAP;
        if ((ZB_NIB().max_children > ZB_NIB().ed_child_num + ZB_NIB().router_child_num)
#ifndef ZB_PRO_STACK
                && (ZB_NIB_DEPTH() < ZB_NIB().max_depth)
#endif
           )
        {
            if (!allocate_addr)
            {
                /* a) check for conflicts in the address translation table */
                zb_ieee_addr_t ieeea;
                if (zb_address_ieee_by_short(*address, ieeea) == RET_OK
                        && !ZB_IEEE_ADDR_CMP(ieeea, device_address))
                {
                    TRACE_MSG(TRACE_NWK1, "found this short address for another device - force address allocate", (FMT__0));
                    allocate_addr = ZB_TRUE;
                }
            }

            /* check child device is a router and we have room for it */
            if (ZB_U2B(ZB_MAC_CAP_GET_DEVICE_TYPE(capability)))
            {
                /* TODO: refactor to remove copy-paste! */
#ifdef ZB_NWK_DISTRIBUTED_ADDRESS_ASSIGN
                if (ZB_NIB().router_child_num < ZB_NIB().max_routers)
#endif
                {
                    /*
                      Table 3.47 Capability Information Bit-Fields (Continued)

                      7 Allocate address This field will have a value of 1 in
                      implementations of this specification, indicating that the joining
                      device must be issued a 16-bit network address, except in the case
                      where a device has self-selected its address while using the NWK
                      rejoin command to join a network for the first time in a secure
                      manner. In this case, it shall have a value of 0.

                     */

                    /*
                      If this bit is 1, allocate address.
                      Else check that we can keep old address and, if can't, allocate it.
                     */
                    if (allocate_addr)
                    {
#ifdef ZB_PRO_ADDRESS_ASSIGNMENT_CB
                        if (ZG->nwk.addr_cb != NULL)
                        {
                            zb_uint16_t short_addr = (ZG->nwk.addr_cb)(device_address);
                            addr = ( !zb_nwk_check_assigned_short_addr(short_addr) ) ? ( ZB_NWK_ROUTER_ADDRESS_ASSIGN() ) : ( short_addr );
                        }
                        else
#endif
                        {
                            addr = ZB_NWK_ROUTER_ADDRESS_ASSIGN();
                        }
                        TRACE_MSG(TRACE_NWK3, "assigned router addr 0x%x", (FMT__D, addr));
                    }
                    else
                    {
                        addr = *address;
                        TRACE_MSG(TRACE_NWK3, "keep old router address 0x%x", (FMT__D, addr));
                    }
                    if (addr != 0xffffU)
                    {
                        ZB_NIB().router_child_num++;
                        status = MAC_SUCCESS;
                    }
                    else
                    {
                        status = MAC_NO_SHORT_ADDRESS;
                    }
                }
            }
            else
            {
                /* child device is an ed and we have room for it */
                if (allocate_addr)
                {
#ifdef ZB_PRO_ADDRESS_ASSIGNMENT_CB
                    if (ZG->nwk.addr_cb != NULL)
                    {
                        zb_uint16_t short_addr = (ZG->nwk.addr_cb)(device_address);
                        addr = ( !zb_nwk_check_assigned_short_addr(short_addr) ) ? ( ZB_NWK_ED_ADDRESS_ASSIGN() ) : ( short_addr );
                    }
                    else
#endif
                    {
                        addr = ZB_NWK_ED_ADDRESS_ASSIGN();
                    }
                }
                else
                {
                    /* AD: if caps prevent us from setting address - use device previous address */
                    TRACE_MSG(TRACE_NWK3, "Old ed address 0x%x left", (FMT__D, *address));
                    addr = *address;
                }
#ifdef ZB_NWK_DISTRIBUTED_ADDRESS_ASSIGN
                TRACE_MSG(TRACE_NWK3, "assigned ed addr 0x%x (%u %hd %u %hd)",
                          (FMT__D_D_H_D_H, addr, ZB_PIBCACHE_NETWORK_ADDRESS(), ZB_NIB().max_routers, ZB_NIB().cskip, ZB_NIB().ed_child_num));
#else
                TRACE_MSG(TRACE_NWK3, "assigned ed addr 0x%x (%u %hd)",
                          (FMT__D_D_H, addr, ZB_PIBCACHE_NETWORK_ADDRESS(), ZB_NIB().ed_child_num));
#endif
                if (addr != 0xffffU)
                {
                    ZB_NIB().ed_child_num++;
                    status = MAC_SUCCESS;
                }
                else
                {
                    status = MAC_NO_SHORT_ADDRESS;
                }
            }
        }
    }

    // recheck or allocate neighbor record
    if (status == MAC_SUCCESS)
    {
        zb_address_ieee_ref_t ieee_ref;
        zb_ret_t ret_addr_by_ieee;

        *address = addr;

        /* allocate address translation entry */
        status = MAC_OUT_OF_CAP;
        /* Handle rejoin with address change: do not update address just now. Do
         * it after rejoin resp sent. */
        ret_addr_by_ieee = zb_address_by_ieee(device_address, ZB_FALSE, ZB_FALSE, &ieee_ref);
        if (ret_addr_by_ieee != RET_OK)
        {
            ret_addr_by_ieee = zb_address_update(device_address, addr, ZB_FALSE, &ieee_ref);
        }
        if (ret_addr_by_ieee == RET_OK)
        {
            /* allocate neighbor table entry if necessary */
            ret_addr_by_ieee = zb_nwk_neighbor_get(ieee_ref, ZB_TRUE, &ent);
        }
        if (ret_addr_by_ieee == RET_OK)
        {
            status = MAC_SUCCESS;
        }
    }

    if (ZB_NIB().router_child_num + ZB_NIB().ed_child_num >= ZB_NIB().max_children)
    {
        TRACE_MSG(TRACE_NWK1, "status %hd, max_children %hd, routers %hd, eds %hd - disable association permit",
                  (FMT__H_H_H_H, status, ZB_NIB().max_children, ZB_NIB().router_child_num, ZB_NIB().ed_child_num));
        /* zb_nwk_update_beacon_payload set ASSOCIATION_PERMIT in MAC */
        TRACE_MSG(TRACE_ZDO1, "schedule zb_nwk_update_beacon_payload", (FMT__0));
        ZB_PIBCACHE_ASSOCIATION_PERMIT() = ZB_FALSE_U;
        ret = zb_buf_get_out_delayed(zb_nwk_update_beacon_payload);
        if (ret != RET_OK)
        {
            status = MAC_OUT_OF_CAP;
            TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
        }
    }

    if (status == MAC_SUCCESS)
    {
        ZB_ASSERT(ent);
        /* update neighbor table entry */
        ent->device_type = ZB_U2B(ZB_MAC_CAP_GET_DEVICE_TYPE(capability)) ? ZB_NWK_DEVICE_TYPE_ROUTER : ZB_NWK_DEVICE_TYPE_ED;
        ent->rx_on_when_idle = ZB_MAC_CAP_GET_RX_ON_WHEN_IDLE(capability);
        ent->relationship = ZB_U2B(ZB_NIB_SECURITY_LEVEL()) ? ZB_NWK_RELATIONSHIP_UNAUTHENTICATED_CHILD : ZB_NWK_RELATIONSHIP_CHILD;
        ent->u.base.incoming_frame_counter = 0;
        ent->depth = ZB_NIB().depth + 1U;
        if (ent->device_type == ZB_NWK_DEVICE_TYPE_ED)
        {
            ent->permit_joining = 0;
#ifdef ZB_CERTIFICATION_HACKS
            /* r22 test-spec - TP/PED-7
             * Objective: DUT ZC applies default timeout to gZED; the default timeout shall be modified
             * from the default found in the specification to speed up testing. (CCB 2201)*/
            zb_init_ed_aging(ent, ZB_GET_ED_TIMEOUT(), ZB_TRUE);
#else
            /* As required by 3.6.10.2 set default aging timeout for end device */
            zb_init_ed_aging(ent, NWK_ED_DEVICE_TIMEOUT_DEFAULT, ZB_TRUE);
#endif
#ifdef ZB_MAC_PENDING_BIT_SOURCE_MATCHING
            {
                /* We are working via nbt_idx here because at least one implementation (for TI) uses this index internally.
                   Even while not used for other implementations (Like pure SW implementation), keep it there.
                 */
                zb_uint8_t nbt_idx = (zb_uint8_t)ZB_NWK_NEIGHBOR_GET_INDEX_BY_ENTRY_ADDRESS(ent);

                ret = zb_buf_get_out_delayed_ext(zb_nwk_src_match_add, nbt_idx, 0);
                if (ret != RET_OK)
                {
                    status = MAC_OUT_OF_CAP;
                    TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
                }
            }
#endif
        }
        ent->lqi = lqi;
#ifdef ZB_PRO_STACK
        ent->u.base.age = 0;
#endif
        TRACE_MSG(TRACE_NWK2, "neighbor ent %p rel %hd dev_t %hd rx.on %hd",
                  (FMT__P_H_H_H, ent, (zb_uint8_t)ent->relationship,
                   (zb_uint8_t)ent->device_type, (zb_uint8_t)ent->rx_on_when_idle));
        /*
        #ifdef ZB_USE_NVRAM
            zb_nvram_store_addr_n_nbt();
        #endif
        */
    }

    TRACE_MSG(TRACE_NWK1, "<<zb_nwk_accept_child status %hd, address 0x%x", (FMT__H_D, status, addr));
    return status;
}

void zb_mlme_associate_indication(zb_uint8_t param)
{
#ifndef ZB_LITE_NO_INDIRECT_QUEUE_PURGE
    /*  Repack request to buffer body */
    zb_mlme_associate_indication_t *indication_param = ZB_BUF_GET_PARAM(param, zb_mlme_associate_indication_t);
    zb_mlme_associate_indication_t *indication_body = (zb_mlme_associate_indication_t *)zb_buf_begin(param);
    zb_uint16_t short_addr;
    zb_ieee_addr_t ieee_addr;
#endif

    TRACE_MSG(TRACE_NWK1, ">> zb_mlme_associate_indication param %hd", (FMT__H, param));

#ifndef ZB_LITE_NO_INDIRECT_QUEUE_PURGE
    /* Quite hacky: put purge req into parameters, save ass indication in the body of the same parameter. */
    ZB_MEMCPY(indication_body, indication_param, sizeof(zb_mlme_associate_indication_t));
    ZB_IEEE_ADDR_COPY(ieee_addr, indication_param->device_address);
    short_addr = zb_address_short_by_ieee(ieee_addr);

    zb_nwk_purge_indirect_queue(param, ZB_MCPS_INDIR_Q_PURGE_TYPE_ASSOCIATION,
                                short_addr, ieee_addr);
#else
    zb_mlme_associate_indication_cont(param);
#endif

    TRACE_MSG(TRACE_NWK1, "<< zb_mlme_associate_indication", (FMT__0));
}

void zb_mlme_associate_indication_cont (zb_uint8_t param)
{
    zb_mac_status_t status;
    zb_uint16_t address = (zb_uint16_t)~0U;
    zb_ieee_addr_t device_address;
    zb_mlme_associate_indication_t *request = ZB_BUF_GET_PARAM(param, zb_mlme_associate_indication_t);

    TRACE_MSG(TRACE_NWK1, ">>zb_mlme_associate_indication_cont %hd", (FMT__H, param));

#ifdef ZB_CERTIFICATION_HACKS
    if (ZB_CERT_HACKS().disable_association_response)
    {
        TRACE_MSG(TRACE_NWK1, "Drop association indication primitive", (FMT__0));
        zb_buf_free(param);
    }
    else
#endif
    {
        CHECK_PARAM_RET_ON_ERROR(request);
#ifndef MAC_CERT_TEST_HACKS
        ZB_DUMP_IEEE_ADDR(request->device_address);

        /* MZ: Parent with mac_association_permit == ZB_FALSE should not respond
         * to MAC association request with success result. */
        if (ZB_U2B(ZB_PIBCACHE_ASSOCIATION_PERMIT()))
        {
            status = zb_nwk_accept_child(request->device_address, request->capability, request->lqi,
                                         &address);
        }
        else
        {
            status = MAC_OUT_OF_CAP;
        }
#else
        status = MAC_SUCCESS;
#endif /* MAC_CERT_TEST_HACKS */
        if (status == MAC_SUCCESS)
        {
            zb_ret_t ret;
            zb_address_ieee_ref_t ieee_ref;

            /* In case we reused address translation entry with different address...  */
            ret = zb_address_update(request->device_address, address, ZB_FALSE, &ieee_ref);
            ZB_ASSERT(ret == RET_OK || ret == RET_ALREADY_EXISTS);

            /* Save associating device address and send
             * ZB_DEVICE_ASSOCIATED_SIGNAL to the
             * zdo_startup_complete. Assumption that only one association
             * will be processed.*/
            ZB_IEEE_ADDR_COPY(ZG->nwk.associating_address, request->device_address);
            ret = zb_buf_get_out_delayed(zb_send_device_associated_signal);
            if (ret != RET_OK)
            {
                TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
            }
#ifdef ZB_PRO_ADDRESS_ASSIGNMENT_CB
            if (ZG->nwk.dev_associate_cb != NULL)
            {
                /* If device successfully associated, inform user App */
                (ZG->nwk.dev_associate_cb)(request->device_address);
            }
#endif
        }

        TRACE_MSG(TRACE_INFO1, "association: status %hd, address 0x%x, device " TRACE_FORMAT_64, (FMT__H_D_A, status, address, TRACE_ARG_64(request->device_address)));
        TRACE_MSG(TRACE_ATM1, "Z< association: status %hd, address 0x%04x, device " TRACE_FORMAT_64, (FMT__H_D_A, status, address, TRACE_ARG_64(request->device_address)));

        ZB_IEEE_ADDR_COPY(device_address, request->device_address);

        ZB_DUMP_IEEE_ADDR(device_address);

        ZB_MLME_BUILD_ASSOCIATE_RESPONSE(param, device_address, address, (zb_uint8_t)status);

        ZB_SCHEDULE_CALLBACK(zb_mlme_associate_response, param);

        /* next MAC sends response and calls our zb_mlme_comm_status_indication callback */
    }

    TRACE_MSG(TRACE_NWK1, "<<mlme_associate_ind", (FMT__0));
}

/**
   Send ZB_ZDO_SIGNAL_DEVICE_ASSOCIATED event to zdo_startup_complete()
   @param param - memory buffer
 */
static void zb_send_device_associated_signal(zb_uint8_t param)
{
    zb_nwk_signal_device_associated_params_t *dev_assoc_params;
    TRACE_MSG(TRACE_NWK1, "zb_send_device_associated_signal param %hd", (FMT__H, param));
    dev_assoc_params = (zb_nwk_signal_device_associated_params_t *)zb_app_signal_pack(param, ZB_NWK_SIGNAL_DEVICE_ASSOCIATED, RET_OK, (zb_uint8_t)sizeof(zb_nwk_signal_device_associated_params_t));
    ZB_IEEE_ADDR_COPY(dev_assoc_params->device_addr, ZG->nwk.associating_address);
    ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete, param);
}

#ifndef ZB_LITE_NO_ORPHAN_SCAN
void zb_mlme_orphan_indication(zb_uint8_t param)
{
#if defined ZB_ROUTER_ROLE
    zb_mac_orphan_ind_t *ind = ZB_BUF_GET_PARAM(param, zb_mac_orphan_ind_t);
    zb_address_ieee_ref_t ref;
    zb_neighbor_tbl_ent_t *nbt;
    zb_ret_t ret;
#endif
    TRACE_MSG(TRACE_NWK1, ">> orphan_ind prm %hd", (FMT__H, param));

#ifdef ZB_ROUTER_ROLE
    /* 3.6.1.4.3.3 Try to find dev in neighbor table */
    ret = zb_address_by_ieee(ind->orphan_addr, ZB_FALSE, ZB_FALSE, &ref);

    if (ret == RET_OK)
    {
        ret = zb_nwk_neighbor_get(ref, ZB_FALSE, &nbt);
    }
    if (ret == RET_OK
            && nbt->relationship == ZB_NWK_RELATIONSHIP_CHILD)
    {
        zb_uint16_t short_addr;
        zb_mac_orphan_response_t *resp = ZB_BUF_GET_PARAM(param, zb_mac_orphan_response_t);

        /* prepare and send orphan response */
        zb_address_ieee_by_ref(resp->orphan_addr, ref);
        /* EE: why short_addr is necessary here? */
        /* NK: Because orphan_response structure is packed, and (zb_uint16_t *) is not equal to
         * (zb_uint16_t__packed_ *) or so. Maybe (zb_uint16_t *)&resp->short_addr will work, but not
         * sure that it is correct. */
        zb_address_short_by_ref(&short_addr, ref);
        resp->short_addr = short_addr;
        resp->associated = ZB_TRUE;

        /* send orph resp */
        ZB_SCHEDULE_CALLBACK(zb_mlme_orphan_response, param);
    }
    else
    {
        /* orphan dev is not our child, drop */
        TRACE_MSG(TRACE_NWK1, "not a chld, drop", (FMT__0));
        zb_buf_free(param);
    }
#else
    /* ed doen't support orpan scan, just drop packet */
    TRACE_MSG(TRACE_NWK1, "orph ind for ed dev, drop", (FMT__0));
    zb_buf_free(param);
#endif

    TRACE_MSG(TRACE_NWK1, "<< orphan_ind", (FMT__0));
}
#endif  /* ZB_LITE_NO_ORPHAN_SCAN */


#endif  /* ROUTER_ROLE */

/* Routine itself not under ifdef to use same MAC */
void zb_mlme_comm_status_indication(zb_uint8_t param)
{
#ifdef ZB_ROUTER_ROLE
    if (ZB_IS_DEVICE_ZC_OR_ZR())
    {
        zb_ret_t ret;
        zb_address_ieee_ref_t addr_ref;
        zb_neighbor_tbl_ent_t *nent = NULL;

        zb_mlme_comm_status_indication_t *request = ZB_BUF_GET_PARAM(param, zb_mlme_comm_status_indication_t);

        TRACE_MSG(TRACE_NWK1, ">>zb_mlme_comm_status_indication " TRACE_FORMAT_64 " short 0x%x status %hd",
                  (FMT__A_D_H, TRACE_ARG_64(request->dst_addr.addr_long), request->dst_addr.addr_short, request->status));


        /*
          7.1.12.1.2 When generated

          The MLME-COMM-STATUS.indication primitive is generated by the MAC sublayer entity following
          either the MLME-ASSOCIATE.response primitive or the MLME-ORPHAN.response primitive.
         */

        /* TODO: handle MLME-ORPHAN.response? */

        if (request->dst_addr_mode == ZB_ADDR_64BIT_DEV)
        {
            ret = zb_address_by_ieee(request->dst_addr.addr_long, ZB_FALSE, ZB_FALSE, &addr_ref);
        }
        else
        {
            ret = zb_address_by_short(request->dst_addr.addr_short, ZB_FALSE, ZB_FALSE, &addr_ref);
        }

        if ( ret == RET_OK )
        {
            ret = zb_nwk_neighbor_get(addr_ref, ZB_FALSE, &nent);
        }

        if ( ret == RET_OK )
        {
            if (request->status == MAC_SUCCESS)
            {
                zb_uint16_t addr;
                /* Done. Issue NLME-JOIN.indication, update  */

                zb_nlme_join_indication_t *resp = ZB_BUF_GET_PARAM(param, zb_nlme_join_indication_t);

                zb_address_by_ref(resp->extended_address, &addr, addr_ref);
                resp->network_address = addr;
                resp->capability_information = 0;
                /* Only that 2 fields of capability information are meaningful */
                ZB_MAC_CAP_SET_DEVICE_TYPE(resp->capability_information, (ZB_B2U(nent->device_type == ZB_NWK_DEVICE_TYPE_ROUTER)));
                ZB_MAC_CAP_SET_RX_ON_WHEN_IDLE(resp->capability_information, nent->rx_on_when_idle);
                resp->rejoin_network = ZB_NLME_REJOIN_METHOD_ASSOCIATION;
                resp->secure_rejoin = 0;
                zb_buf_set_status(param, (zb_int16_t)ZB_NWK_STATUS_SUCCESS);

                ZB_SCHEDULE_CALLBACK(zb_nlme_join_indication, param);
                param = 0;
                /* Network configuration has changed - change update id. */
            }
            else
            {
                /* Failed. Remove this device from address and neighbor tables. */
                zb_nwk_forget_device(addr_ref);
                /* Rollback # of joined devices - potentially enable join */
                if (nent->device_type == ZB_NWK_DEVICE_TYPE_ROUTER)
                {
                    ZB_NIB().router_child_num--;
                }
#ifdef ZB_RAF_PERMIT_JOIN_TURN_OFF_UNEXPECTED
                else if (nent->device_type == ZB_NWK_DEVICE_TYPE_ED)
                {
                    ZB_NIB().ed_child_num--;
                }
#else
                else
                {
                    ZB_NIB().ed_child_num--;
                }
#endif
            }

            TRACE_MSG(TRACE_ZDO1, "schedule zb_nwk_update_beacon_payload", (FMT__0));
            ret = zb_buf_get_out_delayed(zb_nwk_update_beacon_payload);
            if (ret != RET_OK)
            {
                TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
            }
        }
    }

    if (param != 0U)
    {
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_NWK1, "<<zb_mlme_comm_status_indication", (FMT__0));
#else
    /* ED build - do nothing */
    zb_buf_free(param);
#endif  /* ZB_ROUTER_ROLE */
}

/*! @} */
