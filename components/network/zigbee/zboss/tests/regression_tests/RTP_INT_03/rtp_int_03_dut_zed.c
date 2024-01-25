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
/* PURPOSE: DUT ZED
*/

#define ZB_TEST_NAME RTP_INT_03_DUT_ZED
#define ZB_TRACE_FILE_ID 40320

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"

#include "rtp_int_03_common.h"
#include "../common/zb_reg_test_globals.h"

#ifndef ZB_NSNG
#include "nrf.h"
#include "nrf_drv_gpiote.h"
#include "boards.h"
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */
#ifndef ZB_ED_ROLE
#error ED role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error define ZB_USE_NVRAM
#endif

#ifndef ZB_USE_SLEEP
#error define ZB_USE_SLEEP
#endif

#ifndef ZB_NSNG
#define PIN_IN NRF_GPIO_PIN_MAP(TEST_PORT1_DUT, TEST_PIN1_DUT)

#define PIN_OUT_DEBUG BSP_LED_0

void rtp_int_03_zed_in_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);
#endif

static zb_ieee_addr_t g_ieee_addr_dut_zed = IEEE_ADDR_DUT_ZED;

static void test_gpio_init(zb_uint8_t unused);
static void test_enable_external_int(zb_uint8_t unused);
static void test_stop_poll(zb_time_t ms);

static void trigger_steering(zb_uint8_t unused);
static void test_wake_up(zb_uint8_t unused);

static void can_sleep_signal_handler(zb_zdo_signal_can_sleep_params_t *can_sleep_params);

static zb_uint32_t g_sleep_start_time = 0;
static zb_bool_t g_test_completed = ZB_FALSE;

MAIN()
{
    ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
    ZB_SET_TRACE_LEVEL(4);
    ARGV_UNUSED;

    ZB_INIT("zdo_dut_zed");

    zb_set_long_address(g_ieee_addr_dut_zed);

    zb_reg_test_set_common_channel_settings();
    zb_set_network_ed_role((1l << TEST_CHANNEL));
    zb_set_nvram_erase_at_start(ZB_TRUE);
    zb_set_rx_on_when_idle(ZB_FALSE);

    if (zboss_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
    }
    else
    {
        zdo_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);

    zb_zdo_app_signal_hdr_t *sg_p = NULL;
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

    switch (sig)
    {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));

        ZB_SCHEDULE_CALLBACK(test_gpio_init, 0);
        test_stop_poll(0);
        break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_COMMON_SIGNAL_CAN_SLEEP:
        TRACE_MSG(TRACE_APS1, "signal: ZB_COMMON_SIGNAL_CAN_SLEEP, status %d", (FMT__D, status));
        if (status == 0)
        {
            zb_zdo_signal_can_sleep_params_t *can_sleep_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_can_sleep_params_t);

            can_sleep_signal_handler(can_sleep_params);
        }
        break; /* ZB_COMMON_SIGNAL_CAN_SLEEP */

    default:
        TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}

static void trigger_steering(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
}

static void can_sleep_signal_handler(zb_zdo_signal_can_sleep_params_t *can_sleep_params)
{
    zb_uint32_t sleep_tmo = can_sleep_params->sleep_tmo;

    if (sleep_tmo > TEST_SLEEP_THRESHOLD && !g_test_completed)
    {
        g_sleep_start_time = zb_get_utc_time();

#ifndef ZB_NSNG
        nrf_drv_gpiote_out_clear(PIN_OUT_DEBUG);

        test_enable_external_int(0);
#else
        /*
         * Another logic for NSNG. Schedule alarm in TEST_MIN_SLEEP_TIME + 1 and
         * check that device will wake up.
         */
        ZB_SCHEDULE_ALARM(test_enable_external_int, 0, (TEST_MIN_SLEEP_TIME + 1) * ZB_TIME_ONE_SECOND);
#endif
    }

    zb_sleep_now();

    if (sleep_tmo > TEST_SLEEP_THRESHOLD && !g_test_completed)
    {
#ifndef ZB_NSNG
        nrf_drv_gpiote_out_set(PIN_OUT_DEBUG);
#endif

        test_wake_up(0);
        g_sleep_start_time = 0;
        g_test_completed = ZB_TRUE;
    }
}

static void test_wake_up(zb_uint8_t unused)
{
    zb_uint32_t current_time = zb_get_utc_time();
    zb_uint32_t sleep_time = current_time - g_sleep_start_time;

    ZVUNUSED(unused);

    /*
     * Softdevice triggeres it's own ble interrupts, so in case of softdevice
     * present check only that device does not asserts.
     */
#if !defined SOFTDEVICE_PRESENT
    if (g_sleep_start_time != 0 && sleep_time >= TEST_MIN_SLEEP_TIME &&
            sleep_time <= TEST_MAX_SLEEP_TIME)
    {
        TRACE_MSG(TRACE_APP1, "Waking up test OK : %ld", (FMT__L, sleep_time));

        ZB_SCHEDULE_CALLBACK(trigger_steering, 0);
    }
    else
    {
        TRACE_MSG(TRACE_APP1, "Waking up test FAILED, sleep time %ld", (FMT__L, sleep_time));
    }
#else
    TRACE_MSG(TRACE_APP1, "Waking up test OK : %ld", (FMT__L, sleep_time));

    ZB_SCHEDULE_ALARM(trigger_steering, 0, TEST_WAKE_UP_SIGNAL_DELAY);
#endif  /* SOFTDEVICE_PRESENT */
}

static void test_stop_poll(zb_time_t ms)
{
    TRACE_MSG(TRACE_ZDO1, ">> test_stop_poll", (FMT__0));

#ifndef NCP_MODE_HOST
    ZDO_CTX().pim.poll_in_progress = 0;
    zb_zdo_pim_stop_poll(0);
    zb_zdo_pim_permit_turbo_poll(0); /* prohibit adaptive poll */
#else
    ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
#endif

    TRACE_MSG(TRACE_ZDO1, "<< test_stop_poll", (FMT__0));
}

#ifndef ZB_NSNG
static void test_gpio_init(zb_uint8_t unused)
{
    ret_code_t err_code;

    ZVUNUSED(unused);

    TRACE_MSG(TRACE_APP1, ">> test_gpio_init", (FMT__0));

    if (!nrf_drv_gpiote_is_init())
    {
        err_code = nrf_drv_gpiote_init();

        ZB_ASSERT(err_code == NRFX_SUCCESS);
    }

    nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
    in_config.pull = NRF_GPIO_PIN_PULLUP;
    err_code = nrf_drv_gpiote_in_init(PIN_IN, &in_config, rtp_int_03_zed_in_pin_handler);

    ZB_ASSERT(err_code == NRFX_SUCCESS);

    nrf_drv_gpiote_out_config_t out_config_deb = GPIOTE_CONFIG_OUT_SIMPLE(false);
    err_code = nrf_drv_gpiote_out_init(PIN_OUT_DEBUG, &out_config_deb);

    ZB_ASSERT(err_code == NRFX_SUCCESS);

    nrf_drv_gpiote_out_set(PIN_OUT_DEBUG);

    TRACE_MSG(TRACE_APP1, "<< test_gpio_init", (FMT__0));
}

void test_enable_external_int(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    nrf_drv_gpiote_in_event_enable(PIN_IN, true);
}

void rtp_int_03_zed_in_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    ZVUNUSED(pin);
    ZVUNUSED(action);
}

#else
static void test_gpio_init(zb_uint8_t unused)
{
    ZVUNUSED(unused);
}

void test_enable_external_int(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    TRACE_MSG(TRACE_APP1, "test_enable_external_int", (FMT__0));
}
#endif

/*! @} */
