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
/* PURPOSE: TH GPD
*/

#define ZB_TEST_NAME GPP_GPD_MAC_SEQ_NUM_RND_TH_GPD
#define ZB_TRACE_FILE_ID 41503
#include "zb_common.h"
#include "zgpd/zb_zgpd.h"
#include "test_config.h"
#include "../include/zgp_test_templates.h"

#if defined ZB_CERTIFICATION_HACKS

static zb_uint32_t  g_zgpd_srcId = TEST_ZGPD_SRC_ID;

//static zb_uint8_t g_bad_key[16] = { 0xba, 0xd0, 0xba, 0xd0, 0xba, 0xd0, 0xba, 0xd0, 0xba, 0xd0, 0xba, 0xd0, 0xba, 0xd0, 0xba, 0xd0};

static zb_uint8_t g_oob_key[16] = TEST_OOB_KEY;

static zb_ieee_addr_t g_zgpd_addr = TH_GPD_IEEE_ADDR;

/*! Program states according to test scenario */
enum test_states_e
{
    TEST_STATE_INITIATE,
    TEST_STATE_COMMISSIONING,
    TEST_STATE_SEND_SET_MAC_DSN_C3_1,
    TEST_STATE_SEND_CMD_TOGGLE1,
    TEST_STATE_SEND_SET_MAC_DSN_C3_2,
    TEST_STATE_SEND_CMD_TOGGLE2,
    TEST_STATE_SEND_SET_MAC_DSN_C2,
    TEST_STATE_SEND_CMD_TOGGLE3,
    TEST_STATE_SEND_SET_MAC_DSN_C9,
    TEST_STATE_SEND_CMD_TOGGLE4,
    TEST_STATE_FINISHED
};

ZB_ZGPD_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 1000)

ZB_ZGPD_DECLARE_COMMISSIONING_CALLBACK()

static void next_state(zb_uint8_t param)
{
    TEST_DEVICE_CTX.test_state++;
    test_send_command(param);
}

static void set_dsn_and_call(zb_uint8_t param)
{
    zgpd_set_dsn_and_call(param, next_state);
}

static void start_comm(zb_uint8_t param)
{
    ZVUNUSED(param);
    zb_zgpd_start_commissioning(comm_cb);
}

static void set_dsn_and_call_comm(zb_uint8_t param)
{
    zgpd_set_dsn_and_call(param, start_comm);
}

static void perform_next_state(zb_uint8_t param)
{
    ZVUNUSED(param);
    TEST_DEVICE_CTX.test_state++;

    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_COMMISSIONING:
        ZB_GET_OUT_BUF_DELAYED(set_dsn_and_call_comm);
        ZB_ZGPD_SET_PAUSE(2);
        ZB_ZGP_SET_PASSED_STATE_SEQUENCE();
        break;
    case TEST_STATE_SEND_SET_MAC_DSN_C3_1:
    case TEST_STATE_SEND_SET_MAC_DSN_C3_2:
        ZGPD->mac_dsn = 0xc3;
        ZB_GET_OUT_BUF_DELAYED(set_dsn_and_call);
        ZB_ZGPD_SET_PAUSE(3);
        break;
    case TEST_STATE_SEND_SET_MAC_DSN_C2:
        ZGPD->mac_dsn = 0xc2;
        ZB_GET_OUT_BUF_DELAYED(set_dsn_and_call);
        ZB_ZGPD_SET_PAUSE(3);
        break;
    case TEST_STATE_SEND_SET_MAC_DSN_C9:
        ZGPD->mac_dsn = 0xc9;
        ZB_GET_OUT_BUF_DELAYED(set_dsn_and_call);
        ZB_ZGPD_SET_PAUSE(3);
        break;
    case TEST_STATE_FINISHED:
        TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
        break;
    default:
        ZB_SCHEDULE_ALARM(test_send_command, 0, ZB_TIME_ONE_SECOND);
    };
}
static void make_gpdf(zb_buf_t *buf, zb_uint8_t **ptr)
{
    ZVUNUSED(buf);
    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_SEND_CMD_TOGGLE1:
    case TEST_STATE_SEND_CMD_TOGGLE2:
    case TEST_STATE_SEND_CMD_TOGGLE3:
    case TEST_STATE_SEND_CMD_TOGGLE4:
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        break;
    };
}

static void zgp_custom_startup()
{
#if ! (defined KEIL || defined ZB_PLATFORM_LINUX_ARM_2400)
#endif

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("th_gpd");


    ZB_ZGPD_INIT_ZGPD_CTX(ZB_ZGP_APP_ID_0000, ZB_ZGPD_COMMISSIONING_BIDIR, ZB_ZGP_ON_OFF_SWITCH_DEV_ID);

    ZB_ZGPD_SET_SRC_ID(g_zgpd_srcId);
    ZB_IEEE_ADDR_COPY(&g_zgpd_ctx.id.addr.ieee_addr, &g_zgpd_addr);

    ZB_ZGPD_SET_SECURITY_LEVEL(ZB_ZGP_SEC_LEVEL_NO_SECURITY);
    ZB_ZGPD_SET_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_NO_KEY);
    ZB_ZGPD_SET_OOB_KEY(g_oob_key);

    ZB_ZGPD_USE_RANDOM_SEQ_NUM();

    ZGPD->channel = TEST_CHANNEL;
}

#endif /* ZB_CERTIFICATION_HACKS */

ZB_ZGPD_TH_DECLARE_STARTUP_PROCESS()
