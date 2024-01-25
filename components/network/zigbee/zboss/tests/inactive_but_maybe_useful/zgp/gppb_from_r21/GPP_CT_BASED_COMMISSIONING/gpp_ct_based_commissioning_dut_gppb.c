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
/* PURPOSE: Simple GPPB for GP device
*/

#define ZB_TEST_NAME GPP_CT_BASED_COMMISSIONING_DUT_GPPB
#define ZB_TRACE_FILE_ID 41400
#include "zb_common.h"

#if defined ZB_ENABLE_HA && defined ZB_ENABLE_ZGP_PROXY

#include "test_config.h"

#include "../include/zgp_test_templates.h"

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

static zb_ieee_addr_t g_zc_addr = DUT_GPPB_IEEE_ADDR;
static zb_uint8_t g_key_nwk[] = NWK_KEY;

#ifndef ZB_NSNG
static void left_btn_hndlr(zb_uint8_t param)
{
  ZVUNUSED(param);
}
#endif

MAIN()
{
  ARGV_UNUSED;

/* Init device, load IB values from nvram or set it to default */
  ZB_INIT("dut_gppb");

  /* let's always be coordinator */
  ZB_AIB().aps_designated_coordinator = 1;
  ZB_AIB().aps_channel_mask = (1<<TEST_CHANNEL);
  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_zc_addr);

  ZB_PIBCACHE_PAN_ID() = 0x1aaa;

  zb_secur_setup_nwk_key(g_key_nwk, 0);

  /* Must use NVRAM for ZGP */
  ZB_AIB().aps_use_nvram = 1;

  ZB_NIB_SET_USE_MULTICAST(ZB_FALSE);

  ZGP_CTX().device_role = ZGP_DEVICE_PROXY_BASIC;

  HW_INIT();

  if (zdo_dev_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
  }
  else
  {
    zcl_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);

  TRACE_MSG(TRACE_APP1, "> zb_zdo_startup_complete %hd", (FMT__H, param));

  if (buf->u.hdr.status == 0)
  {
    TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
    HW_DEV_START_INDICATION(2);
  }
  else
  {
    TRACE_MSG(
        TRACE_ERROR,
        "Device started FAILED status %d",
        (FMT__D, (int)buf->u.hdr.status));
  }
  zb_free_buf(buf);
  TRACE_MSG(TRACE_APP1, "< zb_zdo_startup_complete", (FMT__0));
}

#else // defined ZB_ENABLE_HA && defined ZB_ENABLE_ZGP_PROXY

#include <stdio.h>
MAIN()
{
  ARGV_UNUSED;

  printf("HA profile and ZGP proxy should be enabled in zb_config.h\n");

  MAIN_RETURN(1);
}

#endif
