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
/* PURPOSE: THe1
*/

#define ZB_TEST_NAME TP_BDB_FB_PRE_TC_01A_ZED1
#define ZB_TRACE_FILE_ID 40634

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_bdb_internal.h"

#include "metering_controller.h"
#include "../common/zb_cert_test_globals.h"
#include "tp_bdb_fb_pre_tc_01a_common.h"

/******************* Declare attributes ************************/

static zb_ieee_addr_t g_ieee_addr_the1 = IEEE_ADDR_THE1;

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(fb_pre_tc_01a_zed_basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(fb_pre_tc_01a_zed_identify_attr_list, &attr_identify_time);

DECLARE_METERING_CONTROLLER_CLUSTER_LIST(fb_pre_tc_01a_zed_metering_controller_clusters,
        fb_pre_tc_01a_zed_basic_attr_list,
        fb_pre_tc_01a_zed_identify_attr_list);

DECLARE_METERING_CONTROLLER_EP(fb_pre_tc_01a_zed_metering_controller_ep,
                               ZED_ENDPOINT,
                               fb_pre_tc_01a_zed_metering_controller_clusters);

DECLARE_METERING_CONTROLLER_CTX(fb_pre_tc_01a_zed_metering_controller_ctx,
                                fb_pre_tc_01a_zed_metering_controller_ep);

MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */
    {

        ZB_INIT("zdo_4_zed");
#if UART_CONTROL
        test_control_init();
        zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    }

    /* set ieee addr */
    zb_set_long_address(g_ieee_addr_the1);

    /* become an ED */
    zb_set_network_ed_role((1l << TEST_CHANNEL));
    zb_set_rx_on_when_idle(ZB_FALSE);

    zb_zdo_set_aps_unsecure_join(ZB_TRUE);

    ZB_AF_REGISTER_DEVICE_CTX(&fb_pre_tc_01a_zed_metering_controller_ctx);
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
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    switch (sig)
    {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        TRACE_MSG(TRACE_APS1, "Device started, status %d", (FMT__D, status));
        break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    default:
        TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}
