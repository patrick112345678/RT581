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
/* PURPOSE: ZLL Commissioning cluster - network features.
*/

#define ZB_TRACE_FILE_ID 2118
#include "zb_common.h"
#include "zb_zcl.h"
#include "zll/zb_zll_common.h"
#include "zll/zb_zll_nwk_features.h"
#include "zll/zll_commissioning_internals.h"
#include "zb_nwk.h"

zb_uint16_t zb_nwk_get_stoch_addr(void);

#if defined ZB_ENABLE_ZLL

void zll_dev_start_continue_fn(zb_uint8_t param);

/** @brief Number of elements in the array. */
#define NUMOF(array)  (sizeof(array) / sizeof(array[0]))

/** @brief ZLL primary channels array. */
static zb_uint8_t g_zll_primary_channels[] = ZB_ZLL_PRIMARY_CHANNELS;

#if defined ZB_ROUTER_ROLE || defined DOXYGEN
/**
 *  @internal @brief Continues start router sequence handling.
 *  Schedules NLME-ROUTER-START.request after adjusting MAC IB.
 *  @param param [IN] - reference to the @ref zb_buf_t "buffer" containing MAC IB correction
 *  result. It would be also used to put NLME-START-ROUTER.request params into.
 */
void zll_start_router_continue(zb_uint8_t param);
#endif /* defined ZB_ROUTER_ROLE */


static void zll_refresh_ext_addr_cnf(zb_uint8_t param)
{
  zb_mlme_get_confirm_t *gconf = (zb_mlme_get_confirm_t *)zb_buf_begin(param);
  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), (gconf+1));
}

static void zll_refresh_ext_addr(zb_uint8_t param)
{
  zb_nwk_pib_get(param, ZB_PIB_ATTRIBUTE_EXTEND_ADDRESS, zll_refresh_ext_addr_cnf);
}


void zll_leave_nwk_confirm(zb_uint8_t param)
{
  zb_bufid_t  buffer = param;
  zb_ret_t ret;

  TRACE_MSG(TRACE_ZLL1, "> zll_leave_nwk_confirm param %hd", (FMT__H, param));

  if (ZB_IEEE_ADDR_IS_ZERO(ZB_PIBCACHE_EXTENDED_ADDRESS()))
  {
    zb_buf_get_out_delayed(zll_refresh_ext_addr);
  }

  if (! ZLL_TRAN_CTX().transaction_id)
  {
    TRACE_MSG(TRACE_ERROR, "ERROR no active transaction, dropping.", (FMT__0));
    zb_buf_free(param);
  }
  else
  {
    ret = zb_buf_get_status(param);
    if (ret != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "ERROR could not leave network.", (FMT__0));
    }
#if defined ZB_ROUTER_ROLE
/* 04/03/2018 EE CR:MINOR Next two branches looks like nearly a copy-paste. Can't it be refactored? */
    else if (ZLL_TRAN_CTX().transaction_task == ZB_ZLL_TRANSACTION_JOIN_ROUTER_TASK_TGT)
    {
      zb_zll_start_router_param_t *start_param = ZB_BUF_GET_PARAM(param, zb_zll_start_router_param_t);

      TRACE_MSG(TRACE_ZLL1, "join router task, call start router", (FMT__0));
      ZB_MEMCPY(start_param, &ZLL_TRAN_CTX().command_data.start_router_param, sizeof(zb_zll_start_router_param_t));

      ZB_SET_JOINED_STATUS(ZB_TRUE);
      ZG->aps.authenticated = ZB_TRUE;
      ret = zb_schedule_callback(zll_start_router, param);
    }
    else if (ZLL_TRAN_CTX().transaction_task == ZB_ZLL_TRANSACTION_NWK_START_TASK_TGT)
    {
      TRACE_MSG(TRACE_ZLL1, "nwk start task, call start router", (FMT__0));
      ZB_SET_JOINED_STATUS(ZB_TRUE);
      ZG->aps.authenticated = ZB_TRUE;
      ZB_ZLL_START_ROUTER(buffer, ZLL_TRAN_CTX().command_data.start_new_nwk.ext_pan_id,
              ZLL_TRAN_CTX().command_data.start_new_nwk.pan_id,
              ZLL_TRAN_CTX().command_data.start_new_nwk.channel,
              ZLL_TRAN_CTX().command_data.start_new_nwk.short_addr,
              ret);

    }
#endif /* defined ZB_ROUTER_ROLE */
#if defined ZB_ED_ROLE
    else if (ZLL_TRAN_CTX().transaction_task == ZB_ZLL_TRANSACTION_JOIN_ED_TASK_TGT)
    {
      TRACE_MSG(TRACE_ZLL1, "join ed task, call rejoin nwk ed", (FMT__0));

      zll_save_nwk_prefs(ZLL_TRAN_CTX().command_data.rejoin_nwk_param.ext_pan_id,
                     ZLL_TRAN_CTX().command_data.rejoin_nwk_param.short_pan_id,
                     ZB_PIBCACHE_NETWORK_ADDRESS(),
                     ZLL_TRAN_CTX().command_data.rejoin_nwk_param.channel);

      ZLL_REJOIN_NWK(buffer,
              ZLL_TRAN_CTX().command_data.rejoin_nwk_param.ext_pan_id,
              ZLL_TRAN_CTX().command_data.rejoin_nwk_param.channel,  ret);
    }
#endif /* defined ZB_ED_ROLE */
    else
    {
      TRACE_MSG(
          TRACE_ERROR,
          "ERROR not appropriate ZLL transaction task %hd, dropping buffer",
          (FMT__H, ZLL_TRAN_CTX().transaction_task));
      zb_buf_free(param);
      ret = RET_ERROR;
    }

    if (ret != RET_OK)
    {
      TRACE_MSG(TRACE_ZLL1, "notify user that transaction is failed ret %hd", (FMT__H, ret));
      zll_notify_task_result(buffer, ZB_ZLL_TASK_STATUS_FAILED);
    }
  }

  TRACE_MSG(TRACE_ZLL1, "< zll_leave_nwk_confirm", (FMT__0));
}/* void zll_leave_nwk_confirm(zb_uint8_t param) */

#if defined ZB_ROUTER_ROLE

void zll_change_me_addr(zb_uint8_t param)
{
  zb_address_ieee_ref_t addr_ref;
  zb_zll_start_router_param_t *start_param = ZB_BUF_GET_PARAM(param, zb_zll_start_router_param_t);

  ZB_PIBCACHE_NETWORK_ADDRESS() = (start_param->short_addr != 0) ? start_param->short_addr : zb_nwk_get_stoch_addr();

  /* lock my own address */
  zb_address_update(ZB_PIBCACHE_EXTENDED_ADDRESS(),
                    ZB_PIBCACHE_NETWORK_ADDRESS(),
                    ZB_TRUE, &addr_ref);
  zb_nwk_pib_set(param, ZB_PIB_ATTRIBUTE_SHORT_ADDRESS, &ZB_PIBCACHE_NETWORK_ADDRESS(), 2, zll_start_router_continue);
}


void zll_set_panid(zb_uint8_t param)
{
  zb_zll_start_router_param_t *start_param = ZB_BUF_GET_PARAM(param, zb_zll_start_router_param_t);

  TRACE_MSG(TRACE_ZLL1, "> zll_set_panid param %hd", (FMT__H, param));

  ZB_PIBCACHE_PAN_ID() = start_param->short_pan_id;
  {
    zb_address_pan_id_ref_t ref;
    zb_address_set_pan_id(ZB_PIBCACHE_PAN_ID(), ZB_NIB_EXT_PAN_ID(), &ref);
  }
  zb_nwk_pib_set(param, ZB_PIB_ATTRIBUTE_PANID, &ZB_PIBCACHE_PAN_ID(), 2, zll_change_me_addr);

  TRACE_MSG(TRACE_ZLL1, "< zll_set_panid", (FMT__0));
}

void zll_start_router(zb_uint8_t param)
{
  zb_zll_start_router_param_t *start_param = ZB_BUF_GET_PARAM(param, zb_zll_start_router_param_t);

  TRACE_MSG(TRACE_ZLL1, "> zll_start_router param %hd", (FMT__H, param));


  ZB_EXTPANID_COPY(ZB_NIB_EXT_PAN_ID(), start_param->ext_pan_id);

  TRACE_MSG(TRACE_ZLL1, "Ext pan Id " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(start_param->ext_pan_id)));

  /* TODO Provide normal callback for the next request */
  ZLL_SET_CHANNEL(param, start_param->channel, zll_set_panid);

//TODO: notify user on error

  TRACE_MSG(TRACE_ZLL1, "< zll_start_router", (FMT__0));
}/* void zll_start_router(zb_uint8_t param) */

void zll_start_router_continue(zb_uint8_t param)
{
  zb_nlme_start_router_request_t* request =
      ZB_BUF_GET_PARAM(param, zb_nlme_start_router_request_t);

  TRACE_MSG(TRACE_ZLL1, "> zll_start_router_continue param %hd", (FMT__H, param));

  zb_buf_reuse((zb_bufid_t )param);

  request->beacon_order = ZB_TURN_OFF_ORDER;
  request->superframe_order = ZB_TURN_OFF_ORDER;
  request->battery_life_extension = 0;
  ZB_SCHEDULE_CALLBACK(zb_nlme_start_router_request, param);

  TRACE_MSG(TRACE_ZLL1, "< zll_start_router_continue", (FMT__0));
}/* void zll_start_router_continue(zb_uint8_t param) */

#if defined ZB_ZLL_ENABLE_COMMISSIONING_SERVER

void zll_direct_join_confirm(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZLL3, "status %hd", (FMT__H, zb_buf_get_status(param)));

  if (    ZLL_TRAN_CTX().transaction_id
      &&  ZLL_TRAN_CTX().transaction_task == ZB_ZLL_TRANSACTION_NWK_START_TASK_TGT)
  {
    zll_notify_task_result(
      param,
      (   (zb_buf_get_status(param) == ZB_ZLL_GENERAL_STATUS_SUCCESS)
          ?  ZB_ZLL_TASK_STATUS_OK
          : ZB_ZLL_TASK_STATUS_FAILED));
  }
  else
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZLL1, "< zll_direct_join_confirm status %hd", (FMT__H, param));
}/* void zll_direct_join_confirm(zb_uint8_t param) */

#endif /* defined ZB_ZLL_ENABLE_COMMISSIONING_SERVER */

#endif /* defined ZB_ROUTER_ROLE */

zb_uint8_t zll_random_primary_channel(void)
{
  zb_uint8_t iter_number;
  zb_uint8_t value = 0;
  zb_uint8_t match = 0;

  TRACE_MSG(TRACE_ZLL1, "> zll_random_primary_channell aps_channel_mask 0x%x", (FMT__L, zb_aib_channel_page_list_get_2_4GHz_mask()/* MMDEVSTUBS */));

  /* 04/03/2018 EE CR:MINOR Why so complex? I would suppose generate
   * first index randomly, then go thry channel mask iterating modulo
   * # of primary channels. */
  for (iter_number = 0; iter_number < ZLL_MAX_RANDOM_CHANNEL_ITERATIONS; ++iter_number)
  {
    value = (zb_uint8_t)ZB_RANDOM_VALUE(NUMOF(g_zll_primary_channels));
    TRACE_MSG(TRACE_ZLL1, "random channel %hd", (FMT__H, g_zll_primary_channels[value]));
    if (((1l << g_zll_primary_channels[value]) & zb_aib_channel_page_list_get_2_4GHz_mask()/* MMDEVSTUBS */) != 0)
    {
      match = 1;
      break;
    }
  }

  if (!match)
  {
    /* Try to use 1st channel from intersection */
    for (iter_number = 0; iter_number < NUMOF(g_zll_primary_channels); ++iter_number)
    {
      if (((1l << g_zll_primary_channels[iter_number]) & zb_aib_channel_page_list_get_2_4GHz_mask()) != 0)
      {
        value = iter_number;
        break;
      }
    }
  }

  TRACE_MSG(
      TRACE_ZLL1,
      "< zll_random_primary_channell #%hd %hd",
      (FMT__H_H, value, g_zll_primary_channels[value]));

  return g_zll_primary_channels[value];
}/* zb_uint8_t zll_random_primary_channell() */


void zb_zdo_startup_complete_zll(zb_uint8_t param)
{
  zb_uint8_t status = zb_buf_get_status(param);
  zb_bool_t is_call_startup_complete=ZB_FALSE;

  TRACE_MSG(TRACE_ZCL1, ">> ZLL zb_zdo_startup_complete_zll %h", (FMT__H, param));

  TRACE_MSG(TRACE_ZCL2, "zll task id: %hd, status %hd",
            (FMT__H_H, ZLL_GET_TRANSACTION_TASK_ID(), status));

  switch(ZLL_GET_TRANSACTION_TASK_ID())
  {
    case ZB_ZLL_DEVICE_DISCOVERY_TASK:
    case ZB_ZLL_IDENTIFY_TASK:
      break;
    case ZB_ZLL_DEVICE_START_TASK:
      {
        is_call_startup_complete=ZB_TRUE;
      }
      break;

    case ZB_ZLL_TRANSACTION_NWK_START_TASK:
      {
        TRACE_MSG(TRACE_ZLL2, "Start new network finished, inform user app", (FMT__0));
        is_call_startup_complete=ZB_TRUE;
      }
      break;

    case ZB_ZLL_TRANSACTION_NWK_START_TASK_TGT:
#if defined ZB_ZLL_ENABLE_COMMISSIONING_SERVER && defined ZB_ROUTER_ROLE
      if (status == ZB_ZLL_TASK_STATUS_FAILED)
      {
        ZLL_DIRECT_JOIN(param, ZLL_TRAN_CTX().src_addr, status);
        if (status != RET_OK)
        {
          status = ZB_ZLL_TASK_STATUS_SCHEDULE_FAILED;
          is_call_startup_complete=ZB_TRUE;
        }
      }
      else
#endif /* defined ZB_ZLL_ENABLE_COMMISSIONING_SERVER */
      {
        TRACE_MSG(TRACE_ZLL2, "Start new network finished, inform user app", (FMT__0));
        is_call_startup_complete=ZB_TRUE;
      }
      break;

    case ZB_ZLL_TRANSACTION_JOIN_ROUTER_TASK:
      TRACE_MSG(TRACE_ZLL2, "join router, inform user app", (FMT__0));
      is_call_startup_complete=ZB_TRUE;
      break;

    case ZB_ZLL_TRANSACTION_JOIN_ROUTER_TASK_TGT:
      TRACE_MSG(TRACE_ZLL2, "JOIN_ROUTER_TGT", (FMT__0));
      ZLL_SET_TRANSACTION_TASK_ID(ZB_ZLL_COMMISSIONED);
      status = ZB_ZLL_TASK_STATUS_FAILED;
      is_call_startup_complete=ZB_TRUE;
      break;

    case ZB_ZLL_TRANSACTION_JOIN_ED_TASK:
      TRACE_MSG(TRACE_ZLL2, "join ED, inform user app", (FMT__0));
      is_call_startup_complete=ZB_TRUE;
      break;

    case ZB_ZLL_TRANSACTION_JOIN_ED_TASK_TGT:
      TRACE_MSG(TRACE_ZLL2, "JOIN_ED_TGT", (FMT__0));
      ZLL_SET_TRANSACTION_TASK_ID(ZB_ZLL_COMMISSIONED);
      status = ZB_ZLL_TASK_STATUS_FAILED;
      is_call_startup_complete=ZB_TRUE;
      break;

      /* TODO Add other cases. */

    default:
      is_call_startup_complete=ZB_TRUE;
      break;
  }

  // invoke standard zb_zdo_startup_complete
  if (is_call_startup_complete)
  {
    zll_notify_task_result(param, status);
  }

  TRACE_MSG(TRACE_ZCL1, "<< ZLL zb_zdo_startup_complete_zll", (FMT__0));
}


/**
 *  @internal @brief function for check channel number for Mgmt_Nwk_Update_Req command
 *  Try to find channel by number in primary channels list.
 *  @param channel [IN] - channel number
 */
zb_bool_t zll_check_channel_for_mgmt_nwk_update_change_channel(zb_uint8_t channel)
{
  zb_bool_t ret = ZB_FALSE;
  zb_uint8_t primary_chans[] = ZB_ZLL_PRIMARY_CHANNELS;
  zb_uindex_t i = 0;
  for (i = 0; i < sizeof(primary_chans)/sizeof(primary_chans[0]); ++i)
  {
    if (primary_chans[i] == channel)
    {
      ret = ZB_TRUE;
      break;
    }
  }
  return ret;
}

/**
 *  @internal @brief Start rejoin procedure after successful transmission Mgmt_Nwk_Update_Req
 *  (change network channel)
 *  This function will be called after ZB_NWK_BROADCAST_DELIVERY_TIME time from callback function,
 *  because otherwise we get collision (will be called zll_send_rejoin_req_change_channel_cb, and
 *  then zb_zdo_set_channel_cb, and in this case rejoin request will be sent from old channel)
 *  @param param [IN] - memory buffer data
 */
void zll_send_rejoin_req_after_change_channel(zb_uint8_t param)
{
  zb_bufid_t  buffer = param;
  zb_uint8_t status = 0;
  ZVUNUSED(status);
  TRACE_MSG(TRACE_ZLL1, ">> zll_send_rejoin_req_after_change_channel %hd", (FMT__H, param));
  TRACE_MSG(TRACE_ZLL3, "###rejoin on channel %hd", (FMT__H, ZB_PIBCACHE_CURRENT_CHANNEL()));
  ZLL_REJOIN_NWK(buffer, ZB_NIB_EXT_PAN_ID(), ZB_PIBCACHE_CURRENT_CHANNEL(), status);
  TRACE_MSG(TRACE_ZLL1, "<< zll_send_rejoin_req_after_change_channel", (FMT__0));
}

/**
 *  @internal @brief Send Mgmt_Nwk_Update_Req callback function
 *  Process ZDO Mgmt_Nwk_Update.confirm with given network update req confirm data.
 *  @param scan_channels [IN] - memory buffer data
 */
void zll_send_mgmt_nwk_update_change_channel_cb(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZLL1, ">> zll_send_mgmt_nwk_update_internal_cb", (FMT__0));
  if (zb_buf_get_status(param == RET_OK))
  {
    TRACE_MSG(TRACE_ZLL3, "zb_zdo_mgmt_nwk_update_req Ok", (FMT__0));
    //ember tests
    ZB_SCHEDULE_ALARM(zll_send_rejoin_req_after_change_channel,
                      param,
                      10*ZB_TIME_ONE_SECOND);
  }
  else
  {
    TRACE_MSG(TRACE_ZLL3, "zb_zdo_mgmt_nwk_update_req Failed", (FMT__0));
    /* AT: what we should do in this case (when broadcast transmission fails)?
    Should we notify application about this?
    */
    zb_buf_free(param);
  }
  TRACE_MSG(TRACE_ZLL1, "<< zll_send_mgmt_nwk_update_internal_cb", (FMT__0));
}

/** ZLL Network Update request handling.
 *  Updates channel and update_id.
 *  @param param [IN] - reference to the @ref zb_buf_t "buffer" containing zb_zll_commissioning_network_update_req_t.
 */
zb_ret_t zll_network_update_req_handler(zb_uint8_t param)
{
  zb_bufid_t  buffer = param;
  zb_zll_commissioning_network_update_req_t nwk_upd_req;
  zb_zcl_parsed_hdr_t cmd_info;
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_ZCL1, ">> zll_network_update_req_handler, param %hd", (FMT__H, param));

  ZB_ZCL_COPY_PARSED_HEADER(buffer, &cmd_info);
  ZB_ZLL_COMMISSIONING_GET_NETWORK_UPDATE_REQ(&nwk_upd_req, buffer, ret);

  if (ret == ZB_ZCL_PARSE_STATUS_FAILURE)
  {
    ret = RET_ILLEGAL_REQUEST;
    TRACE_MSG(TRACE_ERROR, "ERROR could not parse nwk update request", (FMT__0));
    zb_buf_free(param);
  }
  else if (!ZB_EXTPANID_CMP(nwk_upd_req.ext_pan_id, ZB_NIB_EXT_PAN_ID()) && ZB_PIBCACHE_PAN_ID() != nwk_upd_req.pan_id)
  {
    TRACE_MSG(TRACE_ERROR, "our ext pan ID " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(ZB_NIB_EXT_PAN_ID())));
    TRACE_MSG(TRACE_ERROR, "req ext pan ID " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(nwk_upd_req.ext_pan_id)));
    TRACE_MSG(TRACE_ERROR, "our pan ID %d", (FMT__D, ZB_PIBCACHE_PAN_ID()));
    TRACE_MSG(TRACE_ERROR, "req pan ID %d", (FMT__D, nwk_upd_req.pan_id));
    TRACE_MSG(TRACE_ERROR, "ERROR invalid pan ID", (FMT__0));
    zb_buf_free(param);
  }
  else
  {
    /* Compare nwk_update_id,
       see 8.6 in ZLL spec
     */
    if (nwk_upd_req.nwk_upd_id > ZB_NIB_UPDATE_ID() ||
        ZB_ABS(nwk_upd_req.nwk_upd_id - ZB_NIB_UPDATE_ID()) < 200)
    {
      TRACE_MSG(TRACE_ZCL1, "update: nwk_upd_id %i, channel %i", (FMT__H_H, nwk_upd_req.nwk_upd_id, nwk_upd_req.channel));
      ZLL_SET_CHANNEL(param, nwk_upd_req.channel, NULL);
      ZB_NIB_UPDATE_ID() = nwk_upd_req.nwk_upd_id;
    }
    else
    {
      TRACE_MSG(TRACE_ERROR, "Update not needed, drop packet", (FMT__0));
      zb_buf_free(param);
    }
  }

  TRACE_MSG(TRACE_ZCL1, "<< zll_network_update_req_handler", (FMT__0));
  return ret;
}

#endif /* defined ZB_ENABLE_ZLL */
