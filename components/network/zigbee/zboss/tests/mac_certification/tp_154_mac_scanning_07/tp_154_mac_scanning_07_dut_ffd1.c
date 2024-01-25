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
/* PURPOSE: P/154/MAC/SCANNING-07 DUT FFD1
MAC-only build
*/

#define ZB_TEST_NAME TP_154_MAC_SCANNING_07_DUT_FFD1
#define ZB_TRACE_FILE_ID 57211
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "../common/zb_cert_test_globals.h"
#include "tp_154_mac_scanning_07_common.h"
#include "zb_osif.h"

#ifndef ZB_MULTI_TEST
#define USE_ZB_MLME_RESET_CONFIRM
#define USE_ZB_MLME_SCAN_CONFIRM
#include "zb_mac_only_stubs.h"
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_dut_ffd1 = TEST_DUT_FFD1_MAC_ADDRESS;

static void test_started_cb(zb_uint8_t param);
static void test_mlme_reset_request(zb_uint8_t param);
static void test_set_ieee_addr(zb_uint8_t param);
static void test_mlme_scan_request(zb_uint8_t param);

MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("tp_154_mac_scanning_07_dut_ffd1");

    ZB_SCHEDULE_CALLBACK(test_started_cb, 0);
    init_RT569_LED();
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
    while (1)
    {
        zb_sched_loop_iteration();
    }


    TRACE_DEINIT();

    MAIN_RETURN(0);
}


static void test_started_cb(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    //TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));

    zb_buf_get_out_delayed(test_mlme_reset_request);
}

/***************** Test functions *****************/
static void test_mlme_reset_request(zb_uint8_t param)
{
    zb_mlme_reset_request_t *reset_req = ZB_BUF_GET_PARAM(param, zb_mlme_reset_request_t);
    reset_req->set_default_pib = 1;

    //TRACE_MSG(TRACE_APP1, "MLME-RESET.request()", (FMT__0));

    ZB_SCHEDULE_CALLBACK(zb_mlme_reset_request, param);
}


static void test_set_ieee_addr(zb_uint8_t param)
{
    zb_mlme_set_request_t *req;

    //TRACE_MSG(TRACE_APP1, "MLME-SET.request() mac IEEE addr", (FMT__0));

    req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_ieee_addr_t));
    ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_ieee_addr_t));

    req->pib_attr = ZB_PIB_ATTRIBUTE_EXTEND_ADDRESS;
    req->pib_length = sizeof(zb_ieee_addr_t);
    ZB_MEMCPY((zb_uint8_t *)(req + 1), g_ieee_addr_dut_ffd1, sizeof(zb_ieee_addr_t));

    req->confirm_cb_u.cb = test_mlme_scan_request;
    zb_mlme_set_request(param);
}


static void test_mlme_scan_request(zb_uint8_t param)
{

    TRACE_MSG(TRACE_APP1, "MLME-SCAN.request() - Orphan scan", (FMT__0));

    ZB_MLME_BUILD_SCAN_REQUEST(param, TEST_PAGE, ZB_TRANSCEIVER_ALL_CHANNELS_MASK, TEST_SCAN_TYPE, TEST_SCAN_DURATION);
    ZB_SCHEDULE_CALLBACK(zb_mlme_scan_request, param);
}

/***************** MAC Callbacks *****************/
ZB_MLME_RESET_CONFIRM(zb_uint8_t param)
{
    //TRACE_MSG(TRACE_APP1, "MLME-RESET.confirm()", (FMT__0));

    /* Call the next test step */
    ZB_SCHEDULE_CALLBACK(test_set_ieee_addr, param);
}

ZB_MLME_SCAN_CONFIRM(zb_uint8_t param)
{
    zb_mac_scan_confirm_t *scan_confirm = 0;
    scan_confirm = ZB_BUF_GET_PARAM(param, zb_mac_scan_confirm_t);
    /*
    TRACE_MSG(TRACE_APP1, "MLME-SCAN.confirm()", (FMT__0));
    TRACE_MSG(TRACE_APP1, "Status = %x", (FMT__H, scan_confirm->status));
    TRACE_MSG(TRACE_APP1, "Scan type = %x", (FMT__H, scan_confirm->scan_type));
    TRACE_MSG(TRACE_APP1, "Channel page = %d", (FMT__H, scan_confirm->channel_page));
    TRACE_MSG(TRACE_APP1, "UnscannedChannels = %x", (FMT__H, scan_confirm->unscanned_channels));
    TRACE_MSG(TRACE_APP1, "ResultListSize = %d", (FMT__H, scan_confirm->result_list_size));
    */
    if (scan_confirm->status == MAC_SUCCESS &&
            scan_confirm->scan_type == 0x03 &&
            scan_confirm->channel_page == 0x00 &&
            scan_confirm->result_list_size == 0x00 &&
            scan_confirm->unscanned_channels == 0x00000000)
    {
        zb_osif_led_on(0); // blue
        TRACE_MSG(TRACE_APP1, "Scanning07 success", (FMT__0));
    }
    zb_buf_free(param);
}


/*! @} */
