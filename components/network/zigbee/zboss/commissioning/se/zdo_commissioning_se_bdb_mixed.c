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
/*  PURPOSE: SE+BDB specific commissioning
*/

#define ZB_TRACE_FILE_ID 103


#include "zb_common.h"
#include "zb_bdb_internal.h"

#ifdef ZB_SE_BDB_MIXED

static void se_switch_to_bdb_commissioning(zb_bdb_commissioning_mode_mask_t step)
{
    bdb_force_link();

    ZB_TCPOL().update_trust_center_link_keys_required = ZB_TRUE;
    COMM_CTX().commissioning_type = ZB_COMMISSIONING_BDB;
    ZB_BDB().bdb_commissioning_status = ZB_BDB_STATUS_IN_PROGRESS;
    ZB_BDB().bdb_commissioning_step = step;

    /* Should we set ZB_BDB().bdb_application_signal here? */

    /* BDB formation is not initialized here. We don't need BDB formation logic for SE coordinator. */
}


void zb_se_set_bdb_mode_enabled(zb_bool_t enabled)
{
    ZSE_CTXC().commissioning.allow_bdb_in_se_mode = enabled;
    ZSE_CTXC().commissioning.switch_to_bdb_commissioning = se_switch_to_bdb_commissioning;

    /* Enable BDB-style keys update and verify if BDB in SE mode is allowed. */
    /* Note: setting this for a ZC means it will remove a joiner if it doesn't request
       a TCLK during ZB_TCPOL().trust_center_node_join_timeout. This could be undesirable behavior. */
    zb_aib_tcpol_set_update_trust_center_link_keys_required(enabled);
}

#endif /* ZB_SE_BDB_MIXED */
