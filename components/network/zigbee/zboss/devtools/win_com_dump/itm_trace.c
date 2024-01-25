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
#include "win_com_dump_common.h"
#include "itm_trace.h"
#include <stdint.h>

trace_t zb_trace;

/* todo make it argument */
uint8_t dump_swit = 0x1E; /* 1, 2, 3, 4 channels */

uint64_t timestamp = 0;

#define BUF_SIZE 512
unsigned char zb_trace_rb[BUF_SIZE];
unsigned char zb_trace_buf[BUF_SIZE];

static size_t read_varlen(serial_handle_t comf, int c, unsigned char* buf);
static void show_swit(serial_handle_t comf, int c);
static void show_timestamp(serial_handle_t comf, int c);
static uint64_t convert_ticks_to_us(uint64_t ts);

static void write_dump(trace_t * trace, unsigned char * buf, size_t len);
static void parse_pkt(trace_t * trace);
static void parse_zb_pkt(trace_t * trace);

extern win_com_dump_ctx_t wcd_ctx;


void read_itm_trace()
{
  int ret = 0;
  int eof = 0;
  int c = 0;
  bool overflow = false;

  memset(&zb_trace, 0, sizeof(trace_t));
  zb_trace.rb.buf = zb_trace_rb;
  zb_trace.rb.max = BUF_SIZE;
  zb_trace.trace_pkt.buf = zb_trace_buf;
  zb_trace.itm.max = BUF_SIZE;
  zb_trace.itm.state = WAIT_PREAMBLE_1ST;

#ifndef ZB_PLATFORM_LINUX_PC32
  setmode(fileno(stdout),O_BINARY);
  setmode(fileno(stdin),O_BINARY);
#endif

  while (!eof)
  {
    ret = port_read(wcd_ctx.comf, &c, 1, NULL);
    if (ret < 0)
    {
      eof = 1;
      break;
    }

    /* Sync packet ... 7 zeroes, 0x80 */
    if (c == 0)
    {
      int i;
      DBG_PRINT("first zero, start SYNC, c == %x\n", c);

      for (i = 0; i < 6; i++)
      {
        ret = port_read(wcd_ctx.comf, &c, 1, NULL);
        if (ret < 0)
        {
          eof = 1;
          break;
        }
        if (c != 0)
        {
          goto bad_sync;
        }
        else
        {
          DBG_PRINT("continue SYNC, c == %x\n", c);
        }
      }
      ret = port_read(wcd_ctx.comf, &c, 1, NULL);
      if (ret < 0)
      {
        eof = 1;
        break;
      }
      if (c == 0x80)
      {
        DBG_PRINT("SYNC\n");
        continue;
      }
    bad_sync:
      DBG_PRINT("BAD SYNC\n");
      continue;
    }

    /* Overflow packet */
    if (c == 0x70)
    {
      /* REVISIT later, report just what overflowed!
                    * Timestamp and SWIT can happen.  Non-ITM too?
                    */
      overflow = true;
      DBG_PRINT("OVERFLOW ...\n");
      continue;
    }
    overflow = false;

    DBG_PRINT("c = %02x\n", c);
    switch (c & 0x0f)
    {
      case 0x00: /* Timestamp */
        show_timestamp(wcd_ctx.comf, c);
        break;
      case 0x04: /* "Reserved" */
        /* FALLTHROUGH */
      case 0x08: /* ITM Extension */
        /* FALLTHROUGH */
      case 0x0c: /* DWT Extension */
        /* do nothing, the packets are skipped */
        break;
      default:
        if (!(c & 4))
        {
          show_swit(wcd_ctx.comf, c);
        }
        break;
    }

    parse_zb_pkt(&zb_trace);
  }
}

static size_t read_varlen(serial_handle_t comf, int c, unsigned char* buf)
{
    size_t size = 0;
    unsigned char value[4];

    switch (c & 3) {
      case 3:
          size = 4;
          break;
      case 2:
          size = 2;
          break;
      case 1:
          size = 1;
          break;
      default:
          return size;
    }

    memset(value, 0, sizeof(value));
    if (port_read(wcd_ctx.comf, value, size, NULL) == size)
    {
      memcpy(buf, value, size);
    }

    return size;
}

static void show_swit(serial_handle_t comf, int c)
{
    unsigned port = c >> 3;
    unsigned char buf[4];
    size_t size = 0;

    DBG_PRINT("----------PORT %d\n", port);

    if (dump_swit & (1 << port))
    {
      size = read_varlen(wcd_ctx.comf, c, buf);
      if (size && port == 1)
      {
        write_dump(&zb_trace, buf, size);
      }

      return;
    }

    /* skip unexpected data type */
    read_varlen(wcd_ctx.comf, c, buf);

    return;
}

static void show_timestamp(serial_handle_t comf, int c)
{
    unsigned counter = 0;
    char* label = "";
    bool delayed = false;
    int ret = 0;

    /*if (dump_swit)
        return;*/
    DBG_PRINT("TIMESTAMP - ");

    /* Format 2: header only */
    if (!(c & 0x80)) {
        switch (c) {
        case 0:    /* sync packet -- coding error! */
        case 0x70: /* overflow -- ditto! */
            DBG_PRINT("ERROR - %#02x\n", c);
            break;
        default:
            /* synchronous to ITM */
            counter = c >> 4;
            goto done;
        }
        return;
    }

    /* Format 1:  one to four bytes of data too */
    switch (c >> 4) {
    default:
        label = ", reserved control\n";
        break;
    case 0xc:
        /* synchronous to ITM */
        break;
    case 0xd:
        label = ", timestamp delayed";
        delayed = true;
        break;
    case 0xe:
        label = ", packet delayed";
        delayed = true;
        break;
    case 0xf:
        label = ", packet and timetamp delayed";
        delayed = true;
        break;
    }

    ret = port_read(wcd_ctx.comf, &c, 1, NULL);
    if (ret < 0)
    {
      goto err;
    }
    counter = c & 0x7f;
    if (!(c & 0x80))
        goto done;

    ret = port_read(wcd_ctx.comf, &c, 1, NULL);
    if (ret < 0)
    {
      goto err;
    }
    counter |= (c & 0x7f) << 7;
    if (!(c & 0x80))
        goto done;

    ret = port_read(wcd_ctx.comf, &c, 1, NULL);
    if (ret < 0)
    {
      goto err;
    }
    counter |= (c & 0x7f) << 14;
    if (!(c & 0x80))
        goto done;

    ret = port_read(wcd_ctx.comf, &c, 1, NULL);
    if (ret < 0)
    {
      goto err;
    }
    counter |= (c & 0x7f) << 21;

done:
    /* REVISIT should we try to convert from delta values?  */
    DBG_PRINT("+%u%s\n", counter, label);
    timestamp += counter;
    return;

err:
    DBG_PRINT("(ERROR %d - %s) ", errno, strerror(errno));
    goto done;
}

static uint64_t convert_ticks_to_us(uint64_t ts)
{
  uint64_t us = ts/TICKS_PER_US;
}

static void write_dump(trace_t * trace, unsigned char * buf, size_t len)
{
  DBG_PRINT("trace->rb.max = %d\n", trace->rb.max);
  DBG_PRINT("trace->rb.cnt = %d\n", trace->rb.cnt);
  DBG_PRINT("len = %d\n", len);
  if (trace->rb.cnt + len < trace->rb.max)
  {
    for(int i = 0; i < len; i++)
    {
      DBG_PRINT("trace->rb.wr = %d\n", trace->rb.wr);
      trace->rb.buf[trace->rb.wr] = buf[i];
      trace->rb.wr = (trace->rb.wr + 1) % trace->rb.max;
      trace->rb.cnt++;
    }
  }
  else
  {
    DBG_PRINT("Dump overflow %d\n", trace->rb.max);
  }
}

static void parse_zb_pkt(trace_t * trace)
{
  ring_buf_t *rb = &trace->rb;
  itm_ctx_t *itm = &trace->itm;
  trace_pkt_t *trace_pkt = &trace->trace_pkt;

  while(rb->cnt)
  {
    DBG_PRINT("%02x\n", rb->buf[rb->rd]);
    switch(itm->state)
    {
      case WAIT_PREAMBLE_1ST:
        DBG_PRINT("WAIT_PREAMBLE_1ST \n");
        if (ZB_PREAMBLE_1ST == rb->buf[rb->rd])
        {
          itm->state = WAIT_PREAMBLE_2ND;
        }
        break;
      case WAIT_PREAMBLE_2ND:
        DBG_PRINT("WAIT_PREAMBLE_2ND \n");
        if (ZB_PREAMBLE_2ND == rb->buf[rb->rd])
        {
          itm->state = WAIT_LEN_1ST;
        }
        else
        {
          itm->state = WAIT_PREAMBLE_1ST;
        }
        break;
      case WAIT_LEN_1ST:
        DBG_PRINT("WAIT_LEN_1ST \n");
        itm->state = WAIT_TYPE;
        itm->body_size = rb->buf[rb->rd];
        DBG_PRINT("itm->body_size = %d\n", itm->body_size);
        break;
      case WAIT_TYPE:
        DBG_PRINT("WAIT_TYPE \n");
        if (ZB_TRACE_TYPE == rb->buf[rb->rd])
        {
          itm->state = WAIT_BODY;
          itm->body_size -= 2; /* len and type */

          trace_pkt->size = itm->body_size;
          trace_pkt->offset = 0;
        }
        else
        {
          itm->state = WAIT_PREAMBLE_1ST;
        }
        break;
      case WAIT_BODY:
        DBG_PRINT("WAIT_BODY \n");
        trace_pkt->buf[trace_pkt->offset] = rb->buf[rb->rd];
        trace_pkt->offset++;

        itm->body_size--;
        if (0 == itm->body_size)
        {
          DBG_PRINT("parse_trace_line() size %d\n", trace_pkt->size);
          parse_trace_line((char*)trace_pkt->buf, trace_pkt->size, wcd_ctx.tracef);
          itm->state = WAIT_PREAMBLE_1ST;
        }
        break;
      default:
        DBG_PRINT("parse_zb_pkt: unknown state %d\n", itm->state);
        break;
    }

    rb->rd = (rb->rd + 1) % rb->max;
    rb->cnt--;
  }
}
