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
#define ZB_TRACE_FILE_ID 40021
#include "izs_device.h"
#include "zboss_api.h"

#if ! defined ZB_ED_ROLE
#error define ZB_ED_ROLE to compile ze tests
#endif

#ifdef ZTT_USER_INTERACTION
/* remove nordic dependent headers */
#endif

#define BUTTON_1 ZB_BOARD_BUTTON_1
#define BUTTON_2 ZB_BOARD_BUTTON_2
#define BUTTON_3 ZB_BOARD_BUTTON_3

/* Handler for specific zcl commands */
zb_uint8_t izs_zcl_cmd_handler(zb_uint8_t param);
void izs_check_ias_zone_found(zb_uint8_t param);

static void izs_set_default_configuration_values();

izs_device_ctx_t g_device_ctx;

#ifdef IAS_ACE_APP
zb_zcl_ias_ace_zone_table_t g_ias_ace_zone_table_list[IAS_ACE_ZONE_TABLE_LIST_SIZE];
zb_uint8_t g_ias_ace_zone_table_list_bypassed_status[IAS_ACE_ZONE_TABLE_LIST_SIZE];

/*izs_device_panel_status_list_t g_ias_ace_zone_panel_status;*/
zb_zcl_ias_ace_panel_status_changed_t g_ias_ace_zone_panel_status;
#endif
/******************* Declare attributes ************************/

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

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list,
  &g_device_ctx.identify_attr.identify_time);

/* IAS Zone cluster attributes data */
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

#ifdef IAS_ACE_APP
ZB_ZCL_DECLARE_IAS_ACE_ATTRIB_LIST(
  ias_ace_attr_list,
  &g_device_ctx.ias_ace_attr.ias_ace_zone_table_length,
  &g_device_ctx.ias_ace_attr.ias_ace_zone_table_list);
#endif

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

#ifdef IZS_OTA
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
zb_zcl_attr_t ota_upgrade_attr_list[] = { ZB_ZCL_NULL_ID, 0, 0, NULL };
#endif

/********************* Declare device **************************/
#ifdef IAS_ACE_APP

/** @endcond */
/**
 *  @brief Declare cluster list for IAS Zone
 *  @param cluster_list_name [IN] - cluster list variable name.
 *  @param basic_attr_list [IN] - attribute list for Basic cluster.
 *  @param identify_attr_list [IN] - attribute list for Identify cluster.
 *  @param ias_zone_attr_list [IN] - attribute list for IAS Zone cluster.
 *  @param ias_ace_attr_list [IN] - attribute list for IAS ACE cluster.
 *  @param poll_ctrl_attr_list [IN] - attribute list for Level Control cluster.
 *  @param power_config_attr_list [IN] - attribute list for Power Configuration cluster.
 *  @param ota_upgrade_attr_list [IN] - attribute list for OTA Upgrade cluster.
 */
#define ZB_HA_DECLARE_IAS_ZONE_IAS_ACE_CLUSTER_LIST(                       \
  cluster_list_name,                                                       \
  basic_attr_list,                                                         \
  identify_attr_list,                                                      \
  ias_zone_attr_list,                                                      \
  ias_ace_attr_list,                                                       \
  poll_ctrl_attr_list,                                                     \
  power_config_attr_list,                                                  \
  ota_upgrade_attr_list)                                                   \
  zb_zcl_cluster_desc_t cluster_list_name[] =                              \
  {                                                                        \
    ZB_ZCL_CLUSTER_DESC(                                                   \
      ZB_ZCL_CLUSTER_ID_BASIC,                                             \
      ZB_ZCL_ARRAY_SIZE(basic_attr_list, zb_zcl_attr_t),                   \
      (basic_attr_list),                                                   \
      ZB_ZCL_CLUSTER_SERVER_ROLE,                                          \
      ZB_ZCL_MANUF_CODE_INVALID                                            \
      ),                                                                   \
    ZB_ZCL_CLUSTER_DESC(                                                   \
      ZB_ZCL_CLUSTER_ID_IDENTIFY,                                          \
      ZB_ZCL_ARRAY_SIZE(identify_attr_list, zb_zcl_attr_t),                \
      (identify_attr_list),                                                \
      ZB_ZCL_CLUSTER_SERVER_ROLE,                                          \
      ZB_ZCL_MANUF_CODE_INVALID                                            \
      ),                                                                   \
    ZB_ZCL_CLUSTER_DESC(                                                   \
      ZB_ZCL_CLUSTER_ID_IAS_ZONE,                                          \
      ZB_ZCL_ARRAY_SIZE(ias_zone_attr_list, zb_zcl_attr_t),                \
      (ias_zone_attr_list),                                                \
      ZB_ZCL_CLUSTER_SERVER_ROLE,                                          \
      ZB_ZCL_MANUF_CODE_INVALID                                            \
      ),                                                                   \
    ZB_ZCL_CLUSTER_DESC(                                                   \
      ZB_ZCL_CLUSTER_ID_IAS_ACE,                                           \
      ZB_ZCL_ARRAY_SIZE(ias_ace_attr_list, zb_zcl_attr_t),                 \
      (ias_ace_attr_list),                                                 \
      ZB_ZCL_CLUSTER_SERVER_ROLE,                                          \
      ZB_ZCL_MANUF_CODE_INVALID                                            \
      ),                                                                   \
    ZB_ZCL_CLUSTER_DESC(                                                   \
      ZB_ZCL_CLUSTER_ID_POLL_CONTROL,                                      \
      ZB_ZCL_ARRAY_SIZE(poll_ctrl_attr_list, zb_zcl_attr_t),               \
      (poll_ctrl_attr_list),                                               \
      ZB_ZCL_CLUSTER_SERVER_ROLE,                                          \
      ZB_ZCL_MANUF_CODE_INVALID                                            \
      ),                                                                   \
    ZB_ZCL_CLUSTER_DESC(                                                   \
      ZB_ZCL_CLUSTER_ID_POWER_CONFIG,                                      \
      ZB_ZCL_ARRAY_SIZE(power_config_attr_list, zb_zcl_attr_t),            \
      (power_config_attr_list),                                            \
      ZB_ZCL_CLUSTER_SERVER_ROLE,                                          \
      ZB_ZCL_MANUF_CODE_INVALID                                            \
      ),                                                                   \
    ZB_ZCL_CLUSTER_DESC(                                                   \
      ZB_ZCL_CLUSTER_ID_OTA_UPGRADE,                                       \
      ZB_ZCL_ARRAY_SIZE(ota_upgrade_attr_list, zb_zcl_attr_t),             \
      (ota_upgrade_attr_list),                                             \
      ZB_ZCL_CLUSTER_CLIENT_ROLE,                                          \
      ZB_ZCL_MANUF_CODE_INVALID                                            \
     )                                                                     \
  }

  ZB_HA_DECLARE_IAS_ZONE_IAS_ACE_CLUSTER_LIST(
  cs_device_clusters,
  basic_attr_list,
  identify_attr_list,
  ias_zone_attr_list,
  ias_ace_attr_list,
  poll_ctrl_attr_list,
  power_config_attr_list,
  ota_upgrade_attr_list);
#else
ZB_HA_DECLARE_IAS_ZONE_CLUSTER_LIST(
  cs_device_clusters,
  basic_attr_list,
  identify_attr_list,
  ias_zone_attr_list,
  poll_ctrl_attr_list,
  power_config_attr_list,
  ota_upgrade_attr_list);
#endif

ZB_HA_DECLARE_IAS_ZONE_EP(cs_device_ep, IZS_DEVICE_ENDPOINT, cs_device_clusters);
ZB_HA_DECLARE_IAS_ZONE_CTX(izs_device_zcl_ctx, cs_device_ep);

/*************************************************************************/

// Indirect Poll rate during commissioning
#ifdef FAST_POLLING_DURING_COMMISSIONING

void izs_start_fast_polling_for_commissioning(zb_time_t fast_poll_timeout_ms)
{
  zb_zdo_pim_turbo_poll_continuous_leave(0);
  zb_zdo_pim_set_turbo_poll_max(IZS_DEVICE_COMMISSIONING_POLL_RATE);
  zb_zdo_pim_permit_turbo_poll(ZB_TRUE);
  zb_zdo_pim_start_turbo_poll_continuous(fast_poll_timeout_ms);
}

#endif /* FAST_POLLING_DURING_COMMISSIONING */

#ifdef ZTT_USER_INTERACTION

zb_uint8_t g_ias_zone_status_cntr = 0;

void button_2_handler(zb_uint8_t param)
{
  ZVUNUSED(param);

  zb_bufid_t buf = ZB_GET_OUT_BUF();
  switch (g_ias_zone_status_cntr)
  {
    case 0:
      ZB_ZCL_IAS_ZONE_SEND_STATUS_CHANGE_NOTIFICATION_REQ(
          buf, g_device_ctx.zone_attr.cie_short_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
          g_device_ctx.zone_attr.cie_ep, IZS_DEVICE_ENDPOINT, ZB_AF_HA_PROFILE_ID, izs_ias_zone_notification_cb,
          1, 0, g_device_ctx.zone_attr.zone_id,
          10);
      break;
    case 1:
      ZB_ZCL_IAS_ZONE_SEND_STATUS_CHANGE_NOTIFICATION_REQ(
          buf, g_device_ctx.zone_attr.cie_short_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
          g_device_ctx.zone_attr.cie_ep, IZS_DEVICE_ENDPOINT, ZB_AF_HA_PROFILE_ID, izs_ias_zone_notification_cb,
          2, 0, g_device_ctx.zone_attr.zone_id,
          10);
      break;
    case 2:
      ZB_ZCL_IAS_ZONE_SEND_STATUS_CHANGE_NOTIFICATION_REQ(
          buf, g_device_ctx.zone_attr.cie_short_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
          g_device_ctx.zone_attr.cie_ep, IZS_DEVICE_ENDPOINT, ZB_AF_HA_PROFILE_ID, izs_ias_zone_notification_cb,
          4, 0, g_device_ctx.zone_attr.zone_id,
          10);
      break;
    case 3:
      ZB_ZCL_IAS_ZONE_SEND_STATUS_CHANGE_NOTIFICATION_REQ(
          buf, g_device_ctx.zone_attr.cie_short_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
          g_device_ctx.zone_attr.cie_ep, IZS_DEVICE_ENDPOINT, ZB_AF_HA_PROFILE_ID, izs_ias_zone_notification_cb,
          8, 0, g_device_ctx.zone_attr.zone_id,
          10);
      break;
    case 4:
      ZB_ZCL_IAS_ZONE_SEND_STATUS_CHANGE_NOTIFICATION_REQ(
          buf, g_device_ctx.zone_attr.cie_short_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
          g_device_ctx.zone_attr.cie_ep, IZS_DEVICE_ENDPOINT, ZB_AF_HA_PROFILE_ID, izs_ias_zone_notification_cb,
          16, 0, g_device_ctx.zone_attr.zone_id,
          10);
      break;
    case 5:
      ZB_ZCL_IAS_ZONE_SEND_STATUS_CHANGE_NOTIFICATION_REQ(
          buf, g_device_ctx.zone_attr.cie_short_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
          g_device_ctx.zone_attr.cie_ep, IZS_DEVICE_ENDPOINT, ZB_AF_HA_PROFILE_ID, izs_ias_zone_notification_cb,
          1, 0, g_device_ctx.zone_attr.zone_id,
          10);
      break;
    case 6:
      ZB_ZCL_IAS_ZONE_SEND_STATUS_CHANGE_NOTIFICATION_REQ(
          buf, g_device_ctx.zone_attr.cie_short_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
          g_device_ctx.zone_attr.cie_ep, IZS_DEVICE_ENDPOINT, ZB_AF_HA_PROFILE_ID, izs_ias_zone_notification_cb,
          0, 0, g_device_ctx.zone_attr.zone_id,
          10);
      break;
    case 7:
      ZB_ZCL_IAS_ZONE_SEND_STATUS_CHANGE_NOTIFICATION_REQ(
          buf, g_device_ctx.zone_attr.cie_short_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
          g_device_ctx.zone_attr.cie_ep, IZS_DEVICE_ENDPOINT, ZB_AF_HA_PROFILE_ID, izs_ias_zone_notification_cb,
          32, 0, g_device_ctx.zone_attr.zone_id,
          10);
      break;
    case 8:
      ZB_ZCL_IAS_ZONE_SEND_STATUS_CHANGE_NOTIFICATION_REQ(
          buf, g_device_ctx.zone_attr.cie_short_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
          g_device_ctx.zone_attr.cie_ep, IZS_DEVICE_ENDPOINT, ZB_AF_HA_PROFILE_ID, izs_ias_zone_notification_cb,
          64, 0, g_device_ctx.zone_attr.zone_id,
          10);
      break;
    case 9:
      ZB_ZCL_IAS_ZONE_SEND_STATUS_CHANGE_NOTIFICATION_REQ(
          buf, g_device_ctx.zone_attr.cie_short_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
          g_device_ctx.zone_attr.cie_ep, IZS_DEVICE_ENDPOINT, ZB_AF_HA_PROFILE_ID, izs_ias_zone_notification_cb,
          128, 0, g_device_ctx.zone_attr.zone_id,
          10);
      break;
    case 10:
      ZB_ZCL_IAS_ZONE_SEND_STATUS_CHANGE_NOTIFICATION_REQ(
          buf, g_device_ctx.zone_attr.cie_short_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
          g_device_ctx.zone_attr.cie_ep, IZS_DEVICE_ENDPOINT, ZB_AF_HA_PROFILE_ID, izs_ias_zone_notification_cb,
          256, 0, g_device_ctx.zone_attr.zone_id,
          10);
      break;
    case 11:
      ZB_ZCL_IAS_ZONE_SEND_STATUS_CHANGE_NOTIFICATION_REQ(
          buf, g_device_ctx.zone_attr.cie_short_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
          g_device_ctx.zone_attr.cie_ep, IZS_DEVICE_ENDPOINT, ZB_AF_HA_PROFILE_ID, izs_ias_zone_notification_cb,
          512, 0, g_device_ctx.zone_attr.zone_id,
          10);
      break;
    default:
      break;
  }
  g_ias_zone_status_cntr++;
}

#ifdef IAS_ACE_APP
void send_zone_status_changed(zb_uint8_t param)
{
  zb_uint16_t addr = 0;
  ZB_ZCL_IAS_ACE_SEND_ZONE_STATUS_CHANGED_REQ(
    buf, addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, 1, IZS_DEVICE_ENDPOINT, 0x0104, 0,
    NULL, g_device_ctx.ias_ace_attr.ias_ace_zone_table_list[1].zone_id, g_device_ctx.ias_ace_attr.ias_ace_zone_table_list[1].zone_type, 0, g_device_ctx.ias_ace_attr.ias_ace_zone_table_list[1].zone_label);
}

void button_3_handler(zb_uint8_t param)
{
  ZVUNUSED(param);

  zb_buf_get_out_delayed(send_zone_status_changed);
}

void send_panel_status_changed(zb_uint8_t param)
{
  zb_uint16_t addr = 0;

  g_ias_ace_zone_panel_status.panel_status = ZB_ZCL_IAS_ACE_PANEL_STATUS_EXIT_DELAY;
  g_ias_ace_zone_panel_status.seconds_remaining = param;
  g_ias_ace_zone_panel_status.aud_notification = ZB_ZCL_IAS_ACE_AUD_NOTIFICATION_MUTE;
  g_ias_ace_zone_panel_status.alarm_status = ZB_ZCL_IAS_ACE_ALARM_STATUS_FIRE;

  ZB_ZCL_IAS_ACE_SEND_PANEL_STATUS_CHANGED_REQ(
    buf, addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, 1, IZS_DEVICE_ENDPOINT, 0x0104, 0,
    NULL, g_ias_ace_zone_panel_status.panel_status, g_ias_ace_zone_panel_status.seconds_remaining,
    g_ias_ace_zone_panel_status.aud_notification, g_ias_ace_zone_panel_status.alarm_status);

}

void button_4_handler(zb_uint8_t param)
{
  ZVUNUSED(param);

  zb_buf_get_out_delayed(send_panel_status_changed);
}

#endif

static void gpio_init(void)
{
  zb_osif_led_button_init();

  //APP_ERROR_CHECK(err_code);
#ifdef ZB_USE_BUTTONS
  zb_button_register_handler(BUTTON_1, 0, button_2_handler);
#endif

#ifdef IAS_ACE_APP
#ifdef ZB_USE_BUTTONS
  zb_button_register_handler(BUTTON_2, 0, button_3_handler);
  zb_button_register_handler(BUTTON_3, 0, button_4_handler);
#endif
#endif
}
#endif

MAIN()
{
  ARGV_UNUSED;

  //ZB_SET_TRACE_OFF();
  ZB_SET_TRAF_DUMP_OFF();

  ZB_INIT("izs_device");

  /****************** Register Device ********************************/
  ZB_AF_REGISTER_DEVICE_CTX(&izs_device_zcl_ctx);
  ZB_AF_SET_ENDPOINT_HANDLER(IZS_DEVICE_ENDPOINT, izs_zcl_cmd_handler);
  ZB_AF_SET_IDENTIFY_NOTIFICATION_HANDLER(IZS_DEVICE_ENDPOINT, izs_identify_notification);
  /* for OTA */
  ZB_ZCL_REGISTER_DEVICE_CB(izs_device_interface_cb);

#ifdef ZTT_USER_INTERACTION
  gpio_init();
#endif
  /*** Init application structures, ZB settings ***/
  /* h/w init is called from izs_device_app_init */
  izs_device_app_init(0);

  if (zboss_start_no_autostart() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zb_zdo_dev_init failed", (FMT__0));
  }
  else
  {
    /* scheduler infinit loop */
    zboss_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}

static void izs_set_default_configuration_values()
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

void izs_clusters_attr_init(zb_uint8_t param)
{
#ifdef IZS_OTA
  zb_ieee_addr_t default_ota_server_addr = IZS_OTA_UPGRADE_SERVER;
#endif

  TRACE_MSG(TRACE_APP1, ">> izs_clusters_attr_init", (FMT__0));

  ZVUNUSED(param);

  /* Basic cluster attributes data */
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

  /* Identify cluster attributes data */
  g_device_ctx.identify_attr.identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

  /* IAS Zone cluster attributes data */
  g_device_ctx.zone_attr.zone_type = ZB_ZCL_IAS_ZONE_ZONETYPE_MOTION;
  g_device_ctx.zone_attr.zone_status = IZS_INIT_ZONE_STATUS;
  g_device_ctx.zone_attr.zone_id = IZS_INIT_ZONE_ID;
  g_device_ctx.zone_attr.zone_state = ZB_ZCL_IAS_ZONE_ZONESTATE_NOT_ENROLLED;
  ZB_IEEE_ADDR_ZERO(g_device_ctx.zone_attr.cie_addr);
  g_device_ctx.zone_attr.cie_short_addr = IZS_INIT_CIE_SHORT_ADDR;
  g_device_ctx.zone_attr.cie_ep = IZS_INIT_CIE_ENDPOINT;
  g_device_ctx.zone_attr.number_of_zone_sens_levels_supported =
    ZB_ZCL_IAS_ZONE_NUMBER_OF_ZONE_SENSITIVITY_LEVELS_SUPPORTED_DEFAULT_VALUE;

#ifdef IAS_ACE_APP
  {
    zb_uint8_t zone_address_1[] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11};
    zb_uint8_t zone_address_2[] = {0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22};
    zb_uint8_t zone_address_3[] = {0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33};

    g_device_ctx.ias_ace_attr.ias_ace_zone_table_length = IAS_ACE_ZONE_TABLE_LIST_SIZE;

    g_device_ctx.ias_ace_attr.ias_ace_zone_table_list[0].zone_id = 0x00;
    g_device_ctx.ias_ace_attr.ias_ace_zone_table_list[0].zone_type = ZB_ZCL_IAS_ZONE_ZONETYPE_STANDARD_CIE;
    ZB_64BIT_ADDR_COPY(&g_device_ctx.ias_ace_attr.ias_ace_zone_table_list[0].zone_address, zone_address_1);
    g_device_ctx.ias_ace_attr.ias_ace_zone_table_list[0].zone_label = "\nfirst zone";

    g_device_ctx.ias_ace_attr.ias_ace_zone_table_list[1].zone_id = 0x01;
    g_device_ctx.ias_ace_attr.ias_ace_zone_table_list[1].zone_type = ZB_ZCL_IAS_ZONE_ZONETYPE_FIRE_SENSOR;
    ZB_64BIT_ADDR_COPY(&g_device_ctx.ias_ace_attr.ias_ace_zone_table_list[1].zone_address, zone_address_2);
    g_device_ctx.ias_ace_attr.ias_ace_zone_table_list[1].zone_label = "\nsecon zone";

    g_device_ctx.ias_ace_attr.ias_ace_zone_table_list[2].zone_id = 0x02;
    g_device_ctx.ias_ace_attr.ias_ace_zone_table_list[2].zone_type = ZB_ZCL_IAS_ZONE_ZONETYPE_WATER_SENSOR;
    ZB_64BIT_ADDR_COPY(&g_device_ctx.ias_ace_attr.ias_ace_zone_table_list[2].zone_address, zone_address_3);
    g_device_ctx.ias_ace_attr.ias_ace_zone_table_list[2].zone_label = "\nthird zone";

    ZB_MEMSET(g_ias_ace_zone_table_list_bypassed_status, ZB_ZCL_IAS_ACE_BYPASS_RESULT_NOT_BYPASSED, IAS_ACE_ZONE_TABLE_LIST_SIZE*sizeof(zb_uint8_t));
    g_ias_ace_zone_table_list_bypassed_status[1] = ZB_ZCL_IAS_ACE_BYPASS_RESULT_NOT_ALLOWED;

  }
#endif

  /* Poll Control cluster attributes data */
  g_device_ctx.poll_control_attr.checkin_interval       = IZS_DEVICE_CHECKIN_INTERVAL;
  g_device_ctx.poll_control_attr.long_poll_interval     = IZS_DEVICE_LONG_POLL_INTERVAL;
  g_device_ctx.poll_control_attr.short_poll_interval    = IZS_DEVICE_SHORT_POLL_INTERVAL;
  g_device_ctx.poll_control_attr.fast_poll_timeout      = ZB_ZCL_POLL_CONTROL_FAST_POLL_TIMEOUT_DEFAULT_VALUE;
  g_device_ctx.poll_control_attr.checkin_interval_min   = IZS_DEVICE_MIN_CHECKIN_INTERVAL;
  g_device_ctx.poll_control_attr.long_poll_interval_min = IZS_DEVICE_MIN_LONG_POLL_INTERVAL;
  g_device_ctx.poll_control_attr.fast_poll_timeout_max  = ZB_ZCL_POLL_CONTROL_FAST_POLL_TIMEOUT_CLIENT_DEFAULT_VALUE;

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

  g_device_ctx.enroll_req_generated = ZB_FALSE;

  izs_update_ias_zone_status(ZB_ZCL_IAS_ZONE_ZONE_STATUS_TROUBLE, g_device_ctx.detector_trouble);

  TRACE_MSG(TRACE_APP1, "<< izs_clusters_attr_init", (FMT__0));
}

void izs_init_default_reporting()
{
  TRACE_MSG(TRACE_APP1, ">> izs_init_default_reporting", (FMT__0));

  ZB_SCHEDULE_APP_CALLBACK(izs_configure_power_config_default_reporting, 0);

  TRACE_MSG(TRACE_APP1, "<< izs_init_default_reporting", (FMT__0));
}

void izs_device_app_init(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> izs_device_app_init", (FMT__0));

  ZVUNUSED(param);

  izs_app_ctx_init();

  /* Init HA attributes */
  izs_clusters_attr_init(0);

  izs_set_default_configuration_values();

  zb_set_nvram_erase_at_start(ZB_FALSE);
  zb_set_rx_on_when_idle(ZB_FALSE);

  /********** IZS device configuration **********/
  zb_zcl_ias_zone_register_cb(IZS_DEVICE_ENDPOINT,
                              izs_ias_zone_notification_cb,
                              NULL);

  /* Register callback for check-in. Use this callback to send attr
   * values reports each check-in interval */
  zb_zcl_poll_controll_register_cb(izs_check_in_cb);

  izs_init_default_reporting();


  /* Schedule device h/w init. Do it via scheduler - in case some init
     action will need blocking. After init, it will start izs_device_startup() */
  ZB_SCHEDULE_APP_CALLBACK(izs_device_hw_init, 0);

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
  TRACE_MSG(TRACE_APP1, "> izs_check_ias_zone_found param %hd", (FMT__H, param));

  /* If no match descr with short CIE addr or no write attr with CIE long addr - leave network */
  if ((g_device_ctx.zone_attr.cie_short_addr == IZS_INIT_CIE_SHORT_ADDR) ||
      (g_device_ctx.zone_attr.cie_ep == IZS_INIT_CIE_ENDPOINT)
#ifdef IZS_IAS_ZONE_CHECK_CIE_IEEE
      || ZB_IEEE_ADDR_IS_ZERO(IZS_DEVICE_CIE_ADDR())
#endif
    )
  {
    TRACE_MSG(TRACE_APP1, "CIE is NOT found, leaving", (FMT__0));
    /* Do not call retry_join here, we need to do full reset before */
    izs_full_reset_to_defaults(0);
  }

  if (param)
  {
    ZB_FREE_BUF(param);
  }

  TRACE_MSG(TRACE_APP1, "< izs_check_ias_zone_found", (FMT__0));
}

void izs_find_ias_zone_cb(zb_uint8_t param)
{
  zb_zdo_match_desc_resp_t *resp = (zb_zdo_match_desc_resp_t*)zb_buf_begin(param);
  zb_uint8_t *match_ep;
  zb_apsde_data_indication_t *ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);

  TRACE_MSG(TRACE_APP1, "> izs_find_ias_zone_cb param %hd", (FMT__H, param));

  TRACE_MSG(TRACE_APP2, "resp match_len %hd", (FMT__H, resp->match_len));
  if (resp->status == ZB_ZDP_STATUS_SUCCESS && resp->match_len > 0)
  {
    /* The device just joined network and found CIE in this network */

    /* will not enforce Enroll - IAS CIE should send us enroll response */
    TRACE_MSG(TRACE_APP2, "IAS ZONE match desc received, continue normal work...", (FMT__0));

    IZS_SET_DEVICE_STATE(IZS_STATE_SENSOR_NORMAL);

    /* Match EP list follows right after response header */
    match_ep = (zb_uint8_t*)(resp + 1);

    /* set EP value directly to attribute value */
    /* we are searching for exact cluster, so only 1 EP maybe found */
    g_device_ctx.zone_attr.cie_ep = *match_ep;
    g_device_ctx.zone_attr.cie_short_addr = ind->src_addr;

#ifdef ZB_USE_NVRAM
    /* Save attributes to nvram */
    /* If we fail, trace is given and assertion is triggered */
    (void)zb_nvram_write_dataset(ZB_NVRAM_HA_DATA);
#endif

    TRACE_MSG(TRACE_APP2, "find_ias addr %d ep %hd",
              (FMT__D_H, g_device_ctx.zone_attr.cie_short_addr, g_device_ctx.zone_attr.cie_ep));

    /* Check if device is already enrolled, go on guard: it works
     * if device was previously enroled and then restarted  */
    if (IZS_DEVICE_IS_ENROLLED())
    {
      izs_go_on_guard(param);
    }
    else
    {
      TRACE_MSG(TRACE_APP1, "device is not enrolled, do nothing", (FMT__0));
      ZB_FREE_BUF(buf);
    }
  }
  else
  {
    ZB_FREE_BUF(buf);
  }

  TRACE_MSG(TRACE_APP1, "< izs_find_ias_zone_cb", (FMT__0));
}

void izs_start_bdb_commissioning(zb_uint8_t param)
{
  zb_ret_t ret = RET_OK;

  ZVUNUSED(param);

  TRACE_MSG(TRACE_APP1, ">> izs_start_bdb_commissioning", (FMT__0));

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
  TRACE_MSG(TRACE_APP1, "> izs_ota_init", (FMT__0));

  ZB_SCHEDULE_APP_ALARM(zb_zcl_ota_upgrade_init_client, param, 15*ZB_TIME_ONE_SECOND);

  TRACE_MSG(TRACE_APP1, "< izs_ota_init", (FMT__0));
}
#endif

void zboss_signal_handler(zb_uint8_t param)
{
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_SKIP_STARTUP:
        TRACE_MSG(TRACE_APP1, "ZB_ZDO_SIGNAL_SKIP_STARTUP: start join", (FMT__0));
        ZB_SCHEDULE_APP_CALLBACK(izs_start_join, IZS_FIRST_JOIN_ATTEMPT);
        break;
//! [signal_skip_startup]
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
            zb_ret_t ret_code;
            /* We are in "retry" scenario - do not call command send */
            /* But force retry timeout to expire instead */

            ret_code = ZB_SCHEDULE_APP_ALARM_CANCEL(izs_resend_notification, ZB_ALARM_ANY_PARAM);

            if (ret_code == RET_OK)
            {
              if (!g_device_ctx.out_buf)
              {
                g_device_ctx.out_buf = zb_buf_get_out();
              }

              ZB_SCHEDULE_APP_CALLBACK(izs_resend_notification, 0);
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
          izs_find_ias_zone_client(param, izs_find_ias_zone_cb);

          ZB_SCHEDULE_APP_ALARM(izs_check_ias_zone_found, 0, IZS_WAIT_MATCH_IAS_ZONE);
          param = 0;
        }
      }
      break;

      case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APP1, "steering signal", (FMT__0));
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
    ZB_FREE_BUF(param);
  }
}

void izs_stop_checkins(zb_uint8_t param)
{
  zb_uint8_t canceled_param = 0;
  ZVUNUSED(param);

  g_device_ctx.check_in_started = 0;

  canceled_param = zb_zcl_poll_control_stop();
  if (canceled_param)
  {
    TRACE_MSG(TRACE_APP2, "free canceled buffer %hd", (FMT__H, canceled_param));
    ZB_FREE_BUF(ZB_BUF_FROM_REF(canceled_param));
  }
}

zb_uint8_t izs_zcl_cmd_handler(zb_uint8_t param)
{
  zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);
  zb_uint8_t cmd_processed = ZB_FALSE;
  zb_uint8_t src_ep;
  zb_uint16_t src_addr;

  TRACE_MSG(TRACE_APP1, "> izs_zcl_cmd_handler %i", (FMT__H, param));
  ZB_ZCL_DEBUG_DUMP_HEADER(cmd_info);
  TRACE_MSG(TRACE_APP1, "payload size: %i", (FMT__D, zb_buf_len(param)));

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
            /* Check that we receive the Write Attributes cmnd for the checkin Interval AttrId
               If so, start checkin cycle */
            write_attr_req = (zb_zcl_write_attr_req_t*)zb_buf_begin(param);
            if (ZB_ZCL_ATTR_POLL_CONTROL_CHECKIN_INTERVAL_ID == write_attr_req->attr_id)
            {
              if (0 != (*(zb_uint32_t *)write_attr_req->attr_value))
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
                    ZB_FREE_BUF(ZB_BUF_FROM_REF(canceled_param));
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
    } else
    if (cmd_info->cluster_id == ZB_ZCL_CLUSTER_ID_IAS_ZONE &&
        cmd_info->is_common_command)
    {
       switch (cmd_info->cmd_id)
       {
          case ZB_ZCL_CMD_WRITE_ATTRIB:
            {
               zb_zcl_write_attr_req_t *write_attr_req;
               /* Check that we receive the Write Attributes cmnd for the CIE address
                  If so, start fast polling */
               write_attr_req = (zb_zcl_write_attr_req_t*)zb_buf_begin(param);
               if (ZB_ZCL_ATTR_IAS_ZONE_IAS_CIE_ADDRESS_ID == write_attr_req->attr_id)
               {
#ifdef FAST_POLLING_DURING_COMMISSIONING
                 izs_start_fast_polling_for_commissioning(IZS_DEVICE_TURBO_POLL_AFTER_CIE_ADDR_DURATION * 1000l);
#else
                 ZDO_CTX().poll_update_delay = IZS_DEVICE_INITIAL_FAST_POLL_DURATION;
#endif /* FAST_POLLING_DURING_COMMISSIONING */
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
             * already set in izs_find_ias_zone_cb() */
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
    ZB_FREE_BUF(param);
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
    ZB_FREE_BUF(param);
  }

  izs_basic_reset_to_defaults(0);

  zb_buf_get_out_delayed(izs_leave_nwk);

  TRACE_MSG(TRACE_APP1, "<< izs_full_reset_to_defaults", (FMT__0));
}

void izs_basic_reset_to_defaults(zb_uint8_t param)
{
  zb_uint8_t canceled_param;
  (void)param;
  TRACE_MSG(TRACE_APP1, ">> izs_basic_reset_to_defaults", (FMT__0));

  /* Reset to factory defaults:
     - Forget notifications from previous life
     - Reset all settings/HA attrs (except network and network commissioning)
     - Keep polling untill ZDO leave or power cycle
  */

  ZB_SCHEDULE_APP_ALARM_CANCEL(izs_resend_notification, ZB_ALARM_ANY_PARAM);

  if (!g_device_ctx.out_buf)
  {
    g_device_ctx.out_buf = zb_buf_get_out();
  }

  g_device_ctx.retry_backoff_cnt = 0;
  izs_ias_zone_queue_init();
  g_device_ctx.enroll_req_generated = ZB_FALSE;

  /* Stop OTA upgrade */
#ifdef IZS_OTA
  zcl_ota_abort(IZS_DEVICE_ENDPOINT, 0);
#endif
  izs_stop_checkins(0);

  izs_clusters_attr_init(0);

  izs_init_default_reporting();

#ifdef ZB_USE_NVRAM
  /* If we fail, trace is given and assertion is triggered */
  (void)zb_nvram_write_dataset(ZB_NVRAM_HA_DATA);
  (void)zb_nvram_write_dataset(ZB_NVRAM_ZCL_REPORTING_DATA);
  (void)zb_nvram_write_dataset(ZB_NVRAM_HA_POLL_CONTROL_DATA);
#endif

  TRACE_MSG(TRACE_APP1, "<< izs_basic_reset_to_defaults", (FMT__0));
}

void izs_retry_join()
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

void izs_app_ctx_init()
{
  TRACE_MSG(TRACE_APP1, ">> izs_app_ctx_init", (FMT__0));

  ZB_MEMSET(&g_device_ctx, 0, sizeof(g_device_ctx));

  IZS_SET_DEVICE_STATE(IZS_STATE_APP_NOT_INIT);

  g_device_ctx.zone_attr.zone_state = ZB_ZCL_IAS_ZONE_ZONESTATE_NOT_ENROLLED;
  ZB_ASSERT(ZB_FALSE==g_device_ctx.detector_trouble);

  izs_ias_zone_queue_init();

  g_device_ctx.out_buf = ZB_GET_OUT_BUF();
  if (!g_device_ctx.out_buf)
  {
    izs_critical_error();
  }

  TRACE_MSG(TRACE_APP1, "<< izs_app_ctx_init", (FMT__0));
}

void izs_send_notification()
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
  TRACE_MSG(TRACE_APP1, "g_device_ctx.out_buf = %p, queue size is %hd", (FMT__P_H, g_device_ctx.out_buf, g_device_ctx.ias_zone_queue.written));
  /* check device is Entolled or not. if NOT enrolled, send enroll request */
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
    zb_ret_t ret_code;

    /* We are in "retry" scenario - do not call command send */
    /* But force retry timeout to expire instead */

    g_device_ctx.retry_backoff_cnt = 0;

    ret_code = ZB_SCHEDULE_APP_ALARM_CANCEL(izs_resend_notification, ZB_ALARM_ANY_PARAM);

    if (ret_code == RET_OK)
    {
      if (!g_device_ctx.out_buf)
      {
        g_device_ctx.out_buf = zb_buf_get_out();
      }

      izs_retry_backoff();
    }

    /* try to force rejoin backoff */
    zb_zdo_rejoin_backoff_force();
  }
  else
  {
    /* Send notification command only if device is enrolled */
    /* check if we have a buffer => command is not beign sent now, continue  */
    TRACE_MSG(TRACE_APP1, "g_device_ctx.out_buf = %p, queue size is %hd", (FMT__P_H, g_device_ctx.out_buf, g_device_ctx.ias_zone_queue.written));
    TRACE_MSG(TRACE_APP1, "CIE EP: %hd", (FMT__H, g_device_ctx.zone_attr.cie_ep));
    if ((g_device_ctx.out_buf)&&(g_device_ctx.zone_attr.cie_ep))
    {
      while (1)
      {
        zone_info = izs_ias_zone_get_element_from_queue();
        if (!zone_info)
        {
          /* Nothing to send - queue is empty */
          TRACE_MSG(TRACE_APP1, "g_device_ctx.out_buf = %p, queue size is %hd", (FMT__P_H, g_device_ctx.out_buf, g_device_ctx.ias_zone_queue.written));
          TRACE_MSG(TRACE_APP1, "queue is empty", (FMT__0));
          break;
        }

        delay = izs_calc_time_delta_qsec(zone_info->timestamp);

        TRACE_MSG(TRACE_APP2, "call ias_zone_set_status, out_buf %p, param %hd",
                  (FMT__P_H, g_device_ctx.out_buf, ZB_REF_FROM_BUF(g_device_ctx.out_buf)));
        cmd_sent = zb_zcl_ias_zone_set_status(
          IZS_DEVICE_ENDPOINT,
          zone_info->ias_status,
          delay,
          ZB_REF_FROM_BUF(g_device_ctx.out_buf));

        if (cmd_sent)
        {
          /* Command is sent. We are sending only 1 command, then waiting for Ack (or cmd fail...)  */
          TRACE_MSG(TRACE_APP1, "cmd is sent, zero out_buf", (FMT__0));
          g_device_ctx.out_buf = NULL;
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

void izs_ias_zone_notification_cb(zb_uint8_t param)
{
  zb_zcl_command_send_status_t *cmd_send_status = ZB_BUF_GET_PARAM(param, zb_zcl_command_send_status_t);

  TRACE_MSG(TRACE_APP1, ">> izs_ias_zone_notification_cb %d", (FMT__H, param));

  if (!g_device_ctx.out_buf)
  {
    g_device_ctx.out_buf = buf;
  }
  else if (g_device_ctx.out_buf != buf)
  {
    zb_buf_free(buf);
  }

  if (cmd_send_status->status == RET_OK)
  {
    /* Notification is sent OK. Put buffer back to context - it will
     * be reused. Remove element from queue here - it was NOT removed
     * on command send */
    TRACE_MSG(TRACE_APP2, "notification sent OK, buf %p", (FMT__P, buf));

    TRACE_MSG(TRACE_APP1, "g_device_ctx.out_buf = %p, queue size is %h", (FMT__P_H, g_device_ctx.out_buf, g_device_ctx.ias_zone_queue.written));
    izs_ias_zone_release_element_from_queue();
    g_device_ctx.retry_backoff_cnt = 0;
    izs_send_notification();
  }
  else
  {
    /* Error sending notification: start retry backoff algorithm */
    izs_retry_backoff();
  }

  TRACE_MSG(TRACE_APP1, "<< izs_ias_zone_notification_cb", (FMT__0));
}

void izs_retry_backoff(void)
{
  zb_uint16_t timeout;

  /* Retry backoff algirithm: If notification send fails (no APS Ack),
   * resent this command in 2-5-10-15-20-... seconds. Timeout should
   * not be more then 30 minutes. */

  TRACE_MSG(TRACE_APP1, ">> izs_retry_backoff, g_device_ctx.out_buf = %hd", (FMT__H, g_device_ctx.out_buf));

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
  ZB_SCHEDULE_APP_ALARM(izs_resend_notification, 0, timeout * ZB_TIME_ONE_SECOND);

  TRACE_MSG(TRACE_APP1, "<< izs_retry_backoff, g_device_ctx.out_buf = %hd", (FMT__H, g_device_ctx.out_buf));
}

void izs_resend_notification(zb_uint8_t param)
{
  zb_zcl_ias_zone_status_change_not_t notification;
  zb_zcl_parse_status_t parse_status;
  zb_uint16_t delay;
  izs_ias_zone_info_t *zone_info;

  TRACE_MSG(TRACE_APP1, ">> izs_resend_notification, g_device_ctx.out_buf = %hd",
            (FMT__H, g_device_ctx.out_buf));

  ZVUNUSED(param);

  if (!g_device_ctx.out_buf)
  {
    TRACE_MSG(TRACE_ERROR, "No buf to resend notification", (FMT__0));
    izs_critical_error();
  }

  /* NOTE: pure ZCL packet is in the buffer */
  ZB_ZCL_CUT_HEADER(g_device_ctx.out_buf);
  {
    ZB_ZCL_IAS_ZONE_GET_STATUS_CHANGE_NOTIFICATION_REQ(&notification, g_device_ctx.out_buf, parse_status);
    if (parse_status == ZB_ZCL_PARSE_STATUS_SUCCESS)
    {
      zone_info = izs_ias_zone_get_element_from_queue();
      if (zone_info)
      {
        delay = izs_calc_time_delta_qsec(zone_info->timestamp);
        ZB_ZCL_IAS_ZONE_SEND_STATUS_CHANGE_NOTIFICATION_REQ(
          g_device_ctx.out_buf, g_device_ctx.zone_attr.cie_short_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
          g_device_ctx.zone_attr.cie_ep, IZS_DEVICE_ENDPOINT, ZB_AF_HA_PROFILE_ID, izs_ias_zone_notification_cb,
          notification.zone_status, notification.extended_status, notification.zone_id,
          delay);
      }
    }
  }

  zb_buf_free(g_device_ctx.out_buf);
  g_device_ctx.out_buf = 0;

  TRACE_MSG(TRACE_APP1, "<< izs_resend_notification", (FMT__0));
}

/* Function is called when device is ready to serve. There are 3 cases:
   - new device enrolled to network
   - device is alraedy enrolled, it is restarted => joined network
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
      ZB_FREE_BUF(param);
    }
  }

  /* kick notification queue handling... */
  izs_send_notification();

  TRACE_MSG(TRACE_APP1, "<< izs_go_on_guard", (FMT__0));
}

void izs_critical_error()
{
  TRACE_MSG(TRACE_APP1, "izs_critical_error", (FMT__0));

  ZB_ASSERT(0);
}
