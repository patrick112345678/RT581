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
/* PURPOSE: P/154/MAC/CHANNEL-ACCESS-03 DUT functionality for ti1352 build
*/

#define ZB_TRACE_FILE_ID 63883
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"

#include "tp_154_mac_channel_access_03_dut_hal.h"

void test_hal_indicate_action_ti1352(test_action_t event)
{
    TRACE_MSG(TRACE_APP1, "test_hal_indicate_action_ti1352(): event %hd", (FMT__D, event));
    /* TODO: */
}
