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
/* PURPOSE: Definitions common to all R22 regression tests
*/

#define ZB_TRACE_FILE_ID 40105
#include "zb_common.h"
#include "zb_aps.h"
#include "zb_reg_test_globals.h"

#ifndef ZB_MULTI_TEST
static zb_test_ctx_t s_test_ctx;
#endif

void zb_reg_test_set_init_globals()
{
#ifdef ZB_MULTI_TEST
    zb_test_ctx_t *test_ctx = zb_multitest_get_test_ctx();
#else
    zb_test_ctx_t *test_ctx = &s_test_ctx;
    ZB_BZERO(test_ctx, sizeof(test_ctx));
#endif

    test_ctx->page = ZB_REG_TEST_DEAULT_PAGE;
    test_ctx->channel = ZB_REG_TEST_DEAULT_CHANNEL;
    test_ctx->mode = ZB_REG_TEST_DEAULT_MODE;
}


void zb_reg_test_set_common_channel_settings()
{
#ifndef NCP_MODE_HOST
    zb_channel_list_t channel_list;
    zb_channel_list_init(channel_list);
    zb_channel_list_add(channel_list, TEST_PAGE, (1L << TEST_CHANNEL));

    zb_channel_page_list_copy(ZB_AIB().aps_channel_mask_list, channel_list);
#else
    /* TODO: use NCP API for this settings  */
#endif
}


void zb_reg_test_set_zc_role()
{
#ifndef NCP_MODE_HOST
    ZB_AIB().aps_designated_coordinator = ZB_TRUE;
    ZB_NIB().device_type = ZB_NWK_DEVICE_TYPE_COORDINATOR;
#else
    ZB_ASSERT(ZB_FALSE && "TODO: use NCP API for this settings");
#endif
}


void zb_reg_test_set_zr_role()
{
#ifndef NCP_MODE_HOST
    ZB_AIB().aps_designated_coordinator = ZB_FALSE;
    ZB_NIB().device_type = ZB_NWK_DEVICE_TYPE_ROUTER;
#else
    ZB_ASSERT(ZB_FALSE && "TODO: use NCP API for this settings");
#endif
}


void zb_reg_test_set_zed_role()
{
#ifndef NCP_MODE_HOST
    ZB_AIB().aps_designated_coordinator = ZB_FALSE;
    ZB_NIB().device_type = ZB_NWK_DEVICE_TYPE_ED;
#else
    ZB_ASSERT(ZB_FALSE && "TODO: use NCP API for this settings");
#endif
}

#ifdef NCP_MODE_HOST


void fill_nldereq(zb_uint8_t param, zb_uint16_t addr, zb_uint8_t secure)
{
    ZVUNUSED(param);
    ZVUNUSED(addr);
    ZVUNUSED(secure);

    ZB_ASSERT(ZB_FALSE);
}

void zb_nlde_data_request(zb_uint8_t param)
{
    ZVUNUSED(param);

    ZB_ASSERT(ZB_FALSE);
}

void zb_nwk_change_me_addr(zb_uint8_t param)
{
    ZVUNUSED(param);
    ZB_ASSERT(ZB_FALSE);
}


void mac_remove_invisible_short(zb_uint16_t addr)
{
    ZVUNUSED(addr);
    ZB_ASSERT(ZB_FALSE);
}


void zb_nwk_set_address_assignment_cb(zb_addr_assignment_cb_t cb)
{
    ZVUNUSED(cb);
    ZB_ASSERT(ZB_FALSE);
}


void zb_secur_apsme_request_key(zb_uint8_t param)
{
    ZVUNUSED(param);
    ZB_ASSERT(ZB_FALSE);
}


void zb_tp_buffer_test_request(zb_uint8_t param, zb_callback_t cb)
{
    ZVUNUSED(param);
    ZVUNUSED(cb);
    ZB_ASSERT(ZB_FALSE);
}


void zb_zdo_register_device_annce_cb(zb_device_annce_cb_t cb)
{
    ZVUNUSED(cb);
    ZB_ASSERT(ZB_FALSE);
}


void aps_data_hdr_fill_datareq(zb_uint8_t fc, zb_apsde_data_req_t *req, zb_bufid_t param)
{
    ZVUNUSED(fc);
    ZVUNUSED(req);
    ZVUNUSED(param);

    ZB_ASSERT(ZB_FALSE);
}


void zb_aib_tcpol_set_authenticate_always(zb_bool_t authenticate_always)
{
    ZVUNUSED(authenticate_always);

    ZB_ASSERT(ZB_FALSE);
}

#endif /* NCP_MODE_HOST */
