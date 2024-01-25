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
/* PURPOSE: General IAS zone device application
*/
#define ZB_TRACE_FILE_ID 63295
#include "izs_device.h"
#include "zboss_api.h"

#if ! defined ZB_ED_ROLE
#error define ZB_ED_ROLE to compile ze tests
#endif

/* Handler for specific zcl commands */
zb_uint8_t izs_zcl_cmd_handler(zb_uint8_t param);
void izs_check_ias_zone_found(zb_uint8_t param);

static void izs_set_default_configuration_values(void);

/**
 * Declaration of Zigbee device data structures
 */

/* Global device context */
izs_device_ctx_t g_device_ctx;

/* [COMMON_DECLARATION] */
/**
 * Declaring attributes for each cluster
 */

/* Global cluster attributes data initiated into global device context */
ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST_EXT(
  basic_attr_list,
  &g_device_ctx.basic_attr.zcl_version,
  &g_device_ctx.basic_attr.app_version,
  &g_device_ctx.basic_attr.stack_version,
  &g_device_ctx.basic_attr.hw_version,
  &g_device_ctx.basic_attr.mf_name,
  &g_device_ctx.basic_attr.model_id,
  &g_device_ctx.basic_attr.date_code,
  &g_device_ctx.basic_attr.power_source,
  &g_device_ctx.basic_attr.location_id,
  &g_device_ctx.basic_attr.ph_env,
  &g_device_ctx.basic_attr.sw_build_id);

/* Identify cluster attributes data initiated into global device context */
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list,
  &g_device_ctx.identify_attr.identify_time);

/* IAS Zone cluster attributes data initiated into global device context */
ZB_ZCL_DECLARE_IAS_ZONE_ATTRIB_LIST_EXT(
  ias_zone_attr_list,
  &IZS_DEVICE_ZONE_STATE(),
  &g_device_ctx.zone_attr.zone_type,
  &g_device_ctx.zone_attr.zone_status,
  &g_device_ctx.zone_attr.number_of_zone_sens_levels_supported,
  &g_device_ctx.zone_attr.current_zone_sens_level,
  &IZS_DEVICE_CIE_ADDR(),
  &g_device_ctx.zone_attr.zone_id,
  &g_device_ctx.zone_attr.cie_short_addr,
  &g_device_ctx.zone_attr.cie_ep);

/* Poll control cluster attributes data initiated into global device context */
ZB_ZCL_DECLARE_POLL_CONTROL_ATTRIB_LIST(poll_ctrl_attr_list,
  &g_device_ctx.poll_control_attr.checkin_interval,
  &g_device_ctx.poll_control_attr.long_poll_interval,
  &g_device_ctx.poll_control_attr.short_poll_interval,
  &g_device_ctx.poll_control_attr.fast_poll_timeout,
  &g_device_ctx.poll_control_attr.checkin_interval_min,
  &g_device_ctx.poll_control_attr.long_poll_interval_min,
  &g_device_ctx.poll_control_attr.fast_poll_timeout_max);

/* Define 'bat_num' as empty in order to declare default battery set attributes. */
/* According to Table 3-17 of ZCL specification, defining 'bat_num' as 2 or 3 allows */
/* to declare battery set attributes for BATTERY2 and BATTERY3 */
#define bat_num

/* Power config cluster attributes data initiated into global device context */
ZB_ZCL_DECLARE_POWER_CONFIG_BATTERY_ATTRIB_LIST_EXT(
  power_config_attr_list,
  &g_device_ctx.pwr_cfg_attr.voltage,
  &g_device_ctx.pwr_cfg_attr.size,
  &g_device_ctx.pwr_cfg_attr.quantity,
  &g_device_ctx.pwr_cfg_attr.rated_voltage,
  &g_device_ctx.pwr_cfg_attr.alarm_mask,
  &g_device_ctx.pwr_cfg_attr.voltage_min_threshold,
  &g_device_ctx.pwr_cfg_attr.remaining,
  &g_device_ctx.pwr_cfg_attr.threshold1,
  &g_device_ctx.pwr_cfg_attr.threshold2,
  &g_device_ctx.pwr_cfg_attr.threshold3,
  &g_device_ctx.pwr_cfg_attr.min_threshold,
  &g_device_ctx.pwr_cfg_attr.percent_threshold1,
  &g_device_ctx.pwr_cfg_attr.percent_threshold2,
  &g_device_ctx.pwr_cfg_attr.percent_threshold3,
  &g_device_ctx.pwr_cfg_attr.alarm_state);

/* OTA cluster attributes declaration*/
#ifdef IZS_OTA
/* OTA cluster attributes data initiated into global device context */
ZB_ZCL_DECLARE_OTA_UPGRADE_ATTRIB_LIST(ota_upgrade_attr_list,
    &g_device_ctx.ota_attr.upgrade_server,
    &g_device_ctx.ota_attr.file_offset,
    &g_device_ctx.ota_attr.file_version,
    &g_device_ctx.ota_attr.stack_version,
    &g_device_ctx.ota_attr.downloaded_file_ver,
    &g_device_ctx.ota_attr.downloaded_stack_ver,
    &g_device_ctx.ota_attr.image_status,
    &g_device_ctx.ota_attr.manufacturer,
    &g_device_ctx.ota_attr.image_type,
    &g_device_ctx.ota_attr.min_block_reque,
    &g_device_ctx.ota_attr.image_stamp,
    &g_device_ctx.ota_attr.server_addr,
    &g_device_ctx.ota_attr.server_ep,
    IZS_INIT_OTA_HW_VERSION,
    IZS_OTA_IMAGE_BLOCK_DATA_SIZE_MAX,
    ZB_ZCL_OTA_UPGRADE_QUERY_TIMER_COUNT_DEF);
#else
zb_zcl_attr_t ota_upgrade_attr_list[] = { { ZB_ZCL_NULL_ID, 0, 0, NULL } };
#endif

/* WWAH cluster attributes declaration*/
#ifdef IZS_WWAH
ZB_ZCL_DECLARE_WWAH_ATTRIB_LIST(wwah_attr_list);
#else
zb_zcl_attr_t wwah_attr_list[] = { { ZB_ZCL_NULL_ID, 0, 0, NULL } };
#endif

/* Declare cluster list for a device */
#ifdef IZS_WWAH
IZS_DECLARE_IAS_ZONE_CLUSTER_LIST(
  cs_device_clusters,
  basic_attr_list,
  wwah_attr_list,
  identify_attr_list,
  ias_zone_attr_list,
  poll_ctrl_attr_list,
  power_config_attr_list,
  ota_upgrade_attr_list);
#else
IZS_DECLARE_IAS_ZONE_CLUSTER_LIST(
  cs_device_clusters,
  basic_attr_list,
  identify_attr_list,
  ias_zone_attr_list,
  poll_ctrl_attr_list,
  power_config_attr_list,
  ota_upgrade_attr_list);
#endif

/* Declare endpoint */
IZS_DECLARE_IAS_ZONE_EP(cs_device_ep, IZS_DEVICE_ENDPOINT, cs_device_clusters);
/* Declare application's device context for single-endpoint device */
ZB_HA_DECLARE_IAS_ZONE_CTX(izs_device_zcl_ctx, cs_device_ep);
/* [COMMON_DECLARATION] */

/* Indirect Poll rate during commissioning */
#ifdef FAST_POLLING_DURING_COMMISSIONING

void izs_start_fast_polling_for_commissioning(zb_time_t fast_poll_timeout_ms)
{
  /* Stop the Turbo Poll adaptive algorithm */
  zb_zdo_pim_turbo_poll_continuous_leave(0);
  /* Enable Turbo Poll */
  zb_zdo_pim_permit_turbo_poll(ZB_TRUE);
  /* Start the Turbo Poll adaptive algorithm for the specified timeout. */
  zb_zdo_pim_start_turbo_poll_continuous(fast_poll_timeout_ms);
}

#endif /* FAST_POLLING_DURING_COMMISSIONING */

MAIN()
{
  ARGV_UNUSED;

  /* Trace enable */
  /* ZB_SET_TRACE_OFF(); */
  /* Traffic dump enable */
  ZB_SET_TRAF_DUMP_OFF();
  ZB_SET_TRACE_MASK(0x800);

  /* Global ZBOSS initialization */
  ZB_INIT("izs_device");

#ifdef IZS_WWAH
  zb_zcl_wwah_set_wwah_behavior(ZB_ZCL_WWAH_BEHAVIOR_SERVER);
#endif

  /* Register application callback for reading application data from NVRAM */
  zb_nvram_register_app1_read_cb(izs_nvram_read_app_data);
  /* Register application callback for writing application data to NVRAM */
  zb_nvram_register_app1_write_cb(izs_nvram_write_app_data, izs_get_nvram_data_size);

  /* [REGISTER] */
  /* Register device ZCL context */
  ZB_AF_REGISTER_DEVICE_CTX(&izs_device_zcl_ctx);
  /* Register cluster commands handler for a specific endpoint */
  ZB_AF_SET_ENDPOINT_HANDLER(IZS_DEVICE_ENDPOINT, izs_zcl_cmd_handler);
  /* [REGISTER] */

  /* Sets identify notification handler for endpoint */
  ZB_AF_SET_IDENTIFY_NOTIFICATION_HANDLER(IZS_DEVICE_ENDPOINT, izs_identify_notification);
  /* Set Device user application callback (for OTA cluster) */
  ZB_ZCL_REGISTER_DEVICE_CB(izs_device_interface_cb);

  /*** Init application structures, ZB settings ***/
  /* h/w init is called from izs_device_app_init */
  izs_device_app_init(0);

  /* Initiate the stack start without starting the commissioning */
  if (zboss_start_no_autostart() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zb_zdo_dev_init failed", (FMT__0));
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

/* Set up defaults for the commissioning */
static void izs_set_default_configuration_values(void)
{
  static zb_ieee_addr_t ieee_addr = IZS_DEFAULT_EXTENDED_ADDRESS;

  zb_set_network_ed_role(IZS_DEFAULT_APS_CHANNEL_MASK);

  zb_set_long_address(ieee_addr);

  ZB_ZCL_SET_STRING_VAL(g_device_ctx.basic_attr.mf_name, IZS_INIT_BASIC_MANUF_NAME, ZB_ZCL_STRING_CONST_SIZE(g_device_ctx.basic_attr.mf_name));
  ZB_ZCL_SET_STRING_VAL(g_device_ctx.basic_attr.model_id, IZS_INIT_BASIC_DEFAULT_MODEL_ID, ZB_ZCL_STRING_CONST_SIZE(g_device_ctx.basic_attr.model_id));
}

void izs_device_startup(zb_uint8_t param)
{
  zb_time_t delay = 0;
/*
  On startup, after all H/W is initialized and NVRAM is read, check:
- if device is already enrolled or not
- if not enrolled, perform "full" reset to factory defaults and start joining
*/
  TRACE_MSG(TRACE_APP1, ">> izs_device_startup %hd state %hd", (FMT__H_H, param, IZS_GET_DEVICE_STATE()));

  ZVUNUSED(param);

  TRACE_MSG(TRACE_APP1, "zone_state %hd", (FMT__H, g_device_ctx.zone_attr.zone_state));

  if (ZB_JOINED() || IZS_DEVICE_IS_ENROLLED()) /* joined or enrolled - continue */
  {
    TRACE_MSG(TRACE_APP1, "joined or enrolled - continue", (FMT__0));
    ZB_SCHEDULE_APP_ALARM(izs_joined_cont, 0, delay);
  }
  else /* neither joined nor enrolled - do full reset and start */
  {
    TRACE_MSG(TRACE_APP1, "neither joined nor enrolled - do full reset and start", (FMT__0));
    ZB_SCHEDULE_APP_ALARM(izs_full_reset_to_defaults, 0, IZS_NOT_ENROLLED_TIMEOUT);
  }

  TRACE_MSG(TRACE_APP1, "<< izs_device_startup", (FMT__0));
}

void izs_set_power_cfg_battery_pack_init(zb_uint8_t sensor_type)
{
  ZVUNUSED(sensor_type);

  g_device_ctx.pwr_cfg_attr.threshold1 = IZS_BATTERY_VOLTAGE_INVALID_THRESHOLD;
  g_device_ctx.pwr_cfg_attr.threshold2 = IZS_BATTERY_VOLTAGE_INVALID_THRESHOLD;
  g_device_ctx.pwr_cfg_attr.threshold3 = IZS_BATTERY_VOLTAGE_60_DAYS_THRESHOLD/100;

  g_device_ctx.pwr_cfg_attr.quantity = IZS_INIT_BATTERY_QUANTITY;
  g_device_ctx.pwr_cfg_attr.voltage_min_threshold = IZS_BATTERY_VOLTAGE_INVALID_THRESHOLD;
  g_device_ctx.pwr_cfg_attr.percent_threshold1 = IZS_BATTERY_VOLTAGE_INVALID_THRESHOLD;
  g_device_ctx.pwr_cfg_attr.percent_threshold2 = IZS_BATTERY_VOLTAGE_INVALID_THRESHOLD;
  g_device_ctx.pwr_cfg_attr.percent_threshold3 = IZS_BATTERY_VOLTAGE_60_DAYS_THRESHOLD_PERCENTAGE;
}

#ifdef IZS_WWAH
/*Fill Debug Report Table with default values
  Fill #4 Debug Report for testing puprose */
static void izs_setup_wwah_debug_report(void)
{
  zb_uindex_t i;
  zb_char_t *debug_report_message = "Issue #4";
  zb_uint8_t report_id = 4;
  ZB_ASSERT(DEBUG_REPORT_TABLE_SIZE > 0 && DEBUG_REPORT_TABLE_SIZE <= 0xFE);
  ZB_ZCL_SET_ATTRIBUTE(IZS_DEVICE_ENDPOINT, ZB_ZCL_CLUSTER_ID_WWAH, ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_WWAH_CURRENT_DEBUG_REPORT_ID_ID, &report_id, ZB_FALSE);
  g_device_ctx.debug_report_table[0] = (zb_zcl_wwah_debug_report_t){report_id, strlen(debug_report_message), debug_report_message};
  for(i = 1; i < DEBUG_REPORT_TABLE_SIZE; ++i)
  {
    g_device_ctx.debug_report_table[i] = ZB_ZCL_WWAH_DEBUG_REPORT_FREE_RECORD;
  }
}
#endif

void izs_clusters_attr_init(zb_uint8_t param)
{
#ifdef IZS_OTA
  zb_ieee_addr_t default_ota_server_addr = IZS_OTA_UPGRADE_SERVER;
#endif

  TRACE_MSG(TRACE_APP1, ">> izs_clusters_attr_init", (FMT__0));

  ZVUNUSED(param);

  g_device_ctx.enrollment_method = IZS_DEFAULT_ENROLLMENT_METHOD;

  /* Basic cluster attributes */
  g_device_ctx.basic_attr.zcl_version  = ZB_ZCL_VERSION;
  g_device_ctx.basic_attr.app_version = MAP_16BITVERSION_TO_8BITVERSION(IZS_INIT_BASIC_APP_VERSION);
  g_device_ctx.basic_attr.stack_version = MAP_16BITVERSION_TO_8BITVERSION(IZS_INIT_BASIC_STACK_VERSION);
  g_device_ctx.basic_attr.hw_version = MAP_16BITVERSION_TO_8BITVERSION(IZS_INIT_BASIC_HW_VERSION);

  ZB_ZCL_SET_STRING_VAL(
    g_device_ctx.basic_attr.date_code,
    IZS_INIT_BASIC_DATE_CODE,
    ZB_ZCL_STRING_CONST_SIZE(IZS_INIT_BASIC_DATE_CODE));

  g_device_ctx.basic_attr.power_source = ZB_ZCL_BASIC_POWER_SOURCE_BATTERY;

  ZB_ZCL_SET_STRING_VAL(
    g_device_ctx.basic_attr.location_id,
    IZS_INIT_BASIC_LOCATION_ID,
    ZB_ZCL_STRING_CONST_SIZE(IZS_INIT_BASIC_LOCATION_ID));

  g_device_ctx.basic_attr.ph_env = IZS_INIT_BASIC_PH_ENV;
  ZB_ZCL_SET_STRING_LENGTH(g_device_ctx.basic_attr.sw_build_id, 0);

  /* Identify cluster attributes */
  g_device_ctx.identify_attr.identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

  /* IAS Zone cluster attributes */
  g_device_ctx.zone_attr.zone_type = ZB_ZCL_IAS_ZONE_ZONETYPE_MOTION;
  g_device_ctx.zone_attr.zone_status = IZS_INIT_ZONE_STATUS;
  g_device_ctx.zone_attr.zone_id = IZS_INIT_ZONE_ID;
  g_device_ctx.zone_attr.zone_state = ZB_ZCL_IAS_ZONE_ZONESTATE_NOT_ENROLLED;
  ZB_IEEE_ADDR_ZERO(g_device_ctx.zone_attr.cie_addr);
  g_device_ctx.zone_attr.cie_short_addr = IZS_INIT_CIE_SHORT_ADDR;
  g_device_ctx.zone_attr.cie_ep = IZS_INIT_CIE_ENDPOINT;
  g_device_ctx.zone_attr.number_of_zone_sens_levels_supported =
    ZB_ZCL_IAS_ZONE_NUMBER_OF_ZONE_SENSITIVITY_LEVELS_SUPPORTED_DEFAULT_VALUE;

  /* Poll Control cluster attributes */
  g_device_ctx.poll_control_attr.checkin_interval       = IZS_DEVICE_CHECKIN_INTERVAL;
  g_device_ctx.poll_control_attr.long_poll_interval     = IZS_DEVICE_LONG_POLL_INTERVAL;
  g_device_ctx.poll_control_attr.short_poll_interval    = IZS_DEVICE_SHORT_POLL_INTERVAL;
  g_device_ctx.poll_control_attr.fast_poll_timeout      = ZB_ZCL_POLL_CONTROL_FAST_POLL_TIMEOUT_DEFAULT_VALUE;
  g_device_ctx.poll_control_attr.checkin_interval_min   = IZS_DEVICE_MIN_CHECKIN_INTERVAL;
  g_device_ctx.poll_control_attr.long_poll_interval_min = IZS_DEVICE_MIN_LONG_POLL_INTERVAL;
  g_device_ctx.poll_control_attr.fast_poll_timeout_max  = ZB_ZCL_POLL_CONTROL_FAST_POLL_TIMEOUT_CLIENT_DEFAULT_VALUE;

  /* Power config cluster attributes */
  g_device_ctx.pwr_cfg_attr.voltage = IZS_INIT_BATTERY_VOLTAGE;
  g_device_ctx.pwr_cfg_attr.size = ZB_ZCL_POWER_CONFIG_BATTERY_SIZE_AA;
  g_device_ctx.pwr_cfg_attr.alarm_mask = IZS_INIT_BATTERY_ALARM_MASK;

  izs_set_power_cfg_battery_pack_init(0);

  g_device_ctx.pwr_cfg_attr.alarm_state = IZS_INIT_BATTERY_ALARM_STATE;

#ifdef IZS_OTA
  /* OTA Upgrade client cluster attributes data */
  ZB_IEEE_ADDR_COPY(g_device_ctx.ota_attr.upgrade_server, default_ota_server_addr);
  g_device_ctx.ota_attr.file_offset = ZB_ZCL_OTA_UPGRADE_FILE_OFFSET_DEF_VALUE;
  g_device_ctx.ota_attr.file_version = IZS_INIT_OTA_FILE_VERSION;
  g_device_ctx.ota_attr.stack_version = ZB_ZCL_OTA_UPGRADE_STACK_VERSION_DEF_VALUE;
  g_device_ctx.ota_attr.downloaded_file_ver = ZB_ZCL_OTA_UPGRADE_DOWNLOADED_FILE_VERSION_DEF_VALUE;
  g_device_ctx.ota_attr.downloaded_stack_ver = ZB_ZCL_OTA_UPGRADE_DOWNLOADED_STACK_DEF_VALUE;
  g_device_ctx.ota_attr.image_status = ZB_ZCL_OTA_UPGRADE_IMAGE_STATUS_DEF_VALUE;
  g_device_ctx.ota_attr.manufacturer = IZS_INIT_OTA_MANUFACTURER;
  g_device_ctx.ota_attr.image_type = IZS_INIT_OTA_IMAGE_TYPE;
  g_device_ctx.ota_attr.min_block_reque = IZS_INIT_OTA_MIN_BLOCK_REQUE;
  g_device_ctx.ota_attr.image_stamp = IZS_INIT_OTA_IMAGE_STAMP;
#endif
#ifdef IZS_WWAH
  zb_zcl_wwah_init_server_attr();
  izs_setup_wwah_debug_report();
#endif

  g_device_ctx.enroll_req_generated = ZB_FALSE;

  izs_update_ias_zone_status(ZB_ZCL_IAS_ZONE_ZONE_STATUS_TROUBLE, g_device_ctx.detector_trouble);

  TRACE_MSG(TRACE_APP1, "<< izs_clusters_attr_init", (FMT__0));
}

void izs_init_default_reporting(void)
{
  TRACE_MSG(TRACE_APP1, ">> izs_init_default_reporting", (FMT__0));

  ZB_SCHEDULE_APP_CALLBACK(izs_configure_power_config_default_reporting, 0);

  TRACE_MSG(TRACE_APP1, "<< izs_init_default_reporting", (FMT__0));
}

void izs_device_app_init(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> izs_device_app_init", (FMT__0));

  ZVUNUSED(param);

  /* Initialize global application context */
  izs_app_ctx_init();

  /* Init HA attributes */
  izs_clusters_attr_init(0);

  /* Set up defaults for the commissioning */
  izs_set_default_configuration_values();

  /* HAL initialisation for hardware */
  izs_device_hw_init(0);

  /* Erase NVRAM if reset button is pressed on start */
  /* zb_set_nvram_erase_at_start( izs_hal_get_button_state(IZS_RESET_BUTTON) ); */
  /* Set RX on when idle configuration */
  zb_set_rx_on_when_idle(ZB_FALSE);

  /* Set internal application callback */
  zb_zcl_ias_zone_register_cb(IZS_DEVICE_ENDPOINT,
                              izs_ias_zone_notification_cb,
                              NULL);

  /* Register callback for check-in. Use this callback to send attr
   * values reports each check-in interval */
  zb_zcl_poll_controll_register_cb(izs_check_in_cb);

  /* Set reporting configuration */
  izs_init_default_reporting();

  /* Initialize battery voltage measurements context*/
  izs_measure_batteries();

  TRACE_MSG(TRACE_APP1, "<< izs_device_app_init", (FMT__0));
}

void izs_go_for_reset(zb_uint8_t unused)
{
  ZVUNUSED(unused);
  TRACE_MSG(TRACE_APP1, ">> izs_go_for_reset", (FMT__0));

  zb_reset(0);

  TRACE_MSG(TRACE_APP1, "<< izs_go_for_reset", (FMT__0));
}


void izs_leave_indication(zb_uint8_t leave_type)
{
  TRACE_MSG(TRACE_APP1, ">> izs_leave_indication state %hd", (FMT__H, g_device_ctx.device_state));

  IZS_SET_DEVICE_STATE(IZS_STATE_RESET_TO_DEFAULT);

  ZB_SCHEDULE_APP_ALARM_CANCEL(izs_check_ias_zone_found, ZB_ALARM_ANY_PARAM);

  if (leave_type == ZB_NWK_LEAVE_TYPE_RESET)
  {
    /* 2.2.9	Reset to factory defaults: IZS should do RTFD on network leave */
    izs_basic_reset_to_defaults(0);
  }

  ZB_SCHEDULE_APP_CALLBACK(izs_leave_nwk_cb, 0);

  TRACE_MSG(TRACE_APP1, "<< izs_leave_indication", (FMT__0));
}

void izs_device_hw_init(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> izs_device_hw_init", (FMT__0));

  ZVUNUSED(param);

  izs_hal_hw_init();

  IZS_SET_DEVICE_STATE(IZS_STATE_HW_INIT);

  TRACE_MSG(TRACE_APP1, "<< izs_device_hw_init", (FMT__0));
}

void izs_check_ias_zone_found(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> izs_check_ias_zone_found param %hd", (FMT__H, param));

  TRACE_MSG(TRACE_APP1, "CIE is NOT found, leaving", (FMT__0));
  /* Do not call retry_join here, we need to do full reset before */
  izs_full_reset_to_defaults(0);

  zb_buf_free(param);
  TRACE_MSG(TRACE_APP1, "<< izs_check_ias_zone_found", (FMT__0));
}

void izs_start_bdb_commissioning(zb_uint8_t param)
{
  zb_ret_t ret = RET_OK;

  ZVUNUSED(param);

  TRACE_MSG(TRACE_APP1, ">> izs_start_bdb_commissioning", (FMT__0));

  /* Start steering */
  bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
  if (ret != RET_OK)
  {
    /* commissioning failed, show failed and reboot... */
    izs_full_reset_to_defaults(0);
  }

  TRACE_MSG(TRACE_APP1, "<< izs_start_bdb_commissioning", (FMT__0));
}

#ifdef IZS_OTA
void izs_ota_init(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> izs_ota_init", (FMT__0));

  ZB_SCHEDULE_APP_ALARM(zb_zcl_ota_upgrade_init_client, param, 15*ZB_TIME_ONE_SECOND);

  TRACE_MSG(TRACE_APP1, "<< izs_ota_init", (FMT__0));
}
#endif

void izs_find_ias_zone_cb(zb_uint8_t param)
{
  zb_bufid_t buf = param;
  zb_zdo_match_desc_resp_t *resp = (zb_zdo_match_desc_resp_t*)zb_buf_begin(buf);

  TRACE_MSG(TRACE_APP1, ">> izs_find_ias_zone_cb param %hd", (FMT__H, param));

  TRACE_MSG(TRACE_APP2, "resp match_len %hd", (FMT__H, resp->match_len));
  if (resp->status == ZB_ZDP_STATUS_SUCCESS && resp->match_len > 0)
  {
    /* The device just joined network and found CIE in this network */
    /* will not enforce Enroll - IAS CIE should send us enroll response */
    TRACE_MSG(TRACE_APP2, "IAS ZONE match desc received, continue normal work...", (FMT__0));

    IZS_SET_DEVICE_STATE(IZS_STATE_SENSOR_NORMAL);

    ZB_SCHEDULE_APP_ALARM_CANCEL(izs_check_ias_zone_found, ZB_ALARM_ANY_PARAM);
  }

  zb_buf_free(buf);
  TRACE_MSG(TRACE_APP1, "<< izs_find_ias_zone_cb", (FMT__0));
}

void izs_find_ias_zone_client_cb(zb_uint8_t buf)
{
  zb_zdo_match_desc_param_t *req;

  TRACE_MSG(TRACE_ZCL1, ">> izs_find_ias_zone_client %hd", (FMT__H, buf));

  req = zb_buf_initial_alloc(buf, sizeof(zb_zdo_match_desc_param_t) + (1) * sizeof(zb_uint16_t));

  req->nwk_addr = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
  req->addr_of_interest = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
  req->profile_id = ZB_AF_HA_PROFILE_ID;
  req->num_in_clusters = 0;
  /* We are searching for IAS ZONE Client */
  req->num_out_clusters = 1;
  req->cluster_list[0] = ZB_ZCL_CLUSTER_ID_IAS_ZONE;

  zb_zdo_match_desc_req(buf, izs_find_ias_zone_cb);

  ZB_SCHEDULE_APP_ALARM(izs_check_ias_zone_found, 0, IZS_WAIT_MATCH_IAS_ZONE);
  TRACE_MSG(TRACE_ZCL1, "<< izs_find_ias_zone_client", (FMT__0));
}

/* Callback to handle the stack events */
void zboss_signal_handler(zb_uint8_t param)
{
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  /* Get application signal from the buffer */
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_SKIP_STARTUP:
#ifndef ZB_MACSPLIT_HOST
        TRACE_MSG(TRACE_APP1, "ZB_ZDO_SIGNAL_SKIP_STARTUP: start join", (FMT__0));
        /**
         * Commissioning has not started yet, but ZBOSS schedule is already running.
         * Check initial device configuration and start commissioning by a selected way.
         */
        /* ZB_SCHEDULE_APP_CALLBACK(izs_start_join, IZS_FIRST_JOIN_ATTEMPT); */
        ZB_SCHEDULE_APP_CALLBACK(izs_device_start, 0);

#endif /* ZB_MACSPLIT_HOST */
        break;

#ifdef ZB_MACSPLIT_HOST
      case ZB_MACSPLIT_DEVICE_BOOT:
        TRACE_MSG(TRACE_APP1, "ZB_MACSPLIT_DEVICE_BOOT: start join", (FMT__0));
        /**
         * Commissioning has not started yet, but ZBOSS schedule is already running.
         * Check initial device configuration and start commissioning by a selected way.
         */
        /* ZB_SCHEDULE_APP_CALLBACK(izs_start_join, IZS_FIRST_JOIN_ATTEMPT); */
        ZB_SCHEDULE_APP_CALLBACK(izs_device_start, 0);
        break;
#endif /* ZB_MACSPLIT_HOST */
//! [signal_skip_startup]
      case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APP1, "steering signal", (FMT__0));
        /* FALLTHROUGH */
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
      {
        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));

        IZS_SET_DEVICE_STATE(IZS_STATE_STARTUP_COMPLETE);

        /* Successfully (re)joined. Clear rejoin backoff context (even it was not used...) */
        zb_zdo_rejoin_backoff_cancel();

        /* Update Short and Long Poll intervals after new join (will be applied automatically in
         * all other cases). */
        if (sig == ZB_BDB_SIGNAL_DEVICE_FIRST_START)
        {
          zb_uint32_t val = IZS_DEVICE_LONG_POLL_INTERVAL;
          ZB_ZCL_SET_ATTRIBUTE(IZS_DEVICE_ENDPOINT,
                               ZB_ZCL_CLUSTER_ID_POLL_CONTROL,
                               ZB_ZCL_CLUSTER_SERVER_ROLE,
                               ZB_ZCL_ATTR_POLL_CONTROL_LONG_POLL_INTERVAL_ID,
                               (zb_uint8_t *)&val,
                               ZB_FALSE);
          val = IZS_DEVICE_SHORT_POLL_INTERVAL;
          ZB_ZCL_SET_ATTRIBUTE(IZS_DEVICE_ENDPOINT,
                               ZB_ZCL_CLUSTER_ID_POLL_CONTROL,
                               ZB_ZCL_CLUSTER_SERVER_ROLE,
                               ZB_ZCL_ATTR_POLL_CONTROL_SHORT_POLL_INTERVAL_ID,
                               (zb_uint8_t *)&val,
                               ZB_FALSE);
        }

        /* Device joined - may start HW detector. */
        izs_hal_hw_enable();

#ifdef IZS_OTA
        TRACE_MSG(TRACE_OTA1, "Try restart OTA after rejoin", (FMT__0));
        zb_zcl_ota_restart_after_rejoin(IZS_DEVICE_ENDPOINT);
#endif

        if (IZS_DEVICE_IS_ENROLLED())
        {
          TRACE_MSG(TRACE_APP1, "rejoin, resume normal work", (FMT__0));
          /* resume normal operation mode */
          IZS_SET_DEVICE_STATE(IZS_STATE_SENSOR_NORMAL);

          /* Check queue - if we have something to send */
          if (g_device_ctx.retry_backoff_cnt)
          {
            zb_ret_t status;
            /* We are in "retry" scenario - do not call command send */
            /* But force retry timeout to expire instead */

            status = ZB_SCHEDULE_APP_ALARM_CANCEL(izs_resend_notification, ZB_ALARM_ANY_PARAM);
            if (status == RET_OK)
            {
              ZB_SCHEDULE_APP_CALLBACK(izs_resend_notification, g_device_ctx.notification_buf);
            }
          }
          else
          {
            izs_ias_zone_info_t *zone_info;

            /* Send an IAS Zone Status Change Notification after having rejoined the
             * network: make sure the parent has an up-to-date status of the sensor. */
            /* If notification queue is empty, force new measurement */
            zone_info = izs_ias_zone_get_element_from_queue();
            if (!zone_info && g_device_ctx.check_in_started)
            {
#ifndef IZS_APP_TEST_SEND_PERIODIC_NOTIFICATIONS
              /* force a ZoneStatusChange in case of silent tamper or motion alarms */
              ZB_SCHEDULE_APP_ALARM(izs_read_sensor_status, 0, (ZB_TIME_ONE_SECOND>>1));
#endif
            }
            else
            {
              izs_send_notification();
            }
          }

          /* go into checkin mode */
          if (!g_device_ctx.check_in_started)
          {
            /* force a ZoneStatusChange in case of silent tamper or motion alarms */
            ZB_SCHEDULE_APP_ALARM(izs_read_sensor_status, 0, (ZB_TIME_ONE_SECOND >> 1));
            izs_go_on_guard(param);
            param = 0;
          }
        }
        else /* !IZS_DEVICE_IS_ENROLLED() case */
        {
#ifdef FAST_POLLING_DURING_COMMISSIONING
          izs_start_fast_polling_for_commissioning(IZS_DEVICE_TURBO_POLL_AFTER_JOIN_DURATION * 1000l);
#endif /* FAST_POLLING_DURING_COMMISSIONING */
          /* Use Match descriptor request to check if IAS ZONE cluster is
          * supported in this network */
          ZB_SCHEDULE_APP_CALLBACK(izs_find_ias_zone_client_cb, param);
          param = 0;
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
        izs_leave_indication(leave_params->leave_type);
      }
      break;

//! [signal_sleep]
      case ZB_COMMON_SIGNAL_CAN_SLEEP:
      {
        /* zb_zdo_signal_can_sleep_params_t *can_sleep_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_can_sleep_params_t); */
        /* TODO: check if app is ready for sleep and sleep_tmo is ok, disable peripherals if needed */
#ifdef ZB_USE_SLEEP
        zb_sleep_now();
#endif
        /* TODO: enable peripherals if needed */
      }
      break;
//! [signal_sleep]

      case ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY:
      {
        izs_production_config_t *prod_cfg = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, izs_production_config_t);
        TRACE_MSG(TRACE_APP1, "Loading application production config", (FMT__0));

        ZB_ZCL_SET_STRING_VAL(g_device_ctx.basic_attr.mf_name, prod_cfg->manuf_name, ZB_ZCL_STRING_CONST_SIZE(prod_cfg->manuf_name));
        ZB_ZCL_SET_STRING_VAL(g_device_ctx.basic_attr.model_id, prod_cfg->model_id, ZB_ZCL_STRING_CONST_SIZE(prod_cfg->model_id));
        break;
      }

      default:
        TRACE_MSG(TRACE_APP1, "Unknown signal %hd", (FMT__H, sig));
    }
  }
  /* Error handling starts here */
  /* This signal will occur before any network activity, so it's safe to set defaults here */
  else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
  {
    TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));

    if (IZS_DEVICE_IS_ENROLLED())
    {
      IZS_SET_DEVICE_STATE(IZS_STATE_REJOIN_BACKOFF);
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
    }
    else
    {
      if (ZB_JOINED())
      {
        izs_full_reset_to_defaults(0);
      }
      else
      {
        /* make sure rejoin backoff context is cleaned */
        zb_zdo_rejoin_backoff_cancel();
        izs_retry_join();
      }
    }
  }

  if (param)
  {
    zb_buf_free(param);
  }
}

/* [HANDLER] */
zb_uint8_t izs_zcl_cmd_handler(zb_uint8_t param)
{
  zb_bufid_t zcl_cmd_buf = param;
  zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);
  zb_uint8_t cmd_processed = ZB_FALSE;
  zb_uint8_t src_ep;
  zb_uint16_t src_addr;

  TRACE_MSG(TRACE_APP1, ">> izs_zcl_cmd_handler %i", (FMT__H, param));
  ZB_ZCL_DEBUG_DUMP_HEADER(cmd_info);
  TRACE_MSG(TRACE_APP1, "payload size: %i", (FMT__D, zb_buf_len(zcl_cmd_buf)));

  if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_SRV)
  {
   if (cmd_info->cluster_id == ZB_ZCL_CLUSTER_ID_POLL_CONTROL &&
       cmd_info->is_common_command)
    {
      switch (cmd_info->cmd_id)
      {
        case ZB_ZCL_CMD_WRITE_ATTRIB:
          {
            zb_zcl_write_attr_req_t *write_attr_req;
            /* Check that we receive the Write Attributes cmd for the checkin Interval AttrId
               If so, start checkin cycle */
            write_attr_req = (zb_zcl_write_attr_req_t*)zb_buf_begin(zcl_cmd_buf);
            if (ZB_ZCL_ATTR_POLL_CONTROL_CHECKIN_INTERVAL_ID == write_attr_req->attr_id)
            {
              if (0 != ZB_ZCL_ATTR_GET32(write_attr_req->attr_value))
              {
                TRACE_MSG(TRACE_APP2, "check in interval is set", (FMT__0));
                if (!g_device_ctx.check_in_started)
                {
                  zb_uint8_t canceled_param = 0;

                  TRACE_MSG(TRACE_APP2, "save addr and start check-in", (FMT__0));
                  /* Set Addr and EP for Poll control */
                  src_ep = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint;
                  src_addr = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr;
                  zb_zcl_poll_control_set_client_addr(IZS_DEVICE_ENDPOINT, src_addr, src_ep);

                  canceled_param = zb_zcl_poll_control_stop();
                  if (canceled_param)
                  {
                    TRACE_MSG(TRACE_APP2, "free canceled buffer %hd", (FMT__H, canceled_param));
                    zb_buf_free(canceled_param);
                  }
                  zb_buf_get_out_delayed(izs_go_on_guard);
                }
              }
              else
              {
                TRACE_MSG(TRACE_APP2, "mark check-in as not started", (FMT__0));
                g_device_ctx.check_in_started = 0;
              }
            }
          }
          break;
        default:
          /* TRACE_MSG(TRACE_APP1, "skip command %hd", (FMT__H, cmd_info->cmd_id)); */
          break;
      }
    }
    else if (cmd_info->cluster_id == ZB_ZCL_CLUSTER_ID_IAS_ZONE &&
             cmd_info->is_common_command)
    {
       switch (cmd_info->cmd_id)
       {
          case ZB_ZCL_CMD_WRITE_ATTRIB:
            {
               zb_zcl_write_attr_req_t *write_attr_req;
               /* Check that we receive the Write Attributes cmd for the CIE address
                  If so, start fast polling */
               write_attr_req = (zb_zcl_write_attr_req_t*)zb_buf_begin(zcl_cmd_buf);
               if (ZB_ZCL_ATTR_IAS_ZONE_IAS_CIE_ADDRESS_ID == write_attr_req->attr_id)
               {
#ifdef FAST_POLLING_DURING_COMMISSIONING
                 izs_start_fast_polling_for_commissioning(IZS_DEVICE_TURBO_POLL_AFTER_CIE_ADDR_DURATION * 1000l);
#else
                 ZDO_CTX().poll_update_delay = IZS_DEVICE_INITIAL_FAST_POLL_DURATION;
#endif /* FAST_POLLING_DURING_COMMISSIONING */

                 g_device_ctx.zone_attr.cie_short_addr = cmd_info->addr_data.common_data.source.u.short_addr;
                 g_device_ctx.zone_attr.cie_ep = cmd_info->addr_data.common_data.src_endpoint;

                 TRACE_MSG(TRACE_APP1,
                          "CIE address is updated. New cie_short_addr = 0x%x, cie_ep = 0x%x ",
                          (FMT__H_H, g_device_ctx.zone_attr.cie_short_addr, g_device_ctx.zone_attr.cie_ep));

                 if (g_device_ctx.enrollment_method == ZB_ZCL_WWAH_ENROLLMENT_MODE_AUTO_ENROLL_REQUEST)
                 {
                   TRACE_MSG(TRACE_APP1, "auto enroll request mode - send EnrollRequest", (FMT__0));
                   g_device_ctx.enroll_req_generated = ZB_TRUE;

                   zb_buf_get_out_delayed(izs_send_enroll_req);
                 }
               }
             }
             break;
          default:
            break;
       }
    } else
    if (cmd_info->cluster_id == ZB_ZCL_CLUSTER_ID_IAS_ZONE &&
        !cmd_info->is_common_command)
    {
      switch (cmd_info->cmd_id)
      {
        case ZB_ZCL_CMD_IAS_ZONE_ZONE_ENROLL_RESPONSE_ID:
        {
          TRACE_MSG(TRACE_APP1, "ZB_ZCL_CMD_IAS_ZONE_ZONE_ENROLL_RESPONSE_ID", (FMT__0));

          src_ep = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint;
          src_addr = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr;

          /* Note: after a call to this function buffer becomes invalid */
          cmd_processed = zb_zcl_process_ias_zone_specific_commands(param);
          TRACE_MSG(TRACE_APP1, "cmd_processed %hd", (FMT__H, cmd_processed));

          if (IZS_DEVICE_IS_ENROLLED())
          {
            TRACE_MSG(TRACE_APP1, "device is enrolled, save data", (FMT__0));
            /* set cie short addr and ep values; correct values maybe
             * already set in izs_zcl_cmd_handler() */
            g_device_ctx.zone_attr.cie_ep = src_ep;
            g_device_ctx.zone_attr.cie_short_addr = src_addr;

            zb_zcl_poll_control_set_client_addr(IZS_DEVICE_ENDPOINT, src_addr, src_ep);
#ifdef ZB_USE_NVRAM
            /* If we fail, trace is given and assertion is triggered */
            (void)zb_nvram_write_dataset(ZB_NVRAM_HA_DATA);
#endif

            zb_buf_get_out_delayed(izs_go_on_guard);

#ifdef IZS_OTA
            izs_check_and_get_ota_server(0);
#endif

#ifdef FAST_POLLING_DURING_COMMISSIONING
            izs_start_fast_polling_for_commissioning(IZS_DEVICE_TURBO_POLL_AFTER_ENROLL_DURATION * 1000l);
#endif /* FAST_POLLING_DURING_COMMISSIONING */

            /* force a ZoneStatusChange in case of silent tamper or motion alarms */
            ZB_SCHEDULE_APP_ALARM(izs_read_sensor_status, 0, (ZB_TIME_ONE_SECOND>>1));
          }
          break;
        }

        default:
          TRACE_MSG(TRACE_APP1, "skip command %hd", (FMT__H, cmd_info->cmd_id));
          break;
      }
    }
  }

  TRACE_MSG(TRACE_APP1, "<< izs_zcl_cmd_handler processed %hd", (FMT__H, cmd_processed));
  return cmd_processed;
}
/* [HANDLER] */

void izs_joined_cont(zb_uint8_t param)
{
  /*
    - resume working with existing network
  */
  ZVUNUSED(param);

  TRACE_MSG(TRACE_APP1, ">> izs_joined_cont", (FMT__0));

  if ( ZB_JOINED() )
  {
    /* Try to rejoin if we are previously joined. */
    ZB_SCHEDULE_APP_CALLBACK(izs_start_bdb_commissioning, 0);
  }
  else
  {
    /* Not joined to network - need to initiate joining */
    TRACE_MSG(TRACE_APP1, "start joining, call zdo dev start", (FMT__0));
    ZB_SCHEDULE_APP_CALLBACK(izs_start_join, IZS_FIRST_JOIN_ATTEMPT);
  }

  TRACE_MSG(TRACE_APP1, "<< izs_joined_cont", (FMT__0));
}

void izs_leave_nwk_cb(zb_uint8_t param)
{
  IZS_SET_DEVICE_STATE(IZS_STATE_IDLE);

  if (param)
  {
    zb_buf_free(param);
  }

  /* We requested do leave, turn off HW detector. */
  izs_hal_hw_disable();

  /* restart join */
  ZB_SCHEDULE_APP_ALARM(izs_start_join, IZS_FIRST_JOIN_ATTEMPT, IZS_RESTART_JOIN_AFTER_LEAVE_DELAY);
}

void izs_full_reset_to_defaults(zb_uint8_t param)
{
  /* full reset to factory defaults:
     remove network parameters and then call common reset to factory defaults */

  TRACE_MSG(TRACE_APP1, ">> izs_full_reset_to_defaults", (FMT__0));

  IZS_SET_DEVICE_STATE(IZS_STATE_RESET_TO_DEFAULT);

  if (param)
  {
    zb_buf_free(param);
  }

  izs_basic_reset_to_defaults(0);

  zb_buf_get_out_delayed(izs_leave_nwk);

  TRACE_MSG(TRACE_APP1, "<< izs_full_reset_to_defaults", (FMT__0));
}

void izs_basic_reset_to_defaults(zb_uint8_t param)
{
  ZVUNUSED(param);
  TRACE_MSG(TRACE_APP1, ">> izs_basic_reset_to_defaults", (FMT__0));

  /* Reset to factory defaults:
     - Forget notifications from previous life
     - Reset all settings/HA attrs (except network and network commissioning)
     - Keep polling until ZDO leave or power cycle
  */

  ZB_SCHEDULE_APP_ALARM_CANCEL(izs_resend_notification, ZB_ALARM_ANY_PARAM);
  g_device_ctx.notification_in_progress = ZB_FALSE;
  g_device_ctx.retry_backoff_cnt = 0;
  izs_ias_zone_queue_init();
  g_device_ctx.enroll_req_generated = ZB_FALSE;

  /* Stop OTA upgrade */
#ifdef IZS_OTA
  zcl_ota_abort(IZS_DEVICE_ENDPOINT, 0);
#endif

  izs_clusters_attr_init(0);

  if (g_device_ctx.check_in_started)
  {
    zb_zcl_poll_control_start(zb_zcl_poll_control_stop(), IZS_DEVICE_ENDPOINT);
  }

  izs_init_default_reporting();

#ifdef ZB_USE_NVRAM
  /* If we fail, trace is given and assertion is triggered */
  (void)zb_nvram_write_dataset(ZB_NVRAM_HA_DATA);
  (void)zb_nvram_write_dataset(ZB_NVRAM_ZCL_REPORTING_DATA);
  (void)zb_nvram_write_dataset(ZB_NVRAM_HA_POLL_CONTROL_DATA);
#endif

  TRACE_MSG(TRACE_APP1, "<< izs_basic_reset_to_defaults", (FMT__0));
}

void izs_retry_join(void)
{
  /*
    - if error joining to network appeared, wait 5 sec and repeat joining
    - if IZS_JOIN_LIMIT is exceeded - stop trying rejoins and go
    sleep until some event happens (tamper button is pressed, movement, etc)
    this izs_retry_join() is used on App start only; if the device was
    enrolled and then lost network, rejoin backoff algorithm starts
    rejoin_backoff
  */
  TRACE_MSG(TRACE_APP1, "join_counter %hd", (FMT__H, g_device_ctx.join_counter));
  if (g_device_ctx.join_counter < IZS_JOIN_LIMIT)
  {
    ZB_SCHEDULE_APP_ALARM(izs_start_join, IZS_RETRY_JOIN_ATTEMPT, IZS_JOIN_RETRY_DELAY(g_device_ctx.join_counter));
  }
  else
  {
    TRACE_MSG(TRACE_APP1, "out of join attempts, go to sleep", (FMT__0));
    IZS_SET_DEVICE_STATE(IZS_STATE_NO_NWK_SLEEP);
  }
}

void izs_start_join(zb_uint8_t param)
{
/*
  - scan for network
  - if network is not found, wait for ((join_counter + 1) * 5) seconds and try again
  - repeat network searching 20 times
*/
  TRACE_MSG(TRACE_APP1, ">> izs_start_join param %hd", (FMT__H, param));

  izs_hal_led_blink(3);
  if (param == IZS_FIRST_JOIN_ATTEMPT)
  {
    g_device_ctx.join_counter = 0;
  }
  else
  {
    g_device_ctx.join_counter++;
  }

  ZB_SCHEDULE_APP_CALLBACK(izs_start_bdb_commissioning, 0);
  IZS_SET_DEVICE_STATE(IZS_STATE_START_JOIN);

  TRACE_MSG(TRACE_APP1, "<< izs_start_join", (FMT__0));
}

void izs_app_ctx_init(void)
{
  TRACE_MSG(TRACE_APP1, ">> izs_app_ctx_init", (FMT__0));

  ZB_MEMSET(&g_device_ctx, 0, sizeof(g_device_ctx));

  IZS_SET_DEVICE_STATE(IZS_STATE_APP_NOT_INIT);

  g_device_ctx.zone_attr.zone_state = ZB_ZCL_IAS_ZONE_ZONESTATE_NOT_ENROLLED;
  ZB_ASSERT(ZB_FALSE==g_device_ctx.detector_trouble);

  izs_ias_zone_queue_init();

  g_device_ctx.notification_buf = zb_buf_get_out();
  if (!g_device_ctx.notification_buf)
  {
    izs_critical_error();
  }

  TRACE_MSG(TRACE_APP1, "<< izs_app_ctx_init", (FMT__0));
}

void izs_send_notification(void)
{
  izs_ias_zone_info_t *zone_info;
  zb_uint16_t delay;
  zb_bool_t cmd_sent = ZB_FALSE;

/*
  - send notification command if device is enrolled
  - if not enrolled, but have valid CIE address, perform Enroll
  - if even CIE address is unknown, quit
*/

  TRACE_MSG(TRACE_APP1, ">> izs_send_notification", (FMT__0));
  TRACE_MSG(TRACE_APP1, "g_device_ctx.notification_buf = %p, queue size is %hd",
            (FMT__P_H, g_device_ctx.notification_buf, g_device_ctx.ias_zone_queue.written));
  /* check device is Enrolled or not. if NOT enrolled, send enroll request */
  if (!IZS_DEVICE_IS_ENROLLED())
  {
    TRACE_MSG(TRACE_APP1, "not enrolled check CIE addr", (FMT__0));
    if ( (!g_device_ctx.enroll_req_generated)
#ifdef IZS_IAS_ZONE_CHECK_CIE_IEEE
          && (!ZB_IEEE_ADDR_IS_ZERO(IZS_DEVICE_CIE_ADDR()))
#else
         /* Check only short CIE addr and endpoint (from match descr). */
         && (g_device_ctx.zone_attr.cie_short_addr != IZS_INIT_CIE_SHORT_ADDR) &&
         (g_device_ctx.zone_attr.cie_ep != IZS_INIT_CIE_ENDPOINT)
#endif
       )
    {
      TRACE_MSG(TRACE_APP1, "Send Enroll request", (FMT__0));
      g_device_ctx.enroll_req_generated = ZB_TRUE;

      zb_buf_get_out_delayed(izs_send_enroll_req);
#ifdef FAST_POLLING_DURING_COMMISSIONING
      /* Fast polling is started to go smoothly through commissioning procedure */
      izs_start_fast_polling_for_commissioning(IZS_DEVICE_TURBO_POLL_ENROLL_REQ_DURATION * 1000l);
#endif /* FAST_POLLING_DURING_COMMISSIONING */
    }
    else
    {
      TRACE_MSG(TRACE_APP1, "cie addr is NULL - skip sending notification", (FMT__0));
    }
  }
  else if (g_device_ctx.retry_backoff_cnt || zb_zdo_rejoin_backoff_is_running())
  {
    zb_ret_t status;
    /* We are in "retry" scenario - do not call command send */
    /* But force retry timeout to expire instead */

    g_device_ctx.retry_backoff_cnt = 0;

    status = ZB_SCHEDULE_APP_ALARM_CANCEL(izs_resend_notification, ZB_ALARM_ANY_PARAM);
    if (status == RET_OK)
    {
      izs_retry_backoff(g_device_ctx.notification_buf);
    }

    /* try to force rejoin backoff */
    zb_zdo_rejoin_backoff_force();
  }
  else
  {
    /* Send notification command only if device is enrolled */
    /* check if we have a buffer => command is not begin sent now, continue  */
    TRACE_MSG(TRACE_APP1, "g_device_ctx.notification_buf = %p, queue size is %hd", (FMT__P_H, g_device_ctx.notification_buf, g_device_ctx.ias_zone_queue.written));
    TRACE_MSG(TRACE_APP1, "CIE EP: %hd", (FMT__H, g_device_ctx.zone_attr.cie_ep));
    if (!g_device_ctx.notification_in_progress && g_device_ctx.zone_attr.cie_ep)
    {
      while (1)
      {
        zone_info = izs_ias_zone_get_element_from_queue();
        if (!zone_info)
        {
          /* Nothing to send - queue is empty */
          TRACE_MSG(TRACE_APP1, "g_device_ctx.notification_buf = %p, queue size is %hd", (FMT__P_H, g_device_ctx.notification_buf, g_device_ctx.ias_zone_queue.written));
          TRACE_MSG(TRACE_APP1, "queue is empty", (FMT__0));
          break;
        }

        delay = izs_calc_time_delta_qsec(zone_info->timestamp);

        TRACE_MSG(TRACE_APP2, "call ias_zone_set_status, notification_buf %hd",
                  (FMT__H, g_device_ctx.notification_buf));
        cmd_sent = zb_zcl_ias_zone_set_status(
          IZS_DEVICE_ENDPOINT,
          zone_info->ias_status,
          delay,
          g_device_ctx.notification_buf);

        if (cmd_sent)
        {
          /* Command is sent. We are sending only 1 command, then waiting for Ack (or cmd fail...)  */
          TRACE_MSG(TRACE_APP1, "cmd is sent", (FMT__0));
          g_device_ctx.notification_in_progress = ZB_TRUE;
          g_device_ctx.retry_backoff_cnt = 0;
          break;
        }
        else
        {
          /* release if no notification should be send. Check sending conditions */
          TRACE_MSG(TRACE_APP1, "no notification should be send, release", (FMT__0));
          izs_ias_zone_release_element_from_queue();
        }
      }
    }
  }

  TRACE_MSG(TRACE_APP1, "<< izs_send_notification", (FMT__0));
}

void izs_ias_zone_notification_cb(zb_bufid_t buf)
{
  zb_zcl_command_send_status_t *cmd_send_status = ZB_BUF_GET_PARAM(buf, zb_zcl_command_send_status_t);

  TRACE_MSG(TRACE_APP1, ">> izs_ias_zone_notification_cb %d", (FMT__H, buf));

  if (g_device_ctx.notification_buf != buf)
  {
    zb_buf_free(buf);
    buf = g_device_ctx.notification_buf;
  }

  if (cmd_send_status->status == RET_OK)
  {
    /* Notification is sent OK. Put buffer back to context - it will
     * be reused. Remove element from queue here - it was NOT removed
     * on command send */
    TRACE_MSG(TRACE_APP2, "notification sent OK, buf %p", (FMT__P, buf));

    g_device_ctx.notification_in_progress = ZB_FALSE;

    TRACE_MSG(TRACE_APP1, "g_device_ctx.notification_buf = %p, queue size is %h", (FMT__P_H, g_device_ctx.notification_buf, g_device_ctx.ias_zone_queue.written));
    izs_ias_zone_release_element_from_queue();
    g_device_ctx.retry_backoff_cnt = 0;
    izs_send_notification();
  }
  else
  {
    if (g_device_ctx.notification_in_progress)
    {
      /* Error sending notification: start retry backoff algorithm */
      izs_retry_backoff(buf);
    }
  }

  TRACE_MSG(TRACE_APP1, "<< izs_ias_zone_notification_cb", (FMT__0));
}

#ifdef IZS_WWAH
zb_bool_t izs_wwah_app_event_retry_should_discard_event(zb_uint8_t failed_attempts)
{
  return (zb_bool_t)(0xFF != MAX_REDELIVERY_ATTEMPTS() && failed_attempts > MAX_REDELIVERY_ATTEMPTS());
}

/**
 * Returns a timeout (in seconds) to wait the next re-delivery attempt
 * if delivery failed last 'failed_attempts' times
 */
zb_uint16_t izs_wwah_app_event_retry_get_next_timeout(zb_uint8_t failed_attempts)
{
  zb_uint16_t timeout;
  zb_uindex_t i;

  timeout = FIRST_BACKOFF_TIME();

  for (i = 0; i < failed_attempts-1 && timeout < MAX_BACKOFF_TIME(); i++)
  {
    timeout *= COMMON_RATIO();
  }

  return timeout;
}

void izs_wwah_retry_backoff(zb_uint8_t param)
{
  zb_uint16_t timeout;

  /* Retry backoff algorithm: If notification send fails (no APS Ack),
   * resend this command in 2-5-10-15-20-... seconds. Timeout should
   * not be more then 30 minutes. */

  TRACE_MSG(TRACE_APP1, ">> izs_wwah_retry_backoff %hd", (FMT__H, param));

  g_device_ctx.retry_backoff_cnt++;

  if(izs_wwah_app_event_retry_should_discard_event(g_device_ctx.retry_backoff_cnt))
  {
    TRACE_MSG(TRACE_APP2, "max retry count reached; drop the event", (FMT__0));
    izs_ias_zone_release_element_from_queue();
  }

  if(!izs_ias_zone_queue_is_empty())
  {
    timeout = izs_wwah_app_event_retry_get_next_timeout(g_device_ctx.retry_backoff_cnt);
    TRACE_MSG(TRACE_APP2, "retry after %d sec.", (FMT__D, timeout));

    ZB_SCHEDULE_APP_ALARM(izs_resend_notification, param, timeout * ZB_TIME_ONE_SECOND);
  }
  else
  {
    zb_buf_free(param);
  }
}
#else
void izs_custom_retry_backoff(zb_uint8_t param)
{
  zb_uint16_t timeout;

  /* Retry backoff algorithm: If notification send fails (no APS Ack),
   * resent this command in 2-5-10-15-20-... seconds. Timeout should
   * not be more then 30 minutes. */

  TRACE_MSG(TRACE_APP1, ">> izs_retry_backoff %hd", (FMT__H, param));

  g_device_ctx.retry_backoff_cnt++;
  if (g_device_ctx.retry_backoff_cnt == 1)
  {
    timeout = IZS_RETRY_BACKOFF_FIRST_TIMEOUT; /* The first retry timeout in seconds */
  }
  else
  {
    if (g_device_ctx.retry_backoff_cnt >= IZS_RETRY_BACKOFF_MAX_COUNT)
    {
      g_device_ctx.retry_backoff_cnt = IZS_RETRY_BACKOFF_MAX_COUNT;
      timeout = IZS_RETRY_BACKOFF_MAX_TIMEOUT; /* in seconds */
    }
    else
    {
      timeout = (1 << g_device_ctx.retry_backoff_cnt); /* in seconds */
    }
  }
  TRACE_MSG(TRACE_APP1, "retry_backoff_cnt %d timeout %d sec", (FMT__D_D, g_device_ctx.retry_backoff_cnt, timeout));
  ZB_SCHEDULE_APP_ALARM(izs_resend_notification, param, timeout * ZB_TIME_ONE_SECOND);

  TRACE_MSG(TRACE_APP1, "<< izs_retry_backoff %hd", (FMT__H, param));
}
#endif

void izs_retry_backoff(zb_uint8_t param)
{
#ifdef IZS_WWAH
  izs_wwah_retry_backoff(param);
#else
  izs_custom_retry_backoff(param);
#endif
}

void izs_resend_notification(zb_bufid_t buf)
{
  zb_zcl_ias_zone_status_change_not_t notification;
  zb_zcl_parse_status_t parse_status;
  zb_uint16_t delay;
  izs_ias_zone_info_t *zone_info;

  TRACE_MSG(TRACE_APP1, ">> izs_resend_notification %hd", (FMT__H, buf));

  /* NOTE: pure ZCL packet is in the buffer */
  ZB_ZCL_CUT_HEADER(buf);
  {
    ZB_ZCL_IAS_ZONE_GET_STATUS_CHANGE_NOTIFICATION_REQ(&notification, buf, parse_status);
    if (parse_status == ZB_ZCL_PARSE_STATUS_SUCCESS)
    {
      zone_info = izs_ias_zone_get_element_from_queue();
      if (zone_info)
      {
        delay = izs_calc_time_delta_qsec(zone_info->timestamp);
        ZB_ZCL_IAS_ZONE_SEND_STATUS_CHANGE_NOTIFICATION_REQ(
          buf, g_device_ctx.zone_attr.cie_short_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
          g_device_ctx.zone_attr.cie_ep, IZS_DEVICE_ENDPOINT, ZB_AF_HA_PROFILE_ID, izs_ias_zone_notification_cb,
          notification.zone_status, notification.extended_status, notification.zone_id,
          delay);
      }
    }
  }
  TRACE_MSG(TRACE_APP1, "<< izs_resend_notification", (FMT__0));
}

/* Function is called when device is ready to serve. There are 3 cases:
   - new device enrolled to network
   - device is already enrolled, it is restarted => joined network
   and found parent device (match descriptor command)
   - poll control interval changed
*/
void izs_go_on_guard(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> izs_go_on_guard %hd", (FMT__H, param));

  izs_measure_batteries();

  if (!g_device_ctx.check_in_started)
  {
    zb_zcl_attr_t *attr_desc = zb_zcl_get_attr_desc_a(IZS_DEVICE_ENDPOINT,
        ZB_ZCL_CLUSTER_ID_POLL_CONTROL, ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_POLL_CONTROL_CHECKIN_INTERVAL_ID);
    ZB_ASSERT(attr_desc);

    zb_zcl_poll_control_start(param, IZS_DEVICE_ENDPOINT);
    g_device_ctx.check_in_started = 1;
  }
  else
  {
    if (param)
    {
      zb_buf_free(param);
    }
  }

  /* kick notification queue handling... */
  izs_send_notification();

  TRACE_MSG(TRACE_APP1, "<< izs_go_on_guard", (FMT__0));
}

void izs_critical_error(void)
{
  TRACE_MSG(TRACE_APP1, "izs_critical_error", (FMT__0));

  ZB_ASSERT(0);
}
