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
/* PURPOSE: APSME security routines
*/


/*! \addtogroup ZB_SECUR */
/*! @{ */

#define ZB_TRACE_FILE_ID 2462
#include "zb_common.h"
#include "zb_aps.h"
#include "zb_secur.h"
#include "zdo_wwah_stubs.h"
#include "zb_zdo.h"

void zb_apsme_send_2_update_device(zb_uint8_t param, zb_uint16_t param2);

void zb_aps_secur_init()
{
  {
		#if 0
    zb_uint8_t standard_key[ZB_CCM_KEY_SIZE] = ZB_STANDARD_TC_KEY;
    ZB_MEMCPY(&(ZB_AIB().tc_standard_key), standard_key, ZB_CCM_KEY_SIZE);
		#else
		/* let Zigbee TC key could be changed after pack in LIB */
		zb_uint8_t standard_key[ZB_CCM_KEY_SIZE];
		ZB_MEMCPY(&(standard_key), ZB_STANDARD_TC_KEY, ZB_CCM_KEY_SIZE);	
    ZB_MEMCPY(&(ZB_AIB().tc_standard_key), standard_key, ZB_CCM_KEY_SIZE);	
		#endif
  }
#ifdef ZB_DISTRIBUTED_SECURITY_ON
  {
    zb_uint8_t standard_key[ZB_CCM_KEY_SIZE] = ZB_DISTRIBUTED_GLOBAL_KEY;
    ZB_MEMCPY(&ZB_AIB().tc_standard_distributed_key, standard_key, ZB_CCM_KEY_SIZE);
  }
#endif
  ZB_AIB().outgoing_frame_counter = 0;

#ifdef ZB_CERTIFICATION_HACKS
  ZB_CERT_HACKS().send_update_device_unencrypted = -1;
#endif
}

void zb_aps_set_preconfigure_security_key(zb_uint8_t *key)
{
  ZB_MEMCPY(&ZB_AIB().tc_standard_key, key, ZB_CCM_KEY_SIZE);
}


#ifdef ZB_COORDINATOR_ROLE

void zb_apsme_confirm_key_request(zb_uint8_t param)
{
  zb_apsme_confirm_key_req_t req = *ZB_BUF_GET_PARAM(param, zb_apsme_confirm_key_req_t);
  zb_apsme_confirm_key_pkt_t *dsc;
  zb_uint16_t dest_short = zb_address_short_by_ieee(req.dest_address);

  TRACE_MSG(TRACE_SECUR3, "zb_apsme_confirm_key_request %hd dest_short 0x%x", (FMT__H_D, param, dest_short));

  if (dest_short == (zb_uint16_t)-1
      || req.key_type != ZB_TC_LINK_KEY)
  {
    TRACE_MSG(TRACE_SECUR1, "Oops: can find a destionation / bad key type - for zb_apsme_confirm_key_request", (FMT__0));
    zb_buf_free(param);
  }
  else
  {
    zb_aps_device_key_pair_set_t *aps_key = zb_secur_get_link_key_by_address(req.dest_address, ZB_SECUR_VERIFIED_KEY);
    if (!aps_key)
    {
      req.status = ZB_APS_STATUS_SECURITY_FAIL;
    }
    else
    {
    /* The device shall set the IncomingFrameCounter of the apsDeviceKeyPairSet
     * entry to 0 */
#ifndef ZB_NO_CHECK_INCOMING_SECURE_APS_FRAME_COUNTERS
      zb_uint8_t idx = ZB_AIB().aps_device_key_pair_storage.cached_i;
      ZB_AIB().aps_device_key_pair_storage.key_pair_set[idx].incoming_frame_counter = 0;
#endif
#ifdef ZB_USE_NVRAM
      /* If we fail, trace is given and assertion is triggered */
      (void)zb_nvram_write_dataset(ZB_NVRAM_APS_SECURE_DATA);
#endif
    }
    dsc = zb_buf_initial_alloc(param, sizeof(zb_apsme_confirm_key_pkt_t));
    dsc->status = req.status;
    dsc->key_type = req.key_type;
    ZB_IEEE_ADDR_COPY(dsc->dest_address, req.dest_address);
    TRACE_MSG(TRACE_SECUR3, "Sending confirm key to 0x%x status %hd", (FMT__D_H, dest_short, dsc->status));
    zb_aps_send_command(param,
                        dest_short,
                        APS_CMD_CONFIRM_KEY,
                        ZB_TRUE,
                        /* the Status in the Command to FAILURE. The APS Command shall not be APS encrypted. */
                        (req.status == ZB_APS_STATUS_SUCCESS ?
                         zb_secur_aps_send_policy(APS_CMD_CONFIRM_KEY, dest_short, req.key_type)
                         : ZB_FALSE));
  }
}
#endif  /* #ifdef ZB_COORDINATOR_ROLE */


#ifdef ZB_ROUTER_SECURITY
/**
   Send UPDATE-DEVICE.request from ZR to TC.
  4.4.4.1.2 When Generated
  The ZDO (for example, on a router or Zigbee coordinator) shall initiate the
  APSME-UPDATE-DEVICE.request primitive when it wants to send updated
  device information to another device (for example, the Trust Center).

   TC must send nwk key or remove_device to it.
 */
void zb_apsme_update_device_request(zb_uint8_t param)
{
  zb_apsme_update_device_req_t req = *(zb_apsme_update_device_req_t *)ZB_BUF_GET_PARAM(param, zb_apsme_update_device_req_t);
  zb_apsme_update_device_pkt_t *p;
  zb_uint16_t short_dest;

  p = zb_buf_initial_alloc(param, sizeof(zb_apsme_update_device_pkt_t));

  ZB_IEEE_ADDR_COPY(p->device_address, req.device_address);
  ZB_HTOLE16((zb_uint8_t *)&p->device_short_address, (zb_uint8_t *)&req.device_short_address);
  p->status = req.status;
  TRACE_MSG(TRACE_SECUR3, "send UPDATE-DEVICE " TRACE_FORMAT_64 " 0x%hx -->",
            (FMT__A_H, TRACE_ARG_64(p->device_address), p->device_short_address));
  TRACE_MSG(TRACE_SECUR3, "--> status %hd to " TRACE_FORMAT_64,
            (FMT__H_A, p->status, TRACE_ARG_64(req.dest_address)));

/*  Check for existence of shord dest
 * address!. Maybe, under ZB_TC_AT_ZC can set 0 here. */

  short_dest = zb_address_short_by_ieee(req.dest_address);
#ifdef ZB_TC_AT_ZC
  /* If do not know short TC address, send to ZC */
  if (short_dest == (zb_uint16_t)-1)
  {
    TRACE_MSG(TRACE_SECUR3, "Suppose TC is at ZC", (FMT__0));
    short_dest = 0;
    if (!ZB_IEEE_ADDR_IS_ZERO(ZB_AIB().trust_center_address))
    {
      zb_address_ieee_ref_t addr_ref;
      TRACE_MSG(TRACE_SECUR1, "TC is " TRACE_FORMAT_64 " update addres translation",
                (FMT__A, TRACE_ARG_64(ZB_AIB().trust_center_address)));
      (void)zb_address_update(ZB_AIB().trust_center_address, 0, ZB_FALSE, &addr_ref);
    }
  }
#endif
  if (short_dest == (zb_uint16_t)-1)
  {
    TRACE_MSG(TRACE_ERROR, "Can't send UPDATE DEVICE!", (FMT__0));
    zb_buf_free(param);
  }
  else
  {
    zb_uint8_t policy = zb_secur_aps_send_policy(APS_CMD_UPDATE_DEVICE, short_dest, 0);
    if (policy == ZB_SECUR_DATA_KEY_N_UNENCR)
    {
      zb_ret_t ret;

      /* *req might be corrupted. */
      *(zb_apsme_update_device_req_t *)ZB_BUF_GET_PARAM(param, zb_apsme_update_device_req_t) = req;
      ret = zb_buf_get_out_delayed_ext(zb_apsme_send_2_update_device, param, 0);
      if (ret != RET_OK)
      {
        TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed_ext [%d]", (FMT__D, ret));
      }
    }
    else
    {
      zb_aps_send_command(param,
                          short_dest,
                          APS_CMD_UPDATE_DEVICE, ZB_TRUE, policy);
    }

    zb_prepare_and_send_device_update_signal(req.device_address, req.status);
  }
}


void zb_apsme_send_2_update_device(zb_uint8_t param2, zb_uint16_t param)
{
  zb_apsme_update_device_req_t *req = ZB_BUF_GET_PARAM((zb_uint8_t)param, zb_apsme_update_device_req_t);
  zb_uint16_t short_dest = zb_address_short_by_ieee(req->dest_address);

  TRACE_MSG(TRACE_SECUR3, "zb_apsme_send_2_update_device dest_address " TRACE_FORMAT_64 " short_dest 0x%x",
            (FMT__A_D, TRACE_ARG_64(req->dest_address), short_dest));

  zb_buf_copy(param2, (zb_uint8_t)param);

  zb_aps_send_command((zb_uint8_t)param,
                      short_dest,
                      APS_CMD_UPDATE_DEVICE, ZB_TRUE, ZB_SECUR_DATA_KEY);
  zb_aps_send_command(param2,
                      short_dest,
                      APS_CMD_UPDATE_DEVICE, ZB_TRUE, ZB_NOT_SECUR);
}
#endif  /* ZB_ROUTER_SECURITY */


#ifdef ZB_COORDINATOR_ROLE
void zb_apsme_switch_key_request(zb_uint8_t param)
{
  zb_apsme_switch_key_req_t *req = ZB_BUF_GET_PARAM(param, zb_apsme_switch_key_req_t);
  zb_apsme_switch_key_pkt_t *p;
  zb_uint16_t dest_short;
  zb_ieee_addr_t ieee_bcast;
  zb_uint8_t aps_secure = ZB_NOT_SECUR;

  TRACE_MSG(TRACE_SECUR1, ">>zb_apsme_switch_key_req param %hd", (FMT__H, param));

  ZB_MEMSET(ieee_bcast, -1, sizeof(zb_ieee_addr_t));

  p = zb_buf_initial_alloc(param, sizeof(*p));
  /*
    See 4.4.7.1.3:
    The sequence number field of this command frame shall be set to the same
    value as the KeySeqNumber parameter.

    Its strange, but..
  */
  /*ZB_AIB_APS_COUNTER() = req->key_seq_number;*/
  /* Setting aps counter causes false APS dups! */

  /* [MM]: Indeed. Ambiguity is caused by the misinterpretation of
   * the "sequence number field". See section 4.4.10.5, Switch Key
   * command frame, payload section, of the r21 (I
   * suppose, different numeration of the same thing presents in r20)
   * Sequence number field is the part if payload and provides
   * information about the network key to be made active, while the APS
   * counter is named explicitly as APS counter.
   */

  /* 4.4.6.1.   APSME-SWITCH-KEY.request
   * 4.4.6.1.3  Effect on Receipt
   *
   * If the DestAddress is not the broadcast address 0xFFFFFFFFFFFFFFFF,
   * this command frame shall be security-protected as specified in
   * section 4.4.1.1 and then, if security processing succeeds, sent to
   * the device specified by the DestAddress parameter by issuing a
   * NLDE-DATA.request primitive.
   *
   * If the DestAddress is the broadcast address 0xFFFFFFFFFFFFFFFF then
   * the command shall not be security protected at the APS layer. It
   * shall be sent to the NWK broadcast address 0xFFFD by issuing a
   * NLDE-DATA.request primitive.
   */
#ifndef ZB_LITE_NO_UNICAST_SWITCH_KEY
  if (ZB_IEEE_ADDR_CMP(req->dest_address, ieee_bcast))
#endif
  {
    dest_short = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
  }
#ifndef ZB_LITE_NO_UNICAST_SWITCH_KEY
  else
  {
    /* Use unicast - obtain short address and corresponding security */
    dest_short = zb_address_short_by_ieee(req->dest_address);
    aps_secure = zb_secur_aps_send_policy(APS_CMD_SWITCH_KEY, dest_short, 0);
  }
#endif

  TRACE_MSG(TRACE_SECUR1, "switch_key dest " TRACE_FORMAT_64 " short %d key_seq_num %d",
            (FMT__A_D_D, TRACE_ARG_64(req->dest_address), dest_short, req->key_seq_number));

  p->key_seq_number = req->key_seq_number;
  zb_aps_send_command(param,
                      dest_short,
                      APS_CMD_SWITCH_KEY, ZB_TRUE
                      , aps_secure
  );

  TRACE_MSG(TRACE_SECUR1, "<<zb_apsme_switch_key_req", (FMT__0));
}
#endif  /* ZB_COORDINATOR_ROLE */


static void zb_aps_get_dest_address_transport_key(zb_ieee_addr_t dest_addr, zb_transport_key_dsc_pkt_t *dsc)
{
  switch(dsc->key_type)
  {
  case ZB_STANDARD_NETWORK_KEY:
    ZB_IEEE_ADDR_COPY(dest_addr, dsc->key_data.nwk.dest_address);
    break;

  case ZB_APP_LINK_KEY:
    ZB_IEEE_ADDR_COPY(dest_addr, ZB_PIBCACHE_EXTENDED_ADDRESS());
    break;

  case ZB_TC_LINK_KEY:
  default:
    ZB_IEEE_ADDR_COPY(dest_addr, dsc->key_data.tc.dest_address);
    break;
  }
}

static zb_ret_t zb_aps_get_src_address_transport_key(zb_ieee_addr_t src_addr, zb_transport_key_dsc_pkt_t *dsc)
{
  zb_ret_t retcode = RET_OK;
  switch(dsc->key_type)
  {
  case ZB_STANDARD_NETWORK_KEY:
    ZB_IEEE_ADDR_COPY(src_addr, dsc->key_data.nwk.source_address);
    break;

  case ZB_TC_LINK_KEY:
    ZB_IEEE_ADDR_COPY(src_addr, dsc->key_data.tc.source_address);
    break;

  case ZB_APP_LINK_KEY:
  default:
    retcode = RET_NOT_FOUND;
    break;
  }
  return retcode;
}

zb_ret_t zb_aps_get_ieee_source_from_cmd_frame(zb_uint8_t cmd_id, zb_uint8_t cmd_buf_param, zb_ieee_addr_t ieee_addr)
{
  zb_ret_t retcode;
  zb_transport_key_dsc_pkt_t *cmd_ptr = zb_buf_begin(cmd_buf_param);

  switch(cmd_id)
  {
    case APS_CMD_TRANSPORT_KEY:
      retcode = zb_aps_get_src_address_transport_key(ieee_addr, cmd_ptr);
      break;
      /* TODO: maybe need to implement other cmds. */
    default:
      retcode = RET_NOT_FOUND;
      break;
  }
  return retcode;
}

#ifndef ZB_COORDINATOR_ONLY

#ifdef ZB_ROUTER_ROLE
/**
   Reaction on TUNNEL APS command (indication)
 */
void zb_aps_in_tunnel_cmd(zb_uint8_t param)
{
  zb_ret_t ret;
  zb_ieee_addr_t *ieee_addr;
  zb_uint16_t short_addr;
  zb_address_ieee_ref_t ref;

  TRACE_MSG(TRACE_APS2, ">> zb_aps_in_tunnel_cmd(%d)", (FMT__H, param));
  TRACE_MSG(TRACE_APS2, "Cutting tunnel command hdrs", (FMT__0));
  zb_nwk_unlock_in(param);
  ieee_addr = (zb_ieee_addr_t *)zb_buf_begin(param);
  short_addr = zb_address_short_by_ieee(*ieee_addr);
  (void)zb_buf_cut_left(param, sizeof(zb_ieee_addr_t));

  fill_nldereq(param, short_addr, ZB_FALSE);
  zb_buf_flags_clr_encr(param);
  /* lock address; it will be unlocked when key is sent */
  ret = zb_address_by_short(short_addr, ZB_FALSE, ZB_TRUE, &ref);
  ZB_ASSERT(ret == RET_OK);
  /* NLDE-DATA.request to pass tunnel's payload */
  /*4.6.3.7.2   Parent Operations */
  ZB_SCHEDULE_CALLBACK(zb_nlde_data_request, param);

  TRACE_MSG(TRACE_APS2, "<< zb_aps_in_tunnel_cmd", (FMT__0));
}
#endif  /* #ifdef ZB_ROUTER_ROLE */


/**
   Reaction on TRANSPORT-KEY APS command (indication)
 */
void zb_aps_in_transport_key(zb_uint8_t param, zb_uint16_t keypair_i
                             , zb_secur_key_id_t key_id
                            )
{
  /* Note: there is no source address in app lk. Try to get it from APS &
   * NWK headers. */

#ifndef ZB_LITE_NO_APS_DATA_ENCRYPTION
  zb_uint16_t src_short = ((zb_apsde_data_indication_t *)ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t))->src_addr;
#endif
  zb_uint16_t dst_short = ((zb_apsde_data_indication_t *)ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t))->dst_addr;
  zb_apsme_transport_key_indication_t *ind = ZB_BUF_GET_PARAM(param, zb_apsme_transport_key_indication_t);
  zb_transport_key_dsc_pkt_t *dsc = (zb_transport_key_dsc_pkt_t *)zb_buf_begin(param);
  zb_ieee_addr_t dest_addr;
  zb_uint_t flags = zb_buf_flags_get(param);
  zb_bool_t aps_encr = ZB_BIT_IS_SET(flags, ZB_BUF_SECUR_APS_ENCR);
  zb_bool_t nwk_encr = ZB_BIT_IS_SET(flags, ZB_BUF_SECUR_NWK_ENCR);

  TRACE_MSG(TRACE_SECUR3, ">>zb_aps_in_transport_key %d", (FMT__H, param));

  /* See 4.4.3.3  Upon Receipt of a Transport-Key Command */

  zb_aps_get_dest_address_transport_key(dest_addr, dsc);
  ind->key_type = dsc->key_type;

  if (
    /* key is for me */
    ZB_IEEE_ADDR_CMP(dest_addr, ZB_PIBCACHE_EXTENDED_ADDRESS())
    /* key is for all */
      || ZB_IEEE_ADDR_IS_ZERO(dest_addr))
  {
    switch (dsc->key_type)
    {
      case ZB_STANDARD_NETWORK_KEY:
        /* This nwk key is for me. Issue APSME-TRANSPORT-KEY.indication. ZDO will
         * setup keys and remember TC address.
         */
        /* 4.4.2.3. If the device is operating in the joined and
         * authorised state, it _may_ accept a NWK bcast transport key
         * command with Key type field set to 0x01 where the message
         * has no APS encryption. If the key type field is set to
         * 0x01 and the command was not secured using a distributed
         * security link key, TC link key, the command shall be
         * discarded */
        TRACE_MSG(TRACE_SECUR3, "in std nwk key #%d for me", (FMT__D, dsc->key_data.nwk.seq_number));

        if (aps_encr
            /*cstat !MISRAC2012-Rule-13.5 */
            /* After some investigation, the following violation of Rule 13.5 seems to be
             * a false positive. There are no side effects to 'ZB_JOINED()'. This
             * violation seems to be caused by the fact that 'ZB_JOINED()' is an
             * external macro, which cannot be analyzed by C-STAT. */
            || (ZB_JOINED()
                && ZG->aps.authenticated
                && nwk_encr
                && ZB_NWK_IS_ADDRESS_BROADCAST(dst_short))
#ifdef ZB_CERTIFICATION_HACKS
            || ZB_CERT_HACKS().aps_security_off
#endif
        )
        {
          zb_bool_t pass_pkt_up;
          zb_bool_t  is_distributed = ZB_IEEE_ADDR_CMP(g_unknown_ieee_addr, dsc->key_data.nwk.source_address);
          zb_bool_t  is_encr_by_dtclk = ZB_FALSE;
#ifdef ZB_DISTRIBUTED_SECURITY_ON
          zb_aps_device_key_pair_set_t* key_pair = zb_aps_keypair_get_ent_by_idx(keypair_i);

          is_encr_by_dtclk = (key_pair) ?
            ((0==ZB_MEMCMP(key_pair->link_key, ZB_AIB().tc_standard_distributed_key, ZB_CCM_KEY_SIZE)) ? ZB_TRUE:ZB_FALSE ) : ZB_FALSE;

          if (is_distributed)
          {
            /* BDB, 10.4.1 Adding a new node into the network: */
            /* When a node operating on a distributed security network is accepting a new node for */
            /* joining it SHALL use the distributed security global link key (see 6.3.2) to encrypt the */
            /* network key. */

            /* Distributed security: pkt should be encrypted only by standard distributed key! */
            pass_pkt_up = (aps_encr && (keypair_i != (zb_uint16_t)-1)) &&
                          is_encr_by_dtclk;
          }
          else
#endif
          {
            /* VP: according to test CS-KTU-TC-02 joiner should not accept NWK key decrypted
             * with Distributed Global TC LK when joins to centralized network. */
            pass_pkt_up = !is_distributed && !is_encr_by_dtclk;
            TRACE_MSG(TRACE_SECUR3, "pass_pkt_up %d", (FMT__D, pass_pkt_up));
          }
#if defined ZB_ED_FUNC && defined ZB_CONTROL4_NETWORK_SUPPORT
          if (ZB_TCPOL().permit_control4_network)
          {
            if (is_distributed && !is_encr_by_dtclk)
            {
              /* Workaround for joining Control4 Network.
                 Control4 Controller sends Transport Key command encrypted by
                 standard centralized network key, but in Extended Source field,
                 it puts Unknown address that is valid only for distributed network.
              */
              pass_pkt_up = ZB_TRUE;
            }
          }
#endif /* defined ZB_ED_FUNC && defined ZB_CONTROL4_NETWORK_SUPPORT */
#ifdef ZB_CERTIFICATION_HACKS
          if (ZB_CERT_HACKS().aps_security_off)
          {
            pass_pkt_up = ZB_TRUE;
          }
#endif
          /* A device SHALL only process network key rotation commands (APS Transport Key and Network Update commands)
           * which are sent via unicast and are encrypted by the Trust Center Link Key. */

          /*cstat -MISRAC2012-Rule-2.1_b -MISRAC2012-Rule-14.3_b */
          if (!ZB_ZDO_CHECK_NWK_KEY_COMMANDS_BROADCAST_ALLOWED()
              && (ZB_NWK_IS_ADDRESS_BROADCAST(dst_short)
                  || ZB_IEEE_ADDR_IS_ZERO(dest_addr)))
          {
            /*cstat +MISRAC2012-Rule-2.1_b +MISRAC2012-Rule-14.3_b */
            /** @mdr{00015,0} */
            TRACE_MSG(TRACE_APP1, "Network key broadcast disallowed", (FMT__0));
            pass_pkt_up = ZB_FALSE;
          }

#ifdef ZB_STACK_REGRESSION_TESTING_API
          if (ZB_REGRESSION_TESTS_API().ignore_nwk_key)
          {
            pass_pkt_up = ZB_FALSE;
          }
#endif

          if (pass_pkt_up)
          {
            ZB_IEEE_ADDR_COPY(ind->src_address, dsc->key_data.nwk.source_address);
            ind->key.nwk.key_seq_number = dsc->key_data.nwk.seq_number;
            ZB_MEMCPY(ind->key.nwk.key, dsc->key, ZB_CCM_KEY_SIZE);
            ZB_SCHEDULE_CALLBACK(zb_apsme_transport_key_indication, param);
          }
          else
          {
            TRACE_MSG(TRACE_ERROR, "violating secur policy: distributed security %hd, aps_encr %hd, encrypted by dTCLK %hd",
                      (FMT__H_H_H, is_distributed,
                       aps_encr,
                       is_encr_by_dtclk));
            zb_buf_free(param);
          }
        }
        else
        {
          TRACE_MSG(TRACE_ERROR, "violating secur policy, drop the frame, joined %hd authenticated %hd aps_ecnr %hd",
                    (FMT__H_H_H, ZB_JOINED(), ZG->aps.authenticated, aps_encr));
          zb_buf_free(param);
        }
        break;

#ifndef ZB_LITE_NO_APS_DATA_ENCRYPTION
        /* 4.4.2.3  Upon receipt of a secured TK command, the APSME
         * shall check the key type sub-field. If the key type is set
         * to 0x03 or 0x04 and the receiving device is operating in
         * the joined and authorized state and the command was not
         * secured using a distributed security link key or a TC link
         * key, the command shall be discarded. */
      case ZB_APP_LINK_KEY:

        TRACE_MSG(TRACE_SECUR3, "in app link key for me with " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(dsc->key_data.app.partner_address)));

        if (!ZB_JOINED() ||
            !ZG->aps.authenticated ||
            !aps_encr ||
            /* Drop AppLink key during TC LK exchange - joiner excepts TC LK*/
            ZB_TCPOL().waiting_for_tclk_exchange)
        {
          TRACE_MSG(TRACE_ERROR,
                    "violating secur policy, drop the frame, joined %hd authenticated %hd aps_ecnr %hd, waiting_for_tclk_exchange %hd",
                    (FMT__H_H_H_H, ZB_JOINED(), ZG->aps.authenticated,
                     aps_encr, ZB_TCPOL().waiting_for_tclk_exchange));
          zb_buf_free(param);
        }
        else
        {
          zb_ret_t ret;

          ret = zb_address_ieee_by_short(src_short, ind->src_address);
          ZB_ASSERT(ret == RET_OK);
          if (ret == RET_OK)
          {
            ZB_IEEE_ADDR_COPY(ind->key.app.partner_address, dsc->key_data.app.partner_address);
            ZB_MEMCPY(ind->key.app.key, dsc->key, ZB_CCM_KEY_SIZE);
            ZB_SCHEDULE_CALLBACK(zb_apsme_transport_key_indication, param);
          }
          else
          {
            TRACE_MSG(TRACE_ERROR, "zb_address_ieee_by_short failed %d", (FMT__H, ret));
            ZB_ASSERT(0);
          }
        }
        break;
#endif  /* ZB_LITE_NO_APS_DATA_ENCRYPTION */
      case ZB_TC_LINK_KEY:

        TRACE_MSG(TRACE_SECUR3, "in TCLK for me", (FMT__0));

        if (!ZB_JOINED() ||
            !ZG->aps.authenticated ||
            !aps_encr ||
            !(key_id == ZB_SECUR_KEY_LOAD_KEY))
        {
          TRACE_MSG(TRACE_ERROR,
                    "violating secur policy, drop the frame, joined %hd authenticated %hd, aps_ecnr %hd, key_id %hd",
                    (FMT__H_H_H_H, ZB_JOINED(), ZG->aps.authenticated,
                     aps_encr, key_id));
          zb_buf_free(param);
        }
        else
        {
          zb_aps_device_key_pair_set_t* key_pair = zb_aps_keypair_get_ent_by_idx(keypair_i);
          /* Seems like no link_key here is impossible? Check it! */
          if (key_pair != NULL
              && ZB_MEMCMP(dsc->key, key_pair->link_key, ZB_CCM_KEY_SIZE) == 0
              && ZB_TCPOL().waiting_for_tclk_exchange)
          {
            zb_ret_t ret = zb_buf_get_out_delayed(bdb_update_tclk_failed);

            TRACE_MSG(TRACE_ERROR,
                      "received link key identical to provisional link key - drop packet and fail tclk exchange",
                      (FMT__0));
            if (ret != RET_OK)
            {
              TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
            }
            zb_buf_free(param);
          }
          else
          {
            /* Note: there is no source address in app lk.  */
            ZB_IEEE_ADDR_COPY(ind->src_address, dsc->key_data.tc.source_address);
            ZB_MEMCPY(ind->key.tc.key, dsc->key, ZB_CCM_KEY_SIZE);
            ZB_SCHEDULE_CALLBACK(zb_apsme_transport_key_indication, param);
          }
        }
        break;

      default:
        TRACE_MSG(TRACE_ERROR, "Transport-key.indication key-type %d not implemented", (FMT__H, dsc->key_type));
        zb_buf_free(param);
        break;
    }
  }
#if defined(ZB_ROUTER_ROLE) && defined(ZB_CERTIFICATION_HACKS)
  /* Not sure: is it real case to receive transport-key and send it to our
   * child? With APS encryption TC uses TUNEL. */
  else if (ZB_CERT_HACKS().aps_security_off)
  {
    zb_address_ieee_ref_t addr_ref;
    zb_neighbor_tbl_ent_t *nbe;
    /* Search for child in the Neighbor table, mark child as Authenticated,
     * send key to it using unsecured NWK transfer */
    if (zb_address_by_ieee(dest_addr, ZB_FALSE, ZB_FALSE, &addr_ref) == RET_OK
        && zb_nwk_neighbor_get(addr_ref, ZB_FALSE, &nbe) == RET_OK
        && (nbe->relationship == ZB_NWK_RELATIONSHIP_UNAUTHENTICATED_CHILD
            || nbe->relationship == ZB_NWK_RELATIONSHIP_CHILD))
    {
      zb_uint16_t addr;
      zb_address_short_by_ref(&addr, addr_ref);
      TRACE_MSG(TRACE_SECUR3, "send key to child ZE 0x%x, auth ok", (FMT__D, addr));
      zb_aps_send_command(param, addr, APS_CMD_TRANSPORT_KEY,
                              (zb_bool_t)(nbe->relationship != (zb_ushort_t)ZB_NWK_RELATIONSHIP_UNAUTHENTICATED_CHILD)

                          , zb_secur_aps_send_policy(APS_CMD_TRANSPORT_KEY, addr, dsc->key_type)
                            );
      nbe->relationship = ZB_NWK_RELATIONSHIP_CHILD;
    }
    else
    {
      TRACE_MSG(TRACE_SECUR1, "child " TRACE_FORMAT_64 " not found", (FMT__A, TRACE_ARG_64(dsc->key_data.nwk.dest_address)));
      zb_buf_free(param);
    }
  }
#endif  /* ZB_ROUTER_ROLE && ZB_CERTIFICATION_HACKS */
  else
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_SECUR3, "<<zb_aps_in_transport_key", (FMT__0));
}
#endif  /* #ifndef ZB_COORDINATOR_ONLY */


#ifdef ZB_COORDINATOR_ROLE
/**
   Reaction on incoming UPDATE-DEVICE

   Issue UPDATE-DEVICE.indication
 */
void zb_aps_in_update_device(zb_uint8_t param)
{
  /* get source address from the nwk header and convert it to long address */
  zb_apsme_update_device_pkt_t *pkt = (zb_apsme_update_device_pkt_t *)zb_buf_begin(param);
  zb_apsme_update_device_ind_t *ind = ZB_BUF_GET_PARAM(param, zb_apsme_update_device_ind_t);
  zb_uint16_t device_short_address;
  zb_ret_t ret;

  TRACE_MSG(TRACE_SECUR3, ">>zb_aps_in_update_device %d", (FMT__H, param));

  ZB_LETOH16((zb_uint8_t *)&device_short_address, (zb_uint8_t *)&pkt->device_short_address);

  /* Check address conflict */
#ifdef ZB_CERTIFICATION_HACKS
  if (!ZB_CERT_HACKS().disable_addr_conflict_check_on_update_device)
#endif
  {
    ret = zb_nwk_is_conflict_addr(device_short_address, pkt->device_address);
  }
#ifdef ZB_CERTIFICATION_HACKS
  else
  {
    ret = RET_OK;
  }
#endif

  if (ret == RET_OK)
  {
    zb_address_ieee_ref_t ref;
    zb_uint16_t src_short_addr = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t)->src_addr;

    zb_address_ieee_by_short(src_short_addr, ind->src_address);

    ZB_IEEE_ADDR_COPY(ind->device_address, pkt->device_address);
    ind->device_short_address = device_short_address;

    /* We have short and long addresses of the device UPDATE-DEVICE is
     * about. Remember it (if it is allowed). */
    if ((
#ifdef ZB_CERTIFICATION_HACKS
         !ZB_CERT_HACKS().aps_disable_addr_update_on_update_device &&
#endif
         zb_address_update(ind->device_address, ind->device_short_address, ZB_FALSE, &ref) == RET_OK
        )
        || zb_address_by_ieee(ind->device_address, ZB_FALSE, ZB_FALSE, &ref) == RET_OK
        || zb_address_by_short(ind->device_short_address, ZB_FALSE, ZB_FALSE, &ref) == RET_OK )
    {
      /* Remove from NBT -  */
      /* TODO: Why do we need that? */
      zb_nwk_neighbor_delete(ref);
    }

    ind->status = pkt->status;

    ZB_SCHEDULE_CALLBACK(zb_apsme_update_device_indication, param);
  }
  else
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_SECUR3, "<<zb_aps_in_update_device", (FMT__0));
}


void zb_secur_apsme_remove_device(zb_uint8_t param)
{
  zb_apsme_remove_device_req_t *req = ZB_BUF_GET_PARAM(param, zb_apsme_remove_device_req_t);
  zb_apsme_remove_device_pkt_t *p;

  TRACE_MSG(TRACE_APS1, ">>zb_secur_apsme_remove_device", (FMT__0));

  p = zb_buf_initial_alloc(param, sizeof(zb_apsme_remove_device_pkt_t));

  ZB_IEEE_ADDR_COPY(p->child_address, req->child_address);
  TRACE_MSG(TRACE_SECUR2, "send REMOVE-DEVICE " TRACE_FORMAT_64 " to " TRACE_FORMAT_64, (FMT__A_A, TRACE_ARG_64(p->child_address),
                          TRACE_ARG_64(req->parent_address)));

  if (zb_address_short_by_ieee(req->parent_address) != (zb_uint16_t)~0)
  {
    zb_aps_send_command(param,
                        zb_address_short_by_ieee(req->parent_address),
                        APS_CMD_REMOVE_DEVICE, ZB_TRUE
                        , zb_secur_aps_send_policy(APS_CMD_REMOVE_DEVICE, 0, 0)
    );
  }
  else
  {
    TRACE_MSG(TRACE_APS1, "error - unable to get short address", (FMT__0));
    zb_buf_free(param);
  }
  TRACE_MSG(TRACE_APS1, "<<zb_secur_apsme_remove_device", (FMT__0));
}
#endif  /* ZB_COORDINATOR_ROLE */

#ifdef ZB_ROUTER_SECURITY
void zb_aps_in_remove_device(zb_uint8_t param)
{
  zb_ret_t ret;
  /* get source address from the nwk header and convert it to long address */
  zb_apsme_remove_device_pkt_t *pkt = (zb_apsme_remove_device_pkt_t *)zb_buf_begin(param);
  zb_apsme_remove_device_ind_t *ind = ZB_BUF_GET_PARAM(param, zb_apsme_remove_device_ind_t);

  TRACE_MSG(TRACE_SECUR3, ">>zb_aps_in_remove_device %d", (FMT__H, param));

  {
    zb_uint16_t src_short_addr = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t)->src_addr;
    ret = zb_address_ieee_by_short(src_short_addr, ind->src_address);
  }

  if (ret == RET_OK)
  {
    ZB_IEEE_ADDR_COPY(ind->child_address, pkt->child_address);

#ifdef ZB_CERTIFICATION_HACKS
    if (ZB_CERT_HACKS().stay_on_network_after_auth_failure)
    {
      zb_buf_free(param);
    }
  else
#endif /* ZB_CERTIFICATION_HACKS */
    {
      ZB_SCHEDULE_CALLBACK(zb_apsme_remove_device_indication, param);
    }
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "zb_address_ieee_by_short failed %d", (FMT__H, ret));
    ZB_ASSERT(0);
  }

  TRACE_MSG(TRACE_SECUR3, "<<zb_aps_in_remove_device", (FMT__0));
}
#endif  /* ZB_ROUTER_SECURITY */

#ifdef ZB_COORDINATOR_ROLE
void zb_aps_in_request_key(zb_uint8_t param, zb_uint16_t keypair_i
                           , zb_secur_key_id_t key_id
                          )
{
  /* get source address from the nwk header and convert it to long address */
  zb_apsme_request_key_pkt_t *pkt = (zb_apsme_request_key_pkt_t *)zb_buf_begin(param);
  zb_apsme_request_key_ind_t *ind = ZB_BUF_GET_PARAM(param, zb_apsme_request_key_ind_t);
#ifdef ZB_CERTIFICATION_HACKS
  zb_bool_t drop_cmd = ZB_FALSE;
#endif

  TRACE_MSG(TRACE_SECUR2, ">>zb_aps_in_request_key param %hd keypair_i %d", (FMT__H_D, param, keypair_i));
  {
    zb_ret_t ret;
    zb_uint16_t src_short_addr = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t)->src_addr;
    ret = zb_address_ieee_by_short(src_short_addr, ind->src_address);
    /* Invalid address, stop execution */
    if (ret != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "zb_address_ieee_by_short failed %d", (FMT__H, ret));
      ZB_ASSERT(0);
      goto done_zb_aps_in_request_key;
    }
  }
  ind->key_type = pkt->key_type;
  ind->keypair_i = keypair_i;
  ZB_IEEE_ADDR_COPY(ind->partner_address, pkt->partner_address);

#ifdef ZB_CERTIFICATION_HACKS
  if (ZB_CERT_HACKS().req_key_ind_cb)
  {
    drop_cmd = ZB_CERT_HACKS().req_key_ind_cb(param, keypair_i);
  }

  if (drop_cmd == ZB_TRUE)
  {
    TRACE_MSG(TRACE_SECUR2, "Drop incoming Request Key", (FMT__0));
    zb_buf_free(param);
  }
  else
#endif
  {
    if ((key_id != ZB_SECUR_DATA_KEY)
        && !ZB_TCPOL().aps_unencrypted_transport_key_join
       )
    {
      TRACE_MSG(TRACE_SECUR2, "Request key protected with wrong key_type - %hd (expected ZB_SECUR_DATA_KEY)",
                (FMT__H, key_id));
      zb_buf_free(param);
    }
    else
    {
      ZB_SCHEDULE_CALLBACK(zb_apsme_request_key_indication, param);
    }
  }

done_zb_aps_in_request_key:
  TRACE_MSG(TRACE_SECUR2, "<<zb_aps_in_request_key", (FMT__0));
}

void zb_aps_in_verify_key(zb_uint8_t param)
{
  /* get source address from the nwk header and convert it to long address */
  zb_apsme_verify_key_pkt_t *pkt = (zb_apsme_verify_key_pkt_t *)zb_buf_begin(param);
  zb_apsme_verify_key_ind_t *ind = ZB_BUF_GET_PARAM(param, zb_apsme_verify_key_ind_t);
  zb_uint16_t dst_short_addr = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t)->dst_addr;

  TRACE_MSG(TRACE_SECUR2, ">>zb_aps_in_verify_key param %hd", (FMT__H, param));
  if (ZB_NWK_IS_ADDRESS_BROADCAST(dst_short_addr))
  {
    /* 1. If the message is a NWK broadcast, the request shall be dropped and no further processing shall be done. */
    TRACE_MSG(TRACE_SECUR1, "Drop verify key to broadcast nwk %d", (FMT__D, dst_short_addr));
    zb_buf_free(param);
  }
  else
  {
    ind->key_type = pkt->key_type;
    ZB_IEEE_ADDR_COPY(ind->src_address, pkt->src_address);
    ZB_MEMCPY(ind->key_hash, pkt->key_hash, ZB_CCM_KEY_SIZE);

#ifdef ZB_CERTIFICATION_HACKS
    if (ZB_CERT_HACKS().drop_verify_key_indication)
    {
      zb_buf_free(param);
    }
    else
#endif
    {
      ZB_SCHEDULE_CALLBACK(zb_apsme_verify_key_indication, param);
    }
  }
  TRACE_MSG(TRACE_SECUR2, "<<zb_aps_in_verify_key", (FMT__0));
}
#endif  /* ZB_COORDINATOR_ROLE */

#ifndef ZB_COORDINATOR_ONLY
void zb_aps_in_switch_key(zb_uint8_t param)
{
  zb_ret_t ret;
  /* get source address from the nwk header and convert it to long address */
  zb_apsme_switch_key_pkt_t *pkt = (zb_apsme_switch_key_pkt_t *)zb_buf_begin(param);
  zb_apsme_switch_key_ind_t *ind = ZB_BUF_GET_PARAM(param, zb_apsme_switch_key_ind_t);
  {
    zb_uint16_t src_short_addr = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t)->src_addr;
    ret = zb_address_ieee_by_short(src_short_addr, ind->src_address);
    ZB_ASSERT(ret == RET_OK);
  }

  if (ret == RET_OK)
  {
    ind->key_seq_number = pkt->key_seq_number;
    ZB_SCHEDULE_CALLBACK(zb_apsme_switch_key_indication, param);
  }
  else
  {
    TRACE_MSG(TRACE_SECUR2, "zb_address_ieee_by_short invalid address %d", (FMT__H, ret));
  }

  TRACE_MSG(TRACE_SECUR3, "<<zb_aps_in_switch_key", (FMT__0));
}


void zb_aps_in_confirm_key(zb_uint8_t param)
{
  zb_ret_t ret;
  /* get source address from the nwk header and convert it to long address */
  zb_apsme_confirm_key_pkt_t *pkt = (zb_apsme_confirm_key_pkt_t *)zb_buf_begin(param);
  zb_apsme_confirm_key_ind_t *ind = ZB_BUF_GET_PARAM(param, zb_apsme_confirm_key_ind_t);
  zb_uint16_t src_short_addr = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t)->src_addr;
  /* If the message is a NWK broadcast, the request shall be dropped and no further processing shall be done. */
  if (ZB_NWK_IS_ADDRESS_BROADCAST(ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t)->dst_addr))
  {
    TRACE_MSG(TRACE_SECUR1, "NWK broadcast confirm_key - drop", (FMT__0));
    ret = RET_ERROR;
  }
  else
  {
    ind->key_type = pkt->key_type;
    ind->status = pkt->status;
    ZB_BZERO(ind->src_address, sizeof(ind->src_address));
    ret = zb_address_ieee_by_short(src_short_addr, ind->src_address);
    TRACE_MSG(TRACE_SECUR2, "zb_address_ieee_by_short invalid address %d", (FMT__H, ret));
    ZB_ASSERT(ret == RET_OK);
  }

  if (ret == RET_OK)
  {
    ZB_SCHEDULE_CALLBACK(zb_apsme_confirm_key_indication, param);
  }
  else
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_SECUR3, "<<zb_aps_in_confirm_key", (FMT__0));
}


void zb_secur_apsme_request_key(zb_uint8_t param)
{
  zb_apsme_request_key_req_t *req = ZB_BUF_GET_PARAM(param, zb_apsme_request_key_req_t);
  zb_apsme_request_key_pkt_t *p;
  zb_uint8_t size = (zb_uint8_t)sizeof(zb_apsme_request_key_pkt_t)
                    - (req->key_type == ZB_REQUEST_APP_LINK_KEY ? 0U : (zb_uint8_t)sizeof(zb_ieee_addr_t));
  zb_uint16_t dest_short;

  p = zb_buf_initial_alloc(param, size);
  p->key_type = req->key_type;
  if(req->key_type==ZB_REQUEST_APP_LINK_KEY)
  {
    ZB_IEEE_ADDR_COPY(p->partner_address, req->partner_address);
  }

  if(
      (req->key_type != ZB_REQUEST_APP_LINK_KEY) &&
      (req->key_type != ZB_REQUEST_TC_LINK_KEY) )
  {
    TRACE_MSG(TRACE_ERROR, "Unexpected REQUEST-KEY request. Key type=%d ", (FMT__H, req->key_type));
  }

  TRACE_MSG(TRACE_SECUR3, "send REQUEST-KEY to " TRACE_FORMAT_64,
            (FMT__A, TRACE_ARG_64(req->dest_address)));
  dest_short = zb_address_short_by_ieee(req->dest_address);
  zb_aps_send_command(param,
                      dest_short,
                      APS_CMD_REQUEST_KEY, ZB_TRUE
                      , zb_secur_aps_send_policy(APS_CMD_REQUEST_KEY, dest_short, 0)
  );
}


void zb_apsme_verify_key_req(zb_uint8_t param)
{
  zb_apsme_verify_key_req_t *req = ZB_BUF_GET_PARAM(param, zb_apsme_verify_key_req_t);
  zb_aps_device_key_pair_set_t *aps_key = zb_secur_get_link_key_by_address(ZB_AIB().trust_center_address, ZB_SECUR_UNVERIFIED_KEY);
  zb_uint16_t tc_addr = zb_address_short_by_ieee(ZB_AIB().trust_center_address);

  if (req->key_type != ZB_TC_LINK_KEY
      || !ZB_IEEE_ADDR_CMP(req->dest_address, ZB_AIB().trust_center_address)
      || aps_key == NULL
      || tc_addr == (zb_uint16_t)~0U)
  {
    TRACE_MSG(TRACE_SECUR1, "drop apsme_verify_key_req", (FMT__0));
    zb_buf_free(param);
  }
  else
  {
    zb_apsme_verify_key_pkt_t *dsc;
    dsc = zb_buf_initial_alloc(param, sizeof(*dsc));
    /*
    The APSME shall generate an APS Command Verify-Key setting the
    StandardKeyType in the command to the StandardKeyType of this primitive,
    and setting the Hash value to the calculated Initiator Verify-Key Hash
    Value. The APS command shall not be APS encrypted.
    */
    dsc->key_type = ZB_TC_LINK_KEY;
    ZB_IEEE_ADDR_COPY(dsc->src_address, &ZB_PIBCACHE_EXTENDED_ADDRESS());
    /* The Initiator Verify-Key Hash Value shall be calculated according to
     * section 4.5.3 using the LinkKey value of the corresponding
     * apsDeviceKeyPairSet entry found in step 5.  */
    /* 4.4.10.7.4 Initiator Verify-Key Hash Value
       This value is the outcome of executing the specialized keyed hash
       function specified in section B.1.4 using  a key with the 1-octet string
       0x03 as the input string. */
    zb_cmm_key_hash(aps_key->link_key, 3, dsc->key_hash);
    TRACE_MSG(TRACE_SECUR2, "sending apsme_verify_key_req to tc %d", (FMT__D, tc_addr));
    zb_aps_send_command(param,
                        tc_addr,
                        APS_CMD_VERIFY_KEY, ZB_TRUE,
                        zb_secur_aps_send_policy(APS_CMD_VERIFY_KEY, 0, 0));
  }
}
#endif  /* #ifndef ZB_COORDINATOR_ONLY */

/**
   Define policy of using APS security for command frames.

   See r20 errate 4.	Multi-hop Join Incompatibility

   @return key id to be used
 */
zb_uint8_t zb_secur_aps_send_policy(zb_uint_t command, zb_uint16_t dest_addr, zb_uint8_t key_type)
{
  zb_secur_key_id_t ret;

  /* key_type is used only for APS_CMD_TRANSPORT_KEY command */
#if ! (defined(ZB_FORMATION) || (defined(ZB_ROUTER_ROLE) && defined(ZB_CERTIFICATION_HACKS)))
  ZVUNUSED(key_type);
#endif

  if (ZB_NIB_SECURITY_LEVEL() == 0U
#ifdef ZB_CERTIFICATION_HACKS
      || ZB_CERT_HACKS().aps_security_off
#endif
      || (ZB_TCPOL().aps_unencrypted_transport_key_join && (command == APS_CMD_TRANSPORT_KEY))
     )
  {
    return ZB_NOT_SECUR;
  }

/*
  4.4.1.3 Security Processing of APS commands

  Table Y.YY Security Policy for sending APS commands
*/

  /*
    APS Command             Unique Trust Center Link Key	Global Trust Center Link Key
  */
  switch (command)
  {
#if defined(ZB_FORMATION) || (defined(ZB_ROUTER_ROLE) && defined(ZB_CERTIFICATION_HACKS))
    case APS_CMD_TRANSPORT_KEY:
      /*

        Transport Key (0x05)	APS encryption may be optionally used.
        See section 4.4.1.5	APS encryption may be optionally used.  See section 4.4.1.5

        4.4.1.5 Acceptance of Commands based on Security Policy

        The APS transport key command may be sent with or without APS encryption.  The
        decision to do so is based on the trust center's security policies.  The trust
        center may deem it acceptable to send a key without APS encryption based on the
        type, or simply choose to send all requests unencrypted.

        Now always encrypt, but may change it in the future.

      */
      ret = (key_type == ZB_STANDARD_NETWORK_KEY) ? ZB_SECUR_KEY_TRANSPORT_KEY : ZB_SECUR_KEY_LOAD_KEY;
      break;

#endif /* defined(ZB_FORMATION) || (defined(ZB_ROUTER_ROLE) && defined(ZB_CERTIFICATION_HACKS)) */
    case APS_CMD_UPDATE_DEVICE:
    {
      /*
        Update Device (0x06)	APS encryption shall be used.	APS encryption shall be conditionally used as per section 4.4.1.4.


        4.4.1.4 Conditional Encryption of APS commands

        When sending an APS command that must be conditionally encrypted, the
        device shall send the APS command with or without APS encryption
        according to the requirements of the receiving device.  If the receiving
        device is capable of accepting APS encrypted APS commands then the
        sending device may send APS encrypted APS commands.  If the receiving
        device is not capable of receiving APS encrypted APS commands then the
        sending device may send APS commands without APS encryption.

        It is left up to the implementers to determine whether or not the
        receiving device is capable of receiving an APS command with APS
        encryption.
      */

      /* r21: url: rg.ru/2015/12/10/koty-site-anons.html */

#ifdef ZB_CERTIFICATION_HACKS
      /* Hack for ZR in test TP_R20_BV-01: ZR must send second UPDATE
         DEVICE unecrypted. */
      if (ZB_CERT_HACKS().send_update_device_unencrypted == 0)
      {
        ZB_CERT_HACKS().send_update_device_unencrypted--;
        TRACE_MSG(TRACE_SECUR1, "policy for update-device addr 0x%x - not secur (TC hack)",
                  (FMT__D, dest_addr));
        ret = ZB_NOT_SECUR;
      }
      else
      {
#endif
        /* in r21 must send 2 copy of update-device for global key: encrypted
         * and not encrypted. Send only encrypted for unique key */
        zb_ieee_addr_t ieee_addr;

#ifdef ZB_CERTIFICATION_HACKS
        if (ZB_CERT_HACKS().send_update_device_unencrypted > 0)
        {
          ZB_CERT_HACKS().send_update_device_unencrypted--;
          TRACE_MSG(TRACE_SECUR1, "policy for update-device addr 0x%x - data key (TC hack)", (FMT__D, dest_addr));
          ret = ZB_SECUR_DATA_KEY;
        }
        else
        {
#endif
          if (zb_address_ieee_by_short(dest_addr, ieee_addr) == RET_OK)
          {
            zb_aps_device_key_pair_set_t *aps_key = zb_secur_get_link_key_pair_set(ieee_addr, ZB_FALSE);
            if (aps_key == NULL
#ifndef ZB_LITE_NO_GLOBAL_VS_UNIQUE_KEYS
                || aps_key->aps_link_key_type == ZB_SECUR_GLOBAL_KEY
#endif
              )
            {
              TRACE_MSG(TRACE_SECUR1, "policy for update-device addr 0x%x aps-key %p / global - send twice ", (FMT__D_P, dest_addr, aps_key));
              ret = ZB_SECUR_DATA_KEY_N_UNENCR;
            }
            else
            {
              TRACE_MSG(TRACE_SECUR1, "policy for update-device addr 0x%x - data key", (FMT__D, dest_addr));
              ret = ZB_SECUR_DATA_KEY;
            }
          }
          else
          {
            TRACE_MSG(TRACE_SECUR1, "policy for update-device addr 0x%x - send twice ", (FMT__D, dest_addr));
            ret = ZB_SECUR_DATA_KEY_N_UNENCR;
          }
#ifdef ZB_CERTIFICATION_HACKS
          /* MISRA Rule 16.1 - Additional blocks inside the case clause block are not allowed. */
        }
      }
#endif /* ZB_CERTIFICATION_HACKS */
      break;
    }

    case APS_CMD_REMOVE_DEVICE:
      /* Remove Device (0x07)	APS encryption shall be used	APS encryption  shall be used */
      ret = ZB_SECUR_DATA_KEY;
      break;

    case APS_CMD_REQUEST_KEY:
      /* Request Key (0x08)	APS encryption shall be used	APS encryption shall not be used */
      /* Request Key (0x08)
         APS encryption required Trust Center Policy may further restrict, see section 4.4.1.5
         APS encryption required Trust Center Policy may further restrict, see section 4.4.1.5 */
      ret = ZB_SECUR_DATA_KEY;
      break;

    case APS_CMD_SWITCH_KEY:
      /* Switch Key (0x09)	APS encryption may be used	APS encryption shall not be used */
      ret = ZB_NOT_SECUR;
      break;

    case APS_CMD_TUNNEL:
      /* Tunnel Data (0x0E)	APS encryption shall not be used	APS encryption shall not be used */
      ret = ZB_NOT_SECUR;
      break;

    case APS_CMD_VERIFY_KEY:
      /*
        Verify-Key (0x0F)
        APS encryption not required.
        APS encryption not required
      */
      ret = ZB_NOT_SECUR;
      break;
#if defined(ZB_COORDINATOR_ROLE)
    case APS_CMD_CONFIRM_KEY:
/*
  Confirm-Key (0x10)
  APS encryption required
  APS encryption required.
*/
      ret = ZB_SECUR_DATA_KEY;
      break;
#endif /* defined(ZB_COORDINATOR_ROLE) */
    default:
      TRACE_MSG(TRACE_ERROR, "Unexpected command 0x%x", (FMT__D, command));
      ZB_ASSERT(0);
      /* Just in case... */
      ret = ZB_NOT_SECUR;
      break;
  } /* switch */

  return ((zb_uint8_t)ret);
}

/**
   Check that APS command can be accepted depending on its APS encryption

   @return TRUE if command accepted, FALSE if not accepted and must be ignored
 */
zb_bool_t zb_secur_aps_accept_policy(zb_uint8_t cmd_id, zb_bool_t secured, zb_uint16_t keypair_i)
{
  zb_bool_t ret;
  zb_bool_t is_global_tc_key;

  TRACE_MSG(TRACE_SECUR1, "zb_secur_aps_accept_policy: cmd_id %hd secured %hd keypair_i %d", (FMT__H_H_D, cmd_id, secured, keypair_i));
  /* 053474r19, new sections 4.4.1.3 and 4.4.1.4,
     4.4.1.3 Security Processing of APS commands*/

  /*
    If APS encryption is not required for the command but the received message has
    APS encryption, the receiving device shall accept and process the message.
    Accepting additional security on messages is required to support legacy devices
    in the field.
  */
  if (
#ifdef ZB_CERTIFICATION_HACKS
      ZB_CERT_HACKS().aps_security_off
      || (!ZB_CERT_HACKS().disable_encrypted_update_device
          && !ZB_CERT_HACKS().disable_unencrypted_update_device
          && secured)
#else
      secured
#endif
      || ZB_NIB_SECURITY_LEVEL() == 0U
      || (ZB_TCPOL().aps_unencrypted_transport_key_join &&
          (cmd_id == APS_CMD_UPDATE_DEVICE || cmd_id == APS_CMD_REQUEST_KEY))
     )
  {
    return ZB_TRUE;
  }

  if (keypair_i == (zb_uint16_t)-1)
  {
    is_global_tc_key = ZB_TRUE;
  }
  else
  {
#ifndef ZB_LITE_NO_GLOBAL_VS_UNIQUE_KEYS
    zb_aps_device_key_pair_set_t* key_pair = zb_aps_keypair_get_ent_by_idx(keypair_i);
    is_global_tc_key = key_pair != NULL && key_pair->aps_link_key_type == ZB_SECUR_GLOBAL_KEY;
#else
    is_global_tc_key = ZB_FALSE;
#endif
  }

  /* Next checks for acceptance when no security */

  /*
    4.4.1.3 Security Processing of APS commands

    APS Command	                Unique Link Key (0x00)          Global Link Key (0x01)

   */
  switch (cmd_id)
  {
    /*     SKKE Commands (0x01 - 0x04)	APS encryption not required	APS encryption not required */

    case APS_CMD_TRANSPORT_KEY:
    /* Transport Key (0x05)	APS encryption is required as per device policy (See section 4.4.1.5)  */

      /*
The APS transport key command may be sent with or without APS encryption.  The
decision to do so is based on the trust center's security policies.  The trust
center may deem it acceptable to send a key without APS encryption based on the
type, or simply choose to send all requests unencrypted.

Conversely, a device receiving an APS transport key command may choose whether
or not APS encryption is required.  This is most often done during initially
joining.  For example, during joining a device that has no preconfigured link
key would only accept unencrypted transport key messages, while a device with a
preconfigured link key would only accept a transport key APS encrypted with its
preconfigured key.


A device that is in the joined and authorized state shall accept a broadcast NWK
key update sent by the Trust Center using only NWK encryption. A device that is
in state of joined and unauthorized shall require an APS encrypted transport key
if it has a preconfigured link key.
       */
      ret = ZB_TRUE; /* depends on policy, currently for ZCP we need it TRUE, just to accept
                       * APS unencrypted broadcasts   */
      break;

    case APS_CMD_UPDATE_DEVICE:
    {
      /* Update Device (0x06)	APS encryption required	APS encryption not required */
#ifdef ZB_CERTIFICATION_HACKS
      if (ZB_CERT_HACKS().disable_unencrypted_update_device)
      {
        ret = (secured && (is_global_tc_key != 0)) ? (ZB_TRUE): (ZB_FALSE);
      }
      else if (ZB_CERT_HACKS().disable_encrypted_update_device)
      {
        ret = (zb_bool_t)!secured;
      }
      else
      {
#endif /* ZB_CERTIFICATION_HACKS */
        ret = (is_global_tc_key == 0) ? (ZB_FALSE) : (ZB_TRUE);
#ifdef ZB_CERTIFICATION_HACKS
        /* MISRA Rule 16.1 - Additional blocks inside the case clause block are not allowed. */
      }
#endif /* ZB_CERTIFICATION_HACKS */
      break;
    }

    case APS_CMD_REMOVE_DEVICE:
      /* Remove Device (0x07)	APS encryption required	APS encryption required */
      ret = ZB_FALSE;
      break;

    case APS_CMD_REQUEST_KEY:
      /* Request Key (0x08)	APS encryption required  Trust Center Policy may further restrict, see Section 4.4.1.5	APS encryption not required
Trust Center Policy may further restrict, see section 4.4.1.5
 */
      ret = ZB_FALSE;
      break;

    case APS_CMD_SWITCH_KEY:
      /* Switch Key (0x09)	APS encryption not required	APS encryption not required */
      ret = ZB_TRUE;
      break;

    case APS_CMD_TUNNEL:
      /* Tunnel Data (0x0E)
APS encryption not required
APS encryption not required */
      ret = ZB_TRUE;
      break;

    case APS_CMD_VERIFY_KEY:
      /* Verify-Key (0x0F)
APS encryption not required.
APS encryption not required */
      ret = ZB_TRUE;
      break;

    case APS_CMD_CONFIRM_KEY:
      /* Confirm-Key (0x10)
APS encryption required
APS encryption required. */
      ret = ZB_FALSE;
      break;

    default:
      ret = ZB_TRUE;
      break;
  }

  return ret;
}


/*! @} */
