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
/* PURPOSE: Smart Plug device application
*/

#define GP_COMPONENT_ID GP_COMPONENT_ID_APP

#define ZB_TRACE_FILE_ID 40253

#include "sp_device.h"
#include "sp_hal_stub.h"

//#if ! defined ZB_ROUTER_ROLE
//#error define ZB_ROUTER_ROLE to compile smart plug device
//#endif

#ifdef ZB_ASSERT_SEND_NWK_REPORT
void assert_indication_cb(zb_uint16_t file_id, zb_int_t line_number);
#endif

/**
 * Declaration of Zigbee device data structures
 */

/* Global device context */
sp_device_ctx_t g_dev_ctx;

/**
 * Declaring attributes for each cluster
 */

/* Basic cluster attributes data initiated into global device context */
ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST_EXT(
    basic_attr_list,
    &g_dev_ctx.basic_attr.zcl_version,
    &g_dev_ctx.basic_attr.app_version,
    &g_dev_ctx.basic_attr.stack_version,
    &g_dev_ctx.basic_attr.hw_version,
    &g_dev_ctx.basic_attr.mf_name,
    &g_dev_ctx.basic_attr.model_id,
    &g_dev_ctx.basic_attr.date_code,
    &g_dev_ctx.basic_attr.power_source,
    &g_dev_ctx.basic_attr.location_id,
    &g_dev_ctx.basic_attr.ph_env,
    &g_dev_ctx.basic_attr.sw_build_id);

/* Identify cluster attributes data initiated into global device context */
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list,
                                    &g_dev_ctx.identify_attr.identify_time);

/* Metering cluster attributes data initiated into global device context */
ZB_ZCL_DECLARE_METERING_ATTRIB_LIST_EXT(metering_attr_list,
                                        &g_dev_ctx.metering_attr.curr_summ_delivered,
                                        &g_dev_ctx.metering_attr.status,
                                        &g_dev_ctx.metering_attr.unit_of_measure,
                                        &g_dev_ctx.metering_attr.summation_formatting,
                                        &g_dev_ctx.metering_attr.metering_device_type,
                                        &g_dev_ctx.metering_attr.instantaneous_demand,
                                        &g_dev_ctx.metering_attr.demand_formatting,
                                        &g_dev_ctx.metering_attr.historical_consumption_formatting,
                                        &g_dev_ctx.metering_attr.multiplier,
                                        &g_dev_ctx.metering_attr.divisor);


#ifdef ZB_ENABLE_ZLL
ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST_EXT(on_off_attr_list, &g_dev_ctx.on_off_attr.on_off,
                                      &g_dev_ctx.on_off_attr.global_scene_ctrl,
                                      &g_dev_ctx.on_off_attr.on_time,
                                      &g_dev_ctx.on_off_attr.off_wait_time);
#else
ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(on_off_attr_list, &g_dev_ctx.on_off_attr.on_off);
#endif

/* Groups cluster attributes data initiated into global device context */
ZB_ZCL_DECLARE_GROUPS_ATTRIB_LIST(groups_attr_list, &g_dev_ctx.groups_attr.name_support);

#ifdef SP_OTA
ZB_ZCL_DECLARE_OTA_UPGRADE_ATTRIB_LIST(ota_upgrade_attr_list,
                                       &g_dev_ctx.ota_attr.upgrade_server,
                                       &g_dev_ctx.ota_attr.file_offset,
                                       &g_dev_ctx.ota_attr.file_version,
                                       &g_dev_ctx.ota_attr.stack_version,
                                       &g_dev_ctx.ota_attr.downloaded_file_ver,
                                       &g_dev_ctx.ota_attr.downloaded_stack_ver,
                                       &g_dev_ctx.ota_attr.image_status,
                                       &g_dev_ctx.ota_attr.manufacturer,
                                       &g_dev_ctx.ota_attr.image_type,
                                       &g_dev_ctx.ota_attr.min_block_reque,
                                       &g_dev_ctx.ota_attr.image_stamp,
                                       &g_dev_ctx.ota_attr.server_addr,
                                       &g_dev_ctx.ota_attr.server_ep,
                                       SP_INIT_OTA_HW_VERSION,
                                       SP_OTA_IMAGE_BLOCK_DATA_SIZE_MAX,
                                       SP_OTA_UPGRADE_QUERY_TIMER_COUNTER);
#else
#define ota_upgrade_attr_list NULL
#endif

/* WWAH cluster attributes declaration*/
#ifdef SP_WWAH_COMPATIBLE
ZB_ZCL_DECLARE_WWAH_ATTRIB_LIST(wwah_attr_list);
#define DEBUG_REPORT_TABLE_SIZE 10
zb_zcl_wwah_debug_report_t debug_report_table[DEBUG_REPORT_TABLE_SIZE];
#else
zb_zcl_attr_t wwah_attr_list[] = { 0 };
#endif

#ifdef SP_CONTROL4_COMPATIBLE
/* Control4 cluster attributes data initiated into global device context */
ZB_ZCL_DECLARE_CONTROL4_NETWORKING_ATTRIB_LIST_SRV_EXT(c4_attr_list,
        &g_dev_ctx.c4_attr.device_type,
        &g_dev_ctx.c4_attr.announce_window,
        &g_dev_ctx.c4_attr.mtorr_period,
        g_dev_ctx.c4_attr.firmware_version,
        &g_dev_ctx.c4_attr.reflash_version,
        &g_dev_ctx.c4_attr.boot_count,
        g_dev_ctx.c4_attr.product_string,
        &g_dev_ctx.c4_attr.access_point_node_ID,
        &g_dev_ctx.c4_attr.access_point_long_ID,
        &g_dev_ctx.c4_attr.access_point_cost,
        &g_dev_ctx.c4_attr.mesh_channel,
        &g_dev_ctx.c4_attr.avg_rssi,
        &g_dev_ctx.c4_attr.avg_lqi,
        &g_dev_ctx.c4_attr.battery_level,
        &g_dev_ctx.c4_attr.radio_4_bars);
#endif /* SP_CONTROL4_COMPATIBLE */

/* Declare cluster list for a device */
SP_DECLARE_CLUSTER_LIST(
    sp_device_clusters,
    basic_attr_list,
    identify_attr_list,
    metering_attr_list,
    on_off_attr_list,
    groups_attr_list,
    wwah_attr_list,
    ota_upgrade_attr_list
);

/* Declare endpoint */
SP_DECLARE_EP(sp_device_ep, SP_ENDPOINT, sp_device_clusters);

/* Declare application's device context for selected endpoints */
#ifdef SP_CONTROL4_COMPATIBLE
ZB_ZCL_CONTROL4_NETWORK_DECLARE_CLUSTER_LIST(c4_network_srv_list, c4_attr_list);

ZB_ZCL_CONTROL4_NETWORK_DECLARE_EP(c4_srv_ep, c4_network_srv_list);

SP_DECLARE_CTX(sp_device_zcl_ctx, sp_device_ep, c4_srv_ep);
#else
SP_DECLARE_CTX(sp_device_zcl_ctx, sp_device_ep);
#endif /* SP_CONTROL4_COMPATIBLE */

/* OTA upgrade start handler */
#ifdef SP_OTA
void sp_ota_start_upgrade(zb_uint8_t param);
#endif

MAIN()
{
    ARGV_UNUSED;

    /* Trace enable */
    ZB_SET_TRACE_ON();
    /* Traffic dump enable */
    ZB_SET_TRAF_DUMP_ON();

    /* Global ZBOSS initialization */
#ifdef SP_CONTROL4_COMPATIBLE
    ZB_INIT("sp_c4_device");
#else
    ZB_INIT("sp_device");
#endif
    /* zb_enable_distributed(); */
    sp_device_app_init(0);

#ifdef ZB_ASSERT_SEND_NWK_REPORT
    zb_register_zboss_callback(ZB_ASSERT_INDICATION_CB, SET_ZBOSS_CB(assert_indication_cb));
#endif

    /*
      Why no zboss_start_xxx() call here.

      ZBOSS scheduler is initialized in ZB_INIT() and will work when
      zboss_main_loop() called.

      zboss_start_no_autostart() is scheduled from sp_hw_init().
      sp_hw_init() is called as an application init callback for alien MAC.

      For other MAC (including ZBOSS MAC at ns or HW)
      sp_platform_init() in sp_hal_stub.c calls sp_hw_init() directly.
     */
    sp_platform_init();

#ifdef ZB_ENABLE_ZGP_DIRECT
    zb_zgp_set_skip_gpdf(ZB_FALSE);
#endif /* ZB_ENABLE_ZGP_DIRECT */

    /* Call the main loop */
    zboss_main_loop();
    TRACE_DEINIT();

    MAIN_RETURN(0);
}

/* Callback which will be called on critical error (assert). */
#ifdef ZB_ASSERT_SEND_NWK_REPORT
void assert_indication_cb(zb_uint16_t file_id, zb_int_t line_number)
{
    TRACE_MSG(TRACE_ERROR, "assert_indication_cb", (FMT__0));
    /* Send special radio packet which contains encoded file id and line number of assert. */
    zb_nlme_send_assert_ind(file_id, line_number);
}
#endif

void sp_reset_delayed(zb_uint8_t param)
{
    ZVUNUSED(param);
    zb_reset(0);
}

zb_ret_t sp_hw_init(zb_uint8_t param)
{
    zb_ret_t ret = RET_OK;
    ZVUNUSED(param);

    TRACE_MSG(TRACE_APP1, ">> sp_hw_init", (FMT__0));

    ret = sp_hal_init();

    if (ret != RET_BUSY)
    {
        /* set On/Off switch to OFF state */
        sp_relay_on_off(ZB_FALSE);

        /* analyze start button state after HW init */
        if ( sp_get_button_state(SP_BUTTON_PIN) )
        {
            sp_update_button_state_ctx(1); /* remember in ctx that button was pressed (before reset) */
            TRACE_MSG(TRACE_ERROR, "button is pressed, RTFD!", (FMT__0));
#ifdef SP_SYNC_RTFD
            sp_hal_sync_rtfd();
#else
            ZB_SCHEDULE_APP_ALARM(sp_com_button_pressed_reset_fd, 0, SP_RESET_TO_FACTORY_DEFAULT_TIMEOUT);
#endif
        }
    }
    if (ret == RET_OK)
    {
        ZB_SCHEDULE_APP_CALLBACK(sp_start_device, 0);
    }
    else
    {
        g_dev_ctx.reset_device = ZB_TRUE;
        /* go failed blinking and then reset device */
        ZB_SCHEDULE_APP_CALLBACK(sp_start_operation_blink, SP_LED_FAILED);
    }

    TRACE_MSG(TRACE_APP1, "<< sp_hw_init", (FMT__0));
    return ret;
}

void sp_start_device(zb_uint8_t param)
{
    ZVUNUSED(param);
    TRACE_MSG(TRACE_APP1, ">> sp_start_device param %hd", (FMT__H, param));

    /* Function is called on device start up.
     - check if commissioned or not
     - if commissioned, rejoin the network
     - go to idle mode if not commissioned
    */
    /* Go to idle state  */
    g_dev_ctx.dev_state = SP_DEV_IDLE;

    zboss_start_no_autostart();

    TRACE_MSG(TRACE_APP1, "<< sp_start_device", (FMT__0));
}


void sp_clusters_attr_init(zb_uint8_t first_init)
{
#ifdef SP_OTA
    zb_ieee_addr_t default_ota_server_addr = SP_OTA_UPGRADE_SERVER;
#endif
#ifdef SP_CONTROL4_COMPATIBLE
    zb_ieee_addr_t default_ap_long_id = ZB_ZCL_CONTROL4_NETWORKING_ACCESS_POINT_LONG_ID_DEF_VALUE;
#endif

    TRACE_MSG(TRACE_APP1, ">> sp_clusters_attr_init first_init %hd", (FMT__H, first_init));

    if (first_init)
    {
        /* Basic cluster attributes data */
        g_dev_ctx.basic_attr.zcl_version  = ZB_ZCL_VERSION;
        g_dev_ctx.basic_attr.app_version = SP_INIT_BASIC_APP_VERSION;
        g_dev_ctx.basic_attr.stack_version = SP_INIT_BASIC_STACK_VERSION;
        g_dev_ctx.basic_attr.hw_version = SP_INIT_BASIC_HW_VERSION;

        ZB_ZCL_SET_STRING_VAL(
            g_dev_ctx.basic_attr.mf_name,
            SP_INIT_BASIC_MANUF_NAME,
            ZB_ZCL_STRING_CONST_SIZE(SP_INIT_BASIC_MANUF_NAME));

        ZB_ZCL_SET_STRING_VAL(
            g_dev_ctx.basic_attr.model_id,
            SP_INIT_BASIC_MODEL_ID,
            ZB_ZCL_STRING_CONST_SIZE(SP_INIT_BASIC_MODEL_ID));

        ZB_ZCL_SET_STRING_VAL(
            g_dev_ctx.basic_attr.date_code,
            SP_INIT_BASIC_DATE_CODE,
            ZB_ZCL_STRING_CONST_SIZE(SP_INIT_BASIC_DATE_CODE));


        g_dev_ctx.basic_attr.power_source = ZB_ZCL_BASIC_POWER_SOURCE_MAINS_SINGLE_PHASE;

        ZB_ZCL_SET_STRING_VAL(
            g_dev_ctx.basic_attr.location_id,
            SP_INIT_BASIC_LOCATION_ID,
            ZB_ZCL_STRING_CONST_SIZE(SP_INIT_BASIC_LOCATION_ID));


        g_dev_ctx.basic_attr.ph_env = SP_INIT_BASIC_PH_ENV;
        g_dev_ctx.ota_attr.manufacturer = SP_INIT_OTA_MANUFACTURER;

        ZB_ZCL_SET_STRING_LENGTH(g_dev_ctx.basic_attr.sw_build_id, 0);

#ifdef SP_CONTROL4_COMPATIBLE
        g_dev_ctx.c4_attr.device_type = ZB_ZCL_CONTROL4_NETWORKING_DEVICE_TYPE_END_DEVICE;
        g_dev_ctx.c4_attr.reflash_version = ZB_ZCL_CONTROL4_NETWORKING_REFLASH_VERSION_VENDOR_SPECIFIC;

        ZB_ZCL_SET_STRING_VAL(
            g_dev_ctx.c4_attr.firmware_version,
            SP_CONTROL4_FIRMWARE_VERSION,
            ZB_ZCL_STRING_CONST_SIZE(SP_CONTROL4_FIRMWARE_VERSION));

        ZB_ZCL_SET_STRING_VAL(
            g_dev_ctx.c4_attr.product_string,
            SP_CONTROL4_PRODUCT_STRING,
            ZB_ZCL_STRING_CONST_SIZE(SP_CONTROL4_PRODUCT_STRING));
#endif /* SP_CONTROL4_COMPATIBLE */
    }

    /* Identify cluster attributes data */
    g_dev_ctx.identify_attr.identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

    /* Metering cluster attributes data */
    ZB_ASSIGN_UINT48(g_dev_ctx.metering_attr.curr_summ_delivered, 0, 0);
    g_dev_ctx.metering_attr.status = SP_INIT_METERING_STATUS;
    g_dev_ctx.metering_attr.unit_of_measure = SP_INIT_METERING_UNIT_OF_MEASURE;
    g_dev_ctx.metering_attr.summation_formatting = SP_INIT_METERING_SUMMATION_FORMATTING;
    g_dev_ctx.metering_attr.metering_device_type = SP_INIT_METERING_METERING_DEVICE_TYPE;
    ZB_ASSIGN_INT24(g_dev_ctx.metering_attr.instantaneous_demand, 0, 0);

    ZB_ASSIGN_INT24(g_dev_ctx.metering_attr.multiplier, 0, SP_INIT_METERING_MULTIPLIER);
    ZB_ASSIGN_INT24(g_dev_ctx.metering_attr.divisor, 0, SP_INIT_METERING_DIVISOR);

    /* On/Off cluster attributes data */
    g_dev_ctx.on_off_attr.on_off = ZB_FALSE;
#ifdef ZB_ENABLE_ZLL
    g_dev_ctx.on_off_attr.global_scene_ctrl = ZB_TRUE;
    g_dev_ctx.on_off_attr.on_time = 0;
    g_dev_ctx.on_off_attr.off_wait_time = 0;
#endif

#ifdef SP_OTA
    /* OTA Upgrade client cluster attributes data */
    ZB_IEEE_ADDR_COPY(g_dev_ctx.ota_attr.upgrade_server, default_ota_server_addr);
    g_dev_ctx.ota_attr.file_offset = ZB_ZCL_OTA_UPGRADE_FILE_OFFSET_DEF_VALUE;
    g_dev_ctx.ota_attr.file_version = SP_INIT_OTA_FILE_VERSION;
    g_dev_ctx.ota_attr.stack_version = ZB_ZCL_OTA_UPGRADE_STACK_VERSION_DEF_VALUE;
    g_dev_ctx.ota_attr.downloaded_file_ver = ZB_ZCL_OTA_UPGRADE_DOWNLOADED_FILE_VERSION_DEF_VALUE;
    g_dev_ctx.ota_attr.downloaded_stack_ver = ZB_ZCL_OTA_UPGRADE_DOWNLOADED_STACK_DEF_VALUE;
    g_dev_ctx.ota_attr.image_status = ZB_ZCL_OTA_UPGRADE_IMAGE_STATUS_DEF_VALUE;
    g_dev_ctx.ota_attr.image_type = SP_INIT_OTA_IMAGE_TYPE;
    g_dev_ctx.ota_attr.min_block_reque = SP_INIT_OTA_MIN_BLOCK_REQUE;
    g_dev_ctx.ota_attr.image_stamp = SP_INIT_OTA_IMAGE_STAMP;
    g_dev_ctx.ota_attr.server_ep = ZB_ZCL_OTA_UPGRADE_SERVER_ENDPOINT_DEF_VALUE;
    g_dev_ctx.ota_attr.server_addr = ZB_ZCL_OTA_UPGRADE_SERVER_ADDR_DEF_VALUE;
#endif

#ifdef SP_WWAH_COMPATIBLE
    zb_zcl_wwah_init_server_attr();
#endif

#ifdef SP_CONTROL4_COMPATIBLE

    /* Reset ZAP information when leaving network */
    g_dev_ctx.c4_attr.announce_window = ZB_ZCL_CONTROL4_NETWORKING_ANNOUNCE_WINDOW_DEF_VALUE;
    g_dev_ctx.c4_attr.mtorr_period = ZB_ZCL_CONTROL4_NETWORKING_MTORR_PERIOD_DEF_VALUE;
    ZB_IEEE_ADDR_COPY(g_dev_ctx.c4_attr.access_point_long_ID, default_ap_long_id);
    g_dev_ctx.c4_attr.access_point_node_ID = ZB_ZCL_CONTROL4_NETWORKING_ACCESS_POINT_NODE_ID_DEF_VALUE;
    g_dev_ctx.c4_attr.access_point_cost = ZB_ZCL_CONTROL4_NETWORKING_ACCESS_POINT_COST_DEF_VALUE;
    g_dev_ctx.c4_attr.avg_rssi = ZB_MAC_RSSI_UNDEFINED;
    g_dev_ctx.c4_attr.avg_lqi = ZB_MAC_LQI_UNDEFINED;
    /* Percentage, range 0-100 */
    g_dev_ctx.c4_attr.battery_level = 100;
    /* Pseudo “4-bar” radio signal quality indicator, range 0-4 */
    g_dev_ctx.c4_attr.radio_4_bars = 4;

    /* Reset BOOT_COUNT and MESH_CHANNEL attributes when leaving network */
    g_dev_ctx.c4_attr.boot_count = ZB_ZCL_CONTROL4_NETWORKING_BOOT_COUNT_DEF_VALUE;
    g_dev_ctx.c4_attr.mesh_channel = ZB_ZCL_CONTROL4_NETWORKING_MESH_CHANNEL_MIN_VALUE;
#endif /* SP_CONTROL4_COMPATIBLE */

    g_dev_ctx.overcurrent_ma = SP_OVERCURRENT_THRESHOLD;
    g_dev_ctx.overvoltage_dv = SP_OVERVOLTAGE_THRESHOLD;

    TRACE_MSG(TRACE_APP1, "<< sp_clusters_attr_init", (FMT__0));
}

void sp_do_identify(zb_uint8_t param);

void sp_device_app_init(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> sp_device_app_init", (FMT__0));

    ZVUNUSED(param);

    /****************** Register Device ********************************/

#ifdef ZB_USE_NVRAM
    /* Register application callback for reading application data from NVRAM */
    zb_nvram_register_app1_read_cb(sp_nvram_read_app_data);
    /* Register application callback for writing application data to NVRAM */
    zb_nvram_register_app1_write_cb(sp_nvram_write_app_data, sp_get_nvram_data_size);
#endif

    /* Register device ZCL context */
    ZB_AF_REGISTER_DEVICE_CTX(&sp_device_zcl_ctx);
    /* Register cluster commands handler for a specific endpoint */
    ZB_AF_SET_ENDPOINT_HANDLER(SP_ENDPOINT, sp_zcl_cmd_handler);

    /********** SP device configuration **********/
    ZB_ZCL_REGISTER_DEVICE_CB(sp_device_interface_cb);
    /* Set identify notification handler for endpoint */
    ZB_AF_SET_IDENTIFY_NOTIFICATION_HANDLER(SP_ENDPOINT, sp_do_identify);

    sp_app_ctx_init();

    /* Init HA attributes */
    sp_clusters_attr_init(1);

#ifdef SP_MAC_ADDR
    /* Note: we set here fixed ieee address. This is just for test. In production
     * device must have its unique ieee address. */
    {
        zb_ieee_addr_t ieee_addr = SP_MAC_ADDR;
        zb_set_long_address(ieee_addr);
    }
#endif

    /* Set up defaults for the commissioning */
#ifdef ZB_ROUTER_ROLE
    zb_set_network_router_role(SP_DEFAULT_APS_CHANNEL_MASK);
    zb_set_max_children(5);
#else
    zb_set_network_ed_role(SP_DEFAULT_APS_CHANNEL_MASK);
#ifdef SP_CONTROL4_COMPATIBLE
    zb_permit_control4_network();
#endif /* SP_CONTROL4_COMPATIBLE */
#endif /* ZB_ROUTER_ROLE */
    /* Set NVRAM erase at start enabled/disabled */
    zb_set_nvram_erase_at_start(SP_NVRAM_ERASE_AT_START);
#ifdef SP_WWAH_COMPATIBLE
    zb_zcl_wwah_set_wwah_behavior(ZB_ZCL_WWAH_BEHAVIOR_SERVER);
#endif

    TRACE_MSG(TRACE_APP1, "<< sp_device_app_init", (FMT__0));
}

void sp_do_identify(zb_uint8_t param)
{
    sp_led_state_t led_state;

    TRACE_MSG(TRACE_APP1, "> sp_do_identify %hd", (FMT__H, param));

    ZVUNUSED(param);
    /* [AV] Q: Not sure if we should perform identify blinking when we're
        in comissioning state: if we behave such way all finding & binding
        turns into identifying illumination
       [VS] A: let's keep current implementation. If user experience suffers because of it, we'll change it.
    */
    if (param == 1)
    {
        /* start identifying */
        led_state = SP_LED_IDENTIFY;
    }
    else
    {
        /* return led to the previous mode */
        switch (g_dev_ctx.dev_state)
        {
        case SP_DEV_IDLE:
            led_state = SP_LED_IDLE_MODE;
            break;

        case SP_DEV_NORMAL:
            led_state = SP_LED_NORMAL;
            break;

        case SP_DEV_COMMISSIONING:
            led_state =  SP_LED_PROGRESS;
            break;

        default:
            led_state = SP_LED_FAILED; /* impossible case */
        }
    }

    ZB_SCHEDULE_APP_CALLBACK(sp_start_operation_blink, led_state);
    TRACE_MSG(TRACE_APP1, "< sp_do_identify %hd", (FMT__H, param));
}

void sp_leave_indication(zb_uint8_t param)
{
#ifdef SP_ENABLE_CLEAR_ATTR_BEFORE_LEAVE
    zb_uint8_t new_on_off;
#endif

    TRACE_MSG(TRACE_APP1, ">> sp_leave_indication param %hd", (FMT__H, param));



    if (param == ZB_NWK_LEAVE_TYPE_RESET)
    {
#ifdef SP_OTA
        zb_zcl_ota_upgrade_stop_client();
#endif

#ifdef SP_CONTROL4_COMPATIBLE
        zb_zcl_control4_network_cluster_stop();
        g_dev_ctx.is_on_c4_network = ZB_FALSE;
#endif /* SP_CONTROL4_COMPATIBLE */

#ifdef SP_ENABLE_CLEAR_ATTR_BEFORE_LEAVE
        sp_clusters_attr_init(0);

        sp_relay_on_off((zb_bool_t )0);

        new_on_off = ZB_FALSE;

        ZB_ZCL_SET_ATTRIBUTE(SP_ENDPOINT, ZB_ZCL_CLUSTER_ID_ON_OFF, ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID, &new_on_off, ZB_FALSE);

        sp_write_app_data(0);
        /* If we fail, trace is given and assertion is triggered */
        (void)zb_nvram_write_dataset(ZB_NVRAM_HA_DATA);
        (void)zb_nvram_write_dataset(ZB_NVRAM_ZCL_REPORTING_DATA);
#else
        /* Persist application data into NVRAM */
        sp_write_app_data(0);
#endif

        if (!g_dev_ctx.reset_device && g_dev_ctx.led_state != SP_LED_PROGRESS)
        {
            ZB_SCHEDULE_APP_CALLBACK(sp_start_operation_blink, SP_LED_IDLE_MODE);
        }
    }

    TRACE_MSG(TRACE_APP1, "<< sp_leave_indication", (FMT__0));
}

#ifdef SP_WWAH_COMPATIBLE
/*Fill Debug Report Table with default values
  Fill #4 Debug Report for testing puprose */
void setup_debug_report()
{
    ZB_ASSERT(DEBUG_REPORT_TABLE_SIZE > 0 && DEBUG_REPORT_TABLE_SIZE <= 0xFE);
    zb_uindex_t i;
    zb_char_t *debug_report_message = "Issue #4";
    zb_uint8_t report_id = 4;
    ZB_ZCL_SET_ATTRIBUTE(SP_ENDPOINT, ZB_ZCL_CLUSTER_ID_WWAH, ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_WWAH_CURRENT_DEBUG_REPORT_ID_ID, &report_id, ZB_FALSE);
    debug_report_table[0] = (zb_zcl_wwah_debug_report_t)
    {
        report_id, strlen(debug_report_message), debug_report_message
    };
    for (i = 1; i < DEBUG_REPORT_TABLE_SIZE; ++i)
    {
        debug_report_table[i] = ZB_ZCL_WWAH_DEBUG_REPORT_FREE_RECORD;
    }
}

void sp_process_wwah_debug_report_query_cb(zb_uint8_t param)
{
    zb_zcl_device_callback_param_t *device_cb_param = ZB_BUF_GET_PARAM(param, zb_zcl_device_callback_param_t);
    const zb_uint8_t *report_id = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_uint8_t);
    ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_NOT_FOUND;
    for (zb_uint8_t i = 0; i <= DEBUG_REPORT_TABLE_SIZE; ++i)
    {
        if (debug_report_table[i].report_id == *report_id)
        {
            ZB_ZCL_DEVICE_CMD_PARAM_OUT_SET(param, &debug_report_table[i]);
            ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
            TRACE_MSG(TRACE_APP1, "debug report %d found", (FMT__D, *report_id));
            break;
        }
    }
    TRACE_MSG(TRACE_APP1, "<< sp_process_wwah_debug_report_query_cb", (FMT__0));
}
#endif /* SP_WWAH_COMPATIBLE */


/**
 * Callback to handle the stack events
 */
void zboss_signal_handler(zb_uint8_t param)
{
    zb_zdo_app_signal_hdr_t *sg_p = NULL;
    /* Get application signal from the buffer */
    zb_zdo_app_signal_t sig = zb_get_app_signal(param, &sg_p);

    TRACE_MSG(TRACE_APP1, "zboss_signal_handler: status %hd signal %hd",
              (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        /* [signal_skip_startup] */
        case ZB_ZDO_SIGNAL_SKIP_STARTUP:
#ifndef ZB_MACSPLIT_HOST
            TRACE_MSG(TRACE_APP1, "ZB_ZDO_SIGNAL_SKIP_STARTUP: start commissioning", (FMT__0));
            if (zb_bdb_is_factory_new())
            {
                ZB_SCHEDULE_APP_ALARM_CANCEL(sp_com_button_pressed_reset_fd, ZB_ALARM_ANY_PARAM);
            }

            /* NVRAM is loaded, init relay state */
            {
                zb_zcl_attr_t *attr_desc_on_off = zb_zcl_get_attr_desc_a(SP_ENDPOINT, ZB_ZCL_CLUSTER_ID_ON_OFF,
                                                  ZB_ZCL_CLUSTER_SERVER_ROLE,
                                                  ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID);

                sp_relay_on_off((zb_bool_t )ZB_ZCL_GET_ATTRIBUTE_VAL_8(attr_desc_on_off));

#ifdef SP_CONTROL4_COMPATIBLE
                if (!zb_bdb_is_factory_new())
                {
                    /* Increment BOOT_COUNT attribute */
                    g_dev_ctx.c4_attr.boot_count++;
                    ZB_SCHEDULE_APP_CALLBACK(sp_write_app_data, 0);
                }
#endif /* SP_CONTROL4_COMPATIBLE */
            }
            /* Have zb_bdb_is_factory_new() check inside */
            ZB_SCHEDULE_APP_CALLBACK(sp_start_join, SP_FIRST_JOIN_ATTEMPT);
#endif /* ZB_MACSPLIT_HOST */
            break;
#ifdef ZB_MACSPLIT_HOST
        case ZB_MACSPLIT_DEVICE_BOOT:
            TRACE_MSG(TRACE_APP1, "ZB_MACSPLIT_DEVICE_BOOT: start commissioning", (FMT__0));
            if (zb_bdb_is_factory_new())
            {
                ZB_SCHEDULE_APP_ALARM_CANCEL(sp_com_button_pressed_reset_fd, ZB_ALARM_ANY_PARAM);
            }

            /* NVRAM is loaded, init relay state */
            {
                zb_zcl_attr_t *attr_desc_on_off = zb_zcl_get_attr_desc_a(SP_ENDPOINT, ZB_ZCL_CLUSTER_ID_ON_OFF,
                                                  ZB_ZCL_CLUSTER_SERVER_ROLE,
                                                  ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID);

                sp_relay_on_off((zb_bool_t )ZB_ZCL_GET_ATTRIBUTE_VAL_8(attr_desc_on_off));

#ifdef SP_CONTROL4_COMPATIBLE
                if (!zb_bdb_is_factory_new())
                {
                    /* Increment BOOT_COUNT attribute */
                    g_dev_ctx.c4_attr.boot_count++;
                    ZB_SCHEDULE_APP_CALLBACK(sp_write_app_data, 0);
                }
#endif /* SP_CONTROL4_COMPATIBLE */
            }
            /* Have zb_bdb_is_factory_new() check inside */
            ZB_SCHEDULE_APP_CALLBACK(sp_start_join, SP_FIRST_JOIN_ATTEMPT);
            break;
#endif /* ZB_MACSPLIT_HOST */
        /* [signal_skip_startup] */
        case ZB_BDB_SIGNAL_STEERING:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        {
            TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
#ifdef ZB_REJOIN_BACKOFF
            zb_zdo_rejoin_backoff_cancel();
#endif
            /* Set Operational state in CommissionState attribute */
#ifdef SP_OTA
            zb_buf_get_in_delayed(sp_ota_start_upgrade);
#endif
#ifdef SP_WWAH_COMPATIBLE
            zb_zcl_wwah_init_server();
            setup_debug_report();
            zb_enable_auto_pan_id_conflict_resolution(ZB_FALSE);
#endif

#ifdef SP_CONTROL4_COMPATIBLE
            /* Init MESH_CHANNEL attribute */
            g_dev_ctx.c4_attr.mesh_channel = ZB_PIBCACHE_CURRENT_CHANNEL();

            /* Restart Control4 Network Cluster logic */
            zb_zcl_control4_network_cluster_stop();

            if (zb_zcl_control4_network_cluster_start() != RET_OK)
            {
                TRACE_MSG(TRACE_ERROR, "Failed to start Control4 Network Cluster", (FMT__0));
            }
#endif /* SP_CONTROL4_COMPATIBLE */

            /* check, if joined, go to Normal (-restricted) mode */
            if (ZB_JOINED())
            {
                g_dev_ctx.dev_state = SP_DEV_NORMAL;
                /* Currently steering is not needed, switch to normal mode. */
                ZB_SCHEDULE_APP_ALARM_CANCEL(sp_start_operation_blink, ZB_ALARM_ANY_PARAM);
                ZB_SCHEDULE_APP_CALLBACK(sp_start_operation_blink, (sig == ZB_BDB_SIGNAL_DEVICE_FIRST_START) ? SP_LED_SUCCESS : SP_LED_NORMAL);
            }
        }
        break;

        case ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED:
        case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:
            TRACE_MSG(TRACE_APP1, "f&b signal", (FMT__0));
            break;

        case ZB_ZDO_SIGNAL_LEAVE:
        {
            zb_zdo_signal_leave_params_t *leave_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_leave_params_t);
            sp_leave_indication(leave_params->leave_type);
        }
        break;

        case ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY:
        {
            /* application data should be in the buf */
            if (zb_buf_len(param) > sizeof(zb_zdo_app_signal_hdr_t))
            {
                sp_production_config_t *prod_cfg = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, sp_production_config_t);
                TRACE_MSG(TRACE_APP1, "Loading application production config", (FMT__0));
                if (prod_cfg->version >= 1)
                {
                    ZB_ZCL_SET_STRING_VAL(g_dev_ctx.basic_attr.mf_name, prod_cfg->manuf_name, ZB_ZCL_STRING_CONST_SIZE(prod_cfg->manuf_name));
                    ZB_ZCL_SET_STRING_VAL(g_dev_ctx.basic_attr.model_id, prod_cfg->model_id, ZB_ZCL_STRING_CONST_SIZE(prod_cfg->model_id));
                    g_dev_ctx.ota_attr.manufacturer = prod_cfg->manuf_code;
                    zb_set_node_descriptor_manufacturer_code_req(prod_cfg->manuf_code, NULL);
                }
                if (prod_cfg->version >= 2)
                {
                    g_dev_ctx.overcurrent_ma = prod_cfg->overcurrent_ma;
                    g_dev_ctx.overvoltage_dv = prod_cfg->overvoltage_dv;
                }
            }

            break;
        }

        default:
            TRACE_MSG(TRACE_APP1, "Unknown signal %hd", (FMT__H, sig));
        }
    }
    else if (sig != ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
    {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));

        if (sig != ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
        {
            if (!zb_bdb_is_factory_new())
            {
#ifdef SP_CONTROL4_COMPATIBLE
                zb_zcl_control4_network_cluster_stop();
#endif /* SP_CONTROL4_COMPATIBLE */
#ifdef ZB_REJOIN_BACKOFF
                if (zb_zdo_rejoin_backoff_is_running())
                {
                    ZB_SCHEDULE_APP_CALLBACK(zb_zdo_rejoin_backoff_continue, 0);
                }
                else
                {
                    /* start Rejoin backoff: try to rejoin network with timeout
                     * 2-4-8-16-32-... seconds; maximum timeout is 30 minutes */
                    zb_zdo_rejoin_backoff_start(ZB_FALSE);
                }
#endif
            }
            else
            {
                /* Device is factory new - retry join */
                if (ZB_JOINED())
                {
                    sp_com_button_pressed_reset_fd(0);
                }
                else
                {
#ifdef ZB_REJOIN_BACKOFF
                    /* make sure rejoin backoff context is cleaned */
                    zb_zdo_rejoin_backoff_cancel();
                    sp_retry_join();
#endif
                }
            }
        }
    }

    if (param)
    {
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_APP1, "< zboss_signal_handler", (FMT__0));
}

void sp_retry_join()
{
    /*
      - if error joining to network appeared, wait 5 sec and repeat joining
      - if IZS_JOIN_LIMIT is exceeded - stop trying rejoins and go
      sleep until some event happens (tamper button is pressed, movement, etc)
      if the device was enrolled and then lost network, rejoin backoff algorithm starts
      rejoin_backoff
    */
    TRACE_MSG(TRACE_APP1, "join_counter %hd", (FMT__H, g_dev_ctx.join_counter));
    if (g_dev_ctx.join_counter < SP_JOIN_LIMIT)
    {
        ZB_SCHEDULE_APP_ALARM(sp_start_join, SP_RETRY_JOIN_ATTEMPT, ZB_TIME_ONE_SECOND * 5);
    }
    else
    {
        TRACE_MSG(TRACE_APP1, "out of join attempts, go to sleep", (FMT__0));
        g_dev_ctx.join_counter = 0;
        sp_start_operation_blink(SP_LED_FAILED);
    }
}

void sp_start_join(zb_uint8_t param)
{
    /*
      - scan for network
      - if network is not found, wait for 5 seconds and try again
      - repeat network searching SP_JOIN_LIMIT times
    */
    TRACE_MSG(TRACE_APP1, ">> sp_start_join param %hd", (FMT__H, param));

    if (!ZB_JOINED())
    {
        if (param == SP_FIRST_JOIN_ATTEMPT)
        {
            g_dev_ctx.join_counter = 0;
        }
        else
        {
            g_dev_ctx.join_counter++;
        }

        ZB_SCHEDULE_APP_CALLBACK(sp_start_bdb_commissioning, 0);
    }

    TRACE_MSG(TRACE_APP1, "<< sp_start_join", (FMT__0));
}

#ifdef SP_ENABLE_NWK_STEERING_BUTTON
void sp_nwk_steering(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> sp_nwk_steering", (FMT__0));

    bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);

    TRACE_MSG(TRACE_APP1, "<< sp_nwk_steering", (FMT__0));
}
#endif

zb_uint8_t sp_zcl_cmd_handler(zb_uint8_t param)
{
    zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);
    zb_uint8_t cmd_processed = ZB_FALSE;

    TRACE_MSG(TRACE_APP1, "> sp_zcl_cmd_handler %i", (FMT__H, param));
    ZB_ZCL_DEBUG_DUMP_HEADER(cmd_info);
    TRACE_MSG(TRACE_APP1, "payload size: %i", (FMT__D, zb_buf_len(param)));

    // Place for custom ZCL command handlers
    if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_SRV)
    {
        /* Do some processing if needed */
    }

    TRACE_MSG(TRACE_APP1, "<< sp_zcl_cmd_handler processed %hd", (FMT__H, cmd_processed));
    return cmd_processed;
}

void sp_basic_reset_to_defaults_cb(zb_uint8_t param)
{
    zb_uint8_t new_on_off;
    (void)param;
    TRACE_MSG(TRACE_APP1, ">> sp_basic_reset_to_defaults_cb", (FMT__0));

    ZB_SCHEDULE_APP_CALLBACK(sp_start_operation_blink, SP_LED_RESET_FD);

    sp_clusters_attr_init(0);
#ifdef SP_WWAH_COMPATIBLE
    setup_debug_report();
#endif
    sp_relay_on_off((zb_bool_t)0);

    new_on_off = ZB_FALSE;

    ZB_ZCL_SET_ATTRIBUTE(SP_ENDPOINT, ZB_ZCL_CLUSTER_ID_ON_OFF, ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID, &new_on_off, ZB_FALSE);

    sp_write_app_data(0);
#ifdef ZB_USE_NVRAM
    /* If we fail, trace is given and assertion is triggered */
    (void)zb_nvram_write_dataset(ZB_NVRAM_HA_DATA);
    (void)zb_nvram_write_dataset(ZB_NVRAM_ZCL_REPORTING_DATA);
#endif

    TRACE_MSG(TRACE_APP1, "<< sp_basic_reset_to_defaults", (FMT__0));
}


void sp_app_ctx_init()
{
    TRACE_MSG(TRACE_APP1, ">> sp_app_ctx_init", (FMT__0));

    ZB_BZERO(&g_dev_ctx, sizeof(g_dev_ctx));

    TRACE_MSG(TRACE_APP1, "<< sp_app_ctx_init", (FMT__0));
}

void sp_critical_error()
{
    TRACE_MSG(TRACE_APP1, ">> sp_critical_error", (FMT__0));

    /* Indicate h/w failure to user Restart App... */
    ZB_SCHEDULE_APP_CALLBACK(sp_start_operation_blink, SP_LED_CRITICAL_ERROR);

    TRACE_MSG(TRACE_APP1, "<< sp_critical_error", (FMT__0));
}

#ifdef SP_OTA
void sp_ota_start_upgrade(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, "> sp_ota_init_upgrade", (FMT__0));

    ZB_SCHEDULE_APP_ALARM(zb_zcl_ota_upgrade_init_client, param, 15 * ZB_TIME_ONE_SECOND);

    TRACE_MSG(TRACE_APP1, "< sp_ota_init_upgrade", (FMT__0));
}

#ifdef SP_CONTROL4_COMPATIBLE
void sp_ota_upgrade_client_start_manually(zb_uint8_t param)
{
    ZVUNUSED(param);

    if (zb_zcl_control4_network_cluster_get_state() == ZB_ZCL_CONTROL4_NETWORK_STATE_RUNNING)
    {
        /* Manually start OTA Upgrade client */
        zb_ret_t ret = zb_zcl_ota_upgrade_start_client(SP_ENDPOINT, g_dev_ctx.c4_attr.access_point_node_ID);

        if (ret == RET_OK)
        {
            g_dev_ctx.ota_ctx.is_started_manually = ZB_TRUE;
        }
        else
        {
            TRACE_MSG(TRACE_APP1, "sp_ota_upgrade_client_start_manually failed", (FMT__0));
        }
    }
}
#endif /* SP_CONTROL4_COMPATIBLE */

void sp_ota_upgrade_server_not_found(void)
{
    TRACE_MSG(TRACE_APP1, "sp_ota_upgrade_server_not_found", (FMT__0));

#ifdef SP_CONTROL4_COMPATIBLE
    ZB_SCHEDULE_APP_CALLBACK(sp_ota_upgrade_client_start_manually, 0);
#endif /* SP_CONTROL4_COMPATIBLE */
}
#endif /* SP_OTA */

void sp_leave_nwk_cb(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, "> sp_leave_nwk_cb param %hd", (FMT__H, param));

    zb_buf_free(param);

    /* Do HW reset after manual RTFD (by button) - such behaviour is not mandatory, it is possible to do silent
     * leave or restart join etc. */
    /* Do not reset immediately - wait for indication complete. */
    ZB_SCHEDULE_APP_ALARM(sp_reset_delayed, 0, ZB_TIME_ONE_SECOND * 3);

    TRACE_MSG(TRACE_APP1, "< sp_leave_nwk_cb", (FMT__0));
}


/* Perform local operation - leave network */
void sp_leave_nwk(zb_uint8_t param)
{
    zb_zdo_mgmt_leave_param_t *req_param;

    req_param = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_leave_param_t);
    ZB_BZERO(req_param, sizeof(zb_zdo_mgmt_leave_param_t));

    /* Set dst_addr == local address for local leave */
    req_param->dst_addr = ZB_PIBCACHE_NETWORK_ADDRESS();
    zdo_mgmt_leave_req(param, sp_leave_nwk_cb);
}


/* Perform start reset to factory defaults (LED blinks) on button push */
void sp_com_button_pressed_reset_fd(zb_uint8_t param)
{
    ZVUNUSED(param);
    TRACE_MSG(TRACE_APP1, "> sp_com_button_pressed_reset_fd", (FMT__0));

    g_dev_ctx.button_changed_count = SP_BUTTON_NOT_PRESSED;
    g_dev_ctx.reset_device = ZB_TRUE;

    sp_basic_reset_to_defaults_cb(0);

    zb_buf_get_out_delayed(sp_leave_nwk);

    TRACE_MSG(TRACE_APP1, "< sp_com_button_pressed_reset_fd", (FMT__0));
}


void sp_start_bdb_commissioning(zb_uint8_t param)
{
    zb_ret_t ret = RET_OK;

    ZVUNUSED(param);

    TRACE_MSG(TRACE_APP1, ">> sp_start_bdb_commissioning", (FMT__0));

    if (!ZB_JOINED())
    {
        bdb_start_top_level_commissioning(zb_bdb_is_factory_new() ? (zb_uint8_t )ZB_BDB_NETWORK_STEERING : 0);

        if (ret == RET_OK)
        {
            /* comissioning started successfully - start blinking */
            sp_start_operation_blink(SP_LED_PROGRESS);
        }
        else
        {
            /* commissioning failed, show failed and reboot... */
            ZB_SCHEDULE_APP_CALLBACK(sp_start_operation_blink, SP_LED_FAILED);
        }
    }
    TRACE_MSG(TRACE_APP1, "<< sp_start_bdb_commissioning", (FMT__0));
}


void sp_operation_led_off(zb_uint8_t param)
{
    ZVUNUSED(param);
    SP_LED_OFF();
}

/* LED loop:
 * - make Led on
 * - schedule Led off
 * - schedule next Led loop
 * Timeouts Led off & Led next loop correspondent g_dev_ctx.led_state
 * param - blinking count, incremented on each call (if number should be limited) */
void sp_operation_led_loop(zb_uint8_t param)
{
    switch (g_dev_ctx.led_state)
    {
    case SP_LED_SUCCESS:
        /* do success blinks */
        SP_LED_ON();
        ZB_SCHEDULE_APP_ALARM(sp_operation_led_off, SP_OPERATIONAL_LED, SP_LED_LONG_TIMEOUT);

        /* call sp_start_operation_blink() on finish */
        /* set correct mode for blinking */
        ZB_SCHEDULE_APP_ALARM(sp_start_operation_blink,
                              SP_LED_NORMAL,
                              SP_LED_SHORT_TIMEOUT + SP_LED_LONG_TIMEOUT);
        break;

    case SP_LED_FAILED:
        if (param < SP_LED_FALED_CONUT)
        {
            /* do failed blinks */
            SP_LED_ON();
            ZB_SCHEDULE_APP_ALARM(sp_operation_led_off, SP_OPERATIONAL_LED, SP_LED_SHORT_TIMEOUT);
            ZB_SCHEDULE_APP_ALARM(sp_operation_led_loop, param + 1, 2 * SP_LED_SHORT_TIMEOUT);
        }
        else
        {
            /* call sp_start_operation_blink() or reset on finish */
            if (g_dev_ctx.reset_device)
            {
                /* reset means physical reset - not reset to defaults, not nwk leave */
                zb_reset(0);
            }
            else
            {
                ZB_SCHEDULE_APP_ALARM(sp_start_operation_blink, (ZB_JOINED() ? (zb_uint8_t )SP_LED_NORMAL : (zb_uint8_t )SP_LED_IDLE_MODE), SP_LED_LONG_TIMEOUT + SP_LED_SHORT_TIMEOUT);
            }
        }
        break;

    case SP_LED_PROGRESS:
        SP_LED_ON();
        ZB_SCHEDULE_APP_ALARM(sp_operation_led_off, SP_OPERATIONAL_LED, SP_LED_PROGRESS_TIMEOUT);
        ZB_SCHEDULE_APP_ALARM(sp_operation_led_loop, 0, 2 * SP_LED_PROGRESS_TIMEOUT);
        break;

    case SP_LED_IDLE_MODE:
        if (param < SP_LED_IDLE_MODE_COUNT)
        {
            SP_LED_ON();
            ZB_SCHEDULE_APP_ALARM(sp_operation_led_off, SP_OPERATIONAL_LED, SP_LED_SHORT_TIMEOUT);
            ZB_SCHEDULE_APP_ALARM(sp_operation_led_loop, param + 1, 2 * SP_LED_SHORT_TIMEOUT);
        }
        else
        {
            ZB_SCHEDULE_APP_ALARM(sp_operation_led_loop, 0, SP_LED_LONG_TIMEOUT - SP_LED_SHORT_TIMEOUT);
        }
        break;

    case SP_LED_NORMAL:
        /* SP_LED_ON(); */
        /* ZB_SCHEDULE_APP_ALARM(sp_operation_led_off, SP_OPERATIONAL_LED, SP_LED_SHORT_TIMEOUT); */
        /* ZB_SCHEDULE_APP_ALARM(sp_operation_led_loop, 0, SP_LED_LONG_TIMEOUT + SP_LED_SHORT_TIMEOUT); */
        /* New indication pattern: On - green led, Off - red led */
        if (g_dev_ctx.on_off_attr.on_off)
        {
            SP_LED_OFF();
        }
        else
        {
            SP_LED_ON();
        }
        break;

    case SP_LED_IDENTIFY:
        SP_LED_ON();
        ZB_SCHEDULE_APP_ALARM(sp_operation_led_off, SP_OPERATIONAL_LED, SP_LED_SHORT_TIMEOUT);
        ZB_SCHEDULE_APP_ALARM(sp_operation_led_loop, 0, SP_LED_SHORT_TIMEOUT + SP_LED_SMALL_TIMEOUT);
        break;

    case SP_LED_RESET_FD:
        if (param < SP_LED_RESET_FD_COUNT)
        {
            /* do failed blinks */
            SP_LED_ON();
            ZB_SCHEDULE_APP_ALARM(sp_operation_led_off, SP_OPERATIONAL_LED, SP_LED_RESET_FD_TIMEOUT);
            ZB_SCHEDULE_APP_ALARM(sp_operation_led_loop, param + 1, SP_LED_SMALL_TIMEOUT + SP_LED_RESET_FD_TIMEOUT);
        }
        else
        {
            ZB_SCHEDULE_APP_ALARM(sp_start_operation_blink, (ZB_JOINED() ? (zb_uint8_t )SP_LED_NORMAL : (zb_uint8_t )SP_LED_IDLE_MODE), SP_LED_LONG_TIMEOUT + SP_LED_SHORT_TIMEOUT);
        }
        break;

    case SP_LED_CRITICAL_ERROR:
        if (param < SP_LED_CRITICAL_ERROR_COUNT)
        {
            /* do failed blinks */
            SP_LED_ON();
            ZB_SCHEDULE_APP_ALARM(sp_operation_led_off, SP_OPERATIONAL_LED, SP_LED_SHORT_TIMEOUT);
            if ((param + 1) % SP_LED_CRITICAL_ERROR_LONG_SERIES)
            {
                ZB_SCHEDULE_APP_ALARM(sp_operation_led_loop, param + 1, 2 * SP_LED_SHORT_TIMEOUT);
            }
            else
            {
                ZB_SCHEDULE_APP_ALARM(sp_operation_led_loop, param + 1, 3 * SP_LED_SHORT_TIMEOUT);
            }
        }
        else
        {
            /* /\* reset means physical reset - not reset to defaults, not nwk leave *\/ */
            /* zb_reset(0); */
            ZB_SCHEDULE_APP_ALARM(sp_start_operation_blink, (ZB_JOINED() ? (zb_uint8_t )SP_LED_NORMAL : (zb_uint8_t )SP_LED_IDLE_MODE), SP_LED_LONG_TIMEOUT + SP_LED_SHORT_TIMEOUT);
        }
        break;
    }
}


/* Switch blink mode depending on current state
 * param - new led state */
void sp_start_operation_blink(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> sp_start_operation_blink %hd", (FMT__H, param));

    /* start blinking operation unconditionally */
    if (g_dev_ctx.led_state != (sp_led_state_t)param)
    {
        g_dev_ctx.led_state = (sp_led_state_t)param;

        /* discard current LED indication */
        SP_LED_OFF();
        ZB_SCHEDULE_APP_ALARM_CANCEL(sp_operation_led_off, ZB_ALARM_ANY_PARAM);
        ZB_SCHEDULE_APP_ALARM_CANCEL(sp_operation_led_loop, ZB_ALARM_ANY_PARAM);

        /* start new LED indication */
        ZB_SCHEDULE_APP_ALARM(sp_operation_led_loop, 0, SP_LED_SMALL_TIMEOUT);
    }

    TRACE_MSG(TRACE_APP1, "<< sp_start_operation_blink", (FMT__0));
}

#ifdef SP_OTA
void sp_device_reset_after_upgrade(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> sp_ota_upgrade_check_fw", (FMT__0));

    ZVUNUSED(param);

    zb_reset(0);

    TRACE_MSG(TRACE_APP1, "<< sp_ota_upgrade_check_fw", (FMT__0));
}


void sp_process_ota_upgrade_cb(zb_uint8_t param)
{
    zb_zcl_device_callback_param_t *device_cb_param =
        ZB_BUF_GET_PARAM(param, zb_zcl_device_callback_param_t);
    zb_zcl_ota_upgrade_value_param_t *value = &(device_cb_param->cb_param.ota_value_param);

    TRACE_MSG(TRACE_APP1, ">> sp_process_ota_upgrade_cb param %hd",
              (FMT__H, param));

    TRACE_MSG(TRACE_APP1, "status %hd", (FMT__H, value->upgrade_status));

    switch (value->upgrade_status)
    {
    case ZB_ZCL_OTA_UPGRADE_STATUS_START:
        value->upgrade_status = sp_ota_upgrade_init(value->upgrade.start.file_length,
                                value->upgrade.start.file_version);
        break;

    case ZB_ZCL_OTA_UPGRADE_STATUS_RECEIVE:
        value->upgrade_status = sp_ota_upgrade_write_next_portion(value->upgrade.receive.block_data,
                                value->upgrade.receive.file_offset,
                                value->upgrade.receive.data_length);
        break;

    case ZB_ZCL_OTA_UPGRADE_STATUS_CHECK:
        value->upgrade_status = sp_ota_upgrade_check_fw(param);
        break;

    case ZB_ZCL_OTA_UPGRADE_STATUS_APPLY:
        sp_ota_upgrade_mark_fw_ok();
        value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_OK;
        break;

    case ZB_ZCL_OTA_UPGRADE_STATUS_FINISH:
        zb_zcl_ota_upgrade_file_upgraded(SP_ENDPOINT);
        /* Do not reset immediately - lets finish ZCL pkts exchange etc */
        ZB_SCHEDULE_APP_ALARM(sp_device_reset_after_upgrade, 0, ZB_TIME_ONE_SECOND * 15);
        value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_OK;
        break;

    case ZB_ZCL_OTA_UPGRADE_STATUS_ABORT:
        sp_ota_upgrade_abort();
        value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_OK;
        break;

    case ZB_ZCL_OTA_UPGRADE_STATUS_SERVER_NOT_FOUND:
        sp_ota_upgrade_server_not_found();
        value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_OK;
        break;

    default:
        sp_ota_upgrade_abort();
        value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_ERROR;
        break;
    }

    TRACE_MSG(TRACE_APP1, "<< sp_process_ota_upgrade_cb result_status %hd",
              (FMT__H, value->upgrade_status));
}
#endif /* SP_OTA */

void sp_write_app_data(zb_uint8_t param)
{
    ZVUNUSED(param);
#ifdef ZB_USE_NVRAM
    /* If we fail, trace is given and assertion is triggered */
    (void)zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);
#endif
}

#ifdef SP_CONTROL4_COMPATIBLE
void sp_device_configure_default_reporting(void)
{
    zb_zcl_reporting_info_t rep_info;
    ZB_BZERO(&rep_info, sizeof(rep_info));

    /* TODO: remove this function. Currently used only for debugging */
    {
        rep_info.direction = ZB_ZCL_CONFIGURE_REPORTING_SEND_REPORT;
        rep_info.ep = SP_ENDPOINT;
        rep_info.cluster_id = ZB_ZCL_CLUSTER_ID_ON_OFF;
        rep_info.cluster_role = ZB_ZCL_CLUSTER_SERVER_ROLE;
        rep_info.attr_id = ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID;
        rep_info.dst.profile_id = ZB_AF_HA_PROFILE_ID;

        rep_info.u.send_info.def_min_interval = 1;
        rep_info.u.send_info.def_max_interval = 30;
        rep_info.u.send_info.delta.u32 = 0; /* not an analog data type*/
    }
    zb_zcl_put_reporting_info(&rep_info, ZB_TRUE);

    {
        rep_info.direction = ZB_ZCL_CONFIGURE_REPORTING_SEND_REPORT;
        rep_info.ep = SP_ENDPOINT;
        rep_info.cluster_id = ZB_ZCL_CLUSTER_ID_METERING;
        rep_info.cluster_role = ZB_ZCL_CLUSTER_SERVER_ROLE;
        rep_info.attr_id = ZB_ZCL_ATTR_METERING_CURRENT_SUMMATION_DELIVERED_ID;
        rep_info.dst.profile_id = ZB_AF_HA_PROFILE_ID;

        rep_info.u.send_info.def_min_interval = 10;
        rep_info.u.send_info.def_max_interval = 60;
        rep_info.u.send_info.delta.s24.low = 5;
    }
    zb_zcl_put_reporting_info(&rep_info, ZB_TRUE);

    {
        rep_info.direction = ZB_ZCL_CONFIGURE_REPORTING_SEND_REPORT;
        rep_info.ep = SP_ENDPOINT;
        rep_info.cluster_id = ZB_ZCL_CLUSTER_ID_METERING;
        rep_info.cluster_role = ZB_ZCL_CLUSTER_SERVER_ROLE;
        rep_info.attr_id = ZB_ZCL_ATTR_METERING_INSTANTANEOUS_DEMAND_ID;
        rep_info.dst.profile_id = ZB_AF_HA_PROFILE_ID;

        rep_info.u.send_info.def_min_interval = 10;
        rep_info.u.send_info.def_max_interval = 60;
        rep_info.u.send_info.delta.s24.low = 5;
    }
    zb_zcl_put_reporting_info(&rep_info, ZB_TRUE);
}

#ifdef SP_OTA
void sp_device_update_ota_upgrade_server_attributes(void)
{
    if (g_dev_ctx.ota_ctx.is_started_manually)
    {
        /* Update OTA Upgrade server address */
        ZB_ZCL_SET_ATTRIBUTE(SP_ENDPOINT, ZB_ZCL_CLUSTER_ID_OTA_UPGRADE, ZB_ZCL_CLUSTER_CLIENT_ROLE, ZB_ZCL_ATTR_OTA_UPGRADE_SERVER_ID,
                             (zb_uint8_t *)&g_dev_ctx.c4_attr.access_point_long_ID, ZB_FALSE);
        ZB_ZCL_SET_ATTRIBUTE(SP_ENDPOINT, ZB_ZCL_CLUSTER_ID_OTA_UPGRADE, ZB_ZCL_CLUSTER_CLIENT_ROLE, ZB_ZCL_ATTR_OTA_UPGRADE_SERVER_ADDR_ID,
                             (zb_uint8_t *)&g_dev_ctx.c4_attr.access_point_node_ID, ZB_FALSE);
    }
}
#endif /* SP_OTA */

void sp_device_reconfigure_bindings_completed(zb_bufid_t buffer)
{
    zb_buf_free(buffer);

#ifdef SP_OTA
    sp_device_update_ota_upgrade_server_attributes();
#endif /* SP_OTA */

    if (g_dev_ctx.is_on_c4_network == ZB_FALSE)
    {
        g_dev_ctx.is_on_c4_network = ZB_TRUE;
        ZB_SCHEDULE_APP_CALLBACK(sp_write_app_data, 0);
    }
}

void sp_device_reconfigure_bindings_metering(zb_bufid_t buffer)
{
    zb_apsme_binding_req_t *req;

    /* Bind Metering cluster */
    req = ZB_BUF_GET_PARAM(buffer, zb_apsme_binding_req_t);

    /* NOTE: zb_apsme_binding_req_t parameter is already filled here, so fill only changed fields */
    req->clusterid = ZB_ZCL_CLUSTER_ID_METERING;
    req->confirm_cb = sp_device_reconfigure_bindings_completed;

    zb_apsme_bind_request(buffer);
}

void sp_device_reconfigure_bindings_on_off(zb_bufid_t buffer)
{
    zb_apsme_binding_req_t *req;

    if (!buffer)
    {
        zb_buf_get_out_delayed(sp_device_reconfigure_bindings_on_off);
    }
    else
    {
        /* Unbind all old bindings */
        zb_apsme_unbind_all(0);

        /* Bind On/Off cluster */
        req = ZB_BUF_GET_PARAM(buffer, zb_apsme_binding_req_t);

        ZB_IEEE_ADDR_COPY(req->src_addr, &ZB_PIBCACHE_EXTENDED_ADDRESS());
        ZB_IEEE_ADDR_COPY(req->dst_addr.addr_long, g_dev_ctx.c4_attr.access_point_long_ID);

        req->src_endpoint = SP_ENDPOINT;
        req->clusterid = ZB_ZCL_CLUSTER_ID_ON_OFF;
        req->addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
        req->dst_endpoint = SP_ENDPOINT;
        req->confirm_cb = sp_device_reconfigure_bindings_metering;

        zb_apsme_bind_request(buffer);
    }
}

void sp_control4_network_zap_cb(void)
{
    TRACE_MSG(TRACE_APP1, "Control4 ZAP information received", (FMT__0));

    sp_device_configure_default_reporting();
    sp_device_reconfigure_bindings_on_off(0);
}
#endif /* SP_CONTROL4_COMPATIBLE */

void sp_device_interface_cb(zb_uint8_t param)
{
    zb_zcl_device_callback_param_t *device_cb_param =
        ZB_BUF_GET_PARAM(param, zb_zcl_device_callback_param_t);

    TRACE_MSG(TRACE_APP1, "> sp_device_interface_cb param %hd id %hd", (FMT__H_H,
              param, device_cb_param->device_cb_id));

    /* WARNING: Default status rewrited */
    device_cb_param->status = RET_OK;

    switch (device_cb_param->device_cb_id)
    {
#ifdef SP_OTA
    case ZB_ZCL_OTA_UPGRADE_VALUE_CB_ID:
        sp_process_ota_upgrade_cb(param);
        break;
#endif

    /* Set attribute command handling */
    case ZB_ZCL_SET_ATTR_VALUE_CB_ID:
        if (device_cb_param->cb_param.set_attr_value_param.cluster_id == ZB_ZCL_CLUSTER_ID_ON_OFF &&
                device_cb_param->cb_param.set_attr_value_param.attr_id == ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID)
        {
            sp_relay_on_off((zb_bool_t)device_cb_param->cb_param.set_attr_value_param.values.data8);
            ZB_SCHEDULE_APP_CALLBACK(sp_write_app_data, 0);
        }
        break;

    /* Reset to defaults command handling */
    case ZB_ZCL_BASIC_RESET_CB_ID:
        sp_basic_reset_to_defaults_cb(0);
        break;

#ifdef SP_CONTROL4_COMPATIBLE
    case ZB_ZCL_CONTROL4_NETWORK_ZAP_INFO_CB_ID:
    {
        const zb_zcl_control4_zap_info_notify_t *notification = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_control4_zap_info_notify_t);

        if (notification->event == ZB_ZCL_CONTROL4_NETWORK_ZAP_UPDATED)
        {
            TRACE_MSG(TRACE_APP1, "ZAP discover information updated", (FMT__0));

            sp_control4_network_zap_cb();
        }
        else if (notification->event == ZB_ZCL_CONTROL4_NETWORK_ZAP_DISCOVER_FAILED)
        {
            /* ZAP Discover Failed
                Does the joined network is a Control4 network?
                - Restart Contro4 logic if device was in C4 network in the past (is_on_c4_network == True)
                  - Do not restart network since device was not previously connected to a C4 network (is_on_c4_network == False)
            */
            TRACE_MSG(TRACE_ERROR, "Failed ZAP discover.", (FMT__0));

            if (g_dev_ctx.is_on_c4_network == ZB_TRUE)
            {
                TRACE_MSG(TRACE_ERROR, "Restarting Control4 network cluster", (FMT__0));

                /* Restart Control4 Network Cluster logic */
                zb_zcl_control4_network_cluster_stop();

                if (zb_zcl_control4_network_cluster_start() != RET_OK)
                {
                    TRACE_MSG(TRACE_ERROR, "Failed to start Control4 Network Cluster", (FMT__0));
                }
            }
            else
            {
                TRACE_MSG(TRACE_ERROR, "Not restarting Control4 network cluster", (FMT__0));
            }
        }
    }
    break;
#endif /* SP_CONTROL4_COMPATIBLE */

#ifdef SP_WWAH_COMPATIBLE
    case ZB_ZCL_WWAH_DEBUG_REPORT_QUERY_CB_ID:
        sp_process_wwah_debug_report_query_cb(param);
        break;
#endif

    default:
        device_cb_param->status = RET_ERROR;
        break;
    }

    TRACE_MSG(TRACE_APP1, "< sp_device_interface_cb %hd", (FMT__H, device_cb_param->status));
}


#ifdef ZB_USE_NVRAM
zb_uint16_t sp_get_nvram_data_size()
{
    TRACE_MSG(TRACE_APP1, "sp_get_nvram_data_size, ret %hd", (FMT__H, sizeof(sp_device_nvram_dataset_t)));
    return sizeof(sp_device_nvram_dataset_t);
}

void sp_nvram_read_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length)
{
    sp_device_nvram_dataset_t ds;
    zb_ret_t ret;

    TRACE_MSG(TRACE_APP1, ">> sp_nvram_read_app_data page %hd pos %d", (FMT__H_D, page, pos));

    ZB_ASSERT(payload_length == sizeof(ds));

    /* If we fail, trace is given and assertion is triggered */
    ret = zb_nvram_read_data(page, pos, (zb_uint8_t *)&ds, sizeof(ds));

    if (ret == RET_OK)
    {
        ZB_ZCL_SET_ATTRIBUTE(SP_ENDPOINT, ZB_ZCL_CLUSTER_ID_ON_OFF, ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID,
                             &(ds.sp_onoff_state), ZB_FALSE);
        ZB_ZCL_SET_ATTRIBUTE(SP_ENDPOINT,
                             ZB_ZCL_CLUSTER_ID_METERING,
                             ZB_ZCL_CLUSTER_SERVER_ROLE,
                             ZB_ZCL_ATTR_METERING_CURRENT_SUMMATION_DELIVERED_ID,
                             (zb_uint8_t *) & (ds.curr_summ_delivered),
                             ZB_FALSE);

#ifdef SP_CONTROL4_COMPATIBLE
        ZB_ZCL_SET_ATTRIBUTE(ZB_CONTROL4_NETWORK_ENDPOINT,
                             ZB_ZCL_CLUSTER_ID_CONTROL4_NETWORKING,
                             ZB_ZCL_CLUSTER_SERVER_ROLE,
                             ZB_ZCL_ATTR_CONTROL4_NETWORKING_BOOT_COUNT_ID,
                             (zb_uint8_t *) & (ds.boot_count),
                             ZB_FALSE);

        g_dev_ctx.is_on_c4_network = ds.is_on_c4_network;
#endif /* SP_CONTROL4_COMPATIBLE */
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "nvram read error %d", (FMT__D, ret));
    }

    TRACE_MSG(TRACE_APP1, "<< sp_nvram_read_app_data", (FMT__0));
}


zb_ret_t sp_nvram_write_app_data(zb_uint8_t page, zb_uint32_t pos)
{
    zb_ret_t ret;
    sp_device_nvram_dataset_t ds;
    zb_zcl_attr_t *attr_desc_on_off = zb_zcl_get_attr_desc_a(SP_ENDPOINT, ZB_ZCL_CLUSTER_ID_ON_OFF, ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID);

    TRACE_MSG(TRACE_APP1, ">> sp_nvram_write_app_data, page %hd, pos %d", (FMT__H_D, page, pos));

    ds.sp_onoff_state = ZB_ZCL_GET_ATTRIBUTE_VAL_8(attr_desc_on_off);
    ds.curr_summ_delivered = g_dev_ctx.metering_attr.curr_summ_delivered;
#ifdef SP_CONTROL4_COMPATIBLE
    ds.boot_count = g_dev_ctx.c4_attr.boot_count;
    ds.is_on_c4_network = g_dev_ctx.is_on_c4_network;
#endif /* SP_CONTROL4_COMPATIBLE */

    /* If we fail, trace is given and assertion is triggered */
    ret = zb_nvram_write_data(page, pos, (zb_uint8_t *)&ds, sizeof(ds));

    TRACE_MSG(TRACE_APP1, "<< sp_nvram_write_app_data, ret %d", (FMT__D, ret));

    return ret;
}

void sp_update_on_off_state(zb_uint8_t is_on)
{
    ZB_ZCL_SET_ATTRIBUTE(SP_ENDPOINT, ZB_ZCL_CLUSTER_ID_ON_OFF, ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID,
                         &(is_on), ZB_FALSE);
    sp_write_app_data(0);
}

#endif /* ZB_USE_NVRAM */
