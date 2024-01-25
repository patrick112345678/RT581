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
/* PURPOSE: DUT ZC
*/

#define ZB_TEST_NAME RTP_BDB_04_DUT_ZC

#define ZB_TRACE_FILE_ID 40422
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"

#include "rtp_bdb_04_common.h"
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

#ifndef ZB_SECURITY_INSTALLCODES
#error Define ZB_SECURITY_INSTALLCODES
#endif

static zb_uint8_t g_nwk_key[16] = ZB_REG_TEST_DEFAULT_NWK_KEY;
static zb_ieee_addr_t g_ieee_addr_dut_zc = IEEE_ADDR_DUT_ZC;
static zb_ieee_addr_t g_ieee_addr_th_zr1 = IEEE_ADDR_TH_ZR1;

static zb_uint8_t g_ic1[16 + 2] = ZB_REG_TEST_DEFAULT_INSTALL_CODE;

static void trigger_steering(zb_uint8_t unused);
static void zb_trace_device_update_signal(zb_zdo_signal_device_update_params_t *params);
static void zb_trace_device_authorized_signal(zb_zdo_signal_device_authorized_params_t *params);

MAIN()
{
    ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
    ZB_SET_TRACE_LEVEL(4);
    ARGV_UNUSED;

    ZB_INIT("zdo_dut_zc");

    zb_set_long_address(g_ieee_addr_dut_zc);

    zb_set_pan_id(0x1aaa);

    zb_secur_setup_nwk_key((zb_uint8_t *) g_nwk_key, 0);

    zb_reg_test_set_common_channel_settings();
    zb_set_network_coordinator_role((1l << TEST_CHANNEL));
    zb_set_nvram_erase_at_start(ZB_TRUE);
    zb_set_installcode_policy(ZB_TRUE);

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
    zb_zdo_app_signal_hdr_t *sg_p;
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

    TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    switch (sig)
    {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    {
        TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));
        if (status == 0)
        {
            ZB_SCHEDULE_CALLBACK(trigger_steering, 0);
            zb_secur_ic_add(g_ieee_addr_th_zr1, ZB_IC_TYPE_128, g_ic1, NULL);
        }
    }
        break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_ZDO_SIGNAL_DEVICE_UPDATE:
    {
        TRACE_MSG(TRACE_APP1, "signal: ZB_ZDO_SIGNAL_DEVICE_UPDATE, status %d", (FMT__D, status));
        if (status == 0)
        {
            zb_zdo_signal_device_update_params_t *params =
                ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_update_params_t);
            zb_trace_device_update_signal(params);
        }
    }
        break; /* ZB_ZDO_SIGNAL_DEVICE_UPDATE */

    case ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED:
    {
        TRACE_MSG(TRACE_APP1, "signal: ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED, status %d", (FMT__D, status));
        if (status == 0)
        {
            zb_zdo_signal_device_authorized_params_t *params =
                ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_authorized_params_t);
            zb_trace_device_authorized_signal(params);
        }
    }
        break; /* ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED */

    default:
        TRACE_MSG(TRACE_APP1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}

static void trigger_steering(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
}

static void zb_trace_device_update_signal(zb_zdo_signal_device_update_params_t *params)
{
    TRACE_MSG(TRACE_APP3, "ZB_ZDO_SIGNAL_DEVICE_UPDATE", (FMT__0));
    TRACE_MSG(TRACE_APP3, "long_addr: " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(params->long_addr)));
    TRACE_MSG(TRACE_APP3, "short_addr: 0x%x", (FMT__D, params->short_addr));

    switch (params->status)
    {
    case ZB_STD_SEQ_SECURED_REJOIN:
        TRACE_MSG(TRACE_APP3, "status: 0x%hx - SECURED_REJOIN", (FMT__H, params->status));
        break;
    case ZB_STD_SEQ_UNSECURED_JOIN:
        TRACE_MSG(TRACE_APP3, "status: 0x%hx - UNSECURED_JOIN", (FMT__H, params->status));
        break;
    case ZB_DEVICE_LEFT:
        TRACE_MSG(TRACE_APP3, "status: 0x%hx - DEVICE_LEFT", (FMT__H, params->status));
        break;
    case ZB_STD_SEQ_UNSECURED_REJOIN:
        TRACE_MSG(TRACE_APP3, "status: 0x%hx - UNSECURED_REJOIN", (FMT__H, params->status));
        break;
    case ZB_HIGH_SEQ_SECURED_REJOIN:
        TRACE_MSG(TRACE_APP3, "status: 0x%hx - SECURED_REJOIN", (FMT__H, params->status));
        break;
    case ZB_HIGH_SEQ_UNSECURED_JOIN:
        TRACE_MSG(TRACE_APP3, "status: 0x%hx - UNSECURED_JOIN", (FMT__H, params->status));
        break;
    case ZB_HIGH_SEQ_UNSECURED_REJOIN:
        TRACE_MSG(TRACE_APP3, "status: 0x%hx - UNSECURED_REJOIN", (FMT__H, params->status));
        break;
    default:
        TRACE_MSG(TRACE_ERROR, "status: 0x%hx - INVALID STATUS", (FMT__H, params->status));
        break;
    }
}

static void zb_trace_device_authorized_signal(zb_zdo_signal_device_authorized_params_t *params)
{
    TRACE_MSG(TRACE_APP3, "ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED", (FMT__0));
    TRACE_MSG(TRACE_APP3, "long_addr: " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(params->long_addr)));
    TRACE_MSG(TRACE_APP3, "short_addr: 0x%x", (FMT__D, params->short_addr));

    switch (params->authorization_type)
    {
    case ZB_ZDO_AUTHORIZATION_TYPE_LEGACY:
        TRACE_MSG(TRACE_APP3, "auth_type: 0x%hx - LEGACY DEVICE", (FMT__H, params->authorization_type));
        switch (params->authorization_status)
        {
        case ZB_ZDO_LEGACY_DEVICE_AUTHORIZATION_SUCCESS:
            TRACE_MSG(TRACE_APP3, "auth_status: 0x%hx - SUCCESS", (FMT__H, params->authorization_status));
            break;
        case ZB_ZDO_LEGACY_DEVICE_AUTHORIZATION_FAILED:
            TRACE_MSG(TRACE_APP3, "auth_status: 0x%hx - FAILED", (FMT__H, params->authorization_status));
            break;
        default:
            TRACE_MSG(TRACE_APP3, "auth_status: 0x%hx - INVALID VALUE", (FMT__H, params->authorization_status));
            break;
        }
        break;

    case ZB_ZDO_AUTHORIZATION_TYPE_R21_TCLK:
        TRACE_MSG(TRACE_APP3, "auth_type: 0x%hx - R21 TCLK", (FMT__H, params->authorization_type));
        switch (params->authorization_status)
        {
        case ZB_ZDO_TCLK_AUTHORIZATION_SUCCESS:
            TRACE_MSG(TRACE_APP3, "auth_status: 0x%hx - SUCCESS", (FMT__H, params->authorization_status));
            break;
        case ZB_ZDO_TCLK_AUTHORIZATION_TIMEOUT:
            TRACE_MSG(TRACE_APP3, "auth_status: 0x%hx - TIMEOUT", (FMT__H, params->authorization_status));
            break;
        case ZB_ZDO_TCLK_AUTHORIZATION_FAILED:
            TRACE_MSG(TRACE_APP3, "auth_status: 0x%hx - FAILED", (FMT__H, params->authorization_status));
            break;
        default:
            TRACE_MSG(TRACE_APP3, "auth_status: 0x%hx - INVALID VALUE", (FMT__H, params->authorization_status));
            break;
        }
        break;

    default:
        TRACE_MSG(TRACE_APP3, "auth_type: 0x%hx - INVALID VALUE", (FMT__H, params->authorization_type));
        break;
    }
}

/*! @} */
