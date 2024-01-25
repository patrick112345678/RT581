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
/* PURPOSE: Network layer main module
*/

#define ZB_TRACE_FILE_ID 329
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_mac.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "nwk_internal.h"
#include "zb_secur.h"
#include "zb_watchdog.h"
#include "zb_zdo.h"

#ifndef ZB_ALIEN_MAC
#include "zb_mac_globals.h"
#endif

#if defined ZB_SUB_GHZ_LBT && defined DEBUG
#include "mac_internal.h"
#endif

#if defined ZB_ENABLE_ZLL

#if defined ZB_ZLL_ENABLE_COMMISSIONING_CLIENT
#include "zll/zb_zll_common.h"
#endif /* defined ZB_ZLL_ENABLE_COMMISSIONING_CLIENT */

#endif /* defined ZB_ENABLE_ZLL */


/*! \addtogroup ZB_NWK */
/*! @{ */



void zb_nlme_leave_request(zb_uint8_t param)
{
  zb_ret_t ret;
  zb_neighbor_tbl_ent_t *nbt = NULL;
  zb_nwk_status_t status = ZB_NWK_STATUS_SUCCESS;
  zb_nlme_leave_request_t r;

  ZB_MEMCPY(&r, ZB_BUF_GET_PARAM(param, zb_nlme_leave_request_t), sizeof(r));
  TRACE_MSG(TRACE_NWK1, ">>zb_nlme_leave_request %hd", (FMT__H, param));
  if (!ZB_JOINED())
  {
    TRACE_MSG(TRACE_ERROR, "got leave.request when not joined", (FMT__0));
    ret = RET_ERROR;
    status = ZB_NWK_STATUS_INVALID_REQUEST;
  }
  else if (!ZB_IEEE_ADDR_IS_ZERO(r.device_address)
           && !ZB_64BIT_ADDR_CMP(r.device_address, ZB_PIBCACHE_EXTENDED_ADDRESS()))
  {
#ifdef ZB_ROUTER_ROLE
    if (ZB_IS_DEVICE_ZC_OR_ZR())
    {
      if (zb_nwk_neighbor_get_by_ieee(r.device_address, &nbt) != RET_OK)
      {
        ret = RET_ERROR;
        status = ZB_NWK_STATUS_UNKNOWN_DEVICE;
        TRACE_MSG(TRACE_NWK2, "device is not in the neighbor", (FMT__0));
      }
      else if (nbt->device_type != ZB_NWK_DEVICE_TYPE_ED
#ifdef ZB_CERTIFICATION_HACKS
               && !ZB_CERT_HACKS().enable_leave_to_router_hack
#endif
              )
      {
        /* 3.2.2.16.3
         *
         * On receipt of this primitive by a Zigbee coordinator or
         * Zigbee router and with the DeviceAddress parameter not
         * equal to NULL and not equal to the local device's IEEE
         * address, the NLME determines wether the specified device
         * is in the Neighbor Table and the device type is 0x02
         * (Zigbee End device). If the requested device doesn't exist
         * or the device type is not 0x02, the NLME issues the
         * NLME-LEAVE.confirm primitive with a status of
         * UNKNOWN_DEVICE.
         */
        ret = RET_ERROR;
        TRACE_MSG(TRACE_NWK2, "device is not ED", (FMT__0));
        status = ZB_NWK_STATUS_UNKNOWN_DEVICE;
      }
      else if (nbt->relationship == ZB_NWK_RELATIONSHIP_UNAUTHENTICATED_CHILD)
      {
        /*
         * see 3.6.1.10.2 Method for a Device to Remove Its Child
         * from the Network
         *
         * If the relationship field of the neighbor table entry
         * corresponding to the device being removed has a value of
         * 0x05, indicating that it is an unauthenticated child, the
         * device shall not send a network leave command frame.
         */

        /* Trigger NLME-LEAVE.confirm with status ZB_NWK_STATUS_UNKNOWN_DEVICE */
        TRACE_MSG(TRACE_NWK2, "device is not authenticated - don't send LEAVE to it (status ZB_NWK_STATUS_UNKNOWN_DEVICE)", (FMT__0));
        status = ZB_NWK_STATUS_UNKNOWN_DEVICE;
        ret = RET_ERROR;
      }
      else
      {
        ret = RET_OK;
      }
    }
    else
#endif  /* ZB_ROUTER_ROLE */
    {
      /* If not NULL or device itself and an ED, return an error */
      ret = RET_ERROR;
      TRACE_MSG(TRACE_NWK2, "device is unknown", (FMT__0));
      status = ZB_NWK_STATUS_UNKNOWN_DEVICE;
    }
  }
  else
  {
    ret = RET_OK;
  }

#ifdef ZB_COORDINATOR_ROLE
  if (ZB_IS_DEVICE_ZC() && ret == RET_OK && !nbt)
  {
    TRACE_MSG(TRACE_ERROR, "invalid param for coord", (FMT__0));
    ret = RET_ERROR;
    status = ZB_NWK_STATUS_INVALID_REQUEST;
  }
#endif

  if (ret == RET_OK)
  {
    /*
      a) local leave (device_address is empty/self-address)
      b) force remote device leave

      Anyway, send LEAVE_REQUEST
    */
    zb_bool_t secure = (zb_bool_t)(ZB_NIB_SECURITY_LEVEL());
    zb_nwk_hdr_t *nwhdr;
    zb_uint8_t *lp;

    TRACE_MSG(TRACE_NWK3, "secure %hd", (FMT__H, secure));
    /* NOTE: ret is RET_ERROR if we are router and nbt is NULL. */
    if (nbt == NULL)
    {
      /* if no neighbor table entry, this is leave for us */
      ZG->nwk.leave_context.rejoin_after_leave = r.rejoin;
      nwhdr = nwk_alloc_and_fill_hdr(param,
                                     ZB_PIBCACHE_NETWORK_ADDRESS(),
                                     ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE, /* see 3.4.4.2  */
                                     ZB_FALSE, secure, ZB_TRUE, ZB_TRUE);
    }
#ifdef ZB_ROUTER_ROLE
    else
    {
      zb_uint16_t child_addr;
      zb_address_short_by_ref(&child_addr, nbt->u.base.addr_ref);
      nwhdr = nwk_alloc_and_fill_hdr(param,
                                     ZB_PIBCACHE_NETWORK_ADDRESS(),
                                     child_addr,
                                     ZB_FALSE, secure, ZB_TRUE, ZB_TRUE);
    }
#endif  /* ZB_ROUTER_ROLE */

#ifdef ZB_CERTIFICATION_HACKS
    if(ZB_CERT_HACKS().nwk_leave_from_unknown_addr == ZB_TRUE)
    {
      ZB_IEEE_ADDR_COPY(&nwhdr->src_ieee_addr, ZB_CERT_HACKS().nwk_leave_from_unknown_ieee_addr);
      nwhdr->src_addr = ZB_CERT_HACKS().nwk_leave_from_unknown_short_addr;
    }
#endif
    if (secure)
    {
      nwk_mark_nwk_encr(param);
    }
    /* Don't want it to be routed - see 3.4.4.2 */
    nwhdr->radius = 1;

    lp = (zb_uint8_t *)nwk_alloc_and_fill_cmd(param, ZB_NWK_CMD_LEAVE, (zb_uint8_t)sizeof(zb_uint8_t));
    *lp = 0;
    ZB_LEAVE_PL_SET_REJOIN(*lp, r.rejoin);

#ifdef ZB_ROUTER_ROLE
    // Set "remove_children" if device not ZED only
    ZB_LEAVE_PL_SET_REMOVE_CHILDREN(*lp, ZB_B2U(ZB_U2B(r.remove_children) && (nbt != NULL ? nbt->device_type!=ZB_NWK_DEVICE_TYPE_ED : ZB_TRUE)));
    if (nbt != NULL)
    {
      ZB_LEAVE_PL_SET_REQUEST(*lp);
      TRACE_MSG(TRACE_NWK3, "send leave.request request 1 rejoin %hd remove_children %hd",
                (FMT__H_H, r.rejoin, r.remove_children));
    }
    else
#endif  /* router */
    {
      TRACE_MSG(TRACE_NWK3, "send leave.request request 0 (I am leaving) rejoin %hd", (FMT__H, r.rejoin));

      if (secure && !ZG->aps.authenticated)
      {
        /* See spec 4.6.3.6.3. A device that wishes to leave the
         * network and does not have the active network key shall
         * quietly leave the network without sending a NWK leave announcement.  */
        TRACE_MSG(TRACE_NWK3, "Skip sending, silent leave", (FMT__0));
        ret = RET_ERROR;
        status = ZB_NWK_STATUS_NO_KEY;
      }
    }
  }

  if (ret == RET_OK)
  {
    (void)zb_nwk_init_apsde_data_ind_params(param, ZB_NWK_INTERNAL_LEAVE_CONFIRM_AT_DATA_CONFIRM_HANDLE);
#if (defined ZB_ROUTER_ROLE && defined ZB_NWK_DISTRIBUTED_ADDRESS_ASSIGN)
/* NK: with stochastic addr assign is used, this check will never be ok */
    if(nbt)
    {
      zb_uint16_t child_addr;
      zb_address_short_by_ref(&child_addr, nbt->addr_ref);

      /* if we removed last joined device, we could decrease number of child to */
      /* to save some address space */

      /* TODO: check for child - ZR? */
      if (child_addr == ZB_NWK_ED_ADDRESS_ASSIGN()-1)
      {
        ZB_NIB().ed_child_num--;
      }
    }
#endif
    ZB_SCHEDULE_CALLBACK(zb_nwk_forward, param);
  }
  else
  {
    zb_nlme_leave_confirm_t *lc;

    TRACE_MSG(TRACE_NWK1, "leave.request failed %hd", (FMT__H, status));
    (void)zb_buf_reuse(param);
    lc = zb_buf_alloc_tail(param, sizeof(*lc));
    lc->status = status;
    ZB_IEEE_ADDR_COPY(lc->device_address, r.device_address);
    ZB_SCHEDULE_CALLBACK(zb_nlme_leave_confirm, param);
  }
  TRACE_MSG(TRACE_NWK1, "<<zb_nlme_leave_request %d", (FMT__D, ret));
}


//static void nwk_internals_clear(void)
void nwk_internals_clear(void)
{
#if defined ZB_NWK_MESH_ROUTING && defined ZB_ROUTER_ROLE
  if (ZB_IS_DEVICE_ZC_OR_ZR())
  {
    NWK_ARRAY_CLEAR(ZB_NIB().routing_table, ZB_NWK_ROUTING_TABLE_SIZE, ZB_NIB().routing_table_cnt);
    NWK_ARRAY_CLEAR(ZB_NIB().route_disc_table, ZB_NWK_ROUTE_DISCOVERY_TABLE_SIZE, ZB_NIB().route_disc_table_cnt);
    nwk_clear_pending_table();
    zb_nwk_broadcasting_clear();
  }
#endif
}

void zb_nlme_reset_request(zb_uint8_t param)
{
  zb_nlme_reset_request_t *request = ZB_BUF_GET_PARAM(param, zb_nlme_reset_request_t);
  zb_bool_t no_nib_reinit = request->no_nib_reinit;

  TRACE_MSG(TRACE_NWK1, ">>reset_req %hd", (FMT__H, param));

  /* EE refactored nlme_reset_request to confirm specification. Remove hacks. */

/*
  Where WarmStart is set to TRUE, the network layer should not modify
  any NIB values, but rather should resume normal network operations and
  consider itself part of the network specified in the NIB. Routing
  table values and neighbor table values should be cleared.
*/

  ZB_ASSERT(ZB_NWK_MAC_IFACE_TBL_SIZE == 1U);
  /* SS: TODO: loop over all enabled MAC ifaces when their count becomes more than 1. */

  if (request->warm_start)
  {
    TRACE_MSG(TRACE_NWK1, "zb_nlme_reset_request - warm start", (FMT__0));

    nwk_internals_clear();
    /* clear neighbor table */
    if (!no_nib_reinit)
    {
      /* At least should not clear extneighbors! */
      zb_nwk_neighbor_clear();
    }

#ifdef ZB_ROUTER_ROLE
    /* Run ED aging after reset */
    zb_nwk_restart_aging();
#endif /* ZB_ROUTER_ROLE */

    /* call higher layers */
    ZG->nwk.handle.state = ZB_NLME_STATE_IDLE;
    /* zb_nlme_reset_confirm does not check for any status */
    ZB_SCHEDULE_CALLBACK(zb_nlme_reset_confirm, param);
  }
  /*
    Where the WarmStart parameter has a value of FALSE, the NLME issues
    the MLME-RESET.request primitive to each MAC sub-layer with an entry
    in the nwkMacInterfaceTable with the SetDefaultPIB parameter set to
    TRUE. On receipt of the corresponding MLME-RESET.confirm primitive,
    the NWK layer resets itself by clearing all internal variables,
    routing table and route discovery table entries and by set-ting all
    NIB attributes to their default values.
  */
  else
  {
    zb_mlme_reset_request_t *req = ZB_BUF_GET_PARAM(param, zb_mlme_reset_request_t);

    TRACE_MSG(TRACE_NWK1, "zb_nlme_reset_request - cold start, no_nib_reinit %d", (FMT__D, no_nib_reinit));
    ZB_BZERO(req, sizeof(zb_mlme_reset_request_t));
    req->set_default_pib = ZB_TRUE;
    /* TODO: reset all MAC interfaces */
    ZB_SCHEDULE_CALLBACK(zb_mlme_reset_request, param);

    /* We are leaving network after authentication failure - do not
     * clear nib in this case */
    if (no_nib_reinit)
    {
      /* Can be here after authentication failure and leave, also at
         reboot.  Need to reser MAC but keep NIB, addresses and
         neighbor table: we read it from NVRAM, so keep it.
       */
      ZG->nwk.handle.state = ZB_NLME_STATE_RESET_NO_NIB_REINIT;
    }
    else
    {
      ZG->nwk.handle.state = ZB_NLME_STATE_RESET;
    }
  }

  TRACE_MSG(TRACE_NWK1, "<<reset_req", (FMT__0));
}


void zb_mlme_reset_confirm(zb_uint8_t param)
{
  TRACE_MSG(TRACE_NWK1, ">>reset_cnfrm %hd state %hd", (FMT__H_H, param, ZG->nwk.handle.state));

  if (ZG->nwk.handle.state != ZB_NLME_STATE_RESET_NO_NIB_REINIT)
  {
#ifndef ZB_ED_RX_OFF_WHEN_IDLE
    zb_uint8_t rx_on_when_idle = ZB_PIBCACHE_RX_ON_WHEN_IDLE();
#endif
    /* reinit nib */
      zb_nwk_nib_init(ZB_FALSE);
#ifndef ZB_ED_RX_OFF_WHEN_IDLE
      ZB_PIBCACHE_RX_ON_WHEN_IDLE() = rx_on_when_idle;
#endif
    /* clear neighbor table */
    zb_nwk_neighbor_clear();
  }
  nwk_internals_clear();
  /* clear all internal variables */
  ZB_BZERO(&ZG->nwk.handle, sizeof(ZG->nwk.handle));
  ZB_RESYNC_CFG_MEM();

  //ZG->nwk.handle.state = ZB_NLME_STATE_IDLE; set by bzero

#ifdef ZB_ROUTER_ROLE
  /* If it is already started, we need to restart ed aging
     because we have cleared nwk state variables. */
  /* Run ED aging after reset */
  zb_nwk_restart_aging();
#endif /* ZB_ROUTER_ROLE */

  /* call higher layers */
  /* zb_nlme_reset_confirm does not check for any status */
  ZB_SCHEDULE_CALLBACK(zb_nlme_reset_confirm, param);

  TRACE_MSG(TRACE_NWK1, "<<reset_cnfrm state %hd", (FMT__H, ZG->nwk.handle.state));
}


#ifdef ZB_ED_FUNC
#if !defined ZB_LITE_NO_ZDO_POLL
void zb_nlme_sync_request(zb_uint8_t param)
{
  TRACE_MSG(TRACE_NWK1, ">>sync_req %hd", (FMT__H, param));

  if (!ZB_JOINED()
      || ZG->nwk.handle.poll_in_progress)
  {
    TRACE_MSG(TRACE_NWK1, "joined %hd, poll_in_progress %hd: not joined or nested sync",
              (FMT__H_H, ZB_JOINED(), ZG->nwk.handle.poll_in_progress));
    zb_buf_set_status(param, (zb_ret_t)ZB_NWK_STATUS_INVALID_REQUEST);
    ZB_SCHEDULE_CALLBACK(zb_nlme_sync_confirm, param);
  }
  else
  {
    /* call mlme-poll */
    zb_mlme_poll_request_t *req = ZB_BUF_GET_PARAM(param, zb_mlme_poll_request_t);
    zb_uint16_t short_addr = 0;
    zb_time_t poll_rate = ZB_BUF_GET_PARAM(param, zb_nlme_sync_request_t)->poll_rate;

    ZG->nwk.handle.poll_in_progress = ZB_TRUE;
    zb_address_short_by_ref(&short_addr, ZG->nwk.handle.parent);
    req->coord_addr.addr_short = short_addr;
    req->coord_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
    req->coord_pan_id = ZB_PIBCACHE_PAN_ID();
    req->poll_rate = poll_rate;
    ZB_SCHEDULE_CALLBACK(zb_mlme_poll_request, param);
  }

  TRACE_MSG(TRACE_NWK1, "<<sync_req", (FMT__0));
}
#endif


void zb_mlme_poll_confirm(zb_uint8_t param)
{
  zb_ret_t status = zb_buf_get_status(param);
  TRACE_MSG(TRACE_NWK1, ">>zb_mlme_poll_confirm %hd", (FMT__H, param));

  ZG->nwk.handle.poll_in_progress = ZB_FALSE;

  TRACE_MSG(TRACE_NWK1, "state %hd status 0x%hx",
            (FMT__H_H, ZG->nwk.handle.state, status));

  if ( status == (zb_ret_t)MAC_SUCCESS
       || status == (zb_ret_t)MAC_NO_DATA
       || status == (zb_ret_t)MAC_NO_ACK)
  {
    /* Device successfully sent Data Request to coordinator */
    ZB_KICK_WATCHDOG(ZB_WD_ZB_TRAFFIC);
  }

  if (ZG->nwk.handle.state == ZB_NLME_STATE_REJOIN)
  {
    if (ZG->nwk.handle.tmp.rejoin.poll_req != 0U)
    {
      nwk_next_rejoin_poll(param);
    }
    else
    {
      /* do nothing */
      zb_buf_free(param);
    }
  }
#if !defined ZB_LITE_NO_ZDO_POLL
  else
  {
    /* call sync confirm with status ret by poll */
    ZB_SCHEDULE_CALLBACK(zb_nlme_sync_confirm, param);
  }
#endif

  TRACE_MSG(TRACE_NWK1, "<<zb_mlme_poll_confirm", (FMT__0));
}


void nwk_next_rejoin_poll(zb_uint8_t param)
{
  if (ZG->nwk.handle.poll_in_progress)
  {
    ZG->nwk.handle.tmp.rejoin.poll_req = 1;
  }
  else if (param == 0U)
  {
    zb_ret_t ret = zb_buf_get_out_delayed(nwk_next_rejoin_poll);
    if (ret != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
    }
  }
  else
  {
    /* call mlme-poll */
    zb_mlme_poll_request_t *req = ZB_BUF_GET_PARAM(param, zb_mlme_poll_request_t);

    ZG->nwk.handle.poll_in_progress = ZB_TRUE;
    ZG->nwk.handle.tmp.rejoin.poll_req = 0;
    ZG->nwk.handle.tmp.rejoin.poll_attempts++;
    req->coord_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
    req->coord_addr.addr_short = ZG->nwk.handle.tmp.rejoin.parent->u.ext.short_addr;
    req->coord_pan_id = ZB_PIBCACHE_PAN_ID();
    req->poll_rate = ZB_NWK_REJOIN_FIRST_POLL_DELAY;
    TRACE_MSG(TRACE_NWK1, "calling zb_mlme_poll_request", (FMT__0));
    if (ZG->nwk.handle.tmp.rejoin.poll_attempts == 1U)
    {
      ZB_SCHEDULE_ALARM(zb_mlme_poll_request, param, ZB_NWK_REJOIN_FIRST_POLL_DELAY);
    }
    else
    {
      ZB_SCHEDULE_CALLBACK(zb_mlme_poll_request, param);
    }
    param = 0;
  }

  if (param != 0U)
  {
    zb_buf_free(param);
  }
}
#endif  /* ZB_ED_FUNC */


void zb_nlme_get_request(zb_uint8_t param)
{
  zb_nwk_status_t status = ZB_NWK_STATUS_SUCCESS;
  zb_nlme_get_confirm_t *conf;
  zb_nlme_get_request_t req;
  zb_uint8_t v8 = 0;
  zb_bool_t is_v8;

  TRACE_MSG(TRACE_NWK1, ">>zb_nlme_get_request %hd", (FMT__H, param));
  ZB_MEMCPY(&req, ( zb_nlme_get_request_t *)zb_buf_begin(param), sizeof(req));

  /* Most attributes are single-byte */
  is_v8 = ZB_TRUE;
  switch (req.nib_attribute)
  {
    case ZB_NIB_ATTRIBUTE_SEQUENCE_NUMBER:
      v8 = ZB_NIB_SEQUENCE_NUMBER();
      break;
    case ZB_NIB_ATTRIBUTE_MAX_BROADCAST_RETRIES:
      v8 = ZB_NIB().max_broadcast_retries;
      break;
#ifdef ZB_ROUTER_ROLE
    case ZB_NIB_ATTRIBUTE_MAX_CHILDREN:
      v8 = ZB_NIB().max_children;
      break;
    case ZB_NIB_ATTRIBUTE_MAX_DEPTH:
      v8 = ZB_NIB_MAX_DEPTH();
      break;
#ifdef ZB_NWK_DISTRIBUTED_ADDRESS_ASSIGN
    case ZB_NIB_ATTRIBUTE_MAX_ROUTERS:
      v8 = ZB_NIB().max_routers;
      break;
#endif
#endif  /* ZB_ROUTER_ROLE */
    case ZB_NIB_ATTRIBUTE_ADDR_ALLOC:
      v8 = ZB_NIB().addr_alloc;
      break;
#ifdef ZB_NWK_TREE_ROUTING
      /* can anybody ever need it? */
    case ZB_NIB_ATTRIBUTE_USE_TREE_ROUTING:
      v8 = ZB_NIB().use_tree_routing;
      break;
#endif
    case ZB_NIB_ATTRIBUTE_UPDATE_ID:
      v8 = ZB_NIB().update_id;
      break;
    case ZB_NIB_ATTRIBUTE_SECURITY_LEVEL:
      v8 = ZB_NIB_SECURITY_LEVEL();
      break;
    case ZB_NIB_ATTRIBUTE_SECURITY_MATERIAL_SET:
      v8 = ZB_NIB().active_secur_material_i;
      break;
    case ZB_NIB_ATTRIBUTE_ACTIVE_KEY_SEQ_NUMBER:
      v8 = ZB_NIB().active_key_seq_number;
      break;
    case ZB_NIB_ATTRIBUTE_USE_MULTICAST:
      /* No NWK multicast from r21 */
      v8 = 0;
      break;
    case ZB_NIB_ATTRIBUTE_LEAVE_REQ_ALLOWED:
      v8 = ZB_NIB().leave_req_allowed;
      break;
    default:
      is_v8 = ZB_FALSE;
      break;
  }

  if (is_v8)
  {
    conf = zb_buf_initial_alloc(param, sizeof(*conf) + 1U);
    conf->attribute_length = 1;
    ((zb_uint8_t*)(&conf[1]))[0] = v8;
  }
  else
  {
    switch (req.nib_attribute)
    {
      case ZB_NIB_ATTRIBUTE_PASSIVE_ASK_TIMEOUT:
        conf = zb_buf_initial_alloc(param, sizeof(zb_nlme_get_confirm_t) + sizeof(zb_uint16_t));
        conf->attribute_length = (zb_uint16_t)sizeof(zb_uint16_t);
        ZB_MEMCPY(&conf[1], &ZB_NIB().passive_ack_timeout, sizeof(zb_uint16_t));
        break;
      case ZB_NIB_ATTRIBUTE_MANAGER_ADDR:
        conf = zb_buf_initial_alloc(param, sizeof(zb_nlme_get_confirm_t) + sizeof(zb_uint16_t));
        conf->attribute_length = (zb_uint16_t)sizeof(zb_uint16_t);
        ZB_MEMCPY(&conf[1], &ZB_NIB().nwk_manager_addr, sizeof(zb_uint16_t));
        break;
      case ZB_NIB_ATTRIBUTE_EXTENDED_PANID:
        conf = zb_buf_initial_alloc(param, sizeof(zb_nlme_get_confirm_t) + sizeof(zb_ext_pan_id_t));
        conf->attribute_length = (zb_uint16_t)sizeof(zb_ext_pan_id_t);
        ZB_64BIT_ADDR_COPY(&conf[1], ZB_NIB().extended_pan_id);
        break;
      default:
        conf = zb_buf_initial_alloc(param, sizeof(zb_nlme_get_confirm_t));
        conf->attribute_length = 0;
        status = ZB_NWK_STATUS_UNSUPPORTED_ATTRIBUTE;
        break;
    }
  }

  conf->status = status;
  conf->nib_attribute = req.nib_attribute;
  if (req.confirm_cb != NULL)
  {
    TRACE_MSG(TRACE_APS2, "call NLME-GET.confirm %hd status %hd", (FMT__H_H, param, status));
    ZB_SCHEDULE_CALLBACK(req.confirm_cb, param);
  }
  else
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_NWK1, "<<zb_nlme_get_request", (FMT__0));
}


void zb_nlme_set_request(zb_uint8_t param)
{
  zb_nlme_set_request_t *req = zb_buf_begin(param);
  zb_uint8_t *ptr = (zb_uint8_t *)req;
  zb_nlme_set_confirm_t conf;
  zb_nlme_set_confirm_t *conf_p;
  zb_callback_t confirm_cb;

  TRACE_MSG(TRACE_NWK1, ">>zb_nlme_set_request %hd", (FMT__H, param));

  conf.nib_attribute = req->nib_attribute;
  conf.status = ZB_NWK_STATUS_SUCCESS;
  ptr += sizeof(zb_nlme_set_request_t);
  switch (req->nib_attribute)
  {
    case ZB_NIB_ATTRIBUTE_SEQUENCE_NUMBER:
      ZB_NIB_SEQUENCE_NUMBER() = *ptr;
      break;
    case ZB_NIB_ATTRIBUTE_PASSIVE_ASK_TIMEOUT:
      ZB_MEMCPY(&ZB_NIB().passive_ack_timeout, ptr, sizeof(ZB_NIB().passive_ack_timeout));
      break;
    case ZB_NIB_ATTRIBUTE_MAX_BROADCAST_RETRIES:
      ZB_NIB().max_broadcast_retries = *ptr;
      break;
    case ZB_NIB_ATTRIBUTE_MAX_DEPTH:
      ZB_NIB_MAX_DEPTH() = *ptr;
      break;
#if defined ZB_NWK_DISTRIBUTED_ADDRESS_ASSIGN && defined ZB_ROUTER_ROLE
    case ZB_NIB_ATTRIBUTE_MAX_CHILDREN:
      ZB_NIB().max_children = *ptr;
      break;
    case ZB_NIB_ATTRIBUTE_MAX_ROUTERS:
      ZB_NIB().max_routers = *ptr;
      break;
#endif
    case ZB_NIB_ATTRIBUTE_ADDR_ALLOC:
      ZB_NIB().addr_alloc = *ptr;
      break;
#ifdef ZB_NWK_TREE_ROUTING
    case ZB_NIB_ATTRIBUTE_USE_TREE_ROUTING:
      ZB_NIB().use_tree_routing = *ptr;
      break;
#endif
    case ZB_NIB_ATTRIBUTE_MANAGER_ADDR:
      ZB_MEMCPY(&ZB_NIB().nwk_manager_addr, ptr, sizeof(ZB_NIB().nwk_manager_addr));
      break;
    case ZB_NIB_ATTRIBUTE_UPDATE_ID:
      ZB_NIB().update_id = *ptr;
      break;
    case ZB_NIB_ATTRIBUTE_EXTENDED_PANID:
      ZB_64BIT_ADDR_COPY(ZB_NIB().extended_pan_id, ptr);
      break;
    case ZB_NIB_ATTRIBUTE_SECURITY_LEVEL:
      ZB_SET_NIB_SECURITY_LEVEL(*ptr);
      break;
    case ZB_NIB_ATTRIBUTE_SECURITY_MATERIAL_SET:
      ZB_NIB().active_secur_material_i = *ptr;
      break;
    case ZB_NIB_ATTRIBUTE_ACTIVE_KEY_SEQ_NUMBER:
      ZB_NIB().active_key_seq_number = *ptr;
      break;
#ifdef ZB_PRO_STACK
#ifndef ZB_NO_NWK_MULTICAST
    case ZB_NIB_ATTRIBUTE_USE_MULTICAST:
      ZB_NIB().nwk_use_multicast = *ptr;
      break;
#endif
    case ZB_NIB_ATTRIBUTE_LEAVE_REQ_ALLOWED:
      ZB_NIB().leave_req_allowed = *ptr;
      break;
#endif
    default:
      conf.status = ZB_NWK_STATUS_UNSUPPORTED_ATTRIBUTE;
      break;
  }

  confirm_cb = req->confirm_cb;
  conf_p = zb_buf_initial_alloc(param, sizeof(zb_nlme_set_confirm_t));
  ZB_MEMCPY(conf_p, &conf, sizeof(*conf_p));
  if (confirm_cb != NULL)
  {
    TRACE_MSG(TRACE_APS2, "call NLME-SET.confirm %hd status %hd", (FMT__H_H, param, conf.status));
    ZB_SCHEDULE_CALLBACK(confirm_cb, param);
  }
  else
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_NWK1, "<<zb_nlme_set_request", (FMT__0));
}


#ifdef ZB_CHECK_OOM_STATUS
/**
 * Checks that "out of memory" (OOM) happened (no available buffers).
 *
 * If bufpool is in OOM state for a period, specified by ZB_OOM_THRESHOLD,
 * then assertion is triggered.
 *
 */
void zb_check_oom_status(zb_uint8_t param)
{
  /* TRACE_MSG(TRACE_NWK3, ">> zb_check_oom_status param %hd", (FMT__H, param)); */
#if defined ZB_PROMISCUOUS_MODE && defined ZB_DIRECT_PIB_ACCESS
  if (MAC_PIB().mac_promiscuous_mode)
  {
    return;
  }
#endif

  ZVUNUSED(param);
  if (zb_buf_is_oom_state())
  {
    /* Do not spam too much with this line! We possibly can not handle timer interrupts etc because
     * of non-stop tracing of this line! Moreover, we have special trace for OOM detecting/lefting,
     * so it is useless to print this all the time. */
    /* TODO: Maybe it is better to turn off all trace levels (except error) during OOM. */
    /* TRACE_MSG(TRACE_ERROR, "ACHTUNG: Out of memory!", (FMT__0)); */

    if (!ZG->nwk.oom_presents)
    {
      TRACE_MSG(TRACE_ERROR, "OOM state is detected. ACHTUNG: Out of memory!", (FMT__0));
#if defined(ZB_TRACE_LEVEL)
      zb_buf_oom_trace();
#endif

      ZG->nwk.oom_presents = ZB_TRUE;

      ZG->nwk.oom_timestamp = ZB_TIMER_GET();

#ifdef ZB_SEND_OOM_STATUS
      if (ZG->nwk.oom_status == ZB_OOM_STATUS_NOT_SENT)
      {
        // Send OOM status for the first time with no delay.
        ZG->nwk.oom_last_sent = ZB_TIME_SUBTRACT(ZB_TIMER_GET(), ZB_SEND_OOM_DELAY);
      }
#endif
    }

    if (ZB_TIME_SUBTRACT(ZB_TIMER_GET(), ZG->nwk.oom_timestamp) > ZB_OOM_THRESHOLD)
    {
#if defined(ZB_TRACE_LEVEL)
      zb_buf_oom_trace();
#endif
      ZB_SCHEDULE_TRACE_QUEUE();
#ifdef ZB_OOM_REBOOT
      ZB_ASSERT(0);
#endif
    }

    /*
     * use timer to limit OOM sending
     */

#ifdef ZB_SEND_OOM_STATUS
    /* If you want to send status again during one OOM transaction, set oom_status to NOT_SENT.
     * (Make delay before new sending if you want). */
    if ((ZG->nwk.oom_status == ZB_OOM_STATUS_NOT_SENT ||
         ZG->nwk.oom_status == ZB_OOM_STATUS_SENT_CONFIRMED) && ZG->nwk.oom_status_buf_ref &&
        ZB_TIME_SUBTRACT(ZB_TIMER_GET(), ZG->nwk.oom_last_sent) >= ZB_SEND_OOM_DELAY)
    {
      zb_bufid_t buf = ZG->nwk.oom_status_buf_ref;
      zb_nlme_send_status_t *req = NULL;
      TRACE_MSG(TRACE_ERROR, "ACHTUNG: Sending OOM status", (FMT__0));
      req = ZB_BUF_GET_PARAM(buf, zb_nlme_send_status_t);
      req->dest_addr = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
      req->status.status = 0xdb; /* ZB_NWK_COMMAND_STATUS_CUSTOM_OOM */
      req->status.network_addr = ZB_PIBCACHE_NETWORK_ADDRESS();
      /* Q: Maybe it will be good to define new handle to prevent broadcasting more than one
       * OMM Status simultaneously to releif OOM state?
       A: only 1 OOM packet is sent at the moment due to using
       oom_status_buf_ref variable */
      req->ndsu_handle = ZB_NWK_INTERNAL_OMM_STATUS_CONFIRM_HANDLE;
      ZB_SCHEDULE_CALLBACK(zb_nlme_send_status, ZG->nwk.oom_status_buf_ref);

      ZG->nwk.oom_status = ZB_OOM_STATUS_SENT_NOT_CONFIRMED;
      ZG->nwk.oom_last_sent = ZB_TIMER_GET();
    }
#endif
  }
  else
  {
    if (ZG->nwk.oom_presents)
    {
      TRACE_MSG(TRACE_ERROR, "OOM state ends", (FMT__0));
      ZG->nwk.oom_presents = ZB_FALSE;
    }

    /* Allocate oom buf. That case includes allocate at start */
#ifdef ZB_SEND_OOM_STATUS
    if (!ZG->nwk.oom_status_buf_ref)
    {
      zb_bufid_t buf;
      /* Please set oom_status to ether NOT_SENT or SENT_CONFIRMED
       * when you zero oom_status_buf_ref. */
      ZB_ASSERT(ZG->nwk.oom_status == ZB_OOM_STATUS_NOT_SENT ||
                ZG->nwk.oom_status == ZB_OOM_STATUS_SENT_CONFIRMED);

      buf = zb_buf_get_any();
      if (buf)
      {
        ZG->nwk.oom_status_buf_ref = buf;

        ZG->nwk.oom_status = ZB_OOM_STATUS_NOT_SENT;
      }
    }
#endif
  }

  /* TRACE_MSG(TRACE_NWK3, "<< zb_check_oom_status", (FMT__0)); */
}

#ifdef ZB_SEND_OOM_STATUS
void zb_oom_status_confirm(zb_uint8_t param)
{
  TRACE_MSG(TRACE_NWK3, ">> zb_oom_status_confirm param %hd", (FMT__H, param));

  ZB_ASSERT(param);

  if (ZG->nwk.oom_status_buf_ref == param)
  {
    ZB_ASSERT(ZG->nwk.oom_status == ZB_OOM_STATUS_SENT_NOT_CONFIRMED);

    TRACE_MSG(TRACE_NWK3, "confirm to sending oom status is received.", (FMT__0));

    ZG->nwk.oom_status = ZB_OOM_STATUS_SENT_CONFIRMED;

    /* Do not release buf if you want to use this buffer later to send OOM status again. */

    /* Releif oom_status by releasing oom_status_buf_ref.
     *
     * zb_free_buf(ZB_BUF_FROM_REF(ZG->nwk.oom_status_buf_ref));
     * ZG->nwk.oom_status_buf_ref = 0;
     *
     * There is possibility to obtain this buffer in the next zb_check_oom_status call
     * (This case oom_timestamp is reset -> we can not reboot for eternity).
     */
  }
  else
  {
    TRACE_MSG(TRACE_NWK3, "release buffer not equal to oom_status_buf_ref", (FMT__0));

    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_NWK3, "<< zb_oom_status_confirm", (FMT__0));
}
#endif  /* ZB_SEND_OOM_STATUS */
#endif  /* ZB_CHECK_OOM_STATUS */


#if 1
/* we need this command available to all devices so that they could transmit Unknown command status */
//defined ZB_ROUTER_ROLE || defined ZB_SEND_OOM_STATUS
void zb_nlme_send_status(zb_uint8_t param)
{
  zb_nlme_send_status_t *request = ZB_BUF_GET_PARAM(param, zb_nlme_send_status_t);
  zb_nlme_status_indication_t *status_cmd;
  zb_nwk_hdr_t *nwhdr;
  zb_uint_t status_len;
  zb_uint8_t nsdu_handle;
  zb_bool_t secure = (zb_bool_t)(ZB_NIB_SECURITY_LEVEL());

  TRACE_MSG(TRACE_NWK1, ">> zb_nlme_send_status param %hd", (FMT__H, param));
  TRACE_MSG(TRACE_ATM1, "Z< send nwk status to 0x%04x, status = 0x%02x, dst_addr = 0x%04x", (FMT__D_D, request->dest_addr, request->status.status, request->status.network_addr));

  nwhdr = nwk_alloc_and_fill_hdr(param, ZB_PIBCACHE_NETWORK_ADDRESS(), request->dest_addr, ZB_FALSE, secure, ZB_TRUE, ZB_FALSE);
  nwhdr->radius = ZB_NIB_MAX_DEPTH() * 2U;
  ZB_NWK_FRAMECTL_SET_DISCOVER_ROUTE(nwhdr->frame_control, 1U);

  if (secure)
  {
    nwk_mark_nwk_encr(param);
  }

  if (ZB_NWK_IS_ADDRESS_BROADCAST(request->status.network_addr))
  {
  /* The destination address field is 2 octets in length and shall be present
   * if, and only if, the network status command frame is being sent in
   * response to a routing failure. In this case, it shall contain the
   * destination address from the data frame that encountered the failure. */

    /* minus two bytes for dest address and minus one byte for unknown cmd*/
    status_len = sizeof(zb_nlme_status_indication_t) - 3U;
  }
  else if (request->status.status != ZB_NWK_COMMAND_STATUS_UNKNOWN_COMMAND)
  {
    /* we do not need to pass unknown command id
       in commands other than ZB_NWK_COMMAND_STATUS_UNKNOWN_COMMAND */
    status_len = sizeof(zb_nlme_status_indication_t) - 1U;
  }
  else
  {
    status_len = sizeof(zb_nlme_status_indication_t);
  }

  status_cmd = (zb_nlme_status_indication_t *)nwk_alloc_and_fill_cmd(param, ZB_NWK_CMD_NETWORK_STATUS, (zb_uint8_t)status_len);
  status_cmd->status = request->status.status;
  if (!ZB_NWK_IS_ADDRESS_BROADCAST(request->status.network_addr))
  {
    status_cmd->network_addr = request->status.network_addr;
    ZB_NWK_ADDR_TO_LE16(status_cmd->network_addr);

    if (request->status.status == ZB_NWK_COMMAND_STATUS_UNKNOWN_COMMAND)
    {
      status_cmd->unknown_command_id = request->status.unknown_command_id;
    }
  }

  nsdu_handle = request->ndsu_handle;

  /* transmit route request packet */
  (void)zb_nwk_init_apsde_data_ind_params(param, nsdu_handle);
  ZB_SCHEDULE_CALLBACK(zb_nwk_forward, param);

  TRACE_MSG(TRACE_NWK1, "<< zb_nlme_send_status", (FMT__0));
}
#endif

#if defined ZB_ASSERT_SEND_NWK_REPORT
void zb_nlme_send_assert_ind(zb_uint16_t file_id, zb_int_t line)
{
  zb_nwk_report_cmd_t *rep;
  zb_nwk_hdr_t* nwhdr;
  zb_uint32_t assert_frame_sig = 0xdeadbeef; /* assert signature */

  /* NWK report data */
  if (ZB_PIBCACHE_NETWORK_ADDRESS() == ZB_NWK_BROADCAST_ALL_DEVICES)
  {
    ZB_PIBCACHE_NETWORK_ADDRESS() = 0x1234;
  }

  nwhdr = nwk_alloc_and_fill_hdr(param, ZB_PIBCACHE_NETWORK_ADDRESS(), ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE,
                                 ZB_FALSE, ZB_FALSE, ZB_TRUE, ZB_TRUE);
  nwhdr->radius = (zb_uint8_t)(ZB_NIB_MAX_DEPTH() * 2);

  rep = (zb_nwk_report_cmd_t*)nwk_alloc_and_fill_cmd(param, ZB_NWK_CMD_NETWORK_REPORT, sizeof(zb_nwk_report_cmd_t));
  rep->command_options = 0xE0;
  /* Options are: bits 0-4 - counter, 5-7 - report command id (only 0 is standard) - so we use 7 as command id. */

  ZB_MEMCPY(rep->epid, &assert_frame_sig, sizeof(zb_uint32_t));
  ZB_MEMCPY(((zb_uint8_t*)rep->epid) + 4, &file_id, sizeof(zb_uint16_t));
  ZB_MEMCPY(((zb_uint8_t*)rep->epid) + 6, &line, sizeof(zb_uint16_t));

  /* Bypass nwk_forward to send frame immediately. */
  zb_mcps_build_data_request(ZG->nwk.oom_status_buf_ref,
                             ZB_PIBCACHE_NETWORK_ADDRESS(), 0xffff,
                             ((0) | /* not indirect */
                             (0x0)), /* does not want ack */
                             0);      /* handle */
#ifndef ZB_ALIEN_MAC
  MAC_CTX().flags.tx_ok_to_send = ZB_TRUE;
  MAC_CTX().flags.tx_q_busy = ZB_FALSE;
  /* Bypass scheduler to send frame immediately. */
  zb_mcps_data_request_fill_hdr(buf);
  zb_handle_mcps_data_req(ZG->nwk.oom_status_buf_ref);
#else
  zb_mcps_data_request(ZG->nwk.oom_status_buf_ref);
#endif
}
#endif

#ifdef DEBUG
void zb_debug_bcast_key(zb_uint8_t *peer_addr, zb_uint8_t key[ZB_CCM_KEY_SIZE])
{
  zb_bufid_t buf = zb_buf_get_out();
  zb_nwk_report_cmd_t *rep;
  zb_nwk_hdr_t* nwhdr;

  if (buf != 0U)
  {
#if defined(SNCP_MODE) && defined(ZB_HAVE_BLOCK_KEYS)
    if (zb_chip_lock_get())
    {
      zb_buf_free(buf);
      return;
    }
#endif /* SNCP_MODE && ZB_HAVE_BLOCK_KEYS */

    /* NWK report data */
    nwhdr = nwk_alloc_and_fill_hdr(buf, ZB_PIBCACHE_NETWORK_ADDRESS(), ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE,
                                   ZB_FALSE, ZB_FALSE, ZB_TRUE, ZB_FALSE);
    nwhdr->radius = 1;

    /* epid - 8b, then panids[1] */
    rep = (zb_nwk_report_cmd_t*)nwk_alloc_and_fill_cmd(buf, ZB_NWK_CMD_NETWORK_REPORT, (zb_uint8_t)sizeof(zb_nwk_report_cmd_t) + (ZB_CCM_KEY_SIZE - 2U));
    rep->command_options = 6U << 5;
    /* Options are: bits 0-4 - counter, 5-7 - report command id (only 0 is standard) - so we use here 6 as a command id. */

    if (peer_addr != NULL)
    {
      ZB_IEEE_ADDR_COPY(rep->epid, peer_addr);
    }
    else
    {
      ZB_BZERO(rep->epid, sizeof(rep->epid));
    }
    /* put key into panids, so not patched Wireshark shows it as 'data' */
    ZB_MEMCPY((zb_uint8_t *)rep->panids, key, ZB_CCM_KEY_SIZE);

    /* Bypass nwk_forward to send frame immediately. */
    zb_mcps_build_data_request(buf,
                               ZB_PIBCACHE_NETWORK_ADDRESS(), 0xffff,
                               ((0U) | /* not indirect */
                                (0x00U)), /* does not want ack */
                               0);      /* handle */
#ifdef ZB_SUB_GHZ_LBT
    if (ZB_U2B(ZB_PIBCACHE_CURRENT_PAGE()))
    {
      zb_time_t jitter = ZB_MAC_LBT_TX_MIN_OFF_MS + ZB_RANDOM_JTR(ZB_MAC_LBT_TX_MIN_OFF_MS * 2U);
      ZB_SCHEDULE_ALARM(zb_mcps_data_request,
                        buf,
                        ZB_MILLISECONDS_TO_BEACON_INTERVAL(jitter));
    }
    else
#endif /* ZB_SUB_GHZ_LBT */
    {
      ZB_SCHEDULE_CALLBACK(zb_mcps_data_request, buf);
    }
  }
}
#endif

/**
   Load PIB values into NIB cache
 */
void zb_nwk_load_pib(zb_uint8_t param)
{
  ZG->nwk.handle.state = ZB_NLME_STATE_PIB_LOAD1;
  ZB_SCHEDULE_CALLBACK(zb_nwk_load_pib_stm, param);
}


void zb_nwk_pib_set(zb_uint8_t param, zb_uint8_t attr, void *value,
                    zb_ushort_t value_size, zb_callback_t cb)
{
  zb_mlme_set_request_t *req;

  TRACE_MSG(TRACE_NWK3, "pib set, value size %d", (FMT__D, value_size));
  req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + value_size);
  req->pib_attr = attr;
  req->pib_index = 0;
  ZB_ASSERT(value_size <= ZB_UINT8_MAX);
  req->pib_length = (zb_uint8_t)value_size;
  ZB_MEMCPY((req+1), value, value_size);
  req->confirm_cb_u.cb = cb;
  ZB_SCHEDULE_CALLBACK(zb_mlme_set_request, param);
}


void zb_nwk_pib_get(zb_uint8_t param, zb_uint8_t attr,
                    zb_callback_t cb)
{
  zb_mlme_get_request_t *req;

  TRACE_MSG(TRACE_MAC3, "pib get  " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(ZB_PIBCACHE_EXTENDED_ADDRESS())));
  req = zb_buf_initial_alloc(param, sizeof(zb_mlme_get_request_t));
  req->pib_attr = attr;
  req->pib_index = 0;
  req->confirm_cb_u.cb = cb;
  ZB_SCHEDULE_CALLBACK(zb_mlme_get_request, param);
}


static void zb_nwk_set_or_get_pib_attr(zb_uint8_t param, zb_bool_t do_set, zb_uint8_t attr,
                                       void *value,
                                       zb_ushort_t value_size)
{
  if (do_set)
  {
    /* We will be called next as a set confirm. Jump to this state over get confirm */
    ZG->nwk.handle.state++;
    zb_nwk_pib_set(param, attr, value, value_size, zb_nwk_load_pib_stm);
  }
  else
  {
    zb_nwk_pib_get(param, attr, zb_nwk_load_pib_stm);
  }
}


void  zb_nwk_load_pib_stm(zb_uint8_t param)
{
  /* Not always used, but can assign always */
  zb_mlme_get_confirm_t *gconf = (zb_mlme_get_confirm_t *)zb_buf_begin(param);

  TRACE_MSG(TRACE_MAC3, "load pib stm %hd", (FMT__H, ZG->nwk.handle.state));
  switch (ZG->nwk.handle.state++)
  {
    case ZB_NLME_STATE_PIB_LOAD1:
      /* if assigned, set it in PIB, else load from PIB */
      zb_nwk_set_or_get_pib_attr(param,
                                 (zb_bool_t)(ZB_PIBCACHE_PAN_ID() != ZB_BROADCAST_PAN_ID),
                                 ZB_PIB_ATTRIBUTE_PANID, &ZB_PIBCACHE_PAN_ID(), 2);
      break;

    case ZB_NLME_STATE_PIB_LOAD2:
    case ZB_NLME_STATE_PIB_LOAD3:
      /* MISRA prohibits falling through in switch clause (Rule 16.3).
       * Use this strange comparison to workaround it. */
      if ((ZG->nwk.handle.state - 1U) == ZB_NLME_STATE_PIB_LOAD2)
      {
        ZB_MEMCPY(&ZB_PIBCACHE_PAN_ID(), (gconf+1), 2);
        ZG->nwk.handle.state++;
      }

      zb_nwk_set_or_get_pib_attr(param,
                                 (zb_bool_t)(ZB_PIBCACHE_NETWORK_ADDRESS() != 0xFFFFU),
                                 ZB_PIB_ATTRIBUTE_SHORT_ADDRESS, &ZB_PIBCACHE_NETWORK_ADDRESS(), 2);
      break;

    case ZB_NLME_STATE_PIB_LOAD4:
    case ZB_NLME_STATE_PIB_LOAD5:
      /* MISRA prohibits falling through in switch clause (Rule 16.3).
       * Use this strange comparison to workaround it. */
      if ((ZG->nwk.handle.state - 1U) == ZB_NLME_STATE_PIB_LOAD4)
      {
        ZB_MEMCPY(&ZB_PIBCACHE_NETWORK_ADDRESS(), (gconf+1), 2);
        ZG->nwk.handle.state++;
      }

/*Dont need perform a MLME-SET request if we get IEEE address from a radio chip*/
#ifdef USE_HW_LONG_ADDR
      zb_nwk_set_or_get_pib_attr(param,
                                 ZB_FALSE,
                                 ZB_PIB_ATTRIBUTE_EXTEND_ADDRESS, &ZB_PIBCACHE_EXTENDED_ADDRESS(),
                                 sizeof(zb_ieee_addr_t));
#else
      TRACE_MSG(TRACE_MAC3, "exta  " TRACE_FORMAT_64,
                (FMT__A, TRACE_ARG_64(ZB_PIBCACHE_EXTENDED_ADDRESS())));
      zb_nwk_set_or_get_pib_attr(param,
                                 (zb_bool_t)(!ZB_IEEE_ADDR_IS_ZERO(ZB_PIBCACHE_EXTENDED_ADDRESS())),
                                 ZB_PIB_ATTRIBUTE_EXTEND_ADDRESS, &ZB_PIBCACHE_EXTENDED_ADDRESS(),
                                 sizeof(zb_ieee_addr_t));
#endif /* USE_HW_LONG_ADDR */
      break;

    case ZB_NLME_STATE_PIB_LOAD6:
    case ZB_NLME_STATE_PIB_LOAD7:
    {
      /* MISRA prohibits falling through in switch clause (Rule 16.3).
       * Use this strange comparison to workaround it. */
      if ((ZG->nwk.handle.state - 1U) == ZB_NLME_STATE_PIB_LOAD6)
      {
        TRACE_MSG(TRACE_MAC3, "load extaddr   " TRACE_FORMAT_64, (FMT__A,
                                                                TRACE_ARG_64(ZB_PIBCACHE_EXTENDED_ADDRESS())));
        ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), (gconf+1));
        TRACE_MSG(TRACE_MAC3, "after load   " TRACE_FORMAT_64, (FMT__A,
                                                              TRACE_ARG_64(ZB_PIBCACHE_EXTENDED_ADDRESS())));
        ZG->nwk.handle.state++;
      }

      zb_nwk_set_or_get_pib_attr(param,
                                 (zb_bool_t)(ZB_PIBCACHE_ASSOCIATION_PERMIT() != 0xFFU),
                                 ZB_PIB_ATTRIBUTE_ASSOCIATION_PERMIT, &ZB_PIBCACHE_ASSOCIATION_PERMIT(), 1);
      break;
    }

    case ZB_NLME_STATE_PIB_LOAD8:
    case ZB_NLME_STATE_PIB_LOAD9:
    {
      zb_bool_t rx_on;

      /* MISRA prohibits falling through in switch clause (Rule 16.3).
       * Use this strange comparison to workaround it. */
      if ((ZG->nwk.handle.state - 1U) == ZB_NLME_STATE_PIB_LOAD8)
      {
        ZB_PIBCACHE_ASSOCIATION_PERMIT() = *((zb_uint8_t*)(gconf+1));
      }
#ifndef ZB_ED_RX_OFF_WHEN_IDLE
      if (ZB_PIBCACHE_RX_ON_WHEN_IDLE() == 0xFFU)
      {
        /* Application doesn't specify value, use RxOn by default */
        ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_TRUE_U;
      }
#endif

      /* Set RxOnWhenIdle here to be able to start joining with the value specified by the
       * application */

      rx_on = ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE());
      zb_nwk_set_or_get_pib_attr(param, ZB_TRUE, ZB_PIB_ATTRIBUTE_RX_ON_WHEN_IDLE, &rx_on, 1);

      break;
    }

    case ZB_NLME_STATE_PIB_LOAD10:
      /* don't need to get RxOnWhenIdle from MAC, it explicitly set from NWK */
      /* FALLTHROUGH */

#if defined ZB_MAC_PENDING_BIT_SOURCE_MATCHING && defined ZB_ROUTER_ROLE
    case ZB_NLME_STATE_PIB_LOAD_SRC_MATCH_TBL:
      /*
        Note: this state is the last one.  After we initiated
        zb_nwk_src_match_restore it will do things itself. When it
        complete, it call zb_nwk_load_pib_stm once again and fall to
        default.
       */
      zb_nwk_src_match_restore(param);
      ZG->nwk.handle.state++;
      break;
#endif
      /* Make gcc happy */
      /* FALLTHROUGH */

    default:
      /* Done loading/modifying PIB */
      ZG->nwk.handle.state = ZB_NLME_STATE_IDLE;
      ZB_SCHEDULE_CALLBACK(zb_nwk_load_pib_confirm, param);
      break;
  }
}

#if defined ZB_ENABLE_ZLL && defined ZB_ROUTER_ROLE

void zb_nlme_direct_join_request(zb_uint8_t param)
{

  TRACE_MSG(TRACE_NWK1, "> zb_nlme_direct_join_request param %hd", (FMT__H, param));

#if defined ZB_ENABLE_ZLL
  if (    ZLL_TRAN_CTX().transaction_id
      &&  ZLL_TRAN_CTX().transaction_task == ZB_ZLL_TRANSACTION_NWK_START_TASK_TGT)
  {
    TRACE_MSG(TRACE_ERROR,
              "ERROR Actual NLME-DIRECT-JOIN.request handling not implemented.",
              (FMT__0));
    zb_buf_set_status(param, ZB_ZLL_GENERAL_STATUS_FAILURE);
    ZB_SCHEDULE_CALLBACK(zb_nlme_direct_join_confirm, param);
  }
  else
#endif /* defined ZB_ENABLE_ZLL */
  {
    zb_buf_free(param);
    TRACE_MSG(TRACE_ERROR, "ERROR not implemented!", (FMT__0));
  }

  TRACE_MSG(TRACE_NWK1, "< zb_nlme_direct_join_request", (FMT__0));
}/* void zb_nlme_direct_join_request(zb_uint8_t param) */

void zb_nlme_direct_join_confirm(zb_uint8_t param)
{

  TRACE_MSG(TRACE_NWK1, "> zb_nlme_direct_join_confirm param %hd", (FMT__H, param));

#if defined ZB_ZLL_ENABLE_COMMISSIONING_SERVER
    ZB_SCHEDULE_CALLBACK(zll_direct_join_confirm, param);
#else
    zb_buf_free(param);
#endif /* defined ZB_ZLL_ENABLE_COMMISSIONING_SERVER */

  TRACE_MSG(TRACE_NWK1, "< zb_nlme_direct_join_confirm", (FMT__0));
}/* void zb_nlme_direct_join_confirm(zb_uint8_t param) */

#endif /* defined ZB_ENABLE_ZLL && defined ZB_ROUTER_ROLE */

zb_nwk_device_type_t zb_get_device_type(void)
{
  return ZB_NIB().device_type;
}

zb_bool_t zb_is_device_zc(void)
{
  return (zb_bool_t)(ZB_NIB().device_type == ZB_NWK_DEVICE_TYPE_COORDINATOR);
}

zb_bool_t zb_is_device_zr(void)
{
  return (zb_bool_t)(ZB_NIB().device_type == ZB_NWK_DEVICE_TYPE_ROUTER);
}

zb_bool_t zb_is_device_zed(void)
{
  return (zb_bool_t)(ZB_NIB().device_type == ZB_NWK_DEVICE_TYPE_ED);
}

zb_bool_t zb_is_device_zc_or_zr(void)
{
  return (zb_bool_t)(ZB_NIB().device_type == ZB_NWK_DEVICE_TYPE_COORDINATOR
    || ZB_NIB().device_type == ZB_NWK_DEVICE_TYPE_ROUTER);
}

#ifdef ZB_NLME_STATUS_AS_STRING
const char* zb_get_nlme_status_as_string(zb_uint8_t status)
{
  switch (status)
  {
  case ZB_NWK_COMMAND_STATUS_NO_ROUTE_AVAILABLE: return "No route available";
  case ZB_NWK_COMMAND_STATUS_TREE_LINK_FAILURE: return "Tree link failure";
  case ZB_NWK_COMMAND_STATUS_NONE_TREE_LINK_FAILURE: return "None-tree link failure";
  case ZB_NWK_COMMAND_STATUS_LOW_BATTERY_LEVEL: return "Low battery level";
  case ZB_NWK_COMMAND_STATUS_NO_ROUTING_CAPACITY: return "No routing capacity";
  case ZB_NWK_COMMAND_STATUS_NO_INDIRECT_CAPACITY: return "No indirect capacity";
  case ZB_NWK_COMMAND_STATUS_INDIRECT_TRANSACTION_EXPIRY: return "Indirect transaction expiry";
  case ZB_NWK_COMMAND_STATUS_TARGET_DEVICE_UNAVAILABLE: return "Target device unavailable";
  case ZB_NWK_COMMAND_STATUS_TARGET_ADDRESS_UNALLOCATED: return "Target address unallocated";
  case ZB_NWK_COMMAND_STATUS_PARENT_LINK_FAILURE: return "Parent link failure";
  case ZB_NWK_COMMAND_STATUS_VALIDATE_ROUTE: return "Validate route";
  case ZB_NWK_COMMAND_STATUS_SOURCE_ROUTE_FAILURE: return "Source route failure";
  case ZB_NWK_COMMAND_STATUS_MANY_TO_ONE_ROUTE_FAILURE: return "Many-to-one route failure";
  case ZB_NWK_COMMAND_STATUS_ADDRESS_CONFLICT: return "Address conflict";
  case ZB_NWK_COMMAND_STATUS_VERIFY_ADDRESS: return "Verify address";
  case ZB_NWK_COMMAND_STATUS_PAN_IDENTIFIER_UPDATE: return "Pan ID update";
  case ZB_NWK_COMMAND_STATUS_NETWORK_ADDRESS_UPDATE: return "Network address update";
  case ZB_NWK_COMMAND_STATUS_BAD_FRAME_COUNTER: return "Bad frame counter";
  case ZB_NWK_COMMAND_STATUS_BAD_KEY_SEQUENCE_NUMBER: return "Bad key sequence number";
  case ZB_NWK_COMMAND_STATUS_UNKNOWN_COMMAND: return "Command received is not known";
  default:
    break;
  }
  return "unhandled";
}
#endif  /* ZB_NLME_STATUS_AS_STRING */

/*! @} */
