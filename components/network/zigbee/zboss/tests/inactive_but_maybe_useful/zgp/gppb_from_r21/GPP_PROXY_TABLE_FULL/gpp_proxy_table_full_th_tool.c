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
/* PURPOSE: TH Tool (and ZC) for test GPP_PROXY_TABLE_FULL
*/

#define ZB_TEST_NAME GPP_PROXY_TABLE_FULL_TH_TOOL
#define ZB_TRACE_FILE_ID 41442
//#undef ZB_MULTI_TEST
#include "zb_common.h"
#include "zboss_api.h"

#include "test_config.h"

#include "../include/zgp_test_templates.h"

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#ifdef ZB_CERTIFICATION_HACKS


static void dev_annce_cb(zb_zdo_device_annce_t *da);


static zb_ieee_addr_t g_zc_addr = TH_TOOL_IEEE_ADDR;
static zb_ieee_addr_t g_zr_addr = DUT_GPPB_IEEE_ADDR;
static zb_uint8_t g_key_nwk[] = NWK_KEY;
static zb_uint32_t g_gpd_dev_number;
static zb_uint32_t g_dut_short_address;

#define ENDPOINT 10

/*! Program states according to test scenario */
enum test_states_e
{
    TEST_STATE_INITIAL,
    TEST_STATE_FILL_TABLE,
    TEST_STATE_READ_GPP_PROXY_TABLE0,
    TEST_STATE_SEND_PAIRING_AS_UNICAST,
    TEST_STATE_READ_GPP_PROXY_TABLE1,
    TEST_STATE_SEND_PAIRING_AS_BROADCAST,
    TEST_STATE_READ_GPP_PROXY_TABLE2,
    TEST_STATE_FINISHED
};

ZB_ZGPC_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 2000);



static void send_zcl(zb_uint8_t buf_ref, zb_callback_t cb)
{
    zb_zgpd_id_t id;

    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_READ_GPP_PROXY_TABLE0:
    case TEST_STATE_READ_GPP_PROXY_TABLE1:
    case TEST_STATE_READ_GPP_PROXY_TABLE2:
        id.addr.src_id = TH_TOOL_GPD_ID_ADDR_POOL_START_ADDR + g_gpd_dev_number;
        zgp_cluster_send_gp_proxy_table_request(buf_ref, g_dut_short_address, ZB_ADDR_16BIT_DEV_OR_BROADCAST,
                                                /* request entries by index */
                                                ZB_ZGP_GP_PROXY_TBL_REQ_FILL_OPT(ZB_ZGP_APP_ID_0000, 0),
                                                &id, 0, cb);
        TEST_DEVICE_CTX.test_state++;
        ZB_ZGPC_SET_PAUSE(1);
        break;
    };
}


static void zgpc_send_pairing_cb(zb_uint8_t param)
{
    zb_free_buf(ZB_BUF_FROM_REF(param));
    ZB_SCHEDULE_ALARM(PERFORM_NEXT_STATE, 0, 2 * ZB_TIME_ONE_SECOND);
    if (g_gpd_dev_number >= ZB_ZGP_PROXY_TBL_SIZE - 1)
    {
        TEST_DEVICE_CTX.test_state++;
    }
    else
    {
        ++g_gpd_dev_number;
    }
}


static void zgpc_send_pairing(zb_uint8_t param, zb_bool_t add_sink)
{
    zb_zgp_sink_tbl_ent_t ent;

    ZB_BZERO(&ent, sizeof(zb_zgp_sink_tbl_ent_t));

    ent.options = ZGP_TBL_SINK_FILL_OPTIONS(
                      ZB_ZGP_APP_ID_0000,
                      ZGP_COMMUNICATION_MODE_LIGHTWEIGHT_UNICAST,
                      ZGPD_SEQ_NUM_CAP,
                      ZGPD_RX_ON_CAP,
                      ZGPD_FIX_LOC,
                      ZGPD_DO_NOT_USE_ASSIGNED_ALIAS,
                      ZGPD_USE_SECURITY);

    ZB_MEMSET(ent.u.sink.sgrp, 0xff, sizeof(ent.u.sink.sgrp));
    ent.zgpd_id.src_id = TH_TOOL_GPD_ID_ADDR_POOL_START_ADDR + g_gpd_dev_number;
    ent.security_counter = TH_TOOL_START_MAC_DSN_VALUE - 1 + g_gpd_dev_number;
    ent.u.sink.device_id = ZB_ZGP_ON_OFF_SWITCH_DEV_ID;
    ent.u.sink.match_dev_tbl_idx = 0xff;
    ent.groupcast_radius = TH_TOOL_PAIRING_RADIUS;

    if (add_sink)
    {
        zb_zgp_gp_pairing_send_req_t *req;

        ZB_ZGP_GP_PAIRING_SEND_REQ_CREATE(ZB_BUF_FROM_REF(param), req, &ent, zgpc_send_pairing_cb);
        ZB_ZGP_GP_PAIRING_MAKE_SEND_OPTIONS(req, 1, 0, 1);
        zgp_cluster_send_gp_pairing(param);
    }
    else
    {
        zb_zgp_gp_pairing_send_req_t *req;

        ZB_ZGP_GP_PAIRING_SEND_REQ_CREATE(ZB_BUF_FROM_REF(param), req, &ent, zgpc_send_pairing_cb);
        ZB_ZGP_GP_PAIRING_MAKE_SEND_OPTIONS(req, 0, 1, 1);
        zgp_cluster_send_gp_pairing(param);
    }
}


static void open_network(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_nlme_permit_joining_request_t *req;

    TRACE_MSG(TRACE_APP1, ">>open_network", (FMT__0));

    req = ZB_GET_BUF_TAIL(buf, sizeof(zb_nlme_permit_joining_request_t));
    req->permit_duration = 30;

    TRACE_MSG(TRACE_APP1, "Toggling permit join on ZC, buf = %d, join duration = %d",
              (FMT__D_D, param, req->permit_duration));

    zb_nlme_permit_joining_request(param);

    TRACE_MSG(TRACE_APP1, "<<open_network", (FMT__0));
}



#define SEND_PAIRING_REQ(param)                 \
  if (param)                                    \
  {                                             \
    zgpc_send_pairing(param, ZB_TRUE);          \
  }                                             \
  else                                          \
  {                                             \
    ZB_GET_OUT_BUF_DELAYED(perform_next_state); \
  }

static void perform_next_state(zb_uint8_t param)
{
    if (TEST_DEVICE_CTX.pause)
    {
        ZB_SCHEDULE_ALARM(perform_next_state, 0, ZB_TIME_ONE_SECOND * TEST_DEVICE_CTX.pause);
        TEST_DEVICE_CTX.pause = 0;
        return;
    }

    TRACE_MSG(TRACE_APP1, ">>perform_next_state %d", (FMT__D, TEST_DEVICE_CTX.test_state));

    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_INITIAL:
    {
        if (param)
        {
            ZB_SCHEDULE_CALLBACK(open_network, param);
        }
        else
        {
            ZB_GET_OUT_BUF_DELAYED(open_network);
        }
        zb_register_zboss_callback(ZB_DEVICE_ANNCE_CB, SET_ZBOSS_CB(dev_annce_cb));
        break;
    }
    case TEST_STATE_FILL_TABLE:
    {
        SEND_PAIRING_REQ(param);
        break;
    }
    case TEST_STATE_SEND_PAIRING_AS_UNICAST:
    {
        if (param)
        {
            ++g_gpd_dev_number;
            ZB_CERT_HACKS().gp_sink_pairing_dest = g_dut_short_address;
        }
        SEND_PAIRING_REQ(param);
        break;
    }
    case TEST_STATE_SEND_PAIRING_AS_BROADCAST:
    {
        if (param)
        {
            ZB_CERT_HACKS().gp_sink_pairing_dest = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
        }

        SEND_PAIRING_REQ(param);
        break;
    }
    case TEST_STATE_FINISHED:
    {
        TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
        break;
    }
    default:
    {
        if (param)
        {
            zb_free_buf(ZB_BUF_FROM_REF(param));
        }
        ZB_SCHEDULE_ALARM(test_send_command, 0, ZB_TIME_ONE_SECOND);
    }
    }

    TRACE_MSG(TRACE_APP1, "<<perform_next_state", (FMT__0));
}


static void dev_annce_cb(zb_zdo_device_annce_t *da)
{
    TRACE_MSG(TRACE_APP1, ">>dev_annce_cb, ieee = " TRACE_FORMAT_64 "addr = %h",
              (FMT__A_H, TRACE_ARG_64(da->ieee_addr), da->nwk_addr));

    if (ZB_IEEE_ADDR_CMP(g_zr_addr, da->ieee_addr) == ZB_TRUE)
    {
        g_dut_short_address = da->nwk_addr;
        TEST_DEVICE_CTX.test_state++;
        ZB_ZGPC_SET_PAUSE(10);
        ZB_SCHEDULE_CALLBACK(perform_next_state, 0);
        TRACE_MSG(TRACE_APP1, "START THIS TEST!!!", (FMT__0));
    }

    TRACE_MSG(TRACE_APP1, "<<dev_annce_cb", (FMT__0));
}


static void zgpc_custom_startup()
{
    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("th_tool");

    zb_set_long_address(g_zc_addr);
    zb_set_network_coordinator_role(1l << TEST_CHANNEL);
    zb_set_nvram_erase_at_start(ZB_TRUE);
    ZB_NIB_SET_USE_MULTICAST(ZB_FALSE);
    /* Optional step: Setup predefined nwk key - to easily decrypt ZB sniffer logs which does not
     * contain keys exchange. By default nwk key is randomly generated. */
    zb_secur_setup_nwk_key((zb_uint8_t *) g_key_nwk, 0);
    ZB_PIBCACHE_PAN_ID() = 0x1aaa;

#ifdef ZB_ZGP_RUNTIME_WORK_MODE_WITH_PROXIES
    ZGP_CTX().enable_work_with_proxies = 1;
#endif
#ifdef ZB_CERTIFICATION_HACKS
    ZB_CERT_HACKS().ccm_check_cb = NULL;
#endif

    ZGP_CTX().device_role = ZGP_DEVICE_COMBO_BASIC;
    ZB_ZGP_SET_MATCH_INFO(&g_zgps_match_info);

    ZGP_GPS_SECURITY_LEVEL = ZB_ZGP_FILL_GPS_SECURITY_LEVEL(
                                 ZB_ZGP_SEC_LEVEL_NO_SECURITY,
                                 ZB_ZGP_DEFAULT_SEC_LEVEL_PROTECTION_WITH_GP_LINK_KEY,
                                 ZB_ZGP_DEFAULT_SEC_LEVEL_INVOLVE_TC);
    ZGP_GP_SHARED_SECURITY_KEY_TYPE = ZB_ZGP_SEC_KEY_TYPE_NWK;

    ZGP_GPS_COMMISSIONING_EXIT_MODE = ZGP_COMMISSIONING_EXIT_MODE_ON_PAIRING_SUCCESS;
    ZGP_GPS_COMMUNICATION_MODE = ZGP_COMMUNICATION_MODE_LIGHTWEIGHT_UNICAST;
}

#endif /* ZB_CERTIFICATION_HACKS */

ZB_ZGPC_TH_DECLARE_STARTUP_PROCESS()
