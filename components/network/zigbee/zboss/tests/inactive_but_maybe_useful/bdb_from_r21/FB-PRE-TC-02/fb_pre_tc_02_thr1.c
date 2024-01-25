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


#define ZB_TEST_NAME FB_PRE_TC_02_THR1
#define ZB_TRACE_FILE_ID 41253
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
#include "fb_pre_tc_02_common.h"


#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif


/*! \addtogroup ZB_TESTS */
/*! @{ */


enum test_steps_e
{
    NWK_BRCAST_WITH_EXT_RESP1,
    NWK_BRCAST_WITH_SINGLE_RESP,
    NWK_BRCAST_WITH_EXT_RESP2,
    NWK_UNICAST_WITH_EXT_RESP,
    NWK_UNICAST_REQ_TO_DUT_CHILD,
    NWK_BRCAST_WITH_EXT_RESP_TO_DUT_CHILD,
    NWK_BRCAST_WITH_INV_REQ_TYPE,
    NWK_UNICAST_WITH_UNKNOWN_IEEE,
    NWK_BRCAST_WITH_UNKNOWN_IEEE,
    NWK_UNICAST_WITHOUT_PAYLOAD,
    IEEE_UNICAST_WITH_EXT_RESP1,
    IEEE_UNICAST_WITH_SINGLE_RESP,
    IEEE_UNICAST_WITH_EXT_RESP2,
    IEEE_UNICAST_REQ_TO_DUT_CHILD,
    IEEE_BRCAST_WITH_INV_REQ_TYPE,
    IEEE_UNICAST_WITH_UNKNOWN_NWK,
    IEEE_UNICAST_WITHOUT_PAYLOAD,
    TEST_STEPS_COUNT
};


extern void zdo_send_req_by_short(zb_uint16_t command_id,
                                  zb_uint8_t param,
                                  zb_callback_t cb,
                                  zb_uint16_t addr,
                                  zb_uint8_t resp_counter);


static void device_annce_cb(zb_zdo_device_annce_t *da);
static void close_thr1_delayed(zb_uint8_t unused);
static void close_thr1(zb_uint8_t param);
static void start_test(zb_uint8_t unused);

static void send_nwk_addr_req(zb_uint8_t param);
static void send_ieee_addr_req(zb_uint8_t param);
static void send_ieee_addr_req_via_dut(zb_uint8_t param);
static void send_addr_req_without_payload(zb_uint8_t param);

static void switch_to_next_step(zb_uint8_t param);
static void test_step_actions(zb_uint8_t unused);


static zb_ieee_addr_t s_payload_ieee;
static zb_uint16_t    s_payload_nwk;
static zb_uint8_t     s_payload_req_type;
static zb_uint16_t    s_zdo_cmd_clid;
static int            s_step_idx;
static int            s_dut_is_ed;


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_thr1");


    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_thr1);
    ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
    ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;
    ZB_BDB().bdb_mode = 1;

    ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ROUTER;
    ZB_IEEE_ADDR_COPY(ZB_AIB().trust_center_address, g_unknown_ieee_addr);
    zb_secur_setup_nwk_key(g_nwk_key, 0);
    ZB_NIB().max_children = 1;

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


static void device_annce_cb(zb_zdo_device_annce_t *da)
{
    TRACE_MSG(TRACE_ZDO1, ">>dev_annce_cb, ieee = " TRACE_FORMAT_64 " addr = 0x%x",
              (FMT__A_H, TRACE_ARG_64(da->ieee_addr), da->nwk_addr));
    if (ZB_IEEE_ADDR_CMP(g_ieee_addr_the1, da->ieee_addr) == ZB_TRUE)
    {
        TRACE_MSG(TRACE_ZDO2, "dev_annce_cb: TH ZED has joined to network!", (FMT__0));
        ZB_SCHEDULE_ALARM(close_thr1_delayed, 0, THR1_WAITING_DURATION);
        ZB_SCHEDULE_ALARM(test_step_actions, 0, THR1_START_TEST_DELAY);
    }
    else if (ZB_IEEE_ADDR_CMP(g_ieee_addr_dut, da->ieee_addr) == ZB_TRUE)
    {
        s_dut_is_ed = 1;
        TRACE_MSG(TRACE_ZDO2, "dev_annce_cb: DUT ZED has joined to network!", (FMT__0));
    }
    TRACE_MSG(TRACE_ZDO1, "<<dev_annce_cb", (FMT__0));
}


static void close_thr1_delayed(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    ZB_GET_OUT_BUF_DELAYED(close_thr1);
}


static void close_thr1(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_nlme_permit_joining_request_t *req = ZB_GET_BUF_TAIL(buf, sizeof(zb_nlme_permit_joining_request_t));

    TRACE_MSG(TRACE_ZDO1, ">>close_thr1: buf_param = %d", (FMT__D, param));

    req->permit_duration = 0;
    zb_nlme_permit_joining_request(param);

    TRACE_MSG(TRACE_ZDO1, "<<close_thr1", (FMT__0));
}


static void start_test(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    TRACE_MSG(TRACE_ZDO1, "TEST STARTED", (FMT__0));

    if (s_dut_is_ed)
    {
        s_step_idx = NWK_BRCAST_WITH_SINGLE_RESP;
    }
    /* FIXME: There is no implementation of zb_zdo_get_neighbor_device_type_by_ieee in the stack code */
    if (zb_zdo_get_neighbor_device_type_by_ieee(g_ieee_addr_dut) != ZB_NWK_DEVICE_TYPE_ED)
    {
        ZB_GET_OUT_BUF_DELAYED(close_thr1);
    }
    ZB_SCHEDULE_CALLBACK(test_step_actions, 0);
}


static void send_nwk_addr_req(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_zdo_nwk_addr_req_param_t *req = ZB_GET_BUF_TAIL(buf, sizeof(zb_zdo_nwk_addr_req_param_t));

    TRACE_MSG(TRACE_ZDO1, ">>send_nwk_addr_req: buf_param = %d", (FMT__D, param));

    req->dst_addr = s_payload_nwk;
    ZB_IEEE_ADDR_COPY(req->ieee_addr, s_payload_ieee);
    req->request_type = s_payload_req_type;
    req->start_index = 0;
    zb_zdo_nwk_addr_req(param, NULL);

    switch_to_next_step(0);

    TRACE_MSG(TRACE_ZDO1, "<<send_nwk_addr_req", (FMT__0));
}


static void send_ieee_addr_req(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_zdo_ieee_addr_req_param_t *req;

    TRACE_MSG(TRACE_ZDO1, ">>send_ieee_addr_req: buf_param = %d", (FMT__D, param));

    req = ZB_GET_BUF_PARAM(buf, zb_zdo_ieee_addr_req_param_t);

    req->nwk_addr = s_payload_nwk;
    req->dst_addr = req->nwk_addr;
    req->request_type = s_payload_req_type;
    req->start_index = 0;
    ZB_HTOLE16_ONPLACE((zb_uint8_t *) &req->nwk_addr);
    zb_zdo_ieee_addr_req(param, NULL);

    switch_to_next_step(0);

    TRACE_MSG(TRACE_ZDO1, "<<send_ieee_addr_req", (FMT__0));
}


static void send_ieee_addr_req_via_dut(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_uint16_t dest_addr;
    zb_zdo_ieee_addr_req_param_t *req;

    TRACE_MSG(TRACE_ZDO1, ">>send_ieee_addr_req_via_dut: buf_param = %d", (FMT__D, param));

    req = ZB_GET_BUF_PARAM(buf, zb_zdo_ieee_addr_req_param_t);

    req->nwk_addr = s_payload_nwk;
    req->dst_addr = req->nwk_addr;
    req->request_type = s_payload_req_type;
    req->start_index = 0;
    ZB_HTOLE16_ONPLACE((zb_uint8_t *) &req->nwk_addr);
    dest_addr = zb_address_short_by_ieee(g_ieee_addr_dut);

    zdo_send_req_by_short(ZDO_IEEE_ADDR_REQ_CLID, param, NULL, dest_addr, ZB_ZDO_CB_UNICAST_COUNTER);

    switch_to_next_step(0);

    TRACE_MSG(TRACE_ZDO1, "<<send_ieee_addr_req_via_dut", (FMT__0));
}


static void send_addr_req_without_payload(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_uint8_t *ptr;

    TRACE_MSG(TRACE_ZDO2, ">>send_addr_req_without_payload: buf_param = %d", (FMT__D, param));

    ZB_BUF_INITIAL_ALLOC(buf, 0, ptr);
    /* Sure Unicast: we need 1 answer for addr req */
    zdo_send_req_by_short(s_zdo_cmd_clid, param, NULL, s_payload_nwk, ZB_ZDO_CB_UNICAST_COUNTER);

    switch_to_next_step(0);

    TRACE_MSG(TRACE_ZDO2, "<<send_addr_req_without_payload", (FMT__0));
}


static void switch_to_next_step(zb_uint8_t param)
{
    int use_long_delay = 0;
    int stop = 0;

    TRACE_MSG(TRACE_ZDO1, ">>switch_to_next_step", (FMT__0));
    ZVUNUSED(param);

    ++s_step_idx;
    if (s_dut_is_ed)
    {
        switch (s_step_idx)
        {
        case NWK_UNICAST_REQ_TO_DUT_CHILD:
            s_step_idx = NWK_BRCAST_WITH_INV_REQ_TYPE;
            break;

        case IEEE_UNICAST_WITH_EXT_RESP1:
            s_step_idx = IEEE_UNICAST_WITH_SINGLE_RESP;
            break;
        }
    }
    else
    {
        switch (s_step_idx)
        {
        case NWK_BRCAST_WITH_SINGLE_RESP:
            stop = 1;
            break;

        case IEEE_UNICAST_WITH_EXT_RESP1:
            use_long_delay = 1;
            break;

        case IEEE_UNICAST_WITH_SINGLE_RESP:
            stop = 1;
            break;
        }
    }

    if (!stop)
    {
        if (use_long_delay)
        {
            ZB_SCHEDULE_ALARM(test_step_actions, 0, THR1_WAITING_DURATION);
        }
        else
        {
            ZB_SCHEDULE_ALARM(test_step_actions, 1, THR1_SHORT_DELAY);
        }
    }

    TRACE_MSG(TRACE_ZDO1, "<<switch_to_next_step", (FMT__0));
}


static void test_step_actions(zb_uint8_t unused)
{
    int stop_test = 0;
    zb_callback_t next_cb = NULL;

    ZVUNUSED(unused);
    TRACE_MSG(TRACE_ZDO1, ">>test_step_actions: step = %d", (FMT__D, s_step_idx));

    switch (s_step_idx)
    {
    case NWK_BRCAST_WITH_EXT_RESP1:
    case NWK_BRCAST_WITH_EXT_RESP2:
    {
        ZB_IEEE_ADDR_COPY(s_payload_ieee, g_ieee_addr_dut);
        s_payload_nwk = 0xfffd;
        s_payload_req_type = 0x01;
        next_cb = send_nwk_addr_req;
    }
    break;

    case NWK_BRCAST_WITH_SINGLE_RESP:
    {
        ZB_IEEE_ADDR_COPY(s_payload_ieee, g_ieee_addr_dut);
        s_payload_nwk = 0xfffd;
        s_payload_req_type = 0x00;
        next_cb = send_nwk_addr_req;
    }
    break;

    case NWK_UNICAST_WITH_EXT_RESP:
    {
        ZB_IEEE_ADDR_COPY(s_payload_ieee, g_ieee_addr_dut);
        s_payload_nwk = zb_address_short_by_ieee(g_ieee_addr_dut);
        s_payload_req_type = 0x01;
        next_cb = send_nwk_addr_req;
    }
    break;

    case NWK_UNICAST_REQ_TO_DUT_CHILD:
    {
        ZB_IEEE_ADDR_COPY(s_payload_ieee, g_ieee_addr_the1);
        s_payload_nwk = zb_address_short_by_ieee(g_ieee_addr_dut);
        s_payload_req_type = 0x00;
        next_cb = send_nwk_addr_req;
    }
    break;

    case NWK_BRCAST_WITH_EXT_RESP_TO_DUT_CHILD:
    {
        ZB_IEEE_ADDR_COPY(s_payload_ieee, g_ieee_addr_the1);
        s_payload_nwk = 0xfffd;
        s_payload_req_type = 0x01;
        next_cb = send_nwk_addr_req;
    }
    break;

    case NWK_BRCAST_WITH_INV_REQ_TYPE:
    {
        ZB_IEEE_ADDR_COPY(s_payload_ieee, g_ieee_addr_dut);
        s_payload_nwk = 0xfffd;
        s_payload_req_type = 0x02;
        next_cb = send_nwk_addr_req;
    }
    break;

    case NWK_UNICAST_WITH_UNKNOWN_IEEE:
    {
        ZB_IEEE_ADDR_COPY(s_payload_ieee, g_unknown_ieee_addr);
        s_payload_nwk = zb_address_short_by_ieee(g_ieee_addr_dut);
        s_payload_req_type = 0x00;
        next_cb = send_nwk_addr_req;
    }
    break;

    case NWK_BRCAST_WITH_UNKNOWN_IEEE:
    {
        ZB_IEEE_ADDR_COPY(s_payload_ieee, g_unknown_ieee_addr);
        s_payload_nwk = 0xfffd;
        s_payload_req_type = 0x00;
        next_cb = send_nwk_addr_req;
    }
    break;

    case NWK_UNICAST_WITHOUT_PAYLOAD:
    {
        s_payload_nwk = zb_address_short_by_ieee(g_ieee_addr_dut);
        next_cb = send_addr_req_without_payload;
        s_zdo_cmd_clid = ZDO_NWK_ADDR_REQ_CLID;
    }
    break;


    case IEEE_UNICAST_WITH_EXT_RESP1:
    case IEEE_UNICAST_WITH_EXT_RESP2:
    {
        s_payload_nwk = zb_address_short_by_ieee(g_ieee_addr_dut);
        s_payload_req_type = 0x01;
        next_cb = send_ieee_addr_req;
    }
    break;

    case IEEE_UNICAST_WITH_SINGLE_RESP:
    {
        s_payload_nwk = zb_address_short_by_ieee(g_ieee_addr_dut);
        s_payload_req_type = 0x00;
        next_cb = send_ieee_addr_req;
    }
    break;

    case IEEE_UNICAST_REQ_TO_DUT_CHILD:
    {
        s_payload_nwk = zb_address_short_by_ieee(g_ieee_addr_the1);
        s_payload_req_type = 0x00;
        next_cb = send_ieee_addr_req_via_dut;
    }
    break;

    case IEEE_BRCAST_WITH_INV_REQ_TYPE:
    {
        s_payload_nwk = zb_address_short_by_ieee(g_ieee_addr_dut);
        s_payload_req_type = 0x02;
        next_cb = send_ieee_addr_req;
    }
    break;

    case IEEE_UNICAST_WITH_UNKNOWN_NWK:
    {
        s_payload_nwk = zb_random();
        s_payload_req_type = 0x00;
        next_cb = send_ieee_addr_req_via_dut;
    }
    break;

    case IEEE_UNICAST_WITHOUT_PAYLOAD:
    {
        s_payload_nwk = zb_address_short_by_ieee(g_ieee_addr_dut);
        next_cb = send_addr_req_without_payload;
        s_zdo_cmd_clid = ZDO_IEEE_ADDR_REQ_CLID;
    }
    break;

    default:
        stop_test = 1;
        TRACE_MSG(TRACE_ZDO1, "Unknown state, stop test", (FMT__0));
    }

    if (!stop_test)
    {
        ZB_GET_OUT_BUF_DELAYED(next_cb);
    }

    TRACE_MSG(TRACE_ZDO1, ">>test_step_actions", (FMT__0));
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
            zb_zdo_register_device_annce_cb(device_annce_cb);
            ZB_SCHEDULE_ALARM(start_test, 0, THR1_START_TEST_DELAY);

            if (ZB_IEEE_ADDR_CMP(ZB_AIB().trust_center_address, g_unknown_ieee_addr))
            {
                bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
            }
            else
            {
                ZB_GET_OUT_BUF_DELAYED(close_thr1);
            }
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
