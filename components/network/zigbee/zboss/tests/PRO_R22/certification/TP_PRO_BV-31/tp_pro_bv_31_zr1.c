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
/* PURPOSE: Test for ZR application written using ZDO.
*/

#define ZB_TEST_NAME TP_PRO_BV_31_ZR1
#define ZB_TRACE_FILE_ID 40519

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "../common/zb_cert_test_globals.h"

//#define TEST_CHANNEL (1l << 24)


static zb_ieee_addr_t g_ieee_addr = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
static zb_ieee_addr_t g_ext_pan_id = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


MAIN()
{
    ARGV_UNUSED;

    /* FIXME: APS secure is off inside stack, lets use NWK secure */
#if 0
    aps_secure = 1;
#endif

    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("zdo_2_zr1");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    /* let's always be coordinator */
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zr_role();

    zb_set_long_address(g_ieee_addr);

    /* we need this for rejoin */
    zb_set_use_extended_pan_id(g_ext_pan_id);

    zb_set_pan_id(0x1aaa);

    zb_set_max_children(0);

    /* turn off security */
    /* zb_cert_test_set_security_level(0); */

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
        TRACE_MSG(TRACE_ERROR, "Device STARTE FAILED status %d", (FMT__D, status));
    }

    zb_buf_free(param);
}


#if 0
static zb_ret_t initiate_rejoin1(zb_bufid_t buf)
{
    zb_nlme_join_request_t *req = ZB_BUF_GET_PARAM(param, zb_nlme_join_request_t);

    ZB_MEMSET(req, 0, sizeof(*req));

    ZB_EXTPANID_COPY(req->extended_pan_id, ZB_AIB().aps_use_extended_pan_id);
#ifdef ZB_ROUTER_ROLE
    if (ZB_NIB_DEVICE_TYPE() == ZB_NWK_DEVICE_TYPE_NONE)
    {
        ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ROUTER;
        ZB_MAC_CAP_SET_ROUTER_CAPS(req->capability_information);
        TRACE_MSG(TRACE_APS1, "Rejoin to pan " TRACE_FORMAT_64 " as ZR", (FMT__A, TRACE_ARG_64(ZB_AIB().aps_use_extended_pan_id)));
    }
    else
#endif
    {
        TRACE_MSG(TRACE_APS1, "Rejoin to pan " TRACE_FORMAT_64 " as ZE", (FMT__A, TRACE_ARG_64(ZB_AIB().aps_use_extended_pan_id)));
        if (ZB_PIBCACHE_RX_ON_WHEN_IDLE())
        {
            ZB_MAC_CAP_SET_RX_ON_WHEN_IDLE(req->capability_information, 1);
        }
    }
    ZB_MAC_CAP_SET_ALLOCATE_ADDRESS(req->capability_information, 1);
    req->rejoin_network = ZB_NLME_REJOIN_METHOD_REJOIN;
    req->scan_channels = ZB_AIB().aps_channel_mask;
    req->scan_duration = ZB_DEFAULT_SCAN_DURATION;
    ZG->zdo.handle.rejoin = ZB_TRUE;
    ZG->nwk.handle.joined = 0;

    return zb_schedule_callback(zb_nlme_join_request, param);
}
#endif



