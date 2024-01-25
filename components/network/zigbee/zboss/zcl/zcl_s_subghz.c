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
/* PURPOSE: SERVER: Sub-GHz cluster
*/

#define ZB_TRACE_FILE_ID 1945

#include "zb_common.h"

#if !defined ZB_ED_ROLE

#if defined (ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ) || defined DOXYGEN

#include "zcl/zb_zcl_subghz.h"

#if !defined ZB_ENABLE_SE_MIN_CONFIG && !defined ZB_ENABLE_HA
#error "Profile not defined"
#endif /* !ZB_ENABLE_SE_MIN_CONFIG && !ZB_ENABLE_HA */

/* Let it 1 bucket period rounded to 1 minute. Isn't it too much already? */
#define ZB_SUBGHZ_SUSPENSION_TIME (zb_mac_duty_cycle_get_time_period_sec() + 59U) / 60U
#define ZB_SUBGHZ_SUSPEND_NODES_CNT 5U

static zb_uint8_t gs_subghz_server_received_commands[] =
{
  ZB_ZCL_CLUSTER_ID_SUBGHZ_SERVER_ROLE_RECEIVED_CMD_LIST
};

static zb_uint8_t gs_subghz_server_generated_commands[] =
{
  ZB_ZCL_CLUSTER_ID_SUBGHZ_SERVER_ROLE_GENERATED_CMD_LIST
};

static zb_discover_cmd_list_t gs_subghz_server_cmd_list =
{
  sizeof(gs_subghz_server_received_commands), gs_subghz_server_received_commands,
  sizeof(gs_subghz_server_generated_commands), gs_subghz_server_generated_commands
};

/* *****************************************************************************
 * Internal calls
 * *****************************************************************************/
static void zb_subghz_send_suspend_signal(zb_uint8_t param);

/* sends ZCL default response */
static zb_bool_t zb_subghz_send_default_response(zb_uint8_t param,
                                               const zb_zcl_parsed_hdr_t *cmd_info,
                                               zb_zcl_status_t status)
{

  TRACE_MSG(TRACE_ZCL1, "== zb_subghz_send_default_response: status == %d", (FMT__D, status));

  if (!cmd_info->disable_default_response)
  {
    ZB_ZCL_SEND_DEFAULT_RESP(param,
                             ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr,
                             ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                             ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint,
                             ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).dst_endpoint,
                             cmd_info->profile_id,
                             cmd_info->cluster_id,
                             cmd_info->seq_number,
                             cmd_info->cmd_id,
                             status);
    return ZB_TRUE;
  }

  return ZB_FALSE;
}


static void zb_subghz_suspend_device(zb_uint8_t param, zb_uint16_t ref)
{
  zb_zcl_subghz_suspend_zcl_msg_payload_t payload;
  zb_neighbor_tbl_ent_t *ent;
  zb_ieee_addr_t addr;
  zb_addr_u dst_addr;
  zb_uint16_t addr_short;
  zb_ret_t ret;

  ret = zb_nwk_neighbor_get((zb_address_ieee_ref_t)ref, ZB_FALSE, &ent);
  if (ret == RET_OK)
  {
    zb_address_by_ref(addr, &addr_short, (zb_address_ieee_ref_t)ref);
    dst_addr.addr_short = addr_short;

    payload = (ent->suspended) ? (zb_uint8_t)ZB_SUBGHZ_SUSPENSION_TIME : 0U;

    zb_subghz_srv_suspend_zcl_messages(param,
                                      &dst_addr,
                                      ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                      (zb_uint8_t)ent->subghz_ep,
                                      ZCL_CTX().subghz_ctx.ep,
                                      &payload);
  }
}


static void discover_endpoint_cb(zb_uint8_t param)
{
  zb_zdo_match_desc_resp_t *resp = (zb_zdo_match_desc_resp_t*)zb_buf_begin(param);
  zb_apsde_data_indication_t *ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);
  zb_address_ieee_ref_t ref;
  zb_neighbor_tbl_ent_t *ent;

  if (resp->status == ZB_ZDP_STATUS_SUCCESS && resp->match_len > 0U)
  {
    (void)zb_address_by_short(ind->src_addr, ZB_FALSE, ZB_FALSE, &ref);
    (void)zb_nwk_neighbor_get(ref, ZB_FALSE, &ent);

    /* Match EP list follows right after the response header */
    ent->subghz_ep = *(zb_uint8_t *)(resp + 1);

    (void)zb_buf_get_out_delayed_ext(zb_subghz_suspend_device, ref, 0);
  }

  zb_buf_free(param);
}


static void zb_subghz_discover_endpoint(zb_uint8_t param, zb_uint16_t ref)
{
  zb_zdo_match_desc_param_t *req;
  zb_ieee_addr_t addr;
  zb_uint16_t addr_short;

  zb_address_by_ref(addr, &addr_short, (zb_address_ieee_ref_t)ref);

  req = zb_buf_initial_alloc(param, sizeof(zb_zdo_match_desc_param_t) + (1U) * sizeof(zb_uint16_t));

  req->nwk_addr = addr_short;
  req->addr_of_interest = addr_short;
  req->profile_id = ZB_ZCL_SUBGHZ_CLUSTER_PROFILE_ID();
  req->num_in_clusters = 0;
  req->num_out_clusters = 1;
  req->cluster_list[0] = ZB_ZCL_CLUSTER_ID_SUB_GHZ;

  (void)zb_zdo_match_desc_req(param, discover_endpoint_cb);
}


static void zb_subghz_schedule_device_suspension(zb_address_ieee_ref_t ref,
                                                      zb_uint8_t ep,
                                                      zb_bool_t mark_as_suspended,
                                                      zb_bool_t force)
{
  zb_neighbor_tbl_ent_t *ent;

  TRACE_MSG(TRACE_ZCL3, "zb_subghz_schedule_device_suspension ref %hd ep %hd mark_as_suspended %d force %d",
            (FMT__H_H_D_D, ref, ep, mark_as_suspended, force));

  if (zb_nwk_neighbor_get(ref, ZB_FALSE, &ent) == RET_OK
      && (force || mark_as_suspended != ent->suspended))
  {
    /* mark device as suspended immediately to avoid processing of incoming ZCL messages from it
     * in case if the device sent them earlier than it received our SuspendZCLMessages command
     */
    ent->suspended = mark_as_suspended;

    if((ep != 0U) && ep != ZB_ZCL_BROADCAST_ENDPOINT)
    {
      ent->subghz_ep = ep;
    }

    if (ent->subghz_ep != ZB_ZCL_BROADCAST_ENDPOINT)
    {
      /* send SuspendZCLMessages */
      (void)zb_buf_get_out_delayed_ext(zb_subghz_suspend_device, ref, 0);
    }
    else
    {
      /* send MatchDescriptor request */
      (void)zb_buf_get_out_delayed_ext(zb_subghz_discover_endpoint, ref, 0);
    }
  }
}


/* Cluster's callback for handling MLME-DUTY-CYCLE-MODE.indication */
static void zb_subghz_duty_cycle_mode_indication(zb_uint8_t mode)
{
  zb_uint8_t i, cnt;

  cnt = zb_nwk_neighbor_get_subghz_list(ZCL_CTX().subghz_ctx.srv.dev_list, ZB_NEIGHBOR_TABLE_SIZE);
  TRACE_MSG(TRACE_ZCL1, "zb_subghz_duty_cycle_mode_indication mode %hd cnt %d", (FMT__H_D, mode, cnt));
  if (mode > ZCL_CTX().subghz_ctx.srv.mode)
  {
    /* negative transition: NORMAL -> LIMITED -> CRITICAL -> SUSPENDED */

    switch (mode)
    {
      case ZB_MAC_DUTY_CYCLE_STATUS_LIMITED:
        /* SE spec, 5.4.16 Duty Cycle Monitoring
         *
         * When the network Coordinator device transitions from Normal to Limited Duty Cycle
         * state, it shall use implementation-specific mechanisms to determine which Sub-GHz
         * device(s) to throttle on the network.
         *
         * select top N devices to suspend ==> set 'cnt' to N
         *
         * NOTE: this is the simplest (and the dumbest) way of throttling. Should implement
         * something more complex and smarter after it will be clear how we should throttle the
         * network (test this on samples).
         */
        /* EE: TODO: limit not by cnt, but by packets count # decrease. */

        cnt = (cnt > ZB_SUBGHZ_SUSPEND_NODES_CNT) ? ZB_SUBGHZ_SUSPEND_NODES_CNT : cnt;
        break;

      case ZB_MAC_DUTY_CYCLE_STATUS_CRITICAL:
        /* SE spec, 5.4.16 Duty Cycle Monitoring
         *
         * When the network Coordinator device transitions from Limited to Critical Duty Cycle
         * state:
         * a. the Coordinator shall inform all devices except BOMDs, using the Sub-GHz cluster
         * Suspend ZCL Messages command, that they shall suspend their ZCL communications to
         * all devices for the period indicated in the command payload.
         *
         * in critical mode should suspend all Sub-GHz devices ==> left 'cnt' unchanged
         *
         * NOTE: BOMD is not supported for now in stack so we do not know whether device is BOMD
         * or not. Here we need only 1 flag from neighbor table ('is_bomd') to check. Underlying
         * functionality should be implemented to set this flag in table for appropriate devices.
         *
         * NOTE: also it is unclear why we should not suspend BOMD's when achieved 'Critical' mode
         * but nothing said about it in 'Limited' mode (see spec or comment in the above case).
         */
        break;

      default:
        /* In Suspend MAC does not send anything */
        /* nothing for NORMAL and SUSPENDED ==> set 'cnt' to 0 to avoid device suspension */
        cnt = 0;
        break;
    }

    for (i = 0; i < cnt; i++)
    {
      zb_subghz_schedule_device_suspension(ZCL_CTX().subghz_ctx.srv.dev_list[i],
                                           ZB_ZCL_BROADCAST_ENDPOINT,
                                           ZB_TRUE, ZB_FALSE);
    }
  }
  else if (mode < ZCL_CTX().subghz_ctx.srv.mode
           && mode == (zb_uint8_t)ZB_MAC_DUTY_CYCLE_STATUS_NORMAL)
  {
    /* ZC back to normal mode.
       Unsuspend all devices.
    */
    for (i = 0; i < cnt; i++)
    {
      zb_neighbor_tbl_ent_t *ent;

      if (zb_nwk_neighbor_get(ZCL_CTX().subghz_ctx.srv.dev_list[i], ZB_FALSE, &ent) == RET_OK
          && ent->suspended)
      {
        ent->suspended = ZB_FALSE;
        if (ent->subghz_ep != ZB_ZCL_BROADCAST_ENDPOINT)
        {
          /*
            FIXME: not sure really need to send unsuspend.
          */
          (void)zb_buf_get_out_delayed_ext(zb_subghz_suspend_device, ZCL_CTX().subghz_ctx.srv.dev_list[i], 0);
        }
      }
    }
  }

  if (mode != ZCL_CTX().subghz_ctx.srv.mode)
  {
    if (mode == (zb_uint8_t)ZB_MAC_DUTY_CYCLE_STATUS_SUSPENDED)
    {
      ZCL_CTX().subghz_ctx.cli.suspend_zcl_messages = ZB_TRUE;
      ZB_SCHEDULE_CALLBACK(zb_subghz_send_suspend_signal, 0);
    }
    else if (ZCL_CTX().subghz_ctx.srv.mode == (zb_uint8_t)ZB_MAC_DUTY_CYCLE_STATUS_SUSPENDED)
    {
      ZCL_CTX().subghz_ctx.cli.suspend_zcl_messages = ZB_FALSE;
      ZB_SCHEDULE_CALLBACK(zb_subghz_send_suspend_signal, 0);
    }
    else
    {
      /* MISRA rule 15.7 requires empty 'else' branch. */
    }
  }

  /* set new mode */
  ZCL_CTX().subghz_ctx.srv.mode = mode;

  /* TODO: send event to app? */
}


zb_bool_t zb_subghz_catch_ota_image_block_req(zb_uint8_t *paramp)
{
#if defined ZB_ZCL_SUPPORT_CLUSTER_OTA_UPGRADE &&  defined ZB_HA_ENABLE_OTA_UPGRADE_SERVER && defined ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ
  zb_uint8_t param = *paramp;
  if (ZCL_CTX().subghz_ctx.srv.mode == ZB_MAC_DUTY_CYCLE_STATUS_CRITICAL)
  {
    zb_zcl_parsed_hdr_t cmd_info;

    ZB_MEMCPY(&cmd_info, ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t), sizeof(zb_zcl_parsed_hdr_t));
    if (cmd_info.cluster_id == ZB_ZCL_CLUSTER_ID_OTA_UPGRADE
        && cmd_info.cmd_id == ZB_ZCL_CMD_OTA_UPGRADE_IMAGE_BLOCK_ID)
    {
      /*
If active, the OTA server shall respond to any Image Block Request command with an Image Block Response command with a status of WAIT_FOR_DATA.
       */
      ZB_ZCL_OTA_UPGRADE_SEND_IMAGE_BLOCK_WAIT_FOR_DATA_RES(param,
                                                            ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).source.u.short_addr,
                                                            ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                                            ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).src_endpoint,
                                                            ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).dst_endpoint,
                                                            cmd_info.profile_id,
                                                            cmd_info.seq_number,
                                                            0, 120, 0);
      *paramp = 0;
    }
  }
#endif
  return (zb_bool_t)(*paramp == 0U);
}

static void zb_subghz_send_suspend_signal(zb_uint8_t param)
{
  if (param == 0U)
  {
    (void)zb_buf_get_out_delayed(zb_subghz_send_suspend_signal);
  }
  else
  {
    if (ZCL_CTX().subghz_ctx.cli.suspend_zcl_messages)
    {
      zb_uint_t *susp_period = (zb_uint_t *)zb_app_signal_pack(param, ZB_SIGNAL_SUBGHZ_SUSPEND, RET_OK, (zb_uint8_t)sizeof(zb_uint_t));
      *susp_period = 0;
      if (*susp_period == 0U)
      {
        *susp_period = (zb_uint_t)-1;
      }
      ZB_SCHEDULE_CALLBACK(zboss_signal_handler, param);
    }
    else
    {
      (void)zb_app_signal_pack(param, ZB_SIGNAL_SUBGHZ_RESUME, RET_OK, 0);
      ZB_SCHEDULE_CALLBACK(zboss_signal_handler, param);
    }
  }
}


/* *****************************************************************************
 * Server-side implementaiton
 * *****************************************************************************/

/* sends SuspendZCLMessages command */
void zb_subghz_srv_suspend_zcl_messages(zb_uint8_t param,
                                             zb_addr_u *dst_addr,
                                             zb_uint8_t dst_addr_mode,
                                             zb_uint8_t dst_ep,
                                             zb_uint8_t src_ep,
                                             zb_zcl_subghz_suspend_zcl_msg_payload_t *payload)
{
  TRACE_MSG(TRACE_ZCL1, ">> zb_subghz_srv_suspend_zcl_messages", (FMT__0));

  zb_zcl_send_cmd(param,
    dst_addr, dst_addr_mode, dst_ep,
    ZB_ZCL_FRAME_DIRECTION_TO_CLI,
    src_ep,
    payload, (zb_uint8_t)sizeof(*payload), NULL,
    ZB_ZCL_CLUSTER_ID_SUB_GHZ,
    ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
    (zb_uint8_t)ZB_ZCL_SUBGHZ_SRV_CMD_SUSPEND_ZCL_MESSAGES,
    NULL
  );

  TRACE_MSG(TRACE_ZCL1, "<< zb_subghz_srv_suspend_zcl_messages", (FMT__0));

  return;
}


/* Detects whether specified device was suspended and sends SuspendZCLMessages command.
 * If 'send_zero_status' == ZB_TRUE, send command even if payload is zero
 */
static zb_bool_t zb_subghz_srv_device_suspended_int(zb_uint16_t addr,
                                                    zb_uint8_t ep,
                                                    zb_bool_t send_zero_status)
{
  zb_address_ieee_ref_t ref;
  zb_neighbor_tbl_ent_t *ent;
  zb_bool_t ret = ZB_FALSE;
  zb_ret_t ret_nwk, ret_addr;

  /* SE 1.4 spec, subclause 5.14.6
   *
   * 3. ...however, the Coordinator device shall respond to any ZCL-layer messages received from
   * any device it has suspended with a Suspend ZCL Messages command indicating the required
   * non-zero period of suspension. If the Suspension Period of a resultant Suspend ZCL Messages
   * command has a value of zero, then the device is not suspended. If the payload is non-zero,
   * then it shall indicate the duration that the device must wait before it can next send any
   * ZCL commands or a Get Suspend ZCL Messages Status command.
   *
   * 5. The Coordinator device shall respond to ZCL-layer messages received from the device(s)
   * identified in step 4, with a Suspend ZCL Messages command indicating the required period
   * of suspension. The incoming request packet will be ignored.
   *
   * 6.c. The Coordinator device shall respond to ZCL-layer messages received from any device
   *  whilst in the Critical state with a Suspend ZCL Messages command indicating the
   *  required period of suspension. The incoming request packet will be ignored.
   *
   * Thus, according to current throttling logic we should check whether device was suspended
   * earlier (== mode was changed and device was marked as suspended) ==> should be throttled.
   * Otherwise no.
   */
  ret_addr = zb_address_by_short(addr, ZB_FALSE, ZB_FALSE, &ref);
  ret_nwk = zb_nwk_neighbor_get(ref, ZB_FALSE, &ent);
  if (ret_addr == RET_OK
      && ret_nwk == RET_OK)
  {
    if (ZCL_CTX().subghz_ctx.srv.mode >= (zb_uint8_t)ZB_MAC_DUTY_CYCLE_STATUS_CRITICAL)
    {
      /* just in case - suspend it now */
      ent->suspended = ZB_TRUE;
    }
    if (ent->suspended || send_zero_status)
    {
      zb_subghz_schedule_device_suspension(ref, ep, (zb_bool_t)ent->suspended, ZB_TRUE);
    }

    ret = (zb_bool_t)ent->suspended;
  }
  return ret;
}


zb_bool_t zb_subghz_srv_device_suspended(zb_uint16_t addr)
{
  return zb_subghz_srv_device_suspended_int(addr, ZB_ZCL_BROADCAST_ENDPOINT, ZB_FALSE);
}


/* process GetSuspendZCLMessagesStatus command */
static zb_bool_t zb_subghz_srv_get_suspend_zcl_messages_status(zb_uint8_t param,
                                                               const zb_zcl_parsed_hdr_t *cmd_info)
{
  TRACE_MSG(TRACE_ZCL1, ">> zb_subghz_srv_get_suspend_zcl_messages_status %hd", (FMT__H, param));

  /* SE spec, D.14.2.3.1.3
   * The command is also sent in response to a Get Suspend ZCL Messages Status command.
   */
  (void)zb_subghz_srv_device_suspended_int(ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr,
                                           ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint,
                                           ZB_TRUE);

  zb_buf_free(param);

  TRACE_MSG(TRACE_ZCL1, "<< zb_subghz_srv_get_suspend_zcl_messages_status", (FMT__0));
  return ZB_TRUE;
}


static zb_bool_t zb_subghz_srv_handle_client_commands(zb_uint8_t param,
                                                      const zb_zcl_parsed_hdr_t *cmd_info)
{
  zb_bool_t res;

  switch ((zb_zcl_subghz_cli_cmd_t)cmd_info->cmd_id)
  {
    case ZB_ZCL_SUBGHZ_CLI_CMD_GET_SUSPEND_ZCL_MESSAGES_STATUS:
      res = zb_subghz_srv_get_suspend_zcl_messages_status(param, cmd_info);
      break;

    default:
      res = zb_subghz_send_default_response(param, cmd_info, ZB_ZCL_STATUS_UNSUP_CLUST_CMD);
      break;
  }

  return res;
}

/* *****************************************************************************
 * Cluster entry point for ZB Stack
 * *****************************************************************************/

static zb_bool_t zb_zcl_process_s_subghz_specific_command(zb_uint8_t param)
{
  zb_zcl_parsed_hdr_t cmd_info;
  zb_bool_t res = ZB_FALSE;

  TRACE_MSG(TRACE_ZCL1, ">> zb_zcl_process_s_subghz_specific_command, param %hd", (FMT__H, param));

  if ( ZB_ZCL_GENERAL_GET_CMD_LISTS_PARAM == param )
  {
    ZCL_CTX().zb_zcl_cluster_cmd_list = &gs_subghz_server_cmd_list;
    return ZB_TRUE;
  }

  ZB_ZCL_COPY_PARSED_HEADER(param, &cmd_info);

  ZB_ASSERT(cmd_info.cluster_id == ZB_ZCL_CLUSTER_ID_SUB_GHZ);

  if(ZB_ZCL_FRAME_DIRECTION_TO_SRV == cmd_info.cmd_direction)
  {
    res = zb_subghz_srv_handle_client_commands(param, &cmd_info);
  }

  if (res == ZB_FALSE)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZCL1, "<< zb_zcl_process_s_subghz_specific_command: processed == %d", (FMT__D, res));
  return res;
}

void zb_zcl_subghz_init_server(void)
{
  ZCL_CTX().subghz_ctx.ep = get_endpoint_by_cluster(ZB_ZCL_CLUSTER_ID_SUB_GHZ, ZB_ZCL_CLUSTER_SERVER_ROLE);
  TRACE_MSG(TRACE_ZCL1, "zb_subghz_init SERVER ep %hd", (FMT__H, ZCL_CTX().subghz_ctx.ep));
  zb_zdo_register_duty_cycle_mode_indication_cb(zb_subghz_duty_cycle_mode_indication);

  (void)zb_zcl_add_cluster_handlers(ZB_ZCL_CLUSTER_ID_SUB_GHZ,
                                    ZB_ZCL_CLUSTER_SERVER_ROLE,
                                    NULL,
                                    NULL,
                                    zb_zcl_process_s_subghz_specific_command);
}

#endif /* ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ || defined DOXYGEN */

#endif /* !defined ZB_ED_ROLE */
