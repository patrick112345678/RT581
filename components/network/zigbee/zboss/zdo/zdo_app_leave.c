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
/* PURPOSE: LEAVE processing moved there from zdo_app.c
*/

#define ZB_TRACE_FILE_ID 10
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_nwk_nib.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zdo_common.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_nvram.h"
#include "zb_bdb_internal.h"
#include "zb_watchdog.h"
#include "zb_ncp.h"
#include "zdo_wwah_parent_classification.h"

#if defined ZB_ENABLE_ZLL
#include "zll/zb_zll_common.h"
#include "zll/zb_zll_nwk_features.h"
#endif /* defined ZB_ENABLE_ZLL */

/*! \addtogroup ZB_ZDO */
/*! @{ */

#ifndef NCP_MODE_HOST

#ifdef ZB_JOIN_CLIENT

/* This routine looks like ZDO-logic and was moved from nwk level and renamed.
 * Called from zb_nwk_do_leave if device leaves network without rejoin. */
void zdo_clear_after_leave(zb_uint8_t param)
{
    TRACE_MSG(TRACE_NWK1, ">>zb_zdo_clear_after_leave", (FMT__0));

    ZB_SET_JOINED_STATUS(ZB_FALSE);

    /* Clear security params from prev network */
#if defined ZB_TC_GENERATES_KEYS && defined ZB_COORDINATOR_ROLE
    secur_nwk_generate_keys();
#endif  /* ZB_TC_GENERATES_KEYS */
    zb_aps_secur_init();

    /* Merge from r21. Not sure need it here. */
    ZB_SCHEDULE_ALARM_CANCEL(zdo_authentication_failed, ZB_ALARM_ALL_CB);
    ZB_SCHEDULE_ALARM_CANCEL(bdb_request_tclk_alarm, ZB_ALARM_ALL_CB);
    /* Reset all registered ZDO callbacks */
    zdo_cb_reset();
    /*  warm restart - do not reset rx_on_when_idle etc */
    zdo_call_nlme_reset(param, ZB_TRUE, ZB_FALSE, zdo_commissioning_leave_done);

    TRACE_MSG(TRACE_NWK1, "<<zb_zdo_clear_after_leave", (FMT__0));
}
#endif /* ZB_JOIN_CLIENT */

/**
   NLME-LEAVE.indication primitive

   Called when device got LEAVE command from the net. It can be request for us
   to leave or indication that other device has left.
*/
void zb_nlme_leave_indication(zb_uint8_t param)
{
    zb_nlme_leave_indication_t *request = ZB_BUF_GET_PARAM(param, zb_nlme_leave_indication_t);

    if ( ZB_IEEE_ADDR_IS_ZERO(request->device_address) )
    {
        /*cstat !MISRAC2012-Rule-14.3_a */
        /** @mdr{00012,31} */
        if (!ZB_IS_DEVICE_ZC())
        {
#ifdef ZB_JOIN_CLIENT
            /* it is for us */
            TRACE_MSG(TRACE_ZDO2, "do leave", (FMT__0));
#if !defined ZB_LITE_NO_ZDO_POLL
            if (!ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()))
            {
                TRACE_MSG(TRACE_COMMON1, "Stopping poll", (FMT__0));
                zb_zdo_pim_stop_poll(0);
            }
#endif
            zb_nwk_do_leave(param, request->rejoin);
#endif /* #ifdef ZB_JOIN_CLIENT  */
        }
    }
    else
#ifdef ZB_ROUTER_ROLE
        if (ZB_IS_DEVICE_ZC_OR_ZR())
        {
            zb_ret_t ret = zb_buf_get_out_delayed_ext(zb_nlme_leave_indication_cont, param, 0);
            TRACE_MSG(TRACE_ZDO2, "device 0x%x has left", (FMT__H, zb_address_short_by_ieee(request->device_address)));
            if (ret != RET_OK)
            {
                TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
            }
        }
        else
#endif
        {
            TRACE_MSG(TRACE_ZDO2, "I am ZED, drop alien LEAVE", (FMT__0));
            zb_buf_free(param);
        }
}

#ifdef ZB_ROUTER_SECURITY
static void zb_zdo_update_device_on_leave(zb_uint8_t param, zb_ieee_addr_t ieee_addr, zb_bool_t rejoin)
{
    zb_uint8_t relationship;
    zb_address_ieee_ref_t addr_ref;
    zb_uint16_t short_address;

    ZB_ASSERT(param);
    relationship = zb_nwk_get_nbr_rel_by_ieee(ieee_addr);
    if (zb_address_by_ieee(ieee_addr, ZB_FALSE, ZB_FALSE, &addr_ref) == RET_OK)
    {
        zb_address_short_by_ref(&short_address, addr_ref);

        TRACE_MSG(TRACE_SECUR3, "before sending update-device.request, tc %hd, rel %hd, dev_type %hd, rejoin %hd",
                  (FMT__H_H_H_H, ZB_IS_TC(), relationship, zb_nwk_get_nbr_dvc_type_by_ieee(ieee_addr), rejoin));

        if (!ZB_IS_TC()
                /*cstat -MISRAC2012-Rule-13.5 */
                && (relationship == ZB_NWK_RELATIONSHIP_CHILD
                    || relationship == ZB_NWK_RELATIONSHIP_UNAUTHENTICATED_CHILD
                    /* In case when ED was requested to leave network it relationship
                     * already set to ZB_NWK_RELATIONSHIP_PREVIOUS_CHILD in leave.confirm.
                     * Send update device for that device with status DEVICE_LEFT.
                     */
                    || (relationship == ZB_NWK_RELATIONSHIP_PREVIOUS_CHILD
                        /* After some investigation, the following violation of Rule 13.5 seems to be a
                         * false positive. There are no side effects to
                         * 'zb_nwk_get_nbr_dvc_type_by_ieee()'. This violation seems to be caused
                         * by the fact that this function is an external function, which cannot be analyzed
                         * by C-STAT. */
                        && zb_nwk_get_nbr_dvc_type_by_ieee(ieee_addr) == ZB_NWK_DEVICE_TYPE_ED))
                /*cstat +MISRAC2012-Rule-13.5 */
                /* Do not send UPDATE-DEVICE if rejoin is 1. See 4.6.3.6.2 Router
                 * Operation and TC TP_R21_BV-02 */
                && !rejoin)
        {

            /*
              4.6.3.6.2 Router Operation

              Upon receipt of an NLME-LEAVE.indication primitive with the DeviceAddress parameter set to one of its 11532 children and with the Rejoin Parameter = 0, a router that is not also the Trust Center shall issue an 11533 APSME-UPDATE-DEVICE.request primitive
            */
            /* My child has left, I must inform TC */
            zb_apsme_update_device_req_t *req = ZB_BUF_GET_PARAM(param,
                                                zb_apsme_update_device_req_t);
            TRACE_MSG(TRACE_SECUR3, "sending update-device.request (device left) to TC", (FMT__0));

            req->status = ZB_DEVICE_LEFT;
            ZB_IEEE_ADDR_COPY(req->dest_address, ZB_AIB().trust_center_address);
            req->device_short_address = short_address;
            zb_address_ieee_by_ref(req->device_address, addr_ref);
            ZB_SCHEDULE_CALLBACK(zb_apsme_update_device_request, param);
            param = 0;
        }
    }

    if (param != 0U)
    {
        zb_buf_free(param);
    }
}
#endif /* ZB_ROUTER_SECURITY */


void zb_nlme_leave_indication_cont(zb_uint8_t param_buf, zb_uint16_t param_req)
{
    zb_uint8_t param_req_u8 = (zb_uint8_t)param_req;
    zb_nlme_leave_indication_t *request = ZB_BUF_GET_PARAM(param_req_u8, zb_nlme_leave_indication_t);

#ifdef ZB_ROUTER_ROLE
    zb_uint8_t relationship;
    zb_address_ieee_ref_t addr_ref;
#endif

    TRACE_MSG(TRACE_ZDO2, ">>zb_nlme_leave_indication_cont %hd %d", (FMT__H_D, param_buf, param_req_u8));

    zb_buf_copy(param_buf, param_req_u8);
    /* TODO: disable old callbacks-based user API */
    if (ZG->zdo.leave_ind_cb != NULL)
    {
        TRACE_MSG(TRACE_ZDO2, "Calling zdo leave ind cb %p", (FMT__P, ZG->zdo.leave_ind_cb));
        /* Leave callback synchronously. Callback must free or reuse buffer */
        (*ZG->zdo.leave_ind_cb)(param_buf);
    }
    else
    {
        /*Send ZB_DEVICE_LEAVE_INDICATION_SIGNAL to the zdo_startup_complete.*/
        zb_send_leave_indication_signal(param_buf, request->device_address, request->rejoin);
    }

    /* We re-used param_buf for callback/signal, no need to free it. */

#ifdef ZB_ROUTER_ROLE
    relationship = zb_nwk_get_nbr_rel_by_ieee(request->device_address);
    if (zb_address_by_ieee(request->device_address, ZB_FALSE, ZB_FALSE, &addr_ref) == RET_OK)
    {
        TRACE_MSG(TRACE_ZDO2, "device 0x%x relationship = %hd",
                  (FMT__D_H, zb_address_short_by_ieee(request->device_address), relationship));

        /* Handle child remove only if this is really child. */
        if (relationship == ZB_NWK_RELATIONSHIP_CHILD
                || relationship == ZB_NWK_RELATIONSHIP_PREVIOUS_CHILD
                || relationship == ZB_NWK_RELATIONSHIP_UNAUTHENTICATED_CHILD)
        {
#ifdef ZB_ROUTER_SECURITY
            /*cstat !MISRAC2012-Rule-14.3_a */
            /** @mdr{00023,0} */
            if (!IS_DISTRIBUTED_SECURITY())
            {
                zb_zdo_update_device_on_leave(param_req_u8, request->device_address, ZB_U2B(request->rejoin));
                param_req_u8 = 0;
            }
#endif  /* #ifndef ZB_ROUTER_SECURITY */
            ZB_SCHEDULE_CALLBACK2(zdo_device_removed, addr_ref, ZB_B2U(request->rejoin == 0U));
        }

        if (relationship == ZB_NWK_RELATIONSHIP_PARENT
                /*cstat !MISRAC2012-Rule-13.5 */
                /* After some investigation, the following violation of Rule 13.5 seems to be a false
                 * positive. There are no side effects to 'ZB_IS_DEVICE_ZC_OR_ZR()'. This violation
                 * seems to be caused by the fact that this function is an external function, which
                 * cannot be analyzed by C-STAT. */
                && ZB_IS_DEVICE_ZC_OR_ZR())
        {
            TRACE_MSG(TRACE_ERROR, "ZR: parent has left, we have no parent any more", (FMT__0));
            ZG->nwk.handle.parent = (zb_address_ieee_ref_t) -1;
        }

        /* If our neighbor left network, remove him from neighbor table */
        if ( relationship == ZB_NWK_RELATIONSHIP_SIBLING ||
                relationship == ZB_NWK_RELATIONSHIP_NONE_OF_THE_ABOVE)
        {
            /* Check rejoin flag here to exclude removing TCLK for rejoining device. */
            if (ZB_U2B(request->rejoin))
            {
                ZB_SCHEDULE_CALLBACK2(zdo_device_removed, addr_ref, ZB_B2U(request->rejoin == 0U));
            }
            else
            {
                zb_nwk_forget_device(addr_ref);
            }
        }
    }    /* if know that address */
#endif  /* router */
    if (param_req_u8 != 0U)
    {
        zb_buf_free(param_req_u8);
    }

    TRACE_MSG(TRACE_ZDO2, "<<zb_nlme_leave_indication_cont", (FMT__0));
}


/* **************************** << Device Authorized Signal functions << **************************** */


void zdo_device_removed(zb_uint8_t param, zb_uint16_t forget_device)
{
    zb_address_ieee_ref_t addr_ref;
    zb_neighbor_tbl_ent_t *nbt = NULL;
    zb_ret_t ret;

    TRACE_MSG(TRACE_ZDO2, ">> zdo_device_removed param %hd, forget_device = %d",
              (FMT__H_D, param, forget_device));

    addr_ref = (zb_address_ieee_ref_t)param;
#ifdef ZB_ROUTER_ROLE
    ret = zb_nwk_neighbor_get(addr_ref, ZB_FALSE, &nbt);

    if (ret == RET_OK)
    {
        if (( nbt->relationship == ZB_NWK_RELATIONSHIP_CHILD )
                || (nbt->relationship == ZB_NWK_RELATIONSHIP_PREVIOUS_CHILD)
                || (nbt->relationship == ZB_NWK_RELATIONSHIP_UNAUTHENTICATED_CHILD ))
        {
            zb_uindex_t i;
            /* Check if device performs rejoin - if it is so, we need to do 1 unlock */
            for (i = 0 ; i < ZG->nwk.handle.rejoin_req_table_cnt ; ++i)
            {
                /* address translation table still keeps "old" address - search by it */
                if (addr_ref == ZG->nwk.handle.rejoin_req_table[i].addr_ref)
                {
                    break;
                }
            }

            if (i < ZG->nwk.handle.rejoin_req_table_cnt)
            {
                zb_address_unlock(ZG->nwk.handle.rejoin_req_table[i].addr_ref);
                ZG->nwk.handle.rejoin_req_table_cnt--;
                if (ZG->nwk.handle.rejoin_req_table_cnt > i) /* it was not last element */
                {
                    ZB_MEMMOVE(&ZG->nwk.handle.rejoin_req_table[i], &ZG->nwk.handle.rejoin_req_table[i + 1U],
                               (ZG->nwk.handle.rejoin_req_table_cnt - i) * sizeof(zb_rejoin_context_t));
                }
            }

            TRACE_MSG(TRACE_ZDO3, "original ed_child_num %hd router_child_num %hd",
                      (FMT__H_H, ZB_NIB().ed_child_num, ZB_NIB().router_child_num));
            if (nbt->device_type == ZB_NWK_DEVICE_TYPE_ED)
            {
                ZB_ASSERT(ZB_NIB().ed_child_num != 0U);
                ZB_NIB().ed_child_num--;
            }
            else
            {
                ZB_ASSERT(ZB_NIB().router_child_num != 0U);
                ZB_NIB().router_child_num--;
            }

            ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_CHILD_MOVED_ID);
        }
        TRACE_MSG(TRACE_ZDO3, "original ed_child_num %hd router_child_num %hd",
                  (FMT__H_H, ZB_NIB().ed_child_num, ZB_NIB().router_child_num));
    }
    else
    {
        TRACE_MSG(TRACE_INFO1, "no nbt for addr_ref %hd, do nothing", (FMT__H, addr_ref));
        /* Yes, it is unusual, but not a cause for assert. */
        /* ZB_ASSERT(0); */
    }
#else
    ZVUNUSED(nbt);
#endif

    if (ZB_U2B(forget_device))
    {
        ZB_SCHEDULE_CALLBACK(zb_nwk_forget_device, addr_ref);
    }
    else
    {
        zb_uint16_t short_addr;

        /* If child leave with rejoin do not remove all information
         * related to it - at least save address and bindings */
        zb_address_short_by_ref(&short_addr, addr_ref);
        zb_aps_clear_after_leave(short_addr);
        ret = zb_nwk_neighbor_delete(addr_ref);
        if (ret != RET_OK)
        {
            TRACE_MSG(TRACE_ERROR, "zb_nwk_neighbor_delete addr_ref not found [%d]", (FMT__D, ret));
        }
#ifdef ZB_ROUTER_ROLE
        zb_nwk_restart_aging();
#endif
#if defined ZB_PRO_STACK && !defined ZB_LITE_NO_SOURCE_ROUTING && defined ZB_ROUTER_ROLE
        zb_nwk_source_routing_record_delete(short_addr);
#endif
    }

    TRACE_MSG(TRACE_ZDO2, "<< zdo_device_removed", (FMT__0));
}

/**
   NLME-LEAVE.confirm primitive

    Called when LEAVE initiated by LEAVE.REQUEST sent LEAVE command to net.
 */
void zb_nlme_leave_confirm(zb_uint8_t param)
{
    zb_nlme_leave_confirm_t *lc = ZB_BUF_GET_PARAM(param, zb_nlme_leave_confirm_t);
    zb_bool_t will_leave = ZB_FALSE;
    zb_address_ieee_ref_t addr_ref;
    zb_nwk_status_t zdp_resp_status = lc->status;
    zb_bool_t delete_nbt_ent = ZB_FALSE;
    zb_ieee_addr_t ieee_addr;

    TRACE_MSG(TRACE_NWK1, ">> zb_nlme_leave_confirm param %hd, status 0x%x, ieee " TRACE_FORMAT_64,
              (FMT__H_D_A, param, lc->status, TRACE_ARG_64(lc->device_address)));

    ZB_IEEE_ADDR_COPY(ieee_addr, lc->device_address);

#ifndef ZB_COORDINATOR_ONLY
    /* Regardless of the Status parameter to the NLME-LEAVE.confirm, the device
     * shall leave the network using the procedure in 3.6.1.10.4 */
    if (lc->status != ZB_NWK_STATUS_UNKNOWN_DEVICE
            && lc->status != ZB_NWK_STATUS_INVALID_REQUEST
            /*cstat -MISRAC2012-Rule-13.5 */
            /* After some investigation, the following violation of Rule 13.5 seems to be a false
             * positive. There are no side effects to 'ZB_IS_DEVICE_ZED()' or 'ZB_IS_DEVICE_ZC()'.
             * This violation seems to be caused by the fact that these function are external
             * functions, which cannot be analyzed by C-STAT. */
            && (ZB_IS_DEVICE_ZED() || (ZB_IEEE_ADDR_IS_ZERO(lc->device_address) && !ZB_IS_DEVICE_ZC())))
        /*cstat +MISRAC2012-Rule-13.5 */
    {
        will_leave = ZB_TRUE;
    }
    else
#endif
        if (lc->status != ZB_NWK_STATUS_SUCCESS)
        {
            zb_uint8_t nb_device_type = zb_nwk_get_nbr_dvc_type_by_ieee(lc->device_address);
            zb_uint8_t neighbor_relationship = zb_nwk_get_nbr_rel_by_ieee(lc->device_address);

            if (nb_device_type == ZB_NWK_DEVICE_TYPE_ED
                    && (neighbor_relationship == ZB_NWK_RELATIONSHIP_CHILD
                        || neighbor_relationship == ZB_NWK_RELATIONSHIP_UNAUTHENTICATED_CHILD))
            {
                if (lc->status != ZB_NWK_STATUS_UNKNOWN_DEVICE)
                {
                    /* ref: ZCP PRO R22 test specification TP/SEC/BV-28 */
                    /* Send ZDP Mgmt_Leave_resp if pending, and remove the device afterwards */
                    zdp_resp_status = ZB_NWK_STATUS_SUCCESS;
                }

                delete_nbt_ent = ZB_TRUE;
            }
        }
        else
        {
            /* MISRA rule 15.7 requires empty 'else' branch. */
        }

    TRACE_MSG(TRACE_ZDO2, "> LEAVE.CONFIRM status %hd will_leave %hd", (FMT__H_H, lc->status, will_leave));

    if (!zdo_try_send_mgmt_leave_rsp(param, zdp_resp_status))
    {
#ifndef ZB_COORDINATOR_ONLY
        /* not need to send resp. Maybe, leave now. */
        if (will_leave)
        {
            zb_nwk_do_leave(param, ZG->nwk.leave_context.rejoin_after_leave);
            param = 0;
        }
#endif  /* #ifndef ZB_COORDINATOR_ONLY */
    }
    else
    {
        param = 0;
    }

    if (delete_nbt_ent)
    {
        if (zb_address_by_ieee(ieee_addr, ZB_FALSE, ZB_FALSE, &addr_ref) == RET_OK)
        {
            zb_nwk_forget_device(addr_ref);
        }
    }

    if (param != 0U)
    {
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_NWK1, "<< zb_nlme_leave_confirm", (FMT__0));
}


#ifdef ZB_ROUTER_ROLE
void zdo_handle_mgmt_leave_rsp(zb_uint16_t src)
{
    zb_neighbor_tbl_ent_t *ent = NULL;
    zb_address_ieee_ref_t ref;

    /* If we just removed neighbor after receiving LEAVE and then received Mgmt
     * Leave Rsp and created neighbor entry, remove it */
    if (zb_address_by_short(src, ZB_FALSE, ZB_FALSE, &ref) == RET_OK)
    {
        if (zb_nwk_neighbor_get(ref, ZB_FALSE, &ent) == RET_OK
                && ent->relationship == ZB_NWK_RELATIONSHIP_NONE_OF_THE_ABOVE)
        {
            zb_ret_t ret;

            TRACE_MSG(TRACE_ZDO1, "Delete neighbor for addr 0x%x", (FMT__D, src));
            ret = zb_nwk_neighbor_delete(ref);
            if (ret != RET_OK)
            {
                TRACE_MSG(TRACE_ERROR, "zb_nwk_neighbor_delete address reference not found [%d]", (FMT__D, ret));
            }
        }
    }
}
#endif

#ifndef ZB_LITE_NO_OLD_CB
void zb_zdo_register_leave_cb(zb_callback_t leave_cb)
{
    ZDO_CTX().app_leave_cb = leave_cb;
}
#endif

#endif  /* NCP_MODE_HOST */

void zb_send_leave_signal(zb_uint8_t param, zb_uint16_t user_param)
{
    zb_zdo_signal_leave_params_t *leave_params;
    TRACE_MSG(TRACE_NWK1, "zb_send_leave_signal param %hd", (FMT__H, param));
    leave_params = (zb_zdo_signal_leave_params_t *)zb_app_signal_pack(param, ZB_ZDO_SIGNAL_LEAVE, RET_OK, (zb_uint8_t)sizeof(zb_zdo_signal_leave_params_t));
    leave_params->leave_type = (zb_nwk_leave_type_t) ZB_GET_LOW_BYTE(user_param);
    ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete, param);
}


/*! @} */
