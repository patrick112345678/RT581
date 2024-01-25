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
/* PURPOSE: MAC traffic dump
*/


#define ZB_TRACE_FILE_ID 292
#include "zb_common.h"

#if !defined ZB_MACSPLIT_HOST

/*! \addtogroup ZB_MAC */
/*! @{ */


#include "zb_scheduler.h"
#include "zb_nwk.h"
#include "zb_mac.h"
#include "mac_internal.h"
#include "zb_mac_transport.h"
#ifndef ZB_ALIEN_MAC
#include "zb_mac_globals.h"
#endif


#ifdef ZB_TRAFFIC_DUMP_ON

static void mac_traffic_dump_put(zb_uint32_t trace_cnt, zb_uint8_t *buf, zb_uint_t len, zb_bool_t is_w)
{
  zb_dump_hdr_v3_t dh;

  ZB_BZERO(&dh, sizeof(dh));
  dh.version = 3;

#ifdef ZB_ALIEN_MAC
  dh.page = ZB_PIBCACHE_CURRENT_PAGE();
#else
  dh.page = MAC_PIB().phy_current_page;
#endif

#ifdef ZB_ALIEN_MAC
  dh.channel = ZB_PIBCACHE_CURRENT_CHANNEL();
#else
  dh.channel = MAC_PIB().phy_current_channel;
#endif /* ifdef ZB_ALIEN_MAC */

  dh.trace_cnt = trace_cnt;
  ZB_HTOLE32_ONPLACE(dh.trace_cnt);
  dh.len = (zb_uint8_t)len;

  zb_dump_put_2buf((zb_uint8_t*)&dh, sizeof(dh), buf, len, is_w);
}

/**
   Dump traffic to the dump file.

   Dump file format is same as pipe_data_router produce: zb_mac_transport_hdr_t
   before packet.

   @param buf    - output buffer.
   @param is_w   - if 1, this is output, else input

   @return nothing.
*/
void zb_mac_traffic_dump(zb_bufid_t buf, zb_bool_t is_w)
{
  zb_uint8_t save_tail[ZB_TAIL_SIZE_FOR_RECEIVED_MAC_FRAME];
  zb_uint8_t tail_size;
  zb_int8_t rssi = 0;
  zb_uint8_t lqi = 0;
  static zb_uint32_t cnt = 0;
  zb_uint8_t *p;
  zb_uint32_t trace_cnt;

  if (ZB_U2B(g_traf_dump))
  {
    cnt++;

#if TRACE_ENABLED(TRACE_MAC2)
    {
      zb_mac_mhr_t mhr;

      zb_parse_mhr(&mhr, buf);
      trace_cnt = zb_trace_get_counter();
      TRACE_MSG(TRACE_MAC2, "dump #%d trace# %d param %hd len %d is_w %hd seq %hd %x:%x to %x:%x",
                (FMT__D_D_H_H_D_H_D_D_D_D, cnt, trace_cnt, buf, zb_buf_len(buf), is_w, mhr.seq_number,
                 mhr.src_pan_id, mhr.src_addr.addr_short,
                 mhr.dst_pan_id, mhr.dst_addr.addr_short));
    }
#else
    trace_cnt = zb_trace_get_counter();
#endif /* TRACE_ENABLED(TRACE_MAC2) */

/* Let's dump in TI25xx (DISSECT_IEEE802154_OPTION_CC24xx in Wireshark):
   - no length byte
   - data[len-2] = rssi
   data[len-1] = correlation | (crc_ok << 7)

   Dump both in and out at the same way (add zero bytes to out)
*/
    if (is_w)
    {
#ifdef ZB_NSNG
      tail_size = 2;
#else
      tail_size = 0;
#endif
    }
    else
    {
      rssi = ZB_MAC_GET_RSSI(buf);
      lqi = ZB_MAC_GET_LQI(buf);
      tail_size = ZB_TAIL_SIZE_FOR_RECEIVED_MAC_FRAME;
    }
    if (ZB_U2B(tail_size))
    {
      ZB_MEMCPY(save_tail, (zb_uint8_t*)zb_buf_begin(buf) + zb_buf_len(buf) - tail_size, tail_size);
      (void)zb_buf_cut_right(buf, tail_size);
    }
    p = zb_buf_alloc_right(buf, 2);
    p[0] = (zb_uint8_t)rssi;
    /* set CRC ok bit. Show LQI as 1/2 of its real value due to format limitation. */
    p[1] = (lqi/2U) | (1U<<7);

    mac_traffic_dump_put(trace_cnt, zb_buf_begin(buf), zb_buf_len(buf), is_w);

    (void)zb_buf_cut_right(buf, 2);
    if (ZB_U2B(tail_size))
    {
      p = zb_buf_alloc_right(buf, tail_size);
      ZB_MEMCPY(p, save_tail, tail_size);
    }
  }
}

#ifndef ZB_ALIEN_MAC

/* In order not to use a dedicated buffer, this function  */
void zb_mac_dump_mac_ack(zb_bool_t is_out, zb_uint8_t data_pending, zb_uint8_t dsn)
{
  zb_mac_mhr_t mhr;
  zb_uint_t mhr_len, len;
  zb_uint8_t ptr[16];
  zb_uint32_t trace_cnt;

/*
  7.2.2.3 Acknowledgment frame format
  | FCF 2 bytes | Seq num 1 byte |
*/
  mhr_len = zb_mac_calculate_mhr_length(ZB_ADDR_NO_ADDR, ZB_ADDR_NO_ADDR, ZB_FALSE);

  len = mhr_len + 2U;
  ZB_BZERO(ptr, len);
  ZB_BZERO2(mhr.frame_control);
  ZB_FCF_SET_FRAME_TYPE(mhr.frame_control, MAC_FRAME_ACKNOWLEDGMENT);
  ZB_FCF_SET_FRAME_PENDING_BIT(mhr.frame_control, (data_pending != 0U) ? 1U : 0U);
  mhr.seq_number = dsn;
  zb_mac_fill_mhr(ptr, &mhr);

  if (len <= sizeof(ptr))
  {
    /* zero rssi & lqi */
    ptr[len-1U] = (1U<<7);
  }
  else
  {
    ZB_ASSERT(ZB_FALSE);
  }

  trace_cnt = zb_trace_get_counter();
  mac_traffic_dump_put(trace_cnt, ptr, len, is_out);
}
#endif  /* !ZB_ALIEN_MAC */


/*! @} */
#endif /* ZB_TRAFFIC_DUMP_ON */

#endif /* !ZB_MACSPLIT_HOST */
