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
/* PURPOSE: DUT ZED (initiator)
*/


#define ZB_TEST_NAME FB_INI_TC_07B_DUT_EDI
#define ZB_TRACE_FILE_ID 41072
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
#include "shade_controller.h"
#include "fb_ini_tc_07b_common.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */


/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &attr_identify_time);


/********************* Declare device **************************/
DECLARE_SHADE_CONTROLLER_CLUSTER_LIST(shade_controller_clusters,
                                      basic_attr_list,
                                      identify_attr_list);

DECLARE_SHADE_CONTROLLER_EP(shade_controller_ep,
                            DUT_ENDPOINT,
                            shade_controller_clusters);

DECLARE_SHADE_CONTROLLER_CTX(shade_controller_ctx, shade_controller_ep);


static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster);
static void trigger_fb_ini(zb_uint8_t unused);


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_dut");


    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_dut);

    ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
    ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;
    ZB_BDB().bdb_mode = 1;
    ZB_BDB().bdb_commissioning_mode = ZB_BDB_NETWORK_STEERING;
    ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ED;

    ZB_AF_REGISTER_DEVICE_CTX(&shade_controller_ctx);

    if (zdo_dev_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
    }
    else
    {
        zdo_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}


static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster)
{
    TRACE_MSG(TRACE_ZCL1, "finding_binding_cb status %d addr " TRACE_FORMAT_64 " ep %hd cluster %d",
              (FMT__D_A_H_D, status, TRACE_ARG_64(addr), ep, cluster));
    return ZB_TRUE;
}


static void trigger_fb_ini(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    zb_bdb_finding_binding_initiator(DUT_ENDPOINT, finding_binding_cb);
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
            TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
            break;

        case ZB_BDB_SIGNAL_STEERING:
            TRACE_MSG(TRACE_APS1, "Steering on network completed", (FMT__0));
            ZB_SCHEDULE_ALARM(trigger_fb_ini, 0, TEST_TRIGGER_FB_INITIATOR_DELAY);
            break;

        case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:
            TRACE_MSG(TRACE_APS1, "Finding&binding done", (FMT__0));
            break;

        default:
            TRACE_MSG(TRACE_APS1, "Unknown signal", (FMT__0));
        }
    }
    else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
    {
        TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
    }
    zb_free_buf(ZB_BUF_FROM_REF(param));
}


/*! @} */
