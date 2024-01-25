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
/* PURPOSE: DEFERRED: FB-PRE-TC-03B: Service discovery - client side additional tests
*/

#define ZB_TEST_NAME TP_BDB_FB_PRE_TC_03B_DUT_C
#define ZB_TRACE_FILE_ID 40770

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
#include "tp_bdb_fb_pre_tc_03b_common.h"
#include "../common/zb_cert_test_globals.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */


#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error ZB_USE_NVRAM is not compiled!
#endif

static zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;
static zb_ieee_addr_t g_ieee_addr_the1 = IEEE_ADDR_THE1;

static zb_uint8_t g_nwk_key[16] = {0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                                  };

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;
/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time;
/* On/Off cluster attributes data */


ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(fb_pre_tc_03b_dut_c_basic_attr_list,
                                 &attr_zcl_version,
                                 &attr_power_source);

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(fb_pre_tc_03b_dut_c_identify_attr_list, &attr_identify_time);


/********************* Declare device **************************/
DECLARE_INITIATOR_CLUSTER_LIST(fb_pre_tc_03b_dut_c_initiator_clusters,
                               fb_pre_tc_03b_dut_c_basic_attr_list,
                               fb_pre_tc_03b_dut_c_identify_attr_list);

DECLARE_INITIATOR_EP(fb_pre_tc_03b_dut_c_device_ep_list, DUT_ENDPOINT1, fb_pre_tc_03b_dut_c_initiator_clusters);

DECLARE_INITIATOR_NO_REP_CTX(fb_pre_tc_03b_dut_c_initiator_device_ctx, fb_pre_tc_03b_dut_c_device_ep_list);


/*************************Other functions**********************************/

enum match_desc_options_e
{
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
    TEST_OPT_TWO_RESPONSES_3
};

static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster);

static void trigger_fb_initiator(zb_uint8_t unused);
static void send_match_desc_req(zb_uint8_t param);
static void match_des_resp_cb(zb_uint8_t param);

static int s_step_idx;
static zb_uint16_t TEST_PAN_ID = 0x1AAA;

MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_dut");


    zb_set_pan_id(TEST_PAN_ID);
    zb_set_long_address(g_ieee_addr_dut);

    zb_set_network_coordinator_role((1l << TEST_CHANNEL));
    zb_set_nvram_erase_at_start(ZB_TRUE);

    zb_secur_setup_nwk_key(g_nwk_key, 0);

    zb_set_max_children(1);

    ZB_AF_REGISTER_DEVICE_CTX(&fb_pre_tc_03b_dut_c_initiator_device_ctx);

    s_step_idx = 0;

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
    zb_zdo_match_desc_param_t *req;
    zb_uint16_t addr = zb_address_short_by_ieee(g_ieee_addr_the1);

    TRACE_MSG(TRACE_ZDO1, ">>send_match_desc_req: buf_param = %d", (FMT__D, param));

    req = zb_buf_initial_alloc(param, sizeof(zb_zdo_match_desc_param_t) + 2 * sizeof(zb_uint16_t));
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

    TRACE_MSG(TRACE_ZDO1, ">>match_des_resp_cb: buf_param = %d", (FMT__D, param));

    zb_buf_free(param);

    if (s_step_idx < TEST_OPT_TWO_RESPONSES_3)
    {
        TRACE_MSG(TRACE_APP1, "s_step_idx = %d", (FMT__D, s_step_idx));

        zb_apsme_unbind_all(0);
        ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, (2 * DUT_FB_START_DELAY));
        BDB_COMM_CTX().respondent_number = 0;
        ++s_step_idx;
    }

    TRACE_MSG(TRACE_ZDO1, "<<match_des_resp_cb", (FMT__0));
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    switch (sig)
    {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        if (status == 0)
        {
            TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));

            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        }
        else
        {
            TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, status));
        }
        break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_STEERING:
        if (status == 0)
        {
            TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_STEERING, status OK", (FMT__0));

            ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, DUT_ZC_FB_FIRST_START_DELAY);
        }
        else
        {
            TRACE_MSG(TRACE_ERROR, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
        }
        break; /* ZB_BDB_SIGNAL_STEERING */

    case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:

        if (status == 0)
        {
            TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED, status OK", (FMT__0));

            if (BDB_COMM_CTX().state == ZB_BDB_COMM_IDLE)
            {
                if (s_step_idx < TEST_OPT_TWO_RESPONSES_2)
                {
                    TRACE_MSG(TRACE_APP1, "s_step_idx = %d", (FMT__D, s_step_idx));

                    zb_apsme_unbind_all(0);
                    ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, DUT_FB_START_DELAY);
                    BDB_COMM_CTX().respondent_number = 0;
                    ++s_step_idx;
                }
                else
                {
                    zb_buf_get_out_delayed(send_match_desc_req);
                }
            }
        }
        else
        {
            TRACE_MSG(TRACE_ERROR, "signal: ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED, status %d", (FMT__D, status));
        }
        break; /* ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED */

    default:
        if (status == 0)
        {
            TRACE_MSG(TRACE_APS1, "Unknown signal, status OK", (FMT__0));
        }
        else
        {
            TRACE_MSG(TRACE_ERROR, "Unknown signal, status %d", (FMT__D, status));
        }
        break;
    }

    zb_buf_free(param);
}


/*! @} */
