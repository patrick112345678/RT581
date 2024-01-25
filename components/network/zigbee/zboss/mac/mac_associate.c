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
/* PURPOSE: Association - client side
*/

#define ZB_TRACE_FILE_ID 295
#include "zb_common.h"

#if !defined ZB_ALIEN_MAC && !defined ZB_MACSPLIT_HOST

#include "zb_scheduler.h"
#include "zb_nwk.h"
#include "zb_mac.h"
#include "zb_mac_globals.h"
#include "mac_internal.h"



/*! \addtogroup ZB_MAC */
/*! @{ */

#if defined ZB_JOIN_CLIENT || defined ZB_ZGPD_ROLE /*just to compile*/

#ifdef ZB_MAC_TESTING_MODE
static void zb_mac_setup_for_associate(zb_uint8_t page, zb_uint8_t logical_channel, zb_uint16_t pan_id,
                                       zb_uint16_t short_addr, zb_ieee_addr_t long_addr);
#else
static void zb_mac_setup_for_associate(zb_uint8_t page, zb_uint8_t logical_channel, zb_uint16_t pan_id,
                                       zb_uint16_t short_addr/*, zb_ieee_addr_t long_addr*/);
#endif


void zb_mac_assoc_send_data_req(zb_uint8_t param);
void zb_mac_assoc_send_data_req_alarm(zb_uint8_t param);
void mac_association_req_sent(zb_uint8_t param);
void zb_mlme_associate_request_do(zb_uint8_t param);

/**
  7.1.3.1 MLME-ASSOCIATE.request

  - check if device is not associated yet (is done on nwk layer)
  - set parameters:
  phyCurrentChannel = req.logical_channel
  phyCurrentPage = req.channel_page (not used in Zigbee)
  macPANId = req.coord_pan_id
  macCoordExtendedAddress or macCoordShortAddress = req.coord_address
  - send Association request command (mac spec 7.3.1)
  - if send fails, notify higher layer
  - wait for association request acknowledgement
  - after ack is received, wait for macResponseWaitTime symbols and
  then extract associate responce from coordinator (not-beacon mode)
  - if response can not be extracted, set status NO_DATA
  - if response is received, send acknowledgement
  - if assosiation was not successful, set macPANId to 0xFFFF

  @param param - buffer with association parameters
*/
void zb_mlme_associate_request(zb_uint8_t param)
{
#if defined ZB_MAC_API_TRACE_PRIMITIVES
    zb_mac_api_trace_association_request(param);
#endif

    /* Run via tx q to prevent NS failure during prepare for association */
    if (ZB_SCHEDULE_TX_CB(zb_mlme_associate_request_do, param) != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "Oops! No place for zb_mlme_associate_request_do in tx queue", (FMT__0));
        mac_call_associate_confirm_fail(param, MAC_TRANSACTION_OVERFLOW);
    }
}


void zb_mlme_associate_request_do(zb_uint8_t param)
{
    zb_mlme_associate_params_t *params = ZB_BUF_GET_PARAM(param, zb_mlme_associate_params_t);;
    TRACE_MSG(TRACE_MAC2, ">>mlme_ass_req %hd", (FMT__H, param));

    /* Use pending_buf for indirect transmissions. Can't associate is it is busy */
    if (MAC_CTX().pending_buf != 0U
            || MAC_CTX().flags.poll_inprogress
            || MAC_CTX().flags.ass_state != ZB_MAC_ASS_STATE_NONE
#ifndef ZB_MAC_TESTING_MODE
            || params->coord_addr_mode != ZB_ADDR_16BIT_DEV_OR_BROADCAST
#endif
       )
    {
        TRACE_MSG(TRACE_MAC2, "pending_buf is busy or indirect rx is in progress. Failing associate_request. %p %hd %hd",
                  (FMT__P_H_H, MAC_CTX().pending_buf, MAC_CTX().flags.poll_inprogress, MAC_CTX().flags.ass_state));

        mac_call_associate_confirm_fail(param, MAC_INVALID_PARAMETER);
    }
    else
    {
        /* process request immediately*/
#ifdef ZB_MAC_TESTING_MODE
        if (params->coord_addr_mode == ZB_ADDR_64BIT_DEV)
        {
            TRACE_MSG(TRACE_MAC3, "associate to long, addr " TRACE_FORMAT_64,
                      (FMT__A, TRACE_ARG_64(params->coord_addr.addr_long)));
            zb_mac_setup_for_associate(params->channel_page, params->logical_channel, params->pan_id,
                                       (zb_uint16_t)~0, params->coord_addr.addr_long);
        }
        else
        {
            zb_ieee_addr_t laddr;
            ZB_IEEE_ADDR_UNKNOWN(laddr);
            zb_mac_setup_for_associate(params->channel_page, params->logical_channel, params->pan_id,
                                       params->coord_addr.addr_short, laddr);
        }
#else
        zb_mac_setup_for_associate(params->channel_page, params->logical_channel, params->pan_id,
                                   params->coord_addr.addr_short);
#endif
        /* remember association req buffer in pending_buf to be able to call confirm */
        MAC_CTX().pending_buf = param;
        TRACE_MSG(TRACE_MAC2, "scheduling zb_mlme_send_association_req_cmd pending_buf %hd %hd",
                  (FMT__H_H, MAC_CTX().pending_buf, param));

        if (ZB_SCHEDULE_TX_CB(zb_mlme_send_association_req_cmd, param) != RET_OK)
        {
            TRACE_MSG(TRACE_ERROR, "Oops! No place for zb_mlme_send_association_req_cmd in tx queue", (FMT__0));
            mac_call_associate_confirm_fail(param, MAC_TRANSACTION_OVERFLOW);
        }
        else
        {
            MAC_CTX().flags.ass_state = ZB_MAC_ASS_STATE_REQ_SENDING;
        }
    }

    TRACE_MSG(TRACE_MAC2, "<<mlme_ass_req", (FMT__0));
}


/**
   Transceiver setup before associate
 */
#ifdef ZB_MAC_TESTING_MODE
static void zb_mac_setup_for_associate(zb_uint8_t page, zb_uint8_t logical_channel, zb_uint16_t pan_id,
                                       zb_uint16_t short_addr, zb_ieee_addr_t long_addr)
#else
static void zb_mac_setup_for_associate(zb_uint8_t page, zb_uint8_t logical_channel, zb_uint16_t pan_id,
                                       zb_uint16_t short_addr/*, zb_ieee_addr_t long_addr*/)
#endif
{
    ZVUNUSED(logical_channel);
    ZVUNUSED(pan_id);
    ZVUNUSED(short_addr);

    TRACE_MSG(TRACE_MAC1, "mac hw setup before associate page %hd logical_channel %hd pan_id 0x%x short_addr 0x%x",
              (FMT__H_H_D_D, page, logical_channel, pan_id, short_addr));

    ZB_SET_MAC_STATUS(MAC_SUCCESS);
    zb_mac_change_channel(page, logical_channel);
#if 0                           /* Never associate to long addr. BTW normally we do not know long address of the potential parent. */
    ZB_TRANSCEIVER_SET_CHANNEL(page, logical_channel);
    MAC_PIB().phy_current_page = page;
    MAC_PIB().phy_current_channel = logical_channel;
#endif
    ZB_TRANSCEIVER_SET_PAN_ID(pan_id);
    ZB_PIB_SHORT_PAN_ID() = pan_id;
    ZB_TRANSCEIVER_UPDATE_LONGMAC();
#ifdef ZB_MAC_TESTING_MODE
    /* No transceiver except UZ2400 (which was actual tong time ago)
     * requires or supports setting of long coordinator address. Also,
     * we never associate to the long address. */

    /* FIXME: think twice: is association to long address ever possible? Do we need to set long parent address in the HW?
       If answers are "no", can remove here address stuff which is not MAC-related actually.
     */
    /* NK: Association by long address is used in MAC tests (association_04 for example). We need to
     * have it under ZB_MAC_TESTING_MODE at least. */
#ifndef ZB_MACSPLIT_DEVICE
    if (short_addr == (zb_uint16_t)~0)
    {
        short_addr = zb_address_short_by_ieee(long_addr);
    }
    if (ZB_IEEE_ADDR_IS_UNKNOWN(long_addr))
    {
        zb_address_ieee_by_short(short_addr, long_addr);
    }
#endif

    if (!ZB_IEEE_ADDR_IS_UNKNOWN(long_addr))
    {
        TRACE_MSG(TRACE_MAC3, "set coordinator long address " TRACE_FORMAT_64,
                  (FMT__A, TRACE_ARG_64(long_addr)));
        ZB_TRANSCEIVER_SET_COORD_EXT_ADDR(long_addr);
    }
#endif  /* ZB_MAC_TESTING_MODE */
    if (short_addr != (zb_uint16_t)~0U)
    {
        ZB_TRANSCEIVER_SET_COORD_SHORT_ADDR(short_addr);
    }
}


/**
  sends association request command

  called via tx queue
*/
void zb_mlme_send_association_req_cmd(zb_uint8_t param)
{
    zb_mac_mhr_t mhr;
    zb_uint_t packet_length;
    zb_uint_t mhr_len;
    zb_uint8_t *ptr;
    zb_mlme_associate_params_t *params;

    /*
      7.3.1 Association request command
      - fill frame control:
      - source addr mode = ZB_ADDR_64BIT_DEV
      - dst addr mode = req.coord_addr_mode
      - frame pending = 0
      - ack request = 1
      - fill MHR
      - dst pan id = req.coord_pan_id
      - dst address = req.coord_addr
      - src pan id = 0xffff (broadcast pan id)
      - src addr = aExtendedAddress
      - command id = MAC_CMD_ASSOCIATION_REQUEST
    */

    /* | MHR | Command frame id (1 byte) | Capability Info (1 byte) | */

    params = ZB_BUF_GET_PARAM(param, zb_mlme_associate_params_t);
    TRACE_MSG(TRACE_MAC2, ">>mlme_send_ass_req_cmd %hd %p", (FMT__H_P, param, params));
    mhr_len = zb_mac_calculate_mhr_length(ZB_ADDR_64BIT_DEV, params->coord_addr_mode, ZB_FALSE);
    packet_length = mhr_len;
    packet_length += sizeof(zb_uint8_t) * 2U;

    ptr = zb_buf_initial_alloc(MAC_CTX().operation_buf, packet_length);
    ZB_ASSERT(ptr);

    ZB_BZERO(ptr, packet_length);

    /* Fill Frame Controll then call zb_mac_fill_mhr()
       mac spec  7.2.1.1 Frame Control field
       | Frame Type | Security En | Frame Pending | Ack.Request | PAN ID Compres | Reserv | Dest.Addr.Mode | Frame Ver | Src.Addr.gMode |
    */
    ZB_BZERO2(mhr.frame_control);

    ZB_FCF_SET_FRAME_TYPE(mhr.frame_control, MAC_FRAME_COMMAND);
    /* security enable is 0 */
    /* frame pending is 0 */
    ZB_FCF_SET_ACK_REQUEST_BIT(mhr.frame_control, 1U);
    /* pan id compress is 0 */
    ZB_FCF_SET_DST_ADDRESSING_MODE(mhr.frame_control, params->coord_addr_mode);
    /* MAC_FRAME_VERSION defined in zb_config.h */
    ZB_FCF_SET_FRAME_VERSION(mhr.frame_control, MAC_FRAME_VERSION);
    ZB_FCF_SET_SRC_ADDRESSING_MODE(mhr.frame_control, ZB_ADDR_64BIT_DEV);

    /* mac spec 7.5.6.1 Transmission */
    mhr.seq_number = ZB_MAC_DSN();
    ZB_INC_MAC_DSN();

    mhr.dst_pan_id = params->pan_id;
    ZB_MEMCPY(&mhr.dst_addr, &params->coord_addr, sizeof(zb_addr_u));
    mhr.src_pan_id = ZB_BROADCAST_PAN_ID;

    ZB_MEMCPY(&mhr.src_addr, MAC_PIB().mac_extended_address, sizeof(zb_ieee_addr_t));

    zb_mac_fill_mhr(ptr, &mhr);

    /* | MHR | Command frame id 1 byte | Capability info 1 byte | */

    ptr += mhr_len;
    *ptr = MAC_CMD_ASSOCIATION_REQUEST;
    ptr++;
    *ptr = params->capability;

    MAC_CTX().tx_wait_cb = mac_association_req_sent;
    MAC_CTX().tx_wait_cb_arg = param;
    ZB_ASSERT(mhr_len <= ZB_UINT8_MAX);
    zb_mac_send_frame(MAC_CTX().operation_buf, (zb_uint8_t)mhr_len);

    TRACE_MSG(TRACE_MAC2, "<<mlme_send_ass_req_cmd", (FMT__0));
}


void mac_association_req_sent(zb_uint8_t param)
{
    ZVUNUSED(param);

    TRACE_MSG(TRACE_MAC2, ">>zb_mlme_send_association_req_done", (FMT__0));

    MAC_CTX().flags.ass_state = ZB_MAC_ASS_STATE_REQ_SENT;

    if (ZB_GET_MAC_STATUS() == MAC_SUCCESS)
    {
        /* wait for macResponseWaitTime time interval
         * and try to get associate resp (indirect transmission) */
        MAC_CTX().flags.ass_indir_retries = 0;
        ZB_SCHEDULE_ALARM(zb_mac_assoc_send_data_req_alarm, param, ZB_MAC_PIB_RESPONSE_WAIT_TIME);
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "association req is not sent: %hd", (FMT__H, ZB_GET_MAC_STATUS()));
        mac_call_associate_confirm_fail(param, ZB_GET_MAC_STATUS());
    }

    TRACE_MSG(TRACE_MAC2, "<<zb_mlme_send_association_req_done", (FMT__0));
}


void zb_mac_assoc_send_data_req_alarm(zb_uint8_t param)
{
    if (ZB_SCHEDULE_TX_CB(zb_mac_assoc_send_data_req, param) != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "Oops! No place for zb_mac_assoc_send_data_req in tx queue", (FMT__0));
        mac_call_associate_confirm_fail(param, MAC_TRANSACTION_OVERFLOW);
    }
}


/**
   Send data request during association
*/
void zb_mac_assoc_send_data_req(zb_uint8_t param)
{
    zb_mlme_associate_params_t *params;
    zb_mlme_data_req_params_t data_req_cmd_params;

    TRACE_MSG(TRACE_MAC2, ">>zb_mac_assoc_send_data_req %hd", (FMT__H, param));

    params = ZB_BUF_GET_PARAM(param, zb_mlme_associate_params_t);
    data_req_cmd_params.src_addr_mode = ZB_ADDR_64BIT_DEV;
    ZB_MEMCPY(&data_req_cmd_params.src_addr, &MAC_PIB().mac_extended_address, sizeof(zb_ieee_addr_t));
    data_req_cmd_params.dst_addr_mode = params->coord_addr_mode;
    ZB_MEMCPY(&data_req_cmd_params.dst_addr, &params->coord_addr, sizeof(zb_addr_u));

    MAC_CTX().flags.ass_state = ZB_MAC_ASS_STATE_POLLING;

    /* send data request */
    zb_mac_get_indirect_data(&data_req_cmd_params);
}


void mac_call_associate_confirm_fail(zb_uint8_t param, zb_mac_status_t cb_status)
{
    zb_bool_t call_confirm = ZB_FALSE;
    zb_bool_t do_cleanup = ZB_FALSE;

    TRACE_MSG(TRACE_MAC2, ">> mac_call_associate_confirm_fail param %hd cb_status %hd",
              (FMT__H_H, param, cb_status));

    TRACE_MSG(TRACE_MAC3, "ass_state 0x%hx ass_indir_retries %hd",
              (FMT__H_H, MAC_CTX().flags.ass_state, MAC_CTX().flags.ass_indir_retries));

    if (cb_status == MAC_INVALID_PARAMETER)
    {
        TRACE_MSG(TRACE_MAC3, "Assoc process is busy", (FMT__0));
        /* Case when MAC is busy for Poll or another association */
        /* Don't cleanup ongoing associaiton unless we wanna break it */
        call_confirm = ZB_TRUE;
    }
    else
    {
        MAC_CTX().flags.ass_indir_retries++;
        if (MAC_CTX().flags.ass_state == ZB_MAC_ASS_STATE_POLL_FAILED &&
                MAC_CTX().flags.ass_indir_retries < ZB_MAC_ASSOCIATION_DATA_REQUEST_COUNT)
        {
            TRACE_MSG(TRACE_MAC3, "new ass_indir_retries %hd - retry poll", (FMT__H, MAC_CTX().flags.ass_indir_retries));

            MAC_CTX().flags.ass_state = ZB_MAC_ASS_STATE_POLL_RETRY;

            if (ZB_SCHEDULE_TX_CB(zb_mac_assoc_send_data_req, param) != RET_OK)
            {
                TRACE_MSG(TRACE_ERROR, "Oops! No place for zb_mac_assoc_send_data_req in tx queue", (FMT__0));
                call_confirm = ZB_TRUE;
                do_cleanup = ZB_TRUE;
            }
        }
        else
        {
            TRACE_MSG(TRACE_MAC3, "AssReq failed or out of DataReq retransmits", (FMT__0));

            call_confirm = ZB_TRUE;
            do_cleanup = ZB_TRUE;
        }
    }

    TRACE_MSG(TRACE_MAC2, "call_confirm %hd do_cleanup %hd",
              (FMT__H_H, call_confirm, do_cleanup));

    if (call_confirm)
    {
        zb_mlme_associate_confirm_t *ass_confirm_param;

        TRACE_MSG(TRACE_MAC3, ">>mac_call_associate_confirm_fail %hd %hd", (FMT__H_H, param, cb_status));

        ass_confirm_param = ZB_BUF_GET_PARAM(param, zb_mlme_associate_confirm_t);
        ZB_BZERO(ass_confirm_param, sizeof(zb_mlme_associate_confirm_t));
        zb_buf_set_status(param, cb_status);
        ass_confirm_param->status = cb_status;
        TRACE_MSG(TRACE_MAC3, "calling zb_mlme_associate_confirm status %hd", (FMT__H, cb_status));

#if defined ZB_MAC_API_TRACE_PRIMITIVES
        zb_mac_api_trace_association_confirm(param);
#endif

        ZB_SCHEDULE_CALLBACK(zb_mlme_associate_confirm, param);
    }

    if (do_cleanup)
    {
        ZB_ASSERT(MAC_CTX().pending_buf == param);
        TRACE_MSG(TRACE_MAC3, "zero pending_buf", (FMT__0));
        MAC_CTX().pending_buf = 0;
        MAC_CTX().flags.ass_state = ZB_MAC_ASS_STATE_NONE;
    }

    TRACE_MSG(TRACE_MAC2, "<<mac_call_associate_confirm_fail", (FMT__0));
}


void mac_handle_associate_resp(zb_uint8_t param, zb_mac_mhr_t *mhr, zb_uint8_t *cmd_ptr)
{
    /*cstat !MISRAC2012-Rule-20.7 See ZB_BUF_GET_PARAM() for more information. */
    zb_mlme_associate_confirm_t *ass_confirm_param = ZB_BUF_GET_PARAM(MAC_CTX().pending_buf, zb_mlme_associate_confirm_t);

    TRACE_MSG(TRACE_MAC1, ">>mac_handle_associate_resp %hd", (FMT__H, param));

    /* for debugging */
    TRACE_MSG(TRACE_ERROR, "received association response", (FMT__0));

    if (ZB_FCF_GET_SRC_ADDRESSING_MODE(mhr->frame_control) == ZB_ADDR_64BIT_DEV)
    {
        ZB_IEEE_ADDR_COPY(ass_confirm_param->parent_address, mhr->src_addr.addr_long);
        ZB_IEEE_ADDR_COPY(MAC_PIB().mac_coord_extended_address, mhr->src_addr.addr_long);
    }
    else
    {
        ZB_IEEE_ADDR_ZERO(ass_confirm_param->parent_address);
    }

    cmd_ptr++;
    zb_get_next_letoh16(&MAC_PIB().mac_short_address, &cmd_ptr);
    ass_confirm_param->assoc_short_address = MAC_PIB().mac_short_address;
    TRACE_MSG(TRACE_MAC3, "mac_short_address set: 0x%x", (FMT__D, MAC_PIB().mac_short_address));
    ass_confirm_param->status = (zb_mac_status_t) * cmd_ptr;
    zb_buf_set_status(MAC_CTX().pending_buf, ass_confirm_param->status);
    if (ass_confirm_param->status != 0U)
    {
        ZB_PIB_SHORT_PAN_ID() = ZB_BROADCAST_PAN_ID;
        MAC_PIB().mac_short_address = ZB_MAC_SHORT_ADDR_NOT_ALLOCATED;
    }
    else
    {
        ZB_TRANSCEIVER_SET_COORD_EXT_ADDR(MAC_PIB().mac_coord_extended_address);
    }
    ZB_TRANSCEIVER_UPDATE_SHORT_ADDR();
    TRACE_MSG(TRACE_MAC1, "zb_mlme_asociate_confirm scheduled %hd", (FMT__H, MAC_CTX().pending_buf));

#if defined ZB_MAC_API_TRACE_PRIMITIVES
    zb_mac_api_trace_association_confirm(MAC_CTX().pending_buf);
#endif

    ZB_SCHEDULE_CALLBACK(zb_mlme_associate_confirm, MAC_CTX().pending_buf);
    MAC_CTX().flags.ass_state = ZB_MAC_ASS_STATE_NONE;
    MAC_CTX().pending_buf = 0;
    zb_buf_free(param);
    TRACE_MSG(TRACE_MAC3, "zero pending_buf", (FMT__0));

    TRACE_MSG(TRACE_MAC1, "<<mac_handle_associate_resp", (FMT__0));
}

#endif  /* ZB_JOIN_CLIENT */

/*! @} */

#endif  /* !ZB_ALIEN_MAC && !ZB_MACSPLIT_HOST */
