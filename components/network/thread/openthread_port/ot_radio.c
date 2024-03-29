/**
 * @file ot_radio.c
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief
 * @version 0.1
 * @date 2023-07-26
 *
 * @copyright Copyright (c) 2023
 *
 */
//=============================================================================
//                Include
//=============================================================================
#include "string.h"
#include <openthread-system.h>
#include <assert.h>
#include <openthread/config.h>
#include <openthread/link.h>
#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/time.h>
#include <openthread_port.h>


#include "common/logging.hpp"
#include "utils/code_utils.h"
#include "utils/mac_frame.h"

#include "utils/soft_source_match_table.h"
#include "util_list.h"
#include "project_config.h"
#include "lmac15p4.h"
#if OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
#include "subg_ctrl.h"
#endif
#include "log.h"
//=============================================================================
//                Private Definitions of const value
//=============================================================================
#define CCA_THRESHOLD_UNINIT (127)
#if OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
#define CCA_THRESHOLD_DEFAULT (75) // dBm  - default for 2.4GHz 802.15.4
#elif OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
#define CCA_THRESHOLD_DEFAULT (75) // 87
#endif
#define RAFAEL_RECEIVE_SENSITIVITY (100)

#define RX_CONTROL_FIELD_LENGTH (7)
#define RX_STATUS_LENGTH (5)
#define PHR_LENGTH (1)
#define CRC16_LENGTH (2)
#define RX_HEADER_LENGTH (RX_CONTROL_FIELD_LENGTH + PHR_LENGTH)
#define RX_APPEND_LENGTH (RX_STATUS_LENGTH + CRC16_LENGTH)
#define RX_LENGTH (MAX_RF_LEN + RX_HEADER_LENGTH + RX_APPEND_LENGTH)
#define FCF_LENGTH (2)

#if OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
#define PHY_PIB_TURNAROUND_TIMER 192
#define PHY_PIB_CCA_DETECTED_TIME 128 // 8 symbols
#define PHY_PIB_CCA_DETECT_MODE 2
#define PHY_PIB_CCA_THRESHOLD CCA_THRESHOLD_DEFAULT
#define MAC_PIB_UNIT_BACKOFF_PERIOD 320
#define MAC_PIB_MAC_ACK_WAIT_DURATION 544 // non-beacon mode; 864 for beacon mode
#define MAC_PIB_MAC_MAX_BE 8
#define MAC_PIB_MAC_MAX_FRAME_TOTAL_WAIT_TIME 16416
#define MAC_PIB_MAC_MAX_FRAME_RETRIES 4
#define MAC_PIB_MAC_MAX_CSMACA_BACKOFFS 10
#define MAC_PIB_MAC_MIN_BE 3
#elif OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
#define PHY_PIB_TURNAROUND_TIMER 1000
#define PHY_PIB_CCA_DETECTED_TIME 640 // 8 symbols for 50 kbps-data rate
#define PHY_PIB_CCA_DETECT_MODE 2
#define PHY_PIB_CCA_THRESHOLD CCA_THRESHOLD_DEFAULT
#define MAC_PIB_UNIT_BACKOFF_PERIOD 1640   // PHY_PIB_TURNAROUND_TIMER + PHY_PIB_CCA_DETECTED_TIME
#define MAC_PIB_MAC_ACK_WAIT_DURATION 2000 // oqpsk neee more then 2000
#define MAC_PIB_MAC_MAX_BE 5
#define MAC_PIB_MAC_MAX_FRAME_TOTAL_WAIT_TIME 82080 // for 50 kbps-data rate
#define MAC_PIB_MAC_MAX_FRAME_RETRIES 4
#define MAC_PIB_MAC_MAX_CSMACA_BACKOFFS 4
#define MAC_PIB_MAC_MIN_BE 2
#endif

#define MAC_RX_BUFFERS 100
#if OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
#define FREQ (2405)
#elif OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
#define FREQ (920000) // 915 MHZ range is (902~928), but Taiwan range is (920~925)
#define CHANNEL_SPACING 500
#endif

extern uint8_t rfb_port_ack_packet_read(uint8_t *rx_data_address);
static bool hasFramePending(const otRadioFrame *aFrame);
//=============================================================================
//                Private ENUM
//=============================================================================
#if OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
enum
{
    kMinChannel = 11,
    kMaxChannel = 26,
};
#elif OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
enum
{
    kMinChannel = 1,
    kMaxChannel = 10,
};
#endif

enum
{
    IEEE802154_MIN_LENGTH = 5,
    IEEE802154_MAX_LENGTH = 2047,
    IEEE802154_ACK_LENGTH = 5,
    IEEE802154_FRAME_TYPE_MASK = 0x7,
    IEEE802154_FRAME_TYPE_COMMAND = 0x3,
    IEEE802154_FRAME_TYPE_ACK = 0x2,
    IEEE802154_FRAME_PENDING = 1 << 4,
    IEEE802154_ACK_REQUEST = 1 << 5,
    IEEE802154_DSN_OFFSET = 2,
};

#define OTRADIO_MAC_HEADER_ACK_REQUEST_MASK (1 << 5)
#if OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
#define OTRADIO_MAX_PSDU                    (OT_RADIO_FRAME_MAX_SIZE+25)
#elif OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
#define OTRADIO_MAX_PSDU                    (OT_RADIO_FRAME_MAX_SIZE+32)
#endif
#define OTRADIO_RX_FRAME_BUFFER_NUM         16

typedef struct _otRadio_rxFrame_t
{
    utils_dlist_t       dlist;
    otRadioFrame        frame;
} otRadio_rxFrame_t;

#define ALIGNED_RX_FRAME_SIZE  ((sizeof(otRadio_rxFrame_t) + 3) & 0xfffffffc)
#define TOTAL_RX_FRAME_SIZE (ALIGNED_RX_FRAME_SIZE + OTRADIO_MAX_PSDU)
//=============================================================================
//                Private Struct
//=============================================================================
typedef struct _otRadio_t
{
    otInstance              *aInstance;
    utils_dlist_t           rxFrameList;
    utils_dlist_t           frameList;
    otRadioFrame            *pTxFrame;
    otRadioFrame            *pAckFrame;
    uint32_t                isCoexEnabled;

    uint64_t                tstx;
    uint64_t                tsIsr;

    uint32_t                dbgRxFrameNum;
    uint32_t                dbgFrameNum;
    uint32_t                dbgMaxAckFrameLenth;
    uint32_t                dbgMaxPendingFrameNum;

    uint8_t                 buffPool[TOTAL_RX_FRAME_SIZE * (OTRADIO_RX_FRAME_BUFFER_NUM + 2)];
} otRadio_t;

static otRadio_t otRadio_var;
//=============================================================================
//                Private Global Variables
//=============================================================================
static bool sIsSrcMatchEnabled = false;
static int8_t sCcaThresholdDbm = CCA_THRESHOLD_DEFAULT;
static otExtAddress sExtAddress;

static uint8_t sIEEE_EUI64Addr[OT_EXT_ADDRESS_SIZE] = {0xAA, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

static uint16_t sTurnaroundTime = PHY_PIB_TURNAROUND_TIMER;
static uint8_t sCCAMode = PHY_PIB_CCA_DETECT_MODE;
static uint8_t sCCAThreshold = PHY_PIB_CCA_THRESHOLD;
#if OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
static uint16_t sCCADuration = PHY_PIB_CCA_DETECTED_TIME;
static uint32_t sUnitBackoffPeriod = MAC_PIB_UNIT_BACKOFF_PERIOD;
static uint32_t sFrameTotalWaitTime = MAC_PIB_MAC_MAX_FRAME_TOTAL_WAIT_TIME;
#else
static uint16_t CCADuration_choices[8] = {0, 0, 0, 160, 320, 640, 128, 213};                  // use 2.4g parameter for 300k
static uint32_t FrameTotalWaitTime_choices[8] = {0, 0, 0, 20520, 41040, 82080, 16416, 27360}; // use 2.4g parameter for 300k
static uint16_t sCCADuration;
static uint32_t sUnitBackoffPeriod;
static uint32_t sFrameTotalWaitTime;
uint32_t sPhyDataRate = SUBG_CTRL_DATA_RATE_300K;
#endif

static uint16_t sMacAckWaitTime = MAC_PIB_MAC_ACK_WAIT_DURATION;
static uint8_t sMacFrameRetris = MAC_PIB_MAC_MAX_FRAME_RETRIES;

// static uint8_t sPhyTxPower = TX_POWER_20dBm;
// static uint8_t sPhyModulation = MOD_1;
static uint8_t sPromiscuous = false;
static uint16_t sShortAddress = 0xFFFF;
static uint32_t sExtendAddr_0 = 0x01020304;
static uint32_t sExtendAddr_1 = 0x05060709;
static uint16_t sPANID = 0xFFFF;
static uint8_t sCoordinator = 0;
static uint8_t sCurrentChannel = kMinChannel;
static otRadioState sState = OT_RADIO_STATE_DISABLED;

// static queue_t rf_rx_queue;

static uint32_t sMacFrameCounter;
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2

static uint8_t sKeyId;
static otMacKeyMaterial sPrevKey;
static otMacKeyMaterial sCurrKey;
static otMacKeyMaterial sNextKey;
static otRadioKeyType sKeyType;
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
static uint32_t sCslSampleTime;
static uint32_t sCslPeriod;
#endif

static bool sAuto_State_Set = false;

//=============================================================================
//                Functions
//=============================================================================

static void ReverseExtAddress(otExtAddress *aReversed, const otExtAddress *aOrigin)
{
    for (size_t i = 0; i < sizeof(*aReversed); i++)
    {
        aReversed->m8[i] = aOrigin->m8[sizeof(*aOrigin) - 1 - i];
    }
}

/* Radio Configuration */
/**
 * @brief                               Get the bus speed in bits/second between the host and radio chip
 *
 * @param aInstance                     A pointer to an OpenThread instance
 * @return uint32_t                     The bus speed in bits/second between the host and the radio chip.
 *                                      Return 0 when the MAC and above layer and Radio layer resides
 *                                      on the same chip.
 */
uint32_t otPlatRadioGetBusSpeed(otInstance *aInstance)
{
    // log_info("%s", __func__);
    OT_UNUSED_VARIABLE(aInstance);

    return 0;
}

/**
 * @brief                               Get the radio capabilities.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @return otRadioCaps                  The radio capability bit vector (see OT_RADIO_* definitions)
 */
otRadioCaps otPlatRadioGetCaps(otInstance *aInstance)
{
    // log_info("%s", __func__);
    OT_UNUSED_VARIABLE(aInstance);

    /* clang-format off */
    otRadioCaps capabilities = (OT_RADIO_CAPS_ACK_TIMEOUT |
                                OT_RADIO_CAPS_TRANSMIT_RETRIES |
                                OT_RADIO_CAPS_SLEEP_TO_TX |
                                OT_RADIO_CAPS_CSMA_BACKOFF
                               );

    return capabilities;
}

/**
 * @brief                               Get the radio's CCA ED threshold in dBm measured at antenna connector
 *                                      per IEEE 802.15.4 - 2015 section 10.1.4
 * @param aInstance                     The OpenThread instance structure.
 * @param aThreshold                    The CCA ED threshold in dBm.
 * @return otError                      OT_ERROR_NONE               Successfully retrieved the CCA ED threshold.
 *                                      OT_ERROR_INVALID_ARGS       aThreasold was NULL.
 *                                      OT_ERROR_NOT_IMPLEMENTED    CCA ED threshold configuration via dBm is
 *                                                                  not implemented.
 */
otError otPlatRadioGetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t *aThreshold)
{
    // log_info("%s", __func__);

    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;
    otEXPECT_ACTION(aThreshold != NULL, error = OT_ERROR_INVALID_ARGS);

    *aThreshold = -(sCcaThresholdDbm);

exit:
    return error;
}

/**
 * @brief                               Get the external FEM's Rx LNA gain in dBm.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @param aGain                         The external FEM's Rx LNA gain in dBm.
 * @return otError                      OT_ERROR_NONE               Successfully retrieved the external FEM's LNA gain.
 *                                      OT_ERROR_INVALID_ARGS       aGain was NULL.
 *                                      OT_ERROR_NOT_IMPLEMENTED    External FEM's LNA setting is
 *                                                                  not implemented.
 */
otError otPlatRadioGetFemLnaGain(otInstance *aInstance, int8_t *aGain)
{
    OT_UNUSED_VARIABLE(aInstance);

    return OT_ERROR_NOT_IMPLEMENTED;
}

/**
 * @brief                               Get the factory-assigned IEEE EUI-64 for this interface.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @param aIeeeEui64                    A pointer to the factory-assigned IEEE EUI-64.
 */
void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
    OT_UNUSED_VARIABLE(aInstance);

    memcpy(aIeeeEui64, sIEEE_EUI64Addr, OT_EXT_ADDRESS_SIZE);
}

/**
 * @brief                               Get the current estimated time(in microseconds) of the radio chip.
 *
 *                                      This microsecond timer must be a free-running timer. The timer must
 *                                      continue to advance with microsecond presision even when the radio is
 *                                      in the sleep state.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @return uint64_t                     The current time in microseconds. UINT64_MAX when platform does not
 *                                      support or radio time is not ready.
 */
uint64_t otPlatRadioGetNow(otInstance *aInstance)
{
    // log_info("%s", __func__);
    OT_UNUSED_VARIABLE(aInstance);

    return otPlatTimeGet();
}

/**
 * Get current state of the radio.
 *
 * This function is not required by OpenThread. It may be used for debugging and/or application-specific purposes.
 *
 * @note This function may be not implemented. It does not affect OpenThread.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @return  Current state of the radio.
 *
 */
otRadioState otPlatRadioGetState(otInstance *aInstance)
{
    // log_info("%s", __func__);
    OT_UNUSED_VARIABLE(aInstance);


    return sState;
}

/**
 * @brief                               Get the status of promiscuous mode.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @return true                         Promiscuous mode is enabled.
 * @return false                        Promiscuous mode is disable.
 */
bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
    // log_info("%s", __func__);
    OT_UNUSED_VARIABLE(aInstance);


    return sPromiscuous;
}

/**
 * @brief                               Get the radio receive sensitivity value.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @return int8_t                       The radio receive sensitivity value in dBm.
 */
int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
    // log_info("%s", __func__);
    OT_UNUSED_VARIABLE(aInstance);


    return -(RAFAEL_RECEIVE_SENSITIVITY);
}

/**
 * @brief                               Get the radio's transmit power in dBm.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @param aPower                        The transmit power in dBm.
 * @return otError                      OT_ERROR_NONE               Successfully retrieved the transmit power.
 *                                      OT_ERROR_INVALID_ARGS       aPower was NULL.
 *                                      OT_ERROR_NOT_IMPLEMENTED    Transmit power configuration via
 *                                                                  dBm is not implemented.
 */
otError otPlatRadioGetTransmitPower(otInstance *aInstance, int8_t *aPower)
{
    // log_info("%s", __func__);
    OT_UNUSED_VARIABLE(aInstance);


    otError error = OT_ERROR_NONE;

    error = OT_ERROR_NOT_IMPLEMENTED;

    otEXPECT_ACTION(aPower != NULL, error = OT_ERROR_INVALID_ARGS);

exit:
    return error;
}

/**
 * @brief                               Set the radio's CCA ED threshold in dBm measured at antenna
 *                                      connector per IEEE 802.15.4-2015 section 10.1.4.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @param aThreshold                    The CCA ED threshold in dBm.
 * @return otError                      OT_ERROR_NONE               Successfully retrieved the CCA ED threshold.
 *                                      OT_ERROR_INVALID_ARGS       Given threshold is out of range.
 *                                      OT_ERROR_NOT_IMPLEMENTED    CCA ED threshold configuration via
 *                                                                  dBm is not implemented.
 */
otError otPlatRadioSetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t aThreshold)
{
    // log_info("%s", __func__);
    OT_UNUSED_VARIABLE(aInstance);

    sCCAThreshold = aThreshold;

    lmac15p4_phy_pib_set(sTurnaroundTime, sCCAMode, sCCAThreshold, sCCADuration);

    return OT_ERROR_NONE;
}

/**
 * @brief                               Set the Extended Address for address filtering.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @param aAddress                      A pointer to the IEEE 802.15.4 Extended Address stored in
 *                                      little-endian byte order.
 */
void otPlatRadioSetExtendedAddress(otInstance *aInstance, const otExtAddress *aAddress)
{
    // log_info("%s", __func__);
    OT_UNUSED_VARIABLE(aInstance);


    ReverseExtAddress(&sExtAddress, aAddress);


    sExtendAddr_0 = (aAddress->m8[0] | (aAddress->m8[1] << 8) | (aAddress->m8[2] << 16) | (aAddress->m8[3] << 24));
    sExtendAddr_1 = (aAddress->m8[4] | (aAddress->m8[5] << 8) | (aAddress->m8[6] << 16) | (aAddress->m8[7] << 24));

    lmac15p4_address_filter_set(0, sPromiscuous, sShortAddress, sExtendAddr_0, sExtendAddr_1, sPANID, sCoordinator);
}

/**
 * @brief                               Set the external FEM's Rx LNA gain in dBm.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @param aGain                         The external FEM's RX LNA gain in dBm.
 * @return otError                      OT_ERROR_NONE               Successfully set the external FEM's RX LNA gain.
 *                                      OT_ERROR_INVALID_ARGS       Given gain is out of range.
 *                                      OT_ERROR_NOT_IMPLEMENTED    External FEM's Rx LNA gain
 *                                                                  setting is not implemented.
 */
otError otPlatRadioSetFemLnaGain(otInstance *aInstance, int8_t aGain)
{
    // log_info("%s", __func__);
    OT_UNUSED_VARIABLE(aInstance);


    assert(aInstance != NULL);

    return OT_ERROR_NOT_IMPLEMENTED;
}

/**
 * @brief                               Set the PAN ID for address filitering.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @param aPanId                        The IEEE 802.15.4 PAN ID.
 */
void otPlatRadioSetPanId(otInstance *aInstance, uint16_t aPanId)
{
    // log_info("%s", __func__);
    OT_UNUSED_VARIABLE(aInstance);

    sPANID = aPanId;

    utilsSoftSrcMatchSetPanId(aPanId);


    lmac15p4_address_filter_set(0, sPromiscuous, sShortAddress, sExtendAddr_0, sExtendAddr_1, sPANID, sCoordinator);
}

/**
 * @brief                               Enable or disable promiscuous mode.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @param aEnable                       TURE to enable or FALSE to disable promiscuous mode.
 */
void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    // log_info("%s", __func__);
    OT_UNUSED_VARIABLE(aInstance);


    sPromiscuous = aEnable;

    lmac15p4_address_filter_set(0, sPromiscuous, sShortAddress, sExtendAddr_0, sExtendAddr_1, sPANID, sCoordinator);
}

/**
 * @brief                               Set the Short Address for address filitering.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @param aAddress                      The IEEE 802.15.4 Short Address.
 */
void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t aAddress)
{
    // log_info("%s", __func__);
    OT_UNUSED_VARIABLE(aInstance);


    sShortAddress = aAddress;

    lmac15p4_address_filter_set(0, sPromiscuous, sShortAddress, sExtendAddr_0, sExtendAddr_1, sPANID, sCoordinator);
}

/**
 * @brief                               Set the radio's transmit power in dBm.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @param aPower                        The transimit power in dBm.
 * @return otError                      OT_ERROR_NONE               Successfully set the transmit power.
 *                                      OT_ERROR_INVALID_ARGS       aPower is out of range.
 *                                      OT_ERROR_NOT_IMPLEMENTED    Transmit power configuration via
 *                                                                  dBm is not implemented.
 */
otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower)
{
    // log_info("%s", __func__);
    OT_UNUSED_VARIABLE(aInstance);


    return OT_ERROR_NOT_IMPLEMENTED;
}

/* Radio Operation */

/**
 * @brief                               Disable the radio.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @return otError                      OT_ERROR_NONE               Successfully transitioned to Disable.
 *                                      OT_ERROR_INVALID_STATE      The radio was not in sleep state.
 */
otError otPlatRadioDisable(otInstance *aInstance)
{
    // log_info("%s", __func__);
    OT_UNUSED_VARIABLE(aInstance);

    if (otPlatRadioIsEnabled(aInstance))
    {
        otLogDebgPlat("State=OT_RADIO_STATE_DISABLED");
        sState = OT_RADIO_STATE_DISABLED;
    }

    return OT_ERROR_NONE;
}

/**
 * @brief                               Enable the radio.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @return otError                      OT_ERROR_NONE               Successfully transitioned to enable.
 *                                      OT_ERROR_INVALID_FAILED     The radio could not be enabled.
 */
otError otPlatRadioEnable(otInstance *aInstance)
{
    // log_info("%s", __func__);

    if (!otPlatRadioIsEnabled(aInstance))
    {
        otLogDebgPlat("State=OT_RADIO_STATE_SLEEP");
        sState = OT_RADIO_STATE_SLEEP;
    }

    return OT_ERROR_NONE;
}

/**
 * @brief                               Enable/Disable source address match feature.
 *
 *                                      The source address match feature controls how the radio layer descides
 *                                      the "frame pending" bit for acks sent in response to data request commands
 *                                      from children.
 *
 *                                      If disable, the radio layer must set the "frame panding" on all acks to
 *                                      data request commands.
 *
 *                                      If enable, the radio layer uses the source address match table to determine
 *                                      whether to set or clear the "frame pending" bit in an ack to data request command.
 *
 *                                      The source address match table provide the list of children for which there is a
 *                                      pending frame. Either a short address or an extended/long address can be added
 *                                      to the source address match table.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @param aEnable                       Enable/disable source address match feature.
 */
void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
    // log_info("%s", __func__);
    OT_UNUSED_VARIABLE(aInstance);
    // set Frame Pending bit for all outgoing ACKs if aEnable is false
    sIsSrcMatchEnabled = aEnable;

    lmac15p4_src_match_ctrl(0, aEnable);
}

/**
 * @brief                               Biging the energy scan sequence on the radio.
 *                                      The function is used when radio provides OT_RADIO_CAPS_ENERGY_SCAN capability.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @param aScanChannel                  The channel to perform the energy scan on.
 * @param aScanDuration                 The duration, in milliseconds, for the channel to be scanned.
 * @return otError                      OT_ERROR_NONE               Successfully started scanning the channel.
 *                                      OT_ERROR_NOT_IMPLEMENTED    The radio doesn't support energy scanning.
 */
otError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
    // log_info("%s", __func__);
    OT_UNUSED_VARIABLE(aInstance);


    int8_t rssi_value;
    //if (aScanChannel != sCurrentChannel)
    {
        sCurrentChannel        = aScanChannel;
        lmac15p4_channel_set((lmac154_channel_t)(sCurrentChannel - kMinChannel));
    }

    rssi_value = lmac15p4_read_rssi();

    otPlatRadioEnergyScanDone(aInstance, -rssi_value);

    return OT_ERROR_NONE;
}

/**
 * @brief                               Get the most recent RSSI measurement.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @return int8_t                       The RSSI in dBm when it is valid. 127 when RSSI is invalid.
 */
int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
    int8_t   rssi = OT_RADIO_RSSI_INVALID;

    OT_UNUSED_VARIABLE(aInstance);

    rssi = lmac15p4_read_rssi();

    return -rssi;
}


/**
 * @brief                               Get the radio transmit frame buffer.
 *
 *                                      OpenThread forms the IEEE 802.15.4 frame in this buffer then calls
 *                                      otPlatRadioTransmit() to request transmission.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @return otRadioFrame*                A pointer ti the transimit frame buffer.
 */
otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
    // log_info("%s", __func__);
    otRadio_var.aInstance = aInstance;


    otRadioFrame *txframe = (otRadioFrame *)otRadio_var.buffPool;
    txframe->mPsdu = otRadio_var.buffPool + ALIGNED_RX_FRAME_SIZE;

    return txframe;
}

/**
 * @brief                               Check whether radio is enabled or not.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @return true                         If the radio is enabled.
 * @return false                        If the radio is not enabled.
 */
bool otPlatRadioIsEnabled(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);


    return (sState != OT_RADIO_STATE_DISABLED) ? true : false;
}

/**
 * @brief                               Transition the radio from Sleep to Receive(turn on the radio)
 *
 * @param aInstance                     The OpenThread instance structure.
 * @param aChannel                      The channel to use for receiving.
 * @return otError                      OT_ERROR_NONE               Successfully transitioned to Receive.
 *                                      OT_ERROR_INVALID_STATE      The radio was disabled or transmiting.
 */
otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
    OT_UNUSED_VARIABLE(aInstance);


    assert(aInstance != NULL);

    // log_info("%s", __func__);


    //if (sState != OT_RADIO_STATE_DISABLED)
    {
        //log_info("state %d", sState);
        //otLogDebgPlat("State=OT_RADIO_STATE_RECEIVE");
        sState = OT_RADIO_STATE_RECEIVE;

        /* Enable RX and leave SLEEP */
        if (sAuto_State_Set != true)
        {
            lmac15p4_auto_state_set(true);

            sAuto_State_Set = true;
        }

        //if (aChannel != sCurrentChannel)
        {
            sCurrentChannel = aChannel;
#if OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
            lmac15p4_channel_set((lmac154_channel_t)(sCurrentChannel - kMinChannel));
#elif OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
            subg_ctrl_frequency_set(FREQ + (CHANNEL_SPACING * (sCurrentChannel - kMinChannel)));
#endif
        }


    }

    return OT_ERROR_NONE;
}


/**
 * @brief                               Transition the radio from Receive to Sleep (turn off the radio)
 *
 * @param aInstance                     The OpenThread instance structure.
 * @return otError                      OT_ERROR_NONE               Successfully transitioned to Sleep.
 *                                      OT_ERROR_BUSY               The radio was transmiting.
 *                                      OT_ERROR_INVALID_STATE      The radio was disabled.
 */
otError otPlatRadioSleep(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    // log_info("%s", __func__);

    otError error = OT_ERROR_INVALID_STATE;

    //if (sState == OT_RADIO_STATE_SLEEP || sState == OT_RADIO_STATE_RECEIVE)
    {
        otLogDebgPlat("State=OT_RADIO_STATE_SLEEP");
        error  = OT_ERROR_NONE;
        sState = OT_RADIO_STATE_SLEEP;
        /* Disable RX, and enter sleep*/
        if (sAuto_State_Set != false)
        {
            lmac15p4_auto_state_set(false);
            sAuto_State_Set = false;
        }
    }
    return error;
}

/**
 * @brief                               Begin the transmit sequence on the radio.
 *
 *                                      The caller must from the IEEE 802.15.4 frame in the buffer provided by
 *                                      otPlatRadioTransmitBuffer() before requesting transmission. The channel
 *                                      and transmit power are also include in otRadioFrame structure.
 *
 *                                      The transmit sequence consists of:
 *                                          1. Transitioning the radio to Transmit from on of following state:
 *                                              * Receive if Rx is on when the device idle or OT_RADIO_CAPS_SLEEP_TO_TX
 *                                                is not supported.
 *                                              * Sleep if Rx is off when the device is idle or OT_RADIO_CAPS_SLEEP_TO_TX
 *                                                is supported.
 *                                          2. Transmits the psdu on the given channel and at the given transmit power.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @param aFrame                        A pointer to the frame to be transmitted.
 * @return otError                      OT_ERROR_NONE               Successfully transitioned to Transmit.
 *                                      OT_ERROR_INVALID_STATE      The radio was not in Receive state.
 */
otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame)
{
    // log_info("%s", __func__);
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aFrame);

    assert(aInstance != NULL);
    assert(aFrame != NULL);

    uint8_t tx_control = 0;
    //uint8_t temp[OT_RADIO_FRAME_MAX_SIZE + 4];

    otError error = OT_ERROR_INVALID_STATE;


    if (otRadio_var.pTxFrame == NULL)
    {
        otRadio_var.pTxFrame = aFrame;
        //if (sCurrentChannel != aFrame->mChannel)
        {
            sCurrentChannel = aFrame->mChannel;
#if OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
            lmac15p4_channel_set((lmac154_channel_t)(sCurrentChannel - kMinChannel));
#elif OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
            subg_ctrl_frequency_set(FREQ + (CHANNEL_SPACING * (sCurrentChannel - kMinChannel)));
#endif
        }
    }
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    if (sCslPeriod > 0 && !sTransmitFrame.mInfo.mTxInfo.mIsHeaderUpdated)
    {
        otMacFrameSetCslIe(&sTransmitFrame, (uint16_t)sCslPeriod, getCslPhase());
    }
#endif
    if ((otRadio_var.pTxFrame->mPsdu[0] & IEEE802154_ACK_REQUEST))
    {
        tx_control = 0x03;
    }
    else
    {
        tx_control = 0x02;
    }

    if ((otRadio_var.pTxFrame->mPsdu[0] & IEEE802154_FRAME_TYPE_COMMAND))
    {
        tx_control ^= (1 << 1);
    }
    // tx_control |= (1 << 2 );
    // if (otMacFrameIsSecurityEnabled(aFrame))
    // {
    //     // uint32_t temp_fc; //=spRFBCtrl->frame_counter_get();

    //     // if (sMacFrameCounter < temp_fc)
    //     // {
    //     //     sMacFrameCounter = temp_fc + 1;
    //     // }
    // }
    // temp[0] = ((sMacFrameCounter >> 0) & 0xFF);
    // temp[1] = ((sMacFrameCounter >> 8) & 0xFF);
    // temp[2] = ((sMacFrameCounter >> 16) & 0xFF);
    // temp[3] = ((sMacFrameCounter >> 24) & 0xFF);

    // memcpy(temp + 4, otRadio_var.pTxFrame->mPsdu, otRadio_var.pTxFrame->mLength);
    // if (otRadio_var.pTxFrame->mInfo.mTxInfo.mTxDelay != 0)
    // {
    //     tx_control ^= (1 << 1);
    // }

    Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_RESERVED4);

    if (lmac15p4_tx_data_send(0, aFrame->mPsdu,
                              aFrame->mLength - 2,
                              tx_control,
                              otMacFrameGetSequence(aFrame)) != 0)
    {
        Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_RESERVED4);
        OT_NOTIFY(OT_SYSTEM_EVENT_RADIO_TX_NO_ACK);
    }

    otPlatRadioTxStarted(aInstance, aFrame);
    otRadio_var.tstx = otPlatTimeGet() + 1;
    error = OT_ERROR_NONE;
    return error;
}
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
otError otPlatRadioEnableCsl(otInstance         *aInstance,
                             uint32_t            aCslPeriod,
                             otShortAddress      aShortAddr,
                             const otExtAddress *aExtAddr)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aShortAddr);
    OT_UNUSED_VARIABLE(aExtAddr);

    otError error = OT_ERROR_NONE;

    sCslPeriod = aCslPeriod;
    if (sCslPeriod > 0)
    {
        // spRFBCtrl->csl_receiver_ctrl(1, sCslPeriod);
    }
    return error;
}

void otPlatRadioUpdateCslSampleTime(otInstance *aInstance, uint32_t aCslSampleTime)
{
    OT_UNUSED_VARIABLE(aInstance);

    sCslSampleTime = aCslSampleTime;
    // spRFBCtrl->csl_sample_time_update(sCslSampleTime);
}



#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
uint8_t otPlatRadioGetCslAccuracy(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    //return spRFBCtrl->csl_accuracy_get();
    return 0;
}

uint8_t otPlatRadioGetCslUncertainty(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    //return spRFBCtrl->csl_uncertainty_get();
    return 0;
}
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
void otPlatRadioSetMacKey(otInstance             *aInstance,
                          uint8_t                 aKeyIdMode,
                          uint8_t                 aKeyId,
                          const otMacKeyMaterial *aPrevKey,
                          const otMacKeyMaterial *aCurrKey,
                          const otMacKeyMaterial *aNextKey,
                          otRadioKeyType          aKeyType)
{
    // log_info("%s", __func__);
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aKeyIdMode);


    otEXPECT(aPrevKey != NULL && aCurrKey != NULL && aNextKey != NULL);

    sKeyId   = aKeyId;
    sKeyType = aKeyType;
    memcpy(&sPrevKey, aPrevKey, sizeof(otMacKeyMaterial));
    memcpy(&sCurrKey, aCurrKey, sizeof(otMacKeyMaterial));
    memcpy(&sNextKey, aNextKey, sizeof(otMacKeyMaterial));

    lmac15p4_key_set(0, sCurrKey.mKeyMaterial.mKey.m8);
exit:
    return;
}

/**
 * @brief                               The method sets the current MAC frame counter value.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @param aMacFrameCounter              The MAC frame counter value.
 */
void otPlatRadioSetMacFrameCounter(otInstance *aInstance, uint32_t aMacFrameCounter)
{
    OT_UNUSED_VARIABLE(aInstance);


    sMacFrameCounter = aMacFrameCounter;
}
#endif

/**
 * @brief                               The method get the cca value.
 */
otError otPlatRadioGetCca(otInstance *aInstance, int8_t *aThreshold, uint16_t *turnaroundtime, uint16_t *duration)
{
    *aThreshold = -(sCCAThreshold);
    *turnaroundtime = sTurnaroundTime;
    *duration = sCCADuration;


    return OT_ERROR_NONE;
}

/**
 * @brief                               The method sets the cca value.
 * @return otError                                          OT_ERROR_NONE               Successfully set the transmit power.
 *                                      OT_ERROR_INVALID_ARGS       cca value is error.
 */
otError otPlatRadioSetCca(otInstance *aInstance, int8_t aThreshold, uint16_t turnaroundtime, uint16_t duration)
{
    // log_info("%s", __func__);

    if (aThreshold <= 0)
    {
        return OT_ERROR_INVALID_ARGS;
    }
    if (turnaroundtime <= 0)
    {
        return OT_ERROR_INVALID_ARGS;
    }
    if (duration <= 0)
    {
        return OT_ERROR_INVALID_ARGS;
    }
    sCCAThreshold = aThreshold;
    sTurnaroundTime = turnaroundtime;
    sCCADuration = duration;
    lmac15p4_phy_pib_set(sTurnaroundTime, sCCAMode, sCCAThreshold, sCCADuration);
    return OT_ERROR_NONE;
}

#if 0
static otError radioProcessTransmitSecurity(otRadioFrame *aFrame)
{
    otError error = OT_ERROR_NONE;
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
    otMacKeyMaterial *key = NULL;
    uint8_t           keyId;

    otEXPECT(otMacFrameIsSecurityEnabled(aFrame) && otMacFrameIsKeyIdMode1(aFrame) &&
             !aFrame->mInfo.mTxInfo.mIsSecurityProcessed);

    if (otMacFrameIsAck(aFrame))
    {
        keyId = otMacFrameGetKeyId(aFrame);

        otEXPECT_ACTION(keyId != 0, error = OT_ERROR_FAILED);

        if (keyId == sKeyId)
        {
            key = &sCurrKey;
        }
        else if (keyId == sKeyId - 1)
        {
            key = &sPrevKey;
        }
        else if (keyId == sKeyId + 1)
        {
            key = &sNextKey;
        }
        else
        {
            error = OT_ERROR_SECURITY;
            otEXPECT(false);
        }
    }
    else
    {
        key   = &sCurrKey;
        keyId = sKeyId;
    }

    aFrame->mInfo.mTxInfo.mAesKey = key;

    if (!aFrame->mInfo.mTxInfo.mIsHeaderUpdated)
    {
        otMacFrameSetKeyId(aFrame, keyId);
        otMacFrameSetFrameCounter(aFrame, sMacFrameCounter++);
    }
#else
    otEXPECT(!aFrame->mInfo.mTxInfo.mIsSecurityProcessed);
#endif // OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2

    otMacFrameProcessTransmitAesCcm(aFrame, &sExtAddress);

exit:
    return error;
}
#endif
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
static uint16_t getCslPhase(void)
{
    uint32_t curTime       = otPlatAlarmMicroGetNow();//rfb_port_rtc_time_read();
    uint32_t cslPeriodInUs = sCslPeriod * OT_US_PER_TEN_SYMBOLS;
    uint32_t diff = ((sCslSampleTime % cslPeriodInUs) - (curTime % cslPeriodInUs) + cslPeriodInUs) % cslPeriodInUs;

    return (uint16_t)(diff / OT_US_PER_TEN_SYMBOLS);
}
#endif

#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
otError otPlatRadioConfigureEnhAckProbing(otInstance          *aInstance,
        otLinkMetrics        aLinkMetrics,
        const otShortAddress aShortAddress,
        const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);
    if ((aLinkMetrics.mLqi + aLinkMetrics.mLinkMargin + aLinkMetrics.mRssi) < 3 )
    {
        uint8_t temp_extaddr[OT_EXT_ADDRESS_SIZE];
        for (uint8_t i = 0; i < OT_EXT_ADDRESS_SIZE; i++)
        {
            temp_extaddr[i] = aExtAddress->m8[7 - i];
        }
        if (aLinkMetrics.mLqi)
        {
            // spRFBCtrl->short_addr_ctrl(3, &aShortAddress);
            // spRFBCtrl->extend_addr_ctrl(3, temp_extaddr);
        }
        else
        {
            // spRFBCtrl->short_addr_ctrl(4, &aShortAddress);
            // spRFBCtrl->extend_addr_ctrl(4, temp_extaddr);
        }

        if (aLinkMetrics.mLinkMargin)
        {
            // spRFBCtrl->short_addr_ctrl(5, &aShortAddress);
            // spRFBCtrl->extend_addr_ctrl(5, temp_extaddr);
        }
        else
        {
            // spRFBCtrl->short_addr_ctrl(6, &aShortAddress);
            // spRFBCtrl->extend_addr_ctrl(6, temp_extaddr);
        }

        if (aLinkMetrics.mRssi)
        {
            // spRFBCtrl->short_addr_ctrl(7, &aShortAddress);
            // spRFBCtrl->extend_addr_ctrl(7, temp_extaddr);
        }
        else
        {
            //spRFBCtrl->short_addr_ctrl(8, &aShortAddress);
            //spRFBCtrl->extend_addr_ctrl(8, temp_extaddr);
        }
    }

    return otLinkMetricsConfigureEnhAckProbing(aShortAddress, aExtAddress, aLinkMetrics);
}
#endif

static bool hasFramePending(const otRadioFrame *aFrame)
{
    bool         rval = false;
    otMacAddress src;

    otEXPECT_ACTION(sIsSrcMatchEnabled, rval = true);
    otEXPECT(otMacFrameGetSrcAddr(aFrame, &src) == OT_ERROR_NONE);

    switch (src.mType)
    {
    case OT_MAC_ADDRESS_TYPE_SHORT:
        rval = utilsSoftSrcMatchShortFindEntry(src.mAddress.mShortAddress) >= 0;
        break;
    case OT_MAC_ADDRESS_TYPE_EXTENDED:
    {
        otExtAddress extAddr;

        ReverseExtAddress(&extAddr, &src.mAddress.mExtAddress);
        rval = utilsSoftSrcMatchExtFindEntry(&extAddr) >= 0;
        break;
    }
    default:
        break;
    }

exit:
    return rval;
}
void ot_radioTask(ot_system_event_t trxEvent)
{
    otRadio_rxFrame_t *pframe;
    otRadioFrame        *txframe;

    if (!(OT_SYSTEM_EVENT_RADIO_ALL_MASK & trxEvent))
    {
        return;
    }

    if (otRadio_var.pTxFrame)
    {
        if ((OT_SYSTEM_EVENT_RADIO_TX_ALL_MASK & trxEvent))
        {
            txframe = otRadio_var.pTxFrame;
            otRadio_var.pTxFrame = NULL;
            otRadio_var.tstx = 0;

            if (trxEvent & OT_SYSTEM_EVENT_RADIO_TX_DONE_NO_ACK_REQ)
            {
                otPlatRadioTxDone(otRadio_var.aInstance, txframe, NULL, OT_ERROR_NONE);
            }
            if (trxEvent & OT_SYSTEM_EVENT_RADIO_TX_ACKED)
            {
                otPlatRadioTxDone(otRadio_var.aInstance, txframe, otRadio_var.pAckFrame, OT_ERROR_NONE);
            }
            if (trxEvent & OT_SYSTEM_EVENT_RADIO_TX_NO_ACK)
            {
                otPlatRadioTxDone(otRadio_var.aInstance, txframe, NULL, OT_ERROR_NO_ACK);
                // log_warn_hexdump("Tx No ACK", txframe->mPsdu, txframe->mLength);
            }
            if (trxEvent & OT_SYSTEM_EVENT_RADIO_TX_CCA_FAIL)
            {
                otPlatRadioTxDone(otRadio_var.aInstance, txframe, NULL, OT_ERROR_CHANNEL_ACCESS_FAILURE);
                // log_warn_hexdump("Tx CCA Fail", txframe->mPsdu, txframe->mLength);
            }
        }
    }

    if (trxEvent & OT_SYSTEM_EVENT_RADIO_RX_DONE )
    {

        pframe = NULL;
        OT_ENTER_CRITICAL();
        if (!utils_dlist_empty(&otRadio_var.rxFrameList))
        {
            pframe = (otRadio_rxFrame_t *)otRadio_var.rxFrameList.next;
            otRadio_var.dbgRxFrameNum --;
            utils_dlist_del(&pframe->dlist);
        }
        OT_EXIT_CRITICAL();

        if (pframe)
        {
            otPlatRadioReceiveDone(otRadio_var.aInstance, &pframe->frame, OT_ERROR_NONE);

            OT_ENTER_CRITICAL();
            utils_dlist_add_tail(&pframe->dlist, &otRadio_var.frameList);
            otRadio_var.dbgFrameNum ++;

            if (!utils_dlist_empty(&otRadio_var.rxFrameList))
            {
                OT_NOTIFY(OT_SYSTEM_EVENT_RADIO_RX_DONE);
            }
            OT_EXIT_CRITICAL();
        }
        else
        {
            // log_warn("otRadio_var.dbgRxFrameNum %d", otRadio_var.dbgRxFrameNum);
        }
    }
    else if (trxEvent & OT_SYSTEM_EVENT_RADIO_RX_NO_BUFF)
    {
        otPlatRadioReceiveDone(otRadio_var.aInstance, NULL, OT_ERROR_NO_BUFS);
    }
}

#if 1
/**
 * @brief
 *
 * @param u8_tx_status
 */
static void _TxDoneEvent(uint32_t tx_status)
{
    uint32_t ack_nowTime;

    if (otRadio_var.pTxFrame)
    {
        if (0 == tx_status)
        {
            OT_NOTIFY(OT_SYSTEM_EVENT_RADIO_TX_DONE_NO_ACK_REQ);
        }
        else if (0x10 == tx_status)
        {
            OT_NOTIFY(OT_SYSTEM_EVENT_RADIO_TX_CCA_FAIL);
        }
        else if (0x20 == tx_status )
        {
            OT_NOTIFY(OT_SYSTEM_EVENT_RADIO_TX_NO_ACK);
        }
        else if (0x40 == tx_status || 0x80 == tx_status )
        {
            lmac15p4_read_ack((uint8_t *)otRadio_var.pAckFrame->mPsdu, (uint8_t *)&ack_nowTime);
            otRadio_var.pAckFrame->mInfo.mRxInfo.mTimestamp = ack_nowTime;
            OT_NOTIFY(OT_SYSTEM_EVENT_RADIO_TX_ACKED);
        }
        Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_RESERVED4);
    }
    else
    {
        //log_error("lmac154_txDoneEvent, %ld, invalid", tx_status);
    }
}

/**
 * @brief
 *
 * @param packet_length
 * @param rx_data_address
 * @param crc_status
 * @param rssi
 * @param snr
 */
static void _RxDoneEvent(uint16_t packet_length, uint8_t *rx_data_address,
                         uint8_t crc_status, uint8_t rssi, uint8_t snr)
{
    otRadio_rxFrame_t *p = NULL;
    static uint8_t rx_done_cnt = 0;

    if (crc_status == 0)
    {
        OT_ENTER_CRITICAL();
        if (!utils_dlist_empty(&otRadio_var.frameList))
        {
            p = (otRadio_rxFrame_t *)otRadio_var.frameList.next;
            otRadio_var.dbgFrameNum --;
            utils_dlist_del(&p->dlist);
        }
        OT_EXIT_CRITICAL();

        if (p)
        {
            OT_ENTER_CRITICAL();
#if OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
            memcpy(p->frame.mPsdu, (rx_data_address + 8), (packet_length - 13));
            p->frame.mLength = (packet_length - 13);
#elif OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
            memcpy(p->frame.mPsdu, (rx_data_address + 9), (packet_length - 14));
            p->frame.mLength = (packet_length - 14);
#endif
            OT_EXIT_CRITICAL();
            p->frame.mChannel = sCurrentChannel;
            p->frame.mInfo.mRxInfo.mRssi = -rssi;
            p->frame.mInfo.mRxInfo.mLqi = ((RAFAEL_RECEIVE_SENSITIVITY - rssi) * 0xFF) / RAFAEL_RECEIVE_SENSITIVITY;

            p->frame.mInfo.mRxInfo.mAckedWithFramePending = false;
            if ((otMacFrameIsVersion2015(&p->frame) && otMacFrameIsCommand((&p->frame))) ||
                    (otMacFrameIsDataRequest((&p->frame)) && hasFramePending((&p->frame))))
            {
                p->frame.mInfo.mRxInfo.mAckedWithFramePending = true;
            }

#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
            if (otMacFrameIsVersion2015(&p->frame) &&
                    otMacFrameIsSecurityEnabled(&p->frame) &&
                    otMacFrameIsAckRequested(&p->frame))
            {
                p->frame.mInfo.mRxInfo.mAckedWithSecEnhAck = true;
                p->frame.mInfo.mRxInfo.mAckFrameCounter = ++sMacFrameCounter;
            }
#endif // OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2            

            OT_ENTER_CRITICAL();
            utils_dlist_add_tail(&p->dlist, &otRadio_var.rxFrameList);
            otRadio_var.dbgRxFrameNum ++;
            if (otRadio_var.dbgMaxPendingFrameNum < otRadio_var.dbgRxFrameNum)
            {
                otRadio_var.dbgMaxPendingFrameNum = otRadio_var.dbgRxFrameNum;
            }
            OT_EXIT_CRITICAL();

            OT_NOTIFY(OT_SYSTEM_EVENT_RADIO_RX_DONE);
        }
        else
        {
            OT_NOTIFY(OT_SYSTEM_EVENT_RADIO_RX_NO_BUFF);
        }
    }

    ++rx_done_cnt > 4 ? rx_done_cnt = 0 : rx_done_cnt;
}
#endif
void ot_radio_short_addr_ctrl(uint8_t ctrl_type, uint8_t *short_addr)
{
    lmac15p4_src_match_short_entry(ctrl_type, short_addr);
}

void ot_radio_extend_addr_ctrl(uint8_t ctrl_type, uint8_t *extend_addr)
{
    lmac15p4_src_match_extended_entry(ctrl_type, extend_addr);
}

void rafael_radio_cca_threshold_set(uint8_t cca_threshold)
{
    if (sCCAThreshold != cca_threshold)
    {
        sCCAThreshold = cca_threshold;
    }
}

static void rafael_otp_mac_addr(uint8_t *addr)
{
    uint8_t  temp[256];
    flash_read_sec_register((uint32_t)temp, 0x1100);
    memcpy(addr, temp + OT_EXT_ADDRESS_SIZE, OT_EXT_ADDRESS_SIZE);
}

void ot_radioInit()
{
    lmac15p4_callback_t mac_cb;
    uint8_t ieeeAddr[OT_EXT_ADDRESS_SIZE] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    otRadio_rxFrame_t *pframe = NULL;

    rafael_otp_mac_addr(sIEEE_EUI64Addr);

    if (!memcmp(ieeeAddr, sIEEE_EUI64Addr, OT_EXT_ADDRESS_SIZE))
    {
        flash_get_unique_id((uint32_t)sIEEE_EUI64Addr, OT_EXT_ADDRESS_SIZE);
    }

    memset(&otRadio_var, 0, offsetof(otRadio_t, buffPool));
    otRadio_var.pAckFrame = (otRadioFrame *)(otRadio_var.buffPool + TOTAL_RX_FRAME_SIZE);
    otRadio_var.pAckFrame->mPsdu = ((uint8_t *)otRadio_var.pAckFrame) + ALIGNED_RX_FRAME_SIZE;

    OT_ENTER_CRITICAL();
    utils_dlist_init(&otRadio_var.frameList);
    utils_dlist_init(&otRadio_var.rxFrameList);

    for (int i = 0; i < OTRADIO_RX_FRAME_BUFFER_NUM; i ++)
    {
        pframe = (otRadio_rxFrame_t *) (otRadio_var.buffPool + TOTAL_RX_FRAME_SIZE * (i + 2));
        pframe->frame.mPsdu = ((uint8_t *)pframe) + ALIGNED_RX_FRAME_SIZE;
        utils_dlist_add_tail(&pframe->dlist, &otRadio_var.frameList);
    }

    otRadio_var.dbgFrameNum = OTRADIO_RX_FRAME_BUFFER_NUM;
    OT_EXIT_CRITICAL();
    mac_cb.rx_cb = _RxDoneEvent;
    mac_cb.tx_cb = _TxDoneEvent;
    lmac15p4_cb_set(0, &mac_cb);

#if OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
    subg_ctrl_sleep_set(false);

    subg_ctrl_idle_set();

    subg_ctrl_modem_config_set(SUBG_CTRL_MODU_FSK, SUBG_CTRL_DATA_RATE_50K, SUBG_CTRL_FSK_MOD_1);

    subg_ctrl_mac_set(SUBG_CTRL_MODU_FSK, SUBG_CTRL_CRC_TYPE_16, SUBG_CTRL_WHITEN_DISABLE);

    subg_ctrl_preamble_set(SUBG_CTRL_MODU_FSK, 8);

    subg_ctrl_sfd_set(SUBG_CTRL_MODU_FSK, 0x00007209);

    subg_ctrl_filter_set(SUBG_CTRL_MODU_FSK, SUBG_CTRL_FILTER_TYPE_GFSK);
#endif
    /* PHY PIB */
    lmac15p4_phy_pib_set(sTurnaroundTime, sCCAMode, sCCAThreshold, sCCADuration);
    /* MAC PIB */
    lmac15p4_mac_pib_set(sUnitBackoffPeriod, sMacAckWaitTime,
                         MAC_PIB_MAC_MAX_BE,
                         MAC_PIB_MAC_MAX_CSMACA_BACKOFFS,
                         sFrameTotalWaitTime, sMacFrameRetris,
                         MAC_PIB_MAC_MIN_BE);
#if OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
    lmac15p4_channel_set((lmac154_channel_t)(sCurrentChannel - kMinChannel));
#elif OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
    subg_ctrl_frequency_set(FREQ + (CHANNEL_SPACING * (sCurrentChannel - kMinChannel)));
#endif
    /* Auto ACK */
    lmac15p4_auto_ack_set(true);
    /* Auto State */
    lmac15p4_auto_state_set(sAuto_State_Set);

    lmac15p4_src_match_ctrl(0, true);
}
