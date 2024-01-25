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
/* PURPOSE: TH ZED
*/
#define ZB_TEST_NAME RTP_NWK_02_TH_ZED

#define ZB_TRACE_FILE_ID 40278
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"

#include "rtp_nwk_02_common.h"
#include "../common/zb_reg_test_globals.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_ED_ROLE
#error ED role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error ZB_USE_NVRAM is not compiled!
#endif

#ifndef ZB_LIMIT_VISIBILITY
#error define ZB_LIMIT_VISIBILITY
#endif

static zb_uint8_t g_nwk_key[16] = ZB_REG_TEST_DEFAULT_NWK_KEY;
static zb_ieee_addr_t g_ieee_addr_th_zed = IEEE_ADDR_TH_ZED;

/* External stack function */
void aps_data_hdr_fill_datareq(zb_uint8_t fc, zb_apsde_data_req_t *req, zb_bufid_t param);

static void trigger_steering(zb_uint8_t unused);
static void send_buffer_test_req_manually_delayed(zb_uint8_t unused);
static void send_buffer_test_req_manually(zb_uint8_t param);

static zb_uint8_t g_buffer_test_req_len = 10;
static zb_uint16_t g_buffer_test_req_dst_addr = 0x0000;
static zb_bool_t g_buffer_test_req_discovery_route = ZB_FALSE;

MAIN()
{
    ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
    ZB_SET_TRACE_LEVEL(4);
    ARGV_UNUSED;

    ZB_INIT("zdo_th_zed");

    zb_set_long_address(g_ieee_addr_th_zed);

    zb_secur_setup_nwk_key((zb_uint8_t *) g_nwk_key, 0);

    zb_reg_test_set_common_channel_settings();
    zb_set_network_ed_role((1l << TEST_CHANNEL));
    zb_set_nvram_erase_at_start(ZB_TRUE);

    mac_add_invisible_short(0x0000);
    mac_add_invisible_short(0x0001);

    if (zboss_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
    }
    else
    {
        zdo_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    TRACE_MSG(TRACE_APS1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    switch (sig)
    {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));
        if (status == 0)
        {
            ZB_SCHEDULE_CALLBACK(trigger_steering, 0);

            test_step_register(send_buffer_test_req_manually_delayed, 0, RTP_NWK_02_STEP_1_TIME_ZED);

            test_control_start(TEST_MODE, RTP_NWK_02_STEP_1_DELAY_ZED);
        }
        break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
        break; /* ZB_BDB_SIGNAL_STEERING */

    case ZB_COMMON_SIGNAL_CAN_SLEEP:
        TRACE_MSG(TRACE_APS1, "signal: ZB_COMMON_SIGNAL_CAN_SLEEP, status %d", (FMT__D, status));
        if (status == 0)
        {
            zb_sleep_now();
        }
        break; /* ZB_COMMON_SIGNAL_CAN_SLEEP */

    default:
        TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}

static void trigger_steering(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
}

static void send_buffer_test_req_manually_delayed(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    zb_buf_get_out_delayed(send_buffer_test_req_manually);
}

static void send_buffer_test_req_manually(zb_uint8_t param)
{
    int i;
    zb_buffer_test_req_t *req;
    zb_ret_t ret = RET_OK;
    zb_uint8_t fc = 0;
    zb_nlde_data_req_t nldereq;
    zb_apsde_data_req_t *apsreq;
    zb_apsde_data_req_t *dreq;
    zb_address_ieee_ref_t addr_ref;

    req = zb_buf_initial_alloc(param, sizeof(zb_buffer_test_req_t) + g_buffer_test_req_len - 1);
    req->len = g_buffer_test_req_len;

    for (i = 0; i < g_buffer_test_req_len; ++i)
    {
        req->req_data[i] = i;
    }

#ifndef NCP_MODE_HOST
    ZG->zdo.test_prof_ctx.zb_tp_buffer_test_request.user_cb = NULL;
#else
    ZB_ASSERT(ZB_FALSE && "TODO: enable test profile for NCP");
#endif

    dreq = ZB_BUF_GET_PARAM(param, zb_apsde_data_req_t);

    TRACE_MSG(TRACE_ZDO2, "tp_send_req_by_short param %hd", (FMT__H, param));
    ZB_BZERO(dreq, sizeof(*dreq));

    dreq->profileid = ZB_TEST_PROFILE_ID;
    dreq->clusterid = TP_BUFFER_TEST_REQUEST_CLID;
    dreq->dst_endpoint = ZB_TEST_PROFILE_EP;
    dreq->src_endpoint = ZB_TEST_PROFILE_EP;
    dreq->dst_addr.addr_short = g_buffer_test_req_dst_addr;
    dreq->tx_options = 0;
    dreq->tx_options |= ZB_APSDE_TX_OPT_SECURITY_ENABLED;
    dreq->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
    dreq->radius = MAX_NWK_RADIUS;

    apsreq = ZB_BUF_GET_PARAM(param, zb_apsde_data_req_t);

    nldereq.radius = apsreq->radius;
    nldereq.addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
    nldereq.nonmember_radius = 0; /* if multicast, get it from APS IB */
    nldereq.discovery_route = g_buffer_test_req_discovery_route;

    zb_buf_flags_or(param, ZB_BUF_HAS_APS_PAYLOAD);

    ZB_APS_FC_SET_DELIVERY_MODE(fc, ZB_APS_DELIVERY_UNICAST);
    nldereq.dst_addr = apsreq->dst_addr.addr_short;

#ifdef ZB_USEALIAS
    nldereq.use_alias = apsreq->use_alias;
    nldereq.alias_src_addr = apsreq->alias_src_addr;
    nldereq.alias_seq_num = apsreq->alias_seq_num;
#endif

    ZB_APS_FC_SET_ACK_FORMAT(fc, 0);

    aps_data_hdr_fill_datareq(fc, apsreq, param);

    ZB_CHK_ARR(ZB_BUF_BEGIN(param), 8); /* check hdr fill */

    nldereq.security_enable = ZB_TRUE;
    nldereq.ndsu_handle = 0;

    ZB_MEMCPY(ZB_BUF_GET_PARAM(param, zb_nlde_data_req_t), &nldereq, sizeof(nldereq));

    ret = zb_address_by_short(nldereq.dst_addr, ZB_TRUE, ZB_TRUE, &addr_ref);
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_APS1, "Schedule packet to NWK", (FMT__0));
    ZB_SCHEDULE_CALLBACK(zb_nlde_data_request, param);
}

/*! @} */
