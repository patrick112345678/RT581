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
/* PURPOSE: TH ZR
*/
#define ZB_TEST_NAME RTP_NVRAM_02_TH_ZR
#define ZB_TRACE_FILE_ID 40428

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"

#include "rtp_nvram_02_common.h"
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

static zb_ieee_addr_t g_ieee_addr_th_zr = IEEE_ADDR_TH_ZR;

static void test_send_mgmt_lqi_req(zb_uint8_t param);
static void trigger_steering(zb_uint8_t unused);

MAIN()
{
    ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
    ZB_SET_TRACE_LEVEL(4);
    ARGV_UNUSED;

    ZB_INIT("zdo_th_zr");

    zb_set_long_address(g_ieee_addr_th_zr);

    zb_reg_test_set_common_channel_settings();
    zb_set_network_router_role((1l << TEST_CHANNEL));
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

            test_step_register(test_send_mgmt_lqi_req, 0, RTP_NVRAM_02_STEP_1_TIME_ZR);

            test_control_start(TEST_MODE, RTP_NVRAM_02_STEP_1_DELAY_ZR);
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


static void test_send_mgmt_lqi_req(zb_uint8_t param)
{
    zb_zdo_mgmt_lqi_param_t *req_param;
    zb_uint16_t dst_addr = 0x0000;

    if (param == ZB_BUF_INVALID)
    {
        zb_buf_get_out_delayed(test_send_mgmt_lqi_req);
        return;
    }

    TRACE_MSG(TRACE_APP1, ">> test_send_mgmt_lqi_req buf = %d", (FMT__D, param));

    req_param = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_lqi_param_t);

    req_param->dst_addr = dst_addr;
    req_param->start_index = 0;

    zb_zdo_mgmt_lqi_req(param, NULL);

    TRACE_MSG(TRACE_APP1, "<< test_send_mgmt_lqi_req", (FMT__0));
}

/*! @} */
