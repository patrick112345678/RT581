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
/* PURPOSE: TP_R21_BV-27 ZigBee Router (gZR): forming distributed network.
*/

#define ZB_TEST_NAME TP_R21_BV_27_GZR_D
#define ZB_TRACE_FILE_ID 40597

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


#define BV_27_DEV_TYPE_RFD 0
#define BV_27_DEV_TYPE_FFD 1


enum bv_27_step_e
{
    BV_27_STEP_START,
    BV_27_STEP_SEND_LEAVE_CMD1,
    BV_27_STEP_SEND_LEAVE_CMD2,
    BV_27_STEP_SEND_LEAVE_CMD3,
    BV_27_STEP_FINISH
};


static void send_mgmt_leave_cmd1(zb_uint8_t param);
static void send_mgmt_leave_cmd2(zb_uint8_t param);
static void send_mgmt_leave_cmd3(zb_uint8_t param);

static void device_annce_cb(zb_zdo_device_annce_t *da);

static void send_mgmt_leave(zb_uint8_t param,
                            zb_ieee_addr_t device,
                            zb_uint16_t cmd_dest_addr,
                            zb_bool_t flag_rejoin,
                            zb_bool_t flag_remove_children);
static void mgmt_leave_resp(zb_uint8_t param);

static void send_mgmt_lqi_req(zb_uint8_t param);
static void mgmt_lqi_resp(zb_uint8_t param);

static void send_buffer_test_req(zb_uint8_t param);
static void send_buffer_test_req_delayed(zb_uint8_t param);
static void buffer_test_request_cb(zb_uint8_t param);

static const zb_ieee_addr_t g_ieee_addr_gzr = IEEE_ADDR_gZR;
static const zb_ieee_addr_t g_ieee_addr_gzed = IEEE_ADDR_gZED;
static const zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;

static int s_next_step = BV_27_STEP_START;
static zb_uint16_t s_dut_short_addr;
static zb_uint16_t s_gzed_short_addr;
static zb_uint16_t s_dest_addr;
static zb_uint8_t s_dut_type; /* RFD or FFD */
static zb_uint8_t s_start_idx = 0;



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
    /* For starting Distributed Network */
    zb_secur_setup_nwk_key((zb_uint8_t *) g_nwk_key, 0);
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zr_role();
    zb_set_max_children(1);
    zb_set_pan_id(TEST_PAN_ID);
    zb_set_nvram_erase_at_start(ZB_TRUE);
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

            zb_zdo_register_device_annce_cb(device_annce_cb);
            s_next_step = BV_27_STEP_SEND_LEAVE_CMD1;
            ZB_SCHEDULE_ALARM(send_mgmt_leave_cmd1, 0, TEST_SEND_LEAVE_CMD_DELAY);
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


static void send_mgmt_leave_cmd1(zb_uint8_t param)
{
    zb_bufid_t buf = zb_buf_get_out();

    ZVUNUSED(param);

    TRACE_MSG(TRACE_ZDO1, ">>send_mgmt_leave_cmd1", (FMT__0));

    if (buf)
    {
        zb_ieee_addr_t device_addr;

        TRACE_MSG(TRACE_ERROR, "TP_R21_BV-27: TEST STARTED", (FMT__0));
        s_next_step = BV_27_STEP_SEND_LEAVE_CMD2;
        ZB_IEEE_ADDR_ZERO(device_addr);
        send_mgmt_leave(buf,
                        device_addr,
                        s_dut_short_addr,
                        ZB_TRUE /* rejoin */,
                        ZB_FALSE /* remove children */);
        s_dest_addr = s_dut_short_addr;
        ZB_SCHEDULE_ALARM(send_buffer_test_req, 0, TEST_SEND_BUFFER_TEST_REQ_DELAY);
    }
    else
    {
        TRACE_MSG(TRACE_ZDO1, "TEST FAILED: Could not get out buf!", (FMT__0));
    }

    TRACE_MSG(TRACE_ZDO1, "<<send_mgmt_leave_cmd1", (FMT__0));
}


static void send_mgmt_leave_cmd2(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ZDO1, ">>send_mgmt_leave_cmd2: param = %d", (FMT__D, (int)param));

    s_next_step = BV_27_STEP_SEND_LEAVE_CMD3;
    if (s_dut_type == BV_27_DEV_TYPE_FFD)
    {
        zb_ieee_addr_t device_addr;

        ZB_IEEE_ADDR_COPY(device_addr, g_ieee_addr_gzed);
        send_mgmt_leave(param,
                        device_addr,
                        s_dut_short_addr,
                        ZB_FALSE /* rejoin */,
                        ZB_FALSE /* remove children */);
        s_dest_addr = s_gzed_short_addr;
        ZB_SCHEDULE_ALARM(send_buffer_test_req, 0, TEST_SEND_BUFFER_TEST_REQ_DELAY);
    }
    else
    {
        /* skip this step - go to step send_mgmt_leave_cmd3 */
        send_mgmt_leave_cmd3(param);
    }

    TRACE_MSG(TRACE_ZDO1, "<<send_mgmt_leave_cmd2", (FMT__0));
}


static void send_mgmt_leave_cmd3(zb_uint8_t param)
{
    zb_ieee_addr_t device_addr;

    TRACE_MSG(TRACE_ZDO1, ">>send_mgmt_leave_cmd3: param = %d", (FMT__D, (int)param));

    ZB_IEEE_ADDR_ZERO(device_addr);
    s_next_step = BV_27_STEP_FINISH;
    send_mgmt_leave(param,
                    device_addr,
                    s_dut_short_addr,
                    ZB_FALSE /* rejoin */,
                    ZB_FALSE /* remove children */);
    s_dest_addr = s_dut_short_addr;
    ZB_SCHEDULE_ALARM(send_buffer_test_req, 0, TEST_SEND_BUFFER_TEST_REQ_DELAY);

    TRACE_MSG(TRACE_ZDO1, "<<send_mgmt_leave_cmd3", (FMT__0));
}



static void device_annce_cb(zb_zdo_device_annce_t *da)
{
    TRACE_MSG(TRACE_ZDO1, ">>dev_annce_cb, ieee = " TRACE_FORMAT_64 " addr = %h",
              (FMT__A_H, TRACE_ARG_64(da->ieee_addr), da->nwk_addr));
    if (ZB_IEEE_ADDR_CMP(g_ieee_addr_gzed, da->ieee_addr) == ZB_TRUE)
    {
        s_gzed_short_addr = da->nwk_addr;
        TRACE_MSG(TRACE_ZDO2, "dev_annce_cb: gZED is joined!", (FMT__0));
    }
    else if (ZB_IEEE_ADDR_CMP(g_ieee_addr_dut, da->ieee_addr) == ZB_TRUE)
    {
        s_dut_short_addr = da->nwk_addr;
        s_dut_type = ZB_MAC_CAP_GET_DEVICE_TYPE(da->capability);
        /* If ZED is joined to gZR, it is definitely DUTZED case, and we are communicating with DUTZED
         * only.
         * TODO: Maybe it will be better to divide these to 3 different tests if possible - to do not
         * use such complicated logic. */
        if (s_dut_type == BV_27_DEV_TYPE_RFD)
        {
            s_dest_addr = s_dut_short_addr;
        }
        TRACE_MSG(TRACE_ZDO2, "dev_annce_cb: DUT is joined!", (FMT__0));
    }
    TRACE_MSG(TRACE_ZDO1, "<<dev_annce_cb", (FMT__0));
}


static void send_mgmt_leave(zb_uint8_t param,
                            zb_ieee_addr_t device,
                            zb_uint16_t cmd_dest_addr,
                            zb_bool_t flag_rejoin,
                            zb_bool_t flag_remove_children)
{
    zb_zdo_mgmt_leave_param_t *req = NULL;

    TRACE_MSG(TRACE_ZDO2, ">>send_mgmt_leave: param = %d, dest = %h",
              (FMT__D_H, (int)param, cmd_dest_addr));


    req = zb_buf_initial_alloc(param, 0);
    req = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_leave_param_t);
    req->remove_children = flag_remove_children;
    req->rejoin = flag_rejoin;
    ZB_MEMCPY(req->device_address, device, sizeof(zb_ieee_addr_t));
    req->dst_addr = cmd_dest_addr;
    zdo_mgmt_leave_req(param, mgmt_leave_resp);

    TRACE_MSG(TRACE_ZDO2, "<<send_mgmt_leave", (FMT__0));
}


static void mgmt_leave_resp(zb_uint8_t param)
{
    zb_zdo_mgmt_leave_res_t *resp = (zb_zdo_mgmt_leave_res_t *) zb_buf_begin(param);
    zb_bool_t stop_test = ZB_FALSE;
    zb_callback_t call_cb = NULL;

    TRACE_MSG(TRACE_ERROR, ">>mgmt_leave_resp: param = %d", (FMT__D, param));

    if ((resp->status == ZB_ZDP_STATUS_SUCCESS) ||
            (resp->status == ZB_NWK_STATUS_INVALID_REQUEST) ||
            (resp->status == ZB_ZDP_STATUS_TIMEOUT))
    {
        switch (s_next_step)
        {
        case BV_27_STEP_SEND_LEAVE_CMD2:
            call_cb = send_mgmt_leave_cmd2;
            break;

        case BV_27_STEP_SEND_LEAVE_CMD3:
            call_cb = send_mgmt_lqi_req;
            break;

        case BV_27_STEP_FINISH:
            TRACE_MSG(TRACE_ERROR, "TP_R21_BV-27: TEST FINISHED", (FMT__0));
            stop_test = ZB_TRUE;
            break;

        default:
            TRACE_MSG(TRACE_ERROR, "Unknown test state: next state = %d",
                      (FMT__D, s_next_step));
            stop_test = ZB_TRUE;
            break;
        }
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "mgmt_leave_resp: operation failed, status =  %hd",
                  (FMT__H, resp->status));
        stop_test = ZB_TRUE;
    }

    if (stop_test == ZB_TRUE)
    {
        zb_buf_free(param);
    }
    else
    {
        zb_buf_reuse(param);
#if 0
        ZB_SWITCH_BUF(buf, 0); /* Switch buffer to out */
#endif
        ZB_SCHEDULE_ALARM(call_cb, param, TEST_SEND_LEAVE_CMD_DELAY);
    }

    TRACE_MSG(TRACE_ERROR, "mgmt_leave_resp", (FMT__0));
}


static void send_mgmt_lqi_req(zb_uint8_t param)
{
    zb_zdo_mgmt_lqi_param_t *req_param = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_lqi_param_t);

    TRACE_MSG(TRACE_ZDO1, ">>mgmt_lqi_resp buf = %d", (FMT__D, param));

    if (s_dut_short_addr != ZB_UNKNOWN_SHORT_ADDR)
    {
        req_param->dst_addr = s_dut_short_addr;
        req_param->start_index = s_start_idx;
        zb_zdo_mgmt_lqi_req(param, mgmt_lqi_resp);
    }
    else
    {
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_ZDO1, "<<mgmt_lqi_resp buf", (FMT__0));
}


static void mgmt_lqi_resp(zb_uint8_t param)
{
    zb_zdo_mgmt_lqi_resp_t *resp = (zb_zdo_mgmt_lqi_resp_t *) zb_buf_begin(param);

    TRACE_MSG(TRACE_ZDO1, ">>mgmt_lqi_resp buf = %d", (FMT__D, param));

    if (resp->status == ZB_ZDP_STATUS_SUCCESS)
    {
        zb_uint8_t nbrs = resp->neighbor_table_list_count;

        s_start_idx += nbrs;
        if (s_start_idx < resp->neighbor_table_entries)
        {
            TRACE_MSG(TRACE_ZDO2, "mgmt_lqi_resp: retrieved = %d, total = %d",
                      (FMT__D_D, nbrs, resp->neighbor_table_entries));
            zb_buf_get_out_delayed(send_mgmt_lqi_req);
        }
        else
        {
            TRACE_MSG(TRACE_ZDO2, "mgmt_lqi_resp: retrieved all entries - %d",
                      (FMT__D, resp->neighbor_table_entries));
            zb_buf_get_out_delayed(send_mgmt_leave_cmd3);
        }
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "mgmt_lqi_resp: request failure with status = %d", (FMT__D, resp->status));
    }

    zb_buf_free(param);

    TRACE_MSG(TRACE_ZDO1, "<<mgmt_lqi_resp", (FMT__0));
}


static void send_buffer_test_req(zb_uint8_t param)
{
    ZVUNUSED(param);
    zb_buf_get_out_delayed(send_buffer_test_req_delayed);
}


static void send_buffer_test_req_delayed(zb_uint8_t param)
{
    zb_buffer_test_req_param_t *req_param;

    TRACE_MSG(TRACE_APP1, ">>send_buffer_test_req_delayed: param = %d",
              (FMT__D, param));

    req_param = ZB_BUF_GET_PARAM(param, zb_buffer_test_req_param_t);
    BUFFER_TEST_REQ_SET_DEFAULT(req_param);

    req_param->dst_addr = s_dest_addr;
    TRACE_MSG(TRACE_APP1, "addr = 0x%x;", (FMT__H, req_param->dst_addr));

    zb_tp_buffer_test_request(param, buffer_test_request_cb);

    TRACE_MSG(TRACE_APP1, "<<send_buffer_test_req_delayed", (FMT__0));
}


static void buffer_test_request_cb(zb_uint8_t param)
{
    if (!param)
    {
        TRACE_MSG(TRACE_INFO1, "buffer_test_request_cb: status OK", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_INFO1, "buffer_test_request_cb: status FAILED", (FMT__0));
    }
}

/*! @} */

