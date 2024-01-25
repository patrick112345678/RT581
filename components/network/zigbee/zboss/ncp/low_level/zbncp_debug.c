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
/*  PURPOSE: Debug routines implementation.
*/

#define ZB_TRACE_FILE_ID 29
#include "zbncp_debug.h"

ZBNCP_DBG_STATIC_ASSERT(sizeof(zbncp_uint8_t)   == 1u)
ZBNCP_DBG_STATIC_ASSERT(sizeof(zbncp_uint16_t)  == 2u)
ZBNCP_DBG_STATIC_ASSERT(sizeof(zbncp_uint32_t)  == 4u)
ZBNCP_DBG_STATIC_ASSERT(sizeof(zbncp_uint64_t)  == 8u)
ZBNCP_DBG_STATIC_ASSERT(sizeof(zbncp_uintptr_t) == ZBNCP_PTR_SIZE)

#if ZBNCP_DEBUG

#if !ZBNCP_USE_ZBOSS_TRACE

#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct zbncp_dbg_log_s
{
    const char *name;
    FILE *fp;
}
zbncp_dbg_log_t;

static void zbncp_dbg_log_open(zbncp_dbg_log_t *log)
{
    if (log->name != NULL)
    {
        log->fp = fopen(log->name, "w");
    }

    if (log->fp == NULL)
    {
        log->fp = stdout;
    }
}

static void zbncp_dbg_log_close(zbncp_dbg_log_t *log)
{
    if (log->fp != NULL && log->fp != stdout)
    {
        fclose(log->fp);
        log->fp = NULL;
    }
}

static zbncp_dbg_log_t zbncp_dbg_log; /* = { "LL.log" }; */

static void zbncp_dbg_log_atexit(void)
{
    zbncp_dbg_log_close(&zbncp_dbg_log);
}

static void zbncp_dbg_init_once(zbncp_dbg_log_t *log)
{
    static zbncp_bool_t initialized = ZBNCP_FALSE;
    if (!initialized)
    {
        atexit(zbncp_dbg_log_atexit);
        initialized = ZBNCP_TRUE;
    }

    if (log->fp == NULL)
    {
        zbncp_dbg_log_open(log);
    }
}

static void zbncp_dbg_logv(zbncp_dbg_log_t *log, const char *fmt, va_list ap)
{
    zbncp_dbg_init_once(log);
    vfprintf(log->fp, fmt, ap);
}

void zbncp_dbg_trace(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    zbncp_dbg_logv(&zbncp_dbg_log, fmt, ap);
    va_end(ap);
}

#else /* ZBNCP_USE_ZBOSS_TRACE */

#ifdef RET_OK
#undef RET_OK
#undef RET_ERROR
#undef RET_BLOCKED
#undef RET_EXIT
#undef RET_BUSY
#endif

#include "zb_common.h"
#ifdef ZB_TRACE_TO_FILE

void zb_file_trace_vprintf(const char *format, va_list arglist);

void zbncp_dbg_trace(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    zb_file_trace_vprintf(fmt, ap);
    va_end(ap);
}

#else /* ZB_TRACE_TO_FILE */

void zbncp_dbg_trace(const char *fmt, ...)
{
    TRACE_MSG(TRACE_COMMON3, "zbncp_dbg_trace: ZBOSS trace supported only to files", (FMT__0));
    ZBNCP_UNUSED(fmt);
}

#endif /* ZB_TRACE_TO_FILE */

#endif /* ZBNCP_USE_ZBOSS_TRACE */

zbncp_dbg_res_t zbncp_dbg_assert(const char *file, unsigned int line, const char *cond)
{
    zbncp_dbg_trace(
        "ZBNCP ASSERTION FAILED!\n"
        "  FILE: %s\n"
        "  LINE: %u\n"
        "  COND: '%s' does not hold!\n",
        file, line, cond);

    return ZBNCP_DBG_BREAK; /* for now - always break to the debugger */
}

void zbncp_dbg_break(void)
{
    /* Not implemnted yet */
    for (;;)
    {
        /* Infinite loop */
    }
}

#endif /* ZBNCP_DEBUG */
