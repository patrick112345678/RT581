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
/* PURPOSE: ZBOSS channel page API
*/

#define ZB_TRACE_FILE_ID 48
#include "zb_common.h"
#include "zboss_api_core.h"

/*! \addtogroup channel_page */
/*! @{ */

/* Common channel page list helpers */


static void setup_channel_pages(zb_channel_page_t *channel_page)
{
  ZB_CHANNEL_PAGE_SET_PAGE(channel_page[ZB_CHANNEL_LIST_PAGE0_IDX], ZB_CHANNEL_PAGE0_2_4_GHZ);
#if defined ZB_SUBGHZ_BAND_ENABLED
  ZB_CHANNEL_PAGE_SET_PAGE(channel_page[ZB_CHANNEL_LIST_PAGE23_IDX], ZB_CHANNEL_PAGE23_SUB_GHZ);
  ZB_CHANNEL_PAGE_SET_PAGE(channel_page[ZB_CHANNEL_LIST_PAGE24_IDX], ZB_CHANNEL_PAGE24_SUB_GHZ);
  ZB_CHANNEL_PAGE_SET_PAGE(channel_page[ZB_CHANNEL_LIST_PAGE25_IDX], ZB_CHANNEL_PAGE25_SUB_GHZ);
  ZB_CHANNEL_PAGE_SET_PAGE(channel_page[ZB_CHANNEL_LIST_PAGE26_IDX], ZB_CHANNEL_PAGE26_SUB_GHZ);
  ZB_CHANNEL_PAGE_SET_PAGE(channel_page[ZB_CHANNEL_LIST_PAGE27_IDX], ZB_CHANNEL_PAGE27_SUB_GHZ);
  ZB_CHANNEL_PAGE_SET_PAGE(channel_page[ZB_CHANNEL_LIST_PAGE28_IDX], ZB_CHANNEL_PAGE28_SUB_GHZ);
  ZB_CHANNEL_PAGE_SET_PAGE(channel_page[ZB_CHANNEL_LIST_PAGE29_IDX], ZB_CHANNEL_PAGE29_SUB_GHZ);
  ZB_CHANNEL_PAGE_SET_PAGE(channel_page[ZB_CHANNEL_LIST_PAGE30_IDX], ZB_CHANNEL_PAGE30_SUB_GHZ);
  ZB_CHANNEL_PAGE_SET_PAGE(channel_page[ZB_CHANNEL_LIST_PAGE31_IDX], ZB_CHANNEL_PAGE31_SUB_GHZ);
#endif /* ZB_SUBGHZ_BAND_ENABLED */
}

void zb_channel_page_list_copy(zb_channel_list_t dst,
                                    zb_channel_list_t src)
{
  ZB_ASSERT(dst != NULL && src != NULL);
  ZB_MEMCPY(dst, src, sizeof(zb_channel_list_t));
  setup_channel_pages(dst);
}

void zb_channel_page_list_set_mask(zb_channel_list_t list,
                                   zb_uint8_t         idx,
                                   zb_uint32_t        mask)
{
  ZB_ASSERT(list != NULL);
  ZB_ASSERT(idx < ZB_CHANNEL_PAGES_NUM);

  ZB_CHANNEL_PAGE_SET_MASK(list[idx], mask);
}

zb_ret_t zb_channel_page_channel_logical_to_number(zb_uint8_t page,
                                                   zb_uint8_t logical_channel,
                                                   zb_uint8_t *channel_number)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t start_logical_channel = 0;
  zb_uint8_t max_logical_channel = 0;
  zb_uint8_t start_channel = 0; /* 0 for subghz, 11 for 2.4 */

  ZB_ASSERT(channel_number != NULL);

  *channel_number = 0xFF;

  switch (page)
  {
    case ZB_CHANNEL_PAGE0_2_4_GHZ:
    {
      start_logical_channel = ZB_PAGE0_2_4_GHZ_START_LOGICAL_CHANNEL;
      max_logical_channel = ZB_PAGE0_2_4_GHZ_MAX_LOGICAL_CHANNEL;
      start_channel = ZB_PAGE0_2_4_GHZ_START_CHANNEL_NUMBER;
      break;
    }
    case ZB_CHANNEL_PAGE23_SUB_GHZ:
    {
      start_logical_channel = ZB_PAGE23_SUB_GHZ_START_LOGICAL_CHANNEL;
      max_logical_channel = ZB_PAGE23_SUB_GHZ_MAX_LOGICAL_CHANNEL;
      break;
    }
    case ZB_CHANNEL_PAGE24_SUB_GHZ:
    {
      start_logical_channel = ZB_PAGE24_SUB_GHZ_START_LOGICAL_CHANNEL;
      max_logical_channel = ZB_PAGE24_SUB_GHZ_MAX_LOGICAL_CHANNEL;
      break;
    }
    case ZB_CHANNEL_PAGE25_SUB_GHZ:
    {
      start_logical_channel = ZB_PAGE25_SUB_GHZ_START_LOGICAL_CHANNEL;
      max_logical_channel = ZB_PAGE25_SUB_GHZ_MAX_LOGICAL_CHANNEL;
      break;
    }
    case ZB_CHANNEL_PAGE26_SUB_GHZ:
    {
      start_logical_channel = ZB_PAGE26_SUB_GHZ_START_LOGICAL_CHANNEL;
      max_logical_channel = ZB_PAGE26_SUB_GHZ_MAX_LOGICAL_CHANNEL;
      break;
    }
    case ZB_CHANNEL_PAGE27_SUB_GHZ:
    {
      start_logical_channel = ZB_PAGE27_SUB_GHZ_START_LOGICAL_CHANNEL;
      max_logical_channel = ZB_PAGE27_SUB_GHZ_MAX_LOGICAL_CHANNEL;
      break;
    }
    case ZB_CHANNEL_PAGE28_SUB_GHZ:
    {
      start_logical_channel = ZB_PAGE28_SUB_GHZ_START_LOGICAL_CHANNEL;
      max_logical_channel = ZB_PAGE28_SUB_GHZ_MAX_LOGICAL_CHANNEL;
      break;
    }
    case ZB_CHANNEL_PAGE29_SUB_GHZ:
    {
      start_logical_channel = ZB_PAGE29_SUB_GHZ_START_LOGICAL_CHANNEL;
      /* Use pre-max channel, max will be considered below. */
      max_logical_channel = ZB_PAGE29_SUB_GHZ_PRE_MAX_LOGICAL_CHANNEL;
      break;
    }
    case ZB_CHANNEL_PAGE30_SUB_GHZ:
    {
      start_logical_channel = ZB_PAGE30_SUB_GHZ_START_LOGICAL_CHANNEL;
      max_logical_channel = ZB_PAGE30_SUB_GHZ_MAX_LOGICAL_CHANNEL;
      break;
    }
    case ZB_CHANNEL_PAGE31_SUB_GHZ:
    {
      start_logical_channel = ZB_PAGE31_SUB_GHZ_START_LOGICAL_CHANNEL;
      max_logical_channel = ZB_PAGE31_SUB_GHZ_MAX_LOGICAL_CHANNEL;
      break;
    }
    default:
    {
      ret = RET_NOT_FOUND;
      TRACE_MSG(TRACE_ERROR, "Unknown page %hd", (FMT__H, page));
      break;
    }
  }

  if (ret == RET_OK)
  {
    if (start_logical_channel <= logical_channel &&
        logical_channel <= max_logical_channel)
    {
      *channel_number = logical_channel - start_logical_channel + start_channel;
    }
    else if (page == ZB_CHANNEL_PAGE29_SUB_GHZ &&
             logical_channel == ZB_PAGE29_SUB_GHZ_MAX_LOGICAL_CHANNEL)
    {
      *channel_number = ZB_PAGE29_SUB_GHZ_MAX_CHANNEL_NUMBER;
    }
    else
    {
      ret = RET_INVALID_PARAMETER;
      TRACE_MSG(TRACE_ERROR, "Unacceptable logical channel %hd for page %hd",
                (FMT__H_H, logical_channel, page));
    }
  }

  return ret;
}

zb_ret_t zb_channel_page_channel_number_to_logical(zb_uint8_t page,
                                                   zb_uint8_t channel_number,
                                                   zb_uint8_t *logical_channel)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t start_channel_number = 0;
  zb_uint8_t max_channel_number = 0;
  zb_uint8_t start_logical_channel_number = 0;

  ZB_ASSERT(logical_channel != NULL);

  *logical_channel = 0xFF;

  switch (page)
  {
    case ZB_CHANNEL_PAGE0_2_4_GHZ:
    {
      start_channel_number = ZB_PAGE0_2_4_GHZ_START_CHANNEL_NUMBER;
      max_channel_number = ZB_PAGE0_2_4_GHZ_MAX_CHANNEL_NUMBER;
      start_logical_channel_number = ZB_PAGE0_2_4_GHZ_START_LOGICAL_CHANNEL;
      break;
    }
    case ZB_CHANNEL_PAGE23_SUB_GHZ:
    {
      start_channel_number = ZB_PAGE23_SUB_GHZ_START_CHANNEL_NUMBER;
      max_channel_number = ZB_PAGE23_SUB_GHZ_MAX_CHANNEL_NUMBER;
      start_logical_channel_number = ZB_PAGE23_SUB_GHZ_START_LOGICAL_CHANNEL;
      break;
    }
    case ZB_CHANNEL_PAGE24_SUB_GHZ:
    {
      start_channel_number = ZB_PAGE24_SUB_GHZ_START_CHANNEL_NUMBER;
      max_channel_number = ZB_PAGE24_SUB_GHZ_MAX_CHANNEL_NUMBER;
      start_logical_channel_number = ZB_PAGE24_SUB_GHZ_START_LOGICAL_CHANNEL;
      break;
    }
    case ZB_CHANNEL_PAGE25_SUB_GHZ:
    {
      start_channel_number = ZB_PAGE25_SUB_GHZ_START_CHANNEL_NUMBER;
      max_channel_number = ZB_PAGE25_SUB_GHZ_MAX_CHANNEL_NUMBER;
      start_logical_channel_number = ZB_PAGE25_SUB_GHZ_START_LOGICAL_CHANNEL;
      break;
    }
    case ZB_CHANNEL_PAGE26_SUB_GHZ:
    {
      start_channel_number = ZB_PAGE26_SUB_GHZ_START_CHANNEL_NUMBER;
      max_channel_number = ZB_PAGE26_SUB_GHZ_MAX_CHANNEL_NUMBER;
      start_logical_channel_number = ZB_PAGE26_SUB_GHZ_START_LOGICAL_CHANNEL;
      break;
    }
    case ZB_CHANNEL_PAGE27_SUB_GHZ:
    {
      start_channel_number = ZB_PAGE27_SUB_GHZ_START_CHANNEL_NUMBER;
      max_channel_number = ZB_PAGE27_SUB_GHZ_MAX_CHANNEL_NUMBER;
      start_logical_channel_number = ZB_PAGE27_SUB_GHZ_START_LOGICAL_CHANNEL;
      break;
    }
    case ZB_CHANNEL_PAGE28_SUB_GHZ:
    {
      start_channel_number = ZB_PAGE28_SUB_GHZ_START_CHANNEL_NUMBER;
      max_channel_number = ZB_PAGE28_SUB_GHZ_MAX_CHANNEL_NUMBER;
      start_logical_channel_number = ZB_PAGE28_SUB_GHZ_START_LOGICAL_CHANNEL;
      break;
    }
    case ZB_CHANNEL_PAGE29_SUB_GHZ:
    {
      start_channel_number = ZB_PAGE29_SUB_GHZ_START_CHANNEL_NUMBER;
      max_channel_number = ZB_PAGE29_SUB_GHZ_MAX_CHANNEL_NUMBER;
      start_logical_channel_number = ZB_PAGE29_SUB_GHZ_START_LOGICAL_CHANNEL;
      break;
    }
    case ZB_CHANNEL_PAGE30_SUB_GHZ:
    {
      start_channel_number = ZB_PAGE30_SUB_GHZ_START_CHANNEL_NUMBER;
      max_channel_number = ZB_PAGE30_SUB_GHZ_MAX_CHANNEL_NUMBER;
      start_logical_channel_number = ZB_PAGE30_SUB_GHZ_START_LOGICAL_CHANNEL;
      break;
    }
    case ZB_CHANNEL_PAGE31_SUB_GHZ:
    {
      start_channel_number = ZB_PAGE31_SUB_GHZ_START_CHANNEL_NUMBER;
      max_channel_number = ZB_PAGE31_SUB_GHZ_MAX_CHANNEL_NUMBER;
      start_logical_channel_number = ZB_PAGE31_SUB_GHZ_START_LOGICAL_CHANNEL;
      break;
    }
    default:
    {
      ret = RET_NOT_FOUND;
      TRACE_MSG(TRACE_ERROR, "Unknown page %hd", (FMT__H, page));
      break;
    }
  }

  if (ret == RET_OK)
  {
    if (start_channel_number <= channel_number &&
        channel_number <= max_channel_number)
    {
      if (page == ZB_CHANNEL_PAGE29_SUB_GHZ &&
          channel_number == ZB_PAGE29_SUB_GHZ_MAX_CHANNEL_NUMBER)
      {
        *logical_channel = ZB_PAGE29_SUB_GHZ_MAX_LOGICAL_CHANNEL;
      }
      else
      {
        if (page == ZB_CHANNEL_PAGE0_2_4_GHZ)
        {
          *logical_channel = channel_number;
        }
        else
        {
          *logical_channel = channel_number + start_logical_channel_number;
        }
        TRACE_MSG(TRACE_COMMON1, "zb_channel_page_channel_number_to_logical: channel %hd",
                  (FMT__H, *logical_channel));
      }
    }

    if (*logical_channel == 0xFFU)
    {
      ret = RET_INVALID_PARAMETER;
      TRACE_MSG(TRACE_ERROR, "Unacceptable channel number %hd for page %hd",
                (FMT__H_H, channel_number, page));
    }
  }

  return ret;
}

static zb_ret_t zb_channel_page_list_set_unset_channel(zb_channel_list_t list,
                                                       zb_uint8_t         idx,
                                                       zb_uint8_t         channel_number,
                                                       zb_bool_t          unset)
{
  zb_ret_t ret;
  zb_uint8_t channel_page;
  zb_uint8_t max_channel_number;
  zb_uint32_t mask;

  ZB_ASSERT(list != NULL);
  ZB_ASSERT(idx < ZB_CHANNEL_PAGES_NUM);

  channel_page = (zb_uint8_t)ZB_CHANNEL_PAGE_GET_PAGE(list[idx]);
  ret = zb_channel_page_get_max_channel_number(channel_page, &max_channel_number);
  ZB_ASSERT(ret == RET_OK);

  if (channel_number > max_channel_number)
  {
    ret = RET_INVALID_PARAMETER;
    TRACE_MSG(TRACE_ERROR, "Too big channel number %hd specified for channel page %hd",
              (FMT__H_H, channel_number, channel_page));
  }
  else
  {
    mask = ZB_CHANNEL_PAGE_GET_MASK(list[idx]);
    if (unset)
    {
      mask &= ~(1UL << channel_number);
    }
    else
    {
      mask |= (1UL << channel_number);
    }
    ZB_CHANNEL_PAGE_SET_MASK(list[idx], mask);
  }

  return ret;
}

zb_ret_t zb_channel_page_list_set_channel(zb_channel_list_t list,
                                          zb_uint8_t         idx,
                                          zb_uint8_t         channel_number)
{
  return zb_channel_page_list_set_unset_channel(list, idx, channel_number, ZB_FALSE);
}

zb_ret_t zb_channel_page_list_set_logical_channel(zb_channel_list_t list,
                                                  zb_uint8_t         page,
                                                  zb_uint8_t         channel_number)
{
  zb_ret_t ret = zb_channel_page_channel_logical_to_number(page, channel_number, &channel_number);
  if (ret == RET_OK)
  {
    ret = zb_channel_page_list_set_unset_channel(list, ZB_CHANNEL_PAGE_TO_IDX(page), channel_number, ZB_FALSE);
  }
  return ret;
}

zb_ret_t zb_channel_page_list_unset_channel(zb_channel_list_t list,
                                            zb_uint8_t         idx,
                                            zb_uint8_t         channel_number)
{
  return zb_channel_page_list_set_unset_channel(list, idx, channel_number, ZB_TRUE);
}

zb_uint32_t zb_channel_page_list_get_mask(zb_channel_list_t list,
                                          zb_uint8_t         idx)
{
  ZB_ASSERT(list != NULL);
  ZB_ASSERT(idx < ZB_CHANNEL_PAGES_NUM);

  return ZB_CHANNEL_PAGE_GET_MASK(list[idx]);
}

void zb_channel_page_list_set_page(zb_channel_list_t list,
                                        zb_uint8_t         idx,
                                        zb_uint8_t         page)
{
  ZB_ASSERT(list != NULL);
  ZB_ASSERT(idx < ZB_CHANNEL_PAGES_NUM);

  ZB_CHANNEL_PAGE_SET_PAGE(list[idx], page);
}

zb_uint8_t zb_channel_page_list_get_page(zb_channel_list_t list,
                                         zb_uint8_t         idx)
{
  ZB_ASSERT(list != NULL);
  ZB_ASSERT(idx < ZB_CHANNEL_PAGES_NUM);

  return (zb_uint8_t)ZB_CHANNEL_PAGE_GET_PAGE(list[idx]);
}

zb_uint8_t zb_channel_page_list_get_channels_num(zb_channel_list_t list,
                                                 zb_uint8_t         idx)
{
  zb_ret_t ret;
  zb_uint8_t i;
  zb_uint8_t start_channel_number;
  zb_uint8_t max_channel_number;
  zb_uint8_t page;
  zb_uint32_t channel_mask;
  zb_uint8_t channels_num = 0;

  ZB_ASSERT(list != NULL);
  ZB_ASSERT(idx < ZB_CHANNEL_PAGES_NUM);

  channel_mask = ZB_CHANNEL_PAGE_GET_MASK(list[idx]);

  page = zb_channel_page_list_get_page(list, idx);

  ret = zb_channel_page_get_start_channel_number(page, &start_channel_number);
  ZB_ASSERT(ret == RET_OK);
  ret = zb_channel_page_get_max_channel_number(page, &max_channel_number);
  ZB_ASSERT(ret == RET_OK);
  ZVUNUSED(ret);

  for (i = start_channel_number; i <= max_channel_number; i++)
  {
    channels_num += (zb_uint8_t)((channel_mask >> i) & 1U);
    /* TRACE_MSG(TRACE_NWK3, "channel_mask 0x%x i %hd channels_num %hd", (FMT__D_H_H, channel_mask, i, channels_num)); */
  }

  TRACE_MSG(TRACE_COMMON1, "zb_channel_page_list_get_channels_num ret %hd", (FMT__H, channels_num));
  return channels_num;
}

zb_ret_t zb_channel_page_list_get_page_idx(zb_uint8_t page, zb_uint8_t *idx)
{
  zb_ret_t ret = RET_OK;

  ZB_ASSERT(idx != NULL);

  switch (page)
  {
    case ZB_CHANNEL_PAGE0_2_4_GHZ:
    {
      *idx = ZB_CHANNEL_LIST_PAGE0_IDX;
      break;
    }
    case ZB_CHANNEL_PAGE23_SUB_GHZ:
    {
      *idx = ZB_CHANNEL_LIST_PAGE23_IDX;
      break;
    }
    case ZB_CHANNEL_PAGE24_SUB_GHZ:
    {
      *idx = ZB_CHANNEL_LIST_PAGE24_IDX;
      break;
    }
    case ZB_CHANNEL_PAGE25_SUB_GHZ:
    {
      *idx = ZB_CHANNEL_LIST_PAGE25_IDX;
      break;
    }
    case ZB_CHANNEL_PAGE26_SUB_GHZ:
    {
      *idx = ZB_CHANNEL_LIST_PAGE26_IDX;
      break;
    }
    case ZB_CHANNEL_PAGE27_SUB_GHZ:
    {
      *idx = ZB_CHANNEL_LIST_PAGE27_IDX;
      break;
    }
    case ZB_CHANNEL_PAGE28_SUB_GHZ:
    {
      *idx = ZB_CHANNEL_LIST_PAGE28_IDX;
      break;
    }
    case ZB_CHANNEL_PAGE29_SUB_GHZ:
    {
      *idx = ZB_CHANNEL_LIST_PAGE29_IDX;
      break;
    }
    case ZB_CHANNEL_PAGE30_SUB_GHZ:
    {
      *idx = ZB_CHANNEL_LIST_PAGE30_IDX;
      break;
    }
    case ZB_CHANNEL_PAGE31_SUB_GHZ:
    {
      *idx = ZB_CHANNEL_LIST_PAGE31_IDX;
      break;
    }
    default:
    {
      *idx = 0;
      ret = RET_NOT_FOUND;
      TRACE_MSG(TRACE_ERROR, "Unknown page %hd", (FMT__H, page));
      break;
    }
  }

  return ret;
}

zb_ret_t zb_channel_page_get_page_by_idx(zb_uint8_t idx, zb_uint8_t *page)
{
  zb_ret_t ret = RET_OK;

  ZB_ASSERT(page != NULL);

  switch (idx)
  {
    case ZB_CHANNEL_LIST_PAGE0_IDX:
    {
      *page = ZB_CHANNEL_PAGE0_2_4_GHZ;
      break;
    }
    case ZB_CHANNEL_LIST_PAGE23_IDX:
    {
      *page = ZB_CHANNEL_PAGE23_SUB_GHZ;
      break;
    }
    case ZB_CHANNEL_LIST_PAGE24_IDX:
    {
      *page = ZB_CHANNEL_PAGE24_SUB_GHZ;
      break;
    }
    case ZB_CHANNEL_LIST_PAGE25_IDX:
    {
      *page = ZB_CHANNEL_PAGE25_SUB_GHZ;
      break;
    }
    case ZB_CHANNEL_LIST_PAGE26_IDX:
    {
      *page = ZB_CHANNEL_PAGE26_SUB_GHZ;
      break;
    }
    case ZB_CHANNEL_LIST_PAGE27_IDX:
    {
      *page = ZB_CHANNEL_PAGE27_SUB_GHZ;
      break;
    }
    case ZB_CHANNEL_LIST_PAGE28_IDX:
    {
      *page = ZB_CHANNEL_PAGE28_SUB_GHZ;
      break;
    }
    case ZB_CHANNEL_LIST_PAGE29_IDX:
    {
      *page = ZB_CHANNEL_PAGE29_SUB_GHZ;
      break;
    }
    case ZB_CHANNEL_LIST_PAGE30_IDX:
    {
      *page = ZB_CHANNEL_PAGE30_SUB_GHZ;
      break;
    }
    case ZB_CHANNEL_LIST_PAGE31_IDX:
    {
      *page = ZB_CHANNEL_PAGE31_SUB_GHZ;
      break;
    }
    default:
    {
      *page = 0;
      ret = RET_NOT_FOUND;
      TRACE_MSG(TRACE_ERROR, "Unknown page_idx %hd", (FMT__H, idx));
      break;
    }
  }

  return ret;
}

zb_uint32_t zb_channel_page_get_all_channels_mask_by_page(zb_uint8_t page)
{
  zb_uint32_t ret = 0;

  switch (page)
  {
    case ZB_CHANNEL_PAGE0_2_4_GHZ:
      ret = ZB_CHANNEL_PAGE_GET_MASK(ZB_TRANSCEIVER_ALL_CHANNELS_MASK);
      break;
#if defined ZB_SUBGHZ_BAND_ENABLED
    case ZB_CHANNEL_PAGE23_SUB_GHZ:
      ret = ZB_CHANNEL_PAGE_GET_MASK(ZB_TRANSCEIVER_ALL_CHANNELS_MASK_PAGE23);
      break;
    case ZB_CHANNEL_PAGE24_SUB_GHZ:
      ret = ZB_CHANNEL_PAGE_GET_MASK(ZB_TRANSCEIVER_ALL_CHANNELS_MASK_PAGE24);
      break;
    case ZB_CHANNEL_PAGE25_SUB_GHZ:
      ret = ZB_CHANNEL_PAGE_GET_MASK(ZB_TRANSCEIVER_ALL_CHANNELS_MASK_PAGE25);
      break;
    case ZB_CHANNEL_PAGE26_SUB_GHZ:
      ret = ZB_CHANNEL_PAGE_GET_MASK(ZB_TRANSCEIVER_ALL_CHANNELS_MASK_PAGE26);
      break;
    case ZB_CHANNEL_PAGE27_SUB_GHZ:
      ret = ZB_CHANNEL_PAGE_GET_MASK(ZB_TRANSCEIVER_ALL_CHANNELS_MASK_PAGE27);
      break;
    case ZB_CHANNEL_PAGE28_SUB_GHZ:
      ret = ZB_CHANNEL_PAGE_GET_MASK(ZB_TRANSCEIVER_ALL_CHANNELS_MASK_PAGE28);
      break;
    case ZB_CHANNEL_PAGE29_SUB_GHZ:
      ret = ZB_CHANNEL_PAGE_GET_MASK(ZB_TRANSCEIVER_ALL_CHANNELS_MASK_PAGE29);
      break;
    case ZB_CHANNEL_PAGE30_SUB_GHZ:
      ret = ZB_CHANNEL_PAGE_GET_MASK(ZB_TRANSCEIVER_ALL_CHANNELS_MASK_PAGE30);
      break;
    case ZB_CHANNEL_PAGE31_SUB_GHZ:
      ret = ZB_CHANNEL_PAGE_GET_MASK(ZB_TRANSCEIVER_ALL_CHANNELS_MASK_PAGE31);
      break;
#endif /* ZB_SUBGHZ_BAND_ENABLED */
    default:
      break;
  }

  return ret;
}

static zb_ret_t channel_page_get_edge_channel_number(zb_uint8_t page, zb_bool_t max, zb_uint8_t *channel_number)
{
  zb_ret_t ret = RET_OK;

  switch (page)
  {
    case ZB_CHANNEL_PAGE0_2_4_GHZ:
    {
      *channel_number = max ?
        ZB_PAGE0_2_4_GHZ_MAX_CHANNEL_NUMBER :
        ZB_PAGE0_2_4_GHZ_START_CHANNEL_NUMBER;
      break;
    }
    case ZB_CHANNEL_PAGE23_SUB_GHZ:
    {
      *channel_number = max ?
        ZB_PAGE23_SUB_GHZ_MAX_CHANNEL_NUMBER :
        ZB_PAGE23_SUB_GHZ_START_CHANNEL_NUMBER;
      break;
    }
    case ZB_CHANNEL_PAGE24_SUB_GHZ:
    {
      *channel_number = max ?
        ZB_PAGE24_SUB_GHZ_MAX_CHANNEL_NUMBER :
        ZB_PAGE24_SUB_GHZ_START_CHANNEL_NUMBER;
      break;
    }
    case ZB_CHANNEL_PAGE25_SUB_GHZ:
    {
      *channel_number = max ?
        ZB_PAGE25_SUB_GHZ_MAX_CHANNEL_NUMBER :
        ZB_PAGE25_SUB_GHZ_START_CHANNEL_NUMBER;
      break;
    }
    case ZB_CHANNEL_PAGE26_SUB_GHZ:
    {
      *channel_number = max ?
        ZB_PAGE26_SUB_GHZ_MAX_CHANNEL_NUMBER :
        ZB_PAGE26_SUB_GHZ_START_CHANNEL_NUMBER;
      break;
    }
    case ZB_CHANNEL_PAGE27_SUB_GHZ:
    {
      *channel_number = max ?
        ZB_PAGE27_SUB_GHZ_MAX_CHANNEL_NUMBER :
        ZB_PAGE27_SUB_GHZ_START_CHANNEL_NUMBER;
      break;
    }
    case ZB_CHANNEL_PAGE28_SUB_GHZ:
    {
      *channel_number = max ?
        ZB_PAGE28_SUB_GHZ_MAX_CHANNEL_NUMBER :
        ZB_PAGE28_SUB_GHZ_START_CHANNEL_NUMBER;
      break;
    }
    case ZB_CHANNEL_PAGE29_SUB_GHZ:
    {
      *channel_number = max ?
        ZB_PAGE29_SUB_GHZ_MAX_CHANNEL_NUMBER :
        ZB_PAGE29_SUB_GHZ_START_CHANNEL_NUMBER;
      break;
    }
    case ZB_CHANNEL_PAGE30_SUB_GHZ:
    {
      *channel_number = max ?
        ZB_PAGE30_SUB_GHZ_MAX_CHANNEL_NUMBER :
        ZB_PAGE30_SUB_GHZ_START_CHANNEL_NUMBER;
      break;
    }
    case ZB_CHANNEL_PAGE31_SUB_GHZ:
    {
      *channel_number = max ?
        ZB_PAGE31_SUB_GHZ_MAX_CHANNEL_NUMBER :
        ZB_PAGE31_SUB_GHZ_START_CHANNEL_NUMBER;
      break;
    }
    default:
    {
      *channel_number = 0;
      ret = RET_NOT_FOUND;
      TRACE_MSG(TRACE_ERROR, "Unknown page %hd", (FMT__H, page));
      break;
    }
  }

  return ret;
}

zb_ret_t zb_channel_page_get_start_channel_number(zb_uint8_t page, zb_uint8_t *channel_number)
{
  ZB_ASSERT(channel_number != NULL);

  return channel_page_get_edge_channel_number(page, ZB_FALSE, channel_number);
}

zb_ret_t zb_channel_page_get_max_channel_number(zb_uint8_t page, zb_uint8_t *channel_number)
{
  ZB_ASSERT(channel_number != NULL);

  return channel_page_get_edge_channel_number(page, ZB_TRUE, channel_number);
}

void zb_channel_page_list_set_2_4GHz_mask(zb_channel_list_t list,
                                          zb_uint32_t        mask)
{
  zb_channel_page_list_set_mask(list, ZB_CHANNEL_LIST_PAGE0_IDX, mask);
}

zb_uint32_t zb_channel_page_list_get_2_4GHz_mask(zb_channel_list_t list)
{
  return zb_channel_page_list_get_mask(list, ZB_CHANNEL_LIST_PAGE0_IDX);
}

zb_uint8_t zb_channel_page_list_get_first_filled_page(zb_channel_list_t list)
{
  zb_uint8_t i;

  for (i = ZB_CHANNEL_LIST_PAGE0_IDX; i < ZB_CHANNEL_PAGES_NUM; ++i)
  {
    if (zb_channel_page_list_get_mask(list, i) != 0U)
    {
      break;
    }
  }

  return i;
}

void zb_channel_list_init(zb_channel_list_t channel_list)
{
  ZB_ASSERT(channel_list != NULL);
  ZB_BZERO(channel_list, sizeof(zb_channel_list_t));
  setup_channel_pages(channel_list);
}


zb_ret_t zb_channel_list_add(zb_channel_list_t channel_list, zb_uint8_t page_num, zb_uint32_t channel_mask)
{
  zb_ret_t ret = RET_OK;

  ZB_ASSERT(channel_list != NULL);

  switch (page_num)
  {
    case ZB_CHANNEL_PAGE0_2_4_GHZ:
      ZB_CHANNEL_PAGE_SET_MASK(channel_list[ZB_CHANNEL_LIST_PAGE0_IDX], channel_mask);
      break;
#if defined ZB_SUBGHZ_BAND_ENABLED
    case ZB_CHANNEL_PAGE23_SUB_GHZ:
      ZB_CHANNEL_PAGE_SET_MASK(channel_list[ZB_CHANNEL_LIST_PAGE23_IDX], channel_mask);
      break;
    case ZB_CHANNEL_PAGE24_SUB_GHZ:
      ZB_CHANNEL_PAGE_SET_MASK(channel_list[ZB_CHANNEL_LIST_PAGE24_IDX], channel_mask);
      break;
    case ZB_CHANNEL_PAGE25_SUB_GHZ:
      ZB_CHANNEL_PAGE_SET_MASK(channel_list[ZB_CHANNEL_LIST_PAGE25_IDX], channel_mask);
      break;
    case ZB_CHANNEL_PAGE26_SUB_GHZ:
      ZB_CHANNEL_PAGE_SET_MASK(channel_list[ZB_CHANNEL_LIST_PAGE26_IDX], channel_mask);
      break;
    case ZB_CHANNEL_PAGE27_SUB_GHZ:
      ZB_CHANNEL_PAGE_SET_MASK(channel_list[ZB_CHANNEL_LIST_PAGE27_IDX], channel_mask);
      break;
    case ZB_CHANNEL_PAGE28_SUB_GHZ:
      ZB_CHANNEL_PAGE_SET_MASK(channel_list[ZB_CHANNEL_LIST_PAGE28_IDX], channel_mask);
      break;
    case ZB_CHANNEL_PAGE29_SUB_GHZ:
      ZB_CHANNEL_PAGE_SET_MASK(channel_list[ZB_CHANNEL_LIST_PAGE29_IDX], channel_mask);
      break;
    case ZB_CHANNEL_PAGE30_SUB_GHZ:
      ZB_CHANNEL_PAGE_SET_MASK(channel_list[ZB_CHANNEL_LIST_PAGE30_IDX], channel_mask);
      break;
    case ZB_CHANNEL_PAGE31_SUB_GHZ:
      ZB_CHANNEL_PAGE_SET_MASK(channel_list[ZB_CHANNEL_LIST_PAGE31_IDX], channel_mask);
      break;
#endif /* ZB_SUBGHZ_BAND_ENABLED */
    default:
      TRACE_MSG(TRACE_ERROR, "Unknown page_num %hd", (FMT__H, page_num));
      ret = RET_INVALID_PARAMETER;
      break;
  }
  return ret;
}

#ifdef ZB_PAGES_REMAP_TO_2_4GHZ
zb_uint8_t zb_pages_remap_logical_channel(zb_uint8_t channel_page,
                                          zb_uint8_t logical_channel)
{
  zb_ret_t ret;
  zb_uint8_t channel_number;
  zb_uint8_t remap_logical_channel;

  remap_logical_channel = 0xFF;

  ret = zb_channel_page_channel_logical_to_number(channel_page, logical_channel,
                                                  &channel_number);
  TRACE_MSG(TRACE_APP1, "zb_pages_remap_logical_channel channel_page %hd logical_channel %hd ret1 %d channel_number %hd",
            (FMT__H_H_D_H, channel_page, logical_channel, ret, channel_number));
  if (ret == RET_OK)
  {
    remap_logical_channel = ZB_PAGES_REMAP_CHANNEL_NUMBER(channel_page, channel_number);
    TRACE_MSG(TRACE_APP1, "remap_logical_channel %hd", (FMT__H, remap_logical_channel));
    zb_channel_page_channel_number_to_logical(0, remap_logical_channel, &remap_logical_channel);
    TRACE_MSG(TRACE_APP1, "remap_logical_channel (logical) %hd", (FMT__H, remap_logical_channel));
  }
  return remap_logical_channel;
}
#endif

#ifdef ZB_MAC_CONFIGURABLE_TX_POWER
void zb_channel_get_tx_power_offset(zb_uint8_t page, zb_uint8_t channel,
                                    zb_uint8_t *array_idx, zb_uint8_t *array_ofs)
{
  ZB_ASSERT((array_idx != NULL) && (array_ofs != NULL));

  /* default invalid value */
  *array_idx = (zb_uint8_t)-1;
  *array_ofs = (zb_uint8_t)-1;

  switch (page)
  {
    case ZB_CHANNEL_PAGE0_2_4_GHZ: /* 0..26 --> 0..26 */
      *array_idx = ZB_CHANNEL_LIST_PAGE0_IDX;

      if (channel >= ZB_PAGE0_2_4_GHZ_CHANNEL_FROM
          && channel <= ZB_PAGE0_2_4_GHZ_CHANNEL_TO)
      {
        *array_ofs = channel;
      }
      break;

    case ZB_CHANNEL_PAGE23_SUB_GHZ:
      *array_idx = ZB_CHANNEL_LIST_PAGE23_IDX;

      if (channel <= ZB_PAGE23_SUB_GHZ_CHANNEL_TO)
      {
        *array_ofs = channel;
      }
      break;

    case ZB_CHANNEL_PAGE24_SUB_GHZ:
      *array_idx = ZB_CHANNEL_LIST_PAGE24_IDX;

      if (channel >= ZB_PAGE24_SUB_GHZ_CHANNEL_FROM
          && channel <= ZB_PAGE24_SUB_GHZ_CHANNEL_TO)
      {
        *array_ofs = channel - ZB_PAGE24_SUB_GHZ_CHANNEL_FROM;
      }
      break;

    case ZB_CHANNEL_PAGE25_SUB_GHZ:
      *array_idx = ZB_CHANNEL_LIST_PAGE25_IDX;

      if (channel <= ZB_PAGE25_SUB_GHZ_CHANNEL_TO)
      {
        *array_ofs = channel;
      }
      break;

    case ZB_CHANNEL_PAGE26_SUB_GHZ:
      *array_idx = ZB_CHANNEL_LIST_PAGE26_IDX;

      if (channel >= ZB_PAGE26_SUB_GHZ_CHANNEL_FROM
          && channel <= ZB_PAGE26_SUB_GHZ_CHANNEL_TO)
      {
        *array_ofs = channel - ZB_PAGE26_SUB_GHZ_CHANNEL_FROM;
      }
      break;

    case ZB_CHANNEL_PAGE27_SUB_GHZ:
      *array_idx = ZB_CHANNEL_LIST_PAGE27_IDX;

      if (channel >= ZB_PAGE27_SUB_GHZ_CHANNEL_FROM
          && channel <= ZB_PAGE27_SUB_GHZ_CHANNEL_TO)
      {
        *array_ofs = channel - ZB_PAGE27_SUB_GHZ_CHANNEL_FROM;
      }
      break;

    case ZB_CHANNEL_PAGE28_SUB_GHZ: /* 0..26 --> 0..26 */
      *array_idx = ZB_CHANNEL_LIST_PAGE28_IDX;

      if (channel <= ZB_PAGE28_SUB_GHZ_CHANNEL_TO)
      {
        *array_ofs = channel;
      }
      break;

    case ZB_CHANNEL_PAGE29_SUB_GHZ: /* 27..34, 62 --> 0..7, 8 */
      *array_idx = ZB_CHANNEL_LIST_PAGE29_IDX;

      if (channel >= ZB_PAGE29_SUB_GHZ_CHANNEL_FROM
          && channel <= ZB_PAGE29_SUB_GHZ_CHANNEL_TO)
      {
        *array_ofs = channel - ZB_PAGE29_SUB_GHZ_CHANNEL_FROM;
      }
      else if (channel == 62U)
      {
        *array_ofs = 8;
      }
      else
      {
        /* MISRA rule 15.7 requires empty 'else' branch. */
      }
      break;

    case ZB_CHANNEL_PAGE30_SUB_GHZ: /* 35..61 --> 0..26 */
      *array_idx = ZB_CHANNEL_LIST_PAGE30_IDX;

      if (channel >= ZB_PAGE30_SUB_GHZ_CHANNEL_FROM
          && channel <= ZB_PAGE30_SUB_GHZ_CHANNEL_TO)
      {
        *array_ofs = channel - ZB_PAGE30_SUB_GHZ_CHANNEL_FROM;
      }
      break;

    case ZB_CHANNEL_PAGE31_SUB_GHZ:  /* 0..26 --> 0..26 */
      *array_idx = ZB_CHANNEL_LIST_PAGE31_IDX;

      if (channel <= ZB_PAGE31_SUB_GHZ_CHANNEL_TO)
      {
        *array_ofs = channel;
      }
      break;

    default:
      /* trace error? */
      ZB_ASSERT(0);
      break;
  }
}
#endif /* ZB_MAC_CONFIGURABLE_TX_POWER */

/*! @} */
