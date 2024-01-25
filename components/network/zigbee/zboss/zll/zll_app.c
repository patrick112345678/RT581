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
/* PURPOSE: ZLL device start functionality
*/

#define ZB_TRACE_FILE_ID 2111
#include "zb_common.h"
#include "zb_zcl.h"
#include "zll/zb_zll_common.h"
#include "zb_zdo.h"
#include "zll/zll_commissioning_internals.h"
#include "zb_mac.h"
#include "zb_nvram.h"
#include "zll/zb_zll_sas.h"
#include "zb_aps_interpan.h"

#if defined ZB_ENABLE_ZLL
/**
 *  @brief Continues FN ZLL device startup sequence after channel setting.
 *  @param param [in] - reference to a @ref zb_buf_t "buffer" to put further requests to.
 */
void zll_dev_start_continue_fn(zb_uint8_t param);

void zll_init(void)
{
  TRACE_MSG(TRACE_ZLL1, "> zll_init", (FMT__0));

  ZB_MEMSET(&ZLL_CTX(), 0, sizeof(ZLL_CTX()));

  ZB_ZLL_SET_FACTORY_NEW();
#if defined ZB_ZLL_DEVICE_ADDRESS_ASSIGNMENT_CAPABLE
  ZLL_DEVICE_INFO().zll_info |= ZB_ZLL_INFO_ADDR_ASSIGNMENT;
  ZLL_DEVICE_INFO().addr_range.addr_begin = ZB_ZLL_ADDR_BEGIN_INIT_VALUE;
  ZLL_DEVICE_INFO().addr_range.addr_end = ZB_ZLL_ADDR_END_INIT_VALUE;
  ZLL_DEVICE_INFO().group_id_range.group_id_begin = ZB_ZLL_GROUP_ID_BEGIN_INIT_VALUE;
  ZLL_DEVICE_INFO().group_id_range.group_id_end = ZB_ZLL_GROUP_ID_END_INIT_VALUE;
#endif /* defined ZB_ZLL_DEVICE_ADDRESS_ASSIGNMENT_CAPABLE */
  ZLL_DEVICE_INFO().rssi_correction = ZB_ZLL_DEFAULT_RSSI_CORRECTION;
  ZLL_DEVICE_INFO().rssi_threshold  = ZB_ZLL_DEFAULT_RSSI_THRESHOLD;
  ZLL_DEVICE_INFO().report_task_result = zb_zdo_startup_complete;
  /* TODO Fill in key info. */

  ZLL_DEVICE_INFO().identify_duration = ZB_ZLL_IDENTIFY_TIME_DEFAULT_VALUE;

#if defined ZB_ENABLE_ZLL_SAS
  zb_zll_process_sas();
#endif /* defined ZB_ENABLE_ZLL_SAS */

  TRACE_MSG(TRACE_ZLL1, "< zll_init", (FMT__0));
}/* void zll_init(void) */

zb_zll_ctx_t *zb_zll_get_ctx(void)
{
  return &ZG->zll;
}

zb_zll_device_info_t *zb_zll_get_device_info(void)
{
  return &ZG->zll.zll_device_info;
}

zb_zll_transaction_ctx_t *zb_zll_get_transaction_ctx(void)
{
  return &ZG->zll.zll_tran_ctx;
}


#ifndef ZB_BDB_TOUCHLINK
void zll_dev_start_continue_1(zb_uint8_t param);

zb_ret_t zb_zll_dev_start(void)
{
  ZB_AIB().aps_use_nvram = ZB_TRUE;
  ZG->nwk.handle.continue_start_after_nwk_cb = zll_dev_start_continue_1;
  /* Same procedure begin in standard and ZLL: MAC start, sync PIB&NIB  */
  return zdo_dev_start();
}

void zll_dev_start_continue_1(zb_uint8_t param)
{
  zb_uint8_t status;
  zb_bufid_t buffer;

  TRACE_MSG(TRACE_ZLL1, "> zll_dev_start_continue_1 %hd", (FMT__H, param));

  ZLL_TRAN_CTX().transaction_task = ZB_ZLL_DEVICE_START_TASK;
  ZB_ASSERT(param);

  /* AT: save current nwk channel into device context (see: ZLL Frequency agility)
  this will prevent the loss of the configured channel after device discovery
  see: 8.6 Frequency agility
  */
  ZLL_DEVICE_INFO().freqagility_channel = ZB_PIBCACHE_CURRENT_CHANNEL();

  if (ZB_EXTPANID_IS_ZERO(ZB_AIB().aps_use_extended_pan_id /*ZB_NIB_EXT_PAN_ID()*/))
  {
    TRACE_MSG(TRACE_ZLL2, "Starting ZLL FN device.", (FMT__0));
    ZB_PIBCACHE_NETWORK_ADDRESS() = ZB_ZLL_SAS_SHORT_ADDRESS;

    /* AT:
    7.1.2.3.1 Scan response command frame
    The scan response command frame is used to respond to the originator of a scan request
    command frame with device details.
    ....
    PAN ID field shall be set to any value in the range 0x0001 – 0xfffe,
    if the device is factory new, or the PAN identifier of the device, otherwise.
    */
    ZB_PIBCACHE_PAN_ID() = ZLL_SRC_PANID_FIRST_VALUE + ZB_RANDOM_VALUE(ZLL_SRC_PANID_LAST_VALUE - ZLL_SRC_PANID_FIRST_VALUE);
    zb_nwk_pib_set(param, ZB_PIB_ATTRIBUTE_PANID, &ZB_PIBCACHE_PAN_ID(), sizeof(ZB_PIBCACHE_PAN_ID()), zll_dev_start_continue_fn);

    status = RET_OK;
  }
  else
  {
    TRACE_MSG(TRACE_ZLL2, "Starting ZLL NFN device. channel %hd, ext_panid " TRACE_FORMAT_64, (FMT__H_A,
        ZB_PIBCACHE_CURRENT_CHANNEL(), TRACE_ARG_64(ZB_AIB().aps_use_extended_pan_id)));
    ZB_ZLL_CLEAR_FACTORY_NEW();

    ZB_ASSERT(ZB_IS_DEVICE_ZR() || ZB_IS_DEVICE_ZED());

    /* if device is address assignable, modify start/end addr, start/end group id */
    buffer = param;
#if defined ZB_ED_ROLE
    ZLL_REJOIN_NWK(buffer, ZB_AIB().aps_use_extended_pan_id /*ZB_NIB_EXT_PAN_ID()*/, ZB_PIBCACHE_CURRENT_CHANNEL(), status);
#else /* defined ZB_ED_ROLE */
    ZB_SET_JOINED_STATUS(ZB_TRUE);
    ZB_ZLL_START_ROUTER(buffer, ZB_AIB().aps_use_extended_pan_id, ZB_PIBCACHE_PAN_ID(),
                        ZB_PIBCACHE_CURRENT_CHANNEL(), ZB_PIBCACHE_NETWORK_ADDRESS(), status);
#endif /* defined ZB_ED_ROLE */
  }

  TRACE_MSG(TRACE_ZLL1, "< zb_zll_dev_start status %hd", (FMT__H, status));
}/* void zb_zll_dev_start(void) */

#endif  /* ZB_BDB_TOUCHLINK */

void zll_continue_start_channel(zb_uint8_t param);

void zll_continue_start_2(zb_uint8_t param);

void zll_continue_start_rxon(zb_uint8_t param);

void zll_continue_start(zb_uint8_t param)
{
  ZB_PIBCACHE_CURRENT_PAGE() = 0; /* 2.4Ghz */
  zb_nwk_pib_set(param, ZB_PHY_PIB_CURRENT_PAGE, &ZB_PIBCACHE_CURRENT_PAGE(),
                 1, zll_continue_start_channel);
}

void zll_continue_start_channel(zb_uint8_t param)
{
  zb_uint8_t new_channel;

  TRACE_MSG(TRACE_ZLL1, "> zll_continue_start", (FMT__0));
  ZB_ASSERT(param);

  ZB_ASSERT(ZB_IS_DEVICE_ZED() || ZB_IS_DEVICE_ZR());

  new_channel = zll_random_primary_channel();
  /* TODO callback for channel set. */
  TRACE_MSG(TRACE_ZLL1, "set channel %hd", (FMT__H, new_channel));
  ZB_PIBCACHE_CURRENT_PAGE() = 0;
  ZLL_SET_CHANNEL(param, new_channel, zll_continue_start_rxon);

  TRACE_MSG(TRACE_ZLL1, "< zll_continue_start", (FMT__0));
}/* void zll_continue_start(zb_uint8_t param) */


void zll_continue_start_rxon(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZLL1, "> zll_continue_start_rxon", (FMT__0));
  ZB_ASSERT(param);

  zb_nwk_pib_set(param, ZB_PIB_ATTRIBUTE_RX_ON_WHEN_IDLE, &ZB_PIBCACHE_RX_ON_WHEN_IDLE(), 1, zll_continue_start_2);

  TRACE_MSG(TRACE_ZLL1, "< zll_continue_start_rxon", (FMT__0));
}


void zll_continue_start_2(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZLL1, "> zll_continue_start_2", (FMT__0));
  ZB_ASSERT(param);

  ZG->nwk.handle.state = ZB_NLME_STATE_IDLE;
  zll_notify_task_result(param, ZB_ZLL_TASK_STATUS_OK);

  TRACE_MSG(TRACE_ZLL1, "< zll_continue_start_2", (FMT__0));
}

void zll_dev_start_continue_fn(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZLL1, "> zll_dev_start_continue_fn param %hd", (FMT__H, param));

  ZG->nwk.handle.state = ZB_NLME_STATE_ZDO_STARTING;
  /* MP: Starting right now, so we're free to get some buffers. */
  /* 12/08/12 CR:MINOR What does it mean? ^^^^ */
  zdo_call_nlme_reset(param, ZB_FALSE, ZB_FALSE, zll_continue_start);

  TRACE_MSG(TRACE_ZLL1, "< zll_dev_start_continue_fn", (FMT__0));
}/* void zll_dev_start_continue_fn(zb_uint8_t param) */



void zll_parse_hdr(zb_uint8_t param)
{
  zb_zcl_frame_ctrl_t *fc = (zb_zcl_frame_ctrl_t *)zb_buf_begin(param);
  zb_uint8_t *ptr = (zb_uint8_t*)fc + 1;
  zb_intrp_data_ind_t indication;
  zb_zcl_parsed_hdr_t *cmd_param;

  ZB_MEMCPY(&indication, ZB_BUF_GET_PARAM(param, zb_intrp_data_ind_t), sizeof(zb_intrp_data_ind_t));
  cmd_param = (zb_zcl_parsed_hdr_t *)ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);

  ZB_IEEE_ADDR_COPY(cmd_param->addr_data.intrp_data.src_addr, indication.src_addr);
  TRACE_MSG(TRACE_ZLL1, "assign rssi %hd", (FMT__H, indication.rssi));
  cmd_param->rssi = indication.rssi;

  cmd_param->cluster_id = indication.cluster_id;
  cmd_param->profile_id = indication.profile_id;

  cmd_param->cmd_direction = fc->direction;
  cmd_param->is_common_command = (fc->frame_type==ZB_ZCL_FRAME_TYPE_COMMON) ? ZB_TRUE : ZB_FALSE;
  cmd_param->disable_default_response = fc->disable_def_resp;
  cmd_param->is_manuf_specific = fc->manufacturer;

  ZB_ZCL_PACKET_GET_DATA8(&(cmd_param->seq_number), ptr);
  if(cmd_param->is_manuf_specific)
  {
    ZB_ZCL_PACKET_GET_DATA16(&(cmd_param->manuf_specific), ptr);
  }
  else
  {
    cmd_param->manuf_specific = 0;
  }
  ZB_ZCL_PACKET_GET_DATA8(&(cmd_param->cmd_id), ptr);

  zb_buf_cut_left(param,
                  (cmd_param->manuf_specific ? sizeof(zb_zcl_frame_hdr_full_t) : sizeof(zb_zcl_frame_hdr_short_t)));
}

void zll_process_device_command(zb_uint8_t param)
{
  zb_bool_t is_processed = ZB_TRUE;
  zb_zcl_parsed_hdr_t *cmd_param;
  zb_ret_t result;

  zll_parse_hdr(param);

  cmd_param = (zb_zcl_parsed_hdr_t *)ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);

  TRACE_MSG(TRACE_ZLL1, "> zll_process_device_command %hd cmd %hd", (FMT__H_H, param, cmd_param->cmd_id));

  switch (cmd_param->cmd_id) {

    /* REQUEST part */
    case ZB_ZLL_CMD_COMMISSIONING_SCAN_REQ:
      result = zll_handle_scan_req(param);
      break;
    case ZB_ZLL_CMD_COMMISSIONING_DEVICE_INFORMATION_REQ:
      result = zll_handle_devinfo_req(param);
      break;
    case ZB_ZLL_CMD_COMMISSIONING_IDENTIFY_REQ:
      result = zb_zll_identify_handler(param);
      break;
#ifdef ZB_ZLL_ENABLE_COMMISSIONING_SERVER
    case ZB_ZLL_CMD_COMMISSIONING_RESET_TO_FACTORY_NEW_REQ:
      result = zll_reset_to_fn_req_handler(param);
      break;
#endif
    case ZB_ZLL_CMD_COMMISSIONING_NETWORK_START_REQ:
      result = zll_network_start_req_handler(param);
      break;
#ifdef ZB_ZLL_ENABLE_COMMISSIONING_SERVER
#if defined ZB_ROUTER_ROLE
    case ZB_ZLL_CMD_COMMISSIONING_NETWORK_JOIN_ROUTER_REQ:
      result = zll_join_router_req_handler(param);
      break;
#endif
#if defined ZB_ED_ROLE
    case ZB_ZLL_CMD_COMMISSIONING_NETWORK_JOIN_END_DEVICE_REQ:
      result = zll_join_ed_req_handler(param);
      break;
#endif
#endif  /* ZB_ZLL_ENABLE_COMMISSIONING_SERVER */
    case ZB_ZLL_CMD_COMMISSIONING_NETWORK_UPDATE_REQ:
      result = zll_network_update_req_handler(param);
      TRACE_MSG(TRACE_ZLL1, "Inter-PAN network update req", (FMT__0));
      break;

      /* RESPONSE part */
    case ZB_ZLL_CMD_COMMISSIONING_SCAN_RES:
      zll_handle_scan_res(param);
      result = RET_OK;
      break;
    case ZB_ZLL_CMD_COMMISSIONING_DEVICE_INFORMATION_RES:
      zll_handle_devinfo_res(param);
      result = RET_OK;
      break;
    case ZB_ZLL_CMD_COMMISSIONING_NETWORK_START_RES:
      result = zll_network_start_res_handler(param);
      break;
#ifdef ZB_ZLL_ENABLE_COMMISSIONING_CLIENT
    case ZB_ZLL_CMD_COMMISSIONING_NETWORK_JOIN_ROUTER_RES:
      result = zll_join_router_res_handler(param);
      break;
    case ZB_ZLL_CMD_COMMISSIONING_NETWORK_JOIN_END_DEVICE_RES:
      result = zll_join_ed_res_handler(param);
      break;
#endif

      /* RQUEEST/RESPONSE utility part */
    case ZB_ZLL_CMD_COMMISSIONING_ENDPOINT_INFORMATION:
    case ZB_ZLL_CMD_COMMISSIONING_GET_GROUP_IDENTIFIERS_RESPONSE:
    case ZB_ZLL_CMD_COMMISSIONING_GET_ENDPOINT_LIST_RESPONSE:
      is_processed = ZB_FALSE;
      result = RET_NOT_IMPLEMENTED;
      TRACE_MSG(TRACE_ZLL1, "Inter-PAN command %d not implemented", (FMT__H, cmd_param->cmd_id));
      break;

    default:
      is_processed = ZB_FALSE;
      result = RET_ILLEGAL_REQUEST;
      TRACE_MSG(TRACE_ZLL1, "Inter-PAN command %d not implemented", (FMT__H, cmd_param->cmd_id));
      break;
  }

  if (! is_processed)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZLL1, "< zll_process_device_command %d", (FMT__D, result));
}

zb_ret_t zb_zll_send_packet(zb_bufid_t buffer, zb_uint8_t* data_ptr, zb_ieee_addr_t dst_addr_long, zb_callback_t callback)
{
  zb_intrp_data_req_t* _request = ZB_BUF_GET_PARAM((buffer), zb_intrp_data_req_t);
  ZB_ZCL_FINISH_PACKET_O((buffer), (data_ptr));
  _request->dst_addr_mode = ZB_INTRP_ADDR_IEEE;
  ZB_IEEE_ADDR_COPY(_request->dst_addr.addr_long, (dst_addr_long));
  _request->dst_pan_id = ZB_INTRP_BROADCAST_SHORT_ADDR;
  _request->profile_id = ZB_AF_ZLL_PROFILE_ID;
  _request->cluster_id = ZB_ZCL_CLUSTER_ID_TOUCHLINK_COMMISSIONING;
/*CR:MINOR if asdu_handle is not used, remove it from the req, put appropriqte comment */
  /* Sep 28, 2012 CR:Discussed: it could be required by some customers */
  /* using inter-PAN exchange directly. */
  _request->asdu_handle = 0;
  zb_zcl_register_cb(buffer, (callback));
  return zb_schedule_callback(zb_intrp_data_request, buffer);
}

void zb_zdo_touchlink_set_master_key(zb_uint8_t *key)
{
  ZB_MEMCPY(ZLL_DEVICE_INFO().master_key, key, ZB_CCM_KEY_SIZE);
}

void zb_zdo_touchlink_set_nwk_channel(zb_uint8_t channel)
{
  ZLL_DEVICE_INFO().nwk_channel = channel;
}

void zb_zdo_touchlink_set_rssi_threshold(zb_uint8_t rssi_threshold)
{
  ZLL_DEVICE_INFO().rssi_threshold = rssi_threshold;
}

zb_uint8_t zb_zdo_touchlink_get_rssi_threshold(void)
{
  return ZLL_DEVICE_INFO().rssi_threshold;
}

void zb_zdo_touchlink_set_rssi_correction(zb_uint8_t rssi_correction)
{
  ZLL_DEVICE_INFO().rssi_correction = rssi_correction;
}

zb_uint8_t zb_zdo_touchlink_get_rssi_correction(void)
{
  return ZLL_DEVICE_INFO().rssi_correction;
}

#endif /* defined ZB_ENABLE_ZLL */
