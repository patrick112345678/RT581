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
/* PURPOSE: TP/154/MAC/DATA-01 TH RFD0
MAC-only build
*/

#define ZB_TEST_NAME TP_154_MAC_DATA_01_DUT_RFD1
#define ZB_TRACE_FILE_ID 57352
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "../common/zb_cert_test_globals.h"
#include "tp_154_mac_data_01_common.h"
#include "zb_mac_globals.h"
#include "zb_osif.h"

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

static zb_ieee_addr_t g_ieee_addr_dut_rfd1 = TEST_DUT_RFD1_MAC_ADDRESS;
static zb_ieee_addr_t g_ieee_addr_th_ffd0 = TEST_TH_FFD0_MAC_ADDRESS;

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
extern zb_step_status_t step_state;

zb_mac_test_t test_result;

MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("tp_154_mac_data_01_dut_rfd1");

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

#if CHANNEL_CHANGE
static void test_get_command(zb_uint8_t param)
{
    switch (step_state)
    {
    case STARTING:
        printf("TEST_CHANNEL: %d.\n", TEST_CHANNEL);
        printf("Send u/d to increase/decrease channel number. Send p to pass.\n");
        test_step_register(test_get_command, param, 0);
        test_control_start(ZB_TEST_CONTROL_UART, 0);
        break;
    case CHANNEL_INCREASING:
        if (TEST_CHANNEL < 26)
        {
            printf("TEST_CHANNEL++.\n");
            TEST_CHANNEL++;
            printf("Channel has been changed to %d\n", TEST_CHANNEL);
        }
        else
        {
            printf("26 is the max channel number.\n");
        }
        ZB_SCHEDULE_CALLBACK(test_get_command, param);
        step_state = STEP_DONE;
        break;
    case CHANNEL_DECREASING:
        if (TEST_CHANNEL > 11)
        {
            printf("TEST_CHANNEL--.\n");
            TEST_CHANNEL--;
            printf("Channel has been changed to %d\n", TEST_CHANNEL);
        }
        else
        {
            printf("11 is the min channel number.\n");
        }
        ZB_SCHEDULE_CALLBACK(test_get_command, param);
        step_state = STEP_DONE;
        break;
    case TESTING_AND_PRINTING:
        ZB_SCHEDULE_CALLBACK(test_associate_request, param);
        step_state = STEP_DONE;
        break;
    case STEP_DONE:
        test_step_register(test_get_command, param, 0);
        test_control_start(ZB_TEST_CONTROL_UART, 0);
        break;
    default:
        printf("Unused command in this case.\n");
        ZB_SCHEDULE_CALLBACK(test_get_command, param);
        step_state = STEP_DONE;
        break;
    }
}
#endif

static void test_set_ieee_addr(zb_uint8_t param)
{
    zb_mlme_set_request_t *req;

    //TRACE_MSG(TRACE_APP1, "MLME-SET.request() mac IEEE addr", (FMT__0));

    req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_ieee_addr_t));
    ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_ieee_addr_t));

    req->pib_attr = ZB_PIB_ATTRIBUTE_EXTEND_ADDRESS;
    req->pib_length = sizeof(zb_ieee_addr_t);
    ZB_MEMCPY((zb_uint8_t *)(req + 1), g_ieee_addr_dut_rfd1, sizeof(zb_ieee_addr_t));

#if CHANNEL_CHANGE
    req->confirm_cb_u.cb = test_get_command;
#else
    req->confirm_cb_u.cb = test_associate_request;
#endif
    zb_mlme_set_request(param);
}


static void test_set_short_addr_none(zb_uint8_t param)
{
    test_set_short_addr_req(param, ZB_MAC_SHORT_ADDR_NOT_ALLOCATED, test_set_short_addr_none_conf);
}

static void test_set_short_addr_restore(zb_uint8_t param)
{
    test_set_short_addr_req(param, TEST_DUT_RFD1_SHORT_ADDRESS, test_set_short_addr_restore_conf);
}

static void test_set_short_addr_req(zb_uint8_t param, zb_uint16_t short_addr, zb_callback_t cb)
{
    zb_mlme_set_request_t *req;

    //TRACE_MSG(TRACE_APP1, "MLME-SET.request() mac short addr", (FMT__0));

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
    zb_uint16_t addr = TEST_TH_FFD0_SHORT_ADDRESS;
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

    rfd_test_step++;
    TRACE_MSG(TRACE_APP1, "Next: MCPS-DATA.request test_step %hd", (FMT__H, rfd_test_step));

    msdu_length = (rfd_test_step != DUT2TH_SHORT2SHORT_UNICAST_ACK_MAX_PACKET) ? TEST_MSDU_LENGTH : TEST_MAX_MSDU_LENGTH;
    /* Remember the pointer, reassign it to the buffer begin later */
    msdu = zb_buf_initial_alloc(param, msdu_length);
    msdu = zb_buf_begin(param);
    //TRACE_MSG(TRACE_APP1, "rfd_test_step %hd, DUT2TH_SHORT2SHORT_UNICAST_ACK_MAX_PACKET step is %hd, current lenght is %hd",
    //(FMT__H_H_H, rfd_test_step, DUT2TH_SHORT2SHORT_UNICAST_ACK_MAX_PACKET, msdu_length));

    data_req = ZB_BUF_GET_PARAM(param, zb_mcps_data_req_params_t);
    msdu = zb_buf_begin(param);
    ZB_BZERO(data_req, sizeof(zb_mcps_data_req_params_t));

    /* Reassign pointer to the buffer begin */
    msdu = zb_buf_begin(param);

    switch (rfd_test_step)
    {
    case DUT2TH_SHORT2SHORT_UNICAST_NOACK:
        //TRACE_MSG(TRACE_APP1, "DUT2TH_SHORT2SHORT_UNICAST_NOACK", (FMT__0));
        data_req->src_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        data_req->src_addr.addr_short = TEST_DUT_RFD1_SHORT_ADDRESS;
        data_req->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        data_req->dst_addr.addr_short = TEST_TH_FFD0_SHORT_ADDRESS;
        data_req->dst_pan_id = TEST_PAN_ID;
        data_req->msdu_handle = 0;
        data_req->tx_options = 0x00;
        for (i = 0; i < msdu_length; i++)
        {
            msdu[i] = i;
        }
        break;

    case  DUT2TH_SHORT2SHORT_UNICAST_ACK:
        //TRACE_MSG(TRACE_APP1, "DUT2TH_SHORT2SHORT_UNICAST_ACK", (FMT__0));
        data_req->src_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        data_req->src_addr.addr_short = TEST_DUT_RFD1_SHORT_ADDRESS;
        data_req->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        data_req->dst_addr.addr_short = TEST_TH_FFD0_SHORT_ADDRESS;
        data_req->dst_pan_id = TEST_PAN_ID;
        data_req->msdu_handle = 0;
        data_req->tx_options = 0x01;
        for (i = 0; i < msdu_length; i++)
        {
            msdu[i] = i + 0x01;
        }
        break;

    case  DUT2TH_SHORT2EXT_UNICAST_ACK:
        //TRACE_MSG(TRACE_APP1, "DUT2TH_SHORT2EXT_UNICAST_ACK", (FMT__0));
        data_req->src_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        data_req->src_addr.addr_short = TEST_DUT_RFD1_SHORT_ADDRESS;
        data_req->dst_addr_mode = ZB_ADDR_64BIT_DEV;
        ZB_IEEE_ADDR_COPY(data_req->dst_addr.addr_long, g_ieee_addr_th_ffd0);
        data_req->dst_pan_id = TEST_PAN_ID;
        data_req->msdu_handle = 0x09;
        data_req->tx_options = 0x01;
        for (i = 0; i < msdu_length; i++)
        {
            msdu[i] = i + 0x10;
        }
        break;

    case  DUT2TH_EXT2SHORT_UNICAST_ACK:
        //TRACE_MSG(TRACE_APP1, "DUT2TH_EXT2SHORT_UNICAST_ACK", (FMT__0));
        data_req->src_addr_mode = ZB_ADDR_64BIT_DEV;
        ZB_IEEE_ADDR_COPY(data_req->src_addr.addr_long, g_ieee_addr_dut_rfd1);
        data_req->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        data_req->dst_addr.addr_short = TEST_TH_FFD0_SHORT_ADDRESS;
        data_req->dst_pan_id = TEST_PAN_ID;
        data_req->msdu_handle = 0;
        data_req->tx_options = 0x01;
        for (i = 0; i < msdu_length; i++)
        {
            msdu[i] = i + 0x20;
        }
        break;

    case  DUT2TH_EXT2EXT_UNICAST_ACK:
        //TRACE_MSG(TRACE_APP1, "DUT2TH_EXT2EXT_UNICAST_ACK", (FMT__0));
        data_req->src_addr_mode = ZB_ADDR_64BIT_DEV;
        ZB_IEEE_ADDR_COPY(data_req->src_addr.addr_long, g_ieee_addr_dut_rfd1);
        data_req->dst_addr_mode = ZB_ADDR_64BIT_DEV;
        ZB_IEEE_ADDR_COPY(data_req->dst_addr.addr_long, g_ieee_addr_th_ffd0);
        data_req->dst_pan_id = TEST_PAN_ID;
        data_req->msdu_handle = 0;
        data_req->tx_options = 0x01;
        for (i = 0; i < msdu_length; i++)
        {
            msdu[i] = 0x17 + 0x11 * i;
        }
        break;

    case  DUT2TH_EXT2BROADCAST:
        //TRACE_MSG(TRACE_APP1, "DUT2TH_EXT2BROADCAST", (FMT__0));
        data_req->src_addr_mode = ZB_ADDR_64BIT_DEV;
        ZB_IEEE_ADDR_COPY(data_req->src_addr.addr_long, g_ieee_addr_dut_rfd1);
        data_req->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        data_req->dst_addr.addr_short = 0xffff;
        data_req->dst_pan_id = TEST_PAN_ID;
        data_req->msdu_handle = 0;
        data_req->tx_options = 0x00;
        for (i = 0; i < msdu_length; i++)
        {
            msdu[i] = 0x6C + 0x11 * i;
        }
        msdu[msdu_length - 1] -= 0x10;

        break;

    case  DUT2TH_SHORT2BROADCAST:
        //TRACE_MSG(TRACE_APP1, "DUT2TH_SHORT2BROADCAST", (FMT__0));
        data_req->src_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        data_req->src_addr.addr_short = TEST_DUT_RFD1_SHORT_ADDRESS;
        data_req->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        data_req->dst_addr.addr_short = 0xffff;
        data_req->dst_pan_id = TEST_PAN_ID;
        data_req->msdu_handle = 0;
        data_req->tx_options = 0x00;
        for (i = 0; i < msdu_length; i++)
        {
            msdu[i] = i;
        }
        break;

    case  DUT2TH_SHORT2SHORT_UNICAST_ACK_MAX_PACKET:
        //TRACE_MSG(TRACE_APP1, "DUT2TH_SHORT2SHORT_UNICAST_ACK_MAX_PACKET", (FMT__0));
        data_req->src_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        data_req->src_addr.addr_short = TEST_DUT_RFD1_SHORT_ADDRESS;
        data_req->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        data_req->dst_addr.addr_short = TEST_TH_FFD0_SHORT_ADDRESS;
        data_req->dst_pan_id = TEST_PAN_ID;
        data_req->msdu_handle = 0;
        data_req->tx_options = 0x01;
        for (i = 0; i < msdu_length; i++)
        {
            msdu[i] = i;
        }
        break;
    }
    ZB_SCHEDULE_CALLBACK(zb_mcps_data_request, param);
}

static void test_poll_alarm(zb_uint8_t param)
{
    zb_mlme_poll_request_t *req = ZB_BUF_GET_PARAM(param, zb_mlme_poll_request_t);
    rfd_test_step++;
    TRACE_MSG(TRACE_APP1, "Next: poll_alarm test_step %hd", (FMT__H, rfd_test_step));

    switch (rfd_test_step)
    {
    case TH2DUT_SHORT2SHORT_INDIRECT_ACK:
        //TRACE_MSG(TRACE_APP1, "TH2DUT_SHORT2SHORT_INDIRECT_ACK", (FMT__0));
        req->coord_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        req->coord_addr.addr_short = TEST_TH_FFD0_SHORT_ADDRESS;
        req->coord_pan_id = TEST_PAN_ID;
        break;

    case TH2DUT_SHORT2EXT_INDIRECT_ACK:
        //TRACE_MSG(TRACE_APP1, "TH2DUT_SHORT2EXT_INDIRECT_ACK", (FMT__0));
        req->coord_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        req->coord_addr.addr_short = TEST_TH_FFD0_SHORT_ADDRESS;
        req->coord_pan_id = TEST_PAN_ID;
        break;

    case TH2DUT_EXT2SHORT_INDIRECT_ACK:
        //TRACE_MSG(TRACE_APP1, "TH2DUT_EXT2SHORT_INDIRECT_ACK", (FMT__0));
        req->coord_addr_mode = ZB_ADDR_64BIT_DEV;
        ZB_IEEE_ADDR_COPY(req->coord_addr.addr_long, g_ieee_addr_th_ffd0);
        req->coord_pan_id = TEST_PAN_ID;
        break;

    case TH2DUT_EXT2EXT_INDIRECT_ACK:
        //TRACE_MSG(TRACE_APP1, "TH2DUT_EXT2EXT_INDIRECT_ACK", (FMT__0));
        req->coord_addr_mode = ZB_ADDR_64BIT_DEV;
        ZB_IEEE_ADDR_COPY(req->coord_addr.addr_long, g_ieee_addr_th_ffd0);
        req->coord_pan_id = TEST_PAN_ID;
        break;

    case TH2DUT_SHORT2SHORT_INDIRECT_ACK_MAX_PACKET:
        //TRACE_MSG(TRACE_APP1, "TH2DUT_SHORT2SHORT_INDIRECT_ACK_MAX_PACKET", (FMT__0));
        req->coord_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        req->coord_addr.addr_short = TEST_TH_FFD0_SHORT_ADDRESS;
        req->coord_pan_id = TEST_PAN_ID;
        break;
    }

#if UART_CONTROL
    test_step_register(zb_mlme_poll_request, param, 0);
    test_control_start(ZB_TEST_CONTROL_UART, 0);
#else
    ZB_SCHEDULE_CALLBACK(zb_mlme_poll_request, param);
#endif
}

static void test_schedule_poll_alarm(zb_uint8_t param, zb_uint32_t tmo)
{
#ifdef MAC_TEST_ADD_EXT_DELAY
    ZB_SCHEDULE_ALARM(test_poll_alarm, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(10000));
#else
    if (!tmo)
    {
        ZB_SCHEDULE_CALLBACK(test_poll_alarm, param);
    }
    else
    {
        ZB_SCHEDULE_ALARM(test_poll_alarm, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(tmo));
    }
#endif /* MAC_TEST_ADD_DELAY */
}

/***************** MAC Callbacks *****************/
ZB_MLME_RESET_CONFIRM(zb_uint8_t param)
{
    //TRACE_MSG(TRACE_APP1, "MLME-RESET.confirm()", (FMT__0));

    /* Call the next test step */
    ZB_SCHEDULE_CALLBACK(test_set_ieee_addr, param);
}

ZB_MLME_ASSOCIATE_CONFIRM(zb_uint8_t param)
{
    //TRACE_MSG(TRACE_APP1, "MLME-ASSOCIATE.confirm()", (FMT__0));
    ZB_SCHEDULE_CALLBACK(test_mcps_data_request, param);
}

ZB_MCPS_DATA_CONFIRM(zb_uint8_t param)
{
    zb_mcps_data_confirm_params_t data_conf_params = *ZB_BUF_GET_PARAM(param, zb_mcps_data_confirm_params_t);
    TRACE_MSG(TRACE_APP1, "MCPS-DATA.confirm", (FMT__0));
    TRACE_MSG(TRACE_APP1, "msduHandle = %x.", (FMT__H, data_conf_params.msdu_handle));
    TRACE_MSG(TRACE_APP1, "status = %x.", (FMT__H, data_conf_params.status));
    TRACE_MSG(TRACE_APP1, "Timestamp = %lx", (FMT__L, data_conf_params.timestamp));
    if (data_conf_params.status == 0) //h~k
    {
        if (data_conf_params.src_addr_mode == 0x02)
        {
            if (data_conf_params.dst_addr_mode == 0x02) //short to short
            {
                if (data_conf_params.dst_addr.addr_short == 0xFFFF) //short to broadcast
                {
                    test_result.dut2th.g = 1;
                    TRACE_MSG(TRACE_APP1, "Test_step %hd success.", (FMT__H, rfd_test_step));
                }
                else
                {
                    if (data_conf_params.tx_options == 0x00) //no ack
                    {
                        test_result.dut2th.a = 1;
                        TRACE_MSG(TRACE_APP1, "Test_step %hd success.", (FMT__H, rfd_test_step));
                    }
                    else                                    //ack
                    {
                        if (rfd_test_step == DUT2TH_SHORT2SHORT_UNICAST_ACK_MAX_PACKET)
                        {
                            test_result.dut2th.h = 1;
                            TRACE_MSG(TRACE_APP1, "Test_step %hd success.", (FMT__H, rfd_test_step));
                        }
                        else
                        {
                            test_result.dut2th.b = 1;
                            TRACE_MSG(TRACE_APP1, "Test_step %hd success.", (FMT__H, rfd_test_step));
                        }
                    }
                }
            }
            else if (data_conf_params.dst_addr_mode == 0x03) //short to ex
            {
                test_result.dut2th.c = 1;
                TRACE_MSG(TRACE_APP1, "Test_step %hd success.", (FMT__H, rfd_test_step));
            }
        }
        else if (data_conf_params.src_addr_mode == 0x03)
        {
            if (data_conf_params.dst_addr_mode == 0x02) //ex to short
            {
                if (data_conf_params.dst_addr.addr_short == 0xFFFF)
                {
                    test_result.dut2th.f = 1;
                    TRACE_MSG(TRACE_APP1, "Test_step %hd success.", (FMT__H, rfd_test_step));
                }
                else
                {
                    test_result.dut2th.d = 1;
                    TRACE_MSG(TRACE_APP1, "Test_step %hd success.", (FMT__H, rfd_test_step));
                }
            }
            else if (data_conf_params.dst_addr_mode == 0x03) //ex to ex
            {
                test_result.dut2th.e = 1;
                TRACE_MSG(TRACE_APP1, "Test_step %hd success.", (FMT__H, rfd_test_step));
            }
        }
    }

    //TRACE_MSG(TRACE_APP1, "MCPS-DATA.confirm()", (FMT__0));
    if (rfd_test_step < DUT2TH_SHORT2SHORT_UNICAST_ACK_MAX_PACKET)
    {
        ZB_SCHEDULE_CALLBACK(test_mcps_data_request, param);
    }
    else
    {
        //ZB_SCHEDULE_CALLBACK(test_set_short_addr_none, param);
        test_schedule_poll_alarm(param, 500);
    }
}
void test_final_result(void)
{
    if (test_result.dut2th.a == ZB_TRUE &&
            test_result.dut2th.b == ZB_TRUE &&
            test_result.dut2th.c == ZB_TRUE &&
            test_result.dut2th.d == ZB_TRUE &&
            test_result.dut2th.e == ZB_TRUE &&
            test_result.dut2th.f == ZB_TRUE &&
            test_result.dut2th.g == ZB_TRUE &&
            test_result.dut2th.h == ZB_TRUE &&
            test_result.th2dut.i == ZB_TRUE &&
            test_result.th2dut.j == ZB_TRUE &&
            test_result.th2dut.k == ZB_TRUE &&
            test_result.th2dut.l == ZB_TRUE &&
            test_result.th2dut.m == ZB_TRUE)
    {
        zb_osif_led_on(0);
        TRACE_MSG(TRACE_APP1, "Data01 completely success.", (FMT__0));
    }
}
ZB_MCPS_DATA_INDICATION(zb_uint8_t param)
{
    zb_mac_mhr_t mac_hdr;
    zb_uint_t mhr_size = zb_parse_mhr(&mac_hdr, param);
    zb_uint8_t fc1 = mac_hdr.frame_control[1];
    TRACE_MSG(TRACE_APP1, "MCPS-DATA.indication", (FMT__0));
    if (fc1 / 16 < 0x0C) // short to ~
    {
        if (fc1 % 16 < 0x0C) // s2s
        {
            if (rfd_test_step == TH2DUT_SHORT2SHORT_INDIRECT_ACK_MAX_PACKET)
            {
                test_result.th2dut.m = 1;
                TRACE_MSG(TRACE_APP1, "Test_step %hd success.", (FMT__H, rfd_test_step));
                test_final_result();
            }
            else
            {
                test_result.th2dut.i = 1;
                TRACE_MSG(TRACE_APP1, "Test_step %hd success.", (FMT__H, rfd_test_step));
            }
        }
        else                // s2ex
        {
            test_result.th2dut.j = 1;
            TRACE_MSG(TRACE_APP1, "Test_step %hd success.", (FMT__H, rfd_test_step));
        }
    }
    else              // ex to ~
    {
        if (fc1 % 16 < 0x0C) // ex2s
        {
            test_result.th2dut.k = 1;
            TRACE_MSG(TRACE_APP1, "Test_step %hd success.", (FMT__H, rfd_test_step));
        }
        else                // ex2ex
        {
            test_result.th2dut.l = 1;
            TRACE_MSG(TRACE_APP1, "Test_step %hd success.", (FMT__H, rfd_test_step));
        }
    }
    //TRACE_MSG(TRACE_APP1, "MCPS-DATA.indication()", (FMT__0));
    zb_buf_free(param);
}

ZB_MLME_POLL_CONFIRM(zb_uint8_t param)
{
    if (rfd_test_step == TH2DUT_SHORT2SHORT_INDIRECT_ACK ||
            rfd_test_step == TH2DUT_EXT2SHORT_INDIRECT_ACK)
    {
        ZB_SCHEDULE_CALLBACK(test_set_short_addr_none, param);
    }
    else if (rfd_test_step == TH2DUT_SHORT2EXT_INDIRECT_ACK ||
             rfd_test_step == TH2DUT_EXT2EXT_INDIRECT_ACK)
    {
        ZB_SCHEDULE_CALLBACK(test_set_short_addr_restore, param);
    }
    else if (rfd_test_step < TH2DUT_SHORT2SHORT_INDIRECT_ACK_MAX_PACKET)
    {
        test_schedule_poll_alarm(param, 100);
    }
}

static void test_set_short_addr_none_conf(zb_uint8_t param)
{
    if (rfd_test_step == TH2DUT_SHORT2SHORT_INDIRECT_ACK ||
            rfd_test_step == TH2DUT_EXT2SHORT_INDIRECT_ACK)
    {
        test_schedule_poll_alarm(param, 100);
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
    if (rfd_test_step == TH2DUT_SHORT2EXT_INDIRECT_ACK ||
            rfd_test_step == TH2DUT_EXT2EXT_INDIRECT_ACK)
    {
        test_schedule_poll_alarm(param, 100);
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
