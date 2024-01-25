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

#define ZB_TEST_NAME TP_BDB_FB_PRE_TC_03A_15_26_DUT_R
#define ZB_TRACE_FILE_ID 40058

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
#include "tp_bdb_fb_pre_tc_03a_15_26_common.h"
#include "../common/zb_cert_test_globals.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;
static zb_ieee_addr_t g_ieee_addr_thr1 = IEEE_ADDR_THR1;

static zb_uint8_t g_nwk_key[16] = {0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                                  };

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(fb_pre_tc_03a_15_26_dut_r_basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(fb_pre_tc_03a_15_26_dut_r_identify_attr_list, &attr_identify_time);

static zb_bool_t attr_on_off = 1;
ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(fb_pre_tc_03a_15_26_dut_r_on_off_attr_list, &attr_on_off);

/********************* Declare device **************************/
DECLARE_ON_OFF_SERVER_CLUSTER_LIST(fb_pre_tc_03a_15_26_dut_r_on_off_device_clusters,
                                   fb_pre_tc_03a_15_26_dut_r_basic_attr_list,
                                   fb_pre_tc_03a_15_26_dut_r_identify_attr_list,
                                   fb_pre_tc_03a_15_26_dut_r_on_off_attr_list);

DECLARE_ON_OFF_SERVER_EP(fb_pre_tc_03a_15_26_dut_r_on_off_device_ep,
                         DUT_ENDPOINT,
                         fb_pre_tc_03a_15_26_dut_r_on_off_device_clusters);

DECLARE_ON_OFF_SERVER_CTX(fb_pre_tc_03a_15_26_dut_r_on_off_device_ctx, fb_pre_tc_03a_15_26_dut_r_on_off_device_ep);

extern zb_uint8_t zdo_send_req_by_short(zb_uint16_t command_id, zb_uint8_t param, zb_callback_t cb,
                                        zb_uint16_t addr, zb_uint8_t resp_count);

static void device_annce_cb(zb_zdo_device_annce_t *da);
static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster);
static void trigger_fb_initiator(zb_uint8_t unused);
static void send_active_ep_req_delayed(zb_uint8_t unused);
static void send_active_ep_req(zb_uint8_t param);
static void active_ep_callback(zb_uint8_t param);

static zb_ieee_addr_t s_target_ieee;
static zb_bool_t s_send_to_target;

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

    zb_set_network_router_role((1l << TEST_CHANNEL));
    zb_set_nvram_erase_at_start(ZB_TRUE);
    zb_secur_setup_nwk_key(g_nwk_key, 0);

    /* Assignment required to force Distributed formation */
    zb_aib_set_trust_center_address(g_unknown_ieee_addr);
    zb_set_max_children(1);

    ZB_AF_REGISTER_DEVICE_CTX(&fb_pre_tc_03a_15_26_dut_r_on_off_device_ctx);
    s_send_to_target = ZB_TRUE;

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

static void device_annce_cb(zb_zdo_device_annce_t *da)
{
    TRACE_MSG(TRACE_APP1, ">>dev_annce_cb, ieee = " TRACE_FORMAT_64 " addr = 0x%x",
              (FMT__A_H, TRACE_ARG_64(da->ieee_addr), da->nwk_addr));

    if (ZB_IEEE_ADDR_CMP(g_ieee_addr_thr1, da->ieee_addr) == ZB_TRUE)
    {
        TRACE_MSG(TRACE_APP1, "dev_annce_cb: THr1 has joined to network!", (FMT__0));
        ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, DUT_FB_INITIATOR_DELAY);
    }

    TRACE_MSG(TRACE_APP1, "<<dev_annce_cb", (FMT__0));
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
            zb_zdo_register_device_annce_cb(device_annce_cb);
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        }
        break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
        break; /* ZB_BDB_SIGNAL_STEERING */

    case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:
        TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED, status %d", (FMT__D, status));
        if (status == 0)
        {
            if (BDB_COMM_CTX().state == ZB_BDB_COMM_IDLE)
            {
                ZB_SCHEDULE_CALLBACK(send_active_ep_req_delayed, 0);
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

static void send_active_ep_req_delayed(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    zb_buf_get_out_delayed(send_active_ep_req);
}

static void send_active_ep_req(zb_uint8_t param)
{
    zb_zdo_active_ep_req_t *req;
    zb_uint16_t addr;

    req = zb_buf_initial_alloc(param, sizeof(zb_zdo_active_ep_req_t));
    req->nwk_addr = zb_address_short_by_ieee(s_target_ieee);

    addr = req->nwk_addr;
    ZB_HTOLE16(&req->nwk_addr, &addr);

    if (s_send_to_target)
    {
        addr = zb_address_short_by_ieee(s_target_ieee);
    }
    else
    {
        addr = zb_address_short_by_ieee(g_ieee_addr_thr1);
    }

    zdo_send_req_by_short(ZDO_ACTIVE_EP_REQ_CLID, param, active_ep_callback, addr, ZB_ZDO_CB_DEFAULT_COUNTER);
}

static void active_ep_callback(zb_uint8_t param)
{
    zb_uint8_t *zdp_cmd = zb_buf_begin(param);
    zb_zdo_ep_resp_t *resp = (zb_zdo_ep_resp_t *)zdp_cmd;

    TRACE_MSG(TRACE_APS1, "active_ep_callback status %hd, addr 0x%x",
              (FMT__H, resp->status, resp->nwk_addr));

    if (resp->status != ZB_ZDP_STATUS_SUCCESS)
    {
        TRACE_MSG(TRACE_APS1, "Error incorrect status", (FMT__0));
    }

    s_send_to_target = !s_send_to_target;

    zb_buf_free(param);
    ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, DUT_RETRIGGER_FB_DELAY);
}

/*! @} */
