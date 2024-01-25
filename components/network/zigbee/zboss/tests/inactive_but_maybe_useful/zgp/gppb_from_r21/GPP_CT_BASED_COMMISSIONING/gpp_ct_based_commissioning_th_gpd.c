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
/* PURPOSE: Simple ZGPD for send GPDF as described in 5.3.1.2 test specification.
*/

#define ZB_TEST_NAME GPP_CT_BASED_COMMISSIONING_TH_GPD
#define ZB_TRACE_FILE_ID 41399

#include "test_config.h"

#include "../include/zgp_test_templates.h"

#ifdef ZB_CERTIFICATION_HACKS

static zb_ieee_addr_t g_th_gpd_addr_6_1 = TEST_ZGPD_IEEE_ADDR_6_1;
static zb_ieee_addr_t g_th_gpd_addr_6_2 = TEST_ZGPD_IEEE_ADDR_6_2;
static zb_ieee_addr_t g_th_gpd_addr_7 = TEST_ZGPD_IEEE_ADDR_7;

/*! Program states according to test scenario */
enum test_states_e
{
    TEST_STATE_INITIAL,
    TEST_STATE_SEND_DATA_GPDF_5_1,
    TEST_STATE_SEND_DATA_GPDF_5_2,
    TEST_STATE_SEND_DATA_GPDF_6_1,
    TEST_STATE_SEND_DATA_GPDF_6_2,
    TEST_STATE_SEND_DATA_GPDF_7_1,
    TEST_STATE_SEND_DATA_GPDF_7_2,
    TEST_STATE_FINISHED
};

ZB_ZGPD_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 11000)

static void make_gpdf(zb_buf_t *buf, zb_uint8_t **ptr)
{
    ZVUNUSED(buf);
    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_SEND_DATA_GPDF_5_1:
    case TEST_STATE_SEND_DATA_GPDF_6_1:
    case TEST_STATE_SEND_DATA_GPDF_7_1:
    {
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        break;
    }
    case TEST_STATE_SEND_DATA_GPDF_5_2:
    case TEST_STATE_SEND_DATA_GPDF_6_2:
    {
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        ZB_ZGPD_SET_PAUSE(4);
        break;
    }
    case TEST_STATE_SEND_DATA_GPDF_7_2:
    {
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        break;
    }
    }
}

static void update_device()
{
    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_SEND_DATA_GPDF_5_2:
        ZB_ZGPD_SET_SRC_ID(TEST_ZGPD_SRC_ID_5_2);
        break;
    case TEST_STATE_SEND_DATA_GPDF_6_1:
        ZB_ZGPD_INIT_ZGPD_CTX(ZB_ZGP_APP_ID_0010, ZB_ZGPD_COMMISSIONING_UNIDIR, ZB_ZGP_ON_OFF_SWITCH_DEV_ID);
        ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_th_gpd_addr_6_1);
        ZB_IEEE_ADDR_COPY(&g_zgpd_ctx.id.addr.ieee_addr, &g_th_gpd_addr_6_1);
        g_zgpd_ctx.id.endpoint = TEST_ZGPD_ENDPOINT_6;
        break;
    case TEST_STATE_SEND_DATA_GPDF_6_2:
        ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_th_gpd_addr_6_2);
        ZB_IEEE_ADDR_COPY(&g_zgpd_ctx.id.addr.ieee_addr, &g_th_gpd_addr_6_2);
        g_zgpd_ctx.id.endpoint = TEST_ZGPD_ENDPOINT_6;
        break;
    case TEST_STATE_SEND_DATA_GPDF_7_1:
        ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_th_gpd_addr_7);
        ZB_IEEE_ADDR_COPY(&g_zgpd_ctx.id.addr.ieee_addr, &g_th_gpd_addr_7);
        g_zgpd_ctx.id.endpoint = TEST_ZGPD_ENDPOINT_7_1;
        break;
    case TEST_STATE_SEND_DATA_GPDF_7_2:
        g_zgpd_ctx.id.endpoint = TEST_ZGPD_ENDPOINT_7_2;
        break;
    };
}

static void perform_next_state(zb_uint8_t param)
{
    if (param)
    {
        zb_free_buf(ZB_BUF_FROM_REF(param));
        param = 0;
    }

    if (TEST_DEVICE_CTX.pause)
    {
        ZB_SCHEDULE_ALARM(perform_next_state, 0,
                          ZB_TIME_ONE_SECOND * TEST_DEVICE_CTX.pause);
        TEST_DEVICE_CTX.pause = 0;
        return;
    }
    TEST_DEVICE_CTX.test_state++;

    TRACE_MSG(TRACE_APP1, "perform next state: %hd", (FMT__H, TEST_DEVICE_CTX.test_state));

    update_device();

    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_FINISHED:
        TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
        break;
    default:
    {
        ZB_SCHEDULE_ALARM(test_send_command, 0, ZB_TIME_ONE_SECOND);
    }
    };
}

static void zgp_custom_startup()
{
#if ! (defined KEIL || defined ZB_PLATFORM_LINUX_ARM_2400)
#endif

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("th_gpd");


    ZB_ZGPD_INIT_ZGPD_CTX(ZB_ZGP_APP_ID_0000, ZB_ZGPD_COMMISSIONING_UNIDIR, ZB_ZGP_ON_OFF_SWITCH_DEV_ID);

    ZB_ZGPD_SET_SRC_ID(TEST_ZGPD_SRC_ID_5_1);
    ZB_ZGPD_SET_SECURITY_LEVEL(ZB_ZGP_SEC_LEVEL_NO_SECURITY);
    ZB_ZGPD_SET_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_NO_KEY);
    //ZB_ZGPD_USE_RANDOM_SEQ_NUM();

    ZGPD->channel = TEST_CHANNEL;
}

#endif /* ZB_CERTIFICATION_HACKS */

ZB_ZGPD_TH_DECLARE_STARTUP_PROCESS()
