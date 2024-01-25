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
/* PURPOSE: DUT ZED
*/

#define ZB_TEST_NAME RTP_ZCL_17_DUT_ZED
#define ZB_TRACE_FILE_ID 63978

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
#include "device_dut.h"
#include "rtp_zcl_17_common.h"
#include "../common/zb_reg_test_globals.h"

#ifndef ZB_ED_ROLE
#error ED role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error ZB_USE_NVRAM is not compiled!
#endif


/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_dut_zed = IEEE_ADDR_DUT_ZED;

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(rtp_zcl_17_dut_zed_basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Occupancy Sensing cluster attributes data */

static zb_uint8_t attr_occupancy = ZB_ZCL_OCCUPANCY_SENSING_OCCUPANCY_UNOCCUPIED;
static enum zb_zcl_occupancy_sensing_occupancy_sensor_type_e attr_occupancy_sensor_type = ZB_ZCL_OCCUPANCY_SENSING_OCCUPANCY_SENSOR_TYPE_PIR;
static zb_uint8_t attr_occupancy_sensor_type_bitmap = 0x0;

ZB_ZCL_DECLARE_OCCUPANCY_SENSING_ATTRIB_LIST(rtp_zcl_17_dut_zed_occupancy_sensing_attr_list,
        &attr_occupancy,
        &attr_occupancy_sensor_type,
        &attr_occupancy_sensor_type_bitmap);

/********************* Declare device **************************/
DECLARE_DUT_CLUSTER_LIST(rtp_zcl_17_dut_zed_device_clusters,
                         rtp_zcl_17_dut_zed_basic_attr_list,
                         rtp_zcl_17_dut_zed_occupancy_sensing_attr_list);

DECLARE_DUT_EP(rtp_zcl_17_dut_zed_device_ep,
               DUT_ENDPOINT,
               rtp_zcl_17_dut_zed_device_clusters);

DECLARE_DUT_CTX(rtp_zcl_17_dut_zed_device_ctx, rtp_zcl_17_dut_zed_device_ep);

static void trigger_fb_initiator(zb_uint8_t unused);

static void configure_default_reporting(zb_uint8_t unused);
static void trigger_occupancy(zb_uint8_t unused);

/************************Main*************************************/
MAIN()
{
    ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
    ZB_SET_TRACE_LEVEL(4);
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_dut_zed");

    zb_set_long_address(g_ieee_addr_dut_zed);

    zb_reg_test_set_common_channel_settings();
    zb_set_network_ed_role((1l << TEST_CHANNEL));
    zb_set_nvram_erase_at_start(ZB_TRUE);

    ZB_AF_REGISTER_DEVICE_CTX(&rtp_zcl_17_dut_zed_device_ctx);

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

/********************ZDO Startup*****************************/
ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    TRACE_MSG(TRACE_APS1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    switch (sig)
    {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));
        if (status == 0)
        {
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
            configure_default_reporting(0);
        }
        break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
        if (status == 0)
        {
            ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, DUT_FB_INITIATOR_DELAY);
        }
        break; /* ZB_BDB_SIGNAL_STEERING */

    case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:
        TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED, status %d", (FMT__D, status));
        if (status == 0)
        {
            test_step_register(trigger_occupancy, 0, RTP_ZCL_17_STEP_1_TIME_ZC);
            test_step_register(trigger_occupancy, 0, RTP_ZCL_17_STEP_2_TIME_ZC);

            test_control_start(TEST_MODE, RTP_ZCL_17_STEP_1_DELAY_ZC);
        }
        break; /* ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED */


    default:
        TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}

static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster)
{
    TRACE_MSG(TRACE_ZCL1, "finding_binding_cb status %d addr " TRACE_FORMAT_64 " ep %hd cluster %d",
              (FMT__D_A_H_D, status, TRACE_ARG_64(addr), ep, cluster));
    return ZB_TRUE;
}

static void trigger_fb_initiator(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    zb_bdb_finding_binding_initiator(DUT_ENDPOINT, finding_binding_cb);
}

void configure_default_reporting(zb_uint8_t unused)
{
    zb_zcl_reporting_info_t rep_info;
    zb_ret_t status;
    ZVUNUSED(unused);

    ZB_BZERO(&rep_info, sizeof(rep_info));

    rep_info.direction = ZB_ZCL_CONFIGURE_REPORTING_SEND_REPORT;
    rep_info.attr_id = ZB_ZCL_ATTR_OCCUPANCY_SENSING_OCCUPANCY_ID;
    rep_info.ep = DUT_ENDPOINT;
    rep_info.cluster_id = ZB_ZCL_CLUSTER_ID_OCCUPANCY_SENSING;
    rep_info.cluster_role = ZB_ZCL_CLUSTER_SERVER_ROLE;
    rep_info.dst.profile_id = ZB_AF_HA_PROFILE_ID;

    rep_info.u.send_info.min_interval = ZB_ZCL_MIN_REPORTING_INTERVAL_DEFAULT;
    rep_info.u.send_info.max_interval = ZB_ZCL_MAX_REPORTING_INTERVAL_DEFAULT;
    rep_info.u.send_info.def_min_interval = ZB_ZCL_MIN_REPORTING_INTERVAL_DEFAULT;
    rep_info.u.send_info.def_max_interval = ZB_ZCL_MAX_REPORTING_INTERVAL_DEFAULT;
    rep_info.u.send_info.delta.u8 = 0;

    status = zb_zcl_put_reporting_info(&rep_info, ZB_TRUE);
    ZB_ASSERT(status == RET_OK);
}

static void trigger_occupancy(zb_uint8_t unused)
{
    zb_uint8_t opposite_occupancy_value;
    zb_zcl_status_t zcl_status;
    ZVUNUSED(unused);

    opposite_occupancy_value = attr_occupancy ^ 0x01;

    zcl_status = zb_zcl_set_attr_val(DUT_ENDPOINT,
                                     ZB_ZCL_CLUSTER_ID_OCCUPANCY_SENSING,
                                     ZB_ZCL_CLUSTER_SERVER_ROLE,
                                     ZB_ZCL_ATTR_OCCUPANCY_SENSING_OCCUPANCY_ID,
                                     (zb_uint8_t *)&opposite_occupancy_value,
                                     ZB_FALSE);

    ZB_ASSERT(zcl_status == ZB_ZCL_STATUS_SUCCESS);
}

/*! @} */
