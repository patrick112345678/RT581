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
/* PURPOSE: 11.26 TP/SEC/BV-26-I Security NWK Key Switch (ZR Broadcast)
Objective: DUT as ZR receives a new NWK Key via broadcast and switches.
*/

#define ZB_TEST_NAME TP_SEC_BV_26_I_ZR
#define ZB_TRACE_FILE_ID 40517
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

#ifndef ZB_CERTIFICATION_HACKS
#error Define ZB_CERTIFICATION_HACKS
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static const zb_ieee_addr_t g_ieee_addr_c = IEEE_ADDR_C;
static const zb_ieee_addr_t g_ieee_addr_r1 = IEEE_ADDR_R1;
static const zb_ieee_addr_t g_ieee_addr_ed1 = IEEE_ADDR_ED1;

MAIN()
{
    ARGV_UNUSED;

    ZB_SET_TRAF_DUMP_ON();
    ZB_INIT("zdo_2_zr");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif


    zb_set_long_address(g_ieee_addr_r1);
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zr_role();

    MAC_ADD_VISIBLE_LONG((zb_uint8_t *) g_ieee_addr_ed1);
    MAC_ADD_VISIBLE_LONG((zb_uint8_t *) g_ieee_addr_c);

    zb_set_max_children(1);
    zb_zdo_set_aps_unsecure_join(INSECURE_JOIN_ZR1);

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
    zb_zdo_app_signal_hdr_t *sg_p = NULL;
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

    TRACE_MSG(TRACE_APP1, "zboss_signal_handler: status %hd signal %hd",
              (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));

    switch (sig)
    {
    case ZB_ZDO_SIGNAL_DEFAULT_START:
        if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
        {
            TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
        }
        else
        {
            TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d",
                      (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
        }
        break; /* ZB_ZDO_SIGNAL_DEFAULT_START */

    default:
        if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
        {
            TRACE_MSG(TRACE_APS1, "zboss_signal_handler: status OK, status %d",
                      (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
        }
        else
        {
            TRACE_MSG(TRACE_ERROR, "zboss_signal_handler: status FAILED, status %d",
                      (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
        }
        break;
    }

    zb_buf_free(param);
}
