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
/* PURPOSE: TP/R21/BV-03 - ZigBee End Device (gZED) - joining to DUT ZR
*/

#define ZB_TEST_NAME TP_R21_BV_03_GZED
#define ZB_TRACE_FILE_ID 40777

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "test_common.h"
#include "../common/zb_cert_test_globals.h"


/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_ED_ROLE
#error define ZB_ED_ROLE to compile ze tests
#endif

static const zb_ieee_addr_t g_ieee_addr_gzed = IEEE_ADDR_gZED;

MAIN()
{
    ARGV_UNUSED;

    ZB_SET_TRAF_DUMP_ON();
    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("zdo_6_gzed");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    zb_set_long_address(g_ieee_addr_gzed);
    //zb_set_network_ed_role_legacy(1l << TEST_CHANNEL);
    zb_set_network_ed_role_legacy(1l << 15);
    zb_zdo_set_aps_unsecure_join(ZB_TRUE);
    zb_set_rx_on_when_idle(ZB_FALSE);

#ifdef SECURITY_LEVEL
    zb_cert_test_set_security_level(SECURITY_LEVEL);
#endif

    zb_set_nvram_erase_at_start(ZB_TRUE);

    if (zboss_start() != RET_OK)
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
        case ZB_ZDO_SIGNAL_DEFAULT_START:
            TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
            break;
        case ZB_COMMON_SIGNAL_CAN_SLEEP:
#ifdef ZB_USE_SLEEP
            zb_sleep_now();
#endif /* ZB_USE_SLEEP */
            break;

        default:
            TRACE_MSG(TRACE_ERROR, "Unknown signal %hd", (FMT__H, sig));
            break;
        }
    }
    else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
    {
        TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d",
                  (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
    }

    zb_buf_free(param);
}


/*! @} */

