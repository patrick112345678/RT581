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
/*  PURPOSE: NWK Link Power Delta command
*/

#define ZB_TRACE_FILE_ID 41
#include "zb_common.h"
#include "zboss_api.h"
#include "zb_mac.h"
#include "zb_nwk.h"
#include "zb_secur.h"
#include "zb_aps.h"
#include "nwk_internal.h"
#include "zb_zdo.h"
#include "zb_watchdog.h"
#include "zb_bufpool.h"

#if defined ZB_MAC_POWER_CONTROL
static void zb_nwk_send_link_power_delta_notification(zb_uint8_t param,
                                                           zb_uint16_t mac_tbl_idx);

void zb_nwk_link_power_delta_alarm(zb_uint8_t mac_tbl_idx)
{
  TRACE_MSG(TRACE_NWK1, ">> zb_nwk_link_power_control_alarm mac_tbl_idx %d joined %hd",
            (FMT__D_H, mac_tbl_idx, ZB_JOINED()));

  if (ZB_JOINED())
  {
    if (zb_buf_get_out_delayed_ext(zb_nwk_send_link_power_delta_notification, mac_tbl_idx, 0) != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "Out of memory, skip sending NWK link power delta", (FMT__0));
      ZB_SCHEDULE_ALARM(zb_nwk_link_power_delta_alarm, mac_tbl_idx,
                        ZB_NIB_GET_POWER_DELTA_PERIOD(mac_tbl_idx));
    }
  }

  TRACE_MSG(TRACE_NWK1, "<< zb_nwk_link_power_control_alarm", (FMT__0));
}


#if defined ZB_ROUTER_ROLE
static zb_uint8_t zb_nwk_prepare_power_delta_cmd(zb_uint8_t *ptr, zb_uint8_t max_count)
{
  zb_uint8_t i;
  zb_uint16_t addr;
  zb_uint8_t count = 0;
  zb_neighbor_tbl_ent_t *nbt;

  for (i = 0; (i < max_count) &&
         (zb_nwk_get_sorted_neighbor(&(ZG->nwk.handle.send_power_delta_index), &nbt) == RET_OK);
       i++)
  {
    if (nbt->rx_on_when_idle)
    {
      zb_address_short_by_ref(&addr, nbt->u.base.addr_ref);
      ZB_HTOLE16(ptr, &addr);
      ptr += sizeof(zb_uint16_t); /* address shift */

      /* Real power delta based on NBT stored RSSI values */
      *ptr = ZB_MAC_POWER_CONTROL_OPT_SIGNAL_LEVEL - nbt->rssi;
      ptr++;

      count++;
    }

    ZG->nwk.handle.send_power_delta_index++;
  }

  /* Skip all unsuitable entries */
  while (zb_nwk_get_sorted_neighbor(&(ZG->nwk.handle.send_power_delta_index), &nbt) == RET_OK &&
         !((nbt->rx_on_when_idle)))
  {
    ZG->nwk.handle.send_power_delta_index++;
  }

  /* If the entry doesn't exist, we have reached the end */
  if (zb_nwk_get_sorted_neighbor(&(ZG->nwk.handle.send_power_delta_index), &nbt) != RET_OK)
  {
    ZG->nwk.handle.send_power_delta_index = (zb_address_ieee_ref_t)(-1);
  }

  return count;
}


static void prepare_lpd_broadcast(zb_uint8_t param)
{
  zb_nwk_hdr_t *nwk_hdr;
  zb_uint8_t payload_pos;
  zb_uint8_t count;
  zb_uint8_t max_count;
  zb_address_ieee_ref_t ref_p;
  zb_nwk_link_power_delta_payload_t *payload_ptr;
  zb_uint8_t *list_ptr;
  zb_bool_t secure = (zb_bool_t)(ZB_NIB_SECURITY_LEVEL());

  nwk_hdr = nwk_alloc_and_fill_hdr(param,
                                   ZB_PIBCACHE_NETWORK_ADDRESS(),
                                   ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE,
                                   ZB_FALSE, secure, ZB_TRUE, ZB_FALSE);
  nwk_hdr->radius = 1;

  TRACE_MSG(TRACE_NWK2, "power_delta_idx before %hd",
            (FMT__H, ZG->nwk.handle.send_power_delta_index));

  /* Sanity check */
  if (zb_address_by_sorted_table_index(ZG->nwk.handle.send_power_delta_index, &ref_p) != RET_OK)
  {
    /* This is a situation, where link_status_index referes to index of
     * already removed device */
    TRACE_MSG(TRACE_NWK2, "idx is not found, reset", (FMT__0));
    ZG->nwk.handle.send_power_delta_index = (zb_address_ieee_ref_t)0;
  }

  payload_ptr = (zb_nwk_link_power_delta_payload_t *)nwk_alloc_and_fill_cmd(
    param, ZB_NWK_CMD_LINK_POWER_DELTA, sizeof(zb_nwk_link_power_delta_payload_t));
  payload_pos = ((zb_uint8_t *)payload_ptr - (zb_uint8_t*)zb_buf_begin(param));

  /* Fill power delta list */
  max_count = (ZB_NWK_MAX_BROADCAST_PAYLOAD_SIZE - sizeof(zb_uint8_t) - sizeof(zb_nwk_link_power_delta_payload_t)) / sizeof(zb_nwk_power_list_entry_t);

  list_ptr = zb_buf_alloc_right(param, max_count * sizeof(zb_nwk_power_list_entry_t));

  count = zb_nwk_prepare_power_delta_cmd(list_ptr, max_count);

  TRACE_MSG(TRACE_NWK2, "max_count %d count %d power_delta_index %d",
            (FMT__H_H_H, max_count, count, ZG->nwk.handle.send_power_delta_index));

  zb_buf_cut_right(param, (max_count-count) * sizeof(zb_nwk_power_list_entry_t));

  /* Fill power delta payload */
  payload_ptr = (zb_nwk_link_power_delta_payload_t *)((zb_uint8_t*)zb_buf_begin(param) + payload_pos);
  payload_ptr->cmd_options = ZB_NWK_LPD_CMD_OPTIONS_NOTIFICATION;
  payload_ptr->list_count = count;
}
#endif  /* ZB_ROUTER_ROLE */


static zb_ret_t prepare_lpd_unicast(zb_uint8_t param, zb_uint16_t short_addr)
{
  zb_nwk_hdr_t *nwk_hdr;
  zb_nwk_link_power_delta_payload_t *payload_ptr;
  zb_uint8_t *list_ptr;
  zb_bool_t secure = (zb_bool_t)(ZB_NIB_SECURITY_LEVEL());
  zb_neighbor_tbl_ent_t *nbt;
  zb_address_ieee_ref_t addr_ref;
  zb_ret_t ret;

  ret = zb_address_by_short(short_addr, ZB_FALSE, ZB_FALSE, &addr_ref);
  if (ret == RET_OK)
  {
    ret = zb_nwk_neighbor_get(addr_ref, ZB_FALSE, &nbt);
  }

  if (ret == RET_OK)
  {
    nwk_hdr = nwk_alloc_and_fill_hdr(param,
                                     ZB_PIBCACHE_NETWORK_ADDRESS(),
                                     short_addr,
                                     ZB_FALSE, secure, ZB_TRUE, ZB_FALSE);
    nwk_hdr->radius = 1;

    TRACE_MSG(TRACE_NWK2, "power_delta_idx before %hd",
              (FMT__H, ZG->nwk.handle.send_power_delta_index));

    payload_ptr = (zb_nwk_link_power_delta_payload_t *)nwk_alloc_and_fill_cmd(
      param, ZB_NWK_CMD_LINK_POWER_DELTA,
      sizeof(zb_nwk_link_power_delta_payload_t) + sizeof(zb_nwk_power_list_entry_t));

    /* Fill power delta payload */
#if defined ZB_ROUTER_ROLE
    if (ZB_IS_DEVICE_ZC_OR_ZR())
    {
      payload_ptr->cmd_options = ZB_NWK_LPD_CMD_OPTIONS_RESPONSE;
    }
    else
#endif  /* ZB_ROUTER_ROLE */
    {
      if (ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()) && ZB_NIB_NWK_LPD_CMD_MODE() == ZB_NWK_LPD_NOTIFICATION)
      {
        payload_ptr->cmd_options = ZB_NWK_LPD_CMD_OPTIONS_NOTIFICATION;
      }
      else
      {
        payload_ptr->cmd_options = ZB_NWK_LPD_CMD_OPTIONS_REQUEST;
      }
    }
    payload_ptr->list_count = 1;

    list_ptr = (zb_uint8_t *)(payload_ptr + 1);

    ZB_HTOLE16(list_ptr, &short_addr);
    list_ptr += sizeof(zb_uint16_t); /*address shift */

    /* Real power delta based on NBT stored RSSI values */
    *list_ptr = ZB_MAC_POWER_CONTROL_OPT_SIGNAL_LEVEL - nbt->rssi;
  }

  return ret;
}


static void zb_nwk_send_link_power_delta_notification(zb_uint8_t param,
                                                           zb_uint16_t mac_tbl_idx)
{
  TRACE_MSG(TRACE_NWK1, ">> zb_nwk_send_link_power_delta_command param %hd",
            (FMT__H, param));

  if (ZB_JOINED())
  {
    zb_time_t next_alarm_tmo =
      ZB_RANDOM_JTR(ZB_NWK_LINK_POWER_DELTA_TRANSMIT_RATE_JITTER * ZB_TIME_ONE_SECOND);

    /* Identify ourselves here */
#if defined ZB_ROUTER_ROLE
    if (ZB_IS_DEVICE_ZC_OR_ZR())
    {
      prepare_lpd_broadcast(param);
    }
    else
#endif  /* ZB_ROUTER_ROLE */
    {
      zb_uint16_t parent_addr;

      zb_address_short_by_ref(&parent_addr, ZG->nwk.handle.parent);
      prepare_lpd_unicast(param, parent_addr);
    }

    /* Refresh counter if all entries have been processed */
    if (ZB_IS_DEVICE_ZED()
#if defined ZB_ROUTER_ROLE
        || ZG->nwk.handle.send_power_delta_index == (zb_address_ieee_ref_t)(-1)
#endif
      )
    {
#if defined ZB_ROUTER_ROLE
      ZG->nwk.handle.send_power_delta_index = (zb_address_ieee_ref_t)(0);
#endif

      next_alarm_tmo = ZB_NIB_GET_POWER_DELTA_PERIOD(mac_tbl_idx);
    }

    /* Mark as internal buffer */
    zb_nwk_init_apsde_data_ind_params(param, ZB_NWK_INTERNAL_NSDU_HANDLE);

    ZB_SCHEDULE_CALLBACK(zb_nwk_forward, param);

    ZB_SCHEDULE_ALARM(zb_nwk_link_power_delta_alarm, mac_tbl_idx,
			next_alarm_tmo);

#ifndef ZB_COORDINATOR_ONLY
    if (ZB_IS_DEVICE_ZED() &&
        !ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()))
    {
      /* Poll for LPD response */
      TRACE_MSG(TRACE_ZDO3, "Call zb_zdo_pim_start_turbo_poll_packets 1", (FMT__0));
      zb_zdo_pim_start_turbo_poll_packets(1);
    }
#endif
  }
  else
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_NWK1, "<< zb_nwk_send_link_power_delta_command", (FMT__0));
}


void zb_nwk_handle_link_power_delta_command(zb_bufid_t buf,
                                                 zb_nwk_hdr_t *nwk_hdr,
                                                 zb_uint8_t hdr_size)
{
  zb_nwk_link_power_delta_payload_t *lpd_payload;
  zb_uint8_t *list_ptr;
  zb_uint8_t count;
  zb_uint16_t src_addr;
  zb_ieee_addr_t src_ieee;
  zb_address_ieee_ref_t src_ref;
  zb_neighbor_tbl_ent_t *src_nbt;
  zb_ret_t ret;
  zb_int8_t tx_power_diff = 0;

  ZVUNUSED(nwk_hdr);
  ZVUNUSED(hdr_size);

  /* 3.4.13.4 Link Power Delta command behavior */
  /* Upon receipt ... */
  ZB_LETOH16(&src_addr, &nwk_hdr->src_addr);

  TRACE_MSG(TRACE_NWK1, ">> zb_nwk_handle_link_power_delta_command", (FMT__0));

  ret = zb_address_by_short(src_addr, ZB_FALSE, ZB_FALSE, &src_ref);
  if (ret == RET_OK)
  {
    ret = zb_nwk_neighbor_get(src_ref, ZB_FALSE, &src_nbt);
  }

  if (ret == RET_OK)
  {
    lpd_payload = (zb_nwk_link_power_delta_payload_t *)ZB_NWK_CMD_FRAME_GET_CMD_PAYLOAD(buf, hdr_size);

    TRACE_MSG(TRACE_NWK1, "LPD cmd %hd, count %hd",
              (FMT__H_H, lpd_payload->cmd_options, lpd_payload->list_count));

    /* Verify all cmds - check list_count == actual list size */
    count = (zb_buf_len(buf) - ((zb_uint8_t *)(lpd_payload + 1) - (zb_uint8_t*)zb_buf_begin(buf))) /
      sizeof(zb_nwk_power_list_entry_t);

    TRACE_MSG(TRACE_NWK1, "list count: indicated %hd, actual %hd",
              (FMT__H_H, lpd_payload->list_count, count));

    if (count != lpd_payload->list_count ||
        lpd_payload->list_count == 0)
    {
      TRACE_MSG(TRACE_ERROR, "Drop inconsistent frame", (FMT__0));
      ret = RET_ERROR;
    }
  }

  if (ret == RET_OK)
  {
    zb_uint8_t cmd_options;

    cmd_options = lpd_payload->cmd_options & ZB_NWK_LPD_CMD_OPTIONS_MASK;
    /* Verify request/response - they should contain only one entry */
    if (((cmd_options == ZB_NWK_LPD_CMD_OPTIONS_REQUEST) ||
         (cmd_options == ZB_NWK_LPD_CMD_OPTIONS_RESPONSE)) &&
        lpd_payload->list_count != 1)
    {
      TRACE_MSG(TRACE_ERROR, "Unexpected multiple req/resp", (FMT__0));
      ret = RET_ERROR;
    }
  }

  /* 3.4.13.4 of R22 Zigbee spec: Examine Link Power Delta command and find the Device Address
   * in the payload of the message that matches the nwkNetworkAddress value in its NIB.
   * If no match is found and the receiving device is an End Device, then the message shall be dropped
   * and no further processing shall be done.
   */
  if (ret == RET_OK)
  {
    zb_uint8_t i;
    zb_uint16_t addr;

    list_ptr = (zb_uint8_t *)(lpd_payload + 1);
    for (i = 0; i < lpd_payload->list_count; i++)
    {
      ZB_LETOH16(&addr, list_ptr);
      if (addr == ZB_PIBCACHE_NETWORK_ADDRESS())
      {
        list_ptr += 2;
        tx_power_diff = *list_ptr;
        break;
      }
      list_ptr += sizeof(zb_nwk_power_list_entry_t);
    }

    if (i == lpd_payload->list_count)
    {
      TRACE_MSG(TRACE_ERROR, "Can't find myself", (FMT__0));
      ret = RET_ERROR;
    }
  }

  if (ret == RET_OK)
  {
    zb_mlme_set_power_info_tbl_req_t *req = ZB_BUF_GET_PARAM(buf, zb_mlme_set_power_info_tbl_req_t);

#if defined ZB_ROUTER_ROLE
    ZG->nwk.handle.lpd_resp_addr = 0xFFFF;

    if (src_nbt->device_type == ZB_NWK_DEVICE_TYPE_ED &&
        lpd_payload->cmd_options == ZB_NWK_LPD_CMD_OPTIONS_REQUEST)
    {
      ZG->nwk.handle.lpd_resp_addr = src_addr;
    }
#endif  /* ZB_ROUTER_ROLE */

    /* Apply new link power and  */
    req->ent.short_addr = src_addr;
    if (zb_address_ieee_by_short(src_addr, src_ieee) == RET_OK)
    {
      ZB_IEEE_ADDR_COPY(req->ent.ieee_addr, src_ieee);
    }
    else
    {
      ZB_IEEE_ADDR_COPY(req->ent.ieee_addr, g_unknown_ieee_addr);
    }
    /* Again, spec violation - pass only power diff, not the absolute TX power */
    req->ent.tx_power = tx_power_diff;
    req->ent.last_rssi = src_nbt->rssi;
    req->ent.nwk_negotiated = 1;

    ZB_SCHEDULE_CALLBACK(zb_mlme_set_power_information_table_request, buf);
  }

  if (ret != RET_OK)
  {
    /* Free inconsistent buffer */
    TRACE_MSG(TRACE_ERROR, "inconsistent LPD frame", (FMT__0));
    zb_buf_free(buf);
  }
  /* FIXME: EE: double free in case of error? Where is a leak at TH? At the Host?   */
  /* TODO: EA:Need to fix real leak issue. This is not good fix, but probably leak exists only in macsplit. Works for TH. */
  #ifdef ZB_TH_ENABLED
  zb_buf_free(buf);
  #endif

  TRACE_MSG(TRACE_NWK1, "<< zb_nwk_handle_link_power_delta_command", (FMT__0));
}


void zb_nwk_lpd_joined_parent(zb_uint8_t param, zb_uint16_t short_addr)
{
  zb_mlme_set_power_info_tbl_req_t *req = ZB_BUF_GET_PARAM(param, zb_mlme_set_power_info_tbl_req_t);
  zb_ieee_addr_t ieee_addr;
  zb_neighbor_tbl_ent_t *ent;
  zb_address_ieee_ref_t ref;
  zb_ret_t ret;

  ret = zb_address_by_short(short_addr, ZB_FALSE, ZB_FALSE, &ref);
  if (ret == RET_OK)
  {
    ret = zb_nwk_neighbor_get(ref, ZB_FALSE, &ent);
  }

  if (ret == RET_OK)
  {
      req->ent.short_addr = short_addr;
      if (zb_address_ieee_by_short(short_addr, ieee_addr) == RET_OK)
      {
        ZB_IEEE_ADDR_COPY(req->ent.ieee_addr, ieee_addr);
      }
      else
      {
        ZB_IEEE_ADDR_COPY(req->ent.ieee_addr, g_unknown_ieee_addr);
      }
      /* Again, spec violation - pass only power diff, not the absolute TX power */
      req->ent.tx_power = 0;
      req->ent.last_rssi = ent->rssi;
      req->ent.nwk_negotiated = 1;

      ZB_SCHEDULE_CALLBACK(zb_mlme_set_power_information_table_request, param);
  }
  else
  {
    zb_buf_free(param);
  }
}

void zb_nwk_lpd_joined_child(zb_uint8_t param)
{
  zb_mlme_set_power_info_tbl_req_t *req = ZB_BUF_GET_PARAM(param, zb_mlme_set_power_info_tbl_req_t);
  zb_uint16_t parent_addr;
  zb_ieee_addr_t parent_ieee;
  zb_neighbor_tbl_ent_t *ent;
  zb_ret_t ret;

  zb_address_short_by_ref(&parent_addr, ZG->nwk.handle.parent);
  ret = zb_nwk_neighbor_get(ZG->nwk.handle.parent, ZB_FALSE, &ent);
  ZB_ASSERT(ret == RET_OK);

  req->ent.short_addr = parent_addr;
  if (zb_address_ieee_by_short(parent_addr, parent_ieee) == RET_OK)
  {
    ZB_IEEE_ADDR_COPY(req->ent.ieee_addr, parent_ieee);
  }
  else
  {
    ZB_IEEE_ADDR_COPY(req->ent.ieee_addr, g_unknown_ieee_addr);
  }
  /* Again, spec violation - pass only power diff, not the absolute TX power */
  req->ent.tx_power = 0;
  req->ent.last_rssi = ent->rssi;
  req->ent.nwk_negotiated = 1;

  ZB_SCHEDULE_CALLBACK(zb_mlme_set_power_information_table_request, param);
}

void zb_mlme_get_power_information_table_confirm(zb_uint8_t param)
{
  zb_buf_free(param);
}

void zb_mlme_set_power_information_table_confirm(zb_uint8_t param)
{
  TRACE_MSG(TRACE_NWK1, ">> zb_mlme_get_power_information_table_confirm param %hd",
            (FMT__H, param));

#if defined ZB_ROUTER_ROLE
  if (ZB_IS_DEVICE_ZC_OR_ZR() &&
      ZG->nwk.handle.lpd_resp_addr != 0xFFFF)
  {
    TRACE_MSG(TRACE_NWK3, "Send LPD resp to 0x%x", (FMT__D, ZG->nwk.handle.lpd_resp_addr));

    prepare_lpd_unicast(param, ZG->nwk.handle.lpd_resp_addr);
    ZG->nwk.handle.lpd_resp_addr = 0xFFFF;

    /* Mark as internal buffer */
    zb_nwk_init_apsde_data_ind_params(param, ZB_NWK_INTERNAL_NSDU_HANDLE);

    ZB_SCHEDULE_CALLBACK(zb_nwk_forward, param);
  }
  else
#endif
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_NWK1, "<< zb_mlme_get_power_information_table_confirm", (FMT__0));
}




#endif /* ZB_MAC_POWER_CONTROL */
