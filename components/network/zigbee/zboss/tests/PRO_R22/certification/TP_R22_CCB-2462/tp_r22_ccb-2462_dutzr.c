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


#define ZB_TEST_NAME TP_R22_CCB_2462_DUTZR
#define ZB_TRACE_FILE_ID 40077

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"

static const zb_ieee_addr_t g_ieee_addr_dutzr = IEEE_ADDR_DUT_ZR;

static zb_uint8_t g_is_first_start = ZB_TRUE;
static zb_uint8_t g_step_idx;

static void test_send_dev_annce_delayed(zb_uint8_t do_next_ts);

MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("zdo_dutzr");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    /* set ieee addr */
    zb_set_long_address(g_ieee_addr_dutzr);

    /* join as a router */
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zr_role();

    zb_set_max_children(2);
    zb_set_nvram_erase_at_start(ZB_TRUE);

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
            if (g_is_first_start)
            {
                g_is_first_start = ZB_FALSE;

                ZB_SCHEDULE_ALARM(test_send_dev_annce_delayed, 0, TEST_DUTZR_STARTUP_DELAY);
            }
        }
        break; /* ZB_ZDO_SIGNAL_DEFAULT_START */

    default:
        TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}

static void test_send_dev_annce_delayed(zb_uint8_t do_next_ts)
{
    if (do_next_ts)
    {
        g_step_idx++;
    }

    if (g_step_idx < TEST_DUTZR_MAX_TS)
    {
        TRACE_MSG(TRACE_APP1, "test_send_dev_annce_delayed: step %d", (FMT__D, g_step_idx));

        if (zb_buf_get_out_delayed(zdo_send_device_annce) != RET_OK)
        {
            TRACE_MSG(TRACE_ERROR, "test_send_dev_annce_delayed: zb_buf_get_out_delayed failed", (FMT__0));

            ZB_SCHEDULE_ALARM(test_send_dev_annce_delayed, 0, TEST_DUTZR_NEXT_TS_DELAY);
        }
        else
        {
            ZB_SCHEDULE_ALARM(test_send_dev_annce_delayed, 1, TEST_DUTZR_NEXT_TS_DELAY);
        }
    }
}
