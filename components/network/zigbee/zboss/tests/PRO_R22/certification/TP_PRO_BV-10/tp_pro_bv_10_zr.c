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
  DUT ZR
*/

#define ZB_TEST_NAME TP_PRO_BV_10_ZR
#define ZB_TRACE_FILE_ID 40041

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

//#define TEST_CHANNEL (1l << 24)

static void send_data(zb_uint8_t param);

#if (defined ZB_NS_BUILD) | (defined ZB_NSNG)
static zb_ieee_addr_t g_ieee_addr = {0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
#else
static zb_ieee_addr_t g_ieee_addr = {0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};
#endif

#ifndef ZC_EMBER
static zb_ieee_addr_t g_zc_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
#else
static zb_ieee_addr_t g_zc_addr = {0xee, 0x23, 0x07, 0x00, 0x00, 0xed, 0x21, 0x00};
#endif

static zb_uint8_t dev_num = 0;
static zb_uint8_t tr_count = 0;

MAIN()
{
    zb_ieee_addr_t addr;
    ARGV_UNUSED;
    (void)addr;

    /* Init device, load IB values from nvram or set it to default */
    {
        char str[100];
#if (defined ZB_NS_BUILD) | (defined ZB_NSNG)
        dev_num = atoi(ZB_ARGV[3]);
        sprintf(str, "zdo_%i_zr%i", (dev_num + 1), dev_num);
        g_ieee_addr[0] = dev_num;
        g_ieee_addr[4] = dev_num;
#else
#endif

        ZB_INIT(str);
    }

    /* set ieee addr */
    zb_set_long_address(g_ieee_addr);

    /* join as a router */
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zr_role();

    /* turn off security */
    /* zb_cert_test_set_security_level(0); */

#ifdef USE_INVISIBLE_MODE
    if (dev_num > 1)
    {
        MAC_ADD_INVISIBLE_SHORT(0);   /* ignore beacons from ZC for all except
                                   * ZR1. Can't implement it for other ZRs. */
        TRACE_MSG(TRACE_COMMON1, "add unvisible short 0", (FMT__0));
    }

    ZB_IEEE_ADDR_COPY(&addr, &g_ieee_addr);

    if (dev_num == 3)
    {
        zb_set_max_children(0);

        addr[0] = dev_num - 1;
        addr[4] = dev_num - 1;

        MAC_ADD_VISIBLE_LONG(addr);
        TRACE_MSG(TRACE_COMMON1, "add visible long " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(addr)));
    }
    else
    {
        zb_set_max_children(1);

        addr[0] = dev_num + 1;
        addr[4] = dev_num + 1;

        MAC_ADD_VISIBLE_LONG(addr);
        TRACE_MSG(TRACE_COMMON1, "add visible long " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(addr)));

        if (dev_num == 1)
        {
            MAC_ADD_VISIBLE_LONG(g_zc_addr);
            TRACE_MSG(TRACE_COMMON1, "add visible long " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(g_zc_addr)));
        }
        else
        {
            addr[0] = dev_num - 1;
            addr[4] = dev_num - 1;

            MAC_ADD_VISIBLE_LONG(addr);
            TRACE_MSG(TRACE_COMMON1, "add visible long " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(addr)));
        }
    }
#endif

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
    zb_bufid_t buf1 = NULL;
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);

    if (0 == status)
    {
        zb_zdo_app_signal_hdr_t *sg_p = NULL;
        zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        {
            TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));

            /* Send test buffer request to coordinator */
            if (dev_num == 3)
            {
                buf1 = zb_buf_get_out();
                zb_nwk_send_rrec(buf1, ZB_PIBCACHE_NETWORK_ADDRESS(), 0x0000);
                ZB_SCHEDULE_ALARM(send_data, param, 10 * ZB_TIME_ONE_SECOND);
                return;
            }
            else
            {
                zb_buf_free(param);
            }
            break;
        }
        }
    }
    else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
    {
        TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, status));
        zb_buf_free(param);
    }

    return;
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
        zb_bufid_t buf = zb_buf_get_out();
        ZB_SCHEDULE_CALLBACK(send_data, buf);
    }
}

static void send_data(zb_uint8_t param)
{
    zb_buffer_test_req_param_t *req_param;
    TRACE_MSG(TRACE_APS1, "send_data %hd", (FMT__H, param));
    req_param = ZB_BUF_GET_PARAM(param, zb_buffer_test_req_param_t);
    BUFFER_TEST_REQ_SET_DEFAULT(req_param);
    req_param->len = TEST_BUFFER_LEN;
    req_param->dst_addr = 0x0000; /* coordinator short address */
    req_param->src_ep = 0x01;
    zb_tp_buffer_test_request(param, buffer_test_cb);
}

