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
/* PURPOSE: ZCL OTA Cluster - routines which always linked because called from the main ZCL logic.
*/

#define ZB_TRACE_FILE_ID 45

#include "zb_common.h"

/** @internal
    @{
*/

#if defined (ZB_ZCL_SUPPORT_CLUSTER_OTA_UPGRADE) || defined DOXYGEN

#include "zb_time.h"
#include "zb_zdo.h"
#include "zb_zcl.h"
#include "zcl/zb_zcl_ota_upgrade.h"
#include "zb_zdo.h"
#include "zb_aps.h"
#include "zdo_wwah_stubs.h"

/* That code may be called from zcl_poll_control_commands.c, so linked always.*/
zb_uint8_t zb_zcl_ota_upgrade_get_ota_status(zb_uint8_t endpoint)
{
  zb_zcl_attr_t *attr_desc;
  zb_uint8_t status;

  TRACE_MSG(TRACE_ZCL1, "> zb_zcl_ota_upgrade_get_ota_status endpoint %hd", (FMT__H, endpoint));

  attr_desc = zb_zcl_get_attr_desc_a(endpoint,
       ZB_ZCL_CLUSTER_ID_OTA_UPGRADE, ZB_ZCL_CLUSTER_CLIENT_ROLE, ZB_ZCL_ATTR_OTA_UPGRADE_IMAGE_STATUS_ID);
  ZB_ASSERT(attr_desc);

  status = ZB_ZCL_GET_ATTRIBUTE_VAL_8(attr_desc);

  TRACE_MSG(TRACE_ZCL1, "< zb_zcl_ota_upgrade_get_ota_status %hd", (FMT__H, status));

  return status;
}


void zcl_ota_abort_and_set_tc(zb_uint8_t param)
{
  zb_zcl_cluster_handler_t cluster_handler;

  cluster_handler = zb_zcl_get_cluster_handler(ZB_ZCL_CLUSTER_ID_OTA_UPGRADE,
                                               ZB_ZCL_CLUSTER_CLIENT_ROLE);
  if (cluster_handler)
  {
    zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);
    cmd_info->disable_default_response = ZB_TRUE;
    cmd_info->cluster_id = ZB_ZCL_CLUSTER_ID_OTA_UPGRADE;
    cmd_info->cmd_id = ZB_ZCL_CMD_OTA_UPGRADE_INTERNAL_ABORT_ID;
    cluster_handler(param);
  }
  else
  {
    zb_buf_free(param);
  }
}

#endif /* defined ZB_ZCL_SUPPORT_CLUSTER_OTA_UPGRADE || defined DOXYGEN */
