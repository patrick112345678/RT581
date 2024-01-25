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
/* PURPOSE: TH ZR1
*/

#define ZB_TEST_NAME TP_BDB_FB_PRE_TC_02_THR1
#define ZB_TRACE_FILE_ID 40548

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

#include "tp_bdb_fb_pre_tc_02_common.h"
#include "../common/zb_cert_test_globals.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

enum test_steps_e
{
  NWK_BRCAST_WITH_EXT_RESP1,
  NWK_BRCAST_WITH_SINGLE_RESP,
  NWK_BRCAST_WITH_EXT_RESP2,
  NWK_UNICAST_WITH_EXT_RESP,
  NWK_UNICAST_REQ_TO_DUT_CHILD,
  NWK_BRCAST_WITH_EXT_RESP_TO_DUT_CHILD,
  NWK_BRCAST_WITH_INV_REQ_TYPE,
  NWK_UNICAST_WITH_INV_REQ_TYPE,
  NWK_UNICAST_WITH_UNKNOWN_IEEE,
  NWK_BRCAST_WITH_UNKNOWN_IEEE,
  IEEE_UNICAST_WITH_EXT_RESP1,
  IEEE_UNICAST_WITH_SINGLE_RESP,
  IEEE_UNICAST_WITH_EXT_RESP2,
  IEEE_UNICAST_REQ_TO_DUT_CHILD,
  IEEE_BRCAST_WITH_INV_REQ_TYPE,
  IEEE_UNICAST_WITH_UNKNOWN_NWK,
  TEST_STEPS_COUNT
};

static zb_ieee_addr_t g_ieee_addr_thr1 = IEEE_ADDR_THR1;
static zb_ieee_addr_t g_ieee_addr_the1 = IEEE_ADDR_THE1;
static zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;

static zb_ieee_addr_t g_ieee_addr_not_in_network = IEEE_ADDR_NOT_IN_NETWORK;

static zb_uint8_t g_nwk_key[16] = {0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

extern zb_uint8_t zdo_send_req_by_short(zb_uint16_t command_id,
                                  zb_uint8_t param,
                                  zb_callback_t cb,
                                  zb_uint16_t addr,
                                  zb_uint8_t resp_counter);

static void device_annce_cb(zb_zdo_device_annce_t *da);
static void close_thr1_delayed(zb_uint8_t unused);
static void close_thr1(zb_uint8_t param);
static void start_test(zb_uint8_t unused);

static void send_nwk_addr_req(zb_uint8_t param);
static void send_ieee_addr_req(zb_uint8_t param);

static void switch_to_next_step(zb_uint8_t param);
static void test_step_actions(zb_uint8_t unused);

static zb_ieee_addr_t s_payload_ieee;
static zb_uint16_t    s_payload_nwk;
static zb_uint8_t     s_payload_req_type;
static int            s_step_idx;
static int            s_dut_is_ed;

MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_thr1");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif


  zb_set_long_address(g_ieee_addr_thr1);

  zb_cert_test_set_common_channel_settings();
  zb_set_network_router_role((1l << TEST_CHANNEL));
  zb_set_nvram_erase_at_start(ZB_TRUE);

  zb_aib_set_trust_center_address(g_unknown_ieee_addr);
  zb_secur_setup_nwk_key(g_nwk_key, 0);
  zb_set_max_children(1);

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


static void device_annce_cb(zb_zdo_device_annce_t *da)
{
  TRACE_MSG(TRACE_ZDO1, ">>dev_annce_cb, ieee = " TRACE_FORMAT_64 " addr = 0x%x",
            (FMT__A_H, TRACE_ARG_64(da->ieee_addr), da->nwk_addr));

  /* Test cases:
   * DUT-ZC:
   *  - DUT-ZC form the network
   *  - THr1 joins DUT's network
   *  - THr1 sends device announcement, but the DEVICE_ANNCE signal is not generated on the THr1
   *  - THe1 joins DUT-ZC
   *  - DUT ZC sends device announcement, the DEVICE_ANNCE signal is generated on the THr1 for THe1 device
   *
   * DUT-ZR:
   *  - DUT-ZR form the network
   *  - THr1 joins DUT's network
   *  - THr1 sends device announcement, but the DEVICE_ANNCE signal is not generated on the THr1
   *  - THe1 joins DUT-ZR
   *  - DUT ZC sends device announcement, the DEVICE_ANNCE signal is generated on the THr1 for THe1 device
   *
   * DUT-ZED:
   *  - THr1 form the network
   *  - DUT-ZED joins THr1's network
   *  - THr1 sends device announcement, the DEVICE_ANNCE signal is generated on the THr1 for DUT-ZED device
   *
   * Conclusions:
   * - There is only one DEVICE_ANNCE signal received on the THe1 in each test case.
   * - The DEVICE_ANNCE signal for the DUT address is generated on the THe1 only in DUT-ZC test case.
   * - Only in DUT-ZR and DUT-ZC test cases the DEVICE_ANNCE signal for THe1 is generated.
   */
  if (ZB_IEEE_ADDR_CMP(g_ieee_addr_the1, da->ieee_addr) == ZB_TRUE)
  {
    TRACE_MSG(TRACE_ZDO2, "dev_annce_cb: TH ZED has joined to network!", (FMT__0));
    ZB_SCHEDULE_ALARM(close_thr1_delayed, 0, THR1_WAITING_DURATION);
    ZB_SCHEDULE_ALARM(test_step_actions, 0, THR1_START_TEST_DELAY);
  }
  else if (ZB_IEEE_ADDR_CMP(g_ieee_addr_dut, da->ieee_addr) == ZB_TRUE)
  {
    s_dut_is_ed = 1;
    TRACE_MSG(TRACE_ZDO2, "dev_annce_cb: DUT ZED has joined to network!", (FMT__0));
    ZB_SCHEDULE_ALARM(start_test, 0, THR1_START_TEST_DELAY);
  }

  TRACE_MSG(TRACE_ZDO1, "<<dev_annce_cb", (FMT__0));
}


static void close_thr1_delayed(zb_uint8_t unused)
{
  ZVUNUSED(unused);
  zb_buf_get_out_delayed(close_thr1);
}


static void close_thr1(zb_uint8_t param)
{
  zb_nlme_permit_joining_request_t *req = zb_buf_get_tail(param, sizeof(zb_nlme_permit_joining_request_t));

  TRACE_MSG(TRACE_ZDO1, ">>close_thr1: buf_param = %d", (FMT__D, param));

  req->permit_duration = 0;
  zb_nlme_permit_joining_request(param);

  TRACE_MSG(TRACE_ZDO1, "<<close_thr1", (FMT__0));
}

static void start_test(zb_uint8_t unused)
{
  ZVUNUSED(unused);
  TRACE_MSG(TRACE_ZDO1, "TEST STARTED", (FMT__0));

  if (s_dut_is_ed)
  {
    s_step_idx = NWK_BRCAST_WITH_SINGLE_RESP;
  }
  if (zb_nwk_get_nbr_dvc_type_by_ieee(g_ieee_addr_dut) != ZB_NWK_DEVICE_TYPE_ED)
  {
    zb_buf_get_out_delayed(close_thr1);
  }
  ZB_SCHEDULE_CALLBACK(test_step_actions, 0);
}

static void send_nwk_addr_req(zb_uint8_t param)
{
  zb_zdo_nwk_addr_req_param_t *req = zb_buf_get_tail(param, sizeof(zb_zdo_nwk_addr_req_param_t));

  TRACE_MSG(TRACE_ZDO1, ">>send_nwk_addr_req: buf_param = %d", (FMT__D, param));

  req->dst_addr = s_payload_nwk;
  ZB_IEEE_ADDR_COPY(req->ieee_addr, s_payload_ieee);
  req->request_type = s_payload_req_type;
  req->start_index = 0;
  zb_zdo_nwk_addr_req(param, NULL);

  switch_to_next_step(0);

  TRACE_MSG(TRACE_ZDO1, "<<send_nwk_addr_req", (FMT__0));
}

static void send_ieee_addr_req(zb_uint8_t param)
{
  zb_zdo_ieee_addr_req_param_t *req;

  TRACE_MSG(TRACE_ZDO1, ">>send_ieee_addr_req: buf_param = %d", (FMT__D, param));

  req = ZB_BUF_GET_PARAM(param, zb_zdo_ieee_addr_req_param_t);

  req->nwk_addr = s_payload_nwk;
  req->dst_addr = zb_address_short_by_ieee(g_ieee_addr_dut);
  req->request_type = s_payload_req_type;
  req->start_index = 0;
  ZB_HTOLE16_ONPLACE(req->nwk_addr);
  zb_zdo_ieee_addr_req(param, NULL);

  switch_to_next_step(0);

  TRACE_MSG(TRACE_ZDO1, "<<send_ieee_addr_req", (FMT__0));
}

static void switch_to_next_step(zb_uint8_t param)
{
  int use_long_delay = 0;
  int stop = 0;

  TRACE_MSG(TRACE_ZDO1, ">>switch_to_next_step", (FMT__0));
  ZVUNUSED(param);

  ++s_step_idx;
  if (s_dut_is_ed)
  {
    switch (s_step_idx)
    {
      case NWK_UNICAST_REQ_TO_DUT_CHILD:
        s_step_idx = NWK_BRCAST_WITH_INV_REQ_TYPE;
        break;

      case IEEE_UNICAST_WITH_EXT_RESP1:
        s_step_idx = IEEE_UNICAST_WITH_SINGLE_RESP;
        break;
    }
  }
  else
  {
    switch (s_step_idx)
    {
      case NWK_BRCAST_WITH_SINGLE_RESP:
        stop = 1;
        break;

      case IEEE_UNICAST_WITH_EXT_RESP1:
        use_long_delay = 1;
        break;

      case IEEE_UNICAST_WITH_SINGLE_RESP:
        use_long_delay = 1;
        break;
    }
  }

  if (!stop)
  {
    if (use_long_delay)
    {
      ZB_SCHEDULE_ALARM(test_step_actions, 0, THR1_WAITING_DURATION);
    }
    else
    {
      ZB_SCHEDULE_ALARM(test_step_actions, 1, THR1_SHORT_DELAY);
    }
  }

  TRACE_MSG(TRACE_ZDO1, "<<switch_to_next_step", (FMT__0));
}


static void test_step_actions(zb_uint8_t unused)
{
  int stop_test = 0;
  zb_callback_t next_cb = NULL;

  ZVUNUSED(unused);
  TRACE_MSG(TRACE_ZDO1, ">>test_step_actions: step = %d", (FMT__D, s_step_idx));

  switch (s_step_idx)
  {
    case NWK_BRCAST_WITH_EXT_RESP1:
    case NWK_BRCAST_WITH_EXT_RESP2:
      {
        ZB_IEEE_ADDR_COPY(s_payload_ieee, g_ieee_addr_dut);
        s_payload_nwk = 0xfffd;
        s_payload_req_type = 0x01;
        next_cb = send_nwk_addr_req;
      }
      break;

    case NWK_BRCAST_WITH_SINGLE_RESP:
      {
        ZB_IEEE_ADDR_COPY(s_payload_ieee, g_ieee_addr_dut);
        s_payload_nwk = 0xfffd;
        s_payload_req_type = 0x00;
        next_cb = send_nwk_addr_req;
      }
      break;

    case NWK_UNICAST_WITH_EXT_RESP:
      {
        ZB_IEEE_ADDR_COPY(s_payload_ieee, g_ieee_addr_dut);
        s_payload_nwk = zb_address_short_by_ieee(g_ieee_addr_dut);
        s_payload_req_type = 0x01;
        next_cb = send_nwk_addr_req;
      }
      break;

    case NWK_UNICAST_REQ_TO_DUT_CHILD:
      {
        ZB_IEEE_ADDR_COPY(s_payload_ieee, g_ieee_addr_the1);
        s_payload_nwk = zb_address_short_by_ieee(g_ieee_addr_dut);
        s_payload_req_type = 0x00;
        next_cb = send_nwk_addr_req;
      }
      break;

    case NWK_BRCAST_WITH_EXT_RESP_TO_DUT_CHILD:
      {
        ZB_IEEE_ADDR_COPY(s_payload_ieee, g_ieee_addr_the1);
        s_payload_nwk = 0xfffd;
        s_payload_req_type = 0x01;
        next_cb = send_nwk_addr_req;
      }
      break;

    case NWK_BRCAST_WITH_INV_REQ_TYPE:
      {
        ZB_IEEE_ADDR_COPY(s_payload_ieee, g_ieee_addr_dut);
        s_payload_nwk = 0xfffd;
        s_payload_req_type = 0x02;
        next_cb = send_nwk_addr_req;
      }
      break;

    case NWK_UNICAST_WITH_INV_REQ_TYPE:
      {
        ZB_IEEE_ADDR_COPY(s_payload_ieee, g_ieee_addr_dut);
        s_payload_nwk = zb_address_short_by_ieee(g_ieee_addr_dut);;
        s_payload_req_type = 0x02;
        next_cb = send_nwk_addr_req;
      }
      break;

    case NWK_UNICAST_WITH_UNKNOWN_IEEE:
      {
        ZB_IEEE_ADDR_COPY(s_payload_ieee, g_ieee_addr_not_in_network);
        s_payload_nwk = zb_address_short_by_ieee(g_ieee_addr_dut);
        s_payload_req_type = 0x00;
        next_cb = send_nwk_addr_req;
      }
      break;

    case NWK_BRCAST_WITH_UNKNOWN_IEEE:
      {
        ZB_IEEE_ADDR_COPY(s_payload_ieee, g_ieee_addr_not_in_network);
        s_payload_nwk = 0xfffd;
        s_payload_req_type = 0x00;
        next_cb = send_nwk_addr_req;
      }
      break;

    case IEEE_UNICAST_WITH_EXT_RESP1:
    case IEEE_UNICAST_WITH_EXT_RESP2:
      {
        s_payload_nwk = zb_address_short_by_ieee(g_ieee_addr_dut);
        s_payload_req_type = 0x01;
        next_cb = send_ieee_addr_req;
      }
      break;

    case IEEE_UNICAST_WITH_SINGLE_RESP:
      {
        s_payload_nwk = zb_address_short_by_ieee(g_ieee_addr_dut);
        s_payload_req_type = 0x00;
        next_cb = send_ieee_addr_req;
      }
      break;

    case IEEE_UNICAST_REQ_TO_DUT_CHILD:
      {
        s_payload_nwk = zb_address_short_by_ieee(g_ieee_addr_the1);
        s_payload_req_type = 0x00;
        next_cb = send_ieee_addr_req;
      }
      break;

    case IEEE_BRCAST_WITH_INV_REQ_TYPE:
      {
        s_payload_nwk = zb_address_short_by_ieee(g_ieee_addr_dut);
        s_payload_req_type = 0x02;
        next_cb = send_ieee_addr_req;
      }
      break;

    case IEEE_UNICAST_WITH_UNKNOWN_NWK:
      {
        s_payload_nwk = zb_random();
        s_payload_req_type = 0x00;
        next_cb = send_ieee_addr_req;
      }
      break;

    default:
      stop_test = 1;
      TRACE_MSG(TRACE_ZDO1, "Unknown state, stop test", (FMT__0));
  }

  if (!stop_test)
  {
    zb_buf_get_out_delayed(next_cb);
  }

  TRACE_MSG(TRACE_ZDO1, ">>test_step_actions", (FMT__0));
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  switch (sig)
  {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      if (status == 0)
      {
        zb_ieee_addr_t trust_center_address;
        zb_address_ieee_ref_t dut_addr_ref;

        TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));

        zb_zdo_register_device_annce_cb(device_annce_cb);
        zb_aib_get_trust_center_address(trust_center_address);

        /* Look for DUT's address in the address table. If not found - the THr1 has formed the network and is awaiting for the DUT-ZED. */
        if (zb_address_by_ieee(g_ieee_addr_dut, ZB_FALSE, ZB_FALSE, &dut_addr_ref) == RET_OK)
        {
          ZB_SCHEDULE_ALARM(start_test, 0, THR1_START_TEST_DELAY);
        }

        if (ZB_IEEE_ADDR_CMP(trust_center_address, g_unknown_ieee_addr))
        {
          bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        }
        else
        {
          zb_buf_get_out_delayed(close_thr1);
        }
      }
      else
      {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, status));
      }
      break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    default:
      if (status == 0)
      {
        TRACE_MSG(TRACE_APS1, "Unknown signal, status OK", (FMT__0));
      }
      else
      {
        TRACE_MSG(TRACE_ERROR, "Unknown signal, status %d", (FMT__D, status));
      }
      break;
  }

  zb_buf_free(param);
}


/*! @} */
