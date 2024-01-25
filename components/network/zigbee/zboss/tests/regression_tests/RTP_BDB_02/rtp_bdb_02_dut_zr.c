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
/* PURPOSE: DUT ZR
*/

#define ZB_TEST_NAME RTP_BDB_02_DUT_ZR

#define ZB_TRACE_FILE_ID 40261
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"

#include "rtp_bdb_02_common.h"
#include "device_dut.h"
#include "../common/zb_reg_test_globals.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_dut_zr = IEEE_ADDR_DUT_ZR;

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error define ZB_USE_NVRAM
#endif

#define ENDPOINT 123

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(rtp_bdb_02_dut_zr_basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(rtp_bdb_02_dut_zr_identify_attr_list, &attr_identify_time);

/********************* Declare device **************************/
DECLARE_DUT_CLUSTER_LIST(rtp_bdb_02_dut_zr_device_clusters,
                         rtp_bdb_02_dut_zr_basic_attr_list,
                         rtp_bdb_02_dut_zr_identify_attr_list);

DECLARE_DUT_EP(rtp_bdb_02_dut_zr_device_ep,
               ENDPOINT,
               rtp_bdb_02_dut_zr_device_clusters);

DECLARE_DUT_CTX(rtp_bdb_02_dut_zr_device_ctx, rtp_bdb_02_dut_zr_device_ep);

static void trigger_steering(zb_uint8_t unused);
static void trigger_fb_target(zb_uint8_t unused);
static void start_test(zb_uint8_t unused);
static void identify_handler(zb_uint8_t param);

MAIN()
{
    ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
    ZB_SET_TRACE_LEVEL(4);
    ARGV_UNUSED;

    ZB_INIT("zdo_dut_zr");

    zb_set_long_address(g_ieee_addr_dut_zr);

    zb_reg_test_set_common_channel_settings();
    zb_set_network_router_role((1l << TEST_CHANNEL));
    zb_set_nvram_erase_at_start(ZB_TRUE);

    ZB_AF_REGISTER_DEVICE_CTX(&rtp_bdb_02_dut_zr_device_ctx);

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
            ZB_SCHEDULE_CALLBACK(start_test, 0);
        }
        break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
        if (status == 0)
        {
            ZB_SCHEDULE_CALLBACK(trigger_fb_target, 0);
        }
        break; /* ZB_BDB_SIGNAL_STEERING */

    case ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED:
        TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED, status %d", (FMT__D, status));
        if (status == 0)
        {
        }
        break; /* ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED */

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

static void trigger_fb_target(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    ZB_BDB().bdb_commissioning_time = DUT_ZR_FB_DURATION;
    zb_bdb_finding_binding_target(ENDPOINT);
}

static void start_test(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    ZB_AF_SET_IDENTIFY_NOTIFICATION_HANDLER(ENDPOINT, identify_handler);
}

static void identify_handler(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">>identify_handler: param = %d", (FMT__D, param));

    if (param == ZB_FALSE)
    {
        TRACE_MSG(TRACE_APP1, "identify_handler(): identification is cancelled on an endpoint - test OK",
                  (FMT__0));
    }

    TRACE_MSG(TRACE_APP1, "<<identify_handler", (FMT__0));
}

/*! @} */
