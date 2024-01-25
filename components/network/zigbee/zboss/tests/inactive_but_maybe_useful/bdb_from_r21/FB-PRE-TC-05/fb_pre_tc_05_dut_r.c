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
/* PURPOSE: DUT ZR (initiator)
*/


#define ZB_TEST_NAME FB_PRE_TC_05_DUT_R
#define ZB_TRACE_FILE_ID 41056
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
#include "test_device_initiator.h"
#include "fb_pre_tc_05_common.h"


#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
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

/* Temperature Measurement attributes data */
static zb_int16_t attr_temp_value     = ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_UNKNOWN;
static zb_int16_t attr_min_temp_value = ZB_ZCL_ATTR_TEMP_MEASUREMENT_MIN_VALUE_MIN_VALUE;
static zb_int16_t attr_max_temp_value = ZB_ZCL_ATTR_TEMP_MEASUREMENT_MAX_VALUE_MAX_VALUE;
static zb_uint16_t attr_tolerance     = ZB_ZCL_ATTR_TEMP_MEASUREMENT_TOLERANCE_MAX_VALUE;
ZB_ZCL_DECLARE_TEMP_MEASUREMENT_ATTRIB_LIST(temp_meas_attr_list,
        &attr_temp_value,
        &attr_min_temp_value,
        &attr_max_temp_value,
        &attr_tolerance);

/********************* Declare device **************************/
DECLARE_INITIATOR_CLUSTER_LIST(initiator_clusters,
                               basic_attr_list,
                               identify_attr_list,
                               temp_meas_attr_list);

DECLARE_INITIATOR_EP(initiator_ep,
                     DUT_ENDPOINT,
                     initiator_clusters);

DECLARE_INITIATOR_CTX(initiator_ctx, initiator_ep);


/*******************Definitions for Test***************************/

#define GO_TO_NEXT_STEP(action) do                                \
  {                                                               \
    ZB_SCHEDULE_ALARM(buffer_manager, action, DUT_SHORT_DELAY);   \
  } while (0)

enum test_actions_e
{
    ACTION_ADD_GROUP_IF_IDENTIFYING,
    ACTION_VIEW_GROUP,
    ACTION_GET_GROUP_MEMBERSHIP,
    ACTION_REMOVE_GROUP,
    ACTION_REMOVE_ALL_GROUPS
};


static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster);
static void trigger_fb_initiator(zb_uint8_t unused);


static void buffer_manager(zb_uint8_t action);
static void send_add_group_if_identifying(zb_uint8_t param);
static void send_view_group(zb_uint8_t param);
static void send_get_group_membership(zb_uint8_t param);
static void send_remove_group(zb_uint8_t param);
static void send_remove_all_groups(zb_uint8_t param);


static zb_uint8_t s_target_ep;
static zb_uint16_t s_target_nwk_addr;


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_dut");


    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_dut);

    ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
    ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;
    ZB_BDB().bdb_mode = 1;

    /* Assignment required to force Distributed formation */
    ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ROUTER;
    ZB_IEEE_ADDR_COPY(ZB_AIB().trust_center_address, g_unknown_ieee_addr);
    zb_secur_setup_nwk_key(g_nwk_key, 0);

    ZB_AF_REGISTER_DEVICE_CTX(&initiator_ctx);

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


/***********************************Implementation**********************************/

static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster)
{
    TRACE_MSG(TRACE_ZCL1, "finding_binding_cb status %d addr " TRACE_FORMAT_64 " ep %hd cluster %d",
              (FMT__D_A_H_D, status, TRACE_ARG_64(addr), ep, cluster));
    s_target_ep = ep;
    return ZB_TRUE;
}


static void trigger_fb_initiator(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    ZB_BDB().bdb_commissioning_time = DUT_FB_DURATION;
    ZB_BDB().bdb_commissioning_group_id = DUT_COMMISSIONINIG_GROUP_ID;
    zb_bdb_finding_binding_initiator(DUT_ENDPOINT, finding_binding_cb);
}


static void buffer_manager(zb_uint8_t action)
{
    zb_callback_t next_func = NULL;  /* next cb to call */

    TRACE_MSG(TRACE_ZCL1, ">>buffer_manager: action = %d", (FMT__D, action));

    switch (action)
    {
    case ACTION_ADD_GROUP_IF_IDENTIFYING:
        next_func = send_add_group_if_identifying;
        break;
    case ACTION_VIEW_GROUP:
        next_func = send_view_group;
        break;
    case ACTION_GET_GROUP_MEMBERSHIP:
        next_func = send_get_group_membership;
        break;
    case ACTION_REMOVE_GROUP:
        next_func = send_remove_group;
        break;
    case ACTION_REMOVE_ALL_GROUPS:
        next_func = send_remove_all_groups;
        break;
    default:
        TRACE_MSG(TRACE_ZCL1, "STOP TEST - unknown command", (FMT__0));
    }

    if (next_func)
    {
        ZB_GET_OUT_BUF_DELAYED(next_func);
    }

    TRACE_MSG(TRACE_ZCL1, ">>buffer_manager", (FMT__0));
}


static void send_add_group_if_identifying(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);

    TRACE_MSG(TRACE_ZCL2, ">>send_add_group_if_identifying: buf_param = %d", (FMT__D, param));

    ZB_ZCL_GROUPS_SEND_ADD_GROUP_IF_IDENT_REQ(buf, s_target_nwk_addr,
            ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
            s_target_ep, DUT_ENDPOINT,
            ZB_AF_HA_PROFILE_ID,
            ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
            NULL,
            ZB_BDB().bdb_commissioning_group_id);

    TRACE_MSG(TRACE_ZCL2, "<<send_add_group_if_identifying", (FMT__0));
}


static void send_view_group(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);

    TRACE_MSG(TRACE_ZCL2, ">>send_view_group: buf_param = %d", (FMT__D, param));

    ZB_ZCL_GROUPS_SEND_VIEW_GROUP_REQ(buf, s_target_nwk_addr,
                                      ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                      s_target_ep, DUT_ENDPOINT,
                                      ZB_AF_HA_PROFILE_ID,
                                      ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                                      NULL,
                                      ZB_BDB().bdb_commissioning_group_id);

    GO_TO_NEXT_STEP(ACTION_GET_GROUP_MEMBERSHIP);

    TRACE_MSG(TRACE_ZCL2, "<<send_view_group", (FMT__0));
}


static void send_get_group_membership(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_uint8_t *ptr;

    TRACE_MSG(TRACE_ZCL2, ">>send_get_group_membership: buf_param = %d", (FMT__D, param));

    ZB_ZCL_GROUPS_INIT_GET_GROUP_MEMBERSHIP_REQ(buf, ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, 0);
    ZB_ZCL_GROUPS_ADD_ID_GET_GROUP_MEMBERSHIP_REQ(ptr, ZB_BDB().bdb_commissioning_group_id);
    ZB_ZCL_GROUPS_SEND_GET_GROUP_MEMBERSHIP_REQ(buf, ptr, s_target_nwk_addr,
            ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
            s_target_ep, DUT_ENDPOINT,
            ZB_AF_HA_PROFILE_ID, NULL);

    GO_TO_NEXT_STEP(ACTION_REMOVE_GROUP);

    TRACE_MSG(TRACE_ZCL2, "<<send_get_group_membership", (FMT__0));
}


static void send_remove_group(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);

    TRACE_MSG(TRACE_ZCL2, ">>send_remove_group: buf_param = %d", (FMT__D, param));

    ZB_ZCL_GROUPS_SEND_REMOVE_GROUP_REQ(buf, s_target_nwk_addr,
                                        ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                        s_target_ep, DUT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                        ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                                        NULL,
                                        ZB_BDB().bdb_commissioning_group_id);

    GO_TO_NEXT_STEP(ACTION_REMOVE_ALL_GROUPS);

    TRACE_MSG(TRACE_ZCL2, "<<send_remove_group", (FMT__0));
}


static void send_remove_all_groups(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);

    TRACE_MSG(TRACE_ZCL2, ">>send_remove_all_groups: buf_param = %d", (FMT__D, param));

    ZB_ZCL_GROUPS_SEND_REMOVE_ALL_GROUPS_REQ(buf, s_target_nwk_addr,
            ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
            s_target_ep, DUT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
            ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
            NULL);

    TRACE_MSG(TRACE_ZCL2, "<<send_remove_all_groups", (FMT__0));
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
            TRACE_MSG(TRACE_APS1, "Steering on network completed", (FMT__0));
            ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, DUT_FB_INITIATOR_DELAY);
            break;

        case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:
            TRACE_MSG(TRACE_APS1, "Finding&binding done", (FMT__0));
            s_target_nwk_addr = zb_address_short_by_ieee(g_ieee_addr_thr1);
            if (BDB_COMM_CTX().state == ZB_BDB_COMM_IDLE)
            {
                GO_TO_NEXT_STEP(ACTION_VIEW_GROUP);
            }
            else
            {
                GO_TO_NEXT_STEP(ACTION_ADD_GROUP_IF_IDENTIFYING);
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
