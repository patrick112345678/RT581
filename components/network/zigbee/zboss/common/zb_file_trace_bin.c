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
/* PURPOSE: binary trace and traffic dump for platform with filesystem and printf() routine
*/


#define ZB_TRACE_FILE_ID 2151
#include "zboss_api_core.h"
#include "zboss_api_nwk.h" /* MP: for ZB_PIBCACHE_CURRENT_CHANNEL() */

/* MP: Copy-paste from zb_mac.h. Including zb_mac.h requires additional includes. */
#ifdef ZB_ALIEN_MAC
#define ZB_TRANSCEIVER_START_CHANNEL_NUMBER 11
#endif

#if defined(ZB_TRACE_LEVEL) && defined(ZB_TRACE_TO_FILE) && !defined(ZB_TRACE_TO_SYSLOG) && defined(ZB_BINARY_TRACE)

#ifndef ZB_ALIEN_MAC
#  include "zb_mac_globals.h"
#endif
#include "zb_trace_common.h"
#ifdef ZB_NET_TRACE
#  include "zb_net_trace.h"
#endif

/** @cond DOXYGEN_DEBUG_SECTION */
/** @addtogroup ZB_TRACE
 *  @{
 */

/**
 * @par Trace implementation.
 *
 * Binary trace (local and via network).
 */


void zb_trace_put_bytes(zb_uint16_t file_id, zb_uint8_t *buf, zb_short_t len)
{
    ZVUNUSED(file_id);
    (void)zb_osif_file_write(s_trace_file, buf, len);

#ifdef ZB_NET_TRACE
    if (!zb_nettrace_is_blocked_for(file_id))
    {
        zb_nettrace_put_bytes(buf, len);
    }
#endif
}

/**
 * Output trace message.
 *
 * @param file_id - source file's trace id
 * @param line_number - source file's line number
 * @param args_size - number of added parameters
 */
void zb_trace_msg_bin_file(
    zb_uint_t mask,
    zb_uint_t level,
#if defined ZB_BINARY_AND_TEXT_TRACE_MODE
    zb_char_t *file_name,
#endif
    zb_uint16_t file_id,
    zb_int_t line_number,
    zb_int_t args_size, ...)
{
    zb_uint16_t batch_size;

#if defined ZB_BINARY_AND_TEXT_TRACE_MODE
    ZVUNUSED(file_name);
#endif
    if (!zb_trace_check(level, mask))
    {
        return;
    }

    /* If ZB_TRACE_LEVEL not defined, output nothing */
#ifdef ZB_TRACE_LEVEL
    if (!s_trace_file)
    {
        return;
    }

#ifdef ZB_NET_TRACE
    if (zb_nettrace_is_blocked_for(file_id))
    {
        return;
    }
#endif

    zb_osif_trace_lock();

    /* align args_size to multiple of zb_minimal_vararg_t and
     * calculate the whole size of trace record
     */
    batch_size = zb_trace_rec_size((zb_uint16_t *)&args_size);

#ifdef ZB_NET_TRACE
    zb_nettrace_batch_start(batch_size);
#endif

    zb_trace_put_hdr(file_id, batch_size);
    zb_trace_put_u16(file_id, zb_trace_get_counter() & 0xffff);
    zb_trace_put_u16(file_id, file_id);
    zb_trace_put_u16(file_id, line_number);
    zb_trace_inc_counter();

    {
        va_list arglist;
        zb_int_t size = args_size;

        va_start(arglist, args_size);

        while (size > 0)
        {
            zb_minimal_vararg_t v = va_arg(arglist, zb_minimal_vararg_t);
            zb_trace_put_vararg(file_id, v);
            size -= sizeof(v);
        }

        va_end(arglist);
    }

    zb_osif_file_flush(s_trace_file);
#ifdef ZB_NET_TRACE
    zb_nettrace_batch_flush();
#endif
    zb_osif_trace_unlock();
#else
    (void)file_id;
    (void)line_number;
    (void)args_size;
#endif  /* ZB_TRACE_LEVEL */
}


/** @} */
/** @endcond */ /* DOXYGEN_DEBUG_SECTION */

#endif  /* defined ZB_TRACE_TO_FILE && !defined ZB_TRACE_TO_SYSLOG && defined ZB_BINARY_TRACE */
