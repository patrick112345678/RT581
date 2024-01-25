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
/* PURPOSE: Join related logic (client side) moved from zdo_app.c
   In NCP builds - SoC only.
*/

#define ZB_TRACE_FILE_ID 9
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

#if defined ZB_ENABLE_SE
#include "zb_se.h"
#endif /* defined ZB_ENABLE_SE */

#ifdef ZB_JOIN_CLIENT

/* Param is index in scanlist to network descriptor */
void zdo_join_to_nwk_descr(zb_uint8_t param)
{
    zb_ret_t ret;
    zb_nlme_join_request_t *req;
    zb_nlme_network_discovery_confirm_t *cnf;
    zb_nlme_network_descriptor_t *dsc;
    zb_ext_pan_id_t extended_pan_id;

    TRACE_MSG(TRACE_NWK1, ">> zb_nlme_join_to_nwk_descr param %hd", (FMT__H, param));

    cnf = (zb_nlme_network_discovery_confirm_t *)zb_buf_begin(COMM_CTX().discovery_ctx.scanlist_ref);
    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,63} */
    dsc = (zb_nlme_network_descriptor_t *)(cnf + 1);
    /* Removed from here checks for scanlist_idx out of bounds: seems impossible. */

    dsc += COMM_CTX().discovery_ctx.scanlist_idx;
    req = ZB_BUF_GET_PARAM(param, zb_nlme_join_request_t);

    ZB_BZERO(req, sizeof(*req)); /* all defaults to 0 */
    zb_address_get_pan_id(dsc->panid_ref, extended_pan_id);
    ZB_EXTPANID_COPY(req->extended_pan_id, extended_pan_id);
#ifdef ZB_ROUTER_ROLE
    /* joined_pro, here's one of the key moments */
    if ((zb_get_device_type() == ZB_NWK_DEVICE_TYPE_NONE
            /*cstat !MISRAC2012-Rule-13.5 */
            /* After some investigation, the following violation of Rule 13.5 seems to be a false
             * positive. There are no side effects to 'zb_get_device_type()'. This violation
             * seems to be caused by the fact that this function is an external function, which
             * cannot be analyzed by C-STAT. */
            || zb_get_device_type() == ZB_NWK_DEVICE_TYPE_ROUTER)
            /* #AT */
#ifndef ZB_PRO_COMPATIBLE
            && (ZB_STACK_PROFILE == dsc->stack_profile)
#else
            && (dsc->stack_profile == STACK_2007)
#endif
       )
    {
        ZB_MAC_CAP_SET_ROUTER_CAPS(req->capability_information);  /* join as router */
    }
    else
#endif
    {
        if (zb_get_rx_on_when_idle())
        {
            ZB_MAC_CAP_SET_RX_ON_WHEN_IDLE(req->capability_information, 1U);
            ZB_MAC_CAP_SET_POWER_SOURCE(req->capability_information, 1U);
        }
    }
    ZB_MAC_CAP_SET_ALLOCATE_ADDRESS(req->capability_information, 1U);

    /* Set channel list from nwk descr (one channel and page). */
    zb_channel_list_init(req->scan_channels_list);
    ret = zb_channel_page_list_set_logical_channel(req->scan_channels_list, dsc->channel_page, dsc->logical_channel);
    TRACE_MSG(TRACE_NWK1, "join to page %hd logical_channel %hd", (FMT__H_H, dsc->channel_page, dsc->logical_channel));
    /* dsc->channel_page and dsc->logical_channel are obtained during channel scan (MLME-SCAN.{request,confirm}). Therefore, they
     * are valid and supported by the stack. Even if zb_channel_page_list_set_logical_channel() fails due to some implementation error,
     * 'req->scan_channels_list' will stay empty and zb_nlme_join_request() handles it gracefully. */
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_NWK1, "capability_information 0x%x", (FMT__H, req->capability_information));

    ZB_SCHEDULE_CALLBACK(zb_nlme_join_request, param);

    TRACE_MSG(TRACE_NWK1, "<< zb_nlme_join_to_nwk_descr", (FMT__0));
}

void zdo_retry_joining(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ZDO1, "zdo_retry_joining param %hd", (FMT__H, param));

#ifndef NCP_MODE_HOST
    /* Maybe move this check to NWK? If it is needed at all. Maybe it is better to deinitialize
       poll-related stuff after leave. */
    if (ZG->nwk.handle.poll_in_progress)
    {
        /* Wait some more time for poll_confirm */
        ZB_SCHEDULE_ALARM(zdo_retry_joining, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(100));
    }
    else
#endif /* !NCP_MODE_HOST */
    {
        ZB_SCHEDULE_CALLBACK(zdo_commissioning_join_via_scanlist, COMM_CTX().discovery_ctx.scanlist_ref);
    }
}

void zdo_handle_nlme_network_discovery_confirm(zb_uint8_t param)
{
    zb_nlme_network_discovery_confirm_t *cnf;

    TRACE_MSG(TRACE_ZDO1, ">> zdo_handle_nlme_network_discovery_confirm, param %hd",
              (FMT__H, param));

    cnf = (zb_nlme_network_discovery_confirm_t *)zb_buf_begin(param);

    if (cnf->status == RET_OK)
    {
        if (COMM_CTX().discovery_ctx.scanlist_ref == 0U)
        {
            COMM_CTX().discovery_ctx.scanlist_ref = param;
            COMM_CTX().discovery_ctx.scanlist_idx = 0;
            TRACE_MSG(TRACE_ZDO3, "initial assign of discovery_ctx.scanlist_ref %hd", (FMT__H, param));
            param = 0;
        }
        else
        {
            /* Merge scanlist and received nwk descriptor */
            zb_nlme_network_descriptor_t *odsc;
            zb_nlme_network_descriptor_t *ndsc;
            zb_nlme_network_discovery_confirm_t *ocnf;
            zb_uint_t i;
            zb_uint_t j;

            ocnf = (zb_nlme_network_discovery_confirm_t *)zb_buf_begin(COMM_CTX().discovery_ctx.scanlist_ref);
            /*cstat !MISRAC2012-Rule-11.3 */
            /** @mdr{00002,64} */
            ndsc = (zb_nlme_network_descriptor_t *)(cnf + 1);
            /*cstat !MISRAC2012-Rule-11.3 */
            /** @mdr{00002,65} */
            odsc = (zb_nlme_network_descriptor_t *)(ocnf + 1);
            /* 1. Update entries which are already existing, but are now "better". */
            for (i = 0 ; i < ocnf->network_count ; ++i)
            {
                for (j = 0 ; j < cnf->network_count ; ++j)
                {
                    if (odsc[i].panid_ref == ndsc[j].panid_ref)
                    {
                        odsc[i].permit_joining |= ndsc[j].permit_joining;
                        odsc[i].router_capacity |= ndsc[j].router_capacity;
                        odsc[i].end_device_capacity |= ndsc[j].end_device_capacity;
                    }
                }
            }
            /* 2. If have some space in odsc, add there records from ndsc */
            for (j = 0 ;
                    ocnf->network_count < ZB_PANID_TABLE_SIZE
                    && j < cnf->network_count ;
                    ++j)
            {
                for (i = 0 ;
                        i < ocnf->network_count
                        && odsc[i].panid_ref != ndsc[j].panid_ref
                        ; ++i)
                {
                }
                if (i == ocnf->network_count)
                {
                    /* TODO: maybe, ckip closed networks? */
                    odsc[i] = ndsc[j];
                    ocnf->network_count++;
                }
            }
        } /* else (have scanlist) */
    }

    if (COMM_CTX().discovery_ctx.disc_count != 0U)
    {
        COMM_CTX().discovery_ctx.disc_count--;
    }
    TRACE_MSG(TRACE_NWK3, "disc_count %hd", (FMT__H, COMM_CTX().discovery_ctx.disc_count));
    /* If network count is not zero, process with join. */
    if ((COMM_CTX().discovery_ctx.disc_count != 0U) && (cnf->network_count == 0U))
    {
        /* We may not have buffer here */
        (void)zb_buf_reuse(param);
        ZB_SCHEDULE_CALLBACK(zdo_next_nwk_discovery_req, param);
        param = 0;
    }
    else
    {
        if (COMM_CTX().discovery_ctx.scanlist_ref != 0U)
        {
            ZB_SCHEDULE_CALLBACK(zdo_commissioning_join_via_scanlist, 0);
        }
        else
        {
            (void)zb_buf_reuse(param);
            ZB_SCHEDULE_CALLBACK(zdo_commissioning_nwk_discovery_failed, param);
            param = 0;
        }
    }
    if (param != 0U)
    {
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_ZDO1, "<< zdo_handle_nlme_network_discovery_confirm", (FMT__0));
}


void zdo_reset_scanlist(zb_bool_t do_free)
{
    TRACE_MSG(TRACE_ZDO1, ">> zb_zdo_reset_scanlist do_free %d scanlist_ref %hd", (FMT__D_H, do_free, COMM_CTX().discovery_ctx.scanlist_ref));
    if (ZB_U2B(COMM_CTX().discovery_ctx.scanlist_ref) && do_free)
    {
        zb_buf_free(COMM_CTX().discovery_ctx.scanlist_ref);
    }
    COMM_CTX().discovery_ctx.scanlist_ref = 0U;
    COMM_CTX().discovery_ctx.scanlist_idx = 0U;
    COMM_CTX().discovery_ctx.scanlist_join_attempt_n = 0U;
    TRACE_MSG(TRACE_ZDO1, "<< zb_zdo_reset_scanlist", (FMT__0));
}


static zb_uint8_t zdo_scan_duration(void)
{
    zb_uint8_t scan_duration = ZB_DEFAULT_SCAN_DURATION;

    if (COMM_SELECTOR().get_scan_duration != NULL)
    {
        scan_duration = COMM_SELECTOR().get_scan_duration();
    }

    return scan_duration;
}


static void zdo_get_scan_channel_mask(zb_channel_list_t channel_list)
{
    if (COMM_SELECTOR().get_scan_channel_mask != NULL)
    {
        COMM_SELECTOR().get_scan_channel_mask(channel_list);
    }
    else
    {
        zb_aib_get_channel_page_list(channel_list);
    }
}


void zdo_next_nwk_discovery_req(zb_uint8_t param)
{
    zb_ret_t ret;
    zb_nlme_network_discovery_request_t *req;

    if (param == 0U)
    {
        ret = zb_buf_get_out_delayed(zdo_next_nwk_discovery_req);
        if (ret != RET_OK)
        {
            TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
        }
    }
    else
    {
        req = ZB_BUF_GET_PARAM(param, zb_nlme_network_discovery_request_t);

        zdo_get_scan_channel_mask(req->scan_channels_list);
        req->scan_duration = zdo_scan_duration();

#if TRACE_ENABLED(TRACE_NWK3)
        {
            zb_uint_t i;

            TRACE_MSG(TRACE_NWK3, "call discovery_request channels:", (FMT__0));
            for (i = 0; i < ZB_CHANNEL_PAGES_NUM; i++)
            {
                TRACE_MSG(TRACE_NWK3,
                          "page %hd, mask 0x%lx",
                          (FMT__H_L,
                           zb_channel_page_list_get_page(req->scan_channels_list, i),
                           zb_channel_page_list_get_mask(req->scan_channels_list, i)));
            }
        }
#endif /* TRACE_ENABLED(TRACE_NWK3) */

        ZB_SCHEDULE_ALARM(zb_nlme_network_discovery_request, param,
                          ZB_MILLISECONDS_TO_BEACON_INTERVAL(COMM_CTX().discovery_ctx.nwk_time_btwn_scans));
    }
}


#ifndef NCP_MODE_HOST

/*! \addtogroup ZB_ZDO */
/*! @{ */

#if defined ZB_ENABLE_ZLL
void zb_zdo_startup_complete_zll(zb_uint8_t param);
#endif

static void zdo_join_done(zb_uint8_t param);
static void zdo_join_done_cont(zb_uint8_t param);

#ifdef ZB_ED_FUNC
static void start_poll_afrer_join(zb_uint8_t param);
#endif

#ifdef ZB_REJOIN_BACKOFF
void zdo_rejoin_backoff_int(zb_uint8_t param);
void zb_zdo_rejoin_backoff_continue(zb_uint8_t param);
#endif

/*
  Killed from here zdo_initiate_join() and zdo_join(): used only in very few ancient tests.
 */

void zdo_rejoin_clear_prev_join(void)
{
    /* Unlock addresses only when device have been joined, othervise addresses will be undefied */
    if (ZB_JOINED())
    {
        if (ZG->nwk.handle.parent != (zb_uint8_t) -1)
        {
            zb_ret_t ret = zb_nwk_neighbor_delete(ZG->nwk.handle.parent);
            if (ret != RET_OK)
            {
                TRACE_MSG(TRACE_ERROR, "zb_nwk_neighbor_delete failed [%d]", (FMT__D, ret));
                ZB_ASSERT(0);
            }
            ZG->nwk.handle.parent = (zb_uint8_t)(-1);
        }

        {
            zb_address_ieee_ref_t addr_ref;
            /* Unlock our address and our parent's address: it will be locked at rejoin confirm */
            if (zb_address_by_short(ZB_PIBCACHE_NETWORK_ADDRESS(), ZB_FALSE, ZB_FALSE, &addr_ref) == RET_OK)
            {
                zb_address_unlock(addr_ref);
            }
        }
#ifdef ZB_ROUTER_ROLE
        /* Just in case */
        ZB_SCHEDULE_ALARM_CANCEL(zdo_send_parent_annce, 0);
#endif
    }

    ZG->zdo.handle.rejoin = ZB_TRUE;
    ZB_SET_JOINED_STATUS(ZB_FALSE);

#if !defined ZB_LITE_NO_ZDO_POLL
    /* Stop ZDO poll. Rejoin uses its own poll. */
    zb_zdo_pim_stop_poll(0);
#endif
}

static void zdo_initiate_rejoin_cont(zb_uint8_t param)
{
    zb_nlme_join_request_t *req = ZB_BUF_GET_PARAM(param, zb_nlme_join_request_t);
    zb_bool_t secure_rejoin;

    TRACE_MSG(TRACE_ZDO1, ">>zdo_initiate_rejoin_cont ", (FMT__0));

    ZB_BZERO(req, sizeof(zb_nlme_join_request_t));

    secure_rejoin = ZG->zdo.handle.rejoin_ctx.secure_rejoin;
    ZB_EXTPANID_COPY(req->extended_pan_id, ZG->zdo.handle.rejoin_ctx.ext_pan_id);
    zb_channel_page_list_copy(req->scan_channels_list, ZG->zdo.handle.rejoin_ctx.channels_list);

    if (!ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE())
            && ZB_U2B(ZB_MAC_CAP_GET_RX_ON_WHEN_IDLE(ZB_ZDO_NODE_DESC()->mac_capability_flags)))
    {
        zb_uint8_t mask = 0;
        ZB_MAC_CAP_SET_RX_ON_WHEN_IDLE(mask, 1U);
        ZB_ZDO_NODE_DESC()->mac_capability_flags &= ~mask;
        TRACE_MSG(TRACE_ZDO1, "Oops... we have rx-on-when idle in ZDO Node desc but off in pibcache. Strange. Patching..", (FMT__0));
    }

    req->capability_information = ZB_ZDO_NODE_DESC()->mac_capability_flags;

    /* Why should we set it here? */
    ZB_MAC_CAP_SET_ALLOCATE_ADDRESS(req->capability_information, 1U);

    req->rejoin_network = ZB_NLME_REJOIN_METHOD_REJOIN;
    req->scan_duration = ZB_DEFAULT_SCAN_DURATION; /* TODO: configure it somehow? */
    req->security_enable = ZB_B2U(secure_rejoin);
    if (!secure_rejoin)
    {
        /* FIXME: do not use it directly! */
        ZG->nwk.handle.tmp.rejoin.unsecured_rejoin = ZB_TRUE;
        ZG->aps.authenticated = ZB_FALSE;
    }
    else
    {
        ZG->nwk.handle.tmp.rejoin.unsecured_rejoin = ZB_FALSE;
    }

    TRACE_MSG(TRACE_ZDO1, "Rejoin to pan " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(req->extended_pan_id)));
    TRACE_MSG(TRACE_ZDO1, "Security enable %hd", (FMT__H, req->security_enable));

    TRACE_MSG(TRACE_ZDO1, "<<zdo_initiate_rejoin_cont", (FMT__0));

    /* Small delay for test TP_R21_BV-07 */
    ZB_SCHEDULE_ALARM(zb_nlme_join_request, param, 5);
}


zb_ret_t zdo_initiate_rejoin(zb_bufid_t buf, zb_uint8_t *ext_pan_id,
                             zb_channel_page_t *channels_list, zb_bool_t secure_rejoin)
{
    TRACE_MSG(TRACE_ZDO1, ">>zdo_initiate_rejoin ", (FMT__0));

    ZB_ASSERT(!ZB_IS_DEVICE_ZC());

    zdo_rejoin_clear_prev_join();
    zb_aps_clear_after_leave(ZB_NWK_BROADCAST_ALL_DEVICES);

    ZG->zdo.handle.rejoin = ZB_TRUE;
    ZB_SET_JOINED_STATUS(ZB_FALSE);

    /* Save all incoming params in zdo.handle */
    ZG->zdo.handle.rejoin_ctx.secure_rejoin = secure_rejoin;
    ZB_EXTPANID_COPY(ZG->zdo.handle.rejoin_ctx.ext_pan_id, ext_pan_id);
    zb_channel_page_list_copy(ZG->zdo.handle.rejoin_ctx.channels_list, channels_list);
#ifndef ZB_ED_RX_OFF_WHEN_IDLE
    if (ZB_PIBCACHE_RX_ON_WHEN_IDLE() == 0xFFU)
    {
        ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_TRUE_U;
    }
#endif
    {
        zb_uint8_t rx_on = ZB_PIBCACHE_RX_ON_WHEN_IDLE();
        zb_nwk_pib_set(buf, ZB_PIB_ATTRIBUTE_RX_ON_WHEN_IDLE,
                       &rx_on, 1, zdo_initiate_rejoin_cont);
    }

    TRACE_MSG(TRACE_ZDO1, "<<zdo_initiate_rejoin ", (FMT__0));

    return RET_OK;
}


/*
  NWK discovery done. Let's continue join.
 */
void zb_nlme_network_discovery_confirm(zb_uint8_t param)
{
    zb_nlme_network_discovery_confirm_t *cnf;

    TRACE_MSG(TRACE_NWK1, ">> zb_nlme_network_discovery_confirm %hd", (FMT__H, param));

    /* Scanlist feature.
     * Main improvement: Reduce active scans - on next discovery attempt use last scan list if it
     * contains unprocessed records.
     * Actual behavour: Save the buffer from scan confirm in commissioning context
     * (COMM_CTX().scanlist_ref). Override the points of entering to zdo_startup_complete with error
     * status - call discovery_confirm instead (with the scanlist stored on previous scan). Reset the
     * scanlist after successfull authentication (on device_annce) or when the scan is fully completed
     * (all available records are processed) - for the last case pass error status to
     * zdo_startup_complete and switch to the next scan.
     */

    cnf = (zb_nlme_network_discovery_confirm_t *)zb_buf_begin(param);
    if (cnf->status != RET_OK)
    {
        /* Do not need neighbors/extneighbors anymore */
        zb_nwk_neighbor_clear();
#ifdef ZB_ROUTER_ROLE
        zb_nwk_restart_aging(); /* since it depends on neighbor table */
#endif
    }
    TRACE_MSG(TRACE_ZDO1, "nwk_discovery_confirm: status %d, network_count %hd",
              (FMT__D_H,
               cnf->status, cnf->network_count));
    /*cstat !MISRAC2012-Rule-14.3_b */
    /** @mdr{00028,0} */
    if (!NCP_CATCH_NWK_DISC_CFM(param))
    {
        zdo_handle_nlme_network_discovery_confirm(param);
    }
}


static void zdo_join_done(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ZDO1, ">>join_done %hd security_level %hd", (FMT__H_H, param, ZB_NIB_SECURITY_LEVEL()));

    /* Until this point startup logic didn't depend whether we are sleepy device or not.
     * Now set RxOnWhenIdle value specified by application */
#ifndef ZB_ED_RX_OFF_WHEN_IDLE
    if (ZB_PIBCACHE_RX_ON_WHEN_IDLE() == 0xFFU)
    {
        /* Application doesn't specify value, use RxOn by default */
        ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_TRUE_U;
    }
#endif

    {
        zb_uint8_t rx_on = ZB_PIBCACHE_RX_ON_WHEN_IDLE();
        zb_nwk_pib_set(param, ZB_PIB_ATTRIBUTE_RX_ON_WHEN_IDLE,
                       &rx_on, 1, zdo_join_done_cont);
    }

    TRACE_MSG(TRACE_ZDO1, "<<join_done", (FMT__0));
}


static void zdo_join_done_cont(zb_uint8_t param)
{
    zb_mlme_set_confirm_t *cfm = (zb_mlme_set_confirm_t *)zb_buf_begin(param);

    TRACE_MSG(TRACE_ZDO1, ">>zdo_join_done_cont %hd security_level %hd",
              (FMT__H_H, param, ZB_NIB_SECURITY_LEVEL()));

    ZB_ASSERT(cfm->status == MAC_SUCCESS);
    /* ZB_ASSERT(cfm->pib_attr == ZB_PIB_ATTRIBUTE_RX_ON_WHEN_IDLE); */
    ZVUNUSED(cfm);

    if (ZB_U2B(ZB_NIB_SECURITY_LEVEL())
            /* In ZBOSS r20 with some echo system the following issue was */
            /* seen: parent device hasn't sent transport key. This workaround disbles waiting for a transport key. */
            /* R21: Seems like if we are already authenticated, we will not receive Transport key on rejoin
             * (without request). */
            && ((!ZG->aps.authenticated) || ZB_U2B(ZG->nwk.handle.tmp.rejoin.unsecured_rejoin))
       )
    {
        ZG->aps.authenticated = ZB_FALSE;
        /* If security is on, wait for transport-key and send device annonce after
         * it, when authenticated. */
        zb_buf_free(param);
        /*
        2.1 Standard Security Mode

        If the joining device
        did not receive the key via the APSME-TRANSPORT-KEY.indication within the
        apsSecurityTimeOutPeriod since receiving the NLME-JOIN.confirm primitive, it
        shall reset and may choose to start the joining procedure again.
        */
        ZB_SCHEDULE_ALARM(zdo_authentication_failed, 0, ZB_APS_SECURITY_TIME_OUT_PERIOD);
    }
    else
    {
        /* If already have NWK key or no security, send device annce now. */
        TRACE_MSG(TRACE_ZDO3, "scheduling device_annce %hd", (FMT__H, param));
        ZB_SCHEDULE_CALLBACK(zdo_authenticated_send_device_annce, param);
    }

    /* clear poll retry count */
    ZDO_CTX().parent_threshold_retry = 0;

#ifdef ZB_ED_FUNC
    /* Start polling function */
    if (!ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()))
    {
        TRACE_MSG(TRACE_ZDO3, "Start polling if we are sleeping ED. secur level %d authenticated %d", (FMT__D_D, ZB_NIB_SECURITY_LEVEL(), ZG->aps.authenticated));

        /* NK: Reset PIM to defaults - it is new join! */
        zb_zdo_pim_init_defaults();

        /* Start poll with 0.5s delay to give our parent a chance to send
         * Update Device and reecive Transport Key from ZC. Polling
         * immedietaly causes collisions and delays. */
        ZB_SCHEDULE_ALARM(start_poll_afrer_join, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(500));

#if defined ZB_MAC_POWER_CONTROL
        if (ZB_LOGICAL_PAGE_IS_SUB_GHZ(ZB_PIBCACHE_CURRENT_PAGE()))
        {
            zb_buf_get_out_delayed(zb_nwk_lpd_joined_child);
        }
#endif  /* ZB_MAC_POWER_CONTROL */
    }
#endif
    ZB_P3_ON();

    TRACE_MSG(TRACE_ZDO1, "<<zdo_join_done_cont", (FMT__0));
}


#ifdef ZB_ED_FUNC
static void start_poll_afrer_join(zb_uint8_t param)
{
    ZVUNUSED(param);

    zb_zdo_pim_start_poll(0);
    if (ZB_U2B(ZB_NIB_SECURITY_LEVEL()) && !ZG->aps.authenticated)
    {
        /*cstat !MISRAC2012-Rule-14.3_a */
        /** @mdr{00010,4} */
        if (!ZB_SE_MODE())
        {
            /* start turbo poll to receive nwk key:
               Using the value of 2, because two (2) Transport Keys are expected -
               APS encrypted and APS non-encrypted (in case of joining thru router).
            */
            /* FIXME" is it still actual?? */
            TRACE_MSG(TRACE_ZDO3, "Call zb_zdo_pim_start_turbo_poll_packets 2", (FMT__0));
            zb_zdo_pim_start_turbo_poll_packets(2);
        }
        /*cstat !MISRAC2012-Rule-2.1_b */
        /** @mdr{00010,5} */
        else
        {
            TRACE_MSG(TRACE_ZDO3, "Call zb_zdo_pim_start_turbo_poll_packets 1", (FMT__0));
            zb_zdo_pim_start_turbo_poll_packets(1);
        }
    }
    TRACE_MSG(TRACE_COMMON1, "Join done, scheduling poll request", (FMT__0));
}
#endif  /* ZB_ED_FUNC */


/*
  We are here after join or rejoin complete - successful or unsuccessful
 */
void zb_nlme_join_confirm(zb_uint8_t param)
{
    zb_nlme_join_confirm_t *confirm = ZB_BUF_GET_PARAM(param, zb_nlme_join_confirm_t);

    TRACE_MSG(TRACE_ZDO1, ">>nlme_join_conf %hd", (FMT__H, param));

    if (confirm->status == 0)
    {
        TRACE_MSG(TRACE_INFO1, "CONGRATULATIONS! joined status %hd, addr 0x%x, ch %hd rejoin %hd",
                  (FMT__H_D_H_H, confirm->status, confirm->network_address, ZB_PIBCACHE_CURRENT_CHANNEL(), ZG->zdo.handle.rejoin));
        TRACE_MSG(TRACE_INFO1, "xpanid " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(confirm->extended_pan_id)));

        ZB_SCHEDULE_CALLBACK(zdo_join_done, param);
    }
    else
    {
        ZB_SCHEDULE_CALLBACK(zdo_commissioning_join_failed, param);
    }

    TRACE_MSG(TRACE_ZDO1, "<<nlme_join_conf", (FMT__0));
}


void zdo_authenticated_send_device_annce(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ZDO1, "zdo_authenticated_send_device_annce %hd", (FMT__H, param));

    /* Since we are here. we are authenticated ok - cancel alarm */
    ZB_SCHEDULE_ALARM_CANCEL(zdo_authentication_failed, 0);
    /* Join and auth are ok - now start ZB traffic watchdog */
#ifdef ZB_USE_ZB_TRAFFIC_WATCHDOG
    zb_add_watchdog(ZB_WD_ZB_TRAFFIC, ZB_WD_ZB_TRAFFIC_TO);
    /* Let's application enable Zigbee watchdog */
    ZB_DISABLE_WATCHDOG(ZB_WD_ZB_TRAFFIC);
#endif
    /* Joined and authenticated - reset scanlist. */
    zdo_reset_scanlist(ZB_TRUE);
    /* Clear used_pan_addr, store only 1 record - our used pan id. */
    {
        zb_address_clear_pan_id_table(ZB_NIB_EXT_PAN_ID());
    }

    /* Remember device annce param to continue commissioning */
    ZG->zdo.handle.dev_annce_param = param;
    ZB_SCHEDULE_CALLBACK(zdo_send_device_annce, param);
}


void zdo_send_device_annce(zb_uint8_t param)
{
    zb_zdo_device_annce_t da;

    TRACE_MSG(TRACE_ZDO1, "zdo_send_device_annce %hd", (FMT__H, param));

    ZDO_TSN_INC();
    da.tsn = ZDO_CTX().tsn;
    ZB_HTOLE16((zb_uint8_t *)&da.nwk_addr, &ZB_PIBCACHE_NETWORK_ADDRESS());
    ZB_IEEE_ADDR_COPY(da.ieee_addr, ZB_PIBCACHE_EXTENDED_ADDRESS());
    da.capability = 0;
    ZB_MAC_CAP_SET_ALLOCATE_ADDRESS(da.capability, ZB_B2U(ZG->nwk.handle.rejoin_capability_alloc_address));

    TRACE_MSG(TRACE_ATM1, "Z< send device annce, nwk address: 0x%04x", (FMT__D, da.nwk_addr));

#ifdef ZB_ROUTER_ROLE
    if (ZB_IS_DEVICE_ZR())
    {
        ZB_MAC_CAP_SET_ROUTER_CAPS(da.capability);
        /* ZB_MAC_CAP_SET_SECURITY means high security mode - never set it */
    }
    else
#endif
    {
        if (ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()))
        {
            ZB_MAC_CAP_SET_RX_ON_WHEN_IDLE(da.capability, 1U);
            ZB_MAC_CAP_SET_POWER_SOURCE(da.capability, 1U);
        }
    }

    zdo_send_device_annce_ex(param, &da
#ifdef ZB_USEALIAS
                             , ZB_FALSE
#endif
                            );
}


#ifdef ZB_REJOIN_BACKOFF

/* Note: rejoin backof is not standard and oriented to legacy HA
 * commissioning. Do we still need to support it? */

void zdo_rejoin_backoff_initiate_rejoin(zb_uint8_t param)
{
    switch (ZB_COMMISSIONING_TYPE())
    {
#ifdef ZB_BDB_MODE
    case ZB_COMMISSIONING_BDB:
    {
        zb_uint8_t *rejoin_reason = ZB_BUF_GET_PARAM(param, zb_uint8_t);
        *rejoin_reason = ZB_REJOIN_REASON_BACKOFF_REJOIN;
        zdo_commissioning_initiate_rejoin(param);
    }
    break;
#endif
#ifndef ZB_LITE_BDB_ONLY_COMMISSIONING
    case ZB_COMMISSIONING_CLASSIC:
        if (ZDO_CTX().zdo_rejoin_backoff.rjb_scan_all_chan_mask == ZB_TRUE)
        {
            zb_channel_page_list_copy(ZB_AIB().aps_channel_mask_list, ZDO_CTX().zdo_rejoin_backoff.rjb_chan_mask_list);
        }
        else
        {
            ZB_AIB().aps_channel_mask_list[ZB_CHANNEL_PAGE_TO_IDX(ZB_PIBCACHE_CURRENT_PAGE())/* MMDEVSTUBS */] = (1l << ZB_PIBCACHE_CURRENT_CHANNEL());
        }
        zdo_initiate_rejoin(param, ZB_NIB_EXT_PAN_ID(),
                            ZB_AIB().aps_channel_mask_list,
                            ZDO_CTX().zdo_rejoin_backoff.rjb_do_secure);
        break;
#ifdef ZB_SE_COMMISSIONING
    case ZB_COMMISSIONING_SE:
        /* FIXME: Define SE_COMM_SIGNAL_NWK_REJOIN_BACKOFF */
        //se_commissioning_signal(SE_COMM_SIGNAL_NWK_REJOIN_BACKOFF, param);
        break;
#endif
#endif  /* #ifndef ZB_LITE_BDB_ONLY_COMMISSIONING */
    default:
        break;
    }
    ZDO_CTX().zdo_rejoin_backoff.rjb_state = ZB_ZDO_REJOIN_BACKOFF_COMMAND_SENT;
}


void zdo_rejoin_backoff_int(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ZDO1, ">> zdo_rejoin_backoff_int %hd", (FMT__H, param));
    ZVUNUSED(param);

    ZDO_CTX().zdo_rejoin_backoff.rjb_state = ZB_ZDO_REJOIN_BACKOFF_COMMAND_SCHEDULED;

    zb_buf_get_out_delayed(zdo_rejoin_backoff_initiate_rejoin);
    TRACE_MSG(TRACE_ZDO1, "<< zdo_rejoin_backoff_int", (FMT__0));
}

/* Function starts Rejoin backof algorithm */
/* - triggered by parent link failure */
/* - . . .                             */
zb_ret_t zb_zdo_rejoin_backoff_start(zb_bool_t insecure_rejoin)
{
    zb_ret_t ret = RET_OK;

    /* If predefined number of MAC fails appeared, start Rejoin
     * backoff: try to rejoin network with timeout
     * 2-4-8-16-32-... seconds; maximum timeout is 30 minutes */

    TRACE_MSG(TRACE_ZDO1, ">> zb_zdo_rejoin_backoff_start insecure_rejoin %hd", (FMT__H, insecure_rejoin));

    if (ZDO_CTX().zdo_rejoin_backoff.rjb_state == ZB_ZDO_REJOIN_BACKOFF_IDLE)
    {
        /* Currently we are not in "rejoin backoff" state, let's start it */
        ZDO_CTX().zdo_rejoin_backoff.rjb_cnt = 1;
        ZDO_CTX().zdo_rejoin_backoff.rjb_state = ZB_ZDO_REJOIN_BACKOFF_TIMER_RUNNING;
        ZDO_CTX().zdo_rejoin_backoff.rjb_insecure = insecure_rejoin;
        ZDO_CTX().zdo_rejoin_backoff.rjb_do_secure = ZB_TRUE;
        ZDO_CTX().zdo_rejoin_backoff.rjb_scan_all_chan_mask = ZB_FALSE;

        zb_channel_page_list_copy(ZDO_CTX().zdo_rejoin_backoff.rjb_chan_mask_list, ZB_AIB().aps_channel_mask_list);

        ZB_SCHEDULE_ALARM_CANCEL(zdo_rejoin_backoff_int, ZB_ALARM_ANY_PARAM);
        ZB_SCHEDULE_ALARM(zdo_rejoin_backoff_int, 0,
                          ZB_ZDO_REJOIN_BACKOFF_TIMEOUT(ZDO_CTX().zdo_rejoin_backoff.rjb_cnt));
    }
    else
    {
        ret = RET_ALREADY_EXISTS;
        TRACE_MSG(TRACE_ZDO1, "zb_zdo_rejoin_backoff_start, already running", (FMT__0));
    }

    TRACE_MSG(TRACE_ZDO1, "<< zb_zdo_rejoin_backoff_start ret %hd", (FMT__H, ret));
    return ret;
}

void zb_zdo_rejoin_backoff_cancel()
{
    TRACE_MSG(TRACE_ZDO1, ">> zb_zdo_rejoin_backoff_cancel state %hd",
              (FMT__H, ZDO_CTX().zdo_rejoin_backoff.rjb_state));

    if (ZDO_CTX().zdo_rejoin_backoff.rjb_state == ZB_ZDO_REJOIN_BACKOFF_TIMER_RUNNING)
    {
        /* stop the timer */
        ZB_SCHEDULE_ALARM_CANCEL(zdo_rejoin_backoff_int, ZB_ALARM_ANY_PARAM);
    }
    ZDO_CTX().zdo_rejoin_backoff.rjb_state = ZB_ZDO_REJOIN_BACKOFF_IDLE;
    ZDO_CTX().zdo_rejoin_backoff.rjb_cnt = 0;

    TRACE_MSG(TRACE_ZDO1, "<< zb_zdo_rejoin_backoff_cancel", (FMT__0));
}

void zb_zdo_rejoin_backoff_continue(zb_uint8_t param)
{
    /* Continue rejoin until predefined number of rejoins is
     * reached. After that continue rejoin when alarm notification
     * appears or predefined supervision interval expires */

    ZVUNUSED(param);
    TRACE_MSG(TRACE_ZDO1, ">> zb_zdo_rejoin_backoff_continue cnt %hd, state %hd",
              (FMT__H_H, ZDO_CTX().zdo_rejoin_backoff.rjb_cnt,
               ZDO_CTX().zdo_rejoin_backoff.rjb_state));

    if (ZDO_CTX().zdo_rejoin_backoff.rjb_state == ZB_ZDO_REJOIN_BACKOFF_COMMAND_DONE)
    {
        ZDO_CTX().zdo_rejoin_backoff.rjb_cnt++;
        if (ZDO_CTX().zdo_rejoin_backoff.rjb_cnt > ZB_ZDO_REJOIN_BACKOFF_MAX_COUNT)
        {
            /* Maximum predefined rejoin backoff timeout
             * ZB_ZDO_REJOIN_BACKOFF_MAX_COUNT == 30 min (1800 sec). Max
             * timeout using ZB_ZDO_REJOIN_BACKOFF_MAX_COUNT: (1 << 11) == 2048,
             * close to 1800 => lets use this simple check */
            ZDO_CTX().zdo_rejoin_backoff.rjb_cnt = ZB_ZDO_REJOIN_BACKOFF_MAX_COUNT;
        }

        if (ZDO_CTX().zdo_rejoin_backoff.rjb_cnt == 2)
        {
            /* Second attempt - do secure rejoin using all channels in mask. */
            ZDO_CTX().zdo_rejoin_backoff.rjb_scan_all_chan_mask = ZB_TRUE;
        }
        else if (ZDO_CTX().zdo_rejoin_backoff.rjb_cnt == 3)
        {
            /* Third attempt - do unsecure rejoin using all channels in mask.
               Other attempts will also be unsecured. */
            ZDO_CTX().zdo_rejoin_backoff.rjb_do_secure = (zb_bool_t)!(ZDO_CTX().zdo_rejoin_backoff.rjb_insecure == ZB_TRUE);
        }

        /* cancel the timer - just in case... (in common - it shouldn't run for this moment) */
        ZB_SCHEDULE_ALARM_CANCEL(zdo_rejoin_backoff_int, ZB_ALARM_ANY_PARAM);

        ZB_SCHEDULE_ALARM(zdo_rejoin_backoff_int, 0,
                          ZB_ZDO_REJOIN_BACKOFF_TIMEOUT(ZDO_CTX().zdo_rejoin_backoff.rjb_cnt));
        ZDO_CTX().zdo_rejoin_backoff.rjb_state = ZB_ZDO_REJOIN_BACKOFF_TIMER_RUNNING;
    }

    TRACE_MSG(TRACE_ZDO1, "<< zb_zdo_rejoin_backoff_continue", (FMT__0));
}

void zb_zdo_rejoin_backoff_iteration_done()
{
    TRACE_MSG(TRACE_ZDO1, ">> zb_zdo_rejoin_backoff_done state %hd",
              (FMT__H, ZDO_CTX().zdo_rejoin_backoff.rjb_state));

    if (ZDO_CTX().zdo_rejoin_backoff.rjb_state > ZB_ZDO_REJOIN_BACKOFF_IDLE)
    {
        ZB_SCHEDULE_ALARM_CANCEL(zdo_rejoin_backoff_int, ZB_ALARM_ANY_PARAM);
        ZDO_CTX().zdo_rejoin_backoff.rjb_state = ZB_ZDO_REJOIN_BACKOFF_COMMAND_DONE;
    }
    TRACE_MSG(TRACE_ZDO1, "<< zb_zdo_rejoin_backoff_done state %hd", (FMT__D, ZDO_CTX().zdo_rejoin_backoff.rjb_state));
}

zb_bool_t zb_zdo_rejoin_backoff_is_running()
{
    zb_bool_t ret = (zb_bool_t)(ZDO_CTX().zdo_rejoin_backoff.rjb_state > ZB_ZDO_REJOIN_BACKOFF_IDLE);
    TRACE_MSG(TRACE_ZDO1, "zb_zdo_rejoin_backoff_is_running %hd", (FMT__H, ret));
    return ret;
}

zb_bool_t zb_zdo_rejoin_backoff_force()
{
    zb_bool_t ret = ZB_FALSE;

    TRACE_MSG(TRACE_ZDO1, ">> zb_zdo_rejoin_backoff_force rejoin state %hd, rejoin cnt %hd, rejoin do_secure %hd",
              (FMT__H_H_H, ZDO_CTX().zdo_rejoin_backoff.rjb_state,
               ZDO_CTX().zdo_rejoin_backoff.rjb_cnt, ZDO_CTX().zdo_rejoin_backoff.rjb_do_secure));

    /* check if rejoin backoff is going on and its counter... */
    if (ZDO_CTX().zdo_rejoin_backoff.rjb_state > ZB_ZDO_REJOIN_BACKOFF_IDLE)
    {
        /* if rejoin backoff is on and we are waiting for the longest
         * predefined timeout value to try rejoining, start rejoining
         * right now GP req. Rejoin-backoff */

        /* Prevent restarting intervals when we are not in insecure rejoin. */
        /* Background: if counter is restarted from very beginning, there is no chance to move to
           all-channels mask usage and following unsecure rejoin */
        if (ZDO_CTX().zdo_rejoin_backoff.rjb_cnt > 3)
        {
            ZDO_CTX().zdo_rejoin_backoff.rjb_cnt = 0;

            if (ZDO_CTX().zdo_rejoin_backoff.rjb_state == ZB_ZDO_REJOIN_BACKOFF_TIMER_RUNNING)
            {
                /* stop and restart the timer */
                ZB_SCHEDULE_ALARM_CANCEL(zdo_rejoin_backoff_int, ZB_ALARM_ANY_PARAM);
                ZDO_CTX().zdo_rejoin_backoff.rjb_state = ZB_ZDO_REJOIN_BACKOFF_COMMAND_DONE;
                ZB_SCHEDULE_CALLBACK(zb_zdo_rejoin_backoff_continue, 0);

                ret = ZB_TRUE;
            }
            /* else, a rejoin is ongoing. After the scan, zdo_startup_complete will be called and
               only then the zb_zdo_rejoin_backoff_continue must be called */
        }
    }

    TRACE_MSG(TRACE_ZDO1, "<< zb_zdo_rejoin_backoff_force, ret %hd", (FMT__H, ret));
    return ret;
}
#endif /* ZB_REJOIN_BACKOFF */

#endif /* !defined NCP_MODE_HOST */

#endif /* ZB_JOIN_CLIENT */


/*! @} */
