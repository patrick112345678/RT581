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
/*  PURPOSE: NCP High level transport implementation for the host side: MANUF_TEST category
*/

#define ZB_TRACE_FILE_ID 21

#include "ncp_host_hl_proto.h"

void ncp_host_handle_manuf_test_response(void* data, zb_uint16_t len)
{
  ZVUNUSED(data);
  ZVUNUSED(len);
  /* We don't send Manuf Test Request from NCP Host. */
  ZB_ASSERT(0);
}

void ncp_host_handle_manuf_test_indication(void* data, zb_uint16_t len)
{
  ZVUNUSED(data);
  ZVUNUSED(len);
  /* We don't use Manuf Test for ZOI NCP. */
  ZB_ASSERT(0);
}
