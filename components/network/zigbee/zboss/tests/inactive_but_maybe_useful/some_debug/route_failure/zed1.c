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
/* PURPOSE: ZED
*/

#define ZB_TRACE_FILE_ID 40089
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "test_common.h"


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("zdo_4_zed1");

    ZB_PIBCACHE_PAN_ID() = 0x1AAA;

    {
        zb_channel_list_t channel_list;
        zb_channel_list_init(channel_list);
        zb_channel_list_add(channel_list, TEST_PAGE, (1L << TEST_CHANNEL));
        zb_channel_page_list_copy(ZB_AIB().aps_channel_mask_list, channel_list);
    }

    ZB_NIB().device_type = ZB_NWK_DEVICE_TYPE_ED;

    /* set ieee addr */
    zb_set_long_address(g_ieee_addr_ed1);

    zb_set_rx_on_when_idle(ZB_TRUE);

    MAC_ADD_INVISIBLE_SHORT(0x0000);
    MAC_ADD_VISIBLE_LONG(g_ieee_addr_r2);

    zb_set_nvram_erase_at_start(ZB_TRUE);

    if (zboss_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
    }
    else
    {
        zboss_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}


static void send_msg(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_buffer_test_req_param_t *req_param;

    req_param = ZB_GET_BUF_PARAM(buf, zb_buffer_test_req_param_t);
    BUFFER_TEST_REQ_SET_DEFAULT(req_param);

    req_param->len      = 0x10;
    req_param->dst_addr = 0x2222;

    zb_tp_buffer_test_request(ZB_REF_FROM_BUF(buf), NULL);
}

static void trigger_send_msg(zb_uint8_t param);


static void send_msg_loop(zb_uint8_t param)
{
    send_msg(param);

    ZB_SCHEDULE_ALARM(trigger_send_msg, 0, 10 * ZB_TIME_ONE_SECOND);
}

static void trigger_send_msg(zb_uint8_t param)
{
    static zb_uint32_t num_packets = 0;

    /* if (num_packets > 5) */
    /* { */
    /*   ZB_CERT_HACKS().disable_discovery_route = 1; */
    /* } */

    num_packets++;

    ZB_GET_OUT_BUF_DELAYED(send_msg_loop);
}

static void stop_send_msg(zb_uint8_t param)
{
    ZB_SCHEDULE_ALARM_CANCEL(trigger_send_msg, ZB_ALARM_ALL_CB);
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

            ZB_SCHEDULE_ALARM(trigger_send_msg, 0, 60 * ZB_TIME_ONE_SECOND);
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

    zb_free_buf(ZB_BUF_FROM_REF(param));
}

