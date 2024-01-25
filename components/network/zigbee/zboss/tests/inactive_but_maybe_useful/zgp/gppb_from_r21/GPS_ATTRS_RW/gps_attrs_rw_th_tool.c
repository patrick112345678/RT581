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
/* PURPOSE: TH tool
*/

#define ZB_TEST_NAME GPS_ATTRS_RW_TH_TOOL
#define ZB_TRACE_FILE_ID 41310

#include "test_config.h"

#include "../include/zgp_test_templates.h"

#ifdef ZB_CERTIFICATION_HACKS

static zb_ieee_addr_t g_th_tool_addr = TH_TOOL_IEEE_ADDR;
zb_ieee_addr_t g_zgpd_addr = TEST_ZGPD2_IEEE_ADDR;
zb_ieee_addr_t g_zgpd_addr_w = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x07};

/*! Program states according to test scenario */
enum test_states_e
{
  TEST_STATE_INITIAL,
  TEST_STATE_READ_GPS_COMMUNICATION_MODE1,
  TEST_STATE_WRITE_GPS_COMMUNICATION_MODE,
  TEST_STATE_READ_GPS_COMMUNICATION_MODE2,
  TEST_STATE_READ_GPS_COMMISSIONING_EXIT_MODE1,
  TEST_STATE_WRITE_GPS_COMMISSIONING_EXIT_MODE,
  TEST_STATE_READ_GPS_COMMISSIONING_EXIT_MODE2,
  TEST_STATE_READ_GPS_COMMISSIONING_WINDOW1,
  TEST_STATE_WRITE_GPS_COMMISSIONING_WINDOW,
  TEST_STATE_READ_GPS_COMMISSIONING_WINDOW2,
  TEST_STATE_READ_GPS_SECURITY_LEVEL1,  //10
  TEST_STATE_WRITE_GPS_SECURITY_LEVEL,
  TEST_STATE_READ_GPS_SECURITY_LEVEL2,
  TEST_STATE_READ_GPS_MAX_SINK_TABLE_ENTRIES1,
  TEST_STATE_WRITE_GPS_MAX_SINK_TABLE_ENTRIES,
  TEST_STATE_READ_GPS_MAX_SINK_TABLE_ENTRIES2,
  TEST_STATE_READ_GPS_SINK_TABLE1,
  TEST_STATE_WRITE_GPS_SINK_TABLE,
  TEST_STATE_READ_GPS_SINK_TABLE2,
  TEST_STATE_READ_GPS_FUNCTIONALITY1,
  TEST_STATE_WRITE_GPS_FUNCTIONALITY,  //20
  TEST_STATE_READ_GPS_FUNCTIONALITY2,
  TEST_STATE_READ_GPS_ACTIVE_FUNCTIONALITY1,
  TEST_STATE_WRITE_GPS_ACTIVE_FUNCTIONALITY,
  TEST_STATE_READ_GPS_ACTIVE_FUNCTIONALITY2,
  TEST_STATE_SINK_TABLE_REQUEST1,
  TEST_STATE_READ_GPS_SINK_TABLE3,
  TEST_STATE_SINK_TABLE_REQUEST2,
  TEST_STATE_READ_GPS_SINK_TABLE4,
  TEST_STATE_SINK_TABLE_REQUEST3,
  TEST_STATE_SINK_TABLE_REQUEST4,  //30
  TEST_STATE_SINK_TABLE_REQUEST5,
  TEST_STATE_SINK_TABLE_REQUEST6,
  TEST_STATE_SINK_TABLE_REQUEST7,
  TEST_STATE_SINK_TABLE_REQUEST8,
  TEST_STATE_FINISHED
};

ZB_ZGPC_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 1000)

static void send_zcl(zb_uint8_t buf_ref, zb_callback_t cb)
{
  zb_uint32_t val;

  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_READ_GPS_COMMUNICATION_MODE1:
    case TEST_STATE_READ_GPS_COMMUNICATION_MODE2:
      zgp_cluster_read_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                            ZB_ZCL_ATTR_GPS_COMMUNICATION_MODE_ID,
                            ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
      ZB_ZGP_SET_PASSED_STATE_SEQUENCE();
      break;
    case TEST_STATE_WRITE_GPS_COMMUNICATION_MODE:
      val = 0x03;
      zgp_cluster_write_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                             ZB_ZCL_ATTR_GPS_COMMUNICATION_MODE_ID,
                             ZB_ZCL_ATTR_TYPE_8BITMAP, (zb_uint8_t *)&val,
                             ZB_ZCL_DISABLE_DEFAULT_RESPONSE, cb);
      break;
    case TEST_STATE_READ_GPS_COMMISSIONING_EXIT_MODE1:
    case TEST_STATE_READ_GPS_COMMISSIONING_EXIT_MODE2:
      zgp_cluster_read_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                            ZB_ZCL_ATTR_GPS_COMMISSIONING_EXIT_MODE_ID,
                            ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
      break;
    case TEST_STATE_WRITE_GPS_COMMISSIONING_EXIT_MODE:
      val = 0x00;
      zgp_cluster_write_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                             ZB_ZCL_ATTR_GPS_COMMISSIONING_EXIT_MODE_ID,
                             ZB_ZCL_ATTR_TYPE_8BITMAP, (zb_uint8_t *)&val,
                             ZB_ZCL_DISABLE_DEFAULT_RESPONSE, cb);
      break;
    case TEST_STATE_READ_GPS_COMMISSIONING_WINDOW1:
    case TEST_STATE_READ_GPS_COMMISSIONING_WINDOW2:
      zgp_cluster_read_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                            ZB_ZCL_ATTR_GPS_COMMISSIONING_WINDOW_ID,
                            ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
      break;
    case TEST_STATE_WRITE_GPS_COMMISSIONING_WINDOW:
      val = 0x003d;
      zgp_cluster_write_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                             ZB_ZCL_ATTR_GPS_COMMISSIONING_WINDOW_ID,
                             ZB_ZCL_ATTR_TYPE_U16, (zb_uint8_t *)&val,
                             ZB_ZCL_DISABLE_DEFAULT_RESPONSE, cb);
      break;
    case TEST_STATE_READ_GPS_SECURITY_LEVEL1:
    case TEST_STATE_READ_GPS_SECURITY_LEVEL2:
      zgp_cluster_read_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                            ZB_ZCL_ATTR_GPS_SECURITY_LEVEL_ID,
                            ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
      break;
    case TEST_STATE_WRITE_GPS_SECURITY_LEVEL:
      val = 0x02;
      zgp_cluster_write_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                             ZB_ZCL_ATTR_GPS_SECURITY_LEVEL_ID,
                             ZB_ZCL_ATTR_TYPE_8BITMAP, (zb_uint8_t *)&val,
                             ZB_ZCL_DISABLE_DEFAULT_RESPONSE, cb);
      break;
    case TEST_STATE_READ_GPS_MAX_SINK_TABLE_ENTRIES1:
    case TEST_STATE_READ_GPS_MAX_SINK_TABLE_ENTRIES2:
      zgp_cluster_read_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                            ZB_ZCL_ATTR_GPS_MAX_SINK_TABLE_ENTRIES_ID,
                            ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
      break;
    case TEST_STATE_WRITE_GPS_MAX_SINK_TABLE_ENTRIES:
      val = 0x02;
      zgp_cluster_write_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                             ZB_ZCL_ATTR_GPS_MAX_SINK_TABLE_ENTRIES_ID,
                             ZB_ZCL_ATTR_TYPE_U8, (zb_uint8_t *)&val,
                             ZB_ZCL_DISABLE_DEFAULT_RESPONSE, cb);
      break;
    case TEST_STATE_READ_GPS_SINK_TABLE1:
    case TEST_STATE_READ_GPS_SINK_TABLE2:
    case TEST_STATE_READ_GPS_SINK_TABLE3:
      zgp_cluster_read_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                            ZB_ZCL_ATTR_GPS_SINK_TABLE_ID,
                            ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
      break;
    case TEST_STATE_READ_GPS_SINK_TABLE4:
      zgp_cluster_read_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                            ZB_ZCL_ATTR_GPS_SINK_TABLE_ID,
                            ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
      TEST_DEVICE_CTX.skip_next_state = NEXT_STATE_SKIP_AFTER_CMD;
      break;
    case TEST_STATE_WRITE_GPS_SINK_TABLE:
    {
      zb_uint8_t st[5];

      st[0] = 3;
      st[1] = 0;
      st[2] = 0x12;
      st[3] = 0x34;
      st[4] = 0x56;
      zgp_cluster_write_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                             ZB_ZCL_ATTR_GPS_SINK_TABLE_ID,
                             ZB_ZCL_ATTR_TYPE_LONG_OCTET_STRING, st,
                             ZB_ZCL_DISABLE_DEFAULT_RESPONSE, cb);
      break;
    }
    case TEST_STATE_READ_GPS_FUNCTIONALITY1:
    case TEST_STATE_READ_GPS_FUNCTIONALITY2:
      zgp_cluster_read_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                            ZB_ZCL_ATTR_GPS_FUNCTIONALITY_ID,
                            ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
      break;
    case TEST_STATE_WRITE_GPS_FUNCTIONALITY:
      val = 0xffff;
      zgp_cluster_write_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                             ZB_ZCL_ATTR_GPS_FUNCTIONALITY_ID,
                             ZB_ZCL_ATTR_TYPE_24BITMAP, (zb_uint8_t *)&val,
                             ZB_ZCL_DISABLE_DEFAULT_RESPONSE, cb);
      break;
    case TEST_STATE_READ_GPS_ACTIVE_FUNCTIONALITY1:
    case TEST_STATE_READ_GPS_ACTIVE_FUNCTIONALITY2:
      zgp_cluster_read_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                            ZB_ZCL_ATTR_GPS_ACTIVE_FUNCTIONALITY_ID,
                            ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
      break;
    case TEST_STATE_WRITE_GPS_ACTIVE_FUNCTIONALITY:
      val = 0x0000;
      zgp_cluster_write_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                             ZB_ZCL_ATTR_GPS_ACTIVE_FUNCTIONALITY_ID,
                             ZB_ZCL_ATTR_TYPE_24BITMAP, (zb_uint8_t *)&val,
                             ZB_ZCL_DISABLE_DEFAULT_RESPONSE, cb);
      break;
    case TEST_STATE_SINK_TABLE_REQUEST1:
    case TEST_STATE_SINK_TABLE_REQUEST2:
    {
      zb_uint8_t options;

      options = ZB_ZGP_GP_PROXY_TBL_REQ_FILL_OPT(ZB_ZGP_APP_ID_0000,
                                                 ZGP_REQUEST_TABLE_ENTRIES_BY_INDEX);
      zgp_cluster_send_gp_sink_table_request(buf_ref,
                                             DUT_GPS_ADDR,
                                             DUT_GPS_ADDR_MODE,
                                             options,
                                             NULL,
                                             0,
                                             cb);
      TEST_DEVICE_CTX.skip_next_state = NEXT_STATE_SKIP_AFTER_CMD;
      ZB_ZGP_RESET_PASSED_STATE_SEQUENCE();
      break;
    }
    case TEST_STATE_SINK_TABLE_REQUEST3:
    {
      zb_uint8_t options;

      options = ZB_ZGP_GP_PROXY_TBL_REQ_FILL_OPT(ZB_ZGP_APP_ID_0000,
                                                 ZGP_REQUEST_TABLE_ENTRIES_BY_INDEX);
      zgp_cluster_send_gp_sink_table_request(buf_ref,
                                             DUT_GPS_ADDR,
                                             DUT_GPS_ADDR_MODE,
                                             options,
                                             NULL,
                                             1,
                                             cb);
      ZB_ZGP_SET_PASSED_STATE_SEQUENCE();
      break;
    }
    case TEST_STATE_SINK_TABLE_REQUEST4:
    {
      zb_uint8_t   options;
      zb_zgpd_id_t zgpd_id;

      options = ZB_ZGP_GP_PROXY_TBL_REQ_FILL_OPT(ZB_ZGP_APP_ID_0000,
                                                 ZGP_REQUEST_TABLE_ENTRIES_BY_GPD_ID);
      zgpd_id.app_id = ZB_ZGP_APP_ID_0000;
      zgpd_id.endpoint = 0;
      zgpd_id.addr.src_id = TEST_ZGPD_SRC_ID;
      zgp_cluster_send_gp_sink_table_request(buf_ref,
                                             DUT_GPS_ADDR,
                                             DUT_GPS_ADDR_MODE,
                                             options,
                                             &zgpd_id,
                                             0,
                                             cb);
      break;
    }
    case TEST_STATE_SINK_TABLE_REQUEST5:
    {
      zb_uint8_t   options;
      zb_zgpd_id_t zgpd_id;

      options = ZB_ZGP_GP_PROXY_TBL_REQ_FILL_OPT(ZB_ZGP_APP_ID_0010,
                                                 ZGP_REQUEST_TABLE_ENTRIES_BY_GPD_ID);
      zgpd_id.app_id = ZB_ZGP_APP_ID_0010;
      zgpd_id.endpoint = 1;
      ZB_IEEE_ADDR_COPY(&zgpd_id.addr.ieee_addr, &g_zgpd_addr);
      zgp_cluster_send_gp_sink_table_request(buf_ref,
                                             DUT_GPS_ADDR,
                                             DUT_GPS_ADDR_MODE,
                                             options,
                                             &zgpd_id,
                                             0,
                                             cb);
      break;
    }
    case TEST_STATE_SINK_TABLE_REQUEST6:
    {
      zb_uint8_t options;

      options = ZB_ZGP_GP_PROXY_TBL_REQ_FILL_OPT(ZB_ZGP_APP_ID_0000,
                                                 ZGP_REQUEST_TABLE_ENTRIES_BY_INDEX);
      zgp_cluster_send_gp_sink_table_request(buf_ref,
                                             DUT_GPS_ADDR,
                                             DUT_GPS_ADDR_MODE,
                                             options,
                                             NULL,
                                             3,
                                             cb);
      break;
    }
    case TEST_STATE_SINK_TABLE_REQUEST7:
    {
      zb_uint8_t   options;
      zb_zgpd_id_t zgpd_id;

      options = ZB_ZGP_GP_PROXY_TBL_REQ_FILL_OPT(ZB_ZGP_APP_ID_0000,
                                                 ZGP_REQUEST_TABLE_ENTRIES_BY_GPD_ID);
      zgpd_id.app_id = ZB_ZGP_APP_ID_0000;
      zgpd_id.endpoint = 0;
      zgpd_id.addr.src_id = 0x12345677;
      zgp_cluster_send_gp_sink_table_request(buf_ref,
                                             DUT_GPS_ADDR,
                                             DUT_GPS_ADDR_MODE,
                                             options,
                                             &zgpd_id,
                                             0,
                                             cb);
      break;
    }
    case TEST_STATE_SINK_TABLE_REQUEST8:
    {
      zb_uint8_t   options;
      zb_zgpd_id_t zgpd_id;

      options = ZB_ZGP_GP_PROXY_TBL_REQ_FILL_OPT(ZB_ZGP_APP_ID_0010,
                                                 ZGP_REQUEST_TABLE_ENTRIES_BY_GPD_ID);
      zgpd_id.app_id = ZB_ZGP_APP_ID_0010;
      zgpd_id.endpoint = 1;
      ZB_IEEE_ADDR_COPY(&zgpd_id.addr.ieee_addr, &g_zgpd_addr_w);
      zgp_cluster_send_gp_sink_table_request(buf_ref,
                                             DUT_GPS_ADDR,
                                             DUT_GPS_ADDR_MODE,
                                             options,
                                             &zgpd_id,
                                             0,
                                             cb);
      break;
    }
  };
}

static void read_attr_handler(zb_uint8_t buf_ref,
                                   zb_zcl_read_attr_res_t *resp)
{
  zb_bool_t test_error = ZB_FALSE;

  ZVUNUSED(buf_ref);

  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_READ_GPS_COMMUNICATION_MODE1:
    case TEST_STATE_READ_GPS_COMMUNICATION_MODE2:
    case TEST_STATE_READ_GPS_COMMISSIONING_EXIT_MODE1:
    case TEST_STATE_READ_GPS_COMMISSIONING_EXIT_MODE2:
    case TEST_STATE_READ_GPS_COMMISSIONING_WINDOW1:
    case TEST_STATE_READ_GPS_COMMISSIONING_WINDOW2:
    case TEST_STATE_READ_GPS_SECURITY_LEVEL1:
    case TEST_STATE_READ_GPS_SECURITY_LEVEL2:
    case TEST_STATE_READ_GPS_MAX_SINK_TABLE_ENTRIES1:
    case TEST_STATE_READ_GPS_MAX_SINK_TABLE_ENTRIES2:
    case TEST_STATE_READ_GPS_SINK_TABLE1:
    case TEST_STATE_READ_GPS_SINK_TABLE2:
    case TEST_STATE_READ_GPS_SINK_TABLE3:
    case TEST_STATE_READ_GPS_FUNCTIONALITY1:
    case TEST_STATE_READ_GPS_FUNCTIONALITY2:
    case TEST_STATE_READ_GPS_ACTIVE_FUNCTIONALITY1:
    case TEST_STATE_READ_GPS_ACTIVE_FUNCTIONALITY2:
    {
      if (resp->status != ZB_ZCL_STATUS_SUCCESS)
      {
        test_error = ZB_TRUE;
      }
    }
    break;
    case TEST_STATE_READ_GPS_SINK_TABLE4:
    {
      if (resp->status != ZB_ZCL_STATUS_INSUFF_SPACE)
      {
        test_error = ZB_TRUE;
      }
    }
    break;
  }

  if (test_error)
  {
    TEST_DEVICE_CTX.err_cnt++;
    TRACE_MSG(TRACE_APP1, "Error at state: %hd", (FMT__H, TEST_DEVICE_CTX.test_state));
  }
}

static void write_attr_handler(zb_uint8_t buf_ref,
                                    zb_zcl_write_attr_res_t *resp)
{
  zb_bool_t test_error = ZB_FALSE;

  ZVUNUSED(buf_ref);

  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_WRITE_GPS_COMMUNICATION_MODE:
    case TEST_STATE_WRITE_GPS_COMMISSIONING_EXIT_MODE:
    case TEST_STATE_WRITE_GPS_COMMISSIONING_WINDOW:
    case TEST_STATE_WRITE_GPS_SECURITY_LEVEL:
    {
      if (resp->status != ZB_ZCL_STATUS_SUCCESS)
      {
        test_error = ZB_TRUE;
      }
    }
    break;
    case TEST_STATE_WRITE_GPS_MAX_SINK_TABLE_ENTRIES:
    case TEST_STATE_WRITE_GPS_SINK_TABLE:
    case TEST_STATE_WRITE_GPS_FUNCTIONALITY:
    case TEST_STATE_WRITE_GPS_ACTIVE_FUNCTIONALITY:
    {
      if (resp->status != ZB_ZCL_STATUS_READ_ONLY)
      {
        test_error = ZB_TRUE;
      }
    }
    break;
  }

  if (test_error)
  {
    TEST_DEVICE_CTX.err_cnt++;
    TRACE_MSG(TRACE_APP1, "Error at state: %hd", (FMT__H, TEST_DEVICE_CTX.test_state));
  }
}

static void handle_gp_sink_table_response(zb_uint8_t buf_ref)
{
  zb_bool_t   test_error = ZB_FALSE;
  zb_buf_t   *buf = ZB_BUF_FROM_REF(buf_ref);
  zb_uint8_t *ptr = ZB_BUF_BEGIN(buf);

  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_SINK_TABLE_REQUEST1:
    case TEST_STATE_SINK_TABLE_REQUEST2:
    {
      zb_uint8_t status;

      ZB_ZCL_PACKET_GET_DATA8(&status, ptr);

      if (status != ZB_ZCL_STATUS_SUCCESS)
      {
        test_error = ZB_TRUE;
      }
    }
    break;
    case TEST_STATE_SINK_TABLE_REQUEST3:
    case TEST_STATE_SINK_TABLE_REQUEST4:
    case TEST_STATE_SINK_TABLE_REQUEST5:
    {
      zb_uint8_t status;

      ZB_ZCL_PACKET_GET_DATA8(&status, ptr);

      if (status != ZB_ZCL_STATUS_SUCCESS)
      {
        test_error = ZB_TRUE;
      }
    }
    break;
    case TEST_STATE_SINK_TABLE_REQUEST6:
    case TEST_STATE_SINK_TABLE_REQUEST7:
    case TEST_STATE_SINK_TABLE_REQUEST8:
    {
      zb_uint8_t status;

      ZB_ZCL_PACKET_GET_DATA8(&status, ptr);

      if (status != ZB_ZCL_STATUS_NOT_FOUND)
      {
        test_error = ZB_TRUE;
      }
    }
    break;
  }

  if (test_error)
  {
    TEST_DEVICE_CTX.err_cnt++;
    TRACE_MSG(TRACE_APP1, "Error at state: %hd", (FMT__H, TEST_DEVICE_CTX.test_state));
  }
}

static void handle_gp_pairing(zb_uint8_t buf_ref)
{
  ZVUNUSED(buf_ref);

  if (TEST_DEVICE_CTX.test_state == TEST_STATE_SINK_TABLE_REQUEST1 ||
      TEST_DEVICE_CTX.test_state == TEST_STATE_SINK_TABLE_REQUEST2 ||
      TEST_DEVICE_CTX.test_state == TEST_STATE_READ_GPS_SINK_TABLE4)
  {
    ZB_SCHEDULE_ALARM(PERFORM_NEXT_STATE, 0, ZB_TIME_ONE_SECOND);
  }
}

static void perform_next_state(zb_uint8_t param)
{
  if (TEST_DEVICE_CTX.pause)
  {
    ZB_SCHEDULE_ALARM(perform_next_state, 0,
                      ZB_TIME_ONE_SECOND*TEST_DEVICE_CTX.pause);
    TEST_DEVICE_CTX.pause = 0;
    return;
  }

  TEST_DEVICE_CTX.test_state++;

  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_FINISHED:
      if (TEST_DEVICE_CTX.err_cnt)
      {
        TRACE_MSG(TRACE_APP1, "Test finished. Status: ERROR[%hd]", (FMT__H, TEST_DEVICE_CTX.err_cnt));
      }
      else
      {
        TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
      }
      break;
    default:
    {
      if (param)
      {
        zb_free_buf(ZB_BUF_FROM_REF(param));
      }
      ZB_SCHEDULE_ALARM(test_send_command, 0, ZB_TIME_ONE_SECOND);
    }
  }
}

static void zgpc_custom_startup()
{
/* Init device, load IB values from nvram or set it to default */
  ZB_INIT("th_tool");

  ZB_AIB().aps_channel_mask = (1<<TEST_CHANNEL);
  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_th_tool_addr);
  ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);

  zb_set_default_ed_descriptor_values();

  ZGP_CTX().device_role = ZGP_DEVICE_COMMISSIONING_TOOL;

  TEST_DEVICE_CTX.cli_r_attr_hndlr_cb = read_attr_handler;
  TEST_DEVICE_CTX.cli_w_attr_hndlr_cb = write_attr_handler;
  TEST_DEVICE_CTX.gp_pairing_hndlr_cb = handle_gp_pairing;
  TEST_DEVICE_CTX.gp_sink_tbl_req_cb = handle_gp_sink_table_response;
}

#endif /* ZB_CERTIFICATION_HACKS */

ZB_ZGPC_TH_DECLARE_STARTUP_PROCESS()
