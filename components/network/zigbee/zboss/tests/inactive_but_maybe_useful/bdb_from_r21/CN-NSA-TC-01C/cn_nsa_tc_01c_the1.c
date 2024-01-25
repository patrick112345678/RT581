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
/* PURPOSE:
*/


#define ZB_TEST_NAME CN_NSA_TC_01C_THE1
#define ZB_TRACE_FILE_ID 41122
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"
#include "cn_nsa_tc_01c_common.h"
/* for zb_ret_t zb_beacon_request_command(); */
#include "mac_internal.h"


/*! \addtogroup ZB_TESTS */
/*! @{ */


enum test_step_e
{
    TEST_STEP_TRIGGER_STEERING,
    /* send Mgmt_Permit_Join with duration = 0x00 */
    TEST_STEP_MGMT_PERMIT_JOIN_00,
    TEST_STEP_MGMT_PERMIT_JOIN_SHORT,
    TEST_STEP_CHECK_SHORT_DURATION,
    TEST_STEP_MGMT_PERMIT_JOIN_LONG,
    TEST_STEP_CHECK_LONG_DURATION,
    TEST_STEP_CHECK_DUT_STEERING,
    TEST_STEP_UNICAST_MGMT_TO_THR1_180,
    TEST_STEP_UNICAST_MGMT_TO_DUT_180,
    TEST_STEP_UNICAST_MGMT_TO_DUT_0,
    TEST_STEP_UNICAST_MGMT_TO_THR1_0
};


static void zb_mac_send_beacon_request_command(zb_uint8_t unused);


static int s_current_test_step;
static zb_uint8_t s_permit_duration;
static zb_uint16_t s_dut_short_addr;
static zb_uint16_t s_thr1_short_addr;
static zb_uint16_t s_dest_addr;
static zb_ieee_addr_t s_dest_ieee_addr;


static void send_beacon_req_delayed(zb_uint8_t unused);
static void send_beacon_req(zb_uint8_t unused);
static void send_mgmt_permit_join(zb_uint8_t param);
static void test_logic_iteration(zb_uint8_t param);
static void test_start_get_peer_addr(zb_uint8_t unused);
static void test_get_peer_addr_req(zb_uint8_t param);
static void test_get_peer_addr_resp(zb_uint8_t param);


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_the1");


    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_the1);

    ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ED;
    ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);
    ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
    ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;
    ZB_BDB().bdb_mode = 1;

    if (zdo_dev_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
    }
    else
    {
        zdo_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}


static void send_beacon_req_delayed(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    ZB_GET_OUT_BUF_DELAYED(send_beacon_req);
}


static void send_beacon_req(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">>send_beacon_req", (FMT__0));

    ZB_MLME_BUILD_SCAN_REQUEST((zb_buf_t *)ZB_BUF_FROM_REF(param), ZB_BDB().bdb_primary_channel_set,
                               ACTIVE_SCAN, TEST_SCAN_DURATION);

    ZB_SCHEDULE_CALLBACK(zb_mlme_scan_request, param);

    TRACE_MSG(TRACE_APP1, "<<send_beacon_req", (FMT__0));
}


static void send_mgmt_permit_join(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_zdo_mgmt_permit_joining_req_param_t *req;

    TRACE_MSG(TRACE_ZDO2, ">>send_mgmt_permit_join: dest = 0x%x", (FMT__H, s_dest_addr));

    req = ZB_GET_BUF_TAIL(buf, sizeof(zb_zdo_mgmt_permit_joining_req_param_t));
    req->dest_addr = s_dest_addr;
    req->tc_significance = 1;
    req->permit_duration = s_permit_duration;
    zb_zdo_mgmt_permit_joining_req(param, NULL);

    TRACE_MSG(TRACE_ZDO2, "<<send_mgmt_permit_join", (FMT__0));
}


static void test_logic_iteration(zb_uint8_t param)
{
    int stop_test = 0;
    int need_beacon_req = 0; /* if set to one send beacon request after some delay */
    zb_time_t next_step_delay = 0;

    ZVUNUSED(param);
    TRACE_MSG(TRACE_ZDO1, ">>test_logic_iteration: step = %d", (FMT__D, s_current_test_step));

    switch (s_current_test_step)
    {
    case TEST_STEP_TRIGGER_STEERING:
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        need_beacon_req = 1;
        next_step_delay = TEST_ZED1_MGMT_PERMIT_JOIN_00;
        break;

    case TEST_STEP_MGMT_PERMIT_JOIN_00:
        s_dest_addr = 0xfffc;
        s_permit_duration = 0x00;
        need_beacon_req = 1;
        ZB_GET_OUT_BUF_DELAYED(send_mgmt_permit_join);
        next_step_delay = TEST_ZED1_MGMT_PERMIT_JOIN_SHORT;
        break;
    case TEST_STEP_MGMT_PERMIT_JOIN_SHORT:
        s_permit_duration = 0x0a;
        need_beacon_req = 1;
        ZB_GET_OUT_BUF_DELAYED(send_mgmt_permit_join);
        next_step_delay = TEST_ZED1_CHECK_SHORT_DURATION;
        break;
    case TEST_STEP_CHECK_SHORT_DURATION:
        send_beacon_req_delayed(0);
        next_step_delay = TEST_ZED1_MGMT_PERMIT_JOIN_LONG;
        break;

    case TEST_STEP_MGMT_PERMIT_JOIN_LONG:
        s_permit_duration = 0xfe;
        need_beacon_req = 1;
        ZB_GET_OUT_BUF_DELAYED(send_mgmt_permit_join);
        next_step_delay = TEST_ZED1_CHECK_LONG_DURATION;
        break;
    case TEST_STEP_CHECK_LONG_DURATION:
        send_beacon_req_delayed(0);
        next_step_delay = TEST_ZED1_CHECK_DUT_STEERING;
        break;

    case TEST_STEP_CHECK_DUT_STEERING:
        s_permit_duration = 0x00;
        need_beacon_req = 1;
        ZB_GET_OUT_BUF_DELAYED(send_mgmt_permit_join);
        next_step_delay = TEST_ZED1_UNICAST_MGMT_TO_THR1_180;
        break;

    case TEST_STEP_UNICAST_MGMT_TO_THR1_180:
        s_permit_duration = 180;
        need_beacon_req = 1;
        s_dest_addr = s_thr1_short_addr;
        ZB_GET_OUT_BUF_DELAYED(send_mgmt_permit_join);
        next_step_delay = TEST_ZED1_UNICAST_MGMT_TO_DUT_180;
        break;
    case TEST_STEP_UNICAST_MGMT_TO_DUT_180:
        s_permit_duration = 180;
        need_beacon_req = 1;
        s_dest_addr = s_dut_short_addr;
        ZB_GET_OUT_BUF_DELAYED(send_mgmt_permit_join);
        next_step_delay = TEST_ZED1_UNICAST_MGMT_TO_DUT_0;
        break;
    case TEST_STEP_UNICAST_MGMT_TO_DUT_0:
        s_permit_duration = 0;
        need_beacon_req = 1;
        s_dest_addr = s_dut_short_addr;
        ZB_GET_OUT_BUF_DELAYED(send_mgmt_permit_join);
        next_step_delay = TEST_ZED1_UNICAST_MGMT_TO_THR1_0;
        break;
    case TEST_STEP_UNICAST_MGMT_TO_THR1_0:
        s_permit_duration = 0;
        need_beacon_req = 1;
        s_dest_addr = s_thr1_short_addr;
        ZB_GET_OUT_BUF_DELAYED(send_mgmt_permit_join);
        next_step_delay = TEST_ZED1_UNICAST_MGMT_TO_DUT_0;
        break;

    default:
        stop_test = 1;
        break;
    }

    if (!stop_test)
    {
        ++s_current_test_step;
        ZB_SCHEDULE_ALARM(test_logic_iteration, 0, next_step_delay);
    }
    if (need_beacon_req)
    {
        ZB_SCHEDULE_ALARM(send_beacon_req_delayed, 0, TEST_ZED1_SEND_BEACON_REQ_DELAY);
    }

    TRACE_MSG(TRACE_ZDO1, "<<test_logic_iteration", (FMT__0));
}


static void test_start_get_peer_addr(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    ZB_GET_OUT_BUF_DELAYED(test_get_peer_addr_req);
}


static void test_get_peer_addr_req(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_zdo_nwk_addr_req_param_t *req_param = ZB_GET_BUF_PARAM(buf, zb_zdo_nwk_addr_req_param_t);
    zb_uint16_t cmd_dest;

    zb_address_short_by_ref(&cmd_dest, ZG->nwk.handle.parent);

    TRACE_MSG(TRACE_ZDO1, ">>test_get_peer_addr_req: parent_addr = %x",
              (FMT__H, cmd_dest));

    req_param->dst_addr = cmd_dest;
    req_param->start_index = 0;
    req_param->request_type = ZB_ZDO_SINGLE_DEVICE_RESP;
    ZB_IEEE_ADDR_COPY(req_param->ieee_addr, s_dest_ieee_addr);
    zb_zdo_nwk_addr_req(param, test_get_peer_addr_resp);

    TRACE_MSG(TRACE_ZDO1, "<<test_get_peer_addr_req", (FMT__0));
}


static void test_get_peer_addr_resp(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_zdo_nwk_addr_resp_head_t *resp = (zb_zdo_nwk_addr_resp_head_t *) ZB_BUF_BEGIN(buf);

    TRACE_MSG(TRACE_ZDO1, ">>test_get_peer_addr_resp: status = %d",
              (FMT__D_D, resp->status));

    if (resp->status == ZB_ZDP_STATUS_SUCCESS)
    {
        if (ZB_IEEE_ADDR_CMP(s_dest_ieee_addr, g_ieee_addr_dut))
        {
            ZB_LETOH16(&s_dut_short_addr, &resp->nwk_addr);
            ZB_IEEE_ADDR_COPY(s_dest_ieee_addr, g_ieee_addr_thr1);
            ZB_GET_OUT_BUF_DELAYED(test_get_peer_addr_req);
        }
        else if (ZB_IEEE_ADDR_CMP(s_dest_ieee_addr, g_ieee_addr_thr1))
        {
            ZB_LETOH16(&s_thr1_short_addr, &resp->nwk_addr);
        }
    }
    zb_free_buf(buf);

    TRACE_MSG(TRACE_ZDO1, "<<test_get_peer_addr_resp", (FMT__0));
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
            TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
            ZB_IEEE_ADDR_COPY(s_dest_ieee_addr, g_ieee_addr_dut);
            ZB_SCHEDULE_ALARM(test_start_get_peer_addr, 0,
                              TEST_ZED1_GET_PEER_ADDR_REQ_DELAY);
            ZB_SCHEDULE_ALARM(test_logic_iteration, 0, TEST_ZED1_STEERING_DELAY);
            break;
        default:
            TRACE_MSG(TRACE_APS1, "Unknown signal", (FMT__0));
        }
    }
    else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
    {
        TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
    }
    zb_free_buf(ZB_BUF_FROM_REF(param));
}


/*! @} */
