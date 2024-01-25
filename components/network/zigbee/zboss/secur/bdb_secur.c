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
/*  PURPOSE: Base Device specific security routines
*/

/* Supposed to be BDB security. Really also used for old r21-not-bdb as well.
   For NCP architecture put that code into SoC in contrast with commissioning BDB code which is at Host.

   Can be excluded for SE-only builds.
*/

#define ZB_TRACE_FILE_ID 2461
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_zdo_globals.h"
#include "zb_bdb_internal.h"

static void bdb_verify_tclk_alarm(zb_uint8_t param);
static void bdb_key_exchg_node_desc_callback(zb_uint8_t param);

void zdo_secur_init(void)
{
  /* see Table 4.33 Trust Center Policy Values */
  /* Set TC policy to some defaults */
  ZB_BZERO(&ZB_TCPOL(), sizeof(ZB_TCPOL()));
  ZB_TCPOL().allow_joins = ZB_TRUE;
#ifdef ZB_TC_REJOIN_ENABLE
  ZB_TCPOL().allow_tc_rejoins = ZB_TRUE;
  ZB_TCPOL().ignore_unsecure_tc_rejoins = ZB_FALSE;
  ZB_TCPOL().allow_unsecure_tc_rejoins = ZB_FALSE;
#endif
  ZB_TCPOL().allow_tc_link_key_requests = 1;
  ZB_TCPOL().allow_application_link_key_requests = 1;
  ZB_TCPOL().allow_remote_policy_change = ZB_TRUE;

#if defined ZB_SECURITY_INSTALLCODES
#if defined ZB_BDB_FORCE_INSTALLCODES
  ZB_TCPOL().require_installcodes = ZB_TRUE;
#else
  ZB_TCPOL().require_installcodes = ZB_FALSE;
#endif
#endif
  ZB_TCPOL().trust_center_node_join_timeout = ZB_DEFAULT_BDB_TRUST_CENTER_NODE_JOIN_TIMEOUT;
  /* According to BDB do 3 retries of establishing unique TCLK. r21 without BDB
   * does not require retries. But, seems, certification tests will not break
   * because of retries, so let's do BDB always. If need to exclude retries,
   * assign 1 to tclk_exchange_attempts_max somewhere.  */
  ZB_TCPOL().tclk_exchange_attempts_max = ZB_DEFAULT_BDB_TCLINK_KEY_EXCHANGE_ATTEMPTS_MAX;

  ZB_TCPOL().accept_new_unsolicited_trust_center_link_key = ZB_TRUE;
  ZB_TCPOL().accept_new_unsolicited_application_link_key = ZB_TRUE;

  /* Be r21/BDB by default - request unique TCLK. SE or r23 DLK can clear that flag. */
  ZB_TCPOL().update_trust_center_link_keys_required = ZB_TRUE;
}


#ifndef ZB_COORDINATOR_ONLY
void bdb_initiate_key_exchange(zb_uint8_t param)
{
  zb_ret_t ret;
  zb_zdo_node_desc_req_t *req;

  if (param == 0U)
  {
    ret = zb_buf_get_out_delayed(bdb_initiate_key_exchange);
    if (ret != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
    }
    return;
  }

  TRACE_MSG(TRACE_SECUR1, "bdb_initiate_key_exchange %hd: sending node desc req, cb %p", (
              FMT__H_P, param, bdb_key_exchg_node_desc_callback));

  /* TODO: check for bdbTCLinkKeyExchangeMethod other than 0 here (not invented
   * by Zigbee Alliance yet). */

  ZB_TCPOL().waiting_for_tclk_exchange = ZB_TRUE;
  req = zb_buf_initial_alloc(param, sizeof(zb_zdo_node_desc_req_t));
  req->nwk_addr = 0; /* send to coordinator == TC */
  (void)zb_zdo_node_desc_req(param, bdb_key_exchg_node_desc_callback);

  /* Start fast poll for TCLK. */
  zb_zdo_pim_start_fast_poll(0);
}


static void bdb_key_exchg_node_desc_callback(zb_uint8_t param)
{
  zb_zdo_node_desc_resp_t *resp = (zb_zdo_node_desc_resp_t*)zb_buf_begin(param);

  if (resp->hdr.status != ZB_ZDP_STATUS_SUCCESS || resp->hdr.nwk_addr != 0U)
  {
    TRACE_MSG(TRACE_SECUR1, "bdb_key_exchg_node_desc_callback: node desc req failed", (FMT__0));
    ZB_SCHEDULE_CALLBACK(bdb_update_tclk_failed, param);
  }
  else if ((resp->node_desc.server_mask >> 9) < 21U)
  {
    TRACE_MSG(TRACE_SECUR1, "bdb_key_exchg_node_desc_callback: TC version %hd prior r21 - not need to recv TCLK", (FMT__H, (zb_uint8_t)(resp->node_desc.server_mask >> 9)));
    ZB_AIB().coordinator_version = 20;
#ifdef ZB_USE_NVRAM
    /* If we fail, trace is given and assertion is triggered */
    (void)zb_nvram_write_dataset(ZB_NVRAM_APS_SECURE_DATA);
#endif
    /* Finish commissioning and call zdo_startup_complete */
    zdo_secur_update_tclk_done(param);
  }
  else
  {
    TRACE_MSG(TRACE_SECUR1, "bdb_key_exchg_node_desc_callback: ok, >= v21", (FMT__0));
    ZB_AIB().coordinator_version = (zb_uint8_t)((resp->node_desc.server_mask & 0xFF00U) >> 8);
#ifdef ZB_USE_NVRAM
    /* If we fail, trace is given and assertion is triggered */
    (void)zb_nvram_write_dataset(ZB_NVRAM_APS_SECURE_DATA);
#endif
    if (ZB_TCPOL().update_trust_center_link_keys_required)
    {
      TRACE_MSG(TRACE_SECUR1, "bdb_key_exchg_node_desc_callback: ok, doing zb_zdo_update_tclk in bdb", (FMT__0));
      ZB_TCPOL().tclk_exchange_attempts = 0;
      ZB_SCHEDULE_ALARM(bdb_request_tclk_alarm, 0, ZB_BDBC_TCLINK_KEY_EXCHANGE_TIMEOUT);
      ZB_SCHEDULE_CALLBACK(zb_zdo_update_tclk, param);
    }
    else
    {
      TRACE_MSG(TRACE_SECUR1, "bdb_key_exchg_node_desc_callback: ok, no need to update tclk", (FMT__0));
      zdo_secur_update_tclk_done(param);
    }
  }
}


void bdb_request_tclk_alarm(zb_uint8_t param)
{
  if (param == 0U)
  {
    zb_ret_t ret = zb_buf_get_out_delayed(bdb_request_tclk_alarm);
    if (ret != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
    }
    return;
  }

  /*
The node SHALL make an attempt and wait for
up to bdbcTCLinkKeyExchangeTimeout for a
response. If no response is received before this
timeout expires, the node shall repeat the
attempt such that at most bdbTCLinkKey-
ExchangeAttempts are made in total. If no
response is received after bdbTCLinkKey-
ExchangeAttempts are made, the attempt is
considered to have failed.
   */
  ZB_TCPOL().tclk_exchange_attempts++;
  if (ZB_TCPOL().tclk_exchange_attempts > ZB_TCPOL().tclk_exchange_attempts_max)
  {
    TRACE_MSG(TRACE_SECUR1, "bdb_request_tclk_alarm bdb_tclink_key_exchange_attempts %hd - failure", (FMT__H, (zb_uint8_t)ZB_TCPOL().tclk_exchange_attempts));
    ZB_SCHEDULE_CALLBACK(bdb_update_tclk_failed, param);
  }
  else
  {
    TRACE_MSG(TRACE_SECUR1, "bdb_request_tclk_alarm bdb_tclink_key_exchange_attempts %hd - retry key req", (FMT__H, (zb_uint8_t)ZB_TCPOL().tclk_exchange_attempts));
    ZB_SCHEDULE_ALARM(bdb_request_tclk_alarm, 0, ZB_BDBC_TCLINK_KEY_EXCHANGE_TIMEOUT);
    ZB_SCHEDULE_CALLBACK(zb_zdo_update_tclk, param);
  }

}


void bdb_initiate_key_verify(zb_uint8_t param)
{
  /* NCP TODO: make in working in r21 as well as bdb */
  ZB_TCPOL().tclk_exchange_attempts = 0;
  ZB_SCHEDULE_ALARM_CANCEL(bdb_request_tclk_alarm, 0);
  ZB_SCHEDULE_ALARM_CANCEL(bdb_verify_tclk_alarm, 0);
  ZB_SCHEDULE_ALARM(bdb_verify_tclk_alarm, 0, ZB_BDBC_TCLINK_KEY_EXCHANGE_TIMEOUT);
  /* originally we call zb_zdo_verify_tclk directly when in no-bdb no-se mode
   * (== cert tests mode actually) */
  ZB_SCHEDULE_CALLBACK(zb_zdo_verify_tclk, param);
}

zb_bool_t bdb_verify_tclk_in_progress(void)
{
  zb_time_t time_alarm;
  return (zb_bool_t)(ZB_SCHEDULE_GET_ALARM_TIME(bdb_verify_tclk_alarm, 0, &time_alarm) == RET_OK);
}

static void bdb_verify_tclk_alarm(zb_uint8_t param)
{
  if (param == 0U)
  {
    zb_ret_t ret = zb_buf_get_out_delayed(bdb_verify_tclk_alarm);
    if (ret != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
    }
    return;
  }

  /*
The node SHALL make an attempt and wait for
up to bdbcTCLinkKeyExchangeTimeout for a
response. If no response is received before this
timeout expires, the node shall repeat the
attempt such that at most bdbTCLinkKey-
ExchangeAttempts are made in total. If no
response is received after bdbTCLinkKey-
ExchangeAttempts are made, the attempt is
considered to have failed.
   */
  ZB_TCPOL().tclk_exchange_attempts++;
  if (ZB_TCPOL().tclk_exchange_attempts > ZB_TCPOL().tclk_exchange_attempts_max)
  {
    TRACE_MSG(TRACE_SECUR1, "bdb_verify_tclk_alarm bdb_tclink_key_exchange_attempts %hd - failure", (FMT__H, (zb_uint8_t)ZB_TCPOL().tclk_exchange_attempts));
    ZB_SCHEDULE_CALLBACK(bdb_update_tclk_failed, param);
  }
  else
  {
    TRACE_MSG(TRACE_SECUR1, "bdb_verify_tclk_alarm bdb_tclink_key_exchange_attempts %hd - retry key verify", (FMT__H, (zb_uint8_t)ZB_TCPOL().tclk_exchange_attempts));
    ZB_SCHEDULE_ALARM(bdb_verify_tclk_alarm, 0, ZB_BDBC_TCLINK_KEY_EXCHANGE_TIMEOUT);
    ZB_SCHEDULE_CALLBACK(zb_zdo_verify_tclk, param);
  }
}


void bdb_update_tclk_failed(zb_uint8_t param)
{
  TRACE_MSG(TRACE_SECUR1, "bdb_update_tclk_failed %hd", (FMT__H, param));
  ZB_SCHEDULE_ALARM_CANCEL(bdb_request_tclk_alarm, 0);
  ZB_TCPOL().waiting_for_tclk_exchange = ZB_FALSE;

  zdo_commissioning_tclk_upd_failed(param);
}


/**
   Cpommon call for BDB and legacy r21 for TCLK uodate complete event.
 */
void zdo_secur_update_tclk_done(zb_uint8_t param)
{
  TRACE_MSG(TRACE_SECUR1, "TCLK verification done - continue BDB commissioning", (FMT__0));

  ZB_TCPOL().waiting_for_tclk_exchange = ZB_FALSE;
  ZB_SCHEDULE_ALARM_CANCEL(bdb_request_tclk_alarm, 0);
  ZB_SCHEDULE_ALARM_CANCEL(bdb_verify_tclk_alarm, 0);
  /* Stop fast poll (was started in bdb_initiate_key_exchange()). */
  zb_zdo_fast_poll_leave(0);

  zdo_commissioning_tclk_upd_complete(param);
}
#endif  /* ZB_COORDINATOR_ONLY */

#ifdef ZB_COORDINATOR_ROLE
#ifndef ZB_LITE_NO_TRUST_CENTER_REQUIRE_KEY_EXCHANGE
void bdb_link_key_refresh_alarm(zb_uint8_t param)
{
  /* It was for sure transport key request for nwk key */
  zb_apsme_transport_key_req_t ind = *ZB_BUF_GET_PARAM(param, zb_apsme_transport_key_req_t);
  zb_uint8_t dev_type = zb_nwk_get_nbr_dvc_type_by_ieee(ind.dest_address.addr_long);
  zb_uint8_t use_parent = ind.key.nwk.use_parent;

  /* From r21 (4.4.4.1.2 When Generated):
   * The APSME (for example, on a Trust Center) shall initiate the APSME-REMOVE-DEVICE.request
   * primitive when it wants to request that a parent device (specified by the ParentAddress
   * parameter) remove one of its child devices (as specified by the TargetAddress parameter),
   * or if it wants to remove a router from the network.
   */
  if (use_parent || (dev_type == ZB_NWK_DEVICE_TYPE_ROUTER))
  {
    zb_apsme_remove_device_req_t *req = ZB_BUF_GET_PARAM(param, zb_apsme_remove_device_req_t);

    if (use_parent)
    {
      ZB_IEEE_ADDR_COPY(req->parent_address, ind.key.nwk.parent_address);
    }
    else
    {
      /* From 4.4.4.1.3 (Effect on Receipt):
       * If the device to be removed is a router the ParentAddress and TargetAddress
       * shall be the same.
       */
      ZB_IEEE_ADDR_COPY(req->parent_address, ind.dest_address.addr_long);
    }
    ZB_IEEE_ADDR_COPY(req->child_address, ind.dest_address.addr_long);
    TRACE_MSG(TRACE_SECUR1, "bdb_link_key_refresh_alarm: send remove-device to addr " TRACE_FORMAT_64 " via ZR " TRACE_FORMAT_64,
              (FMT__A_A, TRACE_ARG_64(req->child_address), TRACE_ARG_64(req->parent_address)));
    ZB_SCHEDULE_CALLBACK(zb_secur_apsme_remove_device, param);

    /* CS-NFS-TC-02: If device failed TC LK exchange procedure remove TC should remove
     * all related information about device from it's persistent storage.
     * Do not remove device here - remove it in bdb_remove_joiner that will be called in nlde_data_confirm
     * with param == bdb_remove_device_param.
     */
    ZB_AIB().bdb_remove_device_param = param;
  }
  else
  {
    zb_nlme_leave_request_t *lr = ZB_BUF_GET_PARAM(param, zb_nlme_leave_request_t);
    TRACE_MSG(TRACE_SECUR1,
              "bdb_link_key_refresh_alarm: send LEAVE directly to addr " TRACE_FORMAT_64,
              (FMT__A, TRACE_ARG_64(ind.dest_address.addr_long)));
    ZB_IEEE_ADDR_COPY(lr->device_address, ind.dest_address.addr_long);
    lr->remove_children = ZB_TRUE;
    lr->rejoin = ZB_FALSE;
    ZB_SCHEDULE_CALLBACK(zb_nlme_leave_request, param);
  }

  zb_legacy_device_auth_signal_cancel(ind.dest_address.addr_long);
  zb_prepare_and_send_device_authorized_signal(ind.dest_address.addr_long,
                                               ZB_ZDO_AUTHORIZATION_TYPE_R21_TCLK,
                                               ZB_ZDO_TCLK_AUTHORIZATION_TIMEOUT);
}

void bdb_link_key_transport_with_alarm(zb_uint8_t param2, zb_uint16_t param)
{
  zb_apsme_transport_key_req_t *transport_key_req = ZB_BUF_GET_PARAM(param, zb_apsme_transport_key_req_t);
  zb_address_ieee_ref_t addr_ref;

  TRACE_MSG(TRACE_SECUR1, "bdb_link_key_transport_with_alarm %hd %hd", (FMT__H_H, param, param2));

  /* Cancel old refresh alarm to prevent unnecessary leave of child device
   * if the previous authorization attempt failed, but current attempt will succeed */
  if (zb_address_by_ieee(transport_key_req->dest_address.addr_long, ZB_FALSE, ZB_FALSE, &addr_ref) == RET_OK)
  {
    /* GP add */
#if 0
    bdb_cancel_link_key_refresh_alarm(bdb_link_key_refresh_alarm, addr_ref);
#else
    zb_bufid_t buf;
    buf = bdb_cancel_link_key_refresh_alarm(bdb_link_key_refresh_alarm, addr_ref);
    if (buf != 0)
    {
      zb_buf_free(buf);
    }
#endif
  }

  /* Save key transport request to be able to get addresses when calling remove device */
  zb_buf_copy(param2, param);

#ifdef ZB_CERTIFICATION_HACKS
  if (ZB_CERT_HACKS().deliver_nwk_key_cb)
  {
    ZB_CERT_HACKS().deliver_nwk_key_cb(param);
  }
#endif
  ZB_SCHEDULE_CALLBACK(zb_apsme_transport_key_request, param);
  if (ZB_TCPOL().update_trust_center_link_keys_required
      && !IS_DISTRIBUTED_SECURITY())
  {
    /* If we already have TCLK to the device, no need to wait for r21 TCLK establishment. */
    if (zb_aps_keypair_get_index_by_addr(transport_key_req->dest_address.addr_long, ZB_SECUR_VERIFIED_KEY) == (zb_uint16_t)-1)
    {
      TRACE_MSG(TRACE_INFO1, "ACHTUNG: schedule bdb_link_key_refresh_alarm, ref %hd", (FMT__H, param2));
      ZB_SCHEDULE_ALARM(bdb_link_key_refresh_alarm, param2, ZB_TCPOL().trust_center_node_join_timeout);
      param2 = 0;
    }
    else
    {
      TRACE_MSG(TRACE_SECUR1, "Device " TRACE_FORMAT_64 " already has verified TCLK",
                (FMT__A, TRACE_ARG_64(transport_key_req->dest_address.addr_long)));
      /* then will call zb_legacy_device_auth_signal_alarm. The idea is
       * following: maybe, device will still update TCLK. Let's send only one
       * signal. zb_legacy_device_auth_signal_alarm() modifyed to check
       * for TCLK existence. */
    }
  }

  if (param2 != 0)
  {
    zb_buf_free(param2);
  }

  /* if legacy devices supported */
  if (param2 != 0
      && RET_OK == zb_address_by_ieee(transport_key_req->dest_address.addr_long,
                                      ZB_FALSE, /* don't create */
                                      ZB_FALSE, /* don't lock */
                                      &addr_ref))
  {
    /* it's need to send DevAuth sig for a legacy device
     * (will be canceled after receive ConfirmKey) */
    ZB_SCHEDULE_ALARM(zb_legacy_device_auth_signal_alarm,
                      addr_ref,
                      ZB_TCPOL().trust_center_node_join_timeout);
  }
}

zb_bool_t bdb_check_for_alarm_addr(zb_uint8_t param, void* ieeeaddr)
{
  zb_bool_t ret;
  zb_apsme_transport_key_req_t *ind = ZB_BUF_GET_PARAM(param, zb_apsme_transport_key_req_t);

  ret = (zb_bool_t)ZB_IEEE_ADDR_CMP(ind->dest_address.addr_long, ieeeaddr);
  TRACE_MSG(TRACE_SECUR3, "bdb_check_for_alarm_addr ret %d", (FMT__D, ret));
  return ret;
}

zb_uint8_t bdb_cancel_link_key_refresh_alarm(zb_callback_t func, zb_uint8_t param)
{
  zb_uint8_t ret;
  zb_ieee_addr_t ieeeaddr;

  TRACE_MSG(TRACE_SECUR1, "bdb_cancel_link_key_refresh_alarm addr_ref %hd", (FMT__H, param));

  zb_address_ieee_by_ref(ieeeaddr, param);

  ret = zb_schedule_alarm_cancel_compare(func, bdb_check_for_alarm_addr, ieeeaddr);
  zb_schedule_alarm_cancel(zb_legacy_device_auth_signal_alarm, param, NULL);

  return ret;
}
#endif  /* #ifndef ZB_LITE_NO_TRUST_CENTER_REQUIRE_KEY_EXCHANGE */


#ifdef ZB_ROUTER_ROLE
void bdb_remove_joiner(zb_uint8_t param)
{
  zb_address_ieee_ref_t addr_ref;
  zb_ieee_addr_t ieee_addr;
  zb_neighbor_tbl_ent_t *nbt;
  zb_bool_t not_child = ZB_TRUE;
  zb_ret_t ret;

  TRACE_MSG(TRACE_SECUR1, ">>bdb_remove_joiner", (FMT__0));
  /* Remove Device payload is passed in buffer nsdu: cmd_id| target address */
  ZB_IEEE_ADDR_COPY(ieee_addr, (zb_ieee_addr_t*) ((zb_uint8_t*)zb_buf_begin(param) + sizeof(zb_uint8_t)));
  ret = zb_address_by_ieee(ieee_addr, ZB_FALSE, ZB_FALSE, &addr_ref);

  if (ret == RET_OK)
  {
    /* Check that device is our child. If device is our child remove child from neighbor table
     * and modify router_child_num/ed_child_num fields in NIB. If device not a child
     * remove it from neighbor table. If joiner is not a neighbor - delete keys and addresses
     * associated with it.
     */
    if (zb_nwk_neighbor_get(addr_ref, ZB_FALSE, &nbt) == RET_OK &&
        (nbt->relationship == ZB_NWK_RELATIONSHIP_CHILD || nbt->relationship == ZB_NWK_RELATIONSHIP_UNAUTHENTICATED_CHILD))
    {
      zdo_device_removed(addr_ref, ZB_TRUE);
      not_child = ZB_FALSE;
    }

    if (not_child == ZB_TRUE)
    {
      zb_nwk_forget_device(addr_ref);
    }

    ZB_AIB().bdb_remove_device_param = 0;
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "can't remove joiner, no device in address table with IEEE addr " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(ieee_addr)));
  }

  zb_buf_free(param);
  TRACE_MSG(TRACE_SECUR1, ">>bdb_remove_joiner", (FMT__0));
}
#endif  /* ZB_ROUTER_ROLE */

#endif  /* ZB_COORDINATOR_ROLE */
