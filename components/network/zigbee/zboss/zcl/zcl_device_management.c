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
/* PURPOSE: Device management cluster implementation.
*/

#define ZB_TRACE_FILE_ID 88

#include "zb_common.h"

#if defined (ZB_ZCL_SUPPORT_CLUSTER_DEVICE_MANAGEMENT)

#include "zboss_api.h"
#include "zcl/zb_zcl_device_management.h"

static zb_ret_t check_value_device_management(zb_uint16_t attr_id, zb_uint8_t endpoint, zb_uint8_t *value);

void zb_zcl_device_management_init_server(void)
{
  zb_zcl_add_cluster_handlers(ZB_ZCL_CLUSTER_ID_DEVICE_MANAGEMENT,
                              ZB_ZCL_CLUSTER_SERVER_ROLE,
                              check_value_device_management,
                              (zb_zcl_cluster_write_attr_hook_t)NULL,
                              (zb_zcl_cluster_handler_t)NULL);
}


void zb_zcl_device_management_init_client(void)
{
  zb_zcl_add_cluster_handlers(ZB_ZCL_CLUSTER_ID_DEVICE_MANAGEMENT,
                              ZB_ZCL_CLUSTER_CLIENT_ROLE,
                              check_value_device_management,
                              (zb_zcl_cluster_write_attr_hook_t)NULL,
                              (zb_zcl_cluster_handler_t)NULL);
}


static zb_ret_t check_value_device_management(zb_uint16_t attr_id, zb_uint8_t endpoint, zb_uint8_t *value)
{
  ZVUNUSED(attr_id);
  ZVUNUSED(value);
  ZVUNUSED(endpoint);

  /* All values for mandatory attributes are allowed, extra check for
   * optional attributes is needed */

  return RET_OK;
}
#endif  /* ZB_ZCL_SUPPORT_CLUSTER_DEVICE_MANAGEMENT */
