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
/* PURPOSE: TP/R20/BV-12: Commissioning the Short Address
*/

#define ZB_TEST_NAME TP_R20_BV_12_ZED
#define ZB_TRACE_FILE_ID 40860

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "../common/zb_cert_test_globals.h"

#define SHORT_ADDR 0xABCD
//#define TEST_CHANNEL (1l << 24)

#define TEST_WITH_EMBER_COORDINATOR
#ifdef TEST_WITH_EMBER_COORDINATOR
/* Ember EM-ISA3-01 */
static zb_ieee_addr_t g_ieee_addr_c = {0xf0, 0x23, 0x07, 0x00, 0x00, 0xed, 0x21, 0x00};
#else
static zb_ieee_addr_t g_ieee_addr_c = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
#endif

/* IEEE addres ZED */
static zb_ieee_addr_t g_ieee_addr      = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};

/* For key pair setup with ZC */
static zb_uint8_t g_key1[16]           = {0x5A, 0x69, 0x67, 0x42, 0x65, 0x65, 0x41, 0x6C, 0x6C, 0x69, 0x61, 0x6E, 0x63, 0x65, 0x30, 0x39};

/* apsUseExtendedPANID */
static zb_ieee_addr_t g_aps_ext_pan_id = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("zdo_zed");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    /* set ieee addr */
    zb_set_long_address(g_ieee_addr);
    zb_zdo_set_aps_unsecure_join(ZB_FALSE);

    zb_cert_test_set_security_level(0x05);

    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zed_role();
    zb_set_rx_on_when_idle(ZB_TRUE);

    zb_set_pan_id(0x1aaa);

    /* Will do rejoin. xpanid will be == zc long address */
    zb_set_use_extended_pan_id(g_aps_ext_pan_id);
    zb_cert_test_set_network_addr(SHORT_ADDR);

    zb_set_nvram_erase_at_start(ZB_TRUE);
    if (zboss_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
    }
    else
    {
        zb_secur_update_key_pair(g_ieee_addr_c,
                                 g_key1,
                                 ZB_SECUR_GLOBAL_KEY,
                                 ZB_SECUR_VERIFIED_KEY,
                                 ZB_SECUR_KEY_SRC_UNKNOWN);
        zboss_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    if (0 == status)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));
            break;

        case ZB_COMMON_SIGNAL_CAN_SLEEP:
#ifdef ZB_USE_SLEEP
            zb_sleep_now();
#endif /* ZB_USE_SLEEP */
            break;

        default:
            TRACE_MSG(TRACE_ERROR, "Unknown signal %hd", (FMT__H, sig));
        }
    }
    else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
    {
        TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, status));
    }

    if (param)
    {
        zb_buf_free(param);
    }
}
