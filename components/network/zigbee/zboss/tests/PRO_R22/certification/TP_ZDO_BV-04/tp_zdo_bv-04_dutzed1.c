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
/* PURPOSE: TP/ZDO/BV-04 ZED1
*/

#define ZB_TEST_NAME TP_ZDO_BV_04_ZED1
#define ZB_TRACE_FILE_ID 63913

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"


static zb_uint16_t    g_zc_short_addr = 0x0000;
static zb_ieee_addr_t g_zc_ieee_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
static zb_ieee_addr_t g_ieee_addr = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("zdo_2_zed");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    /* set ieee addr */
    zb_set_long_address(g_ieee_addr);

    /* turn off security */
    /* zb_cert_test_set_security_level(0); */

    /* become an ED */
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zed_role();
    zb_set_rx_on_when_idle(ZB_FALSE);

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


//! [zb_zdo_nwk_addr_req]
static void test_nwk_addr_req_cb(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ZDO1, "test_nwk_addr_req_cb: param = %hd;", (FMT__H, param));
    zb_buf_free(param);
}


static void test_nwk_addr_req(zb_uint8_t param)
{
    zb_bufid_t buf = zb_buf_get_out();
    zb_zdo_nwk_addr_req_param_t *req_param = NULL;

    TRACE_MSG(TRACE_ZDO1, "test_nwk_addr_req: req_type=%hd;", (FMT__H, param));
    if (!buf)
    {
        TRACE_MSG(TRACE_ERROR, "test_nwk_addr_req: error - unable to get data buffer", (FMT__0));
        return;
    }

    req_param = ZB_BUF_GET_PARAM(buf, zb_zdo_nwk_addr_req_param_t);
    req_param->dst_addr = 0xffff;
    ZB_IEEE_ADDR_COPY(req_param->ieee_addr, g_zc_ieee_addr);
    req_param->start_index = 0;
    req_param->request_type = param;
    zb_zdo_nwk_addr_req(buf, test_nwk_addr_req_cb);
}
//! [zb_zdo_nwk_addr_req]


//! [zb_zdo_ieee_addr_req]
static void test_ieee_addr_req_cb(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ZDO1, "test_ieee_addr_req_cb: param = %hd;", (FMT__H, param));
    zb_buf_free(param);
}


static void test_ieee_addr_req(zb_uint8_t param)
{
    zb_bufid_t buf = zb_buf_get_out();
    zb_zdo_ieee_addr_req_param_t *req_param = NULL;

    TRACE_MSG(TRACE_ZDO1, "test_ieee_addr_req: req_type=%hd;", (FMT__H, param));
    if (!buf)
    {
        TRACE_MSG(TRACE_ERROR, "test_ieee_addr_req: error - unable to get data buffer", (FMT__0));
        return;
    }

    req_param = ZB_BUF_GET_PARAM(buf, zb_zdo_ieee_addr_req_param_t);

    req_param->nwk_addr = g_zc_short_addr;
    req_param->dst_addr = req_param->nwk_addr;
    req_param->start_index = 0;
    req_param->request_type = param;
    zb_zdo_ieee_addr_req(buf, test_ieee_addr_req_cb);
}
//! [zb_zdo_ieee_addr_req]

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    if (0 == status)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));

            /* NWK_addr_req to DUT ZDO for IEEEAddr=DUT=ZC 64-bit IEEE address; RequestType=0x00; StartIndex=0x00; */
            test_step_register(test_nwk_addr_req, 0, TP_ZDO_BV_04_STEP_1_TIME_DUTZED1);
            /* gZED1 ZDO issues NWK_addr_req to the DUT ZDO IEEEAddr=DUT=ZC 64-bit IEEE address; RequestType=0x01; StartIndex=0x00; */
            test_step_register(test_nwk_addr_req, 1, TP_ZDO_BV_04_STEP_2_TIME_DUTZED1);
            /* gZED1 ZDO issues IEEE_addr_req to the DUT. NWKAddrOfInterest=0x0000 16-bit DUT ZC NWK address; RequestType=0x00; StartIndex=0x00; */
            test_step_register(test_ieee_addr_req, 0, TP_ZDO_BV_04_STEP_3_TIME_DUTZED1);
            /* gZED1 ZDO issues IEEE_addr_req to the DUT. NWKAddrOfInterest=0x0000 16-bit DUT ZC NWK address; RequestType=0x01; StartIndex=0x00; */
            test_step_register(test_ieee_addr_req, 1, TP_ZDO_BV_04_STEP_4_TIME_DUTZED1);

            test_control_start(TEST_MODE, TP_ZDO_BV_04_STEP_1_DELAY_DUTZED1);
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
