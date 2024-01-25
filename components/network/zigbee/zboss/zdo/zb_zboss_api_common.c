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
/*  PURPOSE: ZBOSS core API
*/

#define ZB_TRACE_FILE_ID 61
#include "zb_common.h"
#include "zb_bdb_internal.h"
#include "zb_scheduler.h"
#include "zb_nwk.h"
#include "zboss_api.h"
#include "zb_zdo.h"
#include "zb_aps.h"
#include "zb_bdb_internal.h"


#ifdef ZB_COORDINATOR_ROLE
void zb_set_network_coordinator_role_with_mode(zb_uint32_t channel_mask, zb_uint8_t mode)
{
  zb_set_nwk_role_mode_common(ZB_NWK_DEVICE_TYPE_COORDINATOR,
                                       channel_mask,
                                       (zb_commissioning_type_t)mode);
}
#endif  /* ZB_COORDINATOR_ROLE */

#ifdef ZB_ROUTER_ROLE
void zb_set_network_router_role_with_mode(zb_uint32_t channel_mask, zb_commissioning_type_t bdb_mode)
{
  zb_set_nwk_role_mode_common(ZB_NWK_DEVICE_TYPE_ROUTER, channel_mask, bdb_mode);
}
#endif

#ifdef ZB_ED_FUNC
void zb_set_network_ed_role_with_mode(zb_uint32_t channel_mask, zb_commissioning_type_t bdb_mode)
{
  zb_set_nwk_role_mode_common(ZB_NWK_DEVICE_TYPE_ED, channel_mask, bdb_mode);
}
#endif  /* ZB_ED_FUNC */


void zboss_main_loop()
{
  while (!ZB_SCHEDULER_IS_STOP())
  {
    zb_sched_loop_iteration();
  }
}

void zboss_main_loop_iteration()
{
  zb_sched_loop_iteration();
}

zb_nwk_device_type_t zb_get_network_role(void)
{
  return zb_get_device_type();
}

#ifdef ZB_JOIN_CLIENT
void zb_bdb_reset_via_local_action(zb_uint8_t param)
{
  zb_zdo_mgmt_leave_param_t *req_param;

  if (param == 0U)
  {
    zb_ret_t ret = zb_buf_get_out_delayed(zb_bdb_reset_via_local_action);
    if (ret != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
    }
  }
  else
  {
    TRACE_MSG(TRACE_APP1, "zb_bdb_reset_via_local_action param %hd: send leave to myself", (FMT__H, param));

    req_param = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_leave_param_t);
    ZB_BZERO(req_param, sizeof(zb_zdo_mgmt_leave_param_t));

    zb_get_long_address(req_param->device_address);
    req_param->dst_addr = zb_get_short_address();
    req_param->rejoin = ZB_FALSE;
    req_param->remove_children = ZB_FALSE;
    (void)zdo_mgmt_leave_req(param, NULL);
  }
}
#endif  /* ZB_JOIN_CLIENT */
