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
/* PURPOSE: P/154/MAC/DATA-02 TH RFD0
MAC-only build
*/

#define ZB_TEST_NAME TP_154_MAC_DATA_02_TH_RFD1
#define ZB_TRACE_FILE_ID 57202
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "../common/zb_cert_test_globals.h"
#include "tp_154_mac_data_02_common.h"
#include "zb_mac_globals.h"

#ifndef ZB_MULTI_TEST
#define USE_ZB_MLME_RESET_CONFIRM
#define USE_ZB_MLME_ASSOCIATE_CONFIRM
#define USE_ZB_MCPS_DATA_INDICATION
#define USE_ZB_MCPS_DATA_CONFIRM
#define USE_ZB_MLME_POLL_CONFIRM
#include "zb_mac_only_stubs.h"
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_th_rfd1 = TEST_TH_RFD1_MAC_ADDRESS;
static zb_ieee_addr_t g_ieee_addr_dut_ffd0 = TEST_DUT_FFD0_MAC_ADDRESS;

static void test_started_cb(zb_uint8_t param);
static void test_mlme_reset_request(zb_uint8_t param);
static void test_set_ieee_addr(zb_uint8_t param);
static void test_associate_request(zb_uint8_t param);
static void test_poll_alarm(zb_uint8_t param);
static void test_set_short_addr_none(zb_uint8_t param);
static void test_set_short_addr_restore(zb_uint8_t param);
static void test_set_short_addr_req(zb_uint8_t param, zb_uint16_t short_addr, zb_callback_t cb);
static void test_set_short_addr_none_conf(zb_uint8_t param);
static void test_set_short_addr_restore_conf(zb_uint8_t param);

static zb_uint8_t rfd_test_step = TEST_STEP_INITIAL;

static zb_uint8_t rfd_msdu_payload[TEST_MSDU_LENGTH] = TEST_MSDU;

MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("tp_154_mac_data_02_th_rfd1");

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
    ZB_MEMCPY((zb_uint8_t *)(req + 1), g_ieee_addr_th_rfd1, sizeof(zb_ieee_addr_t));

    req->confirm_cb_u.cb = test_associate_request;
    zb_mlme_set_request(param);
}

static void test_set_short_addr_none(zb_uint8_t param)
{
    test_set_short_addr_req(param, ZB_MAC_SHORT_ADDR_NOT_ALLOCATED, test_set_short_addr_none_conf);
}


static void test_set_short_addr_restore(zb_uint8_t param)
{
    test_set_short_addr_req(param, TEST_TH_RFD1_SHORT_ADDRESS, test_set_short_addr_restore_conf);
}


static void test_set_short_addr_req(zb_uint8_t param, zb_uint16_t short_addr, zb_callback_t cb)
{
    zb_mlme_set_request_t *req;

    TRACE_MSG(TRACE_APP1, "MLME-SET.request() mac short addr", (FMT__0));

    req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));
    ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));

    req->pib_attr = ZB_PIB_ATTRIBUTE_SHORT_ADDRESS;
    req->pib_length = sizeof(zb_uint16_t);
    ZB_MEMCPY((zb_uint8_t *)(req + 1), &short_addr, sizeof(zb_uint16_t));

    req->confirm_cb_u.cb = cb;
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

    TRACE_MSG(TRACE_APP1, "MCPS-DATA.request", (FMT__0));
    rfd_test_step++;

    msdu = zb_buf_initial_alloc(param, TEST_MSDU_LENGTH);
    ZB_MEMCPY(msdu, rfd_msdu_payload, TEST_MSDU_LENGTH * sizeof(zb_uint8_t));

    data_req = ZB_BUF_GET_PARAM(param, zb_mcps_data_req_params_t);
    ZB_BZERO(data_req, sizeof(zb_mcps_data_req_params_t));

    switch (rfd_test_step)
    {
    case  TH2DUT_SHORT2SHORT_UNICAST_NOACK:
        TRACE_MSG(TRACE_APP1, "TH2DUT_SHORT2SHORT_UNICAST_NOACK", (FMT__0));
        data_req->src_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        data_req->src_addr.addr_short = TEST_TH_RFD1_SHORT_ADDRESS;
        data_req->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        data_req->dst_addr.addr_short = TEST_DUT_FFD0_SHORT_ADDRESS;
        data_req->dst_pan_id = TEST_PAN_ID;
        data_req->msdu_handle = 0;
        data_req->tx_options = 0x00;
        break;

    case  TH2DUT_SHORT2SHORT_UNICAST_ACK:
        TRACE_MSG(TRACE_APP1, "TH2DUT_SHORT2SHORT_UNICAST_ACK", (FMT__0));
        data_req->src_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        data_req->src_addr.addr_short = TEST_TH_RFD1_SHORT_ADDRESS;
        data_req->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        data_req->dst_addr.addr_short = TEST_DUT_FFD0_SHORT_ADDRESS;
        data_req->dst_pan_id = TEST_PAN_ID;
        data_req->msdu_handle = 0;
        data_req->tx_options = 0x01;
        break;

    case  TH2DUT_SHORT2EXT_UNICAST_ACK:
        TRACE_MSG(TRACE_APP1, "TH2DUT_SHORT2EXT_UNICAST_ACK", (FMT__0));
        data_req->src_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        data_req->src_addr.addr_short = TEST_TH_RFD1_SHORT_ADDRESS;
        data_req->dst_addr_mode = ZB_ADDR_64BIT_DEV;
        ZB_IEEE_ADDR_COPY(data_req->dst_addr.addr_long, g_ieee_addr_dut_ffd0);
        data_req->dst_pan_id = TEST_PAN_ID;
        data_req->msdu_handle = 0;
        data_req->tx_options = 0x01;
        break;

    case  TH2DUT_EXT2SHORT_UNICAST_ACK:
        TRACE_MSG(TRACE_APP1, "TH2DUT_EXT2SHORT_UNICAST_ACK", (FMT__0));
        data_req->src_addr_mode = ZB_ADDR_64BIT_DEV;
        ZB_IEEE_ADDR_COPY(data_req->src_addr.addr_long, g_ieee_addr_th_rfd1);
        data_req->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        data_req->dst_addr.addr_short = TEST_DUT_FFD0_SHORT_ADDRESS;
        data_req->dst_pan_id = TEST_PAN_ID;
        data_req->msdu_handle = 0;
        data_req->tx_options = 0x01;
        break;

    case  TH2DUT_EXT2EXT_UNICAST_ACK:
        TRACE_MSG(TRACE_APP1, "TH2DUT_EXT2EXT_UNICAST_ACK", (FMT__0));
        data_req->src_addr_mode = ZB_ADDR_64BIT_DEV;
        ZB_IEEE_ADDR_COPY(data_req->src_addr.addr_long, g_ieee_addr_th_rfd1);
        data_req->dst_addr_mode = ZB_ADDR_64BIT_DEV;
        ZB_IEEE_ADDR_COPY(data_req->dst_addr.addr_long, g_ieee_addr_dut_ffd0);
        data_req->dst_pan_id = TEST_PAN_ID;
        data_req->msdu_handle = 0;
        data_req->tx_options = 0x01;
        break;

    case  TH2DUT_EXT2BROADCAST:
        TRACE_MSG(TRACE_APP1, "TH2DUT_EXT2BROADCAST", (FMT__0));
        data_req->src_addr_mode = ZB_ADDR_64BIT_DEV;
        ZB_IEEE_ADDR_COPY(data_req->src_addr.addr_long, g_ieee_addr_th_rfd1);
        data_req->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        data_req->dst_addr.addr_short = 0xffff;
        data_req->dst_pan_id = TEST_PAN_ID;
        data_req->msdu_handle = 0;
        data_req->tx_options = 0x00;
        break;

    case  TH2DUT_SHORT2BROADCAST:
        TRACE_MSG(TRACE_APP1, "TH2DUT_SHORT2BROADCAST", (FMT__0));
        data_req->src_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        data_req->src_addr.addr_short = TEST_TH_RFD1_SHORT_ADDRESS;
        data_req->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        data_req->dst_addr.addr_short = 0xffff;
        data_req->dst_pan_id = TEST_PAN_ID;
        data_req->msdu_handle = 0;
        data_req->tx_options = 0x00;
        break;
    }

    ZB_SCHEDULE_CALLBACK(zb_mcps_data_request, param);
}


static void test_poll_alarm(zb_uint8_t param)
{
    zb_mlme_poll_request_t *req = ZB_BUF_GET_PARAM(param, zb_mlme_poll_request_t);
    rfd_test_step++;
    TRACE_MSG(TRACE_APP1, "poll_alarm test_step %hd", (FMT__H, rfd_test_step));
    switch (rfd_test_step)
    {
    case DUT2TH_SHORT2SHORT_INDIRECT_ACK:
        TRACE_MSG(TRACE_APP1, "DUT2TH_SHORT2SHORT_INDIRECT_ACK", (FMT__0));
        req->coord_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        req->coord_addr.addr_short = TEST_DUT_FFD0_SHORT_ADDRESS;
        req->coord_pan_id = TEST_PAN_ID;
        ZB_SCHEDULE_CALLBACK(zb_mlme_poll_request, param);
        break;

    case DUT2TH_SHORT2EXT_INDIRECT_ACK:
        TRACE_MSG(TRACE_APP1, "DUT2TH_SHORT2EXT_INDIRECT_ACK", (FMT__0));
        req->coord_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        req->coord_addr.addr_short = TEST_DUT_FFD0_SHORT_ADDRESS;
        req->coord_pan_id = TEST_PAN_ID;
        ZB_SCHEDULE_CALLBACK(zb_mlme_poll_request, param);
        break;

    case DUT2TH_EXT2SHORT_INDIRECT_ACK:
        TRACE_MSG(TRACE_APP1, "DUT2TH_EXT2SHORT_INDIRECT_ACK", (FMT__0));
        req->coord_addr_mode = ZB_ADDR_64BIT_DEV;
        ZB_IEEE_ADDR_COPY(req->coord_addr.addr_long, g_ieee_addr_dut_ffd0);
        req->coord_pan_id = TEST_PAN_ID;
        ZB_SCHEDULE_CALLBACK(zb_mlme_poll_request, param);
        break;

    case DUT2TH_EXT2EXT_INDIRECT_ACK:
        TRACE_MSG(TRACE_APP1, "DUT2TH_EXT2EXT_INDIRECT_ACK", (FMT__0));
        req->coord_addr_mode = ZB_ADDR_64BIT_DEV;
        ZB_IEEE_ADDR_COPY(req->coord_addr.addr_long, g_ieee_addr_dut_ffd0);
        req->coord_pan_id = TEST_PAN_ID;
        ZB_SCHEDULE_CALLBACK(zb_mlme_poll_request, param);
        break;

    case DUT2TH_SHORT2SHORT_INDIRECT_ACK_NO_POLL_EXPIRE:
        TRACE_MSG(TRACE_APP1, "DUT2TH_SHORT2SHORT_INDIRECT_ACK_NO_POLL_EXPIRE", (FMT__0));
        req->coord_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        req->coord_addr.addr_short = TEST_DUT_FFD0_SHORT_ADDRESS;
        req->coord_pan_id = TEST_PAN_ID;
        ZB_SCHEDULE_ALARM(zb_mlme_poll_request, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(10000));
        break;

    case DUT2TH_SHORT2SHORT_INDIRECT_ACK_NO_POLL_PURGE:
        TRACE_MSG(TRACE_APP1, "DUT2TH_SHORT2SHORT_INDIRECT_ACK_NO_POLL_PURGE", (FMT__0));
        req->coord_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        req->coord_addr.addr_short = TEST_DUT_FFD0_SHORT_ADDRESS;
        req->coord_pan_id = TEST_PAN_ID;
        ZB_SCHEDULE_ALARM(zb_mlme_poll_request, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(5000));
        break;
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
    ZB_SCHEDULE_CALLBACK(test_mcps_data_request, param);
}

ZB_MCPS_DATA_CONFIRM(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, "MCPS-DATA.confirm()", (FMT__0));
    if (rfd_test_step < TH2DUT_SHORT2BROADCAST)
    {
        ZB_SCHEDULE_CALLBACK(test_mcps_data_request, param);
    }
    else
    {
        ZB_SCHEDULE_ALARM(test_poll_alarm, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(500));
    }
}

ZB_MCPS_DATA_INDICATION(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, "MCPS-DATA.indication()", (FMT__0));
    zb_buf_free(param);
}

ZB_MLME_POLL_CONFIRM(zb_uint8_t param)
{
    if (rfd_test_step == DUT2TH_SHORT2SHORT_INDIRECT_ACK ||
            rfd_test_step == DUT2TH_EXT2SHORT_INDIRECT_ACK)
    {
        ZB_SCHEDULE_CALLBACK(test_set_short_addr_none, param);
    }
    else if (rfd_test_step == DUT2TH_SHORT2EXT_INDIRECT_ACK ||
             rfd_test_step == DUT2TH_EXT2EXT_INDIRECT_ACK)
    {
        ZB_SCHEDULE_CALLBACK(test_set_short_addr_restore, param);
    }
    else if (rfd_test_step < DUT2TH_SHORT2SHORT_INDIRECT_ACK_NO_POLL_EXPIRE)
    {
        ZB_SCHEDULE_ALARM(test_poll_alarm, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(100));
    }
    else
    {
        ZB_SCHEDULE_ALARM(test_poll_alarm, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(1000));
    }
}


static void test_set_short_addr_none_conf(zb_uint8_t param)
{
    if (rfd_test_step == DUT2TH_SHORT2SHORT_INDIRECT_ACK ||
            rfd_test_step == DUT2TH_EXT2SHORT_INDIRECT_ACK)
    {
        ZB_SCHEDULE_ALARM(test_poll_alarm, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(100));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Unexpected state %hd param %hd", (FMT__H_H, rfd_test_step, param));
        ZB_ASSERT(0);
        zb_buf_free(param);
    }
}

static void test_set_short_addr_restore_conf(zb_uint8_t param)
{
    if (rfd_test_step == DUT2TH_SHORT2EXT_INDIRECT_ACK ||
            rfd_test_step == DUT2TH_EXT2EXT_INDIRECT_ACK)
    {
        ZB_SCHEDULE_ALARM(test_poll_alarm, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(100));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Unexpected state %hd param %hd", (FMT__H_H,
                  rfd_test_step, param));
        ZB_ASSERT(0);
        zb_buf_free(param);
    }
}


/*! @} */
