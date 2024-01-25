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
/*  PURPOSE: SE specific commissioning formation
*/

#define ZB_TRACE_FILE_ID 102
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_nwk_nib.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_nvram.h"

#if defined ZB_SE_COMMISSIONING && defined ZB_FORMATION

static void se_commissioning_formation_channels_mask(zb_channel_list_t list)
{
    se_pref_channels_create_mask((zb_bool_t)(ZSE_CTXC().commissioning.formation_retries == 0), list);
}


static void se_start_formation(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ZDO1, "SE start Formation %hd", (FMT__H, param));

    /* SE uses centrilized security, so always ZC. */
    ZB_ASSERT(zb_get_device_type() == ZB_NWK_DEVICE_TYPE_COORDINATOR);
    ZSE_CTXC().commissioning.state = SE_STATE_FORMATION;
    ZSE_CTXC().commissioning.formation_retries = 0;
    ZSE_CTXC().commissioning.startup_control = ZSE_STARTUP_UNCOMMISSIONED;
    se_minimal_tc_init();
    ZB_SCHEDULE_CALLBACK(zdo_start_formation, param);
}


static void se_formation_force_link(void)
{
    zdo_formation_force_link();

    FORMATION_SELECTOR().start_formation = se_start_formation;
    FORMATION_SELECTOR().get_formation_channels_mask = se_commissioning_formation_channels_mask;
}


#ifdef ZB_COORDINATOR_ROLE

void zb_se_set_network_coordinator_role(zb_uint32_t channel_mask)
{
    se_commissioning_force_link();
    se_formation_force_link();
    zb_set_network_coordinator_role_with_mode(channel_mask, ZB_COMMISSIONING_SE);
}

#endif /* ZB_COORDINATOR_ROLE */

#endif /* ZB_SE_COMMISSIONING && ZB_FORMATION */
