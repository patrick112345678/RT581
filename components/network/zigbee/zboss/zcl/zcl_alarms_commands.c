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
/* PURPOSE: ZCL Alarms cluster specific commands handling
*/

#define ZB_TRACE_FILE_ID 2059

#include "zb_common.h"

#if defined (ZB_ZCL_SUPPORT_CLUSTER_ALARMS)

#include "zb_zcl.h"
#include "zcl/zb_zcl_alarms.h"

static zb_uint8_t gs_alarms_client_received_commands[] =
{
    ZB_ZCL_CLUSTER_ID_ALARMS_CLIENT_ROLE_RECEIVED_CMD_LIST
};

static zb_uint8_t gs_alarms_server_received_commands[] =
{
    ZB_ZCL_CLUSTER_ID_ALARMS_SERVER_ROLE_RECEIVED_CMD_LIST
};

static zb_discover_cmd_list_t gs_alarms_client_cmd_list =
{
    sizeof(gs_alarms_client_received_commands), gs_alarms_client_received_commands,
    sizeof(gs_alarms_server_received_commands), gs_alarms_server_received_commands
};

static zb_discover_cmd_list_t gs_alarms_server_cmd_list =
{
    sizeof(gs_alarms_server_received_commands), gs_alarms_server_received_commands,
    sizeof(gs_alarms_client_received_commands), gs_alarms_client_received_commands
};

zb_bool_t zb_zcl_process_alarms_specific_commands_srv(zb_uint8_t param);
zb_bool_t zb_zcl_process_alarms_specific_commands_cli(zb_uint8_t param);

void zb_zcl_alarms_init_server()
{
    zb_zcl_add_cluster_handlers(ZB_ZCL_CLUSTER_ID_ALARMS,
                                ZB_ZCL_CLUSTER_SERVER_ROLE,
                                (zb_zcl_cluster_check_value_t)NULL,
                                (zb_zcl_cluster_write_attr_hook_t)NULL,
                                zb_zcl_process_alarms_specific_commands_srv);
}

void zb_zcl_alarms_init_client()
{
    zb_zcl_add_cluster_handlers(ZB_ZCL_CLUSTER_ID_ALARMS,
                                ZB_ZCL_CLUSTER_CLIENT_ROLE,
                                (zb_zcl_cluster_check_value_t)NULL,
                                (zb_zcl_cluster_write_attr_hook_t)NULL,
                                zb_zcl_process_alarms_specific_commands_cli);
}

static zb_zcl_status_t zb_zcl_alarms_map_ret_code_to_zcl_status(zb_ret_t ret_code)
{
    zb_zcl_status_t status;

    ZB_ASSERT(ret_code != RET_BUSY);

    switch (ret_code)
    {
    case RET_OK:
        status = ZB_ZCL_STATUS_SUCCESS;
        break;
    case RET_INVALID_PARAMETER_1:
        status = ZB_ZCL_STATUS_INVALID_FIELD;
        break;
    case RET_INVALID_PARAMETER:
        status = ZB_ZCL_STATUS_INVALID_VALUE;
        break;
    case RET_ERROR:
        status = ZB_ZCL_STATUS_HW_FAIL;
        break;
    default:
        status = ZB_ZCL_STATUS_FAIL;
        break;
    }

    return status;
}

static zb_ret_t zb_zcl_alarms_reset_alarm_handler(zb_uint8_t param, const zb_zcl_parsed_hdr_t *cmd_info)
{
    zb_ret_t ret = RET_OK;
    zb_zcl_alarms_reset_alarm_req_t pl_in;
    zb_zcl_parse_status_t status;

    TRACE_MSG(TRACE_ZCL1, "> zb_zcl_alarms_reset_alarm_handler %hx", (FMT__H, param));

    ZB_BZERO(&pl_in, sizeof(pl_in));

    ZB_ZCL_ALARMS_GET_RESET_ALARM_REQ(param, pl_in, status);

    if (status != ZB_ZCL_PARSE_STATUS_SUCCESS)
    {
        TRACE_MSG(TRACE_ZCL1, "Error payload of ZB_ZCL_ALARMS_GET_RESET_ALARM_REQ", (FMT__0));

        ret = RET_INVALID_PARAMETER_1;
    }
    else
    {
        ZB_ZCL_DEVICE_CMD_PARAM_INIT_WITH(param,
                                          ZB_ZCL_ALARMS_RESET_ALARM_CB_ID, RET_OK, cmd_info, &pl_in, NULL);

        if (ZCL_CTX().device_cb != NULL)
        {
            (ZCL_CTX().device_cb)(param);
        }

        ret = (ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) == RET_OK ? RET_OK : RET_ERROR);
    }

    TRACE_MSG(TRACE_ZCL1, "< zb_zcl_alarms_reset_alarm_handler ret %hx", (FMT__H, ret));

    return ret;
}

static zb_ret_t zb_zcl_alarms_reset_all_alarms_handler(zb_uint8_t param, const zb_zcl_parsed_hdr_t *cmd_info)
{
    zb_ret_t ret = RET_OK;

    TRACE_MSG(TRACE_ZCL1, "> zb_zcl_alarms_reset_all_alarms_handler %hx", (FMT__H, param));

    ZB_ZCL_DEVICE_CMD_PARAM_INIT_WITH(param,
                                      ZB_ZCL_ALARMS_RESET_ALL_ALARMS_CB_ID, RET_OK, cmd_info, NULL, NULL);

    if (ZCL_CTX().device_cb != NULL)
    {
        (ZCL_CTX().device_cb)(param);
    }

    ret = (ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) == RET_OK ? RET_OK : RET_ERROR);

    TRACE_MSG(TRACE_ZCL1, "< zb_zcl_alarms_reset_all_alarms_handler ret %hx", (FMT__H, ret));

    return ret;
}

static zb_ret_t zb_zcl_alarms_alarm_handler(zb_uint8_t param, const zb_zcl_parsed_hdr_t *cmd_info)
{
    zb_ret_t ret = RET_OK;
    zb_zcl_alarms_alarm_res_t pl_in;
    zb_zcl_parse_status_t status;

    TRACE_MSG(TRACE_ZCL1, "> zb_zcl_alarms_alarm_handler %hx", (FMT__H, param));

    ZB_BZERO(&pl_in, sizeof(pl_in));

    ZB_ZCL_ALARMS_GET_ALARM_RES(&pl_in, param, status);

    if (status != ZB_ZCL_PARSE_STATUS_SUCCESS)
    {
        TRACE_MSG(TRACE_ZCL1, "Error payload of ZB_ZCL_ALARMS_GET_ALARM_RES", (FMT__0));

        ret = RET_INVALID_PARAMETER_1;
    }
    else
    {
        ZB_ZCL_DEVICE_CMD_PARAM_INIT_WITH(param,
                                          ZB_ZCL_ALARMS_ALARM_CB_ID, RET_OK, cmd_info, &pl_in, NULL);

        if (ZCL_CTX().device_cb != NULL)
        {
            (ZCL_CTX().device_cb)(param);
        }

        ret = (ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) == RET_OK ? RET_OK : RET_ERROR);
    }

    TRACE_MSG(TRACE_ZCL1, "< zb_zcl_alarms_alarm_handler ret %hx", (FMT__H, ret));

    return ret;
}

zb_bool_t zb_zcl_process_alarms_specific_commands_srv(zb_uint8_t param)
{
    zb_ret_t ret = RET_OK;
    zb_bool_t processed = ZB_TRUE;
    zb_zcl_parsed_hdr_t cmd_info;

    if ( ZB_ZCL_GENERAL_GET_CMD_LISTS_PARAM == param )
    {
        ZCL_CTX().zb_zcl_cluster_cmd_list = &gs_alarms_server_cmd_list;
        return ZB_TRUE;
    }

    ZB_ZCL_COPY_PARSED_HEADER(param, &cmd_info);

    TRACE_MSG( TRACE_ZCL1,
               "> zb_zcl_process_alarms_specific_commands_srv: param %hd, cmd %hd",
               (FMT__H_H, param, cmd_info.cmd_id));

    ZB_ASSERT(ZB_ZCL_CLUSTER_ID_ALARMS == cmd_info.cluster_id);
    ZB_ASSERT(ZB_ZCL_FRAME_DIRECTION_TO_SRV == cmd_info.cmd_direction);

    switch (cmd_info.cmd_id)
    {
    case ZB_ZCL_CMD_ALARMS_RESET_ALARM_ID:
        ret = zb_zcl_alarms_reset_alarm_handler(param, &cmd_info);
        TRACE_MSG(TRACE_ZCL3, "Processed RESET_ALARM command", (FMT__0));
        break;

    case ZB_ZCL_CMD_ALARMS_RESET_ALL_ALARMS_ID:
        ret = zb_zcl_alarms_reset_all_alarms_handler(param, &cmd_info);
        TRACE_MSG(TRACE_ZCL3, "Processed RESET_ALL_ALARMS command", (FMT__0));
        break;

    default:
        processed = ZB_FALSE;
        break;
    }

    if (processed && ret != RET_BUSY)
    {
        zb_zcl_send_default_handler(param, &cmd_info,
                                    zb_zcl_alarms_map_ret_code_to_zcl_status(ret));
    }

    TRACE_MSG(TRACE_ZCL1,
              "< zb_zcl_process_alarms_specific_commands_srv: processed %d",
              (FMT__D, processed));

    return processed;
}

zb_bool_t zb_zcl_process_alarms_specific_commands_cli(zb_uint8_t param)
{
    zb_ret_t ret = RET_OK;
    zb_bool_t processed = ZB_TRUE;
    zb_zcl_parsed_hdr_t cmd_info;

    if ( ZB_ZCL_GENERAL_GET_CMD_LISTS_PARAM == param )
    {
        ZCL_CTX().zb_zcl_cluster_cmd_list = &gs_alarms_client_cmd_list;
        return ZB_TRUE;
    }

    ZB_ZCL_COPY_PARSED_HEADER(param, &cmd_info);

    TRACE_MSG( TRACE_ZCL1,
               "> zb_zcl_process_alarms_specific_commands_cli: param %hd, cmd %hd",
               (FMT__H_H, param, cmd_info.cmd_id));

    ZB_ASSERT(ZB_ZCL_CLUSTER_ID_ALARMS == cmd_info.cluster_id);
    ZB_ASSERT(ZB_ZCL_FRAME_DIRECTION_TO_CLI == cmd_info.cmd_direction);

    switch (cmd_info.cmd_id)
    {
    case ZB_ZCL_CMD_ALARMS_ALARM_ID:
        ret = zb_zcl_alarms_alarm_handler(param, &cmd_info);
        TRACE_MSG(TRACE_ZCL3, "Processed ALARM command", (FMT__0));
        break;

    default:
        processed = ZB_FALSE;
        break;
    }

    if (processed && ret != RET_BUSY)
    {
        zb_zcl_send_default_handler(param, &cmd_info,
                                    zb_zcl_alarms_map_ret_code_to_zcl_status(ret));
    }

    TRACE_MSG(TRACE_ZCL1,
              "< zb_zcl_process_alarms_specific_commands_cli: processed %d",
              (FMT__D, processed));

    return processed;
}
#endif /* defined ZB_ZCL_SUPPORT_CLUSTER_ALARMS*/
