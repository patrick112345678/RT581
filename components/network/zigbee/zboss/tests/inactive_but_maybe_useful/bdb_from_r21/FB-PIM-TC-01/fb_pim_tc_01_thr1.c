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


#define ZB_TEST_NAME FB_PIM_TC_01_THR1
#define ZB_TRACE_FILE_ID 41064
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
#include "fb_pim_tc_01_common.h"


#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_USE_NVRAM
#error define ZB_USE_NVRAM
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

/********************* Declare device **************************/
DECLARE_ON_OFF_CLIENT_CLUSTER_LIST(on_off_controller_clusters,
                                   basic_attr_list,
                                   identify_attr_list);

DECLARE_ON_OFF_CLIENT_EP(on_off_controller_ep,
                         THR1_ENDPOINT,
                         on_off_controller_clusters);

DECLARE_ON_OFF_CLIENT_CTX(on_off_controller_ctx, on_off_controller_ep);


enum used_profile_id_e
{
    USED_PROFILE_ID_0104,
    USED_PROFILE_ID_0101,
    USED_PROFILE_ID_0102,
    USED_PROFILE_ID_0103,
    USED_PROFILE_ID_0105,
    USED_PROFILE_ID_0106,
    USED_PROFILE_ID_0107,
    USED_PROFILE_ID_0108,
    USED_PROFILE_ID_WILDCARD,
    USED_PROFILE_ID_ZSE,
    USED_PROFILE_ID_GW,
    USED_PROFILE_ID_MSP,
    USED_PROFILE_ID_COUNT
};


static const zb_uint16_t s_arr_prof_id[] =
{
    0x0104, 0x0101, 0x0102, 0x0103,
    0x0105, 0x0106, 0x0107, 0x0108,
    0xffff, 0x0109, 0x7f02, 0x8090
};


static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster);
static void trigger_fb_initiator(zb_uint8_t unused);

static void send_read_attr_req(zb_uint8_t param);
static zb_uint8_t zcl_app_cmd_handler(zb_uint8_t param);
static void read_attr_resp_timeout(zb_uint8_t unused);

static zb_uint8_t s_target_ep;
static zb_uint16_t s_target_cluster;
static int s_matching_clusters;
/* test step index - matching with index of s_arr_prof_id */
static int s_step_idx = USED_PROFILE_ID_0104;


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_thr1");


    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_thr1);
    ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
    ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;
    ZB_BDB().bdb_mode = 1;
    ZB_AIB().aps_use_nvram = 1;

    /* Assignment required to force Distributed formation */
    ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ROUTER;
    ZB_IEEE_ADDR_COPY(ZB_AIB().trust_center_address, g_unknown_ieee_addr);
    zb_secur_setup_nwk_key(g_nwk_key, 0);

    ZB_AF_REGISTER_DEVICE_CTX(&on_off_controller_ctx);
    ZB_AF_SET_ENDPOINT_HANDLER(THR1_ENDPOINT, zcl_app_cmd_handler);

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
    s_target_ep = ep;
    s_target_cluster = cluster;
    ++s_matching_clusters;
    return ZB_TRUE;
}


static void trigger_fb_initiator(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    zb_bdb_finding_binding_initiator(THR1_ENDPOINT, finding_binding_cb);
}


static void send_read_attr_req(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_uint8_t *cmd_ptr;

    TRACE_MSG(TRACE_ZCL1, ">>send_read_attr_req: buf = %d, target prof_id = 0x%x",
              (FMT__D_H, param, s_arr_prof_id[s_step_idx]));
    ZB_ZCL_GENERAL_INIT_READ_ATTR_REQ(buf, cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);
    ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, 0x0000);
    ZB_ZCL_GENERAL_SEND_READ_ATTR_REQ(buf, cmd_ptr,
                                      zb_address_short_by_ieee(g_ieee_addr_dut),
                                      ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT /* ZB_APS_ADDR_MODE_16_ENDP_PRESENT */,
                                      s_target_ep,
                                      THR1_ENDPOINT,
                                      s_arr_prof_id[s_step_idx],
                                      s_target_cluster,
                                      NULL);
    ZB_SCHEDULE_ALARM(read_attr_resp_timeout, 0, CMD_RESP_TIMEOUT);

    TRACE_MSG(TRACE_ZCL1, "<<send_read_attr_req", (FMT__0));
}


static zb_uint8_t zcl_app_cmd_handler(zb_uint8_t param)
{
    zb_buf_t *buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
    zb_zcl_parsed_hdr_t *cmd_info = ZB_GET_BUF_PARAM(buf, zb_zcl_parsed_hdr_t);
    zb_bool_t processed = ZB_FALSE;


    TRACE_MSG(TRACE_ZCL1, ">>zcl_app_cmd_handler: buf = %i", (FMT__H, param));
    TRACE_MSG(TRACE_ZCL1, "payload size: %i", (FMT__D, ZB_BUF_LEN(buf)));

    if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI)
    {
        if (cmd_info->is_common_command)
        {
            if (cmd_info->cmd_id == ZB_ZCL_CMD_READ_ATTRIB_RESP)
            {
                ZB_SCHEDULE_ALARM_CANCEL(read_attr_resp_timeout, ZB_ALARM_ANY_PARAM);
                ++s_step_idx;
                if (s_step_idx < USED_PROFILE_ID_COUNT)
                {
                    ZB_GET_OUT_BUF_DELAYED(send_read_attr_req);
                }
                else
                {
                    TRACE_MSG(TRACE_ZCL1, "TEST FINISHED", (FMT__0));
                }
                processed = ZB_TRUE;
            }
        }
    }

    TRACE_MSG(TRACE_ZCL1, "<<zcl_app_cmd_handler", (FMT__0));

    return processed;
}


static void read_attr_resp_timeout(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    TRACE_MSG(TRACE_ZCL1, ">>read_attr_resp_timeout: prof_id = %d",
              (FMT__D, s_arr_prof_id[s_step_idx]));

    ++s_step_idx;
    if (s_step_idx < USED_PROFILE_ID_COUNT)
    {
        ZB_GET_OUT_BUF_DELAYED(send_read_attr_req);
    }
    else
    {
        TRACE_MSG(TRACE_ZCL1, "TEST FINISHED", (FMT__0));
    }

    TRACE_MSG(TRACE_ZCL1, "<<read_attr_resp_timeout", (FMT__0));
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
            if (ZB_IEEE_ADDR_CMP(g_ieee_addr_thr1, ZB_NIB().extended_pan_id))
            {
                bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
            }
            else
            {
                ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, TEST_TRIGGER_FB_INITIATOR_DELAY);
            }
            break;

        case ZB_BDB_SIGNAL_STEERING:
            TRACE_MSG(TRACE_APS1, "Network steering", (FMT__0));
            ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, TEST_TRIGGER_FB_INITIATOR_DELAY);
            break;

        case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:
            TRACE_MSG(TRACE_APS1, "Finding&binding done", (FMT__0));
            ZB_GET_OUT_BUF_DELAYED(send_read_attr_req);
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
