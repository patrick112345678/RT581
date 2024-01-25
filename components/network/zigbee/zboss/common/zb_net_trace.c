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
/* PURPOSE: trace into RAM area, nvrame and network
*/

#define ZB_TRACE_FILE_ID 3220

#include "zb_common.h"

#ifdef ZB_NET_TRACE
#include "zb_net_trace.h"
#include "zb_mac_transport.h"

#ifdef ZB_USE_MEMORY_PROTECTION
#include "zb_memory_protection.h"
#endif

/*! @cond DOXYGEN_DEBUG_SECTION */
/*! \addtogroup ZB_TRACE */
/*! @{ */

#define INTEGRITY_MAGIC_WORD_VALUE 0xFEEDBEEF

/* The idea of this ring buffer structure is to exclude the possibility
   that a whole trace record could not be accessed as a one continuous block.
*/
typedef struct zb_nettrace_rb_s
{
  zb_uint32_t integrity_magic_word_head;
  zb_uint8_t ring_buf[ZB_MEMTRACE_BUF_SIZE];
  zb_uindex_t read_i;
  zb_uindex_t write_i;
  zb_uindex_t border_i;
  zb_size_t written;
  zb_size_t committed;
  zb_size_t reserved;
  zb_uint32_t integrity_magic_word_tail;
} zb_nettrace_rb_t;

#ifdef ZB_IAR
#error "not implemented for IAR!"
#else
zb_nettrace_rb_t s_nettrace_rb __attribute__ ((section(".net_trace_buffers")));
#endif

static void zb_nettrace_assert(zb_int_t line_number);

#define ZB_NETTRACE_ASSERT(cond) if (!(cond)) zb_nettrace_assert(__LINE__);

static zb_nettrace_ctx_t s_nettrace_ctx;

#define NET_TRACE_INITED()             (s_nettrace_ctx.inited == ZB_TRUE)

static void zb_nettrace_startup_done(void);

static zb_size_t zb_nettrace_rb_get_next_trace_record_size(void)
{
  zb_nettrace_rb_t *rb = &s_nettrace_rb;
  zb_uint8_t *trace_record;

  ZB_NETTRACE_ASSERT(rb->committed);

  trace_record = ZB_RING_BUFFER_GET(rb);

  ZB_NETTRACE_ASSERT(trace_record[0] == 0xde);
  ZB_NETTRACE_ASSERT(trace_record[1] == 0xad);

  /* trace record has header containing length, the first two bytes are signature,
     see zb_mac_transport_hdr_s in zb_mac_transport.h */
  return trace_record[2] + 2;
}

static void zb_nettrace_rb_flush(zb_size_t size)
{
  zb_nettrace_rb_t *rb = &s_nettrace_rb;

  /* we should not flush uncommitted data
     because uncommitted data is a current trace record not fully written yet */
  ZB_NETTRACE_ASSERT(size <= rb->committed && size != 0);

  ZB_RING_BUFFER_FLUSH_GET_BATCH(rb, size);
  rb->committed -= size;

  ZB_NETTRACE_ASSERT(rb->read_i <= rb->border_i);

  if (rb->read_i == rb->border_i)
  {
    rb->read_i = 0;
    rb->border_i = ZB_RING_BUFFER_CAPACITY(rb);
  }
}

static void zb_nettrace_rb_reserve(zb_size_t size)
{
  zb_nettrace_rb_t *rb = &s_nettrace_rb;
  zb_size_t rb_capacity = ZB_RING_BUFFER_CAPACITY(rb);
  zb_bool_t space_found = ZB_FALSE;

  ZB_NETTRACE_ASSERT(size <= rb_capacity);

  ZB_NETTRACE_ASSERT(!s_nettrace_rb.reserved);
  s_nettrace_rb.reserved = size;

  while(!space_found)
  {
    zb_size_t free_space = 0;

    ZB_NETTRACE_ASSERT(rb->read_i < rb->border_i);

    if (rb->read_i == rb->write_i)
    {
      /* if the ring buffer is empty */
      if (!rb->committed)
      {
        ZB_NETTRACE_ASSERT(rb->border_i == rb_capacity);
        ZB_NETTRACE_ASSERT(rb->written == 0);
        rb->read_i = rb->write_i = 0;
        space_found = ZB_TRUE;
      }
      else
      {
        /* if we can't reserve place for a new trace line, we should flush the oldest one from the buffer */
        zb_nettrace_rb_flush(zb_nettrace_rb_get_next_trace_record_size());
      }
    }
    else if (rb->read_i < rb->write_i)
    {
      ZB_NETTRACE_ASSERT(rb->border_i == rb_capacity);

      free_space = rb_capacity - rb->write_i;
      if (free_space >= size)
      {
        space_found = ZB_TRUE;
      }
      else
      {
        rb->border_i = rb->write_i;
        rb->write_i = 0;
      }
    }
    else
    {
      /* if we are here then read_i > write_i */
      free_space = rb->read_i - rb->write_i;

      if (free_space >= size)
      {
        space_found = ZB_TRUE;
      }
      else
      {
        /* if we can't reserve place for a new trace line, we should flush the oldest one from the buffer */
        zb_nettrace_rb_flush(zb_nettrace_rb_get_next_trace_record_size());
      }
    }
  }
}

static void zb_nettrace_rb_put_bytes(zb_uint8_t *buf, zb_size_t size)
{
  zb_size_t written = 0;

  ZB_NETTRACE_ASSERT(s_nettrace_rb.reserved >= size);
  s_nettrace_rb.reserved -= size;

  ZB_RING_BUFFER_BATCH_PUT(&s_nettrace_rb, buf, size, written);
  ZB_NETTRACE_ASSERT(written == size);
}

static void zb_nettrace_rb_commit(void)
{
  /* all reserved bytes should be used by now */
  ZB_NETTRACE_ASSERT(!s_nettrace_rb.reserved);

  s_nettrace_rb.committed = s_nettrace_rb.written;
}

/**
 * Calculates the size of the continuous block of whole trace records not exceeding the specified max size.
 *
 * @param max_size - max allowed block size
 *
 * @return the size of the continuous block of whole trace records */
static zb_size_t zb_nettrace_rb_available_continuous_block(zb_size_t max_size)
{
  zb_nettrace_rb_t *rb = &s_nettrace_rb;
  zb_size_t ret_size = 0;
  zb_size_t curr_record_size = 0;
  zb_uindex_t border_i = 0;

  zb_uindex_t curr_i = rb->read_i;
  zb_size_t max_block_size = 0;

  ZB_NETTRACE_ASSERT(rb->read_i < rb->border_i);

  if (rb->committed)
  {
    if (rb->read_i < rb->write_i)
    {
      max_block_size = rb->committed;
      border_i = rb->read_i + rb->committed;
      ZB_NETTRACE_ASSERT(max_block_size == rb->write_i - rb->read_i);
    }
    else
    {
      max_block_size = rb->border_i - rb->read_i;
      border_i = rb->border_i;
    }
    if (max_block_size < max_size)
    {
      ret_size = max_block_size;
    }
    else
    {
      while (ZB_TRUE)
      {
        curr_record_size = 0;
        curr_record_size += 2; /* signature size */
        ZB_NETTRACE_ASSERT(rb->ring_buf[curr_i] == 0xde);
        ZB_NETTRACE_ASSERT(rb->ring_buf[curr_i + 1] == 0xad);
        curr_record_size += rb->ring_buf[curr_i + 2]; /* length from zb_mac_transport_hdr_t */
        curr_i += curr_record_size;
        if (curr_i >= border_i || ret_size + curr_record_size > max_size)
        {
          break;
        }
        ret_size += curr_record_size;
      }
    }
  }
  return ret_size;
}

static zb_bool_t zb_nettrace_check_rb_integrity()
{
  zb_bool_t ret = ZB_FALSE;
  zb_uint32_t capacity;

  capacity = ZB_RING_BUFFER_CAPACITY(&s_nettrace_rb);

  if (s_nettrace_rb.integrity_magic_word_head == INTEGRITY_MAGIC_WORD_VALUE
      && s_nettrace_rb.integrity_magic_word_tail == INTEGRITY_MAGIC_WORD_VALUE
      && s_nettrace_rb.read_i < capacity
      && s_nettrace_rb.write_i < capacity
      && s_nettrace_rb.written <= capacity
      && s_nettrace_rb.border_i <= capacity
      && s_nettrace_rb.committed <= capacity
      && s_nettrace_rb.reserved <= capacity)
  {
    ret = ZB_TRUE;
  }

  return ret;
}

static void zb_nettrace_rb_reset(void)
{
  ZB_BZERO(&s_nettrace_rb, sizeof(s_nettrace_rb));
  s_nettrace_rb.integrity_magic_word_head = INTEGRITY_MAGIC_WORD_VALUE;
  s_nettrace_rb.integrity_magic_word_tail = INTEGRITY_MAGIC_WORD_VALUE;
  s_nettrace_rb.border_i = ZB_RING_BUFFER_CAPACITY(&s_nettrace_rb);
  }

static void zb_nettrace_rb_init(void)
  {
  if (!zb_nettrace_check_rb_integrity())
  {
    zb_nettrace_rb_reset();
  }
  else
  {
    zb_nettrace_rb_t *rb = &s_nettrace_rb;
    if (!rb->reserved)
    {
      rb->committed = rb->written;
    }
    else
    {
      zb_size_t size_to_clear = rb->written - rb->committed;

      rb->reserved = 0;
      rb->written -= size_to_clear;
      if (size_to_clear > rb->write_i)
    {
        rb->write_i = ZB_RING_BUFFER_CAPACITY(rb) - (size_to_clear - rb->write_i);
    }
      else
      {
        rb->write_i -= size_to_clear;
  }
    }
  }
}


static zb_uint_t zb_nettrace_rb_get_used_space()
{
  return ZB_RING_BUFFER_USED_SPACE(&s_nettrace_rb);
}


void zb_nettrace_init(zb_nettrace_mode_t trace_mode)
{
#ifdef ZB_USE_MEMORY_PROTECTION
  zb_mpu_disable();
#endif

  ZB_BZERO(&s_nettrace_ctx, sizeof(s_nettrace_ctx));

  zb_nettrace_rb_init();

    s_nettrace_ctx.trace_mode = trace_mode;

  /* Finish initializing Net trace subsystem */
  s_nettrace_ctx.inited = ZB_TRUE;

#ifdef ZB_USE_MEMORY_PROTECTION
  zb_mpu_enable();
#endif
}

/**
   Start net trace, optionally put trace to net.

   Must be called after network init done.

   @param trace_mode - trace mode @ref zb_nettrace_mode_t
   @param put2net_start_cb - callback to be used to start writing trace to net
   at runtime
   @param put2net_atboot_func - callback to be used to write trace to net just
   after boot, when applications, like gw, are not running yet
 */
zb_ret_t zb_nettrace_startup(
  zb_callback_t put2net_start_cb,
  zb_callback_t put2net_atboot_func,
  zb_callback_t nettrace_startup_done)
{
  zb_ret_t ret = RET_ERROR;

  /* init s_nettrace_rb */
#ifdef ZB_USE_MEMORY_PROTECTION
  zb_mpu_disable();
#endif

  s_nettrace_ctx.put2net_start_cb = put2net_start_cb;
  s_nettrace_ctx.nettrace_startup_done = nettrace_startup_done;
  if (zb_nettrace_rb_get_used_space() && put2net_atboot_func)
  {
    s_nettrace_ctx.atboot_tracing_active = ZB_TRUE;
    zb_nettrace_start();

    put2net_atboot_func(0);

    ret = RET_BLOCKED;
  }
  else
  {
    zb_nettrace_startup_done();
    ret = RET_OK;
  }

#ifdef ZB_USE_MEMORY_PROTECTION
  zb_mpu_enable();
#endif

  return ret;
}


static void zb_nettrace_startup_done(void)
{
  s_nettrace_ctx.atboot_tracing_active = ZB_FALSE;
  if (s_nettrace_ctx.nettrace_startup_done)
  {
    s_nettrace_ctx.nettrace_startup_done(0);
}
}


/**
   Switch mode of trace to net

   @param arg - trace mode @ref zb_nettrace_mode_t
 */
void zb_nettrace_switch(zb_uint8_t arg)
{
  s_nettrace_ctx.trace_mode = (zb_nettrace_mode_t)arg;
}

void zb_nettrace_batch_start(zb_uint16_t batch_size)
{
#ifdef ZB_USE_MEMORY_PROTECTION
  zb_mpu_disable();
#endif

  do
  {
    if (!NET_TRACE_INITED())
    {
      break;
      }

    if (s_nettrace_ctx.trace_mode == ZB_NETTRACE_OFF)
    {
      break;
}

    zb_nettrace_rb_reserve(batch_size);

    }
  while (0);

#ifdef ZB_USE_MEMORY_PROTECTION
  zb_mpu_enable();
#endif
}

/*
   Routine to be used instead of trace over serial
 */
void zb_nettrace_put_bytes(zb_uint8_t *buf, zb_short_t len)
{
  /* Put to ring buffer.
   */
#ifdef ZB_USE_MEMORY_PROTECTION
  zb_mpu_disable();
#endif

  do
  {
    if (!NET_TRACE_INITED())
    {
      break;
}

    if (s_nettrace_ctx.trace_mode == ZB_NETTRACE_OFF)
{
      break;
    }

    zb_nettrace_rb_put_bytes(buf, len);
  }
  while (0);

#ifdef ZB_USE_MEMORY_PROTECTION
  zb_mpu_enable();
#endif
}

void zb_nettrace_batch_commit()
{
  if (NET_TRACE_INITED())
  {
    zb_nettrace_rb_commit();

    if (s_nettrace_ctx.trace_mode == ZB_NETTRACE_NET
        && !s_nettrace_ctx.net_put_is_running
        && s_nettrace_ctx.put2net_start_cb)
    {
      zb_nettrace_start();
      ZB_SCHEDULE_CALLBACK(s_nettrace_ctx.put2net_start_cb, 0);
      }
    }
  }


/**
   Get data portion from the net trace buffer.

   To be used by mem-net trace transfer callback
 */
zb_uint_t zb_nettrace_get_next_data(zb_uint8_t **buf_p, zb_size_t buf_len)
{
  zb_nettrace_rb_t *rb = &s_nettrace_rb;
  zb_uint_t size = 0;
  *buf_p = ZB_RING_BUFFER_GET(rb);

  if (s_nettrace_ctx.trace_mode == ZB_NETTRACE_NET)
  {
    size = zb_nettrace_rb_available_continuous_block(buf_len);

    if (s_nettrace_ctx.atboot_tracing_active && !size)
{
      zb_nettrace_startup_done();
}
  }

  return size;
}

void zb_nettrace_flush(zb_size_t size)
{
  zb_nettrace_rb_flush(size);
}


/**
   Clean trace buffer

 */
void zb_nettrace_clean()
{
#ifdef ZB_USE_MEMORY_PROTECTION
  zb_mpu_disable();
#endif

  zb_nettrace_rb_reset();

#ifdef ZB_USE_MEMORY_PROTECTION
  zb_mpu_enable();
#endif
}


/**
   Set trace mode

   @param trace_mode - trace mode @ref zb_nettrace_mode_t
 */
void zb_nettrace_set_mode(zb_nettrace_mode_t mode)
{
  s_nettrace_ctx.trace_mode = mode;
}


/**
   Stop trace running

 */
void zb_nettrace_stop()
{
  s_nettrace_ctx.net_put_is_running = ZB_FALSE;
}


/**
   Start trace running

 */
void zb_nettrace_start()
{
  s_nettrace_ctx.net_put_is_running = ZB_TRUE;
}


/**
  Get current net trace mode

  @return current trace mode
 */
zb_nettrace_mode_t zb_nettrace_get_mode()
{
  return s_nettrace_ctx.trace_mode;
}


int zb_nettrace_is_blocked_for(zb_uint16_t file_id)
{
  ZVUNUSED(file_id);
  /* not too funny solution: hard-coded file ids. See ZB_TRACE_FILE_ID from .c
   * files
   */
  return 0;
}

static void zb_nettrace_assert(zb_int_t line_number)
{
  /* try to restore the nettrace buffer and send assert info */
  zb_nettrace_rb_reset();
  TRACE_MSG(TRACE_ERROR, "ZB_NETTRACE_ASSERT, zb_nettrace.c:%d", (FMT__D, line_number));
  ZB_ASSERT(0);
}


/*! @} */
/*! @endcond */ /* DOXYGEN_DEBUG_SECTION */

#endif  /* ZB_NET_TRACE */
