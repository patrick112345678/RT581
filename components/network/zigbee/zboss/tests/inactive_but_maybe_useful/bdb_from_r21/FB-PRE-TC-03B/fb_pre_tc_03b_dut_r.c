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


#define ZB_TEST_NAME FB_PRE_TC_03B_DUT_R
#define ZB_TRACE_FILE_ID 41246
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
#include "test_initiator.h"
#include "fb_pre_tc_03b_common.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */


#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#ifndef ZB_USE_NVRAM
#error ZB_USE_NVRAM is not compiled!
#endif


/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;
/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time;
/* On/Off cluster attributes data */


ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list,
                                 &attr_zcl_version,
                                 &attr_power_source);

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &attr_identify_time);


/********************* Declare device **************************/
DECLARE_INITIATOR_CLUSTER_LIST(initiator_clusters,
                               basic_attr_list,
                               identify_attr_list);

DECLARE_INITIATOR_EP(device_ep_list, DUT_ENDPOINT1, initiator_clusters);

DECLARE_INITIATOR_NO_REP_CTX(initiator_device_ctx, device_ep_list);


/*************************Other functions**********************************/

enum match_desc_options_e
{
    TEST_OPT_LEGACY_0101,
    TEST_OPT_LEGACY_0102,
    TEST_OPT_LEGACY_0103,
    TEST_OPT_LEGACY_0105,
    TEST_OPT_LEGACY_0106,
    TEST_OPT_LEGACY_0107,
    TEST_OPT_LEGACY_0108,
    TEST_OPT_LEGACY_C05E,
    TEST_OPT_WILDCARD,
    TEST_OPT_ZSE,
    TEST_OPT_GW,
    TEST_OPT_MSP,
    TEST_OPT_NEGATIVE_DEVICE_NOT_FOUND,
    TEST_OPT_NEGATIVE_WRONG_ADDR,
    TEST_OPT_NEGATIVE_NO_MATCHING_CLUSTERS,
    TEST_OPT_NEGATIVE_NO_MATCHING_ROLE,
    TEST_OPT_NEGATIVE_EMPTY_CLUSTER_LIST,
    TEST_OPT_TWO_RESPONSES_1,
    TEST_OPT_TWO_RESPONSES_2,
    TEST_OPT_TWO_RESPONSES_3,
    TEST_OPT_COUNT
};


static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster);

static void trigger_fb_initiator(zb_uint8_t unused);

static void send_match_desc_req(zb_uint8_t param);
static void match_des_resp_cb(zb_uint8_t param);


static int s_step_idx;


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_dut");


    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_dut);

    ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
    ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;
    ZB_BDB().bdb_mode = 1;
    ZB_AIB().aps_use_nvram = 1;

    ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ROUTER;
    zb_secur_setup_nwk_key(g_nwk_key, 0);
    ZB_NIB().max_children = 0;

    ZB_AF_REGISTER_DEVICE_CTX(&initiator_device_ctx);

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
    ZB_BDB().bdb_commissioning_time = FB_DURATION;
    zb_bdb_finding_binding_initiator(DUT_ENDPOINT1, finding_binding_cb);
}


static void send_match_desc_req(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_zdo_match_desc_param_t *req;
    zb_uint16_t addr = zb_address_short_by_ieee(g_ieee_addr_the1);

    TRACE_MSG(TRACE_ZDO1, ">>send_match_desc_req: buf_param = %d", (FMT__D, param));

    ZB_BUF_INITIAL_ALLOC(buf, sizeof(zb_zdo_match_desc_param_t) + 2 * sizeof(zb_uint16_t), req);
    req->nwk_addr = addr;
    req->addr_of_interest = addr;
    req->profile_id = ZB_AF_HA_PROFILE_ID;
    req->num_in_clusters = 0;
    req->num_out_clusters = 3;

    req->cluster_list[0] = ZB_ZCL_CLUSTER_ID_BASIC;
    req->cluster_list[1] = ZB_ZCL_CLUSTER_ID_IDENTIFY;
    req->cluster_list[2] = ZB_ZCL_CLUSTER_ID_ON_OFF;

    zb_zdo_match_desc_req(param, match_des_resp_cb);

    TRACE_MSG(TRACE_ZDO1, "<<send_match_desc_req", (FMT__0));
}


static void match_des_resp_cb(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);

    TRACE_MSG(TRACE_ZDO1, ">>match_des_resp_cb: buf_param = %d", (FMT__D, param));

    zb_free_buf(buf);

    if (s_step_idx < TEST_OPT_COUNT)
    {
        zb_apsme_unbind_all(0);
        ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, DUT_FB_START_DELAY);
        BDB_COMM_CTX().respondent_number = 0;
        ++s_step_idx;
    }

    TRACE_MSG(TRACE_ZDO1, "<<match_des_resp_cb", (FMT__0));
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
            ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, DUT_FB_FIRST_START_DELAY);
            break;

        case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:
            TRACE_MSG(TRACE_APS1, "Finding&binding done", (FMT__0));
            if (BDB_COMM_CTX().state == ZB_BDB_COMM_IDLE)
            {
                if (s_step_idx < TEST_OPT_TWO_RESPONSES_2)
                {
                    zb_apsme_unbind_all(0);
                    ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, DUT_FB_START_DELAY);
                    BDB_COMM_CTX().respondent_number = 0;
                    ++s_step_idx;
                }
                else
                {
                    ZB_GET_OUT_BUF_DELAYED(send_match_desc_req);
                }
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
