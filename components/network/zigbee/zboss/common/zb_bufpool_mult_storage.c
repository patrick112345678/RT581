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

#define ZB_TRACE_FILE_ID 23
#include "zb_common.h"
#include "zb_bufpool.h"

#ifndef ZB_LEGACY_BUFS

void zb_bufpool_storage_init(void)
{
    ZB_BZERO(ZG->bpool.busy_bufs_bitmap, ZB_BUF_POOL_BITMAP_SIZE);
}

zb_buf_ent_t *zb_bufpool_storage_allocate(zb_uint8_t buf_cnt)
{
    zb_uint8_t i;
    zb_uint_t seq_free_cnt = 0;
    zb_bufid_t buf_idx;
    zb_buf_ent_t *buf = NULL;

    TRACE_MSG(TRACE_MEMDBG3, "zb_bufpool_storage_allocate buf_cnt=%hd", (FMT__H, buf_cnt));

    /* TODO: this might be a little bit accelerated by skipping 0xFF bytes */
    /* 07/12/2019 EE CR:MINOR sure, skip ff */
    for (i = 0; i < ZB_IOBUF_POOL_SIZE; i++)
    {
        if (!ZB_CHECK_BIT_IN_BIT_VECTOR(ZG->bpool.busy_bufs_bitmap, i))
        {
            seq_free_cnt++;
        }
        else
        {
            seq_free_cnt = 0;
        }

        if (seq_free_cnt == buf_cnt)
        {
            buf_idx = i - buf_cnt + 1U;
            buf = &ZG->bpool.pool[buf_idx];

            TRACE_MSG(TRACE_MEMDBG3, "found a starting location for %hd buffers: idx=%hd",
                      (FMT__H_H, buf_cnt, buf_idx));
            break;
        }
    }

    if (buf != NULL)
    {
        for (i = 0; i < buf_cnt; i++)
        {
            ZB_SET_BIT_IN_BIT_VECTOR(ZG->bpool.busy_bufs_bitmap, (zb_uint_t)buf_idx + i);
        }

        /* filling buf hdr & other things */
        ZB_BZERO(&(buf->hdr), sizeof(zb_buf_hdr_t));

        buf->hdr.multiplicity = buf_cnt;

#ifdef ZB_BUF_SHIELD
        //buf->hdr_signature = ZB_BUF_HDR_SIGNATURE;     //GP add, remove
        //ZG->bpool.pool[buf_idx + buf_cnt - 1].tail_signature = ZB_BUF_TAIL_SIGNATURE;      //GP add, remove
#endif
    }
    else
    {
        TRACE_MSG(TRACE_MEMDBG3, "there is no contiguous space of size %hd", (FMT__H, buf_cnt));
    }

    return buf;
}

void zb_bufpool_storage_free(zb_buf_ent_t *buf)
{
    zb_uint_t i;
    zb_bufid_t buf_id = zb_bufpool_storage_buf_to_bufid(buf);

    TRACE_MSG(TRACE_MEMDBG3, "freed buffers [%hd:%hd]",
              (FMT__H_H, buf_id, buf_id + buf->hdr.multiplicity - 1));

    for (i = 0; i < buf->hdr.multiplicity; i++)
    {
        ZB_ASSERT((i + buf_id) < ZB_IOBUF_POOL_SIZE);
        ZB_ASSERT(ZB_CHECK_BIT_IN_BIT_VECTOR(ZG->bpool.busy_bufs_bitmap, buf_id + i));

        ZB_CLR_BIT_IN_BIT_VECTOR(ZG->bpool.busy_bufs_bitmap, buf_id + i);
    }
}

zb_bufid_t zb_bufpool_storage_buf_to_bufid(zb_buf_ent_t *buf)
{
    if (buf < &ZG->bpool.pool[0] + ZB_IOBUF_POOL_SIZE)
    {
        /*cstat !MISRAC2012-Rule-18.2 */
        /* C-STAT generates false positive here, complaining about subtraction between two pointers
         * addressing different buffers. Actually, ZG->bpool.pool size is ZB_IOBUF_POOL_SIZE and
         * there is a check that 'buf' is in the range of that array. */
        return (zb_bufid_t)(buf - &ZG->bpool.pool[0]);
    }
    else
    {
        ZB_ASSERT(0);
        /* Caller is not checking returned value, let's return some valid value to not trigger MISRA
         * violations for possible out of bounds array access by the caller. */
        return 0;
    }
}

zb_buf_ent_t *zb_bufpool_storage_bufid_to_buf(zb_bufid_t buf_id)
{
    return &ZG->bpool.pool[buf_id];
}

zb_uint8_t zb_bufpool_storage_calc_multiplicity(zb_uint16_t payload_size)
{
    if (payload_size <= ZB_IO_BUF_SIZE)
    {
        return 1U;
    }
    else
    {
        /* We may use up to ZB_IO_BUF_SIZE bytes from the first (head) buffer and each subsequent
           buffer can be completely replaced with payload data.
        */
        zb_size_t ret = 1U + ((zb_size_t)payload_size - ZB_IO_BUF_SIZE + sizeof(zb_mult_buf_t) - 1U) / sizeof(zb_mult_buf_t);
        return (zb_uint8_t)ret;
    }
}

zb_size_t zb_bufpool_storage_calc_payload_size(zb_uint8_t buf_cnt)
{
    if (buf_cnt == 0U)
    {
        return 0;
    }
    else
    {
        return ZB_IO_BUF_SIZE + ((zb_size_t)buf_cnt - 1U) * sizeof(zb_mult_buf_t);
    }
}

#ifdef ZB_BUF_SHIELD

zb_bool_t zb_bufpool_storage_is_buf_corrupted(zb_buf_ent_t *buf)
{
    zb_uint_t last_buf;
    zb_bufid_t bufid = zb_bufpool_storage_buf_to_bufid(buf);

    if (buf->hdr.multiplicity == 0)
    {
        TRACE_MSG(TRACE_MEMDBG3, "buf #%hd (w/o offset) has zero multiplicity", (FMT__H, bufid));
        return ZB_TRUE;
    }

#if 0     //GP add, remove
    if (buf->hdr_signature != ZB_BUF_HDR_SIGNATURE)
    {
        TRACE_MSG(TRACE_MEMDBG3, "buf #%hd (w/o offset) has invalid hdr signature", (FMT__H, bufid));
        return ZB_TRUE;
    }
#endif

    last_buf = bufid + buf->hdr.multiplicity - 1;
    if (last_buf >= ZB_IOBUF_POOL_SIZE)
    {
        TRACE_MSG(TRACE_MEMDBG3, "buf #%hd (w/o offset) has invalid multiplicity", (FMT__H, bufid));
        return ZB_TRUE;
    }

#if 0       //GP add, remove
    if (ZG->bpool.pool[last_buf].tail_signature != ZB_BUF_TAIL_SIGNATURE)
    {
        TRACE_MSG(TRACE_MEMDBG3, "buf #%hd (w/o offset) has tail signature", (FMT__H, bufid));
        return ZB_TRUE;
    }
#endif

    return ZB_FALSE;
}

#endif

#endif /* #ifndef ZB_LEGACY_BUFS */
