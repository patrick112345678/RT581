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
/*  PURPOSE: APS key establishment between 2 non-TC devices
*/

#define ZB_TRACE_FILE_ID 38
#include "zb_common.h"

#ifdef ZB_ENABLE_SE_MIN_CONFIG

#ifdef ZB_SE_ENABLE_KEC_CLUSTER

#include "zb_se.h"
#include "zb_ncp.h"

#if defined(ZB_SE_ENABLE_SERVICE_DISCOVERY_PROCESSING)
void zb_se_service_discovery_op_completed(zb_uint16_t short_addr, zb_uint8_t ep, zb_uint16_t cluster_id, zse_service_disc_step_t step);
void zb_se_service_discovery_process_stored_devs(zb_uint8_t param);
#endif

static void aps_key_est_match_desc_cb(zb_uint8_t param);
static void remote_key_est_bind_callback(zb_uint8_t param);

zb_bool_t zb_se_has_valid_key(zb_uint16_t addr)
{
  zb_aps_device_key_pair_set_t *key;
  zb_ieee_addr_t dst_ieee;
  zb_bool_t ret;

  ret = (zb_address_ieee_by_short(addr, dst_ieee) == RET_OK);
  if (ret)
  {
    key = zb_secur_get_link_key_by_address(dst_ieee, ZB_SECUR_VERIFIED_KEY);
    ret = (zb_bool_t)(key != NULL && key->key_source == ZB_SECUR_KEY_SRC_CBKE);
  }
  TRACE_MSG(TRACE_SECUR3, "zb_se_has_valid_key ret %d", (FMT__D, ret));
  return ret;
}

zb_bool_t zb_se_has_valid_key_by_ieee(zb_ieee_addr_t addr)
{
  zb_aps_device_key_pair_set_t *key;
  zb_bool_t ret;

  key = zb_secur_get_link_key_by_address(addr, ZB_SECUR_VERIFIED_KEY);
  ret = (zb_bool_t)(key != NULL && key->key_source == ZB_SECUR_KEY_SRC_CBKE);
  TRACE_MSG(TRACE_SECUR3, "zb_se_has_valid_key ret %d", (FMT__D, ret));
  return ret;
}


zb_ret_t zb_se_debug_get_link_key(zb_uint16_t addr, zb_uint8_t link_key[ZB_CCM_KEY_SIZE])
{
  zb_aps_device_key_pair_set_t *key;
  zb_ieee_addr_t dst_ieee;
  zb_ret_t ret = RET_NOT_FOUND;

  if (zb_se_has_valid_key(addr))
  {
    ret = zb_address_ieee_by_short(addr, dst_ieee);
    if (ret == RET_OK)
    {
      key = zb_secur_get_link_key_by_address(dst_ieee, ZB_SECUR_VERIFIED_KEY);
      if ((key != NULL)
          && key->key_source == ZB_SECUR_KEY_SRC_CBKE)
      {
        ZB_MEMCPY(link_key, key->link_key, ZB_CCM_KEY_SIZE);
        ret = RET_OK;
      }
    }
  }
  return ret;
}


zb_ret_t zb_se_debug_get_link_key_by_long(zb_ieee_addr_t ieee, zb_uint8_t link_key[ZB_CCM_KEY_SIZE])
{
  zb_aps_device_key_pair_set_t *key;
  zb_ret_t ret = RET_NOT_FOUND;

  if (zb_se_has_valid_key_by_ieee(ieee))
  {
    key = zb_secur_get_link_key_by_address(ieee, ZB_SECUR_VERIFIED_KEY);
    if (key != NULL && key->key_source == ZB_SECUR_KEY_SRC_CBKE)
    {
      ZB_MEMCPY(link_key, key->link_key, ZB_CCM_KEY_SIZE);
      ret = RET_OK;
    }
  }
  return ret;
}


zb_ret_t zb_se_debug_get_nwk_key(zb_uint8_t key[ZB_CCM_KEY_SIZE])
{
  zb_ret_t ret = RET_NOT_FOUND;
  zb_uint8_t *keyp = secur_nwk_key_by_seq(ZB_NIB().active_key_seq_number);
  if (keyp != NULL)
  {
    ZB_MEMCPY(key, keyp, ZB_CCM_KEY_SIZE);
    ret = RET_OK;
  }
  return ret;
}


zb_ret_t zb_se_debug_get_ic_key(zb_uint8_t key[ZB_CCM_KEY_SIZE])
{
  return zb_secur_ic_get_key_by_address(ZB_AIB().trust_center_address, key);
}


void zb_se_start_aps_key_establishment(zb_uint8_t param, zb_uint16_t addr)
{
  zb_zdo_match_desc_param_t *req;

  TRACE_MSG(TRACE_SECUR3, "zb_se_start_aps_key_establishment param %hd addr 0x%x", (FMT__H_D, param, addr));

  ZB_ASSERT(addr != 0U);

  req = zb_buf_initial_alloc(param, sizeof(zb_zdo_match_desc_param_t) + 1U * sizeof(zb_uint16_t));

  /* Send unicast to the peer */
  req->nwk_addr = req->addr_of_interest = addr;
  req->profile_id = ZB_AF_SE_PROFILE_ID;
  req->num_in_clusters = 1;
  req->num_out_clusters = 0;
  req->cluster_list[0] = ZB_ZCL_CLUSTER_ID_KEY_ESTABLISHMENT;

  (void)zb_zdo_match_desc_req(param, aps_key_est_match_desc_cb);
}


static void aps_key_est_match_desc_cb(zb_uint8_t param)
{
  zb_ret_t ret;
  zb_zdo_match_desc_resp_t *resp = zb_buf_begin(param);
  zb_uint8_t local_ep;
  zb_uint8_t remote_ep;

  TRACE_MSG(TRACE_SECUR3, "aps_key_est_match_desc_cb param %hd", (FMT__H, param));
  /* Now seek for our SE endpoint  */
  local_ep = zb_zcl_get_next_target_endpoint(
    0, ZB_ZCL_CLUSTER_ID_KEY_ESTABLISHMENT, ZB_ZCL_CLUSTER_ANY_ROLE, ZB_AF_SE_PROFILE_ID);

  if (local_ep == 0U)
  {
    TRACE_MSG(TRACE_ERROR, "No local SE endpoint!", (FMT__0));
  }
  if (local_ep != 0U && resp->status == (zb_uint8_t)ZB_ZDP_STATUS_SUCCESS && resp->match_len >= 1U)
  {
    zb_zdo_bind_req_param_t *req;
    zb_uint16_t remote_addr = resp->nwk_addr;

    /* There can be only one endpoint in match resp: SE with key establishment cluster. */
    remote_ep = *((zb_uint8_t*)(resp + 1));
    TRACE_MSG(TRACE_ZDO1, "aps_key_est_match_desc_cb local ep %hd remote ep %hd", (FMT__H_H, local_ep, remote_ep));

    req = ZB_BUF_GET_PARAM(param, zb_zdo_bind_req_param_t);
    ret = zb_address_ieee_by_short(remote_addr, req->src_address);
    ZB_ASSERT(ret == RET_OK);
    if (ret == RET_OK)
    {
      req->src_endp = remote_ep;
      req->dst_endp = local_ep;
      req->cluster_id = ZB_ZCL_CLUSTER_ID_KEY_ESTABLISHMENT;
      req->dst_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
      ZB_MEMCPY(&req->dst_address.addr_long, (zb_uint8_t*)ZB_PIBCACHE_EXTENDED_ADDRESS(), sizeof(zb_ieee_addr_t));
      req->req_dst_addr = remote_addr;
      (void)zb_zdo_bind_req(param, remote_key_est_bind_callback);
    }
  }
  else
  {
    ret = RET_ERROR;
  }

  if (ret != RET_OK)
  {
    TRACE_MSG(TRACE_ZDO1, "No local/remote SE endpoint with key establishment cluster", (FMT__0));
    /*cstat !MISRAC2012-Rule-14.3_b */
    /** @mdr{00021,0} */
    if (!NCP_CATCH_PARTNER_LK_FAILED(param))
    {
      zb_buf_free(param);
    }
  }
}


static void partner_lk_est_timed_out(zb_uint8_t unused)
{
  ZVUNUSED(unused);
  (void)NCP_CATCH_PARTNER_LK_FAILED(0);
}


static void remote_key_est_bind_callback(zb_uint8_t param)
{
  zb_ret_t ret;
  zb_zdo_bind_resp_t *bind_resp = (zb_zdo_bind_resp_t*)zb_buf_begin(param);

  if (bind_resp->status == (zb_uint8_t)ZB_ZDP_STATUS_SUCCESS)
  {
    /* Every ZDO command has all addressing info in its param */
    zb_apsde_data_indication_t *ind = (zb_apsde_data_indication_t *)ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);
    zb_ieee_addr_t partner_addr;
    zb_apsme_request_key_req_t *req;

    TRACE_MSG(TRACE_SECUR3, "remote_key_est_bind_callback success param %hd", (FMT__H, param));

    /* Get partner address from the resp */
    ret = zb_address_ieee_by_short(ind->src_addr, partner_addr);
    ZB_ASSERT(ret == RET_OK);
    if (ret == RET_OK)
    {
      /* Bound ok, now ask TC for a key */

      req = ZB_BUF_GET_PARAM(param, zb_apsme_request_key_req_t);
      ZB_IEEE_ADDR_COPY(req->dest_address, ZB_AIB().trust_center_address);
      req->key_type = ZB_REQUEST_APP_LINK_KEY;
      ZB_IEEE_ADDR_COPY(req->partner_address, partner_addr);
      ZB_SCHEDULE_CALLBACK(zb_secur_apsme_request_key, param);

      /* Result will come into zb_apsme_transport_key_indication() -> zb_secur_indicate_app_link_key() */

      /* setup a guard alarm */
      ZB_SCHEDULE_ALARM(partner_lk_est_timed_out, 0, ZB_SE_PARTNER_LK_SETUP_TIMEOUT);
    }
  }
  else
  {
    ret = RET_ERROR;
  }

  if (ret != RET_OK)
  {
    TRACE_MSG(TRACE_ZDO1, "Remote rejected bind to KE cluster", (FMT__0));
    /*cstat !MISRAC2012-Rule-14.3_b */
    /** @mdr{00021,1} */
    if (!NCP_CATCH_PARTNER_LK_FAILED(param))
    {
      zb_buf_free(param);
    }
    /* TODO: handle? */
  }
}


static void secur_indicate_app_link_key(zb_uint8_t param)
{
  zb_apsme_transport_key_indication_t *ind = ZB_BUF_GET_PARAM(param, zb_apsme_transport_key_indication_t);
  zb_uint16_t addr = zb_address_short_by_ieee(ind->key.app.partner_address);

  zb_ieee_addr_t partner_address;
  zb_uint8_t *addr_p;

  TRACE_MSG(TRACE_ZDO1, "zb_secur_indicate_app_link_key param %hd addr 0x%xd", (FMT__H_D, param, addr));

  ZB_SCHEDULE_ALARM_CANCEL(partner_lk_est_timed_out, ZB_ALARM_ANY_PARAM);
  /* NOTE: Previos behaviour: if bound device don't have commodity type, we don't require to get
   * transport key before binding start.
   * Now we always send ZB_SE_SIGNAL_APS_KEY_READY signal. It is necessary because
   * IHD receive transport key between itself and thermostat after ZB_SE_SIGNAL_SERVICE_DISCOVERY_DO_BIND signal.
   */
#if defined(ZB_SE_ENABLE_SERVICE_DISCOVERY_PROCESSING)
  if (ZSE_CTXC().commissioning.state == SE_STATE_SERVICE_DISCOVERY)
  {
    TRACE_MSG(TRACE_ZDO1, "In discovery state - call zb_se_service_discovery_op_completed", (FMT__0));
    zb_se_service_discovery_op_completed(addr, 0, 0, ZSE_SERVICE_DISC_GEN_KEY);
    /* NOTE: we don't extract any data from current buffer */
    zb_buf_get_out_delayed(zb_se_service_discovery_process_stored_devs);
  }
#else
  ZVUNUSED(addr);
#endif

  /* TODO: handle key establishment done from "manual" bind when in steady state. */
  ZB_IEEE_ADDR_COPY(partner_address, ind->key.app.partner_address);
  addr_p = zb_app_signal_pack(param,
                              ZB_SE_SIGNAL_APS_KEY_READY,
                              RET_OK,
                              (zb_uint8_t)sizeof(zb_ieee_addr_t));
  TRACE_MSG(TRACE_ZDO1, "Inform user about link key recv", (FMT__0));
  ZB_IEEE_ADDR_COPY(addr_p, partner_address);
  ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete, param);
}


void se_handle_link_key_indication(zb_bufid_t param)
{
  zb_apsme_transport_key_indication_t *ind = ZB_BUF_GET_PARAM(param,
                                                              zb_apsme_transport_key_indication_t);

  TRACE_MSG(TRACE_SECUR2, "update app link key with " TRACE_FORMAT_64,
            (FMT__A, TRACE_ARG_64(ind->key.app.partner_address)));

  /* 07/26/2017 EE CR:MINOR Note that, when you implement
   * peer-to-peer APS keys in SE, you will be here as well. So
   * key src will be either unknown or cbke depending on
   * availability of cbke key with TC. */
  /* NOTE: if we get application link key from TC, we trust the TC and set this
   * link key as CBKE
   */
  (void)zb_secur_update_key_pair(ind->key.app.partner_address,
                                 ind->key.app.key,
                                 ZB_SECUR_UNIQUE_KEY,
                                 ZB_SECUR_VERIFIED_KEY,
                                 /* If we have valid SE TCLK, mark that APS key also as SE. */
                                 zb_se_has_valid_key(0) ? ZB_SECUR_KEY_SRC_CBKE :
                                 ZB_SECUR_KEY_SRC_UNKNOWN);
#ifdef ZB_USE_NVRAM
  /* If we fail, trace is given and assertion is triggered */
  (void)zb_nvram_write_dataset(ZB_NVRAM_APS_SECURE_DATA);
#endif /* ZB_USE_NVRAM */

  ZB_SCHEDULE_CALLBACK(secur_indicate_app_link_key, param);
}


#endif /* ZB_SE_ENABLE_KEC_CLUSTER */

#endif /* ZB_ENABLE_SE_MIN_CONFIG */
