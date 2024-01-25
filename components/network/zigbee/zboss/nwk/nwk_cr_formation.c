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
/* PURPOSE: Network creation routine
*/

#define ZB_TRACE_FILE_ID 2232
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_mac.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "nwk_internal.h"
#include "zb_magic_macros.h"
#include "zdo_wwah_stubs.h"

/*! \addtogroup ZB_NWK */
/*! @{ */


#ifdef ZB_FORMATION


/**
   Called from the discovery confirm with ED scan results.
   Initiate active scan or reports formation failure

   @param param - buffer for further operations
   @return nothing
 */
static void nwk_formation_ed_scan_confirm(zb_uint8_t param);

/**
   Called from the discovery confirm with active scan results.
   Initiate new network start routine or reports formation failure.

   @param buf - buffer containing results - @see
   zb_mac_scan_confirm_t
   @return nothing
 */
static void nwk_formation_select_channel(zb_bufid_t buf);
static void mlme_start_confirm_formation(zb_uint8_t param);

/**
   Setup force formation code link form the library.

   To be used for conditional switching of distributed security at application compile time.
   If this routine is not called, the entire file will not be linked.
 */
void zb_nwk_formation_force_link(void)
{
  TRACE_MSG(TRACE_NWK1, "nwk_formation_force_link", (FMT__0));
  /* Setup function pointers selector. Run functions indirectly to exclude its linking. */
  ZG->nwk.selector.formation_select_channel = nwk_formation_select_channel;
  ZG->nwk.selector.formation_ed_scan_confirm = nwk_formation_ed_scan_confirm;
  ZG->nwk.selector.formation_mlme_start_conf = mlme_start_confirm_formation;
}


static void nwk_formation_failure_confirm(zb_bufid_t buf, zb_ret_t status)
{
  zb_buf_set_status(buf, status);
  ZB_NIB().nwk_hub_connectivity = 0;
  ZB_SCHEDULE_CALLBACK(zb_nlme_network_formation_confirm, buf);
}


/* called form zdo_start_formation() only (and ncp/ncp_hl_proto.c - ignore it)*/
void zb_nlme_network_formation_request(zb_uint8_t param)
{
  zb_uint8_t nwk_state;
  zb_ret_t ret;
  zb_uint8_t scan_iface_idx;
  zb_uint8_t channel_page;
  zb_uint32_t channel_mask;
  zb_uint8_t scan_iface_idx_tmp;
  zb_uint8_t channel_page_tmp;
  zb_uint32_t channel_mask_tmp;
  zb_uint8_t page_idx;
  zb_channel_list_t *scan_channels_list;

  TRACE_MSG(TRACE_NWK1, ">>nwk_formation_req %hd", (FMT__H, param));
  TRACE_MSG(TRACE_NWK1, "nwk state %hd joined %hd", (FMT__H_H, ZG->nwk.handle.state, ZB_JOINED()));

  if (!(ZG->nwk.handle.state == ZB_NLME_STATE_IDLE
        && !ZB_JOINED() ))
  {
    TRACE_MSG(TRACE_NWK1, "nwk busy or joined %d", (FMT__0, ZG->nwk.handle.state));
    nwk_formation_failure_confirm(param, ZB_NWK_STATUS_INVALID_REQUEST);
  }
  else
  {
    /**
      \par Network formation sequence

      See 3.6.1.1  Establishing a New Network


      * MLME-SCAN (energy scan) - skip if only 1 channel
      * MLME-SCAN (active scan) - skip if ns-3 build
      * select channel          - skip if ns-3 build
      * set PAN ID in PIB
      * set short address in PIB
      * MLME-START
      * return via NLME-NETWORK-FORMATION.confirm


      calls sequence:
      * zb_mlme_scan_request (ed scan) - zb_mlme_scan_confirm - nwk_formation_ed_scan_confirm
      * zb_mlme_scan_request (active scan) - zb_mlme_scan_confirm - nwk_formation_select_channel - call_mlme_start - zb_mlme_start_request
      * zb_mlme_start_confirm - zb_nlme_network_formation_confirm

     */

    /* request was saved in our internal buffer in zb_nlme_network_formation_request() */
    zb_nlme_network_formation_request_t *request =
      ZB_BUF_GET_PARAM(param, zb_nlme_network_formation_request_t);

    if (!ZB_EXTPANID_IS_ZERO(request->extpanid))
    {
      ZB_IEEE_ADDR_COPY(ZB_NIB_EXT_PAN_ID(), request->extpanid);
    }

    /* Start discovery using the first supported page. */
    ret = nwk_scan_find_next_channel_mask(0, 0, request->scan_channels_list, &scan_iface_idx,
                                          &channel_page, &channel_mask);
    if (ret == RET_OK)
    {
    /* Save request values */
      zb_channel_page_list_copy(ZG->nwk.handle.scan_channels_list, request->scan_channels_list);
      scan_channels_list = &ZG->nwk.handle.scan_channels_list;

      /* Set ED scan by default. */
      nwk_state = ZB_NLME_STATE_FORMATION_ED_SCAN;

      /*
       * Scan_channels will be power of 2 if only 1
       * bit is set - means, only 1 channel specified
       */
      if (MAGIC_IS_POWER_OF_TWO(channel_mask))
      {
        /*
         * Try to find another channel to scan just to check whether the ED scan
         * should be performed.
         */
        scan_iface_idx_tmp = scan_iface_idx;
        ret = zb_channel_page_list_get_page_idx(channel_page, &page_idx);
        ZB_ASSERT(ret == RET_OK);

#if defined ZB_SUBGHZ_BAND_ENABLED
        /* for 2.4 this condition will be false and page_idx will be 0 always */
        if ((ZB_CHANNEL_PAGES_NUM > 1) && (page_idx < (ZB_CHANNEL_PAGES_NUM - 1)))
        {
          page_idx++;
        }
        else
        {
          scan_iface_idx_tmp++;
          page_idx = 0;
        }
#else
        scan_iface_idx_tmp++;
#endif /* !ZB_SUBGHZ_BAND_ENABLED */

        ret = nwk_scan_find_next_channel_mask(scan_iface_idx_tmp, page_idx, *scan_channels_list,
                                              &scan_iface_idx_tmp, &channel_page_tmp, &channel_mask_tmp);
        if (ret == RET_NOT_FOUND)
        {
          /*
           * There are no other channels to scan specified, so ED scan may
           * be skipped (see 3.2.2.5.3).
           */
          nwk_state = ZB_NLME_STATE_FORMATION_ACTIVE_SCAN;
        }
        else
        {
          ZB_ASSERT(ZG->nwk.handle.ed_list_param == 0);
        }
      }

      ZB_NIB().nwk_hub_connectivity = ZB_IS_DEVICE_ZC();
      ZG->nwk.handle.state = nwk_state;
      nlme_scan_request(param, nwk_state == ZB_NLME_STATE_FORMATION_ACTIVE_SCAN ? ACTIVE_SCAN : ED_SCAN,
                        request->scan_duration, scan_iface_idx, channel_page, channel_mask);
      /* Call 1 or several scans, select channel, call mlme_start */
    }
    else
    {
      TRACE_MSG(TRACE_NWK1, "Could not get channels mask to start formation!", (FMT__0));
      nwk_formation_failure_confirm(param, ZB_NWK_STATUS_INVALID_PARAMETER);
    }
  }

  TRACE_MSG(TRACE_NWK1, "<<nwk_formation_req", (FMT__0));
}


static void nwk_formation_ed_scan_confirm(zb_uint8_t param)
{
  zb_ret_t ret;
  zb_energy_detect_list_t *ed_list;
  zb_energy_detect_channel_info_t *channel_info;
  zb_energy_detect_channel_info_t min_unacceptable_energy_chan_info;
  zb_channel_list_t *scan_channels_list = &ZG->nwk.handle.scan_channels_list;
  zb_uint32_t channel_mask;
  zb_uint8_t channel_page;
  zb_uint8_t scan_iface_idx;
  zb_uint8_t min_unacceptable_energy = 0xff;
  zb_bool_t acceptable = ZB_FALSE;
  zb_ushort_t i;

  TRACE_MSG(TRACE_NWK1, ">>nwk_form_ed_scan_conf param %hd ed_list_param %hd",
            (FMT__H_H, param, ZG->nwk.handle.ed_list_param));

  ed_list = ZB_BUF_GET_PARAM(ZG->nwk.handle.ed_list_param,
                             zb_energy_detect_list_t);

  CHECK_PARAM_RET_ON_ERROR(ed_list);

  if (ed_list->channel_count != 0)
  {
    ZB_BZERO(&min_unacceptable_energy_chan_info, sizeof(min_unacceptable_energy_chan_info));

    /* Clear initial scan channel list, it will be filled below using ED scan result. */
    zb_channel_list_init(*scan_channels_list);

    /* Generate channels mask for active scan. */
    for (i = 0; i < ed_list->channel_count; i++)
    {
      channel_info = &ed_list->channel_info[i];
      channel_page = zb_channel_page_list_get_page(*scan_channels_list,
                                                   channel_info->channel_page_idx);

      TRACE_MSG(TRACE_NWK1, "page %hd, channel %hd, e_level %hd ",
                (FMT__H_H_H, channel_page, channel_info->channel_number, channel_info->energy_detected));

      /* skip channels which level is higher than acceptable, but store minimum
       * energy value for unacceptable channels. We will use this if all channels
       * have too high energy */
      if (channel_info->energy_detected > ZB_NWK_CHANNEL_ACCEPT_LEVEL)
      {
        TRACE_MSG(TRACE_NWK1, "Unacceptable e_level %hd for page %hd channel %hd",
                  (FMT__H_H_H, channel_info->energy_detected, channel_page, channel_info->channel_number));

        if (channel_info->energy_detected <= min_unacceptable_energy)
        {
          min_unacceptable_energy_chan_info = *channel_info;
          min_unacceptable_energy = channel_info->energy_detected;
        }
      }
      else
      {
        /* Set channel for further active scan. */
        zb_channel_page_list_set_channel(*scan_channels_list,
                                         channel_info->channel_page_idx,
                                         channel_info->channel_number);
        /* Mark that there is at least one channel with acceptable energy. */
        acceptable = ZB_TRUE;
      }
    }

    if (!acceptable)
    {
      /* If no scan channels remained, use the rejected one with minimum energy.
       * So we will not fail to starup if no acceptable energy channels found */
      channel_page = zb_channel_page_list_get_page(*scan_channels_list,
                                                   min_unacceptable_energy_chan_info.channel_page_idx);
      TRACE_MSG(TRACE_NWK1, "Use channel page %hd channel number %hd with minimum energy %hd",
                (FMT__H_H_H, channel_page,
                 min_unacceptable_energy_chan_info.channel_number,
                 min_unacceptable_energy_chan_info.energy_detected));

      zb_channel_page_list_set_channel(*scan_channels_list,
                                       min_unacceptable_energy_chan_info.channel_page_idx,
                                       min_unacceptable_energy_chan_info.channel_number);
    }
  }

  /* Start discovery using the first supported page. */
  ret = nwk_scan_find_next_channel_mask(0, 0, *scan_channels_list, &scan_iface_idx,
                                        &channel_page, &channel_mask);
  if (ret == RET_OK)
  {
    ZG->nwk.handle.state = ZB_NLME_STATE_FORMATION_ACTIVE_SCAN;
    /* call mac to make an active scan */
    nlme_scan_request(param, ACTIVE_SCAN, ZG->nwk.handle.scan_duration,
                      scan_iface_idx, channel_page, channel_mask);
  }
  else
  {
    TRACE_MSG(TRACE_NWK1, "Could not get channels mask to start active scan!", (FMT__0));
    nwk_formation_failure_confirm(param, ZB_NWK_STATUS_INVALID_PARAMETER);
  }

  TRACE_MSG(TRACE_NWK1, "<<nwk_form_ed_scan_conf", (FMT__0));
}


static zb_ret_t nwk_get_channel_ed_level(zb_energy_detect_list_t *ed_scan_results,
                                         zb_uint8_t channel_page_idx,
                                         zb_uint8_t channel_number,
                                         zb_uint8_t *channel_ed_level)
{
  zb_uint8_t i;
  zb_ret_t ret = RET_NOT_FOUND;
  zb_energy_detect_channel_info_t *channel_info;

  ZB_ASSERT(ed_scan_results != NULL);
  ZB_ASSERT(channel_ed_level != NULL);

  *channel_ed_level = 0;

  for (i = 0; i < ed_scan_results->channel_count; i++)
  {
    channel_info = &ed_scan_results->channel_info[i];

    if (channel_info->channel_page_idx == channel_page_idx &&
        channel_info->channel_number == channel_number)
    {
      *channel_ed_level = channel_info->energy_detected;
      ret = RET_OK;
      break;
    }
  }

  TRACE_MSG(TRACE_NWK1, "nwk_get_channel_ed_level: page_idx %hd channel_number %hd found %hd channel_ed_level %hd", (FMT__H_H_H_H, channel_page_idx, channel_number, ret, channel_ed_level));

  return ret;
}


static zb_bool_t pick_pan_id(zb_uint8_t channel_page, zb_uint8_t logical_channel, zb_uint16_t *pan_id)
{
  zb_uint8_t i;
  zb_uint16_t tmp_pan_id = ZB_PIBCACHE_PAN_ID();
  zb_uint16_t nt_panid;
  zb_bool_t pan_id_is_unique = ZB_FALSE;

  ZB_ASSERT(pan_id != NULL);

  *pan_id = 0;

  if (tmp_pan_id == 0 || tmp_pan_id == ZB_BROADCAST_PAN_ID)
  {
    tmp_pan_id = ZB_RANDOM_U16();
  }

  /* generate pan_id if necessary */
  while ( !pan_id_is_unique )
  {
    /* Lets assume that this panid is unique. */
    pan_id_is_unique = ZB_TRUE;

    TRACE_MSG(TRACE_NWK1, "generated pan_id 0x%x", (FMT__D, tmp_pan_id));

    /* check pan_id not used in this channel */
    for (i = 0; i < ZB_NEIGHBOR_TABLE_SIZE ; i++)
    {
      if (ZB_U2B(ZG->nwk.neighbor.neighbor[i].used) && ZG->nwk.neighbor.neighbor[i].ext_neighbor
          && ZG->nwk.neighbor.neighbor[i].u.ext.channel_page == channel_page
          && ZG->nwk.neighbor.neighbor[i].u.ext.logical_channel == logical_channel)
      {
        zb_address_get_short_pan_id(ZG->nwk.neighbor.neighbor[i].u.ext.panid_ref, &nt_panid);
        if (nt_panid == tmp_pan_id)
        {
          TRACE_MSG(TRACE_NWK1, "pan_id 0x%x is on page %hd ch logical %hd",
                    (FMT__H_H_H, tmp_pan_id, channel_page, logical_channel));
          pan_id_is_unique = ZB_FALSE;
          break;
        }
      }
    }

    /* Lets try to generate random panid one more time */
    if (!pan_id_is_unique)
    {
      tmp_pan_id = ZB_RANDOM_U16();
    }
  }

  if (pan_id_is_unique)
  {
    *pan_id = tmp_pan_id;
  }

  return pan_id_is_unique;
}


/**
   Select channel after active scan complete.

   Used at formation time.
   Called by zb_mlme_scan_confirm()

   @param buf - buffer with results
 */
static void nwk_formation_select_channel(zb_bufid_t buf)
{
  zb_ret_t ret;
  zb_uint8_t channel_networks = 0xff;
  zb_uint8_t channel_ed = 0xff;
  zb_uint8_t channel_ed_cur = 0;
  zb_uint8_t channel_page = 0;
  zb_uint8_t channel_page_cur = 0;
  zb_uint8_t channel_page_cur_idx = 0;
  zb_uint8_t channel_number_tmp;
  zb_uint8_t logical_channel = 0;
  zb_uint8_t logical_channel_tmp;
  zb_uint32_t scan_channel_mask;
  zb_uint8_t start_channel_number;
  zb_uint8_t max_channel_number;
  zb_uint16_t pan_id;
  zb_bool_t pan_id_picked;
  zb_nwk_handle_t *nwk_handle = &ZG->nwk.handle;
  zb_uint8_t *panid_handled_bm = &ZG->nwk.handle.tmp.formation.panid_handled_bm[0];
  zb_channel_list_t *scan_channels_list = &ZG->nwk.handle.scan_channels_list;
  zb_energy_detect_list_t *ed_list = NULL;
  zb_ushort_t i, j;
  zb_bool_t channel_found = ZB_FALSE;

  TRACE_MSG(TRACE_NWK1, ">>nwk_formation_select_channel buf %hd", (FMT__H, buf));

  if (ZG->nwk.handle.ed_list_param)
  {
    ed_list = ZB_BUF_GET_PARAM(ZG->nwk.handle.ed_list_param,
                               zb_energy_detect_list_t);

    CHECK_PARAM_RET_ON_ERROR(ed_list);
  }

  ZB_BZERO(panid_handled_bm, sizeof(ZG->nwk.handle.tmp.formation.panid_handled_bm));
  ZB_BZERO(nwk_handle->tmp.formation.channel_pan_count,
           sizeof(nwk_handle->tmp.formation.channel_pan_count));

  /* calc number of networks on each channel */
  for (i = 0 ; i < ZB_NEIGHBOR_TABLE_SIZE ; ++i)
  {
    if (!(ZB_U2B(ZG->nwk.neighbor.neighbor[i].used) && ZG->nwk.neighbor.neighbor[i].ext_neighbor)
        || ZB_CHECK_BIT_IN_BIT_VECTOR(panid_handled_bm, ZG->nwk.neighbor.neighbor[i].u.ext.panid_ref))
    {
      continue;
    }
    ZB_SET_BIT_IN_BIT_VECTOR(panid_handled_bm, ZG->nwk.neighbor.neighbor[i].u.ext.panid_ref);

    ret = zb_channel_page_list_get_page_idx(ZG->nwk.neighbor.neighbor[i].u.ext.channel_page,
                                            &channel_page_cur_idx);
    ZB_ASSERT(ret == RET_OK);
    ret = zb_channel_page_channel_logical_to_number(ZG->nwk.neighbor.neighbor[i].u.ext.channel_page,
                                                    ZG->nwk.neighbor.neighbor[i].u.ext.logical_channel,
                                                    &channel_number_tmp);
    ZB_ASSERT(ret == RET_OK);
    nwk_handle->tmp.formation.channel_pan_count[channel_page_cur_idx][channel_number_tmp]++;
  } /* networks per channel calculation loop */

  /* Try to find suitable channel, first select with the minimal numbers of
   * networks, then with the lowest energy  */
  for (i = 0; i < ZB_CHANNEL_PAGES_NUM; i++)
  {
    channel_page_cur = zb_channel_page_list_get_page(*scan_channels_list, i);

    ret = zb_channel_page_get_start_channel_number(channel_page_cur, &start_channel_number);
    ZB_ASSERT(ret == RET_OK);
    ret = zb_channel_page_get_max_channel_number(channel_page_cur, &max_channel_number);
    ZB_ASSERT(ret == RET_OK);
    scan_channel_mask = zb_channel_page_list_get_mask(*scan_channels_list, i);

    TRACE_MSG(TRACE_NWK1,
              "scan_channel_mask 0x%lx",
              (FMT__L, scan_channel_mask));
    TRACE_MSG(TRACE_NWK1,
              "start_channel_number %hd end_channel_number %hd",
              (FMT__H_H, start_channel_number, max_channel_number));

    for (j = start_channel_number; j <= max_channel_number; j++)
    {
      if ((scan_channel_mask & (1L << j))
          && nwk_handle->tmp.formation.channel_pan_count[i][j] <= channel_networks)
      {
        if (ed_list != NULL)
        {
          ret = nwk_get_channel_ed_level(ed_list, channel_page_cur_idx, j, &channel_ed_cur);
          if (ret != RET_OK)
          {
            /* there is no energy info, skip it */
            continue;
          }
        }

        ret = zb_channel_page_channel_number_to_logical(channel_page_cur, j,
                                                        &logical_channel_tmp);
        ZB_ASSERT(ret == RET_OK);

        TRACE_MSG(TRACE_NWK3, "cur ch  %hd ch_nw %hd en_lev %hd",
                  (FMT__H_H_H, ZB_PIBCACHE_CURRENT_CHANNEL(), channel_networks, channel_ed));
        TRACE_MSG(TRACE_NWK3, "page idx %hd channel %hd (logical %hd) pan_cnt %hd en_lev %hd",
                  (FMT__H_H_H_H_H, i, j, logical_channel_tmp,
                   nwk_handle->tmp.formation.channel_pan_count[i][j], channel_ed_cur));

        /* select with the lower energy */
        if (nwk_handle->tmp.formation.channel_pan_count[i][j] == channel_networks
            && channel_ed_cur >= channel_ed)
        {
          /* skip it */
          continue;
        }

        logical_channel = logical_channel_tmp;
        channel_page = channel_page_cur;
        channel_networks = nwk_handle->tmp.formation.channel_pan_count[i][j];
        channel_ed = channel_ed_cur;
        channel_found = ZB_TRUE;

        /* found channel without networks ? */
        if (channel_found && channel_networks == 0)
        {
          break;
        }
      }
    }
    /* found channel without networks ? */
    if (channel_found && channel_networks == 0)
    {
      break;
    }
    else
    {
      channel_page_cur_idx++;
    }
  }
  if (ZG->nwk.handle.ed_list_param)
  {
    /* Energy scan results are not needed anymore. */
    zb_buf_free(ZG->nwk.handle.ed_list_param);
    ZG->nwk.handle.ed_list_param = 0;
  }

  ZB_ASSERT(channel_found);
  TRACE_MSG(TRACE_INFO1, "sel page %hd ch logical %hd # nw %hd en_lev %hd",
            (FMT__H_H_H_H, channel_page, logical_channel, channel_networks, channel_ed));

  pan_id_picked = pick_pan_id(channel_page, logical_channel, &pan_id);

  /* PAN ID is choosen - exneighbors not needed */
  zb_nwk_exneighbor_stop(0xffff);
  if (pan_id_picked)
  {
    TRACE_MSG(TRACE_NWK1, "selected pan_id %x device type %d", (FMT__D_D, pan_id, ZB_NIB_DEVICE_TYPE()));

#ifdef ZB_COORDINATOR_ROLE
    if (ZB_IS_DEVICE_ZC())
    {
      ZB_NIB().depth = 0;
    /* the NLME will choose 0x0000 as the 16-bit short MAC address. See 7.1.14.1.3
     * Effect on receipt:
     * The address used by the coordinator in its beacon frames is determined by the current value of
     * macShortAddress, which is set by the next higher layer before issuing this
     * primitive.
     */
      ZB_PIBCACHE_NETWORK_ADDRESS() = 0;
    }
#endif
#ifdef ZB_NWK_DISTRIBUTED_ADDRESS_ASSIGN
    ZB_NIB().cskip = zb_nwk_daa_calc_cskip(ZB_NIB_DEPTH());
    TRACE_MSG(TRACE_NWK3, "cskip %d", (FMT__D, ZB_NIB().cskip));
#endif

    /* The problem is that short address in PIB must be changed via MLME-SET -
     * means, via cllback. Must remember pan_id and channel somewhere.
     * Can remember panid in the PIB cache in NWK.
     */
    ZB_PIBCACHE_PAN_ID() = pan_id;
    ZB_PIBCACHE_CURRENT_PAGE() = channel_page;
    ZB_PIBCACHE_CURRENT_CHANNEL() = logical_channel;
#ifndef ZB_COORDINATOR_ONLY
      /* Generate a non-zero short address */
      while (ZB_IS_DEVICE_ZR()
             && (ZB_PIBCACHE_NETWORK_ADDRESS() == 0x00
                 || ZB_NWK_IS_ADDRESS_BROADCAST(ZB_PIBCACHE_NETWORK_ADDRESS())))
      {
        ZB_PIBCACHE_NETWORK_ADDRESS() = ZB_RANDOM_U16();
      }

#endif  /* #ifdef ZB_DISTRIBUTED_SECURITY_ON */
    TRACE_MSG(TRACE_NWK1, "set short address 0x%x", (FMT__D, ZB_PIBCACHE_NETWORK_ADDRESS()));
    zb_nwk_pib_set(buf, ZB_PIB_ATTRIBUTE_SHORT_ADDRESS, &ZB_PIBCACHE_NETWORK_ADDRESS(), 2, zb_nwk_call_mlme_start);
  }
  else
  {
    TRACE_MSG(TRACE_NWK1, "channel select failed", (FMT__0));
    nwk_formation_failure_confirm(buf, ZB_NWK_STATUS_STARTUP_FAILURE);
    ZG->nwk.handle.state = ZB_NLME_STATE_IDLE;
  }

  TRACE_MSG(TRACE_NWK1, "<<sel_ch", (FMT__0));
}


/**
   Fill parameters and call zb_mlme_start_request primitive

 */
void zb_nwk_call_mlme_start(zb_uint8_t param)
{
  zb_mlme_start_req_t *req = ZB_BUF_GET_PARAM(param, zb_mlme_start_req_t);

  /* if the NIB attribute  nwkExtendedPANId is equal to 0x0000000000000000, this
     attribute will be initialized with the value of the MAC constant
     macExtendedAddress.  */
  if (ZB_IEEE_ADDR_IS_ZERO(ZB_NIB_EXT_PAN_ID()))
  {
    ZB_EXTPANID_COPY(ZB_NIB_EXT_PAN_ID(), ZB_PIBCACHE_EXTENDED_ADDRESS());
    TRACE_MSG(TRACE_MAC3, "zb_nwk_call_mlme_start, set panid   " TRACE_FORMAT_64, (FMT__A,
                                                                                   TRACE_ARG_64(ZB_PIBCACHE_EXTENDED_ADDRESS())));

  }

  ZB_BZERO(req, sizeof(*req));

  req->pan_id = ZB_PIBCACHE_PAN_ID();
  req->logical_channel = ZB_PIBCACHE_CURRENT_CHANNEL();
  req->channel_page = ZB_PIBCACHE_CURRENT_PAGE();
  TRACE_MSG(TRACE_NWK2, "zb_nwk_call_mlme_start channel %hd page %hd", (FMT__H_H, ZB_PIBCACHE_CURRENT_CHANNEL(), ZB_PIBCACHE_CURRENT_PAGE()));
#ifdef ZB_COORDINATOR_ROLE
  req->pan_coordinator = ZB_AIB().aps_designated_coordinator;
#endif
  req->coord_realignment = 0;
  req->beacon_order = ZB_TURN_OFF_ORDER;
  req->superframe_order = ZB_TURN_OFF_ORDER;

  ZB_SCHEDULE_CALLBACK(zb_mlme_start_request, param);

  ZG->nwk.handle.state = ZB_NLME_STATE_FORMATION;
}


/**
   Continue ZC start after reboot.

   Formation is already done, so continue start without formation.
 */
void zb_nwk_cont_without_formation(zb_uint8_t param)
{
  TRACE_MSG(TRACE_NWK1, ">>zb_nwk_cont_without_formation param %hd", (FMT__H, param));

  /* moved here from commissioning logic */
  ZB_SET_JOINED_STATUS(ZB_TRUE);

  /* Distributed security means it was formation initiated by ZR */
#ifdef ZB_COORDINATOR_ROLE
  if (!IS_DISTRIBUTED_SECURITY())
  {
    ZB_NIB().depth = 0;
    ZB_PIBCACHE_NETWORK_ADDRESS() = 0;
  }
#endif
  ZB_ZDO_SCHEDULE_UPDATE_LONG_UPTIME();
  zb_nwk_pib_set(param, ZB_PIB_ATTRIBUTE_SHORT_ADDRESS,
                 &ZB_PIBCACHE_NETWORK_ADDRESS(), 2, zb_nwk_call_mlme_start);
  TRACE_MSG(TRACE_NWK1, "<<zb_nwk_cont_without_formation param", (FMT__0));
}


static void mlme_start_confirm_formation(zb_uint8_t param)
{
  zb_uint8_t rx_on;
  /* See 3.6.7.
     The NWK layer of the Zigbee coordinator shall update the beacon payload
     immediately following network formation. All other Zigbee devices shall update
     it immediately after the join is completed and any time the network configuration
     (any of the parameters specified in Table3.10) changes.
  */
  /* simplify all checks: set 'joined' flag. */
  ZB_SET_JOINED_STATUS(ZB_TRUE);
  ZG->nwk.handle.run_after_update_beacon_payload = zb_nlme_network_formation_confirm;

  /* Since we are doing Formation, we are ZC or ZR. ZR or ZC must be rx on when idle */
  rx_on = 1;
#ifndef ZB_ED_RX_OFF_WHEN_IDLE
  ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_TRUE_U;
#endif
  zb_nwk_pib_set(param, ZB_PIB_ATTRIBUTE_RX_ON_WHEN_IDLE,
                 &rx_on, 1,
                 /* contine formation after rx on set */
                 nwk_router_start_common);
}

#endif /* ZB_FORMATION */

/*! @} */
