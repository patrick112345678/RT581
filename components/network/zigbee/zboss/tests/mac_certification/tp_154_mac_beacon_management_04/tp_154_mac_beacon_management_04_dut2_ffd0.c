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
/*  PURPOSE: TP/154/MAC/BEACON-MANAGEMENT-04 DUT2 FFD0 implementation
*/


#define ZB_TEST_NAME TP_154_MAC_BEACON_MANAGEMENT_04_DUT2_FFD0
#define ZB_TRACE_FILE_ID 63844
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_mac_globals.h"
#include "zb_ie.h"
#include "zboss_api.h" /* added as zb_mac_joining_policy_t required */

#include "../common/zb_cert_test_globals.h"
#include "tp_154_mac_beacon_management_04_common.h"

#ifndef ZB_MULTI_TEST
#define USE_ZB_MLME_RESET_CONFIRM
#define USE_ZB_MLME_ASSOCIATE_CONFIRM
#define USE_ZB_MLME_START_CONFIRM
#define USE_ZB_MLME_SCAN_CONFIRM
#define USE_ZB_MLME_BEACON_NOTIFY_INDICATION
#define USE_ZB_MLME_ASSOCIATE_INDICATION
#define USE_ZB_MLME_COMM_STATUS_INDICATION

#include "zb_mac_only_stubs.h"
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static void test_started_cb(zb_uint8_t unused);
static void test_mlme_reset_request(zb_uint8_t param);
static void test_set_ieee_addr(zb_uint8_t param);
static void test_set_beacon_payload_length(zb_uint8_t param);
static void test_set_beacon_payload(zb_uint8_t param);
static void test_set_rx_on_when_idle(zb_uint8_t param);
static void test_set_association_permit(zb_uint8_t param);
static void test_clear_mac_auto_request(zb_uint8_t param);
static void test_ieee_joining_list_clear(zb_uint8_t param);
static void test_set_joining_policy(zb_uint8_t param);
static void test_ieee_joining_list_add(zb_uint8_t param);
static void test_associate_request(zb_uint8_t param);
static void test_mlme_start_request(zb_uint8_t param);
static void test_mlme_scan_request(zb_uint8_t param);
static void test_prepare_enhanced_beacon_discovery(zb_uint8_t param);
static void test_incr_test_step(zb_uint8_t param);

static zb_ieee_addr_t g_ieee_addr_dut2 = TEST_DUT2_FFD0_IEEE_ADDR;
static zb_ieee_addr_t g_ieee_addr_dut1 = TEST_DUT1_FFD1_IEEE_ADDR;
static zb_uint8_t g_test_step = TEST_STEP_INITIAL;
static zb_bool_t g_beacon_received = ZB_FALSE;

MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("tp_154_mac_beacon_management_04_dut2_ffd0");

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
    ZB_MEMCPY((zb_uint8_t *)(req + 1), g_ieee_addr_dut2, sizeof(zb_ieee_addr_t));

    req->confirm_cb_u.cb = test_set_beacon_payload_length;

    zb_mlme_set_request(param);
}

static void test_set_beacon_payload_length(zb_uint8_t param)
{
    zb_mlme_set_request_t *req;

    TRACE_MSG(TRACE_APP1, "MLME-SET.request() BeaconPayloadLength", (FMT__0));

    req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));
    req->pib_attr = ZB_PIB_ATTRIBUTE_BEACON_PAYLOAD_LENGTH;
    req->pib_length = sizeof(zb_uint8_t);
    *((zb_uint8_t *)(req + 1)) = TEST_BEACON_PAYLOAD_LENGTH;

    req->confirm_cb_u.cb = test_set_beacon_payload;

    zb_mlme_set_request(param);
}

static void test_set_beacon_payload(zb_uint8_t param)
{
    zb_mlme_set_request_t *req;
    zb_uint8_t *beacon = TEST_DUT2_BEACON_PAYLOAD;

    TRACE_MSG(TRACE_APP1, "MLME-SET.request() BeaconPayload", (FMT__0));

    req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + TEST_BEACON_PAYLOAD_LENGTH);
    ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + TEST_BEACON_PAYLOAD_LENGTH);

    req->pib_attr = ZB_PIB_ATTRIBUTE_BEACON_PAYLOAD;
    req->pib_length = TEST_BEACON_PAYLOAD_LENGTH;
    ZB_MEMCPY((zb_uint8_t *)(req + 1), beacon, TEST_BEACON_PAYLOAD_LENGTH);

    req->confirm_cb_u.cb = test_set_rx_on_when_idle;

    zb_mlme_set_request(param);
}


static void test_set_rx_on_when_idle(zb_uint8_t param)
{
    zb_mlme_set_request_t *req;
    zb_uint8_t rx_on_when_idle = TEST_RX_ON_WHEN_IDLE;

    TRACE_MSG(TRACE_APP1, "MLME-SET.request() RxOnWhenIdle", (FMT__0));

    req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));
    ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));

    req->pib_attr = ZB_PIB_ATTRIBUTE_RX_ON_WHEN_IDLE;
    req->pib_length = sizeof(zb_uint8_t);
    ZB_MEMCPY((zb_uint8_t *)(req + 1), &rx_on_when_idle, sizeof(zb_uint8_t));

    req->confirm_cb_u.cb = test_set_association_permit;

    zb_mlme_set_request(param);
}

static void test_set_association_permit(zb_uint8_t param)
{
    zb_mlme_set_request_t *req;
    zb_uint8_t association_permit = TEST_ASSOCIATION_PERMIT;

    TRACE_MSG(TRACE_APP1, "MLME-SET.request() AssociationPermit", (FMT__0));

    req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));
    ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));

    req->pib_attr = ZB_PIB_ATTRIBUTE_ASSOCIATION_PERMIT;
    req->pib_length = sizeof(zb_uint8_t);
    ZB_MEMCPY((zb_uint8_t *)(req + 1), &association_permit, sizeof(zb_uint8_t));

    req->confirm_cb_u.cb = test_clear_mac_auto_request;

    zb_mlme_set_request(param);
}

static void test_clear_mac_auto_request(zb_uint8_t param)
{
    zb_mlme_set_request_t *req;
    zb_uint8_t auto_request = 0;

    TRACE_MSG(TRACE_APP1, "MLME-SET.request() macAutoRequest", (FMT__0));

    req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));
    ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));

    req->pib_attr = ZB_PIB_ATTRIBUTE_AUTO_REQUEST;
    req->pib_length = sizeof(zb_uint8_t);
    ZB_MEMCPY((zb_uint8_t *)(req + 1), &auto_request, sizeof(zb_uint8_t));

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

static void test_ieee_joining_list_clear(zb_uint8_t param)
{
    zb_mlme_set_request_t *set_req;
    zb_mlme_set_ieee_joining_list_req_t *jl_set_req;

    TRACE_MSG(TRACE_APP1, "MLME-SET.request() clear mibJoiningPolicy", (FMT__0));


    set_req = zb_buf_initial_alloc(param, sizeof(*set_req) + sizeof(*jl_set_req));
    set_req->pib_attr = ZB_PIB_ATTRIBUTE_JOINING_IEEE_LIST;
    set_req->pib_index = 0;
    set_req->pib_length = sizeof(*jl_set_req);
    set_req->confirm_cb_u.cb = test_set_joining_policy;

    jl_set_req = (zb_mlme_set_ieee_joining_list_req_t *)(set_req + 1);
    jl_set_req->op_type = ZB_MLME_SET_IEEE_JL_REQ_CLEAR;

    zb_mlme_set_request(param);
}

static void test_set_joining_policy(zb_uint8_t param)
{
    zb_mlme_set_request_t *req;
    zb_uint8_t joining_policy = ZB_MAC_JOINING_POLICY_NO_JOIN;

    TRACE_MSG(TRACE_APP1, "MLME-SET.request() mibJoiningPolicy", (FMT__0));

    req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));
    ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));

    TRACE_MSG(TRACE_APP1, "ffd0_g_test_step %hd", (FMT__H, g_test_step));

    switch (g_test_step)
    {
    case DUT1_DUT2_JP_NOJOIN:
        TRACE_MSG(TRACE_APP1, "ZB_MAC_JOINING_POLICY_NO_JOIN", (FMT__0));
        joining_policy = ZB_MAC_JOINING_POLICY_NO_JOIN;

        req->pib_attr = ZB_PIB_ATTRIBUTE_JOINING_POLICY;
        req->pib_length = sizeof(zb_uint8_t);
        ZB_MEMCPY((zb_uint8_t *)(req + 1), &joining_policy, sizeof(zb_uint8_t));
        req->confirm_cb_u.cb = test_mlme_scan_request;
        break;

    case DUT1_DUT2_JP_ALLJOIN:
        TRACE_MSG(TRACE_APP1, "ZB_MAC_JOINING_POLICY_ALL_JOIN", (FMT__0));
        joining_policy = ZB_MAC_JOINING_POLICY_ALL_JOIN;

        req->pib_attr = ZB_PIB_ATTRIBUTE_JOINING_POLICY;
        req->pib_length = sizeof(zb_uint8_t);
        ZB_MEMCPY((zb_uint8_t *)(req + 1), &joining_policy, sizeof(zb_uint8_t));
        req->confirm_cb_u.cb = test_mlme_scan_request;
        break;

    case DUT1_DUT2_JP_IEEELISTJOIN_INLIST:
        TRACE_MSG(TRACE_APP1, "ZB_MAC_JOINING_POLICY_IEEELIST_JOIN_INLIST", (FMT__0));
        joining_policy = ZB_MAC_JOINING_POLICY_IEEELIST_JOIN;

        req->pib_attr = ZB_PIB_ATTRIBUTE_JOINING_POLICY;
        req->pib_length = sizeof(zb_uint8_t);
        ZB_MEMCPY((zb_uint8_t *)(req + 1), &joining_policy, sizeof(zb_uint8_t));
        req->confirm_cb_u.cb = test_ieee_joining_list_add;
        break;

    case DUT1_DUT2_JP_IEEELISTJOIN_NOTINLIST:
        TRACE_MSG(TRACE_APP1, "ZB_MAC_JOINING_POLICY_IEEELIST_JOIN_NOTINLIST", (FMT__0));
        joining_policy = ZB_MAC_JOINING_POLICY_IEEELIST_JOIN;

        req->pib_attr = ZB_PIB_ATTRIBUTE_JOINING_POLICY;
        req->pib_length = sizeof(zb_uint8_t);
        ZB_MEMCPY((zb_uint8_t *)(req + 1), &joining_policy, sizeof(zb_uint8_t));
        req->confirm_cb_u.cb = test_mlme_scan_request;
        break;

    case  TEST_STEP_FINISHED:
        break;
    }

    if (g_test_step != TEST_STEP_FINISHED)
    {
        zb_mlme_set_request(param);
    }
}

static void test_associate_request(zb_uint8_t param)
{

    ZB_MLME_BUILD_ASSOCIATE_REQUEST(param,
                                    TEST_PAGE,
                                    TEST_CHANNEL,
                                    TEST_PAN_ID,
                                    ZB_ADDR_64BIT_DEV,
                                    &g_ieee_addr_dut1,
                                    TEST_ASSOCIATION_CAP_INFO);

    ZB_SCHEDULE_CALLBACK(zb_mlme_associate_request, param);
}

static void test_ieee_joining_list_add(zb_uint8_t param)
{
    zb_mlme_set_request_t *set_req;
    zb_mlme_set_ieee_joining_list_req_t *jl_set_req;

    TRACE_MSG(TRACE_APP1, "MLME-SET.request() ieeeJoiningList", (FMT__0));


    set_req = zb_buf_initial_alloc(param, sizeof(*set_req) + sizeof(*jl_set_req));

    set_req->pib_attr = ZB_PIB_ATTRIBUTE_JOINING_IEEE_LIST;
    set_req->pib_index = 0;
    set_req->pib_length = sizeof(*jl_set_req);
    set_req->confirm_cb_u.cb = test_mlme_scan_request;

    jl_set_req = (zb_mlme_set_ieee_joining_list_req_t *)(set_req + 1);
    jl_set_req->op_type = ZB_MLME_SET_IEEE_JL_REQ_INSERT;
    ZB_MEMCPY(&jl_set_req->param.ieee_value, g_ieee_addr_dut1, sizeof(zb_ieee_addr_t));

    zb_mlme_set_request(param);
}

static void test_mlme_scan_request(zb_uint8_t param)
{

    TRACE_MSG(TRACE_APP1, "MLME-SCAN.request()", (FMT__0));

    ZB_MLME_BUILD_SCAN_REQUEST(param, TEST_PAGE, TEST_CHANNEL_MASK, TEST_SCAN_TYPE, TEST_SCAN_DURATION);
    test_prepare_enhanced_beacon_discovery(param);

    ZB_SCHEDULE_ALARM(zb_mlme_scan_request, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(TEST_TIMEOUT));

}

static void test_prepare_enhanced_beacon_discovery(zb_uint8_t param)
{
    zb_uint8_t *ptr;
    zb_uint8_t pie_len;

    /* MLME PIE with nested IE + EB filter payload
       vendor PIE + tx power descriptor as payload */
    pie_len  = ZB_PIE_HEADER_LENGTH
               + ZB_NIE_HEADER_LENGTH + 1 + ZB_PIE_VENDOR_HEADER_LENGTH
               + ZB_TX_POWER_IE_DESCRIPTOR_LEN;

    ptr = zb_buf_initial_alloc((zb_bufid_t )param,
                               pie_len + ZB_MLME_SCAN_REQUEST_IE_SIZES_HDR_LEN);
    ZB_MLME_SCAN_REQUEST_SET_IE_SIZES_HDR(ptr, 0, pie_len);
    ptr += ZB_MLME_SCAN_REQUEST_IE_SIZES_HDR_LEN;

    /* MLME nested PIE */
    ZB_SET_NEXT_PIE_HEADER(ptr, ZB_PIE_GROUP_MLME, ZB_NIE_HEADER_LENGTH + 1);
    ZB_SET_NEXT_SHORT_NIE_HEADER(ptr, ZB_NIE_SUB_ID_EB_FILTER, 1);

    *ptr = ZB_EB_FILTER_IE_PERMIT_JOINING_ON; /* simple EB Filter*/
    ptr += 1;

    /* Vendor PIE */
    ZB_SET_NEXT_PIE_ZIGBEE_VENDOR_HEADER(ptr, ZB_TX_POWER_IE_DESCRIPTOR_LEN);
    ZB_SET_NEXT_TX_POWER_IE_DESCRIPTOR(ptr, MAC_CTX().current_tx_power);
}

static void test_incr_test_step(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, "test_incr_test_step(), test step %hd", (FMT__H, g_test_step));

    g_test_step++;

    if (g_test_step < TEST_STEP_FINISHED)
    {
        ZB_SCHEDULE_CALLBACK(test_ieee_joining_list_clear, param);
    }
    else
    {
        TRACE_MSG(TRACE_APP1, "test finished, g_test_step %hd", (FMT__H, g_test_step));

        zb_buf_free(param);
    }
}

/***************** MAC Callbacks *****************/
ZB_MLME_RESET_CONFIRM(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, "MLME-RESET.confirm()", (FMT__0));

    ZB_SCHEDULE_CALLBACK(test_set_ieee_addr, param);
}

ZB_MLME_ASSOCIATE_CONFIRM(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, "MLME-ASSOCIATE.confirm()", (FMT__0));

    ZB_SCHEDULE_CALLBACK(test_incr_test_step, param);
}

ZB_MLME_START_CONFIRM(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, "MLME-START.confirm()", (FMT__0));

    ZB_SCHEDULE_CALLBACK(test_incr_test_step, param);
}

ZB_MLME_BEACON_NOTIFY_INDICATION(zb_uint8_t param)
{
    zb_mac_beacon_notify_indication_t *ind = (zb_mac_beacon_notify_indication_t *)zb_buf_begin(param);

    TRACE_MSG(TRACE_APP1, "MLME-BEACON-NOTIFY.indication()", (FMT__0));

    if (ind->beacon_type == ZB_MAC_BEACON_TYPE_ENHANCED_BEACON
            && (ind->pan_descriptor).coord_pan_id == TEST_PAN_ID
            && ZB_IEEE_ADDR_CMP((ind->pan_descriptor).coord_address.addr_long, g_ieee_addr_dut1))
    {
        TRACE_MSG(TRACE_APP1, "coord_addr_mode %hi, coord_pan_id 0x%x, coord_addr " TRACE_FORMAT_64 ", beacon_type %hd, enh_beacon_nwk_addr 0x%x", (FMT__H_D_A_H_D, (ind->pan_descriptor).coord_addr_mode, (ind->pan_descriptor).coord_pan_id, TRACE_ARG_64((ind->pan_descriptor).coord_address.addr_long), ind->beacon_type, (ind->pan_descriptor).enh_beacon_nwk_addr));

        g_beacon_received = ZB_TRUE;
    }
    else
    {
        g_beacon_received = ZB_FALSE;
    }

    zb_buf_free(param);
}

ZB_MLME_SCAN_CONFIRM(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, "MLME-SCAN.confirm()", (FMT__0));

    TRACE_MSG(TRACE_APP1, "test step %hd, g_beacon_received %hd", (FMT__H_H, g_test_step, g_beacon_received));

    if (g_beacon_received)
    {
        g_beacon_received = ZB_FALSE;
        ZB_SCHEDULE_CALLBACK(test_associate_request, param);
    }
    else if ((g_test_step == DUT1_DUT2_JP_ALLJOIN || g_test_step == DUT1_DUT2_JP_IEEELISTJOIN_INLIST) && !g_beacon_received)
    {
        ZB_SCHEDULE_CALLBACK(test_mlme_scan_request, param);
    }
    else
    {
        ZB_SCHEDULE_CALLBACK(test_incr_test_step, param);
    }
}

ZB_MLME_ASSOCIATE_INDICATION(zb_uint8_t param)
{
    zb_ieee_addr_t device_address;
    zb_mlme_associate_indication_t *request = ZB_BUF_GET_PARAM((zb_bufid_t )param, zb_mlme_associate_indication_t);

    TRACE_MSG(TRACE_APP1, "MLME-ASSOCIATE.indication()", (FMT__0));
    /*
      Very simple implementation: accept anybody, assign address 0x1122
     */
    ZB_IEEE_ADDR_COPY(device_address, request->device_address);

    TRACE_MSG(TRACE_APP1, "MLME-ASSOCIATE.response()", (FMT__0));

    ZB_MLME_BUILD_ASSOCIATE_RESPONSE(param, device_address, TEST_DUT1_FFD1_SHORT_ADDRESS, 0);

    ZB_SCHEDULE_CALLBACK(zb_mlme_associate_response, param);
}

ZB_MLME_COMM_STATUS_INDICATION(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, "MLME-COMM-STATUS.indication()", (FMT__0));

    zb_buf_free(param);
}

/*! @} */
