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
/* PURPOSE: TP_R22_SGMB_10 gZC
*/

#define ZB_TEST_NAME TP_R22_SGMB_10_GZC
#define ZB_TRACE_FILE_ID 40649

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "../nwk/nwk_internal.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

enum test_step_e
{
  UNKNOWN_ZDO_BRCAST,
  UNKNOWN_ZDO_UNICAST,
  UNKNOWN_NWK_BRCAST,
  UNKNOWN_NWK_UNICAST,
  CHECK_BIND_TABLE_1,
  BIND_USING_BRCAST,
  CHECK_BIND_TABLE_2,
  BIND_USING_UNICAST,
  CHECK_BIND_TABLE_3,
  UNBIND_USING_BRCAST,
  CHECK_BIND_TABLE_4,
  UNBIND_USING_UNICAST,
  CHECK_BIND_TABLE_5,
  TEST_STEPS_COUNT
};

enum bind_unbind_req_type_e
{
  SEND_UNBIND_REQ,
  SEND_BIND_REQ
};

static const zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;
static const zb_ieee_addr_t g_ieee_addr_gzc = IEEE_ADDR_gZC;

static zb_uint8_t g_step_idx;
static zb_uint16_t g_dut_short_addr;
static zb_zdo_bind_req_param_t g_temp_bind_req;

extern zb_uint8_t zdo_send_req_by_short(zb_uint16_t command_id, zb_uint8_t param, zb_callback_t cb,
                                        zb_uint16_t addr, zb_uint8_t resp_count);


static void test_logic_iteration(zb_uint8_t do_next_ts);
static void test_send_zdo_cmd(zb_uint8_t param, zb_uint16_t dst_addr);
static void test_send_nwk_cmd(zb_uint8_t param, zb_uint16_t dst_addr);
static void test_send_mgmt_bind_req(zb_uint8_t param, zb_uint16_t dst_addr);
static void test_fill_bind_req(zb_uint16_t dst_addr);
static void test_send_bind_unbind_req(zb_uint8_t param, zb_uint16_t bind);

MAIN()
{
  ARGV_UNUSED;

  ZB_INIT("zdo_2_gzc");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	
  zb_set_long_address(g_ieee_addr_gzc);
  zb_cert_test_set_common_channel_settings();

  zb_cert_test_set_zc_role();
  zb_set_use_extended_pan_id(g_ext_pan_id);
  zb_set_pan_id(TEST_PAN_ID);

#ifdef SECURITY_LEVEL
  zb_cert_test_set_security_level(SECURITY_LEVEL);
#endif

  zb_secur_setup_nwk_key((zb_uint8_t*) g_nwk_key0, 0);

  zb_set_nvram_erase_at_start(ZB_TRUE);

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
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

  TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  switch (sig)
  {
    case ZB_ZDO_SIGNAL_DEFAULT_START:
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ZB_BDB_SIGNAL_DEVICE_REBOOT:
      TRACE_MSG(TRACE_APS1, "Device started, status %d", (FMT__D, status));
      if (status == 0)
      {
        ZB_SCHEDULE_ALARM(test_logic_iteration, 0, TEST_GZC_STARTUP_DELAY);
      }
      break; /* ZB_ZDO_SIGNAL_DEFAULT_START */

    case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
      TRACE_MSG(TRACE_APS1, "signal: ZB_ZDO_SIGNAL_DEVICE_ANNCE, status %d", (FMT__D, status));
      if (status == 0)
      {
        zb_zdo_signal_device_annce_params_t *dev_annce_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_annce_params_t);

        g_dut_short_addr = dev_annce_params->device_short_addr;
      }
      break; /* ZB_ZDO_SIGNAL_DEVICE_ANNCE */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}

static void test_logic_iteration(zb_uint8_t do_next_ts)
{
  zb_callback2_t call_cb = NULL;
  zb_uint16_t param2;

  if (do_next_ts)
  {
    g_step_idx++;
  }

  TRACE_MSG(TRACE_ZDO1, ">>test_logic_iteration: step = %d", (FMT__D, g_step_idx));

  switch (g_step_idx)
  {
    case UNKNOWN_ZDO_BRCAST:
      TRACE_MSG(TRACE_ZDO1, "Unknown ZDO broadcast", (FMT__0));
      {
        param2 = ZB_NWK_BROADCAST_ALL_DEVICES;
        call_cb = test_send_zdo_cmd;
      }
      break;

    case UNKNOWN_ZDO_UNICAST:
      TRACE_MSG(TRACE_ZDO1, "Unknown ZDO unicast", (FMT__0));
      {
        param2 = g_dut_short_addr;
        call_cb = test_send_zdo_cmd;
      }
      break;

    case UNKNOWN_NWK_BRCAST:
      TRACE_MSG(TRACE_ZDO1, "Unknown NWK unicast", (FMT__0));
      {
        param2 = ZB_NWK_BROADCAST_ALL_DEVICES;
        call_cb = test_send_nwk_cmd;
      }
      break;

    case UNKNOWN_NWK_UNICAST:
      TRACE_MSG(TRACE_ZDO1, "Unknown NWK unicast", (FMT__0));
      {
        param2 = g_dut_short_addr;
        call_cb = test_send_nwk_cmd;
      }
      break;

    case CHECK_BIND_TABLE_1:
    case CHECK_BIND_TABLE_2:
    case CHECK_BIND_TABLE_3:
    case CHECK_BIND_TABLE_4:
    case CHECK_BIND_TABLE_5:
      TRACE_MSG(TRACE_ZDO1, "Verify binding table", (FMT__0));
      {
        param2 = g_dut_short_addr;
        call_cb = test_send_mgmt_bind_req;
      }
      break;

    case BIND_USING_BRCAST:
      TRACE_MSG(TRACE_ZDO1, "Broadcast ZDO Binding", (FMT__0));
      {
        test_fill_bind_req(ZB_NWK_BROADCAST_ALL_DEVICES);
        param2 = SEND_BIND_REQ;
        call_cb = test_send_bind_unbind_req;
      }
      break;

    case BIND_USING_UNICAST:
      TRACE_MSG(TRACE_ZDO1, "Unicast ZDO Binding", (FMT__0));
      {
        test_fill_bind_req(g_dut_short_addr);
        param2 = SEND_BIND_REQ;
        call_cb = test_send_bind_unbind_req;
      }
      break;

    case UNBIND_USING_BRCAST:
      TRACE_MSG(TRACE_ZDO1, "Broadcast ZDO Unbind", (FMT__0));
      {
        test_fill_bind_req(ZB_NWK_BROADCAST_ALL_DEVICES);
        param2 = SEND_UNBIND_REQ;
        call_cb = test_send_bind_unbind_req;
      }
      break;

    case UNBIND_USING_UNICAST:
      TRACE_MSG(TRACE_ZDO1, "Unicast ZDO Unbind", (FMT__0));
      {
        test_fill_bind_req(g_dut_short_addr);
        param2 = SEND_UNBIND_REQ;
        call_cb = test_send_bind_unbind_req;
      }
      break;

    default:
      if (g_step_idx == TEST_STEPS_COUNT)
      {
        TRACE_MSG(TRACE_ZDO1, "test_logic_iteration: TEST FINISHED", (FMT__0));
      }
      else
      {
        TRACE_MSG(TRACE_ZDO1, "test_logic_iteration: TEST ERROR - illegal state", (FMT__0));
      }
      break;
  }

  if (call_cb != NULL)
  {
    if (zb_buf_get_out_delayed_ext(call_cb, param2, 0) != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "test_logic_iteration: zb_buf_get_out_delayed_ext failed", (FMT__0));

      ZB_SCHEDULE_ALARM(test_logic_iteration, 0, TEST_GZC_NEXT_TS_DELAY);
    }
    else
    {
      ZB_SCHEDULE_ALARM(test_logic_iteration, 1, TEST_GZC_NEXT_TS_DELAY);
    }
  }

  TRACE_MSG(TRACE_ZDO1, "<<test_logic_iteration", (FMT__0));
}

static void test_send_zdo_cmd(zb_bufid_t buf, zb_uint16_t dst_addr)
{
  zb_uint8_t *ptr;
  zb_uint8_t cmd_size;

  TRACE_MSG(TRACE_ZDO1, ">>test_send_zdo_cmd: param = %i", (FMT__D, buf));

  cmd_size = 10;

  ptr = zb_buf_initial_alloc(buf, cmd_size);
  ZB_BZERO(ptr, cmd_size);

  zdo_send_req_by_short(TEST_GZC_UNKNOWN_ID, buf, NULL,
                        dst_addr, ZB_ZDO_CB_DEFAULT_COUNTER);

  TRACE_MSG(TRACE_ZDO1, "<<test_send_zdo_cmd", (FMT__0));
}

static void test_send_nwk_cmd(zb_bufid_t buf, zb_uint16_t dst_addr)
{
  zb_nwk_hdr_t *nwhdr;
  zb_uint8_t *cmdp;
  zb_uint8_t hdr_size;
  zb_uint8_t cmd_size;
  zb_uint8_t has_dst_ieee = !ZB_NWK_IS_ADDRESS_BROADCAST(dst_addr);

  TRACE_MSG(TRACE_ZDO1, ">>test_send_nwk_cmd: param = %i", (FMT__D, buf));

  hdr_size = 3*sizeof(zb_uint16_t) + 2*sizeof(zb_uint8_t) + sizeof(zb_ieee_addr_t)
             + sizeof(zb_nwk_aux_frame_hdr_t);
  cmd_size = 10;

  if (has_dst_ieee)
  {
    hdr_size += sizeof(zb_ieee_addr_t);
  }

 nwhdr = zb_buf_initial_alloc(buf, hdr_size);

  ZB_BZERO2(nwhdr->frame_control);
  ZB_NWK_FRAMECTL_SET_FRAME_TYPE_N_PROTO_VER(nwhdr->frame_control,
  ZB_NWK_FRAME_TYPE_COMMAND, ZB_PROTOCOL_VERSION);
  ZB_NWK_FRAMECTL_SET_SECURITY(nwhdr->frame_control, 1);
  ZB_NWK_FRAMECTL_SET_SRC_DEST_IEEE(nwhdr->frame_control, 1, !!has_dst_ieee);
  zb_buf_flags_or(buf, ZB_BUF_SECUR_NWK_ENCR);

  nwhdr->src_addr = zb_cert_test_get_network_addr();
  nwhdr->dst_addr = dst_addr;
  nwhdr->radius = 1;
  nwhdr->seq_num = zb_cert_test_get_nib_seq_number();
  zb_cert_test_inc_nib_seq_number();

  zb_get_long_address(nwhdr->src_ieee_addr);

  if (has_dst_ieee)
  {
    zb_get_long_address(nwhdr->src_ieee_addr);
    ZB_IEEE_ADDR_COPY(nwhdr->dst_ieee_addr, g_ieee_addr_dut);
  }
  else
  {
    zb_get_long_address(nwhdr->dst_ieee_addr);
  }

  cmdp = (zb_uint8_t *)nwk_alloc_and_fill_cmd(buf, TEST_GZC_UNKNOWN_ID, cmd_size);
  ZB_BZERO(cmdp, cmd_size);

  ZB_BUF_GET_PARAM(buf, zb_apsde_data_ind_params_t)->handle = ZB_NWK_INTERNAL_NSDU_HANDLE;
  ZB_SCHEDULE_CALLBACK(zb_nwk_forward, buf);

  TRACE_MSG(TRACE_ZDO1, "<<test_send_nwk_cmd", (FMT__0));
}

static void test_send_mgmt_bind_req(zb_uint8_t param, zb_uint16_t dst_addr)
{
  zb_zdo_mgmt_bind_param_t *req_params;

  TRACE_MSG(TRACE_ZDO3, ">>test_send_mgmt_bind_req: param = %i", (FMT__D, param));


  req_params = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_bind_param_t);
  req_params->start_index = 0;
  req_params->dst_addr = dst_addr;

  zb_zdo_mgmt_bind_req(param, NULL);

  TRACE_MSG(TRACE_ZDO3, "<<test_send_mgmt_bind_req", (FMT__0));
}

static void test_fill_bind_req(zb_uint16_t dst_addr)
{
  TRACE_MSG(TRACE_ZDO3, ">>test_fill_bind_req: dst_addr = 0x%x", (FMT__H, dst_addr));

  ZB_IEEE_ADDR_COPY(g_temp_bind_req.src_address, g_ieee_addr_dut);
  g_temp_bind_req.src_endp = TEST_GZC_SRC_ENDPOINT;
  g_temp_bind_req.cluster_id = TP_BUFFER_TEST_REQUEST_CLID;
  /* 16-bit group address for DstAddress and DstEndp not present */
  g_temp_bind_req.dst_addr_mode = 0x01;
  g_temp_bind_req.dst_address.addr_short = 0xABCD;
  g_temp_bind_req.req_dst_addr = dst_addr;
}

static void test_send_bind_unbind_req(zb_uint8_t param, zb_uint16_t bind)
{
  zb_zdo_bind_req_param_t *req_params;

  TRACE_MSG(TRACE_ZDO3, ">>send_bind_unbind_req: param = %i", (FMT__D, param));

  req_params = ZB_BUF_GET_PARAM(param, zb_zdo_bind_req_param_t);

  ZB_MEMCPY(req_params, &g_temp_bind_req, sizeof(zb_zdo_bind_req_param_t));
  if (bind)
  {
    zb_zdo_bind_req(param, NULL);
  }
  else
  {
    zb_zdo_unbind_req(param, NULL);
  }

  TRACE_MSG(TRACE_ZDO3, "<<send_bind_unbind_req", (FMT__0));
}
