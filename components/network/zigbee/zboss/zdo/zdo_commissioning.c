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
/*  PURPOSE: Generic ZDO commissioning routines.
 Partially moved from zdo_app.c.
*/

#define ZB_TRACE_FILE_ID 65
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
#include "zb_bdb_internal.h"
#include "zdo_wwah_stubs.h"
#include "zb_address_internal.h"
#include "zb_commissioning.h"

#if defined ZB_ENABLE_ZLL
#include "zll/zb_zll_common.h"
#include "zll/zb_zll_nwk_features.h"
#endif /* defined ZB_ENABLE_ZLL */

#if defined ZB_HA_SUPPORT_EZ_MODE
#include "ha/zb_ha_ez_mode_comissioning.h"
#endif

#ifdef ZB_SE_COMMISSIONING
#include "zb_se.h"
#endif
#ifdef NCP_MODE
#include "zb_ncp.h"
#endif

#ifdef ZB_COMMISSIONING_CLASSIC_SUPPORT
void zdo_restart_association(zb_uint8_t param);
#endif

#ifndef NCP_MODE_HOST
#ifdef ZB_JOIN_CLIENT

zb_bool_t zb_zdo_is_rejoin_active(void)
{
  return ZG->zdo.handle.rejoin;
}

#endif /* ZB_JOIN_CLIENT */
#endif /* NCP_MODE_HOST */

/*
In case of NCP this file is to be compiled for both SoC side, with
only NCP_MODE defined, and Host side with defined both NCP_MODE and NCP_MODE_HOST,
ZB_BDB_MODE and/or ZB_SE_COMMISSIONING.
 */

void zdo_commissioning_init(void)
{
  COMM_CTX().discovery_ctx.nwk_scan_attempts = ZB_ZDO_NWK_SCAN_ATTEMPTS;
  TRACE_MSG(TRACE_ZDO3, "nwk_scan_attempts %hd", (FMT__H, COMM_CTX().discovery_ctx.nwk_scan_attempts));
  COMM_CTX().discovery_ctx.nwk_time_btwn_scans = ZB_ZDO_NWK_TIME_BTWN_SCANS;

#if defined ZB_ROUTER_ROLE && !defined ZB_COORDINATOR_ONLY && !defined NCP_MODE_HOST
  ZG->nwk.selector.no_active_links_left_cb = zb_send_no_active_links_left_signal;
#endif /* ZB_ROUTER_ROLE && !ZB_COORDINATOR_ONLY && !NCP_MODE_HOST */
}

/**
   Start specific top-level commissioning.

   Called from zdo_dev_continue_start_after_nwk() (main call) and se_initiate_commissioning() (some trick of falling to BDB from SE for mixed SE/BDB build).
 */
void zdo_commissioning_start(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO1, "> zdo_commissioning_start param %hd comm tyoe %d", (FMT__H_D, param, ZB_COMMISSIONING_TYPE()));

  ZB_ASSERT(COMM_SELECTOR().signal != NULL);

  COMM_SELECTOR().signal(ZB_COMM_SIGNAL_START, param);
}

#ifdef ZB_JOIN_CLIENT
void zdo_commissioning_join_via_scanlist(zb_uint8_t param)
{
  zb_ret_t ret;
  zb_uint_t i = 0;
  zb_nlme_network_discovery_confirm_t *cnf = NULL;
  zb_nlme_network_descriptor_t *dsc;
  zb_ext_pan_id_t extended_pan_id;
  zb_ext_pan_id_t use_ext_pan_id;

  ZVUNUSED(param);
  if (COMM_CTX().discovery_ctx.scanlist_ref != 0U)
  {
    cnf = (zb_nlme_network_discovery_confirm_t *)zb_buf_begin(COMM_CTX().discovery_ctx.scanlist_ref);
    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,72} */
    dsc = (zb_nlme_network_descriptor_t *)(cnf+1);
#if TRACE_ENABLED(TRACE_ZDO1)
    for (i = 0 ; i < cnf->network_count ; ++i)
    {
      zb_ext_pan_id_t extended_pan_id;

      zb_address_get_pan_id(dsc->panid_ref, extended_pan_id);

      TRACE_MSG(TRACE_ZDO1,
                "net %hd: xpanid " TRACE_FORMAT_64 ", channel %hd, stack profile %hd, permit_joining %hd, router_cap %hd, ed_cap %hd",
                (FMT__H_A_H_H_H_H_H,
                  i, TRACE_ARG_64(extended_pan_id),
                  (zb_uint8_t)dsc->logical_channel, (zb_uint8_t)dsc->stack_profile, (zb_uint8_t)dsc->permit_joining,
                  (zb_uint8_t)dsc->router_capacity, (zb_uint8_t)dsc->end_device_capacity));
      dsc++;
    }

    /* Be polite - restore dsc pointer */
    dsc = (zb_nlme_network_descriptor_t *)(cnf + 1);
#endif /* TRACE_ENABLED(TRACE_ZDO1) */

    /* Now join thru Association */
    for (i = 0 ; i < cnf->network_count ; ++i)
    {
      if (!ZB_U2B(dsc->processed))
      {
        if (COMM_CTX().discovery_ctx.scanlist_join_attempt_n < ZB_ZDO_MAX_JOINING_TRY_ATTEMPTS)
        {
          zb_get_use_extended_pan_id(use_ext_pan_id);
          zb_address_get_pan_id(dsc->panid_ref, extended_pan_id);

          if (
            (ZB_EXTPANID_IS_ZERO(use_ext_pan_id)
             || ZB_EXTPANID_CMP(extended_pan_id, use_ext_pan_id)))
          {
            /*
              Now join to the first network or network with desired ext pan id.
            */
#ifdef ZB_NWK_BLACKLIST
            if (zb_nwk_blacklist_is_full())
            {
              zb_nwk_blacklist_reset();
            }
            /* Add the network into the blacklist only once at the first
             * joining attempt */
            if (COMM_CTX().discovery_ctx.scanlist_join_attempt_n == 0U)
            {
              zb_nwk_blacklist_add(extended_pan_id);
            }
#endif
            COMM_CTX().discovery_ctx.scanlist_idx = i;
            COMM_CTX().discovery_ctx.scanlist_join_attempt_n++;
            TRACE_MSG(TRACE_ZDO3, "scanlist_idx %d scanlist_join_attempt_n %hd",
                      (FMT__D_H, COMM_CTX().discovery_ctx.scanlist_idx, COMM_CTX().discovery_ctx.scanlist_join_attempt_n));
            ret = zb_buf_get_out_delayed(zdo_join_to_nwk_descr);
            if (ret != RET_OK)
            {
              TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
              ZB_ASSERT(0);
            }
            break;
          }
        }
        else
        {
          COMM_CTX().discovery_ctx.scanlist_join_attempt_n = 0;
          dsc->processed = 1;
        }
      }
      dsc++;
    } /* for */
  }
  if (!ZB_U2B(COMM_CTX().discovery_ctx.scanlist_ref)
      || ((cnf != NULL) && (i != 0U) && (i == cnf->network_count)))
  {
    ZB_SCHEDULE_CALLBACK(zdo_commissioning_nwk_discovery_failed, 0);
  }
}


/*
  We are here when we could not find a net to join
 */
void zdo_commissioning_nwk_discovery_failed(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ERROR, "Can't find PAN to join to! param %hd", (FMT__H, param));

  /* Use scanlist ref for our internal purposes */
  if (param == 0U)
  {
    param = COMM_CTX().discovery_ctx.scanlist_ref;
  }
  if (param == 0U)
  {
    zb_ret_t ret = zb_buf_get_out_delayed(zdo_commissioning_nwk_discovery_failed);
    if (ret != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
    }
    return;
  }
  zdo_reset_scanlist(ZB_FALSE);
#ifdef ZB_NWK_BLACKLIST
  zb_nwk_blacklist_reset();
#endif

  ZB_ASSERT(COMM_SELECTOR().signal != NULL);

  COMM_SELECTOR().signal(ZB_COMM_SIGNAL_NWK_DISC_FAILED, param);
}

/*
  We are here when join/rejoin attempt is failed.
 */
void zdo_commissioning_join_failed(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO1, "{re}join failed", (FMT__0));

  ZB_ASSERT(COMM_SELECTOR().signal != NULL);

  COMM_SELECTOR().signal(ZB_COMM_SIGNAL_JOIN_FAILED, param);
}


void zdo_commissioning_authentication_failed(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO1, "zdo_commissioning_authentication_failed, param %d", (FMT__D, param));

  ZB_ASSERT(COMM_SELECTOR().signal != NULL);

  COMM_SELECTOR().signal(ZB_COMM_SIGNAL_AUTH_FAILED, param);
}

#endif  /* ZB_JOIN_CLIENT */


zb_bool_t zb_distributed_security_enabled()
{
#ifdef ZB_SE_COMMISSIONING
  return (zb_bool_t)(ZB_COMMISSIONING_TYPE() != ZB_COMMISSIONING_SE);
#else
  return ZB_TRUE;
#endif
}


#ifdef ZB_JOIN_CLIENT
#if defined ZB_DISTRIBUTED_SECURITY_ON

zb_bool_t zb_tc_is_distributed(void)
{
  return zb_aib_trust_center_address_unknown();
}

void zb_sync_distributed(void)
{
  zb_aib_tcpol_set_is_distributed_security(zb_tc_is_distributed());
}

#endif  /* ZB_DISTRIBUTED_SECURITY_ON */


void zdo_commissioning_initiate_rejoin(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO1, "zdo_commissioning_initiate_rejoin param %hd", (FMT__H, param));

  ZB_ASSERT(COMM_SELECTOR().signal != NULL);

  COMM_SELECTOR().signal(ZB_COMM_SIGNAL_INITIATE_REJOIN, param);
}


void zdo_commissioning_leave(zb_bufid_t buf, zb_bool_t rejoin, zb_bool_t remove_children)
{
  zb_zdo_mgmt_leave_param_t *req = ZB_BUF_GET_PARAM(buf, zb_zdo_mgmt_leave_param_t);

  TRACE_MSG(TRACE_ZDO2, ">> zdo_commissioning_leave, buf %d, rejoin %d, remove_children %d",
            (FMT__D_D, buf, rejoin, remove_children));

  zb_get_long_address(req->device_address);

  req->dst_addr = zb_get_short_address();
  req->rejoin = ZB_B2U(rejoin);
  req->remove_children = ZB_B2U(remove_children);

  (void)zdo_mgmt_leave_req(buf, NULL);

  TRACE_MSG(TRACE_ZDO2, "<< zdo_commissioning_leave", (FMT__0));
}


void zdo_commissioning_handle_dev_annce_sent_event(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO2, ">> zdo_commissioning_handle_dev_annce_sent_event, param %d",
            (FMT__D, param));

  ZB_ASSERT(COMM_SELECTOR().signal != NULL);

  COMM_SELECTOR().signal(ZB_COMM_SIGNAL_DEV_ANNCE_SENT, param);

  TRACE_MSG(TRACE_ZDO2, "<< zdo_commissioning_handle_dev_annce_sent_event", (FMT__0));
}

void zdo_commissioning_start_router_confirm(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO1, ">>zdo_commissioning_start_router_confirm %hd", (FMT__H, param));

  ZB_ASSERT(COMM_SELECTOR().signal != NULL);

  COMM_SELECTOR().signal(ZB_COMM_SIGNAL_ROUTER_STARTED, param);
}


void zdo_commissioning_tclk_upd_complete(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO3, ">> zdo_commissioning_tclk_upd_complete, param %d", (FMT__D, param));

  ZB_ASSERT(COMM_SELECTOR().signal != NULL);

  COMM_SELECTOR().signal(ZB_COMM_SIGNAL_TCLK_UPDATE_COMPLETE, param);

  TRACE_MSG(TRACE_ZDO3, "<< zdo_commissioning_tclk_upd_complete", (FMT__0));
}


void zdo_commissioning_tclk_upd_failed(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO3, ">> zdo_commissioning_tclk_upd_failed, param %d", (FMT__D, param));

  ZB_ASSERT(COMM_SELECTOR().signal != NULL);

  COMM_SELECTOR().signal(ZB_COMM_SIGNAL_TCLK_UPDATE_FAILED, param);

  TRACE_MSG(TRACE_ZDO3, "<< zdo_commissioning_tclk_upd_failed", (FMT__0));
}


void zdo_commissioning_leave_done(zb_uint8_t param)
{
  TRACE_MSG(TRACE_NWK1, ">>zb_zdo_clear_after_leave_cont", (FMT__0));

#if defined ZB_ENABLE_ZLL
  if (    ZLL_TRAN_CTX().transaction_id
          &&  (  ZLL_TRAN_CTX().transaction_task == ZB_ZLL_TRANSACTION_NWK_START_TASK_TGT
                 || ZLL_TRAN_CTX().transaction_task == ZB_ZLL_TRANSACTION_NWK_START_TASK
                 || ZLL_TRAN_CTX().transaction_task == ZB_ZLL_TRANSACTION_JOIN_ROUTER_TASK
                 || ZLL_TRAN_CTX().transaction_task == ZB_ZLL_TRANSACTION_JOIN_ROUTER_TASK_TGT
                 || ZLL_TRAN_CTX().transaction_task == ZB_ZLL_TRANSACTION_JOIN_ED_TASK
                 || ZLL_TRAN_CTX().transaction_task == ZB_ZLL_TRANSACTION_JOIN_ED_TASK_TGT))
  {
    ZG->nwk.leave_context.rejoin_after_leave = ZB_FALSE;
    zb_buf_set_status(param, RET_OK);
    ZB_SCHEDULE_CALLBACK(zll_leave_nwk_confirm, param);
  }
  else
#endif /* defined ZB_ENABLE_ZLL */
  {
    /* Not need anymore */
    zb_buf_free(param);
#ifdef ZB_USE_NVRAM
    zb_nvram_clear();
#endif
    /* Clearing addr map table */
    zb_address_reset();

    ZB_ASSERT(COMM_SELECTOR().signal != NULL);

    COMM_SELECTOR().signal(ZB_COMM_SIGNAL_LEAVE_DONE, ZB_BUF_INVALID);
  }
#if !defined(ZB_LITE_NO_OLD_CB) && !defined(NCP_MODE_HOST)
  /* FIXME: remove leave signel here?? Note: commented out in r21. */
  if (ZDO_CTX().app_leave_cb != NULL)
  {
    (ZDO_CTX().app_leave_cb)(ZB_NWK_LEAVE_TYPE_RESET);
  }
  else
#endif
  {
    zb_ret_t ret = zb_buf_get_out_delayed_ext(zb_send_leave_signal, ZB_NWK_LEAVE_TYPE_RESET, 0);
    if (ret != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed_ext [%d]", (FMT__D, ret));
    }
  }

  TRACE_MSG(TRACE_NWK1, "<<zb_zdo_clear_after_leave_cont", (FMT__0));
}

#endif  /* ZB_JOIN_CLIENT */


#ifndef NCP_MODE_HOST

#ifdef ZB_JOIN_CLIENT

static void zdo_commissioning_authenticated_cont(void)
{
  ZB_ASSERT(COMM_SELECTOR().signal != NULL);

  COMM_SELECTOR().signal(ZB_COMM_SIGNAL_AUTH_OK, ZB_BUF_INVALID);
}


void zdo_send_signal_no_args(zb_uint8_t param, zb_uint16_t signal)
{
  /* user_param contains status. Modify if needed to pass signal etc. */
  (void)zb_app_signal_pack(param, signal, 0, 0);
  ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete_int, param);
}


/**
   Reaction on authentication complete - that is, on NWK key recv.

   Next actions depend on mode: BDB/classic/SE/ZLL
 */
void zdo_commissioning_authenticated(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO3, "scheduling device_annce %hd", (FMT__H, param));

  /* According to BDB send Devive_annce immediately after receiving NWK key. */
  /* Call Device_annce before node desc req (according to BDB Figure 13 Trust Center link key exchange procedure sequence chart) */
  if (param != 0U)
  {
    ZB_SCHEDULE_CALLBACK(zdo_authenticated_send_device_annce, param);
  }

#if defined TC_SWAPOUT && !defined ZB_COORDINATOR_ONLY
  if (ZB_TCPOL().tc_swapped)
  {
    zb_ret_t ret = zb_buf_get_out_delayed_ext(zdo_send_signal_no_args, ZB_TC_SWAPPED_SIGNAL, 0);
    ZB_ASSERT(ret == RET_OK);
    ZB_TCPOL().tc_swapped = ZB_FALSE;
  }
#endif
  zdo_commissioning_authenticated_cont();
}


/*
  Commissioning logic after device started
 */
void zdo_commissioning_dev_annce_sent(zb_uint8_t param)
{
  zb_ret_t status = zb_buf_get_status(param);

  (void)zb_buf_reuse(param);
  /* Indicate startup complete */
  zb_buf_set_status(param, status);

  TRACE_MSG(TRACE_ZDO1, ">> zdo_commissioning_dev_annce_sent param %hd",
            (FMT__H, param));

  TRACE_MSG(TRACE_ZDO3, "sent device_annce, start compl, st %hd  dev type %hd",
            (FMT__H_H, status, zb_get_device_type()));

  /* moved from apsde_data_conf */
#ifdef ZB_USE_NVRAM
  /* If we fail, trace is given and assertion is triggered */
  (void)zb_nvram_write_dataset(ZB_NVRAM_COMMON_DATA);
#endif

  if (status != RET_OK)
  {
    /* rejoin to current PAN disregarding current commissioning type */
    zb_uint8_t *rejoin_reason = ZB_BUF_GET_PARAM(param, zb_uint8_t);
    *rejoin_reason = ZB_REJOIN_REASON_DEV_ANNCE_SENDING_FAILED;
    TRACE_MSG(TRACE_INFO1, "device annce send fail", (FMT__0));
    
    /* disable rejoin active flag & change commission status for able to start rejoin again */
    ZG->zdo.handle.rejoin = ZB_FALSE;
    ZB_BDB().bdb_commissioning_status = ZB_BDB_STATUS_SUCCESS;
    
    ZB_SCHEDULE_CALLBACK(zdo_commissioning_initiate_rejoin, param);
  }
  else
  {
#if defined ZB_ROUTER_ROLE
    /* FIXME: handle start router in bdb? */
    if (ZB_IS_DEVICE_ZR())
    {
      ZB_SCHEDULE_CALLBACK(zb_zdo_start_router, param);
    }
    else
#endif /* ZB_ROUTER_ROLE */
    {
      zdo_commissioning_handle_dev_annce_sent_event(param);
    }

    /* Regardless commissioning mode - it's common for all: schedule end device
     * timeout request */

#ifdef ZB_ED_FUNC
#ifndef ZB_LITE_NO_ED_AGING_REQ
    if (ZB_IS_DEVICE_ZED()
#ifdef ZB_CERTIFICATION_HACKS
    /* Used for test purposes in tp_r21_bv-19 for gZED */
        && (!ZB_CERT_HACKS().disable_end_device_timeout_request_at_join)
#endif
      )
    {
      /* send ED timeout request cmd with delay to make sure
         the coordinator have handled previously sent Dev_annce cmd
      */
      /* We do not know our ED timeout mode until resp to ED Timeout Req from ZC/ZR  */
      ZB_SET_KEEPALIVE_MODE(ED_KEEPALIVE_DISABLED);
      ZB_SCHEDULE_ALARM(zb_nwk_ed_send_timeout_req, 0, ZB_ZDO_SEND_ED_TIMEOUT_REQ_DELAY);
    }
#endif
#endif  /* #ifdef ZB_ED_FUNC */
  }
}

#endif /* ZB_JOIN_CLIENT */

#ifdef ZB_ROUTER_ROLE

#ifndef ZB_COORDINATOR_ONLY
void zdo_comm_set_permit_join_after_router_start(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO1, ">> zdo_commissioning_start_router_confirm %hd", (FMT__H, param));

  zdo_comm_set_permit_join(param, zdo_commissioning_start_router_confirm);

  TRACE_MSG(TRACE_ZDO1, "<< zdo_commissioning_start_router_confirm", (FMT__0));
}
#endif /* #ifndef ZB_COORDINATOR_ONLY */

void zdo_comm_set_permit_join(zb_uint8_t param, zb_callback_t cb)
{
  zb_zdo_mgmt_permit_joining_req_param_t *request;

  request = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_permit_joining_req_param_t);
  request->dest_addr = ZB_PIBCACHE_NETWORK_ADDRESS();
  request->permit_duration = zb_commissioning_default_permit_duration();

  TRACE_MSG(TRACE_ZDO2, "set permit_duration %d", (FMT__D, request->permit_duration));
  /* Go via zdo mgmt to use cb */
  (void)zb_zdo_mgmt_permit_joining_req(param, cb);
}


zb_uint8_t zb_commissioning_default_permit_duration(void)
{
  ZB_ASSERT(COMM_SELECTOR().get_permit_join_duration != NULL);
  return COMM_SELECTOR().get_permit_join_duration();
}

#endif  /* #ifdef ZB_ROUTER_ROLE */


#ifdef ZB_JOIN_CLIENT

void zdo_commissioning_secur_failed(zb_uint8_t param)
{
  ZB_ASSERT(COMM_SELECTOR().signal != NULL);

  COMM_SELECTOR().signal(ZB_COMM_SIGNAL_SECUR_FAILED, param);
}


void zb_secur_rejoin_after_security_failure(zb_uint8_t param)
{
  TRACE_MSG(TRACE_SECUR1, "zb_secur_rejoin_after_security_failure: param %hd;", (FMT__H, param));

  ZB_ASSERT(COMM_SELECTOR().signal != NULL);
  COMM_SELECTOR().signal(ZB_COMM_SIGNAL_REJOIN_AFTER_SECUR_FAILED, param);
}

#endif /* ZB_JOIN_CLIENT */

#if defined ZB_SECURITY_INSTALLCODES && !defined ZB_SECURITY_INSTALLCODES_ONLY
zb_bool_t zdo_secur_must_use_installcode(zb_bool_t is_client)
{
  ZB_ASSERT(COMM_SELECTOR().must_use_install_code != NULL);

  return COMM_SELECTOR().must_use_install_code(is_client);
}
#endif


#if defined ZB_COORDINATOR_ROLE
/**
   After successful secure rejoin in some scenarios need to wait for unique TCLK establishment.

 */
void zdo_commissioning_secure_rejoin_setup_lk_alarm(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO1, "zdo_commissioning_secure_rejoin_setup_lk_alarm, param %d", (FMT__D, param));

  ZB_ASSERT(COMM_SELECTOR().signal != NULL);

  COMM_SELECTOR().signal(ZB_COMM_SIGNAL_SECURED_REJOIN, param);
}


void zdo_commissioning_tclk_verified_remote(zb_address_ieee_ref_t param)
{
  TRACE_MSG(TRACE_ZDO1, "zdo_commissioning_tclk_verified_remote, param %d", (FMT__D, param));

  ZB_ASSERT(COMM_SELECTOR().signal != NULL);

  COMM_SELECTOR().signal(ZB_COMM_SIGNAL_TCLK_VERIFIED_REMOTE, param);
}


void zdo_commissioning_device_left(zb_address_ieee_ref_t param)
{
  TRACE_MSG(TRACE_ZDO1, "zdo_commissioning_device_left, param %d", (FMT__D, param));

  ZB_ASSERT(COMM_SELECTOR().signal != NULL);

  COMM_SELECTOR().signal(ZB_COMM_SIGNAL_DEVICE_LEFT, param);
}

#endif  /* ZB_COORDINATOR_ROLE */

#endif /* !defined NCP_MODE_HOST */

#ifdef ZB_JOIN_CLIENT


void zdo_inform_app_leave(zb_uint8_t leave_type)
{
  /* Notify user about leave with rejoin */
#if !defined(ZB_LITE_NO_OLD_CB) && !defined(NCP_MODE_HOST)
  if (ZDO_CTX().app_leave_cb != NULL)
  {
    (ZDO_CTX().app_leave_cb)(leave_type);
  }
  else
#endif  /* !ZB_LITE_NO_OLD_CB && !NCP_MODE_HOST */
  {
    zb_ret_t ret = zb_buf_get_out_delayed_ext(zb_send_leave_signal, leave_type, 0);
    if (ret != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
    }
  }
}


/*
  Commissioning logic upon reception of Leave Request with rejoin = 1
 */
void zdo_commissioning_leave_with_rejoin(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO1, ">> zdo_commissioning_leave_with_rejoin param %hd",
            (FMT__H, param));

  ZB_ASSERT(COMM_SELECTOR().signal != NULL);

  COMM_SELECTOR().signal(ZB_COMM_SIGNAL_LEAVE_WITH_REJOIN, param);

  TRACE_MSG(TRACE_ZDO1, "<< zdo_commissioning_leave_with_rejoin",
            (FMT__0));
}

#endif /* ZB_JOIN_CLIENT */

#ifdef ZB_FORMATION

void zdo_commissioning_authenticate_remote(zb_bufid_t param)
{
  TRACE_MSG(TRACE_ZDO1, ">> zdo_commissioning_authenticate_remote, param %d", (FMT__D, param));

  ZB_ASSERT(COMM_SELECTOR().signal != NULL);

  COMM_SELECTOR().signal(ZB_COMM_SIGNAL_AUTHENTICATE_REMOTE, param);

  TRACE_MSG(TRACE_ZDO1, "<< zdo_commissioning_authenticate_remote", (FMT__0));
}

#endif /* ZB_FORMATION */
