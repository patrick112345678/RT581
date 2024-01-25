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
/* PURPOSE: TH ZR1 (initiator)
*/

#define ZB_TEST_NAME TP_BDB_FB_PRE_TC_03A_15_27_THR1
#define ZB_TRACE_FILE_ID 40537

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

#include "on_off_client.h"
#include "tp_bdb_fb_pre_tc_03a_15_27_common.h"
#include "../common/zb_cert_test_globals.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif


#ifndef ZB_CERTIFICATION_HACKS
#error ZB_CERTIFICATION_HACKS is not defined!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_thr1 = IEEE_ADDR_THR1;
static zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;

static zb_uint8_t g_nwk_key[16] = {0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                                  };

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(fb_pre_tc_03a_15_27_thr1_basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(fb_pre_tc_03a_15_27_thr1_identify_attr_list, &attr_identify_time);

/********************* Declare device **************************/
DECLARE_ON_OFF_CLIENT_CLUSTER_LIST(fb_pre_tc_03a_15_27_thr1_on_off_controller_clusters,
                                   fb_pre_tc_03a_15_27_thr1_basic_attr_list,
                                   fb_pre_tc_03a_15_27_thr1_identify_attr_list);

DECLARE_ON_OFF_CLIENT_EP(fb_pre_tc_03a_15_27_thr1_on_off_controller_ep,
                         THR1_ENDPOINT,
                         fb_pre_tc_03a_15_27_thr1_on_off_controller_clusters);

DECLARE_ON_OFF_CLIENT_CTX(fb_pre_tc_03a_15_27_thr1_on_off_controller_ctx, fb_pre_tc_03a_15_27_thr1_on_off_controller_ep);


static void trigger_fb_target(zb_uint8_t unused);
static void send_mgmt_bind_req_delayed(zb_uint8_t unused);
static void send_mgmt_bind_req(zb_uint8_t param);

MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_thr1");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif


    zb_set_long_address(g_ieee_addr_thr1);

    zb_set_network_router_role((1l << TEST_CHANNEL));
    zb_set_nvram_erase_at_start(ZB_TRUE);

    /* Assignment required to force Distributed formation */
    zb_aib_set_trust_center_address(g_unknown_ieee_addr);
    zb_secur_setup_nwk_key(g_nwk_key, 0);

    zb_set_max_children(2);

    ZB_AF_REGISTER_DEVICE_CTX(&fb_pre_tc_03a_15_27_thr1_on_off_controller_ctx);

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
            zb_ext_pan_id_t extended_pan_id;
            zb_get_extended_pan_id(extended_pan_id);

            if (ZB_IEEE_ADDR_CMP(g_ieee_addr_thr1, extended_pan_id))
            {
                bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
            }
            else
            {
                ZB_SCHEDULE_ALARM(trigger_fb_target, 0, THR1_FB_TARGET_DELAY);
            }
            ZB_SCHEDULE_ALARM(send_mgmt_bind_req_delayed, 0, THR1_MGMT_BIND_REQ_DELAY);
            ZB_SCHEDULE_ALARM(send_mgmt_bind_req_delayed, 0, 2 * THR1_MGMT_BIND_REQ_DELAY);
        }
        break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
        if (status == 0)
        {
            ZB_SCHEDULE_ALARM(trigger_fb_target, 0, THR1_FB_TARGET_DELAY);
        }
        break; /* ZB_BDB_SIGNAL_STEERING */

    default:
        TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}

static void trigger_fb_target(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    ZB_BDB().bdb_commissioning_time = FB_TARGET_DURATION;
    zb_bdb_finding_binding_target(THR1_ENDPOINT);
}

static void send_mgmt_bind_req_delayed(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    TRACE_MSG(TRACE_ERROR, ">> send_mgmt_bind_req_delayed", (FMT__0));

    if (zb_buf_get_out_delayed(send_mgmt_bind_req) != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "send_mgmt_bind_req_delayed: buffer allocation error", (FMT__0));
    }
}

static void send_mgmt_bind_req(zb_uint8_t param)
{
    zb_zdo_mgmt_bind_param_t *req = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_bind_param_t);

    TRACE_MSG(TRACE_ZDO2, ">>send_mgmt_bind_req: buf_param = %d",
              (FMT__D_H, param));

    req->start_index = 0;
    req->dst_addr = zb_address_short_by_ieee(g_ieee_addr_dut);

    zb_zdo_mgmt_bind_req(param, NULL);

    TRACE_MSG(TRACE_ZDO2, "<<send_mgmt_bind_req", (FMT__0));
}

/*! @} */
