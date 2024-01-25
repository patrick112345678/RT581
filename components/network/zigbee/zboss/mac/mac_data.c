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
/* PURPOSE: Roitines specific mac data transfer
*/

#define ZB_TRACE_FILE_ID 301
#include "zb_common.h"

#if !defined ZB_ALIEN_MAC && !defined ZB_MACSPLIT_HOST

#include "zb_scheduler.h"
#include "zb_nwk.h"
#include "zb_mac.h"
#include "zb_mac_globals.h"
#include "mac_internal.h"
#include "zb_mac_transport.h"
#include "zb_secur.h"
#include "zdo_wwah_stubs.h"

/*! \addtogroup ZB_MAC */
/*! @{ */


void zb_direct_mcps_data_req_tx_done(zb_uint8_t param);
void zb_mlme_send_data_req_done(zb_uint8_t param);

/* Indirect used for associate as well, so can't use ZB_ED_FUNC there */
#ifndef ZB_COORDINATOR_ONLY
/*
  gets pending data from a coordinator (get data indirectly)
  @param params - parameters for data request command
  @return RET_OK, RET_ERROR, RET_BLOCKED, RET_PENDING
  RET_PENDING is returned if there is more data available on
  coordinator
*/
void zb_mac_get_indirect_data(zb_mlme_data_req_params_t *params)
{
  zb_mac_mhr_t mhr;
  zb_uint8_t packet_length;
  zb_uint8_t mhr_len = 0;
  zb_uint8_t *ptr;

/*
  7.5.6.3 Extracting pending data from a coordinator
  NON-beacon-enabled case is implememnted

  - send data request command with ack = 1
  - wait for ack for macAckWaitDuration symbols (7.5.6.4.3 Retransmissions)
*/

  TRACE_MSG(TRACE_MAC2, ">>zb_mac_get_indirect_data", (FMT__0));
  /* send data req cmd */

/*
  7.3.4 Data request command

  If data request command is being sent in response to the receipt
  of a beacon frame, check spec 7.3.4 Data request command for more details:
  set dst and src addr and addr mode according to values specified in
  the beacon frame

  -= USE THIS INFO ON THE CALLER SIDE =-
  - if MLME-POLL.request triggeres data req cmd, set dst addr mode and
  dst addr the same as in the MLME-POLL.req;
  - src addr/mode: if macShortAddress is assigned (is less than
  0xfffe), then use short addr; use extended addr otherwise

  -= USE THIS INFO ON THE CALLER SIDE =-
  - if association req triggers data req cmd, set dst addr info
  according to coordinator addr to wich the request is directed: if
  macCoordShortAddress == 0xfffe, use extended addressing; use short
  addressing otherwise
  - use extended addressing for src addr/mode

  - if dst addr mode == ZB_ADDR_NO_ADDR, see spec for more details
  (beacon frame case)
  - if dst addr is specified, set FCF.PAN id compression = 1, dst PAN
  id = macPANId, src PAN id is omitted.
  - set FCF.Frame pending = 0
  - set FCF.Acknowledgment Request = 1
*/

  /* WARNING: Seems that the code below is outdated */
#if defined ZB_TRANSPORT_8051_DATA_SPI && defined ZB_ED_ROLE && !defined ZB_DBG_NO_IDLE
  if (ZB_MAC_GET_TRANS_SPLEEPING())
  {
    TRACE_MSG(TRACE_COMMON3, "transceiver, WAKE UP", (FMT__0));
    ZB_MAC_CLEAR_TRANS_SPLEEPING();
    zb_uz2400_register_wakeup();
  }
#endif

#if ZB_TRANSCEIVER_ON_BEFORE_TX
  //ZB_TRANSCEIVER_SET_RX_ON_OFF(1);
#endif /* ZB_TRANSCEIVER_ON_BEFORE_TX */

  /* | MHR | Cmd frame id (1 byte) | */

  if (params->dst_addr_mode != ZB_ADDR_NO_ADDR)
  {
    mhr_len = zb_mac_calculate_mhr_length(params->src_addr_mode, params->dst_addr_mode, ZB_TRUE);
  }
  else
  {
    TRACE_MSG(TRACE_MAC1, "dst addr mode ZB_ADDR_NO_ADDR is not supported (beacon frame case)", (FMT__0));
    ZB_ASSERT(0);
  }
  packet_length = mhr_len;
  packet_length += (zb_uint8_t)sizeof(zb_uint8_t);

  ZB_MEMCPY(&mhr.dst_addr, &params->dst_addr, sizeof(zb_addr_u));
  mhr.src_pan_id = 0;
  mhr.dst_pan_id = MAC_PIB().mac_pan_id;
  ZB_MEMCPY(&mhr.src_addr, &params->src_addr, sizeof(zb_addr_u));
  /* mac spec 7.5.6.1 Transmission */
  mhr.seq_number = ZB_MAC_DSN();
  ZB_INC_MAC_DSN();

/* Fill Frame Control then call zb_mac_fill_mhr()
   mac spec  7.2.1.1 Frame Control field
   | Frame Type | Security En | Frame Pending | Ack.Request | PAN ID Compres | Reserv | Dest.Addr.Mode | Frame Ver | Src.Addr.gMode |
*/
  ZB_BZERO(mhr.frame_control, sizeof(mhr.frame_control));

  ZB_FCF_SET_FRAME_TYPE(mhr.frame_control, MAC_FRAME_COMMAND);
  /* security enable is 0 */
  /* frame pending is 0 */
  ZB_FCF_SET_ACK_REQUEST_BIT(mhr.frame_control, 1U);

  ZB_FCF_SET_PANID_COMPRESSION_BIT(mhr.frame_control, 1U);
  ZB_FCF_SET_DST_ADDRESSING_MODE(mhr.frame_control, params->dst_addr_mode);
  ZB_FCF_SET_FRAME_VERSION(mhr.frame_control, MAC_FRAME_VERSION);
  ZB_FCF_SET_SRC_ADDRESSING_MODE(mhr.frame_control, params->src_addr_mode);

  /*re-alloc tx buffer. */
  ptr = zb_buf_initial_alloc(MAC_CTX().operation_buf, packet_length);
  ZB_ASSERT(ptr);

  ZB_BZERO(ptr, packet_length);

  zb_mac_fill_mhr(ptr, &mhr);

  ptr += mhr_len;
  *ptr = MAC_CMD_DATA_REQUEST;

  /* wake up the radio if it is needed! */

  /* callback to setup indirect rx alarm */
  MAC_CTX().tx_wait_cb = zb_mlme_send_data_req_done;

  zb_mac_send_frame(MAC_CTX().operation_buf, mhr_len);

  /*TO DO NEXT:
  - if ack did not come, retry send data req cmd up to macMaxFrameRetries
  - on ack recv, check FCF.Frame pending subfield
  - if pending data is present, wait for macMaxFrameTotalWaitTime symbols to recv data
  - if no data is available on coord, it sends packet with zero payload, no ack is needed
  - if no data is received during timeout, no data is available, set NO_DATA
  - if data is received and ack is needed, send ack
  - if FCF.Frame pending == 1, more data is available
*/


  TRACE_MSG(TRACE_MAC2, "<< zb_mac_get_indirect_data", (FMT__0));
}


/**
   Callback to be called after data request tx done and ack received
 */
void zb_mlme_send_data_req_done(zb_uint8_t param)
{
  zb_mac_status_t mac_status = ZB_GET_MAC_STATUS();

  TRACE_MSG(TRACE_MAC2, ">>zb_mlme_send_data_req_done", (FMT__0));
  ZVUNUSED(param);
  if (mac_status != MAC_SUCCESS)
  {
    TRACE_MSG(TRACE_MAC2, "data req send failed %hd - pass it up", (FMT__H, mac_status));
    //ZB_TRANSCEIVER_SET_RX_ON_OFF(ZB_PIB_RX_ON_WHEN_IDLE());
    zb_mac_indirect_data_rx_failed((zb_uint8_t)mac_status);
  }
  else if (MAC_CTX().flags.in_pending_data)
  {
    TRACE_MSG(TRACE_MAC2, "now waiting for %d ms", (FMT__D, ZB_TIME_BEACON_INTERVAL_TO_MSEC(ZB_MAC_PIB_MAX_FRAME_TOTAL_WAIT_TIME)));
    ZB_SCHEDULE_ALARM(zb_mac_indirect_data_rx_failed,
                      MAC_NO_DATA, /* return NO_DATA after timeout */
                      ZB_MAC_PIB_MAX_FRAME_TOTAL_WAIT_TIME);
  }
  else
  {
    //ZB_TRANSCEIVER_SET_RX_ON_OFF(ZB_PIB_RX_ON_WHEN_IDLE());
    zb_mac_indirect_data_rx_failed(MAC_NO_DATA);
  }
  TRACE_MSG(TRACE_MAC2, "<<zb_mlme_send_data_req_done", (FMT__0));
}
#endif  /* ZB_COORDINATOR_ONLY */

/*
  Adds mac header to the data contained already in data_buf
  forms fata frame according to 7.2.2.2 Data frame format
*/
zb_ret_t zb_mcps_data_request_fill_hdr(zb_bufid_t data_req)
{
  zb_uint8_t *ptr;
  zb_mac_mhr_t mhr = {0};
  zb_ret_t ret = RET_OK;
  zb_mcps_data_req_params_t *data_req_params;

  TRACE_MSG(TRACE_MAC3, ">> zb_mcps_data_request_fill_hdr p %p", (FMT__P, data_req));

  data_req_params = ZB_BUF_GET_PARAM(data_req, zb_mcps_data_req_params_t);
  /* Add parenthesis around 2nd parameter to suppress false-positive from IAR
   * about missing parenthesis for it. */
  zb_buf_set_handle(data_req, (data_req_params->msdu_handle));

#ifndef ZB_MAC_EXT_DATA_REQ
  /* always short addr is used */
  data_req_params->mhr_len = zb_mac_calculate_mhr_length(ZB_ADDR_16BIT_DEV_OR_BROADCAST, ZB_ADDR_16BIT_DEV_OR_BROADCAST, ZB_TRUE);
#else
  data_req_params->mhr_len = zb_mac_calculate_mhr_length(
    data_req_params->src_addr_mode,
    data_req_params->dst_addr_mode,
    (data_req_params->dst_pan_id == MAC_PIB().mac_pan_id));
#endif

  /* In the current implementation only DATA can be encrypted, so it is ok to
   * analyze security only here. */
#ifdef ZB_MAC_SECURITY
  if (data_req_params->security_level)
  {
    /* currently hard-code security level 5 and key_id_mode 1 */
    data_req_params->mhr_len += MAC_SECUR_LEV5_KEYID1_AUX_HDR_SIZE; /* control(1), frame counter(4), key id (1) */
    data_req->u.hdr.encrypt_type |= ZB_SECUR_MAC_ENCR;

    if (MAC_PIB().mac_frame_counter == (zb_uint32_t)~0)
    {
      ret = RET_ERROR;
      ZB_SET_MAC_STATUS(MAC_COUNTER_ERROR);
      TRACE_MSG(TRACE_MAC2, "<< zb_mcps_data_request_fill_hdr ret %d", (FMT__D, ret));
      return ret;
    }
  }
#endif

#if defined ZB_MAC_TESTING_MODE
  if (MAC_CTX().cert_hacks.security_enabled == 1)
  {
    /* Can't use MAC_SECUR_LEV5_KEYID1_AUX_HDR_SIZE macro */
    data_req_params->mhr_len += MAC_SECUR_CERT_AUX_HDR_SIZE;
  }
#endif

  ZB_ASSERT((zb_buf_len(data_req) + (zb_ushort_t)data_req_params->mhr_len) <= ZB_UINT8_MAX);

/* Fill Frame Controll then call zb_mac_fill_mhr()
   mac spec  7.2.1.1 Frame Control field
   | Frame Type | Security En | Frame Pending | Ack.Request | PAN ID Compres | Reserv | Dest.Addr.Mode | Frame Ver | Src.Addr.gMode |
*/
  ZB_BZERO2(mhr.frame_control);
  ZB_FCF_SET_FRAME_TYPE(mhr.frame_control, MAC_FRAME_DATA);

#if defined ZB_MAC_TESTING_MODE
  if (MAC_CTX().cert_hacks.reserved_frame_type == 1)
  {
    /* Decision: don't break logic, i.e. an assertion in the
     * ZB_FCF_SET_FRAME_TYPE, so set Frame Type manually */
    mhr.frame_control[ZB_PKT_16B_ZERO_BYTE] &= 0xF8;
    mhr.frame_control[ZB_PKT_16B_ZERO_BYTE] |= MAC_FRAME_RESERVED1;
  }
#endif

  /* security enable is 0 */
  /* frame pending is 0 */
  ZB_FCF_SET_ACK_REQUEST_BIT(mhr.frame_control, ZB_B2U(ZB_BIT_IS_SET(data_req_params->tx_options,
                                                              MAC_TX_OPTION_ACKNOWLEDGED_BIT)));

#ifndef ZB_MAC_EXT_DATA_REQ
  /* Set panid compress, otherwise Ember don't understand rejoin response */
  ZB_FCF_SET_PANID_COMPRESSION_BIT(mhr.frame_control, 1U);
  ZB_FCF_SET_DST_ADDRESSING_MODE(mhr.frame_control, ZB_ADDR_16BIT_DEV_OR_BROADCAST);
  /* 7.2.3 Frame compatibility: All unsecured frames specified in this
     standard are compatible with unsecured frames compliant with IEEE Std 802.15.4-2003 */
  ZB_FCF_SET_SRC_ADDRESSING_MODE(mhr.frame_control, ZB_ADDR_16BIT_DEV_OR_BROADCAST);
#else
  ZB_FCF_SET_PANID_COMPRESSION_BIT(mhr.frame_control,
                                   ZB_B2U(data_req_params->dst_pan_id == MAC_PIB().mac_pan_id
                                          /* ZGPD does not use src address */
                                          && data_req_params->src_addr_mode != ZB_ADDR_NO_ADDR
                                          && data_req_params->dst_addr_mode != ZB_ADDR_NO_ADDR));
  ZB_FCF_SET_DST_ADDRESSING_MODE(mhr.frame_control, data_req_params->dst_addr_mode);
  ZB_FCF_SET_SRC_ADDRESSING_MODE(mhr.frame_control, data_req_params->src_addr_mode);
#endif

  /* 7.2.3 Frame compatibility: All unsecured frames specified in this
     standard are compatible with unsecured frames compliant with IEEE Std 802.15.4-2003 */
  ZB_FCF_SET_FRAME_VERSION(mhr.frame_control, MAC_FRAME_IEEE_802_15_4_2003);
#ifdef ZB_MAC_SECURITY
  if (data_req_params->security_level)
  {
    ZB_FCF_SET_SECURITY_BIT(mhr.frame_control, 1);
    /* frame security compatible with 2006 */
    ZB_FCF_SET_FRAME_VERSION(mhr.frame_control, MAC_FRAME_IEEE_802_15_4);
  }
#endif

#if defined ZB_MAC_TESTING_MODE
  if (MAC_CTX().cert_hacks.security_enabled == 1)
  {
    ZB_FCF_SET_SECURITY_BIT(mhr.frame_control, 1);
  }
#endif

  /* mac spec 7.5.6.1 Transmission */
  mhr.seq_number = ZB_MAC_DSN();
  ZB_INC_MAC_DSN();

#ifndef ZB_MAC_EXT_DATA_REQ
  /* put our pan id as src and dst pan id */
  mhr.dst_pan_id = MAC_PIB().mac_pan_id;
  mhr.dst_addr.addr_short = data_req_params->dst_addr;
  mhr.src_pan_id = MAC_PIB().mac_pan_id;
  mhr.src_addr.addr_short = data_req_params->src_addr;
#else
  mhr.dst_pan_id = data_req_params->dst_pan_id;
  ZB_MEMCPY(&mhr.dst_addr, &data_req_params->dst_addr, sizeof(zb_addr_u));
  mhr.src_pan_id = MAC_PIB().mac_pan_id;
  /* 25.08.2016 [DT] alignment of addr_short in zb_addr_u and zb_addr_time_u is different
   *                 (when compiling for Telink, at least) */
  if (data_req_params->src_addr_mode == ZB_ADDR_16BIT_DEV_OR_BROADCAST)
  {
    mhr.src_addr.addr_short = data_req_params->src_addr.addr_short;

#ifdef ZB_CERTIFICATION_HACKS
    {
      zb_nwk_hdr_t *nwhdr = zb_buf_begin(data_req);

      if ((ZB_CERT_HACKS().nwk_leave_from_unknown_addr == ZB_TRUE) &&
          (nwhdr->src_addr == ZB_CERT_HACKS().nwk_leave_from_unknown_short_addr))
      {
        mhr.src_addr.addr_short = ZB_CERT_HACKS().nwk_leave_from_unknown_short_addr;
      }
    }
#endif
  }
  else
  {
    ZB_MEMCPY(mhr.src_addr.addr_long, &data_req_params->src_addr.addr_long, sizeof(zb_ieee_addr_t));

#ifdef ZB_CERTIFICATION_HACKS
    {
      zb_nwk_hdr_t *nwhdr = zb_buf_begin(data_req);

      if(ZB_CERT_HACKS().nwk_leave_from_unknown_addr == ZB_TRUE
      && ZB_IEEE_ADDR_CMP(&nwhdr->src_ieee_addr, ZB_CERT_HACKS().nwk_leave_from_unknown_ieee_addr))
      {
        ZB_IEEE_ADDR_COPY(mhr.src_addr.addr_long, &ZB_CERT_HACKS().nwk_leave_from_unknown_ieee_addr);
      }
    }
#endif
  }
#endif

  /* get pointer to put header there */
  ptr = zb_buf_alloc_left(data_req, data_req_params->mhr_len);
  ZB_ASSERT(ptr);
  zb_mac_fill_mhr(ptr, &mhr);

#ifdef ZB_MAC_SECURITY
  if (data_req_params->security_level)
  {
    /* fill Aux security header */
    ptr += (data_req_params->mhr_len - MAC_SECUR_LEV5_KEYID1_AUX_HDR_SIZE);
    /* security control: always level 5, key id mode 1 */
    *ptr = ZB_MAC_SECURITY_LEVEL | (ZB_MAC_KEY_ID_MODE << 3);
    ptr++;
    /* frame counter */
    ZB_HTOLE32(ptr, &MAC_PIB().mac_frame_counter);
    MAC_PIB().mac_frame_counter++;
    ptr += 4;
    /* key identifier */
    *ptr = data_req_params->key_index;
  }
#endif

#if defined ZB_MAC_TESTING_MODE
  if (MAC_CTX().cert_hacks.security_enabled == 1)
  {
    zb_uint32_t frame_counter = MAC_SECUR_CERT_FRAME_COUNTER;

    /* fill Aux security header */
    ptr += (data_req_params->mhr_len - MAC_SECUR_CERT_AUX_HDR_SIZE);
    /* frame counter */
    ZB_HTOLE32(ptr, &frame_counter);
    ptr += sizeof(zb_uint32_t);
    /* key identifier */
    *ptr = MAC_SECUR_CERT_KEY_SEQ_COUNTER;
  }
#endif

  TRACE_MSG(TRACE_MAC3, "<< zb_mcps_data_request_fill_hdr ret %d", (FMT__D, ret));
  return ret;
}

/*
  return RET_OK, RET_ERROR, RET_PENDING
  RET_PENDING is returned if indirect transmission is used and data frame is put to pending queue
 */
static zb_ret_t zb_mcps_place_data_request(zb_bufid_t data_req)
{
  zb_ret_t ret = RET_OK;
#ifdef ZB_ROUTER_ROLE
  zb_mcps_data_req_params_t *data_req_params = ZB_BUF_GET_PARAM(data_req, zb_mcps_data_req_params_t);

  if (ZB_BIT_IS_SET(data_req_params->tx_options, MAC_TX_OPTION_INDIRECT_TRANSMISSION_BIT))
  {
    /* put data request to pending queue */
    zb_mac_pending_data_t pend_data;

    pend_data.pending_param = data_req;
#ifndef ZB_MAC_EXT_DATA_REQ
    pend_data.dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
    pend_data.dst_addr.addr_short = data_req_params->dst_addr;
#else
    pend_data.dst_addr_mode = data_req_params->dst_addr_mode;
    ZB_MEMCPY(&pend_data.dst_addr, &data_req_params->dst_addr, sizeof(zb_addr_u));
#endif

    /* returns RET_PENDING on success */
    ret = zb_mac_put_data_to_pending_queue(&pend_data);
  }
#else
  ZVUNUSED(data_req);
#endif
  return ret;
}

/**
  7.1.1.1 MCPS-DATA.request

  Just call zb_handle_mcps_data_req via tx q
*/
void zb_mcps_data_request(zb_uint8_t param)
{
  zb_mcps_data_req_params_t *data_req_params = ZB_BUF_GET_PARAM(param, zb_mcps_data_req_params_t);

  TRACE_MSG( TRACE_MAC3, ">>zb_mcps_data_request param %hd", (FMT__H, param));

#if defined ZB_MAC_API_TRACE_PRIMITIVES
  zb_mac_api_trace_data_request(param);
#endif

  ZB_ASSERT(param);

#ifdef ZB_PROMISCUOUS_MODE
  /* If devies put in promiscuous mode - reject data request and pass up confirm with fail status */
  /* Do not allow sniffer to send packets */
  if (MAC_PIB().mac_promiscuous_mode != 0U)
  {
    zb_mac_send_data_conf(param, MAC_DENIED, ZB_FALSE);
    return;
  }
  else
#endif
  {
    /* fill MAC header now to not confuse APS in case of confirm call */
    (void)zb_mcps_data_request_fill_hdr(param);
    /* If case of indirect tx do not need to wait for tx complete before put
     * into indirect q. */
    if (ZB_BIT_IS_SET(data_req_params->tx_options, MAC_TX_OPTION_INDIRECT_TRANSMISSION_BIT))
    {
      TRACE_MSG(TRACE_MAC1, "indirect tx param %hd", (FMT__H, param));
      zb_handle_mcps_data_req(param);
    }
    else
    {
      zb_bool_t success = ZB_FALSE;

      TRACE_MSG(TRACE_MAC3, "Direct tx. tx_wait_cb %p(cb arg %hd), put data req param %hd to tx q",
                (FMT__P_H_H, MAC_CTX().tx_wait_cb, MAC_CTX().tx_wait_cb_arg, param));
      /* TODO: recheck: is it still necessary? */
      /* [MM]: Hacky hack especially for TP-NWK-BV-37:
       * Prevent using TX Q while ED scan is in progress.
       *
       * The fix is very tricky and should be used ONLY in certification
       * branch. The fix SHOULDN'T be merged in any branches including
       * trunk.
       *
       * Correct proposal fix: Use MLME-SCAN.request as an atomic unit
       * in TX Q. Once MLME-SCAN.request has been called, TX queue
       * should be locked until scan is finished.
       */
      TRACE_MSG(TRACE_MAC1, "Scan in progress %hd", (FMT__H, MAC_CTX().flags.mlme_scan_in_progress));

      if (MAC_CTX().flags.mlme_scan_in_progress == 0U)
      {
        success = (ZB_SCHEDULE_TX_CB(zb_handle_mcps_data_req, param) == RET_OK);
      }

      if (!success)
      {
        TRACE_MSG(TRACE_MAC1, "max cb q overflow - pass up confirm", (FMT__0));
        zb_mac_send_data_conf(param, MAC_TRANSACTION_OVERFLOW, ZB_TRUE);
      }
    }
  }
  TRACE_MSG(TRACE_MAC3, "<<zb_mcps_data_request ", (FMT__0));
}

#ifndef ZB_MACSPLIT_DEVICE
zb_ret_t zb_mac_check_security(zb_bufid_t data_buf)
{
  zb_ret_t ret = RET_OK;
  zb_uint_t encr_flags = (zb_buf_flags_get(data_buf) & ZB_BUF_SECUR_ALL_ENCR);

  TRACE_MSG(TRACE_MAC2, ">> zb_mac_check_security data_buf %hd buf flags %hx", (FMT__H_H, data_buf, encr_flags));

  if (encr_flags != 0U)
  {
    void *mac_hdr = zb_buf_begin(data_buf);
    zb_ushort_t hlen = zb_mac_calculate_mhr_length(
      ZB_FCF_GET_SRC_ADDRESSING_MODE(mac_hdr),
      ZB_FCF_GET_DST_ADDRESSING_MODE(mac_hdr),
      ZB_U2B(ZB_FCF_GET_PANID_COMPRESSION_BIT(mac_hdr)));

    TRACE_MSG(TRACE_MAC2, "security is on", (FMT__0));

    /*
     * Encrypt nwk payload now, send encrypted buffer, send confirm by
     * unencrypted buffer (!).
     * It is necessary to handle APS acks and retransmissions: we need
     * unencrypted payload.
     */
    TRACE_MSG(TRACE_SECUR3, "encrypt_type %hd operation_buf %hd encryption_buf %hd", (FMT__H_H_H, encr_flags, MAC_CTX().operation_buf, SEC_CTX().encryption_buf));
#if !defined(ZB_MAC_TESTING_MODE) || defined(APS_FRAME_SECURITY)
    if ((encr_flags & (ZB_BUF_SECUR_APS_ENCR | ZB_BUF_SECUR_NWK_ENCR))  /*APS & NWK  */
        == (ZB_BUF_SECUR_APS_ENCR | ZB_BUF_SECUR_NWK_ENCR))
    {
      /* sure MAC_CTX().operation_buf is not in use now */
      ret = zb_aps_secure_frame(data_buf, hlen, MAC_CTX().operation_buf, ZB_FALSE);
      if(ret==RET_OK)
      {
        ret = zb_nwk_secure_frame(MAC_CTX().operation_buf, hlen, SEC_CTX().encryption_buf);
      }
    }
    else if (ZB_BIT_IS_SET(encr_flags, ZB_BUF_SECUR_APS_ENCR)) /* APS only */
    {
      ret = zb_aps_secure_frame(data_buf, hlen, SEC_CTX().encryption_buf, ZB_FALSE);
    }
    else
#endif
    if (ZB_BIT_IS_SET(encr_flags, ZB_BUF_SECUR_NWK_ENCR)) /* NWK only */
    {
      ret = zb_nwk_secure_frame(data_buf, hlen, SEC_CTX().encryption_buf);
    }
#ifdef ZB_MAC_SECURITY
    else if (encr_flags & ZB_BUF_SECUR_MAC_ENCR)
    {
      ret = zb_mac_secure_frame(data_buf, hlen, SEC_CTX().encryption_buf);
    }
#endif
    else
    {
      ZB_ASSERT(ZB_FALSE);
      ret = RET_INVALID_PARAMETER;
    }
  }

  TRACE_MSG(TRACE_MAC2, "<< zb_mac_check_security ret %d", (FMT__D, ret));
  return ret;
}
#endif  /* ZB_MACSPLIT_DEVICE */


/**
   handle mac data request, caller side

   That routine is called via tx q
 */
void zb_handle_mcps_data_req(zb_uint8_t param)
{
  zb_mcps_data_req_params_t *data_req_params;

  TRACE_MSG(TRACE_MAC3, ">> zb_handle_mcps_data_req param %hd", (FMT__H, param));

  //PRINT_BUF_PAYLOAD_BY_REF(param);

  data_req_params = ZB_BUF_GET_PARAM(param, zb_mcps_data_req_params_t);
  ZB_ASSERT(data_req_params);

  TRACE_MSG(TRACE_MAC3, "msdu_handle %hd tx_options %hd", (FMT__H_H, data_req_params->msdu_handle, data_req_params->tx_options));

  ZB_SET_MAC_STATUS(MAC_SUCCESS);

#ifdef ZB_ENABLE_ZGP_DIRECT
  if (data_req_params->tx_options & MAC_TX_OPTION_NO_CSMA_CA)
  {
    MAC_CTX().tx_wait_cb = zb_direct_mcps_data_req_tx_done;
    MAC_CTX().tx_wait_cb_arg = param;
    zb_mac_send_zgpd_frame(param);
    TRACE_MSG(TRACE_MAC3, "<< zb_handle_mcps_data_req RET_OK", (FMT__0));
  }
  else
#endif
  {
    /* this call may put data to pending queue if this is indirect transfer and
     * return RET_PENDING. We do nothing until return here in such case. */
    zb_ret_t ret = zb_mcps_place_data_request(param);

    /* Special case to turn on MAC security */
#ifdef ZB_MAC_TESTING_MODE
    if ( ret == RET_OK
         && data_req_params->msdu_handle == 0xFA )
    {
      ZB_FCF_SET_SECURITY_BIT(zb_buf_begin(param), 1);
      TRACE_MSG(TRACE_MAC1, "set security bit!", (FMT__0));
    }
#endif

#ifndef ZB_MACSPLIT_DEVICE
    if (ret == RET_OK)
    {
      ret = zb_mac_check_security(param);
      if (ret != RET_OK)
      {
        TRACE_MSG(TRACE_ERROR, "zb_mac_check_security() failed with ret = %hd", (FMT__H, ret));
        ZB_SET_MAC_STATUS(MAC_SECURITY_ERROR);
      }
    }
#endif /* ZB_MACSPLIT_DEVICE */

    if (ret == RET_OK)
    {
      TRACE_MSG(TRACE_MAC3, "before reassign tx_wait_cb %p arg %hd",
                (FMT__P_H, MAC_CTX().tx_wait_cb, MAC_CTX().tx_wait_cb_arg));

      MAC_CTX().tx_wait_cb = zb_direct_mcps_data_req_tx_done;
      MAC_CTX().tx_wait_cb_arg = param;

      /* zb_handle_mcps_data_req called indirectly via tx cb, so it is ok to
       * call zb_mac_send_frame from here */
      if (ZB_BIT_IS_SET(zb_buf_flags_get(param), ZB_BUF_SECUR_ALL_ENCR))
      {
        TRACE_MSG(TRACE_MAC3, "zb_mac_send_frame SEC_CTX().encryption_buf param %hd",
                  (FMT__H, SEC_CTX().encryption_buf));
        /* data_req_params->mhr_len is filled by zb_mcps_data_request_fill_hdr */
        zb_mac_send_frame(SEC_CTX().encryption_buf, data_req_params->mhr_len);
      }
      else
      {
        zb_mac_send_frame(param, data_req_params->mhr_len);
      }
    }
    else if (ret == RET_PENDING)
    {
      /* Do nothing for RET_PEDNING error code. The packet is put to pending queue. */
      TRACE_MSG(TRACE_MAC3, "RET_PENDING for param %hd", (FMT__H, param));
    }
    else
    {
      ZB_ASSERT(ZB_GET_MAC_STATUS() != MAC_SUCCESS);
      zb_direct_mcps_data_req_tx_done(param);
    }

  TRACE_MSG(TRACE_MAC3, "<< zb_handle_mcps_data_req %i", (FMT__D, ret));
  }
}


/**
   Callback called when direct data transmission is done
 */
void zb_direct_mcps_data_req_tx_done(zb_uint8_t param)
{
  TRACE_MSG(TRACE_MAC2, "zb_direct_mcps_data_req_tx_done", (FMT__0));
  zb_mac_send_data_conf(param, ZB_GET_MAC_STATUS(), ZB_TRUE);
}


void zb_mac_send_data_conf(zb_bufid_t param, zb_mac_status_t status, zb_bool_t cut_mhr)
{
  zb_mcps_data_req_params_t data_req_params = *ZB_BUF_GET_PARAM(param, zb_mcps_data_req_params_t);
  zb_mcps_data_confirm_params_t *confirm_params;

  TRACE_MSG(TRACE_MAC3, "zb_mac_send_data_conf param %hu status %hu cut_mhr %hd", (FMT__H_H_H, param, status, cut_mhr));

  if (cut_mhr)
  {
    (void)zb_buf_cut_left(param, data_req_params.mhr_len);
  }
  confirm_params = ZB_BUF_GET_PARAM(param, zb_mcps_data_confirm_params_t);
  confirm_params->msdu_handle = data_req_params.msdu_handle;
  confirm_params->status = status;
  zb_buf_set_status(param, (zb_ret_t)status);
  /* Copy different unions.  */
  ZB_MEMCPY(&confirm_params->src_addr, &data_req_params.src_addr, sizeof(data_req_params.src_addr));
  confirm_params->dst_addr = data_req_params.dst_addr;
  confirm_params->dst_pan_id = data_req_params.dst_pan_id;
  confirm_params->src_addr_mode = data_req_params.src_addr_mode;
  confirm_params->dst_addr_mode = data_req_params.dst_addr_mode;
  /* Isolate only standard TX options */
  confirm_params->tx_options = (data_req_params.tx_options & 0x0FU);
#ifdef ZB_ENABLE_NWK_RETRANSMIT
  confirm_params->nwk_retry_cnt = data_req_params.nwk_retry_cnt;
#endif
#if defined ZB_MAC_TESTING_MODE
  confirm_params->timestamp = MAC_CTX().tx_timestamp;
#endif

#if defined ZB_MAC_API_TRACE_PRIMITIVES
  zb_mac_api_trace_data_confirm(param);
#endif

  ZB_SCHEDULE_CALLBACK(zb_mcps_data_confirm, param);
}


void zb_handle_data_frame(zb_uint8_t param)
{
  TRACE_MSG(TRACE_MAC3, ">> zb_handle_data_frame %hd", (FMT__H, param));
  ZB_ASSERT(zb_buf_len(param) > 0U);

#ifdef ZB_MAC_SECURITY
  {
    zb_uint8_t *fc = zb_buf_begin(param);
    if (ZB_FCF_GET_SECURITY_BIT(fc) != 0U)
    {
      zb_ret_t ret = zb_mac_unsecure_frame(param);
      if (ret != RET_OK)
      {
        TRACE_MSG(TRACE_ERROR, "zb_mac_unsecure_frame() failed ret %d!", (FMT__D, ret));
        zb_buf_free(param);
        param = 0;
      }
    }
  }
#endif

  if (param != 0u)
  {
#if defined ZB_MAC_API_TRACE_PRIMITIVES
    zb_mac_api_trace_data_indication(param);
#endif

    ZB_SCHEDULE_CALLBACK(zb_mcps_data_indication, param);
  }

  TRACE_MSG(TRACE_MAC3, "<< zb_handle_data_frame", (FMT__0));
}

/* Note: not ZB_ED_FUNC because of associate */
#ifndef ZB_COORDINATOR_ONLY
void zb_mac_indirect_data_rx_failed(zb_uint8_t status)
{
  TRACE_MSG(TRACE_MAC1, ">>zb_mac_indirect_data_rx_failed status %hd", (FMT__H, status));

  /* In case if indiect rx buffer used for data request send is always in pending_buf
    MA: No. If process received data after process ack then pending_buf==0 and confirm
     were call early */

  if( MAC_CTX().pending_buf != 0U)
  {
    (void)ZB_SCHEDULE_TX_CB(zb_mac_sync_rx_on_off_with_pib, 0);

#if 0                           /* code from r21 */
    if (!(MAC_CTX().flags.tx_q_busy || MAC_CTX().flags.tx_radio_busy))
    {
      ZB_TRANSCEIVER_SET_RX_ON_OFF(ZB_PIB_RX_ON_WHEN_IDLE());
    }
#endif

    ZB_SCHEDULE_ALARM_CANCEL(zb_mac_indirect_data_rx_failed, ZB_ALARM_ANY_PARAM);

    if (MAC_CTX().flags.ass_state == ZB_MAC_ASS_STATE_POLLING)
    {
      MAC_CTX().flags.ass_state = ZB_MAC_ASS_STATE_POLL_FAILED;
      TRACE_MSG(TRACE_ERROR, "polling during association error: %hd", (FMT__H, status));
      mac_call_associate_confirm_fail(MAC_CTX().pending_buf, (zb_mac_status_t)status);
    }
#ifdef ZB_ED_FUNC
    else if (MAC_CTX().flags.poll_inprogress)
    {
      MAC_CTX().flags.poll_inprogress = ZB_FALSE;
      zb_buf_set_status(MAC_CTX().pending_buf, (zb_ret_t)status);
      TRACE_MSG(TRACE_MAC2, "Calling poll confirm, status %hd", (FMT__H, status));
#if defined ZB_MAC_API_TRACE_PRIMITIVES
      zb_mac_api_trace_poll_confirm(MAC_CTX().pending_buf);
#endif
      ZB_SCHEDULE_CALLBACK(zb_mlme_poll_confirm, MAC_CTX().pending_buf);
      MAC_CTX().pending_buf = 0;
      TRACE_MSG(TRACE_MAC3, "zero pending_buf", (FMT__0));
    }
#endif
    /*  ZB_ED_FUNC */
#if 0
    else
    {
      ZB_ASSERT(0);
    }
#endif
  }
  else
  {
    TRACE_MSG(TRACE_MAC3, "pending_buf was used early", (FMT__0));
  }

  TRACE_MSG(TRACE_MAC1, "<<zb_mac_indirect_data_rx_failed", (FMT__0));
}
#endif  /* ZB_ED_FUNC */


/*! @} */

#endif  /* !ZB_ALIEN_MAC && !ZB_MACSPLIT_HOST */
