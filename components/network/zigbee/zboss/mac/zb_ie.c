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
/*  PURPOSE: implementation of IE-parsing related functions
*/

#define ZB_TRACE_FILE_ID 54
#include "zb_common.h"
#include "zb_ie.h"

#if defined ZB_ENHANCED_BEACON_SUPPORT

/*
 * Skips HIEs until no one left or HT* HIE is found
 * See also ZB_SKIP_HIE_TILL_HT
 */
zb_bool_t zb_skip_hie_till_ht(zb_uint8_t *ptr, zb_uint32_t max_len,
                              zb_uint32_t *skipped_bytes)
{
  zb_hie_header_t current_hdr;
  zb_uint32_t space_left;
  zb_bool_t ret = ZB_TRUE;

  TRACE_MSG(TRACE_MAC3, ">>zb_skip_hie_till_ht (%x, %d)", (FMT__D_D, ptr, max_len));

  *skipped_bytes = 0;
  while (ret)
  {
    space_left = (max_len) - *(skipped_bytes);
    if (space_left < ZB_HIE_HEADER_LENGTH)
    {
      /* malformed packet */
      TRACE_MSG(TRACE_MAC3, "malformed packet", (FMT__0));
      ret = ZB_FALSE;
      continue;
    }

    if (ZB_IE_HEADER_GET_TYPE(ptr) != ZB_IE_HEADER_TYPE_HIE)
    {
      /* malformed packet */
      TRACE_MSG(TRACE_MAC3, "unexpected PIE", (FMT__0));
      ret = ZB_FALSE;
      continue;
    }

    ZB_GET_HIE_HEADER(ptr + *skipped_bytes, &current_hdr);

    if (current_hdr.element_id == ZB_HIE_ELEMENT_HT1
        || current_hdr.element_id == ZB_HIE_ELEMENT_HT2)
    {
      /* HT found */
      break;
    }
    else if ((zb_uint_t)ZB_HIE_HEADER_LENGTH + current_hdr.length > space_left)
    {
      /* preventing possible buffer overflow */
      TRACE_MSG(TRACE_MAC3, "IE length overflows the buffer", (FMT__0));
      ret = ZB_FALSE;
      continue;
    }
    else
    {
      /* MISRA rule 15.7 requires empty 'else' branch. */
    }

    *skipped_bytes += current_hdr.length + (zb_uint32_t)ZB_HIE_HEADER_LENGTH;
  }

  TRACE_MSG(TRACE_MAC3, "<<zb_skip_hie_till_ht (ret %d)", (FMT__H, ret));
  return ret;
}


/*
 * Skips all IEs present, starting at ptr
 * Intended usage:
 * fetch command id field in MAC command frames before
 * detailed processing of IEs
 *
 * See also ZB_SKIP_ALL_IE
 */
zb_bool_t zb_skip_all_ie(zb_uint8_t *ptr,  zb_uint32_t max_len,
                              zb_uint32_t *skipped_bytes)
{
  zb_hie_header_t current_hie_hdr;
  zb_pie_header_t current_pie_hdr;
  zb_uint32_t space_left, len_to_skip;
  zb_bool_t ret = ZB_TRUE;

  TRACE_MSG(TRACE_MAC3, ">>zb_skip_all_ie (%x, %d)", (FMT__D_D, ptr, max_len));

  *skipped_bytes = 0;
  while (ret)
  {
    space_left = (max_len) - *(skipped_bytes);
    if (space_left < ZB_HIE_HEADER_LENGTH)
    {
      /* parsing error */
      TRACE_MSG(TRACE_MAC3, "malformed packet", (FMT__0));
      ret = ZB_FALSE;
      continue;
    }

    if (ZB_IE_HEADER_GET_TYPE(ptr + *skipped_bytes) == ZB_IE_HEADER_TYPE_HIE)
    {
      /* HIE processing */
      ZB_GET_HIE_HEADER(ptr + *skipped_bytes, &current_hie_hdr);
      *skipped_bytes += ZB_HIE_HEADER_LENGTH;

      if (current_hie_hdr.element_id == ZB_HIE_ELEMENT_HT2)
      {
        /* no IE should follow, reached frame payload */
        break;
      }

      len_to_skip = current_hie_hdr.length;
    }
    else
    {
      /* PIE processing */
      ZB_GET_PIE_HEADER(ptr + *skipped_bytes, &current_pie_hdr);
      *skipped_bytes += ZB_PIE_HEADER_LENGTH;

      if (current_pie_hdr.group_id == ZB_PIE_GROUP_TERMINATION)
      {
        break;
      }

      len_to_skip = current_pie_hdr.length;
    }

    if (len_to_skip > space_left)
    {
      /* catching possible buffer overflow */
      TRACE_MSG(TRACE_MAC3, "IE length overflows the buffer", (FMT__0));
      ret = ZB_FALSE;
      continue;
    }

    *skipped_bytes += len_to_skip;
  }

  TRACE_MSG(TRACE_MAC3, "<<zb_skip_all_ie (ret %d)", (FMT__H, ret));
  return ret;
}


/* ptr must point to the first OUI byte */
zb_bool_t zb_parse_zigbee_vendor_pie(zb_uint8_t *ptr, zb_uint32_t len,
                                     zb_zigbee_vendor_pie_parsed_t *parsed)
{
  zb_zigbee_pie_header_t hdr;
  zb_uint8_t *max_ptr = ptr + len; /* points outside given buffer */
  zb_bool_t ret = ZB_TRUE;

  TRACE_MSG(TRACE_MAC3, ">>zb_parse_zigbee_vendor_pie (%x, %d)", (FMT__D_D, ptr, len));

  if (len < ZB_PIE_VENDOR_OUI_LENGTH)
  {
    /* no vendor OUI, malformed */
    TRACE_MSG(TRACE_MAC3, "malformed packet", (FMT__0));
    ret = ZB_FALSE;
  }
  else if (!ZB_IE_CHECK_ZIGBEE_VENDOR(ptr))
  {
    /* not Zigbee, nothing to do */
    TRACE_MSG(TRACE_MAC3, "not Zigbee", (FMT__0));
    ret = ZB_TRUE;
  }
  else
  {
    ptr += ZB_PIE_VENDOR_OUI_LENGTH;
    while (max_ptr - ptr >= ZB_ZIGBEE_PIE_HEADER_LENGTH)
    {
      ZB_GET_NEXT_ZIGBEE_PIE_HEADER(ptr, &hdr);
      if (max_ptr - ptr < hdr.length)
      {
        /* buffer overflow */
        TRACE_MSG(TRACE_MAC3, "malformed packet", (FMT__0));
        ret = ZB_FALSE;
        break;
      }

      switch(hdr.sub_id)
      {
        case ZB_ZIGBEE_PIE_SUB_ID_EB_PAYLOAD:
          if (hdr.length < (15U + 2U + 2U))
          {
            /* malformed packet */
            TRACE_MSG(TRACE_MAC3, "malformed EB payload IE, skipping", (FMT__0));
            break;
          }

          /*
           * Payload:
           *   Beacon Payload (15 bytes),
           *   Superframe Specification (2 bytes),
           *   Source Short Address (2 bytes)
           */
          parsed->eb_payload_set = ZB_TRUE;
          parsed->eb_payload.beacon_payload_len = 15;
          parsed->eb_payload.beacon_payload_ptr = ptr;
          ZB_LETOH16(&parsed->eb_payload.superframe_spec, ptr + 15);
          ZB_LETOH16(&parsed->eb_payload.source_short_addr, ptr + 17);
          break;

        case ZB_ZIGBEE_PIE_SUB_ID_TX_POWER:
          parsed->tx_power_set = ZB_TRUE;
          parsed->tx_power.tx_power_value = (zb_int8_t)(*ptr);
          break;

        case ZB_ZIGBEE_PIE_SUB_ID_REJOIN:
          if (hdr.length < (8U + 2U))
          {
            /* malformed */
            TRACE_MSG(TRACE_MAC3, "malformed rejoin IE, skipping", (FMT__0));
            break;
          }

          parsed->rejoin_desc_set = ZB_TRUE;
          ZB_LETOH64(&parsed->rejoin_desc.extended_pan_id, ptr);
          ZB_LETOH16(&parsed->rejoin_desc.src_short_addr, ptr + 8);
          break;

        default:
          TRACE_MSG(TRACE_MAC3, "unexpected sub_id: %x, skipping", (FMT__D, hdr.sub_id));
          break;
      }

      ptr += hdr.length;
    }

    if (ret && ptr != max_ptr)
    {
      TRACE_MSG(TRACE_MAC3, "parsing stopped before reaching the end of block", (FMT__0));
      ret = ZB_FALSE;
    }
  }

  TRACE_MSG(TRACE_MAC3, "<<zb_parse_zigbee_vendor_pie (ret %d)", (FMT__H, ret));
  return ret;
}


zb_bool_t zb_parse_mlme_pie(zb_uint8_t *ptr, zb_uint32_t len,
                            zb_mlme_pie_parsed_t *parsed)
{
  zb_nie_header_t nie_hdr;
  zb_uint8_t *max_ptr = ptr + len; /* pointer outside given buffer */
  zb_bool_t ret = ZB_TRUE;

  TRACE_MSG(TRACE_MAC3, ">>zb_parse_mlme_pie (%x, %d)", (FMT__D_D, ptr, len));

  while (max_ptr - ptr >= ZB_NIE_HEADER_LENGTH)
  {
    ZB_GET_NEXT_NIE_HEADER(ptr, &nie_hdr);
    if (max_ptr - ptr < nie_hdr.length)
    {
      /* buffer overflow */
      TRACE_MSG(TRACE_MAC3, "malformed packet", (FMT__0));
      ret = ZB_FALSE;
      break;
    }

    switch (nie_hdr.sub_id)
    {
      case ZB_NIE_SUB_ID_EB_FILTER:
        if (nie_hdr.length >= 1U){
          parsed->eb_filter_set = ZB_TRUE;
          parsed->eb_filter.mask = *ptr;
        }
        break;

      default:
        TRACE_MSG(TRACE_MAC3, "unexpected sub_id: %x, skipping", (FMT__D, nie_hdr.sub_id));
        break;
    }

    ptr += nie_hdr.length;
  }

  if (ret && ptr != max_ptr)
  {
    TRACE_MSG(TRACE_MAC3, "parsing stopped before reaching the end of block", (FMT__0));
    ret = ZB_FALSE;
  }

  TRACE_MSG(TRACE_MAC3, "<<zb_parse_mlme_pie (ret %d)", (FMT__H, ret));
  return ret;
}

#endif /* ZB_ENHANCED_BEACON_SUPPORT */
