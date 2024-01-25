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
/* PURPOSE: Custom cluster sample
*/

#define ZB_TRACE_FILE_ID 60777

#include "zboss_api.h"
#include "custom_cluster_zr.h"

#ifndef ZB_ROUTER_ROLE
#error define ZB_ROUTER_ROLE to compile sample!
#endif

/**
 * Global variables definitions
 */

zb_ieee_addr_t g_zr_addr = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

/**
 * Declaring attributes for each cluser
 */

/* Basic cluster attributes */
zb_uint8_t g_attr_zcl_version  = ZB_ZCL_VERSION;
zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(
  basic_attr_list, &g_attr_zcl_version, &g_attr_power_source);

/* Identify cluster attributes */
zb_uint16_t g_attr_identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_attr_identify_time);

/* Custom cluster attributes */
zb_uint8_t g_attr_u8 = ZB_ZCL_CUSTOM_CLUSTER_ATTR_U8_DEFAULT_VALUE;
zb_int16_t g_attr_s16 = ZB_ZCL_CUSTOM_CLUSTER_ATTR_S16_DEFAULT_VALUE;
zb_uint24_t g_attr_24bit = ZB_ZCL_CUSTOM_CLUSTER_ATTR_24BIT_DEFAULT_VALUE;
zb_uint32_t g_attr_32bitmap = ZB_ZCL_CUSTOM_CLUSTER_ATTR_32BITMAP_DEFAULT_VALUE;
zb_ieee_addr_t g_attr_ieee = ZB_ZCL_CUSTOM_CLUSTER_ATTR_IEEE_DEFAULT_VALUE;
zb_char_t g_attr_char_string[ZB_ZCL_CUSTOM_CLUSTER_ATTR_CHAR_STRING_MAX_SIZE] =
  ZB_ZCL_CUSTOM_CLUSTER_ATTR_CHAR_STRING_DEFAULT_VALUE;
zb_time_t g_attr_utc_time = ZB_ZCL_CUSTOM_CLUSTER_ATTR_UTC_TIME_DEFAULT_VALUE;
zb_uint8_t g_attr_byte_array[ZB_ZCL_CUSTOM_CLUSTER_ATTR_BYTE_ARRAY_MAX_SIZE] =
  ZB_ZCL_CUSTOM_CLUSTER_ATTR_BYTE_ARRAY_DEFAULT_VALUE;
zb_bool_t g_attr_bool = ZB_ZCL_CUSTOM_CLUSTER_ATTR_BOOL_DEFAULT_VALUE;
zb_uint8_t g_attr_128_bit_key[ZB_CCM_KEY_SIZE] = ZB_ZCL_CUSTOM_CLUSTER_ATTR_128_BIT_KEY_DEFAULT_VALUE;

ZB_ZCL_DECLARE_CUSTOM_ATTR_CLUSTER_ATTRIB_LIST(custom_attr_list,
                                               &g_attr_u8,
                                               &g_attr_s16,
                                               &g_attr_24bit,
                                               &g_attr_32bitmap,
                                               g_attr_ieee,
                                               g_attr_char_string,
                                               &g_attr_utc_time,
                                               g_attr_byte_array,
                                               &g_attr_bool,
                                               g_attr_128_bit_key);

/* Declare cluster list for the device */
ZB_DECLARE_CUSTOM_CLUSTER_LIST(custom_clusters,
                               basic_attr_list,
                               identify_attr_list,
                               custom_attr_list);

/* Declare endpoint */
ZB_DECLARE_CUSTOM_EP(custom_ep, ENDPOINT_SERVER, custom_clusters);

/* Declare application's device context for single-endpoint device */
ZB_DECLARE_CUSTOM_CTX(custom_ctx, custom_ep);

/* Handler for specific ZCL commands */
static zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);

static void zb_zcl_custom_cluster_send_default_response(
  zb_bufid_t buf, zb_zcl_parsed_hdr_t *cmd_info, zb_zcl_status_t status);

/* Handler for specific Custom cluster commands */
static zb_zcl_status_t zb_zcl_custom_cluster_handler(zb_bufid_t buf, zb_zcl_parsed_hdr_t *cmd_info);

/* Handlers for each Custom cluster command */
static void zb_zcl_custom_cluster_cmd1_handler(zb_bufid_t buf, zb_zcl_parsed_hdr_t *cmd_info);
static void zb_zcl_custom_cluster_cmd2_handler(zb_bufid_t buf, zb_zcl_parsed_hdr_t *cmd_info);
static void zb_zcl_custom_cluster_cmd3_handler(zb_bufid_t buf, zb_zcl_parsed_hdr_t *cmd_info);


MAIN()
{
  ARGV_UNUSED;

  /* Trace enable */
  ZB_SET_TRAF_DUMP_ON();

  /* Configure trace */
  ZB_SET_TRACE_LEVEL(4);
  ZB_SET_TRACE_MASK(0x0800);

  /* Global ZBOSS initialization */
  ZB_INIT("custom_cluster_zr");

  /* Set up defaults for the commissioning */
  zb_set_long_address(g_zr_addr);
  zb_set_network_router_role(ZB_CUSTOM_CHANNEL_MASK);
  zb_set_nvram_erase_at_start(ZB_FALSE);

  /* Register device ZCL context */
  ZB_AF_REGISTER_DEVICE_CTX(&custom_ctx);

  /* Register cluster commands handler for a specific endpoint */
  /* callback will be called BEFORE stack handle */
  ZB_AF_SET_ENDPOINT_HANDLER(ENDPOINT_SERVER, zcl_specific_cluster_cmd_handler);

  /* Initiate the stack start with starting the commissioning */
  if (zboss_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
  }
  else
  {
    /* Call the main loop */
    zboss_main_loop();
  }

  /* Deinitialize trace */
  TRACE_DEINIT();

  MAIN_RETURN(0);
}

zb_ret_t check_value_custom_attr(zb_uint16_t attr_id, zb_uint8_t endpoint, zb_uint8_t *value)
{
  ZVUNUSED(attr_id);
  ZVUNUSED(endpoint);
  ZVUNUSED(value);

  return RET_OK;
}

/* For correct workflow of ZCL command proceesing at least cluster_check_value function should be defined in zb_zcl_add_cluster_handlers*/
void zb_zcl_custom_attr_init_server(void)
{
  TRACE_MSG(TRACE_APP1, ">> zb_zcl_custom_attr_init_server", (FMT__0));

  zb_zcl_add_cluster_handlers(ZB_ZCL_CLUSTER_ID_CUSTOM,
                              ZB_ZCL_CLUSTER_SERVER_ROLE,
                              (zb_zcl_cluster_check_value_t)check_value_custom_attr,
                              (zb_zcl_cluster_write_attr_hook_t)NULL,
                              (zb_zcl_cluster_handler_t)NULL);

  TRACE_MSG(TRACE_APP1, "<< zb_zcl_custom_attr_init_server", (FMT__0));
}


static zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
  zb_bufid_t buf = param;
  zb_uint8_t lqi = ZB_MAC_LQI_UNDEFINED;
  zb_int8_t rssi = ZB_MAC_RSSI_UNDEFINED;
  zb_bool_t handle_status = ZB_FALSE;
  zb_zcl_status_t resp_status = ZB_ZCL_STATUS_FAIL;
  zb_zcl_parsed_hdr_t cmd_info;

  ZB_ZCL_COPY_PARSED_HEADER(buf, &cmd_info);

  TRACE_MSG(TRACE_APP1, ">> zcl_specific_cluster_cmd_handler, param %d", (FMT__D, param));
  TRACE_MSG(TRACE_APP1, "dir %d, profile_id 0x%x, cluster_id 0x%x",
            (FMT__D_D_D, cmd_info.cmd_direction, cmd_info.profile_id, cmd_info.cluster_id));

  zb_zdo_get_diag_data(
    ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).source.u.short_addr,
    &lqi, &rssi);
  TRACE_MSG(TRACE_APP1, "lqi %hd rssi %d", (FMT__H_H, lqi, rssi));

  /* Check whether the packet direction is right */
  if (cmd_info.cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI)
  {
    TRACE_MSG(TRACE_ERROR, "Unsupported \"from server\" command direction", (FMT__0));
  }
  else
  {
    /* Check profile ID*/
    switch (cmd_info.profile_id)
    {
      case ZB_AF_HA_PROFILE_ID:
        /* Check cluster ID */
        switch (cmd_info.cluster_id)
        {
          case ZB_ZCL_CLUSTER_ID_CUSTOM:
            resp_status = zb_zcl_custom_cluster_handler(buf, &cmd_info);
            break;

          /* Implement the processing of other clusters here */

          default:
            break;
        }
        break;

      /* Implement the processing of other profiles here */

      default:
        break;
    }

    /*  There are the three ways in this case:
       1) reuse the buffer to send specific response to the custom cluster command;
       2) reuse the buffer to send default response to the custom cluster command
          or we free the buffer, if disable.default.response flag is true
       3) if this is not a custom cluster command - need to pass buffer to the stack
    */
    if (ZB_ZCL_STATUS_ABORT == resp_status)
    {
      handle_status = ZB_TRUE;
    }
  }

  TRACE_MSG(TRACE_APP1, "handle_status %d", (FMT__D, handle_status));
  TRACE_MSG(TRACE_APP1, "<< zcl_specific_cluster_cmd_handler", (FMT__0));

  return handle_status;
}


static void zb_zcl_custom_cluster_send_default_response(
  zb_bufid_t buf, zb_zcl_parsed_hdr_t *cmd_info, zb_zcl_status_t status)
{
  TRACE_MSG(TRACE_APP1, ">> zb_zcl_custom_cluster_send_default_response", (FMT__0));

  if (!buf || !cmd_info)
  {
    TRACE_MSG(TRACE_APP1, "error, invalid ptr", (FMT__0));
    return;
  }

  if (cmd_info->disable_default_response)
  {
    TRACE_MSG(TRACE_APP1, "default response is disabled", (FMT__0));
    zb_buf_free(buf);
    return;
  }

/* [ZCL_SEND_DEFAULT_RESP] */
  ZB_ZCL_SEND_DEFAULT_RESP(
    buf,
    ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr,
    ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
    ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint,
    ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).dst_endpoint,
    cmd_info->profile_id,
    ZB_ZCL_CLUSTER_ID_CUSTOM,
    cmd_info->seq_number,
    cmd_info->cmd_id,
    status);
/* [ZCL_SEND_DEFAULT_RESP] */

  TRACE_MSG(TRACE_APP1, "<<zb_zcl_custom_cluster_send_default_response", (FMT__0));
}


static zb_zcl_status_t zb_zcl_custom_cluster_handler(zb_bufid_t buf, zb_zcl_parsed_hdr_t *cmd_info)
{
  zb_zcl_status_t resp_status = ZB_ZCL_STATUS_ABORT;

  switch (cmd_info->cmd_id)
  {
    case ZB_ZCL_CUSTOM_CLUSTER_CMD1_ID:
    {
      TRACE_MSG(TRACE_APP1, "Custom cluster Command 1 received!", (FMT__0));
      zb_zcl_custom_cluster_cmd1_handler(buf, cmd_info);
    }
    break;

    case ZB_ZCL_CUSTOM_CLUSTER_CMD2_ID:
    {
      TRACE_MSG(TRACE_APP1, "Custom cluster Command 2 received!", (FMT__0));
      zb_zcl_custom_cluster_cmd2_handler(buf, cmd_info);
    }
    break;

    case ZB_ZCL_CUSTOM_CLUSTER_CMD3_ID:
      TRACE_MSG(TRACE_APP1, "Custom cluster Command 3 received!", (FMT__0));
      zb_zcl_custom_cluster_cmd3_handler(buf, cmd_info);
      break;

    case ZB_ZCL_CMD_WRITE_ATTRIB_UNDIV:
    {
      zb_uint8_t endpoint;
      zb_af_endpoint_desc_t *ep_desc;
      zb_zcl_cluster_desc_t *cluster_desc;
      zb_zcl_attr_t *attr_desc;
      zb_zcl_write_attr_req_t *write_attr_req;
      zb_uint8_t *data_ptr;
      zb_uint8_t buf_len;

      TRACE_MSG(TRACE_ERROR, "ZB_ZCL_CMD_WRITE_ATTRIB_UNDIV command received", (FMT__0));
      resp_status = ZB_ZCL_STATUS_FAIL;

      endpoint = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).dst_endpoint;
      ep_desc = zb_af_get_endpoint_desc(endpoint);
      cluster_desc = get_cluster_desc(ep_desc, cmd_info->cluster_id,
                                      (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_SRV) ?
                                      ZB_ZCL_CLUSTER_SERVER_ROLE : ZB_ZCL_CLUSTER_CLIENT_ROLE);

      data_ptr = zb_buf_begin(buf);
      buf_len = zb_buf_len(buf);

      ZB_ZCL_GENERAL_GET_NEXT_WRITE_ATTR_REQ(data_ptr, buf_len, write_attr_req);

      while (write_attr_req)
      {
        attr_desc = zb_zcl_get_attr_desc(cluster_desc, write_attr_req->attr_id);
        attr_desc->access = ZB_ZCL_ATTR_ACCESS_READ_WRITE;
        ZB_ZCL_GENERAL_GET_NEXT_WRITE_ATTR_REQ(data_ptr, buf_len, write_attr_req);
      }

      resp_status = ZB_ZCL_STATUS_FAIL;
    }
    break;

    case ZB_ZCL_CMD_READ_ATTRIB:
      TRACE_MSG(TRACE_ERROR, "Unknown command received, cmd_id 0x%hx", (FMT__H, cmd_info->cmd_id));
      resp_status = ZB_ZCL_STATUS_FAIL;
      break;
    default:
      TRACE_MSG(TRACE_ERROR, "Unknown command received, cmd_id 0x%hx", (FMT__H, cmd_info->cmd_id));
      resp_status = ZB_ZCL_STATUS_FAIL;
      break;
  }

  TRACE_MSG(TRACE_APP1, "resp_stauts %d", (FMT__D, resp_status));
  return resp_status;
}


static void zb_zcl_custom_cluster_cmd1_handler(zb_bufid_t buf, zb_zcl_parsed_hdr_t *cmd_info)
{
  zb_zcl_parse_status_t parse_status;
  zb_zcl_custom_cluster_cmd1_req_t cmd1_req;

  ZB_ZCL_CUSTOM_CLUSTER_GET_CMD1_REQ(buf, cmd1_req, parse_status);

  if (parse_status == ZB_ZCL_PARSE_STATUS_SUCCESS)
  {
    if (cmd1_req.value < 0xFF
        && (cmd1_req.mode == ZB_ZCL_CUSTOM_CLUSTER_CMD1_MODE1
            || cmd1_req.mode == ZB_ZCL_CUSTOM_CLUSTER_CMD1_MODE2))
    {
      /* Implement the processing of the command here */

      TRACE_MSG(TRACE_APP1, "send cmd1 response", (FMT__0));
      ZB_ZCL_CUSTOM_CLUSTER_SEND_CMD1_RESP(
        buf,
        cmd_info->seq_number,
        ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr,
        ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
        ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint,
        ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).dst_endpoint,
        NULL,
        ZB_ZCL_STATUS_SUCCESS);
    }
    else
    {
      zb_zcl_custom_cluster_send_default_response(buf, cmd_info, ZB_ZCL_STATUS_INVALID_VALUE);
    }
  }
  else
  {
    zb_zcl_custom_cluster_send_default_response(buf, cmd_info, ZB_ZCL_STATUS_MALFORMED_CMD);
  }
}


static void zb_zcl_custom_cluster_cmd2_handler(zb_bufid_t buf, zb_zcl_parsed_hdr_t *cmd_info)
{
  zb_zcl_parse_status_t parse_status;
  zb_zcl_custom_cluster_cmd2_req_t cmd2_req;

  ZB_ZCL_CUSTOM_CLUSTER_GET_CMD2_REQ(buf, cmd2_req, parse_status);

  if (parse_status == ZB_ZCL_PARSE_STATUS_SUCCESS)
  {
    if (cmd2_req.value < 0xFFFF
        && cmd2_req.param >= ZB_ZCL_CUSTOM_CLUSTER_CMD2_PARAM1
        && cmd2_req.param <= ZB_ZCL_CUSTOM_CLUSTER_CMD2_PARAM4)
    {
      /* Implement the processing of the command here */

      TRACE_MSG(TRACE_APP1, "send cmd2 response", (FMT__0));
      ZB_ZCL_CUSTOM_CLUSTER_SEND_CMD2_RESP(
        buf,
        cmd_info->seq_number,
        ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr,
        ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
        ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint,
        ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).dst_endpoint,
        NULL,
        ZB_ZCL_STATUS_SUCCESS);
    }
    else
    {
      zb_zcl_custom_cluster_send_default_response(buf, cmd_info, ZB_ZCL_STATUS_INVALID_VALUE);
    }
  }
  else
  {
    zb_zcl_custom_cluster_send_default_response(buf, cmd_info, ZB_ZCL_STATUS_MALFORMED_CMD);
  }
}


static void zb_zcl_custom_cluster_cmd3_handler(zb_bufid_t buf, zb_zcl_parsed_hdr_t *cmd_info)
{
  zb_zcl_parse_status_t parse_status;
  zb_zcl_custom_cluster_cmd3_req_t cmd3_req;

  ZB_ZCL_CUSTOM_CLUSTER_GET_CMD3_REQ(buf, cmd3_req, parse_status);

  if (parse_status == ZB_ZCL_PARSE_STATUS_SUCCESS)
  {
    if ((zb_uint8_t)cmd3_req.zcl_str[0]
        < ZB_ZCL_CUSTOM_CLUSTER_ATTR_CHAR_STRING_MAX_SIZE)
    {
      /* Implement the processing of the command here */

      zb_zcl_custom_cluster_send_default_response(buf, cmd_info, ZB_ZCL_STATUS_SUCCESS);
    }
    else
    {
      zb_zcl_custom_cluster_send_default_response(buf, cmd_info, ZB_ZCL_STATUS_INVALID_VALUE);
    }
  }
  else
  {
    zb_zcl_custom_cluster_send_default_response(buf, cmd_info, ZB_ZCL_STATUS_MALFORMED_CMD);
  }
}


void zboss_signal_handler(zb_uint8_t param)
{
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_t sig = zb_get_app_signal(param, &sg_p);

  TRACE_MSG(TRACE_APP1, ">> zboss_signal_handler: status %hd signal %hd",
            (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
        zb_bdb_finding_binding_target(ENDPOINT_SERVER);
        break;

      case ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED:
        TRACE_MSG(TRACE_APP1, "Finding&binding done", (FMT__0));
        break;

      default:
        TRACE_MSG(TRACE_ERROR, "Unknown signal %hd", (FMT__H, sig));
        break;
    }
  }
  else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
  {
    TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR,
              "Device started FAILED status %d",
              (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
  }

  if (param)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_APP1, "<< zboss_signal_handler", (FMT__0));
}
