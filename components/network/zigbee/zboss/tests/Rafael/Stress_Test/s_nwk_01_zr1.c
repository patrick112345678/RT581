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
/* PURPOSE:
*/


#define ZB_TEST_NAME S_NWK_01_ZR1

#define APS_FRAGMENTATION

#define ZB_TRACE_FILE_ID 18001
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "../common/zb_cert_test_globals.h"


#define PACKET_MAX_LENGTH 55

static zb_uint32_t current_len = 0, err_cnt = 0, ok_cnt = 0;
static zb_ieee_addr_t g_ieee_addr = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};

static zb_ieee_addr_t zed1_ieee_addr = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static zb_ieee_addr_t zed2_ieee_addr = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#define ZED1_SHORT_ADDR            0xAAAA
#define ZED2_SHORT_ADDR            0xBBBB

static void buffer_test_cb(zb_uint8_t param);
static void send_data_ZC(zb_uint8_t param);
static void send_data_ZED1(zb_uint8_t param);
static void send_data_ZED2(zb_uint8_t param);

static uint32_t runID = 0;

static void system_breath(zb_uint8_t param)
{
    zb_osif_led_toggle(2);
    ZB_SCHEDULE_ALARM_CANCEL(system_breath, 0);
    ZB_SCHEDULE_ALARM(system_breath, 0, ZB_TIME_ONE_SECOND);
}

MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("zdo_zr1");

    /* set ieee addr */
    zb_set_long_address(g_ieee_addr);

    /* join as a router */
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zr_role();
    zb_set_nvram_erase_at_start(ZB_TRUE);

    /* #ifdef ZB_SECURITY */
    /*   /\* turn off security *\/ */
    /*   zb_cert_test_set_security_level(0); */
    /* #endif */

    zb_set_max_children(2);

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



#ifdef ZB_PRO_ADDRESS_ASSIGNMENT_CB
static zb_uint16_t addr_ass_cb(zb_ieee_addr_t ieee_addr)
{
    zb_uint16_t res = (zb_uint16_t)~0;

    TRACE_MSG(TRACE_APS3, ">>addr_assignmnet_cb", (FMT__0));

    if (ZB_IEEE_ADDR_CMP(ieee_addr, zed1_ieee_addr))
    {
        res = ZED1_SHORT_ADDR;
    }
    else if (ZB_IEEE_ADDR_CMP(ieee_addr, zed2_ieee_addr))
    {
        res = ZED2_SHORT_ADDR;
    }


    TRACE_MSG(TRACE_APS3, "<<addr_assignmnet_cb: res = 0x%x;", (FMT__H, res));
    return res;
}
#endif

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    switch (sig)
    {
    case ZB_ZDO_SIGNAL_DEFAULT_START:
        TRACE_MSG(TRACE_APS1, "Device started, status %d", (FMT__D, status));
#ifdef ZB_PRO_ADDRESS_ASSIGNMENT_CB
        zb_nwk_set_address_assignment_cb(addr_ass_cb);
#endif
        test_step_register(send_data_ZED1, 0, ZB_TIME_ONE_SECOND);

        test_control_start(TEST_MODE, 10 * ZB_TIME_ONE_SECOND);

        ZB_SCHEDULE_ALARM(system_breath, 0, ZB_TIME_ONE_SECOND);
        break; /* ZB_ZDO_SIGNAL_DEFAULT_START */

    default:
        TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}

static void buffer_test_cb(zb_uint8_t param)
{
    zb_bufid_t buf = 0;
    zb_apsme_add_group_req_t *req_param = NULL;

    if (param == ZB_TP_BUFFER_TEST_OK)
    {
        ok_cnt++;
        zb_osif_led_off(0);
    }
    else
    {
        err_cnt++;
        zb_osif_led_on(0);
    }

    TRACE_MSG(TRACE_APP1, "status OK: %08X, status ERROR: %08X", (FMT__A_A, ok_cnt, err_cnt));

    if (runID == 1)
    {
        ZB_SCHEDULE_ALARM_CANCEL(send_data_ZED1, 0);
        ZB_SCHEDULE_ALARM(send_data_ZED1, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(500));
    }
    else if (runID == 2)
    {
        ZB_SCHEDULE_ALARM_CANCEL(send_data_ZED2, 0);
        ZB_SCHEDULE_ALARM(send_data_ZED2, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(500));
    }
    else if (runID == 3)
    {
        ZB_SCHEDULE_ALARM_CANCEL(send_data_ZC, 0);
        ZB_SCHEDULE_ALARM(send_data_ZC, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(500));
    }
}

static void send_data_ZC(zb_uint8_t param)
{
    zb_buffer_test_req_param_t *req_param;
    zb_bufid_t buf = zb_buf_get_out();
    ZVUNUSED(param);

    TRACE_MSG(TRACE_APP1, "send_data: %hd", (FMT__H, buf));
    req_param = ZB_BUF_GET_PARAM(buf, zb_buffer_test_req_param_t);
    BUFFER_TEST_REQ_SET_DEFAULT(req_param);

    if (current_len > PACKET_MAX_LENGTH)
    {
        current_len = 0;
    }

    req_param->len = current_len;
    current_len ++;

    zb_osif_led_toggle(3);

    runID = 1;

    zb_tp_buffer_test_request(buf, buffer_test_cb);
}

static void send_data_ZED1(zb_uint8_t param)
{
    zb_buffer_test_req_param_t *req_param;
    zb_bufid_t buf = zb_buf_get_out();
    ZVUNUSED(param);

    TRACE_MSG(TRACE_APP1, "send_data: %hd", (FMT__H, buf));
    req_param = ZB_BUF_GET_PARAM(buf, zb_buffer_test_req_param_t);
    BUFFER_TEST_REQ_SET_DEFAULT(req_param);

    req_param->dst_addr = ZED1_SHORT_ADDR;

    if (current_len > PACKET_MAX_LENGTH)
    {
        current_len = 0;
    }

    req_param->len = current_len;
    current_len ++;

    zb_osif_led_toggle(3);

    runID = 2;

    zb_tp_buffer_test_request(buf, buffer_test_cb);
}

static void send_data_ZED2(zb_uint8_t param)
{
    zb_buffer_test_req_param_t *req_param;
    zb_bufid_t buf = zb_buf_get_out();
    ZVUNUSED(param);

    TRACE_MSG(TRACE_APP1, "send_data: %hd", (FMT__H, buf));
    req_param = ZB_BUF_GET_PARAM(buf, zb_buffer_test_req_param_t);
    BUFFER_TEST_REQ_SET_DEFAULT(req_param);

    req_param->dst_addr = ZED2_SHORT_ADDR;

    if (current_len > PACKET_MAX_LENGTH)
    {
        current_len = 0;
    }

    req_param->len = current_len;
    current_len ++;

    zb_osif_led_toggle(3);

    runID = 3;

    zb_tp_buffer_test_request(buf, buffer_test_cb);
}

