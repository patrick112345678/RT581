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
/* PURPOSE: TH ZR1 - joining to network (without TCLK update)
*/


#define ZB_TEST_NAME CS_NFS_TC_04_THR1
#define ZB_TRACE_FILE_ID 41014
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
static void mgmt_join_req(zb_uint8_t param);
static void mgmt_join_resp(zb_uint8_t param);
static void buffer_treq(zb_uint8_t param);
static void buffer_tresp_cb(zb_uint8_t param);


static int s_retry_join_counter;


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_thr1");


    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_thr1);

    ZB_AIB().aps_channel_mask = TEST_BDB_PRIMARY_CHANNEL_SET;
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


static void mgmt_join_req(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_zdo_mgmt_permit_joining_req_param_t *req_param;

    TRACE_MSG(TRACE_ZDO1, ">>mgmt_join_req", (FMT__0));

    req_param = ZB_GET_BUF_PARAM(buf, zb_zdo_mgmt_permit_joining_req_param_t);
    ZB_BZERO(req_param, sizeof(zb_zdo_mgmt_permit_joining_req_param_t));
    req_param->dest_addr = ZB_NWK_BROADCAST_ROUTER_COORDINATOR;
    req_param->permit_duration = ZB_BDBC_MIN_COMMISSIONING_TIME_S;
    req_param->tc_significance = 1;
    zb_zdo_mgmt_permit_joining_req(param, mgmt_join_resp);

    TRACE_MSG(TRACE_ZDO1, "<<mgmt_join_req", (FMT__0));
}


static void mgmt_join_resp(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);

    TRACE_MSG(TRACE_ZDO1, ">>mgmt_join_resp", (FMT__0));

    if (buf->u.hdr.status == RET_OK)
    {
        zb_buf_reuse(buf);
        zb_switch_buf(buf, 0);
        ZB_SCHEDULE_ALARM(buffer_treq, param, MIN_COMMISSIONINIG_TIME_DELAY + THRX_SEND_COMMAND_SKEW);
    }
    else
    {
        TRACE_MSG(TRACE_ZDO1,
                  "TEST: error occured while sending mgmt_permit_join_req - 0x%x",
                  (FMT__H, buf->u.hdr.status));
        zb_free_buf(buf);
    }

    TRACE_MSG(TRACE_ZDO1, "<<mgmt_join_resp", (FMT__0));
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
        TRACE_MSG(TRACE_APS1, "TEST: status - COMPLETED", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_APS1, "TEST: status ERROR(0x%x)", (FMT__H, param));
    }
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);

    if (buf->u.hdr.status == 0)
    {
        TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
        ZB_SCHEDULE_CALLBACK(mgmt_join_req, param);
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, buf->u.hdr.status));
        zb_free_buf(ZB_BUF_FROM_REF(param));
        ZB_SCHEDULE_CALLBACK(retry_join, 0);
    }
}


/*! @} */
