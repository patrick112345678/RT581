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
/* PURPOSE: Color scene controller for ZLL profile, spec 5.3.2
*/

#define ZB_TRACE_FILE_ID 41659
#include "zll/zb_zll_common.h"

#define MY_CHANNEL 11

#if defined ZB_ENABLE_ZLL && defined ZB_ZLL_DEFINE_DEVICE_COLOR_SCENE_CONTROLLER

#if ! defined ZB_ED_ROLE
#error define ZB_ED_ROLE to compile zed tests
#endif

/** Number of the endpoint device operates on. */
#define ENDPOINT  10

/** ZCL command handler. */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);

/** Device's extended address. */
zb_ieee_addr_t g_ed_addr = {0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


/** [COMMON_DECLARATION] */
/********************* Declare device **************************/
ZB_ZLL_DECLARE_COLOR_SCENE_CONTROLLER_CLUSTER_LIST(color_scene_controller_clusters,
        ZB_ZCL_CLUSTER_MIXED_ROLE);

ZB_ZLL_DECLARE_COLOR_SCENE_CONTROLLER_EP(
    color_scene_controller_ep, ENDPOINT, color_scene_controller_clusters);

ZB_ZLL_DECLARE_COLOR_SCENE_CONTROLLER_CTX(
    color_scene_controller_ctx, color_scene_controller_ep);
/** [COMMON_DECLARATION] */

/******************* Declare router parameters *****************/
#define DST_ADDR        0x01
#define DST_ENDPOINT    5
#define DST_ADDR_MODE   ZB_APS_ADDR_MODE_16_ENDP_PRESENT
/******************* Declare test data & constants *************/


MAIN()
{
    ARGV_UNUSED;

#if ! (defined KEIL || defined ZB_PLATFORM_LINUX_ARM_2400)
    if ( argc < 3 )
    {
        printf("%s <read pipe path> <write pipe path>\n", argv[0]);
        return 0;
    }
#endif

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zed1");


    ZB_SET_NIB_SECURITY_LEVEL(0);

    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ed_addr);
    ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);

    zb_set_default_ed_descriptor_values();

    /****************** Register Device ********************************/
    /** [REGISTER] */
    /* Register device list */
    ZB_AF_REGISTER_DEVICE_CTX(&color_scene_controller_ctx);
    ZB_AF_SET_ENDPOINT_HANDLER(ENDPOINT, zcl_specific_cluster_cmd_handler);
    /** [REGISTER] */

    ZB_AIB().aps_channel_mask = 1l << MY_CHANNEL;

    ZG->nwk.nib.security_level = 0;

    if (zb_zll_dev_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "ERROR zdo_dev_start failed", (FMT__0));
    }
    else
    {
        zcl_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}

/** [HANDLER] */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
    zb_buf_t *zcl_cmd_buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
    zb_zcl_parsed_hdr_t *cmd_info = ZB_GET_BUF_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);
    zb_bool_t cmd_processed           = ZB_FALSE;

    TRACE_MSG(TRACE_ZCL1, "> zcl_specific_cluster_cmd_handler %i", (FMT__H, param));
    TRACE_MSG(TRACE_ZCL1, "payload size: %i", (FMT__D, ZB_BUF_LEN(zcl_cmd_buf)));

    if ( cmd_info -> cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI )
    {
        switch ( cmd_info -> cluster_id )
        {
        case ZB_ZCL_CLUSTER_ID_BASIC:
            if ( cmd_info -> is_common_command )
            {
                switch ( cmd_info -> cmd_id )
                {
                case ZB_ZCL_CMD_DEFAULT_RESP:
                    TRACE_MSG(TRACE_ZCL3, "Got response in cluster 0x%04x",
                              ( FMT__D, cmd_info->cluster_id));
                    /* Process default response */
                    cmd_processed = ZB_TRUE;
                    break;

                case ZB_ZCL_CMD_READ_ATTRIB_RESP:
                    cmd_processed = ZB_TRUE;
                    break;

                case ZB_ZCL_CMD_WRITE_ATTRIB_RESP:
                    cmd_processed = ZB_TRUE;
                    break;

                default:
                    TRACE_MSG(TRACE_ZCL2, "Skip general command %hd", (FMT__H, cmd_info->cmd_id));
                    break;
                }
            }
            else
            {
                switch ( cmd_info -> cmd_id )
                {
                default:
                    TRACE_MSG(TRACE_ZCL2, "Cluster command %hd, skip it", (FMT__H, cmd_info->cmd_id));
                    break;
                }
            }
            break;

        default:
            TRACE_MSG(TRACE_ZCL1, "CLNT role cluster 0x%d is not supported", (FMT__D, cmd_info->cluster_id));
            break;
        }
    }
    else
    {
        /* Command from client to server ZB_ZCL_FRAME_DIRECTION_TO_SRV */
        TRACE_MSG(TRACE_ZCL1, "SRV role, cluster 0x%d is not supported", (FMT__D, cmd_info->cluster_id));
    }

    TRACE_MSG(TRACE_ZCL1, "<< zcl_specific_cluster_cmd_handler %hd", (FMT__H, cmd_processed));
    return cmd_processed;
}
/** [HANDLER] */

void zb_zdo_startup_complete(zb_uint8_t param)
{
    zb_buf_t *buffer = ZB_BUF_FROM_REF(param);
    zb_zll_transaction_task_status_t *task_status =
        ZB_GET_BUF_PARAM(buffer, zb_zll_transaction_task_status_t);

    TRACE_MSG(TRACE_ZLL3, "> zb_zdo_startup_complete %hd status %hd", (FMT__H_H, param, task_status->task));

    switch (task_status->task)
    {
    case ZB_ZLL_DEVICE_START_TASK:
        break;

    case ZB_ZLL_TRANSACTION_NWK_START_TASK:
        TRACE_MSG(TRACE_ZLL3, "New Network status %hd", (FMT__H, task_status->status));
        if (task_status->status != ZB_ZLL_TASK_STATUS_OK)
        {
            TRACE_MSG(TRACE_ERROR, "ERROR status %hd", (FMT__H, task_status->status));
        }
        break;

    case ZB_ZLL_TRANSACTION_JOIN_ROUTER_TASK:
        TRACE_MSG(TRACE_ZLL3, "Join Router status %hd", (FMT__H, task_status->status));
        if (task_status->status != ZB_ZLL_TASK_STATUS_OK)
        {
            TRACE_MSG(TRACE_ERROR, "ERROR status %hd", (FMT__H, task_status->status));
        }
        break;

    default:
        TRACE_MSG(TRACE_ERROR, "ERROR unsupported task %hd", (FMT__H, task_status->task));
        break;
    }

    TRACE_MSG(TRACE_ZLL3, "< zb_zdo_startup_complete", (FMT__0));
}/* void zll_task_state_changed(zb_uint8_t param) */

#else // defined ZB_ENABLE_ZLL

#include <stdio.h>
int main()
{
    printf(" ZLL is not supported\n");
    return 0;
}

#endif // defined ZB_ENABLE_ZLL
