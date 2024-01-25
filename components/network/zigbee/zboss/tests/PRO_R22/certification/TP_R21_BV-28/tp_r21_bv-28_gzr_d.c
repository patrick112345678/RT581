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
/* PURPOSE: TP_R21_BV-28 ZigBee Router (gZR): forming distributed network.
*/

#define ZB_TEST_NAME TP_R21_BV_28_GZR_D
#define ZB_TRACE_FILE_ID 40679

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif



enum test_fsm_states_e
{
    TEST_STATE_ITEM_1 = 1,
    TEST_STATE_ITEM_2,
    TEST_STATE_ITEM_3,
    TEST_STATE_ITEM_4,
    TEST_STATE_ITEM_5,
    TEST_STATE_ITEM_6,
    TEST_STATE_ITEM_7,
    TEST_STATE_ITEM_8,
    TEST_STATE_ITEM_9,
    TEST_STATE_ITEM_10,
    TEST_STATE_ITEM_11,
    TEST_STATE_ITEM_12,
    TEST_STATE_ITEM_13,
    TEST_STATE_ITEM_14,
    TEST_STATE_ITEM_15
};


static void get_peer_address(zb_uint8_t param);
static void test_bind_fsm(zb_uint8_t param);
static void bind_resp_cb(zb_uint8_t param);
static void test_unbind_fsm(zb_uint8_t param);
static void unbind_resp_cb(zb_uint8_t param);

static void send_mgmt_bind_req(zb_uint8_t param);
static void mgmt_bind_resp_cb(zb_uint8_t param);

static const zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;
static const zb_ieee_addr_t g_ieee_addr_gzr = IEEE_ADDR_gZR;
static const zb_ieee_addr_t g_ieee_addr_target1 = IEEE_ADDR_TARGET1;
static const zb_ieee_addr_t g_ieee_addr_target2 = IEEE_ADDR_TARGET2;

static zb_uint16_t s_dut_short_addr;
static zb_uint8_t s_start_idx = 0;
static int s_test_fsm_step = TEST_STATE_ITEM_1;


MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("zdo_2_gzr_d");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    zb_set_long_address(g_ieee_addr_gzr);
    zb_aib_set_trust_center_address(g_unknown_ieee_addr);
    zb_zdo_set_aps_unsecure_join(ZB_TRUE);
    zb_set_nvram_erase_at_start(ZB_TRUE);
    zb_set_pan_id(TEST_PAN_ID);
    zb_cert_test_set_network_addr(0x2bbb);
    zb_secur_setup_nwk_key((zb_uint8_t *) g_nwk_key, 0);
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zr_role();

#ifdef SECURITY_LEVEL
    zb_cert_test_set_security_level(SECURITY_LEVEL);
#endif

    if (zboss_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
    }
    else
    {
        zboss_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    TRACE_MSG(TRACE_ERROR, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    if (0 == status)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));
            ZB_SCHEDULE_ALARM(get_peer_address, 0, TEST_SEND_FIRST_CMD_DELAY);
            break;

        default:
            TRACE_MSG(TRACE_ERROR, "Unknown signal %hd", (FMT__H, sig));
        }
    }
    else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
    {
        TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, status));
    }

    if (param)
    {
        zb_buf_free(param);
    }
}


static void get_peer_address(zb_uint8_t param)
{
    zb_bufid_t buf = zb_buf_get_out();

    ZVUNUSED(param);

    TRACE_MSG(TRACE_ZDO1, ">>get_peer_address", (FMT__0));

    if (buf)
    {
        s_dut_short_addr = zb_address_short_by_ieee((zb_uint8_t *) g_ieee_addr_dut);
        s_test_fsm_step = TEST_STATE_ITEM_2;

        TRACE_MSG(TRACE_ZDO2, "get_peer_address: dut_short_addr = %h", (FMT__H, s_dut_short_addr));

        zb_buf_initial_alloc(buf, 0);
        ZB_SCHEDULE_CALLBACK(test_bind_fsm, buf);
    }
    else
    {
        TRACE_MSG(TRACE_ZDO1, "TEST FAILED: Could not get out buf!", (FMT__0));
    }

    TRACE_MSG(TRACE_ZDO1, "<<get_peer_addressh", (FMT__0));
}


static void test_bind_fsm(zb_uint8_t param)
{
    zb_zdo_bind_req_param_t *bind_req_param;

    TRACE_MSG(TRACE_ZDO2, ">>test_bind_fsm: param = %i, test step = %i",
              (FMT__D_D, param, s_test_fsm_step));

    bind_req_param = ZB_BUF_GET_PARAM(param, zb_zdo_bind_req_param_t);

    ZB_IEEE_ADDR_COPY(bind_req_param->src_address, g_ieee_addr_dut);
    bind_req_param->src_endp = 1;
    bind_req_param->req_dst_addr = s_dut_short_addr;

    switch (s_test_fsm_step)
    {
    case TEST_STATE_ITEM_2:
    case TEST_STATE_ITEM_3:
        bind_req_param->cluster_id = 0x0006;
        bind_req_param->dst_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
        ZB_IEEE_ADDR_COPY(bind_req_param->dst_address.addr_long, g_ieee_addr_target1);
        bind_req_param->dst_endp = 1;
        break;

    case TEST_STATE_ITEM_4:
        bind_req_param->cluster_id = 0x0006;
        bind_req_param->dst_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
        ZB_IEEE_ADDR_COPY(bind_req_param->dst_address.addr_long, g_ieee_addr_target1);
        bind_req_param->dst_endp = 2;
        break;

    case TEST_STATE_ITEM_5:
        bind_req_param->cluster_id = 0x0008;
        bind_req_param->dst_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
        ZB_IEEE_ADDR_COPY(bind_req_param->dst_address.addr_long, g_ieee_addr_target1);
        bind_req_param->dst_endp = 1;
        break;

    case TEST_STATE_ITEM_6:
        bind_req_param->cluster_id = 0x0006;
        bind_req_param->dst_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
        ZB_IEEE_ADDR_COPY(bind_req_param->dst_address.addr_long, g_ieee_addr_target2);
        bind_req_param->dst_endp = 1;
        break;

    case TEST_STATE_ITEM_7:
        bind_req_param->cluster_id = 0x0006;
        bind_req_param->dst_addr_mode = ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT;
        bind_req_param->dst_address.addr_short = g_dest_group_addr;
        break;
    }

    zb_zdo_bind_req(param, bind_resp_cb);

    TRACE_MSG(TRACE_ZDO2, "<<test_bind_fsm", (FMT__0));
}


static void bind_resp_cb(zb_uint8_t param)
{
    zb_zdo_bind_resp_t *bind_resp;
    zb_callback_t call_cb = 0;

    TRACE_MSG(TRACE_ZDO2, ">>bind_resp_cb: param = %i, test step = %i",
              (FMT__D_D, param, s_test_fsm_step));

    bind_resp = (zb_zdo_bind_resp_t *)zb_buf_begin(param);

    TRACE_MSG(TRACE_ZDO2, "bind_resp_cb: resp status %hd", (FMT__H, bind_resp->status));

    switch (s_test_fsm_step)
    {
    case TEST_STATE_ITEM_2:
    case TEST_STATE_ITEM_3:
        if (bind_resp->status != ZB_ZDP_STATUS_SUCCESS)
        {
            TRACE_MSG(TRACE_ERROR, "bind_resp_cb: ITEM FAILED (item = %i)",
                      (FMT__D, s_test_fsm_step));
        }
        call_cb = test_bind_fsm;
        break;

    case TEST_STATE_ITEM_4:
    case TEST_STATE_ITEM_5:
    case TEST_STATE_ITEM_6:
        if ( !((bind_resp->status == ZB_ZDP_STATUS_SUCCESS) ||
                (bind_resp->status == ZB_ZDP_STATUS_TABLE_FULL)) )
        {
            TRACE_MSG(TRACE_ERROR, "bind_resp_cb: ITEM FAILED (item = %i)",
                      (FMT__D, s_test_fsm_step));
        }
        call_cb = test_bind_fsm;
        break;

    case TEST_STATE_ITEM_7:
        if ( !((bind_resp->status == ZB_ZDP_STATUS_SUCCESS) ||
                (bind_resp->status == ZB_ZDP_STATUS_TABLE_FULL)) )
        {
            TRACE_MSG(TRACE_ERROR, "bind_resp_cb: ITEM FAILED (item = %i)",
                      (FMT__D, s_test_fsm_step));
        }
        call_cb = send_mgmt_bind_req;
        break;
    }

    s_test_fsm_step++;
    TRACE_MSG(TRACE_ERROR, "bind_resp_cb: go to next item = %i",
              (FMT__D, s_test_fsm_step));

    if (call_cb)
    {
        zb_buf_reuse(param);
#if 0
        ZB_SWITCH_BUF(buf, 0); /* Switch buffer to out */
#endif
        ZB_SCHEDULE_CALLBACK(call_cb, param);
    }
    else
    {
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_ZDO2, "<<bind_resp_cb", (FMT__0));
}


static void test_unbind_fsm(zb_uint8_t param)
{
    zb_zdo_bind_req_param_t *unbind_req_param;

    TRACE_MSG(TRACE_ZDO2, ">>test_unbind_fsm: param = %i, test step = %i",
              (FMT__D_D, param, s_test_fsm_step));

    unbind_req_param = ZB_BUF_GET_PARAM(param, zb_zdo_bind_req_param_t);

    ZB_IEEE_ADDR_COPY(unbind_req_param->src_address, g_ieee_addr_dut);
    unbind_req_param->src_endp = 1;
    unbind_req_param->req_dst_addr = s_dut_short_addr;

    switch (s_test_fsm_step)
    {
    case TEST_STATE_ITEM_9:
    case TEST_STATE_ITEM_10:
        unbind_req_param->cluster_id = 0x0006;
        unbind_req_param->dst_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
        ZB_IEEE_ADDR_COPY(unbind_req_param->dst_address.addr_long, g_ieee_addr_target1);
        unbind_req_param->dst_endp = 1;
        break;

    case TEST_STATE_ITEM_11:
        unbind_req_param->cluster_id = 0x0006;
        unbind_req_param->dst_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
        ZB_IEEE_ADDR_COPY(unbind_req_param->dst_address.addr_long, g_ieee_addr_target1);
        unbind_req_param->dst_endp = 2;
        break;

    case TEST_STATE_ITEM_12:
        unbind_req_param->cluster_id = 0x0008;
        unbind_req_param->dst_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
        ZB_IEEE_ADDR_COPY(unbind_req_param->dst_address.addr_long, g_ieee_addr_target1);
        unbind_req_param->dst_endp = 1;
        break;

    case TEST_STATE_ITEM_13:
        unbind_req_param->cluster_id = 0x0006;
        unbind_req_param->dst_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
        ZB_IEEE_ADDR_COPY(unbind_req_param->dst_address.addr_long, g_ieee_addr_target2);
        unbind_req_param->dst_endp = 1;
        break;

    case TEST_STATE_ITEM_14:
        unbind_req_param->cluster_id = 0x0006;
        unbind_req_param->dst_addr_mode = ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT;
        unbind_req_param->dst_address.addr_short = g_dest_group_addr;
        break;
    }

    zb_zdo_unbind_req(param, unbind_resp_cb);

    TRACE_MSG(TRACE_ZDO2, "<<test_unbind_fsm", (FMT__0));
}


static void unbind_resp_cb(zb_uint8_t param)
{
    zb_zdo_bind_resp_t *unbind_resp;
    zb_callback_t call_cb = 0;

    TRACE_MSG(TRACE_ZDO2, ">>unbind_resp_cb: param = %i, test step = %i",
              (FMT__D_D, param, s_test_fsm_step));

    unbind_resp = (zb_zdo_bind_resp_t *)zb_buf_begin(param);

    TRACE_MSG(TRACE_ZDO2, "unbind_resp_cb: resp status %hd", (FMT__H, unbind_resp->status));

    switch (s_test_fsm_step)
    {
    case TEST_STATE_ITEM_9:
        if (unbind_resp->status != ZB_ZDP_STATUS_SUCCESS)
        {
            TRACE_MSG(TRACE_ERROR, "unbind_resp_cb: ITEM FAILED (item = %i)",
                      (FMT__D, s_test_fsm_step));
        }
        call_cb = test_unbind_fsm;
        break;

    case TEST_STATE_ITEM_10:
        if (unbind_resp->status != ZB_ZDP_STATUS_NO_ENTRY)
        {
            TRACE_MSG(TRACE_ERROR, "unbind_resp_cb: ITEM FAILED (item = %i)",
                      (FMT__D, s_test_fsm_step));
        }
        call_cb = test_unbind_fsm;
        break;

    case TEST_STATE_ITEM_11:
    case TEST_STATE_ITEM_12:
    case TEST_STATE_ITEM_13:
        if ( !((unbind_resp->status == ZB_ZDP_STATUS_SUCCESS) ||
                (unbind_resp->status == ZB_ZDP_STATUS_NO_ENTRY)) )
        {
            TRACE_MSG(TRACE_ERROR, "unbind_resp_cb: ITEM FAILED (item = %i)",
                      (FMT__D, s_test_fsm_step));
        }
        call_cb = test_unbind_fsm;
        break;

    case TEST_STATE_ITEM_14:
        if ( !((unbind_resp->status == ZB_ZDP_STATUS_SUCCESS) ||
                (unbind_resp->status == ZB_ZDP_STATUS_NO_ENTRY)) )
        {
            TRACE_MSG(TRACE_ERROR, "unbind_resp_cb: ITEM FAILED (item = %i)",
                      (FMT__D, s_test_fsm_step));
        }
        call_cb = send_mgmt_bind_req;
        break;
    }

    s_test_fsm_step++;
    TRACE_MSG(TRACE_ERROR, "unbind_resp_cb: go to next item = %i",
              (FMT__D, s_test_fsm_step));

    if (call_cb)
    {
        zb_buf_reuse(param);
#if 0
        ZB_SWITCH_BUF(buf, 0); /* Switch buffer to out */
#endif
        ZB_SCHEDULE_CALLBACK(call_cb, param);
    }
    else
    {
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_ZDO2, "<<unbind_resp_cb", (FMT__0));
}



static void send_mgmt_bind_req(zb_uint8_t param)
{
    zb_zdo_mgmt_bind_param_t *req_params;

    TRACE_MSG(TRACE_ZDO3, ">>send_mgmt_bind_req: param = %i", (FMT__D, param));

    req_params = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_bind_param_t);
    req_params->start_index = s_start_idx;
    req_params->dst_addr = s_dut_short_addr;
    zb_zdo_mgmt_bind_req(param, mgmt_bind_resp_cb);

    TRACE_MSG(TRACE_ZDO3, "<<send_mgmt_bind_req", (FMT__0));
}


static void mgmt_bind_resp_cb(zb_uint8_t param)
{
    zb_zdo_mgmt_bind_resp_t *resp;
    zb_callback_t call_cb = 0;

    TRACE_MSG(TRACE_ZDO1, ">>mgmt_bind_resp_cb: param = %i", (FMT__D, param));

    resp = (zb_zdo_mgmt_bind_resp_t *) zb_buf_begin(param);

    TRACE_MSG(TRACE_ZDO1, "mgmt_bind_resp_cb: status = %i", (FMT__D, resp->status));

    if (resp->status == ZB_ZDP_STATUS_SUCCESS)
    {
        zb_uint8_t nbrs = resp->binding_table_list_count;

        s_start_idx += nbrs;
        if (s_start_idx < resp->binding_table_entries)
        {
            TRACE_MSG(TRACE_ZDO2, "mgmt_bind_resp_cb: retrieved = %d, total = %d",
                      (FMT__D_D, nbrs, resp->binding_table_entries));
            call_cb = send_mgmt_bind_req;
        }
        else
        {
            TRACE_MSG(TRACE_ZDO2, "mgmt_bind_resp_cb: retrieved all entries - %d",
                      (FMT__D, resp->binding_table_entries));

            s_start_idx = 0;
            if (s_test_fsm_step == TEST_STATE_ITEM_8)
            {
                TRACE_MSG(TRACE_ERROR, "mgmt_bind_resp_cb: next item = %i",
                          (FMT__D, s_test_fsm_step));
                ++s_test_fsm_step;
                call_cb = test_unbind_fsm;
            }
            else
            {
                TRACE_MSG(TRACE_ERROR, "mgmt_bind_resp_cb: TEST FINISHED", (FMT__0));
            }
        }
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "mgmt_bind_resp_cb: TEST_FAILED (item = %i)",
                  (FMT__D, s_test_fsm_step));
    }

    if (call_cb)
    {
        zb_buf_reuse(param);
#if 0
        ZB_SWITCH_BUF(buf, 0); /* Switch buffer to out */
#endif
        ZB_SCHEDULE_CALLBACK(call_cb, param);
    }
    else
    {
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_ZDO1, "<<mgmt_bind_resp_cb", (FMT__0));
}

/*! @} */

