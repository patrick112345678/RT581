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
/*  PURPOSE: Tools to configure aging and keep-alive for end device
*/
#define ZB_TRACE_FILE_ID 2163
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zdo_common.h"
#include "zb_nwk_ed_aging.h"

zb_uint8_t zdo_get_aging_timeout()
{
    return ZB_GET_ED_TIMEOUT();
}

void zdo_set_aging_timeout(zb_uint8_t timeout)
{
    ZVUNUSED(timeout);
    zb_set_ed_timeout(timeout);
    zb_set_keepalive_timeout(zb_nwk_get_default_keepalive_timeout());
}

zb_time_t zdo_get_ed_keepalive_timeout()
{
    return ZB_GET_KEEPALIVE_TIMEOUT();
}
