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

#define ZB_TEST_NAME GPP_GPDF_BASIC_APP_000_DUT_GPPB
#define ZB_TRACE_FILE_ID 41341
#include "zb_common.h"

#if defined ZB_ENABLE_HA && defined ZB_ENABLE_ZGP_PROXY

#include "test_config.h"

#include "../include/zgp_test_templates.h"

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

static zb_ieee_addr_t g_zc_addr = DUT_GPPB_IEEE_ADDR;
static zb_uint8_t g_key_nwk[16] = { 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0, 0, 0, 0, 0, 0, 0, 0};

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

    zb_set_long_address(g_zc_addr);
    zb_set_network_coordinator_role(1 << TEST_CHANNEL);

    zb_secur_setup_nwk_key(g_key_nwk, 0);

    zb_set_nvram_erase_at_start(ZB_TRUE);

    zb_set_zgp_device_role(ZGP_DEVICE_PROXY_BASIC);

    HW_INIT();

    if (zboss_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
    }
    else
    {
        zboss_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_zdo_app_signal_hdr_t *sg_p = NULL;
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

    TRACE_MSG(TRACE_ZCL1, "> zboss_signal_handler %hd", (FMT__H, param));

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
            TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
            HW_DEV_START_INDICATION(2);
            break;
        default:
            TRACE_MSG(TRACE_APP1, "Unknown signal", (FMT__0));
        }
    }
    else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
    {
        TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
    }

    if (param)
    {
        zb_free_buf(ZB_BUF_FROM_REF(param));
    }

    TRACE_MSG(TRACE_ZCL1, "< zboss_signal_handler", (FMT__0));
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
