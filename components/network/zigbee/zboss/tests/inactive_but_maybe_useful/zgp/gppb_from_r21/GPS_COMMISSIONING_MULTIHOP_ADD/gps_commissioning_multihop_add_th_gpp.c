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
/* PURPOSE: TH-GPP/TH-TOOL
*/

#define ZB_TEST_NAME GPS_COMMISSIONING_MULTIHOP_ADD_TH_GPP
#define ZB_TRACE_FILE_ID 41494

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_zcl.h"
#include "zb_secur_api.h"
#include "zb_ha.h"
#include "test_config.h"
#include "zb_debug.h"

#ifdef ZB_ENABLE_HA
#include "../include/zgp_test_templates.h"

/*============================================================================*/
/*                             DECLARATIONS                                   */
/*============================================================================*/

static zb_ieee_addr_t g_th_tool_addr = TH_GPP_IEEE_ADDR;
static zb_uint8_t g_key_nwk[ZB_CCM_KEY_SIZE] = NWK_KEY;
static zb_uint8_t g_oob_key[ZB_CCM_KEY_SIZE] = TEST_OOB_KEY;
static zb_uint16_t g_dut_addr;

static void dev_annce_cb(zb_zdo_device_annce_t *da);
static void gp_proxy_comm_mode_handler_cb(zb_uint8_t buf_ref);

static void send_gp_comm_notify(zb_uint8_t param);
static void fill_gpdf_info(
    zb_gpdf_info_t *gpdf_info,
    zb_uint32_t frame_cnt,
    zb_uint16_t opt);
static zb_uint16_t request_options();
/* Calculates MIC */
static void encrypt_payload(zb_gpdf_info_t *gpdf_info, zb_buf_t *packet);

static void send_channel_request(zb_uint8_t param);
static void send_commissioning(zb_uint8_t param, zb_uint32_t frame_cnt);

/* Specific */
static void construct_aes_nonce(
    zb_bool_t from_gpd,
    zb_zgpd_id_t *zgpd_id,
    zb_uint32_t frame_counter,
    zb_zgp_aes_nonce_t *res_nonce);

/*============================================================================*/
/*                             FSM CORE                                       */
/*============================================================================*/

/*! Program states according to test scenario */
enum test_states_e
{
    TEST_STATE_INITIAL,
    /* Initial */
    TEST_STATE_WAIT_DUT_STARTUP,
    /* STEP 0 */
    TEST_STATE_WAIT_COMM_MODE_ENTER_0,
    TEST_STATE_SEND_COMM_NOTIFY_0,
    /* STEP 1 */
    TEST_STATE_WAIT_COMM_MODE_ENTER_1,
    TEST_STATE_SEND_COMM_NOTIFY1_1,
    TEST_STATE_SEND_COMM_NOTIFY2_1,
    TEST_STATE_SEND_COMM_NOTIFY3_1,
    /* STEP 2A */
    TEST_STATE_WAIT_COMM_MODE_ENTER_2A,
    TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_2A,
    TEST_STATE_SEND_COMM_NOTIFY1_2A,
    TEST_STATE_SEND_COMM_NOTIFY2_2A,
    TEST_STATE_SEND_COMM_NOTIFY3_2A,
    /* STEP 2B */
    TEST_STATE_SEND_COMM_NOTIFY_2B,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_2B,
    /* STEP 2C */
    TEST_STATE_SEND_COMM_NOTIFY_2C,
    /* STEP 3.0 */
    TEST_STATE_WAIT_COMM_MODE_ENTER_3,
    TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_3,
    TEST_STATE_SEND_COMMISSIONING_NOTIFY_3,
    /* STEP 3A */
    TEST_STATE_SEND_COMM_NOTIFY_3A,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_3A,
    /* STEP 3B */
    TEST_STATE_SEND_COMM_NOTIFY_3B,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_3B,
    /* STEP 3C */
    TEST_STATE_SEND_COMM_NOTIFY_3C,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_3C,
    /* STEP 3D */
    TEST_STATE_SEND_COMM_NOTIFY_3D,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_3D,
    /* STEP 3E */
    TEST_STATE_SEND_COMM_NOTIFY_3E,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_3E,
    /* STEP 3F */
    TEST_STATE_SEND_COMM_NOTIFY_3F,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_3F,
    /* STEP 3G */
    TEST_STATE_SEND_COMM_NOTIFY_3G,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_3G,
    /* STEP 3H */
    TEST_STATE_SEND_COMM_NOTIFY_3H,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_3H,
    /* STEP 3I */
    TEST_STATE_SEND_COMM_NOTIFY_3I,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_3I,
    /* STEP 3J */
    TEST_STATE_WAIT_COMM_MODE_ENTER_3J,
    TEST_STATE_SEND_COMM_NOTIFY_3J,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_3J,
    /* STEP 3L */
    TEST_STATE_WAIT_COMM_MODE_ENTER_3L,
    TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_3L,
    TEST_STATE_SEND_COMMISSIONING_NOTIFY_3L,
    TEST_STATE_SEND_COMM_NOTIFY_3L,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_3L,
    /* STEP 3M */
    TEST_STATE_SEND_COMM_NOTIFY_3M,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_3M,
    /* STEP 3N */
    TEST_STATE_SEND_COMM_NOTIFY_3N,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_3N,
    /* STEP 4A */
    TEST_STATE_WAIT_COMM_MODE_ENTER_4A,
    TEST_STATE_CLEAN_TH_PROXY_TABLE_4A,
    TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_4A,
    TEST_STATE_SEND_COMMISSIONING_NOTIFY_4A,
    TEST_STATE_SEND_COMM_NOTIFY_4A,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_4A,
    /* STEP 4B */
    TEST_STATE_WAIT_COMM_MODE_ENTER_4B,
    TEST_STATE_CLEAN_TH_PROXY_TABLE_4B,
    TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_4B,
    TEST_STATE_SEND_COMMISSIONING_NOTIFY_4B,
    TEST_STATE_SEND_COMM_NOTIFY_4B,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_4B,
    /* STEP 5A */
    TEST_STATE_WAIT_COMM_MODE_ENTER_5A,
    TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_5A,
    TEST_STATE_SEND_COMM_NOTIFY_5A,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_5A,
    /* STEP 5B */
    TEST_STATE_WAIT_COMM_MODE_ENTER_5B,
    TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_5B,
    TEST_STATE_SEND_COMM_NOTIFY_5B,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_5B,
    /* STEP 6A */
    TEST_STATE_WAIT_COMM_MODE_ENTER_6A,
    TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_6A,
    TEST_STATE_SEND_COMM_NOTIFY_6A,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_6A,
    /* STEP 6B */
    TEST_STATE_WAIT_COMM_MODE_ENTER_6B,
    TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_6B,
    TEST_STATE_SEND_COMM_NOTIFY_6B,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_6B,
    /* FINISH */
    TEST_STATE_FINISHED
};

ZB_ZGPC_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 1000)

static void send_zcl(zb_uint8_t buf_ref, zb_callback_t cb)
{
    ZVUNUSED(cb);

    TRACE_MSG(TRACE_APP1, ">send_zcl: test_state = %d", (FMT__D, TEST_DEVICE_CTX.test_state));

    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_SEND_COMM_NOTIFY_0:
    case TEST_STATE_SEND_COMM_NOTIFY1_1:
    case TEST_STATE_SEND_COMM_NOTIFY2_1:
    case TEST_STATE_SEND_COMM_NOTIFY3_1:
    case TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_2A:
    case TEST_STATE_SEND_COMM_NOTIFY1_2A:
    case TEST_STATE_SEND_COMM_NOTIFY2_2A:
    case TEST_STATE_SEND_COMM_NOTIFY3_2A:
    case TEST_STATE_SEND_COMM_NOTIFY_2C:
    case TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_3:
    case TEST_STATE_SEND_COMMISSIONING_NOTIFY_3:
    case TEST_STATE_SEND_COMM_NOTIFY_3A:
    case TEST_STATE_SEND_COMM_NOTIFY_3B:
    case TEST_STATE_SEND_COMM_NOTIFY_3C:
    case TEST_STATE_SEND_COMM_NOTIFY_3D:
    case TEST_STATE_SEND_COMM_NOTIFY_3E:
    case TEST_STATE_SEND_COMM_NOTIFY_3F:
    case TEST_STATE_SEND_COMM_NOTIFY_3G:
    case TEST_STATE_SEND_COMM_NOTIFY_3H:
    case TEST_STATE_SEND_COMM_NOTIFY_3I:
    case TEST_STATE_SEND_COMM_NOTIFY_3J:
    case TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_3L:
    case TEST_STATE_SEND_COMMISSIONING_NOTIFY_3L:
    case TEST_STATE_SEND_COMM_NOTIFY_3L:
    case TEST_STATE_SEND_COMM_NOTIFY_3M:
    case TEST_STATE_SEND_COMM_NOTIFY_3N:
    case TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_4A:
    case TEST_STATE_SEND_COMMISSIONING_NOTIFY_4A:
    case TEST_STATE_SEND_COMM_NOTIFY_4A:
    case TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_4B:
    case TEST_STATE_SEND_COMMISSIONING_NOTIFY_4B:
    case TEST_STATE_SEND_COMM_NOTIFY_4B:
    case TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_5A:
    case TEST_STATE_SEND_COMM_NOTIFY_5A:
    case TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_5B:
    case TEST_STATE_SEND_COMM_NOTIFY_5B:
    case TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_6A:
    case TEST_STATE_SEND_COMM_NOTIFY_6A:
    case TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_6B:
    case TEST_STATE_SEND_COMM_NOTIFY_6B:
        send_gp_comm_notify(buf_ref);
        break;

    case TEST_STATE_SEND_COMM_NOTIFY_2B:
        ZB_ZGPC_SET_PAUSE(3);
        send_gp_comm_notify(buf_ref);
        break;

    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_2B:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_3A:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_3B:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_3C:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_3D:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_3E:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_3F:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_3G:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_3H:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_3I:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_3J:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_3L:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_3M:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_3N:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_4A:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_4B:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_5A:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_5B:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_6A:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_6B:
        ZB_ZGPC_SET_PAUSE(2);
        zgp_cluster_read_attr(buf_ref, g_dut_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                              ZB_ZCL_ATTR_GPS_SINK_TABLE_ID,
                              ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
        break;
    }

    TRACE_MSG(TRACE_APP1, "<send_zcl", (FMT__0));
}

static void perform_next_state(zb_uint8_t param)
{
    ZVUNUSED(param);
    if (TEST_DEVICE_CTX.pause)
    {
        ZB_SCHEDULE_ALARM(perform_next_state, 0,
                          ZB_TIME_ONE_SECOND * TEST_DEVICE_CTX.pause);
        TEST_DEVICE_CTX.pause = 0;
        return;
    }

    TEST_DEVICE_CTX.test_state++;

    TRACE_MSG(TRACE_APP1, ">perform_next_state: test_state = %d",
              (FMT__D, TEST_DEVICE_CTX.test_state));

    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_WAIT_DUT_STARTUP:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_0:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_1:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_2A:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_3:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_3J:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_3L:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_4A:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_4B:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_5A:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_5B:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_6A:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_6B:
        /* Transition to next state should be performed in handlers */
        break;

    case TEST_STATE_CLEAN_TH_PROXY_TABLE_4A:
    case TEST_STATE_CLEAN_TH_PROXY_TABLE_4B:
        zgp_tbl_clear();
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;

    case TEST_STATE_FINISHED:
        TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
        break;
    default:
    {
        ZB_SCHEDULE_ALARM(test_send_command, 0, ZB_TIME_ONE_SECOND);
    }
    }

    TRACE_MSG(TRACE_APP1, "<perform_next_state", (FMT__0));
}

/*============================================================================*/
/*                            TEST IMPLEMENTATION                             */
/*============================================================================*/

static void dev_annce_cb(zb_zdo_device_annce_t *da)
{
    zb_ieee_addr_t dut_ieee = DUT_GPS_IEEE_ADDR;
    TRACE_MSG(TRACE_APP1, "dev_annce_cb: ieee = " TRACE_FORMAT_64 " NWK = 0x%x",
              (FMT__A_H, TRACE_ARG_64(da->ieee_addr), da->nwk_addr));

    if (ZB_IEEE_ADDR_CMP(dut_ieee, da->ieee_addr) == ZB_TRUE)
    {
        g_dut_addr = da->nwk_addr;
        if (TEST_DEVICE_CTX.test_state == TEST_STATE_WAIT_DUT_STARTUP)
        {
            ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        }
    }
}

static void gp_proxy_comm_mode_handler_cb(zb_uint8_t buf_ref)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(buf_ref);
    zb_uint8_t opt = *ZB_BUF_BEGIN(buf);

    TRACE_MSG(TRACE_APP1, ">gp_proxy_comm_mode_handler_cb: buf_ref = %d, test_state = %d",
              (FMT__D_D, buf_ref, TEST_DEVICE_CTX.test_state));

    ZVUNUSED(buf_ref);
    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_WAIT_COMM_MODE_ENTER_0:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_1:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_2A:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_3:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_3J:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_3L:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_4A:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_4B:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_5A:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_5B:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_6A:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_6B:
        if (ZB_ZGP_COMM_MODE_OPT_GET_ACTION(opt))
        {
            ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        }
        break;
    }

    TRACE_MSG(TRACE_APP1, "<gp_proxy_comm_mode_handler_cb", (FMT__0));
}

static void send_gp_comm_notify(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_uint16_t options = 0;
    zb_gpdf_info_t *gpdf_info = ZB_GET_BUF_PARAM(buf, zb_gpdf_info_t);
    static zb_uint32_t frame_cnt;
    static zb_uint32_t remember_frame_cnt;
    static zb_uint32_t use_frame_cnt;
    zb_bool_t encrypt_frame = ZB_FALSE;

    TRACE_MSG(TRACE_APP1, ">send_gp_comm_notify: param = %d, test_state = %d",
              (FMT__D_D, param, TEST_DEVICE_CTX.test_state));

    use_frame_cnt = frame_cnt;
    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_SEND_COMMISSIONING_NOTIFY_3:
        remember_frame_cnt = frame_cnt;
        break;
    case TEST_STATE_SEND_COMM_NOTIFY_3B:
        use_frame_cnt = remember_frame_cnt - 1;
    /* FALLTHROUGH */
    case TEST_STATE_SEND_COMM_NOTIFY_3C:
    case TEST_STATE_SEND_COMM_NOTIFY_3D:
    case TEST_STATE_SEND_COMM_NOTIFY_3E:
    case TEST_STATE_SEND_COMM_NOTIFY_3L:
    case TEST_STATE_SEND_COMM_NOTIFY_4A:
    case TEST_STATE_SEND_COMM_NOTIFY_4B:
        encrypt_frame = ZB_TRUE;
        break;
    case TEST_STATE_SEND_COMM_NOTIFY_3F:
        use_frame_cnt = remember_frame_cnt - 1;
        break;
    case TEST_STATE_SEND_COMM_NOTIFY_3I:
        use_frame_cnt = 0x00000000;
        break;
    }

    options = request_options();
    fill_gpdf_info(gpdf_info, use_frame_cnt, options);

    switch (gpdf_info->zgpd_cmd_id)
    {
    case ZB_GPDF_CMD_CHANNEL_REQUEST:
        send_channel_request(param);
        break;
    case ZB_GPDF_CMD_COMMISSIONING:
        send_commissioning(param, use_frame_cnt);
        break;
    }

    if (encrypt_frame)
    {
        zb_uint8_t *ptr;
        ZB_BUF_ALLOC_LEFT(buf, 1, ptr);
        *ptr = gpdf_info->zgpd_cmd_id;

        encrypt_payload(gpdf_info, buf);
        gpdf_info->zgpd_cmd_id = *ptr;
        ZB_BUF_CUT_LEFT(buf, 1, ptr);

        switch (TEST_DEVICE_CTX.test_state)
        {
        case TEST_STATE_SEND_COMM_NOTIFY_3D:
        case TEST_STATE_SEND_COMM_NOTIFY_3L:

            /* Corrupt MIC */
        {
            zb_uint8_t i;
            for (i = 0; i < ZB_CCM_M; ++i)
            {
                gpdf_info->mic[i] ^= 0xFF;
            }
        }
        }
    }

    zb_zgp_cluster_gp_comm_notification_req(param,
                                            0 /* do not use alias */, 0x0000, 0x00,
                                            g_dut_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                            options,
                                            NULL);
    frame_cnt++;

    TRACE_MSG(TRACE_APP1, "<send_gp_comm_notify", (FMT__0));
}

static void fill_gpdf_info(
    zb_gpdf_info_t *gpdf_info,
    zb_uint32_t frame_cnt,
    zb_uint16_t opt)
{
    static zb_ieee_addr_t ieee_n = TH_GPD_IEEE_ADDR;
    zb_uint8_t frame_type = ZGP_FRAME_TYPE_DATA;

    TRACE_MSG(TRACE_APP1, ">fill_gpd_info: gpdf_info = %p", (FMT__P, gpdf_info));

    ZB_BZERO(gpdf_info, sizeof(*gpdf_info));
    /* Always excellent */
    gpdf_info->rssi = 0x3;
    gpdf_info->lqi = 0x3b;
    if (ZB_ZGP_COMM_NOTIF_OPT_GET_APP_ID(opt) == ZB_ZGP_APP_ID_0000)
    {
        gpdf_info->zgpd_id.app_id = ZB_ZGP_APP_ID_0000;
        gpdf_info->zgpd_id.addr.src_id = TEST_ZGPD_SRC_ID;
    }
    else if (ZB_ZGP_COMM_NOTIF_OPT_GET_APP_ID(opt) == ZB_ZGP_APP_ID_0010)
    {
        gpdf_info->zgpd_id.app_id = ZB_ZGP_APP_ID_0010;
        gpdf_info->zgpd_id.endpoint = TEST_ZGPD_EP_X;
        ZB_IEEE_ADDR_COPY(gpdf_info->zgpd_id.addr.ieee_addr, ieee_n);
    }

    gpdf_info->sec_frame_counter = frame_cnt;
    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_SEND_COMM_NOTIFY_0:
    case TEST_STATE_SEND_COMM_NOTIFY1_1:
    case TEST_STATE_SEND_COMM_NOTIFY2_1:
    case TEST_STATE_SEND_COMM_NOTIFY3_1:
    case TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_2A:
    case TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_3:
    case TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_3L:
    case TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_4A:
    case TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_4B:
    case TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_5A:
    case TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_5B:
    case TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_6A:
    case TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_6B:
        frame_type = ZGP_FRAME_TYPE_MAINTENANCE;
        gpdf_info->zgpd_cmd_id = ZB_GPDF_CMD_CHANNEL_REQUEST;
        ZB_BZERO(&gpdf_info->zgpd_id, sizeof(gpdf_info->zgpd_id));
        break;
    case TEST_STATE_SEND_COMM_NOTIFY1_2A:
    case TEST_STATE_SEND_COMM_NOTIFY2_2A:
    case TEST_STATE_SEND_COMM_NOTIFY3_2A:
    case TEST_STATE_SEND_COMMISSIONING_NOTIFY_3:
    case TEST_STATE_SEND_COMMISSIONING_NOTIFY_3L:
    case TEST_STATE_SEND_COMMISSIONING_NOTIFY_4A:
    case TEST_STATE_SEND_COMMISSIONING_NOTIFY_4B:
    case TEST_STATE_SEND_COMM_NOTIFY_5A:
    case TEST_STATE_SEND_COMM_NOTIFY_5B:
        gpdf_info->zgpd_cmd_id = ZB_GPDF_CMD_COMMISSIONING;
        break;
    case TEST_STATE_SEND_COMM_NOTIFY_2B:
    case TEST_STATE_SEND_COMM_NOTIFY_2C:
    case TEST_STATE_SEND_COMM_NOTIFY_3A:
    case TEST_STATE_SEND_COMM_NOTIFY_3B:
    case TEST_STATE_SEND_COMM_NOTIFY_3C:
    case TEST_STATE_SEND_COMM_NOTIFY_3D:
    case TEST_STATE_SEND_COMM_NOTIFY_3F:
    case TEST_STATE_SEND_COMM_NOTIFY_3G:
    case TEST_STATE_SEND_COMM_NOTIFY_3I:
    case TEST_STATE_SEND_COMM_NOTIFY_3J:
    case TEST_STATE_SEND_COMM_NOTIFY_3L:
    case TEST_STATE_SEND_COMM_NOTIFY_4A:
    case TEST_STATE_SEND_COMM_NOTIFY_4B:
        gpdf_info->zgpd_cmd_id = ZB_GPDF_CMD_SUCCESS;
        break;
    case TEST_STATE_SEND_COMM_NOTIFY_3E:
    case TEST_STATE_SEND_COMM_NOTIFY_3H:
        gpdf_info->zgpd_cmd_id = ZB_GPDF_CMD_SUCCESS;
        gpdf_info->zgpd_id.addr.src_id ^= 0xFFFFFFFF;
        break;
    case TEST_STATE_SEND_COMM_NOTIFY_3M:
    {
        zb_uint8_t i;
        for (i = 0; i < sizeof(gpdf_info->zgpd_id.addr.ieee_addr); ++i)
        {
            gpdf_info->zgpd_id.addr.ieee_addr[i] ^= 0xFF;
        }
        gpdf_info->zgpd_cmd_id = ZB_GPDF_CMD_SUCCESS;
        break;
    }
    case TEST_STATE_SEND_COMM_NOTIFY_3N:
        gpdf_info->zgpd_cmd_id = ZB_GPDF_CMD_SUCCESS;
        gpdf_info->zgpd_id.endpoint ^= 0xFF;
        break;
    case TEST_STATE_SEND_COMM_NOTIFY_6A:
        gpdf_info->zgpd_cmd_id = ZB_GPDF_CMD_COMMISSIONING;
        gpdf_info->zgpd_id.addr.src_id = 0x00000000;
        break;
    case TEST_STATE_SEND_COMM_NOTIFY_6B:
        gpdf_info->zgpd_cmd_id = ZB_GPDF_CMD_COMMISSIONING;
        ZB_IEEE_ADDR_ZERO(gpdf_info->zgpd_id.addr.ieee_addr);
        break;
    }

    TRACE_MSG(TRACE_APP2, "FILL_GPDF:  options = 0x%x", (FMT__H, opt));

    ZB_GPDF_NWK_FRAME_CTL_EXT(gpdf_info->nwk_ext_frame_ctl,
                              ZB_ZGP_COMM_NOTIF_OPT_GET_APP_ID(opt),
                              ZB_ZGP_GP_COMM_NOTIF_OPT_GET_SEC_LVL(opt),
                              /* Security Key field: */
                              ZB_ZGP_GP_COMM_NOTIF_OPT_GET_KEY_TYPE(opt) == ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL
                              || ZB_ZGP_GP_COMM_NOTIF_OPT_GET_KEY_TYPE(opt) == ZB_ZGP_SEC_KEY_TYPE_DERIVED_INDIVIDUAL,
                              ZB_ZGP_GP_COMM_NOTIF_OPT_GET_RX_AFTER_TX(opt),
                              ZGP_FRAME_DIR_FROM_ZGPD /* Dir: always from ZGPD */);

    ZB_GPDF_NWK_FRAME_CONTROL(gpdf_info->nwk_frame_ctl,
                              frame_type,
                              0, /* Auto-Commissioning */
                              frame_type != ZGP_FRAME_TYPE_MAINTENANCE &&
                              gpdf_info->nwk_ext_frame_ctl /* frame_ext */);

    TRACE_MSG(TRACE_APP2, "FILL_GPDF: nwk_opt = 0x%x, nwk_ext_opt = 0x%x",
              (FMT__H, gpdf_info->nwk_frame_ctl, gpdf_info->nwk_ext_frame_ctl));

    TRACE_MSG(TRACE_APP1, "<fill_gpd_info", (FMT__0));
}

static zb_uint16_t request_options()
{
    zb_uint16_t opt = 0;

    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_SEND_COMM_NOTIFY_0: /* Channel Request: malformed */
    case TEST_STATE_SEND_COMM_NOTIFY_3A: /* Success: malformed */
    {
        opt = ZB_ZGP_FILL_COMM_NOTIFICATION_OPTIONS(ZB_ZGP_APP_ID_0000,
                0, /* RxAfterTX */
                ZB_ZGP_SEC_LEVEL_NO_SECURITY,
                ZB_ZGP_SEC_KEY_TYPE_NO_KEY,
                0, /* Security processing not failed */
                0, /* not BidirCap */
                1 /* ProxyInfo Present */);
        break;
    }

    case TEST_STATE_SEND_COMM_NOTIFY1_1: /* Channel Request: valid */
    case TEST_STATE_SEND_COMM_NOTIFY2_1:
    case TEST_STATE_SEND_COMM_NOTIFY3_1:
    case TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_2A:
    case TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_3:
    case TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_3L:
    case TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_4A:
    case TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_4B:
    case TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_5A:
    case TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_5B:
    case TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_6A:
    case TEST_STATE_SEND_CHANNEL_REQ_NOTIFY_6B:
    /* FALLTHROUGH */
    case TEST_STATE_SEND_COMM_NOTIFY1_2A: /* Commissioning: app_id = 0b0000 */
    case TEST_STATE_SEND_COMM_NOTIFY2_2A:
    case TEST_STATE_SEND_COMM_NOTIFY3_2A:
    case TEST_STATE_SEND_COMMISSIONING_NOTIFY_3:
    case TEST_STATE_SEND_COMM_NOTIFY_5A:
    case TEST_STATE_SEND_COMM_NOTIFY_5B:
    case TEST_STATE_SEND_COMM_NOTIFY_6A:
    {
        opt = ZB_ZGP_FILL_COMM_NOTIFICATION_OPTIONS(ZB_ZGP_APP_ID_0000,
                1, /* RxAfterTX */
                ZB_ZGP_SEC_LEVEL_NO_SECURITY,
                ZB_ZGP_SEC_KEY_TYPE_NO_KEY,
                0, /* Security processing not failed */
                0, /* not BidirCap */
                1 /* ProxyInfo Present */);
        break;
    }

    case TEST_STATE_SEND_COMM_NOTIFY_2B: /* Success: app_id = 0b0000 */
    case TEST_STATE_SEND_COMM_NOTIFY_2C:
    case TEST_STATE_SEND_COMM_NOTIFY_3F:
    case TEST_STATE_SEND_COMM_NOTIFY_3H:
    case TEST_STATE_SEND_COMM_NOTIFY_3I:
    case TEST_STATE_SEND_COMM_NOTIFY_3J:
    {
        opt = ZB_ZGP_FILL_COMM_NOTIFICATION_OPTIONS(ZB_ZGP_APP_ID_0000,
                0, /* RxAfterTX */
                ZB_ZGP_SEC_LEVEL_FULL_NO_ENC,
                ZB_ZGP_SEC_KEY_TYPE_NWK,
                0, /* Security processing not failed */
                0, /* not BidirCap */
                1 /* ProxyInfo Present */);
        break;
    }

    case TEST_STATE_SEND_COMM_NOTIFY_3B: /* Success: app_id = 0b0000, security
                                          * processing failed */
    case TEST_STATE_SEND_COMM_NOTIFY_3D:
    case TEST_STATE_SEND_COMM_NOTIFY_3E:
    {
        opt = ZB_ZGP_FILL_COMM_NOTIFICATION_OPTIONS(ZB_ZGP_APP_ID_0000,
                0, /* RxAfterTX */
                ZB_ZGP_SEC_LEVEL_FULL_NO_ENC,
                ZB_ZGP_SEC_KEY_TYPE_NWK,
                1, /* Security processing failed */
                0, /* not BidirCap */
                1 /* ProxyInfo Present */);
        break;
    }

    case TEST_STATE_SEND_COMM_NOTIFY_3C: /* Success: app_id = 0b0000, malformed */
    {
        opt = ZB_ZGP_FILL_COMM_NOTIFICATION_OPTIONS(ZB_ZGP_APP_ID_0000,
                0, /* RxAfterTX */
                ZB_ZGP_SEC_LEVEL_FULL_NO_ENC,
                ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL,
                1, /* Security processing failed */
                0, /* not BidirCap */
                1 /* ProxyInfo Present */);
        break;
    }
    case TEST_STATE_SEND_COMM_NOTIFY_3G: /* Success: app_id = 0b0000, malformed */
    {
        opt = ZB_ZGP_FILL_COMM_NOTIFICATION_OPTIONS(ZB_ZGP_APP_ID_0000,
                0, /* RxAfterTX */
                ZB_ZGP_SEC_LEVEL_FULL_NO_ENC,
                ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL,
                0, /* Security processing not failed */
                0, /* not BidirCap */
                1 /* ProxyInfo Present */);
        break;
    }

    case TEST_STATE_SEND_COMMISSIONING_NOTIFY_3L: /* Commissioning: app_id = 0b0010 */
    case TEST_STATE_SEND_COMMISSIONING_NOTIFY_4A:
    case TEST_STATE_SEND_COMMISSIONING_NOTIFY_4B:
    case TEST_STATE_SEND_COMM_NOTIFY_6B:
    {
        opt = ZB_ZGP_FILL_COMM_NOTIFICATION_OPTIONS(ZB_ZGP_APP_ID_0010,
                1, /* RxAfterTX */
                ZB_ZGP_SEC_LEVEL_NO_SECURITY,
                ZB_ZGP_SEC_KEY_TYPE_NO_KEY,
                0, /* Security processing not failed */
                0, /* not BidirCap */
                1 /* ProxyInfo Present */);
        break;
    }

    case TEST_STATE_SEND_COMM_NOTIFY_3L: /* Success: app_id = 0b0010, Security
                                          * processing failed */
    case TEST_STATE_SEND_COMM_NOTIFY_4B:
    {
        opt = ZB_ZGP_FILL_COMM_NOTIFICATION_OPTIONS(ZB_ZGP_APP_ID_0010,
                0, /* RxAfterTX */
                ZB_ZGP_SEC_LEVEL_FULL_NO_ENC,
                ZB_ZGP_SEC_KEY_TYPE_NWK,
                1, /* Security processing failed */
                0, /* not BidirCap */
                1 /* ProxyInfo Present */);
        break;
    }
    case TEST_STATE_SEND_COMM_NOTIFY_3M: /* Success: app_id = 0b0010 */
    case TEST_STATE_SEND_COMM_NOTIFY_3N:
    {
        opt = ZB_ZGP_FILL_COMM_NOTIFICATION_OPTIONS(ZB_ZGP_APP_ID_0010,
                0, /* RxAfterTX */
                ZB_ZGP_SEC_LEVEL_FULL_NO_ENC,
                ZB_ZGP_SEC_KEY_TYPE_NWK,
                0, /* Security processing failed */
                0, /* not BidirCap */
                1 /* ProxyInfo Present */);
        break;
    }

    case TEST_STATE_SEND_COMM_NOTIFY_4A: /* Success: app_id = 0b0010, full encryption */
    {
        opt = ZB_ZGP_FILL_COMM_NOTIFICATION_OPTIONS(ZB_ZGP_APP_ID_0010,
                0, /* RxAfterTX */
                ZB_ZGP_SEC_LEVEL_FULL_WITH_ENC,
                ZB_ZGP_SEC_KEY_TYPE_NWK,
                1, /* Security processing failed */
                0, /* not BidirCap */
                1 /* ProxyInfo Present */);
        break;
    }
    }

    TRACE_MSG(TRACE_APP1, "request_options: opt = 0x%x", (FMT__H, opt));

    return opt;
}

static void encrypt_payload(zb_gpdf_info_t *gpdf_info, zb_buf_t *packet)
{
    zb_uint8_t *payload;
    zb_int8_t pld_len;
    zb_uint8_t sec_level = ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl);
    zb_zgp_aes_nonce_t nonce;
    zb_uint8_t pseudo_header[16];
    zb_uint8_t *ptr = pseudo_header;
    zb_uint8_t hdr_len = 0;

    construct_aes_nonce(
        (zb_bool_t)!ZB_GPDF_EXT_NFC_GET_DIRECTION(gpdf_info->nwk_ext_frame_ctl),
        &gpdf_info->zgpd_id,
        gpdf_info->sec_frame_counter,
        &nonce);

    /* Form Header */
    ZB_BZERO(pseudo_header, 16);
    *ptr++ = gpdf_info->nwk_frame_ctl;
    ++hdr_len;
    if (gpdf_info->nwk_ext_frame_ctl)
    {
        *ptr++ = gpdf_info->nwk_ext_frame_ctl;
        ++hdr_len;
    }
    if (gpdf_info->zgpd_id.app_id == ZB_ZGP_APP_ID_0000)
    {
        ptr = zb_put_next_htole32(ptr, gpdf_info->zgpd_id.addr.src_id);
        hdr_len += sizeof(gpdf_info->zgpd_id.addr.src_id);
    }
    else if (gpdf_info->zgpd_id.app_id == ZB_ZGP_APP_ID_0010)
    {
        *ptr++ = gpdf_info->zgpd_id.endpoint;
        ++hdr_len;
    }

    zb_put_next_htole32(ptr, gpdf_info->sec_frame_counter);
    hdr_len += sizeof(gpdf_info->sec_frame_counter);

    payload = ZB_BUF_BEGIN(packet);
    pld_len = ZB_BUF_LEN(packet);

    TRACE_MSG(TRACE_ERROR, "ENCRYPT: dump payload", (FMT__0));
    dump_traf(payload, pld_len);
    TRACE_MSG(TRACE_ERROR, "ENCRYPT: dump header", (FMT__0));
    dump_traf(pseudo_header, hdr_len);
    TRACE_MSG(TRACE_ERROR, "ENCRYPT: dump nonce", (FMT__0));
    dump_traf((zb_uint8_t *) &nonce, sizeof(nonce));

    if (sec_level == ZB_ZGP_SEC_LEVEL_FULL_WITH_ENC)
    {
        zb_ccm_encrypt_n_auth(g_key_nwk, (zb_uint8_t *)&nonce,
                              pseudo_header, hdr_len,
                              payload, pld_len,
                              SEC_CTX().encryption_buf2);

        ZB_MEMCPY(ZB_BUF_BEGIN(packet),
                  ZB_BUF_BEGIN(SEC_CTX().encryption_buf2) + hdr_len,
                  pld_len);
        ZB_MEMCPY(gpdf_info->mic,
                  ZB_BUF_BEGIN(SEC_CTX().encryption_buf2) + hdr_len + pld_len,
                  4);
    }
    else
    {
        hdr_len += pld_len;
        zb_ccm_encrypt_n_auth(g_key_nwk, (zb_uint8_t *)&nonce,
                              pseudo_header, hdr_len,
                              NULL, 0,
                              SEC_CTX().encryption_buf2);

        ZB_MEMCPY(gpdf_info->mic,
                  ZB_BUF_BEGIN(SEC_CTX().encryption_buf2) + hdr_len,
                  ZB_CCM_M);
    }

    TRACE_MSG(TRACE_ERROR, "ENCRYPT: mic = 0x%x", (FMT__L, gpdf_info->mic));
}


static void send_channel_request(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_uint8_t *ptr = ZB_BUF_BEGIN(buf);
    zb_uint8_t cur_ch = ZB_PIBCACHE_CURRENT_CHANNEL() - ZB_ZGPD_FIRST_CH;

    TRACE_MSG(TRACE_APP1, ">send_channel_request: param = %d", (FMT__D, param));

    ZB_BUF_INITIAL_ALLOC(buf, 1, ptr);
    *ptr = cur_ch | (cur_ch << 4);

    TRACE_MSG(TRACE_APP1, "<send_channel_request", (FMT__0));
}

static void send_commissioning(zb_uint8_t param, zb_uint32_t frame_cnt)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_uint8_t *ptr = ZB_BUF_BEGIN(buf);
    zb_uint8_t opt, ext_opt;
    zb_uint8_t encrypted_key[ZB_CCM_KEY_SIZE];
    zb_uint8_t mic[ZB_CCM_M];
    zb_gpdf_info_t *gpdf_info = ZB_GET_BUF_PARAM(buf, zb_gpdf_info_t);
    zb_uint8_t secur_lvl;
    zb_uint8_t s_k_r, s_k_p, s_k_e;
    zb_uint8_t comm_pld_len = 3;

    TRACE_MSG(TRACE_APP1, ">send_commissioning: param = %d", (FMT__D, param));

    s_k_r = 1;
    s_k_p = 1;
    s_k_e = 1;
    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_SEND_COMMISSIONING_NOTIFY_4A:
    case TEST_STATE_SEND_COMMISSIONING_NOTIFY_4B:
        secur_lvl = ZB_ZGP_SEC_LEVEL_FULL_WITH_ENC;
        break;
    case TEST_STATE_SEND_COMM_NOTIFY_5A:
        secur_lvl = ZB_ZGP_SEC_LEVEL_REDUCED;
        break;
    case TEST_STATE_SEND_COMM_NOTIFY_5B:
        secur_lvl = ZB_ZGP_SEC_LEVEL_FULL_NO_ENC;
        s_k_r = s_k_p = s_k_e = 0;
        break;

    default:
        secur_lvl = ZB_ZGP_SEC_LEVEL_FULL_NO_ENC;
    }

    /* SN, Key Request and ext options */
    opt = ZB_GPDF_COMM_OPT_FLD(1, 0, 0, 0, s_k_r, 0, 1);
    /* key present, encrypted and frame counter also present */
    ext_opt = ZB_GPDF_COMM_EXT_OPT_FLD(secur_lvl,
                                       ZB_ZGP_SEC_KEY_TYPE_NWK,
                                       s_k_p, s_k_p && s_k_e, s_k_p);
    if (s_k_p && s_k_e)
    {
        zb_zgpd_id_t id;
        zb_uint8_t link_key[ZB_CCM_KEY_SIZE] = ZB_STANDARD_TC_KEY;

        id = gpdf_info->zgpd_id;
        zb_zgp_protect_gpd_key(ZB_TRUE, &id, g_oob_key, link_key, encrypted_key, gpdf_info->sec_frame_counter, mic);
    }

    comm_pld_len += (s_k_p) ? ((s_k_e) ? 24 : 20) : 0;
    ZB_BUF_INITIAL_ALLOC(buf, comm_pld_len, ptr);
    *ptr++ = ZB_ZGP_ON_OFF_SWITCH_DEV_ID;
    *ptr++ = opt;
    *ptr++ = ext_opt;
    if (s_k_p)
    {
        if (s_k_e)
        {
            ZB_MEMCPY(ptr, encrypted_key, ZB_CCM_KEY_SIZE);
            ptr += ZB_CCM_KEY_SIZE;
            ZB_MEMCPY(ptr, mic, ZB_CCM_M);
            ptr += ZB_CCM_M;
        }
        else
        {
            ZB_MEMCPY(ptr, g_oob_key, ZB_CCM_KEY_SIZE);
            ptr += ZB_CCM_KEY_SIZE;
        }
        zb_put_next_htole32(ptr, frame_cnt);
        ptr += sizeof(frame_cnt);
    }

    TRACE_MSG(TRACE_APP1, "<send_commissioning", (FMT__0));
}


/* Specific */
static void construct_aes_nonce(
    zb_bool_t    from_gpd,
    zb_zgpd_id_t *zgpd_id,
    zb_uint32_t  frame_counter,
    zb_zgp_aes_nonce_t *res_nonce)
{
    TRACE_MSG(TRACE_SECUR2, ">> construct_aes_nonce, from_gpd %hd, zgpd_id %p, frame_counter %d, "
              "res_nonce %p", (FMT__H_P_D, from_gpd, zgpd_id, frame_counter, res_nonce));

    ZB_BZERO(res_nonce, sizeof(zb_zgp_aes_nonce_t));

    if (zgpd_id->app_id == ZB_ZGP_APP_ID_0000)
    {
        zb_uint32_t src_id_le;

        ZB_HTOLE32(&src_id_le, &zgpd_id->addr.src_id);

        if (from_gpd)
        {
            res_nonce->src_addr.splitted_addr[0] = src_id_le;
        }
        else
        {
            res_nonce->src_addr.splitted_addr[0] = 0;
        }

        res_nonce->src_addr.splitted_addr[1] = src_id_le;
    }
    else
    {
        /* zgpd_id->addr.ieee_addr is stored as LE, so res_nonce->src_addr will be LE also */
        ZB_64BIT_ADDR_COPY(&res_nonce->src_addr.ieee_addr, &zgpd_id->addr.ieee_addr);
    }

    ZB_HTOLE32(&res_nonce->frame_counter, &frame_counter);

    if ((zgpd_id->app_id == ZB_ZGP_APP_ID_0010) && (!from_gpd))
    {
        res_nonce->security_control = 0xA3;
    }
    else
    {
        res_nonce->security_control = 0x05;
    }

    TRACE_MSG(TRACE_SECUR2, "<< construct_aes_nonce", (FMT__0));
}

/*============================================================================*/
/*                          STARTUP                                           */
/*============================================================================*/

static void zgpc_custom_startup()
{
    /* Init device, load IB values from nvram or set it to default */
    ZB_SET_TRAF_DUMP_ON();
    ZB_INIT("th_gpp");

    /* let's always be coordinator */
    ZB_AIB().aps_designated_coordinator = 1;
    ZB_AIB().aps_channel_mask = (1 << TEST_CHANNEL);
    zb_set_default_ffd_descriptor_values(ZB_COORDINATOR);
    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_th_tool_addr);
    ZB_PIBCACHE_PAN_ID() = TEST_PAN_ID;
    ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);
    zb_secur_setup_nwk_key(g_key_nwk, 0);
    ZB_NIB_SET_USE_MULTICAST(ZB_FALSE);
    /* Must use NVRAM for ZGP */
    ZB_AIB().aps_use_nvram = 1;

    /* Need to block GPDF recv directly */
#ifdef ZB_ZGP_SKIP_GPDF_ON_NWK_LAYER
    ZG->nwk.skip_gpdf = 0;
#endif
#ifdef ZB_ZGP_RUNTIME_WORK_MODE_WITH_PROXIES
    ZGP_CTX().enable_work_with_proxies = 1;
#endif
#ifdef ZB_CERTIFICATION_HACKS
    ZB_CERT_HACKS().ccm_check_cb = NULL;
#endif

    ZGP_GPS_SECURITY_LEVEL = ZB_ZGP_FILL_GPS_SECURITY_LEVEL(
                                 ZB_ZGP_SEC_LEVEL_FULL_NO_ENC,
                                 ZB_ZGP_DEFAULT_SEC_LEVEL_PROTECTION_WITH_GP_LINK_KEY,
                                 ZB_ZGP_DEFAULT_SEC_LEVEL_INVOLVE_TC);

    ZGP_GP_SET_SHARED_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_NO_KEY);
    ZGP_CTX().device_role = ZGP_DEVICE_COMMISSIONING_TOOL;

    zb_zdo_register_device_annce_cb(dev_annce_cb);
    TEST_DEVICE_CTX.gp_prx_comm_mode_hndlr_cb = gp_proxy_comm_mode_handler_cb;
}

ZB_ZGPC_DECLARE_STARTUP_PROCESS()

#else // defined ZB_ENABLE_HA

#include <stdio.h>
MAIN()
{
    ARGV_UNUSED;

    printf("HA profile should be enabled in zb_config.h\n");

    MAIN_RETURN(1);
}

#endif
