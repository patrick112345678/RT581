/* ZBOSS Zigbee software protocol stack
 *
 * Copyright (c) 2012-2020 DSR Corporation, Denver CO, USA.
 * http://www.dsr-zboss.com
 * http://www.dsr-corporation.com
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
/* PURPOSE: Test sample with clusters Price, DRLC, Metering, Messaging and
    Tunneling
 */

#define ZB_TRACE_FILE_ID 51364
#include "zboss_api.h"
#include "smart_energy_zr.h"

/* Insert that include before any code or declaration. */
#ifdef ZB_CONFIGURABLE_MEM
#include "zb_mem_config_max.h"
#endif

zb_uint16_t g_dst_addr = 0x00;

#define DST_ADDR g_dst_addr
#define DST_ADDR_MODE ZB_APS_ADDR_MODE_16_ENDP_PRESENT
#define DST_EP 5
#define TUNNEL_DATA_SIZE 3

/**
 * Global variables definitions
 */
zb_ieee_addr_t g_zc_addr = {0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb}; /* IEEE address of the
                                                                              * device */
/* Used endpoint */
#define ZB_SERVER_ENDPOINT          5
#define ZB_CLIENT_ENDPOINT          6

void test_loop(zb_bufid_t param);

/**
 * Declaring attributes for each cluster
 */

/* Price cluster attributes (server) */
zb_uint8_t g_attr_srv_commodity_type = 0;
ZB_ZCL_DECLARE_PRICE_SRV_ATTRIB_LIST(price_srv_attr_list, &g_attr_srv_commodity_type);

/* Price cluster attributes (client) */
zb_uint8_t g_attr_inc_rand_min = 0;  /* Missing default value */
zb_uint8_t g_attr_dec_rand_min = 0; /* Missing default value */
zb_uint8_t g_attr_cli_commodity_type = 0;
ZB_ZCL_DECLARE_PRICE_CLI_ATTRIB_LIST(price_cli_attr_list, &g_attr_inc_rand_min,
                                     &g_attr_dec_rand_min, &g_attr_cli_commodity_type);

/* DLRC cluster attributes (client) */
zb_uint8_t g_attr_utility_enrollment_group = 0; /* Missing default value */
zb_uint8_t g_attr_start_randomization_minutes = 0; /* Missing default value */
zb_uint8_t g_attr_duration_randomization_minutes = 0; /* Missing default value */
zb_uint16_t g_attr_device_class = 0;
ZB_ZCL_DECLARE_DRLC_ATTRIB_LIST(drlc_attr_list, &g_attr_utility_enrollment_group, &g_attr_start_randomization_minutes,
                                &g_attr_duration_randomization_minutes, &g_attr_device_class);

/* Metering cluster attributes */
zb_uint48_t g_attr_curr_summ_delivered;
zb_uint8_t g_attr_curr_status = ZB_ZCL_METERING_STATUS_DEFAULT_VALUE;
zb_uint8_t g_attr_curr_unit_of_measure = ZB_ZCL_METERING_UNIT_OF_MEASURE_DEFAULT_VALUE;
zb_uint8_t g_attr_curr_summation_formatting = 0;
zb_uint8_t g_attr_curr_metering_device_type = 0;
zb_uint24_t g_attr_curr_instantaneous_demand; /* ZB_ZCL_METERING_INSTANTANEOUS_DEMAND_DEFAULT_VALUE doesn't work */
zb_uint8_t g_attr_curr_demand_formatting = 0;
zb_uint8_t g_attr_curr_historical_consumption_formatting = 0;
zb_uint24_t g_attr_curr_multiplier;
zb_uint24_t g_attr_curr_divisor;
ZB_ZCL_DECLARE_METERING_ATTRIB_LIST_EXT(metering_attr_list, &g_attr_curr_summ_delivered, &g_attr_curr_status,
                                        &g_attr_curr_unit_of_measure, &g_attr_curr_summation_formatting,
                                        &g_attr_curr_metering_device_type, &g_attr_curr_instantaneous_demand,
                                        &g_attr_curr_demand_formatting, &g_attr_curr_historical_consumption_formatting,
                                        &g_attr_curr_multiplier, &g_attr_curr_divisor);

/* Tunneling cluster attributes */
zb_uint16_t g_attr_close_tunnel_timeout = ZB_ZCL_TUNNELING_CLOSE_TUNNEL_TIMEOUT_DEFAULT_VALUE;
ZB_ZCL_DECLARE_TUNNELING_ATTRIB_LIST(tunneling_attr_list, &g_attr_close_tunnel_timeout);

/* Declare cluster list for the device */
ZB_DECLARE_SMART_ENERGY_SERVER_CLUSTER_LIST(smart_energy_server_clusters, price_srv_attr_list,
        metering_attr_list, tunneling_attr_list);

/* Declare server endpoint */
ZB_DECLARE_SMART_ENERGY_SERVER_EP(smart_energy_server_ep, ZB_SERVER_ENDPOINT, smart_energy_server_clusters);
ZB_DECLARE_SMART_ENERGY_CLIENT_CLUSTER_LIST(smart_energy_client_clusters, price_cli_attr_list, drlc_attr_list);

/* Declare client endpoint */
ZB_DECLARE_SMART_ENERGY_CLIENT_EP(smart_energy_client_ep, ZB_CLIENT_ENDPOINT, smart_energy_client_clusters);

/* Declare application's device context for single-endpoint device */
ZB_DECLARE_SMART_ENERGY_CTX(smart_energy_output_ctx, smart_energy_server_ep, smart_energy_client_ep);


MAIN()
{
    ARGV_UNUSED;

    /* Trace enable */
    ZB_SET_TRACE_ON();
    /* Traffic dump enable*/
    ZB_SET_TRAF_DUMP_ON();

    /* Global ZBOSS initialization */
    ZB_INIT("smart_energy_zr");

    /* Set up defaults for the commissioning */
    zb_set_long_address(g_zc_addr);
    zb_set_network_router_role(1l << 22);

    zb_set_nvram_erase_at_start(ZB_FALSE);
    zb_set_max_children(1);

    /* [af_register_device_context] */
    /* Register device ZCL context */
    ZB_AF_REGISTER_DEVICE_CTX(&smart_energy_output_ctx);

    /* Initiate the stack start without starting the commissioning */
    if (zboss_start_no_autostart() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
    }
    else
    {
        /* Call the main loop */
        zboss_main_loop();
    }

    /* Deinitialize trace */
    TRACE_DEINIT();

    MAIN_RETURN(0);
}

void test_loop(zb_bufid_t param)
{
    static zb_uint8_t test_step = 0;
    zb_zcl_price_get_scheduled_prices_payload_t pl1;
    zb_zcl_price_ack_payload_t pl2;
    zb_zcl_drlc_lce_payload_t pl3;
    zb_zcl_drlc_cancel_lce_payload_t pl4;
    zb_uint8_t pl5;
    zb_zcl_metering_get_profile_payload_t pl6 = { 0 };
    zb_zcl_metering_request_fast_poll_mode_payload_t pl7 = { 0 };
    zb_zcl_messaging_message_confirm_payload_t pl8 = { 0 };
    zb_uint8_t data[TUNNEL_DATA_SIZE] = {1, 2, 3};

    TRACE_MSG(TRACE_APP1, ">> test_loop test_step=%hd", (FMT__H, test_step));

    if (param == 0)
    {
        zb_buf_get_out_delayed(test_loop);
    }
    else
    {
        switch (test_step)
        {
        case 0:
            ZB_ZCL_PRICE_SEND_CMD_GET_CURRENT_PRICE(param, (zb_addr_u *)&DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT, ZB_TRUE);
            break;

        case 1:
            ZB_ZCL_PRICE_SEND_CMD_GET_SCHEDULED_PRICES(param, (zb_addr_u *)&DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT, &pl1);
            break;

        case 2:
            ZB_ZCL_PRICE_SEND_CMD_PRICE_ACK(param, (zb_addr_u *)&DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT, &pl2);
            break;

        case 3:
            ZB_ZCL_DRLC_SEND_CMD_LOAD_CONTROL_EVENT(param, (zb_addr_u *)&DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT, &pl3);
            break;

        case 4:
            ZB_ZCL_DRLC_SEND_CMD_CANCEL_LCE(param, (zb_addr_u *)&DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT, &pl4);
            break;

        case 5:
            ZB_ZCL_DRLC_SEND_CMD_CANCEL_ALL_LCE(param, (zb_addr_u *)&DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT, &pl5);
            break;

        case 6:
            ZB_ZCL_METERING_SEND_CMD_GET_PROFILE(param, (zb_addr_u *)&DST_ADDR, DST_ADDR_MODE, DST_EP,
                                                 ZB_CLIENT_ENDPOINT, &pl6, NULL);
            break;

        case 7:
            ZB_ZCL_METERING_SEND_CMD_REQUEST_FAST_POLL_MODE(param, (zb_addr_u *)&DST_ADDR, DST_ADDR_MODE, DST_EP,
                    ZB_CLIENT_ENDPOINT, &pl7, NULL);
            break;

        case 8:
            ZB_ZCL_MESSAGING_SEND_MSG_CONFIRMATION(param, (zb_addr_u *)&DST_ADDR, DST_ADDR_MODE, DST_EP,
                                                   ZB_CLIENT_ENDPOINT, &pl8);
            break;

        case 9:
            ZB_ZCL_MESSAGING_SEND_GET_LAST_MSG(param, (zb_addr_u *)&DST_ADDR, DST_ADDR_MODE, DST_EP,
                                               ZB_CLIENT_ENDPOINT);
            break;

        case 10:
            ZB_ZCL_TUNNELING_SEND_REQUEST_TUNNEL(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                                 ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL,
                                                 ZB_ZCL_TUNNELING_PROTOCOL_MANUFACTURER_DEFINED, 0, ZB_FALSE, 10);
            break;

        case 11:
            ZB_ZCL_TUNNELING_SEND_CLOSE_TUNNEL(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                               ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, 0);
            break;

        case 12:
            ZB_ZCL_TUNNELING_CLIENT_SEND_TRANSFER_DATA(param, ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                    ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, 0, TUNNEL_DATA_SIZE, data);

            break;

        case 13:
            ZB_ZCL_TUNNELING_SEND_TRANSFER_DATA_ERROR(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                    ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, 0, 0,
                    ZB_ZCL_TUNNELING_TRANSFER_DATA_STATUS_OK,
                    ZB_ZCL_TUNNELING_CLI_CMD_TRANSFER_DATA_ERROR,
                    ZB_ZCL_FRAME_DIRECTION_TO_SRV);
            break;
        }

        test_step++;

        if (test_step <= 13)
        {
            ZB_SCHEDULE_APP_ALARM(test_loop, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(50));
        }
    }

    TRACE_MSG(TRACE_APP1, "<< test_loop", (FMT__0));
}

/* Callback to handle the stack events */
void zboss_signal_handler(zb_uint8_t param)
{
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_SKIP_STARTUP:
            zboss_start_continue();
            break;

        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
            ZB_SCHEDULE_APP_ALARM(test_loop, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(50));
            /* Avoid freeing param */
            param = 0;
            break;

        default:
            TRACE_MSG(TRACE_APP1, "Unknown signal %d", (FMT__D, (zb_uint16_t)sig));
        }
    }
    else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
    {
        TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d sig %d", (FMT__D_D, ZB_GET_APP_SIGNAL_STATUS(param), sig));
    }

    /* Free the buffer if it is not used */
    if (param)
    {
        zb_buf_free(param);
    }
}
