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
/* PURPOSE: DUT ZED
*/

#define ZB_TEST_NAME RTP_NWK_05_DUT_ZED
#define ZB_TRACE_FILE_ID 47445

#include "zb_common.h"
#include "../common/zb_reg_test_globals.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_ED_ROLE
#error ED role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error ZB_USE_NVRAM is not compiled!
#endif

/**
 * Global variables definitions
 */
zb_ieee_addr_t g_ed_addr = {0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21}; /* IEEE address of the
                                                                              * device */

MAIN()
{
    /* Trace enable */
    ZB_SET_TRACE_ON();
    /* Traffic dump enable*/
    ZB_SET_TRAF_DUMP_ON();
    ARGV_UNUSED;

    ZB_INIT("dut_zed");

    zb_set_long_address(g_ed_addr);

    zb_reg_test_set_common_channel_settings();
    zb_set_network_ed_role((1l << TEST_CHANNEL));

    zb_set_nvram_erase_at_start(ZB_FALSE);

    /* Set end-device configuration parameters */
    zb_set_ed_timeout(ED_AGING_TIMEOUT_64MIN);
    zb_set_keepalive_timeout(ZB_MILLISECONDS_TO_BEACON_INTERVAL(1000));
    zb_set_rx_on_when_idle(ZB_FALSE);

    if (zboss_start_no_autostart() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
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

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_SKIP_STARTUP:
            TRACE_MSG(TRACE_APP1, "ZB_ZDO_SIGNAL_SKIP_STARTUP: boot, not started yet", (FMT__0));
            zboss_start_continue();
            break;

        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
            TRACE_MSG(TRACE_APP1, "FIRST_START: start steering", (FMT__0));
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
            break;

        case ZB_BDB_SIGNAL_STEERING:
            TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
            break;

        case ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY:
            TRACE_MSG(TRACE_APP1, "Production config is ready", (FMT__0));
            break;

        default:
            TRACE_MSG(TRACE_ERROR, "Unknown signal %hd, do nothing", (FMT__H, sig));
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
        zb_buf_free(param);
    }
}

/*! @} */
