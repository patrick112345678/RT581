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
/* PURPOSE: TP/R21/BV-10 - DUT ZED
*/

#define ZB_TEST_NAME TP_R21_BV_10_DUTZED
#define ZB_TRACE_FILE_ID 40932

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"


static zb_ieee_addr_t g_ieee_addr = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static zb_ieee_addr_t g_ieee_addr1 = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("zdo_3_dutzed");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    /* set ieee addr */
    zb_set_long_address(g_ieee_addr);

    /* become an ED */
    zb_cert_test_set_common_channel_settings();
    zb_set_network_ed_role((1l << TEST_CHANNEL));
    zb_set_rx_on_when_idle(ZB_TRUE);

    MAC_ADD_VISIBLE_LONG(g_ieee_addr1); /* only ZR1 is visible */
    MAC_ADD_INVISIBLE_SHORT(0);   /* ignore beacons from ZC */

    zb_zdo_set_aps_unsecure_join(ZB_TRUE);

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


static void buffer_test_cb(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APS1, "buffer_test_cb %hd", (FMT__H, param));

    if (param == ZB_TP_BUFFER_TEST_OK)
    {
        TRACE_MSG(TRACE_APS1, "Test status OK", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_APS1, "Test status ERROR", (FMT__0));
    }
}


static void send_buffer_test_request(zb_uint8_t param)
{
    zb_bufid_t buf = zb_buf_get_out();

    ZVUNUSED(param);

    TRACE_MSG(TRACE_ERROR, "send_buffer_test_request>>", (FMT__0));

    if (buf)
    {
        zb_buffer_test_req_param_t *req_param;

        req_param = ZB_BUF_GET_PARAM(buf, zb_buffer_test_req_param_t);
        BUFFER_TEST_REQ_SET_DEFAULT(req_param);
        req_param->dst_addr = 0;

        TRACE_MSG(TRACE_ERROR, "send data to 0x%x", (FMT__H, req_param->dst_addr));

        zb_tp_buffer_test_request(buf, buffer_test_cb);
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "TEST FAILED: Could not get out buf!", (FMT__0));
    }

    TRACE_MSG(TRACE_ERROR, "send_buffer_test_request<<", (FMT__0));
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    switch (sig)
    {
    case ZB_ZDO_SIGNAL_DEFAULT_START:
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APS1, "Device started, status %d", (FMT__D, status));
        if (status == 0)
        {
            test_step_register(send_buffer_test_request, 0, TP_R21_BV_10_STEP_7_TIME_DUTZED);

            test_control_start(TEST_MODE, TP_R21_BV_10_STEP_7_DELAY_DUTZED);
        }
        break; /* ZB_ZDO_SIGNAL_DEFAULT_START */

    case ZB_COMMON_SIGNAL_CAN_SLEEP:
        TRACE_MSG(TRACE_APS1, "signal: ZB_COMMON_SIGNAL_CAN_SLEEP, status %d",
                  (FMT__D, status));
        if (status == 0)
        {
#ifdef ZB_USE_SLEEP
            zb_sleep_now();
#endif /* ZB_USE_SLEEP */
        }
        break; /* ZB_COMMON_SIGNAL_CAN_SLEEP */

    default:
        TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}
