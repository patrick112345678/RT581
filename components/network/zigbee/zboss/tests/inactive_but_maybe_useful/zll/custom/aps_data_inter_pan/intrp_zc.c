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
/* PURPOSE: Custom test for ZLL profile
*/

#define ZB_TRACE_FILE_ID 41639
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#ifndef ZB_ALIEN_MAC
#include "zb_mac_globals.h"
#endif
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_aps_interpan.h"

#include "intrp_test.h"

#if defined ZB_ENABLE_INTER_PAN_EXCHANGE

#if ! defined ZB_ROUTER_ROLE
#error define ZB_ROUTER_ROLE to compile zr1 tests
#endif

void send_intrp_data(zb_uint8_t param);

zb_ieee_addr_t g_ieee_addr_my = INTRP_TEST_IEEE_ADDR_2;

MAIN()
{
  ARGV_UNUSED;

#if ! (defined KEIL || defined ZB_PLATFORM_LINUX_ARM_2400)
  if ( argc < 3 )
  {
    printf("%s <read pipe path> <write pipe path>\n", argv[0]);
    return 0;
  }
#endif

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("intrp_zc");


  zb_set_default_ed_descriptor_values();

  ZB_AIB().aps_channel_mask = 1l << MY_CHANNEL;
  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_my);
  //ZB_IEEE_ADDR_COPY(ZB_NIB_EXT_PAN_ID(), &g_ieee_addr_my);
  ZB_PIBCACHE_PAN_ID() = INTRP_TEST_PAN_ID_2;
#ifndef ZB_ALIEN_MAC
  MAC_PIB().mac_pan_id = INTRP_TEST_PAN_ID_2;
#endif
  ZG->nwk.handle.joined = 1;

  ZB_SET_NIB_SECURITY_LEVEL(0);

  if (zb_zll_dev_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "ERROR zdo_dev_start failed", (FMT__0));
  }
  else
  {
    zdo_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}

void zb_zdo_startup_complete(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  if ((buf->u.hdr.status == 0) || (buf->u.hdr.status == ZB_NWK_STATUS_ALREADY_PRESENT))
  {
    TRACE_MSG(TRACE_ZCL1, "Device STARTED OK", (FMT__0));
  }
  else
  {
    TRACE_MSG(
        TRACE_ERROR,
        "ERROR Device started FAILED status %d",
        (FMT__D, (int)buf->u.hdr.status));
  }
  zb_free_buf(buf);
}

void zb_intrp_data_confirm(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APS1, "> zb_intrp_data_confirm param %hd", (FMT__H, param));

  TRACE_MSG(TRACE_APS1, "< zb_intrp_data_confirm", (FMT__0));
}/* void zb_intrp_data_confirm(zb_uint8_t param) */

void zb_intrp_data_indication(zb_uint8_t param)
{
  zb_buf_t* buffer = ZB_BUF_FROM_REF(param);
  zb_intrp_data_ind_t* indication = ZB_GET_BUF_PARAM(buffer, zb_intrp_data_ind_t);

  TRACE_MSG(TRACE_APS1, "> zb_intrp_data_indication param %hd", (FMT__H, param));

  if (indication->dst_addr_mode != ZB_INTRP_ADDR_IEEE)
  {
    TRACE_MSG(TRACE_ERROR, "DstAddrMode is not unicast inter-PAN", (FMT__0));
    zb_free_buf(buffer);
  }
  else
  {
    ZB_SCHEDULE_CALLBACK(send_intrp_data, param);
  }

  TRACE_MSG(TRACE_APS1, "< zb_intrp_data_indication", (FMT__0));
}/* void zb_intrp_data_indication(zb_uint8_t param) */

void send_intrp_data(zb_uint8_t param)
{
  zb_buf_t* buffer = ZB_BUF_FROM_REF(param);
  zb_intrp_data_ind_t* indication = ZB_GET_BUF_PARAM(buffer, zb_intrp_data_ind_t);
  zb_intrp_data_req_t request;
  zb_uint8_t* data;

  TRACE_MSG(TRACE_APS1, "> send_aps_data param %hd", (FMT__H, param));

  ZB_BUF_REUSE(buffer);
  ZB_BZERO(&request, sizeof(request));
  request.dst_addr_mode = ZB_INTRP_ADDR_IEEE;
  ZB_IEEE_ADDR_COPY(request.dst_addr.addr_long, indication->src_addr);
  ZB_MEMCPY(&(request.dst_pan_id), &(indication->src_pan_id), sizeof(indication->src_pan_id));
  ZB_MEMCPY(&(request.profile_id), &(indication->profile_id), sizeof(indication->profile_id));
  ZB_MEMCPY(&(request.cluster_id), &(indication->cluster_id), sizeof(indication->cluster_id));
  ZB_BUF_INITIAL_ALLOC(buffer, sizeof(zb_ieee_addr_t), data);
  ZB_IEEE_ADDR_COPY(data, g_ieee_addr_my);
  request.asdu_handle = 0;
  ZB_MEMCPY(ZB_GET_BUF_PARAM(buffer, zb_intrp_data_req_t), &request, sizeof(zb_intrp_data_req_t));
  ZB_SCHEDULE_CALLBACK(zb_intrp_data_request, param);

  TRACE_MSG(TRACE_APS1, "< send_aps_data", (FMT__0));
}/* void send_aps_data(zb_uint8_t param) */

#else /* defined ZB_ENABLE_INTER_PAN_EXCHANGE */

#include <stdio.h>

int main()
{
  printf(" Inter-PAN exchange is not supported\n");
  return 0;
}

#endif /* defined ZB_ENABLE_INTER_PAN_EXCHANGE */
