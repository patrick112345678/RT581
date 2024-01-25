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
/*  PURPOSE: NCP High level transport (adapters layer) implementation for the host side
*/
#define ZB_TRACE_FILE_ID 17514

#include "ncp_host_hl_transport_internal_api.h"
#include "ncp_host_soc_state.h"
#include "ncp_hl_proto.h"


void zb_secur_set_tc_rejoin_enabled(zb_bool_t enable)
{
    zb_ret_t ret;
    TRACE_MSG(TRACE_ZDO2, "zb_secur_set_tc_rejoin_enabled, enable %d",
              (FMT__D, enable));

    ret = ncp_host_set_tc_policy(NCP_HL_TC_POLICY_TC_REJOIN_ENABLED, enable);
    ZB_ASSERT(ret == RET_OK);
}


void zb_secur_set_ignore_tc_rejoin(zb_bool_t enable)
{
    zb_ret_t ret;

    TRACE_MSG(TRACE_ZDO2, "zb_secur_set_ignore_tc_rejoin, enable %d",
              (FMT__D, enable));

    ret = ncp_host_set_tc_policy(NCP_HL_TC_POLICY_IGNORE_TC_REJOIN, enable);
    ZB_ASSERT(ret == RET_OK);
}


void zb_zdo_set_aps_unsecure_join(zb_bool_t insecure_join)
{
    zb_ret_t ret;

    TRACE_MSG(TRACE_ZDO2, "zb_zdo_set_aps_unsecure_join, insecure_join %d",
              (FMT__D, insecure_join));

    ret = ncp_host_set_tc_policy(NCP_HL_TC_POLICY_APS_INSECURE_JOIN, insecure_join);
    ZB_ASSERT(ret == RET_OK);
}


void zb_zdo_disable_network_mgmt_channel_update(zb_bool_t disable)
{
    zb_ret_t ret;

    TRACE_MSG(TRACE_ZDO2, "zb_zdo_disable_network_mgmt_channel_update, disable %d",
              (FMT__D, disable));

    ret = ncp_host_set_tc_policy(NCP_HL_TC_POLICY_DISABLE_NWK_MGMT_CHANNEL_UPDATE, disable);
    ZB_ASSERT(ret == RET_OK);
}
