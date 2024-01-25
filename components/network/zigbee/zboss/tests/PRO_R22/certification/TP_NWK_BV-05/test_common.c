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
/* PURPOSE:
*/


#define ZB_TRACE_FILE_ID 40760
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "test_common.h"

zb_ieee_addr_t g_ieee_addr_zc  = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
zb_ieee_addr_t g_ieee_addr_r1  = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
zb_ieee_addr_t g_ieee_addr_r2  = {0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00};
zb_ieee_addr_t g_ieee_addr_r3  = {0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00};
zb_ieee_addr_t g_ieee_addr_r4  = {0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00};
zb_ieee_addr_t g_ieee_addr_r5  = {0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00};
zb_ieee_addr_t g_ieee_addr_r6  = {0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00};
zb_ieee_addr_t g_ieee_addr_r7  = {0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00};
zb_ieee_addr_t g_ieee_addr_r8  = {0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00};
zb_ieee_addr_t g_ieee_addr_r9  = {0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00};
zb_ieee_addr_t g_ieee_addr_r10 = {0x00, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00};
zb_ieee_addr_t g_ieee_addr_r11 = {0x00, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00};
zb_ieee_addr_t g_ieee_addr_r12 = {0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00};
zb_ieee_addr_t g_ieee_addr_r13 = {0x00, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00};
zb_ieee_addr_t g_ieee_addr_r14 = {0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00};
zb_ieee_addr_t g_ieee_addr_r15 = {0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00};
zb_ieee_addr_t g_ieee_addr_r16 = {0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00};
zb_ieee_addr_t g_ieee_addr_r17 = {0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00};

#if 0
static void send_mgmt_lqi_request(zb_bufid_t param)
{
  zb_bufid_t buf = param;
  zb_zdo_mgmt_lqi_param_t *req_param = ZB_BUF_GET_PARAM(buf, zb_zdo_mgmt_lqi_param_t);

  TRACE_MSG(TRACE_ZDO1, ">>send_mgmt_lqi_request buf = %d", (FMT__D, param));

  req_param->dst_addr = 0;
  req_param->start_index = 0;
  zb_zdo_mgmt_lqi_req(param, NULL);

  TRACE_MSG(TRACE_ZDO1, "<<send_mgmt_lqi_request", (FMT__0));
}
#endif

void test_after_startup_action(zb_uint8_t param)
{
  if (param == 0)
  {
    zb_buf_get_out_delayed(test_after_startup_action);
  }
  else
  {
    //ZB_SCHEDULE_ALARM(send_mgmt_lqi_request, param, ZB_TIME_ONE_SECOND * MGMT_LQI_REQUEST_DELAY_SECONDS);
    zb_buf_free(param);
  }
}
