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
/*  PURPOSE: OTA protocol implementation - both host and device side
*/

#define ZB_TRACE_FILE_ID 6670
#include "zb_macsplit_transport.h"

#ifdef ZB_MACSPLIT_FW_UPGRADE

void zb_ota_protocol_init()
{
  TRACE_MSG(TRACE_MAC2, ">zb_ota_protocol_init", (FMT__0));

  ZB_BZERO(&OTA_CTX(), sizeof(zb_ota_protocol_context_t));

  TRACE_MSG(TRACE_MAC2, "<zb_ota_protocol_init", (FMT__0));
}

#endif  /* ZB_MACSPLIT_FW_UPGRADE */

