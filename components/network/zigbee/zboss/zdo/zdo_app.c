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
/* PURPOSE: zdo_app.c was unsiructured collection of intermediate commissioning logic and service routines.
   Now client Join functionality is moved to zdo_app_join.c, LEAVE processing is moved to zdo_app_leave.c,
   production configuration load is moved to zdo_app_prod_conf.c.
   Such a dividing is done during NCP Host development in attempt to group Host and SoC functional.

   In NCP architecture that file is at SoC.
*/

#define ZB_TRACE_FILE_ID 2091
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
#include "zdo_diagnostics.h"

#ifdef ZB_ENABLE_ZGP
#include "zboss_api_zgp.h"
#include "zgp/zgp_internal.h"
#endif

#if defined ZB_ENABLE_ZLL
#include "zll/zb_zll_common.h"
#include "zll/zb_zll_nwk_features.h"
#endif /* defined ZB_ENABLE_ZLL */

/*! \addtogroup ZB_ZDO */
/*! @{ */

#ifndef NCP_MODE_HOST

#if defined ZB_ENABLE_ZLL
void zb_zdo_startup_complete_zll(zb_uint8_t param);
#endif

static void init_config_attr(void);

static void zdo_dev_continue_start_after_nwk(zb_uint8_t param);
static void zb_nlme_status_indication_process(zb_uint8_t new_buf, zb_uint16_t initial_buf);


/************************************
Internal initialization
SoC only.
*/

static void zb_zdo_init_tran_table(void)
{
    zb_uint8_t i;

    ZB_BZERO(ZDO_CTX().zdo_cb, sizeof(zdo_cb_hash_ent_t) * ZDO_TRAN_TABLE_SIZE);

    for (i = 0; i < ZDO_TRAN_TABLE_SIZE; ++i)
    {
        zdo_cb_hash_ent_t *ent = &ZDO_CTX().zdo_cb[i];

        ent->tsn = ZB_ZDO_INVALID_TSN;
    }
}


void zb_zdo_init(void)
{
    ZDO_CTX().max_parent_threshold_retry = ZB_ZDO_MAX_PARENT_THRESHOLD_RETRY;
#ifndef ZB_LITE_NO_END_DEVICE_BIND
    ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZB_ZDO_BIND_DEV_1].end_device_bind_param = ZB_UNDEFINED_BUFFER;
    ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZB_ZDO_BIND_DEV_2].end_device_bind_param = ZB_UNDEFINED_BUFFER;
#endif
#ifdef ZB_ZDO_DENY_LEAVE_CONFIG
    ZDO_CTX().leave_req_allowed = 1;
#endif

    /* All g_zb variables are inited to 0 with memset */

#if !defined ZB_LITE_NO_ZDO_POLL
    zb_zdo_pim_init_defaults();
#endif

    init_config_attr();

    /* TODO: Enable when Enhanced NWK Update Notify will be implemented:
       2.4.4.4.9  Mgmt_NWK_Update_notify
       This message shall not be sent unsolicited - use Mgmt_NWK_Unsolicited_Enhanced_Update_notify
       instead.
    */
    /* ZDO_CTX().nwk_upd_notify_pkt_limit = ZB_ZDO_CHECK_FAILS_NWK_UPDATE_NOTIFY_LIMIT; */

#if defined ZB_JOINING_LIST_SUPPORT && defined ZB_ROUTER_ROLE
    ZDO_CTX().joining_list_ctx.is_consistent = ZB_TRUE;
    ZDO_CTX().joining_list_ctx.update_id = 0;
    ZDO_CTX().joining_list_ctx.list_expiry_interval = ZB_JOINING_LIST_DEFAULT_EXPIRY_INTERVAL; /* default value for mibIeeeExpiryInterval */
#endif /* defined ZB_JOINING_LIST_SUPPORT && defined ZB_ROUTER_ROLE */
    zb_zdo_init_tran_table();

    ZDO_DIAGNOSTICS_INIT();
}


static void init_config_attr()
{
    zdo_commissioning_init();

#ifndef ZB_LITE_NO_END_DEVICE_BIND
    ZDO_CTX().conf_attr.enddev_bind_timeout = ZB_ZDO_ENDDEV_BIND_TIMEOUT;
#endif
    ZDO_CTX().conf_attr.permit_join_duration = ZB_DEFAULT_PERMIT_JOINING_DURATION;

    zb_set_zdo_descriptor();
}

/**
   Check that device is joined.

   It must be a function to exclude exposing of ZDO globals via user's API.
 */
zb_bool_t zb_zdo_joined()
{
    return (zb_bool_t)ZB_TCPOL().node_is_on_a_network;
}

zb_bool_t zb_zdo_authenticated(void)
{
    return ZG->aps.authenticated;
}


zb_bool_t zb_zdo_tclk_valid(void)
{
#ifndef ZB_COORDINATOR_ONLY
    zb_aps_device_key_pair_set_t *aps_key;
#endif
    zb_bool_t ret = ZB_FALSE;

#ifndef ZB_COORDINATOR_ONLY
    aps_key = zb_secur_get_link_key_by_address(ZB_AIB().trust_center_address, ZB_SECUR_VERIFIED_KEY);
    if ((aps_key != NULL)
            || ZB_IEEE_ADDR_CMP(ZB_AIB().trust_center_address, ZB_PIBCACHE_EXTENDED_ADDRESS())
#if defined ZB_DISTRIBUTED_SECURITY_ON
            || IS_DISTRIBUTED_SECURITY()
#endif
       )
#endif /* ZB_COORDINATOR_ONLY */
    {
        ret = ZB_TRUE;
    }
    return ret;
}


#ifdef ZB_JOIN_CLIENT
zb_bool_t zdo_secur_waiting_for_tclk_update()
{
    zb_bool_t ret = ZB_FALSE;

    if (!ZB_IS_TC() && zb_zdo_joined())
    {
        ret = (zb_bool_t)(
                  !IS_DISTRIBUTED_SECURITY() &&
                  ZB_TCPOL().waiting_for_tclk_exchange);
    }

    TRACE_MSG(TRACE_SECUR1, "zdo_secur_waiting_for_tclk_update ret %d", (FMT__D, ret));
    return ret;
}
#endif  /* ZB_JOIN_CLIENT */


#if defined ZB_SUBGHZ_BAND_ENABLED
static void fix_bands_depending_on_dev_type(void)
{
    zb_uint32_t aib_ch_page_list = zb_aib_channel_page_list_get_2_4GHz_mask();
    /* Selection device ZC is impossible, so, if ZC has non-zero mask for 2.4GHz, cleag all sub-ghz masks.
       Sub-GHz ZR is impossible, so clear sub-ghz masks.
     */
    if (
        (
            aib_ch_page_list != 0U
            && !ZB_SUBGHZ_SWITCH_MODE()
            /*cstat !MISRAC2012-Rule-14.3_b */
            /** @mdr{00012,40} */
            && ZB_IS_DEVICE_ZC()
        )
#if !defined ZB_SUB_GHZ_ZB30_SUPPORT
        || ZB_IS_DEVICE_ZR()
#endif
    )
        /*cstat !MISRAC2012-Rule-2.1_b */
        /** @mdr{00012,41} */
    {
        zb_aib_channel_page_list_set_2_4GHz_mask(aib_ch_page_list);
    }
}
#endif /* ZB_SUBGHZ_BAND_ENABLED */


/* That function should be made internal. Used in only one app. */
zb_ret_t zb_zdo_dev_init(void)
{
    TRACE_MSG(TRACE_ZDO1, "zb_zdo_dev_init", (FMT__0));
#ifdef ZB_USE_NVRAM
    zb_nvram_load();
#endif

#ifdef ZB_PRODUCTION_CONFIG
    zdo_load_production_config();
#endif

#ifdef ZB_REGRESSION_TESTS_API
    if (!ZB_REGRESSION_TESTS_API().allow_zero_ieee_addr)
#endif
    {
        ZB_ASSERT(!ZB_IEEE_ADDR_IS_ZERO(ZB_PIBCACHE_EXTENDED_ADDRESS()));
    }

#ifdef ZB_SUBGHZ_BAND_ENABLED
    fix_bands_depending_on_dev_type();
#endif /* ZB_SUBGHZ_BAND_ENABLED */

#if defined ZB_MACSPLIT_HOST
    /* MP: At this point the configuration data was loaded, so macsplit transport initialization
     * is able to find out (with a little helpp of the weak functions redefined by the application
     * programmer) how to handle the initialization process.
     * For example it is able to decide which of two (possible) UART ports to use provided that one is
     * for regular 2.4 GHz ZigBee communications, and another is the sub-GHZ band.
     *
     * Small note on initialization point selection:
     * Some times it is impossible impossible to evaluate transport initialization parameters before
     * productiopn configuration loaded. On the other hand it is impossible to move it to the latest
     * point possible like mlme-reset.request because this request will be interrupted.
     * Macsplit transport initialization begins with SoC reset. After that the host waits for the
     * first (boot indication) fromthe SoC. The application code is to handle this first packet in the
     * BDB signal handler (namely ZB_MACSPLIT_DEVICE_BOOT signal). This packet may actually indicate
     * that SoC is in some special execution mode (for example, bootloader), so must be communicated
     * in the special way or SoC update required.
     * The possible necessity of the special handling for this first packet from the SoC forces
     * macsplit transport initialization as early as possible (that is: here).
     */
    zb_macsplit_transport_init();
#endif /* defined ZB_MACSPLIT_HOST */

    /* Init node descriptor here.
       Rationale: we need to know aps_designated_coordinator value before init, so can not do it from zb_init().
     */
#ifdef ZB_ED_FUNC
    if (ZB_IS_DEVICE_ZED())
    {
#if !(ZB_TH_ENABLED)
        zb_set_default_ed_descriptor_values();
#endif
    }
#endif  /* ZB_ED_FUNC */

#if defined ZB_ROUTER_ROLE
    {
        zb_set_default_ffd_descriptor_values((ZB_AIB().aps_designated_coordinator == 1U) ? ZB_COORDINATOR : ZB_ROUTER);
    }
#endif  /* ZB_ROUTER_ROLE */

#ifdef ZB_ENABLE_ZGP
    if (ZB_IS_DEVICE_ZC_OR_ZR())
    {
        ZB_SCHEDULE_CALLBACK(zgp_init_by_scheduler, 0);
    }
#endif /* ZB_ENABLE_ZGP */

#if (defined ZB_MAC_POWER_CONTROL) && (!defined ZB_LITE_NO_CONFIGURABLE_POWER_DELTA)
    if (ZB_IS_DEVICE_ZC_OR_ZR() ||
            (ZB_IS_DEVICE_ZED() && ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE())))
    {
        ZB_NIB_NWK_LPD_CMD_MODE() = ZB_NWK_LPD_NOTIFICATION;
    }
    else
    {
        ZB_NIB_NWK_LPD_CMD_MODE() = ZB_NWK_LPD_REQUEST;
    }
#endif  /* ZB_MAC_POWER_CONTROL && !ZB_LITE_NO_CONFIGURABLE_POWER_DELTA */


#if defined ZB_DISTRIBUTED_SECURITY_ON
    /* Looks like we mustn't set distiributed flag just now! We might
     * assign TC address to -1 to allow distributed formation, but still
     * join to centralized network. */
    //zb_sync_distributed();
#endif

    ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_NUMBER_OF_RESETS_ID);

    return RET_OK;
}


/* Initiate device start (including commissioning). To be used in monolythic builds only */
zb_ret_t zdo_dev_start()
{
    (void)zb_zdo_dev_init();
    /* Call start procedure via callback to get trace earlier on Cortex-M4 with
     * alien MAC: trace works when alien's MAC scheduling. */
#ifndef ZB_MACSPLIT_HOST
    /* In case of macsplit host device should be started on receiving
     * ZB_MACSPLIT_DEVICE_BOOT signal via zboss_start_continue */
    ZB_SCHEDULE_CALLBACK(zb_zdo_dev_start_cont, 0);
#endif
#ifdef ZB_MACSPLIT_HOST
    /* Flag is used for macsplit host only */
    ZG->zdo.handle.start_no_autostart = ZB_FALSE;
#endif

    return RET_OK;
}


/* Monolythic or SoC part of NCP device start */
void zb_zdo_dev_start_cont(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ZDO1, "> zb_zdo_dev_start_cont %hd", (FMT__H, param));

    ZB_ASSERT(param == 0U);

    /* Startup procedure as defined in 2.5.5.5.6.2    Startup Procedure */
    if (ZDO_CTX().continue_start_after_nwk_cb == NULL)
    {
        /* What to run after NWK init complete. It can be normal start, sniffer or touchlink start  */
        ZDO_CTX().continue_start_after_nwk_cb = zdo_dev_continue_start_after_nwk;
    }

#ifdef ZB_JOIN_CLIENT
    /* We could be here with already filled scanlist. Call startup_complete_int here if
     * scanref exists.
     * EE:FIXME: when? I can't find for sure.
     */
    if (COMM_CTX().discovery_ctx.scanlist_ref != 0U)
    {
        TRACE_MSG(TRACE_ZDO1, "already have scanlist, try it first", (FMT__0));
        zdo_commissioning_join_via_scanlist(COMM_CTX().discovery_ctx.scanlist_ref);
        return;
    }
#endif  /* #ifndef ZB_COORDINATOR_ONLY */

    ZVUNUSED(param);

    {
        zb_bufid_t buf = zb_buf_get_out();
        /* At start we have free buffer for sure, so no need to use blocking buffer alloc */
        ZB_ASSERT(buf);
        TRACE_MSG(TRACE_APS1, "ext pan id 0 - startup", (FMT__0));
        /* Reset NWK. It will reset MAC and call zb_nlme_reset_confirm(). */
        /* Do not need to do nib_reinit here - at least if we are NFN! */
        zdo_call_nlme_reset(buf, ZB_FALSE, ZB_FALSE, zb_nwk_load_pib);
    }
    TRACE_MSG(TRACE_ZDO1, "< zb_zdo_dev_start_cont", (FMT__0));
}


/**
   Continue start ctart and commissioning after NWK layer successfully started
*/
static void zdo_dev_continue_start_after_nwk(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ZDO1, "> zdo_dev_continue_start_after_nwk param %hd", (FMT__H, param));

    zdo_commissioning_start(param);

    TRACE_MSG(TRACE_ZDO1, "< zdo_dev_continue_start_after_nwk", (FMT__0));
}


/**
   Finish start procedure, send legacy startup complete signal.

   Mainly used for classic commissioning (called from classic and SE; calls from SE are to be eliminated)
 */
void zb_zdo_startup_complete_int_delayed(zb_uint8_t param, zb_uint16_t user_param)
{
    /* user_param contains status. Modify if needed to pass signal etc. */
    TRACE_MSG(TRACE_ZDO1, "Calling zdo_startup_complete", (FMT__0));
    (void)zb_app_signal_pack(param, ZB_ZDO_SIGNAL_DEFAULT_START, (zb_int16_t)user_param, 0);
    ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete_int, param);
}

void zb_zdo_device_first_start_int_delayed(zb_uint8_t param, zb_uint16_t user_param)
{
    /* user_param contains status. Modify if needed to pass signal etc. */
    (void)zb_app_signal_pack(param, ZB_SIGNAL_DEVICE_FIRST_START, (zb_int16_t)user_param, 0);
    ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete_int, param);
}

void zb_zdo_device_reboot_int_delayed(zb_uint8_t param, zb_uint16_t user_param)
{
    /* user_param contains status. Modify if needed to pass signal etc. */
    (void)zb_app_signal_pack(param, ZB_SIGNAL_DEVICE_REBOOT, (zb_int16_t)user_param, 0);
    ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete_int, param);
}

void zb_zdo_device_zed_start_rejoin(zb_uint8_t param, zb_uint16_t user_param)
{
    /* user_param contains status. Modify if needed to pass signal etc. */
    (void)zb_app_signal_pack(param, ZB_SIGNAL_DEVICE_START_REJOIN, (zb_int16_t)user_param, 0);
    ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete_int, param);
}

/**
   Complete the internal startup iteration.
 */
void zb_zdo_startup_complete_int(zb_uint8_t param)
{
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    ZVUNUSED(sig);
    TRACE_MSG(TRACE_ZDO1, "> zb_zdo_startup_complete_int param %hd status %hd signal %hd", (FMT__H_H_H, param, zb_buf_get_status(param), sig));

#ifdef ZB_JOIN_CLIENT
    /* If status is not OK and scanlist exists, continue joining with scanlist. */
    /* (NK) Scanlist could be inited and using only during discovery process (forrejoin it is 0
     * all the time). It will be freed after successfull authenication (on device annce), so while
     * authentication is not completed and status is not ok, reuse the scanlist. If scanlist_ref is
     * not zero, it is ready to use. Also pass the startup_complete in any case when status is success. */
    if (zb_buf_get_status(param) == (zb_ret_t)ZB_NWK_STATUS_SUCCESS && ZB_U2B(COMM_CTX().discovery_ctx.scanlist_ref))
    {
        COMM_CTX().discovery_ctx.scanlist_join_attempt_n = ZB_ZDO_MAX_JOINING_TRY_ATTEMPTS;
    }

#endif  /* ZB_JOIN_CLIENT */
    ZB_ASSERT((param != 0U) && (param != ZB_UNDEFINED_BUFFER));

#if defined xZB_ENABLE_ZLL
    /* ZLL-only - do not uncomment! */
    zb_zdo_startup_complete_zll(param);
#else

#ifdef ZB_REJOIN_BACKOFF
    zb_zdo_rejoin_backoff_iteration_done();
#endif
    ZG->zdo.handle.rejoin = ZB_FALSE;

    /* Send signal to the app */
    zboss_signal_handler(param);
#endif  /* !zll */
    TRACE_MSG(TRACE_ZDO1, "< zb_zdo_startup_complete_int", (FMT__0));
}


#ifdef ZB_ROUTER_ROLE
void zb_nlme_permit_joining_confirm(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ZDO1, "zb_nlme_permit_joining_confirm status %hd",
              (FMT__H, zb_buf_get_status(param)));

    ZB_SCHEDULE_CALLBACK(zdo_mgmt_permit_joining_resp_cli, param);
}

void zb_nlme_join_indication(zb_uint8_t param)
{
    zb_nlme_join_indication_t *ind = ZB_BUF_GET_PARAM(param, zb_nlme_join_indication_t);

    TRACE_MSG(TRACE_ZDO1, "JOINED (st %hd) dev %d/" TRACE_FORMAT_64 " cap: dev type %hd rx.w.i. %hd rejoin %hd secur %hd",
              (FMT__H_D_A_H_H_H_H,
               (zb_uint8_t)zb_buf_get_status(param),
               (zb_uint16_t)ind->network_address,
               TRACE_ARG_64(ind->extended_address),
               ZB_MAC_CAP_GET_DEVICE_TYPE(ind->capability_information),
               ZB_MAC_CAP_GET_RX_ON_WHEN_IDLE(ind->capability_information),
               ind->rejoin_network, ind->secure_rejoin));

    if (ind->rejoin_network != ZB_NLME_REJOIN_METHOD_CHANGE_CHANNEL)
    {
        ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_JOIN_INDICATION_ID);
    }

#if defined ZB_MAC_POWER_CONTROL
    if (ZB_LOGICAL_PAGE_IS_SUB_GHZ(ZB_PIBCACHE_CURRENT_PAGE()))
    {
        zb_buf_get_out_delayed_ext(zb_nwk_lpd_joined_parent, ind->network_address, 0);
    }
#else
    ZVUNUSED(ind);
#endif  /* ZB_MAC_POWER_CONTROL */

    if (ZB_IS_DEVICE_ZC_OR_ZR()
            && ZB_NIB_SECURITY_LEVEL() != 0U)
    {
        /* Authenticate device: send network key to it */
        ZB_SCHEDULE_CALLBACK(secur_authenticate_child, param);
    }
    else
    {
        zb_buf_free(param);
    }

#ifdef ZB_USE_NVRAM
    zb_nvram_store_addr_n_nbt();
#endif

}
#endif  /* ZB_ROUTER_ROLE */


#ifdef ZB_JOIN_CLIENT
/**
   Reaction on inability to receive & decrypt NWK key after join.

   This function is used as an alarm or immediate handling of auth failure.
 */
void zdo_authentication_failed(zb_uint8_t param)
{
    if (param == 0U)
    {
        zb_ret_t ret = zb_buf_get_out_delayed(zdo_authentication_failed);
        if (ret != RET_OK)
        {
            TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
        }
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Have not got nwk key - authentication failed", (FMT__0));

#if !defined ZB_LITE_NO_ZDO_POLL
        if (!ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()))
        {
            zb_zdo_pim_stop_poll(0);
        }
#endif
        ZB_SCHEDULE_CALLBACK(zdo_commissioning_authentication_failed, param);
    }
}

void zdo_aps_decryption_failed(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ZDO2, ">> zdo_aps_decryption_failed param %hd", (FMT__H, param));

    ZVUNUSED(param);
    ZB_ASSERT(param == 0U);
    /* Unability to decrypt aps packet is authentication failure only if we are
     * not authorized and this is TK packet. We still can get APS encrypted
     * packet from some 'left' device - this is not the reason to leave network. */
    if (!ZG->aps.authenticated)
    {
        /*
         * Should all this scanlist stuff be moved into dedicated macro?
         */
        COMM_CTX().discovery_ctx.scanlist_join_attempt_n = ZB_ZDO_MAX_JOINING_TRY_ATTEMPTS;

        /*
         * Confirm and respose races are possible in case of th as for example packet and
         * mlme_set commands are equivalent operations so long time operation as mlme_set
         * command can cause situation when zdo_authentication_failed can be called twice
         * (by alarm and here). For default stack this is unpossible situation so condition
         * is not needed. Also condition here blocks zdo_authentication_failed() call.
         */
#if defined ZB_TH_ENABLED
        if (RET_OK != zb_schedule_alarm_cancel(zdo_authentication_failed, 0, NULL))
        {
            ZB_SCHEDULE_CALLBACK(zdo_authentication_failed, 0);
        }
#else
        ZB_SCHEDULE_ALARM_CANCEL(zdo_authentication_failed, ZB_ALARM_ALL_CB);
        ZB_SCHEDULE_CALLBACK(zdo_authentication_failed, 0);
#endif  /* ZB_TH_ENABLED */

    }

    TRACE_MSG(TRACE_ZDO2, "<< zdo_aps_decryption_failed", (FMT__0));
}
#endif  /* ZB_JOIN_CLIENT */

#if defined ZB_ROUTER_ROLE && !defined ZB_COORDINATOR_ONLY
void zb_nlme_start_router_confirm(zb_uint8_t param)
{

#ifdef xZB_ENABLE_ZLL
    zb_bufid_t buf = NULL;
#endif
    TRACE_MSG(TRACE_ZDO1, ">>zb_nlme_start_router_confirm", (FMT__0));
#ifdef ZB_USE_NVRAM
    /* If we fail, trace is given and assertion is triggered */
    (void)zb_nvram_write_dataset(ZB_NVRAM_COMMON_DATA);
#endif

    if (ZB_NIB().ed_child_num != 0U)
    {
        /* Just in case */
        ZB_SCHEDULE_ALARM_CANCEL(zdo_send_parent_annce, 0);
        ZB_SCHEDULE_ALARM(zdo_send_parent_annce, 0, ZB_PARENT_ANNCE_JITTER());
    }

    ZB_SCHEDULE_CALLBACK(zdo_comm_set_permit_join_after_router_start, param);

    TRACE_MSG(TRACE_ZDO1, "<<zb_nlme_start_router_confirm", (FMT__0));
}


void zb_zdo_start_router(zb_uint8_t param)
{
    zb_nlme_start_router_request_t *request;

    TRACE_MSG(TRACE_ZDO1, "zb_zdo_start_router %hd", (FMT__H, param));

    /* moved here from commissioning logic */
    ZB_SET_JOINED_STATUS(ZB_TRUE);

    (void)zb_buf_reuse(param);

    request = ZB_BUF_GET_PARAM(param, zb_nlme_start_router_request_t);
    request->beacon_order = ZB_TURN_OFF_ORDER;
    request->superframe_order = ZB_TURN_OFF_ORDER;
    request->battery_life_extension = 0;
    ZB_SCHEDULE_CALLBACK(zb_nlme_start_router_request, param);
}
#endif  /* ZB_ROUTER_ROLE && !ZB_COORDINATOR_ONLY */


void zdo_send_device_annce_ex(zb_uint8_t             param,
                              zb_zdo_device_annce_t *dev_annce
#ifdef ZB_USEALIAS
                              , zb_bool_t use_alias
#endif
                             )
{
    zb_apsde_data_req_t   *dreq = ZB_BUF_GET_PARAM(param, zb_apsde_data_req_t);
    zb_zdo_device_annce_t *da;

    ZB_ASSERT(dev_annce);

    da = zb_buf_initial_alloc(param, sizeof(*da));
    ZB_BZERO(dreq, sizeof(*dreq));

    ZB_MEMCPY(da, dev_annce, sizeof(zb_zdo_device_annce_t));

    /* Broadcast to all devices for which macRxOnWhenIdle = TRUE.
       MAC layer in ZE sends unicast to its parent.
    */
    dreq->dst_addr.addr_short = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
    dreq->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
    /* use default radius, max_depth * 2 */
    dreq->clusterid = ZDO_DEVICE_ANNCE_CLID;

#ifdef ZB_USEALIAS
    if (use_alias == ZB_TRUE)
    {
        dreq->use_alias = 1;
        dreq->alias_src_addr = da->nwk_addr;
    }
#endif

    ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, param);
}


/* Release addr lock for dst (if it is in addr map). It may be locked before on
 * apsde_data_req/zb_aps_send_command. */
static void zb_apsde_data_confirm_release_dst_addr_lock(zb_uint8_t param)
{
    zb_apsde_data_confirm_t *aps_data_conf = (zb_apsde_data_confirm_t *)ZB_BUF_GET_PARAM(param, zb_apsde_data_confirm_t);
    zb_address_ieee_ref_t addr_ref;
    zb_ret_t ret;

    if (aps_data_conf->need_unlock)
    {
        if (aps_data_conf->addr_mode == ZB_APS_ADDR_MODE_64_ENDP_PRESENT)
        {
            ret = zb_address_by_ieee(aps_data_conf->dst_addr.addr_long, /* create */ ZB_FALSE, /* lock */ ZB_FALSE, &addr_ref);
        }
        else
        {
            ret = zb_address_by_short(aps_data_conf->dst_addr.addr_short, /* create */ ZB_FALSE, /* lock */ ZB_FALSE, &addr_ref);
        }

        if (ret == RET_OK)
        {
            zb_address_unlock(addr_ref);
        }
    }
}


void zb_apsde_data_confirm(zb_uint8_t param)
{

    ZB_TH_PUSH_APSDE_DATA_CONFIRM(param);

    TRACE_MSG(TRACE_APS3, "> apsde_data_conf: param %hd status %d dev_annce %hd key_sw %hd",
              (FMT__H_D_H_H, param, zb_buf_get_status(param), ZG->zdo.handle.dev_annce_param, ZG->zdo.handle.key_sw));

    /* Do some interceptions, then continue handling of apsde.data.conf
     * in zb_apsde_data_acknowledged(). Quite a strange solution... */
    zb_apsde_data_confirm_release_dst_addr_lock(param);

#ifdef ZB_JOIN_CLIENT
    if (ZG->zdo.handle.dev_annce_param == param)
    {
        ZB_SCHEDULE_CALLBACK(zdo_commissioning_dev_annce_sent, param);
        ZG->zdo.handle.dev_annce_param = 0;
    }
    else
#endif  /* ZB_JOIN_CLIENT */
#ifdef ZB_ROUTER_ROLE
        if (ZG->zdo.handle.permit_joining_param == param)
        {
            ZG->zdo.handle.permit_joining_param = 0;
            ZB_SCHEDULE_CALLBACK(zb_zdo_mgmt_permit_joining_confirm_handle, param);
        }
        else
#endif
#if defined ZB_COORDINATOR_ROLE
            if (ZG->zdo.handle.key_sw == param)
            {
                TRACE_MSG(TRACE_SECUR3, "switch nwk key after this frame sent", (FMT__0));
                ZG->zdo.handle.key_sw = 0;
                secur_nwk_key_switch(ZB_NIB().active_key_seq_number + 1);
            }
            else
#endif
                if (ZG->zdo.handle.mgmt_leave_resp_buf == param)
                {
                    /* We are requested to leave the network via mgmt_leave_req */
                    /* Could be here after sending mgmt_leave_resp */
                    zb_nlme_leave_request_t *lr;

                    TRACE_MSG(TRACE_NWK1, "Leaving network after sending mgmt_leave_resp", (FMT__0));
                    ZG->zdo.handle.mgmt_leave_resp_buf = 0;

                    lr = ZB_BUF_GET_PARAM(param, zb_nlme_leave_request_t);
                    ZB_IEEE_ADDR_ZERO(lr->device_address);
                    lr->remove_children = ZG->nwk.leave_context.remove_children;
                    lr->rejoin = ZG->nwk.leave_context.rejoin_after_leave;
                    ZB_SCHEDULE_CALLBACK(zb_nlme_leave_request, param);
                }
                else
                {
                    TRACE_MSG(TRACE_ZDO1, "buffer status %d - call zb_apsde_data_acknowledged", (FMT__D, zb_buf_get_status(param)));
                    /* DA: buffer still contains zb_apsde_data_confirm_t) */
                    ZB_SCHEDULE_CALLBACK(zb_apsde_data_acknowledged, param);
                }

    TRACE_MSG(TRACE_APS3, "< apsde_data_conf", (FMT__0));
}


#ifdef ZB_ED_FUNC
void zb_zdo_forced_parent_link_failure(zb_uint8_t param)
{
    zb_nlme_status_indication_t *status_ind = ZB_BUF_GET_PARAM(param, zb_nlme_status_indication_t);

    TRACE_MSG(TRACE_ZDO1, ">> zb_zdo_forced_parent_link_failure param %hd", (FMT__H, param));

    ZDO_CTX().parent_link_failure = ZB_ZDO_PARENT_LINK_FAILURE_CNT;

    status_ind->status = ZB_NWK_COMMAND_STATUS_PARENT_LINK_FAILURE;
    status_ind->network_addr = ZG->nwk.handle.parent;

    ZB_SCHEDULE_CALLBACK(zb_nlme_status_indication, param);

    TRACE_MSG(TRACE_ZDO1, "<< zb_zdo_forced_parent_link_failure", (FMT__0));
}
#endif  /* ZB_ED_FUNC */


static void zb_send_nlme_status_indication_signal(zb_uint8_t param)
{
    zb_nlme_status_indication_t status;
    zb_zdo_signal_nlme_status_indication_params_t *status_params;

    TRACE_MSG(TRACE_NWK1, "zb_send_nlme_status_indication_signal param %hd", (FMT__H, param));
    ZB_MEMCPY(&status,
              ZB_BUF_GET_PARAM(param, zb_nlme_status_indication_t),
              sizeof(zb_nlme_status_indication_t));
    status_params =
        (zb_zdo_signal_nlme_status_indication_params_t *)zb_app_signal_pack(
            param,
            ZB_NLME_STATUS_INDICATION,
            RET_OK,
            (zb_uint8_t)sizeof(zb_zdo_signal_nlme_status_indication_params_t));
    ZB_MEMCPY(&status_params->nlme_status,
              &status,
              sizeof(zb_nlme_status_indication_t));
    ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete, param);
}


/* 3.2.2.30 NLME-NWK-STATUS.indication
 */
void zb_nlme_status_indication(zb_uint8_t param)
{
    zb_ret_t ret;

    TRACE_MSG(TRACE_ZDO1, ">>zb_nlme_status_indication %hd", (FMT__H, param));

    /* Get new buf and copy data there, since it needs to notify an application about NLME-NWK-STATUS and process the status in stack */
    ret = zb_buf_get_out_delayed_ext(zb_nlme_status_indication_process, param, 0);
    if (ret != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_ZDO1, "<<zb_nlme_status_indication", (FMT__0));
}


static void zb_nlme_status_indication_process(zb_uint8_t new_buf, zb_uint16_t initial_buf)
{
    zb_uint8_t u8_initial_buf = (zb_uint8_t)initial_buf;
    zb_nlme_status_indication_t *status = ZB_BUF_GET_PARAM(u8_initial_buf, zb_nlme_status_indication_t);

    TRACE_MSG(TRACE_ZDO1, ">>zb_nlme_status_indication_process  new_buf %hd, initial_buf %hd", (FMT__H, new_buf, initial_buf));

    TRACE_MSG(TRACE_ZDO1, "Got nwk status indication: status 0x%hx address 0x%x",
              (FMT__H_H, status->status, status->network_addr));

    if (status->status == ZB_NWK_COMMAND_STATUS_BAD_FRAME_COUNTER)
    {
        ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_NWKFC_FAILURE_ID);
    }

    zb_buf_copy(new_buf, u8_initial_buf);

    /* Generate NLME-NWK-STATUS.indication signal */
    zb_send_nlme_status_indication_signal(u8_initial_buf);

#ifdef ZB_ED_FUNC
    if (ZB_IS_DEVICE_ZED())
    {
        if (status->status == ZB_NWK_COMMAND_STATUS_PARENT_LINK_FAILURE)
        {
            ZDO_CTX().parent_link_failure++;
            TRACE_MSG(TRACE_ZDO1, "parent link failure %hd", (FMT__H, ZDO_CTX().parent_link_failure));
            if ( ZDO_CTX().parent_link_failure >= ZB_ZDO_PARENT_LINK_FAILURE_CNT )
            {
                zb_uint8_t *rejoin_reason = ZB_BUF_GET_PARAM(new_buf, zb_uint8_t);
                *rejoin_reason = ZB_REJOIN_REASON_PARENT_LOST;
                ZB_SCHEDULE_CALLBACK(zdo_commissioning_initiate_rejoin, new_buf);
                new_buf = 0;
                ZDO_CTX().parent_link_failure = 0;
            }
        }
    }
#endif  /* ZB_ED_FUNC */

    if (new_buf != 0U)
    {
        zb_buf_free(new_buf);
    }

    TRACE_MSG(TRACE_ZDO1, "<<zb_nlme_status_indication_process", (FMT__0));
}


/*
  After refactoring commissioning procedure calls nlme-reset.req.
  So we can be here during startup.
  Also, now nlme reset does not implicitely call pibcache load, so do it here manually.
 */
void zb_nlme_reset_confirm(zb_uint8_t param)
{

    TRACE_MSG(TRACE_ZDO1, ">>zb_nlme_reset_confirm %hd nlme_reset_cb %p",
              (FMT__H_P, param, ZG->zdo.nlme_reset_cb));

    ZB_ASSERT(ZG->zdo.nlme_reset_cb);
    ZB_SCHEDULE_CALLBACK(ZG->zdo.nlme_reset_cb, param);
    ZG->zdo.nlme_reset_cb = NULL;

    TRACE_MSG(TRACE_ZDO1, "<<zb_nlme_reset_confirm", (FMT__0));
}


/*
  We are here after pibcache synchronization with PIB at start after
  mac & nwk reset (first start).
 */
void zb_nwk_load_pib_confirm(zb_uint8_t param)
{

#if defined ZB_ENABLE_ZGP  && !defined ZB_ZGPD_ROLE
    /* sync skip_gpf flag */
    zb_buf_get_out_delayed(zb_zgp_sync_pib);
#endif

    /* TODO: kill continue_start_after_nwk_cb and use commissioning state instead. */
    ZB_ASSERT(ZDO_CTX().continue_start_after_nwk_cb);
    (*ZDO_CTX().continue_start_after_nwk_cb)(param);
}


void zdo_call_nlme_reset(zb_uint8_t param, zb_bool_t warm_start, zb_bool_t nib_reinit, zb_callback_t cb)
{
    //! [zb_nlme_reset_request]
    zb_nlme_reset_request_t *request = ZB_BUF_GET_PARAM(param, zb_nlme_reset_request_t);

    /* schedule reset request */
    request->warm_start = warm_start;
    request->no_nib_reinit = !nib_reinit;
    TRACE_MSG(TRACE_INFO1, "no_nib_reinit %hd", (FMT__H, request->no_nib_reinit));
    ZB_SCHEDULE_CALLBACK(zb_nlme_reset_request, param);
    //! [zb_nlme_reset_request]

    ZB_ASSERT(ZG->zdo.nlme_reset_cb == NULL);
    ZB_ASSERT(cb != NULL);
    ZG->zdo.nlme_reset_cb = cb;
}



/* **************************** >> Device Updated Signal functions >> **************************** */

/**
 * @brief Send @ZB_ZDO_SIGNAL_DEVICE_UPDATE signal with delay
 *
 * @param param - reference to the buffer
 * @param param2 - it's used to containing two variables:
 *                   1) addr_ref is stored in the HIGH byte;
 *                   2) status is stored in the LOW byte;
 */
static void zb_send_device_update_signal_delayed2(zb_uint8_t param, zb_uint16_t param2)
{
    zb_address_ieee_ref_t addr_ref = (zb_uint8_t)ZB_GET_HI_BYTE(param2);
    zb_uint8_t status = (zb_uint8_t)ZB_GET_LOW_BYTE(param2);
    zb_ieee_addr_t long_addr;
    zb_uint16_t short_addr = ZB_UNKNOWN_SHORT_ADDR;

    ZB_IEEE_ADDR_UNKNOWN(long_addr);
    zb_address_by_ref(long_addr, &short_addr, addr_ref);

    TRACE_MSG(TRACE_ZDO2, "<> zb_send_device_update_signal_delayed2, param %hd", (FMT__H, param));
    TRACE_MSG(TRACE_ZDO2, "long_addr " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(long_addr)));
    TRACE_MSG(TRACE_ZDO2, "short_addr 0x%x, status 0x%hx, addr_ref 0x%hx",
              (FMT__D_H_H, short_addr, status, addr_ref));

    zb_send_device_update_signal(param, long_addr, short_addr, status);
}

void zb_prepare_and_send_device_update_signal(zb_ieee_addr_t long_addr, zb_uint8_t status)
{
    zb_address_ieee_ref_t addr_ref;

    TRACE_MSG(TRACE_ZDO3, "<> zb_prepare_and_send_device_update_signal", (FMT__0));
    TRACE_MSG(TRACE_ZDO3, "long_addr " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(long_addr)));
    TRACE_MSG(TRACE_ZDO3, "status 0x%hx", (FMT__H, status));

    if (RET_OK == zb_address_by_ieee(long_addr,
                                     ZB_FALSE, /* don't create */
                                     ZB_FALSE, /* don't lock */
                                     &addr_ref))
    {
        zb_uint16_t user_param = 0;
        zb_ret_t ret;

        ZB_SET_LOW_BYTE(user_param, (zb_uint16_t)status);
        ZB_SET_HI_BYTE(user_param, addr_ref);

        /* TRICKY: use high byte of user_param for addr_ref
         *         and low byte of user_param as status
         */
        TRACE_MSG(TRACE_ZDO3, "addr_ref 0x%hx", (FMT__H, addr_ref));
        ret = zb_buf_get_out_delayed_ext(zb_send_device_update_signal_delayed2, user_param, 0);
        if (ret != RET_OK)
        {
            TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed_ext [%d]", (FMT__D, ret));
        }
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "DEVICE_UPDATED signal was not sent! Could not found ref by long addr " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(long_addr)));
    }
}
/* **************************** << Device Update Signal functions << **************************** */


/* **************************** >> Device Authorized Signal functions >> **************************** */

/**
 * @brief Send @ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED signal with delay
 *
 * @param param - reference to the buffer
 * @param param2 - it's used to containing two variables:
 *                   1) addr_ref is stored in the HIGH byte;
 *                   2) status is stored in the LOW byte;
 */
static void zb_send_device_authorized_signal_delayed2(zb_uint8_t param, zb_uint16_t param2)
{
    zb_address_ieee_ref_t addr_ref = (zb_address_ieee_ref_t)ZB_GET_HI_BYTE(param2);
    zb_uint8_t authorization_type = (zb_uint8_t)ZB_GET_AUTHORIZATION_TYPE(ZB_GET_LOW_BYTE(param2));
    zb_uint8_t authorization_status = (zb_uint8_t)ZB_GET_AUTHORIZATION_STATUS(ZB_GET_LOW_BYTE(param2));
    zb_ieee_addr_t long_addr;
    zb_uint16_t short_addr = ZB_UNKNOWN_SHORT_ADDR;

    ZB_IEEE_ADDR_UNKNOWN(long_addr);
    zb_address_by_ref(long_addr, &short_addr, addr_ref);

    TRACE_MSG(TRACE_ZDO2, "<> zb_send_device_authorized_signal_delayed2, param %hd", (FMT__H, param));
    TRACE_MSG(TRACE_ZDO2, "long_addr " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(long_addr)));
    TRACE_MSG(TRACE_ZDO2, "short_addr 0x%x, auth_type 0x%hx, auth_status 0x%hx, addr_ref 0x%hx",
              (FMT__D_H_H_H, short_addr, authorization_type, authorization_status, addr_ref));

    zb_send_device_authorized_signal(param, long_addr, short_addr, authorization_type, authorization_status);
}

void zb_prepare_and_send_device_authorized_signal(zb_ieee_addr_t long_addr,
        zb_uint8_t authorization_type,
        zb_uint8_t authorization_status)
{
    zb_address_ieee_ref_t addr_ref;

    TRACE_MSG(TRACE_ZDO3, "<> zb_prepare_and_send_device_authorized_signal", (FMT__0));
    TRACE_MSG(TRACE_ZDO3, "long_addr " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(long_addr)));
    TRACE_MSG(TRACE_ZDO3, "auth_type 0x%hx, auth_status 0x%hx",
              (FMT__H_H, authorization_type, authorization_status));

    if (RET_OK == zb_address_by_ieee(long_addr,
                                     ZB_FALSE, /* don't create */
                                     ZB_FALSE, /* don't lock */
                                     &addr_ref))
    {
        zb_uint16_t user_param = 0;
        zb_ret_t ret;

        ZB_SET_LOW_BYTE(user_param, ZB_PACK_AUTHORIZATION_PARAMS((zb_uint16_t)authorization_status,
                        (zb_uint16_t)authorization_type));
        ZB_SET_HI_BYTE(user_param, addr_ref);

        /* TRICKY: use high byte of user_param for addr_ref;
         *         use 6 high bits of low byte of user_param for authorization_status;
         *         use 2 low bits of low byte of user_param for authorization_type;
         */
        TRACE_MSG(TRACE_ZDO3, "addr_ref 0x%hx", (FMT__H, addr_ref));
        ret = zb_buf_get_out_delayed_ext(zb_send_device_authorized_signal_delayed2, user_param, 0);
        if (ret != RET_OK)
        {
            TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
        }
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "DEVICE_AUTHORIZED signal was not sent! Could not found ref by long addr", (FMT__0));
    }
}

void zb_legacy_device_auth_signal_alarm(zb_uint8_t param)
{
    zb_ret_t ret;
    zb_uint16_t user_param = 0;
    zb_ieee_addr_t ieeeaddr;

    TRACE_MSG(TRACE_ZDO3, ">> zb_legacy_device_auth_signal_alarm, addr_ref 0x%hx", (FMT__H, param));

    zb_address_ieee_by_ref(ieeeaddr, param);
    if (!ZB_IS_64BIT_ADDR_ZERO(ieeeaddr)
            /*cstat !MISRAC2012-Rule-13.5 */
            /* After some investigation, the following violation of Rule 13.5 seems to be
             * a false positive. There are no side effect to 'zb_aps_keypair_get_index_by_addr()'. This
             * violation seems to be caused by the fact that 'zb_aps_keypair_get_index_by_addr()' is an
             * external function, which cannot be analyzed by C-STAT. */
            && zb_aps_keypair_get_index_by_addr(ieeeaddr, ZB_SECUR_VERIFIED_KEY) != (zb_uint16_t) -1)
    {
        ZB_SET_LOW_BYTE(user_param, ZB_PACK_AUTHORIZATION_PARAMS(ZB_ZDO_LEGACY_DEVICE_AUTHORIZATION_SUCCESS,
                        ZB_ZDO_AUTHORIZATION_TYPE_R21_TCLK));
    }
    else
    {
        ZB_SET_LOW_BYTE(user_param, ZB_PACK_AUTHORIZATION_PARAMS(ZB_ZDO_LEGACY_DEVICE_AUTHORIZATION_SUCCESS,
                        ZB_ZDO_AUTHORIZATION_TYPE_LEGACY));
    }
    ZB_SET_HI_BYTE(user_param, param);
    ret = zb_buf_get_out_delayed_ext(zb_send_device_authorized_signal_delayed2, user_param, 0);
    if (ret != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed_ext [%d]", (FMT__D, ret));
    }

    TRACE_MSG(TRACE_ZDO3, "<< zb_legacy_device_auth_signal_alarm, addr_ref 0x%hx", (FMT__H, param));
}

void zb_legacy_device_auth_signal_cancel(zb_ieee_addr_t long_addr)
{
    zb_address_ieee_ref_t addr_ref;
    if (RET_OK == zb_address_by_ieee(long_addr,
                                     ZB_FALSE, /* don't create */
                                     ZB_FALSE, /* don't lock */
                                     &addr_ref))
    {
        ZB_SCHEDULE_ALARM_CANCEL(zb_legacy_device_auth_signal_alarm, addr_ref);
    }
}

void zb_zdo_register_addr_resp_cb(zb_callback_t addr_resp_cb)
{
    ZDO_CTX().app_addr_resp_cb = addr_resp_cb;
}

void zb_zdo_register_assert_indication_cb(zb_assert_indication_cb_t assert_cb)
{
    ZDO_CTX().assert_indication_cb = assert_cb;
}


#ifdef ZB_ROUTER_ROLE


void zdo_send_parent_annce_at_formation(zb_uint8_t param)
{
    ZB_ASSERT(param == 0U);
    if (ZB_NIB().ed_child_num != 0U)
    {
        ZB_SCHEDULE_ALARM(zdo_send_parent_annce, 0, ZB_PARENT_ANNCE_JITTER());
    }
}


void zdo_send_parent_annce(zb_uint8_t param)
{
    zb_zdo_parent_annce_t *pa;
    zb_apsde_data_req_t *dreq;
    zb_ieee_addr_t *ieee_addr;
    zb_uint8_t i;

    TRACE_MSG(TRACE_ZDO1, ">> zdo_send_parent_annce %hd", (FMT__H, param));
    /*
      2.4.3.1.12 Parent_annce
      - Router shall examine its neighbor table for all devices.
      - The router shall construct, but not yet send, an empty Parent_annce message
      and set NumberOfChildren to 0. For each end device in the neighbor table, it
      shall do the following.
      1. If the Neighbor Table entry indicates a Device Type not equal to End
      Device (0x02), do not process this entry. Continue to the next one.
      2. Incorporate end device information into the Parent_annce message by doing
      the following:
      a. Append a ChildInfo structure to the message.
      b. Increment NumberOfChildren by 1.
      3. Note: The value of Keepalive Received for the Neighbor Table Entry is not
      considered.
    */

    if (ZB_NIB().ed_child_num == 0U)
    {
        if (param != 0U)
        {
            zb_buf_free(param);
        }
    }
    else if (param == 0U)
    {
        zb_ret_t ret;

        TRACE_MSG(TRACE_ZDO1, "Get out buffer for zdo_send_parent_annce", (FMT__0));
        ret = zb_buf_get_out_delayed(zdo_send_parent_annce);
        if (ret != RET_OK)
        {
            TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
        }
    }
    else
    {
        pa = zb_buf_initial_alloc(param, sizeof(*pa));
        ZDO_TSN_INC();
        pa->tsn = ZDO_CTX().tsn;
        pa->num_of_children = 0;
        TRACE_MSG(TRACE_ZDO1, "Start NBT scan from %d", (FMT__D, ZDO_CTX().parent_annce_position));

        for (i = ZDO_CTX().parent_annce_position; i < ZB_NEIGHBOR_TABLE_SIZE  ; i++) /*Scan NBT*/
        {
            if (ZB_U2B(ZG->nwk.neighbor.neighbor[i].used)
                    && !ZB_U2B(ZG->nwk.neighbor.neighbor[i].ext_neighbor)
                    && ZG->nwk.neighbor.neighbor[i].device_type == ZB_NWK_DEVICE_TYPE_ED)
            {
                TRACE_MSG(TRACE_ZDO1, "Found child ED, NIB position %d", (FMT__D, i));
                /* 1 byte for tsn + zdo payload size */
                if (sizeof(zb_ieee_addr_t) + zb_buf_len(param) <= ZB_APS_MAX_MAX_BROADCAST_PAYLOAD_SIZE)
                {
                    ieee_addr = zb_buf_alloc_right(param, sizeof(zb_ieee_addr_t));
                    pa = (zb_zdo_parent_annce_t *)zb_buf_begin(param);
                    zb_address_ieee_by_ref(*ieee_addr, ZG->nwk.neighbor.neighbor[i].u.base.addr_ref);
                    pa->num_of_children++;
                    TRACE_MSG(TRACE_ZDO3, "new buf size %d # child %hd", (FMT__D_H, zb_buf_len(param), pa->num_of_children));
                }
                else
                {
                    TRACE_MSG(TRACE_ZDO1, "Buffer is full #ch %hd, get one more for zdo_send_parent_annce", (FMT__H, pa->num_of_children));
                    /* Buffer is full, send it, save position ni the NBT and re-schedule parent_annce */
                    ZDO_CTX().parent_annce_position = i;
                    ZB_SCHEDULE_ALARM(zdo_send_parent_annce, 0, ZB_PARENT_ANNCE_JITTER());
                    break;
                }
            }
        }

        /*If ED entries found in the NBT then send Parent_annce*/
        if (pa->num_of_children != 0U)
        {
            dreq = ZB_BUF_GET_PARAM(param, zb_apsde_data_req_t);
            ZB_BZERO(dreq, sizeof(*dreq));
            dreq->dst_addr.addr_short = ZB_NWK_BROADCAST_ROUTER_COORDINATOR;
            dreq->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
            /* use default radius, max_depth * 2 */
            dreq->clusterid = ZDO_PARENT_ANNCE_CLID;
            TRACE_MSG(TRACE_ZDO1, "Send parent_annce", (FMT__0));
            ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, param);
        }
        else /*If parent_annce is empty*/
        {
            TRACE_MSG(TRACE_ZDO1, "No entries found, free buffer %hd", (FMT__H, param));
            zb_buf_free(param);
        }
    }
    TRACE_MSG(TRACE_ZDO1, "<< zdo_send_parent_annce", (FMT__0));
}


void zb_zdo_register_update_device_indication_cb(zb_apsme_update_device_ind_cb cb)
{
    ZDO_CTX().update_device_ind_cb = cb;
}

#endif /* ZB_ROUTER_ROLE */

#if defined ZB_MAC_DUTY_CYCLE_MONITORING
void zb_zdo_register_duty_cycle_mode_indication_cb(zb_zdo_duty_cycle_mode_ind_cb_t cb)
{
    ZDO_CTX().duty_cycle_mode_ind_cb = cb;
}
#endif /* ZB_MAC_DUTY_CYCLE_MONITORING */

#ifndef ZB_LITE_NO_CONFIGURABLE_POWER_DELTA
zb_ret_t zb_zdo_set_lpd_cmd_mode(zb_uint8_t mode)
{
    zb_ret_t ret = RET_ERROR;

    if (ZB_IS_DEVICE_ZED())
    {
        ZB_NIB_NWK_LPD_CMD_MODE() = mode;

        ret = RET_OK;
    }

    return ret;
}

void zb_zdo_set_lpd_cmd_timeout(zb_uint8_t timeout)
{
    zb_uint_t i;

    for (i = 0; i < ZB_NWK_MAC_IFACE_TBL_SIZE; i++)
    {
        ZB_NIB_SET_POWER_DELTA_PERIOD(i, timeout);
    }
}
#endif  /* ZB_LITE_NO_CONFIGURABLE_POWER_DELTA */


void zb_zdo_set_aps_unsecure_join(zb_bool_t insecure_join)
{
    ZB_AIB().aps_insecure_join = ZB_B2U(insecure_join);
}

#endif  /* NCP_MODE_HOST */

/*! @} */
