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
/* PURPOSE: P/154/MAC/CHANNEL-ACCESS-04 DUT FFD0 test procedure 2
MAC-only build
*/

#define ZB_TEST_NAME TP_154_MAC_CHANNEL_ACCESS_04_DUT_FFD0_TP2
#define ZB_TRACE_FILE_ID 63915

#define ZB_MAC_DUTY_CYCLE_MONITORING

#include "zb_common.h"
#include "zb_mac.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_mac_globals.h"
#include "mac_internal.h"
#include "../common/zb_cert_test_globals.h"
#include "tp_154_mac_channel_access_04_common.h"

#ifndef ZB_MULTI_TEST
#define USE_ZB_MLME_RESET_CONFIRM
#define USE_ZB_MLME_START_CONFIRM
#define USE_ZB_MCPS_DATA_CONFIRM
#define USE_ZB_MLME_DUTY_CYCLE_MODE_INDICATION
#include "zb_mac_only_stubs.h"
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static void test_started_cb(zb_uint8_t param);
static void test_mlme_reset_request(zb_uint8_t param);
static void test_set_ieee_addr(zb_uint8_t param);
static void test_set_short_addr(zb_uint8_t param);
static void test_mlme_start_request(zb_uint8_t param);
static void test_send_packet(zb_uint8_t unused);
static void test_duty_cycle(zb_uint8_t unused);

static zb_ieee_addr_t g_ieee_addr_dut_ffd0 = TEST_DUT_FFD0_MAC_ADDRESS;
static zb_uint8_t g_test_step;
static zb_uint8_t g_main_count = 0;
static zb_uint16_t g_pkt_count;
static zb_uint16_t g_service_count;
static zb_uint8_t g_msg_used[TEST_MSG_LENGTH];

typedef ZB_PACKED_PRE struct test_payload_s
{
    zb_uint8_t lead_ch;
    zb_uint16_t packet_number;
    zb_uint32_t timestamp;
    zb_uint16_t ramp_up;
    zb_uint16_t ramp_down;
    zb_uint8_t step;
    zb_uint8_t msg[TEST_MSG_LENGTH];
} ZB_PACKED_STRUCT test_payload_t;

MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("tp_154_mac_channel_access_04_dut_ffd0_tp2");

    ZB_SCHEDULE_CALLBACK(test_started_cb, 0);

#ifdef ZB_TRACE_LEVEL
    ZB_SET_TRACE_LEVEL(1);
    ZB_SET_TRACE_MASK(0);
#endif /* ZB_TRACE_LEVEL */

    while (1)
    {
        zb_sched_loop_iteration();
    }


    TRACE_DEINIT();

    MAIN_RETURN(0);
}


static void test_started_cb(zb_uint8_t unused)
{
    zb_uint8_t msg_start[] = TEST_STATE_STARTOFTEST;

    ZVUNUSED(unused);

    TRACE_MSG(TRACE_MAC_API1, "Device STARTED OK g_main_count %hd", (FMT__H, g_main_count));

    if (g_main_count < TEST_TP2_GL_MAX_COUNT)
    {
        g_test_step = STARTOFTEST;
        g_pkt_count = 0;
        g_service_count = 0;

        MAC_CTX().cert_hacks.lbt_radio_busy_disabled = 1;

        ZB_MEMCPY(g_msg_used, msg_start, TEST_MSG_LENGTH);

        zb_buf_get_out_delayed(test_mlme_reset_request);
    }
    else
    {
        MAC_CTX().cert_hacks.lbt_radio_busy_disabled = 0;

        TRACE_MSG(TRACE_APP1, "Test finished", (FMT__0));
    }

    g_main_count++;
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
    ZB_MEMCPY((zb_uint8_t *)(req + 1), g_ieee_addr_dut_ffd0, sizeof(zb_ieee_addr_t));

    req->confirm_cb_u.cb = test_set_short_addr;
    zb_mlme_set_request(param);
}


static void test_set_short_addr(zb_uint8_t param)
{
    zb_mlme_set_request_t *req;
    zb_uint16_t short_addr = TEST_DUT_FFD0_SHORT_ADDRESS;

    TRACE_MSG(TRACE_APP1, "MLME-SET.request() mac short addr", (FMT__0));

    req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));
    ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));

    req->pib_attr = ZB_PIB_ATTRIBUTE_SHORT_ADDRESS;
    req->pib_length = sizeof(zb_uint16_t);
    ZB_MEMCPY((zb_uint8_t *)(req + 1), &short_addr, sizeof(zb_ieee_addr_t));

    req->confirm_cb_u.cb = test_mlme_start_request;
    zb_mlme_set_request(param);
}

static void test_mlme_start_request(zb_uint8_t param)
{
    zb_mlme_start_req_t *req = ZB_BUF_GET_PARAM(param, zb_mlme_start_req_t);

    TRACE_MSG(TRACE_APP1, "MLME-START.request()", (FMT__0));

    ZB_BZERO(req, sizeof(zb_mlme_start_req_t));

    req->pan_id = TEST_PAN_ID;
    req->logical_channel = TEST_CHANNEL;
    req->channel_page = TEST_PAGE;
    req->pan_coordinator = 0;      /* will be router */
    req->coord_realignment = 0;
    req->beacon_order = ZB_TURN_OFF_ORDER;
    req->superframe_order = ZB_TURN_OFF_ORDER;

    ZB_SCHEDULE_CALLBACK(zb_mlme_start_request, param);
}

static void test_send_packet(zb_uint8_t unused)
{
    zb_bufid_t buf;
    zb_uint8_t param;
    zb_mcps_data_req_params_t *data_req;
    test_payload_t *msdu;

    TRACE_MSG(TRACE_APP1, "MCPS-DATA.request", (FMT__0));

    ZVUNUSED(unused);

    buf = zb_buf_get_out();
    if (buf)
    {
        param = param;
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "failed while buffer alloc", (FMT__0));
        return;
    }

    msdu = zb_buf_initial_alloc(param, TEST_MAX_MSDU_LENGTH);

    msdu->lead_ch = TEST_LEAD_CH;
    msdu->packet_number = g_pkt_count;
    g_pkt_count++;
    msdu->timestamp = osif_transceiver_time_get() / ZB_EU_FSK_SYMBOL_DURATION_USEC;
    msdu->ramp_up = ZB_MAC_DUTY_CYCLE_RAMP_UP_SYMBOLS;
    msdu->ramp_down = ZB_MAC_DUTY_CYCLE_RAMP_DOWN_SYMBOLS;

    switch (g_test_step)
    {
    case STATE7_WAIT_LIMITED:
    case STATE7_LIMITED_3PACKETS:
        msdu->step = 7;
        break;
    case STATE8_WAIT_NORMAL:
    case STATE8_NORMAL_3PACKETS:
        msdu->step = 8;
        break;
    case ENDOFTEST:
        msdu->step = 9;
        break;
    default:
        msdu->step = g_test_step;
        break;
    }

    ZB_MEMCPY(msdu->msg, g_msg_used, TEST_MSG_LENGTH);

    ZB_MEMSET((zb_uint8_t *)msdu + sizeof(test_payload_t), TEST_FILL_CH, TEST_MAX_MSDU_LENGTH - TEST_MSG_LENGTH);
    data_req = ZB_BUF_GET_PARAM(param, zb_mcps_data_req_params_t);
    ZB_BZERO(data_req, sizeof(zb_mcps_data_req_params_t));

    data_req->src_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
    data_req->src_addr.addr_short = TEST_DUT_FFD0_SHORT_ADDRESS;
    data_req->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
    data_req->dst_addr.addr_short = TEST_DST_SHORT_ADDRESS;
    data_req->dst_pan_id = TEST_PAN_ID;
    data_req->msdu_handle = 0;
    data_req->tx_options = 0x00;

    ZB_SCHEDULE_CALLBACK(zb_mcps_data_request, param);
}

static void test_duty_cycle(zb_uint8_t unused)
{
    zb_uint8_t msg_end[] = TEST_STATE_ENDOFTEST;

    g_test_step++;

    TRACE_MSG(TRACE_APP1, "test_duty_cycle(): g_main_count %hd, g_test_step %hd", (FMT__H_H, g_main_count, g_test_step));

    ZVUNUSED(unused);

    /* TEST STATE #7 here is g_test_step 7 and 8
       TEST STATE #8 here is g_test_step 9 and 10
       TEST STATE #9 here is g_test_step 11
    */

    /* note: there is no implementation for MCPS-DATA.confirm with status of DUTY_CYCLE_EXCEEDED for NSNG */
#ifdef ZB_NSNG
    if (g_test_step == STATE6_CRITICAL_3PACKETS)
    {
        g_test_step++;
    }
#endif  /* ZB_NSNG */

    switch (g_test_step)
    {
    case ENDOFTEST:
        ZB_MEMCPY(g_msg_used, msg_end, TEST_MSG_LENGTH);
    case STATE8_NORMAL_3PACKETS:
    case STATE7_LIMITED_3PACKETS:
    case STATE6_CRITICAL_3PACKETS:
    case STATE5_LIMITED_TO_SUSPENDED:
    case STATE3_NORMAL_TO_CRITICAL:
    case STATE1_NORMAL_TO_LIMITED:
        ZB_SCHEDULE_CALLBACK(test_send_packet, 0);
        break;
    default:
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

ZB_MLME_START_CONFIRM(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, "MLME-START.confirm()", (FMT__0));

    /* Call the next test step */
    ZB_SCHEDULE_CALLBACK(test_send_packet, 0);

    zb_buf_free(param);
}

ZB_MCPS_DATA_CONFIRM(zb_uint8_t param)
{
    zb_time_t time_alarm;
    zb_uint8_t msg_norm[] = TEST_STATE_NORMAL;

    TRACE_MSG(TRACE_APP1, "MCPS-DATA.confirm() status %hd", (FMT__H, zb_buf_get_status(param)));

    switch (g_test_step)
    {
    case STARTOFTEST:
        if (g_service_count < 2)
        {
            g_service_count++;
            ZB_SCHEDULE_CALLBACK(test_send_packet, 0);
        }
        else
        {
            g_service_count = 0;
            ZB_MEMCPY(g_msg_used, msg_norm, TEST_MSG_LENGTH);
            ZB_SCHEDULE_CALLBACK(test_duty_cycle, 0);
        }
        break;
    case STATE1_NORMAL_TO_LIMITED:
    case STATE3_NORMAL_TO_CRITICAL:
        ZB_SCHEDULE_CALLBACK(test_send_packet, 0);
        break;
    case STATE5_LIMITED_TO_SUSPENDED:
        if (ZB_SCHEDULE_GET_ALARM_TIME(test_duty_cycle, ZB_ALARM_ANY_PARAM, &time_alarm) == RET_NOT_FOUND)
        {
            if (!(zb_buf_get_status(param)))
            {
                ZB_SCHEDULE_CALLBACK(test_send_packet, 0);
            }
            else
            {
                ZB_SCHEDULE_ALARM(test_duty_cycle, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(160));
            }
        }
        break;
    case STATE6_CRITICAL_3PACKETS:
        if (!(zb_buf_get_status(param)))
        {
            if (g_service_count < 2)
            {
                g_service_count++;
                ZB_SCHEDULE_CALLBACK(test_send_packet, 0);
            }
            else
            {
                g_service_count = 0;
                ZB_SCHEDULE_CALLBACK(test_duty_cycle, 0);
            }
        }
        else
        {
            ZB_SCHEDULE_CALLBACK(test_send_packet, 0);
        }
        break;
    case STATE7_WAIT_LIMITED:
    case STATE8_WAIT_NORMAL:
        if (g_service_count < 2)
        {
            g_service_count++;
        }
        break;
    case STATE7_LIMITED_3PACKETS:
    case STATE8_NORMAL_3PACKETS:
        if (g_service_count < 2)
        {
            g_service_count++;
            ZB_SCHEDULE_CALLBACK(test_send_packet, 0);
        }
        else
        {
            g_service_count = 0;
            ZB_SCHEDULE_CALLBACK(test_duty_cycle, 0);
        }
        break;
    case ENDOFTEST:
        if (g_service_count < 2)
        {
            g_service_count++;
            ZB_SCHEDULE_CALLBACK(test_send_packet, 0);
        }
        else
        {
            ZB_SCHEDULE_CALLBACK(test_started_cb, 0);
        }
        break;
    default:
        break;
    }

    zb_buf_free(param);
}

ZB_MLME_DUTY_CYCLE_MODE_INDICATION(zb_uint8_t param)
{
    zb_mlme_duty_cycle_mode_indication_t *ind = ZB_BUF_GET_PARAM(param, zb_mlme_duty_cycle_mode_indication_t);

    zb_uint8_t msg_norm[] = TEST_STATE_NORMAL;
    zb_uint8_t msg_lim[] = TEST_STATE_LIMITED;
    zb_uint8_t msg_crit[] = TEST_STATE_CRITICAL;
    zb_uint8_t msg_susp[] = TEST_STATE_SUSPENDED;

    TRACE_MSG(TRACE_APP1, "zb_mlme_duty_cycle_mode_indication() param %hd, status %hd, g_test_step %hd",
              (FMT__H_H_H, param, ind->status, g_test_step));

    if (g_test_step == STARTOFTEST || g_test_step == ENDOFTEST)
    {
        zb_buf_free(param);

        return;
    }

    switch (ind->status)
    {
    case ZB_MAC_DUTY_CYCLE_STATUS_NORMAL:
        ZB_MEMCPY(g_msg_used, msg_norm, TEST_MSG_LENGTH);
        if (g_test_step == STATE2_LIMITED_TO_NORMAL || g_test_step == STATE8_WAIT_NORMAL)
        {
            ZB_SCHEDULE_CALLBACK(test_duty_cycle, 0);
        }
        break;
    case ZB_MAC_DUTY_CYCLE_STATUS_LIMITED:
        ZB_MEMCPY(g_msg_used, msg_lim, TEST_MSG_LENGTH);
        if (g_test_step == STATE1_NORMAL_TO_LIMITED || g_test_step == STATE4_CRITICAL_TO_LIMITED || g_test_step == STATE7_WAIT_LIMITED)
        {
            ZB_SCHEDULE_CALLBACK(test_duty_cycle, 0);
        }
        if (g_test_step == STATE8_NORMAL_3PACKETS && g_service_count < 2)
        {
            g_test_step--;
        }
        break;
    case ZB_MAC_DUTY_CYCLE_STATUS_CRITICAL:
        ZB_MEMCPY(g_msg_used, msg_crit, TEST_MSG_LENGTH);
        if (g_test_step == STATE3_NORMAL_TO_CRITICAL)
        {
            ZB_SCHEDULE_CALLBACK(test_duty_cycle, 0);
        }
        if (g_test_step == STATE7_LIMITED_3PACKETS && g_service_count < 2)
        {
            g_test_step--;
        }
        break;
    case ZB_MAC_DUTY_CYCLE_STATUS_SUSPENDED:
        ZB_MEMCPY(g_msg_used, msg_susp, TEST_MSG_LENGTH);
        if (g_test_step == STATE5_LIMITED_TO_SUSPENDED)
        {
            /* wait until ZB_MCPS_DATA_CONFIRM called with g_test_step = 5 */
            ZB_SCHEDULE_ALARM_CANCEL(test_duty_cycle, ZB_ALARM_ANY_PARAM);
            ZB_SCHEDULE_ALARM(test_duty_cycle, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(160));
        }
        break;
    default:
        break;
    }

    zb_buf_free(param);
}

/*! @} */
