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

#define ZB_TEST_NAME TP_R20_BV_02_ZC
#define ZB_TRACE_FILE_ID 40821
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"

#include "../common/zb_cert_test_globals.h"
#include "tp_r20_bv-02_common.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#if !defined ZB_CERTIFICATION_HACKS
#error ZB_CERTIFICATION_HACKS not defined!
#endif

static const zb_ieee_addr_t g_ext_panid = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const zb_ieee_addr_t g_ieee_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
/* [zb_secur_setup_preconfigured_key_1] */
static const zb_uint8_t g_key_nwk[16] = { 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0, 0, 0, 0, 0, 0, 0, 0};
/* [zb_secur_setup_preconfigured_key_1] */

static const zb_ieee_addr_t g_ieee_addr1 = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
static const zb_uint8_t g_key1[16] = { 0x12, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33};
static const zb_ieee_addr_t g_ieee_addr2 = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const zb_ieee_addr_t g_ieee_addr3 = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_1_zc");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif


    zb_set_long_address(g_ieee_addr);
    zb_set_use_extended_pan_id(g_ext_panid);
    zb_set_pan_id(0x1aaa);

    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zc_role();

    /* [zb_secur_setup_preconfigured_key_2] */
    zb_secur_setup_nwk_key((zb_uint8_t *) g_key_nwk, 0);
    /* [zb_secur_setup_preconfigured_key_2] */
    /* Same global key for all devices.
       FIXME: do we need to use single slot for all devices here??
     */

    /* only ZR1 is visible for ZC */
    MAC_ADD_VISIBLE_LONG((zb_uint8_t *) g_ieee_addr1);
    zb_set_max_children(1);

    zb_zdo_set_aps_unsecure_join(ZB_TRUE);
    zb_bdb_set_legacy_device_support(ZB_TRUE);
    zb_set_nvram_erase_at_start(ZB_TRUE);
    if (zboss_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
    }
    else
    {
        ZB_CERT_HACKS().use_preconfigured_aps_link_key = 1U;

        zb_secur_update_key_pair((zb_uint8_t *) g_ieee_addr1,
                                 (zb_uint8_t *) g_key1,
                                 ZB_SECUR_GLOBAL_KEY,
                                 ZB_SECUR_VERIFIED_KEY,
                                 ZB_SECUR_KEY_SRC_UNKNOWN);
        zb_secur_update_key_pair((zb_uint8_t *) g_ieee_addr2,
                                 (zb_uint8_t *) g_key1,
                                 ZB_SECUR_GLOBAL_KEY,
                                 ZB_SECUR_VERIFIED_KEY,
                                 ZB_SECUR_KEY_SRC_UNKNOWN);
        zb_secur_update_key_pair((zb_uint8_t *) g_ieee_addr3,
                                 (zb_uint8_t *) g_key1,
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

    TRACE_MSG(TRACE_ERROR, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    if (0 == status)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));
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


/*! @} */
