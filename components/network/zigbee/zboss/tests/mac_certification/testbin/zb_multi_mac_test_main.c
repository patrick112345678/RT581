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


#define ZB_TRACE_FILE_ID 41706
#undef ZB_MULTI_TEST

#include "zb_common.h"

#include "zb_console_monitor.h"
#include "zb_command_parser.h"
#include "../common/zb_cert_test_globals.h"
#include "zb_osif.h"

#include "zb_multi_test_mem_conf.h"

#include "../mac_verify_version.h"
#include "../../platform_verify_version.h"

#ifdef UNIX
#include <string.h>
#include <ctype.h>
#endif  /* UNIX */

#if defined ZB_NRF_52 && defined SOFTDEVICE_PRESENT
#include "nrf_log_ctrl.h"
#include "ble_zigbee_cert_uart.h"
#endif /* defined ZB_NRF_52 && defined SOFTDEVICE_PRESENT */

#ifdef ZB_NRF_52

#define MAC_ACK_FRAME_DELIVERY_TESTS
#define MAC_ASSOCIATION_TESTS
#define MAC_DATA_TESTS
#define MAC_FRAME_VALIDATION_TESTS
#define MAC_RETRIES_TESTS
#define MAC_SCANNING_TESTS
#define MAC_WARM_START_TESTS

#define MAC_BEACON_MANAGEMENT_01
#define MAC_BEACON_MANAGEMENT_02
#define MAC_BEACON_MANAGEMENT_03

#define MAC_CHANNEL_ACCESS_01
#define MAC_CHANNEL_ACCESS_02

#ifdef SOFTDEVICE_PRESENT
#include "nrf_log_ctrl.h"
#include "ble_zigbee_cert_uart.h"
#endif /* SOFTDEVICE_PRESENT */

#elif defined ZB_TEST_GROUP_ALL

#define MAC_ACK_FRAME_DELIVERY_TESTS
#define MAC_ASSOCIATION_TESTS
#define MAC_BEACON_MANAGEMENT_TESTS

//#define MAC_CHANNEL_ACCESS_TESTS
/* Removed due to the usage of sub GHz API inside MAC_CHANNEL_ACCESS_03.
 * The following symbols were not defined in 2.4GHz build:
 * - ZB_TRANS_SEND_FRAME_SUB_GHZ
 * - MAC_ADD_FCS
 */
#define MAC_CHANNEL_ACCESS_TESTS
//#define MAC_CHANNEL_ACCESS_01
//#define MAC_CHANNEL_ACCESS_02
//#ifdef ZB_SUB_GHZ
//#define MAC_CHANNEL_ACCESS_03
//#endif /* ZB_SUB_GHZ */
//#define MAC_CHANNEL_ACCESS_04

#define MAC_DATA_TESTS
#define MAC_FRAME_VALIDATION_TESTS
#define MAC_RETRIES_TESTS
#define MAC_SCANNING_TESTS
#define MAC_WARM_START_TESTS

//#define PHY24_RECEIVE_TESTS
//#define PHY24_TRANSMIT_TESTS
//#define PHY24_TURNAROUND_TIME_TESTS

#endif /* ZB_TEST_GROUP_ALL */

#define USE_ZB_MLME_RESET_CONFIRM
#define USE_ZB_MLME_START_CONFIRM
#define USE_ZB_MCPS_DATA_CONFIRM
#define USE_ZB_MLME_BEACON_NOTIFY_INDICATION
#define USE_ZB_MLME_POLL_CONFIRM
#define USE_ZB_MLME_ORPHAN_INDICATION
#define USE_ZB_MLME_ASSOCIATE_CONFIRM
#define USE_ZB_MLME_ASSOCIATE_INDICATION
#define USE_ZB_MLME_SCAN_CONFIRM
#define USE_ZB_MCPS_DATA_INDICATION
#define USE_ZB_MLME_COMM_STATUS_INDICATION
#define USE_ZB_MLME_PURGE_CONFIRM
#define USE_ZB_MLME_DUTY_CYCLE_MODE_INDICATION
#include "zb_mac_only_stubs.h"

typedef struct zb_mac_test_table_s
{
    char *test_name;
    void (*main_p)();
    void (*associate_indication_p)(zb_uint8_t param);
    void (*associate_confirm_p)(zb_uint8_t param);
    void (*beacon_notify_indication_p)(zb_uint8_t param);
    void (*comm_status_indication_p)(zb_uint8_t param);
    void (*orphan_indication_p)(zb_uint8_t param);
    void (*reset_confirm_p)(zb_uint8_t param);
    void (*scan_confirm_p)(zb_uint8_t param);
    void (*start_confirm_p)(zb_uint8_t param);
    void (*poll_confirm_p)(zb_uint8_t param);
    void (*purge_confirm_p)(zb_uint8_t param);
    void (*data_indication_p)(zb_uint8_t param);
    void (*data_confirm_p)(zb_uint8_t param);
    void (*duty_cycle_mode_indication_p)(zb_uint8_t param);
}
zb_mac_test_table_t;

#include "mac_tests_table.h"

static zb_int_t s_test_i;

zb_int_t g_argc;
char **g_argv;

#define SUCCESS_MESSAGE_LENGTH 1
#define BEFORE_TEST_START_DELAY 500000
#define ERROR_MESSAGE_LENGTH 1
zb_uint8_t success[] = "s";
zb_uint8_t error[] = "e";

zb_bool_t lqi_test = 0;

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

static void version_information_for_testing(void)
{
    printf("Varify_Platform: %s\n", VERSION_Z_PLATFORM_VERIFY_STR);
    printf("MAC_Verify: %s\n", VERSION_Z_MAC_VERIFY_STR);
}

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

static void send_success_message()
{
    zb_console_monitor_send_data(success, SUCCESS_MESSAGE_LENGTH);

#if defined ZB_ASYNC_TRACE_CONTROL && defined ZB_SERIAL_FOR_TRACE
    /* TODO: To be moved out to application control etc. */
    zb_osif_serial_flush();
#endif
}

static void send_error_message()
{
    zb_console_monitor_send_data(error, ERROR_MESSAGE_LENGTH);

#if defined ZB_ASYNC_TRACE_CONTROL && defined ZB_SERIAL_FOR_TRACE
    /* TODO: To be moved out to application control etc. */
    zb_osif_serial_flush();
#endif
}

/* Run test by name*/
static void run_test(char *test_name)
{
    zb_uint_t i;

#if defined ZB_NRF_52 && defined SOFTDEVICE_PRESENT
    if (zb_multi_test_parse_ble_token(test_name))
    {
        zb_multi_test_start_ble();
        test_name[strlen(test_name) - 4] = '\0';
    }
#endif /* defined ZB_NRF_52 && defined SOFTDEVICE_PRESENT */

    for (i = 0 ;
            i < ZB_ARRAY_SIZE(s_mac_tests_table)
            && strcmp(s_mac_tests_table[i].test_name, test_name);
            ++i)
    {
    }
    if (i < ZB_ARRAY_SIZE(s_mac_tests_table))
    {
        send_success_message();

        s_test_i = i;
        zb_console_monitor_sleep(BEFORE_TEST_START_DELAY);

        zb_console_monitor_deinit();
        (*s_mac_tests_table[i].main_p)();
    }
    else
    {
        send_error_message();
    }
}

MAIN()
{
    char command_buffer[100], *command_ptr;
    char test_name[60];
    zb_bool_t res;

#ifdef UNIX
    g_argc = argc;
    g_argv = argv;
#endif

#if defined ZB_NRF_52 && defined SOFTDEVICE_PRESENT
    zb_multi_test_init_ble();
#endif /* defined ZB_NRF_52 && defined SOFTDEVICE_PRESENT */

    zb_console_monitor_init();
    version_information_for_testing();
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
        send_error_message();
    }
}

void zb_mlme_reset_confirm(zb_uint8_t param)
{
    if (s_mac_tests_table[s_test_i].reset_confirm_p != NULL)
    {
        (*s_mac_tests_table[s_test_i].reset_confirm_p)(param);
    }
    else
    {
        zb_buf_free(param);
    }
}


void zb_mlme_start_confirm(zb_uint8_t param)
{
    if (s_mac_tests_table[s_test_i].start_confirm_p != NULL)
    {
        (*s_mac_tests_table[s_test_i].start_confirm_p)(param);
    }
    else
    {
        zb_buf_free(param);
    }
}


void zb_mcps_data_confirm(zb_uint8_t param)
{
    if (s_mac_tests_table[s_test_i].data_confirm_p != NULL)
    {
        (*s_mac_tests_table[s_test_i].data_confirm_p)(param);
    }
    else
    {
        zb_buf_free(param);
    }
}


void zb_mlme_beacon_notify_indication(zb_uint8_t param)
{
    if (s_mac_tests_table[s_test_i].beacon_notify_indication_p != NULL)
    {
        (*s_mac_tests_table[s_test_i].beacon_notify_indication_p)(param);
    }
    else
    {
        zb_buf_free(param);
    }
}


void zb_mlme_poll_confirm(zb_uint8_t param)
{
    if (s_mac_tests_table[s_test_i].poll_confirm_p != NULL)
    {
        (*s_mac_tests_table[s_test_i].poll_confirm_p)(param);
    }
    else
    {
        zb_buf_free(param);
    }
}


void zb_mlme_orphan_indication(zb_uint8_t param)
{
    if (s_mac_tests_table[s_test_i].orphan_indication_p != NULL)
    {
        (*s_mac_tests_table[s_test_i].orphan_indication_p)(param);
    }
    else
    {
        zb_buf_free(param);
    }
}


void zb_mlme_associate_confirm(zb_uint8_t param)
{
    if (s_mac_tests_table[s_test_i].associate_confirm_p != NULL)
    {
        (*s_mac_tests_table[s_test_i].associate_confirm_p)(param);
    }
    else
    {
        zb_buf_free(param);
    }
}


void zb_mlme_associate_indication(zb_uint8_t param)
{
    if (s_mac_tests_table[s_test_i].associate_indication_p != NULL)
    {
        (*s_mac_tests_table[s_test_i].associate_indication_p)(param);
    }
    else
    {
        zb_buf_free(param);
    }
}


void zb_mlme_scan_confirm(zb_uint8_t param)
{
    if (s_mac_tests_table[s_test_i].scan_confirm_p != NULL)
    {
        (*s_mac_tests_table[s_test_i].scan_confirm_p)(param);
    }
    else
    {
        zb_buf_free(param);
    }
}


void zb_mcps_data_indication(zb_uint8_t param)
{
    if (s_mac_tests_table[s_test_i].data_indication_p != NULL)
    {
        (*s_mac_tests_table[s_test_i].data_indication_p)(param);
    }
    else
    {
        zb_buf_free(param);
    }
}


void zb_mlme_comm_status_indication(zb_uint8_t param)
{
    if (s_mac_tests_table[s_test_i].comm_status_indication_p != NULL)
    {
        (*s_mac_tests_table[s_test_i].comm_status_indication_p)(param);
    }
    else
    {
        zb_buf_free(param);
    }
}


void zb_mlme_purge_confirm(zb_uint8_t param)
{
    if (s_mac_tests_table[s_test_i].purge_confirm_p != NULL)
    {
        (*s_mac_tests_table[s_test_i].purge_confirm_p)(param);
    }
    else
    {
        zb_buf_free(param);
    }
}

void zb_mlme_duty_cycle_mode_indication(zb_uint8_t param)
{
    if (s_mac_tests_table[s_test_i].duty_cycle_mode_indication_p != NULL)
    {
        (*s_mac_tests_table[s_test_i].duty_cycle_mode_indication_p)(param);
    }
    else
    {
        zb_buf_free(param);
    }
}
