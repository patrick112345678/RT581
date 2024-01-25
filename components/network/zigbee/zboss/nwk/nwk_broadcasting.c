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
/* PURPOSE: Broadcast processing and transmission
*/

#define ZB_TRACE_FILE_ID 42
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_mac.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "nwk_internal.h"
#include "zb_secur.h"
#include "zb_watchdog.h"
#include "zb_zdo.h"
#include "zb_bdb_internal.h"
#include "zb_bufpool.h"
#include "zb_nwk_ed_aging.h"
#include "zb_manuf_prefixes.h"

#define NEIGBOR_TABLE_ITERATOR_END ((zb_uint8_t)~0U)

#ifdef ZB_ROUTER_ROLE
static void nwk_broadcasting_clear_brrt(void);
static void nwk_broadcasting_brrt_countdown_alarm(zb_uint8_t param);
static void nwk_broadcasting_start_brrt_countdown_alarm(void);
static void nwk_broadcasting_unicast_transmission(zb_uint8_t param, zb_uint16_t idx);
static void nwk_broadcasting_force_completion(zb_nwk_broadcast_retransmit_t *ent);
static void nwk_broadcasting_attempt_completion(zb_nwk_broadcast_retransmit_t *ent);

static void brrt_set_passive_ack(zb_nwk_broadcast_retransmit_t *ent, zb_uint16_t mac_src_addr);
static zb_bool_t brrt_check_passive_acks_completion(zb_nwk_broadcast_retransmit_t *ent);
static zb_bool_t brrt_check_completion(zb_nwk_broadcast_retransmit_t *ent);
static zb_uint8_t brrt_allocate_entry(void);

static zb_nwk_broadcast_retransmit_t *brrt_find_entry_by_nwk_hdr(zb_uint16_t src_addr, zb_uint8_t seq_num);
static void brrt_free_entry(zb_nwk_broadcast_retransmit_t *ent);

static zb_uint8_t nwk_get_broadcast_jitter_bi(zb_uint8_t param);
static zb_uint8_t nwk_get_broadcast_interval_bi(zb_uint8_t param);
static zb_uint8_t nwk_get_broadcast_retries_count(zb_uint8_t param);

static void zb_nwk_call_br_confirm(zb_uint8_t param);
#endif

#if !(defined ZB_ED_ROLE && defined ZB_ED_RX_OFF_WHEN_IDLE)
static void nwk_broadcasting_btr_aging_alarm(zb_uint8_t param);
static void nwk_broadcasting_clear_btt(zb_uint8_t param);
#endif


/*! \addtogroup ZB_NWK_BROADCASTING */
/*! @{ */

#if !(defined ZB_ED_ROLE && defined ZB_ED_RX_OFF_WHEN_IDLE)
zb_bool_t zb_nwk_broadcasting_add_btr(zb_nwk_hdr_t *nwk_hdr)
{
  zb_nwk_btr_t *ent;

  if (!ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()))
  {
    return ZB_FALSE;
  }

/* EE If the point is in eliminating of address lock/unlock here, I vote against it. Address lock/unlock MUST be stable. We CAN decrease RAM usage by using it.
   If the reason of direct using of short address is some trick with address conflict in the standard - please, describe it in the comment referring to the specification.

   AN: explained in the structure definition.
 */
  NWK_ARRAY_GET_ENT(ZG->nwk.handle.btt, ent, ZG->nwk.handle.btt_cnt);
  if (ent != NULL)
  {
    ent->source_addr = nwk_hdr->src_addr;
    ent->sequence_number = nwk_hdr->seq_num;
    ent->expiration_time_tx = ZB_NWK_EXPIRY_BROADCAST_TX_CYCLE;  // to avoid address conflict from relaying delay
		ent->expiration_time_rx = ZB_NWK_EXPIRY_BROADCAST_RX_CYCLE;
    TRACE_MSG(TRACE_NWK3, "btt add ent addr %x seq %hd exp t %hd",
              (FMT__D_H_H, ent->source_addr, ent->sequence_number, ent->expiration_time_rx));

    /* start expiry routine if it was not started before */
    if (ZG->nwk.handle.btt_cnt == 1U)
    {
      ZB_SCHEDULE_ALARM_CANCEL(nwk_broadcasting_btr_aging_alarm, ZB_ALARM_ANY_PARAM);
      //ZB_SCHEDULE_ALARM(nwk_broadcasting_btr_aging_alarm, 0, ZB_TIME_ONE_SECOND);
      ZB_SCHEDULE_ALARM(nwk_broadcasting_btr_aging_alarm, 0, ZB_BTR_AGING_CYCLE);
    }
    return ZB_TRUE;
  }
  else
  {
    ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_NWK_BCAST_TABLE_FULL);
  }
  return ZB_FALSE;
}

zb_nwk_btr_t *zb_nwk_broadcasting_find_btr(const zb_nwk_hdr_t *nwk_hdr)
{
  zb_nwk_btr_t *ent;

  NWK_ARRAY_FIND_ENT(ZG->nwk.handle.btt, ZB_NWK_BTR_TABLE_SIZE, ent,
                     (ent->source_addr == nwk_hdr->src_addr) && (ent->sequence_number == nwk_hdr->seq_num));

  return ent;
}

#endif

#ifdef ZB_ROUTER_ROLE

static void zb_nwk_broadcasting_prepare_mcps_data_req(zb_uint8_t param)
{
/* EE: Can we be here if frame is not incoming packet, but our outgoing broadcast? In such case there will be no zb_apsde_data_ind_params_t.
   AN: there will be. See zb_nwk_broadcasting_transmit.
 */
  zb_apsde_data_ind_params_t *ind_param = ZB_BUF_GET_PARAM(param, zb_apsde_data_ind_params_t);

  zb_mcps_build_data_request(param, ZB_PIBCACHE_NETWORK_ADDRESS(), ZB_NWK_BROADCAST_ALL_DEVICES, 0, ind_param->handle);
}

void zb_nwk_broadcasting_transmit(zb_uint8_t param)
{
/* EE: Do we have ind_param in case of outgoing transmit?
   AN: we do. All packets go through zb_nwk_forward, and zb_nwk_forward assumes that
   zb_apsde_data_ind_params_t is always present.
*/
  zb_nwk_broadcast_retransmit_t *ent;
  zb_uint8_t ent_idx;
  zb_apsde_data_ind_params_t *ind_param = ZB_BUF_GET_PARAM(param, zb_apsde_data_ind_params_t);

  TRACE_MSG(TRACE_NWK3, ">> zb_nwk_broadcasting_transmit param %hd", (FMT__H, param));

/*  PRINT_BUF_PAYLOAD_BY_REF(param); */
  zb_nwk_hdr_t *nwhdr = zb_buf_begin(param);
  ent_idx = brrt_allocate_entry();
  if (ent_idx != ZB_INVALID_BRRT_IDX)
  {
    ent = &ZG->nwk.handle.brrt[ent_idx];

    TRACE_MSG(TRACE_NWK3, "begin broadcast transmit ent %p buf %hd",
              (FMT__P_H, ent, param));
		
    ent->source_addr = nwhdr->src_addr;
		ent->seq_num = nwhdr->seq_num;
    ent->buf = param;
    ent->retries_left = nwk_get_broadcast_retries_count(param);
    ent->been_broadcasted = ZB_FALSE;
    ent->neighbor_table_iterator = (zb_uint8_t)zb_nwk_nbr_next_ze_children_i(NWK_DST_FROM_BUF(ent->buf), 0);
    TRACE_MSG(TRACE_NWK3, "neighbor_table_iterator %hd", (FMT__H, ent->neighbor_table_iterator));

    /* set up transmission delay */
    ent->retransmit_countdown = 0;
    if (ind_param->mac_src_addr != ZB_MAC_SHORT_ADDR_NOT_ALLOCATED)
    {
      ent->retransmit_countdown = nwk_get_broadcast_jitter_bi(param) / NWK_RETRANSMIT_COUNTDOWN_QUANT;
    }

    /* Set up passive acks */

    TRACE_MSG(TRACE_NWK3, "mac_src_addr %xd", (FMT__D, ind_param->mac_src_addr));

#ifdef ZB_CONFIGURABLE_MEM
    ZB_BZERO(ent->passive_ack, ZB_NWK_BRCST_PASSIVE_ACK_ARRAY_SIZE);
#else
    ZB_BZERO(ent->passive_ack, sizeof(ent->passive_ack));
#endif

    if (ind_param->mac_src_addr != ZB_MAC_SHORT_ADDR_NOT_ALLOCATED)
    {
      /* in case we are rebroadcasting this packet */
      brrt_set_passive_ack(ent, ind_param->mac_src_addr);
    }

    /* replace buf param */
    zb_nwk_broadcasting_prepare_mcps_data_req(param);

    /* We now need to: */
    /* 1. Send broadcast messages */
    nwk_broadcasting_start_brrt_countdown_alarm();

    if (ent->neighbor_table_iterator != NEIGBOR_TABLE_ITERATOR_END)
    {
      /* 2. Unicast to sleepy ED chlidren */
      ZB_SCHEDULE_CALLBACK2(nwk_broadcasting_unicast_transmission, 0, ent_idx);
    }
  }
  else
  {
    /* EE: Why can't we do MAC broadcast of that packet once even when brrt is full?
       AN: We can, but do we really need to? In this case we haven't actually completed the
       broadcast procedure and still we'll report success to the upper layers.
     */
    TRACE_MSG(TRACE_NWK3, "brrt tbl full, drop pkt", (FMT__0));
    ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_NWK_BCAST_TABLE_FULL);

    if (ind_param->handle < ZB_NWK_MINIMAL_INTERNAL_HANDLE)
    {
      /* indicate that the packet was not sent */
      /* DL: Why is the ZB_NWK_STATUS_BT_TABLE_FULL status not used? */
      nwk_call_nlde_data_confirm(param, ZB_NWK_STATUS_NO_ROUTING_CAPACITY, ZB_TRUE);
    }
    else
    {
      /* no need to indicate internal nwk packets */
      TRACE_MSG(TRACE_NWK3, "freeing the buffer", (FMT__0));
      zb_buf_free(param);
    }
  }
  TRACE_MSG(TRACE_NWK3, "<< zb_nwk_broadcasting_transmit", (FMT__0));
}

void zb_nwk_broadcasting_mark_passive_ack(zb_uint16_t mac_src_addr, zb_nwk_hdr_t *nwk_hdr)
{
  TRACE_MSG(TRACE_NWK3, "zb_nwk_broadcasting_mark_passive_ack 0x%x", (FMT__D, mac_src_addr));
  /* Passive acknoledgements only for broadcast messages 3.6.6.3 */
  if (ZB_NIB().device_type != ZB_NWK_DEVICE_TYPE_ED
#if defined ZB_PRO_STACK && !defined ZB_NO_NWK_MULTICAST
      /*
        3.6.6.3   Upon Receipt of a Member Mode Multicast Frame

        Unlike broadcasts, there is no passive acknowledgement for multicasts. Zigbee end
        devices shall not participate in the relaying of multicast frames.
       */
      && !ZB_NWK_FRAMECTL_GET_MULTICAST_FLAG(nwk_hdr->frame_control)
#endif
    )
  {
    /* mark passive ack for this neighbor */
    zb_nwk_broadcast_retransmit_t *brrt_ent;

    /* 01/15/2019 EE CR:MINOR Can mac_src_addr be not allocated here? If so, we not need to call brrt_find_entry_by_nwk_hdr in such case.
       AN: removed the check. If mac_src_address does not correspond to any neighbor, nothing will
       be changed during brrt_set_passive_ack call anyway.
     */
    brrt_ent = brrt_find_entry_by_nwk_hdr(nwk_hdr->src_addr, nwk_hdr->seq_num);
    if (brrt_ent != NULL)
    {
      brrt_set_passive_ack(brrt_ent, mac_src_addr);
    }
  }
}

#endif

void zb_nwk_broadcasting_clear(void)
{
#ifdef ZB_ROUTER_ROLE
  nwk_broadcasting_clear_brrt();
  ZB_SCHEDULE_ALARM_CANCEL(nwk_broadcasting_brrt_countdown_alarm, 0);
#endif

#if !(defined ZB_ED_ROLE && defined ZB_ED_RX_OFF_WHEN_IDLE)
  nwk_broadcasting_clear_btt(0);
  ZB_SCHEDULE_ALARM_CANCEL(nwk_broadcasting_btr_aging_alarm, 0);
#endif
}

/**** Internal functions ****/

/** BRRT management **/

#ifdef ZB_ROUTER_ROLE

static void brrt_set_passive_ack(zb_nwk_broadcast_retransmit_t *ent, zb_uint16_t mac_src_addr)
{
  zb_neighbor_tbl_ent_t *nbt;

  TRACE_MSG(TRACE_NWK3, "mark neighbor 0x%x as having relayed bcast buf %hd",
            (FMT__D_H, mac_src_addr, ent->buf));

  if (zb_nwk_neighbor_get_by_short(mac_src_addr, &nbt) == RET_OK)
  {
    ZB_SET_BIT_IN_BIT_VECTOR(ent->passive_ack,
                             ZB_NWK_NEIGHBOR_GET_INDEX_BY_ENTRY_ADDRESS(nbt));

    if (brrt_check_passive_acks_completion(ent))
    {
      ent->retries_left = 0;
    }

    nwk_broadcasting_attempt_completion(ent);
  }
}

static zb_uint8_t brrt_allocate_entry(void)
{
  if (ZG->nwk.handle.brrt_cnt < ZB_NWK_BRR_TABLE_SIZE)
  {
    zb_uint8_t i;
    zb_nwk_broadcast_retransmit_t *ent = NULL;

    for (i = 0; i < ZB_NWK_BRR_TABLE_SIZE && ent == NULL; i++)
    {
      if (ZG->nwk.handle.brrt[i].buf == 0U)
      {
        ZG->nwk.handle.brrt_cnt++;
        return i;
      }
    }
  }

  return ZB_INVALID_BRRT_IDX;
}

static zb_bool_t brrt_check_passive_acks_completion(zb_nwk_broadcast_retransmit_t *ent)
{
  zb_bool_t done = ZB_TRUE;
  zb_ushort_t i;

#if TRACE_ENABLED(TRACE_NWK3)
  zb_nwk_hdr_t *nwk_hdr = (zb_nwk_hdr_t *)zb_buf_begin(ent->buf);
#endif /* TRACE_ENABLED(TRACE_NWK3) */

  for (i = 0; i < ZB_NEIGHBOR_TABLE_SIZE ; i++)
  {
    if (ZB_U2B(ZG->nwk.neighbor.neighbor[i].used))
    {
      if ((ZG->nwk.neighbor.neighbor[i].device_type == ZB_NWK_DEVICE_TYPE_ROUTER
           || ZG->nwk.neighbor.neighbor[i].device_type == ZB_NWK_DEVICE_TYPE_COORDINATOR)
           && !ZB_U2B(ZB_CHECK_BIT_IN_BIT_VECTOR(ent->passive_ack,i)))
      {
        done = ZB_FALSE;
        break;
      }
    }
  }

#if TRACE_ENABLED(TRACE_NWK3)
  TRACE_MSG(TRACE_NWK3,
            "< brrt_check_passive_acks_completion buf %hx addr %x ret %hx",
            (FMT__H_D_H, ent->buf, nwk_hdr->dst_addr, done));
#endif /* TRACE_ENABLED(TRACE_NWK3) */

  return done;
}

static zb_bool_t brrt_check_completion(zb_nwk_broadcast_retransmit_t *ent)
{
  zb_bool_t is_completed = ZB_TRUE;

  TRACE_MSG(TRACE_NWK3, ">> brrt_check_completion buf %hd", (FMT__H, ent->buf));

  TRACE_MSG(TRACE_NWK3, "been_broadcasted %hd retries %hd neighbor_table_iterator %hd",
            (FMT__H_H_H, ent->been_broadcasted, ent->retries_left, ent->neighbor_table_iterator));

  /* EE been_broadcasted is a strange flag - a
   * kind of partial check. It set if packet successfully broadcasted
   * at least once. Can eliminate it, probably: we may need to
   * broadcast more than once anyway.
   *
   * AN: during broadcasts (if we are not the originator) it is possible that we've received the packet
   * from all our neighbors before we've attempted to rebroadcast it. Without this flag, we won't
   * broadcast the packet in such case, and our neighbors will not count this as a passive ack.
   * Yes, such situation seems not very likely, but it has already been observed multiple times.
   * If we change retries_left to retries_done, we can do without been_broadcasted, but that would
   * require calling nwk_get_broadcast_retries_count and brrt_check_passive_acks_completion from
   * too many places.
   */
  if (!ZB_U2B(ent->been_broadcasted))
  {
    /* We need at least one broadcast transmission in order to make sure that
       other devices receive a passive ack from us */
    is_completed = ZB_FALSE;
  }

  /* 01/15/2019 EE CR:MINOR Not sure: do we need to wait until all unicasts complete before returning confirm? Can't we ignore unicasts?
     And can eliminate is_buf_busy if never transmit the original buffer.

     AN: done
   */

  if (ent->neighbor_table_iterator != NEIGBOR_TABLE_ITERATOR_END)
  {
    /* We have not yet made MCPS-DATA.requests for all of our neighbors.

     * When the corresponding BTR expires, neibor_table_iterator will be set to
     * NEIGBOR_TABLE_ITERATOR_END anyway (so as not to slow down the device in case
     * when there are lots of child devices and not so many buffers).
     */
    is_completed = ZB_FALSE;
  }

  if (ent->retries_left > 0U)
  {
    /* We have not yet received all necessary passive acks (and there are still available
       transmission attempts). */
    is_completed = ZB_FALSE;
  }

  TRACE_MSG(TRACE_NWK3, "<< brrt_check_completion ret %hd", (FMT__H, is_completed));

  return is_completed;
}

static void nwk_broadcasting_force_completion(zb_nwk_broadcast_retransmit_t *ent)
{
  ZB_ASSERT(ent->buf != 0U);

  ZB_SCHEDULE_CALLBACK(zb_nwk_call_br_confirm, ent->buf);

  /* clear the brrt entry */
  brrt_free_entry(ent);
}

static void nwk_broadcasting_attempt_completion(zb_nwk_broadcast_retransmit_t *ent)
{
  if (ent->buf != 0U && brrt_check_completion(ent))
  {
    nwk_broadcasting_force_completion(ent);
  }
}

static zb_nwk_broadcast_retransmit_t *brrt_find_entry_by_nwk_hdr(zb_uint16_t src_addr, zb_uint8_t seq_num)
{
  zb_uint8_t i;
  for (i = 0; i < ZB_NWK_BRR_TABLE_SIZE; i++)
  {
    zb_nwk_broadcast_retransmit_t *ent = &ZG->nwk.handle.brrt[i];

    if (ent->buf != 0U
        /*cstat -MISRAC2012-Rule-13.5 */
        /* After some investigation, the following violation of Rule 13.5 seems to be
         * a false positive. There are no side effect to 'NWK_SRC_FROM_BUF()' and
         * 'NWK_SEQ_FROM_BUF()'. This violation seems to be caused by the existance of
         * external functions inside the previously mentioned functions which cannot be
         * analyzed by C-STAT. */
        && NWK_SRC_FROM_BUF(ent->buf) == src_addr
        && NWK_SEQ_FROM_BUF(ent->buf) == seq_num)
        /*cstat +MISRAC2012-Rule-13.5 */
    {
      return ent;
    }
  }

  return NULL;
}

static void brrt_free_entry(zb_nwk_broadcast_retransmit_t *ent)
{
  ZB_ASSERT(ent->buf != 0U);
  ZB_ASSERT(ZG->nwk.handle.brrt_cnt > 0U);

  ent->buf = 0;
  ZG->nwk.handle.brrt_cnt--;
}

static void nwk_broadcasting_clear_brrt(void)
{
  zb_uint_t i;
#ifdef ZB_CONFIGURABLE_MEM
  zb_uint8_t *passive_ack;
#endif

  for (i = 0; i < ZB_NWK_BRR_TABLE_SIZE ; i++)
  {
    if (ZG->nwk.handle.brrt[i].buf != 0U)
    {
      zb_buf_free(ZG->nwk.handle.brrt[i].buf);
    }

#ifdef ZB_CONFIGURABLE_MEM
    passive_ack = ZG->nwk.handle.brrt[i].passive_ack;

    ZB_BZERO(passive_ack, ZB_NWK_BRCST_PASSIVE_ACK_ARRAY_SIZE);
    ZB_BZERO(&ZG->nwk.handle.brrt[i], sizeof(*ZG->nwk.handle.brrt));
    ZG->nwk.handle.brrt[i].passive_ack = passive_ack;
#else
    ZB_BZERO(&ZG->nwk.handle.brrt[i], sizeof(*ZG->nwk.handle.brrt));
#endif
  }

  ZG->nwk.handle.brrt_cnt = 0;
}

#endif

/** BTT management **/

#if !(defined ZB_ED_ROLE && defined ZB_ED_RX_OFF_WHEN_IDLE)
static void nwk_broadcasting_clear_btt(zb_uint8_t param)
{
  zb_uindex_t i;

  ZVUNUSED(param);

  for (i = 0; i < ZB_NWK_BTR_TABLE_SIZE; i++)
  {
    if (ZB_U2B(ZG->nwk.handle.btt[i].used))
    {
      ZG->nwk.handle.btt[i].used = ZB_FALSE_U;
    }
  }

  ZG->nwk.handle.btt_cnt = 0;
}
#endif

/** MAC/NWK integration **/

#ifdef ZB_ROUTER_ROLE

/**
   Call confirm for nwk frame passive acked before all retransmits done.

   It is not possible to issue nlde-data.confirm directly because the broadcast
   may have been originated inside NWK layer and there are actions to be done
   after its mcps-data.confirm inside NWK.

   Add MAC hdr to the frame, then call mcps_data_confirm.
*/
static void zb_nwk_call_br_confirm(zb_uint8_t param)
{
  /* EE: Now zb_mcps_data_confirm does not suppose existence of mac header.
   * Instead, it has zb_mcps_data_confirm_params_t parameter - see its implementation.
   */
  zb_uint8_t handle = ZB_BUF_GET_PARAM(param, zb_apsde_data_ind_params_t)->handle;
  zb_mcps_data_confirm_params_t *confirm = ZB_BUF_GET_PARAM(param, zb_mcps_data_confirm_params_t);

  confirm->status = RET_OK;
  confirm->dst_pan_id = ZB_PIBCACHE_PAN_ID();
  confirm->msdu_handle = handle;
  confirm->src_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
  confirm->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
  confirm->status = MAC_SUCCESS;
#ifdef ZB_ENABLE_NWK_RETRANSMIT
  confirm->nwk_retry_cnt = 0;
#endif
  /* this is broadcast - send from me to ffff */
  confirm->src_addr.addr_short = ZB_PIBCACHE_NETWORK_ADDRESS();
  confirm->dst_addr.addr_short = ZB_NWK_BROADCAST_ALL_DEVICES;

  ZB_SCHEDULE_CALLBACK(zb_mcps_data_confirm, param);
}

static void nwk_broadcasting_unicast_transmission(zb_uint8_t param, zb_uint16_t idx)
{
  zb_uint16_t dst_addr;
  zb_nwk_broadcast_retransmit_t *ent;
  zb_neighbor_tbl_ent_t *nent;

  TRACE_MSG(TRACE_NWK1, ">>nwk_broadcasting_unicast_transmission param %hd idx %d", (FMT__H_D, param, idx));

  ent = &ZG->nwk.handle.brrt[idx];
  if (ent->buf == 0U)
  {
    ent = NULL;
  }

  if (ent != NULL && ent->neighbor_table_iterator == NEIGBOR_TABLE_ITERATOR_END)
  {
    nwk_broadcasting_attempt_completion(ent);
    ent = NULL;
  }

  if (ent != NULL)
  {
    nent = &ZG->nwk.neighbor.neighbor[ZG->nwk.handle.brrt[idx].neighbor_table_iterator];

    zb_address_short_by_ref(&dst_addr, nent->u.base.addr_ref);
    ZB_NWK_ADDR_TO_LE16(dst_addr);

    TRACE_MSG(TRACE_NWK2, "brdcst packet %hd is to be unicasted: iterator %hd dst_addr %x",
              (FMT__H_D_D, ent->buf, ent->neighbor_table_iterator, dst_addr));

    /* do not unicast to the originator */
    if (dst_addr == NWK_SRC_FROM_BUF(ent->buf)
         /*
4.6.3.2 Authentication

4.6.3.2.1 Router Operation

The router shall not forward messages to a child device, or respond to ZDO
requests or NWK command requests on that child's behalf, while the value of the
relationship field entry in the corresponding nwkNeighborTable in the NIB is
0x05 (unauthenticated child).
         */
         || (ZB_NIB_SECURITY_LEVEL() != 0U && nent->relationship == ZB_NWK_RELATIONSHIP_UNAUTHENTICATED_CHILD)
      )
    {
      TRACE_MSG(TRACE_NWK2, "No unicast of broadcast: originator or unauthenticated", (FMT__0));
      ent->neighbor_table_iterator = (zb_uint8_t)zb_nwk_nbr_next_ze_children_i(NWK_DST_FROM_BUF(ent->buf), (zb_ushort_t)ent->neighbor_table_iterator + 1U);

      ZB_SCHEDULE_CALLBACK2(nwk_broadcasting_unicast_transmission, param, idx);
      ent = NULL;
      param = 0;
    }
    else
    {
      TRACE_MSG(TRACE_NWK2, "ent %p retries %hd nbtbl_iter %d buf %hd", (FMT__P_H_D_H, ent, ent->retries_left, ent->neighbor_table_iterator, ent->buf));
    }
  }

  /* check if we have a buffer to be sent */
  if (ent != NULL && param == 0U)
  {
    /* get new one */
    TRACE_MSG(TRACE_NWK2, "brdcst retransmition needs an additional buffer", (FMT__0));

    /* When a BTR entry expires, it stops the corresponding BRRT unicasting.
       So the process does not take very long time even when buffers are not available.
       If a buffer is allocated after the BRRT entry is gone, it will be freed.
    */
    if (zb_buf_get_out_delayed_ext(nwk_broadcasting_unicast_transmission, idx, 0) != RET_OK)
    {
      ent->neighbor_table_iterator = NEIGBOR_TABLE_ITERATOR_END;
      nwk_broadcasting_attempt_completion(ent);
    }

    ent = NULL; /* stop processing */
  }

  if (ent != NULL)
  {
    zb_mcps_data_req_params_t *data_req;

    /* find next child */
    ent->neighbor_table_iterator = (zb_uint8_t)zb_nwk_nbr_next_ze_children_i(NWK_DST_FROM_BUF(ent->buf), (zb_ushort_t)ent->neighbor_table_iterator + 1U);
    TRACE_MSG(TRACE_NWK2, "--retries %hd retransmit_countdown %hd neighbor_table_iterator %hd",
              (FMT__H_H_H, ent->retries_left, ent->retransmit_countdown, ent->neighbor_table_iterator));

    /*
      We are not using the original buffers so as not to compicate the algorithm.
    */
    zb_buf_copy(param, ent->buf);
    data_req = ZB_BUF_GET_PARAM(param, zb_mcps_data_req_params_t);
    /* Ensure that mcps-data.confirm will be silently dropped */
    data_req->msdu_handle = ZB_NWK_INTERNAL_NSDU_HANDLE;

#ifndef ZB_MAC_EXT_DATA_REQ
/* AN: hard to say, is that branch still actual. There are still multiple occasions of its usage across the code.
   Replaced the code below with an error.
*/
#error ZB_MAC_EXT_DATA_REQ abscence is not properly handled here
#else
    /*cstat !MISRAC2012-Rule-1.3_* !MISRAC2012-Rule-9.1_* */
    /* Rule-9.1 and Rule-1.3: "dst_addr" is always initialized when the variable
     * "ent" is not NULL. */
    data_req->dst_addr.addr_short = dst_addr;
    TRACE_MSG(TRACE_NWK2, "dst addr 0x%x", (FMT__D, data_req->dst_addr.addr_short));
#endif

    data_req->tx_options |= (MAC_TX_OPTION_INDIRECT_TRANSMISSION_BIT|MAC_TX_OPTION_ACKNOWLEDGED_BIT);
    /* Note that we actually encrypt NWK from MAC just before
     * send. So just in case we switch key while the packet is in
     * indirect queue, set that flag to encrypt by the previous
     * key. */
    zb_buf_flags_or(param, ZB_BUF_USE_SAME_KEY);
    ZB_SCHEDULE_CALLBACK(zb_mcps_data_request, param);
    param = 0;

    if (ent->neighbor_table_iterator == NEIGBOR_TABLE_ITERATOR_END)
    {
      nwk_broadcasting_attempt_completion(ent);
      ent = NULL;
    }
  }

  if (param != 0U)
  {
    /* free the buffer, it was unnecessary */
    zb_buf_free(param);
    param = 0;
  }

  /* schedule next execution */

  if (ent != NULL)
  {
    ZB_SCHEDULE_CALLBACK2(nwk_broadcasting_unicast_transmission, param, idx);
  }

  TRACE_MSG(TRACE_NWK1, "<<nwk_broadcasting_unicast_transmission", (FMT__0));
}

#endif


/* Helpers */

#ifdef ZB_ROUTER_ROLE

static zb_uint8_t nwk_get_broadcast_jitter_bi(zb_uint8_t param)
{
  zb_nwk_hdr_t *nwhdr = (zb_nwk_hdr_t *)zb_buf_begin(param);
  zb_uint8_t cmd_id = 0, ret_bi;
  zb_uint16_t ret_octets;

#ifdef ZB_PRO_STACK
  if ( ZB_NWK_FRAMECTL_GET_FRAME_TYPE(nwhdr->frame_control) == ZB_NWK_FRAME_TYPE_COMMAND )
  {
    cmd_id = ZB_NWK_CMD_FRAME_GET_CMD_ID(param, ZB_NWK_HDR_SIZE(nwhdr));
  }

  if (cmd_id == ZB_NWK_CMD_ROUTE_REQUEST)
  {
    /* r22 spec says: 2 x R[nwkcMinRREQJitter, nwkcMaxRREQJitter] */
    ret_octets = 2U * (ZB_NWKC_MIN_RREQ_JITTER_OCTETS
                       + (zb_uint16_t)ZB_RANDOM_JTR(ZB_NWKC_MAX_RREQ_JITTER_OCTETS - ZB_NWKC_MIN_RREQ_JITTER_OCTETS));
  }
  else
#endif
  {
    ret_octets = (zb_uint16_t)ZB_RANDOM_JTR(ZB_NWKC_MAX_BROADCAST_JITTER_OCTETS);
  }

  ret_bi = (zb_uint8_t)ZB_NWK_OCTETS_TO_BI(ret_octets);
  TRACE_MSG(TRACE_NWK3, "broadcast delay %hd BI", (FMT__H, ret_bi));

  return ret_bi;
}

static zb_uint8_t nwk_get_broadcast_interval_bi(zb_uint8_t param)
{
  zb_nwk_hdr_t *nwhdr = (zb_nwk_hdr_t *)zb_buf_begin(param);
  zb_uint8_t cmd_id = 0, ret_bi;
  zb_uint16_t ret_octets;

#ifdef ZB_PRO_STACK
  if ( ZB_NWK_FRAMECTL_GET_FRAME_TYPE(nwhdr->frame_control) == ZB_NWK_FRAME_TYPE_COMMAND )
  {
    cmd_id = ZB_NWK_CMD_FRAME_GET_CMD_ID(param, ZB_NWK_HDR_SIZE(nwhdr));
  }

  if (cmd_id == ZB_NWK_CMD_ROUTE_REQUEST && nwhdr->src_addr == ZB_PIBCACHE_NETWORK_ADDRESS())
  {
    ret_octets = ZB_NWKC_RREQ_RETRY_INTERVAL;
  }
  else
#endif
  {
    ret_octets = ZB_NIB().passive_ack_timeout;
  }

  ret_bi = (zb_uint8_t)ZB_NWK_OCTETS_TO_BI(ret_octets);
  TRACE_MSG(TRACE_NWK3, "broadcast interval %hd BI", (FMT__H, ret_bi));

  return ret_bi;
}

static zb_uint8_t nwk_get_broadcast_retries_count(zb_uint8_t param)
{
  zb_nwk_hdr_t *nwhdr = (zb_nwk_hdr_t *)zb_buf_begin(param);
  zb_uint8_t cmd_id = 0, retries;

  retries = ZB_NWK_MAX_BROADCAST_RETRIES;

#ifdef ZB_PRO_STACK
  if ( ZB_NWK_FRAMECTL_GET_FRAME_TYPE(nwhdr->frame_control) == ZB_NWK_FRAME_TYPE_COMMAND )
  {
    cmd_id = ZB_NWK_CMD_FRAME_GET_CMD_ID(param, ZB_NWK_HDR_SIZE(nwhdr));
  }

  /* AD: only one retry for link status commands. It could be done via unicast data request, */
  /* but all broadcast routines still needed. Only retransmissions affected */
  if (cmd_id == ZB_NWK_CMD_ROUTE_REQUEST)
  {
    /* A bit tricky: if radius decreased from our default for
     * RREQ, this is retransmit, send it 2 times.
     * Initially send it 3 times. */
    if (nwhdr->radius != ZB_NIB_MAX_DEPTH() * 2U)
    {
      TRACE_MSG(TRACE_NWK3, "RREQ retransmit", (FMT__0));
      retries = ZB_NWK_RREQ_RETRIES;
    }
    else
    {
      TRACE_MSG(TRACE_NWK3, "RREQ first time", (FMT__0));
      retries = ZB_NWK_INITIAL_RREQ_RETRIES;
    }
  }
  else if (cmd_id == ZB_NWK_CMD_LINK_STATUS
#if defined ZB_MAC_POWER_CONTROL
           || cmd_id == ZB_NWK_CMD_LINK_POWER_DELTA
#endif  /* ZB_MAC_POWER_CONTROL */
          )
  {
    retries = 1;
  }
  else
  {
    /* MISRA rule 15.7 requires empty 'else' branch. */
  }

#endif  /* ZB_PRO_STACK */

  TRACE_MSG(TRACE_NWK3, "broadcast retries %hd ", (FMT__H, retries));

  return retries;
}

#endif


/* Various timers */


#if !(defined ZB_ED_ROLE && defined ZB_ED_RX_OFF_WHEN_IDLE)

static void nwk_broadcasting_btr_aging_alarm(zb_uint8_t param)
{
  zb_nwk_btr_t *ent;

  ZVUNUSED(param);
  TRACE_MSG(TRACE_NWK3, ">>nwk_broadcasting_btr_aging_alarm %hd", (FMT__H, param));

  if (ZG->nwk.handle.btt_cnt != 0U)
  {
    zb_ushort_t i;

    TRACE_MSG(TRACE_NWK3, "btt cnt %hd", (FMT__H, ZG->nwk.handle.btt_cnt));
    for (i = 0; i < ZB_NWK_BTR_TABLE_SIZE; i++)
    {
      ent = &ZG->nwk.handle.btt[i];
      if (!ZB_U2B(ent->used))
      {
        continue;
      }

      if (ent->expiration_time_tx > 0U)
      {
        TRACE_MSG(TRACE_NWK3, "brrt expiration time %hd",
                  (FMT__H, ent->expiration_time_tx));
        ent->expiration_time_tx--;
      }
/*
      else
      {
#ifdef ZB_ROUTER_ROLE

        zb_nwk_broadcast_retransmit_t *brrt_ent = brrt_find_entry_by_nwk_hdr(ent->source_addr, ent->sequence_number);
        if (brrt_ent != NULL)
        {
          nwk_broadcasting_force_completion(brrt_ent);
        }

#endif
      }
*/
      if (ent->expiration_time_rx > 0U)
      {
        TRACE_MSG(TRACE_NWK3, "btt expiration time %hd",
                  (FMT__H, ent->expiration_time_rx));
        ent->expiration_time_rx--;
      }
      else
      {
        TRACE_MSG(TRACE_NWK3, "BTT entry for addr 0x%x seq %d has expired",
                  (FMT__D_D, ent->source_addr, ent->sequence_number));
        NWK_ARRAY_PUT_ENT(ZG->nwk.handle.btt, &ZG->nwk.handle.btt[i], ZG->nwk.handle.btt_cnt);
      }
    }

    /* Schedule to call later */
    if (ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()))
    {
      //ZB_SCHEDULE_ALARM(nwk_broadcasting_btr_aging_alarm, 0, ZB_TIME_ONE_SECOND);
      ZB_SCHEDULE_ALARM(nwk_broadcasting_btr_aging_alarm, 0, ZB_BTR_AGING_CYCLE);
    }
  }
  else
  {
    /* Do not schedule this function, cause we don't have btr's . */
  }

  TRACE_MSG(TRACE_NWK3, "<<nwk_broadcasting_btr_aging_alarm", (FMT__0));
}
#endif

#ifdef ZB_ROUTER_ROLE

static void nwk_broadcasting_brrt_countdown_alarm(zb_uint8_t param)
{
  zb_uindex_t i;
  zb_bool_t stop_allocating = 0;
  ZVUNUSED(param);
  
  TRACE_MSG(TRACE_NWK3, "nwk_broadcasting_brrt_countdown_alarm brrt_cnt %hd", (FMT__H, ZG->nwk.handle.brrt_cnt));
  for (i = 0; i < ZB_NWK_BRR_TABLE_SIZE; i++)
  {
    zb_nwk_broadcast_retransmit_t *ent = &(ZG->nwk.handle.brrt[i]);
    zb_bufid_t buf = 0;

    if (ent->buf == 0U)
    {
      /* empty brrt entry */
      continue;
    }

#if 0
    TRACE_MSG(TRACE_NWK3, "param %hd retransmit_countdown %hd retries %hd been_bc %hd", (FMT__H_H_H_H, ent->buf, ent->retransmit_countdown, ent->retries_left, ent->been_broadcasted));
#endif

    if (ent->retries_left == 0U && ZB_U2B(ent->been_broadcasted))
    {
      /* we need at least one broadcast to happen (passive ack'ing) */
      continue;
    }

    if (ent->retransmit_countdown > 0U)
    {
      ent->retransmit_countdown--;
    }

    if (ent->retransmit_countdown > 0U)
    {
      /* the time has not yet come to broadcast it again */
      continue;
    }

    /* JW DBG */
    zb_nwk_btr_t *entt;

    NWK_ARRAY_FIND_ENT(ZG->nwk.handle.btt, ZB_NWK_BTR_TABLE_SIZE,
                      entt,
                      ((entt->sequence_number == ent->seq_num) && (entt->source_addr == ent->source_addr))
    );
    if (entt == NULL)
    {
      //TRACE_MSG(TRACE_ERROR, "Can't find the btt ent. Force complete", (FMT__0));
      nwk_broadcasting_force_completion(ent);
    }
    else if(entt->expiration_time_tx == 0)
    {
      //TRACE_MSG(TRACE_ERROR, "Tx expiration. Force complete", (FMT__0));
      nwk_broadcasting_force_completion(ent);
    }
    else
    {
          /* we won't send the original buffer, so we need another one */
      if (!stop_allocating)
      {
        buf = zb_buf_get_out();
      }
      if (buf > 0U)
      {
/* Do not set been_broadcasted if we failed to transmit due to lack of buffers. Not sure: maybe, also do not decrease retries_left?
*/
      /* HA 9.2.2.2.2.13 RouteDiscInitiated
         A counter that is incremented each time the network layer
         submits a route discovery message to the MAC. */
        zb_uint8_t new_countdown;

        if (ent->retries_left > 0U)
        {
          ent->retries_left--;
        }

      /* set countdown for the next packet transmission */
        new_countdown = nwk_get_broadcast_interval_bi(ent->buf) / NWK_RETRANSMIT_COUNTDOWN_QUANT;
        if (new_countdown > BRRT_MAX_RETRANSMIT_COUNTDOWN_VALUE)
        {
          new_countdown = BRRT_MAX_RETRANSMIT_COUNTDOWN_VALUE - 1U;
        }
        ent->retransmit_countdown = new_countdown;

        zb_buf_copy(buf, ent->buf);

        ZB_BUF_GET_PARAM(buf, zb_mcps_data_req_params_t)->msdu_handle = ZB_NWK_INTERNAL_NSDU_HANDLE;
        ZB_SCHEDULE_CALLBACK(zb_mcps_data_request, buf);

        if (!ZB_U2B(ent->been_broadcasted))
        {
            /* Checks that the buffer contains an NWK Command and an NWK Command == ZB_NWK_CMD_ROUTE_REQUEST,
               then increments the ZDO_DIAGNOSTICS_ROUTE_DISC_INITIATED_ID diagnostic counter */
          ZDO_DIAGNOSTICS_ROUTE_REQ_INC(ent->buf);
        }

        ent->been_broadcasted = 1U;					

        entt->expiration_time_rx = ZB_NWK_EXPIRY_BROADCAST_RX_CYCLE;
        TRACE_MSG(TRACE_NWK2, "broadcast transmition - call mcps-data.req %hd", (FMT__H, ent->buf));
        nwk_broadcasting_attempt_completion(ent);

      }
      else
      {
        /* If there was no available buffer, we'll try again the next time. The process will not
           be infinite since BRRT will be treated as completed right after the corresponding BTR
           entry becomes expired.
        */
        stop_allocating = 1;
        if (ent->retries_left > 0U)
        {
          ent->retries_left--;
        }
      }
    }
  }

  ZB_SCHEDULE_ALARM_CANCEL(nwk_broadcasting_brrt_countdown_alarm, 0);
  if (ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE())
      && ZG->nwk.handle.brrt_cnt != 0U)
  {
    ZB_SCHEDULE_ALARM(nwk_broadcasting_brrt_countdown_alarm, 0, NWK_RETRANSMIT_COUNTDOWN_QUANT);
  }
}

static void nwk_broadcasting_start_brrt_countdown_alarm(void)
{
  if (ZG->nwk.handle.brrt_cnt == 1U)
  {
    TRACE_MSG(TRACE_NWK3, "scheduling nwk_broadcasting_brrt_countdown_alarm", (FMT__0));
    ZB_SCHEDULE_ALARM_CANCEL(nwk_broadcasting_brrt_countdown_alarm, 0);
    if (ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()))
    {
      ZB_SCHEDULE_ALARM(nwk_broadcasting_brrt_countdown_alarm, 0, NWK_RETRANSMIT_COUNTDOWN_QUANT);
    }
  }
}

#endif

/*! @} */
