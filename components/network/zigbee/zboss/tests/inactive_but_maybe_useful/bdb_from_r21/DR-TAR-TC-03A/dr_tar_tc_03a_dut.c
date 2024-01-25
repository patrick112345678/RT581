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
/* PURPOSE: DUT ZR (initiator)
*/


#define ZB_TEST_NAME DR_TAR_TC_03A_DUT
#define ZB_TRACE_FILE_ID 41238
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
#include "on_off_server.h"
#include "dut_reporting.h"
#include "dr_tar_tc_03a_common.h"

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


/********************* Declare device **************************/
DECLARE_ON_OFF_SERVER_CLUSTER_LIST(on_off_device_clusters_ep1,
                                   basic_attr_list_ep1,
                                   identify_attr_list_ep1,
                                   on_off_attr_list_ep1);

DECLARE_ON_OFF_SERVER_CLUSTER_LIST(on_off_device_clusters_ep2,
                                   basic_attr_list_ep2,
                                   identify_attr_list_ep2,
                                   on_off_attr_list_ep2);

ZB_DECLARE_SIMPLE_DESC(3, 0);


ZB_AF_SIMPLE_DESC_TYPE(3, 0) simple_desc_ep1 =
{
    DUT_ENDPOINT1,
    ZB_AF_HA_PROFILE_ID,
    DEVICE_ID_ON_OFF_SERVER,
    DEVICE_VER_ON_OFF_SERVER,
    0,
    3,
    0,
    {
        ZB_ZCL_CLUSTER_ID_BASIC,
        ZB_ZCL_CLUSTER_ID_IDENTIFY,
        ZB_ZCL_CLUSTER_ID_ON_OFF
    }
};

ZB_AF_SIMPLE_DESC_TYPE(3, 0) simple_desc_ep2 =
{
    DUT_ENDPOINT2,
    ZB_AF_HA_PROFILE_ID,
    DEVICE_ID_ON_OFF_SERVER,
    DEVICE_VER_ON_OFF_SERVER,
    0,
    3,
    0,
    {
        ZB_ZCL_CLUSTER_ID_BASIC,
        ZB_ZCL_CLUSTER_ID_IDENTIFY,
        ZB_ZCL_CLUSTER_ID_ON_OFF
    }
};


ZB_AF_START_DECLARE_ENDPOINT_LIST(device_ep_list)
ZB_AF_SET_ENDPOINT_DESC(DUT_ENDPOINT1, ZB_AF_HA_PROFILE_ID,
                        0,
                        NULL,
                        ZB_ZCL_ARRAY_SIZE(on_off_device_clusters_ep1, zb_zcl_cluster_desc_t),
                        on_off_device_clusters_ep1,
                        (zb_af_simple_desc_1_1_t *)&simple_desc_ep1),
                        ZB_AF_SET_ENDPOINT_DESC(DUT_ENDPOINT2, ZB_AF_HA_PROFILE_ID,
                                0,
                                NULL,
                                ZB_ZCL_ARRAY_SIZE(on_off_device_clusters_ep2, zb_zcl_cluster_desc_t),
                                on_off_device_clusters_ep2,
                                (zb_af_simple_desc_1_1_t *)&simple_desc_ep2)
                        ZB_AF_FINISH_DECLARE_ENDPOINT_LIST;

DECLARE_ON_OFF_SERVER_CTX(on_off_device_ctx, device_ep_list);


/*************************Other functions**********************************/
static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster);
static void trigger_fb_initiator(zb_uint8_t unused);

static void reset_dut_ep(zb_uint8_t ep);
static void init_default_reporting(int reset_ep1, int reset_ep2);
static void event_handler_cb(zb_uint8_t param);

static void close_network_req_delayed(zb_uint8_t param);
static void close_network_req(zb_uint8_t param);

#ifdef USE_NVRAM_IN_TEST
/* NVRAM support */
static void read_dut_app_data_cb(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length);
static zb_ret_t write_dut_app_data_cb(zb_uint8_t page, zb_uint32_t pos);
static zb_uint16_t dut_nvram_get_app_data_size_cb();
#endif


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_dut");


    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_dut);

    ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
    ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;
    ZB_BDB().bdb_mode = 1;

    ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ROUTER;

    ZB_ZCL_SET_DEFAULT_VALUE_CB(reset_dut_ep);
    ZB_AF_REGISTER_DEVICE_CTX(&on_off_device_ctx);
    ZB_ZCL_REGISTER_DEVICE_CB(event_handler_cb);

#ifdef USE_NVRAM_IN_TEST
    ZB_AIB().aps_use_nvram = 1;
    zb_nvram_register_app1_read_cb(read_dut_app_data_cb);
    zb_nvram_register_app1_write_cb(write_dut_app_data_cb, dut_nvram_get_app_data_size_cb);
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
    zb_bdb_finding_binding_initiator(DUT_ENDPOINT1, finding_binding_cb);
}


static void close_network_req_delayed(zb_uint8_t param)
{
    ZVUNUSED(param);
    ZB_GET_OUT_BUF_DELAYED(close_network_req);
}


static void close_network_req(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_nlme_permit_joining_request_t *req;

    TRACE_MSG(TRACE_ZDO1, ">>close_network_req: buf_param = %d", (FMT__D, param));

    req = ZB_GET_BUF_TAIL(buf, sizeof(zb_nlme_permit_joining_request_t));
    req->permit_duration = 0;
    zb_nlme_permit_joining_request(param);

    TRACE_MSG(TRACE_ZDO1, "<<close_network_req", (FMT__0));
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
            ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, DUT_FB_START_DELAY);
            ZB_SCHEDULE_ALARM(close_network_req_delayed, 0, DUT_CLOSE_NETWORK_DELAY);
            break;

        case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:
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


/*************************************Additional Routines*********************************/
static void reset_dut_ep(zb_uint8_t ep)
{
    TRACE_MSG(TRACE_ZCL1, ">>reset_dut_ep: ep = %d", (FMT__D, ep));

    if (ep == DUT_ENDPOINT1)
    {
        init_default_reporting(1, 0);
    }
    else if (ep == DUT_ENDPOINT2)
    {
        init_default_reporting(0, 1);
    }
    else if (ep == 0xff)
    {
        init_default_reporting(1, 1);
    }



    TRACE_MSG(TRACE_ZCL1, "<<reset_dut_ep", (FMT__0));
}


static void init_default_reporting(int reset_ep1, int reset_ep2)
{
    zb_zcl_reporting_info_t rep_info;

    TRACE_MSG(TRACE_ZCL1, ">>init_default_reporting: reset_ep1 = %d, reset_ep2 = %d",
              (FMT__D_D, reset_ep1, reset_ep2));

    /* On/Off value */
    if (reset_ep1)
    {
        ZB_BZERO(&rep_info, sizeof(rep_info));
        {
            rep_info.direction = ZB_ZCL_CONFIGURE_REPORTING_SEND_REPORT;
            rep_info.ep = DUT_ENDPOINT1;
            rep_info.cluster_id = ZB_ZCL_CLUSTER_ID_ON_OFF;
            rep_info.attr_id = ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID;
            rep_info.dst.profile_id = ZB_AF_HA_PROFILE_ID;

            rep_info.u.send_info.def_min_interval = DUT_ON_OFF_MIN_INTERVAL1;
            rep_info.u.send_info.def_max_interval = DUT_ON_OFF_MAX_INTERVAL1;
        }
        attr_on_off_epx[0] = DUT_ON_OFF_RESET_VALUE;
    }

    if (reset_ep2)
    {
        ZB_BZERO(&rep_info, sizeof(rep_info));
        {
            rep_info.direction = ZB_ZCL_CONFIGURE_REPORTING_SEND_REPORT;
            rep_info.ep = DUT_ENDPOINT2;
            rep_info.cluster_id = ZB_ZCL_CLUSTER_ID_ON_OFF;
            rep_info.attr_id = ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID;
            rep_info.dst.profile_id = ZB_AF_HA_PROFILE_ID;

            rep_info.u.send_info.def_min_interval = DUT_ON_OFF_MIN_INTERVAL2;
            rep_info.u.send_info.def_max_interval = DUT_ON_OFF_MAX_INTERVAL2;
        }
        attr_on_off_epx[1] = DUT_ON_OFF_RESET_VALUE;
    }

    zb_zcl_put_reporting_info(&rep_info, ZB_TRUE);

    TRACE_MSG(TRACE_ZCL1, "<<init_default_reporting", (FMT__0));
}


static void event_handler_cb(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_zcl_device_callback_param_t *data = ZB_GET_BUF_PARAM(buf, zb_zcl_device_callback_param_t);

    TRACE_MSG(TRACE_ZCL1, ">>event_handler_cb: buf_param = %d, device_cb_id = 0x%x",
              (FMT__D_H, param, data->device_cb_id));

    if ( (data->device_cb_id == ZB_ZCL_SET_ATTR_VALUE_CB_ID) &&
            (data->cb_param.set_attr_value_param.cluster_id == ZB_ZCL_CLUSTER_ID_ON_OFF) &&
            (data->cb_param.set_attr_value_param.attr_id == ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID) )
    {
#ifdef USE_NVRAM_IN_TEST
        zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);
#endif
    }

    TRACE_MSG(TRACE_ZCL1, "<<event_handler_cb", (FMT__0));
}


/* NVRAM support */
#ifdef USE_NVRAM_IN_TEST
static void read_dut_app_data_cb(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length)
{
    zb_ret_t ret;
    dut_nvram_app_dataset_t ds;

    if (payload_length == sizeof(ds))
    {
        ret = zb_osif_nvram_read(page, pos, (zb_uint8_t *)&ds, sizeof(ds));
        if (ret == RET_OK)
        {
            attr_on_off_epx[0] = (zb_bool_t) ds.on_off_arr[0];
            attr_on_off_epx[1] = (zb_bool_t) ds.on_off_arr[1];
        }
        else
        {
            TRACE_MSG(TRACE_ERROR, "read_dut_app_data_cb: nvram read error %d", (FMT__D, ret));
        }
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "read_dut_app_data_cb ds mismatch: got %d wants %d",
                  (FMT__D_D, payload_length, sizeof(ds)));
    }
}


static zb_ret_t write_dut_app_data_cb(zb_uint8_t page, zb_uint32_t pos)
{
    zb_ret_t ret = RET_OK;
    dut_nvram_app_dataset_t ds;

    TRACE_MSG(TRACE_APS3, ">>write_dut_app_data_cb", (FMT__0));

    ds.on_off_arr[0] = (zb_uint16_t) attr_on_off_epx[0];
    ds.on_off_arr[1] = (zb_uint16_t) attr_on_off_epx[1];
    ret = zb_osif_nvram_write(page, pos, (zb_uint8_t *)&ds, sizeof(ds));

    TRACE_MSG(TRACE_APS3, "<<write_dut_app_data_cb ret %d", (FMT__D, ret));
    return ret;
}


static zb_uint16_t dut_nvram_get_app_data_size_cb()
{
    return sizeof(dut_nvram_app_dataset_t);
}
#endif /* USE_NVRAM_IN_TEST */


/*! @} */
