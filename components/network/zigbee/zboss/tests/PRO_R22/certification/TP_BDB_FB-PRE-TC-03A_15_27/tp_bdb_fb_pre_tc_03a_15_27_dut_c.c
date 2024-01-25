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
/* PURPOSE: DUT ZC (target)
*/

#define ZB_TEST_NAME TP_BDB_FB_PRE_TC_03A_15_27_DUT_C
#define ZB_TRACE_FILE_ID 40535

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
#include "tp_bdb_fb_pre_tc_03a_15_27_common.h"
#include "../common/zb_cert_test_globals.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif


#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;

static zb_uint8_t g_nwk_key[16] = {0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                                  };

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(fb_pre_tc_03a_15_27_dut_c_basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(fb_pre_tc_03a_15_27_dut_c_identify_attr_list, &attr_identify_time);

static zb_bool_t attr_on_off = 1;
ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(fb_pre_tc_03a_15_27_dut_c_on_off_attr_list, &attr_on_off);

/********************* Declare device **************************/
DECLARE_ON_OFF_SERVER_CLUSTER_LIST(fb_pre_tc_03a_15_27_dut_c_on_off_device_clusters,
                                   fb_pre_tc_03a_15_27_dut_c_basic_attr_list,
                                   fb_pre_tc_03a_15_27_dut_c_identify_attr_list,
                                   fb_pre_tc_03a_15_27_dut_c_on_off_attr_list);

DECLARE_ON_OFF_SERVER_EP(fb_pre_tc_03a_15_27_dut_c_on_off_device_ep,
                         DUT_ENDPOINT,
                         fb_pre_tc_03a_15_27_dut_c_on_off_device_clusters);

DECLARE_ON_OFF_SERVER_CTX(fb_pre_tc_03a_15_27_dut_c_on_off_device_ctx, fb_pre_tc_03a_15_27_dut_c_on_off_device_ep);


static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster);
static void trigger_fb_initiator(zb_uint8_t unused);
static void send_match_desc_delayed(zb_uint8_t unused);
static void send_match_desc(zb_uint8_t param);
static void match_desc_resp_cb(zb_uint8_t param);

static zb_ieee_addr_t s_target_ieee;

MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_dut");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif


    zb_set_long_address(g_ieee_addr_dut);

    zb_set_network_coordinator_role((1l << TEST_CHANNEL));
    zb_set_nvram_erase_at_start(ZB_TRUE);
    zb_secur_setup_nwk_key(g_nwk_key, 0);
    zb_set_max_children(1);

    /* allow joining without perm_join (for connection through zr) */
    ZB_TCPOL().authenticate_always = 1;

    ZB_AF_REGISTER_DEVICE_CTX(&fb_pre_tc_03a_15_27_dut_c_on_off_device_ctx);

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
        TRACE_MSG(TRACE_APS1, "Device started, status %d", (FMT__D, status));
        if (status == 0)
        {
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
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
            if (BDB_COMM_CTX().state == ZB_BDB_COMM_IDLE)
            {
                ZB_SCHEDULE_CALLBACK(send_match_desc_delayed, 0);
            }
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
    ZB_IEEE_ADDR_COPY(s_target_ieee, addr);
    return ZB_TRUE;
}


static void trigger_fb_initiator(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    zb_apsme_unbind_all(0);
    ZB_BDB().bdb_commissioning_time = FB_INITIATOR_DURATION;
    zb_bdb_finding_binding_initiator(DUT_ENDPOINT, finding_binding_cb);
}

static void send_match_desc_delayed(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    TRACE_MSG(TRACE_ERROR, ">> send_match_desc_delayed", (FMT__0));

    if (zb_buf_get_out_delayed(send_match_desc) != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "send_match_desc_delayed: buffer allocation error", (FMT__0));
    }
}

static void send_match_desc(zb_uint8_t param)
{
    zb_uint8_t req_size = sizeof(zb_zdo_match_desc_param_t) + sizeof(zb_uint32_t);
    zb_zdo_match_desc_param_t *req;

    TRACE_MSG(TRACE_APP1, "send_match_desc_req: buf = %d", (FMT__D, param));

    req = zb_buf_initial_alloc(param, req_size);

    req->nwk_addr = zb_address_short_by_ieee(s_target_ieee);
    req->addr_of_interest = req->nwk_addr;
    req->profile_id = ZB_AF_HA_PROFILE_ID;
    req->num_in_clusters = 2;
    req->num_out_clusters = 1;
    req->cluster_list[0] = ZB_ZCL_CLUSTER_ID_BASIC;
    req->cluster_list[1] = ZB_ZCL_CLUSTER_ID_ON_OFF;
    req->cluster_list[2] = ZB_ZCL_CLUSTER_ID_IDENTIFY;

    zb_zdo_match_desc_req(param, match_desc_resp_cb);
}

static void match_desc_resp_cb(zb_uint8_t param)
{

    TRACE_MSG(TRACE_APP1, "match_desc_resp_cb: buf = %d", (FMT__D, param));

    ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, DUT_RETRIGGER_FB_DELAY);
    zb_buf_free(param);
}

/*! @} */
