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
/* PURPOSE: DUT ZR (answering on discovery requests)
*/


#define ZB_TEST_NAME TP_BDB_FB_PRE_TC_02_DUT_R
#define ZB_TRACE_FILE_ID 40549

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_zcl.h"
#include "zb_bdb_internal.h"
#include "zb_zcl.h"

#include "tp_bdb_fb_pre_tc_02_common.h"
#include "../common/zb_cert_test_globals.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static void device_annce_cb(zb_zdo_device_annce_t *da);
static void remove_child(zb_uint8_t param);
static void remove_child_delayed(zb_uint8_t unused);

static zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;
static zb_ieee_addr_t g_ieee_addr_the1 = IEEE_ADDR_THE1;

static zb_uint8_t g_nwk_key[16] = {0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                                  };

static zb_uint16_t TEST_PAN_ID = 0x1AAA;

MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_dut");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif


    zb_set_pan_id(TEST_PAN_ID);
    zb_set_long_address(g_ieee_addr_dut);
    zb_aib_set_trust_center_address(g_unknown_ieee_addr);

    zb_cert_test_set_common_channel_settings();
    zb_set_network_router_role((1l << TEST_CHANNEL));
    zb_set_nvram_erase_at_start(ZB_TRUE);

    zb_set_max_children(3);
    zb_secur_setup_nwk_key(g_nwk_key, 0);

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

static void device_annce_cb(zb_zdo_device_annce_t *da)
{
    TRACE_MSG(TRACE_ZDO1, ">>dev_annce_cb, ieee = " TRACE_FORMAT_64 " addr = 0x%x",
              (FMT__A_H, TRACE_ARG_64(da->ieee_addr), da->nwk_addr));

    if (ZB_IEEE_ADDR_CMP(g_ieee_addr_the1, da->ieee_addr) == ZB_TRUE)
    {
        TRACE_MSG(TRACE_ZDO2, "dev_annce_cb: TH ZED has joined to network!", (FMT__0));
        ZB_SCHEDULE_ALARM(remove_child_delayed, 0, DUT_REMOVES_THE1_DELAY);
    }

    TRACE_MSG(TRACE_ZDO1, "<<dev_annce_cb", (FMT__0));
}

static void remove_child(zb_uint8_t param)
{
    zb_nlme_leave_request_t *req = zb_buf_get_tail(param, sizeof(zb_nlme_leave_request_t));

    TRACE_MSG(TRACE_ZDO1, ">>remove_child: buf_param = %d", (FMT__D, param));

    ZB_IEEE_ADDR_COPY(req->device_address, g_ieee_addr_the1);
    req->remove_children = 0;
    req->rejoin = 0;
    zb_nlme_leave_request(param);

    TRACE_MSG(TRACE_ZDO1, "<<remove_child", (FMT__0));
}

static void remove_child_delayed(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    zb_buf_get_out_delayed(remove_child);
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    switch (sig)
    {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        if (status == 0)
        {
            TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));

            zb_zdo_register_device_annce_cb(device_annce_cb);
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        }
        else
        {
            TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, status));
        }
        break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_STEERING:
        if (status == 0)
        {
            TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_STEERING, status OK", (FMT__0));
        }
        else
        {
            TRACE_MSG(TRACE_ERROR, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
        }
        break; /* ZB_BDB_SIGNAL_STEERING */

    default:
        if (status == 0)
        {
            TRACE_MSG(TRACE_APS1, "Unknown signal, status OK", (FMT__0));
        }
        else
        {
            TRACE_MSG(TRACE_ERROR, "Unknown signal, status %d", (FMT__D, status));
        }
        break;
    }

    zb_buf_free(param);
}

/*! @} */
