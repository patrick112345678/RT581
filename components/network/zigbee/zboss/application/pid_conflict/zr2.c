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

/* PURPOSE: Sample application showing behavior of a ZR that handles
 * PAN ID conflict.
 */

#define ZB_TRACE_FILE_ID 40109

#include "zb_common.h"
#include "pid_conflict_common.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

static const zb_ieee_addr_t g_zr2_addr = IEEE_ADDR_ZR2;


MAIN()
{
    ARGV_UNUSED;

    /* Trace enable */
    ZB_SET_TRACE_ON();
    /* Traffic dump enable */
    ZB_SET_TRAF_DUMP_ON();

    /* Global ZBOSS initialization */
    ZB_INIT("zdo_zr2");

    /* Set up defaults for the commissioning */
    zb_set_long_address(g_zr2_addr);
    zb_set_network_router_role(CHANNEL_MASK);
    zb_set_max_children(NO_CHILDREN);
    zb_aib_tcpol_set_update_trust_center_link_keys_required(ZB_FALSE);

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


/* Callback to handle the stack events */
void zboss_signal_handler(zb_uint8_t param)
{
    zb_zdo_app_signal_hdr_t *sg_p = NULL;
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

    TRACE_MSG(TRACE_ERROR, ">>zb_zdo_startup_complete status %d",
              (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
            zb_enable_panid_conflict_resolution(ZB_TRUE);
            break;
        case ZB_NWK_SIGNAL_PANID_CONFLICT_DETECTED:
            TRACE_MSG(TRACE_APP1, "PID conflict detected", (FMT__0));
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
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d",
                  (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
    }

    /* Free the buffer if it is not used */
    if (param)
    {
        zb_buf_free(param);
    }
}
