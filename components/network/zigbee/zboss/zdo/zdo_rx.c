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
/* PURPOSE: ZDO RX path
*/


#define ZB_TRACE_FILE_ID 2101
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zdo_common.h"
#include "zdo_wwah_stubs.h"
/*! \addtogroup ZB_ZDO */
/*! @{ */

#ifdef ZB_ROUTER_ROLE
static void zdo_device_annce_srv(zb_uint8_t param, void *dt);
static void zb_parent_annce_resp_handler(zb_uint8_t param, void *dt);

#if (defined ZB_JOINING_LIST_SUPPORT) && defined(ZB_ROUTER_ROLE)
static void zb_nwk_ieee_joining_list_resp_handler(zb_uint8_t param);
#endif /* (defined ZB_JOINING_LIST_SUPPORT) && defined(ZB_ROUTER_ROLE) */
#ifdef ZB_ENABLE_ZGP
void zgp_handle_dev_annce(zb_zdo_device_annce_t *da);
#endif /* ZB_ENABLE_ZGP */

#endif /* ZB_ROUTER_ROLE */

void zb_zdo_data_indication(zb_uint8_t param)
{
  zb_apsde_data_indication_t *ind = (zb_apsde_data_indication_t *)ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);
#ifdef ZB_ROUTER_ROLE
  zb_uint8_t *body;
  zb_uint16_t src;
#endif
  zb_uint8_t skip_free_buf = 1;
  zb_ret_t ret;
  zb_uint8_t fc;
  zb_uint8_t tsn;
  zb_uint16_t clusterid;
  ZB_TH_PUSH_PACKET(ZB_TH_ZDO_DATA, ZB_TH_PRIMITIVE_INDICATION, param);
  TRACE_MSG(TRACE_ZDO1, ">> zb_zdo_data_indication %hd clu 0x%hx", (FMT__H_H, param, ind->clusterid));
  fc = ind->fc; /* APS FC is needed in some response functions */
#ifdef ZB_ROUTER_ROLE
  body = zb_buf_begin(param);
  src = ind->src_addr;
#endif

  /* Put TSN to apsde_data_ind. */
  tsn = *(zb_uint8_t *)zb_buf_begin(param);
  ind->tsn = tsn;
  clusterid = ind->clusterid;

  /*cstat !MISRAC2012-Rule-14.3_b */
  /** @mdr{00015,8} */
  /* disable processing of sensitive ZDO commands that have not been encrypted using the
   * Trust Center Link key */
  if (!ZB_ZDO_CHECK_ZDO_COMMAND(ind))
  /*cstat !MISRAC2012-Rule-2.1_b */
  /** @mdr{00015,9} */
  {
    TRACE_MSG(TRACE_ZDO3, "Not authorized - disable processing", (FMT__0));
    skip_free_buf = 0;
  }
  if ((ZB_ZDO_IS_CLI_CMD(ind->clusterid)) && (skip_free_buf == 1U))
  {
#ifdef ZB_CERTIFICATION_HACKS
    if (ZB_CERT_HACKS().pass_incoming_zdo_cmd_to_app &&
        ZB_CERT_HACKS().zdo_af_handler_cb &&
        ZB_CERT_HACKS().zdo_af_handler_cb(param, ind->clusterid))
    {
      TRACE_MSG(TRACE_ZDO3, "Incoming ZDO command is handled by application", (FMT__0));
    }
    else
#endif /* ZB_CERTIFICATION_HACKS */
    /* Process client commands */
#ifdef ZB_ROUTER_ROLE
    if (ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE())
        && (ind->clusterid == ZDO_DEVICE_ANNCE_CLID))
    {
      zdo_device_annce_srv(param, (void *)(body + 0));
    }
    else if (ZB_IS_DEVICE_ZC_OR_ZR()
             && (ind->clusterid == ZDO_PARENT_ANNCE_CLID))
    {
      ret = zb_buf_get_out_delayed_ext(zdo_parent_annce_handler, param, 0);
      if (ret != RET_OK)
      {
        skip_free_buf = 0;
        TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
      }
    }
    else
#endif
        /* #AT descr_req OR power_descr_req */
    if ((ind->clusterid == ZDO_NODE_DESC_REQ_CLID) || (ind->clusterid == ZDO_POWER_DESC_REQ_CLID))
    {
      zdo_send_desc_resp(param);
    } else if (ind->clusterid == ZDO_SIMPLE_DESC_REQ_CLID)
    {
      zdo_send_simple_desc_resp(param);
    } else if (ind->clusterid == ZDO_NWK_ADDR_REQ_CLID)
    {
      zdo_device_nwk_addr_res(param, fc);
    } else if (ind->clusterid == ZDO_IEEE_ADDR_REQ_CLID)
    {
      zdo_device_ieee_addr_res(param, fc);
    } else if (ind->clusterid == ZDO_ACTIVE_EP_REQ_CLID)
    {
      zdo_active_ep_res(param);
    } else if (ind->clusterid == ZDO_MATCH_DESC_REQ_CLID)
    {
      zdo_match_desc_res(param);
    }
/* Mgmt NWK update handlers were under ZB_ROUTER_ROLE ifdef,
 * but were switched on for all devices types (for WWAH, PICS item AZD514) */
    /*
r22 PICS AZD802
AZD36:
FDT1: M
FDT2: X
FDT3: X

The ability for a non Network Channel Manager to receive and process
the Mgmt_NWK_Update_-req command is mandatory for the network manager
and all routers and optional for end devices.
     */
    else if (ind->clusterid == ZDO_MGMT_NWK_UPDATE_REQ_CLID)
    {
      zb_zdo_mgmt_nwk_update_handler(param);
    }
#ifdef ZB_MGMT_NWK_ENHANCED_UPDATE_ENABLED
    else if (ind->clusterid == ZDO_MGMT_NWK_ENHANCED_UPDATE_REQ_CLID)
    {
      zb_zdo_mgmt_nwk_enhanced_update_handler(param);
    }
#endif  /*  ZB_MGMT_NWK_ENHANCED_UPDATE_ENABLED */
#if !defined(ZB_LITE_NO_ZDO_SYSTEM_SERVER_DISCOVERY)
    else if (ind->clusterid == ZDO_SYSTEM_SERVER_DISCOVERY_REQ_CLID)
    {
      zdo_system_server_discovery_res(param);
    }
#endif
    else if (ind->clusterid == ZDO_MGMT_LQI_REQ_CLID)
    {
      zdo_lqi_resp(param);
    }
    else if (ind->clusterid == ZDO_MGMT_BIND_REQ_CLID)
    {
      zdo_mgmt_bind_resp(param);
    } else if (ind->clusterid == ZDO_MGMT_LEAVE_REQ_CLID)
    {
      zdo_mgmt_leave_srv(param);
    }
#if (defined ZB_JOINING_LIST_SUPPORT) && defined(ZB_ROUTER_ROLE)
    else if (ind->clusterid == ZDO_MGMT_NWK_IEEE_JOINING_LIST_REQ_CLID)
    {
      zdo_nwk_joining_list_resp(param);
    }
#endif /* (defined ZB_JOINING_LIST_SUPPORT) && defined(ZB_ROUTER_ROLE) */
    else if (ind->clusterid == ZDO_BIND_REQ_CLID)
    {
      zb_zdo_bind_unbind_res(param, ZB_TRUE);
    } else if (ind->clusterid == ZDO_UNBIND_REQ_CLID)
    {
      zb_zdo_bind_unbind_res(param, ZB_FALSE);
#ifndef R23_DISABLE_DEPRECATED_ZDO_CMDS
    } else if (ind->clusterid == ZDO_END_DEVICE_BIND_REQ_CLID)
    {
      zb_zdo_end_device_bind_handler(param);

    } else if (ind->clusterid == ZDO_MGMT_RTG_REQ_CLID)  /* GP Add */
    {
      zdo_rtg_resp(param);
#endif /* R23_DISABLE_DEPRECATED_ZDO_CMDS */
#ifdef ZB_FIXED_OPTIONAL_DESC_RESPONSES
    } else if (ind->clusterid == ZDO_COMPLEX_DESC_REQ_CLID)
    {
      zb_zdo_send_complex_desc_resp(param);
    } else if (ind->clusterid == ZDO_USER_DESC_REQ_CLID)
    {
      zb_zdo_send_user_desc_resp(param);
#ifndef R23_DISABLE_DEPRECATED_ZDO_CMDS
    } else if (ind->clusterid == ZDO_USER_DESC_SET_CLID)
    {
      zb_zdo_send_user_desc_conf(param);
#endif /* R23_DISABLE_DEPRECATED_ZDO_CMDS */
#endif
    }
    else
#ifdef ZB_ROUTER_ROLE
    if (ZB_IS_DEVICE_ZC_OR_ZR()
        && ind->clusterid == ZDO_MGMT_PERMIT_JOINING_CLID)
    {
        zb_zdo_mgmt_permit_joining_handle(param);
    }
    else
#endif
    {
      /* Not supported command handling */
      if (!ZB_NWK_IS_ADDRESS_BROADCAST(ind->dst_addr))
      {
        zb_uint16_t addr = ind->src_addr;
        zb_uint16_t command_id = ZB_ZDO_RESP_FROM_REQ_CLUSTER(ind->clusterid);
        zb_zdo_not_supported_resp_t *resp;

        TRACE_MSG(TRACE_ZDO3, "Send unsupported cmd response, clu %d tsn %hd addr %d",
                  (FMT__D_H_D, command_id, tsn, addr));

        resp = zb_buf_initial_alloc(param, sizeof(zb_zdo_not_supported_resp_t));
        ZB_BZERO(resp, sizeof(zb_zdo_not_supported_resp_t));
        resp->tsn = tsn;
        resp->status = ZB_ZDP_STATUS_NOT_SUPPORTED;

        zdo_send_resp_by_short(command_id, param, addr);
      }
      else
      {
        TRACE_MSG(TRACE_ZDO3, "Unsupported broadcast cli cmd - drop", (FMT__0));
        skip_free_buf = 0;
      }
    }
  }
  else
  {
    /* Process server commands */
#ifndef ZB_LITE_NO_ZDO_RESPONSE_VALIDATION
    if (!zb_zdo_validate_reponse(param, ind->clusterid))
    {
      TRACE_MSG(TRACE_ZDO3, "dropped malformed zdo response", (FMT__0));
      skip_free_buf = 0;
    } else
#endif /* ZB_LITE_NO_ZDO_RESPONSE_VALIDATION */
    if ((ind->clusterid == ZDO_NODE_DESC_RESP_CLID) || (ind->clusterid == ZDO_POWER_DESC_RESP_CLID) ||
        (ind->clusterid == ZDO_MGMT_PERMIT_JOINING_RESP_CLID) || (ind->clusterid == ZDO_NWK_ADDR_RESP_CLID) ||
        (ind->clusterid == ZDO_ACTIVE_EP_RESP_CLID) || (ind->clusterid == ZDO_IEEE_ADDR_RESP_CLID) ||
        (ind->clusterid == ZDO_MATCH_DESC_RESP_CLID) || (ind->clusterid == ZDO_MGMT_LEAVE_RESP_CLID) ||
        (ind->clusterid == ZDO_MGMT_LQI_RESP_CLID) || (ind->clusterid == ZDO_BIND_RESP_CLID) ||
        (ind->clusterid == ZDO_UNBIND_RESP_CLID) ||
#ifndef R23_DISABLE_DEPRECATED_ZDO_CMDS
        (ind->clusterid == ZDO_END_DEVICE_BIND_RESP_CLID) ||
        (ind->clusterid == ZDO_MGMT_RTG_RESP_CLID) ||  /* GP add */
#endif /* R23_DISABLE_DEPRECATED_ZDO_CMDS */
        (ind->clusterid == ZDO_MGMT_BIND_RESP_CLID) || (ind->clusterid == ZDO_MGMT_NWK_ENHANCED_UPDATE_NOTIFY_CLID))
    {
      zb_uint8_t status = 0;
      if (clusterid == ZDO_MGMT_LEAVE_RESP_CLID)
      {
        status = *((zb_uint8_t*)zb_buf_begin(param) + 1/*tsn*/);
      }

      if (ind->clusterid == ZDO_NWK_ADDR_RESP_CLID
          || ind->clusterid == ZDO_IEEE_ADDR_RESP_CLID)
      {
        /* Now we have both short and long for some device. Update
         * the address translation table (do not lock - just cache) */
        zb_zdo_nwk_addr_resp_head_t *resp = (zb_zdo_nwk_addr_resp_head_t*)(zb_buf_begin(param));

        if (resp->status == (zb_uint8_t)ZB_ZDP_STATUS_SUCCESS)
        {
          zb_address_ieee_ref_t addr_ref;
          zb_ieee_addr_t ieee_addr;
          zb_uint16_t nwk_addr;

          ZB_LETOH64(ieee_addr, resp->ieee_addr);
          ZB_LETOH16((zb_uint8_t *)&nwk_addr, (zb_uint8_t *)&resp->nwk_addr);

          ZB_MEMCPY(resp->ieee_addr, ieee_addr, sizeof(zb_ieee_addr_t));
          ZB_MEMCPY((zb_uint8_t *)&resp->nwk_addr, (zb_uint8_t *)&nwk_addr, sizeof(zb_uint16_t));

          TRACE_MSG(TRACE_ZDO3, "Got addr resp: long address " TRACE_FORMAT_64 " short 0x%x - update address translation table",
                    (FMT__A_D, TRACE_ARG_64(resp->ieee_addr), resp->nwk_addr));

          ret = zb_address_update(resp->ieee_addr, resp->nwk_addr, ZB_FALSE, &addr_ref);
          if (ret != RET_OK)
          {
            TRACE_MSG(TRACE_ERROR, "zb_address_update failed [%d]", (FMT__D, ret));
          }
        }
        /* TODO: same for ZDO_MGMT_LQI_RESP_CLID */
      }

#ifdef APS_FRAGMENTATION
      if (ind->clusterid == ZDO_NODE_DESC_RESP_CLID)
      {
        /* Update max_in_trans_size for this short addr */
        zb_zdo_node_desc_resp_t *resp = (zb_zdo_node_desc_resp_t*)(zb_buf_begin(param));

        if (resp->hdr.status == (zb_zdp_status_t)ZB_ZDP_STATUS_SUCCESS)
        {
          zb_aps_add_max_trans_size(resp->hdr.nwk_addr,
                                    resp->node_desc.max_incoming_transfer_size,
                                    resp->node_desc.max_buf_size);
        }
      }
#endif

      ret = zdo_af_resp(param);
      /* do not free buffer if callback was found */
      skip_free_buf = ZB_B2U(ret == RET_OK);
#ifdef ZB_ROUTER_ROLE
      /* When received Leave Resp after LEAVE, device must be rempoved from the
       * noighbor: it is already gone. */
      if (clusterid == ZDO_MGMT_LEAVE_RESP_CLID
          && status == 0U)
      {
        zdo_handle_mgmt_leave_rsp(src);
      }
#else
      ZVUNUSED(status);
#endif

#ifdef ZB_ENABLE_SE
      if (ind->clusterid == ZDO_MATCH_DESC_RESP_CLID && !ZB_U2B(skip_free_buf)
          && ZDO_SELECTOR().match_desc_resp_handler != NULL)
      {
        skip_free_buf = ZDO_SELECTOR().match_desc_resp_handler(param);
      }
#endif
    }
#ifdef ZB_ROUTER_ROLE
    else if (ind->clusterid == ZDO_PARENT_ANNCE_RESP_CLID)
    {
      /*Call PARENT_ANNCE_RESP handler  */
      zb_parent_annce_resp_handler(param,(void *)(body + 0));
    }
    else if (ind->clusterid == ZDO_MGMT_NWK_UPDATE_NOTIFY_CLID)
    {
      /* nwk update notify can be received without request, so zdo_af_resp()
         can return error - no callback for response */
      ret = zdo_af_resp(param);
      if (ret != RET_OK)
      {
        zb_zdo_mgmt_handle_unsol_nwk_update_notify(param);
      }
    }
#endif /* ZB_ROUTER_ROLE */
#if defined ZB_JOINING_LIST_SUPPORT && defined ZB_ROUTER_ROLE
    else if (ind->clusterid == ZDO_MGMT_NWK_IEEE_JOINING_LIST_RESP_CLID)
    {
      zb_nwk_ieee_joining_list_resp_handler(param);
    }
#endif /* ZB_JOINING_LIST_SUPPORT && ZB_ROUTER_ROLE */
#if defined ZB_SUBGHZ_BAND_ENABLED && defined ZB_ROUTER_ROLE
#ifdef ZB_MGMT_NWK_ENHANCED_UPDATE_ENABLED
    else if (ind->clusterid == ZDO_MGMT_NWK_UNSOLICITED_ENHANCED_UPDATE_NOTIFY_CLID)
    {
      zb_zdo_mgmt_unsol_enh_nwk_update_notify_handler(param);
    }
#endif /* ZB_MGMT_NWK_ENHANCED_UPDATE_ENABLED */
#endif /* (defined ZB_SUBGHZ_BAND_ENABLED) && defined(ZB_ROUTER_ROLE) */
    else if (ind->clusterid == ZDO_SIMPLE_DESC_RESP_CLID)
    {
      /*  convert payload from ZibBee ZDO format to ZBOSS ZDO format */
      /* Skip response with malformed descriptor in payload */
      ret = zb_zdo_simple_desc_resp_convert_zboss(param);

      if (ret == RET_OK)
      {
        ret = RET_IGNORE;
        if(ZG->zdo.zb_zdo_responce_cb !=NULL)
        {
          ret = (ZG->zdo.zb_zdo_responce_cb)(param, ind->clusterid);
        }

        if( ret != RET_OK)
        {
          ret = zdo_af_resp(param);
        }
      }

      /* do not free buffer if callback was found */
      skip_free_buf = ZB_B2U(ret == RET_OK);
    }
#if !defined(ZB_LITE_NO_ZDO_SYSTEM_SERVER_DISCOVERY)
    else if (ind->clusterid == ZDO_SYSTEM_SERVER_DISCOVERY_RESP_CLID)
    {
      zb_zdo_system_server_discovery_resp_t *resp = (zb_zdo_system_server_discovery_resp_t*)zb_buf_begin(param);

      if (zb_buf_len(param) < sizeof(zb_zdo_system_server_discovery_resp_t))
      {
        TRACE_MSG(TRACE_ZDO1, "Malformed zb_zdo_system_server_discovery_resp_t packet", (FMT__0));
        skip_free_buf = 0;
      }
      else
      {
        TRACE_MSG(TRACE_ZDO1, "SERVER_DISCOVERY_RESP_CLID tsn %hd, disc_tsn %hd ",
                  (FMT__H_H, resp->tsn, ZDO_CTX().system_server_discovery_tsn));

        if (resp->tsn == ZDO_CTX().system_server_discovery_tsn
            && ZDO_CTX().system_server_discovery_cb != NULL
          )
        {
          zb_uint16_t mask_tmp;

          mask_tmp = resp->server_mask;
          ZB_HTOLE16((zb_uint8_t *)&(resp->server_mask), (zb_uint8_t *)&mask_tmp);
          ZB_SCHEDULE_CALLBACK(ZDO_CTX().system_server_discovery_cb, param);
        }
        else
        {
          skip_free_buf = 0;
        }
      }
    }
#endif  /* #ifndef ZB_LITE_NO_ZDO_SYSTEM_SERVER_DISCOVERY */
    else
    {
      TRACE_MSG(TRACE_ZDO1, "unhandl clu %hd - drop", (FMT__H, ind->clusterid));
      skip_free_buf = 0;
    }
  }

  TRACE_MSG(TRACE_ZDO3, "skip_free_buf %hd", (FMT__H, skip_free_buf));
  if (!ZB_U2B(skip_free_buf))
  {
    zb_buf_free(param);
  }
}


void zb_zdo_register_device_annce_cb(zb_device_annce_cb_t cb)
{
  TRACE_MSG(TRACE_ZDO1, "zb_zdo_register_device_annce_cb cb %p", (FMT__P, cb));
  ZG->zdo.device_annce_cb = cb;
}

void zb_zdo_register_zb_zdo_responce_cb(zb_zdo_responce_cb_t cb)
{
  TRACE_MSG(TRACE_ZDO1, "zb_zdo_register_zb_zdo_responce_cb cb %p", (FMT__P, cb));
  ZG->zdo.zb_zdo_responce_cb = cb;
}

#if (defined ZB_JOINING_LIST_SUPPORT) && defined(ZB_ROUTER_ROLE)

/* handling mlme-set.confirm */
static void zb_nwk_ieee_joining_list_resp_handler_cont_cont(zb_uint8_t param)
{
  zb_mlme_set_confirm_t *set_confirm;

  TRACE_MSG(TRACE_ZDO2, ">> zb_nwk_ieee_joining_list_resp_handler_cont_cont %hd", (FMT__H, param));

  ZB_ASSERT(zb_buf_len(param) >= sizeof(*set_confirm));
  set_confirm = (zb_mlme_set_confirm_t *) zb_buf_begin(param);

  if (set_confirm->status == MAC_SUCCESS)
  {
    TRACE_MSG(TRACE_ZDO2, "list updated successfully %hd", (FMT__H, param));
  }
  else
  {
    zb_zdo_mgmt_nwk_ieee_joining_list_rsp_t * resp;
    /* setting failure status */
    resp = (zb_zdo_mgmt_nwk_ieee_joining_list_rsp_t *) zb_buf_begin(ZDO_CTX().joining_list_ctx.original_buffer);
    resp->status = ZB_ZDP_STATUS_NOT_PERMITTED;

    TRACE_MSG(TRACE_ZDO2, "mlme-set failed(%hd)", (FMT__H, set_confirm->status));
  }

  if (zdo_af_resp(ZDO_CTX().joining_list_ctx.original_buffer) != RET_OK)
  {
    zb_buf_free(ZDO_CTX().joining_list_ctx.original_buffer);
  }

  zb_buf_free(param);

  TRACE_MSG(TRACE_ZDO2, "<< zb_nwk_ieee_joining_list_resp_handler_cont_cont", (FMT__0));
}

/* Preparing mlme-set.req */
static void zb_nwk_ieee_joining_list_resp_handler_cont(zb_uint8_t param)
{
  zb_zdo_mgmt_nwk_ieee_joining_list_rsp_t *resp;
  zb_mlme_set_request_t *set_req;
  zb_mlme_set_ieee_joining_list_req_t *set_req_param;
  void *ptr;
  zb_uint8_t addr_list_size;

  TRACE_MSG(TRACE_ZDO2, ">> zb_nwk_ieee_joining_list_resp_handler_cont %hd", (FMT__H, param));
  TRACE_MSG(TRACE_ZDO2, "orig buffer: %hd", (FMT__H, ZDO_CTX().joining_list_ctx.original_buffer));

  resp = (zb_zdo_mgmt_nwk_ieee_joining_list_rsp_t *)zb_buf_begin(ZDO_CTX().joining_list_ctx.original_buffer);

  ZDO_CTX().joining_list_ctx.update_id = resp->ieee_joining_list_update_id;

  addr_list_size = resp->ieee_joining_count * (zb_uint8_t)sizeof(zb_ieee_addr_t);
  ptr = zb_buf_initial_alloc(param,
                             sizeof(*set_req) + sizeof(*set_req_param) + addr_list_size);

  set_req = (zb_mlme_set_request_t *)ptr;
  set_req->pib_attr = ZB_PIB_ATTRIBUTE_JOINING_IEEE_LIST;
  set_req->pib_length = (zb_uint8_t)sizeof(zb_mlme_set_ieee_joining_list_req_t) + addr_list_size;
  set_req->pib_index = 0;
  set_req->confirm_cb_u.cb = &zb_nwk_ieee_joining_list_resp_handler_cont_cont;

  set_req_param = (zb_mlme_set_ieee_joining_list_req_t *)(void *)(set_req + 1);
  set_req_param->op_type = (zb_uint8_t)ZB_MLME_SET_IEEE_JL_REQ_RANGE;
  set_req_param->param.range_params.joining_policy = resp->joining_policy;
  set_req_param->param.range_params.start_index = resp->start_index;
  set_req_param->param.range_params.list_size = resp->ieee_joining_list_total;
  set_req_param->param.range_params.items_count = resp->ieee_joining_count;

  ZB_MEMCPY((zb_uint8_t *)ptr + sizeof(*set_req) + sizeof(*set_req_param),
            (zb_uint8_t *)(resp + 1),
            addr_list_size);

  ZB_SCHEDULE_CALLBACK(zb_mlme_set_request, param);

  TRACE_MSG(TRACE_ZDO2, "<< zb_nwk_ieee_joining_list_resp_handler_cont", (FMT__0));
}

/*
 * @see 2.4.4.4.11 of r22 Zigbee spec
 */
static void zb_nwk_ieee_joining_list_resp_handler(zb_uint8_t param)
{
  zb_bool_t ret = ZB_TRUE;
  zb_uint8_t left_len;
  zb_zdo_mgmt_nwk_ieee_joining_list_rsp_t *resp;

  TRACE_MSG(TRACE_ZDO1, ">> zb_nwk_ieee_joining_list_resp_handler %hd", (FMT__H, param));

  resp = (zb_zdo_mgmt_nwk_ieee_joining_list_rsp_t *)zb_buf_begin(param);
  if (resp->status != ZB_ZDP_STATUS_SUCCESS)
  {
    TRACE_MSG(TRACE_ZDO3, "expected zdp status to be ZB_ZDP_STATUS_SUCCESS", (FMT__0));
    ret = ZB_FALSE;
  }
  else if (zb_buf_len(param) < sizeof(*resp))
  {
    /* otherwise the full zb_zdo_mgmt_nwk_ieee_joining_list_rsp_t is expected */
    ret = ZB_FALSE;
    TRACE_MSG(TRACE_ZDO3, "insufficient buffer length", (FMT__0));
  }
  else if (ZB_U2B(ZB_AIB().aps_designated_coordinator))
  {
    ret = ZB_FALSE;
    TRACE_MSG(TRACE_ZDO3, "coordinators are not supposed to handle this resp", (FMT__0));
  }
  else
  {
    /* MISRA rule 15.7 requires empty 'else' branch. */
  }

  if (ret)
  {
    zb_size_t u32len = zb_buf_len(param) - sizeof(*resp);
    left_len = (zb_uint8_t)u32len;

    /* checking payload */
    ret = (zb_bool_t)(left_len >= (resp->ieee_joining_count * sizeof(zb_ieee_addr_t)));

    /* checking new joining policy value */
    switch (resp->joining_policy)
    {
      case ZB_MAC_JOINING_POLICY_NO_JOIN:
      case ZB_MAC_JOINING_POLICY_ALL_JOIN:
      case ZB_MAC_JOINING_POLICY_IEEELIST_JOIN:
        /* ok */
        break;
      default:
        /* no such policy */
        ret = ZB_FALSE;
        break;
    }
  }

  if (ret)
  {
    /* in order to make mlme-set.req we need to get a new buffer */
    ZDO_CTX().joining_list_ctx.original_buffer = param;
    ret = zb_buf_get_out_delayed(zb_nwk_ieee_joining_list_resp_handler_cont) == RET_OK ? ZB_TRUE : ZB_FALSE;
    TRACE_MSG(TRACE_ZDO3, "preparing mlme-set", (FMT__0));
  }

  if (!ret)
  {
    TRACE_MSG(TRACE_ZDO3, "malformed response", (FMT__0));

    /* setting failure status */
    resp = (zb_zdo_mgmt_nwk_ieee_joining_list_rsp_t *) zb_buf_begin(param);
    if (resp->status == ZB_ZDP_STATUS_SUCCESS)
    {
      resp->status = ZB_ZDP_STATUS_NOT_PERMITTED;
    }

    /* passing to caller */
    if (zdo_af_resp(param) != RET_OK)
    {
      zb_buf_free(param);
    }
  }

  TRACE_MSG(TRACE_ZDO1, "<< zb_nwk_ieee_joining_list_resp_handler (res %hd)", (FMT__H, ret));
}

#endif /* (defined ZB_JOINING_LIST_SUPPORT) && defined(ZB_ROUTER_ROLE) */

#ifdef ZB_ROUTER_ROLE

void zdo_parent_annce_handler(zb_uint8_t param, zb_uint16_t param_req)
{
  zb_uint8_t status = ZB_ZDP_STATUS_SUCCESS;

  zb_neighbor_tbl_ent_t *nbt = NULL;
  zb_uindex_t i;
  zb_ieee_addr_t ieee_address_req;
  zb_ieee_addr_t *ieee_address_rsp;

  zb_apsde_data_indication_t ind;
  zb_zdo_parent_annce_t req;
  zb_zdo_parent_annce_rsp_t *rsp;
  zb_uint8_t *aps_body;
  zb_uint8_t u8_param_req = (zb_uint8_t)param_req;

  TRACE_MSG(TRACE_ZDO1, ">> zdo_parent_annce_srv resp_buf  %d, req_buf %d", (FMT__D_D,param, param_req));

  if (zb_buf_len(u8_param_req) < sizeof(req))
  {
    TRACE_MSG(TRACE_ZDO1, "parent_annce: malformed packet", (FMT__0));
    zb_buf_free(u8_param_req);
    return;
  }

  aps_body = zb_buf_begin(u8_param_req);
  aps_body++;
  req.num_of_children = *aps_body;
  aps_body++;

  if (zb_buf_len(u8_param_req) < (sizeof(req) + sizeof(zb_ieee_addr_t)*req.num_of_children))
  {
    TRACE_MSG(TRACE_ZDO1, "parent_annce: malformed packet, broken ChildInfo array", (FMT__0));
    zb_buf_free(u8_param_req);
    return;
  }

  ZB_MEMCPY(&ind, ZB_BUF_GET_PARAM(u8_param_req, zb_apsde_data_indication_t), sizeof(ind));

/*      2.4.3.1.12 Parent_annce
-	Reset timer for our own Parent_annce, if any
-       A router shall construct an empty Parent_Annce_Rsp message with
        NumberOfChildren set to 0.
-       Examine each Extended Address present in the message and search
        its Neighbor Table for an Extended Address entry that matches. For each
        match, process as follows:
        1. If the Device Type is Zigbee End Device (0x02) and the Keepalive
           Received value is TRUE, do the following:
           a. It shall append to the Parent_annce_rsp frame the ChildInfo structure.
           b. Increment the NumberOfChildren by 1.
        2. If the Device Type is not Zigbee End Device (0x02) or the Keepalive
           Received value is FALSE, do not process any further. Continue to the
           next entry.

-       If the NumberOfChildren field value is 0, the local device shall discard
        the previously constructed Parrent_Annce_rsp. No response message shall
        be sent.

-       If the NumberOfChildren field in the Parent_Annce_rsp is greater than 0,
        it shall unicast the message to the sender of the Parent_Annce message.

-	Send Parent_annce_rsp unicast to the Parent_annce sender; maybe, split
        to more than 1 message.
*/


  if (ZB_IS_DEVICE_ZC_OR_ZR())
  {
    if (zb_schedule_alarm_cancel(zdo_send_parent_annce,0, NULL) == RET_OK)
    {
      TRACE_MSG(TRACE_ZDO1, "Re-Schedule oun zdo_send_parent_annce ", (FMT__0));
      ZB_SCHEDULE_ALARM(zdo_send_parent_annce, 0, ZB_PARENT_ANNCE_JITTER());
    }

    rsp = zb_buf_initial_alloc(param, sizeof(zb_zdo_parent_annce_rsp_t));
    rsp->hdr.status = status;
    rsp->hdr.num_of_children = 0;

/*    Examine each Extended Address present in the message and search
      its Neighbor Table for an Extended Address entry that matches. For each
      match, process as follows:
*/

    for(i = 0; i < req.num_of_children; i++)
    {
      ZB_MEMCPY(&ieee_address_req, aps_body, sizeof(zb_ieee_addr_t));
      aps_body += sizeof(zb_ieee_addr_t);

      if (zb_nwk_neighbor_get_by_ieee(ieee_address_req, &nbt) == RET_OK)
      {
        TRACE_MSG(TRACE_NWK2, "device is in the neighbor table", (FMT__0));
/*
  If the Device Type is Zigbee End Device (0x02) and the Keepalive
  Received value is TRUE, do the following:
  a. It shall append to the Parent_annce_rsp frame the ChildInfo structure.
  b. Increment the NumberOfChildren by 1.
  If the Device Type is not Zigbee End Device (0x02) or the Keepalive
  Received value is FALSE, do not process any further. Continue to the
  next entry.
*/
        if (nbt->device_type == ZB_NWK_DEVICE_TYPE_ED)
        {
          if (ZB_U2B(nbt->keepalive_received))
          {
            rsp->hdr.num_of_children++;
            ieee_address_rsp = zb_buf_alloc_right(param, sizeof(zb_ieee_addr_t));
            zb_address_ieee_by_ref(*ieee_address_rsp, nbt->u.base.addr_ref);
            TRACE_MSG(TRACE_ZDO1, "Add entry %p in responce", (FMT__P, nbt->u.base.addr_ref));
          }
          else
          {
            /*Delete entry from the NBT*/
            zb_ret_t ret = zb_nwk_neighbor_delete(nbt->u.base.addr_ref);
            if (ret == RET_OK)
            {
              TRACE_MSG(TRACE_ZDO1, "Delete entry %p from the NBT", (FMT__P, nbt->u.base.addr_ref));
            }
            else
            {
              TRACE_MSG(TRACE_ERROR, "zb_nwk_neighbor_delete addr_ref not found [%d]", (FMT__D, ret));
              ZB_ASSERT(0);
            }
          }
        } /*If (nbt->device_type == ZB_NWK_DEVICE_TYPE_ED)*/
      } /*Found NBT entry with long address form the parent announce*/
    } /*For all instances from parent_annce*/
    if (rsp->hdr.num_of_children != 0U)
    {
      /*Send response*/
      zdo_send_resp_by_short(ZDO_PARENT_ANNCE_RESP_CLID, param, ind.src_addr);
    }
    else
    {
      /*Don't send parent_annce_resp, free buffer*/
      zb_buf_free(param);
      TRACE_MSG(TRACE_ZDO1, "zb_parent_annce_resp is empty, does not send response", (FMT__0));
    }
    zb_buf_free(u8_param_req); /*Free request buffer*/
  } /*If we are a router or coordinator*/
  TRACE_MSG(TRACE_ZDO1, "<< zdo_parent_annce_srv", (FMT__0));
}


static void zb_parent_annce_resp_handler(zb_uint8_t param, void *dt)
{
  zb_neighbor_tbl_ent_t *nbt = NULL;
  zb_ieee_addr_t ieee_addr;
  zb_zdo_parent_annce_rsp_t rsp;
  zb_uint8_t *aps_body;
  zb_uindex_t i;

  TRACE_MSG(TRACE_ZDO1, ">> zb_parent_annce_resp_handler %p", (FMT__P, dt));

/* 2.4.4.2.22.2 On receipt of a Parent_annce_rsp, the device shall examine its
 * Neighbor Table for each extended address in the ChildInfo entry and do the
 * following.
 * If the entry matches and the Device Type is Zigbee End Device (0x02), it shall
 * delete the entry from the Neigbor table.
 * If the entry does not match, no more processing is performed on this ChildInfo
 * entry.
 * There is no message generated in response to a Parent_annce_rsp.
*/

  aps_body = zb_buf_begin(param);
  aps_body++;
  rsp.hdr.status = *aps_body;
  aps_body++;
  rsp.hdr.num_of_children = *aps_body;
  aps_body++;

  if (zb_buf_len(param) < (sizeof(rsp) + rsp.hdr.num_of_children * sizeof(zb_ieee_addr_t)))
  {
    TRACE_MSG(TRACE_ZDO1, "malformed packet", (FMT__0));
    zb_buf_free(param);
    return;
  }


  for(i = 0; i < rsp.hdr.num_of_children; i++)
  {
    ZB_MEMCPY(&ieee_addr, aps_body, sizeof(zb_ieee_addr_t));
    aps_body += sizeof(zb_ieee_addr_t);

    if (zb_nwk_neighbor_get_by_ieee(ieee_addr, &nbt) == RET_OK)
    {
      TRACE_MSG(TRACE_NWK2, "device is in the neighbor table", (FMT__0));
      if (nbt->device_type == ZB_NWK_DEVICE_TYPE_ED)
      {
          zb_ret_t ret = zb_nwk_neighbor_delete(nbt->u.base.addr_ref);
          if (ret == RET_OK)
          {
            TRACE_MSG(TRACE_ZDO1, "Delete entry %p from the NBT", (FMT__P, nbt->u.base.addr_ref));
          }
          else
          {
            TRACE_MSG(TRACE_ERROR, "zb_nwk_neighbor_delete addr_ref not found [%d]", (FMT__D, ret));
            ZB_ASSERT(0);
          }
      }
    }
  }
  zb_buf_free(param);
  TRACE_MSG(TRACE_ZDO1, "<< zb_parent_annce_resp_handler", (FMT__0));
}

static void zdo_device_annce_srv(zb_uint8_t param, void *dt)
{
  zb_zdo_device_annce_t *da = dt;
  zb_nwk_neighbor_element_t ne = { 0 };
  zb_address_ieee_ref_t addr_ref;
  zb_bool_t unicast = ZB_FALSE;   /* assign initial value for static analyzers and/or very "smart" compilers */
  zb_bool_t neighbor = ZB_FALSE;

  TRACE_MSG(TRACE_ZDO1, ">> zdo_device_annce_srv %p", (FMT__P, dt));

  if (zb_buf_len(param) < sizeof(*da))
  {
    TRACE_MSG(TRACE_ZDO1, "dev_annce: malformed packet", (FMT__0));
    zb_buf_free(param);
    return;
  }

  ZB_LETOH16_ONPLACE(da->nwk_addr);

  TRACE_MSG(TRACE_ZDO1, "zdo_device_annce_srv %d=0x%x ieee=" TRACE_FORMAT_64 " cap=%d ",
            (FMT__D_D_A_D, da->tsn, da->nwk_addr, TRACE_ARG_64(da->ieee_addr), da->capability));

#ifdef ZB_ENABLE_ZGP
  zgp_handle_dev_annce(da);
#endif /* ZB_ENABLE_ZGP */

#if defined ZB_PRO_STACK && defined ZB_ROUTER_ROLE
  /* #AT: cross stack compatibility */
  if (ZB_IS_DEVICE_ZC_OR_ZR())
  {
    /* Test for address conflict */
    if (zb_nwk_test_dev_annce(da->nwk_addr, da->ieee_addr) != RET_OK)
    {
      TRACE_MSG(TRACE_NWK1, "dev_annce: address conflict", (FMT__0));
      zb_buf_free(param);
      return;
    }
  }
#endif

  if (ZB_IS_DEVICE_ZC_OR_ZR())
  {
    zb_apsde_data_indication_t *ind = (zb_apsde_data_indication_t *)ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);

    neighbor = (ind->mac_src_addr == da->nwk_addr)?(ZB_TRUE):(ZB_FALSE);
    unicast = ( (ind->mac_dst_addr != ZB_NWK_BROADCAST_ALL_DEVICES) )?(ZB_TRUE):(ZB_FALSE);

    TRACE_MSG(TRACE_ZDO1, "mac src addr 0x%x mac dst addr 0x%x addr 0x%x neighbor %hd unicast %hd",
              (FMT__D_D_D_H_H, ind->mac_src_addr, ind->mac_dst_addr, da->nwk_addr, neighbor, unicast));
  }

  /*
    First detect former child.
    For 2007 profile is is simple: short address is changed.
    No ideas for PRO: see 2.5.5.5.4.3:
    "ZDO shall arrange that any IEEE address to short
    address mappings which have become known to applications running on this
    device be updated. This behavior is mandatory, but the mechanism by which it is
    achieved is outside the scope of this specification."
  */

  /* There can be 0xFF..FF address - in this case it will be
   * impossible to find an entry */
  if (ZB_IS_DEVICE_ZC_OR_ZR()
      && !ZB_IEEE_ADDR_IS_UNKNOWN(da->ieee_addr))
  {
    if (zb_address_by_ieee(da->ieee_addr, ZB_FALSE, ZB_FALSE, &addr_ref) == RET_OK)
    {
      zb_uint8_t relationship;
      zb_uint16_t old_short;

      zb_address_short_by_ref(&old_short, addr_ref);
      relationship = zb_nwk_get_nbr_rel_by_short(old_short);
      if (old_short != (zb_uint16_t)~0U
          && (relationship == ZB_NWK_RELATIONSHIP_CHILD
              || relationship == ZB_NWK_RELATIONSHIP_UNAUTHENTICATED_CHILD))
      {
        if (old_short != da->nwk_addr
            /*
             * if we got broadcast device annonce from our child - ED, it means this is not our
             * child any more. It works for PRO as well.
             * It works because briadcasts about ED-child will be dropped at NWK as dups.
             */

            /*cstat -MISRAC2012-Rule-13.5 */
            /* After some investigation, the following violation of Rule 13.5 seems to be a false
             * positive. There are no side effects to 'zb_nwk_get_nbr_dvc_type_by_short()'.
             * This violation seems to be caused by the fact that this function is an external
             * function, which cannot be analyzed by C-STAT. */
            || (!unicast
                && zb_nwk_get_nbr_dvc_type_by_short(old_short) == ZB_NWK_DEVICE_TYPE_ED))
            /*cstat +MISRAC2012-Rule-13.5 */
        {
          /* It was our child, now its address has changed - it is previous child */
          /* Device remains on network - do not forget it (app data) */
          ZB_SCHEDULE_CALLBACK2(zdo_device_removed, addr_ref, ZB_FALSE);
          /* There was mark as previous child. It causes problems with routing and
           * really not used.
           * Remove neighbor entry.
           */
          TRACE_MSG(TRACE_ZDO1, "delete 0x%x from neighbor", (FMT__D, da->nwk_addr));
        }
      }
    }
  }

  if (zb_address_update(da->ieee_addr, da->nwk_addr, ZB_FALSE, &addr_ref) == RET_OK)
  {
    /* check if it's a neighbor mac_addr == nwk_addr */
    if (neighbor)
    {
      if (zb_nwk_get_neighbor_element(da->nwk_addr, ZB_TRUE, &ne) == RET_OK)
      {
        TRACE_MSG(TRACE_ZDO1, "update/add neighbor 0x%x", (FMT__D, da->nwk_addr));
      }
      else
      {
        neighbor = ZB_FALSE;
        TRACE_MSG(TRACE_ZDO1, "seems device %d is not our neighbor", (FMT__D, da->nwk_addr));
      }
    }
  }

  if (neighbor)
  {
    zb_ret_t ret;

    if (ne.device_type == ZB_NWK_DEVICE_TYPE_NONE
        || ne.relationship == ZB_NWK_RELATIONSHIP_PREVIOUS_CHILD)
    {
      ne.device_type = ZB_U2B(ZB_MAC_CAP_GET_DEVICE_TYPE(da->capability)) ? ZB_NWK_DEVICE_TYPE_ROUTER : ZB_NWK_DEVICE_TYPE_ED;
    }
    ne.rx_on_when_idle = ZB_U2B(ZB_MAC_CAP_GET_RX_ON_WHEN_IDLE(da->capability));
    /* It was our child, now its address has changed - it is previous child */
    if (ZB_IS_DEVICE_ZC_OR_ZR()
        && ne.relationship == ZB_NWK_RELATIONSHIP_NONE_OF_THE_ABOVE
        && ZB_U2B(ZB_MAC_CAP_GET_DEVICE_TYPE(da->capability)))
    {
      /* relationship was unknown, both our and remote device are Routers, we got this
       * packet - this is our sibling */
      ne.relationship = ZB_NWK_RELATIONSHIP_SIBLING;
    }
    if ((ne.relationship == ZB_NWK_RELATIONSHIP_CHILD
         || ne.relationship == ZB_NWK_RELATIONSHIP_UNAUTHENTICATED_CHILD))
    {
#ifndef ZB_PRO_STACK
      if (ZB_NIB().depth < ZB_NIB().max_depth)
#endif
      {
        ne.depth = ZB_NIB().depth + 1U;
      }
    }
    if (ne.device_type == ZB_NWK_DEVICE_TYPE_ED)
    {
      ne.permit_joining = 0;
    }
    TRACE_MSG(TRACE_ZDO3, "DEV_ANNCE: /0x%x ne %p dev_t %hd, rx.o.i %hd rel %hd ieee " TRACE_FORMAT_64,
        (FMT__A_D_P_H_H_H, da->nwk_addr, &ne, ne.device_type,
        ne.rx_on_when_idle, ne.relationship, TRACE_ARG_64(da->ieee_addr)));

    ret = zb_nwk_set_neighbor_element(da->nwk_addr, &ne);
    ZB_ASSERT(ret == RET_OK);
  }

  if (ZG->zdo.device_annce_cb != NULL)
  {
    /* callback is used to inform App about receiving device_annce */
    ZG->zdo.device_annce_cb(da);
  }
  else
  {
    zb_zdo_signal_device_annce_params_t *dev_annce_params =
      (zb_zdo_signal_device_annce_params_t *)zb_app_signal_pack(param,
                                                               ZB_ZDO_SIGNAL_DEVICE_ANNCE,
                                                               RET_OK,
                                                               (zb_uint8_t)sizeof(zb_zdo_signal_device_annce_params_t));
    dev_annce_params->device_short_addr = da->nwk_addr;
    ZB_IEEE_ADDR_COPY(dev_annce_params->ieee_addr, da->ieee_addr);
    dev_annce_params->capability = da->capability;
    ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete, param);
    param = 0;
  }

  if (param != 0U)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZDO1, "<< zdo_device_annce_srv", (FMT__0));
}

#endif

/*! @} */
