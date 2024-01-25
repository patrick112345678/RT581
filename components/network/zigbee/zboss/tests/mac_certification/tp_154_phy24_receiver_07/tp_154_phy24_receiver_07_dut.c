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
/* PURPOSE: TP/154/PHY24/RECEIVER-07
MAC-only build
*/

#define ZB_TEST_NAME TP_154_PHY24_RECEIVER_07_DUT
#define ZB_TRACE_FILE_ID 57602
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_mac_globals.h"
#include "zb_bufpool.h"
#include "../common/zb_cert_test_globals.h"
#include "tp_154_phy24_receiver_07_common.h"

#ifndef ZB_MULTI_TEST
#define USE_ZB_MLME_RESET_CONFIRM
#include "zb_mac_only_stubs.h"
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static void test_started_cb(zb_uint8_t param);
static void test_mlme_reset_request(zb_uint8_t param);
static void test_set_ieee_addr(zb_uint8_t param);
static void test_set_short_addr(zb_uint8_t param);
static void test_set_pan_id(zb_uint8_t param);
static void test_set_page_req(zb_uint8_t param);
static void test_set_channel_req(zb_uint8_t param);
static void test_get_command(zb_uint8_t param);
static void test_set_channel_conf(zb_uint8_t param);
static void test_perform_cca(zb_uint8_t param);

static zb_ieee_addr_t gs_ieee_addr_dut = TEST_DUT_MAC_ADDRESS;

/* Number of packets sent on operating channel */
static zb_uint8_t gs_current_channel;
extern zb_step_status_t step_state;
extern char uart_command[100];
zb_uint8_t cca_threshold = 68;

MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("tp_154_phy24_receiver_07_dut");

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

    /* Will be incremented in the next call of test_set_channel_req() functions */
    gs_current_channel = TEST_CHANNEL_MIN;

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
    ZB_MEMCPY((zb_uint8_t *)(req + 1), gs_ieee_addr_dut, sizeof(zb_ieee_addr_t));

    req->confirm_cb_u.cb = test_set_short_addr;
    zb_mlme_set_request(param);
}

static void test_set_short_addr(zb_uint8_t param)
{
    zb_mlme_set_request_t *req;
    zb_uint16_t short_addr = TEST_DUT_SHORT_ADDRESS;

    TRACE_MSG(TRACE_APP1, "MLME-SET.request() mac short addr", (FMT__0));

    req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));
    ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));

    req->pib_attr = ZB_PIB_ATTRIBUTE_SHORT_ADDRESS;
    req->pib_length = sizeof(zb_uint16_t);
    ZB_MEMCPY((zb_uint8_t *)(req + 1), &short_addr, sizeof(zb_ieee_addr_t));

    req->confirm_cb_u.cb = test_set_pan_id;
    zb_mlme_set_request(param);
}

static void test_set_pan_id(zb_uint8_t param)
{
    zb_mlme_set_request_t *req;
    zb_uint16_t panid = TEST_PAN_ID;

    TRACE_MSG(TRACE_APP1, "MLME-SET.request() PANID", (FMT__0));

    req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));
    ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));

    req->pib_attr = ZB_PIB_ATTRIBUTE_PANID;
    req->pib_length = sizeof(zb_uint16_t);
    ZB_MEMCPY((zb_uint8_t *)(req + 1), &panid, sizeof(zb_uint16_t));

    req->confirm_cb_u.cb = test_set_page_req;
    zb_mlme_set_request(param);
}

static void test_set_page_req(zb_uint8_t param)
{
    zb_mlme_set_request_t *req;
    zb_uint8_t page = TEST_PAGE;

    TRACE_MSG(TRACE_APP1, "MLME-SET.request() Page %hd", (FMT__H, page));

    req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));
    ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));

    req->pib_attr = ZB_PHY_PIB_CURRENT_PAGE;
    req->pib_length = sizeof(zb_uint8_t);
    ZB_MEMCPY((zb_uint8_t *)(req + 1), &page, sizeof(zb_uint8_t));

    req->confirm_cb_u.cb = test_set_channel_req;
    zb_mlme_set_request(param);
}

static void test_set_channel_req(zb_uint8_t param)
{
    zb_mlme_set_request_t *req;
    zb_uint8_t channel = gs_current_channel;

    TRACE_MSG(TRACE_APP1, "MLME-SET.request() Channel %hd", (FMT__H, channel));

    req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));
    ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));

    req->pib_attr = ZB_PHY_PIB_CURRENT_CHANNEL;
    req->pib_length = sizeof(zb_uint8_t);
    ZB_MEMCPY((zb_uint8_t *)(req + 1), &channel, sizeof(zb_uint8_t));

    req->confirm_cb_u.cb = test_get_command;
    zb_mlme_set_request(param);
}

static void test_get_command(zb_uint8_t param)
{
    if (strstr(&uart_command, "set_ccath") != NULL)
    {
        char val[2] = {uart_command[10], uart_command[11]};
        sscanf(val, "%d", &cca_threshold);
        printf("New cca threshold value: %d.\n", cca_threshold);
        ZB_SCHEDULE_CALLBACK(test_get_command, param);
        step_state = STEP_DONE;
        ZB_BZERO(&uart_command, sizeof(uart_command));
    }
    else
    {
        switch (step_state)
        {
        case STARTING:
            printf("Send u/d to increase/decrease channel number. Send p to perform CCA and print CCA status.\n");
            test_step_register(test_get_command, param, 0);
            test_control_start(ZB_TEST_CONTROL_UART, 0);
            break;
        case CHANNEL_INCREASING:
            if (gs_current_channel < 26)
            {
                printf("Channel++.\n");
                gs_current_channel++;
                ZB_SCHEDULE_CALLBACK(test_set_channel_req, param);
            }
            else
            {
                printf("26 is the max channel number.\n");
                ZB_SCHEDULE_CALLBACK(test_get_command, param);
            }
            step_state = STEP_DONE;
            break;
        case CHANNEL_DECREASING:
            if (gs_current_channel > 11)
            {
                printf("Channel--.\n");
                gs_current_channel--;
                ZB_SCHEDULE_CALLBACK(test_set_channel_req, param);
            }
            else
            {
                printf("11 is the min channel number.\n");
                ZB_SCHEDULE_CALLBACK(test_get_command, param);
            }
            step_state = STEP_DONE;
            break;
        case TESTING_AND_PRINTING:
            ZB_SCHEDULE_CALLBACK(test_perform_cca, param);
            step_state = STEP_DONE;
            break;
        case STEP_DONE:
            test_step_register(test_get_command, param, 0);
            test_control_start(ZB_TEST_CONTROL_UART, 0);
            break;
        default:
            printf("Unused command in this case.\n");
            ZB_SCHEDULE_CALLBACK(test_get_command, param);
            step_state = STARTING;
            break;
        }
    }

}

static void test_set_channel_conf(zb_uint8_t param)
{
    ZB_SCHEDULE_CALLBACK(test_perform_cca, param);
}

static void test_perform_cca(zb_uint8_t param)
{
    zb_bool_t cca_status;
    cca_status = rfb_ctrl.is_channel_free((2405 + 5 * (gs_current_channel - 11)), cca_threshold);

    if (cca_status == ZB_TRUE)
    {
        //TRACE_MSG(TRACE_APP1, "PHY: channel: %hd, status: IDLE",
        //          (FMT__H, gs_current_channel));
        printf("Channel status: IDLE\n");
    }
    else if (cca_status == ZB_FALSE)
    {
        //TRACE_MSG(TRACE_APP1, "PHY: channel: %hd, status: BUSY",
        //          (FMT__H, gs_current_channel));
        printf("Channel status: BUSY\n");
    }
    ZB_SCHEDULE_CALLBACK(test_get_command, param);
}

/***************** MAC Callbacks *****************/
ZB_MLME_RESET_CONFIRM(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, "MLME-RESET.confirm()", (FMT__0));

    /* Call the next test step */
    ZB_SCHEDULE_CALLBACK(test_set_ieee_addr, param);
}

/*! @} */
