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
/* PURPOSE: TH GPP
*/

#define ZB_TEST_NAME GPS_COMMISSIONING_GENERAL_TH_GPP
#define ZB_TRACE_FILE_ID 41268

#include "test_config.h"

#include "../include/zgp_test_templates.h"

#ifdef ZB_CERTIFICATION_HACKS

static zb_uint8_t g_shared_key[] = TEST_SEC_KEY;
static zb_ieee_addr_t g_th_gpp_addr = TH_GPP_IEEE_ADDR;

enum test_states_e
{
  TEST_STATE_INITIAL,
  TEST_STATE_READ_GPS_SINK_TABLE,
#ifdef ZB_NSNG
  TEST_STATE_WAIT_FOR_PAIRING1,
#endif
  TEST_STATE_READ_GPS_SINK_TABLE1,
  TEST_STATE_NT_REPLACE_FRAME_COUNTER,
  TEST_STATE_READ_GPS_SINK_TABLE2,
#ifdef ZB_NSNG
  TEST_STATE_WAIT_FOR_PAIRING2,
#endif
  TEST_STATE_READ_GPS_SINK_TABLE3,
  TEST_STATE_READ_GPS_SECURITY_LVL,
  TEST_STATE_READ_GPS_SINK_TABLE4,
  TEST_STATE_READ_GPS_SINK_TABLE5,
  TEST_STATE_WRITE_GPS_SECURITY_LVL1,
  TEST_STATE_READ_GPS_SINK_TABLE6,//10
  TEST_STATE_WRITE_GPS_SECURITY_LVL2,
  TEST_STATE_READ_GPS_SINK_TABLE7,
  TEST_STATE_WRITE_GPS_SECURITY_LVL3,
  TEST_STATE_READ_GPS_SINK_TABLE8,
  TEST_STATE_REPLACE_APP_ID,
  TEST_STATE_READ_GPS_SINK_TABLE9 ,
  TEST_STATE_REPLACE_GPD_ID,
  TEST_STATE_READ_GPS_SINK_TABLE10,
  TEST_STATE_REPLACE_IEEE_ID,
  TEST_STATE_READ_GPS_SINK_TABLE11,
#ifdef ZB_NSNG
  TEST_STATE_WAIT_FOR_PAIRING3,
#endif
#ifdef ZB_NSNG
  TEST_STATE_WAIT_FOR_PAIRING4,
#endif
  TEST_STATE_READ_GPS_SINK_TABLE12,
#ifdef ZB_NSNG
  TEST_STATE_WAIT_FOR_PAIRING5,
#endif
  TEST_STATE_READ_GPS_SINK_TABLE13,
  TEST_STATE_FINISHED
};

ZB_ZGPC_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 1000)

static void send_zcl(zb_uint8_t buf_ref, zb_callback_t cb)
{
  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_READ_GPS_SINK_TABLE1:
    case TEST_STATE_READ_GPS_SINK_TABLE3:
    case TEST_STATE_READ_GPS_SINK_TABLE5:
    case TEST_STATE_READ_GPS_SINK_TABLE6:
    case TEST_STATE_READ_GPS_SINK_TABLE7:
    case TEST_STATE_READ_GPS_SINK_TABLE8:
    case TEST_STATE_READ_GPS_SINK_TABLE9:
    case TEST_STATE_READ_GPS_SINK_TABLE10:
      zgp_cluster_read_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                            ZB_ZCL_ATTR_GPS_SINK_TABLE_ID,
                            ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
      TEST_DEVICE_CTX.skip_next_state = NEXT_STATE_PASSED;
      break;
    case TEST_STATE_READ_GPS_SINK_TABLE:
    case TEST_STATE_READ_GPS_SINK_TABLE4:
    case TEST_STATE_READ_GPS_SINK_TABLE11:
    case TEST_STATE_READ_GPS_SINK_TABLE12:
    case TEST_STATE_READ_GPS_SINK_TABLE13:
      zgp_cluster_read_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                            ZB_ZCL_ATTR_GPS_SINK_TABLE_ID,
                            ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
      ZB_ZGPC_SET_PAUSE(2);
      break;
    case TEST_STATE_READ_GPS_SINK_TABLE2:
      zgp_cluster_read_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                            ZB_ZCL_ATTR_GPS_SINK_TABLE_ID,
                            ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
      break;
    case TEST_STATE_READ_GPS_SECURITY_LVL:
      zgp_cluster_read_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                            ZB_ZCL_ATTR_GPS_SECURITY_LEVEL_ID,
                            ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
      ZB_ZGPC_SET_PAUSE(2);
      break;
    case TEST_STATE_WRITE_GPS_SECURITY_LVL1:
    {
      zb_uint8_t val;

      val = ZB_ZGP_FILL_GPS_SECURITY_LEVEL(
                            DUT_GPS_SEC_LVL,
                            1,
                            ZB_ZGP_DEFAULT_SEC_LEVEL_INVOLVE_TC);
      zgp_cluster_write_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                             ZB_ZCL_ATTR_GPS_SECURITY_LEVEL_ID,
                             ZB_ZCL_ATTR_TYPE_8BITMAP, (zb_uint8_t *)&val,
                             ZB_ZCL_DISABLE_DEFAULT_RESPONSE, cb);
      ZB_ZGPC_SET_PAUSE(2);
      break;
    }
    case TEST_STATE_WRITE_GPS_SECURITY_LVL2:
    {
      zb_uint8_t val;

      val = ZB_ZGP_FILL_GPS_SECURITY_LEVEL(
                            ZB_ZGP_SEC_LEVEL_FULL_WITH_ENC,
                            0,
                            ZB_ZGP_DEFAULT_SEC_LEVEL_INVOLVE_TC);
      zgp_cluster_write_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                             ZB_ZCL_ATTR_GPS_SECURITY_LEVEL_ID,
                             ZB_ZCL_ATTR_TYPE_8BITMAP, (zb_uint8_t *)&val,
                             ZB_ZCL_DISABLE_DEFAULT_RESPONSE, cb);
      ZB_ZGPC_SET_PAUSE(2);
      break;
    }
    case TEST_STATE_WRITE_GPS_SECURITY_LVL3:
    {
      zb_uint8_t val;

      val = ZB_ZGP_FILL_GPS_SECURITY_LEVEL(
                            DUT_GPS_SEC_LVL,
                            DUT_GPS_SEC_LVL_PLK,
                            ZB_ZGP_DEFAULT_SEC_LEVEL_INVOLVE_TC);
      zgp_cluster_write_attr(buf_ref, DUT_GPS_ADDR, DUT_GPS_ADDR_MODE,
                             ZB_ZCL_ATTR_GPS_SECURITY_LEVEL_ID,
                             ZB_ZCL_ATTR_TYPE_8BITMAP, (zb_uint8_t *)&val,
                             ZB_ZCL_DISABLE_DEFAULT_RESPONSE, cb);
      ZB_ZGPC_SET_PAUSE(2);
      break;
    }
  };
}

static void handle_gp_sink_table_response(zb_uint8_t buf_ref)
{
  zb_bool_t   test_error = ZB_FALSE;
  zb_buf_t   *buf = ZB_BUF_FROM_REF(buf_ref);
  zb_uint8_t *ptr = ZB_BUF_BEGIN(buf);

  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_READ_GPS_SINK_TABLE:
    case TEST_STATE_READ_GPS_SINK_TABLE1:
    case TEST_STATE_READ_GPS_SINK_TABLE2:
    case TEST_STATE_READ_GPS_SINK_TABLE3:
    case TEST_STATE_READ_GPS_SINK_TABLE4:
    case TEST_STATE_READ_GPS_SINK_TABLE5:
    case TEST_STATE_READ_GPS_SINK_TABLE6:
    case TEST_STATE_READ_GPS_SINK_TABLE7:
    case TEST_STATE_READ_GPS_SINK_TABLE8:
    case TEST_STATE_READ_GPS_SINK_TABLE9:
    case TEST_STATE_READ_GPS_SINK_TABLE10:
    case TEST_STATE_READ_GPS_SINK_TABLE11:
    case TEST_STATE_READ_GPS_SINK_TABLE12:
    case TEST_STATE_READ_GPS_SINK_TABLE13:
    {
      zb_uint8_t status;

      ZB_ZCL_PACKET_GET_DATA8(&status, ptr);

      if (status != ZB_ZCL_STATUS_SUCCESS)
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

#ifdef ZB_NSNG
static void handle_gp_pairing(zb_uint8_t buf_ref)
{
  ZVUNUSED(buf_ref);

  if (TEST_DEVICE_CTX.test_state == TEST_STATE_WAIT_FOR_PAIRING1 ||
      TEST_DEVICE_CTX.test_state == TEST_STATE_WAIT_FOR_PAIRING2 ||
      TEST_DEVICE_CTX.test_state == TEST_STATE_WAIT_FOR_PAIRING3 ||
      TEST_DEVICE_CTX.test_state == TEST_STATE_WAIT_FOR_PAIRING3 ||
      TEST_DEVICE_CTX.test_state == TEST_STATE_WAIT_FOR_PAIRING4)
  {
    ZB_SCHEDULE_ALARM(PERFORM_NEXT_STATE, 0, ZB_TIME_ONE_SECOND);
  }
}
#endif

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
#ifdef ZB_NSNG
    case TEST_STATE_WAIT_FOR_PAIRING1:
    case TEST_STATE_WAIT_FOR_PAIRING2:
    case TEST_STATE_WAIT_FOR_PAIRING3:
    case TEST_STATE_WAIT_FOR_PAIRING4:
    case TEST_STATE_WAIT_FOR_PAIRING5:
      break;
#endif
    case TEST_STATE_READ_GPS_SECURITY_LVL:
    case TEST_STATE_WRITE_GPS_SECURITY_LVL1:
    case TEST_STATE_WRITE_GPS_SECURITY_LVL2:
    case TEST_STATE_WRITE_GPS_SECURITY_LVL3:
    case TEST_STATE_READ_GPS_SINK_TABLE:
    case TEST_STATE_READ_GPS_SINK_TABLE1:
    case TEST_STATE_READ_GPS_SINK_TABLE2:
    case TEST_STATE_READ_GPS_SINK_TABLE3:
    case TEST_STATE_READ_GPS_SINK_TABLE4:
    case TEST_STATE_READ_GPS_SINK_TABLE5:
    case TEST_STATE_READ_GPS_SINK_TABLE6:
    case TEST_STATE_READ_GPS_SINK_TABLE7:
    case TEST_STATE_READ_GPS_SINK_TABLE8:
    case TEST_STATE_READ_GPS_SINK_TABLE9:
    case TEST_STATE_READ_GPS_SINK_TABLE10:
    case TEST_STATE_READ_GPS_SINK_TABLE11:
    case TEST_STATE_READ_GPS_SINK_TABLE12:
    case TEST_STATE_READ_GPS_SINK_TABLE13:
      if (param)
      {
        zb_free_buf(ZB_BUF_FROM_REF(param));
      }
      ZB_SCHEDULE_ALARM(test_send_command, 0, (zb_time_t)(0.1f * ZB_TIME_ONE_SECOND));
      break;
    case TEST_STATE_REPLACE_APP_ID:
    case TEST_STATE_REPLACE_GPD_ID:
    case TEST_STATE_REPLACE_IEEE_ID:
      TRACE_MSG(TRACE_APP1, "Test state: [%hd]", (FMT__H, TEST_DEVICE_CTX.test_state));
      break;
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
  };
}

static void gpp_gp_notify_hacks_clear()
{
  ZB_CERT_HACKS().gp_proxy_replace_gp_notif_sec_frame_counter = ZB_FALSE;
}

static void gpp_send_gp_notify_cb(zb_uint8_t param)
{
  zb_buf_t       *buf = ZB_BUF_FROM_REF(param);
  zb_gpdf_info_t *gpdf_info = ZB_GET_BUF_PARAM(buf, zb_gpdf_info_t);

  TRACE_MSG(TRACE_APP1, "Test gp state: [%hd]", (FMT__H, TEST_DEVICE_CTX.test_state));

  gpp_gp_notify_hacks_clear();
  switch(TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_NT_REPLACE_FRAME_COUNTER:
      ZB_CERT_HACKS().gp_proxy_replace_gp_notif_sec_frame_counter = ZB_TRUE;
      ZB_CERT_HACKS().gp_proxy_replace_sec_frame_counter = gpdf_info->sec_frame_counter - 2;
      ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(0.1f * ZB_TIME_ONE_SECOND));
      break;
  };
}

static void gpp_gp_comm_notify_hacks_clear()
{
  ZB_CERT_HACKS().gp_proxy_replace_comm_app_id = ZB_FALSE;
  ZB_CERT_HACKS().gp_proxy_replace_comm_gpd_id = ZB_FALSE;
  ZB_CERT_HACKS().gp_proxy_replace_comm_app_id = ZB_FALSE;
  ZB_CERT_HACKS().gp_proxy_replace_comm_app_id_format = ZB_FALSE;
  ZB_CERT_HACKS().gp_proxy_replace_comm_gpd_id = ZB_FALSE;
}

static void gpp_send_gp_comm_notify_cb(zb_uint8_t param)
{
  ZVUNUSED(param);

  TRACE_MSG(TRACE_APP1, "Test gp comm state: [%hd]", (FMT__H, TEST_DEVICE_CTX.test_state));

  gpp_gp_comm_notify_hacks_clear();
  switch(TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_REPLACE_APP_ID:
      ZB_CERT_HACKS().gp_proxy_replace_comm_app_id = ZB_TRUE;
      ZB_CERT_HACKS().gp_proxy_replace_comm_app_id_value = 3;
      ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(0.1f * ZB_TIME_ONE_SECOND));
      break;
    case TEST_STATE_REPLACE_GPD_ID:
      ZB_CERT_HACKS().gp_proxy_replace_comm_gpd_id = ZB_TRUE;
      ZB_CERT_HACKS().gp_proxy_replace_comm_gpd_id_value = 0;
      ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(0.1f * ZB_TIME_ONE_SECOND));
      break;
    case TEST_STATE_REPLACE_IEEE_ID:
      ZB_CERT_HACKS().gp_proxy_replace_comm_app_id = ZB_TRUE;
      ZB_CERT_HACKS().gp_proxy_replace_comm_app_id_format = ZB_TRUE;
      ZB_CERT_HACKS().gp_proxy_replace_comm_app_id_value = 2;
      ZB_CERT_HACKS().gp_proxy_replace_comm_gpd_id = ZB_TRUE;
      ZB_64BIT_ADDR_ZERO(ZB_CERT_HACKS().gp_proxy_replace_comm_gpd_ieee_value);
      ZB_CERT_HACKS().gp_proxy_replace_comm_gpd_ep_value = 1;
      ZB_SCHEDULE_ALARM(perform_next_state, 0, (zb_time_t)(0.1f * ZB_TIME_ONE_SECOND));
      break;
  };
}

static void zgpc_custom_startup()
{
/* Init device, load IB values from nvram or set it to default */
  ZB_INIT("th_gpp");

  ZB_AIB().aps_channel_mask = (1<<TEST_CHANNEL);
  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_th_gpp_addr);
  ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);

  zb_set_default_ed_descriptor_values();

  ZB_NIB_SET_USE_MULTICAST(ZB_FALSE);

  ZB_AIB().aps_use_nvram = 1;

  ZGP_GP_SET_SHARED_SECURITY_KEY_TYPE(TEST_KEY_TYPE);
  ZB_MEMCPY(ZGP_GP_SHARED_SECURITY_KEY, g_shared_key, ZB_CCM_KEY_SIZE);

  ZGP_CTX().device_role = ZGP_DEVICE_PROXY_BASIC;

  ZB_CERT_HACKS().gp_proxy_gp_notif_req_cb = gpp_send_gp_notify_cb;
  ZB_CERT_HACKS().gp_proxy_gp_comm_notif_req_cb = gpp_send_gp_comm_notify_cb;
#ifdef ZB_NSNG
  TEST_DEVICE_CTX.gp_pairing_hndlr_cb = handle_gp_pairing;
#endif
  TEST_DEVICE_CTX.gp_sink_tbl_req_cb = handle_gp_sink_table_response;
}

#endif /* ZB_CERTIFICATION_HACKS */

ZB_ZGPC_TH_DECLARE_STARTUP_PROCESS()
