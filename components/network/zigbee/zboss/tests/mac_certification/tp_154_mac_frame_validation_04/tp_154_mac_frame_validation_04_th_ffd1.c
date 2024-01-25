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
/* PURPOSE: TP/154/MAC/FRAME-VALIDATION-04 FFD1
MAC-only build
*/

#define ZB_TEST_NAME TP_154_MAC_FRAME_VALIDATION_04_TH_FFD1
#define ZB_TRACE_FILE_ID 57547
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "../common/zb_cert_test_globals.h"
#include "tp_154_mac_frame_validation_04_common.h"

#ifndef ZB_MULTI_TEST
#define USE_ZB_MLME_RESET_CONFIRM
#define USE_ZB_MLME_ASSOCIATE_CONFIRM
#include "zb_mac_only_stubs.h"
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_th_ffd1 = TEST_TH_FFD1_IEEE_ADDRESS;

static void test_started_cb(zb_uint8_t param);
static void test_mlme_reset_request(zb_uint8_t param);
static void test_set_ieee_addr(zb_uint8_t param);
static void test_associate_request(zb_uint8_t param);



MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("tp_154_mac_frame_validation_04_th_ffd1");

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
    ZB_MEMCPY((zb_uint8_t *)(req + 1), g_ieee_addr_th_ffd1, sizeof(zb_ieee_addr_t));

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

static void test_mcps_data_request(zb_uint8_t param)
{

    zb_mcps_data_req_params_t *data_req;
    zb_uint8_t *msdu;
    zb_uint8_t i;
    zb_uint8_t msdu_length;
    TRACE_MSG(TRACE_APP1, "MCPS-DATA.request", (FMT__0));

    msdu_length = TEST_MSDU_LENGTH;
    msdu = zb_buf_initial_alloc(param, msdu_length);
    for (i = 0; i < msdu_length; i++)
    {
        msdu[i] = i;
    }

    data_req = ZB_BUF_GET_PARAM(param, zb_mcps_data_req_params_t);
    ZB_BZERO(data_req, sizeof(zb_mcps_data_req_params_t));

    data_req->src_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
    data_req->src_addr.addr_short = TEST_TH_FFD1_SHORT_ADDRESS;
    data_req->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
    data_req->dst_addr.addr_short = TEST_DUT_FFD0_SHORT_ADDRESS;
    data_req->dst_pan_id = TEST_INVALID_PAN_ID;
    data_req->msdu_handle = 0;
    data_req->tx_options = 0x01;

    ZB_SCHEDULE_CALLBACK(zb_mcps_data_request, param);
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
    ZB_SCHEDULE_ALARM(test_mcps_data_request, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(100));
}

/*! @} */
