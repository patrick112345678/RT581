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
/* PURPOSE: TP/154/PHY24/RECEIVER-05 DUT implementation
MAC-only build
*/

#define ZB_TEST_NAME TP_154_PHY24_RECEIVER_05_DUT
#define ZB_TRACE_FILE_ID 57584
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "../common/zb_cert_test_globals.h"
#include "tp_154_phy24_receiver_05_common.h"

#ifndef ZB_MULTI_TEST
#define USE_ZB_MLME_RESET_CONFIRM
#define USE_ZB_MLME_SCAN_CONFIRM
#include "zb_mac_only_stubs.h"
#endif


static void test_started_cb(zb_uint8_t unused);
static void test_mlme_reset_request(zb_uint8_t param);
static void test_mlme_scan_request(zb_uint8_t param);


static zb_uint8_t g_scan_attempt_number = 0;
static zb_uint8_t g_current_channel = ZB_PAGE0_2_4_GHZ_START_LOGICAL_CHANNEL;
static zb_uint8_t g_current_power_level_offset = 0;

static zb_uint8_t g_scan_results[TEST_SCANS_PER_POWER_LEVEL];
static zb_uint32_t g_unscanned_channel_mask = ZB_TRANSCEIVER_ALL_CHANNELS_MASK;

extern zb_step_status_t step_state;


MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("tp_154_phy24_receiver_05_dut");

    ZB_SCHEDULE_CALLBACK(test_started_cb, 0);

    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);

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

    TRACE_MSG(TRACE_MAC_API1, "Device STARTED OK", (FMT__0));

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

/* Schedule next ED scan or finish the test.
 * Iterations are done over: - all channels in mask,
 *                           - values 0,2,4,...,40 of power_offset
 * 10 ED scans are done on each iteration */

static void test_get_command(zb_uint8_t param)
{
    switch (step_state)
    {
    case STARTING:
        printf("Current channel: 11.\n");
        printf("Send u/d to increase/decrease channel number. Send p to perform ED scan and print ED value at current channel.\n");
        test_step_register(test_get_command, param, 0);
        test_control_start(ZB_TEST_CONTROL_UART, 0);
        break;
    case CHANNEL_INCREASING:
        if (g_current_channel < 26)
        {
            printf("Channel++.\n");
            g_current_channel++;
            printf("Channel has been changed to %d\n", g_current_channel);
        }
        else
        {
            printf("26 is the max channel number.\n");
        }
        ZB_SCHEDULE_CALLBACK(test_get_command, param);
        step_state = STEP_DONE;
        break;
    case CHANNEL_DECREASING:
        if (g_current_channel > 11)
        {
            printf("Channel--.\n");
            g_current_channel--;
            printf("Channel has been changed to %d\n", g_current_channel);
        }
        else
        {
            printf("11 is the min channel number.\n");
        }
        ZB_SCHEDULE_CALLBACK(test_get_command, param);
        step_state = STEP_DONE;
        break;
    case TESTING_AND_PRINTING:
        ZB_SCHEDULE_CALLBACK(test_mlme_scan_request, param);
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

static void test_mlme_scan_request(zb_uint8_t param)
{
    //TRACE_MSG(TRACE_APP1, "ED scan", (FMT__0));
    ZB_MLME_BUILD_SCAN_REQUEST(param, TEST_PAGE, (1L << g_current_channel), ED_SCAN, TEST_SCAN_DURATION);
    ZB_SCHEDULE_CALLBACK(zb_mlme_scan_request, param);
}

/***************** MAC Callbacks *****************/
ZB_MLME_RESET_CONFIRM(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, "MLME-RESET.confirm()", (FMT__0));
    ZB_SCHEDULE_CALLBACK(test_get_command, param);
}
/*
zb_uint8_t test_ed_value_mapping(zb_uint8_t ed_value)
{
    zb_uint8_t ed_show;
    if(ed_value < 37){
        ed_show = ed_value*8/7;
    }
    else if(ed_value < 52){
        ed_show = ed_value*7/6;  // 6/5
    }
    else if(ed_value < 59){
        ed_show = ed_value*63/58;
    }
    else if(ed_value > 70 && ed_value < 82){
        ed_show = ed_value*75/80;
    }
    else if(ed_value > 100){
        ed_show = 100;
    }
    else
    {
        ed_show = ed_value;
    }
    return ed_show;
}
zb_uint8_t test_ed_value_mapping(zb_uint8_t ed_value)
{
    zb_uint8_t ed_show;
    if(ed_value > 66 && ed_value < 78){
        ed_show = ed_value+4;
    }
    if(ed_value > 56 && ed_value < 67){
        ed_show = ed_value+4;
    }
    if(ed_value > 39 && ed_value < 57){
        ed_show = ed_value+2;
    }
    else if(ed_value > 100){
        ed_show = 100;
    }
    else
    {
        ed_show = ed_value;
    }
    return ed_show;
}
*/

zb_uint8_t test_ed_value_mapping(zb_uint8_t ed_value)
{
    zb_uint8_t ed_show;
    zb_uint8_t min_power = 70;
    zb_uint8_t max_power = 4;
    if (ed_value > min_power)
    {
        ed_show = min_power;
    }
    else if (ed_value < max_power)
    {
        ed_show = max_power;
    }
    else
    {
        ed_show = ed_value;
    }
    ed_show = ed_show - max_power;
    ed_show = (min_power - max_power) - ed_show;
    ed_show = ed_show * 0xff / (min_power - max_power);
    return ed_show;
}

ZB_MLME_SCAN_CONFIRM(zb_uint8_t param)
{
    zb_mac_scan_confirm_t *scan_confirm = zb_buf_get_tail(param, sizeof(zb_mac_scan_confirm_t));

    //TRACE_MSG(TRACE_APP1, "MLME-SCAN.confirm(), g_current_channel: %x", (FMT__0, g_current_channel));

    /* Paranoid check: all scans in this test are done on only one channel */
    ZB_ASSERT(scan_confirm->result_list_size == 1);

    g_scan_results[g_scan_attempt_number] = scan_confirm->list.energy_detect[0];
    //TRACE_MSG(TRACE_APP1, "Energy level = %d", (FMT__0, g_scan_results[g_scan_attempt_number]));
    //TRACE_MSG(TRACE_APP1, "Energy level = %d", (FMT__0, test_ed_value_mapping(g_scan_results[g_scan_attempt_number])));

    //TRACE_MSG(TRACE_APP1, "Energy level = %x", (FMT__0, test_ed_value_mapping(g_scan_results[g_scan_attempt_number])));
    printf("Energy level = %d\n", (-1 * g_scan_results[g_scan_attempt_number]));
    ZB_SCHEDULE_CALLBACK(test_get_command, param);
}

