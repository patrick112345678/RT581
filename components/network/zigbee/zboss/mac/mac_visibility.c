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
/* PURPOSE: SW MAC visibility limit (useful for ZCP tests)
*/

#define ZB_TRACE_FILE_ID 291
#include "zb_common.h"

#if !defined ZB_ALIEN_MAC && !defined ZB_MACSPLIT_HOST

/*! \addtogroup ZB_MAC */
/*! @{ */


#include "zb_scheduler.h"
#include "zb_nwk.h"
#include "zb_mac.h"
#include "mac_internal.h"
#include "zb_mac_transport.h"
#include "zb_mac_globals.h"


#ifdef ZB_LIMIT_VISIBILITY

/**
   Check whether frame is visible according to soft visibility limits

   @param mhr - parsed frame mhr

   @return ZB_TRUE is frame is visible, ZB_FALSE otherwhise
 */
zb_bool_t mac_is_frame_visible(const zb_mac_mhr_t *mhr)
{
    zb_bool_t ret = ZB_FALSE;
    zb_ushort_t i;

    /*
      Invisible short addresses used mainly to limit beacons visibility, when long
      address is not available yet.
    */

    if (ZB_FCF_GET_SRC_ADDRESSING_MODE(mhr->frame_control) == ZB_ADDR_16BIT_DEV_OR_BROADCAST)
    {
        for (i = 0 ; i < MAC_CTX().n_invisible_short_addr ; ++i)
        {
            if (mhr->src_addr.addr_short == MAC_CTX().invisible_short_addreesses[i])
            {
                return ZB_FALSE;
            }
        }
    }

    if (MAC_CTX().n_visible_addr == 0U
            || ZB_FCF_GET_FRAME_TYPE(mhr->frame_control) == MAC_FRAME_BEACON)
    {
        /* visibility control is off */
        return ZB_TRUE;
    }

    switch (ZB_FCF_GET_SRC_ADDRESSING_MODE(mhr->frame_control))
    {
    case ZB_ADDR_16BIT_DEV_OR_BROADCAST:
    {
        for (i = 0 ; i < MAC_CTX().n_visible_addr ; ++i)
        {
            zb_ieee_addr_t long_addr;

            if (zb_address_ieee_by_short(mhr->src_addr.addr_short, long_addr) == RET_OK
                    && ZB_IEEE_ADDR_CMP(long_addr, MAC_CTX().visible_addresses[i]))
            {
                TRACE_MSG(TRACE_MAC2, "visible dev " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(long_addr)));
                ret = ZB_TRUE;
                break;
            }
        }
        break;
    }
    case ZB_ADDR_64BIT_DEV:
    {
        ret = zb_mac_is_long_addr_visible(mhr->src_addr.addr_long);
        break;
    }
    default:
        ret = ZB_TRUE;
        break;
    }

    TRACE_MSG(TRACE_COMMON1, "mac_is_frame_visible: ret %hd, src_addr_mode %hd, addr " TRACE_FORMAT_64,
              (FMT__H_H_A, ret, ZB_FCF_GET_SRC_ADDRESSING_MODE(mhr->frame_control), TRACE_ARG_64(mhr->src_addr.addr_long)));

    return ret;
}


void mac_add_visible_device(zb_ieee_addr_t long_addr)
{
    ZB_IEEE_ADDR_COPY(MAC_CTX().visible_addresses[MAC_CTX().n_visible_addr], long_addr);

    TRACE_MSG(TRACE_MAC2, "added visible dev " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(long_addr)));
    MAC_CTX().n_visible_addr++;
}


void mac_add_invisible_short(zb_uint16_t addr)
{
    MAC_CTX().invisible_short_addreesses[MAC_CTX().n_invisible_short_addr] = addr;

    TRACE_MSG(TRACE_MAC2, "added invisible dev 0x%x", (FMT__D, addr));
    MAC_CTX().n_invisible_short_addr++;
}


void mac_remove_invisible_short(zb_uint16_t addr)
{
    zb_ushort_t i;

    for (i = 0 ; i < MAC_CTX().n_invisible_short_addr ; ++i)
    {
        if (MAC_CTX().invisible_short_addreesses[i] == addr)
        {
            ZB_MEMMOVE(&MAC_CTX().invisible_short_addreesses[i],
                       &MAC_CTX().invisible_short_addreesses[i + 1U],
                       MAC_CTX().n_invisible_short_addr - i - 1U);
            MAC_CTX().n_invisible_short_addr--;
            break;
        }
    }

    TRACE_MSG(TRACE_MAC2, "removed invisible dev 0x%x", (FMT__D, addr));
}

void mac_clear_filters(void)
{
    TRACE_MSG(TRACE_MAC2, "clear mac filters", (FMT__0));
    MAC_CTX().n_visible_addr = 0;
    MAC_CTX().n_invisible_short_addr = 0;
}

zb_bool_t zb_mac_is_long_addr_visible(const zb_ieee_addr_t ieee_addr)
{
    zb_ushort_t i;

    for (i = 0; i < MAC_CTX().n_visible_addr; ++i)
    {
        if (ZB_IEEE_ADDR_CMP(ieee_addr, MAC_CTX().visible_addresses[i]))
        {
            return ZB_TRUE;
        }
    }

    return ZB_FALSE;
}

#endif  /* ZB_LIMIT_VISIBILITY */


/*! @} */

#endif /* !ZB_ALIEN_MAC && !ZB_MACSPLIT_HOST */
