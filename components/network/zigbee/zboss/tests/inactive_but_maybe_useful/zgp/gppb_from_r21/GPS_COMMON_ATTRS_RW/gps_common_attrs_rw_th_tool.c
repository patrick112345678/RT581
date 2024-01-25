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

#define ZB_TEST_NAME GPS_COMMON_ATTRS_RW_TH_TOOL
#define ZB_TRACE_FILE_ID 41397

#include "test_config.h"

#include "../include/zgp_test_templates.h"

#ifdef ZB_CERTIFICATION_HACKS

static zb_ieee_addr_t g_th_tool_addr = TH_TOOL_IEEE_ADDR;

/*! Program states according to test scenario */
enum test_states_e
{
  TEST_STATE_INITIAL,
  TEST_STATE_READ_GP_SHARED_SECURITY_KEY_TYPE1,
  TEST_STATE_WRITE_GP_SHARED_SECURITY_KEY_TYPE,
  TEST_STATE_READ_GP_SHARED_SECURITY_KEY_TYPE2,
  TEST_STATE_READ_GP_SHARED_SECURITY_KEY1,
  TEST_STATE_WRITE_GP_SHARED_SECURITY_KEY,
  TEST_STATE_READ_GP_SHARED_SECURITY_KEY2,
  TEST_STATE_READ_GP_LINK_KEY1,
  TEST_STATE_WRITE_GP_LINK_KEY,
  TEST_STATE_READ_GP_LINK_KEY2,
  TEST_STATE_READ_CLUSTER_REVISION,
  TEST_STATE_FINISHED
};

ZB_ZGPC_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 1000)

static void send_zcl(zb_uint8_t buf_ref, zb_callback_t cb)
{
  zb_uint32_t val;

  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_READ_GP_SHARED_SECURITY_KEY_TYPE1:
    case TEST_STATE_READ_GP_SHARED_SECURITY_KEY_TYPE2:
      zgp_cluster_read_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                            ZB_ZCL_ATTR_GP_SHARED_SECURITY_KEY_TYPE_ID,
                            ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
      ZB_ZGP_SET_PASSED_STATE_SEQUENCE();
      break;
    case TEST_STATE_WRITE_GP_SHARED_SECURITY_KEY_TYPE:
      val = 0x07;
      zgp_cluster_write_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                             ZB_ZCL_ATTR_GP_SHARED_SECURITY_KEY_TYPE_ID,
                             ZB_ZCL_ATTR_TYPE_8BITMAP, (zb_uint8_t *)&val,
                             ZB_ZCL_DISABLE_DEFAULT_RESPONSE, cb);
      break;
    case TEST_STATE_READ_GP_SHARED_SECURITY_KEY1:
    case TEST_STATE_READ_GP_SHARED_SECURITY_KEY2:
      zgp_cluster_read_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                            ZB_ZCL_ATTR_GP_SHARED_SECURITY_KEY_ID,
                            ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
      break;
    case TEST_STATE_WRITE_GP_SHARED_SECURITY_KEY:
    {
      zb_uint8_t key[] = {0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF};
      zgp_cluster_write_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                             ZB_ZCL_ATTR_GP_SHARED_SECURITY_KEY_ID,
                             ZB_ZCL_ATTR_TYPE_128_BIT_KEY, key,
                             ZB_ZCL_DISABLE_DEFAULT_RESPONSE, cb);
      break;
    }
    case TEST_STATE_READ_GP_LINK_KEY1:
    case TEST_STATE_READ_GP_LINK_KEY2:
      zgp_cluster_read_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                            ZB_ZCL_ATTR_GP_LINK_KEY_ID,
                            ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
      break;
    case TEST_STATE_WRITE_GP_LINK_KEY:
    {
      zb_uint8_t key[] = "ZigbeeAlliance11";
      zgp_cluster_write_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                             ZB_ZCL_ATTR_GP_LINK_KEY_ID,
                             ZB_ZCL_ATTR_TYPE_128_BIT_KEY, key,
                             ZB_ZCL_DISABLE_DEFAULT_RESPONSE, cb);
      break;
    }
    case TEST_STATE_READ_CLUSTER_REVISION:
      zgp_cluster_read_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                            ZB_ZCL_ATTR_GLOBAL_CLUSTER_REVISION_ID,
                            ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
      break;
  };
}

static void read_attr_handler(zb_uint8_t buf_ref,
                                   zb_zcl_read_attr_res_t *resp)
{
  zb_bool_t test_error = ZB_FALSE;

  ZVUNUSED(buf_ref);

  TRACE_MSG(TRACE_APP1, "cur state: %hd", (FMT__H, TEST_DEVICE_CTX.test_state));

  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_READ_GP_SHARED_SECURITY_KEY_TYPE1:
    case TEST_STATE_READ_GP_SHARED_SECURITY_KEY_TYPE2:
    case TEST_STATE_READ_GP_SHARED_SECURITY_KEY1:
    case TEST_STATE_READ_GP_SHARED_SECURITY_KEY2:
    case TEST_STATE_READ_GP_LINK_KEY1:
    case TEST_STATE_READ_GP_LINK_KEY2:
    case TEST_STATE_READ_CLUSTER_REVISION:
    {
      if (resp->status != ZB_ZCL_STATUS_SUCCESS)
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

  TRACE_MSG(TRACE_APP1, "cur state: %hd", (FMT__H, TEST_DEVICE_CTX.test_state));

  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_WRITE_GP_SHARED_SECURITY_KEY_TYPE:
    case TEST_STATE_WRITE_GP_SHARED_SECURITY_KEY:
    case TEST_STATE_WRITE_GP_LINK_KEY:
    {
      if (resp->status != ZB_ZCL_STATUS_SUCCESS)
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
      ZB_SCHEDULE_ALARM(test_send_command, 0, (zb_time_t)(0.1f * ZB_TIME_ONE_SECOND));
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
}

#endif /* ZB_CERTIFICATION_HACKS */

ZB_ZGPC_TH_DECLARE_STARTUP_PROCESS()
