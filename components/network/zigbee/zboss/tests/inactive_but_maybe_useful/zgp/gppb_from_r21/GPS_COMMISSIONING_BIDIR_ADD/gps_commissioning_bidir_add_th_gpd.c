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

#define ZB_TEST_NAME GPS_COMMISSIONING_BIDIR_ADD_TH_GPD
#define ZB_TRACE_FILE_ID 41495
#include "zb_common.h"
#include "zb_mac.h"
#include "zb_mac_globals.h"
#include "zgpd/zb_zgpd.h"
#include "test_config.h"
#include "../include/zgp_test_templates.h"

#ifdef ZB_CERTIFICATION_HACKS

static zb_uint8_t g_bad_key[16] = { 0xba, 0xd0, 0xba, 0xd0, 0xba, 0xd0, 0xba, 0xd0, 0xba, 0xd0, 0xba, 0xd0, 0xba, 0xd0, 0xba, 0xd0};

static zb_uint32_t  g_zgpd_srcId = TEST_ZGPD_SRC_ID;

static zb_uint8_t g_key_nwk[] = TEST_NWK_KEY;
static zb_uint8_t g_oob_key[16] = TEST_OOB_KEY;

static zb_ieee_addr_t g_zgpd_addr = TH_GPD_IEEE_ADDR;
static zb_ieee_addr_t g_zgpd_addr_bad = TH_GPD_BAD_IEEE_ADDR;

/*! Program states according to test scenario */
enum test_states_e
{
    TEST_STATE_INITIATE,
    TEST_STATE_COMMISSIONING1A,
    TEST_STATE_COMMISSIONING1B1,
    TEST_STATE_COMMISSIONING1B2,
    TEST_STATE_DECOMMISSIONING1,
    TEST_STATE_COMMISSIONING1C1,
    TEST_STATE_COMMISSIONING1C2,
    TEST_STATE_DECOMMISSIONING2,
    TEST_STATE_COMMISSIONING1D1,
    TEST_STATE_COMMISSIONING1D2,
    TEST_STATE_COMMISSIONING1D3,
    TEST_STATE_COMMISSIONING1D4,
    TEST_STATE_COMMISSIONING2A,
    TEST_STATE_COMMISSIONING2B,
    TEST_STATE_DECOMMISSIONING2C,
    TEST_STATE_COMMISSIONING31,
    TEST_STATE_COMMISSIONING32,
    TEST_STATE_COMMISSIONING33,
    TEST_STATE_COMMISSIONING34,
    TEST_STATE_COMMISSIONING41,
    TEST_STATE_COMMISSIONING42,
    TEST_STATE_COMMISSIONING43,
    TEST_STATE_COMMISSIONING44,
    TEST_STATE_COMMISSIONING45,
    TEST_STATE_COMMISSIONING46,
    TEST_STATE_COMMISSIONING51,
    TEST_STATE_COMMISSIONING52,
    TEST_STATE_COMMISSIONING53,
    TEST_STATE_COMMISSIONING54,
    TEST_STATE_COMMISSIONING55,
    TEST_STATE_DECOMMISSIONING5,
    TEST_STATE_COMMISSIONING6,
    TEST_STATE_COMMISSIONING6B,
    TEST_STATE_COMMISSIONING6C,
    TEST_STATE_COMMISSIONING6D,
    TEST_STATE_COMMISSIONING6E,
    TEST_STATE_COMMISSIONING6F,
    TEST_STATE_COMMISSIONING6G,
    TEST_STATE_COMMISSIONING6H,
    TEST_STATE_COMMISSIONING6I,
    TEST_STATE_COMMISSIONING6J,
    //  TEST_STATE_DEC1,
    //  TEST_STATE_DEC2,
    //  TEST_STATE_DEC3,
    TEST_STATE_COMMISSIONING7A,
    TEST_STATE_COMMISSIONING7A2,
    TEST_STATE_COMMISSIONING7B,
    TEST_STATE_COMMISSIONING7B2,
    TEST_STATE_COMMISSIONING7C,
    TEST_STATE_COMMISSIONING7C2,
    TEST_STATE_COMMISSIONING7D,
    TEST_STATE_COMMISSIONING7D2,
    TEST_STATE_DECOMMISSIONING7,
    TEST_STATE_COMMISSIONING8A1,
    TEST_STATE_COMMISSIONING8A2,
    TEST_STATE_COMMISSIONING8B1,
    TEST_STATE_COMMISSIONING8B2,
    TEST_STATE_COMMISSIONING8C1,
    TEST_STATE_COMMISSIONING8C2,
    TEST_STATE_COMMISSIONING9A,
    TEST_STATE_COMMISSIONING10A,
    TEST_STATE_COMMISSIONING10A1,
    TEST_STATE_COMMISSIONING10B,
    TEST_STATE_COMMISSIONING10B1,

    TEST_STATE_FINISHED
};

static void perform_next_state(zb_uint8_t param);

static void my_comm_cb(zb_uint8_t param)
{
    zb_buf_t   *buf = ZB_BUF_FROM_REF(param);
    //  zb_uint8_t  comm_result = buf->u.hdr.status;

    zb_free_buf(buf);
    TRACE_BUFFERS_LIST();
    TRACE_MSG(TRACE_APP1, "commissioning stopped", (FMT__0));
}

static void decommission_delayed(zb_uint8_t param)
{
    ZVUNUSED(param);
    zb_zgpd_decommission();
    //    ZB_SCHEDULE_ALARM(perform_next_state, 0, 2*ZB_TIME_ONE_SECOND);
}

ZB_ZGPD_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 1000)

ZB_ZGPD_DECLARE_COMMISSIONING_CALLBACK()

static void perform_next_state(zb_uint8_t param)
{
    //  zb_uint32_t sfc;
    ZVUNUSED(param);
    TEST_DEVICE_CTX.test_state++;
    //  if(TEST_DEVICE_CTX.test_state==TEST_STATE_COMMISSIONING1A)  TEST_DEVICE_CTX.test_state=TEST_STATE_COMMISSIONING41;
    //  if(TEST_DEVICE_CTX.test_state==TEST_STATE_COMMISSIONING1A)  TEST_DEVICE_CTX.test_state=TEST_STATE_COMMISSIONING9A;

    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_DECOMMISSIONING1:
        ZB_SCHEDULE_ALARM(decommission_delayed, 0, ZB_TIME_ONE_SECOND);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, 3 * ZB_TIME_ONE_SECOND);
        ZB_ZGPD_SET_PAUSE(2);
        break;
    case TEST_STATE_DECOMMISSIONING2:
        //      ZB_SCHEDULE_ALARM(decommission_delayed, 0, ZB_TIME_ONE_SECOND);
        zb_zgpd_decommission();
        ZB_SCHEDULE_ALARM(perform_next_state, 0, ZB_TIME_ONE_SECOND * 3);
        break;
    case TEST_STATE_DECOMMISSIONING2C:
        ZB_ZGPD_CHACK_RESET_ALL();
        TRACE_MSG(TRACE_APP1, "state:D2C", (FMT__0));
        ZGPD->channel = TEST_CHANNEL;
        ZGPD->commissioning_method = ZB_ZGPD_COMMISSIONING_BIDIR;
        ZGPD->auto_commissioning_pending = 0;
        //      ZB_SCHEDULE_ALARM(decommission_delayed, 0, ZB_TIME_ONE_SECOND);
        zb_zgpd_decommission();
        ZB_SCHEDULE_ALARM(perform_next_state, 0, ZB_TIME_ONE_SECOND * 3);
        break;
    case TEST_STATE_DECOMMISSIONING5:
        ZB_ZGPD_CHACK_RESET_ALL();
        TRACE_MSG(TRACE_APP1, "state:D5", (FMT__0));
        ZGPD->channel = TEST_CHANNEL;
        ZGPD->commissioning_method = ZB_ZGPD_COMMISSIONING_BIDIR;
        ZGPD->auto_commissioning_pending = 0;
        //      ZB_SCHEDULE_ALARM(decommission_delayed, 0, ZB_TIME_ONE_SECOND);
        zb_zgpd_decommission();
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 2.5));
        break;

    case TEST_STATE_DECOMMISSIONING7:
        ZB_ZGPD_CHACK_RESET_ALL();
        TRACE_MSG(TRACE_APP1, "state:D7", (FMT__0));
        ZGPD->channel = TEST_CHANNEL;
        ZGPD->commissioning_method = ZB_ZGPD_COMMISSIONING_BIDIR;
        ZGPD->auto_commissioning_pending = 0;
        zb_zgpd_decommission();
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 1.5));
        break;

    case TEST_STATE_COMMISSIONING10A:
        TRACE_MSG(TRACE_APP1, "state:10A", (FMT__0));
        ZB_ZGPD_CHACK_RESET_ALL();
        ZB_ZGPD_INIT_ZGPD_CTX(ZB_ZGP_APP_ID_0000, ZB_ZGPD_COMMISSIONING_BIDIR, ZB_ZGP_ON_OFF_SWITCH_DEV_ID);
        //    ZB_ZGPD_SET_SRC_ID(g_zgpd_srcId);
        ZB_ZGPD_SET_SRC_ID(0);

        ZB_ZGPD_SET_SECURITY_LEVEL(ZB_ZGP_SEC_LEVEL_FULL_NO_ENC);
        ZB_ZGPD_SET_SECURITY_KEY_TYPE(TEST_KEY_TYPE);
        ZB_ZGPD_SET_SECURITY_KEY(g_key_nwk);
        ZB_ZGPD_SET_OOB_KEY(g_oob_key);
        //      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_REQ_SERIES);
        //      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH);
        //      ZGPD->ch_replace_cr_next_ch = (TEST_CHANNEL+6-ZB_ZGPD_FIRST_CH)|((TEST_CHANNEL+6-ZB_ZGPD_FIRST_CH)<<4);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_STOP_AFTER_1_COMMREQ);
        zb_zgpd_start_commissioning(my_comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 3.1));
        break;
    case TEST_STATE_COMMISSIONING10A1:
        TRACE_MSG(TRACE_APP1, "state:10A1", (FMT__0));
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->commissioning_method = ZB_ZGPD_COMMISSIONING_UNIDIR;
        ZGPD->comm_state = ZB_ZGPD_STATE_NOT_COMMISSIONED;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_RXAFTERTX);
        ZGPD->ch_replace_rxaftertx = 1;
        ZGPD->rx_on = ZB_FALSE;
        ZGPD->toggle_channel = ZB_TRUE;
        zb_zgpd_start_commissioning(my_comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, ZB_TIME_ONE_SECOND * 3);
        break;

    case TEST_STATE_COMMISSIONING10B:
        TRACE_MSG(TRACE_APP1, "state:10B", (FMT__0));
        ZB_ZGPD_CHACK_RESET_ALL();
        ZB_ZGPD_INIT_ZGPD_CTX(ZB_ZGP_APP_ID_0010, ZB_ZGPD_COMMISSIONING_BIDIR, ZB_ZGP_ON_OFF_SWITCH_DEV_ID);
        ZGPD->comm_state = ZB_ZGPD_STATE_NOT_COMMISSIONED;
        //    ZB_ZGPD_SET_SRC_ID(g_zgpd_srcId);
        {
            zb_ieee_addr_t g_zgpd_addr0 = {0, 0, 0, 0, 0, 0, 0, 0};
            ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_zgpd_addr0);
            ZB_IEEE_ADDR_COPY(&g_zgpd_ctx.id.addr.ieee_addr, &g_zgpd_addr0);
        }
        g_zgpd_ctx.id.endpoint = 1;
        ZB_ZGPD_SET_SRC_ID(0);

        ZB_ZGPD_SET_SECURITY_LEVEL(ZB_ZGP_SEC_LEVEL_FULL_NO_ENC);
        ZB_ZGPD_SET_SECURITY_KEY_TYPE(TEST_KEY_TYPE);
        ZB_ZGPD_SET_SECURITY_KEY(g_key_nwk);
        ZB_ZGPD_SET_OOB_KEY(g_oob_key);
        //      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_REQ_SERIES);
        //      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH);
        //      ZGPD->ch_replace_cr_next_ch = (TEST_CHANNEL+6-ZB_ZGPD_FIRST_CH)|((TEST_CHANNEL+6-ZB_ZGPD_FIRST_CH)<<4);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_STOP_AFTER_1_COMMREQ);
        zb_zgpd_start_commissioning(my_comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 3.1));
        break;
    case TEST_STATE_COMMISSIONING10B1:
        TRACE_MSG(TRACE_APP1, "state:10B1", (FMT__0));
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->commissioning_method = ZB_ZGPD_COMMISSIONING_UNIDIR;
        ZGPD->comm_state = ZB_ZGPD_STATE_NOT_COMMISSIONED;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_RXAFTERTX);
        ZGPD->ch_replace_rxaftertx = 1;
        ZGPD->rx_on = ZB_FALSE;
        ZGPD->toggle_channel = ZB_TRUE;
        zb_zgpd_start_commissioning(my_comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, ZB_TIME_ONE_SECOND * 1);
        break;
    case TEST_STATE_COMMISSIONING9A:
        ZB_ZGPD_CHACK_RESET_ALL();
        TRACE_MSG(TRACE_APP1, "state:9A", (FMT__0));
        ZGPD->comm_state = ZB_ZGPD_STATE_NOT_COMMISSIONED;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_RXAFTERTX);
        ZGPD->ch_replace_rxaftertx = 1;
        ZGPD->rx_on = ZB_TRUE;
        ZGPD->security_level = 2;
        ZGPD->channel = TEST_CHANNEL;
        ZGPD->security_key_present = 0;
        ZGPD->gpd_security_key_request = 0;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_SKP);
        ZGPD->ch_replace_security_key_present = 0;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_INSERT_SK);
        ZGPD->ch_insert_security_key = 0;
        //      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_TRANSMIT_CH);
        //      ZGPD->ch_replace_cr_transmit_ch = TEST_CHANNEL;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_STOP_AFTER_1_COMMREQ);
        zb_zgpd_start_commissioning(my_comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 2.5));
        break;

    case TEST_STATE_COMMISSIONING8A1:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH);
        ZGPD->ch_replace_cr_next_ch = (TEST_CHANNEL + 6 - ZB_ZGPD_FIRST_CH) | ((TEST_CHANNEL + 6 - ZB_ZGPD_FIRST_CH) << 4);
        zb_zgpd_start_commissioning(my_comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 4.8));
        break;
    case TEST_STATE_COMMISSIONING8A2:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        ZGPD->comm_state = ZB_ZGPD_STATE_NOT_COMMISSIONED;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_TRANSMIT_CH);
        ZGPD->ch_replace_cr_transmit_ch = TEST_CHANNEL + 6;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH);
        ZGPD->ch_replace_cr_next_ch = (TEST_CHANNEL + 8 - ZB_ZGPD_FIRST_CH) | ((TEST_CHANNEL + 8 - ZB_ZGPD_FIRST_CH) << 4);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_AUTO_COMM);
        ZGPD->ch_replace_autocomm = 0;
        zb_zgpd_start_commissioning(my_comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 2.5));
        break;

    case TEST_STATE_COMMISSIONING8B1:
        ZB_ZGPD_CHACK_RESET_ALL();
        TRACE_MSG(TRACE_APP1, "state:8B1", (FMT__0));
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH);
        ZGPD->ch_replace_cr_next_ch = (TEST_CHANNEL + 6 - ZB_ZGPD_FIRST_CH) | ((TEST_CHANNEL + 6 - ZB_ZGPD_FIRST_CH) << 4);
        zb_zgpd_start_commissioning(my_comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 5.12));
        break;
    case TEST_STATE_COMMISSIONING8B2:
        ZB_ZGPD_CHACK_RESET_ALL();
        TRACE_MSG(TRACE_APP1, "state:8B2", (FMT__0));
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        ZGPD->comm_state = ZB_ZGPD_STATE_NOT_COMMISSIONED;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_TRANSMIT_CH);
        ZGPD->ch_replace_cr_transmit_ch = TEST_CHANNEL + 6;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH);
        ZGPD->ch_replace_cr_next_ch = (TEST_CHANNEL + 10 - ZB_ZGPD_FIRST_CH) | ((TEST_CHANNEL + 10 - ZB_ZGPD_FIRST_CH) << 4);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_AUTO_COMM);
        ZGPD->ch_replace_autocomm = 0;
        zb_zgpd_start_commissioning(my_comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, ZB_TIME_ONE_SECOND * 2);
        break;

    case TEST_STATE_COMMISSIONING8C1:
        ZB_ZGPD_CHACK_RESET_ALL();
        TRACE_MSG(TRACE_APP1, "state:8C1", (FMT__0));
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH);
        ZGPD->ch_replace_cr_next_ch = (TEST_CHANNEL + 6 - ZB_ZGPD_FIRST_CH) | ((TEST_CHANNEL + 6 - ZB_ZGPD_FIRST_CH) << 4);
        zb_zgpd_start_commissioning(my_comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 5.12));
        break;
    case TEST_STATE_COMMISSIONING8C2:
        ZB_ZGPD_CHACK_RESET_ALL();
        TRACE_MSG(TRACE_APP1, "state:8C2", (FMT__0));
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        ZGPD->comm_state = ZB_ZGPD_STATE_NOT_COMMISSIONED;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_TRANSMIT_CH);
        ZGPD->ch_replace_cr_transmit_ch = TEST_CHANNEL;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH);
        ZGPD->ch_replace_cr_next_ch = (TEST_CHANNEL + 4 - ZB_ZGPD_FIRST_CH) | ((TEST_CHANNEL + 4 - ZB_ZGPD_FIRST_CH) << 4);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_AUTO_COMM);
        ZGPD->ch_replace_autocomm = 0;
        zb_zgpd_start_commissioning(my_comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 7.5));
        break;

    case TEST_STATE_COMMISSIONING7A:
        ZB_ZGPD_CHACK_RESET_ALL();
        TRACE_MSG(TRACE_APP1, "state:7A", (FMT__0));
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH);
        ZGPD->id.endpoint = 1;
        ZGPD->ch_replace_cr_next_ch = (TEST_CHANNEL - ZB_ZGPD_FIRST_CH) | ((TEST_CHANNEL - ZB_ZGPD_FIRST_CH) << 4);
        zb_zgpd_start_commissioning(comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, ZB_TIME_ONE_SECOND);
        break;
    case TEST_STATE_COMMISSIONING7A2:
        ZB_ZGPD_CHACK_RESET_ALL();
        TRACE_MSG(TRACE_APP1, "state:7A2", (FMT__0));
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH);
        ZGPD->ch_replace_cr_next_ch = (TEST_CHANNEL - ZB_ZGPD_FIRST_CH) | ((TEST_CHANNEL - ZB_ZGPD_FIRST_CH) << 4);
        zb_zgpd_start_commissioning(comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 1.5));
        break;
    case TEST_STATE_COMMISSIONING7B:
        ZB_ZGPD_CHACK_RESET_ALL();
        TRACE_MSG(TRACE_APP1, "state:7B", (FMT__0));
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH);
        ZGPD->ch_replace_cr_next_ch = (TEST_CHANNEL - ZB_ZGPD_FIRST_CH) | ((TEST_CHANNEL - ZB_ZGPD_FIRST_CH) << 4);
        zb_zgpd_start_commissioning(comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, ZB_TIME_ONE_SECOND);
        break;
    case TEST_STATE_COMMISSIONING7B2:
        ZB_ZGPD_CHACK_RESET_ALL();
        TRACE_MSG(TRACE_APP1, "state:7B2", (FMT__0));
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH);
        ZGPD->ch_replace_cr_next_ch = (TEST_CHANNEL - ZB_ZGPD_FIRST_CH) | ((TEST_CHANNEL - ZB_ZGPD_FIRST_CH) << 4);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_AUTO_COMM);
        ZGPD->ch_replace_autocomm = 0;
        zb_zgpd_start_commissioning(comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 1.5));
        break;

    case TEST_STATE_COMMISSIONING7C:
        ZB_ZGPD_CHACK_RESET_ALL();
        TRACE_MSG(TRACE_APP1, "state:7C", (FMT__0));
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_STOP_AFTER_1_COMMREQ);
        //      ZGPD->channel = TEST_CHANNEL;
        //      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH);
        //      ZGPD->ch_replace_cr_next_ch = (TEST_CHANNEL-ZB_ZGPD_FIRST_CH)|((TEST_CHANNEL-ZB_ZGPD_FIRST_CH)<<4);
        zb_zgpd_start_commissioning(comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, ZB_TIME_ONE_SECOND * 2.0);
        break;
    case TEST_STATE_COMMISSIONING7C2:
        ZB_ZGPD_CHACK_RESET_ALL();
        TRACE_MSG(TRACE_APP1, "state:7C2", (FMT__0));
        ZGPD->commissioning_method = ZB_ZGPD_COMMISSIONING_UNIDIR;
        ZGPD->comm_state = ZB_ZGPD_STATE_NOT_COMMISSIONED;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_RXAFTERTX);
        ZGPD->ch_replace_rxaftertx = 1;
        //      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_AUTO_COMM);
        ZGPD->rx_on = ZB_FALSE;
        //      ZGPD->ch_replace_autocomm = 1;
        ZGPD->toggle_channel = ZB_FALSE;
        zb_zgpd_start_commissioning(my_comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 4.5));
        break;

    case TEST_STATE_COMMISSIONING7D:
        ZB_ZGPD_CHACK_RESET_ALL();
        TRACE_MSG(TRACE_APP1, "state:7D", (FMT__0));
        ZGPD->commissioning_method = ZB_ZGPD_COMMISSIONING_BIDIR;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_STOP_AFTER_1_COMMREQ);
        zb_zgpd_start_commissioning(comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, ZB_TIME_ONE_SECOND * 2.0);
        break;
    case TEST_STATE_COMMISSIONING7D2:
        ZB_ZGPD_CHACK_RESET_ALL();
        TRACE_MSG(TRACE_APP1, "state:7D2", (FMT__0));
        ZGPD->commissioning_method = ZB_ZGPD_COMMISSIONING_UNIDIR;
        ZGPD->comm_state = ZB_ZGPD_STATE_NOT_COMMISSIONED;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_RXAFTERTX);
        ZGPD->ch_replace_rxaftertx = 1;
        ZGPD->channel = TEST_CHANNEL;
        ZGPD->rx_on = ZB_FALSE;
        ZGPD->toggle_channel = ZB_FALSE;
        zb_zgpd_start_commissioning(my_comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 5.5));
        break;

    case TEST_STATE_COMMISSIONING6:
        TRACE_MSG(TRACE_APP1, "state:6", (FMT__0));
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->security_frame_counter = 128;

        ZB_ZGPD_INIT_ZGPD_CTX(ZB_ZGP_APP_ID_0010, ZB_ZGPD_COMMISSIONING_BIDIR, ZB_ZGP_ON_OFF_SWITCH_DEV_ID);

        ZGPD->rx_on = ZB_TRUE;
        ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_zgpd_addr);
        ZB_IEEE_ADDR_COPY(&g_zgpd_ctx.id.addr.ieee_addr, &g_zgpd_addr);
        g_zgpd_ctx.id.endpoint = 1;

        //      ZB_ZGPD_SET_SRC_ID(g_zgpd_srcId);
        ZB_ZGPD_SET_SECURITY_LEVEL(ZB_ZGP_SEC_LEVEL_FULL_NO_ENC);
        ZB_ZGPD_SET_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_NWK);
        ZB_ZGPD_SET_OOB_KEY(g_oob_key);
        ZB_ZGPD_SET_SECURITY_KEY(g_key_nwk);

        //      ZGPD->security_frame_counter=sfc;
        //      ZGPD->ch_replace_security_key_encrypted = 0;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_UNPROTECT_SUCCESS);
        zb_zgpd_start_commissioning(my_comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 2.5));
        //      ZB_ZGPD_SET_PAUSE(2);
        break;
    case TEST_STATE_COMMISSIONING6B:
        TRACE_MSG(TRACE_APP1, "state:6B", (FMT__0));
        ZB_ZGPD_CHACK_RESET_ALL();

        //      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_FRAME_COUNTER);
        //      ZGPD->ch_replace_frame_counter = 128;
        ZGPD->security_frame_counter = 0;

        zgpd_send_success_cmd_delayed(0);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 0.2));
        break;
    case TEST_STATE_COMMISSIONING6C:
        TRACE_MSG(TRACE_APP1, "state:6C", (FMT__0));
        ZB_ZGPD_CHACK_RESET_ALL();
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_KEY_TYPE);
        ZGPD->ch_replace_key_type = ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL;
        ZGPD->security_frame_counter = 10;

        zgpd_send_success_cmd_delayed(0);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 0.2));
        break;
    case TEST_STATE_COMMISSIONING6D:
        TRACE_MSG(TRACE_APP1, "state:6D", (FMT__0));
        ZB_ZGPD_CHACK_RESET_ALL();
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_KEY);
        ZB_MEMCPY(ZGPD->ch_replace_key, g_bad_key, ZB_CCM_KEY_SIZE);
        zgpd_send_success_cmd_delayed(0);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 0.2));
        break;
    case TEST_STATE_COMMISSIONING6E:
        TRACE_MSG(TRACE_APP1, "state:6E", (FMT__0));
        ZB_ZGPD_CHACK_RESET_ALL();
        ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_zgpd_addr_bad);
        ZB_IEEE_ADDR_COPY(&g_zgpd_ctx.id.addr.ieee_addr, &g_zgpd_addr_bad);
        zgpd_send_success_cmd_delayed(0);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 0.2));
        break;
    case TEST_STATE_COMMISSIONING6F:
        TRACE_MSG(TRACE_APP1, "state:6F", (FMT__0));
        ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_zgpd_addr);
        ZB_IEEE_ADDR_COPY(&g_zgpd_ctx.id.addr.ieee_addr, &g_zgpd_addr);
        ZGPD->security_frame_counter = 0;
        zgpd_send_success_cmd_delayed(0);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, ZB_TIME_ONE_SECOND * 5);
        break;
    case TEST_STATE_COMMISSIONING6G:
        TRACE_MSG(TRACE_APP1, "state:6G", (FMT__0));
        ZGPD->security_frame_counter = 20;
        zgpd_send_success_cmd_delayed(0);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, ZB_TIME_ONE_SECOND * 2);
        break;
    case TEST_STATE_COMMISSIONING6H:
        TRACE_MSG(TRACE_APP1, "state:6H", (FMT__0));
        ZGPD->security_frame_counter = 20;
        //      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_DO_NOT_SEND_SUCCESS);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_KEY);
        ZB_MEMCPY(ZGPD->ch_replace_key, g_bad_key, ZB_CCM_KEY_SIZE);
        zb_zgpd_start_commissioning(my_comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 2.5));
        break;
    case TEST_STATE_COMMISSIONING6I:
        TRACE_MSG(TRACE_APP1, "state:6I", (FMT__0));
        ZB_ZGPD_CHACK_RESET_ALL();
        ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_zgpd_addr_bad);
        ZB_IEEE_ADDR_COPY(&g_zgpd_ctx.id.addr.ieee_addr, &g_zgpd_addr_bad);
        zgpd_send_success_cmd_delayed(0);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 0.3));
        break;
    case TEST_STATE_COMMISSIONING6J:
        TRACE_MSG(TRACE_APP1, "state:6J", (FMT__0));
        ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_zgpd_addr);
        ZB_IEEE_ADDR_COPY(&g_zgpd_ctx.id.addr.ieee_addr, &g_zgpd_addr);
        ZGPD->id.endpoint = 31;
        zgpd_send_success_cmd_delayed(0);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, ZB_TIME_ONE_SECOND * 6);
        break;
    case TEST_STATE_COMMISSIONING51:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_STOP_AFTER_1_COMMREQ);
        zb_zgpd_start_commissioning(comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 1.5));
        break;
    case TEST_STATE_COMMISSIONING52:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->commissioning_method = ZB_ZGPD_COMMISSIONING_UNIDIR;
        ZGPD->comm_state = ZB_ZGPD_STATE_NOT_COMMISSIONED;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_RXAFTERTX);
        ZGPD->ch_replace_rxaftertx = 1;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_AUTO_COMM);
        ZGPD->rx_on = ZB_FALSE;
        ZGPD->ch_replace_autocomm = 1;
        ZGPD->toggle_channel = ZB_TRUE;
        zb_zgpd_start_commissioning(my_comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 0.2));
        break;
    case TEST_STATE_COMMISSIONING53:
        TRACE_MSG(TRACE_APP1, "state:53", (FMT__0));
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->commissioning_method = ZB_ZGPD_COMMISSIONING_UNIDIR;
        ZGPD->comm_state = ZB_ZGPD_STATE_NOT_COMMISSIONED;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_RXAFTERTX);
        ZGPD->ch_replace_rxaftertx = 0;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_AUTO_COMM);
        ZGPD->ch_replace_autocomm = 1;
        zb_zgpd_start_commissioning(my_comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 0.2));
        break;
    case TEST_STATE_COMMISSIONING54:
        TRACE_MSG(TRACE_APP1, "state:54", (FMT__0));
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->channel = TEST_CHANNEL;
        ZGPD->commissioning_method = ZB_ZGPD_COMMISSIONING_BIDIR;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        //    ZGPD->comm_state = ZB_ZGPD_STATE_NOT_COMMISSIONED;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_RXAFTERTX);
        ZGPD->ch_replace_rxaftertx = 0;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CMD);
        ZGPD->ch_replace_cmd = 0xe2;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_SEC_LEVEL);
        ZGPD->ch_replace_sec_level = ZB_ZGP_SEC_LEVEL_FULL_NO_ENC;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_AUTO_COMM);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_USE_CH_SEC_LEVEL);
        ZGPD->ch_replace_autocomm = 1;
        zb_zgpd_start_commissioning(my_comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 0.2));
        break;
    case TEST_STATE_COMMISSIONING55:
        TRACE_MSG(TRACE_APP1, "state:55", (FMT__0));
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->commissioning_method = ZB_ZGPD_COMMISSIONING_UNIDIR;
        ZGPD->rx_on = ZB_TRUE;
        //      ZGPD->comm_state = ZB_ZGPD_STATE_NOT_COMMISSIONED;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_RXAFTERTX);
        ZGPD->ch_replace_rxaftertx = 1;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_AUTO_COMM);
        ZGPD->ch_replace_autocomm = 0;
        //      ZGPD->toggle_channel = ZB_TRUE;
        zb_zgpd_start_commissioning(my_comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 0.5));
        break;
    case TEST_STATE_COMMISSIONING31:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH);
        ZGPD->ch_replace_cr_next_ch = (TEST_CHANNEL - ZB_ZGPD_FIRST_CH) | ((TEST_CHANNEL - ZB_ZGPD_FIRST_CH) << 4);
        zb_zgpd_start_commissioning(my_comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 0.5));
        break;
    case TEST_STATE_COMMISSIONING32:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_FRAME_TYPE);
        ZGPD->ch_replace_frame_type = 0;
        ZGPD->ch_replace_cr_next_ch = (TEST_CHANNEL - ZB_ZGPD_FIRST_CH) | ((TEST_CHANNEL - ZB_ZGPD_FIRST_CH) << 4);
        ZB_ZGPD_SET_SRC_ID(0);
        zb_zgpd_start_commissioning(my_comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 0.5));
        break;
    case TEST_STATE_COMMISSIONING33:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_PROTO_VERSION);
        ZGPD->ch_replace_proto_version = 2;
        ZGPD->ch_replace_cr_next_ch = (TEST_CHANNEL - ZB_ZGPD_FIRST_CH) | ((TEST_CHANNEL - ZB_ZGPD_FIRST_CH) << 4);
        zb_zgpd_start_commissioning(my_comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 0.5));
        break;
    case TEST_STATE_COMMISSIONING34:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_EXTNWK_FC_FLAG);
        ZGPD->ch_replace_extnwk_flag = 1;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_INSERT_EXTNWK_FC_DATA);
        ZGPD->ch_insert_extnwk_data = 1;
        ZGPD->ch_replace_cr_next_ch = (TEST_CHANNEL - ZB_ZGPD_FIRST_CH) | ((TEST_CHANNEL - ZB_ZGPD_FIRST_CH) << 4);
        zb_zgpd_start_commissioning(my_comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 0.5));
        break;
    case TEST_STATE_COMMISSIONING41:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH);
        ZGPD->ch_replace_cr_next_ch = (TEST_CHANNEL + 6 - ZB_ZGPD_FIRST_CH) | ((TEST_CHANNEL + 6 - ZB_ZGPD_FIRST_CH) << 4);
        zb_zgpd_start_commissioning(my_comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 0.5));
        break;
    case TEST_STATE_COMMISSIONING42:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        ZGPD->comm_state = ZB_ZGPD_STATE_NOT_COMMISSIONED;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_TRANSMIT_CH);
        ZGPD->ch_replace_cr_transmit_ch = TEST_CHANNEL + 6;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH);
        ZGPD->ch_replace_cr_next_ch = (TEST_CHANNEL + 6 - ZB_ZGPD_FIRST_CH) | ((TEST_CHANNEL + 6 - ZB_ZGPD_FIRST_CH) << 4);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CMD);
        ZGPD->ch_replace_cmd = 0xf3;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_AUTO_COMM);
        ZGPD->ch_replace_autocomm = 0;
        zb_zgpd_start_commissioning(my_comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 0.5));
        break;
    case TEST_STATE_COMMISSIONING43:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->comm_state = ZB_ZGPD_STATE_NOT_COMMISSIONED;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_TRANSMIT_CH);
        ZGPD->ch_replace_cr_transmit_ch = TEST_CHANNEL + 6;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_RXAFTERTX);
        ZGPD->ch_replace_rxaftertx = 1;
        ZGPD->commissioning_method = ZB_ZGPD_COMMISSIONING_UNIDIR;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_AUTO_COMM);
        ZGPD->ch_replace_autocomm = 0;
        zb_zgpd_start_commissioning(my_comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 0.5));
        break;
    case TEST_STATE_COMMISSIONING44:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->comm_state = ZB_ZGPD_STATE_NOT_COMMISSIONED;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_TRANSMIT_CH);
        ZGPD->ch_replace_cr_transmit_ch = TEST_CHANNEL + 6;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_RXAFTERTX);
        ZGPD->ch_replace_rxaftertx = 0;
        ZGPD->commissioning_method = ZB_ZGPD_COMMISSIONING_UNIDIR;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_AUTO_COMM);
        ZGPD->ch_replace_autocomm = 0;
        zb_zgpd_start_commissioning(my_comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 0.5));
        break;
    case TEST_STATE_COMMISSIONING45:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->commissioning_method = ZB_ZGPD_COMMISSIONING_BIDIR;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        //      ZGPD->comm_state = ZB_ZGPD_STATE_NOT_COMMISSIONED;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_TRANSMIT_CH);
        ZGPD->ch_replace_cr_transmit_ch = TEST_CHANNEL + 6;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_RXAFTERTX);
        ZGPD->ch_replace_rxaftertx = 0;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CMD);
        ZGPD->ch_replace_cmd = 0xe2;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_AUTO_COMM);
        ZGPD->ch_replace_autocomm = 0;
        zb_zgpd_start_commissioning(my_comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 0.5));
        break;
    case TEST_STATE_COMMISSIONING46:
        ZB_ZGPD_CHACK_RESET_ALL();
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH);
        ZGPD->ch_replace_cr_next_ch = (TEST_CHANNEL + 6 - ZB_ZGPD_FIRST_CH) | ((TEST_CHANNEL + 6 - ZB_ZGPD_FIRST_CH) << 4);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_TRANSMIT_CH);
        ZGPD->ch_replace_cr_transmit_ch = TEST_CHANNEL + 6;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_AUTO_COMM);
        ZGPD->ch_replace_autocomm = 0;
        ZGPD->auto_commissioning_pending = 0;
        zb_zgpd_start_commissioning(my_comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, ZB_TIME_ONE_SECOND * 2);
        break;
    case TEST_STATE_COMMISSIONING2B:
        TRACE_MSG(TRACE_APP1, "state:2B", (FMT__0));
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->gpd_security_key_request = 0;
        ZGPD->commissioning_method = ZB_ZGPD_COMMISSIONING_UNIDIR;
        ZGPD->comm_state = ZB_ZGPD_STATE_NOT_COMMISSIONED;
        ZGPD->channel = TEST_CHANNEL;
        //      zb_zgpd_stop_commissioning(comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(ZB_TIME_ONE_SECOND * 5.5));
        break;

    case TEST_STATE_COMMISSIONING2A:
        TRACE_MSG(TRACE_APP1, "state:2A", (FMT__0));
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->gpd_security_key_request = 1;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_SKIP_FIRST_COMM_REPLY);
        ZGPD->ch_skip_first_n_comm_reply = 1;
        ZGPD->ch_resend_success_gpdf = 10;
        //      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_TRANSMIT_CH);
        //      ZGPD->ch_replace_cr_transmit_ch = TEST_CHANNEL;
        zb_zgpd_start_commissioning(my_comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, ZB_TIME_ONE_SECOND * 4);
        //      ZB_ZGPD_SET_PAUSE(3);
        break;
    case TEST_STATE_COMMISSIONING1A:
        TRACE_MSG(TRACE_APP1, "state:1A", (FMT__0));
        ZB_ZGPD_CHACK_RESET_ALL();
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_REQ_SERIES);
        //      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_SKIP_CH_INC);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_TRANSMIT_CH);
        ZGPD->ch_replace_cr_transmit_ch = TEST_CHANNEL;
        zb_zgpd_start_commissioning(comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, ZB_TIME_ONE_SECOND * 6);
        break;
    case TEST_STATE_COMMISSIONING1B1:
        TRACE_MSG(TRACE_APP1, "state:1B1", (FMT__0));
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->comm_state = ZB_ZGPD_STATE_NOT_COMMISSIONED;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        //      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_SKIP_CH_INC);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH);
        ZGPD->ch_replace_cr_next_ch = (TEST_CHANNEL - ZB_ZGPD_FIRST_CH) | ((TEST_CHANNEL - ZB_ZGPD_FIRST_CH) << 4);
        zb_zgpd_start_commissioning(comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, ZB_TIME_ONE_SECOND);
        //      ZB_ZGPD_SET_PAUSE(0);
        break;
    case TEST_STATE_COMMISSIONING1B2:
        TRACE_MSG(TRACE_APP1, "state:1B2", (FMT__0));
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->comm_state = ZB_ZGPD_STATE_NOT_COMMISSIONED;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_REQ_SERIES);
        //      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_SKIP_CH_INC);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH);
        ZGPD->ch_replace_cr_next_ch = (TEST_CHANNEL - ZB_ZGPD_FIRST_CH) | ((TEST_CHANNEL - ZB_ZGPD_FIRST_CH) << 4);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_TRANSMIT_CH);
        ZGPD->ch_replace_cr_transmit_ch = TEST_CHANNEL;
        zb_zgpd_start_commissioning(comm_cb);
        //      ZB_SCHEDULE_ALARM(perform_next_state, 0, ZB_TIME_ONE_SECOND*6);
        //      ZB_ZGPD_SET_PAUSE(2);
        break;
    case TEST_STATE_COMMISSIONING1C1:
        TRACE_MSG(TRACE_APP1, "state:1C1", (FMT__0));
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->comm_state = ZB_ZGPD_STATE_NOT_COMMISSIONED;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        //      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_SKIP_CH_INC);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH);
        ZGPD->ch_replace_cr_next_ch = (TEST_CHANNEL + 6 - ZB_ZGPD_FIRST_CH) | ((TEST_CHANNEL + 6 - ZB_ZGPD_FIRST_CH) << 4);
        zb_zgpd_start_commissioning(comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, ZB_TIME_ONE_SECOND);
        //      ZB_ZGPD_SET_PAUSE(0);
        break;
    case TEST_STATE_COMMISSIONING1C2:
        TRACE_MSG(TRACE_APP1, "state:1C2", (FMT__0));
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->comm_state = ZB_ZGPD_STATE_NOT_COMMISSIONED;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_REQ_SERIES);
        //      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_SKIP_CH_INC);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH);
        ZGPD->ch_replace_cr_next_ch = (TEST_CHANNEL + 6 - ZB_ZGPD_FIRST_CH) | ((TEST_CHANNEL + 6 - ZB_ZGPD_FIRST_CH) << 4);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_TRANSMIT_CH);
        ZGPD->ch_replace_cr_transmit_ch = TEST_CHANNEL + 6;
        zb_zgpd_start_commissioning(comm_cb);
        //      ZB_SCHEDULE_ALARM(perform_next_state, 0, ZB_TIME_ONE_SECOND*6);
        //      ZB_ZGPD_SET_PAUSE(2);
        break;
    case TEST_STATE_COMMISSIONING1D1:
        TRACE_MSG(TRACE_APP1, "state:1D1", (FMT__0));
        ZB_ZGPD_CHACK_RESET_ALL();
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        //      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_SKIP_CH_INC);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH);
        ZGPD->ch_replace_cr_next_ch = (TEST_CHANNEL - ZB_ZGPD_FIRST_CH) | ((TEST_CHANNEL - ZB_ZGPD_FIRST_CH) << 4);
        zb_zgpd_start_commissioning(comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(0.5 * ZB_TIME_ONE_SECOND));
        break;
    case TEST_STATE_COMMISSIONING1D2:
        TRACE_MSG(TRACE_APP1, "state:1D2", (FMT__0));
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->channel = TEST_CHANNEL + 1;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        //      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_SKIP_CH_INC);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH);
        ZGPD->ch_replace_cr_next_ch = (TEST_CHANNEL - ZB_ZGPD_FIRST_CH) | ((TEST_CHANNEL - ZB_ZGPD_FIRST_CH) << 4);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_TRANSMIT_CH);
        ZGPD->ch_replace_cr_transmit_ch = TEST_CHANNEL;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_START_CH);
        ZGPD->ch_replace_cr_start_ch = TEST_CHANNEL + 1;
        zb_zgpd_start_commissioning(comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(0.05 * ZB_TIME_ONE_SECOND));
        break;
    case TEST_STATE_COMMISSIONING1D3:
        TRACE_MSG(TRACE_APP1, "state:1D3", (FMT__0));
        //    ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->toggle_channel = ZB_FALSE;
        ZGPD->comm_state = ZB_ZGPD_STATE_NOT_COMMISSIONED;
        ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(0.5 * ZB_TIME_ONE_SECOND));
        break;
    case TEST_STATE_COMMISSIONING1D4:
        TRACE_MSG(TRACE_APP1, "state:1D4", (FMT__0));
        ZB_ZGPD_CHACK_RESET_ALL();
        ZGPD->channel = TEST_CHANNEL + 1;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ);
        //      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_SKIP_CH_INC);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH);
        ZGPD->ch_replace_cr_next_ch = (TEST_CHANNEL - ZB_ZGPD_FIRST_CH) | ((TEST_CHANNEL - ZB_ZGPD_FIRST_CH) << 4);
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_TRANSMIT_CH);
        ZGPD->ch_replace_cr_transmit_ch = TEST_CHANNEL;
        ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_CR_START_CH);
        ZGPD->ch_replace_cr_start_ch = TEST_CHANNEL + 1;
        zb_zgpd_start_commissioning(comm_cb);
        ZB_SCHEDULE_ALARM(perform_next_state, 0, 5 * ZB_TIME_ONE_SECOND);
        break;
    case TEST_STATE_FINISHED:
        TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
        break;
    default:
        ZB_SCHEDULE_ALARM(test_send_command, 0, ZB_TIME_ONE_SECOND);
    };
    if (param)
    {
        zb_free_buf(ZB_BUF_FROM_REF(param));
    }
}

static void make_gpdf(zb_buf_t *buf, zb_uint8_t **ptr)
{
    ZVUNUSED(buf);
    ZVUNUSED(ptr);
    switch (TEST_DEVICE_CTX.test_state)
    {
    /*    case TEST_STATE_SEND_CMD_TOGGLE2:
    //      ZB_ZGPD_SET_SECURITY_LEVEL(ZB_ZGP_SEC_LEVEL_FULL_NO_ENC);
        case TEST_STATE_SEND_CMD_TOGGLE1:
          ZB_ZGPD_CHACK_RESET_ALL();
          g_fc_start = ZGPD->security_frame_counter;
          ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
    //      ZB_ZGPD_SET_PAUSE(1);
        break;
    */
    /*    case TEST_STATE_SEND_CMD_TOGGLE2:
          g_fc_start = ZGPD->security_frame_counter;
          ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
          ZB_ZGPD_SET_PAUSE(1);
        break;*/
    default:
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

    ZB_ZGPD_SET_SECURITY_LEVEL(ZB_ZGP_SEC_LEVEL_FULL_NO_ENC);
    ZB_ZGPD_SET_SECURITY_KEY_TYPE(TEST_KEY_TYPE);
    //ZB_ZGPD_SET_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL);
    ZB_ZGPD_SET_SECURITY_KEY(g_key_nwk);

    ZB_ZGPD_SET_OOB_KEY(g_oob_key);
    //  ZB_ZGPD_REQUEST_SECURITY_KEY();

    //  ZB_ZGPD_USE_MAINTENANCE_FRAME_FOR_CHANNEL_REQ();

    ZGPD->channel = TEST_CHANNEL;
}

#endif /* ZB_CERTIFICATION_HACKS */

ZB_ZGPD_TH_DECLARE_STARTUP_PROCESS()
