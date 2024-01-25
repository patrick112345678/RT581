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
/* PURPOSE: ZDO network management functions, client side
*/

#define ZB_TRACE_FILE_ID 2094
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_hash.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zdo_common.h"
#include "zb_magic_macros.h"


/*! \addtogroup ZB_ZDO */
/*! @{ */

/* Mgmt NWK update handling functions were under ZB_ROUTER_ROLE ifdef,
 * but were switched on for all devices types (for WWAH, PICS item AZD514) */

#ifdef ZB_MGMT_NWK_ENHANCED_UPDATE_ENABLED
static void zdo_send_mgmt_nwk_unsol_enh_update_notify(zb_uint8_t param, zb_callback_t cb);
#endif

#ifndef ZB_ED_ROLE
static void zb_zdo_check_channel_conditions_alarm(zb_uint8_t unused);
static void zdo_perform_ed_scan(zb_uint8_t param);
static void zdo_ed_scan_completed_channel_manager(zb_uint8_t param);
#endif /* ZB_ED_ROLE */

#if !defined ZB_ED_ROLE || defined ZB_MGMT_NWK_ENHANCED_UPDATE_ENABLED
static void zb_zdo_check_fails_finish_cb(zb_uint8_t param);
static void zb_zdo_clear_channel_check_flag(zb_uint8_t unused);
#endif /* !ZB_ED_ROLE || ZB_MGMT_NWK_ENHANCED_UPDATE_ENABLED */

void zb_zdo_channel_change_timer_cb(zb_uint8_t param);
void zb_zdo_long_timer_cb(zb_uint8_t param);
void zb_zdo_start_long_timer(zb_callback_t func, zb_uint8_t param, zb_uint8_t long_timer);

void zb_zdo_check_fails_cont(zb_uint8_t param);

void zb_zdo_check_fails(zb_uint8_t param)
{
  ZVUNUSED(param);

  TRACE_MSG(TRACE_MAC2, ">> zb_zdo_check_fails tx total %d fail %d", (FMT__D_D, ZB_NIB_NWK_TX_TOTAL(), ZB_NIB_NWK_TX_FAIL()));

  if (ZB_NIB_NWK_TX_TOTAL() >= ZB_TX_TOTAL_THRESHOLD && !ZB_U2B(ZB_ZDO_GET_CHECK_FAILS()))
  {
    TRACE_MSG(TRACE_MAC2, "checking for tx fails vs treshold", (FMT__0));
    if (ZB_NIB_NWK_TX_FAIL() > (ZB_NIB_NWK_TX_TOTAL() / ZB_FAILS_PERCENTAGE))
    {
      if (zb_buf_get_out_delayed(zb_zdo_check_fails_cont) == RET_OK)
      {
        ZB_ZDO_SET_CHECK_FAILS();
      }
    }
  }

  TRACE_MSG(TRACE_MAC2, "<< zb_zdo_check_fails", (FMT__0));
}


/* Well, this is slightly different implementation than declared in R22, Annex E:
 *
 * Our implementation is based on SE1.4 specification,
 * Zigbee document 07-53560-21
 *
 * 5.14.7 Frequency Agility
 * \t 2. Each end device shall use the
 * Mgmt_NWK_Unsolicited_Enhanced_Update_notify command to notify the
 * Coordinator's Network Manager of problems with its link.
 *
 * 5.14.7.1 BOMDs, 5.14.7.2 Other Devices (e.g IHDs):
 * \t When the device has determined that poor link quality exists, it shall
 * unicast a Mgmt_NWK_Unsolicited_Enhanced_Update_notifu command to inform the
 * Coordinator's Network Manager of problems with its link.
 *
 * 5.14.7.3 Network Manager
 * \t 1. As per [B3] Annex E, only the Coordinator's Network Manager shall
 * take the decision to perform a channel Energy scan.
 *
 * \t 2. As per [B3] Annex E, only the Coordinator's Network Manager shall
 * take the decision to change channel.
 *
 * So, send only Ehnaced_Update_notify and clear statistics
 */
static void zdo_get_mac_diag_info_cb(zb_uint8_t param);

void zb_zdo_check_fails_cont(zb_uint8_t param)
{
  if (ZB_PIBCACHE_NETWORK_ADDRESS() != ZB_NIB_NWK_MANAGER_ADDR())
  {
    zb_nwk_pib_get(param, ZB_PIB_ATTRIBUTE_IEEE_DIAGNOSTIC_INFO, zdo_get_mac_diag_info_cb);
  }
  else
  {
    /* This is not a check for role, but just a size optimization. */
#ifndef ZB_ED_ROLE
    if (!ZB_ZDO_NETWORK_MGMT_CHANNEL_UPDATE_IS_DISABLED())
    {
      ZB_SCHEDULE_CALLBACK(zb_zdo_check_channel_conditions, param);
    }
    else
    {
      TRACE_MSG(TRACE_ZDO3, "Network update is not permitted", (FMT__0));
      ZB_ZDO_CLEAR_CHECK_FAILS();
      zb_buf_free(param);
    }
#else
    TRACE_MSG(TRACE_ERROR, "End device is NWK manager here!", (FMT__0));
#endif /* ZB_ED_ROLE */
  }
}


static void zdo_get_mac_diag_info_cb(zb_uint8_t param)
{
  zb_mac_diagnostic_info_t *diag_info_src;
  zb_mac_diagnostic_info_t *diag_info_dst;
  zb_mlme_get_confirm_t *conf;
  zb_mac_status_t status;

  conf = (zb_mlme_get_confirm_t *)zb_buf_begin(param);
  status = conf->status;
  (void)zb_buf_cut_left(param, sizeof(zb_mlme_get_confirm_t));

  /* Replace diag info into the tail */
  diag_info_dst = ZB_BUF_GET_PARAM(param, zb_mac_diagnostic_info_t);

  if (status == MAC_SUCCESS)
  {
    diag_info_src = (zb_mac_diagnostic_info_t *)zb_buf_begin(param);
    ZB_MEMCPY(diag_info_dst, diag_info_src, sizeof(zb_mac_diagnostic_info_t));
  }
  else
  {
    ZB_BZERO(diag_info_dst, sizeof(zb_mac_diagnostic_info_t));
  }
#ifdef ZB_MGMT_NWK_ENHANCED_UPDATE_ENABLED
  zdo_send_mgmt_nwk_unsol_enh_update_notify(param, zb_zdo_check_fails_finish_cb);
#else
  zb_buf_free(param);
#endif
}

#ifdef ZB_MGMT_NWK_ENHANCED_UPDATE_ENABLED

static void zdo_send_mgmt_nwk_unsol_enh_update_notify(zb_uint8_t param,
                                                           zb_callback_t cb)
{
  zb_ret_t ret;
  zb_uint8_t channel_number;
  zb_zdo_mgmt_nwk_unsol_enh_update_notify_param_t *req_param;
  zb_mac_diagnostic_info_t mac_diag_info;

  TRACE_MSG(TRACE_ZDO1, ">> zb_zdo_send_mgmt_nwk_unsol_enh_update_notify param %hd",
            (FMT__H, param));

  ZB_MEMCPY(&mac_diag_info, ZB_BUF_GET_PARAM(param, zb_mac_diagnostic_info_t),
            sizeof(zb_mac_diagnostic_info_t));

  req_param = ZB_BUF_GET_PARAM(param,
                               zb_zdo_mgmt_nwk_unsol_enh_update_notify_param_t);

  req_param->notification.status = ZB_ZDP_STATUS_SUCCESS;

  /* Set channel */
  ZB_CHANNEL_PAGE_SET_PAGE(req_param->notification.channel_in_use, ZB_PIBCACHE_CURRENT_PAGE());
  ret = zb_channel_page_channel_logical_to_number(ZB_PIBCACHE_CURRENT_PAGE(), ZB_PIBCACHE_CURRENT_CHANNEL(),
                                                  &channel_number);
  ZB_ASSERT(ret == RET_OK);
  ZB_CHANNEL_PAGE_SET_MASK(req_param->notification.channel_in_use, (1UL << channel_number));

  req_param->notification.mac_tx_ucast_total = mac_diag_info.mac_tx_ucast_total;
  req_param->notification.mac_tx_ucast_failures = mac_diag_info.mac_tx_ucast_failures;
  req_param->notification.mac_tx_ucast_retries = mac_diag_info.mac_tx_ucast_retries;
  req_param->notification.period = mac_diag_info.period_of_time;

  req_param->addr = ZB_NIB_NWK_MANAGER_ADDR();

  zb_zdo_mgmt_nwk_unsol_enh_update_notify(param, cb);

  TRACE_MSG(TRACE_ZDO1, "<< zb_zdo_send_mgmt_nwk_unsol_enh_update_notify", (FMT__0));
}

#endif /* ZB_MGMT_NWK_ENHANCED_UPDATE_ENABLED */

#ifndef ZB_ED_ROLE

void zb_zdo_check_channel_conditions(zb_uint8_t param)
{
  zb_ret_t ret;
  zb_time_t dummy_time;

  TRACE_MSG(TRACE_ZDO1, ">> zb_zdo_check_channel_conditions param %hd",
            (FMT__H, param));

  /* Check 24hrs limit */
  ret = ZB_SCHEDULE_GET_ALARM_TIME(zb_zdo_check_channel_conditions_alarm, 0, &dummy_time);
  if (ret == RET_NOT_FOUND)
  {
    ZB_SCHEDULE_CALLBACK(zdo_perform_ed_scan, param);

    ZB_SCHEDULE_ALARM(zb_zdo_check_channel_conditions_alarm, 0,
                      ZB_ZDO_CHECK_CHANNEL_TIMEOUT);
  }
  else
  {
    TRACE_MSG(TRACE_ZDO1, "zb_zdo_check_channel_conditions_timeout not expired yet!", (FMT__0));
    zb_zdo_clear_channel_check_flag(0);
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZDO1, "<< zb_zdo_check_channel_conditions", (FMT__0));
}

static void zb_zdo_check_channel_conditions_alarm(zb_uint8_t unused)
{
  /* Do nothing */
  ZVUNUSED(unused);
  TRACE_MSG(TRACE_ZDO1, "zb_zdo_check_channel_conditions_alarm timeout expired", (FMT__0));
}

#endif /* ZB_ED_ROLE */

#if !defined ZB_ED_ROLE || defined ZB_MGMT_NWK_ENHANCED_UPDATE_ENABLED

static void zb_zdo_clear_channel_check_flag(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  if (ZB_U2B(ZB_ZDO_GET_CHECK_FAILS()))
  {
    ZB_SCHEDULE_ALARM(zb_zdo_channel_check_timer_cb, 0, ZB_ZDO_CHECK_FAILS_CLEAR_TIMEOUT);
  }
}

static void zb_zdo_check_fails_finish_cb(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO2, "channel_check_finished param %hd",
            (FMT__H, param));

  if (zb_buf_get_status(param) == (zb_ret_t)ZB_NWK_STATUS_SUCCESS)
  {
    nwk_txstat_clear();
  }

  zb_zdo_clear_channel_check_flag(0);

  zb_buf_free(param);
}

#endif /* !ZB_ED_ROLE || ZB_MGMT_NWK_ENHANCED_UPDATE_ENABLED */

void zb_zdo_ed_scan_request(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO1, ">> zb_zdo_ed_scan_request param %hd busy %hd",
            (FMT__H_H, param, ZG->zdo.zdo_ctx.nwk_upd_req.ed_scan_busy));

  if (ZB_U2B(ZB_ZDO_GET_ED_SCAN_BUSY()))
  {
    TRACE_MSG(TRACE_ZDO3, "ZDO ED scan busy, re-schedule", (FMT__0));

    /* Re-schedule myself if busy */
    ZB_SCHEDULE_CALLBACK(zb_zdo_ed_scan_request, param);
  }
  else
  {

    zb_zdo_ed_scan_param_t *zdo_req_ptr;
    zb_nlme_ed_scan_request_t *nwk_req_ptr;
    zb_nlme_ed_scan_request_t nwk_req;

    /* Lock the queue */
    ZB_ZDO_SET_ED_SCAN_BUSY();

    /* Copy request parameters to temporary variable... */
    zdo_req_ptr = ZB_BUF_GET_PARAM(param, zb_zdo_ed_scan_param_t);
    ZB_MEMCPY(&nwk_req, &zdo_req_ptr->nwk_param, sizeof(zb_nlme_ed_scan_request_t));

    ZG->zdo.zdo_ctx.nwk_upd_req.cb = zdo_req_ptr->cb;

    /* ... and restore them */
    nwk_req_ptr = ZB_BUF_GET_PARAM(param, zb_nlme_ed_scan_request_t);
    ZB_MEMCPY(nwk_req_ptr, &nwk_req, sizeof(zb_nlme_ed_scan_request_t));

    ZB_SCHEDULE_CALLBACK(zb_nlme_ed_scan_request, param);
  }

  TRACE_MSG(TRACE_ZDO1, "<< zb_zdo_ed_scan_request", (FMT__0));
}

#ifndef ZB_ED_ROLE
static void zdo_perform_ed_scan(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO1, ">> zdo_perform_energy_scan param %hd", (FMT__H, param));

  /* Check if it's only one available channel */
  if (MAGIC_IS_POWER_OF_TWO(zb_aib_channel_page_list_get_mask(ZB_CHANNEL_PAGE_TO_IDX(ZB_PIBCACHE_CURRENT_PAGE()))))
  {
    /* Well, do nothing here */
    TRACE_MSG(TRACE_ZDO2, "Single channel in APS channel mask: 0x%lx, skip ed scan",
              (FMT__L, zb_aib_channel_page_list_get_mask(
                ZB_CHANNEL_PAGE_TO_IDX(ZB_PIBCACHE_CURRENT_PAGE()))));

    ZB_SCHEDULE_CALLBACK(zb_zdo_check_fails_finish_cb, param);
  }
  else
  {
    /* FIXME: Get all pages for current PHY (when implement Implement Enhanced NWK Update Req):
       Annex E:
       Prerequisites: Devices shall limit their operations to channels within their current PHY
       (i.e. 868/915 MHz or 2450 MHz). Commands including channels outside the band shall be
       ignored.
    */
    zb_zdo_ed_scan_param_t *req;

    req = ZB_BUF_GET_PARAM(param, zb_zdo_ed_scan_param_t);

    TRACE_MSG(TRACE_ZDO2, "Perform energy detection scan", (FMT__0));
    req->cb = zdo_ed_scan_completed_channel_manager;
    req->nwk_param.scan_duration = ZB_DEFAULT_SCAN_DURATION;

    zb_channel_list_init(req->nwk_param.scan_channels_list);
    zb_channel_page_list_set_mask(req->nwk_param.scan_channels_list,
                                  ZB_CHANNEL_PAGE_TO_IDX(ZB_PIBCACHE_CURRENT_PAGE()),
                                  zb_aib_channel_page_list_get_mask(ZB_CHANNEL_PAGE_TO_IDX(ZB_PIBCACHE_CURRENT_PAGE())));

    ZB_SCHEDULE_CALLBACK(zb_zdo_ed_scan_request, param);
  }

  TRACE_MSG(TRACE_ZDO1, "<< zdo_perform_energy_scan", (FMT__0));
}


static void zdo_ed_scan_completed_channel_manager(zb_uint8_t param)
{
  zb_bool_t free_buf = ZB_TRUE;

  TRACE_MSG(TRACE_ZDO1, ">> zdo_ed_scan_completed_channel_manager param %hd",
            (FMT__H, param));

  TRACE_MSG(TRACE_ZDO3, "status %hd", (FMT__H, zb_buf_get_status(param)));

  if (zb_buf_get_status(param) == (zb_ret_t)ZB_NWK_STATUS_SUCCESS &&
      !ZB_U2B(ZB_ZDO_GET_CHANNEL_CHANGED()))
  {
    zb_uindex_t i;
    zb_uint8_t current_energy = 0;
    zb_uint8_t min_energy = 0xFF;
    zb_uint8_t new_channel = 0;

    zb_energy_detect_list_t *ed_list = ZB_BUF_GET_PARAM(param, zb_energy_detect_list_t);

    TRACE_MSG(TRACE_ZDO3, "ED list: count %hd", (FMT__H, ed_list->channel_count));

    /* Find current channel ed value */
    for (i = 0; i < ed_list->channel_count; i++)
    {
      /* Search for channels only in current page */
      if (ed_list->channel_info[i].channel_page_idx ==
          ZB_CHANNEL_PAGE_TO_IDX(ZB_PIBCACHE_CURRENT_PAGE()))
      {
        if (ed_list->channel_info[i].channel_number == ZB_PIBCACHE_CURRENT_CHANNEL())
        {
          current_energy = ed_list->channel_info[i].energy_detected;
        }

        if (ed_list->channel_info[i].energy_detected < min_energy)
        {
          min_energy = ed_list->channel_info[i].energy_detected;
          new_channel = ed_list->channel_info[i].channel_number;
        }
      }

      TRACE_MSG(TRACE_ZDO3, "page idx: %hd, channel: %hd, energy %hd",
                (FMT__H_H_H, ed_list->channel_info[i].channel_page_idx,
                 ed_list->channel_info[i].channel_number,
                 ed_list->channel_info[i].energy_detected));
    }

    TRACE_MSG(TRACE_ZDO3,
              "Current channel %hd, current energy %hd, candidate channel %hd, its energy %hd",
              (FMT__H_H_H_H, ZB_PIBCACHE_CURRENT_CHANNEL(), current_energy,
               new_channel, min_energy));

    if (current_energy > ZB_CHANNEL_BUSY_ED_VALUE)
    {
      if (min_energy < ZB_CHANNEL_FREE_ED_VALUE)
      {
        /* TODO: Why should we do it multiple times? */
        ZB_NIB_UPDATE_ID()++;
        zb_buf_flags_or(param, ZB_BUF_ZDO_CMD_NO_RESP);

        if (ZB_LOGICAL_PAGE_IS_2_4GHZ(ZB_PIBCACHE_CURRENT_PAGE()))
        {
          /* NWK update request for 2.4GHz */
          zb_zdo_mgmt_nwk_update_req_t *req;

          req = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_nwk_update_req_t);

          req->hdr.scan_channels = (1UL << new_channel);
          req->hdr.scan_duration = ZB_ZDO_NEW_ACTIVE_CHANNEL;
          req->dst_addr = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;

          if (zb_zdo_mgmt_nwk_update_req(param, NULL) != ZB_ZDO_INVALID_TSN)
          {
            free_buf = ZB_FALSE;
          }
        }
#ifdef ZB_MGMT_NWK_ENHANCED_UPDATE_ENABLED
        else
        {
          zb_ret_t ret;
          /* NWK Enhanced Update request for Sub-GHz */
          zb_zdo_mgmt_nwk_enhanced_update_req_param_t *enh_req;

          enh_req = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_nwk_enhanced_update_req_param_t);

          zb_channel_list_init(enh_req->channel_list);
          ret = zb_channel_page_list_set_logical_channel(enh_req->channel_list,
                                                         ZB_PIBCACHE_CURRENT_PAGE(),
                                                         new_channel);
          ZB_ASSERT(ret == RET_OK);

          enh_req->scan_duration = ZB_ZDO_NEW_ACTIVE_CHANNEL;
          enh_req->dst_addr = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;

          if (zb_zdo_mgmt_nwk_enh_update_req(param, NULL) != ZB_ZDO_INVALID_TSN)
          {
            free_buf = ZB_FALSE;
          }
        }
#endif /* ZB_MGMT_NWK_ENHANCED_UPDATE_ENABLED */

        TRACE_MSG(TRACE_ZDO2, "set new channel %hd after %d",
                  (FMT__H_D, new_channel,
                   ZB_NWK_OCTETS_TO_BI(ZB_NWK_MAX_BROADCAST_RETRIES * ZB_NWK_BROADCAST_DELIVERY_TIME_OCTETS)));

        ZB_SCHEDULE_ALARM(zb_zdo_do_set_channel, new_channel, ZB_NWK_OCTETS_TO_BI(
			    ZB_NWK_JITTER(ZB_NWK_MAX_BROADCAST_RETRIES * ZB_NWK_BROADCAST_DELIVERY_TIME_OCTETS)
			 ));

        ZB_ZDO_SET_CHANNEL_CHANGED();
        zb_zdo_start_long_timer(zb_zdo_channel_change_timer_cb, 0, ZB_ZDO_APS_CHANEL_TIMER);
      }
    }
  }

  zb_zdo_clear_channel_check_flag(0);

  if (free_buf)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZDO1, "<< zdo_ed_scan_completed_channel_manager", (FMT__0));
}

#endif /* ZB_ED_ROLE */

void zb_zdo_channel_check_timer_cb(zb_uint8_t param)
{
  ZVUNUSED(param);
  TRACE_MSG(TRACE_MAC2, "channel_check_timer_cb", (FMT__0));
  ZB_ZDO_CLEAR_CHECK_FAILS(); /* clear flag to allow next channel interference resolution */
}

void zb_nlme_ed_scan_confirm(zb_uint8_t param)
{

  TRACE_MSG(TRACE_NWK1, ">> zb_nlme_ed_scan_confirm %hd", (FMT__H, param));

  if (ZB_U2B(ZB_ZDO_GET_ED_SCAN_BUSY()))
  {
    ZB_ZDO_CLEAR_ED_SCAN_BUSY();
    ZB_SCHEDULE_CALLBACK(ZG->zdo.zdo_ctx.nwk_upd_req.cb, param);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "ZDO ED scan is not busy, ignore", (FMT__0));
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_NWK1, "<< zb_nlme_ed_scan_confirm", (FMT__0));
}

/*
 * "zb_zdo_reset_nwk_update_notify_limit()" will be used for a future Enhanced
 * NWK Update Notify command implementation.
 */
#if 0
void zb_zdo_reset_nwk_update_notify_limit(zb_uint8_t param)
{
  ZG->zdo.nwk_upd_notify_pkt_limit = param;
}
#endif


void zb_zdo_channel_check_finish_cb(zb_uint8_t param)
{
  ZVUNUSED(param);
}

void zb_zdo_start_long_timer(zb_callback_t func, zb_uint8_t param, zb_uint8_t long_timer)
{
  ZB_ASSERT(ZG->zdo.long_timer_cb == NULL); /* long timer should be free */
  ZG->zdo.long_timer = long_timer;
  ZG->zdo.long_timer_cb = func;
  ZG->zdo.long_timer_param = param;
  ZB_SCHEDULE_ALARM(zb_zdo_long_timer_cb, 0, ZB_ZDO_1_MIN_TIMEOUT);
}


void zb_zdo_long_timer_cb(zb_uint8_t param)
{
  ZVUNUSED(param);
  ZG->zdo.long_timer--;
  TRACE_MSG(TRACE_MAC3, "long_timer_cb timer val %hd", (FMT__H, ZG->zdo.long_timer));
  if (ZG->zdo.long_timer != 0U)
  {
    ZB_SCHEDULE_ALARM(zb_zdo_long_timer_cb, 0, ZB_ZDO_1_MIN_TIMEOUT);
  }
  else
  {
    ZB_SCHEDULE_CALLBACK(ZG->zdo.long_timer_cb, ZG->zdo.long_timer_param);
    ZG->zdo.long_timer_cb = NULL;
  }
}

/* clear channel_changed flag */
void zb_zdo_channel_change_timer_cb(zb_uint8_t param)
{
  ZVUNUSED(param);
  TRACE_MSG(TRACE_MAC1, "zb_zdo_channel_change_timer_cb", (FMT__0));
  ZB_ZDO_CLEAR_CHANNEL_CHANGED();
}



/*! @} */
