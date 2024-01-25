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
/* PURPOSE: TH ZC
*/

#define ZB_TEST_NAME RTP_INT_01_TH_ZC

#define ZB_TRACE_FILE_ID 40308
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"

#include "rtp_int_01_common.h"
#include "../common/zb_reg_test_globals.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error define ZB_USE_NVRAM
#endif

static zb_uint8_t g_nwk_key[16] = ZB_REG_TEST_DEFAULT_NWK_KEY;
static zb_ieee_addr_t g_ieee_addr_th_zc = IEEE_ADDR_TH_ZC;
static zb_ieee_addr_t g_ieee_addr_dut_zr = IEEE_ADDR_DUT_ZR;

static void trigger_steering(zb_uint8_t unused);
static void test_send_buffer_test_request_delayed(zb_uint8_t unused);
static void test_send_buffer_test_request(zb_uint8_t param);

static zb_uint8_t g_buffer_test_request_data_size = 8;

MAIN()
{
    ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
    ZB_SET_TRACE_LEVEL(4);
    ARGV_UNUSED;

    ZB_INIT("zdo_th_zc");

    zb_set_pan_id(0x1aaa);

    zb_secur_setup_nwk_key((zb_uint8_t *) g_nwk_key, 0);

    zb_reg_test_set_common_channel_settings();
    zb_set_network_coordinator_role((1l << TEST_CHANNEL));
    zb_set_long_address(g_ieee_addr_th_zc);
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
        TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));
        if (status == 0)
        {
            ZB_SCHEDULE_CALLBACK(trigger_steering, 0);

            ZB_SCHEDULE_ALARM(test_send_buffer_test_request_delayed, 0, (ZB_TIME_ONE_SECOND * 20));
        }
        break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    default:
        TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}

static void trigger_steering(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
}

static void test_send_buffer_test_request_delayed(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    zb_buf_get_out_delayed(test_send_buffer_test_request);
}

static void test_send_buffer_test_request(zb_uint8_t param)
{
    zb_bufid_t buf = param;
    zb_buffer_test_req_param_t *req_param;

    TRACE_MSG(TRACE_APP1, ">>send_buffer_test_request", (FMT__0));

    req_param = ZB_BUF_GET_PARAM(buf, zb_buffer_test_req_param_t);
    BUFFER_TEST_REQ_SET_DEFAULT(req_param);
    req_param->dst_addr = zb_address_short_by_ieee(g_ieee_addr_dut_zr);
    req_param->len = g_buffer_test_request_data_size;

    TRACE_MSG(TRACE_APP1, "send buffer test request to 0x%x", (FMT__H, req_param->dst_addr));

    zb_tp_buffer_test_request(buf, NULL);

    ZB_SCHEDULE_ALARM(test_send_buffer_test_request_delayed, 0, (ZB_TIME_ONE_SECOND * 10));

    g_buffer_test_request_data_size++;
    TRACE_MSG(TRACE_APP1, "<<send_buffer_test_request", (FMT__0));
}

/*! @} */
