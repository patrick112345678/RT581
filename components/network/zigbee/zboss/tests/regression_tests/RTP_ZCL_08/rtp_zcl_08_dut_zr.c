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
/* PURPOSE: DUT ZR
*/

#define ZB_TEST_NAME RTP_ZCL_08_DUT_ZR

#define ZB_TRACE_FILE_ID 40257
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

#include "device_dut.h"
#include "rtp_zcl_08_common.h"
#include "../common/zb_reg_test_globals.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error ZB_USE_NVRAM is not compiled!
#endif


/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_uint8_t g_nwk_key[16] = ZB_REG_TEST_DEFAULT_NWK_KEY;
static zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(rtp_zcl_08_dut_zr_basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(rtp_zcl_08_dut_zr_identify_attr_list, &attr_identify_time);

/* On Off cluster attributes data */
static zb_bool_t attr_on_off = (zb_bool_t)ZB_ZCL_ON_OFF_IS_ON;

ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(rtp_zcl_08_dut_zr_on_off_attr_list,
                                  &attr_on_off);

/********************* Declare device **************************/
DECLARE_DUT_CLUSTER_LIST(rtp_zcl_08_dut_zr_device_clusters,
                         rtp_zcl_08_dut_zr_basic_attr_list,
                         rtp_zcl_08_dut_zr_identify_attr_list,
                         rtp_zcl_08_dut_zr_on_off_attr_list);

DECLARE_DUT_EP(rtp_zcl_08_dut_zr_device_ep,
               DUT_ENDPOINT,
               rtp_zcl_08_dut_zr_device_clusters);

DECLARE_DUT_CTX(rtp_zcl_08_dut_zr_device_ctx, rtp_zcl_08_dut_zr_device_ep);

/************************Main*************************************/
MAIN()
{
    ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
    ZB_SET_TRACE_LEVEL(4);
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_dut_zr");


    zb_set_long_address(g_ieee_addr_dut);

    zb_reg_test_set_common_channel_settings();
    zb_set_network_router_role((1l << TEST_CHANNEL));
    zb_set_nvram_erase_at_start(ZB_TRUE);


    zb_secur_setup_nwk_key(g_nwk_key, 0);

    ZB_AF_REGISTER_DEVICE_CTX(&rtp_zcl_08_dut_zr_device_ctx);

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

/********************ZDO Startup*****************************/
ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

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

    default:
        TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}

/*! @} */
