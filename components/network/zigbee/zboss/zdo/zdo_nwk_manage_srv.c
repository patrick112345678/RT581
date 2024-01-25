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

#define ZB_TRACE_FILE_ID 20099
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_hash.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "../nwk/nwk_internal.h"   /* GP add */
#include "zdo_common.h"
#include "zb_secur.h"
#include "zb_address.h"
#include "zb_magic_macros.h"
#include "zdo_wwah_stubs.h"
/*! \addtogroup ZB_ZDO */
/*! @{ */
#ifndef ZB_COORDINATOR_ONLY
void zb_nwk_do_leave_local(zb_uint8_t param);
#endif


void zb_zdo_addr_resp_handle(zb_uint8_t param)
{
  zb_zdo_nwk_addr_resp_head_t *resp;
  zb_ieee_addr_t ieee_addr;
  zb_uint16_t nwk_addr;
  zb_address_ieee_ref_t addr_ref;
  zb_ret_t ret;

  TRACE_MSG(TRACE_ZDO2, ">> zb_zdo_addr_resp_handle param %hd", (FMT__H, param));

  /* Skip seq number.
     Do not cut it here, because it will be useful later when looking for registered callback */
  resp = (zb_zdo_nwk_addr_resp_head_t*)zb_buf_begin(param);
  TRACE_MSG(TRACE_ZDO2, "resp status %hd, nwk addr %d", (FMT__H_D, resp->status, resp->nwk_addr));
  ZB_DUMP_IEEE_ADDR(resp->ieee_addr);
  if (resp->status == (zb_uint8_t)ZB_ZDP_STATUS_SUCCESS)
  {
    ZB_LETOH64(ieee_addr, resp->ieee_addr);
    ZB_LETOH16((zb_uint8_t *)&nwk_addr, (zb_uint8_t *)&resp->nwk_addr);

    ret = zb_address_update(ieee_addr, nwk_addr, ZB_FALSE, &addr_ref);
    if (ret != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "zb_address_update failed [%d]", (FMT__D, ret));
    }
  }

  ret = zdo_af_resp(param);

  /* do not free buffer if callback was found */
  if (ret != RET_OK)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZDO2, "<< zb_zdo_addr_resp_handle", (FMT__0));
}


/* Mgmt NWK update handling functions were under ZB_ROUTER_ROLE ifdef,
 * but were switched on for all devices types (for WWAH, PICS item AZD514) */

static zb_ret_t zdo_nwk_update_req_process_new_active_channel(zb_channel_page_t channel_page)
{
  zb_ret_t ret;
  zb_uint8_t start_channel;
  zb_uint8_t max_channel;
  zb_uint8_t u8_channel_page = (zb_uint8_t)ZB_CHANNEL_PAGE_GET_PAGE(channel_page);

  TRACE_MSG(TRACE_ZDO2,
            ">> zdo_nwk_update_req_process_new_active_channel current_page %hd new_page %hd new_mask 0x%lx",
            (FMT__H_H_L, ZB_PIBCACHE_CURRENT_PAGE(),
             ZB_CHANNEL_PAGE_GET_PAGE(channel_page),
             ZB_CHANNEL_PAGE_GET_MASK(channel_page)));

  ret = zb_channel_page_get_start_channel_number(u8_channel_page, &start_channel);
  if (ret == RET_OK)
  {
    ret = zb_channel_page_get_max_channel_number(u8_channel_page, &max_channel);
  }

  /* Protect myself from PHY change */
  if ((ret == RET_OK) &&
      ((ZB_LOGICAL_PAGE_IS_2_4GHZ(ZB_PIBCACHE_CURRENT_PAGE())) == (ZB_CHANNEL_PAGE_IS_2_4GHZ(channel_page))) &&
      /* Only one channel should be set */
      MAGIC_IS_POWER_OF_TWO(ZB_CHANNEL_PAGE_GET_MASK(channel_page)) &&
      ZB_CHANNEL_PAGE_GET_MASK(channel_page) >= (1UL<<start_channel) &&
      ZB_CHANNEL_PAGE_GET_MASK(channel_page) <= (1UL<<max_channel))
  {
    zb_uint8_t i = start_channel;
    /*
      start nwkNetworkBroadcastDeliveryTime timer On timer
      expiration, change channel to the new value, increment
      NIB.UpdateId and reset counters
    */
    while ((ZB_CHANNEL_PAGE_GET_MASK(channel_page) & (1UL << i)) == 0U)
    {
      i++;
    }

    TRACE_MSG(TRACE_ZDO2, "new act ch %hd", (FMT__H, i));

    ZB_SCHEDULE_ALARM(zb_zdo_set_channel_cb, i,
                      ZB_NWK_OCTETS_TO_BI(ZB_NWK_BROADCAST_DELIVERY_TIME_OCTETS));

    ret = RET_OK;
  }

  TRACE_MSG(TRACE_ZDO2, "<< zdo_nwk_update_req_process_new_active_channel ret %d",
            (FMT__D, ret));

  return ret;
}


#ifndef ZB_LITE_NO_FULL_FUNCLIONAL_MGMT_NWK_UPDATE
static zb_ret_t zdo_nwk_update_req_process_new_channel_mask(zb_uint8_t count,
                                                            zb_channel_list_t channel_list,
                                                            zb_uint8_t update_id,
                                                            zb_uint16_t manager_addr)
{
  zb_ret_t ret = RET_ERROR;
  zb_uindex_t i;
  zb_channel_page_t channel_page;
  zb_uint8_t page;
  zb_uint32_t mask;

  TRACE_MSG(TRACE_ZDO3,
            ">> zdo_nwk_update_req_process_new_channel_mask count %hd, update_id %hd, manager_addr 0x%x",
            (FMT__H_H_D, count, update_id, manager_addr));

  if (update_id > ZB_NIB_UPDATE_ID() && (
#if !defined ZB_COORDINATOR_ONLY && defined ZB_DISTRIBUTED_SECURITY_ON
        zb_tc_is_distributed() ||
#endif  /* !ZB_COORDINATOR_ONLY */
       ZB_NIB_NWK_MANAGER_ADDR() == 0U) &&
      (count != 0U))
  {
    for (i = 0; i < count; i++)
    {
      channel_page = channel_list[i];

      page = (zb_uint8_t)ZB_CHANNEL_PAGE_GET_PAGE(channel_page);
      mask = ZB_CHANNEL_PAGE_GET_MASK(channel_page);

      /* Update only requested fields */
      TRACE_MSG(TRACE_ZDO3, "new AIB channel page %hd mask 0x%lx",
                (FMT__H_L, page, mask));


      zb_aib_channel_page_list_set_mask(ZB_CHANNEL_PAGE_TO_IDX(page), mask);
    }

    ZB_NIB_NWK_MANAGER_ADDR() = manager_addr;
    ZB_NIB_UPDATE_ID() = update_id;

#if defined ZB_USE_NVRAM
    /* If we fail, trace is given and assertion is triggered */
    ret = zb_nvram_write_dataset(ZB_NVRAM_COMMON_DATA);
#else
    ret = RET_OK;
#endif
  }

  TRACE_MSG(TRACE_ZDO3, "<< zdo_nwk_update_req_process_new_channel_mask ret %d",
            (FMT__D, ret));

  return ret;
}


static void zdo_ed_scan_completed_by_update_request(zb_uint8_t param)
{
  zb_zdo_mgmt_nwk_update_notify_param_t *update_notify_param_ptr;
  zb_zdo_mgmt_nwk_update_notify_param_t update_notify_param;
  zb_energy_detect_list_t *ed_list = ZB_BUF_GET_PARAM(param, zb_energy_detect_list_t);
  zb_uindex_t i;

  TRACE_MSG(TRACE_ZDO1, ">>zdo_ed_scan_completed_by_update_request param %hd",
            (FMT__H, param));

  ZB_BZERO(&update_notify_param, sizeof(zb_zdo_mgmt_nwk_update_notify_param_t));

  /* Sanity check */
  TRACE_MSG(TRACE_ZDO3, "status %hd channel_count %hd",
            (FMT__H_H, zb_buf_get_status(param), ed_list->channel_count));
  if (zb_buf_get_status(param) == (zb_ret_t)ZB_NWK_STATUS_SUCCESS &&
      ed_list->channel_count > ZB_MAC_SUPPORTED_CHANNELS)
  {
    zb_buf_set_status(param, ZB_NWK_STATUS_INVALID_REQUEST);
  }

  if (zb_buf_get_status(param) == (zb_ret_t)ZB_NWK_STATUS_SUCCESS)
  {
    for (i = 0; i < ed_list->channel_count; i++)
    {
      update_notify_param.energy_values[i] = ed_list->channel_info[i].energy_detected;
      update_notify_param.hdr.scanned_channels |= (1UL << ed_list->channel_info[i].channel_number);
    }

    update_notify_param.hdr.scanned_channels_list_count = ed_list->channel_count;
    ZB_CHANNEL_PAGE_SET_PAGE(update_notify_param.hdr.scanned_channels,
                             ZB_CHANNEL_PAGE_FROM_IDX((zb_uint32_t)ed_list->channel_info[0].channel_page_idx));
    update_notify_param.hdr.total_transmissions = ZB_NIB_NWK_TX_TOTAL();
    update_notify_param.hdr.transmission_failures = ZB_NIB_NWK_TX_FAIL();
  }

  update_notify_param.hdr.tsn = ZG->zdo.zdo_ctx.nwk_upd_req.tsn;
  update_notify_param.dst_addr = ZG->zdo.zdo_ctx.nwk_upd_req.dst_addr;
  update_notify_param.enhanced = ZG->zdo.zdo_ctx.nwk_upd_req.enhanced;
  update_notify_param.hdr.status = (zb_uint8_t)zb_buf_get_status(param);

  update_notify_param_ptr = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_nwk_update_notify_param_t);
  ZB_MEMCPY(update_notify_param_ptr, &update_notify_param,
            sizeof(zb_zdo_mgmt_nwk_update_notify_param_t));

  zb_zdo_nwk_upd_notify(param);

  TRACE_MSG(TRACE_ZDO1, "<<zdo_ed_scan_completed_by_update_request", (FMT__0));
}

static zb_ret_t zdo_nwk_update_req_process_channel_ed_scan(zb_uint8_t param,
                                                           zb_channel_page_t scan_channels,
                                                           zb_uint8_t scan_duration,
                                                           zb_uint8_t scan_count)
{
  zb_apsde_data_indication_t *ind;
  zb_ret_t ret = RET_ERROR;

  ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);

  if (!ZB_NWK_IS_ADDRESS_BROADCAST(ind->dst_addr))
  {
    if (scan_count <= ZB_ZDO_MAX_SCAN_COUNT_PARAM)
    {
      zb_zdo_ed_scan_param_t *req;
      zb_uint8_t tsn, page;
      zb_uint16_t dst_addr = ind->src_addr;

      tsn = *(zb_uint8_t*)zb_buf_begin(param);
      page = (zb_uint8_t)ZB_CHANNEL_PAGE_GET_PAGE(scan_channels);
      req = ZB_BUF_GET_PARAM(param, zb_zdo_ed_scan_param_t);

      req->cb = zdo_ed_scan_completed_by_update_request;
      req->nwk_param.scan_duration = scan_duration;

      zb_channel_list_init(req->nwk_param.scan_channels_list);
      zb_channel_page_list_set_mask(req->nwk_param.scan_channels_list,
                                    ZB_CHANNEL_PAGE_TO_IDX(page),
                                    ZB_CHANNEL_PAGE_GET_MASK(scan_channels));

      /* Save ctx */
      ZG->zdo.zdo_ctx.nwk_upd_req.dst_addr = dst_addr;
      ZG->zdo.zdo_ctx.nwk_upd_req.tsn = tsn;
      ZG->zdo.zdo_ctx.nwk_upd_req.enhanced = ZB_B2U(ZB_CHANNEL_PAGE_TO_IDX(page) != 0U);

      ZB_SCHEDULE_CALLBACK(zb_zdo_ed_scan_request, param);

      ret = RET_OK;
    }
  }

  return ret;
}
#endif /* ZB_LITE_NO_FULL_FUNCLIONAL_MGMT_NWK_UPDATE */


/* Handle nwk_update_req, 2.4.3.3.9 Mgmt_NWK_Update_req */
void zb_zdo_mgmt_nwk_update_handler(zb_uint8_t param)
{
  zb_apsde_data_indication_t *ind;
  zb_zdo_mgmt_nwk_update_req_hdr_t *req_hdr;
  zb_uint8_t *aps_body;
  zb_uint8_t tsn;
  zb_uint32_t scan_channels = 0;
  zb_ret_t ret = RET_OK;
  zb_bool_t send_reply = ZB_TRUE;
  zb_size_t expected_len;

  TRACE_MSG(TRACE_ZDO3, ">>mgmt_nwk_update_handler %hd", (FMT__H, param));

  aps_body = zb_buf_begin(param);
  ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);
  /*
    2.4.2.8 Transmission of ZDP Commands
   | Transaction sequence number (1byte) | Transaction data (variable) |
  */
  tsn = *aps_body;

  /* Casting from zb_zb_uint8_t * to packed struct zb_zdo_mgmt_nwk_update_req_hdr_t * via interim pointer to void.
   * There is no misalignment */
  aps_body++;
  /*cstat !MISRAC2012-Rule-11.3 */
  /** @mdr{00002,78} */
  req_hdr = (zb_zdo_mgmt_nwk_update_req_hdr_t*)aps_body;

  expected_len = sizeof(*req_hdr) + sizeof(tsn);
  if (req_hdr->scan_duration == ZB_ZDO_NEW_ACTIVE_CHANNEL)
  {
    expected_len += sizeof(zb_uint8_t); /* nwkUpdateId */
  }
  else if (req_hdr->scan_duration == ZB_ZDO_NEW_CHANNEL_MASK)
  {
    expected_len += sizeof(zb_uint8_t) + sizeof(zb_uint16_t);  /* nwkUpdateId + nwkManagerAddr*/
  }
  else if (req_hdr->scan_duration <= ZB_ZDO_MAX_SCAN_DURATION)
  {
    expected_len += sizeof(zb_uint8_t);
  }
  else
  {
    ret = RET_ERROR;
    TRACE_MSG(TRACE_ZDO3, "malformed zdo_mgmt_nwk_update: invalid scan duration", (FMT__0));
  }

  if (zb_buf_len(param) < expected_len)
  {
    ret = RET_ERROR;
    send_reply = ZB_FALSE;

    TRACE_MSG(TRACE_ZDO3, "malformed zdo_mgmt_nwk_update: the length is less than expected - drop packet", (FMT__0));
  }

  if (ret == RET_OK)
  {
  aps_body += sizeof(zb_zdo_mgmt_nwk_update_req_hdr_t);
  ZB_LETOH32((zb_uint8_t *)&scan_channels, (zb_uint8_t *)&req_hdr->scan_channels);
  TRACE_MSG(TRACE_ZDO2, "scan_duration %hx, scan_channels 0x%lx",
            (FMT__H_L, req_hdr->scan_duration, scan_channels));
  }


  if (ret == RET_OK && req_hdr->scan_duration == ZB_ZDO_NEW_ACTIVE_CHANNEL
  /*cstat !MISRAC2012-Rule-14.3_b */
  /** @mdr{00015,4} */
  /* See PendingNetworkUpdateChannel attribute. This attribute contains the channel number of
   * the only channel the device SHALL accept in a ZDO Mgmt Network Update command. */
      && !ZB_ZDO_CHECK_NEW_CHANNEL(scan_channels))
    /* TODO: Doubt - Add  remaining condition parameters to deviation description? */
    /*cstat !MISRAC2012-Rule-2.1_b */
    /** @mdr{00015,3} */
  {
    ret = RET_ERROR;
  }

  if (ret == RET_OK && !ZB_CHANNEL_PAGE_IS_2_4GHZ(scan_channels))
  {
    TRACE_MSG(TRACE_ERROR, "'%d' is not a 2.4GHz channel page",
              (FMT__L, ZB_CHANNEL_PAGE_GET_PAGE(scan_channels)));
    ret = RET_ERROR;
  }

  if (ret == RET_OK)
  {
    if (req_hdr->scan_duration == ZB_ZDO_NEW_ACTIVE_CHANNEL)
    {
      ret = zdo_nwk_update_req_process_new_active_channel(scan_channels);
    }
#ifndef ZB_LITE_NO_FULL_FUNCLIONAL_MGMT_NWK_UPDATE
    else if (req_hdr->scan_duration == ZB_ZDO_NEW_CHANNEL_MASK)
    {
      zb_uint8_t update_id;
      zb_uint16_t manager_addr;
      zb_channel_list_t channel_list;

      /* Prepare channel... */
      zb_channel_list_init(channel_list);
      zb_channel_page_list_set_2_4GHz_mask(channel_list,
                                           ZB_CHANNEL_PAGE_GET_MASK(scan_channels));

      /* update_id and manager_addr */
      update_id = *aps_body;
      aps_body++;
      ZB_LETOH16(&manager_addr, aps_body);

      ret = zdo_nwk_update_req_process_new_channel_mask(1, channel_list, update_id, manager_addr);
    }
    else if (req_hdr->scan_duration <= ZB_ZDO_MAX_SCAN_DURATION)
    {
      zb_uint8_t scan_count = *aps_body;

      ret = zdo_nwk_update_req_process_channel_ed_scan(param, scan_channels, req_hdr->scan_duration, scan_count);
      if (ret == RET_OK)
      {
        /* Mgmt_NWK_Update_notify will be sent after ed scan is completed */
        send_reply = ZB_FALSE;
        param = 0;
      }
    }
    else
    {
      ZB_ASSERT(ZB_FALSE);
    }
#endif  /* ZB_LITE_NO_FULL_FUNCLIONAL_MGMT_NWK_UPDATE */
  }

  if (ZB_NWK_IS_ADDRESS_BROADCAST(ind->dst_addr))
  {
    send_reply = ZB_FALSE;
  }

  if (send_reply)
  {
    zb_zdo_mgmt_nwk_update_notify_param_t *update_notify_param;
    zb_uint16_t dst_addr = ind->src_addr;

    update_notify_param = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_nwk_update_notify_param_t);
    ZB_BZERO(update_notify_param, sizeof(zb_zdo_mgmt_nwk_update_notify_param_t));

    update_notify_param->hdr.status = (zb_uint8_t)((ret == RET_OK) ? (ZB_ZDP_STATUS_SUCCESS)
      : (ZB_ZDP_STATUS_INV_REQUESTTYPE));
    update_notify_param->hdr.tsn = tsn;
    update_notify_param->dst_addr = dst_addr;

    zb_zdo_nwk_upd_notify(param);
  }
  else
  {
    if (param != 0U)
    {
      zb_buf_free(param);
    }
  }

  TRACE_MSG(TRACE_ZDO3, "<<mgmt_nwk_update_handler", (FMT__0));
}


#ifdef ZB_MGMT_NWK_ENHANCED_UPDATE_ENABLED

void zb_zdo_mgmt_nwk_enhanced_update_handler(zb_uint8_t param)
{
  zb_apsde_data_indication_t *ind;
  zb_zdo_mgmt_nwk_enhanced_update_req_hdr_t *req_hdr;
  zb_uint8_t *aps_body;
  zb_uint8_t tsn;
  zb_channel_page_t channel_page;
  zb_uint8_t scan_duration;
  zb_size_t expected_len;
  zb_ret_t ret = RET_ERROR;

  TRACE_MSG(TRACE_ZDO3, ">>zb_zdo_mgmt_nwk_enhanced_update_handler %hd", (FMT__H, param));

  aps_body = zb_buf_begin(param);
  ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);
  /*
    2.4.2.8 Transmission of ZDP Commands
   | Transaction sequence number (1byte) | Transaction data (variable) |
  */
  tsn = *aps_body;
  aps_body++;
  /*cstat !MISRAC2012-Rule-11.3 */
  /** @mdr{00002,79} */
  req_hdr = (zb_zdo_mgmt_nwk_enhanced_update_req_hdr_t *)aps_body;
  aps_body += sizeof(zb_zdo_mgmt_nwk_enhanced_update_req_hdr_t);

  TRACE_MSG(TRACE_ZDO2, "channel_count %hd",
            (FMT__H, req_hdr->channel_page_count));

  expected_len = sizeof(zb_zdo_mgmt_nwk_enhanced_update_req_hdr_t) + /* Header */
    req_hdr->channel_page_count * sizeof(zb_channel_page_t) + /*  Channel list */
    sizeof(zb_uint8_t); /* Scan duration */

  if (req_hdr->channel_page_count == 0U)
  {
    TRACE_MSG(TRACE_ERROR, "error - Channel list is empty", (FMT__0));
  }
  else if (req_hdr->channel_page_count > ZB_CHANNEL_PAGES_MAX_NUM)
  {
    TRACE_MSG(TRACE_ERROR, "Too many channel pages - malformed packet", (FMT__0));
  }
  /* Check if packet is malformed - check only mandatory fields */
  else if (zb_buf_len(param) < expected_len)
  {
    TRACE_MSG(TRACE_ERROR, "Too short packet - malformed packet", (FMT__0));
    zb_buf_free(param);
    param = 0;
  }
  else
  {
    aps_body += req_hdr->channel_page_count * sizeof(zb_channel_page_t);
    scan_duration = *aps_body;
    aps_body++;

    if (scan_duration == ZB_ZDO_NEW_ACTIVE_CHANNEL)
    {
      expected_len += sizeof(zb_uint8_t); /* nwkUpdateId */
    }
    else if (scan_duration == ZB_ZDO_NEW_CHANNEL_MASK)
    {
      expected_len += 2U * sizeof(zb_uint8_t); /* nwkUpdateId and nwkManagerAddr */
    }
    else
    {
      /* MISRA rule 15.7 requires empty 'else' branch. */
    }

    if (zb_buf_len(param) < expected_len)
    {
      TRACE_MSG(TRACE_ERROR, "necessary fields missing - malformed packet", (FMT__0));
      zb_buf_free(param);
      param = 0;
    }
    else if (scan_duration == ZB_ZDO_NEW_ACTIVE_CHANNEL &&
             req_hdr->channel_page_count == 1U)
    {
      /* Channel page is just after the header */
      ZB_LETOH32(&channel_page, (req_hdr + 1));

      /* See PendingNetworkUpdateChannel attribute. This attribute contains the channel number of
       * the only channel the device SHALL accept in a Mgmt Network Enhanced Update command. */
      /*cstat !MISRAC2012-Rule-14.3_b */
      /** @mdr{00015,6} */
      if (!ZB_ZDO_CHECK_NEW_CHANNEL(channel_page))
      /*cstat !MISRAC2012-Rule-2.1_b */
      /** @mdr{00015,5} */
      {
          zb_buf_free(param);
          param = 0;
      }
      else
      {
        ret = zdo_nwk_update_req_process_new_active_channel(channel_page);
      }
    }
#ifndef ZB_LITE_NO_FULL_FUNCLIONAL_MGMT_NWK_UPDATE
    else if (scan_duration == ZB_ZDO_NEW_CHANNEL_MASK)
    {
      zb_uint8_t update_id;
      zb_uint16_t manager_addr;
      zb_channel_list_t channel_list;
      zb_channel_page_t *src_channel_page;
      zb_channel_page_t *dst_channel_page;
      zb_uindex_t i = 0;
      /* It's OK if the list is not initialized,
       * the usage is slightly different */
      /*cstat !MISRAC2012-Rule-11.3 */
      /** @mdr{00002,80} */
      src_channel_page = (zb_channel_page_t *)(req_hdr+1);
      dst_channel_page = (zb_channel_page_t *)&channel_list[0];
      while (i < req_hdr->channel_page_count)
      {
        ZB_MEMCPY(dst_channel_page, src_channel_page, sizeof(zb_channel_page_t));
        ZB_LETOH32_ONPLACE(dst_channel_page);
        i++;
      }

      /* update_id and manager_addr */
      update_id = *aps_body;
      aps_body++;
      ZB_LETOH16(&manager_addr, aps_body);

      ret = zdo_nwk_update_req_process_new_channel_mask(req_hdr->channel_page_count, channel_list,
                                                        update_id, manager_addr);
    }
    /* c) If the request is a Mgmt_NWK_Enhanced_Update_Request and the
     * ScanCHannelsListStructure includes more than one page, do the following:
     *
     * i) Follow the Error Response procedure setting the status to Invalid_request
     * ii) The request shall be dropped blah-blah-blah...
     *
     */
    else if (scan_duration <= ZB_ZDO_MAX_SCAN_DURATION &&
             req_hdr->channel_page_count == 1U)
    {
      zb_uint8_t scan_count = *aps_body;

      /* ZB_LETOH32 can handle unaligned pointers */
      ZB_LETOH32(&channel_page, (req_hdr + 1));
      ret = zdo_nwk_update_req_process_channel_ed_scan(param, channel_page, scan_duration, scan_count);
      if (ret == RET_OK)
      {
        param = 0;
      }
    }
#endif  /* ZB_LITE_NO_FULL_FUNCLIONAL_MGMT_NWK_UPDATE */
  }

  if ((param != 0U) && !ZB_NWK_IS_ADDRESS_BROADCAST(ind->dst_addr))
  {
    zb_zdo_mgmt_nwk_update_notify_param_t *update_notify_param;
    zb_uint16_t dst_addr = ind->src_addr;

    update_notify_param = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_nwk_update_notify_param_t);
    ZB_BZERO(update_notify_param, sizeof(zb_zdo_mgmt_nwk_update_notify_param_t));

    update_notify_param->hdr.status = (zb_uint8_t)((ret == RET_OK) ? (ZB_ZDP_STATUS_SUCCESS)
      : (ZB_ZDP_STATUS_INV_REQUESTTYPE));
    update_notify_param->hdr.tsn = tsn;
    update_notify_param->dst_addr = dst_addr;
    update_notify_param->enhanced = 1;

    zb_zdo_nwk_upd_notify(param);

    param = 0;
  }

  if (param != 0U)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZDO3, "<<mgmt_nwk_update_handler", (FMT__0));
}

#endif /* ZB_MGMT_NWK_ENHANCED_UPDATE_ENABLED */

/* sends 2.4.4.3.9 Mgmt_NWK_Update_notify */
void zb_zdo_nwk_upd_notify(zb_uint8_t param)
{
  zb_zdo_mgmt_nwk_update_notify_param_t notify_param;
  zb_zdo_mgmt_nwk_update_notify_hdr_t *notify_resp;
  zb_uint8_t *ed_scan_values;
  zb_uint16_t cluster_id;

  TRACE_MSG(TRACE_ZDO3, ">>nwk_upd_notify %hd", (FMT__H, param));

  ZB_MEMCPY(&notify_param, ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_nwk_update_notify_param_t), sizeof(zb_zdo_mgmt_nwk_update_notify_param_t));

  notify_resp = zb_buf_initial_alloc(param, sizeof(zb_zdo_mgmt_nwk_update_notify_hdr_t));

  ZB_MEMCPY(notify_resp, &notify_param.hdr, sizeof(zb_zdo_mgmt_nwk_update_notify_hdr_t));
  ZB_HTOLE32_ONPLACE(notify_resp->scanned_channels);

  if (notify_resp->scanned_channels_list_count != 0U)
  {
    /* Assert just to be sure that we have enough space for response */
    ZB_ASSERT((ZB_APS_PAYLOAD_MAX_LEN - sizeof(zb_zdo_mgmt_nwk_update_notify_hdr_t)) > notify_resp->scanned_channels_list_count);

    ed_scan_values = zb_buf_alloc_right(param, notify_resp->scanned_channels_list_count);
    ZB_MEMCPY(ed_scan_values, notify_param.energy_values, notify_resp->scanned_channels_list_count);
    TRACE_MSG(TRACE_ZDO3, "ch count %hd, buf len %hd",
              (FMT__H_H, notify_resp->scanned_channels_list_count, zb_buf_len(param)));
  }

  TRACE_MSG(TRACE_ZDO3, "total tr %hd, tr fail %hd, ack %hd",
    (FMT__H_H_H, notify_resp->total_transmissions, notify_resp->transmission_failures, ZB_ZDO_GET_SEND_WITH_ACK()));
  ZB_HTOLE16_ONPLACE(notify_resp->total_transmissions);
  ZB_HTOLE16_ONPLACE(notify_resp->transmission_failures);

  cluster_id = (ZB_U2B(notify_param.enhanced)) ? (ZDO_MGMT_NWK_ENHANCED_UPDATE_NOTIFY_CLID)
    : (ZDO_MGMT_NWK_UPDATE_NOTIFY_CLID);
  zdo_send_resp_by_short(cluster_id, param, notify_param.dst_addr);

  TRACE_MSG(TRACE_ATM1, "Z< send mgmt_nwk_update_notify, stauts = 0x%x", (FMT__H, notify_resp->status));
  TRACE_MSG(TRACE_ZDO3, "<<nwk_upd_notify", (FMT__0));
}


void zb_zdo_mgmt_handle_unsol_nwk_update_notify(zb_uint8_t param)
{
  zb_bool_t free_buf = ZB_TRUE;
  zb_apsde_data_indication_t *ind;

  TRACE_MSG(TRACE_ZDO1, ">> zb_zdo_mgmt_handle_unsol_nwk_update_notify param %hd",
            (FMT__H, param));

  ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);

  if (ZB_NWK_IS_ADDRESS_BROADCAST(ind->dst_addr))
  {
    TRACE_MSG(TRACE_ERROR, "Broadcast command is not allowed, drop frame", (FMT__0));
  }
  else if (ind->dst_addr == ZB_PIBCACHE_NETWORK_ADDRESS() &&
           ZB_NIB_NWK_MANAGER_ADDR() == ZB_PIBCACHE_NETWORK_ADDRESS())
  {
    /* This is not a check for role, but just a size optimization. */
#ifndef ZB_ED_ROLE
    zb_zdo_mgmt_nwk_update_notify_hdr_t *update_notify_ptr;
    zb_zdo_mgmt_nwk_update_notify_hdr_t update_notify;

    update_notify_ptr = (zb_zdo_mgmt_nwk_update_notify_hdr_t *)zb_buf_begin(param);

    ZB_MEMCPY(&update_notify, update_notify_ptr,
              sizeof(zb_zdo_mgmt_nwk_update_notify_hdr_t));
    ZB_LETOH32_ONPLACE(update_notify.scanned_channels);
    ZB_LETOH16_ONPLACE(update_notify.total_transmissions);
    ZB_LETOH16_ONPLACE(update_notify.transmission_failures);

    TRACE_MSG(TRACE_ZDO3, "status %hd, scanned_channels 0x%lx, total_tx %d, failed_tx %d, scanned_count %hd",
              (FMT__H_L_D_D_H, update_notify.status, update_notify.scanned_channels,
               update_notify.total_transmissions, update_notify.transmission_failures,
                update_notify.scanned_channels_list_count));

    if (update_notify.status == (zb_uint8_t)ZB_ZDP_STATUS_SUCCESS &&
        (update_notify.scanned_channels_list_count != 0U) &&
        (ZB_CHANNEL_PAGE_IS_2_4GHZ(update_notify.scanned_channels) &&
        (ZB_CHANNEL_PAGE_GET_PAGE(update_notify.scanned_channels) == ZB_PIBCACHE_CURRENT_PAGE()) &&
        (ZB_CHANNEL_PAGE_GET_MASK(update_notify.scanned_channels) & (1UL << ZB_PIBCACHE_CURRENT_CHANNEL())) != 0U))
    {
      zb_uindex_t i;
      zb_uint8_t current_energy = 0;

      for (i = 0; i < update_notify.scanned_channels_list_count; i++)
      {
        if ((ZB_CHANNEL_PAGE_GET_MASK(update_notify.scanned_channels) &
            (1UL << ZB_PIBCACHE_CURRENT_CHANNEL())) != 0U)
        {
          current_energy = *((zb_uint8_t *)(update_notify_ptr + 1) + i);
          break;
        }
      }

      TRACE_MSG(TRACE_ZDO3, "Current energy level %hd", (FMT__H, current_energy));
      if (current_energy > ZB_CHANNEL_BUSY_ED_VALUE &&
          !ZB_U2B(ZB_ZDO_GET_CHECK_FAILS()))
      {
        if (!ZB_ZDO_NETWORK_MGMT_CHANNEL_UPDATE_IS_DISABLED())
        {
          ZB_ZDO_SET_CHECK_FAILS();
          ZB_SCHEDULE_CALLBACK(zb_zdo_check_channel_conditions, param);
          free_buf = ZB_FALSE;
        }
        else
        {
          TRACE_MSG(TRACE_ZDO3, "Network update is not permitted", (FMT__0));
        }
      }
    }
#else
    TRACE_MSG(TRACE_ERROR, "End device is NWK manager here!", (FMT__0));
#endif /* ZB_ED_ROLE */
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "I'm not a NWK manager, drop the frame", (FMT__0));
  }

  if (free_buf)
  {
    zb_buf_free(param);
  }


  TRACE_MSG(TRACE_ZDO1, "<< zb_zdo_mgmt_handle_unsol_nwk_update_notify", (FMT__0));
}

#ifdef ZB_MGMT_NWK_ENHANCED_UPDATE_ENABLED

void zb_zdo_mgmt_unsol_enh_nwk_update_notify_handler(zb_uint8_t param)
{
  zb_bool_t free_buf = ZB_FALSE;
  zb_apsde_data_indication_t *ind;

  TRACE_MSG(TRACE_ZDO1, ">> zb_zdo_mgmt_unsol_enh_nwk_update_notify_handler param %hd",
            (FMT__H, param));

  ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);

  if (zb_buf_len(param) < 1U + sizeof(zb_zdo_mgmt_nwk_unsol_enh_update_notify_t)) /*  */
  {
    TRACE_MSG(TRACE_ERROR, "Drop malformed frame: expected len %hd, actual len %hd",
              (FMT__H_H, sizeof(zb_zdo_mgmt_nwk_unsol_enh_update_notify_t), zb_buf_len(param) - 1));
    free_buf = ZB_TRUE;
  }

  if (!free_buf &&
      ZB_NWK_IS_ADDRESS_BROADCAST(ind->dst_addr))
  {
    TRACE_MSG(TRACE_ERROR, "Broadcast command is not allowed, drop frame", (FMT__0));
    free_buf = ZB_TRUE;
  }

  if (!free_buf &&
      ind->dst_addr == ZB_PIBCACHE_NETWORK_ADDRESS() &&
      ZB_NIB_NWK_MANAGER_ADDR() == ZB_PIBCACHE_NETWORK_ADDRESS())
  {
    /* This is not a check for role, but just a size optimization. */
#ifndef ZB_ED_ROLE
    zb_uint8_t *aps_body;
    zb_zdo_mgmt_nwk_unsol_enh_update_notify_t *notify_ptr;
    zb_zdo_mgmt_nwk_unsol_enh_update_notify_t notification;

    aps_body = zb_buf_begin(param);

    /* "aps_body" holds TSN, "aps_body+1" holds ZDO payload */
    aps_body++;
    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,81} */
    notify_ptr = (zb_zdo_mgmt_nwk_unsol_enh_update_notify_t *)aps_body;
    ZB_MEMCPY(&notification, notify_ptr, sizeof(zb_zdo_mgmt_nwk_unsol_enh_update_notify_t));
    ZB_LETOH32_ONPLACE(notification.channel_in_use);
    ZB_LETOH16_ONPLACE(notification.mac_tx_ucast_total);
    ZB_LETOH16_ONPLACE(notification.mac_tx_ucast_failures);
    ZB_LETOH16_ONPLACE(notification.mac_tx_ucast_retries);

    TRACE_MSG(TRACE_ZDO3,
              "Device 0x%x complains, status: %hd, channel_page %lx, channel_mask 0x%lx, tx_total %d, tx_failures %d, tx_retries %d",
              (FMT__D_H_L_L_D_D_D, ind->src_addr,
               notification.status,
               ZB_CHANNEL_PAGE_GET_PAGE(notification.channel_in_use),
               ZB_CHANNEL_PAGE_GET_MASK(notification.channel_in_use),
               notification.mac_tx_ucast_total,
               notification.mac_tx_ucast_failures,
               notification.mac_tx_ucast_retries));

    TRACE_MSG(TRACE_ZDO3, "my config: page %hd, mask 0x%lx",
              (FMT__H_L, ZB_PIBCACHE_CURRENT_PAGE(), (1L << ZB_PIBCACHE_CURRENT_CHANNEL())));

    if (notification.status == (zb_uint8_t)ZB_ZDP_STATUS_SUCCESS &&
        (ZB_CHANNEL_PAGE_GET_PAGE(notification.channel_in_use) == ZB_PIBCACHE_CURRENT_PAGE()) &&
        ZB_CHANNEL_PAGE_GET_MASK(notification.channel_in_use) == (1UL << ZB_PIBCACHE_CURRENT_CHANNEL()))
    {
      /* Maybe, implement more complex vendor-specific analysis? =) */
      if (!ZB_U2B(ZB_ZDO_GET_CHECK_FAILS()))
      {
        if (!ZB_ZDO_NETWORK_MGMT_CHANNEL_UPDATE_IS_DISABLED())
        {
          ZB_ZDO_SET_CHECK_FAILS();
          ZB_SCHEDULE_CALLBACK(zb_zdo_check_channel_conditions, param);
        }
        else
        {
          TRACE_MSG(TRACE_ZDO1, "Network update is not permitted", (FMT__0));
          free_buf = ZB_TRUE;
        }
      }
      else
      {
        TRACE_MSG(TRACE_ERROR, "Channel check is already in progress", (FMT__0));
        free_buf = ZB_TRUE;
      }
    }
    else
    {
      TRACE_MSG(TRACE_ERROR, "Different channel in use, drop the frame", (FMT__0));
      free_buf = ZB_TRUE;
    }
#else
    TRACE_MSG(TRACE_ERROR, "End device is NWK manager here!", (FMT__0));
#endif /* ZB_ED_ROLE */
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "I'm not a NWK manager, drop the frame", (FMT__0));
    free_buf = ZB_TRUE;
  }

  if (free_buf)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZDO1, "<< zb_zdo_mgmt_unsol_enh_nwk_update_notify_handler", (FMT__0));
}


void zb_zdo_mgmt_nwk_unsol_enh_update_notify(zb_uint8_t param,
                                             zb_callback_t cb)
{
  zb_zdo_mgmt_nwk_unsol_enh_update_notify_t *req;
  zb_zdo_mgmt_nwk_unsol_enh_update_notify_param_t req_param;
  zb_uint8_t *ptr;

  TRACE_MSG(TRACE_ZDO1, ">> zb_zdo_mgmt_nwk_unsol_enh_update_notify param %hd",
            (FMT__H, param));

  ptr = (zb_uint8_t *)ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_nwk_unsol_enh_update_notify_param_t);
  ZB_MEMCPY(&req_param, ptr, sizeof(zb_zdo_mgmt_nwk_unsol_enh_update_notify_param_t));

  req = zb_buf_initial_alloc(param, sizeof(zb_zdo_mgmt_nwk_unsol_enh_update_notify_t));

  ZB_MEMCPY(req, &req_param.notification, sizeof(zb_zdo_mgmt_nwk_unsol_enh_update_notify_t));
  ZB_HTOLE32_ONPLACE(req->channel_in_use);
  ZB_HTOLE16_ONPLACE(req->mac_tx_ucast_total);
  ZB_HTOLE16_ONPLACE(req->mac_tx_ucast_failures);
  ZB_HTOLE16_ONPLACE(req->mac_tx_ucast_retries);

  zb_buf_flags_or(param, ZB_BUF_ZDO_CMD_NO_RESP);

  (void)zdo_send_req_by_short(ZDO_MGMT_NWK_UNSOLICITED_ENHANCED_UPDATE_NOTIFY_CLID ,
                              param, cb, req_param.addr, ZB_ZDO_CB_DEFAULT_COUNTER);

  TRACE_MSG(TRACE_ZDO1, "<< zb_zdo_mgmt_nwk_unsol_enh_update_notify", (FMT__0));
}

#endif /* ZB_MGMT_NWK_ENHANCED_UPDATE_ENABLED */


#if OBSOLETE
void zb_start_ed_scan(zb_uint8_t param)
{
  zb_nlme_ed_scan_request_t *rq;

  TRACE_MSG(TRACE_ZDO3, "zb_start_ed_scan, param %d", (FMT__D, param));
  rq = ZB_BUF_GET_PARAM(param, zb_nlme_ed_scan_request_t);

  ZG->zdo.zdo_ctx.nwk_upd_req.scan_count--;

  zb_channel_list_init(rq->scan_channels_list);
  zb_channel_page_list_set_mask(rq->scan_channels_list,
                                ZB_CHANNEL_PAGE_GET_PAGE(ZG->zdo.zdo_ctx.nwk_upd_req.scan_channels),
                                ZG->zdo.zdo_ctx.nwk_upd_req.scan_channels);/* MMDEVSTUBS */
  rq->scan_duration = ZG->zdo.zdo_ctx.nwk_upd_req.scan_duration;

  ZB_SCHEDULE_CALLBACK(zb_nlme_ed_scan_request, param);
}
#endif  /* OBSOLETE */

static void zb_zdo_set_channel(zb_uint8_t param);
static void zb_zdo_set_channel_cont(zb_uint8_t param);

void zb_zdo_set_channel_cb(zb_uint8_t channel)
{
  ZB_NIB_UPDATE_ID()++;
  zb_zdo_do_set_channel(channel);
}

void zb_zdo_do_set_channel(zb_uint8_t channel)
{
  zb_ret_t ret;

  /* Upon receipt of a Mgmt_NWK_Update_req with a change of channels,
   * change channel to the new value, increment NIB.UpdateId and reset
   * counters */
  TRACE_MSG(TRACE_ZDO2, "new_channel_cb ch %hd", (FMT__H, channel));

  nwk_txstat_clear();

  /* change channel co channel # channel */
  ZB_PIBCACHE_CURRENT_CHANNEL() = channel;

  ret = zb_buf_get_out_delayed(zb_zdo_set_channel);
  if (ret != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
  }
}

static void zb_zdo_set_channel(zb_uint8_t param)
{
  zb_uint8_t channel;

  TRACE_MSG(TRACE_ZDO2, ">>zb_zdo_set_channel param %hd", (FMT__H, param));

  ZB_ASSERT(param);

  /* Assumed to work with current page only! */

  /* change channel co channel # channel */
  channel = ZB_PIBCACHE_CURRENT_CHANNEL();

  zb_nwk_pib_set(param, ZB_PHY_PIB_CURRENT_CHANNEL, &channel, 1, zb_zdo_set_channel_cont);

  TRACE_MSG(TRACE_ZDO2, "<<zb_zdo_set_channel", (FMT__0));
}

static void zb_zdo_set_channel_cont(zb_uint8_t param)
{
  zb_mlme_set_confirm_t     *cfm;
  zb_uint8_t                 status;

  TRACE_MSG(TRACE_ZDO2, "zb_zdo_set_channel_cont param %hd", (FMT__H, param));

  /* AT: [ZLL, frequency agility] function zb_zdo_set_channel_cb is called from zb_zdo_mgmt_nwk_update_req after
  ZB_NWK_BROADCAST_DELIVERY_TIME timeout, but in case when our device is a ED(controller), we should
  wait for confirm from parent and change channel only if transmission was sucessful
  */
#ifdef ZB_USE_NVRAM
  /* AT: new channel value should be stored in local storage */
  /* If we fail, trace is given and assertion is triggered */
  (void)zb_nvram_write_dataset(ZB_NVRAM_COMMON_DATA);
#endif

  ZB_ASSERT(param);


  if (ZG->zdo.set_channel_confirm_cb != NULL)
  {
    TRACE_MSG(TRACE_ZDO2, "scheduling confirm", (FMT__0));

    cfm = (zb_mlme_set_confirm_t *)zb_buf_begin(param);

    if (cfm->status == MAC_SUCCESS)
    {
      status = ZB_ZDP_STATUS_SUCCESS;
    }
    else
    {
      status = ZB_ZDP_STATUS_NOT_PERMITTED;
    }

    ZB_SCHEDULE_CALLBACK(ZG->zdo.set_channel_confirm_cb, status);
  }
  else
  {
    TRACE_MSG(TRACE_ZDO2, "no confirm to schedule", (FMT__0));
  }

  zb_buf_free(param);

  TRACE_MSG(TRACE_ZDO2, "<<zb_zdo_set_channel_cont", (FMT__0));
}

void zb_zdo_register_set_channel_confirm_cb(zb_zdo_set_channel_confirm_cb_t cb)
{
  ZG->zdo.set_channel_confirm_cb = cb;
}

void zb_zdo_get_channel_cont(zb_uint8_t param);

void zb_zdo_get_channel(zb_uint8_t param)
{
  zb_zdo_get_channel_req_t *req = ZB_BUF_GET_PARAM(param, zb_zdo_get_channel_req_t);

  ZVUNUSED(req);

  TRACE_MSG(TRACE_ZDO3, ">>zb_zdo_get_channel param %hd cb %p", (FMT__H_P, param, req->cb));
  zb_nwk_pib_get(param, ZB_PHY_PIB_CURRENT_CHANNEL, zb_zdo_get_channel_cont);
  TRACE_MSG(TRACE_ZDO3, "<<zb_zdo_get_channel", (FMT__0));
}

void zb_zdo_get_channel_cont(zb_uint8_t param)
{
  zb_zdo_get_channel_req_t *req = ZB_BUF_GET_PARAM(param, zb_zdo_get_channel_req_t);
  zb_zdo_get_channel_resp_t *resp = ZB_BUF_GET_PARAM(param, zb_zdo_get_channel_resp_t);
  zb_mlme_get_confirm_t *cfm = (zb_mlme_get_confirm_t *)zb_buf_begin(param);
  zb_callback_t cb;

  TRACE_MSG(TRACE_ZDO3, ">>zb_zdo_get_channel_cont param %hd", (FMT__H, param));
  ZB_ASSERT(cfm->pib_attr == ZB_PHY_PIB_CURRENT_CHANNEL);

  cb = req->cb;

  if (cfm->status == MAC_SUCCESS)
  {
    resp->status = ZB_ZDP_STATUS_SUCCESS;
    resp->channel = *(zb_uint8_t *)(cfm+1);
  }
  else
  {
    resp->status = ZB_ZDP_STATUS_NOT_PERMITTED;
  }

  ZB_SCHEDULE_CALLBACK(cb, param);
  TRACE_MSG(TRACE_ZDO3, "<<zb_zdo_get_channel_cont", (FMT__0));
}


#ifndef ZB_LITE_NO_ZDO_SYSTEM_SERVER_DISCOVERY
void zdo_system_server_discovery_res(zb_uint8_t param)
{
  zb_uint8_t *aps_body;
  zb_uint8_t tsn;
  zb_uint16_t server_mask;
  zb_zdo_system_server_discovery_resp_t *resp;
  zb_apsde_data_indication_t *ind;

  TRACE_MSG(TRACE_ZDO3, ">>zdo_system_server_discovery_res %hd", (FMT__H, param));

  aps_body = zb_buf_begin(param);
  ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);
  tsn = *aps_body;
  aps_body++;

  ZB_LETOH16(&server_mask, aps_body);

  TRACE_MSG(TRACE_ZDO3, "param server_mask %x, desc server_mask %x",
            (FMT__D_D, server_mask, ZB_ZDO_NODE_DESC()->server_mask));
  server_mask &= ZB_ZDO_NODE_DESC()->server_mask;
  if (server_mask != 0U)
  {
    TRACE_MSG(TRACE_ZDO3, "send response mask %x", (FMT__D, server_mask));
    resp = zb_buf_initial_alloc(param, sizeof(zb_zdo_system_server_discovery_resp_t));
    resp->tsn = tsn;
    resp->status = ZB_ZDP_STATUS_SUCCESS;
    ZB_HTOLE16((zb_uint8_t *)&resp->server_mask, (zb_uint8_t *)&server_mask);
    TRACE_MSG(TRACE_ZDO3, "send response addr %x", (FMT__D, ind->src_addr));
    zdo_send_resp_by_short(ZDO_SYSTEM_SERVER_DISCOVERY_RESP_CLID, param, ind->src_addr);
  }
  else
  {
    zb_buf_free(param);
  }
  TRACE_MSG(TRACE_ZDO3, "<<zdo_system_server_discovery_res", (FMT__0));
  TRACE_MSG(TRACE_ATM1, "Z< zdo_system_server_discovery_res server_mask 0x%x, status 0x%x", (FMT__D_D, resp->server_mask, resp->status));
}
#endif

void zdo_lqi_resp(zb_uint8_t param)
{
  zb_uint8_t *aps_body;
  zb_uint8_t tsn;
  zb_uint16_t dst_addr;
  zb_uint8_t start_index;
  zb_zdo_mgmt_lqi_resp_t *resp;
  zb_apsde_data_indication_t *ind;
  zb_zdo_mgmt_lqi_req_t *req;
  zb_ushort_t i;
  zb_zdo_neighbor_table_record_t *record;
  zb_size_t max_records_num;
  zb_uint8_t records_num;
  zb_uint8_t current_index = 0;

  TRACE_MSG(TRACE_ZDO3, ">>zdo_lqi_resp %hd", (FMT__H, param));

  if (zb_buf_len(param) < (sizeof(*req) + sizeof(tsn)))
  {
    TRACE_MSG(TRACE_ZDO3, "malformed zdo_lqi_req - drop packet", (FMT__0));
    zb_buf_free(param);
    return;
  }

  aps_body = zb_buf_begin(param);
  ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);
  dst_addr = ind->src_addr;
  tsn = *aps_body;

  aps_body++;
  /*cstat !MISRAC2012-Rule-11.3 */
  /** @mdr{00002,82} */
  req = (zb_zdo_mgmt_lqi_req_t*)aps_body;

  max_records_num = (ZB_ZDO_MAX_PAYLOAD_SIZE - sizeof(zb_zdo_mgmt_lqi_resp_t)) / sizeof(zb_zdo_neighbor_table_record_t);

  records_num = (zb_nwk_neighbor_table_size() > req->start_index) ?
                (zb_uint8_t)(zb_nwk_neighbor_table_size() - req->start_index) : 0U;
  TRACE_MSG(TRACE_ZDO3, "max rec %hd, total %hd, start indx %hd",
            (FMT__H_H_H, max_records_num, zb_nwk_neighbor_table_size(), req->start_index));

  records_num = (zb_uint8_t)((records_num < max_records_num) ? records_num : max_records_num);
  start_index = req->start_index;

  resp = zb_buf_initial_alloc(param, sizeof(zb_zdo_mgmt_lqi_resp_t) + records_num * sizeof(zb_zdo_neighbor_table_record_t));

  resp->tsn = tsn;
  resp->status = ZB_ZDP_STATUS_SUCCESS;
  resp->neighbor_table_entries = zb_nwk_neighbor_table_size();

  resp->start_index = start_index;
  resp->neighbor_table_list_count = records_num;

  /*cstat !MISRAC2012-Rule-11.3 */
  /** @mdr{00002,83} */
  record = (zb_zdo_neighbor_table_record_t*)(resp+1);

  TRACE_MSG(TRACE_ZDO3, "will add records %hd", (FMT__H, records_num));
  /* FIXME:TODO: rewrite excluding direct access to neighbor arrays from here! */
  for (i = 0; i < ZB_NEIGHBOR_TABLE_SIZE && (records_num != 0U); ++i)
  {
    if (ZB_U2B(ZG->nwk.neighbor.neighbor[i].used))
    {
      if (current_index>=start_index)
      {
        zb_uint16_t addr;
        ZB_MEMCPY(record->ext_pan_id, ZB_NIB_EXT_PAN_ID(), sizeof(zb_ext_pan_id_t));
        zb_address_by_ref(record->ext_addr, &addr, ZG->nwk.neighbor.neighbor[i].u.base.addr_ref);
        record->network_addr = addr;

        ZB_ZDO_RECORD_SET_DEVICE_TYPE(record->type_flags, ZG->nwk.neighbor.neighbor[i].device_type);
        ZB_ZDO_RECORD_SET_RX_ON_WHEN_IDLE(record->type_flags, ZG->nwk.neighbor.neighbor[i].rx_on_when_idle);
        ZB_ZDO_RECORD_SET_RELATIONSHIP(record->type_flags, ZG->nwk.neighbor.neighbor[i].relationship);
        record->permit_join = ZG->nwk.neighbor.neighbor[i].permit_joining;
        record->depth = ZG->nwk.neighbor.neighbor[i].depth;
        //record->lqi = ZG->nwk.neighbor.neighbor[i].lqi; /* GP change */
        record->lqi = ZG->nwk.neighbor.neighbor[i].rssi;   /* GP change lqi to rssi as JH request */
        records_num--;
        record++;
      }
      current_index++;
    }
  }
  resp->tsn = tsn;

  zdo_send_resp_by_short(ZDO_MGMT_LQI_RESP_CLID, param, dst_addr);

  TRACE_MSG(TRACE_ZDO3, "<< zdo_lqi_resp", (FMT__0));
}

/* GP add, start */

void zdo_rtg_resp(zb_uint8_t param)
{
  zb_uint8_t *aps_body;
  zb_uint8_t tsn;
  zb_uint16_t dst_addr;
  zb_uint8_t start_index;
  zb_zdo_mgmt_rtg_resp_t *resp;
  zb_apsde_data_indication_t *ind;
  zb_zdo_mgmt_rtg_req_t *req;
  zb_ushort_t i;
  zb_zdo_routing_table_record_t *record;
  zb_size_t max_records_num;
  zb_uint8_t records_num;
  zb_uint8_t current_index = 0;

  TRACE_MSG(TRACE_ZDO3, ">>zdo_rtg_resp %hd", (FMT__H, param));

  if (zb_buf_len(param) < (sizeof(*req) + sizeof(tsn)))
  {
    TRACE_MSG(TRACE_ZDO3, "malformed zdo_rtg_req - drop packet", (FMT__0));
    zb_buf_free(param);
    return;
  }

  aps_body = zb_buf_begin(param);
  ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);
  dst_addr = ind->src_addr;
  tsn = *aps_body;

  aps_body++;
  /*cstat !MISRAC2012-Rule-11.3 */
  /** @mdr{00002,82} */
  req = (zb_zdo_mgmt_rtg_req_t*)aps_body;

  max_records_num = (ZB_ZDO_MAX_PAYLOAD_SIZE - sizeof(zb_zdo_mgmt_rtg_resp_t)) / sizeof(zb_zdo_routing_table_record_t);

#if defined ZB_NWK_MESH_ROUTING && defined ZB_ROUTER_ROLE
  records_num = (zb_nwk_routing_table_size() > req->start_index) ?
                (zb_uint8_t)(zb_nwk_routing_table_size() - req->start_index) : 0U;

  TRACE_MSG(TRACE_ZDO3, "max rec %hd, total %hd, start indx %hd",
            (FMT__H_H_H, max_records_num, zb_nwk_routing_table_size(), req->start_index));
#else
  records_num = 0U;
#endif

  records_num = (zb_uint8_t)((records_num < max_records_num) ? records_num : max_records_num);
  start_index = req->start_index;

  resp = zb_buf_initial_alloc(param, sizeof(zb_zdo_mgmt_rtg_resp_t) + records_num * sizeof(zb_zdo_routing_table_record_t));

  resp->tsn = tsn;
  resp->status = ZB_ZDP_STATUS_SUCCESS;
#if defined ZB_NWK_MESH_ROUTING && defined ZB_ROUTER_ROLE
  resp->routing_table_entries = zb_nwk_routing_table_size();
#else
  resp->routing_table_entries = 0U;
#endif

  resp->start_index = start_index;
  resp->routing_table_list_count = records_num;

  /*cstat !MISRAC2012-Rule-11.3 */
  /** @mdr{00002,83} */
  record = (zb_zdo_routing_table_record_t*)(resp+1);

  TRACE_MSG(TRACE_ZDO3, "will add records %hd", (FMT__H, records_num));
  /* FIXME:TODO: rewrite excluding direct access to routing arrays from here! */
#if defined ZB_NWK_MESH_ROUTING && defined ZB_ROUTER_ROLE
  for (i = 0; i < ZB_NWK_ROUTING_TABLE_SIZE && (records_num != 0U); ++i)
  {

    if (ZB_U2B(ZB_NIB().routing_table[i].used))
    {
      if (current_index>=start_index)
      {
        zb_uint16_t addr;

        record->dest_addr = ZB_NIB().routing_table[i].dest_addr;
        record->flags = 0U;
        ZB_ZDO_RECORD_SET_ROUTE_STATUS(record->flags, ZB_NIB().routing_table[i].status);
#ifndef ZB_LITE_NO_SOURCE_ROUTING
        ZB_ZDO_RECORD_SET_ROUTE_NO_ROUTE_CACHE(record->flags, ZB_NIB().routing_table[i].no_route_cache);
        ZB_ZDO_RECORD_SET_ROUTE_MANY_TO_ONE(record->flags, ZB_NIB().routing_table[i].many_to_one);
        ZB_ZDO_RECORD_SET_ROUTE_RECORD_REQUIRED(record->flags, ZB_NIB().routing_table[i].route_record_required);
#endif
        zb_address_short_by_ref(&addr, ZB_NIB().routing_table[i].next_hop_addr_ref);
        record->next_hop_addr = addr;

        records_num--;
        record++;
      }
      current_index++;
    }
  }
#endif
  resp->tsn = tsn;

  zdo_send_resp_by_short(ZDO_MGMT_RTG_RESP_CLID, param, dst_addr);

  TRACE_MSG(TRACE_ZDO3, "<< zdo_rtg_resp", (FMT__0));
}

/* GP add, end */


void zdo_mgmt_bind_resp(zb_uint8_t param)
{
  zb_uint8_t *aps_body;
  zb_uint8_t tsn;
  zb_apsde_data_indication_t *ind;
  zb_uint8_t start_index;
  zb_zdo_mgmt_bind_resp_t *resp;
  zb_zdo_binding_table_record_t *record;

  zb_size_t record_length = 0;
  zb_uint8_t bind_table_length = 0;
  zb_size_t max_records_num;
  zb_uint8_t records_num;
  zb_uint8_t src_index;
  zb_uint8_t dst_index;

  TRACE_MSG(TRACE_ZDO3, ">>zdo_mgmt_bind_resp %hd", (FMT__H, param));

  if (zb_buf_len(param) < (sizeof(zb_zdo_mgmt_bind_req_t) + sizeof(tsn)))
  {
    TRACE_MSG(TRACE_ZDO3, "malformed zdo_mgmt_bind_resp - drop packet", (FMT__0));
    zb_buf_free(param);
    return;
  }

/* Get request data form param  */
  aps_body = zb_buf_begin(param);
  ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);
  tsn = *aps_body;

  if (ZB_NWK_IS_ADDRESS_BROADCAST(ind->dst_addr) ||
      (ZB_APS_FC_GET_DELIVERY_MODE(ind->fc) != ZB_APS_DELIVERY_UNICAST) )
  {
    TRACE_MSG(TRACE_ZDO2, "zb_zdo_mgmt_bind_req is not unicast", (FMT__0));
    zb_buf_free(param);
    return;
  }

  aps_body++;
  /* only 1 byte is stored in request */
  /*cstat !MISRAC2012-Rule-11.3 */
  /** @mdr{00002,84} */
  start_index = ((zb_zdo_mgmt_bind_req_t*)aps_body)->start_index;

/* Calculate max_records num  - number of records that fits
 * the packet */
  max_records_num = (ZB_ZDO_MAX_PAYLOAD_SIZE - sizeof(zb_zdo_mgmt_bind_resp_t)) / sizeof(zb_zdo_binding_table_record_t);

/* Count records_num  */
/* If start_index is greater then table entries number return 0*/
  records_num = (ZG->aps.binding.dst_n_elements  > start_index) ?
                (zb_uint8_t)(ZG->aps.binding.dst_n_elements - start_index) : 0U;

  TRACE_MSG(TRACE_ZDO3, "max rec %hd, used %hd, start indx %hd records_nim %hd",
            (FMT__H_H_H_H, max_records_num, ZG->aps.binding.dst_n_elements, start_index, records_num));

/* If requested records number is greater then maximun records number
 * in this packet return max_records_num */
  records_num = (zb_uint8_t)((records_num < max_records_num) ? records_num : max_records_num);

  TRACE_MSG(TRACE_ZDO3, "records_num after check for max_records_num %hd",
            (FMT__H, records_num));

/* Allocate responce buffer and fill general response fields   */
  resp = zb_buf_initial_alloc(param, sizeof(zb_zdo_mgmt_bind_resp_t) + records_num * sizeof(zb_zdo_binding_table_record_t));
  resp->tsn = tsn;
  resp->status = ZB_ZDP_STATUS_SUCCESS;
  resp->binding_table_entries = ZG->aps.binding.dst_n_elements;
  resp->start_index = start_index;
  resp->binding_table_list_count = records_num;

  TRACE_MSG(TRACE_ZDO3, "mgmt_bind_resp: Status: %d, Bind_Table_Entries %d,Start_index %d, Binding_Table_List_Cnt %d",
            (FMT__D_D_D_D, resp->status, resp->binding_table_entries, resp->start_index, resp->binding_table_list_count));

  /*cstat !MISRAC2012-Rule-11.3 */
  /** @mdr{00002,85} */
  record = (zb_zdo_binding_table_record_t*)(resp+1);

/* TODO: copy records_num binding table entries. */
  TRACE_MSG(TRACE_ZDO3, "will add %hd binding records", (FMT__H,
                                                        records_num));

/* Stepping on designation address table */
  for (dst_index = start_index; dst_index < (records_num + start_index); dst_index++)
  {
    /* SRC entry index */

    src_index = ZG->aps.binding.dst_table[dst_index].src_table_index;
    TRACE_MSG(TRACE_ZDO3, "Calculating src_index = %d", (FMT__D, src_index));

    /* Copy records from binding.src_table */
    zb_address_ieee_by_ref(record->src_address, ZG->aps.binding.src_table[src_index].src_addr);
    record->src_endp = ZG->aps.binding.src_table[src_index].src_end;
    record->cluster_id = ZG->aps.binding.src_table[src_index].cluster_id;

    /* Copy records from binding.dst_table */
    record->dst_addr_mode = ZG->aps.binding.dst_table[dst_index].dst_addr_mode;
    /* Address fields depands on dst_addr_mode value */
    switch (record->dst_addr_mode)
    {
      case ZB_APS_BIND_DST_ADDR_GROUP:
        /* If it is a 16-bit group address without DstEndpoint*/
        record->dst_address.addr_short = ZG->aps.binding.dst_table[dst_index].u.group_addr;
        /* record = SrcIEEEAddr + SrcEndp + ClusterID + DstAddrMode + GroupAddr = 14 bytes  */
        record_length = sizeof(zb_zdo_binding_table_record_t) - 7U;
      break;

      case ZB_APS_BIND_DST_ADDR_LONG:
        /* If it is a 64-bit extended address and DstEndpoint present */
       zb_address_ieee_by_ref(record->dst_address.addr_long, ZG->aps.binding.dst_table[dst_index].u.long_addr.dst_addr);
       record->dst_endp = ZG->aps.binding.dst_table[dst_index].u.long_addr.dst_end;
       /* record = SrcIEEEAddr + SrcEndp + ClusterID + DstAddrMode +
        * DestIEEEAddr + DestEndp = 21 bytes
        */
       record_length = sizeof(zb_zdo_binding_table_record_t);
      break;

      default:
        /* MISRA rule 16.4 - Mandatory default label */
        break;
    }
     /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,86} */
    record = (zb_zdo_binding_table_record_t*)((zb_uint8_t*)record + record_length);
    /* Calculate space used by binding table in buffer */
    bind_table_length += (zb_uint8_t)record_length;
  }

  if (bind_table_length < records_num * sizeof(zb_zdo_binding_table_record_t))
  {
    /* Cut unused space in buffer */
    zb_buf_cut_right(param, records_num * sizeof(zb_zdo_binding_table_record_t) - bind_table_length);
  }

  zdo_send_resp_by_short(ZDO_MGMT_BIND_RESP_CLID, param, ind->src_addr);

  TRACE_MSG(TRACE_ZDO3, "<< zdo_mgmt_bind_resp", (FMT__0));
}


static zb_ret_t zb_zdo_validate_leave_req(zb_uint16_t short_src_addr, zb_zdo_mgmt_leave_req_t *req)
{
  zb_ret_t ret;
#ifndef ZB_LITE_NO_INDIRECT_MGMT_LEAVE
  zb_uint8_t addr_ref;
  zb_neighbor_tbl_ent_t *nbr;
#endif
  TRACE_MSG(TRACE_NWK1, ">>zb_zdo_validate_leave_req src_addr %x role %hd",
            (FMT__D_H, short_src_addr, zb_get_device_type()));

#ifndef ZB_COORDINATOR_ONLY
  ret = zb_nwk_validate_leave_req(short_src_addr);
#else
  ret = RET_ERROR;
#endif
  /*cstat !MISRAC2012-Rule-14.3_b */
  /** @mdr{00015,11} */
  if (ret == RET_OK &&
      /* If MGMTLeaveWithoutRejoinEnabled attribute is set to FALSE,
       * the node SHALL ignore MGMT Leave Without Rejoin commands. */
      !(ZB_U2B(ZB_ZDO_CHECK_IF_LEAVE_WITHOUT_REJOIN_ALLOWED()) || ZB_U2B(req->rejoin)))
  {
    /*cstat !MISRAC2012-Rule-2.1_b */
    /** @mdr{00015,7} */
    ret = RET_UNAUTHORIZED;
  }
#ifndef ZB_LITE_NO_INDIRECT_MGMT_LEAVE
  if (ret != RET_OK && ZB_IEEE_ADDR_IS_VALID(req->device_address))
  {
    /* If we have ieee dst and we are router and it is for our child, process  */
    zb_ret_t ret2 = zb_address_by_ieee(req->device_address, ZB_FALSE, ZB_FALSE, &addr_ref);
    if (ret2 == RET_OK)
    {
      ret2 = zb_nwk_neighbor_get(addr_ref, ZB_FALSE, &nbr);
    }
    if (ret2 == RET_OK
        /*cstat !MISRAC2012-Rule-13.5 */
        /* After some investigation, the following violation of Rule 13.5 seems to be
         * a false positive. There are no side effect to 'ZB_IS_DEVICE_ZC_OR_ZR()'. This
         * violation seems to be caused by the fact that 'ZB_IS_DEVICE_ZC_OR_ZR()' is an
         * external macro, which cannot be analyzed by C-STAT. */
        && ZB_IS_DEVICE_ZC_OR_ZR()
        /*cstat !MISRAC2012-Rule-1.3_* !MISRAC2012-Rule-9.1_* */
        /* Rule-9.1 and Rule-1.3: The function zb_nwk_neighbor_get returns RET_OK
         * only when nbr variable was initialized */
        && nbr->relationship == ZB_NWK_RELATIONSHIP_CHILD)
    {
      ret = RET_OK;
    }
  }
#else
  ZVUNUSED(ieee_dst_addr);
#endif
  TRACE_MSG(TRACE_NWK1, "<<zb_zdo_validate_leave_req ret %d", (FMT__D, ret));

  return ret;
}


void zdo_mgmt_leave_srv(zb_uint8_t param)
{
#ifndef ZB_LITE_NO_INDIRECT_MGMT_LEAVE
  zb_ushort_t i;
#endif
  zb_zdo_mgmt_leave_req_t req;
  zb_uint8_t *aps_body;
  zb_apsde_data_indication_t *ind;
  zb_uint8_t status = ZB_ZDP_STATUS_SUCCESS;
  zb_bool_t leave_to_us;
  zb_uint8_t relationship;
  zb_ret_t validate_result;

  TRACE_MSG(TRACE_ZDO3, ">>zdo_mgmt_leave_srv %hd", (FMT__H, param));

  /*
    We are here because we got mgmt_leave_req from the remote (or locally, but
    thru aps & nwk).
  */

  /**
     \par Notes about LEAVE

     - when got mgmt_leave_req, fill pending list and call nlme.leave.request

     - nlme.leave.request either :
     - unicasts to its child, call leave.confirm at packet send complete
     - if its leave for us, broadcasts LEAVE command, call leave.confirm at packet send complete

     - when got LEAVE command from the net:
     - from child with 'request' = 0 and we are not TC, send UPDATE-DEVICE
     to TC, then forget this child
     - from parent with 'request' or 'remove child' == 1, send broadcast LEAVE
     with same 'remove child' and 'request' = 0
     call leave.confirm at packet send complete
     - else - from any device with request = 0 - forget this device

     - leave.confirm called when LEAVE command has sent, from mcps-data.confirm
     (not when LEAVE procedure complete - see later)

     - if LEAVE was caused by mgmt_leave_req receive, we must send mgmt_leave_rsp.
     We can do it only if we did not leave network yet.
     Means, we must remember somehow address of device which issued mgmt_leave_req and,
     if it not empty, send resp to it.
     Need a list of pending mgmt_leave_req. It holds address ref (1b) and buffer id (1b).
     Do not clear entry in this list now - we still need it.

     If no entry in this list, there was no mgmt_leave_req, so can call "leave finish" now.

     - when mgmt_leave_rsp successfully sent (means - from aps-data.confirm), we must check:
     do we need to leave network and rejoin after it?
     We use here, again, same buffer, so can use same list of pending mgmt_leave_req.
     If it was leave rsp, call call "leave finish" now.
  */

  aps_body = zb_buf_begin(param);
  ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);
  ZB_MEMCPY(&req, (aps_body + 1), sizeof(zb_zdo_mgmt_leave_req_t));

  /* [VP]: + reject broadcast mgmt_leave_req (according to test DR-TAR-TC-05D)*/
  if ( (zb_buf_len(param) < sizeof(zb_zdo_mgmt_leave_req_t) + sizeof(zb_uint8_t))
       || ZB_NWK_IS_ADDRESS_BROADCAST(ind->dst_addr) )
  {
    TRACE_MSG(TRACE_ZDO3, "Malformed or broadcast mgmt_leave request - drop packet", (FMT__0));
    zb_buf_free(param);
    return;
  }

  relationship = zb_nwk_get_nbr_rel_by_short(ind->mac_src_addr);

  if (ZB_IS_DEVICE_ZED() && relationship != ZB_NWK_RELATIONSHIP_PARENT)
  {
    /* If the corresponding entry in the nwkNeighborTable has a Relationship value that
       is not 0x00 (neighbor is the parent), then no further processing shall be done. */

    TRACE_MSG(TRACE_ZDO3, "mgmt_leave for zed, relationship %hd - drop packet", (FMT__H, relationship));
    zb_buf_free(param);
    return;
  }

  leave_to_us = ( ZB_IEEE_ADDR_CMP(ZB_PIBCACHE_EXTENDED_ADDRESS(), req.device_address) ||
                  ZB_IEEE_ADDR_IS_ZERO(req.device_address) ) ? ZB_TRUE:ZB_FALSE;

  validate_result = zb_zdo_validate_leave_req(ind->mac_src_addr, &req);
  if (validate_result == RET_ERROR
#ifdef ZB_LITE_NO_INDIRECT_MGMT_LEAVE
      || !leave_to_us
#endif
  )
  {
    TRACE_MSG(TRACE_ERROR, "leave req is not validated", (FMT__0));
    status = ZB_ZDP_STATUS_NOT_SUPPORTED;
  }
  else if (leave_to_us && validate_result == RET_UNAUTHORIZED)
  {
    TRACE_MSG(TRACE_ZDO3, "Leave without rejoin disallowed - drop packet", (FMT__0));
    zb_buf_free(param);
    return;
  }
  else
  {
    /* MISRA rule 15.7 requires empty 'else' branch. */
  }

  if (leave_to_us)
  {
#ifdef ZB_ZDO_DENY_LEAVE_CONFIG
    /*leave request is to us, check if we are allowed to leave by ZDO leave request */
    if (!ZB_U2B(ZDO_CTX().leave_req_allowed))
    {
      TRACE_MSG(TRACE_ZDO3, "Leave not allowed", (FMT__0));
      zb_buf_free(param);
      return;
    }
#endif
    if (status == (zb_uint8_t)ZB_ZDP_STATUS_SUCCESS)
    {
    /* When we are requested to leave remember rejoin and remove_children flags
       and send mgmt_leave_resp immediately; after confrim for mgmt_leave_resp
       leave the network. */
      ZG->nwk.leave_context.rejoin_after_leave = req.rejoin;
      ZG->nwk.leave_context.remove_children = req.remove_children;
      ZG->zdo.handle.mgmt_leave_resp_buf = param;
    }
  }
#ifndef ZB_LITE_NO_INDIRECT_MGMT_LEAVE
  else
  {
    /* add entry to the leave req table */
    for (i = 0;
	 i < ZB_ZDO_PENDING_LEAVE_SIZE
	   && ZB_U2B(ZB_IS_LEAVE_PENDING(i));
	 ++i)
    {
    }

    if (i == ZB_ZDO_PENDING_LEAVE_SIZE)
    {
      TRACE_MSG(TRACE_ERROR, "out of pending leave list send resp now.!", (FMT__0));
      status = ZB_ZDP_STATUS_INSUFFICIENT_SPACE;
      /* Decrement for prevention onward MISRA array borders violation */
      i--;
    }

    if (status == (zb_uint8_t)ZB_ZDP_STATUS_SUCCESS)
    {
      ZB_SET_LEAVE_PENDING(i);
      ZG->nwk.leave_context.pending_list[i].tsn = *aps_body;
      ZG->nwk.leave_context.pending_list[i].src_addr = ind->src_addr;
      ZG->nwk.leave_context.pending_list[i].buf_ref = param;
      TRACE_MSG(TRACE_ZDO3, "remember mgmt_leave at i %hd, tsn %hd, addr %d, buf_ref %hd",
		(FMT__H_H_D_H, i, ZG->nwk.leave_context.pending_list[i].tsn,
		 ZG->nwk.leave_context.pending_list[i].src_addr,
		 ZG->nwk.leave_context.pending_list[i].buf_ref));

      /* Now locally call LEAVE.request */
      {
	zb_nlme_leave_request_t *lr;

	lr = ZB_BUF_GET_PARAM(param, zb_nlme_leave_request_t);
#if defined R21_SPOOKY_MGMT_LEAVE_REQ_DESCRIPTION
	/* 2.4.3.3.5.2. Effect on Receipt
	 * ... If the leave request was validated and accepted, then the
	 * receiving device shall generate the NLME-LEAVE.request to
	 * disassiciate from the currently associated network. The
	 * NLME-LEAVE.request shall have the DeviceAddress parameter set
	 * to the local device's nwkIeeeAddress from the NIB, the
	 * RemoveChildren shall be set to FALSE, and the Rejoin
	 * parameter shall be set to FALSE
	 *
	 * [MM]: VVVEEEERRRYYY UNCLEAR, why ???!!!!
	 */
	ZB_IEEE_ADDR_COPY(lr->device_address, ZB_PIBCACHE_EXTENDED_ADDRESS());
	lr->remove_children = ZB_FALSE;
	lr->rejoin = ZB_FALSE;
#else
	ZB_IEEE_ADDR_COPY(lr->device_address, req.device_address);
	lr->remove_children = req.remove_children;
	lr->rejoin = req.rejoin;
#endif

	ZB_SCHEDULE_CALLBACK(zb_nlme_leave_request, param);
      }
    }
  }
#endif  /* ZB_LITE_NO_INDIRECT_MGMT_LEAVE */

  if (leave_to_us || (status != (zb_uint8_t)ZB_ZDP_STATUS_SUCCESS))
  {
    zb_zdo_default_resp_t *resp;

    resp = zb_buf_initial_alloc(param, sizeof(zb_zdo_default_resp_t));
    resp->tsn = *aps_body; /* incoming tsn **/
    resp->status = status;
    zdo_send_resp_by_short(ZDO_MGMT_LEAVE_RESP_CLID, param, ind->src_addr);
  }

  TRACE_MSG(TRACE_ZDO3, "<<zdo_mgmt_leave_srv", (FMT__0));
}

zb_bool_t zdo_try_send_mgmt_leave_rsp(zb_uint8_t param, zb_uint8_t status)
{
  zb_ushort_t i;
  for (i = 0 ; i < ZB_ZDO_PENDING_LEAVE_SIZE ; ++i)
  {
    if (ZB_U2B(ZB_IS_LEAVE_PENDING(i))
        && ZG->nwk.leave_context.pending_list[i].buf_ref == param)
    {
      zb_zdo_default_resp_t *resp;

      TRACE_MSG(TRACE_ZDO3, "sending mgmt_leave_rsp i %hd, tsn %hd, addr %d, buf_ref %hd",
                (FMT__H_H_D_H, i, ZG->nwk.leave_context.pending_list[i].tsn,
                 ZG->nwk.leave_context.pending_list[i].src_addr,
                 ZG->nwk.leave_context.pending_list[i].buf_ref));

      resp = zb_buf_initial_alloc(param, sizeof(zb_zdo_default_resp_t));
      resp->tsn = ZG->nwk.leave_context.pending_list[i].tsn;
      resp->status = status;

#ifndef ZB_COORDINATOR_ONLY
      if (ZG->nwk.leave_context.pending_list[i].src_addr == ZB_PIBCACHE_NETWORK_ADDRESS())
      {
        TRACE_MSG(TRACE_ZDO3, "local leave mgmt leave complete", (FMT__0));
        ZB_RESET_LEAVE_PENDING(i);
        if (zdo_af_resp(param) == RET_NOT_FOUND)
        {
          zb_nwk_do_leave_local(param);
        }
        else
        {
          zb_ret_t ret = zb_buf_get_out_delayed(zb_nwk_do_leave_local);
          if (ret != RET_OK)
          {
            TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
            return ZB_FALSE;
          }
        }
      }
      else
#endif  /* #ifndef ZB_COORDINATOR_ONLY */
      {
        zdo_send_resp_by_short(ZDO_MGMT_LEAVE_RESP_CLID, param,
                               ZG->nwk.leave_context.pending_list[i].src_addr);
      }

      ZB_RESET_LEAVE_PENDING(i);

      return ZB_TRUE;
    }
  }
  return ZB_FALSE;
}


#ifndef ZB_COORDINATOR_ONLY
void zb_nwk_do_leave_local(zb_uint8_t param)
{
  zb_nwk_do_leave(param, ZG->nwk.leave_context.rejoin_after_leave);
}
#endif  /* ZB_COORDINATOR_ONLY */

#ifdef ZB_ROUTER_ROLE
void zb_zdo_mgmt_permit_joining_confirm_handle(zb_uint8_t param)
{
  zb_nlme_permit_joining_request_t *req_param;

  TRACE_MSG(TRACE_ZDO3, ">>zb_zdo_mgmt_permit_joining_confirm_handle %hd", (FMT__H, param));

  TRACE_MSG(TRACE_ZDO3, "permit_duration %hd tc_significance %hd", (FMT__H_H,
      ZDO_CTX().handle.permit_duration, ZDO_CTX().handle.tc_significance));

  req_param = (zb_nlme_permit_joining_request_t *)ZB_BUF_GET_PARAM(param, zb_nlme_permit_joining_request_t);
  req_param->permit_duration = ZDO_CTX().handle.permit_duration;
  ZB_SCHEDULE_CALLBACK(zb_nlme_permit_joining_request, param);

  TRACE_MSG(TRACE_ZDO3, "<<zb_zdo_mgmt_permit_joining_confirm_handle", (FMT__0));
}

static void broadcast_close_network(zb_uint8_t param)
{
  zb_zdo_mgmt_permit_joining_req_param_t *req_param;

  TRACE_MSG(TRACE_ZDO3, "broadcast_close_network", (FMT__0));
  req_param = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_permit_joining_req_param_t);

  ZB_BZERO(req_param, sizeof(zb_zdo_mgmt_permit_joining_req_param_t));
  req_param->dest_addr = ZB_NWK_BROADCAST_ROUTER_COORDINATOR;
  req_param->permit_duration = 0;
  req_param->tc_significance = 1;
  (void)zb_zdo_mgmt_permit_joining_req(param, NULL);
}


void zb_zdo_mgmt_permit_joining_handle(zb_uint8_t param)
{
  zb_ret_t ret;
  zb_uint8_t *aps_body = zb_buf_begin(param);
  zb_zdo_mgmt_permit_joining_req_t *req;
  zb_uint8_t tsn;
  zb_apsde_data_indication_t *ind;
  zb_zdo_mgmt_permit_joining_resp_t *resp;
  zb_uint8_t status = ZB_ZDP_STATUS_SUCCESS;

  TRACE_MSG(TRACE_ZDO3, ">>zb_zdo_mgmt_permit_joining_handle param %hd", (FMT__H, param));

  if (zb_buf_len(param) < (sizeof(*req) + sizeof(tsn)))
  {
    TRACE_MSG(TRACE_ZDO3, "malformed zdo_mgmt_permit_joining_handle_req - drop packet", (FMT__0));
    zb_buf_free(param);
    return;
  }

  tsn = *aps_body;
  ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);

  aps_body++;
  /*cstat !MISRAC2012-Rule-11.3 */
  /** @mdr{00002,87} */
  req = (zb_zdo_mgmt_permit_joining_req_t *)aps_body;
  TRACE_MSG(TRACE_ZDO3, "permit_duration %hd tc_significance %hd", (FMT__H_H, req->permit_duration, req->tc_significance));

  if (ZB_IS_TC())
  /*cstat !MISRAC2012-Rule-2.1_b */
  /** @mdr{00012,32} */
  {
    /* [MM]: Q: It looks like the status code of INVALID_REQUEST is
     * not the part of the ZDP status codes, but the part of NWK
     * codes. Is my understanding correct? */
    status = (zb_uint8_t)((zb_secur_changing_tc_policy_check() == ZB_APS_STATUS_SUCCESS)
      ? (ZB_APS_STATUS_SUCCESS) : (ZB_ZDP_STATUS_INV_REQUESTTYPE));

    if (!ZB_TCPOL().allow_remote_policy_change)
    {
      /*
        When a Trust Center receives a Mgmt_Permit_join_req where the
        acceptRemoteTcPolicyChange=FALSE, the Trust Center may broadcast a Mgmt_permit_join_req
        with permitDuration=0 to close the network and prevent it from advertising
        that new devices are being accepted. (CCB2156)
      */

      TRACE_MSG(TRACE_ZDO3, "allow_remote_policy_change=False, closing the network", (FMT__0));
      ret = zb_buf_get_out_delayed(broadcast_close_network);
      if (ret != RET_OK)
      {
        /* Not able to broadcast Trust Center message */
        TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
      }
    }
  }

  if (status == (zb_uint8_t)ZB_ZDP_STATUS_SUCCESS)
  {
    /* 2.4.3.3.7.2.
     * Versions of this specification prior to revision 21 allowed a
     * value of 0xFF to be interpreted as forever. Version 21 and later
     * do not allow this. All devices conforming to this specification
     * shall interpret 0xFF as 0xFE. Devices that wish to extend the
     * PermitDuration beyond 0xFE seconds shall periodically re-send the
     * Mgmt_Permit_Joining_req.
     */
    ZDO_CTX().handle.permit_duration = (req->permit_duration == 0xFFU) ? (0xFEU) : (req->permit_duration);

    /* ... A value of zero for the TC_Significance field has been
     * deprecated. The field shall always be included in the message and
     * all received frames shall be treated as though set to 1,
     * regardless of the actual received value. In other words, all
     * Mgmt_Permit_Joining_req  shall be treated as a request to change
     * the TC Policy. */
    ZDO_CTX().handle.tc_significance = ZB_TRUE;
  }

  if( !ZB_NWK_IS_ADDRESS_BROADCAST(ind->dst_addr) )
  {
    resp = zb_buf_initial_alloc(param, sizeof(zb_zdo_mgmt_permit_joining_resp_t));
    ZB_BZERO(resp, sizeof(zb_zdo_mgmt_permit_joining_resp_t));
    resp->tsn = tsn;

    /* If the remote device is the Trust Center the Trust Center
     * authorization policy may be affected. Whether the Trust Center
     * accepts a change in its authorization policy is dependent upon
     * its Trust Center policies. A Trust Center device receiving a
     * Mgmt_Permit_Joining_req shall execute the procedure in section
     * 4.7.3.2 to determine if the request is permitted. If the
     * operation was not permitted, the status code of INVALID_REQUEST
     * shall be set. If the operation was allowed, the status code of
     * SUCCESS shall be set.
     */

    resp->status = ZB_IS_DEVICE_ZC_OR_ZR()
      ? (status) : ((zb_uint8_t)ZB_ZDP_STATUS_NOT_SUPPORTED);

#ifdef ZB_ROUTER_ROLE
    if ( ZB_IS_DEVICE_ZC_OR_ZR())
    {
      /* Register confirm on response command */
      ZDO_CTX().handle.permit_joining_param = param;
    }
#endif
    zdo_send_resp_by_short(ZDO_MGMT_PERMIT_JOINING_RESP_CLID, param, ind->src_addr);
  }
  else
  {
    TRACE_MSG(TRACE_ZDO3, "permit_duration for broadcast", (FMT__0));
    if (status == (zb_uint8_t)ZB_ZDP_STATUS_SUCCESS)
    {
      ZG->zdo.handle.permit_joining_param = 0;
#ifdef ZB_ROUTER_ROLE
      if (ZB_IS_DEVICE_ZC_OR_ZR())
      {
        ZB_SCHEDULE_CALLBACK(zb_zdo_mgmt_permit_joining_confirm_handle, param);
        param = 0;
      }
#endif
    }

    if (param != 0U)
    {
      zb_buf_free(param);
    }
  }
  TRACE_MSG(TRACE_ZDO3, "<<zb_zdo_mgmt_permit_joining_handle", (FMT__0));
}
#endif /* ZB_ROUTER_ROLE */


#if (defined ZB_JOINING_LIST_SUPPORT) && defined ZB_ROUTER_ROLE

void zdo_nwk_joining_list_resp_send(zb_uint8_t param)
{
  zb_mlme_get_confirm_t cfm;
  zb_mlme_get_ieee_joining_list_res_t get_res_params;
  zb_zdo_mgmt_nwk_ieee_joining_list_rsp_t *resp;
  zb_uint8_t addr_list_bytes;
  zb_uint8_t *ptr;

  TRACE_MSG(TRACE_ZDO3, ">> zdo_nwk_joining_list_resp_send param %hd",
            (FMT__H, param));

  /* extract and cut zb_mlme_get_confirm_t */
  ZB_MEMCPY(&cfm, zb_buf_begin(param), sizeof(zb_mlme_get_confirm_t));
  if (cfm.status == MAC_SUCCESS)
  {
    (void)zb_buf_cut_left(param, sizeof(zb_mlme_get_confirm_t));

    /* extract and cut zb_mlme_get_ieee_joining_list_res_t */
    ZB_ASSERT(zb_buf_len(param) >= sizeof(get_res_params));
    ZB_MEMCPY(&get_res_params, zb_buf_begin(param), sizeof(get_res_params));
    (void)zb_buf_cut_left(param, sizeof(get_res_params));

    addr_list_bytes = (zb_uint8_t)zb_buf_len(param);
    ZB_ASSERT(addr_list_bytes % sizeof(zb_ieee_addr_t) == 0U);

    /* TODO: do not transmit rest of frame if total cnt is 0? */

    /* success */
    resp = zb_buf_alloc_left(param, sizeof(zb_zdo_mgmt_nwk_ieee_joining_list_rsp_t));
    resp->tsn = ZDO_CTX().joining_list_ctx.tsn;
    resp->status = ZB_ZDP_STATUS_SUCCESS;
    resp->ieee_joining_list_update_id = ZDO_CTX().joining_list_ctx.update_id;
    resp->joining_policy = get_res_params.joining_policy;
    resp->ieee_joining_list_total = get_res_params.total_length;
    resp->start_index = get_res_params.start_index;
    resp->ieee_joining_count = addr_list_bytes / (zb_uint8_t)sizeof(zb_ieee_addr_t);
  }
  else
  {
    /* index out of bounds */
    ptr = zb_buf_initial_alloc(param, 2);
    ptr[0] = ZDO_CTX().joining_list_ctx.tsn;
    ptr[1] = ZB_ZDP_STATUS_INVALID_INDEX;
  }

  if (ZB_NWK_IS_ADDRESS_BROADCAST(ZDO_CTX().joining_list_ctx.dst_addr))
  {
    TRACE_MSG(TRACE_ZDO3, "broadcasting case", (FMT__0));
    zb_buf_flags_or(param, ZB_BUF_ZDO_CMD_NO_RESP);
    /* do it some other way? */
    ptr = zb_buf_cut_left(param, 1);
    ZVUNUSED(ptr);
    (void)zdo_send_req_by_short(ZDO_MGMT_NWK_IEEE_JOINING_LIST_RESP_CLID,
                                param,
                                ZDO_CTX().joining_list_ctx.broadcast_confirm_cb,
                                ZDO_CTX().joining_list_ctx.dst_addr,
                                ZB_ZDO_CB_BROADCAST_COUNTER);
  }
  else
  {
    TRACE_MSG(TRACE_ZDO3, "responding to request case", (FMT__0));
    zdo_send_resp_by_short(ZDO_MGMT_NWK_IEEE_JOINING_LIST_RESP_CLID,
                           param,
                           ZDO_CTX().joining_list_ctx.dst_addr);
    zb_ieee_joining_list_cb_completed();
  }

  TRACE_MSG(TRACE_ZDO3, "<< zdo_nwk_joining_list_resp_send", (FMT__0));
}

static void zdo_nwk_joining_list_resp_operation(zb_uint8_t param)
{
  zb_bool_t ret = ZB_FALSE;
  zb_uint8_t *ptr;
  zb_uint8_t start_index;
  zb_apsde_data_indication_t *ind;
  zb_zdo_mgmt_nwk_ieee_joining_list_req_t *req;
  zb_mlme_get_request_t *joining_list_req;
  zb_mlme_get_ieee_joining_list_req_t *joining_list_req_params;

  TRACE_MSG(TRACE_ZDO3, ">>zdo_nwk_joining_list_resp_operation %hd", (FMT__H, param));

  if (zb_buf_len(param) < (sizeof(*req) + sizeof(zb_uint8_t)))
  {
    TRACE_MSG(TRACE_ZDO3, "malformed zdo_mgmt_nwk_ieee_joining_list_req - drop packet", (FMT__0));
    zb_buf_free(param);
    return;
  }

  if (!ZDO_CTX().joining_list_ctx.is_consistent)
  {
    TRACE_MSG(TRACE_ZDO3, "joining list is no consistent, skipping", (FMT__0));
    zb_ieee_joining_list_op_delay();
    ret = ZB_TRUE;
  }

  if (!ret && !ZB_U2B(ZB_AIB().aps_designated_coordinator))
  {
    TRACE_MSG(TRACE_ZDO3, "only coordinators are expected to handle this", (FMT__0));
    ret = ZB_TRUE;
  }

  if (!ret)
  {
    void *p;
    ptr = zb_buf_begin(param);
    ZDO_CTX().joining_list_ctx.tsn = *ptr;
    ptr++;

    ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);
    ZDO_CTX().joining_list_ctx.dst_addr = ind->src_addr;

    p = (void *)ptr;
    req = (zb_zdo_mgmt_nwk_ieee_joining_list_req_t *)p;
    start_index = req->start_index;

    p = zb_buf_initial_alloc(param, sizeof(*joining_list_req) + sizeof(*joining_list_req_params));

    joining_list_req = (zb_mlme_get_request_t *)p;
    p = (void *)(joining_list_req + 1);
    joining_list_req_params = (zb_mlme_get_ieee_joining_list_req_t *)(p);

    joining_list_req->pib_attr = ZB_PIB_ATTRIBUTE_JOINING_IEEE_LIST;
    joining_list_req->pib_index = 0;
    joining_list_req->confirm_cb_u.cb = &zdo_nwk_joining_list_resp_send;

    /* put request params */
    joining_list_req_params->start_index = start_index;
    joining_list_req_params->count = ZB_JOINING_LIST_RESP_ITEMS_LIMIT;

    ZB_SCHEDULE_CALLBACK(zb_mlme_get_request, param);
  }

  TRACE_MSG(TRACE_ZDO3, "<< zdo_nwk_joining_list_resp_operation", (FMT__0));
}

void zdo_nwk_joining_list_resp(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO3, "scheduling zdo_nwk_joining_list_resp %hd", (FMT__H, param));
  if (!zb_ieee_joining_list_put_cb(&zdo_nwk_joining_list_resp_operation, param))
  {
    /* just ignore this request */
    TRACE_MSG(TRACE_ZDO3, "no free space in JL operation queue", (FMT__0));
    zb_buf_free(param);
  }
}

#endif /* (defined ZB_JOINING_LIST_SUPPORT) && defined ZB_ROUTER_ROLE */

/*! @} */
