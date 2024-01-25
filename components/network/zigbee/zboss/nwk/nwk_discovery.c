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
/* PURPOSE: Network discovery routine
*/

#define ZB_TRACE_FILE_ID 2236
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_mac.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "nwk_internal.h"
#include "zb_zdo.h"
#include "zb_ie.h"
#include "zb_watchdog.h"
#include "zdo_wwah_survey_beacons.h"
#include "zdo_wwah_parent_classification.h"
#include "zdo_wwah_stubs.h"

#ifdef ZB_MAC_TESTING_MODE
#include "zb_mac_globals.h"
#endif

/*! \addtogroup ZB_NWK */
/*! @{ */

#ifdef ZB_JOIN_CLIENT
/* find weakest panid from panid table  */
static zb_ret_t get_weakest_pan_to_remove(zb_uint8_t *min_lqi, zb_address_pan_id_ref_t *panid_ref)
{
  zb_uint8_t i;
  zb_ret_t ret = RET_NOT_FOUND;
  /* Use lookup table for maximum lqi per each PAN ID */
  zb_uint8_t lookup_max_lqi[ZB_PANID_TABLE_SIZE];
  zb_uint8_t used_pan_addr[(ZB_PANID_TABLE_SIZE +7U)/ 8U] = { 0 };
  zb_uint8_t min_lqi_val = 0xFF; /* Max possible LQI */

  ZB_ASSERT(min_lqi);
  ZB_ASSERT(panid_ref);

  TRACE_MSG(TRACE_NWK1, ">>get_weakest_pan %p %p", (FMT__P_P, min_lqi, panid_ref));

  ZB_BZERO(lookup_max_lqi, ZB_PANID_TABLE_SIZE * sizeof(zb_uint8_t));

  for (i = 0 ; i < ZB_NEIGHBOR_TABLE_SIZE ; i++)
  {
    zb_ext_neighbor_tbl_ent_t *eent = &ZG->nwk.neighbor.neighbor[i];
    if (ZB_U2B(eent->used) && ZB_U2B(eent->ext_neighbor))
    {
      zb_set_bit_in_bit_vector(used_pan_addr, eent->u.ext.panid_ref);
      /* find max lqi for a pan: there may be several neighbors from the same pan */
      if (eent->lqi > lookup_max_lqi[eent->u.ext.panid_ref])
      {
        lookup_max_lqi[eent->u.ext.panid_ref] = eent->lqi;
      }
    }
  }

  /* Find the PAN with the weakest beacon */
  for (i = 0; i < ZB_PANID_TABLE_SIZE; i++)
  {
#ifdef ZB_NWK_BLACKLIST
    /* We're not allowed to remove blacklisted PAN from PANID table */
    zb_ext_pan_id_t ext_pan_id;
    zb_address_get_pan_id(ZG->nwk.neighbor.neighbor[i].u.ext.panid_ref, ext_pan_id);

    if (zb_nwk_blacklist_check(ext_pan_id) != RET_OK)
    {
      ret = RET_BLOCKED;
    }
#endif
    if (ret == RET_NOT_FOUND
        && ZB_CHECK_BIT_IN_BIT_VECTOR(used_pan_addr, i))
    {
      /* find a pan with minimal lqi value */
      if (lookup_max_lqi[i] < min_lqi_val)
      {
        *panid_ref = i;
        min_lqi_val = lookup_max_lqi[i];
        ret = RET_OK;
      }
    }
  }
  *min_lqi = min_lqi_val;

  TRACE_MSG(TRACE_NWK1, "<<get_weakest_pan ret %d panid_ref %hd min_lqi %hd",
            (FMT__D_H_H, ret, *panid_ref, *min_lqi));

  return ret;
}
#endif  /* ZB_JOIN_CLIENT */

static void convert_beacon_to_exneighbor(zb_ext_neighbor_tbl_ent_t *enbt,
                                         zb_address_pan_id_ref_t panid_ref,
                                         zb_mac_beacon_notify_indication_t *ind,
                                         zb_mac_beacon_payload_t *beacon_payload)
{
  /* Table 3.48, page 368. Address is assigned already. */
  /* have no rx_on_when_idle here: it could be defined by capabilitries at
   * join time only, or by device annonsment */
  /* don't touch relationship: no relationship before join */
  /* don't touch transmit_failure here */
  enbt->lqi = ind->pan_descriptor.link_quality;

  /* don't touch outgoing_cost */
  /* nod't use age */
  /* additional fields - table 3.49 */
  enbt->u.ext.panid_ref = panid_ref;
  enbt->u.ext.logical_channel = ind->pan_descriptor.logical_channel;
  enbt->u.ext.channel_page = ind->pan_descriptor.channel_page;
  TRACE_MSG(TRACE_NWK2, "ch %hd lqi %hd (ok for join %hd)",
            (FMT__H_H_H, enbt->u.ext.logical_channel, enbt->lqi,
            (zb_uint8_t) ZB_LINK_QUALITY_IS_OK_FOR_JOIN(enbt->lqi)));
  TRACE_MSG(TRACE_NWK2, "panid_ref 0x%x", (FMT__D, enbt->u.ext.panid_ref));

  enbt->depth = beacon_payload->device_depth;
  /* fields for the Network Descriptor - table 3.8 */
  enbt->u.ext.stack_profile = beacon_payload->stack_profile;
  /* do not store Zigbee version: assume compatible only */
  enbt->u.ext.update_id = beacon_payload->nwk_update_id;

#ifdef ZB_REJOIN_IGNORES_FLAGS
  if (ZG->nwk.handle.state != ZB_NLME_STATE_REJOIN)
#else
  if (ZB_TRUE)
#endif
  {
    enbt->permit_joining = ind->pan_descriptor.super_frame_spec.associate_permit;
    enbt->u.ext.router_capacity = beacon_payload->router_capacity;
    enbt->u.ext.end_device_capacity = beacon_payload->end_device_capacity;
    enbt->u.ext.potential_parent = enbt->permit_joining;
  }
  else
  {
    enbt->permit_joining = enbt->u.ext.router_capacity = enbt->u.ext.end_device_capacity = enbt->u.ext.potential_parent = ZB_TRUE;
  }

#ifdef ZB_ROUTER_ROLE
  /* AT: Join fixed */
  if (ZB_IS_DEVICE_ZC_OR_ZR() &&
      (enbt->u.ext.stack_profile == STACK_PRO))
  {
    enbt->u.ext.potential_parent &= enbt->u.ext.router_capacity;
  }
  else
#endif
  {
    enbt->u.ext.potential_parent &= enbt->u.ext.end_device_capacity;
  }

  /* No sure about device type. Can detect some cases, but not all. */
  if (!ZB_U2B(enbt->depth) && (
#if defined ZB_ENHANCED_BEACON_SUPPORT
      ((ind->beacon_type == ZB_MAC_BEACON_TYPE_ENHANCED_BEACON)
    && (ind->pan_descriptor.coord_addr_mode == ZB_ADDR_64BIT_DEV)
    && (ind->pan_descriptor.enh_beacon_nwk_addr == 0U)) ||
#endif /* ZB_ENHANCED_BEACON_SUPPORT */
      ((ind->beacon_type == ZB_MAC_BEACON_TYPE_BEACON)
    && (ind->pan_descriptor.coord_addr_mode == ZB_ADDR_16BIT_DEV_OR_BROADCAST)
    && (ind->pan_descriptor.coord_address.addr_short == 0U))))
  {
    enbt->device_type = ZB_NWK_DEVICE_TYPE_COORDINATOR; /* see table 3.56*/
  }
  else if (ZB_U2B(enbt->permit_joining) || ZB_U2B(enbt->u.ext.router_capacity) || ZB_U2B(enbt->u.ext.end_device_capacity))
  {
    enbt->device_type = ZB_NWK_DEVICE_TYPE_ROUTER;
  }
  else
  {
    /* No more info - device type is unknown */
    enbt->device_type = ZB_NWK_DEVICE_TYPE_NONE;
  }

  /* 0 bit is reserved for TC Connectivity;
   * 1 bit is reserved for Long Uptime;
   *
   * see ZCL WWAH Cluster Definition,
   * 'Survey Beacons Response' Command, Classification Mask Field;
   */
  ZB_ZDO_SET_TC_CONNECTIVITY_BIT_VALUE(&enbt->u.ext.classification_mask, beacon_payload->tc_connectivity);
  ZB_ZDO_SET_LONG_UPTIME_BIT_VALUE(&enbt->u.ext.classification_mask, beacon_payload->long_uptime);
}


#if TRACE_ENABLED(TRACE_NWK2)
static void print_beacon_payload(zb_mac_beacon_notify_indication_t *ind, zb_mac_beacon_payload_t *beacon_payload, zb_uint8_t beacon_payload_len)
{
  TRACE_MSG(TRACE_NWK2,
            ">>zb_mlme_beacon_notify_indication bsn %hd pandsc { addr_mode %hd panid 0x%x channel %hd superframe{ b.ord %hd sf.ord %hd pan c. %hd ass p. %hd  } } gts_perm %hd link_q %hd sdu_len %hd",
            (FMT__H_H_D_H_H_H_H_H_H_H_H,
             ind->bsn,
             ind->pan_descriptor.coord_addr_mode,
             ind->pan_descriptor.coord_pan_id,
             ind->pan_descriptor.logical_channel,
             ind->pan_descriptor.super_frame_spec.beacon_order,
             ind->pan_descriptor.super_frame_spec.superframe_order,
             ind->pan_descriptor.super_frame_spec.pan_coordinator,
             ind->pan_descriptor.super_frame_spec.associate_permit,
             ind->pan_descriptor.gts_permit,
             ind->pan_descriptor.link_quality,
             ind->sdu_length));

  if (ind->pan_descriptor.coord_addr_mode == ZB_ADDR_16BIT_DEV_OR_BROADCAST)
  {
    TRACE_MSG(TRACE_NWK3, "coord 0x%x", (FMT__D, ind->pan_descriptor.coord_address.addr_short));
  }
  else
  {
    TRACE_MSG(TRACE_NWK3, "coord " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(ind->pan_descriptor.coord_address.addr_long)));
  }

  TRACE_MSG(TRACE_NWK2, "beacon payload %p[%hd]: protocol id %hd, version %hd, stack profile %hd xpanid " TRACE_FORMAT_64,
            (FMT__P_D_H_H_H_A,
             beacon_payload, beacon_payload_len,
             (zb_uint8_t)beacon_payload->protocol_id,
             (zb_uint8_t)beacon_payload->protocol_version,
             (zb_uint8_t)beacon_payload->stack_profile,
             TRACE_ARG_64(beacon_payload->extended_panid)));
}
#else
#define print_beacon_payload(ind, p, p_len) do { (void)ind; (void)p; (void)p_len; } while(0)
#endif /* TRACE_ENABLED(TRACE_NWK2) */


static zb_ret_t iface_find_next_channel_mask(zb_nwk_mac_iface_tbl_ent_t *iface,
                                             zb_channel_list_t scan_channels_list,
                                             zb_uint8_t channel_page_start_idx,
                                             zb_uint8_t *channel_page,
                                             zb_uint32_t *channel_mask)
{
  zb_ret_t ret = RET_NOT_FOUND;
  zb_uint8_t i;
  zb_uint8_t page;
  zb_uint32_t scan_channel_mask, iface_channel_mask;
  zb_channel_list_t supported_channels_list;

  ZB_ASSERT(iface != NULL);
  ZB_ASSERT(channel_page_start_idx < ZB_CHANNEL_PAGES_NUM);
  ZB_ASSERT(channel_page != NULL);
  ZB_ASSERT(channel_mask != NULL);

  TRACE_MSG(TRACE_NWK2, ">> iface_find_next_channel_mask", (FMT__0));

  ZB_MEMCPY((zb_uint8_t *)supported_channels_list, (zb_uint8_t *)iface->supported_channels, sizeof(zb_channel_list_t));

  *channel_page = 0;
  *channel_mask = 0;

  if (ZB_U2B(iface->state))
  {
    for (i = channel_page_start_idx; i < ZB_CHANNEL_PAGES_NUM; i++)
    {
      page = zb_channel_page_list_get_page(supported_channels_list, i);
      iface_channel_mask = zb_channel_page_list_get_mask(supported_channels_list, i);
      scan_channel_mask = zb_channel_page_list_get_mask(scan_channels_list, i);

      TRACE_MSG(TRACE_NWK2, "iface_channel_mask %lx scan_channel_mask %lx", (FMT__L_L, iface_channel_mask, scan_channel_mask));

      if (ZB_U2B(iface_channel_mask & scan_channel_mask))
      {
        *channel_page = page;
        *channel_mask = scan_channel_mask & iface_channel_mask;
        ret = RET_OK;
        break;
      }
    }
  }

  TRACE_MSG(TRACE_NWK2, "<< iface_find_next_channel_mask ret %hd", (FMT__H, ret));
  return ret;
}

zb_ret_t nwk_scan_find_next_channel_mask(zb_uint8_t iface_start_idx,
                                         zb_uint8_t channel_page_start_idx,
                                         zb_channel_list_t scan_channels_list,
                                         zb_uint8_t *iface_idx,
                                         zb_uint8_t *channel_page,
                                         zb_uint32_t *channel_mask)
{
  zb_ret_t ret = RET_NOT_FOUND;
  zb_uint8_t page_idx;
  zb_uint8_t iface_iter_idx;
  zb_nwk_mac_iface_tbl_ent_t *iface;

  TRACE_MSG(TRACE_NWK1, ">> nwk_scan_find_next_channel_mask iface_start_idx %hd", (FMT__H, iface_start_idx));

  /* ZB_ASSERT(iface_start_idx < ZB_NWK_MAC_IFACE_TBL_SIZE); */
  ZB_ASSERT(channel_page_start_idx < ZB_CHANNEL_PAGES_NUM);
  ZB_ASSERT(scan_channels_list != NULL);
  ZB_ASSERT(channel_page != NULL);
  ZB_ASSERT(channel_mask != NULL);

  if (iface_start_idx < ZB_NWK_MAC_IFACE_TBL_SIZE)
  {
    *channel_page = 0;
    *channel_mask = 0;
    if (iface_idx != NULL)
    {
      *iface_idx = 0;
    }

    iface_iter_idx = iface_start_idx;
    page_idx = channel_page_start_idx;

    while (iface_iter_idx < ZB_NWK_MAC_IFACE_TBL_SIZE && ret == RET_NOT_FOUND)
    {
      iface = &ZB_NIB().mac_iface_tbl[iface_iter_idx];
      if (ZB_U2B(iface->state))
      {
        ret = iface_find_next_channel_mask(iface, scan_channels_list, page_idx, channel_page, channel_mask);
        if (ret == RET_OK)
        {
          if (iface_idx != NULL)
          {
            *iface_idx = iface_iter_idx;
          }
        }
        else
        {
          /* Go to the next entry in nwkMacInterfaceTable. */
          ZB_ASSERT(ret == RET_NOT_FOUND);
          iface_iter_idx++;
          page_idx = 0;
        }
      }
      else
      {
        /* Go to the next entry in nwkMacInterfaceTable. */
        ZB_ASSERT(ret == RET_NOT_FOUND);
        iface_iter_idx++;
        page_idx = 0;
      }
    }
  }

  TRACE_MSG(TRACE_NWK2, "<< nwk_scan_find_next_channel_mask ret %hd", (FMT__H, ret));
  return ret;
}

#if defined ZB_ENHANCED_BEACON_SUPPORT
static void prepare_enhanced_beacon_discovery(zb_uint8_t param)
{
  zb_uint8_t *ptr;
  zb_uint8_t pie_len;

  /* MLME PIE with nested IE + EB filter payload
     vendor PIE + tx power descriptor as payload */
  pie_len  = ZB_PIE_HEADER_LENGTH
    + ZB_NIE_HEADER_LENGTH + 1U + ZB_PIE_VENDOR_HEADER_LENGTH
    + ZB_TX_POWER_IE_DESCRIPTOR_LEN;

  ptr = zb_buf_initial_alloc(param,
                             (zb_uint32_t)pie_len + ZB_MLME_SCAN_REQUEST_IE_SIZES_HDR_LEN);
  ZB_MLME_SCAN_REQUEST_SET_IE_SIZES_HDR(ptr, 0U, pie_len);
  ptr += ZB_MLME_SCAN_REQUEST_IE_SIZES_HDR_LEN;

  /* MLME nested PIE */
  ZB_SET_NEXT_PIE_HEADER(ptr, ZB_PIE_GROUP_MLME, ZB_NIE_HEADER_LENGTH + 1U);
  ZB_SET_NEXT_SHORT_NIE_HEADER(ptr, ZB_NIE_SUB_ID_EB_FILTER, 1U);

  *ptr = ZB_EB_FILTER_IE_PERMIT_JOINING_ON; /* simple EB Filter*/
  ptr += 1;

  /* Vendor PIE */
  ZB_SET_NEXT_PIE_ZIGBEE_VENDOR_HEADER(ptr, ZB_TX_POWER_IE_DESCRIPTOR_LEN);
  ZB_SET_NEXT_TX_POWER_IE_DESCRIPTOR(ptr, 0x7FU); /* @TODO: tx power */
  ZVUNUSED(ptr);
}

static void prepare_enhanced_beacon_rejoin(zb_uint8_t param)
{
  /* Vendor PIE + Rejoin IE + Tx Power IE as payload */
  zb_uint8_t pie_len;
  zb_uint8_t *ptr;

  TRACE_MSG(TRACE_NWK1, ">> prepare_enhanced_beacon_rejoin param %hd", (FMT__H, param));

  pie_len = ZB_PIE_VENDOR_HEADER_LENGTH + ZB_TX_POWER_IE_DESCRIPTOR_LEN
    + ZB_ZIGBEE_PIE_HEADER_LENGTH + 8U + 2U;

  ptr = zb_buf_initial_alloc(param,
                             (zb_uint32_t)pie_len + ZB_MLME_SCAN_REQUEST_IE_SIZES_HDR_LEN);
  ZB_MLME_SCAN_REQUEST_SET_IE_SIZES_HDR(ptr, 0U, pie_len);
  ptr += ZB_MLME_SCAN_REQUEST_IE_SIZES_HDR_LEN;

  /* Vendor PIE */
  ZB_SET_NEXT_PIE_ZIGBEE_VENDOR_HEADER(ptr, ZB_TX_POWER_IE_DESCRIPTOR_LEN
                                       + ZB_ZIGBEE_PIE_HEADER_LENGTH + 8U + 2U);

  ZB_SET_NEXT_ZIGBEE_PIE_HEADER(ptr, ZB_ZIGBEE_PIE_SUB_ID_REJOIN, 8U + 2U);
  ZB_HTOLE64(ptr, ZG->nwk.handle.tmp.rejoin.extended_pan_id);
  ptr += 8;
  ZB_PUT_NEXT_HTOLE16(ptr, ZB_PIBCACHE_NETWORK_ADDRESS());

  ZB_SET_NEXT_TX_POWER_IE_DESCRIPTOR(ptr, 0x7FU); /* @TODO: tx power */
  ZVUNUSED(ptr);

  TRACE_MSG(TRACE_NWK1, "<< prepare_enhanced_beacon_rejoin", (FMT__0));
}

#endif /* ZB_ENHANCED_BEACON_SUPPORT */

void nlme_scan_request(zb_uint8_t  param,
                       zb_uint8_t  scan_type,
                       zb_uint8_t  scan_duration,
                       zb_uint8_t  scan_iface_idx,
                       zb_uint8_t  scan_channel_page,
                       zb_uint32_t scan_channel_mask)
{
  TRACE_MSG(TRACE_NWK1, ">>nlme_scan_request scan_type %hd iface_idx %hd",
            (FMT__H_H, scan_type, scan_iface_idx));

  ZB_ASSERT(ZB_U2B(scan_channel_mask));

  TRACE_MSG(TRACE_NWK1, "scan channel page %hd, channels 0x%lx, scan_duration %hd",
            (FMT__H_L_H, scan_channel_page, scan_channel_mask, scan_duration));

#ifdef ZB_RAF_FAST_JOIN_CONFIGURATION
  TRACE_MSG(TRACE_NWK1, "Z< scan channel page %hd, channels 0x%08lx, scan_duration %hd\n",
            (FMT__H_L_H, scan_channel_page, scan_channel_mask, scan_duration));
#endif

  ZG->nwk.handle.scan_iface_idx = scan_iface_idx;

#if defined ZB_ENHANCED_BEACON_SUPPORT
  if (scan_type == ENHANCED_ACTIVE_SCAN)
  {
    prepare_enhanced_beacon_discovery(param);
  }
  if (ZB_LOGICAL_PAGE_IS_SUB_GHZ(scan_channel_page))
  {
    /* Increase scan_duration for EU FSK bands */
    scan_duration = ZB_DEFAULT_SCAN_DURATION_SUB_GHZ;

    if (scan_type == ACTIVE_SCAN)
    {
      if (ZG->nwk.handle.state == ZB_NLME_STATE_DISC)
      {
        scan_type = ENHANCED_ACTIVE_SCAN;
        prepare_enhanced_beacon_discovery(param);
      }
      else if (ZG->nwk.handle.state == ZB_NLME_STATE_REJOIN)
      {
        scan_type = ENHANCED_ACTIVE_SCAN;
        prepare_enhanced_beacon_rejoin(param);
      }
      else
      {
        /* MISRA rule 15.7 requires empty 'else' branch. */
      }
    }
  }
#endif /* ZB_ENHANCED_BEACON_SUPPORT */

  ZG->nwk.handle.scan_duration = scan_duration;

  /* or to keep ZB_MLME_BUILD_SCAN_REQUEST as a macro: used only here and in tests */
  ZB_MLME_BUILD_SCAN_REQUEST(param, scan_channel_page,
                             scan_channel_mask, scan_type, scan_duration);

  ZB_SCHEDULE_CALLBACK(zb_mlme_scan_request, param);
}

#ifdef ZB_JOIN_CLIENT
void zb_nlme_network_discovery_request(zb_uint8_t param)
{
  zb_ret_t ret;
  zb_uint8_t scan_iface_idx;
  zb_uint8_t channel_page;
  zb_uint32_t channel_mask;
  zb_nlme_network_discovery_request_t *request = ZB_BUF_GET_PARAM(param, zb_nlme_network_discovery_request_t);

  TRACE_MSG(TRACE_NWK1, ">>zb_nlme_network_discovery_request %p", (FMT__P, request));
  CHECK_PARAM_RET_ON_ERROR(request);

  /*
    see 3.6.1.3  Network Discovery
    Can be run before or after join. If run before join, fills neighbor table
    (both its "main" and "additional" parts.
    If run after join - not sure yet. When could it be used?


    Calls sequence:
    > NETWORK-DISCOVERY.request
      > MLME-SCAN.request
      < MLME-BEACON-INDICATION.indication
      < MLME-BEACON-INDICATION.indication
      ...
      < MLME-SCAN.confirm
    < NLME-NETWORK-DISCOVERY.confirm
   */

  if ( ZG->nwk.handle.state == ZB_NLME_STATE_IDLE )
  {
    /* Don't forget to call zb_nwk_exneighbor_stop() after successful join (not
     * discovery! ext neighbor used for potential parent search). */

    /* CR:MAJOR: MM: Need to handle it correcly, because it looks
     * very inconsistent in main development branch.
     * EE: Kill extneighbor and put all into just neighbor table? :)
     */

    /* NK:WARNING: Dirty hack: reset ZG->addr before new scan (to prevent dev_manufacturer overflow).
       TODO: Implement correct reusing for dev_manufacturer table and correct cleanup for addr_map/short_sorted
       unused elements. */
#if 0
    /* EE: causing assert at second join attempt because
     * panid ref exists in ext neighbor but panid in use bit is not set in the address. */
    ZB_BZERO(&ZG->addr, sizeof(zb_addr_globals_t));
#endif
    /* Start discovery using the first supported page. */
    ret = nwk_scan_find_next_channel_mask(0, 0, request->scan_channels_list, &scan_iface_idx,
                                          &channel_page, &channel_mask);
    if (ret == RET_OK)
    {
      /* Make a copy to use it for further scan request sequence. */
      zb_channel_page_list_copy(ZG->nwk.handle.scan_channels_list, request->scan_channels_list);
      ZG->nwk.handle.state = ZB_NLME_STATE_DISC;
      nlme_scan_request(param, ACTIVE_SCAN, request->scan_duration,
                        scan_iface_idx, channel_page, channel_mask);
    }
    else
    {
      zb_nlme_network_discovery_confirm_t *discovery_confirm;

      TRACE_MSG(TRACE_NWK1, "Could not get channels mask to start discovery!", (FMT__0));
      discovery_confirm= zb_buf_initial_alloc(param,
                                              sizeof(*discovery_confirm));
      discovery_confirm->status = RET_INVALID_PARAMETER;
      ZB_SCHEDULE_CALLBACK(zb_nlme_network_discovery_confirm, param);
    }
  }
  else
  {
    zb_nlme_network_discovery_confirm_t *discovery_confirm;
    TRACE_MSG(TRACE_NWK1, "nwk is busy, state %d", (FMT__D, ZG->nwk.handle.state));
    discovery_confirm = zb_buf_initial_alloc(param,
                                             sizeof(*discovery_confirm));
    discovery_confirm->status = RET_BUSY;
    ZB_SCHEDULE_CALLBACK(zb_nlme_network_discovery_confirm, param);
  }

  TRACE_MSG(TRACE_NWK1, "<<zb_nlme_network_discovery_request", (FMT__0));
}

#endif  /* ZB_JOIN_CLIENT */


/* ED Scan functions were under ZB_ROUTER_ROLE ifdef,
 * but were switched on for all devices types (for WWAH, PICS item AZD514) */

static void call_ed_scan_confirm(zb_bufid_t param, zb_nwk_status_t status)
{
  zb_buf_set_status(param, ERROR_CODE(ERROR_CATEGORY_NWK, status));
  ZB_SCHEDULE_CALLBACK(zb_nlme_ed_scan_confirm, param);
}


void zb_nlme_ed_scan_request(zb_uint8_t param)
{
  zb_ret_t ret;
  zb_uint8_t scan_iface_idx;
  zb_uint8_t channel_page;
  zb_uint32_t channel_mask;
  zb_nlme_ed_scan_request_t *request = ZB_BUF_GET_PARAM(param, zb_nlme_ed_scan_request_t);

  TRACE_MSG(TRACE_NWK1, ">>zb_nlme_ed_scan_request %p", (FMT__P, request));
  CHECK_PARAM_RET_ON_ERROR(request);

  if (ZB_NIB_DEVICE_TYPE() != ZB_NWK_DEVICE_TYPE_NONE )
  {
    if ( ZG->nwk.handle.state == ZB_NLME_STATE_IDLE )
    {
      /* Start discovery using the first supported page. */
      ret = nwk_scan_find_next_channel_mask(0, 0, request->scan_channels_list, &scan_iface_idx,
                                            &channel_page, &channel_mask);
      if (ret == RET_OK)
      {
        ZB_ASSERT(ZG->nwk.handle.ed_list_param == 0U);
        /* Make a copy to use it for further scan request sequence. */
        zb_channel_page_list_copy(ZG->nwk.handle.scan_channels_list, request->scan_channels_list);
        ZG->nwk.handle.state = ZB_NLME_STATE_ED_SCAN;
        nlme_scan_request(param, ED_SCAN, request->scan_duration,
                          scan_iface_idx, channel_page, channel_mask);
      }
      else
      {
        call_ed_scan_confirm(param, ZB_NWK_STATUS_INVALID_PARAMETER);
      }
    }
    else
    {
      TRACE_MSG(TRACE_NWK1, "nwk is busy, state %d", (FMT__D, ZG->nwk.handle.state));
      call_ed_scan_confirm(param, ZB_NWK_STATUS_INVALID_REQUEST);
    }
  }
  else
  {
    call_ed_scan_confirm(param, ZB_NWK_STATUS_INVALID_REQUEST);
  }

  TRACE_MSG(TRACE_NWK1, "<<zb_nlme_ed_scan_request", (FMT__0));
}


void zb_nlme_beacon_survey_scan(zb_uint8_t param)
{
  zb_nlme_beacon_survey_scan_request_t *req =
    ZB_BUF_GET_PARAM(param, zb_nlme_beacon_survey_scan_request_t);

  TRACE_MSG(
    TRACE_NWK1,
    ">> zb_nlme_beacon_survey_scan, param %hd, "
    "channel_mask 0x%lx, channel_page %hd, scan_type %hd",
    (FMT__H_L_H_H, param, req->channel_mask, req->channel_page, req->scan_type));


  nlme_scan_request(
    param,
    req->scan_type,
    ZB_DEFAULT_SCAN_DURATION,
    0, /* scan_iface_idx */
    req->channel_page,
    req->channel_mask);

  TRACE_MSG(TRACE_NWK1, "<< zb_nlme_beacon_survey_scan", (FMT__0));
}


void zb_mlme_beacon_notify_indication(zb_uint8_t param)
{
  zb_mac_beacon_notify_indication_t *ind = (zb_mac_beacon_notify_indication_t *)zb_buf_begin(param);
  zb_mac_beacon_payload_t *beacon_payload = NULL;
  zb_uint8_t beacon_payload_len = 0;
  zb_ret_t ret = RET_OK;
  zb_bool_t is_joined = ZB_JOINED();

  if (ind->beacon_type == ZB_MAC_BEACON_TYPE_BEACON)
  {
    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,25} */
    beacon_payload = (zb_mac_beacon_payload_t *)ind->sdu;
    beacon_payload_len = ind->sdu_length;
  }
  /* TODO: divide that huge routine by functions!
   *
   * Guys, when you need to insert somewhere you code to parse some
   * funny packet introduced in the spec v123, rethink: it is proper
   * place to put your parse into? Maybe, it sis better to parse in
   * the separate function and insert here a cell?? EE. */
#if defined ZB_ENHANCED_BEACON_SUPPORT
  else if (ind->beacon_type == ZB_MAC_BEACON_TYPE_ENHANCED_BEACON)
  {
    zb_uint8_t *ptr, *ptr_end;
    zb_pie_header_t pie_header;
    zb_zigbee_vendor_pie_parsed_t vendor_parsed;

    ZB_MEMSET(&vendor_parsed, 0, sizeof(zb_zigbee_vendor_pie_parsed_t));

    /* need to parse PIEs, we can assume that IEs are correct */
    ptr = ind->sdu  + ind->total_hie_size;
    ptr_end = ind->sdu + ind->total_hie_size + ind->total_pie_size;

    while (ptr < ptr_end)
    {
      ZB_GET_NEXT_PIE_HEADER(ptr, &pie_header);

      /* interested in Zigbee payload IE only  */
      if (pie_header.group_id == ZB_PIE_GROUP_VENDOR_SPECIFIC)
      {
        (void)zb_parse_zigbee_vendor_pie(ptr, pie_header.length, &vendor_parsed);
      }

      ptr += pie_header.length;
    }

    if (!vendor_parsed.eb_payload_set)
    {
      TRACE_MSG(TRACE_NWK1, "no eb payload found", (FMT__0));
      ret = RET_IGNORE;
    }
    else
    {
      void *p = (void *)vendor_parsed.eb_payload.beacon_payload_ptr;
      beacon_payload = (zb_mac_beacon_payload_t *)p;
      beacon_payload_len = vendor_parsed.eb_payload.beacon_payload_len;
    }
  }
#endif /* ZB_ENHANCED_BEACON_SUPPORT */
  else
  {
    ret = RET_IGNORE;
    TRACE_MSG(TRACE_NWK1, "unsupported beacon type", (FMT__0));
  }

#ifdef ZB_MAC_TESTING_MODE
  {
    TRACE_MSG(TRACE_NWK1, "beacon payload dump", (FMT__0));
    dump_traf((zb_uint8_t*)beacon_payload, beacon_payload_len);
  }
#else  /* ZB_MAC_TESTING_MODE */

  if (ret != RET_OK)
  {
    TRACE_MSG(TRACE_NWK1, "skipping beacon because of errors", (FMT__0));
  }
  else if ( ZG->nwk.handle.state == ZB_NLME_STATE_DISC
            || ZG->nwk.handle.state == ZB_NLME_STATE_REJOIN )
  {
    /*
      Fill neighbor tables here (both base and additional parts).
      Note that additional part has some fields which used to construct
      NetworkDescriptor for the NLME-NETWORK-DISCOVERY.confirm.
      MAC does not keep PAN descriptors table: MAC_PIB().mac_auto_request is
      FALSE; pan desc table not used in PAN and it is
      simpler to us to not analyze it.
      Instead put additional fields into additional neighbor table entry.
    */

    /* Analyze beacon payload - see 3.6.7 */
    if (beacon_payload_len > 0U)
    {
      print_beacon_payload(ind, beacon_payload, beacon_payload_len);
    }

    /* For discovery state, ignore the incoming beacon with poor link quality OR
     * with closed PAN (associate_permit == 0) */
    if (ZG->nwk.handle.state == ZB_NLME_STATE_DISC &&
        !(ZB_LINK_QUALITY_IS_OK_FOR_JOIN(ind->pan_descriptor.link_quality) &&
          ZB_U2B((zb_uint8_t)ind->pan_descriptor.super_frame_spec.associate_permit)))
    {
      TRACE_MSG(TRACE_NWK3, "Ignore this beacon : link_quality %d ass perm %d", (FMT__D_D, ind->pan_descriptor.link_quality, ind->pan_descriptor.super_frame_spec.associate_permit));
      ret = RET_IGNORE;
    }

    /*
      Check for "not our" nets. Not sure it is really necessary.
      Also not sure do we need to ignore beacons without payload?
      Seems yes: we returns extended panid only in MLME-SCAN.confirm and it can
      be get from the beacon payload only.
      According to bdb test cn-nsa-tc-02 we should not reject beacons
      with payload length > sizeof(zb_mac_beacon_payload_t) - 15 octets.
    */

    if ( ret == RET_OK &&
         (beacon_payload_len >= sizeof(zb_mac_beacon_payload_t))
         && (beacon_payload->protocol_id == 0U)
         && (beacon_payload->protocol_version == ZB_PROTOCOL_VERSION)
         /* [VP]: added STACK_NETWORK_SELECT in condition.
            There can be STACK_NETWORK_SELECT. Ember likes it.
            Not sure this is legal, but kind of interoperability issue */
         && ((beacon_payload->stack_profile == STACK_2007) ||
             (beacon_payload->stack_profile == STACK_PRO)
#ifndef ZB_CERTIFICATION_HACKS
             /* CN-NSA-TC-02 prohibites all profiles except STACK_2007 and STACK_PRO,
                but STACK_NETWORK_SELECT can be used in real life - so permit
                STACK_NETWORK_SELECT for non-certification mode. */
             || (beacon_payload->stack_profile == STACK_NETWORK_SELECT)
#endif
           )
      )
    {
      TRACE_MSG(TRACE_NWK1, "proccess in-beacon. State %hd", (FMT__H, ZG->nwk.handle.state));
      if ( (ZG->nwk.handle.state != ZB_NLME_STATE_REJOIN)
           || ZB_64BIT_ADDR_CMP(beacon_payload->extended_panid, ZG->nwk.handle.tmp.rejoin.extended_pan_id))
        /* In rejoin state we update devices only from defined ExtendedPanId */
      {
        zb_ext_neighbor_tbl_ent_t *enbt = NULL; /* shutup sdcc */
        zb_address_pan_id_ref_t panid_ref = 0;

#ifdef ZB_NWK_BLACKLIST
        /* for safety it's better to add check here
           if (ZG->nwk.handle.state != ZB_NLME_STATE_REJOIN)
           check it, add check if needed */
        /* OK. Double check that we are not in rejoin state before blacklisting (upper
         * if-condition allows to be here in rejoin state). */
        if (ZG->nwk.handle.state != ZB_NLME_STATE_REJOIN)
        {
          ret = zb_nwk_blacklist_check(beacon_payload->extended_panid);
          if (ret == RET_ERROR)
          {
            ret = RET_IGNORE;
          }
        }
#endif
        if (ret == RET_OK)
        {
          /* remember extended pan id */
          ret = zb_address_set_pan_id(ind->pan_descriptor.coord_pan_id, beacon_payload->extended_panid, &panid_ref);
        }

        if (ret == RET_ALREADY_EXISTS)
        {
          ret = RET_OK;
        }
#ifdef ZB_JOIN_CLIENT
        else if (ret == RET_OVERFLOW)
        {
          /* No room in PAN ID table: search entry to delete. Find the
           * weakest PAN ID */
          zb_uint8_t min_lqi;

          TRACE_MSG(TRACE_NWK1, "PANID table is full, try to reuse", (FMT__0));

          ret = get_weakest_pan_to_remove(&min_lqi, &panid_ref);
          if (ret == RET_OK)
          {
            TRACE_MSG(TRACE_NWK2, "min_lqi %hd beacon_lqi %hd",
                      (FMT__H_H, min_lqi, ind->pan_descriptor.link_quality));

            if (ind->pan_descriptor.link_quality > min_lqi)
            {
              /* Remove all extneighbors, who refers to the PAN ID ref,
               * as far as we are going to reuse it */
              TRACE_MSG(TRACE_NWK2, "Accept new beacon, reuse panid_ref %hd", (FMT__H, panid_ref));

              ret = zb_nwk_exneighbor_remove_by_panid(panid_ref);
              ZB_ASSERT(ret == RET_OK);

              ret = zb_address_reuse_pan_id(ind->pan_descriptor.coord_pan_id, beacon_payload->extended_panid, panid_ref);
              ZB_ASSERT(ret == RET_OK);
            }
            else
            {
              ret = RET_IGNORE;
            }
          }
        }
#endif  /* ZB_JOIN_CLIENT */

        /* Fill or update extended neighbor table */
        /* First get address from the MAC header */
        if (ret == RET_OK)
        {
          /* Not sure: can beacon use 64-bit address? Let's handle it anyway */
          if (ind->pan_descriptor.coord_addr_mode == ZB_ADDR_16BIT_DEV_OR_BROADCAST)
          {
            /* 16 bit address. */
            ret = zb_nwk_exneighbor_by_short(panid_ref, ind->pan_descriptor.coord_address.addr_short, &enbt);
          }
#if defined ZB_ENHANCED_BEACON_SUPPORT
          else if (ind->beacon_type == ZB_MAC_BEACON_TYPE_ENHANCED_BEACON)
          {
            /* enhanced beacons are sent from ieee address, short address is stored in EB payload */
            ret = zb_nwk_exneighbor_by_short(panid_ref, ind->pan_descriptor.enh_beacon_nwk_addr, &enbt);
          }
#endif /* ZB_ENHANCED_BEACON_SUPPORT */
          else
          {
            ret = zb_nwk_exneighbor_by_ieee(panid_ref, ind->pan_descriptor.coord_address.addr_long, &enbt);
          }

          if (ret == RET_NO_MEMORY)
          {
            /* No space in the ext neighbor table - find and reuse the
             * weakest neighbor */
            zb_uindex_t i;
            zb_uint8_t min_lqi = 0xff; /* ZG->nwk.neighbor.neighbor[0].lqi; */

            TRACE_MSG(TRACE_NWK1, "nbt is full, try to reuse the weakest nb", (FMT__0));
            for (i = EXN_START_I; i < ZB_NEIGHBOR_TABLE_SIZE ; i++)
            {
#ifdef ZB_NWK_BLACKLIST
              /* We're not allowed to reuse/remove ext_neighbor devices
               * who referes to blacklisted PAN */
              {
                zb_ext_pan_id_t ext_pan_id;
                zb_address_get_pan_id(ZG->nwk.neighbor.neighbor[i].u.ext.panid_ref, ext_pan_id);
                ret = zb_nwk_blacklist_check(ext_pan_id);
                if (ret != RET_OK)
                {
                  continue;
                }
              }
#endif

              if (ZB_U2B(ZG->nwk.neighbor.neighbor[i].used)
                  && ZB_U2B(ZG->nwk.neighbor.neighbor[i].ext_neighbor)
                  && (min_lqi > ZG->nwk.neighbor.neighbor[i].lqi))
              {
                min_lqi = ZG->nwk.neighbor.neighbor[i].lqi;
                enbt = &ZG->nwk.neighbor.neighbor[i];
              }
            }

            if (enbt != NULL)
            {
              if (ind->pan_descriptor.link_quality > min_lqi)
              {
                zb_uint8_t n_refs = 0;
                TRACE_MSG(TRACE_NWK2, "Will reuse extneighbor %p with lqi %hd", (FMT__P_H, enbt, min_lqi));

                /* Check if it's unique entry with its panid_ref */
                for (i = 0; i < ZB_NEIGHBOR_TABLE_SIZE; i++)
                {
                  if (ZB_U2B(ZG->nwk.neighbor.neighbor[i].used)
                      && ZB_U2B(ZG->nwk.neighbor.neighbor[i].ext_neighbor)
                      && (ZG->nwk.neighbor.neighbor[i].u.ext.panid_ref == enbt->u.ext.panid_ref)
                      && &ZG->nwk.neighbor.neighbor[i] != enbt)
                  {
                    n_refs++;
                  }
                }

                /* Clear unique entry in PANID table */
                if (!ZB_U2B(n_refs))
                {
                  TRACE_MSG(TRACE_NWK2, "Unique reference, will delete panid entry %hd", (FMT__H, enbt->u.ext.panid_ref));
                  ret = zb_address_delete_pan_id(enbt->u.ext.panid_ref);
                  ZB_ASSERT(ret == RET_OK);
                }

                ZB_BZERO(enbt, sizeof(zb_ext_neighbor_tbl_ent_t));
                if (ind->pan_descriptor.coord_addr_mode == ZB_ADDR_16BIT_DEV_OR_BROADCAST)
                {
                  /* 16 bit address. */
                  enbt->u.ext.short_addr = ind->pan_descriptor.coord_address.addr_short;
                }
                else
                {
                  zb_ieee_addr_compress(ind->pan_descriptor.coord_address.addr_long, &(enbt->u.ext.long_addr));
                }
                ret = RET_OK;
              }
              else
              {
                ret = RET_IGNORE;
              }
            }
            else
            {
              ret = RET_NO_MEMORY; /* No place found in nbt */
            }
          }
        }

        if (ret == RET_OK)
        {
          convert_beacon_to_exneighbor(enbt, panid_ref, ind, beacon_payload);
        } /* if ok */
      } /* if this beacon could be interesting for us */
      else
      {
        TRACE_MSG(TRACE_NWK2, "Skip beacon not for us: its protocol id %hd, version %hd, stack profile %hd",
                  (FMT__H_H_H,
                   beacon_payload->protocol_id,
                   beacon_payload->protocol_version,
                   beacon_payload->stack_profile));
      }
    }
    else
    {
      TRACE_MSG(TRACE_NWK1, "Drop in-beacon!", (FMT__0));
    }
  }
#ifdef ZB_ROUTER_ROLE
  else if (ZB_IS_DEVICE_ZC_OR_ZR() && is_joined)
  {
#ifndef ZB_LITE_NO_PANID_CONFLICT_DETECTION
    if (ZG->nwk.selector.panid_conflict_in_beacon != NULL)
    {
      (*ZG->nwk.selector.panid_conflict_in_beacon)(param);
      param = 0;
    }
#endif  /* #ifndef ZB_LITE_NO_PANID_CONFLICT_DETECTION */
  }
  else if (ZG->nwk.handle.state == ZB_NLME_STATE_FORMATION_ACTIVE_SCAN)
  {
    zb_address_pan_id_ref_t panid_ref;
    zb_ext_neighbor_tbl_ent_t *enbt = NULL; /* shutup sdcc */
    zb_uint16_t coord_pan_id = ind->pan_descriptor.coord_pan_id;

    print_beacon_payload(ind, beacon_payload, beacon_payload_len);

    /* Add only unique PAN id's in table, if no memory for new PAN id - drop it */
    ret = zb_address_set_pan_id(coord_pan_id, beacon_payload->extended_panid, &panid_ref);
    if (ret == RET_ALREADY_EXISTS)
    {
      TRACE_MSG(TRACE_NWK1, "pan 0x%x already exist", (FMT__H, coord_pan_id));
    }
    else if (ret == RET_OVERFLOW)
    {
      TRACE_MSG(TRACE_NWK1, "No room for pan 0x%x - drop incoming beacon", (FMT__H, coord_pan_id));
    }
    else
    {
      TRACE_MSG(TRACE_NWK1, "Add new pan id in list -  0x%x", (FMT__H, coord_pan_id));
    }

    /* For each PAN id - add only one device from this PAN to exneighbors - to count number of networks on channel */
    if (ret == RET_OK)
    {
      if (ind->pan_descriptor.coord_addr_mode == ZB_ADDR_16BIT_DEV_OR_BROADCAST)
      {
        /* 16 bit address. */
        ret = zb_nwk_exneighbor_by_short(panid_ref, ind->pan_descriptor.coord_address.addr_short, &enbt);
      }

#if defined ZB_ENHANCED_BEACON_SUPPORT
      else if (ind->beacon_type == ZB_MAC_BEACON_TYPE_ENHANCED_BEACON)
      {
        /* enhanced beacons are sent from ieee address, short address is stored in EB payload */
        ret = zb_nwk_exneighbor_by_short(panid_ref, ind->pan_descriptor.enh_beacon_nwk_addr, &enbt);
      }
#endif /* ZB_ENHANCED_BEACON_SUPPORT */
      else
      {
        ret = zb_nwk_exneighbor_by_ieee(panid_ref, ind->pan_descriptor.coord_address.addr_long, &enbt);
      }
      if (ret == RET_NO_MEMORY)
      {
        TRACE_MSG(TRACE_NWK1, "No room for new exneighbor - drop incoming beacon", (FMT__0));
      }
      else
      {
        convert_beacon_to_exneighbor(enbt, panid_ref, ind, beacon_payload);
      }
    }
  }
#endif /* ZB_ROUTER_ROLE */
#if defined ZB_BEACON_SURVEY && defined ZB_ZCL_ENABLE_WWAH_SERVER
  else if (ZB_NLME_STATE_SURVEY_BEACON == ZG->nwk.handle.state)
  {
    zdo_wwah_process_beacon_info(ind, beacon_payload);
  }
#endif
  else
  {
    TRACE_MSG(TRACE_NWK1, "NWK state is %d - not ZB_NLME_STATE_DISC - ignore beacon", (FMT__D, ZG->nwk.handle.state));
  }

  if (ret == RET_OK && is_joined && ZG->nwk.handle.state != ZB_NLME_STATE_REJOIN)
  {
    /*  if it is our neighbor, update Depth and Permit joining. Note: neighbor table can be used is we joined only. */
    zb_neighbor_tbl_ent_t *nbt;

    if (ind->pan_descriptor.coord_addr_mode == ZB_ADDR_16BIT_DEV_OR_BROADCAST)
    {
      /* 16 bit address. */
      ret = zb_nwk_neighbor_get_by_short(ind->pan_descriptor.coord_address.addr_short, &nbt);
    }
    else
    {
      ret = zb_nwk_neighbor_get_by_ieee(ind->pan_descriptor.coord_address.addr_long, &nbt);
    }
    if (ret == RET_OK)
    {
      nbt->depth = beacon_payload->device_depth;
      nbt->permit_joining = ind->pan_descriptor.super_frame_spec.associate_permit;
    }
  }

  if (param != 0U)
  {
    zb_buf_free(param);
  }
#endif /* else ZB_MAC_TESTING_MODE */

  TRACE_MSG(TRACE_NWK1, "<<zb_mlme_beacon_notify_indication", (FMT__0));
}

#if defined ZB_ENHANCED_BEACON_SUPPORT

static zb_bool_t nwk_check_scan_continue(zb_mac_scan_confirm_t *scan_confirm,
                                  zb_uint8_t *iface_idx,
                                  zb_uint8_t *channel_page,
                                  zb_uint32_t *channel_mask)
{
  zb_ret_t ret;
  zb_bool_t contin = ZB_FALSE;
  zb_uint8_t iface_start_idx;
  zb_uint8_t page_idx;

  ZB_ASSERT(scan_confirm != NULL);
  ZB_ASSERT(channel_page != NULL);
  ZB_ASSERT(channel_mask != NULL);

  *channel_page = 0;
  *channel_mask = 0;
  if (iface_idx != NULL)
  {
    *iface_idx = 0;
  }

  iface_start_idx = ZG->nwk.handle.scan_iface_idx;
  ZB_ASSERT(iface_start_idx < ZB_NWK_MAC_IFACE_TBL_SIZE);

  ret = zb_channel_page_list_get_page_idx(scan_confirm->channel_page, &page_idx);
  /* Check the logic if this assertion failed, all this code is under our control. */
  ZB_ASSERT(ret == RET_OK);

  if (page_idx < ZB_CHANNEL_PAGES_NUM - 1U)
  {
    page_idx++;
  }
  else
  {
    /* Go to the next entry in nwkMacInterfaceTable. */
    iface_start_idx++;
    page_idx = 0;
  }

  ret = nwk_scan_find_next_channel_mask(iface_start_idx, page_idx, ZG->nwk.handle.scan_channels_list,
                                        iface_idx, channel_page, channel_mask);
  ZB_ASSERT(ret == RET_OK || ret == RET_NOT_FOUND);
  if (ret == RET_OK)
  {
    ZB_ASSERT(*channel_mask != 0U);
    contin = ZB_TRUE;
  }

  TRACE_MSG(TRACE_NWK1, "nwk_check_scan_continue: %hd", (FMT__H, contin));

  return contin;
}

#endif /* !ZB_ENHANCED_BEACON_SUPPORT */

#ifdef ZB_ROUTER_ROLE
static void ed_scan_assemble_result(zb_uint8_t ed_list_param, zb_uint16_t user_param)
{
  zb_ret_t ret;
  zb_uint8_t j;
  zb_energy_detect_list_t *ed_list;
  zb_energy_detect_channel_info_t *channel_info;
  zb_mac_scan_confirm_t *scan_confirm;
  zb_uint8_t channel_number;
  zb_uint8_t start_channel_number;
  zb_uint8_t max_channel_number;
  zb_uint32_t scan_channel_mask;
  zb_uint32_t scanned_channels;
  zb_uint8_t channel_page_idx;

  TRACE_MSG(TRACE_NWK1, ">> ed_scan_assemble_result ed_list_param %hd user_param %hd",
            (FMT__H_H, ed_list_param, user_param));

  if (ed_list_param == 0U)
  {
    /* TODO: refactor zb_mlme_scan_confirm to be able to block here in alloc */
    ed_list_param = zb_buf_get_out();
    ZB_ASSERT(ed_list_param);
    (void)zb_buf_reuse(ed_list_param);
  }

  ed_list = ZB_BUF_GET_PARAM(ed_list_param, zb_energy_detect_list_t);
  scan_confirm = ZB_BUF_GET_PARAM((zb_uint8_t)user_param, zb_mac_scan_confirm_t);

  if (ZG->nwk.handle.ed_list_param == 0U)
  {
    ZB_BZERO(ed_list, sizeof(ed_list));
    ZG->nwk.handle.ed_list_param = ed_list_param;
  }

  /* Determine mask of scanned channels. */
  ret = zb_channel_page_list_get_page_idx(scan_confirm->channel_page, &channel_page_idx);
  ZB_ASSERT(ret == RET_OK);
  scan_channel_mask = zb_channel_page_list_get_mask(ZG->nwk.handle.scan_channels_list, channel_page_idx);
  scanned_channels = scan_channel_mask & ~scan_confirm->unscanned_channels;

  /* Determine start and max channel numbers to iterate between them. */
  ret = zb_channel_page_get_start_channel_number(scan_confirm->channel_page, &start_channel_number);
  ZB_ASSERT(ret == RET_OK);
  ret = zb_channel_page_get_max_channel_number(scan_confirm->channel_page, &max_channel_number);
  ZB_ASSERT(ret == RET_OK);
  ZVUNUSED(ret);

  j = 0;
  for (channel_number = start_channel_number; channel_number <= max_channel_number; channel_number++)
  {
    if (ZB_U2B(scanned_channels & (1UL << channel_number)))
    {
      /* Record ED info. */
      ZB_ASSERT(ed_list->channel_count < ZB_ED_SCAN_MAX_CHANNELS_COUNT);
      channel_info = &ed_list->channel_info[ed_list->channel_count];
      channel_info->channel_page_idx = channel_page_idx;
      channel_info->channel_number = channel_number;
      channel_info->energy_detected = scan_confirm->list.energy_detect[j];
      ed_list->channel_count++;
      TRACE_MSG(TRACE_NWK1, "add rec: page_idx %hd channel_number %hd energy %hd ", (FMT__H_H_H, channel_info->channel_page_idx, channel_info->channel_number, channel_info->energy_detected));
      j++;
    }
  }
  TRACE_MSG(TRACE_NWK1, "<< ed_scan_assemble_result %hd", (FMT__H, ed_list->channel_count));
}
#endif  /* ZB_ROUTER_ROLE */


void zb_mlme_scan_confirm(zb_uint8_t param)
{
  zb_ret_t ret = RET_OK;
  zb_mac_scan_confirm_t *scan_confirm;
#if defined ZB_ENHANCED_BEACON_SUPPORT
  zb_uint8_t iface_idx;
  zb_uint8_t channel_page;
  zb_uint32_t channel_mask;
#endif

  TRACE_MSG(TRACE_NWK1, ">>zb_mlme_scan_confirm %hd", (FMT__H, param));
  TRACE_MSG(TRACE_NWK1, "nwk state %d", (FMT__D, (int)ZG->nwk.handle.state));

#ifdef ZB_COORDINATOR_ROLE
  zb_nwk_formation_force_link();
#endif

#ifdef ZB_USE_ZB_TRAFFIC_WATCHDOG
  /* We successfully sent scan_request */
  ZB_KICK_WATCHDOG(ZB_WD_ZB_TRAFFIC);
#endif

  scan_confirm = ZB_BUF_GET_PARAM(param, zb_mac_scan_confirm_t);
  ZVUNUSED(scan_confirm);

  if (
#ifdef ZB_JOIN_CLIENT
    ZG->nwk.handle.state == ZB_NLME_STATE_DISC ||
    ZG->nwk.handle.state == ZB_NLME_STATE_REJOIN ||
#endif
#ifdef ZB_FORMATION
    ZG->nwk.handle.state == ZB_NLME_STATE_FORMATION_ACTIVE_SCAN ||
#endif
#ifndef ZB_LITE_NO_ORPHAN_SCAN
    ZG->nwk.handle.state == ZB_NLME_STATE_ORPHAN_SCAN ||
#endif
    ZB_FALSE)
  {
#ifndef ZB_LITE_NO_ORPHAN_SCAN
    if (!(ZG->nwk.handle.state == ZB_NLME_STATE_ORPHAN_SCAN &&
          scan_confirm->status == MAC_SUCCESS))
#endif
    {
#if defined ZB_ENHANCED_BEACON_SUPPORT
      if (ZB_TRUE == nwk_check_scan_continue(scan_confirm, &iface_idx, &channel_page, &channel_mask))
      {
        /* There are channels on other pages/interfaces to scan. Initiate the next discovery request. */
        nlme_scan_request(param, ZG->nwk.handle.state == ZB_NLME_STATE_ORPHAN_SCAN ? ORPHAN_SCAN : ACTIVE_SCAN,
                          ZG->nwk.handle.scan_duration, iface_idx, channel_page, channel_mask);
        ret = RET_ERROR;
      }
#endif /* ZB_ENHANCED_BEACON_SUPPORT */
    }
  }

#ifdef ZB_JOIN_CLIENT
  if ( RET_OK == ret && ZG->nwk.handle.state == ZB_NLME_STATE_DISC )
  {
    /* scan ext neighbor table created in zb_mlme_beacon_notify_indication() - create Network descriptor  */
    zb_nlme_network_discovery_confirm_t *discovery_confirm;
    zb_nlme_network_descriptor_t *network_descriptor;
    zb_ushort_t i, j;
    zb_ushort_t n_nwk_dsc = 0;
    zb_uint8_t dev_type_cmp = 2;

    /* Moved multiple scan attempts logic to zdo_app.c: this is ZDO-level logic. */
#ifdef ZB_MAC_TESTING_MODE
    /* Do not remove access to MAC_PIB here: this code is just for testing of
     * MAC layer features not used in normal ZB */
    TRACE_MSG(TRACE_NWK3, "scan type %hd, status %hd, auto_req %hd",
              (FMT__H_H_H, scan_confirm->scan_type, scan_confirm->status, MAC_PIB().mac_auto_request));
    if (scan_confirm->scan_type == ACTIVE_SCAN && scan_confirm->status == MAC_SUCCESS && MAC_PIB().mac_auto_request)
    {
      zb_pan_descriptor_t *pan_desc;
      pan_desc = (zb_pan_descriptor_t*)zb_buf_begin(param);
      TRACE_MSG(TRACE_NWK3, "ative scan res count %hd", (FMT__H, scan_confirm->result_list_size));
      for(i = 0; i < scan_confirm->result_list_size; i++)
      {
        TRACE_MSG(TRACE_NWK3,
                  "pan desc: coord addr mode %hd, coord addr %x, pan id %x, channel %hd, superfame %x, lqi %hx",
                  (FMT__H_D_D_H_D_H, pan_desc->coord_addr_mode, pan_desc->coord_address.addr_short, pan_desc->coord_pan_id,
                   pan_desc->logical_channel, pan_desc->super_frame_spec, pan_desc->link_quality));

        if (pan_desc->coord_addr_mode == ZB_ADDR_64BIT_DEV)
        {
          TRACE_MSG(TRACE_MAC3, "Extended coord addr " TRACE_FORMAT_64,
                    (FMT__A, TRACE_ARG_64(pan_desc->coord_address.addr_long)));
        }

        pan_desc++;
      }
    }
#endif  /* ZB_MAC_TESTING_MODE */

    /* zb_nwk_exneigbor_sort_by_lqi always return RET_OK */
    (void)zb_nwk_exneigbor_sort_by_lqi();

    /* There was u.hdr.status save and restore. Seems, we do not need it. */
    discovery_confirm = zb_buf_initial_alloc(param,
                                             sizeof(*discovery_confirm) + sizeof(*network_descriptor) * ZB_PANID_TABLE_SIZE);

    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,26} */
    network_descriptor = (zb_nlme_network_descriptor_t *)(discovery_confirm + 1);

    while (dev_type_cmp != 0U)
    {
      dev_type_cmp--;
      /* First pass: get info about coordinators only */
      for (i = 0 ; i < ZB_NEIGHBOR_TABLE_SIZE ; ++i)
      {
        if (ZB_U2B(ZG->nwk.neighbor.neighbor[i].used)
            && ZB_U2B(ZG->nwk.neighbor.neighbor[i].ext_neighbor)
            && (dev_type_cmp == 0U || ZG->nwk.neighbor.neighbor[i].device_type == ZB_NWK_DEVICE_TYPE_COORDINATOR))
        {
          for (j = 0 ;
               j < n_nwk_dsc;
               /* !zb_address_cmp_pan_id_by_ref(ZG->nwk.neighbor.ext_neighbor[i].panid_ref, network_descriptor[j].extended_pan_id) ; */
               ++j)
          {
            if (ZG->nwk.neighbor.neighbor[i].u.ext.panid_ref == network_descriptor[j].panid_ref)
            {
              /* Accumulate permit_joining, router_capacity and end_device_capacity for the network. */
              /* NK: For the network descriptor it is really does not matter how much is router
               * and end device capacity. It just can be zero or not zero. Really it will be
               * checked for every potential parent from extnb again (nwk_choose_parent() call). */
              if (ZB_U2B(ZG->nwk.neighbor.neighbor[i].permit_joining))
              {
                network_descriptor[j].permit_joining = ZB_TRUE_U;
                if (ZB_U2B(ZG->nwk.neighbor.neighbor[i].u.ext.router_capacity))
                {
                  network_descriptor[j].router_capacity = ZB_TRUE_U;
                }
                if (ZB_U2B(ZG->nwk.neighbor.neighbor[i].u.ext.end_device_capacity))
                {
                  network_descriptor[j].end_device_capacity = ZB_TRUE_U;
                }
              }
              break;
            }
          }
          if (j == n_nwk_dsc && j < ZB_PANID_TABLE_SIZE)
          {
            /* This ext pan id not found - add this PAN */
            /* zb_address_get_pan_id(ZG->nwk.neighbor.ext_neighbor[i].panid_ref, network_descriptor[j].extended_pan_id); */
            ZB_BZERO(&(network_descriptor[j]), sizeof(zb_nlme_network_descriptor_t));
            network_descriptor[j].panid_ref = ZG->nwk.neighbor.neighbor[i].u.ext.panid_ref;
            network_descriptor[j].channel_page = ZG->nwk.neighbor.neighbor[i].u.ext.channel_page;
            network_descriptor[j].logical_channel = ZG->nwk.neighbor.neighbor[i].u.ext.logical_channel;
            network_descriptor[j].stack_profile = ZG->nwk.neighbor.neighbor[i].u.ext.stack_profile;
            /* in order to optimize RAM usage exclude  Zigbee_version, beacon_order, suoerframe_order from the struct below */
            network_descriptor[j].permit_joining = ZG->nwk.neighbor.neighbor[i].permit_joining;
            network_descriptor[j].router_capacity = ZG->nwk.neighbor.neighbor[i].u.ext.router_capacity;
            network_descriptor[j].end_device_capacity = ZG->nwk.neighbor.neighbor[i].u.ext.end_device_capacity;
            network_descriptor[j].nwk_update_id = ZG->nwk.neighbor.neighbor[i].u.ext.update_id;
            n_nwk_dsc++;
          }
        }
      } /* ext neighbor table iterate */
    } /* while */

    discovery_confirm->network_count = (zb_uint8_t)n_nwk_dsc;
    TRACE_MSG(TRACE_NWK1, "discovery_confirm->network_count %hd param %hd", (FMT__H_H, discovery_confirm->network_count, param));
    discovery_confirm->status = RET_OK;
    if (n_nwk_dsc == 0U)
    {
      discovery_confirm->status = ERROR_CODE(ERROR_CATEGORY_MAC, MAC_NO_BEACON);
    }
    ZB_SCHEDULE_CALLBACK(zb_nlme_network_discovery_confirm, param);
    param = 0;
    ZG->nwk.handle.state = ZB_NLME_STATE_IDLE;
  }
  else
  {
#endif  /* ZB_JOIN_CLIENT */
#ifndef ZB_ED_ROLE
    if ( RET_OK == ret
         && (ZG->nwk.handle.state == ZB_NLME_STATE_ED_SCAN
#ifdef ZB_FORMATION
             || ZG->nwk.handle.state == ZB_NLME_STATE_FORMATION_ED_SCAN
#endif
           )
      )
    {
      /* Assemble ED scan result. */
      /* FIXME: Now can not work asynchronously here!
         TODO: divide function to 2 parts and use blocked alloc!
       */
      ed_scan_assemble_result(ZG->nwk.handle.ed_list_param, param);

#if defined ZB_ENHANCED_BEACON_SUPPORT
      if (ZB_TRUE == nwk_check_scan_continue(scan_confirm, &iface_idx, &channel_page, &channel_mask))
      {
        /* There are channels to scan, initiate the next ED scan request. */
        nlme_scan_request(param, ED_SCAN, ZG->nwk.handle.scan_duration,
                          iface_idx, channel_page, channel_mask);
        ret = RET_ERROR;
      }
      else
#endif /* ZB_ENHANCED_BEACON_SUPPORT */
      { /* Means scan complete, no any unscanned channels found. */
        if ( ZG->nwk.handle.state == ZB_NLME_STATE_ED_SCAN )
        {
          call_ed_scan_confirm(ZG->nwk.handle.ed_list_param, ZB_NWK_STATUS_SUCCESS);
          ZG->nwk.handle.state = ZB_NLME_STATE_IDLE;
          ZG->nwk.handle.ed_list_param = 0;
        }
#ifdef ZB_FORMATION
        else
        {
          ZB_ASSERT(ZG->nwk.selector.formation_ed_scan_confirm);
          (*ZG->nwk.selector.formation_ed_scan_confirm)(param);
          /* Run via pointer to exclude Formation linkage if ZR & !distributed.
             There was was nwk_formation_ed_scan_confirm(param); */
          ret = RET_EXIT;
        }
#endif
      }
    }
#endif  /* !ZB_ED_ROLE */
#ifdef ZB_JOIN_CLIENT
  }
#endif

#if defined ZB_FORMATION || defined ZB_ENHANCED_BEACON_SUPPORT
  if (ret == RET_OK)
#endif
  {
#ifdef ZB_FORMATION
    if ( ZG->nwk.handle.state == ZB_NLME_STATE_FORMATION_ACTIVE_SCAN )
    {
      ZB_ASSERT(ZG->nwk.selector.formation_select_channel);
      (*ZG->nwk.selector.formation_select_channel)(param);
      /* was nwk_formation_select_channel(param); */
    }
    else
#endif
#ifdef ZB_JOIN_CLIENT
      if ( ZG->nwk.handle.state == ZB_NLME_STATE_REJOIN )
      {
        zb_nlme_rejoin_scan_confirm(param);
      }
      else
#ifndef ZB_LITE_NO_ORPHAN_SCAN
        if ( ZG->nwk.handle.state == ZB_NLME_STATE_ORPHAN_SCAN )
        {
          zb_nlme_orphan_scan_confirm(param);
        }
        else
#endif
#endif  /* ZB_JOIN_CLIENT */
#if defined ZB_BEACON_SURVEY && defined ZB_ZCL_ENABLE_WWAH_SERVER
        if (ZG->nwk.handle.state == ZB_NLME_STATE_SURVEY_BEACON)
        {
          zb_nlme_beacon_survey_scan_confirm(param);
        }
        else
#endif
          if (ZG->nwk.handle.state == ZB_NLME_STATE_IDLE)
          {
            if (param != 0U)
            {
              TRACE_MSG(TRACE_NWK1, "Beacon request sent from the application", (FMT__0));
              zb_buf_free(param);
            }
          }
          else
          {
            TRACE_MSG(TRACE_ERROR, "wrong nwk state %d", (FMT__D, (int)ZG->nwk.handle.state));
            ZB_ASSERT(0);
          }
    }

  TRACE_MSG(TRACE_NWK1, "<<zb_mlme_scan_confirm", (FMT__0));
}


#ifdef ZB_NWK_BLACKLIST

void zb_nwk_blacklist_add(zb_ext_pan_id_t ext_pan_id)
{
  TRACE_MSG(TRACE_NWK2, "zb_nwk_blacklist_add, panid " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(ext_pan_id)));
  ZB_EXTPANID_COPY(ZG->nwk.blacklist.blacklisted[ZG->nwk.blacklist.used], ext_pan_id);
  ZG->nwk.blacklist.used++;
}


void zb_nwk_blacklist_reset(void)
{
  TRACE_MSG(TRACE_NWK2, "zb_nwk_blacklist_reset: was used %hd", (FMT__H, ZG->nwk.blacklist.used));
  ZG->nwk.blacklist.used = 0;
}


zb_ret_t zb_nwk_blacklist_check(zb_ext_pan_id_t ext_pan_id)
{
  zb_ret_t ret = RET_OK;
  zb_uindex_t i;

  TRACE_MSG(TRACE_NWK2, ">>zb_nwk_blacklist_check for panid " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(ext_pan_id)));

  for (i = 0; i < ZG->nwk.blacklist.used; i++)
  {
    if (ZB_EXTPANID_CMP(ext_pan_id, ZG->nwk.blacklist.blacklisted[i]))
    {
      ret = RET_ERROR;
      break;
    }
  }
  TRACE_MSG(TRACE_NWK2, "<<zb_nwk_blacklist_check ret %hd", (FMT__H, ret));
  return ret;
}


zb_bool_t zb_nwk_blacklist_is_full()
{
  zb_bool_t ret = (zb_bool_t)(ZG->nwk.blacklist.used >= ZB_NWK_BLACKLIST_SIZE);
  TRACE_MSG(TRACE_NWK1, "<<zb_nwk_blacklist_is_full ret %hd", (FMT__H, ret));
  return ret;
}


zb_bool_t zb_nwk_blacklist_is_empty(void)
{
  return !ZB_U2B(ZG->nwk.blacklist.used);
}


#endif /* ZB_NWK_BLACKLIST */

/*! @} */
