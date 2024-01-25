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
/* PURPOSE: CN-NSA-TC-01D: Handling of unicast Mgmt_Permit_Joining_req (THE1)
*/

#define ZB_TEST_NAME TP_BDB_CN_NSA_TC_01D_THE1
#define ZB_TRACE_FILE_ID 40655
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"

#include "tp_bdb_cn_nsa_tc_01d_common.h"
#include "../common/zb_cert_test_globals.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */


enum test_step_e
{
    TEST_STEP_MGMT_PERMIT_JOIN_180_THR1,
    TEST_STEP_MGMT_PERMIT_JOIN_180_DUT,
    TEST_STEP_MGMT_PERMIT_JOIN_00_DUT
};

static const zb_ieee_addr_t g_ieee_addr_the1 = IEEE_ADDR_THE1;
static const zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;
static const zb_ieee_addr_t g_ieee_addr_thr1 = IEEE_ADDR_THR1;

static int s_current_test_step;
static zb_uint8_t s_permit_duration;
static zb_uint16_t s_dest_addr;
static zb_uint16_t s_dut_short_addr;
static zb_uint16_t s_thr1_short_addr;
static zb_ieee_addr_t s_dest_ieee_addr;

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
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    zb_set_long_address(g_ieee_addr_the1);
    zb_set_rx_on_when_idle(ZB_TRUE);

    zb_set_network_ed_role((1l << TEST_CHANNEL));
    zb_set_nvram_erase_at_start(ZB_TRUE);

    if (zboss_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
    }
    else
    {
        zdo_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    switch (sig)
    {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        if (status == 0)
        {
            TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));

            ZB_IEEE_ADDR_COPY(s_dest_ieee_addr, g_ieee_addr_dut);
            ZB_SCHEDULE_ALARM(test_start_get_peer_addr, 0, TEST_ZED1_GET_PEER_ADDR_REQ_DELAY);

            ZB_SCHEDULE_ALARM(test_logic_iteration, 0, TEST_ZED1_MGMT_PERMIT_JOIN_180_THR1_DELAY);
        }
        else
        {
            TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, status));
        }
        break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    default:
        if (status == 0)
        {
            TRACE_MSG(TRACE_APS1, "Unknown signal, status OK", (FMT__0));
        }
        else
        {
            TRACE_MSG(TRACE_ERROR, "Unknown signal, status %d", (FMT__D, status));
        }
        break;
    }

    zb_buf_free(param);
}

static void test_logic_iteration(zb_uint8_t param)
{
    int stop_test = 0;
    zb_time_t next_step_delay = 0;

    ZVUNUSED(param);
    TRACE_MSG(TRACE_ZDO1, ">>test_logic_iteration: step = %d", (FMT__D, s_current_test_step));

    switch (s_current_test_step)
    {
    case TEST_STEP_MGMT_PERMIT_JOIN_180_THR1:
        s_dest_addr = s_thr1_short_addr;
        s_permit_duration = 0xb4;
        zb_buf_get_out_delayed(send_mgmt_permit_join);
        next_step_delay = TEST_ZED1_MGMT_PERMIT_JOIN_180_DUT_DELAY * 3;
        break;

    case TEST_STEP_MGMT_PERMIT_JOIN_180_DUT:
        s_dest_addr = s_dut_short_addr;
        s_permit_duration = 0xb4;
        zb_buf_get_out_delayed(send_mgmt_permit_join);
        next_step_delay = TEST_ZED1_MGMT_PERMIT_JOIN_00_DUT_DELAY;
        break;

    case TEST_STEP_MGMT_PERMIT_JOIN_00_DUT:
        s_permit_duration = 0x00;
        zb_buf_get_out_delayed(send_mgmt_permit_join);
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

    TRACE_MSG(TRACE_ZDO1, "<<test_logic_iteration", (FMT__0));
}

static void send_mgmt_permit_join(zb_uint8_t param)
{
    zb_zdo_mgmt_permit_joining_req_param_t *req;

    TRACE_MSG(TRACE_ZDO2, ">>send_mgmt_permit_join: dest = 0x%x", (FMT__H, s_dest_addr));

    req = zb_buf_get_tail(param, sizeof(zb_zdo_mgmt_permit_joining_req_param_t));
    req->dest_addr = s_dest_addr;
    req->tc_significance = 1;
    req->permit_duration = s_permit_duration;
    zb_zdo_mgmt_permit_joining_req(param, NULL);

    TRACE_MSG(TRACE_ZDO2, "<<send_mgmt_permit_join", (FMT__0));
}


static void test_start_get_peer_addr(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    zb_buf_get_out_delayed(test_get_peer_addr_req);
}


static void test_get_peer_addr_req(zb_uint8_t param)
{
    zb_zdo_nwk_addr_req_param_t *req_param = ZB_BUF_GET_PARAM(param, zb_zdo_nwk_addr_req_param_t);

    TRACE_MSG(TRACE_ZDO1, ">>test_get_peer_addr_req", (FMT__0));

    req_param->dst_addr = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
    req_param->start_index = 0;
    req_param->request_type = ZB_ZDO_SINGLE_DEVICE_RESP;
    ZB_IEEE_ADDR_COPY(req_param->ieee_addr, s_dest_ieee_addr);
    zb_zdo_nwk_addr_req(param, test_get_peer_addr_resp);

    TRACE_MSG(TRACE_ZDO1, "<<test_get_peer_addr_req", (FMT__0));
}

static void test_get_peer_addr_resp(zb_uint8_t param)
{
    zb_zdo_nwk_addr_resp_head_t *resp = (zb_zdo_nwk_addr_resp_head_t *) zb_buf_begin(param);

    TRACE_MSG(TRACE_ZDO1, ">>test_get_peer_addr_resp: status = %d",
              (FMT__D_D, resp->status));

    if (resp->status == ZB_ZDP_STATUS_SUCCESS)
    {
        if (ZB_IEEE_ADDR_CMP(s_dest_ieee_addr, g_ieee_addr_dut))
        {
            ZB_LETOH16(&s_dut_short_addr, &resp->nwk_addr);
            ZB_IEEE_ADDR_COPY(s_dest_ieee_addr, g_ieee_addr_thr1);
            zb_buf_get_out_delayed(test_get_peer_addr_req);
        }
        else if (ZB_IEEE_ADDR_CMP(s_dest_ieee_addr, g_ieee_addr_thr1))
        {
            ZB_LETOH16(&s_thr1_short_addr, &resp->nwk_addr);
        }
    }
    zb_buf_free(param);

    TRACE_MSG(TRACE_ZDO1, "<<test_get_peer_addr_resp", (FMT__0));
}

/*! @} */
