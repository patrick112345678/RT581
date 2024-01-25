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
/* PURPOSE: CLIENT: ZCL MDU Pairing cluster
         Allow devices joining the NAN to acquire a list of the devices
         forming the 'virtual HAN' for the respective household.
*/

#define ZB_TRACE_FILE_ID 89

#include "zb_common.h"

#if defined (ZB_ZCL_SUPPORT_CLUSTER_MDU_PAIRING) || defined DOXYGEN

#include "zcl/zb_zcl_mdu_pairing.h"

#define ZB_ZCL_MDU_PAIRING_MAX_PAYLOAD_FOR_TRANSFER 50
#define ZB_ZCL_MDU_PAIRING_HAN_TABLE_ONE_TRANSFER_LIMIT 5

zb_uint8_t gs_client_received_commands[] =
{
  ZB_ZCL_CLUSTER_ID_MDU_PAIRING_CLIENT_ROLE_RECEIVED_CMD_LIST
};

zb_uint8_t gs_client_generated_commands[] =
{
  ZB_ZCL_CLUSTER_ID_MDU_PAIRING_CLIENT_ROLE_GENERATED_CMD_LIST
};

zb_discover_cmd_list_t gs_mdu_pairing_client_cmd_list =
{
  sizeof(gs_client_received_commands), gs_client_received_commands,
  sizeof(gs_client_generated_commands), gs_client_generated_commands
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

typedef struct zb_zcl_mdu_cli_ctx_s
{
  zb_uint32_t pairing_version;
  zb_uint8_t virtual_han_size;
  zb_uint8_t virtual_han_size_max;
  zb_ieee_addr_t *virtual_han_table;
  zb_uint8_t items_transferred;
  zb_uint8_t current_state;
  mdu_pairing_table_t ctx;
  zb_callback_t pairing_request_cb;
} zb_zcl_mdu_cli_ctx_t;

zb_zcl_mdu_cli_ctx_t zcl_mdu_cli_ctx;

/* convert payload into raw data */
static zb_uint8_t *zb_zcl_mdu_pairing_request_put_payload(
  zb_uint8_t *data, const void *_payload)
{
  const zb_zcl_mdu_pairing_request_t *payload = _payload;
  ZB_ZCL_PACKET_PUT_DATA32(data, &payload->lpi_version);
  ZB_ZCL_PACKET_PUT_DATA64(data, &payload->eui64);
  return data;
}

zb_bool_t zb_zcl_mdu_pairing_response_handle(zb_uint8_t param, zb_zcl_parsed_hdr_t *cmd_info)
{
  zb_zcl_mdu_pairing_response_t resp;
  zb_ret_t ret = RET_ERROR;
  zb_uint8_t data_len = 0,i;
  zb_zcl_mdu_pairing_response_t *src_ptr = (zb_zcl_mdu_pairing_response_t*)zb_buf_begin(param);
  ZVUNUSED(cmd_info);

  if (zb_buf_len((param)) >= (sizeof(zb_zcl_mdu_pairing_response_t)-sizeof(void *)+sizeof(zb_ieee_addr_t)-1))
  {
    data_len = (zb_buf_len(param) - sizeof(zb_zcl_mdu_pairing_response_t) + sizeof(void *) + sizeof(zb_uint8_t)) / sizeof(zb_ieee_addr_t);

    TRACE_MSG( TRACE_ZCL1, "> data_len %hd", (FMT__H, data_len));

/* User CB data processing */
    ZB_HTOLE32(&resp.lpi_version, src_ptr);
    resp.total_number_of_devices = src_ptr->total_number_of_devices;
    resp.command_index = src_ptr->command_index;
    resp.total_number_of_commands = src_ptr->total_number_of_commands;
    resp.eui64 = (zb_ieee_addr_t *)&(src_ptr->eui64);
    resp.num_dev_cmd = data_len;

/* Internal MDU context processing */
    if (zcl_mdu_cli_ctx.current_state == ZB_ZCL_MDU_PAIRING_STATE_RESPONSE_RECEIVED)
    {
      if(zcl_mdu_cli_ctx.pairing_version<resp.lpi_version)
        zcl_mdu_cli_ctx.current_state = ZB_ZCL_MDU_PAIRING_STATE_READY;
    }
    if (zcl_mdu_cli_ctx.current_state == ZB_ZCL_MDU_PAIRING_STATE_READY)
    {
      zcl_mdu_cli_ctx.current_state = ZB_ZCL_MDU_PAIRING_STATE_RESPONSE_IN_PROGRESS;
      zcl_mdu_cli_ctx.virtual_han_size = resp.total_number_of_devices;
      zcl_mdu_cli_ctx.ctx.cmd_total = resp.total_number_of_commands;
      zcl_mdu_cli_ctx.ctx.cmd_transferred = 0;
      zcl_mdu_cli_ctx.items_transferred = 0;
      zcl_mdu_cli_ctx.pairing_version = resp.lpi_version;

      ZB_ASSERT( zb_address_ieee_by_short(
                    ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr,
                    zcl_mdu_cli_ctx.ctx.partner ) == RET_OK);

      zcl_mdu_cli_ctx.ctx.dst_addr.addr_short = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr;
      zcl_mdu_cli_ctx.ctx.dst_endpoint = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint;
      zcl_mdu_cli_ctx.ctx.src_endpoint = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).dst_endpoint;
    }
    if (zcl_mdu_cli_ctx.current_state == ZB_ZCL_MDU_PAIRING_STATE_RESPONSE_IN_PROGRESS)
    {
      zb_uint8_t valid = ZB_TRUE;
/* Validate partner's data*/
/*      if( zcl_mdu_cli_ctx.pairing_version<resp.lpi_version &&
          zcl_mdu_cli_ctx.ctx.dst_addr.addr_short == ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr &&
          zcl_mdu_cli_ctx.ctx.src_endpoint == ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).dst_endpoint &&
          (resp.command_index - zcl_mdu_cli_ctx.ctx.cmd_transferred) == 1 &&
          resp.command_index <= resp.total_number_of_commands &&
          zcl_mdu_cli_ctx.ctx.cmd_total == resp.total_number_of_commands &&
          zcl_mdu_cli_ctx.virtual_han_size == resp.total_number_of_devices )*/
      if( zcl_mdu_cli_ctx.pairing_version != resp.lpi_version )
      {
        TRACE_MSG( TRACE_ZCL1, "zcl_mdu_cli_ctx.pairing_version != resp.lpi_version", (FMT__0) );
        valid = ZB_FALSE;
      }
      if( zcl_mdu_cli_ctx.ctx.dst_addr.addr_short != ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr )
      {
        TRACE_MSG( TRACE_ZCL1, "zcl_mdu_cli_ctx.ctx.dst_addr.addr_short != ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr", (FMT__0) );
        valid = ZB_FALSE;
      }
      if( zcl_mdu_cli_ctx.ctx.src_endpoint != ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).dst_endpoint )
      {
        TRACE_MSG( TRACE_ZCL1, "zcl_mdu_cli_ctx.ctx.src_endpoint != ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).dst_endpoint", (FMT__0) );
        valid = ZB_FALSE;
      }
      if( (resp.command_index - zcl_mdu_cli_ctx.ctx.cmd_transferred) != 0 )
      {
        TRACE_MSG( TRACE_ZCL1, "(resp.command_index - zcl_mdu_cli_ctx.ctx.cmd_transferred) %hd != 0", (FMT__H,
         resp.command_index - zcl_mdu_cli_ctx.ctx.cmd_transferred) );
        valid = ZB_FALSE;
      }
      if( resp.command_index > zcl_mdu_cli_ctx.ctx.cmd_total )
      {
        TRACE_MSG( TRACE_ZCL1, "resp.command_index > zcl_mdu_cli_ctx.ctx.cmd_total", (FMT__0) );
        valid = ZB_FALSE;
      }
      if( zcl_mdu_cli_ctx.ctx.cmd_total != resp.total_number_of_commands )
      {
        TRACE_MSG( TRACE_ZCL1, "zcl_mdu_cli_ctx.ctx.cmd_total != resp.total_number_of_commands", (FMT__0) );
        valid = ZB_FALSE;
      }
      if( zcl_mdu_cli_ctx.virtual_han_size != resp.total_number_of_devices )
      {
        TRACE_MSG( TRACE_ZCL1, "zcl_mdu_cli_ctx.virtual_han_size != resp.total_number_of_devices", (FMT__0) );
        valid = ZB_FALSE;
      }
      if (valid)
      {
        TRACE_MSG( TRACE_ZCL1, "VALID data received: %hd", (FMT__H, data_len));
        zcl_mdu_cli_ctx.pairing_version = resp.lpi_version;
        for (i=0;i<data_len;i++)
        {
          if (zcl_mdu_cli_ctx.items_transferred<zcl_mdu_cli_ctx.virtual_han_size_max)
          {
            zb_ieee_addr_t my_long_address;
            zb_get_long_address(my_long_address);
            if( ZB_MEMCMP( resp.eui64, my_long_address, sizeof(zb_ieee_addr_t)) == 0 )
            {
              resp.eui64++;
              TRACE_MSG( TRACE_ZCL1, "My eui64 received", (FMT__0));
            }
            else
            {
              ZB_IEEE_ADDR_COPY(zcl_mdu_cli_ctx.virtual_han_table[zcl_mdu_cli_ctx.items_transferred++],resp.eui64++);
            }
          }
          else
          {
            TRACE_MSG( TRACE_ZCL1, "WARNING: overflow in virtual_han_table: table full: %hd", (FMT__H, zcl_mdu_cli_ctx.items_transferred));
          }
        }
        zcl_mdu_cli_ctx.ctx.cmd_transferred++;
        if(zcl_mdu_cli_ctx.ctx.cmd_total == zcl_mdu_cli_ctx.ctx.cmd_transferred)
        {
          if ((zcl_mdu_cli_ctx.virtual_han_size-1) != zcl_mdu_cli_ctx.items_transferred)
          {
            TRACE_MSG( TRACE_ZCL1, "WARNING: MDU table check mismatch - dropped : devices: %hd, received: %hd (min 2 dev)",
                       (FMT__H_H, zcl_mdu_cli_ctx.virtual_han_size, zcl_mdu_cli_ctx.items_transferred));
            zcl_mdu_cli_ctx.current_state = ZB_ZCL_MDU_PAIRING_STATE_READY;
          }
          else
          {
            zcl_mdu_cli_ctx.current_state = ZB_ZCL_MDU_PAIRING_STATE_RESPONSE_RECEIVED;
            resp.eui64 = (zb_ieee_addr_t *)zcl_mdu_cli_ctx.virtual_han_table;
            resp.num_dev_cmd = --zcl_mdu_cli_ctx.virtual_han_size;
          }
        }
      }
      else
      {
        TRACE_MSG( TRACE_ZCL1, "WARNING: MDU packet check mismatch - dropped", (FMT__0));
      }
    }
  }
  if (zcl_mdu_cli_ctx.current_state == ZB_ZCL_MDU_PAIRING_STATE_RESPONSE_RECEIVED)
  {
    if (ZCL_CTX().device_cb)
    {
      ZB_ZCL_DEVICE_CMD_PARAM_INIT_WITH(param,
                                        ZB_ZCL_MDU_PAIRING_RESPONSE_CB_ID,
                                        RET_OK,
                                        cmd_info,
                                        &resp,
                                        NULL);

      ZCL_CTX().device_cb(param);
    }
    if (zcl_mdu_cli_ctx.pairing_request_cb)
    {
      zcl_mdu_cli_ctx.pairing_request_cb(zcl_mdu_cli_ctx.virtual_han_size);
    }
    ret = RET_OK;
  }

  zb_zcl_send_default_handler(param, cmd_info, ret == RET_OK ? ZB_ZCL_STATUS_SUCCESS : ZB_ZCL_STATUS_FAIL);

  return ZB_TRUE;
}

zb_ret_t zb_zcl_mdu_pairing_send_cmd_pairing_request(
  zb_uint8_t param,
  const zb_addr_u *dst_addr,
  zb_aps_addr_mode_t dst_addr_mode,
  zb_uint8_t dst_ep,
  zb_uint8_t src_ep,
  const zb_zcl_mdu_pairing_request_t *payload,
  zb_ieee_addr_t *buf,
  zb_uint8_t buf_len,
  zb_callback_t cb
)
{
  zb_ret_t ret = RET_OK;
  if(buf)
  {
    zcl_mdu_cli_ctx.virtual_han_table = buf;
    zcl_mdu_cli_ctx.virtual_han_size_max = buf_len;

    zb_zcl_send_cmd(param,
      dst_addr,
      dst_addr_mode,
      dst_ep,
      ZB_ZCL_FRAME_DIRECTION_TO_SRV,
      src_ep,
      payload,
      sizeof(*payload),
      zb_zcl_mdu_pairing_request_put_payload,
      ZB_ZCL_CLUSTER_ID_MDU_PAIRING,
      ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
      ZB_ZCL_MDU_PAIRING_CLI_CMD_PAIRING_REQUEST,
      NULL
    );

    if (cb)
    {
      zcl_mdu_cli_ctx.pairing_request_cb = cb;
    }
  }
  else
  {
    TRACE_MSG( TRACE_ZCL1,"> zb_zcl_mdu_pairing_send_cmd_pairing_request: buffer not passed!", (FMT__0));
    ret = RET_INVALID_PARAMETER;
  }
  return ret;
}

zb_bool_t zb_zcl_process_c_mdu_pairing_specific_commands(zb_uint8_t param)
{
  zb_bool_t processed = ZB_FALSE;
  zb_zcl_parsed_hdr_t cmd_info;

  if ( ZB_ZCL_GENERAL_GET_CMD_LISTS_PARAM == param )
  {
    ZCL_CTX().zb_zcl_cluster_cmd_list = &gs_mdu_pairing_client_cmd_list;
    return ZB_TRUE;
  }

  ZB_ZCL_COPY_PARSED_HEADER(param, &cmd_info);

  TRACE_MSG( TRACE_ZCL1,
             "> zb_zcl_process_c_mdu_pairing_specific_commands: param %hd",
             (FMT__H, param));

  ZB_ASSERT(cmd_info.cluster_id == ZB_ZCL_CLUSTER_ID_MDU_PAIRING);

  if (cmd_info.cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI)
  {
    switch (cmd_info.cmd_id)
    {
      case ZB_ZCL_MDU_PAIRING_SRV_CMD_PAIRING_RESPONSE:
        TRACE_MSG(TRACE_ZCL1, "ZB_ZCL_MDU_PAIRING_SRV_CMD_PAIRING_RESPONSE", (FMT__0));
        processed = zb_zcl_mdu_pairing_response_handle(param, &cmd_info);
        break;
      default:
        break;
    }
  }

  TRACE_MSG( TRACE_ZCL1,
             "< zb_zcl_process_c_mdu_pairing_specific_commands: processed %d",
             (FMT__D, processed));
  return processed;
}

void zb_zcl_mdu_pairing_init_client()
{
  ZB_BZERO(&zcl_mdu_cli_ctx, sizeof(zb_zcl_mdu_cli_ctx_t));

  zb_zcl_add_cluster_handlers(ZB_ZCL_CLUSTER_ID_MDU_PAIRING,
                              ZB_ZCL_CLUSTER_CLIENT_ROLE,
                              (zb_zcl_cluster_check_value_t)NULL,
                              (zb_zcl_cluster_write_attr_hook_t)NULL,
                              zb_zcl_process_c_mdu_pairing_specific_commands);
}

#endif /* ZB_ZCL_SUPPORT_CLUSTER_MDU_PAIRING || defined DOXYGEN */
