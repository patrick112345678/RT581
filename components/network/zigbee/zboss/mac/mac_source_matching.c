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
/* PURPOSE: Roitines specific mac data transfer for coordinator/router
*/
#define ZB_TRACE_FILE_ID 27123

#include "zb_common.h"
#include "zb_mac_globals.h"
#include "mac_source_matching.h"

#if defined ZB_MAC_SOFTWARE_PB_MATCHING && defined ZB_ROUTER_ROLE

#define FNV1_32_INIT ((zb_uint32_t)0x811c9dc5)

static inline zb_uint32_t
zb_fnv_32a_uint16(zb_uint16_t v)
{
    zb_uint32_t hval = FNV1_32_INIT;

    /* xor the bottom with the current octet */
    hval ^= (zb_uint32_t)(v & 0xff);
    /* multiply by the 32 bit FNV magic prime mod 2^32 */
    hval += (hval << 1) + (hval << 4) + (hval << 7) + (hval << 8) + (hval << 24);
    v >>= 8;
    hval ^= (zb_uint32_t)(v);
    /* multiply by the 32 bit FNV magic prime mod 2^32 */
    hval += (hval << 1) + (hval << 4) + (hval << 7) + (hval << 8) + (hval << 24);
    return hval;
}

static zb_uint32_t short_addr_hash(zb_uint16_t addr)
{
#ifdef USE_RANDOMS_TABLE
    /* random values */
    static zb_uint8_t randoms[256] =
    {
        0x25, 0xbf, 0x45, 0x3c, 0x18, 0x67, 0xb8, 0xaf, 0xb2, 0x26, 0x73, 0x57, 0x69, 0x0a, 0x10, 0x3e,
        0x25, 0xee, 0x49, 0xfb, 0x1f, 0x42, 0x97, 0x2e, 0x54, 0x3e, 0x70, 0x6d, 0x9e, 0x5a, 0x7a, 0x6b,
        0x77, 0x1e, 0x42, 0x0d, 0x68, 0x5e, 0x00, 0x64, 0x1a, 0x35, 0x26, 0x80, 0x9d, 0x85, 0xf8, 0x39,
        0x5d, 0x98, 0xcb, 0x6e, 0x6a, 0xbe, 0xbe, 0x15, 0x06, 0x76, 0x71, 0xaa, 0x3d, 0x48, 0x75, 0x0e,
        0x09, 0x39, 0xb5, 0x20, 0xcb, 0x1d, 0x9d, 0x31, 0xab, 0x1a, 0x5b, 0x88, 0x61, 0xd5, 0xcd, 0x65,
        0x09, 0xfc, 0x82, 0xc6, 0xbf, 0xfa, 0x1f, 0xe1, 0xfa, 0xe7, 0x6a, 0xb9, 0x74, 0x80, 0xde, 0x4d,
        0x09, 0xc9, 0x86, 0x80, 0x69, 0x6c, 0xbd, 0xdf, 0x8d, 0x83, 0x01, 0x15, 0x1a, 0x29, 0x54, 0x09,
        0xdb, 0x4b, 0xc4, 0x95, 0x67, 0x97, 0xab, 0x5e, 0xf5, 0x4a, 0xee, 0xf8, 0xf3, 0x49, 0xd8, 0xe7,
        0xd9, 0x38, 0x4c, 0xd0, 0x86, 0xfc, 0x02, 0xf4, 0xb0, 0xe2, 0x7f, 0xcf, 0xc3, 0xde, 0xa3, 0xbe,
        0xcd, 0x9e, 0xb2, 0x10, 0x5f, 0x83, 0x02, 0x51, 0x56, 0x36, 0xba, 0x1a, 0x71, 0x89, 0x43, 0x83,
        0x52, 0x65, 0x87, 0x50, 0xae, 0x0f, 0x40, 0x7e, 0x77, 0x65, 0x0e, 0xb0, 0x85, 0xe8, 0x69, 0xb1,
        0x23, 0x51, 0x57, 0xb8, 0x26, 0x57, 0xc7, 0xa4, 0xb5, 0x7b, 0x1b, 0x9a, 0xf4, 0xeb, 0xdb, 0x5d,
        0x2d, 0x05, 0x63, 0x86, 0x97, 0x71, 0x0e, 0xc5, 0xfd, 0x9e, 0x26, 0x2c, 0x76, 0x31, 0x03, 0x27,
        0x55, 0x88, 0x17, 0x8f, 0x33, 0x8e, 0xfc, 0x88, 0xef, 0xf2, 0xe0, 0x1e, 0x28, 0x8e, 0x19, 0xf5,
        0x6d, 0xe7, 0x43, 0x5b, 0x2c, 0x59, 0x08, 0x56, 0x0e, 0x2d, 0x98, 0x74, 0xf3, 0x5f, 0x8f, 0xc3,
        0xf8, 0x68, 0x2d, 0x03, 0x1c, 0x55, 0x99, 0xb2, 0x50, 0x36, 0x5c, 0x1a, 0x09, 0xf8, 0xc8, 0x34
    };
    zb_uint32_t h = randoms[addr & 0xff];
    addr >>= 8;
    h = randoms[(addr ^ h) & 0xff];
    return h % ZB_CHILD_HASH_TABLE_SIZE;
#else
    return zb_fnv_32a_uint16(addr) % ZB_CHILD_HASH_TABLE_SIZE;
#endif
}

zb_bool_t mac_child_hash_table_search(zb_uint16_t short_addr, zb_uint32_t *ref_p,
                                      zb_bool_t called_from_interrupt,
                                      source_matching_search_action_t action)
{
    zb_uint32_t starthashindex = short_addr_hash(short_addr);
    zb_uint32_t ref = starthashindex;
    zb_bool_t   ret = ZB_FALSE;

    TRACE_MSG(TRACE_MAC3, ">> mac_child_hash_table_search 0x%x, called_from_interrupt %hd, action %hd",
              (FMT__D_H_H, MAC_CTX().child_hash_table[ref], called_from_interrupt, action));

    if (!called_from_interrupt)
    {
        ZB_RADIO_INT_DISABLE();
    }

    do
    {
        while (MAC_CTX().child_hash_table[ref])
        {
            if (MAC_CTX().child_hash_table[ref] == short_addr)
            {
                *ref_p = ref;
                TRACE_MSG(TRACE_MAC3, "addr founded at %hd", (FMT__H, ref));

                if (action == SEARCH_ACTION_DELETE_IF_EXIST)
                {
                    MAC_CTX().child_hash_table[ref] = 0x0000;
                }
                ret = ZB_TRUE;
                break;
            }
            ref = (ref + 1) % ZB_CHILD_HASH_TABLE_SIZE;
            if (ref == starthashindex)
            {
                break;
            }
        }

        if (ret == ZB_FALSE)
        {
            TRACE_MSG(TRACE_MAC3, "addr 0x%x not founded", (FMT__D, short_addr));

            if (action == SEARCH_ACTION_ALLOW_CREATE &&
                    MAC_CTX().child_hash_table[ref] == 0x0000)
            {
                *ref_p = ref;
                MAC_CTX().child_hash_table[ref] = short_addr;
                TRACE_MSG(TRACE_MAC3, "addr created at %hd", (FMT__H, ref));
                ret = ZB_TRUE;
                break;
            }
        }
    } while (0);

    if (!called_from_interrupt)
    {
        ZB_RADIO_INT_ENABLE();
    }

    return ret;
}

zb_uint8_t mac_software_src_match_add_short_addr(zb_uint16_t short_addr)
{
    zb_uint32_t ref;

    TRACE_MSG(TRACE_MAC3, ">> mac_software_src_match_add_short_addr 0x%x", (FMT__D, short_addr));

    return  (zb_uint8_t) (mac_child_hash_table_search(short_addr, &ref, ZB_FALSE, SEARCH_ACTION_ALLOW_CREATE) ?
                          (zb_uint8_t) MAC_SUCCESS : (zb_uint8_t) MAC_INVALID_PARAMETER);
}

#ifdef ZB_MAC_POLL_INDICATION_CALLS_REDUCED
void mac_software_src_match_update_poll_ind_call_timeout(zb_uint16_t short_addr, zb_uint16_t poll_timeout)
{
    zb_uint32_t ref;

    TRACE_MSG(TRACE_MAC3, ">> mac_software_src_match_update_poll_ind_call_timeout: addr 0x%x, timeout %hd",
              (FMT__D_H, short_addr, poll_timeout));

    if (mac_child_hash_table_search(short_addr, &ref, ZB_FALSE, SEARCH_ACTION_NO_ACTION))
    {
        MAC_POLL_TIMEOUT_SET(ref, poll_timeout);
    }
}
#endif /* ZB_MAC_POLL_INDICATION_CALLS_REDUCED */

zb_uint8_t mac_software_src_match_delete_short_addr(zb_uint16_t short_addr)
{
    zb_uint32_t ref;
    zb_bool_t   search_res;

    TRACE_MSG(TRACE_MAC3, ">> mac_software_src_match_delete_short_addr 0x%x", (FMT__D, short_addr));

    search_res = mac_child_hash_table_search(short_addr, &ref, ZB_FALSE,
                 SEARCH_ACTION_DELETE_IF_EXIST);

    if (search_res)
    {
        MAC_PENDING_BITMAP_CLR(ref);
    }
    return (zb_uint8_t) (search_res ? (zb_uint8_t) MAC_SUCCESS : (zb_uint8_t) MAC_INVALID_PARAMETER);
}

void mac_software_src_match_tbl_drop(void)
{
    TRACE_MSG(TRACE_MAC3, ">> mac_software_src_match_tbl_drop", (FMT__0));
    ZB_RADIO_INT_DISABLE();
    ZB_BZERO(MAC_CTX().child_hash_table, ZB_CHILD_HASH_TABLE_SIZE * sizeof(zb_uint16_t));
    ZB_BZERO(MAC_CTX().pending_bitmap, ZB_PENDING_BITMAP_SIZE * sizeof(zb_uint32_t));
    ZB_RADIO_INT_ENABLE();
};

void mac_software_src_match_short_set_pending_bit(zb_uint16_t short_addr, zb_uint8_t pending)
{
    zb_uint32_t ref;
    TRACE_MSG(TRACE_MAC3, ">> mac_software_src_match_short_set_pending_bit 0x%x, val = %hd", (FMT__D_H, short_addr, pending));

    /* Let's make sure that child is placed into hash table while setting pending bit for this address
       Rationale: maybe, when we have PURGE, can be called from its logic to clear pending bit.
    */
    if (mac_child_hash_table_search(short_addr, &ref, ZB_FALSE, pending ?
                                    SEARCH_ACTION_ALLOW_CREATE : SEARCH_ACTION_NO_ACTION))
    {
        if (pending)
        {
            MAC_PENDING_BITMAP_SET(ref);
        }
        else
        {
            MAC_PENDING_BITMAP_CLR(ref);
        }
    }
}

#endif  /* ZB_MAC_SOFTWARE_PB_MATCHING */
