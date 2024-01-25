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
/* PURPOSE: TH ZED1
*/


#define ZB_TEST_NAME FB_PRE_TC_03B_THE1
#define ZB_TRACE_FILE_ID 41249
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_zcl.h"
#include "zb_bdb_internal.h"
#include "zb_zcl.h"
#include "fb_pre_tc_03b_common.h"
#include "test_target.h"


#ifndef ZB_ED_ROLE
#error ED role is not compiled!
#endif

#ifndef ZB_USE_NVRAM
#error ZB_USE_NVRAM is not compiled!
#endif


/*! \addtogroup ZB_TESTS */
/*! @{ */
/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &attr_identify_time);

static zb_bool_t attr_on_off = 1;
ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(on_off_attr_list, &attr_on_off);


/********************* Declare device **************************/
DECLARE_TARGET_CLUSTER_LIST(target_device_clusters,
                            basic_attr_list,
                            identify_attr_list,
                            on_off_attr_list);

DECLARE_TARGET_EP(target_device_ep, THR1_ENDPOINT, target_device_clusters);

DECLARE_TARGET_NO_REP_CTX(target_device_ctx, target_device_ep);


/**********************General definitions for test***********************/
static void trigger_fb_target(zb_uint8_t unused);


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_the1");


    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_the1);

    ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
    ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;
    ZB_BDB().bdb_mode = 1;
    ZB_AIB().aps_use_nvram = 1;

    ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ED;
    ZB_IEEE_ADDR_COPY(ZB_AIB().trust_center_address, g_unknown_ieee_addr);

    ZB_AF_REGISTER_DEVICE_CTX(&target_device_ctx);

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


/******************************Implementation********************************/
static void trigger_fb_target(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    ZB_BDB().bdb_commissioning_time = FB_DURATION;
    zb_bdb_finding_binding_target(THR1_ENDPOINT);
}


/*==============================ZDO Startup Complete===============================*/
ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
            TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
            ZB_SCHEDULE_ALARM(trigger_fb_target, 0, THE1_FB_FIRST_START_DELAY);
            break;

        case ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED:
            TRACE_MSG(TRACE_APS1, "Finding&binding done", (FMT__0));
            if (BDB_COMM_CTX().state == ZB_BDB_COMM_IDLE)
            {
                ZB_SCHEDULE_ALARM(trigger_fb_target, 0, THE1_FB_START_DELAY);
            }
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
