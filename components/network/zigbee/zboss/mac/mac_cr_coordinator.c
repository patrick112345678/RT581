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
/* PURPOSE: Roitines specific to coordinator role
*/

#define ZB_TRACE_FILE_ID 299
#include "zb_common.h"

#if !defined ZB_ALIEN_MAC && !defined ZB_MACSPLIT_HOST

#include "zb_scheduler.h"
#include "zb_nwk.h"
#include "zb_mac_globals.h"
#include "zb_mac.h"
#include "mac_internal.h"
#include "zb_ie.h"


/*! \addtogroup ZB_MAC */
/*! @{ */



#if defined ZB_ROUTER_ROLE


void zb_mac_update_superframe_and_pib(zb_uint8_t param);
static void zb_mlme_handle_orphan_response_continue(zb_uint8_t param);

void zb_mlme_start_request(zb_uint8_t param)
{
  zb_mlme_start_req_t *params;
  zb_mac_status_t status;

  TRACE_MSG( TRACE_MAC2, "+zb_mlme_start_request %hd", (FMT__H, param ));

#if defined ZB_MAC_API_TRACE_PRIMITIVES
    zb_mac_api_trace_start_request(param);
#endif

  params = ZB_BUF_GET_PARAM(param, zb_mlme_start_req_t);

  /* mac spec 7.1.14.1 MLME-START.request */
  /* Table 72 - MLME-START.request parameters */
  /* 7.5.1.1 Superframe structure */
  if (params->beacon_order > ZB_TURN_OFF_ORDER ||
      (params->superframe_order > params->beacon_order && params->superframe_order != ZB_TURN_OFF_ORDER))
  {
    /* mac spec 7.1.14.1.3 Effect on receipt */
    TRACE_MSG( TRACE_ERROR, "zb_mlme_start_request : invalid parameter!", (FMT__0 ));
    status = MAC_INVALID_PARAMETER;
  }
  /* mac spec 7.1.14.1.3 Effect on receipt */
  else if (ZB_PIB_SHORT_ADDRESS() == ZB_MAC_SHORT_ADDR_NO_VALUE)
  {
    TRACE_MSG(TRACE_ERROR, "no short address!", (FMT__0));
    status = MAC_NO_SHORT_ADDRESS;
  }
  else
  {
    status = MAC_SUCCESS;
  }

  if (status == MAC_SUCCESS)
  {
    /* process request immediately*/

/*
  MAC spec, 7.1.14.1 MLME-START.request
  1) check macShortAddress, if it is == 0xffff report NO_SHORT_ADDRESS
  (it seems local setting of short address should be checked)
  2) check CoordRealignment, if it is == TRUE, transmit a coordinator
  realignment command frame (7.5.2.3.2.) (Shall we check if we are
  Coordinator????) (*)
  - if command finishes successfully, update BeaconOrder,
  SuperframeOrder, PANId, ChannelPage, and LogicalChannel parameters, as
  described in 7.5.2.3.4: zb_mac_update_superframe_and_pib
  - confirm success: MLME-START.confirm, status == SUCCESS
  3) if CoordRealignment FALSE, update PIB parameters according to
  7.5.2.3.4: zb_mac_update_superframe_and_pib()
  4) if BeaconOrder < 15, pib.macBattLifeExt = req.BatteryLifeExtension;
  if if BeaconOrder == 15, ignore req.BatteryLifeExtension
  5) if req.SecurityLevel > 0  ----------------------------  NOT SUPPORTED YET
  - set Frame Control.Security Enabled = 1
  - use outgoing frame security procedure (7.5.8.2.1)
  - if CoordRealignment == TRUE, use CoordRealignSecurityLevel,
  CoordRealignKeyIdMode,  CoordRealignKey-Source, and CoordRealignKeyInde
  - if BeaconOrder < 15 (beacon-enabled) use BeaconSecurityLevel,
  BeaconKeyIdMode, BeaconKeySource, and BeaconKeyIndex
  - if the beacon frame length > aMaxPHYPacketSize, set status FRAME_TOO_LONG
  6) if BeaconOrder < 15 (beacon enabled)  ----------------------- NOT USED IN ZB
  - if PAN coordinator == TRUE, StartTime = 0
  - if StartTime > 0, calculate the beacon transmission time
  - beacon transmission time = time of receiving the beacon of the coordinator + StartTime
  - if beacon transmission time causes outgoing superframe to overlap
  the incoming superframe, set status SUPERFRAME_OVERLAP
  - if StartTime > 0 and the MLME is not currently tracking the beacon
  of the coordinator, set status TRACKING_OFF

  (*) CoordRealignment == TRUE case
  1) if beacon mode is on, set Frame Control.Frame Pending = 1, do NOT change
  other parameters, send scheduled beacon
  2) send realignment command
  3) if beacon mode is off, send realignment command immediately
  4) if realignment command send fails, set status CHANNEL_ACCESS_FAILURE
  5) update PIB parameters according to 7.5.2.3.4

*/

    ZB_SET_MAC_STATUS(MAC_SUCCESS);
    if (params->beacon_order == ZB_TURN_OFF_ORDER)
    {
      params->superframe_order = ZB_TURN_OFF_ORDER;
    }

    MAC_PIB().mac_pan_coordinator = params->pan_coordinator;

#ifdef ZB_AUTO_ACK_TX
    ZB_TRANSCEIVER_SET_PAN_COORDINATOR(MAC_PIB().mac_pan_coordinator);
#endif /* ZB_AUTO_ACK_TX */

#ifdef ZB_OPTIONAL_MAC_FEATURES
/*seems like realignment never worked. At least, never call
 * zb_nlme_start_router_request more then once. */
    if (params->coord_realignment)
    {
      /* TODO: Currently, zb_realign_pan() always return RET_OK. To not complicate this function,
       * the error code is simply ignored. It should be decided if zb_realign_pan() may return non
       * OK result in the future, and do proper error handling here or make zb_realign_pan() void.
       */
      (void)zb_realign_pan(0);
    }
#endif  /* ZB_OPTIONAL_MAC_FEATURES */
      if (MAC_PIB().mac_beacon_order < ZB_TURN_OFF_ORDER)
      {
        /* if BeaconOrder < 15, pib.macBattLifeExt = req.BatteryLifeExtension */
        MAC_PIB().mac_batt_life_ext = params->battery_life_extension;
      }

    if (ZB_SCHEDULE_TX_CB(zb_mac_update_superframe_and_pib, param) != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "Oops! No place for zb_mac_update_superframe_and_pib in tx queue", (FMT__0));
      zb_buf_set_status(param, MAC_TRANSACTION_OVERFLOW);
      ZB_SCHEDULE_CALLBACK(zb_mlme_start_confirm, param);
    }
  }
  TRACE_MSG(TRACE_MAC2, "<< zb_mlme_start_request", (FMT__0));
}


void zb_mac_update_superframe_and_pib(zb_uint8_t param)
{
  zb_mlme_start_req_t *params;

/*
  mac spec 7.5.2.3.4:
  - pib.macBeaconOrder =  req.BeaconOrder
  - if pib.macBeaconOrder == 15, pib.macSuperframeOrder = 15 (NON beacon PAN case)
  - if pib.macBeaconOrder < 15, pib.macSuperframeOrder = req.SuperframeOrder
  - pib.macPANID = req.PANId
  - pib.phyCurrentPage = req.ChannelPage (use PLME-SET.request)
  - pib.phyCurrentChannel = req.LogicalChannel (use PLME-SET.request)
*/

  TRACE_MSG( TRACE_MAC1, ">>zb_mac_update_superframe_and_pib", (FMT__0 ));

  params = ZB_BUF_GET_PARAM(param, zb_mlme_start_req_t);

  MAC_PIB().mac_beacon_order = params->beacon_order;
  if (MAC_PIB().mac_beacon_order == ZB_TURN_OFF_ORDER)
  {
    MAC_PIB().mac_superframe_order = ZB_TURN_OFF_ORDER;
  }
  else
  {
    MAC_PIB().mac_superframe_order = params->superframe_order;
  }
  MAC_PIB().mac_pan_id = params->pan_id;

  zb_mac_change_channel(params->channel_page, params->logical_channel);

  TRACE_MSG(TRACE_MAC1, "set pan id 0x%x page %hd channel 0x%hx",
            (FMT__D_H_H, MAC_PIB().mac_pan_id, MAC_PIB().phy_current_page,
             MAC_PIB().phy_current_channel));

  ZB_TRANSCEIVER_UPDATE_LONGMAC();
  ZB_TRANSCEIVER_UPDATE_SHORT_ADDR();
  ZB_TRANSCEIVER_UPDATE_PAN_ID();

  /* for Zigbee it is not needed to support beacon mode, so do
   * not support beacon - oriented stuff here */

  zb_buf_set_status(param, MAC_SUCCESS);
#if defined ZB_MAC_API_TRACE_PRIMITIVES
  zb_mac_api_trace_start_confirm(param);
#endif
  ZB_SCHEDULE_CALLBACK(zb_mlme_start_confirm, param);

  TRACE_MSG(TRACE_MAC1, "<<zb_mac_update_superframe_and_pib", (FMT__0));
}


/**
   Alarm used to transmit beacon after delay.

   The rationale is: beacon, as any data packet. must be transmitted via tx
   queue to prevent situation when data packet is transmitted but its transmit
   status is not received yet (no status from NS, no interrupt at HW).
 */
void zb_handle_beacon_req_alarm(zb_uint8_t param)
{
  if (ZB_SCHEDULE_TX_CB(zb_handle_beacon_req, param) != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "Oops - out of MAC queue!", (FMT__0));
  }
}

#if defined ZB_ENHANCED_BEACON_SUPPORT

/**
   Alarm used to transmit enhanced beacon after delay.

   Idea is the same as for zb_handle_beacon_req_alarm
 */
void zb_handle_enhanced_beacon_req_alarm(zb_uint8_t param)
{
  if (ZB_SCHEDULE_TX_CB(zb_handle_enhanced_beacon_req, param) != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "Oops - out of MAC queue!", (FMT__0));
  }
}


/*
 * Section D.9.1 of r22 specification
 */
void zb_handle_enhanced_beacon_req(zb_uint8_t param)
{
  // extract zb_mac_eb_params_t from buf param??
  // @TODO set tx power from somewhere

  zb_uint8_t packet_length;
  zb_uint8_t mhr_len;
  zb_mac_mhr_t mhr;
  zb_uint8_t *ptr;

  ZVUNUSED(param);
  TRACE_MSG(TRACE_MAC1,
            ">> zb_handle_enhanced_beacon_req mac_beacon_payload_length 0x%hx",
            (FMT__H, MAC_PIB().mac_beacon_payload_length));

#ifndef ZB_MAC_TESTING_MODE
  /* skip this check in test mode */
  if (MAC_PIB().mac_beacon_payload_length == 0U)
  {
    TRACE_MSG(TRACE_MAC1, "no beacon payload - ignore enh beacon req", (FMT__0));
  }
  else
#endif  /* ZB_MAC_TESTING_MODE */
  {
    /*
     * IE structure:
     *
     * HIE (Termination)
     * PIE (Vendor specific)
     *   Sub-IE (EB Payload)
     *   Sub-IE (TX Power)
     * PIE (Termination)
     */

    mhr_len = zb_mac_calculate_mhr_length(ZB_ADDR_64BIT_DEV, ZB_ADDR_NO_ADDR, ZB_FALSE);

    /* MHR + HIE Termination + PIE vendor hdr +  Zigbee payload IE
       + Beacon payload + Superframe spec + sender short addr
       + TX Power descriptor + PIE Termination */
    packet_length = mhr_len + ZB_HIE_HEADER_LENGTH + ZB_PIE_VENDOR_HEADER_LENGTH
      + ZB_ZIGBEE_PIE_HEADER_LENGTH + (zb_uint8_t)sizeof(zb_super_frame_spec_t)
      + MAC_PIB().mac_beacon_payload_length + 2U + ZB_TX_POWER_IE_DESCRIPTOR_LEN
      + ZB_PIE_HEADER_LENGTH;

    TRACE_MSG(TRACE_MAC1,
              "packet length %hd, payload len %hd",
              (FMT__H_H, packet_length, MAC_PIB().mac_beacon_payload_length));

    ptr = zb_buf_initial_alloc(MAC_CTX().operation_buf, packet_length);
    ZB_ASSERT(ptr);

    ZB_BZERO(ptr, packet_length);

    /* Set FCF field attributes */
    ZB_BZERO(&mhr, sizeof(zb_mac_mhr_t));
    ZB_FCF_SET_FRAME_TYPE(mhr.frame_control, MAC_FRAME_BEACON);
    /* security enabled = 0*/
    /* frame pending = 0 */
    /* ack required = 0 */
    /* pan id compression = 0 */
    /* reserved = 0 */
    /* seq number suppression = 0*/
    ZB_FCF_SET_IE_LIST_PRESENT_BIT(mhr.frame_control, 1U);
    /* destination address mode = 0 */
    ZB_FCF_SET_SRC_ADDRESSING_MODE(mhr.frame_control, ZB_ADDR_64BIT_DEV);
    ZB_FCF_SET_FRAME_VERSION(mhr.frame_control, MAC_FRAME_IEEE_802_15_4_2015);

    mhr.seq_number = ZB_MAC_EBSN();
    ZB_INC_MAC_EBSN();
    mhr.src_pan_id = MAC_PIB().mac_pan_id;

    /* aExtendedAddress The 64-bit (IEEE) address assigned to the device */
    ZB_MEMCPY(&mhr.src_addr, MAC_PIB().mac_extended_address, sizeof(zb_ieee_addr_t));

    /* AUX security header not present */

    zb_mac_fill_mhr(ptr, &mhr);
    ptr += mhr_len;

    /* HIE termination */
    ZB_SET_NEXT_HIE_HEADER(ptr, ZB_HIE_ELEMENT_HT1, 0U);

    /* Vendor-specific PIE */
    ZB_SET_NEXT_PIE_ZIGBEE_VENDOR_HEADER(ptr,
                                         ZB_TX_POWER_IE_DESCRIPTOR_LEN +
                                         ZB_ZIGBEE_PIE_HEADER_LENGTH +
                                         (zb_uint8_t)sizeof(zb_super_frame_spec_t) +
                                         MAC_PIB().mac_beacon_payload_length + 2U);

    /* EB payload */
    ZB_SET_NEXT_ZIGBEE_PIE_HEADER(ptr,
                                  (zb_uint8_t)ZB_ZIGBEE_PIE_SUB_ID_EB_PAYLOAD,
                                  (zb_uint8_t)sizeof(zb_super_frame_spec_t) +
                                  MAC_PIB().mac_beacon_payload_length + 2U);

    /*
     * EB payload has the following format (D.9.1.2 section of r22 spec):
     * Beacon payload | Superframe Specification | Source Short Address (1 bytes)
     */

    /*
      Beacon payload field (as in older versions)
    */
    ZB_MEMCPY(ptr, (zb_uint8_t*)&MAC_PIB().mac_beacon_payload, MAC_PIB().mac_beacon_payload_length);
    ptr += MAC_PIB().mac_beacon_payload_length;

    /*
      Superframe Specification field
      as defined in D.9.1.2 of r22 and 7.3.1.3 of 802.15.4-2015

      - Beacon order = req.beacon_order
      - superframe order = req.superframe_order
      - final CAP slot = aMinCAPLength
      - BLE = req.BLE (Battery Life Extension)
      - set PAN coordinator = ZB_COORDINATOR_ROLE
      - set association permit = pib.macAssociationPermit
    */
    ZB_BZERO2(ptr);
    ZB_SUPERFRAME_SET_BEACON_ORDER(ptr, MAC_PIB().mac_beacon_order);
    ZB_SUPERFRAME_SET_SUPERFRAME_ORDER(ptr, MAC_PIB().mac_superframe_order);
    /* spec says final cap slot should be not less then MAC_MIN_CAP_LENGTH, but this
     * const == 440 and doesn't fit into 4 bits; other stacks use value 0x0f */
    ZB_SUPERFRAME_SET_FINAL_CAP_SLOT(ptr, 0x0FU);
    ZB_SUPERFRAME_SET_BLE(ptr, MAC_PIB().mac_batt_life_ext);

    ZB_SUPERFRAME_SET_PAN_COORD(ptr, MAC_PIB().mac_pan_coordinator);

    ZB_SUPERFRAME_SET_ASS_PERMIT(ptr, MAC_PIB().mac_association_permit);

    TRACE_MSG(TRACE_MAC3, "ass perm %hd", (FMT__H, (zb_uint8_t)MAC_PIB().mac_association_permit));

    ptr += sizeof(zb_uint16_t);

    /* Put sender short address */
    ZB_PUT_NEXT_HTOLE16(ptr, MAC_PIB().mac_short_address);

    /* Put tx power */
    ZB_SET_NEXT_TX_POWER_IE_DESCRIPTOR(ptr, (zb_uint8_t)MAC_CTX().default_tx_power);

    /* Finally put PIE termination */
    ZB_SET_NEXT_PIE_HEADER(ptr, ZB_PIE_GROUP_TERMINATION, 0U);

    ZVUNUSED(ptr);

#if defined ZB_MAC_POWER_CONTROL
    ZB_MAC_POWER_CONTROL_LOCK_POWER_APPLY();
#endif /* ZB_MAC_POWER_CONTROL */

    /* No need tx wait cb: can just ignore tx status */
    zb_mac_send_frame(MAC_CTX().operation_buf, mhr_len);

#if defined ZB_MAC_POWER_CONTROL
    ZB_MAC_POWER_CONTROL_UNLOCK_POWER_APPLY();
#endif /* ZB_MAC_POWER_CONTROL */
  }

#ifdef ZB_MULTIPLE_BEACONS
  MAC_CTX().beacons_sent++;
#endif
}

#endif /* ZB_ENHANCED_BEACON_SUPPORT */

void zb_handle_beacon_req(zb_uint8_t param)
{
  zb_uint8_t packet_length;
  zb_uint8_t mhr_len;
  zb_mac_mhr_t mhr;
  zb_uint8_t *ptr;
  zb_uint8_t src_addr_mode;

  ZVUNUSED(param);
  TRACE_MSG(TRACE_MAC2, ">> zb_handle_beacon_req mac_beacon_payload_length 0x%x", (FMT__D, MAC_PIB().mac_beacon_payload_length));
  /* Ignore beacon requests if we are not ZC/ZR or if not started net/joined -
   * that means empty beacon payload. */
#if !defined ZB_MAC_TESTING_MODE
  /* skip this check in test mode */
  if (MAC_PIB().mac_beacon_payload_length == 0U
#ifdef ZB_CERTIFICATION_HACKS
      && !ZB_CERT_HACKS().set_empty_beacon_payload
#endif
    )
  {
    TRACE_MSG(TRACE_ERROR, "no beacon payload - ignore beacon req", (FMT__0));
  }
  else
#endif  /* !ZB_MAC_TESTING_MODE && !ZB_PAN_ID_CONFLICT_TEST_CASE */
  {

#ifdef ZB_MAC_TESTING_MODE
    if (ZB_PIB_SHORT_ADDRESS() == ZB_MAC_SHORT_ADDR_NOT_ALLOCATED)
    {
      src_addr_mode = ZB_ADDR_64BIT_DEV;
    }
    else
#endif
    {
      src_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
    }

    TRACE_MSG(TRACE_MAC2, ">>zb_send_beacon_frame", (FMT__0));

/* mac spec  7.2.2.1 Beacon frame format
   | MHR | Superframe | GTS | Pending address | Beacon Payload | MFR |
*/
    mhr_len = zb_mac_calculate_mhr_length(src_addr_mode, ZB_ADDR_NO_ADDR, ZB_FALSE);

    packet_length = mhr_len + (zb_uint8_t)sizeof(zb_super_frame_spec_t)
                    + 2U * (zb_uint8_t)sizeof(zb_uint8_t) + MAC_PIB().mac_beacon_payload_length;

    TRACE_MSG(TRACE_MAC1, "packet length %hd, payload len %hd", (FMT__H_H, packet_length, MAC_PIB().mac_beacon_payload_length));

    ptr = zb_buf_initial_alloc(MAC_CTX().operation_buf, packet_length);
    ZB_ASSERT(ptr);

    ZB_BZERO(ptr, packet_length);

/* mac spec  7.2.1.1 Frame Control field
   | Frame Type | Security En | Frame Pending | Ack.Request | PAN ID Compres | Reserv | Dest.Addr.Mode | Frame Ver | Src.Addr.gMode |

   mac spec 7.2.2.1.1 Beacon frame MHR fields
   Frame Control field
   - Frame type = MAC_FRAME_BEACON
   - src addr mode specified by coordinator
   - set security enabled = 1 if specified
   - security enabled == 1 set frame ver = 1, 0 otherwise
   - If a broadcast data or command frame is pending, set frame pending = 1
   - set other fields to 0
*/
    ZB_BZERO(&mhr, sizeof(zb_mac_mhr_t));
    ZB_FCF_SET_FRAME_TYPE(mhr.frame_control, MAC_FRAME_BEACON);
    ZB_FCF_SET_SRC_ADDRESSING_MODE(mhr.frame_control, src_addr_mode);
    /* MAC_FRAME_VERSION defined in zb_config.h */
    ZB_FCF_SET_FRAME_VERSION(mhr.frame_control, MAC_FRAME_VERSION);
    ZB_FCF_SET_FRAME_PENDING_BIT(mhr.frame_control, 0U); /* no beacon-enabled network */


/*
  mac spec 7.2.2.1.1 Beacon frame MHR fields
  MHR field
  - Sequence Number = pib.macBSN
  - src pan id = pib.pan_id
  - src pan addr = pan addres
  - if security enabled, set Auxiliary Security Header field (7.2.1.7 Auxiliary Security Header field)

*/
    mhr.seq_number = ZB_MAC_BSN();
    ZB_INC_MAC_BSN();
    mhr.src_pan_id = MAC_PIB().mac_pan_id;

#ifdef ZB_MAC_TESTING_MODE
    if (src_addr_mode == ZB_ADDR_64BIT_DEV)
    {
      /* aExtendedAddress The 64-bit (IEEE) address assigned to the device */
      ZB_MEMCPY(&mhr.src_addr, MAC_PIB().mac_extended_address, sizeof(zb_ieee_addr_t));
    }
    else
#endif
    {
      mhr.src_addr.addr_short = MAC_PIB().mac_short_address;
      TRACE_MSG(TRACE_MAC1, "beacon set short addr %x", (FMT__D, mhr.src_addr.addr_short));
    }

    zb_mac_fill_mhr(ptr, &mhr);
    ptr += mhr_len;

/*
  7.2.2.1.2 Superframe Specification field
  | Beacon    | Superframe | Final CAP | Battery Life Extension | Reserved | PAN Coordinator | Association |
  | Order 0-3 | Order 4-7  | Slot 8-11 | (BLE) 12               | 13       | 14              | Permit 15   |
  - Beacon order = req.beacon_order
  - superframe order = req.superframe_order
  - final CAP slot = aMinCAPLength
  - BLE = req.BLE (Battery Life Extension)
  - set PAN coordinator = ZB_COORDINATOR_ROLE
  - set association permit = pib.macAssociationPermit
*/
    ZB_BZERO2(ptr);
    ZB_SUPERFRAME_SET_BEACON_ORDER(ptr, MAC_PIB().mac_beacon_order);
    ZB_SUPERFRAME_SET_SUPERFRAME_ORDER(ptr, MAC_PIB().mac_superframe_order);
/* spec says final cap slot should be not less then MAC_MIN_CAP_LENGTH, but this
 * const == 440 and doesn't fit into 4 bits; other stacks use value 0x0f */
    ZB_SUPERFRAME_SET_FINAL_CAP_SLOT(ptr, 0x0FU);
    ZB_SUPERFRAME_SET_BLE(ptr, MAC_PIB().mac_batt_life_ext);

    ZB_SUPERFRAME_SET_PAN_COORD(ptr, MAC_PIB().mac_pan_coordinator);

    ZB_SUPERFRAME_SET_ASS_PERMIT(ptr, MAC_PIB().mac_association_permit);

    TRACE_MSG(TRACE_MAC3, "ass perm %hd", (FMT__H, (zb_uint8_t)MAC_PIB().mac_association_permit));

    ptr += sizeof(zb_uint16_t);

/*
  7.2.2.1.3 GTS Specification field
  - set all fields to 0
  7.2.2.1.5 GTS List field
  is omitted
  7.2.2.1.6 Pending Address Specification field
  - set all fields to 0
  7.2.2.1.7 Address List field
  is omitted
*/
    /* next 2 bytes value is already == 0 */
    ptr += 2U * sizeof(zb_uint8_t);

/*
  7.2.2.1.8 Beacon Payload field
  cfill beacon payload with value pib.macBeaconPayload. Size of data is
  sizeof(zb_mac_beacon_payload_t) == 15 bytes (Zigbee spec, 3.6.7 NWK
  Information in the MAC Beacons)
*/
    ZB_MEMCPY(ptr, (zb_uint8_t*)&MAC_PIB().mac_beacon_payload, MAC_PIB().mac_beacon_payload_length);

    /* No need tx wait cb: can just ignore tx status */
    zb_mac_send_frame(MAC_CTX().operation_buf, mhr_len);
  }

#if defined ZB_CERTIFICATION_HACKS
  if (ZB_CERT_HACKS().set_empty_beacon_payload)
  {
    ZB_CERT_HACKS().set_empty_beacon_payload = 0;
    TRACE_MSG(TRACE_ZDO1, "schedule zb_nwk_update_beacon_payload", (FMT__0));
    zb_buf_get_out_delayed(zb_nwk_update_beacon_payload);
  }
#endif

#ifdef ZB_MULTIPLE_BEACONS
  MAC_CTX().beacons_sent++;
#endif

  TRACE_MSG(TRACE_MAC1, "<< zb_handle_beacon_req", (FMT__0));
}

#ifndef ZB_LITE_NO_ORPHAN_SCAN
static void zb_handle_mlme_orphan_response(zb_uint8_t param)
{
  zb_mac_orphan_response_t *oresp;

  TRACE_MSG(TRACE_MAC1, ">>zb_handle_mlme_orphan_response", (FMT__0));

  TRACE_MSG(TRACE_MAC3, "pending_buf %p", (FMT__P, MAC_CTX().pending_buf));

  MAC_CTX().pending_buf = param;
  /*cstat !MISRAC2012-Rule-20.7 See ZB_BUF_GET_PARAM() for more information. */
  oresp = ZB_BUF_GET_PARAM(MAC_CTX().pending_buf, zb_mac_orphan_response_t);

  MAC_CTX().tx_wait_cb = zb_mlme_handle_orphan_response_continue;
  MAC_CTX().tx_wait_cb_arg = param;

  (void)zb_tx_coord_realignment_cmd(ZB_FALSE, oresp->orphan_addr, oresp->short_addr,
                                    ZB_PIB_SHORT_PAN_ID(), MAC_PIB().phy_current_channel,
                                    MAC_PIB().phy_current_page);
  TRACE_MSG(TRACE_MAC1, "<<zb_handle_mlme_orphan_response, continue scheduled", (FMT__0));
}

void zb_mlme_orphan_response(zb_uint8_t param)
{
#if defined ZB_MAC_API_TRACE_PRIMITIVES
  zb_mac_api_trace_orphan_response(param);
#endif

  if (ZB_SCHEDULE_TX_CB(zb_handle_mlme_orphan_response, param) != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "Oops - out of MAC queue!", (FMT__0));
    zb_buf_free(param);
  }
}


static void zb_mlme_handle_orphan_response_continue(zb_uint8_t param)
{
  /* 22.08.2016 CR [DT] begin */
  /* MLME-COMM-STATUS.indication should use long addresses */
  /* check command TX status */
  //if (ZB_GET_MAC_STATUS() == MAC_SUCCESS)
  {
    zb_bufid_t buf = MAC_CTX().pending_buf;
    zb_mac_orphan_response_t *oresp; /* possible better to send it via MAC_CTX() */
    zb_ieee_addr_t orphan_addr;
    zb_mlme_comm_status_indication_t *ind;

    param = buf;
    TRACE_MSG(TRACE_MAC1, ">>zb_mlme_handle_orphan_response_continue", (FMT__0));

    oresp = ZB_BUF_GET_PARAM(param, zb_mac_orphan_response_t);
    ZB_IEEE_ADDR_COPY(orphan_addr, oresp->orphan_addr);

    ind = ZB_BUF_GET_PARAM(param, zb_mlme_comm_status_indication_t);
    ind->status = ZB_GET_MAC_STATUS();
    ind->src_addr_mode = ZB_ADDR_64BIT_DEV;
    ZB_IEEE_ADDR_COPY(ind->src_addr.addr_long, MAC_PIB().mac_extended_address);
    ind->dst_addr_mode = ZB_ADDR_64BIT_DEV;
    ZB_IEEE_ADDR_COPY(ind->dst_addr.addr_long, orphan_addr);
    ind->pan_id = MAC_PIB().mac_pan_id;

#if defined ZB_MAC_API_TRACE_PRIMITIVES
    zb_mac_api_trace_comm_status_indication(param);
#endif

    /* call nwk comm status */
    ZB_SCHEDULE_CALLBACK(zb_mlme_comm_status_indication, param);
  }
  /* 22.08.2016 CR [DT] end */

  TRACE_MSG(TRACE_MAC1, "<<zb_mlme_handle_orphan_response_continue", (FMT__0));
}
#endif  /* #ifndef ZB_LITE_NO_ORPHAN_SCAN */

#endif  /* router */

/*! @} */

#endif  /* !ZB_ALIEN_MAC && !ZB_MACSPLIT_HOST*/
