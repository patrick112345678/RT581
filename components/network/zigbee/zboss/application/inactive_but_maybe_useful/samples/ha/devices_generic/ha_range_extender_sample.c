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
/* PURPOSE: Range extender sample for HA profile
*/

#define ZB_TRACE_FILE_ID 40161
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_zcl.h"

#define ZB_HA_DEFINE_DEVICE_RANGE_EXTENDER
#include "zb_ha.h"


#if ! defined ZB_ROUTER_ROLE
#error define ZB_ROUTER_ROLE to compile ze tests
#endif

#define HA_RANGE_EXTENDER_ENDPOINT          14

#define PAN_ID 0x1AAA

zb_ieee_addr_t g_zc_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};


/* Handler for specific zcl commands */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);


/** [COMMON_DECLARATION] */
/******************* Declare attributes ************************/

/* Basic cluster attributes data */
zb_uint8_t g_attr_zcl_version  = ZB_ZCL_VERSION;
zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list, &g_attr_zcl_version, &g_attr_power_source);

/* Identify cluster attributes data */
zb_uint16_t g_attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_attr_identify_time);

/********************* Declare device **************************/

ZB_HA_DECLARE_RANGE_EXTENDER_CLUSTER_LIST(range_extender_clusters, basic_attr_list, identify_attr_list);

ZB_HA_DECLARE_RANGE_EXTENDER_EP(range_extender_ep, HA_RANGE_EXTENDER_ENDPOINT, range_extender_clusters);

ZB_HA_DECLARE_RANGE_EXTENDER_CTX(range_extender_ctx, range_extender_ep);
/** [COMMON_DECLARATION] */

MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("ha_range_extender_sample");

    /* 2018/08/10 CR:MAJOR (rev. 39308): Coordinator is always rx on when idle. */
    zb_set_rx_on_when_idle(ZB_TRUE);
    zb_set_long_address(g_zc_addr);
    zb_set_network_coordinator_role(ZB_DEFAULT_APS_CHANNEL_MASK);
    zb_set_default_ffd_descriptor_values(ZB_COORDINATOR);
    ZB_PIBCACHE_PAN_ID() = PAN_ID;

    /****************** Register Device ********************************/
    /** [REGISTER] */
    ZB_AF_REGISTER_DEVICE_CTX(&range_extender_ctx);
    ZB_AF_SET_ENDPOINT_HANDLER(HA_RANGE_EXTENDER_ENDPOINT, zcl_specific_cluster_cmd_handler);
    /** [REGISTER] */

    if (zboss_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
    }
    else
    {
        zboss_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}

/** [VARIABLE] */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
    zb_buf_t *zcl_cmd_buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
    zb_zcl_parsed_hdr_t *cmd_info = ZB_GET_BUF_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);
    zb_bool_t cmd_processed = ZB_FALSE;
    /** [VARIABLE] */

    TRACE_MSG(TRACE_ZCL1, "> zcl_specific_cluster_cmd_handler %i", (FMT__H, param));
    TRACE_MSG(TRACE_ZCL3, "payload size: %i", (FMT__D, ZB_BUF_LEN(zcl_cmd_buf)));

    if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI)
    {
        TRACE_MSG(TRACE_ZCL1, "CLNT role cluster 0x%d is not supported", (FMT__D, cmd_info->cluster_id));
    }
    else
    {
        /* Command from client to server ZB_ZCL_FRAME_DIRECTION_TO_SRV */
        switch (cmd_info->cluster_id)
        {
        case ZB_ZCL_CLUSTER_ID_IDENTIFY:
            if (cmd_info->is_common_command)
            {
                TRACE_MSG(TRACE_ZCL2, "Skip general command %hd", (FMT__H, cmd_info->cmd_id));
            }
            else
            {
                /** [HANDLER_CODE] */
                switch (cmd_info->cmd_id)
                {
                case ZB_ZCL_CMD_IDENTIFY_IDENTIFY_ID:
                    TRACE_MSG(TRACE_ZCL3, "Got cluster command 0x%04x", (FMT__D, cmd_info->cmd_id));
                    /* process identify command */
                    cmd_processed = ZB_TRUE;
                    break;

                default:
                    TRACE_MSG(TRACE_ZCL2, "Cluster command %hd, skip it", (FMT__H, cmd_info->cmd_id));
                    break;
                }
                /** [HANDLER_CODE] */
            }
            break;

        default:
            TRACE_MSG(TRACE_ZCL1, "SRV role, cluster 0x%d is not supported", (FMT__D, cmd_info->cluster_id));
            break;
        }
    }

    TRACE_MSG(TRACE_ZCL1, "< zcl_specific_cluster_cmd_handler %hd", (FMT__H, cmd_processed));
    return cmd_processed;
}


void zboss_signal_handler(zb_uint8_t param)
{
    zb_zdo_app_signal_hdr_t *sg_p = NULL;
    zb_zdo_app_signal_t sig = zb_get_app_signal(param, &sg_p);

    TRACE_MSG(TRACE_APP1, ">>zboss_signal_handler: status %hd signal %hd",
              (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
            break;

        default:
            TRACE_MSG(TRACE_APP1, "Unknown signal %hd", (FMT__H, sig));
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

    if (param)
    {
        ZB_FREE_BUF(ZB_BUF_FROM_REF(param));
    }

    TRACE_MSG(TRACE_APP1, "<<zboss_signal_handler", (FMT__0));
}
