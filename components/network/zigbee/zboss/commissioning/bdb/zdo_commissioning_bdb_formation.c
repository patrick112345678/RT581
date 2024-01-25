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
/*  PURPOSE: BDD specific commissioning. Formation part.
*/

#define ZB_TRACE_FILE_ID 99

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
#include "zb_bdb_internal.h"


#if defined ZB_FORMATION

static void bdb_formation(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ZDO3, "bdb_formation param %hd", (FMT__H, param));

    /* See Figure 5 - Network formation procedure */

    if (ZB_BDB().v_do_primary_scan == ZB_BDB_JOIN_MACHINE_PRIMARY_SCAN
            && ZB_BDB().bdb_primary_channel_set != 0)
    {
        ZB_BDB().v_do_primary_scan = ZB_BDB_JOIN_MACHINE_SECONDARY_SCAN_START;
        ZB_BDB().bdb_commissioning_status = ZB_BDB_STATUS_IN_PROGRESS;
        ZB_BDB().v_scan_channels = ZB_BDB().bdb_primary_channel_set;
        TRACE_MSG(TRACE_ZDO1, "Doing formation channel mask 0x%lx", (FMT__L, ZB_BDB().v_scan_channels));
        ZB_SCHEDULE_CALLBACK(zdo_start_formation, param);
    }
    else if ((ZB_BDB().v_do_primary_scan == ZB_BDB_JOIN_MACHINE_SECONDARY_SCAN_START
              && ZB_BDB().bdb_secondary_channel_set != 0)
             ||
             (ZB_BDB().v_do_primary_scan == ZB_BDB_JOIN_MACHINE_PRIMARY_SCAN
              && ZB_BDB().bdb_secondary_channel_set != 0
              && ZB_BDB().bdb_primary_channel_set == 0))
    {
        ZB_BDB().bdb_commissioning_status = ZB_BDB_STATUS_IN_PROGRESS;
        ZB_BDB().v_do_primary_scan = ZB_BDB_JOIN_MACHINE_SECONDARY_SCAN_DONE;
        ZB_BDB().v_scan_channels = ZB_BDB().bdb_secondary_channel_set;
        TRACE_MSG(TRACE_ZDO1, "Doing formation channel mask 0x%lx", (FMT__L, ZB_BDB().v_scan_channels));
        ZB_SCHEDULE_CALLBACK(zdo_start_formation, param);
    }
    else
    {
        ZB_BDB().bdb_commissioning_status = ZB_BDB_STATUS_FORMATION_FAILURE;
        TRACE_MSG(TRACE_ZDO1, "BDB: Formation failed run next bdb machine step", (FMT__0));
        ZB_BDB().v_do_primary_scan = ZB_BDB_JOIN_MACHINE_PRIMARY_SCAN;
        ZB_BDB().bdb_commissioning_step <<= 1;
        ZB_SCHEDULE_CALLBACK(bdb_commissioning_machine, param);
    }
}


static void bdb_commissioning_formation_channels_mask(zb_channel_list_t list)
{
    zb_uint32_t mask;

#if defined ZB_SUB_GHZ_ZB30_SUPPORT
    zb_uint8_t used_page;
#endif /* ZB_SUB_GHZ_ZB30_SUPPORT */

    mask = ZB_BDB().v_scan_channels;

    zb_channel_list_init(list);

#if defined ZB_SUB_GHZ_ZB30_SUPPORT
    used_page = zb_aib_channel_page_list_get_first_filled_page();
    zb_channel_page_list_set_mask(list, used_page, mask);
#else
    zb_channel_page_list_set_mask(list, ZB_CHANNEL_LIST_PAGE0_IDX, mask);
#endif /* !ZB_SUB_GHZ_ZB30_SUPPORT */

}


static void bdb_formation_force_link(void)
{
    zdo_formation_force_link();

    FORMATION_SELECTOR().start_formation = bdb_formation;
    FORMATION_SELECTOR().get_formation_channels_mask = bdb_commissioning_formation_channels_mask;
}

#ifdef ZB_COORDINATOR_ROLE

void zb_set_network_coordinator_role(zb_uint32_t channel_mask)
{
    bdb_force_link();
    bdb_formation_force_link();
    zb_set_network_coordinator_role_with_mode(channel_mask, ZB_COMMISSIONING_BDB);
}

void zb_set_network_coordinator_role_ext(zb_channel_list_t channel_list)
{
    bdb_force_link();
    bdb_formation_force_link();
    zb_set_nwk_role_mode_common_ext(ZB_NWK_DEVICE_TYPE_COORDINATOR,
                                    channel_list,
                                    ZB_COMMISSIONING_BDB);
}

#endif /* ZB_COORDINATOR_ROLE */


#ifdef ZB_DISTRIBUTED_SECURITY_ON

void zb_enable_distributed(void)
{
    bdb_formation_force_link();
}

#endif /* ZB_DISTRIBUTED_SECURITY_ON */

#endif /* ZB_FORMATION */
