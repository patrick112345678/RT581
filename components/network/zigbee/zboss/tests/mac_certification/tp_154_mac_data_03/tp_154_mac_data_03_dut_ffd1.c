/* ZBOSS Zigbee software protocol stack
 *
 * Copyright (c) 2012-2020 DSR Corporation, Denver CO, USA.
 * http://www.dsr-zboss.com
 * http://www.dsr-corporation.com
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
/* PURPOSE: TP/154/MAC/DATA-03 DUT FFD1
MAC-only build
*/

#define ZB_TEST_NAME TP_154_MAC_DATA_03_DUT_FFD1
#define ZB_TRACE_FILE_ID 57370
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "../common/zb_cert_test_globals.h"
#include "tp_154_mac_data_03_common.h"
#include "zb_mac_globals.h"
#include "zb_osif.h"

#ifndef ZB_MULTI_TEST
#define USE_ZB_MCPS_DATA_CONFIRM
#define USE_ZB_MCPS_DATA_INDICATION
#define USE_ZB_MLME_RESET_CONFIRM
#define USE_ZB_MLME_START_CONFIRM
#include "zb_mac_only_stubs.h"
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_dut_ffd1 = TEST_DUT_FFD1_MAC_ADDRESS;
static zb_ieee_addr_t g_ieee_addr_th_ffd0 = TEST_TH_FFD0_MAC_ADDRESS;

static void test_started_cb(zb_uint8_t param);
static void test_mlme_reset_request(zb_uint8_t param);
static void test_set_ieee_addr(zb_uint8_t param);
static void test_set_short_addr(zb_uint8_t param);
static void test_set_pan_id(zb_uint8_t param);
static void test_set_rx_on_when_idle(zb_uint8_t param);
static void test_mlme_start_request(zb_uint8_t param);

static zb_uint8_t rfd_test_step = TEST_STEP_INITIAL;

zb_mac_test3_t test_result3;
extern zb_step_status_t step_state;

MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("tp_154_mac_data_03_dut_ffd1");

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

    req->confirm_cb_u.cb = test_set_short_addr;
    zb_mlme_set_request(param);
}

static void test_set_short_addr(zb_uint8_t param)
{
    zb_mlme_set_request_t *req;
    zb_uint16_t short_addr = TEST_DUT_FFD1_SHORT_ADDRESS;

    //TRACE_MSG(TRACE_APP1, "MLME-SET.request() mac short addr", (FMT__0));

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
    zb_uint16_t panid = TEST_DUT_FFD1_PAN_ID;

    //TRACE_MSG(TRACE_APP1, "MLME-SET.request() PANID", (FMT__0));

    req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));
    ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));

    req->pib_attr = ZB_PIB_ATTRIBUTE_PANID;
    req->pib_length = sizeof(zb_uint16_t);
    ZB_MEMCPY((zb_uint8_t *)(req + 1), &panid, sizeof(zb_uint16_t));

    req->confirm_cb_u.cb = test_set_rx_on_when_idle;
    zb_mlme_set_request(param);
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
        ZB_SCHEDULE_CALLBACK(test_mlme_start_request, param);
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

static void test_set_rx_on_when_idle(zb_uint8_t param)
{
    zb_mlme_set_request_t *req;
    zb_uint8_t rx_on_when_idle = TEST_DUT_FFD1_RX_ON_WHEN_IDLE;

    //TRACE_MSG(TRACE_APP1, "MLME-SET.request() RxOnWhenIdle", (FMT__0));

    req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));
    ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));

    req->pib_attr = ZB_PIB_ATTRIBUTE_RX_ON_WHEN_IDLE;
    req->pib_length = sizeof(zb_uint8_t);
    ZB_MEMCPY((zb_uint8_t *)(req + 1), &rx_on_when_idle, sizeof(zb_uint8_t));

#if CHANNEL_CHANGE
    req->confirm_cb_u.cb = test_get_command;
#else
    req->confirm_cb_u.cb = test_mlme_start_request;
#endif
    zb_mlme_set_request(param);
}

static void test_mlme_start_request(zb_uint8_t param)
{
    zb_mlme_start_req_t *req = ZB_BUF_GET_PARAM(param, zb_mlme_start_req_t);

    //TRACE_MSG(TRACE_APP1, "MLME-START.request()", (FMT__0));

    ZB_BZERO(req, sizeof(zb_mlme_start_req_t));

    req->pan_id = TEST_DUT_FFD1_PAN_ID;
    req->logical_channel = TEST_CHANNEL;
    req->channel_page = TEST_PAGE;
    req->pan_coordinator = 1;      /* will be coordinator */
    req->coord_realignment = 0;
    req->beacon_order = ZB_TURN_OFF_ORDER;
    req->superframe_order = ZB_TURN_OFF_ORDER;

    ZB_SCHEDULE_CALLBACK(zb_mlme_start_request, param);
}

static void test_mcps_data_request(zb_uint8_t param)
{

    zb_mcps_data_req_params_t *data_req;
    zb_uint8_t *msdu;
    zb_uint8_t i;
    zb_uint8_t msdu_length = TEST_MSDU_LENGTH;

    rfd_test_step++;
    TRACE_MSG(TRACE_APP1, "Next: MCPS-DATA.request test_step %hd", (FMT__H, rfd_test_step));

    msdu = zb_buf_initial_alloc(param, msdu_length);
    for (i = 0; i < msdu_length; i++)
    {
        msdu[i] = i;
    }

    data_req = ZB_BUF_GET_PARAM(param, zb_mcps_data_req_params_t);
    ZB_BZERO(data_req, sizeof(zb_mcps_data_req_params_t));
    //TH & DUT should be reserved
    switch (rfd_test_step)
    {
    case  TH2DUT_SHORT2SHORT_UNICAST_NOACK:
        //TRACE_MSG(TRACE_APP1, "TH2DUT_SHORT2SHORT_UNICAST_NOACK", (FMT__0));
        data_req->src_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        data_req->src_addr.addr_short = TEST_DUT_FFD1_SHORT_ADDRESS;
        data_req->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        data_req->dst_addr.addr_short = TEST_TH_FFD0_SHORT_ADDRESS;
        data_req->dst_pan_id = TEST_TH_FFD0_PAN_ID;
        data_req->msdu_handle = 0x0A;
        data_req->tx_options = 0x00;
        break;

    case  TH2DUT_SHORT2SHORT_UNICAST_ACK:
        //TRACE_MSG(TRACE_APP1, "TH2DUT_SHORT2SHORT_UNICAST_ACK", (FMT__0));
        data_req->src_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        data_req->src_addr.addr_short = TEST_DUT_FFD1_SHORT_ADDRESS;
        data_req->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        data_req->dst_addr.addr_short = TEST_TH_FFD0_SHORT_ADDRESS;
        data_req->dst_pan_id = TEST_TH_FFD0_PAN_ID;
        data_req->msdu_handle = 0x0A;
        data_req->tx_options = 0x01;
        break;

    case  TH2DUT_SHORT2EXT_UNICAST_ACK:
        //TRACE_MSG(TRACE_APP1, "TH2DUT_SHORT2EXT_UNICAST_ACK", (FMT__0));
        data_req->src_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        data_req->src_addr.addr_short = TEST_DUT_FFD1_SHORT_ADDRESS;
        data_req->dst_addr_mode = ZB_ADDR_64BIT_DEV;
        ZB_IEEE_ADDR_COPY(data_req->dst_addr.addr_long, g_ieee_addr_th_ffd0);
        data_req->dst_pan_id = TEST_TH_FFD0_PAN_ID;
        data_req->msdu_handle = 0x0C;
        data_req->tx_options = 0x01;
        break;

    case  TH2DUT_EXT2SHORT_UNICAST_ACK:
        //TRACE_MSG(TRACE_APP1, "TH2DUT_EXT2SHORT_UNICAST_ACK", (FMT__0));
        data_req->src_addr_mode = ZB_ADDR_64BIT_DEV;
        ZB_IEEE_ADDR_COPY(data_req->src_addr.addr_long, g_ieee_addr_dut_ffd1);
        data_req->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        data_req->dst_addr.addr_short = TEST_TH_FFD0_SHORT_ADDRESS;
        data_req->dst_pan_id = TEST_TH_FFD0_PAN_ID;
        data_req->msdu_handle = 0x0C;
        data_req->tx_options = 0x01;
        break;

    case  TH2DUT_EXT2EXT_UNICAST_ACK:
        //TRACE_MSG(TRACE_APP1, "TH2DUT_EXT2EXT_UNICAST_ACK", (FMT__0));
        data_req->src_addr_mode = ZB_ADDR_64BIT_DEV;
        ZB_IEEE_ADDR_COPY(data_req->src_addr.addr_long, g_ieee_addr_dut_ffd1);
        data_req->dst_addr_mode = ZB_ADDR_64BIT_DEV;
        ZB_IEEE_ADDR_COPY(data_req->dst_addr.addr_long, g_ieee_addr_th_ffd0);
        data_req->dst_pan_id = TEST_TH_FFD0_PAN_ID;
        data_req->msdu_handle = 0x0C;
        data_req->tx_options = 0x01;
        break;

    case  TH2DUT_EXT2BROADCAST:
        //TRACE_MSG(TRACE_APP1, "TH2DUT_EXT2BROADCAST", (FMT__0));
        data_req->src_addr_mode = ZB_ADDR_64BIT_DEV;
        ZB_IEEE_ADDR_COPY(data_req->src_addr.addr_long, g_ieee_addr_dut_ffd1);
        data_req->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        data_req->dst_addr.addr_short = 0xffff;
        data_req->dst_pan_id = TEST_TH_FFD0_PAN_ID;
        data_req->msdu_handle = 0x0C;
        data_req->tx_options = 0x00;
        break;

    case  TH2DUT_SHORT2BROADCAST:
        //TRACE_MSG(TRACE_APP1, "TH2DUT_SHORT2BROADCAST", (FMT__0));
        data_req->src_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        data_req->src_addr.addr_short = TEST_DUT_FFD1_SHORT_ADDRESS;
        data_req->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        data_req->dst_addr.addr_short = 0xffff;
        data_req->dst_pan_id = TEST_TH_FFD0_PAN_ID;
        data_req->msdu_handle = 0x0B;
        data_req->tx_options = 0x00;
        break;

    case  TH2DUT_SHORT2BROADCAST_BCAST_PANID:
        //TRACE_MSG(TRACE_APP1, "TH2DUT_SHORT2BROADCAST_BCAST_PANID", (FMT__0));
        data_req->src_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        data_req->src_addr.addr_short = TEST_DUT_FFD1_SHORT_ADDRESS;
        data_req->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        data_req->dst_addr.addr_short = 0xffff;
        data_req->dst_pan_id = 0xFFFF;
        data_req->msdu_handle = 0x0B;
        data_req->tx_options = 0x00;
        break;
    }

    ZB_SCHEDULE_CALLBACK(zb_mcps_data_request, param);
}

/***************** MAC Callbacks *****************/
ZB_MLME_RESET_CONFIRM(zb_uint8_t param)
{
    //TRACE_MSG(TRACE_APP1, "MLME-RESET.confirm()", (FMT__0));

    /* Call the next test step */
    ZB_SCHEDULE_CALLBACK(test_set_ieee_addr, param);
}

ZB_MLME_START_CONFIRM(zb_uint8_t param)
{
    //TRACE_MSG(TRACE_APP1, "MLME-START.confirm()", (FMT__0));
    //rfd_test_step++;
    ZB_SCHEDULE_CALLBACK(test_mcps_data_request, param);
}

ZB_MCPS_DATA_CONFIRM(zb_uint8_t param)
{
    zb_mcps_data_confirm_params_t data_conf_params = *ZB_BUF_GET_PARAM(param, zb_mcps_data_confirm_params_t);
    if (data_conf_params.status == 0) //h~k
    {
        if (data_conf_params.src_addr_mode == 0x02)
        {
            if (data_conf_params.dst_addr_mode == 0x02) //short to short
            {
                if (data_conf_params.dst_addr.addr_short == 0xFFFF) //short to broadcast
                {
                    if (rfd_test_step == TH2DUT_SHORT2BROADCAST)
                    {
                        test_result3.dut2th.g = 1;
                        TRACE_MSG(TRACE_APP1, "MCPS-DATA.confirm success. test_step %hd", (FMT__H, rfd_test_step));
                    }
                    else
                    {
                        test_result3.dut2th.g2 = 1;
                        TRACE_MSG(TRACE_APP1, "MCPS-DATA.confirm success. test_step %hd", (FMT__H, rfd_test_step));
                    }
                }
                else
                {
                    if (data_conf_params.tx_options == 0x00) //no ack
                    {
                        test_result3.dut2th.a = 1;
                        TRACE_MSG(TRACE_APP1, "MCPS-DATA.confirm success. test_step %hd", (FMT__H, rfd_test_step));
                    }
                    else                                    //ack
                    {
                        test_result3.dut2th.b = 1;
                        TRACE_MSG(TRACE_APP1, "MCPS-DATA.confirm success. test_step %hd", (FMT__H, rfd_test_step));
                    }
                }
            }
            else if (data_conf_params.dst_addr_mode == 0x03) //short to ex
            {
                test_result3.dut2th.c = 1;
                TRACE_MSG(TRACE_APP1, "MCPS-DATA.confirm success. test_step %hd", (FMT__H, rfd_test_step));
            }
        }
        else if (data_conf_params.src_addr_mode == 0x03)
        {
            if (data_conf_params.dst_addr_mode == 0x02) //ex to short
            {
                if (data_conf_params.dst_addr.addr_short == 0xFFFF)
                {
                    test_result3.dut2th.f = 1;
                    TRACE_MSG(TRACE_APP1, "MCPS-DATA.confirm success. test_step %hd", (FMT__H, rfd_test_step));
                }
                else
                {
                    test_result3.dut2th.d = 1;
                    TRACE_MSG(TRACE_APP1, "MCPS-DATA.confirm success. test_step %hd", (FMT__H, rfd_test_step));
                }
            }
            else if (data_conf_params.dst_addr_mode == 0x03) //ex to ex
            {
                test_result3.dut2th.e = 1;
                TRACE_MSG(TRACE_APP1, "MCPS-DATA.confirm success. test_step %hd", (FMT__H, rfd_test_step));
            }
        }
    }

    if (rfd_test_step < TH2DUT_SHORT2BROADCAST_BCAST_PANID)
    {
        ZB_SCHEDULE_CALLBACK(test_mcps_data_request, param);
    }
    else
    {
        zb_buf_free(param);
    }
}

void test_final_result3(void)
{
    if (test_result3.dut2th.a == ZB_TRUE &&
            test_result3.dut2th.b == ZB_TRUE &&
            test_result3.dut2th.c == ZB_TRUE &&
            test_result3.dut2th.d == ZB_TRUE &&
            test_result3.dut2th.e == ZB_TRUE &&
            test_result3.dut2th.f == ZB_TRUE &&
            test_result3.dut2th.g == ZB_TRUE &&
            test_result3.dut2th.g2 == ZB_TRUE &&
            test_result3.th2dut.h == ZB_TRUE &&
            test_result3.th2dut.i == ZB_TRUE &&
            test_result3.th2dut.j == ZB_TRUE &&
            test_result3.th2dut.k == ZB_TRUE &&
            test_result3.th2dut.l == ZB_TRUE &&
            test_result3.th2dut.m == ZB_TRUE &&
            test_result3.th2dut.n == ZB_TRUE &&
            test_result3.th2dut.n2 == ZB_TRUE)
    {
        zb_osif_led_on(0);
        TRACE_MSG(TRACE_APP1, "Data03 completely success.", (FMT__0));
    }
}

ZB_MCPS_DATA_INDICATION(zb_uint8_t param)
{
    zb_mac_mhr_t mac_hdr;
    zb_uint_t mhr_size = zb_parse_mhr(&mac_hdr, param);
    rfd_test_step++;
    zb_uint8_t fc1 = mac_hdr.frame_control[1];
    if (fc1 / 16 < 0x0C) // short to ~
    {
        if (fc1 % 16 < 0x0C) // s2s
        {
            if (rfd_test_step == (TH2DUT_SHORT2BROADCAST_BCAST_PANID + 1))
            {
                test_result3.th2dut.h = 1;
                TRACE_MSG(TRACE_APP1, "MCPS-DATA.indication success. test_step %hd", (FMT__H, rfd_test_step));
            }
            else if (rfd_test_step == (TH2DUT_SHORT2BROADCAST_BCAST_PANID + 2))
            {
                test_result3.th2dut.i = 1;
                TRACE_MSG(TRACE_APP1, "MCPS-DATA.indication success. test_step %hd", (FMT__H, rfd_test_step));
            }
            else if (rfd_test_step == (TH2DUT_SHORT2BROADCAST_BCAST_PANID + 7))
            {
                test_result3.th2dut.n = 1;
                TRACE_MSG(TRACE_APP1, "MCPS-DATA.indication success. test_step %hd", (FMT__H, rfd_test_step));
            }
            else
            {
                test_result3.th2dut.n2 = 1;
                TRACE_MSG(TRACE_APP1, "MCPS-DATA.indication success. test_step %hd", (FMT__H, rfd_test_step));
                test_final_result3();
            }
        }
        else                // s2ex
        {
            test_result3.th2dut.j = 1;
            TRACE_MSG(TRACE_APP1, "MCPS-DATA.indication success. test_step %hd", (FMT__H, rfd_test_step));
        }
    }
    else              // ex to ~
    {
        if (fc1 % 16 < 0x0C) // ex2s
        {
            if (rfd_test_step == (TH2DUT_SHORT2BROADCAST_BCAST_PANID + 4))
            {
                test_result3.th2dut.k = 1;
                TRACE_MSG(TRACE_APP1, "MCPS-DATA.indication success. test_step %hd", (FMT__H, rfd_test_step));
            }
            else
            {
                test_result3.th2dut.m = 1;
                TRACE_MSG(TRACE_APP1, "MCPS-DATA.indication success. test_step %hd", (FMT__H, rfd_test_step));
            }
        }
        else                // ex2ex
        {
            test_result3.th2dut.l = 1;
            TRACE_MSG(TRACE_APP1, "MCPS-DATA.indication success. test_step %hd", (FMT__H, rfd_test_step));
        }
    }
    zb_buf_free(param);
}

/*! @} */
