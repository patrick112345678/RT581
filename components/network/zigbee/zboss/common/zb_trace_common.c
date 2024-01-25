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
/* PURPOSE: internal definitions for ZigBee trace.
*/

#define ZB_TRACE_FILE_ID 1593

#include "zb_common.h"
#ifndef ZB_ALIEN_MAC
#  include "zb_mac_transport.h"
#endif
#include "zb_trace_common.h"
#ifdef ZB_NET_TRACE
#  include "zb_net_trace.h"
#endif

#if defined(ZB_TRACE_LEVEL) || defined(ZB_TRAFFIC_DUMP_ON)

#if defined(ZB_TRACE_LEVEL)
zb_uint8_t g_trace_level = ZB_TRACE_LEVEL;
zb_uint8_t g_o_trace_level = ZB_TRACE_LEVEL;
zb_uint32_t g_trace_mask = ZB_TRACE_MASK;
#endif
zb_uint_t g_trace_inside_intr;

#ifdef ZB_TRAFFIC_DUMP_ON
zb_uint8_t g_traf_dump = 0;     /* Disabled by default */
#if !defined ZB_BINARY_TRACE && defined ZB_TRACE_TO_FILE
static zb_osif_file_t *s_dump_file;
#endif
#endif /* ZB_TRAFFIC_DUMP_ON */

#if defined (ZB_TRACE_TO_FILE)
zb_osif_file_t *s_trace_file;
static zb_bool_t trace_inited = 0;

#ifdef __ANDROID__
zb_char_t g_tag[128];
#endif

#if defined ZB_BINARY_AND_TEXT_TRACE_MODE
zb_uint8_t g_trace_text_mode = ZB_TRACE_MODE_BINARY;
#endif
#endif  /* ZB_TRACE_TO_FILE */

/**
 * Global trace file for Unix trace implementation
 */
static zb_uint32_t trace_msg_counter = 0;
#ifdef ZB_TRACE_TO_PORT
ZB_WEAK_PRE void ZB_WEAK zb_trace_msg_port_do(void);
#endif
zb_uint32_t zb_trace_get_counter()
{
  return trace_msg_counter;
}

void zb_trace_inc_counter()
{
  trace_msg_counter++;
}

#ifndef ZB_ALWAYS_ENABLED_INFO_LEVEL
/* Print only ERROR messages by default */
#define ZB_ALWAYS_ENABLED_INFO_LEVEL 0
#endif

#define ZB_TRACE_MAX(a, b) ((a) > (b) ? (a) : (b))

#if defined(ZB_TRACE_LEVEL)
zb_bool_t zb_trace_check(zb_uint_t level, zb_uint_t mask)
{
  if (mask == TRACE_SUBSYSTEM_INFO &&
      level <= ZB_TRACE_MAX(g_trace_level, ZB_ALWAYS_ENABLED_INFO_LEVEL))
  {
    return ZB_TRUE;
  }

  return g_trace_level >= (zb_int_t)level && (mask & g_trace_mask);
}
#endif

#if defined (ZB_TRACE_TO_FILE)
#if defined ZB_BINARY_AND_TEXT_TRACE_MODE
void zb_trace_set_mode(zb_uint8_t mode)
{
  g_trace_text_mode = mode;
}
#endif

void zb_trace_init_file(zb_char_t *name)
{
  zb_osif_file_t *f;

  if (trace_inited)
  {
    TRACE_MSG(TRACE_COMMON3, "trace is already inited", (FMT__0));
    return;
  }

#ifdef __ANDROID__
  strncpy(g_tag, name, sizeof(g_tag));
#endif

  if ((f = zb_osif_file_open(ZB_TRACE_SWITCH_OFF_FILE_NAME, "r")) != NULL)
  {
    zb_osif_file_close(f);
  }
  else
  {
#ifndef ZB_TRACE_TO_SYSLOG
    if (g_trace_level != 0)
    {
      s_trace_file = zb_osif_init_trace(name);
#ifndef ZB_BINARY_TRACE
      if (!s_trace_file)
      {
        s_trace_file = zb_osif_file_stdout();
      }
#endif
    }
#ifdef ZB_TRAFFIC_DUMP_ON
    if (g_traf_dump != 0)
    {
      s_dump_file = zb_osif_init_dump(name);
    }
#endif /* ZB_TRAFFIC_DUMP_ON */
#else /* ZB_TRACE_TO_SYSLOG */
    s_trace_file = NULL;
#endif /* ZB_TRACE_TO_SYSLOG */
  }
  trace_inited = 1;
}

void zb_trace_deinit_file()
{
  if (s_trace_file && s_trace_file != zb_osif_file_stdout())
  {
    zb_osif_file_close(s_trace_file);
    s_trace_file = 0;
  }
#ifdef ZB_TRAFFIC_DUMP_ON
  if (s_dump_file)
  {
    zb_osif_file_close(s_dump_file);
    s_dump_file = 0;
  }
#endif
  trace_inited = 0;
}

void zb_file_trace_commit(void)
{
  zb_osif_file_sync(s_trace_file);
}

#ifdef ZB_TRAFFIC_DUMP_ON
zb_osif_file_t *zb_trace_dump_file()
{
#ifdef ZB_BINARY_TRACE
  return s_trace_file;
#else
  return s_dump_file;
#endif
}
#endif  /* ZB_TRAFFIC_DUMP_ON */
#endif  /* ZB_TRACE_TO_FILE */

#ifdef ZB_TRAFFIC_DUMP_ON
static void zb_dump_put_bytes(zb_uint8_t *buf, zb_short_t len)
{
#ifdef ZB_BINARY_TRACE
  zb_trace_put_bytes((zb_uint16_t)(~0U), buf, len);
#else
  (void)zb_osif_file_write(s_dump_file, buf, len);
#endif
}

#if !defined(ZB_TOOL) && !defined(NCP_MODE_HOST)
void zb_dump_put_buf(zb_uint8_t *buf, zb_uint_t len, zb_bool_t is_w)
{
  if (ZB_U2B(g_traf_dump))
  {
    zb_uint16_t dump_size = zb_dump_rec_size((zb_uint16_t)len);
    zb_uint8_t mask = (is_w
                       ? 0x80U | (ZB_PIBCACHE_CURRENT_CHANNEL() - ZB_TRANSCEIVER_START_CHANNEL_NUMBER + 1U)
                       : 0U);

#ifdef ZB_NET_TRACE
    zb_nettrace_batch_start(dump_size);
#endif

    zb_dump_put_hdr(dump_size, mask);
    zb_dump_put_bytes(buf, (zb_short_t)len);
#ifdef ZB_TRACE_TO_PORT
    zb_trace_msg_port_do();
#else
    (void)zb_osif_file_flush(zb_trace_dump_file());
#endif

#ifdef ZB_NET_TRACE
    zb_nettrace_batch_commit();
#endif
  }
}
#endif /* !ZB_TOOL && !NCP_MODE_HOST */

void zb_dump_put_2buf(zb_uint8_t *buf1, zb_uint_t len1, zb_uint8_t *buf2, zb_uint_t len2, zb_bool_t is_w)
{
  if (ZB_U2B(g_traf_dump))
  {
    zb_uint16_t dump_size = zb_dump_rec_size((zb_uint16_t)len1 + (zb_uint16_t)len2);

#ifdef ZB_NET_TRACE
    zb_nettrace_batch_start(dump_size);
#endif
    zb_dump_put_hdr(dump_size, (is_w ? 0x80U : 0U));
    zb_dump_put_bytes(buf1, (zb_short_t)len1);
    zb_dump_put_bytes(buf2, (zb_short_t)len2);
#ifdef ZB_TRACE_TO_PORT
    zb_trace_msg_port_do();
#else
    (void)zb_osif_file_flush(zb_trace_dump_file());
#endif
#ifdef ZB_NET_TRACE
    zb_nettrace_batch_commit();
#endif
  }
}
#endif  /* ZB_TRAFFIC_DUMP_ON */

#if defined(ZB_BINARY_TRACE)

/** @cond DOXYGEN_DEBUG_SECTION */

/*
 * trace functions that does not use printf()
 */

/** @addtogroup ZB_TRACE
 *  @{
 */
zb_uint16_t zb_trace_rec_size(zb_uint16_t *args_size)
{
  zb_size_t local_args_size = ((zb_size_t)*args_size + sizeof(zb_minimal_vararg_t) - 1U)/sizeof(zb_minimal_vararg_t)*sizeof(zb_minimal_vararg_t);
  *args_size = (zb_uint16_t)local_args_size;
  return *args_size + 2U + 2U + 2U + (zb_uint16_t)sizeof(struct trace_hdr_s);
}


void zb_trace_put_hdr(zb_uint16_t file_id, zb_uint16_t trace_rec_len)
{
  struct trace_hdr_s hdr;
  zb_time_t timer = ZB_TIMER_GET();

  hdr.sig[0] = TRACE_HDR_SIG0;
  hdr.sig[1] = TRACE_HDR_SIG1;
  hdr.h.len =  (zb_uint8_t)trace_rec_len;
  hdr.h.len -= (zb_uint8_t)sizeof(hdr.sig[0]);
  hdr.h.len -= (zb_uint8_t)sizeof(hdr.sig[1]);
  hdr.h.type = (zb_uint8_t)ZB_MAC_TRANSPORT_TYPE_TRACE;
  /* memcpy wouldn't accept volatile pointer.
     Is this cast acceptable?
  */
  ZB_HTOLE16((zb_uint8_t *)&hdr.h.time, (void const *)&timer);
  zb_trace_put_bytes(file_id, (zb_uint8_t *)&hdr, (zb_short_t)sizeof(hdr));
}


void zb_trace_put_u16(zb_uint16_t file_id, zb_uint16_t val)
{
  zb_uint8_t buf[2];
  /* print in little endian */
  buf[0] = ((zb_uint8_t)val & 0xFFU);
  buf[1] = ((zb_uint8_t)(val >> 8) & 0xFFU);
  zb_trace_put_bytes(file_id, buf, 2);
}


void zb_trace_put_vararg(zb_uint16_t file_id, zb_minimal_vararg_t val)
{
  zb_uint8_t buf[sizeof(zb_minimal_vararg_t)];
  zb_uint_t i;

  /* print in little endian */
  for (i = 0 ; i < sizeof(val) ; ++i)
  {
    buf[i] = ((zb_uint8_t)val & 0xFFU);
    val >>= 8;
  }
  zb_trace_put_bytes(file_id, buf, (zb_short_t)sizeof(buf));
}

#endif /* ZB_BINARY_TRACE */
#endif  /* ZB_TRACE_LEVEL || ZB_TRAFFIC_DUMP_ON */

#ifdef ZB_TRAFFIC_DUMP_ON

/* should be implemented in the caller module */
void zb_dump_put_bytes(zb_uint8_t *buf, zb_short_t len);


zb_uint16_t zb_dump_rec_size(zb_uint16_t dump_size)
{
  return dump_size + (zb_uint16_t)sizeof(struct trace_hdr_s);
}

void zb_dump_put_hdr(zb_uint_t dump_rec_len, zb_uint8_t mask)
{
  static struct trace_hdr_s hdr;
  zb_time_t timer = ZB_TIMER_GET();

  hdr.sig[0] = TRACE_HDR_SIG0;
  hdr.sig[1] = TRACE_HDR_SIG1;
  hdr.h.len =  (zb_uint8_t)dump_rec_len;
  hdr.h.len -= (zb_uint8_t)sizeof(hdr.sig[0]);
  hdr.h.len -= (zb_uint8_t)sizeof(hdr.sig[1]);
  hdr.h.type = (zb_uint8_t)ZB_MAC_TRANSPORT_TYPE_DUMP | mask;
  ZB_HTOLE16((zb_uint8_t *)&hdr.h.time, (void const *)&timer);

#if defined ZB_TRACE_TO_FILE && !defined ZB_BINARY_TRACE
  zb_dump_put_bytes((zb_uint8_t *)&hdr.h, sizeof(hdr.h));
#else
  zb_dump_put_bytes((zb_uint8_t *)&hdr, (zb_short_t)sizeof(hdr));
#endif
}

#endif /* ZB_TRAFFIC_DUMP_ON */

/** @} */
/** @endcond */ /* DOXYGEN_DEBUG_SECTION */
