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
/* PURPOSE: APS layer
*/

#define ZB_TRACE_FILE_ID 105
#include "zb_common.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "aps_internal.h"
#include "zb_zdo.h"
#include "../nwk/nwk_internal.h"
#include "zb_bdb_internal.h"
#include "zb_bufpool.h"
#include "zdo_diagnostics.h"
#include "zb_ncp.h"
#if defined ZB_ENABLE_INTER_PAN_EXCHANGE
#include "zb_aps_interpan.h"
#endif /* defined ZB_ENABLE_INTER_PAN_EXCHANGE */

/*! \addtogroup ZB_APS */
/*! @{ */


/**
   APS Informational Base in-memory data structure.

   Access to this structure using macros.
 */



void aps_data_hdr_fill_datareq(zb_uint8_t fc, zb_apsde_data_req_t *req, zb_bufid_t param);
static void aps_ack_send_handle(zb_bufid_t packet, zb_aps_hdr_t *aps_hdr);
static zb_ret_t save_ack_data(zb_uint8_t param, zb_apsde_data_req_t *req, zb_uint8_t *ref);
static void aps_ack_frame_handle(zb_aps_hdr_t *aps_hdr);
static void done_with_this_ack(zb_ushort_t i, const zb_aps_hdr_t *aps_hdr, zb_ret_t status);
static void aps_send_frame_via_binding(zb_uint8_t param);
static zb_bool_t zb_aps_buf_retrans_isbusy(zb_bufid_t bufi);
static void aps_retrans_ent_free(zb_uint8_t idx);
static zb_bool_t aps_retrans_ent_busy_state(zb_uint8_t idx);

#ifdef APS_FRAGMENTATION

/* Special value indicating invalid fragment payload length. */
#define INVALID_FRAG_PAYLOAD_LEN 0xFFu

static void aps_out_frag_schedule_queued_requests(void);

static zb_ret_t aps_prepare_frag_block(zb_bufid_t original_buf, zb_bufid_t target_buf, zb_uint8_t block_no);
static void aps_prepare_frag_transmission(zb_bufid_t buf, zb_uint8_t max_fragment_size);
static void zb_aps_out_frag_schedule_send_next_block_delayed(void);
static void zb_aps_out_frag_schedule_send_next_block(zb_uint8_t param);
static void aps_in_frag_mac_ack_for_aps_ack_arrived(zb_uint8_t aps_counter, zb_uint16_t buf_num_ack);
#if defined SNCP_MODE || defined ZB_APS_FRAG_FORCE_DISABLE_TURBO_POLL
static zb_uint8_t aps_in_frag_trans_exist(void);
#endif

/*
   The description of how fragmented RX is implemented here.

   The following is happening for each window:
   1. What triggers the start of a new RX window (both are processed in aps_in_frag_remember_frame() ):
      a) A new transaction is started
      b) We have completely received and processed the previous window and a packet of the successive
      window has been received.
   2. Two timers get started:
      a) Fail timer - if the window does not move in apscAckWaitDuration * (1 + apscMaxFrameRetries),
      then the transaction will be dropped. To synchronize the timeout with the sending side, we must
      calculate the timeout based on our rx_on_when_idle state.
      b) Ack timer - if we do not receive any new fragments from the current window
      within apscAckWaitDuration, we transmit a APS acknowledgement indicating currently received
      blocks. This is NOT required by r22 specification (although it is not restricted either), but
      it helps to reduce the number of retransmissions and thus speeds up the process. Also, this is
      useful to handle the loss of APS acks.
   2.1. Ack timer started only if ZG->aps.aib.aps_max_window_size > 1
   3. Right after a new window has began, the whole transaction is assigned a APS_IN_FRAG_RECEIVING
      state.
   4. When we receive a new block:
      a) If the window is in APS_IN_FRAG_RECEIVING state and the block belongs to the current window,
      then we remember the block.
      b) State APS_IN_FRAG_WINDOW_MERGED_NO_ACK means that the window is ready, but the MAC ACK to
      our APS ACK has not been received. And while we can not move on.
      c) After receiving the MAC ACK on our APS ACK, the state changes to APS_IN_FRAG_WINDOW_MERGED.
      d) If the window is in APS_IN_FRAG_WINDOW_MERGED state and the block belongs to the next window,
      we clear previously received blocks and the window is moved forward.
      e) Otherwise, the block is ignored.
   5. Each time we receive a block, we are also attempting to change the current state of the
      transaction - the window or the whole transaction might have just been received completely.
      This happens inside the aps_in_frag_attempt_state_transition() function.
      a) If the last window has not been fully received, exit.
      b) If we are not in the APS_IN_FRAG_RECEIVING state, exit. That means we are currently processing
      the window or that we have just processed it, but we have not yet moved the window forward.
      c) We send an APS acknowledgement - spec says we must do it when we've received the whole window
      or the whole transaction.
      d) We start the process of merging the window (see later) and change the state to
      APS_IN_FRAG_COMPLETE or APS_IN_FRAG_WINDOW_RECEIVED (depending on the current situation).

   After all blocks of a window have been received, we merge them into the transcation buffer (a
   buffer that will later be used to indicate the result of the transaction). The process is started
   by calling the aps_in_frag_assemble_window() function.
   1. We calculate the minimum required buffer size to store the received payload and some additional
      data (e.g. APS header, buffer paramter).
   2. We need to ensure that our transcation buffer ("buffer" fied of zb_aps_in_fragment_frame_t) has
      the required capacity.
      a) If we haven't allocated it yet, then allocate (aps_in_frag_assemble_window_delayed()).
      b) If the current transcation buffer is big enough to store the data, do nothing
      c) Otherwise, allocate a buffer of required size and relocate the existing data into it.
   3. For each received block of the current window we do the following steps
      (aps_in_frag_do_merge_blocks() function):
      a) Allocate space at the right of the transaction buffer
      b) Copy data from the block to the transaction buffer.
      c) Also we must take care of APS header (prepended at the left) and the buffer parameter.
   4. If the transaction is in the APS_IN_FRAG_WINDOW_RECEIVED state, we change the state to
      APS_IN_FRAG_WINDOW_MERGED_NO_ACK and further to APS_IN_FRAG_WINDOW_MERGED after got MAC ACK
      for our APS ACK. This is a sign for aps_in_frag_remember_frame() that we are now allowed to
      move the current window forward. Note that we do not free the blocks at the moment.
   5. If the transaction is in the APS_IN_FRAG_COMPLETE state, then we indicate the packet after
      got MAC ACK for our APS ACK and stop the transcation by calling the aps_in_frag_stop_entry().

   Additionally, in order to recover from the loss of the last APS ACK we do the following:
   1. We persist the entry for 2 * apscAckWaitDuration (then it is killed by
      aps_in_frag_kill_stopped_entries())
   2. When a fragment is received that belongs to an already completed transaction (that would normally
      happen when if the transmitter hasn't received an APS ack and decided to retransmit all non
      acked blocks), we transmit an APS ACK with 0xFF block mask. That happens inside
      aps_in_frag_ack_completed_transaction(). The function also ACKs the transaction with
      the APS_IN_FRAG_WINDOW_MERGED_NO_ACK status.

   How timers work:

   Since we are supporting simultaneous incoming transactions, we need to distinguish between them
   inside all fragmentation-related functions. In ZBOSS, we are passing buffers as callback (and
   alarm) parameters, not IDs. The biggest problem here is that we do not have any buffer that is
   associated with a transaction throughout its lifetime and that can be used as its const identifier:
   1. Buffers that store window block are freed each time we move the window.
   2. Transaction buffer can be re-allocated if we've received more payload data than the buffer's size

   Current implementation uses the following approach: we abandon the idea of a constant identifier,
   and instead use any of buffers that are currently used to store window fragments.
   1. When a function is called, it goes through all transaction and all their currently received
      buffers. The current transaction is the one which contained the buffer passed as an argument.
   2. When we need to schedule a function, we take any of the currently used buffers.
   3. When we need to cancel an alarm, we cancel the alarm for all currently available buffers.

   It works since each time we clear buffers (e.g. when a window moves) we need to restart all
   timers anyway.

   This behaviour is encapsulated in the following functions:
   aps_in_frag_*()
*/

static zb_uint8_t aps_in_frag_find_by_hdr(zb_uint16_t src_addr, zb_uint8_t aps_counter);
static zb_uint8_t aps_in_frag_find_by_buf(zb_bufid_t buf);
static zb_aps_in_fragment_frame_t* aps_in_frag_allocate(zb_uint16_t src_addr, zb_uint8_t aps_counter);
static void aps_in_frag_free_window(zb_aps_in_fragment_frame_t *ent);

static void aps_in_frag_kill_stopped_entries(zb_uint8_t param);
static void aps_in_frag_stop_entry(zb_aps_in_fragment_frame_t *ent, zb_bool_t delayed_kill);

static zb_uint8_t aps_in_frag_get_received_mask(zb_aps_in_fragment_frame_t *ent);
static zb_uint_t aps_in_frag_total_window_size(const zb_aps_in_fragment_frame_t *ent);

static void aps_in_frag_do_merge_blocks(zb_aps_in_fragment_frame_t *ent);
static void aps_in_frag_assemble_window_delayed(zb_bufid_t buf, zb_uint16_t params);
static zb_ret_t aps_in_frag_assemble_window(zb_uint8_t frag_idx);

static zb_bool_t aps_in_frag_is_block_within_window(zb_uint8_t current_window,
                                                    zb_uint8_t block_no,
                                                    zb_uint8_t total_blocks_num);
static zb_bufid_t aps_in_frag_find_any_block(zb_aps_in_fragment_frame_t *ent);

static void aps_in_frag_send_ack(zb_bufid_t ack_buf, zb_uint16_t idx);
static void aps_in_frag_send_ack_delayed(zb_bufid_t buf);
static void aps_in_frag_stop_ack_timer(zb_aps_in_fragment_frame_t *ent);
static void aps_in_frag_restart_ack_timer(zb_uint8_t in_frag_idx);

static void aps_in_frag_transaction_failed(zb_bufid_t buf);
static void aps_in_frag_stop_fail_timer(zb_aps_in_fragment_frame_t *ent);
static void aps_in_frag_restart_fail_timer(zb_aps_in_fragment_frame_t *ent);

static zb_ret_t aps_in_frag_ack_completed_transaction(zb_bufid_t param, zb_aps_hdr_t *aps_hdr);
static zb_ret_t aps_in_frag_attempt_state_transition(zb_bufid_t param,
                                                     zb_bufid_t ack_param,
                                                     zb_aps_hdr_t *aps_hdr);
static zb_ret_t aps_in_frag_remember_frame(zb_bufid_t param, zb_aps_hdr_t *aps_hdr);
#endif

void zb_aps_ack_timer_cb(zb_uint8_t param);
void zb_aps_pass_group_msg_up(zb_uint8_t new_param, zb_uint16_t param);
void zb_aps_pass_local_group_pkt_up(zb_uint8_t param);
void zb_aps_pass_up_group_buf(zb_uint8_t param);
void zb_nlde_data_indication_continue(zb_uint8_t ack_param, zb_uint16_t param);

/* 14/06/2016 CR [AEV] start */
void zb_aps_pass_local_msg_up(zb_uint8_t newbuf, zb_uint16_t param);
/* 14/06/2016 CR [AEV] end */

static zb_ret_t zb_check_bind_trans(zb_uint8_t param);
static zb_ret_t zb_process_bind_trans(zb_uint8_t param);
static void zb_clear_bind_trans(zb_uint8_t param, zb_aps_hdr_t *aps_hdr);

void zb_aps_init()
{
  zb_ushort_t i;

  TRACE_MSG(TRACE_APS1, "+zb_aps_init", (FMT__0));
  /* Initialize APSIB */
  ZG->aps.aib.aps_counter = ZB_RANDOM_U8();

#ifndef ZB_NO_NWK_MULTICAST
  /* Init by default value */
  ZB_AIB().aps_nonmember_radius = ZB_APS_AIB_DEFAULT_NONMEMBER_RADIUS;
  ZB_AIB().aps_max_nonmember_radius = ZB_APS_AIB_DEFAULT_MAX_NONMEMBER_RADIUS;
#endif

  for (i = 0 ; i < ZB_N_APS_RETRANS_ENTRIES ; ++i)
  {
    ZG->aps.retrans.hash[i].addr = (zb_uint16_t)-1;
    ZG->aps.retrans.hash[i].buf = (zb_uint8_t)-1;
  }

#ifdef APS_FRAGMENTATION
  ZB_BZERO(&ZG->aps.in_frag, sizeof(ZG->aps.in_frag));
  for (i = 0; i < ZB_APS_MAX_IN_FRAGMENT_TRANSMISSIONS; i++)
  {
    ZG->aps.in_frag[i].src_addr = ZB_APS_IN_FRAGMENTED_FRAME_EMPTY;
  }

  ZB_BZERO(&ZG->aps.out_frag, sizeof(zb_aps_out_fragment_frame_t));
  ZG->aps.aib.aps_max_window_size = ZB_APS_MAX_WINDOW_SIZE;
  ZG->aps.aib.aps_interframe_delay = ZB_APS_INTERFRAME_DELAY;
#endif

#ifdef SNCP_MODE
  /* remote bind not set */
  ZG->aps.binding.remote_bind_offset = (zb_uint8_t)-1;
#endif

  zb_aps_secur_init();

#ifdef ZB_CERTIFICATION_HACKS
  ZB_CERT_HACKS().src_binding_table_size = ZB_APS_SRC_BINDING_TABLE_SIZE;
  ZB_CERT_HACKS().dst_binding_table_size = ZB_APS_DST_BINDING_TABLE_SIZE;
  ZB_CERT_HACKS().aps_mcast_nwk_dst_addr = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
#endif

  TRACE_MSG(TRACE_APS3,
            "ZB_N_APS_RETRANS_ENTRIES %hd ZB_N_APS_ACK_WAIT_DURATION_SLEEPY %d, NON_SLEEPY %d",
            (FMT__H_D_D, ZB_N_APS_RETRANS_ENTRIES,
             ZB_N_APS_ACK_WAIT_DURATION_FROM_SLEEPY, ZB_N_APS_ACK_WAIT_DURATION_FROM_NON_SLEEPY));

  TRACE_MSG(TRACE_APS1, "-zb_aps_init", (FMT__0));
}

/* This function is iterational, called from aps packet request call.
   It checks binding table and creates a new record in transmission
   table for the next packet to be transmitted, also fill it's header.
*/
static zb_ret_t zb_process_bind_trans(zb_uint8_t param)
{
  zb_apsde_data_req_t *apsreq = ZB_BUF_GET_PARAM(param, zb_apsde_data_req_t);
  zb_uint8_t i;
  zb_uint8_t next_index = 0xFF, free_index = ZB_APS_BIND_TRANS_TABLE_SIZE;
  zb_bool_t  is_new = ZB_TRUE;
  zb_ret_t ret;
  zb_address_ieee_ref_t addr_ref;
  zb_uint8_t ind_src;
  zb_bool_t is_last = ZB_FALSE;

  TRACE_MSG(TRACE_APS1, "+zb_process_bind_trans %hd", (FMT__H, param));

  ret = zb_address_by_short(ZB_PIBCACHE_NETWORK_ADDRESS(), ZB_FALSE, ZB_FALSE, &addr_ref);
  if (ret == RET_OK)
  {
    ind_src = aps_find_src_ref(addr_ref, apsreq->src_endpoint, apsreq->clusterid);
  }
  else
  {
    ind_src = 0xFF; /* invalid index */
  }
  if (ind_src == 0xFFU)
  {
    TRACE_MSG(TRACE_APS1, "no bound device", (FMT__0));
    ret = ERROR_CODE(ERROR_CATEGORY_APS,ZB_APS_STATUS_NO_BOUND_DEVICE);
  }

  if (ret == RET_OK)
  {
    for (i = 0; i < ZB_APS_BIND_TRANS_TABLE_SIZE; i++)
    {
      if (ZG->aps.binding.trans_table[i] == param)
      {
        TRACE_MSG(TRACE_APS1, "found binding transmission entry, index %hd", (FMT__H, i));
        is_new = ZB_FALSE;
        free_index = i;
        break;
      }
      else if (ZG->aps.binding.trans_table[i] == 0U)
      {
        free_index = i;
      }
      else
      {
        /* MISRA rule 15.7 requires empty 'else' branch. */
      }
    }

    /* Strict condition for prevention array borders violation */
    if (free_index < ZB_APS_BIND_TRANS_TABLE_SIZE)
    {
      if (is_new)
      {
        ZG->aps.binding.trans_table[free_index] = param;
      }
      TRACE_MSG(TRACE_APS1, "moving through dst table, steps:  %hd", (FMT__H, ZG->aps.binding.dst_n_elements));
      for (i = 0; i < ZG->aps.binding.dst_n_elements; i++)
      {
        TRACE_MSG(TRACE_APS1, "loop:  %hd, src_index %hd", (FMT__H_H, i, ZG->aps.binding.dst_table[i].src_table_index));
        if ( ZG->aps.binding.dst_table[i].src_table_index == ind_src )
        {
          if (is_new)
          {
            if (next_index == 0xFFU)
            {
              next_index = i;
              is_last = ZB_TRUE;
              TRACE_MSG(TRACE_APS1, "adding record to dst table index %hd", (FMT__H, i));
              ZG->aps.binding.trans_table[free_index] = param;
            }
            else
            {
              is_last = ZB_FALSE;
            }
            TRACE_MSG(TRACE_APS1, "new transmission via binding", (FMT__0));
            ZG->aps.binding.dst_table[i].trans_index[free_index/8U] |= (1U << (free_index % 8U));
          }
          else if (ZB_APS_IS_TRANS_INDEX(i, free_index) != 0U)
          {
            if (next_index == 0xFFU)
            {
              next_index = i;
              is_last = ZB_TRUE;
            }
            else
            {
              is_last = ZB_FALSE;
              break;
            }
          }
          else
          {
            /* MISRA rule 15.7 requires empty 'else' branch. */
          }
        }
      }
    }
    else
    {
      ret = RET_TABLE_FULL;
    }
  }


  TRACE_MSG(TRACE_APS1, "ret %hd", (FMT__H, ret));

  /* Fill structure for data_request, convert long address ref from binding table to dst short address */
  if (ret == RET_OK)
  {
    zb_aps_bind_dst_table_t *dst_ent = &ZG->aps.binding.dst_table[next_index];
    zb_aps_bind_src_table_t *src_ent = &ZG->aps.binding.src_table[dst_ent->src_table_index];

    if (dst_ent->dst_addr_mode == ZB_APS_BIND_DST_ADDR_LONG)
    {
      /* Send by long here - may try to discover short by long in apsde_data_req(). */
      zb_address_ieee_by_ref(apsreq->dst_addr.addr_long, dst_ent->u.long_addr.dst_addr);
      apsreq->clusterid = src_ent->cluster_id;
      apsreq->src_endpoint = src_ent->src_end;
      apsreq->addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
    }
    else
    {
      apsreq->dst_addr.addr_short = dst_ent->u.group_addr;
      apsreq->addr_mode = ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT;
    }
    /* TRACE_MSG(TRACE_APS3, "new apsreq dst_addr short = 0x%x", (FMT__D, apsreq->dst_addr.addr_short)); */

    apsreq->dst_endpoint  = dst_ent->u.long_addr.dst_end;

    if (is_last
        /* Strict condition for prevention array borders violation */
        && free_index < ZB_APS_BIND_TRANS_TABLE_SIZE)
    {
      TRACE_MSG(TRACE_APS1, "last element, clear tables param %hd, index %hd", (FMT__H_H, param, free_index));
      ZG->aps.binding.trans_table[free_index] = 0x00;
    }
    if (/* Strict condition for prevention array borders violation */
        free_index < ZB_APS_BIND_TRANS_TABLE_SIZE
        && free_index / 8U < ZB_SINGLE_TRANS_INDEX_SIZE)
    {
      ZG->aps.binding.dst_table[next_index].trans_index[free_index/8U] &= ~(1U << (free_index % 8U));
    }
  }
  TRACE_MSG(TRACE_APS1, "-zb_process_bind_trans %hd", (FMT__H, param));

  return ret;
}

/*
   This function clears ZG->aps.binding.trans_table and
   ZG->aps.binding.dst_table[(dtbli)].trans_index for certain transmission.
   Called in zb_nlde_data_confirm() for the case when NWK
   has rejected our frame because we are no longer joined to the network.
*/
static void zb_clear_bind_trans(zb_uint8_t param, zb_aps_hdr_t *aps_hdr)
{
  zb_ret_t ret;
  zb_address_ieee_ref_t addr_ref;
  zb_uint8_t i;
  zb_uint8_t target_ind = 0xFF;
  zb_uint8_t ind_src = (zb_uint8_t)(-1);

  TRACE_MSG(TRACE_APS1, "+zb_clear_bind_trans %hd", (FMT__H, param));

  ret = zb_address_by_short(ZB_PIBCACHE_NETWORK_ADDRESS(), ZB_FALSE, ZB_FALSE, &addr_ref);
  if (ret == RET_OK)
  {
    ind_src = aps_find_src_ref(addr_ref, aps_hdr->src_endpoint, aps_hdr->clusterid);
  }

  if (ret != RET_OK || ind_src == (zb_uint8_t)(-1))
  {
    TRACE_MSG(TRACE_APS1, "no bound device", (FMT__0));
    ret = ERROR_CODE(ERROR_CATEGORY_APS,ZB_APS_STATUS_NO_BOUND_DEVICE);
  }

  if (ret == RET_OK)
  {
    for (i = 0; i < ZB_APS_BIND_TRANS_TABLE_SIZE; i++)
    {
      if (ZG->aps.binding.trans_table[i] == param)
      {
        TRACE_MSG(TRACE_APS1, "found binding transmission entry, index %hd", (FMT__H, i));
        ZG->aps.binding.trans_table[i] = 0;
        target_ind = i;
      }
    }

    if (target_ind == 0xFFU)
    {
      ret = RET_NOT_FOUND;
    }
  }

  if (ret == RET_OK)
  {
    TRACE_MSG(TRACE_APS1, "moving through dst table, steps:  %hd", (FMT__H, ZG->aps.binding.dst_n_elements));
    for (i = 0; i < ZG->aps.binding.dst_n_elements; i++)
    {
      TRACE_MSG(TRACE_APS1, "loop:  %hd, src_index %hd", (FMT__H_H, i, ZG->aps.binding.dst_table[i].src_table_index));
      if ( ZG->aps.binding.dst_table[i].src_table_index == ind_src )
      {
        if (/* Strict condition for prevention array borders violation */
            target_ind < ZB_APS_BIND_TRANS_TABLE_SIZE
            && target_ind / 8U < ZB_SINGLE_TRANS_INDEX_SIZE)
        {
          ZG->aps.binding.dst_table[i].trans_index[target_ind/8U] &= ~(1U << (target_ind % 8U));
        }
      }
    }
  }

  TRACE_MSG(TRACE_APS1, "-zb_clear_bind_trans %hd", (FMT__H, param));
}

#ifdef APS_FRAGMENTATION
zb_uint16_t zb_aps_get_max_trans_size(zb_uint16_t short_addr)
{
#ifdef ZB_CERTIFICATION_HACKS
  if (ZB_CERT_HACKS().frag_skip_node_descr)
  {
    return ZB_APS_MAX_FRAGMENT_NUM * ZB_ZCL_HI_WO_IEEE_MAX_PAYLOAD_SIZE;
  }
  else
#endif
  {
    zb_address_ieee_ref_t addr_ref;
    zb_uint16_t ret = ZB_APS_INVALID_MAX_TRANS_SIZE;

    if (zb_address_by_short(short_addr,
                            ZB_TRUE, /* create if absent (is it ever possible?) */
                            ZB_FALSE, /* do not lock */
                            &addr_ref) == RET_OK)
    {
      zb_uint8_t i;

      for (i = 0; i < ZB_ARRAY_SIZE(ZG->aps.max_trans_size); ++i)
      {
        if (ZG->aps.max_trans_size[i].clock > 0U &&
            ZG->aps.max_trans_size[i].addr_ref == addr_ref)
        {
          break;
        }
      }

      if (i < ZB_ARRAY_SIZE(ZG->aps.max_trans_size))
      {
        ret = ZG->aps.max_trans_size[i].max_in_trans_size;
      }
    }
    TRACE_MSG(TRACE_APS1, "zb_aps_get_max_trans_size %d", (FMT__D, ret));
    return ret;
  }
}

zb_uint8_t zb_aps_get_max_buffer_size(zb_uint16_t short_addr)
{
#ifdef ZB_CERTIFICATION_HACKS
  if (ZB_CERT_HACKS().frag_skip_node_descr)
  {
    return ZB_ZCL_HI_WO_IEEE_MAX_PAYLOAD_SIZE;
  }
  else
#endif
  {
    zb_address_ieee_ref_t addr_ref;
    zb_uint8_t ret = ZB_APS_INVALID_MAX_BUFFER_SIZE;

    if (zb_address_by_short(short_addr,
                            ZB_TRUE, /* create if absent (is it ever possible?) */
                            ZB_FALSE, /* do not lock */
                            &addr_ref) == RET_OK)
    {
      zb_uint8_t i;

      for (i = 0; i < ZB_ARRAY_SIZE(ZG->aps.max_trans_size); ++i)
      {
        if (ZG->aps.max_trans_size[i].clock > 0U &&
            ZG->aps.max_trans_size[i].addr_ref == addr_ref)
        {
          break;
        }
      }

      if (i < ZB_ARRAY_SIZE(ZG->aps.max_trans_size))
      {
        ret = ZG->aps.max_trans_size[i].max_buffer_size;
      }
    }
    TRACE_MSG(TRACE_APS1, "zb_aps_get_max_trans_size %d", (FMT__D, ret));
    return ret;
  }
}

static zb_uint8_t zb_aps_frag_payload_len(zb_uint16_t short_addr)
{
  zb_uint8_t len;
  zb_uint8_t max_buffer_size;

#ifdef ZB_CERTIFICATION_HACKS
  if (ZB_CERT_HACKS().frag_block_size)
  {
    return ZB_CERT_HACKS().frag_block_size;
  }
#endif

  max_buffer_size = zb_aps_get_max_buffer_size(short_addr);
  if (max_buffer_size > ZB_APS_HEADER_MAX_LEN + 2U)
  {
    len = max_buffer_size - ZB_APS_HEADER_MAX_LEN - 2U;

    /*we have some problems with sending 66 bytes*/
    if (len > ZB_APS_MAX_PAYLOAD_SIZE - 13U)
    {
      len = ZB_APS_MAX_PAYLOAD_SIZE - 13U;
    }
  }
  else
  {
    /* Too small negotiated max buffer size with a remote device. */
    len = INVALID_FRAG_PAYLOAD_LEN;
  }
  ZB_ASSERT(len > 0u);
  return len;
}

static zb_bool_t zb_aps_age_max_trans_size(void)
{
  zb_ushort_t i;
  zb_bool_t non_empty = ZB_FALSE;

  for (i = 0; i < ZB_ARRAY_SIZE(ZG->aps.max_trans_size); i++)
  {
    zb_aps_max_trans_size_t *ent = &ZG->aps.max_trans_size[i];

    if (ent->clock != 0U)
    {
      ent->clock--;
      if(!non_empty && (ent->clock != 0U))
      {
        non_empty = ZB_TRUE;
      }
    }
  }
  return non_empty;
}

/* TODO: May use some unified aging scheme (one alarm, different counters)?
   Already have many independent aging systems (route, route discovery, ed aging, dups, etc) which
   brings additional load for alarms subsystem. */
static void zb_aps_check_max_trans_size_timer_cb(zb_uint8_t param)
{
  ZVUNUSED(param);

  ZG->aps.max_trans_size_alarm_running = zb_aps_age_max_trans_size();

  /* schedule myself again */
  if (ZG->aps.max_trans_size_alarm_running)
  {
    ZB_SCHEDULE_ALARM(zb_aps_check_max_trans_size_timer_cb, 0, ZB_APS_MAX_TRANS_SIZE_TMO);
  }
}

void zb_aps_add_max_trans_size(zb_uint16_t short_addr, zb_uint16_t max_trans_size, zb_uint8_t max_buffer_size)
{
  zb_address_ieee_ref_t addr_ref;

  if (zb_address_by_short(short_addr,
                          ZB_TRUE, /* create if absent (is it ever possible?) */
                          ZB_FALSE, /* do not lock */
                          &addr_ref) == RET_OK)
  {
    zb_uint8_t i, ent_i = 0xFF, free_i = 0xFF, old_i = 0xFF;

    for (i = 0; i < ZB_ARRAY_SIZE(ZG->aps.max_trans_size); ++i)
    {
      if (ZG->aps.max_trans_size[i].clock > 0U && ZG->aps.max_trans_size[i].addr_ref == addr_ref)
      {
        ent_i = i;
        break;
      }
      else if (ZG->aps.max_trans_size[i].clock == 0U  && free_i == 0xFFU)
      {
        free_i = i;
      }
      else if (ZG->aps.max_trans_size[i].clock > 0U && old_i == 0xFFU)
      {
        old_i = i;
      }
      else if (/* Strict condition for prevention array borders violation */
               old_i < ZB_ARRAY_SIZE(ZG->aps.max_trans_size)
               && ZG->aps.max_trans_size[old_i].clock > ZG->aps.max_trans_size[i].clock)
      {
        old_i = i;
      }
      else
      {
        /* MISRA rule 15.7 requires empty 'else' branch. */
      }
    }

    i = ent_i != 0xFFU ? ent_i :
      (free_i != 0xFFU ? free_i :
       old_i);

    /* Strict condition for prevention array borders violation */
    if (i < ZB_ARRAY_SIZE(ZG->aps.max_trans_size))
    {
      ZG->aps.max_trans_size[i].addr_ref = addr_ref;
      ZG->aps.max_trans_size[i].max_in_trans_size = max_trans_size;
      ZG->aps.max_trans_size[i].max_buffer_size = max_buffer_size;
      ZG->aps.max_trans_size[i].clock = ZB_APS_MAX_TRANS_SIZE_CLOCK;
    }
    else
    {
      ZB_ASSERT(0);
    }

    if (!ZG->aps.max_trans_size_alarm_running)
    {
      ZG->aps.max_trans_size_alarm_running = ZB_TRUE;
      ZB_SCHEDULE_ALARM(zb_aps_check_max_trans_size_timer_cb, 0, ZB_APS_MAX_TRANS_SIZE_TMO);
    }
  }
}
#endif

static void zb_check_frag_queue_cleanup_and_send_fail_confirm(zb_uint8_t param, zb_ret_t status)
{
#ifdef APS_FRAGMENTATION
  zb_uint8_t i,j;
  zb_bool_t packet_found = ZB_FALSE;
  zb_bool_t block_ref_j_busy;  /* Busy block status for ZG->aps.out_frag.block_ref[j] */

  TRACE_MSG(TRACE_APS2, "+zb_check_frag_queue_and_cleanup: ref %hd status %d", (FMT__H_D, param, status));

  for ( j = 0 ; j < ZG->aps.out_frag.total_blocks_num; ++j)
  {
    if (param == ZG->aps.out_frag.block_ref[j])
    {
      packet_found = ZB_TRUE;
      break;
    }
  }
  if (packet_found)
  {
    TRACE_MSG(TRACE_APS2, "param [%hd] found in fragmented queue: idx = %d, cleanup...", (FMT__H_D, param, j));

    for ( j = 0 ; j < ZG->aps.out_frag.total_blocks_num; ++j)
    {
      zb_uint8_t block_ref_j = ZG->aps.out_frag.block_ref[j];
      if (block_ref_j == 0U)
      {
        continue;
      }
      block_ref_j_busy = ZB_FALSE;
      for ( i = 0 ; i < ZB_N_APS_RETRANS_ENTRIES ; ++i)
      {
        if (block_ref_j == ZG->aps.retrans.hash[i].buf)
        {
          TRACE_MSG(TRACE_APS2, "clean retrans entry for buf ref %hd with idx=%d", (FMT__H_D, ZG->aps.retrans.hash[i].buf, i));
          if (aps_retrans_ent_busy_state(i))
          {
            block_ref_j_busy = ZB_TRUE;
          }
          aps_retrans_ent_free(i);
        }
      }

      if (block_ref_j_busy)
      {
        TRACE_MSG(TRACE_APS4, "bufid %hd idx %hd - busy", (FMT__H_H, block_ref_j, j));
      }
      else
      {
        TRACE_MSG(TRACE_APS4, "free buf entry in out_frag_q for buf ref %hd with idx=%d", (FMT__H_D, block_ref_j, j));
        zb_buf_free(block_ref_j);
      }
    }
    TRACE_MSG(TRACE_APS2, "reset frag tx queue!", (FMT__0));
    ZB_BZERO(ZG->aps.out_frag.block_ref, ZB_ARRAY_SIZE(ZG->aps.out_frag.block_ref));
    ZG->aps.out_frag.total_blocks_num = 0;
    ZG->aps.out_frag.aps_counter = 0;
    ZG->aps.out_frag.current_window = 0;
    ZG->aps.out_frag.retry_count = 0;
    ZG->aps.out_frag.state = APS_FRAG_INACTIVE;
    TRACE_MSG(TRACE_APS4, "free main out_frag.data_param bufid %hd", (FMT__H, ZG->aps.out_frag.data_param));
    /* Reassign param to be passeed to aps_send_fail_confirm() */
    param = ZG->aps.out_frag.data_param;
    ZG->aps.out_frag.data_param = 0;

#ifdef SNCP_MODE
    /* disable auto turbo poll for fragmented APS tx */
    sncp_auto_turbo_poll_aps_tx(ZB_FALSE);
#endif
    aps_out_frag_schedule_queued_requests();
  }
  else
  {
    TRACE_MSG(TRACE_APS2, "OK: param %hd not found in out_frag_q:", (FMT__H, param));
    for ( j = 0 ; j < ZG->aps.out_frag.total_blocks_num; ++j)
    {
      TRACE_MSG(TRACE_APS2, "out_frag_q(%d) ref %hd", (FMT__D_H, j, ZG->aps.out_frag.block_ref[j]));
    }
    for ( i = 0 ; i < ZB_N_APS_RETRANS_ENTRIES ; ++i)
    {
      TRACE_MSG(TRACE_APS2, "ZG->aps.retrans.hash(%d) ref %hd", (FMT__D_H, i, ZG->aps.retrans.hash[i].buf));
    }
  }
  TRACE_MSG(TRACE_APS2, "-zb_check_frag_queue_and_cleanup", (FMT__0));
#endif /* APS_FRAGMENTATION */
  aps_send_fail_confirm(param, status);
}

#ifndef ZB_LITE_APS_DONT_TX_PACKET_TO_MYSELF
/* Check if we must send packet to ourself (long addresses compared, result in selfie_packet boolean) */
static zb_bool_t zb_apsde_check_selfie(zb_uint16_t addr_short)
{
  zb_ieee_addr_t m_ieee_addr,m_ieee_addr2;
  zb_address_ieee_ref_t addr_ref;
  zb_bool_t selfie_packet = ZB_FALSE;

  if (zb_address_ieee_by_short( addr_short, m_ieee_addr ) == RET_OK){
    TRACE_MSG(TRACE_APS1, "new apsreq dst_addr = " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(m_ieee_addr)));
    if (zb_address_by_short(ZB_PIBCACHE_NETWORK_ADDRESS(), ZB_FALSE, ZB_FALSE, &addr_ref) == RET_OK)
    {
      zb_address_ieee_by_ref((zb_uint8_t *)&m_ieee_addr2,addr_ref);
      TRACE_MSG(TRACE_APS1, "new apsreq my_ieee  = " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(m_ieee_addr2)));
      if ( ZB_IEEE_ADDR_CMP(m_ieee_addr, m_ieee_addr2) )
      {
        selfie_packet=ZB_TRUE;
      } /* if ZB_IEEE_ADDR_CMP(m_ieee_addr, m_ieee_addr2)*/
    } /* if ieee_by_short == RET_OK */
  } /* if ieee_by_short == RET_OK */
  return selfie_packet;
}
#endif

/*
  Devices where nwkUseMulticast is set to TRUE, shall never set the delivery mode
  of an outgoing frame to 0b11. In this case, the delivery mode of the outgoing
  frame shall be set to 0b10 (broadcast) and the frame shall be sent using an NLDE-
  DATA.request with the destination address mode set to group addressing.
 */
static void zb_apsde_data_request_main(zb_uint8_t param)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t fc = 0;
  zb_nlde_data_req_t nldereq;
  zb_apsde_data_req_t *apsreq = ZB_BUF_GET_PARAM(param, zb_apsde_data_req_t);
  /* Added as a fix for sending group commands to local ep */
  zb_uint8_t apsreq_addr_mode = apsreq->addr_mode;

#if defined ZB_CERTIFICATION_HACKS && defined APS_FRAGMENTATION
  /* NOTE: It is supposed that APS fragmentation is not used without ZB_APSDE_TX_OPT_ACK_TX.
   * Otherwise, it is required to initialize the variable properly. */
  zb_uint8_t block_num = 0;
#endif

  TRACE_MSG(TRACE_APS1, "+apsde_data_req_main %hd, tx_options 0x%hd", (FMT__H_H, param, apsreq->tx_options));

  ZB_ASSERT(param);

/* Check that passed buffer not used in fragmented queue */
#ifdef APS_FRAGMENTATION
  if ( (apsreq->tx_options & ZB_APSDE_TX_OPT_FRAG_PERMITTED) == 0U ) /* this transaction is not fragmented */
  {
    zb_uint8_t j;

    TRACE_MSG(TRACE_APS1, "CHECK OUT_FRAG Q: aps.out_frag.total_blocks_num %hd", (FMT__H, ZG->aps.out_frag.total_blocks_num));
    for (j = 0; j < ZG->aps.out_frag.total_blocks_num; ++j)
    {
      if (ZG->aps.out_frag.block_ref[j] == param)
      {
        /* Fragments of one APS frame should have same APS counter */
        TRACE_MSG(TRACE_APS1, "found dirty fragmented queue: aps_counter %hd", (FMT__H, param));
        ZB_ASSERT(0);
        break;
      }
    }
  }
#endif  /* #ifdef APS_FRAGMENTATION */
#ifdef ZB_USEALIAS
  TRACE_MSG(TRACE_APS3, "apsreq->use_alias %hd", (FMT__H, apsreq->use_alias));
  TRACE_MSG(TRACE_APS3, "apsreq->alias_src_addr 0x%hx", (FMT__H, apsreq->alias_src_addr));
  TRACE_MSG(TRACE_APS3, "apsreq->alias_seq_num %hd", (FMT__H, apsreq->alias_seq_num));
#endif
/*
  That macros are implied (set values to 0):
  ZB_APS_FC_SET_FRAME_TYPE(fc, ZB_APS_FRAME_DATA);
  ZB_APS_FC_SET_ACK_FORMAT(fc, 0);
  ZB_APS_FC_SET_EXT_HDR_PRESENT(fc, 0);
*/
  nldereq.radius = apsreq->radius;
  nldereq.addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
  nldereq.nonmember_radius = 0; /* if multicast, get it from APS IB */
#ifdef ZB_CERTIFICATION_HACKS
  if (ZB_CERT_HACKS().disable_discovery_route)
  {
    nldereq.discovery_route = 0;
  }
  else
#endif  /* ZB_CERTIFICATION_HACKS */
  {
    nldereq.discovery_route = 1;  /* always! see 2.2.4.1.1.3 */
  }
  /* use NWK security only if not use APS security */

  TRACE_MSG(TRACE_APS3, "apsde_data_req_main addr mode=%hd", (FMT__H, apsreq_addr_mode));

  /* This flag is needed to increase diagnostics counter average_mac_retry_per_aps_message_sent */
  zb_buf_flags_or(param, ZB_BUF_HAS_APS_PAYLOAD);

  if (apsreq_addr_mode == ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT )
  {
    /* call this function and check is there are binded entries for the DST_ADDR present
       if there are no more entries, confirm */
    ret = zb_process_bind_trans(param);
    apsreq_addr_mode = apsreq->addr_mode;
    if (ret != RET_OK)
    {
      goto done;
   }
    /* else process each packet, prepared in zb_process_bind_trans */
  }

/* 14/06/2016 CR [AEV] start */
#ifdef ZB_USEALIAS
  /*
    If the UseAlias parameter has the value of TRUE, and the Acknowledged transmission
    field of the TxOptions parameter is set to 0b1, then the APSDE issues the
    APSDE-DATA.confirm primitive with a status of NOT_SUPPORTED.
  */
  /* 15-02014-005-GP_Errata_for_GP Basic_specification_14-0563.docx:
     If the UseAlias parameter has the value of TRUE, and the Acknowledged transmission field of the
     TxOptions parameter is set to 0b1, then the Acknowledged transmission field of the TxOptions
     parameter SHALL be ignored.
   */
  if ( ( apsreq->tx_options & ZB_APSDE_TX_OPT_ACK_TX ) && ( apsreq->use_alias==ZB_TRUE ) )
  {
    /* zb_apsde_data_acknowledged wants APS hdr */
    apsreq->tx_options &= (~ZB_APSDE_TX_OPT_ACK_TX);
  }

#endif
  /* 14/06/2016 CR [AEV] end */

#if defined ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ && defined ZB_SUSPEND_APSDE_DATAREQ_BY_SUBGHZ_CLUSTER
  if (ZCL_CTX().subghz_ctx.cli.suspend_zcl_messages)
  {
    TRACE_MSG(TRACE_APS1, "Transmission suspended by SubGHz cluster", (FMT__0));
    ret = RET_CANCELLED;
    goto done;
  }
#endif

  switch ((int)apsreq_addr_mode)
  {
    case ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT:
    {
#ifdef ZB_PRO_STACK
      /* should be processed before switch-case */
      TRACE_MSG(TRACE_APS3, "case ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT", (FMT__0));
      ZB_ASSERT(0);

#endif /* ZB_PRO_STACK */
      break;
    }
    case ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT:
    {
      TRACE_MSG(TRACE_APS3, "case ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT", (FMT__0));

      /* group addressing: NWK or APS multicast comunication;
       * NWK (nwk_use_multicast == 1): NWK dstAddr - group addr, MAC - broadcast;
       * APS (nwk_use_multicast == 0): in APS header - group address; NWK - broadcast;
       */
#ifdef ZB_PRO_STACK
#ifndef ZB_NO_NWK_MULTICAST
      if (ZB_NIB().nwk_use_multicast)
      {
/* #AT */
#ifdef ZB_TEST_PROFILE
        /* Check for Test Profile 2 */
        {
          zb_bool_t is_test_profile = ZB_FALSE;
          if (apsreq->profileid == ZB_TEST_PROFILE_ID)
          {
            if ( IS_CLUSTERID_TEST_PROFILE2(apsreq->clusterid) )
            {
              TRACE_MSG(TRACE_APS3, "its test profile", (FMT__0));
              is_test_profile = ZB_TRUE;
            }
          }
          if (!is_test_profile)
          {
            apsreq->dst_endpoint = 0xFF;
          }
        }
#else  /* test profile */
        apsreq->dst_endpoint = 0xFF;
#endif /* ZB_TEST_PROFILE */

        nldereq.dst_addr = apsreq->dst_addr.addr_short;
        nldereq.addr_mode = ZB_ADDR_16BIT_MULTICAST;
        nldereq.nonmember_radius = ZB_AIB().aps_nonmember_radius;
        ZB_APS_FC_SET_DELIVERY_MODE(fc, ZB_APS_DELIVERY_BROADCAST);
        TRACE_MSG(TRACE_APS3, "apsde_data nwk multicast, dst 0x%x", (FMT__D, nldereq.dst_addr));
      }
      else
      {
#endif  /* !ZB_NO_NWK_MULTICAST */
#ifdef ZB_CERTIFICATION_HACKS
        if (ZB_CERT_HACKS().aps_mcast_addr_overridden)
        {
          nldereq.dst_addr = ZB_CERT_HACKS().aps_mcast_nwk_dst_addr;
        }
        else
        {
#endif
#ifdef ZB_APS_ENABLE_GROUP_BROADCASTING
          /* Ask MV for details */
          nldereq.dst_addr = ZB_NWK_BROADCAST_ALL_DEVICES;
#else
          /* See sub-clause 2.2.4.1.1.1 */
          nldereq.dst_addr = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
#endif
#ifdef ZB_CERTIFICATION_HACKS
        /* MISRA Rule 16.1 - Additional blocks inside the case clause block are not allowed. */
        }
#endif /* ZB_CERTIFICATION_HACKS */
        ZB_APS_FC_SET_DELIVERY_MODE(fc, ZB_APS_DELIVERY_GROUP);
        TRACE_MSG(TRACE_APS3, "apsde_data aps multicast, dst 0x%x", (FMT__D, nldereq.dst_addr));
#ifndef ZB_NO_NWK_MULTICAST
      /* MISRA Rule 16.1 - Additional blocks inside the case clause block are not allowed. */
      }
#endif /* !ZB_NO_NWK_MULTICAST */
      apsreq->tx_options &= (zb_uint8_t)(~(zb_uint16_t)ZB_APSDE_TX_OPT_ACK_TX);
#else
      nldereq.dst_addr = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
      nldereq.nonmember_radius = ZB_AIB().aps_nonmember_radius;
      apsreq->tx_options &= (zb_uint8_t)(~(zb_uint16_t)ZB_APSDE_TX_OPT_ACK_TX);
      ZB_APS_FC_SET_DELIVERY_MODE(fc, ZB_APS_DELIVERY_GROUP);
      TRACE_MSG(TRACE_APS3, "apsde_data group, group %d, options 0x%hx", (FMT__D_H, apsreq->dst_addr.addr_short, apsreq->tx_options));
#endif /* ZB_PRO_STACK */
      break;
    }

    case ZB_APS_ADDR_MODE_16_ENDP_PRESENT:
      /* unicast - address is known*/
      TRACE_MSG(TRACE_APS3, "case ZB_APS_ADDR_MODE_16_ENDP_PRESENT", (FMT__0));

      if (apsreq->dst_addr.addr_short == ZB_NWK_BROADCAST_ALL_DEVICES ||
          apsreq->dst_addr.addr_short == ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE ||
          apsreq->dst_addr.addr_short == ZB_NWK_BROADCAST_ROUTER_COORDINATOR ||
          apsreq->dst_addr.addr_short == ZB_NWK_BROADCAST_LOW_POWER_ROUTER)
      {
        ZB_APS_FC_SET_DELIVERY_MODE(fc, ZB_APS_DELIVERY_BROADCAST);
      }
      else
      {
        ZB_APS_FC_SET_DELIVERY_MODE(fc, ZB_APS_DELIVERY_UNICAST);
      }
      nldereq.dst_addr = apsreq->dst_addr.addr_short;
      TRACE_MSG(TRACE_APS3, "apsde_data unicast, dst 0x%x, dst_ep %hd, options 0x%hx", (FMT__D_D_H, nldereq.dst_addr, apsreq->dst_endpoint, apsreq->tx_options));
      break;

    case ZB_APS_ADDR_MODE_64_ENDP_PRESENT:
      TRACE_MSG(TRACE_APS3, "case ZB_APS_ADDR_MODE_64_ENDP_PRESENT", (FMT__0));
      ZB_APS_FC_SET_DELIVERY_MODE(fc, ZB_APS_DELIVERY_UNICAST);
      /* convert long (64) to short (16) address, then unicast */
      nldereq.dst_addr = zb_address_short_by_ieee(apsreq->dst_addr.addr_long);
      if (nldereq.dst_addr != ZB_UNKNOWN_SHORT_ADDR)
      {
        /* Address found - continue */
        apsreq->dst_addr.addr_short = nldereq.dst_addr;
        /* FIXME: unclear update merged from LCGW. It will send packet correctly, but will set wrong mode in apsde-data.confirm. */
        apsreq->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
        apsreq_addr_mode = apsreq->addr_mode;
      }
      else
      {
        if (zb_buf_get_out_delayed_ext(zb_zdo_initiate_nwk_addr_req_2param, param, 0) != RET_OK)
        {
          ret = ERROR_CODE(ERROR_CATEGORY_APS,ZB_APS_STATUS_TABLE_FULL);
          goto done;
        }
        TRACE_MSG(TRACE_APS1, "-apsde_data_req: param %hd try to search nwk addr by IEEE " TRACE_FORMAT_64, (FMT__H_A, param, TRACE_ARG_64(apsreq->dst_addr.addr_long)));
        return;
      }
      TRACE_MSG(TRACE_APS3, "apsde_data unicast, dst %x, options 0x%hx", (FMT__D_H, nldereq.dst_addr, apsreq->tx_options));
      break;

    default:
      TRACE_MSG(TRACE_ERROR, "buf %hd strange addr mode 0x%x", (FMT__H_D, param, apsreq_addr_mode));
      ZB_ASSERT(0);
      break;
  }
#ifdef APS_FRAGMENTATION
  /* Zigbee-16-05033-011 SE spec, 5.3.8 APS Fragmentation Parameters:

     It is highly recommended all devices first query the Node Descriptor of the device it will
     communicate with to determine the Maximum Incoming Transfer Size (if ASDU size is greater
     than 128 bytes). This will establish the largest ASDU that can be supported with
     fragmentation.
     The sending device must use a message size during fragmentation that is smaller than this
     value.
 */

  if (ZB_APS_FC_GET_DELIVERY_MODE(fc) == ZB_APS_DELIVERY_UNICAST
      && ZB_BIT_IS_SET(apsreq->tx_options, ZB_APSDE_TX_OPT_FRAG_PERMITTED)
      /* Do not block TCLK: device will also send Node Desc Req during TCLK */
      && !ZB_TCPOL().waiting_for_tclk_exchange
      /*cstat !MISRAC2012-Rule-13.5 */
      /* After some investigation, the following violation of Rule 13.5 seems to be
       * a false positive. There are no side effect to 'zb_aps_get_max_trans_size()'. This
       * violation seems to be caused by the fact that 'zb_aps_get_max_trans_size()' is an
       * external function, which cannot be analyzed by C-STAT. */
      && zb_aps_get_max_trans_size(nldereq.dst_addr) == ZB_APS_INVALID_MAX_TRANS_SIZE)
  {
    TRACE_MSG(TRACE_APS1, "profileid %d clusterid %d APS TX to be fragmented - gen max payload size via node_desc_req", (FMT__D_D, apsreq->profileid, apsreq->clusterid));

    ret = zb_buf_get_out_delayed_ext(zb_zdo_init_node_desc_req_2param, param, 0);
    if (ret != RET_OK)
    {
      /* ERROR - can not discovery max transfer size */
      TRACE_MSG(TRACE_ERROR, "ERROR - can not discovery max transfer size, ret %d", (FMT__D, ret));
      zb_check_frag_queue_cleanup_and_send_fail_confirm(param, RET_NO_MEMORY);
    }
    /* we will return here after zb_zdo_init_node_desc_req_2param completion so return */
    return;
  }

  if ( ( apsreq->tx_options & ZB_APSDE_TX_OPT_ACK_TX ) != 0U &&
       ( apsreq->tx_options & ZB_APSDE_TX_OPT_FRAG_PERMITTED ) != 0U &&
       ZB_APS_FC_GET_DELIVERY_MODE(fc) == ZB_APS_DELIVERY_UNICAST
    )
  {
    ZB_APS_FC_SET_EXT_HDR_PRESENT(fc, 1U);

    /* TODO: move this into block preparation? */
  }
  /* check for incompatible fragmented TX options and pass up error */
  else if ((apsreq->tx_options & ZB_APSDE_TX_OPT_FRAG_PERMITTED) != 0U)
  {
    TRACE_MSG(TRACE_ERROR, "incompatible fragmentation tx option addr_mode = 0x%x", (FMT__D, apsreq_addr_mode));
    zb_check_frag_queue_cleanup_and_send_fail_confirm(param, ret);
    return;
  }
  else
  {
    /* MISRA rule 15.7 requires empty 'else' branch. */
  }

#endif /* APS_FRAGMENTATION */

  if(ZB_APS_FC_GET_DELIVERY_MODE(fc)==ZB_APS_DELIVERY_BROADCAST)
  {
    ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_APS_TX_BCAST_ID);
  }

  /* 14/06/2016 CR [AEV] start */
  /* 2.2.4.1.1.3 */
  /* If one or more binding table entries are found, then the APSDE examines
      the destination address information in each binding table entry.
      If this indicates a device itself, then the APSDE SHALL
      issue an APSDE-DATA.indication primitive to the next higher layer with the
      DstEndpoint parameter set to the destination endpoint identifier in the binding
      table entry.
   */
  /* this packet is from me to me */
#ifndef ZB_LITE_APS_DONT_TX_PACKET_TO_MYSELF
  /* Check if we must send packet to ourself (long addresses compared, result in selfie_packet boolean) */
  if (zb_apsde_check_selfie(apsreq->dst_addr.addr_short) == ZB_TRUE)
  {
    {
      TRACE_MSG(TRACE_APS3, "ack requested %hd, used retx ents %hd - don't use ack req",
                (FMT__H_H, (zb_uint8_t)(!!(apsreq->tx_options & ZB_APSDE_TX_OPT_ACK_TX)),
                 ZG->aps.retrans.n_packets));
      /* fill header for network layer */
      aps_data_hdr_fill_datareq(fc, apsreq, param);
    }
    TRACE_MSG(TRACE_APS1, "My Address, raise packet %hd to myself", (FMT__H, param));
    if (zb_buf_get_out_delayed_ext(zb_aps_pass_local_msg_up, param, zb_buf_get_max_size(param)) != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "Oops - out of memory; can't send selfie_packet for buf %hd - drop it", (FMT__H, param));
			zb_buf_free(param);
    }
  }
  else /* not selfie_packet */
#endif   /* ZB_LITE_APS_DONT_TX_PACKET_TO_MYSELF */
  {
  /* Otherwise, if the binding table entries do not indicate
     the device itself, the APSDE constructs the APDU with the
     endpoint information from the binding table entry, if present,
     and uses the destination address information from the binding
     table entry when transmitting the frame via the NWK layer.
     If more than one binding table entry is present, then the APSDE
     processes each binding table entry as described above;
     until no more binding table entries remain.
  */

    /* The parameters UseAlias, AliasSrcAddr and AliasSeqNumb shall
       be used in the invocation of the NLDE-DATA.request primitive */
#ifdef ZB_USEALIAS
    nldereq.use_alias = apsreq->use_alias;
    nldereq.alias_src_addr = apsreq->alias_src_addr;
    nldereq.alias_seq_num = apsreq->alias_seq_num;
#endif
    /* 14/06/2016 CR [AEV] end */

    /* Filling header, prepare packet to the lower layer */

    /* Check payload length and if length == 0, then disable APS encryption */
#ifdef ZB_SEND_EMPTY_DATA_WITHOUT_APS_ENCRYPTION
    if ( zb_buf_len(param) == 0U && (apsreq->tx_options & ZB_APSDE_TX_OPT_SECURITY_ENABLED) != 0U )
    {
      TRACE_MSG(TRACE_APS1, "apsde_data: no payload, disable aps security", (FMT__0));
      apsreq->tx_options &= ZB_APSDE_TX_OPT_SECURITY_ENABLED^0xFFU;
    }
#endif
#ifdef ZB_ENABLE_SE
    /* SE+BDB: If APS security is enabled here and we do not have a key, drop pkt. */
    if (ZB_SE_MODE()
        && cluster_needs_aps_encryption(apsreq->src_endpoint, apsreq->clusterid)
        && (apsreq->tx_options & ZB_APSDE_TX_OPT_SECURITY_ENABLED))
    {
      /* FIXME: Hope we know short+ieee pair here... */
      zb_ieee_addr_t dst_ieee;
      if (zb_address_ieee_by_short(nldereq.dst_addr, dst_ieee) == RET_OK)
      {
        zb_aps_device_key_pair_set_t *aps_key = zb_secur_get_link_key_pair_set(dst_ieee, ZB_TRUE);
        if (!aps_key ||
            /* Need to encrypt only if key_source is CBKE */
            (aps_key && aps_key->key_source != ZB_SECUR_KEY_SRC_CBKE))
        {
          /* TRACE_MSG(TRACE_APS1, "apsde_data: no CBKE-verified key, disable aps security", (FMT__0)); */
          /* apsreq->tx_options &= ZB_APSDE_TX_OPT_SECURITY_ENABLED^0xFF; */
          /* Should not send this pkt, drop! */
          ret = ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_SECURITY_FAIL);
          goto done;
        }
      }
    }
#endif /* ZB_ENABLE_SE */

    ZB_APS_FC_SET_ACK_FORMAT(fc, 0U);

    if ((apsreq->tx_options & ZB_APSDE_TX_OPT_ACK_TX) != 0U)
    {
      zb_apsde_data_req_t req;
      zb_uint8_t ref = 0;
      zb_uint8_t rx_on_when_idle;

      ZB_MEMCPY(&req, apsreq, sizeof(zb_apsde_data_req_t));

      ZB_APS_FC_SET_ACK_REQUEST(fc, 1U);
      TRACE_MSG(TRACE_APS3, "param %hd aps ack req", (FMT__H, param));
#if defined ZB_CERTIFICATION_HACKS && defined APS_FRAGMENTATION
      block_num = apsreq->block_num;
#endif
      aps_data_hdr_fill_datareq(fc, apsreq, param);
      ret = save_ack_data(param, &req, &ref);
      if (ret != RET_OK)
      {
        TRACE_MSG(TRACE_APS1, "Could not save ack data - pass up confirm on %hd", (FMT__H, param));
        zb_check_frag_queue_cleanup_and_send_fail_confirm(param, ret);
        return;
      }

      /* Optimize traffic for ZED: if wait for ACK, send POLL sooner to be able to
       * retrive APS ACK. */
      TRACE_MSG(TRACE_ZDO3, "Call zb_zdo_pim_start_turbo_poll_packets 1", (FMT__0));
      zb_zdo_pim_start_turbo_poll_packets(1);

      rx_on_when_idle = zb_nwk_get_nbr_rx_on_idle_short_or_false(apsreq->dst_addr.addr_short);

      TRACE_MSG(TRACE_APS2, "schedule zb_aps_ack_timer_cb time %u", (FMT__D, ZB_N_APS_ACK_WAIT_DURATION(rx_on_when_idle)));
      //ZB_SCHEDULE_ALARM(zb_aps_ack_timer_cb, ref, ZB_N_APS_ACK_WAIT_DURATION(ZB_U2B(rx_on_when_idle)));
      if(zb_schedule_alarm(zb_aps_ack_timer_cb, ref, ZB_N_APS_ACK_WAIT_DURATION(ZB_U2B(rx_on_when_idle))) == RET_OVERFLOW)
      {
        aps_retrans_ent_free(ref);
        zb_buf_free(param);
        goto done;
      }
    }
    else
    {
      TRACE_MSG(TRACE_APS3, "ack requested %hd, used retx ents %hd - don't use ack req",
              (FMT__H_H, (zb_uint8_t)(!!(apsreq->tx_options & ZB_APSDE_TX_OPT_ACK_TX)),
               ZG->aps.retrans.n_packets));
      aps_data_hdr_fill_datareq(fc, apsreq, param);
    }/* if (apsreq->tx_options & ZB_APSDE_TX_OPT_ACK_TX) */

    ZB_CHK_ARR(ZB_BUF_BEGIN(param), 8); /* check hdr fill */

/* Was:
    If not secured at APS layer by NWK key, enable secure at NWK layer.
    nldereq.security_enable = !(param->u.hdr.encrypt_type & ZB_SECUR_NWK_ENCR);
    In >=r21 securing APS frames by NWK key is impossible, so always enable security.
    Note that NWK checks for authenticated state itself.
*/
    nldereq.security_enable = ZB_TRUE;

    nldereq.ndsu_handle = 0;

#if defined ZB_ROUTER_ROLE && defined ZB_APSDE_REQ_ROUTING_FEATURES
    nldereq.extension_flags = 0;
    if (ZB_BIT_IS_SET(apsreq->tx_options, ZB_APSDE_TX_OPT_FORCE_MESH_ROUTE_DISC)
        && ZB_BIT_IS_SET(apsreq->tx_options, ZB_APSDE_TX_OPT_FORCE_SEND_ROUTE_REC))
    {
      TRACE_MSG(TRACE_APS3, "It is impossible to set both specific routing features flags. Let's drop them", (FMT__0));
      ret = ERROR_CODE(ERROR_CATEGORY_APS,ZB_APS_STATUS_ILLEGAL_REQUEST);
      goto done;
    }
    else if (ZB_BIT_IS_SET(apsreq->tx_options, ZB_APSDE_TX_OPT_FORCE_MESH_ROUTE_DISC))
    {
      nldereq.extension_flags |= ZB_NLDE_OPT_FORCE_MESH_ROUTE_DISC;
      TRACE_MSG(TRACE_APS3, "Set ZB_NLDE_OPT_FORCE_MESH_ROUTE_DESC flag", (FMT__0));
    }
    else if (ZB_BIT_IS_SET(apsreq->tx_options, ZB_APSDE_TX_OPT_FORCE_SEND_ROUTE_REC))
    {
      nldereq.extension_flags |= ZB_NLDE_OPT_FORCE_SEND_ROUTE_REC;
      TRACE_MSG(TRACE_APS3, "Set ZB_NLDE_OPT_FORCE_SEND_ROUTE_REC flag", (FMT__0));
    }
    else
    {
      /* MISRA rule 15.7 requires empty 'else' branch. */
    }
#endif

    /* construct needed tail parameters for NWK layer*/
    ZB_MEMCPY(ZB_BUF_GET_PARAM(param, zb_nlde_data_req_t), &nldereq, sizeof(nldereq));

#ifndef ZB_LITE_APS_DONT_TX_PACKET_TO_MYSELF
    /* Check for our group table and transmit buffer to us locally */
    if (ZB_APS_FC_GET_DELIVERY_MODE(fc) == ZB_APS_DELIVERY_GROUP
        && ZG->aps.group.n_groups > 0U
        && !ZB_RING_BUFFER_IS_FULL(&ZG->aps.group.local_dup_q)
#ifdef ZB_PRO_STACK
        /*cstat !MISRAC2012-Rule-13.5 */
        /* After some investigation, the following violation of Rule 13.5 seems to be
         * a false positive. There are no side effect to 'zb_aps_is_in_group()'. This
         * violation seems to be caused by the fact that 'zb_aps_is_in_group()' is an
         * external function, which cannot be analyzed by C-STAT. */
        && zb_aps_is_in_group(apsreq->dst_addr.addr_short) != 0U
        && apsreq_addr_mode == ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT
#endif
    )
    {
      TRACE_MSG(TRACE_APS3, "its aps group packet, send packet to itself [%hd]", (FMT__H, param));
      /* Must send this packet to myself as well */
      ZB_RING_BUFFER_PUT(&ZG->aps.group.local_dup_q, param);
      ret = zb_buf_get_in_delayed(zb_aps_pass_local_group_pkt_up);
    }
    else
#endif  /* ZB_LITE_APS_DONT_TX_PACKET_TO_MYSELF */
    {
      /* Now send (pass to NWK). If address got from the bind, send more then once (?).  */
      /* Add one more addr lock for dst (if it is unicast).
         Rationale: This addr may be released during the transmission (ed aging etc). */
      if (ZB_APS_FC_GET_DELIVERY_MODE(fc) == ZB_APS_DELIVERY_UNICAST)
      {
        zb_address_ieee_ref_t addr_ref;
        /* It is unusual, but still possible that addr is not in map here. In that case lets
         * create it and add lock (hope that if this address is found or not - in any case - we will
         * finally come to confirm which will release that lock). */
        ret = zb_address_by_short(nldereq.dst_addr, /* create */ ZB_TRUE, /* lock */ ZB_TRUE, &addr_ref);
        ZB_ASSERT(ret == RET_OK);               /* Should be OK! */
      }
      TRACE_MSG(TRACE_APS1, "Schedule packet to NWK", (FMT__0));
#if defined APS_FRAGMENTATION && defined ZB_CERTIFICATION_HACKS
      if (ZB_CERT_HACKS().frag_skip_0_and_2nd_block_1st_time &&
          ZG->aps.out_frag.total_blocks_num > 0U &&
          (block_num == ZG->aps.out_frag.total_blocks_num || block_num == 2U))
      {
        TRACE_MSG(TRACE_APS1, "ZB_CERT_HACKS().frag_skip_0_and_2nd_block_1st_time, apsreq->block_num %hd", (FMT__H, apsreq->block_num));
      }
      else
#endif /*  APS_FRAGMENTATION && defined ZB_CERTIFICATION_HACKS */
      {
        ZB_SCHEDULE_CALLBACK(zb_nlde_data_request, param);
      }
    } /* if ZB_APS_FC_GET_DELIVERY_MODE(fc) == ZB_APS_DELIVERY_GROUP */

  } /* if (selfie_packet==ZB_TRUE) else end */

done:
  if (ret != RET_OK)
  {
    zb_check_frag_queue_cleanup_and_send_fail_confirm(param, ret);
  }

  TRACE_MSG(TRACE_APS1, "-apsde_data_req_main (status:%hd [0=SUCCESS, see aps_status])", (FMT__H, ret));
}

#ifdef APS_FRAGMENTATION

static zb_bool_t check_is_param_in_out_chain(zb_uint8_t param)
{
  zb_bool_t ret = ZB_FALSE;
  /* detect what no active fragmented transfer active */
#ifdef APS_FRAGMENTATION
  if (ZG->aps.out_frag.total_blocks_num != 0U)
  {
    zb_uint8_t j;
    for (j = 0; j < ZG->aps.out_frag.total_blocks_num; ++j)
    {
      if (ZG->aps.out_frag.block_ref[j] == param)
      {
        ret = ZB_TRUE;
      }
    }
  }
#endif
  return ret;
}


static zb_bool_t check_is_param_need_to_be_fragmented(zb_uint8_t param, zb_uint8_t dst_max_in, zb_uint8_t dst_max_buf_size)
{
  zb_bool_t ret = ZB_FALSE;
  zb_uint16_t buf_len = (zb_uint16_t)zb_buf_len(param);

  TRACE_MSG(TRACE_APS2, ">> APSF: check_is_param_need_to_be_fragmented %hd", (FMT__H, param));
#ifdef APS_FRAGMENTATION
  TRACE_MSG(TRACE_APS2, "APSF: dst_max_in = %hd, buf_len() = %hd, dst_buf_max_size = %hd", (FMT__H_H_H, dst_max_in, buf_len, dst_max_buf_size));
  DUMP_TRAF("APSF: DATA CHECK:", zb_buf_begin(param), buf_len, 0);
  if ((buf_len > (zb_uint16_t)dst_max_in + 2U) && ((zb_uint16_t)dst_max_in > 0U))
  {
    ret = ZB_TRUE;
  }
/*  if((buf_len <= dst_max_buf_size-ZB_APS_HEADER_MAX_LEN)&&(dst_max_in<dst_max_buf_size-ZB_APS_HEADER_MAX_LEN-2))
  {
    TRACE_MSG(TRACE_APS2, "<< APSF: dst_max_buf_size-ZB_APS_HEADER_MAX_LEN = %hd, we will fit in non-frag packet, will try", (FMT__H, dst_max_buf_size-ZB_APS_HEADER_MAX_LEN));
    ret = ZB_FALSE;
  }*/
#endif
  TRACE_MSG(TRACE_APS2, "<< APSF: check_is_param_need_to_be_fragmented %hd", (FMT__H, ret));
  return ret;
}


static zb_bool_t can_start_new_fragmented_transmission(void)
{
  return (zb_bool_t)(ZG->aps.out_frag.state == APS_FRAG_INACTIVE
    /* In order to avoid races, we should wait until block transmision procedure is complete
       (it can hang for a while after the previous transmission is completed in case of OOM) */
    && !ZG->aps.out_frag.transmission_is_scheduled);
}

static void aps_out_frag_schedule_queued_requests(void)
{
  zb_bufid_t buf;

  if (ZB_RING_BUFFER_IS_EMPTY(&ZG->aps.out_frag_q)
      || !can_start_new_fragmented_transmission())
  {
    /* We cannot schedule requests at the moment */
    TRACE_MSG(TRACE_APS4, "aps_out_frag_schedule_queued_requests cannot schedule request buf_is_empty %hd can_start_new_frag_transm() %hd",
            (FMT__H_H, ZB_RING_BUFFER_IS_EMPTY(&ZG->aps.out_frag_q), can_start_new_fragmented_transmission()));
    return;
  }

  buf = *ZB_RING_BUFFER_GET(&ZG->aps.out_frag_q);
  ZB_RING_BUFFER_FLUSH_GET(&ZG->aps.out_frag_q);
  ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, buf);
  TRACE_MSG(TRACE_APS4, "aps_out_frag_schedule_queued_requests next frag trans scheduled! buf %hd", (FMT__H, buf));
}


/*
   Calculates maximum possible APS payload for a non-fragmented data transmission
*/
static zb_uint8_t aps_max_nonfragmented_payload(zb_apsde_data_req_t *req)
{
  /* TODO: make the calculation more explicit, also it should take into account the specifics of
     the destination (e.g. the need for source routing) */

  if (ZB_NIB_SECURITY_LEVEL() != 0U && (req->tx_options & ZB_APSDE_TX_OPT_SECURITY_ENABLED) != 0U )
  {
    /* We need APS-level encryption */
    return ZB_APS_GUARANTEED_PAYLOAD_SIZE_WO_SECURITY - ZB_APS_MAX_APS_SECURITY_SIZE;
  }

  return ZB_APS_GUARANTEED_PAYLOAD_SIZE_WO_SECURITY;
}

#endif  /* APS_FRAGMENTATION */

/* function supporting fragmentation in se-mode */
void zb_apsde_data_request(zb_uint8_t param)
{
#ifdef APS_FRAGMENTATION
  zb_apsde_data_req_t *apsde_req = ZB_BUF_GET_PARAM(param, zb_apsde_data_req_t);
  zb_uint8_t dst_max_in;
  zb_uint16_t short_addr;
  zb_uint16_t dst_max_trans_size;
  zb_uint16_t buf_len = (zb_uint16_t)zb_buf_len(param);
#endif

#if defined ZB_ENABLE_INTER_PAN_NON_DEFAULT_CHANNEL
  /* Put this packet to queue if Inter-Pan procedure is enabled and in progress */
  if (ZB_APS_INTRP_MCHAN_FUNC_SELECTOR().put_aps_packet_to_queue != NULL)
  {
    if (ZB_APS_INTRP_MCHAN_FUNC_SELECTOR().put_aps_packet_to_queue(param) == RET_OK)
    {
      return;
    }
  }
#endif /* defined ZB_ENABLE_INTER_PAN_NON_DEFAULT_CHANNEL */

#ifndef APS_FRAGMENTATION
  /* forward cmd to standart zb_apsde_data_request_main */
  zb_apsde_data_request_main(param);
  return;
#else

  TRACE_MSG(TRACE_APS1, "+zb_apsde_data_request frag machine param %hd state %hd", (FMT__H_H, param, ZG->aps.out_frag.state));

  if((0U == apsde_req->profileid) ||
     (apsde_req->tx_options & ZB_APSDE_TX_OPT_ACK_TX) == 0U ||
     (apsde_req->tx_options & ZB_APSDE_TX_OPT_FRAG_PERMITTED) == 0U)
  {
    TRACE_MSG(TRACE_APS2, "APSF: ZDO command (without APS ACK) or APSF is not permitted - pass further without parsing", (FMT__0));
    ZB_SCHEDULE_CALLBACK(zb_apsde_data_request_main, param);
    return;
  }

  if ((ZG->aps.out_frag.state > APS_FRAG_INACTIVE) && (check_is_param_in_out_chain(param)))
  {
    TRACE_MSG(TRACE_APS2, "APSF: another frag TX in progress: check_is_param_in_out_chain = true",(FMT__0));
    ZB_SCHEDULE_CALLBACK(zb_apsde_data_request_main, param);
    return;
  }

  if (buf_len <= aps_max_nonfragmented_payload(apsde_req))
  {
    apsde_req->tx_options &= ~ZB_APSDE_TX_OPT_FRAG_PERMITTED;
    TRACE_MSG(TRACE_APS2, "APSF: packet len guaranteed less than one fragment (%hd<%hd): schedule direct transmission without fragmentation.",(FMT__H_H, buf_len, ZB_APS_MAX_PAYLOAD_SIZE-13));
    ZB_SCHEDULE_CALLBACK(zb_apsde_data_request_main, param);
    return;
  }

  if (ZB_APS_ADDR_MODE_64_ENDP_PRESENT == apsde_req->addr_mode)
  {
    TRACE_MSG(TRACE_APS3, "frag TX ZB_APS_ADDR_MODE_64_ENDP_PRESENT", (FMT__0));
    /* convert long (64) to short (16) address, then unicast */
    short_addr = zb_address_short_by_ieee(apsde_req->dst_addr.addr_long);
    if (short_addr != ZB_UNKNOWN_SHORT_ADDR)
    {
      /* Address found - continue */
      apsde_req->dst_addr.addr_short = short_addr;
      apsde_req->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
    }
    else
    {
      if (zb_buf_get_out_delayed_ext(zb_zdo_initiate_nwk_addr_req_2param, param, 0) != RET_OK)
      {
        aps_send_fail_confirm(param, ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_INVALID_PARAMETER));
        TRACE_MSG(TRACE_APS1, "-zb_apsde_data_request frag machine param %hd zb_buf_get_out_delayed_ext failed", (FMT__H, param));
        return;
      }
      TRACE_MSG(TRACE_APS1, "-zb_apsde_data_request frag machine param %hd try to search nwk addr by IEEE " TRACE_FORMAT_64, (FMT__H_A, param, TRACE_ARG_64(apsde_req->dst_addr.addr_long)));
      return;
    }
  }
  else if (ZB_APS_ADDR_MODE_16_ENDP_PRESENT == apsde_req->addr_mode)
  {
    short_addr = apsde_req->dst_addr.addr_short;
  }
  else
  {
    /* 2019-03-06 CR:MAJOR [ZOI-12]  What about other modes - binding for example? We just broke it?
       All the stuff with max_transfer_size and fragmenting is only needed when:
       a) fragmentation is enabled (define)
       b) packet can be fragmented (it is not ZDO, APS ACK flag is set)
       c) packet is long enough and should be fragmented
       UPDATE: 07/31/2019 EE MAJOR Still can't send fragmented frame via binding, even selfy packet
    */
    TRACE_MSG(TRACE_APS1, "APSF: addr mode %hd not supported", (FMT__H, apsde_req->addr_mode));
    aps_send_fail_confirm(param, ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_INVALID_PARAMETER));
    return;
  }

  TRACE_MSG(TRACE_APS1, "APSF: packet to peer 0x%x, profile 0x%x", (FMT__D_D, short_addr, apsde_req->profileid));

  if(ZB_NWK_IS_ADDRESS_BROADCAST(short_addr))
  {
    TRACE_MSG(TRACE_APS2, "APSF: Broadcast - pass further without parsing", (FMT__0));
    ZB_SCHEDULE_CALLBACK(zb_apsde_data_request_main, param);
  }

  /* 2019-03-06 CR:MAJOR Do not need max_transfer_size if fragmentation is disabled! And probably
   * for some other cases. */

  /* Check for selfie packet */
  if (zb_apsde_check_selfie(short_addr) == ZB_TRUE)
  {
    TRACE_MSG(TRACE_APS1, "APSF: SELFIE: My Address, raise packet %hd to myself", (FMT__H, param));
    TRACE_MSG(TRACE_APS3, "ack requested %hd, used retx ents %hd - don't use ack req",
              (FMT__H_H, (zb_uint8_t)(!!(apsde_req->tx_options & ZB_APSDE_TX_OPT_ACK_TX)),
                ZG->aps.retrans.n_packets));
    /* fill header for network layer */
    aps_data_hdr_fill_datareq(0, apsde_req, param);

    if (zb_buf_get_out_delayed_ext(zb_aps_pass_local_msg_up, param, zb_buf_get_max_size(param)) != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "Oops - out of memory; can't send selfie_packet for buf %hd - drop it", (FMT__H, param));
			zb_buf_free(param);
    }
    return;
  }

  /* Check if we have peer max trans len */
  dst_max_trans_size = zb_aps_get_max_trans_size(short_addr);
  if (dst_max_trans_size == ZB_APS_INVALID_MAX_TRANS_SIZE)
  {
    /* if no, than schedule node descriptor request with param - our buffer */
    /* todo: block somehow further node descriptor requests if one in progress */
    zb_ret_t ret;
    TRACE_MSG(TRACE_APS2, "APSF: no info about buf len of peer: 0x%x", (FMT__D, short_addr));
    TRACE_MSG(TRACE_APS2, "APSF: request node descriptor from 0x%x...",(FMT__D, short_addr));
    ret = zb_buf_get_out_delayed_ext(zb_zdo_init_node_desc_req_2param, param, 0);
    if (ret != RET_OK)
    {
      /* ERROR - can not discovery max transfer size */
      TRACE_MSG(TRACE_ERROR, "APSF: ERROR - can not discovery max transfer size, fail operation, ret %d", (FMT__D, ret));
      aps_send_fail_confirm(param, RET_NO_MEMORY);
    }
    return;
  }

  /* Check if we fit in partner buffer */
  if (dst_max_trans_size < buf_len)
  {
    /* ERROR - we will not fit */
    TRACE_MSG(TRACE_ERROR, "APSF: ERROR - can not fit into partner buffer (%hd), send fail confirm", (FMT__H, zb_buf_len(param)));
    aps_send_fail_confirm(param, ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_ASDU_TOO_LONG));
    return;
  }

  dst_max_in = zb_aps_frag_payload_len(short_addr);
  if (dst_max_in == INVALID_FRAG_PAYLOAD_LEN)
  {
    /* ERROR - invalid payload length */
    TRACE_MSG(TRACE_ERROR, "APSF: ERROR - can not fit payload in the buffer of size %hd, send fail confirm",
              (FMT__H, zb_aps_get_max_buffer_size(short_addr)));
    aps_send_fail_confirm(param, ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_ASDU_TOO_LONG));
    return;
  }

  if (check_is_param_need_to_be_fragmented(param, dst_max_in, zb_aps_get_max_buffer_size(short_addr)))
  {
    /* Implement a queue of fragmented tx.
     */
    if (ZB_RING_BUFFER_IS_EMPTY(&ZG->aps.out_frag_q) && can_start_new_fragmented_transmission())
    {
      TRACE_MSG(TRACE_APS2, "APSF: check_is_param_need_to_be_fragmented = true, prepare fragmented transmission", (FMT__0));
      aps_prepare_frag_transmission(param, dst_max_in);
#ifdef SNCP_MODE
      /* enable auto turbo poll for fragmented APS transmission */
      sncp_auto_turbo_poll_aps_tx(ZB_TRUE);
#endif
      zb_aps_out_frag_schedule_send_next_block_delayed();
    }
    else if (!ZB_RING_BUFFER_IS_FULL(&ZG->aps.out_frag_q))
    {
      /* Put the request into aps frag tx queue, it will be started after the current outgoing
         fragmented transmission finishes. */
      TRACE_MSG(TRACE_APS2, "queued the fragmented apsde-data.request", (FMT__0));
      ZB_RING_BUFFER_PUT(&ZG->aps.out_frag_q, param);
    }
    else
    {
      /* We can neither handle nor queue the request. Report an error. */
      TRACE_MSG(TRACE_APS2, "we cannot handle the fragmented data request at the moment", (FMT__0));
      aps_send_fail_confirm(param, RET_NO_MEMORY);
    }
  }
  else
  {
    TRACE_MSG(TRACE_APS2, "APSF: check_is_param_need_to_be_fragmented = false, call zb_apsde_data_request_main", (FMT__0));
    apsde_req->tx_options &= ~ZB_APSDE_TX_OPT_FRAG_PERMITTED;
    ZB_SCHEDULE_CALLBACK(zb_apsde_data_request_main, param);
  }
  TRACE_MSG(TRACE_APS1, "-zb_apsde_data_request frag machine %hd", (FMT__H, param));
#endif
}


/* Send confirm with given status */
void aps_send_fail_confirm(zb_uint8_t param, zb_ret_t status)
{
  zb_apsde_data_req_t *apsreq = ZB_BUF_GET_PARAM(param, zb_apsde_data_req_t);
  zb_apsde_data_confirm_t apsde_data_conf;

  TRACE_MSG(TRACE_APS3, "aps_send_fail_confirm: param %hd status: %d", (FMT__H_D, param, status));
  /* DA: replace it with macro? */
  ZB_BZERO(&apsde_data_conf, sizeof(zb_apsde_data_confirm_t));
  apsde_data_conf.addr_mode = apsreq->addr_mode;
  apsde_data_conf.src_endpoint = apsreq->src_endpoint;
  apsde_data_conf.dst_endpoint = apsreq->dst_endpoint;
  apsde_data_conf.dst_addr = apsreq->dst_addr;
  apsde_data_conf.status = status;
  zb_buf_set_status(param, status);
  /* Pkt was not passed through apsde_data_req. */
  apsde_data_conf.need_unlock = ZB_FALSE;

  ZB_MEMCPY(ZB_BUF_GET_PARAM(param, zb_apsde_data_confirm_t), &apsde_data_conf, sizeof(zb_apsde_data_confirm_t));

  ZB_SCHEDULE_CALLBACK(zb_apsde_data_confirm, param);
  return;
}

static void aps_send_frame_via_binding(zb_uint8_t param)
{
  zb_uint8_t *body;
  zb_apsde_data_req_t *apsreq = ZB_BUF_GET_PARAM(param, zb_apsde_data_req_t);
  zb_aps_hdr_t aps_hdr;

  TRACE_MSG(TRACE_APS3, "+aps_send_frame_via_binding( %hd )", (FMT__H, param));

  zb_aps_hdr_parse(param, &aps_hdr, ZB_FALSE);
  ZB_BZERO(apsreq, sizeof(*apsreq));
  apsreq->profileid = aps_hdr.profileid;
  apsreq->clusterid = aps_hdr.clusterid;
  apsreq->src_endpoint = aps_hdr.src_endpoint;
  apsreq->addr_mode = ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT;
  if (ZB_APS_FC_GET_ACK_REQUEST(aps_hdr.fc) != 0U)
  {
    apsreq->tx_options |= ZB_APSDE_TX_OPT_ACK_TX;
  }
  ZB_APS_HDR_CUT_P(param, body);
  ZVUNUSED(body);

  ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, param);

  TRACE_MSG(TRACE_APS3, "data confirmed for the packet sent via binding %hd", (FMT__H, param));
  TRACE_MSG(TRACE_APS3, "-aps_send_frame_via_binding( %hd )", (FMT__H, param));
}

#ifdef APS_FRAGMENTATION

static zb_ret_t aps_prepare_frag_block(zb_bufid_t original_buf, zb_bufid_t target_buf, zb_uint8_t block_no)
{
  zb_ret_t ret = RET_OK;
  zb_apsde_data_req_t *apsde_req;
  zb_apsde_data_req_t *apsde_req_target;
  zb_uint16_t offset = (zb_uint16_t)ZG->aps.out_frag.dst_max_in * (zb_uint16_t)block_no;

  TRACE_MSG(TRACE_APS3, "+aps_prepare_frag_block orig %hd targ %hd block %hd",
            (FMT__H_H_H, original_buf, target_buf, block_no));

  if (offset >= zb_buf_len(original_buf))
  {
    ret = RET_INVALID_PARAMETER;
  }

  if (ret == RET_OK)
  {
    zb_uint8_t *dst_ptr;
    zb_uint16_t size = (zb_uint16_t)zb_buf_len(original_buf) - offset;

    if (size > ZG->aps.out_frag.dst_max_in)
    {
      size = ZG->aps.out_frag.dst_max_in;
    }

    dst_ptr = zb_buf_initial_alloc(target_buf, size);
    ZB_MEMCPY(dst_ptr, zb_buf_data(original_buf, offset), size);

    /* copy aps data req param */
    apsde_req = ZB_BUF_GET_PARAM(original_buf, zb_apsde_data_req_t);
    apsde_req_target = ZB_BUF_GET_PARAM(target_buf, zb_apsde_data_req_t);
    ZB_MEMCPY(apsde_req_target, apsde_req, sizeof(zb_apsde_data_req_t));

    apsde_req_target->extended_fc = 0;
    if (block_no == 0U)
    {
      ZB_APS_FC_SET_EXT_HDR_FRAG(apsde_req_target->extended_fc, ZB_APS_FIRST_FRAGMENT);
      apsde_req_target->block_num = ZG->aps.out_frag.total_blocks_num;
    }
    else
    {
      ZB_APS_FC_SET_EXT_HDR_FRAG(apsde_req_target->extended_fc, ZB_APS_NOT_FIRST_FRAGMENT);
      apsde_req_target->block_num = block_no;
    }

    TRACE_MSG(TRACE_APS3, "extended_fc %x", (FMT__D, apsde_req_target->extended_fc));

    apsde_req_target->tx_options |= ZB_APSDE_TX_OPT_ACK_TX | ZB_APSDE_TX_OPT_FRAG_PERMITTED;
  }

  return ret;
}

/*
   It is assumed that all necessary buffer length checks have already been done.
*/
static void aps_prepare_frag_transmission(zb_bufid_t buf, zb_uint8_t max_fragment_size)
{
  zb_apsde_data_req_t *apsreq = ZB_BUF_GET_PARAM(buf, zb_apsde_data_req_t);
  zb_uint_t blocks_num;

  ZB_ASSERT(max_fragment_size != INVALID_FRAG_PAYLOAD_LEN);
  ZB_ASSERT(max_fragment_size != 0U);
  ZB_BZERO(&ZG->aps.out_frag, sizeof(ZG->aps.out_frag));
  ZG->aps.out_frag.state = APS_FRAG_RECEIVED;
  ZG->aps.out_frag.data_param = buf;
  ZG->aps.out_frag.addr_mode = apsreq->addr_mode;
  if (max_fragment_size != 0U)
  {
      /* MISRA Rule 1.3
       * Possible division by zero. Actually, max_fragment_size can't be 0 because
       * zb_aps_frag_payload_len() returns values > 0. However, C-STAT can't deduct it.
       * Therefore, explicit check 'max_fragment_size != 0U' is added.
       */
    ZG->aps.out_frag.dst_max_in = max_fragment_size;
    blocks_num = (zb_buf_len(buf) + max_fragment_size - 1U) / max_fragment_size;
    if (blocks_num <= ZB_UINT8_MAX)
    {
      ZG->aps.out_frag.total_blocks_num = (zb_uint8_t)blocks_num;
    }
    else
    {
      TRACE_MSG(TRACE_ERROR, "Too many blocks (%u), doesn't fit in ZG->aps.out_frag.total_blocks_num",
                (FMT__D, blocks_num));
      ZG->aps.out_frag.total_blocks_num = 0;
    }

    TRACE_MSG(TRACE_APS3, "aps_prepare_frag_transmission buf %hd blocks %hd",
              (FMT__H_H, buf, ZG->aps.out_frag.total_blocks_num));
  }
}

static void zb_aps_out_frag_schedule_send_next_block_delayed(void)
{
  zb_ret_t ret;

  if (!ZG->aps.out_frag.transmission_is_scheduled)
  {
    ret = zb_buf_get_out_delayed(zb_aps_out_frag_schedule_send_next_block);
    if (ret != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
    }
    else
    {
      ZG->aps.out_frag.transmission_is_scheduled = ZB_TRUE;
    }

#ifdef ZB_APS_FRAG_FORCE_DISABLE_TURBO_POLL
    /* DL: If the frag window_size > 1, we don't know how many APS ACKs can arrive for one sended window.
     * For each sended fragmented packet, one turbo poll_packet is created from zb_apsde_data_request_main().
     * Before starting sending the next window, needs to stop the poll_packets to prevent uncontrolled
     * increase the turbo_poll_n_packets.
     */
    TRACE_MSG(TRACE_APS4, "stop turbo poll_packets before send next block", (FMT__0));
    zb_zdo_turbo_poll_packets_leave(0);
#endif
  }
}

static void zb_aps_out_frag_schedule_send_next_block(zb_uint8_t param)
{
  zb_uint8_t block_to_send_num;

#if 0
  /* uncomment to debug in transaction cleanup testing */
  {
    static int cnt = 0;
    cnt++;
    if (cnt == 11)
    {
      ZB_SCHEDULE_ALARM(zb_aps_out_frag_schedule_send_next_block, param, ZB_TIME_ONE_SECOND * 13);
      return;
    }
  }
#endif

  TRACE_MSG(TRACE_APS2, ">> zb_aps_out_frag_schedule_send_next_block %hd out_frag.total_blocks_num %hd", (FMT__H_H, param, ZG->aps.out_frag.total_blocks_num));
  if (ZG->aps.out_frag.total_blocks_num != 0U)
  {
    /* Skip ZG->aps.out_frag.current_window blocks - they are already acked */
    block_to_send_num = ZG->aps.out_frag.current_window * ZG->aps.aib.aps_max_window_size;
    TRACE_MSG(TRACE_APS2, "current_window %hd", (FMT__H, ZG->aps.out_frag.current_window));
    /* Go through blocks_acked_mask and select next block not acked yet (in current_window) */
    for (; block_to_send_num < ZG->aps.out_frag.total_blocks_num &&
           block_to_send_num < (ZG->aps.out_frag.current_window + 1U) * ZG->aps.aib.aps_max_window_size; ++block_to_send_num)
    {
      if (ZB_CHECK_BIT_IN_BIT_VECTOR(ZG->aps.out_frag.blocks_sent_mask, block_to_send_num) == 0U)
      {
        zb_ret_t ret;

        if (ZG->aps.out_frag.block_ref[block_to_send_num] == 0U && param != 0U)
        {

          ZG->aps.out_frag.block_ref[block_to_send_num] = param;
          ret = aps_prepare_frag_block(ZG->aps.out_frag.data_param, param, block_to_send_num);
          /* Ignore return code, assuming it always succeed. However, maybe it is not correct, see ZOI-437. */
          ZB_ASSERT(ret == RET_OK);
          param = 0;
        }

        if (ZG->aps.out_frag.block_ref[block_to_send_num] == 0U)
        {
          /* We need a buffer for the block transmission. */
          ret = zb_buf_get_out_delayed(zb_aps_out_frag_schedule_send_next_block);
          if (ret != RET_OK)
          {
            TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
          }
          ZG->aps.out_frag.transmission_is_scheduled = ZB_FALSE;
          return;
        }

        /* Schedule to send */
        ZB_SET_BIT_IN_BIT_VECTOR(ZG->aps.out_frag.blocks_sent_mask, block_to_send_num);

        /* TODO: Optimization: may not put every block into aps retrans - need one entry per window
         * (say, entry from last sent block in window). */
        if (ZB_CHECK_BIT_IN_BIT_VECTOR(ZG->aps.out_frag.blocks_retry_mask, block_to_send_num) == 0U)
        {
          TRACE_MSG(TRACE_APS2, "transmitting block %hd", (FMT__H, block_to_send_num));
          ZB_SCHEDULE_ALARM(zb_apsde_data_request_main,
                            ZG->aps.out_frag.block_ref[block_to_send_num],
                            ZB_MILLISECONDS_TO_BEACON_INTERVAL(ZG->aps.aib.aps_interframe_delay));
        }
        else
        {
          TRACE_MSG(TRACE_APS2, "retransmitting block %hd", (FMT__H, block_to_send_num));
          zb_zdo_pim_continue_turbo_poll();
          /* Spec, 2.2.8.4.5 :
           * If an apscAckWaitDuration timer expires, then the block with the lowest unacknowledged
           * block number shall be passed to the NWK data service again, and the retryCounter parameter
           * shall be incremented.

           * Looks like we can retransmit only one unacknowledged block in that case. */

          ZB_SCHEDULE_CALLBACK(zb_nlde_data_request, ZG->aps.out_frag.block_ref[block_to_send_num]);
          ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_APS_TX_UCAST_RETRY_ID);
        }

        ZB_SCHEDULE_ALARM(zb_aps_out_frag_schedule_send_next_block,
                          0,
                          ZB_MILLISECONDS_TO_BEACON_INTERVAL(ZG->aps.aib.aps_interframe_delay));

        ZB_SET_BIT_IN_BIT_VECTOR(ZG->aps.out_frag.blocks_sent_mask, block_to_send_num);

        if (param != 0U)
        {
          zb_buf_free(param);
        }

        TRACE_MSG(TRACE_APS2, "<< zb_aps_out_frag_schedule_send_next_block ret %hd", (FMT__H, RET_BUSY));

        return;
        /* FIXME: What if this send will fail? Reset it in blocks_sent_mask? */
      }
      else
      {
        TRACE_MSG(TRACE_APS3, "block %hd is sent", (FMT__H, block_to_send_num));
      }
    }
  }

  if (param != 0U)
  {
    zb_buf_free(param);
  }

  ZG->aps.out_frag.transmission_is_scheduled = ZB_FALSE;
  TRACE_MSG(TRACE_APS2, "<< zb_aps_out_frag_schedule_send_next_block ret %hd", (FMT__H, RET_OK));
}

#endif  /* APS_FRAGMENTATION */

#ifndef ZB_LITE_APS_DONT_TX_PACKET_TO_MYSELF
void zb_aps_pass_local_group_pkt_up(zb_uint8_t param)
{
  zb_uint8_t out_param;
  zb_aps_hdr_t *aps_hdr;
  TRACE_MSG(TRACE_APS1, "+zb_aps_pass_local_group_pkt_up", (FMT__0));

  if (!ZB_RING_BUFFER_IS_EMPTY(&ZG->aps.group.local_dup_q))
  {
    out_param = *ZB_RING_BUFFER_GET(&ZG->aps.group.local_dup_q);
    ZB_RING_BUFFER_FLUSH_GET(&ZG->aps.group.local_dup_q);
    zb_buf_copy(param, out_param);
    /* send original frame to nwk */
    ZB_SCHEDULE_CALLBACK(zb_nlde_data_request, out_param);

    /* Fill parsed aps hdr. Note that no nwk hdr in the buffer. */
    aps_hdr = ZB_BUF_GET_PARAM(param, zb_aps_hdr_t);
    zb_aps_hdr_parse(param, aps_hdr, ZB_FALSE);
    /* this packet is from me to me */
    aps_hdr->src_addr = ZB_PIBCACHE_NETWORK_ADDRESS();

    ZB_SCHEDULE_CALLBACK(zb_aps_pass_up_group_buf, param);
  }
  else
  {
    zb_buf_free(param);
    ZB_ASSERT(0);
  }
  TRACE_MSG(TRACE_APS1, "-zb_aps_pass_local_group_pkt_up", (FMT__0));
}
#endif  /* #ifndef ZB_LITE_APS_DONT_TX_PACKET_TO_MYSELF */

void zb_aps_pass_up_group_buf(zb_uint8_t param)
{
  zb_bufid_t new_buf;

  TRACE_MSG(TRACE_APS1, "+zb_aps_pass_up_group_buf param %hd", (FMT__H, param));

  new_buf = zb_buf_get(ZB_TRUE, 0);
  if (new_buf != 0U)
  {
    ZB_SCHEDULE_CALLBACK2(zb_aps_pass_group_msg_up, new_buf, param);
  }
  else
  {
    zb_nwk_unlock_in(param);
    if (zb_buf_get_out_delayed_ext(zb_aps_pass_group_msg_up, param, 0) != RET_OK)
    {
      ZB_SCHEDULE_CALLBACK2(zb_aps_pass_group_msg_up, 0, param);
    }
  }

  TRACE_MSG(TRACE_APS1, "-zb_aps_pass_up_group_buf", (FMT__0));
}


#ifndef ZB_LITE_APS_DONT_TX_PACKET_TO_MYSELF
/* 14/06/2016 CR [AEV] start */
/**
    Function generates new packet from source and pass it to higher layer with APSDE.DATA.indication callback.
    After this, if needed, zb_send_frame_via_binding called to continue trans_packet table processing

    @param newbuf - new allocated buffer
    @param param - source packet for forward to higher layer
 */

void zb_aps_pass_local_msg_up(zb_uint8_t newbuf, zb_uint16_t param)
{
  zb_apsde_data_req_t *apsreq;
  zb_uint8_t param_u8 = (zb_uint8_t)param;
  zb_apsde_data_indication_t *apsde_data_ind = ZB_BUF_GET_PARAM(newbuf, zb_apsde_data_indication_t);
  apsreq = ZB_BUF_GET_PARAM(param_u8, zb_apsde_data_req_t);
  /* Copy (param) buffer to new buffer ind_buf*/
  ZB_ASSERT(newbuf);
  zb_buf_copy(newbuf, param_u8);
  TRACE_MSG(TRACE_APS3, "+zb_aps_pass_local_msg_up", (FMT__0));

  /* Get FC from the beginning of the buffer */
  apsde_data_ind->fc = *(zb_uint8_t*)zb_buf_begin(newbuf);
#ifdef ZB_USEALIAS
  if((apsreq->use_alias) == ZB_TRUE)
  {
    TRACE_MSG(TRACE_APS2, "use ALIAS source address 0x%hx", (FMT__H, apsreq->alias_src_addr ));
    /* 2.2.4.1.1.3
       If UseAlias parameter has the value of TRUE, the supplied value of the AliasSrcAddr
       SHALL be used for the SrcAddress parameter of the APSDE-DATA.indication primitive.
    */
    apsde_data_ind->src_addr = apsreq->alias_src_addr;
  } else
#endif
  {
    apsde_data_ind->src_addr = ZB_PIBCACHE_NETWORK_ADDRESS();
  }
  apsde_data_ind->dst_endpoint = apsreq->dst_endpoint;
  apsde_data_ind->src_endpoint = apsreq->src_endpoint;
  apsde_data_ind->clusterid = apsreq->clusterid;
  apsde_data_ind->profileid = apsreq->profileid;

  TRACE_MSG(TRACE_APS3, "schedule callback zb_apsde_data_indication %hd (copied from %hd)",
            (FMT__H_H, newbuf, param_u8));

  ZB_SCHEDULE_CALLBACK(zb_apsde_data_indication, newbuf);

  /* Check if we have more binding entries to proceed */
  if (zb_check_bind_trans(param_u8) != RET_OK)
  {
    /* Call another iteration of trans_table processing */
    aps_send_frame_via_binding(param_u8);
  }
  else
  {
    TRACE_MSG(TRACE_APS3, "No more bind trans when passing frame to myself - call zb_nlde_data_confirm %d", (FMT__H, param));

    /* NOTE: aps_send_fail_confirm is used here to confirm packet sending
     * immediately without additional ACK checks or addresses unlocking */
    aps_send_fail_confirm(param_u8, 0);
  }

  TRACE_MSG(TRACE_APS3, "-zb_aps_pass_local_msg_up", (FMT__0));
}
/* 14/06/2016 CR [AEV] end */
#endif  /* ZB_LITE_APS_DONT_TX_PACKET_TO_MYSELF */


/**
   Fill APS header in the packet for data request.

   @param fc - FC field (created at upper layer)
   @param req - APS data request
 */
void aps_data_hdr_fill_datareq(zb_uint8_t fc, zb_apsde_data_req_t *req, zb_bufid_t param)
{
  zb_uint8_t *aps_hdr;
  zb_bool_t is_group = (ZB_APS_FC_GET_DELIVERY_MODE(fc) == ZB_APS_DELIVERY_GROUP);
#ifdef APS_FRAGMENTATION
  zb_bool_t with_extended_hdr = ZB_U2B(ZB_APS_FC_GET_EXT_HDR_PRESENT(fc));
#endif
  zb_uint8_t aps_hdr_size =
      1              /* fc */
    + 1              /* dst_endpoint (if not group addressing) */
    + 2              /* cluster id*/
    + 2              /* profile id */
    + 1              /* src_endpoint */
    + 1;             /* APS counter */

  zb_bool_t ext_nonce = ZB_FALSE;

  /* Data request is bcst (nwk-broadcst with addr 0xFFF* or multicast message in member mode) */
  /* If group addressing, dst_endpoint is absent but instead of it 2-bytes
   * Group address exists */
  aps_hdr_size += ZB_B2U(is_group);
#ifdef APS_FRAGMENTATION
  if (with_extended_hdr)
  {
    aps_hdr_size += 2U;          /* extended_hdr + block_num */
  }
#endif

  zb_buf_flags_clr_encr(param);
  if ( ZB_NIB_SECURITY_LEVEL() != 0U && (req->tx_options & ZB_APSDE_TX_OPT_SECURITY_ENABLED) == 1U
      && (ZB_APS_FC_GET_DELIVERY_MODE(fc) != ZB_APS_DELIVERY_BROADCAST) )
  {
    TRACE_MSG(TRACE_SECUR1, "Secure APS by app key", (FMT__0));

    /* Now we always use ext. nonce: see zb_secur_aps_aux_hdr_fill */
    if ((req->tx_options & ZB_APSDE_TX_OPT_INC_EXT_NONCE) != 0U)
    {
      aps_hdr_size += (zb_uint8_t)sizeof(zb_aps_data_aux_nonce_frame_hdr_t);
      ext_nonce = ZB_TRUE;
    }
    else
    {
      aps_hdr_size += (zb_uint8_t)sizeof(zb_aps_data_aux_frame_hdr_t);
    }
    ZB_APS_FC_SET_SECURITY(fc, 1U);
    zb_buf_flags_or(param, ZB_BUF_SECUR_APS_ENCR);
  }

  aps_hdr = (zb_uint8_t *)zb_buf_alloc_left(param, aps_hdr_size);

  *aps_hdr++ = fc;

  /* Assume always ZB_APS_FC_GET_ACK_FORMAT(fc) == 0: command can't be here! */
  ZB_ASSERT(ZB_APS_FC_GET_ACK_FORMAT(fc) == 0U);

  /* If Group addressing, no dest endpoint but have Group address */
  if (is_group)
  {
    ZB_PUT_NEXT_HTOLE16(aps_hdr, req->dst_addr.addr_short);
  }
  else
  {
    *aps_hdr++ = req->dst_endpoint;
  }

  /*
    If data or ack, has cluster and profile id.
    Command can't be here.
   */
  ZB_PUT_NEXT_HTOLE16(aps_hdr, req->clusterid);
  ZB_PUT_NEXT_HTOLE16(aps_hdr, req->profileid);
  *aps_hdr++ = req->src_endpoint;

/* 08/05/2016 CR [AEV] start */
/* 2.2.4.1.1.3
   In addition, if the UseAlias parameter is set to TRUE, the AliasSeqNumb SHALL
   be copied into the APS counter field of the APS header. If the UseAlias parameter
   has a value of FALSE, then APS counter field of the APS header SHALL take the
   value as maintained by the APS.
*/
#ifdef ZB_USEALIAS
if ( (req->use_alias)==ZB_TRUE )
  {
    TRACE_MSG(TRACE_APS3, "aps_data_hdr_fill_datareq: using alias seq_num=%hd", (FMT__H, req->alias_seq_num));
    *aps_hdr++ = req->alias_seq_num;
  }
  else
#endif /*ZB_USEALIAS*/
/* 08/05/2016 CR [AEV] end */
  {
#ifdef APS_FRAGMENTATION
    if (with_extended_hdr && ZG->aps.out_frag.total_blocks_num != 0U)
    {
      /* Fragments of one APS frame should have same APS counter. */
      if (ZB_APS_FC_GET_EXT_HDR_FRAG(req->extended_fc) == ZB_APS_FIRST_FRAGMENT)
      {
        ZG->aps.out_frag.aps_counter = ZB_AIB_APS_COUNTER();
        ZB_AIB_APS_COUNTER_INC();
      }
      *aps_hdr++ = ZG->aps.out_frag.aps_counter;
    }
    else
#endif
    {
      *aps_hdr++ = ZB_AIB_APS_COUNTER();
      ZB_AIB_APS_COUNTER_INC();
    }
  }
#ifdef ZB_CERTIFICATION_HACKS
  if (ZB_CERT_HACKS().aps_counter_custom_setup)
  {
    *(aps_hdr - 1) = ZB_CERT_HACKS().aps_counter_custom_value;
  }
#endif

#ifdef APS_FRAGMENTATION
  if (with_extended_hdr)
  {
    TRACE_MSG(TRACE_APS3, "extended_fc %x block_num %hd", (FMT__D_H, req->extended_fc, req->block_num));
    *aps_hdr++ = req->extended_fc;
    *aps_hdr++ = req->block_num;
  }
#endif

  if (ZB_APS_FC_GET_SECURITY(fc) != 0U)
  {
    zb_uint8_t secur_key_type = ZB_SECUR_DATA_KEY;
    /* Anytime APS retransmitted APS header must use same counter - see
     * 2.2.8.4.4.
     * So, fill aux header now. Payload will be encrypted layer, in MAC
     */
#ifdef ZB_CERTIFICATION_HACKS
    if (ZB_CERT_HACKS().use_transport_key_for_aps_frames)
    {
      secur_key_type = ZB_SECUR_KEY_TRANSPORT_KEY;
    }
#endif
    if (ZB_TCPOL().tc_swapped) /* && payload == unsecure rejoin transport key */
    {
      // secur_key_type = ZB_SECUR_KEY_TRANSPORT_KEY;
    }
    zb_secur_aps_aux_hdr_fill(aps_hdr,
                              secur_key_type,
                              ext_nonce);
  }
}

zb_ushort_t zb_aps_full_hdr_size(const zb_uint8_t *pkt)
{
  zb_ushort_t size = ZB_APS_HDR_SIZE(*pkt);

  if (ZB_APS_FC_GET_SECURITY(*pkt) != 0U)
  {
    size += zb_aps_secur_aux_size(pkt[size + ZB_OFFSETOF(zb_aps_nwk_aux_frame_hdr_t, secur_control)]);
  }

  return size;
}

/**
   Func to check presence of unsend packets
   @param param - index of source buffer
   @return RET_OK if table empty
           RET_ERROR if more table entries present
 */
static zb_ret_t zb_check_bind_trans(zb_uint8_t param)
{
  zb_uindex_t i;
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_APS1, ">>zb_check_bind_trans %hd", (FMT__H, param));

  for (i = 0; i < ZB_APS_BIND_TRANS_TABLE_SIZE; i++)
  {
    if (ZG->aps.binding.trans_table[i] == param)
    {
      TRACE_MSG(TRACE_APS1, "zb_check_bind_trans ret error", (FMT__0));
      ret = RET_ERROR; /* found! */
      break;
    }
  }

  return ret;
}


/**
   Check for group addressing to pass addr mode via confirm. Also do some common stuff.
 */
static void upd_conf_param(zb_uint8_t param, zb_uint8_t fc, zb_aps_hdr_t *aps_hdr)
{
  zb_apsde_data_confirm_t *conf = ZB_BUF_GET_PARAM(param, zb_apsde_data_confirm_t);
  if (ZB_APS_FC_GET_FRAME_TYPE(fc) == ZB_APS_DELIVERY_GROUP)
  {
    conf->addr_mode = ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT;
    conf->dst_addr.addr_short = aps_hdr->group_addr;
  }
  conf->need_unlock = (zb_bool_t)(ZB_APS_FC_GET_DELIVERY_MODE(fc) == ZB_APS_DELIVERY_UNICAST);
}


void zb_nlde_data_confirm(zb_uint8_t param)
{
  zb_uint8_t *fc_p;
  zb_aps_hdr_t aps_hdr;
  zb_bool_t is_binding_trans = ZB_FALSE;
  zb_apsde_data_confirm_t *conf = ZB_BUF_GET_PARAM(param, zb_apsde_data_confirm_t);
  zb_ret_t status = conf->status;
  zb_bool_t inc_aps_ucast_success = ZB_FALSE;

  ZB_TH_PUSH_NLDE_DATA_CONFIRM(param);

  ZB_ASSERT(param);
  /* Cleared in zb_aps_hdr_parse()
  ZB_BZERO(&aps_hdr, sizeof(zb_aps_hdr_t)); */

  /* NWK and MAC headers are cut already */
  fc_p = zb_buf_begin(param);

  TRACE_MSG(TRACE_APS2, "+zb_nlde_data_confirm %hd addr_short 0x%x aps_fc 0x%x status %d len 0x%x",
            (FMT__H_H_H_D_D, param, conf->dst_addr.addr_short, *fc_p, status, zb_buf_len(param)));

  /* #AT we need check status code from zb_nlde_data_request, if we in unjoined state, in confirm we get buffer with trash data */
  if (status == (zb_ret_t)ZB_NWK_STATUS_SUCCESS
      && ZB_NWK_IS_ADDRESS_BROADCAST(conf->dst_addr.addr_short) == 0
#ifndef ZDO_DIAGNOSTICS_COUNT_APS_ACKS
      && ZB_APS_FC_GET_FRAME_TYPE(*fc_p) != ZB_APS_FRAME_ACK
#endif
     )
  {
    /* Presumably the counter ZDO_DIAGNOSTICS_APS_TX_UCAST_SUCCESS_ID
     * will need to be increased. But this can be changed later. */
    inc_aps_ucast_success = ZB_TRUE;
  }
  zb_aps_hdr_parse(param, &aps_hdr, ZB_FALSE);
/* VP: move lot of code into separate function -> aps_send_frame_via_binding(zb_uint8_t) */
  if (zb_check_bind_trans(param) != RET_OK)
  {
    is_binding_trans = ZB_TRUE;
  }

  /* there was zb_secur_send_transport_key_second call - rewritten bu using
   * second buffer - KISS */

  if (ZB_APS_FC_GET_FRAME_TYPE(*fc_p) == ZB_APS_FRAME_ACK)
  {
    /* This is APS ACK - free it */
    /* Check that binding transmission is not in process before deleting APS ack */
    ZB_ASSERT(!is_binding_trans);
#ifdef APS_FRAGMENTATION
    if (status == RET_OK && ZB_APS_FC_GET_EXT_HDR_PRESENT(aps_hdr.fc) != 0U)
    {
      zb_uint8_t frag_idx = aps_in_frag_find_by_hdr(conf->dst_addr.addr_short, aps_hdr.aps_counter);
      if (frag_idx != ZB_INVALID_FRAG_ID)
      {
        zb_uint16_t param2 = 0;
        /* Packing - block_num in the low byte, block_ack in the high byte */
        ZB_SET_LOW_BYTE(param2, aps_hdr.block_num);
        ZB_SET_HI_BYTE(param2, aps_hdr.block_ack);
        /* Attention! Non-standard use ZB_SCHEDULE_CALLBACK2 - the first function argument is not a buffer */
        ZB_SCHEDULE_CALLBACK2(aps_in_frag_mac_ack_for_aps_ack_arrived, frag_idx, param2);
      }
      else
      {
        TRACE_MSG(TRACE_APS2, "Oops! MAC Ack for our APS ACK. frag not found", (FMT__0));
      }
    }
#endif
    TRACE_MSG(TRACE_APS2, "fc %hd our APS ACK - free it. bufid %hd", (FMT__H_H, *fc_p, param));
    zb_buf_free(param);
  }
  else if ( ZB_APS_FC_GET_ACK_REQUEST(*fc_p) == 0U )
  {
    /* Not need to wait for APS ACK - pass confirm up, to AF */
#ifdef ZB_COORDINATOR_ROLE
/* 07/31/2019 EE CR:MINOR Can we move that check up, somewhere into zb_apsde_data_confirm()? Seems we can't send remove_device vis binding :) */
    if (ZB_AIB().bdb_remove_device_param == param && !is_binding_trans)
    {
      /* confirm on Remove Device */
      /* Remove addr_lock at first - it was locked before on zb_aps_send_command. */
      {
        zb_address_ieee_ref_t addr_ref;

        /* NWK now always fills short address if address is valid */
        if (conf->addr_mode != ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT
            && zb_address_by_short(conf->dst_addr.addr_short, /* create */ ZB_FALSE, /* lock */ ZB_FALSE, &addr_ref) == RET_OK)
        {
          zb_address_unlock(addr_ref);
        }
      }
      /* Cut APS header and pass buffer starting at Remove Device payload */
      zb_buf_cut_left(param, ZB_APS_HDR_SIZE(*fc_p) + sizeof(zb_aps_data_aux_nonce_frame_hdr_t));
      bdb_remove_joiner(param);
    }
    else
#endif  /* ZB_COORDINATOR_ROLE */
    if (!is_binding_trans)
    {
      TRACE_MSG(TRACE_APS2, "confirm for aps w/o ack bit - pass up now", (FMT__0));

      /* TODO: store somewhere aps data req mode and pass up correct tx mode parameters  */

      upd_conf_param(param, *fc_p, &aps_hdr);

      conf->src_endpoint = aps_hdr.src_endpoint;
      conf->dst_endpoint = aps_hdr.dst_endpoint;

      ZB_SCHEDULE_CALLBACK(zb_apsde_data_confirm, param);
    }
    else
    {
      /* If binding transmission is in progress and frame does not requires APS ACK
         send new frame from here */
      aps_send_frame_via_binding(param);
      TRACE_MSG(TRACE_APS2, "skip confirm, will be passed up at the end of transmission", (FMT__0));
    }
  }
  else                          /* no ack bit */
  {
    zb_uindex_t i;
    /* This is confirm to APS packet that must be acked, or acked already */
    for (i = 0 ; i < ZB_N_APS_RETRANS_ENTRIES ; ++i)
    {
      if ((ZG->aps.retrans.hash[i].buf == param) && (ZG->aps.retrans.hash[i].aps_counter == aps_hdr.aps_counter))
      {
        TRACE_MSG(TRACE_APS2, "found record in retrans table %hd, cnt %hd state %hd retries %hd",
                  (FMT__H_H_H_H, i, aps_hdr.aps_counter, ZG->aps.retrans.hash[i].state,
                   ZG->aps.retrans.hash[i].aps_retries));

        if (ZG->aps.retrans.hash[i].state == ZB_APS_RETRANS_ENT_KILL_AT_MAC_CONFIRM
            || status == ERROR_CODE(ERROR_CATEGORY_NWK, ZB_NWK_STATUS_FRAME_NOT_BUFFERED) /* purged - see mac_status_2_nwk_command_status */
          )
        {
          TRACE_MSG(TRACE_APS2, "record in retrans table for unbuffered or purged frame %hd, is_binding_trans %hd",
                    (FMT__H_H, param, is_binding_trans));

          /* peer has left */
          /* If bind tx: even if this destination is
             gone, we might have more to tx via binding */
          if (is_binding_trans)
          {
            /* RET_PENDING status to prevent apsde_data_confirm call */
            done_with_this_ack(i, &aps_hdr, RET_PENDING);
            /* More tx via binding can be done */
            aps_send_frame_via_binding(param);
          }
          else
          {
            done_with_this_ack(i, &aps_hdr, ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_NO_SHORT_ADDRESS));
          }
        }
        else if (ZG->aps.retrans.hash[i].state == ZB_APS_RETRANS_ENT_SENT_MAC_NOT_CONFIRMED_APS_ACKED_ALRM_RUNNING)
        {
          /* That packet ACKed while we were sending retransmit. */
          /* If we already received APS ACK and waiting for NWK confirm on our
           * retransmit, APS frame is delivered ok even if its retransmit
           * failed, so need not check NWK confirm status.
           */
          TRACE_MSG(TRACE_APS2, "APS ACK OK, retransmit of %hd confirmed %hd",
                    (FMT__H_H, param, ZG->aps.retrans.hash[i].state));

#ifdef ZB_ZCL_ENABLE_WWAH_SERVER
          zb_zcl_wwah_bad_parent_recovery_signal(ZB_ZCL_WWAH_BAD_PARENT_RECOVERY_APS_ACK_OK);
#endif

          if (is_binding_trans)
          {
            /* CR: 06/06/2016 CR:MAJOR EE: tx via bind from here. We have not
             * done it from aps_ack_frame_handle(). */
            done_with_this_ack(i, &aps_hdr, RET_PENDING);
            /* We have not send tx via binding from aps_ack_frame_handle(). Send it here. */
            aps_send_frame_via_binding(param);
          }
          else
          {
            done_with_this_ack(i, &aps_hdr, RET_EXIT);
          }
        }
        else if (status == (zb_ret_t)ZB_NWK_STATUS_SUCCESS)
        {
          TRACE_MSG(TRACE_APS2, "packet %hd len %hd confirm state %hd, set st %hd, still wait for ack",
                    (FMT__H_H_H_H, param, zb_buf_len(param), ZG->aps.retrans.hash[i].state, ZB_APS_RETRANS_ENT_SENT_MAC_CONFIRMED_ALRM_RUNNING));
          ZB_ASSERT(ZG->aps.retrans.hash[i].state == ZB_APS_RETRANS_ENT_SENT_MAC_NOT_CONFIRMED_ALRM_RUNNING);
          /* Confirmed APS frame sent, waiting for APS ACK */
          ZG->aps.retrans.hash[i].state = ZB_APS_RETRANS_ENT_SENT_MAC_CONFIRMED_ALRM_RUNNING;
          /* Packet sent ok, waiting for APS ACK */
          inc_aps_ucast_success = ZB_FALSE;
        }
        else if (
                 /* NWK rejected our frame because we are no longer joined. It is
                  * possible if we lost network when APS packet was in progress of retransmitting.
                  */
          (status == ERROR_CODE(ERROR_CATEGORY_NWK, ZB_NWK_STATUS_INVALID_REQUEST))
#ifdef ZB_REDUCE_NWK_LOAD_ON_LOW_MEMORY
                 /* Do not retransmit pkt with route discovery error - it is rather long operation
                  * (3 x Pending Route Discovery time). */
          || (status == ERROR_CODE(ERROR_CATEGORY_NWK, ZB_NWK_STATUS_ROUTE_ERROR))
#endif
          )
        {
          TRACE_MSG(TRACE_APS2, "Got %hd status from NWK - confirm APS packet send failure now", (FMT__H, status));
          /*
          ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_APS_TX_UCAST_FAIL_ID);
          */
          if (is_binding_trans)
          {
            /* need to clear bind transmission table as confirm is passed up */
            zb_clear_bind_trans(param, &aps_hdr);
            /* CR: 06/06/2016 CR:MAJOR EE: Do not pass up confirm from
             * done_with_this_ack and do not send via bind. Will it produce
             * buffer leak?
             * Seems, still must send confirm up if we "unjoined" suddenly. Use
             * retcode other than RET_BLOCKED.
             */
            done_with_this_ack(i, &aps_hdr, ERROR_CODE(ERROR_CATEGORY_APS,ZB_APS_STATUS_NO_ACK));
          }
          else
          {
            done_with_this_ack(i, &aps_hdr, ERROR_CODE(ERROR_CATEGORY_APS,ZB_APS_STATUS_NO_ACK));
          }
        }
        else
        {
          /* This is not fatal case yet: may try to send again. */
          TRACE_MSG(TRACE_APS2, "APS resend failed at NWK/MAC %d, will resend by alarm", (FMT__D, status));
          /*
          ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_APS_TX_UCAST_FAIL_ID);
          */
          /* change state to be able to re-send from the alarm. Else alarm will
           * be looping. */
          ZG->aps.retrans.hash[i].state = ZB_APS_RETRANS_ENT_SENT_MAC_CONFIRMED_ALRM_RUNNING;
        }
        break;
      }
    }
    if ( i == ZB_N_APS_RETRANS_ENTRIES )
    {
      zb_uint8_t j;
      /* not found (hmm...) - free it */
      TRACE_MSG(TRACE_APS2, "strange buf %hd - free it", (FMT__H, param));
      zb_buf_free(param);
      /*aev - this is not enough, we must clean timer_cb on this ack!*/
      for ( j = 0 ; j < ZB_N_APS_RETRANS_ENTRIES ; ++j)
      {
        if (param == ZG->aps.retrans.hash[j].buf)
        {
          /* Remove all entries from hash - old transaction is finished. Skip only current hash
           * entry */
          TRACE_MSG(TRACE_APS2, "Drop ZG->aps.retrans.hash[j].buf param %hd", (FMT__H, param));
          aps_retrans_ent_free(j);
        }
      }
    }
  }

  if (inc_aps_ucast_success)
  {
    ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_APS_TX_UCAST_SUCCESS_ID);
  }
  TRACE_MSG(TRACE_APS2, "-zb_nlde_data_confirm", (FMT__0));
}

/*
   This is APS handler for nlde-data.indication - receive path entry point
*/
void zb_nlde_data_indication(zb_uint8_t param)
{
  zb_uint8_t fc;
  zb_uint8_t nwk_hdr_size;

  ZB_TH_PUSH_PACKET(ZB_TH_NLDE_DATA, ZB_TH_PRIMITIVE_INDICATION, param);

  {
    zb_nwk_hdr_t *nwk_hdr = (zb_nwk_hdr_t *)zb_buf_begin(param);
    nwk_hdr_size = ZB_NWK_HDR_SIZE(nwk_hdr);
    fc = *((zb_uint8_t*)nwk_hdr + nwk_hdr_size);
  }

  /* Result of the headers parse */

  TRACE_MSG(TRACE_APS1, "> zb_nlde_data_indication param %hd", (FMT__H, param));

  /* Parse NWK header */
  /* get source address from the NWK header */

  if (ZB_APS_FC_GET_FRAME_TYPE(fc) == ZB_APS_FRAME_ACK)
  {
    zb_aps_hdr_t aps_hdr;
    zb_aps_hdr_parse(param, &aps_hdr, ZB_TRUE);
    aps_ack_frame_handle(&aps_hdr);
    zb_buf_free(param);
  }
  else
  {
    if (ZB_APS_FC_GET_ACK_REQUEST(fc) != 0U
#ifdef ZB_CERTIFICATION_HACKS
        && !ZB_CERT_HACKS().disable_aps_acknowledgements
#endif /* ZB_CERTIFICATION_HACKS */
#ifdef ZB_STACK_REGRESSION_TESTING_API
        && !ZB_REGRESSION_TESTS_API().disable_aps_acknowledgements
#endif /* ZB_STACK_REGRESSION_TESTING_API */
      )
    {
      /* Note: zb_get_buf has higher priority than blocked alloc. It is
       * more probable to have 1 out buf reserved for aps ack */
      zb_bufid_t ack_buf = zb_buf_get_hipri(ZB_FALSE);
      if (ack_buf != 0U)
      {
        if (zb_schedule_callback2(zb_nlde_data_indication_continue, ack_buf, param) != RET_OK)
        {
          TRACE_MSG(TRACE_ERROR, "Oops - out of queue; can't send APS ACK for %hd - drop it.", (FMT__H, param));
          zb_buf_free(param);
          zb_buf_free(ack_buf);
        }
      }
      else
      {
        /* prevent in nwk q block if we will lock in buffer allocate */
        zb_nwk_unlock_in(param);
        if (zb_buf_get_out_delayed_ext(zb_nlde_data_indication_continue, param, 0) != RET_OK)
        {
          TRACE_MSG(TRACE_ERROR, "Oops - out of memory; can't send APS ACK for %hd - drop it.", (FMT__H, param));
          zb_buf_free(param);
        }
      }
    }
    else
    {
      zb_nlde_data_indication_continue(0, param);
    }
  }
  TRACE_MSG(TRACE_APS1, "< zb_nlde_data_indication", (FMT__0));
}

#ifdef APS_FRAGMENTATION

static zb_uint8_t aps_in_frag_find_by_hdr(zb_uint16_t src_addr, zb_uint8_t aps_counter)
{
  zb_uint8_t i;

  TRACE_MSG(TRACE_APS3, "> aps_in_frag_find_by_hdr addr 0x%x aps_counter %u",
            (FMT__D_D, src_addr, aps_counter));

  for (i = 0; i < ZB_APS_MAX_IN_FRAGMENT_TRANSMISSIONS; i++)
  {
    zb_aps_in_fragment_frame_t *ent = &ZG->aps.in_frag[i];
    if (ent->src_addr == src_addr && ent->aps_counter == aps_counter)
    {
      TRACE_MSG(TRACE_APS3, "< aps_in_frag_find_by_hdr found: #%hd", (FMT__H, i));
      return i;
    }
  }

  TRACE_MSG(TRACE_APS3, "< aps_in_frag_find_by_hdr not found", (FMT__0));
  return ZB_INVALID_FRAG_ID;
}

static zb_aps_in_fragment_frame_t* aps_in_frag_allocate(zb_uint16_t src_addr, zb_uint8_t aps_counter)
{
  zb_uindex_t i;

  TRACE_MSG(TRACE_APS3, "> aps_in_frag_allocate 0x%x %u",
            (FMT__D_D, src_addr, aps_counter));

  for (i = 0; i < ZB_APS_MAX_IN_FRAGMENT_TRANSMISSIONS; i++)
  {
    zb_aps_in_fragment_frame_t *ent = &ZG->aps.in_frag[i];
    if (ent->src_addr == ZB_APS_IN_FRAGMENTED_FRAME_EMPTY)
    {
      ZB_BZERO(ent, sizeof(zb_aps_in_fragment_frame_t));
      ent->aps_counter = aps_counter;
      ent->src_addr = src_addr;

      TRACE_MSG(TRACE_APS3, "< aps_in_frag_allocate: allocated #%hd", (FMT__H, i));
      return ent;
    }
  }

  TRACE_MSG(TRACE_APS3, "< aps_in_frag_allocate: no free space", (FMT__0));
  return NULL;
}

static zb_uint8_t aps_in_frag_find_by_buf(zb_bufid_t buf)
{
  zb_uint8_t trans_id;
  zb_uint8_t block_id;

  TRACE_MSG(TRACE_APS3, ">> aps_find_in_frag_entr_by_buf buf %hd", (FMT__H, buf));

  for (trans_id = 0; trans_id < ZB_APS_MAX_IN_FRAGMENT_TRANSMISSIONS; trans_id++)
  {
    if (ZG->aps.in_frag[trans_id].src_addr == ZB_APS_IN_FRAGMENTED_FRAME_EMPTY)
    {
      continue;
    }

    for (block_id  = 0; block_id < ZB_APS_MAX_FRAGMENT_NUM_IN_WINDOW; block_id ++)
    {
      if (ZG->aps.in_frag[trans_id].window_buffers[block_id] == buf)
      {
        TRACE_MSG(TRACE_APS3, "<< aps_find_in_frag_entr_by_buf found: #%hd", (FMT__H, trans_id));
        return trans_id;
      }
    }
  }

  TRACE_MSG(TRACE_APS3, "<< aps_find_in_frag_entr_by_buf not found", (FMT__0));
  return ZB_INVALID_FRAG_ID;
}

static void aps_in_frag_free_window(zb_aps_in_fragment_frame_t *ent)
{
  zb_uint_t i;

  TRACE_MSG(TRACE_APS2, ">> aps_in_frag_free_window (src_addr %x aps_counter %u)",
            (FMT__D_D, ent->src_addr, ent->aps_counter));

  for (i = 0; i < ZG->aps.aib.aps_max_window_size; i++)
  {
    if (ent->window_buffers[i] != 0U)
    {
      zb_buf_free(ent->window_buffers[i]);
      ent->window_buffers[i] = 0;
    }
  }
  TRACE_MSG(TRACE_APS4, "<< aps_in_frag_free_window", (FMT__0));
}

static void aps_in_frag_kill_stopped_entries(zb_uint8_t param)
{
  zb_uindex_t i = 0;
  ZVUNUSED(param);

  for (; i < ZB_APS_MAX_IN_FRAGMENT_TRANSMISSIONS; i++)
  {
    if (ZG->aps.in_frag[i].src_addr != ZB_APS_IN_FRAGMENTED_FRAME_EMPTY
        && ZG->aps.in_frag[i].flags.state == APS_IN_FRAG_COMPLETE
        && ZG->aps.in_frag[i].buffer == 0U)
    {
      aps_in_frag_free_window(&ZG->aps.in_frag[i]);
      ZG->aps.in_frag[i].src_addr = ZB_APS_IN_FRAGMENTED_FRAME_EMPTY;
    }
  }
}

static void aps_in_frag_stop_entry(zb_aps_in_fragment_frame_t *ent, zb_bool_t delayed_kill)
{
  ZB_ASSERT(ent->src_addr != ZB_APS_IN_FRAGMENTED_FRAME_EMPTY);

  /* Free buffers and stop timers, but keep state */
  aps_in_frag_stop_fail_timer(ent);
  aps_in_frag_stop_ack_timer(ent);

  if (ent->buffer != 0U)
  {
    zb_buf_free(ent->buffer);
  }

  if (delayed_kill)
  {
    ZB_ASSERT(ent->flags.state == APS_IN_FRAG_COMPLETE);
    /*
       TODO: We should schedule a timer for each stopped entry, but since we have released all buffers
       we do not have anything to use as an identifier of the entry. Passing index as a param violates
       currently used coding standard, and adding a counter to each entry seems like an overkill.

       Since this feature is not mandatory, probably it is ok for now just to kill all such entires
       at once - anyway, it is unlikely that many fragmented transactions would finish at the same
       time.

       Let the persistence time interval be 2 * apscAckWaitDuration - so that the sender will
       certainly start retransmitting in the case it missed ACK.
     */
    /*
    zb_uint8_t rx_on_when_idle = zb_nwk_get_nbr_rx_on_idle_short_or_false(ent->src_addr);
    */
    ZB_SCHEDULE_ALARM(aps_in_frag_kill_stopped_entries, 0, ZB_N_APS_ACK_WAIT_DURATION(ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE())) * 2U);
    /* DL: Window buffers can be freed right now, the whole fragmented packet
     * is ready, intermediate window buffers are no longer needed.
     */
    aps_in_frag_free_window(ent);
  }
  else
  {
    ent->src_addr = ZB_APS_IN_FRAGMENTED_FRAME_EMPTY;
    aps_in_frag_free_window(ent);
  }

#ifdef SNCP_MODE
  /* stop turbo poll for fragmented APS RX if there is no incoming packets */
  if (aps_in_frag_trans_exist() == 0U)
  {
    TRACE_MSG(TRACE_APS3, "no incoming frag transmission - stopping SNCP auto turbo poll", (FMT__0));
    sncp_auto_turbo_poll_aps_rx(ZB_FALSE);
  }
#endif

#if defined ZB_ED_FUNC && defined ZB_APS_FRAG_FORCE_DISABLE_TURBO_POLL
  /* Stop turbo poll. Will check for other incoming fragmented transmissions before stopping.
   */
  if (aps_in_frag_trans_exist() == 0U)
  {
    TRACE_MSG(TRACE_APS3, "no incoming frag transmission - stopping turbo poll", (FMT__0));
    /* zb_zdo_pim_turbo_poll_continuous_leave(0); */
    zb_zdo_turbo_poll_packets_leave(0);
    zb_zdo_pim_reset_turbo_poll_max(0);
  }
#endif /* ZB_ED_FUNC && ZB_APS_FRAG_FORCE_DISABLE_TURBO_POLL*/
}

static zb_uint8_t aps_in_frag_get_received_mask(zb_aps_in_fragment_frame_t *ent)
{
  zb_uint32_t blocks_acked_mask = ~0U;
  zb_uint8_t i;
  zb_uint8_t last_window;
  zb_uint8_t last_block;

  last_window = (ZG->aps.aib.aps_max_window_size - 1U + ent->total_blocks_num) / ZG->aps.aib.aps_max_window_size - 1U;
  if (ent->current_window == last_window)
  {
    last_block = (ent->total_blocks_num - 1U) % ZG->aps.aib.aps_max_window_size;
  }
  else
  {
    last_block = ZG->aps.aib.aps_max_window_size - 1U;
  }

  TRACE_MSG(TRACE_APS3, "current_window %hd last_window %hd last_block %hd",
            (FMT__H_H_H, ent->current_window, last_window, last_block));

  for (i = 0; i <= last_block; i++)
  {
    TRACE_MSG(TRACE_APS3, "block %hd within the window: received %hd",
              (FMT__H_H, i, ent->window_buffers[i] > 0U ? 1U : 0U));

    if (ent->window_buffers[i] == 0U)
    {
      ZB_CLR_BIT_IN_BIT_VECTOR(&blocks_acked_mask, i);
    }
  }

  if (ent->flags.state == APS_IN_FRAG_WINDOW_RECEIVED
      || ent->flags.state == APS_IN_FRAG_WINDOW_MERGED
      || ent->flags.state == APS_IN_FRAG_COMPLETE)
  {
    blocks_acked_mask = ~0U;
  }

  return (zb_uint8_t)blocks_acked_mask;
}

static zb_uint_t aps_in_frag_total_window_size(const zb_aps_in_fragment_frame_t *ent)
{
  zb_uint_t bytes_to_allocate = 0;
  zb_uint_t i;

  for (i = 0; i < ZG->aps.aib.aps_max_window_size; i++)
  {
    zb_uint_t aps_hdr_len;
    if (ent->window_buffers[i] == 0U)
    {
      /* This might happen if it is the last window */
      break;
    }

    aps_hdr_len = zb_aps_full_hdr_size(zb_buf_begin(ent->window_buffers[i]));
    bytes_to_allocate += zb_buf_len(ent->window_buffers[i]) - aps_hdr_len;
    TRACE_MSG(TRACE_APS3, "i %d bytes_to_allocate %d", (FMT__D_D, i, bytes_to_allocate));
  }

  return bytes_to_allocate;
}


static zb_uint_t aps_single_frag_size(zb_aps_in_fragment_frame_t *ent)
{
  zb_uint_t frag_size = 0;
  zb_uint8_t i;

  for (i = 0; i < ZG->aps.aib.aps_max_window_size; i++)
  {
    zb_uint_t aps_hdr_len;
    if (ent->window_buffers[i] == 0U)
    {
      /* This might happen if it is the last window */
      break;
    }

    aps_hdr_len = zb_aps_full_hdr_size(zb_buf_begin(ent->window_buffers[i]));
    if (frag_size < zb_buf_len(ent->window_buffers[i]) - aps_hdr_len)
    {
      frag_size = zb_buf_len(ent->window_buffers[i]) - aps_hdr_len;
    }
  }

  return frag_size;
}


static void aps_in_frag_do_merge_blocks(zb_aps_in_fragment_frame_t *ent)
{
  zb_uint8_t i;
  zb_apsde_data_indication_t *buf_param;

  ZB_ASSERT(ent->buffer);
  for (i = 0; i < ZG->aps.aib.aps_max_window_size; i++)
  {
    zb_uint8_t *ptr;
    zb_uint_t aps_hdr_len;

    if (ent->window_buffers[i] == 0U)
    {
      /* This might happen if it is the last window */
      ZB_ASSERT(i != 0U);
      break;
    }

    /* We are assuming there is an aps header present */
    aps_hdr_len = zb_aps_full_hdr_size(zb_buf_begin(ent->window_buffers[i]));

    /* Copy aps header if this is the first packet of the whole transaction */
    if (i == 0U && ent->current_window == 0U)
    {
      ptr = zb_buf_alloc_right(ent->buffer, aps_hdr_len);
      ZB_MEMCPY(ptr, zb_buf_begin(ent->window_buffers[i]), aps_hdr_len);
    }

    TRACE_MSG(TRACE_APS3, "appending %hd to %hd: existing len %d allocated len %d",
              (FMT__H_H_D_D, ent->window_buffers[i],
               ent->buffer,
               zb_buf_len(ent->buffer),
               zb_buf_len(ent->window_buffers[i]) - aps_hdr_len
              ));

    /* Appending buffer payload to the right */
    ptr = zb_buf_alloc_right(ent->buffer, zb_buf_len(ent->window_buffers[i]) - aps_hdr_len);
    ZB_MEMCPY(ptr,
              zb_buf_data(ent->window_buffers[i], aps_hdr_len),
              zb_buf_len(ent->window_buffers[i]) - aps_hdr_len);
  }

  /* Copy buffer parameter */
  /*cstat !MISRAC2012-Rule-20.7 See ZB_BUF_GET_PARAM() for more information. */
  buf_param = ZB_BUF_GET_PARAM(ent->buffer, zb_apsde_data_indication_t);
  ZB_MEMCPY(buf_param,
            /*cstat !MISRAC2012-Rule-20.7 */
            ZB_BUF_GET_PARAM(ent->window_buffers[0], zb_apsde_data_indication_t),
            sizeof(zb_apsde_data_indication_t));
  TRACE_MSG(TRACE_APS3, "copied aps params: cluster %x profile %x", (FMT__D_D,
                                                                     buf_param->clusterid,
                                                                     buf_param->profileid));

  if (ent->flags.state == APS_IN_FRAG_WINDOW_RECEIVED)
  {
    ent->flags.state = APS_IN_FRAG_WINDOW_MERGED_NO_ACK;
    /* Moved to aps_in_frag_mac_ack_for_aps_ack_arrived()
     * Status changed to APS_IN_FRAG_WINDOW_MERGED if MAC ACK to our APS ACK received
     */
    /*
    ent->flags.state = APS_IN_FRAG_WINDOW_MERGED;
    */
  }

  if (ent->flags.state == APS_IN_FRAG_COMPLETE)
  {
    /* It's too early to send a packet to the NHLE here,
     * because we have not received the MAC ACK for our APS ACK yet
     */
    /* Moved to aps_in_frag_mac_ack_for_aps_ack_arrived()
    TRACE_MSG(TRACE_APS3, "sending apse-data.confirm buf %hd ent->flags.state %hd", (FMT__H_H, ent->buffer, ent->flags.state));
    ZB_SCHEDULE_CALLBACK(zb_apsde_data_indication, ent->buffer);
    ent->buffer = 0;

    aps_in_frag_stop_entry(ent, ZB_TRUE);
    */
  }

  ent->flags.assemble_in_progress = ZB_FALSE;
}

static void aps_in_frag_assemble_window_delayed(zb_bufid_t buf, zb_uint16_t params)
{
  zb_aps_in_fragment_frame_t *ent = &(ZG->aps.in_frag[params]);
  zb_uint8_t *ptr;

  ZB_ASSERT(buf);

  TRACE_MSG(TRACE_APS3, "aps_in_frag_assemble_window_delayed buf %hd params %d",
            (FMT__H_D, buf, params));

  if (ent->src_addr == ZB_APS_IN_FRAGMENTED_FRAME_EMPTY)
  {
    /*
       Probably, the entry has already expired.
       We can be here if we were using a function like zb_buf_get_out_delayed and the buffer hasn't
       been allocated for so much time that the transaction has expired.
    */
    TRACE_MSG(TRACE_APS4, "frag entry is empty", (FMT__0));
    zb_buf_free(buf);
    return;
  }
  /*cstat !MISRAC2012-Rule-13.5 */
  /* After some investigation both this and the following violation of the Rule 13.5 seem to
   * be false positives. There is no side effect to aps_in_frag_total_window_size(). This seems
   * to be due to the existance of external functions inside aps_in_frag_total_window_size()
   * which cannot be analyzed by C-STAT. 'const' was added to this and other function parameters. */
  if (ent->buffer == 0U && zb_buf_get_max_size(buf) > aps_in_frag_total_window_size(ent))
  {
    ent->buffer = buf;
  }
  else if (ent->buffer != 0U
           /*cstat !MISRAC2012-Rule-13.5 */
           && zb_buf_get_max_size(buf) >= (aps_in_frag_total_window_size(ent)
                                           + zb_buf_get_max_size(ent->buffer)))
  {
    /* Move all data into the new buffer */
    TRACE_MSG(TRACE_APS3, "moving %d bytes from buf %hd to %hd",
              (FMT__D_H_H, zb_buf_len(ent->buffer), ent->buffer, buf));

    ptr = zb_buf_initial_alloc(buf, zb_buf_len(ent->buffer));
    ZB_MEMCPY(ptr, zb_buf_begin(ent->buffer), zb_buf_len(ent->buffer));

    zb_buf_free(ent->buffer);
    ent->buffer = buf;
  }
  else
  {
    /*
       It might happen that the entry has not only expired, but also it was replaced by another one.
       If the buffer is too small for the new entry, we just skip it and wait until the required one
       gets allocated.
    */
    zb_buf_free(buf);
    return;
  }

  aps_in_frag_do_merge_blocks(ent);
}

static zb_ret_t aps_in_frag_assemble_window(zb_uint8_t frag_idx)
{
  zb_uint_t required_size;
  zb_ret_t ret = RET_OK;
  zb_aps_in_fragment_frame_t *ent = &ZG->aps.in_frag[frag_idx];

  ZB_ASSERT(ent);
  ZB_ASSERT(!ent->flags.assemble_in_progress);

  ent->flags.assemble_in_progress = ZB_TRUE;

  /* In addition to the payload we need to store a buf param. */
  required_size = sizeof(zb_apsde_data_indication_t);
  if (ent->buffer != 0U)
  {
    required_size += zb_buf_len(ent->buffer);
  }
  else
  {
    /* Also we need to store an APS header (once) */
    required_size += ZB_APS_HEADER_MAX_LEN + 2U /* fragmentation ext header */;
  }

  /* TODO: add a check for maximum incoming total size */

  /* Current window */
  required_size += aps_in_frag_total_window_size(ent);

  if (ent->buffer != 0U
      /*cstat !MISRAC2012-Rule-13.5 !MISRAC2012-Rule-13.6 */
      /* After some investigation, the following violations of Rule 13.5 and 13.6 seem to be false
       * positives. There are no side effect to 'zb_buf_get_max_size()'. This violation seems to be
       * caused by the fact that 'zb_buf_get_max_size()' is an external macro, which cannot be
       * analyzed by C-STAT. */
      && required_size <= zb_buf_get_max_size(ent->buffer))
  {
    TRACE_MSG(TRACE_APS3, "ent %p buffer %d required buffer size: %d, max size %d - merge",
              (FMT__P_D_D_D, ent, ent->buffer, required_size, zb_buf_get_max_size(ent->buffer)));

    aps_in_frag_do_merge_blocks(ent);
  }
  else
  {
    zb_uint_t new_buf_size;

    if (ent->buffer != 0U)
    {
      TRACE_MSG(TRACE_APS3, "ent %p buffer %d required buffer size: %d, max size %d - reallocate", (FMT__P_D_D_D, ent, ent->buffer, required_size, zb_buf_get_max_size(ent->buffer)));
      /* Let the buffer increase x 1.5 times */
      /* Increment re-allocation it is bad solution causing extra buffers usage
       * in the real scenario of window size 1, packet size 1500b: lot of
       * reallocates, begin of buffer pool is not usable.  Keep that logic of
       * increment re-allocation for now until r23 where we can remove window
       * size != 1. */
      new_buf_size = zb_buf_get_max_size(ent->buffer) * 3U / 2U;
    }
    else
    {
      /* First allocate. Try to allocate buffer from the first attempt and
       * exclude reallocates to save buffers. */
      zb_uint_t aps_hdr_len = zb_aps_full_hdr_size(zb_buf_begin(ent->window_buffers[0]));
      zb_uint_t single_frag_size = aps_single_frag_size(ent);

      new_buf_size = aps_hdr_len + sizeof(zb_apsde_data_indication_t) + single_frag_size * ent->total_blocks_num + 32U/* just in case */ ;
      TRACE_MSG(TRACE_APS3, "aps_hdr_len %d data_ind_size %d single_frag_size %d total_blocks_num %d", (FMT__D_D_D_D, aps_hdr_len, sizeof(zb_apsde_data_indication_t), single_frag_size, ent->total_blocks_num));
      TRACE_MSG(TRACE_APS3, "ent %p buffer %d required buffer size: %d - allocate %d", (FMT__P_D_D_D, ent, ent->buffer, required_size, new_buf_size));
    }
    if (new_buf_size > APS_IN_FRAG_MAX_BUF_SIZE)
    {
      new_buf_size = APS_IN_FRAG_MAX_BUF_SIZE;
      TRACE_MSG(TRACE_APS3, "new_buf_size %d", (FMT__D, new_buf_size));
    }

    if (new_buf_size < required_size)
    {
      new_buf_size = required_size;
      TRACE_MSG(TRACE_APS3, "new_buf_size %d", (FMT__D, new_buf_size));
    }

    ret = zb_buf_get_in_delayed_ext(aps_in_frag_assemble_window_delayed,
                                    frag_idx,
                                    new_buf_size);
  }

  if (ret != RET_OK)
  {
    ent->flags.assemble_in_progress = ZB_TRUE;
  }

  return ret;
}

static zb_bool_t aps_in_frag_is_block_within_window(zb_uint8_t current_window,
                                            zb_uint8_t block_no,
                                            zb_uint8_t total_blocks_num)
{
  zb_uint8_t window_start = current_window * ZG->aps.aib.aps_max_window_size;
  zb_uint8_t window_end = window_start + ZG->aps.aib.aps_max_window_size - 1U;

  if (total_blocks_num >= window_end)
  {
    window_end = total_blocks_num - 1U;
  }

  return (zb_bool_t)(window_start <= block_no && block_no <= window_end);
}

static void aps_in_frag_send_ack(zb_bufid_t ack_buf, zb_uint16_t idx)
{
  zb_aps_in_fragment_frame_t *ent = &(ZG->aps.in_frag[idx]);
  zb_aps_hdr_t aps_hdr;
  zb_bufid_t buf;

  TRACE_MSG(TRACE_APS3, ">> aps_in_frag_send_ack ack_buf %hd idx %u", (FMT__H_D, ack_buf, idx));
  ZB_ASSERT(ack_buf != 0U);

  if (ent->src_addr == ZB_APS_IN_FRAGMENTED_FRAME_EMPTY)
  {
    /* While we've waited, the entry's become cancelled/stopped */
    ent = NULL;
  }
  else
  {
    buf = aps_in_frag_find_any_block(ent);
    if (buf == 0U)
    {
      /* Seems that while we've waited, another transmission has taken the [idx] position */
      ent = NULL;
    }
  }

  if (ent != NULL)
  {
    /*cstat !MISRAC2012-Rule-1.3_* !MISRAC2012-Rule-9.1_* */
    /* Rule-9.1 and Rule-1.3: The variable buf is used only when ent is not NULL
     * and there is no path when ((ent) != NULL) that buf is not initialized */
    zb_aps_hdr_parse(buf, &aps_hdr, ZB_FALSE);
    /* Yes, sort of hacky, but it seems there is no other way without the
       reconstuction of APS buf param. */
    aps_hdr.src_addr = ent->src_addr;

    aps_ack_send_handle(ack_buf, &aps_hdr);
  }
  else
  {
    zb_buf_free(ack_buf);
  }
  TRACE_MSG(TRACE_APS3, "<< aps_in_frag_send_ack", (FMT__0));
}

static void aps_in_frag_send_ack_delayed(zb_bufid_t buf)
{
  zb_ret_t ret;
  zb_uint8_t frag_idx;

  TRACE_MSG(TRACE_APS2, ">>aps_in_frag_send_ack_delayed buf %hd", (FMT__H, buf));

  frag_idx = aps_in_frag_find_by_buf(buf);
  /* Assertion - We should've cancelled this alarm not to let this happen */
  ZB_ASSERT(frag_idx != ZB_INVALID_FRAG_ID);

  ret = zb_buf_get_out_delayed_ext(aps_in_frag_send_ack, frag_idx, 0);
  if (ret != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed_ext [%d]", (FMT__D, ret));
  }

  /* Reschedule itself by aps_in_frag_restart_ack_timer().
   * A limit on the number of times has been introduced.
   * FIXME: probably need a better value than ZB_N_APS_MAX_FRAME_RETRIES.
   * Note that flags.aps_ack_retry_cnt is three bits long.
   */
  if (ZG->aps.aib.aps_max_window_size > 1U
      /* Strict condition for prevention array borders violation */
      && frag_idx < ZB_ARRAY_SIZE(ZG->aps.in_frag)
      && ZG->aps.in_frag[frag_idx].flags.aps_ack_retry_cnt < ZB_N_APS_MAX_FRAME_RETRIES)
  {
    ZG->aps.in_frag[frag_idx].flags.aps_ack_retry_cnt++;
    aps_in_frag_restart_ack_timer(frag_idx);
  }
}

static zb_bufid_t aps_in_frag_find_any_block(zb_aps_in_fragment_frame_t *ent)
{
  zb_uint8_t i;
  for (i = 0; i < ZG->aps.aib.aps_max_window_size; i++)
  {
    if (ent->window_buffers[i] == 0U)
    {
      continue;
    }

    return ent->window_buffers[i];
  }

  return 0;
}

#if defined SNCP_MODE || defined ZB_APS_FRAG_FORCE_DISABLE_TURBO_POLL
/*
 * Counts and returns the number of concurrent incoming live fragmented transmissions
 */
static zb_uint8_t aps_in_frag_trans_exist(void)
{
  zb_uint8_t i, in_trans_cnt = 0;
  for (i = 0; i < ZB_APS_MAX_IN_FRAGMENT_TRANSMISSIONS; i++)
  {
    zb_aps_in_fragment_frame_t *ent = &ZG->aps.in_frag[i];
    /* Only counts transmissions with src_addr and
     * state other than APS_IN_FRAG_COMPLETE
     */
    if (ent->src_addr != ZB_APS_IN_FRAGMENTED_FRAME_EMPTY && ent->flags.state != APS_IN_FRAG_COMPLETE)
    {
      in_trans_cnt++;
    }
  }
  return in_trans_cnt;
}
#endif /* defined SNCP_MODE || defined ZB_APS_FRAG_FORCE_DISABLE_TURBO_POLL */

static void aps_in_frag_mac_ack_for_aps_ack_arrived(zb_uint8_t frag_idx, zb_uint16_t block_num_ack)
{
  zb_aps_in_fragment_frame_t *ent;
  /* Unpacking - block_num from low byte, block_ack from high byte */
  zb_uint8_t block_num = ZB_GET_LOW_BYTE(block_num_ack);
  zb_uint8_t block_ack = ZB_GET_HI_BYTE(block_num_ack);

  TRACE_MSG(TRACE_APS3, ">> aps_in_frag_mac_ack_for_aps_ack_arrived frag_idx %hd block_num %hd block_ack 0x%x",
    (FMT__H_H_H, frag_idx, block_num, block_ack));

  ZVUNUSED(block_ack);

  ent = &ZG->aps.in_frag[frag_idx];

  if (ent->flags.state == APS_IN_FRAG_WINDOW_MERGED_NO_ACK)
  {
    TRACE_MSG(TRACE_APS3, "change frag pkt state from APS_IN_FRAG_WINDOW_MERGED_NO_ACK to APS_IN_FRAG_WINDOW_MERGED", (FMT__0));
    ent->flags.state = APS_IN_FRAG_WINDOW_MERGED;
  }

  /* Check - if confirmed APS ACK for last window */
  if ((block_num + ZG->aps.aib.aps_max_window_size) >= ent->total_blocks_num)
  {
    /* And whole fragmented packet is complete */
    if (ent->flags.state == APS_IN_FRAG_COMPLETE)
    {
      TRACE_MSG(TRACE_APS3, "frag pkt complete. apsde-data.indication buf %hd ent->flags.state %hd", (FMT__H_H, ent->buffer, ent->flags.state));
      ZB_SCHEDULE_CALLBACK(zb_apsde_data_indication, ent->buffer);
      ent->buffer = 0;
      aps_in_frag_stop_entry(ent, ZB_TRUE);
    }
    else
    {
      TRACE_MSG(TRACE_APS3, "frag pkt incomplete, turbo polling for 1 pkt", (FMT__0));
      /* Add turbo poll for one packet. Not one window! */
      TRACE_MSG(TRACE_ZDO3, "Call zb_zdo_pim_start_turbo_poll_packets 1", (FMT__0));
      zb_zdo_pim_start_turbo_poll_packets(1);
    }
  }
  else
  {
    /* Not last window, add turbo poll for one packet. Not one window! */
    TRACE_MSG(TRACE_ZDO3, "Call zb_zdo_pim_start_turbo_poll_packets 1", (FMT__0));
    zb_zdo_pim_start_turbo_poll_packets(1);
  }
  TRACE_MSG(TRACE_APS3, "<< aps_in_frag_mac_ack_for_aps_ack_arrived", (FMT__0));
}

static void aps_in_frag_stop_ack_timer(zb_aps_in_fragment_frame_t *ent)
{
  zb_uint8_t i;

  TRACE_MSG(TRACE_APS2, "aps_in_frag_stop_ack_timer ack_timer_scheduled %hd", (FMT__H, ent->flags.ack_timer_scheduled));

  if (ent->flags.ack_timer_scheduled)
  {
    ent->flags.ack_timer_scheduled = ZB_FALSE;

    /* We don't know for sure which buffer was used to schedule the function */
    for (i = 0; i < ZG->aps.aib.aps_max_window_size; i++)
    {
      if (ent->window_buffers[i] != 0U)
      {
        ZB_SCHEDULE_ALARM_CANCEL(aps_in_frag_send_ack_delayed, ent->window_buffers[i]);
      }
    }
  }
}

static void aps_in_frag_transaction_failed(zb_bufid_t buf)
{
  zb_aps_in_fragment_frame_t *ent;
  zb_uint8_t frag_idx;

  TRACE_MSG(TRACE_APS1, "aps_in_frag_transaction_failed buf %hd", (FMT__H, buf));
  frag_idx = aps_in_frag_find_by_buf(buf);
  ZB_ASSERT(frag_idx != ZB_INVALID_FRAG_ID);
  ent = &ZG->aps.in_frag[frag_idx];

  /* TODO: handle the case when there is an ongoing assembly that is hanging because of buffer
     shortage.
     Currently we drop the in frag transaction anyway and delayed buffer allocation continues
     to hang until the memory manager successfully handles that. After the allocation happens,
     we either replace buffer of another transaction occupying the same place or free the buffer.

     An alternative solution is not to release the entry in such case, but set a special state that
     would indicate that it needs to be freed after the buffer gets allocated.
  */

  aps_in_frag_stop_entry(ent, ZB_FALSE);
}

static void aps_in_frag_stop_fail_timer(zb_aps_in_fragment_frame_t *ent)
{
  zb_uint8_t i;

  /* We don't know for sure which buffer was used to schedule the function */
  for (i = 0; i < ZG->aps.aib.aps_max_window_size; i++)
  {
    if (ent->window_buffers[i] != 0U)
    {
      ZB_SCHEDULE_ALARM_CANCEL(aps_in_frag_transaction_failed, ent->window_buffers[i]);
    }
  }
}

static void aps_in_frag_restart_fail_timer(zb_aps_in_fragment_frame_t *ent)
{
  zb_bufid_t buf;
  zb_time_t timeout_bi;

  aps_in_frag_stop_fail_timer(ent);

  buf = aps_in_frag_find_any_block(ent);
  ZB_ASSERT(buf);

  /* DL: To synchronize the timeout with the sending side, we must
   * calculate the timeout based on our rx_on_when_idle state.
   * In the case where a sleepy ZED sends to the ZC,
   * ZED will use the "short" timeout, and the ZC - "long".
   */
  #if 0
  if (!ZB_PIBCACHE_RX_ON_WHEN_IDLE())
  {
    /* If I am a sleepy zed, wait longer */
    rx_on_when_idle = 0;
  }
  else
  {
    rx_on_when_idle = zb_nwk_get_nbr_rx_on_idle_short_or_false(ent->src_addr);
  }
  #endif
  /* The timeout is used for one window. Therefore, the window size
   * is taken into account. And one extra ZB_N_APS_ACK_WAIT_DURATION
   * is added so that the timeout of the receiving side is greater
   * than the timeout of the sender.
   */
  timeout_bi = ZB_N_APS_ACK_WAIT_DURATION(ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE())) * (ZB_N_APS_MAX_FRAME_RETRIES * (zb_time_t)ZG->aps.aib.aps_max_window_size + 1U);
  ZB_SCHEDULE_ALARM(aps_in_frag_transaction_failed, buf, timeout_bi);
}

static void aps_in_frag_restart_ack_timer(zb_uint8_t in_frag_idx)
{
  zb_bufid_t buf;
  /* zb_uint8_t rx_on_when_idle; */
  zb_time_t timeout_bi;
  zb_aps_in_fragment_frame_t *ent;

  TRACE_MSG(TRACE_APS1, "aps_in_frag_restart_ack_timer in_frag_idx %u", (FMT__D, in_frag_idx));

#ifdef ZB_CERTIFICATION_HACKS
  if (ZB_CERT_HACKS().frag_disable_custom_ack_timer)
  {
    return;
  }
#endif

  ent = &ZG->aps.in_frag[in_frag_idx];

  if (ent->flags.ack_timer_scheduled)
  {
    aps_in_frag_stop_ack_timer(ent);
  }

  ent->flags.ack_timer_scheduled = ZB_TRUE;

  buf = aps_in_frag_find_any_block(ent);
  ZB_ASSERT(buf);

  /* DL: On the receiving side it's better to use a timeout based on
   * our rx_on_when_idle. The timeout will match the transmitting side.
   */
  /* rx_on_when_idle = zb_nwk_get_nbr_rx_on_idle_short_or_false(ent->src_addr); */

  /* DL: On the receiving side (here), we reduce the timeout
   * so that it happens earlier than on the sender's side
   */
  timeout_bi = (ZB_N_APS_ACK_WAIT_DURATION(ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE())) * 5U) / 6U;
  ZB_SCHEDULE_ALARM(aps_in_frag_send_ack_delayed, buf, timeout_bi);
}

/*
   As recommended by the specification, we are persisting the transaction for
   apscAckWaitDuration after the transcation is successfully completed.
   If we receive again a block from the last window, then we'll resend APS ACK.
   This would help resolve situations when our APS ACK gets lost.

   Returns RET_OK if this buffer belongs to the completed transacation. The buffer will be used
   for APS ACK transmission.

   Returns RET_EXIT otherwise.
*/
static zb_ret_t aps_in_frag_ack_completed_transaction(zb_bufid_t param, zb_aps_hdr_t *aps_hdr)
{
  zb_uint8_t frag_idx;
  zb_aps_in_fragment_frame_t *ent;

  TRACE_MSG(TRACE_APS2, ">> aps_in_frag_ack_completed_transaction (param %hd src_addr 0x%x aps_counter %u)",
            (FMT__H_D_D, param, aps_hdr->src_addr, aps_hdr->aps_counter));

  frag_idx = aps_in_frag_find_by_hdr(aps_hdr->src_addr, aps_hdr->aps_counter);

  if (frag_idx == ZB_INVALID_FRAG_ID)
  {
    TRACE_MSG(TRACE_APS3, "in frag entry not found", (FMT__0));
    ent = NULL;
  }
  else
  {
    ent = &ZG->aps.in_frag[frag_idx];
  }

  if (ent != NULL)
  {
    /* send APS ACK to this states */
    zb_bool_t out_state = ent->flags.state == APS_IN_FRAG_COMPLETE || ent->flags.state == APS_IN_FRAG_WINDOW_MERGED_NO_ACK;
    if (out_state != ZB_TRUE)
    {
      TRACE_MSG(TRACE_APS3, "in frag entry is not in the required state: %hd",
                (FMT__H, ent->flags.state));
      ent = NULL;
    }
    else
    {
      TRACE_MSG(TRACE_APS3, "in frag entry is in the required state: %hd",
                (FMT__H, ent->flags.state));
    }
  }

  if (ent != NULL)
  {
    aps_ack_send_handle(param, aps_hdr);

    return RET_OK;
  }
  else
  {
    return RET_EXIT;
  }
}

/*
  The function attempts to change the state of the incoming fragmented transaction.

  If we have just received all fragments of the latest window, then the function
  1) Sends an APS acknowledgement
  2) Assembles the window into the main transaction buffer

  If we have just received all fragments of the whole transaction, then the function
  1) Assembles latest window into the main transaction buffer
  2) Sends an APS acknowledgement
  3) Indicates the frame
  4) Stops the incoming fragmented transaction

  Returns RET_OK if the state of the transaction has changed. No further steps need to be done. The
  function will take care of freeing 'param' and 'ack_param'

  Returns RET_BUSY if the state of the transaction has not changed. The 'param' buffer must not be
  freed. 'ack_param' will be freed automatically.

  Returns RET_OPERATION_FAILED if some exceptional situation has happened (e.g. delayed buffer queue
  is full), and the transcation has been dropped. The 'param' and 'ack_param' have already beed freed.
*/
static zb_ret_t aps_in_frag_attempt_state_transition(zb_bufid_t param,
                                                     zb_bufid_t ack_param,
                                                     zb_aps_hdr_t *aps_hdr)
{
  zb_ret_t ret;
  zb_aps_in_fragment_frame_t *ent;
  zb_uint8_t last_window;
  zb_uint8_t frag_idx;

  ZVUNUSED(param);

  TRACE_MSG(TRACE_APS2, ">> aps_in_frag_attempt_state_transition (param %hd ack_param %hd src_addr 0x%x aps_counter %u)",
            (FMT__H_H_D_D, param, ack_param, aps_hdr->src_addr, aps_hdr->aps_counter));

  frag_idx = aps_in_frag_find_by_hdr(aps_hdr->src_addr, aps_hdr->aps_counter);
  ZB_ASSERT(frag_idx != ZB_INVALID_FRAG_ID);

  ent = &ZG->aps.in_frag[frag_idx];

  if (ent->total_blocks_num == 0U)
  {
    /* We have not yet received the first block. */
    TRACE_MSG(TRACE_APS3, "first blk not received yet", (FMT__0));
    ent = NULL;
  }
  else if (ent->flags.state != APS_IN_FRAG_RECEIVING)
  {
    TRACE_MSG(TRACE_APS3, "state %d - not APS_IN_FRAG_RECEIVING", (FMT__D, ent->flags.state));
    /* Other state transitions are triggered elsewhere. */
    ent = NULL;
  }
  else if (aps_in_frag_get_received_mask(ent) != 0xFFU)
  {
    TRACE_MSG(TRACE_APS3, "not all frags of the current window received - mask 0x%x", (FMT__D, aps_in_frag_get_received_mask(ent)));
    /* We have not received all fragments of the current window. */
    ent = NULL;
  }
  else
  {
    last_window = (ZG->aps.aib.aps_max_window_size - 1U + ent->total_blocks_num) / ZG->aps.aib.aps_max_window_size - 1U;
    TRACE_MSG(TRACE_APS3, "current_window %hd last_window %hd", (FMT__H_H, ent->current_window, last_window));
    if (ent->current_window == last_window)
    {
      TRACE_MSG(TRACE_APS3, "APS_IN_FRAG_COMPLETE state frag_idx %hd", (FMT__H, frag_idx));
      ent->flags.state = APS_IN_FRAG_COMPLETE;
      /* We have received all fragments of the whole transaction */
    }
    else
    {
      TRACE_MSG(TRACE_APS3, "APS_IN_FRAG_WINDOW_RECEIVED state frag_idx %hd", (FMT__H, frag_idx));
      ent->flags.state = APS_IN_FRAG_WINDOW_RECEIVED;
      /* We have completed the window */
    }
  }

  /* Sending an APS ack. */
  if (ent != NULL)
  {
    /* DL: Why schedule a delayed ACK when we confirm the whole window?
           Moreover, it is necessary to stop it.
    if (ZG->aps.aib.aps_max_window_size > 1)
    {
      aps_in_frag_restart_ack_timer(frag_idx);
      ent->flasg.aps_ack_retry_cnt = 0;
    }
    */
    aps_in_frag_stop_ack_timer(ent);

    if (ack_param != 0U)
    {
      TRACE_MSG(TRACE_APS3, "sending ack using param %hd", (FMT__H, ack_param));
      aps_in_frag_send_ack(ack_param, frag_idx);
      ack_param = 0;
    }
    else
    {
      TRACE_MSG(TRACE_APS3, "sending ack by allocating buf", (FMT__0));
      /* We don't really care if we didn't manage to put it into the delayed queue -
         if we don't ACK right now, it will be done via the timer. */
      (void)zb_buf_get_out_delayed_ext(aps_in_frag_send_ack, frag_idx, 0);
    }
  }

  if (ack_param != 0U)
  {
    zb_buf_free(ack_param);
  }

  /* Assembling the window */
  if (ent != NULL)
  {
    ret = aps_in_frag_assemble_window(frag_idx);
    if (ret != RET_OK)
    {
      aps_in_frag_stop_entry(ent, ZB_FALSE);
      ret = RET_OPERATION_FAILED;
    }
  }
  else
  {
    /* We didn't change state. */
    ret = RET_BUSY;
  }

  TRACE_MSG(TRACE_APS2, "<< aps_in_frag_attempt_state_transition ret %d", (FMT__D, ret));

  return ret;
}

/*
  Stores the buffer in the corresponding incoming fragmented transaction.
  If there was no transcation, it will be created and set up.

  Returns RET_OK if the packet is accepted, RET_EXIT if it should be dropped.
*/
static zb_ret_t aps_in_frag_remember_frame(zb_bufid_t param, zb_aps_hdr_t *aps_hdr)
{
  zb_aps_in_fragment_frame_t *ent = NULL;
  zb_bool_t start_window = ZB_FALSE;
  zb_bool_t new_entry = ZB_FALSE;
  zb_uint8_t frag_idx;
  zb_uint8_t block_no = 0; /* MISRA Rule-1.3_k, Rule-9.1_f. C-STAT is unable to parse that a block_no is not used uninitialized. */
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_APS1, ">> aps_in_frag_remember_frame (param %hd src_addr 0x%x aps_counter %u)",
            (FMT__H_D_D, param, aps_hdr->src_addr, aps_hdr->aps_counter));

  frag_idx = aps_in_frag_find_by_hdr(aps_hdr->src_addr, aps_hdr->aps_counter);
  if (frag_idx == ZB_INVALID_FRAG_ID)
  {
    if (ZB_APS_FC_GET_EXT_HDR_FRAG(aps_hdr->extended_fc) == ZB_APS_FIRST_FRAGMENT)
    {
      TRACE_MSG(TRACE_APS3, "First fragment - allocate transaction", (FMT__0));
      ent = aps_in_frag_allocate(aps_hdr->src_addr, aps_hdr->aps_counter);
    }
    else
    {
      TRACE_MSG(TRACE_APS3, "Not first fragment while no transaction - was it timed out?", (FMT__0));
    }
    if (ent != NULL)
    {
      new_entry = ZB_TRUE;
      frag_idx = (zb_uint8_t)(ent - &ZG->aps.in_frag[0]);
      ent->flags.state = APS_IN_FRAG_RECEIVING;
      TRACE_MSG(TRACE_APS2, "created a new incoming fragmented transaction", (FMT__0));
    }
    else
    {
      TRACE_MSG(TRACE_APS2, "failed to allocate a new entry", (FMT__0));
      ret = RET_EXIT;
    }
  }
  else
  {
    ent = &ZG->aps.in_frag[frag_idx];
  }

  /* Fetch the current block number */
  if (ret == RET_OK)
  {
    if (ZB_APS_FC_GET_EXT_HDR_FRAG(aps_hdr->extended_fc) == ZB_APS_FIRST_FRAGMENT
        && ent->total_blocks_num == 0U)
    {
      ent->total_blocks_num = aps_hdr->block_num;
      block_no = 0;
    }
    else if (ZB_APS_FC_GET_EXT_HDR_FRAG(aps_hdr->extended_fc) == ZB_APS_NOT_FIRST_FRAGMENT)
    {
#ifdef ZB_CERTIFICATION_HACKS
      if (ZB_CERT_HACKS().frag_drop_1st_block_1st_time && aps_hdr->block_num == 1U)
      {
        ZB_CERT_HACKS().frag_drop_1st_block_1st_time = 0;
        ret = RET_EXIT;
        TRACE_MSG(TRACE_APS2, "ZB_CERT_HACKS().frag_drop_1st_block_1st_time", (FMT__0));
      }
      else
#endif
      {
        block_no = aps_hdr->block_num;
      }
    }
    else
    {
      /* Something strange */
      ret = RET_EXIT;
      TRACE_MSG(TRACE_APS3, "skipping due to invalid header", (FMT__0));
    }
  }

  if (ret == RET_OK)// && ent->flags.state == APS_IN_FRAG_RECEIVING)
  {
    if (ent->flags.state == APS_IN_FRAG_RECEIVING)
    {
      if (aps_in_frag_is_block_within_window(ent->current_window, block_no, ent->total_blocks_num) == 0U)
      {
        /* We received a packet outside of the current window */
        ret = RET_EXIT;
        TRACE_MSG(TRACE_APS3, "block outside of current window", (FMT__0));
      }
#ifdef ZB_CERTIFICATION_HACKS
      else if (ZB_CERT_HACKS().frag_disable_custom_ack_timer
               && block_no == (ZG->aps.aib.aps_max_window_size - 1)
               && aps_in_frag_get_received_mask(ent) != 0xFF)
      {
      /* We received the last block, but the whole transaction is not ready ret.
         Spec says we must send an APS ack in such case, and we'll do so - but not right after it is
         received. In our implementation, it is the responsibility of ack timer.

         From TP/PRO/BV-44's description it seems that we should wait for
         "apsFragmentationRetryTimeoutseconds" before a retransmission, while the r22 spec says
         "The receiver may send an acknowledgement to the sender with the block or blocks missed.",
         and in such case the delay will be shorter.
      */
        zb_buf_get_out_delayed_ext(aps_in_frag_send_ack, frag_idx, 0);
      }
#endif
    }
    else if (ent->flags.state == APS_IN_FRAG_WINDOW_MERGED)
    {
      if (aps_in_frag_is_block_within_window(ent->current_window + 1U, block_no, ent->total_blocks_num))
      {
        /* Starting a new window */
        ent->current_window++;
        ent->flags.state = APS_IN_FRAG_RECEIVING;

        /* Stoping timers since they were using window buffers */

        if (ZG->aps.aib.aps_max_window_size > 1U)
        {
          aps_in_frag_stop_ack_timer(ent);
        }
        aps_in_frag_stop_fail_timer(ent);

        aps_in_frag_free_window(ent);

        start_window = ZB_TRUE;
      }
      else
      {
        /* We received a packet outside of the next window */
        ret = RET_EXIT;
      }
    }
    else
    {
      /* The transcation is currently not in the right state for storing new fragments */
      ret = RET_EXIT;
      TRACE_MSG(TRACE_APS3, "not in the receiving state", (FMT__0));
    }
  }
  else
  {
    /* MISRA rule 15.7 requires empty 'else' branch. */
  }

  if (ret == RET_OK)
  {
    /*cstat !MISRAC2012-Rule-1.3_* !MISRAC2012-Rule-9.1_* */
    /* Rule-9.1 and Rule-1.3: if ret == RET_OK then block_no is always
     * initialized.*/
    /* Remember the buffer, if we didn't receive it */
    if (ent->window_buffers[block_no % ZG->aps.aib.aps_max_window_size] == 0U)
    {
      ent->window_buffers[block_no % ZG->aps.aib.aps_max_window_size] = param;
    }
    else
    {
      zb_buf_free(param);
    }

    if (start_window || new_entry)
    {
#if defined ZB_ED_FUNC
      if (new_entry)
      {
        /* Caution, the function accepts milliseconds, not a BI */
        /* zb_zdo_pim_set_turbo_poll_max(100); */
      TRACE_MSG(TRACE_ZDO3, "Call zb_zdo_pim_start_turbo_poll_packets 1", (FMT__0));
        zb_zdo_pim_start_turbo_poll_packets(1);
#ifdef SNCP_MODE
        /* enable auto turbo poll for fragmented APS rx */
        sncp_auto_turbo_poll_aps_rx(ZB_TRUE);
#endif
      }
      else
      {
        /* Poll control for next fragments implemented in aps_in_frag_mac_ack_for_aps_ack_arrived() */
      }
#endif /* ZB_ED_FUNC */

      /* Restart timers */
      if (ZG->aps.aib.aps_max_window_size > 1U)
      {
        aps_in_frag_restart_ack_timer(frag_idx);
        ent->flags.aps_ack_retry_cnt = 0;
      }
      aps_in_frag_restart_fail_timer(ent);
    }
  }
  else if (ent != NULL && new_entry)
  {
    /* We shouldn't have created the entry. Free it. */
    aps_in_frag_stop_entry(ent, ZB_FALSE);
    TRACE_MSG(TRACE_APS2, "dropping the newly created entry", (FMT__0));
  }
  else
  {
    /* MISRA rule 15.7 requires empty 'else' branch. */
  }

  TRACE_MSG(TRACE_APS1, "<< aps_in_frag_remember_frame ret %hd", (FMT__H, ret));

  return ret;
}

#endif /* APS_FRAGMENTATION */

void zb_nlde_data_indication_continue(zb_uint8_t ack_param, zb_uint16_t param)
{
  zb_aps_hdr_t aps_hdr;
  zb_bool_t is_fragmented;
  zb_bool_t is_command;
  zb_bool_t pass_pkt_up = ZB_FALSE;
  zb_uint8_t param_u8 = (zb_uint8_t)param;
  zb_nwk_hdr_t *nwk_hdr = (zb_nwk_hdr_t *)zb_buf_begin(param_u8);
  zb_uint16_t keypair_i = (zb_uint16_t)-1;
  zb_secur_key_id_t key_id = ZB_NOT_SECUR;
  zb_bool_t decrypt_error = ZB_FALSE;
  zb_aps_dup_tbl_ent_t *ent = NULL;
  zb_bool_t is_verified_tclk = ZB_FALSE;
#ifdef ZB_PRO_STACK
#ifndef ZB_NO_NWK_MULTICAST
  zb_bool_t is_nwk_multicast = (zb_bool_t)ZB_NWK_FRAMECTL_GET_MULTICAST_FLAG(nwk_hdr->frame_control);
  zb_uint16_t nwk_group_addr = nwk_hdr->dst_addr;
#endif
#endif
  TRACE_MSG(TRACE_APS1, "> zb_nlde_data_indication_continue %hd", (FMT__H, param_u8));

  ZVUNUSED(nwk_hdr);

  /* Parse APS and NWK headers, then cut NWK header */
  zb_aps_hdr_parse(param_u8, &aps_hdr, ZB_TRUE);

  is_fragmented = (zb_bool_t)(ZB_APS_FC_GET_EXT_HDR_PRESENT(aps_hdr.fc) != 0U &&
    (ZB_APS_FC_GET_EXT_HDR_FRAG(aps_hdr.extended_fc) == ZB_APS_FIRST_FRAGMENT ||
     ZB_APS_FC_GET_EXT_HDR_FRAG(aps_hdr.extended_fc) == ZB_APS_NOT_FIRST_FRAGMENT));

  is_command = (zb_bool_t)(ZB_APS_FC_GET_FRAME_TYPE(aps_hdr.fc) == ZB_APS_FRAME_COMMAND);

  if ((!is_fragmented) && (!is_command))
  {
    /* Do not check dups for fragmented frames - it is done manually upper */
    /* Do not check dups for APS commands because they don't have ACK_request bit*/
    ent = aps_check_dups(aps_hdr.src_addr,
                         aps_hdr.aps_counter,
                         (zb_bool_t)(ZB_APS_FC_GET_DELIVERY_MODE(aps_hdr.fc) == ZB_APS_DELIVERY_UNICAST));
  }

  /* Detect and reject dup */
  if ((NULL != ent) && (0U != ent->clock))
  {
    TRACE_MSG(TRACE_APS2, "pkt #%d is a dup or fragment transaction is in progress - drop.", (FMT__D, aps_hdr.aps_counter));
    zb_buf_free(param_u8);
  }
  else
  {
    zb_ret_t unsecure_frame_ret = RET_OK;
    zb_bool_t have_verified_key = ZB_FALSE;

    ZB_MEMCPY(ZB_BUF_GET_PARAM(param_u8, zb_apsde_data_indication_t), &aps_hdr, sizeof(aps_hdr));

    if (ZB_APS_FC_GET_SECURITY(aps_hdr.fc) != 0U)
    {
      unsecure_frame_ret = zb_aps_unsecure_frame(param_u8, &keypair_i, &key_id, &is_verified_tclk);

#ifdef SNCP_MODE
      if (is_verified_tclk == ZB_FALSE)
      {
        have_verified_key = zb_secur_has_verified_key_by_short(nwk_hdr->src_addr);
      }
      TRACE_MSG(TRACE_SECUR2, "APS decryption ret %d is_verified_tclk %hd have_verified_key %hd",
                (FMT__D_H_H, unsecure_frame_ret, is_verified_tclk, have_verified_key));
#endif
    }
    if (ZB_APS_FC_GET_SECURITY(aps_hdr.fc) != 0U
        && (unsecure_frame_ret != RET_OK
#ifdef SNCP_MODE
        || (ZG->aps.authenticated && is_verified_tclk == ZB_FALSE && have_verified_key)
#endif
        ))
    {
      TRACE_MSG(TRACE_SECUR2, "APS frame decryption error", (FMT__0));
      /* Free packet here to simplify dealing with buffer leaks.  */
      zb_buf_free(param_u8);
      if (ack_param != 0U)
      {
        zb_buf_free(ack_param);
      }

      decrypt_error = ZB_TRUE;
      if (unsecure_frame_ret == RET_OK && is_verified_tclk == ZB_FALSE && have_verified_key)
      {
        ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_APS_UNAUTHORIZED_KEY_ID);
      }
#ifdef ZB_JOIN_CLIENT
      /* Client may leave & rejoin in case of decription failure. ZC never leaves. */
      ZB_SCHEDULE_CALLBACK(zdo_aps_decryption_failed, 0);
#endif
    }
    else
    {
      /* Update dups table and run aging*/
      if (ent != NULL)
      {
        aps_update_entry_clock_and_start_aging(ent);
      }
#if 0
      /* Why we should check for SE mode? Buf param will be filled only if key exists*/
      if (ZB_SE_MODE())
#endif
      {
        /* Save key_src & key_attr of the APS key used to unsecure this frame.
         *
         * NOTE: key == NULL means than frame has no security!
         *  - ZB_APS_FC_GET_SECURITY(aps_hdr.fc) == 0 and zb_aps_unsecure_frame() wasn't called
         *  - keypair_i == -1 and, thus, key wasn't found in a key-pair storage
         */
        zb_aps_device_key_pair_set_t *key = zb_aps_keypair_get_ent_by_idx(keypair_i);
        zb_apsde_data_indication_t *ind = ZB_BUF_GET_PARAM(param_u8, zb_apsde_data_indication_t);

        if (key != NULL)
        {
          ind->aps_key_source = key->key_source;
          ind->aps_key_attrs = key->key_attributes;
        }
        ind->aps_key_from_tc = ZB_B2U(is_verified_tclk);
      }

      if (is_fragmented)
      {
        /* Send APS ack before calling zb_aps_process_in_frag_frame function,
           because it resets aps fragmentation context. */
        TRACE_MSG(TRACE_APS1, "fragmented, do not try to decrypt ( decrypt_error: 0 )", (FMT__0));
        zb_nwk_unlock_in(param_u8);

#ifdef APS_FRAGMENTATION
        /* Facilitating recovery of lost APS acks */
        if (aps_in_frag_ack_completed_transaction(param_u8, &aps_hdr) == RET_OK)
        {
          if (ack_param != 0U)
          {
            zb_buf_free(ack_param);
          }

          return;
        }

        if (aps_in_frag_remember_frame(param_u8, &aps_hdr) != RET_OK)
        {
          if (ack_param != 0U)
          {
            zb_buf_free(ack_param);
          }

          zb_buf_free(param_u8);
          zb_zdo_pim_continue_polling_for_pkt();
          return;
        }

        if (aps_in_frag_attempt_state_transition(param_u8, ack_param, &aps_hdr) == RET_OK)
        {
          /* The frame will be indicated after it is reassembled */
          return;
        }
        else
        {
          /* The frame is either busy or the transcation is invalidated and the frame has already
             been freed. The function takes care of 'ack_param' regardless of the return code.
          */
          zb_zdo_pim_continue_polling_for_pkt();
          return;
        }
#else
        /* Drop fragmented frames. */
        zb_buf_free(param_u8);
        zb_zdo_pim_continue_polling_for_pkt();
        return;
#endif
      }

      if (ZB_APS_FC_GET_FRAME_TYPE(aps_hdr.fc) == ZB_APS_FRAME_COMMAND)
      {
        /* Q: don't we need there aps_key_source & attr?
         * A: I think no for APS commands. All cluster security-related checks made
         *    on ZCL level.
         *    Moreover, I moved attributes parsing above. Thus, they are available
         *    here if needed.
         */
        zb_aps_in_command_handle(param_u8, keypair_i
                                 , key_id
                                );
      }
      else if (!(ZB_APS_FC_GET_FRAME_TYPE(aps_hdr.fc) == ZB_APS_FRAME_DATA
                 && ((ZB_APS_FC_GET_DELIVERY_MODE(aps_hdr.fc) == ZB_APS_DELIVERY_GROUP)
#if defined ZB_PRO_STACK && !defined ZB_NO_NWK_MULTICAST
                     || (is_nwk_multicast)
#endif
                     )))
      {
        TRACE_MSG(TRACE_APS3, "data pkt", (FMT__0));
        ZB_SCHEDULE_CALLBACK(zb_apsde_data_indication, param_u8);
        pass_pkt_up = ZB_TRUE;
      }
      else
      {
        TRACE_MSG(TRACE_APS3, "data pkt - group addressed", (FMT__0));
#if defined ZB_PRO_STACK && !defined ZB_NO_NWK_MULTICAST
        if (is_nwk_multicast)
        {
          TRACE_MSG(TRACE_APS3, "data pkt - group msg by NWK multicast", (FMT__0));
          if ( zb_aps_is_in_group(nwk_group_addr) )
          {
            zb_apsde_data_indication_t *ind = ZB_BUF_GET_PARAM(param_u8, zb_apsde_data_indication_t);
            ind->group_addr = nwk_group_addr;
            ZB_SCHEDULE_CALLBACK(zb_aps_pass_up_group_buf, param_u8);
            pass_pkt_up = ZB_TRUE;
          }
        }
        else
#endif
        {
          TRACE_MSG(TRACE_APS3, "data pkt - group msg by APS multicast: for 0x%04x", (FMT__D, aps_hdr.group_addr));
          if ( zb_aps_is_in_group(aps_hdr.group_addr) )
          {
            TRACE_MSG(TRACE_APS3, "zb_aps_is_in_group = true => zb_aps_pass_up_group_buf", (FMT__0));
            ZB_SCHEDULE_CALLBACK(zb_aps_pass_up_group_buf, param_u8);
            pass_pkt_up = ZB_TRUE;
          }
          else
          {
            zb_buf_free(param_u8);
          }
        } /* else (nwk multicast) */
      } /* else (group) */
    } /* else (unsecured ok) */
  } /* else (not dup) */
  /* From this moment - if APS pkt is passed up, assume that it is what we are waiting for (do not
   * increment poll pkt counter). */

  if (!is_fragmented)
  {
    /* If data is fragmented, APS ack have been sent above */
    TRACE_MSG(TRACE_APS1, "decrypt_error: %hd", (FMT__H, decrypt_error));
    if (ack_param != 0U && (!decrypt_error))
    {
      aps_ack_send_handle(ack_param, &aps_hdr);
    }
  }

  if (!pass_pkt_up)
  {
    zb_zdo_pim_continue_polling_for_pkt();
  }

  TRACE_MSG(TRACE_APS1, "< zb_nlde_data_indication_continue", (FMT__0));
}

/*
  Process input network packet and fills given zb_aps_hdr_t aps_hdt from nwk packet's fields.
  Can also cut header of nwk packet for futher processing if cut_nwk_hdr == ZB_TRUE.
*/
void zb_aps_hdr_parse(zb_bufid_t packet, zb_aps_hdr_t *aps_hdr, zb_bool_t cut_nwk_hdr)
{
  zb_nwk_hdr_t *nwk_hdr = (zb_nwk_hdr_t *)zb_buf_begin(packet);
  zb_uint8_t *apshdr;
  zb_uint16_t group_addr;
  zb_uint16_t clusterid;
  zb_uint16_t profileid;

  /* Parse NWK header */
  /* get src and dst address from the NWK header */
  /* if packet is stored in APS retransmit queue it doesn't have
   * nwk header */

  ZB_BZERO(aps_hdr, sizeof(*aps_hdr));
  if (cut_nwk_hdr)
  {
    zb_apsde_data_ind_params_t *mac_addrs = ZB_BUF_GET_PARAM(packet, zb_apsde_data_ind_params_t);
    /* Pass up mac addresses.

       When cutitng nwk hdr, we have
       indication parameters as well. When not, we need not care about
       filling mac data.

       If this is OUT packet, there will be
       ZB_MAC_SHORT_ADDR_NOT_ALLOCATED in addresses and 0 in LQI and
       RSSI - that seems ok.
     */
    aps_hdr->mac_src_addr = mac_addrs->mac_src_addr;
    aps_hdr->mac_dst_addr = mac_addrs->mac_dst_addr;
    aps_hdr->lqi = mac_addrs->lqi;
    aps_hdr->rssi = mac_addrs->rssi;

    aps_hdr->src_addr = nwk_hdr->src_addr;
    aps_hdr->dst_addr = nwk_hdr->dst_addr;
    /* Remove NWK header from the packet */
    ZB_NWK_HDR_CUT(packet, apshdr);
  }
  else
  {
    /* src and dst addr are not available in this case */
    aps_hdr->src_addr = 0;
    aps_hdr->dst_addr = 0;
    apshdr = zb_buf_begin(packet);
  }

  aps_hdr->src_endpoint = (zb_uint8_t)~0U;
  aps_hdr->fc = *apshdr;
  apshdr++;

  if (
    /* Data frame always has address fields - see 2.2.5.2.1.1 Data Frame APS Header Fields */
    ZB_APS_FC_GET_FRAME_TYPE(aps_hdr->fc) == ZB_APS_FRAME_DATA
    /* ACK frame has address fields if no ack format bit set - see
     * 2.2.5.1.1.3 Ack Format Field */
    || (ZB_APS_FC_GET_FRAME_TYPE(aps_hdr->fc) == ZB_APS_FRAME_ACK
        && ZB_APS_FC_GET_ACK_FORMAT(aps_hdr->fc) == 0U)
    /* Command frame has no address fields - see 2.2.5.2.2 APS Command Frame Format */
    )
  {
    if (ZB_APS_FC_GET_FRAME_TYPE(aps_hdr->fc) == ZB_APS_FRAME_ACK
        || ZB_APS_FC_GET_DELIVERY_MODE(aps_hdr->fc) != ZB_APS_DELIVERY_GROUP)
    {
      /* has endpoint if not group addressing. No group addressing for ACKs */
      aps_hdr->dst_endpoint = *apshdr;
      apshdr++;
    }
    else
    {
      zb_get_next_letoh16(&group_addr, &apshdr);
      aps_hdr->group_addr = group_addr;

      TRACE_MSG(TRACE_APS3, "Group addressing, group 0x%x", (FMT__D, aps_hdr->group_addr));
    }
    /* if data or ack, has cluster and profile id */
    /* Not sure pointer is aligned to 2. Use macro. */
    zb_get_next_letoh16(&clusterid, &apshdr);
    aps_hdr->clusterid = clusterid;

    zb_get_next_letoh16(&profileid, &apshdr);
    aps_hdr->profileid = profileid;

    aps_hdr->src_endpoint = *apshdr;
    apshdr++;
  }

  aps_hdr->aps_counter = *apshdr;

#ifdef APS_FRAGMENTATION
  if (ZB_APS_FC_GET_EXT_HDR_PRESENT(aps_hdr->fc) != 0U)
  {
    TRACE_MSG(TRACE_APS3, "Ext hdr present!", (FMT__0));
    apshdr++;
    aps_hdr->extended_fc = *apshdr;
    if (ZB_APS_FC_GET_EXT_HDR_FRAG(aps_hdr->extended_fc) == ZB_APS_FIRST_FRAGMENT ||
        ZB_APS_FC_GET_EXT_HDR_FRAG(aps_hdr->extended_fc) == ZB_APS_NOT_FIRST_FRAGMENT)
    {
      apshdr++;
      aps_hdr->block_num = *apshdr;
      apshdr++;
      aps_hdr->block_ack = *apshdr;
      TRACE_MSG(TRACE_APS3, "block_num %hd block_ack %hd", (FMT__H_H, aps_hdr->block_num, aps_hdr->block_ack));
    }
  }

#endif
#if (defined ZB_ENABLE_SE_MIN_CONFIG)
  aps_hdr->aps_key_source = ZB_SECUR_KEY_SRC_UNKNOWN;
  /* W: Use some "illegal" value instead?
   * DV  Not necessary as this initialization is just a pro-forma. These attributes
   *    will be explicitly assigned later at APS level.
   *    Moreover, 'illegal value' == ZB_SECUR_ANY_KEY_ATTR doesn't fit the bitfield size.
   */
  aps_hdr->aps_key_attrs = ZB_SECUR_PROVISIONAL_KEY;
#endif
  aps_hdr->aps_key_from_tc = 0;
}


static void zb_aps_hdr_mod(zb_bufid_t packet, zb_aps_hdr_t *aps_hdr)
{
  zb_uint8_t *apshdr;

  /* Parse NWK header */
  /* get src and dst address from the NWK header */
  /* if packet is stored in APS retransmit queue it doesn't have
   * nwk header */


  apshdr = zb_buf_begin(packet);

  *apshdr = aps_hdr->fc;

   apshdr++;

  if (
    /* Data frame always has address fields - see 2.2.5.2.1.1 Data Frame APS Header Fields */
    ZB_APS_FC_GET_FRAME_TYPE(aps_hdr->fc) == ZB_APS_FRAME_DATA
    /* ACK frame has address fields if no ack format bit set - see
     * 2.2.5.1.1.3 Ack Format Field */
    || (ZB_APS_FC_GET_FRAME_TYPE(aps_hdr->fc) == ZB_APS_FRAME_ACK
        && ZB_APS_FC_GET_ACK_FORMAT(aps_hdr->fc) == 0U)
    /* Command frame has no address fields - see 2.2.5.2.2 APS Command Frame Format */
    )
  {
    if (ZB_APS_FC_GET_FRAME_TYPE(aps_hdr->fc) == ZB_APS_FRAME_ACK
        || ZB_APS_FC_GET_DELIVERY_MODE(aps_hdr->fc) != ZB_APS_DELIVERY_GROUP)
    {
      /* has endpoint if not group addressing. No group addressing for ACKs */
      *apshdr = aps_hdr->dst_endpoint;
       apshdr++;
    }
    else
    {
      ZB_MEMCPY(apshdr, (zb_uint8_t *)&aps_hdr->group_addr, sizeof(zb_uint16_t));
      TRACE_MSG(TRACE_APS3, "Group addressing, group %d", (FMT__D, aps_hdr->group_addr));
    }
    /* if data or ack, has cluster and profile id */
    /* Not sure pointer is aligned to 2. Use macro. */
    ZB_HTOLE16(apshdr, (zb_uint8_t *)&aps_hdr->clusterid);
    apshdr += sizeof(aps_hdr->clusterid);
    ZB_HTOLE16(apshdr, (zb_uint8_t *)&aps_hdr->profileid);
    apshdr += sizeof(aps_hdr->profileid);
    *apshdr = aps_hdr->src_endpoint;
    apshdr++;
  }

  *apshdr = aps_hdr->aps_counter;
  if (ZB_APS_FC_GET_EXT_HDR_PRESENT(aps_hdr->fc) != 0U)
  {
    TRACE_MSG(TRACE_APS3, "Ext hdr present!", (FMT__0));
  }
}


void zb_aps_pass_group_msg_up(zb_uint8_t new_buf, zb_uint16_t param)
{
  zb_aps_hdr_t *aps_hdr;
  zb_uint16_t group_addr;
  zb_ushort_t g_i;
  zb_uint8_t handle;
  zb_bool_t schedule_next = ZB_FALSE;
  zb_uint8_t param_u8 = (zb_uint8_t)param;
  handle = zb_buf_get_handle(param_u8);
  aps_hdr = ZB_BUF_GET_PARAM(param_u8, zb_aps_hdr_t);
  group_addr = aps_hdr->group_addr;

  TRACE_MSG(TRACE_APS1, "+zb_aps_pass_group_msg_up new_param %hd param %hd",
            (FMT__H_H, new_buf, param_u8));

  for (g_i = 0;
       g_i < ZG->aps.group.n_groups
         && ZG->aps.group.groups[g_i].group_addr != group_addr; ++g_i)
  {
  }

  TRACE_MSG(TRACE_APS3, "group_addr 0x%x index %d", (FMT__D_D, group_addr, g_i));
  if (g_i == ZG->aps.group.n_groups
      || handle >= ZG->aps.group.groups[g_i].n_endpoints)
  {
    zb_buf_free(param_u8);
    if (new_buf != 0U)
    {
      zb_buf_free(new_buf);
    }
  }
  else
  {
    if (new_buf == 0U)
    {
      aps_hdr->dst_endpoint = ZG->aps.group.groups[g_i].endpoints[handle];
      ZB_SCHEDULE_CALLBACK(zb_apsde_data_indication, param_u8);
    }
    else
    {
      zb_buf_copy(new_buf, param_u8);
      aps_hdr = ZB_BUF_GET_PARAM(new_buf, zb_aps_hdr_t);
      aps_hdr->dst_endpoint = ZG->aps.group.groups[g_i].endpoints[handle];

      TRACE_MSG(TRACE_APS3, "buf_handle %hd endpoint %hd",
                (FMT__H_H, handle, aps_hdr->dst_endpoint));
      /* Add parenthesis around 'handle + 1' to suppress false-positive from IAR
       * about missing parenthesis for zb_buf_set_handle() parameter. */
      /* TODO: why zb_buf_set_handle() is needed here?? */
      zb_buf_set_handle(param_u8, (handle + 1U));

      ZB_SCHEDULE_CALLBACK(zb_apsde_data_indication, new_buf);
      schedule_next = ZB_TRUE;
    }
  }

  if (schedule_next)
  {
      zb_nwk_unlock_in(param_u8);
      if (zb_buf_get_out_delayed_ext(zb_aps_pass_group_msg_up, param, 0) != RET_OK)
      {
        ZB_SCHEDULE_CALLBACK2(zb_aps_pass_group_msg_up, 0, param);
      }
    }

  TRACE_MSG(TRACE_APS1, "-zb_aps_pass_group_msg_up", (FMT__0));
}

/**
   Checking the existence of the endpoint by group address in group table
   @param [in] group_id - group address
   @param [in] endpoint - endpoint id
   @return ZB_TRUE if it was found and ZB_FALSE otherwise
*/
zb_bool_t zb_aps_is_endpoint_in_group(zb_uint16_t group_id, zb_uint8_t endpoint)
{
  return zb_aps_group_table_is_endpoint_in_group(&ZG->aps.group, group_id, endpoint);
}

void zb_aps_send_command(zb_uint8_t param, zb_uint16_t dest_addr, zb_uint8_t command, zb_bool_t secure,
                         zb_uint8_t secure_aps_i
  )
{
#ifndef ZB_APS_DISABLE_ACK_IN_COMMANDS
  zb_ushort_t need_ack = !ZB_NWK_IS_ADDRESS_BROADCAST(dest_addr);
  TRACE_MSG(TRACE_SECUR3, "zb_aps_send_command need_ack %hd", (FMT__H, need_ack));
#endif /* ZB_APS_DISABLE_ACK_IN_COMMANDS */

  zb_secur_key_id_t secure_aps = (zb_secur_key_id_t)secure_aps_i;

#ifdef ZB_CERTIFICATION_HACKS
  TRACE_MSG(TRACE_SECUR3,
            "zb_aps_send_command param %hd dest_addr 0x%x nwk secur %hd secure_key_i # %hd send_unencr %hd",
            (FMT__H_D_H_H_H, param, dest_addr, secure, secure_aps,
             ZB_CERT_HACKS().send_update_device_unencrypted));
#else
  TRACE_MSG(TRACE_SECUR3,
            "zb_aps_send_command param %hd dest_addr 0x%x nwk secur %hd secure_key_i # %hd",
            (FMT__H_D_H_H, param, dest_addr, secure, secure_aps));
#endif /* ZB_CERTIFICATION_HACKS */

  TRACE_MSG(TRACE_SECUR3, ">>zb_aps_send_command param %hd cmd %hd secur %hd "
     "secur_aps %hd to 0x%x",
     (FMT__H_H_H_H_D, param, command, secure, secure_aps_i, dest_addr));

#ifndef ZB_APS_DISABLE_ACK_IN_COMMANDS /* in r21 ack in commands are officially
                                        * prohibited, before r21 - "everybody knows". */
  if (need_ack)
  {
    zb_uindex_t i;
    for (i = 0 ; i < ZB_N_APS_RETRANS_ENTRIES ; ++i)
    {
      if (ZG->aps.retrans.hash[i].state == ZB_APS_RETRANS_ENT_FREE)
      {
        ZG->aps.retrans.hash[i].addr = dest_addr;
        ZG->aps.retrans.hash[i].aps_counter = ZB_AIB_APS_COUNTER();
        ZG->aps.retrans.hash[i].buf = param;
        ZG->aps.retrans.hash[i].nwk_insecure = !secure;
        /* APS sending logic counts in origin packet as retransmission.
         * Increase aps_retries by 1 to have correct number of retransmissions.
         */
        ZG->aps.retrans.hash[i].aps_retries = (ZB_N_APS_MAX_FRAME_RETRIES + 1);
        ZG->aps.retrans.hash[i].state = ZB_APS_RETRANS_ENT_SENT_MAC_NOT_CONFIRMED_ALRM_RUNNING;
        ZG->aps.retrans.n_packets++;
        TRACE_MSG(TRACE_APS2, "Store buf %hd len %hd in retrans hash %d", (FMT__H_H_D, param, zb_buf_len(buf), i));

        DUMP_TRAF("sending aps cmd", zb_buf_begin(param), zb_buf_len(buf), 0);

        break;
      }
    }
    if (i == ZB_N_APS_RETRANS_ENTRIES)
    {
      TRACE_MSG(TRACE_APS2, "ACK table is FULL", (FMT__0));
    }
    else
    {
      zb_uint8_t rx_on_when_idle = zb_nwk_get_nbr_rx_on_idle_short_or_false(dest_addr);

      TRACE_MSG(TRACE_APS2, "schedule zb_aps_ack_timer_cb time %u", (FMT__D, ZB_N_APS_ACK_WAIT_DURATION(rx_on_when_idle)));
      //ZB_SCHEDULE_ALARM(zb_aps_ack_timer_cb, i, ZB_N_APS_ACK_WAIT_DURATION(rx_on_when_idle));
      zb_schedule_alarm(zb_aps_ack_timer_cb, i, ZB_N_APS_ACK_WAIT_DURATION(rx_on_when_idle));
    }
  }
#endif  /* ZB_APS_DISABLE_ACK_IN_COMMANDS */

  /* Fill APS command header - see 2.2.5.2.2  APS Command Frame Format.
     At the same time alloc and fill aux security header
  */

  {
    zb_aps_command_header_t *hdr;

    zb_buf_flags_clr_encr(param);
    if (secure_aps != ZB_NOT_SECUR)
    {
      /* Allocate here space for APS command header, aux header and command id
       * (it is in payload). */
      zb_aps_command_add_secur(param, command, secure_aps);
      hdr = (zb_aps_command_header_t *)zb_buf_begin(param);
      TRACE_MSG(TRACE_SECUR3, "Will secure at APS, key type %hd", (FMT__H, secure_aps));
      zb_buf_flags_or(param, ZB_BUF_SECUR_APS_ENCR);
    }
    else
    {
      /* no security - just aps command header */
      hdr = zb_buf_alloc_left(param, sizeof(*hdr));
      TRACE_MSG(TRACE_SECUR3, "no aps security", (FMT__0));
      hdr->fc = 0;
      hdr->aps_command_id = command;
    }
#ifndef ZB_APS_DISABLE_ACK_IN_COMMANDS
    ZB_APS_FC_SET_COMMAND(hdr->fc, need_ack);
#else
    /* according to r21 spec (4.4.10) Ack Requet and Ack Format bits shall be set to 0 */
    ZB_APS_FC_SET_FRAME_TYPE(hdr->fc, ZB_APS_FRAME_COMMAND);
#endif
    ZB_APS_FC_SET_DELIVERY_MODE(hdr->fc, ZB_NWK_IS_ADDRESS_BROADCAST(dest_addr)?
                                         ZB_APS_DELIVERY_BROADCAST:
                                         ZB_APS_DELIVERY_UNICAST);
    hdr->aps_counter = ZB_AIB_APS_COUNTER();

    /* Add one more addr lock for dst (if it is unicast).
       Rationale: This addr may be released during the transmission (ed aging etc). */
    if (ZB_APS_FC_GET_DELIVERY_MODE(hdr->fc) == ZB_APS_DELIVERY_UNICAST)
    {
      zb_address_ieee_ref_t addr_ref;
      zb_ret_t ret = RET_OK;
      ZVUNUSED(ret);

      /* It is unusial, but still possible that addr is not in map here. In that case lets
       * create it and add lock (hope that if this address is found or not - in any case - we will
       * finally come to confirm which will release that lock). */
      ret = zb_address_by_short(dest_addr, /* create */ ZB_TRUE, /* lock */ ZB_TRUE, &addr_ref);
      ZB_ASSERT(ret == RET_OK);               /* Short addr should be in addr map! */
    }
  }
  ZB_AIB_APS_COUNTER_INC();

  fill_nldereq(param, dest_addr, ZB_B2U(secure));

  TRACE_MSG(TRACE_SECUR3, "send APS cmd %hd secur %hd param %hd to 0x%x", (FMT__H_H_H_D, command, secure, param, dest_addr));

  ZB_SCHEDULE_CALLBACK(zb_nlde_data_request, param);
}

void fill_nldereq(zb_uint8_t param, zb_uint16_t addr, zb_uint8_t secure)
{
  zb_nlde_data_req_t *nldereq = ZB_BUF_GET_PARAM(param, zb_nlde_data_req_t);
  ZB_BZERO(nldereq, sizeof(zb_nlde_data_req_t));
#if defined ZB_NWK_STOCHASTIC_ADDRESS_ASSIGN
  nldereq->radius = 2U * ZB_NWK_STOCH_DEPTH;
#else
  nldereq->radius = 2U * ZB_NWK_TREE_ROUTING_DEPTH;
#endif
  nldereq->addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
  nldereq->discovery_route = 1;
  nldereq->dst_addr = addr;
  nldereq->security_enable = secure;
}

#ifdef APS_FRAGMENTATION
static void aps_in_frag_fill_ack_hdr(zb_aps_hdr_t *aps_hdr, zb_uint8_t **apshdr)
{
  zb_uint8_t blocks_acked_mask;
  zb_uint8_t block_num;
  zb_aps_in_fragment_frame_t *ent;
  zb_uint8_t frag_idx;

  frag_idx = aps_in_frag_find_by_hdr(aps_hdr->src_addr, aps_hdr->aps_counter);
  ZB_ASSERT(frag_idx != ZB_INVALID_FRAG_ID);

  ent = &ZG->aps.in_frag[frag_idx];

  TRACE_MSG(TRACE_APS2, "fragmentation ack! current_window %hd blocks_num %hd",
            (FMT__H_H, ent->current_window, ent->total_blocks_num));

  /* Skip ZG->aps.out_frag.current_window blocks - they are already acked */
  block_num = ent->current_window * ZG->aps.aib.aps_max_window_size;
  blocks_acked_mask = aps_in_frag_get_received_mask(ent);

  /* extended_fc */
  **apshdr = 0;
  ZB_APS_FC_SET_EXT_HDR_FRAG(
    **apshdr,
    block_num > 0U ? ZB_APS_NOT_FIRST_FRAGMENT : ZB_APS_FIRST_FRAGMENT);
  (*apshdr)++;
  /* The block number field shall contain the value of the lowest block number in the current
     receive window, using the value 0 as the value of the first block.
  */

  TRACE_MSG(TRACE_APS2, "first block in window %hd", (FMT__H, ent->current_window * ZG->aps.aib.aps_max_window_size));
  **apshdr = ent->current_window * ZG->aps.aib.aps_max_window_size;
  (*apshdr)++;
  /* The first bit of the ack bitfield shall be set to 1 if the first fragment in the current
     receive window has been correctly received, and 0 otherwise. Subsequent bits shall be set
     similarly, with values corresponding to subsequent fragments in the current receive
     window. If apsMaxWindowSize is less than 8, then the remaining bits shall be set to 1.
  */
  TRACE_MSG(TRACE_APS2, "blocks acked mask %hd", (FMT__H, blocks_acked_mask));
  **apshdr = blocks_acked_mask;
  (*apshdr)++;
}
#endif  /* #ifdef APS_FRAGMENTATION */

static void aps_ack_send_handle(zb_bufid_t packet, zb_aps_hdr_t *aps_hdr)
{
  zb_uint8_t aps_hdr_size;
  zb_uint8_t *apshdr;
  zb_uint8_t fc = 0;
#ifdef APS_FRAGMENTATION
  zb_bool_t is_fragmented = (zb_bool_t)(ZB_APS_FC_GET_EXT_HDR_PRESENT(aps_hdr->fc) != 0U &&
    (ZB_APS_FC_GET_EXT_HDR_FRAG(aps_hdr->extended_fc) == ZB_APS_FIRST_FRAGMENT ||
     ZB_APS_FC_GET_EXT_HDR_FRAG(aps_hdr->extended_fc) == ZB_APS_NOT_FIRST_FRAGMENT));
#endif

  TRACE_MSG(TRACE_APS2, "+aps_ack_send_handle %hd", (FMT__H, packet));
  ZB_APS_FC_SET_FRAME_TYPE(fc, ZB_APS_FRAME_ACK);

  if (ZB_APS_FC_GET_FRAME_TYPE(aps_hdr->fc) != ZB_APS_FRAME_COMMAND)
  {
    aps_hdr_size = (2U +                   /* fc + aps counter */
                    2U +                   /* src & dest endpoint */
                    4U                     /* cluster id, profile id */
#ifdef APS_FRAGMENTATION
                    + (is_fragmented ? 3U : 0U) /* extended fc + block number + ack bitfield */
#endif
      );
    ZB_APS_FC_SET_ACK_FORMAT(fc, 0U);
  }
  else
  {
    aps_hdr_size = 2U           /* fc + aps counter */
#ifdef APS_FRAGMENTATION
                   + (is_fragmented ? 3U : 0U) /* extended fc + block number + ack bitfield */
#endif
      ;
    ZB_APS_FC_SET_ACK_FORMAT(fc, 1U);
  }

  /* If pkt was APS secured, reply with APS secured ACK */
  if (ZB_APS_FC_GET_SECURITY(aps_hdr->fc) != 0U)
  {
    TRACE_MSG(TRACE_APS2, "SECURE ACK: dst_endpoint 0x%x, cluster 0x%x", (FMT__D_D, aps_hdr->dst_endpoint, aps_hdr->clusterid));
    ZB_APS_FC_SET_SECURITY(fc, 1U);
    aps_hdr_size += (zb_uint8_t)sizeof(zb_aps_data_aux_frame_hdr_t);
  }

  apshdr = zb_buf_initial_alloc(packet, aps_hdr_size);

#ifdef APS_FRAGMENTATION
  if (is_fragmented)
  {
    ZB_APS_FC_SET_EXT_HDR_PRESENT(fc, 1U);
  }
#endif

  *apshdr = fc;
  apshdr++;

  if (ZB_APS_FC_GET_ACK_FORMAT(fc) == 0U)
  {
    *apshdr = aps_hdr->src_endpoint;
    apshdr++;

    TRACE_MSG(TRACE_APS2, "clusterid 0x%x", (FMT__D, aps_hdr->clusterid));
    ZB_PUT_NEXT_HTOLE16(apshdr, aps_hdr->clusterid);

    ZB_PUT_NEXT_HTOLE16(apshdr, aps_hdr->profileid);

    TRACE_MSG(TRACE_APS2, "src_endpoint %d", (FMT__D, aps_hdr->src_endpoint));
    *apshdr = aps_hdr->dst_endpoint;
    apshdr++;
  }
  *apshdr = aps_hdr->aps_counter;
  apshdr++;

#ifdef APS_FRAGMENTATION
  if (is_fragmented)
  {
    aps_in_frag_fill_ack_hdr(aps_hdr, &apshdr);
  }
#endif

  if (ZB_APS_FC_GET_SECURITY(fc) != 0U)
  {
    zb_buf_flags_or(packet, ZB_BUF_SECUR_APS_ENCR);
    zb_secur_aps_aux_hdr_fill(apshdr,
                              ZB_SECUR_DATA_KEY,
                              ZB_FALSE /* ext_nonce */);
  }

  fill_nldereq(packet, aps_hdr->src_addr, 1);

  ZB_SCHEDULE_CALLBACK(zb_nlde_data_request, packet);

  TRACE_MSG(TRACE_APS2, "-aps_ack_send_handle", (FMT__0));
}


static zb_ret_t save_ack_data(zb_uint8_t param, zb_apsde_data_req_t *req, zb_uint8_t *ref)
{
  zb_uint8_t i = 0;
  zb_ret_t ret = RET_OK;
#ifdef APS_FRAGMENTATION
  zb_uint8_t j;
#endif

  TRACE_MSG(TRACE_APS2, "+save_ack_data %hd", (FMT__H, param));

  if (ZG->aps.retrans.n_packets == ZB_N_APS_RETRANS_ENTRIES)
  {
    TRACE_MSG(TRACE_APS2, "ACK table is FULL", (FMT__0));
    ret = ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_TABLE_FULL);
  }
  else
  {
    do
    {
      if (ZG->aps.retrans.hash[i].state == ZB_APS_RETRANS_ENT_FREE)
      {
        if (req->addr_mode != ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT)
        {
            ZG->aps.retrans.hash[i].addr = req->dst_addr.addr_short;
        }
        ZG->aps.retrans.hash[i].clusterid = req->clusterid;

#ifdef APS_FRAGMENTATION
        /* Check for fragmented tx is done by out_frag.total_blocks_num 0 */
        /* No cycle if block_num == 0. */
        for (j = 0; j < ZG->aps.out_frag.total_blocks_num; ++j)
        {
          if (ZG->aps.out_frag.block_ref[j] == param)
          {
            /* Fragments of one APS frame should have same APS counter */
            ZG->aps.retrans.hash[i].aps_counter = ZG->aps.out_frag.aps_counter;
            break;
          }
        }

        if (j == ZG->aps.out_frag.total_blocks_num)
#endif
        {
          /* handle counter overflow - decrement modulo 256: (n + 256 - 1) % 256 */
          ZG->aps.retrans.hash[i].aps_counter =
            (ZB_AIB_APS_COUNTER() + ZB_UINT8_MAX) % (ZB_UINT8_MAX + 1U);
        }
        ZG->aps.retrans.hash[i].src_endpoint = req->src_endpoint;
        ZG->aps.retrans.hash[i].dst_endpoint = req->dst_endpoint;
        ZG->aps.retrans.hash[i].buf = param;
        ZG->aps.retrans.n_packets++;
        /* In >= r21 data always encrypted at NWK. So always keep NWK secure flag (nwk_insecure - inverted).
           Next line is meaningless beause buffer has no security flags yet.
         */
        /* ZG->aps.retrans.hash[i].nwk_insecure = (ZB_BUF_FROM_REF(param)->u.hdr.encrypt_type & ZB_SECUR_NWK_ENCR); */
        /* APS sending logic counts in origin packet as retransmission.
         * Increase aps_retries by 1 to have correct number of retransmissions.
         */
        ZG->aps.retrans.hash[i].aps_retries = (ZB_N_APS_MAX_FRAME_RETRIES + 1U);
        ZG->aps.retrans.hash[i].state = ZB_APS_RETRANS_ENT_SENT_MAC_NOT_CONFIRMED_ALRM_RUNNING;
        *ref = i;
        TRACE_MSG(TRACE_APS2, "save_ack_data %hd clusterid %d aps_counter %d src_endpoint %d dst_endpoint %d, i %hd",
                  (FMT__H_D_D_D_D_H, param, ZG->aps.retrans.hash[i].clusterid, ZG->aps.retrans.hash[i].aps_counter,
                   ZG->aps.retrans.hash[i].src_endpoint, ZG->aps.retrans.hash[i].dst_endpoint, i));
        break;
      }
      i++;
    } while (i < ZB_N_APS_RETRANS_ENTRIES);
    ZB_ASSERT(i < ZB_N_APS_RETRANS_ENTRIES);
  }

  TRACE_MSG(TRACE_APS2, "-save_ack_data %hd", (FMT__H, ret));
  return ret;
}


static void aps_ack_frame_handle(zb_aps_hdr_t *aps_hdr)
{
  zb_uint8_t i;
  zb_bool_t is_command = (ZB_APS_FC_GET_ACK_FORMAT(aps_hdr->fc) != 0U);

  TRACE_MSG(TRACE_APS2, "+aps_ack_frame_handle, aps_counter=%hd", (FMT__H, aps_hdr->aps_counter));

  /* First check if this ack was expected */
#ifdef APS_FRAGMENTATION
  if (ZB_APS_FC_GET_EXT_HDR_PRESENT(aps_hdr->fc) != 0U
      && ZB_APS_FC_GET_DELIVERY_MODE(aps_hdr->fc) == ZB_APS_DELIVERY_UNICAST
      && aps_hdr->block_num != ZG->aps.out_frag.current_window * ZG->aps.aib.aps_max_window_size)
  {
    /*
      We are currently acting in other window.
      This could happen if the receiver lost all our fragments of the current window,
      concluded that we didn't receive his ACK and decided to retransmit it.
    */
    TRACE_MSG(TRACE_APS2, "skipping aps ack from another window (block_num %hd current window %hd)",
              (FMT__H_H, aps_hdr->block_num, ZG->aps.out_frag.current_window));
    return;
  }
#endif

  /* Search for retransmit table entry */
  for ( i = 0 ; i < ZB_N_APS_RETRANS_ENTRIES ; ++i)
  {
    /* Not need to check status here: when entry freed, addr set to -1 */
    if (ZG->aps.retrans.hash[i].addr == aps_hdr->src_addr
        && ZG->aps.retrans.hash[i].aps_counter == aps_hdr->aps_counter
        && (is_command
            || (ZG->aps.retrans.hash[i].clusterid == aps_hdr->clusterid
                && ZG->aps.retrans.hash[i].src_endpoint == aps_hdr->dst_endpoint
                && (ZG->aps.retrans.hash[i].dst_endpoint == aps_hdr->src_endpoint ||
                    ZG->aps.retrans.hash[i].dst_endpoint == ZB_APS_BROADCAST_ENDPOINT_NUMBER))))
    {
      break;
    }
  }

  if (i < ZB_N_APS_RETRANS_ENTRIES)
  {
    TRACE_MSG(TRACE_APS2, "found ent %hd state %hd", (FMT__H_H, i, ZG->aps.retrans.hash[i].state));

    ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_APS_TX_UCAST_SUCCESS_ID);

    if (aps_retrans_ent_busy_state(i))
    {
      /* We are still waiting for MAC ACK: buffer is re-sent and is busy
       * now. MAC and NWK will use it to pass us confirm. Can't pass buffer up
       * and use it at upper layer. Let's wait for confirm from nwk - either
       * successful or not. */
      TRACE_MSG(TRACE_APS2, "ent %hd: got ACK while waiting for nwk confirm", (FMT__H, i));
      ZG->aps.retrans.hash[i].state = ZB_APS_RETRANS_ENT_SENT_MAC_NOT_CONFIRMED_APS_ACKED_ALRM_RUNNING;
#ifdef APS_FRAGMENTATION
      if (ZB_APS_FC_GET_EXT_HDR_PRESENT(aps_hdr->fc) != 0U &&
          ZB_APS_FC_GET_DELIVERY_MODE(aps_hdr->fc) == ZB_APS_DELIVERY_UNICAST)
      {
        TRACE_MSG(TRACE_APS2, "frag wait: block_num %hd block_ack %hd",
                  (FMT__H_H, aps_hdr->block_num, aps_hdr->block_ack));
        /* Fragmentation ack: store block_ack for future use */
        ZG->aps.out_frag.wait_block_num = aps_hdr->block_num;
        ZG->aps.out_frag.wait_block_ack = aps_hdr->block_ack;
      }
#endif
    }
    else
    {
      zb_uint8_t j;
      zb_ret_t ret = RET_OK;
      TRACE_MSG(TRACE_APS2, "packet %hd aps_cnt %hd acked ok", (FMT__H_H, ZG->aps.retrans.hash[i].buf, ZG->aps.retrans.hash[i].aps_counter));

      if (zb_check_bind_trans(ZG->aps.retrans.hash[i].buf) != RET_OK)
      {
        ret = RET_PENDING;
        aps_send_frame_via_binding(ZG->aps.retrans.hash[i].buf);
      }
      else
      {
        for ( j = 0 ; j < ZB_N_APS_RETRANS_ENTRIES ; ++j)
        {
          if (( ZG->aps.retrans.hash[j].buf == ZG->aps.retrans.hash[i].buf ) && (i != j))
          {
            TRACE_MSG(TRACE_APS2, "%hd and %hd retrans contain same buf %hd, block!", (FMT__H_H_H, i, j, ZG->aps.retrans.hash[i].buf));
            TRACE_MSG(TRACE_APS2, "aps_cnt i %hd aps_cnt j %hd", (FMT__H_H, ZG->aps.retrans.hash[i].aps_counter, ZG->aps.retrans.hash[j].aps_counter));
            ret = RET_BLOCKED;
            break;
          }
        }
      }
      done_with_this_ack(i, aps_hdr, ret);
    }
  }
  else
  {
    TRACE_MSG(TRACE_APS2, "No entry to this ACK - just drop it", (FMT__0));
    /* We are not waiting for this ack. */
    zb_zdo_pim_continue_polling_for_pkt();
  }

  TRACE_MSG(TRACE_APS2, "-aps_ack_frame_handle", (FMT__0));
}


#ifdef APS_FRAGMENTATION

static zb_bool_t all_fragmented_blocks_acked(void)
{
  zb_uint8_t i;
  for (i = 0; i < ZG->aps.out_frag.total_blocks_num; i++)
  {
    if (ZB_CHECK_BIT_IN_BIT_VECTOR(ZG->aps.out_frag.blocks_sent_mask, i) == 0U)
    {
      return ZB_FALSE;
    }
  }

  return ZB_TRUE;
}

static zb_ret_t done_with_frag_tx_ack(zb_ushort_t i, const zb_aps_hdr_t *aps_hdr, zb_ret_t status)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t j;
  zb_uint8_t fc = aps_hdr->fc;
  /* According to Zigbee spec the maximum number of fragments is 256
     ZBOSS supports 32 fragments now*/
  zb_uint8_t block_num;
  zb_bool_t reset_retry_count = ZB_FALSE;
  zb_uint8_t pkt_block_num = 0;
  zb_uint8_t pkt_block_ack = 0;
  zb_bool_t free_buf = ZB_FALSE;
  zb_address_ieee_ref_t ref;

  TRACE_MSG(TRACE_APS2, ">> done_with_frag_tx_ack buf %hu aps_cnt %hu status %d", (FMT__H_H_D, ZG->aps.retrans.hash[i].buf, ZG->aps.retrans.hash[i].aps_counter, status));
  /* If status is RET_EXIT (ndle_confirm after APS ack) and block_ack is not 0xFF - do not do
     anything, wait for next ack.
     Rationale: We may receive ack "block not received", fail to wait state, and then send this
     fragment. In that case pending ack is already outdated - simply wait for the next ack. Should
     receive ack with this block "received" later.
  */
  if (status == RET_EXIT &&
      ZG->aps.out_frag.wait_block_ack != 0xFFU)
  {
    ZG->aps.retrans.hash[i].state = ZB_APS_RETRANS_ENT_FRAG_SENT_MAC_CONFIRMED_AFTER_APS_ACKED;
    return RET_BLOCKED;
  }

  if (ZG->aps.out_frag.total_blocks_num != 0U)
  {
    if (ZB_APS_FC_GET_EXT_HDR_PRESENT(fc) != 0U &&
        ZB_APS_FC_GET_DELIVERY_MODE(fc) == ZB_APS_DELIVERY_UNICAST)
    {
      block_num = ZG->aps.out_frag.current_window * ZG->aps.aib.aps_max_window_size;

      if (status == RET_OK)
      {
        pkt_block_ack = aps_hdr->block_ack;
        pkt_block_num = aps_hdr->block_num;
      }
      else if (status == RET_EXIT)
      {
        pkt_block_ack = ZG->aps.out_frag.wait_block_ack;
        pkt_block_num = ZG->aps.out_frag.wait_block_num;
      }
      else if (status == RET_TIMEOUT)
      {
        pkt_block_num = ZG->aps.out_frag.current_window * ZG->aps.aib.aps_max_window_size;
      }
      else if (status == ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_NO_SHORT_ADDRESS))
      {
        /* We need to terminate the transmission */
        ret = RET_CANCELLED;
      }
      else
      {
        /* MISRA rule 15.7 requires empty 'else' branch. */
      }

      TRACE_MSG(TRACE_APS2, "aps ack: block_num %d block_ack 0x%x",
                (FMT__D_D, pkt_block_num, pkt_block_ack));

      ZB_BZERO(&(ZG->aps.out_frag.blocks_retry_mask), sizeof(ZG->aps.out_frag.blocks_retry_mask));

      /* - Remove all entries from aps hash connected with frag tx - except this */
      for (; block_num < ZG->aps.out_frag.total_blocks_num &&
             block_num < ((zb_uint_t)ZG->aps.out_frag.current_window + 1U) * ZG->aps.aib.aps_max_window_size; ++block_num)
      {
        /* Block num should fit into block ack mask*/
        if (block_num < pkt_block_num
            || block_num >= (pkt_block_num + ZG->aps.aib.aps_max_window_size))
        {
          continue;
        }

        /* If (somehow) we got acked fragments that are not marked as sent */
        if (ZB_CHECK_BIT_IN_BIT_VECTOR(&pkt_block_ack, block_num - pkt_block_num))
        {
          ZB_SET_BIT_IN_BIT_VECTOR(ZG->aps.out_frag.blocks_sent_mask, block_num);
        }
        else if (ZG->aps.out_frag.block_ref[block_num] != 0U)
        {
          /* Non-empty block_ref is an indicator that we've already attempted to send the block
             and that it has not been confirmed yet.
             This check is here to handle situations when we receive an APS ACK that contradicts
             previously received ACKs.
          */
          ZB_CLR_BIT_IN_BIT_VECTOR(ZG->aps.out_frag.blocks_sent_mask, block_num);
        }
        else
        {
          /* Otherwise we have nothing to do */
          continue;
        }

        for ( j = 0 ; j < ZB_N_APS_RETRANS_ENTRIES ; ++j)
        {
          if (ZG->aps.out_frag.block_ref[block_num] == ZG->aps.retrans.hash[j].buf)
          {
            ZB_ASSERT(ZG->aps.out_frag.block_ref[block_num] != 0U);

            if (ZB_CHECK_BIT_IN_BIT_VECTOR(&pkt_block_ack, block_num - pkt_block_num))
            {
              TRACE_MSG(TRACE_APS2, "block %hd (buf %hd) is acked: 1", (FMT__H_H, block_num, ZG->aps.out_frag.block_ref[block_num]));
              /* - Free acked block */
              if (ZG->aps.out_frag.block_ref[block_num] != ZG->aps.retrans.hash[i].buf)
              {
                /* Avoid freeing the buffer if it is already in the MAC tx queue */
                if (zb_aps_buf_retrans_isbusy(ZG->aps.out_frag.block_ref[block_num]))
                {
                  TRACE_MSG(TRACE_APS4, "done_with_frag_tx_ack bufid %hd - busy", (FMT__H, ZG->aps.out_frag.block_ref[block_num]));
                }
                else
                {
                  if (zb_address_by_short(ZG->aps.retrans.hash[j].addr, ZB_FALSE, ZB_FALSE, &ref) == RET_OK)
                  {
                    zb_address_unlock(ref);
                  }
                  zb_buf_free(ZG->aps.out_frag.block_ref[block_num]);
                }
              }
              else
              {
                free_buf = ZB_TRUE;
              }

              /* We can only be here if the block was not acked before */
              reset_retry_count = ZB_TRUE;

              /* Remove from ref */
              ZG->aps.out_frag.block_ref[block_num] = 0;

              /* Remove all entries from hash - old transaction is finished. Skip only current hash
               * entry */
              if (j != i)
              {
                aps_retrans_ent_free(j);
              }
            }
            else
            {
              zb_aps_hdr_t aps_header;

              ZB_SET_BIT_IN_BIT_VECTOR(ZG->aps.out_frag.blocks_retry_mask, block_num);

              TRACE_MSG(TRACE_APS2, "block %hd is acked: 0", (FMT__H, block_num));
              TRACE_MSG(TRACE_APS2, "refill nldereq for block %hd", (FMT__H, block_num));
              /* TODO: Cut NWK and MAC headers if exists! */
              DUMP_TRAF("payload", zb_buf_begin(ZG->aps.retrans.hash[j].buf),
                        zb_buf_len(ZG->aps.retrans.hash[j].buf), 0);
              zb_aps_hdr_parse(ZG->aps.retrans.hash[j].buf, &aps_header, ZB_FALSE);
              aps_header.clusterid = ZG->aps.retrans.hash[j].clusterid;
              aps_header.src_endpoint = ZG->aps.retrans.hash[j].src_endpoint;
              aps_header.dst_endpoint = ZG->aps.retrans.hash[j].dst_endpoint;
              aps_header.aps_counter  = ZG->aps.retrans.hash[j].aps_counter;

              zb_aps_hdr_mod(ZG->aps.retrans.hash[j].buf, &aps_header);

              fill_nldereq(ZG->aps.retrans.hash[j].buf,
                           ZG->aps.retrans.hash[j].addr,
                           /* use NWK security only if not use APS security */
                           ZB_B2U(ZG->aps.retrans.hash[j].nwk_insecure == 0U));
              ZG->aps.retrans.hash[j].state = ZB_APS_RETRANS_ENT_SENT_MAC_NOT_CONFIRMED_ALRM_RUNNING;
            }


            /* Can not have more than one retrans entry for the block */
            break;
          }
        }

        /* We didn't find an entry in the APS retransmissions table - this is a sign that
           apsde-data.request failed for that buffer.
           In such case we can set that block to zero - the buffer might have already been freed
           inside the apde-data.confirm handler.
         */
        if (j == ZB_N_APS_RETRANS_ENTRIES)
        {
          ZG->aps.out_frag.block_ref[block_num] = 0;
        }
      }

      /* - If all blocks in the current window are acked, switch to the next window and continue */
      /* tx */
      if (pkt_block_ack == 0xFFU)
      {
        ZG->aps.out_frag.current_window++;
        TRACE_MSG(TRACE_APS2, "new current_window %hd ack_mask 0x%x sent mask 0x%x retry_cnt %hd",
                (FMT__H_H_H_H, ZG->aps.out_frag.current_window, pkt_block_ack,
                 ZG->aps.out_frag.blocks_sent_mask, ZG->aps.out_frag.retry_count));
      }

      if (reset_retry_count)
      {
        ZG->aps.out_frag.retry_count = 0;
      }
      else
      {
        if (ZB_CHECK_BIT_IN_BIT_VECTOR(ZG->aps.out_frag.blocks_retry_mask, block_num) != 0U)
        {
          ZG->aps.out_frag.retry_count++;
        }
      }

      if (ZG->aps.out_frag.retry_count < ZB_N_APS_MAX_FRAME_RETRIES)
      {
        if (ret == RET_OK && !all_fragmented_blocks_acked())
        {
          /* kick the transmission procedure */
          zb_aps_out_frag_schedule_send_next_block_delayed();

          if (free_buf)
          {
            TRACE_MSG(TRACE_APS2, "free retrans ent %hd (buf %hd)", (FMT__H_H, i, ZG->aps.retrans.hash[i].buf));
            /* Current frag is acked - may free it */
            /* Avoid freeing the buffer if it is already in the MAC tx queue */
            if (zb_aps_buf_retrans_isbusy(ZG->aps.retrans.hash[i].buf))
            {
              TRACE_MSG(TRACE_APS4, "bufid %hd - busy", (FMT__H, ZG->aps.retrans.hash[i].buf));
            }
            else
            {
              if (zb_address_by_short(ZG->aps.retrans.hash[i].addr, ZB_FALSE, ZB_FALSE, &ref) == RET_OK)
              {
                zb_address_unlock(ref);
              }
              zb_buf_free(ZG->aps.retrans.hash[i].buf);
            }
            aps_retrans_ent_free((zb_uint8_t)i);
          }
          TRACE_MSG(TRACE_APS4, "<< done_with_frag_tx_ack ret %hd", (FMT__H, RET_BLOCKED));
          return RET_BLOCKED;
        }
      }
      else
      {
        TRACE_MSG(TRACE_APS2, "out of retries!", (FMT__0));
        ret = RET_TIMEOUT;        /* Out of retries  */
      }
      /* Else TX is finished. Return RET_OK with this buf */

      if (ret != RET_OK)
      {
        /* If we are here, something goes wrong. Reset transmission and send fail confirm */
        for (j = 0; j < ZG->aps.out_frag.total_blocks_num; ++j)
        {
          if (ZG->aps.retrans.hash[i].buf == ZG->aps.out_frag.block_ref[j])
          {
            break;
          }
        }

        if (j < ZG->aps.out_frag.total_blocks_num)
        {
          TRACE_MSG(TRACE_APS2, "fail confirm on block %hd", (FMT__H, j));
          /* It is confirm for one of the fragments.
             Status is not ok - abort all pending fragments and pass confirm with error */
          for (j = 0; j < ZG->aps.out_frag.total_blocks_num; ++j)
          {
            if (ZG->aps.out_frag.block_ref[j] != 0U)
            {
              if (ZG->aps.out_frag.block_ref[j] != ZG->aps.retrans.hash[i].buf)
              {
                TRACE_MSG(TRACE_APS2, "drop pending fragment, param %hd, status %hd",
                          (FMT__H_H, ZG->aps.out_frag.block_ref[j], status));

                ZB_SCHEDULE_ALARM_CANCEL(zb_aps_ack_timer_cb, j);
                zb_buf_free(ZG->aps.out_frag.block_ref[j]);
              }
              ZG->aps.out_frag.block_ref[j] = 0;
            }
          }
        }
      }

      /* If we are here, it is time to send confirm, so reset fragment tx. */
      {
        zb_apsde_data_confirm_t *apsde_data_conf;

        TRACE_MSG(TRACE_APS2, "reset frag tx!", (FMT__0));

        ZB_ASSERT(ZG->aps.out_frag.data_param);
        /*cstat !MISRAC2012-Rule-20.7 See ZB_BUF_GET_PARAM() for more information. */
        apsde_data_conf = ZB_BUF_GET_PARAM(ZG->aps.out_frag.data_param, zb_apsde_data_confirm_t);

        ZB_BZERO(apsde_data_conf, sizeof(zb_apsde_data_confirm_t));

        apsde_data_conf->addr_mode = ZG->aps.out_frag.addr_mode;
        if (apsde_data_conf->addr_mode == ZB_APS_ADDR_MODE_64_ENDP_PRESENT)
        {
          zb_ret_t addr_ret;
          addr_ret = zb_address_ieee_by_short(ZG->aps.retrans.hash[i].addr,
                                              apsde_data_conf->dst_addr.addr_long);
          ZB_ASSERT(addr_ret == RET_OK); /* It must be present since we lock it before the
                                            transmission of each fragment. */
        }
        else
        {
          apsde_data_conf->dst_addr.addr_short = ZG->aps.retrans.hash[i].addr;
        }

        apsde_data_conf->src_endpoint = ZG->aps.retrans.hash[i].src_endpoint;
        apsde_data_conf->dst_endpoint = ZG->aps.retrans.hash[i].dst_endpoint;
        apsde_data_conf->status = status;

        ZB_SCHEDULE_CALLBACK(zb_apsde_data_confirm, ZG->aps.out_frag.data_param);

        /* Clearing out_frag entry */
        ZB_BZERO(&ZG->aps.out_frag, sizeof(ZG->aps.out_frag));
        ZG->aps.out_frag.state = APS_FRAG_INACTIVE;

#ifdef SNCP_MODE
        /* disable auto turbo poll for fragmented APS tx */
        sncp_auto_turbo_poll_aps_tx(ZB_FALSE);
#endif

#ifdef ZB_APS_FRAG_FORCE_DISABLE_TURBO_POLL
        TRACE_MSG(TRACE_APS4, "stop turbo poll_packets because whole frag TX transaction ACKed", (FMT__0));
        zb_zdo_turbo_poll_packets_leave(0);
#endif
        aps_out_frag_schedule_queued_requests();
      }
    }
  }
  TRACE_MSG(TRACE_APS4, "<< done_with_frag_tx_ack ret %hd", (FMT__H, ret));
  return ret;
}
#endif

/*
 * if APS frame type == command || APS frame type == APS ack
 *   if status != RET_BLOCKED - free buf from retrans context, clear retrans context
 * if APS frame type == data
 *   if status == RET_BLOCKED - no confirm, clear retrans context
 *   if status == RET_PENDING - no confirm, unlock address, clear retrans context
 *   if status != RET_BLOCKED && status != RET_PENDING - confirm, clear retrans context
 */
static void done_with_this_ack(zb_ushort_t i, const zb_aps_hdr_t *aps_hdr, zb_ret_t status)
{
  zb_uint8_t fc = aps_hdr->fc;

  zb_buf_set_status(ZG->aps.retrans.hash[i].buf, status);

/* If status is OK, find corresponding entry in routing_table and reset its expiry field */
#if defined ZB_NWK_MESH_ROUTING && defined ZB_ROUTER_ROLE
  if (status == RET_OK ||
      status == RET_EXIT)
  {
    zb_nwk_reset_route_expire(ZG->aps.retrans.hash[i].addr);
  }
#endif

  /* We can pass here fc from either our packet or incoming ACK. Check is our
   * original frame command or data. */
  if (
    /* command itself */
    ZB_APS_FC_GET_FRAME_TYPE(fc) == ZB_APS_FRAME_COMMAND
    /* ACK to a command */
      || (ZB_APS_FC_GET_FRAME_TYPE(fc) == ZB_APS_FRAME_ACK
          && ZB_APS_FC_GET_ACK_FORMAT(fc) != 0U))
  {
    TRACE_MSG(TRACE_APS2, "finally done with this commmand i %hd status %hd", (FMT__H_H, i, status));
#if defined ZB_COORDINATOR_ROLE
    if (ZG->zdo.handle.key_sw == ZG->aps.retrans.hash[i].buf)
    {
      TRACE_MSG(TRACE_SECUR3, "switch nwk key after this frame sent", (FMT__0));
      ZG->zdo.handle.key_sw = 0;
      secur_nwk_key_switch(ZB_NIB().active_key_seq_number + 1);
    }
#endif
    if (status != RET_BLOCKED)
    {
      zb_buf_free(ZG->aps.retrans.hash[i].buf);
    }
  }
  else
  {
    TRACE_MSG(TRACE_APS2, "finally done with this data pkt i %hd status %hd", (FMT__H_H, i, status));
    if (status == RET_PENDING)
    {
      zb_address_ieee_ref_t addr_ref;

      /* WARNING:
       * RET_PENDING status here only for binding transmissions. In this case we are transmitting
       * this packet to the other devices (binding transmission entries), so the
       * zb_apsde_data_confirm() will not be called and destination address will not be unlocked
       * (see. zb_apsde_data_confirm_release_dst_addr_lock()).
       * Unlock address here (it locked in zb_apsde_data_request_main()) and send packet to the
       * next binding transmission entry (aps_send_frame_via_binding()).
       */
      if (zb_address_by_short(ZG->aps.retrans.hash[i].addr, ZB_FALSE, ZB_FALSE, &addr_ref) == RET_OK)
      {
        zb_address_unlock(addr_ref);
      }
    }
    else if (status != RET_BLOCKED)
    {
#ifdef APS_FRAGMENTATION
      /* If it was fragmented.. */
      if (ZB_APS_FC_GET_EXT_HDR_PRESENT(aps_hdr->fc) != 0U &&
          ZB_APS_FC_GET_DELIVERY_MODE(aps_hdr->fc) == ZB_APS_DELIVERY_UNICAST)
      {
        zb_address_ieee_ref_t ref;

        if (done_with_frag_tx_ack(i, aps_hdr, status) == RET_BLOCKED)
        {
          /* buffer and hash entry are blocked by frag tx, do nothing */
          return;
        }

        /* we can be here if our fragmented transmission failed and we still have some blocks
           in the retransmission table */

        /* it was a single block of fragmented transmission, we can free it now, upper layers
           do not care about this packet */
        /* Avoid freeing the buffer if it is already in the MAC tx queue */
        if (zb_aps_buf_retrans_isbusy(ZG->aps.retrans.hash[i].buf))
        {
          TRACE_MSG(TRACE_APS4, "bufid %hd - busy", (FMT__H, ZG->aps.retrans.hash[i].buf));
        }
        else
        {
          if (zb_address_by_short(ZG->aps.retrans.hash[i].addr, ZB_FALSE, ZB_FALSE, &ref) == RET_OK)
          {
            zb_address_unlock(ref);
          }
          zb_buf_free(ZG->aps.retrans.hash[i].buf);
        }
      }
      else /* Not a fragmented transmission */
#endif
      {
        zb_apsde_data_confirm_t *apsde_data_conf;
        zb_aps_hdr_t aps_header;

        zb_aps_hdr_parse(ZG->aps.retrans.hash[i].buf, &aps_header, ZB_FALSE);
        /*cstat !MISRAC2012-Rule-20.7 See ZB_BUF_GET_PARAM() for more information. */
        apsde_data_conf = ZB_BUF_GET_PARAM(ZG->aps.retrans.hash[i].buf, zb_apsde_data_confirm_t);

        apsde_data_conf->src_endpoint = ZG->aps.retrans.hash[i].src_endpoint;
        apsde_data_conf->dst_endpoint = ZG->aps.retrans.hash[i].dst_endpoint;

        /* Try to get ieee by short (before unlock)! */
        if (zb_address_ieee_by_short(ZG->aps.retrans.hash[i].addr,
                                     apsde_data_conf->dst_addr.addr_long) == RET_OK)
        {
          /* FIXME: must pass up same address mode as it was in data request! */
          apsde_data_conf->addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
        }
        else
        {
          apsde_data_conf->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
          apsde_data_conf->dst_addr.addr_short = ZG->aps.retrans.hash[i].addr;
        }

        apsde_data_conf->status = (status == RET_EXIT) ? RET_OK : status;

        /* Check for group addressing */
        upd_conf_param(ZG->aps.retrans.hash[i].buf, aps_header.fc, &aps_header);

        //ZB_SCHEDULE_CALLBACK(zb_apsde_data_confirm, ZG->aps.retrans.hash[i].buf);
        zb_schedule_callback(zb_apsde_data_confirm, ZG->aps.retrans.hash[i].buf);
      }
    }
  }

  aps_retrans_ent_free((zb_uint8_t)i);
}

/**
  Check - if buffer is busy in retrans queue
 */
static zb_bool_t zb_aps_buf_retrans_isbusy(zb_bufid_t bufi)
{
  zb_bool_t is_busy = ZB_FALSE;
  zb_uint8_t i;

  TRACE_MSG(TRACE_APS4, ">> zb_aps_buf_retrans_isbusy bufid %hd", (FMT__H, bufi));
  for (i = 0; i < ZB_N_APS_RETRANS_ENTRIES; ++i)
  {
    TRACE_MSG(TRACE_APS4, "i %hu buf %hu state %hu", (FMT__H_H_H, i, ZG->aps.retrans.hash[i].buf, ZG->aps.retrans.hash[i].state));
    if (ZG->aps.retrans.hash[i].buf == bufi)
    {
      if (aps_retrans_ent_busy_state(i))
      {
        TRACE_MSG(TRACE_APS2, "retrans[%hd] bufid %hd state %hd - busy", (FMT__H_H_H, i, bufi, ZG->aps.retrans.hash[i].state));
        is_busy = ZB_TRUE;
        break;
      }
    }
  }

  TRACE_MSG(TRACE_APS4, "<< zb_aps_buf_retrans_isbusy ret %hu", (FMT__H, is_busy));
  return is_busy;
}

/**
  Special timer callback for handling retransmissions, checks transmission queue,
  confirm packets, continues binded sending processing, restarts itself with alarm timer or stops processing
 */
void zb_aps_ack_timer_cb(zb_uint8_t idx)
{
  zb_bool_t is_busy;
  /* zb_uindex_t i; */
  /* Note: 'param' here is index, not packet buffer!  */
  zb_aps_retrans_ent_t *ent = &ZG->aps.retrans.hash[idx];
  zb_uint8_t bufi = ent->buf;
  zb_bool_t is_binding_trans = (zb_bool_t) (zb_check_bind_trans(bufi) != RET_OK);
  zb_uint8_t rx_on_when_idle =
    zb_nwk_get_nbr_rx_on_idle_short_or_false(ent->addr);
  zb_aps_hdr_t aps_hdr;

  TRACE_MSG(TRACE_APS2, "+zb_aps_ack_timer_cb idx %hd buf %hd state %hd is_binding_trans %hd", (FMT__H_H_H_H, idx, bufi, ent->state, is_binding_trans));

  zb_aps_hdr_parse(bufi, &aps_hdr, ZB_FALSE);

#if 0
  /* In case of bind we can have buf more than once in the hash - check for it */
  for ( i = 0 ; i < ZB_N_APS_RETRANS_ENTRIES ; ++i)
  {
    if ((ZG->aps.retrans.hash[i].buf == bufi) &&
        ((ZG->aps.retrans.hash[i].state == ZB_APS_RETRANS_ENT_SENT_MAC_NOT_CONFIRMED_ALRM_RUNNING)||
         (ZG->aps.retrans.hash[i].state ==  ZB_APS_RETRANS_ENT_SENT_MAC_NOT_CONFIRMED_APS_ACKED_ALRM_RUNNING)))
    {
      TRACE_MSG(TRACE_APS2, "idx %hd %hd state %hd - busy", (FMT__H_H_H, idx, bufi, ent->state));
      break;
    }
  }
#endif

  is_busy = zb_aps_buf_retrans_isbusy(bufi);

  if ((!is_binding_trans) && (!is_busy))
  {
    TRACE_MSG(TRACE_APS1, "APS FC = %hd, aps_sec_in_fc = %hd, nwk_insecure = %hd",
        (FMT__H_H_H, aps_hdr.fc, ZB_APS_FC_GET_SECURITY(aps_hdr.fc), ent->nwk_insecure));

    if ((ent->aps_retries > 0U) && (ent->state != ZB_APS_RETRANS_ENT_SENT_MAC_NOT_CONFIRMED_ALRM_RUNNING))
    {
      ent->aps_retries--;
    }

    if ((ent->aps_retries == 0U)
        && (ent->state == ZB_APS_RETRANS_ENT_SENT_MAC_CONFIRMED_ALRM_RUNNING
           || ent->state == ZB_APS_RETRANS_ENT_KILL_AT_MAC_CONFIRM))
    {
/* GP add, try to fixed, stop retry with offline device */
#if 0
      TRACE_MSG(TRACE_APS2, "out of retransmissions - APS transmission failed", (FMT__0));
      //done_with_this_ack(idx, &aps_hdr, ERROR_CODE(ERROR_CATEGORY_APS,ZB_APS_STATUS_NO_ACK));
      done_with_this_ack(idx, &aps_hdr, ERROR_CODE(ERROR_CATEGORY_APS,/*ZB_APS_STATUS_NO_SHORT_ADDRESS*/ZB_APS_STATUS_NO_ACK));   /* GP add */
      ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_APS_TX_UCAST_FAIL_ID);
#else

      zb_ieee_addr_t ieee_addr_by_short_ref;

      /* Found ieee address stored in addr map. Searching for its short address */
      zb_address_ieee_by_short(ent->addr, ieee_addr_by_short_ref);

      /* If ieee address is unknown (filled by 0xff) */
      if (ZB_IS_64BIT_ADDR_UNKNOWN(ieee_addr_by_short_ref))
      {
        TRACE_MSG(TRACE_APS2, "out of retransmissions - APS transmission failed, short address unreachable", (FMT__0));
        done_with_this_ack(idx, &aps_hdr, ERROR_CODE(ERROR_CATEGORY_APS,ZB_APS_STATUS_NO_SHORT_ADDRESS));   /* GP add */
        ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_APS_TX_UCAST_FAIL_ID);
      }
      else
      {
        TRACE_MSG(TRACE_APS2, "out of retransmissions - APS transmission failed", (FMT__0));
        done_with_this_ack(idx, &aps_hdr, ERROR_CODE(ERROR_CATEGORY_APS,/*ZB_APS_STATUS_NO_SHORT_ADDRESS*/ZB_APS_STATUS_NO_ACK));   /* GP add */
        ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_APS_TX_UCAST_FAIL_ID);
      }

#endif
    }
    else
    {
      /* If we stuck in some intermediate state, let's schedule alarm again
       * even if counter is already 0 */
      TRACE_MSG(TRACE_APS2, "schedule zb_aps_ack_timer_cb time %u", (FMT__D, ZB_N_APS_ACK_WAIT_DURATION(ZB_U2B(rx_on_when_idle))));
      //ZB_SCHEDULE_ALARM(zb_aps_ack_timer_cb, idx, ZB_N_APS_ACK_WAIT_DURATION(ZB_U2B(rx_on_when_idle)));
      zb_schedule_alarm(zb_aps_ack_timer_cb, idx, ZB_N_APS_ACK_WAIT_DURATION(ZB_U2B(rx_on_when_idle)));

      if (ent->state == ZB_APS_RETRANS_ENT_SENT_MAC_CONFIRMED_ALRM_RUNNING)
      {
#ifdef ZB_ROUTER_ROLE
        /* Last aps retransmission.
           In case of assimetrical link it can be useful to try TX via routing.
           After additinal checks (device is in nbt etc) set send_via_routing flag if nbt.
        */
        if (ent->aps_retries == 1U)
        {
          nwk_maybe_force_send_via_routing(ent->addr);
        }
#endif  /* ZB_ROUTER_ROLE */

        aps_hdr.clusterid = ent->clusterid;
        aps_hdr.src_endpoint = ent->src_endpoint;
        aps_hdr.dst_endpoint = ent->dst_endpoint;
        aps_hdr.aps_counter  = ent->aps_counter;

        zb_aps_hdr_mod(bufi, &aps_hdr);


        fill_nldereq(bufi,
                     ent->addr,
                     /* use NWK security only if not use APS security */
                     ent->nwk_insecure == 0U ? 1U : 0U);

        TRACE_MSG(TRACE_APS2, "ACK FAILED - retransmit idx=%hd, buf [%hd] st %hd, retries %hd, insecure %hd",
                  (FMT__H_H_H_H_H,
                   idx, bufi, ent->state,
                   ent->aps_retries,
                   ent->nwk_insecure));
        ent->state = ZB_APS_RETRANS_ENT_SENT_MAC_NOT_CONFIRMED_ALRM_RUNNING;

#ifdef ZB_ZCL_ENABLE_WWAH_SERVER
        zb_zcl_wwah_bad_parent_recovery_signal(ZB_ZCL_WWAH_BAD_PARENT_RECOVERY_APS_ACK_FAILED);
#endif

        DUMP_TRAF("retransmitting aps", zb_buf_begin(ent->buf),
                  zb_buf_len(bufi), 0);

#ifdef APS_FRAGMENTATION
        /* If it is last block in the window from fragmented tx, remove all existing blocks from
         * hash, increase retry_count and restart. */
        {
          zb_uint8_t k;

          TRACE_MSG(TRACE_APS4, "ZG->aps.out_frag.blocks_sent_mask:", (FMT__0));
          for (k = 0U ; k < ZB_APS_BLOCK_MASK_SIZE; k += 2U)
          {
            /* ACHTUNG! If ZB_APS_MAX_FRAGMENT_NUM_IN_WINDOW is odd, easy out of bounds is possible  */
            TRACE_MSG(TRACE_APS4, "  0x%hx 0x%hx", (FMT__H_H, ZG->aps.out_frag.blocks_sent_mask[k], ZG->aps.out_frag.blocks_sent_mask[k+1]));
          }

          for (k = 0U ; k < ZB_ARRAY_SIZE(ZG->aps.out_frag.block_ref); ++k)
          {
            if (ZG->aps.out_frag.block_ref[k] == bufi &&
                ZB_CHECK_BIT_IN_BIT_VECTOR(ZG->aps.out_frag.blocks_sent_mask, k) == 1U)
            {
              zb_uint8_t last_block_in_window = ZG->aps.out_frag.current_window * ZG->aps.aib.aps_max_window_size;

              TRACE_MSG(TRACE_APS3, "current_window %hu last_block_in_window %hu",
                        (FMT__H_H, ZG->aps.out_frag.current_window, last_block_in_window));
              TRACE_MSG(TRACE_APS3, "ZB_CHECK_BIT_IN_BIT_VECTOR(ZG->aps.out_frag.blocks_sent_mask, %hu) = %hu",
                        (FMT__H_H, k+1U, ZB_CHECK_BIT_IN_BIT_VECTOR(ZG->aps.out_frag.blocks_sent_mask, k+1U)));

              if (
                /* Last possible block in window */
                k == last_block_in_window
                /* No blocks with number bigger than k in blocks_sent_mask - last block we sent in
                 * this window */
                || (/* Strict condition for prevention array borders violation */
                    k < ZB_APS_BLOCK_REF_SIZE - 1U &&
                    k < last_block_in_window && ZB_CHECK_BIT_IN_BIT_VECTOR(ZG->aps.out_frag.blocks_sent_mask, k + 1U) == 0U)
#ifdef ZB_CERTIFICATION_HACKS
                || ZB_CERT_HACKS().frag_skip_0_and_2nd_block_1st_time
#endif
                )
              {
                TRACE_MSG(TRACE_APS2, "-zb_aps_ack_timer_cb: last sent frag tx, restart tx for current_window", (FMT__0));
                /*TODO: should returned value handling be here? [ZOI-575] */
                (void)done_with_frag_tx_ack(idx, &aps_hdr, RET_TIMEOUT);
#ifdef ZB_CERTIFICATION_HACKS
                ZB_CERT_HACKS().frag_skip_0_and_2nd_block_1st_time = 0;
#endif
              }

              /* WARNING: If it was not the last block, silently return - wait until last block will
               * time out! */
              TRACE_MSG(TRACE_APS2, "-zb_aps_ack_timer_cb: not the last sent frag tx, silently ret", (FMT__0));
              return;
            }
          }
        }
#endif  /* #ifdef APS_FRAGMENTATION */
        /* NK: If we are in turbo poll already, then continue, else start 1 pkt turbo polling. */
        /* DL: zb_zdo_pim_continue_polling_for_pkt() may not start turbo poll
         *   Default long poll interval is 5 sec, APS retry, eg, may be 3 sec.
         *   If data comes to the next poll, not expected APS ACK, then we can never get APS ACK */
        /* zb_zdo_pim_continue_polling_for_pkt(); */
        zb_zdo_pim_continue_turbo_poll();
        ZB_SCHEDULE_CALLBACK(zb_nlde_data_request, bufi);
        if (ZB_NWK_IS_ADDRESS_BROADCAST(ent->addr) == ZB_FALSE)
        {
          ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_APS_TX_UCAST_RETRY_ID);
        }
      }
      else
      {
        TRACE_MSG(TRACE_APS2, "slot %hd state %hd ref %hd- skip this retransmit",
                  (FMT__H_H_D, idx, ent->state, bufi));
      }
    }
  }
  else if (is_busy)
  {
    TRACE_MSG(TRACE_APS2, "is_busy buf %hu trans state %hd - schedule zb_aps_ack_timer_cb time %u",
              (FMT__H_H_D, bufi, ent->state, ZB_N_APS_ACK_WAIT_DURATION(rx_on_when_idle) / 2U));
    //ZB_SCHEDULE_ALARM(zb_aps_ack_timer_cb, idx, ZB_N_APS_ACK_WAIT_DURATION(ZB_U2B(rx_on_when_idle)) / 2U);
    zb_schedule_alarm(zb_aps_ack_timer_cb, idx, ZB_N_APS_ACK_WAIT_DURATION(ZB_U2B(rx_on_when_idle)) / 2U);
  }
  else if (is_binding_trans)
  {
    TRACE_MSG(TRACE_APS2, "is_busy %hd idx %hd bufi %hd is busy for binding trans state %hd",
              (FMT__H_H_H_H, (zb_uint8_t)zb_aps_buf_retrans_isbusy(ent->buf), idx, bufi,
               ent->state));

    if ((ent->aps_retries == 0U) && (ent->state == ZB_APS_RETRANS_ENT_SENT_MAC_CONFIRMED_ALRM_RUNNING))
    {
      done_with_this_ack(idx, &aps_hdr, RET_PENDING);
      aps_send_frame_via_binding(bufi);
      ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_APS_TX_UCAST_RETRY_ID);
    }
    else
    {
      /* decrement retriers for binding transmissions */
      if ((ent->aps_retries > 0U) && (ent->state != ZB_APS_RETRANS_ENT_SENT_MAC_NOT_CONFIRMED_ALRM_RUNNING))
      {
        ent->aps_retries--;
      }

      /* Try it once more when binding transmission is probably finished */
      TRACE_MSG(TRACE_APS2, "schedule zb_aps_ack_timer_cb time %u *", (FMT__D, ZB_N_APS_ACK_WAIT_DURATION(rx_on_when_idle)/2));
      ZB_SCHEDULE_ALARM(zb_aps_ack_timer_cb, idx, ZB_N_APS_ACK_WAIT_DURATION(ZB_U2B(rx_on_when_idle))/2U);
    }
  }
  else
  {
    TRACE_MSG(TRACE_APS2, "is_binding_trans && is_busy buf %hu trans state %hd - do nothing", (FMT__H_H, bufi, ent->state));
  }

  TRACE_MSG(TRACE_APS2, "-zb_aps_ack_timer_cb", (FMT__0));
}


/**
   Clear retransmission table entries for this address.

   To be used after other device left the network.

   @param address - destination address to be cleared or 0xffff to clear all
 */
void zb_aps_clear_after_leave(zb_uint16_t address)
{
  zb_ushort_t i;

  TRACE_MSG(TRACE_APS1, "+zb_aps_clear_after_leave 0x%x", (FMT__D, address));

  if (address != ZB_NWK_BROADCAST_ALL_DEVICES)
  {
    aps_clear_dups(address);
  }
  else
  {
    aps_clear_all_dups();
  }

  for ( i = 0 ; i < ZB_N_APS_RETRANS_ENTRIES ; ++i)
  {
    if (ZG->aps.retrans.hash[i].addr == address
        || address == ZB_NWK_BROADCAST_ALL_DEVICES)
    {
      if (ZG->aps.retrans.hash[i].state == ZB_APS_RETRANS_ENT_SENT_MAC_CONFIRMED_ALRM_RUNNING)
      {
        zb_aps_hdr_t aps_hdr;

        zb_aps_hdr_parse(ZG->aps.retrans.hash[i].buf, &aps_hdr, ZB_FALSE);
        /* aps_hdr.fc = 0;         /\* Not sure *\/ */

        /* Do not send next binding transmission and pass up the confirm. */
        zb_clear_bind_trans(ZG->aps.retrans.hash[i].buf, &aps_hdr);

        /* Buffer is not busy. Can pass confirm up. */
        done_with_this_ack(i, &aps_hdr, ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_NO_SHORT_ADDRESS));
      }
      else if (ZG->aps.retrans.hash[i].state == ZB_APS_RETRANS_ENT_SENT_MAC_NOT_CONFIRMED_ALRM_RUNNING)
      {
        /* This bufffer is busy somewhere inside NWK or MAC. */
        /* If they are purged, the entry will be killed on nlde-data.confirm */
      }
      else
      {
        /* MISRA rule 15.7 requires empty 'else' branch. */
      }
    }
  }

#ifdef APS_FRAGMENTATION
  /* Stopping incoming fragmented transmissions from that device */
  for (i = 0; i < ZB_APS_MAX_IN_FRAGMENT_TRANSMISSIONS ; i++)
  {
    zb_aps_in_fragment_frame_t *ent = &ZG->aps.in_frag[i];
    if (ent->src_addr == ZB_APS_IN_FRAGMENTED_FRAME_EMPTY)
    {
      continue;
    }

    if (ent->src_addr == address || address == ZB_NWK_BROADCAST_ALL_DEVICES)
    {
      aps_in_frag_stop_entry(ent, ZB_FALSE);
    }
  }
#endif  /* #ifdef APS_FRAGMENTATION */

  TRACE_MSG(TRACE_APS1, "-zb_aps_clear_after_leave", (FMT__0));
}


zb_uint8_t zb_aps_hdr_size(zb_uint8_t fc)
{
  zb_uint8_t size = 2; /* fc + aps counter */
  zb_uint8_t frame_type = ZB_APS_FC_GET_FRAME_TYPE(fc);

  /* 3110 Acknowledgement Frame APS Header Fields
     3111 If the ack format field is not set in the frame control field, the destination endpoint, cluster identifier, profile identifier
     3112 and source endpoint shall be present.*/
  if (frame_type == ZB_APS_FRAME_DATA ||
     (frame_type == ZB_APS_FRAME_ACK && ZB_APS_FC_GET_ACK_FORMAT(fc) == 0U))
  {
    size += 1U; /* Packet either has dest endpoint (1) or group address (2) */
    size += ZB_B2U(ZB_APS_FC_GET_DELIVERY_MODE(fc) == ZB_APS_DELIVERY_GROUP);
    size +=
      /* cluster id, profile id */
      2U + 2U +
      /* src endpoint */
      1U;
#ifdef APS_FRAGMENTATION
    if (ZB_APS_FC_GET_EXT_HDR_PRESENT(fc) != 0U)
    {
      size += 2U; /* extended hdr + block number */
      if (frame_type == ZB_APS_FRAME_ACK)
      {
        size += 1U; /* + block ack */
      }
    }
#endif
  }

  return size;
}

#ifdef ZB_APS_USER_PAYLOAD

zb_ret_t zb_aps_send_user_payload(
  zb_uint8_t param,
  zb_addr_u dst_addr,
  zb_uint16_t profile_id,
  zb_uint16_t cluster_id,
  zb_uint8_t dst_endpoint,
  zb_uint8_t src_endpoint,
  zb_uint8_t addr_mode,
  zb_bool_t aps_ack_is_enabled,
  zb_uint8_t *payload_ptr,
  zb_uint8_t payload_size)
{
  zb_ret_t ret = RET_OK;
  zb_bufid_t buf = param;

  TRACE_MSG(TRACE_APS1, ">> zb_aps_send_user_payload, param %hd", (FMT__H, param));

  if (buf == 0U)
  {
    ret = RET_INVALID_PARAMETER_1;
    TRACE_MSG(TRACE_ERROR, "buffer is invalid", (FMT__0));
  }

  if ((RET_OK == ret) && (payload_ptr == NULL))
  {
    ret = RET_INVALID_PARAMETER_2;
    TRACE_MSG(TRACE_ERROR, "payload ptr is invalid", (FMT__0));
  }

  if (RET_OK == ret && payload_size > ZB_APS_MAX_MAX_BROADCAST_PAYLOAD_SIZE)
  {
    ret = RET_INVALID_PARAMETER_3;
    TRACE_MSG(TRACE_ERROR, "payload size is too large, max_payload_size: 82",
              (FMT__0));
  }

  if (RET_OK == ret)
  {
    zb_uint8_t *ptr;
    zb_apsde_data_req_t *data_req = ZB_BUF_GET_PARAM(buf, zb_apsde_data_req_t);

    ZB_BZERO(data_req, sizeof(zb_apsde_data_req_t));

    ptr = zb_buf_initial_alloc(buf, payload_size);
    ZB_MEMCPY(ptr, payload_ptr, payload_size);

    if (ZB_APS_ADDR_MODE_64_ENDP_PRESENT == addr_mode)
    {
      ZB_IEEE_ADDR_COPY(data_req->dst_addr.addr_long, dst_addr.addr_long);
    }
    else
    {
      data_req->dst_addr.addr_short = dst_addr.addr_short;
    }

    data_req->profileid = profile_id;
    data_req->clusterid = cluster_id;
    data_req->src_endpoint = src_endpoint;
    data_req->dst_endpoint = dst_endpoint;
    data_req->addr_mode = addr_mode;

    if (aps_ack_is_enabled)
    {
      data_req->tx_options |= ZB_APSDE_TX_OPT_ACK_TX;
    }

    zb_buf_flags_or(param, ZB_BUF_HAS_APS_USER_PAYLOAD);

    TRACE_MSG(TRACE_APS1, "SCHEDULE zb_apsde_data_request", (FMT__0));
    ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, param);
  }

  TRACE_MSG(TRACE_APS1, "<< zb_aps_send_user_payload", (FMT__0));

  return ret;
}

zb_uint8_t *zb_aps_get_aps_payload(zb_uint8_t param, zb_uint8_t *aps_payload_size)
{
  zb_bufid_t buf = param;
  zb_uint8_t *aps_payload_ptr = NULL;
  zb_aps_hdr_t aps_header = { 0 };

  TRACE_MSG(TRACE_APS1, ">> zb_aps_get_aps_payload, param %hd", (FMT__H, param));

  if (buf == 0U)
  {
    TRACE_MSG(TRACE_ERROR, "buffer is invalid", (FMT__0));
  }
  else if (aps_payload_size == NULL)
  {
    TRACE_MSG(TRACE_ERROR, "aps_payload_size ptr is invalid", (FMT__0));
  }
  else
  {
    zb_uint8_t aps_hdr_size;

    zb_aps_hdr_parse(buf, &aps_header, ZB_FALSE /* don't cut NWK header */);
    aps_hdr_size = ZB_APS_HDR_SIZE(aps_header.fc);

    aps_payload_ptr = (zb_uint8_t *)zb_buf_begin(buf) + aps_hdr_size;
    *aps_payload_size = (zb_uint8_t)zb_buf_len(buf) - aps_hdr_size;
  }

  TRACE_MSG(TRACE_APS1, "<< zb_aps_get_aps_payload", (FMT__0));
  return aps_payload_ptr;
}

void zb_aps_set_user_data_tx_cb(zb_aps_user_payload_callback_t cb)
{
  ZG->aps.aps_user_payload_cb = cb;
}

zb_bool_t zb_aps_call_user_payload_cb(zb_uint8_t param)
{
  zb_bool_t processed = ZB_FALSE;
  zb_bufid_t buf = param;

  if (buf != 0U)
  {
    if ((zb_buf_flags_get(buf) & ZB_BUF_HAS_APS_USER_PAYLOAD) != 0U)
    {
      if (ZG->aps.aps_user_payload_cb != NULL)
      {
        ZG->aps.aps_user_payload_cb(param);
      }

      zb_buf_flags_clr(param, ZB_BUF_HAS_APS_USER_PAYLOAD);

      processed = ZB_TRUE;
    }
  }

  return processed;
}

#endif /* #ifdef ZB_APS_USER_PAYLOAD */

static void aps_retrans_ent_free(zb_uint8_t idx)
{
  ZG->aps.retrans.hash[idx].state = ZB_APS_RETRANS_ENT_FREE;
  ZG->aps.retrans.hash[idx].addr = (zb_uint16_t)-1;
  ZG->aps.retrans.hash[idx].buf = (zb_uint8_t)-1;
  ZB_SCHEDULE_ALARM_CANCEL(zb_aps_ack_timer_cb, idx);
  ZB_ASSERT(ZG->aps.retrans.n_packets != 0U);
  ZG->aps.retrans.n_packets--;
  TRACE_MSG(TRACE_APS3, "retrans.n_packets %hd", (FMT__H, ZG->aps.retrans.n_packets));
}

static zb_bool_t aps_retrans_ent_busy_state(zb_uint8_t idx)
{
  return
    ZG->aps.retrans.hash[idx].state == ZB_APS_RETRANS_ENT_SENT_MAC_NOT_CONFIRMED_ALRM_RUNNING ||
    ZG->aps.retrans.hash[idx].state == ZB_APS_RETRANS_ENT_SENT_MAC_NOT_CONFIRMED_APS_ACKED_ALRM_RUNNING;
}

#if defined ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ && defined ZB_SUSPEND_APSDE_DATAREQ_BY_SUBGHZ_CLUSTER && defined SNCP_MODE
static void aps_retrans_ent_free_all(zb_ret_t status)
{
  zb_uint8_t i;
  zb_address_ieee_ref_t ref;

  TRACE_MSG(TRACE_APS4, ">> aps_retrans_ent_free_all", (FMT__0));
  for (i = 0 ; i < ZB_N_APS_RETRANS_ENTRIES; ++i)
  {
    if (ZG->aps.retrans.hash[i].state != ZB_APS_RETRANS_ENT_FREE)
    {
      if (aps_retrans_ent_busy_state(i))
      {
        TRACE_MSG(TRACE_APS4, "bufid %hd - busy", (FMT__H, ZG->aps.retrans.hash[i].buf));
      }
      else
      {
        if (zb_address_by_short(ZG->aps.retrans.hash[i].addr, ZB_FALSE, ZB_FALSE, &ref) == RET_OK)
        {
          zb_address_unlock(ref);
        }
        TRACE_MSG(TRACE_APS4, "aps_send_fail_confirm for bufid %hd", (FMT__H, ZG->aps.retrans.hash[i].buf));
        aps_send_fail_confirm(ZG->aps.retrans.hash[i].buf, status);
      }
      aps_retrans_ent_free(i);
    }
  }
  TRACE_MSG(TRACE_APS4, "<< aps_retrans_ent_free_all", (FMT__0));
}

void zb_aps_cancel_outgoing_trans(zb_uint8_t param)
{
  zb_uindex_t i;

  ZVUNUSED(param);
  TRACE_MSG(TRACE_APS1, ">> zb_aps_cancel_out_trans", (FMT__0));
#ifdef APS_FRAGMENTATION
  /* Send failure for each item in the output queue */
  while (!ZB_RING_BUFFER_IS_EMPTY(&ZG->aps.out_frag_q))
  {
    zb_bufid_t frag_buf = *ZB_RING_BUFFER_GET(&ZG->aps.out_frag_q);
    ZB_RING_BUFFER_FLUSH_GET(&ZG->aps.out_frag_q);
    TRACE_MSG(TRACE_APS4, "aps_send_fail_confirm for bufid %hd", (FMT__H, frag_buf));
    aps_send_fail_confirm(frag_buf, RET_CANCELLED);
  }

  /* Checks for current output fragmented transaction */
  for (i = 0; i < ZG->aps.out_frag.total_blocks_num; ++i)
  {
    if (ZG->aps.out_frag.block_ref[i] != 0U)
    {
      zb_check_frag_queue_cleanup_and_send_fail_confirm(ZG->aps.out_frag.block_ref[i], RET_CANCELLED);
      break;
    }
  }
#endif

  aps_retrans_ent_free_all(RET_CANCELLED);
  TRACE_MSG(TRACE_APS1, "<< zb_aps_cancel_out_trans", (FMT__0));
}

#endif /* defined ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ && defined ZB_SUSPEND_APSDE_DATAREQ_BY_SUBGHZ_CLUSTER */

/*! @} */
