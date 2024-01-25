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
/* PURPOSE: ZCL WWAH cluster specific commands handling - Client role
*/

#define ZB_TRACE_FILE_ID 12083

#include "zb_common.h"

#if defined ZB_ZCL_SUPPORT_CLUSTER_WWAH

#ifdef ZB_ZCL_ENABLE_WWAH_CLIENT

static const zb_uint8_t gs_wwah_client_received_commands[] =
{
  ZB_ZCL_CLUSTER_ID_WWAH_CLIENT_ROLE_RECEIVED_CMD_LIST
};

static const zb_uint8_t gs_wwah_client_generated_commands[] =
{
  ZB_ZCL_CLUSTER_ID_WWAH_CLIENT_ROLE_GENERATED_CMD_LIST
};

zb_discover_cmd_list_t gs_wwah_client_cmd_list =
{
  sizeof(gs_wwah_client_received_commands), (zb_uint8_t *)gs_wwah_client_received_commands,
  sizeof(gs_wwah_client_generated_commands), (zb_uint8_t *)gs_wwah_client_generated_commands
};

zb_bool_t zb_zcl_process_wwah_specific_commands_cli(zb_uint8_t param);

void zb_zcl_wwah_init_client()
{
  TRACE_MSG(TRACE_ZCL1, "> zb_zcl_wwah_init_client", (FMT__0));
  zb_zcl_add_cluster_handlers(ZB_ZCL_CLUSTER_ID_WWAH,
                              ZB_ZCL_CLUSTER_CLIENT_ROLE,
                              (zb_zcl_cluster_check_value_t)NULL,
                              (zb_zcl_cluster_write_attr_hook_t)NULL,
                              zb_zcl_process_wwah_specific_commands_cli);
}

zb_bool_t zb_zcl_process_wwah_specific_commands_cli(zb_uint8_t param)
{
  if ( ZB_ZCL_GENERAL_GET_CMD_LISTS_PARAM == param )
  {
    ZCL_CTX().zb_zcl_cluster_cmd_list = &gs_wwah_client_cmd_list;
    return ZB_TRUE;
  }
  return ZB_FALSE;
}

#endif  /* ZB_ZCL_ENABLE_WWAH_CLIENT */

#endif /* defined ZB_ZCL_SUPPORT_CLUSTER_WWAH */
