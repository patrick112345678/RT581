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
/* PURPOSE: TH ZC
*/

#define ZB_TEST_NAME RTP_BDB_11_TH_ZC
#define ZB_TRACE_FILE_ID 40326

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"

#include "rtp_bdb_11_common.h"
#include "../common/zb_reg_test_globals.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error define ZB_USE_NVRAM
#endif

#ifndef ZB_REGRESSION_TESTS_API
#error define ZB_REGRESSION_TESTS_API
#endif

static zb_uint8_t g_nwk_key[16] = ZB_REG_TEST_DEFAULT_NWK_KEY;
static zb_ieee_addr_t g_ieee_addr_th_zc = IEEE_ADDR_TH_ZC;
static zb_ieee_addr_t g_ieee_addr_th_zr = IEEE_ADDR_TH_ZR;

static void test_trigger_steering(zb_uint8_t unused);
static void test_enable_key_sending(zb_uint8_t enable);

MAIN()
{
    ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
    ZB_SET_TRACE_LEVEL(4);
    ARGV_UNUSED;

    ZB_INIT("zdo_th_zc");

    zb_set_long_address(g_ieee_addr_th_zc);

    zb_set_pan_id(0x1aaa);

    zb_secur_setup_nwk_key((zb_uint8_t *) g_nwk_key, 0);

    zb_reg_test_set_common_channel_settings();
    zb_set_network_coordinator_role((1l << TEST_CHANNEL));
    zb_set_nvram_erase_at_start(ZB_TRUE);

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
    zb_zdo_app_signal_hdr_t *sg_p = NULL;
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

    TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    switch (sig)
    {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));
        if (status == 0)
        {
            ZB_SCHEDULE_CALLBACK(test_trigger_steering, 0);
        }
        break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
        TRACE_MSG(TRACE_APP1, "signal: ZB_ZDO_SIGNAL_DEVICE_ANNCE, status %d", (FMT__D, status));
        if (status == 0)
        {
            zb_zdo_signal_device_annce_params_t *params =
                ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_annce_params_t);

            TRACE_MSG(TRACE_APS2, ">> ieee_addr  " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(params->ieee_addr)));
            TRACE_MSG(TRACE_APS2, ">> nwk_addr 0x%hx, caps %hd", (FMT__H_H, params->device_short_addr, params->capability));

            if (ZB_IEEE_ADDR_CMP(params->ieee_addr, g_ieee_addr_th_zr))
            {
                TRACE_MSG(TRACE_APS2, "TH ZR device annce", (FMT__0));

                test_enable_key_sending(ZB_FALSE);
            }
        }
        break; /* ZB_ZDO_SIGNAL_DEVICE_ANNCE */

    case ZB_ZDO_SIGNAL_DEVICE_UPDATE:
        TRACE_MSG(TRACE_APP1, "signal: ZB_ZDO_SIGNAL_DEVICE_UPDATE, status %d", (FMT__D, status));
        if (status == 0)
        {
        }
        break;

    default:
        TRACE_MSG(TRACE_APP1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}

static void test_trigger_steering(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
}


static void test_enable_key_sending(zb_uint8_t enable)
{
    ZB_REGRESSION_TESTS_API().disable_sending_nwk_key = !enable;
}

/*! @} */
