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
/* PURPOSE: Typical ZDO applications: ZC, ZR, ZE
*/

#define ZB_TRACE_FILE_ID 49
#include "zb_common.h"
#include "zb_sniffer.h"

#ifdef ZB_PROMISCUOUS_MODE
static void zboss_sniffer_mac_started(zb_uint8_t param);

static zb_uint8_t g_sniffer_page = ZB_SNIFFER_PAGE_NOT_SET;

static void zboss_sniffer_mac_started(zb_uint8_t param);

static void zboss_sniffer_direct_pib_set(zb_uint8_t attr, void *value, zb_uint8_t value_size)
{
  zb_bufid_t buf = zb_buf_get_any();

  if (buf != 0U)
  {
    zb_mlme_set_request_t *req;

    TRACE_MSG(TRACE_ERROR, "zboss_sniffer_direct_pib_set: attr = %d", (FMT__D, attr));
    req = zb_buf_initial_alloc(buf, sizeof(zb_mlme_set_request_t) + value_size);
    req->pib_attr = attr;
    req->pib_index = 0;
    req->pib_length = value_size;
    ZB_MEMCPY((req+1), value, value_size);
    req->confirm_cb_u.cb = NULL;
    zb_mlme_set_request(buf);
  }
}


zb_ret_t zboss_start_in_sniffer_mode(void)
{
  zb_bufid_t buf = zb_buf_get_out();
  /* At start we have free buffer for sure, so no need to use blocking buffer alloc */
  ZB_ASSERT(buf);

  ZDO_CTX().continue_start_after_nwk_cb = zboss_sniffer_mac_started;
  g_sniffer_page = ZB_SNIFFER_PAGE_NOT_SET;

  /* Reset NWK. It will reset MAC and call zb_nlme_reset_confirm(). */
  /* Do not need to do nib_reinit here */
  zdo_call_nlme_reset(buf, ZB_FALSE, ZB_FALSE, zb_nwk_load_pib);

  return RET_OK;
}

static void zboss_sniffer_mac_started(zb_uint8_t param)
{
  zb_uint8_t value = 1;

  zboss_sniffer_direct_pib_set(ZB_PIB_ATTRIBUTE_PROMISCUOUS_MODE, (void *)&value, 1);
  (void)zb_app_signal_pack(param, ZB_ZDO_SIGNAL_DEFAULT_START, 0, 0);
  ZB_SCHEDULE_CALLBACK(zboss_signal_handler, param);
}


void zboss_sniffer_stop(void)
{
  zb_uint8_t value = 0;

  g_sniffer_page = ZB_SNIFFER_PAGE_NOT_SET;
  zboss_sniffer_direct_pib_set(ZB_PIB_ATTRIBUTE_PROMISCUOUS_RX_ON, (void *)&value, 1);
}

zb_ret_t zboss_sniffer_set_channel_page(zb_uint8_t channel_page_cmd)
{
  zb_uint8_t logical_channel;
  zb_uint8_t page;
  zb_uint8_t page_idx;
  zb_ret_t ret;

  if (g_sniffer_page == ZB_SNIFFER_PAGE_NOT_SET)
  {
    /*
     * Page and channel may be received as 2 bytes (1st for page and 2nd for channel) or packed in single byte.
     * For single byte cases, the following pattern is found:
     * -> 0bPPPC CCCC (P - Page, C - Channel).
     * -> 3 MS bits for pages 0, [28;31].
     * -> remaining 5 bits for channel.
     * Out of the 8 combinations for the 'single byte' interval, 3 are free and we use 1 to flag that it is a 2 byte scheme.
     * 0b000c cccc -> page 0
     * 0b001c cccc -> page 28
     * 0b010c cccc -> page 29
     * 0b011c cccc -> page 30
     * 0b100c cccc -> page 31
     * 0b101c cccc -> free
     * 0b110c cccc -> free
     * 0b111c cccc -> used by sniffer GUI/console to indicate 2 byte scenario
     */

    if ((channel_page_cmd & ZB_PAGE_AND_CHANNEL_PACKED_PAGE_BITS_MASK)
        == ZB_PAGE_AND_CHANNEL_PACKED_PAGE_BITS_MASK)
    {
      channel_page_cmd &= ZB_PAGE_AND_CHANNEL_2_BYTE_SIGNAL_REMOVE;
      if (channel_page_cmd >= ZB_CHANNEL_LIST_PAGE23_IDX && channel_page_cmd <= ZB_CHANNEL_LIST_PAGE27_IDX)
      {
        g_sniffer_page = channel_page_cmd;
        /* Waiting channel */
        ret = RET_BLOCKED;
      }
      else
      {
        ret = RET_ERROR;
      }
    }
    else /* 1 byte for channel and page */
    {
      page_idx
          = ((channel_page_cmd & ZB_PAGE_AND_CHANNEL_PACKED_PAGE_BITS_MASK) >> ZB_PAGE_AND_CHANNEL_PACKED_PAGE_SHIFT);

      if (page_idx == ZB_CHANNEL_LIST_PAGE_0_PACKED_IDX
          || (page_idx >= ZB_CHANNEL_LIST_PAGE_28_PACKED_IDX && page_idx <= ZB_CHANNEL_LIST_PAGE_31_PACKED_IDX))
      {

        TRACE_MSG(TRACE_ERROR, "zboss_sniffer_start page_idx %hd ch %hd",
                  (FMT__H_H, page_idx,
                   channel_page_cmd & ZB_PAGE_AND_CHANNEL_PACKED_CHANNEL_BITS_MASK));
        if (page_idx != ZB_CHANNEL_LIST_PAGE_0_PACKED_IDX)
        {
          page_idx += ZB_PAGE_AND_CHANNEL_PACKED_OFFSET;
        }
        ret = zb_channel_page_get_page_by_idx(page_idx, &page);
      }
      else
      {
        ret = RET_ERROR;
      }
    }
  }
  else /* 2nd byte carrying channel */
  {
    ret = zb_channel_page_get_page_by_idx(g_sniffer_page, &page);
  }

  if (ret == RET_OK)
  {
    if ((channel_page_cmd & ZB_PAGE_AND_CHANNEL_PACKED_PAGE_BITS_MASK)
        == ZB_PAGE_AND_CHANNEL_PACKED_PAGE_BITS_MASK)
    {
      channel_page_cmd &= ZB_PAGE_AND_CHANNEL_2_BYTE_SIGNAL_REMOVE;
      ret = zb_channel_page_channel_number_to_logical(page, channel_page_cmd, &logical_channel);
    }
    else
    {
      ret = zb_channel_page_channel_number_to_logical(
          page, channel_page_cmd & ZB_PAGE_AND_CHANNEL_PACKED_CHANNEL_BITS_MASK, &logical_channel);
    }

    TRACE_MSG(TRACE_ERROR, "page %hd ch %hd", (FMT__H_H, page, logical_channel));

    zboss_sniffer_stop();
    zboss_sniffer_direct_pib_set(ZB_PHY_PIB_CURRENT_PAGE, (void *)&page, 1);
    zboss_sniffer_direct_pib_set(ZB_PHY_PIB_CURRENT_CHANNEL, (void *)&logical_channel, 1);
  }
  else if (ret == RET_BLOCKED)
  {
    TRACE_MSG(TRACE_ERROR, "Waiting for channel", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "incorrect page/channel value", (FMT__0));
  }

  return ret;
}

void zboss_sniffer_start(zb_callback_t data_ind_cb)
{
  zb_uint8_t value = 1;

  zboss_sniffer_direct_pib_set(ZB_PIB_ATTRIBUTE_PROMISCUOUS_MODE_CB, (void *)&data_ind_cb, (zb_uint8_t)sizeof(data_ind_cb));
  zboss_sniffer_direct_pib_set(ZB_PIB_ATTRIBUTE_PROMISCUOUS_RX_ON, (void *)&value, 1);
}
#endif  /* #ifdef ZB_PROMISCUOUS_MODE */
