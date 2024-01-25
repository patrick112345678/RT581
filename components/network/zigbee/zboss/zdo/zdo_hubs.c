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
/* PURPOSE: Routines to support WWAH moved there from ZCL
*/

#define ZB_TRACE_FILE_ID 20100
#include "zb_common.h"

#ifdef ZB_ZCL_SUPPORT_CLUSTER_WWAH

#ifdef ZB_ZCL_ENABLE_WWAH_SERVER
/*
 * Check if nwk key routines allowed
 * @return ZB_TRUE if new PANID is allowed
 */
zb_bool_t zb_wwah_check_nwk_key_commands_broadcast_allowed(void)
{
  /* Quick simpoe check fpor ZC role - here and below. It will also cut extra code for ZC-only build. */
  return ZB_IS_DEVICE_ZC() || !HUBS_CTX().key_commands_broadcast_disallowed;
}


void zb_wwah_set_nwk_key_commands_broadcast_allowed(zb_bool_t allowed)
{
  HUBS_CTX().key_commands_broadcast_disallowed = !allowed;
}


zb_bool_t zb_wwah_in_configuration_mode(void)
{
  return ZB_IS_DEVICE_ZC() || !HUBS_CTX().configuration_mode_disabled;
}


void zb_wwah_set_configuration_mode(zb_bool_t allowed)
{
  HUBS_CTX().configuration_mode_disabled = !allowed;
}


/*
 * Checks ZDO command. See ConfigurationModeEnabled attribute specification.
 * @return ZB_TRUE if ZDO command allowed by WWAH
 * @return ZB_FALSE otherwise
 */
zb_bool_t zb_wwah_check_zdo_command(zb_apsde_data_indication_t *di)
{
  zb_bool_t ret = ZB_TRUE;

  if (!ZB_IS_DEVICE_ZC() && zb_is_wwah_server())
  {
    /* if ConfigurationMode is disabled */
    if (!zb_wwah_in_configuration_mode())
    {
      /* if ZDO command have not been encrypted using the Trust Center Link key */
      if (!di->aps_key_from_tc)
      {
        /* if ZDO command not in exemption list*/
        if (di->clusterid != ZDO_NWK_ADDR_REQ_CLID && di->clusterid != ZDO_IEEE_ADDR_REQ_CLID &&
            di->clusterid != ZDO_NODE_DESC_REQ_CLID && di->clusterid != ZDO_POWER_DESC_REQ_CLID &&
            di->clusterid != ZDO_SIMPLE_DESC_REQ_CLID && di->clusterid != ZDO_ACTIVE_EP_REQ_CLID &&
            di->clusterid != ZDO_MATCH_DESC_REQ_CLID && di->clusterid != ZDO_COMPLEX_DESC_REQ_CLID &&
            di->clusterid != ZDO_USER_DESC_REQ_CLID && di->clusterid != ZDO_DEVICE_ANNCE_CLID &&
#ifndef R23_DISABLE_DEPRECATED_ZDO_CMDS
            di->clusterid != ZDO_SYSTEM_SERVER_DISCOVERY_REQ_CLID &&
            di->clusterid != ZDO_EXTENDED_SIMPLE_DESC_REQ_CLID &&
            di->clusterid != ZDO_EXTENDED_ACTIVE_EP_REQ_CLID &&
            di->clusterid != ZDO_MGMT_RTG_REQ_CLID &&
#endif
            di->clusterid != ZDO_PARENT_ANNCE_CLID &&
            di->clusterid != ZDO_MGMT_LQI_REQ_CLID &&
            di->clusterid != ZDO_MGMT_NWK_UPDATE_REQ_CLID && di->clusterid != ZDO_MGMT_NWK_ENHANCED_UPDATE_REQ_CLID &&
            di->clusterid != ZDO_MGMT_NWK_IEEE_JOINING_LIST_REQ_CLID && di->clusterid != ZDO_NWK_ADDR_RESP_CLID &&
            di->clusterid != ZDO_IEEE_ADDR_RESP_CLID && di->clusterid != ZDO_NODE_DESC_RESP_CLID &&
            di->clusterid != ZDO_POWER_DESC_RESP_CLID && di->clusterid != ZDO_SIMPLE_DESC_RESP_CLID &&
            di->clusterid != ZDO_ACTIVE_EP_RESP_CLID && di->clusterid != ZDO_MATCH_DESC_RESP_CLID &&
            di->clusterid != ZDO_COMPLEX_DESC_RESP_CLID && di->clusterid != ZDO_USER_DESC_RESP_CLID)
        {
          ret = ZB_FALSE;
        }
      }
    }
  }
  return ret;
}


/*
 * Check if leave without rejoin allowed
 * @return ZB_TRUE if leave without rejoin is allowed
 */
zb_bool_t zb_wwah_check_if_leave_without_rejoin_allowed(void)
{
  return ZB_IS_DEVICE_ZC() || !HUBS_CTX().leave_without_rejoin_disallowed;
}


void zb_wwah_set_leave_without_rejoin_allowed(zb_bool_t allowed)
{
  HUBS_CTX().leave_without_rejoin_disallowed = !allowed;
}


zb_bool_t zb_wwah_check_if_interpan_supported(void)
{
  return ZB_IS_DEVICE_ZC() || !HUBS_CTX().interpan_disabled;
}


void zb_wwah_set_interpan_supported(zb_bool_t enabled)
{
  HUBS_CTX().interpan_disabled = !enabled;
}
#endif  /* #ifdef ZB_ZCL_ENABLE_WWAH_SERVER */

#endif  /* #ifdef ZB_ZCL_SUPPORT_CLUSTER_WWAH */
