/**
 * @file zb_radio.c
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief
 * @version 0.1
 * @date 2023-08-24
 *
 * @copyright Copyright (c) 2023
 *
 */
//=============================================================================
//                Include
//=============================================================================
#include <stdio.h>
#include <string.h>

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_nwk.h"
#include "zb_mac.h"
#include "zb_mac_globals.h"
#include "zb_mac_transport.h"
#include "zb_secur.h"
#include "mac_internal.h"

#include "zigbee_platform.h"

#include "project_config.h"
#include "lmac15p4.h"
#include "util_list.h"
#include "log.h"
//=============================================================================
//                Private Definitions of const value
//=============================================================================
#define ZB_TRACE_FILE_ID 294

#define PHY_PIB_TURNAROUND_TIMER 192
#define PHY_PIB_CCA_DETECTED_TIME 128 // 8 symbols
#define PHY_PIB_CCA_DETECT_MODE 2
#define PHY_PIB_CCA_THRESHOLD 75
#define MAC_PIB_UNIT_BACKOFF_PERIOD 320
#define MAC_PIB_MAC_ACK_WAIT_DURATION 544 // non-beacon mode; 864 for beacon mode
#define MAC_PIB_MAC_MAX_BE 5
#define MAC_PIB_MAC_MAX_FRAME_TOTAL_WAIT_TIME 16416
#define MAC_PIB_MAC_MAX_FRAME_RETRIES 4
#define MAC_PIB_MAC_MAX_CSMACA_BACKOFFS 4
#define MAC_PIB_MAC_MIN_BE 3

#define ZB_RADIO_MAX_PSDU                    152
#define ZB_RADIO_RX_FRAME_BUFFER_NUM         16


typedef struct
{
    uint8_t *mPsdu; ///< The PSDU.

    uint16_t mLength;  ///< Length of the PSDU.
    uint8_t  mChannel; ///< Channel used to transmit/receive the frame.
    union
    {
        /**
         * Structure representing radio frame transmit information.
         */
        struct
        {
            uint8_t mMaxCsmaBackoffs; ///< Maximum number of backoffs attempts before declaring CCA failure.
            uint8_t mMaxFrameRetries; ///< Maximum number of retries allowed after a transmission failure.

            bool mIsHeaderUpdated : 1;
            bool mIsARetx : 1;             ///< Indicates whether the frame is a retransmission or not.
            bool mCsmaCaEnabled : 1;       ///< Set to true to enable CSMA-CA for this packet, false otherwise.
            bool mCslPresent : 1;          ///< Set to true if CSL header IE is present.
            bool mIsSecurityProcessed : 1; ///< True if SubMac should skip the AES processing of this frame.
        } mTxInfo;

        /**
         * Structure representing radio frame receive information.
         */
        struct
        {
            /**
             * The timestamp when the frame was received in microseconds.
             *
             * The value SHALL be the time when the SFD was received when TIME_SYNC or CSL is enabled.
             * Otherwise, the time when the MAC frame was fully received is also acceptable.
             *
             */
            uint64_t mTimestamp;

            uint32_t mAckFrameCounter; ///< ACK security frame counter (applicable when `mAckedWithSecEnhAck` is set).
            uint8_t  mAckKeyId;        ///< ACK security key index (applicable when `mAckedWithSecEnhAck` is set).
            int8_t   mRssi;            ///< Received signal strength indicator in dBm for received frames.
            uint8_t  mLqi;             ///< Link Quality Indicator for received frames.

            // Flags
            bool mAckedWithFramePending : 1; ///< This indicates if this frame was acknowledged with frame pending set.
            bool mAckedWithSecEnhAck : 1; ///< This indicates if this frame was acknowledged with secured enhance ACK.
        } mRxInfo;
    } mInfo;
} zbRadioFrame;

typedef struct
{
    utils_dlist_t       dlist;
    zbRadioFrame        frame;
} zbRadio_rxFrame_t;
#define ALIGNED_RX_FRAME_SIZE  ((sizeof(zbRadio_rxFrame_t) + 3) & 0xfffffffc)
#define TOTAL_RX_FRAME_SIZE (ALIGNED_RX_FRAME_SIZE + ZB_RADIO_MAX_PSDU)
//=============================================================================
//                Private ENUM
//=============================================================================
enum
{
    kMinChannel = 11,
    kMaxChannel = 26,
};
//=============================================================================
//                Private Struct
//=============================================================================

typedef struct _otRadio_t
{
    utils_dlist_t           rxFrameList;
    utils_dlist_t           frameList;
    uint32_t                dbgRxFrameNum;
    uint32_t                dbgFrameNum;
    uint32_t                dbgMaxAckFrameLenth;
    uint32_t                dbgMaxPendingFrameNum;

    uint8_t                 buffPool[TOTAL_RX_FRAME_SIZE * (ZB_RADIO_RX_FRAME_BUFFER_NUM + 2)];
} zbRadio_t;

//=============================================================================
//                Private Global Variables
//=============================================================================
static zbRadio_t zbRadio_var;

static uint8_t sPromiscuous = false;
static uint16_t sShortAddress = 0xFFFF;
static uint32_t sExtendAddr_0 = 0x01020304;
static uint32_t sExtendAddr_1 = 0x05060709;
static uint16_t sPANID = 0xFFFF;
static uint8_t sCoordinator = 0;
static uint8_t sCurrentChannel = kMinChannel;
static uint8_t sPendingBit = 1;
static uint8_t sIEEE_EUI64Addr[] = {0xAA, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
//=============================================================================
//                Functions
//=============================================================================
void zb_radioSendPacket(zb_bufid_t buf, zb_uint8_t wait_type)
{
    zb_time_t tx_at = 0;
    zb_uint8_t *pkt;
    zb_uint_t len;

    if (wait_type == ZB_MAC_TX_WAIT_ZGP)
    {
        zb_mcps_data_req_params_t *data_req_params = ZB_BUF_GET_PARAM(buf, zb_mcps_data_req_params_t);
        tx_at = data_req_params->src_addr.tx_at;
    }

    pkt = zb_buf_begin(buf);
    len = zb_buf_len(buf);

    if (ZB_FCF_GET_ACK_REQUEST_BIT(ZB_MAC_GET_FCF_PTR(pkt)))
    {
        lmac15p4_tx_data_send(1, pkt, len, 0x03, ZB_FCF_GET_SEQ_NUMBER(pkt));
    }
    else
    {
        lmac15p4_tx_data_send(1, pkt, len, 0x02, ZB_FCF_GET_SEQ_NUMBER(pkt));
    }

    //log_info_hexdump("Tx", pkt, len);
}
void zb_radioSetChannel(uint8_t channel)
{
    sCurrentChannel = channel;
    lmac15p4_channel_set((lmac154_channel_t)(sCurrentChannel - kMinChannel));
}

void zb_radioUpdateAddressFilter(void)
{
    sShortAddress = MAC_PIB().mac_short_address;
    sPANID = MAC_PIB().mac_pan_id;
    sCoordinator = MAC_PIB().mac_pan_coordinator;
    sPromiscuous = MAC_PIB().mac_promiscuous_mode;

    sExtendAddr_0 = (MAC_PIB().mac_extended_address[0] | MAC_PIB().mac_extended_address[1] << 8 |
                     MAC_PIB().mac_extended_address[2] << 16 | MAC_PIB().mac_extended_address[3] << 24);
    sExtendAddr_1 = (MAC_PIB().mac_extended_address[4] | MAC_PIB().mac_extended_address[5] << 8 |
                     MAC_PIB().mac_extended_address[6] << 16 | MAC_PIB().mac_extended_address[7] << 24);

    lmac15p4_address_filter_set(1, sPromiscuous, sShortAddress, sExtendAddr_0, sExtendAddr_1, sPANID, sCoordinator);
}

void zb_radioSetPendingBit(void)
{
    if (sPendingBit != 1)
    {
        sPendingBit = 1;
        lmac15p4_ack_pending_bit_set(1, sPendingBit);
    }
}
void zb_radioClearPendingBit(void)
{
    if (sPendingBit != 0)
    {
        sPendingBit = 0;
        lmac15p4_ack_pending_bit_set(1, sPendingBit);
    }
}
zb_bool_t zb_radioPendBit(void)
{
    return sPendingBit ? ZB_TRUE : ZB_FALSE;
}

void zb_radioTask(zb_system_event_t trxEvent)
{
    zb_uint8_t *b_p = NULL;
    zb_uint8_t *bptr;

    zbRadio_rxFrame_t *pframe;

    if (!(ZB_SYSTEM_EVENT_RADIO_ALL_MASK & trxEvent))
    {
        return;
    }

    if ((ZB_SYSTEM_EVENT_RADIO_TX_ALL_MASK & trxEvent))
    {
        if (trxEvent & ZB_SYSTEM_EVENT_RADIO_TX_DONE_NO_ACK_REQ)
        {
            TRANS_CTX().tx_status = 0;
            TRANS_CTX().csma_backoffs = 0;
        }
        if (trxEvent & ZB_SYSTEM_EVENT_RADIO_TX_ACKED)
        {
            TRANS_CTX().tx_status = 0;
            TRANS_CTX().csma_backoffs = 0;
            ZB_MAC_SET_ACK_RECEIVED(0);
        }
        if (trxEvent & ZB_SYSTEM_EVENT_RADIO_TX_ACKED_PD)
        {
            TRANS_CTX().tx_status = 0;
            TRANS_CTX().csma_backoffs = 0;
            ZB_MAC_SET_ACK_RECEIVED(1);
            MAC_CTX().flags.in_pending_data = ZB_TRUE;
        }
        if (trxEvent & ZB_SYSTEM_EVENT_RADIO_TX_NO_ACK || trxEvent & ZB_SYSTEM_EVENT_RADIO_TX_CCA_FAIL)
        {
            TRANS_CTX().tx_status = 1;
            TRANS_CTX().failed_tx++;
            TRANS_CTX().csma_backoffs = 0;
            MAC_CTX().retry_counter = MAC_PIB().mac_max_frame_retries;
        }

        ZB_MAC_SET_TX_INT_STATUS_BIT();
    }

    if (trxEvent & ZB_SYSTEM_EVENT_RADIO_RX_DONE )
    {
        pframe = NULL;

        b_p = ZB_RING_BUFFER_PUT_RESERVE(&MAC_CTX().mac_rx_queue);

        if (!b_p)
        {
            ZB_NOTIFY(ZB_SYSTEM_EVENT_RADIO_RX_DONE);
        }

        ZB_ENTER_CRITICAL();
        if (!utils_dlist_empty(&zbRadio_var.rxFrameList))
        {
            pframe = (zbRadio_rxFrame_t *)zbRadio_var.rxFrameList.next;
            zbRadio_var.dbgRxFrameNum --;
            utils_dlist_del(&pframe->dlist);
        }
        ZB_EXIT_CRITICAL();

        if (pframe)
        {
            /* Process Rx pakcet */
            bptr = zb_buf_initial_alloc(*b_p, (zb_uint_t)pframe->frame.mLength);
            memcpy(bptr, pframe->frame.mPsdu, pframe->frame.mLength);

            bptr[pframe->frame.mLength - 2] = pframe->frame.mInfo.mRxInfo.mLqi;
            bptr[pframe->frame.mLength - 1] = pframe->frame.mInfo.mRxInfo.mRssi;

            //log_info_hexdump("Rx", bptr, pframe->frame.mLength);

            zb_buf_set_status(*b_p, 0);

            ZB_RING_BUFFER_FLUSH_PUT(&MAC_CTX().mac_rx_queue);
            ZB_MAC_SET_RX_INT_STATUS_BIT();

            ZB_ENTER_CRITICAL();
            utils_dlist_add_tail(&pframe->dlist, &zbRadio_var.frameList);
            zbRadio_var.dbgFrameNum ++;

            if (!utils_dlist_empty(&zbRadio_var.rxFrameList))
            {
                ZB_NOTIFY(ZB_SYSTEM_EVENT_RADIO_RX_DONE);
            }
            ZB_EXIT_CRITICAL();
        }
        else
        {
            log_warn("zbRadio_var.dbgRxFrameNum %d", zbRadio_var.dbgRxFrameNum);
        }
    }
    else if (trxEvent & ZB_SYSTEM_EVENT_RADIO_RX_NO_BUFF)
    {

    }
}


static void _RxDoneEvent(uint16_t packet_length, uint8_t *rx_data_address,
                         uint8_t crc_status, uint8_t rssi, uint8_t snr)
{
    zbRadio_rxFrame_t *p = NULL;

    if (zboss_start_run == 0)
    {
        return;
    }

    if (crc_status == 0)
    {
        ZB_ENTER_CRITICAL();
        if (!utils_dlist_empty(&zbRadio_var.frameList))
        {
            p = (zbRadio_rxFrame_t *)zbRadio_var.frameList.next;
            zbRadio_var.dbgFrameNum --;
            utils_dlist_del(&p->dlist);
        }
        ZB_EXIT_CRITICAL();

        if (p)
        {
            memcpy(p->frame.mPsdu, (rx_data_address + 8), (packet_length - 9));
            p->frame.mLength = (packet_length - 13);

            p->frame.mChannel = sCurrentChannel;
            p->frame.mInfo.mRxInfo.mRssi = -rssi;
            p->frame.mInfo.mRxInfo.mLqi = ((100 - rssi) * 0xFF) / 100;

            ZB_ENTER_CRITICAL();
            utils_dlist_add_tail(&p->dlist, &zbRadio_var.rxFrameList);
            zbRadio_var.dbgRxFrameNum ++;
            if (zbRadio_var.dbgMaxPendingFrameNum < zbRadio_var.dbgRxFrameNum)
            {
                zbRadio_var.dbgMaxPendingFrameNum = zbRadio_var.dbgRxFrameNum;
            }
            ZB_EXIT_CRITICAL();
            TRANS_CTX().rx_timestamp = Timer_25us_Tick;
            ZB_NOTIFY(ZB_SYSTEM_EVENT_RADIO_RX_DONE);
        }
        else
        {
            ZB_NOTIFY(ZB_SYSTEM_EVENT_RADIO_RX_NO_BUFF);
        }
    }
}

static void _TxDoneEvent(uint32_t tx_status)
{
    if (0 == tx_status)
    {
        ZB_NOTIFY(ZB_SYSTEM_EVENT_RADIO_TX_DONE_NO_ACK_REQ);
    }
    else if (0x10 == tx_status)
    {
        ZB_NOTIFY(ZB_SYSTEM_EVENT_RADIO_TX_CCA_FAIL);
    }
    else if (0x20 == tx_status )
    {
        ZB_NOTIFY(ZB_SYSTEM_EVENT_RADIO_TX_NO_ACK);
    }
    else if (0x80 == tx_status )
    {
        ZB_NOTIFY(ZB_SYSTEM_EVENT_RADIO_TX_ACKED);
    }
    else if (0x40 == tx_status )
    {
        ZB_NOTIFY(ZB_SYSTEM_EVENT_RADIO_TX_ACKED_PD);
    }
}

void zb_radioInit(void)
{
    zbRadio_rxFrame_t *pframe = NULL;
    lmac15p4_callback_t mac_cb;

    memset(&zbRadio_var, 0, offsetof(zbRadio_t, buffPool));
    ZB_ENTER_CRITICAL();
    utils_dlist_init(&zbRadio_var.frameList);
    utils_dlist_init(&zbRadio_var.rxFrameList);

    for (int i = 0; i < ZB_RADIO_RX_FRAME_BUFFER_NUM; i ++)
    {
        pframe = (zbRadio_rxFrame_t *) (zbRadio_var.buffPool + TOTAL_RX_FRAME_SIZE * (i + 2));
        pframe->frame.mPsdu = ((uint8_t *)pframe) + ALIGNED_RX_FRAME_SIZE;
        utils_dlist_add_tail(&pframe->dlist, &zbRadio_var.frameList);
    }

    zbRadio_var.dbgFrameNum = ZB_RADIO_RX_FRAME_BUFFER_NUM;
    ZB_EXIT_CRITICAL();

    mac_cb.rx_cb = _RxDoneEvent;
    mac_cb.tx_cb = _TxDoneEvent;
    lmac15p4_cb_set(1, &mac_cb);

    //lmac15p4_init(LMAC15P4_2P4G_OQPSK);

    /* PHY PIB */
    //lmac15p4_phy_pib_set(PHY_PIB_TURNAROUND_TIMER, PHY_PIB_CCA_DETECT_MODE, PHY_PIB_CCA_THRESHOLD, PHY_PIB_CCA_DETECTED_TIME);
    /* MAC PIB */
    //lmac15p4_mac_pib_set(MAC_PIB_UNIT_BACKOFF_PERIOD, MAC_PIB_MAC_ACK_WAIT_DURATION,
    //                    MAC_PIB_MAC_MAX_BE,
    //                    MAC_PIB_MAC_MAX_CSMACA_BACKOFFS,
    //                    MAC_PIB_MAC_MAX_FRAME_TOTAL_WAIT_TIME, MAC_PIB_MAC_MAX_FRAME_RETRIES,
    //                    MAC_PIB_MAC_MIN_BE);


    lmac15p4_channel_set(LMAC154_CHANNEL_22);

    /* Auto ACK */
    lmac15p4_auto_ack_set(true);
    /* Auto State */
    //lmac15p4_auto_state_set(true);

    lmac15p4_ack_pending_bit_set(1, sPendingBit);
}

void zb_mac_transport_init(void)
{
    zb_TimerInit();
    zb_radioInit();
}