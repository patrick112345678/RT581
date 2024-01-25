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
/* PURPOSE: TH tool
*/

#define ZB_TEST_NAME GPP_GPDF_BASIC_APP_000_TH_TOOL
#define ZB_TRACE_FILE_ID 41343

#include "test_config.h"

#include "../include/zgp_test_templates.h"

#ifdef ZB_CERTIFICATION_HACKS

static zb_ieee_addr_t g_th_tool_addr = TH_TOOL_IEEE_ADDR;
static zb_uint8_t g_key_nwk[] = { 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0, 0, 0, 0, 0, 0, 0, 0};

static zb_uint8_t g_key_oob[] = TEST_SEC_KEY;

#define ENDPOINT 10

#define ZGPD_SEQ_NUM_CAP 1
#define ZGPD_RX_ON_CAP 0
#define ZGPD_FIX_LOC 0
#define ZGPD_USE_SECURITY 1
#define ZGPD_USE_ASSIGNED_ALIAS 0

enum test_states_e
{
    TEST_STATE_INITIAL,
    TEST_STATE_READ_GPP_PROXY_TABLE,
    TEST_STATE_SEND_PAIRING_1,
    TEST_STATE_READ_GPP_PROXY_TABLE_1,
    TEST_STATE_READ_GPP_PROXY_TABLE_2,
    TEST_STATE_FINISHED
};

ZB_ZGPC_DECLARE_ZCL_ON_OFF_TOGGLE_TEST_TEMPLATE(TEST_DEVICE_CTX, ENDPOINT, 1000)

static void send_zcl(zb_uint8_t buf_ref, zb_callback_t cb)
{
    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_READ_GPP_PROXY_TABLE:
    case TEST_STATE_READ_GPP_PROXY_TABLE_1:
    case TEST_STATE_READ_GPP_PROXY_TABLE_2:
        zgp_cluster_read_attr(buf_ref, DUT_GPPB_ADDR, DUT_GPPB_ADDR_MODE,
                              ZB_ZCL_ATTR_GPP_PROXY_TABLE_ID,
                              ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
        ZB_ZGPC_SET_PAUSE(1);
        break;
    };
}

static void zgpc_send_pairing_cb(zb_uint8_t param)
{
    zb_free_buf(ZB_BUF_FROM_REF(param));
    ZB_SCHEDULE_ALARM(PERFORM_NEXT_STATE, 0, ZB_TIME_ONE_SECOND);
}

static void zgpc_send_pairing(zb_uint8_t param, zb_bool_t add_sink)
{
    zb_zgp_sink_tbl_ent_t ent;

    ZB_BZERO(&ent, sizeof(zb_zgp_sink_tbl_ent_t));
    ZB_MEMSET(ent.u.sink.sgrp, 0xff, sizeof(ent.u.sink.sgrp));
    ent.u.sink.device_id = ZB_ZGP_ON_OFF_SWITCH_DEV_ID;
    ent.u.sink.match_dev_tbl_idx = 0xff;

    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_SEND_PAIRING_1:
        ent.options = ZGP_TBL_SINK_FILL_OPTIONS(ZB_ZGP_APP_ID_0000,
                                                ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED,
                                                ZGPD_SEQ_NUM_CAP,
                                                ZGPD_RX_ON_CAP,
                                                ZGPD_FIX_LOC,
                                                ZGPD_USE_ASSIGNED_ALIAS,
                                                ZGPD_USE_SECURITY);
        ent.zgpd_id.src_id = TEST_ZGPD_SRC_ID;
        ent.sec_options = ZGP_TBL_FILL_SEC_OPTIONS(ZB_ZGP_SEC_LEVEL_FULL_NO_ENC, ZB_ZGP_SEC_KEY_TYPE_GROUP);
        ent.security_counter = 0x01;
        ZB_MEMCPY(ent.zgpd_key, g_key_oob, ZB_CCM_KEY_SIZE);
        break;
    };

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

static void perform_next_state(zb_uint8_t param)
{
    if (TEST_DEVICE_CTX.pause)
    {
        ZB_SCHEDULE_ALARM(perform_next_state, 0,
                          ZB_TIME_ONE_SECOND * TEST_DEVICE_CTX.pause);
        TEST_DEVICE_CTX.pause = 0;
        return;
    }

    TEST_DEVICE_CTX.test_state++;

    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_SEND_PAIRING_1:
        if (param == 0)
        {
            ZB_GET_OUT_BUF_DELAYED(perform_next_state);
            TEST_DEVICE_CTX.test_state--;
            return;
        }
        zgpc_send_pairing(param, ZB_TRUE);
        break;
    case TEST_STATE_FINISHED:
        TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
        break;
    default:
    {
        if (param)
        {
            zb_free_buf(ZB_BUF_FROM_REF(param));
        }
        ZB_SCHEDULE_ALARM(test_send_command, 0, ZB_TIME_ONE_SECOND);
    }
    }
}

static void zgpc_custom_startup()
{
    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("th_tool");

    zb_set_long_address(g_th_tool_addr);

    zb_set_network_router_role(1 << TEST_CHANNEL);

    zb_secur_setup_nwk_key(g_key_nwk, 0);

    /* Need to block GPDF recv directly */
#if defined ZB_ZGP_SKIP_GPDF_ON_NWK_LAYER
    zb_zgp_set_skip_gpfd(ZB_TRUE);
#endif
#ifdef ZB_ZGP_RUNTIME_WORK_MODE_WITH_PROXIES
    ZGP_CTX().enable_work_with_proxies = 1;
#endif

    //zb_zgps_set_communication_mode(ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED);

    zb_set_zgp_device_role(ZGP_DEVICE_COMBO_BASIC);
}

#endif /* ZB_CERTIFICATION_HACKS */

ZB_ZGPC_TH_DECLARE_STARTUP_PROCESS()
