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
/* PURPOSE: ZED
*/

#define ZB_TEST_NAME TP_R21_BV_09_DUTZED
#define ZB_TRACE_FILE_ID 40736

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "../common/zb_cert_test_globals.h"

//#define TEST_CHANNEL (1l << 24)

static zb_ieee_addr_t g_ieee_addr_zed = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static zb_ieee_addr_t g_ieee_addr1 = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};

#if 0
static zb_ieee_addr_t g_ieee_addr_c = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
#endif

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
    zb_set_long_address(g_ieee_addr_zed);

    /* become an ED */
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zed_role();
    zb_set_rx_on_when_idle(ZB_TRUE);

    MAC_ADD_VISIBLE_LONG(g_ieee_addr1); /* only ZR1 is visible */
    MAC_ADD_INVISIBLE_SHORT(0);   /* ignore beacons from ZC */

    zb_zdo_set_aps_unsecure_join(ZB_TRUE);
    zb_bdb_set_legacy_device_support(ZB_FALSE);

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

#if 0
static void send_request_key(zb_uint8_t param)
{
    zb_bufid_t buf = zb_buf_get_out();

    ZVUNUSED(param);

    TRACE_MSG(TRACE_ERROR, ">>send_request_key", (FMT__0));

    if (buf)
    {
        zb_apsme_request_key_req_t *req_param;

        req_param = ZB_BUF_GET_PARAM(buf, zb_apsme_request_key_req_t);

        ZB_IEEE_ADDR_COPY(req_param->dest_address, &g_ieee_addr_c);
        req_param->key_type = ZB_REQUEST_TC_LINK_KEY;
        ZB_MEMSET(req_param->partner_address, 0, sizeof(req_param->partner_address));

        zb_secur_apsme_request_key(buf);
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "TEST FAILED: Could not get out buf!", (FMT__0));
    }

    TRACE_MSG(TRACE_ERROR, "<<send_request_key", (FMT__0));
}
#endif


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

    zb_buf_free(param);
}


static void send_buffer_test_request(zb_uint8_t param)
{
    zb_bufid_t buf = zb_buf_get_out();
    ZVUNUSED(param);

    if (buf)
    {
        zb_buffer_test_req_param_t *req_param;

        TRACE_MSG(TRACE_ERROR, "send_data", (FMT__0));

        req_param = ZB_BUF_GET_PARAM(buf, zb_buffer_test_req_param_t);
        BUFFER_TEST_REQ_SET_DEFAULT(req_param);
        req_param->dst_addr = 0;

        TRACE_MSG(TRACE_ERROR, "send to 0x%x", (FMT__H, req_param->dst_addr));

        zb_tp_buffer_test_request(buf, buffer_test_cb);
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "TEST FAILED: Could not get buf!", (FMT__0));
    }
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    TRACE_MSG(TRACE_ERROR, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    if (0 == status)
    {
        static zb_bool_t is_first_start = ZB_TRUE;

        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));

            if (is_first_start)
            {
#if 0
                ZB_SCHEDULE_CALLBACK(send_request_key, 0);
#endif
                ZB_SCHEDULE_ALARM(send_buffer_test_request, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(5000));

                is_first_start = ZB_FALSE;
            }
            break;

        case ZB_COMMON_SIGNAL_CAN_SLEEP:
#ifdef ZB_USE_SLEEP
            zb_sleep_now();
#endif /* ZB_USE_SLEEP */
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

    zb_buf_free(param);
}

