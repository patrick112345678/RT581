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
/* PURPOSE: ZBOSS specific IR Blaster cluster.
*/


#define ZB_TRACE_FILE_ID 87

#include "zb_common.h"

#if defined (ZB_ZCL_SUPPORT_CLUSTER_IR_BLASTER)

#include "zb_zdo.h"
#include "zb_zcl.h"
#include "zcl/zb_zcl_ir_blaster.h"
#include "zb_zdo.h"
#include "zb_aps.h"

zb_uint8_t gs_ir_blaster_client_received_commands[] =
{
  ZB_ZCL_CMD_IR_BLASTER_TRANSMISSION_STATUS,
  ZB_ZCL_CMD_IR_BLASTER_GET_IR_SIGNATURE_RESP
};

zb_uint8_t gs_ir_blaster_server_received_commands[] =
{
  ZB_ZCL_CMD_IR_BLASTER_TRANSMIT_IR_DATA,
  ZB_ZCL_CMD_IR_BLASTER_GET_IR_SIGNATURE
};

zb_discover_cmd_list_t gs_ir_blaster_client_cmd_list =
{
  sizeof(gs_ir_blaster_client_received_commands), gs_ir_blaster_client_received_commands,
  sizeof(gs_ir_blaster_server_received_commands), gs_ir_blaster_server_received_commands
};

zb_discover_cmd_list_t gs_ir_blaster_server_cmd_list =
{
  sizeof(gs_ir_blaster_server_received_commands), gs_ir_blaster_server_received_commands,
  sizeof(gs_ir_blaster_client_received_commands), gs_ir_blaster_client_received_commands
};

zb_ret_t check_value_ir_blaster(zb_uint16_t attr_id, zb_uint8_t endpoint, zb_uint8_t *value);

zb_bool_t zb_zcl_process_ir_blaster_specific_commands_srv(zb_uint8_t param);
zb_bool_t zb_zcl_process_ir_blaster_specific_commands_cli(zb_uint8_t param);

void zb_zcl_ir_blaster_init_server()
{
  zb_zcl_add_cluster_handlers(ZB_ZCL_CLUSTER_ID_IR_BLASTER,
                              ZB_ZCL_CLUSTER_SERVER_ROLE,
                              check_value_ir_blaster,
                              (zb_zcl_cluster_write_attr_hook_t)NULL,
                              zb_zcl_process_ir_blaster_specific_commands_srv);
}

void zb_zcl_ir_blaster_init_client()
{
  zb_zcl_add_cluster_handlers(ZB_ZCL_CLUSTER_ID_IR_BLASTER,
                              ZB_ZCL_CLUSTER_CLIENT_ROLE,
                              check_value_ir_blaster,
                              (zb_zcl_cluster_write_attr_hook_t)NULL,
                              zb_zcl_process_ir_blaster_specific_commands_cli);
}

zb_ret_t check_value_ir_blaster(zb_uint16_t attr_id, zb_uint8_t endpoint, zb_uint8_t *value)
{
  ZVUNUSED(attr_id);
  ZVUNUSED(value);
  ZVUNUSED(endpoint);

  /* All values for mandatory attributes are allowed, extra check for
   * optional attributes is needed */

  return RET_OK;
}

zb_ret_t zb_zcl_ir_blaster_transmit_ir_data_handler(zb_uint8_t param)
{
  zb_ret_t ret = RET_ERROR;
  zb_zcl_parsed_hdr_t cmd_info;
  zb_zcl_ir_blaster_transmit_ir_data_t req_data;
  zb_zcl_parse_status_t status;

  TRACE_MSG( TRACE_ZCL1, "> zb_zcl_ir_blaster_transmit_ir_data_handler param %hd", (FMT__H, param));

  ZB_ZCL_COPY_PARSED_HEADER(param, &cmd_info);

  ZB_ZCL_IR_BLASTER_GET_TRANSMIT_IR_DATA(&req_data, param, status);
  if (status != ZB_ZCL_PARSE_STATUS_SUCCESS)
  {
    TRACE_MSG( TRACE_ZCL2, "Invalid paylaod", (FMT__0));
    ret = RET_INVALID_PARAMETER;
  }
  else
  {
    //TRACE_MSG( TRACE_ZCL2, "IR code ID: " TRACE_FORMAT_128, (FMT__B, TRACE_ARG_128(req_data.ir_code_id)));
    if (ZCL_CTX().device_cb)
    {
      zb_zcl_device_callback_param_t *device_cb_param =
        ZB_BUF_GET_PARAM(param, zb_zcl_device_callback_param_t);

      device_cb_param->device_cb_id = ZB_ZCL_IR_BLASTER_TRANSMIT_IR_DATA_CB_ID;
      ZB_MEMCPY(&device_cb_param->cb_param.irb_tr_value_param.cmd_info,
                &cmd_info, sizeof(zb_zcl_parsed_hdr_t));
      ZB_MEMCPY(&device_cb_param->cb_param.irb_tr_value_param.payload,
                &req_data, sizeof(zb_zcl_ir_blaster_transmit_ir_data_t));
      device_cb_param->status = RET_OK;
      (ZCL_CTX().device_cb)(param);
      ret = device_cb_param->status;
    }
  }

  TRACE_MSG( TRACE_ZCL1, "< zb_zcl_ir_blaster_transmit_ir_data_handler ret %hd", (FMT__H, ret));

  return ret;
}

zb_ret_t zb_zcl_ir_blaster_transmission_status_handler(zb_uint8_t param)
{
  zb_ret_t ret = RET_OK;
  zb_zcl_parsed_hdr_t cmd_info;
  zb_zcl_ir_blaster_transmission_status_t resp_data;
  zb_zcl_parse_status_t status;

  TRACE_MSG( TRACE_ZCL1, "> zb_zcl_ir_blaster_transmission_status_handler param %hd", (FMT__H, param));

  ZB_ZCL_COPY_PARSED_HEADER(param, &cmd_info);

  ZB_ZCL_IR_BLASTER_GET_TRANSMISSION_STATUS(&resp_data, param, status);

  if(status != ZB_ZCL_PARSE_STATUS_SUCCESS)
  {
    ret = RET_INVALID_PARAMETER;
  }
  else
  {
    TRACE_MSG( TRACE_ZCL1, "IR transmission status: 0x%x", (FMT__H, resp_data.status));
    if (ZCL_CTX().device_cb)
    {
      zb_zcl_device_callback_param_t *device_cb_param =
        ZB_BUF_GET_PARAM(param, zb_zcl_device_callback_param_t);

      device_cb_param->device_cb_id = ZB_ZCL_IR_BLASTER_TRANSMISSION_STATUS_CB_ID;
      /*ZB_MEMCPY(&device_cb_param->cb_param.irb_tr_status_value_param.cmd_info,
                &cmd_info, sizeof(zb_zcl_parsed_hdr_t));*/
      ZB_MEMCPY(&device_cb_param->cb_param.irb_tr_status_value_param.payload,
                &resp_data, sizeof(zb_zcl_ir_blaster_transmission_status_t));
      device_cb_param->status = RET_OK;
      (ZCL_CTX().device_cb)(param);
      ret = device_cb_param->status;
    }
  }

  TRACE_MSG( TRACE_ZCL1, "< zb_zcl_ir_blaster_transmission_status_handler ret %hd", (FMT__H, ret));
  return ret;
}

zb_ret_t zb_zcl_ir_blaster_get_ir_signature_handler(zb_uint8_t param)
{
  zb_ret_t ret = RET_ERROR;
  zb_zcl_parsed_hdr_t cmd_info;

  TRACE_MSG( TRACE_ZCL1, "> zb_zcl_ir_blaster_get_ir_signature_handler param %hd", (FMT__H, param));

  ZB_ZCL_COPY_PARSED_HEADER(param, &cmd_info);

  if (ZCL_CTX().device_cb)
  {
    zb_zcl_device_callback_param_t *device_cb_param =
      ZB_BUF_GET_PARAM(param, zb_zcl_device_callback_param_t);

    device_cb_param->device_cb_id = ZB_ZCL_IR_BLASTER_GET_IR_SIGNATURE_CB_ID;
    ZB_MEMCPY(&device_cb_param->cb_param.irb_get_ir_sig_value_param.cmd_info,
              &cmd_info, sizeof(zb_zcl_parsed_hdr_t));
    device_cb_param->status = RET_OK;
    (ZCL_CTX().device_cb)(param);
    ret = device_cb_param->status;
  }

  TRACE_MSG( TRACE_ZCL1, "< zb_zcl_ir_blaster_get_ir_signature_handler ret %hd", (FMT__H, ret));
  return ret;
}

zb_ret_t zb_zcl_ir_blaster_get_ir_signature_resp_handler(zb_uint8_t param)
{
  zb_ret_t ret = RET_ERROR;
  zb_zcl_parsed_hdr_t cmd_info;
  zb_zcl_ir_blaster_get_ir_signature_resp_t resp_data;
  zb_zcl_parse_status_t status;

  TRACE_MSG( TRACE_ZCL1, "> zb_zcl_ir_blaster_get_ir_signature_resp_handler param %hd", (FMT__H, param));

  ZB_ZCL_COPY_PARSED_HEADER(param, &cmd_info);

  ZB_ZCL_IR_BLASTER_GET_GET_IR_SIGNATURE_RESP(&resp_data, param, status);

  if(status != ZB_ZCL_PARSE_STATUS_SUCCESS)
  {
    ret = RET_INVALID_PARAMETER;
  }
  else if (ZCL_CTX().device_cb)
  {
    zb_zcl_device_callback_param_t *device_cb_param =
      ZB_BUF_GET_PARAM(param, zb_zcl_device_callback_param_t);

    device_cb_param->device_cb_id = ZB_ZCL_IR_BLASTER_GET_IR_SIGNATURE_RESP_CB_ID;
    ZB_MEMCPY(&device_cb_param->cb_param.irb_get_ir_sig_resp_value_param.cmd_info,
              &cmd_info, sizeof(zb_zcl_parsed_hdr_t));
    ZB_MEMCPY(&device_cb_param->cb_param.irb_get_ir_sig_resp_value_param.payload,
              &resp_data, sizeof(zb_zcl_ir_blaster_get_ir_signature_resp_t));
    device_cb_param->status = RET_OK;
    (ZCL_CTX().device_cb)(param);
    ret = device_cb_param->status;
  }

  TRACE_MSG( TRACE_ZCL1, "< zb_zcl_ir_blaster_get_ir_signature_resp_handler ret %hd", (FMT__H, ret));
  return ret;
}

zb_uint8_t zb_zcl_process_ir_blaster_specific_commands(zb_uint8_t param)
{
  zb_bool_t processed = ZB_TRUE;
  zb_zcl_parsed_hdr_t cmd_info;
  //zb_uint8_t endpoint;
  zb_ret_t status = RET_OK;

  ZB_ZCL_COPY_PARSED_HEADER(param, &cmd_info);

  TRACE_MSG( TRACE_ZCL1,
             "> zb_zcl_process_ir_blaster_specific_commands: buf %hd, cmd_info %p",
             (FMT__H_P, param, &cmd_info));

  //endpoint = ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).dst_endpoint;

  if (!cmd_info.is_common_command)
  {
    if(ZB_ZCL_FRAME_DIRECTION_TO_SRV == cmd_info.cmd_direction)
    {
      switch( cmd_info.cmd_id )
      {
      case ZB_ZCL_CMD_IR_BLASTER_TRANSMIT_IR_DATA:
        status = zb_zcl_ir_blaster_transmit_ir_data_handler(param);
        break;

      case ZB_ZCL_CMD_IR_BLASTER_GET_IR_SIGNATURE:
        status = zb_zcl_ir_blaster_get_ir_signature_handler(param);
        break;

      default:
        processed = ZB_FALSE;
        break;
      }
    }
    else  // ZB_ZCL_FRAME_DIRECTION_TO_CLI
    {
      switch( cmd_info.cmd_id )
      {
      case ZB_ZCL_CMD_IR_BLASTER_TRANSMISSION_STATUS:
        status = zb_zcl_ir_blaster_transmission_status_handler(param);
        break;

      case ZB_ZCL_CMD_IR_BLASTER_GET_IR_SIGNATURE_RESP:
        status = zb_zcl_ir_blaster_get_ir_signature_resp_handler(param);
        break;

      default:
        processed = ZB_FALSE;
        break;
      }
    }
  }

  if( processed )
  {
    if (status != RET_BUSY)
    {
      if (ZB_ZCL_CHECK_IF_SEND_DEFAULT_RESP(cmd_info, status) || status != RET_OK)
      {
        zb_zcl_send_default_resp_ext(param,
                                     &cmd_info,
                                     ((status == RET_OK) ?
                                      ZB_ZCL_STATUS_SUCCESS :
                                      ZB_ZCL_STATUS_INVALID_FIELD));
        param = 0;
      }
    }
  }

  if (param)
  {
    zb_buf_free(param);
  }

  TRACE_MSG( TRACE_ZCL1,
             "< zb_zcl_process_ir_blaster_specific_commands: processed %d",
             (FMT__D, processed));
  return processed;
}

zb_bool_t zb_zcl_process_ir_blaster_specific_commands_srv(zb_uint8_t param)
{
  if ( ZB_ZCL_GENERAL_GET_CMD_LISTS_PARAM == param )
  {
    ZCL_CTX().zb_zcl_cluster_cmd_list = &gs_ir_blaster_server_cmd_list;
    return ZB_TRUE;
  }
  return zb_zcl_process_ir_blaster_specific_commands(param);
}

zb_bool_t zb_zcl_process_ir_blaster_specific_commands_cli(zb_uint8_t param)
{
  if ( ZB_ZCL_GENERAL_GET_CMD_LISTS_PARAM == param )
  {
    ZCL_CTX().zb_zcl_cluster_cmd_list = &gs_ir_blaster_client_cmd_list;
    return ZB_TRUE;
  }
  return zb_zcl_process_ir_blaster_specific_commands(param);
}

#endif /* defined ZB_ZCL_SUPPORT_CLUSTER_IR_BLASTER */
