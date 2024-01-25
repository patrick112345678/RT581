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
/*  PURPOSE: New buffer pool API implemented as adapters to the old API
*/

#define ZB_TRACE_FILE_ID 24
#include "zb_common.h"
#include "zboss_api_buf.h"
#include "zb_bufpool.h"
#include "zdo_diagnostics.h"

#include "zb_osif.h"

#ifndef ZB_LEGACY_BUFS

/* Note: disabling interrupts during buffers operations conflicts with trace from buffers subsystem!
   Must switch off trace from buffers.
   Normally we do not use from interrupt handlers buffer pool API which can touch global buffer pool data structures,
   so unlikely that feature is required.
 */
#ifdef ZB_INTERRUPT_SAFE_BUFS
#define ZB_BUF_INT_DISABLE() ZB_OSIF_GLOBAL_LOCK()
#define ZB_BUF_INT_ENABLE() ZB_OSIF_GLOBAL_UNLOCK()
#else /* ZB_INTERRUPT_SAFE_BUFS */
#define ZB_BUF_INT_DISABLE()
#define ZB_BUF_INT_ENABLE()
#endif /* ZB_INTERRUPT_SAFE_BUFS */

/*! \addtogroup buf */
/*! @{ */

#ifdef ZB_DEBUG_BUFFERS_EXT
static void add_buf_usage(zb_bufid_t buf, zb_uint16_t from_file, zb_uint16_t from_line);
static void init_buf_usages(zb_bufid_t buf, zb_uint16_t from_file, zb_uint16_t from_line);
#endif
#ifdef ZB_DEBUG_BUFFERS_EXT_RAF
static void alloc_buf_usages(zb_bufid_t buf, zb_uint16_t from_file, zb_uint16_t from_line);
#ifdef ZB_DEBUG_BUFFERS_RAF_USAGE_RECORD
static void add_buf_usage_table(zb_bufid_t buf, zb_uint16_t from_file, zb_uint16_t from_line);
#endif
#ifdef ZB_DEBUG_BUFFERS_RAF_SIZE_RECORD
static void add_buf_size_table(zb_bufid_t buf, zb_uint16_t from_file, zb_uint16_t from_line, zb_uint8_t size_b, zb_uint8_t size_c, zb_uint8_t size_a);
#endif
static void free_buf_usages(zb_bufid_t buf, zb_uint16_t from_file, zb_uint16_t from_line);
#endif

#ifdef ZB_BUF_SHIELD
static zb_bool_t zb_buf_check_full(TRACE_PROTO zb_bufid_t buf);
#define ZB_BUF_CHECK(param) ZB_ASSERT(zb_buf_check_full(TRACE_CALL param))
#else
#define ZB_BUF_CHECK(param)
#endif /* else #ifdef ZB_BUF_SHIELD */


/* TODO: implement zb_buf_initial_alloc_in? */

#define NBUF_ID_SHIFT (1U)

#define MK_REF(b) (zb_bufpool_storage_buf_to_bufid(b) + NBUF_ID_SHIFT)
#define MK_BUF(r) zb_bufpool_storage_bufid_to_buf((r) - NBUF_ID_SHIFT)
#define REF_UNSHIFTED(r) ((r) - NBUF_ID_SHIFT)

void zb_init_buffers(void)
{
#ifdef ZB_INTERRUPT_SAFE_BUFS
    if (TRACE_ENABLED(TRACE_MEMDBG1))
    {
        TRACE_MSG(TRACE_ERROR, "Buffer pool trace conflicts with safe buffers build leading to lock inside serial trace!", (FMT__0));
        ZB_ASSERT(0);
    }
#endif
    zb_bufpool_storage_init();
}

/*
   The function (if possible) allocates a buffer of required parameters.
   If it is impossible to allocate the required buffer at the moment, NULL will be returned.

   Note that the function is not thread-safe and it is the responsibility of the caller to enforce it.
*/
static zb_bufid_t allocate_buffer_unsafe(TRACE_PROTO zb_uint8_t is_in, zb_bool_t high_priority, zb_uint8_t buf_cnt)
{
    zb_bufid_t buf = 0;
    zb_buf_ent_t *zbbuf;
    zb_uint8_t buf_cnt_limit;
    /* DL: If a "large" block is being allocated, then an action will definitely follow,
       which also requires buffers. Therefore, we are trying to stay out of OOM right now.
    */
    zb_uint8_t buf_cnt_addon = buf_cnt > 1U ? 1U : 0U;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, "allocate_buffer_unsafe is_in %hd high_priority %d buf_cnt %hd", (FMT__D_H_D, is_in, high_priority, buf_cnt));
#else
    TRACE_MSG(TRACE_MEMDBG1, "allocate_buffer_unsafe from ZB_TRACE_FILE_ID %d : %d is_in %hd high_priority %d buf_cnt %hd", (FMT__D_D_H_D_H, from_file, from_line, is_in, high_priority, buf_cnt));
#endif
    /*
      Logically pool divided into 2 parts: input or output.
      Do not allow one part to eat entire another part to exclude deadlock.
     */

    /* 07/12/2019 EE CR:MAJOR When allocating a big buffer, be sure there are some buffers available in the pool - not just HI_PRIOR limit. Else you will not be able to send fragmented buf etc */
    buf_cnt_limit = ZB_BUFS_LIMIT - (high_priority ? 0U : ZB_BUFS_HI_PRIOR_RESERVE);
    if ((ZG->bpool.bufs_allocated[is_in] + buf_cnt + buf_cnt_addon) <= buf_cnt_limit)
    {
        /* 07/12/2019 EE CR:MINOR Woildn't it be simpler if storage return buf id instead of pointer? */
        zbbuf = zb_bufpool_storage_allocate(buf_cnt);
        if (zbbuf != NULL)
        {
            buf = MK_REF(zbbuf);
            ZB_BUF_SET_BUSY(REF_UNSHIFTED(buf));

            /* That bzero is required for split architecture, but not for all ZBOSS builds, while it eats CPU. */
#ifdef ZB_MACSPLIT
            ZB_BZERO(&(zbbuf->buf), zb_bufpool_storage_calc_payload_size(buf_cnt));
#endif

            ZG->bpool.bufs_allocated[is_in] += buf_cnt;
            ZB_ASSERT(ZG->bpool.bufs_allocated[is_in] <= ZB_BUFS_LIMIT);
            zbbuf->hdr.is_in_buf = is_in;

#ifdef ZB_DEBUG_BUFFERS_EXT
            init_buf_usages(buf, from_file, from_line);
#endif
#if !defined ZB_DEBUG_BUFFERS_EXT_RAF && !defined ZB_DEBUG_BUFFERS_EXT && defined ZB_DEBUG_BUFFERS
            ZVUNUSED(from_file);
            ZVUNUSED(from_line);
#endif
        }
    }
    else
    {
        TRACE_MSG(TRACE_MEMDBG1, "ZG->bpool overflow", (FMT__0));
    }

    return buf;
}


zb_bufid_t zb_buf_get_func(TRACE_PROTO zb_bool_t is_in, zb_uint_t max_size)
{
    zb_bufid_t bufid;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_get_func is_in %d max_size %d", (FMT__D_D, is_in, max_size));
#else
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_get_func from ZB_TRACE_FILE_ID %d : %d is_in %d max_size %d", (FMT__D_D_D_D, from_file, from_line, is_in, max_size));
#endif

    if (max_size == 0U)
    {
        /* default value */
        max_size = ZB_IO_BUF_SIZE;
    }

    ZB_BUF_INT_DISABLE();
    bufid = allocate_buffer_unsafe(TRACE_CALL ZB_B2U(is_in), ZB_FALSE, zb_bufpool_storage_calc_multiplicity((zb_uint16_t)max_size));
    ZB_BUF_INT_ENABLE();

    if (bufid == 0U)
    {
        ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_PACKET_BUFFER_ALLOCATE_FAILURES_ID);
    }
    else
    {
#ifdef ZB_DEBUG_BUFFERS_EXT_RAF
        alloc_buf_usages(bufid, from_file, from_line);
#endif
    }

    TRACE_MSG(TRACE_MEMDBG3, "zb_buf_get, ref %hd, in buf %hi {%hd i-o %hd}",
              (FMT__H_H_H_H, bufid, is_in,
               ZG->bpool.bufs_allocated[1], ZG->bpool.bufs_allocated[0]));

    TRACE_MSG(TRACE_MEMDBG1, "<zb_buf_get_func bufid %d", (FMT__D, bufid));
    return bufid;
}


zb_bufid_t zb_buf_get_out_func(TRACE_PROTO_VOID)
{
#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_get_out_func", (FMT__0));
#else
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_get_out_func from ZB_TRACE_FILE_ID %d : %d ", (FMT__D_D, from_file, from_line));
#endif
    return zb_buf_get_func(TRACE_CALL ZB_FALSE, 0);
}


zb_bufid_t zb_buf_get_any_func(TRACE_PROTO_VOID)
{
    zb_bufid_t bufid;
#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_get_any_func", (FMT__0));
#else
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_get_any_func from ZB_TRACE_FILE_ID %d : %d ", (FMT__D_D, from_file, from_line));
#endif

    if (ZG->bpool.bufs_allocated[0] > ZG->bpool.bufs_allocated[1])
    {
        bufid = zb_buf_get_func(TRACE_CALL ZB_TRUE, 0);
    }
    else
    {
        bufid = zb_buf_get_func(TRACE_CALL ZB_FALSE, 0);
    }

    TRACE_MSG(TRACE_MEMDBG1, "<zb_buf_get_any_func bufid %d", (FMT__D, bufid));
    return bufid;
}


zb_bufid_t zb_buf_get_hipri_func(TRACE_PROTO zb_bool_t is_in)
{
    zb_bufid_t bufid;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_get_hipri_func is_in %d", (FMT__D, is_in));
#else
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_get_hipri_func from ZB_TRACE_FILE_ID %d : %d is_in %d", (FMT__D_D_D, from_file, from_line, is_in));
#endif
    ZB_BUF_INT_DISABLE();
    bufid = allocate_buffer_unsafe(TRACE_CALL ZB_B2U(is_in), ZB_TRUE, 1U);
    ZB_BUF_INT_ENABLE();

    if (bufid != 0)
    {
#ifdef ZB_DEBUG_BUFFERS_EXT_RAF
        alloc_buf_usages(bufid, from_file, from_line);
#endif
    }

    TRACE_MSG(TRACE_MEMDBG1, "<zb_buf_get_hipri_func bufid %d", (FMT__D, bufid));
    return bufid;
}


zb_uint_t zb_buf_get_max_size_func(TRACE_PROTO zb_bufid_t buf)
{
#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_get_max_size_func bufid %d", (FMT__D, buf));
#else
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_get_max_size_func from ZB_TRACE_FILE_ID %d : %d bufid %d", (FMT__D_D_D, from_file, from_line, buf));
#endif
    ZB_BUF_CHECK(buf);

    return zb_bufpool_storage_calc_payload_size(MK_BUF(buf)->hdr.multiplicity);
}

/*
  This function is responsible for handling a single delayed buffer request queue entry.

  Note that it is not thread-safe.
*/
static zb_bool_t process_delayed_buf_entry_unsafe(zb_delayed_buf_q_ent_t *ent,
        zb_buffer_types_t buf_type)
{
    zb_bufid_t buf = 0;
    zb_bool_t res = ZB_FALSE;

    TRACE_MSG(TRACE_MEMDBG3, "process_delayed_buf_entry cb %p buf_cnt %hd type %hd",
              (FMT__P_H_H, ZB_DELAYED_BUF_QENT_FPTR(ent), ent->buf_cnt, buf_type));


    /* [conditions below are copied from the old buffers] */

    /* if we need a buffer for rx packet, we should not pass it to some */
    /* other callback. Out buffer can be handled unconditionally */
    if (buf_type == ZB_OUT_BUFFER
            || (buf_type == ZB_IN_BUFFER
                && !ZB_U2B(ZG->bpool.mac_rx_need_buf)
#ifdef ZB_MAC_RX_QUEUE_CAP
                && (ZB_IOBUF_POOL_SIZE / 2U - ZG->bpool.bufs_allocated[1] > ZB_MAC_RX_QUEUE_CAP)
#endif
               )
       )
    {
        buf = allocate_buffer_unsafe(TRACE_CALL buf_type, ZB_FALSE, ent->buf_cnt);
    }

    ZB_BUF_INT_ENABLE();
    if (buf != 0U)
    {
#ifdef ZB_DEBUG_BUFFERS_EXT_RAF
        alloc_buf_usages(buf, 0, 0);
#endif
        res = ZB_TRUE;
        if (ZB_U2B(ent->is_2param))
        {
            ZB_SCHEDULE_CALLBACK2(ent->u.func2_ptr, buf, ent->user_param);
        }
        else
        {
            //ZB_SCHEDULE_CALLBACK(ent->u.func_ptr, buf);
            zb_schedule_callback(ent->u.func_ptr, buf);
        }
    }
    else
    {
        TRACE_MSG(TRACE_MEMDBG3, "failed to allocate buffer", (FMT__0));
    }
    ZB_BUF_INT_DISABLE();

    TRACE_MSG(TRACE_MEMDBG3, "process_delayed_buf_entry res %d buf %d", (FMT__H_H, res, buf));
    return res;
}

/*
  Attempts to process the front element of  the specified delayed buffer request queue and,
  if successful, drops that element from the queue.

  This function is not thread-safe.
*/
static void attempt_delayed_buffer_queue_flush_unsafe(zb_buffer_types_t buf_type)
{
    zb_delayed_buf_q_ent_t *ent_p;

    TRACE_MSG(TRACE_MEMDBG3, "attempt_delayed_buffer_queue_flush type %hd",
              (FMT__H, buf_type));

    ent_p = ZB_RING_BUFFER_PEEK(&ZG->sched.delayed_queue[buf_type]);
    if (ent_p != NULL)
    {
        if (process_delayed_buf_entry_unsafe(ent_p, buf_type))
        {
            ZB_RING_BUFFER_FLUSH_GET(&ZG->sched.delayed_queue[buf_type]);
            TRACE_MSG(TRACE_MEMDBG1, "delayed_buf_usage was %hd buf_cnt %hd",
                      (FMT__H_H, ZG->sched.delayed_buf_usage, ent_p->buf_cnt));
            ZB_ASSERT(ZG->sched.delayed_buf_usage >= ent_p->buf_cnt);
            ZG->sched.delayed_buf_usage -= ent_p->buf_cnt;
        }
    }
}

static zb_ret_t zb_get_buf_delayed(zb_delayed_buf_q_ent_t *ent,
                                   zb_buffer_types_t buf_type)
{
    zb_bool_t already_scheduled;

    ZB_BUF_INT_DISABLE();
    already_scheduled = process_delayed_buf_entry_unsafe(ent, buf_type);
    ZB_BUF_INT_ENABLE();

    if (!already_scheduled)
    {
        zb_delayed_buf_q_ent_t *q_ent;

        ZB_BUF_INT_DISABLE();
        q_ent = ZB_RING_BUFFER_PUT_RESERVE(&ZG->sched.delayed_queue[buf_type]);
        if (q_ent != NULL)
        {
            if (ent->is_2param == 0)
            {
                *q_ent = *ent;
                ZB_RING_BUFFER_FLUSH_PUT(&ZG->sched.delayed_queue[buf_type]);
                ZG->sched.delayed_buf_usage += ent->buf_cnt;
                TRACE_MSG(TRACE_MEMDBG1, "delayed_buf_usage %hd", (FMT__H, ZG->sched.delayed_buf_usage));
                TRACE_MSG(TRACE_MEMDBG1, "new delayed_queue[%hd]->written %hd ent %p",
                          (FMT__H_H_P, buf_type, (zb_uint8_t)ZG->sched.delayed_queue[buf_type].written, *ent));
            }
            else
            {
                TRACE_MSG(TRACE_MEMDBG1, "Don't put it into delayed_queue because is_2param = 1.", (FMT__0));
            }
        }
        else
        {
            ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_PACKET_BUFFER_ALLOCATE_FAILURES_ID);
        }

        ZB_BUF_INT_ENABLE();
        if (q_ent != NULL)
        {
            TRACE_MSG(TRACE_ERROR, "delayed buf q[%hd] is full", (FMT__H, buf_type));
            return RET_ERROR;
        }
    }

    return RET_OK;
}

static zb_ret_t zb_get_buf_delayed_1param(zb_callback_t callback,
        zb_buffer_types_t buf_type,
        zb_uint8_t buf_cnt)
{
    zb_ret_t ret;
    zb_delayed_buf_q_ent_t ent;

#ifdef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG2, "zb_get_buf_delayed_1param cb %p type %hd from ISR %hd",
              (FMT__P_H_H, callback, (zb_uint8_t)buf_type, (zb_uint8_t)ZB_HW_IS_INSIDE_ISR()));
#else
    TRACE_MSG(TRACE_MEMDBG1, "zb_get_buf_delayed_1param cb %p type %d",
              (FMT__P_H, callback, (zb_uint8_t)buf_type));
#endif

    ent.u.func_ptr = callback;
    ent.is_2param = 0;
    ent.buf_cnt = buf_cnt;
    ent.user_param = 0;

    ret = zb_get_buf_delayed(&ent, buf_type);

    return ret;
}

static zb_ret_t zb_get_buf_delayed_2param(zb_callback2_t callback,
        zb_buffer_types_t buf_type,
        zb_uint16_t user_param,
        zb_uint8_t buf_cnt)
{
    zb_ret_t ret;
    zb_delayed_buf_q_ent_t ent;

#ifdef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG2, "zb_get_buf_delayed_2param cb %p type %hd par2 %hd from ISR %hd",
              (FMT__P_H_H_H_H, callback, (zb_uint8_t)buf_type, (zb_uint8_t)user_param,
               (zb_uint8_t)ZB_HW_IS_INSIDE_ISR()));
#else
    TRACE_MSG(TRACE_MEMDBG1, "zb_get_buf_delayed_2param cb %p type %hd par2 %hd",
              (FMT__P_H_H_H, callback, (zb_uint8_t)buf_type, (zb_uint8_t)user_param));
#endif

    ent.u.func2_ptr = callback;
    ent.is_2param = 1;
    ent.buf_cnt = buf_cnt;
    ent.user_param = user_param;

    ret = zb_get_buf_delayed(&ent, buf_type);

    return ret;
}

zb_ret_t zb_buf_get_out_delayed_func(TRACE_PROTO zb_callback_t callback)
{
    zb_ret_t ret;
#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_get_out_delayed_func callback %p", (FMT__P, callback));
#else
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_get_out_delayed_func from ZB_TRACE_FILE_ID %d : %d callback %p", (FMT__D_D_P, from_file, from_line, callback));
#endif
    ret = zb_get_buf_delayed_1param(callback,
                                    ZB_OUT_BUFFER,  /* buf type */
                                    zb_bufpool_storage_calc_multiplicity(ZB_IO_BUF_SIZE));

    TRACE_MSG(TRACE_MEMDBG1, "<zb_buf_get_out_delayed_func ret %d", (FMT__D, ret));
    return ret;
}


zb_ret_t zb_buf_get_in_delayed_func(TRACE_PROTO zb_callback_t callback)
{
    zb_ret_t ret;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_get_in_delayed_func callback %p", (FMT__P, callback));
#else
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_get_in_delayed_func from ZB_TRACE_FILE_ID %d : %d callback %p", (FMT__D_D_P, from_file, from_line, callback));
#endif
    ret = zb_get_buf_delayed_1param(callback,
                                    ZB_IN_BUFFER,   /* buf type */
                                    zb_bufpool_storage_calc_multiplicity(ZB_IO_BUF_SIZE));
    TRACE_MSG(TRACE_MEMDBG1, "<zb_buf_get_in_delayed_func ret %d", (FMT__D, ret));
    return ret;
}


zb_ret_t  zb_buf_get_out_delayed_ext_func(TRACE_PROTO zb_callback2_t callback, zb_uint16_t arg, zb_uint_t max_size)
{
    zb_ret_t ret;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_get_out_delayed_ext_func callback %p arg %d max_size %d", (FMT__P_D_D, callback, arg, max_size));
#else
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_get_out_delayed_ext_func from ZB_TRACE_FILE_ID %d : %d callback %p arg %d max_size %d", (FMT__D_D_P_D_D, from_file, from_line, callback, arg, max_size));
#endif

    if (max_size == 0U)
    {
        /* default value */
        max_size = ZB_IO_BUF_SIZE;
    }

    ret = zb_get_buf_delayed_2param(callback,
                                    ZB_OUT_BUFFER, /* buf type */
                                    arg,           /* user_param */
                                    zb_bufpool_storage_calc_multiplicity((zb_uint16_t)max_size));
    return ret;
}


zb_ret_t  zb_buf_get_in_delayed_ext_func(TRACE_PROTO zb_callback2_t callback, zb_uint16_t arg, zb_uint_t max_size)
{
    zb_ret_t ret;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_get_in_delayed_ext_func callback %p arg %d max_size %d", (FMT__P_D_D, callback, arg, max_size));
#else
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_get_in_delayed_ext_func from ZB_TRACE_FILE_ID %d : %d callback %p arg %d max_size %d", (FMT__D_D_P_D_L, from_file, from_line, callback, arg, max_size));
#endif
    if (max_size == 0U)
    {
        /* default value */
        max_size = ZB_IO_BUF_SIZE;
    }

    ret = zb_get_buf_delayed_2param(callback,
                                    ZB_IN_BUFFER,  /* buf type */
                                    arg,           /* user_param */
                                    zb_bufpool_storage_calc_multiplicity((zb_uint16_t)max_size));
    return ret;
}


zb_ret_t zb_buf_requalify_in_to_out_func(TRACE_PROTO zb_bufid_t buf)
{
    zb_buf_ent_t *zbbuf;
    zb_ret_t ret = RET_OK;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, ">> zb_buf_requalify_in_to_out_func buf %hd", (FMT__H, buf));
#else
    TRACE_MSG(TRACE_MEMDBG1, ">> zb_buf_requalify_in_to_out_func from ZB_TRACE_FILE_ID %d : %d buf %hd", (FMT__D_D_H, from_file, from_line, buf));
#endif

    ZB_BUF_CHECK(buf);

    zbbuf = MK_BUF(buf);

    if (!zbbuf->hdr.is_in_buf)
    {
        ret = RET_INVALID_PARAMETER_1;
    }

    ZB_BUF_INT_DISABLE();

    /* Buffer requalification is needed only if "in" buffers count is greater than "out" buffers count */
    if (ZG->bpool.bufs_allocated[1] <= ZG->bpool.bufs_allocated[0]
            || (ZG->bpool.bufs_allocated[0] + zbbuf->hdr.multiplicity
                > ZB_BUFS_LIMIT - ZB_BUFS_HI_PRIOR_RESERVE))
    {
        ret = RET_IGNORE;
    }

    if (ret == RET_OK)
    {
        /* Change "in" buffer type to "out" */
        ZG->bpool.bufs_allocated[1] -= zbbuf->hdr.multiplicity;
        ZG->bpool.bufs_allocated[0] += zbbuf->hdr.multiplicity;

        zbbuf->hdr.is_in_buf = !zbbuf->hdr.is_in_buf;
        ZB_ASSERT(ZG->bpool.bufs_allocated[0] <= ZB_BUFS_LIMIT);
    }

    ZB_BUF_INT_ENABLE();

    TRACE_MSG(TRACE_MEMDBG1, "<< zb_buf_requalify_in_to_out_func, ret %d", (FMT__D, ret));

    return ret;
}


void zb_buf_free_func(TRACE_PROTO zb_bufid_t buf)
{
    zb_buf_ent_t *zbbuf;
    zb_buffer_types_t is_in_buf;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_free_func bufid %d", (FMT__D, buf));
#else
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_free_func from ZB_TRACE_FILE_ID %d : %d bufid %d", (FMT__D_D_D, from_file, from_line, buf));
#endif
    ZB_BUF_CHECK(buf);

    zbbuf = MK_BUF(buf);
    is_in_buf = zbbuf->hdr.is_in_buf;

    ZB_ASSERT(ZG->bpool.bufs_allocated[is_in_buf] >= zbbuf->hdr.multiplicity);

    /* critical section */
    ZB_BUF_INT_DISABLE();

    ZB_BUF_CLEAR_BUSY(REF_UNSHIFTED(buf));
    zb_bufpool_storage_free(zbbuf);

#ifdef ZB_DEBUG_BUFFERS_EXT_RAF
    free_buf_usages(buf, from_file, from_line);
#endif

    ZB_ASSERT(zbbuf->hdr.multiplicity > 0U);
    ZG->bpool.bufs_allocated[is_in_buf] -= zbbuf->hdr.multiplicity;

    ZB_BUF_INT_ENABLE();

    /* Maybe, unlock IN processing in nwk */
    ZB_NWK_UNLOCK_IN(buf);

    ZB_BUF_INT_DISABLE();
    attempt_delayed_buffer_queue_flush_unsafe(is_in_buf);
    ZB_BUF_INT_ENABLE();

    TRACE_MSG(TRACE_MEMDBG3, "zb_buf_free, ref %hd, in buf %hi {%hd i-o %hd}",
              (FMT__H_H_H_H, buf, zbbuf->hdr.is_in_buf,
               ZG->bpool.bufs_allocated[1], ZG->bpool.bufs_allocated[0]));

    TRACE_MSG(TRACE_MEMDBG3, "<< zb_buf_free_func", (FMT__0));
}


void *zb_buf_begin_func(TRACE_PROTO zb_bufid_t buf)
{
    zb_buf_ent_t *buf_p;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_begin_func bufid %d", (FMT__D, buf));
#else
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_begin_func from ZB_TRACE_FILE_ID %d : %d bufid %d", (FMT__D_D_D, from_file, from_line, buf));
#endif
    ZB_BUF_CHECK(buf);

    buf_p = MK_BUF(buf);
    return ((buf_p)->buf + (buf_p)->hdr.data_offset);
}

zb_bufid_t zb_buf_from_data0_func(TRACE_PROTO void *ptr)
{
    void *buf_p;
    zb_size_t offset = ZB_OFFSETOF(zb_buf_ent_t, buf);

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_from_data0_func ptr %p", (FMT__P, ptr));
#else
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_from_data0_func from ZB_TRACE_FILE_ID %d : %d ptr %p", (FMT__D_D_P, from_file, from_line, ptr));
#endif
    buf_p = (zb_int8_t *)(ptr) - offset;

    return MK_REF((zb_buf_ent_t *)buf_p);
}


void *zb_buf_end_func(TRACE_PROTO zb_bufid_t buf)
{
#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_end_func bufid %d", (FMT__D, buf));
#else
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_end_func from ZB_TRACE_FILE_ID %d : %d bufid %d", (FMT__D_D_D, from_file, from_line, buf));
#endif
    ZB_BUF_CHECK(buf);

    return (zb_uint8_t *)zb_buf_begin_func(TRACE_CALL buf) + zb_buf_len_func(TRACE_CALL buf);
}


zb_uint_t zb_buf_len_func(TRACE_PROTO zb_bufid_t buf)
{
#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_len_func bufid %d", (FMT__D, buf));
#else
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_len_func from ZB_TRACE_FILE_ID %d : %d bufid %d", (FMT__D_D_D, from_file, from_line, buf));
#endif
    ZB_BUF_CHECK(buf);

    return MK_BUF(buf)->hdr.len;
}


#ifdef ZB_TH_ENABLED

void zb_buf_set_len_func(TRACE_PROTO zb_bufid_t buf, zb_uint16_t len)
{
    zb_buf_ent_t *b;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_set_len_func bufid %d len %d", (FMT__D_D, buf, len));
#else
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_set_len_func from ZB_TRACE_FILE_ID %d : %d bufid %d len %d", (FMT__D_D_D_D, from_file, from_line, buf, len));
#endif
    ZB_BUF_CHECK(buf);

    b = MK_BUF(buf);
    ZB_ASSERT(len + b->hdr.data_offset < ZB_IO_BUF_SIZE);
    b->hdr.len = len;
}
#endif /* ZB_TH_ENABLED */


void *zb_buf_data_func(TRACE_PROTO zb_bufid_t buf, zb_uint_t off)
{
#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_data_func bufid %d off %d", (FMT__D_D, buf, off));
#else
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_data_func from ZB_TRACE_FILE_ID %d : %d bufid %d off %d", (FMT__D_D_D_D, from_file, from_line, buf, off));
#endif
    ZB_BUF_CHECK(buf);

    return (zb_uint8_t *)zb_buf_begin_func(TRACE_CALL buf) + off;
}

#ifdef ZB_TH_ENABLED

zb_uint16_t zb_buf_get_offset_func(TRACE_PROTO zb_bufid_t buf)
{
    zb_buf_ent_t *b;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_get_data_offset_func bufid %d", (FMT__D, buf));
#else
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_get_data_offset_func from ZB_TRACE_FILE_ID %d : %d bufid %d", (FMT__D_D_D, from_file, from_line, buf));
#endif
    ZB_BUF_CHECK(buf);

    b = MK_BUF(buf);
    return b->hdr.data_offset;
}
#endif /* ZB_TH_ENABLED */

#ifdef ZB_MACSPLIT
zb_uint8_t zb_buf_serialize_func(TRACE_PROTO zb_bufid_t buf, zb_uint8_t *ptr)
{
    zb_uint8_t *p;
    zb_uint_t param_offset;
    zb_uint_t buf_len;
    zb_uint_t size;
    zb_uint_t max_size;
    zb_buf_ent_t *zbbuf;
#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_serialize_func bufid %d ptr %p", (FMT__D_P, buf, ptr));
#else
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_serialize_func from ZB_TRACE_FILE_ID %d : %d bufid %d ptr %p", (FMT__D_D_D_P, from_file, from_line, buf, ptr));
#endif
    ZB_BUF_CHECK(buf);
    zbbuf = MK_BUF(buf);

    buf_len = zbbuf->hdr.len;
    max_size = zb_bufpool_storage_calc_payload_size(zbbuf->hdr.multiplicity);

    /* Fill header */
    ZB_MEMCPY(ptr, &zbbuf->hdr, sizeof(zb_buf_hdr_t));
    /* Fill body */
    ZB_MEMCPY(ptr + sizeof(zb_buf_hdr_t), zb_buf_begin_func(TRACE_CALL buf), buf_len);
    param_offset = zbbuf->hdr.data_offset + buf_len;
    /* Optimization for params data, no need to pass zero bytes because other side should
     * receive data and write it to an empty (zero) buffer */
    p = zbbuf->buf;
    while (param_offset < max_size && p[param_offset] == 0)
    {
        param_offset++;
    }
    /* Fill parameter */
    ZB_MEMCPY(ptr + sizeof(zb_buf_hdr_t) + buf_len,
              p + param_offset,
              max_size - param_offset);
    size = max_size - param_offset + sizeof(zb_buf_hdr_t) + buf_len;
    TRACE_MSG(TRACE_MEMDBG1, "<zb_buf_serialize_func param_size %d", (FMT__D, size));
    return size;
}


void zb_buf_deserialize_func(TRACE_PROTO zb_bufid_t buf, zb_uint8_t *ptr, zb_uint8_t payload_size)
{
    zb_uint8_t *int_ptr;
    zb_uint16_t param_size;
    zb_uint_t is_in_buf;
    zb_uint_t buf_len;
    zb_uint8_t prev_multiplicity;
    zb_uint_t max_size;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_deserialize_func bufid %d ptr %p payload_size %d", (FMT__D_P_D, buf, ptr, payload_size));
#else
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_deserialize_func from ZB_TRACE_FILE_ID %d : %d bufid %d ptr %p", (FMT__D_D_D_P, from_file, from_line, buf, ptr));
#endif
    ZB_BUF_CHECK(buf);

    is_in_buf = zb_buf_flags_get(buf) & ZB_BUF_IS_IN;
    prev_multiplicity = MK_BUF(buf)->hdr.multiplicity;

    /* Fill header */
    ZB_MEMCPY(&MK_BUF(buf)->hdr, ptr, sizeof(zb_buf_hdr_t));
    ZB_ASSERT(MK_BUF(buf)->hdr.multiplicity == prev_multiplicity);

    max_size = zb_bufpool_storage_calc_payload_size(prev_multiplicity);
    buf_len = zb_buf_len_func(TRACE_CALL buf);
    int_ptr = zb_buf_begin_func(TRACE_CALL buf);
    /* Fill body */
    ZB_MEMCPY(int_ptr, ptr + sizeof(zb_buf_hdr_t), buf_len);
    /* Fill parameter */
    param_size = payload_size - sizeof(zb_buf_hdr_t) - buf_len;
    int_ptr = &MK_BUF(buf)->buf[max_size - param_size];
    /* It all works because get_in_buf fills buffer contents by zeroes. */
    ZB_MEMCPY(int_ptr, ptr + sizeof(zb_buf_hdr_t) + buf_len, param_size);
    /* prevent original buf type corruption */
    zb_buf_flags_clr(buf, ZB_BUF_IS_IN);
    if (is_in_buf)
    {
        zb_buf_flags_or(buf, ZB_BUF_IS_IN);
    }
    TRACE_MSG(TRACE_MEMDBG1, "<zb_buf_deserialize_func", (FMT__0));
}

zb_uint8_t *zb_buf_partial_deserialize_func(TRACE_PROTO zb_uint8_t *ptr, zb_uint8_t *size)
{
    zb_uint8_t *out_ptr;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_partial_deserialize ptr %p ", (FMT__P, ptr));
#else
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_partial_deserialize from ZB_TRACE_FILE_ID %d : %d ptr %p", (FMT__D_D_P, from_file, from_line, ptr));
#endif
    out_ptr = ptr + sizeof(zb_buf_hdr_t);
    *size = sizeof(zb_buf_hdr_t);
    TRACE_MSG(TRACE_MEMDBG1, "<zb_buf_partial_deserialize out_ptr %p", (FMT__P, ptr));
    return out_ptr;
}

#endif  /* ZB_MACSPLIT */


zb_uint8_t *zb_buf_data0_func(TRACE_PROTO zb_bufid_t buf)
{
    zb_uint8_t *p;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_data0_func bufid %d", (FMT__D, buf));
#else
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_data0_func from ZB_TRACE_FILE_ID %d : %d bufid %d", (FMT__D_D_D, from_file, from_line, buf));
#endif
    ZB_BUF_CHECK(buf);

    p = MK_BUF(buf)->buf;
    TRACE_MSG(TRACE_MEMDBG1, "<zb_buf_data0_func ret %p", (FMT__P, p));
    return p;
}


void zb_buf_copy_func(TRACE_PROTO zb_bufid_t dst_buf, zb_bufid_t src_buf)
{
    zb_uint8_t is_in;
    zb_buf_ent_t *dst_buf_p;
    zb_buf_ent_t *src_buf_p;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_copy_func dst_bufid %d src_bufid %d", (FMT__D_D, dst_buf, src_buf));
#else
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_copy_func from ZB_TRACE_FILE_ID %d : %d dst_bufid %d src_bufid %d", (FMT__D_D_D_D, from_file, from_line, dst_buf, src_buf));
#endif
    ZB_ASSERT(src_buf != 0U);
    ZB_ASSERT(dst_buf != 0U);

    ZB_BUF_CHECK(dst_buf);
    ZB_BUF_CHECK(src_buf);

    dst_buf_p = MK_BUF(dst_buf);
    src_buf_p = MK_BUF(src_buf);

    if (dst_buf_p->hdr.multiplicity == src_buf_p->hdr.multiplicity)
    {
        is_in = dst_buf_p->hdr.is_in_buf;

        ZB_MEMCPY(dst_buf_p, src_buf_p, sizeof(zb_buf_ent_t) * src_buf_p->hdr.multiplicity);
        dst_buf_p->hdr.is_in_buf = is_in;
    }
    else
    {
        ZB_ASSERT(0);
    }
}


void *zb_buf_initial_alloc_func(TRACE_PROTO zb_bufid_t buf, zb_uint_t size)
{
    zb_uint8_t is_in_buf;
    zb_uint8_t *p;
    zb_uint8_t multiplicity;
    zb_uint_t max_size;
    zb_buf_ent_t *zbbuf;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_initial_alloc_func bufid %d size %d", (FMT__D_D, buf, size));
#else
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_initial_alloc_func from ZB_TRACE_FILE_ID %d : %d bufid %d size %d", (FMT__D_D_D_D, from_file, from_line, buf, size));
#endif
    ZB_BUF_CHECK(buf);
    zbbuf = MK_BUF(buf);

    is_in_buf = zbbuf->hdr.is_in_buf;
    multiplicity = zbbuf->hdr.multiplicity;
    max_size = zb_bufpool_storage_calc_payload_size(multiplicity);

    ZB_ASSERT(size < max_size);

#if defined ZB_DEBUG_BUFFERS_EXT_RAF && defined ZB_DEBUG_BUFFERS_RAF_SIZE_RECORD
    zb_uint_t size_before;
    size_before = zbbuf->hdr.len;
#endif
    ZB_BZERO(&zbbuf->hdr, sizeof(zbbuf->hdr));
    zbbuf->hdr.is_in_buf = is_in_buf;
    zbbuf->hdr.len = (zb_uint16_t)size;
    zbbuf->hdr.multiplicity = multiplicity;
    zbbuf->hdr.data_offset = (zb_uint16_t)(max_size - size) / 2U;

    {
        /* create aligned pointer to body begin - just in case in general. Useful for signal send. */
        zb_size_t align = ((zb_size_t)(zbbuf)->buf + (zb_size_t)(zbbuf)->hdr.data_offset) % ZB_BUF_ALLOC_ALIGN;
        if (align > 0U)                  /* Need to align */
        {
            if ((zbbuf)->hdr.data_offset >= align) /* Can align to left */
            {
                (zbbuf)->hdr.data_offset -= (zb_uint16_t)align;
            }
        }
    }

#if defined ZB_DEBUG_BUFFERS_EXT_RAF && defined ZB_DEBUG_BUFFERS_RAF_SIZE_RECORD
    add_buf_size_table(buf, from_file, from_line, size_before, size, zbbuf->hdr.len);
#endif

    /* Maybe, unlock IN processing in nwk */
    ZB_NWK_UNLOCK_IN(buf);

    p = (void *)zb_buf_begin_func(TRACE_CALL buf);
    TRACE_MSG(TRACE_MEMDBG1, "<zb_buf_initial_alloc_func ret %p", (FMT__P, p));
    return p;
}


void *zb_buf_reuse_func(TRACE_PROTO zb_bufid_t buf)
{
    zb_uint8_t is_in_buf;
    zb_uint8_t multiplicity;
    zb_buf_ent_t *zbbuf;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_reuse_func bufid %d", (FMT__D, buf));
#else
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_reuse_func from ZB_TRACE_FILE_ID %d : %d bufid %d", (FMT__D_D_D, from_file, from_line, buf));
#endif
    ZB_BUF_CHECK(buf);

    zbbuf = MK_BUF(buf);
    is_in_buf = zbbuf->hdr.is_in_buf;
    multiplicity = zbbuf->hdr.multiplicity;

#ifdef ZB_DEBUG_BUFFERS_EXT
    add_buf_usage(buf, from_file, from_line);
#endif
#if defined ZB_DEBUG_BUFFERS_EXT_RAF && defined ZB_DEBUG_BUFFERS_RAF_USAGE_RECORD
    add_buf_usage_table(buf, from_file, from_line);
#endif

    TRACE_MSG(TRACE_MEMDBG3, "zb_buf_reuse %p, ref %hd, in buf %hi {%hd i-o %hd}",
              (FMT__P_H_H_H_H, zbbuf, buf, zbbuf->hdr.is_in_buf,
               ZG->bpool.bufs_allocated[1], ZG->bpool.bufs_allocated[0]));
    /* Maybe, unlock IN processing in nwk */
    ZB_NWK_UNLOCK_IN(buf);

    ZB_BZERO(&zbbuf->hdr, sizeof(zbbuf->hdr));
    /* EE: That bzero is required for split architectue, but not for all ZBOSS builds, while it eats CPU. */
#ifdef ZB_MACSPLIT
    ZB_BZERO(&(zbbuf->buf), zb_bufpool_storage_calc_payload_size(multiplicity));
#endif

    zbbuf->hdr.multiplicity = multiplicity;
    zbbuf->hdr.is_in_buf = is_in_buf;

    TRACE_MSG(TRACE_MEMDBG1, "<zb_buf_reuse_func ret %p", (FMT__P, zbbuf->buf));
    return zbbuf->buf;
}


zb_uint_t zb_buf_get_ptr_off_func(TRACE_PROTO zb_bufid_t buf, zb_uint8_t *ptr)
{
    zb_uint8_t off;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_get_ptr_off_func bufid %d ptr %p", (FMT__D_P, buf, ptr));
#else
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_get_ptr_off_func from ZB_TRACE_FILE_ID %d : %d bufid %d ptr %p", (FMT__D_D_D_P, from_file, from_line, buf, ptr));
#endif
    ZB_BUF_CHECK(buf);

    off = ptr - (zb_uint8_t *)zb_buf_begin_func(TRACE_CALL buf);
    TRACE_MSG(TRACE_MEMDBG1, "<zb_buf_get_ptr_off_func ret %d", (FMT__D, off));
    return (zb_uint_t)off;
}


static void *zb_get_buf_tail_ptr(zb_bufid_t buf, zb_uint8_t size)
{
    zb_buf_ent_t *zbbuf;
    zb_uint_t max_size;
    zb_uint_t right_end;
    zb_uint_t len_available;

    ZB_BUF_CHECK(buf);
    zbbuf = MK_BUF(buf);
    ZB_ASSERT(zbbuf);

    /* Align size to 4.
       Rationale: may have unaligned memory access errors when requested size is not aligned to 4. */
    if ((size % ZB_BUF_ALLOC_ALIGN) != 0U)
    {
        zb_uint8_t a = ZB_BUF_ALLOC_ALIGN;

        TRACE_MSG(TRACE_MEMDBG1, "zb_buf_get_tail_ptr: align size %d->%d", (FMT__D_D, size, (((size + a - 1) / a) * a)));
        size = ((size + a - 1U) / a) * a;
    }
    max_size = zb_bufpool_storage_calc_payload_size(zbbuf->hdr.multiplicity);
    len_available = (zb_uint_t)zbbuf->hdr.len + (zb_uint_t)size;
    if (len_available > max_size)
    {
        TRACE_MSG(TRACE_ERROR, "corrupted buf %p %hd", (FMT__P_H, zbbuf, buf));
        TRACE_MSG(TRACE_ERROR, "buf_len %hd size %hd max_size %hd ZB_IO_BUF_SIZE %d", (FMT__H_H_D_D, zbbuf->hdr.len, size, max_size, ZB_IO_BUF_SIZE));
    }

    ZB_ASSERT(len_available <= max_size);

    right_end = zbbuf->hdr.data_offset + len_available;
    if (right_end > max_size)
    {
        (void)zb_buf_alloc_right_func(TRACE_CALL buf, size);
        zb_buf_cut_right_func(TRACE_CALL buf, size);
    }

    return (void *)(zbbuf->buf + (max_size - size));
}


void *zb_buf_get_tail_func(TRACE_PROTO zb_bufid_t buf, zb_uint_t size)
{
    void *p;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_get_tail_func bufid %d size %d", (FMT__D_P, buf, size));
#else
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_get_tail_func from ZB_TRACE_FILE_ID %d : %d bufid %d size %d", (FMT__D_D_D_D, from_file, from_line, buf, size));
#endif
    ZB_BUF_CHECK(buf);

#ifdef ZB_DEBUG_BUFFERS_EXT
    add_buf_usage(buf, from_file, from_line);
#endif
#if defined ZB_DEBUG_BUFFERS_EXT_RAF && defined ZB_DEBUG_BUFFERS_RAF_USAGE_RECORD
    add_buf_usage_table(buf, from_file, from_line);
#endif

    p = zb_get_buf_tail_ptr(buf, (zb_uint8_t)size);
    TRACE_MSG(TRACE_MEMDBG1, "<zb_buf_get_tail_func_func ret %p", (FMT__P, p));
    return p;
}


void *zb_buf_alloc_tail_func(TRACE_PROTO zb_bufid_t buf, zb_uint_t size)
{
    void *p;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_get_alloc_func bufid %d size %d", (FMT__D_P, buf, size));
#else
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_get_alloc_func from ZB_TRACE_FILE_ID %d : %d bufid %d size %d", (FMT__D_D_D_D, from_file, from_line, buf, size));
#endif
    ZB_BUF_CHECK(buf);

#ifdef ZB_DEBUG_BUFFERS_EXT
    add_buf_usage(buf, from_file, from_line);
#endif
#if defined ZB_DEBUG_BUFFERS_EXT_RAF && defined ZB_DEBUG_BUFFERS_RAF_USAGE_RECORD
    add_buf_usage_table(buf, from_file, from_line);
#endif

    p = zb_get_buf_tail_ptr(buf, (zb_uint8_t)size);
    if (p != NULL)
    {
        ZB_BZERO(p, size);
    }
    TRACE_MSG(TRACE_MEMDBG1, "<zb_buf_alloc_tail_func_func ret %p", (FMT__P, p));
    return p;
}


void zb_buf_cut_right_func(TRACE_PROTO zb_bufid_t buf, zb_uint_t size)
{
    zb_buf_ent_t *zbbuf;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_cut_right_func bufid %d size %d", (FMT__D_P, buf, size));
#else
    TRACE_MSG(TRACE_MEMDBG1, "zb_buf_cut_right_func from ZB_TRACE_FILE_ID %d : %d bufid %d size %d", (FMT__D_D_D_D, from_file, from_line, buf, size));
#endif
    ZB_BUF_CHECK(buf);

    zbbuf = MK_BUF(buf);
    ZB_ASSERT(zbbuf != NULL);

    ZB_ASSERT(zb_buf_len_func(TRACE_CALL buf) >= (size));

#if defined ZB_DEBUG_BUFFERS_EXT_RAF && defined ZB_DEBUG_BUFFERS_RAF_SIZE_RECORD
    zb_uint_t size_before;
    size_before = zbbuf->hdr.len;
#endif
    zbbuf->hdr.len -= (zb_uint16_t)size;

#if defined ZB_DEBUG_BUFFERS_EXT_RAF && defined ZB_DEBUG_BUFFERS_RAF_SIZE_RECORD
    add_buf_size_table(buf, from_file, from_line, size_before, size, zbbuf->hdr.len);
#endif
}


void *zb_buf_cut_left_func(TRACE_PROTO zb_bufid_t buf, zb_uint_t size)
{
    zb_buf_ent_t *zbbuf;
    void *p;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_cut_left_func bufid %d size %d", (FMT__D_P, buf, size));
#else
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_cut_left_func from ZB_TRACE_FILE_ID %d : %d bufid %d size %d", (FMT__D_D_D_D, from_file, from_line, buf, size));
#endif
    ZB_BUF_CHECK(buf);
    zbbuf = MK_BUF(buf);

    ZB_ASSERT(zbbuf != NULL);
    ZB_ASSERT(zb_buf_len_func(TRACE_CALL buf) >= (size));

#if defined ZB_DEBUG_BUFFERS_EXT_RAF && defined ZB_DEBUG_BUFFERS_RAF_SIZE_RECORD
    zb_uint_t size_before;
    size_before = zbbuf->hdr.len;
#endif
    zbbuf->hdr.len -= (zb_uint16_t)size;
    zbbuf->hdr.data_offset += (zb_uint16_t)size;

#if defined ZB_DEBUG_BUFFERS_EXT_RAF && defined ZB_DEBUG_BUFFERS_RAF_SIZE_RECORD
    add_buf_size_table(buf, from_file, from_line, size_before, size, zbbuf->hdr.len);
#endif

    p = zb_buf_begin_func(TRACE_CALL buf);

    TRACE_MSG(TRACE_MEMDBG1, "<zb_buf_cut_left_leg ret %p", (FMT__P, p));
    return p;
}


void *zb_buf_alloc_right_func(TRACE_PROTO zb_bufid_t buf, zb_uint_t size)
{
    zb_buf_ent_t *zbbuf;
    void *p;
    zb_uint_t right_space;
    zb_uint_t shift = 0;
    zb_uint_t max_size;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_alloc_right_func bufid %d size %d", (FMT__D_P, buf, size));
#else
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_alloc_right_func from ZB_TRACE_FILE_ID %d : %d bufid %d size %d", (FMT__D_D_D_D, from_file, from_line, buf, size));
#endif
    ZB_BUF_CHECK(buf);

    zbbuf = MK_BUF(buf);
    ZB_ASSERT(zbbuf != NULL);
    max_size = zb_bufpool_storage_calc_payload_size(zbbuf->hdr.multiplicity);

    right_space = (max_size - zbbuf->hdr.data_offset - zbbuf->hdr.len);

    ZB_ASSERT(size + zb_buf_len_func(TRACE_CALL buf) <= max_size);

    if (right_space < size)
    {
        /* not sure: try to align to 2 or 4? Or it can cause more problems? */
        zb_uint8_t *ptr = zb_buf_begin_func(TRACE_CALL buf);
        shift = size - right_space;
        TRACE_MSG(TRACE_MEMDBG3, "zb_buf_smart_alloc_right: memmove <-(%hd)", (FMT__H, (shift)));
        ZB_MEMMOVE(ptr - shift, ptr, zbbuf->hdr.len);
    }

#if defined ZB_DEBUG_BUFFERS_EXT_RAF && defined ZB_DEBUG_BUFFERS_RAF_SIZE_RECORD
    zb_uint_t size_before;
    size_before = zbbuf->hdr.len;
#endif
    zbbuf->hdr.len += (zb_uint16_t)size;
    zbbuf->hdr.data_offset -= (zb_uint16_t)shift;

#if defined ZB_DEBUG_BUFFERS_EXT_RAF && defined ZB_DEBUG_BUFFERS_RAF_SIZE_RECORD
    add_buf_size_table(buf, from_file, from_line, size_before, size, zbbuf->hdr.len);
#endif

    p = (void *)((zb_uint8_t *)zb_buf_begin_func(TRACE_CALL buf) + zbbuf->hdr.len - (zb_uint16_t)size);
    TRACE_MSG(TRACE_MEMDBG1, "<zb_buf_smart_alloc_right ret %p", (FMT__P, p));
    return p;
}


void *zb_buf_alloc_left_func(TRACE_PROTO zb_bufid_t buf, zb_uint_t size)
{
    zb_buf_ent_t *zbbuf;
    zb_uint_t max_size;
    void *p;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_alloc_left_func bufid %d size %d", (FMT__D_P, buf, size));
#else
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_alloc_left_func from ZB_TRACE_FILE_ID %d : %d bufid %d size %d", (FMT__D_D_D_D, from_file, from_line, buf, size));
#endif
    ZB_BUF_CHECK(buf);

    zbbuf = MK_BUF(buf);
    ZB_ASSERT(zbbuf != NULL);
    max_size = zb_bufpool_storage_calc_payload_size(zbbuf->hdr.multiplicity);

    ZB_ASSERT(size + zbbuf->hdr.len <= max_size);

    if (zbbuf->hdr.data_offset < size)
    {
        /* not sure: try to align to 2 or 4? Or it can cause more problems? */
        zb_uint8_t *ptr = zb_buf_begin_func(TRACE_CALL buf);
        TRACE_MSG(TRACE_MEMDBG3, "zb_buf_smart_alloc_left: memmove ->(%hd), size=%hd", (FMT__H_H, (size - zbbuf->hdr.data_offset), size));
        ZB_MEMMOVE(ptr + (size - zbbuf->hdr.data_offset), ptr, zbbuf->hdr.len);
        zbbuf->hdr.data_offset = (zb_uint16_t)size;
    }

#if defined ZB_DEBUG_BUFFERS_EXT_RAF && defined ZB_DEBUG_BUFFERS_RAF_SIZE_RECORD
    zb_uint_t size_before;
    size_before = zbbuf->hdr.len;
#endif
    zbbuf->hdr.len += (zb_uint16_t)size;
    zbbuf->hdr.data_offset -= (zb_uint16_t)size;

#if defined ZB_DEBUG_BUFFERS_EXT_RAF && defined ZB_DEBUG_BUFFERS_RAF_SIZE_RECORD
    add_buf_size_table(buf, from_file, from_line, size_before, size, zbbuf->hdr.len);
#endif

    p = zb_buf_begin_func(TRACE_CALL buf);
    TRACE_MSG(TRACE_MEMDBG1, "<zb_buf_smart_alloc_left ret %p", (FMT__P, p));
    return p;
}


static zb_uint_t compose_flags(zb_buf_ent_t *b)
{
    zb_uint_t ret = b->hdr.is_in_buf;
    ret |= (zb_uint8_t)(b->hdr.encrypt_type << 1);
    ret |= (zb_uint8_t)(b->hdr.use_same_key << 4);
    ret |= (zb_uint8_t)(b->hdr.zdo_cmd_no_resp << 5);
    ret |= (zb_uint8_t)(b->hdr.has_aps_payload << 6);
    ret |= (zb_uint8_t)(b->hdr.has_aps_user_payload << 7);
    return ret;
}


static void decompose_flags(zb_buf_ent_t *b, zb_uint_t flags)
{
    b->hdr.is_in_buf = (zb_bitfield_t)(flags & 1U);
    b->hdr.encrypt_type = (zb_bitfield_t)((flags >> 1) & 7U);
    b->hdr.use_same_key = (zb_bitfield_t)(flags >> 4);
    b->hdr.zdo_cmd_no_resp = (zb_bitfield_t)(flags >> 5);
    b->hdr.has_aps_payload = (zb_bitfield_t)(flags >> 6);
    b->hdr.has_aps_user_payload = (zb_bitfield_t)(flags >> 7);
}


void zb_buf_flags_or_func(TRACE_PROTO zb_bufid_t buf, zb_uint_t val)
{
    zb_buf_ent_t *b;
    zb_uint_t flags;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_flags_or_func bufid %d val %d", (FMT__D_P, buf, val));
#else
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_flags_or_func from ZB_TRACE_FILE_ID %d : %d bufid %d val %d", (FMT__D_D_D_D, from_file, from_line, buf, val));
#endif
    ZB_BUF_CHECK(buf);

    b = MK_BUF(buf);
    flags = compose_flags(b);
    flags |= val;
    decompose_flags(b, flags);
}


void zb_buf_flags_clr_func(TRACE_PROTO zb_bufid_t buf, zb_uint_t mask)
{
    zb_buf_ent_t *b;
    zb_uint_t flags;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_flags_clr_func bufid %d mask %d", (FMT__D_P, buf, mask));
#else
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_flags_clr_func from ZB_TRACE_FILE_ID %d : %d bufid %d mask %d", (FMT__D_D_D_D, from_file, from_line, buf, mask));
#endif
    ZB_BUF_CHECK(buf);

    b = MK_BUF(buf);
    flags = compose_flags(b);
    flags &= ~mask;
    decompose_flags(b, flags);
}


void zb_buf_flags_clr_encr_func(TRACE_PROTO zb_bufid_t buf)
{
#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_flags_clr_encr_func bufid %d", (FMT__D, buf));
#else
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_flags_clr_encr_func from ZB_TRACE_FILE_ID %d : %d bufid %d", (FMT__D_D_D, from_file, from_line, buf));
#endif
    ZB_BUF_CHECK(buf);

    zb_buf_flags_clr_func(TRACE_CALL buf, ZB_BUF_SECUR_ALL_ENCR);
}


zb_uint_t zb_buf_flags_get_func(TRACE_PROTO zb_bufid_t buf)
{
    zb_uint_t flags;
    zb_buf_ent_t *b;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_flags_get_func bufid %d", (FMT__D, buf));
#else
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_flags_get_func from ZB_TRACE_FILE_ID %d : %d bufid %d", (FMT__D_D_D, from_file, from_line, buf));
#endif
    ZB_BUF_CHECK(buf);

    b = MK_BUF(buf);
    flags = compose_flags(b);
    TRACE_MSG(TRACE_MEMDBG1, "<zb_buf_flags_get_func ret %d", (FMT__D, flags));
    return flags;
}


zb_ret_t zb_buf_get_status_func(TRACE_PROTO zb_bufid_t buf)
{
    zb_buf_ent_t *b;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_get_status_func bufid %d", (FMT__D, buf));
#else
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_get_status_func from ZB_TRACE_FILE_ID %d : %d bufid %d", (FMT__D_D_D, from_file, from_line, buf));
#endif
    ZB_BUF_CHECK(buf);

    b = MK_BUF(buf);
    TRACE_MSG(TRACE_MEMDBG1, "<zb_buf_get_status_func ret %ld", (FMT__L, b->hdr.status));
    return b->hdr.status;
}


void zb_buf_set_status_func(TRACE_PROTO zb_bufid_t buf, zb_ret_t status)
{
    zb_buf_ent_t *b;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_set_status_func bufid %d status %ld", (FMT__D_L, buf, status));
#else
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_set_status_func from ZB_TRACE_FILE_ID %d : %d bufid %d status %ld", (FMT__D_D_D_L, from_file, from_line, buf, status));
#endif
    ZB_BUF_CHECK(buf);

    b = MK_BUF(buf);
    b->hdr.status = status;
    TRACE_MSG(TRACE_MEMDBG1, "<zb_buf_set_status_func ret %ld", (FMT__L, b->hdr.status));
}


void zb_buf_set_len_and_offset_func(TRACE_PROTO zb_bufid_t buf, zb_uint16_t len, zb_uint16_t offset)
{
    zb_buf_ent_t *b;
    zb_size_t len_offset;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_set_len_and_offset_func bufid %d offset %d", (FMT__D_D, buf, offset));
#else
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_set_len_and_offset_func from ZB_TRACE_FILE_ID %d : %d bufid %d offset %d", (FMT__D_D_D_D, from_file, from_line, buf, offset));
#endif
    ZB_BUF_CHECK(buf);

    b = MK_BUF(buf);
    len_offset = (zb_size_t)len + (zb_size_t)offset;
    ZB_ASSERT(len_offset < zb_bufpool_storage_calc_payload_size(b->hdr.multiplicity));
    b->hdr.len = len;
    b->hdr.data_offset = offset;
    TRACE_MSG(TRACE_MEMDBG1, "<zb_buf_set_len_and_offset_func", (FMT__0));
}


zb_bool_t zb_buf_is_oom_state(void)
{
    return (zb_bool_t)ZB_BUF_IS_OOM_STATE();
}


#ifdef ZB_REDUCE_NWK_LOAD_ON_LOW_MEMORY

zb_bool_t zb_buf_memory_close_to_low(void)
{
    zb_uint8_t n0 = ZG->bpool.bufs_allocated[0];
    zb_uint8_t n1 = ZG->bpool.bufs_allocated[1];
    TRACE_MSG(TRACE_MEMDBG1, "buffers {%hd i-o %hd}", (FMT__H_H, n1, n0));
    return (n0 > ZB_BUFS_LIMIT - ZB_BUFS_HI_PRIOR_RESERVE - ZB_BUFS_RESERVE
            || n1 > ZB_BUFS_LIMIT - ZB_BUFS_HI_PRIOR_RESERVE - ZB_BUFS_RESERVE )
           ? ZB_TRUE : ZB_FALSE;
}
#endif


zb_bool_t zb_buf_memory_low(void)
{
    zb_uint8_t n0 = ZG->bpool.bufs_allocated[0];
    zb_uint8_t n1 = ZG->bpool.bufs_allocated[1];
    TRACE_MSG(TRACE_MEMDBG1, "buffers {%hd i-o %hd}", (FMT__H_H, n1, n0));
    return (n0 > ZB_BUFS_LIMIT - ZB_BUFS_HI_PRIOR_RESERVE
            || n1 > ZB_BUFS_LIMIT - ZB_BUFS_HI_PRIOR_RESERVE )
           ? ZB_TRUE : ZB_FALSE;
}


#if defined(ZB_TRACE_LEVEL)

void zb_buf_oom_trace(void)
{
    TRACE_MSG(TRACE_ERROR, "OOM! bufs allocated {%hd i-o %hd} max %hd",
              (FMT__H_H_H, ZG->bpool.bufs_allocated[1], ZG->bpool.bufs_allocated[0], ZB_IOBUF_POOL_SIZE));

#ifdef ZB_DEBUG_BUFFERS_EXT
    zb_trace_bufs_usage();
#endif
}

#endif


zb_uint8_t zb_buf_get_handle_func(TRACE_PROTO zb_bufid_t buf)
{
    zb_buf_ent_t *b;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_get_handle_func bufid %d", (FMT__D, buf));
#else
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_get_handle_func from ZB_TRACE_FILE_ID %d : %d bufid %d", (FMT__D_D_D, from_file, from_line, buf));
#endif
    ZB_BUF_CHECK(buf);

    b = MK_BUF(buf);
    TRACE_MSG(TRACE_MEMDBG1, "<zb_buf_get_handle_func ret %d", (FMT__D, b->hdr.handle));
    return b->hdr.handle;
}


void zb_buf_set_handle_func(TRACE_PROTO zb_bufid_t buf, zb_uint8_t handle)
{
    zb_buf_ent_t *b;

#ifndef ZB_DEBUG_BUFFERS
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_set_handle_func bufid %d handle %hd", (FMT__D_H, buf, handle));
#else
    TRACE_MSG(TRACE_MEMDBG1, ">zb_buf_set_handle_func from ZB_TRACE_FILE_ID %d : %d bufid %d handle %hd", (FMT__D_D_D_H, from_file, from_line, buf, handle));
#endif
    ZB_BUF_CHECK(buf);

    b = MK_BUF(buf);
    b->hdr.handle = handle;
}


void zb_buf_set_mac_rx_need(zb_bool_t needs)
{
    ZG->bpool.mac_rx_need_buf = needs;
}


zb_bool_t zb_buf_get_mac_rx_need(void)
{
    return (zb_bool_t)ZG->bpool.mac_rx_need_buf;
}


zb_bool_t zb_buf_have_rx_bufs(void)
{
    return (zb_bool_t)(ZG->bpool.bufs_allocated[1] < ZB_BUFS_LIMIT);
}

#ifdef ZB_BUF_SHIELD

static zb_bool_t zb_buf_check_full(TRACE_PROTO zb_bufid_t buf)
{
    zb_buf_ent_t *zbbuf;
    zb_uint_t max_size;
    zb_uint_t occupied_space;

    ZVUNUSED(from_file);
    ZVUNUSED(from_line);

    if (buf == 0
            || ((zb_uint_t)REF_UNSHIFTED(buf)) >= ZB_IOBUF_POOL_SIZE
            || !ZB_BUF_IS_BUSY(REF_UNSHIFTED(buf))
       )
    {
        TRACE_MSG(TRACE_MEMDBG3, "buf %hd has invalid number or is not busy", (FMT__H, buf));
        return ZB_FALSE;
    }

    zbbuf = MK_BUF(buf);
    if (zb_bufpool_storage_is_buf_corrupted(zbbuf))
    {
        TRACE_MSG(TRACE_MEMDBG3, "buf %hd is corrupted", (FMT__H, buf));
        return ZB_FALSE;
    }

    max_size = zb_bufpool_storage_calc_payload_size(zbbuf->hdr.multiplicity);
    occupied_space = zbbuf->hdr.data_offset + zbbuf->hdr.len;

    if (occupied_space > max_size)
    {
        TRACE_MSG(TRACE_MEMDBG3, "buf %hd has invalid data_offset and len (%d %d)",
                  (FMT__H_D_D, buf, zbbuf->hdr.data_offset, zbbuf->hdr.len));
        return ZB_FALSE;
    }

    return ZB_TRUE;
}

#endif

#ifdef ZB_DEBUG_BUFFERS_EXT

static void init_buf_usages(zb_bufid_t buf, zb_uint16_t from_file, zb_uint16_t from_line)
{
    zb_buf_usage_t *buf_usage;
    zb_uint8_t      i;
    zb_buf_ent_t   *zbbuf;

    zbbuf = MK_BUF(buf);
    zbbuf->buf_allocation.time = ZB_TIMER_GET();
    zbbuf->buf_allocation.file = from_file;
    zbbuf->buf_allocation.line = from_line;

    for (i = 0; i < ZB_DEBUG_BUFFERS_EXT_USAGES_COUNT; i++)
    {
        buf_usage = &zbbuf->buf_usages[i];
        buf_usage->line = ZB_LINE_IS_UNDEF;
    }
}

static void add_buf_usage(zb_bufid_t buf, zb_uint16_t from_file, zb_uint16_t from_line)
{
    zb_buf_ent_t   *zbbuf;
    zb_uint8_t      i;

    i = 0;
    zbbuf = MK_BUF(buf);

    for (i = 0; i < ZB_DEBUG_BUFFERS_EXT_USAGES_COUNT; i++)
    {
        if (zbbuf->buf_usages[i].line == ZB_LINE_IS_UNDEF)
        {
            break;
        }
    }

    if (i == ZB_DEBUG_BUFFERS_EXT_USAGES_COUNT)
    {
        for (i = 1; i < ZB_DEBUG_BUFFERS_EXT_USAGES_COUNT; i++)
        {
            zbbuf->buf_usages[i - 1].time = zbbuf->buf_usages[i].time;
            zbbuf->buf_usages[i - 1].file = zbbuf->buf_usages[i].file;
            zbbuf->buf_usages[i - 1].line = zbbuf->buf_usages[i].line;
        }

        i = ZB_DEBUG_BUFFERS_EXT_USAGES_COUNT - 1;
    }

    zbbuf->buf_usages[i].time = ZB_TIMER_GET();
    zbbuf->buf_usages[i].file = from_file;
    zbbuf->buf_usages[i].line = from_line;
}

#if defined(ZB_ROUTER_ROLE) && !defined(NCP_MODE_HOST)
static void zb_trace_bufs_usage_nwk()
{
    zb_nwk_broadcast_retransmit_t *brrt;
    zb_nwk_pend_t     *pend;
    zb_uint8_t                     i;

    for (i = 0; i < ZB_NWK_PENDING_TABLE_SIZE; i++)
    {
        pend = &ZB_NIB().pending_table[i];

        if (ZB_U2B(pend->used))
        {
            TRACE_MSG(TRACE_ERROR, "pend i %hd param %hd waiting_buf %hd expiry %hd",
                      (FMT__H_H_H_H, i, pend->param, pend->waiting_buf, pend->expiry));
        }
    }

    for (i = 0; i < ZB_NWK_BRR_TABLE_SIZE; i++)
    {
        brrt = &ZG->nwk.handle.brrt[i];

        if (brrt->buf != 0)
        {
            TRACE_MSG(TRACE_ERROR, "brrt i %hd buf %hd retries %hd",
                      (FMT__H_H_H, i, brrt->buf, brrt->retries_left));
        }
    }
}
#endif /* ZB_ROUTER_ROLE && !NCP_MODE_HOST */

void zb_trace_bufs_usage()
{
    zb_time_t          curr_time;
    zb_uint8_t         i;
    zb_uint8_t         j;
    zb_buf_ent_t      *buf;
    zb_buf_usage_t    *buf_usage;

    curr_time = ZB_TIMER_GET();

#if defined(ZB_ROUTER_ROLE) && !defined(NCP_MODE_HOST)
    zb_trace_bufs_usage_nwk();
#endif /* ZB_ROUTER_ROLE && !NCP_MODE_HOST */

    TRACE_MSG(TRACE_ERROR, "in_delayed_queue %hd out_delayed_queue %hd",
              (FMT__H_H, ZG->sched.delayed_queue[1].written, ZG->sched.delayed_queue[0].written));

#if defined(ZB_CHECK_OOM_STATUS) && !defined(NCP_MODE_HOST)
    TRACE_MSG(TRACE_ERROR, "oom_time     %d",  (FMT__D, ZG->nwk.oom_timestamp));
#endif /* ZB_CHECK_OOM_STATUS && !NCP_MODE_HOST  */

    TRACE_MSG(TRACE_ERROR, "reboot_time  %d",  (FMT__D, curr_time));
    TRACE_MSG(TRACE_ERROR, "scheduled_cb %d",  (FMT__D, ZB_CB_Q->written));

#ifndef NCP_MODE_HOST
    TRACE_MSG(TRACE_ERROR, "input_blc_by %hd", (FMT__H, ZG->nwk.handle.input_blocked_by));
#endif /* NCP_MODE_HOST */

    for (i = 0; i < ZB_IOBUF_POOL_SIZE; i++)
    {
        if (!ZB_BUF_IS_BUSY(i))
        {
            continue;
        }

        buf = zb_bufpool_storage_bufid_to_buf(i);
        buf_usage = &buf->buf_allocation;

        TRACE_MSG(TRACE_ERROR, "buf: number %hd is_in %hd allocation_time %d file %d line %d",
                  (FMT__H_H_D_D_D, i + NBUF_ID_SHIFT, buf->hdr.is_in_buf, buf_usage->time,
                   buf_usage->file, buf_usage->line));

        for (j = 0; j < ZB_DEBUG_BUFFERS_EXT_USAGES_COUNT; j++)
        {
            buf_usage = &buf->buf_usages[j];

            if (buf_usage->line == ZB_LINE_IS_UNDEF)
            {
                break;
            }

            TRACE_MSG(TRACE_ERROR, "buf_usage time %d file %d line %d",
                      (FMT__D_D_D, buf_usage->time, buf_usage->file, buf_usage->line));
        }
    }
}

#endif  /* ZB_DEBUG_BUFFERS_EXT */

#ifdef ZB_DEBUG_BUFFERS_EXT_RAF

static void alloc_buf_usages(zb_bufid_t buf, zb_uint16_t from_file, zb_uint16_t from_line)
{
    zb_buf_usage_t *buf_usage;
    zb_buf_usage_t *buf_usage_temp;
    zb_buf_size_t *buf_size;
    zb_buf_size_t *buf_size_temp;
    zb_buf_schedule_t *buf_schedule;
    zb_buf_schedule_t *buf_schedule_temp;
    zb_uint8_t      i;
    zb_uint8_t      j;
    zb_buf_ent_t   *zbbuf;

    zbbuf = MK_BUF(buf);
    for (i = 1; i < ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT; i++)
    {
        zbbuf->buf_allocation_table[i - 1].time = zbbuf->buf_allocation_table[i].time;
        zbbuf->buf_allocation_table[i - 1].file = zbbuf->buf_allocation_table[i].file;
        zbbuf->buf_allocation_table[i - 1].line = zbbuf->buf_allocation_table[i].line;
        zbbuf->buf_free_table[i - 1].time = zbbuf->buf_free_table[i].time;
        zbbuf->buf_free_table[i - 1].file = zbbuf->buf_free_table[i].file;
        zbbuf->buf_free_table[i - 1].line = zbbuf->buf_free_table[i].line;
    }
    zbbuf->buf_allocation_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1].time = ZB_TIMER_GET();
    zbbuf->buf_allocation_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1].file = from_file;
    zbbuf->buf_allocation_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1].line = from_line;
    zbbuf->buf_free_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1].line = ZB_LINE_IS_UNDEF;

    for (i = 1; i < ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT; i++)
    {
        for (j = 0; j < ZB_DEBUG_BUFFERS_EXT_RAF_RECORD_COUNT; j++)
        {
#ifdef ZB_DEBUG_BUFFERS_RAF_USAGE_RECORD
            buf_usage = &zbbuf->buf_usages_table[i - 1][j];
            buf_usage_temp = &zbbuf->buf_usages_table[i][j];
            buf_usage->time = buf_usage_temp->time;
            buf_usage->file = buf_usage_temp->file;
            buf_usage->line = buf_usage_temp->line;
#endif
#ifdef ZB_DEBUG_BUFFERS_RAF_SIZE_RECORD
            buf_size = &zbbuf->buf_size_table[i - 1][j];
            buf_size_temp = &zbbuf->buf_size_table[i][j];
            buf_size->time = buf_size_temp->time;
            buf_size->file = buf_size_temp->file;
            buf_size->line = buf_size_temp->line;
            buf_size->size_before = buf_size_temp->size_before;
            buf_size->size_change = buf_size_temp->size_change;
            buf_size->size_after = buf_size_temp->size_after;
#endif
#ifdef ZB_DEBUG_BUFFERS_RAF_SCHEDULE_RECORD
            buf_schedule = &zbbuf->buf_schedule_table[i - 1][j];
            buf_schedule_temp = &zbbuf->buf_schedule_table[i][j];
            buf_schedule->time = buf_schedule_temp->time;
            buf_schedule->func = buf_schedule_temp->func;
            buf_schedule->func2 = buf_schedule_temp->func2;
            buf_schedule->usage = buf_schedule_temp->usage;
            buf_schedule->used = buf_schedule_temp->used;
            buf_schedule->schedulerid = buf_schedule_temp->schedulerid;
#endif
        }

    }

    for (j = 0; j < ZB_DEBUG_BUFFERS_EXT_RAF_RECORD_COUNT; j++)
    {
#ifdef ZB_DEBUG_BUFFERS_RAF_USAGE_RECORD
        buf_usage = &zbbuf->buf_usages_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][j];
        buf_usage->line = ZB_LINE_IS_UNDEF;
#endif
#ifdef ZB_DEBUG_BUFFERS_RAF_SIZE_RECORD
        buf_size = &zbbuf->buf_size_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][j];
        buf_size->line = ZB_LINE_IS_UNDEF;
#endif
#ifdef ZB_DEBUG_BUFFERS_RAF_SCHEDULE_RECORD
        buf_schedule = &zbbuf->buf_schedule_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][j];
        buf_schedule->used = 0;
#endif
    }
}

#ifdef ZB_DEBUG_BUFFERS_RAF_USAGE_RECORD
static void add_buf_usage_table(zb_bufid_t buf, zb_uint16_t from_file, zb_uint16_t from_line)
{
    zb_buf_ent_t   *zbbuf;
    zb_uint8_t      i;

    i = 0;
    zbbuf = MK_BUF(buf);

    for (i = 0; i < ZB_DEBUG_BUFFERS_EXT_RAF_RECORD_COUNT; i++)
    {
        if (zbbuf->buf_usages_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].line == ZB_LINE_IS_UNDEF)
        {
            break;
        }
    }

    if (i == ZB_DEBUG_BUFFERS_EXT_RAF_RECORD_COUNT)
    {
        for (i = 1; i < ZB_DEBUG_BUFFERS_EXT_RAF_RECORD_COUNT; i++)
        {
            zbbuf->buf_usages_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i - 1].time = zbbuf->buf_usages_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].time;
            zbbuf->buf_usages_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i - 1].file = zbbuf->buf_usages_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].file;
            zbbuf->buf_usages_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i - 1].line = zbbuf->buf_usages_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].line;
        }
        i = ZB_DEBUG_BUFFERS_EXT_RAF_RECORD_COUNT - 1;
    }

    zbbuf->buf_usages_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].time = ZB_TIMER_GET();
    zbbuf->buf_usages_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].file = from_file;
    zbbuf->buf_usages_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].line = from_line;
}
#endif

#ifdef ZB_DEBUG_BUFFERS_RAF_SIZE_RECORD
static void add_buf_size_table(zb_bufid_t buf, zb_uint16_t from_file, zb_uint16_t from_line, zb_uint8_t size_b, zb_uint8_t size_c, zb_uint8_t size_a)
{
    zb_buf_ent_t   *zbbuf;
    zb_uint8_t      i;

    i = 0;
    zbbuf = MK_BUF(buf);

    for (i = 0; i < ZB_DEBUG_BUFFERS_EXT_RAF_RECORD_COUNT; i++)
    {
        if (zbbuf->buf_size_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].line == ZB_LINE_IS_UNDEF)
        {
            break;
        }
    }

    if (i == ZB_DEBUG_BUFFERS_EXT_RAF_RECORD_COUNT)
    {
        for (i = 1; i < ZB_DEBUG_BUFFERS_EXT_RAF_RECORD_COUNT; i++)
        {
            zbbuf->buf_size_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i - 1].time = zbbuf->buf_size_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].time;
            zbbuf->buf_size_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i - 1].file = zbbuf->buf_size_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].file;
            zbbuf->buf_size_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i - 1].line = zbbuf->buf_size_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].line;
            zbbuf->buf_size_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i - 1].size_before = zbbuf->buf_size_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].size_before;
            zbbuf->buf_size_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i - 1].size_change = zbbuf->buf_size_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].size_change;
            zbbuf->buf_size_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i - 1].size_after = zbbuf->buf_size_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].size_after;
        }
        i = ZB_DEBUG_BUFFERS_EXT_RAF_RECORD_COUNT - 1;
    }

    zbbuf->buf_size_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].time = ZB_TIMER_GET();
    zbbuf->buf_size_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].file = from_file;
    zbbuf->buf_size_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].line = from_line;
    zbbuf->buf_size_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].size_before = size_b;
    zbbuf->buf_size_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].size_change = size_c;
    zbbuf->buf_size_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].size_after = size_a;
}
#endif

static void free_buf_usages(zb_bufid_t buf, zb_uint16_t from_file, zb_uint16_t from_line)
{
    zb_buf_usage_t *buf_usage;
    zb_buf_usage_t *buf_usage_temp;
    zb_uint8_t      i;
    zb_buf_ent_t   *zbbuf;

    zbbuf = MK_BUF(buf);

    zbbuf->buf_free_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1].time = ZB_TIMER_GET();
    zbbuf->buf_free_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1].file = from_file;
    zbbuf->buf_free_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1].line = from_line;
}

void dump_buf_usage(zb_bufid_t bufid)
{
    zb_uint8_t         i;
    zb_uint8_t         j;
    zb_uint8_t         k;
    zb_uint8_t         start;
    zb_uint8_t         end;
    zb_buf_ent_t      *buf;
    zb_buf_usage_t    *buf_usage;
    zb_buf_size_t     *buf_size;
    zb_buf_schedule_t *buf_schedule;

    buf = zb_bufpool_storage_bufid_to_buf((bufid - NBUF_ID_SHIFT));
    for (k = 0; k < ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT; k++)
    {
        buf_usage = &buf->buf_allocation_table[k];
        TRACE_MSG(TRACE_RAFDBG1, "buf: number %hd round %hd",
                  (FMT__H_H, bufid, k + 1));
        if (buf_usage->time != 0 || buf_usage->file != 0)
        {
            TRACE_MSG(TRACE_RAFDBG1, "buf_allocation_time %d file %d line %d",
                      (FMT__D_D_D, buf_usage->time, buf_usage->file, buf_usage->line));

#ifdef ZB_DEBUG_BUFFERS_RAF_USAGE_RECORD
            for (j = 0; j < ZB_DEBUG_BUFFERS_EXT_RAF_RECORD_COUNT; j++)
            {
                buf_usage = &buf->buf_usages_table[k][j];

                if (buf_usage->line == ZB_LINE_IS_UNDEF)
                {
                    break;
                }

                TRACE_MSG(TRACE_RAFDBG1, "buf_usage time %d file %d line %d",
                          (FMT__D_D_D, buf_usage->time, buf_usage->file, buf_usage->line));
            }
#endif
#ifdef ZB_DEBUG_BUFFERS_RAF_SIZE_RECORD
            for (j = 0; j < ZB_DEBUG_BUFFERS_EXT_RAF_RECORD_COUNT; j++)
            {
                buf_size = &buf->buf_size_table[k][j];

                if (buf_size->line == ZB_LINE_IS_UNDEF)
                {
                    break;
                }

                TRACE_MSG(TRACE_RAFDBG1, "buf_size time %d file %d line %d, size: %d (+/-/set to) %d = %d",
                          (FMT__D_D_D_D_D_D, buf_size->time, buf_size->file, buf_size->line, buf_size->size_before, buf_size->size_change, buf_size->size_after));
            }
#endif
#ifdef ZB_DEBUG_BUFFERS_RAF_SCHEDULE_RECORD
            for (j = 0; j < ZB_DEBUG_BUFFERS_EXT_RAF_RECORD_COUNT; j++)
            {
                buf_schedule = &buf->buf_schedule_table[k][j];

                if (buf_schedule->used == 0)
                {
                    break;
                }
                TRACE_MSG(TRACE_RAFDBG1, "buf_schedule time %d usage %d func %p func2 %p schedulerid %d",
                          (FMT__D_D_D_D_D, buf_schedule->time, buf_schedule->usage, buf_schedule->func, buf_schedule->func2, buf_schedule->schedulerid));
            }
#endif
            buf_usage = &buf->buf_free_table[k];
            if (buf_usage->line != ZB_LINE_IS_UNDEF)
            {
                TRACE_MSG(TRACE_RAFDBG1, "buf_free_time %d file %d line %d",
                          (FMT__D_D_D, buf_usage->time, buf_usage->file, buf_usage->line));
            }
        }
        else
        {
            TRACE_MSG(TRACE_RAFDBG1, "no record", (FMT__0));
        }
    }
}

#endif /* ZB_DEBUG_BUFFERS_EXT_RAF */

#endif /* ifndef ZB_LEGACY_BUFS */
