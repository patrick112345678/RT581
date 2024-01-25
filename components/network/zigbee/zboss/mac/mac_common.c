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
/* PURPOSE: MAC functions to be put into common bank; also used in both our MAC
implementation and our stubs for GreenPeak
*/


#define ZB_TRACE_FILE_ID 297
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_nwk.h"
#include "zb_mac.h"
#include "mac_internal.h"
#include "zb_mac_transport.h"

#ifndef ZB_ALIEN_MAC
#include "zb_mac_globals.h"
#endif

/*! \addtogroup ZB_MAC */
/*! @{ */


/* Both MAC and NWK placed to the bank1, so all MAC calls are not banked */

/**`
   @brief Parses packed mhr header, fills mhr structure

   @param mhr - out pointer to mhr structure
   @param ptr - pointer to packed mhr header buffer
   @return packed mhr buffer length
*/
/*
  MA: remove trace - use in interrupt
*/
zb_uint8_t zb_parse_mhr_ptr(zb_mac_mhr_t *mhr, zb_uint8_t *ptr)
{
    zb_uint8_t *ptr_init = ptr;
    zb_uint8_t val;
    zb_uint16_t addr_short;

    //TRACE_MSG(TRACE_MAC2, ">>zb_parse_mhr mhr %p, ptr %p", (FMT__P_P, mhr, ptr));
    ZB_BZERO(mhr, sizeof(zb_mac_mhr_t));
    ZB_MEMCPY(mhr->frame_control, ptr, 2);
    ptr += sizeof(zb_uint16_t);
    mhr->seq_number = *ptr;
    ptr += sizeof(zb_uint8_t);


    /* mac spec 7.2.1.1.6 Destination Addressing Mode subfield */
    val = ZB_FCF_GET_DST_ADDRESSING_MODE(mhr->frame_control);
    //TRACE_MSG(TRACE_MAC2, "dst addr mode: %hd", (FMT__H, val));
    if (val != 0U)
    {
        zb_get_next_letoh16(&mhr->dst_pan_id, &ptr);

        /* dst addr mode: ZB_ADDR_NO_ADDR, ZB_ADDR_16BIT_DEV_OR_BROADCAST or ZB_ADDR_64BIT_DEV */
        if (val == ZB_ADDR_16BIT_DEV_OR_BROADCAST)
        {
            zb_get_next_letoh16(&addr_short, &ptr);
            mhr->dst_addr.addr_short = addr_short;
        }
        else
        {
            ZB_LETOH64(&mhr->dst_addr.addr_long, ptr);
            ptr += sizeof(zb_ieee_addr_t);
        }
    }
    else
    {
        mhr->dst_pan_id = ZB_BROADCAST_PAN_ID;
        mhr->dst_addr.addr_short = 0;
    }

    /* mac spec 7.2.1.1.8 Source Addressing Mode subfield */
    val = ZB_FCF_GET_SRC_ADDRESSING_MODE(mhr->frame_control);
    //TRACE_MSG(TRACE_MAC2, "src addr mode: %hd", (FMT__H, val));
    if (val != 0U)
    {
        if (ZB_FCF_GET_PANID_COMPRESSION_BIT(mhr->frame_control) == 0U)
        {
            zb_get_next_letoh16(&mhr->src_pan_id, &ptr);
        }
        else
        {
            mhr->src_pan_id = mhr->dst_pan_id;
        }

        /* dst addr mode: ZB_ADDR_NO_ADDR, ZB_ADDR_16BIT_DEV_OR_BROADCAST or ZB_ADDR_64BIT_DEV */
        if (val == ZB_ADDR_16BIT_DEV_OR_BROADCAST)
        {
            zb_get_next_letoh16(&addr_short, &ptr);
            mhr->src_addr.addr_short = addr_short;
        }
        else
        {
            ZB_LETOH64(&mhr->src_addr.addr_long, ptr);
            ptr += sizeof(zb_ieee_addr_t);
        }
    }

#ifdef ZB_MAC_SECURITY
    val = ZB_FCF_GET_SECURITY_BIT(mhr->frame_control);
    //TRACE_MSG(TRACE_MAC2, "security: %hd", (FMT__H, val));
    if (val)
    {
        /*
          Aux security header is here.
          Its format (see 7.6.2 Auxiliary security header):
          1 - security control (bits 0-2 Security Level bits 3-4 Key Identifier  Mode)
          4 - Frame Counter
          0/1/5/9 Key Identifier
        */

        /* security control */
        mhr->security_level = (*ptr) & 7;
        mhr->key_id_mode = ((*ptr) >> 3) & 7;
        ptr++;
        /* Frame Counter */
        ZB_LETOH32((zb_uint8_t *)&mhr->frame_counter, ptr);
        ptr += 4;
        /* Key Identifier */
        switch (mhr->key_id_mode)
        {
        case 2:
            ZB_MEMCPY(mhr->key_source, ptr, 4);
            ptr += 4;
            break;
        case 3:
            ZB_MEMCPY(mhr->key_source, ptr, 8);
            ptr += 8;
            break;
        }
        if (mhr->key_id_mode)
        {
            mhr->key_index = *ptr;
            ptr++;
        }
    }
#endif

    val = ptr - ptr_init;
    mhr->mhr_len = val;
    //TRACE_MSG(TRACE_MAC2, "<< zb_parse_mhr, ret val %i", (FMT__D, (int)val));
    return val;
}


zb_uint8_t zb_parse_mhr(zb_mac_mhr_t *mhr, zb_bufid_t buf)
{
    return zb_parse_mhr_ptr(mhr, zb_buf_begin(buf));
}

/*
  MA: remove trace - use in interrupt
*/
/**
   Calculates length of mac header (MHR) inside MAC frame

   param src_addr_mode   - source addressing mode one of @ref address_modes
   param dst_addr_mode   - destination addressing mode one of @ref address_modes

   Note that this function does not handle information elements
*/
zb_uint8_t zb_mac_calculate_mhr_length(zb_uint8_t src_addr_mode, zb_uint8_t dst_addr_mode, zb_bool_t equal_panid)
{
    zb_uint8_t len = ZB_MAC_DST_PANID_OFFSET;
    /* Compess PANId if and only if both addresses present */
    zb_bool_t panid_compression
        = equal_panid && (src_addr_mode != ZB_ADDR_NO_ADDR) && (dst_addr_mode != ZB_ADDR_NO_ADDR);

    TRACE_MSG(
        TRACE_MAC3,
        "> zb_mac_calculate_mhr_length src_addr_mode %hd dst_addr_mode %hd panid_compression %hd",
        (FMT__H_H_H, src_addr_mode, dst_addr_mode, panid_compression));

    if ( ( src_addr_mode ) != ZB_ADDR_NO_ADDR )
    {
        ZB_ASSERT( ( src_addr_mode ) == ZB_ADDR_16BIT_DEV_OR_BROADCAST ||
                   ( src_addr_mode ) == ZB_ADDR_64BIT_DEV);
        /* Source PAN ID Field */
        if (!panid_compression)
        {
            len += PAN_ID_LENGTH;
        }
        /* Source Address Field */
        len += 2U;
        if (src_addr_mode == ZB_ADDR_64BIT_DEV)
        {
            len += 6U;
        }
    }

    if ( ( dst_addr_mode ) != ZB_ADDR_NO_ADDR )
    {
        ZB_ASSERT( ( dst_addr_mode ) == ZB_ADDR_16BIT_DEV_OR_BROADCAST ||
                   ( dst_addr_mode ) == ZB_ADDR_64BIT_DEV);

        /* Destination Address Field */
        len += (PAN_ID_LENGTH + 2U);
        if (dst_addr_mode == ZB_ADDR_64BIT_DEV)
        {
            len += 6U;
        }
    }

    TRACE_MSG(TRACE_MAC3, "< zb_mac_calculate_mhr_length %hd", (FMT__H, len));
    return len;
}/* zb_uint8_t zb_mac_calculate_mhr_length(zb_uint8_t ... */
/*
  Fill packed mac header

  param ptr - pointer to out data
  param mhr - structure with mhr data
*/
void zb_mac_fill_mhr(zb_uint8_t *ptr, zb_mac_mhr_t *mhr)
{
    zb_uint8_t val;

    TRACE_MSG( TRACE_MAC3, ">>mac_fill_mhr", (FMT__0));
    ZB_ASSERT(ptr != NULL && mhr != NULL);

    /* mac spec 7.2.1 General MAC frame format */
    /* MHR structure
       | Frame     | Seq       | dst PAN | dst addr| src PAN | src addr| aux security      |
       | Control 2b| Number 1b | id 0/2b | 0/2/8b  | id 0/2b | 0/2/8b  | header 0/5/6/10/14|*/

    ZB_MEMCPY(ptr, &mhr->frame_control, 2);
    ptr += 2;
    *ptr = mhr->seq_number;
    ptr += sizeof(zb_uint8_t);

    /* mac spec 7.2.1.1.6 Destination Addressing Mode subfield */
    val = ZB_FCF_GET_DST_ADDRESSING_MODE(mhr->frame_control);
    if (val != ZB_ADDR_NO_ADDR)
    {

        ZB_PUT_NEXT_HTOLE16(ptr, mhr->dst_pan_id);

        /* dst addr mode: ZB_ADDR_NO_ADDR, ZB_ADDR_16BIT_DEV_OR_BROADCAST or ZB_ADDR_64BIT_DEV */
        if (val == ZB_ADDR_16BIT_DEV_OR_BROADCAST)
        {
            ZB_PUT_NEXT_HTOLE16(ptr, mhr->dst_addr.addr_short);
        }
        else
        {
            ZB_HTOLE64(ptr, &mhr->dst_addr.addr_long);
            ptr += sizeof(zb_ieee_addr_t);
        }
    }

    /* mac spec 7.2.1.1.8 Source Addressing Mode subfield */
    val = ZB_FCF_GET_SRC_ADDRESSING_MODE(mhr->frame_control);
    if (val != ZB_ADDR_NO_ADDR)
    {
        if (ZB_FCF_GET_PANID_COMPRESSION_BIT(mhr->frame_control) == 0U)
        {
            ZB_PUT_NEXT_HTOLE16(ptr, mhr->src_pan_id);
        }

        /* dst addr mode: ZB_ADDR_NO_ADDR, ZB_ADDR_16BIT_DEV_OR_BROADCAST or ZB_ADDR_64BIT_DEV */
        if (val == ZB_ADDR_16BIT_DEV_OR_BROADCAST)
        {
            ZB_PUT_NEXT_HTOLE16(ptr, mhr->src_addr.addr_short);
        }
        else
        {
            ZB_HTOLE64(ptr, &mhr->src_addr.addr_long);
        }
    }
    TRACE_MSG( TRACE_MAC3, "<<mac_fill_mhr", (FMT__0));
}


/**
   Gets source addressing mode subfield in frame control field ( FCF )
   Return values is one of @ref address_modes.

   @param p_fcf - pointer to 16bit FCF field.
*/

void zb_fcf_set_src_addressing_mode(zb_uint8_t *p_fcf, zb_uint8_t addr_mode)
{
    ZB_ASSERT((addr_mode) == 0U || (addr_mode) == 1U || (addr_mode) == 2U || (addr_mode) == 3U);

    (((zb_uint8_t *)(p_fcf))[ZB_PKT_16B_FIRST_BYTE]) &= 0x3FU;
    (((zb_uint8_t *)(p_fcf))[ZB_PKT_16B_FIRST_BYTE]) |= (addr_mode) << 6;
}

/**
   Sets dst addressing subfield in frame control field ( FCF )

   @param p_fcf     - pointer to 16bit FCF field.
   @param addr_mode - 0 or 1.
*/
void zb_fcf_set_dst_addressing_mode(zb_uint8_t *p_fcf, zb_uint8_t addr_mode)
{
    ZB_ASSERT((addr_mode) == 0U || (addr_mode) == 1U || (addr_mode) == 2U || (addr_mode) == 3U);

    ((((zb_uint8_t *)(p_fcf))[ZB_PKT_16B_FIRST_BYTE])) &= 0xF3U;
    ((((zb_uint8_t *)(p_fcf))[ZB_PKT_16B_FIRST_BYTE])) |= (addr_mode) << 2;
}

/*! @} */
