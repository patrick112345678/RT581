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
/* PURPOSE: binary trace and traffic dump for platform with trace to serial
*/


#define ZB_TRACE_FILE_ID 2150
#include <stdarg.h>
#include "zb_common.h"
#include "zb_trace_common.h"
#ifdef ZB_NET_TRACE
#include "zb_net_trace.h"
#endif


#if defined ZB_TRACE_TO_PORT && defined ZB_BINARY_TRACE

#if defined ZB_TRACE_LEVEL

#ifdef ZB_NET_TRACE
static int zb_net_trace_is_blocked(zb_uint16_t file_id);
#endif

#endif

#if defined ZB_TRAFFIC_DUMP_ON || defined ZB_TRACE_LEVEL

#ifdef ZB_TRACE_FROM_INTR
ZB_RING_BUFFER_DECLARE(intr_ring, zb_uint8_t, 256);
#endif

typedef struct zb_trace_ctx_s
{
#ifdef ZB_TRACE_FROM_INTR
    intr_ring_t intr_ring;
#endif
} zb_trace_ctx_t;

static zb_trace_ctx_t s_trace_ctx;

void zb_trace_flush_bytes(void)
{
#if defined ZB_MACSPLIT_DEVICE && defined ZB_TRACE_OVER_MACSPLIT
    zb_macsplit_transport_flush_trace();
#endif

#ifdef ZB_NET_TRACE
    if (!ZB_TRACE_INSIDE_INTR())
    {
        zb_nettrace_batch_commit();
    }
#endif

#if defined ZB_TRACE_OVER_JTAG
    zb_osif_jtag_flush();
#endif
}

void zb_trace_put_bytes(zb_uint16_t file_id, zb_uint8_t *buf, zb_short_t len)
{
    zb_bool_t hw_inside_isr = ZB_HW_IS_INSIDE_ISR();
#ifndef ZB_NET_TRACE
    ZVUNUSED(file_id);
#endif

    if (ZB_U2B(g_trace_inside_intr) || hw_inside_isr)
    {
#ifdef ZB_TRACE_FROM_INTR
        /* If we are inside interrupt, put to auxiluary ring buffer */
        while (!ZB_RING_BUFFER_IS_FULL(&s_trace_ctx.intr_ring)
                && (len != 0))
        {
            ZB_RING_BUFFER_PUT(&s_trace_ctx.intr_ring, *buf);
            buf++;
            len--;
        }
#endif
        return;
    }

#if defined ZB_TRACE_OVER_MACSPLIT
    zb_macsplit_transport_put_trace_bytes(buf, len);
#endif
#if defined ZB_TRACE_OVER_JTAG
    zb_osif_jtag_put_bytes(buf, len);
#endif
#if defined ZB_TI13XX_ITM_TRACE
    zb_osif_itm_put_bytes(buf, (zb_uint16_t)len);
#endif
#if defined ZB_SERIAL_FOR_TRACE || defined ZB_TRACE_OVER_USART
    zb_osif_serial_put_bytes(buf, len);
#endif
#if defined ZB_TRACE_OVER_SIF
    zb_osif_sif_put_bytes(buf, len);
#endif

#if defined ZB_NET_TRACE
    /* Block tracing of some files over net. The idea is to exclude of tracing of
     * net tracing itself */
    if (!zb_nettrace_is_blocked_for(file_id))
    {
        zb_nettrace_put_bytes(buf, len);
    }
#endif /* ZB_NET_TRACE */
}

#endif

#ifdef ZB_TRACE_LEVEL
/** @cond DOXYGEN_DEBUG_SECTION */
/** @addtogroup ZB_TRACE
 *  @{
 */

/**
 * Weak implementation of the serial tracing for the single trace message.
 * Can be replaced with another implementation.
 * It should include next steps while the ring buffer is not empty:
 * 1. Get the new portion of trace data from the ring buffer.
 * 2. Tracing.
 * 3. Flushing trace portion. NB! It should be done in EVERY iteration.
 */
ZB_WEAK_PRE void ZB_WEAK zb_trace_msg_port_do()
{
    zb_trace_flush_bytes();

#ifdef ZB_TRACE_FROM_INTR
    while (!ZB_TRACE_INSIDE_INTR()
            && !ZB_RING_BUFFER_IS_EMPTY(&s_trace_ctx.intr_ring))
    {
        zb_uint_t n;
        zb_uint8_t *p = ZB_RING_BUFFER_GET_BATCH(&s_trace_ctx.intr_ring, n);

#if defined ZB_TRACE_OVER_MACSPLIT
        /* it's impossible to use the zb_macsplit_transport_put_trace_bytes() function,
         * because we can brake a trace packet with trace from an interruption
         */
        zb_macsplit_transport_send_trace(p, n);
#elif defined ZB_TRACE_OVER_JTAG
        zb_osif_jtag_put_bytes(p, n);
#elif defined ZB_TI13XX_ITM_TRACE
        zb_osif_itm_put_bytes(p, n);
#elif defined ZB_SERIAL_FOR_TRACE || defined ZB_TRACE_OVER_USART
        zb_osif_serial_put_bytes(p, n);
#endif

        ZB_RING_BUFFER_FLUSH_GET_BATCH(&s_trace_ctx.intr_ring, n);
    }
#endif
}

#ifdef ZB_TRACE_FROM_INTR
void zb_trace_get_last_message(zb_uint8_t **ptr, zb_uint_t *size)
{
    *ptr = ZB_RING_BUFFER_GET_BATCH(&s_trace_ctx.intr_ring, *size);
}

void zb_trace_flush(zb_uint_t size)
{
    ZB_RING_BUFFER_FLUSH_GET_BATCH(&s_trace_ctx.intr_ring, size);
}
#endif  /* #ifdef ZB_TRACE_FROM_INTR */

/**
 * @par Trace implementation.
 *
 * Serial trace (local and via network). */

/**
 * Output trace message.
 *
 * @param file_id - source file id
 * @param line_number - source file line
 * @param args_size - total size of the trace arguments
 * @param ... - trace arguments
 */
void zb_trace_msg_port(
    zb_uint_t mask,
    zb_uint_t level,
    zb_uint16_t file_id,
    zb_uint16_t line_number,
    zb_uint_t  args_size, ...)
{
    zb_uint16_t batch_size;

    if (!zb_trace_check(level, mask) && !ZB_TRACE_INSIDE_INTR_BLOCK())
    {
        return;
    }

#ifdef ZB_NET_TRACE
    if (zb_net_trace_is_blocked(file_id))
    {
        return;
    }
#endif

    /* align args_size to multiple of zb_minimal_vararg_t and
     * calculate the whole size of trace record
     */
    batch_size = zb_trace_rec_size((zb_uint16_t *)&args_size);

#ifdef ZB_NET_TRACE
    if (!ZB_TRACE_INSIDE_INTR())
    {
        zb_nettrace_batch_start(batch_size);
    }
#endif

    zb_trace_put_hdr(file_id, batch_size);
    /* Has no always running timer - print counter */
    /* %d %d %s:%d */
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

    zb_trace_msg_port_do();
}


#ifdef ZB_NET_TRACE
/**
 * Block trace from some source files.
 *
 * The reason is to not trace tracing process itself.
 */
static int zb_net_trace_is_blocked(zb_uint16_t file_id)
{
    int blocked = 1;
#if defined ZB_SERIAL_FOR_TRACE || defined ZB_TRACE_OVER_JTAG
    blocked = 0;
#endif
#ifdef ZB_NET_TRACE
    if (blocked)
    {
        blocked = zb_nettrace_is_blocked_for(file_id);
    }
#endif
    return blocked;
}
#endif

#endif /* trace on */


/** @} */
/** @endcond */ /* DOXYGEN_DEBUG_SECTION */

#endif  /* ZB_TRACE_TO_PORT && ZB_BINARY_TRACE */
