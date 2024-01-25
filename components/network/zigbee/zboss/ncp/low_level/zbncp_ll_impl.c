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
/*  PURPOSE: NCP low level protocol implementation.
*/

#define ZB_TRACE_FILE_ID 31
#include "zbncp_ll_impl.h"
#include "zbncp_tr_impl.h"

static void zbncp_ll_tx_start_sending(zbncp_ll_proto_t *ll);
static void zbncp_ll_tx_complete_pkt(zbncp_ll_proto_t *ll, zbncp_uint8_t flags);
static zbncp_bool_t zbncp_ll_is_tx_ready_send_ack(zbncp_ll_proto_t *ll);
static void zbncp_ll_tx_send_ack(zbncp_ll_proto_t *ll, zbncp_uint8_t frameno, zbncp_uint8_t flags);
static void zbncp_ll_change_sending_pkt_state(zbncp_ll_proto_t *ll, zbncp_bool_t state);

#if ZBNCP_DEBUG

static const char *zbncp_ll_rx_st(const zbncp_ll_proto_t *ll)
{
    const char *result;

    switch (ll->rx.state)
    {
    case ZBNCP_LL_RX_STATE_IDLE          :
        result = "RX-IDLE";
        break;
    case ZBNCP_LL_RX_STATE_RECEIVING_HDR :
        result = "RX-RECEIVING-HDR";
        break;
    case ZBNCP_LL_RX_STATE_HDR_RECEIVED  :
        result = "RX-HDR-RECEIVED";
        break;
    case ZBNCP_LL_RX_STATE_HDR_VALIDATED :
        result = "RX-HDR-VALIDATED";
        break;
    case ZBNCP_LL_RX_STATE_RECEIVING_BODY:
        result = "RX-RECEIVING_BODY";
        break;
    case ZBNCP_LL_RX_STATE_BODY_RECEIVED :
        result = "RX-BODY-RECEIVED";
        break;
    case ZBNCP_LL_RX_STATE_SEND_ACK      :
        result = "RX-SEND-ACK";
        break;
    case ZBNCP_LL_RX_STATE_SENDING_ACK   :
        result = "RX-SENDING-ACK";
        break;
    case ZBNCP_LL_RX_STATE_RESYNC        :
        result = "RX-RESYNC";
        break;
    case ZBNCP_LL_RX_STATE_ERROR         :
        result = "RX-ERROR";
        break;
    default:
        result = "RX-@UNK";
        break;
    }

    return result;
}

static const char *zbncp_ll_tx_st(const zbncp_ll_proto_t *ll)
{
    const char *result;

    switch (ll->tx.state)
    {
    case ZBNCP_LL_TX_STATE_IDLE         :
        result = "TX-IDLE";
        break;
    case ZBNCP_LL_TX_STATE_SENDING_ACK  :
        result = "TX-SENDING-ACK";
        break;
    case ZBNCP_LL_TX_STATE_ACK_SENT     :
        result = "TX-ACK-SENT";
        break;
    case ZBNCP_LL_TX_STATE_SENDING_PKT  :
        result = "TX-SENDING-PKT";
        break;
    case ZBNCP_LL_TX_STATE_PKT_SENT     :
        result = "TX-PKT-SENT";
        break;
    case ZBNCP_LL_TX_STATE_WAITING_ACK  :
        result = "TX-RECEIVING-ACK";
        break;
    case ZBNCP_LL_TX_STATE_ACK_RECEIVED :
        result = "TX-ACK_RECEIVED";
        break;
    case ZBNCP_LL_TX_STATE_ERROR        :
        result = "TX-ERROR";
        break;
    default:
        result = "TX-@UNK";
        break;
    }

    return result;
}

static zbncp_size_t zbncp_ll_rx_expected_size(zbncp_ll_proto_t *ll)
{
    zbncp_size_t result;

    switch (ll->rx.state)
    {
    case ZBNCP_LL_RX_STATE_RECEIVING_HDR:
        result = ll->rx.pkt.size;
        break;
    case ZBNCP_LL_RX_STATE_RECEIVING_BODY:
        result = (ll->rx.pkt.size - ZBNCP_LL_BODY_CRC_OFFSET);
        break;
    case ZBNCP_LL_RX_STATE_RESYNC:
        result = 1u;
        break;
    default:
        result = 0u;
        break;
    }

    return result;
}
#endif /* ZBNCP_DEBUG */

static void zbncp_ll_callme(zbncp_ll_proto_t *ll)
{
    if (ll->cb.callme != ZBNCP_NULL)
    {
        ll->cb.callme(ll->cb.arg);
    }
}

static void zbncp_ll_init_complete(zbncp_ll_proto_t *ll)
{
    ZBNCP_DBG_TRACE("(%s : %s)", zbncp_ll_rx_st(ll), zbncp_ll_tx_st(ll));

    zbncp_ll_callme(ll);
}

static void zbncp_ll_send_complete(zbncp_ll_proto_t *ll, zbncp_tr_send_status_t status)
{
#if ZBNCP_DEBUG
    const char *prev = zbncp_ll_tx_st(ll);
#endif /* ZBNCP_DEBUG */

    switch (ll->tx.state)
    {
    case ZBNCP_LL_TX_STATE_SENDING_ACK:
        if (status == ZBNCP_TR_SEND_STATUS_SUCCESS)
        {
            ll->tx.tx_attempts_cnt = 0;
            ll->tx.state = ZBNCP_LL_TX_STATE_ACK_SENT;
        }
        else if (status == ZBNCP_TR_SEND_STATUS_BUSY)
        {
            /* BUSY status can mean TX/TX conflict or another transaction in progress */
            ll->tx.state = ZBNCP_LL_TX_STATE_IDLE; /* Retransmit last ACK */
        }
        else
        {
            ll->tx.tx_attempts_cnt++;
            ll->tx.state = ZBNCP_LL_TX_STATE_IDLE; /* Retransmit last ACK */
        }
        break;

    case ZBNCP_LL_TX_STATE_SENDING_PKT:
        if (status == ZBNCP_TR_SEND_STATUS_SUCCESS)
        {
            ll->tx.tx_attempts_cnt = 0;
            ll->tx.state = ZBNCP_LL_TX_STATE_PKT_SENT;
        }
        else if (status == ZBNCP_TR_SEND_STATUS_BUSY)
        {
            /* BUSY status can mean TX/TX conflict or another transaction in progress */
            ll->tx.state = ZBNCP_LL_TX_STATE_IDLE; /* Retransmit last packet */
        }
        else
        {
            ll->tx.tx_attempts_cnt++;
            ll->tx.state = ZBNCP_LL_TX_STATE_IDLE; /* Retransmit last packet */
        }
        break;

    default:
        ll->tx.state = ZBNCP_LL_TX_STATE_ERROR;
        ZBNCP_DBG_TRACE("    TX ERROR!!!");
        break;
    }

    ZBNCP_DBG_TRACE("(%s : %s --> %s)", zbncp_ll_rx_st(ll), prev, zbncp_ll_tx_st(ll));

    zbncp_ll_callme(ll);
}

static void zbncp_ll_recv_complete(zbncp_ll_proto_t *ll, zbncp_size_t size)
{
#if ZBNCP_DEBUG
    const char *prev = zbncp_ll_rx_st(ll);
    zbncp_size_t expected = zbncp_ll_rx_expected_size(ll);
    if (size != expected)
    {
        ZBNCP_DBG_TRACE("    RX ERROR!!! expected size %zu != recv size %zu", expected, size);
    }
#else
    ZBNCP_UNUSED(size);
#endif /* ZBNCP_DEBUG */

    switch (ll->rx.state)
    {
    case ZBNCP_LL_RX_STATE_RESYNC:
    case ZBNCP_LL_RX_STATE_RECEIVING_HDR:
        ll->rx.state = ZBNCP_LL_RX_STATE_HDR_RECEIVED;
        break;

    case ZBNCP_LL_RX_STATE_RECEIVING_BODY:
        ll->rx.state = ZBNCP_LL_RX_STATE_BODY_RECEIVED;
        break;

    default:
        ll->rx.state = ZBNCP_LL_RX_STATE_ERROR;
        ZBNCP_DBG_TRACE("    RX ERROR!!!");
        break;
    }

    ZBNCP_DBG_TRACE("(%s --> %s : %s)", prev, zbncp_ll_rx_st(ll), zbncp_ll_tx_st(ll));

    zbncp_ll_callme(ll);
}

static inline void zbncp_ll_rx_set_state(zbncp_ll_proto_t *ll, zbncp_ll_rx_state_t state)
{
#if ZBNCP_DEBUG
    const char *prev = zbncp_ll_rx_st(ll);
#endif /* ZBNCP_DEBUG */

    ll->rx.state = state;
    ll->service = ZBNCP_TRUE;

    ZBNCP_DBG_TRACE("(%s --> %s)", prev, zbncp_ll_rx_st(ll));
}

static inline void zbncp_ll_tx_set_state(zbncp_ll_proto_t *ll, zbncp_ll_tx_state_t state)
{
#if ZBNCP_DEBUG
    const char *prev = zbncp_ll_tx_st(ll);
#endif /* ZBNCP_DEBUG */

    ll->tx.state = state;
    ll->service = ZBNCP_TRUE;

    ZBNCP_DBG_TRACE("(%s --> %s)", prev, zbncp_ll_tx_st(ll));
}

static void zbncp_ll_alert_sending(zbncp_ll_proto_t *ll)
{
    ZBNCP_DBG_TRACE("(%s : %s)", zbncp_ll_rx_st(ll), zbncp_ll_tx_st(ll));

    /* Handle timeout */
    switch (ll->tx.state)
    {
    case ZBNCP_LL_TX_STATE_SENDING_ACK:
    case ZBNCP_LL_TX_STATE_SENDING_PKT:
    case ZBNCP_LL_TX_STATE_PKT_SENT:
        ZBNCP_DBG_TRACE("    ALARM TOO FAST! Sending was not complete.");
        ll->tx.tx_attempts_cnt++;
        zbncp_ll_tx_set_state(ll, ZBNCP_LL_TX_STATE_IDLE);
        break;

    default:
        ZBNCP_DBG_TRACE("    Unexpected TX state!");
        break;
    }

    /* Note that we are in the context of zbncp_ll_poll() routine
       so we do not need to call the "callme" callback because
       we just changed TX state, which means we will return zero
       timeout from the poll routine. */
}

static void zbncp_ll_alert_wait_ack(zbncp_ll_proto_t *ll)
{
    ZBNCP_DBG_TRACE("(%s : %s)", zbncp_ll_rx_st(ll), zbncp_ll_tx_st(ll));

    zbncp_ll_change_sending_pkt_state(ll, ZBNCP_FALSE);

    /* Handle timeout */
    switch (ll->tx.state)
    {
    case ZBNCP_LL_TX_STATE_WAITING_ACK:
        ll->tx.missing_ack_cnt++;
        zbncp_ll_tx_set_state(ll, ZBNCP_LL_TX_STATE_IDLE);
        break;

    default:
        ZBNCP_DBG_TRACE("    Unexpected TX state!");
        break;
    }

    /* Note that we are in the context of zbncp_ll_poll() routine
       so we do not need to call the "callme" callback because
       we just changed TX state, which means we will return zero
       timeout from the poll routine. */
}

void zbncp_ll_construct(zbncp_ll_proto_t *ll, zbncp_transport_t *tr)
{
    zbncp_mem_zero(ll, sizeof(*ll));
    ll->tr = tr;
}

void zbncp_ll_init(zbncp_ll_proto_t *ll, const zbncp_ll_proto_cb_t *cb, zbncp_ll_time_t time)
{
    zbncp_transport_cb_t tcb;

    ll->time = 0;
    zbncp_ll_alarm_init(&ll->alarm_sending, zbncp_ll_alert_sending, ll, time);
    zbncp_ll_alarm_init(&ll->alarm_wait_ack, zbncp_ll_alert_wait_ack, ll, time);
    ll->service = ZBNCP_FALSE;
    ll->status = ZBNCP_RET_OK;

    zbncp_ll_fifo_init(&ll->tx.fifo, ll->tx.pool, ZBNCP_LL_TX_FIFO_SIZE);

    ll->rx.state = ZBNCP_LL_RX_STATE_IDLE;
    ll->rx.frameno_handled_pkt = ZBNCP_LL_NUM_RX_INIT;
    ll->rx.frameno_ack = ZBNCP_LL_NUM_RX_INIT;
    ll->rx.ackflags = ZBNCP_LL_FLAGS_NONE;
    ll->rx.received = 0u;
    zbncp_ll_pktbuf_init(&ll->rx.pkt, 0u);

    ll->tx.state = ZBNCP_LL_TX_STATE_IDLE;
    ll->tx.frameno = ZBNCP_LL_NUM_TX_INIT;
    zbncp_ll_pktbuf_init(&ll->tx.ack, 0u);

    ll->cb = *cb;

    tcb.init = zbncp_ll_init_complete;
    tcb.send = zbncp_ll_send_complete;
    tcb.recv = zbncp_ll_recv_complete;
    tcb.arg = ll;

    zbncp_transport_init(ll->tr, &tcb);
}

static inline void zbncp_ll_rx_recv_pkt(zbncp_ll_proto_t *ll, zbncp_ll_pktbuf_t *pb)
{
    zbncp_memref_t mem = zbncp_make_memref(&pb->pkt, pb->size);
    zbncp_transport_recv(ll->tr, mem);
}

static inline void zbncp_ll_rx_recv_next(zbncp_ll_proto_t *ll, zbncp_ll_pktbuf_t *pb, zbncp_size_t size)
{
    zbncp_size_t offset = pb->size;

    if ((offset + size) <= sizeof(pb->pkt))
    {
        void *ptr = zbncp_offset_ptr(&pb->pkt, offset);
        zbncp_memref_t mem = zbncp_make_memref(ptr, size);

        pb->size += size;

        zbncp_transport_recv(ll->tr, mem);
    }
    else
    {
        ZBNCP_DBG_TRACE("ERROR: size %zu too big!", size);
    }
}

static inline void zbncp_ll_tx_send_pkt(zbncp_ll_proto_t *ll, const zbncp_ll_pktbuf_t *pb)
{
    zbncp_cmemref_t mem = zbncp_make_cmemref(&pb->pkt, pb->size);
    zbncp_transport_send(ll->tr, mem);
}

static void zbncp_ll_rx_start_receiving(zbncp_ll_proto_t *ll)
{
    zbncp_ll_pktbuf_t *pb = &ll->rx.pkt;

    ZBNCP_DBG_TRACE("(%s)", zbncp_ll_rx_st(ll));

    zbncp_ll_pktbuf_init(pb, zbncp_ll_calc_total_pkt_size(0));
    zbncp_ll_rx_set_state(ll, ZBNCP_LL_RX_STATE_RECEIVING_HDR);
    zbncp_ll_rx_recv_pkt(ll, pb);
}

static void zbncp_ll_rx_resync(zbncp_ll_proto_t *ll)
{
    zbncp_ll_pktbuf_t *pb = &ll->rx.pkt;
    char *ptr = (char *) &pb->pkt;

    ZBNCP_DBG_TRACE("(%s) RESYNC!!!", zbncp_ll_rx_st(ll));

    /* Drop the first byte of the packet and receive the
     * next from the transport, then revalidate the header
     * after recv completion. */
    --pb->size;
    zbncp_mem_move(ptr, &ptr[1], pb->size);

    zbncp_ll_rx_set_state(ll, ZBNCP_LL_RX_STATE_RESYNC);
    zbncp_ll_rx_recv_next(ll, pb, 1);
}

static void zbncp_ll_rx_validate_size(zbncp_ll_proto_t *ll, zbncp_size_t expected)
{
    zbncp_size_t size = ll->rx.pkt.size;
    if (size != expected)
    {
        /* INTERNAL ERROR */
        ZBNCP_DBG_TRACE("     Internal ERROR!!! expected size %zu != pkt size %zu", expected, size);
    }
}

static void zbncp_ll_rx_validate_header(zbncp_ll_proto_t *ll)
{
    zbncp_ll_pktbuf_t *pb = &ll->rx.pkt;

    ZBNCP_DBG_TRACE("(%s) size %zu", zbncp_ll_rx_st(ll), pb->size);

    zbncp_ll_rx_validate_size(ll, ZBNCP_LL_BODY_CRC_OFFSET);

    if (pb->pkt.sign == ZBNCP_LL_SIGN)
    {
        if (zbncp_ll_hdr_crc_is_valid(&pb->pkt.hdr))
        {
            if (zbncp_ll_hdr_size_is_valid(&pb->pkt.hdr))
            {
                /* Header is OK - continue to receive body */
                zbncp_ll_rx_set_state(ll, ZBNCP_LL_RX_STATE_HDR_VALIDATED);
            }
            else
            {
                /* Invalid header length - try to receive next header */
                zbncp_ll_rx_start_receiving(ll);
            }
        }
        else
        {
            /* Bad HDR crc - scan for a valid signature */
            zbncp_ll_rx_resync(ll);
        }
    }
    else
    {
        /* Bad signature - scan for a valid signature */
        zbncp_ll_rx_resync(ll);
    }
}

static void zbncp_ll_rx_continue_receiving(zbncp_ll_proto_t *ll)
{
    zbncp_ll_pktbuf_t *pb = &ll->rx.pkt;
    zbncp_size_t bsize = zbncp_ll_hdr_body_size(&pb->pkt.hdr);

    ZBNCP_DBG_TRACE("(%s) size %zu flags", zbncp_ll_rx_st(ll), pb->size);

    if (bsize != 0u)
    {
        /* Receive the body of the packet */
        zbncp_ll_rx_set_state(ll, ZBNCP_LL_RX_STATE_RECEIVING_BODY);
        zbncp_ll_rx_recv_next(ll, pb, bsize + ZBNCP_LL_BODY_CRC_SIZE);
    }
    else
    {
        /* No body - go straight to processing ACK */
        zbncp_ll_rx_set_state(ll, ZBNCP_LL_RX_STATE_BODY_RECEIVED);
    }
}

static void zbncp_ll_rx_complete_receiving(zbncp_ll_proto_t *ll)
{
    zbncp_ll_pktbuf_t *pb = &ll->rx.pkt;
    zbncp_uint8_t rxflags = pb->pkt.hdr.flags;
    zbncp_uint8_t txflags = ZBNCP_LL_FLAG_ACK;

    ZBNCP_DBG_TRACE("(%s) rxflags %#02x", zbncp_ll_rx_st(ll), rxflags);

    if ((rxflags & ZBNCP_LL_FLAG_ACK) != ZBNCP_LL_FLAGS_NONE)
    {
        /* Try to acknowledge any pending TX packet */
        zbncp_ll_tx_complete_pkt(ll, rxflags);
        /* Recevie next packet */
        zbncp_ll_rx_set_state(ll, ZBNCP_LL_RX_STATE_IDLE);
    }
    else
    {
        zbncp_uint8_t rxframeno = zbncp_ll_flags_pkt_num(rxflags);

        /* Received packet can be a RESET response which always has 0x00 frameno and hasn't to be rejected as a duplicate */
        if ((ll->rx.frameno_handled_pkt == rxframeno) && (ll->rx.frameno_handled_pkt != 0u))
        {
            /* Duplicate - silently acknowledge and discard the packet */
            ZBNCP_DBG_TRACE("     Duplicate frame %#02x - discard", rxframeno);
        }
        else
        {
            /* Save received frame number to acknowledge received packet */
            ll->rx.frameno_ack = rxframeno;

            if (zbncp_ll_pkt_has_body(pb))
            {
                if (zbncp_ll_pkt_body_crc_is_valid(pb))
                {
                    if (zbncp_ll_hdr_type_is_supported(&pb->pkt.hdr))
                    {
                        /* It is a valid packet with a valid body - save the size
                           of the body to extract the payload later after sending
                           acknowledge packet */
                        ll->rx.received = zbncp_ll_hdr_body_size(&pb->pkt.hdr);
                    }
                    else
                    {
                        /* HDR type is not supported - silently acknowledge and discard the packet */
                        ZBNCP_DBG_TRACE("     HDR type %#02x is not supported - discard", pb->pkt.hdr.type);
                    }
                    /* Will use this frameno to detect duplicates */
                    ll->rx.frameno_handled_pkt = rxframeno;
                }
                else
                {
                    /* Body CRC invalid - ask other side to retransmit the packet */
                    txflags |= ZBNCP_LL_FLAG_RETRANSMIT;
                    ZBNCP_DBG_TRACE("     Invalid body CRC %#04x - should retransmit", pb->pkt.body.crc);
                }
            }
            else
            {
                /* Packet has no body - nothing to extract */
                ZBNCP_DBG_TRACE("     Packet without body - discard");
                /* Will use this frameno to detect duplicates */
                ll->rx.frameno_handled_pkt = rxframeno;
            }
        }

        /* Save TX flags for sending ACK or NACK */
        ll->rx.ackflags = txflags;
        zbncp_ll_rx_set_state(ll, ZBNCP_LL_RX_STATE_SEND_ACK);
    }
}

static zbncp_ll_rx_info_t zbncp_ll_rx_acknowledge_pkt(zbncp_ll_proto_t *ll, zbncp_memref_t mem)
{
    zbncp_ll_pktbuf_t *pb = &ll->rx.pkt;
    zbncp_ll_rx_info_t rx_info = {0u, 0u};

    ZBNCP_DBG_TRACE("(%s) frameno %#02x ackflags %#02x received %zu",
                    zbncp_ll_rx_st(ll), ll->rx.frameno_ack, ll->rx.ackflags, ll->rx.received);

    /* Sanity check - ackflags should have the ACK flag set */
    if ((ll->rx.ackflags & ZBNCP_LL_FLAG_ACK) != ZBNCP_LL_FLAGS_NONE)
    {
        /* Try to send ACK or NACK */
        if (zbncp_ll_is_tx_ready_send_ack(ll))
        {
            /* OK, TX will eventually send ACK/NACK which means we can return
               body data to the user and start to receive the next packet */
            /* If LL doesn't have free space to store the packet ACK isn't sent for flow control purposes */
            if (zbncp_memref_is_valid(mem))
            {
                zbncp_ll_tx_send_ack(ll, ll->rx.frameno_ack, ll->rx.ackflags);

                rx_info.rxbytes = size_min(mem.size, ll->rx.received);
                rx_info.flags = pb->pkt.hdr.flags;
                zbncp_mem_copy(mem.ptr, pb->pkt.body.data, rx_info.rxbytes);

                /* Reset saved flags/size to detect possible implementation errors */
                ll->rx.ackflags = ZBNCP_LL_FLAGS_NONE;
                ll->rx.received = 0u;

                zbncp_ll_rx_set_state(ll, ZBNCP_LL_RX_STATE_SENDING_ACK);
            }
        }
        else
        {
            /* TX is busy, we need to try to send ACK/NACK once more -
               stay in the same state */
        }
    }
    else
    {
        ZBNCP_DBG_TRACE("     INTERNAL ERROR: bad ack flags!");
        zbncp_ll_rx_set_state(ll, ZBNCP_LL_RX_STATE_ERROR);
    }

    return rx_info;
}

static zbncp_ll_rx_info_t zbncp_ll_process_rx(zbncp_ll_proto_t *ll, zbncp_memref_t mem)
{
    zbncp_ll_rx_info_t rx_info = {0u, 0u};

    switch (ll->rx.state)
    {
    case ZBNCP_LL_RX_STATE_IDLE:
        zbncp_ll_rx_start_receiving(ll);
        break;

    case ZBNCP_LL_RX_STATE_RECEIVING_HDR:
        /* DO NOTHING - wait for HDR */
        break;

    case ZBNCP_LL_RX_STATE_HDR_RECEIVED:
        zbncp_ll_rx_validate_header(ll);
        break;

    case ZBNCP_LL_RX_STATE_HDR_VALIDATED:
        zbncp_ll_rx_continue_receiving(ll);
        break;

    case ZBNCP_LL_RX_STATE_RECEIVING_BODY:
        /* DO NOTHING - wait for BODY */
        break;

    case ZBNCP_LL_RX_STATE_BODY_RECEIVED:
        zbncp_ll_rx_complete_receiving(ll);
        break;

    case ZBNCP_LL_RX_STATE_SEND_ACK:
        rx_info = zbncp_ll_rx_acknowledge_pkt(ll, mem);
        break;

    case ZBNCP_LL_RX_STATE_SENDING_ACK:
        /* DO NOTHING - wait when ACK will be sent */
        break;

    case ZBNCP_LL_RX_STATE_RESYNC:
        /* DO NOTHING - wait for next byte */
        break;

    case ZBNCP_LL_RX_STATE_ERROR:
        ZBNCP_DBG_TRACE("     RX ERROR!");
        zbncp_ll_rx_set_state(ll, ZBNCP_LL_RX_STATE_IDLE);
        break;

    default:
        ZBNCP_DBG_TRACE("     Unknown RX state %u", ll->rx.state);
        zbncp_ll_rx_set_state(ll, ZBNCP_LL_RX_STATE_IDLE);
        break;
    }

    /*
     * Static analysis tool generates false positives for MISRA Rule 2.2.
     * It claims that rx_info.flags and rx_info.rxbytes are not used. Actually, rx_info is returned from a function.
     * Mark the fields explicitly as unused to silence MISRA checker's false positive.
     */
    (void)rx_info.flags;
    (void)rx_info.rxbytes;
    return rx_info;
}

static zbncp_bool_t zbncp_ll_is_tx_ready_send_ack(zbncp_ll_proto_t *ll)
{
    zbncp_ll_pktbuf_t *ack = &ll->tx.ack;

    return (ack->size == 0u);
}

static void zbncp_ll_tx_send_ack(zbncp_ll_proto_t *ll, zbncp_uint8_t frameno, zbncp_uint8_t flags)
{
    zbncp_ll_pktbuf_t *ack = &ll->tx.ack;

    ZBNCP_DBG_TRACE("(%s)", zbncp_ll_tx_st(ll));

    /* Prepare TX packet with no body */
    zbncp_ll_pktbuf_fill(ack, frameno, flags, zbncp_cmemref_null());

    if (ll->tx.state == ZBNCP_LL_TX_STATE_IDLE)
    {
        /* If we can send ACK immediately - do it now */
        zbncp_ll_tx_start_sending(ll);
    }
    else if (ll->tx.state == ZBNCP_LL_TX_STATE_WAITING_ACK)
    {
        /* We're in the process of waiting ACK for our TX packet,
            but we've got non-ACK RX packet instead, so stop
            waiting and acknowledge incoming RX packet with
            highest priority. */
        zbncp_ll_tx_set_state(ll, ZBNCP_LL_TX_STATE_IDLE);
    }
    else
    {
        /* Otherwise wait until the previous TX operation is done.
            State machine will detect pending ACK packet by
            looking at its non-zero size. */
    }
}

static zbncp_uint8_t zbncp_ll_tx_alloc_frameno(zbncp_ll_proto_t *ll)
{
    zbncp_uint8_t prev = ll->tx.frameno;
    zbncp_uint8_t next = (prev + 1u) & ZBNCP_LL_NUM_MASK;
    if (next == ZBNCP_LL_NUM_BOOT)
    {
        /* To prevent throwing out boot indication from NCP as a duplicate
         * at a host side use packet #0 only once, then always skip it. */
        ++next;
    }
    ll->tx.frameno = next;

    return prev;
}

static zbncp_size_t zbncp_ll_tx_alloc_pkt(zbncp_ll_proto_t *ll, zbncp_ll_tx_pkt_t tx_pkt)
{
    zbncp_size_t txbytes = 0;

    if (zbncp_cmemref_is_valid(tx_pkt.mem))
    {
        zbncp_ll_pktbuf_t *pb = zbncp_ll_fifo_enqueue(&ll->tx.fifo);
        if (pb != ZBNCP_NULL)
        {
            zbncp_uint8_t frameno = zbncp_ll_tx_alloc_frameno(ll);
            zbncp_ll_pktbuf_fill(pb, frameno, tx_pkt.flags, tx_pkt.mem);
            pb->pkt_was_sent = ZBNCP_FALSE;
            ZBNCP_DBG_TRACE("(%s) size %zu frame #%x", zbncp_ll_tx_st(ll), pb->size, frameno);
            txbytes = pb->size;
        }
    }

    return txbytes;
}

static void zbncp_ll_tx_dealloc_pkt(zbncp_ll_proto_t *ll)
{
    ZBNCP_DBG_TRACE("(%s)", zbncp_ll_tx_st(ll));
    zbncp_ll_tx_set_state(ll, ZBNCP_LL_TX_STATE_IDLE);
    zbncp_ll_fifo_dequeue(&ll->tx.fifo);
}

static void zbncp_ll_tx_start_sending(zbncp_ll_proto_t *ll)
{
    /* ACK packets are TOP RIORITY, so if we have a pending ACK packet
     * then we should send it immediately, and only then we can return
     * to the packets from TX queue. */
    zbncp_ll_pktbuf_t *pb = &ll->tx.ack;
    zbncp_ll_tx_state_t state = ZBNCP_LL_TX_STATE_SENDING_ACK;
    zbncp_bool_t need_to_send_pkt = ZBNCP_FALSE;

    if (ll->tx.tx_attempts_cnt >= ZBNCP_LL_TX_ATTEMPTS)
    {
        zbncp_ll_tx_set_state(ll, ZBNCP_LL_TX_STATE_ERROR);
        zbncp_ll_alarm_cancel(&ll->alarm_sending);
        zbncp_ll_alarm_cancel(&ll->alarm_wait_ack);
        ll->status = ZBNCP_RET_TX_FAILED;
    }
    else
    {
        if (ll->tx.missing_ack_cnt >= ZBNCP_LL_WAIT_ACK_RETRY)
        {
            ll->tx.missing_ack_cnt = 0;
            zbncp_ll_fifo_dequeue(&ll->tx.fifo);
            ll->status = ZBNCP_RET_NO_ACK;
        }

        if (pb->size == 0u)
        {
            /* No ACK pending - send regular TX packet */
            pb = zbncp_ll_fifo_head(&ll->tx.fifo);
            state = ZBNCP_LL_TX_STATE_SENDING_PKT;

            if (pb != ZBNCP_NULL)
            {
                if (pb->pkt_was_sent)
                {
                    /* In this state if pkt was sent it means that it wasn't acknowledged */
                    state = ZBNCP_LL_TX_STATE_WAITING_ACK;
                    zbncp_ll_tx_set_state(ll, state);
                }
                else
                {
                    need_to_send_pkt = ZBNCP_TRUE;
                }
            }
        }
        else
        {
            need_to_send_pkt = ZBNCP_TRUE;
        }

        if ((need_to_send_pkt) && (pb != ZBNCP_NULL))
        {
            ZBNCP_DBG_TRACE("(%s) size %zu%s", zbncp_ll_tx_st(ll),
                            pb->size, ((pb == &ll->tx.ack) ? " -- ACK" : ""));

            zbncp_ll_tx_set_state(ll, state);
            zbncp_ll_alarm_set(&ll->alarm_sending, ZBNCP_LL_PKT_SEND_TIMEOUT);
            zbncp_ll_tx_send_pkt(ll, pb);
        }
    }
}

static void zbncp_ll_tx_complete_ack(zbncp_ll_proto_t *ll)
{
    ZBNCP_DBG_TRACE("(%s) (%s)", zbncp_ll_tx_st(ll), zbncp_ll_rx_st(ll));

    zbncp_ll_alarm_cancel(&ll->alarm_sending);
    ll->tx.ack.size = 0u;
    zbncp_ll_tx_set_state(ll, ZBNCP_LL_TX_STATE_IDLE);

    /* Permit RX to handle new packets only when ACK for received packet is sent */
    zbncp_ll_rx_set_state(ll, ZBNCP_LL_RX_STATE_IDLE);
}

static void zbncp_ll_tx_start_waiting_ack(zbncp_ll_proto_t *ll)
{
    zbncp_bool_t tx_ack_is_pending = (ll->tx.ack.size != 0u);

    ZBNCP_DBG_TRACE("(%s)%s (%s)", zbncp_ll_tx_st(ll),
                    (tx_ack_is_pending ? "RX wants to send ACK" : ""),
                    zbncp_ll_rx_st(ll));

    if (tx_ack_is_pending)
    {
        /* We have pending ACK to send from RX - transmit it with high priority */
        zbncp_ll_tx_set_state(ll, ZBNCP_LL_TX_STATE_IDLE);
    }
    else
    {
        zbncp_ll_tx_set_state(ll, ZBNCP_LL_TX_STATE_WAITING_ACK);
    }
}

static void zbncp_ll_tx_wait_ack(zbncp_ll_proto_t *ll)
{
    zbncp_bool_t tx_ack_is_pending = (ll->tx.ack.size != 0u);

    if (tx_ack_is_pending)
    {
        ZBNCP_DBG_TRACE("(%s) RX wants to send ACK (%s)",
                        zbncp_ll_tx_st(ll), zbncp_ll_rx_st(ll));

        /* We have pending ACK to send from RX - transmit it with high priority */
        zbncp_ll_tx_set_state(ll, ZBNCP_LL_TX_STATE_IDLE);
    }
    else
    {
        /* DO NOTHING - zbncp_ll_rx_complete_receiving() will call
           zbncp_ll_tx_complete_pkt() on ACK recv */
    }
}

static void zbncp_ll_tx_complete_pkt(zbncp_ll_proto_t *ll, zbncp_uint8_t rxflags)
{
    ZBNCP_DBG_TRACE("(%s) ACK flags %#02x", zbncp_ll_tx_st(ll), rxflags);

    if (ll->tx.state == ZBNCP_LL_TX_STATE_WAITING_ACK)
    {
        zbncp_ll_pktbuf_t *pb = zbncp_ll_fifo_head(&ll->tx.fifo);
        zbncp_uint8_t txflags = pb->pkt.hdr.flags;
        zbncp_uint8_t rxframeno = zbncp_ll_flags_ack_num(rxflags);
        zbncp_uint8_t txframeno = zbncp_ll_flags_pkt_num(txflags);

        /* Check TX pkt # */
        if (rxframeno == txframeno)
        {
            zbncp_ll_alarm_cancel(&ll->alarm_wait_ack);

            if ((rxflags & ZBNCP_LL_FLAG_RETRANSMIT) != ZBNCP_LL_FLAGS_NONE)
            {
                zbncp_ll_change_sending_pkt_state(ll, ZBNCP_FALSE);
                zbncp_ll_tx_start_sending(ll);
            }
            else
            {
                ll->tx.missing_ack_cnt = 0;
                zbncp_ll_tx_set_state(ll, ZBNCP_LL_TX_STATE_ACK_RECEIVED);
            }
        }
        else
        {
            /* ACK with unexpected frame number - silently ignore it */
            ZBNCP_DBG_TRACE("     ACK with unexpected #%x (expected #%x)", txframeno, rxframeno);
        }
    }
    else
    {
        /* Unexpected ACK - silently ignore it */
        ZBNCP_DBG_TRACE("     Unexpected ACK");
    }
}

static void zbncp_ll_change_sending_pkt_state(zbncp_ll_proto_t *ll, zbncp_bool_t state)
{
    zbncp_ll_pktbuf_t *pb = zbncp_ll_fifo_head(&ll->tx.fifo);

    if (pb != ZBNCP_NULL)
    {
        pb->pkt_was_sent = state;
    }
    else
    {
        /* FIFO is empty */
        ZBNCP_DBG_TRACE("     Packet for sending is absent");
    }
}

static zbncp_size_t zbncp_ll_process_tx(zbncp_ll_proto_t *ll, zbncp_ll_tx_pkt_t tx_pkt)
{
    zbncp_size_t txbytes = zbncp_ll_tx_alloc_pkt(ll, tx_pkt);

    switch (ll->tx.state)
    {
    case ZBNCP_LL_TX_STATE_IDLE:
        zbncp_ll_tx_start_sending(ll);
        break;

    case ZBNCP_LL_TX_STATE_SENDING_ACK:
        /* DO NOTHING - state will be changed on send completion */
        break;

    case ZBNCP_LL_TX_STATE_ACK_SENT:
        zbncp_ll_tx_complete_ack(ll);
        break;

    case ZBNCP_LL_TX_STATE_SENDING_PKT:
        /* DO NOTHING - state will be changed on send completion */
        break;

    case ZBNCP_LL_TX_STATE_PKT_SENT:
        /* Made things below since TX can go to ZBNCP_LL_TX_STATE_SENDING_ACK from here,
           but we need to store state that we've already sent pkt  */
        zbncp_ll_alarm_cancel(&ll->alarm_sending);
        zbncp_ll_alarm_set(&ll->alarm_wait_ack, ZBNCP_LL_ACK_WAIT_TIMEOUT);
        zbncp_ll_change_sending_pkt_state(ll, ZBNCP_TRUE);
        zbncp_ll_tx_start_waiting_ack(ll);
        break;

    case ZBNCP_LL_TX_STATE_WAITING_ACK:
        zbncp_ll_tx_wait_ack(ll);
        break;

    case ZBNCP_LL_TX_STATE_ACK_RECEIVED:
        zbncp_ll_tx_dealloc_pkt(ll);
        break;

    case ZBNCP_LL_TX_STATE_ERROR:
        ZBNCP_DBG_TRACE("     TX ERROR!");
        ZBNCP_DBG_TRACE("tx attempts: %d", ll->tx.tx_attempts_cnt);
        ll->status = ZBNCP_RET_TX_FAILED;
        break;

    default:
        ZBNCP_DBG_TRACE("     Unknown TX state %u", ll->tx.state);
        zbncp_ll_tx_set_state(ll, ZBNCP_LL_TX_STATE_IDLE);
        break;
    }

    return txbytes;
}

void zbncp_ll_poll(zbncp_ll_proto_t *ll, zbncp_ll_quant_t *q)
{
    zbncp_ll_time_t timeout;

    ZBNCP_DBG_TRACE("@ time %zu [ack size %zu]", q->req.time, ll->tx.ack.size);

    ll->service = ZBNCP_FALSE;
    ll->status = ZBNCP_RET_OK;

    zbncp_ll_alarm_update_time(&ll->alarm_sending, q->req.time);
    zbncp_ll_alarm_update_time(&ll->alarm_wait_ack, q->req.time);

    q->res.txbytes = zbncp_ll_process_tx(ll, q->req.tx_pkt);
    q->res.rx_info = zbncp_ll_process_rx(ll, q->req.rxmem);

    timeout = zbncp_ll_alarm_timeout(&ll->alarm_sending);
    if (timeout > zbncp_ll_alarm_timeout(&ll->alarm_wait_ack))
    {
        timeout = zbncp_ll_alarm_timeout(&ll->alarm_wait_ack);
    }
    q->res.timeout = timeout;

    zbncp_ll_alarm_check(&ll->alarm_sending);
    zbncp_ll_alarm_check(&ll->alarm_wait_ack);

    ZBNCP_DBG_TRACE("                   >>> timeout %zu%s [ack size %zu]",
                    q->res.timeout, ((ll->service == ZBNCP_TRUE) ? "/SERVICE" : ""), ll->tx.ack.size);

    if (ll->service == ZBNCP_TRUE)
    {
        q->res.timeout = 0;
    }

    q->res.status = ll->status;
}
