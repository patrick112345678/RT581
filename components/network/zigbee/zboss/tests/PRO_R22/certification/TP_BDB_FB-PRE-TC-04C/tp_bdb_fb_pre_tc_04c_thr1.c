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
/* PURPOSE: FB-PRE-TC-04C: Service discovery - server side tests (TH ZR1)
*/

#define ZB_TEST_NAME TP_BDB_FB_PRE_TC_04C_THR1
#define ZB_TRACE_FILE_ID 40742

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

#include "tp_bdb_fb_pre_tc_04c_common.h"
#include "test_initiator.h"
#include "../common/zb_cert_test_globals.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
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

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(fb_pre_tc_04c_thr1_basic_attr_list,
                                 &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(fb_pre_tc_04c_thr1_identify_attr_list, &attr_identify_time);

static zb_bool_t attr_on_off = 1;
ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(fb_pre_tc_04c_thr1_on_off_attr_list, &attr_on_off);

/********************* Declare device **************************/
DECLARE_INITIATOR_CLUSTER_LIST(fb_pre_tc_04c_thr1_initiator_device_clusters,
                               fb_pre_tc_04c_thr1_basic_attr_list,
                               fb_pre_tc_04c_thr1_identify_attr_list,
                               fb_pre_tc_04c_thr1_on_off_attr_list);

DECLARE_INITIATOR_EP(fb_pre_tc_04c_thr1_initiator_device_ep,
                     THR1_ENDPOINT, fb_pre_tc_04c_thr1_initiator_device_clusters);

DECLARE_INITIATOR_CTX(fb_pre_tc_04c_thr1_initiator_device_ctx,
                      fb_pre_tc_04c_thr1_initiator_device_ep);

/**********************General definitions for test***********************/

static void send_simple_desc_req_delayed(zb_uint8_t unused);
static void send_simple_desc_req(zb_uint8_t param);
static void simple_desc_callback(zb_uint8_t param);

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

    zb_aib_set_trust_center_address(g_unknown_ieee_addr);
    zb_secur_setup_nwk_key(g_nwk_key, 0);

    ZB_AF_REGISTER_DEVICE_CTX(&fb_pre_tc_04c_thr1_initiator_device_ctx);

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
static void send_simple_desc_req_delayed(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    zb_buf_get_out_delayed(send_simple_desc_req);
}

static void send_simple_desc_req(zb_uint8_t param)
{
    zb_zdo_simple_desc_req_t *req;

    req = zb_buf_initial_alloc(param, sizeof(zb_zdo_simple_desc_req_t));

    req->nwk_addr = zb_address_short_by_ieee(g_ieee_addr_dut);
    req->endpoint = DUT_ENDPOINT;

    zb_zdo_simple_desc_req(param, simple_desc_callback);
}

static void simple_desc_callback(zb_uint8_t param)
{
    zb_uint8_t *zdp_cmd = zb_buf_begin(param);
    zb_zdo_simple_desc_resp_t *resp = (zb_zdo_simple_desc_resp_t *)(zdp_cmd);

    TRACE_MSG(TRACE_APS1, "simple_desc_callback status %hd, addr 0x%x",
              (FMT__H, resp->hdr.status, resp->hdr.nwk_addr));
    if (resp->hdr.status != ZB_ZDP_STATUS_SUCCESS)
    {
        TRACE_MSG(TRACE_APS1, "Error incorrect status/addr", (FMT__0));
    }

    zb_buf_free(param);
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
                ZB_SCHEDULE_ALARM(send_simple_desc_req_delayed, 0, THR1_DELAY);
            }
        }
        break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
        if (status == 0)
        {
            ZB_SCHEDULE_ALARM(send_simple_desc_req_delayed, 0, THR1_DELAY);
        }
        break; /* ZB_BDB_SIGNAL_STEERING */

    default:
        TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}

/*! @} */
