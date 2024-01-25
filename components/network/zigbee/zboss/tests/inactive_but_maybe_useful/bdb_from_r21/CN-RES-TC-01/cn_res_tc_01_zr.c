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


#define ZB_TEST_NAME CN_RES_TC_01_ZR
#define ZB_TRACE_FILE_ID 41200
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "cn_res_tc_01_common.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif


/*! \addtogroup ZB_TESTS */
/*! @{ */

static void send_mgmt_lqi_req(zb_uint8_t param, zb_uint8_t idx);

MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_2_zr");


    /* Pass verdict is: broadcast Beacon request at all channels */
    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_r);
    ZB_BDB().bdb_primary_channel_set = (1l << 14);
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

static void test_finished(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, "Test finished", (FMT__0));
    zb_free_buf(ZB_BUF_FROM_REF(param));
}

static void send_node_desc_req(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_zdo_node_desc_req_t *req;

    TRACE_MSG(TRACE_APP1, ">>send_node_desc_req", (FMT__0));

    ZB_BUF_INITIAL_ALLOC(buf, sizeof(zb_zdo_node_desc_req_t), req);
    req->nwk_addr = 0;            /* Coordinator */
    zb_zdo_node_desc_req(param, test_finished);

    TRACE_MSG(TRACE_APP1, "<<send_node_desc_req", (FMT__0));
}

static void mgmt_lqi_resp(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_uint8_t *zdp_cmd = ZB_BUF_BEGIN(buf);
    zb_zdo_mgmt_lqi_resp_t *resp = (zb_zdo_mgmt_lqi_resp_t *)(zdp_cmd);

    if (resp->status == ZB_ZDP_STATUS_SUCCESS &&
            (resp->neighbor_table_entries > (resp->start_index + resp->neighbor_table_list_count)))
    {
        send_mgmt_lqi_req(param, resp->start_index + resp->neighbor_table_list_count);
    }
    else
    {
        ZB_SCHEDULE_CALLBACK(send_node_desc_req, param);
    }
}

static void send_mgmt_lqi_req(zb_uint8_t param, zb_uint8_t idx)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_zdo_mgmt_lqi_param_t *req_param = ZB_GET_BUF_PARAM(buf, zb_zdo_mgmt_lqi_param_t);

    TRACE_MSG(TRACE_APP1, ">>send_mgmt_lqi_req", (FMT__0));

    req_param->dst_addr = 0x0;    /* Coordinator */
    req_param->start_index = idx;
    zb_zdo_mgmt_lqi_req(param, mgmt_lqi_resp);

    TRACE_MSG(TRACE_APP1, "<<send_mgmt_lqi_req", (FMT__0));
}

static void start_mgmt_lqi_req(zb_uint8_t param)
{
    if (param == 0)
    {
        ZB_GET_OUT_BUF_DELAYED(start_mgmt_lqi_req);
    }
    else
    {
        send_mgmt_lqi_req(param, 0);
    }
}

static void send_beacon_request(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">>send_beacon_request", (FMT__0));

    ZB_MLME_BUILD_SCAN_REQUEST((zb_buf_t *)ZB_BUF_FROM_REF(param), ZB_BDB().bdb_primary_channel_set,
                               ACTIVE_SCAN, TEST_SCAN_DURATION);

    ZB_SCHEDULE_CALLBACK(zb_mlme_scan_request, param);

    ZB_SCHEDULE_ALARM(start_mgmt_lqi_req, 0, TEST_MGMT_LQI_REQ_DELAY);

    TRACE_MSG(TRACE_APP1, "<<send_beacon_request", (FMT__0));
}

static void start_test_sequence(zb_uint8_t param)
{
    ZB_SCHEDULE_CALLBACK(send_beacon_request, param);
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    if (buf->u.hdr.status == 0)
    {
        TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
        ZB_SCHEDULE_ALARM(start_test_sequence, param, TEST_START_TIMEOUT);
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, (int)buf->u.hdr.status));
        zb_free_buf(buf);
    }
}


/*! @} */
