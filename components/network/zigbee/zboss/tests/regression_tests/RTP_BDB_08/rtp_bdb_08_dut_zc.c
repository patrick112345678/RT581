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
/* PURPOSE: DUT ZC
*/

#define ZB_TEST_NAME RTP_BDB_08_DUT_ZC

#define ZB_TRACE_FILE_ID 40489
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
#include "rtp_bdb_08_common.h"
#include "../common/zb_reg_test_globals.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error ZB_USE_NVRAM is not compiled!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_uint8_t g_nwk_key[16] = ZB_REG_TEST_DEFAULT_NWK_KEY;
static zb_ieee_addr_t g_ieee_addr_dut  = IEEE_ADDR_DUT;

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(rtp_bdb_08_dut_zc_basic_attr_list, &attr_zcl_version, &attr_power_source);

/********************* Declare device **************************/
DECLARE_DUT_CLUSTER_LIST(rtp_bdb_08_dut_zc_device_clusters,
                         rtp_bdb_08_dut_zc_basic_attr_list);

DECLARE_DUT_EP(rtp_bdb_08_dut_zc_device_ep,
               DUT_ENDPOINT,
               rtp_bdb_08_dut_zc_device_clusters);

DECLARE_DUT_CTX(rtp_bdb_08_dut_zc_device_ctx, rtp_bdb_08_dut_zc_device_ep);
/*************************************************************************/

/*******************Definitions for Test***************************/

static void send_bind_req(zb_uint8_t param);
static void send_bind_req_cb(zb_uint8_t param);

static void send_mgmt_bind_req_delayed(zb_uint8_t unused);
static void send_mgmt_bind_req(zb_uint8_t param);
static void mgmt_bind_resp_cb(zb_uint8_t param);

static zb_uint8_t g_bind_idx = 0;
static zb_uint16_t g_bind_clusters[DUT_BIND_CLUSTERS_NUM] =
{
    ZB_ZCL_CLUSTER_ID_IDENTIFY,
    ZB_ZCL_CLUSTER_ID_ILLUMINANCE_MEASUREMENT,
    ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
    ZB_ZCL_CLUSTER_ID_PRESSURE_MEASUREMENT,
    ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT,
    ZB_ZCL_CLUSTER_ID_OCCUPANCY_SENSING,
    ZB_ZCL_CLUSTER_ID_ON_OFF,
    ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL,
    ZB_ZCL_CLUSTER_ID_OTA_UPGRADE,
    ZB_ZCL_CLUSTER_ID_THERMOSTAT,
    ZB_ZCL_CLUSTER_ID_FAN_CONTROL,
    ZB_ZCL_CLUSTER_ID_APPLIANCE_EVENTS_AND_ALERTS,
    ZB_ZCL_CLUSTER_ID_ELECTRICAL_MEASUREMENT,
    ZB_ZCL_CLUSTER_ID_DIAGNOSTICS,
    ZB_ZCL_CLUSTER_ID_WWAH,
    ZB_ZCL_CLUSTER_ID_TOUCHLINK_COMMISSIONING,
    ZB_ZCL_CLUSTER_ID_ALARMS,
    ZB_ZCL_CLUSTER_ID_TIME,
    ZB_ZCL_CLUSTER_ID_RSSI_LOCATION,
    ZB_ZCL_CLUSTER_ID_ANALOG_VALUE,
    ZB_ZCL_CLUSTER_ID_BINARY_INPUT,
    ZB_ZCL_CLUSTER_ID_BINARY_OUTPUT,
    ZB_ZCL_CLUSTER_ID_BINARY_VALUE,
    ZB_ZCL_CLUSTER_ID_MULTI_INPUT,
    ZB_ZCL_CLUSTER_ID_MULTI_OUTPUT,
    ZB_ZCL_CLUSTER_ID_MULTI_VALUE,
    ZB_ZCL_CLUSTER_ID_COMMISSIONING,
    ZB_ZCL_CLUSTER_ID_SHADE_CONFIG,
    ZB_ZCL_CLUSTER_ID_DOOR_LOCK,
    ZB_ZCL_CLUSTER_ID_WINDOW_COVERING,
    ZB_ZCL_CLUSTER_ID_PUMP_CONFIG_CONTROL,
    ZB_ZCL_CLUSTER_ID_DEHUMID_CONTROL,
    /* ZB_ZCL_CLUSTER_ID_THERMOSTAT_UI_CONFIG */
};
static zb_uint16_t g_dut_short_addr = 0x0000;
static zb_uint8_t g_start_idx = 0;


static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster);
static void trigger_fb_initiator(zb_uint8_t unused);
static void cancel_fb_initiator(zb_uint8_t unused);
static void clear_binding_table(zb_uint8_t unused);

MAIN()
{
    ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP | TRACE_SUBSYSTEM_ZCL);
    ZB_SET_TRACE_LEVEL(4);
    ARGV_UNUSED;

    ZB_INIT("zdo_dut_zc");

    zb_set_long_address(g_ieee_addr_dut);

    zb_reg_test_set_common_channel_settings();
    zb_set_network_coordinator_role((1l << TEST_CHANNEL));
    zb_secur_setup_nwk_key(g_nwk_key, 0);

    zb_set_nvram_erase_at_start(ZB_TRUE);

    ZB_AF_REGISTER_DEVICE_CTX(&rtp_bdb_08_dut_zc_device_ctx);

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


/***********************************Implementation**********************************/
ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);

    zb_zdo_app_signal_hdr_t *sg_p = NULL;
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);
    zb_zdo_signal_fb_initiator_finished_params_t *fb_initiator_finished_params;

    TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    switch (sig)
    {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));
        if (status == 0)
        {
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        }
        break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
        TRACE_MSG(TRACE_APS1, "signal: ZB_ZDO_SIGNAL_DEVICE_ANNCE, status %d", (FMT__D, status));
        if (status == 0)
        {
            test_step_register(trigger_fb_initiator, 0, RTP_BDB_08_STEP_1_TIME_ZC);
            test_step_register(send_bind_req, 0, RTP_BDB_08_STEP_2_TIME_ZC);
            test_step_register(trigger_fb_initiator, 0, RTP_BDB_08_STEP_3_TIME_ZC);
            test_step_register(send_mgmt_bind_req_delayed, 0, RTP_BDB_08_STEP_4_TIME_ZC);
            test_step_register(clear_binding_table, 0, RTP_BDB_08_STEP_5_TIME_ZC);
            test_step_register(trigger_fb_initiator, 0, RTP_BDB_08_STEP_6_TIME_ZC);
            test_step_register(cancel_fb_initiator, 0, RTP_BDB_08_STEP_7_TIME_ZC);
            test_step_register(trigger_fb_initiator, 0, RTP_BDB_08_STEP_8_TIME_ZC);
            test_step_register(send_mgmt_bind_req_delayed, 0, RTP_BDB_08_STEP_9_TIME_ZC);

            test_control_start(TEST_MODE, RTP_BDB_08_STEP_1_DELAY_ZC);
        }
        break; /* ZB_ZDO_SIGNAL_DEVICE_ANNCE */

    case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:
        TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED, status %d", (FMT__D, status));

        fb_initiator_finished_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_fb_initiator_finished_params_t);

        TRACE_MSG(TRACE_APP1, "BDB finding and binding initiator finished, status %d", (FMT__D, fb_initiator_finished_params->status));
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
    TRACE_MSG(TRACE_APP1, "finding_binding_cb status %d addr " TRACE_FORMAT_64 " ep %hd cluster %d",
              (FMT__D_A_H_D, status, TRACE_ARG_64(addr), ep, cluster));
    return ZB_TRUE;
}

static void trigger_fb_initiator(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    TRACE_MSG(TRACE_APP1, ">> trigger_fb_initiator", (FMT__0));

    zb_bdb_finding_binding_initiator(DUT_ENDPOINT, finding_binding_cb);
}

static void cancel_fb_initiator(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    TRACE_MSG(TRACE_APP1, ">> cancel_fb_initiator", (FMT__0));

    zb_bdb_finding_binding_initiator_cancel();
}

static void clear_binding_table(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    zb_apsme_unbind_all(0);
}

static void send_bind_req(zb_uint8_t param)
{
    zb_apsme_binding_req_t *req;

    if (param == 0)
    {
        zb_buf_get_out_delayed(send_bind_req);
        return;
    }

    req = ZB_BUF_GET_PARAM(param, zb_apsme_binding_req_t);

    TRACE_MSG(TRACE_APP1, ">>send_bind_req: buf_param = %d, cluster 0x%x",
              (FMT__D_D, param, g_bind_clusters[g_bind_idx]));

    ZB_IEEE_ADDR_COPY(req->src_addr, &g_ieee_addr_dut);
    ZB_IEEE_ADDR_COPY(req->dst_addr.addr_long, g_ieee_addr_dut);

    req->src_endpoint = DUT_ENDPOINT;
    req->clusterid = g_bind_clusters[g_bind_idx];
    req->addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
    req->dst_endpoint = DUT_ENDPOINT;
    req->confirm_cb = send_bind_req_cb;

    zb_apsme_bind_request(param);

    TRACE_MSG(TRACE_APP1, "<<send_bind_req", (FMT__0));
}

static void send_bind_req_cb(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> send_bind_req_cb", (FMT__0));

    g_bind_idx++;

    if (g_bind_idx < DUT_BIND_CLUSTERS_NUM)
    {
        zb_buf_reuse(param);
        ZB_SCHEDULE_CALLBACK(send_bind_req, param);
    }
    else
    {
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_APP1, "<<send_bind_req_cb", (FMT__0));
}

static void send_mgmt_bind_req_delayed(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    g_start_idx = 0;
    zb_buf_get_out_delayed(send_mgmt_bind_req);
}

static void send_mgmt_bind_req(zb_uint8_t param)
{
    zb_zdo_mgmt_bind_param_t *req_params;

    TRACE_MSG(TRACE_APP1, ">>send_mgmt_bind_req: param = %i", (FMT__D, param));

    req_params = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_bind_param_t);
    req_params->start_index = g_start_idx;
    req_params->dst_addr = g_dut_short_addr;
    zb_zdo_mgmt_bind_req(param, mgmt_bind_resp_cb);

    TRACE_MSG(TRACE_APP1, "<<send_mgmt_bind_req", (FMT__0));
}

static void mgmt_bind_resp_cb(zb_uint8_t param)
{
    zb_zdo_mgmt_bind_resp_t *resp;
    zb_zdo_binding_table_record_t *record;
    zb_uindex_t i;
    zb_callback_t call_cb = 0;

    TRACE_MSG(TRACE_APP1, ">>mgmt_bind_resp_cb: param = %i", (FMT__D, param));

    resp = (zb_zdo_mgmt_bind_resp_t *) zb_buf_begin(param);

    TRACE_MSG(TRACE_APP1, "mgmt_bind_resp_cb: status = %i", (FMT__D, resp->status));

    if (resp->status == ZB_ZDP_STATUS_SUCCESS)
    {
        zb_uint8_t nbrs = resp->binding_table_list_count;

        g_start_idx += nbrs;
        if (g_start_idx < resp->binding_table_entries)
        {
            TRACE_MSG(TRACE_APP1, "mgmt_bind_resp_cb: retrieved = %d, total = %d",
                      (FMT__D_D, nbrs, resp->binding_table_entries));
            call_cb = send_mgmt_bind_req;
        }
        else
        {
            TRACE_MSG(TRACE_APP1, "mgmt_bind_resp_cb: retrieved all entries - %d",
                      (FMT__D, resp->binding_table_entries));
        }

        record = (zb_zdo_binding_table_record_t *) (resp + 1);

        for (i = 0; i < nbrs; i++)
        {
            TRACE_MSG(TRACE_APP1, "mgmt_bind_resp_cb: record - src endp %d, dst endp %d, cluster id %d",
                      (FMT__D_D_D, record->src_endp, record->dst_endp, record->cluster_id));

            record++;
        }
    }
    else
    {
        TRACE_MSG(TRACE_APP1, "mgmt_bind_resp_cb: FAILED", (FMT__0));
    }

    if (call_cb)
    {
        zb_buf_reuse(param);
        ZB_SCHEDULE_CALLBACK(call_cb, param);
    }
    else
    {
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_APP1, "<<mgmt_bind_resp_cb", (FMT__0));
}

/*! @} */
