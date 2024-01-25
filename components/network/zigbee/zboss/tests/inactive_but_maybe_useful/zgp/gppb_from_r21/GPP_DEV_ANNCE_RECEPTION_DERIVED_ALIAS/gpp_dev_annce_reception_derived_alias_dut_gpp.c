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
/* PURPOSE: DUT ZR
*/

#define ZB_TEST_NAME GPP_DEV_ANNCE_RECEPTION_DERIVED_ALIAS_DUT_GPP
#define ZB_TRACE_FILE_ID 41303

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_zcl.h"
#include "zb_secur_api.h"
#include "zb_ha.h"
#include "test_config.h"

#ifdef ZB_ENABLE_HA
#include "../include/zgp_test_templates.h"

#ifndef ZB_NSNG
static void left_btn_hndlr(zb_uint8_t param)
{
    ZVUNUSED(param);
}
#endif

static zb_ieee_addr_t g_dut_gpp_addr = DUT_GPP_IEEE_ADDR;
static zb_uint8_t g_key_nwk[] = NWK_KEY;

MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("dut_gpp");

    zb_set_default_ffd_descriptor_values(ZB_ROUTER);

    ZB_AIB().aps_channel_mask = (1 << TEST_CHANNEL);
    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_dut_gpp_addr);
    ZB_PIBCACHE_PAN_ID() = TEST_PAN_ID;
    ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);

    zb_secur_setup_nwk_key(g_key_nwk, 0);

    /* Must use NVRAM for ZGP */
    ZB_AIB().aps_use_nvram = 1;
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

#else // defined ZB_ENABLE_HA

#include <stdio.h>
MAIN()
{
    ARGV_UNUSED;

    printf("HA profile should be enabled in zb_config.h\n");

    MAIN_RETURN(1);
}

#endif
