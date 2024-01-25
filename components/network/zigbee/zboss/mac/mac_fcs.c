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
/* PURPOSE: routine for FCS calculation
*/

#define ZB_TRACE_FILE_ID 303
#include "zb_common.h"
#include "zb_mac.h"

#if defined ZB_MAC_TESTING_MODE
#include "zb_mac_globals.h"
#endif

/*! \addtogroup ZB_MAC */
/*! @{ */

#if defined ZB_TRAFFIC_DUMP_ON || defined ZB_NSNG || defined ZB_MAC_TESTING_MODE
/* FCS is necessary for ns build only. Real transiver calc fcs itself */

void zb_mac_fcs_add(zb_bufid_t buf)
{
  zb_uint16_t crc;
  zb_uint8_t *p;

  crc = zb_crc16(zb_buf_begin(buf), 0, zb_buf_len(buf));

#if defined ZB_MAC_TESTING_MODE
  if (MAC_CTX().cert_hacks.invalid_fcs)
  {
    crc = 0xBAD1;
  }
#endif

  p = zb_buf_alloc_right(buf, 2);
  ZB_HTOLE16(p, &crc);
}

#endif


/*! @} */
