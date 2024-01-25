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
/* PURPOSE: TP/R21/BV-10 - legacy (r20) ZC
*/

#define ZB_TEST_NAME TP_R21_BV_10_GZC
#define ZB_TRACE_FILE_ID 40931

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

#ifndef ZB_CERTIFICATION_HACKS
#error Define ZB_CERTIFICATION_HACKS
#endif


static const zb_ieee_addr_t g_ext_panid = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const zb_ieee_addr_t g_ieee_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
/* [zb_secur_setup_preconfigured_key_1] */
static const zb_uint8_t g_key_nwk[16] = { 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0, 0, 0, 0, 0, 0, 0, 0};
/* [zb_secur_setup_preconfigured_key_1] */

static const zb_ieee_addr_t g_ieee_addr1 = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("zdo_1_gzc");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    zb_set_long_address(g_ieee_addr);
    zb_set_use_extended_pan_id(g_ext_panid);

    zb_set_pan_id(0x1aaa);

    /* let's always be coordinator */
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zc_role();
    zb_secur_setup_nwk_key((zb_uint8_t *) g_key_nwk, 0);

    /* only ZR1 is visible for ZC */
    MAC_ADD_VISIBLE_LONG((zb_uint8_t *) g_ieee_addr1);
    zb_set_max_children(1);

    /* allow joining without perm_join (for connection through zr) */
    ZB_TCPOL().authenticate_always = 1;

    zb_zdo_set_aps_unsecure_join(ZB_TRUE);

    /* Simulate legacy ZC (without Stack Revision in Node Descriptor's Server Flags). */
    /* Set this flag before device initialization. */
    ZB_CERT_HACKS().report_legacy_stack_revision_in_node_descr = 1;
    zb_set_default_ffd_descriptor_values(ZB_COORDINATOR);
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
        TRACE_MSG(TRACE_APS1, "Device started, status %d", (FMT__D, status));
        break; /* ZB_ZDO_SIGNAL_DEFAULT_START */

    default:
        TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}

/*! @} */
