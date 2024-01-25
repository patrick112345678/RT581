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
/*  PURPOSE: TP/154/MAC/ACK-FRAME-DELIVERY-01 TH FFD1 implementation
*/

#define ZB_TEST_NAME TP_154_MAC_ACK_FRAME_DELIVERY_01_TH_FFD1
#define ZB_TRACE_FILE_ID 57458

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "../common/zb_cert_test_globals.h"
#include "tp_154_mac_ack_frame_delivery_01_common.h"

#ifndef ZB_MULTI_TEST

#define USE_ZB_MLME_RESET_CONFIRM
#define USE_ZB_MLME_ASSOCIATE_CONFIRM
#define USE_ZB_MCPS_DATA_INDICATION

#include "zb_mac_only_stubs.h"

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif // #ifndef ZB_COORDINATOR_ROLE

#endif // #ifndef ZB_MULTI_TEST

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr = TEST_TH_FFD1_IEEE_ADDR;

static void test_started_cb(zb_uint8_t unused);
static void test_mlme_reset_request(zb_uint8_t param);
static void test_set_ieee_addr(zb_uint8_t param);
static void test_associate_request(zb_uint8_t param);
static void test_toggle_rx_on_when_idle_confirm(zb_uint8_t param);
static void test_toggle_no_auto_ack(zb_uint8_t param);
static void test_toggle_no_auto_ack_confirm(zb_uint8_t param);

static zb_uint8_t g_th_test_step = TEST_STEP_TH_INITIAL;
static zb_uint8_t g_rx_on_when_idle = ZB_FALSE;
static zb_uint8_t g_no_auto_ack = ZB_TRUE;

MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("tp_154_mac_ack_frame_delivery_01_th_ffd1");

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

    req->confirm_cb_u.cb = test_associate_request;
    zb_mlme_set_request(param);
}

static void test_associate_request(zb_uint8_t param)
{
    zb_uint16_t addr = TEST_DUT_FFD0_SHORT_ADDRESS;
    ZB_MLME_BUILD_ASSOCIATE_REQUEST(param,
                                    TEST_PAGE,
                                    TEST_CHANNEL,
                                    TEST_PAN_ID,
                                    ZB_ADDR_16BIT_DEV_OR_BROADCAST,
                                    &addr,
                                    TEST_ASSOCIATION_CAP_INFO);
    ZB_SCHEDULE_CALLBACK(zb_mlme_associate_request, param);
}

static void test_toggle_rx_on_when_idle(zb_uint8_t param)
{
    zb_mlme_set_request_t *req;
    zb_uint8_t rx_on_when_idle = g_rx_on_when_idle;

    g_rx_on_when_idle = !g_rx_on_when_idle;
    g_th_test_step++;

    TRACE_MSG(TRACE_APP1, "MLME-SET.request() RxOnWhenIdle %hd", (FMT__H, rx_on_when_idle));

    req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));
    ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));

    req->pib_attr = ZB_PIB_ATTRIBUTE_RX_ON_WHEN_IDLE;
    req->pib_length = sizeof(zb_uint8_t);
    *(zb_uint8_t *)(req + 1) = rx_on_when_idle;

    req->confirm_cb_u.cb = test_toggle_rx_on_when_idle_confirm;
    zb_mlme_set_request(param);
}

static void test_toggle_rx_on_when_idle_confirm(zb_uint8_t param)
{
    if (g_th_test_step == TEST_STEP_TH_RX_OFF_WHEN_IDLE1)
    {
        ZB_SCHEDULE_ALARM(test_toggle_no_auto_ack, param, TEST_DATA_FRAME_PERIOD * 3 / 2);
    }
    else
    {
        zb_buf_free(param);
    }
}

static void test_toggle_no_auto_ack(zb_uint8_t param)
{
    zb_mlme_set_request_t *req;
    zb_uint8_t no_auto_ack = g_no_auto_ack;

    g_no_auto_ack = !g_no_auto_ack;
    g_th_test_step++;

    TRACE_MSG(TRACE_APP1, "MLME-SET.request() NoAutoAck %hd", (FMT__H, no_auto_ack));

    req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));
    ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));

    req->pib_attr = ZB_PIB_ATTRIBUTE_NO_AUTO_ACK;
    req->pib_length = sizeof(zb_uint8_t);
    *(zb_uint8_t *)(req + 1) = no_auto_ack;

    req->confirm_cb_u.cb = test_toggle_no_auto_ack_confirm;
    zb_mlme_set_request(param);
}

static void test_toggle_no_auto_ack_confirm(zb_uint8_t param)
{
    if (g_th_test_step == TEST_STEP_TH_DISABLE_AUTO_ACK1 ||
            g_th_test_step == TEST_STEP_TH_ENABLE_AUTO_ACK ||
            g_th_test_step == TEST_STEP_TH_DISABLE_AUTO_ACK2)
    {
        test_toggle_rx_on_when_idle(param);
    }
    else
    {
        zb_buf_free(param);
    }
}

/***************** MAC Callbacks *****************/
ZB_MLME_RESET_CONFIRM(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, "MLME-RESET.confirm()", (FMT__0));

    /* Call the next test step */
    ZB_SCHEDULE_CALLBACK(test_set_ieee_addr, param);
}

ZB_MLME_ASSOCIATE_CONFIRM(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, "MLME-ASSOCIATE.confirm()", (FMT__0));

    ZB_SCHEDULE_CALLBACK(test_toggle_no_auto_ack, param);
}

ZB_MCPS_DATA_INDICATION(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, "MCPS-DATA.indication()", (FMT__0));

    ZB_SCHEDULE_ALARM(test_toggle_no_auto_ack, param, TEST_DATA_FRAME_PERIOD / 2);
}

/*! @} */
