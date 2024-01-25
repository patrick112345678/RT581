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


#define ZB_TEST_NAME TP_R21_BV_12_DUTZED
#define ZB_TRACE_FILE_ID 40529

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_console_monitor.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"


/*! \addtogroup ZB_TESTS */
/*! @{ */

static const zb_ieee_addr_t g_ieee_addr_dutzed = IEEE_ADDR_DUT_ZED;

static void test_leave_itself(zb_uint8_t param);

MAIN()
{
    zb_ext_pan_id_t use_extended_pan_id = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    ARGV_UNUSED;

    char command_buffer[100], *command_ptr;
    char next_cmd[40];
    zb_bool_t res;
    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("zdo_4_dutzed");
#if UART_CONTROL
    test_control_init();
#endif

    zb_set_long_address(g_ieee_addr_dutzed);
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zr_role();
    zb_set_use_extended_pan_id(use_extended_pan_id);
    zb_zdo_set_aps_unsecure_join(ZB_TRUE);
    zb_bdb_set_legacy_device_support(ZB_TRUE);

#ifdef SECURITY_LEVEL
    zb_cert_test_set_security_level(SECURITY_LEVEL);
#endif

#ifdef ZB_USE_NVRAM
    zb_cert_test_set_aps_use_nvram();
#endif

    TRACE_MSG(TRACE_APP1, "Send 'erase' for flash erase or just press enter to be continued \n", (FMT__0));
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_handler);
    zb_console_monitor_get_cmd((zb_uint8_t *)command_buffer, sizeof(command_buffer));
    command_ptr = (char *)(&command_buffer);
    res = parse_command_token(&command_ptr, next_cmd, sizeof(next_cmd));
    if (strcmp(next_cmd, "erase") == 0)
    {
        zb_set_nvram_erase_at_start(ZB_TRUE);
    }
    else
    {
        zb_set_nvram_erase_at_start(ZB_FALSE);
    }
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);

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

    if (0 == status)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));
            ZB_SCHEDULE_ALARM(test_leave_itself, 0, 30 * ZB_TIME_ONE_SECOND);
            break;

#ifdef ZB_USE_SLEEP
        case ZB_COMMON_SIGNAL_CAN_SLEEP:
            zb_sleep_now();
            break;
#endif /* ZB_USE_SLEEP */

        default:
            TRACE_MSG(TRACE_ERROR, "Unknown signal %hd", (FMT__H, sig));
            break;
        }
    }
    else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
    {
        TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "zboss_signal_handler: status %hd signal %hd",
                  (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));
    }

    zb_buf_free(param);
}


static void test_leave_itself(zb_uint8_t param)
{
    zb_bufid_t buf = zb_buf_get_out();

    ZVUNUSED(param);

    TRACE_MSG(TRACE_ERROR, ">>test_leave_itself", (FMT__0));

    if (buf)
    {
        zb_nlme_leave_request_t *req;
        req = zb_buf_get_tail(buf, sizeof(zb_nlme_leave_request_t));

        zb_get_long_address(req->device_address);
        req->remove_children = ZB_FALSE;
        req->rejoin = ZB_FALSE;

        zb_nlme_leave_request(buf);
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "TEST FAILED: Could not get out buf!", (FMT__0));
    }

    TRACE_MSG(TRACE_ERROR, "<<test_leave_itself", (FMT__0));
}

/*! @} */

