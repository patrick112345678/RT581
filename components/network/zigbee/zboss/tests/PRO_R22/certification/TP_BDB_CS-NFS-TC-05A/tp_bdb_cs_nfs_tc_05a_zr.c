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

#define ZB_TEST_NAME TP_BDB_CS_NFS_TC_05A_ZR1
#define ZB_TRACE_FILE_ID 40566

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"

#include "../common/zb_cert_test_globals.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr = {0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};

static zb_uint8_t g_ic1[16 + 2] = {0x83, 0xFE, 0xD3, 0x40, 0x7A, 0x93, 0x97, 0x23,
                                   0xA5, 0xC6, 0x39, 0xB2, 0x69, 0x16, 0xD5, 0x05,
                                   /* CRC */ 0xC3, 0xB5
                                  };

MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_2_zr");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif


    zb_set_long_address(g_ieee_addr);
    zb_set_network_router_role((1l << TEST_CHANNEL));
    zb_set_nvram_erase_at_start(ZB_TRUE);

    zb_zdo_set_aps_unsecure_join(ZB_TRUE);

    zb_set_max_children(0);

    zb_set_installcode_policy(ZB_TRUE);
    zb_secur_ic_set(ZB_IC_TYPE_128, g_ic1);

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
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APS1, "Device started, status %d", (FMT__D, status));
        if (status == 0)
        {
            /*  */
        }
        break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    default:
        TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}

/*! @} */
