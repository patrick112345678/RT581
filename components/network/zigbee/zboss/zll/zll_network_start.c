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
/* PURPOSE: ZLL Commissioning cluster - network start command processing.
*/

#define ZB_TRACE_FILE_ID 2117
#include "zb_common.h"
#include "zb_zcl.h"
#include "zll/zb_zll_common.h"
#include "zll/zb_zll_nwk_features.h"
#include "zll/zll_commissioning_internals.h"
#include "zb_zdo.h"
#include "zll/zb_zll_sas.h"

#ifdef ZB_BDB_TOUCHLINK
#include "zb_bdb_internal.h"
#endif /* ZB_BDB_TOUCHLINK */

#if defined ZB_ENABLE_ZLL

#define ZB_ZLL_MAX_NON_BROADCAST_ADDRESS    0xfffd

/** @internal @brief A placeholder variable to temporarily store encrypted network key. */

/**
 *  @internal @brief Parses parameters for @ref zb_zll_start_new_network() call.
 *  @param buffer [IN] - pointer to the @ref zb_buf_t "buffer" containing call parameters (of type
 *  zb_zll_start_new_nwk_param_t) in its body.
 *  @param data_ptr [IN] - pointer to the variable of type @ref zb_zll_start_new_nwk_param_t to put
 *  call parameters to.
 *  @param status [OUT] - @ref zb_zcl_parse_status_e "parse status."
 */
/*#define ZLL_COMMISSIONING_GET_PARAM_START_NEW_NETWORK_REQ(buffer, data_ptr, status)    \
{                                                                                        \
  if (zb_buf_len(buffer) != sizeof(zb_zll_start_new_nwk_param_t))                        \
  {                                                                                      \
    (status) = ZB_ZCL_PARSE_STATUS_FAILURE;                                              \
  }                                                                                      \
  else                                                                                   \
  {                                                                                      \
    (status) = ZB_ZCL_PARSE_STATUS_SUCCESS;                                              \
    ZB_MEMCPY((data_ptr), zb_buf_begin((buffer)), sizeof(zb_zll_start_new_nwk_param_t)); \
  }                                                                                      \
}
*/
void zll_network_start_req_confirm(zb_uint8_t param);

/**
 *  @internal
 *  @brief Start new network request-to-response time guard.
 *  @param param [IN] - formal parameter for asyncronous Zigbee calls, not used.
 */
void zll_start_new_nwk_time_guard(zb_uint8_t param);

/**
 *  @brief Handles network start response after waiting for target actions.
 *  Target supposed to start a new network to the time the function would be executed.
 *  @param param [IN] - reference to the @ref zb_buf_t "buffer" to put rejoin network request to.
 */
void zll_network_start_res_handler_continue(zb_uint8_t param);

void zll_network_start_res_confirm(zb_uint8_t param);

zb_ret_t zb_zll_start_new_network(zb_uint8_t param)
{
  zb_ret_t result = RET_ERROR;
  zb_zll_start_new_nwk_param_t req_param;
  zb_zll_group_id_range_t group_id_range;
  zb_zll_group_id_range_t free_group_id_range;
  zb_zll_addr_range_t free_nwk_addr_range;

  TRACE_MSG(TRACE_ZLL1, "> zb_zll_start_new_network param %hd", (FMT__H, param));

  if (!ZLL_TRAN_CTX().transaction_id)
  {
    TRACE_MSG(TRACE_ERROR, "ERROR no active transaction.", (FMT__0));
    zb_buf_free(param);
    result = RET_INVALID_STATE;
  }
  else
  {
    ZB_MEMCPY(&req_param, ZB_BUF_GET_PARAM(param, zb_zll_start_new_nwk_param_t),
                                                sizeof(zb_zll_start_new_nwk_param_t));

    ZLL_TRAN_CTX().transaction_task = ZB_ZLL_TRANSACTION_NWK_START_TASK;

    ZB_ZLL_FIND_DEVICE_INFO_BY_ADDR(req_param.dst_addr, result);
    if (result == RET_ERROR)
    {
      TRACE_MSG(TRACE_ERROR, "ERROR find device info", (FMT__0));
      zb_buf_free(param);
      result = RET_INVALID_PARAMETER;
    }
    else
    {
      ZLL_ACTION_SEQUENCE_INIT();

      zll_get_group_id_range(&group_id_range, ZB_ZLL_TRANS_GET_CURRENT_DEV_GROUP_ID_RANGE_LEN());

      if (ZB_ZLL_TRANS_GET_CURRENT_DEV_ADDR_ASSIGN_CAPABILITY())
      {
        zll_get_group_id_range(&free_group_id_range, ZB_ZLL_GROUP_ID_RANGE_ALLOC_LENGTH);
        zll_get_addr_range(&free_nwk_addr_range);
      }
      else
      {
        free_group_id_range.group_id_begin = free_group_id_range.group_id_end = 0;
        free_nwk_addr_range.addr_begin = free_nwk_addr_range.addr_end = 0;
      }

      ZB_IEEE_ADDR_COPY(ZLL_TRAN_CTX().command_data.start_new_nwk.target_ieee_addr,
                        req_param.dst_addr);

      ZB_ZLL_COMMISSIONING_SEND_NETWORK_START_REQ(
          param,
          req_param.ext_pan_id,
          ZLL_DEVICE_INFO().key_index,
          ZLL_DEVICE_INFO().encr_nwk_key,
          (ZLL_DEVICE_INFO().nwk_channel) ? ZLL_DEVICE_INFO().nwk_channel : req_param.channel,
          (req_param.pan_id==ZB_ZLL_SAS_PAN_ID ? 0 : req_param.pan_id),
          zll_get_new_addr(),
          group_id_range.group_id_begin,
          group_id_range.group_id_end,
          free_nwk_addr_range.addr_begin,
          free_nwk_addr_range.addr_end,
          free_group_id_range.group_id_begin,
          free_group_id_range.group_id_end,
          req_param.dst_addr,
          zll_network_start_req_confirm,
          result);

      if (result != RET_OK)
      {
        result = RET_ERROR;
        ZLL_ACTION_SEQUENCE_ROLLBACK();
        /* TODO possibly issue transaction state changed callback? */
      }
      else
      {
        ZB_SCHEDULE_ALARM(zll_start_new_nwk_time_guard,
                          ZB_ALARM_ANY_PARAM,
                          ZB_ZLL_APLC_RX_WINDOW_DURATION);
      }
    }
  }

  TRACE_MSG(TRACE_ZLL1, "< zb_zll_start_new_network result %d", (FMT__D, result));

  return result;
}/* zb_ret_t zb_zll_start_new_network(zb_uint8_t param) */

void zll_start_new_nwk_time_guard(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZLL1, "> zll_start_new_nwk_time_guard param %hd", (FMT__H, param));

  if (param == ZB_ALARM_ANY_PARAM)
  {
    zb_buf_get_out_delayed(zll_start_new_nwk_time_guard);
  }
  else
  {
    zll_notify_task_result(param, ZB_ZLL_TASK_STATUS_FAILED);
  }

  TRACE_MSG(TRACE_ZLL1, "< zll_start_new_nwk_time_guard", (FMT__0));
}/* void zll_start_new_nwk_time_guard(zb_uint8_t param) */


void zll_network_start_req_confirm(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZLL1, "> zll_network_start_req_confirm param %hd", (FMT__H, param));

  if (zb_buf_get_status(param))
  {
    TRACE_MSG(TRACE_ERROR, "ERROR could not send start new nwk req", (FMT__0));
    ZLL_ACTION_SEQUENCE_ROLLBACK();
    zll_notify_task_result(param, ZB_ZLL_TASK_STATUS_FAILED);
  }
  else
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZLL1, "< zll_network_start_req_confirm", (FMT__0));
}/* void zll_network_start_req_confirm(zb_uint8_t param) */

zb_ret_t zll_network_start_res_handler(zb_uint8_t param)
{
  zb_zll_commissioning_network_start_res_t response;
  zb_zcl_parsed_hdr_t cmd_info;
  zb_ret_t status;

  TRACE_MSG(TRACE_ZLL1, "> zll_network_start_res_handler param %hd", (FMT__H, param));

  ZB_ZCL_COPY_PARSED_HEADER(param, &cmd_info);
  ZB_ZLL_COMMISSIONING_GET_NETWORK_START_RES(&response, param, status);

  if (status == ZB_ZCL_PARSE_STATUS_FAILURE)
  {
    status = RET_ILLEGAL_REQUEST;
    TRACE_MSG(TRACE_ERROR, "ERROR could not parse start new network response.", (FMT__0));
    zb_buf_free(param);
  }
  else if (   response.trans_id != ZLL_TRAN_CTX().transaction_id
           || ZLL_TRAN_CTX().transaction_task != ZB_ZLL_TRANSACTION_NWK_START_TASK)
  {
    status = RET_INVALID_STATE;
    TRACE_MSG(
        TRACE_ZLL1,
        "ERROR invalid transaction state 0x%04x",
        (FMT__D, ZLL_TRAN_CTX().transaction_task));
    zb_buf_free(param);
  }
  else
  {
    ZB_SCHEDULE_ALARM_CANCEL(zll_start_new_nwk_time_guard, ZB_ALARM_ANY_PARAM);

    if (response.status) /* Start new network failed. */
    {
      status = RET_ERROR;
      TRACE_MSG(TRACE_ZLL1, "ERROR Target failed to start new network", (FMT__0));
      ZLL_ACTION_SEQUENCE_ROLLBACK();
      zll_notify_task_result(param, ZB_ZLL_TASK_STATUS_FAILED);
    }
    else
    {
      ZB_EXTPANID_COPY(ZLL_TRAN_CTX().command_data.start_new_nwk.ext_pan_id, response.ext_pan_id);
      ZLL_TRAN_CTX().command_data.start_new_nwk.pan_id = response.pan_id;
      ZLL_TRAN_CTX().command_data.start_new_nwk.channel = response.channel;
      status = zb_schedule_alarm(zll_network_start_res_handler_continue, param,
                                 ZB_ZLL_APLC_MIN_STARTUP_DELAY_TIME);
    }
  }

  TRACE_MSG(TRACE_ZLL1, "< zll_network_start_res_handler status %d", (FMT__D, status));
  return status;
}/* zb_ret_t zll_network_start_res_handler(zb_uint8_t param) */

void zll_network_started_signal(zb_uint8_t param)
{
  zb_ret_t status = RET_ERROR;
  zb_zll_sub_device_info_t *dev_info = ZB_ZLL_TRANS_GET_CURRENT_DEV_EP_INFO();
  zb_bdb_signal_touchlink_nwk_started_params_t *sig_params =
    (zb_bdb_signal_touchlink_nwk_started_params_t *)zb_app_signal_pack(
      param,
      ZB_BDB_SIGNAL_TOUCHLINK_NWK_STARTED,
      RET_OK,
      sizeof(zb_bdb_signal_touchlink_nwk_started_params_t));

  TRACE_MSG(TRACE_ZLL1, "zll_network_started_signal", (FMT__0));
  ZB_IEEE_ADDR_COPY(sig_params->device_ieee_addr, ZLL_TRAN_CTX().command_data.start_new_nwk.target_ieee_addr);
/* 04/03/2018 EE CR:MINOR Why that "find.." is a macro? It should be a function. */
  ZB_ZLL_FIND_DEVICE_INFO_BY_ADDR(ZLL_TRAN_CTX().command_data.start_new_nwk.target_ieee_addr, status);
  ZB_ASSERT((status == RET_OK) && (dev_info != NULL));
  sig_params->endpoint = dev_info->ep_id;
  sig_params->profile_id = dev_info->profile_id;
  TRACE_MSG(TRACE_APP1, "profile 0x%x ep %hd", (FMT__D_H, sig_params->profile_id, sig_params->endpoint));
  ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete, param);
}

void zll_network_start_res_handler_continue(zb_uint8_t param)
{
  zb_uint8_t status;

  TRACE_MSG(TRACE_ZLL1, "> zll_network_start_res_handler_continue param %hd", (FMT__H, param));

  zll_save_nwk_prefs(ZLL_TRAN_CTX().command_data.start_new_nwk.ext_pan_id,
                     ZLL_TRAN_CTX().command_data.start_new_nwk.pan_id,
                     ZLL_TRAN_CTX().command_data.start_new_nwk.short_addr,
                     ZLL_TRAN_CTX().command_data.start_new_nwk.channel);

  /* NK: Hope we will not start new network before sending this signal... */
  zb_buf_get_out_delayed(zll_network_started_signal);

  /* Rejoin as authenticated device */
  ZG->aps.authenticated = ZB_TRUE;
/* 04/03/2018 EE CR:MINOR Is it ok that we send to app signal before completed rejoin? */
  ZLL_REJOIN_NWK(
      param,
      ZLL_TRAN_CTX().command_data.start_new_nwk.ext_pan_id,
      ZLL_TRAN_CTX().command_data.start_new_nwk.channel,
      status);
  if (status != RET_OK)
  {
    zll_notify_task_result(param, ZB_ZLL_TASK_STATUS_FAILED);
    /* TODO Fail a transaction? */
  }

  ZB_ZLL_CLEAR_FACTORY_NEW();

  TRACE_MSG(TRACE_ZLL1, "< zll_network_start_res_handler_continue", (FMT__0));
}/* void zll_network_start_res_handler_continue(zb_uint8_t param) */

zb_ret_t zll_network_start_req_handler(zb_uint8_t param)
{
  zb_ret_t result = RET_ERROR;
  zb_zcl_parsed_hdr_t cmd_info;
  zb_zll_commissioning_network_start_req_t request;
  zb_uint8_t nwk_status;
  zb_nlme_network_discovery_request_t* nlme_req;

  TRACE_MSG(TRACE_ZLL1, "> zll_network_start_req_handler param %hd", (FMT__H, param));

  ZB_ZCL_COPY_PARSED_HEADER(param, &cmd_info);
  ZB_ZLL_COMMISSIONING_GET_NETWORK_START_REQ(&request, param, result);

  if (result == ZB_ZCL_PARSE_STATUS_FAILURE)
  {
    result = RET_INVALID_PARAMETER;
    TRACE_MSG(TRACE_ERROR, "ERROR could not parse request.", (FMT__0));
    zb_buf_free(param);
  }
  else if (ZLL_TRAN_CTX().transaction_id != request.trans_id)
  {
    result = RET_INVALID_STATE;
    TRACE_MSG(TRACE_ERROR, "ERROR request doesn't correspond to the current transaction", (FMT__0));
    zb_buf_free(param);
  }
  else
  {
    nwk_status = (    ZLL_TRAN_CTX().check_action_allowed
                  &&  ! ZLL_TRAN_CTX().check_action_allowed(ZB_ZLL_ACTION_START_NEW_NETWORK));

    if (! nwk_status) /* Action allowed by application. */
    {
      ZLL_TRAN_CTX().transaction_task = ZB_ZLL_TRANSACTION_NWK_START_TASK_TGT;

      ZLL_TRAN_CTX().command_data.start_new_nwk.seq_number = cmd_info.seq_number;
      ZLL_TRAN_CTX().command_data.start_new_nwk.pan_id = request.pan_id;
      ZLL_TRAN_CTX().command_data.start_new_nwk.channel = request.channel;
      ZLL_TRAN_CTX().command_data.start_new_nwk.short_addr = request.short_addr;
      ZB_IEEE_ADDR_COPY(ZLL_TRAN_CTX().src_addr, cmd_info.addr_data.intrp_data.src_addr);
      ZB_EXTPANID_COPY(ZLL_TRAN_CTX().command_data.start_new_nwk.ext_pan_id, request.ext_pan_id);

      if (ZB_NIB_SECURITY_LEVEL())
      {
        zll_calc_enc_dec_nwk_key(request.encr_nwk_key,
                                 secur_nwk_key_by_seq(ZB_NIB().active_key_seq_number),
                                 request.key_idx,
                                 ZLL_DEVICE_INFO().transaction_id,
                                 ZLL_DEVICE_INFO().response_id,
                                 ZB_FALSE);
        ZG->aps.authenticated = ZB_TRUE;
      }

      zb_buf_reuse(param);
      nlme_req = ZB_BUF_GET_PARAM(param, zb_nlme_network_discovery_request_t);
      ZB_BZERO(nlme_req, sizeof(zb_nlme_network_discovery_request_t));

#ifndef ZB_BDB_TOUCHLINK
      nlme_req->scan_channels = zb_aib_channel_page_list_get_2_4GHz_mask()/* MMDEVSTUBS */ & ZB_ZLL_PRIMARY_CHANNEL_MASK;
#else
      if (ZB_BDB().ignore_aps_channel_mask)
      {
        zb_channel_page_list_set_2_4GHz_mask(nlme_req->scan_channels_list, ZB_BDBC_TL_PRIMARY_CHANNEL_SET);
      }
      else
      {
        zb_channel_page_list_set_2_4GHz_mask(nlme_req->scan_channels_list,
                                             zb_aib_channel_page_list_get_2_4GHz_mask() & ZB_BDBC_TL_PRIMARY_CHANNEL_SET);
      }
#endif
      nlme_req->scan_duration = ZB_ZLL_NWK_DISC_DURATION;
      TRACE_MSG(TRACE_ZLL3, "Starting nwk discovery, scan_attempts %hd",
                (FMT__H, COMM_CTX().discovery_ctx.nwk_scan_attempts));
      COMM_CTX().discovery_ctx.nwk_scan_attempts = 2;
      COMM_CTX().discovery_ctx.disc_count = COMM_CTX().discovery_ctx.nwk_scan_attempts;
      nwk_status = (zb_schedule_callback(zb_nlme_network_discovery_request, param) != RET_OK);
    }

    /* MP: looks inobviously, but we must report that we could not schedule network discovery.
     * Local application will be notified from zll_network_start_res_confirm() callback.
     */
    if (nwk_status)
    {
      ZB_ZLL_COMMISSIONING_SEND_NETWORK_START_RES(
          param,
          cmd_info.seq_number,
          nwk_status,
          request.ext_pan_id,
          request.pan_id,
          request.channel,
          cmd_info.addr_data.intrp_data.src_addr,
          zll_network_start_res_confirm,
          result);
    }
  }

  TRACE_MSG(TRACE_ZLL1, "< zll_network_start_req_handler result %d", (FMT__D, result));

  return result;
}/* zb_ret_t zll_network_start_req_handler(zb_uint8_t param) */

void zll_network_start_res_confirm(zb_uint8_t param)
{
  zb_zll_commissioning_network_start_res_t response;
  zb_uint8_t status;

  TRACE_MSG(TRACE_ZLL1, "> zll_network_start_res_confirm param %hd", (FMT__H, param));

  if (zb_buf_get_status(param))
  {
    /* TODO Fail a transaction? */
    TRACE_MSG(TRACE_ERROR, "ERROR Could not send response.", (FMT__0));
    zll_notify_task_result(param, ZB_ZLL_TASK_STATUS_FAILED);
  }
  else
  {
    // cut header and cmd_id fields
    zb_buf_cut_left(param, ZB_ZCL_FRAME_HDR_GET_SIZE(*(zb_uint8_t*)zb_buf_begin(param))+1);

    ZB_ZLL_COMMISSIONING_GET_NETWORK_START_RES(&response, param, status);
    if (status)
    {
      TRACE_MSG(TRACE_ZLL3, "Decided not to create network %hd.", (FMT__H, status));
      zll_notify_task_result(param, ZB_ZLL_TASK_STATUS_FINISHED);
    }
    else
    {
      if (! (ZLL_DEVICE_INFO().zll_info & ZB_ZLL_INFO_FACTORY_NEW))
      {
        // save parameters for start router
        ZB_EXTPANID_COPY(ZLL_TRAN_CTX().command_data.start_new_nwk.ext_pan_id, response.ext_pan_id);
        ZLL_TRAN_CTX().command_data.start_new_nwk.pan_id = response.pan_id;
        ZLL_TRAN_CTX().command_data.start_new_nwk.channel = response.channel;

        /* after send LEAVE-Req we'll start router by calling macro ZB_ZLL_START_ROUTER
        see functions: zb_nlme_leave_confirm and zll_leave_nwk_confirm
        */
        ZLL_LEAVE_NWK(param, status);
        if (status != RET_OK)
        {
          zll_notify_task_result(param, ZB_ZLL_TASK_STATUS_FAILED);
          /* TODO Fail transaction? */
        }
      }
#if defined ZB_ROUTER_ROLE
      else
      {
        ZB_SET_JOINED_STATUS(ZB_TRUE);

        ZB_ZLL_CLEAR_FACTORY_NEW();

        ZB_ZLL_START_ROUTER(param, response.ext_pan_id, response.pan_id, response.channel,
            ZLL_TRAN_CTX().command_data.start_new_nwk.short_addr, status);
      }
#endif /* defined ZB_ROUTER_ROLE */
    }
  }

  TRACE_MSG(TRACE_ZLL1, "< zll_network_start_res_confirm", (FMT__0));
}/* void zll_network_start_res_confirm(zb_uint8_t param) */

void zll_network_start_continue(zb_uint8_t param)
{
  zb_nlme_network_discovery_confirm_t *cnf;
  zb_nlme_network_descriptor_t *dsc;
  zb_uint8_t pan_id_generated = ZB_FALSE;
  zb_uint8_t idx_map;
  zb_uint8_t idx_conf;
  zb_uint8_t check_failed = ZB_FALSE;
  zb_uint8_t result = 0;

  ZVUNUSED(result);

  TRACE_MSG(TRACE_ZLL1, "> zll_network_start_continue param %hd", (FMT__H, param));

  /* TODO Figure out: can network discovery fail? */

  if (! ZLL_TRAN_CTX().command_data.start_new_nwk.channel)
  {
    ZLL_TRAN_CTX().command_data.start_new_nwk.channel = zll_random_primary_channel();
  }

  /* MP: It is not obvious how we could gain a new extended PAN Id. This code block was inspired by
   * ZB spec, subclause 3.2.2.3.3.
   */
  if (ZB_EXTPANID_IS_ZERO(ZLL_TRAN_CTX().command_data.start_new_nwk.ext_pan_id))
  {
    ZB_EXTPANID_COPY(ZLL_TRAN_CTX().command_data.start_new_nwk.ext_pan_id,
                     ZB_PIBCACHE_EXTENDED_ADDRESS());
  }
  cnf = (zb_nlme_network_discovery_confirm_t *)zb_buf_begin(param);
  dsc = (zb_nlme_network_descriptor_t *)(cnf + 1);

  if (! ZLL_TRAN_CTX().command_data.start_new_nwk.pan_id)
  {
    pan_id_generated = ZB_TRUE;
  }

  do
  {
    while (! ZLL_TRAN_CTX().command_data.start_new_nwk.pan_id &&
        ZLL_TRAN_CTX().command_data.start_new_nwk.pan_id < ZB_ZLL_MAX_NON_BROADCAST_ADDRESS)
    {
      ZLL_TRAN_CTX().command_data.start_new_nwk.pan_id = ZB_RANDOM_U16();
    }
    for (idx_map = 0; idx_map < ZB_PANID_TABLE_SIZE; ++idx_map)
    {

      if (ZG->addr.pan_map[idx_map].short_panid ==
          ZLL_TRAN_CTX().command_data.start_new_nwk.pan_id)
      {
        for (idx_conf = 0; idx_conf < cnf->network_count; ++idx_conf)
        {
          zb_ext_pan_id_t extended_pan_id;

          zb_address_get_pan_id((dsc+idx_conf)->panid_ref, extended_pan_id);

          if (ZB_EXTPANID_CMP(extended_pan_id,
                                  ZG->addr.pan_map[idx_map].long_panid)
              && (dsc+idx_conf)->logical_channel ==
                      ZLL_TRAN_CTX().command_data.start_new_nwk.channel)
          {
            if (pan_id_generated)
            {
              ZLL_TRAN_CTX().command_data.start_new_nwk.pan_id = 0;
            }
            TRACE_MSG(TRACE_ZLL1, "check_failed %hd", (FMT__H, check_failed));
            check_failed = ZB_TRUE;
            goto exit_cycles;
          }
        }
      }
    }
    check_failed = ZB_FALSE;
exit_cycles:
    ;
  }
  while (! ZLL_TRAN_CTX().command_data.start_new_nwk.pan_id);

  TRACE_MSG(TRACE_ZLL1, "zll_network_start_continue check_failed %hd", (FMT__H, check_failed));
  ZB_ZLL_COMMISSIONING_SEND_NETWORK_START_RES(
      param,
      ZLL_TRAN_CTX().command_data.start_new_nwk.seq_number,
      check_failed,
      ZLL_TRAN_CTX().command_data.start_new_nwk.ext_pan_id,
      ZLL_TRAN_CTX().command_data.start_new_nwk.pan_id,
      ZLL_TRAN_CTX().command_data.start_new_nwk.channel,
      ZLL_TRAN_CTX().src_addr,
      zll_network_start_res_confirm,
      result);

  TRACE_MSG(TRACE_ZLL1, "< zll_network_start_continue", (FMT__0));
}/* void zll_network_start_continue(zb_uint8_t param) */

#endif /* defined ZB_ENABLE_ZLL */
