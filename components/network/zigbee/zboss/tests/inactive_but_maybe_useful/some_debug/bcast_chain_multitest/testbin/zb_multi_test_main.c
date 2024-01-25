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


#define ZB_TRACE_FILE_ID 40064
#undef ZB_MULTI_TEST
#include "zb_common.h"

#include "zb_console_monitor.h"
#include "zb_command_parser.h"
#include "../common/zb_cert_test_globals.h"
#include "zb_osif.h"

#if defined ZB_PLATFORM_NRF52 && defined SOFTDEVICE_PRESENT
#include "nrf_log_ctrl.h"
#include "ble_zigbee_cert_uart.h"
#endif /* defined ZB_PLATFORM_NRF52 && defined SOFTDEVICE_PRESENT */

#include "zb_multi_test_mem_conf.h"

typedef struct zb_test_table_s
{
    char *test_name;
    void (*main_p)();
    void (*startup_complete_p)(zb_uint8_t param);
}
zb_test_table_t;

#ifdef ZB_NSNG
#define ZB_TEST_GROUP_ZCP_R22_APS
#define ZB_TEST_GROUP_ZCP_R22_NWK
#define ZB_TEST_GROUP_ZCP_R22_PED
#define ZB_TEST_GROUP_ZCP_R22_PRO
#define ZB_TEST_GROUP_ZCP_R22_R20
#define ZB_TEST_GROUP_ZCP_R22_R21
#define ZB_TEST_GROUP_ZCP_R22_ZDO
#define ZB_TEST_GROUP_ZCP_R22_SEC
#define ZB_TEST_GROUP_ZCP_R22_BDB
#define ZB_TEST_GROUP_ZCP_R22_R22
#elif defined ZB_CONFIG_NRF52
#define ZB_TEST_GROUP_ZCP_R22_APS
#define ZB_TEST_GROUP_ZCP_R22_BDB
#define ZB_TEST_GROUP_ZCP_R22_NWK
#define ZB_TEST_GROUP_ZCP_R22_PED
#define ZB_TEST_GROUP_ZCP_R22_PRO
#define ZB_TEST_GROUP_ZCP_R22_R20
#define ZB_TEST_GROUP_ZCP_R22_R21
#define ZB_TEST_GROUP_ZCP_R22_R22
#define ZB_TEST_GROUP_ZCP_R22_SEC
#define ZB_TEST_GROUP_ZCP_R22_ZDO
#endif

#ifdef ZB_ED_ROLE
#include "zed_tests_table.h"
#else
#include "zc_tests_table.h"
#endif


static zb_int_t s_test_i;

zb_int_t g_argc;
char **g_argv;

#define SUCCESS_MESSAGE_LENGTH 1
#define ERROR_MESSAGE_LENGTH 1
#define BEFORE_TEST_START_DELAY 500000
zb_uint8_t success[] = "s";
zb_uint8_t error[] = "e";


#if defined ZB_PLATFORM_NRF52 && defined SOFTDEVICE_PRESENT
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
#endif /* defined ZB_PLATFORM_NRF52 && defined SOFTDEVICE_PRESENT */

static zb_bool_t command_parameter_handler(char *key, char *value)
{
    if (strcmp(key, "page") == 0)
    {
        /* TODO: add validation? */
        g_cert_test_ctx.page = (zb_uint8_t)atoi(value);
    }
    else if (strcmp(key, "channel") == 0)
    {
        /* TODO: add validation? */
        g_cert_test_ctx.channel = (zb_uint8_t)atoi(value);
    }

    return ZB_TRUE;
}

/* Run test by name*/
static void run_test(char *test_name)
{
    zb_uint_t i;

#if defined ZB_PLATFORM_NRF52 && defined SOFTDEVICE_PRESENT
    if (zb_multi_test_parse_ble_token(test_name))
    {
        zb_multi_test_start_ble();
        test_name[strlen(test_name) - 4] = '\0';
    }
#endif /* defined ZB_PLATFORM_NRF52 && defined SOFTDEVICE_PRESENT */

    for (i = 0 ;
            i < ZB_ARRAY_SIZE(s_tests_table)
            && strcmp(s_tests_table[i].test_name, test_name);
            ++i)
    {
    }

    if (i < ZB_ARRAY_SIZE(s_tests_table))
    {
        zb_console_monitor_send_data(success, SUCCESS_MESSAGE_LENGTH);

        s_test_i = i;
        zb_console_monitor_sleep(BEFORE_TEST_START_DELAY);

        zb_console_monitor_deinit();
        (*s_tests_table[i].main_p)();
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

#if defined ZB_PLATFORM_NRF52 && defined SOFTDEVICE_PRESENT
    zb_multi_test_init_ble();
#endif /* defined ZB_PLATFORM_NRF52 && defined SOFTDEVICE_PRESENT */

    zb_console_monitor_init();
    zb_console_monitor_get_cmd((zb_uint8_t *)command_buffer, sizeof(command_buffer));

    /* parsing the command: fetching test name */
    command_ptr = (char *)(&command_buffer);
    res = parse_command_token(&command_ptr, test_name, sizeof(test_name));
    if (res)
    {
        zb_cert_test_set_init_globals();
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
    (*s_tests_table[s_test_i].startup_complete_p)(param);
}


