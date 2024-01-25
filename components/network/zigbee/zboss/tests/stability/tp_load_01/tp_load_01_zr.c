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
/* PURPOSE: ZR
*/

#define ZB_TRACE_FILE_ID 64912
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

#include "tp_load_01_common.h"
#include "tp_load_01_zr.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error ZB_USE_NVRAM is not compiled!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(tp_load_01_zr_basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(tp_load_01_zr_identify_attr_list, &attr_identify_time);

/********************* Declare device **************************/
DECLARE_ZR_CLUSTER_LIST(tp_load_01_zr_device_clusters,
                        tp_load_01_zr_basic_attr_list,
                        tp_load_01_zr_identify_attr_list);

DECLARE_ZR_EP(tp_load_01_zr_device_ep,
              ZR_ENDPOINT,
              tp_load_01_zr_device_clusters);

DECLARE_ZR_CTX(tp_load_01_zr_device_ctx, tp_load_01_zr_device_ep);
/*************************************************************************/

/*******************Definitions for Test***************************/

static zb_ieee_addr_t g_ieee_addr_zr = IEEE_ADDR_ZR;

static zb_ieee_addr_t g_ieee_addr_zc = IEEE_ADDR_ZC;
static zb_uint16_t g_short_addr_zcl_client = ZB_NWK_BROADCAST_ALL_DEVICES;

static zb_bool_t test_finding_binding_cb(zb_int16_t status, zb_ieee_addr_t addr,
        zb_uint8_t ep, zb_uint16_t cluster);
static void test_start_fb_initiator(zb_uint8_t param);
static void test_send_zcl_req_cb(zb_uint8_t param);
static void test_send_zcl_req(zb_uint8_t param);
static void test_send_zcl_reqs(zb_uint8_t param);

MAIN()
{
    ZB_SET_TRACE_MASK(TP_LOAD_01_DEVICES_TRACE_MASK);
    ZB_SET_TRACE_LEVEL(4);
    ARGV_UNUSED;

    ZB_INIT("zdo_zr");

    zb_set_long_address(g_ieee_addr_zr);

    zb_set_network_router_role((1l << TEST_CHANNEL));

    zb_set_nvram_erase_at_start(ZB_TRUE);

    ZB_AF_REGISTER_DEVICE_CTX(&tp_load_01_zr_device_ctx);

    if (zboss_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
    }
    else
    {
        zdo_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}


/***********************************Implementation**********************************/
ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    switch (sig)
    {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));
        if (status == 0)
        {
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        }
        break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APP1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));

        if (status == 0)
        {
            ZB_SCHEDULE_ALARM(test_start_fb_initiator, 0, TP_LOAD_01_ZR_FB_INITIATOR_START_DELAY);
        }

        break; /* ZB_BDB_SIGNAL_STEERING */

    case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:
        TRACE_MSG(TRACE_APP1, "signal: ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED, status %d", (FMT__D, status));
        break;

    default:
        TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}


static zb_bool_t test_finding_binding_cb(zb_int16_t status, zb_ieee_addr_t addr,
        zb_uint8_t ep, zb_uint16_t cluster)
{
    TRACE_MSG(TRACE_APP1, ">> test_finding_binding_cb, status %d, addr " TRACE_FORMAT_64 ", ep %hd, cluster %d",
              (FMT__D_A_H_D, status, TRACE_ARG_64(addr), ep, cluster));

    if (status == ZB_BDB_COMM_BIND_SUCCESS && cluster == ZB_ZCL_CLUSTER_ID_ON_OFF)
    {
        ZB_ASSERT(ZB_IEEE_ADDR_CMP(addr, g_ieee_addr_zc));
        g_short_addr_zcl_client = 0x0000;

        ZB_SCHEDULE_APP_ALARM(test_send_zcl_reqs, 0, TP_LOAD_01_ZR_ZCL_REQ_DELAY);
    }

    return ZB_TRUE;
}


static void test_start_fb_initiator(zb_uint8_t param)
{
    ZVUNUSED(param);

    TRACE_MSG(TRACE_APP1, ">> test_start_fb_initiator", (FMT__0));

    zb_bdb_finding_binding_initiator(ZR_ENDPOINT, test_finding_binding_cb);

    TRACE_MSG(TRACE_APP1, "<< test_start_fb_initiator", (FMT__0));
}


static void test_send_zcl_req_cb(zb_uint8_t param)
{
    zb_zcl_command_send_status_t *cmd_send_status = ZB_BUF_GET_PARAM(param, zb_zcl_command_send_status_t);

    TRACE_MSG(TRACE_APP1, ">> test_send_zcl_req_cb, param %hd, status %hd", (FMT__H_H, param, cmd_send_status->status));

    zb_buf_reuse(param);

    if (cmd_send_status->status == ZB_ZCL_STATUS_SUCCESS)
    {
        ZB_SCHEDULE_APP_CALLBACK(test_send_zcl_req, param);
    }
    else
    {
        ZB_SCHEDULE_APP_ALARM(test_send_zcl_req, param, TP_LOAD_01_DELAY_AFTER_FAILED_REQUEST);
    }

    TRACE_MSG(TRACE_APP1, "<< test_send_zcl_req_cb", (FMT__0));
}


static void test_send_zcl_req(zb_uint8_t param)
{
    zb_uint16_t addr = 0;

    if (param == ZB_BUF_INVALID)
    {
        zb_buf_get_out_delayed(test_send_zcl_req);
        return;
    }

    TRACE_MSG(TRACE_APP1, ">> test_send_zcl_req, param %hd", (FMT__H, param));

    ZB_ZCL_ON_OFF_SEND_REQ(
        param,
        addr,
        ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT,
        0,
        ZR_ENDPOINT,
        ZB_AF_HA_PROFILE_ID,
        ZB_FALSE,
        ZB_ZCL_CMD_ON_OFF_OFF_ID,
        test_send_zcl_req_cb
    );

    TRACE_MSG(TRACE_APP1, "<< test_send_zcl_req", (FMT__0));
}


static void test_send_zcl_reqs(zb_uint8_t param)
{
    zb_uindex_t req_index;

    TRACE_MSG(TRACE_APP1, ">> test_send_zcl_reqs, param %hd, addr 0x%x", (FMT__H_D, param, g_short_addr_zcl_client));

    for (req_index = 0; req_index < TP_LOAD_01_ZR_ZCL_REQ_COUNT; req_index++)
    {
#ifdef TP_LOAD_01_MAX_REQ_JITTER
        ZB_SCHEDULE_APP_ALARM(test_send_zcl_req, 0, ZB_RANDOM_JTR(TP_LOAD_01_MAX_REQ_JITTER));
#else
        zb_buf_get_out_delayed(test_send_zcl_req);
#endif
    }

    TRACE_MSG(TRACE_APP1, "<< test_send_zcl_reqs", (FMT__0));
}

/*! @} */
