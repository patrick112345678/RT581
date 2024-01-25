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
/* PURPOSE: ZLL device discovery functions
*/

#define ZB_TRACE_FILE_ID 2113
#include "zb_common.h"
#include "zb_zcl.h"
#include "zll/zb_zll_common.h"
#include "zll/zll_commissioning_internals.h"
#ifdef ZB_BDB_TOUCHLINK
#include "zb_bdb_internal.h"
#include "zb_magic_macros.h"
#endif
#include "zb_aps_interpan.h"

#if defined ZB_ENABLE_ZLL

#ifndef ZB_BDB_TOUCHLINK

/** @internal @brief Channel per scan step array.
  *
  * Array index corresponds to number of the ScanRequest packet in the device discovery procedure,
  * and value of the element determines the channel transceiver should be tuned to.
  */
static zb_uint8_t g_zll_scan_num_to_channel[] =
    {11, 11, 11, 11, 11, 15, 20, 25, 12, 13, 14, 16, 17, 18, 19, 21, 22, 23, 24, 26};

/** @internal @brief Number of steps to perform for normal device scanning process during device
  * discovery procedure.
  */
#define ZLL_COMMISSIONING_NORMAL_SCAN_BOUNDARY 8

/** @internal @brief Number of steps to perform for extended device scanning process during device
  * discovery procedure.
  */
#define ZLL_COMMISSIONING_EXTENDED_SCAN_BOUNDARY (sizeof(g_zll_scan_num_to_channel))

/** @internal @brief Evaluates minimum of two numbers. */
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#endif

/** @internal @brief Schedules next appropriate scan request in the device discovery sequence.
  * @param param reference to the buffer to put packet data to.
  */
void zll_send_next_scan_req(zb_uint8_t param);

/** @internal @brief ZLL.ScanRequest send confirmation processing.
  * @param param reference to the buffer containing ZCL "confirmation".
  */
void zll_scan_req_sent(zb_uint8_t param);

/** @internal @brief Schedules for sending next device information request.
  * @param param reference to the buffer to put packet to.
  */
void zll_send_next_devinfo_req(zb_uint8_t param);

/** @internal @brief Device information request packet send confirmation.
  * @param param reference to the buffer containing confirmation.
  */
void zll_devinfo_sent(zb_uint8_t param);

/** @internal @brief Handles incoming ScanRequest frame.
  * @param param reference to the buffer containing ScanRequest frame.
  */
zb_ret_t zll_handle_scan_req(zb_uint8_t param);

/** @internal @brief Processes confirmation on ScanResponse frame.
  * @param param reference to the packet containing confirmation.
  */
void zll_scan_res_sent(zb_uint8_t param);

/** @internal @brief Protects transaction identifier from timing out during ScanResponse sending on
  * the target side.
  * @param param - formal parameter, should be ignored.
  */
void zll_scan_res_timeout(zb_uint8_t param);

/** @internal @brief Handle incoming DeviceInformationRequest frame.
  * @param param reference to the buffer containing the packet.
  */
zb_ret_t zll_handle_devinfo_req(zb_uint8_t param);

/** @internal @brief Handles incoming DeviceInformationResponse frame.
  * @param param reference to the buffer containing packet.
  */
void zll_handle_scan_res(zb_uint8_t param);

/** @internal @brief Handles sending scan request after channel adjustment. */
void zll_send_next_scan_req_continue(zb_uint8_t param);

#ifndef ZB_BDB_TOUCHLINK

void zb_zll_start_device_discovery(zb_uint8_t param)
{
  zb_zll_device_discovery_req_t* request =
      ZB_BUF_GET_PARAM(param, zb_zll_device_discovery_req_t);

  TRACE_MSG(TRACE_ZLL1, "> zb_zll_start_device_discovery param %hd", (FMT__H, param));

  /* TODO some other checks are required to discover we're in a non-intrusive state, and are able
   * to cancel.
   */
  if (ZLL_TRAN_CTX().transaction_id)
  {
    zll_cancel_transaction();
  }

  /* Set initial transaction state. */
  ZB_MEMSET(&ZLL_TRAN_CTX(), 0, sizeof(ZLL_TRAN_CTX()));
  ZLL_TRAN_CTX().transaction_id = ZB_ZLL_GET_NEW_TRANS_ID();
  ZLL_TRAN_CTX().transaction_task = ZB_ZLL_DEVICE_DISCOVERY_TASK;
  ZLL_TRAN_CTX().ext_scan = request->ext_scan;

#ifndef ZB_BDB_TOUCHLINK
  ZB_SCHEDULE_ALARM(zll_intrp_transaction_guard, 0, ZB_ZLL_APLC_INTRP_TRANSID_LIFETIME);
#endif
  /* MP: Acccording to ZLL spec, subclause 8.4.1.1, we should adjust transmitter power here to
   * 0dBm. However, we have no handle to turn currently.
   */

  zll_send_next_scan_req(param);

  TRACE_MSG(TRACE_ZLL1, "< zb_zll_start_device_discovery", (FMT__0));
}/* void zb_zll_start_device_discovery(zb_uint8_t param) */

/** @internal @brief Evaluates scan step boundary according to the ZLL transaction context. */
#define ZLL_GET_SCAN_STEP_BOUNDARY()           \
  (   (ZLL_TRAN_CTX().ext_scan)                \
   ?  ZLL_COMMISSIONING_EXTENDED_SCAN_BOUNDARY \
   : ZLL_COMMISSIONING_NORMAL_SCAN_BOUNDARY)

/** @internal @brief Evaluates channel number according to the current scan step (in the ZLL
  * transaction context.
  */
#define ZLL_SCAN_STEP_GET_CURRENT_CHANNEL() (g_zll_scan_num_to_channel[ZLL_TRAN_CTX().pckt_cnt])

#define ZLL_SCAN_STEP_GET_CURRENT_CHANNEL_PTR() (g_zll_scan_num_to_channel + ZLL_TRAN_CTX().pckt_cnt)

/** @internal @brief Checks that a channel corresponding to the current scan step is a restricted
  * one.
  */
#define ZLL_SCAN_STEP_CHANNEL_RESTRICTED(cur_channel_mask)                                                 \
  ((zb_aib_channel_page_list_get_mask((cur_channel_mask)) & (((zb_uint32_t)1) << ZLL_SCAN_STEP_GET_CURRENT_CHANNEL())) == 0)

/** @internal @brief Prepares device information gathering. */
#define ZLL_PREPARE_DEVINFO_GATHER()                                         \
{                                                                            \
  TRACE_MSG(TRACE_ZLL3, "Preparing to gather device information", (FMT__0)); \
  ZLL_TRAN_CTX().pckt_cnt = 0;                                               \
  zll_send_next_devinfo_req(param);                                          \
}


void zll_send_next_scan_req(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZLL1, "> zll_send_next_scan_req param %hd", (FMT__H, param));

  TRACE_MSG(TRACE_ZLL1, "aps_channel_mask 0x%lx pckt_cnt %hd SCAN_STEP_BOUNDARY %hd",
            (FMT__L_H_H, zb_aib_channel_page_list_get_mask(0)/* MMDEVSTUBS */,
             ZLL_TRAN_CTX().pckt_cnt, ZLL_GET_SCAN_STEP_BOUNDARY()));

  while (   ZLL_TRAN_CTX().pckt_cnt < ZLL_GET_SCAN_STEP_BOUNDARY()
         && ZLL_SCAN_STEP_CHANNEL_RESTRICTED(0/* MMDEVSTUBS */))
  {
    ++ZLL_TRAN_CTX().pckt_cnt;
    TRACE_MSG(TRACE_ZLL1, "new pckt_cnt %hd", (FMT__H, ZLL_TRAN_CTX().pckt_cnt));
  }

  if (ZLL_TRAN_CTX().pckt_cnt < ZLL_GET_SCAN_STEP_BOUNDARY())
  {
    TRACE_MSG(TRACE_ZLL3, "Preparing to send next ScanRequest", (FMT__0));
    /* TODO: Implement callback for channel setting, put all following ops in it. */
    ZLL_SET_CHANNEL(param, ZLL_SCAN_STEP_GET_CURRENT_CHANNEL(), zll_send_next_scan_req_continue);
  }
  else
  {
    ZLL_PREPARE_DEVINFO_GATHER();
  }

  TRACE_MSG(TRACE_ZLL1, "< zll_send_next_scan_req", (FMT__0));
}/* void zll_send_next_scan_req(zb_uint8_t param) */

void zll_scan_req_sent(zb_uint8_t param)
{
  zb_bufid_t  buffer = param;
  zb_uint8_t status = buffer->u.hdr.status;

  TRACE_MSG(TRACE_ZLL1, "> zll_scan_req_sent param %hd status %hd", (FMT__H_H, param, status));

  if (status)
  {
    ZB_SCHEDULE_ALARM_CANCEL(zll_scan_req_guard, ZB_ALARM_ANY_PARAM);
    zll_notify_task_result(buffer, ZB_ZLL_TASK_STATUS_FAILED);
  }
  else
  {
    /* MP: Waiting for response ot timeout, no need for the buffer. */
    zb_free_buf(buffer);
  }

  TRACE_MSG(TRACE_ZLL1, "< zll_scan_req_sent status %hd", (FMT__H, status));
}/* void zll_scan_req_sent(zb_uint8_t param) */

void zll_scan_req_guard_process(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZLL1, "> zll_scan_req_guard_process param %hd", (FMT__H, param));

  if (ZLL_TRAN_CTX().send_confirmed)
  {
    if (ZLL_TRAN_CTX().pckt_cnt < ZLL_GET_SCAN_STEP_BOUNDARY())
    {
      TRACE_MSG(TRACE_ZLL3, "Preparing to send next ScanRequest", (FMT__0));
      zll_send_next_scan_req(param);
    }
    else
    {
      ZLL_PREPARE_DEVINFO_GATHER();
    }
  }
  else
  {
    zb_buf_free(param);
    ZB_SCHEDULE_ALARM(zll_scan_req_guard, 0, ZB_ZLL_APLC_SCAN_TIME_BASE_DURATION);
  }

  TRACE_MSG(TRACE_ZLL1, "< zll_scan_req_guard_process", (FMT__0));
}/* void zll_scan_req_guard(zb_uint8_t param) */

void zll_scan_req_guard(zb_uint8_t param)
{
  (void)param;

  TRACE_MSG(TRACE_ZLL1, "> zll_scan_req_guard param %hd", (FMT__H, param));

  TRACE_MSG(TRACE_ZLL1, "in %hd out %hd", (FMT__H_H, ZG->bpool.bufs_allocated[0], ZG->bpool.bufs_allocated[1]));
  zb_buf_get_in_delayed(zll_scan_req_guard_process);

  TRACE_MSG(TRACE_ZLL1, "< zll_scan_req_guard", (FMT__0));
}

void zll_intrp_transaction_guard_process(zb_uint8_t param)
{
  zb_uint8_t old_state = ZLL_TRAN_CTX().transaction_task;

  TRACE_MSG(TRACE_ZLL1, "> zll_intrp_transaction_guard param %hd", (FMT__H, param));

  if (! ZLL_TRAN_CTX().transaction_id)
  {
    TRACE_MSG(
        TRACE_ERROR,
        "ERROR ZLL inter-PAN transaction timed out while no active transactions present.",
        (FMT__0));
  }
  else if (old_state == ZB_ZLL_DEVICE_DISCOVERY_TASK)
  {
    ZB_SCHEDULE_ALARM_CANCEL(zll_scan_req_guard, ZB_ALARM_ANY_PARAM);
  }
  /* Nothing to do for ZB_ZLL_DEVICE_INFO_GATHER */
  /* TODO Add approptiate "else if" branches for other transaction states. */

  ZLL_TRAN_CTX().transaction_id = ZLL_NO_TRANSACTION_ID;

  zll_notify_task_result(buf, ZB_ZLL_TASK_STATUS_TIMED_OUT);

  TRACE_MSG(TRACE_ZLL1, "< zll_intrp_transaction_guard", (FMT__0));
}/* void zll_intrp_transaction_guard(zb_uint8_t param) */

void zll_intrp_transaction_guard(zb_uint8_t param)
{
  (void)param;
  zb_buf_get_in_delayed(zll_intrp_transaction_guard_process);
}

#endif  /* ZB_BDB_TOUCHLINK */

void zll_send_next_devinfo_req(zb_uint8_t param)
{
  zb_uint8_t status;

  TRACE_MSG(TRACE_ZLL1, "> zll_send_next_devinfo_req param %hd", (FMT__H, param));

  while (ZLL_TRAN_CTX().current_dev_info_idx < ZLL_TRAN_CTX().n_device_infos)
  {
    if (ZLL_TRAN_CTX().device_infos[ZLL_TRAN_CTX().current_dev_info_idx].sub_device_count > 1)
    {
      TRACE_MSG(TRACE_ZLL1, "device index %hd count sub %hd", (FMT__H_H,
            ZLL_TRAN_CTX().current_dev_info_idx,
            ZLL_TRAN_CTX().device_infos[ZLL_TRAN_CTX().current_dev_info_idx].sub_device_count));
      /* MP: It's only the first packet, for a device with more than 5 endpoints we'll send
       * additional requests from zll_devinfo_sent().
       */
      ZB_ZLL_COMMISSIONING_SEND_DEVICE_INFO_REQ(
          param,
          0,
          ZLL_TRAN_CTX().device_infos[ZLL_TRAN_CTX().current_dev_info_idx].device_addr,
          zll_devinfo_sent,
          status);
      if (status != RET_OK)
      {
        TRACE_MSG(TRACE_ERROR, "ERROR could not schedule dev info req for sending", (FMT__0));
        zll_notify_task_result(param, ZB_ZLL_TASK_STATUS_SCHEDULE_FAILED);
      }
      ++ZLL_TRAN_CTX().current_dev_info_idx;
      return;
    }
    else
    {
      ++ZLL_TRAN_CTX().current_dev_info_idx;
    }
  }
#ifndef ZB_BDB_TOUCHLINK
  ZB_SCHEDULE_ALARM_CANCEL(zll_intrp_transaction_guard, ZB_ALARM_ANY_PARAM);
#endif
  zll_notify_task_result(param, ZB_ZLL_TASK_STATUS_FINISHED);
  TRACE_MSG(TRACE_ZLL1, "< zll_send_next_devinfo_req", (FMT__0));
}/* void zll_send_next_devinfo_req(zb_uint8_t param) */

void zll_devinfo_sent(zb_uint8_t param)
{
  zb_zll_commissioning_device_information_req_t old_request;
  zb_uint8_t status;

  TRACE_MSG(TRACE_ZLL1, "> zll_devinfo_sent param %hd", (FMT__H, param));

  if (*(zb_uint8_t*)ZB_BUF_GET_PARAM(param, zb_uint8_t) == ZB_APS_STATUS_SUCCESS)
  {
/* CR:MEDIUM looks like it is better to store start_idx in context, analysis of REQ takes a lot of code */
/* CR:DELAYED In "start new network" implementation I introduced a kind of command handling
 * context. I suppose it'll fit our needs here.
 */
    /* TODO Implement command handling status, use it instead of request parsing. */
    ZB_ZLL_COMMISSIONING_GET_DEVICE_INFO_REQ(&old_request, param, status);
    if (status == ZB_ZCL_PARSE_STATUS_FAILURE)
    {
      zll_send_next_devinfo_req(param);
    }
    else
    {
      old_request.start_idx += 5;
      if (old_request.start_idx <
            ZLL_TRAN_CTX().device_infos[ZLL_TRAN_CTX().current_dev_info_idx - 1].sub_device_count)
      {
        ZB_ZLL_COMMISSIONING_SEND_DEVICE_INFO_REQ(
            param,
            old_request.start_idx,
            ZLL_TRAN_CTX().device_infos[ZLL_TRAN_CTX().current_dev_info_idx - 1].device_addr,
            zll_devinfo_sent,
            status);
        if (status != RET_OK)
        {
          TRACE_MSG(TRACE_ERROR, "ERROR could not schedule dev info req for sending", (FMT__0));
          zll_notify_task_result(param, ZB_ZLL_TASK_STATUS_SCHEDULE_FAILED);
          goto zll_devinfo_sent_finals;
        }
      }
      else
      {
        zll_send_next_devinfo_req(param);
      }
    }
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Could not send DeviceInformationRequest, transaction failed", (FMT__0));
    zll_notify_task_result(param, ZB_ZLL_TASK_STATUS_FAILED);
  }

zll_devinfo_sent_finals:
  TRACE_MSG(TRACE_ZLL1, "< zll_devinfo_sent", (FMT__0));
}/* void zll_devinfo_sent(zb_uint8_t param) */


/* target side routines */

/**
   Handle incoming scan request
 */
zb_ret_t zll_handle_scan_req(zb_uint8_t param)
{
  zb_uint8_t status;
  zb_ret_t result;
  zb_zll_commissioning_scan_req_t request;
  zb_zcl_parsed_hdr_t cmd_info;

  TRACE_MSG(TRACE_ZLL1, "> zll_handle_scan_req param %hd", (FMT__H, param));

#ifdef ZB_BDB_TOUCHLINK
  if (/* ZB_BDB().v_do_primary_scan >= ZB_BDB_JOIN_MACHINE_PRIMARY_SCAN */
    ZB_BDB().bdb_commissioning_step == ZB_BDB_TOUCHLINK_COMMISSIONING &&
    ZB_BDB().bdb_commissioning_status == ZB_BDB_STATUS_IN_PROGRESS
    )
  {
    /*
If, during its scan, an initiator with the bdbNodeIsOnANetwork attribute
equal to FALSE receives another scan request inter-PAN command frame with
the factory new sub-field of the touchlink information field equal to 1, it
SHALL be ignored. Conversely, if, during its scan, an initiator with the
bdbNodeIsOnANetwork attribute equal to FALSE receives another scan request
inter-PAN command frame with the factory new sub-field of the touchlink
information field equal to 0, it MAY stop sending its own scan request
inter-PAN command frames and assume the role of a target (see sub-clause
8.8)

MAY - means, we can ignore this request if we are scanning now.

This is TLI5 - it is O, set it to N.
     */
    TRACE_MSG(TRACE_ZLL1, "BDB Touchlink Initiator is in progress. Drop Scan request.", (FMT__0));
    zb_buf_free(param);
    return RET_OK;
  }
#endif
  ZB_ZCL_COPY_PARSED_HEADER(param, &cmd_info);
  ZB_ZLL_COMMISSIONING_GET_SCAN_REQ(&request, param, status);

/*
TODO: Handle it.

TLI8

If a scan request command frame is received from a target on the same
network as the initiator with the network update identifier field
lower than nwkUpdateId, does the initiator transmit a network update
request inter-PAN command frame to the selected target and then
terminate the touchlink procedure? 8.7

TLI1: M	Y
 */

  if (status == ZB_ZCL_PARSE_STATUS_SUCCESS)
  {
    TRACE_MSG(TRACE_ZLL1,
        "status %hd transaction_id %ld request.trans_id %ld rssi %hd src_addr " TRACE_FORMAT_64,
        (FMT__H_L_L_H_A, status, ZLL_TRAN_CTX().transaction_id, request.trans_id, cmd_info.rssi,
              TRACE_ARG_64(cmd_info.addr_data.intrp_data.src_addr)));
  }

  if (status == ZB_ZCL_PARSE_STATUS_FAILURE)
  {
    result = RET_ERROR;
    TRACE_MSG(TRACE_ERROR, "ERROR Could not parse ZLL.Commissioning.ScanRequest frame", (FMT__0));
    zb_buf_free(param);
  }
  else if (ZLL_TRAN_CTX().transaction_id
           && request.trans_id == ZLL_TRAN_CTX().transaction_id
           && ZB_IEEE_ADDR_CMP(ZLL_TRAN_CTX().src_addr, cmd_info.addr_data.intrp_data.src_addr))
  {
    result = RET_ERROR;
    TRACE_MSG(
        TRACE_ZLL3,
        "We already communicated with this initiator, dropping the packet.",
        (FMT__0));
    zb_buf_free(param);
  }
  else if (!ZLL_TRAN_CTX().transaction_id
           && cmd_info.rssi > ZLL_DEVICE_INFO().rssi_threshold)
  {
    ZLL_TRAN_CTX().transaction_id = request.trans_id;
    ZB_IEEE_ADDR_COPY(ZLL_TRAN_CTX().src_addr, cmd_info.addr_data.intrp_data.src_addr);
#ifndef ZB_BDB_TOUCHLINK
    ZB_SCHEDULE_ALARM(zll_scan_res_timeout, 0,
        sizeof(g_zll_scan_num_to_channel)*ZB_ZLL_APLC_SCAN_TIME_BASE_DURATION);
#else
    ZB_SCHEDULE_ALARM(zll_scan_res_timeout, 0,
                      MAGIC_ONE_32(ZB_BDBC_TL_PRIMARY_CHANNEL_SET) * ZB_BDBC_TL_SCAN_TIME_BASE_DURATION);
#endif
    /* MP: According to ZLL spec, subclause 8.4.1.2, we'd set transmitter's power to 0 dBm.
     * However, ve have no such a handle currently.
     */
    TRACE_MSG(TRACE_ZLL2, "status2 %hd %hd %hd", (FMT__H_H_H,
        status, ZLL_TRAN_CTX().transaction_id, ZLL_TRAN_CTX().transaction_task));

//NK_TEST
// store trans_id for security needs
    ZLL_DEVICE_INFO().transaction_id = request.trans_id;
    TRACE_MSG(TRACE_ZLL1, "trans id %d", (FMT__P, ZLL_DEVICE_INFO().transaction_id));

    ZB_ZLL_COMMISSIONING_SEND_SCAN_RES(
        param,
        cmd_info.seq_number,
        zll_scan_res_sent,
        result);
    if (result != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "ERROR Could not schedule scan response for sending.", (FMT__0));
      zb_buf_free(param);
    }
  }
  else
  {
    result= RET_ERROR;
    TRACE_MSG(
        TRACE_ZLL3,
        "Have transaction from other device or with other transaction identifier, droppping.",
        (FMT__0));
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZLL1, "< zll_handle_scan_req %d", (FMT__D, result));
  return result;
}/* void zll_handle_scan_req(zb_uint8_t param) */

void zll_scan_res_sent(zb_uint8_t param)
{
  zb_uint32_t packet_trans_id;

  TRACE_MSG(TRACE_ZLL1, "> zll_scan_res_sent param %hd", (FMT__H, param));
  /* CR:below we're sure it is for the last response scheduled for sending. */
  ZB_SCHEDULE_ALARM_CANCEL(zll_scan_res_timeout, ZB_ALARM_ANY_PARAM);
  if (zb_buf_get_handle(param) == ZB_APS_STATUS_SUCCESS)
  {
    /* CR:MEDIUM It is not good to parse scan response here. a lot of src
     * code, not so usefull. All checks maybe replaced by 1 check: buffer
     * parameter - if buffer is the same as it was used while sending
     * response, it is OK. Are other checkings for command send done in
     * the same way?
     */

    /* CR:NOTE In all cases I have to check that transaction the packet belongs to was not cancelled by
     * application. Nevertheless, I have to remove address check: as far as I know, I have no parsed
     * ZCL header here.
     */

    /* Exploiting knowledge of internals: transaction identifier goes first. */

    /*
    ZB_LETOH32(&packet_trans_id, zb_buf_begin(buffer));

    AT: it's incorrect, because incoming buffer consists of:
    ZCL FC[1]
    in case when we are using manufacturer specific commands we must ignore manufacturer specific code [2]
    Sequence number[1]
    CommandId[1]
    TransactionId[4]
    so we must inc pointer by 3, and then take transaction id value
    */
    zb_uint8_t *ptr = zb_buf_begin(param);
    zb_zcl_frame_ctrl_t *fc = (zb_zcl_frame_ctrl_t *)ptr;
    ptr += (fc->manufacturer)?(sizeof(zb_zcl_frame_hdr_full_t)):(sizeof(zb_zcl_frame_hdr_short_t));
    /* we must shure that confirmation size buffer more FC+[ManCode]+Seq+CmdId+TranId */
    ZB_ASSERT((zb_int_t)zb_buf_len(param)>(ptr+sizeof(packet_trans_id) - (zb_uint8_t*)zb_buf_begin(param)));
    ZB_LETOH32(&packet_trans_id, ptr);
    if (packet_trans_id == ZLL_TRAN_CTX().transaction_id)
    {
      /* CR:MINOR Why transaction timeout is scheduled here? Maybe schedule it on request receipt? */
      /* CR:NOTE Both places have the same effect. Should I move it to the request processing routine? */
      ZB_SCHEDULE_ALARM(zll_scan_res_timeout,
                        0,
                        ZB_ZLL_APLC_INTRP_TRANSID_LIFETIME - ZB_ZLL_APLC_SCAN_TIME_BASE_DURATION);
    }
    else
    {
      /* AT: in the beginig of this function we cancel alarm (tranId),
       * so in this place we must nullify transaction id
       */
      if (ZLL_TRAN_CTX().transaction_id)
      {
        zll_scan_res_timeout(0);
      }
    }
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "ERROR could not send ScanResponse, failing transaction", (FMT__0));
    /* AT: in the beginig of this function we cancel alarm (tranId),
     * so in this place we must nullify transaction id
     */
    if (ZLL_TRAN_CTX().transaction_id)
    {
      zll_scan_res_timeout(0);
    }
  }

  zb_buf_free(param);

  TRACE_MSG(TRACE_ZLL1, "< zll_scan_res_sent", (FMT__0));
}/* void zll_scan_res_sent(zb_uint8_t param) */

void zll_scan_res_timeout(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZLL1, "> zll_scan_res_timeout param %hd", (FMT__H, param));

  TRACE_MSG(TRACE_ZLL2, "trans ID %hd task %hd", (FMT__H_H,
      ZLL_TRAN_CTX().transaction_id, ZLL_TRAN_CTX().transaction_task));

  /*
  AT: in case when we here compare current taskId with ZB_ZLL_DEVICE_START_TASK
  unable to start deviceInfo, resetToFactoryNew tasks from controller side (sacn procedure).
  Because after 1st sucessfull touchlink procedure transaction_task always will have
  ZB_ZLL_TRANSACTION_NWK_START_TASK_TGT value (target side)/
  I disable if statement only for debug purpose.
  */
  {
    ZLL_TRAN_CTX().transaction_id = ZLL_NO_TRANSACTION_ID;
  }

  TRACE_MSG(TRACE_ZLL1, "< zll_scan_res_timeout", (FMT__0));
}/* void zll_scan_res_timeout(zb_uint8_t param) */


zb_ret_t zll_handle_devinfo_req(zb_uint8_t param)
{
  zb_zll_commissioning_device_information_req_t request;
  zb_ret_t result = RET_ERROR;
  zb_uint8_t status;
  zb_zcl_parsed_hdr_t cmd_info;
  zb_uint8_t idx;
  zb_uint8_t limit;
  zb_uint8_t* dst_ptr;
  zb_af_endpoint_desc_t* ep_desc;

  TRACE_MSG(TRACE_ZLL1, "> zll_handle_devinfo_req param %hd", (FMT__H, param));

  ZB_ZCL_COPY_PARSED_HEADER(param, &cmd_info);
  ZB_ZLL_COMMISSIONING_GET_DEVICE_INFO_REQ(&request, param, status);

  if (    status != ZB_ZCL_PARSE_STATUS_SUCCESS
      ||  request.trans_id != ZLL_TRAN_CTX().transaction_id
/* CR:MINOR is this check really needed? let's discuss it */
/* CR:NOTE Two distinct initiators are able to generate same transaction identifiers (very rarely,
 * but possible). Totally unique transaction identifier is trans_id + initiator's IEEE address. To
 * save some code, we could assume that we may ignore the probability of generating two same
 * trans_ids on distinct initiators in the same time, couldn't we?
 */
      /* TODO Remove these address checks from everywhere in ZLL.Commissioning: it is practically
       * impossible for two devices to initiate a transaction in the same time, generate two equal
       * transaction identifiers and try to query or something else the same target.
       */
      ||  request.start_idx >= ZCL_CTX().device_ctx->ep_count)
  {
    TRACE_MSG(TRACE_ERROR, "ERROR Wrong or malformed packet, dropping.", (FMT__0));
    zb_buf_free(param);
  }
  else
  {
    limit = ZCL_CTX().device_ctx->ep_count - request.start_idx;
    if (limit > ZB_ZLL_COMMISSIONING_DEVICE_INFO_MAX_RECORDS)
    {
      limit = ZB_ZLL_COMMISSIONING_DEVICE_INFO_MAX_RECORDS;
    }

    ZB_ZLL_COMMISSIONING_INIT_DEVICE_INFO_RES(
        param, dst_ptr, cmd_info.seq_number, request.start_idx, limit);
    for (idx = request.start_idx; idx < limit; ++idx)
    {
      /* TODO: test multi-ep! */
      ep_desc = ZCL_CTX().device_ctx->ep_desc_list[0] + idx;
      ZB_ZLL_COMMISSIONING_ADD_EP_INFO_DEVICE_INFO_RES(
          dst_ptr,
          ep_desc->ep_id,
          ep_desc->profile_id,
          ep_desc->simple_desc->app_device_id,
          ep_desc->simple_desc->app_device_version,
          ep_desc->group_id_count,
          ZB_FALSE);
    }
    ZB_ZLL_COMMISSIONING_SEND_DEVICE_INFO_RES(
        param,
        dst_ptr,
        cmd_info.addr_data.intrp_data.src_addr,
        NULL,
        result);
    if (result != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "ERROR could not send dev info res for sending %hd", (FMT__H, result));
      zb_buf_free(param);
    }
  }

  TRACE_MSG(TRACE_ZLL1, "< zll_handle_devinfo_req", (FMT__0));
  return result;
}/* zb_ret_t zll_handle_devinfo_req(zb_uint8_t param) */

void zll_handle_scan_res(zb_uint8_t param)
{
  zb_zcl_parsed_hdr_t cmd_info;
  zb_zll_commissioning_scan_res_t response;
  zb_uint8_t status;
  zb_uint8_t idx;
  zb_zll_ext_device_info_t* dev_info;

  TRACE_MSG(TRACE_ZLL1, "> zll_handle_scan_res param %hd", (FMT__H, param));

  ZB_ZCL_COPY_PARSED_HEADER(param, &cmd_info);
  /* Without that bzero gcc produces warnings which looks like a false positive */
  ZB_BZERO(&response, sizeof(response));
  ZB_ZLL_COMMISSIONING_GET_SCAN_RES(&response, param, status);

  if ( status != ZB_ZCL_PARSE_STATUS_SUCCESS
       || response.trans_id != ZLL_TRAN_CTX().transaction_id
       || ZLL_TRAN_CTX().transaction_task != ZB_ZLL_DEVICE_DISCOVERY_TASK
       || ZLL_TRAN_CTX().n_device_infos == ZB_ZLL_TRANS_CTX_DEV_INFO_MAX_NUMBER
      )
  {
    TRACE_MSG(TRACE_ERROR, "Wrong or malformed packet, or no storage space left, dropping.", (FMT__0));
    TRACE_MSG(TRACE_ERROR, "%hd %hd %hd %hd %hd %hd %hd %hd", (FMT__H_H_H_H_H_H_H_H,
        status, ZB_ZCL_PARSE_STATUS_SUCCESS,
        response.trans_id, ZLL_TRAN_CTX().transaction_id,
        ZLL_TRAN_CTX().transaction_task, ZB_ZLL_DEVICE_DISCOVERY_TASK,
        ZLL_TRAN_CTX().n_device_infos, ZB_ZLL_TRANS_CTX_DEV_INFO_MAX_NUMBER));
  }
  else
  {
    for (idx = 0; idx < ZLL_TRAN_CTX().n_device_infos; ++idx)
    {
      if (ZB_IEEE_ADDR_CMP(ZLL_TRAN_CTX().device_infos[idx].device_addr,
                           cmd_info.addr_data.intrp_data.src_addr))
      {
        TRACE_MSG(TRACE_ZLL3, "Already have device response, dropping packet.", (FMT__0));
        break;
      }
    }

    if (idx == ZLL_TRAN_CTX().n_device_infos)
    {
      if (idx < ZB_ZLL_TRANS_CTX_DEV_INFO_MAX_NUMBER)
      {
        TRACE_MSG(TRACE_ZLL3, "store device_info (idx %hd)", (FMT__H, idx));
        dev_info = ZLL_TRAN_CTX().device_infos + ZLL_TRAN_CTX().n_device_infos;
        ZB_IEEE_ADDR_COPY(dev_info->device_addr, cmd_info.addr_data.intrp_data.src_addr);
        dev_info->rssi_correction = response.rssi_correction;
        dev_info->zb_info = response.zb_info;
        dev_info->zll_info = response.zll_info;
        dev_info->key_info = response.key_bitmask;
        dev_info->resp_id = response.resp_id;
        ZB_IEEE_ADDR_COPY(dev_info->ext_pan_id, response.ext_pan_id);
        dev_info->nwk_update_id = response.nwk_upd_id;
        dev_info->channel_number = response.channel; /* AT: is it real phy channel index? */
        dev_info->pan_id = response.pan_id;
        dev_info->nwk_addr = response.nwk_addr;
        dev_info->sub_device_count = response.n_subdevs;
        dev_info->total_group_id_count = response.group_ids_total;
        dev_info->ep_info_idx = 0xFF;

#ifdef DEBUG
        if (response.channel != ZB_PIBCACHE_CURRENT_CHANNEL())
        {
          TRACE_MSG(TRACE_ZLL3, "Warning: current chan = %hd; scan resp chan = %hd;", (FMT__H_H, ZB_PIBCACHE_CURRENT_CHANNEL(), response.channel));
        }
#endif /* DEBUG */

        if (1 == response.n_subdevs
            && ZLL_TRAN_CTX().n_ep_infos < ZB_ZLL_TRANS_CTX_EP_INFO_MAX_NUMBER)
        {
          TRACE_MSG(TRACE_ZLL3, "store ep_info (idx %hd)", (FMT__H, ZLL_TRAN_CTX().n_ep_infos));
          ZLL_TRAN_CTX().ep_infos[ZLL_TRAN_CTX().n_ep_infos].ep_id = response.endpoint_info.endpoint_id;
          ZLL_TRAN_CTX().ep_infos[ZLL_TRAN_CTX().n_ep_infos].profile_id = response.endpoint_info.profile_id;
          ZLL_TRAN_CTX().ep_infos[ZLL_TRAN_CTX().n_ep_infos].device_id = response.endpoint_info.device_id;
          ZLL_TRAN_CTX().ep_infos[ZLL_TRAN_CTX().n_ep_infos].version = response.endpoint_info.version;
          ZLL_TRAN_CTX().ep_infos[ZLL_TRAN_CTX().n_ep_infos].group_id_count = response.endpoint_info.n_group_ids;
          ZLL_TRAN_CTX().ep_infos[ZLL_TRAN_CTX().n_ep_infos].dev_idx = ZLL_TRAN_CTX().n_device_infos;

          TRACE_MSG(TRACE_APP1, "profile 0x%x ep %hd", (FMT__D_H, ZLL_TRAN_CTX().ep_infos[ZLL_TRAN_CTX().n_ep_infos].profile_id, ZLL_TRAN_CTX().ep_infos[ZLL_TRAN_CTX().n_ep_infos].ep_id));

          dev_info->ep_info_idx = ZLL_TRAN_CTX().n_ep_infos;

          ++ZLL_TRAN_CTX().n_ep_infos;
        }
        ++ZLL_TRAN_CTX().n_device_infos;
      }
      else
      {
        TRACE_MSG(TRACE_ERROR, "No space left to store device info record, dropping.", (FMT__0));
        ZLL_TRAN_CTX().out_of_memory = 1;
      }
    }

    /* AT: code with nwkUpdateReq (change channel unicast interpan command) was moved into zll_add_device_to_network function */
  }

  zb_buf_free(param);
  TRACE_MSG(TRACE_ZLL1, "< zll_handle_scan_res", (FMT__0));
}/* void zll_handle_scan_res(zb_uint8_t param) */

void zll_handle_devinfo_res(zb_uint8_t param)
{
  zb_zll_commissioning_device_information_common_res_t response;
  zb_zll_commissioning_device_information_ep_info_res_t ep_info;
  zb_zcl_parsed_hdr_t cmd_info;
  zb_uint8_t status;
  zb_uint8_t idx;
  zb_zll_sub_device_info_t* ep_info_record;

  TRACE_MSG(TRACE_ZLL1, "> zll_handle_devinfo_res param %hd", (FMT__H, param));

  ZB_ZCL_COPY_PARSED_HEADER(param, &cmd_info);
  ZB_ZLL_COMMISSIONING_GET_DEVICE_INFO_RES(&response, param, status);

  if (    status != ZB_ZCL_PARSE_STATUS_SUCCESS
      ||  ZLL_TRAN_CTX().transaction_id != response.trans_id
      ||  ZLL_TRAN_CTX().transaction_task != ZB_ZLL_DEVICE_DISCOVERY_TASK
      ||  ZLL_TRAN_CTX().n_ep_infos >= ZB_ZLL_TRANS_CTX_EP_INFO_MAX_NUMBER)
  {
    TRACE_MSG(TRACE_ERROR, "Wrong or malformed packet, dropping.", (FMT__0));
    TRACE_MSG(TRACE_ERROR, "%hd %hd %hd %hd %hd %hd %hd", (FMT__H_H_H_H_H_H_H,
        status, ZB_ZCL_PARSE_STATUS_SUCCESS,
        ZLL_TRAN_CTX().transaction_id,
        ZLL_TRAN_CTX().transaction_task, ZB_ZLL_DEVICE_DISCOVERY_TASK,
        ZLL_TRAN_CTX().n_ep_infos, ZB_ZLL_TRANS_CTX_DEV_INFO_MAX_NUMBER));
    zb_buf_free(param);
  }
  else
  {
    for (ZLL_TRAN_CTX().current_dev_info_idx = 0;
         ZLL_TRAN_CTX().current_dev_info_idx < ZLL_TRAN_CTX().n_device_infos;
         ++ZLL_TRAN_CTX().current_dev_info_idx)
    {
      if (ZB_IEEE_ADDR_CMP(
            ZLL_TRAN_CTX().device_infos[ZLL_TRAN_CTX().current_dev_info_idx].device_addr,
            cmd_info.addr_data.intrp_data.src_addr))
      {
        break;
      }
    }
    if (ZLL_TRAN_CTX().current_dev_info_idx < ZLL_TRAN_CTX().n_device_infos)
    {
      for (idx = 0;
           idx < response.n_records
           && ZLL_TRAN_CTX().n_ep_infos < ZB_ZLL_TRANS_CTX_EP_INFO_MAX_NUMBER;
           ++idx)
      {
        ZB_ZLL_COMMISSIONING_GET_NEXT_EP_INFO_DEVICE_INFO_RES(&ep_info, param, status);
        if (status == ZB_ZCL_PARSE_STATUS_FAILURE)
        {
          break;
        }
        ep_info_record = ZLL_TRAN_CTX().ep_infos + ZLL_TRAN_CTX().n_ep_infos;
        ++ZLL_TRAN_CTX().n_ep_infos;
        /* We have no content for ep_info_record->dev_idx in the structure being copied. */
        ep_info_record->ep_id = ep_info.ep_id;
        ep_info_record->profile_id = ep_info.profile_id;
        ep_info_record->device_id = ep_info.device_id;
        ep_info_record->version = ep_info.version;
        ep_info_record->group_id_count = ep_info.group_id_count;
        ep_info_record->dev_idx = ZLL_TRAN_CTX().current_dev_info_idx;
      }
    }
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZLL1, "<zll_handle_devinfo_res", (FMT__0));
}/* void zll_handle_devinfo_res(zb_uint8_t param) */

#ifndef ZB_BDB_TOUCHLINK
void zll_send_next_scan_req_continue(zb_uint8_t param)
{

  TRACE_MSG(TRACE_ZLL1, "> zll_send_next_scan_req_continue param %hd", (FMT__H, param));

  ++ZLL_TRAN_CTX().pckt_cnt;
  ZB_ZLL_COMMISSIONING_SEND_SCAN_REQ(param, zll_scan_req_sent);
  ZB_SCHEDULE_ALARM(zll_scan_req_guard, 0, ZB_ZLL_APLC_SCAN_TIME_BASE_DURATION);

  TRACE_MSG(TRACE_ZLL1, "< zll_send_next_scan_req_continue", (FMT__0));
}/* void zll_send_next_scan_req_continue(zb_uint8_t param) */
#endif

void zb_zll_commissioning_send_scan_req(zb_bufid_t buffer, zb_callback_t callback)
{
  zb_uint8_t* data_ptr;
  zb_intrp_data_req_t* request;
  data_ptr = ZB_ZCL_START_PACKET((buffer));
  request = ZB_BUF_GET_PARAM((buffer), zb_intrp_data_req_t);
  ZB_BZERO(request, sizeof(*request));
  ZB_ZCL_CONSTRUCT_SPECIFIC_COMMAND_REQ_FRAME_CONTROL_O(data_ptr, ZB_ZCL_DISABLE_DEFAULT_RESPONSE);
  ZB_ZCL_CONSTRUCT_COMMAND_HEADER(
      data_ptr,
      ZB_ZCL_GET_SEQ_NUM(),
      ZB_ZLL_CMD_COMMISSIONING_SCAN_REQ);
  ZB_ZCL_PACKET_PUT_DATA32_VAL(data_ptr, ZB_ZLL_GET_NEW_TRANS_ID());
  ZLL_DEVICE_INFO().transaction_id = ZLL_TRAN_CTX().transaction_id;
  ZB_ZCL_PACKET_PUT_DATA8(data_ptr, ZB_ZLL_ZB_INFO_CURRENT_VALUE());
  ZB_ZCL_PACKET_PUT_DATA8(data_ptr, ZLL_DEVICE_INFO().zll_info);
  ZB_ZCL_FINISH_PACKET_O((buffer), data_ptr);
  request->dst_addr_mode = ZB_INTRP_ADDR_NETWORK;
  request->dst_pan_id = request->dst_addr.addr_short = ZB_INTRP_BROADCAST_SHORT_ADDR;
  request->profile_id = ZB_AF_ZLL_PROFILE_ID;
  request->cluster_id = ZB_ZCL_CLUSTER_ID_TOUCHLINK_COMMISSIONING;
  request->asdu_handle = 0;
  zb_zcl_register_cb(buffer, (callback));
  ZB_SCHEDULE_CALLBACK(zb_intrp_data_request, buffer);
}


#endif /* defined ZB_ENABLE_ZLL */
