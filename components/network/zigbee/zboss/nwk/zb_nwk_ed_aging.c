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
/*  PURPOSE: End device aging
*/

#define ZB_TRACE_FILE_ID 4452

#include "zb_common.h"
#include "zb_nwk.h"
#include "zb_nwk_globals.h"
#include "zb_nwk_ed_aging.h"
#include "nwk_internal.h"
#include "zb_time.h"
#include "zb_zdo.h"
#include "zb_ncp.h"

static ZB_CONST ZB_CODE zb_uint32_t nwk_ed_aging_timeout_sec[] = {10, 120, 240, 480, 920, 1920, 3840, 7680, 15360, 30720, 61440, 122880, 245760, 491520, 983040};

void zb_set_keepalive_mode(nwk_keepalive_supported_method_t mode)
{
  ZB_NIB().nwk_keepalive_modes = mode;
}

#ifdef ZB_ED_FUNC

void zb_set_ed_timeout(zb_uint_t to)
{
  /* This is ED timeout after which parent will age us out */
  ZB_ASSERT(to <= ZB_UINT8_MAX);
  ZB_NIB().nwk_ed_timeout_default = (zb_uint8_t)to;
}

void zb_set_keepalive_timeout(zb_uint_t to)
{
  /* This is poll rate used for keepalive. Not sure why we set it
   * explicitly. Would be useful to remove that API and derive poll
   * rate from ED timeout and current poll rate (long/fast poll
   * etc).
   * Now it is user who must coordinate ed timeout and keepalive poll rate.
   */
  ZB_NIB().nwk_ed_keepalive_timeout = to;
}


zb_time_t zb_nwk_get_default_keepalive_timeout(void)
{
  /* ZB_TIME_ONE_SECOND is already in BI!
  return ZB_MILLISECONDS_TO_BEACON_INTERVAL((ZB_TIME_ONE_SECOND * nwk_ed_aging_timeout_sec[ZB_GET_ED_TIMEOUT()]) / 3U);
  */
  return ZB_MILLISECONDS_TO_BEACON_INTERVAL((1000U / 3U * nwk_ed_aging_timeout_sec[ZB_GET_ED_TIMEOUT()]));
}

#ifndef ZB_LITE_NO_ED_AGING_REQ
static void send_ed_timeout_req(zb_uint8_t param);
static void ed_timeout_resp_recv_fail(zb_uint8_t param);
static void process_ed_timeout_resp(zb_uint8_t param);
static void ed_timeout_resp_ok(zb_uint8_t param);
static void ed_timeout_resp_aged(zb_uint8_t param);


/* Entry point from ZDO and consequent alarms.

   That function called either when we just send device annce and
   wants to negotiate child keepalive mode with our parent, or sending
   next keepalive when in ED_TIMEOUT_REQUEST_KEEPALIVE mode.
 */
void zb_nwk_ed_send_timeout_req(zb_uint8_t param)
{
  TRACE_MSG(TRACE_NWK1, ">> zb_nwk_ed_send_timeout_req %hd", (FMT__H, param));

  if(param != 0U)
  {
    ZB_SCHEDULE_CALLBACK(send_ed_timeout_req, param);
  }
  else
  {
    zb_ret_t ret = zb_buf_get_out_delayed(send_ed_timeout_req);
    if (ret != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
    }
  }

  TRACE_MSG(TRACE_NWK1, "<< zb_nwk_ed_send_timeout_req", (FMT__0));
}


static void send_ed_timeout_req(zb_uint8_t param)
{
  zb_nwk_ed_timeout_request_t *time_req;
  zb_bool_t secure;
  zb_uint16_t dst_addr;

  TRACE_MSG(TRACE_NWK2, ">> send_ed_timeout_req %hd", (FMT__H, param));

  if (ZB_JOINED())
  {
    secure = ZG->aps.authenticated && (ZB_NIB_SECURITY_LEVEL() > 0U);

    zb_address_short_by_ref(&dst_addr, ZG->nwk.handle.parent);

    (void)nwk_alloc_and_fill_hdr(param, ZB_PIBCACHE_NETWORK_ADDRESS(), dst_addr, ZB_FALSE, secure,
                           ZB_TRUE, ZB_FALSE);

    time_req = (zb_nwk_ed_timeout_request_t *)nwk_alloc_and_fill_cmd(
        param, ZB_NWK_CMD_ED_TIMEOUT_REQUEST, (zb_uint8_t)sizeof(zb_nwk_ed_timeout_request_t));

    time_req->request_timeout = ZB_GET_ED_TIMEOUT();
    /* reserved value - 0 */
    time_req->ed_config = NWK_ED_DEVICE_CONFIG_DEFAULT;

    (void)zb_nwk_init_apsde_data_ind_params(param, ZB_NWK_INTERNAL_ED_TIMEOUT_REQ_FRAME_COFIRM_HANDLE);
    ZB_SCHEDULE_CALLBACK(zb_nwk_forward, param);
  }
  else
  {
    TRACE_MSG(TRACE_NWK1, "not joined to the current pan", (FMT__0));

    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_NWK2, "<< send_keepalive_timeout_req", (FMT__0));
}


void zb_nwk_ed_timeout_req_frame_confirm(zb_uint8_t param)
{
  zb_ret_t status = zb_buf_get_status(param);
  zb_nwk_ed_timeout_response_param_t *resp_param;

  TRACE_MSG(TRACE_NWK2, ">> ed_timeout_req_frame_confirm param %hd status %hd",
            (FMT__H_H, param, status));

  if (status == (zb_ret_t)MAC_SUCCESS)
  {
    ZB_SCHEDULE_ALARM(zb_nwk_ed_timeout_resp_recv_fail_trig, 0, ZB_NWK_ED_TIMEOUT_RESP_FAILURE_TMO);

    if (!ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()))
    {
      /* Do delayed poll after macResponseWaitTime */
      TRACE_MSG(TRACE_ZDO3, "Call zb_zdo_pim_start_turbo_poll_packets 1 after delay %u", (FMT__D, ZB_MAC_RESPONSE_WAIT_TIME));
      ZB_SCHEDULE_ALARM(zb_zdo_pim_start_turbo_poll_packets, 1, ZB_MAC_RESPONSE_WAIT_TIME);
#ifdef SNCP_MODE
      sncp_auto_turbo_poll_ed_timeout(ZB_TRUE);
#endif
    }

    zb_buf_free(param);
  }
  else
  {
    resp_param = ZB_BUF_GET_PARAM(param, zb_nwk_ed_timeout_response_param_t);
    ZB_BZERO(resp_param, sizeof(zb_nwk_ed_timeout_response_param_t));
    resp_param->status = ED_KEEPALIVE_REQ_FAILED;

    ZB_SCHEDULE_CALLBACK(process_ed_timeout_resp, param);
  }

  TRACE_MSG(TRACE_NWK2, "<< ed_timeout_req_frame_confirm", (FMT__0));
}


void zb_nwk_ed_timeout_resp_recv_fail_trig(zb_uint8_t unused)
{
  zb_ret_t ret;

  ZVUNUSED(unused);
  TRACE_MSG(TRACE_NWK1, "+keepalive_timeout_resp_fail_trig", (FMT__0));
  ret = zb_buf_get_out_delayed(ed_timeout_resp_recv_fail);
  if (ret != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
  }
}


static void ed_timeout_resp_recv_fail(zb_uint8_t param)
{
  zb_nwk_ed_timeout_response_param_t *resp_param;

  TRACE_MSG(TRACE_NWK1, ">> keealive_timeout_resp_failed param %hd", (FMT__H, param));

  resp_param = ZB_BUF_GET_PARAM(param, zb_nwk_ed_timeout_response_param_t);
  ZB_BZERO(resp_param, sizeof(zb_nwk_ed_timeout_response_param_t));
  resp_param->status = ED_KEEPALIVE_RESP_FAILED;

  ZB_SCHEDULE_CALLBACK(process_ed_timeout_resp, param);

  TRACE_MSG(TRACE_NWK1, "<< keealive_timeout_resp_failed", (FMT__0));
}


static void process_ed_timeout_resp(zb_uint8_t param)
{
  zb_nwk_ed_timeout_response_param_t *resp_param = ZB_BUF_GET_PARAM(param, zb_nwk_ed_timeout_response_param_t);

  TRACE_MSG(TRACE_NWK1, ">> process_keepalive_timeout_resp param %hd status %hd resp_status %hd parent_info %hd",
            (FMT__H_H_H_H, param, resp_param->status, resp_param->resp_status, resp_param->parent_info));

  /* Handle error case firstly */
  /* resp_param->status is our internal var: either ok or request timed out, or failed to be sent. */
  if ((resp_param->status == ED_KEEPALIVE_SUCCESS && /* Explicitly not supported case... */
       /* resp_status  */
       resp_param->resp_status != ED_AGING_SUCCESS) ||
      /* ... or Req hasn't been sent or Resp hasn't been received */
      (resp_param->status != ED_KEEPALIVE_SUCCESS && ZB_GET_PARENT_INFO() == 0U)
    )
  {
    TRACE_MSG(TRACE_ERROR, "ED aging is not supported", (FMT__0));
    zb_buf_free(param);
  }
  /* ... or everything is ok... */
  else if (resp_param->status == ED_KEEPALIVE_SUCCESS &&
           resp_param->resp_status == ED_AGING_SUCCESS)
  {
    /* Handle normal case - call as function*/
    ed_timeout_resp_ok(param);
  }
  /* ... or it is a fail */
  else if (resp_param->status != ED_KEEPALIVE_SUCCESS &&
           ZB_GET_PARENT_INFO() != 0U &&
           ZB_GET_KEEPALIVE_MODE() == ED_TIMEOUT_REQUEST_KEEPALIVE)
  {
    /* I am ED in timeout request keepalive mode, and failed to send packet or receive an answer. */
    /* Error case */
    if (resp_param->status == ED_KEEPALIVE_RESP_FAILED)
    {
        ed_timeout_resp_aged(param);
    }
    else if (resp_param->status == ED_KEEPALIVE_REQ_FAILED)
    {
        ZB_SCHEDULE_CALLBACK(send_ed_timeout_req, param);
    }
		
  }
  else if (resp_param->status != ED_KEEPALIVE_SUCCESS)
  {
    /* could not receive a resp, this is our initial negotiation - resend */
    TRACE_MSG(TRACE_NWK1, "Resending ED Timeout Req", (FMT__0));
    ZB_SCHEDULE_CALLBACK(send_ed_timeout_req, param);
  }
  else
  {
    /* This is debug code */
    TRACE_MSG(TRACE_ERROR, "UNKNOWN CASE", (FMT__0));
    ZB_ASSERT(0);
  }

#ifdef SNCP_MODE
  if (!ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()))
  {
    sncp_auto_turbo_poll_ed_timeout(ZB_FALSE);
  }
#endif

  TRACE_MSG(TRACE_NWK1, "<< process_keepalive_timeout_resp", (FMT__0));
}


static void ed_timeout_resp_ok(zb_uint8_t param)
{
  zb_nwk_ed_timeout_response_param_t *resp_param = ZB_BUF_GET_PARAM(param, zb_nwk_ed_timeout_response_param_t);

  TRACE_MSG(TRACE_NWK1, ">> ed_timeout_resp_ok param %hd", (FMT__H, param));

/*
  If the status is SUCCESS it shall set the nwkParentInformation value in the NIB
  to value of the Parent Information field of the received command. No further
  processing shall take place.
*/
  if (resp_param->resp_status == ED_AGING_SUCCESS)
  {
    ZB_SET_PARENT_INFO(resp_param->parent_info);
  }

/*All end devices (including RxOnWhenIdle=TRUE) that have received an End Device
 * Timeout Response Command with a status of SUCCESS may periodically send a
 * keepalive their router parent to insure they remain in the router's neighbor
 * table.
 */

/*The keepalive message will refresh the timeout on the parent device so that
 * the parent does not delete the child from its neighbor table. The period for
 * sending the keepalive to the router parent shall be determined by the
 * manufacturer of the device and is not specified by this standard. It is
 * recommended that the period allows the end device to send 3 keepalive
 * messages during the Device Timeout period. This will help insure that a
 * single missed keepalive message will not age out the end device on the router
 * parent.
 */
  switch (resp_param->parent_info & 0x03U)
  {
    case BOTH_KEEPALIVE_METHODS:
    case ED_TIMEOUT_REQUEST_KEEPALIVE:
      /* MISRA prohibits falling through in switch clause (Rule 16.3).
       * Use this strange comparison to workaround it. */
      if ((resp_param->parent_info & 0x03U) == BOTH_KEEPALIVE_METHODS)
      {
        /* Select the most secure in this case: ED_TIMEOUT_REQUEST_KEEPALIVE. */
        TRACE_MSG(TRACE_NWK1, "Parent supports both methods; select most secure (ED_TIMEOUT_REQUEST_KEEPALIVE)", (FMT__0));
      }
      ZB_SET_KEEPALIVE_MODE(ED_TIMEOUT_REQUEST_KEEPALIVE);
      /* Why translate into milliseconds???
      ZB_SCHEDULE_ALARM(zb_nwk_ed_send_timeout_req, 0,
                        ZB_TIME_BEACON_INTERVAL_TO_MSEC(ZB_GET_KEEPALIVE_TIMEOUT()));
      */
      ZB_SCHEDULE_ALARM(zb_nwk_ed_send_timeout_req, 0, zb_nwk_get_default_keepalive_timeout());
      if (!ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()))
      {
        if (ZB_TIME_GE(zb_zdo_pim_get_long_poll_ms_interval(), ZB_TIME_BEACON_INTERVAL_TO_MSEC(ZB_GET_KEEPALIVE_TIMEOUT())))
        {
          /*Set long poll interval in milliseconds*/
          TRACE_MSG(TRACE_NWK1, "set poll interval %ld msec %hd ed_timeout", (FMT__L_H, ZB_TIME_BEACON_INTERVAL_TO_MSEC(ZB_GET_KEEPALIVE_TIMEOUT()), ZB_GET_ED_TIMEOUT()));
          zb_zdo_pim_set_long_poll_interval(ZB_TIME_BEACON_INTERVAL_TO_MSEC(ZB_GET_KEEPALIVE_TIMEOUT()));
          zb_zdo_pim_start_poll(0);
        }
      }
      break;
    case MAC_DATA_POLL_KEEPALIVE:
      /* Our keepalive mode was ED_KEEPALIVE_DISABLED before we got
       * resp to ED timneout req from the parent. Now remember new
       * keepalive mode. */
      ZB_SET_KEEPALIVE_MODE(MAC_DATA_POLL_KEEPALIVE);
      if (ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()))
      {
        /*Set long poll interval in milliseconds*/
        TRACE_MSG(TRACE_NWK1, "set poll interval %ld msec %hd ed_timeout", (FMT__L_H, ZB_TIME_BEACON_INTERVAL_TO_MSEC(ZB_GET_KEEPALIVE_TIMEOUT()), ZB_GET_ED_TIMEOUT()));
        zb_zdo_pim_set_long_poll_interval(ZB_TIME_BEACON_INTERVAL_TO_MSEC(ZB_GET_KEEPALIVE_TIMEOUT()));
        zb_zdo_pim_start_poll(0);
      }
      else
      {
        /* Why ED_TIMEOUT_REQUEST_KEEPALIVE mode?! WHY?!&!& */
        //ZB_SET_KEEPALIVE_MODE(ED_TIMEOUT_REQUEST_KEEPALIVE);
        if (ZB_TIME_GE(zb_zdo_pim_get_long_poll_ms_interval(), ZB_TIME_BEACON_INTERVAL_TO_MSEC(ZB_GET_KEEPALIVE_TIMEOUT())))
        {
          zb_zdo_pim_set_long_poll_interval(ZB_TIME_BEACON_INTERVAL_TO_MSEC(ZB_GET_KEEPALIVE_TIMEOUT()));
        }
      }
      break;
    default:
      /* do nothing */
      break;
  }

#if defined ZB_MAC_POWER_CONTROL
  if (resp_param->parent_info & POWER_NEGOTIATION_SUPPORT &&
      ZB_NIB_GET_POWER_DELTA_PERIOD(0))
  {
    ZB_SCHEDULE_ALARM(zb_nwk_link_power_delta_alarm, 0, ZB_NIB_GET_POWER_DELTA_PERIOD(0));
  }
#endif  /* ZB_MAC_POWER_CONTROL */

  /* Free the buffer */
  zb_buf_free(param);

  TRACE_MSG(TRACE_NWK1, "<< ed_timeout_resp_ok", (FMT__0));
}

static void ed_timeout_resp_aged(zb_uint8_t param)
{
  TRACE_MSG(TRACE_NWK1, ">> ed_timeout_resp_aged param %hd", (FMT__H, param));

  ZB_SCHEDULE_CALLBACK(zb_zdo_forced_parent_link_failure, param);

  TRACE_MSG(TRACE_NWK1, "<< ed_timeout_resp_aged", (FMT__0));
}
#endif  /* ZB_LITE_NO_ED_AGING_REQ */
#endif  /* ZB_ED_FUNC */

#ifdef ZB_ROUTER_ROLE
void zb_update_ed_aging()
{
  zb_uindex_t i;

  ZB_ASSERT(ZG->nwk.handle.next_aging_end_device);

  if (zb_nwk_neighbor_table_size() != 0U)
  {
    /* If some device sent poll or timeout request find entry with nearest aging
     * time and reschedule ed_aging_timeout alarm. The new aging time is
     * calculated as difference between time_to_age of next_aging_ed and
     * current time. */
    zb_time_t next_exp_time;

    for (i = 0; i < ZB_NEIGHBOR_TABLE_SIZE; i++)
    {
      if (ZB_U2B(ZG->nwk.neighbor.neighbor[i].used)
          && (ZG->nwk.neighbor.neighbor[i].relationship == ZB_NWK_RELATIONSHIP_CHILD
              || ZG->nwk.neighbor.neighbor[i].relationship == ZB_NWK_RELATIONSHIP_UNAUTHENTICATED_CHILD)
          && (ZG->nwk.neighbor.neighbor[i].device_type == ZB_NWK_DEVICE_TYPE_ED)
          && ZB_TIME_GE(ZG->nwk.handle.next_aging_end_device->u.base.time_to_expire, ZG->nwk.neighbor.neighbor[i].u.base.time_to_expire))
      {
        ZB_ASSERT(ZG->nwk.handle.next_aging_end_device != NULL);
        ZG->nwk.handle.next_aging_end_device = &ZG->nwk.neighbor.neighbor[i];
      }
    }

    ZB_SCHEDULE_ALARM_CANCEL(zb_nwk_ed_aging_timeout, ZB_ALARM_ANY_PARAM);

    ZB_ASSERT(ZG->nwk.handle.next_aging_end_device != NULL);
    if (zb_nwk_check_aging())
    {
      TRACE_MSG(TRACE_NWK1, "ED aging timeout already expired: addr_ref %hd",(FMT__H, ZG->nwk.handle.next_aging_end_device->u.base.addr_ref));

      /* ED aging timeout expired. Remove device immediately. */
      zb_nwk_ed_aging_timeout(0);
    }
    else
    {
      next_exp_time = ZB_TIME_SUBTRACT(ZG->nwk.handle.next_aging_end_device->u.base.time_to_expire, ZB_TIMER_GET());
      ZB_SCHEDULE_ALARM(zb_nwk_ed_aging_timeout, 0, next_exp_time);

      TRACE_MSG(TRACE_NWK1, "Updating aging timeout: addr_ref %hd, exp_time %d",(FMT__H_D, ZG->nwk.handle.next_aging_end_device->u.base.addr_ref, next_exp_time));
    }
  }
  else
  {
    ZG->nwk.handle.next_aging_end_device = NULL;
    TRACE_MSG(TRACE_NWK1, "No more ED for aging", (FMT__0));
  }
}

void zb_nwk_restart_aging()
{
  zb_uindex_t i;
  zb_neighbor_tbl_ent_t  *nbt_ent = NULL;

  /* Find next aging time except device that we just kicked out */
  for (i = 0 ;i < ZB_NEIGHBOR_TABLE_SIZE  ;i++) /*Scan NBT*/
  {
    if (ZB_U2B(ZG->nwk.neighbor.neighbor[i].used)
        && (ZG->nwk.neighbor.neighbor[i].relationship == ZB_NWK_RELATIONSHIP_CHILD
            || ZG->nwk.neighbor.neighbor[i].relationship == ZB_NWK_RELATIONSHIP_UNAUTHENTICATED_CHILD)
        && (ZG->nwk.neighbor.neighbor[i].device_type == ZB_NWK_DEVICE_TYPE_ED)
        &&  (ZG->nwk.neighbor.neighbor[i].u.base.nwk_ed_timeout <= ED_AGING_TIMEOUT_16384MIN))
    {
      if (nbt_ent == NULL)
      {
        nbt_ent = &ZG->nwk.neighbor.neighbor[i];   /*First element*/
      }
      else
      {
        if (ZB_TIME_GE(nbt_ent->u.base.time_to_expire, ZG->nwk.neighbor.neighbor[i].u.base.time_to_expire))
        {
          nbt_ent = &ZG->nwk.neighbor.neighbor[i]; /* Find NBT entry with
                                                    * nearest aging time*/
        }
      }
    }
  }

  /* In case when our last ED child was removed alarm is running - cancel it now */
  ZB_SCHEDULE_ALARM_CANCEL(zb_nwk_ed_aging_timeout, ZB_ALARM_ANY_PARAM);

  if (nbt_ent != NULL) /* If found device with next aging time */
  {
    zb_time_t next_exp_time;

    ZG->nwk.handle.next_aging_end_device = nbt_ent;
    next_exp_time = ZB_TIME_SUBTRACT(nbt_ent->u.base.time_to_expire, ZB_TIMER_GET());
/*Schedule timeout*/
    ZB_SCHEDULE_ALARM(zb_nwk_ed_aging_timeout, 0, next_exp_time);
    TRACE_MSG(TRACE_NWK1, "Next aging ED: addr_ref %hd, timeout %d", (FMT__H_D, ZG->nwk.handle.next_aging_end_device->u.base.addr_ref, ZG->nwk.handle.next_aging_end_device->u.base.nwk_ed_timeout));
  }
  else
  {
    ZG->nwk.handle.next_aging_end_device = NULL;
    TRACE_MSG(TRACE_NWK1, "No more ED for aging", (FMT__0));
  }
}

void zb_stop_ed_aging(void)
{
  TRACE_MSG(TRACE_NWK1, "Stop ED aging", (FMT__0));

  ZB_SCHEDULE_ALARM_CANCEL(zb_nwk_ed_aging_timeout, ZB_ALARM_ANY_PARAM);
  ZG->nwk.handle.next_aging_end_device = NULL;
}

zb_bool_t zb_nwk_check_aging()
{
  zb_bool_t result = ZB_FALSE;

  if (ZB_TIME_GE(ZB_TIMER_GET(),ZG->nwk.handle.next_aging_end_device->u.base.time_to_expire))
  {
    result = ZB_TRUE;
  }
  return result;
}

void zb_nwk_ed_aging_timeout(zb_uint8_t param)
{

  TRACE_MSG(TRACE_NWK1, ">> zb_nwk_ed_aging_timeout", (FMT__0));
  ZVUNUSED(param);

  /*
    3.6.10.1 End Device Aging Mechanism
    When a neighbor table entry Timeout counter value reaches 0, parent
    shall delete the entry from its neighbor table.
  */
  if (ZG->nwk.handle.next_aging_end_device != NULL)
  {
#if !defined ZB_SKIP_ZEDS_AGING
    TRACE_MSG(TRACE_NWK1, "Forget device entry %hd", (FMT__H, ZG->nwk.handle.next_aging_end_device->u.base.addr_ref));
    ZB_SCHEDULE_CALLBACK2(zdo_device_removed,
                          ZG->nwk.handle.next_aging_end_device->u.base.addr_ref,
                          ZB_TRUE);
#endif

    /* clear pointer to the next aging ED to prevent updating ED aging before the device
     * will be removed. After device will be removed ED aging will be restarted.
     */
    ZG->nwk.handle.next_aging_end_device = NULL;

    ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_NEIGHBOR_STALE_ID);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "ED device for Aging is not specified", (FMT__0));
    ZB_ASSERT(0);
  }

  TRACE_MSG(TRACE_NWK1, "<< zb_nwk_ed_aging_timeout", (FMT__0));
}

void zb_nwk_ed_timeout_request_handler(zb_bufid_t buf, zb_nwk_hdr_t *nwk_hdr, zb_nwk_ed_timeout_request_t *cmd_ed_time_req)
{
  nwk_requested_timeout_status_t status;
  zb_neighbor_tbl_ent_t *nbt = NULL;
  zb_bool_t secure;
  zb_nwk_ed_timeout_response_t *time_resp;

  ZVUNUSED(nwk_hdr);
  secure = ZG->aps.authenticated && (ZB_NIB_SECURITY_LEVEL() > 0U);

  TRACE_MSG(TRACE_NWK1, ">> zb_nwk_ed_timeout_request_handler buf %p nwk_hdr %p nwk_cmd_rrep %p", (FMT__P_P_P, buf, nwk_hdr, cmd_ed_time_req));

  /*
    The parent shall find the neighbor table entry for the sending device and
    verify that the entry corresponds to an end device. If no entry is found or
    the entry is not an end device, then the message shall be dropped and no
    further processing should take place.
  */
  if ( zb_nwk_neighbor_get_by_short(nwk_hdr->src_addr, &nbt) == RET_OK
       && nbt != NULL
       && (nbt->relationship == ZB_NWK_RELATIONSHIP_CHILD
           || nbt->relationship == ZB_NWK_RELATIONSHIP_UNAUTHENTICATED_CHILD)
       && (nbt->device_type == ZB_NWK_DEVICE_TYPE_ED))
  {
    TRACE_MSG(TRACE_NWK1, "Address found in NBT", (FMT__0));

    /*
      If the Requested Timeout Enumeration value in the frame is not within the
      valid range, it shall generate an End Device Timeout Response command with a
      status of INCORRECT_VALUE and no further processing of the message shall take
      place.
    */
    if (/*cmd_ed_time_req->request_timeout >= ED_AGING_TIMEOUT_10SEC always true*/
#ifdef SNCP_MODE
      /* according to GBCS 3.2 10.7.3
         not change the value of a Device’s keepalive timeout where it receives
         an End Device Timeout Request command with a Requested Timeout Enumeration Value
         greater than 10 (so meaning greater than 1,024 minutes)
      */
      cmd_ed_time_req->request_timeout <= ED_AGING_TIMEOUT_1024MIN)
#else
      cmd_ed_time_req->request_timeout <= ED_AGING_TIMEOUT_16384MIN)
#endif
    {
      if (nbt->u.base.nwk_ed_timeout != cmd_ed_time_req->request_timeout)
      {
        nbt->u.base.nwk_ed_timeout = cmd_ed_time_req->request_timeout;

#ifdef ZB_USE_NVRAM
        zb_nvram_store_addr_n_nbt();
#endif

#if defined ZB_MAC_PENDING_BIT_SOURCE_MATCHING && defined ZB_MAC_POLL_INDICATION_CALLS_REDUCED
        {
          /* Update Poll Indication call timeout */
          zb_uint8_t nbt_idx = ZB_NWK_NEIGHBOR_GET_INDEX_BY_ENTRY_ADDRESS(nbt);

          zb_buf_get_out_delayed_ext(zb_nwk_src_match_add, nbt_idx, 0);
        }
#endif /* ZB_MAC_PENDING_BIT_SOURCE_MATCHING && ZB_MAC_POLL_INDICATION_CALLS_REDUCED */
      }

      TRACE_MSG(TRACE_INFO1, "request_timeout=%d", (FMT__D, cmd_ed_time_req->request_timeout));
      zb_init_ed_aging(nbt, cmd_ed_time_req->request_timeout, ZB_TRUE);

      /*
        The parent shall generate an End Device Timeout Response command with a
        status of SUCCESS. It shall fill in the value of the Parent Information
        Bitmask field according to the keepalive methods it supports.
      */

      status = ED_AGING_SUCCESS;
    } /*If the Requested Timeout value is not within the valid range*/
    else
    {
      status = ED_AGING_INCORRECT_VALUE;
    }
    /*Send ed timeout response */
    nwk_hdr = nwk_alloc_and_fill_hdr(buf, ZB_PIBCACHE_NETWORK_ADDRESS(), nwk_hdr->src_addr, ZB_FALSE, secure, ZB_TRUE, ZB_FALSE);

    time_resp = (zb_nwk_ed_timeout_response_t *)nwk_alloc_and_fill_cmd(buf, ZB_NWK_CMD_ED_TIMEOUT_RESPONSE, (zb_uint8_t)sizeof(zb_nwk_ed_timeout_response_t));

    time_resp->status = status;
    time_resp->parent_info = ZB_GET_KEEPALIVE_MODE();
#if defined ZB_MAC_POWER_CONTROL
    if (ZB_LOGICAL_PAGE_IS_SUB_GHZ(ZB_PIBCACHE_CURRENT_PAGE()))
    {
#ifdef ZB_CERTIFICATION_HACKS
      if (!ZB_CERT_HACKS().disable_power_negotiation_support)
#endif /* ZB_CERTIFICATION_HACKS */
      {
        time_resp->parent_info |= POWER_NEGOTIATION_SUPPORT;
      }
    }
#endif /* ZB_MAC_POWER_CONTROL */

    (void)zb_nwk_init_apsde_data_ind_params(buf, ZB_NWK_INTERNAL_NSDU_HANDLE);
    ZB_SCHEDULE_CALLBACK(zb_nwk_forward, buf);
  } /*End device found in the neighbor table*/
  else
  {
    /*Drop the buffer*/
    zb_buf_free(buf);
  }

  ZVUNUSED(nwk_hdr);

  TRACE_MSG(TRACE_NWK1, "<< zb_nwk_ed_timeout_request_handler", (FMT__0));
}
#endif /* ZB_ROUTER_ROLE */


#if defined ZB_ED_FUNC && !defined ZB_LITE_NO_ED_AGING_REQ
void nwk_timeout_resp_handler(zb_bufid_t buf, zb_nwk_hdr_t *nwk_hdr, zb_nwk_ed_timeout_response_t *cmd_ed_time_resp)
{
  zb_uint8_t resp_status;
  zb_uint8_t parent_info;
  zb_nwk_ed_timeout_response_param_t *resp_param;

  TRACE_MSG(TRACE_NWK1, ">> zb_nwk_ed_timeout_reqsponse_handler buf %p nwk_hdr %p ed_time_resp %p", (FMT__P_P_P, buf, nwk_hdr, cmd_ed_time_resp));

  ZB_SCHEDULE_ALARM_CANCEL(zb_nwk_ed_timeout_resp_recv_fail_trig, ZB_ALARM_ALL_CB);

  resp_status = cmd_ed_time_resp->status;
  parent_info = cmd_ed_time_resp->parent_info;

  resp_param = ZB_BUF_GET_PARAM(buf, zb_nwk_ed_timeout_response_param_t);
  resp_param->status = ED_KEEPALIVE_SUCCESS;
  resp_param->resp_status = resp_status;
  resp_param->parent_info = parent_info;

  ZB_SCHEDULE_CALLBACK(process_ed_timeout_resp, buf);

  TRACE_MSG(TRACE_NWK1, "<< zb_nwk_ed_timeout_reqsponse_handler",  (FMT__0));
}
#endif  /* ZB_ED_FUNC & !no aging */

#ifdef ZB_ROUTER_ROLE

zb_uint32_t zb_convert_timeout_value(zb_uint8_t timeout)
{
  if (/*timeout >= ED_AGING_TIMEOUT_10SEC always true*/
      timeout <= ED_AGING_TIMEOUT_16384MIN)
  {
    return nwk_ed_aging_timeout_sec[timeout];
  }
  else
  {
    return 0;
  }
}

static void zb_run_ed_aging(void)
{
  if (ZG->nwk.handle.next_aging_end_device != NULL)
  {
    /* ED aging is running. Update. */
    zb_update_ed_aging();
  }
  else
  {
    /* ED aging is not running. Restart. */
    zb_nwk_restart_aging();
  }
}

void zb_init_ed_aging(zb_neighbor_tbl_ent_t *nbt, zb_uint8_t timeout, zb_bool_t run_aging)
{
  zb_time_t aging_time;

  /*
    The received value shall be converted into an actual timeout amount. This
    shall be done by obtaining the actual timeout value for the corresponding
    Requested Timeout Enumeration in Table 3.44. The value shall be converted
    from minutes into seconds if it is not already a value in seconds. The parent
    shall set the Timeout Counter and Device Timeout values of the neighbor table
    entry to the converted value.
  */
  nbt->u.base.nwk_ed_timeout = timeout;
  aging_time  = ZB_TIME_ADD(ZB_TIMER_GET(), (ZB_TIME_ONE_SECOND * nwk_ed_aging_timeout_sec[timeout]));
  nbt->u.base.time_to_expire = aging_time;

  TRACE_MSG(TRACE_NWK1, "Init ED aging: ed_timeout = %hd, aging time = %d, addr_ref %hd",
            (FMT__H_D_H, nbt->u.base.nwk_ed_timeout, nbt->u.base.time_to_expire, nbt->u.base.addr_ref));

  if (run_aging)
  {
    /*
      The parent shall set the End Device Configuration information in the neighbor
      table for the corresponding end device entry to the value of the End Device
      Configuration field in the received message.
    */
    nbt->keepalive_received = ZB_TRUE_U;

    /* run or update ED aging mechanism */
    zb_run_ed_aging();
  }
}

static void send_empty_mac_frame_if_pending(zb_uint8_t param);


static void update_aging_info_on_poll(zb_neighbor_tbl_ent_t* nbt)
{
  TRACE_MSG(TRACE_NWK3, ">> update_aging_info_on_poll, nbt %p", (FMT__P, nbt));

  if (ZB_GET_KEEPALIVE_MODE() == MAC_DATA_POLL_KEEPALIVE
      || ZB_GET_KEEPALIVE_MODE() == BOTH_KEEPALIVE_METHODS)
  {
    nbt->u.base.time_to_expire = ZB_TIME_ADD(ZB_TIMER_GET(), (ZB_TIME_ONE_SECOND * nwk_ed_aging_timeout_sec[nbt->u.base.nwk_ed_timeout]));
    TRACE_MSG(TRACE_NWK1, "Updated aging time %d",(FMT__D, nbt->u.base.time_to_expire));
    nbt->keepalive_received = ZB_TRUE_U;

    /* If ZC have a ZED child device, it can crash after a reset.
       This happens when ZC is turned off and then turned on before
       a child begins to rejoin. ZED sends data request and ZC tries
       to use the null 'ZG->nwk.handle.next_aging_end_device' pointer
       in order to update ZED aging info.

       So, need use zb_run_ed_aging() instead zb_update_ed_aging().
    */
    zb_run_ed_aging();
  }

  TRACE_MSG(TRACE_NWK3, "<< update_aging_info_on_poll", (FMT__0));
}


/*3.6.10.4 MAC Data Poll Processing*/
void zb_mcps_poll_indication(zb_uint8_t param)
{
  zb_mac_mhr_t mhr;

  TRACE_MSG(TRACE_NWK1, "> zb_mcps_poll_indication param %hd",(FMT__H, param));
  (void)zb_parse_mhr(&mhr, param);

  if (!ZB_JOINED())
  {
    zb_buf_free(param);
  }
  else
  {
    zb_ret_t ret;
    zb_neighbor_tbl_ent_t *nbt = NULL;
    /*
      The parent shall find the neighbor table entry for the sending device and
      verify that the entry corresponds to an end device. If no entry is found or
      the entry is not an end device, then the message shall be dropped and no
      further processing should take place.
    */
    if (ZB_FCF_GET_SRC_ADDRESSING_MODE(mhr.frame_control) == ZB_ADDR_16BIT_DEV_OR_BROADCAST)
    {
      ret = zb_nwk_neighbor_get_by_short(mhr.src_addr.addr_short, &nbt);
    }
    else if (ZB_FCF_GET_SRC_ADDRESSING_MODE(mhr.frame_control) == ZB_ADDR_64BIT_DEV)
    {
      ret = zb_nwk_neighbor_get_by_ieee(mhr.src_addr.addr_long, &nbt);
    }
    else
    {
      ret = RET_NOT_IMPLEMENTED;
    }

    if (ret == RET_OK
        && nbt != NULL
        && (nbt->relationship == ZB_NWK_RELATIONSHIP_CHILD
            || nbt->relationship == ZB_NWK_RELATIONSHIP_UNAUTHENTICATED_CHILD)
        && (nbt->device_type == ZB_NWK_DEVICE_TYPE_ED))
    {
      TRACE_MSG(TRACE_NWK1, "Got keepalive data request from 0x%x",
                (FMT__H, mhr.src_addr.addr_short));

#if !defined ZB_ED_ROLE && defined ZB_MAC_DUTY_CYCLE_MONITORING && defined ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ
      if (nbt->is_subghz)
      {
        nbt_inc_in_pkt_count(nbt);
        // remove it        nbt->suspended = ZB_FALSE;
      }
#endif

      update_aging_info_on_poll(nbt);

      send_empty_mac_frame_if_pending(param);
    }
#ifndef ZB_LITE_NO_LEAVE_INJECTION
    else
    {
      /*
        If there is no entry in the neighbor table corresponding to the DeviceAddress of
        the MLME-Poll.Indication primitive, then the device shall construct a leave
        message. The destination NWK address shall be set to the value of the MAC
        source of the MAC data poll. See section 3.6.10.4.1 for more information on
        the leave message.

        NWK Leave Request:
        A device that chooses to send a NWK leave request shall set fields of the NWK
        Command as follows.
        The destination IEEE address sub-field of the frame control shall be set to
        0, indicating that no destination IEEE address is present.
        The destination IEEE address field shall not be present in the message.
        The request sub-field of the command options field shall be set to 1.
        The rejoin request sub-field of the command shall be set to 1.
      */
      TRACE_MSG(TRACE_NWK1, "Data request from the unknown device. Send Leave to 0x%x",(FMT__H, mhr.src_addr.addr_short));

      /* We actually already do not know that ZED. We sent MAC ACK with Pending
       * bit set. Now "inject" LEAVE: send it now directly while ZED waits for
       * it as an indirect tx. */
      zb_nwk_send_direct_leave_req(param, mhr.src_addr.addr_short);
    }
#endif  /* #ifndef ZB_LITE_NO_LEAVE_INJECTION */
  }

  TRACE_MSG(TRACE_NWK1, "< zb_mcps_poll_indication", (FMT__0));
}

static void send_empty_mac_frame_if_pending(zb_uint8_t param)
{
#ifndef ZB_LITE_NO_LEAVE_INJECTION
  /* If we are in OOM state, we'd better just ignore already acked data requests */
  if (zb_buf_get_status(param) == RET_OK
      /*cstat !MISRAC2012-Rule-13.5 */
      /* After some investigation, the following violation of Rule 13.5 seems to be
       * a false positive. There are no side effect to 'zb_buf_is_oom_state()'. This
       * violation seems to be caused by the fact that 'zb_buf_is_oom_state()' is an
       * external function, which cannot be analyzed by C-STAT. */
      && !zb_buf_is_oom_state())
  {
    /* status == RET_OK means we set Pending bit in ack */
    zb_mac_resp_by_empty_frame(param);
  }
  else
#endif  /* #ifndef ZB_LITE_NO_LEAVE_INJECTION  */
  {
    zb_buf_free(param);
  }
}

#if defined ZB_MAC_PENDING_BIT_SOURCE_MATCHING

#ifdef ZB_MAC_POLL_INDICATION_CALLS_REDUCED
static zb_uint16_t zb_nwk_get_poll_indication_call_timeout(zb_uint8_t nbt_idx)
{
  zb_neighbor_tbl_ent_t *ent = &ZG->nwk.neighbor.neighbor[nbt_idx];
  zb_uint32_t poll_timeout;

  /* Divide ED timeout by four to be sure that a child won't be incorrectly aged out */
  poll_timeout = (nwk_ed_aging_timeout_sec[ent->u.base.nwk_ed_timeout] / 4);

  if (poll_timeout > ZB_MAC_POLL_INDICATION_CALL_MAX_TIMEOUT)
  {
    /* Limit the poll timeout value */
    poll_timeout = ZB_MAC_POLL_INDICATION_CALL_MAX_TIMEOUT;
  }

  /* Should fit into 2 bytes varible */
  ZB_ASSERT(poll_timeout <= ZB_UINT16_MAX);

  return (zb_uint16_t)poll_timeout;
}
#endif /* ZB_MAC_POLL_INDICATION_CALLS_REDUCED */

static void zb_nwk_src_match_add_ent(zb_uint8_t param, zb_uint8_t nbt_idx, zb_callback_t cb)
{
  zb_uint16_t short_addr;
  zb_mlme_set_request_t *req;
  zb_mac_src_match_params_t *src_match;
  zb_neighbor_tbl_ent_t *ent;

  TRACE_MSG(TRACE_NWK1, ">>zb_nwk_src_match_add_ent param %hd nbt_idx %hd",
            (FMT__H_H, param, nbt_idx));

  ent = &ZG->nwk.neighbor.neighbor[nbt_idx];
  zb_address_short_by_ref(&short_addr, ent->u.base.addr_ref);

  /* Sanity check for Bcast addresses */
  ZB_ASSERT(!ZB_NWK_IS_ADDRESS_BROADCAST(short_addr));

  /* The current approach is to simply call mlme_set_req(). If multimac was implemented,
   * short_address parameter can be used to select which MAC should be called */
  req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_mac_src_match_params_t));

  req->pib_attr = ZB_PIB_ATTRIBUTE_SRC_MATCH;
  req->pib_index = 0;
  req->pib_length = (zb_uint8_t)sizeof(zb_mac_src_match_params_t);
  req->confirm_cb_u.cb = cb;

  /*cstat !MISRAC2012-Rule-11.3 */
  /** @mdr{00002,29} */
  src_match = (zb_mac_src_match_params_t *)(req+1);
  src_match->cmd = ZB_MAC_SRC_MATCH_CMD_ADD;
  src_match->index = nbt_idx;
  src_match->addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
  src_match->addr.addr_short = short_addr;
#ifdef ZB_MAC_POLL_INDICATION_CALLS_REDUCED
  src_match->poll_timeout = zb_nwk_get_poll_indication_call_timeout(nbt_idx);
#endif /* ZB_MAC_POLL_INDICATION_CALLS_REDUCED */

  ZB_SCHEDULE_CALLBACK(zb_mlme_set_request, param);

  TRACE_MSG(TRACE_NWK1, "<<zb_nwk_src_match_add_ent", (FMT__0));
}

void zb_nwk_src_match_add(zb_uint8_t param, zb_uint16_t nbt_idx)
{
  ZB_ASSERT(nbt_idx <= ZB_UINT8_MAX);
  zb_nwk_src_match_add_ent(param, (zb_uint8_t)nbt_idx, NULL);
}

/* Use Nordic implementtaion: it fills both index and address, so
 * compatible with TI's HW and all other's SW implementations; also it
 * integrated with zb_nwk_neighbor_complete_deletion(). */
void zb_nwk_src_match_delete(zb_uint8_t param, zb_address_ieee_ref_t ieee_ref)
{
  zb_mlme_set_request_t *req;
  zb_mac_src_match_params_t *src_match;
  zb_uint16_t short_addr;
  zb_uint8_t nbt_idx = ZG->nwk.neighbor.addr_to_neighbor[ieee_ref];

  zb_address_short_by_ref(&short_addr, ieee_ref);

  TRACE_MSG(TRACE_NWK1, ">> zb_nwk_src_match_delete param %hd index %hf",
            (FMT__H_H, param, nbt_idx));

  /* Sanity check for Bcast addresses */
  ZB_ASSERT(!ZB_NWK_IS_ADDRESS_BROADCAST(short_addr));

  req = zb_buf_initial_alloc(param,
    sizeof(zb_mlme_set_request_t) + sizeof(zb_mac_src_match_params_t));

  req->pib_attr = ZB_PIB_ATTRIBUTE_SRC_MATCH;
  req->pib_index = 0;
  req->pib_length = (zb_uint8_t)sizeof(zb_mac_src_match_params_t);
  req->confirm_cb_u.cb = NULL;

  /*cstat !MISRAC2012-Rule-11.3 */
  /** @mdr{00002,30} */
  src_match = (zb_mac_src_match_params_t *)(req+1);
  src_match->cmd = ZB_MAC_SRC_MATCH_CMD_DELETE;
  src_match->index = nbt_idx;
  src_match->addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
  src_match->addr.addr_short = short_addr;

  ZB_SCHEDULE_CALLBACK(zb_mlme_set_request, param);
  /* There was zb_nwk_neighbor_complete_deletion(). Now moved to nwk_neighbor_delete_ed() */

  TRACE_MSG(TRACE_NWK1, "<< zb_nwk_src_match_delete", (FMT__0));
}


void zb_nwk_src_match_drop(zb_uint8_t param)
{
  zb_mlme_set_request_t *req;
  zb_mac_src_match_params_t *src_match;

  TRACE_MSG(TRACE_NWK1, ">> zb_nwk_src_match_drop param %hd", (FMT__H, param));

  req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_mac_src_match_params_t));

  req->pib_attr = ZB_PIB_ATTRIBUTE_SRC_MATCH;
  req->pib_index = 0;
  req->pib_length = (zb_uint8_t)sizeof(zb_mac_src_match_params_t);
  req->confirm_cb_u.cb = NULL;

  /*cstat !MISRAC2012-Rule-11.3 */
  /** @mdr{00002,31} */
  src_match = (zb_mac_src_match_params_t *)(req+1);
  /* Fill only cmd. Other fields will not be analyzed */
  src_match->cmd = ZB_MAC_SRC_MATCH_CMD_DROP;

  ZB_SCHEDULE_CALLBACK(zb_mlme_set_request, param);

  TRACE_MSG(TRACE_NWK1, "<< zb_nwk_src_match_drop", (FMT__0));
}

static void zb_nwk_src_match_restore_iter(zb_uint8_t param)
{
  TRACE_MSG(TRACE_NWK1, ">> nwk_src_match_restore_iter param %hd", (FMT__H, param));

  while (ZG->nwk.handle.src_match_nbr_idx < ZB_NEIGHBOR_TABLE_SIZE)
  {
    zb_neighbor_tbl_ent_t *ent =
      &ZG->nwk.neighbor.neighbor[ZG->nwk.handle.src_match_nbr_idx];

    if (ent->device_type == ZB_NWK_DEVICE_TYPE_ED)
    {
      TRACE_MSG(TRACE_NWK1, "Restore nbr %hd", (FMT__H, ZG->nwk.handle.src_match_nbr_idx));
      zb_nwk_src_match_add_ent(param, ZG->nwk.handle.src_match_nbr_idx,
                               zb_nwk_src_match_restore_iter);
      break;
    }
    ZG->nwk.handle.src_match_nbr_idx++;
  }

  if (ZG->nwk.handle.src_match_nbr_idx < ZB_NEIGHBOR_TABLE_SIZE)
  {
    ZG->nwk.handle.src_match_nbr_idx++;
  }
  else
  {
    TRACE_MSG(TRACE_NWK1, "Src match restore done", (FMT__0));
    ZB_SCHEDULE_CALLBACK(zb_nwk_load_pib_stm, param);
  }

  TRACE_MSG(TRACE_NWK1, "<< nwk_src_match_restore_iter", (FMT__0));
}

void zb_nwk_src_match_restore(zb_uint8_t param)
{
  TRACE_MSG(TRACE_NWK1, ">> zb_nwk_src_match_restore param %hd", (FMT__H, param));

  ZG->nwk.handle.src_match_nbr_idx = 0;
  zb_nwk_src_match_restore_iter(param);

  TRACE_MSG(TRACE_NWK1, "<< zb_nwk_src_match_restore", (FMT__0));
}
#endif  /* ZB_MAC_PENDING_BIT_SOURCE_MATCHING */

#endif  /* ZB_ROUTER_ROLE */
