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
/* PURPOSE: ZR
*/

#define ZB_TEST_NAME TP_PRO_BV_12_ZR
#define ZB_TRACE_FILE_ID 40662

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"


#ifndef ZB_PRO_STACK
#error This test is not applicable for 2007 stack. ZB_PRO_STACK should be defined to enable PRO.
#endif

#include "../nwk/nwk_internal.h"
#include "../common/zb_cert_test_globals.h"

//#define TEST_CHANNEL (1l << 24)

static zb_ieee_addr_t g_ieee_addr     = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
static zb_ieee_addr_t g_ieee_addr_zed = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static zb_uint16_t g_zed_short_addr;
static zb_uint16_t g_test_completed = 0;


static void test_get_peer_addr_resp(zb_uint8_t param);
static void test_send_nwk_addr_req_wrap(zb_uint8_t param);
static void test_send_nwk_addr_req(zb_uint8_t param);


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("zdo_2_zr");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    zb_set_long_address(g_ieee_addr);
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zr_role();

    /* turn off security */
    /* zb_cert_test_set_security_level(0); */

    zb_set_max_children(0);

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

/*
 * Must use Test Profile 3 to send device announcement
 * MA: Changed
 */

/*
 * This Device Announse must be sent before any Link Status command, otherwise Link Status will
 * cause Address Conflict
 */
static void pib_set_callback(zb_uint8_t param)
{
    TRACE_MSG(TRACE_INFO3, "pib_set_callback: send announce with new address", (FMT__0));
    g_test_completed = 1;
    zb_tp_device_announce(param);
}


static void change_address(zb_uint8_t param)
{
    zb_bufid_t buf = zb_buf_get(ZB_TRUE, 0);

    ZVUNUSED(param);

    TRACE_MSG(TRACE_INFO3, ">>change_address", (FMT__0));

    if (buf)
    {
        zb_cert_test_set_network_addr(g_zed_short_addr);

        zb_nwk_pib_set(buf,
                       ZB_PIB_ATTRIBUTE_SHORT_ADDRESS,
                       &g_zed_short_addr,
                       2,
                       pib_set_callback);
    }
    else
    {
        TRACE_MSG(TRACE_INFO3, "TEST FAILED: Could not get in buf!", (FMT__0));
    }

    TRACE_MSG(TRACE_INFO3, "<<change_address", (FMT__0));
}


static void test_get_peer_addr_resp(zb_uint8_t param)
{
    zb_zdo_nwk_addr_resp_head_t *resp = (zb_zdo_nwk_addr_resp_head_t *) zb_buf_begin(param);

    TRACE_MSG(TRACE_ZDO1, ">>test_get_peer_addr_resp, param = %d, status = %d",
              (FMT__D_D, param, resp->status));

    if (resp->status == ZB_ZDP_STATUS_SUCCESS)
    {
        ZB_LETOH16(&g_zed_short_addr, &resp->nwk_addr);
    }
    zb_buf_free(param);

    TRACE_MSG(TRACE_ZDO1, "<<test_get_peer_addr_resp", (FMT__0));
}


static void test_send_nwk_addr_req_wrap(zb_uint8_t param)
{
    ZVUNUSED(param);
    zb_buf_get_out_delayed(test_send_nwk_addr_req);
}


static void test_send_nwk_addr_req(zb_uint8_t param)
{
    zb_zdo_nwk_addr_req_param_t *req_param = ZB_BUF_GET_PARAM(param, zb_zdo_nwk_addr_req_param_t);
    zb_callback_t cb;

    TRACE_MSG(TRACE_ZDO1, ">>test_send_secur_nwk_addr_req, param = %h", (FMT__H, param));

    req_param->dst_addr = 0x0000;
    req_param->start_index = 0;
    req_param->request_type = ZB_ZDO_SINGLE_DEVICE_RESP;
    ZB_IEEE_ADDR_COPY(req_param->ieee_addr, g_ieee_addr_zed);
    cb = test_get_peer_addr_resp;
    zb_zdo_nwk_addr_req(param, cb);

    TRACE_MSG(TRACE_ZDO1, "<<test_send_secur_nwk_addr_req", (FMT__0));
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

            /* prevent second call of this code after ZR resolves it's address conflict */
            if (!g_test_completed)
            {
                /* Ask ZC about ZED short address before switching own address to ZED address */
                ZB_SCHEDULE_ALARM(test_send_nwk_addr_req_wrap, 0, 20 * ZB_TIME_ONE_SECOND);
                ZB_SCHEDULE_ALARM(change_address, 0, 30 * ZB_TIME_ONE_SECOND);
            }
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
        TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d", (FMT__D, status));
    }

    zb_buf_free(param);
}

