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
/* PURPOSE: TH ZR1
*/

#define ZB_TEST_NAME RTP_NWK_06_DUT_ZR1
#define ZB_TRACE_FILE_ID 64906

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_zcl.h"
#include "zb_bdb_internal.h"
#include "zb_zcl.h"

#include "rtp_nwk_06_common.h"
#include "../common/zb_reg_test_globals.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error ZB_USE_NVRAM is not compiled!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_dut_zr1 = IEEE_ADDR_DUT_ZR1;
static zb_ieee_addr_t g_ieee_addr_th_zed = IEEE_ADDR_TH_ZED;

/*******************Definitions for Test***************************/

static void start_concenstartor_mode(zb_uint8_t param);
static void stop_concenstartor_mode(zb_uint8_t param);

MAIN()
{
    ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
    ZB_SET_TRACE_LEVEL(4);
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("zdo_dut_zr1");

    zb_set_long_address(g_ieee_addr_dut_zr1);

    zb_reg_test_set_common_channel_settings();
    zb_set_network_router_role((1l << TEST_CHANNEL));

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


/***********************************Implementation**********************************/
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
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        }
        break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
    {
        zb_zdo_signal_device_annce_params_t *dev_annce_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_annce_params_t);

        ZB_SCHEDULE_CALLBACK(start_concenstartor_mode, ZB_BUF_INVALID);

        if (ZB_IEEE_ADDR_CMP(dev_annce_params->ieee_addr, g_ieee_addr_th_zed))
        {
            ZB_SCHEDULE_ALARM(stop_concenstartor_mode, 0, RTP_NWK_06_CONCENTRATOR_MODE_STOP_DELAY);
        }
    }
    break;

    case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APP1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
        break; /* ZB_BDB_SIGNAL_STEERING */

    default:
        TRACE_MSG(TRACE_APP1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}


static void start_concenstartor_mode(zb_uint8_t param)
{
    ZVUNUSED(param);
    zb_start_concentrator_mode(RTP_NWK_06_CONCENTRATOR_MODE_RADIUS, RTP_NWK_06_CONCENTRATOR_MODE_DISC_TIME);
}


static void stop_concenstartor_mode(zb_uint8_t param)
{
    ZVUNUSED(param);
    zb_stop_concentrator_mode();
}

/*! @} */
