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
/*  PURPOSE: SE specific commissioning
*/

#define ZB_TRACE_FILE_ID 66
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_nwk_nib.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_nvram.h"
#include "zb_ncp.h"
#include "zdo_wwah_parent_classification.h"

/* Enable this define to debug unknown signals in different states etc. */
/* #define SE_COMMISSIONING_DEBUG */

#ifdef ZB_SE_ENABLE_KEC_CLUSTER
static void kec_done_cb(zb_uint8_t param);
static void call_client_ke_done_cb(zb_uint8_t param, zb_ret_t status);
static void client_ke_done(zb_uint8_t param, zb_ret_t status);
#ifdef ZB_JOIN_CLIENT
static void key_est_match_desc_cb(zb_uint8_t param);
#endif
#endif
#ifdef ZB_SE_COMMISSIONING

#include "se/zb_se_sas.h"

static void se_auto_join_iteration(zb_uint8_t param);
void se_send_key_est_match_desc(zb_uint8_t param);
void se_send_key_est_match_desc_addr(zb_uint8_t param, zb_uint16_t addr_of_interest);
static void kec_done_cb(zb_uint8_t param);
#ifdef ZB_JOIN_CLIENT
static zb_uint_t se_next_auto_join_timeout();
static void se_precommissioned_rejoin_machine(zb_uint8_t param);
static void se_joining_auth_common_machine(zb_uint8_t paraDPm);
static void se_auto_join_machine(zb_uint8_t param);
static void se_rejoin_recovery_machine(zb_uint8_t param);

#if defined(ZB_SE_ENABLE_SERVICE_DISCOVERY_PROCESSING)
static void se_service_discovery_machine(zb_uint8_t param);
#endif

static void se_device_steady_machine(zb_uint8_t param);
static void se_savepoint_before_rr();
static void se_retry_rejoin_recovery(zb_uint8_t param);
static void se_restore_saved_rr(zb_uint8_t param);
#endif
#ifdef ZB_FILTER_OUT_CLUSTERS
static zb_int_t zdo_check_subghz_cluster_filtered(zb_uint16_t clid);
#endif

#ifdef ZB_COORDINATOR_ROLE
static void se_formation_machine(zb_uint8_t param);
static void se_tc_steady_machine(zb_uint8_t param);
static void se_link_key_refresh_alarm(zb_uint8_t param);
static void kec_srv_done_cb(zb_uint8_t ref, zb_uint16_t status);
static void se_broadcast_permit_join(zb_uint8_t param);
static void zdo_send_joined_cbke_ok_sig(zb_uint8_t param, zb_uint16_t aref);
static void zdo_send_joined_non_cbke_ok_sig(zb_uint8_t param, zb_uint16_t aref);
static void send_tc_key_est_match_desc(zb_uint8_t param, zb_uint16_t addr_of_interest);
static void tc_key_est_match_desc_cb(zb_uint8_t param);
void se_tc_update_link_key(zb_uint16_t addr_of_interest);
#endif

#ifdef ZB_SE_BDB_MIXED
static void se_switch_to_bdb_commissioning(zb_bdb_commissioning_mode_mask_t step);
#endif /* ZB_SE_BDB_MIXED */

/**
   Initiate SE commissioning at startup time.

   Note: commissioning by user's action is to be initiated by another call.
*/
void se_initiate_commissioning(zb_uint8_t param)
{
  zb_int_t joined;

  TRACE_MSG(TRACE_ZDO1, "se_initiate_commissioning %hd", (FMT__H, param));

#ifdef ZB_SE_ENABLE_KEC_CLIENT
  zse_kec_set_client_cb(client_ke_done);
#endif /* ZB_SE_ENABLE_KEC_CLIENT */

  /* In SE always can accept APS link keys */
  ZB_TCPOL().accept_new_unsolicited_application_link_key = ZB_TRUE;
  ZB_TCPOL().update_trust_center_link_keys_required = ZB_FALSE;

  ZSE_CTXC().commissioning.startup_control = ZSE_STARTUP_UNCOMMISSIONED;
  /* use logic similar to BDB to define that we are commissioned already. */
  joined = (!ZB_EXTPANID_IS_ZERO(ZB_NIB_EXT_PAN_ID()) &&
            zb_zdo_authenticated()
#ifndef ZB_COORDINATOR_ONLY
            && !ZB_IEEE_ADDR_IS_ZERO(ZB_AIB().trust_center_address)
#endif
    );
  TRACE_MSG(TRACE_ZDO3, "joined %d device type %d", (FMT__D_D, joined, zb_get_device_type()));
  if (joined)
  {
    if (ZB_IS_DEVICE_ZC())
    {
#ifndef ZB_COORDINATOR_ONLY
      joined &= ZB_IEEE_ADDR_CMP(ZB_AIB().trust_center_address, ZB_PIBCACHE_EXTENDED_ADDRESS());
#endif
      TRACE_MSG(TRACE_ZDO3, "joined %d", (FMT__D, joined));
    }
#ifdef ZB_JOIN_CLIENT
    else
    {
      /* Joined device must have TCLK got over CBKE */
      zb_aps_device_key_pair_set_t *aps_key = zb_secur_get_link_key_by_address(ZB_AIB().trust_center_address, ZB_SECUR_VERIFIED_KEY);
      joined &= (aps_key && aps_key->key_source == ZB_SECUR_KEY_SRC_CBKE);
#ifdef ZB_SE_BDB_MIXED
      if (aps_key->key_source != ZB_SECUR_KEY_SRC_CBKE &&
          ZSE_CTXC().commissioning.allow_bdb_in_se_mode)
      {
        /* TODO: Add BDB key src? */
        TRACE_MSG(TRACE_ERROR, "Have verified key, but not from CBKE. Try to start in BDB", (FMT__0));
        se_switch_to_bdb_commissioning(ZB_BDB_INITIALIZATION);
        zdo_commissioning_start(param);
        return;
      }
#endif
      TRACE_MSG(TRACE_ZDO3, "joined %d", (FMT__D, joined));
    }
#endif
  }
  if (joined)
  {
    TRACE_MSG(TRACE_ZDO3, "commissioned after reboot", (FMT__D, joined));
    ZSE_CTXC().commissioning.startup_control = ZSE_STARTUP_COMMISSIONED;
  }

#ifdef ZB_FILTER_OUT_CLUSTERS
  zdo_add_cluster_filter(zdo_check_subghz_cluster_filtered);
#endif

  if (ZSE_CTXC().commissioning.startup_control == ZSE_STARTUP_COMMISSIONED)
  {
    ZB_ASSERT(!ZB_EXTPANID_IS_ZERO(ZB_NIB_EXT_PAN_ID()));

    if (ZB_IS_DEVICE_ZC())
    {
      TRACE_MSG(TRACE_ZDO1, "ZC started after reboot %hd", (FMT__H, param));
#ifdef DEBUG_RR
//      ZB_TCPOL().allow_tc_rejoins = 0;
#endif
      ZSE_CTXC().commissioning.state = SE_STATE_TC_STEADY;

#ifdef ZB_COORDINATOR_ROLE
      se_minimal_tc_init();
#endif /* ZB_COORDINATOR_ROLE */

#ifdef ZB_FORMATION
      ZB_SCHEDULE_CALLBACK(zb_nwk_cont_without_formation, param);
#endif /* ZB_FORMATION */
    }
    else
    {
      /* I found no description how SE device works after reboot. In classic commissioning ZR/ZED must rejoin.
         In BDB only ZED must rejoin.
         TODO: recheck it!
      */
      /* Rejoin after reboot */

      /*
        This is from the classical commissioning.

        Try to rejoin the current network:
        use apsChannelMask as channel mask and do unsecure rejoin
        if we are not authenticated.

        TODO: check: is it valid for SE??

        5.3.1 Startup Parameters

        Startup Control
        2 (two) if un-commissioned, so it will join network by association when a join command is indicated.
        0 (zero) if commissioned. Indicates that the device should consider itself a part of the network indicated by the ExtendedPANId attribute. In this
        case it will not perform any explicit join or rejoin operation.

        It looks like after reboot we must do nothing. That is strange: even ZED??

        Let's work as in BDB: ZED rejoin, ZR does not. Hmm?
      */

      /* For now let's rejoin at start. */
      /* Now go to rr state to rejoin. */
      /* EA: Looks like this procedure described in 5.4.2.2.3.5 Trust Center Swap-out Process section */
      ZSE_CTXC().commissioning.just_booted = 1;
      /* debud r&r */
#ifdef DEBUG_RR
      {
        /* Set wrong nwk key to fail secure rejoin */
        static zb_uint8_t g_key[16] = { 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0, 0, 0, 0, 0, 0, 0, 1};
        zb_secur_setup_nwk_key(g_key, 0);

        /* Uncomment to test for TC rejoins only */
        /* ZG->aps.authenticated = ZB_FALSE; */
      }
#endif

#ifndef DEBUG_RR
      if (ZB_IS_DEVICE_ZED())
#else
      if (1)
#endif
      {
        TRACE_MSG(TRACE_ZDO1, "se_initiate_commissioning - rejoin after reboot (utilize r&r state)", (FMT__0));
        ZSE_CTXC().commissioning.state = SE_STATE_REJOIN_RECOVERY;
        se_commissioning_signal(SE_COMM_SIGNAL_START_REJOIN_RECOVERY, param);
      }
      else
      {
        TRACE_MSG(TRACE_ZDO2, "Initiate start router for ZR", (FMT__0));
        ZSE_CTXC().commissioning.state = SE_STATE_DEVICE_STEADY;
        ZB_SCHEDULE_CALLBACK(zb_zdo_start_router, param);
      }
    }
  }
  else
  {
    zb_ext_pan_id_t use_ext_pan_id;

    zb_get_use_extended_pan_id(use_ext_pan_id);

    /* Not commissioned */
    TRACE_MSG(TRACE_ZDO1, "First time commissioning %hd", (FMT__H, param));
#ifdef ZB_COORDINATOR_ROLE
    if (ZB_IS_DEVICE_ZC())
    {
      TRACE_MSG(TRACE_ZDO1, "formation", (FMT__H, param));
      ZB_ASSERT(FORMATION_SELECTOR().start_formation != NULL);
      ZB_SCHEDULE_CALLBACK(FORMATION_SELECTOR().start_formation, param);
      return;
    }
#endif
#ifndef ZB_COORDINATOR_ONLY
    ZB_ASSERT(zb_get_device_type() != ZB_NWK_DEVICE_TYPE_NONE);

/* Rejoin to the net defined by aps_use_extended_pan_id */
/*
  1 There are two methods for joining such a device onto an existing network:
  a The device is commissioned using a tool with the necessary network
  and security start up parameters to allow it to rejoin the network as
  a new device. The device can rejoin any device in the network
  since it has all the network information.
*/
    if (!ZB_EXTPANID_IS_ZERO(use_ext_pan_id))
    {
      TRACE_MSG(TRACE_ZDO1, "se_initiate_commissioning - rejoin to aps_use_extended_pan_id " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(use_ext_pan_id)));
      /* Can we have NWK key precommissioned? */
      ZSE_CTXC().commissioning.state = SE_STATE_PRECOMMISSIONED_REJOIN;
      se_commissioning_signal(SE_COMM_SIGNAL_START_REJOIN, param);
    }
    else if (ZSE_CTXC().commissioning.auto_join)
    {
      /* ZSE_CTXC().commissioning.auto_join must be loaded from nvram or set by default. */
      TRACE_MSG(TRACE_ZDO1, "se_initiate_commissioning - start auto-join", (FMT__0));
      if (zb_se_auto_join_start(param) != RET_OK)
      {
        ZB_ASSERT(0);
      }
    }
    else
    {
      /* We can be here if no rejoin configured and we are not in the auto join state */
      TRACE_MSG(TRACE_ZDO1, "se_initiate_commissioning - signal ok to user", (FMT__0));
      ZSE_CTXC().commissioning.state = SE_STATE_FAILED;
      zb_app_signal_pack(param, ZB_SE_SIGNAL_SKIP_JOIN, RET_OK, 0);
      ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete_int, param);
    }
#endif  /* #ifndef ZB_COORDINATOR_ONLY */
  }
}

#ifdef ZB_FILTER_OUT_CLUSTERS
static zb_int_t zdo_check_subghz_cluster_filtered(zb_uint16_t clid)
{
  if (zb_zdo_joined()
      && !(ZB_IS_DEVICE_ZC() && ZB_SUBGHZ_SWITCH_MODE())
      && clid == ZB_ZCL_CLUSTER_ID_SUB_GHZ
      && ZB_PIBCACHE_CURRENT_PAGE() == 0)
  {
    /* Select device of 2.4 only, not at sub-ghz. Filter out subghz cluster. */
    return 1;
  }
  else
  {
    return 0;
  }
}
#endif

zb_ret_t zb_se_auto_join_start(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO1, "se_auto_join_start param %hd", (FMT__H, param));
  if (!zb_zdo_joined() &&
      ZSE_CTXC().commissioning.state != SE_STATE_PRECOMMISSIONED_REJOIN &&
      ZSE_CTXC().commissioning.state != SE_STATE_REJOIN_RECOVERY &&
      ZSE_CTXC().commissioning.state != SE_STATE_AUTO_JOIN)
  {
    ZSE_CTXC().commissioning.state = SE_STATE_AUTO_JOIN;
    se_commissioning_signal(SE_COMM_SIGNAL_START_AUTO_JOIN, param);
    return RET_OK;
  }
  else
  {
    TRACE_MSG(TRACE_ZDO1, "skip auto_join: ZB_JOINED %hd commissioning_state %hd",
              (FMT__H_H, zb_zdo_joined(), ZSE_CTXC().commissioning.state));
    return RET_ERROR;
  }
}

zb_ret_t zb_se_auto_join_stop()
{
  TRACE_MSG(TRACE_ZDO1, "se_auto_join_stop", (FMT__0));
  if (ZSE_CTXC().commissioning.state == SE_STATE_AUTO_JOIN)
  {
    zb_uint8_t cancelled_param = 0;

    ZSE_CTXC().commissioning.state = SE_STATE_FAILED;
    ZB_SCHEDULE_ALARM_CANCEL_AND_GET_BUF(se_auto_join_iteration, ZB_ALARM_ANY_PARAM, &cancelled_param);
    if (cancelled_param)
    {
      zb_buf_free(cancelled_param);
    }

    return RET_OK;
  }
  else
  {
    TRACE_MSG(TRACE_ZDO1, "can not stop auto_join: commissioning_state %hd",
              (FMT__H, ZSE_CTXC().commissioning.state));
    return RET_ERROR;
  }
}

/**
   After {re}join failure in auto-join state call that function to define timeout to attempt.
*/
static zb_uint_t se_next_auto_join_timeout()
{
  zb_time_t t = ZB_TIMER_GET();
  zb_time_t time_delta = ZB_TIME_SUBTRACT(t, ZSE_CTXC().commissioning.auto_join_start_time);

  /*
    1 When auto-joining state is initiated, a device shall periodically
    scan all startup set channels for networks that are allowing
    joining. (See sub-clause 5.3.1 for startup set channel description). A
    recommended periodic schedule would be:

    a Immediately when auto-joining state is initiated.
    b If auto-joining state fails, retry once a minute for the next 15 minutes, jittered by +/- 15 seconds.
    c If those joining states fail, then retry to join once an hour jittered +/- 30 minutes.

  */
  if (time_delta <= ZB_MILLISECONDS_TO_BEACON_INTERVAL(15 * 60 * 1000))
  {
    t = ZB_MILLISECONDS_TO_BEACON_INTERVAL((1 * 60 - 15 + ZB_RANDOM_JTR(15 * 2)) * 1000);
  }
  else
  {
    t = ZB_MILLISECONDS_TO_BEACON_INTERVAL((60 - 30 + ZB_RANDOM_JTR(30 * 2)) * 60 * 1000);
  }
#ifdef ZSE_TEST_HACKS
  t = ZB_MILLISECONDS_TO_BEACON_INTERVAL(1000);
#endif
  TRACE_MSG(TRACE_ZDO3, "se_next_auto_join_timeout ret %d %d ms", (FMT__D_D, t, ZB_TIME_BEACON_INTERVAL_TO_MSEC(t)));
  return t;
}


static void se_auto_join_iteration(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO2, "se_auto_join_iteration %hd", (FMT__H, param));
/*
  To find prospective networks to join, the joining node shall send
  Beacon Request packets on each channel, dwelling on each channel
  as specified by the Zigbee PRO specification beacon response
  window.
*/
  COMM_CTX().discovery_ctx.disc_count = COMM_CTX().discovery_ctx.nwk_scan_attempts;
  /*
    Code in zdo_app.c calls nwk discovery 3 times, then join by call zdo_commissioning_join_via_scanlist.

    Possible failures:
    - zdo_commissioning_nwk_discovery_failed: nothing found. Do next iteration later.
    - zdo_commissioning_join_failed: can't join. Retry join via scanlist. If no more devices, do next iteratiuon later.
    - zdo_commissioning_authentication_failed: can't get NWK key. Same action as above.
  */
  ZSE_CTXC().commissioning.cbke_retries = 0;
  /* ZG->aps.authenticated = ZB_FALSE; */
  ZB_SCHEDULE_CALLBACK(zdo_next_nwk_discovery_req, param);
}


/* Implement preferred channels feature: first 2.4 GHz ZB_SE_PREFERRED_24_CHANNELS, then all channels. */
void se_pref_channels_create_mask(zb_bool_t first_time, zb_channel_list_t list)
{
  zb_uint32_t mask = 0;
  if (first_time)
  {
    mask = zb_channel_page_list_get_2_4GHz_mask(ZB_AIB().aps_channel_mask_list) & ZB_SE_PREFERRED_24_CHANNELS;
  }
  if (mask)
  {
    zb_channel_list_init(list);
    zb_channel_page_list_set_mask(list, 0, mask);
  }
  else
  {
    zb_channel_page_list_copy(list, ZB_AIB().aps_channel_mask_list);
  }
}


static void se_commissioning_scan_channels_mask(zb_channel_list_t list)
{
  se_pref_channels_create_mask((zb_bool_t)(COMM_CTX().discovery_ctx.disc_count == COMM_CTX().discovery_ctx.nwk_scan_attempts), list);
}


#ifdef ZB_COORDINATOR_ROLE
/**
   Open network for joining.

   Can be called at TC only.
*/
void zb_se_permit_joining(zb_time_t timeout_s)
{
  TRACE_MSG(TRACE_ZDO2, "zse_permit_joining %d s", (FMT__D, timeout_s));
  /* From 5.4.1 Joining with Preinstalled Trust Center Link Keys */

  /*
    Permit joining is turned on in the network. The Trust Center enables
    joining by calling the NLME-PERMIT-JOINING.request
    primitive. Joining must be managed for an appropriate amount of
    time but SHALL NOT be broadcast with a time of greater than 254
    seconds should not repeatedly broadcast without hearing device
    announcement or network administrator action. The appropriate
    amount of time will be dictated by the overall performance of the
    system and business processes driving the registration and device
    authorization activities. See sub-clause 5.4.1.2, Best
    Practice for Coordinator Permit Joining Broadcasts.
  */

  /*
    Be aware Joining has an internal time out within the Zigbee stack,
    therefore joining may need 1665 to be enabled multiple times during
    the overall Registration and device authorization process.
  */

  if (!ZB_IS_DEVICE_ZC())
  {
    /* oops - only ZC can open network for join */
    TRACE_MSG(TRACE_ERROR, "zse_permit_joining: device role %d not ZC!", (FMT__D, zb_get_device_type()));
  }
  else
  {
    ZSE_CTXC().commissioning.permit_join_rest = timeout_s;
    ZB_SCHEDULE_CALLBACK(se_broadcast_permit_join, 0);
  }
}


void se_minimal_tc_init(void)
{
  /* SE allows (and recommends) Trust Center rejoins. */
  ZB_TCPOL().allow_tc_rejoins = ZB_TCPOL().allow_joins = ZB_TRUE;
  /* See 5.5.1 Forming the Network (Start-up Sequence) */
  zse_kec_set_server_cb(kec_srv_done_cb);
}


static void se_permit_join_for_me(zb_uint8_t param)
{
  zb_zdo_mgmt_permit_joining_req_param_t *req = ZB_BUF_GET_PARAM(param,
                                                                 zb_zdo_mgmt_permit_joining_req_param_t);

  ZB_BZERO(req, sizeof(zb_zdo_mgmt_permit_joining_req_param_t));
  req->permit_duration = ZDO_CTX().permit_join_time;
  req->dest_addr = ZB_PIBCACHE_NETWORK_ADDRESS();

  zb_zdo_mgmt_permit_joining_req(param, NULL);
}


static void se_broadcast_permit_join(zb_uint8_t param)
{
  if (!param)
  {
    zb_buf_get_out_delayed(se_broadcast_permit_join);
  }
  else
  {
    /*
      It will be left to the coordinator / administrators of the network to
      determine when a network 1702 should be allowing joining. However when
      the network is allowing joining: 1703 1704

      1 At the start of the joining period the coordinator will allow
      joining and broadcast a permit join 1705 message for the lesser of the
      permit join period or 254 seconds. 1706

      2 Every 240 seconds or whenever a device announce is received the
      coordinator will broadcast a 1707 permit join message for the lesser
      of the remaining permit join period or 254 seconds. 1708
      Administrators of a network shall try to keep the amount of time
      devices on their networks 1709 allow joining to a minimum.
    */
    zb_uint8_t duration, alarm;
    zb_zdo_mgmt_permit_joining_req_param_t *req_param = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_permit_joining_req_param_t);

#define MAX_PERMIT_JOIN_TIME 240
#define PERMIT_JOIN_QUANT 254
    if (ZSE_CTXC().commissioning.permit_join_rest > MAX_PERMIT_JOIN_TIME)
    {
      ZSE_CTXC().commissioning.permit_join_rest -= MAX_PERMIT_JOIN_TIME;
      duration = PERMIT_JOIN_QUANT;
      alarm = MAX_PERMIT_JOIN_TIME;
    }
    else
    {
      alarm = duration = ZSE_CTXC().commissioning.permit_join_rest;
      ZSE_CTXC().commissioning.permit_join_rest = 0;
    }

    /*
       store duration in global context to set the same value to our mac after
       sending permit_join to other coordinators/routers in network
    */
    ZDO_CTX().permit_join_time = duration;

    ZB_BZERO(req_param, sizeof(zb_zdo_mgmt_permit_joining_req_param_t));
    req_param->dest_addr = ZB_NWK_BROADCAST_ROUTER_COORDINATOR;
    req_param->permit_duration = duration;
    req_param->tc_significance = ZB_TRUE; /* must be 1 in >= r21 */
    TRACE_MSG(TRACE_ZDO3, "permit_joining for %hd sec rest %d",
              (FMT__H_D, duration, ZSE_CTXC().commissioning.permit_join_rest));
    zb_zdo_mgmt_permit_joining_req(param, se_permit_join_for_me);

    if (ZSE_CTXC().commissioning.permit_join_rest)
    {
      ZB_SCHEDULE_ALARM(se_broadcast_permit_join, 0, alarm * ZB_TIME_ONE_SECOND);
    }
  }
}


static void se_formation_machine(zb_uint8_t param)
{
  switch (ZSE_CTXC().commissioning.signal)
  {
      /* ZC case */
    case SE_COMM_SIGNAL_NWK_FORMATION_OK:
      TRACE_MSG(TRACE_ZDO1, "SE formation done status %hd", (FMT__H, zb_buf_get_status(param)));
      /* FIXME: ZB_ZDO_SIGNAL_DEFAULT_START signal is used, is it ok?
         It is marked as legacy (pre-R21) signal. */
      ZSE_CTXC().commissioning.startup_control = ZSE_STARTUP_COMMISSIONED;
      ZSE_CTXC().commissioning.state = SE_STATE_TC_STEADY;
      /* TODO: save nvram! */
      ZB_SCHEDULE_CALLBACK2(zb_zdo_device_first_start_int_delayed, param, zb_buf_get_status(param));
      break;

    case SE_COMM_SIGNAL_NWK_FORMATION_FAILED:
      ZSE_CTXC().commissioning.formation_retries++;
      if (ZSE_CTXC().commissioning.formation_retries >= ZB_SE_MAX_FORMATION_RETRIES)
      {
        TRACE_MSG(TRACE_ZDO1, "SE formation failed retry count %hd - report unsuccessful formation to user", (FMT__H, ZSE_CTXC().commissioning.formation_retries));
        ZB_SCHEDULE_CALLBACK2(zb_zdo_startup_complete_int_delayed, param, ZB_NWK_STATUS_STARTUP_FAILURE);
      }
      else
      {
        TRACE_MSG(TRACE_ZDO1, "SE formation failed retry count %hd - retry", (FMT__H, ZSE_CTXC().commissioning.formation_retries));
        ZB_ASSERT(FORMATION_SELECTOR().start_formation);
        ZB_SCHEDULE_CALLBACK(FORMATION_SELECTOR().start_formation, param);
      }
      break;

    default:
      TRACE_MSG(TRACE_ERROR, "Unknown signal %hd", (FMT__H, ZSE_CTXC().commissioning.signal));
#ifdef SE_COMMISSIONING_DEBUG
      ZB_ASSERT(0);
#else
      if (param)
      {
        zb_buf_free(param);
      }
#endif
      break;
  }
}


static void se_tc_steady_machine(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO1, "se_tc_steady_machine param %hd signal %hd state %hd",
            (FMT__H_H_H, param, ZSE_CTXC().commissioning.signal, ZSE_CTXC().commissioning.state));

  switch (ZSE_CTXC().commissioning.signal)
  {
    case SE_COMM_SIGNAL_NWK_FORMATION_OK:
      /* We are here after ZC reboot */
      ZB_SCHEDULE_CALLBACK2(zb_zdo_device_reboot_int_delayed, param, RET_OK);
      break;
    /*
      A device that does not initiate Key 2419 Establishment
      with the Trust Center within a reasonable period of time MAY be told
      to leave 2420 depending on the network operator's policy. A maximum
      period of 20 minutes is 2421 recommended.
    */
    case SE_COMM_SIGNAL_NWK_AUTHENTICATE_REMOTE:
      /* Send NWK key to remote - either directly, or by tunneling.
         param holds already filled zb_apsme_transport_key_req_t.
         Need to keep information about joined device, so allocate second buffer.
      */
#if defined ZB_TH_ENABLED
      if (TH_CTX().options.nwk_security_disabled == 0)
#endif
      {
        /* Now commin logic in
         * zdo_commissioning_send_nwk_key_to_joined_dev->bdb_link_key_transport_with_alarm
         * sends NWK key, then inform SE. So no need to send NWK key
         * from SE. Just setup an alarm. */
        TRACE_MSG(TRACE_ZDO1, "authenticate remote dev %hd", (FMT__H, param));
        if (!se_cbke_exchange_schedule_alarm(param, se_link_key_refresh_alarm))
        {
          zb_buf_free(param);
        }
      }
      break;

    case SE_COMM_SIGNAL_CHILD_SECURED_REJOIN:
      {
      zb_apsme_transport_key_req_t *req =
        ZB_BUF_GET_PARAM(param, zb_apsme_transport_key_req_t);

      /* If it was secured rejoin, but no good TCLK exists, rejoined
       * device must complete TCLK establishment via CBKE in 20
       * minutes. */
      if (se_cbke_exchange_schedule_alarm(param, se_link_key_refresh_alarm))
      {
        TRACE_MSG(TRACE_ZDO1, "Secured rejoin. Setup alarm waiting for CBKE complete param %hd", (FMT__H, param));
      }
      else
      {
        zb_ieee_addr_t laddr;
        zb_uint8_t *ptr;

        ZB_IEEE_ADDR_COPY(laddr, req->dest_address.addr_long);
        /* Inform application about rejoin done */
        TRACE_MSG(TRACE_ZDO1, "Have a key. Inform app about remote dev rejoin param %hd", (FMT__H, param));
        /* fill info about a child! */
        ptr = zb_app_signal_pack(param, ZB_SE_SIGNAL_CHILD_REJOIN, RET_OK, sizeof(zb_ieee_addr_t));
        ZB_IEEE_ADDR_COPY(ptr, laddr);
        ZB_SCHEDULE_CALLBACK(zboss_signal_handler, param);
      }
      break;
    }

    case SE_COMM_SIGNAL_JOINED_DEVICE_CBKE_FAILED:
      TRACE_MSG(TRACE_ZDO1, "CBKE failed for addr ref %hd", (FMT__H, param));
      /* Cancel alarm and remove device immediately */
      /* Note: param is addr ref here */
      param = bdb_cancel_link_key_refresh_alarm(se_link_key_refresh_alarm, param);
      /* Now param is buffer id of filled transport key */
      /* After failed CBKE force device leave (remove device or leave). */
      /* Call BDB routine: it is adequate here. */

      /* FALLTHROUGH */
    case SE_COMM_SIGNAL_JOINED_DEVICE_CBKE_TIMED_OUT:
      TRACE_MSG(TRACE_ZDO1, "Will send remove-device param %hd", (FMT__H, param));
      if (param)
      {
        ZB_SCHEDULE_CALLBACK(bdb_link_key_refresh_alarm, param);
      }
      break;

      /* WARNING: Now the simplest scheme of BDB-SE mixed join is used - if Verify Key is ok, send
       * SE_COMM_SIGNAL_JOINED_DEVICE_NON_CBKE_OK. It is possible to authenticate device with BDB in
       * CBKE timeout, fail after multiple unsuccessful BDB join attempts, and other non-clear mixed
       * cases.
       * It is better to start both authentication types (BDB and SE) simultaneously with full
       * handling of error cases from both of them - if BDB fails in 30 sec, may still wait for SE
       * in 20 min etc. */
    case SE_COMM_SIGNAL_JOINED_DEVICE_NON_CBKE_OK:
    case SE_COMM_SIGNAL_JOINED_DEVICE_CBKE_OK:
    {
      /* here param is addr ref */
      zb_uint8_t ref = param;
      /* Joined device completed CBKE or TCLK. Cancel CBKE timeout. */
      /* Note: param is addr ref here */
      TRACE_MSG(TRACE_ZDO1, "TCLK with remote ok ref %hd", (FMT__H, param));
      param = bdb_cancel_link_key_refresh_alarm(se_link_key_refresh_alarm, param);
      /* Now param is buffer id of filled transport key */
      /* Inform application about client authenticated ok. */
      if (param)
      {
        ZB_SCHEDULE_CALLBACK2(
          ((ZSE_CTXC().commissioning.signal == SE_COMM_SIGNAL_JOINED_DEVICE_CBKE_OK) ?
          zdo_send_joined_cbke_ok_sig : zdo_send_joined_non_cbke_ok_sig),
          param,
          ref);
      }
      else
      {
        zb_buf_get_out_delayed_ext(
          (ZSE_CTXC().commissioning.signal == SE_COMM_SIGNAL_JOINED_DEVICE_CBKE_OK) ?
          zdo_send_joined_cbke_ok_sig : zdo_send_joined_non_cbke_ok_sig,
          ref, 0);
      }
    }
    break;

    default:
      TRACE_MSG(TRACE_ERROR, "Unknown signal %hd", (FMT__H, ZSE_CTXC().commissioning.signal));
#ifdef SE_COMMISSIONING_DEBUG
      ZB_ASSERT(0);
#else
      if (param)
      {
        zb_buf_free(param);
      }
#endif
      break;
  }
}


static void zdo_send_joined_cbke_ok_sig(zb_uint8_t param, zb_uint16_t aref)
{
  zb_uint8_t *ptr = zb_app_signal_pack(param, ZB_SE_TC_SIGNAL_CHILD_JOIN_CBKE, RET_OK, sizeof(zb_ieee_addr_t));

  zb_address_ieee_by_ref(ptr, aref);

#ifdef ZB_TH_ENABLED
  zb_th_expose_keys((zb_ieee_addr_t *)ptr);
#endif

  ZB_SCHEDULE_CALLBACK(zboss_signal_handler, param);
}

static void zdo_send_joined_non_cbke_ok_sig(zb_uint8_t param, zb_uint16_t aref)
{
  zb_uint8_t *ptr = zb_app_signal_pack(param, ZB_SE_TC_SIGNAL_CHILD_JOIN_NON_CBKE, RET_OK, sizeof(zb_ieee_addr_t));
  zb_address_ieee_by_ref(ptr, aref);
  ZB_SCHEDULE_CALLBACK(zboss_signal_handler, param);
}

#endif  /* ZB_COORDINATOR_ROLE */

#endif  /* ZB_SE_COMMISSIONING */


#if defined ZB_ENABLE_SE_MIN_CONFIG
void se_commissioning_signal(zse_commissioning_signal_t sig, zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO3, "se_commissioning_signal sig %hd param %hd", (FMT__H_H, sig, param));
  ZSE_CTXC().commissioning.signal = sig;

#ifdef ZB_SE_COMMISSIONING
  if (param)
  {
    ZB_SCHEDULE_CALLBACK(se_commissioning_machine, param);
  }
  else
  {
    zb_buf_get_out_delayed(se_commissioning_machine);
  }
#endif
}


zb_uint8_t se_get_ke_status_code(void)
{
  return (zb_uint8_t)ZSE_CTXC().commissioning.ke_term_info.status_code;
}
#endif  /* ZB_ENABLE_SE_MIN_CONFIG */



#ifdef ZB_SE_COMMISSIONING

void se_commissioning_machine(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO1, "se_commissioning_machine param %hd signal %hd state %hd",
            (FMT__H_H_H, param, ZSE_CTXC().commissioning.signal, ZSE_CTXC().commissioning.state));

  /* Run machines specific to the current SE device state */
  switch (ZSE_CTXC().commissioning.state)
  {
#ifdef ZB_COORDINATOR_ROLE
    case SE_STATE_FORMATION:
      ZB_SCHEDULE_CALLBACK(se_formation_machine, param);
      break;
    case SE_STATE_TC_STEADY:
      ZB_SCHEDULE_CALLBACK(se_tc_steady_machine, param);
      break;
#endif  /* ZB_COORDINATOR_ROLE */
#ifdef ZB_JOIN_CLIENT
    case SE_STATE_PRECOMMISSIONED_REJOIN:
      ZB_SCHEDULE_CALLBACK(se_precommissioned_rejoin_machine, param);
      break;
    case SE_STATE_AUTO_JOIN:
      ZB_SCHEDULE_CALLBACK(se_auto_join_machine, param);
      break;

#if defined(ZB_SE_ENABLE_SERVICE_DISCOVERY_PROCESSING)

    case SE_STATE_SERVICE_DISCOVERY_MDU:
    /* FALLTHRU */
    case SE_STATE_SERVICE_DISCOVERY:
      ZB_SCHEDULE_CALLBACK(se_service_discovery_machine, param);
      break;

#endif

    case SE_STATE_DEVICE_STEADY:
      ZB_SCHEDULE_CALLBACK(se_device_steady_machine, param);
      break;
    case SE_STATE_REJOIN_RECOVERY:
      ZB_SCHEDULE_CALLBACK(se_rejoin_recovery_machine, param);
      break;
#endif
    default:
      TRACE_MSG(TRACE_ERROR, "Unknown commissioning state %d", (FMT__D, ZSE_CTXC().commissioning.state));
#ifdef SE_COMMISSIONING_DEBUG
      ZB_ASSERT(0);
#else
      if (param)
      {
        zb_buf_free(param);
      }
#endif
      break;
  }
}


#ifdef ZB_JOIN_CLIENT

static void se_tc_precomm_rejoin_over_all_channels(zb_uint8_t param)
{
  zb_ext_pan_id_t use_ext_pan_id;
  zb_get_use_extended_pan_id(use_ext_pan_id);

  if (!param)
  {
    zb_buf_get_out_delayed(se_tc_precomm_rejoin_over_all_channels);
  }
  else
  {
    TRACE_MSG(TRACE_ZDO1, "se_tc_precomm_rejoin_over_all_channels %hd", (FMT__H, param));
    zdo_initiate_rejoin(param, use_ext_pan_id,
                        ZB_AIB().aps_channel_mask_list, ZB_FALSE);
  }
}

#if defined ZB_BDB_MODE && defined ZB_SE_BDB_MIXED
static void se_switch_to_bdb_commissioning(zb_bdb_commissioning_mode_mask_t step)
{
  ZB_ASSERT(ZSE_CTXC().commissioning.switch_to_bdb_commissioning != NULL);
  ZSE_CTXC().commissioning.switch_to_bdb_commissioning(step);
}
#endif /* ZB_BDB_MODE && ZB_SE_BDB_MIXED */

static void se_precommissioned_rejoin_machine(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO1, "se_precommissioned_rejoin_machine param %hd signal %hd state %hd",
            (FMT__H_H_H, param, ZSE_CTXC().commissioning.signal, ZSE_CTXC().commissioning.state));
  switch (ZSE_CTXC().commissioning.signal)
  {
    case SE_COMM_SIGNAL_START_REJOIN:
      /* ZG->aps.authenticated = ZB_FALSE; */
      ZSE_CTXC().commissioning.auto_join_start_time = ZB_TIMER_GET();
      ZB_SCHEDULE_CALLBACK(se_tc_precomm_rejoin_over_all_channels, param);
      break;

    case SE_COMM_SIGNAL_NWK_JOIN_FAILED:
    case SE_COMM_SIGNAL_NWK_AUTH_FAILED:
      /* ZG->aps.authenticated = ZB_FALSE; */
      TRACE_MSG(TRACE_ZDO1, "retry precommissioned rejoin", (FMT__0));
      ZB_SCHEDULE_ALARM(se_tc_precomm_rejoin_over_all_channels, param, se_next_auto_join_timeout());
      break;

    default:
      ZB_SCHEDULE_CALLBACK(se_joining_auth_common_machine, param);
      break;
  }
}


static void se_joining_auth_common_machine(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO1, "se_joining_auth_common_machine param %hd sig %hd",
            (FMT__H_H, param, ZSE_CTXC().commissioning.signal));
  switch (ZSE_CTXC().commissioning.signal)
  {
    case SE_COMM_SIGNAL_NWK_AUTH_OK:
      TRACE_MSG(TRACE_ZDO1, "NWK authenticatied ok. Starting CBKE", (FMT__0));
      /* Authenticated (got NWK key). After Join we have no TCLK. */
      /* Discover Key establishment cluster, then initiate CBKE with TC */
      se_commissioning_signal(SE_COMM_SIGNAL_START_CBKE, param);
      break;

    case SE_COMM_SIGNAL_START_CBKE:
      ZSE_CTXC().commissioning.cbke_retries = 0;

      /* FALLTHROUGH */
    case SE_COMM_SIGNAL_RETRY_CBKE:
      if (ZSE_CTXC().commissioning.cbke_retries == ZB_SE_MAX_SUCCESSION_CBKE_RETRIES)
      {
        ZSE_CTXC().commissioning.cbke_retries = 1;
        /* Retry after 15 minutes */
        TRACE_MSG(TRACE_SECUR3, "Retry CBKE after 15 minutes", (FMT__0));
        ZB_SCHEDULE_ALARM(se_send_key_est_match_desc, param, ZB_SE_PAUSE_IN_CBKE_RETRIES);
      }
      else
      {
        ZSE_CTXC().commissioning.cbke_retries++;
        TRACE_MSG(TRACE_SECUR3, "Retry CBKE # %hd", (FMT__H, ZSE_CTXC().commissioning.cbke_retries));
        ZB_SCHEDULE_CALLBACK(se_send_key_est_match_desc, param);
      }
      break;

    case SE_COMM_SIGNAL_CBKE_OK:
      /* Keypair is already saved in NVRAM, so, even if reboot now, we are commissioned. */

      /*
        Once key establishment succeeds, the device has joined the correct PAN
        and shall never leave 2423 the PAN without direction from another
        device in the network (typically an APS Remove 2424 Device command
        from an ESI or Zigbee Network Manager) or direction from the user via
        the 2425 device user interface. Example user interfaces could be a
        text menu or a simple button push 2426 sequence. It is strongly
        recommended that the user interface procedure to get a device to leave
        2427 the PAN be explicit and difficult to trigger accidentally. Leave
        commands received over the air 2428 should only be followed if the
        command is an APS encrypted APS Remove command. 2429 Network layer
        leave commands should be ignored unless the device is an end device,
        and the 2430 network leave command originated from the parent device.
      */

      TRACE_MSG(TRACE_ZDO1, "CBKE OK - send event to app", (FMT__0));
      /* Join and authentication to SE network done.
         No need to distinguish join/rr rejoin/
         Going to Steady state. */
      ZSE_CTXC().commissioning.state = SE_STATE_DEVICE_STEADY;
      /* FIXME: do we need to send that signal? Can it be ever useful for an application? */
      zb_app_signal_pack(param, ZB_SE_SIGNAL_CBKE_OK, RET_OK, 0);
      ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete, param);
      /* Send default start event to app only now, when got TCLK */
      zb_buf_get_out_delayed_ext(zb_zdo_device_first_start_int_delayed, RET_OK, 0);
      /* Start TC poll */
      if (zb_get_rx_on_when_idle())
      {
        se_commissioning_signal(SE_COMM_SIGNAL_STEADY_START_TC_POLL, 0);
      }
      else
      {
        /*
4 Sleepy end devices are not required to periodically communicate with
the Trust Center. Instead they should periodically poll their
parents and, if no parent is found after a suitable period, find 2531
and rejoin to a new parent. If no parent is found on the original
channel, the end device should enter the Rejoin and Recovery
phase described below to find a new parent.
         */
        TRACE_MSG(TRACE_ZDO3, "Sleepy ZED - no need TC poll (act like it started)", (FMT__0));
        se_commissioning_signal(SE_COMM_SIGNAL_STEADY_TC_POLL_STARTED, 0);
      }
      break;

    case SE_COMM_SIGNAL_CBKE_FAILED:
      /* Failed to pass CBKE with TC */
      /*
        If this key establishment fails, it is likely that one side of the
        exchange is configured with an invalid certificate or with no
        certificate at all. It is permissible to retry this step multiple
        times in succession, but it is expressly not allowed that a
        device repeat this step more than ten times in succession without
        pausing for a minimum of least fifteen minutes. Since the device was
        able to get a network key from the Trust Center, the device must
        have found the correct PAN to join, so there is no need to leave
        the network. A device that does not initiate Key Establishment
        with the Trust Center within a reasonable period of time MAY be told
        to leave depending on the network operator's policy. A maximum
        period of 20 minutes is recommended.
      */
      if (ZSE_CTXC().commissioning.ke_term_info.status_code == ZB_SE_KEY_ESTABLISHMENT_TERMINATE_NO_RESOURCES)
      {
        TRACE_MSG(TRACE_ZDO1, "CBKE failed - retry", (FMT__0));
        ZB_SCHEDULE_CALLBACK(zb_se_retry_cbke_with_tc, param);
      }
      else
      {
        /*
          Probably, CBKE retry can be reasonable after ecrtificate change which is an explicit action to be done by user.
          Suppose user will call zse_load_ecc_cert with another certificate/key then retry CBKE by calling zse_retry_cbke_with_tc.

          Send signel to user's app - let it decide. Stay in the network.
        */
        /* Note: we are now in the original state: auto-join/rr/precomm.rejoin */
        TRACE_MSG(TRACE_ZDO1, "CBKE failed - send event to app", (FMT__0));
        zb_app_signal_pack(param, ZB_SE_SIGNAL_CBKE_FAILED, RET_OK, 0);
        ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete, param);
#ifdef ZB_SE_BDB_MIXED
        /* If no KE endpoint found and it is 2.4 network, try to stay in the network - switch to BDB
         * mode and perform TCLK. */
        if (ZSE_CTXC().commissioning.allow_bdb_in_se_mode &&
            ZSE_CTXC().ke.dst_addr.addr_short == ZB_UNKNOWN_SHORT_ADDR &&
            ZSE_CTXC().ke.dst_ep == ZB_ZCL_BROADCAST_ENDPOINT &&
            ZB_PIBCACHE_CURRENT_PAGE() == 0)
        {
          se_switch_to_bdb_commissioning(ZB_BDB_NETWORK_STEERING);
          zdo_commissioning_authenticated(0);
        }
        else
#endif
        {
          /* Notify app about inability to establish TCLK */
          zb_buf_get_out_delayed_ext(zb_zdo_startup_complete_int_delayed, (zb_uint16_t)RET_UNAUTHORIZED, 0);
        }
      }
      break;

    case SE_COMM_SIGNAL_NWK_START_ROUTER_CONF:
      TRACE_MSG(TRACE_ZDO1, "started router", (FMT__0));
      zb_buf_free(param);
      break;

    default:
      TRACE_MSG(TRACE_ERROR, "Unknown signal %hd", (FMT__H, ZSE_CTXC().commissioning.signal));
#ifdef SE_COMMISSIONING_DEBUG
      ZB_ASSERT(0);
#else
      if (param)
      {
        zb_buf_free(param);
      }
#endif
      break;
  }
}


static void se_auto_join_machine(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO1, "se_auto_join_machine param %hd signal %hd state %hd",
            (FMT__H_H_H, param, ZSE_CTXC().commissioning.signal, ZSE_CTXC().commissioning.state));

  switch (ZSE_CTXC().commissioning.signal)
  {
    case SE_COMM_SIGNAL_START_AUTO_JOIN:
      TRACE_MSG(TRACE_ZDO1, "starting auto join", (FMT__0));
      ZSE_CTXC().commissioning.auto_join = 1;
      ZSE_CTXC().commissioning.startup_control = ZSE_STARTUP_UNCOMMISSIONED;
      ZSE_CTXC().commissioning.auto_join_start_time = ZB_TIMER_GET();
      ZB_SCHEDULE_CALLBACK(se_auto_join_iteration, param);
      break;

    case SE_COMM_SIGNAL_NWK_DISCOVERY_FAILED:
      /* Failed to find any network to join to */
      TRACE_MSG(TRACE_ZDO1, "se_commissioning_machine - retry auto-join", (FMT__0));
      /* In auto join - retry. */
      ZB_SCHEDULE_ALARM(se_auto_join_iteration, param, se_next_auto_join_timeout());
      break;

    case SE_COMM_SIGNAL_NWK_JOIN_FAILED:
      /* Failed to join to single network */
      /* Try another net from the list of discovered nets. If went out of discovered networks, will
       * go to SE_COMM_SIGNAL_NWK_DISCOVERY_FAILED. */
      TRACE_MSG(TRACE_ZDO1, "se_commissioning_machine - join failed - retry", (FMT__0));
      ZB_SCHEDULE_CALLBACK(zdo_retry_joining, param);
      break;

    case SE_COMM_SIGNAL_NWK_AUTH_FAILED:
      TRACE_MSG(TRACE_SECUR3, "Auth failed - silent leave", (FMT__0));
      /* Silent Leave, then run auto join again. Will continue after SE_COMM_SIGNAL_NWK_LEAVE signal. */
      /* FIXME: send LEAVE frame?? Do we need it in SE? TODO: Check certification logs. */
      zdo_commissioning_leave(param, ZB_FALSE, ZB_FALSE);
      break;

    case SE_COMM_SIGNAL_NWK_LEAVE:
      /* Was it leave after auth failure? If yes, retry join via scanlist.  */
      /* When we went out of scanlist, will repeat nwk discovery after timeout. */
      TRACE_MSG(TRACE_ZDO1, "Retry join after leave", (FMT__0));
      ZB_SCHEDULE_CALLBACK(zdo_retry_joining, param);
      break;

    default:
      ZB_SCHEDULE_CALLBACK(se_joining_auth_common_machine, param);
      break;
  }
}



static void se_rejoin_recovery_machine(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO1, "se_rejoin_recovery_machine param %hd signal %hd state %hd",
            (FMT__H_H_H, param, ZSE_CTXC().commissioning.signal, ZSE_CTXC().commissioning.state));


/*
  5.5.5.4 Rejoin and Recovery State

  - Now Trigger it from :
  - zb_zdo_check_fails - after N times TX failure. -- No! It forces ED scan at ZC/ZR. Keep it? TODO: check.
  - zb_secur_rejoin_after_security_failure - after unsuccessful nwk key switch.
  zb_secur_rejoin_after_security_failure also called from old tests - ignore it.
  TODO: clear clear ZG->aps.authenticated, call zdo_commissioning_initiate_rejoin
  - After unsuccessful frame unsecure zb_secur_nwk_status - zb_nlme_status_indication - if ZB_NWK_COMMAND_STATUS_IS_SECURE & parent - clear ZG->aps.authenticated, call zdo_commissioning_initiate_rejoin
  - call_status_indication - zb_nlme_status_indication - zdo_commissioning_initiate_rejoin - ZED only, ZB_NWK_COMMAND_STATUS_PARENT_LINK_FAILURE only. Call it if TX to parent failed.
  - from zb_nlme_sync_confirm if status indicates error - call zdo_commissioning_initiate_rejoin

  TODO: add common entry points:
  - NWK security failure
  - tx or poll failure
  - SE-only: APS encrypyted Communicate with TC failure (see Steady State).

  zdo_initiate_rejoin called from:
  - zb_secur_rejoin_after_security_failure
  - zdo_rejoin_backoff_initiate_rejoin - ignore it (rjb is not for se build)
  - bdb - keep it
  - zdo_commissioning_authentication_failed (classic)
  - zdo_commissioning_initiate_rejoin (classic)
  - se_initiate_commissioning after reboot - not sure need it
  - when commissioning by rejoin
  - in reaction to SE_COMM_SIGNAL_NWK_REJOIN - actually from zdo_commissioning_initiate_rejoin

  Go to r&r from Device Steady State

  To try:
  -1. Remember channel, panid, all keys, bindings before starting r&r state. Jump to 1.
  0. Wait keeping myself joined, but in r&r state.
  1. Secure rejoin on current channel
  2. TC rejoin on current channel
  3. Retry 1 then 2 three times. Timeout is ...
  4. Do network discovery seeking for long (not short!) panid. Stop discovery after receiving 1 good beacon.
  5. Maybe, reassign short panid. Try steps 1 and 2 (security then TC rejoin) at that channel.
  6. If rejoin failed, do network discovery at all channels (3 times??). Keep blacklist on.
  7. If nothing found, clear blacklist, recover using data remembered at 0 and return to 1. Note: device now think it still joined to the same PAN as before.
  8. Go to state 0
  If in state 0 received good APS & NWK encrypted packet from TC, leave r&r.

  FIXME: in state 0 retry APS exchange with TC which is normal in Device Steady State?

  Blacklists: yes, seems useful.
  Blacklist reset time: clear for r&r. TODO: recheck it for auto-join state.

  Rejoin backoff (our old implementation) - not use it with SE.
  Probably, do not compile it.
*/

  switch (ZSE_CTXC().commissioning.signal)
  {
    case SE_COMM_SIGNAL_ENTER_REJOIN_ONLY:
      /* Added for safety */
      /* FALLTHROUGH */
    case SE_COMM_SIGNAL_ENTER_REJOIN_RECOVERY:
      TRACE_MSG(TRACE_ZDO1, "already in r&r", (FMT__0));
      zb_buf_free(param);
      break;

    case SE_COMM_SIGNAL_NWK_START_ROUTER_CONF:
      if (ZSE_CTXC().commissioning.rr_ignore_start_router_conf)
      {
        ZSE_CTXC().commissioning.rr_ignore_start_router_conf = 0;
        /* We are here because we did start-router after previous r&r attempt. Do nothing staying in r&r state.*/
        TRACE_MSG(TRACE_ZDO1, "r&r iteration: started router, waiting to retry", (FMT__0));
        zb_buf_free(param);
      }
      else
      {
        se_commissioning_signal(SE_COMM_SIGNAL_REJOIN_RECOVERY_OK, param);
      }
      break;

    case SE_COMM_SIGNAL_START_REJOIN_ONLY:
      TRACE_MSG(TRACE_ZDO1, "r&r starting", (FMT__0));
      ZSE_CTXC().commissioning.rr_skip_savepoint = 1;
      ZSE_CTXC().commissioning.rr_sv_authenticated = ZB_B2U(ZG->aps.authenticated);
      /* FALLTHROUGH */
    case SE_COMM_SIGNAL_START_REJOIN_RECOVERY:
      /* step 0: save, start secured rejoin */
      TRACE_MSG(TRACE_ZDO1, "r&r starting", (FMT__0));
      if (!ZSE_CTXC().commissioning.rr_skip_savepoint)
      {
        se_savepoint_before_rr();
      }
      ZSE_CTXC().commissioning.rr_retries = 0;
      /* Will restart nwk scan from scratch */
      zb_nwk_blacklist_reset();
      se_commissioning_signal(SE_COMM_SIGNAL_REJOIN_RECOVERY_TRY_REJOIN, param);
      break;

#define SE_REJOIN_RECOVERY_RETRIES 3

    case SE_COMM_SIGNAL_REJOIN_RECOVERY_TRY_REJOIN:
    {
      zb_uint_t secure;
      zb_channel_list_t channel_list;


      /* 3 retries of secure then unsecure rejoins at current channel.
         Then 3 retries of secure then unsecure rejoins at all channels.
      */
      if (ZSE_CTXC().commissioning.rr_retries <= SE_REJOIN_RECOVERY_RETRIES * 2)
      {
        TRACE_MSG(TRACE_ZDO1, "Try rejoin over current page %hd channel %hd",
                  (FMT__H_H, ZB_PIBCACHE_CURRENT_PAGE(), ZB_PIBCACHE_CURRENT_CHANNEL()));
        zb_channel_list_init(channel_list);
        zb_channel_page_list_set_logical_channel(channel_list, ZB_PIBCACHE_CURRENT_PAGE(),
                                                 ZB_PIBCACHE_CURRENT_CHANNEL());
      }
      else
      {
        /* FIXME: do we need to use preferred channels here? */
        TRACE_MSG(TRACE_ZDO1, "Try rejoin over all channels", (FMT__0));
        zb_channel_page_list_copy(channel_list, ZB_AIB().aps_channel_mask_list);
      }
      if (!ZSE_CTXC().commissioning.rr_sv_authenticated)
      {
        ZSE_CTXC().commissioning.rr_retries++;
      }
      /* Every second retry is unsecured */
      secure = (ZSE_CTXC().commissioning.rr_retries % 2 == 0);
      if (secure)
      {
        ZG->aps.authenticated = ZB_TRUE;
      }
      TRACE_MSG(TRACE_ZDO1, "rejoin secure %d", (FMT__D, secure));
      zdo_initiate_rejoin(param, ZB_NIB_EXT_PAN_ID(),
                          channel_list,
                          (zb_bool_t)secure);
      ZSE_CTXC().commissioning.rr_retries++;
      TRACE_MSG(TRACE_ZDO1, "new rejoin retries %d", (FMT__D, ZSE_CTXC().commissioning.rr_retries));
      break;
    }

    case SE_COMM_SIGNAL_NWK_AUTH_FAILED:
      TRACE_MSG(TRACE_ZDO1, "Failed NWK authentication", (FMT__0));
      /* FALLTHROUGH */
    case SE_COMM_SIGNAL_NWK_LEAVE: /* Is it possible? */
    case SE_COMM_SIGNAL_NWK_JOIN_FAILED:
      TRACE_MSG(TRACE_ZDO1, "Failed rejoin", (FMT__0));
      if (ZSE_CTXC().commissioning.rr_retries <= SE_REJOIN_RECOVERY_RETRIES * 2 * 2)
      {
        TRACE_MSG(TRACE_ZDO1, "Keep rejoining", (FMT__0));
        se_commissioning_signal(SE_COMM_SIGNAL_REJOIN_RECOVERY_TRY_REJOIN, param);
      }
      else
      {
        zb_time_t t;

        /*
While in the R&R phase, the device shall retry steps 1-8 periodically,
at least once every 24 hours. Sleepy end devices may use a longer
period. After four failed rejoin attempts, devices should not try
to rejoin any faster than once per hour, with a jitter of +/- 30
minutes.
         */
        ZSE_CTXC().commissioning.rr_global_retries++;
#ifdef DEBUG_RR
#define REJOIN_TIME_DIVIDER 300
#else
#define REJOIN_TIME_DIVIDER 1
#endif
        if (ZSE_CTXC().commissioning.rr_global_retries < 4)
        {
          t = ZB_MILLISECONDS_TO_BEACON_INTERVAL((60 - 15 + ZB_RANDOM_JTR(15 * 2)) * 1000 / REJOIN_TIME_DIVIDER);
        }
        else
        {
          /* once per hour +/- 30 minutes */
          t = ZB_MILLISECONDS_TO_BEACON_INTERVAL((60 - 30 + ZB_RANDOM_JTR(30 * 2)) * 60 * 1000 / REJOIN_TIME_DIVIDER);
        }
        TRACE_MSG(TRACE_ZDO1, "Out of rejoin attempts. Retries %d. Restore joined state and wait there for %d sec",
                  (FMT__D_D, ZSE_CTXC().commissioning.rr_global_retries,
                   t / ZB_TIME_ONE_SECOND));
        if (ZSE_CTXC().commissioning.rr_global_retries == ZB_UINT_MAX - 1)
        {
          /* Highly unlikely we can stay here for such a long time,
           * but let's handle retry counters overflow and stay in a
           * long retry. */
          ZSE_CTXC().commissioning.rr_global_retries = 4;
        }

        TRACE_MSG(TRACE_ZDO1, "skip savepoint %hd", (FMT__H, !ZSE_CTXC().commissioning.rr_skip_savepoint));
        if (!ZSE_CTXC().commissioning.rr_skip_savepoint)
        {
          ZB_SCHEDULE_CALLBACK(se_restore_saved_rr, param);
        }
        ZB_SCHEDULE_ALARM(se_retry_rejoin_recovery, 0, t);
      }
      break;

    case SE_COMM_SIGNAL_NWK_AUTH_OK:
      /* No FSM transition, waiting for SE_COMM_SIGNAL_REJOIN_RECOVERY_OK
       * signal */
      zb_buf_free(param);
      break;

    case SE_COMM_SIGNAL_REJOIN_RECOVERY_OK:
      /* In r&r we already have TCLK, so we are now done */
      TRACE_MSG(TRACE_ZDO1, "Completed successful rejoin & authentication. Leaving r&r - go to steady state", (FMT__0));
      ZSE_CTXC().commissioning.state = SE_STATE_DEVICE_STEADY;
      ZSE_CTXC().commissioning.rr_global_retries = 0;
      ZSE_CTXC().commissioning.rr_skip_savepoint = 0;
      if (ZSE_CTXC().commissioning.just_booted)
      {
        /* Utilized r&r state to rejoin after reboot. Now inform app that started ok */
        zb_buf_get_out_delayed_ext(zb_zdo_device_reboot_int_delayed, RET_OK, 0);
        ZSE_CTXC().commissioning.just_booted = 0;

#if defined(ZB_SE_ENABLE_STEADY_STATE_PROCESSING)
        /* Start TC poll after reboot */
        if (zb_get_rx_on_when_idle())
        {
          se_commissioning_signal(SE_COMM_SIGNAL_STEADY_START_TC_POLL, 0);
        }
#endif
      }
      ZB_SCHEDULE_ALARM_CANCEL(se_retry_rejoin_recovery, ZB_ALARM_ANY_PARAM);
#if defined(ZB_SE_ENABLE_SERVICE_DISCOVERY_PROCESSING)
      /* advise app to start discovery */
      /* FIXME: MM: Temporary disabled during state machine debugging */
      //ZB_SCHEDULE_CALLBACK(zb_se_service_discovery_send_start_signal, param);
#endif
      zb_buf_free(param);
      break;

    default:
      TRACE_MSG(TRACE_ERROR, "Unknown signal %hd", (FMT__H, ZSE_CTXC().commissioning.signal));
#ifdef SE_COMMISSIONING_DEBUG
      ZB_ASSERT(0);
#else
      if (param)
      {
        zb_buf_free(param);
      }
#endif
      break;
  }
}


static void se_savepoint_before_rr()
{
  TRACE_MSG(TRACE_ZDO1, "se_savepoint_before_rr", (FMT__0));

  ZSE_CTXC().commissioning.rr_sv_authenticated = ZB_B2U(ZG->aps.authenticated);
  ZSE_CTXC().commissioning.rr_sv_device_type = zb_get_device_type();

  /* Save parent info. We do not want to reload it from nvram because neighbor table load supposes clear neighbor table. */
  ZSE_CTXC().commissioning.rr_sv_parent_short = 0xffff;
  if (ZG->nwk.handle.parent != (zb_uint8_t)-1)
  {
    zb_neighbor_tbl_ent_t *nent = NULL;
    zb_address_by_ref(ZSE_CTXC().commissioning.rr_sv_parent_long, &ZSE_CTXC().commissioning.rr_sv_parent_short, ZG->nwk.handle.parent);
    if (zb_nwk_neighbor_get(ZG->nwk.handle.parent, ZB_FALSE, &nent) == RET_OK)
    {
      ZSE_CTXC().commissioning.rr_sv_parent_nent = *nent;
    }
  }
  /* All other information is now in our nvram. We can restore it from there. */
}


static zb_bool_t se_ds_filter_cb(zb_uint16_t i)
{
  switch (i)
  {
    case ZB_NVRAM_COMMON_DATA:
      return ZB_TRUE;
    default:
      return ZB_FALSE;
  }
}


static void se_restore_cont(zb_uint8_t param)
{
  /* Undo effect of zdo_rejoin_clear_prev_join(). */
  /* If ZED, start poll again */
  if (ZB_IS_DEVICE_ZED())
  {
    if (!ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()))
    {
      zb_zdo_pim_start_poll(0);
    }
    zb_buf_free(param);
  }
  else
  {
    ZSE_CTXC().commissioning.rr_ignore_start_router_conf = 1;
    ZB_SCHEDULE_CALLBACK(zb_zdo_start_router, param);
  }
}


static void se_restore_saved_rr(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO1, "se_restore_saved_rr param %hd", (FMT__H, param));

  /* get common dataset from nvram */
  ZB_NVRAM().ds_filter_cb = se_ds_filter_cb;
  zb_nvram_load();
  ZB_NVRAM().ds_filter_cb = NULL;

  if (ZSE_CTXC().commissioning.rr_sv_parent_short != 0xffff)
  {
    zb_neighbor_tbl_ent_t *nent = NULL;
    /* Create and lock parent */
    zb_address_update(ZSE_CTXC().commissioning.rr_sv_parent_long, ZSE_CTXC().commissioning.rr_sv_parent_short, ZB_TRUE, &ZG->nwk.handle.parent);
    if (zb_nwk_neighbor_get(ZG->nwk.handle.parent, ZB_TRUE, &nent) == RET_OK)
    {
      nent->relationship = ZB_NWK_RELATIONSHIP_PARENT;
    }
  }

  {
    zb_address_ieee_ref_t ref;
    zb_address_update(ZB_PIBCACHE_EXTENDED_ADDRESS(),
                      ZB_PIBCACHE_NETWORK_ADDRESS(),
                      ZB_TRUE, &ref);
  }
  ZB_NIB().device_type = (zb_nwk_device_type_t)ZSE_CTXC().commissioning.rr_sv_device_type;
  ZG->aps.authenticated = ZB_U2B(ZSE_CTXC().commissioning.rr_sv_authenticated);
  ZB_SET_JOINED_STATUS(ZB_TRUE);

  /* Now pibcache is filled by right values, but values are not pushed to MAC. Switch MAC channel etc. */
  zb_nwk_sync_pibcache_with_mac(param, se_restore_cont);
}


static void se_retry_rejoin_recovery(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO1, "se_retry_rejoin_recovery param %hd", (FMT__H, param));
  if (!param)
  {
    zb_buf_get_out_delayed(se_retry_rejoin_recovery);
  }
  else
  {
    se_commissioning_signal(SE_COMM_SIGNAL_START_REJOIN_RECOVERY, param);
  }
}


#if defined(ZB_SE_ENABLE_SERVICE_DISCOVERY_PROCESSING)

static void se_service_discovery_machine(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO1, "se_service_discovery_machine param %hd signal %hd state %hd",
            (FMT__H_H_H, param, ZSE_CTXC().commissioning.signal, ZSE_CTXC().commissioning.state));

  switch (ZSE_CTXC().commissioning.signal)
  {
    case SE_COMM_SIGNAL_SERVICE_DISCOVERY_OK:
      zb_app_signal_pack(param, ZB_SE_SIGNAL_SERVICE_DISCOVERY_OK, 0, 0);
      ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete, param);
      /* TODO: Link key exchange between devices through TC? */
      /* Back to steady state */
#ifdef ZB_COORDINATOR_ROLE
        if (ZB_IS_DEVICE_ZC())
        {
          ZSE_CTXC().commissioning.state = SE_STATE_TC_STEADY;
        }
        else
#endif  /* ZB_COORDINATOR_ROLE */
        {
          ZSE_CTXC().commissioning.state = SE_STATE_DEVICE_STEADY;
        }
      break;

    case SE_COMM_SIGNAL_SERVICE_DISCOVERY_FAILED:
      /* Notify user about failure */
      ZSE_CTXC().commissioning.state = SE_STATE_JOINED_NODEV;
      zb_app_signal_pack(param, ZB_SE_SIGNAL_SERVICE_DISCOVERY_FAILED, 0, 0);
      ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete, param);
      break;

    case SE_COMM_SIGNAL_ENTER_REJOIN_ONLY:
      TRACE_MSG(TRACE_ZDO1, "Starting rejoin and recovery", (FMT__0));
      ZSE_CTXC().commissioning.state = SE_STATE_REJOIN_RECOVERY;
      se_commissioning_signal(SE_COMM_SIGNAL_START_REJOIN_ONLY, param);
      break;

    case SE_COMM_SIGNAL_ENTER_REJOIN_RECOVERY:
      /* FIXME: save current state and continue after recovered? Or, start discovery from scratch then? */
      TRACE_MSG(TRACE_ZDO1, "Starting rejoin and recovery", (FMT__0));
      ZSE_CTXC().commissioning.state = SE_STATE_REJOIN_RECOVERY;
      se_commissioning_signal(SE_COMM_SIGNAL_START_REJOIN_RECOVERY, param);
      break;

    case SE_COMM_SIGNAL_NWK_START_ROUTER_CONF:
      /* FIXME: Ignore? */
      TRACE_MSG(TRACE_ZDO1, "started router", (FMT__0));
      zb_buf_free(param);
      break;

    default:
      TRACE_MSG(TRACE_ZDO1, "Signal %hd not handled in Service Discovery state, we are TC, let's try to handle it using steady machine", (FMT__H, ZSE_CTXC().commissioning.signal));
#if defined ZB_COORDINATOR_ROLE
      if (ZB_IS_DEVICE_ZC())
      {
        se_tc_steady_machine(param);
      }
      else
#endif /* ZB_ROUTER_ROLE */
      {
        se_device_steady_machine(param);
      }
      break;
  }
}

#endif


static void se_device_steady_machine(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO1, "se_device_steady_machine param %hd signal %hd state %hd",
            (FMT__H_H_H, param, ZSE_CTXC().commissioning.signal, ZSE_CTXC().commissioning.state));


/*
  5.5.5.3 Device Steady State

  a) ZED
  - poll parent. Rejoin after N failures. We have this logic already.

  b) ZR
  Periodically read something APS encrypted from TC.

  - Detect if TC supports Metering cluster. If yes, read its the current consumption attribute.
  - Implement Keep-Alive cluster and use it.
  - Failure == no good reply from TC, or can't decrypt reply. Do not count failed TX attempts.

*/

  switch (ZSE_CTXC().commissioning.signal)
  {
    case SE_COMM_SIGNAL_NWK_START_ROUTER_CONF:
      /* FIXME: Ignore? */
      TRACE_MSG(TRACE_ZDO1, "started router", (FMT__0));
      if (ZSE_CTXC().commissioning.just_booted)
      {
        TRACE_MSG(TRACE_ZDO1, "Inform app about start complete", (FMT__0));
        /* Utilized r&r state to rejoin after reboot. Now inform app that started ok */
        ZB_SCHEDULE_CALLBACK2(zb_zdo_device_reboot_int_delayed, param, RET_OK);
#if defined(ZB_SE_ENABLE_SERVICE_DISCOVERY_PROCESSING)
        ZB_SCHEDULE_CALLBACK(zb_se_service_discovery_send_start_signal_delayed, 0);
#endif
        ZSE_CTXC().commissioning.just_booted = 0;

#if defined(ZB_SE_ENABLE_STEADY_STATE_PROCESSING)
        /* Start TC poll after reboot */
        if (zb_get_rx_on_when_idle())
        {
          se_commissioning_signal(SE_COMM_SIGNAL_STEADY_START_TC_POLL, 0);
        }
#endif
      }
      else
      {
        zb_buf_free(param);
      }
      break;

#if defined(ZB_SE_ENABLE_STEADY_STATE_PROCESSING)

    case SE_COMM_SIGNAL_STEADY_START_TC_POLL:
      zb_se_steady_state_start_periodic_tc_poll();
      if (param)
      {
        zb_buf_free(param);
      }
      break;

    case SE_COMM_SIGNAL_STEADY_TC_POLL_FAILED:
      /* TC poll failed - do R&R */
      /* EA: To make parent search */
      TRACE_MSG(TRACE_ZDO1, "SE_COMM_SIGNAL_STEADY_TC_POLL_FAILED: Starting rejoin and recovery", (FMT__0));
      ZSE_CTXC().commissioning.state = SE_STATE_REJOIN_RECOVERY;
      se_commissioning_signal(SE_COMM_SIGNAL_START_REJOIN_RECOVERY, param);
      /* ZB_FREE_BUF_BY_REF(param); */
      break;

    case SE_COMM_SIGNAL_STEADY_TC_POLL_STARTED:
#if defined ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ && defined ZB_ED_ROLE
      /* We may be at 2.4 or not ZED - zb_subghz_start_suspend_status_poll checks for it. */
      /* Now seek for our Sub-GHz cluster endpoint  */
      {
        zb_uint8_t subghz_ep = zb_zcl_get_next_target_endpoint(
          0, ZB_ZCL_CLUSTER_ID_SUB_GHZ, ZB_ZCL_CLUSTER_ANY_ROLE, ZB_AF_SE_PROFILE_ID);

        if (subghz_ep == 0)
        {
          subghz_ep = zb_zcl_get_next_target_endpoint(
            0, ZB_ZCL_CLUSTER_ID_SUB_GHZ, ZB_ZCL_CLUSTER_ANY_ROLE, ZB_AF_HA_PROFILE_ID);
        }

        if (subghz_ep == 0)
        {
          TRACE_MSG(TRACE_ZDO1, "No SubGHz endpoint, skip suspend_status_poll", (FMT__0));
        }
        else
        {
          ZB_SCHEDULE_ALARM(zb_subghz_start_suspend_status_poll, 0, ZB_TIME_ONE_SECOND);
        }
      }
#endif
      /* Now can start Service Discovery: send app signal */
#if defined(ZB_SE_ENABLE_SERVICE_DISCOVERY_PROCESSING)
      zb_se_service_discovery_send_start_signal_delayed(0);
#endif
      if (param)
      {
        zb_buf_free(param);
      }
      break;

    case SE_COMM_SIGNAL_STEADY_TC_POLL_NOT_SUPPORTED:
/* 08/01/2017 EE CR:MAJOR No need to rejoin if we can't find poll method. Send another signal? Ignore? */
#ifdef SE_COMMISSIONING_DEBUG
      ZB_ASSERT(0);
#else
      if (param)
      {
        zb_buf_free(param);
      }
#endif
      break;

#endif

    case SE_COMM_SIGNAL_NWK_LEAVE:
      /* Leave forced by other device or our app */
      TRACE_MSG(TRACE_ZDO1, "report LEAVE to app", (FMT__0));
      /* TODO: clear everything, like in BDB? */
#if defined(ZB_SE_ENABLE_STEADY_STATE_PROCESSING)
      zb_se_steady_state_stop_periodic_tc_poll();
#endif
#if defined(ZB_SE_ENABLE_SERVICE_DISCOVERY_PROCESSING)
      zb_se_service_discovery_stop();
#endif
      ZB_SCHEDULE_CALLBACK2(zb_zdo_startup_complete_int_delayed, param, (zb_uint16_t)RET_CONNECTION_LOST);
      break;

    case SE_COMM_SIGNAL_ENTER_REJOIN_ONLY:
      TRACE_MSG(TRACE_ZDO1, "Starting rejoin only in scope of rejoin & recovery", (FMT__0));
      ZSE_CTXC().commissioning.state = SE_STATE_REJOIN_RECOVERY;
      se_commissioning_signal(SE_COMM_SIGNAL_START_REJOIN_ONLY, param);
      break;

    case SE_COMM_SIGNAL_ENTER_REJOIN_RECOVERY:
      TRACE_MSG(TRACE_ZDO1, "Starting rejoin and recovery", (FMT__0));
      ZSE_CTXC().commissioning.state = SE_STATE_REJOIN_RECOVERY;
      se_commissioning_signal(SE_COMM_SIGNAL_START_REJOIN_RECOVERY, param);
      break;

#if defined(ZB_SE_ENABLE_SERVICE_DISCOVERY_PROCESSING)

    case SE_COMM_SIGNAL_SERVICE_DISCOVERY_OK:
      TRACE_MSG(TRACE_ZDO1, "Received SE_COMM_SIGNAL_SERVICE_DISCOVERY_OK signal", (FMT__0));
      if (param)
      {
        zb_buf_free(param);
      }
      break;

#endif

    default:
      TRACE_MSG(TRACE_ERROR, "Unknown signal %hd", (FMT__H, ZSE_CTXC().commissioning.signal));
#ifdef SE_COMMISSIONING_DEBUG
      ZB_ASSERT(0);
#else
      if (param)
      {
        zb_buf_free(param);
      }
#endif
      break;
  }
}


#endif  /* #ifndef ZB_COORDINATOR_ONLY */

void zb_se_retry_cbke_with_tc(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO1, "zse_retry_cbke_with_tc %hd", (FMT__H, param));
  se_commissioning_signal(SE_COMM_SIGNAL_RETRY_CBKE, param);
}

#ifdef ZB_COORDINATOR_ROLE

/* Schedules CBKE refresh alarm if necessary
 * Returns ZB_TRUE if scheduled, ZB_FALSE otherwise
 */
zb_bool_t se_cbke_exchange_schedule_alarm(zb_uint8_t param, zb_callback_t alarm)
{
  /* Schedule only in case CBKE key is missing */
  zb_bool_t scheduled = ZB_FALSE;
  zb_apsme_transport_key_req_t *req =
    ZB_BUF_GET_PARAM(param, zb_apsme_transport_key_req_t);
  zb_aps_device_key_pair_set_t *aps_key;
  zb_address_ieee_ref_t addr_ref;
  zb_uint8_t param2;

  aps_key = zb_secur_get_link_key_by_address(req->dest_address.addr_long, ZB_SECUR_VERIFIED_KEY);
  if (!aps_key || aps_key->key_source != ZB_SECUR_KEY_SRC_CBKE)
  {
    if (zb_address_by_ieee(req->dest_address.addr_long, ZB_FALSE, ZB_FALSE, &addr_ref) == RET_OK)
    {
      param2 = bdb_cancel_link_key_refresh_alarm(alarm, addr_ref);
      if (param2)
      {
        zb_buf_free(param2);
      }
    }

    ZB_SCHEDULE_ALARM(alarm, param, ZB_SE_JOIN_DEV_CBKE_TIMEOUT);
    scheduled = ZB_TRUE;
  }

  return scheduled;
}


static void se_link_key_refresh_alarm(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO1, "se_link_key_refresh_alarm %hd", (FMT__H, param));
  se_commissioning_signal(SE_COMM_SIGNAL_JOINED_DEVICE_CBKE_TIMED_OUT, param);
}


static void kec_srv_done_cb(zb_uint8_t ref, zb_uint16_t status)
{
  TRACE_MSG(TRACE_ZDO2, "kec_srv_done_cb %hd %d", (FMT__H_D, ref, status));
  if (status == RET_OK)
  {
    se_commissioning_signal(SE_COMM_SIGNAL_JOINED_DEVICE_CBKE_OK, ref);
  }
  else
  {
    se_commissioning_signal(SE_COMM_SIGNAL_JOINED_DEVICE_CBKE_FAILED, ref);
  }
}
/**
   Seek for key establishment cluster at TC.
*/
void send_tc_key_est_match_desc(zb_uint8_t param, zb_uint16_t addr_of_interest)
{
  if (!param)
  {
    zb_buf_get_out_delayed_ext(send_tc_key_est_match_desc, addr_of_interest, 0);
  }
  else
  {
    zb_zdo_match_desc_param_t *req;

    zb_uint8_t ke_ep = zb_zcl_get_next_target_endpoint(
      0, ZB_ZCL_CLUSTER_ID_KEY_ESTABLISHMENT, ZB_ZCL_CLUSTER_ANY_ROLE, ZB_AF_SE_PROFILE_ID);

    if (ke_ep == 0)
    {
      ke_ep = zb_zcl_get_next_target_endpoint(
        0, ZB_ZCL_CLUSTER_ID_KEY_ESTABLISHMENT, ZB_ZCL_CLUSTER_ANY_ROLE, ZB_AF_HA_PROFILE_ID);
    }

    TRACE_MSG(TRACE_SECUR3, "send_tc_key_est_match_desc to %x", (FMT__D, addr_of_interest));

    req = zb_buf_initial_alloc(param, sizeof(zb_zdo_match_desc_param_t) + 1 * sizeof(zb_uint16_t));

    /* Send unicast to ZC */
    req->nwk_addr = req->addr_of_interest = addr_of_interest;
    req->profile_id = get_profile_id_by_endpoint(ke_ep);
    req->num_in_clusters = 1;
    req->num_out_clusters = 0;
    req->cluster_list[0] = ZB_ZCL_CLUSTER_ID_KEY_ESTABLISHMENT;

    zb_zdo_match_desc_req(param, tc_key_est_match_desc_cb);
  }
}

/**
   Callback called when match desc req seeking for key establishment cluster is completed.
*/
static void tc_key_est_match_desc_cb(zb_uint8_t param)
{
  zb_uint8_t *zdp_cmd = zb_buf_begin(param);
  zb_zdo_match_desc_resp_t *resp = (zb_zdo_match_desc_resp_t*)zdp_cmd;
  zb_uint8_t local_ep;
  zb_uint8_t remote_ep;
  zb_uint16_t remote_short_addr = resp->nwk_addr;
  /*
    asdu=Match_Descr_rsp(Status=0x00=Success, NWKAddrOfInterest=0x0000,
    MatchLength=0x01, MatchList=0x01)
  */

  /* User's application, when declaring clusters & endpoints for SE, MUST declate Key Establishment cluster.
     We can have multiple endpoints.
     Now see for one supporting SE.
  */
  /* Now seek for our SE endpoint  */
  local_ep = zb_zcl_get_next_target_endpoint(
    0, ZB_ZCL_CLUSTER_ID_KEY_ESTABLISHMENT, ZB_ZCL_CLUSTER_ANY_ROLE, ZB_AF_SE_PROFILE_ID);

  if (local_ep == 0)
  {
    local_ep = zb_zcl_get_next_target_endpoint(
    0, ZB_ZCL_CLUSTER_ID_KEY_ESTABLISHMENT, ZB_ZCL_CLUSTER_ANY_ROLE, ZB_AF_HA_PROFILE_ID);
  }

  if (local_ep == 0)
  {
    TRACE_MSG(TRACE_ERROR, "No local KE endpoint!", (FMT__0));
  }
  if (local_ep && resp->status == ZB_ZDP_STATUS_SUCCESS && resp->match_len >= 1)
  {
    /* There can be only one endpoint in match resp: SE with key establishment cluster. */
    remote_ep = *((zb_uint8_t*)(resp + 1));
    TRACE_MSG(TRACE_ZDO1, "key_est_match_desc_cb local ep %hd remote ep %hd", (FMT__H_H, local_ep, remote_ep));
    /* Note: we remember local/remote endpoints inside, so can reuse it later if need. */
#ifndef ZSE_HACKS_SKIP_KEC
    zb_se_kec_initiate_key_establishment(param, local_ep, remote_ep, remote_short_addr, kec_done_cb);
#else
    zb_buf_free(param);
#endif /* ZSE_HACKS_SKIP_KEC */
  }
  else
  {
    TRACE_MSG(TRACE_ZDO1, "No local/remote SE endpoint with key establishment cluster", (FMT__0));
    ZSE_CTXC().commissioning.ke_term_info.status_code = ZB_SE_KEY_ESTABLISHMENT_TERMINATE_NO_KE_EP;
    call_client_ke_done_cb(param, RET_ERROR);
  }
}

void se_tc_update_link_key(zb_uint16_t addr_of_interest)
{
  zb_buf_get_out_delayed_ext(send_tc_key_est_match_desc, addr_of_interest, 0);
}
#endif  /* #ifdef ZB_COORDINATOR_ROLE */
#endif  /* #ifdef ZB_SE_COMMISSIONING */

#if defined ZB_JOIN_CLIENT && defined ZB_ENABLE_SE_MIN_CONFIG
/**
   Seek for key establishment cluster at addr_of_interest.
*/
void se_send_key_est_match_desc_addr(zb_uint8_t param, zb_uint16_t addr_of_interest)
{
  if (param == 0U)
  {
    zb_ret_t ret = zb_buf_get_out_delayed_ext(se_send_key_est_match_desc_addr, addr_of_interest, 0);
    if (ret != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
    }
  }
  else
  {
    zb_zdo_match_desc_param_t *req;

    zb_uint8_t ke_ep = zb_zcl_get_next_target_endpoint(
      0, ZB_ZCL_CLUSTER_ID_KEY_ESTABLISHMENT, ZB_ZCL_CLUSTER_ANY_ROLE, ZB_AF_SE_PROFILE_ID);

    if (ke_ep == 0U)
    {
      ke_ep = zb_zcl_get_next_target_endpoint(
        0, ZB_ZCL_CLUSTER_ID_KEY_ESTABLISHMENT, ZB_ZCL_CLUSTER_ANY_ROLE, ZB_AF_HA_PROFILE_ID);
    }

    TRACE_MSG(TRACE_SECUR3, "send_key_est_match_desc", (FMT__0));

    {
      zb_size_t req_len = sizeof(zb_zdo_match_desc_param_t) + 1U * sizeof(zb_uint16_t);
      req = zb_buf_initial_alloc(param, req_len);
    }

    /* Send unicast to ZC */
    req->nwk_addr = req->addr_of_interest = addr_of_interest;
    req->profile_id = get_profile_id_by_endpoint(ke_ep);
    req->num_in_clusters = 1;
    req->num_out_clusters = 0;
    req->cluster_list[0] = ZB_ZCL_CLUSTER_ID_KEY_ESTABLISHMENT;

    (void)zb_zdo_match_desc_req(param, key_est_match_desc_cb);
  }
}

/**
   Seek for key establishment cluster at TC.
*/
void se_send_key_est_match_desc(zb_uint8_t param)
{
  if (param == 0U)
  {
    zb_ret_t ret = zb_buf_get_out_delayed_ext(se_send_key_est_match_desc_addr, 0x0000, 0);
    if (ret != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
    }
  }
  else
  {
    se_send_key_est_match_desc_addr(param, 0x0000);
  }
}


/**
   Callback called when match desc req seeking for key establishment cluster is completed.
*/
static void key_est_match_desc_cb(zb_uint8_t param)
{
  zb_zdo_match_desc_resp_t *resp = zb_buf_begin(param);
  zb_uint8_t local_ep;
  zb_uint8_t remote_ep;
  zb_uint16_t remote_short_addr = resp->nwk_addr;
  /*
    asdu=Match_Descr_rsp(Status=0x00=Success, NWKAddrOfInterest=0x0000,
    MatchLength=0x01, MatchList=0x01)
  */

  /* User's application, when declaring clusters & endpoints for SE, MUST declate Key Establishment cluster.
     We can have multiple endpoints.
     Now see for one supporting SE.
  */
  /* Now seek for our SE endpoint  */
  local_ep = zb_zcl_get_next_target_endpoint(
    0, ZB_ZCL_CLUSTER_ID_KEY_ESTABLISHMENT, ZB_ZCL_CLUSTER_ANY_ROLE, ZB_AF_SE_PROFILE_ID);

  if (local_ep == 0U)
  {
    local_ep = zb_zcl_get_next_target_endpoint(
      0, ZB_ZCL_CLUSTER_ID_KEY_ESTABLISHMENT, ZB_ZCL_CLUSTER_ANY_ROLE, ZB_AF_HA_PROFILE_ID);
  }

  if (local_ep == 0U)
  {
    TRACE_MSG(TRACE_ERROR, "No local KE endpoint!", (FMT__0));
  }
  if (
    (local_ep != 0U)
    && resp->status == (zb_uint8_t)ZB_ZDP_STATUS_SUCCESS
    && resp->match_len >= 1U
  )
  {
    /* There can be only one endpoint in match resp: SE with key establishment cluster. */
    remote_ep = *((zb_uint8_t*)(resp + 1));
    TRACE_MSG(TRACE_ZDO1, "key_est_match_desc_cb local ep %hd remote ep %hd", (FMT__H_H, local_ep, remote_ep));
    /* Note: we remember local/remote endpoints inside, so can reuse it later if need. */
#ifndef ZSE_HACKS_SKIP_KEC
    (void)zb_se_kec_initiate_key_establishment(param, local_ep, remote_ep, remote_short_addr, kec_done_cb);
#else
    zb_buf_free(param);
#endif /* ZSE_HACKS_SKIP_KEC */
  }
  else
  {
    TRACE_MSG(TRACE_ZDO1, "No local/remote SE endpoint with key establishment cluster", (FMT__0));
    ZSE_CTXC().commissioning.ke_term_info.status_code = ZB_SE_KEY_ESTABLISHMENT_TERMINATE_NO_KE_EP;
    call_client_ke_done_cb(param, RET_ERROR);
  }
}

#endif /* defined ZB_JOIN_CLIENT && defined ZB_ENABLE_SE_MIN_CONFIG */


#ifdef ZB_SE_ENABLE_KEC_CLUSTER
void zse_kec_set_client_cb(zb_kec_client_cb_t cb)
{
  ZSE_CTXC().ke.client_cb = cb;
}

static void kec_done_cb(zb_uint8_t param)
{
  call_client_ke_done_cb(param, zb_buf_get_status(param));
}

static void call_client_ke_done_cb(zb_uint8_t param, zb_ret_t status)
{
  ZB_ASSERT(ZSE_CTXC().ke.client_cb != NULL);
  ZSE_CTXC().ke.client_cb(param, status);
}

static void client_ke_done(zb_uint8_t param, zb_ret_t status)
{
  if (status == RET_OK)
  {
    TRACE_MSG(TRACE_ZDO1, "SE Key establishment with TC done ok param %hd", (FMT__H, param));
    se_commissioning_signal(SE_COMM_SIGNAL_CBKE_OK, param);
  }
  else
  {
    TRACE_MSG(TRACE_ZDO1, "SE Key establishment with TC failed param %hd", (FMT__H, param));
    se_commissioning_signal(SE_COMM_SIGNAL_CBKE_FAILED, param);
  }
}
#endif  /* ZB_ENABLE_SE */

#ifdef ZB_SE_COMMISSIONING

#if defined ZB_SUBGHZ_BAND_ENABLED
static zb_bool_t se_get_high_freq_slot(zb_uint16_t clusterid, zb_bool_t force)
{
  zb_int_t i;
  zb_int_t free_i = -1;
  zb_bool_t hifreq = ZB_FALSE;
  zb_time_t t = ZB_TIMER_GET();

  switch (clusterid)
  {
    case ZB_ZCL_CLUSTER_ID_SUB_GHZ:
    case ZB_ZCL_CLUSTER_ID_KEY_ESTABLISHMENT:
    case ZB_ZCL_CLUSTER_ID_KEEP_ALIVE:
      /* Do not try to auto-detect hi freq msgs for internal clusters */
      TRACE_MSG(TRACE_ZDO3, "se_get_high_freq_slot - ignore cluster %d", (FMT__D, clusterid));
      return ZB_FALSE;
  }
  if (ZSE_CTXC().commissioning.state != SE_STATE_TC_STEADY
      && ZSE_CTXC().commissioning.state != SE_STATE_DEVICE_STEADY)
  {
      /* hi freq is meaningful in steady state only */
      TRACE_MSG(TRACE_ZDO3, "se_get_high_freq_slot - commissioning is in progress, state %d", (FMT__D, ZSE_CTXC().commissioning.state));
      return ZB_FALSE;
  }
  if (ZB_PIBCACHE_CURRENT_PAGE() == 0)
  {
    /* hifreq is valid in sub-ghz only */
    TRACE_MSG(TRACE_ZDO3, "se_get_high_freq_slot - in 2.4 - skip", (FMT__0));
    return ZB_FALSE;
  }

  {
    static zb_bool_t inited = ZB_FALSE;
    if (!inited)
    {
      TRACE_MSG(TRACE_ZDO3, "se_get_high_freq_slot - init", (FMT__0));
      for (i = 0 ; i < ZB_SE_HI_FREQ_MSG_N_SLOTS ; ++i)
      {
        ZSE_CTXC().subghz.hifreq[i].timestamp = t;
        ZSE_CTXC().subghz.hifreq[i].clusterid = 0;
      }
      inited = ZB_TRUE;
    }
  }

  for (i = 0 ; i < ZB_SE_HI_FREQ_MSG_N_SLOTS ; ++i)
  {
    if (ZSE_CTXC().subghz.hifreq[i].clusterid == clusterid)
    {
      TRACE_MSG(TRACE_ZDO3, "found match i %d", (FMT__D, i));
      break;
    }
    else if (free_i == -1
             || ZB_TIME_GE(ZSE_CTXC().subghz.hifreq[free_i].timestamp, ZSE_CTXC().subghz.hifreq[i].timestamp))
    {
      free_i = i;
    }
  }
  if (i < ZB_SE_HI_FREQ_MSG_N_SLOTS)
  {
    /* found a slot */
    hifreq = (zb_bool_t)(ZB_TIME_GE(ZSE_CTXC().subghz.hifreq[i].timestamp, t) ||
              ZB_TIME_SUBTRACT(t, ZSE_CTXC().subghz.hifreq[i].timestamp) < ZB_SE_HI_FREQ_MSG_TIMEOUT);
  }
  else
  {
    i = free_i;
    TRACE_MSG(TRACE_ZDO3, "free_i %d", (FMT__D, i));
    hifreq = force;
  }
  ZSE_CTXC().subghz.hifreq[i].timestamp = t + (force ? ZB_SE_HI_FREQ_MSG_TIMEOUT : 0);
  ZSE_CTXC().subghz.hifreq[i].clusterid = clusterid;
  TRACE_MSG(TRACE_ZDO3, "se_get_high_freq_slot cluster %d force %d - i %d new ts %ld hifreq %d",
            (FMT__D_D_D_D_D, clusterid, force, i, ZSE_CTXC().subghz.hifreq[i].timestamp, hifreq));

  return hifreq;
}

zb_bool_t se_is_high_freq_msg(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO3, "se_is_high_freq_msg param %hd page %d", (FMT__H_D, param, ZB_PIBCACHE_CURRENT_PAGE()));
  if (ZB_PIBCACHE_CURRENT_PAGE() != 0)
  {
    zb_bool_t hi_freq;
    zb_apsde_data_req_t * apsde_req = ZB_BUF_GET_PARAM(param, zb_apsde_data_req_t);
    hi_freq = se_get_high_freq_slot(apsde_req->clusterid, ZB_FALSE);
    return hi_freq;
  }
  else
  {
    return ZB_FALSE;
  }
}

void zb_start_high_freq_msgs(zb_uint16_t clusterid)
{
  TRACE_MSG(TRACE_ZDO3, "zb_start_high_freq_msgs 0x%x", (FMT__D, clusterid));
  /* Create slot "in the future" to force high freq mode */
  se_get_high_freq_slot(clusterid, ZB_TRUE);
}

#else

zb_bool_t se_is_high_freq_msg(zb_uint8_t param)
{
  ZVUNUSED(param);
  return ZB_FALSE;
}

void zb_start_high_freq_msgs(zb_uint16_t clusterid)
{
  ZVUNUSED(clusterid);
}

#endif /* !ZB_SUBGHZ_BAND_ENABLED */


#ifdef ZB_JOIN_CLIENT

static void se_handle_dev_annce_sent_signal(zb_bufid_t param)
{
  if ( !ZB_NIB_SECURITY_LEVEL() ||
       (zb_secur_get_link_key_by_address(ZB_AIB().trust_center_address, ZB_SECUR_ANY_KEY_ATTR)
        && ZG->zdo.handle.rejoin))
  {
    zb_app_signal_pack(param, ZB_SIGNAL_DEVICE_REBOOT, zb_buf_get_status(param), 0);
    ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete_int, param);

    se_commissioning_signal(SE_COMM_SIGNAL_REJOIN_RECOVERY_OK, 0);
  }
}


static void se_handle_tclk_signal(zb_bufid_t param)
{
  /* there should be no TCLK events in SE */

  ZB_ASSERT(0);
  if (param != ZB_BUF_INVALID)
  {
    zb_buf_free(param);
  }
}


static void se_handle_leave_with_rejoin_signal(zb_bufid_t param)
{
#ifdef ZB_USE_NVRAM
      /* If we fail, trace is given and assertion is triggered */
      (void)zb_nvram_write_dataset(ZB_NVRAM_COMMON_DATA);
#endif
      zdo_inform_app_leave(ZB_NWK_LEAVE_TYPE_REJOIN);
      se_commissioning_signal(SE_COMM_SIGNAL_ENTER_REJOIN_ONLY, param);
}

#endif /* ZB_JOIN_CLIENT */


static void se_handle_comm_signal(zb_commissioning_signal_t signal, zb_bufid_t param)
{
  TRACE_MSG(TRACE_ZDO1, ">> se_handle_comm_signal, signal %d, param %d",
            (FMT__D_D, signal, param));

  switch(signal)
  {
    case ZB_COMM_SIGNAL_START:
      se_initiate_commissioning(param);
      break;
#ifdef ZB_JOIN_CLIENT
    case ZB_COMM_SIGNAL_NWK_DISC_FAILED:
      se_commissioning_signal(SE_COMM_SIGNAL_NWK_DISCOVERY_FAILED, param);
      break;
    case ZB_COMM_SIGNAL_JOIN_FAILED:
      se_commissioning_signal(SE_COMM_SIGNAL_NWK_JOIN_FAILED, param);
      break;
    case ZB_COMM_SIGNAL_AUTH_FAILED:
      se_commissioning_signal(SE_COMM_SIGNAL_NWK_AUTH_FAILED, param);
      break;
    case ZB_COMM_SIGNAL_INITIATE_REJOIN:
      se_commissioning_signal(SE_COMM_SIGNAL_ENTER_REJOIN_RECOVERY, param);
      break;
    case ZB_COMM_SIGNAL_DEV_ANNCE_SENT:
      se_handle_dev_annce_sent_signal(param);
      break;
    case ZB_COMM_SIGNAL_ROUTER_STARTED:
      se_commissioning_signal(SE_COMM_SIGNAL_NWK_START_ROUTER_CONF, param);
      break;
    case ZB_COMM_SIGNAL_TCLK_UPDATE_COMPLETE:
      se_handle_tclk_signal(param);
      break;
    case ZB_COMM_SIGNAL_TCLK_UPDATE_FAILED:
      se_handle_tclk_signal(param);
      break;
    case ZB_COMM_SIGNAL_LEAVE_DONE:
      se_commissioning_signal(SE_COMM_SIGNAL_NWK_LEAVE, 0);
      break;
    case ZB_COMM_SIGNAL_AUTH_OK:
      se_commissioning_signal(SE_COMM_SIGNAL_NWK_AUTH_OK, 0);
      break;
    case ZB_COMM_SIGNAL_SECUR_FAILED:
      se_commissioning_signal(SE_COMM_SIGNAL_BAD, param);
      break;
    case ZB_COMM_SIGNAL_REJOIN_AFTER_SECUR_FAILED:
      se_commissioning_signal(SE_COMM_SIGNAL_ENTER_REJOIN_RECOVERY, param);
      break;
    case ZB_COMM_SIGNAL_LEAVE_WITH_REJOIN:
      se_handle_leave_with_rejoin_signal(param);
      break;
#endif /* ZB_JOIN_CLIENT */

#ifdef ZB_COORDINATOR_ROLE
    case ZB_COMM_SIGNAL_SECURED_REJOIN:
      se_commissioning_signal(SE_COMM_SIGNAL_CHILD_SECURED_REJOIN, param);
      break;
    case ZB_COMM_SIGNAL_TCLK_VERIFIED_REMOTE:
      se_commissioning_signal(SE_COMM_SIGNAL_JOINED_DEVICE_NON_CBKE_OK, param);
      break;
    case ZB_COMM_SIGNAL_DEVICE_LEFT:
      bdb_cancel_link_key_refresh_alarm(se_link_key_refresh_alarm, param);
      break;
#endif /* ZB_COORDINATOR_ROLE */

#ifdef ZB_FORMATION
    case ZB_COMM_SIGNAL_FORMATION_DONE:
      se_commissioning_signal(SE_COMM_SIGNAL_NWK_FORMATION_OK, param);
      break;
    case ZB_COMM_SIGNAL_FORMATION_FAILED:
      se_commissioning_signal(SE_COMM_SIGNAL_NWK_FORMATION_FAILED, param);
      break;
    case ZB_COMM_SIGNAL_AUTHENTICATE_REMOTE:
      se_commissioning_signal(SE_COMM_SIGNAL_NWK_AUTHENTICATE_REMOTE, param);
      break;

#endif /* ZB_FORMATION */

    default:
      TRACE_MSG(TRACE_ERROR, "unknown commissioning signal: %d", (FMT__D, signal));
      ZB_ASSERT(0);
      if (param != ZB_BUF_INVALID)
      {
        zb_buf_free(param);
      }
  }

  TRACE_MSG(TRACE_ZDO1, "<< se_handle_comm_signal", (FMT__0));
}


#ifdef ZB_ROUTER_ROLE

static zb_uint8_t se_get_permit_join_duration(void)
{
  /* No permit join after start in SE */
  return 0;
}

#endif /* ZB_ROUTER_ROLE */


static zb_bool_t se_must_use_installcode(zb_bool_t is_client)
{
  /*
    See 5.4.1 Joining with Preinstalled Trust Center Link Keys
    Trust Center link keys SHALL be installed in each device prior to joining the utility network


    a new device could be installed by the home owner or installer 2372 communicating the device install code out of band to the coordinator/trust center

    a new device could be installed by the home owner or installer 2372 communicating the device install code out of band to the coordinator/trust center
  */
#ifndef ZB_SE_BDB_MIXED
  return ZB_TRUE;
#else
  /* BDB/SE mixed mode: SE does not allow to join w/o installcode, BDB usually allows.
     If we are here:
     - we are joining device - switch from SE to BDB mode
     - we are parent device - mark somehow that child is not SE (it is not allowed to discover
     Key Establishment etc)
  */
  if (is_client && !(zb_bool_t)ZB_TCPOL().require_installcodes
      && ZSE_CTXC().commissioning.allow_bdb_in_se_mode)
  {
    se_switch_to_bdb_commissioning(ZB_BDB_NETWORK_STEERING);
  }
  return (zb_bool_t)ZB_TCPOL().require_installcodes;
#endif
}


static void se_send_app_bind_signal(zb_uint8_t param, zb_uint16_t idx)
{
  zb_se_signal_service_discovery_bind_params_t *bind_params;

  if(ZG->aps.binding.dst_table[idx].dst_addr_mode == ZB_APS_BIND_DST_ADDR_LONG)
  {
    bind_params = (zb_se_signal_service_discovery_bind_params_t *)zb_app_signal_pack(param,
      ZB_SE_SIGNAL_SERVICE_DISCOVERY_BIND_INDICATION,
      zb_buf_get_status(param),
      sizeof(zb_se_signal_service_discovery_bind_params_t));
    zb_address_ieee_by_ref(bind_params->device_addr, ZG->aps.binding.dst_table[idx].u.long_addr.dst_addr);
    bind_params->endpoint = ZG->aps.binding.dst_table[idx].u.long_addr.dst_end;
    bind_params->cluster_id = ZG->aps.binding.src_table[ZG->aps.binding.dst_table[idx].src_table_index].cluster_id;
    bind_params->commodity_type = 0;
    TRACE_MSG(TRACE_APP3, "zb_send_app_bind_signal device " TRACE_FORMAT_64 " cluster:0x%04x, endpoint %d", (FMT__A_D_D,
            TRACE_ARG_64(bind_params->device_addr), bind_params->cluster_id, bind_params->endpoint));
    //ZB_ASSERT(zb_address_ieee_by_short(dev->short_addr, bind_params->device_addr) == RET_OK);
    ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete, param);
    TRACE_MSG(TRACE_APP3, "bind device ok, dev 0x%x - inform user", (FMT__D, zb_address_short_by_ieee(bind_params->device_addr)));
  }
}


void se_new_binding_handler(zb_uint16_t bind_tbl_idx)
{
  zb_buf_get_out_delayed_ext(se_send_app_bind_signal, bind_tbl_idx, 0);
}


static zb_bool_t se_handle_match_desc_resp(zb_bufid_t param)
{
  zb_bool_t skip_free_buf = ZB_FALSE;

#if defined(ZB_SE_ENABLE_SERVICE_DISCOVERY_PROCESSING)
  skip_free_buf = zb_se_service_discovery_match_desc_resp_handle(param);
#endif

#if defined(ZB_SE_ENABLE_STEADY_STATE_PROCESSING)
  if (ZB_SE_MODE() && !skip_free_buf)
  {
    skip_free_buf = zb_se_steady_state_match_desc_resp_handle(param);
  }
#endif

  return skip_free_buf;
}


static zb_bool_t se_block_zcl_cmd(zb_zcl_parsed_hdr_t *cmd_info)
{
  zb_bool_t ret = ZB_FALSE;

#if defined(ZB_SE_ENABLE_SERVICE_DISCOVERY_PROCESSING)
      /* For some SE Service Discovery steps we need to handle ZCL commands on stack layer.
         Do not pass ZCL command to application device_handler if command is sent from Service
         Discovery (check by combination short addr + endpoint + ZCL sequence number).
       */
  ret = ret || zb_se_service_discovery_block_zcl_cmd(
    ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr,
    ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint,
    cmd_info->seq_number);
#endif /* ZB_SE_ENABLE_SERVICE_DISCOVERY_PROCESSING */

#if defined(ZB_SE_ENABLE_STEADY_STATE_PROCESSING)
  ret = ret || zb_se_steady_state_block_zcl_cmd(
    ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr,
    ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint,
    cmd_info->seq_number);
#endif /* ZB_SE_ENABLE_STEADY_STATE_PROCESSING */

  return ret;
}


static zb_bool_t se_handle_read_attr_resp(zb_bufid_t param)
{
  zb_bool_t processed = ZB_FALSE;
  zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);

#if defined(ZB_SE_ENABLE_SERVICE_DISCOVERY_PROCESSING)
  if (zb_se_service_discovery_block_zcl_cmd(
        ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr,
        ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint,
        cmd_info->seq_number))
  {
    if ((cmd_info->cluster_id == ZB_ZCL_CLUSTER_ID_METERING ||
         cmd_info->cluster_id == ZB_ZCL_CLUSTER_ID_PRICE))
    {
      processed = zb_se_service_discovery_read_commodity_attr_handle(param);
    }
#ifdef ZB_ENABLE_TIME_SYNC
    else if ((ZB_ZCL_CLUSTER_ID_TIME == cmd_info->cluster_id))
    {
      processed = zb_se_service_discovery_read_time_attrs_handle(param);
    }
#endif
  }
#endif

#if defined(ZB_SE_ENABLE_STEADY_STATE_PROCESSING)
  if (!processed && zb_se_steady_state_block_zcl_cmd(
        ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr,
        ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint,
        cmd_info->seq_number))
  {
    if ((cmd_info->cluster_id == ZB_ZCL_CLUSTER_ID_METERING ||
         cmd_info->cluster_id == ZB_ZCL_CLUSTER_ID_PRICE ||
         cmd_info->cluster_id == ZB_ZCL_CLUSTER_ID_KEEP_ALIVE))
    {
      processed = zb_se_steady_state_read_attr_handle(param);
    }
  }
#endif

  return processed;
}


void se_commissioning_force_link(void)
{
  zb_zse_init();

  COMM_SELECTOR().signal = se_handle_comm_signal;

#ifdef ZB_ROUTER_ROLE
  COMM_SELECTOR().get_permit_join_duration = se_get_permit_join_duration;
#endif /* ZB_ROUTER_ROLE */

  COMM_SELECTOR().must_use_install_code = se_must_use_installcode;

#ifdef ZB_JOIN_CLIENT
  COMM_SELECTOR().get_scan_channel_mask = se_commissioning_scan_channels_mask;
#endif /* ZB_JOIN_CLIENT */

  APS_SELECTOR().new_binding_handler = se_new_binding_handler;

  ZDO_SELECTOR().match_desc_resp_handler = se_handle_match_desc_resp;
  ZDO_SELECTOR().app_link_key_ind_handler = se_handle_link_key_indication;

  ZCL_SELECTOR().is_high_freq_msg = se_is_high_freq_msg;
  ZCL_SELECTOR().block_zcl_cmd = se_block_zcl_cmd;
  ZCL_SELECTOR().read_attr_resp_handler= se_handle_read_attr_resp;
}


#ifdef ZB_ROUTER_ROLE

void zb_se_set_network_router_role(zb_uint32_t channel_mask)
{
  se_commissioning_force_link();
  zb_set_network_router_role_with_mode(channel_mask, ZB_COMMISSIONING_SE);
}

#endif /* ZB_ROUTER_ROLE */


#ifdef ZB_ED_FUNC

void zb_se_set_network_ed_role(zb_uint32_t channel_mask)
{
  se_commissioning_force_link();
  zb_set_network_ed_role_with_mode(channel_mask, ZB_COMMISSIONING_SE);
}

#endif /* ZB_ED_FUNC */


#endif  /* #ifdef ZB_SE_COMMISSIONING */
