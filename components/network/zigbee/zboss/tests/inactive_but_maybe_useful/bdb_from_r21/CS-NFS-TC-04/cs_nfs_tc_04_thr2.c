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
/* PURPOSE: TH ZR2 - joining to network (require TCLK update)
*/


#define ZB_TEST_NAME CS_NFS_TC_04_THR2
#define ZB_TRACE_FILE_ID 41013
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"
#include "cs_nfs_tc_04_common.h"


/*! \addtogroup ZB_TESTS */
/*! @{ */


#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_USE_NVRAM
#error define ZB_USE_NVRAM
#endif


static void retry_join(zb_uint8_t unused);
static void buffer_treq(zb_uint8_t param);
static void buffer_tresp_cb(zb_uint8_t param);


static int s_retry_join_counter;


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_thr2");


    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_thr2);

    ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
    ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;
    ZB_BDB().bdb_mode = 1;
    ZB_NIB().max_children = 0;
    ZB_AIB().aps_use_nvram = 1;

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


static void retry_join(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    if (s_retry_join_counter < MAX_RETRY_JOIN_ATTEMPTS)
    {
        TRACE_MSG(TRACE_ERROR, "JOIN: retry attempt = %d", (FMT__D, s_retry_join_counter));
        ++s_retry_join_counter;
        zb_reset_network_settings();
        ZB_SCHEDULE_ALARM(zb_zdo_dev_start_cont, 0, DO_RETRY_JOIN_DELAY);
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "JOIN: retry join attempts exhausted", (FMT__0));
    }
}


static void buffer_treq(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_buffer_test_req_param_t *req_param;

    TRACE_MSG(TRACE_APS1, ">>buffer_treq", (FMT__0));

    req_param = ZB_GET_BUF_PARAM(buf, zb_buffer_test_req_param_t);
    BUFFER_TEST_REQ_SET_DEFAULT(req_param);
    req_param->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;

    zb_tp_buffer_test_request(param, buffer_tresp_cb);
    TRACE_MSG(TRACE_APS1, "<<buffer_treq", (FMT__0));
}


static void buffer_tresp_cb(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APS1, "buffer_tresp_cb: status = %d", (FMT__D, param));
    if (param == ZB_TP_BUFFER_TEST_OK)
    {
        TRACE_MSG(TRACE_APS1, "TEST: status - SUCCESSFUL response on APS command", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_APS1, "TEST: status ERROR(0x%x)", (FMT__H, param));
    }
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
            ZB_SCHEDULE_ALARM(buffer_treq, param,
                              MIN_COMMISSIONINIG_TIME_DELAY + THRX_SEND_COMMAND_SKEW);
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
        zb_free_buf(ZB_BUF_FROM_REF(param));
        ZB_SCHEDULE_CALLBACK(retry_join, 0);
    }
}


/*! @} */
