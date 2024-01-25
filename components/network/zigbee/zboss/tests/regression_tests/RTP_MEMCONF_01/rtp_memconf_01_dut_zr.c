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

#define ZB_TEST_NAME RTP_MEMCONF_01_DUT_ZR

#define ZB_TRACE_FILE_ID 40316
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"

#include "rtp_memconf_01_common.h"
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

static zb_ieee_addr_t g_ieee_addr_dut_zr = IEEE_ADDR_DUT_ZR;

static void test_send_bind_req_delayed(zb_uint8_t unused);
static void test_send_bind_req(zb_uint8_t param);
static void test_send_bind_req_cb(zb_uint8_t param);
static void trigger_steering(zb_uint8_t unused);
static void tests_validate_trans_index_size(zb_uint8_t unused);

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
    ZB_ZCL_CLUSTER_ID_TOUCHLINK_COMMISSIONING
};

MAIN()
{
    ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
    ZB_SET_TRACE_LEVEL(4);
    ARGV_UNUSED;

    ZB_INIT("zdo_dut_zr");

    zb_set_long_address(g_ieee_addr_dut_zr);
    zb_reg_test_set_common_channel_settings();
    zb_set_network_router_role((1l << TEST_CHANNEL));
    zb_set_nvram_erase_at_start(ZB_FALSE);

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

            TRACE_MSG(TRACE_APP1, "zb_zdo_startup_complete(): ZB_SINGLE_TRANS_INDEX_SIZE %d",
                      (FMT__D, ZB_SINGLE_TRANS_INDEX_SIZE));
            TRACE_MSG(TRACE_APP1, "zb_zdo_startup_complete(): ZB_NWK_BRCST_PASSIVE_ACK_ARRAY_SIZE %d",
                      (FMT__D, ZB_NWK_BRCST_PASSIVE_ACK_ARRAY_SIZE));
        }
        break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APP1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
        if (status == 0)
        {
            ZB_SCHEDULE_CALLBACK(test_send_bind_req_delayed, 0);
        }
        break; /* ZB_BDB_SIGNAL_STEERING */

    default:
        TRACE_MSG(TRACE_APP1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}

static void trigger_steering(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
}

static void test_send_bind_req_delayed(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    zb_buf_get_out_delayed(test_send_bind_req);
}

static void test_send_bind_req(zb_uint8_t param)
{
    zb_zdo_bind_req_param_t *req = ZB_BUF_GET_PARAM(param, zb_zdo_bind_req_param_t);

    TRACE_MSG(TRACE_APP1, ">>test_send_bind_req: buf_param = %hd, cluster 0x%x",
              (FMT__H_D, param, g_bind_clusters[g_bind_idx]));

    ZB_IEEE_ADDR_COPY(req->src_address, g_ieee_addr_dut_zr);
    req->src_endp = DUT_ENDPOINT;
    req->cluster_id = g_bind_clusters[g_bind_idx];
    req->dst_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
    ZB_IEEE_ADDR_COPY(req->dst_address.addr_long, g_ieee_addr_dut_zr);
    req->dst_endp = DUT_ENDPOINT;
    req->req_dst_addr = ZB_PIBCACHE_NETWORK_ADDRESS();

    zb_zdo_bind_req(param, test_send_bind_req_cb);

    TRACE_MSG(TRACE_APP1, "<<test_send_bind_req", (FMT__0));
}

static void test_send_bind_req_cb(zb_uint8_t param)
{
    zb_zdo_bind_resp_t *bind_resp = (zb_zdo_bind_resp_t *)zb_buf_begin(param);

    TRACE_MSG(TRACE_APP1, ">>test_send_bind_req_cb: param = %hd, status = %d",
              (FMT__H_D, param, bind_resp->status));

    g_bind_idx++;
    if (g_bind_idx < DUT_BIND_CLUSTERS_NUM)
    {
        ZB_SCHEDULE_CALLBACK(test_send_bind_req_delayed, 0);
    }
    else
    {
        ZB_SCHEDULE_CALLBACK(tests_validate_trans_index_size, 0);
    }

    TRACE_MSG(TRACE_APP1, "<<test_send_bind_req_cb", (FMT__0));

    zb_buf_free(param);
}

static void tests_validate_trans_index_size(zb_uint8_t unused)
{
    zb_uint8_t i, dif;

    ZVUNUSED(unused);

    TRACE_MSG(TRACE_APP1, "tests_validate_trans_index_size()", (FMT__0));

#ifndef NCP_MODE_HOST
    for (i = 1; i < ZB_APS_DST_BINDING_TABLE_SIZE; ++i)
    {
        dif = ZG->aps.binding.dst_table[i].trans_index - ZG->aps.binding.dst_table[i - 1].trans_index;

        if (dif != ZB_SINGLE_TRANS_INDEX_SIZE)
        {
            TRACE_MSG(TRACE_ERROR, "tests_validate_trans_index_size(): TEST FAILED, ZB_SINGLE_TRANS_INDEX_SIZE does not match , ind %d",
                      (FMT__H, i));
            break;
        }
    }

    for (i = 1; i < ZB_NWK_BRR_TABLE_SIZE; ++i)
    {
        dif = ZG->nwk.handle.brrt[i].passive_ack - ZG->nwk.handle.brrt[i - 1].passive_ack;

        if (dif != ZB_NWK_BRCST_PASSIVE_ACK_ARRAY_SIZE)
        {
            TRACE_MSG(TRACE_ERROR, "tests_validate_trans_index_size(): TEST FAILED, ZB_NWK_BRCST_PASSIVE_ACK_ARRAY_SIZE does not match, ind %d",
                      (FMT__H, i));
            break;
        }
    }
#else
    ZVUNUSED(i);
    ZVUNUSED(dif);
    ZB_ASSERT(ZB_FALSE && "TODO: implement binding table validation in case of NCP mode");
#endif
}

/*! @} */
