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
/* PURPOSE: TP/APS/BV-15-I Group Management-Group Addition-Tx
*/

#define ZB_TEST_NAME TP_APS_BV_15_I_ZR2
#define ZB_TRACE_FILE_ID 40714
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "tp_aps_bv-15-i_common.h"
#include "../common/zb_cert_test_globals.h"


static const zb_ieee_addr_t g_ieee_addr_r2 = IEEE_ADDR_R2;

MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_zr2");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif


    zb_cert_test_set_common_channel_settings();

    zb_set_long_address(g_ieee_addr_r2);
    zb_cert_test_set_zr_role();
    zb_set_max_children(0);

    ZB_CERT_HACKS().allow_entry_for_unregistered_ep = 1;
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

static void add_group_request(zb_uint8_t param)
{
    zb_bufid_t req = zb_buf_get_out();
    zb_apsme_add_group_req_t *req_param = ZB_BUF_GET_PARAM(req, zb_apsme_add_group_req_t);
    ZB_BZERO(req_param, sizeof(*req_param));
    (void)param;

    req_param->group_address = GROUP_ADDR;
    req_param->endpoint = GROUP_EP;
    zb_apsme_add_group_request(req);
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_zdo_app_signal_hdr_t *sg_p = NULL;
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

    TRACE_MSG(TRACE_APP1, "zboss_signal_handler: status %hd signal %hd",
              (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
            TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
            ZB_SCHEDULE_ALARM(add_group_request, 0, TIME_ZR2_ADD_TO_GROUP);
            break;
        default:
            break;
        }
    }
    else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
    {
        TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d",
                  (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
    }

    zb_buf_free(param);
}
