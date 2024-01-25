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
/* PURPOSE: MAC layer main module
*/

#define ZB_TRACE_FILE_ID 294
#include "zb_common.h"
#include "util_log.h"
#if !defined ZB_ALIEN_MAC && !defined ZB_MACSPLIT_HOST

#include "zb_mac.h"
#include "mac_internal.h"
#include "zb_mac_globals.h"
#include "zb_ie.h"
#include "zb_nwk.h"

/*! \addtogroup ZB_MAC */
/*! @{ */


zb_mac_globals_t g_mac;
volatile zb_intr_mac_globals_t g_imac;

static void zb_mac_recv_data(void);
void zb_mlme_handle_in_command(zb_uint8_t param);
void zb_mlme_handle_data_req_command(zb_uint8_t param);
void zb_handle_mlme_poll_request(zb_uint8_t param);

#ifdef ZB_ROUTER_ROLE
static zb_bool_t is_association_resp(zb_bufid_t buf);
#endif
#ifdef MAC_RADIO_TX_WATCHDOG_ALARM
void mac_tx_watchdog(zb_uint8_t param);
#endif

static void zb_mlme_reset_request_sync(zb_uint8_t param);

static void zb_mac_create_pan_desc_from_beacon(zb_bufid_t beacon_buf, const zb_mac_mhr_t *mhr,
                                               zb_zigbee_vendor_pie_parsed_t *vendor_parsed,
                                               zb_pan_descriptor_t *pan_desc);

static zb_bool_t can_accept_frame(const zb_mac_mhr_t *mhr);

#if defined ZB_ENHANCED_BEACON_SUPPORT
static void mac_handle_802154_ver2_frame(zb_uint8_t param);
#endif /* ZB_ENHANCED_BEACON_SUPPORT */

static void zb_check_cmd_tx_status(void);

void zb_mac_init(void)
{
  TRACE_MSG( TRACE_MAC1, ">>mac_init", (FMT__0 ));

  /* Init globals. Note: zb_mac_init must be run before default IB load */
#ifndef ZB_CONFIGURABLE_MEM
  ZB_BZERO(&g_mac, sizeof(g_mac));
  g_mac.mac_ctx.current_tx_power = ZB_MAC_TX_POWER_INVALID_DBM;
#endif
  zb_bzero_volatile(&g_imac, sizeof(zb_intr_mac_globals_t));
#ifdef USE_HW_LONG_ADDR
  ZB_GET_HW_LONG_ADDR((zb_ieee_addr_t*)MAC_PIB().mac_extended_address);
#endif

  MAC_CTX().operation_buf = zb_buf_get_any();
#ifndef ZB_AUTO_ACK_TX
  MAC_CTX().ack_tx_buf = zb_buf_get_any();
#endif

  TRACE_MSG(TRACE_MAC2, "operation_buf %hd", (FMT__H, MAC_CTX().operation_buf));

#ifdef ZB_MAC_RX_QUEUE_CAP
  {
    zb_uindex_t i;

    for (i = 0; i < ZB_MAC_RX_QUEUE_CAP; ++i)
    {
      MAC_CTX().mac_rx_queue.ring_buf[i] = zb_buf_get(ZB_TRUE, 0);
    }
  }
#endif

#ifdef ZB_MAC_TESTING_MODE
  MAC_CTX().active_scan.buf_param = 0;
#endif

  zb_mac_reinit_pib();
  TRACE_MSG(TRACE_MAC2, "CLEAR_ACK_NEEDED", (FMT__0));
  ZB_MAC_CLEAR_ACK_NEEDED();

#if defined ZB_MACSPLIT_DEVICE
  zb_macsplit_transport_init();
#endif
  TRACE_MSG(TRACE_MAC1, "<<mac_init", (FMT__0));
}


/**
   Return true if MAC is idle - means, MCU can be put asleep.
 */
static zb_bool_t zb_mac_is_idle(void)
{
  return
#ifdef ZB_MAC_RX_QUEUE_CAP
    /* MISRA Rule 13.5 - Can't access volatile objects in the right operand of && or || operations,
       recv_buf_full is volatile. */
    /*TRANS_CTX().recv_buf_full == 0U
    && */
    ZB_RING_BUFFER_IS_EMPTY(&MAC_CTX().mac_rx_queue)
#else
    ZB_TRUE
#endif
    && !MAC_CTX().flags.mlme_scan_in_progress /* Do not sleep during mlme scan */
    && MAC_CTX().flags.ass_state == ZB_MAC_ASS_STATE_NONE /* Do not sleep during association */
    && !MAC_CTX().flags.tx_q_busy
    && ZB_RING_BUFFER_IS_EMPTY(&ZG->sched.mac_tx_q)
#ifdef ZB_SUB_GHZ_LBT
    && !MAC_CTX().flags.lbt_radio_busy
#endif
    && (MAC_CTX().flags.poll_inprogress == 0U) /* Do not sleep during poll */
    && (MAC_CTX().flags.tx_radio_busy == 0U) /* TX is busy */
    && !ZB_IS_TRANSPORT_BUSY()
    && !ZG->bpool.mac_rx_need_buf;
}

zb_bool_t zb_mac_allows_transport_iteration(void)
{
  /* Blocking transport iteration can not be executed if there is some scheduled
   * TX callback that is ready to be called (radio is not trying to send any other packet).
   *
   * TX callback should be called before blocking iterations to prevent unnecessary delays
   * before packet sending. If radio is already busy, it is impossible to call new TX callback,
   * so blocking iteration is allowed to wait for free radio state.
   */
  return ZB_RING_BUFFER_IS_EMPTY(&ZG->sched.mac_tx_q) || MAC_CTX().flags.tx_radio_busy;
}

/**
   Synchronizes transceiver state with ZB_PIB_RX_ON_WHEN_IDLE() values.

   IMPORTANT: must not be called when there is a possibility of an ongoing transmission.
   The safest way is to schedule the function via tx_q.
 */
void zb_mac_sync_rx_on_off_with_pib(zb_uint8_t unused)
{
  ZVUNUSED(unused);
  //ZB_TRANSCEIVER_SET_RX_ON_OFF(ZB_PIB_RX_ON_WHEN_IDLE());
}

/**
   Main MAC logic iteration.

   To be called by the scheduler

   First get int status, then do rx and in data parse, then tx/retransmit

   @return RET_OK if success, RET_BLOCKED if MCU can go asleep
*/
zb_ret_t zb_mac_logic_iteration (void)
{
  zb_ret_t ret = RET_OK;

  /* Linux/ns i/o here */
//  (void)ZB_TRANSPORT_NONBLOCK_ITERATION();
  /* Radio process statistics here: iteration logic here */
  ZB_RADIO_PROCESS_STATISTIC(RADIO_STAT_ITERATION);

#ifdef ZB_MAC_SYNC_RSSI
  if (MAC_CTX().flags.sync_rssi_in_progess)
  {
    zb_int8_t new_value = ZB_TRANSCEIVER_GET_SYNC_RSSI();
    if (new_value > MAC_CTX().max_measured_sync_rssi != 0)
    {
      MAC_CTX().max_measured_sync_rssi = new_value;
    }
  }
#endif /* ZB_MAC_SYNC_RSSI */

  /* Transceiver int flag is set by the interrupt handler */
  if (ZB_MAC_GET_TRANS_INT_FLAG() != 0U)
  {
    ZB_MAC_CLEAR_TRANS_INT();
    /* Read transceiver status register (registers). Read pending data bit  */
    ZB_MAC_READ_INT_STATUS_REG(); /*Look for the interrupt reason:read status
                                   * register */
#ifdef ZB_AUTO_ACK_N_RETX
    /* Actually purt to uz2400 is not alive, but let's keep it there.. */
    /* Transceiver-specific logic for UZ sets pending data bit after every tx, but it is valid
     * only after data request is sent */
    if (!ZB_MAC_INDIRECT_IN_PROGRESS())
    {
      MAC_CTX().flags.in_pending_data = ZB_FALSE;
    }
    TRACE_MSG(TRACE_MAC3, "in_pending_data %hd", (FMT__H, MAC_CTX().flags.in_pending_data));
#endif
  }

  {
    zb_bool_t need_rx;

    ZB_GET_RADIO_FLAG_INT_DISABLE();
    need_rx = (ZB_U2B(ZB_MAC_GET_RX_INT_STATUS_BIT())
#ifdef ZB_MAC_RX_QUEUE_CAP
               || !ZB_RING_BUFFER_IS_EMPTY(&MAC_CTX().mac_rx_queue)
#endif
      );
    ZB_GET_RADIO_FLAG_INT_ENABLE();
    /*Check if we have something to recieve*/
    if (need_rx)
    {
#ifdef ZB_MAC_RX_QUEUE_CAP
      TRACE_MSG(TRACE_MAC3, "RX int: recv_buf_full %d rx q empty %d - call zb_mac_recv_data",
                (FMT__D_D, TRANS_CTX().recv_buf_full,
                 ZB_RING_BUFFER_IS_EMPTY(&MAC_CTX().mac_rx_queue)));
#else /* ZB_MAC_RX_QUEUE_CAP */
      TRACE_MSG(TRACE_MAC3, "RX int: call zb_mac_recv_data", (FMT__0));
#endif /* ZB_MAC_RX_QUEUE_CAP */
      zb_mac_recv_data();
    } /*End of RX buffer checkout*/
  }

  {
    zb_bool_t need_tx;

    /* Check for TX done */
    ZB_GET_RADIO_FLAG_INT_DISABLE();
    need_tx = ZB_U2B(ZB_MAC_GET_TX_INT_STATUS_BIT());
    ZB_GET_RADIO_FLAG_INT_ENABLE();
    if (need_tx)
    {
      TRACE_MSG(TRACE_MAC3, "TX int. ACK need %d", (FMT__D, ZB_MAC_GET_ACK_NEEDED()));

#if defined ZB_MAC_TESTING_MODE
      MAC_CTX().tx_timestamp = ZB_TRANS_GET_TX_TIMESTAMP();
      TRACE_MSG(TRACE_MAC1, "TX timestamp=0x%lx", (FMT__L, MAC_CTX().tx_timestamp));
#endif  /* ZB_MAC_TESTING_MODE */

    /* For UZ4200 get ACK recv status here. For other transceivers just check
     * tx status */
      zb_check_cmd_tx_status();
      ZB_MAC_CLEAR_TX_INT_STATUS_BIT();

#if defined ZB_MAC_POWER_CONTROL
      zb_mac_power_ctrl_restore_tx_power();
#endif  /* ZB_MAC_POWER_CONTROL */

      if (!ZB_MAC_GET_ACK_NEEDED()
#if defined ZB_SUB_GHZ_LBT
          && (ZB_LOGICAL_PAGE_IS_2_4GHZ(MAC_PIB().phy_current_page)
              || !MAC_CTX().flags.lbt_radio_busy
#if defined ZB_MAC_TESTING_MODE
          || MAC_CTX().cert_hacks.lbt_radio_busy_disabled
#endif /* ZB_MAC_TESTING_MODE */
            )
#endif /* ZB_SUB_GHZ_LBT */
        )
      {
        MAC_CTX().flags.tx_q_busy = ZB_FALSE;
      }
    }
  }


  /* At modern HW ports parse acks from the fifo read routine. It can be called from int
   * handler or from zb_mac_recv_data() */
  if (ZB_MAC_GET_ACK_RECEIVED() != 0U
#if defined ZB_SUB_GHZ_LBT
      /* Usecase:
       * - we try to send the first time a packet that requires
       * an mac-ack and can't do it because of duty_cycle overflow;
       * - we don't schedule zb_mac_ack_timeout() that doesn't
       * set ZB_MAC_SET_ACK_TIMED_OUT(); and we won't be able to drop
       * the tx_q_busy flag;
       *
       * We must terminate any transmission because of duty_cycle overflow
       * and send mcps_data_confirm with the MAC_LIMIT_REACHED status.
       */
      || ZB_GET_MAC_STATUS() == MAC_LIMIT_REACHED
#endif /* ZB_SUB_GHZ_LBT */
    )
  {
    if (ZB_MAC_GET_ACK_NEEDED())
    {
      MAC_CTX().flags.in_pending_data = (ZB_MAC_GET_ACK_RECEIVED_PEND() != 0U && ZB_MAC_INDIRECT_IN_PROGRESS());

#if defined ZB_MAC_AUTO_ACK_RECV && defined ZB_TRAFFIC_DUMP_ON
      {
        zb_uint_t dsn = MAC_ICTX().ack_dsn;
        zb_mac_dump_mac_ack(ZB_FALSE, (zb_uint8_t)ZB_MAC_GET_ACK_RECEIVED_PEND(), (zb_uint8_t)dsn);
      }
#endif

      TRACE_MSG(TRACE_MAC1, "got mac ack, pending data %hd, ack_dsn %d",
                (FMT__H_D, (zb_uint8_t)MAC_CTX().flags.in_pending_data, MAC_ICTX().ack_dsn));

      ZB_MAC_CLR_ACK_RECEIVED();
      ZB_MAC_CLEAR_ACK_NEEDED();
      ZB_MAC_CLEAR_ACK_TIMED_OUT();
      ZB_SCHEDULE_ALARM_CANCEL(zb_mac_ack_timeout, ZB_ALARM_ANY_PARAM);
      MAC_CTX().retx_buf = 0;
      MAC_CTX().retry_counter = 0;
      MAC_CTX().flags.tx_q_busy = ZB_FALSE;

      if (!ZB_U2B(ZB_PIB_RX_ON_WHEN_IDLE()) && !MAC_CTX().flags.in_pending_data && !ZB_MAC_INDIRECT_IN_PROGRESS())
      {
        /* TODO: implement a switcher for lower level: no auto rx on after rx. Some lower layers
         * have it done already. */
        //ZB_TRANSCEIVER_SET_RX_ON_OFF(ZB_PIB_RX_ON_WHEN_IDLE());
      }
    }
  }

  /* Transmit section */

  /* If not manual ack, next condition is if (1) */
  if (!ZB_MAC_GET_ACK_NEEDED()) /* If waiting for ACK, do not work with tx
                                 * callbacks queue: TX is not really done, retransmit
                                 * is possible */
  {
#if 0
      /* Not waiting for ack - done with previous packet tx */
      TRACE_MSG(TRACE_MAC2, "tx_wait_cb %p tx_q_busy %hd tx q %p trans time %u",
                (FMT__P_H_P_D, MAC_CTX().tx_wait_cb, MAC_CTX().flags.tx_q_busy,
                ZB_RING_BUFFER_PEEK(&ZG->sched.mac_tx_q), osif_transceiver_time_get()
                ));
#endif
    /* tx_wait_cb is set if we are waiting for TX complete */

    if (!MAC_CTX().flags.tx_q_busy
        && !MAC_CTX().flags.tx_radio_busy
        && !ZB_IS_TRANSPORT_BUSY()
      )
    {
      zb_mac_cb_ent_t *mac_cb_ent;

      if (MAC_CTX().tx_wait_cb != NULL)
      {
        /* TX complete. Call function waiting for it. */
        TRACE_MSG(TRACE_MAC2, "call tx waiting callback %p arg %d",
                  (FMT__P_H, MAC_CTX().tx_wait_cb, MAC_CTX().tx_wait_cb_arg));
        (MAC_CTX().tx_wait_cb)(MAC_CTX().tx_wait_cb_arg);
        MAC_CTX().tx_wait_cb = NULL;
      }
      /* After tx callback done can execute next cb from the tx queue */

      /* Nobody is waiting for TX complete. TX was not in
       * progress. Can get next TX request from TX queue. */

      /* TRACE_MSG(TRACE_MAC2, "No TX cb - try tx q (hdr %p)", (FMT__P, ZB_RING_BUFFER_PEEK(&ZG->sched.mac_tx_q))); */

      /* Use separate callbacks queue for TX: can call next callback only
       * after previous TX finished.
       * If no tx_wait_cb assigned, TX is idle.
       */

      /* calling callbacks, that need free tx */
      if ((mac_cb_ent = ZB_RING_BUFFER_PEEK(&ZG->sched.mac_tx_q)) != NULL
#if defined ZB_SUB_GHZ_LBT
          /* For Sub-GHz LBT check the LBT finished flag only for callbacks, that need free TX.
           * TX complete callback (MAC_CTX().tx_wait_cb) should be called during LBT as
           * LBT can take quite a long time. */
        && !MAC_CTX().flags.lbt_radio_busy
#endif
        )
      {
        TRACE_MSG(TRACE_MAC2, "tx callback call func %p", (FMT__P, mac_cb_ent->func));
        MAC_CTX().flags.tx_ok_to_send = ZB_TRUE; /* to be able to do assert */
        (*mac_cb_ent->func)(mac_cb_ent->param);
        ZB_RING_BUFFER_FLUSH_GET(&ZG->sched.mac_tx_q);
      }
    } /* if tx done */
  } /* if ack not need or no manual acks */

  /* retransmit section*/

  /* No need to put under ifdef for automatic ACKs: ZB_MAC_GET_ACK_TIMEOUT is
   * defined to 0 in such case */
  if (ZB_MAC_GET_ACK_TIMED_OUT()        /*Check for ACK timeout*/
      && !ZB_IS_TRANSPORT_BUSY()
      && !MAC_CTX().flags.tx_radio_busy
#if defined ZB_SUB_GHZ_LBT
      && !MAC_CTX().flags.lbt_radio_busy
#endif
    )
  {
    ZB_MAC_CLR_ACK_RECEIVED();
    /* ack is not received during timeout period or ack is failed */
    /* increase re-try counter, check it and try again to send command */
    ZB_SCHEDULE_ALARM_CANCEL(zb_mac_ack_timeout, ZB_ALARM_ANY_PARAM); /* cancel scheduled alarm */
    MAC_CTX().retry_counter++;

    if (
        /*If retransmit attempt counter > maximum retransmit attepmt number
          stop retransmittion*/
        MAC_CTX().retry_counter > ZB_MAC_PIB_MAX_FRAME_RETRIES
/*
  If a single transmission attempt has failed and the transmission was indirect, the coordinator shall not
  retransmit the data or MAC command frame. Instead, the frame shall remain in the transaction queue of the
  coordinator and can only be extracted following the reception of a new data request command.

  Note that we do NOT count number of retransmits for indirect transmissions. Instead, they stays at the queue until timeout.
  That is how I can understand 802.15.4 specification which mentiuons macMaxFrameRetries for direct transmissions only.
*/
#ifdef ZB_ROUTER_ROLE
        /*cstat -MISRAC2012-Rule-13.5 */
        /* After some investigation, the following violation of Rule 13.5 seems to be
         * a false positive. There are no side effects to 'is_association_resp()'. This
         * violation seems to be caused by the existance of external functions inside
         * which cannot be analyzed by C-STAT. */
        || (MAC_CTX().tx_wait_cb == zb_handle_data_request_cmd_continue
            /* Some radio (MAC), if misses association resp, does not send data
             * request again. Let's increase their chances to associate by
             * re-sending. */
            && !is_association_resp(MAC_CTX().retx_buf))
            /*cstat +MISRAC2012-Rule-13.5 */
#endif
    )
    {
      TRACE_MSG(TRACE_MAC1, "out of retransmits (%d), no ACK", (FMT__D, MAC_CTX().retry_counter));
      //ZB_TRANSCEIVER_SET_RX_ON_OFF(ZB_PIB_RX_ON_WHEN_IDLE());
      ZB_MAC_CLEAR_ACK_NEEDED();
      ZB_MAC_CLEAR_ACK_TIMED_OUT();

      MAC_CTX().flags.tx_q_busy = ZB_FALSE;

      /* Forget buf for retx. Pass it up in the confirm. If it was poll req,
       * retx is operation buf, so no need to free it. */
      MAC_CTX().retx_buf = 0;
      MAC_CTX().retry_counter = 0;

      ZB_SET_MAC_STATUS(MAC_NO_ACK);

      if (MAC_CTX().tx_wait_cb != NULL)
      {
        ZB_MAC_DIAGNOSTIC_UNICAST_TX_FAILED_INC();
        TRACE_MSG(TRACE_MAC1, "Data TX failed. Calling tx waiting callback %p arg %d",
                  (FMT__P_H, MAC_CTX().tx_wait_cb, MAC_CTX().tx_wait_cb_arg));
        (*MAC_CTX().tx_wait_cb)(MAC_CTX().tx_wait_cb_arg);
        MAC_CTX().tx_wait_cb = NULL;
      }
    }
    else /* next retransmit attempt */
    {
      ZB_ASSERT(MAC_CTX().retx_buf != 0U);
      TRACE_MSG(TRACE_MAC1, "ACK timeout - retransmit # %hd buf %hd", (FMT__H_H, MAC_CTX().retry_counter, MAC_CTX().retx_buf));

      ZB_MAC_CLEAR_TX_INT_STATUS_BIT();
      MAC_CTX().flags.tx_ok_to_send = ZB_TRUE; /* to be able to do assert */

      /*
       * zb_mac_send_frame() detects retransmit and skip loading of packet into HW
       * TX FIFO if it is possible. */

      zb_mac_send_frame(MAC_CTX().retx_buf, MAC_CTX().retx_len);
    }
  } /*if (ZB_MAC_GET_ACK_TIMEOUT()) - valid for manual acks only */
  MAC_CTX().flags.tx_ok_to_send = ZB_FALSE;

#if !defined ZB_ED_ROLE && !defined ZB_MAC_STICKY_PENDING_BIT
  if (mac_pending_queue_is_empty() && ZB_MAC_TRANS_PENDING_BIT())
  {
    TRACE_MSG(TRACE_MAC1, "Clearing pending bit", (FMT__0));
    ZB_MAC_TRANS_CLEAR_PENDING_BIT();
  }
#endif

  if (zb_mac_is_idle())
  {
    ret = RET_BLOCKED;
  }

#if 0
  TRACE_MSG(TRACE_MAC3, "<<zb_mac_logic_iteration %d", (FMT__D, ret));
#endif
  return ret;
} /*zb_mac_logic_iteration()*/


#ifdef ZB_ROUTER_ROLE
static zb_bool_t is_association_resp(zb_bufid_t buf)
{
  zb_mac_mhr_t mhr;
  zb_bool_t ret = ZB_FALSE;

  if (buf != 0U)
  {
    (void)zb_parse_mhr(&mhr, buf);
    if (ZB_FCF_GET_FRAME_TYPE(mhr.frame_control) == MAC_FRAME_COMMAND
        /*cstat !MISRAC2012-Rule-13.5 */
        /* After some investigation, the following violation of Rule 13.5 seems to be a false
         * positive. There are no side effect to 'zb_buf_data()'. This violation seems to be caused
         * by the fact that 'zb_buf_data()' is an external macro, which cannot be analyzed by
         * C-STAT. */
        && *(zb_uint8_t *)zb_buf_data(buf, mhr.mhr_len) == MAC_CMD_ASSOCIATION_RESPONSE)
    {
      ret = ZB_TRUE;
    }
  }
  return ret;
}
#endif

/**
   Get Beacon payload offset
*/
zb_ushort_t zb_mac_get_beacon_payload_offset(zb_uint8_t *beacon)
{
  zb_mac_mhr_t mhr;
  zb_uint8_t   gts_len;
  zb_uint8_t   pend_addr_len;

  /* Calc MAC header length */
  (void)zb_parse_mhr_ptr(&mhr, beacon);
  /* Superframe Specification field length is constant */
  beacon = &beacon[mhr.mhr_len + SUREPFRAME_SPEC_LENGTH];

  /* Calc GTS filds length. If GTS Descriptor Count field (first 3 bits in GTS Specification field) is
   * not zero then the packet contains GTS Descriptors (3 bytes each) plus GTS Directions field (1 byte) */
  if ((*beacon & 0x7U) == 0U)
  {
    gts_len = GTS_SPEC_LENGTH;
  }
  else
  {
    gts_len = (*beacon & 0x7U) * GTS_DESCRIPTOR_LENGTH + GTS_DIRECTIONS_LENGTH + GTS_SPEC_LENGTH;
  }
  beacon = &beacon[gts_len];

  /* Calc Pending Address fields length. */
  pend_addr_len = (((*beacon & 0x7U) == 0U) && (((*beacon >> 4) & 0x7U) == 0U) ? PENDING_ADDRESS_SPEC_LENGTH :
                   (PENDING_ADDRESS_SPEC_LENGTH + (*beacon & 0x7U) * (zb_uint8_t)sizeof(zb_uint16_t) +
                    ((*beacon >> 4) & 0x7U) * (zb_uint8_t)sizeof(zb_ieee_addr_t)));

  return ((zb_ushort_t)mhr.mhr_len + SUREPFRAME_SPEC_LENGTH + gts_len + pend_addr_len);
}

/**
   Read data from the transceiver FIFO, parse it.

   Allocate buffer if necessary
*/
static void zb_mac_recv_data(void)
{
  zb_uint8_t *cmd_ptr = NULL;
  zb_mac_mhr_t mhr;
  zb_bufid_t new_buf;
  zb_bufid_t buf;
  zb_uint8_t l_recv_buf_full;

#ifndef ZB_COORDINATOR_ONLY
  zb_mac_status_t mac_status;
#endif

  TRACE_MSG(TRACE_MAC3, ">>zb_mac_recv_data", (FMT__0));

  /* NK: Try to use hi-priority buffers also */
  new_buf = zb_buf_get_hipri(ZB_TRUE);
#ifdef ZB_PROMISCUOUS_MODE
  if (ZB_U2B(MAC_PIB().mac_promiscuous_mode) && new_buf == 0U)
  {
    /* Use out buffers too */
    new_buf = zb_buf_get_hipri(ZB_FALSE);
  }
#endif


  buf = 0U;
  if (new_buf != 0U)
  {
#ifdef ZB_MAC_RX_QUEUE_CAP
    zb_bool_t radio_int_disabled = ZB_TRUE;
    /* put this buf into the queue, get buf from queue to buf */

    ZB_RADIO_INT_DISABLE();

    ZB_MAC_CLEAR_RX_INT_STATUS_BIT();

    if (!ZB_RING_BUFFER_IS_EMPTY(&MAC_CTX().mac_rx_queue))
    {
      zb_uint8_t bi = *ZB_RING_BUFFER_GET(&MAC_CTX().mac_rx_queue);
      /* mac_rx_queue is used in both directions: as received packets
       * from ISR to the main logic and as free packets from the main
       * logic to ISR. Reassign buffer to pass fresh buffer to ISR. */
#ifndef MAC_Q_BUFFER_AVAILABLE
      /* Note: most of the radio: pass new buffer to the ISR using the same ring buffer */
      *ZB_RING_BUFFER_GET(&MAC_CTX().mac_rx_queue) = new_buf;
#endif
      /* Must flush RX queue before adding buf before adding buf to the low level RX buffer */
      ZB_RING_BUFFER_FLUSH_GET(&MAC_CTX().mac_rx_queue);
#ifdef MAC_Q_BUFFER_AVAILABLE
      zb_bool_t need_ena_rx = MAC_Q_BUFFER_AVAILABLE(new_buf);
#endif
      ZB_RADIO_INT_ENABLE();
#ifdef MAC_Q_BUFFER_AVAILABLE
      /*cstat !MISRAC2012-Rule-14.3_b */
      if (need_ena_rx)
      {
        /** @mdr{00025,0} */
        MAC_Q_ENA_RX();
      }
#endif
      TRANS_CTX().recv_buf_full = 0;
      buf = bi;
      new_buf = 0U;
      if(zb_buf_len(buf) == 0U)
      {
        TRACE_MSG(TRACE_MAC2, "zero-length buf %hd. Hmm.", (FMT__D, buf));
        zb_buf_free(buf);
        buf = 0;
      }

      /* There is a free slot in the ring buffer, so we can accept a new packet */
      ZB_TRANSCEIVER_ENABLE_AUTO_ACK();

#ifdef ZB_MAC_SINGLE_PACKET_IN_FIFO
      /* Just added buffer into rx queue. If interrupt logic was out of rx
       * buffers, re-start rx. */
      ZB_RADIO_INT_DISABLE();
      radio_int_disabled = ZB_TRUE;
      l_recv_buf_full = TRANS_CTX().recv_buf_full;
      if (TRANS_CTX().recv_buf_full != 0U)
      {
        /* ISR stopped recv because it was out of buffers. Now start rx again. Valit for some radio only. */
        TRANS_CTX().recv_buf_full = 0U;
        /* Better enable interrupts before radio start. */
        ZB_RADIO_INT_ENABLE();
        radio_int_disabled = ZB_FALSE;
        (void)ZB_SCHEDULE_TX_CB(zb_mac_sync_rx_on_off_with_pib, 0);
        ZB_MAC_DIAGNOSTICS_RX_QUE_FULL_INC(l_recv_buf_full);
      }
#else
      radio_int_disabled = ZB_FALSE;
#endif

      /* to fix MISRAC2012-Rule-2.2_c */
      ZVUNUSED(l_recv_buf_full);

      /* Radio process statistics here: queue item free operation */
      ZB_RADIO_PROCESS_STATISTIC(RADIO_STAT_QUEUE_ITEM_FREE);
    }

    if (radio_int_disabled)
    {
      ZB_RADIO_INT_ENABLE();
    }

#ifdef ZB_TRANS_RECV_PACKET
    if (buf == 0U)
    {
      /* Defined ZB_TRANS_RECV_PACKET means we can get packet from fifo not only from ISR, but here, too. Only one platform utilizes it. Kill it? */
      zb_bufid_t *b_p = ZB_RING_BUFFER_PUT_RESERVE(&MAC_CTX().mac_rx_queue);
      while (TRANS_CTX().recv_buf_full != 0U && b_p != NULL)
      {
        /* Note: packet read when all interrupts disabled is not always possible.
           Disable transceiver only?
        */

        /*cstat -MISRAC2012-Rule-2.1_b */
        if (ZB_TRANS_RECV_PACKET(*b_p))
        {
          /*cstat +MISRAC2012-Rule-2.1_b */
          /** @mdr{00013,0} */
          ZB_RING_BUFFER_FLUSH_PUT(&MAC_CTX().mac_rx_queue);
          ZB_ASSERT(zb_buf_len(*b_p) > 0);
          b_p = ZB_RING_BUFFER_PUT_RESERVE(&MAC_CTX().mac_rx_queue);
        }
        else
        {
          TRANS_CTX().recv_buf_full = 0U;
        }
      }
    }
#endif  /* ZB_TRANS_RECV_PACKET */

#else  /* ZB_MAC_RX_QUEUE_CAP */
    /* No rx queue. Get packet from fifo only here. Do we have such a platform now? */
    ZB_RADIO_INT_DISABLE();
    ZB_MAC_CLEAR_RX_INT_STATUS_BIT();
    ZB_RADIO_INT_ENABLE();

    buf = new_buf;
    if (ZB_TRANS_RECV_PACKET(buf) == 0)
    {
      buf = 0;
    }
#endif  /* ZB_MAC_RX_QUEUE_CAP */

    if (buf == 0U)
    {
      if (new_buf != 0U)
      {
        zb_buf_free(new_buf);
      }
    }

    /* note: no explicit  initial alloc call. Buffer will be started from
     * the beginning. */
    zb_buf_set_mac_rx_need(ZB_FALSE);
  }
  else
  {
    TRACE_MSG(TRACE_MAC2, "set mac_rx_need_buf", (FMT__0));
    zb_buf_set_mac_rx_need(ZB_TRUE);

  }

#ifdef ZB_PROMISCUOUS_MODE
  if (buf != 0U && MAC_PIB().mac_promiscuous_mode != 0U)
  {
    if (MAC_PIB().mac_promiscuous_mode_cb != NULL)
    {
      ZB_SCHEDULE_CALLBACK(MAC_PIB().mac_promiscuous_mode_cb, buf);
    }
    else
    {
      zb_buf_free(buf);
    }

    buf = 0U;
  }
#endif

  if (buf != 0U)
  {
    ZB_ASSERT(zb_buf_len(buf) > 0U);
    ZB_TRANS_CUT_SPECIFIC_HEADER(buf);

    cmd_ptr = zb_buf_begin(buf);
    cmd_ptr += zb_parse_mhr(&mhr, buf);

    TRACE_MSG(TRACE_MAC1, "RX buf %hd, len %d, frame_control 0x%hx 0x%hx, seq_number %hu, dst_pan_id 0x%x, src_pan_id 0x%x dst mode %hi addr " TRACE_FORMAT_64 " src mode %hi addr " TRACE_FORMAT_64,
              (FMT__H_D_H_H_H_D_D_H_A_H_A,
               buf, zb_buf_len(buf),
               mhr.frame_control[0], mhr.frame_control[1], mhr.seq_number, mhr.dst_pan_id, mhr.src_pan_id,
               ZB_FCF_GET_DST_ADDRESSING_MODE((mhr).frame_control), TRACE_ARG_64((mhr).dst_addr.addr_long),
               ZB_FCF_GET_SRC_ADDRESSING_MODE((mhr).frame_control), TRACE_ARG_64((mhr).src_addr.addr_long)));

    ZB_MAC_UPDATE_RX_ZCL_DIAGNOSTIC(&mhr, buf);

    /* check visibility, destination, panid */
    if (!can_accept_frame(&mhr)
        /* NK: check corrupted packet - wrong len */

        /*cstat !MISRAC2012-Rule-13.5 */
        /* After some investigation, the following violation of Rule 13.5 seems to be
         * a false positive. There are no side effects to 'zb_buf_len()'. This violation
         * seems to be caused by the fact that 'zb_buf_len()' is an external function,
         * which cannot be analyzed by C-STAT. */
        || (mhr.mhr_len + (unsigned)ZB_TAIL_SIZE_FOR_RECEIVED_MAC_FRAME > zb_buf_len(buf)))
    {
      TRACE_MSG(TRACE_MAC2, "Cannot accept frame or wrong pkt len", (FMT__0));
#ifndef PHY_RX_TEST_CMW			
      zb_buf_free(buf);
      buf = 0;
#endif
	  ZB_MAC_DIAGNOSTICS_VALIDATE_DROP_CNT_INC();
    }
    else
    {
#ifndef MAC_DUMP_FROM_INTR
      ZB_DUMP_INCOMING_DATA(buf);
#endif
#if defined ZB_AUTO_ACK_TX && defined ZB_TRAFFIC_DUMP_ON
      if (ZB_FCF_GET_ACK_REQUEST_BIT(mhr.frame_control) != 0U)
      {
        zb_uint8_t data_pending =
#ifdef ZB_ROUTER_ROLE
          /*
          mac_check_pending_data(&mhr, 0) >= 0 ? 1 : 0;
          */
          /* The presence MAC_ACK_pending_bit is passed up from the platform in status */
          ((zb_uint8_t)zb_buf_get_status(buf)) == MAC_NO_DATA ? 0U : 1U;
#else
          0;
#endif
        zb_mac_dump_mac_ack(ZB_TRUE, data_pending, mhr.seq_number);
      }
#endif
    }
  }


#ifdef ZB_MAC_TESTING_MODE
  if (buf && ZB_FCF_GET_SECURITY_BIT(mhr.frame_control)
      && ZB_FCF_GET_FRAME_VERSION(mhr.frame_control) < MAC_FRAME_IEEE_802_15_4)
  {
    TRACE_MSG(TRACE_MAC1, "unsupported 2003 security frame dropped", (FMT__0));
    ZB_SET_MAC_STATUS(MAC_UNSUPPORTED_LEGACY);
    zb_mac_call_comm_status_ind(buf, ZB_GET_MAC_STATUS(), buf);
    buf = 0;
    ZB_MAC_DIAGNOSTICS_VALIDATE_DROP_CNT_INC();
  }
#endif


#ifndef ZB_COORDINATOR_ONLY
  if (buf != 0U
      && ZB_MAC_INDIRECT_IN_PROGRESS())
  {
/* if there is indirect request, check if received frame is sent as a
 * response to this request: dst addr == current device addr; maybe it
 * will be good to check source addr. Handle received frame right
 * now. */
    TRACE_MSG(TRACE_MAC3, "indirect. My addr 0x%x / " TRACE_FORMAT_64,
              (FMT__D_A, MAC_PIB().mac_short_address, TRACE_ARG_64(MAC_PIB().mac_extended_address)));
    /*cstat !MISRAC2012-Rule-14.3_a */
    /** @mdr{00004,0} */
#ifdef ZB_RAF_INDIRECT_DATA_RX_FIX
    if (zb_mac_check_frame_unicast_to_myself(&mhr))
#else
    if (zb_mac_check_frame_dst_addr(&mhr) && zb_mac_check_frame_pan_id(&mhr))
#endif
    {
      /* data is received indirectly */
      /* cancel indirect data transfer timeout */
      ZB_SCHEDULE_ALARM_CANCEL(zb_mac_indirect_data_rx_failed, ZB_ALARM_ANY_PARAM);
      TRACE_MSG(TRACE_MAC2, "got indirect data, OK", (FMT__0));
      (void)ZB_SCHEDULE_TX_CB(zb_mac_sync_rx_on_off_with_pib, 0);

      /* check for empty frame in answer to poll request: if this one is
       * empty, return NO_DATA, then pass data up. */
      TRACE_MSG(TRACE_MAC3, "buf len %hd mhr %hd tail %hd - data size %hd",
                (FMT__H_H_H_H, zb_buf_len(buf), mhr.mhr_len, ZB_TAIL_SIZE_FOR_RECEIVED_MAC_FRAME,
                 zb_buf_len(buf) - (mhr.mhr_len + ZB_TAIL_SIZE_FOR_RECEIVED_MAC_FRAME)));
      if (zb_buf_len(buf) == mhr.mhr_len + (unsigned)ZB_TAIL_SIZE_FOR_RECEIVED_MAC_FRAME)
      {
        mac_status = MAC_NO_DATA;
        zb_buf_free(buf);  /* Drop empty frame */
        buf = 0;
        TRACE_MSG(TRACE_MAC2, "Empty frame - clear pending data bit", (FMT__0));
      }
      else
      {
        mac_status = MAC_SUCCESS;
        TRACE_MSG(TRACE_MAC2, "pass data up", (FMT__0));
      }
      /* received data frame (either empty or with data), so clear pending data flag - can go asleep */
      MAC_CTX().flags.in_pending_data = ZB_FALSE;
#ifdef ZB_ED_FUNC
      if (mac_status != MAC_SUCCESS
          || MAC_CTX().flags.poll_inprogress)
      {
        /* Note: zb_mac_indirect_data_rx_failed can be used for poll in success
         * case also */
        zb_mac_indirect_data_rx_failed((zb_uint8_t)mac_status);
      }
#else
      ZVUNUSED(mac_status);
#endif  /* ZB_ED_FUNC */
    }
  } /* if indirect */
#endif  /* !ZB_COORDINATOR_ONLY */

  /* SCAN RX filter */
  if (buf != 0U &&
      MAC_CTX().flags.mlme_scan_in_progress)
  {
    zb_bool_t drop_frame = ZB_FALSE;

    switch (MAC_CTX().scan_type)
    {
      case ED_SCAN:
        drop_frame = ZB_TRUE;
        break;

      case ENHANCED_ACTIVE_SCAN:
        /* FALL THRU */
      case ACTIVE_SCAN:
        /* FALL THRU */
      case PASSIVE_SCAN:
        if (ZB_FCF_GET_FRAME_TYPE(mhr.frame_control) != MAC_FRAME_BEACON)
        {
          drop_frame = ZB_TRUE;
        }
        break;

#ifndef ZB_LITE_NO_ORPHAN_SCAN
      case ORPHAN_SCAN:
        if (ZB_FCF_GET_FRAME_TYPE(mhr.frame_control) != MAC_FRAME_COMMAND)
        {
          /* This filter doesn't analyse Command Identifier field, so
           * potentially we WILL accept any command frame during
           * Orphan scan, but the probability of this event is very low.
           *
           * The best way is to analyse Command Identifier as well,
           * but the probability of this event is very low.
           */
          drop_frame = ZB_TRUE;
        }
        break;
#endif  /* #ifndef ZB_LITE_NO_ORPHAN_SCAN */
      default:
        TRACE_MSG(TRACE_ERROR, "Invalid scan type: %hd", (FMT__H, MAC_CTX().scan_type));
        ZB_ASSERT(ZB_FALSE);
        break;
    }

    if (drop_frame)
    {
      TRACE_MSG(TRACE_MAC2, "Unexpected frame during scan - drop it", (FMT__0));
      zb_buf_free(buf);  /* Drop empty frame */
      buf = 0;
    }
  }

  /* Finally analyze data itself */
#if defined ZB_ENHANCED_BEACON_SUPPORT
  if (buf != 0U && ZB_FCF_GET_FRAME_VERSION(mhr.frame_control) == MAC_FRAME_IEEE_802_15_4_2015)
  {
    ZB_SCHEDULE_CALLBACK(mac_handle_802154_ver2_frame, buf);
  }
  else
#endif /* ZB_ENHANCED_BEACON_SUPPORT */
    if (buf != 0U)
    {
      switch (ZB_FCF_GET_FRAME_TYPE(mhr.frame_control))
      {
        case MAC_FRAME_DATA:
        {
          /* GPDF. Maybe, Rx after TX is set, so need to send data from TX
           * q. Do it from here to exclude unpredictable delays.
           */
          if (ZB_NWK_FRAMECTL_GET_PROTOCOL_VERSION(cmd_ptr) == ZB_ZGP_PROTOCOL_VERSION
              /*cstat -MISRAC2012-Rule-13.5 -MISRAC2012-Rule-2.1_b */
              /* After some investigation, the following violation of Rule 13.5 seems to be
               * a false positive. There are no side effect to 'zb_buf_memory_low()'. This
               * violation seems to be caused by the fact that 'zb_buf_memory_low()' is an
               * external function, which cannot be analyzed by C-STAT. */
              && (ZB_MAC_GET_SKIP_GPDF()
                  || zb_buf_memory_low()))
          {
            /*cstat +MISRAC2012-Rule-13.5 +MISRAC2012-Rule-2.1_b */
            TRACE_MSG(TRACE_MAC3, "drop zgp frame", (FMT__0));
            zb_buf_free(buf);
            ZB_MAC_DIAGNOSTICS_VALIDATE_DROP_CNT_INC();
          }
          else
          {
            TRACE_MSG(TRACE_MAC3, "data frame - schedule zb_handle_data_frame", (FMT__0));
            ZB_SCHEDULE_CALLBACK(zb_handle_data_frame, buf);
          }
          break;
        }

        case MAC_FRAME_BEACON:
        {
          zb_pan_descriptor_t pan_descriptor;
          zb_ushort_t payload_len;
          zb_ushort_t payload_off = zb_mac_get_beacon_payload_offset(ZB_MAC_GET_FCF_PTR(zb_buf_begin(buf)));
          zb_mac_beacon_notify_indication_t *ind;
          zb_int8_t rssi = ZB_MAC_GET_RSSI(buf);

#ifndef ZB_MAC_TESTING_MODE
          /* check the buffer length is correct */
          if ((zb_buf_len(buf) < (payload_off + ZB_BEACON_PAYLOAD_TAIL + ZB_BEACON_PAYLOAD_LENGTH_MIN))
#ifdef ZB_RAF_INVALID_BEACON_PREVENT
              || (zb_buf_len(buf) != ZB_VALID_BEACON_LENGTH)
#endif
              )
          {
            TRACE_MSG(TRACE_ERROR, "Oops - error beacon payload length - drop it", (FMT__0));
            zb_buf_free(buf);
            ZB_MAC_DIAGNOSTICS_VALIDATE_DROP_CNT_INC();
            break;
          }
#endif  /* ZB_MAC_TESTING_MODE */

          zb_mac_create_pan_desc_from_beacon(buf, &mhr, NULL, &pan_descriptor);

          /* Tricky: Store beacon payload in the buffer. Cut MAC header and MAC
           * tail */
          (void)zb_buf_cut_left(buf, payload_off);
          zb_buf_cut_right(buf, ZB_BEACON_PAYLOAD_TAIL);

          payload_len = zb_buf_len(buf);
          /* reuse buf for beacon indication to nwk */
          ind = zb_buf_alloc_left(buf, sizeof(*ind) - sizeof(ind->sdu));
          ind->bsn = mhr.seq_number;
          ind->beacon_type = ZB_MAC_BEACON_TYPE_BEACON;
          ZB_MEMCPY(&ind->pan_descriptor, &pan_descriptor, sizeof(pan_descriptor));
          ZB_BZERO(&ind->pend_addr_spec, sizeof(ind->pend_addr_spec) + sizeof(ind->addr_list));
          ind->sdu_length = (zb_uint8_t)payload_len;
          ind->rssi = rssi;

          TRACE_MSG(TRACE_MAC3, "BEACON frame payload len %hd", (FMT__H, payload_len));
          /* Payload is already in the buffer. */

#ifdef ZB_MAC_TESTING_MODE
          {
            TRACE_MSG(TRACE_NWK1, "payload len %hd, auto_req %hd", (FMT__H_H, payload_len, MAC_PIB().mac_auto_request));

            /* 7.1.11.1 MLME-SCAN.request
             * if macAutoRequest == TRUE, MLME-SCAN.confirm primitive
             * will contain PAN descriptor list; otherwise, MLME-BEACON-NOTIFY is
             * called only if not-empty beacon payload */
            if (MAC_CTX().flags.mlme_scan_in_progress &&
                MAC_PIB().mac_auto_request)
            {
              zb_mac_store_pan_desc(buf, &pan_descriptor);
            }

            /* if mac auto request == FALSE or beacon payload length is not 0, call beacon notify indication */
            if (!MAC_PIB().mac_auto_request || payload_len)
            {
#if defined ZB_MAC_API_TRACE_PRIMITIVES
              zb_mac_api_trace_beacon_notify_indication(buf);
#endif  /* ZB_MAC_API_TRACE_PRIMITIVES */
              ZB_SCHEDULE_CALLBACK(zb_mlme_beacon_notify_indication, buf);
            }
            else
            {
              zb_buf_free(buf);
            }
          }
#else /* ZB_MAC_TESTING_MODE */

#if defined ZB_MAC_API_TRACE_PRIMITIVES
          zb_mac_api_trace_beacon_notify_indication(buf);
#endif  /* ZB_MAC_API_TRACE_PRIMITIVES */
          ZB_SCHEDULE_CALLBACK(zb_mlme_beacon_notify_indication, buf);
#endif /* ZB_MAC_TESTING_MODE */

          /* Remember that at least 1 beacon came during active scan, so active
           * scan will have success status */
          if (MAC_CTX().flags.mlme_scan_in_progress)
          {
            MAC_CTX().flags.active_scan_beacon_found = ZB_TRUE;
            TRACE_MSG(TRACE_MAC3, "some beacon found!", (FMT__0));
          }
          break;
        }

        case MAC_FRAME_COMMAND:
          /* If waiting for ACK, ignore incoming commands */
          if (*cmd_ptr == MAC_CMD_DATA_REQUEST)
          {
            zb_mlme_handle_data_req_command(buf);
          }
          else if(((ZB_FCF_GET_SRC_ADDRESSING_MODE(mhr.frame_control) == ZB_ADDR_64BIT_DEV) ?
             ZB_IEEE_ADDR_IS_VALID(mhr.src_addr.addr_long):ZB_TRUE))
          {
            TRACE_MSG(TRACE_MAC3, "schedule cmd handle", (FMT__0));
            ZB_SCHEDULE_CALLBACK(zb_mlme_handle_in_command, buf);
          }
          else
          {
            TRACE_MSG(TRACE_MAC3, "MAC command while waiting for ACK - drop it", (FMT__0));
            zb_buf_free(buf);
            ZB_MAC_DIAGNOSTICS_VALIDATE_DROP_CNT_INC();
          }
          break;

        default:
#ifdef PHY_RX_TEST_CMW			
          ZB_SCHEDULE_CALLBACK(zb_handle_data_frame, buf);
#else
          TRACE_MSG(TRACE_ERROR, "Oops - unknown frame type %hd - drop it", (FMT__H, ZB_FCF_GET_FRAME_TYPE(mhr.frame_control)));
          zb_buf_free(buf);
#endif
          ZB_MAC_DIAGNOSTICS_VALIDATE_DROP_CNT_INC();
          break;
      } /*switch (ZB_FCF_GET_FRAME_TYPE(fcf))*/
    }   /* if parse_continue */
  TRACE_MSG(TRACE_MAC3, "<<zb_mac_recv_data", (FMT__0));
}

#if defined ZB_ROUTER_ROLE && defined ZB_JOINING_LIST_SUPPORT

/* Returns true if the given address is in mibJoiningIeeeList */
static zb_bool_t zb_mac_check_joining_ieee_list(zb_ieee_addr_t address)
{
  zb_uint8_t i;
  zb_bool_t found = ZB_FALSE;

  for (i = 0; i < MAC_PIB().mac_ieee_joining_list.length && !found; i++)
  {
    if (ZB_64BIT_ADDR_CMP(address, MAC_PIB().mac_ieee_joining_list.items[i]))
    {
      found = ZB_TRUE;
      break;
    }
  }

  return found;
}

#endif /* ZB_ROUTER_ROLE && ZB_JOINING_LIST_SUPPORT */

#if defined ZB_ENHANCED_BEACON_SUPPORT

static zb_bool_t mac_handle_enhanced_beacon(zb_bufid_t buf, zb_mac_mhr_t *mhr)
{
  zb_uint8_t *ptr = zb_buf_begin(buf);
  zb_uint_t i = 0;
  zb_uint_t len = zb_buf_len(buf);
  zb_bool_t graceful_stop = ZB_FALSE, ret = ZB_TRUE;
  zb_pie_header_t pie_hdr;
  zb_zigbee_vendor_pie_parsed_t vendor_parsed;
  zb_pan_descriptor_t pan_descriptor;
  zb_mac_beacon_notify_indication_t *ind;
  zb_int8_t rssi = ZB_MAC_GET_RSSI(buf);

  TRACE_MSG(TRACE_MAC3, ">>mac_handle_enhanced_beacon %hd", (FMT__H, buf));

  ZB_MEMSET(&vendor_parsed, 0, sizeof(vendor_parsed));

  /* HT1 is expected */
  if (!ZB_CHECK_IS_HT1(ptr))
  {
    TRACE_MSG(TRACE_MAC3, "HT1 is expected here!", (FMT__0));
    ret = ZB_FALSE;
  }

  if (ret)
  {
    i = ZB_HIE_HEADER_LENGTH; /* skipping HT1 header*/

    while (len - i >= ZB_PIE_HEADER_LENGTH)
    {
      if (ZB_IE_HEADER_GET_TYPE(&ptr[i]) != ZB_IE_HEADER_TYPE_PIE)
      {
        /* malformed packet */
        break;
      }

      ZB_GET_PIE_HEADER(&ptr[i], &pie_hdr);
      i += ZB_PIE_HEADER_LENGTH;
      if (i + pie_hdr.length > len)
      {
        /* buffer overflow */
        break;
      }

      switch (pie_hdr.group_id)
      {
        case ZB_PIE_GROUP_VENDOR_SPECIFIC:
          ret = zb_parse_zigbee_vendor_pie(&ptr[i], pie_hdr.length, &vendor_parsed);
          break;

        case ZB_PIE_GROUP_TERMINATION:
          graceful_stop = ZB_TRUE;
          break;

        default:
          TRACE_MSG(TRACE_MAC3, "unexpected group_id: %x", (FMT__D, pie_hdr.group_id));
          break;
      }

      if (graceful_stop)
      {
        break;
      }

      i += pie_hdr.length;
    } /* while */

    if (!graceful_stop && i != len)
    {
      /* normally we would reach the end or termination */
      TRACE_MSG(TRACE_MAC3, "IE parsing failed", (FMT__0));
      ret = ZB_FALSE;
    }
  }

  if (ret && !vendor_parsed.eb_payload_set)
  {
    TRACE_MSG(TRACE_MAC3, "no EB payload IE", (FMT__0));
    ret = ZB_FALSE;
  }

#if defined ZB_MAC_POWER_CONTROL
  if (ret && vendor_parsed.tx_power_set)
  {
    if (ZB_FCF_GET_SRC_ADDRESSING_MODE(mhr->frame_control) == ZB_ADDR_64BIT_DEV)
    {
      zb_mac_power_ctrl_update_ent(mhr->src_addr.addr_long,
                                   vendor_parsed.eb_payload.source_short_addr,
                                   vendor_parsed.tx_power.tx_power_value, rssi, 1);
    }
    else
    {
      TRACE_MSG(TRACE_ERROR, "Short src addresses are not supported for EB", (FMT__0));
    }
  }
#endif  /* ZB_MAC_POWER_CONTROL */

  if (ret)
  {
    zb_mac_create_pan_desc_from_beacon(buf, mhr, &vendor_parsed, &pan_descriptor);

    /* currently 'i' points to the first byte that is not needed */
    /* cut the buffer so that only PIEs remain */
    zb_buf_cut_right(buf, zb_buf_len(buf) - i);
    (void)zb_buf_cut_left(buf, ZB_HIE_HEADER_LENGTH);

    /* 7.1.11.1 MLME-SCAN.request
     * if macAutoRequest == TRUE, MLME-SCAN.confirm primitive
     * will contain PAN descriptor list; otherwise, MLME-BEACON-NOTIFY is
     * called only if not-empty beacon payload */
    if (MAC_CTX().flags.mlme_scan_in_progress &&
        MAC_PIB().mac_auto_request != 0U)
    {
      /* Todo: implement this case if needed */
      ZB_ASSERT(ZB_FALSE);
    }

    ind = zb_buf_alloc_left(buf, sizeof(*ind) - sizeof(ind->sdu));
    ZB_BZERO(ind, sizeof(*ind) - sizeof(ind->sdu));
    ind->beacon_type = ZB_MAC_BEACON_TYPE_ENHANCED_BEACON;
    ind->bsn = mhr->seq_number;
    ind->total_hie_size = 0;
    ind->total_pie_size = (zb_uint8_t)(zb_buf_len(buf) - sizeof(*ind) + sizeof(ind->sdu));
    ZB_MEMCPY(&ind->pan_descriptor, &pan_descriptor, sizeof(pan_descriptor));
    ind->sdu_length = ind->total_hie_size + ind->total_pie_size;
    ind->rssi = rssi;

    ZB_SCHEDULE_CALLBACK(zb_mlme_beacon_notify_indication, buf);

#if defined ZB_MAC_API_TRACE_PRIMITIVES
    zb_mac_api_trace_beacon_notify_indication(buf);
#endif  /* ZB_MAC_API_TRACE_PRIMITIVES */

    if (MAC_CTX().flags.mlme_scan_in_progress)
    {
      MAC_CTX().flags.active_scan_beacon_found = ZB_TRUE;
      TRACE_MSG(TRACE_MAC3, "some enhanced beacon found!", (FMT__0));
    }
  }
  else
  {
    /*
    zb_buf_free(buf);
    */
  }

  TRACE_MSG(TRACE_MAC3, "<< mac_handle_enhanced_beacon (ret %d)", (FMT__D, ret));
  return ret;
}

#if defined ZB_ROUTER_ROLE

/* Parsing a received enhanced beacon request */
static zb_bool_t mac_handle_enhanced_beacon_request(zb_bufid_t buf, zb_mac_mhr_t *mhr)
{
  zb_uint8_t *ptr = zb_buf_begin(buf);
  zb_uint_t len = zb_buf_len(buf);
  zb_uint_t i;
  /* FIXME: keep warning about 'res' until we decide what to do with it! */
  zb_bool_t graceful_stop = ZB_FALSE, ret = ZB_TRUE;
  zb_pie_header_t pie_hdr;
  zb_mlme_pie_parsed_t mlme_parsed;
  zb_zigbee_vendor_pie_parsed_t vendor_parsed;
#ifdef ZB_SEND_BEACON_AFTER_RANDOM_DELAY
  zb_time_t dummy;
#endif

  TRACE_MSG(TRACE_MAC3, ">> mac_handle_enhanced_beacon_request", (FMT__0));

  /* Here we should be guaranteed to meet HT1 HIE */
  if (!ZB_CHECK_IS_HT1(ptr))
  {
    /* just in case */
    TRACE_MSG(TRACE_MAC3, "HT1 is expected!", (FMT__0));
    ret = ZB_FALSE;
  }

  if (ret && ZB_FCF_GET_SRC_ADDRESSING_MODE(mhr->frame_control) != ZB_ADDR_64BIT_DEV)
  {
    TRACE_MSG(TRACE_MAC3, "long src addr is expected!", (FMT__0));
    ret = ZB_FALSE;
  }

  if (ret)
  {
    ZB_MEMSET(&mlme_parsed, 0, sizeof(mlme_parsed));
    ZB_MEMSET(&vendor_parsed, 0, sizeof(vendor_parsed));

    i = ZB_HIE_HEADER_LENGTH; /* skipping HT1 header */

    while (len - i >= ZB_PIE_HEADER_LENGTH && !graceful_stop)
    {
      if (ZB_IE_HEADER_GET_TYPE(&ptr[i]) != ZB_IE_HEADER_TYPE_PIE)
      {
        /* malformed packet */
        break;
      }

      ZB_GET_PIE_HEADER(&ptr[i], &pie_hdr);
      i += ZB_PIE_HEADER_LENGTH;
      if (i + pie_hdr.length > len)
      {
        /* buffer overflow */
        break;
      }

      switch (pie_hdr.group_id)
      {
        case ZB_PIE_GROUP_MLME:
          (void)zb_parse_mlme_pie(&ptr[i], pie_hdr.length, &mlme_parsed);
          break;

        case ZB_PIE_GROUP_VENDOR_SPECIFIC:
          (void)zb_parse_zigbee_vendor_pie(&ptr[i], pie_hdr.length, &vendor_parsed);
          break;

        case ZB_PIE_GROUP_TERMINATION:
          graceful_stop = ZB_TRUE;
          break;

        default:
          TRACE_MSG(TRACE_MAC3, "unexpected group_id: %x", (FMT__D, pie_hdr.group_id));
          break;
      }

      /* do something if res is false? */
      i += pie_hdr.length;
    }

    if (!graceful_stop && i != len)
    {
      /* normally we would reach the end or termination */
      TRACE_MSG(TRACE_MAC3, "IE parsing failed", (FMT__0));
      ret = ZB_FALSE;
    }
  }

  if (ret)
  {
#if defined ZB_MAC_POWER_CONTROL
    /* handle filtering */
    if (vendor_parsed.tx_power_set)
    {
      zb_int8_t rssi = ZB_MAC_GET_RSSI(buf);

      /* Enhanced beacon request has only IEEE addresses*/
      if (ZB_FCF_GET_SRC_ADDRESSING_MODE(mhr->frame_control) == ZB_ADDR_64BIT_DEV)
      {
        zb_mac_power_ctrl_update_ent_by_ieee(mhr->src_addr.addr_long,
                                             vendor_parsed.tx_power.tx_power_value, rssi, 1);
      }
    }
#endif  /* ZB_MAC_POWER_CONTROL */

    if (vendor_parsed.rejoin_desc_set)
    {
      /* compare extended pan id */
      TRACE_MSG(TRACE_MAC3, "extended pan id in EBR: " TRACE_FORMAT_64,
                (FMT__A, TRACE_ARG_64(vendor_parsed.rejoin_desc.extended_pan_id)));
      /* Tricky moment: let's get our extpanid from the beacon payload. It must be set already. */
      TRACE_MSG(TRACE_MAC3, "my extended pan id: " TRACE_FORMAT_64,
                (FMT__A, TRACE_ARG_64(ZB_PIB_BEACON_PAYLOAD().extended_panid)));

      ret = ZB_64BIT_ADDR_CMP(vendor_parsed.rejoin_desc.extended_pan_id, ZB_PIB_BEACON_PAYLOAD().extended_panid);
      TRACE_MSG(TRACE_MAC3, "compared: %d ",
                (FMT__D, ret));
    }
    else if (mlme_parsed.eb_filter_set)
    {
      if (ZB_EB_FILTER_HAS_FLAG(mlme_parsed.eb_filter.mask, ZB_EB_FILTER_IE_PERMIT_JOINING_ON))
      {
        /* respond only when permit joining on */
        if (MAC_PIB().mac_association_permit == 0U)
        {
          TRACE_MSG(TRACE_MAC3, "association disabled, skipping", (FMT__0));
          ret = ZB_FALSE;
        }
      }

#if defined ZB_JOINING_LIST_SUPPORT
      switch (MAC_PIB().mac_joining_policy)
      {
        case ZB_MAC_JOINING_POLICY_NO_JOIN:
          TRACE_MSG(TRACE_MAC3, "NO_JOIN policy", (FMT__0));
          ret = ZB_FALSE;
          break;

        case ZB_MAC_JOINING_POLICY_IEEELIST_JOIN:
          ret = zb_mac_check_joining_ieee_list(mhr->src_addr.addr_long);
          TRACE_MSG(TRACE_MAC3, "IEEELIST_JOIN policy: result %d", (FMT__D, ret));
          break;

        case ZB_MAC_JOINING_POLICY_ALL_JOIN:
          TRACE_MSG(TRACE_MAC3, "ALL_JOIN policy", (FMT__0));
          break;
      default:
          ZB_ASSERT(0);
          TRACE_MSG(TRACE_ERROR, "Unknown JOIN policy %hd", (FMT__H, MAC_PIB().mac_joining_policy));
          break;
      }
#endif /* ZB_JOINING_LIST_SUPPORT */

    }
    else
    {
      ret = ZB_FALSE;
      TRACE_MSG(TRACE_MAC3, "neither EB payload IE nor Rejoin IE were not found", (FMT__0));
    }
  }

  if (ret)
  {
#ifndef ZB_SEND_BEACON_AFTER_RANDOM_DELAY
    if (ZB_SCHEDULE_TX_CB(zb_handle_enhanced_beacon_req, 0) != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "Oops! No place for zb_handle_enhanced_beacon_req in tx queue", (FMT__0));
    }
#else
    /*
      To prevent sending beacons by all devices at once, so some will be missed,
      send beacon after random delay (0 to 3 beacon intervals)
    */
    /* Start alarm only if not already running */
    if (zb_schedule_get_alarm_time(zb_handle_enhanced_beacon_req_alarm, 0, &dummy) == RET_NOT_FOUND)
    {
      ZB_SCHEDULE_ALARM(zb_handle_enhanced_beacon_req_alarm,
                        0,
                        ZB_RANDOM_VALUE(ZB_MAC_HANDLE_BEACON_REQ_HI_TMO) + ZB_MAC_HANDLE_BEACON_REQ_LOW_TMO);
    }
#endif
  }
  else
  {
    TRACE_MSG(TRACE_MAC3, "EB is not to be sent", (FMT__0));
  }

  zb_buf_free(buf);

  TRACE_MSG(TRACE_MAC3, "<< mac_handle_enhanced_beacon_request (ret %d)", (FMT__D, ret));

  return ret;
}

#endif /* ZB_ROUTER_ROLE */

/*
 * Handle incoming 802.15.4-2015 packet
 */
static void mac_handle_802154_ver2_frame(zb_uint8_t param)
{
  zb_bool_t res, ret = ZB_TRUE;
  zb_mac_mhr_t mhr;
  zb_uint8_t *ptr;
  zb_uint32_t skipped_bytes, packet_len;

  TRACE_MSG(TRACE_MAC2, "+mac_handle_802154_ver2_frame", (FMT__0));

  ptr = zb_buf_cut_left(param, zb_parse_mhr(&mhr, param));

  /*
   * Currently only EB and EBR frames are supported.
   * They both require IEs to be present and both do not need to store
   * information in HIEs
   */

  if (ZB_FCF_GET_IE_LIST_PRESENT_BIT(mhr.frame_control) == 0U)
  {
    TRACE_MSG(TRACE_MAC2, "IEs must be present", (FMT__0));
    ret = ZB_FALSE;
  }

  if (ret)
  {
    packet_len = zb_buf_len(param);
    /* Skipping HIEs as they are not handled anyway */
    res = ZB_SKIP_HIE_TILL_HT(ptr, packet_len, &skipped_bytes);
    if (!res || skipped_bytes == packet_len)
    {
      TRACE_MSG(TRACE_MAC2, "<< HT IE not found, returning", (FMT__0));
      /* Either malformed packet or has no PIEs (yet must have them) */
      ret = ZB_FALSE;
    }
  }

  if (ret)
  {
    ptr = zb_buf_cut_left(param, skipped_bytes);

    switch (ZB_FCF_GET_FRAME_TYPE(mhr.frame_control))
    {
      case MAC_FRAME_COMMAND:
        /* Skip remaining IEs in order to reach frame payload */
        res = ZB_SKIP_ALL_IE(ptr, zb_buf_len(param), &skipped_bytes);
        if (!res)
        {
          TRACE_MSG(TRACE_MAC2, "<< malformed packet, returning", (FMT__0));
          /* malformed packet */
          ret = ZB_FALSE;
          break;
        }

        TRACE_MSG(TRACE_MAC2, "type: cmd %d", (FMT__D, *ptr));
        /* command id field */
        switch(*(ptr + skipped_bytes))
        {
#ifdef ZB_ROUTER_ROLE
          case MAC_CMD_BEACON_REQUEST:
            (void)mac_handle_enhanced_beacon_request(param, &mhr);
            break;
#endif
          default:
            /* No other packets supported */
            ret = ZB_FALSE;
            TRACE_MSG(TRACE_MAC2, "unsupported command id: %x", (FMT__H, *(ptr + skipped_bytes)));
            break;
        }
        break;

      case MAC_FRAME_BEACON:
        TRACE_MSG(TRACE_MAC2, "type: beacon frame - 0x%hx", (FMT__H, *ptr));

        /* transfer parsing to other functions? */
        res = mac_handle_enhanced_beacon(param, &mhr);
        if (!res)
        {
          TRACE_MSG(TRACE_MAC2, "enhanced beacon handling encountered a problem", (FMT__0));
          ret = ZB_FALSE;
        }
        break;

      default:
        TRACE_MSG(TRACE_MAC2, "unsupported frame type: %d", (FMT__D, ZB_FCF_GET_FRAME_TYPE(mhr.frame_control)));
        ret = ZB_FALSE;
        break;
    }
  }


  if (!ret)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_MAC2, "<<mac_handle_802154_ver2_frame ret", (FMT__0));
}

#endif /* ZB_ENHANCED_BEACON_SUPPORT */

/**
   Dups check

*/
zb_bool_t mac_is_dup(zb_mac_mhr_t *mhr, zb_uint8_t len)
{
  zb_bool_t ret = ZB_FALSE;
  zb_time_t min_expire_time = MAC_CTX().dups[0].expire;
  zb_uint_t i, free_i = 0;

  /* do not treate acks as dups */
  if (ZB_FCF_GET_FRAME_TYPE(mhr->frame_control) != MAC_FRAME_ACKNOWLEDGMENT
      && ZB_FCF_GET_FRAME_TYPE(mhr->frame_control) != MAC_FRAME_BEACON)
  {
    for (i = 0 ; i < MAC_N_DUPS && !ret ; ++i)
    {
      if (ZB_MEMCMP(mhr, &MAC_CTX().dups[i].mhr, sizeof(*mhr)) == 0
          && MAC_CTX().dups[i].len == len)
      {
        if (ZB_TIME_GE(MAC_CTX().dups[i].expire, ZB_TIMER_GET()))
        {
          ret = ZB_TRUE;
          TRACE_MSG(TRACE_MACLL3, "i %hd dup expire t %d t %d - Dup",
                    (FMT__H_D_D, i, MAC_CTX().dups[i].expire, ZB_TIMER_GET()));
          free_i = ZB_ARRAY_SIZE(MAC_CTX().dups);
        }
        else
        {
          TRACE_MSG(TRACE_MACLL3, "i %hd dup expire t %d t %d - NOT a dup",
                    (FMT__H_D_D, i, MAC_CTX().dups[i].expire, ZB_TIMER_GET()));
          free_i = i;
        }
        break;
      }
      else if (ZB_TIME_GE(min_expire_time, MAC_CTX().dups[i].expire)
               && min_expire_time != MAC_CTX().dups[i].expire)
      {
        min_expire_time = MAC_CTX().dups[i].expire;
        free_i = i;
      }
      else
      {
        /* MISRA rule 15.7 requires empty 'else' branch. */
      }
    } /* for */

    if (free_i < ZB_ARRAY_SIZE(MAC_CTX().dups))
    {
      zb_time_t expire_timeout;

      ZB_MEMCPY(&MAC_CTX().dups[free_i].mhr, mhr, sizeof(*mhr));
      MAC_CTX().dups[free_i].len = len;

      /* [MM] Hacky hack for R21 certification:
       *
       * expire_timeout should be based on polling interval, which is
       * the part of ZDO_CTX(). But it's not allowed to access ZDO
       * layer in MAC to avoid MAC -> ZDO dependecy. As a hotfix,
       * let's assume 5.5 seconds should be enough.
       *
       * Also please note incremented size of MAC_CTX().dups array:
       * taking in account 5.5 seconds timeout and, as usual 2
       * seconds poll rate, 5.5/2 = 2.75, so increasing it in 3
       * (three) times.
       *
       * Correct solution: create custom attribute in MAC PIB and use
       * MLME-SET.request() primitive. Call MLME-SET.request() from
       * ZDO layer.
       */
      #define HACKY_HACK_MAC_DUPS_TIMEOUT_FOR_SLEEPY_ZED \
        (ZB_MILLISECONDS_TO_BEACON_INTERVAL(5500U))

      expire_timeout = ZB_U2B(ZB_PIB_RX_ON_WHEN_IDLE())
                           ? (ZB_MAC_PIB_ACK_WAIT_DURATION * ZB_MAC_PIB_MAX_FRAME_RETRIES)
                           : (HACKY_HACK_MAC_DUPS_TIMEOUT_FOR_SLEEPY_ZED);

      MAC_CTX().dups[free_i].expire = ZB_TIME_ADD(ZB_TIMER_GET(), expire_timeout);
      TRACE_MSG(TRACE_MACLL3, "Not a dup. i %hd expire at %d",
                (FMT__H_D, free_i, MAC_CTX().dups[free_i].expire));
    }
  }
  TRACE_MSG(TRACE_MACLL3, "mac_is_dup ret %hd", (FMT__H, (zb_uint8_t)ret));
  return ret;
}


zb_bool_t mac_can_accept_frame(zb_bufid_t buf)
{
  zb_bool_t ret;

  if (zb_buf_len(buf) == 0U)
  {
    TRACE_MSG(TRACE_MAC1, "buf %hd len %d - bad buffer", (FMT__H_D, buf, zb_buf_len(buf)));
    ret = ZB_FALSE;
  }
  else
  {
    zb_mac_mhr_t mhr;
    zb_uint8_t *cmd_ptr = zb_buf_begin(buf);
    zb_uint_t mhr_len = zb_parse_mhr_ptr(&mhr, cmd_ptr);
    if (mhr_len + (unsigned)ZB_TAIL_SIZE_FOR_RECEIVED_MAC_FRAME > zb_buf_len(buf))
    {
      TRACE_MSG(TRACE_MAC1, "buf %hd len %d mhr_len %d - bad buffer", (FMT__H_H_H, buf, zb_buf_len(buf), mhr_len));
      ret = ZB_FALSE;
    }
    else
    {
      cmd_ptr += mhr_len;
      ret = (zb_bool_t)(can_accept_frame(&mhr)
          /* Drop incoming GPDFS if low on memory */
          /* 07/08/2020 EE CR:MINOR Why can't you use definition from ZB_nwk.h? */

          /*cstat -MISRAC2012-Rule-13.5 */
          /* After some investigation, the following violation of Rule 13.5 seems to be
           * a false positive. There are no side effects to 'zb_buf_memory_low()'. This
           * violation seems to be caused by the fact that 'zb_buf_memory_low()' is an
           * external function, which cannot be analyzed by C-STAT. */
          && !(ZB_NWK_FRAMECTL_GET_PROTOCOL_VERSION(cmd_ptr) == ZB_ZGP_PROTOCOL_VERSION
               && zb_buf_memory_low()));
          /*cstat +MISRAC2012-Rule-13.5 */
    }
  }
  TRACE_MSG(TRACE_MAC1, "buf %hd len %d mac_can_accept_frame ret %d", (FMT__H_D_D, buf, zb_buf_len(buf), ret));
  return ret;
}


/**
   Check frame visibility and brodacasts from sleeping ED

   @param param - buffer for receive data
   @return ZB_TRUE, ZB_FALSE
*/
static zb_bool_t can_accept_frame(const zb_mac_mhr_t *mhr)
{
  zb_bool_t ret = ZB_FALSE;

#if (defined ZB_MAC_TESTING_MODE || defined ZB_MAC_DROP_PKT_WHEN_RX_OFF)
  /* Manually check transceiver ability to receive a frame, if it's not
   * supported in HW */

  /* Also this flushes packets that have been received between
   * the ZB_TRANSCEIVER_SET_RX_ON_OFF(0) and the last mac_logic_iteration execution before it
   * Not flushing these packets causes a race condition (at least) during association
   *
   * TODO: these flushed packets are ACK'ed, do we need a more elegant approach?
   * */
  /* 2019-08-06: This looks like some workaround for TI platform. Put it back under
   * ZB_MAC_TESTING_MODE.
   * This check is useful for debug/testing purposes, but not sure why it should be used in
   * production. In general case, if radio gives us the packet, we don't need to check that during
   * this process radio is still in RX state - at least for the platforms with auto-ack.
   *
   * If you have more details on the original issue, please specify. Should be fixed in other way
   * for TI.
   *
   * 2020-12-14: If the stack drops a packet received between ZB_TRANSCEIVER_SET_RX_ON_OFF(0)
   * and mac_logic_iteration() in a fragmented transfer, it can break the fragmented transfer.
   */
  if (!ZB_TRANSCEIVER_GET_RX_ON_OFF())
  {
    TRACE_MSG(TRACE_MAC1, "Receiver is OFF, drop packet", (FMT__0));
  }
  else
#endif /* (defined ZB_MAC_TESTING_MODE || defined ZB_TI_CC13XX) */
#ifdef ZB_LIMIT_VISIBILITY
  if (!mac_is_frame_visible(mhr))
  {
    TRACE_MSG(TRACE_MAC1, "filtered frame dropped", (FMT__0));
  }
  else
#endif  /* ZB_LIMIT_VISIBILITY */
  {
#ifdef ZB_BLOCK_BROADCASTS_SLEEPY_ED
    if (!ZB_U2B(ZB_PIB_RX_ON_WHEN_IDLE()) && mhr->dst_addr.addr_short == 0xffffU)
    {
      TRACE_MSG(TRACE_MAC1, "drop broadcast", (FMT__0));
    }
    else
#endif
    {
#ifndef ZB_MAC_TESTING_MODE
      if (ZB_FCF_GET_SECURITY_BIT(mhr->frame_control) != 0U
          && ZB_FCF_GET_FRAME_VERSION(mhr->frame_control) < MAC_FRAME_IEEE_802_15_4)
      {
        TRACE_MSG(TRACE_MAC1, "unsupported 2003 security frame dropped", (FMT__0));
      }
      else
#endif
      {
        ret = ZB_TRUE;
      }
    }
  }
  if (ret)
  {
    /*cstat !MISRAC2012-Rule-14.3_b */
    /** @mdr{00004,1} */
    if (!(ZB_FCF_GET_FRAME_TYPE(mhr->frame_control) == MAC_FRAME_ACKNOWLEDGMENT
          /*cstat !MISRAC2012-Rule-13.5 */
          /* After some investigation, the following violation of Rule 13.5 seems to be
           * a false positive. There are no side effects to 'zb_mac_check_frame_is_broadcast()'. */
          || ((zb_mac_check_frame_dst_addr(mhr) || zb_mac_check_frame_is_broadcast(mhr))
              && zb_mac_check_frame_pan_id(mhr))))
    {
      ret = ZB_FALSE;
    }
  }
#ifdef ZB_MAC_TESTING_MODE
  /* Drop frame if frame type has one of reserved values */
  if (ZB_FCF_GET_FRAME_TYPE(mhr->frame_control) > MAC_FRAME_COMMAND)
  {
    ret = ZB_FALSE;
  }
#endif

  TRACE_MSG(TRACE_MAC3, "<<can_accept_fr, ret %i", (FMT__D, ret));
  return ret;
}


/**
   Create PAN descriptor to pass to NWK
*/
static void zb_mac_create_pan_desc_from_beacon(zb_bufid_t beacon_buf, const zb_mac_mhr_t *mhr,
                                               zb_zigbee_vendor_pie_parsed_t *vendor_parsed,
                                               zb_pan_descriptor_t *pan_desc)
{
#if !defined ZB_SUBGHZ_BAND_ENABLED
  ZVUNUSED(vendor_parsed);
#endif /* !ZB_SUBGHZ_BAND_ENABLED */

  TRACE_MSG(TRACE_NWK1, ">>zb_mac_create_pan_desc_from_beacon %p", (FMT__P, beacon_buf));

  pan_desc->coord_addr_mode = ZB_FCF_GET_SRC_ADDRESSING_MODE(&mhr->frame_control);
  pan_desc->coord_pan_id = mhr->src_pan_id;
  ZB_MEMCPY(&pan_desc->coord_address, &mhr->src_addr, sizeof(zb_addr_u));
  pan_desc->logical_channel = MAC_PIB().phy_current_channel;
  pan_desc->channel_page = MAC_PIB().phy_current_page;

  /* TODO: LOGICAL channel and PAGE should be handled here. It's not a part of
   * CR. */
#if defined ZB_ENHANCED_BEACON_SUPPORT
  if (vendor_parsed != NULL)
  {
    /* enhanced beacon */
    ZB_MEMCPY(&pan_desc->super_frame_spec, &vendor_parsed->eb_payload.superframe_spec, 2);
    pan_desc->enh_beacon_nwk_addr = vendor_parsed->eb_payload.source_short_addr;
    TRACE_MSG(TRACE_NWK1, "enhanced beacon case", (FMT__0));
  }
  else
#endif /* ZB_ENHANCED_BEACON_SUPPORT */
  {
    ZB_GET_SUPERFRAME(ZB_MAC_GET_FCF_PTR(zb_buf_begin(beacon_buf)),
                      mhr->mhr_len,
                      &pan_desc->super_frame_spec);
  }

  pan_desc->gts_permit = 0; /* use ZB_MAC_GET_GTS_FIELDS() to get exact gts value.
                               Zigbee uses beaconless mode, so gts is not used. */
  pan_desc->link_quality = ZB_MAC_GET_LQI(beacon_buf);

#if defined ZB_MAC_TESTING_MODE
  pan_desc->timestamp = *(ZB_BUF_GET_PARAM(beacon_buf, zb_time_t));
#endif

  TRACE_MSG(TRACE_MAC3, "beacon, lqi %hd rssi %hd",
            (FMT__H_H, pan_desc->link_quality, ZB_MAC_GET_RSSI(beacon_buf)));

  TRACE_MSG(TRACE_NWK1, "<<zb_mac_create_pan_desc_from_beacon", (FMT__0));
}

/**
   MLME.reset-request

   @param param - reset parameter
*/
void zb_mlme_reset_request(zb_uint8_t param)
{
  /* Schedule mlme_reset via tx queue. Reason: transmission can be in progress
   * when we perform MAC reset. Do it via tx queue. */
  if (ZB_SCHEDULE_TX_CB_WITH_HIGH_PRIORITY(zb_mlme_reset_request_sync, param) != RET_OK)
  {
    zb_mlme_reset_confirm_t *reset_conf;

    TRACE_MSG(TRACE_ERROR, "Oops! No place for zb_mlme_reset_request in tx queue", (FMT__0));
    reset_conf = ZB_BUF_GET_PARAM(param, zb_mlme_reset_confirm_t);
    reset_conf->status = MAC_TRANSACTION_OVERFLOW;
    ZB_SCHEDULE_CALLBACK(zb_mlme_reset_confirm, param);
  }
}

static void zb_mlme_reset_request_sync(zb_uint8_t param)
{
  zb_mlme_reset_request_t *reset_req;

#ifdef MAC_RADIO_TX_WATCHDOG_ALARM
  ZB_SCHEDULE_ALARM_CANCEL(mac_tx_watchdog, ZB_ALARM_ANY_PARAM);
#endif

  zb_buf_set_status(param, 0);
  reset_req = ZB_BUF_GET_PARAM(param, zb_mlme_reset_request_t);

#if defined ZB_MAC_API_TRACE_PRIMITIVES
#if defined ZB_MAC_TESTING_MODE
  if (!MAC_CTX().cert_hacks.reset_init_only_radio)
#endif
  {
    zb_mac_api_trace_reset_request(param);
  }
#endif

  MAC_PIB().phy_ephemeral_page = ZB_MAC_INVALID_LOGICAL_PAGE;
  zb_mac_change_channel(ZB_MAC_INVALID_LOGICAL_PAGE,
                        ZB_MAC_INVALID_LOGICAL_CHANNEL);

  if (reset_req->set_default_pib == 1U
#if defined ZB_MAC_TESTING_MODE
      && !MAC_CTX().cert_hacks.reset_init_only_radio
#endif
    )
  {
    TRACE_MSG(TRACE_NWK1, "Reset PIB", (FMT__0));
    zb_mac_reinit_pib();
  }

  ZB_TRANSCEIVER_UPDATE_PAN_ID();
#if defined ZB_MAC_STICKY_PENDING_BIT && defined ZB_ROUTER_ROLE
  ZB_MAC_TRANS_SET_PENDING_BIT();
  MAC_CTX().flags.pending_bit = ZB_TRUE;
#endif

  /* If resetted radio, must not continue scan */
  ZB_SCHEDULE_ALARM_CANCEL(zb_mlme_scan_step, ZB_ALARM_ANY_PARAM);
  MAC_CTX().flags.mlme_scan_in_progress = ZB_FALSE;

#ifdef ZB_MAC_RESET_AT_SHUT
  if (reset_req->reset_at_shut == 0U)
#endif
  {
    zb_mlme_reset_confirm_t *reset_conf;

    if (!ZB_U2B(ZB_PIB_RX_ON_WHEN_IDLE()))
    {
      ZB_TRANSCEIVER_SET_RX_ON_OFF(ZB_PIB_RX_ON_WHEN_IDLE());
    }

    reset_conf = ZB_BUF_GET_PARAM(param, zb_mlme_reset_confirm_t);
    reset_conf->status = MAC_SUCCESS;

#if defined ZB_MAC_API_TRACE_PRIMITIVES
#if defined ZB_MAC_TESTING_MODE
    if (!MAC_CTX().cert_hacks.reset_init_only_radio)
#endif
    {
      zb_mac_api_trace_reset_confirm(param);
    }
#endif

#if defined ZB_MAC_DUTY_CYCLE_MONITORING
    ZB_SCHEDULE_ALARM_CANCEL(zb_mac_duty_cycle_periodic, ZB_ALARM_ANY_PARAM);
    ZB_SCHEDULE_CALLBACK(zb_mac_duty_cycle_periodic, 0);
#endif  /* ZB_MAC_DUTY_CYCLE_MONITORING */

#if defined ZB_MAC_DIAGNOSTICS
    zb_mac_diagnostics_init();
#endif

    ZB_SCHEDULE_CALLBACK(zb_mlme_reset_confirm, param);
  }
#ifdef ZB_MAC_RESET_AT_SHUT
  else
  {
    ZB_TRANSCEIVER_SET_RX_ON_OFF(0);
    zb_buf_free(param);
  }
#endif
}


/**
   Function parses incoming MAC command and executes it
*/
void zb_mlme_handle_in_command(zb_uint8_t param)
{
  zb_uint8_t *cmd_ptr;
  zb_mac_mhr_t mhr;

  TRACE_MSG(TRACE_MAC2, ">>zb_mlme_handle_in_command %hd", (FMT__H, param));

/*
  5.5.3.4 MAC command frame
  | MHR (var length) | Command type 1 byte | Command payload (var length) | FCS 2 bytes |
*/
  cmd_ptr = zb_buf_begin(param);
  cmd_ptr += zb_parse_mhr(&mhr, param);

  TRACE_MSG(TRACE_MAC2, "cmd %hu", (FMT__H,(*cmd_ptr)));
  switch (*cmd_ptr)
  {
#ifdef ZB_ROUTER_ROLE
    case MAC_CMD_BEACON_REQUEST:
    {
      zb_time_t dummy = 0;
      TRACE_MSG(TRACE_MAC2, "IN Beacon request", (FMT__0));
#ifndef ZB_SEND_BEACON_AFTER_RANDOM_DELAY
      ZVUNUSED(dummy);
      if (ZB_SCHEDULE_TX_CB(zb_handle_beacon_req, 0) != RET_OK)
      {
        TRACE_MSG(TRACE_ERROR, "Oops! No place for zb_handle_beacon_req in tx queue", (FMT__0));
      }
#else
      /*
        To prevent sending beacons by all devices at once, so some will be missed,
        send beacon after random delay (0 to 3 beacon intervals)
      */
      /* Start alarm only if not already running */
      if (zb_schedule_get_alarm_time(zb_handle_beacon_req_alarm, 0, &dummy) == RET_NOT_FOUND)
      {
        ZB_SCHEDULE_ALARM(zb_handle_beacon_req_alarm, 0,
                          ZB_RANDOM_VALUE(ZB_MAC_HANDLE_BEACON_REQ_HI_TMO) + ZB_MAC_HANDLE_BEACON_REQ_LOW_TMO);
      }
#endif
      TRACE_MSG(TRACE_MAC3, "free buf %hd", (FMT__H, param));
      zb_buf_free(param);
      break;
    }

    case MAC_CMD_ASSOCIATION_REQUEST:
      ZB_SCHEDULE_CALLBACK(zb_process_ass_request_cmd, param);
      break;

    case MAC_CMD_DATA_REQUEST:   /*
                                   Coordinator side
                                   7.3.4 Data request command
                                   | MHR | cmd id |
                                 */
      TRACE_MSG(TRACE_MAC3, "MAC_CMD_DATA_REQUEST", (FMT__0));
      /* Some platforms can mark in the packet status Pending bit of MAC ACK sent in response to recieved POLL packet.
         If we sent ACK with peding bit 0, do not send data to ZED: it already switched off its radio.
         Still pass POLL into NWK so it can handle ED timeout.
      */

      if (zb_buf_get_status(param) == (zb_ret_t)MAC_NO_DATA
          /* A bit of optimization: if nothing for that device in the
           * indirect queue, pass the poll to NWK immedietly. */

          /*cstat !MISRAC2012-Rule-13.5 */
          /* After some investigation both this and the following violation of the Rule 13.5
           * seem to be false positives. There is no side effect to mac_check_pending_data().
           * This seems to be due to the existance of external functions inside
           * mac_check_pending_data() which cannot be analyzed by C-STAT. 'const' was added
           * to this function parameters. */
          || mac_check_pending_data(&mhr, 0) == -1)
      {
        TRACE_MSG(TRACE_MAC1, "mac status = %d: we sent ACK without Pending bit or no data in indirect q.", (FMT__D, zb_buf_get_status(param)));
        zb_mac_call_mcps_poll_indication(param, &mhr);
      }
      /* else call via tx q to be able to send */
      else
      {
        if (ZB_SCHEDULE_TX_CB_WITH_HIGH_PRIORITY(zb_accept_data_request_cmd, param) != RET_OK)
        {
          TRACE_MSG(TRACE_ERROR, "Oops - MAC tx queue is full", (FMT__0));
          /* Still pass to NWK - we can do it at least */
          zb_mac_call_mcps_poll_indication(param, &mhr);
        }
      }
      break;

#ifndef ZB_LITE_NO_ORPHAN_SCAN
    case MAC_CMD_ORPHAN_NOTIFICATION:
      if (ZB_FCF_GET_SRC_ADDRESSING_MODE(mhr.frame_control) == ZB_ADDR_64BIT_DEV)
      {
        zb_mac_orphan_ind_t *orphan_ind = ZB_BUF_GET_PARAM(param, zb_mac_orphan_ind_t);
        ZB_IEEE_ADDR_COPY(orphan_ind->orphan_addr, mhr.src_addr.addr_long);

#if defined ZB_MAC_API_TRACE_PRIMITIVES
        zb_mac_api_trace_orphan_indication(param);
#endif

        ZB_SCHEDULE_CALLBACK(zb_mlme_orphan_indication, param);
      }
      else
      {
        /* malformed packet */
        TRACE_MSG(TRACE_MAC1, "orph ind should have long src addr", (FMT__0));
        zb_buf_free(param);
      }
      break;
#endif  /* #ifndef ZB_LITE_NO_ORPHAN_SCAN */

#endif  /* ZB_ROUTER_ROLE */

      /* common common for ZR/ZED */
#ifdef ZB_JOIN_CLIENT
      /*
        parse response command and make associate confirm call
        7.3.2 Association response command
        | MHR | command id 1 byte | short addr 2 bytes | status 1 byte |
      */
    case MAC_CMD_ASSOCIATION_RESPONSE:
      if (MAC_CTX().flags.ass_state == ZB_MAC_ASS_STATE_POLLING)
      {
        mac_handle_associate_resp(param, &mhr, cmd_ptr);
      }
      else
      {
        TRACE_MSG(TRACE_ERROR, "got unexpected association resp", (FMT__0));
        zb_buf_free(param);
      }
      break;
#endif  /* ZB_JOIN_CLIENT */

#ifdef ZB_OPTIONAL_MAC_FEATURES
#ifndef ZB_LITE_NO_ORPHAN_SCAN
    case MAC_CMD_COORDINATOR_REALIGNMENT:
      TRACE_MSG(TRACE_MAC3, "COORD_REAL_CMD", (FMT__0));

      if (MAC_CTX().flags.mlme_scan_in_progress
          && MAC_CTX().scan_type == ORPHAN_SCAN)
      {
        MAC_CTX().flags.got_realignment = ZB_TRUE;

        if (ZB_SCHEDULE_TX_CB(zb_handle_coord_realignment_cmd, param) != RET_OK)
        {
          TRACE_MSG(TRACE_ERROR, "Oops! No place for zb_handle_coord_realignment_cmd in tx queue", (FMT__0));
          zb_buf_free(param);
        }
      }
      else
      {
        TRACE_MSG(TRACE_MAC1, "got unexpected coordinator realignment cmd", (FMT__0));
        zb_buf_free(param);
      }
      break;
#endif
#endif
    default:
      zb_buf_free(param);
      TRACE_MSG(TRACE_MAC1, "unsupported MAC cmd %hu", (FMT__H,*cmd_ptr));
      break;
  } /* switch */

  TRACE_MSG(TRACE_MAC2, "<<zb_mlme_handle_in_command", (FMT__0));
}

void zb_mlme_handle_data_req_command(zb_uint8_t param)
{
  zb_uint8_t *cmd_ptr;
  zb_mac_mhr_t mhr;

  TRACE_MSG(TRACE_MAC2, ">>zb_mlme_handle_in_command %hd", (FMT__H, param));

/*
  5.5.3.4 MAC command frame
  | MHR (var length) | Command type 1 byte | Command payload (var length) | FCS 2 bytes |
*/
  cmd_ptr = zb_buf_begin(param);
  cmd_ptr += zb_parse_mhr(&mhr, param);

  TRACE_MSG(TRACE_MAC2, "cmd %hu", (FMT__H,(*cmd_ptr)));
  switch (*cmd_ptr)
  {
#ifdef ZB_ROUTER_ROLE
    
    case MAC_CMD_DATA_REQUEST:   /*
                                   Coordinator side
                                   7.3.4 Data request command
                                   | MHR | cmd id |
                                 */
      TRACE_MSG(TRACE_MAC3, "MAC_CMD_DATA_REQUEST", (FMT__0));
      /* Some platforms can mark in the packet status Pending bit of MAC ACK sent in response to recieved POLL packet.
         If we sent ACK with peding bit 0, do not send data to ZED: it already switched off its radio.
         Still pass POLL into NWK so it can handle ED timeout.
      */

      if (zb_buf_get_status(param) == (zb_ret_t)MAC_NO_DATA
          /* A bit of optimization: if nothing for that device in the
           * indirect queue, pass the poll to NWK immedietly. */

          /*cstat !MISRAC2012-Rule-13.5 */
          /* After some investigation both this and the following violation of the Rule 13.5
           * seem to be false positives. There is no side effect to mac_check_pending_data().
           * This seems to be due to the existance of external functions inside
           * mac_check_pending_data() which cannot be analyzed by C-STAT. 'const' was added
           * to this function parameters. */
          || mac_check_pending_data(&mhr, 0) == -1)
      {
        TRACE_MSG(TRACE_MAC1, "mac status = %d: we sent ACK without Pending bit or no data in indirect q.", (FMT__D, zb_buf_get_status(param)));
        zb_mac_call_mcps_poll_indication(param, &mhr);
      }
      /* else call via tx q to be able to send */
      else
      {
        if (ZB_SCHEDULE_TX_CB_WITH_HIGH_PRIORITY(zb_accept_data_request_cmd, param) != RET_OK)
        {
          TRACE_MSG(TRACE_ERROR, "Oops - MAC tx queue is full", (FMT__0));
          /* Still pass to NWK - we can do it at least */
          zb_mac_call_mcps_poll_indication(param, &mhr);
        }
      }
      break;

#endif  /* ZB_ROUTER_ROLE */

    default:
      zb_buf_free(param);
      TRACE_MSG(TRACE_MAC1, "unsupported MAC cmd %hu", (FMT__H,*cmd_ptr));
      break;
  } /* switch */

  TRACE_MSG(TRACE_MAC2, "<<zb_mlme_handle_in_command", (FMT__0));
}

/**
   Send frame to the transmitter. Check for manual acknowlege.

   To be called vie tx callback except retransmit from zb_mac_logic_iteration.
   Use tx_ok_to_send flag to enforce check for send via tx cb.

   param hdr_len - frame header size
   param buf - data buffer
   return RET_OK, RET_ERROR
*/
void zb_mac_send_frame(zb_bufid_t buf, zb_uint8_t mhr_len)
{
  zb_bool_t retx = ZB_FALSE;
  zb_uint8_t *cmd_ptr;
  //ns_api_tx_wait_t tx_wait_type = ZB_MAC_TX_WAIT_CSMACA;
  zb_uint8_t tx_wait_type = ZB_MAC_TX_WAIT_CSMACA;
  zb_mac_mhr_t mhr;
  zb_uint8_t dst_addr_mode;
  zb_bool_t is_unicast;

  ZB_ASSERT(MAC_CTX().flags.tx_ok_to_send != 0U);
  /* tx can be busy only if this is retransmit */
  ZB_ASSERT(!MAC_CTX().flags.tx_q_busy || MAC_CTX().retry_counter != 0U);
  ZB_ASSERT(mhr_len != 0xffU);

  MAC_CTX().flags.tx_q_busy = ZB_TRUE;
  MAC_CTX().flags.tx_radio_busy = ZB_TRUE;
#ifdef MAC_RADIO_TX_WATCHDOG_ALARM
  ZB_SCHEDULE_ALARM(mac_tx_watchdog, 0, MAC_RADIO_TX_WATCHDOG_ALARM);
#endif

  cmd_ptr = zb_buf_begin(buf);

#ifdef ZB_CERTIFICATION_HACKS
  if (ZB_CERT_HACKS().make_frame_not_valid)
  {
    ZB_FCF_SET_SECURITY_BIT(cmd_ptr, 1);
    ZB_CERT_HACKS().make_frame_not_valid = 0;
  }
#endif

  (void)zb_parse_mhr(&mhr, buf);

  dst_addr_mode = ZB_FCF_GET_DST_ADDRESSING_MODE(mhr.frame_control);
  is_unicast = ((dst_addr_mode == ZB_ADDR_64BIT_DEV)
                || ((dst_addr_mode == ZB_ADDR_16BIT_DEV_OR_BROADCAST)
                    && !ZB_NWK_IS_ADDRESS_BROADCAST(mhr.dst_addr.addr_short)));

  TRACE_MSG(TRACE_MAC3,
            "zb_mac_send_frame buf %hu flags %hu is_unicast %hd, dst_addr 0x%x, seq_num %hd",
            (FMT__H_H_H_D_H,
             buf, (zb_uint16_t)zb_buf_flags_get(buf), is_unicast,
             mhr.dst_addr.addr_short, ZB_FCF_GET_SEQ_NUMBER(cmd_ptr)));

#if defined ZB_MAC_POWER_CONTROL
  if (!ZB_MAC_POWER_CONTROL_IS_POWER_APPLY_LOCKED() &&
      ZB_LOGICAL_PAGE_IS_SUB_GHZ(MAC_PIB().phy_current_page))
  {
    if (dst_addr_mode == ZB_ADDR_64BIT_DEV)
    {
      zb_mac_power_ctrl_apply_tx_power_by_ieee(mhr.dst_addr.addr_long);
    }
    else if (dst_addr_mode == ZB_ADDR_16BIT_DEV_OR_BROADCAST &&
             mhr.dst_addr.addr_short != 0xFFFF)
    {
      zb_mac_power_ctrl_apply_tx_power_by_short(mhr.dst_addr.addr_short);
    }
  }
  else
  {
    TRACE_MSG(TRACE_MAC2, "Power apply locked", (FMT__0));
  }
#endif  /* ZB_MAC_POWER_CONTROL */

  //JJ disable below line, since RT569 will check the ack packet automatically
  //if (ZB_FCF_GET_ACK_REQUEST_BIT(ZB_MAC_GET_FCF_PTR(cmd_ptr)) != 0U)
  if (0)
  {
    /* see 2003 7.2.1 General MAC frame format */
    if (MAC_ICTX().ack_dsn == ZB_FCF_GET_SEQ_NUMBER(cmd_ptr)
        && MAC_CTX().retx_buf == buf
        && MAC_CTX().retx_len == mhr_len)
    {
      TRACE_MSG(TRACE_MAC2, "retx_buf %hd ack_dsn %hd - retx", (FMT__H_H, MAC_CTX().retx_buf, MAC_ICTX().ack_dsn));
      retx = ZB_TRUE;
    }

    /* Put under lock: LL MAC layer may access it from the interrupt */
    ZB_RADIO_INT_DISABLE();
    MAC_ICTX().ack_dsn = cmd_ptr[2]; /* save DSN to check acks */
    ZB_RADIO_INT_ENABLE();
    MAC_CTX().retx_buf = buf;
    MAC_CTX().retx_len = mhr_len;
    TRACE_MSG(TRACE_MAC1, "sending frame with ack_dsn %hd. START_ACK_WAITING t/o %hd. retx_buf %hd len %d retx %hd phy_current_page %hd",
              (FMT__H_H_H_H_H_H,
               MAC_ICTX().ack_dsn, ZB_MAC_PIB_ACK_WAIT_DURATION, MAC_CTX().retx_buf, (zb_uint8_t)zb_buf_len(buf), retx, MAC_PIB().phy_current_page));

    ZB_MAC_CLEAR_ACK_TIMED_OUT();
    ZB_SCHEDULE_ALARM_CANCEL(zb_mac_ack_timeout, ZB_ALARM_ANY_PARAM);

#ifndef ZB_MAC_AUTO_ACK_RECV
    /* Tricky: if radio waits for ACK after TX, no need to explicitely switch
     * RX ON here. */
    //ZB_TRANSCEIVER_SET_RX_ON_OFF(1);
    /* Note that CSMA/CA time is bigger than ACK wait duration. So set ACK timeout only after TX completed. */
#endif  /* #if !defined ZB_MAC_AUTO_ACK_RECV */
  }
  else
  {
    ZB_MAC_CLEAR_ACK_NEEDED();
    ZB_MAC_CLEAR_ACK_TIMED_OUT();
    TRACE_MSG(TRACE_MAC2, "CLEAR_ACK_NEEDED", (FMT__0));
  }

  ZB_SET_MAC_STATUS(MAC_SUCCESS);
  /* If thsi is subg, use LBA instead of CSMA/CA. LBA is hand-made in MAC.
     Potential problem 1: not sure nsng can handle accurate timings (actually it was the reason of moving csma/ca into ns).
     Problem 2: this check is for single-interface MAC. Somewhere in time, when implement >1 interface, must rewrite it.
   */
  if (MAC_PIB().phy_current_page != 0U
#ifdef ZB_CERTIFICATION_HACKS
      || ZB_CERT_HACKS().zc_flood_mode
#endif
     )
  {
    tx_wait_type = ZB_MAC_TX_WAIT_NONE;
  }
  /* messages for autotest */
	zb_uint8_t *pkt;
  zb_uint_t len;
	pkt = zb_buf_begin(buf);
  len = zb_buf_len(buf);
  switch (ZB_FCF_GET_FRAME_TYPE(mhr.frame_control))
    {
      case MAC_FRAME_DATA:
				//TRACE_MSG(TRACE_ATM1, "Z< send data frame, seq_num %hd", (FMT__H, ZB_FCF_GET_SEQ_NUMBER(cmd_ptr)));
			  TRACE_MSG(TRACE_ATM1, "Z< send data frame to 0x%04x", (FMT__D, mhr.dst_addr.addr_short));
        break;
      case MAC_FRAME_BEACON:
				TRACE_MSG(TRACE_ATM1, "Z< send beacon frame", (FMT__0));
				//TRACE_MSG(TRACE_ATM1, "Z< send beacon frame, seq_num %hd", (FMT__H, ZB_FCF_GET_SEQ_NUMBER(cmd_ptr)));
        break;
			case MAC_FRAME_COMMAND:
        if (*(cmd_ptr+mhr_len) == MAC_CMD_BEACON_REQUEST)
					TRACE_MSG(TRACE_ATM1, "Z< send beacon request frame", (FMT__0));
					//TRACE_MSG(TRACE_ATM1, "Z< send beacon request frame, seq_num %hd", (FMT__H, ZB_FCF_GET_SEQ_NUMBER(cmd_ptr)));
				else if (*(cmd_ptr+mhr_len) == MAC_CMD_DATA_REQUEST)
					TRACE_MSG(TRACE_ATM1, "Z< send data request frame", (FMT__0));
					//TRACE_MSG(TRACE_ATM1, "Z< send data request frame, seq_num %hd", (FMT__H, ZB_FCF_GET_SEQ_NUMBER(cmd_ptr)));
        break;
			default:
			  break;
    }
  /* messages for autotest - end */

  if (!retx)
  {
    if (is_unicast)
    {
      ZB_MAC_DIAGNOSTIC_UNICAST_TX_TOTAL_INC();
    }
    else
    {
      ZB_MAC_DIAGNOSTIC_BCAST_TX_TOTAL_INC();
    }

    ZB_TRANS_SEND_FRAME(mhr_len, buf, tx_wait_type);
  }
  else
  {
    ZB_MAC_DIAGNOSTIC_UNICAST_TX_RETRY_INC();
    ZB_TRANS_REPEAT_SEND_FRAME(mhr_len, buf, tx_wait_type);
  }
#if defined ZB_MAC_DIAGNOSTICS
  if (ZB_BIT_IS_SET(zb_buf_flags_get(buf), ZB_BUF_HAS_APS_PAYLOAD))
  {
    ZB_MAC_DIAGNOSTICS_INC_TX_FOR_APS_MESSAGES();
  }
#endif

  /* to fix MISRAC2012-Rule-2.2_c */
  ZVUNUSED(tx_wait_type);
}

/**
   Send MAC ACK packet, do not wait for TX complete

   To be called via tx callback.
   Use operation_buf.

   @param ack_dsn - frame sequence number to acknowledge
*/
#ifndef ZB_AUTO_ACK_TX
void zb_mac_send_ack(zb_uint8_t ack_dsn)
{
/*
  7.2.2.3 Acknowledgment frame format
  | FCF 2 bytes | Seq num 1 byte |
*/
  zb_mac_mhr_t mhr;
  zb_uint8_t *ptr = zb_buf_initial_alloc(MAC_CTX().ack_tx_buf, ZB_MAC_TX_ACK_FRAME_LEN);
  ZB_ASSERT(ptr);
  ZB_BZERO(ptr, ZB_MAC_TX_ACK_FRAME_LEN);

  ZB_BZERO2(mhr.frame_control);
  ZB_FCF_SET_FRAME_TYPE(mhr.frame_control, MAC_FRAME_ACKNOWLEDGMENT);
  ZB_FCF_SET_FRAME_PENDING_BIT(mhr.frame_control, MAC_CTX().flags.pending_bit);
  mhr.seq_number = ack_dsn;
  zb_mac_fill_mhr(ptr, &mhr);

  if (ZB_TRANS_SEND_FRAME(2, MAC_CTX().ack_tx_buf, NS_TX_WAIT_ACK) == RET_OK)
  {
    MAC_CTX().flags.transmitting_ack = ZB_TRUE;
  }

  TRACE_MSG(TRACE_MAC2, "<<zb_mac_send_ack", (FMT__0));
}
#endif  /*  ZB_AUTO_ACK_TX */

/**
   Send MAC ACK packet, do not wait for TX complete

   To be called via tx callback.
   Use operation_buf.

   @param ack_dsn - frame sequence number to acknowledge
*/

#ifndef ZB_AUTO_ACK_N_RETX
/**
   Alarm which set ack timeout flag.

   Useful for manual acks
*/
void zb_mac_ack_timeout(zb_uint8_t param)
{
  ZVUNUSED(param);
  TRACE_MSG(TRACE_MAC2, "zb_mac_ack_timeout", (FMT__0));

  ZB_ASSERT(MAC_CTX().flags.tx_q_busy);
  ZB_MAC_SET_ACK_TIMED_OUT();
  if (!MAC_CTX().flags.tx_radio_busy)
  {
    //ZB_TRANSCEIVER_SET_RX_ON_OFF(ZB_PIB_RX_ON_WHEN_IDLE());
  }
}
#endif


void zb_mac_scan_timeout(zb_uint8_t param)
{
  ZVUNUSED(param);
  TRACE_MSG(TRACE_MAC2, "mac_scan_tmo", (FMT__0));
  MAC_CTX().flags.scan_timeout = ZB_TRUE;
}


#ifdef MAC_RADIO_TX_WATCHDOG_ALARM
void mac_tx_watchdog(zb_uint8_t param)
{
  ZVUNUSED(param);
  TRACE_MSG(TRACE_ERROR, "mac_tx_watchdog", (FMT__0));
  ZB_MAC_TX_ABORT();
}
#endif

/**
   Check for TX status when sure TX is just done.

   Set mac_status.
   That function does not block.
*/

/*
 * This function must be retruning zb_ret_t value
 * beacuse it need for MAC-certification tests.
 */
void zb_check_cmd_tx_status(void)
{
#ifdef MAC_RADIO_TX_WATCHDOG_ALARM
  ZB_SCHEDULE_ALARM_CANCEL(mac_tx_watchdog, ZB_ALARM_ANY_PARAM);
#endif

  MAC_CTX().flags.tx_radio_busy = ZB_FALSE;
  if (ZB_TRANS_CHECK_CHANNEL_BUSY_ERROR())
  {
    TRACE_MSG(TRACE_MAC3, "zb_check_cmd_tx_status TX failed - Channel busy", (FMT__0));
    ZB_SET_MAC_STATUS(MAC_CHANNEL_ACCESS_FAILURE);
    ZB_MAC_DIAGNOSTICS_PHY_CCA_FAIL_INC();
    /* If unicast, let's retransmit after TX channel busy state.
       Retransmit only in case of ACK timeout is not expired yet.
       Moreover, we need to schedule only via scheduler alarm queue (not callback queue),
       because we will not be able to remove the function from callback queue and
       it could be called after a long time delay that will cause a race condition.
     */
    ZB_SCHEDULE_ALARM_CANCEL(zb_mac_ack_timeout, ZB_ALARM_ANY_PARAM);
    if (ZB_MAC_GET_ACK_NEEDED() && !ZB_MAC_GET_ACK_TIMED_OUT())
    {
#ifdef ZB_ROUTER_ROLE
      if (MAC_CTX().tx_wait_cb == zb_handle_data_request_cmd_continue
          /* Some radio (MAC), if misses association resp, does not send data
           * request again. Let's increase their chances to associate by
           * re-sending it immediately. */

          /*cstat !MISRAC2012-Rule-13.5 */
          /* After some investigation, the following violation of Rule 13.5 seems to be
           * a false positive. There are no side effects to 'is_association_resp()'. This
           * violation seems to be caused by the existance of external functions inside
           * which cannot be analyzed by C-STAT. */
          && is_association_resp(MAC_CTX().retx_buf))
      {
        /* This is fine, this callback will not break anything. I hope. */
        ZB_SCHEDULE_CALLBACK(zb_mac_ack_timeout, 0);
      }
      else
#endif
      {
        ZB_SCHEDULE_ALARM(zb_mac_ack_timeout, 0, ZB_MAC_PIB_ACK_WAIT_DURATION);
      }
    }
  }
  /*cstat !MISRAC2012-Rule-14.3_b */
  /** @mdr{00005,0} */
  else if (ZB_U2B(ZB_TRANS_CHECK_TX_RETRY_COUNT_EXCEEDED_ERROR()))
  {
    /* we can be here only if radio supports automatical retransmits */
    TRACE_MSG(TRACE_MAC3, "zb_check_cmd_tx_status Out of tx retries - no ACK", (FMT__0));
    ZB_SET_MAC_STATUS(MAC_NO_ACK);
  }
#if defined ZB_MAC_AUTO_ACK_RECV
  /* Note: in case of MAC LL which can work with or without ZB_MAC_AUTO_ACK_RECV:
     radio without ZB_MAC_AUTO_ACK_RECV just never set "no ack" error code.
     No additional checks are required here.

     There were checks for 2.4 vs subGHz pages - that were wrong!
  */
  else if (ZB_TRANS_CHECK_NO_ACK())
  {
    /* we can be here only if radio supports single ack rx after tx */
    /* Radio (or low MAC level) waited for ACK after TX complete, but timed out */
    TRACE_MSG(TRACE_MAC3, "zb_check_cmd_tx_status ACK RX failed", (FMT__0));
    /* ZB_SCHEDULE_ALARM(zb_mac_ack_timeout, 0, ZB_MAC_PIB_ACK_WAIT_DURATION); */
    /* Not sure why we need to wait additional ZB_MAC_PIB_ACK_WAIT_DURATION timeout?
       We already got ACK RX failed (ACK RX is already timed out), so may retransmit
       immediately.

       Q: Do we need to do ZB_SCHEDULE_ALARM_CANCEL(zb_mac_ack_timeout) here?
       A: Seems like it is not needed - for ZB_MAC_AUTO_ACK_RECV the only place where
       ZB_SCHEDULE_ALARM(zb_mac_ack_timeout) is done is on ZB_TRANS_CHECK_CHANNEL_BUSY_ERROR() in
       zb_check_cmd_tx_status() - this place is protected by
       ZB_SCHEDULE_ALARM_CANCEL(zb_mac_ack_timeout) before scheduling.
    */
    ZB_SCHEDULE_CALLBACK(zb_mac_ack_timeout, 0);
//    ZB_ASSERT(ZB_MAC_GET_ACK_NEEDED());
    /* Do not set MAC status yet. Will decide in the main loop: retransmit of NO_ACK. */
  }
#endif  /*  ZB_MAC_AUTO_ACK_RECV */
#if defined ZB_SUB_GHZ_LBT
  else if (ZB_TRANS_CHECK_TX_LBT_TO_ERROR())
  {
    TRACE_MSG(TRACE_MAC3, "zb_check_cmd_tx_status Tx failed while LBT performing", (FMT__0));
    ZB_SET_MAC_STATUS(MAC_LIMIT_REACHED);
  }
#endif /* ZB_SUB_GHZ_LBT */
  else
  {
    TRACE_MSG(TRACE_MAC3, "zb_check_cmd_tx_status TX ok", (FMT__0));
    ZB_SET_MAC_STATUS(MAC_SUCCESS);
#ifndef ZB_MAC_AUTO_ACK_RECV
    if (ZB_MAC_GET_ACK_NEEDED())
    {
      /* Note that CSMA/CA time is bigger than ACK wait duration. So set ACK timeout only after TX completed. */
      TRACE_MSG(TRACE_MAC3, "alarm for zb_mac_ack_timeout %d", (FMT__D, ZB_MAC_PIB_ACK_WAIT_DURATION));
      ZB_SCHEDULE_ALARM(zb_mac_ack_timeout, 0, ZB_MAC_PIB_ACK_WAIT_DURATION);
    }
#endif
  }

#if defined ZB_MAC_DIAGNOSTICS
  if (ZB_GET_MAC_STATUS() != MAC_SUCCESS
      /*cstat !MISRAC2012-Rule-13.5 !MISRAC2012-Rule-13.6 */
      /* After some investigation, the following violations of Rule 13.5 and 13.6 seem to be false
       * positives. There are no side effect to 'ZB_MAC_GET_ACK_NEEDED()'. This violation seems to
       * be caused by the fact that 'ZB_MAC_GET_ACK_NEEDED()' is an external macro, which cannot be
       * analyzed by C-STAT. */
      && !ZB_MAC_GET_ACK_NEEDED())
  {
    ZB_MAC_DIAGNOSTIC_UNICAST_TX_FAILED_INC();
  }
#endif  /* ZB_MAC_DIAGNOSTICS */
}

/* returns true if frame is sent to the current PAN */
zb_bool_t zb_mac_check_frame_pan_id(const zb_mac_mhr_t *mhr)
{
#ifdef ZB_MANUAL_ADDR_FILTER
  zb_bool_t ret = ZB_FALSE;
  /*TO DO: Add interpan communication if it is necessary*/
  TRACE_MSG(TRACE_MAC3, ">>zb_mac_check_frame_pan_id dst pan id 0x%x", (FMT__D, mhr->dst_pan_id));
  if ((mhr->dst_pan_id == MAC_PIB().mac_pan_id)
      || (mhr->dst_pan_id == ZB_BROADCAST_PAN_ID)
      || (MAC_PIB().mac_pan_id == ZB_BROADCAST_PAN_ID &&
          ZB_FCF_GET_FRAME_TYPE(mhr->frame_control) == MAC_FRAME_BEACON))
  {
    ret =  ZB_TRUE;
  }
  TRACE_MSG(TRACE_MAC3, "<<zb_mac_check_frame_pan_id ret %hd", (FMT__H, ret));
  return ret;
#else
  (void)mhr;
  return ZB_TRUE;
#endif
}


/* returns true if frame is sent directly to current device */
zb_bool_t zb_mac_check_frame_dst_addr(const zb_mac_mhr_t *mhr)
{
#ifdef ZB_MANUAL_ADDR_FILTER
  zb_bool_t ret = ZB_FALSE;

  TRACE_MSG(TRACE_MAC3, ">>zb_mac_check_frame_dst_addr dst addr mode %i", (FMT__D, ZB_FCF_GET_DST_ADDRESSING_MODE(mhr->frame_control)));
  if (ZB_FCF_GET_DST_ADDRESSING_MODE(mhr->frame_control) == ZB_ADDR_NO_ADDR)
  {
    /* 7.2.1.1.6 Destination Addressing Mode subfield
       If this subfield is equal to zero and the Frame Type subfield does not specify that this frame is an
       acknowledgment or beacon frame, the Source Addressing Mode subfield shall be nonzero, implying that the
       frame is directed to the PAN coordinator with the PAN identifier as specified in the Source PAN Identifier
       field. */
    ret =
      (zb_bool_t)
#ifdef ZB_COORDINATOR_ROLE
      (MAC_PIB().mac_pan_coordinator == 1) ||
#endif
      (ZB_FCF_GET_FRAME_TYPE(mhr->frame_control) == MAC_FRAME_ACKNOWLEDGMENT) || (ZB_FCF_GET_FRAME_TYPE(mhr->frame_control) == MAC_FRAME_BEACON);
  }
  else if (ZB_FCF_GET_DST_ADDRESSING_MODE(mhr->frame_control) == ZB_ADDR_16BIT_DEV_OR_BROADCAST)
  {
    TRACE_MSG(TRACE_MAC3, "mhr dst 0x%x, pib short a 0x%x", (FMT__D_D,
                                                             mhr->dst_addr.addr_short, MAC_PIB().mac_short_address));
    ret = (zb_bool_t)(mhr->dst_addr.addr_short == MAC_PIB().mac_short_address
                      || MAC_PIB().mac_short_address == ZB_MAC_SHORT_ADDR_NO_VALUE);
  }
  else
  {
    /* 64 bit case */
    TRACE_MSG(TRACE_MAC3, "mhr addr " TRACE_FORMAT_64 " pib ext " TRACE_FORMAT_64, (FMT__A_A,
                                                                                    TRACE_ARG_64(mhr->dst_addr.addr_long),
                                                                                    TRACE_ARG_64(MAC_PIB().mac_extended_address)));
    ret = ZB_64BIT_ADDR_CMP(mhr->dst_addr.addr_long, MAC_PIB().mac_extended_address);
  }
  TRACE_MSG(TRACE_MAC3, "<<zb_mac_check_frame_dst_addr ret %hd", (FMT__H, ret));
  return ret;
#else
  (void)mhr;
  return ZB_TRUE;
#endif
}

#ifdef ZB_RAF_INDIRECT_DATA_RX_FIX
/* returns true if frame is sent directly to current device */
zb_bool_t zb_mac_check_frame_unicast_to_myself(const zb_mac_mhr_t *mhr)
{
  zb_bool_t ret = ZB_FALSE;
  zb_bool_t ret_pan_id = ZB_FALSE;
  zb_bool_t ret_dst_addr = ZB_FALSE;
  /*TO DO: Add interpan communication if it is necessary*/
  TRACE_MSG(TRACE_MAC3, ">>zb_mac_check_frame_pan_id dst pan id 0x%x", (FMT__D, mhr->dst_pan_id));
  if (mhr->dst_pan_id == MAC_PIB().mac_pan_id)
  {
    ret_pan_id =  ZB_TRUE;
  }
	
  TRACE_MSG(TRACE_MAC3, ">>zb_mac_check_frame_dst_addr dst addr mode %i", (FMT__D, ZB_FCF_GET_DST_ADDRESSING_MODE(mhr->frame_control)));
  if (ZB_FCF_GET_DST_ADDRESSING_MODE(mhr->frame_control) == ZB_ADDR_NO_ADDR)
  {
    ret_dst_addr = ZB_FALSE;
  }
  else if (ZB_FCF_GET_DST_ADDRESSING_MODE(mhr->frame_control) == ZB_ADDR_16BIT_DEV_OR_BROADCAST)
  {
    TRACE_MSG(TRACE_MAC3, "mhr dst 0x%x, pib short a 0x%x", (FMT__D_D,
                                                             mhr->dst_addr.addr_short, MAC_PIB().mac_short_address));
    ret_dst_addr = (zb_bool_t)(mhr->dst_addr.addr_short == MAC_PIB().mac_short_address
                  && MAC_PIB().mac_short_address != ZB_MAC_SHORT_ADDR_NOT_ALLOCATED
		              && MAC_PIB().mac_short_address != ZB_MAC_SHORT_ADDR_NO_VALUE);
  }
  else
  {
    /* 64 bit case */
    TRACE_MSG(TRACE_MAC3, "mhr addr " TRACE_FORMAT_64 " pib ext " TRACE_FORMAT_64, (FMT__A_A,
                                                                                    TRACE_ARG_64(mhr->dst_addr.addr_long),
                                                                                    TRACE_ARG_64(MAC_PIB().mac_extended_address)));
    ret_dst_addr = ZB_64BIT_ADDR_CMP(mhr->dst_addr.addr_long, MAC_PIB().mac_extended_address);
  }
	
  if(ret_pan_id && ret_dst_addr)
  {
    ret = ZB_TRUE;
  }
  TRACE_MSG(TRACE_MAC3, "<<zb_mac_check_frame_unicast_to_myself ret %hd", (FMT__H, ret));
  return ret;
}
#endif

/* returns true if frame is broadcast */
zb_bool_t zb_mac_check_frame_is_broadcast(const zb_mac_mhr_t *mhr)
{
  zb_bool_t ret = ZB_FALSE;

  /* Direct assignment to 'ret' instead of using 'if' may produce C-STAT warning in the places
   * where this function is called on right side of '&&' or '||'.
   */
  if (ZB_FCF_GET_DST_ADDRESSING_MODE(mhr->frame_control) == ZB_ADDR_16BIT_DEV_OR_BROADCAST
      && mhr->dst_addr.addr_short == ZB_BROADCAST_PAN_ID)
  {
    ret = ZB_TRUE;
  }

  TRACE_MSG(TRACE_MAC3, "zb_mac_check_frame_is_broadcast %hd", (FMT__H, ret));
  return ret;
}


#ifdef ZB_ED_FUNC
/**
   MLME-POLL.request

   Fill and send data request for indirect transmission
*/
void zb_mlme_poll_request(zb_uint8_t param)
{
  /* poll request sends data, so call it via tx q */
  zb_bool_t success = ZB_FALSE;

  if (MAC_CTX().pending_buf == 0U && !ZB_MAC_INDIRECT_IN_PROGRESS())
  {
    success = (ZB_SCHEDULE_TX_CB(zb_handle_mlme_poll_request, param) == RET_OK);
  }

  if (!success)
  {
    zb_buf_set_status(param, MAC_TRANSACTION_OVERFLOW);
    TRACE_MSG(
        TRACE_MAC2,
        "indirect rcv is in progress or tx cb queue full when called POLL - call poll.confirm",
        (FMT__0));
#if defined ZB_MAC_API_TRACE_PRIMITIVES
    zb_mac_api_trace_poll_confirm(param);
#endif
    ZB_SCHEDULE_CALLBACK(zb_mlme_poll_confirm, param);
  }
}


/**
   Handle MLME-POLL.request - called via tx q, so safe to do tx
*/
void zb_handle_mlme_poll_request(zb_uint8_t param)
{
  zb_ret_t ret = RET_OK;
  zb_mlme_poll_request_t *request;
  zb_mlme_data_req_params_t data_req = {0};

  TRACE_MSG(TRACE_MAC2, ">>zb_mlme_poll_request", (FMT__0));

  ZVUNUSED(ret);

  /* At client side pending_buf used for poll and associate. If it is non zero,
   * associate or poll is in progress already */
  if (MAC_CTX().pending_buf != 0U
      || ZB_MAC_INDIRECT_IN_PROGRESS())
  {
    zb_buf_set_status(param, MAC_INVALID_PARAMETER);
    TRACE_MSG(TRACE_MAC2, "indirect rcv is in progress when called POLL - call poll.confirm", (FMT__0));
#if defined ZB_MAC_API_TRACE_PRIMITIVES
    zb_mac_api_trace_poll_confirm(param);
#endif
    ZB_SCHEDULE_CALLBACK(zb_mlme_poll_confirm, param);
  }
  else
  {
    MAC_CTX().pending_buf = param;
    TRACE_MSG(TRACE_MAC3, "pending_buf %hd", (FMT__H, MAC_CTX().pending_buf));
    /*cstat !MISRAC2012-Rule-20.7 See ZB_BUF_GET_PARAM() for more information. */
    request = ZB_BUF_GET_PARAM(MAC_CTX().pending_buf, zb_mlme_poll_request_t);

    /* fill indirect data request cmd params */
    data_req.src_addr_mode = (ZB_PIB_SHORT_ADDRESS() < ZB_MAC_SHORT_ADDR_NOT_ALLOCATED) ? ZB_ADDR_16BIT_DEV_OR_BROADCAST : ZB_ADDR_64BIT_DEV;
    if (data_req.src_addr_mode == ZB_ADDR_64BIT_DEV)
    {
      ZB_IEEE_ADDR_COPY(data_req.src_addr.addr_long, ZB_PIB_EXTENDED_ADDRESS());
    }
    else
    {
      data_req.src_addr.addr_short = ZB_PIB_SHORT_ADDRESS();
    }

    data_req.dst_addr_mode = request->coord_addr_mode;
    if (data_req.dst_addr_mode == ZB_ADDR_64BIT_DEV)
    {
      ZB_IEEE_ADDR_COPY(data_req.dst_addr.addr_long, request->coord_addr.addr_long);

      TRACE_MSG(TRACE_MAC3, "poll_request: dst_addr (long) " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(data_req.dst_addr.addr_long)));
    }
    else
    {
      data_req.dst_addr.addr_short = request->coord_addr.addr_short;
    }

    MAC_CTX().flags.poll_inprogress = ZB_TRUE;
    /* actually poll_rate is not used */
    MAC_CTX().poll_rate = request->poll_rate;
    TRACE_MSG(TRACE_MAC3, "poll_request: poll_rate = %ld", (FMT__L, MAC_CTX().poll_rate));

    zb_mac_get_indirect_data(&data_req);
  }
#if defined ZB_MAC_API_TRACE_PRIMITIVES
  zb_mac_api_trace_poll_request(param);
#endif
  TRACE_MSG(TRACE_MAC2, "<<zb_mlme_poll_request %hd", (FMT__H, ret));
}
#endif  /* ZB_ED_FUNC */


/*Stubs for MAC only build*/
#ifdef ZB_MAC_TESTING_MODE
void zb_nwk_unlock_in(zb_uint8_t param)
{
  ZVUNUSED(param);
}

void zb_check_oom_status(zb_uint8_t param)
{
  ZVUNUSED(param);
}

void zb_zcl_init()
{
}
#endif

static void mac_update_mac_pib_constants(void)
{
  if (MAC_PIB().phy_current_page == ZB_MAC_INVALID_LOGICAL_PAGE)
  {
    /* Do nothing */
  }
  else if (ZB_LOGICAL_PAGE_IS_2_4GHZ(MAC_PIB().phy_current_page))
  {
    MAC_PIB().mac_max_frame_total_wait_time = ZB_MAC_PIB_MAX_FRAME_TOTAL_WAIT_TIME_CONST_2_4_GHZ;
    MAC_PIB().mac_ack_wait_duration = ZB_MAC_PIB_ACK_WAIT_DURATION_CONST;
  }
  else if (ZB_LOGICAL_PAGE_IS_SUB_GHZ(MAC_PIB().phy_current_page))
  {
    MAC_PIB().mac_max_frame_total_wait_time = ZB_MAC_PIB_MAX_FRAME_TOTAL_WAIT_TIME_CONST_SUB_GHZ;
    MAC_PIB().mac_ack_wait_duration = ZB_MAC_PIB_ACK_WAIT_DURATION_CONST_EU_FSK;
  }
  else
  {
    /* Unknown page */
    ZB_ASSERT(ZB_FALSE);
  }
}

#ifdef ZB_MAC_CONFIGURABLE_TX_POWER

void zb_mac_set_tx_power_provider_function(zb_tx_power_provider_t new_provider)
{
  MAC_CTX().tx_power_provider = new_provider;
}
#endif /*ZB_MAC_CONFIGURABLE_TX_POWER*/

void zb_mac_set_tx_power(zb_int8_t new_power)
{
  TRACE_MSG(TRACE_MAC3, "zb_mac_set_tx_power old power %hd, new power %hd",
            (FMT__H_H, MAC_CTX().current_tx_power, new_power));

  if (new_power != MAC_CTX().current_tx_power)
  {
    MAC_CTX().current_tx_power = new_power;

#ifdef ZB_MAC_CONFIGURABLE_TX_POWER
    ZB_TRANSCEIVER_SET_TX_POWER(new_power);

    /*
    * Get back actual TX power from transceiver
    * (it could be changed by transceiver in case of invalid value)
    */
    ZB_TRANSCEIVER_GET_TX_POWER(&MAC_CTX().current_tx_power);
#endif /*ZB_MAC_CONFIGURABLE_TX_POWER*/
  }
}

//JJJ
#if 0
zb_int8_t zb_mac_get_tx_power(void)
{
  /* get Tx power value and at the same time keep MAC_CTX().current_tx_power up to date */
  ZB_TRANSCEIVER_GET_TX_POWER(&MAC_CTX().current_tx_power);
  TRACE_MSG(TRACE_MAC3, "zb_mac_get_tx_power power %hd",
            (FMT__H, MAC_CTX().current_tx_power));
  return MAC_CTX().current_tx_power;
}
#endif

#ifdef ZB_MAC_CONFIGURABLE_TX_POWER
static zb_int8_t mac_get_tx_power(zb_uint8_t page, zb_uint8_t channel)
{
  zb_int8_t power_dbm = ZB_MAC_TX_POWER_INVALID_DBM;

  if (MAC_CTX().tx_power_provider != NULL)
  {
    zb_int8_t out_dbm;

    if (MAC_CTX().tx_power_provider(page, channel, &out_dbm) == RET_OK)
    {
      power_dbm = out_dbm;
      TRACE_MSG(TRACE_MAC3, "tx_power_provider ret OK out_dbm %hd", (FMT__H, out_dbm));
    }
    else
    {
      TRACE_MSG(TRACE_MAC3, "tx_power_provider error", (FMT__0));
    }
  }

  if (power_dbm == ZB_MAC_TX_POWER_INVALID_DBM)
  {
    if (ZB_LOGICAL_PAGE_IS_2_4GHZ(page))
    {
      power_dbm = ZB_MAC_DEFAULT_TX_POWER_24_GHZ;
    }
    else if (ZB_LOGICAL_PAGE_IS_SUB_GHZ_GB_FSK(page) || ZB_LOGICAL_PAGE_IS_SUB_GHZ_EU_FSK(page))
    {
      power_dbm = ZB_MAC_DEFAULT_TX_POWER_GB_EU_SUB_GHZ;
    }
    else if (ZB_LOGICAL_PAGE_IS_SUB_GHZ_NA_FSK(page))
    {
      power_dbm = ZB_MAC_DEFAULT_TX_POWER_NA_SUB_GHZ;
    }
    else
    {
      ZB_ASSERT(ZB_FALSE);
    }
  }

  TRACE_MSG(TRACE_MAC1, "mac_get_tx_power page %hd channel %hd ret(%hd)",
            (FMT__H_H_H, page, channel, power_dbm));

  return power_dbm;
}
#endif /* ZB_MAC_CONFIGURABLE_TX_POWER */

void zb_mac_change_channel(zb_uint8_t logical_page, zb_uint8_t logical_channel)
{
  zb_bool_t iface_changed;

  TRACE_MSG(TRACE_MAC1, "zb_mac_change_channel page %hd channel %hd",
            (FMT__H_H, logical_page, logical_channel));

  /* TODO: turn on this check and fix
   * ZB_TRANSCEIVER_START_CHANNEL_NUMBER/ZB_TRANSCEIVER_MAX_CHANNEL_NUMBER if needed.  */
  /* ZB_ASSERT(MAC_PIB().phy_current_channel >= ZB_TRANSCEIVER_START_CHANNEL_NUMBER */
  /*           && MAC_PIB().phy_current_channel <= ZB_TRANSCEIVER_MAX_CHANNEL_NUMBER); */

  /* So, logic is simple: disable old interface, enable new interface if they are different */
  /* Old interface is MAC_PIB().phy_current_page, new is logical_page */
  if (MAC_PIB().phy_current_page == ZB_MAC_INVALID_LOGICAL_PAGE)
  {
    /* Disable nothing here */
    iface_changed = ZB_TRUE;
  }
  else if (logical_page == ZB_MAC_INVALID_LOGICAL_PAGE
           || ZB_LOGICAL_PAGE_IS_2_4GHZ(MAC_PIB().phy_current_page)
                  != ZB_LOGICAL_PAGE_IS_2_4GHZ(logical_page)
           || ZB_LOGICAL_PAGE_IS_SUB_GHZ_NA_FSK(MAC_PIB().phy_current_page)
                  != ZB_LOGICAL_PAGE_IS_SUB_GHZ_NA_FSK(logical_page))
  {
    /* Disable old iface */
    ZB_TRANSCEIVER_DEINIT_RADIO();
    iface_changed = ZB_TRUE;
  }
  else
  {
    iface_changed = ZB_FALSE;
  }

  MAC_PIB().phy_current_page = logical_page;
  MAC_PIB().phy_ephemeral_page = logical_page;
  MAC_PIB().phy_current_channel = logical_channel;

  if (iface_changed)
  {
    /* Enable new iface */
    ZB_TRANSCEIVER_INIT_RADIO();
#if defined ZB_MAC_STICKY_PENDING_BIT && defined ZB_ROUTER_ROLE
    /* Some radios clears pending bit. Not sure about other radios. Let's set it back. */
    ZB_MAC_TRANS_SET_PENDING_BIT();
#endif
    mac_update_mac_pib_constants();
  }

  if (MAC_PIB().phy_current_page != ZB_MAC_INVALID_LOGICAL_PAGE &&
      MAC_PIB().phy_current_channel != ZB_MAC_INVALID_LOGICAL_CHANNEL)
  {
#ifdef ZB_MAC_DUTY_CYCLE_MONITORING
    zb_mac_duty_cycle_update_regulated(logical_page);
#endif  /* ZB_MAC_DUTY_CYCLE_MONITORING */

#ifdef ZB_MAC_CONFIGURABLE_TX_POWER
    MAC_CTX().default_tx_power = mac_get_tx_power(MAC_PIB().phy_current_page, MAC_PIB().phy_current_channel);
    zb_mac_set_tx_power(MAC_CTX().default_tx_power);
#endif  /* ZB_MAC_CONFIGURABLE_TX_POWER */

    /* This trace msg is used to verify regression test RTP_ZDO_03. */
    TRACE_MSG(TRACE_MAC1, "zb_mac_change_channel: MAC_PIB().mac_rx_on_when_idle %hd",
              (FMT__H, MAC_PIB().mac_rx_on_when_idle));

    ZB_TRANSCEIVER_SET_CHANNEL(logical_page, logical_channel);
    //ZB_TRANSCEIVER_SET_RX_ON_OFF(MAC_PIB().mac_rx_on_when_idle != 0U);
  }
}

/*! @} */

#endif  /* !ZB_ALIEN_MAC && !ZB_MACSPLIT_HOST */
