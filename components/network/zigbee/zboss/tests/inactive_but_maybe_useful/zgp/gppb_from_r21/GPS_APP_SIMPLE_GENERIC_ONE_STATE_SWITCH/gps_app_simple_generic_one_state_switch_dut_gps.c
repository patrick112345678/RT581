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

#define ZB_TEST_NAME GPS_APP_SIMPLE_GENERIC_ONE_STATE_SWITCH_DUT_GPS

#define ZB_TRACE_FILE_ID 63540
#include "../include/zgp_test_templates.h"

#include "test_config.h"

#define ENDPOINT DUT_ENDPOINT

/*============================================================================*/
/*                             DECLARATIONS                                   */
/*============================================================================*/

static zb_ieee_addr_t g_dut_gps_addr = DUT_GPS_IEEE_ADDR;

static void schedule_delay(zb_uint32_t timeout);
static zb_bool_t custom_comm_cb(zb_zgpd_id_t *zgpd_id, zb_zgp_comm_status_t result);
static void start_comm(zb_uint32_t timeout);

/*============================================================================*/
/*                             FSM CORE                                       */
/*============================================================================*/
/*! Program states according to test scenario */
enum test_states_e
{
    TEST_STATE_INITIAL,
    /* STEP 1A */
    TEST_STATE_START_COMM_MODE_1A,
    TEST_STATE_WAIT_PAIRING_1A,
    TEST_STATE_WAIT_1A,
    /* STEP 1B */
    TEST_STATE_CLEAR_ST_1B,
    TEST_STATE_START_COMM_MODE_1B,
    TEST_STATE_WAIT_PAIRING_1B,
    TEST_STATE_WAIT_1B,
    /* STEP 2 */
    TEST_STATE_CLEAR_ST_2,
    TEST_STATE_STRAT_COMM_MODE_2,
    TEST_STATE_WAIT_PAIRING_2,
    TEST_STATE_WAIT_2,
    /* STEP 3 */
    TEST_STATE_CLEAR_ST_3,
    TEST_STATE_STRAT_COMM_MODE_3,
    TEST_STATE_WAIT_PAIRING_3,
    TEST_STATE_WAIT_3,
    /* FINISH */
    TEST_STATE_FINISHED
};

ZB_ZGPC_DECLARE_ZCL_ON_OFF_TOGGLE_TEST_TEMPLATE(TEST_DEVICE_CTX, ENDPOINT, 1000)

static void send_zcl(zb_uint8_t buf_ref, zb_callback_t cb)
{
    ZVUNUSED(buf_ref);
    ZVUNUSED(cb);
}

static void perform_next_state(zb_uint8_t param)
{
    if (param)
    {
        zb_free_buf(ZB_BUF_FROM_REF(param));
    }

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
    case TEST_STATE_START_COMM_MODE_1A:
    case TEST_STATE_START_COMM_MODE_1B:
    case TEST_STATE_STRAT_COMM_MODE_2:
    case TEST_STATE_STRAT_COMM_MODE_3:
    {
        start_comm(25);
        TRACE_MSG(TRACE_APP1, "Start commissioning", (FMT__0));
        break;
    }

    case TEST_STATE_WAIT_1A:
    case TEST_STATE_WAIT_1B:
    case TEST_STATE_WAIT_2:
    case TEST_STATE_WAIT_3:
    {
        TRACE_MSG(TRACE_APP1, "DELAY", (FMT__0));
        schedule_delay(TEST_PARAM_DUT_GPS_LONG_DELAY);
        break;
    }

    case TEST_STATE_CLEAR_ST_1B:
    case TEST_STATE_CLEAR_ST_2:
    case TEST_STATE_CLEAR_ST_3:
    {
        TRACE_MSG(TRACE_APP1, "CLEAR SINK TABLE", (FMT__0));
        zgp_tbl_clear();
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
        ZB_SCHEDULE_ALARM(test_send_command, 0, ZB_TIME_ONE_SECOND);
    }
    }

    TRACE_MSG(TRACE_APP1, "<perform_next_state", (FMT__0));
}

/*============================================================================*/
/*                            TEST IMPLEMENTATION                             */
/*============================================================================*/

static void schedule_delay(zb_uint32_t timeout)
{
    ZB_ZGPC_SET_PAUSE(timeout);
    ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
}

static zb_bool_t custom_comm_cb(zb_zgpd_id_t *zgpd_id, zb_zgp_comm_status_t result)
{
    zb_bool_t ret = ZB_FALSE;

    ZVUNUSED(zgpd_id);

    TRACE_MSG(TRACE_APP1, ">custom_comm_cb: result = %d, test_state = %d",
              (FMT__D_D, result, TEST_DEVICE_CTX.test_state));

    if (result == ZB_ZGP_COMMISSIONING_COMPLETED)
    {
#ifdef ZB_USE_BUTTONS
        zb_led_blink_off(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif
        switch (TEST_DEVICE_CTX.test_state)
        {
        case TEST_STATE_WAIT_PAIRING_1A:
        case TEST_STATE_WAIT_PAIRING_1B:
        case TEST_STATE_WAIT_PAIRING_2:
        case TEST_STATE_WAIT_PAIRING_3:
        {
            ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
            ret = ZB_FALSE;
            break;
        }
        }
    }

    TRACE_MSG(TRACE_APP1, "<custom_comm_cb", (FMT__0));

    return ret;
}

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
    ZG->nwk.skip_gpdf = 0;
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
                                 ZB_ZGP_SEC_LEVEL_NO_SECURITY,
                                 ZB_ZGP_DEFAULT_SEC_LEVEL_PROTECTION_WITH_GP_LINK_KEY,
                                 ZB_ZGP_DEFAULT_SEC_LEVEL_INVOLVE_TC);

    ZGP_GP_SHARED_SECURITY_KEY_TYPE = ZB_ZGP_SEC_KEY_TYPE_NO_KEY;
    ZGP_CTX().device_role = ZGP_DEVICE_COMBO_BASIC;
    TEST_DEVICE_CTX.custom_comm_cb = custom_comm_cb;
}

ZB_ZGPC_DECLARE_STARTUP_PROCESS()
