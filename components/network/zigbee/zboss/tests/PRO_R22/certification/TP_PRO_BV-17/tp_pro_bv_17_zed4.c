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
/* PURPOSE: ZED2
*/

#define ZB_TEST_NAME TP_PRO_BV_17_ZED4
#define ZB_TRACE_FILE_ID 40926

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "tp_pro_bv_17_common.h"
#include "../common/zb_cert_test_globals.h"

static zb_ieee_addr_t g_ieee_addr = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("zdo_5_zed4");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    /* set ieee addr */
    zb_set_long_address(g_ieee_addr);

    /* turn off security */
    /* zb_cert_test_set_security_level(0); */

    //MAC_ADD_VISIBLE_LONG(g_zr_ieee_addr);
    MAC_ADD_INVISIBLE_SHORT(0);
    MAC_ADD_INVISIBLE_SHORT(1);   /* ignore beacons from ZR1 */

    /* become an ED */
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zed_role();

    zb_set_rx_on_when_idle(ZB_TRUE);

    /* configure poll timer */
    //ZDO_CTX().conf_attr.nwk_indirect_poll_rate = 2*ZB_TIME_ONE_SECOND;

    /* zb_zdo_pim_set_long_poll_interval(500); */

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

    if (0 == status)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));
            break;

        case ZB_COMMON_SIGNAL_CAN_SLEEP:
#ifdef ZB_USE_SLEEP
            zb_sleep_now();
#endif /* ZB_USE_SLEEP */
            break;

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

    zb_buf_free(param);
}

