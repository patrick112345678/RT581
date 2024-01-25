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
/* PURPOSE: Simple coordinator for GP device
*/

#define ZB_TRACE_FILE_ID 40152
#include "zboss_api.h"

#if ! defined ZB_ROUTER_ROLE
#error define ZB_ROUTER_ROLE to compile zr tests
#endif


zb_ieee_addr_t g_zr_addr = {0xa2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
zb_uint8_t g_key_nwk[16] = { 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0, 0, 0, 0, 0, 0, 0, 0};

zb_uint8_t test_specific_cluster_cmd_handler(zb_uint8_t param);

MAIN()
{
    ARGV_UNUSED;

    /* ZB_SET_TRACE_LEVEL(0);
    ZB_SET_TRACE_MASK(0);
    ZB_SET_TRACE_OFF(); */
    ZB_SET_TRAF_DUMP_ON();

    ZB_INIT("zr_proxy");

    /* use well-known key to simplify decrypt in Wireshark */
    zb_secur_setup_nwk_key(g_key_nwk, 0);

    zb_set_long_address(g_zr_addr);
    /*zb_set_network_router_role(ZB_TRANSCEIVER_ALL_CHANNELS_MASK);*/
    zb_set_network_router_role(1l << 21);
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


/**
   ZDO start/commissioning state change callback

   @param - buffer with event
 */
void zboss_signal_handler(zb_uint8_t param)
{
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    TRACE_MSG(TRACE_ZCL1, "> zboss_signal_handler %h", (FMT__H, param));

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
            TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
            break;

        case ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY:
            TRACE_MSG(TRACE_APP1, "Loading application production config", (FMT__0));
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
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
    }

    if (param)
    {
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_ZCL1, "< zboss_signal_handler", (FMT__0));
}
