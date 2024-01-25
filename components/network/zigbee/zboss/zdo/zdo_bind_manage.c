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
/* PURPOSE: ZDO Bind management
*/

#define ZB_TRACE_FILE_ID 2092
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_hash.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zdo_common.h"
#include "zb_ncp.h"


/*! \addtogroup ZB_ZDO */
/*! @{ */
/* todo: this define used only to fit in 64k */

static zb_uint8_t zb_zdo_bind_unbind_req(zb_uint8_t param, zb_callback_t cb, zb_bool_t bind);
#ifndef ZB_LITE_NO_END_DEVICE_BIND
static zb_uint8_t send_bind_unbind_req(zb_uint8_t param, zb_uint8_t target_dev_num, zb_uint16_t cluster_id, zb_callback_t cb, zb_bool_t bind);
#endif
void zb_zdo_send_check_bind_unbind(zb_uint8_t param);
void zb_zdo_bind_unbind_check_cb(zb_uint8_t param);
void zb_zdo_end_device_bind_cb(zb_uint8_t param);
void zb_zdo_end_device_unbind_cb(zb_uint8_t param);

zb_uint8_t zb_zdo_bind_req(zb_uint8_t param, zb_callback_t cb)
{
  return zb_zdo_bind_unbind_req(param, cb, ZB_TRUE);
}

zb_uint8_t zb_zdo_unbind_req(zb_uint8_t param, zb_callback_t cb)
{
  return zb_zdo_bind_unbind_req(param, cb, ZB_FALSE);
}

static zb_uint8_t zb_zdo_bind_unbind_req(zb_uint8_t param, zb_callback_t cb, zb_bool_t bind)
{
  zb_zdo_bind_req_param_t *bind_param;
  zb_zdo_bind_req_head_t *bind_req;
  zb_uint8_t               tsn;

  TRACE_MSG(TRACE_ZDO2, ">> zb_zdo_bind_unbind_req param %hd, bind %hd", (FMT__D_H, param, bind));
  bind_param = ZB_BUF_GET_PARAM(param, zb_zdo_bind_req_param_t);
  bind_req = zb_buf_initial_alloc(param, sizeof(zb_zdo_bind_req_head_t));

  ZB_HTOLE64(bind_req->src_address, bind_param->src_address);
  bind_req->src_endp = bind_param->src_endp;
  ZB_HTOLE16((zb_uint8_t*)&bind_req->cluster_id, &bind_param->cluster_id);
  bind_req->dst_addr_mode = bind_param->dst_addr_mode;

  if (bind_param->dst_addr_mode == ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT)
  {
    zb_zdo_bind_req_tail_1_t *bind_req_tail_1;
    bind_req_tail_1 = zb_buf_alloc_right(param, sizeof(zb_zdo_bind_req_tail_1_t));
    ZB_HTOLE16((zb_uint8_t*)&bind_req_tail_1->dst_addr, (zb_uint8_t*)&bind_param->dst_address.addr_short);
    TRACE_MSG(TRACE_ZDO3, "dst addr %d", (FMT__D, bind_req_tail_1->dst_addr));
  }
  else if (bind_param->dst_addr_mode == ZB_APS_ADDR_MODE_64_ENDP_PRESENT)
  {
    zb_zdo_bind_req_tail_2_t *bind_req_tail_2;
    bind_req_tail_2 = zb_buf_alloc_right(param, sizeof(zb_zdo_bind_req_tail_2_t));
    ZB_HTOLE64(bind_req_tail_2->dst_addr, bind_param->dst_address.addr_long);
    bind_req_tail_2->dst_endp = bind_param->dst_endp;

    ZB_DUMP_IEEE_ADDR(bind_req_tail_2->dst_addr);
    TRACE_MSG(TRACE_ZDO3, "dst endpoint %hd", (FMT__H, bind_req_tail_2->dst_endp));
  }
  else
  {
    ZB_ASSERT(0);
  }

  ZB_DUMP_IEEE_ADDR(bind_req->src_address);
  TRACE_MSG(TRACE_ZDO3, "src_endpoint %hd, clusterid %d, addr_mode %hd",
    (FMT__H_D_H, bind_req->src_endp, bind_req->cluster_id, bind_req->dst_addr_mode));

  tsn = zdo_send_req_by_short(bind == ZB_TRUE ? ZDO_BIND_REQ_CLID : ZDO_UNBIND_REQ_CLID, param, cb, bind_param->req_dst_addr, ZB_ZDO_CB_DEFAULT_COUNTER);

  TRACE_MSG(TRACE_ZDO2, "<< zb_zdo_bind_unbind_req is_bind: %hd tsn: %hd", (FMT__H_H, bind, tsn));
  return tsn;
}

static void send_bind_unbind_response(zb_uint8_t param,
                                       zb_uint16_t dst_addr,
                                       zb_uint8_t tsn,
                                       zb_uint8_t status,
                                       zb_bool_t bind)
{
  zb_zdo_bind_resp_t *bind_resp;

  TRACE_MSG(TRACE_ZDO3, ">> send_bind_unbind_response, param %d, tsn %d, status 0x%x, bind %d",
            (FMT__D_D_D_D, param, tsn, status, bind));

  bind_resp = zb_buf_initial_alloc(param, sizeof(zb_zdo_bind_resp_t));
  bind_resp->tsn = tsn;
  bind_resp->status = status;
  TRACE_MSG(TRACE_ZDO3, "send resp, dst addr %d status %d", (FMT__D_D, dst_addr, status));

  zdo_send_resp_by_short(bind == ZB_TRUE ? ZDO_BIND_RESP_CLID : ZDO_UNBIND_RESP_CLID,
                         param,
                         dst_addr);

  TRACE_MSG(TRACE_ZDO3, "<< send_bind_unbind_response", (FMT__0));
}


static void handle_bind_unbind_confirm(zb_uint8_t param, zb_bool_t bind)
{
  zb_apsde_data_indication_t *ind;
  zb_uint8_t status;

  TRACE_MSG(TRACE_ZDO3, ">> handle_bind_confirm, param %d, bind %d", (FMT__D_D, param, bind));

  ind = zb_buf_begin(param);

  switch (zb_buf_get_status(param))
  {
    case RET_NOT_FOUND:
      status = ZB_ZDP_STATUS_NOT_SUPPORTED;
      break;
    case ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_INVALID_BINDING):
      status = ZB_ZDP_STATUS_NO_ENTRY;
      break;
    case ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_TABLE_FULL):
      status = ZB_ZDP_STATUS_TABLE_FULL;
      break;
    default:
      status = (zb_uint8_t)zb_buf_get_status(param);
      break;
  }

#ifdef SNCP_MODE
  ncp_apsme_remote_bind_unbind_ind(param, bind);
#endif

  send_bind_unbind_response(param, ind->src_addr, ind->tsn, status, bind);

  TRACE_MSG(TRACE_ZDO3, "<< handle_bind_confirm", (FMT__0));
}


static void handle_bind_confirm(zb_uint8_t param)
{
  handle_bind_unbind_confirm(param, ZB_TRUE);
}


static void handle_unbind_confirm(zb_uint8_t param)
{
  handle_bind_unbind_confirm(param, ZB_FALSE);
}


void zb_zdo_bind_unbind_res(zb_uint8_t param, zb_bool_t bind)
{
  zb_uint8_t tsn;
  zb_uint8_t *aps_body;
  zb_apsde_data_indication_t ind;
  zb_uint16_t dst_addr;
#ifndef ZB_DISABLE_BIND_REQ
  zb_zdo_bind_req_head_t *bind_req;
  zb_apsme_binding_req_t *aps_bind_req;
  zb_uint8_t status = ZB_ZDP_STATUS_SUCCESS;
  zb_size_t expected_len;
#endif
  TRACE_MSG(TRACE_ZDO2, ">> zb_zdo_bind_res param %hd, bind %hd", (FMT__D_H, param, bind));

  ZB_MEMCPY(&ind, ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t), sizeof(ind));

  dst_addr = ind.src_addr;

  aps_body = zb_buf_begin(param);
  tsn = *aps_body;
  aps_body++;

  if (ZB_NWK_IS_ADDRESS_BROADCAST(ind.dst_addr) ||
      (ZB_APS_FC_GET_DELIVERY_MODE(ind.fc) != ZB_APS_DELIVERY_UNICAST) )
  {
    TRACE_MSG(TRACE_ZDO2, "zb_zdo_bind_req/zb_zdo_unbind_req is not unicast", (FMT__0));
    zb_buf_free(param);
    return;
  }

#ifndef ZB_DISABLE_BIND_REQ
  /*cstat !MISRAC2012-Rule-11.3 */
  /** @mdr{00002,66} */
  bind_req = (zb_zdo_bind_req_head_t*)aps_body;
  expected_len = sizeof(*bind_req) + sizeof(tsn);
  switch (bind_req->dst_addr_mode)
  {
    case ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT:
      expected_len += sizeof(aps_bind_req->dst_addr.addr_short);
      break;
    case ZB_APS_ADDR_MODE_64_ENDP_PRESENT:
      expected_len += sizeof(aps_bind_req->dst_addr.addr_long);
      break;
    default:
      TRACE_MSG(TRACE_ERROR, "Unkown dst_addr_mode %hd", (FMT__H, bind_req->dst_addr_mode));
      zb_buf_free(param);
      return;
      break; /* MISRA Rule 16.3 requires switch cases to end with 'break' */
  }

  if (zb_buf_len(param) < expected_len)
  {
    TRACE_MSG(TRACE_ZDO2, "zb_zdo_bind_req/zb_zdo_unbind_req malformed packet!", (FMT__0));
    zb_buf_free(param);
    return;
  }

  aps_body += sizeof(zb_zdo_bind_req_head_t);
  aps_bind_req = ZB_BUF_GET_PARAM(param, zb_apsme_binding_req_t);

  ZB_HTOLE64(aps_bind_req->src_addr, bind_req->src_address);
  aps_bind_req->src_endpoint = bind_req->src_endp;
  ZB_HTOLE16((zb_uint8_t*)&aps_bind_req->clusterid, (zb_uint8_t*)&bind_req->cluster_id);
  aps_bind_req->addr_mode = bind_req->dst_addr_mode;

  if (aps_bind_req->addr_mode == ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT)
  {
    ZB_HTOLE16((zb_uint8_t*)&aps_bind_req->dst_addr.addr_short, aps_body);
    TRACE_MSG(TRACE_ZDO3, "dst addr %d", (FMT__D, aps_bind_req->dst_addr.addr_short));
  }
  else if (aps_bind_req->addr_mode == ZB_APS_ADDR_MODE_64_ENDP_PRESENT)
  {
    ZB_HTOLE64(&aps_bind_req->dst_addr.addr_long, aps_body);
    aps_body += sizeof(zb_ieee_addr_t);
    aps_bind_req->dst_endpoint = *aps_body;
    ZB_DUMP_IEEE_ADDR(aps_bind_req->dst_addr.addr_long);
    TRACE_MSG(TRACE_ZDO3, "dst_endpoint %hd", (FMT__H, aps_bind_req->dst_endpoint));
  }
  else
  {
    /* MISRA rule 15.7 requires empty 'else' branch. */
  }

  ZB_DUMP_IEEE_ADDR(aps_bind_req->src_addr);
  TRACE_MSG(TRACE_ZDO3, "src_endpoint %hd, clusterid %d, addr_mode %hd, dst_endpoint %hd",
    (FMT__H_D_H_H, aps_bind_req->src_endpoint, aps_bind_req->clusterid, aps_bind_req->addr_mode, aps_bind_req->dst_endpoint));

  /* According to 2.4.4.3.2.1 - If the Remote Device is not a Primary binding table cache or the SrcAddress, a Status of NOT_SUPPORTED is returned. */
  if (!ZB_IEEE_ADDR_CMP(ZB_PIBCACHE_EXTENDED_ADDRESS(), aps_bind_req->src_addr))
  {
    TRACE_MSG(TRACE_ZDO3, "invalid src addr - " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(aps_bind_req->src_addr)));
    status = ZB_ZDP_STATUS_NOT_SUPPORTED;
  }

#if 0
  /* 2.4.4.3.2.1.
   * The Simple Descriptor in the receiving device correlating to the
   * endpoint in the Bind_req shall be looked up. If the Simple
   * Descriptor cannot be found then INVALID_EP shall be returned. If
   * the Simple Descriptor is found, it shall be examined to see if
   * the value of the ClusterID field in the Bind_Req message can be
   * found within the Application output cluster list of the Simple
   * Descriptor. If it cannot be found, then INVALID_EP shall be
   * returned. */

  /* [MM]: I've got a couple of questions regarding this
   *
   * Q1: Can "aps_bind_req->src_addr" address differ from the own
   * extended address? If yes, what are the cases? Moreover, upon
   * receipt, the device will be not able to check the simple
   * descriptor for different address.
   * If no, the following simple descriptor check should be done
   * unconditionally.
   *
   * Q2: I expect absolutely the same handling for the unbind request,
   * but it's not described in the r21 spec. Any guess, should it be
   * handled in the same way?
   */

  if (ZB_IEEE_ADDR_CMP(ZB_PIBCACHE_EXTENDED_ADDRESS(), aps_bind_req->src_addr))
  {
    zb_uindex_t i;
    zb_uindex_t j;
    zb_bool_t found = ZB_FALSE;
    zb_uint8_t n_clusters;

    TRACE_MSG(TRACE_ZDO3, "simple desc num %hd", (FMT__H, ZB_ZDO_SIMPLE_DESC_NUMBER()));
    for (i = 0; i < ZB_ZDO_SIMPLE_DESC_NUMBER() && !found; i++)
    {
      TRACE_MSG(TRACE_ZDO3, "endpoint %hd", (FMT__D, ZB_ZDO_SIMPLE_DESC_LIST()[i]->endpoint));
      if (ZB_ZDO_SIMPLE_DESC_LIST()[i]->endpoint == aps_bind_req->src_endpoint)
      {
        n_clusters = ZB_ZDO_SIMPLE_DESC_LIST()[i]->app_input_cluster_count +
          ZB_ZDO_SIMPLE_DESC_LIST()[i]->app_output_cluster_count;

        for (j = 0; j < n_clusters; j++)
        {
          TRACE_MSG(TRACE_ZDO3, "endpoint %hd", (FMT__D, ZB_ZDO_SIMPLE_DESC_LIST()[i]->endpoint));
          if (ZB_ZDO_SIMPLE_DESC_LIST()[i]->app_cluster_list[j] == aps_bind_req->clusterid)
          {
            TRACE_MSG(TRACE_ZDO3, "found ep/cluster_id", (FMT__0));
            found = ZB_TRUE;
            break;
          }
        }
      }
    }

    if (!found)
    {
      TRACE_MSG(TRACE_ZDO3, "ep/cluster_id not found", (FMT__0));
      status = ZB_ZDP_STATUS_INVALID_EP;
    }
  }
#else
  /* CCB 2126:
   * "The Simple Descriptor in the receiving device correlating ..." text is removed.
   * The Bind_rsp is generated in response to a Bind_req. If the Bind_req is processed and the Binding Table
   * entry committed on the Remote Device, a Status of SUCCESS is returned. If the Remote Device is not a
   * Primary binding table cache or the SrcAddress, a Status of NOT_SUPPORTED is returned.
   * The endpoint of the Bind_req shall be checked to determine whether it is between the inclusive range of
   * 0x01 to 0xFE, and if not a Bind_rsp shall be generated with a status of INVALID_EP. */
  if (!(aps_bind_req->src_endpoint > 0U && aps_bind_req->src_endpoint < ZB_ZCL_BROADCAST_ENDPOINT))
  {
    TRACE_MSG(TRACE_ZDO3, "invalid ep - %d", (FMT__D, aps_bind_req->src_endpoint));
    status = ZB_ZDP_STATUS_INVALID_EP;
  }
#endif

#ifdef ZB_ZCL_SUPPORT_CLUSTER_IAS_ZONE
  if (status == ZB_ZDP_STATUS_SUCCESS)
  {
   if (!zb_zcl_ias_zone_check_bind_unbind_request(aps_bind_req))
   {
     TRACE_MSG(TRACE_ZDO3, "invalid CIE src address", (FMT__0));
     status = ZB_ZDP_STATUS_NOT_AUTHORIZED;
   }
  }
#endif

#ifdef SNCP_MODE
  /* set remote bind flag to True since this is a remote binding  */
  if ((status == (zb_uint8_t)ZB_ZDP_STATUS_SUCCESS) &&
      (bind == ZB_TRUE ))
  {
    /* id will be assigned in bind_req */
    aps_bind_req->remote_bind = ZB_TRUE;
  }

  /* Check if partner lk establisment procedure is accepted or not */
  if (SEC_CTX().accept_partner_lk == ZB_FALSE
      && aps_bind_req->clusterid == ZB_ZCL_CLUSTER_ID_KEY_ESTABLISHMENT
  )
  {
    status = ZB_ZDP_STATUS_NOT_AUTHORIZED;
  }
#endif /* SNCP_MODE */

  if (status == (zb_uint8_t)ZB_ZDP_STATUS_SUCCESS)
  {
    zb_uint8_t *p = zb_buf_initial_alloc(param, sizeof(ind));

    /* copy indication info to the buffer body so we can access it in (un)bind confirm cb */
    ZB_MEMCPY(p, &ind, sizeof(ind));

    if (bind == ZB_TRUE)
    {
      aps_bind_req->confirm_cb = handle_bind_confirm;
      zb_apsme_bind_request(param);
    }
    else
    {
      aps_bind_req->confirm_cb = handle_unbind_confirm;
      zb_apsme_unbind_request(param);
    }
  }
  else
#endif
  {
#ifdef ZB_DISABLE_BIND_REQ
    send_bind_unbind_response(param, dst_addr, tsn, ZB_ZDP_STATUS_NOT_SUPPORTED, bind);
#else
    send_bind_unbind_response(param, dst_addr, tsn, status, bind);
#endif /* ZB_DISABLE_BIND_REQ */
  }
  TRACE_MSG(TRACE_ZDO2, "<< zb_zdo_bind_res", (FMT__0));
}

void zb_zdo_add_group_req(zb_uint8_t param)
{
  ZB_SCHEDULE_CALLBACK(zb_apsme_add_group_request, param);
}

void zb_zdo_remove_group_req(zb_uint8_t param)
{
  ZB_SCHEDULE_CALLBACK(zb_apsme_remove_group_request, param);
}

void zb_zdo_remove_all_groups_req(zb_uint8_t param)
{
  ZB_SCHEDULE_CALLBACK(zb_apsme_remove_all_groups_request, param);
}

void zb_zdo_get_group_membership_req(zb_uint8_t param)
{
  ZB_SCHEDULE_CALLBACK(zb_apsme_get_group_membership_request, param);
}

static void zb_get_peer_short_addr_cb(zb_uint8_t param)
{
  zb_zdo_nwk_addr_resp_head_t *resp;
  zb_ieee_addr_t ieee_addr;
  zb_uint16_t nwk_addr;
  zb_address_ieee_ref_t addr_ref;

  TRACE_MSG(TRACE_ZDO2, "zb_get_peer_short_addr_cb param %hd", (FMT__H, param));

  resp = (zb_zdo_nwk_addr_resp_head_t*)zb_buf_begin(param);
  TRACE_MSG(TRACE_ZDO2, "resp status %hd, nwk addr %d", (FMT__H_D, resp->status, resp->nwk_addr));
  ZB_DUMP_IEEE_ADDR(resp->ieee_addr);
  if (resp->status == (zb_uint8_t)ZB_ZDP_STATUS_SUCCESS)
  {
    zb_ret_t ret;

    ZB_LETOH64(ieee_addr, resp->ieee_addr);
    ZB_LETOH16((zb_uint8_t *)&nwk_addr, (zb_uint8_t *)&resp->nwk_addr);
    ret = zb_address_update(ieee_addr, nwk_addr, ZB_TRUE, &addr_ref);
    if (ret != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "zb_address_update failed [%d]", (FMT__D, ret));
    }
  }
  TRACE_MSG(TRACE_ZDO2, "schedule cb %p param %hd",
            (FMT__P_H, ZDO_CTX().zdo_ctx.get_short_addr_ctx.cb, ZDO_CTX().zdo_ctx.get_short_addr_ctx.param));
  if (ZDO_CTX().zdo_ctx.get_short_addr_ctx.cb != NULL)
  {
    TRACE_MSG(TRACE_NWK3, "zb_get_peer_short_addr_cb: call callback", (FMT__0));
    ZB_SCHEDULE_CALLBACK(ZDO_CTX().zdo_ctx.get_short_addr_ctx.cb, ZDO_CTX().zdo_ctx.get_short_addr_ctx.param);
  }

  zb_buf_free(param);
}

/*
   Calls zb_zdo_nwk_addr_req to get peer network address
   param param - buffer reference to use for i/o
 */

static void zb_get_peer_short_addr(zb_uint8_t param)
{
  zb_zdo_nwk_addr_req_param_t *req_param;

  TRACE_MSG(TRACE_ZDO2, "zb_get_peer_short_addr param %hd", (FMT__H, param));

  req_param = ZB_BUF_GET_PARAM(param, zb_zdo_nwk_addr_req_param_t);

  req_param->dst_addr = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
  zb_address_ieee_by_ref(req_param->ieee_addr, ZDO_CTX().zdo_ctx.get_short_addr_ctx.dst_addr_ref);
  req_param->request_type = ZB_ZDO_SINGLE_DEVICE_RESP;
  req_param->start_index = 0;

  if (zb_zdo_nwk_addr_req(param, zb_get_peer_short_addr_cb) == ZB_ZDO_INVALID_TSN)
  {
    zb_zdo_nwk_addr_resp_head_t *resp;

    resp = zb_buf_initial_alloc(param, sizeof(zb_zdo_nwk_addr_resp_head_t));
    resp->tsn = ZB_ZDO_INVALID_TSN;
    resp->status = ZB_ZDP_STATUS_INSUFFICIENT_SPACE;
    TRACE_MSG(TRACE_ERROR, "no mem space for zb_zdo_nwk_addr_req", (FMT__0));
    ZB_SCHEDULE_CALLBACK(zb_get_peer_short_addr_cb, param);
  }
}

void zb_start_get_peer_short_addr(zb_address_ieee_ref_t dst_addr_ref, zb_callback_t cb, zb_uint8_t param)
{
  zb_ret_t ret;

  TRACE_MSG(TRACE_ZDO2, "zb_start_get_peer_short_addr dst_addr_ref %hd", (FMT__H, dst_addr_ref));
  ZDO_CTX().zdo_ctx.get_short_addr_ctx.dst_addr_ref = dst_addr_ref;
  ZDO_CTX().zdo_ctx.get_short_addr_ctx.cb = cb;
  ZDO_CTX().zdo_ctx.get_short_addr_ctx.param = param;

  ret = zb_buf_get_out_delayed(zb_get_peer_short_addr);
  if (ret != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
  }
}

#ifndef R23_DISABLE_DEPRECATED_ZDO_CMDS
#ifndef ZB_LITE_NO_END_DEVICE_BIND
static void copy_end_device_bind_req_head(zb_zdo_end_device_bind_req_head_t *dst_head, zb_zdo_end_device_bind_req_head_t *src_head)
{
  TRACE_MSG(TRACE_ZDO2, "copy_end_device_bind_req_head", (FMT__0));
  ZB_HTOLE16((zb_uint8_t*)&dst_head->binding_target, (zb_uint8_t*)&src_head->binding_target);
  ZB_HTOLE64(dst_head->src_ieee_addr, src_head->src_ieee_addr);
  dst_head->src_endp = src_head->src_endp;
  ZB_HTOLE16((zb_uint8_t*)&dst_head->profile_id, (zb_uint8_t*)&src_head->profile_id);
  dst_head->num_in_cluster = src_head->num_in_cluster;
}

zb_uint8_t zb_end_device_bind_req(zb_uint8_t param, zb_callback_t cb)
{
  zb_end_device_bind_req_param_t *req_param_init;
  zb_end_device_bind_req_param_t *req_param;
  zb_zdo_end_device_bind_req_head_t *req_head;
  zb_zdo_end_device_bind_req_tail_t *req_tail;
  zb_size_t payload_length;
  zb_size_t num_in_out_cluster;

  TRACE_MSG(TRACE_ZDO2, "zb_end_device_bind_req, param %hd", (FMT__H, param));

  req_param_init = (zb_end_device_bind_req_param_t*)zb_buf_begin(param);
  num_in_out_cluster = ((zb_size_t)req_param_init->head_param.num_in_cluster + (zb_size_t)req_param_init->tail_param.num_out_cluster);
  payload_length = sizeof(zb_end_device_bind_req_param_t) +
        (num_in_out_cluster) * sizeof(zb_uint16_t);
  req_param = zb_buf_get_tail(param, payload_length);

  ZB_MEMCPY(req_param, req_param_init, payload_length);

  num_in_out_cluster = ((zb_size_t)req_param->head_param.num_in_cluster + (zb_size_t)req_param->tail_param.num_out_cluster);
  req_head = zb_buf_initial_alloc(param, sizeof(zb_zdo_end_device_bind_req_head_t) + sizeof(zb_zdo_end_device_bind_req_tail_t) +
                       (num_in_out_cluster) * (zb_uint8_t)sizeof(zb_uint16_t));
  copy_end_device_bind_req_head(req_head, &req_param->head_param);
  (void)zb_copy_cluster_id((zb_uint8_t*)(req_head + 1), (zb_uint8_t *)req_param->cluster_list, req_param->head_param.num_in_cluster);
  /* Casting from zb_zdo_end_device_bind_req_head_t * to packed struct zb_zdo_end_device_bind_req_tail_t * via interim pointer to void
   * to avoid MISRA 11.3 violation. There is no misalignment */
  req_tail = (zb_zdo_end_device_bind_req_tail_t*)(void *)( (zb_uint8_t*)(req_head + 1) +
                                                         sizeof(zb_uint16_t) *req_param->head_param.num_in_cluster);

  req_tail->num_out_cluster = req_param->tail_param.num_out_cluster;
  (void)zb_copy_cluster_id((zb_uint8_t*)(req_tail + 1), (zb_uint8_t *)(req_param->cluster_list + req_param->head_param.num_in_cluster),
                  req_param->tail_param.num_out_cluster);
  TRACE_MSG(TRACE_ZDO3, "binding_target %d, src_endp %hd, profile_id %d, num_in_cluster %hd, num_out_cluster %hd",
            (FMT__D_H_D_H_H, req_head->binding_target, req_head->src_endp, req_head->profile_id, req_head->num_in_cluster,
             req_tail->num_out_cluster));
  ZB_DUMP_IEEE_ADDR(req_head->src_ieee_addr);

  return zdo_send_req_by_short(ZDO_END_DEVICE_BIND_REQ_CLID, param, cb, req_param->dst_addr, ZB_ZDO_CB_DEFAULT_COUNTER);
}

static void zb_zdo_end_device_bind_timer(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO2, "zb_zdo_end_device_bind_timer param %hd", (FMT__H, param));
  ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZB_ZDO_BIND_DEV_1].end_device_bind_param = ZB_UNDEFINED_BUFFER;
  ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZB_ZDO_BIND_DEV_2].end_device_bind_param = ZB_UNDEFINED_BUFFER;
  zb_zdo_end_device_bind_resp(param, ZB_ZDP_STATUS_TIMEOUT);
}

#ifndef ZB_DISABLE_ED_BIND_REQ
static void check_cluster_list(zb_uint8_t device_num,
                               zb_uint8_t *cluster_list1, zb_uint8_t num_cluster1,
                               zb_uint8_t *cluster_list2, zb_uint8_t num_cluster2)
{
  zb_uindex_t i;
  zb_uindex_t j;
  zb_uint16_t cluster_id;

  TRACE_MSG(TRACE_ZDO3, "check cluster device num %hd, list1 %p, num1 %hd, list2 %p, num2 %hd",
            (FMT__H_P_H_P_H, device_num, (zb_uint8_t *)cluster_list1, num_cluster1, (zb_uint8_t *)cluster_list2, num_cluster2));

  ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[device_num].cluster_num = 0;
  for(i = 0; i < num_cluster1; i++)
  {
    for (j =0; j < num_cluster2; j++)
    {
      if (ZB_MEMCMP(cluster_list1 + i * sizeof(zb_uint16_t), cluster_list2 + j * sizeof(zb_uint16_t), sizeof(zb_uint16_t)) == 0)
      {
        /* convert endian - it was not converted on receive */
        ZB_LETOH16(&cluster_id, cluster_list1 + i * sizeof(zb_uint16_t));
        ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[device_num].cluster_list[
          ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[device_num].cluster_num++] = cluster_id;

        if (ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[device_num].cluster_num >= ZB_ZDO_MAX_CLUSTER_LIST)
        {
          TRACE_MSG(TRACE_ZDO2, "max cluster num exceeded!", (FMT__0));
          break;
        }
        TRACE_MSG(TRACE_ZDO2, "add cluster id %d, num %hd",
                  (FMT__D_H, cluster_id, ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[device_num].cluster_num));
      }
    }
  }
}
#endif

#endif  /* ZB_LITE_NO_END_DEVICE_BIND */


void zb_zdo_end_device_bind_handler(zb_uint8_t param)
{
#if defined ZB_DISABLE_ED_BIND_REQ || defined ZB_LITE_NO_END_DEVICE_BIND
  zb_zdo_end_device_bind_resp(param, ZB_ZDP_STATUS_NOT_SUPPORTED);
#else
  zb_ret_t ret;
  zb_zdo_end_device_bind_req_head_t *req_head1;
  zb_zdo_end_device_bind_req_head_t *req_head2;
  zb_zdo_end_device_bind_req_tail_t *req_tail1;
  zb_zdo_end_device_bind_req_tail_t *req_tail2;
  zb_uint8_t *aps_body;
  zb_zdp_status_t status = ZB_ZDP_STATUS_SUCCESS;
  zb_bool_t is_malformed;
  zb_size_t expected_len;
  zb_uint8_t *tmp;

  TRACE_MSG(TRACE_ZDO2, "zb_zdo_end_device_bind_handler, param %hd", (FMT__H, param));

  aps_body = zb_buf_begin(param);
  /*cstat !MISRAC2012-Rule-11.3 */
  /** @mdr{00002,67} */
  req_head2 = (zb_zdo_end_device_bind_req_head_t*)(aps_body + 1);
  expected_len  = sizeof(zb_uint8_t) + sizeof(zb_zdo_end_device_bind_req_head_t)
                                     + sizeof(zb_zdo_end_device_bind_req_tail_t);

  is_malformed = (zb_bool_t)(zb_buf_len(param) < expected_len);
  if (!is_malformed)
  {
    expected_len += sizeof(zb_uint16_t) * req_head2->num_in_cluster;
    is_malformed = (zb_bool_t)(zb_buf_len(param) < expected_len);
  }

  if (!is_malformed)
  {
    tmp = (zb_uint8_t *)(req_head2 + 1) + sizeof(zb_uint16_t) * req_head2->num_in_cluster;
    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,68} */
    req_tail2 = (zb_zdo_end_device_bind_req_tail_t *)tmp;
    TRACE_MSG(TRACE_ZDO2, "mf %d %d", (FMT__D_D, req_head2->num_in_cluster, req_tail2->num_out_cluster));

    expected_len += sizeof(zb_uint16_t) * req_tail2->num_out_cluster;
    is_malformed = (zb_bool_t)(zb_buf_len(param) < expected_len);
  }

  if (is_malformed)
  {
    TRACE_MSG(TRACE_ZDO2, "malformed zb_zdo_end_device_bind packet", (FMT__0));
    zb_buf_free(param);
    return;
  }

  TRACE_MSG(TRACE_ZDO3, "endp %hd, end_device_bind_param[0] %hd end_device_bind_param[1] %hd",
            (FMT__H_H_H, req_head2->src_endp, ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZB_ZDO_BIND_DEV_1].end_device_bind_param,
             ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZB_ZDO_BIND_DEV_2].end_device_bind_param));
  if (req_head2->src_endp < ZB_MIN_ENDPOINT_NUMBER || req_head2->src_endp > ZB_MAX_ENDPOINT_NUMBER)
  {
    /* send resp with fail status */
    zb_zdo_end_device_bind_resp(param, ZB_ZDP_STATUS_INVALID_EP);
  }
  else
  {
    if (ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZB_ZDO_BIND_DEV_1].end_device_bind_param == ZB_UNDEFINED_BUFFER)
    {
      /* Schedule time out and wait for the second end device bund request */
      ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZB_ZDO_BIND_DEV_1].end_device_bind_param = param;
      ZB_SCHEDULE_ALARM(zb_zdo_end_device_bind_timer, param, ZDO_CTX().conf_attr.enddev_bind_timeout * ZB_TIME_ONE_SECOND);
      zb_nwk_unlock_in(param);
    }
    else if (ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZB_ZDO_BIND_DEV_2].end_device_bind_param != ZB_UNDEFINED_BUFFER)
    {
      /* bind end device process is in progress, so return error to
       * this new request. Spec doesn't have BUSY error code, so use TIMEOUT */
      TRACE_MSG(TRACE_ZDO3, "return error TIMEOUT status", (FMT__0));
      zb_zdo_end_device_bind_resp(param, ZB_ZDP_STATUS_TIMEOUT);
    }
    else
    {
      zb_bufid_t buf1 = ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZB_ZDO_BIND_DEV_1].end_device_bind_param;

      ZB_SCHEDULE_ALARM_CANCEL(zb_zdo_end_device_bind_timer, ZB_ALARM_ALL_CB);
      aps_body = zb_buf_begin(buf1);
      /*cstat !MISRAC2012-Rule-11.3 */
      /** @mdr{00002,69} */
      req_head1 = (zb_zdo_end_device_bind_req_head_t*)(aps_body+1);
      /* both requests req_head1 and req_head2 has not converted
       * LETOH parameters - compare it without conversion */
      TRACE_MSG(TRACE_ZDO3, "req1 binding_target %d, src_endp %hd, profile_id %d, num_in_cluster %hd",
                (FMT__D_H_D_H, req_head1->binding_target, req_head1->src_endp, req_head1->profile_id, req_head1->num_in_cluster));

      TRACE_MSG(TRACE_ZDO3, "req2 binding_target %d, src_endp %hd, profile_id %d, num_in_cluster %hd",
                (FMT__D_H_D_H, req_head2->binding_target, req_head2->src_endp, req_head2->profile_id, req_head2->num_in_cluster));

      if (req_head1->profile_id != req_head2->profile_id
        && req_head1->profile_id != 0xFFFFU
        && req_head2->profile_id != 0xFFFFU)
      {
        /* 2.4.3.2.1 of r22:
         * If the ProfileID does not match and if the Profile Id is not 0xFFFF, or <...>,
         * a status of NO_MATCH shall be supplied to both Local Devices
         */

        status = ZB_ZDP_STATUS_NO_MATCH;
      }
      else
      {
        tmp = (zb_uint8_t*)req_head1 + sizeof(zb_zdo_end_device_bind_req_head_t) + req_head1->num_in_cluster * sizeof(zb_uint16_t);
        /*cstat !MISRAC2012-Rule-11.3 */
        /** @mdr{00002,70} */
        req_tail1 = (zb_zdo_end_device_bind_req_tail_t*)tmp;
        tmp = (zb_uint8_t*)req_head2 + sizeof(zb_zdo_end_device_bind_req_head_t) + req_head2->num_in_cluster * sizeof(zb_uint16_t);
        /*cstat !MISRAC2012-Rule-11.3 */
        /** @mdr{00002,71} */
        req_tail2 = (zb_zdo_end_device_bind_req_tail_t*)tmp;
        TRACE_MSG(TRACE_ZDO3, "req1 num_out_cluster %hd, req2 num_out_cluster %hd",
                  (FMT__H_H, req_tail1->num_out_cluster, req_tail2->num_out_cluster));

        check_cluster_list(ZB_ZDO_BIND_DEV_1,
                           (zb_uint8_t*)(req_tail1 + 1) /* req1 out cluster list */, req_tail1->num_out_cluster,
                           (zb_uint8_t*)(req_head2 + 1) /* req2 in cluster list */, req_head2->num_in_cluster);
        check_cluster_list(ZB_ZDO_BIND_DEV_2,
                           (zb_uint8_t*)(req_head1 + 1) /* req1 in cluster list */, req_head1->num_in_cluster,
                           (zb_uint8_t*)(req_tail2 + 1) /* req2 out cluster list */, req_tail2->num_out_cluster);

        if (ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZB_ZDO_BIND_DEV_1].cluster_num == 0U &&
            ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZB_ZDO_BIND_DEV_2].cluster_num == 0U)
        {
          status = ZB_ZDP_STATUS_NO_MATCH;
        }
      } /* else, profile_id */

      if (status == ZB_ZDP_STATUS_SUCCESS)
      {
        /* Will send bind/unbind requests to devices binding_target1,
         * binding_target2. We have only local binding tables, so
         * binding_target address equal to end_device_bind_req src
         * address and src_ieee_addr is ieee address of the
         * binding_target - do not save it. If binding table is on the
         * primary binding cahce device, we'll need to save
         * src_ieee_addr also  */

        ZB_LETOH16((zb_uint8_t*)&ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZB_ZDO_BIND_DEV_1].binding_target, (zb_uint8_t*)&req_head1->binding_target);
        ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZB_ZDO_BIND_DEV_1].src_endp = req_head1->src_endp;

        ZB_LETOH16((zb_uint8_t*)&ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZB_ZDO_BIND_DEV_2].binding_target, (zb_uint8_t*)&req_head2->binding_target);
        ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZB_ZDO_BIND_DEV_2].src_endp = req_head2->src_endp;
        ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZB_ZDO_BIND_DEV_2].end_device_bind_param = param;

        TRACE_MSG(TRACE_ZDO3, "save ctx dev1 %d, endp %hd, param %hd, cluster_num %hd",
                  (FMT__D_H_H_H,
                   ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZB_ZDO_BIND_DEV_1].binding_target,
                   ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZB_ZDO_BIND_DEV_1].src_endp,
                   ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZB_ZDO_BIND_DEV_1].end_device_bind_param,
                   ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZB_ZDO_BIND_DEV_1].cluster_num));
        TRACE_MSG(TRACE_ZDO3, "save ctx dev2 %d, endp %hd, param %hd, cluster_num %hd",
                  (FMT__D_H_H_H,
                   ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZB_ZDO_BIND_DEV_2].binding_target,
                   ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZB_ZDO_BIND_DEV_2].src_endp,
                   ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZB_ZDO_BIND_DEV_2].end_device_bind_param,
                   ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZB_ZDO_BIND_DEV_2].cluster_num));

        ret = zb_buf_get_out_delayed(zb_zdo_send_check_bind_unbind);
        status = (ret == RET_OK) ? ZB_ZDP_STATUS_SUCCESS : ZB_ZDP_STATUS_INSUFFICIENT_SPACE;
      }
      /* send response to both devices */
      zb_zdo_end_device_bind_resp(param, status);
      zb_zdo_end_device_bind_resp(ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZB_ZDO_BIND_DEV_1].end_device_bind_param, status);
      if (status != ZB_ZDP_STATUS_SUCCESS)
      {
        TRACE_MSG(TRACE_ZDO3, "no match found", (FMT__0));
        ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZB_ZDO_BIND_DEV_1].end_device_bind_param = ZB_UNDEFINED_BUFFER;
        ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZB_ZDO_BIND_DEV_2].end_device_bind_param = ZB_UNDEFINED_BUFFER;
      }
    } /* else, end_device_bind_param[ZB_ZDO_BIND_DEV_1] */
  }
#endif  /* ZB_LITE_NO_END_DEVICE_BIND */
}


#ifndef ZB_LITE_NO_END_DEVICE_BIND
void zb_zdo_send_check_bind_unbind(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO3, "zb_zdo_send_check_bind_unbind param %hd", (FMT__H, param));

  if (ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZB_ZDO_BIND_DEV_1].cluster_num != 0U)
  {
    ZDO_CTX().zdo_ctx.end_device_bind_ctx.current_device = ZB_ZDO_BIND_DEV_1;
  }
  else
  {
    ZDO_CTX().zdo_ctx.end_device_bind_ctx.current_device = ZB_ZDO_BIND_DEV_2;
  }
  /*
    Send check unbind for the last cluster to exclude its second unbind attempt
  */
  if (send_bind_unbind_req(param, ZDO_CTX().zdo_ctx.end_device_bind_ctx.current_device,
                           ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZDO_CTX().zdo_ctx.end_device_bind_ctx.current_device].cluster_list[
                             ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZDO_CTX().zdo_ctx.end_device_bind_ctx.current_device].cluster_num - 1U],
                           zb_zdo_bind_unbind_check_cb, ZB_FALSE) == ZB_ZDO_INVALID_TSN)
  {
    zb_zdo_bind_resp_t *resp;

    resp = zb_buf_initial_alloc(param, sizeof(zb_zdo_bind_resp_t));
    resp->tsn = ZB_ZDO_INVALID_TSN;
    resp->status = ZB_ZDP_STATUS_INSUFFICIENT_SPACE;
    TRACE_MSG(TRACE_ERROR, "no mem space for zb_zdo_bind_resp_t", (FMT__0));
    ZB_SCHEDULE_CALLBACK(zb_zdo_bind_unbind_check_cb, param);
  }
}

static zb_uint8_t send_bind_unbind_req(zb_uint8_t param, zb_uint8_t target_dev_num, zb_uint16_t cluster_id, zb_callback_t cb, zb_bool_t bind)
{
  zb_ret_t ret;
  zb_zdo_bind_req_param_t *bind_param;
  zb_uint8_t peer_dev_num = ZB_ZDO_PEER_DEVICE_NUM(target_dev_num);
  zb_uint8_t zdo_tsn;

  TRACE_MSG(TRACE_ZDO3, "send_bind_unbind_req param %hd, dev num %hd, peer %hd, cluster_id %d, bind %hd, cb %p",
            (FMT__H_H_H_D_H_P, param, target_dev_num, peer_dev_num, cluster_id, (zb_uint8_t)bind, cb));

  (void)zb_buf_reuse(param);
  bind_param = ZB_BUF_GET_PARAM(param, zb_zdo_bind_req_param_t);
  ret = zb_address_ieee_by_short(ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[target_dev_num].binding_target,
                                 bind_param->src_address);
  if (ret != RET_OK)
  {
    return ZB_ZDO_INVALID_TSN;
  }
  bind_param->src_endp = ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[target_dev_num].src_endp;

  bind_param->cluster_id = cluster_id;
  bind_param->dst_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;

  ret = zb_address_ieee_by_short(ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[peer_dev_num].binding_target,
                                 bind_param->dst_address.addr_long);
  if (ret != RET_OK)
  {
    return ZB_ZDO_INVALID_TSN;
  }

  bind_param->dst_endp = ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[peer_dev_num].src_endp;

  bind_param->req_dst_addr = ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[target_dev_num].binding_target;
  TRACE_MSG(TRACE_COMMON1, "dst addr %d", (FMT__D, bind_param->req_dst_addr));

  if (bind)
  {
    zdo_tsn = zb_zdo_bind_req(param, cb);
  }
  else
  {
    zdo_tsn = zb_zdo_unbind_req(param, cb);
  }
  return zdo_tsn;
}

static void send_bind_unbind_with_check(zb_uint8_t param, zb_bool_t bind, zb_callback_t cb)
{
  TRACE_MSG(TRACE_ZDO2, "send_bind_unbind_with_check param %hd, bind %hd, cur device %hd, cluster_num %hd",
            (FMT__H_H_H_H, param, bind, ZDO_CTX().zdo_ctx.end_device_bind_ctx.current_device,
             ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZDO_CTX().zdo_ctx.end_device_bind_ctx.current_device].cluster_num));

  if (ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZDO_CTX().zdo_ctx.end_device_bind_ctx.current_device].cluster_num == 0U)
  {
    ZDO_CTX().zdo_ctx.end_device_bind_ctx.current_device++;
    if (ZDO_CTX().zdo_ctx.end_device_bind_ctx.current_device == ZB_ZDO_BIND_DEV_2)
    {
      ZB_SCHEDULE_CALLBACK(cb, param);
    }
    else
    {
      TRACE_MSG(TRACE_ZDO2, "All bind req are sent, free buf", (FMT__0));
      ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZB_ZDO_BIND_DEV_1].end_device_bind_param = ZB_UNDEFINED_BUFFER;
      ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZB_ZDO_BIND_DEV_2].end_device_bind_param = ZB_UNDEFINED_BUFFER;
      zb_buf_free(param);
    }
  }
  else
  {
    if (send_bind_unbind_req(param, ZDO_CTX().zdo_ctx.end_device_bind_ctx.current_device,
                             ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZDO_CTX().zdo_ctx.end_device_bind_ctx.current_device].cluster_list[
                               ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZDO_CTX().zdo_ctx.end_device_bind_ctx.current_device].cluster_num - 1U],
                             cb, bind) == ZB_ZDO_INVALID_TSN)
    {
      zb_buf_free(param);
    }
    else
    {
      ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZDO_CTX().zdo_ctx.end_device_bind_ctx.current_device].cluster_num--;
    }
  }
}

void zb_zdo_bind_unbind_check_cb(zb_uint8_t param)
{
  zb_zdo_bind_resp_t *resp = (zb_zdo_bind_resp_t*)zb_buf_begin(param);
  zb_bool_t bind_req;
  zb_callback_t cb;

  TRACE_MSG(TRACE_ZDO2, "zb_zdo_bind_unbind_check_cb resp param %hd, status %hd", (FMT__H_H, param, resp->status));

  if (resp->status == (zb_uint8_t)ZB_ZDP_STATUS_NO_ENTRY)
  {
    TRACE_MSG(TRACE_COMMON1, "will do bind", (FMT__0));
    bind_req = ZB_TRUE;
    cb = zb_zdo_end_device_bind_cb;
  }
  else
  {
    TRACE_MSG(TRACE_COMMON1, "will do unbind", (FMT__0));
    bind_req = ZB_FALSE;
    cb = zb_zdo_end_device_unbind_cb;
    /* Note: one cluster is already unmound - that is
                         ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZDO_CTX().zdo_ctx.end_device_bind_ctx.current_device].cluster_list[
                           ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZDO_CTX().zdo_ctx.end_device_bind_ctx.current_device].cluster_num - 1],

       See zb_zdo_send_check_bind_unbind()
    */
    ZDO_CTX().zdo_ctx.end_device_bind_ctx.bind_device_info[ZDO_CTX().zdo_ctx.end_device_bind_ctx.current_device].cluster_num--;
  }
  send_bind_unbind_with_check(param, bind_req, cb);
}

void zb_zdo_end_device_bind_cb(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO2, "zb_zdo_end_device_bind_cb param %hd", (FMT__H, param));
  send_bind_unbind_with_check(param, ZB_TRUE, zb_zdo_end_device_bind_cb);
}

void zb_zdo_end_device_unbind_cb(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO2, "zb_zdo_end_device_unbind_cb param %hd", (FMT__H, param));
  send_bind_unbind_with_check(param, ZB_FALSE, zb_zdo_end_device_unbind_cb);
}

#endif  /* ZB_LITE_NO_END_DEVICE_BIND */

void zb_zdo_end_device_bind_resp(zb_uint8_t param, zb_zdp_status_t status)
{
  zb_zdo_end_device_bind_resp_t *resp;
  zb_apsde_data_indication_t *ind;
  zb_uint8_t tsn;

  TRACE_MSG(TRACE_ZDO2, "zb_zdo_end_device_bind_resp param %hd, status %hd", (FMT__H_H, param, status));

  ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);
  tsn = *(zb_uint8_t*)zb_buf_begin(param);

  resp = zb_buf_initial_alloc(param, sizeof(zb_zdo_end_device_bind_resp_t));
  resp->tsn = tsn;
  resp->status = (zb_uint8_t)status;

  TRACE_MSG(TRACE_ZDO3, "send resp, dst addr %d, tsn %hd", (FMT__D_H, ind->src_addr, tsn));
  zdo_send_resp_by_short(ZDO_END_DEVICE_BIND_RESP_CLID, param, ind->src_addr);
}
#endif /* R23_DISABLE_DEPRECATED_ZDO_CMDS */

#if 0
zb_bool_t zb_zdo_find_bind_record(zb_uint8_t src_end, zb_uint16_t cluster_id)
{
  zb_bool_t bind_entries_found = ZB_TRUE;
  zb_address_ieee_ref_t addr_ref;
  zb_int8_t lnk=0;

  TRACE_MSG(TRACE_APS1, "+zb_zdo_find_bind_record %hd, cluster %d", (FMT__H_D, src_end, cluster_id));
  if (RET_OK == zb_address_by_short(ZB_PIBCACHE_NETWORK_ADDRESS(), ZB_FALSE, ZB_FALSE, &addr_ref))
  {
    lnk = aps_find_src_ref(addr_ref, src_end, cluster_id);
    if(lik >= 0)
    {
      aps_find_dst_record(lnk, , zb_aps_bind_dst_record_t *dst_record)
    }
  }
}

#endif
/*! @} */
