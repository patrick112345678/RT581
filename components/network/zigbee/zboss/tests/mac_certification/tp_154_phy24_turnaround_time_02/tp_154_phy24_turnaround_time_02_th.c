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
/* PURPOSE: TP/154/PHY24/TURNAROUNDTIME-01
MAC-only build
*/

#define ZB_TEST_NAME TP_154_PHY24_TURNAROUND_TIME_02_TH
#define ZB_TRACE_FILE_ID 57221
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "../common/zb_cert_test_globals.h"
#include "tp_154_phy24_turnaround_time_02_common.h"

#ifndef ZB_MULTI_TEST
#define USE_ZB_MLME_RESET_CONFIRM
#define USE_ZB_MCPS_DATA_INDICATION
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
static void test_set_channel(zb_uint8_t param);
static void test_dump_packet(zb_uint8_t *buf, zb_ushort_t len);
static zb_ret_t test_verify_packet(zb_uint8_t *pkt, zb_uint8_t len);

static zb_ieee_addr_t gs_ieee_addr_th = TEST_TH_MAC_ADDRESS;

/* Number of packets sent on operating channel */
static zb_uint8_t gs_current_channel;
/* Number of packets sent on operating channel */
static zb_uint16_t gs_packets_rx_channel[16];

MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("tp_154_phy24_turnaround_time_02_th");

    ZB_SCHEDULE_CALLBACK(test_started_cb, 0);

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

    gs_current_channel = TEST_CHANNEL_MIN;
    ZB_BZERO(gs_packets_rx_channel, 16 * sizeof(zb_uint16_t));

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
    ZB_MEMCPY((zb_uint8_t *)(req + 1), gs_ieee_addr_th, sizeof(zb_ieee_addr_t));

    req->confirm_cb_u.cb = test_set_short_addr;
    zb_mlme_set_request(param);
}

static void test_set_short_addr(zb_uint8_t param)
{
    zb_mlme_set_request_t *req;
    zb_uint16_t short_addr = TEST_TH_SHORT_ADDRESS;

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

    req->confirm_cb_u.cb = test_set_channel;
    zb_mlme_set_request(param);
}

static void test_set_channel(zb_uint8_t param)
{
    zb_mlme_set_request_t *req;
    zb_uint8_t channel = gs_current_channel;

    TRACE_MSG(TRACE_APP1, "MLME-SET.request() Channel %hd", (FMT__H, channel));

    req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));
    ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));

    req->pib_attr = ZB_PHY_PIB_CURRENT_CHANNEL;
    req->pib_length = sizeof(zb_uint8_t);
    ZB_MEMCPY((zb_uint8_t *)(req + 1), &channel, sizeof(zb_uint8_t));

    zb_mlme_set_request(param);
}

static void test_next_channel_caller(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    TRACE_MSG(TRACE_APP1, "Total number of packets %d on channel %hd",
              (FMT__D, gs_packets_rx_channel[gs_current_channel - 11], gs_current_channel));

    if (gs_current_channel < TEST_CHANNEL_MAX)
    {
        gs_current_channel++;
        zb_buf_get_out_delayed(test_set_channel);
    }
    else
    {
        TRACE_MSG(TRACE_APP1, "Test finished", (FMT__0));
    }
}

static void test_next_packet_recvd(zb_uint8_t param)
{

    if (gs_packets_rx_channel[gs_current_channel - 11] == 0)
    {
        ZB_SCHEDULE_ALARM(test_next_channel_caller, 0, TEST_TH_SWITCH_CHANNEL_TMO);
    }

    /* Cut tail */
    zb_buf_cut_right(param, ZB_TAIL_SIZE_FOR_RECEIVED_MAC_FRAME);

#if defined TEST_DUMP_PACKETS
    test_dump_packet(zb_buf_begin(param), zb_buf_len(param));
#endif  /* TEST_DUMP_PACKETS */

#if defined TEST_VERIFY_PACKETS
    if (RET_ERROR == test_verify_packet(zb_buf_begin(param), zb_buf_len(param)))
    {
        TRACE_MSG(TRACE_ERROR, "ERROR: Received nexpected packet", (FMT__0));
    }
    else
#endif
    {
        gs_packets_rx_channel[gs_current_channel - 11]++;

        TRACE_MSG(TRACE_APP1, "Received packet #%d:",
                  (FMT__D, gs_packets_rx_channel[gs_current_channel - 11]));
    }

    zb_buf_free(param);
}

static void test_dump_packet(zb_uint8_t *buf, zb_ushort_t len)
{
    zb_ushort_t i;
    ZVUNUSED(buf);

    TRACE_MSG(TRACE_APP1, "DUMP: packet len %hd", (FMT__H, len));

#define HEX_ARG(n) buf[i+n]

    for (i = 0 ; i < len ; )
    {
        if (len - i >= 8)
        {
            TRACE_MSG(TRACE_APP1, "DUMP: %hx %hx %hx %hx %hx %hx %hx %hx",
                      (FMT__H_H_H_H_H_H_H_H,
                       HEX_ARG(0), HEX_ARG(1), HEX_ARG(2), HEX_ARG(3),
                       HEX_ARG(4), HEX_ARG(5), HEX_ARG(6), HEX_ARG(7)));
            i += 8;
        }
        else
        {
            switch (len - i)
            {
            case 7:
                TRACE_MSG(TRACE_APP1, "DUMP: %hx %hx %hx %hx %hx %hx %hx",
                          (FMT__H_H_H_H_H_H_H,
                           HEX_ARG(0), HEX_ARG(1), HEX_ARG(2), HEX_ARG(3),
                           HEX_ARG(4), HEX_ARG(5), HEX_ARG(6)));
                break;
            case 6:
                TRACE_MSG(TRACE_APP1, "DUMP: %hx %hx %hx %hx %hx %hx",
                          (FMT__H_H_H_H_H_H,
                           HEX_ARG(0), HEX_ARG(1), HEX_ARG(2), HEX_ARG(3),
                           HEX_ARG(4), HEX_ARG(5)));
                break;
            case 5:
                TRACE_MSG(TRACE_APP1, "DUMP: %hx %hx %hx %hx %hx",
                          (FMT__H_H_H_H_H,
                           HEX_ARG(0), HEX_ARG(1), HEX_ARG(2), HEX_ARG(3),
                           HEX_ARG(4)));
                break;
            case 4:
                TRACE_MSG(TRACE_APP1, "DUMP: %hx %hx %hx %hx",
                          (FMT__H_H_H_H_H_H_H,
                           HEX_ARG(0), HEX_ARG(1), HEX_ARG(2), HEX_ARG(3)));
                break;
            case 3:
                TRACE_MSG(TRACE_APP1, "DUMP: %hx %hx %hx",
                          (FMT__H_H_H,
                           HEX_ARG(0), HEX_ARG(1), HEX_ARG(2)));
                break;
            case 2:
                TRACE_MSG(TRACE_APP1, "DUMP: %hx %hx",
                          (FMT__H_H,
                           HEX_ARG(0), HEX_ARG(1)));
                break;
            case 1:
                TRACE_MSG(TRACE_APP1, "DUMP: %hx",
                          (FMT__H,
                           HEX_ARG(0)));
                break;
            }
            i = len;
        }
    }
}


static zb_ret_t test_verify_packet(zb_uint8_t *pkt, zb_uint8_t len)
{
    zb_ret_t ret = RET_ERROR;

    if (len == (TEST_MSDU_HEADER_SIZE + TEST_MSDU_PAYLOAD_SIZE))
    {
        zb_uint8_t msdu_header[TEST_MSDU_HEADER_SIZE] = TEST_MSDU_HEADER;

        /* Don't compare DSN */
        msdu_header[2] = pkt[2];

        if (!ZB_MEMCMP(pkt, msdu_header, TEST_MSDU_HEADER_SIZE))
        {
            zb_uint8_t i;

            for (i = 0; i < TEST_MSDU_PAYLOAD_NUM; i++)
            {
                if (!ZB_MEMCMP(pkt + TEST_MSDU_HEADER_SIZE, gs_msdu_payload[i], TEST_MSDU_PAYLOAD_SIZE))
                {
                    ret = RET_OK;
                    break;
                }
            }
        }
    }

    return ret;
}


/***************** MAC Callbacks *****************/
ZB_MLME_RESET_CONFIRM(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, "MLME-RESET.confirm()", (FMT__0));

    /* Call the next test step */
    ZB_SCHEDULE_CALLBACK(test_set_ieee_addr, param);
}

ZB_MCPS_DATA_INDICATION(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, "MCPS-DATA.indication()", (FMT__0));

    ZB_SCHEDULE_CALLBACK(test_next_packet_recvd, param);
}


/*! @} */
