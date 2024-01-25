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

/*
  gZR
*/

#define ZB_TEST_NAME TP_PRO_BV_09_ZR4
#define ZB_TRACE_FILE_ID 40701

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "../common/zb_cert_test_globals.h"

#ifndef ZB_PRO_STACK
#error This test is not applicable for 2007 stack. ZB_PRO_STACK should be defined to enable PRO.
#endif


/* Size of test buffer */
#define TEST_BUFFER_LEN                0x0010

#define USE_INVISIBLE_MODE

#ifdef ZB_EMBER_TESTS
#define ZC_EMBER
#endif

static void send_data(zb_uint8_t param);

static zb_ieee_addr_t g_ieee_addr = {0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00};
static zb_ieee_addr_t g_ieee_up = {0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00};

static zb_uint8_t tr_count = 0;

//#define TEST_CHANNEL (1l << 24)

MAIN()
{
    zb_ieee_addr_t addr;
    ARGV_UNUSED;
    (void)addr;

    ZB_INIT("zdo_5_zr4");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    /* set ieee addr */
    zb_set_long_address(g_ieee_addr);

    /* join as a router */
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zr_role();
    zb_aib_tcpol_set_update_trust_center_link_keys_required(ZB_FALSE);

    /* turn off security */
    /* zb_cert_test_set_security_level(0); */

    MAC_ADD_INVISIBLE_SHORT(0);   /* ignore beacons from ZC for all except
                                 * ZR1. Can't implement it for other ZRs. */
    MAC_ADD_VISIBLE_LONG(g_ieee_up);

    zb_set_max_children(0);

    zb_set_nvram_erase_at_start(ZB_TRUE);

    if (zboss_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "#####zboss_start failed", (FMT__0));
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
    zb_zdo_app_signal_hdr_t *sg_p = NULL;
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

    TRACE_MSG(TRACE_APP1, "zboss_signal_handler: status %hd signal %hd",
              (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
            ZB_SCHEDULE_ALARM(send_data, 0, 35 * ZB_TIME_ONE_SECOND);
            TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
            break;

        default:
            break;
        }
    }
    else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
    {
        TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d",
                  (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
    }

    if (param)
    {
        zb_buf_free(param);
    }
}

static void buffer_test_cb(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APS1, "buffer_test_cb %hd", (FMT__H, param));

    if (param == ZB_TP_BUFFER_TEST_OK)
    {
        TRACE_MSG(TRACE_APS1, "status OK", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_APS1, "status ERROR", (FMT__0));
    }

    ++tr_count;

    if (tr_count != 2)
    {
        ZB_SCHEDULE_ALARM(send_data, 0, 10 * ZB_TIME_ONE_SECOND);
    }
}

static void send_data(zb_uint8_t param)
{
    zb_bufid_t  buf = zb_buf_get_out();
    zb_buffer_test_req_param_t *req_param;

    ZVUNUSED(param);

    TRACE_MSG(TRACE_APS1, "send_data %hd", (FMT__H, buf));
    req_param = ZB_BUF_GET_PARAM(buf, zb_buffer_test_req_param_t);
    BUFFER_TEST_REQ_SET_DEFAULT(req_param);
    req_param->len = TEST_BUFFER_LEN;
    req_param->dst_addr = 0x0000; /* coordinator short address */
    req_param->src_ep = 0x01;
    zb_tp_buffer_test_request(buf, buffer_test_cb);
}
