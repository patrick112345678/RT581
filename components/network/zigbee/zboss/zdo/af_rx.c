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
/* PURPOSE: AF: RX path.
*/

#define ZB_TRACE_FILE_ID 2089
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zdo_common.h"
#ifdef ZB_TEST_PROFILE
#include "zb_test_profile.h"
#endif /* ZB_TEST_PROFILE */
#include "zb_ncp.h"

#if defined ZB_ENABLE_ZCL
#include "zb_zcl.h"
#include "zcl/zb_zcl_common.h"
#endif /* defined ZB_ENABLE_ZCL */

/*! @cond internals_doc */
/*! \addtogroup zb_af */
/*! @{ */

#if defined ZB_ENABLE_ZCL

zb_bool_t zb_af_is_confirm_for_zcl_frame(zb_uint8_t param)
{
  zb_af_endpoint_desc_t *ep_desc = NULL;
  zb_apsde_data_confirm_t* aps_data_conf;
  zb_bool_t is_zcl_frame = ZB_FALSE;

  TRACE_MSG(TRACE_ZDO2, ">> zb_af_is_confirm_for_zcl_frame, param %d", (FMT__D, param));

  aps_data_conf = ZB_BUF_GET_PARAM(param, zb_apsde_data_confirm_t);

  /* check if checking dst EP is correct!!!
     NO. If status is RET_NO_BOUND_DEVICE, ex, then dst EP is 0
     Check by real source EP - src EP !=0 && descriptor EP exist && known ProfileID then
     ZCL
  */

  if (aps_data_conf->src_endpoint != 0U)
  {
    ep_desc = zb_af_get_endpoint_desc(aps_data_conf->src_endpoint);
  }

  /* This check is needed to understand if ZCL (or some other) packet is confirmed.
     Assume, that if EP profile ID is HA or ZLL then ZCL packets only are sent over APS */
  if (ep_desc != NULL
      && (ep_desc->profile_id == ZB_AF_HA_PROFILE_ID
//FIXME:AEV - not sure about SE packets
          || ep_desc->profile_id == ZB_AF_SE_PROFILE_ID
          || ep_desc->profile_id == ZB_AF_ZLL_PROFILE_ID
          || ep_desc->profile_id == ZB_AF_GP_PROFILE_ID))
  {
    is_zcl_frame = ZB_TRUE;
  }


  TRACE_MSG(TRACE_ZDO2, "<< zb_af_is_confirm_for_zcl_frame, is_zcl_frame %d", (FMT__D, is_zcl_frame));
  return is_zcl_frame;
}


void zb_af_handle_zcl_frame_data_confirm(zb_uint8_t param)
{
  zb_apsde_data_confirm_t aps_data_conf;
  zb_zcl_command_send_status_t *cmd_send_status;

  TRACE_MSG(TRACE_ZDO2, ">> zb_af_handle_zcl_frame_data_confirm, param %d", (FMT__D, param));

  ZB_MEMCPY(&aps_data_conf,
            ZB_BUF_GET_PARAM(param, zb_apsde_data_confirm_t),
            sizeof(zb_apsde_data_confirm_t));

  /* FIXME: why can't we use zb_apsde_data_confirm_t instead of zb_zcl_command_send_status_t? */
  cmd_send_status = ZB_BUF_GET_PARAM(param, zb_zcl_command_send_status_t);

  cmd_send_status->status = aps_data_conf.status;
  cmd_send_status->src_endpoint = aps_data_conf.src_endpoint;
  cmd_send_status->dst_endpoint = aps_data_conf.dst_endpoint;
  if (aps_data_conf.addr_mode == ZB_APS_ADDR_MODE_64_ENDP_PRESENT)
  {
    cmd_send_status->dst_addr.addr_type = ZB_ZCL_ADDR_TYPE_IEEE;
    ZB_IEEE_ADDR_COPY(cmd_send_status->dst_addr.u.ieee_addr, aps_data_conf.dst_addr.addr_long);
  }
  else
  {
    cmd_send_status->dst_addr.addr_type = ZB_ZCL_ADDR_TYPE_SHORT;
    cmd_send_status->dst_addr.u.short_addr = aps_data_conf.dst_addr.addr_short;
  }

  TRACE_MSG(TRACE_APS3, "confirm: received address 0x%x", (FMT__D, aps_data_conf.dst_addr));

  if (zb_zcl_ack_callback(param) != RET_OK)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZDO2, "<< zb_af_handle_zcl_frame_data_confirm", (FMT__0));
}


zb_bool_t zb_af_handle_zcl_frame(zb_uint8_t param)
{
  zb_apsde_data_indication_t *ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);
  zb_bool_t packet_processed;

  TRACE_MSG(TRACE_ZDO1, ">> zb_af_handle_zcl_frame, param %hd", (FMT__H, param));

  if (ind->dst_endpoint == ZB_ZCL_BROADCAST_ENDPOINT)
  {
    /*
      Packet is not skipped if at least one endpoint
      with matching cluster and profile is found
    */
    packet_processed =
      (zb_zcl_get_next_target_endpoint(0U, ind->clusterid, ZB_ZCL_CLUSTER_ANY_ROLE, ind->profileid) != 0U);
  }
  else
  {
    zb_af_endpoint_desc_t *endpoint_desc;
#ifdef ZB_CONTROL4_NETWORK_SUPPORT
    /*
      Control4 controller answers to read attribute command with dst_endpoint equal to 0x01. To prevent the packet
      from being discarded here the destination endpoint is set to ZB_CONTROL4_NETWORK_ENDPOINT.
    */
    if ((ind->profileid == ZB_AF_CONTROL4_PROFILE_ID)
        && (ind->dst_endpoint != ZB_CONTROL4_NETWORK_ENDPOINT))
    {
      ind->dst_endpoint = ZB_CONTROL4_NETWORK_ENDPOINT;
    }
#endif

    endpoint_desc = zb_af_get_endpoint_desc(ind->dst_endpoint);
    packet_processed = zb_zcl_is_target_endpoint(endpoint_desc, ind->profileid);
  }

  if (packet_processed)
  {
    ZB_SCHEDULE_CALLBACK(zb_zcl_process_device_command, param);
  }

  TRACE_MSG(TRACE_ZDO1, "<< zb_af_handle_zcl_frame, processed %d", (FMT__D, packet_processed));

  return packet_processed;
}

#endif /* ZB_ENABLE_ZCL */

#ifndef NCP_MODE_HOST

static void zb_af_handle_apsde_data_confirm(zb_uint8_t param)
{
  zb_ret_t status;
#ifdef ZB_TEST_PROFILE
  zb_apsde_data_confirm_t* aps_data_conf = ZB_BUF_GET_PARAM(param, zb_apsde_data_confirm_t);
#endif
  zb_uint8_t *body;

  TRACE_MSG(TRACE_ZDO2, ">> zb_af_handle_apsde_data_confirm, param %d", (FMT__D, param));

  status = zb_buf_get_status(param);

#ifdef ZB_APS_USER_PAYLOAD
  if ((zb_buf_flags_get(param) & ZB_BUF_HAS_APS_USER_PAYLOAD) != 0U)
  {
    if (status == ERROR_CODE(ERROR_CATEGORY_APS,ZB_APS_STATUS_NO_ACK))
    {
      zb_buf_set_status(param, ZB_APS_USER_PAYLOAD_CB_STATUS_NO_APS_ACK);
    }

    if (!ZB_APS_CALL_USER_PAYLOAD_CB(param))
    {
      zb_buf_free(param);
    }
  }
  else
#endif
  {
#if defined ZB_ENABLE_ZCL
  if (zb_af_is_confirm_for_zcl_frame(param))
  {
    if (/* zb_process_bind_trans() ref was not found */
      (status != ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_NO_BOUND_DEVICE)) &&
      /* zb_process_bind_trans() can set status RET_TABLE_FULL
       * for binding transmission table */
      (status != RET_TABLE_FULL) &&
      (status != RET_DEVICE_NOT_FOUND) &&
      (status != RET_OPERATION_FAILED))
    {
      ZB_APS_HDR_CUT(param);
    }
    zb_af_handle_zcl_frame_data_confirm(param);
  }
  else
#endif /* ZB_ENABLE_ZCL */
    {
      /* If zdo request send failed, call callback now */
      if (status == ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_NO_ACK)
        /* If this is ZDP request which has no response, callback should be called too */
        /*cstat !MISRAC2012-Rule-13.5 */
        /* After some investigation, the following violation of Rule 13.5 seems to be
         * a false positive. There are no side effect to 'zb_buf_flags_get()'. This violation
         * seems to be caused by the fact that 'zb_buf_flags_get()' is an external function,
         * which cannot be analyzed by C-STAT. */
        || ZB_BIT_IS_SET(zb_buf_flags_get(param), ZB_BUF_ZDO_CMD_NO_RESP))
      {
        ZB_APS_HDR_CUT_P(param, body);

        if (status == ERROR_CODE(ERROR_CATEGORY_APS,ZB_APS_STATUS_NO_ACK))
        {
          /* status is 1-byte field at beginning of every ZDP resp */
          /* Note: there was ZB_ZDP_STATUS_TIMEOUT_BY_STACK (0xff). Seems, ZB_ZDP_STATUS_TIMEOUT is better. */
          body[1] = ZB_ZDP_STATUS_TIMEOUT;
        }

        if (zdo_af_resp(param) != RET_OK)
        {
          zb_buf_free(param);
        }
      }
      else
      {
        /* transmission successful */
#ifdef ZB_TEST_PROFILE
        if (aps_data_conf->src_endpoint == ZB_TEST_PROFILE_EP)
        {
          zb_aps_hdr_t aps_hdr;
          ZB_MEMSET(&aps_hdr, 0, sizeof(aps_hdr));
          zb_aps_hdr_parse(param, &aps_hdr, ZB_FALSE);
          if (ZB_APS_FC_GET_ACK_REQUEST(aps_hdr.fc))
          {
            TRACE_MSG(TRACE_APS3, "test_profile buf", (FMT__0));
            tp_packet_ack(param);
          }
          else
          {
            zb_buf_free(param);
          }
        }
        else
#endif
        {
          zb_buf_free(param);
        }
      } /* << successful transmission */
    } /* aps_hdr.dst_endpoint == 0 */
  } /* packet without APS user payload */

  TRACE_MSG(TRACE_ZDO2, "<< zb_af_handle_apsde_data_confirm", (FMT__0));
}

static zb_bool_t zb_af_handle_zdo_profile_frame(zb_uint8_t param)
{
  zb_apsde_data_indication_t *ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);
  zb_bool_t packet_processed = ZB_FALSE;

  TRACE_MSG(TRACE_ZDO1, ">> zb_af_handle_zdo_profile_frame, param %d", (FMT__D, param));

  /*Endpoint 0 pass to ZDO, else drop */
  if (ind->dst_endpoint == 0U)
  {
    TRACE_MSG(TRACE_APS3, "call zdo", (FMT__0));
    ZB_SCHEDULE_CALLBACK(zb_zdo_data_indication, param);
    packet_processed = ZB_TRUE;
  }

  TRACE_MSG(TRACE_ZDO1, "<< zb_af_handle_zdo_profile_frame, processed %d",
            (FMT__D, packet_processed));

  return packet_processed;
}


#ifdef ZB_TEST_PROFILE

static zb_bool_t zb_af_handle_test_profile_frame(zb_uint8_t param)
{
  zb_apsde_data_indication_t *ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);
  zb_bool_t packet_processed = ZB_FALSE;

  TRACE_MSG(TRACE_ZDO1, ">> zb_af_handle_test_profile_frame, param %d", (FMT__D, param));

  if ( IS_CLUSTERID_TEST_PROFILE2(ind->clusterid) )
  {
    ZB_SCHEDULE_CALLBACK(zb_test_profile_indication, param);
    packet_processed = ZB_TRUE;
  }

  TRACE_MSG(TRACE_ZDO1, "<< zb_af_handle_test_profile_frame, processed %d",
            (FMT__D, packet_processed));

  return packet_processed;
}

#endif /* ZB_TEST_PROFILE */

void zb_apsde_data_indication(zb_uint8_t param)
{
  zb_apsde_data_indication_t *ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);
  zb_bool_t packet_processed = ZB_FALSE;

  TRACE_MSG(TRACE_APS3, "> apsde_data_ind: pkt %hd h 0x%hx len %hd, dst endp %hd",
            (FMT__H_H_H_H, param, zb_buf_get_handle(param), zb_buf_len(param), ind->dst_endpoint));
  ZB_TH_PUSH_PACKET(ZB_TH_APSDE_DATA, ZB_TH_PRIMITIVE_INDICATION, param);

  ZB_APS_HDR_CUT(param);

  TRACE_MSG(TRACE_ZDO1, "apsde_data_ind %p", (FMT__P, ZG->zdo.af_data_cb));
  /* In NCP simple descriptor can be filled explicitly by Host, but ZCL ep desc is absent.
     So packet is not processed by ZCL.
     Pass it up to ncp layer and then host via af_data_cb.
     Same mechanism works for debug tests like zdo_startup.
  */
  if (ZG->zdo.af_data_cb != NULL)
  {
    TRACE_MSG(TRACE_ZDO1, "APS pkt for ep %hd - call %p", (FMT__H_P, ind->dst_endpoint, ZG->zdo.af_data_cb));
    zb_nwk_unlock_in(param);
    packet_processed = (zb_bool_t)(ZG->zdo.af_data_cb(param));
  }

  if (!packet_processed)
  {
    switch (ind->profileid)     /*Switch by profile id.*/
    {
      case ZB_AF_ZDO_PROFILE_ID: /*ZDO*/
        packet_processed = zb_af_handle_zdo_profile_frame(param);
        break; /*ZB_AF_ZDO_PROFILE_ID*/

#ifdef ZB_TEST_PROFILE
      case ZB_TEST_PROFILE_ID: /* pass to Test Profile*/
        packet_processed = zb_af_handle_test_profile_frame(param);
        break; /*ZB_TEST_PROFILE_ID*/
#endif

#if defined ZB_ENABLE_SE_MIN_CONFIG
      case ZB_AF_SE_PROFILE_ID:
#endif
        /* r21 spec, 2.3.3.2 Profile ID Endpoint Matching rules:
           Legacy profiles from incoming message should match Common (HA) profile on destination endpoint */
      case ZB_AF_LEGACY_PROFILE1_ID:
      case ZB_AF_LEGACY_PROFILE2_ID:
      case ZB_AF_LEGACY_PROFILE3_ID:
      case ZB_AF_LEGACY_PROFILE4_ID:
      case ZB_AF_LEGACY_PROFILE5_ID:
      case ZB_AF_LEGACY_PROFILE6_ID:
      case ZB_AF_LEGACY_PROFILE7_ID:
      case ZB_AF_HA_PROFILE_ID:
      case ZB_AF_ZLL_PROFILE_ID:
      case ZB_AF_GP_PROFILE_ID:
#ifdef ZB_CONTROL4_NETWORK_SUPPORT
      case ZB_AF_CONTROL4_PROFILE_ID:
#endif
      {
#if defined ZB_ENABLE_ZCL
        packet_processed = zb_af_handle_zcl_frame(param);
#endif
        break; /* ZB_AF_HA_PROFILE_ID */
      }

      case ZB_AF_WILDCARD_PROFILE_ID:  /*Wildcard profile*/
        switch (ind->dst_endpoint)
        {
          case 0: /*Endpoint 0 pass to ZDO*/
            packet_processed = zb_af_handle_zdo_profile_frame(param);
            break; /*ZDO*/

#ifdef ZB_TEST_PROFILE
          case ZB_TEST_PROFILE_EP: /*If ep == TP and cluster is for the test profileV pass to TP*/
            packet_processed = zb_af_handle_test_profile_frame(param);
            break; /*ZB_TEST_PROFILE_EP*/
#endif
          default: /*Look for Simple descriptor*/
#if defined ZB_ENABLE_ZCL
            packet_processed = zb_af_handle_zcl_frame(param);
#endif
            break; /*default:*/
        } /*switch (ind->dst_endpoint) */
        break; /*ZB_AF_WILDCARD_PROFILE_ID*/

      default: /*For test cases when no device profile is set. ZDO_STARTUP for example*/
        break;
    } /*switch (ind->profileid*/
  }

  if (!packet_processed) /*Drop unrecognized frame*/
  {
    TRACE_MSG(TRACE_INFO1, "APS pkt %hd for ep %hd is not recognized - drop", (FMT__H_H, param, ind->dst_endpoint));
    zb_buf_free(param);
  }
  else
  {
    zb_nwk_unlock_in(param);
  }
  TRACE_MSG(TRACE_APS3, "< apsde_data_ind", (FMT__0));
}


void zb_af_set_data_indication(zb_device_handler_t cb)
{
  TRACE_MSG(TRACE_ZDO1, "set apsde_data_ind cb to %p, was %p", (FMT__P_P, cb, ZG->zdo.af_data_cb));
  ZG->zdo.af_data_cb = cb;
}


void zb_zdo_device_is_unreachable(zb_uint8_t addr_ref)
{
  zb_bufid_t buf = zb_buf_get_out();
  zb_uint16_t short_addr;
  zb_ieee_addr_t ieee_addr;

  TRACE_MSG(TRACE_INFO1, "zb_zdo_device_is_unreachable: addr_ref %hd", (FMT__H, addr_ref));

#if (defined ZB_NWK_MESH_ROUTING && defined ZB_ROUTER_ROLE)
  {
    zb_ret_t ret;
    zb_neighbor_tbl_ent_t *nbt;

    if (zb_nwk_neighbor_get(addr_ref, ZB_FALSE, &nbt) == RET_OK
        && nbt->relationship == ZB_NWK_RELATIONSHIP_PARENT)
    {
      /* WARNING: Remove parent from the neighbors, but stay in the network and do not touch
       * bindings etc. On the next pkt send attempt it will try to discover the route. */
      /* Not clear what functions of router will not work after parent lost. At least, it
       * will not receive unsolicited rejoin. Check/debug all the cases. */

      /* TODO: Need to recheck. Maybe it will be better to initiate rejoin in this case? */

      ZG->nwk.handle.parent = (zb_address_ieee_ref_t)-1;

      ret = zb_nwk_neighbor_delete(addr_ref);
      if (ret != RET_OK)
      {
        TRACE_MSG(TRACE_ERROR, "zb_nwk_neighbor_delete addr_ref not found [%d]", (FMT__D, ret));
        ZB_ASSERT(0);
      }
      zb_buf_free(buf);
      return;
    }
  }
#endif

  if (buf != 0U)
  {
    /* If long addr is known, try to search it by NWK addr req */
    zb_address_ieee_by_ref(ieee_addr, addr_ref);
    if (ZB_IEEE_ADDR_IS_VALID(ieee_addr))
    {
      if (zb_zdo_initiate_nwk_addr_req(buf, ieee_addr) == ZB_ZDO_INVALID_TSN)
      {
        zb_buf_free(buf);
      }
    }
    else
    {
      /* MM: Why do we need to resolve a IEEE address? What is the use case?
       * Isn't it redundant to resolve IEEE address if a child device
       * already gone to another parent?
       */
      /* NK: We need to cover the situation when another device is joined with the same
       * short address. */
      zb_address_short_by_ref(&short_addr, addr_ref);
      if (zb_zdo_initiate_ieee_addr_req(buf, short_addr) == ZB_ZDO_INVALID_TSN)
      {
        zb_buf_free(buf);
      }
    }
  }

  ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_NEIGHBOR_STALE_ID);

  /* If it is not a parent, try to remove it from nbt, but leave bindings etc. */
  ZB_SCHEDULE_CALLBACK2(zdo_device_removed, addr_ref, ZB_FALSE);
}


void zb_apsde_data_acknowledged(zb_uint8_t param)
{
  zb_apsde_data_confirm_t* aps_data_conf;
  zb_ret_t status = zb_buf_get_status(param);

  TRACE_MSG(TRACE_APS3, ">> zb_apsde_data_acknowledged: param %hd", (FMT__H, param));

  aps_data_conf = ZB_BUF_GET_PARAM(param, zb_apsde_data_confirm_t);

  TRACE_MSG(TRACE_APS3, "confirm: dst ep %hd", (FMT__H, aps_data_conf->dst_endpoint));
  TRACE_MSG(TRACE_APS3, "confirm: dst address 0x%x", (FMT__D, aps_data_conf->dst_addr));

#if 1//def ZB_ROUTER_ROLE
  /* For any profile:
   * If it is our neighbor (sure for child, not sure for another types) and status is
   * RET_NO_ACK, we can try to discover the device via nwk/ieee_addr_req - it can be still
   * available.
   * Lets wait some time (ZB_ZDO_NEIGHBOR_FAILURE_PKT_THRESHOLD pkts +
   * ZB_ZDO_NEIGHBOR_UNREACHABLE_TIMEOUT) trying to communicate with device,
   * and then remove it from the neighbor (zb_zdo_device_is_unreachable()). */
  if (ZB_JOINED())
  {
    zb_neighbor_tbl_ent_t *nbt;
    zb_ret_t ret;
    zb_ret_t dst_addr_ref_ret;
    zb_uint8_t dst_addr_ref = 0xFFU;

    if (aps_data_conf->addr_mode == ZB_APS_ADDR_MODE_64_ENDP_PRESENT)
    {
      ret = zb_nwk_neighbor_get_by_ieee(aps_data_conf->dst_addr.addr_long, &nbt);

      dst_addr_ref_ret = zb_address_by_ieee(aps_data_conf->dst_addr.addr_long,
        ZB_FALSE, ZB_FALSE, &dst_addr_ref);
    }
    else
    {
      ret = zb_nwk_neighbor_get_by_short(aps_data_conf->dst_addr.addr_short, &nbt);

      dst_addr_ref_ret = zb_address_by_short(aps_data_conf->dst_addr.addr_short,
        ZB_FALSE, ZB_FALSE, &dst_addr_ref);
    }

    ZB_ASSERT(ret == RET_OK || ret == RET_NOT_FOUND);
    ZB_ASSERT(dst_addr_ref_ret == RET_OK || dst_addr_ref_ret == RET_NOT_FOUND);


    if (ret == RET_OK)
    {
      /* Check for all neighbors, even for the parent (ZR).
         Rationale: Router can work w/o a parent. One case when ZR can legally lost its parent
         is Leave to parent with RemoveChildren=0 (ZR now should not rejoin). Note, that router
         should follow parent-child relationship on the joining, but the case when parent leaves
         without child-router removing is not clearly described.
         Also note, that r21/r22 spec allows to remove router without parent-child relationship
         check (3.6.1.10.3 - Validation on leave request) - so such router after parent lost can
         be successfully removed from the network.

         Now use following scheme:
         - on detecting that neighbor is unreachable, remove it from nbt; if it is the parent -
         remove it and remove internal parent links
         - on parent removing start special timer check that whole network is alive
         - if we do not have a parent and can not say that network is alive (all neighbors are
         aged), initiate rejoin/rejoin_backoff (network may migrate to another channel etc)
      */
        if (!(ZB_IS_DEVICE_ZED() && ZG->nwk.handle.parent == dst_addr_ref))
        {
      if (status == ERROR_CODE(ERROR_CATEGORY_APS,ZB_APS_STATUS_NO_ACK))
      {
        if (nbt->transmit_failure_cnt < ZG->nwk.neighbor.transmit_failure_threshold)
        {
          nbt->transmit_failure_cnt++;
        }
        else
        {
          zb_time_t time_alarm;
          ret = ZB_SCHEDULE_GET_ALARM_TIME(zb_zdo_device_is_unreachable, nbt->u.base.addr_ref, &time_alarm);

          if (ret == RET_NOT_FOUND)
          {
            ZB_SCHEDULE_ALARM(zb_zdo_device_is_unreachable, nbt->u.base.addr_ref, ZG->nwk.neighbor.transmit_failure_timeout * ZB_TIME_ONE_SECOND);
          }
        }
      }
      else                  /* FIXME: only for RET_OK maybe? */
      {
        nbt->transmit_failure_cnt = 0;
        ZB_SCHEDULE_ALARM_CANCEL(zb_zdo_device_is_unreachable, nbt->u.base.addr_ref);
      }
    }
    }
    else
    {
      /* We sure it is not our neighbor. If we know long address, let's try to discover short
       * address immediately. */
      if (dst_addr_ref != 0xFFU
          /*cstat !MISRAC2012-Rule-13.5 */
          /* After some investigation, the following violation of Rule 13.5 seems to be
           * a false positive. There are no side effect to 'zb_buf_get_status()'. This violation
           * seems to be caused by the fact that 'zb_buf_get_status()' is an external function,
           * which cannot be analyzed by C-STAT. */
          && zb_buf_get_status(param) == ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_NO_ACK))
      {
        zb_zdo_device_is_unreachable(dst_addr_ref);
      }
    }
  }
#endif  /* #ifdef ZB_ROUTER_ROLE */

  if (!NCP_CATCH_APS_DATA_CONF(param))
  {
    zb_af_handle_apsde_data_confirm(param);
  }

  TRACE_MSG(TRACE_APS3, "<< zb_apsde_data_acknowledged", (FMT__0));
}

#endif /* !defined NCP_MODE_HOST */

/*! @} */
/*! @endcond */
