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
/* PURPOSE: DUT ZED (initiator)
*/


#define ZB_TEST_NAME FB_PRE_TC_03A_DUT_ED
#define ZB_TRACE_FILE_ID 40966
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
#include "on_off_server.h"
#include "fb_pre_tc_03a_common.h"


#ifndef ZB_ED_ROLE
#error End device role is not compiled!
#endif



/*! \addtogroup ZB_TESTS */
/*! @{ */


/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &attr_identify_time);

static zb_bool_t attr_on_off = 1;
ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(on_off_attr_list, &attr_on_off);

/********************* Declare device **************************/
DECLARE_ON_OFF_SERVER_CLUSTER_LIST(on_off_device_clusters,
                                   basic_attr_list,
                                   identify_attr_list,
                                   on_off_attr_list);

DECLARE_ON_OFF_SERVER_EP(on_off_device_ep,
                         DUT_ENDPOINT,
                         on_off_device_clusters);

DECLARE_ON_OFF_SERVER_CTX(on_off_device_ctx, on_off_device_ep);


static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster);
static void trigger_fb_initiator(zb_uint8_t unused);
static void send_match_desc(zb_uint8_t param);
static void match_desc_resp_cb(zb_uint8_t param);

static zb_ieee_addr_t s_target_ieee;
static zb_uint8_t s_target_ep;
static zb_uint16_t s_target_cluster;
static int s_matching_clusters;


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_dut");


    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_dut);

    ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
    ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;
    ZB_BDB().bdb_mode = 1;

    /* Start as ED */
    ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ED;

    ZB_AF_REGISTER_DEVICE_CTX(&on_off_device_ctx);

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


static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster)
{
    TRACE_MSG(TRACE_ZCL1, "finding_binding_cb status %d addr " TRACE_FORMAT_64 " ep %hd cluster %d",
              (FMT__D_A_H_D, status, TRACE_ARG_64(addr), ep, cluster));
    ZB_IEEE_ADDR_COPY(s_target_ieee, addr);
    s_target_ep = ep;
    s_target_cluster = cluster;
    ++s_matching_clusters;
    return ZB_TRUE;
}


static void trigger_fb_initiator(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    zb_bdb_finding_binding_initiator(DUT_ENDPOINT, finding_binding_cb);
}


static void send_match_desc(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_uint8_t req_size = sizeof(zb_zdo_match_desc_param_t) + sizeof(zb_uint32_t);
    zb_zdo_match_desc_param_t *req;

    TRACE_MSG(TRACE_APP1, "send_match_desc_req: buf = %d", (FMT__D, param));

    ZB_BUF_INITIAL_ALLOC(buf, req_size, req);

    req->nwk_addr = zb_address_short_by_ieee(s_target_ieee);
    req->addr_of_interest = req->nwk_addr;
    req->profile_id = ZB_AF_HA_PROFILE_ID;
    req->num_in_clusters = 2;
    req->num_out_clusters = 1;
    req->cluster_list[0] = ZB_ZCL_CLUSTER_ID_BASIC;
    req->cluster_list[1] = ZB_ZCL_CLUSTER_ID_ON_OFF;
    req->cluster_list[2] = ZB_ZCL_CLUSTER_ID_IDENTIFY;

    zb_zdo_match_desc_req(param, match_desc_resp_cb);
}


static void match_desc_resp_cb(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);

    TRACE_MSG(TRACE_APP1, "match_desc_resp_cb: buf = %d", (FMT__D, param));

    zb_apsme_unbind_all(100);
    zb_free_buf(buf);
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
            ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, DUT_FB_INITIATOR_DELAY);
            break;

        case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:
            TRACE_MSG(TRACE_APS1, "Finding&binding done", (FMT__0));
            if (BDB_COMM_CTX().state != ZB_BDB_COMM_IDLE)
            {
                ZB_GET_OUT_BUF_DELAYED(send_match_desc);
            }
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
