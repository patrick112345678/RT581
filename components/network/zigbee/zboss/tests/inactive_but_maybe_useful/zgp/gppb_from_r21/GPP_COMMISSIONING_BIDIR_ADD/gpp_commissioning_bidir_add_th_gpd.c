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

#define ZB_TEST_NAME GPP_COMMISSIONING_BIDIR_ADD_TH_GPD
#define ZB_TRACE_FILE_ID 41458
#include "zb_common.h"
#include "zb_mac.h"
#include "zb_mac_globals.h"
#include "zgpd/zb_zgpd.h"
#include "test_config.h"
#include "../include/zgp_test_templates.h"

#ifdef ZB_CERTIFICATION_HACKS

/*============================================================================*/
/*                             DECLARATIONS                                   */
/*============================================================================*/

static zb_uint32_t  g_zgpd_srcId = TEST_ZGPD_SRC_ID;
static zb_uint8_t g_oob_key[16] = TEST_OOB_KEY;
static zb_uint8_t g_oper_channel;

static zb_ieee_addr_t g_zgpd_addr = TH_GPD_IEEE_ADDR;

static void th_gpd_comm_cb(zb_uint8_t param);
static zb_uint8_t get_next_channel();

static void send_channel_cfg(zb_uint8_t param);
static void send_commissioning(zb_uint8_t param, zb_uint16_t options);
static void send_channel_req_delayed(zb_uint8_t param);

static void send_commissioning_9a(zb_uint8_t param);
static void send_commissioning_9b(zb_uint8_t param);
/* Additional functions */
#ifndef TH_SKIP_STEP_4
static void gpd_set_channel_req(zb_uint8_t param);
static void gpd_set_channel_conf(zb_uint8_t param);
#endif

#define COMM_OPT_AUTO_COMM        (0x0001)
#define COMM_OPT_RX_AFTER_TX_COMM (0x0002)
#define COMM_OPT_APP_ID_SHIFT     (2)

/*============================================================================*/
/*                             FSM CORE                                       */
/*============================================================================*/

/*! Program states according to test scenario */
enum test_states_e
{
    TEST_STATE_INITIATE,
    /* STEPS 1 and 2 */
    TEST_STATE_START_1A,
#ifndef TH_SKIP_STEPS_1_2
    TEST_STATE_SEND_CHANNEL_REQ_1A,
    TEST_STATE_SEND_CHANNEL_REQ_1B,
    TEST_STATE_SEND_CHANNEL_REQ_1C,
    TEST_STATE_START_COMMISSIONING_1D,
    TEST_STATE_COMM_COMPLETED_2D,
    TEST_STATE_SEND_SUCCESS_2E,
#endif
    /* STEP 3 */
#ifndef TH_SKIP_STEP_3
    TEST_STATE_START_COMMISSIONING_3,
    TEST_STATE_SEND_CHANNEL_REQ1_3,
    TEST_STATE_SEND_CHANNEL_REQ2_3,
    TEST_STATE_SEND_CHANNEL_REQ3_3,
#endif
    /* STEP 4 */
#ifndef TH_SKIP_STEP_4
    TEST_STATE_START_COMMISSIONING_4,
    TEST_STATE_SWITCH_TO_TRANSMIT_CHANNEL_4,
    TEST_STATE_SEND_CHANNEL_CONFIG_4,
    TEST_STATE_SEND_COMMISSIONING1_4,
    TEST_STATE_SEND_COMMISSIONING2_4,
    TEST_STATE_SEND_SUCCESS_4,
#endif
    /* STEP 5 */
#ifndef TH_SKIP_STEP_5
    TEST_STATE_START_COMMISSIONING_5,
    TEST_STATE_SEND_COMMISSIONING1_5,
    TEST_STATE_SEND_COMMISSIONING2_5,
    TEST_STATE_SEND_SUCCESS_5,
#endif
    /* STEP 7A */
#ifndef TH_SKIP_STEP_7
    TEST_STATE_SEND_CHANNEL_REQ_7A,
    /* STEP 7B */
    TEST_STATE_SEND_CHANNEL_REQ_7B,
    /* STEP 7C */
    TEST_STATE_SEND_COMMISSIONING_7C,
#endif
    /* STEP 8A */
#ifndef TH_SKIP_STEP_8A
    TEST_STATE_SEND_CHANNEL_REQ_8A,
#endif
    /* STEP 8B */
#ifndef TH_SKIP_STEP_8B
    TEST_STATE_SEND_CHANNEL_REQ_8B,
#endif
    /* STEP 8C */
#ifndef TH_SKIP_STEP_8C
    TEST_STATE_SEND_CHANNEL_REQ_8C,
#endif
    /* STEP 9A */
#ifndef TH_SKIP_STEP_9A
    TEST_STATE_SEND_COMMISSIONING_9A,
#endif
    /* STEP 9B */
    TEST_STATE_SEND_COMMISSIONING_9B,
    /* Finish */
    TEST_STATE_FINISHED
};

ZB_ZGPD_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 1000)

ZB_ZGPD_DECLARE_COMMISSIONING_CALLBACK()

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
    ZB_ZGPD_CHACK_RESET_ALL();
    ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_AUTO_COMM);
    ZGPD->ch_replace_autocomm = 0;

    TRACE_MSG(TRACE_APP1,
              "TH-GPD: perform_next_state: state = %d",
              (FMT__D, TEST_DEVICE_CTX.test_state));

    switch (TEST_DEVICE_CTX.test_state)
    {
    /* STEPS 1 and 2 */
    case TEST_STATE_START_1A:
    {
        g_oper_channel = ZGPD->channel;
        ZB_ZGPD_SET_PAUSE(1);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
    }
    break;
#ifndef TH_SKIP_STEPS_1_2
    case TEST_STATE_SEND_CHANNEL_REQ_1A:
    {
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        zb_zgpd_start_commissioning(comm_cb);
        ZB_ZGPD_SET_PAUSE(1);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
    }
    break;
    case TEST_STATE_SEND_CHANNEL_REQ_1B:
    {
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_TRANSMIT_CH);
        ZGPD->ch_replace_cr_transmit_ch = get_next_channel();
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_AUTO_COMM);
        ZGPD->ch_replace_autocomm = 1;
        zb_zgpd_start_commissioning(comm_cb);

        ZB_ZGPD_SET_PAUSE(1);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
    }
    break;
    case TEST_STATE_SEND_CHANNEL_REQ_1C:
    {
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_TRANSMIT_CH);
        ZGPD->ch_replace_cr_transmit_ch = get_next_channel();
        zb_zgpd_start_commissioning(comm_cb);

        ZB_ZGPD_SET_PAUSE(1);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
    }
    case TEST_STATE_START_COMMISSIONING_1D:
    {
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_SKIP_FIRST_COMM_REPLY);
        ZGPD->ch_skip_first_n_comm_reply = TH_GPD_SKIP_N_COMM_REPLYES_IN_2C;
        zb_zgpd_start_commissioning(th_gpd_comm_cb);
        break;
    }
    case TEST_STATE_COMM_COMPLETED_2D:
    {
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        ZB_ZGPD_SET_PAUSE(5);
        break;
    }
#endif

#ifndef TH_SKIP_STEP_3
    case TEST_STATE_START_COMMISSIONING_3:
    {
        zb_uint8_t replace_ch = ZGPD->channel - ZB_ZGPD_FIRST_CH;

        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH);
        ZGPD->ch_replace_cr_next_ch = replace_ch | (replace_ch << 4);
        zb_zgpd_start_commissioning(comm_cb);
        ZB_ZGPD_SET_PAUSE(3);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
    }
    case TEST_STATE_SEND_CHANNEL_REQ1_3:
    {
        /* Frame Type = DATA */
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_FRAME_TYPE);
        ZGPD->ch_replace_frame_type = ZGP_FRAME_TYPE_DATA;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_SRCID);
        ZGPD->ch_replace_src_id = 0x00000000;
        zb_zgpd_start_commissioning(comm_cb);
        ZB_ZGPD_SET_PAUSE(1);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
    }
    case TEST_STATE_SEND_CHANNEL_REQ2_3:
    {
        /* Protocol Version = 0x02 */
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_PROTO_VERSION);
        ZGPD->ch_replace_proto_version = 0x2;
        zb_zgpd_start_commissioning(comm_cb);

        ZB_ZGPD_SET_PAUSE(1);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
    }
    case TEST_STATE_SEND_CHANNEL_REQ3_3:
    {
        /* NWK Frame Control Extension = 1 */
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_EXTNWK_FC_FLAG);
        ZGPD->ch_replace_extnwk_flag = 1;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_INSERT_EXTNWK_FC_DATA);
        ZGPD->ch_insert_extnwk_data = 1;
        zb_zgpd_start_commissioning(comm_cb);

        ZB_ZGPD_SET_PAUSE(TH_GPD_PAUSE_BEFORE_STEP_4);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
    }
#endif

#ifndef TH_SKIP_STEP_4
    case TEST_STATE_START_COMMISSIONING_4:
    {
        g_oper_channel = ZGPD->channel;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        zb_zgpd_start_commissioning(comm_cb);
        ZB_ZGPD_SET_PAUSE(2);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
    }
    case TEST_STATE_SWITCH_TO_TRANSMIT_CHANNEL_4:
    {
        ZGPD->channel = get_next_channel();
        ZB_GET_OUT_BUF_DELAYED(gpd_set_channel_req);
        break;
    }
    case TEST_STATE_SEND_CHANNEL_CONFIG_4:
    {
        ZB_GET_OUT_BUF_DELAYED(send_channel_cfg);
        ZB_ZGPD_SET_PAUSE(1);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
    }
    case TEST_STATE_SEND_COMMISSIONING1_4:
    {
        ZB_GET_OUT_BUF_DELAYED2(send_commissioning, COMM_OPT_RX_AFTER_TX_COMM);
        ZB_ZGPD_SET_PAUSE(1);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
    }
    case TEST_STATE_SEND_COMMISSIONING2_4:
    {
        ZB_GET_OUT_BUF_DELAYED2(send_commissioning, 0);
        ZB_ZGPD_SET_PAUSE(1);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
    }
#endif

#ifndef TH_SKIP_STEP_5
    case TEST_STATE_START_COMMISSIONING_5:
    {
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_STOP_AFTER_1_COMMREQ);
        zb_zgpd_start_commissioning(comm_cb);
        ZB_ZGPD_SET_PAUSE(2);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
    }
    case TEST_STATE_SEND_COMMISSIONING1_5:
    {
        ZB_GET_OUT_BUF_DELAYED2(send_commissioning,
                                COMM_OPT_RX_AFTER_TX_COMM | COMM_OPT_AUTO_COMM);
        ZB_ZGPD_SET_PAUSE(1);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
    }
    case TEST_STATE_SEND_COMMISSIONING2_5:
    {
        ZB_GET_OUT_BUF_DELAYED2(send_commissioning, COMM_OPT_AUTO_COMM);
        ZB_ZGPD_SET_PAUSE(1);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
    }
#endif

#ifndef TH_SKIP_STEP_7
    case TEST_STATE_SEND_CHANNEL_REQ_7A:
    {
        zb_uint8_t replace_ch;

        replace_ch = g_oper_channel = ZGPD->channel - ZB_ZGPD_FIRST_CH;

        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_AUTO_COMM);
        ZGPD->ch_replace_autocomm = 0;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH);
        ZGPD->ch_replace_cr_next_ch = replace_ch | (replace_ch << 4);
        zb_zgpd_start_commissioning(comm_cb);

        ZB_ZGPD_SET_PAUSE(TH_GPD_PAUSE_BEFORE_CH_REQ_7B);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
    }
    case TEST_STATE_SEND_CHANNEL_REQ_7B:
    {
        zb_uint8_t replace_ch = g_oper_channel;

        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_AUTO_COMM);
        ZGPD->ch_replace_autocomm = 0;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH);
        ZGPD->ch_replace_cr_next_ch = replace_ch | (replace_ch << 4);
        zb_zgpd_start_commissioning(comm_cb);

        ZB_ZGPD_SET_PAUSE(TH_GPD_PAUSE_BEFORE_COMM_7C);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
    }
    case TEST_STATE_SEND_COMMISSIONING_7C:
    {
        ZB_GET_OUT_BUF_DELAYED2(send_commissioning, COMM_OPT_RX_AFTER_TX_COMM);
        ZB_ZGPD_SET_PAUSE(TH_GPD_PAUSE_BEFORE_STEP_8A);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
    }
#endif

#ifndef TH_SKIP_STEP_8A
    case TEST_STATE_SEND_CHANNEL_REQ_8A:
    {
        ZB_SCHEDULE_ALARM(send_channel_req_delayed, 0,
                          (zb_time_t) (4.8 * ZB_TIME_ONE_SECOND));
        ZB_ZGPD_SET_PAUSE(TH_GPD_PAUSE_BEFORE_STEP_8B);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
    }
#endif

#ifndef TH_SKIP_STEP_8B
    case TEST_STATE_SEND_CHANNEL_REQ_8B:
    {
        ZB_SCHEDULE_ALARM(send_channel_req_delayed, 0,
                          (zb_time_t) ((5.1) * ZB_TIME_ONE_SECOND));
        ZB_ZGPD_SET_PAUSE(TH_GPD_PAUSE_BEFORE_STEP_8C);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
    }
#endif

#ifndef TH_SKIP_STEP_8C
    case TEST_STATE_SEND_CHANNEL_REQ_8C:
    {
        ZB_SCHEDULE_ALARM(send_channel_req_delayed, 0,
                          (zb_time_t) ((3.1) * ZB_TIME_ONE_SECOND));
        ZB_ZGPD_SET_PAUSE(TH_GPD_PAUSE_BEFORE_STEP_9A);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
    }
#endif

#ifndef TH_SKIP_STEP_9A
    case TEST_STATE_SEND_COMMISSIONING_9A:
    {
        ZB_SCHEDULE_ALARM(send_commissioning_9a, 0,
                          TH_GPD_SEND_COMM_DELAY_STEP_9 * ZB_TIME_ONE_SECOND);
        ZB_ZGPD_SET_PAUSE(TH_GPD_PAUSE_BEFORE_STEP_9B);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
    }
#endif
    /* STEP 9B */
    case TEST_STATE_SEND_COMMISSIONING_9B:
    {
        ZB_SCHEDULE_ALARM(send_commissioning_9b, 0,
                          TH_GPD_SEND_COMM_DELAY_STEP_9 * ZB_TIME_ONE_SECOND);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
    }

    case TEST_STATE_FINISHED:
        TRACE_MSG(TRACE_APP1, "TH-GPD: Test finished. Status: OK", (FMT__0));
        break;
    default:
        ZB_SCHEDULE_ALARM(test_send_command, 0, ZB_TIME_ONE_SECOND);
    }

    if (param)
    {
        zb_free_buf(ZB_BUF_FROM_REF(param));
    }
}

/*============================================================================*/
/*                            TEST IMPLEMENTATION                             */
/*============================================================================*/

static void th_gpd_comm_cb(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);

    TRACE_MSG(TRACE_APP1,
              "TH-GPD: commissioning stopped, state = %d",
              (FMT__D, TEST_DEVICE_CTX.test_state));

#ifndef TH_SKIP_STEPS_1_2
    if (TEST_DEVICE_CTX.test_state == TEST_STATE_START_COMMISSIONING_1D)
    {
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
    }
#endif

    zb_free_buf(buf);
    TRACE_BUFFERS_LIST();
}

static zb_uint8_t get_next_channel()
{
    zb_uint8_t offset = ((g_oper_channel - ZB_ZGPD_FIRST_CH) / ZB_ZGPD_CH_SERIES + 2) * ZB_ZGPD_CH_SERIES - 1;
    return g_oper_channel + offset;
}

static void make_gpdf(zb_buf_t *buf, zb_uint8_t **ptr)
{
    ZVUNUSED(buf);
    switch (TEST_DEVICE_CTX.test_state)
    {
#ifndef TH_SKIP_STEPS_1_2
    case TEST_STATE_SEND_SUCCESS_2E:
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_SUCCESS);
        ZB_ZGPD_SET_PAUSE(TH_GPD_PAUSE_BEFORE_STEP_3);
        break;
#endif
#ifndef TH_SKIP_STEP_4
    case TEST_STATE_SEND_SUCCESS_4:
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_SUCCESS);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_AUTO_COMM);
        ZGPD->ch_replace_autocomm = 0;
        ZB_ZGPD_SET_PAUSE(TH_GPD_PAUSE_BEFORE_STEP_5);
        break;
#endif
#ifndef TH_SKIP_STEP_5
    case TEST_STATE_SEND_SUCCESS_5:
        ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_SUCCESS);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_AUTO_COMM);
        ZGPD->ch_replace_autocomm = 1;
        ZB_ZGPD_SET_PAUSE(TH_GPD_PAUSE_BEFORE_CH_REQ_7A);
        break;
#endif

    default:
        break;
    };
}

static void send_channel_cfg(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_uint8_t *ptr;

    TRACE_MSG(TRACE_APP1,
              "TH-GPD: send_channel_cfg: state = %d",
              (FMT__D, TEST_DEVICE_CTX.test_state));

    ptr = ZB_START_GPDF_PACKET(buf);
    ZGPD->tx_cmd = ZB_GPDF_CMD_CHANNEL_CONFIGURATION;
    ZB_GPDF_PUT_UINT8(ptr, ZB_GPDF_CMD_CHANNEL_CONFIGURATION);
    ZB_GPDF_PUT_UINT8(ptr, g_oper_channel - ZB_ZGPD_FIRST_CH);
    ZB_FINISH_GPDF_PACKET(buf, ptr);

    ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_AUTO_COMM);
    ZGPD->ch_replace_autocomm = 0;

    ZB_SEND_MAINTENANCE_GPDF(param);
}

static void send_commissioning(zb_uint8_t param, zb_uint16_t options)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_uint8_t *ptr;
    zb_uint16_t app_id = (options >> COMM_OPT_APP_ID_SHIFT) & 0x000F;

    TRACE_MSG(TRACE_APP1,
              "TH-GPD: send_commissioning: state = %d, options = 0x%x",
              (FMT__D_D, TEST_DEVICE_CTX.test_state, options));

    ptr = ZB_START_GPDF_PACKET(buf);
    ZGPD->tx_cmd = ZB_GPDF_CMD_COMMISSIONING;
    ZB_GPDF_PUT_UINT8(ptr, ZB_GPDF_CMD_COMMISSIONING);

    ZB_GPDF_PUT_UINT8(ptr, ZB_ZGP_ON_OFF_SWITCH_DEV_ID);
    /* Only sequence number capcabilities */
    ZB_GPDF_PUT_UINT8(ptr, ZB_GPDF_COMM_OPT_FLD(1, 0, 0, 0, 0, 0, 0));

    ZB_FINISH_GPDF_PACKET(buf, ptr);

    ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_RXAFTERTX);
    ZGPD->ch_replace_rxaftertx = !!(options & COMM_OPT_RX_AFTER_TX_COMM);
    ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_AUTO_COMM);
    ZGPD->ch_replace_autocomm = (options & COMM_OPT_AUTO_COMM);
    ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_SEC_LEVEL);
    ZGPD->ch_replace_sec_level = ZB_ZGP_SEC_LEVEL_NO_SECURITY;

    TRACE_MSG(TRACE_APP1, "TH-GPD: rx_after_tx = %d", (FMT__D, ZGPD->ch_replace_rxaftertx));

    switch (app_id)
    {
    case ZB_ZGP_APP_ID_0010:
    {
        ZB_ZGPD_INIT_ZGPD_CTX(ZB_ZGP_APP_ID_0010, ZB_ZGPD_COMMISSIONING_BIDIR, ZB_ZGP_ON_OFF_SWITCH_DEV_ID);
        ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_zgpd_addr);
        ZB_IEEE_ADDR_COPY(&g_zgpd_ctx.id.addr.ieee_addr, &g_zgpd_addr);
        g_zgpd_ctx.id.endpoint = TEST_ZGPD_EP_X;
    }
    break;
    }

    zb_zgpd_send_data_req(param, ZB_FALSE);
}

static void send_channel_req_delayed(zb_uint8_t param)
{
    ZVUNUSED(param);

    ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
    ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_AUTO_COMM);
    ZGPD->ch_replace_autocomm = 0;
    ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_START_CH);

    switch (TEST_DEVICE_CTX.test_state)
    {
#ifndef TH_SKIP_STEP_8A
    case TEST_STATE_SEND_CHANNEL_REQ_8A:
    {
        ZGPD->ch_replace_cr_start_ch = TH_GPS_TMP_MASTER_TX_CHANNEL_X;
        break;
    }
#endif
#ifndef TH_SKIP_STEP_8B
    case TEST_STATE_SEND_CHANNEL_REQ_8B:
    {
        ZGPD->ch_replace_cr_start_ch = TH_GPS_TMP_MASTER_TX_CHANNEL_X;
        break;
    }
#endif
#ifndef TH_SKIP_STEP_8C
    case TEST_STATE_SEND_CHANNEL_REQ_8C:
    {
        zb_uint8_t replace_ch = g_oper_channel = ZGPD->channel - ZB_ZGPD_FIRST_CH;

        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH);
        ZGPD->ch_replace_cr_next_ch = replace_ch | (replace_ch << 4);
        ZGPD->ch_replace_cr_start_ch = ZGPD->channel;
        break;
    }
#endif
    default:
    {
        ;
    }
    }

    zb_zgpd_start_commissioning(comm_cb);
}

static void send_commissioning_9a(zb_uint8_t param)
{
    ZVUNUSED(param);
    ZB_GET_OUT_BUF_DELAYED2(send_commissioning,
                            COMM_OPT_RX_AFTER_TX_COMM | (ZB_ZGP_APP_ID_0000 << COMM_OPT_APP_ID_SHIFT));
}

static void send_commissioning_9b(zb_uint8_t param)
{
    ZVUNUSED(param);
    ZB_GET_OUT_BUF_DELAYED2(send_commissioning,
                            COMM_OPT_RX_AFTER_TX_COMM | (ZB_ZGP_APP_ID_0010 << COMM_OPT_APP_ID_SHIFT));
}


/* Additional functions */
#ifndef TH_SKIP_STEP_4
static void gpd_set_channel_req(zb_uint8_t param)
{
    zb_mlme_set_request_t *req;

    TRACE_MSG(TRACE_APP1,
              "TH-GPD: gpd_set_channel_req: new_ch = %hd",
              (FMT__H, ZGPD->channel));

    ZB_BUF_INITIAL_ALLOC(ZB_BUF_FROM_REF(param), sizeof(*req), req);
    *((zb_uint8_t *)(req + 1)) = ZGPD->channel;
    req->confirm_cb = gpd_set_channel_conf;
    ZB_SCHEDULE_CALLBACK(zb_mlme_set_request, param);
}

static void gpd_set_channel_conf(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, "TH-GPD: gpd_set_channel_conf", (FMT__0));
    zb_free_buf(ZB_BUF_FROM_REF(param));

    if (TEST_DEVICE_CTX.test_state == TEST_STATE_SWITCH_TO_TRANSMIT_CHANNEL_4)
    {
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
    }
}
#endif

/*============================================================================*/
/*                          STARTUP                                           */
/*============================================================================*/

static void zgp_custom_startup()
{
    /* Init device, load IB values from nvram or set it to default */
    ZB_SET_TRAF_DUMP_ON();
    ZB_INIT("th_gpd");

    ZB_ZGPD_INIT_ZGPD_CTX(ZB_ZGP_APP_ID_0000, ZB_ZGPD_COMMISSIONING_BIDIR, ZB_ZGP_ON_OFF_SWITCH_DEV_ID);

    ZB_ZGPD_SET_SRC_ID(g_zgpd_srcId);
    ZB_ZGPD_SET_SECURITY_LEVEL(ZB_ZGP_SEC_LEVEL_FULL_NO_ENC);
    ZB_ZGPD_SET_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL);
    ZB_ZGPD_SET_OOB_KEY(g_oob_key);
    ZB_ZGPD_REQUEST_SECURITY_KEY();

    ZGPD->channel = TEST_CHANNEL;
}

#endif /* ZB_CERTIFICATION_HACKS */

ZB_ZGPD_TH_DECLARE_STARTUP_PROCESS()
