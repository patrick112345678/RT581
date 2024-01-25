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
/*  PURPOSE: SE Steady State.
*/

#define ZB_TRACE_FILE_ID 37
#include "zb_common.h"

#ifdef ZB_ENABLE_SE

#if defined(ZB_SE_ENABLE_STEADY_STATE_PROCESSING)

#include "zb_se.h"

void zb_se_steady_state_tc_poll(zb_uint8_t param);
void zb_se_steady_state_match_desc_req(zb_uint8_t param);
void zb_se_steady_state_match_desc_req_delayed(zb_uint8_t param);

zb_bool_t zb_se_steady_state_match_desc_resp_handle(zb_uint8_t param)
{
  zb_zdo_match_desc_resp_t *resp = (zb_zdo_match_desc_resp_t*)zb_buf_begin(param);
  zb_uint8_t *match_ep;

  TRACE_MSG(TRACE_ZCL1, ">> zb_se_steady_state_match_desc_resp_handle param %hd", (FMT__H, param));

  if (resp->status == ZB_ZDP_STATUS_SUCCESS
      && resp->tsn == ZSE_CTXC().steady_state.tsn
      && resp->match_len > 0)
  {
    /* Match EP list follows right after response header */
    match_ep = (zb_uint8_t*)(resp + 1);
    ZSE_CTXC().steady_state.endpoint = *match_ep;

    ZB_SCHEDULE_ALARM_CANCEL(zb_se_steady_state_match_desc_req_delayed, ZB_ALARM_ANY_PARAM);
    ZSE_CTXC().steady_state.countdown = 0;
    ZB_SCHEDULE_CALLBACK(zb_se_steady_state_tc_poll, param);
    param = 0;

    /* TC poll method found, can start Service Discovery */
    se_commissioning_signal(SE_COMM_SIGNAL_STEADY_TC_POLL_STARTED, param);
  }

  TRACE_MSG(TRACE_ZCL1, "<< zb_se_steady_state_match_desc_resp_handle", (FMT__0));
  return (zb_bool_t)(!param);
}

void zb_se_steady_state_match_desc_req_delayed(zb_uint8_t param)
{
  ZVUNUSED(param);

#if defined(ZB_SE_ENABLE_STEADY_STATE_PROCESSING)
  ++ZSE_CTXC().steady_state.poll_method;
#endif

  zb_buf_get_out_delayed(zb_se_steady_state_match_desc_req);
}

void zb_se_steady_state_match_desc_req(zb_uint8_t param)
{
  zb_zdo_match_desc_param_t *req;
  zb_af_endpoint_desc_t *ep_desc = NULL;
  zb_uint16_t tc_short_addr = zb_address_short_by_ieee(ZB_AIB().trust_center_address);

  TRACE_MSG(TRACE_ZCL1, ">> zb_se_steady_state_match_desc_req %hd", (FMT__H, param));

  if (ZSE_CTXC().steady_state.poll_method != ZSE_TC_POLL_NOT_SUPPORTED)
  {
    req = zb_buf_initial_alloc(param, sizeof(zb_zdo_match_desc_param_t) + (1) * sizeof(zb_uint16_t));

    req->nwk_addr = tc_short_addr;
    req->addr_of_interest = tc_short_addr;
    req->num_in_clusters = 1;
    req->num_out_clusters = 0;

    /* TODO: If not support Keep-Alive, Metering or Price as client, skip these poll methods! */
    switch (ZSE_CTXC().steady_state.poll_method)
    {
      case ZSE_TC_POLL_READ_KEEPALIVE:
        ep_desc = get_endpoint_by_cluster_with_role(ZB_ZCL_CLUSTER_ID_KEEP_ALIVE,
                                                    ZB_ZCL_CLUSTER_CLIENT_ROLE);
        if (ep_desc)
        {
          req->cluster_list[0] = ZB_ZCL_CLUSTER_ID_KEEP_ALIVE;
          break;
        }
        ++ZSE_CTXC().steady_state.poll_method;

#if defined(ZB_ZCL_SUPPORT_CLUSTER_METERING)
      /* FALLTHROUGH */
      case ZSE_TC_POLL_READ_METERING_CONSUMPTION:
        ep_desc = get_endpoint_by_cluster_with_role(ZB_ZCL_CLUSTER_ID_METERING,
                                                    ZB_ZCL_CLUSTER_CLIENT_ROLE);
        if (ep_desc)
        {
          req->cluster_list[0] = ZB_ZCL_CLUSTER_ID_METERING;
          break;
        }
        ++ZSE_CTXC().steady_state.poll_method;

#endif

#if defined(ZB_ZCL_SUPPORT_CLUSTER_PRICE)
      /* FALLTHROUGH */
      case ZSE_TC_POLL_READ_PRICING_INFO:
        ep_desc = get_endpoint_by_cluster_with_role(ZB_ZCL_CLUSTER_ID_PRICE,
                                                    ZB_ZCL_CLUSTER_CLIENT_ROLE);
        if (ep_desc)
        {
          req->cluster_list[0] = ZB_ZCL_CLUSTER_ID_PRICE;
          break;
        }
        ++ZSE_CTXC().steady_state.poll_method;

#endif
      /* FALLTHROUGH */
      default:
        TRACE_MSG(TRACE_ERROR, "Not found keep-alive polling method. Add keep-alive client or other polling cluster(metering, price) to device.", (FMT__0));
        ZB_ASSERT(0);
        break;
    }

    req->profile_id = ep_desc->simple_desc->app_profile_id;

    TRACE_MSG(TRACE_ZCL1, "send match descr for cluster 0x%x", (FMT__D, req->cluster_list[0]));

    ZSE_CTXC().steady_state.tsn = zb_zdo_match_desc_req(param, NULL);

    if (ZSE_CTXC().steady_state.tsn != ZB_ZDO_INVALID_TSN)
    {
      ZB_SCHEDULE_ALARM(zb_se_steady_state_match_desc_req_delayed, 0, ZB_SE_STEADY_STATE_CLUSTER_MATCH_DESC_TIME);
    }
    else
    {
      TRACE_MSG(TRACE_ZCL1, "tc poll finished: can not send match desc", (FMT__0));
      se_commissioning_signal(SE_COMM_SIGNAL_STEADY_TC_POLL_FAILED, param);
    }
  }
  else
  {
    TRACE_MSG(TRACE_ZCL1, "tc poll finished: can not find supported method", (FMT__0));
    se_commissioning_signal(SE_COMM_SIGNAL_STEADY_TC_POLL_NOT_SUPPORTED, param);
  }

  TRACE_MSG(TRACE_ZCL1, "<< zb_se_steady_state_match_desc_req", (FMT__0));
}

void zb_se_steady_state_start_periodic_tc_poll()
{
  /* Get trust center IEEE */
  zb_uint16_t tc_short_addr = zb_address_short_by_ieee(ZB_AIB().trust_center_address);

  /* Check it was not started before */
  if (ZSE_CTXC().steady_state.endpoint == 0 &&
      ZSE_CTXC().steady_state.poll_method == ZSE_TC_POLL_NOT_SUPPORTED)
  {
    /* Send Match Desc - Keep-Alive cluster */
    if (!ZB_IEEE_ADDR_IS_ZERO(ZB_AIB().trust_center_address) &&
        tc_short_addr != ZB_UNKNOWN_SHORT_ADDR)
    {
      ZSE_CTXC().steady_state.poll_method = ZSE_TC_POLL_READ_KEEPALIVE;
      zb_buf_get_out_delayed(zb_se_steady_state_match_desc_req);
    }
  }
}

void zb_se_steady_state_tc_poll_countdown(zb_uint8_t param)
{
  ZVUNUSED(param);

  if (ZSE_CTXC().steady_state.countdown > 0)
  {
    ZB_SCHEDULE_ALARM(zb_se_steady_state_tc_poll_countdown, 0, ZB_TIME_ONE_SECOND * ZSE_CTXC().steady_state.countdown);
    ZSE_CTXC().steady_state.countdown = 0;
  }
  else
  {
    zb_buf_get_out_delayed(zb_se_steady_state_tc_poll);
  }
}

void zb_se_steady_state_stop_periodic_tc_poll()
{
  ZSE_CTXC().steady_state.countdown = 0;
  ZSE_CTXC().steady_state.endpoint = 0;
  ZSE_CTXC().steady_state.poll_method = ZSE_TC_POLL_NOT_SUPPORTED;
  ZB_SCHEDULE_ALARM_CANCEL(zb_se_steady_state_tc_poll_countdown, ZB_ALARM_ANY_PARAM);
}

void zb_se_steady_state_tc_poll_timeout(zb_uint8_t param)
{
  ZVUNUSED(param);

  TRACE_MSG(TRACE_ZCL1, ">> zb_se_steady_state_tc_poll_timeout %hd", (FMT__H, param));
#ifdef ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ
  if (!ZCL_CTX().subghz_ctx.cli.suspend_zcl_messages)
#endif
  {
  ++ZSE_CTXC().steady_state.failure_cnt;
  }
  if (ZSE_CTXC().steady_state.failure_cnt == ZB_SE_STEADY_STATE_MAX_FAILURE_CNT)
  {
    se_commissioning_signal(SE_COMM_SIGNAL_STEADY_TC_POLL_FAILED, 0);
  }
  else
  {
    /* Reschedule next poll */
    ZB_ASSERT(ZSE_CTXC().steady_state.poll_method != ZSE_TC_POLL_NOT_SUPPORTED);

    ZSE_CTXC().steady_state.countdown = ZSE_CTXC().steady_state.keepalive_base * 60 +
      ZB_RANDOM_VALUE(ZSE_CTXC().steady_state.keepalive_jitter);

    ZB_SCHEDULE_CALLBACK(zb_se_steady_state_tc_poll_countdown, 0);
  }
}

void zb_se_steady_state_tc_poll(zb_uint8_t param)
{
  zb_ret_t ret = RET_BUSY;
  zb_uint8_t *cmd_ptr = zb_buf_begin(param);
  zb_uint16_t tc_short_addr = zb_address_short_by_ieee(ZB_AIB().trust_center_address);
  zb_af_endpoint_desc_t *ep_desc = NULL;

  TRACE_MSG(TRACE_ZCL1, ">> zb_se_steady_state_tc_poll param %hd", (FMT__H, param));

#ifdef ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ
  if (ZCL_CTX().subghz_ctx.cli.suspend_zcl_messages)
  {
    /* Reschedule next poll */
    TRACE_MSG(TRACE_ZCL1, ">> suspended, reschedule countdown", (FMT__0));

    ZB_ASSERT(ZSE_CTXC().steady_state.poll_method != ZSE_TC_POLL_NOT_SUPPORTED);

    ZSE_CTXC().steady_state.countdown = ZSE_CTXC().steady_state.keepalive_base * 60 +
      ZB_RANDOM_VALUE(ZSE_CTXC().steady_state.keepalive_jitter);

    ZB_SCHEDULE_CALLBACK(zb_se_steady_state_tc_poll_countdown, 0);
    zb_buf_free(param);
    return;
  }

  ZCL_CTX().subghz_ctx.cli.suspend_zcl_messages = ZB_FALSE;
#endif
  ZB_ASSERT(ZSE_CTXC().steady_state.endpoint &&
            ZSE_CTXC().steady_state.poll_method < ZSE_TC_POLL_NOT_SUPPORTED &&
            tc_short_addr != ZB_UNKNOWN_SHORT_ADDR);

  ZB_ZCL_GENERAL_INIT_READ_ATTR_REQ(param, cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);

  switch (ZSE_CTXC().steady_state.poll_method)
  {
    case ZSE_TC_POLL_READ_KEEPALIVE:
    {
      ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, ZB_ZCL_ATTR_KEEP_ALIVE_TC_KEEP_ALIVE_BASE_ID);
      ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, ZB_ZCL_ATTR_KEEP_ALIVE_TC_KEEP_ALIVE_JITTER_ID);

      ep_desc = get_endpoint_by_cluster_with_role(ZB_ZCL_CLUSTER_ID_KEEP_ALIVE,
                                                  ZB_ZCL_CLUSTER_CLIENT_ROLE);
      ZB_ASSERT(ep_desc);

      ret = zb_zcl_finish_and_send_packet(param,
                                              cmd_ptr,
                                              (zb_addr_u *)(&tc_short_addr),
                                              ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                              ZSE_CTXC().steady_state.endpoint,
                                              ep_desc->ep_id,
                                              get_profile_id_by_endpoint(ep_desc->ep_id),
                                              ZB_ZCL_CLUSTER_ID_KEEP_ALIVE,
                                              NULL);
    }
    break;

    case ZSE_TC_POLL_READ_METERING_CONSUMPTION:
    {
      ZSE_CTXC().steady_state.keepalive_base = 60;
      ZSE_CTXC().steady_state.keepalive_jitter = 0;

      ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, ZB_ZCL_ATTR_METERING_CURRENT_SUMMATION_DELIVERED_ID);

      ep_desc = get_endpoint_by_cluster_with_role(ZB_ZCL_CLUSTER_ID_METERING,
                                                  ZB_ZCL_CLUSTER_CLIENT_ROLE);
      ZB_ASSERT(ep_desc);

      ret = zb_zcl_finish_and_send_packet(param,
                                              cmd_ptr,
                                              (zb_addr_u *)(&tc_short_addr),
                                              ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                              ZSE_CTXC().steady_state.endpoint,
                                              ep_desc->ep_id,
                                              get_profile_id_by_endpoint(ep_desc->ep_id),
                                              ZB_ZCL_CLUSTER_ID_METERING,
                                              NULL);
    }
    break;

    case ZSE_TC_POLL_READ_PRICING_INFO:
    {
      ZSE_CTXC().steady_state.keepalive_base = 60;
      ZSE_CTXC().steady_state.keepalive_jitter = 0;

      ep_desc = get_endpoint_by_cluster_with_role(ZB_ZCL_CLUSTER_ID_PRICE,
                                                  ZB_ZCL_CLUSTER_CLIENT_ROLE);
      ZB_ASSERT(ep_desc);

#if defined ZB_ZCL_SUPPORT_CLUSTER_PRICE
      zb_zcl_price_send_cmd_get_current_price(param,
                                              (zb_addr_u *) &tc_short_addr,
                                              ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                              ZSE_CTXC().steady_state.endpoint,
                                              ep_desc->ep_id,
                                              zb_get_rx_on_when_idle(),
                                              NULL);
#endif
      ret = RET_OK;
    }
    break;

    default:
      ZB_ASSERT(0);
      break;
  }

  if (ret != RET_OK)
  {
    ++ZSE_CTXC().steady_state.failure_cnt;
  }
  else
  {
    ZSE_CTXC().steady_state.tsn = ZCL_CTX().seq_number - 1;
    ZB_SCHEDULE_ALARM(zb_se_steady_state_tc_poll_timeout, 0, ZB_TIME_ONE_SECOND * 60);
  }

  if (ZSE_CTXC().steady_state.failure_cnt == ZB_SE_STEADY_STATE_MAX_FAILURE_CNT)
  {
    se_commissioning_signal(SE_COMM_SIGNAL_STEADY_TC_POLL_FAILED, param);
  }
  else if (ret != RET_OK)
  {
    /* Reschedule next read attempt. */
    ZB_SCHEDULE_CALLBACK(zb_se_steady_state_tc_poll, param);
  }
}

zb_bool_t zb_se_steady_state_read_attr_handle(zb_uint8_t param)
{
  zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);
  zb_zcl_read_attr_res_t *resp = NULL;
  zb_uint8_t read_attr_resp_cnt = 0;
  zb_uint16_t cluster_id = cmd_info->cluster_id;

  TRACE_MSG(TRACE_ZCL1, ">> zb_se_steady_state_read_attr_handle param %hd", (FMT__H, param));

  ZB_SCHEDULE_ALARM_CANCEL(zb_se_steady_state_tc_poll_timeout, ZB_ALARM_ANY_PARAM);

  do
  {
    ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(param, resp);

    if (resp && resp->status == ZB_ZCL_STATUS_SUCCESS)
    {
      if (cluster_id == ZB_ZCL_CLUSTER_ID_KEEP_ALIVE)
      {
        if (resp->attr_id == ZB_ZCL_ATTR_KEEP_ALIVE_TC_KEEP_ALIVE_BASE_ID)
        {
          ZSE_CTXC().steady_state.keepalive_base = resp->attr_value[0];
          TRACE_MSG(TRACE_ZCL1, "keep-alive: base %hd", (FMT__H, ZSE_CTXC().steady_state.keepalive_base));
        }
        else if (resp->attr_id == ZB_ZCL_ATTR_KEEP_ALIVE_TC_KEEP_ALIVE_JITTER_ID)
        {
          ZB_HTOLE16(&ZSE_CTXC().steady_state.keepalive_jitter, resp->attr_value);
          TRACE_MSG(TRACE_ZCL1, "keep-alive: jitter %hd", (FMT__H, ZSE_CTXC().steady_state.keepalive_jitter));
        }
        ++read_attr_resp_cnt;
      }
#if defined(ZB_ZCL_SUPPORT_CLUSTER_METERING) || defined(ZB_ZCL_SUPPORT_CLUSTER_PRICE)
      else if (cluster_id == ZB_ZCL_CLUSTER_ID_METERING ||
               cluster_id == ZB_ZCL_CLUSTER_ID_PRICE)
      {
        ++read_attr_resp_cnt;
      }
#endif
    }
  }
  while (resp);

  if (!read_attr_resp_cnt ||
      (ZSE_CTXC().steady_state.poll_method == ZSE_TC_POLL_READ_KEEPALIVE &&
       read_attr_resp_cnt != 2))
  {
    ++ZSE_CTXC().steady_state.failure_cnt;
  }
  else
  {
      ZSE_CTXC().steady_state.failure_cnt = 0;
  }

  if (ZSE_CTXC().steady_state.failure_cnt == ZB_SE_STEADY_STATE_MAX_FAILURE_CNT)
  {
    se_commissioning_signal(SE_COMM_SIGNAL_STEADY_TC_POLL_FAILED, param);
    param = 0;
  }
  else
  {
    /* Reschedule next poll */
    ZB_ASSERT(ZSE_CTXC().steady_state.poll_method != ZSE_TC_POLL_NOT_SUPPORTED);

    ZSE_CTXC().steady_state.countdown = ZSE_CTXC().steady_state.keepalive_base * 60 +
      ZB_RANDOM_VALUE(ZSE_CTXC().steady_state.keepalive_jitter);

    ZB_SCHEDULE_CALLBACK(zb_se_steady_state_tc_poll_countdown, 0);
  }

  TRACE_MSG(TRACE_ZCL1, "<< zb_se_steady_state_read_attr_handle", (FMT__0));

  return (zb_bool_t)(!param);
}

#if defined(ZB_ZCL_SUPPORT_CLUSTER_PRICE)

zb_ret_t zb_se_steady_publish_price_handle(zb_uint8_t param)
{

  TRACE_MSG(TRACE_ZCL1, ">> zb_se_steady_publish_price_handle param %hd", (FMT__H, param));

  /* TODO: Looks like we do not need to check anything here. Add checks of Publish Price fields if
   * needed. */

  /* TODO: Integrate with common work - may reset countdown on common price commands etc. */

  ZB_SCHEDULE_ALARM_CANCEL(zb_se_steady_state_tc_poll_timeout, ZB_ALARM_ANY_PARAM);

  /* Reschedule next poll */
  ZB_ASSERT(ZSE_CTXC().steady_state.poll_method != ZSE_TC_POLL_NOT_SUPPORTED);

  ZSE_CTXC().steady_state.countdown = ZSE_CTXC().steady_state.keepalive_base * 60 +
    ZB_RANDOM_VALUE(ZSE_CTXC().steady_state.keepalive_jitter);

  ZB_SCHEDULE_CALLBACK(zb_se_steady_state_tc_poll_countdown, 0);

  TRACE_MSG(TRACE_ZCL1, "<< zb_se_steady_publish_price_handle", (FMT__0));

  zb_buf_free(param);

  return RET_OK;
}

#endif

zb_bool_t zb_se_steady_state_block_zcl_cmd(zb_uint16_t short_addr, zb_uint8_t ep, zb_uint8_t zcl_tsn)
{
  zb_uint16_t tc_short_addr = zb_address_short_by_ieee(ZB_AIB().trust_center_address);

  TRACE_MSG(TRACE_ZCL1, "zb_se_steady_state_block_zcl_cmd: addr %d ep %hd tsn %hd", (FMT__D_H_H, short_addr, ep, zcl_tsn));
  return (zb_bool_t)(tc_short_addr != ZB_UNKNOWN_SHORT_ADDR &&
                     tc_short_addr == short_addr &&
                     ZSE_CTXC().steady_state.endpoint == ep &&
                     ZSE_CTXC().steady_state.tsn == zcl_tsn &&
                     ZSE_CTXC().steady_state.countdown == 0);
}

#endif

#endif /* ZB_ENABLE_SE */
