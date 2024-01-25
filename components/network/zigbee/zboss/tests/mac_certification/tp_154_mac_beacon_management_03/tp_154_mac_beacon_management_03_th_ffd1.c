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
/*  PURPOSE: TP/154/MAC/BEACON-MANAGEMENT-03 TH FFD1 implementation
*/

#define ZB_TEST_NAME TP_154_MAC_BEACON_MANAGEMENT_03_TH_FFD1
#define ZB_TRACE_FILE_ID 57297
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "../common/zb_cert_test_globals.h"
#include "tp_154_mac_beacon_management_03_common.h"

#ifndef ZB_MULTI_TEST
#define USE_ZB_MLME_RESET_CONFIRM
#define USE_ZB_MLME_SCAN_CONFIRM
#define USE_ZB_MLME_BEACON_NOTIFY_INDICATION
#include "zb_mac_only_stubs.h"
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr = TEST_TH_FFD1_IEEE_ADDR;

static void test_started_cb(zb_uint8_t unused);
static void test_mlme_reset_request(zb_uint8_t param);
static void test_set_ieee_addr(zb_uint8_t param);
static void test_mlme_scan_request(zb_uint8_t param);

MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("tp_154_mac_beacon_management_03_th_ffd1");

    ZB_SCHEDULE_CALLBACK(test_started_cb, 0);
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

    TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));

    zb_buf_get_out_delayed(test_mlme_reset_request);
}


static void test_mlme_reset_request(zb_uint8_t param)
{
    zb_mlme_reset_request_t *reset_req = ZB_BUF_GET_PARAM(param, zb_mlme_reset_request_t);
    reset_req->set_default_pib = 1;

    TRACE_MSG(TRACE_APP1, "MLME-RESET.request()", (FMT__0));

    ZB_SCHEDULE_CALLBACK(zb_mlme_reset_request, param);
}

static void test_set_ieee_addr(zb_uint8_t param)
{
    zb_mlme_set_request_t *req;

    TRACE_MSG(TRACE_APP1, "MLME-SET.request() mac IEEE addr", (FMT__0));

    req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_ieee_addr_t));
    ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_ieee_addr_t));

    req->pib_attr = ZB_PIB_ATTRIBUTE_EXTEND_ADDRESS;
    req->pib_length = sizeof(zb_ieee_addr_t);
    ZB_MEMCPY((zb_uint8_t *)(req + 1), g_ieee_addr, sizeof(zb_ieee_addr_t));

    req->confirm_cb_u.cb = test_mlme_scan_request;
    zb_mlme_set_request(param);
}

static void test_mlme_scan_request(zb_uint8_t param)
{

    TRACE_MSG(TRACE_APP1, "MLME-SCAN.request()", (FMT__0));


    ZB_MLME_BUILD_SCAN_REQUEST(param, TEST_PAGE, TEST_CHANNEL_MASK, TEST_SCAN_TYPE, TEST_SCAN_DURATION);
    ZB_SCHEDULE_CALLBACK(zb_mlme_scan_request, param);
}

/***************** MAC Callbacks *****************/
ZB_MLME_RESET_CONFIRM(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, "MLME-RESET.confirm()", (FMT__0));

    /* Call the next test step */
    ZB_SCHEDULE_CALLBACK(test_set_ieee_addr, param);
}

ZB_MLME_BEACON_NOTIFY_INDICATION(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, "MLME-BEACON-NOTIFY.indication()", (FMT__0));

    zb_buf_free(param);
}

ZB_MLME_SCAN_CONFIRM(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, "MLME-SCAN.confirm()", (FMT__0));

    zb_buf_free(param);
}

/*! @} */
