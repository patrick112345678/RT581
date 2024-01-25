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
/* PURPOSE: DUT ZR
*/

#define ZB_TEST_NAME RTP_OTA_CLI_07_DUT_ZR
#define ZB_TRACE_FILE_ID 40300

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"

#include "../common/zb_reg_test_globals.h"
#include "rtp_ota_cli_07_dut_zr.h"
#include "rtp_ota_cli_07_common.h"

#if ! defined ZB_ROUTER_ROLE
#error define ZB_ROUTER_ROLE to compile zr tests
#endif

#ifndef ZB_STACK_REGRESSION_TESTING_API
#error Define ZB_STACK_REGRESSION_TESTING_API
#endif

static zb_ieee_addr_t g_zr_addr = {0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};

#define ENDPOINT  10

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t g_attr_zcl_version  = ZB_ZCL_VERSION;
static zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(rtp_ota_cli_07_dut_zr_basic_attr_list, &g_attr_zcl_version, &g_attr_power_source);

/* Time server cluster attributes data */

static zb_uint32_t g_time = ZB_ZCL_TIME_TIME_DEFAULT_VALUE;
static zb_uint8_t g_time_status = ZB_ZCL_TIME_TIME_STATUS_DEFAULT_VALUE;
static zb_uint32_t g_time_zone = ZB_ZCL_TIME_TIME_ZONE_DEFAULT_VALUE;
static zb_uint32_t g_dst_start = 0;
static zb_uint32_t g_dst_end = 0;
static zb_uint32_t g_dst_shift = ZB_ZCL_TIME_DST_SHIFT_DEFAULT_VALUE;
static zb_uint32_t g_standard_time = 0;
static zb_uint32_t g_local_time = 0;
static zb_uint32_t g_last_set_time = ZB_ZCL_TIME_LAST_SET_TIME_DEFAULT_VALUE;
static zb_uint32_t g_valid_until_time = ZB_ZCL_TIME_VALID_UNTIL_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_TIME_ATTRIB_LIST(rtp_ota_cli_07_dut_zr_time_attr_list, &g_time, &g_time_status, &g_time_zone,
                                &g_dst_start, &g_dst_end, &g_dst_shift, &g_standard_time,
                                &g_local_time, &g_last_set_time, &g_valid_until_time);

/* OTA Upgrade client cluster attributes data */
static zb_ieee_addr_t upgrade_server = ZB_ZCL_OTA_UPGRADE_SERVER_DEF_VALUE;
static zb_uint32_t file_offset = ZB_ZCL_OTA_UPGRADE_FILE_OFFSET_DEF_VALUE;

static zb_uint32_t file_version = OTA_UPGRADE_TEST_FILE_VERSION;   // custom data

static zb_uint16_t stack_version = ZB_ZCL_OTA_UPGRADE_FILE_HEADER_STACK_PRO;
static zb_uint32_t downloaded_file_ver = ZB_ZCL_OTA_UPGRADE_DOWNLOADED_FILE_VERSION_DEF_VALUE;
static zb_uint16_t downloaded_stack_ver = ZB_ZCL_OTA_UPGRADE_DOWNLOADED_STACK_DEF_VALUE;
static zb_uint8_t image_status = ZB_ZCL_OTA_UPGRADE_IMAGE_STATUS_DEF_VALUE;

static zb_uint16_t manufacturer = OTA_UPGRADE_TEST_MANUFACTURER;   // custom data
static zb_uint16_t image_type = OTA_UPGRADE_TEST_IMAGE_TYPE;       // custom data

static zb_uint16_t min_block_reque = 0;
static zb_uint16_t image_stamp = ZB_ZCL_OTA_UPGRADE_IMAGE_STAMP_MIN_VALUE;
static zb_uint16_t server_addr;
static zb_uint8_t server_ep;

ZB_ZCL_DECLARE_OTA_UPGRADE_ATTRIB_LIST(rtp_ota_cli_07_dut_zr_ota_upgrade_attr_list,
    &upgrade_server, &file_offset, &file_version, &stack_version, &downloaded_file_ver,
    &downloaded_stack_ver, &image_status, &manufacturer, &image_type, &min_block_reque, &image_stamp,
    &server_addr, &server_ep, 0x0101, OTA_UPGRADE_TEST_DATA_SIZE, ZB_ZCL_OTA_UPGRADE_QUERY_TIMER_COUNT_DEF);

static zb_bool_t g_device_upgraded = ZB_FALSE;
static zb_uint32_t g_upgrade_timestamp = 0;

/********************* Declare device **************************/

ZB_HA_DECLARE_OTA_UPGRADE_CLIENT_CLUSTER_LIST(rtp_ota_cli_07_dut_zr_ota_upgrade_client_clusters,
          rtp_ota_cli_07_dut_zr_basic_attr_list, 
          rtp_ota_cli_07_dut_zr_time_attr_list,
          rtp_ota_cli_07_dut_zr_ota_upgrade_attr_list);

ZB_HA_DECLARE_OTA_UPGRADE_CLIENT_EP(rtp_ota_cli_07_dut_zr_ota_upgrade_client_ep, ENDPOINT, rtp_ota_cli_07_dut_zr_ota_upgrade_client_clusters);

ZB_HA_DECLARE_OTA_UPGRADE_CLIENT_CTX(rtp_ota_cli_07_dut_zr_ota_upgrade_client_ctx, rtp_ota_cli_07_dut_zr_ota_upgrade_client_ep);

/******************************************************************/

static zb_uint8_t test_data_indication_cb(zb_uint8_t param);
static void test_device_cb(zb_uint8_t param);

static void test_device_upgrade_status(zb_uint8_t unused);

MAIN()
{
  ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
  ZB_SET_TRACE_LEVEL(4);
  ARGV_UNUSED;

  ZB_INIT("zdo_dut_zr");

  zb_set_long_address(g_zr_addr);
  zb_reg_test_set_common_channel_settings();
  zb_set_network_router_role((1l << TEST_CHANNEL));
  zb_set_nvram_erase_at_start(ZB_TRUE);

  /* need to generate jitter value equal to value from Image Notify Command */
  ZB_REGRESSION_TESTS_API().zcl_ota_custom_query_jitter = 1;

  /****************** Register Device ********************************/
  ZB_AF_REGISTER_DEVICE_CTX(&rtp_ota_cli_07_dut_zr_ota_upgrade_client_ctx);
  ZB_ZCL_REGISTER_DEVICE_CB(test_device_cb);

  zb_zcl_time_init_server();
  zb_af_set_data_indication(test_data_indication_cb);

  if (zboss_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
  }
  else
  {
    zboss_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  TRACE_MSG(TRACE_ZCL1, "> zboss_signal_handler %h", (FMT__H, param));

  switch(sig)
  {
    case ZB_ZDO_SIGNAL_DEFAULT_START:
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));

      if (status == 0)
      {
        ZB_SCHEDULE_APP_ALARM(zb_zcl_ota_upgrade_init_client, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(2*1000));
        param = 0;
      }
      break;

    case ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY:
      TRACE_MSG(TRACE_APP1, "Production configuration block is ready", (FMT__0));
      break;

    default:
      TRACE_MSG(TRACE_ERROR, "Unknown signal %hd", (FMT__H, sig));
  }

  if (param)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZCL1, "< zboss_signal_handler", (FMT__0));
}

static void test_device_upgrade_status(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  if (g_device_upgraded)
  {
    TRACE_MSG(TRACE_APP1, "Device is upgraded", (FMT__0));
  }
  else 
  {
    TRACE_MSG(TRACE_APP1, "Device is still not upgraded", (FMT__0));
  }
}


static zb_uint8_t test_data_indication_cb(zb_uint8_t param)
{
  zb_uint8_t param_tmp = zb_buf_get_out();
  zb_apsde_data_indication_t *ind;
  zb_zcl_parsed_hdr_t cmd_info;
  zb_zcl_status_t status;
  zb_zcl_ota_upgrade_upgrade_end_res_t payload;
  zb_ret_t ret;

  TRACE_MSG(TRACE_APP1, "test_data_indication_cb(): param %d", (FMT__D, param));

  if (param_tmp)
  {
    zb_buf_copy(param_tmp, param);
  }
  else
  {
    return ZB_FALSE;
  }

  ind = ZB_BUF_GET_PARAM(param_tmp, zb_apsde_data_indication_t);

  if (ind->profileid == ZB_AF_HA_PROFILE_ID) {
    ZB_BZERO(&cmd_info, sizeof(cmd_info));
    zb_zcl_parse_header(param_tmp, &cmd_info);

    if (cmd_info.cluster_id == ZB_ZCL_CLUSTER_ID_OTA_UPGRADE && 
        cmd_info.cmd_id == ZB_ZCL_CMD_OTA_UPGRADE_UPGRADE_END_RESP_ID)
    {
      ZB_ZCL_CUT_HEADER(param_tmp);
      ZB_ZCL_OTA_UPGRADE_GET_UPGRADE_END_RES(&payload, param_tmp, status);
      ZB_ASSERT(status == ZB_ZCL_PARSE_STATUS_SUCCESS);

      g_upgrade_timestamp = payload.upgrade_time;
    }
  }

  zb_buf_free(param_tmp);

  return ZB_FALSE;
}

static void test_device_cb(zb_uint8_t param)
{
  zb_zcl_device_callback_param_t *device_cb_param = ZB_BUF_GET_PARAM(param, zb_zcl_device_callback_param_t);
  TRACE_MSG(TRACE_APP1, "> test_device_cb param %hd id %hd", (FMT__H_H, param, device_cb_param->device_cb_id));

  device_cb_param->status = RET_OK;
  switch (device_cb_param->device_cb_id)
  {
    case ZB_ZCL_OTA_UPGRADE_VALUE_CB_ID:
    {
      zb_zcl_ota_upgrade_value_param_t *ota_upgrade_value = &(device_cb_param->cb_param.ota_value_param);

      switch (ota_upgrade_value->upgrade_status)
      {
        case ZB_ZCL_OTA_UPGRADE_STATUS_START:
          /* Start OTA upgrade. */
          if (image_status == ZB_ZCL_OTA_UPGRADE_IMAGE_STATUS_NORMAL)
          {
            /* Accept image */
            ota_upgrade_value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_OK;
          }
          else
          {
            /* Another download is in progress, deny new image */
            ota_upgrade_value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_BUSY;
          }
          break;
        case ZB_ZCL_OTA_UPGRADE_STATUS_RECEIVE:
          /* Process image block. */
          ota_upgrade_value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_OK;
          break;
        case ZB_ZCL_OTA_UPGRADE_STATUS_CHECK:
          /* Downloading is finished, do additional checks if needed etc before Upgrade End Request. */
          ota_upgrade_value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_OK;
          break;
        case ZB_ZCL_OTA_UPGRADE_STATUS_APPLY:
          /* Upgrade End Resp is ok, ZCL checks for manufacturer, image type etc are ok.
             Last step before actual upgrade. */
          zb_zcl_time_update_current_time(ENDPOINT);

          ZB_ASSERT(g_upgrade_timestamp >= g_time);

          zb_uint32_t upgrade_delta = g_upgrade_timestamp - g_time;

          ZB_SCHEDULE_ALARM(test_device_upgrade_status, 0, (upgrade_delta - 2) * ZB_TIME_ONE_SECOND);
          ZB_SCHEDULE_ALARM(test_device_upgrade_status, 0, (upgrade_delta + 2) * ZB_TIME_ONE_SECOND);

          ota_upgrade_value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_OK;
          break;
        case ZB_ZCL_OTA_UPGRADE_STATUS_FINISH:
          /* It is time to upgrade FW. */
          TRACE_MSG(TRACE_APP1, "device cb with status ZB_ZCL_OTA_UPGRADE_STATUS_FINISH is called", (FMT__0));
          g_device_upgraded = ZB_TRUE;
          break;
      }
    }
    break;

    default:
      device_cb_param->status = RET_ERROR;
      break;
  }

  TRACE_MSG(TRACE_APP1, "< test_device_cb %hd", (FMT__H, device_cb_param->status));
}
