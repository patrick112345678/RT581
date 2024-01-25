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
/*  PURPOSE: Main module for multi-test FW
*/

#undef ZB_MULTI_TEST
#define ZB_TRACE_FILE_ID 40361
#include "zb_common.h"

#include "zb_console_monitor.h"
#include "zb_command_parser.h"
#include "zb_test_step.h"
#include "zb_osif.h"

#if defined ZB_NRF_52 && defined SOFTDEVICE_PRESENT
#include "nrf_log_ctrl.h"
#include "ble_zigbee_cert_uart.h"
#endif /* defined ZB_NRF_52 && defined SOFTDEVICE_PRESENT */

#ifdef ZB_CONFIGURABLE_MEM

#if defined ZB_MEM_CONFIG_MIN
#include "zb_mem_config_min.h"
#elif defined ZB_MEM_CONFIG_MED
#include "zb_mem_config_med.h"
#elif defined ZB_MEM_CONFIG_MAX
#include "zb_mem_config_max.h"
#else
/* there are no *.h files for the default mem-config */
#endif

#endif /* ZB_CONFIGURABLE_MEM */

#include "zb_multitest.h"

static zb_int_t s_test_i;

zb_int_t g_argc;
char **g_argv;

#define SUCCESS_MESSAGE_LENGTH 1
#define ERROR_MESSAGE_LENGTH 1
#define BEFORE_TEST_START_DELAY 500000
zb_uint8_t success[] = "s";
zb_uint8_t error[] = "e";

#if defined ZB_NRF_52 && defined SOFTDEVICE_PRESENT
static void zb_multi_test_init_ble()
{
    /* Initialize BLE stack. */
    ble_zigbee_cert_uart_init();
}

static void zb_multi_test_start_ble()
{
    /* Start BLE stack */
    uint32_t err_code = ble_zigbee_cert_uart_start();
    APP_ERROR_CHECK(err_code);
}

zb_bool_t zb_multi_test_parse_ble_token(const char *str)
{
    zb_int_t len = strlen(str);
    if (len > 3 && !strcmp(&str[len - 4], "_BLE"))
    {
        return ZB_TRUE;
    }
    return ZB_FALSE;
}
#endif /* defined ZB_NRF_52 && defined SOFTDEVICE_PRESENT */

static zb_test_control_mode_t convert_mode_value(char *src)
{
    zb_test_control_mode_t ret = ZB_TEST_CONTROL_UNUSED;

    if (strcmp(src, "alarms") == 0)
    {
        ret = ZB_TEST_CONTROL_ALARMS;
    }
    else if (strcmp(src, "buttons") == 0)
    {
        ret = ZB_TEST_CONTROL_BUTTONS;
    }

    return ret;
}

static zb_bool_t command_parameter_handler(char *key, char *value)
{
    zb_test_control_mode_t used_mode;
    zb_test_ctx_t *test_ctx = zb_multitest_get_test_ctx();

    if (strcmp(key, "page") == 0)
    {
        /* TODO: add validation? */
        test_ctx->page = (zb_uint8_t)atoi(value);
    }
    else if (strcmp(key, "channel") == 0)
    {
        /* TODO: add validation? */
        test_ctx->channel = (zb_uint8_t)atoi(value);
    }
    else if (strcmp(key, "mode") == 0)
    {
        used_mode = convert_mode_value(value);

        if (used_mode != ZB_TEST_CONTROL_UNUSED)
        {
            test_ctx->mode = used_mode;
        }
    }

    return ZB_TRUE;
}

/* Run test by name*/
static void run_test(char *test_name)
{
    zb_uint_t i;
    const zb_test_table_t *tests_table = zb_multitest_get_tests_table();
    zb_uindex_t tests_table_size = zb_multitest_get_tests_table_size();

#if defined ZB_NRF_52 && defined SOFTDEVICE_PRESENT
    if (zb_multi_test_parse_ble_token(test_name))
    {
        zb_multi_test_start_ble();
        test_name[strlen(test_name) - 4] = '\0';
    }
#endif /* defined ZB_NRF_52 && defined SOFTDEVICE_PRESENT */

    for (i = 0 ; i < tests_table_size && strcmp(tests_table[i].test_name, test_name); i++)
    {
    }

    if (i < tests_table_size)
    {
        zb_console_monitor_send_data(success, SUCCESS_MESSAGE_LENGTH);

        s_test_i = i;
        zb_console_monitor_sleep(BEFORE_TEST_START_DELAY);

        zb_console_monitor_deinit();
        (*tests_table[i].main_p)();
    }
    else
    {
        zb_console_monitor_send_data(error, ERROR_MESSAGE_LENGTH);
    }
}

MAIN()
{
    char command_buffer[100], *command_ptr;
    char test_name[40];
    zb_bool_t res;

#ifdef UNIX
    g_argc = argc;
    g_argv = argv;
#endif

#if defined ZB_NRF_52 && defined SOFTDEVICE_PRESENT
    zb_multi_test_init_ble();
#endif /* defined ZB_NRF_52 && defined SOFTDEVICE_PRESENT */

    zb_console_monitor_init();
    zb_console_monitor_get_cmd((zb_uint8_t *)command_buffer, sizeof(command_buffer));

    /* parsing the command: fetching test name */
    command_ptr = (char *)(&command_buffer);
    res = parse_command_token(&command_ptr, test_name, sizeof(test_name));
    if (res)
    {
        zb_multitest_reset_test_ctx();
        zb_multitest_init();
        /* fetching parameters*/
        res = handle_command_parameter_list(command_ptr, command_parameter_handler);
    }

    if (res)
    {
        run_test(test_name);
    }
    else
    {
        zb_console_monitor_send_data(error, ERROR_MESSAGE_LENGTH);
    }
}


void zboss_signal_handler(zb_uint8_t param)
{
    const zb_test_table_t *tests_table = zb_multitest_get_tests_table();

    (*tests_table[s_test_i].startup_complete_p)(param);
}
