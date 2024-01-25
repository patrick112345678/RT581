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
/* PURPOSE: debug stuff
*/

#define ZB_TRACE_FILE_ID 2144
#include "zboss_api_core.h"


/*! \addtogroup ZB_DEBUG */
/*! @{ */

#if (defined USE_ASSERT || defined DOXYGEN)

#include "zb_net_trace.h"

#if !defined ZB_TOOL && !defined ZB_MACSPLIT_DEVICE
#include "zb_common.h"
#endif

/**
   Abort execution in system-specific manner.

   Write trace message before abort.
 */
/** [abort] */
void zb_abort(char *caller_file, int caller_line)
{
    ZVUNUSED(caller_file);
    ZVUNUSED(caller_line);
#if !defined UNIX && !defined ZB_PLATFORM_LINUX
    TRACE_MSG (TRACE_ERROR, "Abort from %d ", (FMT__D, (int)caller_line));
#else
    TRACE_MSG (TRACE_ERROR, "Abort from %s:%d ", (FMT__P_D, caller_file, (int)caller_line));
#endif
    //ZB_ABORT();
    while (1) {}
}

#ifndef ZB_BINARY_TRACE
void zb_assert(const zb_char_t *file_name, zb_int_t line_number)
{
    ZVUNUSED(file_name);
    ZVUNUSED(line_number);
#if !defined UNIX && !defined ZB_PLATFORM_LINUX && !defined ZB_WINDOWS
    //TRACE_MSG (TRACE_ERROR, "Assertion failed %d", (FMT__D, line_number));
    TRACE_MSG (TRACE_ERROR, "Assertion failed %s:%d", (FMT__P_D, file_name, line_number));
#else
    TRACE_MSG (TRACE_ERROR, "Assertion failed %s:%d", (FMT__P_D, file_name, line_number));
#endif
    //ZB_ABORT();
    while (1) {}
}
/** [abort] */

#else /* !ZB_BINARY_TRACE */

void zb_assert(zb_uint16_t file_id, zb_int_t line_number)
{
    ZVUNUSED(file_id);
    ZVUNUSED(line_number);
    TRACE_MSG(TRACE_ERROR, "Assertion failed ZB_TRACE_FILE_ID %d line %d", (FMT__D_D, file_id, line_number));
#ifdef ZB_TRACE_CPU_STATE
    ZB_TRACE_CPU_STATE();
#endif /* ZB_TRACE_CPU_STATE */
#if !defined ZB_TOOL && !defined ZB_MACSPLIT_DEVICE
    if (ZDO_CTX().assert_indication_cb)
    {
        (ZDO_CTX().assert_indication_cb)(file_id, line_number);
    }
#endif

#if defined ZB_RESET_AT_ASSERT
    zb_reset(0);
#else
    ZB_ABORT();
#endif /* ZB_RESET_AT_ASSERT */
}
#endif /* !ZB_BINARY_TRACE */

void lwip_zb_assert(zb_uint16_t file_id, zb_int_t line_number)
{
    ZVUNUSED(file_id);
    ZVUNUSED(line_number);
    TRACE_MSG(TRACE_ERROR, "LWIP assert failed file_id %d line %d", (FMT__D_D, file_id, line_number));
}
#endif  /* DEBUG */


#if defined DEBUG
#define HEX_ARG(n) buf[i+n]

void dump_traf(zb_uint8_t *buf, zb_ushort_t len)
{
    zb_ushort_t i;

    ZVUNUSED(buf);
    ZVUNUSED(len);

    TRACE_MSG(TRACE_MAC3, "len %hd", (FMT__H, len));

    for (i = 0 ; i < len ; i += 8U)
    {
        zb_ushort_t tmp = len - i;
        if (tmp >= 8U)
        {
            TRACE_MSG(TRACE_MAC3, "%02hx %02hx %02hx %02hx %02hx %02hx %02hx %02hx",
                      (FMT__H_H_H_H_H_H_H_H,
                       HEX_ARG(0), HEX_ARG(1), HEX_ARG(2), HEX_ARG(3),
                       HEX_ARG(4), HEX_ARG(5), HEX_ARG(6), HEX_ARG(7)));
        }
        else
        {
            switch (tmp)
            {
            case 7:
                TRACE_MSG(TRACE_MAC3, "%02hx %02hx %02hx %02hx %02hx %02hx %02hx",
                          (FMT__H_H_H_H_H_H_H,
                           HEX_ARG(0), HEX_ARG(1), HEX_ARG(2), HEX_ARG(3),
                           HEX_ARG(4), HEX_ARG(5), HEX_ARG(6)));
                break;
            case 6:
                TRACE_MSG(TRACE_MAC3, "%02hx %02hx %02hx %02hx %02hx %02hx",
                          (FMT__H_H_H_H_H_H,
                           HEX_ARG(0), HEX_ARG(1), HEX_ARG(2), HEX_ARG(3),
                           HEX_ARG(4), HEX_ARG(5)));
                break;
            case 5:
                TRACE_MSG(TRACE_MAC3, "%02hx %02hx %02hx %02hx %02hx",
                          (FMT__H_H_H_H_H,
                           HEX_ARG(0), HEX_ARG(1), HEX_ARG(2), HEX_ARG(3),
                           HEX_ARG(4)));
                break;
            case 4:
                TRACE_MSG(TRACE_MAC3, "%02hx %02hx %02hx %02hx",
                          (FMT__H_H_H_H,
                           HEX_ARG(0), HEX_ARG(1), HEX_ARG(2), HEX_ARG(3)));
                break;
            case 3:
                TRACE_MSG(TRACE_MAC3, "%02hx %02hx %02hx",
                          (FMT__H_H_H,
                           HEX_ARG(0), HEX_ARG(1), HEX_ARG(2)));
                break;
            case 2:
                TRACE_MSG(TRACE_MAC3, "%02hx %02hx",
                          (FMT__H_H,
                           HEX_ARG(0), HEX_ARG(1)));
                break;
            case 1:
                TRACE_MSG(TRACE_MAC3, "%02hx",
                          (FMT__H,
                           HEX_ARG(0)));
                break;
            default:
                break;
            }
            /* Exit for loop */
            break;
        }
    }
}

void dump_usb_traf(zb_uint8_t *buf, zb_ushort_t len)
{
    zb_ushort_t i;
    ZVUNUSED(buf);

    TRACE_MSG(TRACE_USB1, "len %hd", (FMT__H, len));

    for (i = 0 ; i < len ; i += 8U)
    {
        zb_ushort_t tmp = len - i;
        if (tmp >= 8U)
        {
            TRACE_MSG(TRACE_USB1, "%02hx %02hx %02hx %02hx %02hx %02hx %02hx %02hx",
                      (FMT__H_H_H_H_H_H_H_H,
                       HEX_ARG(0), HEX_ARG(1), HEX_ARG(2), HEX_ARG(3),
                       HEX_ARG(4), HEX_ARG(5), HEX_ARG(6), HEX_ARG(7)));
        }
        else
        {
            switch (tmp)
            {
            case 7:
                TRACE_MSG(TRACE_USB1, "%02hx %02hx %02hx %02hx %02hx %02hx %02hx",
                          (FMT__H_H_H_H_H_H_H,
                           HEX_ARG(0), HEX_ARG(1), HEX_ARG(2), HEX_ARG(3),
                           HEX_ARG(4), HEX_ARG(5), HEX_ARG(6)));
                break;
            case 6:
                TRACE_MSG(TRACE_USB1, "%02hx %02hx %02hx %02hx %02hx %02hx",
                          (FMT__H_H_H_H_H_H,
                           HEX_ARG(0), HEX_ARG(1), HEX_ARG(2), HEX_ARG(3),
                           HEX_ARG(4), HEX_ARG(5)));
                break;
            case 5:
                TRACE_MSG(TRACE_USB1, "%02hx %02hx %02hx %02hx %02hx",
                          (FMT__H_H_H_H_H,
                           HEX_ARG(0), HEX_ARG(1), HEX_ARG(2), HEX_ARG(3),
                           HEX_ARG(4)));
                break;
            case 4:
                TRACE_MSG(TRACE_USB1, "%02hx %02hx %02hx %02hx",
                          (FMT__H_H_H_H,
                           HEX_ARG(0), HEX_ARG(1), HEX_ARG(2), HEX_ARG(3)));
                break;
            case 3:
                TRACE_MSG(TRACE_USB1, "%02hx %02hx %02hx",
                          (FMT__H_H_H,
                           HEX_ARG(0), HEX_ARG(1), HEX_ARG(2)));
                break;
            case 2:
                TRACE_MSG(TRACE_USB1, "%02hx %02hx",
                          (FMT__H_H,
                           HEX_ARG(0), HEX_ARG(1)));
                break;
            case 1:
                TRACE_MSG(TRACE_USB1, "%02hx",
                          (FMT__H,
                           HEX_ARG(0)));
                break;
            default:
                break;
            }
            /* Exit for loop */
            break;
        }
    }
}

void dump_hex_data(zb_uint_t trace_mask, zb_uint8_t trace_level,
                   zb_uint8_t *buf, zb_ushort_t len)
{
    zb_ushort_t i;
    ZVUNUSED(buf);

#define TRACE_DUMP     trace_mask, trace_level

    TRACE_MSG(TRACE_DUMP, "len %hd", (FMT__H, len));

    for (i = 0 ; i < len ; i += 8U)
    {
        zb_ushort_t tmp = len - i;
        if (tmp >= 8U)
        {
            TRACE_MSG(TRACE_DUMP, "0x%hx 0x%hx 0x%hx 0x%hx 0x%hx 0x%hx 0x%hx 0x%hx",
                      (FMT__H_H_H_H_H_H_H_H,
                       HEX_ARG(0), HEX_ARG(1), HEX_ARG(2), HEX_ARG(3),
                       HEX_ARG(4), HEX_ARG(5), HEX_ARG(6), HEX_ARG(7)));
        }
        else
        {
            switch (tmp)
            {
            case 7:
                TRACE_MSG(TRACE_DUMP, "0x%hx 0x%hx 0x%hx 0x%hx 0x%hx 0x%hx 0x%hx",
                          (FMT__H_H_H_H_H_H_H,
                           HEX_ARG(0), HEX_ARG(1), HEX_ARG(2), HEX_ARG(3),
                           HEX_ARG(4), HEX_ARG(5), HEX_ARG(6)));
                break;
            case 6:
                TRACE_MSG(TRACE_DUMP, "0x%hx 0x%hx 0x%hx 0x%hx 0x%hx 0x%hx",
                          (FMT__H_H_H_H_H_H,
                           HEX_ARG(0), HEX_ARG(1), HEX_ARG(2), HEX_ARG(3),
                           HEX_ARG(4), HEX_ARG(5)));
                break;
            case 5:
                TRACE_MSG(TRACE_DUMP, "0x%hx 0x%hx 0x%hx 0x%hx 0x%hx",
                          (FMT__H_H_H_H_H,
                           HEX_ARG(0), HEX_ARG(1), HEX_ARG(2), HEX_ARG(3),
                           HEX_ARG(4)));
                break;
            case 4:
                TRACE_MSG(TRACE_DUMP, "0x%hx 0x%hx 0x%hx 0x%hx",
                          (FMT__H_H_H_H,
                           HEX_ARG(0), HEX_ARG(1), HEX_ARG(2), HEX_ARG(3)));
                break;
            case 3:
                TRACE_MSG(TRACE_DUMP, "0x%hx 0x%hx 0x%hx",
                          (FMT__H_H_H,
                           HEX_ARG(0), HEX_ARG(1), HEX_ARG(2)));
                break;
            case 2:
                TRACE_MSG(TRACE_DUMP, "0x%hx 0x%hx",
                          (FMT__H_H,
                           HEX_ARG(0), HEX_ARG(1)));
                break;
            case 1:
                TRACE_MSG(TRACE_DUMP, "0x%hx",
                          (FMT__H,
                           HEX_ARG(0)));
                break;
            default:
                break;
            }
            /* Exit for loop */
            break;
        }
    }
}

static void trace_arr(zb_uint8_t *ptr, zb_bool_t format)
{
    if (format)
    {
        TRACE_MSG(TRACE_ERROR, TRACE_FORMAT_128, (FMT__A_A, TRACE_ARG_128(ptr)));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(ptr)));
    }
}

void trace_hex_data_func(zb_uint8_t *ptr, zb_short_t size, zb_bool_t format)
{
    zb_uint8_t buf[16];
    zb_uint16_t i, mod, div;
    zb_uint16_t bytes_per_line;

    ZB_ASSERT(ptr != NULL);
    ZB_ASSERT(size > 0);

    bytes_per_line = format ? 16U : 8U;

    div = ((zb_uint16_t)size) / bytes_per_line;
    mod = ((zb_uint16_t)size) % bytes_per_line;

    for (i = 0; i < div; ++i)
    {
        ZB_MEMCPY(buf, ptr + i * bytes_per_line, bytes_per_line);
        trace_arr(buf, format);
    }

    if (ZB_U2B(mod))
    {
        ZB_BZERO(buf, sizeof(buf));
        ZB_MEMCPY(buf, ptr + div * bytes_per_line, mod);
        trace_arr(buf, format);
    }
}

#endif /* DEBUG */

/*! @} */
