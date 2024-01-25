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
/* PURPOSE: DUT ZC (initiator)
*/


#define ZB_TEST_NAME FB_PRE_TC_04A_DUT_C
#define ZB_TRACE_FILE_ID 41177
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
#include "test_target.h"
#include "fb_pre_tc_04a_common.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */


#if defined(USE_NVRAM_IN_TEST) && !defined(ZB_USE_NVRAM)
#error ZB_USE_NVRAM is not compiled!
#endif

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif


/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version_epx[2]  =
{
    ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,
    ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE
};
static zb_uint8_t attr_power_source_epx[2] =
{
    ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE,
    ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE
};
/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time_epx[2];
/* On/Off cluster attributes data */
static zb_bool_t attr_on_off_epx[2] = {0, 0};

static zb_int16_t attr_temp_value = 0;


ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list_ep1,
                                 &attr_zcl_version_epx[0],
                                 &attr_power_source_epx[0]);
ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list_ep2,
                                 &attr_zcl_version_epx[1],
                                 &attr_power_source_epx[1]);

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list_ep1, &attr_identify_time_epx[0]);
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list_ep2, &attr_identify_time_epx[1]);

ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(on_off_attr_list_ep1, &attr_on_off_epx[0]);
ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(on_off_attr_list_ep2, &attr_on_off_epx[1]);

ZB_ZCL_DECLARE_TEMP_MEASUREMENT_ATTRIB_LIST(temp_meas_attr_list_ep2,
        &attr_temp_value, NULL, NULL, NULL);


/********************* Declare device **************************/
DECLARE_TARGET_CLUSTER_LIST_EP1(target_clusters_ep1,
                                basic_attr_list_ep1,
                                identify_attr_list_ep1,
                                on_off_attr_list_ep1);

DECLARE_TARGET_CLUSTER_LIST_EP2(target_clusters_ep2,
                                basic_attr_list_ep2,
                                identify_attr_list_ep2,
                                on_off_attr_list_ep2,
                                temp_meas_attr_list_ep2);

ZB_DECLARE_SIMPLE_DESC(3, 0);
ZB_DECLARE_SIMPLE_DESC(4, 0);


ZB_AF_SIMPLE_DESC_TYPE(3, 0) simple_desc_ep1 =
{
    DUT_ENDPOINT1,
    ZB_AF_HA_PROFILE_ID,
    DEVICE_ID_TARGET,
    DEVICE_VER_TARGET,
    0,
    3,
    0,
    {
        ZB_ZCL_CLUSTER_ID_BASIC,
        ZB_ZCL_CLUSTER_ID_IDENTIFY,
        ZB_ZCL_CLUSTER_ID_ON_OFF
    }
};

ZB_AF_SIMPLE_DESC_TYPE(4, 0) simple_desc_ep2 =
{
    DUT_ENDPOINT2,
    ZB_AF_HA_PROFILE_ID,
    DEVICE_ID_TARGET,
    DEVICE_VER_TARGET,
    0,
    4,
    0,
    {
        ZB_ZCL_CLUSTER_ID_BASIC,
        ZB_ZCL_CLUSTER_ID_IDENTIFY,
        ZB_ZCL_CLUSTER_ID_ON_OFF,
        ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT
    }
};


ZB_AF_START_DECLARE_ENDPOINT_LIST(device_ep_list)
ZB_AF_SET_ENDPOINT_DESC(DUT_ENDPOINT1, ZB_AF_HA_PROFILE_ID,
                        0,
                        NULL,
                        ZB_ZCL_ARRAY_SIZE(target_clusters_ep1, zb_zcl_cluster_desc_t),
                        target_clusters_ep1,
                        (zb_af_simple_desc_1_1_t *)&simple_desc_ep1),
                        ZB_AF_SET_ENDPOINT_DESC(DUT_ENDPOINT2, ZB_AF_HA_PROFILE_ID,
                                0,
                                NULL,
                                ZB_ZCL_ARRAY_SIZE(target_clusters_ep2, zb_zcl_cluster_desc_t),
                                target_clusters_ep2,
                                (zb_af_simple_desc_1_1_t *)&simple_desc_ep2)
                        ZB_AF_FINISH_DECLARE_ENDPOINT_LIST;

DECLARE_TARGET_NO_REP_CTX(target_device_ctx, device_ep_list);


/*************************Other functions**********************************/
static void trigger_fb_target(zb_uint8_t unused);



MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_dut");


    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_dut);

    ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
    ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;
    ZB_BDB().bdb_mode = 1;

    ZB_AIB().aps_designated_coordinator = 1;
    zb_secur_setup_nwk_key(g_nwk_key, 0);

    ZB_AF_REGISTER_DEVICE_CTX(&target_device_ctx);

#ifdef ZB_USE_NVRAM
    ZB_AIB().aps_use_nvram = 1;
#endif

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


static void trigger_fb_target(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    ZB_BDB().bdb_commissioning_time = FB_DURATION;
    zb_bdb_finding_binding_target(DUT_ENDPOINT1);
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
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
            break;

        case ZB_BDB_SIGNAL_STEERING:
            ZB_SCHEDULE_ALARM(trigger_fb_target, 0, DUT_FB_START_DELAY);
            break;

        case ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED:
            TRACE_MSG(TRACE_APS1, "Finding&binding done", (FMT__0));
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
