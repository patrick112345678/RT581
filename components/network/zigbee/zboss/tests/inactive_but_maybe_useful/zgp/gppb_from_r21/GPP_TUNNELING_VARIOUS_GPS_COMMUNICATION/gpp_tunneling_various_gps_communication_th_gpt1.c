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
/* PURPOSE: TH gpt
*/

#define ZB_TEST_NAME GPP_TUNNELING_VARIOUS_GPS_COMMUNICATION_TH_GPT1
#define ZB_TRACE_FILE_ID 41532

#include "test_config.h"

#include "../include/zgp_test_templates.h"

#ifdef ZB_CERTIFICATION_HACKS

#define ENDPOINT 10

#define ZGPD_SEQ_NUM_CAP 1
#define ZGPD_RX_ON_CAP 0
#define ZGPD_FIX_LOC 0
#define ZGPD_USE_SECURITY 0

static zb_ieee_addr_t g_th_gpt1_addr = TH_GPT1_IEEE_ADDR;
static zb_ieee_addr_t g_th_gpt2_addr = TH_GPT2_IEEE_ADDR;
static zb_ieee_addr_t g_th_gpt3_addr = TH_GPT3_IEEE_ADDR;
static zb_uint8_t g_key_nwk[] = NWK_KEY;

/*! Program states according to test scenario */
enum test_states_e
{
    TEST_STATE_INITIAL,
    TEST_STATE_SEND_PAIRING_CONFIGURATION1,
    TEST_STATE_SEND_PAIRING_CONFIGURATION2,
    TEST_STATE_READY_TO_COMMISSIONING,
    TEST_STATE_READ_GPP_PROXY_TABLE,
    TEST_STATE_FINISHED
};

ZB_ZGPC_DECLARE_ZCL_ON_OFF_TOGGLE_TEST_TEMPLATE(TEST_DEVICE_CTX, ENDPOINT, 3000)

static void send_zcl(zb_uint8_t buf_ref, zb_callback_t cb)
{
    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_READ_GPP_PROXY_TABLE:
        zgp_cluster_read_attr(buf_ref, DUT_GPPB_ADDR, DUT_GPPB_ADDR_MODE,
                              ZB_ZCL_ATTR_GPP_PROXY_TABLE_ID,
                              ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
        ZB_ZGPC_SET_PAUSE(3);
        break;
    }
}

static zb_bool_t custom_comm_cb(zb_zgpd_id_t *zgpd_id, zb_zgp_comm_status_t result)
{
    ZVUNUSED(zgpd_id);

    if (result == ZB_ZGP_COMMISSIONING_COMPLETED)
    {
#ifdef ZB_USE_BUTTONS
        zb_led_blink_off(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif
        return ZB_TRUE;
    }
    return ZB_FALSE;
}

static void send_pairing_configuration(zb_uint8_t param, zb_ieee_addr_t ieee_addr, zb_zgp_sink_tbl_ent_t *ent)
{
    zb_zgp_cluster_list_t cll;
    zb_uint8_t            actions;
    zb_uint8_t            app_info;
    zb_uint8_t            num_paired_endpoints;
    zb_uint8_t            num_gpd_commands;
    zb_address_ieee_ref_t addr_ref;
    zb_uint16_t           short_address;

    ZB_ASSERT(param);
    if (zb_address_by_ieee(ieee_addr, ZB_FALSE, ZB_FALSE, &addr_ref) == RET_OK)
    {
        zb_address_short_by_ref(&short_address, addr_ref);
    }
    else
    {
        zb_free_buf(ZB_BUF_FROM_REF(param));
        TRACE_MSG(TRACE_ERROR, "Error find short address for TH_GPT by IEEE", (FMT__0));
        return;
    }

    actions = ZB_ZGP_FILL_GP_PAIRING_CONF_ACTIONS(ZGP_PAIRING_CONF_REPLACE,
              ZGP_PAIRING_CONF_SEND_PAIRING);

    app_info = ZB_ZGP_FILL_GP_PAIRING_CONF_APP_INFO(ZB_ZGP_GP_PAIRING_CONF_APP_INFO_MANUF_ID_NO_PRESENT,
               ZB_ZGP_GP_PAIRING_CONF_APP_INFO_MODEL_ID_NO_PRESENT,
               ZB_ZGP_GP_PAIRING_CONF_APP_INFO_GPD_CMDS_NO_PRESENT,
               ZB_ZGP_GP_PAIRING_CONF_APP_INFO_CLSTS_NO_PRESENT);
    num_paired_endpoints = 0;
    num_gpd_commands = 0;
    cll.server_cl_num = 0;
    cll.client_cl_num = 0;

    zgp_cluster_send_pairing_configuration(param,
                                           short_address,
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
                                           NULL);
}

static void send_pairing_configuration_to_th_gpt2(zb_uint8_t param)
{
    zb_zgp_sink_tbl_ent_t ent;

    ZB_BZERO(&ent, sizeof(zb_zgp_sink_tbl_ent_t));

    ent.options = ZGP_TBL_SINK_FILL_OPTIONS(
                      ZB_ZGP_APP_ID_0000,
                      ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED,
                      ZGPD_SEQ_NUM_CAP,
                      ZGPD_RX_ON_CAP,
                      ZGPD_FIX_LOC,
                      1, /* ZGPD_USE_ASSIGNED_ALIAS */
                      ZGPD_USE_SECURITY);

    ZB_MEMSET(ent.u.sink.sgrp, 0xff, sizeof(ent.u.sink.sgrp));
    ent.zgpd_id.src_id = TEST_ZGPD_SRC_ID;
    ent.security_counter = ZGPD_INITIAL_FRAME_COUNTER;
    ent.u.sink.device_id = ZB_ZGP_ON_OFF_SWITCH_DEV_ID;
    ent.u.sink.match_dev_tbl_idx = 0xff;

    ent.zgpd_assigned_alias = 0x5577;

    send_pairing_configuration(param, g_th_gpt2_addr, &ent);
}

static void send_pairing_configuration_to_th_gpt3(zb_uint8_t param)
{
    zb_zgp_sink_tbl_ent_t ent;

    ZB_BZERO(&ent, sizeof(zb_zgp_sink_tbl_ent_t));

    ent.options = ZGP_TBL_SINK_FILL_OPTIONS(
                      ZB_ZGP_APP_ID_0000,
                      ZGP_COMMUNICATION_MODE_GROUPCAST_PRECOMMISSIONED,
                      ZGPD_SEQ_NUM_CAP,
                      ZGPD_RX_ON_CAP,
                      ZGPD_FIX_LOC,
                      0, /* ZGPD_USE_ASSIGNED_ALIAS */
                      ZGPD_USE_SECURITY);

    ZB_MEMSET(ent.u.sink.sgrp, 0xff, sizeof(ent.u.sink.sgrp));
    ent.zgpd_id.src_id = TEST_ZGPD_SRC_ID;
    ent.security_counter = ZGPD_INITIAL_FRAME_COUNTER;
    ent.u.sink.device_id = ZB_ZGP_ON_OFF_SWITCH_DEV_ID;
    ent.u.sink.match_dev_tbl_idx = 0xff;

    ent.u.sink.sgrp[0].sink_group = 0x4444;
    ent.u.sink.sgrp[0].alias = 0x1122;

    send_pairing_configuration(param, g_th_gpt3_addr, &ent);
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
    case TEST_STATE_READY_TO_COMMISSIONING:
        zb_zgps_start_commissioning(ZGP_GPS_GET_COMMISSIONING_WINDOW() *
                                    ZB_TIME_ONE_SECOND);
#ifdef ZB_USE_BUTTONS
        zb_led_blink_on(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif
        break;
    case TEST_STATE_SEND_PAIRING_CONFIGURATION1:
        ZB_GET_OUT_BUF_DELAYED(send_pairing_configuration_to_th_gpt2);
        ZB_SCHEDULE_ALARM(PERFORM_NEXT_STATE, 0, ZB_TIME_ONE_SECOND);
        break;
    case TEST_STATE_SEND_PAIRING_CONFIGURATION2:
        ZB_GET_OUT_BUF_DELAYED(send_pairing_configuration_to_th_gpt3);
        ZB_SCHEDULE_ALARM(PERFORM_NEXT_STATE, 0, ZB_TIME_ONE_SECOND);
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
    ZB_INIT("th_gpt1");

    ZB_AIB().aps_channel_mask = (1 << TEST_CHANNEL);
    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_th_gpt1_addr);
    ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);

    zb_secur_setup_nwk_key(g_key_nwk, 0);

    zb_set_default_ed_descriptor_values();

    /* Must use NVRAM for ZGP */
    ZB_AIB().aps_use_nvram = 1;

    /* Need to block GPDF recv directly */
#ifdef ZB_ZGP_SKIP_GPDF_ON_NWK_LAYER
    ZG->nwk.skip_gpdf = 1;
#endif
#ifdef ZB_ZGP_RUNTIME_WORK_MODE_WITH_PROXIES
    ZGP_CTX().enable_work_with_proxies = 1;
#endif
#ifdef ZB_CERTIFICATION_HACKS
    ZB_CERT_HACKS().ccm_check_cb = NULL;
#endif

    ZB_NIB_SET_USE_MULTICAST(ZB_FALSE);
    ZGP_GPS_COMMUNICATION_MODE = ZGP_COMMUNICATION_MODE_LIGHTWEIGHT_UNICAST;

    ZGP_GPS_COMMISSIONING_EXIT_MODE = ZGP_COMMISSIONING_EXIT_MODE_ON_PAIRING_SUCCESS;

    TEST_DEVICE_CTX.custom_comm_cb = custom_comm_cb;
}

#endif /* ZB_CERTIFICATION_HACKS */

ZB_ZGPC_TH_DECLARE_STARTUP_PROCESS()
