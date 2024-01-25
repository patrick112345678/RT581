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
/* PURPOSE: TP/154/MAC/SCANNING-04 FFD1
MAC-only build
*/

#define ZB_TEST_NAME TP_154_MAC_SCANNING_04_DUT_FFD1
#define ZB_TRACE_FILE_ID 57207
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "../common/zb_cert_test_globals.h"
#include "tp_154_mac_scanning_04_common.h"
#include "zb_osif.h"

#ifndef ZB_MULTI_TEST
#define USE_ZB_MLME_RESET_CONFIRM
#define USE_ZB_MLME_SCAN_CONFIRM
#define USE_ZB_MLME_BEACON_NOTIFY_INDICATION
#include "zb_mac_only_stubs.h"
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_dut_ffd1 = TEST_DUT_FFD1_MAC_ADDRESS;

static void test_started_cb(zb_uint8_t param);
static void test_mlme_reset_request(zb_uint8_t param);
static void test_set_ieee_addr(zb_uint8_t param);
static void test_mlme_scan_request(zb_uint8_t param);

static zb_bool_t gs_mac_auto_req_processed = ZB_FALSE;

MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("tp_154_mac_scanning_04_dut_ffd1");

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

    req->confirm_cb_u.cb = test_mlme_scan_request;
    zb_mlme_set_request(param);
}

static void test_mlme_scan_request(zb_uint8_t param)
{

    TRACE_MSG(TRACE_APP1, "MLME-SCAN.request() - Active scan", (FMT__0));

    ZB_MLME_BUILD_SCAN_REQUEST(param, TEST_PAGE, ZB_TRANSCEIVER_ALL_CHANNELS_MASK, TEST_SCAN_TYPE, TEST_SCAN_DURATION);
    //ZB_MLME_BUILD_SCAN_REQUEST(param, TEST_PAGE, 0x07FBE000, TEST_SCAN_TYPE, TEST_SCAN_DURATION);
    ZB_SCHEDULE_CALLBACK(zb_mlme_scan_request, param);
}

static void test_clear_mac_auto_request(zb_uint8_t param)
{
    zb_mlme_set_request_t *req;
    zb_uint8_t auto_request = 0;

    //TRACE_MSG(TRACE_APP1, "MLME-SET.request() macAutoRequest", (FMT__0));

    req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));
    ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));

    req->pib_attr = ZB_PIB_ATTRIBUTE_AUTO_REQUEST;
    req->pib_length = sizeof(zb_uint8_t);
    ZB_MEMCPY((zb_uint8_t *)(req + 1), &auto_request, sizeof(zb_uint8_t));

    req->confirm_cb_u.cb = test_mlme_scan_request;
    zb_mlme_set_request(param);
}

/***************** MAC Callbacks *****************/
ZB_MLME_RESET_CONFIRM(zb_uint8_t param)
{
    //TRACE_MSG(TRACE_APP1, "MLME-RESET.confirm()", (FMT__0));

    /* Call the next test step */
    ZB_SCHEDULE_CALLBACK(test_set_ieee_addr, param);
}


ZB_MLME_BEACON_NOTIFY_INDICATION(zb_uint8_t param)
{
    zb_mac_beacon_notify_indication_t *ind = (zb_mac_beacon_notify_indication_t *)zb_buf_begin(param);
    zb_pan_descriptor_t *pan_desc = &(ind->pan_descriptor);
    /*
    TRACE_MSG(TRACE_APP1, "MLME-BEACON-NOTIFY.indication()", (FMT__0));
    TRACE_MSG(TRACE_APP1, "BSN = %x", (FMT__H, ind->bsn));
    TRACE_MSG(TRACE_APP1, "CoordAddrMode = %x", (FMT__H, pan_desc->coord_addr_mode));
    TRACE_MSG(TRACE_APP1, "CoordPanId = %x", (FMT__H, pan_desc->coord_pan_id));
    TRACE_MSG(TRACE_APP1, "CoordAddr = %x", (FMT__H, pan_desc->coord_address));
    TRACE_MSG(TRACE_APP1, "LogicalChannel = %x", (FMT__H, pan_desc->logical_channel));
    TRACE_MSG(TRACE_APP1, "SuperframeSpec = %x", (FMT__H, pan_desc->super_frame_spec));
    TRACE_MSG(TRACE_APP1, "GTSPermit = %x", (FMT__H, pan_desc->gts_permit));
    TRACE_MSG(TRACE_APP1, "LinkQuality = %x", (FMT__H, pan_desc->link_quality));
    TRACE_MSG(TRACE_APP1, "Timestamp = %lx", (FMT__L, pan_desc->timestamp));
    TRACE_MSG(TRACE_APP1, "PendAddrSpec = %x", (FMT__H, ind->pend_addr_spec));
    TRACE_MSG(TRACE_APP1, "AddrList = %x", (FMT__H, ind->addr_list[0]));
    TRACE_MSG(TRACE_APP1, "sduLength = %x", (FMT__H, ind->sdu_length));
    TRACE_MSG(TRACE_APP1, "sdu = %x", (FMT__H, ind->sdu[0]));
    */
    zb_buf_free(param);
}

zb_bool_t auto_req_result4 = ZB_FALSE;
zb_bool_t no_auto_req_result4 = ZB_FALSE;

ZB_MLME_SCAN_CONFIRM(zb_uint8_t param)
{
    zb_mac_scan_confirm_t *scan_confirm = 0;
    scan_confirm = ZB_BUF_GET_PARAM(param, zb_mac_scan_confirm_t);
    TRACE_MSG(TRACE_APP1, "MLME-SCAN.confirm(): result_list_size = %hd", (FMT__0, scan_confirm->result_list_size));
    if (!gs_mac_auto_req_processed)
    {
        TRACE_MSG(TRACE_APP1, "You should have seen one beacon notify.", (FMT__0));
        if (scan_confirm->status == 0x00 &&
                scan_confirm->scan_type == 0x01 &&
                scan_confirm->channel_page == 0x00 &&
                scan_confirm->result_list_size == 0x01 &&
                scan_confirm->unscanned_channels == 0x00000000)
        {
            auto_req_result4 = ZB_TRUE;
        }
    }
    else
    {
        TRACE_MSG(TRACE_APP1, "You should have seen one beacon notify.", (FMT__0));
        if (scan_confirm->status == 0x00 &&
                scan_confirm->scan_type == 0x01 &&
                scan_confirm->channel_page == 0x00 &&
                scan_confirm->result_list_size == 0x00 &&
                scan_confirm->unscanned_channels == 0x00000000)
        {
            no_auto_req_result4 = ZB_TRUE;
        }
    }

    if (auto_req_result4 && no_auto_req_result4)
    {
        zb_osif_led_on(0); // blue
        TRACE_MSG(TRACE_APP1, "Scanning04 success", (FMT__0));
    }

    if (!gs_mac_auto_req_processed)
    {
        gs_mac_auto_req_processed = ZB_TRUE;
        ZB_SCHEDULE_CALLBACK(test_clear_mac_auto_request, param);
    }
    else
    {
        zb_buf_free(param);
    }
}


/*! @} */
