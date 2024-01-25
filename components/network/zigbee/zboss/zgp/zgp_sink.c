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
/* PURPOSE: ZGP sink specific functions
*/
#define ZB_TRACE_FILE_ID 2104

#include "zb_common.h"

#ifdef ZB_ENABLE_ZGP_SINK

#include "zcl/zb_zcl_common.h"
#include "zboss_api_zgp.h"
#include "zgp/zgp_internal.h"

#define ZB_ZGP_REPORT_ATTRIBUTE_REPORT_HEADER           3u
#define ZB_ZGP_REPORT_ATTRIBUTE_REPORT_OFFSET           2u
#define ZB_ZGP_MULTI_REPORT_CLUSTER_REPORT_HEADER       5u
#define ZB_ZGP_MULTI_REPORT_TYPE_OFFSET                 4u
#define ZB_ZGP_MULTI_REPORT_DATA_OFFSET                 5u

static void zb_zgps_finalize_commissioning(zb_uint8_t param);
static void zb_zgps_process_commissioning_cmd_cont(zb_uint8_t param);
static void zgps_send_commissioning_reply(zb_uint8_t param);
static void zb_zgp_from_gpdf_to_zcl(zb_uint8_t param);
static void zgp_send_cluster_proxy_commissioning_mode_enter_req_cb(zb_uint8_t param);
static void zgp_send_cluster_proxy_commissioning_mode_enter_req(zb_uint8_t param);
static void zgp_send_cluster_proxy_commissioning_mode_leave_req(zb_uint8_t param);

static void zb_zgps_process_after_select_temp_master(zb_uint8_t param);
static zb_bool_t zb_zgps_collect_gp_commissioning_notifications(zb_uint8_t param);

static zb_ret_t zb_zgp_get_next_point_descr(zb_uint8_t **rpos, zb_uint8_t *max_pos, zgp_data_point_desc_t *point_desc);

void zb_zgps_set_security_level(zb_uint_t level)
{
  ZGP_GPS_SECURITY_LEVEL = level;
}

void zb_zgps_set_commissioning_exit_mode(zb_uint_t cem)
{
  ZGP_GPS_COMMISSIONING_EXIT_MODE = cem;
}

void zb_zgps_set_communication_mode(zgp_communication_mode_t mode)
{
  ZGP_GPS_COMMUNICATION_MODE = mode;
}

void zb_zgps_set_match_info(const zb_zgps_match_info_t *info)
{
  ZGP_CTXC().match_info = info;
}

void zb_zgps_register_comm_req_cb(zb_zgp_comm_req_cb_t cb)
{
  ZGP_CTXC().comm_req_cb = cb;
}

/* obsolete, needs to implement and send new zdo signal */
void zb_zgps_register_app_cic_cb(zb_zgp_app_comm_ind_cb_t cb)
{
  ZGP_CTXC().app_comm_op_cb = cb;
}

void zb_zgps_register_app_cfm_cb(zb_zgp_app_cfm_cb_t cb)
{
  ZGP_CTXC().app_cfm_cb = cb;
}

static void zb_send_zgp_commissioning_signal(zb_uint8_t param)
{
  zb_zgp_signal_commissioning_params_t *comm_params;

  TRACE_MSG(TRACE_NWK1, "zb_send_zgp_commissioning_signal param %hd", (FMT__H, param));
  comm_params = (zb_zgp_signal_commissioning_params_t *)zb_app_signal_pack(param, ZB_ZGP_SIGNAL_COMMISSIONING, RET_OK, sizeof(zb_zgp_signal_commissioning_params_t));
  comm_params->zgpd_id = ZGP_CTXC().comm_data.zgpd_id;
  comm_params->result = (zb_zgp_comm_status_t)ZGP_CTXC().comm_data.result;
  ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete, param);
}

/**
 * @brief Call user commissioning callback and switch
 *        to operational mode
 * @param buf Buffer. Not used now, only freed.
 *            Maybe will be useful in future.
 */
static void notify_user_and_switch_to_oper_mode(zb_bufid_t buf)
{
  zb_uint8_t status = ZGP_CTXC().comm_data.result;

  TRACE_MSG(TRACE_ZGP2, ">> notify_user_and_switch_to_oper_mode", (FMT__0));

  if (ZGP_CTXC().comm_cb)
  {
    /* TODO: obsolete, needs to remove and use zdo signal */
    ZGP_CTXC().comm_cb(&ZGP_CTXC().comm_data.zgpd_id,
        (zb_zgp_comm_status_t) status);
  }
  else
  {
    if (buf)
    {
      /* WARNING: This call is asynchronous - ZGP_CTXC().comm_data.zgpd_id may be changed to the time
     * when signal will be actually processed. Not the best way, but currently do not have better
     * solution... */
      zb_send_zgp_commissioning_signal(buf);
      buf = 0;
    }
  }

  ZB_INIT_ZGPD_ID(&ZGP_CTXC().comm_data.zgpd_id);
  ZGP_CTXC().sink_mode = ZB_ZGP_OPERATIONAL_MODE;

  if (buf != 0)
  {
    zb_buf_free(buf);
  }

  TRACE_MSG(TRACE_ZGP2, "<< notify_user_and_switch_to_oper_mode", (FMT__0));
}

static void zb_zgps_commissioning_timed_out(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZGP2, ">> zb_zgps_commissioning_timed_out, param %hd", (FMT__H, param));

  if (ZGP_CTXC().comm_data.state != ZGP_COMM_STATE_COMMISSIONING_CANCELLED)
  {
    if (param == 0)
    {
      ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_COMMISSIONING_TIMED_OUT);
      zb_buf_get_out_delayed(zb_zgps_commissioning_timed_out);
    }
    else
    {
      *ZB_BUF_GET_PARAM(param, zb_uint8_t) = ZB_ZGP_COMMISSIONING_TIMED_OUT;
      zb_zgps_finalize_commissioning(param);
      param = 0;
    }
  }
  if (param)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_commissioning_timed_out", (FMT__0));
}

static void clear_comm_context()
{
  ZB_BZERO(&ZGP_CTXC().comm_data, sizeof(ZGP_CTXC().comm_data));
  ZB_INIT_ZGPD_ID(&ZGP_CTXC().comm_data.zgpd_id);

  zb_zgps_clear_temp_master_list_ctx();
}

/**
 * @brief Set ZGPS into commissioning mode
 */
void zb_zgps_start_commissioning(zb_time_t timeout)
{
  TRACE_MSG(TRACE_ZGP2, ">> zb_zgps_start_commissioning, timeout %d", (FMT__D, timeout));

  /* A.3.9.1
   * 1b.i) In the current version of the specification, the sink SHALL first check
   * if it needs to contact the Trust Centre, by checking the Involve TC sub-field
   * of the gpsSecurityLevel attribute. If the Involve TC sub-field is set to 0b1,
   * the sink SHALL NOT enter GP commissioning mode.
   */
  if (ZGP_GPS_GET_INVOLVE_TC())
  {
    TRACE_MSG(TRACE_ZGP2,
              "Can not enter commissioning mode due to Involve TC flag is set",
              (FMT__0));
  }
  else
  {
#ifdef ZB_ENABLE_ZGP_PROXY
    if (ZGP_IS_PROXY_IN_COMMISSIONING_MODE())
    {
      TRACE_MSG(TRACE_ZGP3, "Proxy part of combo already in commissioning, skip.", (FMT__0));
    }
    else
#endif  /* ZB_ENABLE_ZGP_PROXY */
    if (ZGP_IS_SINK_IN_OPERATIONAL_MODE())
    {
      ZGP_CTXC().sink_mode = ZB_ZGP_COMMISSIONING_MODE;

      clear_comm_context();

      if (ZB_ZGP_SINK_IS_SEND_ENTER_OR_LEAVE_FOR_PROXIES())
      {
        zb_buf_get_out_delayed(zgp_send_cluster_proxy_commissioning_mode_enter_req);
      }

      if (timeout > 0)
      {
        TRACE_MSG(TRACE_ZGP2,
                  "Setup commissioning timeout: %hd",
                  (FMT__H, (timeout / ZB_TIME_ONE_SECOND)));
        ZB_SCHEDULE_ALARM(zb_zgps_commissioning_timed_out, 0, timeout);
      }
      /* TODO: send signal for the application about begining sink commissioning */
    }
    else
    {
      TRACE_MSG(TRACE_ZGP2, "Sink already in commissioning mode", (FMT__0));
    }
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_start_commissioning", (FMT__0));
}

static void zb_zgps_commissioning_cancel(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZGP2, ">> zb_zgps_commissioning_cancel, param %hd", (FMT__H, param));

  if (ZGP_IS_SINK_IN_COMMISSIONING_MODE())
  {
    if (ZGP_CTXC().comm_data.state != ZGP_COMM_STATE_COMMISSIONING_TIMED_OUT)
    {
      if (param == 0)
      {
        ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_COMMISSIONING_CANCELLED);
        zb_buf_get_out_delayed(zb_zgps_commissioning_cancel);
      }
      else
      {
        *ZB_BUF_GET_PARAM(param, zb_uint8_t) = ZB_ZGP_COMMISSIONING_CANCELLED_BY_USER;
        zb_zgps_finalize_commissioning(param);
        param = 0;
      }
    }
  }
  if (param)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_commissioning_cancel", (FMT__0));
}

void zb_zgps_stop_commissioning()
{
  TRACE_MSG(TRACE_ZGP2, ">> zb_zgps_stop_commissioning", (FMT__0));

  if (ZGP_IS_SINK_IN_COMMISSIONING_MODE())
  {
    TRACE_MSG(TRACE_ZGP2, "commissioning state %hd", (FMT__H, ZGP_CTXC().comm_data.state));
    if ((ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_COMMISSIONING_REPLY_SENT))
    {
      TRACE_MSG(TRACE_ZGP1, "Last comm stage. Give some time for it to complete", (FMT__0));
      ZB_SCHEDULE_ALARM(zb_zgps_commissioning_cancel, 0, ZB_ZGP_TIMEOUT_BEFORE_FORCE_CANCEL);
    }
    else if ((ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_COMMISSIONING_FINALIZING)
          || (ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_COMMISSIONING_TIMED_OUT)
          || (ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_COMMISSIONING_CANCELLED))
    {
      TRACE_MSG(TRACE_ZGP1, "Too late to cancel commissioning, state %hd",
                (FMT__H, ZGP_CTXC().comm_data.state));
    }
    else
    {
      zb_zgps_commissioning_cancel(0);
    }
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_stop_commissioning", (FMT__0));
}

/**
 * @brief Callback after sending channel configuration via ZCL to selected temp master
 *
 * @param param     [in]  Buffer reference
 *
 */
static void zb_zgps_channel_req_send_response_cb(zb_uint8_t param)
{
  zb_zcl_command_send_status_t *cmd_send_status = ZB_BUF_GET_PARAM(param, zb_zcl_command_send_status_t);
  zb_uint16_t                   tmp_addr;

  TRACE_MSG(TRACE_ZGP2, ">> zb_zgps_channel_req_send_response_cb %hd",
            (FMT__H, param));

  if (cmd_send_status->status != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zb_zgps_channel_req_send_response error %hd",
              (FMT__H, cmd_send_status->status));
  }

  if (ZGP_CTXC().sink_mode == ZB_ZGP_COMMISSIONING_MODE)
  {
    ZB_ASSERT(ZGP_CTXC().comm_data.selected_temp_master_idx < ZB_ZGP_MAX_TEMP_MASTER_COUNT);
    tmp_addr = ZGP_CTXC().comm_data.temp_master_list[ZGP_CTXC().comm_data.selected_temp_master_idx].short_addr;
    ZB_ASSERT(tmp_addr != ZB_ZGP_TEMP_MASTER_EMPTY_ENTRY);

#ifndef ZB_ENABLE_ZGP_DIRECT
    ZB_ASSERT(tmp_addr != ZB_PIBCACHE_NETWORK_ADDRESS());
#else
    if (tmp_addr == ZB_PIBCACHE_NETWORK_ADDRESS())
    {
      ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_CHANNEL_CONFIG_SET_TEMP_CHANNEL);
      zgp_channel_config_transceiver_channel_change(param, ZB_FALSE);
    }
    else
#endif  /* ZB_ENABLE_ZGP_DIRECT */
    {
      ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_CHANNEL_CONFIG_SENT);
      zb_buf_free(param);
    }
  }
  else
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_channel_req_send_response_cb", (FMT__0));
}

/**
 * @brief Prepare and send channel configuration via ZCL to selected temp master
 *
 * @param param     [in]  Buffer reference
 *
 */
void zb_zgps_channel_req_send_response(zb_uint8_t param)
{
  zb_zgp_gp_response_t *resp = ZB_BUF_GET_PARAM(param, zb_zgp_gp_response_t);

  TRACE_MSG(TRACE_ZGP2, ">> zb_zgps_channel_req_send_response %hd",
            (FMT__H, param));

  zb_buf_reuse(param);

  ZB_ASSERT(ZGP_CTXC().sink_mode == ZB_ZGP_COMMISSIONING_MODE);
  ZB_ASSERT(ZGP_CTXC().comm_data.selected_temp_master_idx < ZB_ZGP_MAX_TEMP_MASTER_COUNT);
  ZB_ASSERT(ZGP_CTXC().comm_data.temp_master_list[ZGP_CTXC().comm_data.selected_temp_master_idx].short_addr
            != ZB_ZGP_TEMP_MASTER_EMPTY_ENTRY);

  resp->options = ZB_ZGP_FILL_GP_RESPONSE_OPTIONS(ZGP_CTXC().comm_data.zgpd_id.app_id,
                                                  /* TODO: set actual ep match bit flag */
                                                  0);
  resp->temp_master_addr = ZGP_CTXC().comm_data.temp_master_list[ZGP_CTXC().comm_data.selected_temp_master_idx].short_addr;
  resp->zgpd_addr = ZGP_CTXC().comm_data.zgpd_id.addr;
  resp->endpoint = ZGP_CTXC().comm_data.zgpd_id.endpoint;
  resp->temp_master_tx_chnl = ZGP_CTXC().comm_data.temp_master_tx_chnl - ZB_ZGPD_FIRST_CH;
  resp->gpd_cmd_id = ZB_GPDF_CMD_CHANNEL_CONFIGURATION;
  resp->payload[0] = 1;
  resp->payload[1] = ZGP_CTXC().comm_data.oper_channel - ZB_ZGPD_FIRST_CH;
#ifndef ZB_ENABLE_ZGP_ADVANCED
    /* GPD Channel Configuration command, carrying:
         - channel: bits 0-3
         - the Basic sub-field set to 0b1 for a Basic Sink (Combo): bit 4
     */
    resp->payload[1] |= (1<<4);
#endif  /* !ZB_ENABLE_ZGP_ADVANCED */

  TRACE_MSG(TRACE_ZGP3, "buf %hd, cb %p", (FMT__H_P, param, zb_zgps_channel_req_send_response_cb));

  zb_zgp_cluster_gp_response_send(param,
                                  ZB_NWK_BROADCAST_ALL_DEVICES,
                                  ZB_ADDR_16BIT_DEV_OR_BROADCAST,
                                  zb_zgps_channel_req_send_response_cb);
  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_channel_req_send_response", (FMT__0));
}

void zb_gp_sink_mlme_get_cfm_cb(zb_uint8_t param)
{
  zb_mlme_get_confirm_t *cfm = (zb_mlme_get_confirm_t *)zb_buf_begin(param);

  TRACE_MSG(TRACE_ZGP2, ">> zb_gp_sink_mlme_get_cfm_cb %hd", (FMT__H, param));

  ZB_ASSERT(cfm->pib_attr == ZB_PHY_PIB_CURRENT_CHANNEL);

  if (cfm->status == MAC_SUCCESS)
  {
    if (ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_CHANNEL_CONFIG_GET_CUR_CHANNEL)
    {
      ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_CHANNEL_REQ_RECEIVED);
      zb_zgps_channel_req_send_response(param);
      param = 0;
    }
    else
    {
      TRACE_MSG(TRACE_ZGP2, "Hmmm... Unhandled sink commissioning state.", (FMT__0));
      *ZB_BUF_GET_PARAM(param, zb_uint8_t) = ZB_ZGP_COMMISSIONING_CRITICAL_ERROR;
      zb_zgps_finalize_commissioning(param);
      param = 0;
    }
  }
  else
  {
    TRACE_MSG(TRACE_ZGP2, "Sink failed get current channel value", (FMT__0));
    *ZB_BUF_GET_PARAM(param, zb_uint8_t) = ZB_ZGP_COMMISSIONING_CRITICAL_ERROR;
    zb_zgps_finalize_commissioning(param);
    param = 0;
  }

  if (param)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_gp_sink_mlme_get_cfm_cb", (FMT__0));
}

/**
 * @brief Process channel request command received from ZGPD
 *
 * @param [in] param  Reference to buffer with channel request command
 */
static void zb_zgps_process_channel_req(zb_uint8_t param)
{
  zb_gpdf_info_t *gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);

  TRACE_MSG(TRACE_ZGP2, ">> zb_zgps_process_channel_req, param %hd,", (FMT__H, param));

  TRACE_MSG(TRACE_ZGP3, "mode %hd comm state %hd",
            (FMT__H_H, ZGP_CTXC().sink_mode, ZGP_CTXC().comm_data.state));

  if (ZGP_CTXC().sink_mode == ZB_ZGP_COMMISSIONING_MODE)
  {
    /* Update commissioning data */
    ZB_MEMCPY(&ZGP_CTXC().comm_data.zgpd_id, &gpdf_info->zgpd_id, sizeof(zb_zgpd_id_t));

    /* ZGP spec, A.4.2.1.4:
     * The Rx channel in the (second) next attempt sub-field can take the following values:
     * 0b0000: channel 11, 0b0001: channel 12, ..., 0b1111: channel 26
     */
    ZGP_CTXC().comm_data.temp_master_tx_chnl = ZB_ZGPD_FIRST_CH +
      ZB_GPDF_CHANNEL_REQ_NEXT_RX_CHANNEL(*(zb_uint8_t*)zb_buf_begin(param));

    /* TODO: check proxy-based commissioning capability */
    if (/*zb_zgps_check_proxy_based_commissioning_cap() == ZB_TRUE*/1)
    {
      if (ZGP_CTXC().comm_data.selected_temp_master_idx != ZB_ZGP_UNSEL_TEMP_MASTER_IDX)
      {
        /* already select temp master */
        ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_CHANNEL_REQ_COLLECT);
        zb_zgps_process_after_select_temp_master(param);
        param = 0;
      }
      else
        /* collect a couple of GP Commissioning Notification commands (from various GPPs) */
        if (zb_zgps_collect_gp_commissioning_notifications(param) == ZB_TRUE)
        {
          ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_CHANNEL_REQ_COLLECT);
          param = 0;
        }
    }
    else
    {
      ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_CHANNEL_CONFIG_GET_CUR_CHANNEL);
      zb_zgp_channel_config_get_current_channel(param);
      param = 0;
    }
  }

  if (param)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_process_channel_req", (FMT__0));
}

zb_bool_t zb_zgps_get_dev_matching_tbl_index(zb_uint8_t zgpd_dev_id, zb_zgpd_id_t *zgpd_id, zb_uint8_t *idx)
{
  zgps_dev_match_rec_t *match = (zgps_dev_match_rec_t *)ZGP_CTXC().match_info->match_tbl;
  zb_bool_t ret = ZB_FALSE;
  zb_uindex_t i;

  ZVUNUSED(zgpd_id);

  TRACE_MSG(TRACE_ZGP2, ">> zb_zgps_get_dev_matching_tbl_index, zgpd_dev_id %hd", (FMT__H, zgpd_dev_id));

  for (i=0; i < ZGP_CTXC().match_info->match_tbl_size; i++, match++)
  {
    if (IS_STANDART_ZGPS_DEVICE(match) && match->dev_id.zgpd_dev_id == zgpd_dev_id)
    {
      *idx = (zb_uint8_t)i;
      ret = ZB_TRUE;
      break;
      }
    }

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_get_dev_matching_tbl_index, ret %hd i %hd", (FMT__H_H, ret, i));

  return ret;
}

zb_bool_t zb_zgps_get_ms_dev_matching_tbl_index(zb_uint16_t ms_model_id, zb_zgpd_id_t *zgpd_id, zb_uint8_t *idx)
{
  zgps_dev_match_rec_t *match = (zgps_dev_match_rec_t *)ZGP_CTXC().match_info->match_tbl;
  zb_bool_t ret = ZB_FALSE;
  zb_uint8_t i;

  TRACE_MSG(TRACE_ZGP2,">> zb_zgps_get_ms_dev_matching_tbl_index, ms_model_id 0x%hx, zgpd_id %p",
      (FMT__H_P, ms_model_id, zgpd_id));

  for (i=0; i < ZGP_CTXC().match_info->match_tbl_size; i++, match++)
  {
    if (!IS_STANDART_ZGPS_DEVICE(match) && match->dev_id.zgpd_manuf_model == ms_model_id)
    {
      *idx = i;
      ret = ZB_TRUE;
      break;
    }
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_get_ms_dev_matching_tbl_index, ret %hd", (FMT__H, ret));

  return ret;
}

zb_bool_t zb_zgps_get_matching_tbl_device_id(zb_uint8_t idx, zb_uint8_t *device_id)
{
  zb_bool_t ret = ZB_FALSE;

  ZB_ASSERT(device_id);

  TRACE_MSG(TRACE_ZGP2,">> zb_zgps_get_matching_tbl_device_id, tbl_idx %hd device_id %p",
            (FMT__H_P, idx, device_id));

  if (idx != ZGP_INVALID_MATCH_DEV_TABLE_IDX &&
      idx < ZGP_CTXC().match_info->match_tbl_size)
  {
    zgps_dev_match_rec_t *match = (zgps_dev_match_rec_t *)(&ZGP_CTXC().match_info->match_tbl[idx]);

    *device_id = (IS_STANDART_ZGPS_DEVICE(match)) ? (match->dev_id.zgpd_dev_id) : (ZB_ZGP_MANUF_SPECIFIC_DEV_ID);

    ret = ZB_TRUE;

    TRACE_MSG(TRACE_ZGP2, "found device_id %hd", (FMT__H, *device_id));
  }

  TRACE_MSG(TRACE_ZGP2,"<< zb_zgps_get_matching_tbl_device_id, ret %hd",
            (FMT__H, ret));

  return ret;
}

static void zb_zgps_parse_commissioning_app_info(zb_uint8_t *buf_ptr, zb_gpdf_comm_app_info_t *app_info)
{
  TRACE_MSG(TRACE_ZGP2, ">> zb_zgps_parse_commissioning_app_info", (FMT__0));

  app_info->options.manuf_id_present       = *buf_ptr & 0x1;
  app_info->options.manuf_model_id_present = (*buf_ptr >> 1) & 0x1;
  app_info->options.gpd_cmds_present       = (*buf_ptr >> 2) & 0x1;
  app_info->options.cluster_list_present   = (*buf_ptr >> 3) & 0x1;
  app_info->options.switch_info_present    = (*buf_ptr >> 4) & 0x1;
  app_info->options.app_descr_flw          = (*buf_ptr >> 5) & 0x1;
  buf_ptr += 1;

  if (app_info->options.manuf_id_present)
  {
    ZB_LETOH16(&app_info->manuf_id, buf_ptr);
    buf_ptr += 2;
  }

  if (app_info->options.manuf_model_id_present)
  {
    ZB_LETOH16(&app_info->manuf_model_id, buf_ptr);
    buf_ptr += 2;
  }

  if (app_info->options.gpd_cmds_present)
  {
    app_info->gpd_cmds_len = *buf_ptr;
    buf_ptr += 1;

    buf_ptr += app_info->gpd_cmds_len; /* skip, todo: save gpd cmds list */
  }

  if (app_info->options.cluster_list_present)
  {
    app_info->srv_cluster_num = *buf_ptr & 0xF;
    app_info->client_cluster_num = (*buf_ptr >> 4) & 0xF;
    buf_ptr += 1;

    buf_ptr += sizeof(zb_uint16_t) * app_info->srv_cluster_num; /* skip, todo: save server clusterIDs list */
    buf_ptr += sizeof(zb_uint16_t) * app_info->client_cluster_num; /* skip, todo: save client clusterIDs list */
  }

  if (app_info->options.switch_info_present)
  {
    app_info->switch_info.len = *buf_ptr;
    buf_ptr += 1;

    if (app_info->switch_info.len == 2) /* spec v1.1.1, 7168 */
    {
      app_info->switch_info.configuration.num_of_contacs = *buf_ptr & 0xF;
      app_info->switch_info.configuration.switch_type = (*buf_ptr >> 4) & 0x3;
      buf_ptr += 1;

      app_info->switch_info.current_contact_status = *buf_ptr;
      buf_ptr += 1;
    }
    else
    {
      TRACE_MSG(TRACE_ERROR, "zb_zgps_parse_commissioning_app_info : invalid switch info length, skip", (FMT__0));
      app_info->options.switch_info_present = 0;
      buf_ptr += app_info->switch_info.len;
    }
  }
  ZVUNUSED(buf_ptr);

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_parse_commissioning_app_info", (FMT__0));
}

/**
 * @brief Parse commissioning command payload received from ZGPD
 *
 * @param buf_ptr   [in]   Pointer to commissioning command payload
 * @param params    [out]  Pointer to parsed commissioning parameters
 * @param gpdf_info [out]  Pointer to context parameters
 * @param save_ofc  [in]   If ZB_TRUE then OutgoingFrameCounter will save into gpdf_info->sec_frame_counter
 */
static void zb_zgps_parse_commissioning_cmd(zb_uint8_t            *buf_ptr,
                                            zb_gpdf_comm_params_t *params,
                                            zb_gpdf_info_t        *gpdf_info,
                                            zb_bool_t              save_ofc)
{
  TRACE_MSG(TRACE_ZGP2, ">> zb_zgps_parse_commissioning_cmd, buf_ptr %p, params %p",
                        (FMT__H_P, buf_ptr, params));

  ZB_BZERO(params, sizeof(params));

  params->zgpd_device_id = *buf_ptr++;
  params->options = *buf_ptr++;

  TRACE_MSG(TRACE_ZGP2, "Commiss frame options = 0x%x",
                         (FMT__D, params->options));

  if (ZB_GPDF_COMM_EXT_OPT_PRESENT(params->options))
  {
    params->ext_options = *buf_ptr++;
  }
  else
  {
    /* ZGP spec, A.4.2.1.1.3 (about Extended Options field default value):
     *
     * When the Extended Options field is not present in the Commissioning GPDF and the
     * GP Security Key request sub-field of the Options field is set to 0b1, the 0b01
     * is taken as the default value. When the Extended Options field is not present in
     * the Commissioning GPDF and the GP Security Key request sub-field of the Options field
     * is set to 0b0, the 0b00 is taken as the default value.
     */
    params->ext_options = ZB_GPDF_COMM_OPT_SEC_KEY_REQ(params->options);

  }

  if (ZB_GPDF_COMM_OPT_ZGPD_KEY_PRESENT(params->ext_options))
  {
    ZB_MEMCPY(gpdf_info->key, buf_ptr, sizeof(gpdf_info->key));
    buf_ptr += sizeof(gpdf_info->key);
  }

  if (ZB_GPDF_COMM_OPT_ZGPD_KEY_ENCRYPTED(params->ext_options))
  {
    ZB_MEMCPY(gpdf_info->mic, buf_ptr, 4);
    buf_ptr += 4;
  }

  if (ZB_GPDF_COMM_OPT_ZGPD_OUT_COUNTER_PRESENT(params->ext_options))
  {
    if (save_ofc)
    {
      ZB_LETOH32(&gpdf_info->sec_frame_counter, buf_ptr);
    }
    buf_ptr += 4;
  }

  if (ZB_GPDF_COMM_OPT_APP_INF_PRESENT(params->options))
  {
    zb_zgps_parse_commissioning_app_info(buf_ptr, &params->app_info);

    /* save switch info */
    if (params->app_info.options.switch_info_present)
    {
      zgp_runtime_app_tbl_ent_t *ent = zb_zgp_alloc_app_tbl_ent_by_id(&gpdf_info->zgpd_id);
      if (ent)
      {
        if (ent->status == ZGP_APP_TBL_ENT_STATUS_INIT)
        {
          ent->status = ZGP_APP_TBL_ENT_STATUS_INIT_WITH_SW_INFO;
        }

        ent->base.info.options.switch_info_present = 1;
        ent->base.info.switch_info_configuration = params->app_info.switch_info.configuration;

        TRACE_MSG(TRACE_ZGP2, "zb_nvram_write_dataset", (FMT__0));
        ZB_SCHEDULE_CALLBACK(zb_zgp_write_dataset, ZB_NVRAM_DATASET_GP_APP_TBL);
      }
    }
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_parse_commissioning_cmd", (FMT__0));
}

static zb_bool_t zb_zgps_precheck_commissioning_payoad(zb_uint8_t param)
{
  zb_gpdf_comm_params_t *comm_params = (zb_gpdf_comm_params_t *)
                                                zb_buf_get_tail(param,
                                                                sizeof(zb_gpdf_comm_params_t) +
                                                                sizeof(zb_gpdf_info_t));
  zb_gpdf_info_t        *gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);
  zb_bool_t              ret = ZB_TRUE;

  TRACE_MSG(TRACE_ZGP2, ">> zb_zgps_precheck_commissioning_payoad %hd", (FMT__H, param));

  zb_zgps_parse_commissioning_cmd(zb_buf_begin(param), comm_params, gpdf_info, ZB_TRUE);

  if (ZB_GPDF_COMM_OPT_SEC_LEVEL_CAPS(comm_params->ext_options) < ZGP_GPS_GET_SECURITY_LEVEL())
  {
    TRACE_MSG(TRACE_ZGP1, "Drop commissioning frame with too weak sec level", (FMT__0));
    ret = ZB_FALSE;
  }
  else
  {
    if (ZB_GPDF_COMM_OPT_SEC_LEVEL_CAPS(comm_params->ext_options))
    {
      if (!ZB_GPDF_COMM_OPT_ZGPD_KEY_PRESENT(comm_params->ext_options))
      {
        TRACE_MSG(TRACE_ZGP1, "Drop commissioning frame with missing key", (FMT__0));
        ret = ZB_FALSE;
      }
      else
      if (ZB_GPDF_COMM_OPT_ZGPD_KEY_PRESENT(comm_params->ext_options) &&
          !ZB_GPDF_COMM_OPT_ZGPD_KEY_ENCRYPTED(comm_params->ext_options) &&
          ZGP_GPS_GET_PROTECT_WITH_GP_LINK_KEY())
      {
        TRACE_MSG(TRACE_ZGP1, "Drop commissioning frame with unencrypted key", (FMT__0));
        ret = ZB_FALSE;
      }
    }
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_precheck_commissioning_payoad, ret %hd", (FMT__H, ret));
  return ret;
}

static zb_ret_t zb_app_descr_precheck_reports(zb_uint8_t *buf_ptr, zb_uint8_t reports_num, zb_uint8_t buf_len)
{
  zb_ret_t ret = RET_OK;
  zb_uint16_t cur_len = 0;
  zb_uint16_t report_len;
  zb_uint8_t i;

  TRACE_MSG(TRACE_ZGP2, ">> zb_app_descr_precheck_reports", (FMT__0));

  for (i = 0; i < reports_num; i++)
  {
    report_len = 0;

    /* skip Report Identif */
    report_len += 1;

    /* check options */
    if (cur_len + report_len + 1 > buf_len)
    {
      ret = RET_ERROR;
      break;
    }

    if (buf_ptr[report_len] & 0x1) /* Timeout period present */
    {
      /* skip Timeout period */
      report_len += 2;
    }

    report_len += 1;

    /* check remaining length */
    if (cur_len + report_len + 1 > buf_len)
    {
      ret = RET_ERROR;
      break;
    }

    report_len += buf_ptr[report_len] + 1;

    /* check and move pointer */
    cur_len += report_len;
    if (cur_len > buf_len)
    {
      ret = RET_ERROR;
      break;
    }

    buf_ptr += report_len;
  }

  if (cur_len != buf_len)
  {
    ret = RET_ERROR;
    TRACE_MSG(TRACE_ERROR, "cur_len %hd, report_len %hd, buf_len %hd", (FMT__H_H_H, cur_len, report_len, buf_len));
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_app_descr_precheck_reports, ret %hd", (FMT__H, ret));

  return ret;
}

static void zb_app_descr_parse_reports(zgp_runtime_app_tbl_ent_t *ent, zb_uint8_t *buf_ptr, zb_uint8_t reports_num)
{
  zb_uint8_t i;
  zb_uint8_t idx = ent->receive_reports_num;

  for (i = 0; i < reports_num; i++)
  {
    /* skip report ident */
    /* todo: it's right? */
    buf_ptr += 1;

    /* fill options */
    ent->base.reports[idx].options.timeout_present = *buf_ptr & 0x1;
    buf_ptr += 1;

    if (ent->base.reports[idx].options.timeout_present)
    {
      ZB_LETOH16(&ent->base.reports[idx].timeout, buf_ptr);
      buf_ptr += 2;
    }

    ent->base.reports[idx].point_descs_data_len = *buf_ptr;
    buf_ptr += 1;

    ZB_MEMCPY(ent->base.reports[idx].point_descs_data, buf_ptr, ent->base.reports[idx].point_descs_data_len);
    buf_ptr += ent->base.reports[idx].point_descs_data_len;

    idx += 1;
  }
}

static zb_ret_t zb_zgp_app_descr_parse(zgp_runtime_app_tbl_ent_t *ent, zb_uint8_t *buf_ptr, zb_uint8_t buf_len)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t total_reports;
  zb_uint8_t reports_num;;

  ZB_ASSERT(ent);

  TRACE_MSG(TRACE_ZGP2, ">> zb_zgp_app_descr_parse", (FMT__0));

  /* check status */
  if (ent->status != ZGP_APP_TBL_ENT_STATUS_INIT &&
      ent->status != ZGP_APP_TBL_ENT_STATUS_INIT_WITH_SW_INFO &&
      ent->status != ZGP_APP_TBL_ENT_STATUS_APP_DESCR_PROCESS)
  {
    TRACE_MSG(TRACE_ERROR, "invalid status %hd", (FMT__H, ent->status));
    ret = RET_ERROR;
  }

  if (ret == RET_OK)
  {
    /* check length and reports counters */
    if (buf_len > 2)
    {
      total_reports = *buf_ptr;
      buf_ptr += 1;
      reports_num = *buf_ptr;
      buf_ptr += 1;

      if (
          !total_reports ||
          !reports_num   ||
          (reports_num + ent->receive_reports_num > total_reports) ||
          ((ent->status == ZGP_APP_TBL_ENT_STATUS_APP_DESCR_PROCESS) && (ent->base.info.total_reports_num != total_reports))
         )
      {
        TRACE_MSG(TRACE_ERROR, "check reports counters failed, total_reports %hd, reports_num %hd, recieve_reports %hd",
            (FMT__H_H_H, total_reports, reports_num, ent->receive_reports_num));
        ret = RET_ERROR;
      }
    }
    else
    {
      TRACE_MSG(TRACE_ERROR, "invalid buf_len", (FMT__0));
      ret = RET_ERROR;
    }
  }

  if (ret == RET_OK)
  {
    ret = zb_app_descr_precheck_reports(buf_ptr, reports_num, buf_len - 2);
  }

  if (ret == RET_OK)
  {
    zb_app_descr_parse_reports(ent, buf_ptr, reports_num);

    /* update status */
    if ((ent->status == ZGP_APP_TBL_ENT_STATUS_INIT) ||
        (ent->status == ZGP_APP_TBL_ENT_STATUS_INIT_WITH_SW_INFO))
    {
      ent->base.info.total_reports_num = total_reports;
      ent->receive_reports_num = reports_num;
    }
    else if (ent->status == ZGP_APP_TBL_ENT_STATUS_APP_DESCR_PROCESS)
    {
      ent->receive_reports_num += reports_num;
    }
    else
    {
      /* never, we check status above */
      ZB_ASSERT(0);
    }

    ent->status = ent->base.info.total_reports_num == ent->receive_reports_num ?
        ZGP_APP_TBL_ENT_STATUS_COMPLETE
        : ZGP_APP_TBL_ENT_STATUS_APP_DESCR_PROCESS;

    TRACE_MSG(TRACE_ZGP2, "app_tbl_ent: status update %hd", (FMT__H, ent->status));

    if (ent->status == ZGP_APP_TBL_ENT_STATUS_COMPLETE)
    {
      TRACE_MSG(TRACE_ZGP2, "write to nvram", (FMT__0));
      ZB_SCHEDULE_CALLBACK(zb_zgp_write_dataset, ZB_NVRAM_DATASET_GP_APP_TBL);
    }
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgp_app_descr_parse, ret %hd", (FMT__H, ret));

  return ret;
}

static void zb_zgps_commis_cancel(zb_uint8_t param)
{
  *ZB_BUF_GET_PARAM(param, zb_uint8_t) = ZB_ZGP_COMMISSIONING_TIMED_OUT;
  zb_zgps_finalize_commissioning(param);
}

static void zb_zgps_app_descr_timeout(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ERROR, "zb_zgps_app_descr_timeout", (FMT__0));
  if (param)
  {
    zb_zgps_commis_cancel(param);
  }
  else
  {
    zb_buf_get_out_delayed(zb_zgps_commis_cancel);
  }
}

static void zb_zgp_app_descr_send_attr_report_from_point_desc(zb_uint8_t param, zgp_data_point_desc_t *point_desc)
{
  zb_uint8_t i;
  zb_uint8_t *p;

  TRACE_MSG(TRACE_ZGP3, ">> zb_zgp_app_descr_send_attr_report_from_point_desc", (FMT__0));

  /* Fill buffer */
  p = zb_buf_initial_alloc(param, 2); /* for ClID */

  /* Place manuf_id, if present */
  if (point_desc->options.manuf_id_present)
  {
    ZB_HTOLE16(p, &point_desc->manuf_id);
    p = zb_buf_alloc_right(param, 2);
  }

  /* Place cluster ID */
  ZB_HTOLE16(p, &point_desc->cluster_id);

  /* Place attribute id, data_type and value */
  for (i = 0; i < point_desc->options.attr_records_num + 1; i++)
  {
    if (ZGP_ATTR_OPT_GET_VAL_PRESENT(point_desc->attr_records_data[i].options))
    {
      zb_uint8_t value_len = zb_zcl_get_attribute_size(point_desc->attr_records_data[i].data_type, NULL);

      TRACE_MSG(TRACE_ZGP3, "attribute value present, len = %hd", (FMT__H, value_len));

      if (value_len != 0xff)
      {
        p = zb_buf_alloc_right(param, 3 + value_len); /* 3 = AttrID + AttrDataType */

        if (p)
        {
          ZB_HTOLE16(p, &point_desc->attr_records_data[i].id);
          p += 2;

          ZB_MEMCPY(p, &point_desc->attr_records_data[i].data_type, 1);
          p += 1;

          ZB_MEMCPY(p, point_desc->attr_records_data[i].value, value_len);
        }
      }
    }
  }

  /* If we have one or more attr records - call zb_zgp_from_gpdf_to_zcl*/
  if (zb_buf_len(param) > 5) /* 5 = ClID(2) + AttrID(2) + AttrDType(1)*/
  {
    zb_gpdf_info_t *gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);

    /* set command */
    gpdf_info->zgpd_cmd_id = point_desc->options.manuf_id_present ?
        ZB_GPDF_CMD_MANUF_SPEC_ATTR_REPORT
        : ZB_GPDF_CMD_ATTR_REPORT;

    TRACE_MSG(TRACE_ZGP3, "schedule zb_zgp_from_gpdf_to_zcl", (FMT__0));
    ZB_SCHEDULE_CALLBACK(zb_zgp_from_gpdf_to_zcl, param);
  }
  else /* skip this data point */
  {
    TRACE_MSG(TRACE_ZGP3, "skip this data point", (FMT__0));
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZGP3, "<< zb_zgp_app_descr_send_attr_report_from_point_desc", (FMT__0));
}

static void zb_zgp_app_descr_send_attr_report(zb_uint8_t param)
{
  zb_gpdf_info_t        *gpdf_info = &ZGP_CTXC().app_descr_ctx.gpdf_info;
  zgp_report_desc_t     *report_desc;
  zgp_data_point_desc_t point_desc;
  zb_uint8_t            *rpos;
  zb_uint8_t            *maxpos;

  TRACE_MSG(TRACE_ZGP3, ">> zb_zgp_app_descr_send_attr_report", (FMT__0));

  while (param)
  {
    /* Get report description */
    report_desc = zb_zgp_get_report_desc_from_app_tbl(&gpdf_info->zgpd_id, ZGP_CTXC().app_descr_ctx.report_idx);

    /* Get point description */
    rpos   = report_desc->point_descs_data + ZGP_CTXC().app_descr_ctx.point_desc_offset;
    maxpos = &report_desc->point_descs_data[report_desc->point_descs_data_len];

    if (zb_zgp_get_next_point_descr(&rpos, maxpos, &point_desc) == RET_OK)
    {
      zb_gpdf_info_t *gpdf_info_param = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);
      ZB_MEMCPY(gpdf_info_param, gpdf_info, sizeof(zb_gpdf_info_t));
      zb_zgp_app_descr_send_attr_report_from_point_desc(param, &point_desc);

      ZGP_CTXC().app_descr_ctx.point_desc_offset = rpos - report_desc->point_descs_data;
      zb_buf_get_out_delayed(zb_zgp_app_descr_send_attr_report);

      param = 0;
    }
    else
    {
      zgp_runtime_app_tbl_ent_t *ent = zb_zgp_get_app_tbl_ent_by_id(&gpdf_info->zgpd_id);

      ZGP_CTXC().app_descr_ctx.report_idx += 1;
      if (ZGP_CTXC().app_descr_ctx.report_idx < ent->base.info.total_reports_num)
      {
        ZGP_CTXC().app_descr_ctx.point_desc_offset = 0;
      }
      else
      {
        zb_buf_free(param);
        param = 0;
      }
    }
  }

  TRACE_MSG(TRACE_ZGP3, "<< zb_zgp_app_descr_send_attr_report", (FMT__0));
}

static void zb_zgps_process_app_descr_cmd(zb_uint8_t param)
{
  zb_gpdf_info_t         *gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);
  zgp_runtime_app_tbl_ent_t *app_descr_ent = zb_zgp_alloc_app_tbl_ent_by_id(&gpdf_info->zgpd_id);

  TRACE_MSG(TRACE_ZGP2, ">> zb_zgps_process_app_descr_cmd", (FMT__0));

  if (app_descr_ent)
  {
    if (app_descr_ent->status != ZGP_APP_TBL_ENT_STATUS_COMPLETE) /* else drop */
    {
       ZB_SCHEDULE_ALARM_CANCEL(zb_zgps_app_descr_timeout, app_descr_ent->reply_buf);

       if (zb_zgp_app_descr_parse(app_descr_ent, zb_buf_begin(param), zb_buf_len(param)) == RET_OK)
       {
         if (ZB_GPDF_EXT_NFC_GET_RX_AFTER_TX(gpdf_info->nwk_ext_frame_ctl))
         {
           app_descr_ent->need_reply = ZB_TRUE;
         }

         if (app_descr_ent->status == ZGP_APP_TBL_ENT_STATUS_COMPLETE)
         {
           TRACE_MSG(TRACE_ZGP2, "Application description full received", (FMT__0));

           /* save ctx for handle default attribute values */
           ZB_MEMCPY(&ZGP_CTXC().app_descr_ctx.gpdf_info, gpdf_info, sizeof(zb_gpdf_info_t));
           ZGP_CTXC().app_descr_ctx.report_idx = 0;
           ZGP_CTXC().app_descr_ctx.point_desc_offset = 0;

           ZB_ASSERT(app_descr_ent->reply_buf);

           if (app_descr_ent->need_reply)
           {
             TRACE_MSG(TRACE_ZGP2, "zgps_send_commissioning_reply", (FMT__0));
             zgps_send_commissioning_reply(app_descr_ent->reply_buf);

             #ifdef ZB_ENABLE_ZGP_DIRECT
             if (gpdf_info->rx_directly)
             {
               ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_COMMISSIONING_REPLY_ADDED_TO_Q);
             }
             else
             #endif  /* ZB_ENABLE_ZGP_DIRECT */
             {
               ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_COMMISSIONING_REPLY_SENT);
             }

             app_descr_ent->reply_buf = 0;
           }
           else
           {
             zb_buf_free(app_descr_ent->reply_buf);
             app_descr_ent->reply_buf = 0;

             TRACE_MSG(TRACE_ZGP1, "Unidirectional Commissioning is successful", (FMT__0));
             *ZB_BUF_GET_PARAM(param, zb_uint8_t) = ZB_ZGP_COMMISSIONING_COMPLETED;
             zb_zgps_finalize_commissioning(param);
           }
         }
         else
         {
           ZB_SCHEDULE_ALARM(zb_zgps_app_descr_timeout, app_descr_ent->reply_buf, ZB_APP_DESCR_TIMEOUT);
         }
       }
       else
       {
         TRACE_MSG(TRACE_ERROR, "Application Description process fail, drop frame", (FMT__0));
         ZB_SCHEDULE_ALARM(zb_zgps_app_descr_timeout, app_descr_ent->reply_buf, ZB_APP_DESCR_TIMEOUT);
       }
    }
    else
    {
      TRACE_MSG(TRACE_ERROR, "Application Description already filled, drop frame", (FMT__0));
    }
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Application table is full, drop frame", (FMT__0));
  }

  zb_buf_free(param);

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_process_app_descr_cmd", (FMT__0));
}

static void zgp_aprove_commissioning(zb_uint8_t param)
{
  if (ZGP_CTXC().comm_req_cb != NULL)
  {
    zb_gpdf_info_t        *gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);
    zb_gpdf_comm_params_t *comm_params = (zb_gpdf_comm_params_t *)zb_buf_get_tail(param,
                                                                                  sizeof(zb_gpdf_comm_params_t) + sizeof(zb_gpdf_info_t));
    zb_ieee_addr_t         ieee_addr;

    ZGP_CTXC().comm_data.comm_req_buf = param;
    ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_COMM_REQ_RECEIVED_WAIT_FOR_APP);

    /* If comm frame has broadcast dst addr and extended source address */
    if (gpdf_info->mac_addr_flds_len == 12)
    {
      ZB_LETOH64(ieee_addr, gpdf_info->mac_addr_flds.comb.src_addr);
    }
    else if (gpdf_info->zgpd_id.app_id == ZB_ZGP_APP_ID_0010)
    {
      ZB_LETOH64(ieee_addr, gpdf_info->zgpd_id.addr.ieee_addr);
    }
    else
    {
      ZB_64BIT_ADDR_ZERO(ieee_addr);
    }
    ZGP_CTXC().comm_req_cb(&gpdf_info->zgpd_id,
                          comm_params->zgpd_device_id,
                          comm_params->app_info.manuf_id,
                          comm_params->app_info.manuf_model_id,
                          ieee_addr);
  }
  else
  {
    ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_COMM_REQ_RECEIVED_AND_APPROVED);
    ZB_SCHEDULE_CALLBACK(zb_zgps_process_commissioning_cmd_cont, param);
  }
}

/**
 * @brief Process commissioning command received from ZGPD
 *
 * Function parses commissioning command, then tries to find
 * functionality matching.
 *
 * If some functionality matching is found:
 *   - translation table is updated accordingly to these matches
 *   - new entry is created in sink table for ZGPD
 *   - if ZGPD requested unidirectional commissioning, then it
 *     is considered successful and user is notified about it and
 *     ZGPS is placed into operational mode
 *
 * @param [in] param  Reference to buffer with commissioning command.
 */
static void zb_zgps_process_commissioning_cmd(zb_uint8_t param)
{
  zb_gpdf_comm_params_t *comm_params = (zb_gpdf_comm_params_t *)
                                                zb_buf_get_tail(param,
                                                                sizeof(zb_gpdf_comm_params_t) +
                                                                sizeof(zb_gpdf_info_t));
  zb_gpdf_info_t        *gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);
  zb_bool_t              key_auth_success = ZB_TRUE;

  TRACE_MSG(TRACE_ZGP2, ">> zb_zgps_process_commissioning_cmd, param %hd state %hd",
            (FMT__H_H, param, ZGP_CTXC().comm_data.state));

  ZB_ASSERT(ZGP_CTXC().sink_mode == ZB_ZGP_COMMISSIONING_MODE);

#ifdef ZB_ENABLE_ZGP_DIRECT
  ZB_ASSERT(ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_IDLE ||
            ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_CHANNEL_CONFIG_SENT ||
            ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_COMMISSIONING_REPLY_ADDED_TO_Q ||
            ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_CHANNEL_REQ_RECEIVED ||
            ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_COMISSIONING_REQ_COLLECT);
#else
  ZB_ASSERT(ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_IDLE ||
            ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_CHANNEL_CONFIG_SENT ||
            ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_CHANNEL_REQ_RECEIVED ||
            ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_COMISSIONING_REQ_COLLECT);
#endif  /* ZB_ENABLE_ZGP_DIRECT */

  if (
#ifdef ZB_ENABLE_ZGP_DIRECT
      ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_COMMISSIONING_REPLY_ADDED_TO_Q ||
#endif  /* ZB_ENABLE_ZGP_DIRECT */
      ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_CHANNEL_REQ_RECEIVED)
  {
    /* We added commissioning reply after this commissioning cmd is received from radio.
     * Drop this packet, we will reply to next commissioning cmd */
    zb_buf_free(param);
    return;
  }

  if (ZB_GPDF_COMM_OPT_ZGPD_KEY_PRESENT(comm_params->ext_options) &&
      ZB_GPDF_COMM_OPT_ZGPD_KEY_ENCRYPTED(comm_params->ext_options))
  {
    zb_uint8_t plain_key[ZB_CCM_KEY_SIZE];
		#if 0
    zb_uint8_t tclk_key[] = ZB_STANDARD_TC_KEY;
		#else
		/* let Zigbee TC key could be changed after pack in LIB */
    zb_uint8_t tclk_key[]
    ZB_MEMCPY(&(tclk_key), ZB_STANDARD_TC_KEY, ZB_CCM_KEY_SIZE);	
		#endif
		
    /* Use GP link key to proteck key-exchange during commissioning. */
    ZB_MEMCPY (tclk_key, ZGP_GP_LINK_KEY, ZB_CCM_KEY_SIZE );

    key_auth_success = (zb_bool_t)(zb_zgp_decrypt_n_auth_gpd_key(
                                     ZB_TRUE,  /* Is frame from GPD */
                                     &gpdf_info->zgpd_id,
                                     tclk_key,
                                     gpdf_info->key,
                                     gpdf_info->sec_frame_counter,
                                     gpdf_info->mic,
                                     plain_key) == RET_OK);

    TRACE_MSG(TRACE_ZGP3, "decrypted GPD key: " TRACE_FORMAT_128, (FMT__A_A, TRACE_ARG_128(plain_key)));

    ZB_MEMCPY(gpdf_info->key, plain_key, ZB_CCM_KEY_SIZE);
  }

  if (key_auth_success)
  {
    zgp_aprove_commissioning(param);
  }
  else
  {
    TRACE_MSG(TRACE_ZGP2, "key auth failed, drop frame", (FMT__0));
    zb_buf_free(param);
    ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_IDLE);
    clear_comm_context();
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_process_commissioning_cmd", (FMT__0));
}

void zb_zgps_accept_commissioning(zb_bool_t accept)
{
  TRACE_MSG(TRACE_ZGP2, ">> zb_zgps_accept_commissioning, accept %hd", (FMT__H, accept));

  if (ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_COMM_REQ_RECEIVED_WAIT_FOR_APP)
  {
    TRACE_MSG(TRACE_ZGP3, "comm_req_buf %hd", (FMT__H, ZGP_CTXC().comm_data.comm_req_buf));
    ZB_ASSERT(ZGP_CTXC().comm_data.comm_req_buf);

    if (accept)
    {
      ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_COMM_REQ_RECEIVED_AND_APPROVED);
      ZB_SCHEDULE_CALLBACK(zb_zgps_process_commissioning_cmd_cont,
                           ZGP_CTXC().comm_data.comm_req_buf);
    }
    else
    {
      zgp_sink_table_del(&ZGP_CTXC().comm_data.zgpd_id);
      zb_buf_free(ZGP_CTXC().comm_data.comm_req_buf);
      ZGP_CTXC().comm_data.comm_req_buf = 0;
      TRACE_MSG(TRACE_ZGP3, "free comm_req_buf", (FMT__0));
      ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_IDLE);
      clear_comm_context();
    }
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_accept_commissioning", (FMT__0));
}

static void zb_zgps_process_commissioning_cmd_cont(zb_uint8_t param)
{
  zb_gpdf_comm_params_t *comm_params = (zb_gpdf_comm_params_t *)zb_buf_get_tail(param,
                                                                                sizeof(zb_gpdf_comm_params_t) + sizeof(zb_gpdf_info_t));
  zb_gpdf_info_t        *gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);
  zb_uint8_t             zgpd_sec_level = ZB_GPDF_COMM_OPT_SEC_LEVEL_CAPS(comm_params->ext_options);
  zb_uint8_t             zgpd_sec_key_type = ZB_GPDF_COMM_OPT_SEC_KEY_TYPE(comm_params->ext_options);
  zb_bool_t              match_found;
  zb_uint8_t             match_table_idx;

  TRACE_MSG(TRACE_ZGP2, ">> zb_zgps_process_commissioning_cmd_cont, param %hd", (FMT__H, param));

  if (!ZB_ZGPD_IS_SPECIFIED(&ZGP_CTXC().comm_data.zgpd_id))
  {
    ZB_MEMCPY(&ZGP_CTXC().comm_data.zgpd_id, &gpdf_info->zgpd_id, sizeof(zb_zgpd_id_t));
  }

  if (ZGP_CTXC().match_info == NULL)
  {
    TRACE_MSG(TRACE_ERROR, "Error: match table does not exist", (FMT__0));
    match_found = ZB_FALSE;
  }
  else
  {
    if (comm_params->zgpd_device_id != ZB_ZGP_MANUF_SPECIFIC_DEV_ID)
    {
      match_found = zb_zgps_get_dev_matching_tbl_index(comm_params->zgpd_device_id,
                                                       &gpdf_info->zgpd_id,
                                                       &match_table_idx);
    }
    else
    {
      match_found = zb_zgps_get_ms_dev_matching_tbl_index(comm_params->app_info.manuf_model_id,
                                                          &gpdf_info->zgpd_id,
                                                          &match_table_idx);
    }
  }

  TRACE_MSG(TRACE_ZGP3, "Functional match found %hd", (FMT__H, match_found));

  /* A.3.9 GP commissioning */
  /* A.3.9.1 The procedure */
  /*  The  sink  checks  if  the  minimum  security  level  supported  by  the  GPD,  as  indicated  by  the  6000
      SecurityLevel  capabilities  sub-field  and  the  GPD  Key  encryption  sub-field  of  the  Extended    6001
      Options  field of the received  Commissioning GPDF.  The SecurityLevel  capabilities  sub-field of      6002
      the  received  GPD  Commissioning  command  SHALL  be  equal  to  or  larger  than  the  Minimal        6003
      GPD Security Level sub-field of the gpsSecurityLevel (see sec. A.3.3.2.6).   If the Protection with     6004
      gpLinkKey  sub-field  of the  gpsSecurityLevel  is set to 0b1, then the  GPD Key  encryption  sub-      6005
      field of the Extended Options field of the received Commissioning GPDF SHALL be set as well.            6006
      According  to  the  current  version  of  the  specification,  the  sink  SHALL  NOT  accept  GPDs      6007
      supporting gpdSecurityLevel = 0b00 or GPDs not supporting TC-LK protection, unless explicitly           6008
      configured to do so, using gpsSecurityLevel.*/
  if (zgpd_sec_level < ZGP_GPS_GET_SECURITY_LEVEL()
/* if only certified devices are supported, enable next line */
/*     ||zgpd_sec_level<ZB_ZGP_SEC_LEVEL_FULL_NO_ENC*/
    ||((ZGP_GPS_GET_PROTECT_WITH_GP_LINK_KEY()==1)&&(ZB_GPDF_COMM_OPT_ZGPD_KEY_ENCRYPTED(comm_params->ext_options)==0))
    )
  {
    TRACE_MSG(TRACE_ERROR, "Mismatch SECURITY_1! zgpd_sl?%d:%d:gps_sl, zgpd_gpkey:%d?%d:gps_gpkey drop frame",
      (FMT__H_H_H_H, zgpd_sec_level, ZGP_GPS_GET_SECURITY_LEVEL(), ZB_GPDF_COMM_OPT_ZGPD_KEY_ENCRYPTED(comm_params->ext_options),
      ZGP_GPS_GET_PROTECT_WITH_GP_LINK_KEY()));
    match_found = ZB_FALSE;
  }

  /* Copy options to context */
  ZGP_CTXC().comm_data.gpdf_ext_options       = comm_params->ext_options;
  ZGP_CTXC().comm_data.gpdf_options           = comm_params->options;
  ZGP_CTXC().comm_data.gpdf_security_frame_counter = gpdf_info->sec_frame_counter;

  TRACE_MSG(TRACE_ZGP3, "nwk_opt = 0x%x", (FMT__H, gpdf_info->nwk_frame_ctl));

  if (match_found && (ZB_GPDF_NFC_GET_AUTO_COMMISSIONING(gpdf_info->nwk_frame_ctl) == 0))
  {
    /* A.3.9.1 The procedure */
    /* If  GPD  application  functionality matches,  the  sink  SHALL  check the contents of the security-        6014
       related  fields  of  the  Commissioning  GPDF  payload  (see  sec.  A.1.5.3).  I.a.,  the  sink  SHALL     6015
       check the following: if the  gpdSecurityLevel  has value other than 0b00 AND the sink does not             6016
       have  a  key  for  this  GPD  yet  AND  EITHER  RxAfterTx  is  NOT  set  and  the  GPD  Key  is  not       6017
       included  in  the  Commissioning  GPDF  OR  RxAfterTx  is  set  and  neither  the  GPD  Key  field  is     6018
       present  nor  the  GPSecurityKeyRequest  sub-field  is  set,  then  the  sink  shall  silently  drop  the  6019
       frame. */
    if (zgpd_sec_level > ZB_ZGP_SEC_LEVEL_NO_SECURITY)
    {
      if(
        (
          (ZB_GPDF_EXT_NFC_GET_RX_AFTER_TX(gpdf_info->nwk_ext_frame_ctl)==0) && (ZB_GPDF_COMM_OPT_ZGPD_KEY_PRESENT(comm_params->ext_options)==0)
          )
        ||
        (
          (ZB_GPDF_EXT_NFC_GET_RX_AFTER_TX(gpdf_info->nwk_ext_frame_ctl)==1) &&
          ((ZB_GPDF_COMM_OPT_ZGPD_KEY_PRESENT(comm_params->ext_options)==0) && (ZB_GPDF_COMM_OPT_SEC_KEY_REQ(comm_params->options)==0))
          )
        )
      {
        TRACE_MSG(TRACE_ERROR, "Mismatch SECURITY_2! RX_AFTER_TX=%d::%d=ZGPD_KEY_PRESENT, SEC_KEY_REQ=%d, drop frame", (FMT__H_H_H,
                                                                                                                        ZB_GPDF_EXT_NFC_GET_RX_AFTER_TX(gpdf_info->nwk_ext_frame_ctl), ZB_GPDF_COMM_OPT_ZGPD_KEY_PRESENT(comm_params->ext_options),ZB_GPDF_COMM_OPT_SEC_KEY_REQ(comm_params->options)));
        match_found = ZB_FALSE;
      }
    }
  }

  if (match_found)
  {
    zb_uint8_t            zgpd_seq_num_caps = ZB_GPDF_COMM_OPT_SEQ_NUM_CAPS(comm_params->options);
    zb_zgp_sink_tbl_ent_t ent;
    zb_uint8_t            cm = ZGP_COMMUNICATION_MODE_LIGHTWEIGHT_UNICAST;
    zb_uint16_t           apsme_group = 0;

    cm = ZGP_GPS_COMMUNICATION_MODE;

    if (zgp_sink_table_read(&gpdf_info->zgpd_id, &ent) != RET_OK)
    {
      /* To assure that usage of the alias does not cause any disturbance to Zigbee network
      operation, the sink SHALL send the Zigbee Device_annce command [1], after adding an active
      entry for a new GPD into its Sink Table as a result of proximity or multi-hop
      commissioning. (see ZGP spec, A.3.6.3.4.1) */
      /* If the sink sends the GP Pairing command with AddSink sub-field set to 0b1, it SHALL also
      send De-vice_annce for the corresponding alias ( with the exception of lightweight unicast
      communication mode). (see ZGP spec, A.3.5.2.5) */
      if (ZGP_GPS_COMMUNICATION_MODE == ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED ||
          ZGP_GPS_COMMUNICATION_MODE == ZGP_COMMUNICATION_MODE_GROUPCAST_PRECOMMISSIONED)
      {
        TRACE_MSG(TRACE_ZGP2, "need send dev annce for alias", (FMT__0));
        ZGP_CTXC().comm_data.need_send_dev_annce = 1;
      }
    }

    ZB_BZERO(&ent, sizeof(zb_zgp_sink_tbl_ent_t));

    ent.options = ZGP_TBL_SINK_FILL_OPTIONS(
      gpdf_info->zgpd_id.app_id,
      cm,
      zgpd_seq_num_caps,
      ZB_GPDF_COMM_OPT_RX_CAPABILITY(comm_params->options),
      ZB_GPDF_COMM_OPT_FIX_LOC(comm_params->options),
      0,                /* Assigned alias */
      (zgpd_sec_level > ZB_ZGP_SEC_LEVEL_NO_SECURITY));

      ent.sec_options = ZGP_TBL_FILL_SEC_OPTIONS(zgpd_sec_level, zgpd_sec_key_type);

      TRACE_MSG(TRACE_ZGP3, "sec_level %hd sec_key_type %hd options 0x%x sec_options 0x%hx",
                (FMT__H_H_D_H, zgpd_sec_level, zgpd_sec_key_type, ent.options, ent.sec_options));

      ZB_MEMSET(&ent.u.sink.sgrp, (-1), sizeof(ent.u.sink.sgrp));

      if (ZGP_TBL_SINK_GET_COMMUNICATION_MODE(&ent) == ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED)
      {
        apsme_group = zgp_calc_alias_source_address(&gpdf_info->zgpd_id);
#ifdef ZB_CERTIFICATION_HACKS
        if (ZGP_CERT_HACKS().gp_sink_use_assigned_alias_for_dgroup_commissioning)
        {
          ent.zgpd_assigned_alias = ZGP_CERT_HACKS().gp_sink_assigned_alias;
          ZGP_TBL_SINK_SET_ASSIGNED_ALIAS(&ent);
        }
#endif  /* ZB_CERTIFICATION_HACKS */
      }

      ent.zgpd_id = gpdf_info->zgpd_id.addr;
      ent.u.sink.device_id = comm_params->zgpd_device_id;
      ent.u.sink.match_dev_tbl_idx = match_table_idx;
      TRACE_MSG(TRACE_ZGP3, "match_dev_tbl_idx %hd", (FMT__H, match_table_idx));

      if (gpdf_info->zgpd_id.app_id == ZB_ZGP_APP_ID_0010)
      {
        ent.endpoint = gpdf_info->zgpd_id.endpoint;
      }

      if (ZB_GPDF_COMM_OPT_SEC_KEY_REQ(comm_params->options) == 0)
      {
        if (zb_zgp_key_gen((enum zb_zgp_security_key_type_e)zgpd_sec_key_type, &gpdf_info->zgpd_id, gpdf_info->key, ent.zgpd_key) == RET_OK)
        {
          TRACE_MSG(TRACE_ZGP3, "key generated successfully: " TRACE_FORMAT_128, (FMT__A_A, TRACE_ARG_128(ent.zgpd_key)));
        }
      }
      else
      {
        if (zb_zgp_key_gen((enum zb_zgp_security_key_type_e)ZGP_GP_SHARED_SECURITY_KEY_TYPE, &gpdf_info->zgpd_id, gpdf_info->key, ent.zgpd_key) == RET_OK)
        {
          TRACE_MSG(TRACE_ZGP3, "key generated successfully: " TRACE_FORMAT_128, (FMT__A_A, TRACE_ARG_128(ent.zgpd_key)));

          /* Set sink entry table security key type according to SINK decision (was as device sent)*/
          if (ZGP_GP_SHARED_SECURITY_KEY_TYPE != ZB_ZGP_SEC_KEY_TYPE_NO_KEY)
          {
            ent.sec_options = ZGP_TBL_FILL_SEC_OPTIONS(zgpd_sec_level, ZGP_GP_SHARED_SECURITY_KEY_TYPE);
            TRACE_MSG(TRACE_ZGP3, "new entry key type set: %hd", (FMT__H, ZGP_GP_SHARED_SECURITY_KEY_TYPE));
          }
          else
          {
            ent.sec_options = ZGP_TBL_FILL_SEC_OPTIONS(zgpd_sec_level, ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL);
            TRACE_MSG(TRACE_ZGP3, "new entry key type set: %hd", (FMT__H, ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL));
          }
        }
      }

#ifdef ZGP_INCLUDE_DST_LONG_ADDR
    /* Save source IEEE address if it's present and AppID=0x00.
     * GPD uses SrcID for identification, so sink table entry will contain SrcID.
     * But add IEEE address anyway in order to perform acknowledged transmission to GPD
     */
    if ((gpdf_info->mac_addr_flds_len == 12) /* broadcast dst addr and extended source address */
        && (gpdf_info->zgpd_id.app_id == ZB_ZGP_APP_ID_0000))
    {
      ZB_LETOH64(ent.u.sink.ieee_addr, gpdf_info->mac_addr_flds.comb.src_addr);
    }
#endif
    ent.security_counter = gpdf_info->sec_frame_counter;

    if (zgp_sink_table_write(&gpdf_info->zgpd_id, &ent) != RET_OK)
    {
      TRACE_MSG(TRACE_ZGP1, "No space left in sink table. Abort commissioning", (FMT__0));
      zb_buf_free(param);
    }
    else
    {
      if (apsme_group)
      {
        if (zb_apsme_add_group_internal(apsme_group, ZGP_ENDPOINT) == ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_TABLE_FULL))
        {
          TRACE_MSG(TRACE_ZGP2, "Error adding SINK GP to APS group", (FMT__0));
        }
        else
        {
          TRACE_MSG(TRACE_ZGP3, "Create APS-layer link to GP EP: group: 0x%04x", (FMT__D, apsme_group));
        }
      }

      if (ZB_GPDF_COMM_OPT_APP_INF_PRESENT(comm_params->options) && (comm_params->app_info.options.app_descr_flw))
      {
        zgp_runtime_app_tbl_ent_t *ent = zb_zgp_alloc_app_tbl_ent_by_id(&gpdf_info->zgpd_id);
        if (ent)
        {
          ent->reply_buf = param;
          TRACE_MSG(TRACE_ZGP3, "Waiting app description", (FMT__0));
          ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_COMMISSIONING_WAIT_APP_DESCR);
          ZB_SCHEDULE_ALARM(zb_zgps_app_descr_timeout, ent->reply_buf, ZB_APP_DESCR_TIMEOUT);
        }
        else
        {
          TRACE_MSG(TRACE_ERROR, "GP App table is full, cancel commissioning", (FMT__0));
          ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_COMMISSIONING_CANCELLED);
          *ZB_BUF_GET_PARAM(param, zb_uint8_t) = ZB_ZGP_COMMISSIONING_NO_MATCH_ERROR;
          zb_zgps_finalize_commissioning(param);
        }

        param = 0;
      }

      if (param)
      {
        if (param && ZB_GPDF_EXT_NFC_GET_RX_AFTER_TX(gpdf_info->nwk_ext_frame_ctl))
        {
          #ifdef ZB_ENABLE_ZGP_DIRECT
          zb_uint8_t rx_directly = gpdf_info->rx_directly;
          #endif  /* ZB_ENABLE_ZGP_DIRECT */

          zgps_send_commissioning_reply(param);

          #ifdef ZB_ENABLE_ZGP_DIRECT
          if (rx_directly)
          {
            ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_COMMISSIONING_REPLY_ADDED_TO_Q);
          }
          else
          #endif  /* ZB_ENABLE_ZGP_DIRECT */
          {
            ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_COMMISSIONING_REPLY_SENT);
          }
        }
        else
        {
          TRACE_MSG(TRACE_ZGP1, "Unidirectional Commissioning is successful", (FMT__0));
          *ZB_BUF_GET_PARAM(param, zb_uint8_t) = ZB_ZGP_COMMISSIONING_COMPLETED;
          zb_zgps_finalize_commissioning(param);
        }
      }
    }
  }
  else
  {
    *ZB_BUF_GET_PARAM(param, zb_uint8_t) = ZB_ZGP_COMMISSIONING_NO_MATCH_ERROR;
    zb_zgps_finalize_commissioning(param);
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_process_commissioning_cmd_cont", (FMT__0));
}

/**
 * @brief Fill packet payload for commissioning reply
 *
 * @param param     [payload]  Buffer reference
 * @param param     [ent]  entry table to get info from
 *
 */
static zb_uint8_t fill_comm_resp_security_header(zb_uint8_t *payload, zb_zgp_sink_tbl_ent_t *ent)
{
  zb_uint8_t     *ptr;
  zb_uint8_t      pan_id_present = 0;
  zb_uint8_t      gpd_key_present = 0;
  zb_uint8_t      gpd_key_encrypted = 0;
  zb_uint8_t      gpd_sec_level;

  ptr = payload + 1;
  gpd_sec_level = ZGP_TBL_SINK_GET_SEC_LEVEL(ent);

  if (ZB_ZGPS_COMM_GET_PAN_ID_REQ())
  {
    pan_id_present = 1;
    TRACE_MSG(TRACE_ZGP2, "pan_id_present = 1;",(FMT__0));
    ZB_GPDF_PUT_UINT16(ptr, &ZB_PIBCACHE_PAN_ID());
  }

  /* proceed security options */
  if (ZB_ZGPS_COMM_GET_ZGPD_KEY_PRESENT() && ZB_ZGPS_COMM_GET_SEC_KEY_REQ())
  {
    if (ZGP_GP_SHARED_SECURITY_KEY_TYPE > ZB_ZGP_SEC_KEY_TYPE_NO_KEY)
    {
      gpd_key_present = 1;
      TRACE_MSG(TRACE_ZGP2, "gpd_key_present = 1;",(FMT__0));

      if (ZB_ZGPS_COMM_GET_ZGPD_KEY_ENCRYPTED())
      {
        zb_uint8_t encrypted_key[ZB_CCM_KEY_SIZE];
        zb_uint8_t mic[ZB_CCM_M];

        gpd_key_encrypted = 1;
        TRACE_MSG(TRACE_ZGP2, "gpd_key_encrypted = 1;",(FMT__0));

        TRACE_MSG(TRACE_ZGP3, "encrypting key with gp_link_key: " TRACE_FORMAT_128, (FMT__A_A, TRACE_ARG_128(ZGP_GP_LINK_KEY)));
        zb_zgp_protect_gpd_key(ZB_FALSE, &ZGP_CTXC().comm_data.zgpd_id, ent->zgpd_key, ZGP_GP_LINK_KEY, encrypted_key, ZGP_CTXC().comm_data.gpdf_security_frame_counter, mic);

        ZB_MEMCPY(ptr, encrypted_key, ZB_CCM_KEY_SIZE);
        ptr += ZB_CCM_KEY_SIZE;
        ZB_MEMCPY(ptr, mic, ZB_CCM_M);
        TRACE_MSG(TRACE_ZGP3, "key_mic: 0x%02x%02x%02x%02x"  , (FMT__H_H_H_H, mic[0],mic[1],mic[2],mic[3]));
        ptr += ZB_CCM_M;
      }
      else
      {
        ZB_MEMCPY(ptr, ent->zgpd_key, ZB_CCM_KEY_SIZE);
        TRACE_MSG(TRACE_ERROR, "unencrypted key passed to GPD: " TRACE_FORMAT_128, (FMT__A_A, TRACE_ARG_128(ptr)));
        ptr += ZB_CCM_KEY_SIZE;
      }
    }

    if ((gpd_sec_level >= ZB_ZGP_SEC_LEVEL_FULL_NO_ENC) && gpd_key_present && gpd_key_encrypted)
    {
      zb_uint32_t cntr = ZGP_CTXC().comm_data.gpdf_security_frame_counter + 1;
      TRACE_MSG(TRACE_ZGP2, "gpd_outgoing_counter_present 0x%08x",(FMT__L, cntr));
      ZB_GPDF_PUT_UINT32(ptr, &cntr);
    }
  }

  payload[0] = ZB_GPDF_COMM_REPLY_OPT_FLD(pan_id_present, /* PAN ID present */
                                          gpd_key_present, /* GPD key present */
                                          gpd_key_encrypted, /* GPD key encrypted */
                                          gpd_sec_level,
                                          ZGP_TBL_GET_SEC_KEY_TYPE(ent)); /*GPD key type */

  return ptr - payload;
}

#ifdef ZB_ENABLE_ZGP_DIRECT
/**
 * @brief Put commissioning reply GPDF packet into TX queue
 *
 * @param param     [in]  Buffer reference
 *
 */
static void zb_zgps_direct_send_commissioning_reply(zb_uint8_t param)
{
  zb_zgp_sink_tbl_ent_t  ent;
  zb_gp_data_req_t      *req;

  TRACE_MSG(TRACE_ZGP2, ">> zb_zgps_direct_send_commissioning_reply, param %hd", (FMT__H, param));

  if (zgp_sink_table_read(&ZGP_CTXC().comm_data.zgpd_id, &ent) == RET_OK)
  {
    zb_uint8_t x[ZB_ZGP_TX_CMD_PLD_MAX_SIZE];
    zb_uint8_t x_len = 0;

    /* Fill commissioning reply fields other than Options */
    x_len = fill_comm_resp_security_header(x, &ent);
    req = zb_buf_initial_alloc(param, sizeof(zb_gp_data_req_t)-sizeof(req->pld)+x_len+1);
    req->payload_len = x_len;

    ZB_MEMCPY(req->pld,x,req->payload_len);

    req->handle = ZB_ZGP_HANDLE_ADD_COMMISSIONING_REPLY;
    req->action = ZB_GP_DATA_REQ_ACTION_ADD_GPDF;
    req->tx_options = ZB_GP_DATA_REQ_USE_GP_TX_QUEUE;
    req->cmd_id = ZB_GPDF_CMD_COMMISSIONING_REPLY;

    TRACE_MSG(TRACE_ZGP2, "Payload_len: %d",(FMT__H, req->payload_len));

    ZB_MEMCPY(&req->zgpd_id, &ZGP_CTXC().comm_data.zgpd_id, sizeof(zb_zgpd_id_t));
    req->tx_q_ent_lifetime = ZB_GP_TX_QUEUE_ENTRY_LIFETIME_INF;

    schedule_gp_txdata_req(param);
    ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_COMMISSIONING_REPLY_ADDED_TO_Q);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Oops: can't get sink table entry", (FMT__0));
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_direct_send_commissioning_reply", (FMT__0));
}
#endif  /* ZB_ENABLE_ZGP_DIRECT */

static void zb_zgp_handle_app_descr_init_values(zb_zgpd_id_t *zgpd_id_p)
{
  zgp_runtime_app_tbl_ent_t *ent = zb_zgp_get_app_tbl_ent_by_id(zgpd_id_p);

  if (ent && ent->status == ZGP_APP_TBL_ENT_STATUS_COMPLETE)
  {
    /* handle default attribute values */
    zb_buf_get_out_delayed(zb_zgp_app_descr_send_attr_report);
  }
}

/**
 * @brief Callback after sending commissioning reply via ZCL to selected temp master
 *
 * @param param     [in]  Buffer reference
 *
 */
static void zb_zgps_commissioning_req_send_response_cb(zb_uint8_t param)
{
  zb_zcl_command_send_status_t *cmd_send_status = ZB_BUF_GET_PARAM(param, zb_zcl_command_send_status_t);
  zb_uint16_t                   tmp_addr;

  TRACE_MSG(TRACE_ZGP2, ">> zb_zgps_commissioning_req_send_response_cb %hd",
            (FMT__H, param));

  if (cmd_send_status->status != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zb_zgps_commissioning_req_send_response error %hd",
              (FMT__H, cmd_send_status->status));
  }

  if (ZGP_CTXC().sink_mode == ZB_ZGP_COMMISSIONING_MODE)
  {
    ZB_ASSERT(ZGP_CTXC().comm_data.selected_temp_master_idx < ZB_ZGP_MAX_TEMP_MASTER_COUNT);
    tmp_addr = ZGP_CTXC().comm_data.temp_master_list[ZGP_CTXC().comm_data.selected_temp_master_idx].short_addr;
    ZB_ASSERT(tmp_addr != ZB_ZGP_TEMP_MASTER_EMPTY_ENTRY);

#ifndef ZB_ENABLE_ZGP_DIRECT
    ZB_ASSERT(tmp_addr != ZB_PIBCACHE_NETWORK_ADDRESS());
#else
    if (tmp_addr == ZB_PIBCACHE_NETWORK_ADDRESS())
    {
      TRACE_MSG(TRACE_ZGP2, "zb_zgps_direct_send_commissioning_reply", (FMT__0));
      zb_zgps_direct_send_commissioning_reply(param);
    }
    else
#endif  /* ZB_ENABLE_ZGP_DIRECT */
    {
      ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_COMMISSIONING_REPLY_SENT);
      zb_buf_free(param);
    }
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "sink in operational mode, can't send commissioning reply; free buf", (FMT__0));
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_commissioning_req_send_response_cb", (FMT__0));
}

/**
 * @brief Send commissioning reply via ZCL to selected temp master
 *
 * @param param     [in]  Buffer reference
 *
 */
static void zb_zgps_commissioning_req_send_response(zb_uint8_t param)
{
  zb_gpdf_info_t       *gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);
  zb_zgp_gp_response_t *resp_ptr;
  zb_zgp_gp_response_t  resp;
  zb_zgp_sink_tbl_ent_t ent;

  TRACE_MSG(TRACE_ZGP2, ">> zb_zgps_commissioning_req_send_response %hd",
            (FMT__H, param));

  if (zgp_sink_table_read(&ZGP_CTXC().comm_data.zgpd_id, &ent) == RET_OK)
  {
    //SL==0 or SKT== 0b011 (GPD group key), 0b001 (NWK key) or 0b111 (derived individual GPD key) - no GPD key, key is in the gpSharedSecurityKey (GPD key not used)
    //SL>0 and SKT==0b111 - no GPD key, in gpSharedSecurityKey - base for calc result key ? (not used, where is the oob?)

    ZB_ASSERT(ZGP_CTXC().sink_mode == ZB_ZGP_COMMISSIONING_MODE);
    ZB_ASSERT(ZGP_CTXC().comm_data.selected_temp_master_idx < ZB_ZGP_MAX_TEMP_MASTER_COUNT);
    ZB_ASSERT(ZGP_CTXC().comm_data.temp_master_list[ZGP_CTXC().comm_data.selected_temp_master_idx].short_addr
              != ZB_ZGP_TEMP_MASTER_EMPTY_ENTRY);

    resp.options = ZB_ZGP_FILL_GP_RESPONSE_OPTIONS(gpdf_info->zgpd_id.app_id,
                                                   /* TODO: set actual ep match bit flag */
                                                   0);
    resp.temp_master_addr = ZGP_CTXC().comm_data.temp_master_list[ZGP_CTXC().comm_data.selected_temp_master_idx].short_addr;
    resp.temp_master_tx_chnl = ZGP_CTXC().comm_data.oper_channel - ZB_ZGPD_FIRST_CH;
    resp.zgpd_addr = gpdf_info->zgpd_id.addr;
    resp.endpoint = gpdf_info->zgpd_id.endpoint;
    resp.gpd_cmd_id = ZB_GPDF_CMD_COMMISSIONING_REPLY;

    resp.payload[0] = fill_comm_resp_security_header(&resp.payload[1], &ent);

    ZB_ZCL_START_PACKET(param);
    resp_ptr = ZB_BUF_GET_PARAM(param, zb_zgp_gp_response_t);
    ZB_MEMCPY(resp_ptr, &resp, sizeof(zb_zgp_gp_response_t));

    TRACE_MSG(TRACE_ZGP2, "Payload_len:%d",(FMT__H, resp.payload[0]));

    zb_zgp_cluster_gp_response_send(param,
                                    ZB_NWK_BROADCAST_ALL_DEVICES,
                                    ZB_ADDR_16BIT_DEV_OR_BROADCAST,
                                    zb_zgps_commissioning_req_send_response_cb);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Oops: can't get sink table entry", (FMT__0));
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_commissioning_req_send_response", (FMT__0));
}

/**
 * @brief Send commissioning reply via ZCL to selected temp master
 *        or direct to ZGPD
 *
 * @param param     [in]  Buffer reference
 *
 */
static void zgps_send_commissioning_reply(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZGP2, ">> zgps_send_commissioning_reply, param %hd", (FMT__H, param));

  zb_zgps_commissioning_req_send_response(param);

  TRACE_MSG(TRACE_ZGP2, "<< zgps_send_commissioning_reply", (FMT__0));
}

static void zb_zgps_finalize_commissioning_cont(zb_uint8_t param)
{
  ZGP_CTXC().comm_data.need_send_dev_annce = 0;
  notify_user_and_switch_to_oper_mode(param);
}

static void zb_zgps_finalize_commissioning(zb_uint8_t param)
{
  zb_uint8_t  status = *ZB_BUF_GET_PARAM(param, zb_uint8_t);

  TRACE_MSG(TRACE_ZGP2, ">> zb_zgps_finalize_commissioning, param %hd", (FMT__H, param));

  TRACE_MSG(TRACE_ZGP2, "status %hd", (FMT__H, status));

  if (ZGP_IS_SINK_IN_COMMISSIONING_MODE())
  {
    if (status != ZB_ZGP_COMMISSIONING_TIMED_OUT)
    {
      ZB_SCHEDULE_ALARM_CANCEL(zb_zgps_commissioning_timed_out, 0);
    }

    if (status != ZB_ZGP_COMMISSIONING_CANCELLED_BY_USER)
    {
      ZB_SCHEDULE_ALARM_CANCEL(zb_zgps_commissioning_cancel, 0);
    }

    if (status != ZB_ZGP_COMMISSIONING_COMPLETED)
    {
      ZGP_CTXC().comm_data.result = status;

      if (ZB_ZGPD_IS_SPECIFIED(&ZGP_CTXC().comm_data.zgpd_id))
      {
        zgp_sink_table_del(&ZGP_CTXC().comm_data.zgpd_id);
#ifdef ZB_ENABLE_ZGP_DIRECT
        zgp_clean_zgpd_info_from_queue(param, &ZGP_CTXC().comm_data.zgpd_id,
            ZB_ZGP_HANDLE_REMOVE_AFTER_FAILED_COMM);
#endif  /* ZB_ENABLE_ZGP_DIRECT */
      }
#ifdef ZB_ENABLE_ZGP_DIRECT
      else
      {
        /* Delete channel configuration packet from queue and
         * then return to operational channel */
        zb_zgpd_id_t zgpd_id_all = { ZB_ZGP_APP_ID_0000, 0, {ZB_ZGP_SRC_ID_ALL} };

        zgp_clean_zgpd_info_from_queue(param, &zgpd_id_all,
                                       ZB_ZGP_HANDLE_REMOVE_AFTER_FAILED_COMM);
      }
#endif  /* ZB_ENABLE_ZGP_DIRECT */
    }
    else
    {
      zb_zgp_sink_tbl_ent_t ent;

      if (zgp_sink_table_read(&ZGP_CTXC().comm_data.zgpd_id, &ent) == RET_OK)
      {
        zb_zgp_gp_pairing_send_req_t *req;

        ZB_ZGP_GP_PAIRING_SEND_REQ_CREATE(param, req, &ent, zb_zgps_finalize_commissioning_cont);
        ZB_ZGP_GP_PAIRING_MAKE_SEND_OPTIONS(req, 1, 0, ZGP_CTXC().comm_data.need_send_dev_annce);
        zgp_cluster_send_gp_pairing(param);

        /* check and handle app description init values */
        zb_zgp_handle_app_descr_init_values(&ZGP_CTXC().comm_data.zgpd_id);
      }
    }

    if (ZB_ZGP_SINK_IS_SEND_ENTER_OR_LEAVE_FOR_PROXIES())
    {
      zb_buf_get_out_delayed(zgp_send_cluster_proxy_commissioning_mode_leave_req);
    }
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "ZGPS is already in operational mode", (FMT__0));
    zb_buf_free(param);
  }
  ZB_ZGP_CLR_SINK_COMM_MODE();

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_finalize_commissioning", (FMT__0));
}

static void zb_zgps_process_decommissioning_cmd_cont_cont(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZGP2, ">> zb_zgps_process_decommissioning_cmd_cont_cont, param %hd",
            (FMT__H, param));

  zgp_sink_table_del(&ZGP_CTXC().comm_data.zgpd_id);
#ifdef ZB_ENABLE_ZGP_DIRECT
  zgp_clean_zgpd_info_from_queue(param, &ZGP_CTXC().comm_data.zgpd_id,
                                 ZB_ZGP_HANDLE_DEFAULT_HANDLE);
#endif  /* ZB_ENABLE_ZGP_DIRECT */
  notify_user_and_switch_to_oper_mode(0);

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_process_decommissioning_cmd_cont_cont", (FMT__0));
}

static void zb_zgps_process_decommissioning_cmd_cont(zb_uint8_t param)
{
  zb_zgp_sink_tbl_ent_t ent;

  TRACE_MSG(TRACE_ZGP2, ">> zb_zgps_process_decommissioning_cmd_cont, param %hd",
      (FMT__H, param));

  if (zgp_sink_table_read(&ZGP_CTXC().comm_data.zgpd_id, &ent) == RET_OK)
  {
    zb_zgp_gp_pairing_send_req_t *req;
    zb_uint8_t dest_ep = ((zb_gpdf_info_t*) ZB_BUF_GET_PARAM(param, zb_gpdf_info_t))->zgpd_id.endpoint;

    if (dest_ep == 0xff)
    {
      ent.endpoint = dest_ep;
    }
    ZB_ZGP_GP_PAIRING_SEND_REQ_CREATE(param, req, &ent, zb_zgps_process_decommissioning_cmd_cont_cont);
    ZB_ZGP_GP_PAIRING_MAKE_SEND_OPTIONS(req, 0, 1, 0);
    zgp_cluster_send_gp_pairing(param);
  }
  else
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_process_decommissioning_cmd_cont", (FMT__0));
}

static void zb_zgps_process_decommissioning_cmd(zb_uint8_t param)
{
  zb_gpdf_info_t        *gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);
  zb_zgp_sink_tbl_ent_t  ent;

  TRACE_MSG(TRACE_ZGP2, ">> zb_zgps_process_decommissioning_cmd, param %hd",
      (FMT__H, param));

  /* On receipt of  GPD Decommissioning command, the sink checks if it has a Sink Table entry for
   * this GPD (and Endpoint, matching or 0x00 or 0xff, if ApplicationID = 0b010). If not, the frame
   * is ignored. If yes, the sink performs a freshness check, as described in A.3.6.1.2.1 and
   * compares the SecurityLevel and SecurityKeyType with the values stored in the Sink Table
   * entry. If any of those checks fails, the frame is silently dropped. If all those checks
   * succeed, the sink removes this Sink Table entry, removes/replaces with generic entries the
   * corresponding Translation Table entries if Translation Table functionality is supported, and
   * removes Green Power EndPoint membership at APS level in the groups listed in the removed entry,
   * if any. Then, the sink schedules sending of a GP Pairing command for this GPD (and Endpoint,
   * matching or 0x00 or 0xff, if ApplicationID = 0b010), with the RemoveGPD sub-field set. If the
   * removed Sink Table entry included any pre-commissioned groups,  and if the GPD Decommissioning
   * command was received in commissioning mode, the sink SHALL send GP Pairing Configuration
   * message, with Action sub-field of the Actions field set to 0b100, SendGPPairing sub-field of
   * the Actions field set to 0b0, and Number of paired endpoints field set to 0xfe.
   */

  if (zgp_sink_table_read(&gpdf_info->zgpd_id, &ent) == RET_OK)
  {
    ZGP_CTXC().comm_data.result = ZB_ZGP_ZGPD_DECOMMISSIONED;
    ZGP_CTXC().comm_data.zgpd_id = gpdf_info->zgpd_id;

    if (ZGP_CTXC().sink_mode == ZB_ZGP_COMMISSIONING_MODE &&
        ZGP_TBL_SINK_GET_COMMUNICATION_MODE(&ent) == ZGP_COMMUNICATION_MODE_GROUPCAST_PRECOMMISSIONED)
    {
      zgp_cluster_send_pairing_configuration(param,
                                             ZB_NWK_BROADCAST_ALL_DEVICES,
                                             ZB_ADDR_16BIT_DEV_OR_BROADCAST,
                                             ZB_ZGP_FILL_GP_PAIRING_CONF_ACTIONS(ZGP_PAIRING_CONF_REMOVE_GPD, 0),
                                             &ent,
                                             0xfe,
                                             NULL,
                                             0,  // app_info,
                                             0,  // manuf_id,
                                             0,  // model_id,
                                             0,  // num_gpd_commands,
                                             NULL,//gpd_commands,
                                             NULL,//cluster_list,
                                             zb_zgps_process_decommissioning_cmd_cont);
    }
    else
    {
      zb_zgps_process_decommissioning_cmd_cont(param);
    }
  }
  else
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_process_decommissioning_cmd", (FMT__0));
}

/**
 * @brief Select operation and perform it after
 *        selecting best temp master
 *
 * @param param     [in]  Buffer reference
 *
 */
static void zb_zgps_process_after_select_temp_master(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZGP2, ">> zb_zgps_process_after_select_temp_master %hd",
            (FMT__H, param));

  ZB_ASSERT(ZGP_CTXC().comm_data.selected_temp_master_idx < ZB_ZGP_MAX_TEMP_MASTER_COUNT);
  ZB_ASSERT(ZGP_CTXC().comm_data.temp_master_list[ZGP_CTXC().comm_data.selected_temp_master_idx].short_addr
            != ZB_ZGP_TEMP_MASTER_EMPTY_ENTRY);

  switch (ZGP_CTXC().comm_data.state)
  {
    case ZGP_COMM_STATE_CHANNEL_REQ_COLLECT:
      ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_CHANNEL_CONFIG_GET_CUR_CHANNEL);
      zb_zgp_channel_config_get_current_channel(param);
      break;
    case ZGP_COMM_STATE_CHANNEL_CONFIG_SENT:
      TRACE_MSG(TRACE_ZGP2, "Send resp again", (FMT__0));
      zb_zgps_channel_req_send_response(param);
      break;
    case ZGP_COMM_STATE_COMMISSIONING_REPLY_SENT:
    {
      zb_gpdf_info_t *gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);
      zb_uint16_t     short_addr;

#ifdef ZB_ENABLE_ZGP_DIRECT
      if (gpdf_info->rx_directly)
      {
        short_addr = ZB_PIBCACHE_NETWORK_ADDRESS();
      }
      else
#endif  /* ZB_ENABLE_ZGP_DIRECT */
      {
        short_addr = gpdf_info->mac_addr_flds.proxy_info.short_addr;
      }

      if (ZGP_CTXC().comm_data.temp_master_list[ZGP_CTXC().comm_data.selected_temp_master_idx].short_addr !=
          short_addr)
      {
        TRACE_MSG(TRACE_ZGP2, "Already selected other TempMaster, drop buffer", (FMT__0));
        zb_buf_free(param);
      }
      else
      {
        TRACE_MSG(TRACE_ZGP2, "Send comm reply again", (FMT__0));
        zgps_send_commissioning_reply(param);
      }
      break;
    }
    case ZGP_COMM_STATE_COMISSIONING_REQ_COLLECT:
      zb_zgps_process_commissioning_cmd(param);
      break;
    default:
      TRACE_MSG(TRACE_ZGP2, "Strange state - free buffer %hd", (FMT__H, param));
      zb_buf_free(param);
      break;
  };

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_process_after_select_temp_master",
            (FMT__0));
}

#ifdef ZB_ENABLE_ZGP_ADVANCED
/* A.3.6.2.3
  The sink selects the node with the best GPP-GPD link value for this GPD
  (and Endpoint, if Applica-tionID = 0b010 and the sink selects Transmit on endpoint match = 0b1),
  whereby better GPP-GPD link is defined as one having higher value of the Link quality sub-field,
  and if Link quality is equal, as one having higher value of the RSSI sub-field;
  or if multiple have the same GPP-GPD link value, the one with the best GPP-GPD link value and
  lowest short address.
*/
static zb_int8_t zb_zgps_compare_link_quality(zb_uint8_t lq1, zb_uint8_t lq2)
{
  if (((lq1 & 0xC0) >> 6) > ((lq2 & 0xC0) >> 6))
  {
    return 1;
  }
  else
    if (((lq1 & 0xC0) >> 6) < ((lq2 & 0xC0) >> 6))
  {
    return -1;
  }
  else
  if ((lq1 & 0x3F) > (lq2 & 0x3F))
  {
    return 1;
  }
  else
  if ((lq1 & 0x3F) < (lq2 & 0x3F))
  {
    return -1;
  }
  return 0;
}

/**
 * @brief Select best temp master after wait DMAX timeout
 *
 * @param param     [in]  Buffer reference
 *
 */
void zb_zgps_select_temp_master_after_delay(zb_uint8_t param)
{
  zb_uindex_t i;

  TRACE_MSG(TRACE_ZGP2, ">> zb_zgps_select_temp_master_after_delay %hd",
            (FMT__H, param));

  for (i = 0; i < ZB_ZGP_MAX_TEMP_MASTER_COUNT; i++)
  {
    if (ZGP_CTXC().comm_data.temp_master_list[i].short_addr !=
        ZB_ZGP_TEMP_MASTER_EMPTY_ENTRY)
    {
      zb_int8_t cmp_res;

      TRACE_MSG(TRACE_ZGP2, "temp_master idx %hd, short_addr 0x%hx, lqi %hd, rssi %d",
                (FMT__H_D_H_D, i, ZGP_CTXC().comm_data.temp_master_list[i].short_addr,
                 ((ZGP_CTXC().comm_data.temp_master_list[i].link & 0xC0) >> 6),
                  ZGP_CTXC().comm_data.temp_master_list[i].link & 0x3F));

      if (ZGP_CTXC().comm_data.selected_temp_master_idx == ZB_ZGP_UNSEL_TEMP_MASTER_IDX)
      {
        ZGP_CTXC().comm_data.selected_temp_master_idx = i;
        continue;
      }

      ZB_ASSERT(ZGP_CTXC().comm_data.selected_temp_master_idx < ZB_ZGP_MAX_TEMP_MASTER_COUNT);

      cmp_res = zb_zgps_compare_link_quality(ZGP_CTXC().comm_data.temp_master_list[i].link,
                                             ZGP_CTXC().comm_data.temp_master_list[ZGP_CTXC().comm_data.selected_temp_master_idx].link);
      if (cmp_res > 0)
      {
        ZGP_CTXC().comm_data.selected_temp_master_idx = i;
      }
      else
      if (cmp_res == 0)
      {
        if (ZGP_CTXC().comm_data.temp_master_list[i].short_addr < ZGP_CTXC().comm_data.temp_master_list[ZGP_CTXC().comm_data.selected_temp_master_idx].short_addr)
        {
          ZGP_CTXC().comm_data.selected_temp_master_idx = i;
        }
      }
    }
  }

  TRACE_MSG(TRACE_ZGP2, "selected_temp_master_idx %hd short_addr 0x%hx",
            (FMT__H_D, ZGP_CTXC().comm_data.selected_temp_master_idx,
              ZGP_CTXC().comm_data.temp_master_list[ZGP_CTXC().comm_data.selected_temp_master_idx].short_addr));

  zb_zgps_process_after_select_temp_master(param);

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_select_temp_master_after_delay",
            (FMT__0));
}
#endif  /* ZB_ENABLE_ZGP_ADVANCED */

/**
 * @brief Get temp master entry by short address from commissioning context
 *
 * @param short_addr     [in]  Short address of temp master
 *
 * @return proxy entry info reserved for this short address
 */
static zb_zgp_gp_proxy_info_t* zb_zgps_get_temp_master_entry(zb_uint16_t short_addr)
{
  zb_zgp_gp_proxy_info_t *res = NULL;
  zb_uint8_t              i;

  TRACE_MSG(TRACE_ZGP2, ">> zb_zgps_get_temp_master_entry", (FMT__0));

  for (i = 0; i < ZB_ZGP_MAX_TEMP_MASTER_COUNT; i++)
  {
    if (ZGP_CTXC().comm_data.temp_master_list[i].short_addr ==
        short_addr)
    {
      TRACE_MSG(TRACE_ZGP2, "found temp master entry at index: %hd",
                (FMT__H, i));
      res = &ZGP_CTXC().comm_data.temp_master_list[i];
      break;
    }
  }
  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_get_temp_master_entry", (FMT__0));
  return res;
}

/**
 * @brief Get empty temp master entry from commissioning context
 *
 * @return empty proxy entry info
 */
static zb_zgp_gp_proxy_info_t* zb_zgps_get_empty_temp_master_entry(void)
{
  zb_zgp_gp_proxy_info_t *res = NULL;
  zb_uint8_t              i;

  TRACE_MSG(TRACE_ZGP2, ">> zb_zgps_get_empty_temp_master_entry", (FMT__0));

  for (i = 0; i < ZB_ZGP_MAX_TEMP_MASTER_COUNT; i++)
  {
    if (ZGP_CTXC().comm_data.temp_master_list[i].short_addr ==
        ZB_ZGP_TEMP_MASTER_EMPTY_ENTRY)
    {
      TRACE_MSG(TRACE_ZGP2, "found empty temp master entry at index: %hd",
                (FMT__H, i));
      res = &ZGP_CTXC().comm_data.temp_master_list[i];
      break;
    }
  }
  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_get_empty_temp_master_entry", (FMT__0));
  return res;
}

static zb_uint8_t zb_zgps_get_temp_master_entry_idx(zb_zgp_gp_proxy_info_t* ent)
{
  return ZB_ARRAY_IDX_BY_STRUCT_ELEM(ZGP_CTXC().comm_data.temp_master_list, ent, sizeof(zb_zgp_gp_proxy_info_t));
}

/**
 * @brief Collect commissioning notifications and select best temp master
 *
 * @param param     [in]  Buffer reference
 *
 * @return ZB_FALSE if need free buffer, ZB_TRUE otherwise
 *
 * @see ZGP spec, A.3.6.2.3
 */
static zb_bool_t zb_zgps_collect_gp_commissioning_notifications(zb_uint8_t param)
{
  zb_gpdf_info_t         *gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);
  zb_uint8_t              comm_state = ZGP_CTXC().comm_data.state;
  zb_zgp_gp_proxy_info_t *proxy_info_ent;
  zb_bool_t               start_collect = ZB_FALSE;
  zb_uint16_t             short_addr;

  TRACE_MSG(TRACE_ZGP2, ">> zb_zgps_collect_gp_commissioning_notifications %hd",
            (FMT__H, param));

  if (comm_state == ZGP_COMM_STATE_IDLE ||
      comm_state == ZGP_COMM_STATE_CHANNEL_CONFIG_SENT)
  {
    zb_zgps_clear_temp_master_list_ctx();
    start_collect = ZB_TRUE;
  }

#ifndef ZB_ENABLE_ZGP_ADVANCED
  if (!start_collect)
  {
    /* If multi-hop commissioning and GP Basic sink: the sink can select the first proxy from
     * which it receives the GP Commissioning Notification.
     */
  }
  else
#endif
  {
#ifndef ZB_ENABLE_ZGP_DIRECT
    ZB_ASSERT(!gpdf_info->rx_directly);
#else
    if (gpdf_info->rx_directly)
    {
      short_addr = ZB_PIBCACHE_NETWORK_ADDRESS();
    }
    else
#endif  /* ZB_ENABLE_ZGP_DIRECT */
    {
      short_addr = gpdf_info->mac_addr_flds.proxy_info.short_addr;
    }

    proxy_info_ent = zb_zgps_get_temp_master_entry(short_addr);

    if (!proxy_info_ent)
    {
      proxy_info_ent = zb_zgps_get_empty_temp_master_entry();
    }

    if (proxy_info_ent)
    {
      proxy_info_ent->short_addr = short_addr;

#ifdef ZB_ENABLE_ZGP_DIRECT
      if (gpdf_info->rx_directly)
      {
        proxy_info_ent->link = zb_zgp_cluster_encode_link_quality(gpdf_info->rssi,
                                                                  gpdf_info->lqi);
        TRACE_MSG(TRACE_ZGP3, "direct GPDF, short_addr %d link %hd",
                  (FMT__D_H, proxy_info_ent->short_addr, proxy_info_ent->link));
      }
      else
#endif  /* ZB_ENABLE_ZGP_DIRECT */
      {
        proxy_info_ent->link = gpdf_info->mac_addr_flds.proxy_info.link;
        TRACE_MSG(TRACE_ZGP3, "indirect GPDF, short_addr %d link %hd",
                  (FMT__D_H, proxy_info_ent->short_addr, proxy_info_ent->link));
      }

      TRACE_MSG(TRACE_ZGP3, "TempMaster 0x%hx link %hd",
                (FMT__D_H, short_addr, proxy_info_ent->link));

      if (start_collect)
      {
#ifdef ZB_ENABLE_ZGP_ADVANCED
        TRACE_MSG(TRACE_ZGP3, "wait for collect a couple of temp masters", (FMT__0));
        ZB_SCHEDULE_ALARM(zb_zgps_select_temp_master_after_delay,
                          param,
                          ZB_MILLISECONDS_TO_BEACON_INTERVAL(ZB_ZGP_DMAX_FOR_ACCUMULATE_TEMP_MASTER_INFO));
#else
        ZGP_CTXC().comm_data.selected_temp_master_idx = zb_zgps_get_temp_master_entry_idx(proxy_info_ent);
        TRACE_MSG(TRACE_ZGP2, "selected_temp_master_idx %hd short_addr 0x%hx",
            (FMT__H_D, ZGP_CTXC().comm_data.selected_temp_master_idx,
              ZGP_CTXC().comm_data.temp_master_list[ZGP_CTXC().comm_data.selected_temp_master_idx].short_addr));
        ZB_SCHEDULE_CALLBACK(zb_zgps_process_after_select_temp_master, param);
#endif
      }
      else
      {
        TRACE_MSG(TRACE_ZGP3, "collect temp master, drop buffer", (FMT__0));
      }
    }
    else
    {
      TRACE_MSG(TRACE_ZGP2, "not enough free temp master entries, drop buffer",
                (FMT__0));
    }
  }
  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_collect_gp_commissioning_notifications, start_collect %hd",
            (FMT__H, start_collect));
  return start_collect;
}

static void zgp_send_cluster_proxy_commissioning_mode_enter_req_cb(zb_uint8_t param)
{
  zb_buf_free(param);
  ZB_ZGP_CLR_SINK_COMM_MODE();
}

static void zgp_send_cluster_proxy_commissioning_mode_enter_req(zb_uint8_t param)
{
  zgp_cluster_send_proxy_commissioning_mode_enter_req(param,
                                                      ZGP_GPS_COMMISSIONING_EXIT_MODE,
                                                      ZGP_GPS_COMMISSIONING_WINDOW,
                                                      zgp_send_cluster_proxy_commissioning_mode_enter_req_cb);
}

static void zgp_send_cluster_proxy_commissioning_mode_leave_req(zb_uint8_t param)
{
  zgp_cluster_send_proxy_commissioning_mode_leave_req(param, NULL);
}

#ifdef ZB_ENABLE_ZGP_DIRECT
void zb_gp_sink_mlme_set_cfm_cb(zb_uint8_t param)
{
  zb_mac_status_t  status = MAC_SUCCESS;

  TRACE_MSG(TRACE_ZGP2, ">> zb_gp_sink_mlme_set_cfm_cb %hd", (FMT__H, param));

  /* If buffer is not empty, then it contains mlme-set.cfm for channel change */
  if (zb_buf_len(param))
  {
    zb_mlme_set_confirm_t *cfm = (zb_mlme_set_confirm_t *)zb_buf_begin(param);

    ZB_ASSERT(cfm->pib_attr == ZB_PHY_PIB_CURRENT_CHANNEL);

    TRACE_MSG(TRACE_ZGP2, "cfm->status %hd", (FMT__H, cfm->status));

    status = (zb_mac_status_t)cfm->status;
  }

  if (ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_COMMISSIONING_FINALIZING)
  {
    if (status != MAC_SUCCESS)
    {
      ZGP_CTXC().comm_data.result = ZB_ZGP_COMMISSIONING_CRITICAL_ERROR;
    }
    notify_user_and_switch_to_oper_mode(param);
    param = 0;
  }
  else if (status != MAC_SUCCESS)
  {
    *ZB_BUF_GET_PARAM(param, zb_uint8_t) = ZB_ZGP_COMMISSIONING_CRITICAL_ERROR;
    zb_zgps_finalize_commissioning(param);
    param = 0;
  }
  else if (ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_CHAN_CFG_SENT_RET_CHANNEL)
  {
    ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_CHANNEL_CONFIG_SENT);
  }
  else if (ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_CHAN_CFG_FAILED_RET_CHANNEL)
  {
    ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_IDLE);
  }
  else if (ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_CHANNEL_CONFIG_SET_TEMP_CHANNEL)
  {
    zb_uint8_t payload = ZGP_CTXC().comm_data.oper_channel - ZB_ZGPD_FIRST_CH;

#ifndef ZB_ENABLE_ZGP_ADVANCED
    /* GPD Channel Configuration command, carrying:
       - channel: bits 0-3
       - the Basic sub-field set to 0b1 for a Basic Sink (Combo): bit 4
    */
    payload |= 16;
#endif  /* !ZB_ENABLE_ZGP_ADVANCED */

    TRACE_MSG(TRACE_ZGP2, "Sink switch to temp master channel", (FMT__0));
    zgp_channel_config_add_to_queue(param, payload);
    param = 0;
  }
  else
  {
    TRACE_MSG(TRACE_ZGP2, "Sink back to IDLE", (FMT__0));
    ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_IDLE);
  }

  if (param)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_gp_sink_mlme_set_cfm_cb", (FMT__0));
}
#endif  /* ZB_ENABLE_ZGP_DIRECT */

/**
 * @brief Translate ZGPD command id into Zigbee ZCL command id
 */
static zb_uint8_t translate_zgpd_to_zigbee_cmd_id(zb_uint8_t zgpd_cmd_id)
{
  zb_uint_t   i;
  zb_uint8_t ret = 0;

  TRACE_MSG(TRACE_ZGP2, ">> translate_zgpd_to_zigbee_cmd_id, zgpd_cmd_id %hx",
      (FMT__H, zgpd_cmd_id));

  for (i = 0; i < ZGP_CTXC().match_info->cmd_mappings_count; i++)
  {
    TRACE_MSG(TRACE_ZGP3, "i %d cmd_mapping[i].zgp_cmd_id 0x%hx", (FMT__D_H, i, ZGP_CTXC().match_info->cmd_mapping[i].zgp_cmd_id));

    if (ZGP_CTXC().match_info->cmd_mapping[i].zgp_cmd_id == zgpd_cmd_id)
    {
      ret = ZGP_CTXC().match_info->cmd_mapping[i].zb_cmd_id;
      break;
    }
  }

  TRACE_MSG(TRACE_ZGP2, "<< translate_zgpd_to_zigbee_cmd_id, ret %hx", (FMT__H, ret));

  return ret;
}

/**
 * @brief Construct parsed ZCL header of Zigbee ZCL frame that is analogue of
 * received ZGPD command
 *
 * Given translation table entry this function fills ZCL header of Zigbee command.
 *
 * @param buf [in]  Buffer with incoming packet
 * @param sink_tbl_ent [in]  Pointer to the sink table entry
 * @param cluster_rec [in] Pointer to the cluster record
 * @param endpoint_desc [in] Pointer to the endpoint descriptor
 * @param zgpd_cmd_id [in] Command id
 * @param hdr [out] Pointer to the output ZCL parsed header
 * @param zgpd_addr [in] Pointer to the ZGPD addr
 * @param mac_seq_num [in] current mac sequence number
 */
static zb_ret_t zgps_create_zcl_parsed_hdr_use_ste(zb_bufid_t             buf,
                                                   zb_zgp_sink_tbl_ent_t  *sink_tbl_ent,
                                                   zgps_dev_cluster_rec_t *cluster_rec,
                                                   zb_af_endpoint_desc_t  *endpoint_desc,
                                                   zb_uint8_t              zgpd_cmd_id,
                                                   zb_zcl_parsed_hdr_t    *hdr,
                                                   zb_zgpd_addr_t         *zgpd_addr,
                                                   zb_uint8_t              mac_seq_num)
{
  zb_ret_t               ret = RET_OK;
  zb_zcl_cluster_desc_t *cluster_desc = get_cluster_desc(endpoint_desc, cluster_rec->cluster_id,
                                                         GET_CLUSTER_ROLE(cluster_rec));
  zb_zcl_addr_t         *zcl_src_addr = &ZB_ZCL_PARSED_HDR_SHORT_DATA(hdr).source;
  zb_uint8_t             Zigbee_cmd_id = translate_zgpd_to_zigbee_cmd_id(zgpd_cmd_id);

  TRACE_MSG(TRACE_ZGP2, ">> zgps_create_zcl_parsed_hdr_use_ste", (FMT__0));

  ZB_MEMSET(hdr, 0, sizeof(zb_zcl_parsed_hdr_t));
#ifdef ZB_ENABLE_HA
  hdr->addr_data.common_data.dst_endpoint = endpoint_desc->ep_id;
#endif
  hdr->cluster_id = cluster_rec->cluster_id;
  hdr->profile_id = endpoint_desc->profile_id;
  hdr->cmd_id = Zigbee_cmd_id;
  hdr->is_manuf_specific = ZB_FALSE;
  hdr->disable_default_response = ZB_TRUE;
  hdr->seq_number = mac_seq_num;

  if (ZGP_TBL_GET_APP_ID(sink_tbl_ent) == ZB_ZGP_APP_ID_0000)
  {
    zcl_src_addr->addr_type = ZB_ZCL_ADDR_TYPE_SRC_ID_GPD;
    zcl_src_addr->u.src_id = zgpd_addr->src_id;
  }
  else
  {
    zcl_src_addr->addr_type = ZB_ZCL_ADDR_TYPE_IEEE_GPD;
    hdr->addr_data.common_data.src_endpoint = sink_tbl_ent->endpoint;
    ZB_IEEE_ADDR_COPY(zcl_src_addr->u.ieee_addr, zgpd_addr->ieee_addr);
  }

  if (zgpd_cmd_id == ZB_GPDF_CMD_ATTR_REPORT ||
      zgpd_cmd_id == ZB_GPDF_CMD_READ_ATTRIBUTES ||
      zgpd_cmd_id == ZB_GPDF_CMD_READ_ATTR_RESP ||
      zgpd_cmd_id == ZB_GPDF_CMD_MANUF_SPEC_ATTR_REPORT ||
      zgpd_cmd_id == ZB_GPDF_CMD_MULTI_CLUSTER_ATTR_REPORT ||
      zgpd_cmd_id == ZB_GPDF_CMD_MANUF_SPEC_MULTI_CLUSTER_ATTR_REPORT ||
      zgpd_cmd_id == ZB_GPDF_CMD_REQUEST_ATTRIBUTES)
  {
    hdr->is_common_command = ZB_TRUE;
    if (zgpd_cmd_id == ZB_GPDF_CMD_MANUF_SPEC_ATTR_REPORT)
    {
      /* NK: Now set manuf id and cut the buffer (manuf id and cluster id) */
      hdr->is_manuf_specific = ZB_TRUE;
      ZB_LETOH16(&hdr->manuf_specific, zb_buf_begin(buf));
      zb_buf_cut_left(buf, 4);
    }
  }

  if (zgpd_cmd_id == ZB_GPDF_CMD_REQUEST_ATTRIBUTES)
  {
    zb_uint8_t options = *(zb_uint8_t*)zb_buf_begin(buf);
    zb_buf_cut_left(buf, 1);

    if (ZB_GPDF_REQUEST_ATTR_IS_MULTI_RECORD(options))
    {
      TRACE_MSG(TRACE_ERROR,
          "Multi-record request attributes command is not supported", (FMT__0));
      ret = RET_NOT_IMPLEMENTED;
    }
    else if (ZB_GPDF_REQUEST_ATTR_MANUF_FIELD_PRESENT(options))
    {
      hdr->is_manuf_specific = ZB_TRUE;
      ZB_LETOH16(&hdr->manuf_specific, zb_buf_begin(buf));
      zb_buf_cut_left(buf, 2);
    }
  }

  hdr->cmd_direction = (cluster_desc->role_mask == ZB_ZCL_CLUSTER_SERVER_ROLE) ?
                       ZB_ZCL_FRAME_DIRECTION_TO_CLI : ZB_ZCL_FRAME_DIRECTION_TO_SRV;

  TRACE_MSG(TRACE_ZGP2, "<< zgps_create_zcl_parsed_hdr_use_ste, ret %d", (FMT__D, ret));

  return ret;
}

static zb_uint8_t *zb_zgp_cut_clusters(zb_uint8_t *p, zb_uint8_t len, zb_bool_t cut_first)
{
  zb_uint16_t first_cl_id;
  zb_uint16_t cur_cl_id;
  zb_uint8_t  attr_record_len;
  zb_uint8_t  attr_value_len;
  zb_bool_t first_rec = ZB_TRUE;

  TRACE_MSG(TRACE_ZGP3, "zb_zgp_cut_clusters, p %p, len %hd, cut_first %hd", (FMT__P_H_H, p, len, cut_first));

  ZB_LETOH16(&first_cl_id, p);

  while(len > 5)
  {
    ZB_LETOH16(&cur_cl_id, p);
    attr_value_len = zb_zcl_get_attribute_size(*(p+4), p+5);

    if (attr_value_len == 0xFF)
    {
      return p;
    }

    attr_record_len = 5 + attr_value_len;
    if (len >= attr_record_len)
    {
      len -= attr_record_len;
    }
    else
    {
      return p;
    }

    if (((cur_cl_id == first_cl_id) && cut_first) ||
        ((cur_cl_id != first_cl_id) && !cut_first))
    {
      /* cut attribute record */
      ZB_MEMMOVE(p, p+attr_record_len, len);
    }
    else if ((cur_cl_id == first_cl_id) && !cut_first)
    {
      if (!first_rec)
      {
        /* cut only Cluster ID */
        ZB_MEMMOVE(p, p+2, len+attr_record_len-2);

        /* skip remaining */
        p += attr_record_len - 2;
      }
      else
      {
        first_rec = ZB_FALSE;
        /* skip */
        p += attr_record_len;
      }
    }
    else
    {
      /* skip */
      p += attr_record_len;
    }
  }

  return p;
}

static zb_uint8_t *zb_zgp_cut_first_cluster(zb_uint8_t *p, zb_uint8_t len)
{
  return zb_zgp_cut_clusters(p, len, ZB_TRUE);
}

static zb_uint8_t *zb_zgp_cut_extra_clusters(zb_uint8_t *p, zb_uint8_t len)
{
  return zb_zgp_cut_clusters(p, len, ZB_FALSE);
}

/**
 * @brief Extract one GPDF Report command from GPDF Multi-clusters command
 *
 * @param param [in] Reference to buffer with GPDF packet.
 *                   Buffer parameter contains filled @ref zb_gpdf_info_t struct
 */
static void zb_zgp_extract_report_from_multi_report(zb_uint8_t param, zb_uint16_t param_orig)
{
  zb_uint8_t  *rpos;
  zb_uint8_t  *max_pos;
  zb_uint8_t  pl_len_orig;
  zb_uint16_t cl_id;
  zb_uint16_t cur_cl_id;
  zb_uint8_t  manuf_specific_offset = 0;
  zb_uint16_t zgpd_cmd_id = ((zb_gpdf_info_t *)ZB_BUF_GET_PARAM(param_orig, zb_gpdf_info_t))->zgpd_cmd_id;
  zb_uint8_t  attr_len;

  TRACE_MSG(TRACE_ZGP1, ">>zb_zgp_extract_report_from_multi_report param %hd, param_orig %hd", (FMT__H_H, param, param_orig));

  if (zgpd_cmd_id == ZB_GPDF_CMD_MANUF_SPEC_MULTI_CLUSTER_ATTR_REPORT)
  {
    manuf_specific_offset = 2;
  }

  /* init vars */
  rpos = (zb_uint8_t *)zb_buf_begin(param_orig) + manuf_specific_offset;
  pl_len_orig = zb_buf_len(param_orig) - manuf_specific_offset;
  ZB_LETOH16(&cl_id, rpos);
  cur_cl_id = cl_id;
  max_pos = rpos + pl_len_orig;

  /* First check: maybe, only one cluster is here? Exclude extra buffer alloc
    * if possible. */
  while ((rpos < max_pos) && (cl_id == cur_cl_id))
  {
    /* skip Cluster ID, Attr ID */
    rpos += 4;

    if (rpos + 1 < max_pos)
    {
      /* get attribute length and skip attribute value */
      attr_len = zb_zcl_get_attribute_size(*rpos, rpos + 1);
      if (attr_len == 0xFF)
      {
        TRACE_MSG(TRACE_ERROR, "zb_zcl_get_attribute_size returns 0xFF, drop frame", (FMT__0));
        zb_buf_free(param_orig);
        return;
      }
      rpos += attr_len + 1;

      /* get new Cluster ID */
      if (rpos + 1 < max_pos)
      {
        ZB_LETOH16(&cl_id, rpos);
      }
    }
  }

  if ((cl_id != cur_cl_id) && (!param))
  {
    /* Need new buffer */
    TRACE_MSG(TRACE_ZGP1, "<< zb_zgp_extract_report_from_multi_report: reschedule with additional buf", (FMT__0));
    zb_buf_get_out_delayed_ext(zb_zgp_extract_report_from_multi_report, param, 0);
    return;
  }

  /* prepare buffer */
  {
    zb_uint8_t *end_pos;
    zb_uint8_t new_len;

    if (cl_id != cur_cl_id)
    {
      zb_gpdf_info_t *gpdf_info = ZB_BUF_GET_PARAM(param_orig, zb_gpdf_info_t);

      ZB_ASSERT(param);
      zb_buf_copy(param, param_orig);

      /* fix orig buffer */
      rpos = (zb_uint8_t *)zb_buf_begin(param_orig) + manuf_specific_offset;
      end_pos = zb_zgp_cut_first_cluster(rpos, pl_len_orig);

      new_len = end_pos - (zb_uint8_t *)zb_buf_begin(param_orig);
      zb_buf_cut_right(param_orig, pl_len_orig - new_len);

      gpdf_info->mac_seq_num += 0x40; /* different sequence number */

      ZB_SCHEDULE_CALLBACK2(zb_zgp_extract_report_from_multi_report, 0, param_orig);
    }
    else
    {
      /* We have only one cluster */
      ZB_ASSERT(!param);
      param = param_orig;
    }

    rpos = (zb_uint8_t *)zb_buf_begin(param) + manuf_specific_offset;
    end_pos = zb_zgp_cut_extra_clusters(rpos, pl_len_orig);

    /* Cut garbage at tail */
    new_len = end_pos - (zb_uint8_t *)zb_buf_begin(param);
    zb_buf_cut_right(param, pl_len_orig - new_len);
  }

  /* change cmd id */
  {
    zb_gpdf_info_t *gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);
    if (zgpd_cmd_id == ZB_GPDF_CMD_MANUF_SPEC_MULTI_CLUSTER_ATTR_REPORT)
    {
      gpdf_info->zgpd_cmd_id = ZB_GPDF_CMD_MANUF_SPEC_ATTR_REPORT;
    }
    else
    {
      gpdf_info->zgpd_cmd_id = ZB_GPDF_CMD_ATTR_REPORT;
    }
  }

  ZB_SCHEDULE_CALLBACK(zb_zgp_from_gpdf_to_zcl, param);

  TRACE_MSG(TRACE_ZGP1, "<<zb_zgp_extract_report_from_multi_report", (FMT__0));
}

static zb_ret_t zb_zgp_get_next_point_descr(zb_uint8_t **rpos, zb_uint8_t *max_pos, zgp_data_point_desc_t *point_desc)
{
  zb_uint8_t i;

  TRACE_MSG(TRACE_ZGP3, "zb_zgp_get_next_point_descr *rpos %p, max_pos %p", (FMT__P_P, *rpos, max_pos));

  #define INC_RPOS(n) { \
     *rpos += n; \
     if (*rpos > max_pos) { \
      TRACE_MSG(TRACE_ZGP2, "zb_zgp_get_next_point_descr ERR", (FMT__0)); \
      return RET_ERROR; \
     } \
  }

  if (*rpos >= max_pos)
  {
     return RET_ERROR;
  }

  TRACE_MSG(TRACE_ZGP3, "Point desription:", (FMT__0));

  ZB_MEMCPY(&point_desc->options, *rpos, 1);
  TRACE_MSG(TRACE_ZGP3, "opt: 0x%x", (FMT__H, **rpos));
  INC_RPOS(1);

  ZB_LETOH16(&point_desc->cluster_id, *rpos);
  TRACE_MSG(TRACE_ZGP3, "cluster_id: 0x%x", (FMT__D, point_desc->cluster_id));
  INC_RPOS(2);

  if (point_desc->options.manuf_id_present)
  {
    ZB_LETOH16(&point_desc->manuf_id, *rpos);
    TRACE_MSG(TRACE_ZGP3, "manuf_id: 0x%x", (FMT__D, point_desc->manuf_id));
    INC_RPOS(2);
  }

  TRACE_MSG(TRACE_ZGP3, "attr_rec_num: %hd", (FMT__H, point_desc->options.attr_records_num));
  for (i = 0; i < point_desc->options.attr_records_num + 1; i++)
  {
    ZB_LETOH16(&point_desc->attr_records_data[i].id, *rpos);
    TRACE_MSG(TRACE_ZGP3, "attr_id: 0x%x", (FMT__D, point_desc->attr_records_data[i].id));
    INC_RPOS(2);

    ZB_MEMCPY(&point_desc->attr_records_data[i].data_type, *rpos, 1);
    TRACE_MSG(TRACE_ZGP3, "attr_data_type: 0x%x", (FMT__H, point_desc->attr_records_data[i].data_type));
    INC_RPOS(1);

    ZB_MEMCPY(&point_desc->attr_records_data[i].options, *rpos, 1);
    TRACE_MSG(TRACE_ZGP3, "attr_options: 0x%x", (FMT__H, point_desc->attr_records_data[i].options));
    INC_RPOS(1);

    if (ZGP_ATTR_OPT_GET_REPORTED(point_desc->attr_records_data[i].options))
    {
      ZB_MEMCPY(&point_desc->attr_records_data[i].offset, *rpos, 1);
      TRACE_MSG(TRACE_ZGP3, "attr_reported_offset: %hd", (FMT__H, point_desc->attr_records_data[i].offset));
      INC_RPOS(1);
    }

    if (ZGP_ATTR_OPT_GET_VAL_PRESENT(point_desc->attr_records_data[i].options))
    {
      zb_uint8_t attr_size = zb_zcl_get_attribute_size(point_desc->attr_records_data[i].data_type, *rpos);

      if (attr_size != 0xff)
      {
        ZB_MEMCPY(&point_desc->attr_records_data[i].value, *rpos, attr_size);
        INC_RPOS(attr_size);
      }
      else
      {
        return RET_ERROR;
      }
    }
  }

  TRACE_MSG(TRACE_ZGP2, "zb_zgp_get_next_point_descr OK", (FMT__0));
  return RET_OK;
}

static void zb_zgp_procc_point_desc(zb_uint8_t param, zb_uint16_t cr_buf) /* cr_buf - compact reporting buffer */
{
  zb_gpdf_info_t        *gpdf_info = ZB_BUF_GET_PARAM(cr_buf, zb_gpdf_info_t);
  zgp_data_point_desc_t point_desc;
  zgp_report_desc_t     *report_desc;
  zb_uint8_t            *rpos;
  zb_uint8_t            *maxpos;
  zb_uint8_t            report_idx;
  zb_uint8_t            pd_offset;

  TRACE_MSG(TRACE_ZGP2, ">> zb_zgp_procc_point_desc param %hd, cr_buf %hd",
      (FMT__H_H, param, cr_buf));

  /* cut additional params */
  report_idx = *((zb_uint8_t *) zb_buf_begin(cr_buf));
  pd_offset = *((zb_uint8_t *) zb_buf_begin(cr_buf) + 1);
  zb_buf_cut_left(cr_buf, 2);
  TRACE_MSG(TRACE_ZGP3, "report_idx %hd, pd_offset %hd", (FMT__H_H, report_idx, pd_offset));

  /* Get report description */
  report_desc = zb_zgp_get_report_desc_from_app_tbl(&gpdf_info->zgpd_id, report_idx);

  if (!report_desc)
  {
    TRACE_MSG(TRACE_ZGP2, "<< zb_zgp_procc_point_desc: report description not found, drop frame", (FMT__0));
    zb_buf_free(param);
    zb_buf_free(cr_buf);
    return;
  }

  /* Get data point description */
  rpos = report_desc->point_descs_data + pd_offset;
  maxpos = &report_desc->point_descs_data[report_desc->point_descs_data_len];

  if (zb_zgp_get_next_point_descr(&rpos, maxpos, &point_desc) == RET_OK)
  {
    zb_uint8_t i;
    zb_uint8_t *p;

    TRACE_MSG(TRACE_ZGP3, "restore to simple attribute reporting", (FMT__0));

    /* Fill buffer */
    p = zb_buf_initial_alloc(param, 2); /* for ClID */

    /* Place manuf_id, if present */
    if (point_desc.options.manuf_id_present)
    {
      ZB_HTOLE16(p, &point_desc.manuf_id);
      p = zb_buf_alloc_right(param, 2);
    }

    /* Place cluster ID */
    ZB_HTOLE16(p, &point_desc.cluster_id);

    /* Place attribute id, data_type and value */
    for (i = 0; i < point_desc.options.attr_records_num + 1; i++)
    {
      if (ZGP_ATTR_OPT_GET_REPORTED(point_desc.attr_records_data[i].options))
      {
        zb_uint8_t value_len = zb_zcl_get_attribute_size(point_desc.attr_records_data[i].data_type, NULL);
        if ((value_len != 0xff) && ((point_desc.attr_records_data[i].offset + value_len) <= zb_buf_len(cr_buf)))
        {
          p = zb_buf_alloc_right(param, 3 + value_len); /* 3 = AttrID + AttrDataType */

          if (p)
          {
            ZB_HTOLE16(p, &point_desc.attr_records_data[i].id);
            p += 2;

            ZB_MEMCPY(p, &point_desc.attr_records_data[i].data_type, 1);
            p += 1;

            ZB_MEMCPY(p, ((zb_uint8_t *)zb_buf_begin(cr_buf)) + point_desc.attr_records_data[i].offset, value_len);
          }
        }
      }
    }

    /* If we have one or more attr records - call zb_zgp_from_gpdf_to_zcl*/
    if (zb_buf_len(param) > 5) /* 5 = ClID(2) + AttrID(2) + AttrDType(1)*/
    {
      /* copy gpdf_info to new buffer */
      zb_gpdf_info_t *new_gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);
      ZB_MEMCPY(new_gpdf_info, gpdf_info, sizeof(zb_gpdf_info_t));

      /* set command */
      new_gpdf_info->zgpd_cmd_id = point_desc.options.manuf_id_present ?
          ZB_GPDF_CMD_MANUF_SPEC_ATTR_REPORT
          : ZB_GPDF_CMD_ATTR_REPORT;

      TRACE_MSG(TRACE_ZGP3, "restore complete, schedule zb_zgp_from_gpdf_to_zcl, cmd 0x%x, param %hd",
          (FMT__H_H, new_gpdf_info->zgpd_cmd_id, param));

      ZB_SCHEDULE_CALLBACK(zb_zgp_from_gpdf_to_zcl, param);
    }
    else /* skip this data point */
    {
      TRACE_MSG(TRACE_ZGP3, "skip this data point", (FMT__0));
      zb_buf_free(param);
    }

    /* we may have additional point descs*/
    if (rpos < maxpos)
    {
      zb_uint8_t *q;
      TRACE_MSG(TRACE_ZGP3, "schedule zb_zgp_procc_point_desc", (FMT__0));
      q = zb_buf_alloc_left(cr_buf, 2);
      *q = report_idx;
      *(q+1) = rpos - (report_desc->point_descs_data + pd_offset);
      zb_buf_get_out_delayed_ext(zb_zgp_procc_point_desc, cr_buf, 0);
    }
    else
    {
      TRACE_MSG(TRACE_ZGP3, "compact reporting is processed", (FMT__0));
      zb_buf_free(cr_buf);
    }
  }
  else
  {
    TRACE_MSG(TRACE_ZGP3, "compact reporting is processed", (FMT__0));
    zb_buf_free(param);
    zb_buf_free(cr_buf);
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgp_procc_point_desc", (FMT__0));
}

static void zb_zgp_extract_report_from_compact_report(zb_uint8_t param)
{
  zb_uint8_t     report_idx = *((zb_uint8_t *) zb_buf_begin(param));
  zb_uint8_t     *p;

  TRACE_MSG(TRACE_ZGP1, ">> zb_zgp_extract_report_from_compact_report param %hd, len %hd, report_id %hd",
      (FMT__H_H_H, param, zb_buf_len(param), report_idx));

  zb_buf_cut_left(param, 1); /* cut report identifier */

  p  = zb_buf_alloc_left(param, 2);
  *p = report_idx;
  *(p + 1) = 0; /* offset */

  zb_buf_get_out_delayed_ext(zb_zgp_procc_point_desc, param, 0);

  TRACE_MSG(TRACE_ZGP1, "<< zb_zgp_extract_report_from_compact_report", (FMT__0));
}

static zgps_dev_match_rec_t* get_dev_match_record(zb_uint8_t match_dev_tbl_idx)
{
  if (ZGP_CTXC().match_info == NULL)
  {
    TRACE_MSG(TRACE_ERROR, "Error: match table does not exist", (FMT__0));
    return NULL;
  }

  if (match_dev_tbl_idx >= ZGP_CTXC().match_info->match_tbl_size)
  {
    TRACE_MSG(TRACE_ERROR, "Error: match_dev_tbl_idx >= ZGP_CTXC().match_info.match_tbl_size", (FMT__0));
    return NULL;
  }
  return (zgps_dev_match_rec_t*)&ZGP_CTXC().match_info->match_tbl[match_dev_tbl_idx];
}

static zb_bool_t find_cluster_record_and_endpoint_description_for_command(zgps_dev_match_rec_t *match_rec,
                                                                          zb_uint8_t cluster_id_present,
                                                                          zb_uint16_t cluster_id,
                                                                          zb_uint8_t zgpd_cmd_id,
                                                                          zgps_dev_cluster_rec_t **cluster_rec,
                                                                          zb_af_endpoint_desc_t **ep_desc)
{
  zb_uint_t               i;
  zgps_dev_cluster_rec_t* cluster_rec_ret = NULL;
  zb_af_endpoint_desc_t*  ep_desc_ret = NULL;

  TRACE_MSG(TRACE_ZGP3, "find_cluster_record_and_endpoint_description_for_command match_rec %p cluster_id_present %hd cluster_id 0x%x zgpd_cmd_id 0x%hx",
            (FMT__P_H_D_H, match_rec, cluster_id_present, cluster_id, zgpd_cmd_id));

  for (i = 0; i < ZB_ZGP_TBL_MAX_CLUSTERS; i++)
  {
    zb_uint_t        j = 0;
    zb_uint8_t cluster_idx = match_rec->clusters[i];

    TRACE_MSG(TRACE_ZGP3, "cluster_idx %hd", (FMT__H, cluster_idx));
    cluster_rec_ret = NULL;
    if (cluster_idx < ZGP_CTXC().match_info->clusters_tbl_size)
    {
      cluster_rec_ret = (zgps_dev_cluster_rec_t*)&ZGP_CTXC().match_info->clusters_tbl[cluster_idx];
    }

    if (!cluster_rec_ret ||
        (cluster_id_present && cluster_rec_ret->cluster_id != cluster_id))
    {
      TRACE_MSG(TRACE_ZGP3, "cluster_rec_ret %p", (FMT__P, cluster_rec_ret));
      continue;
    }

    while ((cluster_rec_ret->cmd_ids[j] != 0) && (j < ZB_ZGP_MATCH_TBL_MAX_CMDS_FOR_MATCH))
    {
      TRACE_MSG(TRACE_ZGP3, "cmd_ids[%d] 0x%hx", (FMT__D_D, j, cluster_rec_ret->cmd_ids[j]));
      if (cluster_rec_ret->cmd_ids[j] == zgpd_cmd_id)
      {
        ep_desc_ret = get_endpoint_by_cluster_with_role(cluster_rec_ret->cluster_id,
                                                        GET_CLUSTER_ROLE(cluster_rec_ret));

        TRACE_MSG(TRACE_ZGP3, "ep_desc_ret %p", (FMT__P, ep_desc_ret));
        if (ep_desc_ret)
        {
          *cluster_rec = cluster_rec_ret;
          *ep_desc = ep_desc_ret;
          return ZB_TRUE;
        }
      }
      j++;
    }
  }
  return ZB_FALSE;
}

/**
 * @brief Construct payload for ZCL packet
 *
 * @param param [in] Reference to buffer with GPDF packet.
 *                   Buffer parameter contains filled @ref zb_gpdf_info_t struct
 * @param cluster_rec [in] Pointer to cluster record info
 */
static void zb_zgp_create_zcl_payload(zb_uint8_t param, const zgps_dev_cluster_rec_t* cluster_rec)
{
  zb_gpdf_info_t *gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);
  /* hacky hack: need 4 bytes for level control while we have ZB_ZGP_TRANSL_CMD_PLD_MAX_SIZE 3 in every zb_vendor! */
  zb_uint8_t      payload[ZB_ZGP_TRANSL_CMD_PLD_MAX_SIZE+1];
  zb_uint8_t      payload_len = 0;

  TRACE_MSG(TRACE_ZGP3, "zb_zgp_create_zcl_payload param %hd", (FMT__H, param));

  switch (cluster_rec->cluster_id)
  {
    case ZB_ZCL_CLUSTER_ID_SCENES:
    {
      if (gpdf_info->zgpd_cmd_id >= ZB_GPDF_CMD_RECALL_SCENE0 &&
          gpdf_info->zgpd_cmd_id <= ZB_GPDF_CMD_RECALL_SCENE7)
      {
        zb_zcl_scenes_recall_scene_req_t *zcl_pld;

        ZB_ASSERT_COMPILE_TIME(ZB_ZGP_TRANSL_CMD_PLD_MAX_SIZE >= 3);
        payload_len = 0x03;
        zcl_pld = (zb_zcl_scenes_recall_scene_req_t *)payload;
        zcl_pld->group_id = 0;
        zcl_pld->scene_id = gpdf_info->zgpd_cmd_id - ZB_GPDF_CMD_RECALL_SCENE0;
      }
      break;
    }

#if ZB_ZGP_TRANSL_CMD_PLD_MAX_SIZE + 1 >= 4
    case ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL:
    {
      if (gpdf_info->zgpd_cmd_id == ZB_GPDF_CMD_MOVE_UP ||
          gpdf_info->zgpd_cmd_id == ZB_GPDF_CMD_MOVE_DOWN)
      {
        payload_len = 4;
        ZB_BZERO(payload, 4);

        if (gpdf_info->zgpd_cmd_id == ZB_GPDF_CMD_MOVE_DOWN)
        {
          payload[0] = 0x1; /* options: move down */
        }

        payload[1] = 0xFF; /* default rate */
      }
      else if (gpdf_info->zgpd_cmd_id == ZB_GPDF_CMD_LC_STOP)
      {
        payload_len = 2;
        ZB_BZERO(payload, 2);
      }
      break;
    }
#endif
  }

  if (payload_len)
  {
    TRACE_MSG(TRACE_ZGP1, "zcl payload, cluster_id %hd, length %hd",
			(FMT__H_H, cluster_rec->cluster_id, payload_len));
    ZB_MEMCPY(zb_buf_initial_alloc(param, payload_len), payload, payload_len);
  }
}


static zb_ret_t zb_zgp_get_cluster_id(zb_uint8_t param,
                                      zb_uint8_t cmd_id,
                                      zb_uint8_t *cluster_id_present_p,
                                      zb_uint16_t *cluster_id_p)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t buf_len = zb_buf_len(param);
  *cluster_id_present_p = ZB_FALSE;

  switch (cmd_id)
  {
    case ZB_GPDF_CMD_ATTR_REPORT:
      if (buf_len >= 2) /* cluster id */
      {
        ZB_ASSERT(zb_buf_len(param) >= 2);
        ZB_LETOH16(cluster_id_p, zb_buf_begin(param));
        zb_buf_cut_left(param, 2);
        *cluster_id_present_p = ZB_TRUE;
      }
      else
      {
        TRACE_MSG(TRACE_ERROR, "Invalid buffer length %hd", (FMT__H, buf_len));
        ret = RET_ERROR;
      }
    break;

    case ZB_GPDF_CMD_MANUF_SPEC_ATTR_REPORT:
      if (buf_len >= 4) /* manuf_id + cluster_id*/
      {
        ZB_LETOH16(cluster_id_p, (zb_uint8_t*)zb_buf_begin(param) + 2);
        *cluster_id_present_p = ZB_TRUE;
      }
      else
      {
        TRACE_MSG(TRACE_ERROR, "Invalid buffer length %hd", (FMT__H, buf_len));
        ret = RET_ERROR;
      }
    break;

    default:
    // Do nothing
    break;
  }

  return ret;
}

static zb_ret_t zb_zgp_dimm_get_button_switch_cmd(zb_uint8_t zgpd_cmd_id, zb_uint8_t bit_num, zb_uint8_t num_of_contacs, zb_uint8_t *cmd)
{
  zb_ret_t ret = RET_ERROR;

  TRACE_MSG(TRACE_ZGP4, ">> zb_zgp_dimm_get_button_switch_cmd", (FMT__0));

  switch (num_of_contacs)
  {
    case 1:
      if ((zgpd_cmd_id == ZB_GPDF_CMD_8BIT_VECTOR_PRESS) && (bit_num == 0))
      {
        *cmd = ZB_GPDF_CMD_TOGGLE;
        ret = RET_OK;
      }
    break;

    case 2:
      if (zgpd_cmd_id == ZB_GPDF_CMD_8BIT_VECTOR_PRESS)
      {
        if (bit_num == 0)
        {
          *cmd = ZB_GPDF_CMD_OFF;
          ret = RET_OK;
        }
        else if (bit_num == 1)
        {
          *cmd = ZB_GPDF_CMD_ON;
          ret = RET_OK;
        }
      }
    break;

    case 3:
      if (zgpd_cmd_id == ZB_GPDF_CMD_8BIT_VECTOR_PRESS)
      {
        if (bit_num == 0)
        {
          *cmd = ZB_GPDF_CMD_MOVE_DOWN;
          ret = RET_OK;
        }
        else if (bit_num == 1)
        {
          *cmd = ZB_GPDF_CMD_MOVE_UP;
          ret = RET_OK;
        }
        else if (bit_num == 2)
        {
          *cmd = ZB_GPDF_CMD_TOGGLE;
          ret = RET_OK;
        }
      }
      else if (zgpd_cmd_id == ZB_GPDF_CMD_8BIT_VECTOR_RELEASE)
      {
        if ((bit_num == 0) || (bit_num == 1))
        {
          *cmd = ZB_GPDF_CMD_LC_STOP;
          ret = RET_OK;
        }
      }
    break;

    case 4:
      if (zgpd_cmd_id == ZB_GPDF_CMD_8BIT_VECTOR_PRESS)
      {
        if (bit_num == 0)
        {
          *cmd = ZB_GPDF_CMD_OFF;
          ret = RET_OK;
        }
        else if (bit_num == 1)
        {
          *cmd = ZB_GPDF_CMD_ON;
          ret = RET_OK;
        }
        else if (bit_num == 2)
        {
          *cmd = ZB_GPDF_CMD_MOVE_DOWN;
          ret = RET_OK;
        }
        else if (bit_num == 3)
        {
          *cmd = ZB_GPDF_CMD_MOVE_UP;
          ret = RET_OK;
        }
      }
      else if (zgpd_cmd_id == ZB_GPDF_CMD_8BIT_VECTOR_RELEASE)
      {
        if ((bit_num == 2) || (bit_num == 3))
        {
          *cmd = ZB_GPDF_CMD_LC_STOP;
          ret = RET_OK;
        }
      }
    break;
  }

  TRACE_MSG(TRACE_ZGP4, "<< zb_zgp_dimm_get_button_switch_cmd, ret %hd", (FMT__H, ret));
  return ret;
}

static zb_ret_t zb_zgp_blinds_get_button_switch_cmd(zb_uint8_t zgpd_cmd_id, zb_uint8_t bit_num, zb_uint8_t num_of_contacs, zb_uint8_t *cmd)
{
  zb_ret_t ret = RET_ERROR;

  TRACE_MSG(TRACE_ZGP4, ">> zb_zgp_blinds_get_button_switch_cmd", (FMT__0));

  switch (num_of_contacs)
  {
    case 2:
      if (zgpd_cmd_id == ZB_GPDF_CMD_8BIT_VECTOR_PRESS)
      {
        if (bit_num == 0)
        {
          *cmd = ZB_GPDF_CMD_MOVE_DOWN;
          ret = RET_OK;
        }
        else if (bit_num == 1)
        {
          *cmd = ZB_GPDF_CMD_MOVE_UP;
          ret = RET_OK;
        }
      }
      else if (zgpd_cmd_id == ZB_GPDF_CMD_8BIT_VECTOR_RELEASE)
      {
        if ((bit_num == 0) || (bit_num == 1))
        {
          *cmd = ZB_GPDF_CMD_LC_STOP;
          ret = RET_OK;
        }
      }
    break;

    case 3:
      if (zgpd_cmd_id == ZB_GPDF_CMD_8BIT_VECTOR_PRESS)
      {
        if (bit_num == 0)
        {
          *cmd = ZB_GPDF_CMD_MOVE_DOWN;
          ret = RET_OK;
        }
        else if (bit_num == 1)
        {
          *cmd = ZB_GPDF_CMD_MOVE_UP;
          ret = RET_OK;
        }
        else if (bit_num == 2)
        {
          *cmd = ZB_GPDF_CMD_LC_STOP;
          ret = RET_OK;
        }
      }
    break;
  }

  TRACE_MSG(TRACE_ZGP4, "<< zb_zgp_blinds_get_button_switch_cmd, ret %hd", (FMT__H, ret));
  return ret;
}

static zb_ret_t zb_zgp_dimm_get_rocker_switch_cmd(zb_uint8_t zgpd_cmd_id, zb_uint8_t bit_num, zb_uint8_t num_of_contacs, zb_uint8_t *cmd)
{
  zb_ret_t ret = RET_ERROR;

  TRACE_MSG(TRACE_ZGP4, ">> zb_zgp_dimm_get_rocker_switch_cmd", (FMT__0));

  switch (num_of_contacs)
  {
    case 1:
      ret = zb_zgp_dimm_get_button_switch_cmd(zgpd_cmd_id, bit_num, 2, cmd);
    break;

    case 2:
      ret = zb_zgp_dimm_get_button_switch_cmd(zgpd_cmd_id, bit_num, 4, cmd);
    break;
  }

  TRACE_MSG(TRACE_ZGP4, "<< zb_zgp_dimm_get_rocker_switch_cmd, ret %hd", (FMT__H, ret));
  return ret;
}

static zb_ret_t zb_zgp_blinds_get_rocker_switch_cmd(zb_uint8_t zgpd_cmd_id, zb_uint8_t bit_num, zb_uint8_t num_of_contacs, zb_uint8_t *cmd)
{
  zb_ret_t ret = RET_ERROR;

  TRACE_MSG(TRACE_ZGP4, ">> zb_zgp_blinds_get_rocker_switch_cmd", (FMT__0));

  switch (num_of_contacs)
  {
    case 1:
      ret = zb_zgp_blinds_get_button_switch_cmd(zgpd_cmd_id, bit_num, 2, cmd);
    break;
  }

  TRACE_MSG(TRACE_ZGP4, "<< zb_zgp_blinds_get_rocker_switch_cmd, ret %hd", (FMT__H, ret));
  return ret;
}


ZB_WEAK_PRE zb_ret_t ZB_WEAK zb_zgp_convert_8bit_vector(zb_uint8_t vector_8bit_cmd_id,      /* press or release cmd */
                                                        zb_uint8_t switch_type,             /* see zb_zgpd_switch_type_e */
                                                        zb_uint8_t num_of_contacs,
                                                        zb_uint8_t contact_status,
                                                        zb_uint8_t *zgp_cmd_out)
{
  zb_ret_t ret = RET_ERROR;

  TRACE_MSG(TRACE_ZGP4, ">> zb_zgp_convert_8bit_vector", (FMT__0));

  if (contact_status & (0xFF >> (8 - num_of_contacs)))
  {
    zb_uint8_t i;
    zb_uint8_t j;
    zb_uint16_t dev_id = 0xffff;

    for (j = 0; j < ZB_ZDO_SIMPLE_DESC_NUMBER(); j++)
    {
      if (/* default dimm light */
          ZB_ZDO_SIMPLE_DESC_LIST()[j]->app_device_id == ZB_HA_ON_OFF_OUTPUT_DEVICE_ID ||
          ZB_ZDO_SIMPLE_DESC_LIST()[j]->app_device_id == ZB_HA_LEVEL_CONTROLLABLE_OUTPUT_DEVICE_ID ||
          ZB_ZDO_SIMPLE_DESC_LIST()[j]->app_device_id == ZB_HA_ON_OFF_LIGHT_DEVICE_ID ||
          ZB_ZDO_SIMPLE_DESC_LIST()[j]->app_device_id == ZB_HA_DIMMABLE_LIGHT_DEVICE_ID ||
          ZB_ZDO_SIMPLE_DESC_LIST()[j]->app_device_id == ZB_HA_COLOR_DIMMABLE_LIGHT_DEVICE_ID ||
          /* default blinds */
          ZB_ZDO_SIMPLE_DESC_LIST()[j]->app_device_id == ZB_HA_WINDOW_COVERING_CONTROLLER_DEVICE_ID ||
          ZB_ZDO_SIMPLE_DESC_LIST()[j]->app_device_id == ZB_HA_SHADE_CONTROLLER_DEVICE_ID
      )
      {
        dev_id = ZB_ZDO_SIMPLE_DESC_LIST()[j]->app_device_id;
        break;
      }
    }

    if (dev_id != 0xffff)
    {
      for (i = 0; i < ZB_ZGP_MAX_CONTACT_STATUS_BITS; i++) /* search position of least significant bit in contact_status */
      {
        if (contact_status & (1 << i))
        {
          if (switch_type == ZB_GPD_SWITCH_TYPE_BUTTON) /* button switch */
          {
            /* sink is blinds control */
            if (dev_id == ZB_HA_WINDOW_COVERING_CONTROLLER_DEVICE_ID ||
                dev_id == ZB_HA_SHADE_CONTROLLER_DEVICE_ID)
            {
              ret = zb_zgp_blinds_get_button_switch_cmd(vector_8bit_cmd_id, i, num_of_contacs, zgp_cmd_out);
            }
            else /* sink is dimm light */
            {
              ret = zb_zgp_dimm_get_button_switch_cmd(vector_8bit_cmd_id, i, num_of_contacs, zgp_cmd_out);
            }
          }
          else if (switch_type == ZB_GPD_SWITCH_TYPE_ROCKER) /* rocker switch */
          {
            /* sink is blinds control */
            if (dev_id == ZB_HA_WINDOW_COVERING_CONTROLLER_DEVICE_ID ||
                dev_id == ZB_HA_SHADE_CONTROLLER_DEVICE_ID)
            {
              ret = zb_zgp_blinds_get_rocker_switch_cmd(vector_8bit_cmd_id, i, num_of_contacs, zgp_cmd_out);
            }
            else /* sink is dimm light */
            {
              ret = zb_zgp_dimm_get_rocker_switch_cmd(vector_8bit_cmd_id, i, num_of_contacs, zgp_cmd_out);
            }
          }
          break;
        }
      }
    }
  }

  TRACE_MSG(TRACE_ZGP4, "<< zb_zgp_convert_8bit_vector, ret %hd", (FMT__H, ret));
  return ret;
}

static zb_ret_t zb_zgp_8bit_vector_cmd_handler(zb_uint8_t param, zb_gpdf_info_t *gpdf_info)
{
  zgp_runtime_app_tbl_ent_t *app_ent = zb_zgp_get_app_tbl_ent_by_id(&gpdf_info->zgpd_id);
  zb_ret_t ret = RET_ERROR;

  TRACE_MSG(TRACE_ZGP3, ">> zb_zgp_8bit_vector_cmd_handler, param %hd", (FMT__H, param));

  if ((zb_buf_len(param) >= 1) && app_ent)
  {
    zb_gpdf_comm_switch_gen_cfg_t switch_info = app_ent->base.info.switch_info_configuration;
    /* get payload */
    zb_uint8_t contact_status = *((zb_uint8_t *)zb_buf_begin(param));
    zb_buf_cut_left(param, 1);

    ret = zb_zgp_convert_8bit_vector(gpdf_info->zgpd_cmd_id, switch_info.switch_type, switch_info.num_of_contacs, contact_status, &gpdf_info->zgpd_cmd_id);

    TRACE_MSG(TRACE_ZGP1, "zb_zgp_8bit_vector_cmd_handler: ZGP_CMD_ID 0x%x", (FMT__H, gpdf_info->zgpd_cmd_id));
  }

  TRACE_MSG(TRACE_ZGP3, "<< zb_zgp_8bit_vector_cmd_handler, ret %hd", (FMT__H, ret));
  return ret;
}

static zb_bool_t find_cluster_record_and_endpoint_description_for_tunneling(zgps_dev_match_rec_t *match_rec,
                                                                            zb_uint16_t cluster_id,
                                                                            zgps_dev_cluster_rec_t **cluster_rec,
                                                                            zb_af_endpoint_desc_t **ep_desc)
{
  zb_uint_t               i;
  zgps_dev_cluster_rec_t* cluster_rec_ret = NULL;
  zb_af_endpoint_desc_t*  ep_desc_ret = NULL;

  TRACE_MSG(TRACE_ZGP3, "find_cluster_record_and_endpoint_description_for_tunneling match_rec %p cluster_id 0x%x",
            (FMT__P_D, match_rec, cluster_id));

  for (i = 0; i < ZB_ZGP_TBL_MAX_CLUSTERS; i++)
  {
    zb_uint8_t cluster_idx = match_rec->clusters[i];

    TRACE_MSG(TRACE_ZGP3, "cluster_idx %hd", (FMT__H, cluster_idx));
    cluster_rec_ret = NULL;
    if (cluster_idx < ZGP_CTXC().match_info->clusters_tbl_size)
    {
      cluster_rec_ret = (zgps_dev_cluster_rec_t*)&ZGP_CTXC().match_info->clusters_tbl[cluster_idx];
    }

    if (cluster_rec_ret && cluster_rec_ret->cluster_id == cluster_id)
    {
      ep_desc_ret = get_endpoint_by_cluster_with_role(cluster_rec_ret->cluster_id, GET_CLUSTER_ROLE(cluster_rec_ret));

      TRACE_MSG(TRACE_ZGP3, "ep_desc_ret %p", (FMT__P, ep_desc_ret));
      if (ep_desc_ret)
      {
        *cluster_rec = cluster_rec_ret;
        *ep_desc     = ep_desc_ret;
        return ZB_TRUE;
      }
    }
  }
  return ZB_FALSE;
}

static void zb_zgp_process_tunneling_cmd(zb_uint8_t param)
{
  zb_gpdf_info_t gpdf_info = *((zb_gpdf_info_t *) ZB_BUF_GET_PARAM(param, zb_gpdf_info_t));
  zb_zgp_sink_tbl_ent_t sink_tbl_ent;
  zgps_dev_match_rec_t  *match_rec;

  zb_uint8_t  options;
  zb_uint16_t manuf_id = (zb_uint16_t)~0u;
  zb_uint16_t cluster_id;
  zb_uint8_t  zigbee_cmd_id;
  zb_uint8_t  payload_len;

  TRACE_MSG(TRACE_ZGP3, ">> zb_zgp_process_tunneling_cmd, param %hd", (FMT__H, param));

  /* Read sink entry */
  if (zgp_sink_table_read(&gpdf_info.zgpd_id, &sink_tbl_ent) != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "No entry in sink table", (FMT__0));
    zb_buf_free(param);
    param = 0;
  }

  /* Get match record */
  if (param)
  {
    match_rec = get_dev_match_record(sink_tbl_ent.u.sink.match_dev_tbl_idx);
    if (match_rec == NULL)
    {
      TRACE_MSG(TRACE_ERROR, "Error: match record does not exist", (FMT__0));
      zb_buf_free(param);
      param = 0;
    }
  }

  /* Parse tunneling hdr */
  if (param)
  {
    zb_uint8_t offset = 0;
    zb_uint8_t *ptr = zb_buf_begin(param);
    zb_uint8_t buf_len = zb_buf_len(param);
    zb_uint8_t require_buf_len = sizeof(options) + sizeof(cluster_id) + sizeof(zigbee_cmd_id) + sizeof(payload_len);

    if (buf_len)
    {
      options = *ptr;
      offset += 1;

      if (ZGP_TUNNELING_OPT_GET_MANUF_PRESENT(options))
      {
        require_buf_len += sizeof(manuf_id);
      }
    }

    if (buf_len >= require_buf_len)
    {
      if (ZGP_TUNNELING_OPT_GET_MANUF_PRESENT(options))
      {
        ZB_LETOH16(&manuf_id, (ptr + offset));
        offset += 2;
      }

      ZB_LETOH16(&cluster_id, (ptr + offset));
      offset += 2;

      zigbee_cmd_id = *(ptr + offset);
      offset += 1;

      payload_len = *(ptr + offset);
      offset += 1;

      /* Cut parsed values */
      zb_buf_cut_left(param, offset);
    }
    else
    {
      TRACE_MSG(TRACE_ERROR, "Tunneling cmd parsing fail (broken frame)", (FMT__0));
      zb_buf_free(param);
      param = 0;
    }

    /* check payload length */
    if (param)
    {
      if (payload_len != zb_buf_len(param))
      {
        TRACE_MSG(TRACE_ERROR, "Tunneling cmd parsing fail (invalid payload length)", (FMT__0));
        zb_buf_free(param);
        param = 0;
      }
    }
  }

  /* Create a parsed zcl header and pass to ZCL*/
  if (param)
  {
    zgps_dev_cluster_rec_t *cluster_rec = NULL;
    zb_af_endpoint_desc_t  *ep_desc = NULL;

    if (find_cluster_record_and_endpoint_description_for_tunneling(
                 match_rec,
                 cluster_id,
                 &cluster_rec,
                 &ep_desc))
    {
      zb_zcl_parsed_hdr_t   *parsed_hdr = ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);

      zb_zgpd_addr_t        zgpd_addr;
      zb_zcl_addr_t         *zcl_src_addr = &ZB_ZCL_PARSED_HDR_SHORT_DATA(parsed_hdr).source;

      if (sink_tbl_ent.zgpd_id.src_id == ZB_ZGP_SRC_ID_ALL)
      {
        ZB_MEMCPY(&zgpd_addr, &gpdf_info.zgpd_id.addr, sizeof(zb_zgpd_addr_t));
      }
      else if (ZGP_TBL_GET_APP_ID(&sink_tbl_ent) == ZB_ZGP_APP_ID_0000)
      {
        zgpd_addr.src_id = sink_tbl_ent.zgpd_id.src_id;
      }
      else
      {
        ZB_MEMCPY(&zgpd_addr, sink_tbl_ent.zgpd_id.ieee_addr, sizeof(zb_zgpd_addr_t));
      }

      ZB_BZERO(parsed_hdr, sizeof(zb_zcl_parsed_hdr_t));
    #ifdef ZB_ENABLE_HA
      parsed_hdr->addr_data.common_data.dst_endpoint = ep_desc->ep_id;
    #endif
      parsed_hdr->cluster_id = cluster_rec->cluster_id;
      parsed_hdr->profile_id = ep_desc->profile_id;
      parsed_hdr->cmd_id = zigbee_cmd_id;
      parsed_hdr->is_manuf_specific = ZB_FALSE;
      parsed_hdr->disable_default_response = ZB_TRUE;
      parsed_hdr->seq_number = gpdf_info.mac_seq_num;
      parsed_hdr->cmd_direction = ZGP_TUNNELING_OPT_GET_DIRECTION(options);
      parsed_hdr->is_common_command = (ZGP_TUNNELING_OPT_GET_FRAME_TYPE(options) == ZB_ZCL_FRAME_TYPE_CLUSTER_SPECIFIC ) ? ZB_FALSE : ZB_TRUE;

      if (ZGP_TBL_GET_APP_ID(&sink_tbl_ent) == ZB_ZGP_APP_ID_0000)
      {
        zcl_src_addr->addr_type = ZB_ZCL_ADDR_TYPE_SRC_ID_GPD;
        zcl_src_addr->u.src_id = zgpd_addr.src_id;
      }
      else
      {
        zcl_src_addr->addr_type = ZB_ZCL_ADDR_TYPE_IEEE_GPD;
        parsed_hdr->addr_data.common_data.src_endpoint = sink_tbl_ent.endpoint;
        ZB_IEEE_ADDR_COPY(zcl_src_addr->u.ieee_addr, zgpd_addr.ieee_addr);
      }

      if (ZGP_TUNNELING_OPT_GET_MANUF_PRESENT(options)
          && manuf_id != (zb_uint16_t)~0)
      {
        parsed_hdr->is_manuf_specific = ZB_TRUE;
        parsed_hdr->manuf_specific = manuf_id;
      }

      ZB_SCHEDULE_CALLBACK(zb_zcl_process_parsed_zcl_cmd, param);
    }
    else
    {
      TRACE_MSG(TRACE_ZGP1, "Error: cluster record or endpoint description does not exist", (FMT__0));
    }
  }

  TRACE_MSG(TRACE_ZGP3, "<< zb_zgp_process_tunneling_cmd", (FMT__0));
}

static void zb_zgp_from_gpdf_to_zcl_default(zb_uint8_t param)
{
  zb_gpdf_info_t        *gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);
  zb_zgp_sink_tbl_ent_t  sink_tbl_ent;
  zgps_dev_match_rec_t  *match_rec;

  zb_uint16_t            cluster_id = 0xffff;
  zb_uint8_t             cluster_id_present;
  zgps_dev_cluster_rec_t *cluster_rec = NULL;
  zb_af_endpoint_desc_t  *ep_desc = NULL;

  TRACE_MSG(TRACE_ZGP1, ">> zb_zgp_from_gpdf_to_zcl_default, param %hd len %hd cmd %hx",
              (FMT__H_H_H, param, zb_buf_len(param), gpdf_info->zgpd_cmd_id));


  /* Read sink entry */
  if (zgp_sink_table_read(&gpdf_info->zgpd_id, &sink_tbl_ent) != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "No entry in sink table", (FMT__0));
    zb_buf_free(param);
    param = 0;
  }

  /* Get match record */
  if (param)
  {
    match_rec = get_dev_match_record(sink_tbl_ent.u.sink.match_dev_tbl_idx);
    if (match_rec == NULL)
    {
      TRACE_MSG(TRACE_ERROR, "Error: match record does not exist", (FMT__0));
      zb_buf_free(param);
      param = 0;
    }
  }

   /* Get cluster id if present */
  if (param)
  {
    if (zb_zgp_get_cluster_id(param, gpdf_info->zgpd_cmd_id, &cluster_id_present, &cluster_id) != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "Broken frame, drop", (FMT__0));
      zb_buf_free(param);
      param = 0;
    }
  }

  if (param)
  {
    TRACE_MSG(TRACE_ZGP3, "match_dev_tbl_idx %hd", (FMT__H, sink_tbl_ent.u.sink.match_dev_tbl_idx));

    if (find_cluster_record_and_endpoint_description_for_command(
             match_rec,
             cluster_id_present,
             cluster_id,
             gpdf_info->zgpd_cmd_id,
             &cluster_rec,
             &ep_desc))
    {
      zb_zgpd_addr_t       zgpd_addr;
      zb_zcl_parsed_hdr_t *parsed_hdr;

      zb_zgp_create_zcl_payload(param, cluster_rec);
      parsed_hdr = ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);

      if (sink_tbl_ent.zgpd_id.src_id == ZB_ZGP_SRC_ID_ALL)
      {
        ZB_MEMCPY(&zgpd_addr, &gpdf_info->zgpd_id.addr, sizeof(zb_zgpd_addr_t));
      }
      else if (ZGP_TBL_GET_APP_ID(&sink_tbl_ent) == ZB_ZGP_APP_ID_0000)
      {
        zgpd_addr.src_id = sink_tbl_ent.zgpd_id.src_id;
      }
      else
      {
        ZB_MEMCPY(&zgpd_addr, sink_tbl_ent.zgpd_id.ieee_addr, sizeof(zb_zgpd_addr_t));
      }

      if (RET_OK == zgps_create_zcl_parsed_hdr_use_ste(param,
                                            &sink_tbl_ent,
                                            cluster_rec,
                                            ep_desc,
                                            gpdf_info->zgpd_cmd_id,
                                            parsed_hdr,
                                            &zgpd_addr,
                                            gpdf_info->mac_seq_num))
      {
        ZB_SCHEDULE_CALLBACK(zb_zcl_process_parsed_zcl_cmd, param);
        param = 0;
      }
    }
    else
    {
      TRACE_MSG(TRACE_ZGP1, "Error: cluster record or endpoint description does not exist", (FMT__0));
    }
  }

  if (param)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZGP1, "<< zb_zgp_from_gpdf_to_zcl_default", (FMT__0));
}

/**
 * @brief Convert GPDF packet to ZCL packet (packets)
 *
 * @param param [in] Reference to buffer with GPDF packet.
 *                   Buffer parameter contains filled @ref zb_gpdf_info_t struct
 */
static void zb_zgp_from_gpdf_to_zcl(zb_uint8_t param)
{
  zb_gpdf_info_t        *gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);
  zb_zgp_sink_tbl_ent_t  sink_tbl_ent;

  TRACE_MSG(TRACE_ZGP1, ">> zb_zgp_from_gpdf_to_zcl, param %hd len %hd cmd %hx",
            (FMT__H_H_H, param, zb_buf_len(param), gpdf_info->zgpd_cmd_id));

  /* Precheck sink entry */
  if (zgp_sink_table_read(&gpdf_info->zgpd_id, &sink_tbl_ent) != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "No entry in sink table", (FMT__0));
    zb_buf_free(param);
  }
  else
  {
    switch (gpdf_info->zgpd_cmd_id)
    {
      /* Checking commands requiring special handling */
      case ZB_GPDF_CMD_MULTI_CLUSTER_ATTR_REPORT:
      case ZB_GPDF_CMD_MANUF_SPEC_MULTI_CLUSTER_ATTR_REPORT:
      {
        ZB_SCHEDULE_CALLBACK2(zb_zgp_extract_report_from_multi_report, 0, param); /* First try without extra bufer allocate */
      }
      break;

      case ZB_GPDF_CMD_COMPACT_ATTR_REPORTING:
      {
        ZB_SCHEDULE_CALLBACK(zb_zgp_extract_report_from_compact_report, param);
      }
      break;

      case ZB_GPDF_CMD_ZCL_TUNNELING_FROM_ZGPD:
      {
        ZB_SCHEDULE_CALLBACK(zb_zgp_process_tunneling_cmd, param);
      }
      break;

      case ZB_GPDF_CMD_8BIT_VECTOR_PRESS:
      case ZB_GPDF_CMD_8BIT_VECTOR_RELEASE:
      {
        if (zb_zgp_8bit_vector_cmd_handler(param, gpdf_info) != RET_OK)
        {
          TRACE_MSG(TRACE_ERROR, "Can't convert 8bit_vector command, drop", (FMT__0));
          zb_buf_free(param);
        }
        else
        {
          /* Call default func */
          ZB_SCHEDULE_CALLBACK(zb_zgp_from_gpdf_to_zcl_default, param);
        }
      }
      break;

      /* Call default function for regular commands */
      default:
      {
        ZB_SCHEDULE_CALLBACK(zb_zgp_from_gpdf_to_zcl_default, param);
      }
    }
  }

  TRACE_MSG(TRACE_ZGP1, "<< zb_zgp_from_gpdf_to_zcl", (FMT__0));
}

/**
 * @brief Remove all the information about ZGPD from stack
 *
 * @param buf_ref  reference to the free buffer
 * @param zgpd_id  identifier of ZGPD to be removed
 *
 * @note It is safe to free or overwrite memory pointed by zgpd_id
 *       after call
 */
void zb_zgps_delete_zgpd(zb_uint8_t buf_ref, zb_zgpd_id_t *zgpd_id)
{
  TRACE_MSG(TRACE_ZGP2, ">> zb_zgps_delete_zgpd, zgpd_id %p, buf_ref %hd",
      (FMT__P_H, zgpd_id, buf_ref));

  zgp_sink_table_del(zgpd_id);
#ifdef ZB_ENABLE_ZGP_DIRECT
  zgp_clean_zgpd_info_from_queue(buf_ref, zgpd_id, ZB_ZGP_HANDLE_REMOVE_BY_USER_REQ);
#endif  /* ZB_ENABLE_ZGP_DIRECT */

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_delete_zgpd", (FMT__0));
}

#ifdef ZB_ENABLE_ZGP_DIRECT
static void zb_zgp_tx_q_clean()
{
  TRACE_MSG(TRACE_ZGP2, ">> zb_zgp_tx_q_clean", (FMT__0));

  ZB_BZERO(&ZGP_CTXC().tx_queue, sizeof(ZGP_CTXC().tx_queue));
  ZB_SCHEDULE_ALARM_CANCEL(zb_zgp_tx_q_entry_expired, ZB_ALARM_ALL_CB);

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgp_tx_q_clean", (FMT__0));
}
#endif  /* ZB_ENABLE_ZGP_DIRECT */

/**
 * @brief Remove all the information about all ZGPD from stack
 */
void zb_zgps_delete_all_zgpd()
{
  TRACE_MSG(TRACE_ZGP2, ">> zb_zgps_delete_all_zgpd", (FMT__0));

  /* TODO: Place me here! */

  /* Clean proxy, sink, translation tables and TX queue */
  zgp_tbl_clear();
#ifdef ZB_ENABLE_ZGP_DIRECT
  zb_zgp_tx_q_clean();
#endif  /* ZB_ENABLE_ZGP_DIRECT */
  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_delete_all_zgpd", (FMT__0));
}

void zb_zgps_get_diag_data(zb_zgpd_id_t *zgpd_id, zb_uint8_t *lqi, zb_int8_t *rssi)
{
  TRACE_MSG(TRACE_ZGP2, ">> zb_zgps_get_diag_data zgpd_id %p",
      (FMT__P, zgpd_id));

  zgp_sink_get_lqi_rssi(zgpd_id, lqi, rssi);

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_get_diag_data zgpd_id %p lqi %u rssi %d",
            (FMT__H_H_P, zgpd_id, *lqi, *rssi));
}

static void zgp_sink_handle_commissioning_gpdf(zb_uint8_t param)
{
  zb_gpdf_info_t *gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);

  TRACE_MSG(TRACE_ZGP2, "zgp_sink_handle_commissioning_gpdf param %hd state %d", (FMT__H_D, param, ZGP_CTXC().comm_data.state));

  TRACE_MSG(TRACE_ZGP2, "cmd: 0x%x", (FMT__H, gpdf_info->zgpd_cmd_id));

  switch (gpdf_info->zgpd_cmd_id)
  {
    case ZB_GPDF_CMD_CHANNEL_REQUEST:
    {
        ZB_SCHEDULE_CALLBACK(zb_zgps_process_channel_req, param);
        param = 0;
      }
    break;

    case ZB_GPDF_CMD_COMMISSIONING:
    {
      TRACE_MSG(TRACE_ZGP2, "comm state %d rx after tx %d",
                (FMT__D_D, ZGP_CTXC().comm_data.state, ZB_GPDF_EXT_NFC_GET_RX_AFTER_TX(gpdf_info->nwk_ext_frame_ctl)));

      if (zb_zgps_precheck_commissioning_payoad(param) == ZB_TRUE)
      {
        /* TODO: check proxy-based commissioning capability */
        if (/*zb_zgps_check_proxy_based_commissioning_cap() == ZB_TRUE*/1)
        {
          /* If GPD missed our commissioning reply and send commissioning
           * again, let's resend */
          if (ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_COMMISSIONING_REPLY_SENT
              && ZB_GPDF_EXT_NFC_GET_RX_AFTER_TX(gpdf_info->nwk_ext_frame_ctl))
          {
            TRACE_MSG(TRACE_ZGP2, "will resend commissioning reply", (FMT__0));
            zb_zgps_process_after_select_temp_master(param);
            param = 0;
          }
          else
          if (ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_IDLE ||
              ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_CHANNEL_CONFIG_SENT ||
              ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_COMISSIONING_REQ_COLLECT)
          {
            /* collect a couple of GP Commissioning Notification commands (from various GPPs) */
            if (zb_zgps_collect_gp_commissioning_notifications(param) == ZB_TRUE)
            {
              ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_COMISSIONING_REQ_COLLECT);
              param = 0;
            }
          }
        }
        else
        {
          zb_zgps_process_commissioning_cmd(param);
          param = 0;
        }
      }
    }
    break;

    case ZB_GPDF_CMD_APPLICATION_DESCR:
    {
      zb_zgps_process_app_descr_cmd(param);
      param = 0;
    }
    break;

    case ZB_GPDF_CMD_DECOMMISSIONING:
    {
      zb_zgps_process_decommissioning_cmd(param);
      param = 0;
    }
    break;

    case ZB_GPDF_CMD_SUCCESS:
    {
      if (ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_COMMISSIONING_REPLY_SENT)
      {
        *ZB_BUF_GET_PARAM(param, zb_uint8_t) = ZB_ZGP_COMMISSIONING_COMPLETED;
        TRACE_MSG(TRACE_ZGP3, "ZB_GPDF_CMD_SUCCESS, finalize commissioning", (FMT__0));
        zgp_sink_table_set_security_counter(&gpdf_info->zgpd_id, ZB_GPDF_INFO_GET_DUP_COUNTER(gpdf_info));
        ZB_SCHEDULE_CALLBACK(zb_zgps_finalize_commissioning, param);
        param = 0;
      }
      else
      {
        TRACE_MSG(TRACE_ZGP3, "Drop ZB_GPDF_CMD_SUCCESS, out-of order command, I have not send commissioning reply yet!", (FMT__0));
      }
    }
    break;

    default:
    {
      TRACE_MSG(TRACE_ZGP1, "Unknown command 0x%02x in commissioning mode",
                (FMT__H, gpdf_info->zgpd_cmd_id));
    }
  }

  if (param)
  {
    zb_buf_free(param);
  }
}

static void zgp_sink_handle_autocommissioning_gpdf(zb_uint8_t param)
{
  zb_gpdf_info_t *gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);
  zb_uint8_t      zgpd_device_id;

  TRACE_MSG(TRACE_ZGP2, "zgp_sink_handle_autocommissioning_gpdf %hd", (FMT__H, param));

  /* We have no ZGPD Device ID. All we have is GPD command id.
     But we need device id to setup matching.
     Try to guess using command id.
  */
  switch (gpdf_info->zgpd_cmd_id)
  {
    case ZB_GPDF_CMD_PRESS_1_OF_1:
    case ZB_GPDF_CMD_RELEASE_1_OF_1:
      zgpd_device_id = ZB_ZGP_SIMPLE_GEN_1_STATE_SWITCH_DEV_ID;
      break;

    case ZB_GPDF_CMD_RECALL_SCENE0:
    case ZB_GPDF_CMD_RECALL_SCENE1:
    case ZB_GPDF_CMD_RECALL_SCENE2:
    case ZB_GPDF_CMD_RECALL_SCENE3:
    case ZB_GPDF_CMD_RECALL_SCENE4:
    case ZB_GPDF_CMD_RECALL_SCENE5:
    case ZB_GPDF_CMD_RECALL_SCENE6:
    case ZB_GPDF_CMD_RECALL_SCENE7:

    case ZB_GPDF_CMD_OFF:
    case ZB_GPDF_CMD_ON:
    case ZB_GPDF_CMD_TOGGLE:
      zgpd_device_id = ZB_ZGP_ON_OFF_SWITCH_DEV_ID;
      break;

    case ZB_GPDF_CMD_MOVE_UP:
    case ZB_GPDF_CMD_MOVE_DOWN:
    case ZB_GPDF_CMD_STEP_UP:
    case ZB_GPDF_CMD_STEP_DOWN:
    case ZB_GPDF_CMD_MOVE_UP_W_ONOFF:
    case ZB_GPDF_CMD_MOVE_DOWN_W_ONOFF:
    case ZB_GPDF_CMD_STEP_UP_W_ONOFF:
    case ZB_GPDF_CMD_STEP_DOWN_W_ONOFF:
      zgpd_device_id = ZB_ZGP_LEVEL_CONTROL_SWITCH_DEV_ID;
      break;

    case ZB_GPDF_CMD_ATTR_REPORT:
    case ZB_GPDF_CMD_MANUF_SPEC_ATTR_REPORT:
    case ZB_GPDF_CMD_MULTI_CLUSTER_ATTR_REPORT:
    case ZB_GPDF_CMD_MANUF_SPEC_MULTI_CLUSTER_ATTR_REPORT:
    case ZB_GPDF_CMD_REQUEST_ATTRIBUTES:
    case ZB_GPDF_CMD_READ_ATTR_RESP:
      zgpd_device_id = ZB_ZGP_TEMPERATURE_SENSOR_DEV_ID;
      break;
    default:
      zgpd_device_id = 0xff;
      break;
  }

  if (zgpd_device_id != 0xff)
  {
    zb_gpdf_comm_params_t *comm_params = (zb_gpdf_comm_params_t *)zb_buf_get_tail(
      param, sizeof(zb_gpdf_comm_params_t) + sizeof(zb_gpdf_info_t));
    zb_uint8_t secur_lvl = ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl);

    TRACE_MSG(TRACE_ZGP2, "cmd %hd - suppose zgp_dev_id 0x%hx", (FMT__H_H, gpdf_info->zgpd_cmd_id, zgpd_device_id));
    TRACE_MSG(TRACE_ZGP2, "gpdf: ext = 0x%x", (FMT__H, gpdf_info->nwk_ext_frame_ctl));

    /* Note: do not touch zb_gpdf_info_t at tail! */
    ZB_BZERO(comm_params, sizeof(zb_gpdf_comm_params_t));
    comm_params->zgpd_device_id = zgpd_device_id;
    /* TODO: fill more options?? */
    /* VP: of course */
    /* Fill commissioning options from DATA frame; some of options can not be derived from frame */
    comm_params->options = ZB_GPDF_COMM_OPT_FLD(ZB_FALSE /* seq number caps - unknown */,
                                                ZB_FALSE /* rx_cap - definitely no */,
                                                ZB_FALSE /* mf ext - unknown */,
                                                ZB_FALSE /* pan id request - definitely no */,
                                                ZB_FALSE /* security key request - definitely no */,
                                                ZB_FALSE /* fixed location - unknown */,
                                                ZB_TRUE /* extended options - yes*/ );
    /* And fill extended options */
    comm_params->ext_options = ZB_GPDF_COMM_EXT_OPT_FLD(secur_lvl,
                                                        0x00 /* key type - key is not present */,
                                                        ZB_FALSE /* key not present */,
                                                        ZB_FALSE /* key encription */,
                                                        secur_lvl >= ZB_ZGP_SEC_LEVEL_FULL_NO_ENC /* outgoing frame counter present */);
    zgp_aprove_commissioning(param);
  }
  else
  {
    TRACE_MSG(TRACE_ZGP2, "Can't guess device id - drop autocommissioning gpdf", (FMT__0));
    zb_buf_free(param);
  }
}

static void zgp_sink_indicate_incoming_commissioning(zb_uint8_t param)
{
  zb_gpdf_info_t *gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);

  TRACE_MSG(TRACE_ZGP2, ">> zgp_sink_indicate_incoming_commissioning %hd", (FMT__H, param));

  if (ZGP_CTXC().app_comm_op_cb != NULL)
  {
    ZGP_CTXC().app_comm_op_cb(&gpdf_info->zgpd_id, param);
  }
  else
  {
    TRACE_MSG(TRACE_ZGP2, "Use ZB_ZGP_REGISTER_APP_CIC_CB(cb) to register callback", (FMT__0));
  }

  TRACE_MSG(TRACE_ZGP2, "<< zgp_sink_indicate_incoming_commissioning", (FMT__0));
  zb_buf_free(param);
}

/**
   A.3.5.2.4 Operation of sink side of GP Combo Basic (or our TargetMinus)
 */
void zb_gp_sink_data_indication(zb_uint8_t param)
{
  zb_gpdf_info_t *gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);
  zgp_tbl_ent_t   ent;
  zb_bool_t       have_sink = ZB_FALSE;

  TRACE_MSG(TRACE_ZGP2, ">> zb_gp_sink_data_indication param %hd", (FMT__H, param));
/*
  While in commissioning mode, the Basic Combo SHALL behave as described in
  sec. A.3.9.1, according to the supported commissioning functionality.
*/

  /*
  Then the Basic Combo checks if it has a Sink Table entry for this GPD
  (and Endpoint, matching or 0x00 or 0xff, if ApplicationID = 0b010).
*/
  if (ZB_GPDF_NFC_GET_FRAME_TYPE(gpdf_info->nwk_frame_ctl) != ZGP_FRAME_TYPE_MAINTENANCE)
  {
    have_sink = (zb_bool_t)(zgp_sink_table_read(&gpdf_info->zgpd_id, &ent) == RET_OK);
    TRACE_MSG(TRACE_ZGP3, "have_sink %hd", (FMT__H, have_sink));

    if (have_sink)
    {
      zgp_sink_set_lqi_rssi(&gpdf_info->zgpd_id, gpdf_info->lqi, gpdf_info->rssi);
    }
  }

  if (ZGP_IS_SINK_IN_COMMISSIONING_MODE())
  {
    TRACE_MSG(TRACE_ZGP3, "ZB_ZGP_COMMISSIONING_MODE active", (FMT__0));

    if (gpdf_info->zgpd_cmd_id >= ZB_GPDF_CMD_COMMISSIONING)
    {
      if (gpdf_info->zgpd_cmd_id == ZB_GPDF_CMD_COMMISSIONING
          && !ZB_GPDF_EXT_NFC_GET_RX_AFTER_TX(gpdf_info->nwk_ext_frame_ctl))
      {
        TRACE_MSG(TRACE_ZGP3, "Sink handles unidirectional commissioning frame", (FMT__0));
      }
      else
      {
        TRACE_MSG(TRACE_ZGP3, "Sink handles commissioning frame", (FMT__0));
      }
      ZB_SCHEDULE_CALLBACK(zgp_sink_handle_commissioning_gpdf, param);
      param = 0;
    }
    else
    if (ZB_GPDF_NFC_GET_AUTO_COMMISSIONING(gpdf_info->nwk_frame_ctl) &&
       ZB_GPDF_NFC_GET_FRAME_TYPE(gpdf_info->nwk_frame_ctl) == ZGP_FRAME_TYPE_DATA)
    {
      /*
        13.	Sink receives commissioning command: The pairing sink receives a
        Commissioning GPDF or Data GPDF with Auto-Commissioning 0b1 on the
        operational channel (in GP Commissioning Notification command or directly).
      */
      if (have_sink)
      {
        TRACE_MSG(TRACE_ZGP2, "Already have sink table entry - exit commissioning", (FMT__0));
        *ZB_BUF_GET_PARAM(param, zb_uint8_t) = ZB_ZGP_COMMISSIONING_COMPLETED;
        ZB_SCHEDULE_CALLBACK(zb_zgps_finalize_commissioning, param);
        param = 0;
      }
      else
      {
        TRACE_MSG(TRACE_ZGP3, "Sink handles autocommissioning frame", (FMT__0));
        ZB_SCHEDULE_CALLBACK(zgp_sink_handle_autocommissioning_gpdf, param);
        param = 0;
      }
    }
    else
    {
      TRACE_MSG(TRACE_ZGP3, "Data frame in commissioning mode. Drop.", (FMT__0));
      zb_buf_free(param);
      param = 0;
    }
  }

  /*
    If all previous checks succeed, the Combo Basic SHALL accept the GPD commands
    received in GP Notification with Proxy info present sub-field of the Options
    field set to 0b0. Then  if the Basic Combo has a Translation Table, the Basic
    Combo checks the value of the EndPoint field of the Translation Table entries
    for the GPD. If there is a Translation Table with value of the EndPoint field
    other than 0x00 and 0xfd, the Basic Combo SHALL also translate the GPD command
    into a Zigbee command, as indicated in the Translation Table entry, and send it
    to the paired local endpoint(s), as indicated in the EndPoint field, for
    execution.
  */
  /* EES: why it needed? */
  /* TODO: check for "GP Notification with Proxy info present 0" */
  if (param && gpdf_info->zgpd_cmd_id < ZB_GPDF_CMD_COMMISSIONING)
  {
    TRACE_MSG(TRACE_ZGP3, "Passing ZGPD {%hd} up to ZCL", (FMT__H, zb_buf_len(param)));
    ZB_SCHEDULE_CALLBACK(zb_zgp_from_gpdf_to_zcl, param);
    param = 0;
  }

  /*
    If the Basic Combo has a Sink Table entry for this GPD (and Endpoint, matching
    or 0x00 or 0xff, if ApplicationID = 0b010), the Basic Combo is in operational
    mode and if the received GPD command is either a GPD Commissioning command , the
    Basic Combo SHALL NOT enter commissioning mode and SHALL NOT perform any
    commissioning action. The Basic Combo MAY provide some indication to the user
    about the attempted commissioning action. Other GPD commissioning commands
    received in operational mode SHALL be silently dropped,  unless their handling
    in operation is explicitly described.
  */
  if (param && gpdf_info->zgpd_cmd_id == ZB_GPDF_CMD_COMMISSIONING)
  {
    TRACE_MSG(TRACE_ZGP3, "Commissioning frame when sink is in operating mode - indicate it to user", (FMT__0));
    ZB_SCHEDULE_CALLBACK(zgp_sink_indicate_incoming_commissioning, param);
    param = 0;
  }

  if (param && gpdf_info->zgpd_cmd_id == ZB_GPDF_CMD_APPLICATION_DESCR)
  {
    TRACE_MSG(TRACE_ERROR, "App descr frame when sink is in operating mode", (FMT__0));
    param = 0;
  }

  if (param && gpdf_info->zgpd_cmd_id == ZB_GPDF_CMD_DECOMMISSIONING)
  {
    /*
      On receipt of GPD Decommissioning command,  both in operational and in
      commissioning mode, the sink checks if it has a Sink Table entry for this GPD
      ID (and Endpoint, matching or 0x00 or 0xff, if ApplicationID = 0b010). If not,
      the frame is ignored. If yes, the sink decrypts the frame, if directly
      received, performs a freshness check, as described in A.3.6.1.2.1 and compares
      the SecurityLevel and SecurityKeyType with the values stored in the Sink Table
      entry. If any of those checks fails, the frame is silently dropped. If all
      those checks succeed, the sink removes this Sink Table entry, removes/replaces
      with generic entries the corresponding Translation Table entries if Translation
      Table functionality is supported, and removes Green Power EndPoint membership
      at APS level in the groups listed in the removed entry, if any. Then, the sink
      schedules sending of a GP Pairing command for this GPD ID (and Endpoint,
      matching or 0x00 or 0xff, if ApplicationID = 0b010), with the RemoveGPD
      sub-field set to 0b1. If the removed Sink Table entry included any
      pre-commissioned groups,  and if the GPD Decommissioning command was received in
      commissioning mode, the sink SHALL also send GP Pairing Configuration message,
      with Action sub-field of the Actions field set to 0b100, SendGPPairing
      sub-field of the Actions field set to 0b0, and Number of paired endpoints field
      set to 0xfe.
    */
    TRACE_MSG(TRACE_ZGP3, "Decommissioning frame", (FMT__0));
    ZB_SCHEDULE_CALLBACK(zb_zgps_process_decommissioning_cmd, param);
    param = 0;
  }

  if (param)
  {
    /* If the Basic Combo does not have a Sink Table entry for this GPD (and Endpoint, matching or
     * 0x00 or 0xff, if ApplicationID = 0b010), or the Sink Table entry exists but with another
     * communication mode, and the incoming GP Notification message was received as lightweight or
     * full unicast, the sink SHALL drop the command; it SHOULD broadcast a GP Pairing command for
     * this GPD with the AddSink flag set to 0b1 and the correct value in the CommunicationMode
     * sub-field and then a GP Pairing command for this GPD, the CommunicationMode flag set to the
     * incorrect communication mode as in the triggering GP Notification, and AddSink flag set to
     * 0b0. If the GPD command was received directly or in groupcast and the sink does not have a
     * Sink Table entry for this GPD (and Endpoint, matching or 0x00 or 0xff, if ApplicationID =
     * 0b010) and communication mode, the sink SHALL silently ignore it.
     */
    if (!have_sink)
    {
      TRACE_MSG(TRACE_ZGP3, "Not have a sink table entry for this GPD. Drop.", (FMT__0));
      zb_buf_free(param);
      param = 0;
    }
    else
    if (!gpdf_info->rx_directly)
    {
      TRACE_MSG(TRACE_ZGP3, "We received this GPDF in non-directly mode.", (FMT__0));
      if (gpdf_info->recv_as_unicast &&
          (ZGP_TBL_SINK_GET_COMMUNICATION_MODE(&ent) != ZGP_COMMUNICATION_MODE_LIGHTWEIGHT_UNICAST))  /* EES: we support only LW unicast in ProxyBasic */
      {
        zb_zgp_gp_pairing_send_req_t *req;

        TRACE_MSG(TRACE_ZGP3, "Communication mismatch. GPDF recv as UNICAST but we have groupcasr entry: %hd",
                  (FMT__H, ZGP_TBL_SINK_GET_COMMUNICATION_MODE(&ent)));

        ZB_ZGP_GP_PAIRING_SEND_REQ_CREATE(param, req, &ent, NULL);
        ZB_ZGP_GP_PAIRING_MAKE_SEND_OPTIONS(req, 1, 0, 1);
        ZB_ZGP_GP_PAIRING_OPTIONS_SET_SEND_INCORRECT_LW_PAIR_REMOVE(req);

        zgp_cluster_send_gp_pairing(param);
        param = 0;
      }
    }
  }

  if (param)
  {
    TRACE_MSG(TRACE_ZGP3, "Not handled frame - hmm? Drop it", (FMT__0));
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_gp_sink_data_indication", (FMT__0));
}

#ifdef ZB_ENABLE_ZGP_DIRECT
void zb_zgps_send_data(zb_uint8_t param)
{
  zb_gp_data_req_t          *req;
  zb_zgps_send_cmd_params_t *params;
  zb_uint8_t                 payload_len;

  TRACE_MSG(TRACE_ZGP2, ">> zb_zgps_send_data, param %hd", (FMT__H, param));

  ZB_ASSERT(param);
  params = ZB_BUF_GET_PARAM(param, zb_zgps_send_cmd_params_t);
  payload_len = zb_buf_len(param);

  ZB_ASSERT(payload_len <= sizeof(req->pld));
  req = zb_buf_alloc_left(param, sizeof(zb_gp_data_req_t)-sizeof(req->pld));
  req->handle         = params->handle;
  req->action = ZB_GP_DATA_REQ_ACTION_ADD_GPDF;
  req->tx_options     = params->tx_options;
  req->cmd_id = params->cmd_id;
  req->payload_len = payload_len;
  req->tx_q_ent_lifetime = params->lifetime;
  ZB_MEMCPY(&req->zgpd_id, &params->zgpd_id, sizeof(zb_zgpd_id_t));
  ZB_64BIT_ADDR_COPY(&req->ieee_addr, &params->ieee_addr);

  schedule_gp_txdata_req(param);

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgps_send_data", (FMT__0));
}
#endif  /* ZB_ENABLE_ZGP_DIRECT */
#endif //ZB_ENABLE_ZGP_SINK
