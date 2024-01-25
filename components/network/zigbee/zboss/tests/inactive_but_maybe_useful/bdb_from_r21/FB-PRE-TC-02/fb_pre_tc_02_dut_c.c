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
/* PURPOSE: DUT ZC1 (answering on discovery requests)
*/


#define ZB_TEST_NAME FB_PRE_TC_02_DUT_C
#define ZB_TRACE_FILE_ID 41252
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_zcl.h"
#include "zb_bdb_internal.h"
#include "zb_zcl.h"
#include "fb_pre_tc_02_common.h"


#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif


#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif


/*! \addtogroup ZB_TESTS */
/*! @{ */


static void remove_child(zb_uint8_t param);
static void remove_child_delayed(zb_uint8_t unused);


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_dut");


    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_dut);
    ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
    ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;
    ZB_BDB().bdb_mode = 1;

    zb_secur_setup_nwk_key(g_nwk_key, 0);

    ZB_NIB().max_children = 3;
    ZB_AIB().aps_designated_coordinator = 1;

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


static void remove_child(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_nlme_leave_request_t *req = ZB_GET_BUF_TAIL(buf, sizeof(zb_nlme_leave_request_t));

    TRACE_MSG(TRACE_ZDO1, ">>remove_child: buf_param = %d", (FMT__D, param));

    ZB_IEEE_ADDR_COPY(req->device_address, g_ieee_addr_the1);
    req->remove_children = 0;
    req->rejoin = 0;
    zb_nlme_leave_request(param);

    TRACE_MSG(TRACE_ZDO1, "<<remove_child", (FMT__0));
}


static void remove_child_delayed(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    ZB_GET_OUT_BUF_DELAYED(remove_child);
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

        case ZB_BDB_SIGNAL_STEERING:
            ZB_SCHEDULE_ALARM(remove_child_delayed, 0, DUT_REMOVES_THE1_DELAY);;
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
