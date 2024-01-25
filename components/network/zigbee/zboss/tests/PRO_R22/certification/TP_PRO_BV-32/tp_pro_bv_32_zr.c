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


#define ZB_TEST_NAME TP_PRO_BV_32_ZR
#define ZB_TRACE_FILE_ID 40892

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zboss_api_internal.h"
#include "zb_console_monitor.h"

#ifdef ZB_BDB_MODE
#include "zb_bdb_internal.h"
#include "../common/zb_cert_test_globals.h"
#endif

#if !defined(ZB_USE_NVRAM)
#error "ZB_USE_NVRAM must be defined"
#endif

//#define TEST_CHANNEL (1l << 24)

static zb_ieee_addr_t g_ext_pan_id = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
/* For NS build first ieee addr byte should be unique */
static zb_ieee_addr_t g_ieee_addr = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};

MAIN()
{
    ARGV_UNUSED;

    char command_buffer[100], *command_ptr;
    char next_cmd[40];
    zb_bool_t res;
    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("zdo_2_zr");
#if UART_CONTROL
    test_control_init();
#endif

    /* set ieee addr */
    zb_set_long_address(g_ieee_addr);

    zb_set_use_extended_pan_id(g_ext_pan_id);

    zb_set_pan_id(0x1aaa);

    zb_zdo_set_aps_unsecure_join(ZB_TRUE);

#ifdef ZB_USE_NVRAM
    zb_cert_test_set_aps_use_nvram();
#endif

    /* join as a router */
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zr_role();
    zb_set_network_router_role_with_mode(zb_get_channel_mask(), ZB_COMMISSIONING_BDB);

    /* turn off security */
    /* zb_cert_test_set_security_level(0); */

    zb_set_max_children(0);

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


#ifdef ZB_BDB_MODE
ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_ieee_addr_t parent_addr;
    zb_uint16_t s_parent_addr;
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        {
            zb_ext_pan_id_t extended_pan_id;
            zb_ext_pan_id_t use_extended_pan_id;

            TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));

            zb_cert_test_get_parent_ieee_addr(parent_addr);
            s_parent_addr = zb_cert_test_get_parent_short_addr();

            zb_get_extended_pan_id(extended_pan_id);
            zb_get_use_extended_pan_id(use_extended_pan_id);

            TRACE_MSG(TRACE_APS1, "NIB Pan ID = " TRACE_FORMAT_64 " AIB Pan ID = " TRACE_FORMAT_64,
                      (FMT__A_A, TRACE_ARG_64(extended_pan_id), TRACE_ARG_64(use_extended_pan_id)));
        }

        /* FALLTHROUGH */
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_ERROR, "Device resumed %d", (FMT__D, status));
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
            break;

        case ZB_BDB_SIGNAL_STEERING:
            TRACE_MSG(TRACE_APP1, "Successfull steering", (FMT__0));
            break;

        default:
            TRACE_MSG(TRACE_APP1, "Unknown signal", (FMT__0));
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
#else
ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_ieee_addr_t parent_addr;
    zb_uint16_t s_parent_addr;
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);

    if (status == 0)
    {
        TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));

        zb_address_ieee_by_ref(parent_addr, ZG->nwk.handle.parent);
        zb_address_short_by_ref(&s_parent_addr, ZG->nwk.handle.parent);

        TRACE_MSG(TRACE_APS1, "NIB Pan ID = " TRACE_FORMAT_64 " AIB Pan ID = " TRACE_FORMAT_64,
                  (FMT__A_A, TRACE_ARG_64(ZB_NIB_EXT_PAN_ID()), TRACE_ARG_64(ZB_AIB().aps_use_extended_pan_id)));
    }
    else if (status == ZB_NWK_STATUS_ALREADY_PRESENT)
    {
        TRACE_MSG(TRACE_ERROR, "Device resumed %d", (FMT__D, status));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, status));
    }

    zb_buf_free(param);
}
#endif  /* ZB_BDB_MODE */

