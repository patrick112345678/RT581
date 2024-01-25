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
/* PURPOSE: TH-ZC
*/

#define ZB_TEST_NAME GPS_DTCLK_JOINING_IC_BASED_JOINING_TH_GPP

#define ZB_TRACE_FILE_ID 63499
#include "zboss_api.h"
#include "test_config.h"

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_zcl.h"
#include "zb_secur_api.h"
#include "zb_ha.h"

#ifdef ZB_ENABLE_HA

/*============================================================================*/
/*                             DECLARATIONS                                   */
/*============================================================================*/

static zb_ieee_addr_t g_th_zc_addr = TH_ZC_IEEE_ADDR;
static zb_ieee_addr_t g_dut_gps_ieee_addr = DUT_GPS_IEEE_ADDR;
static zb_uint8_t g_key_nwk[] = NWK_KEY;
static zb_uint8_t g_ic[16 + 2] = SETUP_IC;
/* Derived key is 66 b6 90 09 81 e1 ee 3c a4 20 6b 6b 86 1c 02 bb (normal). Add
 * it to Wireshark. */

/*============================================================================*/
/*                            TEST IMPLEMENTATION                             */
/*============================================================================*/

/* BLANK */

/*============================================================================*/
/*                          STARTUP                                           */
/*============================================================================*/

MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("th_zc");

    zb_set_long_address(g_th_zc_addr);
    zb_set_network_coordinator_role(1l << TEST_CHANNEL);
    //zb_set_max_children(5);

    /* Setup predefined nwk key - to easily decrypt ZB sniffer logs which does not
     * contain keys exchange. By default nwk key is randomly generated. */
    zb_secur_setup_nwk_key((zb_uint8_t *) g_key_nwk, 0);

    /* Start ZBOSS main loop. */
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
    zb_zdo_app_event_t *ev_p = NULL;
    zb_zdo_app_signal_t sig = zb_get_app_event(param, &ev_p);

    if (ZB_GET_APP_EVENT_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
            /* Turn off link key exchange if legacy device support (<ZB3.0) is neeeded */
            zb_bdb_set_legacy_device_support(1);
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
            zb_secur_ic_add(g_dut_gps_ieee_addr, g_ic);
            break;

        case ZB_BDB_SIGNAL_STEERING:
            TRACE_MSG(TRACE_APP1, "Successfull steering", (FMT__0));
            break;

        default:
            TRACE_MSG(TRACE_APP1, "Unknown signal", (FMT__0));
        }
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, ZB_GET_APP_EVENT_STATUS(param)));
    }

    if (param)
    {
        zb_free_buf(ZB_BUF_FROM_REF(param));
    }
}

#else // defined ZB_ENABLE_HA

#include <stdio.h>
MAIN()
{
    ARGV_UNUSED;

    printf("HA profile should be enabled in zb_config.h\n");

    MAIN_RETURN(1);
}

#endif
