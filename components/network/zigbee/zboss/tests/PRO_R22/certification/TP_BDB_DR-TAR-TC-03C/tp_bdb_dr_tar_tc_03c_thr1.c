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
/* PURPOSE: DR-TAR-TC-03C: Reset via NWK layer Leave command: DUT: ZC (TH ZR1)
*/

#define ZB_TEST_NAME TP_BDB_DR_TAR_TC_03C_THR1
#define ZB_TRACE_FILE_ID 40716

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

#include "tp_bdb_dr_tar_tc_03c_common.h"
#include "../common/zb_cert_test_globals.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_CERTIFICATION_HACKS
#error ZB_CERTIFICATION_HACKS is not compiled!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_thr1 = IEEE_ADDR_THR1;
static zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;

static void send_leave_delayed(zb_uint8_t unused);
static void send_leave(zb_uint8_t param);

MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_thr1");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif


    zb_set_long_address(g_ieee_addr_thr1);

    zb_cert_test_set_common_channel_settings();
    zb_set_network_router_role((1l << TEST_CHANNEL));
    zb_set_nvram_erase_at_start(ZB_TRUE);

    ZB_CERT_HACKS().enable_leave_to_router_hack = 1;

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


static void send_leave_delayed(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    zb_buf_get_out_delayed(send_leave);
}


static void send_leave(zb_uint8_t param)
{
    zb_nlme_leave_request_t *req = ZB_BUF_GET_PARAM(param, zb_nlme_leave_request_t);

    TRACE_MSG(TRACE_ZDO1, ">>send_leave: buf_param = %d", (FMT__D, param));

    req->remove_children = 0;
    req->rejoin = 0;
    ZB_IEEE_ADDR_COPY(req->device_address, g_ieee_addr_dut);
    zb_nlme_leave_request(param);

    TRACE_MSG(TRACE_ZDO1, "<<send_leave", (FMT__0));
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

            ZB_SCHEDULE_ALARM(send_leave_delayed, 0, 5 * ZB_TIME_ONE_SECOND);
        }
        else
        {
            TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, status));
        }
        break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

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
