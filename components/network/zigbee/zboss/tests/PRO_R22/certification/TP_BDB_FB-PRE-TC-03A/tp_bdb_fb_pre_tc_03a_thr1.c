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
/* PURPOSE: TH ZR1 (initiator)
*/

#define ZB_TEST_NAME TP_BDB_FB_PRE_TC_03A_THR1
#define ZB_TRACE_FILE_ID 40063

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

#include "on_off_client.h"
#include "tp_bdb_fb_pre_tc_03a_common.h"
#include "../common/zb_cert_test_globals.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif


#ifndef ZB_CERTIFICATION_HACKS
#error ZB_CERTIFICATION_HACKS is not defined!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_thr1 = IEEE_ADDR_THR1;
static zb_ieee_addr_t g_ieee_addr_the1 = IEEE_ADDR_THE1;

static zb_uint8_t g_nwk_key[16] = {0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                                  };

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(fb_pre_tc_03a_thr1_basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(fb_pre_tc_03a_thr1_identify_attr_list, &attr_identify_time);

/********************* Declare device **************************/
DECLARE_ON_OFF_CLIENT_CLUSTER_LIST(fb_pre_tc_03a_thr1_on_off_controller_clusters,
                                   fb_pre_tc_03a_thr1_basic_attr_list,
                                   fb_pre_tc_03a_thr1_identify_attr_list);

DECLARE_ON_OFF_CLIENT_EP(fb_pre_tc_03a_thr1_on_off_controller_ep,
                         THR1_ENDPOINT,
                         fb_pre_tc_03a_thr1_on_off_controller_clusters);

DECLARE_ON_OFF_CLIENT_CTX(fb_pre_tc_03a_thr1_on_off_controller_ctx, fb_pre_tc_03a_thr1_on_off_controller_ep);


static void trigger_fb_target(zb_uint8_t unused);
static void toggle_frame_retransmission(zb_uint8_t unused);
static zb_uint8_t identify_handler(zb_uint8_t param);
static zb_bool_t zdo_rx_handler(zb_uint8_t param, zb_uint16_t cluster_id);


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_thr1");


    zb_set_long_address(g_ieee_addr_thr1);

    zb_set_network_router_role((1l << TEST_CHANNEL));
    zb_set_nvram_erase_at_start(ZB_TRUE);

    /* Assignment required to force Distributed formation */
    zb_aib_set_trust_center_address(g_unknown_ieee_addr);
    zb_secur_setup_nwk_key(g_nwk_key, 0);

    zb_set_max_children(1);

    ZB_AF_REGISTER_DEVICE_CTX(&fb_pre_tc_03a_thr1_on_off_controller_ctx);

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


static void trigger_fb_target(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    ZB_BDB().bdb_commissioning_time = FB_TARGET_DURATION;
    zb_bdb_finding_binding_target(THR1_ENDPOINT);
}


static void toggle_frame_retransmission(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    TRACE_MSG(TRACE_ZDO1, "Toggle frame retransmission", (FMT__0));
    if (!ZB_CERT_HACKS().disable_frame_retransmission)
    {
        ZB_AF_SET_ENDPOINT_HANDLER(THR1_ENDPOINT, identify_handler);
    }
    else
    {
        ZB_AF_SET_ENDPOINT_HANDLER(THR1_ENDPOINT, NULL);
        ZB_CERT_HACKS().disable_frame_retransmission = 0;
        ZB_CERT_HACKS().force_frame_indication = 0;
        ZB_CERT_HACKS().pass_incoming_zdo_cmd_to_app = 0;
        ZB_CERT_HACKS().zdo_af_handler_cb = NULL;
    }
}


static zb_uint8_t identify_handler(zb_uint8_t param)
{
    ZVUNUSED(param);
    ZB_CERT_HACKS().pass_incoming_zdo_cmd_to_app = 1;
    ZB_CERT_HACKS().zdo_af_handler_cb = zdo_rx_handler;
    ZB_CERT_HACKS().disable_frame_retransmission = 0;
    ZB_CERT_HACKS().force_frame_indication = 1;
    return ZB_FALSE;
}


static zb_bool_t zdo_rx_handler(zb_uint8_t param, zb_uint16_t cluster_id)
{
    zb_uint16_t my_addr = zb_cert_test_get_network_addr();
    zb_uint16_t ed1_addr;

    TRACE_MSG(TRACE_ZDO1, ">>zdo_rx_handler: buf_param = %hd", (FMT__H, param));

    ZB_CERT_HACKS().disable_frame_retransmission = 1;
    ed1_addr = zb_address_short_by_ieee(g_ieee_addr_the1);
    zb_cert_test_set_network_addr(ed1_addr);

    switch (cluster_id)
    {
    case ZDO_ACTIVE_EP_REQ_CLID:
    {
        zdo_active_ep_res(param);
    }
    break;
    case ZDO_SIMPLE_DESC_REQ_CLID:
    {
        zdo_send_simple_desc_resp(param);
    }
    break;
    case ZDO_MATCH_DESC_REQ_CLID:
    {
        zdo_match_desc_res(param);
    }
    break;
    default:
        zb_buf_free(param);
    }

    zb_cert_test_set_network_addr(my_addr);

    TRACE_MSG(TRACE_ZDO1, "<<zdo_rx_handler", (FMT__0));

    return ZB_TRUE;
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    switch (sig)
    {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        TRACE_MSG(TRACE_APS1, "Device started, status %d", (FMT__D, status));
        if (status == 0)
        {
            zb_ext_pan_id_t extended_pan_id;
            zb_get_extended_pan_id(extended_pan_id);

            if (ZB_IEEE_ADDR_CMP(g_ieee_addr_thr1, extended_pan_id))
            {
                bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
            }
            else
            {
                ZB_SCHEDULE_ALARM(trigger_fb_target, 0, THR1_FB_TARGET_DELAY);
            }
            ZB_SCHEDULE_ALARM(toggle_frame_retransmission, 0, THE1_FB_TARGET_DELAY1);
            ZB_SCHEDULE_ALARM(toggle_frame_retransmission, 0, THE1_FB_TARGET_DELAY2);
        }
        break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
        if (status == 0)
        {
            ZB_SCHEDULE_ALARM(trigger_fb_target, 0, THR1_FB_TARGET_DELAY);
        }
        break; /* ZB_BDB_SIGNAL_STEERING */

    default:
        TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}

/*! @} */
