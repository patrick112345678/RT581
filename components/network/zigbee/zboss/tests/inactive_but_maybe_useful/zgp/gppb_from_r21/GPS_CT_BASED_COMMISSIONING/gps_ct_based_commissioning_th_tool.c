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

#define ZB_TEST_NAME GPS_CT_BASED_COMMISSIONING_TH_TOOL
#define ZB_TRACE_FILE_ID 41520

#include "test_config.h"

#include "../include/zgp_test_templates.h"

#ifdef ZB_CERTIFICATION_HACKS

#define ZGPD_SEQ_NUM_CAP 1
#define ZGPD_RX_ON_CAP 0
#define ZGPD_FIX_LOC 0
#define ZGPD_USE_SECURITY 0

static zb_ieee_addr_t g_th_tool_addr = TH_TOOL_IEEE_ADDR;
static zb_ieee_addr_t g_th_gpd_addr = TH_GPD_IEEE_ADDR;
static zb_uint8_t g_oob_key[] = TEST_OOB_KEY;

/*! Program states according to test scenario */
enum test_states_e
{
    TEST_STATE_INITIAL,
    TEST_STATE_READ_GPS_SINK_TABLE,

    //ZGP_COMMUNICATION_MODE_LIGHTWEIGHT_UNICAST
    TEST_STATE_SEND_PAIRING_CONFIGURATION_10_A_LU,
    TEST_STATE_READ_GPS_SINK_TABLE_10_A_LU,
    TEST_STATE_SEND_PAIRING_CONFIGURATION_10_B_LU,
    TEST_STATE_READ_GPS_SINK_TABLE_10_B_LU,
    TEST_STATE_SEND_PAIRING_CONFIGURATION_10_C_LU,
    TEST_STATE_READ_GPS_SINK_TABLE_10_C_LU,
    TEST_STATE_SEND_PAIRING_CONFIGURATION_10_D_LU,
    TEST_STATE_SEND_PAIRING_CONFIGURATION_10_E_LU,
    TEST_STATE_READ_GPS_SINK_TABLE_10_E_LU,

    //ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED
    TEST_STATE_SEND_PAIRING_CONFIGURATION_10_A_GD,
    TEST_STATE_READ_GPS_SINK_TABLE_10_A_GD,
    TEST_STATE_SEND_PAIRING_CONFIGURATION_10_B_GD,
    TEST_STATE_READ_GPS_SINK_TABLE_10_B_GD,
    TEST_STATE_SEND_PAIRING_CONFIGURATION_10_C_GD,
    TEST_STATE_READ_GPS_SINK_TABLE_10_C_GD,
    TEST_STATE_SEND_PAIRING_CONFIGURATION_10_D_GD,
    TEST_STATE_SEND_PAIRING_CONFIGURATION_10_E_GD,
    TEST_STATE_READ_GPS_SINK_TABLE_10_E_GD,

    //ZGP_COMMUNICATION_MODE_GROUPCAST_PRECOMMISSIONED
    TEST_STATE_SEND_PAIRING_CONFIGURATION_10_A_GP,
    TEST_STATE_READ_GPS_SINK_TABLE_10_A_GP,
    TEST_STATE_SEND_PAIRING_CONFIGURATION_10_B_GP,
    TEST_STATE_READ_GPS_SINK_TABLE_10_B_GP,
    TEST_STATE_SEND_PAIRING_CONFIGURATION_10_C_GP,
    TEST_STATE_READ_GPS_SINK_TABLE_10_C_GP,
    TEST_STATE_SEND_PAIRING_CONFIGURATION_10_D_GP,
    TEST_STATE_SEND_PAIRING_CONFIGURATION_10_E_GP,
    TEST_STATE_READ_GPS_SINK_TABLE_10_E_GP,

    TEST_STATE_SEND_PAIRING_CONFIGURATION_11_A,
    TEST_STATE_READ_GPS_SINK_TABLE_11_A,
    TEST_STATE_SEND_PAIRING_CONFIGURATION_11_B,
    TEST_STATE_READ_GPS_SINK_TABLE_11_B,
    TEST_STATE_SEND_PAIRING_CONFIGURATION_11_C,
    TEST_STATE_READ_GPS_SINK_TABLE_11_C,
    TEST_STATE_SEND_PAIRING_CONFIGURATION_12_C,
    TEST_STATE_READ_GPS_SINK_TABLE_12_C,
    TEST_STATE_FINISHED
};

ZB_ZGPC_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 1000)

static void send_pairing_configuration(zb_uint8_t             buf_ref,
                                       zb_uint8_t             actions,
                                       zb_zgp_sink_tbl_ent_t *ent,
                                       zb_callback_t          cb)
{
    zb_zgp_cluster_list_t cll;
    zb_uint8_t            app_info;
    zb_uint8_t            num_paired_endpoints;
    zb_uint8_t            num_gpd_commands;

    app_info = ZB_ZGP_FILL_GP_PAIRING_CONF_APP_INFO(ZB_ZGP_GP_PAIRING_CONF_APP_INFO_MANUF_ID_NO_PRESENT,
               ZB_ZGP_GP_PAIRING_CONF_APP_INFO_MODEL_ID_NO_PRESENT,
               ZB_ZGP_GP_PAIRING_CONF_APP_INFO_GPD_CMDS_NO_PRESENT,
               ZB_ZGP_GP_PAIRING_CONF_APP_INFO_CLSTS_NO_PRESENT);
    num_paired_endpoints = 0xfe;
    num_gpd_commands = 0;
    cll.server_cl_num = 0;
    cll.client_cl_num = 0;

    zgp_cluster_send_pairing_configuration(buf_ref,
                                           DUT_GPS_ADDR,
                                           ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                           actions,
                                           ent,
                                           num_paired_endpoints,
                                           NULL,
                                           app_info,
                                           0,
                                           0,
                                           num_gpd_commands,
                                           NULL,
                                           &cll,
                                           cb);
}

static zb_uint8_t get_pairing_conf_action()
{
    zb_uint8_t action = 0;

    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_A_LU:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_A_GD:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_A_GP:
        action = ZB_ZGP_FILL_GP_PAIRING_CONF_ACTIONS(ZGP_PAIRING_CONF_REPLACE,
                 ZGP_PAIRING_CONF_SEND_PAIRING);
        break;
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_B_LU:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_B_GD:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_B_GP:
        action = ZB_ZGP_FILL_GP_PAIRING_CONF_ACTIONS(ZGP_PAIRING_CONF_REMOVE_PAIRING,
                 ZGP_PAIRING_CONF_SEND_PAIRING);
        break;
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_C_LU:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_C_GD:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_C_GP:
        action = ZB_ZGP_FILL_GP_PAIRING_CONF_ACTIONS(ZGP_PAIRING_CONF_EXTEND,
                 ZGP_PAIRING_CONF_NO_SEND_PAIRING);
        break;
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_11_A:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_11_B:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_11_C:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_12_C:
        action = ZB_ZGP_FILL_GP_PAIRING_CONF_ACTIONS(ZGP_PAIRING_CONF_EXTEND,
                 ZGP_PAIRING_CONF_SEND_PAIRING);
        break;
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_D_LU:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_D_GD:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_D_GP:
        action = ZB_ZGP_FILL_GP_PAIRING_CONF_ACTIONS(ZGP_PAIRING_CONF_NO_ACTION,
                 ZGP_PAIRING_CONF_SEND_PAIRING);
        break;
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_E_LU:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_E_GD:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_E_GP:
        action = ZB_ZGP_FILL_GP_PAIRING_CONF_ACTIONS(ZGP_PAIRING_CONF_REMOVE_GPD,
                 ZGP_PAIRING_CONF_SEND_PAIRING);
        break;
    };

    return action;
}

static zb_uint8_t get_pairing_conf_cm()
{
    zb_uint8_t cm = 0;

    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_A_LU:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_B_LU:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_C_LU:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_D_LU:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_E_LU:
        cm = ZGP_COMMUNICATION_MODE_LIGHTWEIGHT_UNICAST;
        break;
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_A_GD:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_B_GD:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_C_GD:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_D_GD:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_E_GD:
        cm = ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED;
        break;
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_A_GP:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_B_GP:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_C_GP:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_D_GP:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_E_GP:
        cm = ZGP_COMMUNICATION_MODE_GROUPCAST_PRECOMMISSIONED;
        break;
    default:
        cm = DUT_GPS_COMMUNICATION_MODE;
    };

    return cm;
}

static void setup_ent(zb_zgp_sink_tbl_ent_t *ent)
{
    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_11_A:
        //empty src_id
        break;
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_11_B:
        ent->options = (ent->options & 0xfffc) | ZB_ZGP_APP_ID_0010;
        ent->endpoint = 1;
        break;
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_11_C:
        ent->options = ZGP_TBL_SINK_FILL_OPTIONS(
                           ZB_ZGP_APP_ID_0000,
                           get_pairing_conf_cm(),
                           ZGPD_SEQ_NUM_CAP,
                           ZGPD_RX_ON_CAP,
                           ZGPD_FIX_LOC,
                           0, /* ZGPD_USE_ASSIGNED_ALIAS */
                           1 /*ZGPD_USE_SECURITY*/);
        ent->sec_options = ZGP_TBL_FILL_SEC_OPTIONS(ZB_ZGP_SEC_LEVEL_REDUCED,
                           ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL);
        ZB_MEMCPY(ent->zgpd_key, g_oob_key, sizeof(g_oob_key));
        ent->endpoint = 0;
        ent->zgpd_id.src_id = TEST_ZGPD2_SRC_ID;
        break;
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_12_C:
        ent->options = ZGP_TBL_SINK_FILL_OPTIONS(
                           ZB_ZGP_APP_ID_0010,
                           get_pairing_conf_cm(),
                           ZGPD_SEQ_NUM_CAP,
                           ZGPD_RX_ON_CAP,
                           ZGPD_FIX_LOC,
                           0, /* ZGPD_USE_ASSIGNED_ALIAS */
                           1 /*ZGPD_USE_SECURITY*/);
        ent->sec_options = ZGP_TBL_FILL_SEC_OPTIONS(ZB_ZGP_SEC_LEVEL_FULL_WITH_ENC,
                           ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL);
        ZB_MEMCPY(ent->zgpd_key, g_oob_key, sizeof(g_oob_key));
        ent->endpoint = 0xff;
        ent->zgpd_id.src_id = TEST_ZGPD2_SRC_ID;
        ZB_MEMCPY(ent->zgpd_id.ieee_addr, g_th_gpd_addr, sizeof(g_th_gpd_addr));
        break;
    default:
        ent->options = (ent->options & 0xfffc) | ZB_ZGP_APP_ID_0000;
        ent->endpoint = 0;
        ent->zgpd_id.src_id = TEST_ZGPD_SRC_ID;
    };
}

static void create_pairing_configuration(zb_uint8_t    buf_ref,
        zb_callback_t cb)
{
    zb_zgp_sink_tbl_ent_t ent;

    ZB_BZERO(&ent, sizeof(zb_zgp_sink_tbl_ent_t));

    ent.options = ZGP_TBL_SINK_FILL_OPTIONS(
                      ZB_ZGP_APP_ID_0000,
                      get_pairing_conf_cm(),
                      ZGPD_SEQ_NUM_CAP,
                      ZGPD_RX_ON_CAP,
                      ZGPD_FIX_LOC,
                      0, /* ZGPD_USE_ASSIGNED_ALIAS */
                      ZGPD_USE_SECURITY);

    ZB_MEMSET(ent.u.sink.sgrp, 0xff, sizeof(ent.u.sink.sgrp));

    setup_ent(&ent);

    ent.u.sink.device_id = ZB_ZGP_ON_OFF_SWITCH_DEV_ID;
    ent.u.sink.match_dev_tbl_idx = 0xff;

    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_A_GP:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_B_GP:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_C_GP:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_D_GP:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_E_GP:
        ent.u.sink.sgrp[0].sink_group = 0x3487;
        ent.u.sink.sgrp[0].alias = 0x7654;
        break;
    };

    send_pairing_configuration(buf_ref, get_pairing_conf_action(), &ent, cb);
}

static void send_zcl(zb_uint8_t buf_ref, zb_callback_t cb)
{
    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_READ_GPS_SINK_TABLE:
    case TEST_STATE_READ_GPS_SINK_TABLE_10_A_LU:
    case TEST_STATE_READ_GPS_SINK_TABLE_10_B_LU:
    case TEST_STATE_READ_GPS_SINK_TABLE_10_C_LU:
    case TEST_STATE_READ_GPS_SINK_TABLE_10_E_LU:
    case TEST_STATE_READ_GPS_SINK_TABLE_10_A_GD:
    case TEST_STATE_READ_GPS_SINK_TABLE_10_B_GD:
    case TEST_STATE_READ_GPS_SINK_TABLE_10_C_GD:
    case TEST_STATE_READ_GPS_SINK_TABLE_10_E_GD:
    case TEST_STATE_READ_GPS_SINK_TABLE_10_A_GP:
    case TEST_STATE_READ_GPS_SINK_TABLE_10_B_GP:
    case TEST_STATE_READ_GPS_SINK_TABLE_10_C_GP:
    case TEST_STATE_READ_GPS_SINK_TABLE_10_E_GP:
    case TEST_STATE_READ_GPS_SINK_TABLE_11_A:
    case TEST_STATE_READ_GPS_SINK_TABLE_11_B:
    case TEST_STATE_READ_GPS_SINK_TABLE_11_C:
    case TEST_STATE_READ_GPS_SINK_TABLE_12_C:
        zgp_cluster_read_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                              ZB_ZCL_ATTR_GPS_SINK_TABLE_ID,
                              ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
        ZB_ZGPC_SET_PAUSE(1);
        break;
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_A_LU:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_B_LU:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_C_LU:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_D_LU:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_E_LU:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_A_GD:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_B_GD:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_C_GD:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_D_GD:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_E_GD:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_A_GP:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_B_GP:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_C_GP:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_D_GP:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_10_E_GP:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_11_A:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_11_B:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_11_C:
    case TEST_STATE_SEND_PAIRING_CONFIGURATION_12_C:
        create_pairing_configuration(buf_ref, cb);
        ZB_ZGPC_SET_PAUSE(1);
        break;
    };
}

static void handle_gp_sink_table_response(zb_uint8_t buf_ref)
{
    zb_bool_t   test_error = ZB_FALSE;
    zb_buf_t   *buf = ZB_BUF_FROM_REF(buf_ref);
    zb_uint8_t *ptr = ZB_BUF_BEGIN(buf);

    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_READ_GPS_SINK_TABLE:
    case TEST_STATE_READ_GPS_SINK_TABLE_10_A_LU:
    case TEST_STATE_READ_GPS_SINK_TABLE_10_B_LU:
    case TEST_STATE_READ_GPS_SINK_TABLE_10_C_LU:
    case TEST_STATE_READ_GPS_SINK_TABLE_10_E_LU:
    case TEST_STATE_READ_GPS_SINK_TABLE_10_A_GD:
    case TEST_STATE_READ_GPS_SINK_TABLE_10_B_GD:
    case TEST_STATE_READ_GPS_SINK_TABLE_10_C_GD:
    case TEST_STATE_READ_GPS_SINK_TABLE_10_E_GD:
    case TEST_STATE_READ_GPS_SINK_TABLE_10_A_GP:
    case TEST_STATE_READ_GPS_SINK_TABLE_10_B_GP:
    case TEST_STATE_READ_GPS_SINK_TABLE_10_C_GP:
    case TEST_STATE_READ_GPS_SINK_TABLE_10_E_GP:
    case TEST_STATE_READ_GPS_SINK_TABLE_11_A:
    case TEST_STATE_READ_GPS_SINK_TABLE_11_B:
    case TEST_STATE_READ_GPS_SINK_TABLE_11_C:
    case TEST_STATE_READ_GPS_SINK_TABLE_12_C:
    {
        zb_uint8_t status;

        ZB_ZCL_PACKET_GET_DATA8(&status, ptr);

        if (status != ZB_ZCL_STATUS_SUCCESS)
        {
            test_error = ZB_TRUE;
        }
    }
    break;
    }

    if (test_error)
    {
        TEST_DEVICE_CTX.err_cnt++;
        TRACE_MSG(TRACE_APP1, "Error at state: %hd", (FMT__H, TEST_DEVICE_CTX.test_state));
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
    case TEST_STATE_FINISHED:
        if (TEST_DEVICE_CTX.err_cnt)
        {
            TRACE_MSG(TRACE_APP1, "Test finished. Status: ERROR[%hd]", (FMT__H, TEST_DEVICE_CTX.err_cnt));
        }
        else
        {
            TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
        }
        break;
    default:
    {
        if (param)
        {
            zb_free_buf(ZB_BUF_FROM_REF(param));
        }
        ZB_SCHEDULE_ALARM(test_send_command, 0, (zb_time_t)(0.1f * ZB_TIME_ONE_SECOND));
    }
    }
}

static void zgpc_custom_startup()
{
    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("th_tool");

    ZB_AIB().aps_channel_mask = (1 << TEST_CHANNEL);
    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_th_tool_addr);
    ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);

    zb_set_default_ed_descriptor_values();
#ifdef ZB_ZGP_SKIP_GPDF_ON_NWK_LAYER
    ZG->nwk.skip_gpdf = 1;
#endif
    ZGP_CTX().device_role = ZGP_DEVICE_COMMISSIONING_TOOL;

    TEST_DEVICE_CTX.gp_sink_tbl_req_cb = handle_gp_sink_table_response;
}

#endif /* ZB_CERTIFICATION_HACKS */

ZB_ZGPC_TH_DECLARE_STARTUP_PROCESS()
