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
/* PURPOSE: TP/154/MAC/SCANNING-06 TH FFD5
MAC-only build
*/

#define ZB_TEST_NAME TP_154_MAC_SCANNING_06_TH_FFD5
#define ZB_TRACE_FILE_ID 57240
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "../common/zb_cert_test_globals.h"
#include "tp_154_mac_scanning_06_common.h"

#ifndef ZB_MULTI_TEST
#define USE_ZB_MLME_RESET_CONFIRM
#define USE_ZB_MLME_START_CONFIRM
#include "zb_mac_only_stubs.h"
#endif

#define TEST_TH_FFD_NUMBER          5
/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_th_ffd;
#define TEST_PAN_ID                 TEST_TH_FFD_NUMBER

static void test_started_cb(zb_uint8_t param);
static void test_mlme_reset_request(zb_uint8_t param);
static void test_set_ieee_addr(zb_uint8_t param);
static void test_set_short_addr(zb_uint8_t param);
static void test_set_pan_id(zb_uint8_t param);
static void test_set_rx_on_when_idle(zb_uint8_t param);
static void test_set_association_permit(zb_uint8_t param);
static void test_mlme_start_request(zb_uint8_t param);

MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("tp_154_mac_scanning_06_th_ffd5");

    ZB_SCHEDULE_CALLBACK(test_started_cb, 0);

    ZB_MEMSET(g_ieee_addr_th_ffd, TEST_TH_FFD_NUMBER, sizeof(zb_ieee_addr_t));
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

/***************** Test functions *****************/
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
    ZB_MEMCPY((zb_uint8_t *)(req + 1), g_ieee_addr_th_ffd, sizeof(zb_ieee_addr_t));

    req->confirm_cb_u.cb = test_set_short_addr;
    zb_mlme_set_request(param);
}


static void test_set_short_addr(zb_uint8_t param)
{
    zb_mlme_set_request_t *req;
    zb_uint16_t short_addr = 0;

    TRACE_MSG(TRACE_APP1, "MLME-SET.request() mac short addr", (FMT__0));

    req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));
    ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));

    req->pib_attr = ZB_PIB_ATTRIBUTE_SHORT_ADDRESS;
    req->pib_length = sizeof(zb_uint16_t);
    ZB_MEMCPY((zb_uint8_t *)(req + 1), &short_addr, sizeof(zb_uint16_t));

    req->confirm_cb_u.cb = test_set_pan_id;
    zb_mlme_set_request(param);
}

static void test_set_pan_id(zb_uint8_t param)
{
    zb_mlme_set_request_t *req;
    zb_uint16_t panid = TEST_TH_FFD_NUMBER;

    TRACE_MSG(TRACE_APP1, "MLME-SET.request() PANID", (FMT__0));

    req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));
    ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));

    req->pib_attr = ZB_PIB_ATTRIBUTE_PANID;
    req->pib_length = sizeof(zb_uint16_t);
    ZB_MEMCPY((zb_uint8_t *)(req + 1), &panid, sizeof(zb_uint16_t));

    req->confirm_cb_u.cb = test_set_rx_on_when_idle;
    zb_mlme_set_request(param);
}

static void test_set_rx_on_when_idle(zb_uint8_t param)
{
    zb_mlme_set_request_t *req;
    zb_uint8_t rx_on_when_idle = TEST_RX_ON_WHEN_IDLE;

    TRACE_MSG(TRACE_APP1, "MLME-SET.request() RxOnWhenIdle", (FMT__0));

    req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));
    ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));

    req->pib_attr = ZB_PIB_ATTRIBUTE_RX_ON_WHEN_IDLE;
    req->pib_length = sizeof(zb_uint8_t);
    ZB_MEMCPY((zb_uint8_t *)(req + 1), &rx_on_when_idle, sizeof(zb_uint8_t));

    req->confirm_cb_u.cb = test_set_association_permit;
    zb_mlme_set_request(param);
}

static void test_set_association_permit(zb_uint8_t param)
{
    zb_mlme_set_request_t *req;
    zb_uint8_t association_permit = TEST_ASSOCIATION_PERMIT;

    TRACE_MSG(TRACE_APP1, "MLME-SET.request() Association permit", (FMT__0));

    req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));
    ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));

    req->pib_attr = ZB_PIB_ATTRIBUTE_ASSOCIATION_PERMIT;
    req->pib_length = sizeof(zb_uint8_t);
    ZB_MEMCPY((zb_uint8_t *)(req + 1), &association_permit, sizeof(zb_uint8_t));

    req->confirm_cb_u.cb = test_mlme_start_request;

    zb_mlme_set_request(param);
}

static void test_mlme_start_request(zb_uint8_t param)
{
    zb_mlme_start_req_t *req = ZB_BUF_GET_PARAM(param, zb_mlme_start_req_t);

    TRACE_MSG(TRACE_APP1, "MLME-START.request()", (FMT__0));

    ZB_BZERO(req, sizeof(zb_mlme_start_req_t));

    req->pan_id = TEST_PAN_ID;
    //req->logical_channel = TEST_CHANNEL;
    req->logical_channel = 15;
    req->channel_page = TEST_PAGE;
    req->pan_coordinator = 1;      /* will be coordinator */
    req->coord_realignment = 0;
    req->beacon_order = ZB_TURN_OFF_ORDER;
    req->superframe_order = ZB_TURN_OFF_ORDER;

    ZB_SCHEDULE_CALLBACK(zb_mlme_start_request, param);
}

/***************** MAC Callbacks *****************/
ZB_MLME_RESET_CONFIRM(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, "MLME-RESET.confirm()", (FMT__0));

    /* Call the next test step */
    ZB_SCHEDULE_CALLBACK(test_set_ieee_addr, param);
}

ZB_MLME_START_CONFIRM(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, "MLME-START.confirm()", (FMT__0));

    zb_buf_free(param);
}

/*! @} */
