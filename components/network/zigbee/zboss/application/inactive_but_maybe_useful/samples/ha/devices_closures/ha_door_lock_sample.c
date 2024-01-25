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
/* PURPOSE: Door Lock for HA profile
*/

#define ZB_TRACE_FILE_ID 40175
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_zcl.h"

#define ZB_HA_DEFINE_DEVICE_DOOR_LOCK
#include "zb_ha.h"

#if defined ZB_ENABLE_HA


#ifndef ZB_COORDINATOR_ROLE
#error define ZB_COORDINATOR_ROLE to compile zc tests
#endif



#define HA_DOOR_LOCK_ENDPOINT          15

/* Handler for specific zcl commands */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);

zb_ieee_addr_t g_ed_addr = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/** [COMMON_DECLARATION] */
/******************* Declare attributes ************************/
/* Basic cluster attributes data */
zb_uint8_t g_attr_zcl_version  = ZB_ZCL_VERSION;
zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list, &g_attr_zcl_version, &g_attr_power_source);

/* Identify cluster attributes data */
zb_uint16_t g_attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_attr_identify_time);

/* Door Lock cluster attributes data */
zb_uint8_t g_lock_state       = ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_DEFAULT_VALUE;
zb_uint8_t g_lock_type        = ZB_ZCL_ATTR_DOOR_LOCK_LOCK_TYPE_DEFAULT_VALUE;
zb_uint8_t g_actuator_enabled = ZB_ZCL_ATTR_DOOR_LOCK_ACTUATOR_ENABLED_DEFAULT_VALUE;


ZB_ZCL_DECLARE_DOOR_LOCK_ATTRIB_LIST(door_lock_attr_list,
                                     &g_lock_state,
                                     &g_lock_type,
                                     &g_actuator_enabled);

/* Groups cluster attributes data */
zb_uint8_t g_attr_name_support = 0;
ZB_ZCL_DECLARE_GROUPS_ATTRIB_LIST(groups_attr_list, &g_attr_name_support);

/********************* Declare device **************************/
ZB_HA_DECLARE_DOOR_LOCK_CLUSTER_LIST(door_lock_cluster,
                                     door_lock_attr_list,
                                     basic_attr_list,
                                     identify_attr_list,
                                     groups_attr_list);

ZB_HA_DECLARE_DOOR_LOCK_EP(door_lock_ep, HA_DOOR_LOCK_ENDPOINT, door_lock_cluster);

ZB_HA_DECLARE_DOOR_LOCK_CTX(door_lock_ctx, door_lock_ep);
/** [COMMON_DECLARATION] */

MAIN()
{
    ARGV_UNUSED;

#ifndef KEIL
    if ( argc < 3 )
    {
        printf("%s <read pipe path> <write pipe path>\n", argv[0]);
        return 0;
    }
#endif

    /* Init device, load IB values from nvram or set it to default */
#ifndef ZB8051
    ZB_INIT("ha_th");

    ZB_SET_NIB_SECURITY_LEVEL(0);

    ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_TRUE_U;
    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ed_addr);

    zb_set_default_ed_descriptor_values();

    /****************** Register Device ********************************/
    /** [REGISTER] */
    ZB_AF_REGISTER_DEVICE_CTX(&door_lock_ctx);
    ZB_AF_SET_ENDPOINT_HANDLER(HA_DOOR_LOCK_ENDPOINT, zcl_specific_cluster_cmd_handler);
    /** [REGISTER] */

    ZB_SET_NIB_SECURITY_LEVEL(0);

    if ( zdo_dev_start() != RET_OK )
    {
        TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
    }
    else
    {
        zcl_main_loop();
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
    zb_zcl_attr_t        *attr_desc;
    zb_uint8_t            status      = ZB_ZCL_STATUS_SUCCESS;
    /** [VARIABLE] */

    TRACE_MSG(TRACE_ZCL1, "> zcl_specific_cluster_cmd_handler %i", (FMT__H, param));
    TRACE_MSG(TRACE_ZCL3, "payload size: %i", (FMT__D, ZB_BUF_LEN(zcl_cmd_buf)));

    if ( cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI )
    {
        TRACE_MSG(TRACE_ZCL1, "CLIENT role cluster 0x%d is not supported", (FMT__D, cmd_info->cluster_id));
    }
    else
    {
        /* Command from client to server ZB_ZCL_FRAME_DIRECTION_TO_SRV */
        /** [HANDLER] */
        switch ( cmd_info -> cluster_id )
        {
        case ZB_ZCL_CLUSTER_ID_DOOR_LOCK:
            if ( cmd_info->is_common_command )
            {
                TRACE_MSG(TRACE_ZCL2, "Skip general command %hd", (FMT__H, cmd_info->cmd_id));
            }
            else
            {

                attr_desc = zb_zcl_get_attr_desc_a(ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).dst_endpoint,
                                                   ZB_ZCL_CLUSTER_ID_DOOR_LOCK,
                                                   ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_ID);
                switch ( cmd_info->cmd_id )
                {
                case ZB_ZCL_CMD_DOOR_LOCK_LOCK_DOOR:
                    TRACE_MSG(TRACE_ZCL3, "Got cluster command 0x%04x", (FMT__D, cmd_info->cmd_id));
                    TRACE_MSG(TRACE_ZCL1, "Cmd: ZB_ZCL_CMD_DOOR_LOCK_LOCK_DOOR", (FMT__0));
                    /* Process cluster command */
                    cmd_processed = ZB_TRUE;
                    /* For user: --- Command for door lock put here --- */
                    /* For user: --- Check and change attribute value --- */
                    ZB_ZCL_SET_DIRECTLY_ATTR_VAL8(attr_desc, ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_LOCKED);

                    /** [Send Door Lock response] */
                    ZB_ZCL_DOOR_LOCK_SEND_LOCK_DOOR_RES(zcl_cmd_buf,
                                                        ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr,
                                                        ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                                        ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint,
                                                        ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).dst_endpoint,
                                                        cmd_info->profile_id,
                                                        cmd_info->seq_number,
                                                        status);
                    /** [Send Door Lock response] */
                    break;

                case ZB_ZCL_CMD_DOOR_LOCK_UNLOCK_DOOR:
                    TRACE_MSG(TRACE_ZCL1, "Cmd: ZB_ZCL_CMD_DOOR_LOCK_UNLOCK_DOOR", (FMT__0));
                    /* Process cluster command */
                    cmd_processed = ZB_TRUE;
                    /* For user: --- Command for door unlock put here --- */
                    /* For user: --- Check and change attribute value --- */
                    ZB_ZCL_SET_DIRECTLY_ATTR_VAL8(attr_desc, ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_UNLOCKED);
                    /** [Send Door Unlock response] */
                    ZB_ZCL_DOOR_LOCK_SEND_UNLOCK_DOOR_RES(zcl_cmd_buf,
                                                          ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr,
                                                          ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                                          ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint,
                                                          ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).dst_endpoint,
                                                          cmd_info->profile_id,
                                                          cmd_info->seq_number,
                                                          status);
                    /** [Send Door Unlock response] */
                    break;
                default:
                    TRACE_MSG(TRACE_ZCL2, "Cluster command %hd, skip it", (FMT__H, cmd_info->cmd_id));
                    break;
                }
            }
            break;

        default:
            TRACE_MSG(TRACE_ZCL1, "SRV role, cluster 0x%d is not supported", (FMT__D, cmd_info->cluster_id));
            break;
        }
    }
    /** [HANDLER] */

    TRACE_MSG(TRACE_ZCL1, "<< zcl_specific_cluster_cmd_handler %hd", (FMT__H, cmd_processed));
    return cmd_processed;
}

void zb_zdo_startup_complete(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);

    TRACE_MSG(TRACE_ZCL1, ">> zb_zdo_startup_complete %h", (FMT__H, param));
    if ( buf->u.hdr.status == 0 )
    {
        TRACE_MSG(TRACE_ZCL1, "Device STARTED OK", (FMT__0));
        /* It is a good place to start custom automatic activities (if any) */
    }
    else
    {
        TRACE_MSG(
            TRACE_ERROR,
            "Device started FAILED status %d",
            (FMT__D, (int)buf->u.hdr.status));
        zb_free_buf(buf);
    }
    TRACE_MSG(TRACE_ZCL1, "<< zb_zdo_startup_complete", (FMT__0));
}

#else // defined ZB_ENABLE_HA

#include <stdio.h>
    int main()
    {
        printf(" HA is not supported\n");
        return 0;
    }

#endif // defined ZB_ENABLE_HA
