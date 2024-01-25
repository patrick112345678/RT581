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
/* PURPOSE: Parent loss test - ZR test harness
*/


#define ZB_TRACE_FILE_ID 63796
#include "zboss_api.h"
#include "pl_common.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

zb_ieee_addr_t g_zr1_addr = PL_ZR1_IEEE_ADDR;

static void pl_th_zr_init(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    zb_set_max_children(1);
    zb_set_long_address(g_zr1_addr);
    zb_set_network_router_role_legacy(TEST_CHANNEL_MASK);
}


MAIN()
{
    ARGV_UNUSED;

    ZB_SET_TRACE_ON();
    ZB_SET_TRAF_DUMP_ON();

    ZB_INIT("pl_th_zr");

    pl_th_zr_init(0);

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


static void pl_send_permit_joining(zb_uint8_t param)
{
    zb_zdo_mgmt_permit_joining_req_param_t *req =
        ZB_GET_BUF_PARAM(ZB_BUF_FROM_REF(param), zb_zdo_mgmt_permit_joining_req_param_t);

    ZB_BZERO(req, sizeof(zb_zdo_mgmt_permit_joining_req_param_t));
    req->permit_duration = 2;     /* 2 seconds */
    req->dest_addr = ZB_PIBCACHE_NETWORK_ADDRESS();

    zb_zdo_mgmt_permit_joining_req(param, NULL);
}


void zboss_signal_handler(zb_uint8_t param)
{
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    TRACE_MSG(TRACE_ZCL1, ">> zboss_signal_handler %h", (FMT__H, param));

    if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
    {
        TRACE_MSG(TRACE_APP1, "Skip production config ready signal %d",
                  (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));

        zb_free_buf(ZB_BUF_FROM_REF(param));
        return;
    }

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_SIGNAL_DEVICE_FIRST_START:
            TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));

            ZB_SCHEDULE_CALLBACK(pl_send_permit_joining, param);
            param = 0;
            break;

        default:
            TRACE_MSG(TRACE_APP1, "Unknown signal", (FMT__0));
        }
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d",
                  (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
    }

    if (param)
    {
        zb_free_buf(ZB_BUF_FROM_REF(param));
    }

    TRACE_MSG(TRACE_ZCL1, "<< zboss_signal_handler", (FMT__0));
}
