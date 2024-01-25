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
/* PURPOSE: Definitions common to all R22 certification tests
*/

#define ZB_TRACE_FILE_ID 40101
#include "zb_common.h"
#include "zb_aps.h"
#include "zb_cert_test_globals.h"

zb_r22_cert_test_ctx_t g_cert_test_ctx;

void zb_cert_test_set_init_globals()
{
    g_cert_test_ctx.page = ZB_CERT_TEST_DEAULT_PAGE;
    g_cert_test_ctx.channel = ZB_CERT_TEST_DEAULT_CHANNEL;
}


void zb_cert_test_set_common_channel_settings()
{
    zb_channel_list_t channel_list;
    zb_channel_list_init(channel_list);
    zb_channel_list_add(channel_list, TEST_PAGE, (1L << TEST_CHANNEL));
    zb_channel_page_list_copy(ZB_AIB().aps_channel_mask_list, channel_list);
}


void zb_cert_test_set_zc_role()
{
    ZB_AIB().aps_designated_coordinator = ZB_TRUE;
    ZB_NIB().device_type = ZB_NWK_DEVICE_TYPE_COORDINATOR;
}


void zb_cert_test_set_zr_role()
{
    ZB_AIB().aps_designated_coordinator = ZB_FALSE;
    ZB_NIB().device_type = ZB_NWK_DEVICE_TYPE_ROUTER;
}


void zb_cert_test_set_zed_role()
{
    ZB_AIB().aps_designated_coordinator = ZB_FALSE;
    ZB_NIB().device_type = ZB_NWK_DEVICE_TYPE_ED;
}
