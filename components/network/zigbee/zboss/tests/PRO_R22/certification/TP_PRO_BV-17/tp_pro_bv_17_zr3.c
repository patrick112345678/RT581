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
/* PURPOSE: ZR
*/

#define ZB_TEST_NAME TP_PRO_BV_17_ZR3
#define ZB_TRACE_FILE_ID 40929

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "tp_pro_bv_17_common.h"
#include "../common/zb_cert_test_globals.h"


#define START_RANDOM  0x11223344
#define START_RANDOM2 0x12345678

#define BAD_ADDR 0xbc2d


static zb_ieee_addr_t g_ieee_addr = {0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00};
/* zb_ieee_addr_t g_ieee_addr_ed2 = {0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; */
static zb_ieee_addr_t g_ieee_addr_c = IEEE_ADDR_C;
static zb_ieee_addr_t g_ieee_addr_ed2 = IEEE_ADDR_ED2;

MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("zdo_3_zr3");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    /* set ieee addr */
    zb_set_long_address(g_ieee_addr);

    /* join as a router */
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zr_role();

    MAC_ADD_INVISIBLE_SHORT(1);

    MAC_ADD_VISIBLE_LONG(g_ieee_addr_c);
    MAC_ADD_VISIBLE_LONG(g_ieee_addr_ed2);

    /* turn off security */
    /* zb_cert_test_set_security_level(0); */

    zb_set_max_children(1);

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


static zb_uint16_t addr_ass_cb(zb_ieee_addr_t ieee_addr)
{
    zb_uint16_t res = (zb_uint16_t)0xf00d;
    zb_address_ieee_ref_t ref;

    ZVUNUSED(ieee_addr);

    TRACE_MSG(TRACE_APS3, ">>addr_assignmnet_cb", (FMT__0));

    if (RET_OK == zb_address_by_short(res, ZB_FALSE, ZB_FALSE, &ref))
    {
        zb_address_delete(ref);
    }

    TRACE_MSG(TRACE_APS3, "<<addr_assignmnet_cb: res = 0x%x;", (FMT__H, res));
    return res;
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    TRACE_MSG(TRACE_ERROR, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    if (0 == status)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));

            zb_nwk_set_address_assignment_cb(addr_ass_cb);
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

