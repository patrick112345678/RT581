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
/* PURPOSE: TP_R22_SGMB_09 ZigBee Coordinator (gZMC)
*/

#define ZB_TEST_NAME TP_R22_SGMB_09_GZMC
#define ZB_TRACE_FILE_ID 40046

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

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

static void test_clear_power_neg_support_bit(zb_uint8_t unused);

static const zb_ieee_addr_t g_ieee_addr_gzmc = IEEE_ADDR_GZMC;

MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("zdo_1_gzmc");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    zb_set_long_address(g_ieee_addr_gzmc);
    zb_cert_test_set_common_channel_settings();

    zb_cert_test_set_zc_role();
    zb_set_use_extended_pan_id(g_ext_pan_id);
    zb_set_pan_id(TEST_PAN_ID);

#ifdef SECURITY_LEVEL
    zb_cert_test_set_security_level(SECURITY_LEVEL);
#endif

    zb_secur_setup_nwk_key((zb_uint8_t *) g_nwk_key0, 0);

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
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    switch (sig)
    {
    case ZB_ZDO_SIGNAL_DEFAULT_START:
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APS1, "Device started, status %d", (FMT__D, status));
        if (status == 0)
        {
            ZB_SCHEDULE_ALARM(test_clear_power_neg_support_bit, 0, TEST_GZMC_DISABLE_POWER_NEG);
        }
        break; /* ZB_ZDO_SIGNAL_DEFAULT_START */

    default:
        TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}

static void test_clear_power_neg_support_bit(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    TRACE_MSG(TRACE_APP1, ">>test_clear_power_neg_support_bit", (FMT__0));

    ZB_CERT_HACKS().disable_power_negotiation_support = 1;
}
