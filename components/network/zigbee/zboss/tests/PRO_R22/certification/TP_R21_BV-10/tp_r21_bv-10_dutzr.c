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
/* PURPOSE: TP/R21/BV-10 - DUT ZR
*/


#define ZB_TEST_NAME TP_R21_BV_10_DUTZR
#define ZB_TRACE_FILE_ID 40933

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};

static void test_set_permit_join(zb_uint8_t param, zb_uint16_t permit_join_duration);

static void test_open_network(zb_uint8_t param)
{
    ZB_SCHEDULE_CALLBACK2(test_set_permit_join, param, 60);
}

static void test_set_permit_join(zb_uint8_t param, zb_uint16_t permit_join_duration)
{
    TRACE_MSG(TRACE_APS1, ">> test_close_permit_join param %hd", (FMT__H, param));

    if (!param)
    {
        zb_buf_get_out_delayed_ext(test_set_permit_join, permit_join_duration, 0);
    }
    else
    {
        zb_nlme_permit_joining_request_t *req;

        req = zb_buf_get_tail(param, sizeof(zb_nlme_permit_joining_request_t));
        ZB_BZERO(req, sizeof(zb_nlme_permit_joining_request_t));
        req->permit_duration = (permit_join_duration & 0xFF); /* Safe downcast */

        zb_nlme_permit_joining_request(param);
    }

    TRACE_MSG(TRACE_APS1, "<< test_close_permit_join", (FMT__0));
}


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("zdo_2_dutzr");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    zb_set_long_address(g_ieee_addr);
    zb_cert_test_set_common_channel_settings();
    zb_set_network_router_role((1l << TEST_CHANNEL));
    zb_set_max_children(1);

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
            ZB_SCHEDULE_ALARM(test_open_network, 0, ZB_TIME_ONE_SECOND);
        }
        break; /* ZB_ZDO_SIGNAL_DEFAULT_START */
    default:
        TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}

/*! @} */
