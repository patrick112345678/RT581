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
/* PURPOSE:
*/


#define ZB_TEST_NAME TP_R22_CCB_2469_GZC
#define ZB_TRACE_FILE_ID 63723

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"

enum test_step_e
{
    TS_TEST_BUF_REQ_FFFF,
    TS_TEST_BUF_REQ_FFFD,
    TS_TEST_BUF_REQ_FFFC,
    TEST_STEPS_COUNT
};

static zb_uint8_t g_step_idx;

static void test_buffer_request_delayed(zb_uint8_t do_next_ts);
static void test_buffer_request(zb_uint8_t param);

static const zb_ieee_addr_t g_ieee_addr_gzc = IEEE_ADDR_gZC;

MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("zdo_gzc");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    /* let's always be coordinator */
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zc_role();
    zb_set_nvram_erase_at_start(ZB_TRUE);

    /* set ieee addr */
    zb_set_long_address(g_ieee_addr_gzc);
    zb_set_pan_id(TEST_PAN_ID);

    /* accept only one child */
    zb_set_max_children(1);

#ifdef SECURITY_LEVEL
    zb_cert_test_set_security_level(SECURITY_LEVEL);
#endif

    if ( zboss_start() != RET_OK )
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

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    switch (sig)
    {
    case ZB_ZDO_SIGNAL_DEFAULT_START:
        TRACE_MSG(TRACE_APS1, "Device started, status %d", (FMT__D, status));
        if (status == 0)
        {
            ZB_CERT_HACKS().aps_mcast_addr_overridden = ZB_TRUE;
            ZB_SCHEDULE_ALARM(test_buffer_request_delayed, 0, TEST_GZC_STARTUP_DELAY);
        }
        break; /* ZB_ZDO_SIGNAL_DEFAULT_START */

    default:
        TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}

static void test_buffer_request_delayed(zb_uint8_t do_next_ts)
{
    TRACE_MSG(TRACE_APP1, ">>test_buffer_request_delayed", (FMT__0));

    if (do_next_ts)
    {
        g_step_idx++;
    }

    TRACE_MSG(TRACE_APP1, "test_buffer_request_delayed: step %d", (FMT__D, g_step_idx));

    if (g_step_idx < TEST_STEPS_COUNT)
    {
        switch (g_step_idx)
        {
        case TS_TEST_BUF_REQ_FFFF:
            ZB_CERT_HACKS().aps_mcast_nwk_dst_addr = ZB_NWK_BROADCAST_ALL_DEVICES;
            break;
        case TS_TEST_BUF_REQ_FFFD:
            ZB_CERT_HACKS().aps_mcast_nwk_dst_addr = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
            break;
        case TS_TEST_BUF_REQ_FFFC:
            ZB_CERT_HACKS().aps_mcast_nwk_dst_addr = ZB_NWK_BROADCAST_ROUTER_COORDINATOR;
            break;
        default:
            /* [Max]: CR: This is a bad coding style to make a decision in default branch.
             * Generally, default branch should be used for error handling */
            ZB_CERT_HACKS().aps_mcast_addr_overridden = ZB_FALSE;
            break;
        }

        if (zb_buf_get_out_delayed(test_buffer_request) != RET_OK)
        {
            TRACE_MSG(TRACE_ERROR, "test_buffer_request_delayed: zb_buf_get_out_delayed failed", (FMT__0));

            ZB_SCHEDULE_ALARM(test_buffer_request_delayed, 0, TEST_GZC_NEXT_TS_DELAY);
        }
        else
        {
            ZB_SCHEDULE_ALARM(test_buffer_request_delayed, 1, TEST_GZC_NEXT_TS_DELAY);
        }
    }

    TRACE_MSG(TRACE_APP1, "<<test_buffer_request_delayed", (FMT__0));
}

static void test_buffer_request(zb_uint8_t param)
{
    zb_buffer_test_req_param_t *req_param = NULL;

    TRACE_MSG(TRACE_APS1, ">>test_buffer_request", (FMT__0));

    req_param = ZB_BUF_GET_PARAM(param, zb_buffer_test_req_param_t);
    BUFFER_TEST_REQ_SET_DEFAULT(req_param);
    req_param->addr_mode = ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT;
    req_param->dst_addr = GROUP_ADDR;

    zb_tp_buffer_test_request(param, NULL);

    TRACE_MSG(TRACE_APS1, "<<test_buffer_request", (FMT__0));
}
