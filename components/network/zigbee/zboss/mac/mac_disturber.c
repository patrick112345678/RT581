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
/* PURPOSE: Disturber device implementation
*/

#define ZB_TRACE_FILE_ID 862
#include "zb_common.h"

#ifndef ZB_ALIEN_MAC

#include "zb_scheduler.h"
#include "zb_nwk.h"
#include "zb_mac.h"
#include "mac_internal.h"
#include "zb_mac_transport.h"
#include "zb_secur.h"

#ifdef MAC_CERT_TEST_HACKS

/*
 * 02.02.2018 CR vyacheslav.kostyuchenko start
 *
 * I not found this define in the stack, so i
 * declare his this.
 */
#define ZB_DISTURBER_PANID ((zb_uint16_t)0xFFFF)
/*
 * 02.02.2018 CR vyacheslav.kostyuchenko end
 */

/*! \addtogroup ZB_MAC */
/*! @{ */

void zb_mac_disturber_loop(zb_uint8_t logical_channel)
{
  zb_uint8_t *ptr;
  zb_bufid_t buf;
  zb_mac_mhr_t mhr;
  zb_ushort_t mhr_len = zb_mac_calculate_mhr_length(ZB_ADDR_16BIT_DEV_OR_BROADCAST, ZB_ADDR_16BIT_DEV_OR_BROADCAST, ZB_TRUE);

  ZB_TRANSCEIVER_SET_CHANNEL(MAC_PIB().phy_current_page, logical_channel);

  ZB_BZERO2(mhr.frame_control);
  ZB_FCF_SET_FRAME_TYPE(mhr.frame_control, MAC_FRAME_DATA);
  /* security enable is 0 */
  /* frame pending is 0 */
  /* ack req 0 */
  ZB_FCF_SET_PANID_COMPRESSION_BIT(mhr.frame_control, 1U);
  ZB_FCF_SET_DST_ADDRESSING_MODE(mhr.frame_control, ZB_ADDR_16BIT_DEV_OR_BROADCAST);
  /* 7.2.3 Frame compatibility: All unsecured frames specified in this
     standard are compatible with unsecured frames compliant with IEEE Std 802.15.4-2003 */
  ZB_FCF_SET_FRAME_VERSION(mhr.frame_control, MAC_FRAME_IEEE_802_15_4_2003);
  ZB_FCF_SET_SRC_ADDRESSING_MODE(mhr.frame_control, ZB_ADDR_16BIT_DEV_OR_BROADCAST);
  /* mac spec 7.5.6.1 Transmission */
  mhr.seq_number = 8;
  /* put our pan id as src and dst pan id */
  mhr.dst_pan_id = mhr.src_pan_id = ZB_DISTURBER_PANID;
  mhr.dst_addr.addr_short = mhr.src_addr.addr_short = -1;

  buf = zb_buf_get_out();

  while (1)
  {
    zb_buf_reuse(buf);
    ptr = (zb_uint8_t *)zb_buf_initial_alloc(buf, 100);
    ZB_MEMSET(ptr, 0xf0, 100);
    ptr = (zb_uint8_t *)zb_buf_alloc_left(buf, mhr_len);
    zb_mac_fill_mhr(ptr, &mhr);
    /* TODO: switch off CSMA/CA! */
    
    /*
     * 02.02.2018 CR vyacheslav.kostyuchenko start
     *
     * Add third parameter
     */
    ZB_TRANS_SEND_FRAME(mhr_len, buf, ZB_MAC_TX_WAIT_NONE);
    /*
     * 02.02.2018 CR vyacheslav.kostyuchenko end
     */

#if 0
    /**
     * FIXME: The function zb_check_cmd_tx_status() is void as defined in ZOI-512
     * As this functionality is not used for a long time, it's disable now.
     */
    while (zb_check_cmd_tx_status() != RET_OK)
    {
      if (ZB_GET_TRANS_INT())
      {
        TRACE_MSG(TRACE_COMMON2, "uz2400 int!", (FMT__0 ));
        ZB_CLEAR_TRANS_INT();
        ZB_CHECK_INT_STATUS();
      }
    }
#endif
  }
}

/*! @} */
#endif  /* MAC_CERT_TEST_HACKS */

#endif  /* ZB_ALIEN_MAC */
