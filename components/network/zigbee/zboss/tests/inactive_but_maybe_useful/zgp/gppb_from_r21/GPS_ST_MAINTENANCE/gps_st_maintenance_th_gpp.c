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

#define ZB_TEST_NAME GPS_ST_MAINTENANCE_TH_GPP
#define ZB_TRACE_FILE_ID 41490

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

#ifdef ZB_ENABLE_HA
#include "../include/zgp_test_templates.h"

/*============================================================================*/
/*                             DECLARATIONS                                   */
/*============================================================================*/

static zb_ieee_addr_t g_th_tool_addr = TH_TOOL_IEEE_ADDR;
static zb_uint8_t g_key_nwk[] = NWK_KEY;
static zb_uint16_t g_dut_addr;

static void dev_annce_cb(zb_zdo_device_annce_t *da);
static void gp_pairing_handler_cb(zb_uint8_t buf_ref);
static void gp_proxy_comm_mode_handler_cb(zb_uint8_t buf_ref);

static void send_gpp_comm_notify(zb_uint8_t param);
static void fill_gpdf_info(zb_gpdf_info_t *gpdf_info);
static void save_sec_frame_counter(zb_gpdf_info_t *gpdf_info);
/* Options are defined by test state */
static zb_uint16_t request_options();
static void send_gp_notify(zb_uint8_t param);

/*============================================================================*/
/*                             FSM CORE                                       */
/*============================================================================*/

/*! Program states according to test scenario */
enum test_states_e
{
    TEST_STATE_INITIAL,
    /* Initial */
    TEST_STATE_WAIT_DUT_STARTUP,
    TEST_STATE_WAIT_PAIRING_INIT,
    TEST_STATE_WAIT_COMM_MODE_EXIT,
    /* STEP 2*/
    TEST_STATE_WAIT_COMM_MODE_ENTER_2,
    TEST_STATE_SEND_COMM_NOTIFY_2,
    TEST_STATE_WAIT_DECOMM_2,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_2,
    /* STEP 3 */
    TEST_STATE_WAIT_PAIRING_3,
    TEST_STATE_SEND_GP_NOTIFY_3,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_3,
    /* STEP 4A */
    TEST_STATE_WAIT_PAIRING_4A,
    TEST_STATE_WAIT_COMM_MODE_EXIT_4A,
    TEST_STATE_WAIT_COMM_MODE_ENTER_4A,
    TEST_STATE_SEND_COMM_NOTIFY_4A,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_4A,
    /* STEP 4B */
    TEST_STATE_SEND_COMM_NOTIFY_4B,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_4B,
    /* STEP 4B */
    TEST_STATE_SEND_COMM_NOTIFY_4C,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_4C,
    /* STEP 5 */
    TEST_STATE_WAIT_PAIRING_NE_5,
    TEST_STATE_WAIT_PAIRING_NF_5,
    TEST_STATE_WAIT_COMM_MODE_EXIT_5,
    TEST_STATE_WAIT_COMM_MODE_ENTER_5,
    TEST_STATE_SEND_COMM_NOTIFY_5,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_5,
    /* STEP 6 */
    TEST_STATE_WAIT_PAIRING_NE_6,
    TEST_STATE_WAIT_PAIRING_NF_6,
    TEST_STATE_WAIT_COMM_MODE_EXIT_6,
    TEST_STATE_WAIT_COMM_MODE_ENTER_6,
    TEST_STATE_SEND_COMM_NOTIFY_6,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_6,
    /* STEP 7 */
    TEST_STATE_WAIT_PAIRING_NE_7,
    TEST_STATE_WAIT_COMM_MODE_EXIT_7,
    TEST_STATE_WAIT_COMM_MODE_ENTER_7,
    TEST_STATE_SEND_COMM_NOTIFY_7,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_7,
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
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_2:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_3:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_4A:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_4B:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_4C:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_5:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_6:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_7:
        zgp_cluster_read_attr(buf_ref, g_dut_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                              ZB_ZCL_ATTR_GPS_SINK_TABLE_ID,
                              ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
        ZB_ZGPC_SET_PAUSE(TH_TOOL_SEND_NEXT_CMD_DELAY);
        break;
    case TEST_STATE_SEND_COMM_NOTIFY_2:
    case TEST_STATE_SEND_COMM_NOTIFY_4A:
    case TEST_STATE_SEND_COMM_NOTIFY_4B:
    case TEST_STATE_SEND_COMM_NOTIFY_4C:
    case TEST_STATE_SEND_COMM_NOTIFY_5:
    case TEST_STATE_SEND_COMM_NOTIFY_6:
    case TEST_STATE_SEND_COMM_NOTIFY_7:
        send_gpp_comm_notify(buf_ref);
        break;
    case TEST_STATE_SEND_GP_NOTIFY_3:
        send_gp_notify(buf_ref);
        break;
    }

    TRACE_MSG(TRACE_APP1, "<send_zcl", (FMT__0));
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

    TRACE_MSG(TRACE_APP1, ">perform_next_state: test_state = %d",
              (FMT__D, TEST_DEVICE_CTX.test_state));

    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_WAIT_DUT_STARTUP:
    case TEST_STATE_WAIT_PAIRING_INIT:
    case TEST_STATE_WAIT_COMM_MODE_EXIT:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_2:
    case TEST_STATE_WAIT_DECOMM_2:
    case TEST_STATE_WAIT_PAIRING_3:
    case TEST_STATE_WAIT_PAIRING_4A:
    case TEST_STATE_WAIT_COMM_MODE_EXIT_4A:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_4A:
    case TEST_STATE_WAIT_PAIRING_NE_5:
    case TEST_STATE_WAIT_PAIRING_NF_5:
    case TEST_STATE_WAIT_COMM_MODE_EXIT_5:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_5:
    case TEST_STATE_WAIT_PAIRING_NE_6:
    case TEST_STATE_WAIT_PAIRING_NF_6:
    case TEST_STATE_WAIT_COMM_MODE_EXIT_6:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_6:
    case TEST_STATE_WAIT_PAIRING_NE_7:
    case TEST_STATE_WAIT_COMM_MODE_EXIT_7:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_7:
        /* Transition to next state should be performed in handlers */
        if (param)
        {
            zb_free_buf(ZB_BUF_FROM_REF(param));
        }
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
            PERFORM_NEXT_STATE(0);
        }
    }
}

static void gp_pairing_handler_cb(zb_uint8_t buf_ref)
{
    TRACE_MSG(TRACE_APP1, ">gp_pairing_handler_cb: buf_ref = %d, test_state = %d",
              (FMT__D_D, buf_ref, TEST_DEVICE_CTX.test_state));

    ZVUNUSED(buf_ref);
    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_WAIT_PAIRING_INIT:
    case TEST_STATE_WAIT_DECOMM_2:
    case TEST_STATE_WAIT_PAIRING_3:
    case TEST_STATE_WAIT_PAIRING_4A:
    case TEST_STATE_WAIT_PAIRING_NE_5:
    case TEST_STATE_WAIT_PAIRING_NF_5:
    case TEST_STATE_WAIT_PAIRING_NE_6:
    case TEST_STATE_WAIT_PAIRING_NF_6:
    case TEST_STATE_WAIT_PAIRING_NE_7:
        PERFORM_NEXT_STATE(0);
        break;
    }

    TRACE_MSG(TRACE_APP1, "<gp_pairing_handler_cb", (FMT__0));
}

static void gp_proxy_comm_mode_handler_cb(zb_uint8_t buf_ref)
{
    TRACE_MSG(TRACE_APP1, ">gp_proxy_comm_mode_handler_cb: buf_ref = %d, test_state = %d",
              (FMT__D_D, buf_ref, TEST_DEVICE_CTX.test_state));

    ZVUNUSED(buf_ref);
    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_WAIT_COMM_MODE_EXIT:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_2:
    case TEST_STATE_WAIT_COMM_MODE_EXIT_4A:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_4A:
    case TEST_STATE_WAIT_COMM_MODE_EXIT_5:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_5:
    case TEST_STATE_WAIT_COMM_MODE_EXIT_6:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_6:
    case TEST_STATE_WAIT_COMM_MODE_EXIT_7:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_7:
        ZB_ZGPC_SET_PAUSE(TH_TOOL_SEND_NEXT_STATE_DELAY);
        PERFORM_NEXT_STATE(0);
        break;
    }

    TRACE_MSG(TRACE_APP1, "<gp_proxy_comm_mode_handler_cb", (FMT__0));
}

static void send_gpp_comm_notify(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_uint16_t options = 0;
    zb_gpdf_info_t *gpdf_info = ZB_GET_BUF_PARAM(buf, zb_gpdf_info_t);

    TRACE_MSG(TRACE_APP1, ">send_gpp_comm_notify: param = %d, test_state = %d",
              (FMT__D_D, param, TEST_DEVICE_CTX.test_state));

    fill_gpdf_info(gpdf_info);
    save_sec_frame_counter(gpdf_info);
    options = request_options();
    zb_zgp_cluster_gp_comm_notification_req(param,
                                            0 /* do not use alias */, 0x0000, 0x00,
                                            g_dut_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                            options,
                                            NULL);

    TRACE_MSG(TRACE_APP1, "<send_gpp_comm_notify", (FMT__0));
}

static void fill_gpdf_info(zb_gpdf_info_t *gpdf_info)
{
    static zb_ieee_addr_t ieee_n = TH_GPD_IEEE_ADDR;
    TRACE_MSG(TRACE_APP1, ">fill_gpd_info: gpdf_info = %p", (FMT__P, gpdf_info));

    ZB_BZERO(gpdf_info, sizeof(*gpdf_info));
    /* Always excellent */
    gpdf_info->rssi = 0x3;
    gpdf_info->lqi = 0x3b;
    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_SEND_COMM_NOTIFY_5:
    {
        gpdf_info->zgpd_id.app_id = ZB_ZGP_APP_ID_0010;
        gpdf_info->zgpd_id.endpoint = TEST_ZGPD_EP_E;
        ZB_IEEE_ADDR_COPY(gpdf_info->zgpd_id.addr.ieee_addr, ieee_n);
        break;
    }
    case TEST_STATE_SEND_COMM_NOTIFY_6:
    {
        gpdf_info->zgpd_id.app_id = ZB_ZGP_APP_ID_0010;
        gpdf_info->zgpd_id.endpoint = 0xFF;
        ZB_IEEE_ADDR_COPY(gpdf_info->zgpd_id.addr.ieee_addr, ieee_n);
        break;
    }
    case TEST_STATE_SEND_COMM_NOTIFY_7:
    {
        gpdf_info->zgpd_id.app_id = ZB_ZGP_APP_ID_0010;
        gpdf_info->zgpd_id.endpoint = TEST_ZGPD_EP_X;
        ZB_IEEE_ADDR_COPY(gpdf_info->zgpd_id.addr.ieee_addr, ieee_n);
        break;
    }
    default:
    {
        gpdf_info->zgpd_id.addr.src_id = TEST_ZGPD_SRC_ID;
    }
    }

    if (TEST_DEVICE_CTX.test_state != TEST_STATE_SEND_COMM_NOTIFY_4C)
    {
        gpdf_info->sec_frame_counter = zgp_proxy_table_get_security_counter(&gpdf_info->zgpd_id) + 1;
    }
    else
    {
        gpdf_info->sec_frame_counter = zgp_proxy_table_get_security_counter(&gpdf_info->zgpd_id) - 2;
    }

    gpdf_info->zgpd_cmd_id = ZB_GPDF_CMD_DECOMMISSIONING;

    TRACE_MSG(TRACE_APP1, "<fill_gpd_info", (FMT__0));
}

static void save_sec_frame_counter(zb_gpdf_info_t *gpdf_info)
{
    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_SEND_COMM_NOTIFY_4A:
    case TEST_STATE_SEND_COMM_NOTIFY_4B:
        zgp_proxy_table_set_security_counter(&gpdf_info->zgpd_id, gpdf_info->sec_frame_counter);
        break;
    }
}

static zb_uint16_t request_options()
{
    zb_uint16_t opt = 0;

    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_SEND_COMM_NOTIFY_2:
    case TEST_STATE_SEND_COMM_NOTIFY_4C:
    {
        TRACE_MSG(TRACE_APP3, "OPTIONS: #1", (FMT__0));
        /* APP ID */
        opt |= ZB_ZGP_APP_ID_0000;
        /* Security level */
        opt |= ZB_ZGP_SEC_LEVEL_FULL_NO_ENC << 4;
        /* Security Key type */
        opt |= ZB_ZGP_SEC_KEY_TYPE_NWK << 6;
        /* Proxy info present */
        opt |= (0x01 << 11);
        break;
    }

    case TEST_STATE_SEND_COMM_NOTIFY_4A:
    {
        TRACE_MSG(TRACE_APP3, "OPTIONS: #2", (FMT__0));
        /* APP ID */
        opt |= ZB_ZGP_APP_ID_0000;
        /* Security level */
        opt |= ZB_ZGP_SEC_LEVEL_NO_SECURITY << 4;
        /* Security Key type */
        opt |= ZB_ZGP_SEC_KEY_TYPE_NWK << 6;
        /* Proxy info present */
        opt |= (0x01 << 11);
        break;
    }

    case TEST_STATE_SEND_COMM_NOTIFY_4B:
    {
        TRACE_MSG(TRACE_APP3, "OPTIONS: #3", (FMT__0));
        /* APP ID */
        opt |= ZB_ZGP_APP_ID_0000;
        /* Security level */
        opt |= ZB_ZGP_SEC_LEVEL_FULL_NO_ENC << 4;
        /* Security Key type */
        opt |= ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL << 6;
        /* Proxy info present */
        opt |= (0x01 << 11);
        break;
    }

    case TEST_STATE_SEND_COMM_NOTIFY_5:
    case TEST_STATE_SEND_COMM_NOTIFY_6:
    case TEST_STATE_SEND_COMM_NOTIFY_7:
    {
        TRACE_MSG(TRACE_APP3, "OPTIONS: #4", (FMT__0));
        /* APP ID */
        opt |= ZB_ZGP_APP_ID_0010;
        /* Security level */
        opt |= ZB_ZGP_SEC_LEVEL_FULL_NO_ENC << 4;
        /* Security Key type */
        opt |= ZB_ZGP_SEC_KEY_TYPE_NWK << 6;
        /* Proxy info present */
        opt |= (0x01 << 11);
        break;
    }
    }

    TRACE_MSG(TRACE_APP1, "request_options: opt = 0x%x", (FMT__H, opt));

    return opt;
}


static void send_gp_notify(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_uint16_t options = 0;
    zb_gpdf_info_t *gpdf_info = ZB_GET_BUF_PARAM(buf, zb_gpdf_info_t);

    TRACE_MSG(TRACE_APP1, ">send_gp_notify: param = %d, test_state = %d",
              (FMT__D_D, param, TEST_DEVICE_CTX.test_state));

    /* APP ID */
    options |= ZB_ZGP_APP_ID_0000;
    /* Security level */
    options |= ZB_ZGP_SEC_LEVEL_FULL_NO_ENC << 6;
    /* Security Key type */
    options |= ZB_ZGP_SEC_KEY_TYPE_NWK << 8;
    /* Proxy info present */
    options |= (0x01 << 14);

    fill_gpdf_info(gpdf_info);
    zb_zgp_cluster_gp_notification_req(param,
                                       0 /* do not use alias */, 0x0000, 0x00,
                                       g_dut_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                       options, 0, /* groupcast radius */
                                       NULL);

    TRACE_MSG(TRACE_APP1, "<send_gp_notify", (FMT__0));
}

/*============================================================================*/
/*                          STARTUP                                           */
/*============================================================================*/

static void zgpc_custom_startup()
{
    /* Init device, load IB values from nvram or set it to default */
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
    //ZGP_CTX().device_role = ZGP_DEVICE_COMMISSIONING_TOOL;
    ZGP_CTX().device_role = ZGP_DEVICE_PROXY;

    zb_zdo_register_device_annce_cb(dev_annce_cb);
    TEST_DEVICE_CTX.gp_pairing_hndlr_cb = gp_pairing_handler_cb;
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
