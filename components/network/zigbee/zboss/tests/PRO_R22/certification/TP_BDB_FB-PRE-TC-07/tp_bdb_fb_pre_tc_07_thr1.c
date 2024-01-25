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
/* PURPOSE: TH ZR1
*/

#define ZB_TEST_NAME TP_BDB_FB_PRE_TC_07_THR1
#define ZB_TRACE_FILE_ID 40796

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
#include "device_th.h"

#include "device_dut.h"
#include "tp_bdb_fb_pre_tc_07_common.h"
#include "../common/zb_cert_test_globals.h"

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

static zb_ieee_addr_t g_ieee_addr_thr1 = IEEE_ADDR_THR1;
static zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;
static zb_uint8_t g_nwk_key[16] = {0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                                  };

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(fb_pre_tc_07_thr1_basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(fb_pre_tc_07_thr1_identify_attr_list, &attr_identify_time);

/********************* Declare device **************************/
DECLARE_TH_CLUSTER_LIST(fb_pre_tc_07_thr1_device_clusters,
                        fb_pre_tc_07_thr1_basic_attr_list,
                        fb_pre_tc_07_thr1_identify_attr_list);

DECLARE_TH_EP(fb_pre_tc_07_thr1_device_ep,
              THR1_ENDPOINT,
              fb_pre_tc_07_thr1_device_clusters);

DECLARE_TH_CTX(fb_pre_tc_07_thr1_device_ctx, fb_pre_tc_07_thr1_device_ep);

/***********************F&B***************************************/
static void trigger_fb_target(zb_uint8_t unused);


/************************General functions************************/
enum test_commands_e
{
    TEST_COMMAND_START_TEST,
    TEST_COMMAND_CHECK_BIND_TABLE1,
    TEST_COMMAND_BIND_CLUSTER,
    TEST_COMMAND_CHECK_BIND_TABLE2,
    TEST_COMMAND_WAIT_FOR_DUT_REBOOT,
    TEST_COMMAND_CHECK_BIND_TABLE3
};

static void device_annce_cb(zb_zdo_device_annce_t *da);
static void buffer_manager(zb_uint8_t cmd_idx);
static void send_mgmt_bind_req(zb_uint8_t param);
static void mgmt_bind_resp_cb(zb_uint8_t param);
static void send_bind_req(zb_uint8_t param);
static void bind_resp_cb(zb_uint8_t param);

/************************Static variables*************************/
static int N;
static int M;
static int s_bind_num;
static int s_test_step;

/************************Main*************************************/
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

    ZB_AF_REGISTER_DEVICE_CTX(&fb_pre_tc_07_thr1_device_ctx);

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
static void trigger_fb_target(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    ZB_BDB().bdb_commissioning_time = THR1_FB_DURATION;
    s_test_step = TEST_COMMAND_CHECK_BIND_TABLE1;
    zb_bdb_finding_binding_target(THR1_ENDPOINT);
}

/************************General functions************************/
static void buffer_manager(zb_uint8_t cmd_idx)
{
    TRACE_MSG(TRACE_APP1, ">>buffer_manager: cmd_idx = %d", (FMT__D, cmd_idx));
    TRACE_MSG(TRACE_APP1, "buffer_manager: N = %d, M = %d", (FMT__D_D, N, M));

    switch (cmd_idx)
    {
    case TEST_COMMAND_CHECK_BIND_TABLE1:
        zb_buf_get_out_delayed(send_mgmt_bind_req);
        break;
    case TEST_COMMAND_BIND_CLUSTER:
        zb_buf_get_out_delayed(send_bind_req);
        break;
    case TEST_COMMAND_CHECK_BIND_TABLE2:
        zb_buf_get_out_delayed(send_mgmt_bind_req);
        break;
    case TEST_COMMAND_WAIT_FOR_DUT_REBOOT:
        ++s_test_step;
        ZB_SCHEDULE_ALARM(buffer_manager, s_test_step, THR1_LONG_DELAY);
        break;
    case TEST_COMMAND_CHECK_BIND_TABLE3:
        zb_buf_get_out_delayed(send_mgmt_bind_req);
        break;
    default:
        TRACE_MSG(TRACE_APP1, "buffer_manager: unknown state transition", (FMT__0));
    }

    TRACE_MSG(TRACE_APP1, "<<buffer_manager", (FMT__0));
}


static void send_mgmt_bind_req(zb_uint8_t param)
{
    zb_zdo_mgmt_bind_param_t *req = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_bind_param_t);

    TRACE_MSG(TRACE_APP2, ">>send_mgmt_bind_req: buf_param = %d",
              (FMT__D_H, param));

    req->start_index = 0;
    req->dst_addr = zb_address_short_by_ieee(g_ieee_addr_dut);

    zb_zdo_mgmt_bind_req(param, mgmt_bind_resp_cb);

    TRACE_MSG(TRACE_APP2, "<<send_mgmt_bind_req", (FMT__0));
}

static void mgmt_bind_resp_cb(zb_uint8_t param)
{
    zb_zdo_mgmt_bind_resp_t *resp;
    zb_uint8_t start_idx, num_entries, table_size;

    resp = (zb_zdo_mgmt_bind_resp_t *) zb_buf_begin(param);

    TRACE_MSG(TRACE_APP2, ">>mgmt_bind_resp_cb: buf_param = %d, status = %d",
              (FMT__D_D, param, resp->status));

    if (resp->status == ZB_ZDP_STATUS_SUCCESS)
    {
        start_idx = resp->start_index;
        num_entries = resp->binding_table_list_count;
        table_size = resp->binding_table_entries;


        if (s_test_step == TEST_COMMAND_CHECK_BIND_TABLE1)
        {
            M = table_size;
        }

        ++s_test_step;
        TRACE_MSG(TRACE_APP2, "mgmt_bind_resp_cb: move to next step - %d",
                  (FMT__D, s_test_step));
        ZB_SCHEDULE_CALLBACK(buffer_manager, s_test_step);
    }

    zb_buf_free(param);

    TRACE_MSG(TRACE_APP2, "<<mgmt_bind_resp_cb", (FMT__0));
}

static void send_bind_req(zb_uint8_t param)
{
    zb_zdo_bind_req_param_t *req = ZB_BUF_GET_PARAM(param, zb_zdo_bind_req_param_t);
    zb_uint8_t endpoint_offset = THR1_ENDPOINT + 1;

    TRACE_MSG(TRACE_APP2, ">>send_bind_req: buf_param = %d",
              (FMT__D, param));

    ZB_IEEE_ADDR_COPY(req->src_address, g_ieee_addr_dut);
    req->src_endp = DUT_ENDPOINT;
    req->cluster_id = ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT;
    req->dst_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
    ZB_IEEE_ADDR_COPY(req->dst_address.addr_long, g_ieee_addr_thr1);
    req->dst_endp = endpoint_offset + s_bind_num;
    req->req_dst_addr = zb_address_short_by_ieee(g_ieee_addr_dut);

    zb_zdo_bind_req(param, bind_resp_cb);

    TRACE_MSG(TRACE_APP2, "<<send_bind_req", (FMT__0));
}

static void bind_resp_cb(zb_uint8_t param)
{
    zb_zdo_bind_resp_t *bind_resp = (zb_zdo_bind_resp_t *)zb_buf_begin(param);
    int make_new_bind = 0;
    int move_to_next_step = 0;

    TRACE_MSG(TRACE_APP2, ">>bind_resp_cb: buf_param = %d, status = %d",
              (FMT__D_D, param, bind_resp->status));

    if (bind_resp->status == ZB_ZDP_STATUS_SUCCESS)
    {
        ++s_bind_num;
        make_new_bind = s_bind_num < N - M;
        move_to_next_step = s_bind_num >= N - M;
    }
    else
    {
        TRACE_MSG(TRACE_APP1, "Test failed, invalid status", (FMT__0));
        move_to_next_step = (bind_resp->status == ZB_ZDP_STATUS_TABLE_FULL);
    }

    if (make_new_bind)
    {
        zb_buf_reuse(param);
        send_bind_req(param);
    }
    else
    {
        zb_buf_free(param);
    }

    if (move_to_next_step)
    {
        ++s_test_step;
        TRACE_MSG(TRACE_APP2, "bind_resp_cb: move to next step - %d",
                  (FMT__D, s_test_step));
        ZB_SCHEDULE_CALLBACK(buffer_manager, s_test_step);
    }

    TRACE_MSG(TRACE_APP2, "<<bind_resp_cb", (FMT__0));
}


/********************ZDO Startup*****************************/
static void device_annce_cb(zb_zdo_device_annce_t *da)
{
    TRACE_MSG(TRACE_APP1, ">>dev_annce_cb, ieee = " TRACE_FORMAT_64 " addr = 0x%x",
              (FMT__A_H, TRACE_ARG_64(da->ieee_addr), da->nwk_addr));

    if (ZB_IEEE_ADDR_CMP(g_ieee_addr_dut, da->ieee_addr) == ZB_TRUE)
    {
        TRACE_MSG(TRACE_APP1, "dev_annce_cb: DUT has joined to network!", (FMT__0));
        ZB_SCHEDULE_CALLBACK(trigger_fb_target, 0);
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
        TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));
        if (status == 0)
        {
            zb_ext_pan_id_t extended_pan_id;
            zb_get_extended_pan_id(extended_pan_id);

            N = ZB_CERT_HACKS().dst_binding_table_size;
            if (ZB_IEEE_ADDR_CMP(g_ieee_addr_thr1, extended_pan_id))
            {
                zb_zdo_register_device_annce_cb(device_annce_cb);
                bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
            }
            else
            {
                ZB_SCHEDULE_CALLBACK(trigger_fb_target, 0);
            }
        }
        break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APP1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
        break; /* ZB_BDB_SIGNAL_STEERING */

    case ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED:
        TRACE_MSG(TRACE_APP1, "signal: ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED, status %d", (FMT__D, status));
        if (status == 0)
        {
            if ((BDB_COMM_CTX().state == ZB_BDB_COMM_IDLE) &&
                    (s_test_step == TEST_COMMAND_CHECK_BIND_TABLE1))
            {
                ZB_SCHEDULE_CALLBACK(buffer_manager, s_test_step);
            }
        }
        break; /* ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED */

    default:
        TRACE_MSG(TRACE_APP1, "Unknown signal %d, status %d", (FMT__D_D, sig, status));
        break;
    }

    zb_buf_free(param);
}



/*! @} */
