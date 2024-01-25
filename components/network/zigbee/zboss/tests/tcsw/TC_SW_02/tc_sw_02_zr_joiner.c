/* ZBOSS Zigbee software protocol stack
 *
 * Copyright (c) 2012-2020 DSR Corporation, Denver CO, USA.
 * http://www.dsr-zboss.com
 * http://www.dsr-corporation.com
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

/**
 * PURPOSE: TH ZR
 */

#define ZB_TEST_NAME TC_5_1_TH_ZR
#define ZB_TRACE_FILE_ID 64902

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
#include "zb_mem_config_max.h"

#include "tc_sw_02_common.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_USE_NVRAM
#error ZB_USE_NVRAM is not compiled!
#endif

static zb_ieee_addr_t g_ieee_addr_th_zr = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
void call_simple_desc_req(zb_uint8_t param);

static void start_rejoin(zb_uint8_t param)
{
    zb_ext_pan_id_t ext_pan_id;
    /* rejoin to current pan */
    zb_get_extended_pan_id(ext_pan_id);
    zdo_initiate_rejoin(param,
                        ext_pan_id,
                        ZB_AIB().aps_channel_mask_list,
                        ZB_FALSE);
}

static void zc_is_dead(zb_uint8_t param)
{
    ZVUNUSED(param);
    /* rejoin */
    TRACE_MSG(TRACE_APP1, "LET'S REJOIN", (FMT__0));
    zb_buf_get_out_delayed(start_rejoin);
}

static void simple_desc_callback(zb_uint8_t param)
{
    ZB_SCHEDULE_ALARM_CANCEL(zc_is_dead, 0);
    ZB_SCHEDULE_ALARM(call_simple_desc_req, param, 5 * ZB_TIME_ONE_SECOND);
}


void call_simple_desc_req(zb_uint8_t param)
{
    zb_zdo_simple_desc_req_t *req;

    req = zb_buf_initial_alloc(param, sizeof(zb_zdo_simple_desc_req_t));

    req->nwk_addr = 0; //send to coordinator
    req->endpoint = 1;

    ZB_SCHEDULE_ALARM(zc_is_dead, 0, 3 * ZB_TIME_ONE_SECOND);
    zb_zdo_simple_desc_req(param, simple_desc_callback);
}

MAIN()
{
    ARGV_UNUSED;

    ZB_SET_TRAF_DUMP_ON();
    ZB_SET_TRACE_ON();
    ZB_INIT("zdo_th_zr_joiner");

    zb_set_long_address(g_ieee_addr_th_zr);

    zb_set_network_router_role(1l << TEST_CHANNEL);

    zb_set_max_children(0);

    zb_set_nvram_erase_at_start(ZB_TRUE);

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

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    switch (sig)
    {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));
        ZB_SCHEDULE_ALARM(call_simple_desc_req, param, 5 * ZB_TIME_ONE_SECOND);
        param = 0;
        break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APP1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
        break; /* ZB_BDB_SIGNAL_STEERING */

    default:
        TRACE_MSG(TRACE_APP1, "Unknown signal, sig %hd, status %d", (FMT__H_D, sig, status));
        break;
    }

    if (param)
    {
        zb_buf_free(param);
    }
}
