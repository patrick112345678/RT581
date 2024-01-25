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
/* PURPOSE: Roitines specific to mlme scan for coordinator/router
*/

#define ZB_TRACE_FILE_ID 298
#include "zb_common.h"

#if !defined ZB_ALIEN_MAC && !defined ZB_MACSPLIT_HOST

#include "zb_scheduler.h"
#include "zb_nwk.h"
#include "zb_mac.h"
#include "zb_mac_globals.h"
#include "mac_internal.h"



/*! \addtogroup ZB_MAC */
/*! @{ */

#if defined ZB_ROUTER_ROLE

static zb_bool_t zb_mac_find_pending_ass_resp(zb_ieee_addr_t ieee_addr)
{
  zb_uint8_t i;

  for (i = 0; i < ZB_MAC_PENDING_QUEUE_SIZE; i++)
  {
    if (MAC_CTX().pending_data_queue[i].pending_param != 0U
        && MAC_CTX().pending_data_queue[i].dst_addr_mode == ZB_ADDR_64BIT_DEV)
    {
      zb_uint8_t *pend_addr = MAC_CTX().pending_data_queue[i].dst_addr.addr_long;
      if (ZB_IEEE_ADDR_CMP(ieee_addr, pend_addr))
      {
        return ZB_TRUE;
      }
    }
  }

  return ZB_FALSE;
}

void zb_mlme_associate_response(zb_uint8_t param)
{
  zb_ret_t ret;
  zb_mlme_associate_response_t params;
  zb_mac_mhr_t mhr = {0};
  zb_uint_t packet_length;
  zb_uint_t mhr_len;
  zb_uint8_t *ptr;
  zb_mac_pending_data_t pend_data = {0};

  TRACE_MSG(TRACE_MAC2, ">>zb_mlme_associate_response %hd", (FMT__H, param));
  ZB_SET_MAC_STATUS(MAC_SUCCESS);


/*
  7.1.3.3 MLME-ASSOCIATE.response
  - send Association response command, using indirect transmission
  - add packet to send to pending list; if there is no space, set
  status TRANSACTION_OVERFLOW
  - if packet is not handled during macTransactionPersistenceTime,
  set status TRANSACTION_EXPIRED
  - if any parameter value is incorrect, set status INVALID_PARAMETER
  - if frame is transmited and ack is received, set status SUCCESS
  - send indication primitive
*/

  TRACE_MSG(TRACE_MAC2, ">>handle_ass_resp", (FMT__0));

#if defined ZB_MAC_API_TRACE_PRIMITIVES
  zb_mac_api_trace_association_response(param);
#endif

  ZB_MEMCPY(&params, ZB_BUF_GET_PARAM(param, zb_mlme_associate_response_t), sizeof(zb_mlme_associate_response_t));

/*
  7.3.2 Association response command
  | MHR | cmd frame id 1b | short addr 2b | ass status 1b |
  - command id = MAC_CMD_ASSOCIATION_RESPONSE
  - dst addr mode = ZB_ADDR_64BIT_DEV
  - src addr mode = ZB_ADDR_64BIT_DEV
  - frame pending = 0
  - ack req = 1
  - pan id compress = 1
  - dst pan id = macPANid
  - src pan id = 0
  - dst addr = device ext addr
  - src addr = aExtendedAddress

  - short addr = associated addr
  - ass status = association status
*/

  mhr_len = zb_mac_calculate_mhr_length(ZB_ADDR_64BIT_DEV, ZB_ADDR_64BIT_DEV, ZB_TRUE);
  packet_length = mhr_len;
  packet_length += (sizeof(zb_uint8_t) + sizeof(zb_uint16_t) +sizeof(zb_uint8_t));

  ptr = zb_buf_initial_alloc(param, packet_length);
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
  ZB_FCF_SET_PANID_COMPRESSION_BIT(mhr.frame_control, 1U);
  ZB_FCF_SET_DST_ADDRESSING_MODE(mhr.frame_control, ZB_ADDR_64BIT_DEV);
  ZB_FCF_SET_FRAME_VERSION(mhr.frame_control, MAC_FRAME_VERSION);
  ZB_FCF_SET_SRC_ADDRESSING_MODE(mhr.frame_control, ZB_ADDR_64BIT_DEV);

  /* mac spec 7.5.6.1 Transmission */
  mhr.seq_number = ZB_MAC_DSN();
  ZB_INC_MAC_DSN();

  mhr.dst_pan_id = MAC_PIB().mac_pan_id;
  ZB_IEEE_ADDR_COPY(mhr.dst_addr.addr_long, params.device_address);

  TRACE_MSG(TRACE_MAC3, "ASS RESP dst mod %hi addr " TRACE_FORMAT_64, (FMT__H_A,
                                                                       ZB_FCF_GET_DST_ADDRESSING_MODE(mhr.frame_control),
                                                                       TRACE_ARG_64(mhr.dst_addr.addr_long)));
  mhr.src_pan_id = MAC_PIB().mac_pan_id;
  ZB_IEEE_ADDR_COPY(mhr.src_addr.addr_long, MAC_PIB().mac_extended_address);
  TRACE_MSG(TRACE_MAC3, "ASS RESP src mod %hi addr " TRACE_FORMAT_64, (FMT__H_A,
                                                                       ZB_FCF_GET_SRC_ADDRESSING_MODE(mhr.frame_control),
                                                                       TRACE_ARG_64(mhr.src_addr.addr_long)));


  zb_mac_fill_mhr(ptr, &mhr);

  ptr += mhr_len;
  *ptr = MAC_CMD_ASSOCIATION_RESPONSE;
  ptr++;
  ZB_PUT_NEXT_HTOLE16(ptr, params.short_address);
  *ptr = params.status;

  /* Association response is filled, lets put it to pending queue */

  pend_data.pending_param = param;
  pend_data.dst_addr_mode = ZB_ADDR_64BIT_DEV;
  ZB_IEEE_ADDR_COPY(pend_data.dst_addr.addr_long, params.device_address);

  ret = zb_mac_put_data_to_pending_queue(&pend_data);

  if (ret == RET_ERROR)
  {
    TRACE_MSG(TRACE_MAC3, "calling zb_mac_send_comm_status", (FMT__0));
    (void)zb_mac_call_comm_status_ind(param, ZB_GET_MAC_STATUS(), param);
  }
  TRACE_MSG(TRACE_MAC2, "<<zb_mlme_associate_response", (FMT__0));
}


/* Coordinator side: get request command, say ACK to end device,
 * signal to high level with associate.indication */
void zb_process_ass_request_cmd(zb_uint8_t param)
{
  zb_uint8_t *cmd_ptr;
  zb_mac_mhr_t mhr;
  zb_mlme_associate_indication_t *ass_indication;

  TRACE_MSG(TRACE_MAC2, ">>zb_process_ass_request_cmd %hd", (FMT__H, param));
  cmd_ptr = zb_buf_begin(param);
  cmd_ptr += zb_parse_mhr(&mhr, param);

  {
    ass_indication = ZB_BUF_GET_PARAM(param, zb_mlme_associate_indication_t);
/* 7.3.1 Association request command
   | MHR | Command frame id 1 byte | Capability info 1 byte | */

    cmd_ptr++;
    ass_indication->capability = *cmd_ptr;
    ass_indication->lqi = ZB_MAC_GET_LQI(param);
    ZB_MEMCPY(&ass_indication->device_address, &mhr.src_addr, sizeof(zb_ieee_addr_t));
  }

  if (zb_mac_find_pending_ass_resp(ass_indication->device_address))
  {
    TRACE_MSG(TRACE_MAC2,
              "Drop buffer - association for device " TRACE_FORMAT_64 " is ongoing",
              (FMT__A, TRACE_ARG_64(ass_indication->device_address)));
    zb_buf_free(param);
  }
  else
  {
#if defined ZB_MAC_API_TRACE_PRIMITIVES
    zb_mac_api_trace_association_indication(param);
#endif

    ZB_SCHEDULE_CALLBACK(zb_mlme_associate_indication, param);
  }

  TRACE_MSG(TRACE_MAC2, "<<zb_process_ass_request_cmd", (FMT__0));
}
/*! @} */

#endif /*ZB_COORDINATOR_ROLE || ZB_ROUTER_ROLE */


#endif  /* !ZB_ALIEN_MAC && !ZB_MACSPLIT_HOST */
