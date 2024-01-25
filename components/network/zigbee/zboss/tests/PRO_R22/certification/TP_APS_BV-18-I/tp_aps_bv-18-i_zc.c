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
/* PURPOSE: TP/APS/BV-18-I Group Management-Group Remove All
*/

#define ZB_TEST_NAME TP_APS_BV_18_I_ZC
#define ZB_TRACE_FILE_ID 40688

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"

static zb_ieee_addr_t g_ieee_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
static zb_uint8_t test_step = 0;

enum test_step_e
{
    STEP_ZC_SEND_DATA1,
    STEP_ZC_SEND_DATA2,
    STEP_ZC_SEND_DATA3,
    STEP_ZC_SEND_DATA4,
};

static void send_data(zb_uint8_t param);
static void buffer_test_cb(zb_uint8_t param);

MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("zdo_zc");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zc_role();
    zb_set_pan_id(TEST_PAN_ID);
    zb_set_long_address(g_ieee_addr);
    ZB_NIB_SET_USE_MULTICAST(ZB_FALSE);
    zb_set_max_children(1);
    /* zb_cert_test_set_security_level(0); */

    zb_set_nvram_erase_at_start(ZB_TRUE);
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

    TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    if (0 == status)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
            break;

        case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
            TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
            test_step_register(send_data, 0, TP_APS_BV_18_STEP_1_TIME_ZC);
            test_step_register(send_data, 0, TP_APS_BV_18_STEP_2_TIME_ZC);
            test_step_register(send_data, 0, TP_APS_BV_18_STEP_3_TIME_ZC);
            test_step_register(send_data, 0, TP_APS_BV_18_STEP_4_TIME_ZC);
            test_control_start(TEST_MODE, TP_APS_BV_18_STEP_1_DELAY_ZC);
            break;

        default:
            TRACE_MSG(TRACE_APP1, "Unknown signal %hd", (FMT__H, sig));
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


static void send_data(zb_uint8_t param)
{
    zb_buffer_test_req_param_t *req_param;

    if (param == 0U)
    {
        zb_buf_get_out_delayed(send_data);
        return;
    }

    TRACE_MSG(TRACE_APP1, ">> send_data, param %hd", (FMT__H, param));

    req_param = ZB_BUF_GET_PARAM(param, zb_buffer_test_req_param_t);

    BUFFER_TEST_REQ_SET_DEFAULT(req_param);
    req_param->src_ep = 0x01;

    switch (test_step)
    {
    case STEP_ZC_SEND_DATA1:
    case STEP_ZC_SEND_DATA3:
    {
        TRACE_MSG(TRACE_APP1, "send_data: send to group 0x0001", (FMT__0));
        req_param->dst_addr = 0x0001;
        req_param->addr_mode = ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT;
        break;
    }
    case STEP_ZC_SEND_DATA2:
    case STEP_ZC_SEND_DATA4:
    {
        TRACE_MSG(TRACE_APP1, "send_data: send to group 0x0002", (FMT__0));
        req_param->dst_addr = 0x0002;
        req_param->addr_mode = ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT;
        break;
    }
    default:
    {
        TRACE_MSG(TRACE_APP1, "send_data: ERROR invalid test step index", (FMT__0));
        ZB_ASSERT(ZB_FALSE);
        return;
    }
    }

    zb_tp_buffer_test_request(param, buffer_test_cb);

    test_step++;

    TRACE_MSG(TRACE_APP1, "<< send_data", (FMT__0));
}

static void buffer_test_cb(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> buffer_test_cb, param %hd", (FMT__H, param));

    if (param != 0U)
    {
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_APP1, "<< buffer_test_cb", (FMT__0));
}
