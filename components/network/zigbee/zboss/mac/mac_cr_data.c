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
/* PURPOSE: Roitines specific mac data transfer for coordinator/router
*/

#define ZB_TRACE_FILE_ID 300
#include "zb_common.h"

#if !defined ZB_ALIEN_MAC && !defined ZB_MACSPLIT_HOST

#include "zb_scheduler.h"
#include "zb_nwk.h"
#include "zb_mac.h"
#include "zb_mac_globals.h"
#include "mac_internal.h"
#include "zb_mac_transport.h"
#include "zb_secur.h"
#ifndef ZB_MACSPLIT_DEVICE
#include "zb_zdo.h"
#endif
#include "mac_source_matching.h"



/*! \addtogroup ZB_MAC */
/*! @{ */

#if defined ZB_ROUTER_ROLE
static zb_uint8_t mac_fill_empty_frame(zb_mac_mhr_t *mhr_req, zb_bufid_t buf);

static void mac_confirm_pending_queue_item(zb_uint8_t pending_param, zb_mac_status_t mac_status);
static void mac_clear_pending_queue_slot(zb_uint_t num);


/*
  Handles data request command, coordinator side. Finds pending data
  for the device if any and sends it. spec 7.3.4 Data request
  command, 7.5.6.3 Extracting pending data from a coordinator.
  return RET_OK, RET_ERROR, RET_BLOCKED
*/
void zb_accept_data_request_cmd(zb_uint8_t param)
{
  zb_int8_t data_found_index;
  zb_uint8_t mhr_len = 0;
  zb_mac_mhr_t mhr;
  zb_bufid_t b4send = 0;
  zb_mac_mhr_t mhr_send;

  TRACE_MSG(TRACE_MAC3, ">> zb_accept_data_request_cmd %hd", (FMT__H, param));

  /*Situation if pending bit is found in MAC.CTX */
  (void)zb_parse_mhr(&mhr, param);
  data_found_index = mac_check_pending_data(&mhr, 0);

  TRACE_MSG(TRACE_MAC3, "data_found_index %hd", (FMT__H, data_found_index));
  ZB_SET_MAC_STATUS(MAC_SUCCESS);

  /* 18.08.2016 CR [DT] Start */
  /* Previous logic would not work with MAC only build (which always sets pending bit to 1.
   * Particularly, NSNG build wasn't able to send empty data frames.
   * Enable this logic for MAC certification build */
  if (data_found_index < 0)
  {
#if defined ZB_MAC_TESTING_MODE
    /* In r21 NWK must decide to send NULL frame. See zb_mcps_poll_indication. */
    /* if ack is sent automatically and it was sent with pending data
     * bit == 1, we must send data frame with zero payload.
     *
     * When we send manual ack, we set pending bit accordinly, so can silently
     * drop that data request.
     */
     /* Something is in the queue, so we set pending bit in our ACK, but we have
     * nothing to send for that device - send empty frame. */
    if (!mac_pending_queue_is_empty()
#if defined ZB_MAC_TESTING_MODE
        || MAC_CTX().flags.pending_bit
#endif
        )
    {
      /* FIXME: can MAC_CTX().operation_buf be busy?? Use buf and free it from
       * tc cb? Anyway, in r21 thia is NWK who sends empty frame. */
      mhr_len = mac_fill_empty_frame(&mhr, MAC_CTX().operation_buf);
      b4send = MAC_CTX().operation_buf;
      TRACE_MSG(TRACE_MAC3, "Sending empty frame", (FMT__0));
    }
  /* 18.08.2016 CR [DT] End */
#endif  /* 0 */
  }
  else
  {
    /* Do not send empty data frame from zb_mcps_poll_indication */
    zb_buf_set_status(param, MAC_NO_DATA);
    TRACE_MSG(TRACE_MAC3, "pending data i %hd param %hd",
              (FMT__H_H, data_found_index, MAC_CTX().pending_data_queue[data_found_index].pending_param));

    /* get MHR stored in pending_data, need to analyse its frame type*/
    mhr_len = zb_parse_mhr(&mhr_send, MAC_CTX().pending_data_queue[data_found_index].pending_param);
#ifndef ZB_MACSPLIT_DEVICE
    /* data and command frames are sent using the same call */
    if (RET_OK == zb_mac_check_security(MAC_CTX().pending_data_queue[data_found_index].pending_param))
#endif
    {
#ifndef ZB_MACSPLIT_DEVICE
      if (ZB_BIT_IS_SET(zb_buf_flags_get(MAC_CTX().pending_data_queue[data_found_index].pending_param),
                        ZB_BUF_SECUR_ALL_ENCR))

      {
        b4send = SEC_CTX().encryption_buf;
      }
      else
#endif
      {
        b4send = MAC_CTX().pending_data_queue[data_found_index].pending_param;
      }
    }
  }
  if (b4send != 0U)
  {
#ifdef ZB_MAC_TESTING_MODE
    if (MAC_CTX().cert_hacks.delay_frame_transmission)
    {
      osif_sleep_using_transc_timer(16000);
    }
#endif

    zb_mac_send_frame(b4send, mhr_len);
  }
  if (data_found_index >= 0)
  {
    MAC_CTX().tx_wait_cb = zb_handle_data_request_cmd_continue;
    MAC_CTX().tx_wait_cb_arg = MAC_CTX().pending_data_queue[data_found_index].pending_param;
  }
  if (b4send != param)
  {
    zb_mac_call_mcps_poll_indication(param, &mhr);
  }

  TRACE_MSG(TRACE_MAC2, "<< zb_accept_data_request_cmd", (FMT__0));
}

#if defined ZB_MAC_SOFTWARE_PB_MATCHING && defined ZB_MAC_POLL_INDICATION_CALLS_REDUCED
static zb_bool_t zb_mac_check_poll_indication_call_timeout(zb_uint16_t short_addr)
{
  zb_uint32_t ref;
  zb_bool_t   ret = ZB_TRUE; /* by default */

  if (mac_child_hash_table_search(short_addr, &ref, ZB_FALSE,
                                  SEARCH_ACTION_NO_ACTION))
  {
    if (ZB_TIME_SUBTRACT(ZB_TIMER_GET(), MAC_POLL_TIMESTAMP_GET(ref)) >= MAC_POLL_TIMEOUT_GET(ref))
    {
      /* Update the last poll timestamp and call mcps_poll_indication */
      MAC_POLL_TIMESTAMP_UPD(ref);
    }
    else
    {
      /* Don't call mcps_poll_indication yet */
      ret = ZB_FALSE;
    }
  }

  return ret;
}
#endif /* ZB_MAC_SOFTWARE_PB_MATCHING && ZB_MAC_POLL_INDICATION_CALLS_REDUCED */

void zb_mac_call_mcps_poll_indication(zb_uint8_t param, zb_mac_mhr_t *mhr)
{
  ZB_ASSERT(mhr);

  if (ZB_FCF_GET_SRC_ADDRESSING_MODE(mhr->frame_control) == ZB_ADDR_16BIT_DEV_OR_BROADCAST)
  {
#if defined ZB_MAC_SOFTWARE_PB_MATCHING && defined ZB_MAC_POLL_INDICATION_CALLS_REDUCED
    /* This is optimization for MAC split working with number of noisy
     * devices which constantly poll every second or faster. If we
     * just called NWK, let's drop that poll and don't waste MCU-Host
     * channel. */
    if (!zb_mac_check_poll_indication_call_timeout(mhr->src_addr.addr_short))
    {
      /* Drop that Data Request */
      zb_buf_free(param);
    }
    else
#endif /* ZB_MAC_SOFTWARE_PB_MATCHING && ZB_MAC_POLL_INDICATION_CALLS_REDUCED */
    {
      /* Call mcps_poll_indication */
      ZB_SCHEDULE_CALLBACK(zb_mcps_poll_indication, param);
    }
  }
  else
  {
    /* This is poll during association, so sure NWK can't know about that
     * device, so do not pass this poll up. */
    zb_buf_free(param);
  }
}

static void zb_empty_frame_tx_done(zb_uint8_t param);

static void zb_mac_resp_by_empty_frame_cb(zb_uint8_t param)
{
  zb_mac_mhr_t mhr;
  zb_uint8_t mhr_len;

  (void)zb_parse_mhr(&mhr, param);
  mhr_len = mac_fill_empty_frame(&mhr, param);

  MAC_CTX().tx_wait_cb = zb_empty_frame_tx_done;
  MAC_CTX().tx_wait_cb_arg = param;
  zb_mac_send_frame(param, mhr_len);

  /* Free buffer after sending. It's safe, because sending is synchronous and no
   * retransmits are expected */
  /* Oops - got crash because of removing FCS in zb_nsng_io_iteration((). Must free from the cb when got TX status from the transceiver. */
}


static void zb_empty_frame_tx_done(zb_uint8_t param)
{
  zb_buf_free(param);
}



void zb_mac_resp_by_empty_frame(zb_uint8_t param)
{
  if (ZB_SCHEDULE_TX_CB_WITH_HIGH_PRIORITY(zb_mac_resp_by_empty_frame_cb, param) != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "Oops! No place for zb_mac_resp_by_empty_frame_cb in tx queue", (FMT__0));
    zb_buf_free(param);
  }
}

zb_int8_t mac_check_pending_data(const zb_mac_mhr_t *mhr, zb_uint8_t index)
{
  zb_uindex_t i;
  zb_bool_t data_found = ZB_FALSE;
  zb_uint8_t src_addr_mode;

  src_addr_mode = ZB_FCF_GET_SRC_ADDRESSING_MODE(mhr->frame_control);

  TRACE_MSG(TRACE_MAC3, "mac_check_pending_data: src_addr_mode %hd", (FMT__H, src_addr_mode));

  for (i = index; i < ZB_MAC_PENDING_QUEUE_SIZE; i++)
  {
    TRACE_MSG(TRACE_MAC3, "i %hd pending_data %hd dst_addr_mode %hd",
              (FMT__H_H_H, i, MAC_CTX().pending_data_queue[i].pending_param,
               MAC_CTX().pending_data_queue[i].dst_addr_mode));

    if (MAC_CTX().pending_data_queue[i].pending_param != 0U)
    {
      if (src_addr_mode == ZB_ADDR_64BIT_DEV
          && src_addr_mode == MAC_CTX().pending_data_queue[i].dst_addr_mode)
      {
        data_found = ZB_IEEE_ADDR_CMP(MAC_CTX().pending_data_queue[i].dst_addr.addr_long,
                                      mhr->src_addr.addr_long);
        TRACE_MSG(TRACE_MAC3, "data_found %hd pending dst " TRACE_FORMAT_64 " src " TRACE_FORMAT_64,
                  (FMT__H_A_A, data_found,
                   TRACE_ARG_64(MAC_CTX().pending_data_queue[i].dst_addr.addr_long),
                   TRACE_ARG_64(mhr->src_addr.addr_long)));
      }
      else if (src_addr_mode == ZB_ADDR_16BIT_DEV_OR_BROADCAST
               && src_addr_mode == MAC_CTX().pending_data_queue[i].dst_addr_mode)
      {
        data_found = (mhr->src_addr.addr_short == MAC_CTX().pending_data_queue[i].dst_addr.addr_short);
        TRACE_MSG(TRACE_MAC3, "data_found %hd ", (FMT__H, data_found));
      }
      /* That code is required for buggy external MAC only. Put it under ifdef: exclude address call from MAC. */
#ifdef BUGGY_ALIEN_MAC_PKT_ADDRESSING
      else if (src_addr_mode == ZB_ADDR_64BIT_DEV
               && MAC_CTX().pending_data_queue[i].dst_addr_mode == ZB_ADDR_16BIT_DEV_OR_BROADCAST)
      {
        zb_uint16_t short_src;
        zb_address_ieee_ref_t ref;

        if (zb_address_by_ieee(mhr->src_addr.addr_long, ZB_FALSE, ZB_FALSE, &ref) == RET_OK)
        {
          zb_address_short_by_ref(&short_src, ref);
          data_found = (short_src == MAC_CTX().pending_data_queue[i].dst_addr.addr_short);
        }
        TRACE_MSG(TRACE_MAC3, "data_found %hd ", (FMT__H, data_found));
      }
#endif
      else
      {
        /* MISRA rule 15.7 requires empty 'else' branch. */
      }
    }
    if (data_found)
    {
      break;
    }
  }
  return data_found ? (zb_int8_t)i : -1;
}

/*
  Fills frame header with zero payload length. mhr_req contains mac
  header of the incoming request.
*/
static zb_uint8_t mac_fill_empty_frame(zb_mac_mhr_t *mhr_req, zb_bufid_t buf)
{
  zb_mac_mhr_t mhr;
  zb_uint_t mhr_len;
  zb_uint8_t *ptr;
  zb_addr_u dst_addr;
#ifdef BUGGY_ALIEN_MAC_PKT_ADDRESSING
  zb_address_ieee_ref_t ref;
#endif
  zb_uint8_t dst_addr_mode;

  TRACE_MSG(TRACE_MAC3, ">> fill_empty_frame mhr_req %p, buf %hu", (FMT__P_H, mhr_req, buf));

#ifdef BUGGY_ALIEN_MAC_PKT_ADDRESSING
  /* Put that complex logic under define: it is for buggy extern MAC only. Normally we send empty frame to the short address. */
  if (ZB_FCF_GET_SRC_ADDRESSING_MODE(mhr_req->frame_control) == ZB_ADDR_64BIT_DEV
      && ZB_FCF_GET_DST_ADDRESSING_MODE(mhr_req->frame_control) == ZB_ADDR_16BIT_DEV_OR_BROADCAST
      && zb_address_by_ieee(mhr_req->src_addr.addr_long, ZB_FALSE, ZB_FALSE, &ref) == RET_OK)
  {
    dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
    zb_address_short_by_ref((zb_uint16_t *) &dst_addr.addr_short, ref);
  }
  else
#endif
  {
    dst_addr_mode = ZB_FCF_GET_SRC_ADDRESSING_MODE(mhr_req->frame_control);
    ZB_MEMCPY(&dst_addr, &mhr_req->src_addr, sizeof(zb_addr_u));
  }

  mhr_len = zb_mac_calculate_mhr_length(ZB_FCF_GET_DST_ADDRESSING_MODE(mhr_req->frame_control),
                                        dst_addr_mode, ZB_TRUE);
  /* packet length == mhr_len */
  ptr = zb_buf_initial_alloc(buf, mhr_len);
  ZB_ASSERT(ptr);

  ZB_BZERO(ptr, mhr_len);
/* Fill Frame Controll then call zb_mac_fill_mhr()
   mac spec  7.2.1.1 Frame Control field
   | Frame Type | Security En | Frame Pending | Ack.Request | PAN ID Compres | Reserv | Dest.Addr.Mode | Frame Ver | Src.Addr.gMode |
*/
  ZB_BZERO2(mhr.frame_control);
  ZB_FCF_SET_FRAME_TYPE(mhr.frame_control, MAC_FRAME_DATA);
  /* security enable is 0 */
  /* frame pending is 0 */
  /* ack request is 0 */
  ZB_FCF_SET_PANID_COMPRESSION_BIT(mhr.frame_control, 1U);
  ZB_FCF_SET_DST_ADDRESSING_MODE(mhr.frame_control, dst_addr_mode);
  ZB_FCF_SET_FRAME_VERSION(mhr.frame_control, MAC_FRAME_VERSION);
  ZB_FCF_SET_SRC_ADDRESSING_MODE(mhr.frame_control, ZB_FCF_GET_DST_ADDRESSING_MODE(mhr_req->frame_control));

  /* mac spec 7.5.6.1 Transmission */
  mhr.seq_number = ZB_MAC_DSN();
  ZB_INC_MAC_DSN();

  mhr.dst_pan_id = MAC_PIB().mac_pan_id;
  ZB_MEMCPY(&mhr.dst_addr, &dst_addr, sizeof(zb_addr_u));
  mhr.src_pan_id = 0;
  ZB_MEMCPY(&mhr.src_addr, &mhr_req->dst_addr, sizeof(zb_addr_u));
  zb_mac_fill_mhr(ptr, &mhr);

  TRACE_MSG(TRACE_MAC3, "<< fill_empty_frame, ret %i", (FMT__D, mhr_len));

  ZB_ASSERT(mhr_len <= ZB_UINT8_MAX);
  return (zb_uint8_t)mhr_len;
}


/**
   Callback called after indirect data is sent.

   If we just sent ass resp, call mac command status.
   If we just sent data, call data confirm.
   Free queue slot.
 */

void zb_handle_data_request_cmd_continue(zb_uint8_t pending_param)
{
  zb_uint8_t *fcf;
  zb_uint8_t mhr_pend_len;
  zb_mac_mhr_t mhr_pend;
  zb_uint8_t data_found_index;

  TRACE_MSG(TRACE_MAC2, ">> zb_handle_data_request_cmd_continue", (FMT__0));

  for (data_found_index = 0; data_found_index < ZB_MAC_PENDING_QUEUE_SIZE; data_found_index++)
  {
    if (MAC_CTX().pending_data_queue[data_found_index].pending_param == pending_param)
    {
      break;
    }
  }

  if (ZB_GET_MAC_STATUS() != MAC_SUCCESS)
  {
    if (data_found_index < ZB_MAC_PENDING_QUEUE_SIZE)
    {
      /* We are not retransmitting witin indirect tx, so here we are just waiting next DR

        802.15.4-2015, 6.7.4.3 Retransmissions:
        If a single transmission attempt has failed and the transmission was indirect, the coordinator shall not
        retransmit the frame. Instead, the frame shall remain in the transaction queue of the coordinator and can only
        be extracted following the reception of a new Data Request command. If a new Data Request command is
        received, the originating device shall transmit the frame using the same DSN as was used in the original
        transmission.
      */
      TRACE_MSG(TRACE_MAC2, "single attempt to send data to sleepy ED failed. Retry at next data request from it", (FMT__0));
      ZB_SET_MAC_STATUS(MAC_SUCCESS);
    }
    else
    {
      /* If the pending table record was not found, it indicates that it is expired */
      /* We need to set up a confirm. */
/* 10/10/2018 EE CR:MINOR Is it really EXPIRED, or NO_ACK? No big difference for higher levels, but...  */
      mac_confirm_pending_queue_item(pending_param, MAC_TRANSACTION_EXPIRED);
    }
  }
  else
  {
    /* If the transmission attempt succeeds, we do not care whether it fit exactly within
       the time limit. */

    fcf = zb_buf_begin(pending_param);
    mhr_pend_len = zb_parse_mhr(&mhr_pend, pending_param);

    TRACE_MSG(TRACE_MAC2, "frame type %i data_found_index %hd", (FMT__D_H, ZB_FCF_GET_FRAME_TYPE(fcf), data_found_index));
    if (ZB_FCF_GET_FRAME_TYPE(fcf) == MAC_FRAME_COMMAND)
    {
      /* Warning: If you add some more functions, that need comm_status */
      /* Make sure, that there will be no races and context parameters are safe! */
      if (fcf[mhr_pend_len] == MAC_CMD_ASSOCIATION_RESPONSE)
      {
        TRACE_MSG(TRACE_MAC3, "Sent ass resp. Calling zb_mac_send_comm_status status %hd", (FMT__H, ZB_GET_MAC_STATUS()));
        (void)zb_mac_call_comm_status_ind(pending_param,
                                    ZB_GET_MAC_STATUS(),
                                    pending_param);
      }
      else
      {
        /* We can send indirectly only one command: ass resp */
        ZB_ASSERT(0);
      }
    }
    else
    {
      zb_mac_send_data_conf(pending_param, ZB_GET_MAC_STATUS(), ZB_TRUE);
    }

    if (data_found_index < ZB_MAC_PENDING_QUEUE_SIZE)
    {
      mac_clear_pending_queue_slot(data_found_index);
    }
    ZB_SCHEDULE_ALARM_CANCEL(zb_mac_pending_data_timeout, pending_param);
  }

  TRACE_MSG(TRACE_MAC2, "<< zb_handle_data_request_cmd_continue", (FMT__0));
}


/* sends MLME-COMM-STATUS.indication to higher level. Parameters for
 * indication are taken from panding_buf.mhr */
zb_ret_t zb_mac_call_comm_status_ind(zb_bufid_t pending_buf, zb_mac_status_t mac_status, zb_bufid_t buffer)
{
  zb_mac_mhr_t mhr_pend;
  zb_mlme_comm_status_indication_t *ind_params;

  TRACE_MSG(TRACE_MAC2, ">> zb_mac_send_comm_status pend %p, status %hi, buf %p", (FMT__P_H_P,
                                                                                   pending_buf, mac_status, buffer));

  (void)zb_parse_mhr(&mhr_pend, pending_buf);

  ind_params = ZB_BUF_GET_PARAM(buffer, zb_mlme_comm_status_indication_t);
  ind_params->status = mac_status;
  zb_buf_set_status(buffer, mac_status);

  ZB_MEMCPY(&ind_params->src_addr, &mhr_pend.src_addr, sizeof(zb_addr_u));
  ind_params->src_addr_mode = ZB_FCF_GET_SRC_ADDRESSING_MODE(&mhr_pend.frame_control);
  ZB_MEMCPY(&ind_params->dst_addr, &mhr_pend.dst_addr, sizeof(zb_addr_u));
  ind_params->dst_addr_mode = ZB_FCF_GET_DST_ADDRESSING_MODE(&mhr_pend.frame_control);
  ind_params->pan_id = mhr_pend.dst_pan_id;

#if defined ZB_MAC_API_TRACE_PRIMITIVES
  zb_mac_api_trace_comm_status_indication(buffer);
#endif

  ZB_SCHEDULE_CALLBACK(zb_mlme_comm_status_indication, buffer);
  TRACE_MSG(TRACE_MAC2, "<< zb_mac_send_comm_status pend RET_OK", (FMT__0));
  return RET_OK;
}


#ifdef ZB_MAC_TESTING_MODE
void zb_mlme_purge_request(zb_uint8_t param)
{
  zb_ushort_t i;
  zb_mlme_purge_request_t *req = ZB_BUF_GET_PARAM(param, zb_mlme_purge_request_t);
  zb_mcps_data_req_params_t *req_params;

  TRACE_MSG(TRACE_MAC2, ">> zb_mlme_purge_request %hd handle 0x%hx",
            (FMT__H_H, param, req->msdu_handle));
#if defined ZB_MAC_API_TRACE_PRIMITIVES
  zb_mac_api_trace_purge_request(param);
#endif

  for (i = 0; i < ZB_MAC_PENDING_QUEUE_SIZE; i++)
  {
    TRACE_MSG(TRACE_MAC2, "i %hd pending_data %hd ",
              (FMT__H_P, i, MAC_CTX().pending_data_queue[i].pending_param));

    req_params = ZB_BUF_GET_PARAM(MAC_CTX().pending_data_queue[i].pending_param, zb_mcps_data_req_params_t);

    if ((MAC_CTX().pending_data_queue[i].pending_param != 0) &&
        (req_params->msdu_handle == req->msdu_handle))
    {
      break;
    }
  }

  if (i == ZB_MAC_PENDING_QUEUE_SIZE)
  {
    TRACE_MSG(TRACE_MAC1, "Purge: handle 0x%hx not found", (FMT__H, req->msdu_handle));
    zb_buf_set_status(param, MAC_INVALID_HANDLE);
  }
  else
  {
    zb_buf_set_status(param, MAC_SUCCESS);
    ZB_SCHEDULE_ALARM_CANCEL(zb_mac_pending_data_timeout, MAC_CTX().pending_data_queue[i].pending_param);

    mac_clear_pending_queue_slot(i);
#ifndef ZB_MAC_STICKY_PENDING_BIT
    if (mac_pending_queue_is_empty())
    {
      TRACE_MSG(TRACE_MAC1, "Purge: Clearing pending bit", (FMT__0));
      ZB_MAC_TRANS_CLEAR_PENDING_BIT();
    }
#endif
  }
#if defined ZB_MAC_API_TRACE_PRIMITIVES
  zb_mac_api_trace_purge_confirm(param);
#endif

  ZB_SCHEDULE_CALLBACK(zb_mlme_purge_confirm, param);

  TRACE_MSG(TRACE_MAC2, "<< zb_mlme_purge_request", (FMT__0));
}
#endif /*ZB_MAC_TESTING_MODE*/


static void mac_confirm_pending_queue_item(zb_uint8_t pending_param, zb_mac_status_t mac_status)
{
  zb_mac_mhr_t mhr;
  zb_uint8_t *cmd_ptr;
  zb_uint8_t *fcf;

  ZB_BZERO(&mhr, sizeof(zb_mac_mhr_t));
  cmd_ptr = zb_buf_begin(pending_param);
  cmd_ptr += zb_parse_mhr(&mhr, pending_param);
  fcf = ZB_MAC_GET_FCF_PTR(zb_buf_begin(pending_param));

  TRACE_MSG(TRACE_MAC3, "fcf %x %x cmd %hd", (FMT__D_D_H, *(fcf), *(fcf + 1), (*cmd_ptr)));
  if ((ZB_FCF_GET_FRAME_TYPE(fcf) == MAC_FRAME_COMMAND)
      &&((*cmd_ptr) == MAC_CMD_ASSOCIATION_RESPONSE))
  {
    TRACE_MSG(TRACE_MAC3, "calling zb_mac_send_comm_status buf %hd", (FMT__H, pending_param));
    (void)zb_mac_call_comm_status_ind(pending_param, mac_status, pending_param);
  }
  else
  {
    TRACE_MSG(TRACE_MAC3, "it's data. call confirm param %hd", (FMT__H, pending_param));
    ZB_SET_MAC_STATUS(mac_status);
    zb_mac_send_data_conf(pending_param, ZB_GET_MAC_STATUS(), ZB_TRUE);
  }
}

#if !defined ZB_LITE_NO_INDIRECT_QUEUE_PURGE

/* Non-classic MAC purge */
void zb_mcps_purge_indirect_queue_request(zb_uint8_t param)
{
  zb_mcps_purge_indir_q_req_t *req =
    ZB_BUF_GET_PARAM(param, zb_mcps_purge_indir_q_req_t);
  zb_mcps_purge_indir_q_conf_t *conf;
  zb_mac_status_t status = MAC_INVALID_PARAMETER;
  zb_uindex_t i;
  zb_uint8_t type;
  zb_uint16_t short_addr;
  zb_ieee_addr_t ieee_addr;

  TRACE_MSG(TRACE_MAC1, ">> zb_mcps_purge_indirect_queue_request param %hd, type %hd", (FMT__H_H, param, req->type));

  type = req->type;
  short_addr = req->short_addr;
  ZB_IEEE_ADDR_COPY(ieee_addr, req->ieee_addr);

  conf = ZB_BUF_GET_PARAM(param, zb_mcps_purge_indir_q_conf_t);
  ZB_BZERO(conf, sizeof(*conf));
  conf->type = type;

  if (type <= ZB_MCPS_INDIR_Q_PURGE_TYPE_REJOIN)
  {
    status = MAC_SUCCESS;

    for (i = 0; i < ZB_MAC_PENDING_QUEUE_SIZE; i++)
    {
      if (MAC_CTX().pending_data_queue[i].pending_param != 0U &&
          /* Check short address */
          ((MAC_CTX().pending_data_queue[i].dst_addr_mode == ZB_ADDR_16BIT_DEV_OR_BROADCAST &&
           MAC_CTX().pending_data_queue[i].dst_addr.addr_short == short_addr &&
           short_addr != 0xFFFFU) ||
           /* Check IEEE address */
          (MAC_CTX().pending_data_queue[i].dst_addr_mode == ZB_ADDR_64BIT_DEV &&
           ZB_IEEE_ADDR_CMP(ieee_addr, MAC_CTX().pending_data_queue[i].dst_addr.addr_long) &&
           !ZB_IEEE_ADDR_IS_UNKNOWN(ieee_addr))))
      {
        /* Clear the item and schedule mlme confirm - it does not exactly conform to specs,
         * but allows to avoid address leaks and other problems in layers above */

        ZB_SCHEDULE_ALARM_CANCEL(zb_mac_pending_data_timeout,
                                 MAC_CTX().pending_data_queue[i].pending_param);
        mac_confirm_pending_queue_item(MAC_CTX().pending_data_queue[i].pending_param,
                                       MAC_PURGED);
        MAC_CTX().pending_data_queue[i].pending_param = 0;
      }
    }

#if defined ZB_MAC_PENDING_BIT_SOURCE_MATCHING
    if (short_addr != 0xFFFFU)
    {
      /* And clear pending bit if necessary */
      ZB_SRC_MATCH_SHORT_SET_PENDING_BIT(short_addr, ZB_FALSE);
    }
#endif  /* ZB_MAC_PENDING_BIT_SOURCE_MATCHING */
  }

  conf->status = status;
  ZB_SCHEDULE_CALLBACK(zb_mcps_purge_indirect_queue_confirm, param);

  TRACE_MSG(TRACE_MAC1, "<< zb_mcps_purge_indirect_queue_request status %hd, type %hd", (FMT__H_H, conf->status, conf->type));
}

#endif

void zb_mac_pending_data_timeout(zb_uint8_t param)
{
  zb_uint8_t i;

  TRACE_MSG(TRACE_MAC2, ">> zb_mac_pending_data_timeout param %hd", (FMT__H, param));

  for (i = 0; i < ZB_MAC_PENDING_QUEUE_SIZE; i++)
  {
    if (MAC_CTX().pending_data_queue[i].pending_param == param)
    {
      break;
    }
  }

  if (i < ZB_MAC_PENDING_QUEUE_SIZE)
  {
    /* If the buffer is currently being processed by the MAC layer (e.g. we are waiting for ACK),
       then it would not be a good idea to pass it up to the higher layers.
       In this case, we will postpone the data confirm until the transmission is done.
       Otherwise, we can confirm it right now.
    */
    if (MAC_CTX().tx_wait_cb == NULL || param != MAC_CTX().tx_wait_cb_arg)
    {
      mac_confirm_pending_queue_item(param, MAC_TRANSACTION_EXPIRED);
    }

    /* Clearing the pending table record anyway. */
    mac_clear_pending_queue_slot(i);
  }
  else
  {
    /* this should not happen. Timeout for missed entry */
    ZB_ASSERT(0);
  }

#ifndef ZB_MAC_STICKY_PENDING_BIT
  if (mac_pending_queue_is_empty())
  {
    TRACE_MSG(TRACE_MAC1, "Clearing pending bit", (FMT__0));
    ZB_MAC_TRANS_CLEAR_PENDING_BIT();
  }
#endif

  TRACE_MSG(TRACE_MAC2, "<< zb_mac_pending_data_timeout", (FMT__0));
}

/*
  Puts data to pending queue. It is used for indirect
  transmission. Coordinator side
  return RET_PENDING on success, RET_ERROR on error
*/
zb_ret_t zb_mac_put_data_to_pending_queue(zb_mac_pending_data_t *pend_data)
{
  zb_ret_t ret = RET_PENDING;
  zb_uint8_t i;

/*
  7.1.1.1.3 Effect on receipt
  - if indirect transmission is requested, put data frame to pending
  trasaction queue
*/

  TRACE_MSG(TRACE_MAC2, ">> zb_mac_put_data_to_pending_queue ptr %p", (FMT__P, pend_data));
  /* If use sticky pending bit, it set once from zb_mlme_reset_request() */
#ifndef ZB_MAC_STICKY_PENDING_BIT
  TRACE_MSG(TRACE_MAC2, "Setting pending bit in HW supporting it", (FMT__0));
  ZB_MAC_TRANS_SET_PENDING_BIT();
  MAC_CTX().flags.pending_bit = ZB_TRUE;
#endif
  /* find free slot */
  for (i = 0; i < ZB_MAC_PENDING_QUEUE_SIZE; i++)
  {
    if (MAC_CTX().pending_data_queue[i].pending_param == 0U)
    {
      break;
    }
  }

  if (i == ZB_MAC_PENDING_QUEUE_SIZE)
  {
    TRACE_MSG(TRACE_MAC1, "error, TRANSACTION OVERFLOW", (FMT__0));
    ret = RET_ERROR;
    ZB_SET_MAC_STATUS(MAC_TRANSACTION_OVERFLOW);
  }
  else
  {
    TRACE_MSG(TRACE_MAC2, "Put buf %hd to pending q i %hd", (FMT__H_H, pend_data->pending_param, i));
    ZB_MEMCPY(&MAC_CTX().pending_data_queue[i], pend_data, sizeof(zb_mac_pending_data_t));

    /* timeout value = macTransactionPersistenceTime (in unites),
       unite period = aBaseSuperframeDuration
       Our time quant, which ZB_SCHEDULE_ALARM uses, is beacon interval.
       1 beacon interval(us) = aBaseSuperframeDuration * symbol duration(us)
       Need to schedule alarm for (us):

       mac_transaction_persistence_time * aBaseSuperframeDuration * symbol_duration(us).

       In beacon intervals it will be mac_transaction_persistence_time.

    */
    /*ZB_SCHEDULE_ALARM(zb_mac_pending_data_timeout, MAC_CTX().pending_data_queue[i].pending_param,
                        ZB_MAC_PIB_TRANSACTION_PERSISTENCE_TIME);*/
    if(zb_schedule_alarm(zb_mac_pending_data_timeout, MAC_CTX().pending_data_queue[i].pending_param, ZB_MAC_PIB_TRANSACTION_PERSISTENCE_TIME) == RET_OVERFLOW)
    {      
      if(MAC_CTX().pending_data_queue[i].pending_param > 0 && MAC_CTX().pending_data_queue[i].pending_param<= ZB_IOBUF_POOL_SIZE)
      {
        zb_buf_free(MAC_CTX().pending_data_queue[i].pending_param);
        mac_clear_pending_queue_slot(i);
      }
    }
#if defined ZB_MAC_PENDING_BIT_SOURCE_MATCHING
    /* ZBOSS currently support only short addresses for pending bit
     * setting */
    if (pend_data->dst_addr_mode == ZB_ADDR_16BIT_DEV_OR_BROADCAST)
    {
      ZB_SRC_MATCH_SHORT_SET_PENDING_BIT(pend_data->dst_addr.addr_short, ZB_TRUE);
    }
#endif  /* ZB_MAC_PENDING_BIT_SOURCE_MATCHING */

  }

  TRACE_MSG(TRACE_MAC2, "<< zb_mac_put_data_to_pending_queue, ret %i", (FMT__D, ret));
  return ret;
}


zb_bool_t mac_pending_queue_is_empty()
{
  zb_uindex_t i;
  zb_bool_t ret = ZB_TRUE;

  /* TODO: optimiuze, exclude a loop! */
  for (i = 0; i < ZB_MAC_PENDING_QUEUE_SIZE; i++)
  {
    /* param 0 is used for system purposes */
    if (MAC_CTX().pending_data_queue[i].pending_param != 0U)
    {
      ret = ZB_FALSE;
      break;
    }
  }
  return ret;
}


/* use memmove to guarantee order of packets to be sent
   and clear last slot */
static void mac_clear_pending_queue_slot(zb_uint_t num)
{
  TRACE_MSG(TRACE_MAC2, "mac_clear_pending_queue_slot %d", (FMT__D, num));
  ZB_ASSERT(MAC_CTX().pending_data_queue[(num)].pending_param != 0U);
  MAC_CTX().pending_data_queue[(num)].pending_param = 0;
  /* Note: we have naming conflict in r22 and Nordic branches:
   * ZB_SRC_MATCH_xxx vs ZB_SRC_MATCH_SHORT_xxx. Resolve
   * it later by migrating to r22 namings everywhere. */
#if defined ZB_MAC_PENDING_BIT_SOURCE_MATCHING
  if (MAC_CTX().pending_data_queue[num].dst_addr_mode == ZB_ADDR_16BIT_DEV_OR_BROADCAST)
  {
    zb_uindex_t i;

    /* Check if the entry is unique. If not, don't clear pending bit */
    for (i = 0; i<ZB_MAC_PENDING_QUEUE_SIZE; i++)
    {
      if (MAC_CTX().pending_data_queue[i].pending_param != 0U &&
          MAC_CTX().pending_data_queue[i].dst_addr_mode == ZB_ADDR_16BIT_DEV_OR_BROADCAST &&
          MAC_CTX().pending_data_queue[i].dst_addr.addr_short == MAC_CTX().pending_data_queue[num].dst_addr.addr_short)
      {
        break;
      }
    }

    if (i == ZB_MAC_PENDING_QUEUE_SIZE)
    {
      ZB_SRC_MATCH_SHORT_SET_PENDING_BIT(
        MAC_CTX().pending_data_queue[num].dst_addr.addr_short, ZB_FALSE);
    }
  }
#endif  /* ZB_MAC_PENDING_BIT_SOURCE_MATCHING */
  if (num < ZB_MAC_PENDING_QUEUE_SIZE - 1U)
  {
    ZB_MEMMOVE(&MAC_CTX().pending_data_queue[(num)], &MAC_CTX().pending_data_queue[(num + 1U)],
               (ZB_MAC_PENDING_QUEUE_SIZE - num - 1U) * sizeof(zb_mac_pending_data_t));
    MAC_CTX().pending_data_queue[ZB_MAC_PENDING_QUEUE_SIZE - 1U].pending_param = 0;
  }

  {
    zb_uindex_t i;
    for (i = 0; i < ZB_MAC_PENDING_QUEUE_SIZE; i++)
    {
      TRACE_MSG(TRACE_MAC3, "i %hd pending_data %hd dst_addr_mode %hd",
                (FMT__H_H_H, i, MAC_CTX().pending_data_queue[i].pending_param,
                 MAC_CTX().pending_data_queue[i].dst_addr_mode));
    }
  }
}

#if defined ZB_MAC_PENDING_BIT_SOURCE_MATCHING
static zb_mac_status_t mac_src_match_add(zb_mac_src_match_params_t *match_params)
{
  zb_mac_status_t status;

  if (match_params->addr_mode == ZB_ADDR_16BIT_DEV_OR_BROADCAST)
  {
    /* Return status in case of overflow */
    status = (zb_mac_status_t) ZB_SRC_MATCH_ADD_SHORT_ADDR(match_params->index,
                                         match_params->addr.addr_short);

#ifdef ZB_MAC_POLL_INDICATION_CALLS_REDUCED
    if (status == MAC_SUCCESS)
    {
      /* Update Poll Indication call timeout */
      ZB_MAC_UPDATE_POLL_IND_CALL_TIMEOUT(match_params->index,
                                          match_params->addr.addr_short,
                                          match_params->poll_timeout);
    }
#endif /* ZB_MAC_POLL_INDICATION_CALLS_REDUCED */

    TRACE_MSG(TRACE_MAC3, "mac_src_match_add index %hd short_addr 0x%x status %hd",
              (FMT__H_D_H, match_params->index, match_params->addr.addr_short, status));
  }
  else if (match_params->addr_mode == ZB_ADDR_64BIT_DEV)
  {
    /* Return status in case of overflow */
    status = (zb_mac_status_t)ZB_SRC_MATCH_ADD_IEEE_ADDR(match_params->index,
                                                    match_params->addr.addr_long);

    TRACE_MSG(TRACE_MAC3, "mac_src_match_add index %hd ieee_addr " TRACE_FORMAT_64 " status %hd ",
              (FMT__H_A_H, match_params->index, TRACE_ARG_64(match_params->addr.addr_long), status));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "mac_src_match_add: unknown addr_mode %hd index %hd",
              (FMT__H_H, match_params->addr_mode, match_params->index));
    status = MAC_INVALID_PARAMETER;
  }

  return status;
}

static zb_mac_status_t mac_src_match_delete(zb_mac_src_match_params_t *match_params)
{
  zb_mac_status_t status;

  if (match_params->addr_mode == ZB_ADDR_16BIT_DEV_OR_BROADCAST)
  {
#ifdef ZB_MAC_POLL_INDICATION_CALLS_REDUCED
    /* Reset Poll Indication call timeout */
    ZB_MAC_UPDATE_POLL_IND_CALL_TIMEOUT(match_params->index,
                                        match_params->addr.addr_short,
                                        0);
#endif /* ZB_MAC_POLL_INDICATION_CALLS_REDUCED */

    /* Return status in case of overflow */
    status = (zb_mac_status_t) ZB_SRC_MATCH_DELETE_SHORT_ADDR(match_params->index,
                                            match_params->addr.addr_short);

    TRACE_MSG(TRACE_MAC3, "mac_src_match_delete index %hd short_addr 0x%x status %hd",
              (FMT__H_D_H, match_params->index, match_params->addr.addr_short, status));
  }
  else if (match_params->addr_mode == ZB_ADDR_64BIT_DEV)
  {
    /* Return status in case of overflow */
    status = (zb_mac_status_t)ZB_SRC_MATCH_DELETE_IEEE_ADDR(match_params->index, match_params->addr.addr_long);

    TRACE_MSG(TRACE_MAC3, "mac_src_match_delete index %hd ieee_addr " TRACE_FORMAT_64 " status %hd ",
              (FMT__H_A_H, match_params->index, TRACE_ARG_64(match_params->addr.addr_long), status));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "mac_src_match_delete: Unknown addr_mode %hd",
                  (FMT__H, match_params->addr_mode));
    status = MAC_INVALID_PARAMETER;
  }

  return status;
}


zb_mac_status_t zb_mac_src_match_process_cmd(zb_mac_src_match_params_t *match_params)
{
  zb_mac_status_t status = MAC_SUCCESS;

  TRACE_MSG(TRACE_MAC3, ">> zb_mac_src_match_process_cmd match_params %p",
            (FMT__P, match_params));

  if (match_params != NULL)
  {
    switch (match_params->cmd)
    {
      case ZB_MAC_SRC_MATCH_CMD_ADD:
        status = mac_src_match_add(match_params);
        break;

      case ZB_MAC_SRC_MATCH_CMD_DELETE:
        status = mac_src_match_delete(match_params);
        break;

      case ZB_MAC_SRC_MATCH_CMD_DROP:
        ZB_SRC_MATCH_TBL_DROP();
        break;

      default:
        TRACE_MSG(TRACE_ERROR, "Unsupported command", (FMT__0));
        status = MAC_INVALID_PARAMETER;
        break;
    }
  }
  else
  {
    /* Will assert in debug build, return error in release build */
    TRACE_MSG(TRACE_ERROR, "match_params is NULL", (FMT__0));
    ZB_ASSERT(0);

    status = MAC_INVALID_PARAMETER;
  }

  TRACE_MSG(TRACE_MAC3, "<< zb_mac_src_match_process_cmd status %hd", (FMT__H, status));

  return status;
}
#endif  /* ZB_MAC_PENDING_BIT_SOURCE_MATCHING */

#endif /* ZB_ROUTER_ROLE */

/*! @} */

#endif  /* !ZB_ALIEN_MAC && !ZB_MACSPLIT_HOST */
