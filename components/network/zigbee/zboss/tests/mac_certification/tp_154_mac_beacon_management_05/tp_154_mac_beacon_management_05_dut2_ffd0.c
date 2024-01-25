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
/*  PURPOSE: TP/154/MAC/BEACON-MANAGEMENT-05 DUT2 FFD0 implementation
*/


#define ZB_TEST_NAME TP_154_MAC_BEACON_MANAGEMENT_05_DUT2_FFD0
#define ZB_TRACE_FILE_ID 63841
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_mac_globals.h"
#include "zb_ie.h"
#include "../common/zb_cert_test_globals.h"
#include "tp_154_mac_beacon_management_05_common.h"

#ifndef ZB_MULTI_TEST
#define USE_ZB_MLME_RESET_CONFIRM
#define USE_ZB_MLME_START_CONFIRM
#define USE_ZB_MLME_BEACON_NOTIFY_INDICATION
#define USE_ZB_MLME_SCAN_CONFIRM
#define USE_ZB_MCPS_DATA_INDICATION
#define USE_ZB_MCPS_DATA_CONFIRM

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
static void test_set_short_addr(zb_uint8_t param);
static void test_set_beacon_payload_length(zb_uint8_t param);
static void test_set_beacon_payload(zb_uint8_t param);
static void test_set_rx_on_when_idle(zb_uint8_t param);
static void test_set_association_permit(zb_uint8_t param);
static void test_clear_mac_auto_request(zb_uint8_t param);
static void test_mlme_start_request(zb_uint8_t param);
static void test_incr_test_step(zb_uint8_t param);
static void test_mlme_scan_request(zb_uint8_t param);
static void test_prepare_enhanced_beacon_rejoin(zb_uint8_t param);
static void test_rejoin_req_send_pkt(zb_uint8_t param);
static void test_rejoin_resp_send_pkt(zb_uint8_t param);

static zb_ieee_addr_t g_ieee_addr_dut1 = TEST_DUT1_FFD1_IEEE_ADDR;
static zb_ieee_addr_t g_ieee_addr_dut2 = TEST_DUT2_FFD0_IEEE_ADDR;
static zb_uint8_t g_test_step = TEST_STEP_INITIAL;
static zb_bool_t g_beacon_received = ZB_FALSE;

MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("tp_154_mac_beacon_management_05_dut2_ffd0");

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

    req->confirm_cb_u.cb = test_set_short_addr;

    zb_mlme_set_request(param);
}

static void test_set_short_addr(zb_uint8_t param)
{
    zb_mlme_set_request_t *req;
    zb_uint16_t short_addr = TEST_DUT2_FFD0_SHORT_ADDRESS;

    TRACE_MSG(TRACE_APP1, "MLME-SET.request() mac short addr", (FMT__0));

    req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));
    ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));

    req->pib_attr = ZB_PIB_ATTRIBUTE_SHORT_ADDRESS;
    req->pib_length = sizeof(zb_uint16_t);
    ZB_MEMCPY((zb_uint8_t *)(req + 1), &short_addr, sizeof(zb_uint16_t));

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

static void test_incr_test_step(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, "test_incr_test_step()", (FMT__0));

    g_test_step++;

    if (g_test_step < TEST_STEP_FINISHED)
    {
        ZB_SCHEDULE_CALLBACK(test_mlme_scan_request, param);
    }
    else
    {
        TRACE_MSG(TRACE_APP1, "test finished, g_test_step %hd", (FMT__H, g_test_step));

        zb_buf_free(param);
    }
}

static void test_mlme_scan_request(zb_uint8_t param)
{

    TRACE_MSG(TRACE_APP1, "MLME-SCAN.request()", (FMT__0));

    ZB_MLME_BUILD_SCAN_REQUEST(param, TEST_PAGE, TEST_CHANNEL_MASK, TEST_SCAN_TYPE, TEST_SCAN_DURATION);
    test_prepare_enhanced_beacon_rejoin(param);

    ZB_SCHEDULE_ALARM(zb_mlme_scan_request, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(TEST_TIMEOUT));

}

static void test_prepare_enhanced_beacon_rejoin(zb_uint8_t param)
{
    zb_uint8_t pie_len;
    zb_uint8_t *ptr;
    zb_ext_pan_id_t ext_pan_id;
    zb_uint16_t short_addr = TEST_DUT2_FFD0_SHORT_ADDRESS;

    if (g_test_step == DUT1_DUT2_NIB_SAME)
    {
        ZB_EXTPANID_COPY(ext_pan_id, g_ieee_addr_dut1);
    }
    else
    {
        ZB_EXTPANID_ZERO(ext_pan_id);
    }

    TRACE_MSG(TRACE_APP1, ">> test_prepare_enhanced_beacon_rejoin param %hd", (FMT__H, param));

    pie_len = ZB_PIE_VENDOR_HEADER_LENGTH + ZB_TX_POWER_IE_DESCRIPTOR_LEN + ZB_ZIGBEE_PIE_HEADER_LENGTH + 8 + 2;

    ptr = zb_buf_initial_alloc((zb_bufid_t )param,
                               pie_len + ZB_MLME_SCAN_REQUEST_IE_SIZES_HDR_LEN);
    ZB_MLME_SCAN_REQUEST_SET_IE_SIZES_HDR(ptr, 0, pie_len);
    ptr += ZB_MLME_SCAN_REQUEST_IE_SIZES_HDR_LEN;

    /* Vendor PIE */
    ZB_SET_NEXT_PIE_ZIGBEE_VENDOR_HEADER(ptr, ZB_TX_POWER_IE_DESCRIPTOR_LEN
                                         + ZB_ZIGBEE_PIE_HEADER_LENGTH + 8 + 2);

    ZB_SET_NEXT_ZIGBEE_PIE_HEADER(ptr, ZB_ZIGBEE_PIE_SUB_ID_REJOIN, 8 + 2);
    ZB_HTOLE64(ptr, ext_pan_id);
    ptr += 8;
    ZB_PUT_NEXT_HTOLE16(ptr, short_addr);

    ZB_SET_NEXT_TX_POWER_IE_DESCRIPTOR(ptr, MAC_CTX().current_tx_power);

    TRACE_MSG(TRACE_APP1, "<< test_prepare_enhanced_beacon_rejoin", (FMT__0));
}

static void test_rejoin_req_send_pkt(zb_uint8_t param)
{
    zb_mcps_data_req_params_t *mac_params = ZB_BUF_GET_PARAM(param, zb_mcps_data_req_params_t);
    zb_nwk_hdr_t *nwhdr;
    zb_uint8_t *cmd_id;
    zb_nwk_rejoin_request_t *rejoin_request;
    zb_uint16_t dut1_short_addr = TEST_DUT1_FFD1_SHORT_ADDRESS;
    zb_uint16_t dut2_short_addr = TEST_DUT2_FFD0_SHORT_ADDRESS;
    zb_uint8_t nwhdr_size = 0;

    TRACE_MSG(TRACE_APP1, ">>test_rejoin_req_send_pkt", (FMT__0));

    zb_buf_reuse(param);
    nwhdr_size = ZB_OFFSETOF(zb_nwk_hdr_t, mcast_control);
    nwhdr = zb_buf_initial_alloc(param, nwhdr_size);

    ZB_NWK_FRAMECTL_SET_FRAME_TYPE_N_PROTO_VER(nwhdr->frame_control, ZB_NWK_FRAME_TYPE_COMMAND, ZB_PROTOCOL_VERSION);
    ZB_NWK_FRAMECTL_SET_DISCOVER_ROUTE(nwhdr->frame_control, 0);
    ZB_NWK_FRAMECTL_SET_SECURITY(nwhdr->frame_control, ZB_FALSE);
    ZB_NWK_FRAMECTL_SET_SRC_DEST_IEEE(nwhdr->frame_control, ZB_TRUE, ZB_TRUE);
    ZB_NWK_FRAMECTL_SET_PROTOCOL_VERSION(nwhdr->frame_control, 2);

    nwhdr->dst_addr = dut1_short_addr;
    nwhdr->src_addr = dut2_short_addr;
    nwhdr->radius = 1;
    nwhdr->seq_num = 200;

    ZB_64BIT_ADDR_COPY(nwhdr->dst_ieee_addr, g_ieee_addr_dut1);
    ZB_64BIT_ADDR_COPY(nwhdr->src_ieee_addr, g_ieee_addr_dut2);

    // Set Command Identifier
    cmd_id = zb_buf_alloc_right(param, sizeof(zb_uint8_t));
    *cmd_id = ZB_NWK_CMD_REJOIN_REQUEST;

    // Set Capability Information
    rejoin_request = zb_buf_alloc_right(param, sizeof(zb_nwk_rejoin_request_t));
    rejoin_request->capability_information = TEST_ASSOCIATION_CAP_INFO;

    mac_params->src_addr.addr_short = dut2_short_addr;
    mac_params->dst_addr.addr_short = dut1_short_addr;
    mac_params->src_addr_mode = mac_params->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
    mac_params->tx_options = 1;
    mac_params->dst_pan_id = TEST_PAN_ID;
    mac_params->msdu_handle = param;

    ZB_SCHEDULE_CALLBACK(zb_mcps_data_request, param);

    TRACE_MSG(TRACE_APP1, "<<test_rejoin_req_send_pkt", (FMT__0));
}

static void test_rejoin_resp_send_pkt(zb_uint8_t param)
{
    zb_mcps_data_req_params_t *mac_params = ZB_BUF_GET_PARAM(param, zb_mcps_data_req_params_t);
    zb_nwk_hdr_t *nwhdr;
    zb_uint8_t *cmd_id;
    zb_nwk_rejoin_response_t *rejoin_response;
    zb_uint16_t dut1_short_addr = TEST_DUT1_FFD1_SHORT_ADDRESS;
    zb_uint16_t dut2_short_addr = TEST_DUT2_FFD0_SHORT_ADDRESS;
    zb_uint8_t nwhdr_size = 0;

    TRACE_MSG(TRACE_APP1, ">>test_rejoin_resp_send_pkt", (FMT__0));

    zb_buf_reuse(param);
    nwhdr_size = ZB_OFFSETOF(zb_nwk_hdr_t, mcast_control);
    nwhdr = zb_buf_initial_alloc(param, nwhdr_size);

    ZB_NWK_FRAMECTL_SET_FRAME_TYPE_N_PROTO_VER(nwhdr->frame_control, ZB_NWK_FRAME_TYPE_COMMAND, ZB_PROTOCOL_VERSION);
    ZB_NWK_FRAMECTL_SET_DISCOVER_ROUTE(nwhdr->frame_control, 0);
    ZB_NWK_FRAMECTL_SET_SECURITY(nwhdr->frame_control, ZB_FALSE);
    ZB_NWK_FRAMECTL_SET_SRC_DEST_IEEE(nwhdr->frame_control, ZB_TRUE, ZB_TRUE);
    ZB_NWK_FRAMECTL_SET_PROTOCOL_VERSION(nwhdr->frame_control, 2);

    nwhdr->dst_addr = dut1_short_addr;
    nwhdr->src_addr = dut2_short_addr;
    nwhdr->radius = 1;
    nwhdr->seq_num = 201;

    ZB_64BIT_ADDR_COPY(nwhdr->dst_ieee_addr, g_ieee_addr_dut1);
    ZB_64BIT_ADDR_COPY(nwhdr->src_ieee_addr, g_ieee_addr_dut2);

    // Set Command Identifier
    cmd_id = zb_buf_alloc_right(param, sizeof(zb_uint8_t));
    *cmd_id = ZB_NWK_CMD_REJOIN_RESPONSE;

    // Set Address and Rejoin Status
    rejoin_response = zb_buf_alloc_right(param, sizeof(zb_nwk_rejoin_response_t));
    rejoin_response->network_addr = dut1_short_addr;
    rejoin_response->rejoin_status = ZB_NWK_STATUS_SUCCESS;

    mac_params->src_addr.addr_short = dut2_short_addr;
    mac_params->dst_addr.addr_short = dut1_short_addr;
    mac_params->src_addr_mode = mac_params->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
    mac_params->tx_options = 1;
    mac_params->dst_pan_id = TEST_PAN_ID;
    mac_params->msdu_handle = param;

    ZB_SCHEDULE_CALLBACK(zb_mcps_data_request, param);

    TRACE_MSG(TRACE_APP1, "<<test_rejoin_resp_send_pkt", (FMT__0));
}

/***************** MAC Callbacks *****************/
ZB_MLME_RESET_CONFIRM(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, "MLME-RESET.confirm()", (FMT__0));

    ZB_SCHEDULE_CALLBACK(test_set_ieee_addr, param);
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
            && (ind->pan_descriptor).enh_beacon_nwk_addr == TEST_DUT1_FFD1_SHORT_ADDRESS
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
    TRACE_MSG(TRACE_APP1, "MLME-SCAN.confirm(), test step %hd, g_beacon_received %hd", (FMT__H_H, g_test_step, g_beacon_received));

    if (g_beacon_received)
    {
        ZB_SCHEDULE_CALLBACK(test_rejoin_req_send_pkt, param);
    }
    else if (g_test_step == DUT1_DUT2_NIB_SAME && !g_beacon_received)
    {
        ZB_SCHEDULE_CALLBACK(test_mlme_scan_request, param);
    }
    else
    {
        ZB_SCHEDULE_CALLBACK(test_incr_test_step, param);
    }
}

ZB_MCPS_DATA_INDICATION(zb_uint8_t param)
{
    zb_ushort_t mhr_size;
    zb_mac_mhr_t mac_hdr;
    zb_nwk_hdr_t *nwk_hdr = NULL;
    zb_ushort_t hdr_size;

    TRACE_MSG(TRACE_APP1, "MCPS-DATA.indication()", (FMT__0));

    if (g_beacon_received)
    {
        TRACE_MSG(TRACE_APP1, "test step %hd, g_beacon_received %hd", (FMT__H_H, g_test_step, g_beacon_received));

        g_beacon_received = ZB_FALSE;
    }
    else
    {
        /* parse and remove MAC header */
        mhr_size = zb_parse_mhr(&mac_hdr, param);
        ZB_MAC_CUT_HDR(param, mhr_size, nwk_hdr);

        if (nwk_hdr->dst_addr == TEST_DUT2_FFD0_SHORT_ADDRESS && ZB_IEEE_ADDR_CMP(nwk_hdr->dst_ieee_addr, g_ieee_addr_dut2))
        {
            TRACE_MSG(TRACE_APP1, "dst_addr 0x%x, src_addr 0x%x dst_ieee_addr " TRACE_FORMAT_64 " src_ieee_addr " TRACE_FORMAT_64, (FMT__D_D_A_A, nwk_hdr->dst_addr, nwk_hdr->src_addr, TRACE_ARG_64(nwk_hdr->dst_ieee_addr), TRACE_ARG_64((nwk_hdr)->src_ieee_addr)));

            hdr_size = ZB_OFFSETOF(zb_nwk_hdr_t, mcast_control);

            if (ZB_NWK_FRAMECTL_GET_FRAME_TYPE(nwk_hdr->frame_control) == ZB_NWK_FRAME_TYPE_COMMAND && ZB_NWK_CMD_FRAME_GET_CMD_ID(param, hdr_size) == ZB_NWK_CMD_REJOIN_REQUEST)
            {
                TRACE_MSG(TRACE_APP1, "test step %hd, call test_rejoin_resp_send_pkt()", (FMT__H, g_test_step));

                ZB_SCHEDULE_CALLBACK(test_rejoin_resp_send_pkt, param);
            }
        }
    }
}

ZB_MCPS_DATA_CONFIRM(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, "MCPS-DATA.confirm()", (FMT__0));

    if (g_beacon_received)
    {
        TRACE_MSG(TRACE_APP1, "test step %hd, call test_mlme_scan_request()", (FMT__H, g_test_step));

        ZB_SCHEDULE_CALLBACK(test_incr_test_step, param);
    }
    else
    {
        zb_buf_free(param);
    }
}

/*! @} */
