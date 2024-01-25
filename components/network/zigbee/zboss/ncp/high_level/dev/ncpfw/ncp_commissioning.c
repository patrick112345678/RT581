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
/* PURPOSE: NCP FW application commissioning logic
*/

#define ZB_TRACE_FILE_ID 14004

#include "zb_common.h"
#include "zb_ncp.h"


static void ncp_comm_handle_start_signal(zb_bufid_t param)
{
  /* NCP is a special case: after NWL reset and PIB load at SoC do nothing. Let Host define. */
  (void)zb_app_signal_pack(param, ZB_SIGNAL_NWK_INIT_DONE, RET_OK, 0);
  ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete_int, param);
}


#ifdef ZB_JOIN_CLIENT

static void ncp_comm_handle_nwk_disc_failed_signal(zb_bufid_t param)
{
  /* We should not be here in case of NCP FW,
     NCP_CATCH_NWK_DISC_CFM should intercept a call to zdo_handle_nlme_network_discovery_confirm */
  ZB_ASSERT(0);

  zb_buf_free(param);
}


static void ncp_comm_handle_join_failed_signal(zb_bufid_t param)
{
  zb_nlme_join_confirm_t *confirm = ZB_BUF_GET_PARAM(param, zb_nlme_join_confirm_t);
  zb_buf_set_status(param, confirm->status);

  if (ZG->zdo.handle.rejoin)
  {
    ncp_signal(NCP_SIGNAL_NWK_REJOIN_FAILED, param);
  }
  else
  {
    ncp_signal(NCP_SIGNAL_NWK_JOIN_FAILED, param);
  }
}


static void ncp_comm_handle_initiate_rejoin_signal(zb_bufid_t param)
{
  zb_uint8_t *p = zb_buf_get_tail(param, sizeof(zb_uint8_t));
  zb_uint8_t rejoin_reason = *p;

  switch(rejoin_reason)
  {
    case ZB_REJOIN_REASON_PARENT_LOST:
      ncp_signal(NCP_SIGNAL_NLME_PARENT_LOST, param);
      break;
    case ZB_REJOIN_REASON_DEV_ANNCE_SENDING_FAILED:
      if (ZG->zdo.handle.rejoin != 0)
      {
        ncp_signal(NCP_SIGNAL_NWK_REJOIN_FAILED, param);
      }
      else
      {
        ncp_signal(NCP_SIGNAL_NWK_JOIN_FAILED, param);
      }
      break;
    default:
      zb_buf_free(param);
      break;
  }
}


static void ncp_comm_handle_dev_annce_sent_signal(zb_bufid_t param)
{
  if (ZG->zdo.handle.rejoin)
  {
    ncp_signal(NCP_SIGNAL_NWK_REJOIN_DONE, param);
  }
  else
  {
    ncp_signal(NCP_SIGNAL_NWK_JOIN_DONE, param);
  }
}


static void ncp_comm_handle_router_started_signal(zb_bufid_t param)
{
  if (ZG->zdo.handle.rejoin)
  {
    ncp_signal(NCP_SIGNAL_NWK_REJOIN_DONE, param);
  }
  else
  {
    ncp_signal(NCP_SIGNAL_NWK_JOIN_DONE, param);
  }
}


static void ncp_comm_handle_leave_done_signal(zb_bufid_t param)
{
  /* we shouldn't do anything here. This signal is handled at the app layer.
     See ncp_hl_nwk_leave_itself()
     Maybe move that stuff here? */
  if (param != ZB_BUF_INVALID)
  {
    zb_buf_free(param);
  }
}


static void ncp_comm_handle_auth_ok_signal(zb_bufid_t param)
{
  if (ZB_IN_BDB()
      && !IS_DISTRIBUTED_SECURITY()
      && ZB_TCPOL().update_trust_center_link_keys_required
      && !ZB_TCPOL().waiting_for_tclk_exchange) /* TCLK is not already in progress */
  {
    zb_uint16_t aps_key_idx = (zb_uint16_t)-1;

    if (!ZB_IEEE_ADDR_IS_ZERO(ZB_AIB().trust_center_address))
    {
      aps_key_idx = zb_aps_keypair_get_index_by_addr(ZB_AIB().trust_center_address,
                                                     ZB_SECUR_VERIFIED_KEY);
    }
    if (aps_key_idx == (zb_uint16_t)-1)
    {
      TRACE_MSG(TRACE_SECUR3, "BDB & !distributed - get TCLK over APS", (FMT__0));
      zdo_initiate_tclk_gen_over_aps(0);
    }
  }

  if (param != ZB_BUF_INVALID)
  {
    zb_buf_free(param);
  }

}


static void ncp_comm_handle_rejoin_after_secur_failed_signal(zb_bufid_t param)
{
  /* TODO: inform NCP Host */

  if (param != ZB_BUF_INVALID)
  {
    zb_buf_free(param);
  }
}


static void ncp_comm_handle_leave_with_rejoin_signal(zb_bufid_t param)
{
#ifdef ZB_USE_NVRAM
  /* If we fail, trace is given and assertion is triggered */
  (void)zb_nvram_write_dataset(ZB_NVRAM_COMMON_DATA);
#endif
#ifdef ZB_NWK_BLACKLIST
  zb_nwk_blacklist_reset();
#endif
  zdo_inform_app_leave(ZB_NWK_LEAVE_TYPE_REJOIN);
#ifdef ZB_NSNG
  /* A hack because of unperfect NSNG CSMA/CA mechanism, hack for CCB2255 */
  ZB_SCHEDULE_ALARM(zb_nwk_do_rejoin_after_leave, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(500));
#else
  zb_nwk_do_rejoin_after_leave(param);
#endif
}

#endif /* ZB_JOIN_CLIENT */


#ifdef ZB_COORDINATOR_ROLE

#ifdef ZB_SE_COMMISSIONING

/* ncp_comm_se_link_key_refresh_alarm() and bdb_link_key_refresh_alarm() do the same thing,
   but they are used as different refresh alarms and we need a capability to cancel them
   independently */
void ncp_comm_se_link_key_refresh_alarm(zb_bufid_t param)
{
  bdb_link_key_refresh_alarm(param);
}

#endif /* ZB_SE_COMMISSIONING */

static void ncp_comm_handle_secured_rejoin_signal(zb_bufid_t param)
{
  zb_apsme_transport_key_req_t *req =
    ZB_BUF_GET_PARAM(param, zb_apsme_transport_key_req_t);

#ifdef ZB_SE_COMMISSIONING
  if (ZB_SE_MODE())
  {
    /* If it was secured rejoin, but no good TCLK exists, rejoined
     * device must complete TCLK establishment via CBKE in 20
     * minutes. */
    if (se_cbke_exchange_schedule_alarm(param, ncp_comm_se_link_key_refresh_alarm))
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
  }
  else
#endif /* ZB_SE_COMMISSIONING */

#ifdef ZB_BDB_MODE
  if (ZB_IN_BDB())
  {
    if (ZB_TCPOL().update_trust_center_link_keys_required
        && !IS_DISTRIBUTED_SECURITY()
        && !zb_secur_get_link_key_by_address(req->dest_address.addr_long, ZB_SECUR_VERIFIED_KEY))
    {
      ZB_SCHEDULE_ALARM(bdb_link_key_refresh_alarm, param, ZB_TCPOL().trust_center_node_join_timeout);
    }
  }
  else
#endif /* ZB_BDB_MODE */

  if (param != ZB_BUF_INVALID)
  {
    zb_buf_free(param);
  }
}


static void ncp_comm_handle_tclk_verified_remote_signal(zb_address_ieee_ref_t param)
{
#ifdef ZB_SE_COMMISSIONING
  zb_bufid_t buf;

  if (ZB_SE_MODE())
  {
    buf = bdb_cancel_link_key_refresh_alarm(ncp_comm_se_link_key_refresh_alarm, param);

    if (buf != 0)
    {
      zb_buf_free(buf);
    }

    /* TODO: indicate a device got a TCLK.
       Possible solutions:
       1. raise ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED signal
       2. add some SE+NCP specific command */
    ZB_ASSERT(0);
  }
#else
  ZVUNUSED(param);
#endif /* ZB_SE_COMMISSIONING */
}


static void ncp_comm_handle_device_left_signal(zb_address_ieee_ref_t param)
{
#ifdef ZB_SE_COMMISSIONING
  if (ZB_SE_MODE())
  {
    bdb_cancel_link_key_refresh_alarm(ncp_comm_se_link_key_refresh_alarm, param);
  }
#else
  ZVUNUSED(param);
#endif /* ZB_SE_COMMISSINING */
}

#endif /* ZB_COORDINATOR_ROLE */

#ifdef ZB_FORMATION

static void ncp_comm_handle_authenticate_remote_signal(zb_bufid_t param)
{
#ifdef ZB_SE_COMMISSIONING
  TRACE_MSG(TRACE_ZDO1, "authenticate remote dev %hd", (FMT__H, param));
  if (!ZB_SE_MODE() || !se_cbke_exchange_schedule_alarm(param, bdb_link_key_refresh_alarm))
  {
    zb_buf_free(param);
  }
#else
  zb_buf_free(param);
#endif /* ZB_SE_COMMISSIONING */
}

#endif /* ZB_FORMATION */

static void ncp_comm_handle_comm_signal(zb_commissioning_signal_t signal, zb_bufid_t param)
{
  TRACE_MSG(TRACE_ZDO1, ">> ncp_comm_handle_comm_signal, signal %d, param %d",
            (FMT__D_D, signal, param));

  switch(signal)
  {
    case ZB_COMM_SIGNAL_START:
      ncp_comm_handle_start_signal(param);
      break;
#ifdef ZB_JOIN_CLIENT
    case ZB_COMM_SIGNAL_NWK_DISC_FAILED:
      ncp_comm_handle_nwk_disc_failed_signal(param);
      break;
    case ZB_COMM_SIGNAL_JOIN_FAILED:
      ncp_comm_handle_join_failed_signal(param);
      break;
    case ZB_COMM_SIGNAL_AUTH_FAILED:
      ncp_signal(NCP_SIGNAL_NWK_JOIN_AUTH_FAILED, param);
      break;
    case ZB_COMM_SIGNAL_INITIATE_REJOIN:
      ncp_comm_handle_initiate_rejoin_signal(param);
      break;
    case ZB_COMM_SIGNAL_DEV_ANNCE_SENT:
      ncp_comm_handle_dev_annce_sent_signal(param);
      break;
    case ZB_COMM_SIGNAL_ROUTER_STARTED:
      ncp_comm_handle_router_started_signal(param);
      break;
    case ZB_COMM_SIGNAL_TCLK_UPDATE_COMPLETE:
      ncp_signal(NCP_SIGNAL_SECUR_TCLK_EXCHANGE_COMPLETED, param);
      break;
    case ZB_COMM_SIGNAL_TCLK_UPDATE_FAILED:
      ncp_signal(NCP_SIGNAL_SECUR_TCLK_EXCHANGE_FAILED, param);
      break;
    case ZB_COMM_SIGNAL_LEAVE_DONE:
      ncp_comm_handle_leave_done_signal(param);
      break;
    case ZB_COMM_SIGNAL_AUTH_OK:
      ncp_comm_handle_auth_ok_signal(param);
      break;
    case ZB_COMM_SIGNAL_SECUR_FAILED:
      ncp_signal(NCP_SIGNAL_SECUR_TCLK_EXCHANGE_FAILED, param);
      break;
    case ZB_COMM_SIGNAL_REJOIN_AFTER_SECUR_FAILED:
      ncp_comm_handle_rejoin_after_secur_failed_signal(param);
      break;
    case ZB_COMM_SIGNAL_LEAVE_WITH_REJOIN:
      ncp_comm_handle_leave_with_rejoin_signal(param);
      break;
#endif /* ZB_JOIN_CLIENT */

#ifdef ZB_COORDINATOR_ROLE
    case ZB_COMM_SIGNAL_SECURED_REJOIN:
      ncp_comm_handle_secured_rejoin_signal(param);
      break;
    case ZB_COMM_SIGNAL_TCLK_VERIFIED_REMOTE:
      ncp_comm_handle_tclk_verified_remote_signal(param);
      break;
    case ZB_COMM_SIGNAL_DEVICE_LEFT:
      ncp_comm_handle_device_left_signal(param);
      break;
#endif /* ZB_COORDINATOR_ROLE */

#ifdef ZB_FORMATION
    case ZB_COMM_SIGNAL_FORMATION_DONE:
      ncp_signal(NCP_SIGNAL_NWK_FORMATION_OK, param);
      break;
    case ZB_COMM_SIGNAL_FORMATION_FAILED:
      ncp_signal(NCP_SIGNAL_NWK_FORMATION_FAILED, param);
      break;
    case ZB_COMM_SIGNAL_AUTHENTICATE_REMOTE:
      ncp_comm_handle_authenticate_remote_signal(param);
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

  TRACE_MSG(TRACE_ZDO1, "<< ncp_comm_handle_comm_signal", (FMT__0));
}


#ifdef ZB_ROUTER_ROLE

static zb_uint8_t ncp_comm_get_permit_join_duration(void)
{
  /* The network is closed by default */
  return 0;
}

#endif /* ZB_ROUTER_ROLE */


static zb_bool_t ncp_comm_must_use_installcode(zb_bool_t is_client)
{
  ZVUNUSED(is_client);

  return (zb_bool_t)ZB_TCPOL().require_installcodes;
}


#ifdef ZB_SE_COMMISSIONING

void ncp_se_commissioning_force_link(void)
{
  APS_SELECTOR().new_binding_handler = se_new_binding_handler;

  /* Do we need it on the SoC side? */
  /* ZDO_SELECTOR().match_desc_resp_handler = se_handle_match_desc_resp; */
  ZDO_SELECTOR().app_link_key_ind_handler = se_handle_link_key_indication;
}

#endif /* ZB_SE_COMMISSIONING */


void ncp_comm_commissioning_force_link(void)
{
  COMM_SELECTOR().signal = ncp_comm_handle_comm_signal;

#ifdef ZB_ROUTER_ROLE
  COMM_SELECTOR().get_permit_join_duration = ncp_comm_get_permit_join_duration;
#endif /* ZB_ROUTER_ROLE */

  COMM_SELECTOR().must_use_install_code = ncp_comm_must_use_installcode;

#ifdef ZB_SE_COMMISSIONING
  if (ZB_SE_MODE())
  {
    ncp_se_commissioning_force_link();
  }
#endif /* ZB_SE_COMMISSIONING */

#ifdef ZB_FORMATION
  zdo_formation_force_link();
#endif
}
