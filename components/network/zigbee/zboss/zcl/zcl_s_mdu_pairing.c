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
/* PURPOSE: SERVER: ZCL MDU Pairing cluster
         Allow devices joining the NAN to acquire a list of the devices
         forming the 'virtual HAN' for the respective household.
*/

#define ZB_TRACE_FILE_ID 83

#include "zb_common.h"

#if defined (ZB_ZCL_SUPPORT_CLUSTER_MDU_PAIRING) || defined DOXYGEN

#include "zcl/zb_zcl_mdu_pairing.h"

#define ZB_ZCL_MDU_PAIRING_MAX_PAYLOAD_FOR_TRANSFER 50
#define ZB_ZCL_MDU_PAIRING_HAN_TABLE_ONE_TRANSFER_LIMIT 5

zb_uint8_t gs_server_received_commands[] =
{
  ZB_ZCL_CLUSTER_ID_MDU_PAIRING_SERVER_ROLE_RECEIVED_CMD_LIST
};

zb_uint8_t gs_server_generated_commands[] =
{
  ZB_ZCL_CLUSTER_ID_MDU_PAIRING_SERVER_ROLE_GENERATED_CMD_LIST
};

zb_discover_cmd_list_t gs_server_cmd_list =
{
  sizeof(gs_server_received_commands), gs_server_received_commands,
  sizeof(gs_server_generated_commands), gs_server_generated_commands
};

enum {
  ZB_ZCL_MDU_PAIRING_STATE_READY = 0,
  ZB_ZCL_MDU_PAIRING_STATE_RESPONSE_IN_PROGRESS,
  ZB_ZCL_MDU_PAIRING_STATE_RESPONSE_RECEIVED
};

typedef struct mdu_pairing_table_s
{
  zb_uint8_t cmd_transferred;
  zb_uint8_t cmd_total;
  zb_ieee_addr_t partner;
  zb_addr_u dst_addr;
  zb_uint8_t  src_endpoint;
  zb_uint8_t  dst_endpoint;
} mdu_pairing_table_t;

typedef struct zb_zcl_mdu_srv_ctx_s
{
  zb_uint32_t pairing_version;
  zb_uint8_t virtual_han_size;
  zb_ieee_addr_t *virtual_han_table;
  zb_uint8_t current_state;
  zb_uint8_t tsn;
  mdu_pairing_table_t ctx;
} zb_zcl_mdu_srv_ctx_t;

zb_zcl_mdu_srv_ctx_t zcl_mdu_srv_ctx;

/* convert payload into raw data */

static inline const zb_uint8_t *zb_zcl_mdu_pairing_request_get_payload_from_data(
  zb_zcl_mdu_pairing_request_t *payload, const zb_uint8_t *data)
{
  ZB_ZCL_PACKET_GET_DATA32(&payload->lpi_version, data);
  ZB_ZCL_PACKET_GET_DATA64(&payload->eui64, data);
  return data;
}

void zb_zcl_mdu_internal_cmd_server_send_response(zb_uint8_t param)
{
  ZB_ASSERT(param);

  TRACE_MSG(TRACE_APP1, ">> zb_zcl_mdu_internal_cmd_server_send_response: param %hd", (FMT__H, param));

  if(zcl_mdu_srv_ctx.current_state == ZB_ZCL_MDU_PAIRING_STATE_RESPONSE_IN_PROGRESS)
  {
    zb_uint8_t payload[ZB_ZCL_MDU_PAIRING_MAX_PAYLOAD_FOR_TRANSFER];
    zb_zcl_mdu_pairing_response_t *pl = (zb_zcl_mdu_pairing_response_t *)payload;
    zb_uint8_t pl_size, count = ZB_ZCL_MDU_PAIRING_HAN_TABLE_ONE_TRANSFER_LIMIT;
    zb_ieee_addr_t *src_eui, *dst_eui;
    //send framed actual MDU list

    ZB_HTOLE32(&pl->lpi_version, &zcl_mdu_srv_ctx.pairing_version);
    pl->total_number_of_devices = zcl_mdu_srv_ctx.virtual_han_size;

    pl->command_index = zcl_mdu_srv_ctx.ctx.cmd_transferred;
    pl->total_number_of_commands = zcl_mdu_srv_ctx.ctx.cmd_total;

    dst_eui = (zb_ieee_addr_t *)&pl->eui64;
    src_eui = ((zb_ieee_addr_t *)zcl_mdu_srv_ctx.virtual_han_table + pl->command_index*ZB_ZCL_MDU_PAIRING_HAN_TABLE_ONE_TRANSFER_LIMIT);

    if((zcl_mdu_srv_ctx.ctx.cmd_total-zcl_mdu_srv_ctx.ctx.cmd_transferred)==1)
      count = (zcl_mdu_srv_ctx.virtual_han_size-
               zcl_mdu_srv_ctx.ctx.cmd_transferred*ZB_ZCL_MDU_PAIRING_HAN_TABLE_ONE_TRANSFER_LIMIT
               )%ZB_ZCL_MDU_PAIRING_HAN_TABLE_ONE_TRANSFER_LIMIT;

    ZB_MEMCPY( dst_eui, src_eui, sizeof(zb_ieee_addr_t)*count );

    zcl_mdu_srv_ctx.ctx.cmd_transferred++;

    pl_size = sizeof(zb_zcl_mdu_pairing_response_t)-sizeof(void *)-sizeof(zb_uint8_t)+sizeof(zb_ieee_addr_t)*count;

    ZB_ASSERT( pl_size <= ZB_ZCL_MDU_PAIRING_MAX_PAYLOAD_FOR_TRANSFER );

    if (zcl_mdu_srv_ctx.ctx.cmd_transferred == zcl_mdu_srv_ctx.ctx.cmd_total)
      zcl_mdu_srv_ctx.current_state = ZB_ZCL_MDU_PAIRING_STATE_READY;

    zb_zcl_send_cmd_tsn(param,
      &zcl_mdu_srv_ctx.ctx.dst_addr,
      ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
      zcl_mdu_srv_ctx.ctx.dst_endpoint,
      ZB_ZCL_FRAME_DIRECTION_TO_CLI,
      zcl_mdu_srv_ctx.ctx.src_endpoint,
      payload,
      pl_size,
      NULL,
      ZB_ZCL_CLUSTER_ID_MDU_PAIRING,
      ZB_ZCL_DISABLE_DEFAULT_RESPONSE,
      ZB_ZCL_MDU_PAIRING_SRV_CMD_PAIRING_RESPONSE,
      zcl_mdu_srv_ctx.tsn,
      NULL
    );

    if(zcl_mdu_srv_ctx.current_state == ZB_ZCL_MDU_PAIRING_STATE_RESPONSE_IN_PROGRESS)
    {
      TRACE_MSG(TRACE_ZCL3, "Reschedule zb_zcl_mdu_internal_cmd_server_send_response", (FMT__0));
      zb_buf_get_out_delayed(zb_zcl_mdu_internal_cmd_server_send_response);
    }
  }
  TRACE_MSG(TRACE_APP1, "<< zb_zcl_mdu_internal_cmd_server_send_response", (FMT__0));
}

zb_bool_t zb_zcl_mdu_pairing_request_handle(zb_uint8_t param, zb_zcl_parsed_hdr_t *cmd_info)
{
  zb_zcl_mdu_pairing_request_t req;
  zb_zcl_mdu_pairing_response_t resp;

  ZB_BZERO(&resp, sizeof(zb_zcl_mdu_pairing_response_t));

  TRACE_MSG(TRACE_APP1, ">> zb_zcl_mdu_pairing_request_handle", (FMT__0));

  if (zb_buf_len((param)) >= sizeof(zb_zcl_mdu_pairing_request_t))
  {
    zb_zcl_mdu_pairing_request_get_payload_from_data(&req, zb_buf_begin(param));

    if (zcl_mdu_srv_ctx.current_state == ZB_ZCL_MDU_PAIRING_STATE_RESPONSE_IN_PROGRESS)
    {
      zb_zcl_send_default_handler(param, cmd_info, ZB_ZCL_STATUS_WAIT_FOR_DATA);
      param = 0;
    } else
    if ((zcl_mdu_srv_ctx.pairing_version != 0) && (zcl_mdu_srv_ctx.pairing_version == req.lpi_version))
    {
      zb_zcl_send_default_handler(param, cmd_info, ZB_ZCL_STATUS_WAIT_FOR_DATA);
      param = 0;
    }
    else
    if (ZCL_CTX().device_cb)
    {
      ZB_ZCL_DEVICE_CMD_PARAM_INIT_WITH(param,
        ZB_ZCL_MDU_PAIRING_REQUEST_CB_ID, RET_ERROR, cmd_info, &req, &resp);

      ZCL_CTX().device_cb(param);

      TRACE_MSG(TRACE_ZCL1, "cb status %hd cmd status %hd", (FMT__H_H, ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param), *ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(param, zb_uint8_t)));

      if (ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) == RET_EMPTY)
      {
        zb_zcl_send_default_handler(param, cmd_info, ZB_ZCL_STATUS_WAIT_FOR_DATA);
        param = 0;
        TRACE_MSG(TRACE_APP1, "RET_EMPTY == send wait for data", (FMT__0));
      } else
      if (ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) == RET_OK)
      {
        //send framed actual MDU list
        zcl_mdu_srv_ctx.pairing_version = resp.lpi_version;
        zcl_mdu_srv_ctx.virtual_han_size = resp.total_number_of_devices;
        zcl_mdu_srv_ctx.virtual_han_table = resp.eui64;
        zcl_mdu_srv_ctx.current_state = ZB_ZCL_MDU_PAIRING_STATE_RESPONSE_IN_PROGRESS;
        zcl_mdu_srv_ctx.tsn = cmd_info->seq_number;

        zcl_mdu_srv_ctx.ctx.cmd_transferred = 0;
        zcl_mdu_srv_ctx.ctx.cmd_total = (zcl_mdu_srv_ctx.virtual_han_size / ZB_ZCL_MDU_PAIRING_HAN_TABLE_ONE_TRANSFER_LIMIT);
        if (zcl_mdu_srv_ctx.virtual_han_size % ZB_ZCL_MDU_PAIRING_HAN_TABLE_ONE_TRANSFER_LIMIT)
        {
          zcl_mdu_srv_ctx.ctx.cmd_total++;
        }

        zcl_mdu_srv_ctx.ctx.dst_addr.addr_short = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr;
        zcl_mdu_srv_ctx.ctx.dst_endpoint = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint;
        zcl_mdu_srv_ctx.ctx.src_endpoint = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).dst_endpoint;
        ZB_IEEE_ADDR_COPY(zcl_mdu_srv_ctx.ctx.partner,req.eui64);

        ZB_SCHEDULE_CALLBACK(zb_zcl_mdu_internal_cmd_server_send_response, param);
        param = 0;
      }
    }
  }

  if (param)
  {
    zb_zcl_send_default_handler(param, cmd_info, ZB_ZCL_STATUS_FAIL);
  }

  TRACE_MSG(TRACE_APP1, "<< zb_zcl_mdu_pairing_request_handle", (FMT__0));

  return ZB_TRUE;              /* cmd processed */
}


zb_bool_t zb_zcl_process_s_mdu_pairing_specific_commands(zb_uint8_t param)
{
  zb_bool_t processed = ZB_FALSE;
  zb_zcl_parsed_hdr_t cmd_info;

  if ( ZB_ZCL_GENERAL_GET_CMD_LISTS_PARAM == param )
  {
    ZCL_CTX().zb_zcl_cluster_cmd_list = &gs_server_cmd_list;
    return ZB_TRUE;
  }

  ZB_ZCL_COPY_PARSED_HEADER(param, &cmd_info);

  TRACE_MSG( TRACE_ZCL1,
             "> zb_zcl_process_s_mdu_pairing_specific_commands: param %hd",
             (FMT__H, param));

  ZB_ASSERT(cmd_info.cluster_id == ZB_ZCL_CLUSTER_ID_MDU_PAIRING);

  if (cmd_info.cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_SRV)
  {
    switch (cmd_info.cmd_id)
    {
      case ZB_ZCL_MDU_PAIRING_CLI_CMD_PAIRING_REQUEST:
        TRACE_MSG(TRACE_ZCL1, "ZB_ZCL_MDU_PAIRING_CLI_CMD_PAIRING_REQUEST", (FMT__0));
        processed = zb_zcl_mdu_pairing_request_handle(param, &cmd_info);
        break;
      default:
        break;
    }
  }

  TRACE_MSG( TRACE_ZCL1,
             "< zb_zcl_process_s_mdu_pairing_specific_commands: processed %d",
             (FMT__D, processed));
  return processed;
}

void zb_zcl_mdu_pairing_init_server()
{
  ZB_BZERO(&zcl_mdu_srv_ctx, sizeof(zb_zcl_mdu_srv_ctx_t));

  zb_zcl_add_cluster_handlers(ZB_ZCL_CLUSTER_ID_MDU_PAIRING,
                              ZB_ZCL_CLUSTER_SERVER_ROLE,
                              (zb_zcl_cluster_check_value_t)NULL,
                              (zb_zcl_cluster_write_attr_hook_t)NULL,
                              zb_zcl_process_s_mdu_pairing_specific_commands);
}

#endif /* ZB_ZCL_SUPPORT_CLUSTER_MDU_PAIRING || defined DOXYGEN */
