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

#define ZB_TRACE_FILE_ID 44456

#include "test_config.h"

#include "../include/zgp_test_templates.h"

#ifdef ZB_CERTIFICATION_HACKS

enum test_btn_e
{
    TEST_BTN_LEFT,
    TEST_BTN_UP,
    TEST_BTN_DOWN,
    TEST_BTN_RIGHT
};

static zb_uint8_t cur_btn = TEST_BTN_LEFT;

static zb_uint8_t     g_zgpd_key[] = { 0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF };

/*! Program states according to test scenario */
enum test_states_e
{
    TEST_STATE_INITIAL,
    TEST_STATE_NOP,
    TEST_STATE_FINISHED
};

ZB_ZGPD_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 0)

ZB_ZGPD_DECLARE_COMMISSIONING_CALLBACK()

static void make_gpdf(zb_buf_t *buf, zb_uint8_t **ptr)
{
    ZVUNUSED(buf);
    switch (cur_btn)
    {
    case TEST_BTN_UP:
    {
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_ON);
        break;
    }
    case TEST_BTN_DOWN:
    {
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_OFF);
        break;
    }
    case TEST_BTN_RIGHT:
    {
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        break;
    }
    };
}

static void zgpd_left_btn_hndlr(zb_uint8_t param)
{
    ZVUNUSED(param);
    cur_btn = TEST_BTN_LEFT;
    PERFORM_NEXT_STATE(0);
}

static void zgpd_up_btn_hndlr(zb_uint8_t param)
{
    ZVUNUSED(param);
    cur_btn = TEST_BTN_UP;
    PERFORM_NEXT_STATE(0);
}

static void zgpd_down_btn_hndlr(zb_uint8_t param)
{
    ZVUNUSED(param);
    cur_btn = TEST_BTN_DOWN;
    PERFORM_NEXT_STATE(0);
}

static void zgpd_right_btn_hndlr(zb_uint8_t param)
{
    ZVUNUSED(param);
    cur_btn = TEST_BTN_RIGHT;
    PERFORM_NEXT_STATE(0);
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

    TRACE_MSG(TRACE_APP1, "perform next state: %hd", (FMT__H, TEST_DEVICE_CTX.test_state));

    switch (cur_btn)
    {
    case TEST_BTN_LEFT:
        zb_zgpd_start_commissioning(comm_cb);
        break;
    default:
    {
        test_send_command(0);
    }
    };
}

static void zgp_custom_startup()
{
#if ! (defined KEIL || defined ZB_PLATFORM_LINUX_ARM_2400)
#endif

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("th_gpd");


    ZB_ZGPD_INIT_ZGPD_CTX(ZB_ZGP_APP_ID_0000, ZB_ZGPD_COMMISSIONING_BIDIR, ZB_ZGP_ON_OFF_SWITCH_DEV_ID);

    ZB_ZGPD_SET_SRC_ID(TEST_ZGPD_SRC_ID);
    ZB_ZGPD_SET_SECURITY_LEVEL(ZB_ZGP_SEC_LEVEL_FULL_WITH_ENC);
    ZB_ZGPD_SET_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL);
    ZB_ZGPD_SET_SECURITY_KEY(g_zgpd_key);
    ZB_ZGPD_SET_OOB_KEY(g_zgpd_key);

    ZGPD->channel = TEST_CHANNEL;

    TEST_DEVICE_CTX.left_button_handler = zgpd_left_btn_hndlr;
    TEST_DEVICE_CTX.up_button_handler = zgpd_up_btn_hndlr;
    TEST_DEVICE_CTX.down_button_handler = zgpd_down_btn_hndlr;
    TEST_DEVICE_CTX.right_button_handler = zgpd_right_btn_hndlr;
}

#endif /* ZB_CERTIFICATION_HACKS */

ZB_ZGPD_TH_DECLARE_STARTUP_PROCESS()
