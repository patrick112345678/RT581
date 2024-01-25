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
/* PURPOSE: ZC
*/

#define ZB_TEST_NAME TP_PRO_BV_17_ZC
#define ZB_TRACE_FILE_ID 40928

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "tp_pro_bv_17_common.h"
#include "../common/zb_cert_test_globals.h"

#ifndef ZB_CERTIFICATION_HACKS
#error "Define ZB_CERTIFICATION_HACKS"
#endif

static zb_ieee_addr_t g_ieee_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
static zb_ieee_addr_t g_ieee_addr_r1 = IEEE_ADDR_R1;
static zb_ieee_addr_t g_ieee_addr_r2 = IEEE_ADDR_R2;

MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("zdo_1_zc");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    zb_set_pan_id(0x1aaa);

    ZB_CERT_HACKS().aps_disable_addr_update_on_update_device = 1;
    ZB_CERT_HACKS().disable_addr_conflict_check_on_update_device = 1;

    /* let's always be coordinator */
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zc_role();

    /* set ieee addr */
    zb_set_long_address(g_ieee_addr);

    MAC_ADD_VISIBLE_LONG(g_ieee_addr_r1);
    MAC_ADD_VISIBLE_LONG(g_ieee_addr_r2);


    /* turn off security */
    /* zb_cert_test_set_security_level(0); */

    zb_set_max_children(2);

    zb_bdb_set_legacy_device_support(ZB_TRUE);
    zb_set_nvram_erase_at_start(ZB_TRUE);
    zb_set_nvram_erase_at_start(ZB_TRUE);

    if (zboss_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
    }
    else
    {
        zboss_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}


static zb_uint16_t addr_ass_cb(zb_ieee_addr_t ieee_addr)
{
    static zb_uint16_t res = (zb_uint16_t)0x0;
    (void)ieee_addr;

    TRACE_MSG(TRACE_APS3, ">>addr_assignmnet_cb", (FMT__0));
    res++;

    TRACE_MSG(TRACE_APS3, "<<addr_assignmnet_cb: res = 0x%x;", (FMT__H, res));
    return res;
}

#define PERMIT_JOINING_TIME_FOR_PRO_BV_17 250

static void send_permit_joining_req(zb_uint8_t param)
{
    zb_zdo_mgmt_permit_joining_req_param_t *request;

    TRACE_MSG(TRACE_ZDO2, "send_permit_joining_req %hd", (FMT__H, param));

    request = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_permit_joining_req_param_t);
    request->dest_addr = zb_cert_test_get_network_addr();
    request->permit_duration = PERMIT_JOINING_TIME_FOR_PRO_BV_17;
    zb_zdo_mgmt_permit_joining_req(param, NULL);
}

static void send_permit_joining_req_alarm(zb_uint8_t param)
{
    ZVUNUSED(param);
    zb_buf_get_out_delayed(send_permit_joining_req);
    ZB_SCHEDULE_ALARM(send_permit_joining_req_alarm, 0, ZB_TIME_ONE_SECOND * PERMIT_JOINING_TIME_FOR_PRO_BV_17);
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    TRACE_MSG(TRACE_ERROR, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    if (0 == status)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));
            zb_nwk_set_address_assignment_cb(addr_ass_cb);
            ZB_SCHEDULE_CALLBACK(send_permit_joining_req_alarm, 0);
            break;

        default:
            TRACE_MSG(TRACE_ERROR, "Unknown signal %hd", (FMT__H, sig));
        }
    }
    else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
    {
        TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Device START FAILED", (FMT__0));
    }

    zb_buf_free(param);
}

