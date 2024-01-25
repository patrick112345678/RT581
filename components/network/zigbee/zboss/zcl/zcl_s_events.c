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
/* PURPOSE: SERVER: Events cluster implementation.
*/

#define ZB_TRACE_FILE_ID 69

#include "zb_common.h"

#if defined (ZB_ZCL_SUPPORT_CLUSTER_EVENTS) || defined DOXYGEN

#include "zboss_api.h"
#include "zcl/zb_zcl_events.h"

zb_uint8_t gs_events_server_received_commands[] =
{
    ZB_ZCL_CLUSTER_ID_EVENTS_SERVER_ROLE_RECEIVED_CMD_LIST
};

zb_uint8_t gs_events_server_generated_commands[] =
{
    ZB_ZCL_CLUSTER_ID_EVENTS_SERVER_ROLE_GENERATED_CMD_LIST
};

zb_discover_cmd_list_t gs_events_server_cmd_list =
{
    sizeof(gs_events_server_received_commands), gs_events_server_received_commands,
    sizeof(gs_events_server_generated_commands), gs_events_server_generated_commands
};

/* 2017/08/23 NK:MEDIUM Do not put the code under ifdef - may return ZB_FALSE indicating that
 * command is not processed. */
/** DD:Fixed
 * Comment: Discuss with NK that it doesn't make a sence to add follow code
 * under #ifdef block. This functions should return default values to indicate
 * that commands are not handled or processed.
 */

/* FUNCTIONS FOR SERVER */

/*Handle get event log request from client*/
zb_ret_t zb_zcl_events_server_handle_get_event_log(zb_uint8_t param, const zb_zcl_parsed_hdr_t *cmd_info)
{
    /*TODO: need to implement*/
    ZVUNUSED(param);
    ZVUNUSED(cmd_info);
    return RET_OK;
}


/*Handle clear event log request from client*/
zb_ret_t zb_zcl_events_server_handle_clear_event_log_request(zb_uint8_t param, const zb_zcl_parsed_hdr_t *cmd_info)
{
    /*TODO: need to implement*/
    ZVUNUSED(param);
    ZVUNUSED(cmd_info);
    return RET_OK;
}


void zb_zcl_events_server_send_publish_event(zb_uint8_t param, zb_addr_u *dst_addr,
        zb_uint8_t dst_addr_mode, zb_uint8_t dst_ep,
        zb_uint8_t src_ep, zb_zcl_events_publish_event_payload_t *payload)
{
    /*TODO: need to implement*/;
    ZVUNUSED(param);
    ZVUNUSED(dst_addr);
    ZVUNUSED(dst_addr_mode);
    ZVUNUSED(dst_ep);
    ZVUNUSED(src_ep);
    ZVUNUSED(payload);
}


void zb_zcl_events_server_send_publish_event_log(zb_uint8_t param, zb_addr_u *dst_addr,
        zb_uint8_t dst_addr_mode, zb_uint8_t dst_ep,
        zb_uint8_t src_ep, zb_zcl_events_publish_event_log_payload_t *payload)
{
    /*TODO: need to implement*/;
    ZVUNUSED(param);
    ZVUNUSED(dst_addr);
    ZVUNUSED(dst_addr_mode);
    ZVUNUSED(dst_ep);
    ZVUNUSED(src_ep);
    ZVUNUSED(payload);
}


void zb_zcl_events_server_send_clear_event_log_response(zb_uint8_t param, zb_addr_u *dst_addr,
        zb_uint8_t dst_addr_mode, zb_uint8_t dst_ep,
        zb_uint8_t src_ep, zb_uint8_t *payload) /*zb_uint8_t zb_zcl_events_ClearedEventsLogs;*/
{
    /*TODO: need to implement*/;
    ZVUNUSED(param);
    ZVUNUSED(dst_addr);
    ZVUNUSED(dst_addr_mode);
    ZVUNUSED(dst_ep);
    ZVUNUSED(src_ep);
    ZVUNUSED(payload);
}


/** Funciton to process particular server commands
 * @param - pointer to buffer
 * @cmd_info - pointer to parsed zcl header
 * @return ZB_TRUE or ZB_FALSE
 */
static zb_bool_t zb_zcl_process_events_server_commands(zb_uint8_t param, const zb_zcl_parsed_hdr_t *cmd_info)
{
    zb_bool_t processed = ZB_FALSE;
    zb_ret_t result = RET_ERROR;
    ZVUNUSED(result);

    switch ((zb_zcl_events_cli_cmd_t)cmd_info->cmd_id)
    {
    case ZB_ZCL_EVENTS_CLI_CMD_GET_EVENT_LOG:
        /* 2017/08/23 NK:MEDIUM Uncomment but return that command is not processed. */
        /* DD:Fixed */
        result = zb_zcl_events_server_handle_get_event_log(param, cmd_info);
        break;
    case ZB_ZCL_EVENTS_CLI_CMD_CLEAR_EVENT_LOG_REQUEST:
        result = zb_zcl_events_server_handle_clear_event_log_request(param, cmd_info);
        break;
    default:
        break;
    }
    return processed;
}


/*Function to choose one of command sets: client commands or server commands*/
zb_bool_t zb_zcl_process_s_events_specific_commands(zb_uint8_t param)
{
    zb_zcl_parsed_hdr_t cmd_info;

    TRACE_MSG(TRACE_ZCL1, ">> zb_zcl_process_s_events_specific_commands", (FMT__0));

    if ( ZB_ZCL_GENERAL_GET_CMD_LISTS_PARAM == param )
    {
        ZCL_CTX().zb_zcl_cluster_cmd_list = &gs_events_server_cmd_list;
        return ZB_TRUE;
    }

    ZB_ZCL_COPY_PARSED_HEADER(param, &cmd_info);

    ZB_ASSERT(ZB_ZCL_CLUSTER_ID_EVENTS == cmd_info.cluster_id);

    /* 2017/08/23 NK:MEDIUM Remove - we put this check in zcl_common for all clusters. */
    /* DD:Fixed */
    TRACE_MSG(TRACE_ZCL1, "<< zb_zcl_process_s_events_specific_commands", (FMT__0));

    if (ZB_ZCL_FRAME_DIRECTION_TO_SRV == cmd_info.cmd_direction)
    {
        return zb_zcl_process_events_server_commands(param, &cmd_info);
    }
    return ZB_FALSE;
}

void zb_zcl_events_init_server()
{
    zb_zcl_add_cluster_handlers(ZB_ZCL_CLUSTER_ID_EVENTS,
                                ZB_ZCL_CLUSTER_SERVER_ROLE,
                                (zb_zcl_cluster_check_value_t)NULL,
                                (zb_zcl_cluster_write_attr_hook_t)NULL,
                                zb_zcl_process_s_events_specific_commands);
}

#endif /* ZB_ZCL_SUPPORT_CLUSTER_EVENTS || defined DOXYGEN */
