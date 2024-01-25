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

#define ZB_TEST_NAME GPP_SECURED_10_DATA_GPDF_APP_010_TH_TOOL
#define ZB_TRACE_FILE_ID 41516

#include "test_config.h"

#include "../include/zgp_test_templates.h"

#ifdef ZB_CERTIFICATION_HACKS

static zb_ieee_addr_t g_th_tool_addr = TH_TOOL_IEEE_ADDR;
static zb_uint8_t g_key_nwk[] = NWK_KEY;
static zb_uint8_t g_sec_key[] = TEST_SEC_KEY;

#define ENDPOINT 10

enum test_states_e
{
    TEST_STATE_INITIAL,
    TEST_STATE_READY_TO_COMMISSIONING,
    TEST_STATE_READ_GPP_PROXY_TABLE1,
    TEST_STATE_READ_GPP_PROXY_TABLE2,
    TEST_STATE_READ_GPP_PROXY_TABLE3,
    TEST_STATE_FINISHED
};

ZB_ZGPC_DECLARE_ZCL_ON_OFF_TOGGLE_TEST_TEMPLATE(TEST_DEVICE_CTX, ENDPOINT, 1000)

static void send_zcl(zb_uint8_t buf_ref, zb_callback_t cb)
{
    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_READ_GPP_PROXY_TABLE1:
        zgp_cluster_read_attr(buf_ref, DUT_GPPB_ADDR, DUT_GPPB_ADDR_MODE,
                              ZB_ZCL_ATTR_GPP_PROXY_TABLE_ID,
                              ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
        ZB_ZGPC_SET_PAUSE(2);
        break;
    case TEST_STATE_READ_GPP_PROXY_TABLE2:
        zgp_cluster_read_attr(buf_ref, DUT_GPPB_ADDR, DUT_GPPB_ADDR_MODE,
                              ZB_ZCL_ATTR_GPP_PROXY_TABLE_ID,
                              ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
        ZB_ZGPC_SET_PAUSE(2);
        break;
    case TEST_STATE_READ_GPP_PROXY_TABLE3:
        zgp_cluster_read_attr(buf_ref, DUT_GPPB_ADDR, DUT_GPPB_ADDR_MODE,
                              ZB_ZCL_ATTR_GPP_PROXY_TABLE_ID,
                              ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
        break;
    };
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
        ZB_ZGPC_SET_PAUSE(2);
#ifdef ZB_USE_BUTTONS
        zb_led_blink_on(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif
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
    ZB_INIT("th_tool");

    ZB_AIB().aps_channel_mask = (1 << TEST_CHANNEL);
    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_th_tool_addr);
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
    ZGP_GPS_COMMUNICATION_MODE = ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED;

    ZGP_GPS_SECURITY_LEVEL = ZB_ZGP_FILL_GPS_SECURITY_LEVEL(
                                 ZB_ZGP_SEC_LEVEL_FULL_NO_ENC,
                                 ZB_ZGP_DEFAULT_SEC_LEVEL_PROTECTION_WITH_GP_LINK_KEY,
                                 ZB_ZGP_DEFAULT_SEC_LEVEL_INVOLVE_TC);
    ZGP_GP_SHARED_SECURITY_KEY_TYPE = ZB_ZGP_SEC_KEY_TYPE_GROUP;

    ZB_MEMCPY(ZGP_GP_SHARED_SECURITY_KEY, g_sec_key, sizeof(ZGP_GP_SHARED_SECURITY_KEY));

    ZGP_CTX().device_role = ZGP_DEVICE_COMBO_BASIC;

    TEST_DEVICE_CTX.custom_comm_cb = custom_comm_cb;
}

#endif /* ZB_CERTIFICATION_HACKS */

ZB_ZGPC_TH_DECLARE_STARTUP_PROCESS()
