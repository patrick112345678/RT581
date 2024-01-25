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
/* PURPOSE: TP/APS/BV-16-I Group Management-Group Addition (FULL)
*/

#define ZB_TEST_NAME TP_APS_BV_16_I_ZR
#define ZB_TRACE_FILE_ID 40505

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"

static zb_ieee_addr_t g_ieee_addr = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};

MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("zdo_zr1");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    zb_set_long_address(g_ieee_addr);
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zr_role();
    zb_set_max_children(0);

    ZB_CERT_HACKS().allow_entry_for_unregistered_ep = 1;
    /* zb_cert_test_set_security_level(0); */

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

static void dummy_handler(zb_uint8_t param)
{
    static zb_uint8_t count = 0;

    if (++count == ZB_APS_GROUP_TABLE_SIZE)
    {
        zb_buf_free(param);
    }
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_apsme_add_group_req_t *req_param;
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);
    zb_uint16_t i = 0;

    TRACE_MSG(TRACE_ERROR, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    if (0 == status)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        {
            TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));

            req_param = NULL;

            for (i = 0; i < ZB_APS_GROUP_TABLE_SIZE; ++i)
            {
                req_param = ZB_BUF_GET_PARAM(param, zb_apsme_add_group_req_t);

                /* Need to disable ZB_ENABLE_ZCL to be able add entry in group table with nonregistered endpoint id */
                req_param->endpoint = ZB_TEST_PROFILE_EP;
                req_param->group_address = i + 1;

                /* Register dummy callback just to do not free the buffer after zb_apsme_add_group_request(). */
                req_param->confirm_cb = dummy_handler;

                zb_apsme_add_group_request(param);

                TRACE_MSG(TRACE_APS1, "zb_apsme_add_group_request: group_addr = %hd;", (FMT__H, req_param->group_address));
            }
            param = 0;

            break;
        }

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

    if (param)
    {
        zb_buf_free(param);
    }
}
