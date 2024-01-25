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
/* PURPOSE: CLIENT: ZCL Tunneling cluster, purpose: general data tunneling.
*/

#define ZB_TRACE_FILE_ID 84

#include "zb_common.h"

#if defined (ZB_ZCL_SUPPORT_CLUSTER_TUNNELING) || defined DOXYGEN

#include "zcl/zb_zcl_tunneling.h"

/* WARNING: Assume one tunnel on client and multiple tunnels on server.
   It is not clearly described how much tunnels may be on the client and on the server, but
   according to the ZCL spec - 10.6.1 Overview:

   Client: Requests a tunnel from the server and closes the tunnel if it is no longer needed.
   Server: Provides and manages tunnels to the clients.

   looks like client supports only one tunnel to one server.
 */

/* TODO: Not implemented:
   - MaxIncomingTransferSize handling (for example, what to do if it is too small?)
 */

zb_uint8_t gs_tunneling_client_received_commands[] =
{
    ZB_ZCL_CLUSTER_ID_TUNNELING_CLIENT_ROLE_RECEIVED_CMD_LIST
};

zb_uint8_t gs_tunneling_client_generated_commands[] =
{
    ZB_ZCL_CLUSTER_ID_TUNNELING_CLIENT_ROLE_GENERATED_CMD_LIST
};

zb_discover_cmd_list_t gs_tunneling_client_cmd_list =
{
    sizeof(gs_tunneling_client_received_commands), gs_tunneling_client_received_commands,
    sizeof(gs_tunneling_client_generated_commands), gs_tunneling_client_generated_commands
};

typedef struct zb_zcl_tunneling_ctx_s
{
    zb_zcl_tunneling_cli_t cli;
} zb_zcl_tunneling_ctx_t;

static zb_zcl_tunneling_ctx_t zcl_tun_ctx;

zb_bool_t zb_zcl_tunneling_client_request_tunnel_response_handle(zb_uint8_t param, zb_zcl_parsed_hdr_t *cmd_info)
{
    zb_zcl_tunneling_request_tunnel_response_t request_tunnel_resp;
    zb_uint8_t parse_status = ZB_ZCL_PARSE_STATUS_FAILURE;
    zb_ret_t ret = RET_ERROR;
    ZVUNUSED(cmd_info);

    ZB_ZCL_TUNNELING_GET_REQUEST_TUNNEL_RESPONSE(&request_tunnel_resp, param, parse_status);

    if (parse_status == ZB_ZCL_PARSE_STATUS_SUCCESS)
    {
        if (ZCL_CTX().device_cb)
        {
            ZB_ZCL_DEVICE_CMD_PARAM_INIT_WITH(param,
                                              ZB_ZCL_TUNNELING_REQUEST_TUNNEL_RESPONSE_CB_ID,
                                              RET_OK,
                                              cmd_info,
                                              &request_tunnel_resp,
                                              NULL);
            zcl_tun_ctx.cli.dst_addr = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr;
            zcl_tun_ctx.cli.dst_ep = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint;
            zcl_tun_ctx.cli.tunnel_id = request_tunnel_resp.tunnel_id;
            TRACE_MSG(TRACE_ZCL1, "cli: remember tunnel_id %d addr %d ep %hd", (FMT__D_D_H, zcl_tun_ctx.cli.tunnel_id, zcl_tun_ctx.cli.dst_addr, zcl_tun_ctx.cli.dst_ep));
            zcl_tun_ctx.cli.max_outgoing_to_srv_transfer_size =
                request_tunnel_resp.max_incoming_transfer_size;

            ZCL_CTX().device_cb(param);
        }

        ret = RET_OK;
    }

    zb_zcl_send_default_handler(param, cmd_info,
                                ret == RET_OK ? ZB_ZCL_STATUS_SUCCESS : ZB_ZCL_STATUS_FAIL);

    return ZB_TRUE;
}

zb_bool_t zb_zcl_tunneling_client_transfer_data_handle(zb_uint8_t param, zb_zcl_parsed_hdr_t *cmd_info)
{
    zb_zcl_tunneling_transfer_data_payload_t tr_data;
    zb_uint8_t parse_status = ZB_ZCL_PARSE_STATUS_FAILURE;
    zb_uint8_t transfer_data_status = ZB_ZCL_TUNNELING_TRANSFER_DATA_STATUS_NO_SUCH_TUNNEL;

    ZB_ZCL_TUNNELING_GET_TRANSFER_DATA(&tr_data, param, parse_status);

    if (parse_status == ZB_ZCL_PARSE_STATUS_SUCCESS)
    {
        /* CLI: Check existing tunnel paremeters */
        if (zcl_tun_ctx.cli.tunnel_id == tr_data.hdr.tunnel_id)
        {
            transfer_data_status = ZB_ZCL_TUNNELING_TRANSFER_DATA_STATUS_OK;

            if (ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr != zcl_tun_ctx.cli.dst_addr ||
                    ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint != zcl_tun_ctx.cli.dst_ep)
            {
                transfer_data_status = ZB_ZCL_TUNNELING_TRANSFER_DATA_STATUS_WRONG_DEVICE;
            }
            else if (tr_data.data_size > zcl_tun_ctx.cli.max_incoming_to_cli_transfer_size)
            {
                transfer_data_status = ZB_ZCL_TUNNELING_TRANSFER_DATA_STATUS_DATA_OVERFLOW;
            }
        }

        if (transfer_data_status == ZB_ZCL_TUNNELING_TRANSFER_DATA_STATUS_OK && ZCL_CTX().device_cb)
        {
            ZB_ZCL_DEVICE_CMD_PARAM_INIT_WITH(
                param,
                ZB_ZCL_TUNNELING_TRANSFER_DATA_SRV_CB_ID,
                RET_OK,
                cmd_info,
                &tr_data,
                NULL);

            ZCL_CTX().device_cb(param);

            if (ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) != RET_OK)
            {
                transfer_data_status = ZB_ZCL_TUNNELING_TRANSFER_DATA_INTERNAL_ERROR;
            }
        }

        if (transfer_data_status != ZB_ZCL_TUNNELING_TRANSFER_DATA_STATUS_OK)
        {
            ZB_ZCL_TUNNELING_SEND_TRANSFER_DATA_ERROR(param,
                    ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr,
                    ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                    ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint,
                    ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).dst_endpoint,
                    cmd_info->profile_id,
                    ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                    NULL,                     /* no callback */
                    cmd_info->seq_number,
                    tr_data.hdr.tunnel_id,
                    transfer_data_status,
                    ZB_ZCL_TUNNELING_CLI_CMD_TRANSFER_DATA_ERROR,
                    ZB_ZCL_FRAME_DIRECTION_TO_SRV);
            param = 0;
        }
    }

    /* Do not send Default Response on Transfer Data - have optional Ack cmd. */

    if (param)
    {
        zb_buf_free(param);
    }

    return ZB_TRUE;
}


zb_bool_t zb_zcl_tunneling_client_transfer_data_error_handle(zb_uint8_t param, zb_zcl_parsed_hdr_t *cmd_info)
{
    zb_zcl_tunneling_transfer_data_error_t tr_error;
    zb_uint8_t parse_status = ZB_ZCL_PARSE_STATUS_FAILURE;
    zb_ret_t ret = RET_ERROR;

    ZB_ZCL_TUNNELING_GET_TRANSFER_DATA_ERROR(&tr_error, param, parse_status);

    if (parse_status == ZB_ZCL_PARSE_STATUS_SUCCESS)
    {
        /* CLI: Check existing tunnel paremeters */
        if (zcl_tun_ctx.cli.tunnel_id == tr_error.tunnel_id
                && zcl_tun_ctx.cli.dst_addr == ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr
                && zcl_tun_ctx.cli.dst_ep == ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint)
        {
            ret = RET_OK;
        }

        if (ret == RET_OK && ZCL_CTX().device_cb)
        {
            ZB_ZCL_DEVICE_CMD_PARAM_INIT_WITH(
                param,
                ZB_ZCL_TUNNELING_TRANSFER_DATA_ERROR_SRV_CB_ID,
                RET_OK,
                cmd_info,
                &tr_error,
                NULL);

            ZCL_CTX().device_cb(param);
        }
    }

    /* Do not send Default Response on Transfer Data Error - client may send Close Tunnel. */

    zb_buf_free(param);

    return ZB_TRUE;
}

zb_bool_t zb_zcl_process_c_tunneling_specific_commands(zb_uint8_t param)
{
    zb_bool_t processed = ZB_FALSE;
    zb_zcl_parsed_hdr_t cmd_info;

    if ( ZB_ZCL_GENERAL_GET_CMD_LISTS_PARAM == param )
    {
        ZCL_CTX().zb_zcl_cluster_cmd_list = &gs_tunneling_client_cmd_list;
        return ZB_TRUE;
    }

    ZB_ZCL_COPY_PARSED_HEADER(param, &cmd_info);

    TRACE_MSG( TRACE_ZCL1,
               "> zb_zcl_process_c_tunneling_specific_commands: param %hd, cmd %hd",
               (FMT__H_H, param, cmd_info.cmd_id));

    ZB_ASSERT(cmd_info.cluster_id == ZB_ZCL_CLUSTER_ID_TUNNELING);

    if (cmd_info.cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI)
    {
        switch (cmd_info.cmd_id)
        {
        case ZB_ZCL_TUNNELING_SRV_CMD_REQUEST_TUNNEL_RESPONSE:
            TRACE_MSG(TRACE_ZCL1, "ZB_ZCL_TUNNELING_SRV_CMD_REQUEST_TUNNEL_RESPONSE", (FMT__0));
            processed = zb_zcl_tunneling_client_request_tunnel_response_handle(param, &cmd_info);
            break;

        case ZB_ZCL_TUNNELING_SRV_CMD_TRANSFER_DATA:
            TRACE_MSG(TRACE_ZCL1, "ZB_ZCL_TUNNELING_SRV_CMD_TRANSFER_DATA", (FMT__0));
            processed = zb_zcl_tunneling_client_transfer_data_handle(param, &cmd_info);
            break;

        case ZB_ZCL_TUNNELING_SRV_CMD_TRANSFER_DATA_ERROR:
            TRACE_MSG(TRACE_ZCL1, "ZB_ZCL_TUNNELING_SRV_CMD_TRANSFER_DATA_ERROR", (FMT__0));
            processed = zb_zcl_tunneling_client_transfer_data_error_handle(param, &cmd_info);
            break;

        /* TODO: Handle other commands. */

        default:
            break;
        }
    }

    TRACE_MSG( TRACE_ZCL1,
               "< zb_zcl_process_c_tunneling_specific_commands: processed %d",
               (FMT__D, processed));
    return processed;
}

zb_ret_t zb_zcl_tunneling_client_send_transfer_data(zb_uint8_t param, zb_uint8_t ep, zb_uint16_t prfl_id,
        zb_uint8_t def_resp, zb_callback_t cb, zb_uint16_t tunnel_id,
        zb_uint16_t data_size, zb_uint8_t *image_data)
{
    zb_ret_t ret = RET_ERROR;
    zb_bufid_t buffer = param;
    zb_uint16_t dst_addr = ZB_UNKNOWN_SHORT_ADDR;
    zb_uint8_t dst_ep = 0;

    TRACE_MSG(TRACE_ZCL1, "zb_zcl_tunneling_send_transfer_data param %hd tunnel_id %d", (FMT__H_D, param, tunnel_id));

    if (zcl_tun_ctx.cli.tunnel_id != tunnel_id)
    {
        TRACE_MSG(TRACE_ERROR, "transfer data failed - no tunnel with this id", (FMT__0));
    }
    else if (data_size > zcl_tun_ctx.cli.max_outgoing_to_srv_transfer_size)
    {
        TRACE_MSG(TRACE_ERROR, "transfer data failed - too much data size", (FMT__0));
    }
    else
    {
        dst_addr = zcl_tun_ctx.cli.dst_addr;
        dst_ep = zcl_tun_ctx.cli.dst_ep;

        if (dst_addr != ZB_UNKNOWN_SHORT_ADDR && dst_ep)
        {
            /* Both Transfer Data (from cli and from srv) are requests, right? */
            zb_uint8_t *ptr = zb_zcl_start_command_header(buffer,
                              ZB_ZCL_CONSTRUCT_FRAME_CONTROL(ZB_ZCL_FRAME_TYPE_CLUSTER_SPECIFIC,
                                      ZB_ZCL_NOT_MANUFACTURER_SPECIFIC,
                                      ZB_ZCL_FRAME_DIRECTION_TO_SRV,
                                      def_resp),
                              0, ZB_ZCL_TUNNELING_CLI_CMD_TRANSFER_DATA, NULL);
            ZB_ZCL_PACKET_PUT_DATA16_VAL(ptr, tunnel_id);
            if (data_size > 0)
            {
                ZB_ZCL_PACKET_PUT_DATA_N(ptr, image_data, data_size);
            }
            ZB_ZCL_FINISH_N_SEND_PACKET(buffer, ptr,
                                        dst_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, dst_ep, ep, prfl_id,
                                        ZB_ZCL_CLUSTER_ID_TUNNELING, cb);
            ret = RET_OK;
        }
    }

    return ret;
}


void zb_zcl_c_tunneling_init()
{
    ZB_BZERO(&zcl_tun_ctx, sizeof(zb_zcl_tunneling_ctx_t));
    zcl_tun_ctx.cli.max_incoming_to_cli_transfer_size = ZB_ZCL_TUNNELING_MAX_INCOMING_TRANSFER_SIZE;
}


void zb_zcl_tunneling_init_client()
{
    if (zb_zcl_add_cluster_handlers(ZB_ZCL_CLUSTER_ID_TUNNELING,
                                    ZB_ZCL_CLUSTER_CLIENT_ROLE,
                                    (zb_zcl_cluster_check_value_t)NULL,
                                    (zb_zcl_cluster_write_attr_hook_t)NULL,
                                    zb_zcl_process_c_tunneling_specific_commands) == RET_OK)
    {
        zb_zcl_c_tunneling_init();
    }
}


void zb_zcl_tunneling_set_max_incoming_to_cli_transfer_size(zb_uint16_t transfer_size)
{
    zcl_tun_ctx.cli.max_incoming_to_cli_transfer_size = transfer_size;
}

#endif /* ZB_ZCL_SUPPORT_CLUSTER_TUNNELING || defined DOXYGEN */
