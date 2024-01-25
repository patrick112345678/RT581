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
/*  PURPOSE: frames processing logic of GP cluster
*/

#define ZB_TRACE_FILE_ID 894

#include "zb_common.h"

#ifdef ZB_ENABLE_ZGP

#include "zb_zdo.h"
#include "zb_aps.h"
#include "zgp/zgp_internal.h"

enum zgp_gp_notif_iterator_mode_e
{
  ZGP_GP_NOTIF_ITERATOR_MODE_UNDEFINED,
  ZGP_GP_NOTIF_ITERATOR_MODE_LWADDR,
  ZGP_GP_NOTIF_ITERATOR_MODE_DGROUP,
  ZGP_GP_NOTIF_ITERATOR_MODE_CGROUP,
  /* end of enum */
  ZGP_GP_NOTIF_ITERATOR_MODE_END
};

/* A.3.6.3.3.2   Derivation of alias sequence number
 * GP command list for derivation of alias sequence number
 */
typedef enum zb_gpc_enum
{
  ZB_GPC_GP_DERIVED_GROUPCAST_GP_NOTIFICATION,
  ZB_GPC_GP_PAIRING_SEARCH,
  ZB_GPC_GP_TUNELLING_STOP,
  ZB_GPC_GP_COMMISIONING_NOTIFICATION,
  ZB_GPC_GP_COMMISSIONED_GROUPCAST_GP_NOTIFICATION,
  ZB_GPC_GP_BROADCAST_GP_NOTIFICATION,
  ZB_GPC_GP_DEVICE_ANNCE
} zb_gpc_enum_t;

typedef struct zgp_gp_notif_iterator_s
{
  zb_uint8_t idx;
  zb_uint8_t mode;
} zgp_gp_notif_iterator_t;

#ifdef ZB_ENABLE_ZGP_PROXY
static void zgp_proxy_send_gp_notification_req(zb_uint8_t param);
#endif  /* ZB_ENABLE_ZGP_PROXY */

#if defined ZB_ENABLE_ZGP_SINK && defined ZB_ENABLE_ZGP_PROXY
static void zb_gp_data_ind_dup(zb_uint8_t param, zb_uint16_t oparam)
{
  zb_buf_copy(param, oparam);

  TRACE_MSG(TRACE_ZGP2, "scheduling zb_gp_proxy_data_indication param %hd", (FMT__H, param));
  ZB_SCHEDULE_CALLBACK(zb_gp_proxy_data_indication, param);
  TRACE_MSG(TRACE_ZGP2, "scheduling zb_gp_sink_data_indication param %hd", (FMT__H, oparam));
  ZB_SCHEDULE_CALLBACK(zb_gp_sink_data_indication, oparam);
}
#endif  /* defined ZB_ENABLE_ZGP_SINK && defined ZB_ENABLE_ZGP_PROXY */

/**
   Described in A.3.5.2.3 Operation of GP Proxy Basic and proxy side of GP Combo
   Basic and A.3.5.2.4 Operation of sink side of GP Combo Basic
 */
void zb_gp_data_indication(zb_uint8_t param)
{
  zb_gpdf_info_t *gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);

  TRACE_MSG(TRACE_ZGP2, ">> zb_gp_data_indication param %hd", (FMT__H, param));

#if defined ZB_ENABLE_ZGP_SINK && defined ZB_ENABLE_ZGP_PROXY

  TRACE_MSG(TRACE_ZGP2, "Sink mode: %hd, Proxy mode: %hd, RX directly: %hd",
            (FMT__H_H_H, ZGP_CTXC().sink_mode, ZGP_CTXC().proxy_mode, gpdf_info->rx_directly));

  if (gpdf_info->rx_directly)
  {
    /*
      On receiving a GPD frame in direct mode, the GP Combo Basic device SHALL NOT
      only forward it to local paired end points, but also participate in forwarding
      this frame to other sinks listed in its Proxy Table for this GPD  (and GPD
      Endpoint, matching or 0x00 or 0xff, if ApplicationID = 0b010), if any, as
      specified in section A.3.5.2.1.
    */
    zb_buf_get_out_delayed_ext(zb_gp_data_ind_dup, param, 0);
    param = 0;
  }
  else
  {
    ZB_SCHEDULE_CALLBACK(zb_gp_sink_data_indication, param);
    param = 0;
  }
#else
#ifdef ZB_ENABLE_ZGP_SINK

  TRACE_MSG(TRACE_ZGP2, "Sink mode: %hd, RX directly: %hd",
            (FMT__H_H, ZGP_CTXC().sink_mode, gpdf_info->rx_directly));

  ZB_SCHEDULE_CALLBACK(zb_gp_sink_data_indication, param);
  param = 0;
#endif  /* ZB_ENABLE_ZGP_SINK */
#ifdef ZB_ENABLE_ZGP_PROXY

  TRACE_MSG(TRACE_ZGP2, "Proxy mode: %hd, RX directly: %hd",
            (FMT__H_H, ZGP_CTXC().proxy_mode, gpdf_info->rx_directly));

  if (gpdf_info->rx_directly)
  {
    ZB_SCHEDULE_CALLBACK(zb_gp_proxy_data_indication, param);
    param = 0;
  }
#endif  /* ZB_ENABLE_ZGP_PROXY */
#endif  /* defined ZB_ENABLE_ZGP_SINK && defined ZB_ENABLE_ZGP_PROXY */

  if (param)
  {
    TRACE_MSG(TRACE_ZGP2, "Drop buffer %hd. RX directly: %hd",
              (FMT__H_H, param, gpdf_info->rx_directly));
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_gp_data_indication", (FMT__0));
}

static zb_ret_t zgp_add_group_alias_for_entry(zgp_pair_group_list_t *group_list,
                                              zb_uint16_t            sink_group,
                                              zb_uint16_t            alias)
{
  zb_uint8_t i;
  zb_uint8_t free_slot = 0xff;
  zb_ret_t   ret = RET_OK;

  TRACE_MSG(TRACE_ZGP2, ">> zgp_add_group_alias_for_entry 0x%x 0x%x",
            (FMT__D_D, sink_group, alias));

  if (sink_group != ZGP_PROXY_GROUP_INVALID_IDX)
  {
    for (i = 0; i < ZB_ZGP_MAX_SINK_GROUP_PER_GPD; i++)
    {
      //search that table already have this group-alias pair
      if (group_list[i].sink_group == sink_group &&
          group_list[i].alias == alias)
      {
        ret = RET_ALREADY_EXISTS;
        break;
      }

      if (group_list[i].sink_group == ZGP_PROXY_GROUP_INVALID_IDX &&
          free_slot == 0xff)
      {
        //we found free slot, remember it
        free_slot = i;
      }
    }

    if (ret == RET_OK)
    {
      if (free_slot != 0xff)
      {
        group_list[free_slot].sink_group = sink_group;
        group_list[free_slot].alias = alias;
        TRACE_MSG(TRACE_ZGP2, "add into sgrp: group[0x%x]-alias[0x%x]",
                  (FMT__D_D, sink_group, alias));
      }
      else
      {
        TRACE_MSG(TRACE_ERROR, "No free space in sgrp table", (FMT__0));
        ret = RET_TABLE_FULL;
      }
    }
  }

  TRACE_MSG(TRACE_ZGP2, "<< zgp_add_group_alias_for_entry %hd", (FMT__H, ret));
  return ret;
}

static zb_ret_t zgp_del_group_from_entry(zgp_pair_group_list_t *group_list,
                                         zb_uint16_t            sink_group)
{
  zb_uint8_t i;
  zb_ret_t   ret = RET_NOT_FOUND;

  TRACE_MSG(TRACE_ZGP2, ">> zgp_del_group_from_entry 0x%x",
            (FMT__D, sink_group));

  if (sink_group != ZGP_PROXY_GROUP_INVALID_IDX)
  {
    for (i = 0; i < ZB_ZGP_MAX_SINK_GROUP_PER_GPD; i++)
    {
      //search that table already have this group-alias pair
      if (group_list[i].sink_group == sink_group)
      {
        group_list[i].sink_group = ZGP_PROXY_GROUP_INVALID_IDX;
        group_list[i].alias = 0xffff;
        ret = RET_OK;
      }
    }
  }

  TRACE_MSG(TRACE_ZGP2, "<< zgp_del_group_from_entry %hd", (FMT__H, ret));
  return ret;
}

#ifdef ZB_ENABLE_ZGP_PROXY
static zb_uint16_t zgp_calc_alias_sequence_number(zb_gpc_enum_t command, zb_uint8_t mac_sn)
{
  zb_uint8_t a;

  switch(command)
  {
    case ZB_GPC_GP_DERIVED_GROUPCAST_GP_NOTIFICATION:
    {
      a = mac_sn;
      break;
    }
    case ZB_GPC_GP_PAIRING_SEARCH:
    {
      /* TODO:
         if(from another GP command) - derived from this frame
         else a = random;
      */
      ZB_ASSERT(0);
      break;
    }
    case ZB_GPC_GP_TUNELLING_STOP:
    {
      a = (zb_uint8_t)(mac_sn - 11);
      break;
    }
    case ZB_GPC_GP_COMMISIONING_NOTIFICATION:
    {
      a = (zb_uint8_t)(mac_sn - 12);
      break;
    }
    case ZB_GPC_GP_COMMISSIONED_GROUPCAST_GP_NOTIFICATION:
    {
      a = (zb_uint8_t)(mac_sn - 9);
      break;
    }
    case ZB_GPC_GP_BROADCAST_GP_NOTIFICATION:
    {
      a = (zb_uint8_t)(mac_sn - 14);
      break;
    }
    case ZB_GPC_GP_DEVICE_ANNCE:
    {
      a = 0;
      break;
    }
    default:
      ZB_ASSERT(0);
  }
  TRACE_MSG(TRACE_ZGP2, "zgp_calc_alias_sequence_number (%hd) = %hd", (FMT__H_H, mac_sn, a));
  return a;
}

static void zb_zgp_cluster_gp_comm_notification_req_cb(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZGP2, "zb_zgp_cluster_gp_comm_notification_req_cb: %hd", (FMT__H, param));
  zb_buf_free(param);
}

/**
 * @brief Send GP Commissiong Notification request
 *
 * @param param      [in]  Buffer reference
 *
 */
static void zgp_send_cluster_gp_comm_notification_req(zb_uint8_t param)
{
  zb_gpdf_info_t *gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);
  zb_uint8_t      key_type = gpdf_info->key_type;

  if (gpdf_info->status == GP_DATA_IND_STATUS_UNPROCESSED)
  {
    key_type = ZB_ZGP_SEC_KEY_TYPE_NO_KEY;

    if (ZB_GPDF_EXT_NFC_GET_SEC_KEY(gpdf_info->nwk_ext_frame_ctl))
    {
      key_type = ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL;
    }
  }

/* All proxies form a GP Commissioning Notification message */

  if (ZB_ZGP_PROXY_COMM_MODE_IS_UNICAST())
  {
    zb_zgp_cluster_gp_comm_notification_req(param,
                                            0, /* use alias */
                                            0, /* alias src */
                                            0, /* alias seq */
                                            ZGP_CTXC().comm_data.sink_addr,
                                            ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                            ZB_ZGP_FILL_COMM_NOTIFICATION_OPTIONS(gpdf_info->zgpd_id.app_id,
                                                                                  ZB_GPDF_EXT_NFC_GET_RX_AFTER_TX(gpdf_info->nwk_ext_frame_ctl),
                                                                                  ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl),
                                                                                  key_type,
                                                                                  (gpdf_info->status == GP_DATA_IND_STATUS_UNPROCESSED),
                                                                                  0,1),
                                            zb_zgp_cluster_gp_comm_notification_req_cb);
  }
  else
  {
    zb_uint16_t alias_addr;
    zb_uint8_t  alias_seq;

    alias_addr = zgp_calc_alias_source_address(&gpdf_info->zgpd_id);
    alias_seq = zgp_calc_alias_sequence_number(ZB_GPC_GP_COMMISIONING_NOTIFICATION, gpdf_info->mac_seq_num);
    TRACE_MSG(TRACE_ZGP3, "use aliasing : 0x%lx, %hd", (FMT__D_H, alias_addr, alias_seq));

    zb_zgp_cluster_gp_comm_notification_req(param,
                                            1, /* use alias */
                                            alias_addr, /* alias src */
                                            alias_seq, /* alias seq */
                                            ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE,
                                            ZB_ADDR_16BIT_DEV_OR_BROADCAST,
                                            ZB_ZGP_FILL_COMM_NOTIFICATION_OPTIONS(gpdf_info->zgpd_id.app_id,
                                                                                  ZB_GPDF_EXT_NFC_GET_RX_AFTER_TX(gpdf_info->nwk_ext_frame_ctl),
                                                                                  ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl),
                                                                                  key_type,
                                                                                  (gpdf_info->status == GP_DATA_IND_STATUS_UNPROCESSED),
                                                                                  0,1),
                                            zb_zgp_cluster_gp_comm_notification_req_cb);
  }
}

static void zgp_proxy_handle_commissioning_gpdf(zb_uint8_t param)
{
  zb_gpdf_info_t *gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);
  zb_bool_t       pass_command = ZB_TRUE;

  TRACE_MSG(TRACE_ZGP2, ">> zgp_proxy_handle_commissioning_gpdf %hd", (FMT__H, param));

#ifdef ZB_CERTIFICATION_HACKS
  if (ZGP_CERT_HACKS().gp_proxy_gp_comm_notif_req_cb)
  {
    ZGP_CERT_HACKS().gp_proxy_gp_comm_notif_req_cb(param);
  }
#endif

  if (ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_CHANNEL_CONFIG_ADDED_TO_Q)
  {
    TRACE_MSG(TRACE_ZGP2, "Receive unexpected commissioning command (0x%x) on Transmit channel",
              (FMT__H, gpdf_info->zgpd_cmd_id));
    pass_command = ZB_FALSE;
  }

  if (pass_command)
  {
    if (ZB_GPDF_NFC_GET_FRAME_TYPE(gpdf_info->nwk_frame_ctl) == ZGP_FRAME_TYPE_MAINTENANCE)
    {
      if (gpdf_info->zgpd_cmd_id == ZB_GPDF_CMD_CHANNEL_REQUEST)
      {
        /* If they are in commissioning mode, each proxy forms a GP Commissioning Notification
         * message, with RxAfterTx sub-field of the Options field set to 0b1
         */
        ZB_GPDF_EXT_NFC_SET_RX_AFTER_TX(gpdf_info->nwk_ext_frame_ctl, 1);
      }
    }

    if (ZB_ZGP_PROXY_COMM_MODE_IS_UNICAST())
    {
      /* If the Unicast communication sub-field of the Options field of the GP Proxy
       * Commissioning Mode was set to 0b1, the GP Commissioning Notification is
       * sent as unicast to the originator of the GP Proxy Commissioning Mode
       * command, without alias, i.e. with proxy's own address and sequence number,
       * after Dmin_u.   */
      ZB_SCHEDULE_ALARM(zgp_send_cluster_gp_comm_notification_req, param,
                        (ZB_GPDF_EXT_NFC_GET_RX_AFTER_TX(gpdf_info->nwk_ext_frame_ctl) ?
                         ZB_MILLISECONDS_TO_BEACON_INTERVAL(ZB_GP_DMIN_D_MS):
                         ZB_MILLISECONDS_TO_BEACON_INTERVAL(ZB_GP_DMIN_U_MS)));
    }
    else
    {
      /* The Basic proxy, if the Unicast communication sub-field of the Options field of the GP Proxy
       * Commissioning Mode was set to 0b0, sends the GP Commissioning Notification as broadcast on
       * the operational channel, with alias, after gppTunnelingDelay, and with
       * BidirectionalCommunicationCapability sub-field set to 0b0.
       */
      ZB_SCHEDULE_ALARM(zgp_send_cluster_gp_comm_notification_req, param,
                        (ZB_GPDF_EXT_NFC_GET_RX_AFTER_TX(gpdf_info->nwk_ext_frame_ctl) ?
                         ZB_MILLISECONDS_TO_BEACON_INTERVAL(ZB_GP_DMIN_D_MS):
                         ZB_MILLISECONDS_TO_BEACON_INTERVAL(ZB_GP_DMIN_U_MS)));
    }
  }
  else
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZGP2, "<< zgp_proxy_handle_commissioning_gpdf", (FMT__0));
}

/**
  A.3.5.2.3 Operation of GP Proxy Basic and proxy side of GP Combo Basic
 */
void zb_gp_proxy_data_indication(zb_uint8_t param)
{
  zb_gpdf_info_t *gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);
  zgp_tbl_ent_t   ent;
  zb_bool_t       have_proxy = ZB_FALSE;
  zb_bool_t       rx_after_tx = (zb_bool_t)ZB_GPDF_EXT_NFC_GET_RX_AFTER_TX(gpdf_info->nwk_ext_frame_ctl);

  TRACE_MSG(TRACE_ZGP2, ">> zb_gp_proxy_data_indication param %hd", (FMT__H, param));

  /* maintanence frame has no device id */
  if (ZB_GPDF_NFC_GET_FRAME_TYPE(gpdf_info->nwk_frame_ctl) != ZGP_FRAME_TYPE_MAINTENANCE)
  {
    have_proxy = (zb_bool_t)(zgp_proxy_table_read(&gpdf_info->zgpd_id, &ent) == RET_OK);
  }

  /*
    If the GP-DATA.indication Status is SECURITY_SUCCESS/NO_SECURITY and the GPDF
    is a Data GPDF, independent of whether the Auto-Commissioning flag is set to
    0b0 or 0b1, the Basic Proxy searches its Proxy Table for a matching entry
    related to the received GPD ID (and any Endpoint, if ApplicationID = 0b010).
    If there is any Proxy Table entry for this GPD with the InRange flag set to 0b0
    (even if the GPDfixed flag is also set to 0b1 or if the Endpoint field has
    value other than in the received GPDF), the Basic Proxy sets the InRange flag
    to 0b1.*/
  if (have_proxy && (gpdf_info->status == GP_DATA_IND_STATUS_SECURITY_SUCCESS ||
                     gpdf_info->status == GP_DATA_IND_STATUS_NO_SECURITY) &&
      ZB_GPDF_NFC_GET_FRAME_TYPE(gpdf_info->nwk_frame_ctl) == ZGP_FRAME_TYPE_DATA &&
      !ZGP_TBL_GET_INRANGE(&ent))
  {
    TRACE_MSG(TRACE_ZGP3, "Set InRange for this ZGPD in Proxy table", (FMT__0));
    /* Must update InRange.
       No ideas how it used for Basic Proxy. Seems, just by read attribute.
    */
    ZGP_TBL_SET_INRANGE(&ent);
    zgp_proxy_table_write(&gpdf_info->zgpd_id, &ent);
  }

  if (ZGP_IS_PROXY_IN_COMMISSIONING_MODE())
  {
    if (gpdf_info->status == GP_DATA_IND_STATUS_UNPROCESSED ||
        gpdf_info->status == GP_DATA_IND_STATUS_COMMISSIONING_UNPROCESSED)
    {
      TRACE_MSG(TRACE_ZGP3, "Proxy in commissioning mode - transfer unprocessed frame", (FMT__0));
      zgp_proxy_handle_commissioning_gpdf(param);
      param = 0;
    }
    else
    if (gpdf_info->status == GP_DATA_IND_STATUS_NO_SECURITY &&
        (gpdf_info->zgpd_cmd_id >= ZB_GPDF_CMD_MANUF_DEFINED_B0 &&
         gpdf_info->zgpd_cmd_id <= ZB_GPDF_CMD_MANUF_DEFINED_BF))
    {
      TRACE_MSG(TRACE_ZGP3, "Proxy in commissioning mode - transfer manuf-defined frame", (FMT__0));
      zgp_proxy_handle_commissioning_gpdf(param);
      param = 0;
    }
    else
    if (gpdf_info->zgpd_cmd_id < ZB_GPDF_CMD_COMMISSIONING &&
        !(ZB_GPDF_NFC_GET_AUTO_COMMISSIONING(gpdf_info->nwk_frame_ctl) &&
          ZB_GPDF_NFC_GET_FRAME_TYPE(gpdf_info->nwk_frame_ctl) == ZGP_FRAME_TYPE_DATA))
    {
      TRACE_MSG(TRACE_ZGP3, "Proxy in commissioning mode - drop data GPDF", (FMT__0));
      zb_buf_free(param);
      param = 0;
    }
    else
    {
      TRACE_MSG(TRACE_ZGP3, "Proxy in commissioning mode - process commissioning or autocommissiong command", (FMT__0));
      zgp_proxy_handle_commissioning_gpdf(param);
      param = 0;
    }
  }

  if (param && !have_proxy)
  {
    TRACE_MSG(TRACE_ZGP3, "Proxy in operational mode and not have a proxy table entry for this GPD - drop.", (FMT__0));
    zb_buf_free(param);
    param = 0;
  }

  if (param)
  {
    if (gpdf_info->zgpd_cmd_id == ZB_GPDF_CMD_COMMISSIONING)
    {
      /* EES: in operational mode */
      ZB_GPDF_EXT_NFC_CLR_RX_AFTER_TX(gpdf_info->nwk_ext_frame_ctl);
      /* EES: pass Commissioning GPDF to all paired sinks despite security & freshness */
      TRACE_MSG(TRACE_ZGP3, "pass commissioning frame in operational mode to all paired sinks",
                (FMT__0));
    }
    else
    if (gpdf_info->status == GP_DATA_IND_STATUS_SECURITY_SUCCESS || gpdf_info->status == GP_DATA_IND_STATUS_NO_SECURITY)
    {
      if (gpdf_info->zgpd_cmd_id == ZB_GPDF_CMD_DECOMMISSIONING)
      {
        TRACE_MSG(TRACE_ZGP3, "pass decommissioning frame in operational mode to all paired sinks",
                (FMT__0));
      }
      else
      if (gpdf_info->zgpd_cmd_id < ZB_GPDF_CMD_COMMISSIONING)
      {
        TRACE_MSG(TRACE_ZGP3, "pass data frame (0x%x) in operational mode to all paired sinks",
                (FMT__H, gpdf_info->zgpd_cmd_id));
      }
      else
      {
      TRACE_MSG(TRACE_ZGP3, "Receive commissioning command (0x%x) in operational mode",
                (FMT__H, gpdf_info->zgpd_cmd_id));
        zb_buf_free(param);
        param = 0;
    }
    }
    else
    {
      TRACE_MSG(TRACE_ZGP3, "GPDF (0x%x) with status %hd in operational mode",
                (FMT__H_H, gpdf_info->zgpd_cmd_id, gpdf_info->status));
      zb_buf_free(param);
      param = 0;
    }

    if (param)
    {
      ZB_SCHEDULE_ALARM(zgp_proxy_send_gp_notification_req, param,
                        (rx_after_tx ?
                         ZB_MILLISECONDS_TO_BEACON_INTERVAL(ZB_GP_DMIN_D_MS):
                         ZB_MILLISECONDS_TO_BEACON_INTERVAL(ZB_GP_DMIN_U_MS)));
      param = 0;
    }
  }

  if (param)
  {
    TRACE_MSG(TRACE_ZGP3, "Not handled frame - drop it", (FMT__0));
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_gp_proxy_data_indication", (FMT__0));
}

static void zb_zgp_proxy_commissioning_mode_expired(zb_uint8_t param);

static void zb_zgp_proxy_leave_from_commissioning_mode()
{
  TRACE_MSG(TRACE_ZGP2, ">> zb_zgp_proxy_leave_from_commissioning_mode", (FMT__0));

  if (ZGP_IS_PROXY_IN_COMMISSIONING_MODE())
  {
    TRACE_MSG(TRACE_ZGP2, "Proxy leaving commissioning mode", (FMT__0));
    ZGP_CTXC().proxy_mode = ZB_ZGP_OPERATIONAL_MODE;
    ZB_SCHEDULE_ALARM_CANCEL(zb_zgp_proxy_commissioning_mode_expired, 0);
  }
  TRACE_MSG(TRACE_ZGP2, "<< zb_zgp_proxy_leave_from_commissioning_mode", (FMT__0));
}

static void zb_zgp_proxy_commissioning_mode_expired(zb_uint8_t param)
{
  ZVUNUSED(param);
  zb_zgp_proxy_leave_from_commissioning_mode();
}

void zgp_proxy_handle_commissioning_mode(zb_uint8_t  param,
                                         zb_uint8_t  options,
                                         zb_uint16_t comm_wind,
                                         zb_uint8_t  channel)
{
  zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);

  TRACE_MSG(TRACE_ZGP2, "zgp_proxy_handle_commissioning_mode %hd", (FMT__H, param));

#ifdef ZB_ENABLE_ZGP_SINK
  if (ZGP_IS_SINK_IN_COMMISSIONING_MODE())
  {
    TRACE_MSG(TRACE_ZGP3, "Sink part of combo already in commissioning mode, drop.", (FMT__0));
    zb_buf_free(param);
    param = 0;
  }
#endif  /* ZB_ENABLE_ZGP_SINK */

  if (param && ZGP_IS_PROXY_IN_COMMISSIONING_MODE())
  {
    TRACE_MSG(TRACE_ZGP3, "proxy already in commissioning mode for %d, cmd recv from %d",
              (FMT__D_D, ZGP_CTXC().comm_data.sink_addr,
                cmd_info->addr_data.common_data.source.u.short_addr));
/* A.3.9.1.2
  While in commissioning mode, the proxies shall only accept
  GP Proxy Commissioning Mode commands from the device that originally put them
  in commissioning mode, and shall silently drop
  GP Proxy Commissioning Mode commands from other devices.
*/
    if (ZGP_CTXC().comm_data.sink_addr != cmd_info->addr_data.common_data.source.u.short_addr)
    {
      TRACE_MSG(TRACE_ZGP1, "proxy already in commissioning mode for %d, but cmd recv from %d, drop packet",
              (FMT__D_D, ZGP_CTXC().comm_data.sink_addr,
                cmd_info->addr_data.common_data.source.u.short_addr));
      zb_buf_free(param);
      param = 0;
    }
  }

  if (param)
  {
    if (ZB_ZGP_COMM_MODE_OPT_GET_ACTION(options) == ZGP_PROXY_COMM_MODE_ENTER)
    {
      zb_uint16_t comm_window_timeout = ZB_ZGP_DEFAULT_COMMISSIONING_WINDOW;

      ZB_BZERO(&ZGP_CTXC().comm_data, sizeof(ZGP_CTXC().comm_data));
      ZGP_CTXC().proxy_mode = ZB_ZGP_COMMISSIONING_MODE;
      ZGP_CTXC().comm_data.proxy_comm_mode_options = (options >> 1); //skip action bit field
      ZGP_CTXC().comm_data.sink_addr = cmd_info->addr_data.common_data.source.u.short_addr;
      TRACE_MSG(TRACE_ZGP2, "Proxy entering commissioning mode, Sink 0x%x, is unicast %hd",
                (FMT__D_H, cmd_info->addr_data.common_data.source.u.short_addr,
                 ZB_ZGP_PROXY_COMM_MODE_INT_OPT_GET_UNICAST_COMMUNICATION(
                   ZGP_CTXC().comm_data.proxy_comm_mode_options)));

      /* handle commissioning window */
      /* A.3.3.5.3 The CommissioningWindow field SHALL be present, if the On Commissioning Window
       * expiration flag of the Exit mode sub-field is set to 0b1. It carries the value of
       * gpsCommissioningWindow attribute (see A.3.3.2.5), which overrides - for this particular
       * commissioning operation - the default gppCommissioningWindow value (see A.3.6.3.2) of the
       * receiving proxy.
       */
      if (ZB_ZGP_COMM_MODE_OPT_GET_ON_COMM_WIND_EXP(options))
      {
        comm_window_timeout = comm_wind;
      }

      /* GP Commissioning procedure. The proxies receiving a GP Proxy Commissioning Mode
       * (Action=enter) command on the operational channel (if sent) in operational mode SHALL start
       * the CommissioningWindow/gppCommissioningWindow timeout (see sec. A.3.3.2.5/A.3.6.3.2)
       * to exit commissioning mode in case of no pairing/no explicit exit command
       */
      ZB_SCHEDULE_ALARM(zb_zgp_proxy_commissioning_mode_expired, 0,
                        comm_window_timeout * ZB_TIME_ONE_SECOND);

      /* handle channel */
      /* A.3.3.5.3 In the current version of the GP specification, the Channel present sub-field SHALL always
       * be set to 0b0 and the Channel field SHALL NOT be present.
       */
    }
    else
    {
      zb_zgp_proxy_leave_from_commissioning_mode();
    }
  }

  if (param)
  {
    zb_buf_free(param);
  }

  ZVUNUSED(channel);
}

void zb_gp_proxy_mlme_set_cfm_cb(zb_uint8_t param)
{
  zb_mlme_set_confirm_t *cfm = (zb_mlme_set_confirm_t *)zb_buf_begin(param);

  TRACE_MSG(TRACE_ZGP2, ">> zb_gp_proxy_mlme_set_cfm_cb %hd", (FMT__H, param));

  ZB_ASSERT(cfm->pib_attr == ZB_PHY_PIB_CURRENT_CHANNEL);

  if (cfm->status == MAC_SUCCESS)
  {
    if (ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_CHANNEL_CONFIG_SET_TEMP_CHANNEL)
    {
      TRACE_MSG(TRACE_ZGP2, "Proxy switch to temp master channel", (FMT__0));
      zgp_channel_config_add_to_queue(param, ZGP_CTXC().comm_data.channel_conf_payload);
      param = 0;
    }
    else
    {
      TRACE_MSG(TRACE_ZGP2, "Proxy back to operational channel", (FMT__0));
      ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_IDLE);
    }
  }
  else
  {
    if (ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_CHANNEL_CONFIG_SET_TEMP_CHANNEL)
    {
      TRACE_MSG(TRACE_ZGP2, "Proxy failed switch to temp master channel", (FMT__0));
    }
    else
    {
      TRACE_MSG(TRACE_ZGP2, "Proxy failed back to operational channel", (FMT__0));
    }
    ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_IDLE);
  }

  if (param)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_gp_proxy_mlme_set_cfm_cb", (FMT__0));
}

void zb_gp_proxy_mlme_get_cfm_cb(zb_uint8_t param)
{
  zb_mlme_get_confirm_t *cfm = (zb_mlme_get_confirm_t *)zb_buf_begin(param);

  TRACE_MSG(TRACE_ZGP2, ">> zb_gp_proxy_mlme_get_cfm_cb %hd", (FMT__H, param));

  ZB_ASSERT(cfm->pib_attr == ZB_PHY_PIB_CURRENT_CHANNEL);

  if (cfm->status == MAC_SUCCESS)
  {
    if (ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_CHANNEL_CONFIG_GET_CUR_CHANNEL)
    {
      if (ZGP_CTXC().comm_data.oper_channel != ZGP_CTXC().comm_data.temp_master_tx_chnl)
      {
      ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_CHANNEL_CONFIG_SET_TEMP_CHANNEL);
      zgp_channel_config_transceiver_channel_change(param, ZB_FALSE);
      }
      else
      {
        TRACE_MSG(TRACE_ZGP3, "Proxy already on TX channel", (FMT__0));
        zgp_channel_config_add_to_queue(param, ZGP_CTXC().comm_data.channel_conf_payload);
      }
      param = 0;
    }
  }
  else
  {
    TRACE_MSG(TRACE_ZGP2, "Proxy failed get operational channel value", (FMT__0));
    ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_IDLE);
  }

  if (param)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_gp_proxy_mlme_get_cfm_cb", (FMT__0));
}

/**
 * @brief Prepare runtime context for sending channel configuration packet
 *
 * @param param      [in]  Buffer reference
 *
 */
static void zgp_proxy_handle_channel_configuration(zb_uint8_t param)
{
  zb_zgp_gp_response_t *resp = ZB_BUF_GET_PARAM(param, zb_zgp_gp_response_t);

  TRACE_MSG(TRACE_ZGP2, ">> zgp_proxy_handle_channel_configuration %hd", (FMT__H, param));

  if (ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_IDLE)
  {
    /* Update commissioning data */
    ZGP_CTXC().comm_data.zgpd_id.app_id = ZB_ZGP_GP_RESPONSE_OPT_GET_APP_ID(resp->options);
    ZGP_CTXC().comm_data.zgpd_id.endpoint = resp->endpoint;
    ZB_MEMCPY(&ZGP_CTXC().comm_data.zgpd_id.addr, &resp->zgpd_addr, sizeof(zb_zgpd_addr_t));
    ZGP_CTXC().comm_data.temp_master_tx_chnl = ZB_ZGPD_FIRST_CH + resp->temp_master_tx_chnl;
    ZGP_CTXC().comm_data.channel_conf_payload = resp->payload[1];

    ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_CHANNEL_CONFIG_GET_CUR_CHANNEL);

    /* Get current channel */
    zb_zgp_channel_config_get_current_channel(param);
  }
  else
  {
    TRACE_MSG(TRACE_ZGP2, "Proxy already process other command", (FMT__0));
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZGP2, "<< zgp_proxy_handle_channel_configuration", (FMT__0));
}

/**
 * @brief Prepare and put GPDF packet into TX queue
 *
 * @param param      [in]  Buffer reference
 *
 */
static void zgp_proxy_put_gpdf_in_tx_queue(zb_uint8_t param)
{
  zb_zgp_gp_response_t  resp;
  zb_gp_data_req_t     *req;

  TRACE_MSG(TRACE_ZGP2, ">> zgp_proxy_put_gpdf_in_tx_queue, param %hd", (FMT__H, param));

  ZB_MEMCPY(&resp, ZB_BUF_GET_PARAM(param, zb_zgp_gp_response_t),
            sizeof(zb_zgp_gp_response_t));

  req = zb_buf_initial_alloc(param,
                             sizeof(zb_gp_data_req_t)-sizeof(req->pld)+resp.payload[0]);

  switch (resp.gpd_cmd_id)
  {
    case ZB_GPDF_CMD_COMMISSIONING_REPLY:
      req->handle = ZB_ZGP_HANDLE_ADD_COMMISSIONING_REPLY;
      break;
    default:
      req->handle = ZB_ZGP_HANDLE_DEFAULT_HANDLE;
  };

  req->action = ZB_GP_DATA_REQ_ACTION_ADD_GPDF;
  req->tx_options = ZB_GP_DATA_REQ_USE_GP_TX_QUEUE;
  req->cmd_id = resp.gpd_cmd_id;
  req->payload_len = resp.payload[0];

  ZB_ASSERT(resp.payload[0] <= ZB_ZGP_TX_CMD_PLD_MAX_SIZE);
  ZB_MEMCPY(req->pld, &resp.payload[1], resp.payload[0]);

  if ((ZB_ZGP_GP_RESPONSE_OPT_GET_APP_ID(resp.options) == ZB_ZGP_APP_ID_0000
       && resp.zgpd_addr.src_id != 0)
      || (ZB_ZGP_GP_RESPONSE_OPT_GET_APP_ID(resp.options) == ZB_ZGP_APP_ID_0010
          && !ZB_IEEE_ADDR_IS_ZERO(resp.zgpd_addr.ieee_addr)))
  {
    req->zgpd_id.app_id = ZB_ZGP_GP_RESPONSE_OPT_GET_APP_ID(resp.options);
    req->zgpd_id.endpoint = resp.endpoint;
    if (ZB_ZGP_GP_RESPONSE_OPT_GET_APP_ID(resp.options) == ZB_ZGP_APP_ID_0000)
    {
      req->zgpd_id.addr.src_id = resp.zgpd_addr.src_id;
    }
    else
    {
      ZB_IEEE_ADDR_COPY(req->zgpd_id.addr.ieee_addr, resp.zgpd_addr.ieee_addr);
    }
  }
  else if (!ZB_ZGPD_IS_SPECIFIED(&ZGP_CTXC().comm_data.zgpd_id))
  {
    req->zgpd_id.app_id = ZB_ZGP_APP_ID_0000;
    req->zgpd_id.endpoint = 0;
    req->zgpd_id.addr.src_id = ZB_ZGP_SRC_ID_ALL;
  }
  else
  {
    ZB_MEMCPY(&req->zgpd_id, &ZGP_CTXC().comm_data.zgpd_id, sizeof(zb_zgpd_id_t));
  }

  req->tx_q_ent_lifetime = ZB_ZGP_CHANNEL_REQ_ON_TX_CH_TIMEOUT;

  schedule_gp_txdata_req(param);

  TRACE_MSG(TRACE_ZGP2, "<< zgp_proxy_put_gpdf_in_tx_queue", (FMT__0));
}

/**
 * @brief Update FirstToForward flag for relevant proxy entry
 *
 * @param param      [in]  ZGPD ID
 * @param value      [in]  first to forward new value
 *
 */
static void zgp_proxy_update_first_to_forward(zb_zgpd_id_t *zgpd_id, zb_uint8_t value)
{
  zb_zgp_proxy_tbl_ent_t ent;

  if (zgp_proxy_table_read(zgpd_id, &ent) == RET_OK)
  {
    ZGP_TBL_RUNTIME_FIRST_TO_FORWARD_UPDATE(&ent, value);
  }
}

void zgp_proxy_handle_gp_response(zb_uint8_t param)
{
  zb_zgp_gp_response_t *resp = ZB_BUF_GET_PARAM(param, zb_zgp_gp_response_t);

  TRACE_MSG(TRACE_ZGP2, ">> zgp_proxy_handle_gp_response %hd", (FMT__H, param));
/*
  On receipt of a GP Response frame from sink in groupcast, both in operational and commissioning mode, the proxy checks
  if its short address matches the value in the TempMaster short address field.

  If yes and also if the GP Response command was sent to this proxy in unicast, the proxy adds the GPDF frame derived from the GP Response frame to its gpTxQueue
  for sending to the indicated GPD ID (and Endpoint, if ApplicationID = 0b010) by calling GP-DATA.request with Action parameter set to TRUE with
  bit5 of the TxOptions set to the value of the Tx on matching endpoint sub-field of the Options field
  of the GP Response command, and sets its FirstToForward flag for this GPD to 0b1.
*/
  if (resp->temp_master_addr == ZB_PIBCACHE_NETWORK_ADDRESS())
  {
    switch (resp->gpd_cmd_id)
    {
      case ZB_GPDF_CMD_CHANNEL_CONFIGURATION:
      {
        zgp_proxy_handle_channel_configuration(param);
        break;
      }
      default:
        zgp_proxy_put_gpdf_in_tx_queue(param);
    };
  }
/*
  If the TempMaster short address field of the GP Response command carries an address different than the short address of the receiving proxy,
  the proxy drops the current command, sets the FirstToForward flag for the relevant Proxy Table entry
  to 0b0, and proceeds as follows.

  If ApplicationID sub-field of the GP Response command is set to 0b000, the proxy removes any previous pending GPDF for this GPD from its gpTxQueue
  by calling GP-DATA.request with the Action parameter set to FALSE, and sets the FirstToForward flag for this SrcID in its Proxy Table to 0b0.

  If ApplicationID sub-field of the GP Response command is set to 0b010, the proxy instructs the dGP
  stub to remove pending relevant GPDF for this GPD IEEE address (see sec. A.1.3.2.3) from its gpTxQueue by calling GP-DATA.request with
  the Action parameter set to FALSE, bit5 of the TxOptions set to the value of the Tx on matching endpoint sub-field of the Options field
  of the GP Response command, and the GPD IEEE address and GPD Endpoint copied from the GP Response; and sets the FirstToForward flag for this GPD in its Proxy Table to 0b0.
*/
  else
  {
    zb_zgpd_id_t zgpd_id;

    TRACE_MSG(TRACE_ZGP2, "proxy handle gp_response for other temp master %d, drop buffer",
              (FMT__D, resp->temp_master_addr));

    ZB_MAKE_ZGPD_ID(zgpd_id,
                    ZB_ZGP_GP_RESPONSE_OPT_GET_APP_ID(resp->options),
                    resp->endpoint,
                    resp->zgpd_addr);

    zgp_proxy_update_first_to_forward(&zgpd_id, 0);
    zgp_clean_zgpd_info_from_queue(param, &zgpd_id, ZB_ZGP_HANDLE_DEFAULT_HANDLE);
  }

  TRACE_MSG(TRACE_ZGP2, "<< zgp_proxy_handle_gp_response", (FMT__0));
}

/**
 * @brief Init new proxy entry for paring ZGPD with sink
 *
 * @param ent  [in]  New proxy entry
 * @param req  [in]  Pointer to the GP Paring request information
 *
 */
static void zgp_proxy_entry_init(zb_zgp_proxy_tbl_ent_t  *ent,
                                 zb_zgp_gp_pairing_req_t *req)
{
  zb_uindex_t i;

  TRACE_MSG(TRACE_ZGP2, ">> zgp_proxy_entry_init", (FMT__0));

  ZB_BZERO(ent, sizeof(zb_zgp_proxy_tbl_ent_t));

  /* Fill proxy entry tables with predefined invalid values */
  for (i = 0; i < ZB_ZGP_MAX_LW_UNICAST_ADDR_PER_GPD; i++)
  {
    ent->u.proxy.lwsaddr[i].addr_ref = ZGP_PROXY_LWADDR_INVALID_IDX;
  }
  for (i = 0; i < ZB_ZGP_MAX_SINK_GROUP_PER_GPD; i++)
  {
    ent->u.proxy.sgrp[i].sink_group = ZGP_PROXY_GROUP_INVALID_IDX;
    ent->u.proxy.sgrp[i].alias = ZGP_PROXY_GROUP_DERIVED_ALIAS;
  }

  ent->options = ZGP_TBL_PROXY_FILL_OPTIONS(
    ZB_ZGP_PAIRING_OPT_GET_APP_ID(req->options),
    1,/*EntryActive*/
    1,/*EntryValid*/
    ZB_ZGP_PAIRING_OPT_GET_SEQ_NUM_CAP(req->options),
    (ZB_ZGP_PAIRING_OPT_GET_COMMUNICATION_MODE(req->options) == ZGP_COMMUNICATION_MODE_LIGHTWEIGHT_UNICAST),
    (ZB_ZGP_PAIRING_OPT_GET_COMMUNICATION_MODE(req->options) == ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED),
    (ZB_ZGP_PAIRING_OPT_GET_COMMUNICATION_MODE(req->options) == ZGP_COMMUNICATION_MODE_GROUPCAST_PRECOMMISSIONED),
    0,/*FirstToForward*/
    0,/*InRange*/
    ZB_ZGP_PAIRING_OPT_GET_FIX_LOC(req->options),
    0,/*HasAllUnicastRoutes*/
    (ZB_ZGP_PAIRING_OPT_GET_ASSIGNED_ALIAS_PRESENT(req->options) &&
      ZB_ZGP_PAIRING_OPT_GET_COMMUNICATION_MODE(req->options) == ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED),
    (ZB_ZGP_PAIRING_OPT_GET_SEC_LEVEL(req->options) > ZB_ZGP_SEC_LEVEL_NO_SECURITY),
    0 /*OptionsExtension*/
    );

  ent->sec_options = ZGP_TBL_FILL_SEC_OPTIONS(ZB_ZGP_PAIRING_OPT_GET_SEC_LEVEL(req->options),
                                              ZB_ZGP_PAIRING_OPT_GET_KEY_TYPE(req->options));

  ent->zgpd_id = req->zgpd_addr;
  ent->security_counter = req->sec_frame_counter;

  if (ZB_ZGP_PAIRING_OPT_GET_APP_ID(req->options) == ZB_ZGP_APP_ID_0010)
  {
    ent->endpoint = req->endpoint;
  }

  if (ZB_ZGP_PAIRING_OPT_GET_SEC_LEVEL(req->options) > ZB_ZGP_SEC_LEVEL_NO_SECURITY)
  {
    TRACE_MSG(TRACE_ZGP3, "key: " TRACE_FORMAT_128, (FMT__A_A, TRACE_ARG_128(req->key)));
    ZB_MEMCPY(ent->zgpd_key, req->key, ZB_CCM_KEY_SIZE);
  }

  TRACE_MSG(TRACE_ZGP2, "<< zgp_proxy_entry_init", (FMT__0));
}

static zb_ret_t zgp_proxy_add_lwsink_addr_ref(zgp_tbl_ent_t *ent, zb_address_ieee_ref_t addr_ref)
{
  zb_uint8_t i;
  zb_uint8_t free_slot = 0xff;
  zb_bool_t  add = ZB_TRUE;
  zb_ret_t   ret = RET_ALREADY_EXISTS;

  TRACE_MSG(TRACE_ZGP2, ">> zgp_proxy_add_lwsink_addr_ref: %d", (FMT__D, addr_ref));

  for (i = 0; i < ZB_ZGP_MAX_LW_UNICAST_ADDR_PER_GPD; i++)
  {
    TRACE_MSG(TRACE_ZGP3, "addr_ref[%hd]: %d", (FMT__H_D, i, ent->u.proxy.lwsaddr[i].addr_ref));
    //search that table already have this addr ref
    if (ent->u.proxy.lwsaddr[i].addr_ref == addr_ref)
    {
      add = ZB_FALSE;
      break;
    }

    if (ent->u.proxy.lwsaddr[i].addr_ref == ZGP_PROXY_LWADDR_INVALID_IDX &&
        free_slot == 0xff)
    {
      //we found free slot, remember it
      free_slot = i;
    }
  }

  if (add)
  {
    if (free_slot != 0xff)
    {
      ent->u.proxy.lwsaddr[free_slot].addr_ref = addr_ref;
      TRACE_MSG(TRACE_ZGP2, "add addr_ref: %d in proxy lwsaddr table", (FMT__D, addr_ref));
      ret = RET_OK;
    }
    else
    {
      TRACE_MSG(TRACE_ERROR, "No free space in lwsaddr table", (FMT__0));
      ret = RET_TABLE_FULL;
    }
  }

  TRACE_MSG(TRACE_ZGP2, "<< zgp_proxy_add_lwsink_addr_ref %hd", (FMT__H, ret));
  return ret;
}

static zb_ret_t zgp_proxy_del_lwsink_addr_ref(zgp_tbl_ent_t *ent, zb_address_ieee_ref_t addr_ref)
{
  zb_uint8_t i;
  zb_ret_t   ret = RET_NOT_FOUND;

  TRACE_MSG(TRACE_ZGP2, ">> zgp_proxy_del_lwsink_addr_ref", (FMT__0));

  for (i = 0; i < ZB_ZGP_MAX_LW_UNICAST_ADDR_PER_GPD; i++)
  {
    //search that table already have this addr ref
    if (ent->u.proxy.lwsaddr[i].addr_ref == addr_ref)
    {
      ent->u.proxy.lwsaddr[i].addr_ref = ZGP_PROXY_LWADDR_INVALID_IDX;
      ret = RET_OK;
      break;
    }
  }

  TRACE_MSG(TRACE_ZGP2, "<< zgp_proxy_del_lwsink_addr_ref %hd", (FMT__H, ret));
  return ret;
}

zb_uint8_t zgp_proxy_get_lwsink_addr_list_size(zgp_tbl_ent_t *ent)
{
  zb_uint8_t ret = 0;
  zb_uint8_t i;

  for (i = 0; i < ZB_ZGP_MAX_LW_UNICAST_ADDR_PER_GPD; i++)
  {
    if (ent->u.proxy.lwsaddr[i].addr_ref != 0xff)
    {
      ret++;
    }
  }
  return ret;
}

static void zgp_pairing_reverse_ieee(zb_uint8_t *ptr, zb_uint8_t *val)
{
  zb_uint8_t i;

  for (i = 0; i < sizeof(zb_ieee_addr_t); i++)
  {
    ptr[i] = val[sizeof(zb_ieee_addr_t) - 1 - i];
  }
}

/**
 * @brief Update proxy entry for paring ZGPD with sink
 *
 * @param ent  [in]  Existing proxy entry
 * @param req  [in]  Pointer to the GP Paring request information
 *
 */
static zb_ret_t zgp_proxy_entry_update(zb_zgp_proxy_tbl_ent_t  *ent,
                                         zb_zgp_gp_pairing_req_t *req)
{
  zb_ret_t ret = RET_ALREADY_EXISTS;

  TRACE_MSG(TRACE_ZGP2, ">> zgp_proxy_entry_update", (FMT__0));

  switch (ZB_ZGP_PAIRING_OPT_GET_COMMUNICATION_MODE(req->options))
  {
    case ZGP_COMMUNICATION_MODE_FULL_UNICAST:
    case ZGP_COMMUNICATION_MODE_LIGHTWEIGHT_UNICAST:
    {
      zb_address_ieee_ref_t short_addr_ref = 0xff;
      zb_address_ieee_ref_t long_addr_ref = 0xff;
      zb_address_ieee_ref_t addr_ref = 0xff;
      zb_ieee_addr_t        reverse;

      zgp_pairing_reverse_ieee(reverse, req->sink_ieee_addr);

      TRACE_MSG(TRACE_ZGP2, "short: 0x%x", (FMT__D, req->sink_nwk_addr));
      TRACE_MSG(TRACE_ZGP2, "ieee: " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(req->sink_ieee_addr)));
      TRACE_MSG(TRACE_ZGP2, "reverse: " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(reverse)));

      if (zb_address_by_short(req->sink_nwk_addr, ZB_FALSE, ZB_FALSE, &short_addr_ref) == RET_OK &&
          (zb_address_by_ieee(req->sink_ieee_addr, ZB_FALSE, ZB_FALSE, &long_addr_ref) == RET_OK ||
           zb_address_by_ieee(reverse, ZB_FALSE, ZB_FALSE, &long_addr_ref) == RET_OK) &&
          short_addr_ref == long_addr_ref)
      {
        addr_ref = short_addr_ref;
      }
      else
      {
        TRACE_MSG(TRACE_ZGP2, "short_addr_ref: %hd long_addr_ref: %hd", (FMT__H_H, short_addr_ref, long_addr_ref));
      }

      if (short_addr_ref == 0xff && long_addr_ref == 0xff)
        {
        /* EES: Let's create new zb_address record for this short-long pair.
         * At least it's needed for certification test 5.4.2.5 */
        if (zb_address_update(req->sink_ieee_addr, req->sink_nwk_addr, ZB_TRUE, &addr_ref) != RET_OK)
        {
          TRACE_MSG(TRACE_ERROR, "failed create new zb_address record, skip pairing", (FMT__0));
        }
      }

      if (addr_ref != 0xff)
      {
        TRACE_MSG(TRACE_ZGP3, "addr_ref: %hd", (FMT__H, addr_ref));
        ZGP_TBL_SET_LWUC(ent, 1);
        ret = zgp_proxy_add_lwsink_addr_ref(ent, addr_ref);
      }
      else
      {
        TRACE_MSG(TRACE_ERROR, "failed get addr_ref, skip pairing", (FMT__0));
        ret = RET_ERROR;
      }
      break;
    }
    case ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED:
    case ZGP_COMMUNICATION_MODE_GROUPCAST_PRECOMMISSIONED:
    {
      TRACE_MSG(TRACE_ZGP3, "sink_group_id: 0x%lx", (FMT__D, req->sink_group_id));

      switch (ZB_ZGP_PAIRING_OPT_GET_COMMUNICATION_MODE(req->options))
      {
        case ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED:
        {
          if (!ZGP_TBL_GET_DGGC(ent))
          {
            TRACE_MSG(TRACE_ZGP3, "set derived groupcast communication mode", (FMT__0));
            ZGP_TBL_SET_DGGC(ent, 1);
            ret = RET_OK;
          }

            if (ZB_ZGP_PAIRING_OPT_GET_ASSIGNED_ALIAS_PRESENT(req->options))
            {
              ent->zgpd_assigned_alias = req->assigned_alias;
              ZGP_TBL_PROXY_SET_ASSIGNED_ALIAS(ent);
              TRACE_MSG(TRACE_ZGP3, "assigned_alias: 0x%x", (FMT__D, req->assigned_alias));
            ret = RET_OK;
            }
          break;
        }
        case ZGP_COMMUNICATION_MODE_GROUPCAST_PRECOMMISSIONED:
        {
          if (ZB_ZGP_PAIRING_OPT_GET_ASSIGNED_ALIAS_PRESENT(req->options))
          {
            TRACE_MSG(TRACE_ZGP3, "Precommissioned Groupcast Pairing with assigned alias 0x%x",
                      (FMT__D, req->assigned_alias));

            ret = zgp_add_group_alias_for_entry(ent->u.proxy.sgrp,
                                              req->sink_group_id,
                                                req->assigned_alias);
            }
          else
          {
            zb_zgpd_id_t zgpd_id;
            zb_uint16_t  derived_alias;

            ZB_MAKE_ZGPD_ID(zgpd_id,
                            ZB_ZGP_PAIRING_OPT_GET_APP_ID(req->options),
                            req->endpoint,
                            req->zgpd_addr);

            derived_alias = zgp_calc_alias_source_address(&zgpd_id);

            TRACE_MSG(TRACE_ZGP3, "Precommissioned Groupcast Pairing with derived alias 0x%x",
                      (FMT__D, derived_alias));
            ret = zgp_add_group_alias_for_entry(ent->u.proxy.sgrp,
                                                req->sink_group_id,
                                                derived_alias);
          }
          if (ret == RET_OK)
          {
            if (!ZGP_TBL_GET_CGGC(ent))
            {
              TRACE_MSG(TRACE_ZGP3, "set commissioned groupcast communication mode", (FMT__0));
              ZGP_TBL_SET_CGGC(ent, 1);
              ret = RET_OK;
            }
          }
          break;
        }
      };
      break;
    }
      default:
        ZB_ASSERT(0);
  };

  if ((ret == RET_ALREADY_EXISTS || ret == RET_OK) && ZB_ZGP_PAIRING_OPT_GET_FRWD_RADIUS(req->options))
  {
    /* A.3.4.2.2.2.7 Groupcast radius parameter */
    /* If Groupcast radius parameter is set to a value 0x00 and another value is received, the new
     * value SHALL be kept. If Groupcast radius parameter is set to a value other than 0x00 and a new
     * value is received, the higher value SHALL be kept.
     */
    if (!ent->groupcast_radius || (req->frwd_radius > ent->groupcast_radius))
    {
      ent->groupcast_radius = req->frwd_radius;
      ret = RET_OK;
      TRACE_MSG(TRACE_ZGP3, "update groupcast radius: %hd", (FMT__H, req->frwd_radius));
    }
  }

  if ((ret == RET_ALREADY_EXISTS || ret == RET_OK) && ZB_ZGP_PAIRING_OPT_GET_FRAME_CNT_PRESENT(req->options))
  {
    if (req->sec_frame_counter != ent->security_counter)
    {
      ent->security_counter = req->sec_frame_counter;
      ret = RET_OK;
      TRACE_MSG(TRACE_ZGP3, "update security frame counter: 0x%x", (FMT__D, req->sec_frame_counter));
}
  }

  if ((ret == RET_ALREADY_EXISTS || ret == RET_OK) && ZGP_TBL_GET_INRANGE(ent))
  {
    /* EES: Reset InRange flag after pairing request */
    ZGP_TBL_CLR_INRANGE(ent);
    ret = RET_OK;
    TRACE_MSG(TRACE_ZGP3, "reset InRange flag", (FMT__0));
  }

  TRACE_MSG(TRACE_ZGP2, "<< zgp_proxy_entry_update, ret: %hd", (FMT__H, ret));
  return ret;
}

/**
 * @brief Perform add paring ZGPD with sink
 *
 * @param req    [in]  Pointer to the GP Paring request information
 *
 * @see ZGP spec, A.3.3.5.2
 */
static zb_ret_t zgp_proxy_add_pairing(zb_zgp_gp_pairing_req_t *req)
{
  zb_zgp_proxy_tbl_ent_t   ent;
  zb_zgpd_id_t             zgpd_id;
  zb_ret_t                 ret = RET_ALREADY_EXISTS;

  TRACE_MSG(TRACE_ZGP2, ">> zgp_proxy_add_pairing", (FMT__0));

  ZB_MAKE_ZGPD_ID(zgpd_id,
                  ZB_ZGP_PAIRING_OPT_GET_APP_ID(req->options),
                  req->endpoint,
                  req->zgpd_addr);

  if (zgp_proxy_table_read(&zgpd_id, &ent) != RET_OK)
  {
    zgp_proxy_entry_init(&ent, req);
    ret = zgp_proxy_entry_update(&ent, req);

    if (ret == RET_ALREADY_EXISTS)
    {
      ret = RET_OK;
    }
  }
  else
  {
    ret = zgp_proxy_entry_update(&ent, req);
  }

  if (ret == RET_OK)
  {
    ret = zgp_proxy_table_write(&zgpd_id, &ent);
    }

  TRACE_MSG(TRACE_ZGP2, "<< zgp_proxy_add_pairing %hd", (FMT__H, ret));
  return ret;
}

/**
 * @brief Perform remove paring ZGPD with sink
 *
 * @param req    [in]  Pointer to the GP Paring request information
 *
 * @see ZGP spec, A.3.3.5.2
 */
static zb_ret_t zgp_proxy_remove_pairing(zb_zgp_gp_pairing_req_t *req)
{
  zb_zgp_proxy_tbl_ent_t   ent;
  zb_zgpd_id_t             zgpd_id;
  zb_ret_t                 ret = RET_NOT_FOUND;

  TRACE_MSG(TRACE_ZGP2, ">> zgp_proxy_remove_pairing", (FMT__0));

  ZB_MAKE_ZGPD_ID(zgpd_id,
                  ZB_ZGP_PAIRING_OPT_GET_APP_ID(req->options),
                  req->endpoint,
                  req->zgpd_addr);

  ret = zgp_proxy_table_read(&zgpd_id, &ent);

  if (ret == RET_OK)
  {
  switch (ZB_ZGP_PAIRING_OPT_GET_COMMUNICATION_MODE(req->options))
  {
    case ZGP_COMMUNICATION_MODE_FULL_UNICAST:
    case ZGP_COMMUNICATION_MODE_LIGHTWEIGHT_UNICAST:
    {
      zb_address_ieee_ref_t addr_ref;

      TRACE_MSG(TRACE_ZGP2, "LW UNICAST", (FMT__0));

      if (zb_address_by_short(req->sink_nwk_addr, ZB_FALSE, ZB_FALSE, &addr_ref) != RET_OK)
      {
        TRACE_MSG(TRACE_ERROR, "failed get addr_ref by short: %d", (FMT__D, req->sink_nwk_addr));
          ret = RET_ERROR;
      }
      else
      {
        TRACE_MSG(TRACE_ZGP3, "addr_ref: %d", (FMT__D, addr_ref));

          ret = zgp_proxy_del_lwsink_addr_ref(&ent, addr_ref);

          if (ret == RET_OK && zgp_proxy_get_lwsink_addr_list_size(&ent) == 0)
        {
          ZGP_TBL_SET_LWUC(&ent, 0);
          TRACE_MSG(TRACE_ZGP3, "proxy lwsaddr array now is empty", (FMT__0));
        }
      }
      break;
    }
    case ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED:
    {
      zb_uint16_t derived_group;

      derived_group = zgp_calc_alias_source_address(&zgpd_id);

      if (derived_group == req->sink_group_id && ZGP_TBL_GET_DGGC(&ent))
      {
        ZGP_TBL_SET_DGGC(&ent, 0);
        ZGP_TBL_PROXY_CLR_ASSIGNED_ALIAS(&ent);
        ent.zgpd_assigned_alias = 0;

          ret = RET_OK;
      }
      else
      {
        TRACE_MSG(TRACE_ZGP3, "group received != derived, not accepted", (FMT__0));
          ret = RET_INVALID_PARAMETER;
      }
      break;
    }
    case ZGP_COMMUNICATION_MODE_GROUPCAST_PRECOMMISSIONED:
    {
        ret = zgp_del_group_from_entry(ent.u.proxy.sgrp,
                                       req->sink_group_id);

        if (ret == RET_OK && zgp_get_group_list_size(ent.u.proxy.sgrp) == 0)
      {
        ZGP_TBL_SET_CGGC(&ent, 0);
        TRACE_MSG(TRACE_ZGP3, "proxy groups array now is empty", (FMT__0));
      }
      break;
    }
    default:
      ZB_ASSERT(0);
    };

    if (ret == RET_OK)
  {
      if (ZGP_PROXY_TABLE_ENTRY_IS_EMPTY(&ent))
    {
      /* Test 5.4.2.5: supports Proxy Table maintenance, it sets the entry to inactive valid or
       * shifts the GPD to gppBlockedGPDID list, with SearchCounter=0x00; does not support Proxy Table
       * maintenance, it removes the entry (or adds it to gppBlockedGPDID).
       */
      if (ZB_ZGP_PROXY_IS_SUPPORT_FUNCTIONALITY(ZGP_GPP_PROXY_TABLE_MAINTENANCE))
      {
        //TODO:
        ZB_ASSERT(0);
      }
      else
      {
        TRACE_MSG(TRACE_ZGP3, "delete empty proxy table entry", (FMT__0));
          ret = zgp_proxy_table_del(&zgpd_id);
      }
    }
    else
    {
        ret = zgp_proxy_table_write(&zgpd_id, &ent);
      }
      }
    }

  TRACE_MSG(TRACE_ZGP2, "<< zgp_proxy_remove_pairing %hd", (FMT__H, ret));
  return ret;
}

/**
 * @brief Perform remove ZGPD
 *
 * @param req    [in]  Pointer to the GP Paring request information
 *
 * @see ZGP spec, A.3.3.5.2
 */
static zb_ret_t zgp_proxy_remove_zgpd(zb_zgp_gp_pairing_req_t *req)
{
  zb_zgpd_id_t             zgpd_id;
  zb_ret_t      ret;

  TRACE_MSG(TRACE_ZGP2, ">> zgp_proxy_remove_zgpd", (FMT__0));

  ZB_MAKE_ZGPD_ID(zgpd_id,
                  ZB_ZGP_PAIRING_OPT_GET_APP_ID(req->options),
                  req->endpoint,
                  req->zgpd_addr);

  ret = zgp_proxy_table_del(&zgpd_id);

  TRACE_MSG(TRACE_ZGP2, "<< zgp_proxy_remove_zgpd %hd", (FMT__H, ret));
  return ret;
}

static zb_bool_t zgp_proxy_check_comm_mode_is_supported(zb_uint8_t comm_mode)
{
  zb_uint16_t gpp_comm_modes = (ZGP_GPPB_FUNCTIONALITY >> 2) & 0x0f;
  zb_uint16_t recv_comm_mode_mask;

  switch ((zgp_communication_mode_t) comm_mode)
  {
    case ZGP_COMMUNICATION_MODE_FULL_UNICAST:
      recv_comm_mode_mask = 0x04;
      break;
    case ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED:
      recv_comm_mode_mask = 0x01;
      break;
    case ZGP_COMMUNICATION_MODE_GROUPCAST_PRECOMMISSIONED:
      recv_comm_mode_mask = 0x02;
      break;
    case ZGP_COMMUNICATION_MODE_LIGHTWEIGHT_UNICAST:
      recv_comm_mode_mask = 0x08;
      break;
    default:
      recv_comm_mode_mask = 0;
  }

  return (zb_bool_t) (gpp_comm_modes & recv_comm_mode_mask);
}

void zgp_proxy_handle_gp_pairing_req(zb_uint8_t param, zb_zgp_gp_pairing_req_t *req)
{
  zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);
  zb_uint8_t           status = ZB_ZCL_STATUS_SUCCESS;

  zb_uint8_t add_sink = ZB_ZGP_PAIRING_OPT_GET_ADD_SINK(req->options);
  zb_uint8_t communication_mode = ZB_ZGP_PAIRING_OPT_GET_COMMUNICATION_MODE(req->options);
  zb_uint8_t app_id = ZB_ZGP_PAIRING_OPT_GET_APP_ID(req->options);
  zb_uint8_t remove_gpd = ZB_ZGP_PAIRING_OPT_GET_REMOVE_GPD(req->options);

  TRACE_MSG(TRACE_ZGP2, ">> zgp_proxy_handle_gp_pairing_req %hd", (FMT__H, param));

  /* On receipt of GP Pairing command, the Basic Proxy updates its Proxy Table, as instructed by the
   * GP Pairing command, both in commissioning and operational mode. */

  if (add_sink &&
      zgp_proxy_check_comm_mode_is_supported(
        communication_mode) == ZB_FALSE)
  {
    /* If the proxy does not support this CommunicationMode and the GP Pairing command was
       received in unicast, the proxy SHALL respond with ZCL Default response command with
       Status INVALID_FIELD; if the GP Pairing command was received in broadcast,
       the proxy SHALL silently drop it. */
    TRACE_MSG(TRACE_ZGP2, "the proxy does not support this CommunicationMode %hd, DROP buffer",
              (FMT__H, communication_mode));
    status = ZB_ZCL_STATUS_INVALID_FIELD;
  }
  else if (add_sink &&
          (zb_zgp_proxy_table_non_empty_entries_count() >= ZGP_CTXC().proxy_table.base.tbl_size))
  {
    /* If the entry could not be created due to a lack of capacity in the Proxy Table,
       and the GP Pairing command was received in unicast, the proxy SHALL respond with
       ZCL Default response command with Status INSUFFICIENT_SPACE;
       if the GP Pairing command was received in broadcast, the proxy SHALL silently drop it. */
    TRACE_MSG(TRACE_ZGP2, "the entry could not be created due to a lack of capacity in the Proxy Table, DROP buffer",
              (FMT__0));
    status = ZB_ZCL_STATUS_INSUFF_SPACE;
  }
  else
  /* A.3.5.2.3 Operation of GP Proxy Basic and proxy side of GP Combo Basic */
  /* A received GP Pairing command carrying SrcID = 0x00000000 (if ApplicationID = 0b000) or GPD
   * IEEE address 0x0000000000000000 (if ApplicationID = 0b010) SHALL be silently dropped; Proxy
   * Table entry SHALL NOT be created or updated.
   */
  if ((app_id == ZB_ZGP_APP_ID_0000 && req->zgpd_addr.src_id == 0) ||
      (app_id == ZB_ZGP_APP_ID_0010 && ZB_IEEE_ADDR_IS_ZERO(req->zgpd_addr.ieee_addr)))
  {
    TRACE_MSG(TRACE_ZGP2, "recieved zero src or ieee addr, DROP buffer", (FMT__0));
    status = ZB_ZCL_STATUS_INVALID_VALUE;
  }
  else
  /* If in the received GP Pairing command both AddSink sub-field of the Options field and RemoveGPD
   * sub-field of the Options field are set to 0b1, the command SHALL be silently dropped, Proxy
   * Table entries SHALL NOT be modified.
   */
  if (remove_gpd && add_sink)
  {
    TRACE_MSG(TRACE_ZGP2, "recieved both AddSink and RemoveGPD sub-fields, DROP buffer", (FMT__0));
    status = ZB_ZCL_STATUS_INVALID_VALUE;
  }

  if (status == ZB_ZCL_STATUS_SUCCESS)
  {
    if (remove_gpd)
    {
      if (zgp_proxy_remove_zgpd(req) != RET_OK)
      {
        status = ZB_ZCL_STATUS_NOT_FOUND;
    }
    }
    else
    {
      /* If the RemoveGPD sub-field of the Options field was set to 0b0 and the SecurityLevel field
       * of the Options field is set to 0b01, the proxy SHALL NOT update (if existent) nor create a
       * Proxy Table entry.
       */
      if (ZB_ZGP_PAIRING_OPT_GET_SEC_LEVEL(req->options) != ZB_ZGP_SEC_LEVEL_REDUCED)
      {
        zb_ret_t ret;

        if (add_sink)
        {
          ret = zgp_proxy_add_pairing(req);
        }
        else
        {
          ret = zgp_proxy_remove_pairing(req);
        }

        switch (ret)
        {
          case RET_OK:
          case RET_ALREADY_EXISTS:
            status = ZB_ZCL_STATUS_SUCCESS;
            break;
          case RET_ERROR:
            /* zb_address_by_short() failed */
            status = ZB_ZCL_STATUS_FAIL;
            break;
          case RET_INVALID_PARAMETER:
            status = ZB_ZCL_STATUS_INVALID_FIELD;
            break;
          case RET_NOT_FOUND:
            status = ZB_ZCL_STATUS_NOT_FOUND;
            break;
          case RET_TABLE_FULL:
            status = ZB_ZCL_STATUS_INSUFF_SPACE;
            break;
          default:
            ZB_ASSERT(0);
        };
      }
      else
      {
        status = ZB_ZCL_STATUS_INVALID_FIELD;
    }
    }
    /* If the ExitMode had the On first Pairing success sub-field set to 0b1, the proxy SHALL exit
     * commissioning mode upon reception of any GP Pairing command, including GP Pairing command
     * with RemoveGPD sub-field set to 0b1 or AddSink sub-field set to 0b0.
     */
    if (ZGP_IS_PROXY_IN_COMMISSIONING_MODE() &&
        ZB_ZGP_PROXY_COMM_MODE_IS_EXIT_AFTER_FIRST_PAIRING_SUCCESS())
    {
      zb_zgp_proxy_leave_from_commissioning_mode();
    }
  }

  if (!ZB_NWK_IS_ADDRESS_BROADCAST(cmd_info->addr_data.common_data.dst_addr) &&
      /* Let's use ZB_ZCL_STATUS_INVALID_VALUE value as internal value which indicates that we
         should silently drop the frame */
      status != ZB_ZCL_STATUS_INVALID_VALUE)
  {
    if (status != ZB_ZCL_STATUS_SUCCESS ||
        (status == ZB_ZCL_STATUS_SUCCESS && cmd_info->disable_default_response == ZB_FALSE))
    {
      zb_zcl_parsed_hdr_t local_cmd_info;

      ZB_MEMCPY(&local_cmd_info, cmd_info, sizeof(zb_zcl_parsed_hdr_t));
      cmd_info = &local_cmd_info;
      zb_buf_reuse(param);
      zb_zcl_send_default_resp_ext(param,
                                   cmd_info,
                                   (zb_zcl_status_t)status);
      param = 0;
    }
  }

  if (param)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZGP2, "<< zgp_proxy_handle_gp_pairing_req", (FMT__0));
}

/**
 * @brief Search next lightweight pairing information in proxy entry
 *
 * @param ent        [in]   Pointer to the proxy entry
 * @param it         [in]   Current proxy entry iterator
 * @param remaining  [out]  Count of remaining pairing information
 *
 */
static void zgp_proxy_get_next_lwaddr_valid_idx(zgp_tbl_ent_t           *ent,
                                                zgp_gp_notif_iterator_t *it,
                                                zb_uint8_t              *remaining)
{
  zb_uindex_t i;
  zb_uint8_t res = ZGP_PROXY_GP_NOTIF_ITERATOR_INVALID_IDX;

  TRACE_MSG(TRACE_ZGP2, ">> zgp_proxy_get_next_lwaddr_valid_idx", (FMT__0));

  for (i = it->idx; i < ZB_ZGP_MAX_LW_UNICAST_ADDR_PER_GPD; i++)
  {
    if (ent->u.proxy.lwsaddr[i].addr_ref != ZGP_PROXY_LWADDR_INVALID_IDX)
    {
      if (res == ZGP_PROXY_GP_NOTIF_ITERATOR_INVALID_IDX)
      {
        res = i;
      }
      else
      {
        (*remaining)++;
      }
    }
  }
  it->idx = res;

  if (res != ZGP_PROXY_GP_NOTIF_ITERATOR_INVALID_IDX)
  {
    TRACE_MSG(TRACE_ZGP2, "addr_ref=%hd", (FMT__H, ent->u.proxy.lwsaddr[res].addr_ref));
}

  TRACE_MSG(TRACE_ZGP2, "<< zgp_proxy_get_next_lwaddr_valid_idx, idx: %hd, rem: %d",
            (FMT__H_H, it->idx, *remaining));
}

/**
 * @brief Search next groupcast pairing information in proxy entry
 *
 * @param ent        [in]   Pointer to the proxy entry
 * @param it         [in]   Current proxy entry iterator
 * @param remaining  [out]  Count of remaining pairing information
 *
 */
static void zgp_proxy_get_next_group_valid_idx(zgp_tbl_ent_t           *ent,
                                               zgp_gp_notif_iterator_t *it,
                                               zb_uint8_t              *remaining)
{
  zb_uindex_t i;
  zb_uint8_t res = ZGP_PROXY_GP_NOTIF_ITERATOR_INVALID_IDX;

  TRACE_MSG(TRACE_ZGP2, ">> zgp_proxy_get_next_group_valid_idx", (FMT__0));

  for (i = it->idx; i < ZB_ZGP_MAX_SINK_GROUP_PER_GPD; i++)
  {
    if (ent->u.proxy.sgrp[i].sink_group != ZGP_PROXY_GROUP_INVALID_IDX)
    {
      if (res == ZGP_PROXY_GP_NOTIF_ITERATOR_INVALID_IDX)
      {
        res = i;
      }
      else
      {
        (*remaining)++;
      }
    }
  }
  it->idx = res;
  TRACE_MSG(TRACE_ZGP2, "<< zgp_proxy_get_next_group_valid_idx, idx: %hd, rem: %d",
            (FMT__H_H, it->idx, *remaining));
}

/**
 * @brief Search next pairing information for perform gp notification request
 *
 * @param zgpd_id    [in]   Pointer to the ZGPD ID
 * @param it         [in]   Current proxy entry iterator
 * @param remaining  [out]  Count of remaining pairing information
 *
 */
static void zgp_proxy_get_next_gp_notification_valid_idx(zb_zgpd_id_t            *zgpd_id,
                                                         zgp_gp_notif_iterator_t *it,
                                                         zb_uint8_t              *remaining)
{
  zgp_tbl_ent_t ent;

  TRACE_MSG(TRACE_ZGP2, ">> zgp_proxy_get_next_gp_notification_valid_idx", (FMT__0));

  if (zgp_proxy_table_read(zgpd_id, &ent) == RET_OK)
  {
    if (it->mode == ZGP_GP_NOTIF_ITERATOR_MODE_UNDEFINED)
    {
      it->mode = ZGP_GP_NOTIF_ITERATOR_MODE_LWADDR;
      it->idx = 0;
    }
    else
    {
      it->idx = it->idx + 1;
    }

    while (it->mode < ZGP_GP_NOTIF_ITERATOR_MODE_END)
    {
      switch (it->mode)
      {
        case ZGP_GP_NOTIF_ITERATOR_MODE_LWADDR:
        {
          zgp_proxy_get_next_lwaddr_valid_idx(&ent, it, remaining);
          break;
        }
        case ZGP_GP_NOTIF_ITERATOR_MODE_DGROUP:
        {
          if(ZGP_TBL_GET_DGGC(&ent)&&(it->idx==0))
          {
            it->idx=1;
            *remaining=0;
          }
          else
          {
            it->idx=ZGP_PROXY_GP_NOTIF_ITERATOR_INVALID_IDX;
          }
          break;
        }
        case ZGP_GP_NOTIF_ITERATOR_MODE_CGROUP:
        {
          if (ZGP_TBL_GET_CGGC(&ent) && (it->idx == 0))
          {
          zgp_proxy_get_next_group_valid_idx(&ent, it, remaining);
          }
          else
          {
            it->idx = ZGP_PROXY_GP_NOTIF_ITERATOR_INVALID_IDX;
          }
          break;
        }
      };

      if (it->idx != ZGP_PROXY_GP_NOTIF_ITERATOR_INVALID_IDX)
      {
        break;
      }
      TRACE_MSG(TRACE_ZGP2, "invalid index", (FMT__0));
      it->mode = it->mode + 1;
      it->idx = 0;
    }

    if (it->mode == ZGP_GP_NOTIF_ITERATOR_MODE_END)
    {
      it->idx = ZGP_PROXY_GP_NOTIF_ITERATOR_INVALID_IDX;
    }
  }
  else
  {
    TRACE_MSG(TRACE_ZGP1, "failed read proxy table", (FMT__0));
    it->idx = ZGP_PROXY_GP_NOTIF_ITERATOR_INVALID_IDX;
  }

  TRACE_MSG(TRACE_ZGP2, "<< zgp_proxy_get_next_gp_notification_valid_idx idx: %hd mode: %hd",
            (FMT__H_H, it->idx, it->mode));
}


static void zgp_proxy_construct_gp_notification_options(zb_uint8_t   param,
                                                        zb_uint16_t *options, zgp_tbl_ent_t *ent)
{
  zb_gpdf_info_t *gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);

  *options = 0;
  /* ApplicationID */
  *options |= ZB_GPDF_EXT_NFC_GET_APP_ID(gpdf_info->nwk_ext_frame_ctl);

/* TODO: get actual values from runtime context */
  *options |= ((( ent->options >> 6 ) & 0x7 ) << 3 );

/*
  *options |= (0x01 << 3);                Also Unicast
  *options |= (0x00 << 4);                Also Derived Group
  *options |= (0x00 << 5);                Also Commissioned Group
*/

  /* SecurityLevel */
  *options |= (ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl) << 6);

  /* SecurityKey-Type */
  *options |= (gpdf_info->key_type << 8);

/* The RxAfterTx sub-field SHALL be copied from the RxAfterTx sub-field
   of the Extended NWK Frame Control field of the triggering GPDF was set;
   irrespective of bidirectional communication capabilities of
   the device sending the GP Notification.
*/
  *options |= (ZB_GPDF_EXT_NFC_GET_RX_AFTER_TX(gpdf_info->nwk_ext_frame_ctl) << 11);

/* The gpTxQueueFull sub-field indicates whether the proxy can still receive
   and store a GPDF Response for this GPD. If this field value is 0b0,
   there is space in the gpTxQueue for this GPD. If this field is set to 0b1,
   there is no space left in the gpTxQueue for this GPD.
   A forwarding device not supporting bidirectional communication
   SHALL always set this field to 0b1.
*/
  *options |= (0x01 << 12);

/* The BidirectionalCommunicationCapability sub-field, when set to 0b0,
   indicates that the device send-ing the GP Notification command does NOT support
   bidirectional communication. All proxy basic devices implementing
   the current specification SHALL always set
   the BidirectionalCommunicationCapability sub-field to 0b0.
*/
  *options |= (0x00 << 13);

/* The Proxy info present sub-field, when set to 0b1, indicates
   that the fields GPP short address and GPP-GPD link fields are present.
   All proxy basic device implementing the current specification
   SHALL always set Proxy info present sub-field to 0b1.
*/
  *options |= (0x01 << 14);
}

static void zgp_proxy_send_gp_notification(zb_uint8_t param)
{
  zgp_gp_notif_iterator_t *it = (zgp_gp_notif_iterator_t *)zb_buf_get_tail(
    param, sizeof(zgp_gp_notif_iterator_t) + sizeof(zb_gpdf_info_t));
  zb_gpdf_info_t         *gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);
  zgp_tbl_ent_t           ent;

  TRACE_MSG(TRACE_ZGP2, ">> zgp_proxy_send_gp_notification param %hd", (FMT__H, param));

  if (zgp_proxy_table_read(&gpdf_info->zgpd_id, &ent) == RET_OK)
  {
    zb_uint16_t dst_addr = 0U;
    zb_uint16_t alias_addr = 0;
    zb_uint8_t alias_seq = 0;
    zb_uint8_t dst_addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
    zb_bool_t use_alias = ZB_FALSE;
    TRACE_MSG(TRACE_ZGP3, "switch (it->mode)", (FMT__0));

    switch (it->mode)
    {
      case ZGP_GP_NOTIF_ITERATOR_MODE_LWADDR:
      {
        zb_address_ieee_ref_t addr_ref;
        TRACE_MSG(TRACE_ZGP3, "ZGP_GP_NOTIF_ITERATOR_MODE_LWADDR", (FMT__0));
        ZB_ASSERT(it->idx < ZB_ZGP_MAX_LW_UNICAST_ADDR_PER_GPD);

        addr_ref = ent.u.proxy.lwsaddr[it->idx].addr_ref;

        if (addr_ref != ZGP_PROXY_LWADDR_INVALID_IDX)
        {
          zb_address_short_by_ref(&dst_addr, addr_ref);
          dst_addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
          TRACE_MSG(TRACE_ZGP2, "addr_ref=%hd dst_addr 0x%x", (FMT__H_D,addr_ref, dst_addr));
        }
        break;
      }
      case ZGP_GP_NOTIF_ITERATOR_MODE_DGROUP:
      {
        TRACE_MSG(TRACE_ZGP3, "ZGP_GP_NOTIF_ITERATOR_MODE_DGROUP", (FMT__0));

        dst_addr = zgp_calc_alias_source_address(&gpdf_info->zgpd_id);
        TRACE_MSG(TRACE_ZGP3, "use derived group : 0x%x", (FMT__D, dst_addr));
        dst_addr_mode = ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT;
        use_alias = ZB_TRUE;
        if (ZGP_TBL_PROXY_GET_ASSIGNED_ALIAS(&ent))
        {
          alias_addr = ent.zgpd_assigned_alias;
          TRACE_MSG(TRACE_ZGP3, "use zgpd_assigned_alias : 0x%x", (FMT__D, ent.zgpd_assigned_alias));
        }
        else
        {
          alias_addr = zgp_calc_alias_source_address(&gpdf_info->zgpd_id);
          TRACE_MSG(TRACE_ZGP3, "use derived alias : 0x%x", (FMT__D, alias_addr));
        }
        alias_seq = zgp_calc_alias_sequence_number(ZB_GPC_GP_DERIVED_GROUPCAST_GP_NOTIFICATION, gpdf_info->mac_seq_num);
        TRACE_MSG(TRACE_ZGP3, "use alias sequence: %d", (FMT__H, alias_seq));
        break;
      }
      case ZGP_GP_NOTIF_ITERATOR_MODE_CGROUP:
      /* Groupcast – One of the communication modes used for tunneling GPD commands between the
          proxies and sinks. In Zigbee terms, it is the APS level multicast, with NWK level broadcast to the
          RxOnWhenIdle=TRUE (0xfffd) broadcast address. */
      {
        TRACE_MSG(TRACE_ZGP3, "ZGP_GP_NOTIF_ITERATOR_MODE_CGROUP", (FMT__0));
        /* Groupcast: One of the communication modes used for tunneling GPD commands
         * between the proxies and sinks. In ZigBee terms, it is the APS level multicast,
         * with NWK level broadcast to the RxOnWhenIdle=TRUE (0xfffd) broadcast address.
         */

          dst_addr = ent.u.proxy.sgrp[it->idx].sink_group;
          TRACE_MSG(TRACE_ZGP3, "use group : 0x%x", (FMT__D, dst_addr));
          dst_addr_mode = ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT;
          use_alias = ZB_TRUE;
          /* A.3.3.2.2.2.4   Group list parameter
           * The Group list parameter stores the GroupID and the corresponding
           * alias for groupcast communication.
           * The Alias field of the Group list entry set to 0xffff indicates usage of derived alias
           * for the Sink group in the same Group list entry.
           */
          if ((ent.u.proxy.sgrp[it->idx].alias)==ZGP_PROXY_GROUP_DERIVED_ALIAS)
          {
/* FIXME: if really need to replace alias with zgpd_assigned_alias if derived or full unicast */
            if (ZGP_TBL_PROXY_GET_ASSIGNED_ALIAS(&ent))
            {
              alias_addr = ent.zgpd_assigned_alias;
              TRACE_MSG(TRACE_ZGP3, "use zgpd_assigned_alias : 0x%x", (FMT__D, ent.zgpd_assigned_alias));
            }
            else
            {
              alias_addr = zgp_calc_alias_source_address(&gpdf_info->zgpd_id);
              TRACE_MSG(TRACE_ZGP3, "use derived alias : 0x%x", (FMT__D, alias_addr));
            }
          }
          else
          {
            alias_addr = (ent.u.proxy.sgrp[it->idx].alias);
            TRACE_MSG(TRACE_ZGP3, "use alias from table (precommissioned): 0x%x", (FMT__D, alias_addr));
          }
        alias_seq = zgp_calc_alias_sequence_number(ZB_GPC_GP_COMMISSIONED_GROUPCAST_GP_NOTIFICATION, gpdf_info->mac_seq_num);
          TRACE_MSG(TRACE_ZGP3, "use alias sequence: %d", (FMT__H, alias_seq));
        break;
      }
      default:
        ZB_ASSERT(0);
    };

    if (param)
    {
      zb_uint16_t options;

      zgp_proxy_construct_gp_notification_options(param, &options, &ent);
      zb_zgp_cluster_gp_notification_req(param,
                                         use_alias, alias_addr, alias_seq,
                                         dst_addr,
                                         dst_addr_mode,
                                         options,
                                         ent.groupcast_radius,
                                         NULL);
      param = 0;
    }
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "failed read proxy entry", (FMT__0));
  }

  if (param)
  {
    zb_buf_free(param);
  }
  TRACE_MSG(TRACE_ZGP3, "<< zgp_proxy_send_gp_notification", (FMT__0));
}

static void zgp_proxy_gp_notification_dup(zb_uint8_t param, zb_uint16_t user_param);

static void zgp_proxy_send_next_gp_notification(zb_uint8_t param)
{
  zb_gpdf_info_t          *gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);
  zgp_gp_notif_iterator_t *it = (zgp_gp_notif_iterator_t *)zb_buf_get_tail(
    param, sizeof(zgp_gp_notif_iterator_t) + sizeof(zb_gpdf_info_t));
  zb_uint8_t               remaining = 0;

  TRACE_MSG(TRACE_ZGP2, ">> zgp_proxy_send_next_gp_notification %hd",
            (FMT__H, param));
  /*
    constructs from the received GPDF a GP Notification command(s) for each
    group address and each unicast sink stored in the Proxy Table for this GPD
  */
  zgp_proxy_get_next_gp_notification_valid_idx(&gpdf_info->zgpd_id,
                                               it,
                                               &remaining);

  if (it->idx != ZGP_PROXY_GP_NOTIF_ITERATOR_INVALID_IDX)
  {
    TRACE_MSG(TRACE_ZGP3, "iterator: mode[%hd], idx[%hd]; remaining: %hd",
              (FMT__H_H_H, it->mode, it->idx, remaining));
    if (remaining || it->mode < ZGP_GP_NOTIF_ITERATOR_MODE_END)
    {
      zb_buf_get_out_delayed_ext(zgp_proxy_gp_notification_dup, param, 0);
    }
    else
    {
      zgp_proxy_send_gp_notification(param);
    }
  }
  else
  {
    zb_buf_free(param);
  }
  TRACE_MSG(TRACE_ZGP3, "<< zgp_proxy_send_next_gp_notification", (FMT__0));
}

static void zgp_proxy_gp_notification_dup(zb_uint8_t param, zb_uint16_t user_param)
{
  TRACE_MSG(TRACE_ZGP3, ">> zgp_proxy_gp_notification_dup %hd %hd", (FMT__H_H, param, user_param));
  zb_buf_copy(param, user_param);
  zgp_proxy_send_gp_notification(param);
  zgp_proxy_send_next_gp_notification(user_param);
  TRACE_MSG(TRACE_ZGP3, "<< zgp_proxy_gp_notification_dup", (FMT__0));
}

static void zgp_proxy_send_gp_notification_req(zb_uint8_t param)
{
  zgp_gp_notif_iterator_t *it = (zgp_gp_notif_iterator_t *)zb_buf_get_tail(
    param, sizeof(zgp_gp_notif_iterator_t) + sizeof(zb_gpdf_info_t));

  TRACE_MSG(TRACE_ZGP3, ">> zgp_proxy_send_gp_notification_req %hd", (FMT__H, param));

#ifdef ZB_CERTIFICATION_HACKS
  if (ZGP_CERT_HACKS().gp_proxy_gp_notif_req_cb)
  {
    ZGP_CERT_HACKS().gp_proxy_gp_notif_req_cb(param);
  }
#endif  /* ZB_CERTIFICATION_HACKS */

  it->mode = ZGP_GP_NOTIF_ITERATOR_MODE_UNDEFINED;
  zgp_proxy_send_next_gp_notification(param);

  TRACE_MSG(TRACE_ZGP3, "<< zgp_proxy_send_gp_notification_req", (FMT__0));
}
#endif  /* ZB_ENABLE_ZGP_PROXY */

#ifdef ZB_ENABLE_ZGP_SINK
/**
 * @brief Send GP Pairing if needed using gp pairing configuration info
 *
 * @param param      [in]  Buffer reference
 *
 * @see ZGP spec, A.3.3.4.6
 */
static void zgp_sink_gp_pairing_configuration_no_action(zb_uint8_t param)
{
  zb_zgp_gp_pairing_conf_t *conf = ZB_BUF_GET_PARAM(param, zb_zgp_gp_pairing_conf_t);
  zb_ret_t                  ret = RET_OK;

  TRACE_MSG(TRACE_ZGP2, ">> zgp_sink_gp_pairing_configuration_no_action %hd", (FMT__H, param));

  if (ZB_ZGP_GP_PAIRING_CONF_GET_SEND_PAIRING(conf->actions))
  {
    zb_zgp_sink_tbl_ent_t     ent;
    zb_zgpd_id_t              zgpd_id;

    ZB_MAKE_ZGPD_ID(zgpd_id,
                    ZB_ZGP_GP_PAIRING_CONF_OPT_GET_APP_ID(conf->options),
                    conf->endpoint,
                    conf->zgpd_addr);

    if (zgp_sink_table_read(&zgpd_id, &ent) != RET_OK)
    {
      TRACE_MSG(TRACE_ZGP2, "Sink table entry not found", (FMT__0));
      ret = RET_ERROR;
    }

    if (ZB_ZGP_GP_PAIRING_CONF_GET_SEND_PAIRING(conf->actions) && ret == RET_OK)
    {
      zb_zgp_gp_pairing_send_req_t *req;

      ZB_ZGP_GP_PAIRING_SEND_REQ_CREATE(param, req, &ent, NULL);
      ZB_ZGP_GP_PAIRING_MAKE_SEND_OPTIONS(req, 1, 0, 1);
      zgp_cluster_send_gp_pairing(param);
      param = 0;
    }
  }

  if (param)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZGP2, "<< zgp_sink_gp_pairing_configuration_no_action", (FMT__0));
}

/**
 * @brief Add new sink entry using gp pairing configuration info
 *
 * @param param      [in]  Buffer reference
 *
 * @see ZGP spec, A.3.3.4.6
 */
static zb_ret_t zgp_sink_gp_pairing_configuration_add_new_entry(zb_uint8_t param)
{
  zb_zgp_gp_pairing_conf_t *conf = ZB_BUF_GET_PARAM(param, zb_zgp_gp_pairing_conf_t);
  zb_zgp_sink_tbl_ent_t     ent;
  zb_zgpd_id_t              zgpd_id;
  zb_bool_t                 match_found = ZB_FALSE;
  zb_uint8_t                match_table_idx;
  zb_uint8_t                apsme_group_cnt = 0;
  zb_uint16_t               apsme_groups[ZB_ZGP_MAX_SINK_GROUP_PER_GPD + 1];
  zb_ret_t                  ret = RET_ERROR;

  TRACE_MSG(TRACE_ZGP2, ">> zgp_sink_gp_pairing_configuration_add_new_entry %hd", (FMT__H, param));

  ZB_BZERO(&ent, sizeof(zb_zgp_sink_tbl_ent_t));

  ZB_MAKE_ZGPD_ID(zgpd_id,
                  ZB_ZGP_GP_PAIRING_CONF_OPT_GET_APP_ID(conf->options),
                  conf->endpoint,
                  conf->zgpd_addr);

  ent.options = ZGP_TBL_SINK_FILL_OPTIONS(
    zgpd_id.app_id,
    ZB_ZGP_GP_PAIRING_CONF_GET_COMMUNICATION_MODE(conf->options),
    ZB_ZGP_GP_PAIRING_CONF_GET_SEQ_NUM_CAPS(conf->options),
    ZB_ZGP_GP_PAIRING_CONF_GET_RX_ON_CAPS(conf->options),
    ZB_ZGP_GP_PAIRING_CONF_GET_FIXED_LOC(conf->options),
    ZB_ZGP_GP_PAIRING_CONF_GET_ASSIGNED_ALIAS_PRESENT(conf->options),
    ZB_ZGP_GP_PAIRING_CONF_GET_SEC_USE(conf->options));
  ent.zgpd_id = zgpd_id.addr;
  ent.u.sink.device_id = conf->device_id;

  if (zgpd_id.app_id == ZB_ZGP_APP_ID_0000)
  {
#ifdef ZGP_INCLUDE_DST_LONG_ADDR
    ZB_BZERO(ent.u.sink.ieee_addr, sizeof(zb_ieee_addr_t));
#endif
  }
  else
  {
    ent.endpoint = zgpd_id.endpoint;
#ifdef ZGP_INCLUDE_DST_LONG_ADDR
    ZB_MEMCPY(ent.u.sink.ieee_addr, zgpd_id.addr.ieee_addr, sizeof(zb_ieee_addr_t));
#endif
  }

  if (ZB_ZGP_GP_PAIRING_CONF_GET_SEC_USE(conf->options))
  {
    ent.sec_options = conf->sec_options;
    ent.security_counter = conf->sec_frame_counter;
    ZB_MEMCPY(ent.zgpd_key, conf->key, ZB_CCM_KEY_SIZE);
  }
  else
  if (ZB_ZGP_GP_PAIRING_CONF_GET_SEQ_NUM_CAPS(conf->options))
  {
    ent.security_counter = conf->sec_frame_counter;
  }

  ent.groupcast_radius = conf->frwd_radius;

  if (ZB_ZGP_GP_PAIRING_CONF_GET_COMMUNICATION_MODE(conf->options) ==
      ZGP_COMMUNICATION_MODE_GROUPCAST_PRECOMMISSIONED)
  {
    zb_uindex_t i;

    ZB_MEMSET(&ent.u.sink.sgrp, (-1), sizeof(ent.u.sink.sgrp));

    for (i = 0; i < ZB_ZGP_MAX_SINK_GROUP_PER_GPD; i++)
    {
      if (conf->sgrp[i].sink_group == ZGP_PROXY_GROUP_INVALID_IDX)
      {
        break;
      }
      ent.u.sink.sgrp[i].sink_group = conf->sgrp[i].sink_group;
      ent.u.sink.sgrp[i].alias = conf->sgrp[i].alias;
      apsme_groups[apsme_group_cnt++] = conf->sgrp[i].sink_group;
    }
  }
  else
  if (ZB_ZGP_GP_PAIRING_CONF_GET_COMMUNICATION_MODE(conf->options) == ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED)
  {
    apsme_groups[apsme_group_cnt] = zgp_calc_alias_source_address(&zgpd_id);
    apsme_group_cnt++;
  }

  if (ZB_ZGP_GP_PAIRING_CONF_GET_ASSIGNED_ALIAS_PRESENT(conf->options))
  {
    ent.zgpd_assigned_alias = conf->assigned_alias;
    ZGP_TBL_SINK_SET_ASSIGNED_ALIAS(&ent);
  }

  if (conf->device_id != ZB_ZGP_MANUF_SPECIFIC_DEV_ID)
  {
    match_found = zb_zgps_get_dev_matching_tbl_index(conf->device_id,
                                                     &zgpd_id,
                                                     &match_table_idx);
  }
  else
  {
    if (ZB_ZGP_GP_PAIRING_CONF_APP_INFO_GET_MODEL_ID_PRESENT(conf->app_info))
    {
      match_found = zb_zgps_get_ms_dev_matching_tbl_index(conf->model_id,
                                                          &zgpd_id,
                                                          &match_table_idx);
    }
  }

  if (match_found)
  {
    ent.u.sink.match_dev_tbl_idx = match_table_idx;

    if (zgp_sink_table_write(&zgpd_id, &ent) == RET_OK)
    {
      zb_uindex_t i;

      ret = RET_OK;

      for (i = 0; i < apsme_group_cnt; i++)
      {
        if (zb_apsme_add_group_internal(apsme_groups[i], ZGP_ENDPOINT) == ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_TABLE_FULL))
        {
          TRACE_MSG(TRACE_ZGP2, "Error adding SINK GP to APS group", (FMT__0));
          ret = RET_ERROR;
          break;
        }
        else
        {
          TRACE_MSG(TRACE_ZGP3, "Create APS-layer link to GP EP: group: 0x%04x", (FMT__D, apsme_groups[i]));
        }
      }
    }
    else
    {
      TRACE_MSG(TRACE_ERROR, "No space left in sink table, drop buffer", (FMT__0));
    }
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "No match found for pairing configuration device_id or model_id, drop buffer", (FMT__0));
  }

  TRACE_MSG(TRACE_ZGP2, "<< zgp_sink_gp_pairing_configuration_add_new_entry %hd", (FMT__H, ret));
  return ret;
}

/**
 * @brief Extend already exist sink entry using gp pairing configuration info
 *
 * @param param      [in]  Buffer reference
 * @param ent        [in]  Pointer to the exist entry
 *
 * @see ZGP spec, A.3.3.4.6
 */
static zb_ret_t zgp_sink_gp_pairing_configuration_extend_exist_entry(zb_uint8_t             param,
                                                                     zb_zgp_sink_tbl_ent_t *ent)
{
  zb_zgp_gp_pairing_conf_t *conf = ZB_BUF_GET_PARAM(param, zb_zgp_gp_pairing_conf_t);
  zb_ret_t                  ret = RET_ERROR;
  zb_uint8_t                apsme_group_cnt = 0;
  zb_uint16_t               apsme_groups[ZB_ZGP_MAX_SINK_GROUP_PER_GPD + 1];
  zb_zgpd_id_t              zgpd_id;

  TRACE_MSG(TRACE_ZGP2, ">> zgp_sink_gp_pairing_configuration_extend_exist_entry %hd", (FMT__H, param));

  ZB_MAKE_ZGPD_ID(zgpd_id,
                  ZB_ZGP_GP_PAIRING_CONF_OPT_GET_APP_ID(conf->options),
                  conf->endpoint,
                  conf->zgpd_addr);

  if (ZB_ZGP_GP_PAIRING_CONF_GET_COMMUNICATION_MODE(conf->options) ==
      ZGP_COMMUNICATION_MODE_GROUPCAST_PRECOMMISSIONED)
  {
    zb_uindex_t i;

    for (i = 0; i < ZB_ZGP_MAX_SINK_GROUP_PER_GPD; i++)
    {
      zb_ret_t res = zgp_add_group_alias_for_entry(ent->u.sink.sgrp,
                                                   conf->sgrp[i].sink_group,
                                                   conf->sgrp[i].alias);

      if (res == RET_TABLE_FULL)
      {
        TRACE_MSG(TRACE_ERROR, "No free space in sgrp sink table, drop buffer", (FMT__0));
        TRACE_MSG(TRACE_ZGP2, "<< zgp_sink_gp_pairing_configuration_extend_exist_entry %hd", (FMT__H, ret));
        return ret;
      }
      apsme_groups[apsme_group_cnt++] = conf->sgrp[i].sink_group;
      ret = RET_OK;
    }
  }

  if (ret == RET_OK)
  {
    if (zgp_sink_table_write(&zgpd_id, ent) == RET_OK)
    {
      zb_uindex_t i;

      ret = RET_OK;

      for (i = 0; i < apsme_group_cnt; i++)
      {
        if (zb_apsme_add_group_internal(apsme_groups[i], ZGP_ENDPOINT) == ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_TABLE_FULL))
        {
          TRACE_MSG(TRACE_ZGP2, "Error adding SINK GP to APS group", (FMT__0));
          ret = RET_ERROR;
          break;
        }
        else
        {
          TRACE_MSG(TRACE_ZGP3, "Create APS-layer link to GP EP: group: 0x%04x", (FMT__D, apsme_groups[i]));
        }
      }
    }
    else
    {
      TRACE_MSG(TRACE_ERROR, "No space left in sink table, drop buffer", (FMT__0));
      ret = RET_ERROR;
    }
  }

  TRACE_MSG(TRACE_ZGP2, "<< zgp_sink_gp_pairing_configuration_extend_exist_entry %hd", (FMT__H, ret));
  return ret;
}

/**
 * @brief Analyze the Number of paired endpoints in  gp pairing configuration info
 *
 * @param param      [in]  Buffer reference
 *
 * @see ZGP spec, A.3.5.2.4.1
 */
static zb_ret_t zgp_sink_gp_pairing_configuration_analyze_paired_endpoints(zb_uint8_t             param,
                                                                           zb_zgp_sink_tbl_ent_t *ent)
{
  zb_zgp_gp_pairing_conf_t *conf = ZB_BUF_GET_PARAM(param, zb_zgp_gp_pairing_conf_t);
  zb_ret_t                  ret = RET_IGNORE;

  TRACE_MSG(TRACE_ZGP2, ">> zgp_sink_gp_pairing_configuration_analyze_paired_endpoints %hd", (FMT__H, param));

  switch (conf->num_paired_endpoints)
  {
    case 0x00:
    case 0xfd:
    {
      /* If the Number of paired endpoints field is set to 0x00 or 0xfd, the data from this GPD is
       * not meant for local execution on this sink.
       */
      if (ZB_ZGP_SINK_IS_SUPPORT_FUNCTIONALITY(ZGP_GPS_SINK_TABLE_BASED_GROUPCAST_FORWARDING))
      {
        /* If the sink does support Sink Table-based forwarding in the requested CommunicationMode,
         * it SHALL create a Sink Table entry with the supplied information and a Translation Table
         * entry for the GPD ID (and GPD Endpoint, matching or 0x00 or 0xff, if ApplicationID =
         * 0b010) , with the EndPoint field having the value 0xfd. If the CommunicationMode supplied
         * in the Pairing Configuration command was groupcast, the sink SHALL add its Green Power
         * EndPoint as a member of the supplied group or de-rived group at APS level if not already
         * a member.
         */
        //TODO:
        ZB_ASSERT(0);
      }
      else
      {
        /* If the sink does NOT support Sink Table-based forwarding or it does not support Sink
         * Table-based for-warding in the requested CommunicationMode, the sink (i) MAY create a
         * Sink Table entry with the supplied information and a Translation Table entry for this GPD
         * ID (and GPD Endpoint, matching or 0x00 or 0xff, if ApplicationID = 0b010)  with Endpoint
         * field set to 0x00; (ii) MAY create a Sink Ta-ble entry with the supplied information and
         * refrain from creating any Translation Table entry for this GPD ID (and matching GPD
         * Endpoint, if ApplicationID = 0b010)  (sink SHALL NOT use this op-tion if it has generic
         * Translation Table entries for this GPD command(s)); or (iii) MAY refrain from cre-ating
         * both Sink Table entry and Translation Table entry for this GPD ID (and matching GPD
         * Endpoint, if ApplicationID = 0b010) . If the Sink Table entry is created and the
         * CommunicationMode supplied in the Pairing Configuration command was groupcast, the sink
         * SHALL add its Green Power EndPoint as a member of the supplied group or derived group at
         * APS level if not already a member.
         */
        //EES: do nothing
      }
      break;
    }
    case 0xff:
    {
      /* If the Number of paired endpoints field is set to 0xff, all matching endpoints are to be
       * paired; the sink MAY then create a Sink Table entry with the supplied information and
       * Translation Table entry for the GPD ID (and GPD Endpoint, matching or 0x00 or 0xff, if
       * ApplicationID = 0b010) , with the End-Point field having the value 0xff; the unmodified
       * generic entry, if available, MAY be used instead. If the CommunicationMode supplied in
       * the Pairing Configuration command was groupcast, the sink SHALL add its Green Power
       * EndPoint as a member of the supplied group or derived group at APS level if not already a
       * member. If no match is found, the sink SHALL act as described above for Number of
       * paired endpoints equal to 0x00 or 0xfd.
       */
      //EES: do nothing
      break;
    }
    case 0xfe:
    {
      /* If the Number of paired endpoints field is set to 0xfe, the paired endpoints are to be
       * derived by the sink. If the GP Pairing Configuration command carries a CommunicationMode
       * 0b10 and the GroupList is present, all application endpoints being members of this group
       * are to be paired; otherwise, the sink is to derive the paired endpoints in an
       * application-specific manner. The sink SHOULD then create a Sink Table entry with the
       * supplied information and a Translation Table entry/entries for the GPD ID (and GPD
       * Endpoint, matching or 0x00 or 0xff, if ApplicationID = 0b010) , with the EndPoint field
       * con-taining the derived value of the sink's endpoint; the unmodified generic entry, if
       * available, MAY be used instead. If the CommunicationMode supplied in the Pairing
       * Configuration command was group-cast, the sink SHALL add its Green Power EndPoint as a
       * member of the supplied group or derived group at APS level if not already a member.  If no
       * match is found, the sink SHALL act as described above for Number of paired endpoints equal
       * to 0x00 or 0xfd.
       */
      /* 15-02014-006-GP_Errata_for_GP Basic_specification_14-0563.docx:
       * If no match is found (i.e., in case of CommunicationMode 0b10, none of the application
       * endpoints of the sink is a member of any of the groups listed in the GroupList field),
       * the sink SHALL act as described above for Number of paired endpoints equal to 0x00 or 0xfd.
       */
      /* EES: currently our ZGP implementation support dynamic translation of ZGP command to ZCL
       * command instead Translation Table. We dynamic search endpoint for incoming GPD command.
       */
      if (ent == NULL)
      {
        ret = zgp_sink_gp_pairing_configuration_add_new_entry(param);
      }
      else
      {
        ret = zgp_sink_gp_pairing_configuration_extend_exist_entry(param, ent);
      }
      break;
    }
    default:
    {
      /* If the Number of paired endpoints field has values other than 0x00, 0xfd, 0xfe, or 0xff,
       * the Paired end-points field is present and contains the list of local endpoints paired to
       * this GPD; the sink creates a Translation Table entry for this GPD ID (and GPD Endpoint, if
       * ApplicationID = 0b010) and each EndPoint listed in the Paired endpoints field. If the
       * CommunicationMode supplied in the Pairing Configuration command was groupcast, the sink
       * SHALL add its Green Power EndPoint as a member of the supplied group or derived group at
       * APS level if not already a member.
       */
      /* EES: currently our ZGP implementation support dynamic translation of ZGP command to ZCL
       * command instead Translation Table. We dynamic search endpoint for incoming GPD command.
       * For this case we can't match endpoints.
       */
      ZB_ASSERT(0);
    }
  };

  TRACE_MSG(TRACE_ZGP2, "<< zgp_sink_gp_pairing_configuration_analyze_paired_endpoints %hd", (FMT__H, ret));
  return ret;
}

/**
 * @brief Extend sink entry using new gp pairing configuration info
 *
 * @param param      [in]  Buffer reference
 *
 * @see ZGP spec, A.3.3.4.6
 */
static zb_ret_t zgp_sink_gp_pairing_configuration_extend(zb_uint8_t param)
{
  zb_zgp_gp_pairing_conf_t *conf = ZB_BUF_GET_PARAM(param, zb_zgp_gp_pairing_conf_t);
  zb_zgp_sink_tbl_ent_t     ent;
  zb_zgp_sink_tbl_ent_t    *ent_p = NULL;
  zb_zgpd_id_t              zgpd_id;
  zb_ret_t                  ret = RET_ERROR;
  zb_bool_t                 need_analyze_endpoints = ZB_FALSE;

  TRACE_MSG(TRACE_ZGP2, ">> zgp_sink_gp_pairing_configuration_extend %hd", (FMT__H, param));

  ZB_MAKE_ZGPD_ID(zgpd_id,
                  ZB_ZGP_GP_PAIRING_CONF_OPT_GET_APP_ID(conf->options),
                  conf->endpoint,
                  conf->zgpd_addr);

  if (zgp_sink_table_read(&zgpd_id, &ent) == RET_OK)
  {
    TRACE_MSG(TRACE_ZGP2, "Entry already exist", (FMT__0));

    /* If the Action sub-field of the Actions field is set to 0b001 and a Sink Table entry for this
     * GPD (and Endpoint, matching or 0x00 or 0xff, if ApplicationID = 0b010)  already exists, the
     * sink checks the match between the CommunicationMode in the GP Pairing Configuration command
     * and the Sink Table entry. If the existing entry contains different CommunicationMode, the
     * existing entry SHALL NOT be overwritten;
     */

    if (ZGP_TBL_SINK_GET_COMMUNICATION_MODE(&ent) == ZB_ZGP_GP_PAIRING_CONF_GET_COMMUNICATION_MODE(conf->options))
    {
      /* If the CommunicationMode does match, the sink checks the Number of paired endpoints field. */
      need_analyze_endpoints = ZB_TRUE;
      ent_p = &ent;
    }
    else
    {
      //Do nothing
    }
  }
  else
  {
    /* Both for Action sub-field equal to 0b001 if there is no Sink Table entry for this GPD ID
     * (and Endpoint, matching or 0x00 or 0xff, if ApplicationID = 0b010)  and 0b010, the sink
     * SHALL then analyze the Number of paired endpoints field.
     */
    need_analyze_endpoints = ZB_TRUE;
  }

  if (need_analyze_endpoints)
  {
    ret = zgp_sink_gp_pairing_configuration_analyze_paired_endpoints(param, ent_p);

    if (ret == RET_IGNORE)
    {
      ret = RET_OK;
    }
  }

  TRACE_MSG(TRACE_ZGP2, "<< zgp_sink_gp_pairing_configuration_extend %hd", (FMT__H, ret));
  return ret;
}

static void zgp_sink_gp_pairing_configuration_send_pairing(zb_uint8_t param)
{
  zb_zgp_gp_pairing_conf_t     *conf_p = ZB_BUF_GET_PARAM(param, zb_zgp_gp_pairing_conf_t);
  zb_zgp_gp_pairing_send_req_t *req;
  zb_zgp_gp_pairing_conf_t      conf;

  TRACE_MSG(TRACE_ZGP2, ">> zgp_sink_gp_pairing_configuration_send_pairing %hd", (FMT__H, param));

  ZB_MEMCPY(&conf, conf_p, sizeof(conf));

  ZB_ZGP_GP_PCONF_PAIRING_SEND_REQ_CREATE(param, req, &conf, NULL);

  {
    zb_bool_t add_sink = ZB_FALSE;
    zb_bool_t remove_gpd = ZB_FALSE;

    switch (ZB_ZGP_GP_PAIRING_CONF_GET_ACTIONS(conf.actions))
    {
      case ZGP_PAIRING_CONF_NO_ACTION:
      case ZGP_PAIRING_CONF_EXTEND:
      case ZGP_PAIRING_CONF_REPLACE:
        add_sink = ZB_TRUE;
        break;
      case ZGP_PAIRING_CONF_REMOVE_GPD:
        remove_gpd = ZB_TRUE;
        break;
    };

    ZB_ZGP_GP_PAIRING_MAKE_SEND_OPTIONS(req, add_sink, remove_gpd, 1);
  }

  zgp_cluster_send_gp_pairing(param);

  TRACE_MSG(TRACE_ZGP2, "<< zgp_sink_gp_pairing_configuration_send_pairing", (FMT__0));
}

static void zgp_sink_gp_pairing_configuration_replace_cont(zb_uint8_t param)
{
  zb_zgp_gp_pairing_conf_t *conf = ZB_BUF_GET_PARAM(param, zb_zgp_gp_pairing_conf_t);
  zb_ret_t                  ret;

  TRACE_MSG(TRACE_ZGP2, ">> zgp_sink_gp_pairing_configuration_replace_cont %hd", (FMT__H, param));

  /* Both for Action sub-field equal to 0b001 if there is no Sink Table entry for this GPD ID
   * (and Endpoint, matching or 0x00 or 0xff, if ApplicationID = 0b010)  and 0b010, the sink
   * SHALL then analyze the Number of paired endpoints field.
   */
  ret = zgp_sink_gp_pairing_configuration_analyze_paired_endpoints(param, NULL);

  if (ret == RET_IGNORE)
  {
    ret = RET_OK;
  }

  if (ZB_ZGP_GP_PAIRING_CONF_GET_SEND_PAIRING(conf->actions) && ret == RET_OK)
  {
    zgp_sink_gp_pairing_configuration_send_pairing(param);
  }
  else
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZGP2, "<< zgp_sink_gp_pairing_configuration_replace_cont", (FMT__0));
}

static zb_ret_t zgp_del_alias_from_entry(zgp_pair_group_list_t *group_list,
                                         zb_uint16_t            alias)
{
  zb_uint8_t i;
  zb_ret_t   ret = RET_NOT_FOUND;

  TRACE_MSG(TRACE_ZGP2, ">> zgp_del_alias_from_entry 0x%x",
            (FMT__D, alias));

  for (i = 0; i < ZB_ZGP_MAX_SINK_GROUP_PER_GPD; i++)
  {
    //search that table already have this group-alias pair
    if (group_list[i].alias == alias)
    {
      group_list[i].sink_group = ZGP_PROXY_GROUP_INVALID_IDX;
      group_list[i].alias = 0xffff;
      ret = RET_OK;
    }
  }

  TRACE_MSG(TRACE_ZGP2, "<< zgp_del_alias_from_entry %hd", (FMT__H, ret));
  return ret;
}

static void zgp_sink_gp_pairing_configuration_send_remove_pairing(zb_uint8_t param)
{
  zb_zgp_gp_pairing_conf_t *conf = ZB_BUF_GET_PARAM(param, zb_zgp_gp_pairing_conf_t);
  zb_zgpd_id_t              zgpd_id;
  zb_zgp_sink_tbl_ent_t     ent;

  TRACE_MSG(TRACE_ZGP2, ">> zgp_sink_gp_pairing_configuration_send_remove_pairing %hd", (FMT__H, param));

  ZB_MAKE_ZGPD_ID(zgpd_id,
                  ZB_ZGP_GP_PAIRING_CONF_OPT_GET_APP_ID(conf->options),
                  conf->endpoint,
                  conf->zgpd_addr);

  if (zgp_sink_table_read(&zgpd_id, &ent) == RET_OK)
  {
    zb_zgp_gp_pairing_send_req_t *req;

    //remove alias to be replaced
    {
      zb_uindex_t i;

      for (i = 0; i < ZB_ZGP_MAX_SINK_GROUP_PER_GPD; i++)
      {
        zgp_del_alias_from_entry(ent.u.sink.sgrp, conf->sgrp[i].alias);
      }
    }

    if (zgp_get_group_list_size(ent.u.sink.sgrp))
    {
      ZB_ZGP_GP_PAIRING_SEND_REQ_CREATE(param, req, &ent, NULL);
      ZB_ZGP_GP_PAIRING_MAKE_SEND_OPTIONS(req, 0, 0, 0);
      zgp_cluster_send_gp_pairing(param);
      param = 0;
    }
  }

  if (param)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZGP3, "<< zgp_sink_gp_pairing_configuration_send_remove_pairing", (FMT__0));
}

static void zgp_sink_gp_pairing_configuration_replace_dup(zb_uint8_t param, zb_uint16_t user_param)
{
  TRACE_MSG(TRACE_ZGP3, ">> zgp_sink_gp_pairing_configuration_replace_dup %hd %hd", (FMT__H_H, param, user_param));

  zb_buf_copy(param, user_param);
  zgp_sink_gp_pairing_configuration_send_remove_pairing(user_param);
  zgp_sink_gp_pairing_configuration_replace_cont(param);

  TRACE_MSG(TRACE_ZGP3, "<< zgp_sink_gp_pairing_configuration_replace_dup", (FMT__0));
}

/**
 * @brief Replace sink entry using new gp pairing configuration info
 *
 * @param param      [in]  Buffer reference
 *
 * @see ZGP spec, A.3.3.4.6
 */
static zb_ret_t zgp_sink_gp_pairing_configuration_replace(zb_uint8_t param)
{
  zb_zgp_gp_pairing_conf_t *conf = ZB_BUF_GET_PARAM(param, zb_zgp_gp_pairing_conf_t);
  zb_zgpd_id_t              zgpd_id;
  zb_zgp_sink_tbl_ent_t     ent;
  zb_ret_t                  ret = RET_ERROR;

  TRACE_MSG(TRACE_ZGP2, ">> zgp_sink_gp_pairing_configuration_replace %hd", (FMT__H, param));

  ZB_MAKE_ZGPD_ID(zgpd_id,
                  ZB_ZGP_GP_PAIRING_CONF_OPT_GET_APP_ID(conf->options),
                  conf->endpoint,
                  conf->zgpd_addr);

  if (ZB_ZGP_GP_PAIRING_CONF_GET_SEND_PAIRING(conf->actions))
  {
    if (zgp_sink_table_read(&zgpd_id, &ent) == RET_OK)
    {
      TRACE_MSG(TRACE_ZGP2, "Entry exist", (FMT__0));

      /* EES: we need allocate additional buffer for remove old entry and send GP Pairing with
         removing info */
      ret = RET_BLOCKED;
      zb_buf_get_out_delayed_ext(zgp_sink_gp_pairing_configuration_replace_dup, param, 0);
      param = 0;
    }
  }

  if (param)
  {
    zgp_sink_table_del(&zgpd_id);

    /* Both for Action sub-field equal to 0b001 if there is no Sink Table entry for this GPD ID
     * (and Endpoint, matching or 0x00 or 0xff, if ApplicationID = 0b010)  and 0b010, the sink
     * SHALL then analyze the Number of paired endpoints field.
     */
    ret = zgp_sink_gp_pairing_configuration_analyze_paired_endpoints(param, NULL);

    if (ret == RET_IGNORE)
    {
      ret = RET_OK;
    }
  }

  TRACE_MSG(TRACE_ZGP2, "<< zgp_sink_gp_pairing_configuration_replace %hd", (FMT__H, ret));
  return ret;
}

/**
 * @brief Remove pairing using new gp pairing configuration info
 *
 * @param param      [in]  Buffer reference
 *
 * @see ZGP spec, A.3.3.4.6
 */
static void zgp_sink_gp_pairing_configuration_remove_pairing(zb_uint8_t param)
{
  zb_zgp_gp_pairing_conf_t *conf_p = ZB_BUF_GET_PARAM(param, zb_zgp_gp_pairing_conf_t);
  zb_zgpd_id_t              zgpd_id;
  zb_zgp_sink_tbl_ent_t     ent;

  TRACE_MSG(TRACE_ZGP2, ">> zgp_sink_gp_pairing_configuration_remove_pairing %hd", (FMT__H, param));

  /* If the Action sub-field of the Actions field is set to 0b011, the sink SHALL check if it has
   * Sink Table entry for the supplied SrcID/GPD IEEE address (and Endpoint, matching or 0x00 or
   * 0xff, if Applica-tionID = 0b010)  with the supplied CommunicationMode and, in case of groupcast
   * CommunicationMode, the supplied GroupID. If yes, this pairing SHALL be removed. In case of
   * groupcast, the sink SHALL remove its Green Power EndPoint as a member of this group at APS
   * level.
   */
  ZB_MAKE_ZGPD_ID(zgpd_id,
                  ZB_ZGP_GP_PAIRING_CONF_OPT_GET_APP_ID(conf_p->options),
                  conf_p->endpoint,
                  conf_p->zgpd_addr);

  if (zgp_sink_table_read(&zgpd_id, &ent) == RET_OK)
  {
    zb_bool_t need_to_delete = ZB_TRUE;

    if (ZGP_TBL_SINK_GET_COMMUNICATION_MODE(&ent) == ZGP_COMMUNICATION_MODE_GROUPCAST_PRECOMMISSIONED)
    {
      zb_uint8_t  i;
      zb_uint8_t  apsme_group_cnt = 0;
      zb_uint16_t apsme_groups[ZB_ZGP_MAX_SINK_GROUP_PER_GPD];

      for (i = 0; i < ZB_ZGP_MAX_SINK_GROUP_PER_GPD; i++)
      {
        zb_ret_t res = zgp_del_group_from_entry(ent.u.sink.sgrp,
                                                conf_p->sgrp[i].sink_group);

        if (res == RET_OK)
        {
          apsme_groups[apsme_group_cnt++] = conf_p->sgrp[i].sink_group;
        }
      }

      if (zgp_get_group_list_size(ent.u.sink.sgrp))
      {
        need_to_delete = ZB_FALSE;

        if (zgp_sink_table_write(&zgpd_id, &ent) == RET_OK)
        {
          for (i = 0; i < apsme_group_cnt; i++)
          {
            TRACE_MSG(TRACE_ZGP2, "unbind sink group: 0x%x", (FMT__D, apsme_groups[i]));
            zb_apsme_remove_group_internal(apsme_groups[i], ZGP_ENDPOINT);
          }
        }
      }
    }

    if (need_to_delete)
    {
      zgp_sink_table_del(&zgpd_id);
    }

    if (ZB_ZGP_GP_PAIRING_CONF_GET_SEND_PAIRING(conf_p->actions))
    {
      zb_zgp_gp_pairing_conf_t      conf;
      zb_zgp_gp_pairing_send_req_t *req;

      ZB_MEMCPY(&conf, conf_p, sizeof(zb_zgp_gp_pairing_conf_t));
      ZB_ZGP_GP_PCONF_PAIRING_SEND_REQ_CREATE(param, req, &conf, NULL);
      ZB_ZGP_GP_PAIRING_MAKE_SEND_OPTIONS(req, 0, 0, 0);
      zgp_cluster_send_gp_pairing(param);
      param = 0;
    }
  }

  if (param)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZGP2, "<< zgp_sink_gp_pairing_configuration_remove_pairing", (FMT__0));
}

/**
 * @brief Remove GPD from sink table using new gp pairing configuration info
 *
 * @param param      [in]  Buffer reference
 *
 * @see ZGP spec, A.3.3.4.6
 */
static void zgp_sink_gp_pairing_configuration_remove_gpd(zb_uint8_t param)
{
  zb_zgp_gp_pairing_conf_t *conf_p = ZB_BUF_GET_PARAM(param, zb_zgp_gp_pairing_conf_t);
  zb_zgpd_id_t              zgpd_id;

  TRACE_MSG(TRACE_ZGP2, ">> zgp_sink_gp_pairing_configuration_remove_gpd %hd", (FMT__H, param));

  ZB_MAKE_ZGPD_ID(zgpd_id,
                  ZB_ZGP_GP_PAIRING_CONF_OPT_GET_APP_ID(conf_p->options),
                  conf_p->endpoint,
                  conf_p->zgpd_addr);

  /* If the Action sub-field of the Actions field is set to 0b100, the sink SHALL remove all the
   * Sink Table entry(s) for this GPD and Endpoint, matching or 0x00 or 0xff, if ApplicationID =
   * 0b010 , if they exist. For all the pairings that were for groupcast, the sink SHALL remove its
   * Green Power EndPoint as a member of the group at APS level.
   */
  zgp_sink_table_del(&zgpd_id);

  if (ZB_ZGP_GP_PAIRING_CONF_GET_SEND_PAIRING(conf_p->actions))
  {
    zb_zgp_gp_pairing_conf_t      conf;
    zb_zgp_gp_pairing_send_req_t *req;

    ZB_MEMCPY(&conf, conf_p, sizeof(zb_zgp_gp_pairing_conf_t));
    ZB_ZGP_GP_PCONF_PAIRING_SEND_REQ_CREATE(param, req, &conf, NULL);
    ZB_ZGP_GP_PAIRING_MAKE_SEND_OPTIONS(req, 0, 1, 0);
    zgp_cluster_send_gp_pairing(param);
  }
  else
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZGP2, "<< zgp_sink_gp_pairing_configuration_remove_gpd", (FMT__0));
}

void zgp_sink_handle_gp_pairing_configuration(zb_uint8_t param)
{
  zb_zgp_gp_pairing_conf_t *conf = ZB_BUF_GET_PARAM(param, zb_zgp_gp_pairing_conf_t);

  TRACE_MSG(TRACE_ZGP2, ">> zgp_sink_handle_gp_pairing_configuration %hd", (FMT__H, param));

  /* A.3.5.2.4.1 A received GP Pairing Configuration command carrying SrcID = 0x00000000 (if
   * ApplicationID = 0b000) or GPD IEEE address 0x0000000000000000 (if ApplicationID = 0b010) SHALL
   * be silently dropped; Sink Table entry SHALL NOT be created or updated.
   */

  if (ZB_ZGP_GP_PAIRING_CONF_OPT_GET_APP_ID(conf->options) == ZB_ZGP_APP_ID_0000)
  {
    if (conf->zgpd_addr.src_id == ZB_ZGP_SRC_ID_UNSPECIFIED)
    {
      zb_buf_free(param);
      param = 0;
    }
  }
  else
  {
    if (ZB_IEEE_ADDR_IS_ZERO(conf->zgpd_addr.ieee_addr))
    {
      zb_buf_free(param);
      param = 0;
    }
  }

  if (param)
  {
    /* A.3.5.2.4.1 GP Pairing Configuration command with SrcID = 0xffffffff (if ApplicationID = 0b000) or
     * GPD IEEE address 0xffffffffffffffff (if ApplicationID = 0b010) denotes a pairing for all GPD
     * with a particular ApplicationID and SHALL be created if there is space in the Sink Table.
     */
    switch (ZB_ZGP_GP_PAIRING_CONF_GET_ACTIONS(conf->actions))
    {
      case ZGP_PAIRING_CONF_NO_ACTION:
        zgp_sink_gp_pairing_configuration_no_action(param);
        param = 0;
        break;
      case ZGP_PAIRING_CONF_EXTEND:
      case ZGP_PAIRING_CONF_REPLACE:
      {
        zb_ret_t   ret = RET_IGNORE;
        zb_uint8_t sec_lvl = 0;

        if (ZB_ZGP_GP_PAIRING_CONF_GET_SEC_USE(conf->options))
        {
          sec_lvl = ZGP_SINK_GET_SEC_LEVEL(conf->sec_options);
        }

        TRACE_MSG(TRACE_ZGP3, "sec_lvl: %d", (FMT__D, sec_lvl));

        /* If the Action sub-field of the Actions field was set to 0b000, 0b001 or 0b010 and the
         * SecurityLevel field of the SecurityUse field is set to 0b01, the sink SHALL NOT update
         * (if existent) nor create a Sink Table entry for this GPD ID (and Endpoint, matching or
         * 0x00 or 0xff, if ApplicationID = 0b010).
         */

        /* 15-02014-005-GP_Errata_for_GP Basic_specification_14-0563.docx: For Action sub-field
         * equal to 0b001 or 0b010, the sink starts as follows. The sink checks if it supports the
         * SecurityLevel requested (i.e., if it is higher than or equal to the gpsSecurityLevel) and
         * if it supports the requested CommunicationMode (as indicated in the
         * gpsFunctionality/gpsActiveFunctionality attribute). If either of those checks fails, it
         * drops the frame; Sink Table and Translation Table is not modified. If the command was
         * sent in unicast, it MAY send ZCL Default Response Command with the Status code field
         * indicating FAILURE (see [3]). If both checks succeed, the sink proceeds as follows,
         * depending on the Action sub-field value.
         */
        if (sec_lvl != ZB_ZGP_SEC_LEVEL_REDUCED &&
            sec_lvl >= ZGP_GPS_GET_SECURITY_LEVEL() &&
            ZB_ZGP_SINK_IS_SUPPORT_COMMUNICATION_MODE(ZB_ZGP_GP_PAIRING_CONF_GET_COMMUNICATION_MODE(conf->options)))
        {
          if (ZB_ZGP_GP_PAIRING_CONF_GET_ACTIONS(conf->actions) == ZGP_PAIRING_CONF_EXTEND)
          {
            ret = zgp_sink_gp_pairing_configuration_extend(param);
          }
          else
          {
            ret = zgp_sink_gp_pairing_configuration_replace(param);

            if (ret == RET_BLOCKED)
            {
              param = 0;
            }
          }
        }

        if (ZB_ZGP_GP_PAIRING_CONF_GET_SEND_PAIRING(conf->actions) && ret == RET_OK)
        {
          zgp_sink_gp_pairing_configuration_send_pairing(param);
          param = 0;
        }
        break;
      }
      case ZGP_PAIRING_CONF_REMOVE_PAIRING:
      {
        zgp_sink_gp_pairing_configuration_remove_pairing(param);
        param = 0;
        break;
      }
      case ZGP_PAIRING_CONF_REMOVE_GPD:
      {
        zgp_sink_gp_pairing_configuration_remove_gpd(param);
        param = 0;
        break;
      }
    };
  }

  if (param)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZGP2, "<< zgp_sink_handle_gp_pairing_configuration", (FMT__0));
}

void zgp_sink_handle_gp_sink_commissioning_mode(zb_uint8_t param)
{
  zb_zgp_gp_sink_comm_mode_t *req = ZB_BUF_GET_PARAM(param, zb_zgp_gp_sink_comm_mode_t);

  TRACE_MSG(TRACE_ZGP2, ">> zgp_sink_handle_gp_sink_commissioning_mode %hd", (FMT__H, param));

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
      TRACE_MSG(TRACE_ZGP3, "Proxy part of combo already in commissioning mode, drop.", (FMT__0));
    }
    else
#endif  /* ZB_ENABLE_ZGP_PROXY */
    {
      ZB_ZGP_SET_SINK_COMM_MODE(req->options);

      if (ZB_ZGP_GP_SINK_COMM_MODE_GET_ACTION(req->options))
      {
        zb_zgps_start_commissioning(ZGP_GPS_GET_COMMISSIONING_WINDOW() * ZB_TIME_ONE_SECOND);
      }
      else
      {
        zb_zgps_stop_commissioning();
      }
    }
  }

  if (param)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZGP2, "<< zgp_sink_handle_gp_sink_commissioning_mode", (FMT__0));
}

void zgp_sink_handle_gp_comm_notification_req(zb_uint8_t param)
{
  zb_zgp_gp_comm_notification_req_t  req;
  zb_gpdf_info_t                    *gpdf_info;

  TRACE_MSG(TRACE_ZGP2, ">> zgp_sink_handle_gp_comm_notification_req %hd", (FMT__H, param));

  req.payload[0] = 0;

  ZB_MEMCPY(&req, ZB_BUF_GET_PARAM(param, zb_zgp_gp_comm_notification_req_t),
            sizeof(zb_zgp_gp_comm_notification_req_t));

  zb_buf_reuse(param);

  if (req.payload[0] > MAX_ZGP_CLUSTER_GPDF_PAYLOAD_SIZE)
  {
    TRACE_MSG(TRACE_ERROR, "Invalid payload len %hd", (FMT__H, req.payload[0]));
    zb_buf_free(param);
    return;
  }

  gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);

  ZB_BZERO(gpdf_info, sizeof(zb_gpdf_info_t));

  gpdf_info->zgpd_id.app_id = ZB_ZGP_COMM_NOTIF_OPT_GET_APP_ID(req.options);

  if (ZB_ZGP_COMM_NOTIF_OPT_GET_APP_ID(req.options) == ZB_ZGP_APP_ID_0000)
  {
    gpdf_info->zgpd_id.addr.src_id = req.zgpd_addr.src_id;
  }
  else
  {
    ZB_MEMCPY(&gpdf_info->zgpd_id.addr, &req.zgpd_addr, sizeof(zb_zgpd_addr_t));
    gpdf_info->zgpd_id.endpoint = req.endpoint;
  }

  if (ZB_ZGP_GP_COMM_NOTIF_OPT_GET_SECUR_FAILED(req.options))
  {
    //15-02014-005-GP_Errata_for_GP Basic_specification_14-0563.docx
    /* On reception of GP Commissioning Notification command with Security processing failed
     * sub-field of the Options field set to 0b1, thus carrying encrypted GPD CommandID and GPD
     * Command payload, and the corresponding MIC field, the sink takes the following values to
     * reconstruct the Frame Control field and Extended Frame Control field, required for
     * decryption:
         - Sub-fields of the Frame Control field:
             - Frame type = 0b00 (since according to the current specification, a Maintenance GPDF cannot use security);
             - Zigbee Protocol Version = 0x3 (fixed value);
             - Auto-Commissioning = 0b0 (according to the current specification);
             - NWK Frame Control Extension = 0b1 (implicit, since security was used);
         - Sub-fields of the Extended Frame Control field:
             - ApplicationID sub-field is copied from the ApplicationID sub-field of the Options field of the GP Commissioning Notification;
             - Security Level sub-field is copied from the SecurityLevel sub-field of the Options field of the GP Commissioning Notification;
             - Security Key sub-field is derived from the SecurityKeyType sub-field of the Options field of the GP Commissioning Notification (see Table 12);
             - RxAfterTx sub-field is copied from the RxAfterTx sub-field of the Options field of the GP Commissioning Notification;
             - Direction = 0b0 (implicit; GPD frames sent to the GPD are not forwarded).
    */
    ZB_GPDF_NWK_FRAME_CONTROL(gpdf_info->nwk_frame_ctl, ZGP_FRAME_TYPE_DATA, 0, 1);
  }
  else
  {
    if (req.gpd_cmd_id == ZB_GPDF_CMD_CHANNEL_REQUEST)
    {
      TRACE_MSG(TRACE_ZGP1, "Looks like it was a channel request frame", (FMT__0));
      ZB_GPDF_NWK_FRAME_CONTROL(gpdf_info->nwk_frame_ctl, ZGP_FRAME_TYPE_MAINTENANCE, 0, 0);
    }
    else
    {
      if (req.gpd_cmd_id < ZB_GPDF_CMD_COMMISSIONING)
      {
        /* Sinse proxy sent us that frame, it had auto-commissioning bit set */
        TRACE_MSG(TRACE_ZGP1, "Looks like it was an auto-commissioning data frame", (FMT__0));
        ZB_GPDF_NWK_FRAME_CONTROL(gpdf_info->nwk_frame_ctl, ZGP_FRAME_TYPE_DATA, 1, 0);
      }
      else
      {
        TRACE_MSG(TRACE_ZGP1, "Looks like it was a commissioning frame", (FMT__0));
        ZB_GPDF_NWK_FRAME_CONTROL(gpdf_info->nwk_frame_ctl, ZGP_FRAME_TYPE_DATA, 0, 0);
      }
    }
  }
  ZB_GPDF_NWK_FRAME_CTL_EXT(gpdf_info->nwk_ext_frame_ctl,
                            ZB_ZGP_COMM_NOTIF_OPT_GET_APP_ID(req.options),
                            ZB_ZGP_GP_COMM_NOTIF_OPT_GET_SEC_LVL(req.options),
                            ZGP_KEY_TYPE_IS_INDIVIDUAL(ZB_ZGP_GP_COMM_NOTIF_OPT_GET_KEY_TYPE(req.options)),
                            ZB_ZGP_GP_COMM_NOTIF_OPT_GET_RX_AFTER_TX(req.options),
                            0);

  if (gpdf_info->nwk_ext_frame_ctl)
  {
    gpdf_info->nwk_frame_ctl |= 0x80;
  }

  ZB_ZGP_CLUSTER_SET_DUP_COUNTER(req.gpd_sec_frame_counter, gpdf_info);
  gpdf_info->zgpd_cmd_id = req.gpd_cmd_id;

  gpdf_info->mac_addr_flds.proxy_info.short_addr = req.proxy_info.short_addr;
  gpdf_info->mac_addr_flds.proxy_info.link = req.proxy_info.link;

  if ((req.payload[0] && req.payload[0] != 0xff) ||
      ZB_ZGP_GP_COMM_NOTIF_OPT_GET_SECUR_FAILED(req.options))
  {
    zb_uint8_t *ptr;
    zb_uint32_t initial_alloc_size = 0;

    zb_uint8_t mic_present = ZB_ZGP_COMM_NOTIF_OPT_GET_MIC_PRESENT(req.options);

    if (mic_present)
    {
      initial_alloc_size += sizeof(gpdf_info->nwk_frame_ctl);
      initial_alloc_size += sizeof(gpdf_info->nwk_ext_frame_ctl);

      if (ZB_ZGP_COMM_NOTIF_OPT_GET_APP_ID(req.options) == ZB_ZGP_APP_ID_0000)
      {
        initial_alloc_size += sizeof(gpdf_info->zgpd_id.addr.src_id);
      }
      else
      {
        initial_alloc_size += sizeof(gpdf_info->zgpd_id.endpoint);
      }

      initial_alloc_size += sizeof(gpdf_info->sec_frame_counter);

      gpdf_info->nwk_hdr_len = initial_alloc_size;

      initial_alloc_size += sizeof(gpdf_info->zgpd_cmd_id);
      initial_alloc_size += sizeof(req.mic);
    }

    if (req.payload[0] && req.payload[0] != 0xff)
    {
      initial_alloc_size += req.payload[0];
    }

    ptr = zb_buf_initial_alloc(param, initial_alloc_size);

    if (mic_present)
    {
      ZB_ZCL_PACKET_PUT_DATA8(ptr, gpdf_info->nwk_frame_ctl);
      ZB_ZCL_PACKET_PUT_DATA8(ptr, gpdf_info->nwk_ext_frame_ctl);

      if (ZB_ZGP_COMM_NOTIF_OPT_GET_APP_ID(req.options) == ZB_ZGP_APP_ID_0000)
      {
        ZB_ZCL_PACKET_PUT_DATA32_VAL(ptr, gpdf_info->zgpd_id.addr.src_id);
      }
      else
      {
        ZB_ZCL_PACKET_PUT_DATA8(ptr, gpdf_info->zgpd_id.endpoint);
      }
      ZB_ZCL_PACKET_PUT_DATA32_VAL(ptr, gpdf_info->sec_frame_counter);
      ZB_ZCL_PACKET_PUT_DATA8(ptr, gpdf_info->zgpd_cmd_id);
    }

    if (req.payload[0] && req.payload[0] != 0xff)
    {
      ZB_ZCL_PACKET_PUT_DATA_N(ptr, &req.payload[1], req.payload[0]);
    }

    if (mic_present)
    {
      ZB_ZCL_PACKET_PUT_DATA32_VAL(ptr, req.mic);
    }
  }

  if (ZB_ZGP_GP_COMM_NOTIF_OPT_GET_SECUR_FAILED(req.options))
  {
    gpdf_info->status = GP_DATA_IND_STATUS_AUTH_FAILURE;
  }

  ZB_SCHEDULE_CALLBACK(zb_dgp_data_ind, param);

  TRACE_MSG(TRACE_ZGP2, "<< zgp_sink_handle_gp_comm_notification_req", (FMT__0));
}

void zgp_sink_handle_gp_notification_req(zb_uint8_t param)
{
  zb_zgp_gp_notification_req_t  req;
  zb_gpdf_info_t               *gpdf_info;

  TRACE_MSG(TRACE_ZGP2, ">> zgp_sink_handle_gp_notification_req %hd", (FMT__H, param));

  req.payload[0] = 0;

  ZB_MEMCPY(&req, ZB_BUF_GET_PARAM(param, zb_zgp_gp_notification_req_t),
            sizeof(zb_zgp_gp_notification_req_t));

  zb_buf_reuse(param);

  if (req.payload[0] > MAX_ZGP_CLUSTER_GPDF_PAYLOAD_SIZE)
  {
    TRACE_MSG(TRACE_ERROR, "Invalid payload len %hd", (FMT__H, req.payload[0]));
    zb_buf_free(param);
    return;
  }

  if (req.payload[0] && req.payload[0] != 0xff)
  {
    zb_uint8_t *ptr;
    ptr = zb_buf_initial_alloc(param, req.payload[0]);
    ZB_ZCL_PACKET_PUT_DATA_N(ptr, &req.payload[1], req.payload[0]);
  }

  gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);

  ZB_BZERO(gpdf_info, sizeof(zb_gpdf_info_t));

  gpdf_info->zgpd_id.app_id = ZB_ZGP_GP_NOTIF_OPT_GET_APP_ID(req.options);

  if (gpdf_info->zgpd_id.app_id == ZB_ZGP_APP_ID_0000)
  {
    gpdf_info->zgpd_id.addr.src_id = req.zgpd_addr.src_id;
  }
  else
  {
    ZB_MEMCPY(&gpdf_info->zgpd_id.addr, &req.zgpd_addr, sizeof(zb_zgpd_addr_t));
    gpdf_info->zgpd_id.endpoint = req.endpoint;
  }

  ZB_GPDF_NWK_FRAME_CONTROL(gpdf_info->nwk_frame_ctl, ZGP_FRAME_TYPE_DATA, 0, 0);
  ZB_GPDF_NWK_FRAME_CTL_EXT(gpdf_info->nwk_ext_frame_ctl,
                            ZB_ZGP_GP_NOTIF_OPT_GET_APP_ID(req.options),
                            ZB_ZGP_GP_NOTIF_OPT_GET_SEC_LVL(req.options),
                            ZGP_KEY_TYPE_IS_INDIVIDUAL(ZB_ZGP_GP_NOTIF_OPT_GET_KEY_TYPE(req.options)),
                            ZB_ZGP_GP_NOTIF_OPT_GET_RX_AFTER_TX(req.options),
                            ZB_ZGP_GP_NOTIF_OPT_GET_BIDIR_CAP(req.options));

  if (gpdf_info->nwk_ext_frame_ctl)
  {
    gpdf_info->nwk_frame_ctl |= 0x80;
  }

  ZB_ZGP_CLUSTER_SET_DUP_COUNTER(req.gpd_sec_frame_counter, gpdf_info);
  gpdf_info->zgpd_cmd_id = req.gpd_cmd_id;

  if (ZB_ZGP_GP_NOTIF_OPT_GET_RECV_AS_UNICAST(req.options))
  {
    gpdf_info->recv_as_unicast = 1;
  }

  gpdf_info->mac_addr_flds.proxy_info.short_addr = req.proxy_info.short_addr;
  gpdf_info->mac_addr_flds.proxy_info.link = req.proxy_info.link;

  ZB_SCHEDULE_CALLBACK(zb_dgp_data_ind, param);

  TRACE_MSG(TRACE_ZGP2, "<< zgp_sink_handle_gp_notification_req", (FMT__0));
}
#endif /* ZB_ENABLE_ZGP_SINK */

#ifdef ZB_ENABLE_ZGP_DIRECT
zb_uint8_t zb_zgp_cluster_encode_link_quality(zb_int8_t rssi, zb_uint8_t lqi)
{
  zb_uint8_t res = 0;

  TRACE_MSG(TRACE_ZGP3, ">> zb_zgp_cluster_encode_link_quality rssi: %d, lqi: %d", (FMT__D_D, rssi, lqi));

/* The RSSI sub-field of the GPP-GPD link field encodes the RSSI
   from the  range <+8 ; -109> [dBm], with 2dBm granularity.
   It SHALL be calculated as follows:
   - The RSSI parameter value as supplied by the dGP-DATA.indication primitive
     SHALL be  capped to the  range <+8 ; -109> [dBm],
     i.e. any value higher than +8dBm is represented as +8 dBm;
     any value lower than  -109dBm is represented as -109dBm,
     the values within the range remain unmodified;
   - 110 SHALL be added to the capped RSSI value, to obtain a non-negative value;
   - The obtained non-negative RSSI value SHALL be divided by 2.
*/
  if (rssi > 8)
  {
    rssi = 8;
  }
  else
  if (rssi < -109)
  {
    rssi = -109;
  }
  rssi += 110;
  rssi /= 2;

  res |= rssi;

  /* TODO: set correct LQI */
/* The Link quality sub-field of the GPP-GPD link field encodes the quality of the link
   between the GPD and the forwarding proxy, as defined in Table 32.
   Its calculation is vendor-specific and may be based e.g. on LQI or correlation value.
*/
  if (lqi == 255)
  {
    lqi = 3;
  }
  else
  if (lqi > 135)
  {
    lqi = 2;
  }
  else
  if (lqi > 11)
  {
    lqi = 1;
  }
  else
  {
    lqi = 0;
  }
  res |= (lqi << 6);

  return res;
}
#endif  /* ZB_ENABLE_ZGP_DIRECT */

#endif  /* #ifdef ZB_ENABLE_ZGP */
