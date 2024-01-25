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
/*  PURPOSE: Zigbee R22 NWK MultiMAC
*/

#define ZB_TRACE_FILE_ID 40
#include "zb_common.h"
#include "zb_nwk.h"
#include "zb_nwk_mm.h"

void zb_nwk_mm_mac_iface_table_init(void)
{
  zb_uint8_t i;
  zb_nwk_mac_iface_tbl_ent_t *ent;
  zb_channel_list_t supported_channels_list;

  /* OSIF layer (as a system layer) should be pretty informed about MAC
   * interfaces disregarding the way they connected to the host */

  TRACE_MSG(TRACE_NWK1, ">> zb_nwk_mm_mac_iface_table_init", (FMT__0));

  /* For each interface */
  for (i = 0; i < ZB_NWK_MAC_IFACE_TBL_SIZE; i++)
  {
    ent = &ZB_NIB().mac_iface_tbl[i];

    ent->index = i;
    zb_channel_list_init(supported_channels_list);

#if defined ZB_SUBGHZ_BAND_ENABLED
    /* Set default APS channel mask for SubGhz pages/channels. */
    ZB_CHANNEL_PAGE_SET_MASK(supported_channels_list[ZB_CHANNEL_LIST_PAGE23_IDX], ZB_TRANSCEIVER_ALL_CHANNELS_MASK_PAGE23);
    ZB_CHANNEL_PAGE_SET_MASK(supported_channels_list[ZB_CHANNEL_LIST_PAGE24_IDX], ZB_TRANSCEIVER_ALL_CHANNELS_MASK_PAGE24);
    ZB_CHANNEL_PAGE_SET_MASK(supported_channels_list[ZB_CHANNEL_LIST_PAGE25_IDX], ZB_TRANSCEIVER_ALL_CHANNELS_MASK_PAGE25);
    ZB_CHANNEL_PAGE_SET_MASK(supported_channels_list[ZB_CHANNEL_LIST_PAGE26_IDX], ZB_TRANSCEIVER_ALL_CHANNELS_MASK_PAGE26);
    ZB_CHANNEL_PAGE_SET_MASK(supported_channels_list[ZB_CHANNEL_LIST_PAGE27_IDX], ZB_TRANSCEIVER_ALL_CHANNELS_MASK_PAGE27);
    ZB_CHANNEL_PAGE_SET_MASK(supported_channels_list[ZB_CHANNEL_LIST_PAGE28_IDX], ZB_TRANSCEIVER_ALL_CHANNELS_MASK_PAGE28);
    ZB_CHANNEL_PAGE_SET_MASK(supported_channels_list[ZB_CHANNEL_LIST_PAGE29_IDX], ZB_TRANSCEIVER_ALL_CHANNELS_MASK_PAGE29);
    ZB_CHANNEL_PAGE_SET_MASK(supported_channels_list[ZB_CHANNEL_LIST_PAGE30_IDX], ZB_TRANSCEIVER_ALL_CHANNELS_MASK_PAGE30);
    ZB_CHANNEL_PAGE_SET_MASK(supported_channels_list[ZB_CHANNEL_LIST_PAGE31_IDX], ZB_TRANSCEIVER_ALL_CHANNELS_MASK_PAGE31);
#endif /* ZB_SUBGHZ_BAND_ENABLED */
#ifndef ZB_SUBGHZ_ONLY_MODE
    /* In case of only 2.4GHz page, the channel_page points to 2.4GHz */
    ZB_CHANNEL_PAGE_SET_MASK(supported_channels_list[ZB_CHANNEL_LIST_PAGE0_IDX], ZB_TRANSCEIVER_ALL_CHANNELS_MASK);
#endif

    ZB_MEMCPY((zb_uint8_t *)ent->supported_channels, (zb_uint8_t *)supported_channels_list, sizeof(zb_channel_list_t));

    /* FIXME: Where to enable?? */
    ent->state = 1;
    ent->link_power_data_rate = ZB_NWK_LINK_POWER_DELTA_TRANSMIT_RATE;
  }

  TRACE_MSG(TRACE_NWK1, "<< zb_nwk_mm_mac_iface_table_init", (FMT__0));
}

zb_uint8_t zb_nwk_mm_get_freq_band(void)
{
  zb_ushort_t i;
  zb_uint8_t freq_band;

  TRACE_MSG(TRACE_NWK1, ">> zb_nwk_mm_get_freq_band", (FMT__0));

  freq_band = 0;
  for (i = 0; i < ZB_NWK_MAC_IFACE_TBL_SIZE; i++)
  {
    zb_ushort_t j;
    zb_nwk_mac_iface_tbl_ent_t *ent = &ZB_NIB().mac_iface_tbl[i];

    /* For all supported channel pages */
    for (j = 0; j < ZB_CHANNEL_PAGES_NUM; j++)
    {
      if (ZB_CHANNEL_PAGE_IS_2_4GHZ(ent->supported_channels[j]) &&
          !ZB_CHANNEL_PAGE_IS_MASK_EMPTY(ent->supported_channels[j]))
      {
        freq_band |= ZB_FREQ_BAND_2400;
      }

      if (ZB_CHANNEL_PAGE_IS_SUB_GHZ(ent->supported_channels[j]) &&
          !ZB_CHANNEL_PAGE_IS_MASK_EMPTY(ent->supported_channels[j]))
      {
        /* Only one entry will be enough */
        freq_band |= ZB_FREQ_BAND_SUB_GHZ_EU_FSK;
        break;
      }
    }
  }

  TRACE_MSG(TRACE_NWK1, "<< zb_nwk_mm_get_freq_band, freq_band %hd", (FMT__H, freq_band));

  return freq_band;
}

void zb_nlme_set_interface_request(zb_uint8_t param)
{
  /* TODO: Implement me */
  TRACE_MSG(TRACE_NWK1, ">> zb_nlme_set_interface_request param %hd", (FMT__H, param));
  zb_buf_free(param);
  TRACE_MSG(TRACE_NWK1, "<< zb_nlme_set_interface_request", (FMT__0));
}

void zb_nlme_get_interface_request(zb_uint8_t param)
{
  /* TODO: Implement me */
  TRACE_MSG(TRACE_NWK1, ">> zb_nlme_get_interface_request param %hd", (FMT__H, param));
  zb_buf_free(param);
  TRACE_MSG(TRACE_NWK1, "<< zb_nlme_get_interface_request", (FMT__0));
}
