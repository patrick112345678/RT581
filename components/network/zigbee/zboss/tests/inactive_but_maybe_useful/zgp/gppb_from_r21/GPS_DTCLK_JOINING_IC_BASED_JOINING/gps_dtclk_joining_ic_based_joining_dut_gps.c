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
/* PURPOSE: DUT GPS
*/

#define ZB_TEST_NAME GPS_DTCLK_JOINING_IC_BASED_JOINING_DUT_GPS

#define ZB_TRACE_FILE_ID 63498
#include "../include/zgp_test_templates.h"

#include "test_config.h"

#define ENDPOINT DUT_ENDPOINT

/*============================================================================*/
/*                             DECLARATIONS                                   */
/*============================================================================*/

static zb_ieee_addr_t g_dut_gps_addr = DUT_GPS_IEEE_ADDR;
static zb_uint8_t g_key_nwk[] = NWK_KEY;
static zb_uint8_t g_ic[16 + 2] = SETUP_IC;

static void start_comm(zb_uint32_t timeout);

/*============================================================================*/
/*                             FSM CORE                                       */
/*============================================================================*/
/*! Program states according to test scenario */
enum test_states_e
{
    TEST_STATE_INITIAL,                /* 0 */
    TEST_STATE_WAITING1,               /* 1 */
    TEST_STATE_START_COMMISSIONING,    /* 2 */
    TEST_STATE_WAITING2,               /* 3 */
    /* FINISH */
    TEST_STATE_FINISHED                /* 4 */
};

ZB_ZGPC_DECLARE_ZCL_ON_OFF_TOGGLE_TEST_TEMPLATE(TEST_DEVICE_CTX, ENDPOINT, 1000)

static void send_zcl(zb_uint8_t buf_ref, zb_callback_t cb)
{
    ZVUNUSED(buf_ref);
    ZVUNUSED(cb);
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

    TRACE_MSG(TRACE_APP1, ">perform_next_state: state = %d",
              (FMT__D, TEST_DEVICE_CTX.test_state));

    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_WAITING1:
    {
        ZB_ZGPC_SET_PAUSE(TEST_STEP1_DUT_DELAY);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
    }
    case TEST_STATE_START_COMMISSIONING:
    {
        start_comm(70);
        break;
    }
    case TEST_STATE_WAITING2:
    {
        ZB_ZGPC_SET_PAUSE(TEST_STEP_REST_DUT_DELAY);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
    }

    case TEST_STATE_FINISHED:
    {
        TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
        break;
    }
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

static void start_comm(zb_uint32_t timeout)
{
    zb_zgps_start_commissioning(timeout * ZB_TIME_ONE_SECOND);
#ifdef ZB_USE_BUTTONS
    zb_led_blink_on(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif
    ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
}

/*============================================================================*/
/*                          STARTUP                                           */
/*============================================================================*/

static void zgpc_custom_startup()
{
    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("dut_gps");

    zb_set_long_address(g_dut_gps_addr);
    zb_set_network_router_role(1l << TEST_CHANNEL);

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

    ZGP_GPS_COMMUNICATION_MODE = ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED;
    ZGP_GPS_COMMISSIONING_EXIT_MODE = ZGP_COMMISSIONING_EXIT_MODE_ON_PAIRING_SUCCESS;

    ZGP_GPS_SECURITY_LEVEL = ZB_ZGP_FILL_GPS_SECURITY_LEVEL(
                                 ZB_ZGP_SEC_LEVEL_FULL_NO_ENC,
                                 ZB_TRUE /* ZB_ZGP_DEFAULT_SEC_LEVEL_PROTECTION_WITH_GP_LINK_KEY */,
                                 ZB_TRUE /* ZB_ZGP_DEFAULT_SEC_LEVEL_INVOLVE_TC */);

    ZGP_GP_SHARED_SECURITY_KEY_TYPE = ZB_ZGP_SEC_KEY_TYPE_NWK;
    ZGP_CTX().device_role = ZGP_DEVICE_COMBO_BASIC;

    ZB_BDB().bdb_join_uses_install_code_key = 1;
    zb_secur_ic_set(g_ic);
}

ZB_ZGPC_DECLARE_STARTUP_PROCESS()
