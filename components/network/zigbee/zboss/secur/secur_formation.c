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
/* PURPOSE: Security functionality of device capable to do formation:
either TC (ZC) or ZR in Distributed security mode.
*/


#define ZB_TRACE_FILE_ID 91
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_zdo_globals.h"
#include "zb_bdb_internal.h"

/*! \addtogroup ZB_SECUR */
/*! @{ */

#ifdef ZB_FORMATION

#ifdef ZB_COORDINATOR_ROLE
static void zb_apsme_send_via_tunnel(zb_uint8_t param2, zb_uint16_t param);
#endif


/**
   TC initialization
 */
void secur_tc_init(void)
{
  TRACE_MSG(TRACE_SECUR1, "TC init", (FMT__0));
  if (!IS_DISTRIBUTED_SECURITY())
  {
#ifndef ZB_COORDINATOR_ONLY
    ZB_IEEE_ADDR_COPY(ZB_AIB().trust_center_address, ZB_PIBCACHE_EXTENDED_ADDRESS());
#endif /* ZB_COORDINATOR_ONLY */
  }
#ifdef ZB_DISTRIBUTED_SECURITY_ON
  else
  {
    /* If distributed, we are not TC but just a ZR */
    /* set TC addr to -1 */
    ZB_IEEE_ADDR_COPY(ZB_AIB().trust_center_address, g_unknown_ieee_addr);
  }
#endif
  if (secur_nwk_key_is_empty(ZB_NIB().secur_material_set[0].key))
  {
    TRACE_MSG(TRACE_SECUR1, "Genetating NWK keys", (FMT__0));
    secur_nwk_generate_keys();
  }
  /* TC is always authenticated */
  ZG->aps.authenticated = ZB_TRUE;
}


void secur_nwk_generate_key(zb_uint8_t i, zb_uint_t key_seq)
{
  secur_generate_key(ZB_NIB().secur_material_set[i].key);
  ZB_NIB().secur_material_set[i].key_seq_number = key_seq;
}


void secur_nwk_generate_keys()
{
  zb_ushort_t i;

  for (i = 0 ; i < ZB_SECUR_N_SECUR_MATERIAL ; ++i)
  {
    /* active_key_seq_number, active_secur_material_i set to 0 by global init -
       not need to init it here */
    secur_nwk_generate_key(i, i);
  }
}


/**
   Authenticate child - see 4.6.2.2

   Called from nlme-join.indication at ZC or ZR
 */
void secur_authenticate_child_directly(zb_uint8_t param)
{
  zb_nlme_join_indication_t ind = *ZB_BUF_GET_PARAM(param, zb_nlme_join_indication_t);
  zb_apsme_update_device_ind_t uind;

  /* Create Update device indication to reuse zb_authenticate_dev() */
  uind.status = zb_secur_gen_upd_dev_status(ind.rejoin_network, ind.secure_rejoin);
  /* That us not actually Update Device, so set ZC as a src */
  ZB_IEEE_ADDR_COPY(uind.src_address, ZB_PIBCACHE_EXTENDED_ADDRESS());
  ZB_IEEE_ADDR_COPY(uind.device_address, ind.extended_address);
  uind.device_short_address = ind.network_address;

  TRACE_MSG(TRACE_SECUR3, "authenticate child 0x%x, upd_dev_status %hd devt %hd is_tc %hd",
            (FMT__D_H_H_H, ind.network_address, uind.status, zb_get_device_type(), ZB_IS_TC()));

  if (zb_authenticate_dev(param, &uind) == RET_UNAUTHORIZED)
  {
    zb_nlme_leave_request_t *lr = ZB_BUF_GET_PARAM(param, zb_nlme_leave_request_t);

    /*Here we shall remove device from the network*/
    TRACE_MSG(TRACE_NWK1, "force child leave", (FMT__0));
    ZB_IEEE_ADDR_COPY(lr->device_address, ind.extended_address);
    lr->remove_children = ZB_TRUE;
    lr->rejoin = ZB_FALSE;
    ZB_SCHEDULE_CALLBACK(zb_nlme_leave_request, param);
  }
}


/**
   Send nwk key either directly or using tunnel.

   Upper level is already filled zb_apsme_transport_key_req_t in param.
 */
void zdo_commissioning_send_nwk_key_to_joined_dev(zb_uint8_t param, zb_uint16_t second_buf)
{
  zb_ret_t ret;

  if (ZB_SE_MODE()
      && second_buf == 0)
  {
    zb_buf_get_out_delayed_ext(zdo_commissioning_send_nwk_key_to_joined_dev, param, 0);
    return;
  }
  /* TRICKY: zb_buf_get_out_delayed_ext() passes its arg param as the SECOND parameter of
     zdo_commissioning_send_nwk_key_to_joined_dev(), i.e. param now is second_buf and we need
     to swap them because without swapping if param was locked at NWK level nobody will unlock it.
  */
  if (second_buf != 0)
  {
    zb_bufid_t tmp = param;
    param = second_buf;
    second_buf = tmp;
  }
  if (!ZB_NWK_KEY_DISABLE())
  {
    /* Use bdb_link_key_transport_with_alarm even for
     * SE, but do not set ZB_TCPOL().update_trust_center_link_keys_required because
     * SE use CBKE.
     * Still need to inform SE about NWK key send, so SE can use its own alarm for CBKE.
     */
    if (ZB_SE_MODE())
    {
      TRACE_MSG(TRACE_SECUR3, "SE - authenticating device", (FMT__0));
      zb_buf_copy(second_buf, param);
      /* If this is NCP SoC, remote signal to SE commissioning layer on Host */
      zdo_commissioning_authenticate_remote(second_buf);
    }
    /*fix OOM issue, 20220712*/
    ret = zb_buf_get_out_delayed_ext(bdb_link_key_transport_with_alarm, param, 0);
    if(ret == RET_OK)
    {
      param = 0;
    }
  }
  else
  {
    /* REGRESSION: need for RTP_BDB_11 test */
    ZB_NWK_KEY_DISABLE_RESET();
  }
  if (param)
  {
    zb_buf_free(param);
  }
}


static zb_uint8_t zb_aps_secur_get_transport_key_hdr_size(zb_secur_key_type_t key_type)
{
  zb_uint8_t size = sizeof(zb_uint8_t)*(1+ZB_CCM_KEY_SIZE); /* sizeof(key_type) + sizeof(key) */
  switch(key_type)
  {
  case ZB_STANDARD_NETWORK_KEY:
    size += sizeof(zb_transport_key_nwk_pkt);
    break;
#ifndef ZB_LITE_NO_APS_DATA_ENCRYPTION
  case ZB_APP_LINK_KEY:
    size += sizeof(zb_transport_key_app_pkt);
    break;
#endif
  case ZB_TC_LINK_KEY:
    size += sizeof(zb_transport_key_tc_pkt);
    break;
  default:
      break;
  }
  return size;
}


#ifdef ZB_COORDINATOR_ROLE
static void zb_apsme_send_via_tunnel(zb_uint8_t param2, zb_uint16_t param)
{
  zb_ret_t ret;
  zb_ieee_addr_t *ieee_addr;
  zb_aps_command_header_t *hdr;
  zb_apsme_transport_key_req_t *req = ZB_BUF_GET_PARAM(param, zb_apsme_transport_key_req_t);
  /* key-transport key is used to protect transported network keys */
  /* The key-load key is used to protect transported application link key or trust center link key */
  zb_secur_key_id_t secure_aps = (zb_secur_key_id_t)(req->key_type == ZB_STANDARD_NETWORK_KEY ? ZB_SECUR_KEY_TRANSPORT_KEY : ZB_SECUR_KEY_LOAD_KEY);
  zb_uint16_t dest_short = zb_address_short_by_ieee(req->key.nwk.parent_address);

  TRACE_MSG(TRACE_SECUR3, ">> zb_apsme_send_via_tunnel param2 %hd param %d", (FMT__H_D, param2, param));
  /*
    4.4.9.8 Tunnel Commands
    The APS command frame used by a device for sending a command to a device
    that lacks the current network key is specified in this clause.

    4.6.3.7 Command Tunnelling
    Devices shall follow the procedures described in this sub-clause to allow secure
    communication between the Trust Center and a remote device that does not have
    the current network key.

    Tunnel command can be used to send network key only.
    Means, use transport key.
  */
  /* Fill header according to the Tunnel destination */
  zb_aps_command_add_secur(param, APS_CMD_TRANSPORT_KEY, secure_aps);
  hdr = (zb_aps_command_header_t *)zb_buf_begin(param);
  ZB_APS_FC_SET_COMMAND(hdr->fc, 0); /* AD: no ack for T-key */
  hdr->aps_counter = ZB_AIB_APS_COUNTER();
  ZB_AIB_APS_COUNTER_INC();

  /* try to secure the frame */
  ret = zb_aps_secure_frame(param, 0, param2, ZB_TRUE);

  if (ret == RET_OK)
  {
    ieee_addr = zb_buf_alloc_left(param2, sizeof(zb_ieee_addr_t));
    ZB_IEEE_ADDR_COPY(ieee_addr, req->dest_address.addr_long);

    /*
      Tunnel comman itself secured at NWK layer and not secured at APS layer
      (while payload is secured at APS).
    */
    zb_aps_send_command(param2,
                        zb_address_short_by_ieee(
                        req->key_type == ZB_STANDARD_NETWORK_KEY ?
                        req->key.nwk.parent_address : req->key.app.partner_address),
                        APS_CMD_TUNNEL,
                        ZB_TRUE,
                        zb_secur_aps_send_policy(APS_CMD_TUNNEL, dest_short, req->key_type));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Can't send APS command via tunnel due to frame securing problem", (FMT__0));
    zb_buf_free(param2);
  }

  zb_buf_free(param);

  TRACE_MSG(TRACE_SECUR3, "<< zb_apsme_send_via_tunnel", (FMT__0));
}

/* 8/14/2019 AN: I suppose to add a new function for use of APS tunnel
 * for transmitting APS-unencrypted transport key instead of adding
 * a plenty of #ifdef lines inside of zb_aps_secure_frame etc.
 */
void zb_apsme_send_via_tunnel_with_unencrypted_payload(zb_uint8_t param)
{
  zb_ieee_addr_t *ieee_addr;
  zb_aps_command_header_t *hdr;
  zb_bufid_t buf = param;
  zb_apsme_transport_key_req_t *req = ZB_BUF_GET_PARAM(buf, zb_apsme_transport_key_req_t);

  zb_uint16_t dest_short = zb_address_short_by_ieee(req->key.nwk.parent_address);

  TRACE_MSG(TRACE_SECUR3, ">> zb_apsme_send_via_tunnel_with_unencrypted_payload param %d", (FMT__D, param));

  /*
    Tunnel command can be used to send network key only.
    Means, use transport key.
  */
  hdr = zb_buf_alloc_left(buf, sizeof (*hdr));
  TRACE_MSG(TRACE_SECUR3, "no aps security", (FMT__0));
  hdr->fc = 0;
  hdr->aps_command_id = APS_CMD_TRANSPORT_KEY;
  ZB_APS_FC_SET_FRAME_TYPE(hdr->fc, ZB_APS_FRAME_COMMAND);
  hdr->aps_counter = ZB_AIB_APS_COUNTER();
  ZB_AIB_APS_COUNTER_INC();
  ieee_addr = zb_buf_alloc_left(buf, sizeof(zb_ieee_addr_t));
  ZB_IEEE_ADDR_COPY(ieee_addr, req->dest_address.addr_long);

  zb_aps_send_command(param, zb_address_short_by_ieee(
                      req->key_type == ZB_STANDARD_NETWORK_KEY ?
                      req->key.nwk.parent_address : req->key.app.partner_address),
                      APS_CMD_TUNNEL,
                      ZB_TRUE,
                      zb_secur_aps_send_policy(APS_CMD_TUNNEL, dest_short, req->key_type));

  TRACE_MSG(TRACE_SECUR3, "<< zb_apsme_send_via_tunnel_with_unencrypted_payload", (FMT__0));
}
#endif  /* ZB_COORDINATOR_ROLE */


void zb_apsme_transport_key_request(zb_uint8_t param)
{
  zb_bool_t secur_transfer = ZB_FALSE;
#ifdef ZB_COORDINATOR_ROLE
  zb_bool_t use_tunnel = ZB_FALSE;
#endif
  zb_uint16_t dest_short = 0;
  zb_apsme_transport_key_req_t *req = ZB_BUF_GET_PARAM(param, zb_apsme_transport_key_req_t);
  zb_transport_key_dsc_pkt_t *dsc;

  zb_uint8_t size = zb_aps_secur_get_transport_key_hdr_size(req->key_type);

  TRACE_MSG(TRACE_SECUR1, ">>zb_apsme_transport_key_request %hd", (FMT__H, param));

  dsc = zb_buf_initial_alloc(param, size);
  dsc->key_type = req->key_type;

  switch (req->key_type)
  {
  case ZB_STANDARD_NETWORK_KEY:
    ZB_MEMCPY(dsc->key, req->key.nwk.key, ZB_CCM_KEY_SIZE);
    dsc->key_data.nwk.seq_number = req->key.nwk.key_seq_number;
    if (req->addr_mode == ZB_ADDR_16BIT_DEV_OR_BROADCAST)
    {
      ZB_IEEE_ADDR_ZERO(dsc->key_data.nwk.dest_address);
    }
    else
    {
      ZB_IEEE_ADDR_COPY(dsc->key_data.nwk.dest_address, req->dest_address.addr_long);
    }
    if (!IS_DISTRIBUTED_SECURITY())
    {
      if (!IS_CONTROL4_NETWORK_EMULATOR())
      {
        ZB_IEEE_ADDR_COPY(dsc->key_data.nwk.source_address, ZB_PIBCACHE_EXTENDED_ADDRESS());
      }
      else
      {
        ZB_IEEE_ADDR_COPY(dsc->key_data.nwk.source_address, g_unknown_ieee_addr);
      }
    }
#ifdef ZB_DISTRIBUTED_SECURITY_ON
    else
    {
      ZB_IEEE_ADDR_COPY(dsc->key_data.nwk.source_address, g_unknown_ieee_addr);
    }
#endif
#ifdef ZB_COORDINATOR_ROLE
    if (req->key.nwk.use_parent)
    {
      dest_short = zb_address_short_by_ieee(req->key.nwk.parent_address);
      secur_transfer = ZB_TRUE;
#ifdef ZB_CERTIFICATION_HACKS
      if (!ZB_CERT_HACKS().aps_security_off)
#endif
      {
        use_tunnel = ZB_TRUE;
        TRACE_MSG(TRACE_SECUR3, "tunnelling transport key command to " TRACE_FORMAT_64 " via parent %d", (FMT__A_D, TRACE_ARG_64(dsc->key_data.nwk.dest_address), dest_short));
      }
    }
    else
#endif  /* #ifdef ZB_COORDINATOR_ROLE */
    {
      /* go directly */
      zb_address_ieee_ref_t addr_ref;
      zb_neighbor_tbl_ent_t *nbe;

      if (req->addr_mode == ZB_ADDR_16BIT_DEV_OR_BROADCAST)
      {
        dest_short = req->dest_address.addr_short;
        /* Broadcast transfer used for key update in the entire network. It
         * is secured. */
        secur_transfer = (zb_bool_t)ZB_NWK_IS_ADDRESS_BROADCAST(dest_short);
      }
      else
      {
        /* transfer to the child is not secured */
        dest_short = zb_address_short_by_ieee(req->dest_address.addr_long);
      }
      if ( !ZB_NWK_IS_ADDRESS_BROADCAST(dest_short)
           && ((zb_address_by_short(dest_short, ZB_FALSE, ZB_FALSE, &addr_ref) == RET_OK
               && zb_nwk_neighbor_get(addr_ref, ZB_FALSE, &nbe) == RET_OK
                && (nbe->relationship != ZB_NWK_RELATIONSHIP_UNAUTHENTICATED_CHILD)))
         )
      {
        secur_transfer = ZB_TRUE;
      }
    }
    TRACE_MSG(TRACE_SECUR3, "send std nwk key #%hd from " TRACE_FORMAT_64,
              (FMT__H_A, dsc->key_data.nwk.seq_number, TRACE_ARG_64(dsc->key_data.nwk.source_address)));
    break;

#ifdef ZB_COORDINATOR_ROLE
#ifndef ZB_LITE_NO_APS_DATA_ENCRYPTION
  case ZB_APP_LINK_KEY:
    ZB_MEMCPY(dsc->key, req->key.app.key, ZB_CCM_KEY_SIZE);
    ZB_IEEE_ADDR_COPY(dsc->key_data.app.partner_address, req->key.app.partner_address);
    dsc->key_data.app.initiator = req->key.app.initiator;
    dest_short = zb_address_short_by_ieee(req->dest_address.addr_long);
    secur_transfer = ZB_TRUE;

    TRACE_MSG(TRACE_SECUR3, "send App key initiator #%hd from " TRACE_FORMAT_64,
              (FMT__H_A, dsc->key_data.app.initiator, TRACE_ARG_64(dsc->key_data.app.partner_address)));
    break;
#endif
  case ZB_TC_LINK_KEY:
    ZB_MEMCPY(dsc->key, req->key.app.key, ZB_CCM_KEY_SIZE);
    ZB_IEEE_ADDR_COPY(dsc->key_data.tc.dest_address, req->dest_address.addr_long);
    ZB_IEEE_ADDR_COPY(dsc->key_data.tc.source_address, ZB_PIBCACHE_EXTENDED_ADDRESS());
    dest_short = zb_address_short_by_ieee(req->dest_address.addr_long);
    secur_transfer = ZB_TRUE;

    TRACE_MSG(TRACE_SECUR3, "send TC key to " TRACE_FORMAT_64,
              (FMT__A, TRACE_ARG_64(dsc->key_data.tc.dest_address)));
    break;
#endif
  default:
    TRACE_MSG( TRACE_ERROR, "Oops: unknown key type!", (FMT__0));
  }

#ifdef ZB_COORDINATOR_ROLE
  if (use_tunnel)
  {
    if (ZB_TCPOL().aps_unencrypted_transport_key_join)
    {
      ZB_SCHEDULE_CALLBACK(zb_apsme_send_via_tunnel_with_unencrypted_payload, param);
    }
    else
    {
      zb_buf_get_out_delayed_ext(zb_apsme_send_via_tunnel, param, 0);
    }
  } /* tunnel */
  else
#endif
  {
    /* AD: check for broadcast added for test profile function of sending unencrypted on APS layer
       TK */
    zb_aps_send_command(param, dest_short, APS_CMD_TRANSPORT_KEY,
                        secur_transfer,
                        (!ZB_NWK_IS_ADDRESS_BROADCAST(dest_short))?
                        zb_secur_aps_send_policy(APS_CMD_TRANSPORT_KEY, dest_short, req->key_type) : ZB_NOT_SECUR);
  }

  TRACE_MSG(TRACE_SECUR1, "<<zb_apsme_transport_key_request", (FMT__0));
}


void secur_generate_key(zb_uint8_t *key)
{
  zb_ushort_t j;
#ifdef ZB_CERTIFICATION_HACKS
  if (ZB_CERT_HACKS().enable_alldoors_key)
  {
    /*
     * Single key for everything and everybody.
     * Nothing similar in the spec, but we already have it and it might be
     * useful for debug purposes. */
    ZB_MEMCPY(key, ZB_AIB().tc_standard_key, ZB_CCM_KEY_SIZE);
  }
  else
#endif
  {
    for (j = 0 ; j < ZB_CCM_KEY_SIZE ; ++j)
    {
      key[j] = (ZB_RANDOM_U16() >> 4) & 0xff;
    }
  }
}


#ifdef ZB_DISTRIBUTED_SECURITY_ON

void zb_zdo_set_tc_standard_distributed_key(zb_uint8_t *key_ptr)
{
  ZB_MEMCPY(&ZB_AIB().tc_standard_distributed_key, key_ptr, ZB_CCM_KEY_SIZE);
}

void zb_zdo_setup_network_as_distributed(void)
{
  ZB_AIB().aps_insecure_join = ZB_TRUE;
  ZB_IEEE_ADDR_COPY(ZB_AIB().trust_center_address, g_unknown_ieee_addr);
}

#endif /* ZB_DISTRIBUTED_SECURITY_ON */

#endif  /* ZB_FORMATION */

/*! @} */
