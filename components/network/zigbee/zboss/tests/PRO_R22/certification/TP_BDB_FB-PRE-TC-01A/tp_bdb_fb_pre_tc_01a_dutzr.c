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

#define ZB_TEST_NAME TP_BDB_FB_PRE_TC_01A_DUTZR
#define ZB_TRACE_FILE_ID 40636

/*
  Attention:	This test requires sending ieee_addr_req, but also it
                requires switching DUT off before THr1 and THe1 connected.
                This causes to DUT does not know short addresses of THr1 and THe1.
                Now DUT broadcasts nwk_addr_req to get this short addresses.
*/

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_zcl.h"
#include "zb_bdb_internal.h"
#include "zb_zcl.h"
#include "zb_console_monitor.h"

#include "shade_controller.h"
#include "tp_bdb_fb_pre_tc_01a_common.h"
#include "../common/zb_cert_test_globals.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_CERTIFICATION_HACKS
#error Define ZB_CERTIFICATION_HACKS
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

enum test_steps_e
{
  TEST_COMMAND_START_TEST,
  TEST_COMMAND_NWK_ADDR_REQ_THR1,
  TEST_COMMAND_IEEE_ADDR_REQ_THR1,
  TEST_COMMAND_NWK_ADDR_REQ_THE1,
  TEST_COMMAND_IEEE_ADDR_REQ_THE1,
  TEST_COMMAND_STEPS_COUNT
};

typedef ZB_PACKED_PRE struct test_device_nvram_dataset_s
{
  zb_uint8_t step_idx;
  zb_uint8_t align[3];
} ZB_PACKED_STRUCT test_device_nvram_dataset_t;

static zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;
static zb_ieee_addr_t g_ieee_addr_thr1 = IEEE_ADDR_THR1;
static zb_ieee_addr_t g_ieee_addr_the1 = IEEE_ADDR_THE1;

static zb_uint8_t g_nwk_key[16] = {0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static zb_uint8_t     s_step_idx;
static zb_uint16_t    s_payload_nwk;
static zb_ieee_addr_t s_payload_ieee;

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(fb_pre_tc_01a_dut_r_basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(fb_pre_tc_01a_dut_r_identify_attr_list, &attr_identify_time);


/********************* Declare device **************************/
DECLARE_SHADE_CONTROLLER_CLUSTER_LIST(fb_pre_tc_01a_dut_r_shade_controller_clusters,
                                      fb_pre_tc_01a_dut_r_basic_attr_list,
                                      fb_pre_tc_01a_dut_r_identify_attr_list);

DECLARE_SHADE_CONTROLLER_EP(fb_pre_tc_01a_dut_r_shade_controller_ep,
                            DUT_ENDPOINT,
                            fb_pre_tc_01a_dut_r_shade_controller_clusters);

DECLARE_SHADE_CONTROLLER_CTX(fb_pre_tc_01a_dut_r_shade_controller_ctx,
                             fb_pre_tc_01a_dut_r_shade_controller_ep);

#ifdef ZB_USE_NVRAM
static zb_uint16_t test_get_nvram_data_size();
static void test_nvram_read_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length);
static zb_ret_t test_nvram_write_app_data(zb_uint8_t page, zb_uint32_t pos);
#endif

static void test_buffer_manager(zb_uint8_t unused);
static void test_nwk_addr_req(zb_uint8_t param);
static void test_nwk_addr_resp(zb_uint8_t param);
static void test_ieee_addr_req(zb_uint8_t param);
static void send_active_ep_req(zb_uint8_t param);

MAIN()
{
  ARGV_UNUSED;
	
	char command_buffer[100], *command_ptr;
  char next_cmd[40];
  zb_bool_t res;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_1_dutzr");
#if UART_CONTROL	
	test_control_init();
#endif


#ifdef ZB_USE_NVRAM
  zb_nvram_register_app1_read_cb(test_nvram_read_app_data);
  zb_nvram_register_app1_write_cb(test_nvram_write_app_data, test_get_nvram_data_size);
#endif

  zb_set_long_address(g_ieee_addr_dut);

  zb_zdo_set_aps_unsecure_join(ZB_TRUE);
  zb_set_network_router_role((1l << TEST_CHANNEL));
  zb_set_max_children(1);

/* Assignment required to force Distributed formation */
  zb_aib_set_trust_center_address(g_unknown_ieee_addr);
  zb_secur_setup_nwk_key(g_nwk_key, 0);

  /* Need NVRAM to transparrantly restart */
  TRACE_MSG(TRACE_APP1, "Send 'erase' for flash erase or just press enter to be continued \n", (FMT__0));
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_handler);
  zb_console_monitor_get_cmd((zb_uint8_t*)command_buffer, sizeof(command_buffer));
  command_ptr = (char *)(&command_buffer);
  res = parse_command_token(&command_ptr, next_cmd, sizeof(next_cmd));
  if (strcmp(next_cmd, "erase") == 0)
    zb_set_nvram_erase_at_start(ZB_TRUE);
  else
    zb_set_nvram_erase_at_start(ZB_FALSE);
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);

  /* uncomment if f&b required */
  /* ZB_AF_REGISTER_DEVICE_CTX(&fb_pre_tc_01a_dut_r_shade_controller_ctx); */

  if (zboss_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
  }
  else
  {
    zdo_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  switch (sig)
  {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      TRACE_MSG(TRACE_APS1, "Device started, status %d", (FMT__D, status));
      if (status == 0)
      {
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
      }
      break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_DEVICE_REBOOT:
      TRACE_MSG(TRACE_APS1, "Device STARTED: REBOOT, status %d", (FMT__D, status));
      if (status == 0)
      {
        ZB_SCHEDULE_CALLBACK(test_buffer_manager, 0);
      }
      break; /* ZB_BDB_SIGNAL_DEVICE_REBOOT */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}

#ifdef ZB_USE_NVRAM
static zb_uint16_t test_get_nvram_data_size()
{
  TRACE_MSG(TRACE_ERROR, "test_get_nvram_data_size, ret %hd", (FMT__H, sizeof(test_device_nvram_dataset_t)));
  return sizeof(test_device_nvram_dataset_t);
}

static void test_nvram_read_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length)
{
  test_device_nvram_dataset_t ds;
  zb_ret_t ret;

  TRACE_MSG(TRACE_ERROR, ">> test_nvram_read_app_data page %hd pos %d", (FMT__H_D, page, pos));

  ZB_ASSERT(payload_length == sizeof(ds));

  ret = zb_osif_nvram_read(page, pos, (zb_uint8_t*)&ds, sizeof(ds));

  if (ret == RET_OK)
  {
    s_step_idx = ds.step_idx;
  }

  TRACE_MSG(TRACE_ERROR, "<< test_nvram_read_app_data ret %d", (FMT__D, ret));
}

static zb_ret_t test_nvram_write_app_data(zb_uint8_t page, zb_uint32_t pos)
{
  zb_ret_t ret;
  test_device_nvram_dataset_t ds;

  TRACE_MSG(TRACE_ERROR, ">> test_nvram_write_app_data, page %hd, pos %d", (FMT__H_D, page, pos));

  ds.step_idx = s_step_idx;

  ret = zb_osif_nvram_write(page, pos, (zb_uint8_t*)&ds, sizeof(ds));

  TRACE_MSG(TRACE_ERROR, "<< test_nvram_write_app_data, ret %d", (FMT__D, ret));

  return ret;
}
#endif /* ZB_USE_NVRAM */

static void test_buffer_manager(zb_uint8_t unused)
{
  zb_callback_t call_func = 0;

  TRACE_MSG(TRACE_APP1, ">>test_buffer_manager", (FMT__0));

  ZVUNUSED(unused);
  s_step_idx++;
  zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);

  TRACE_MSG(TRACE_APP1, "test_buffer_manager: step = %d", (FMT__D, s_step_idx));

  switch (s_step_idx)
  {
    case TEST_COMMAND_NWK_ADDR_REQ_THR1:
      ZB_IEEE_ADDR_COPY(s_payload_ieee, g_ieee_addr_thr1);
      call_func = test_nwk_addr_req;
      break;
    case TEST_COMMAND_IEEE_ADDR_REQ_THR1:
      call_func = test_ieee_addr_req;
      break;
    case TEST_COMMAND_NWK_ADDR_REQ_THE1:
      ZB_IEEE_ADDR_COPY(s_payload_ieee, g_ieee_addr_the1);
      call_func = test_nwk_addr_req;
      break;
    case TEST_COMMAND_IEEE_ADDR_REQ_THE1:
      call_func = test_ieee_addr_req;
      break;
    default:
      TRACE_MSG(TRACE_ZDO1, "Unknown state, stop test, s_step_idx = %d", (FMT__D, s_step_idx));
      break;
  }

  if (call_func && s_payload_nwk != ZB_UNKNOWN_SHORT_ADDR)
  {
    zb_buf_get_out_delayed(call_func);
  }

  TRACE_MSG(TRACE_APP1, "<<test_buffer_manager", (FMT__0));
}

static void test_nwk_addr_req(zb_uint8_t param)
{
  zb_zdo_nwk_addr_req_param_t *req = zb_buf_get_tail(param, sizeof(zb_zdo_nwk_addr_req_param_t));

  TRACE_MSG(TRACE_ZDO1, ">>send_nwk_addr_req: buf_param = %d", (FMT__D, param));

  req->dst_addr = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
  ZB_IEEE_ADDR_COPY(req->ieee_addr, s_payload_ieee);
  req->request_type = 0x00;
  req->start_index = 0;
  zb_zdo_nwk_addr_req(param, test_nwk_addr_resp);

  TRACE_MSG(TRACE_ZDO1, "<<send_nwk_addr_req", (FMT__0));
}

static void test_nwk_addr_resp(zb_uint8_t param)
{
  zb_zdo_nwk_addr_resp_head_t *resp = (zb_zdo_nwk_addr_resp_head_t*) zb_buf_begin(param);

  TRACE_MSG(TRACE_ZDO1, ">>test_nwk_addr_resp: status = %d",
            (FMT__D_D, resp->status));

  if (resp->status == ZB_ZDP_STATUS_SUCCESS)
  {
    ZB_LETOH16(&s_payload_nwk, &resp->nwk_addr);
    ZB_SCHEDULE_CALLBACK(test_buffer_manager, 0);
  }

  zb_buf_free(param);

  TRACE_MSG(TRACE_ZDO1, "<<test_nwk_addr_resp", (FMT__0));
}

static void test_ieee_addr_req(zb_uint8_t param)
{
  zb_zdo_ieee_addr_req_param_t *req_param = ZB_BUF_GET_PARAM(param, zb_zdo_ieee_addr_req_param_t);

  TRACE_MSG(TRACE_ZDO1, "test_ieee_addr_req, dest_addr 0x%x", (FMT__H, s_payload_nwk));

  req_param->dst_addr = s_payload_nwk;
  req_param->nwk_addr = s_payload_nwk;
  req_param->start_index = 0;
  req_param->request_type = 0x00;

  //zb_zdo_ieee_addr_req(param, NULL);
	zb_zdo_ieee_addr_req(param, send_active_ep_req);
	
}

static void send_active_ep_req(zb_uint8_t param)
{
  zb_uint8_t *ptr;

  TRACE_MSG(TRACE_ZDO2, ">>send_active_ep_req: buf_param = %d", (FMT__D, param));

  ptr = zb_buf_initial_alloc(param, sizeof(zb_uint16_t));
	ZB_HTOLE16(ptr, &s_payload_nwk);
	
  zdo_send_req_by_short(ZDO_ACTIVE_EP_REQ_CLID,
                        param,
                        NULL,
                        s_payload_nwk,
                        ZB_ZDO_CB_DEFAULT_COUNTER);

  ZB_SCHEDULE_ALARM(test_buffer_manager, 0, DUT_TEST_DELAY);

  TRACE_MSG(TRACE_ZDO2, "<<send_active_ep_req", (FMT__0));
}

/*! @} */
