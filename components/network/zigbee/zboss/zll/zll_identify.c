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
/* PURPOSE: ZLL Commissioning cluster: Identify process implementation
*/

#define ZB_TRACE_FILE_ID 2114
#include "zb_common.h"
#include "zb_zcl.h"
#include "zll/zb_zll_common.h"
#include "zll/zll_commissioning_internals.h"

#if defined ZB_ENABLE_ZLL

void zll_identify_req_confirm(zb_uint8_t param);

zb_ret_t zb_zll_identify(zb_uint8_t param)
{
  zb_bufid_t  buffer = param;
  zb_zll_commissioning_identify_req_param_t request;
  zb_ret_t result = RET_ERROR;

  TRACE_MSG(TRACE_ZLL1, "> zb_zll_identify param %hd", (FMT__H, param));

  ZB_ZLL_COMMISSIONING_GET_IDENTIFY_REQ_PARAM(&request, buffer);

  if (!ZLL_TRAN_CTX().transaction_id)
  {
    TRACE_MSG(TRACE_ERROR, "ERROR no active transaction or not an initiator.", (FMT__0));
    result = RET_INVALID_STATE;
  }
  else if (   (ZLL_TRAN_CTX().identify_sent && ! request.identify_time)
           || (! ZLL_TRAN_CTX().identify_sent && request.identify_time))
  {
    ZLL_TRAN_CTX().transaction_task = ZB_ZLL_IDENTIFY_TASK;

    ZB_ZLL_COMMISSIONING_SEND_IDENTIFY_REQ(
        buffer, &request.identify_time, request.dst_addr, zll_identify_req_confirm, result);
    if (result != RET_OK && ZLL_DEVICE_INFO().report_task_result)
    {
      zll_notify_task_result(buffer, ZB_ZLL_TASK_STATUS_SCHEDULE_FAILED);
    }
  }
  else
  {
    TRACE_MSG(
        TRACE_ERROR,
        "ERROR invalid state: ZLL_TRAN_CTX().identify_sent: %d, request.identify_time %d",
        (FMT__D_D, ZLL_TRAN_CTX().identify_sent, request.identify_time));
    result = RET_INVALID_STATE;
  }

  TRACE_MSG(TRACE_ZLL1, "< zb_zll_identify result %d", (FMT__D, result));
  return result;
}

void zll_identify_req_confirm(zb_uint8_t param)
{
  zb_bufid_t  buffer = param;
  zb_uint8_t status = zb_buf_get_status(param);

  TRACE_MSG(TRACE_ZLL1, "> zll_identify_req_confirm param %hd", (FMT__H, param));

  zll_notify_task_result(
      buffer,
      (   (status == RET_OK )
       ?  ZB_ZLL_TASK_STATUS_FINISHED
       : ZB_ZLL_TASK_STATUS_FAILED));

  TRACE_MSG(TRACE_ZLL1, "< zll_identify_req_confirm", (FMT__0));
}/* void zll_identify_req_confirm(zb_uint8_t param) */

/* TODO remove 1 after debugging */

void zb_zll_identify_time_handler(zb_uint8_t param);

/**
 *  @brief Handles ZLL Commissioning Identify command.
 *  Implements identify process utilizing ZCL Identify cluster functionality.
 *  @param param - reference to the buffer containing @ref zb_zll_commissioning_identify_req_s
 *  structure allocated, and @ref zb_zcl_parsed_hdr_s structure as buffer parameter.
 *  @return Command processing status.
 */
zb_ret_t zb_zll_identify_handler(zb_uint8_t param)
{
  zb_ret_t result = RET_ERROR;
  zb_uint8_t status;
  zb_zll_commissioning_identify_req_t request;
  zb_zcl_parsed_hdr_t cmd_info;
  zb_bufid_t  buffer = param;

  TRACE_MSG(TRACE_ZLL1, "> zb_zll_identify_handler param %hd", (FMT__H, param));

  ZB_ZCL_COPY_PARSED_HEADER(buffer, &cmd_info);
  ZB_ZLL_COMMISSIONING_GET_IDENTIFY_REQ(&request, buffer, status);

  if (status == ZB_ZCL_PARSE_STATUS_SUCCESS)
  {
    TRACE_MSG(TRACE_ZLL3, "req duration %d trid %l", (FMT__D_L, request.duration, request.trans_id));

    if (request.duration == ZB_ZLL_IDENTIFY_DEFAULT_TIME)
    {
      request.duration = ZLL_DEVICE_INFO().identify_duration;
    }

    TRACE_MSG(TRACE_ZCL3, "Identify requested for %ds", (FMT__D, request.duration));

    ZB_SCHEDULE_ALARM_CANCEL(zb_zll_identify_time_handler, ZB_ALARM_ANY_PARAM);
    if (request.duration)
    {
      ZB_SCHEDULE_ALARM(zb_zll_identify_time_handler,
                        0,
                        ZB_TIME_ONE_SECOND * request.duration);
    }
    if (ZLL_DEVICE_INFO().identify_handler)
    {
      ZB_SCHEDULE_CALLBACK(ZLL_DEVICE_INFO().identify_handler, (0 != request.duration));
    }
  }
  //AT: here we should free incoming buffer with command payload, otherwise we get deadlock state
  zb_buf_free(param);

  TRACE_MSG(TRACE_ZLL1, "< zb_zll_identify_handler result %d", (FMT__D, result));
  return result;
}/* zb_ret_t zb_zll_identify_handler(zb_uint8_t param) */


void zb_zll_identify_time_handler(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZCL1, "> zb_zll_identify_time_handler %hd", (FMT__H, param));

  if (ZLL_DEVICE_INFO().identify_handler)
  {
    ZB_SCHEDULE_CALLBACK(ZLL_DEVICE_INFO().identify_handler, ZB_FALSE);
  }

  TRACE_MSG(TRACE_ZCL1, "< zb_zll_identify_time_handler", (FMT__0));
}/* void zb_zll_identify_time_handler(zb_uint8_t param) */


#endif /* defined ZB_ENABLE_ZLL */
