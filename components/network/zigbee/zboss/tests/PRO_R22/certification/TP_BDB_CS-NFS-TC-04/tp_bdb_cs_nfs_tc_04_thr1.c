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
/* PURPOSE: TH ZR1 - joining to network (without TCLK update)
*/

#define ZB_TEST_NAME TP_BDB_CS_NFS_TC_04_THR1
#define ZB_TRACE_FILE_ID 40896

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"
#include "zb_console_monitor.h"

#include "tp_bdb_cs_nfs_tc_04_common.h"
#include "../common/zb_cert_test_globals.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */


#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error define ZB_USE_NVRAM
#endif

static zb_ieee_addr_t g_ieee_addr_thr1 = IEEE_ADDR_THR1;

static void zb_association_permit(zb_uint8_t unused);
static void trigger_steering(zb_uint8_t unused);

static zb_uint8_t g_association_permit = 0;

MAIN()
{
    ARGV_UNUSED;

    char command_buffer[100], *command_ptr;
    char next_cmd[40];
    zb_bool_t res;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_thr1");
#if UART_CONTROL
    test_control_init();
#endif

    zb_set_long_address(g_ieee_addr_thr1);
    zb_set_network_router_role((1l << TEST_CHANNEL));

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

    zb_set_max_children(1);

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

static void zb_association_permit(zb_uint8_t unused)
{
    //! [zb_get_in_buf]
    zb_bufid_t buf = zb_buf_get(ZB_TRUE, 0);

    ZVUNUSED(unused);

#ifndef NCP_MODE_HOST
    ZB_PIBCACHE_ASSOCIATION_PERMIT() = g_association_permit;
    zb_nwk_update_beacon_payload(buf);
#else
    ZVUNUSED(buf);
    ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
#endif

    //! [zb_get_in_buf]
}

static void trigger_steering(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    //zb_association_permit(0);
    bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    switch (sig)
    {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        TRACE_MSG(TRACE_APS1, "Device started, status %d", (FMT__D, status));
        if (status == 0)
        {
            //zb_association_permit(0);
            g_association_permit = 1;
            ZB_SCHEDULE_ALARM(trigger_steering, 0, (MIN_COMMISSIONINIG_TIME_DELAY / 2));
        }
        else
        {
            ZB_SCHEDULE_ALARM(trigger_steering, 0, DO_RETRY_JOIN_DELAY);
        }
        break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        if (status == 0)
        {
            g_association_permit = 1;
            ZB_SCHEDULE_CALLBACK(trigger_steering, 0);
        }
        break; /* ZB_BDB_SIGNAL_DEVICE_REBOOT */

    default:
        TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}


/*! @} */
