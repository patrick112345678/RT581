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
/* PURPOSE: ZC
*/

#define ZB_TEST_NAME TP_PED_13_DUTZC
#define ZB_TRACE_FILE_ID 40901

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


#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

static const zb_ieee_addr_t g_ieee_addr_dutzc = IEEE_ADDR_DUT_ZC;

MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("zdo_1_dutzc");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    /* set ieee addr */
    zb_set_long_address(g_ieee_addr_dutzc);

    /* let's always be coordinator */
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zc_role();
    zb_set_max_children(3);
    zb_set_use_extended_pan_id(g_ext_pan_id);
    zb_set_pan_id(g_pan_id);

#ifdef SECURITY_LEVEL
    zb_cert_test_set_security_level(SECURITY_LEVEL);
#endif

    zb_secur_setup_nwk_key((zb_uint8_t *) g_nwk_key0, 0);
    zb_secur_setup_nwk_key((zb_uint8_t *) g_nwk_key1, 1);

    zb_cert_test_set_security_level(SECURITY_LEVEL);

    zb_cert_test_set_keepalive_mode(ED_TIMEOUT_REQUEST_KEEPALIVE);

    zb_bdb_set_legacy_device_support(ZB_TRUE);
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
        if (status == 0)
        {
            TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
        }
        else
        {
            TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, status));
        }
        break; /* ZB_ZDO_SIGNAL_DEFAULT_START */

    default:
        if (status == 0)
        {
            TRACE_MSG(TRACE_APS1, "Unknown signal, status OK", (FMT__0));
        }
        else
        {
            TRACE_MSG(TRACE_ERROR, "Unknown signal, status %d", (FMT__D, status));
        }
        break;
    }

    zb_buf_free(param);
}

