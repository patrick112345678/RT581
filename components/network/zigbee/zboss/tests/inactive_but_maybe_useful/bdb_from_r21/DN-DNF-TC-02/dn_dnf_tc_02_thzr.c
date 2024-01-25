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
/* PURPOSE: ZED
*/

#define ZB_TEST_NAME DN_DNF_TC_02_THZR
#define ZB_TRACE_FILE_ID 41050
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_bdb_internal.h"

static zb_ieee_addr_t g_ieee_addr = {0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};

MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */
    {

        ZB_INIT("zdo_2_thzr");

    }

    /* set ieee addr */
    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr);

    /* become an ED */
    ZB_BDB().bdb_primary_channel_set = (1 << 14);
    ZB_BDB().bdb_mode = 1;

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

static void second_join(zb_uint8_t param)
{
    (void)param;
    TRACE_MSG(TRACE_ZCL1, "Starting second network steering", (FMT__0));
    bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    static zb_int_t fail_cnt = 0;
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);

    if (buf->u.hdr.status == 0)
    {
        TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
        zb_free_buf(buf);
    }
    else if (fail_cnt > 0)
    {
        TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
    }
    else
    {
        fail_cnt++;
        TRACE_MSG(TRACE_ERROR, "start attempt FAILED status %d", (FMT__D, (int)buf->u.hdr.status));
        zb_free_buf(buf);
        ZB_SCHEDULE_ALARM(second_join, 0, 25 * ZB_TIME_ONE_SECOND);
    }
}
