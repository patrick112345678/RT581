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
/* PURPOSE: Test sample with clusters IAS Zone, IAS ACE and IAS WD */

#define ZB_TRACE_FILE_ID 51364
#include "zboss_api.h"
#include "security_safety_zr.h"

/* Insert that include before any code or declaration. */
#ifdef ZB_CONFIGURABLE_MEM
#include "zb_mem_config_max.h"
#endif

zb_uint16_t g_dst_addr = 0x00;

#define DST_ADDR g_dst_addr
#define DST_ADDR_MODE ZB_APS_ADDR_MODE_16_ENDP_PRESENT
#define DST_EP 5

/**
 * Global variables definitions
 */
zb_ieee_addr_t g_zc_addr = {0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb}; /* IEEE address of the
                                                                              * device */
/* Used endpoint */
#define ZB_SERVER_ENDPOINT          5
#define ZB_CLIENT_ENDPOINT          6

void test_loop(zb_bufid_t param);
void test_device_cb(zb_uint8_t param);

/**
 * Declaring attributes for each cluster
 */

/* IAS Zone cluster attributes */
zb_uint8_t g_attr_zone_state = ZB_ZCL_IAS_ZONE_ZONESTATE_DEF_VALUE;
zb_uint16_t g_attr_zone_type = 0;
zb_uint16_t g_attr_zone_status = ZB_ZCL_IAS_ZONE_ZONE_STATUS_DEF_VALUE;
zb_uint8_t g_attr_number_of_zone_sens_levels_supported = ZB_ZCL_IAS_ZONE_NUMBER_OF_ZONE_SENSITIVITY_LEVELS_SUPPORTED_DEFAULT_VALUE;
zb_uint8_t g_attr_current_zone_sens_level = ZB_ZCL_IAS_ZONE_CURRENT_ZONE_SENSITIVITY_LEVEL_DEFAULT_VALUE;
zb_ieee_addr_t g_attr_ias_cie_address = {0, 0, 0, 0, 0, 0, 0, 0};
zb_uint8_t g_attr_zone_id = ZB_ZCL_IAS_ZONEID_ID_DEF_VALUE;
zb_uint16_t g_attr_cie_short_addr = 0;
zb_uint8_t g_attr_cie_ep = 0;

ZB_ZCL_DECLARE_IAS_ZONE_ATTRIB_LIST_EXT(ias_zone_attr_list, &g_attr_zone_state, &g_attr_zone_type, &g_attr_zone_status,
                                      &g_attr_number_of_zone_sens_levels_supported, &g_attr_current_zone_sens_level,
                                      &g_attr_ias_cie_address, &g_attr_zone_id, &g_attr_cie_short_addr, &g_attr_cie_ep);

/* IAS ACE cluster attributes */
zb_uint16_t g_attr_length = 0;
zb_uint8_t g_attr_table = 0;
ZB_ZCL_DECLARE_IAS_ACE_ATTRIB_LIST(ias_ace_attr_list, &g_attr_length, &g_attr_table);

/* IAS WD cluster attributes */
zb_uint16_t g_attr_max_duration = ZB_ZCL_ATTR_IAS_WD_MAX_DURATION_DEF_VALUE;
ZB_ZCL_DECLARE_IAS_WD_ATTRIB_LIST(ias_wd_attr_list, &g_attr_max_duration);

/* Declare cluster list for the device */
ZB_DECLARE_SECURITY_SAFETY_SERVER_CLUSTER_LIST(security_safety_server_clusters,ias_zone_attr_list, ias_ace_attr_list,
                                    ias_wd_attr_list);

/* Declare server endpoint */
ZB_DECLARE_SECURITY_SAFETY_SERVER_EP(security_safety_server_ep, ZB_SERVER_ENDPOINT, security_safety_server_clusters);
ZB_DECLARE_SECURITY_SAFETY_CLIENT_CLUSTER_LIST(security_safety_client_clusters);

/* Declare client endpoint */
ZB_DECLARE_SECURITY_SAFETY_CLIENT_EP(security_safety_client_ep, ZB_CLIENT_ENDPOINT, security_safety_client_clusters);

/* Declare application's device context for single-endpoint device */
ZB_DECLARE_SECURITY_SAFETY_CTX(security_safety_output_ctx, security_safety_server_ep, security_safety_client_ep);

MAIN()
{
  ARGV_UNUSED;

  /* Trace enable */
  ZB_SET_TRACE_ON();
  /* Traffic dump enable*/
  ZB_SET_TRAF_DUMP_ON();

  /* Global ZBOSS initialization */
  ZB_INIT("security_safety_zr");

  /* Set up defaults for the commissioning */
  zb_set_long_address(g_zc_addr);
  zb_set_network_router_role(1l<<22);

  zb_set_nvram_erase_at_start(ZB_FALSE);
  zb_set_max_children(1);

  /* [af_register_device_context] */
  /* Register device ZCL context */
  ZB_AF_REGISTER_DEVICE_CTX(&security_safety_output_ctx);
  ZB_ZCL_REGISTER_DEVICE_CB(test_device_cb);

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
  zb_uint8_t *cmd_ptr = NULL;
  zb_char_t arm_disarm_code[5] = { 4, 'a', 'b', 'c', 'd' };

  TRACE_MSG(TRACE_APP1, ">> test_loop test_step=%hd", (FMT__H, test_step));

  if (param == 0)
  {
    zb_buf_get_out_delayed(test_loop);
  }
  else
  {
    switch(test_step)
    {
      case 0:
        ZB_ZCL_IAS_ZONE_SEND_ZONE_ENROLL_RES(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                          ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, ZB_ZCL_IAS_ZONE_ENROLL_RESPONCE_CODE_SUCCESS,
                                          0);
        break;

      case 1:
        ZB_ZCL_IAS_ZONE_SEND_INITIATE_TEST_MODE_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                          ZB_AF_HA_PROFILE_ID, NULL, 0, 0);
        break;

      case 2:
        ZB_ZCL_IAS_ZONE_SEND_STATUS_CHANGE_NOTIFICATION_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                          ZB_AF_HA_PROFILE_ID, NULL, ZB_ZCL_IAS_ZONE_ZONE_STATUS_TEST, 0, 0, 0 );
        break;

      case 3:
        ZB_ZCL_IAS_ZONE_SEND_ZONE_ENROLL_REQUEST_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                          ZB_AF_HA_PROFILE_ID, NULL, ZB_ZCL_IAS_ZONE_ZONETYPE_STANDARD_CIE, 0);
        break;

      case 4:
        ZB_ZCL_IAS_ZONE_SEND_INITIATE_NORMAL_OPERATION_MODE_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                          ZB_AF_HA_PROFILE_ID, NULL);

      case 5:
        ZB_ZCL_IAS_ACE_SEND_ARM_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                  ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, ZB_ZCL_IAS_ACE_ARM_MODE_DISARM, arm_disarm_code, 0);
        break;

      case 6:
        ZB_ZCL_IAS_ACE_SEND_EMERGENCY_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                  ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
        break;

      case 7:
        ZB_ZCL_IAS_ACE_SEND_FIRE_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                  ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
        break;

      case 8:
        ZB_ZCL_IAS_ACE_SEND_PANIC_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                  ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
        break;

      case 9:
        ZB_ZCL_IAS_ACE_SEND_BYPASS_REQ_START(param, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, 1, cmd_ptr);
        ZB_ZCL_IAS_ACE_SEND_BYPASS_REQ_ADD(cmd_ptr, 0);
        ZB_ZCL_IAS_ACE_SEND_BYPASS_REQ_END(cmd_ptr, param, arm_disarm_code, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                        ZB_AF_HA_PROFILE_ID, NULL);
        break;

      case 10:
        ZB_ZCL_IAS_ACE_SEND_GET_ZONE_INFO_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                  ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, 0);
        break;

      case 11:
        ZB_ZCL_IAS_ACE_SEND_GET_PANEL_STATUS_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                  ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
        break;

      case 12:
        ZB_ZCL_IAS_ACE_SEND_GET_ZONE_STATUS_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                  ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, 0, 0, 0, 0);
        break;

      case 13:
        ZB_ZCL_IAS_ACE_SEND_GET_ZONE_ID_MAP_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                  ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
        break;

      case 14:
        ZB_ZCL_IAS_ACE_SEND_GET_BYPASSED_ZONE_LIST_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                  ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
        break;

      case 15:
        ZB_ZCL_IAS_WD_SEND_SQUAWK_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                  ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, ZB_ZCL_IAS_WD_MAKE_SQUAWK_STATUS(ZB_ZCL_IAS_WD_SQUAWK_MODE_DISARMED,
                                  ZB_ZCL_IAS_WD_SQUAWK_STROBE_USE_STROBE, ZB_ZCL_IAS_WD_SQUAWK_LEVEL_HIGH));
        break;

      case 16:
        ZB_ZCL_IAS_WD_SEND_START_WARNING_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                  ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, ZB_ZCL_IAS_WD_MAKE_START_WARNING_STATUS(
                                  ZB_ZCL_IAS_WD_WARNING_MODE_BURGLAR, ZB_ZCL_IAS_WD_STROBE_USE_STROBE, ZB_ZCL_IAS_WD_SIREN_LEVEL_HIGH),
                                  0, 0, 0);
        break;
    }

    test_step++;

    if(test_step <= 16)
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
    switch(sig)
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

void test_device_cb(zb_uint8_t param)
{
  zb_bufid_t buffer = param;
  zb_zcl_device_callback_param_t *device_cb_param = ZB_BUF_GET_PARAM(buffer, zb_zcl_device_callback_param_t);

  TRACE_MSG(TRACE_APP1, "> test_device_cb param %hd id %hd", (FMT__H_H, param, device_cb_param->device_cb_id));

  switch (device_cb_param->device_cb_id)
  {
    case ZB_ZCL_IAS_WD_SQUAWK_VALUE_CB_ID:
      device_cb_param->status = RET_OK;
      break;

    case ZB_ZCL_IAS_WD_START_WARNING_VALUE_CB_ID:
      device_cb_param->status = RET_OK;
      break;

    case ZB_ZCL_IAS_ZONE_ENROLL_RESPONSE_VALUE_CB_ID:
      device_cb_param->status = RET_OK;
      break;

    default:
      TRACE_MSG(TRACE_APP1, "nothing to do, skip", (FMT__0));
      break;
  }
  TRACE_MSG(TRACE_APP1, "< test_device_cb %hd", (FMT__H, device_cb_param->status));
}
