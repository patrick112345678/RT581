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
/*  PURPOSE: NVRAM ERASE "Pseudo test"
*/

#define ZB_TRACE_FILE_ID 64903
#include "zb_multitest.h"

static zb_test_ctx_t s_test_ctx;


void zb_multitest_reset_test_ctx(void)
{
  ZB_BZERO(&s_test_ctx, sizeof(s_test_ctx));

  s_test_ctx.page = ZB_MULTITEST_DEAULT_PAGE;
  s_test_ctx.channel = ZB_MULTITEST_DEAULT_CHANNEL;
  s_test_ctx.mode = ZB_TEST_CONTROL_ALARMS;
}


zb_test_ctx_t* zb_multitest_get_test_ctx(void)
{
  return &s_test_ctx;
}
