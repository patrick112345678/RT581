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

#define ZB_TEST_NAME GPS_GPDF_SECURITY_11_APP_000_TH_GPD
#define ZB_TRACE_FILE_ID 41405
#include "zb_common.h"
#include "zgpd/zb_zgpd.h"
#include "test_config.h"
#include "../include/zgp_test_templates.h"

#ifdef ZB_CERTIFICATION_HACKS

static zb_uint32_t  g_zgpd_srcId = TEST_ZGPD_SRC_ID;

static zb_uint8_t g_bad_key[16] = { 0xba, 0xd0, 0xba, 0xd0, 0xba, 0xd0, 0xba, 0xd0, 0xba, 0xd0, 0xba, 0xd0, 0xba, 0xd0, 0xba, 0xd0};

static zb_uint8_t g_oob_key[16] = TEST_OOB_KEY;

static zb_uint32_t g_fc_start;

/*! Program states according to test scenario */
enum test_states_e
{
    TEST_STATE_INITIATE,
    TEST_STATE_COMMISSIONING,
    TEST_STATE_SEND_CMD_TOGGLE1,
    TEST_STATE_SEND_CMD_TOGGLE2,
    TEST_STATE_SEND_CMD_TOGGLE3,
    TEST_STATE_SEND_CMD_TOGGLE4,
    TEST_STATE_SEND_CMD_TOGGLE5,
    TEST_STATE_SEND_CMD_TOGGLE6A,
    TEST_STATE_SEND_CMD_TOGGLE6B,
    TEST_STATE_SEND_CMD_TOGGLE7A,
    TEST_STATE_SEND_CMD_TOGGLE7B,
    TEST_STATE_SEND_CMD_TOGGLE7C,
    TEST_STATE_SEND_CMD_TOGGLE7D,
    TEST_STATE_SEND_CMD_TOGGLE7E,
    TEST_STATE_SEND_CMD_TOGGLE7F,
    TEST_STATE_SEND_CMD_TOGGLE7G,
    TEST_STATE_SEND_CMD_TOGGLE7H,
    TEST_STATE_SEND_CMD_TOGGLE7I,
    TEST_STATE_SEND_CMD_TOGGLE7J,
    TEST_STATE_SEND_CMD_TOGGLE7K,
    TEST_STATE_SEND_CMD_TOGGLE7L,
    TEST_STATE_SEND_CMD_TOGGLE8,
    TEST_STATE_FINISHED
};


ZB_ZGPD_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 1000)

ZB_ZGPD_DECLARE_COMMISSIONING_CALLBACK()

static void perform_next_state(zb_uint8_t param)
{
    ZVUNUSED(param);
    TEST_DEVICE_CTX.test_state++;

    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_COMMISSIONING:
        zb_zgpd_start_commissioning(comm_cb);
        ZB_ZGPD_SET_PAUSE(2);
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
        g_fc_start = ZGPD->security_frame_counter;
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        ZB_ZGPD_SET_PAUSE(2);
        break;
    case TEST_STATE_SEND_CMD_TOGGLE2:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZB_MEMCPY(ZGPD->ch_replace_key, g_bad_key, ZB_CCM_KEY_SIZE);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_KEY);
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        ZB_ZGP_SET_PASSED_STATE_SEQUENCE();
        break;
    case TEST_STATE_SEND_CMD_TOGGLE3:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->ch_replace_key_type = ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_KEY_TYPE);
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        break;
    case TEST_STATE_SEND_CMD_TOGGLE4:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->ch_replace_sec_level = ZB_ZGP_SEC_LEVEL_NO_SECURITY;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_SEC_LEVEL);
        ZGPD->ch_insert_frame_counter = 1;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_INSERT_FC);
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        break;
    case TEST_STATE_SEND_CMD_TOGGLE5:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->ch_replace_sec_level = ZB_ZGP_SEC_LEVEL_REDUCED;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_SEC_LEVEL);
        ZGPD->ch_insert_frame_counter = 1;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_INSERT_FC);
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        break;
    case TEST_STATE_SEND_CMD_TOGGLE6A:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->security_frame_counter = g_fc_start - 1;
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        ZB_ZGPD_SET_PAUSE(2);
        ZB_ZGP_RESET_PASSED_STATE_SEQUENCE();
        break;
    case TEST_STATE_SEND_CMD_TOGGLE6B:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->security_frame_counter = g_fc_start + 1;
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        ZB_ZGPD_SET_PAUSE(2);
        break;
    case TEST_STATE_SEND_CMD_TOGGLE7A:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->ch_replace_frame_type = ZGP_FRAME_TYPE_RESERVED1;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_FRAME_TYPE);
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        ZB_ZGP_SET_PASSED_STATE_SEQUENCE();
        break;
    case TEST_STATE_SEND_CMD_TOGGLE7B:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->ch_replace_frame_type = ZGP_FRAME_TYPE_RESERVED2;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_FRAME_TYPE);
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        break;
    case TEST_STATE_SEND_CMD_TOGGLE7C:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->ch_replace_frame_type = ZGP_FRAME_TYPE_MAINTENANCE;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_FRAME_TYPE);
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        break;
    case TEST_STATE_SEND_CMD_TOGGLE7D:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->ch_replace_proto_version = 2;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_PROTO_VERSION);
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        break;
    case TEST_STATE_SEND_CMD_TOGGLE7E:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->ch_replace_rxaftertx = 1;
        ZGPD->ch_replace_autocomm  = 1;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_RXAFTERTX);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_AUTO_COMM);
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        break;
    case TEST_STATE_SEND_CMD_TOGGLE7F:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->ch_replace_extnwk_flag = 0;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_EXTNWK_FC_FLAG);
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        break;
    case TEST_STATE_SEND_CMD_TOGGLE7G:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->ch_insert_extnwk_data = 0;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_INSERT_EXTNWK_FC_DATA);
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        break;
    case TEST_STATE_SEND_CMD_TOGGLE7H:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->ch_replace_app_id = 1;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_APPID);
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        break;
    case TEST_STATE_SEND_CMD_TOGGLE7I:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->ch_replace_app_id = 3;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_APPID);
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        break;
    case TEST_STATE_SEND_CMD_TOGGLE7J:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->ch_replace_direction = 1;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_DIRECTION);
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        ZB_ZGPD_SET_PAUSE(2);
        ZB_ZGP_RESET_PASSED_STATE_SEQUENCE();
        break;
    case TEST_STATE_SEND_CMD_TOGGLE7K:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->ch_replace_src_id = 0;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_SRCID);
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        ZB_ZGPD_SET_PAUSE(2);
        break;
    case TEST_STATE_SEND_CMD_TOGGLE7L:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->ch_replace_src_id = (ZGPD->id.addr.src_id) ^ 1;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_SRCID);
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
        ZB_ZGPD_SET_PAUSE(2);
        break;
    case TEST_STATE_SEND_CMD_TOGGLE8:
        ZB_ZGPD_CHACK_RESET_ALL();
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

    /*ZB_ZGPD_SEND_IEEE_SRC_ADDR_IN_COMM_REQ();*/
    ZB_ZGPD_SET_SRC_ID(g_zgpd_srcId);

    ZB_ZGPD_SET_SECURITY_LEVEL(ZB_ZGP_SEC_LEVEL_FULL_WITH_ENC);
    ZB_ZGPD_SET_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL);
    ZB_ZGPD_SET_OOB_KEY(g_oob_key);
    ZB_ZGPD_REQUEST_SECURITY_KEY();


    ZGPD->channel = TEST_CHANNEL;
}

#endif /* ZB_CERTIFICATION_HACKS */

ZB_ZGPD_TH_DECLARE_STARTUP_PROCESS()

