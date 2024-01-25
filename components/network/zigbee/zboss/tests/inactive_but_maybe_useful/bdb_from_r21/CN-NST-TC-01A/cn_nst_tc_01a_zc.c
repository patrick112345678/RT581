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

#define ZB_TEST_NAME CN_NST_TC_01A_ZC
#define ZB_TRACE_FILE_ID 40996
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_bdb_internal.h"
#include "zb_secur_api.h"


/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

static zb_ieee_addr_t g_ext_panid = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static zb_ieee_addr_t g_ieee_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
/* [zb_secur_setup_preconfigured_key_1] */
static zb_uint8_t g_key_nwk[16] = { 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0, 0, 0, 0, 0, 0, 0, 0};
/* [zb_secur_setup_preconfigured_key_1] */

static zb_ieee_addr_t g_ieee_addr1 = {0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
static zb_uint8_t g_key1[16] = { 0x12, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33};
static zb_ieee_addr_t g_ieee_addr2 = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static zb_ieee_addr_t g_ieee_addr3 = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_1_zc");

    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr);


    ZB_PIBCACHE_PAN_ID() = 0x1aaa;

    /* let's always be coordinator */
    ZB_AIB().aps_designated_coordinator = 1;
    /* [zb_secur_setup_preconfigured_key_2] */
    zb_secur_setup_nwk_key(g_key_nwk, 0);
    /* [zb_secur_setup_preconfigured_key_2] */

    ZB_NIB().max_children = 2;

    ZB_AIB().aps_insecure_join = ZB_TRUE;

    ZB_BDB().bdb_mode = ZB_TRUE;

    if (zdo_dev_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
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
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
            TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
            break;
        default:
            TRACE_MSG(TRACE_APS1, "Unknown signal", (FMT__0));
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
    zb_free_buf(ZB_BUF_FROM_REF(param));
}


/*! @} */
