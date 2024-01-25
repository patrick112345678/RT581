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
/* PURPOSE: DUT ZC
*/

#define ZB_TEST_NAME RTP_BDB_17_DUT_ZC
#define ZB_TRACE_FILE_ID 40455

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"

#include "rtp_bdb_17_common.h"
#include "../common/zb_reg_test_globals.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error define ZB_USE_NVRAM
#endif

#ifndef ZB_SECURITY_INSTALLCODES
#error Define ZB_SECURITY_INSTALLCODES
#endif

typedef enum test_step_num_e
{
  TEST_ZED_LEAVE_WITH_REJOIN,
  TEST_ZED_LEAVE_WITHOUT_REJOIN,
  TEST_ZR1_LEAVE_WITH_REJOIN,
  TEST_ZR1_LEAVE_WITHOUT_REJOIN,
  TEST_RESERVED
} test_step_num_t;

static zb_uint8_t g_nwk_key[16] = {0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static test_step_num_t g_test_step = TEST_ZED_LEAVE_WITH_REJOIN;
static zb_ieee_addr_t g_ieee_addr_dut_zc  = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
static zb_ieee_addr_t g_ieee_addr_th_zr1  = {0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
static zb_uint16_t g_short_addr_th_zr1 = ZB_NWK_BROADCAST_ALL_DEVICES;
static zb_ieee_addr_t g_ieee_addr_th_zed1 = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static zb_uint16_t g_short_addr_th_zed1 = ZB_NWK_BROADCAST_ALL_DEVICES;

static void trigger_steering(zb_uint8_t unused);
static void zb_trace_device_update_signal(zb_zdo_signal_device_update_params_t *params);
static void zb_trace_device_authorized_signal(zb_zdo_signal_device_authorized_params_t *params);
static void test_send_leave_delayed(zb_uint8_t unused);
static void test_send_leave(zb_uint8_t param);

MAIN()
{
  ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
  ZB_SET_TRACE_LEVEL(4);
  ARGV_UNUSED;

  ZB_INIT("zdo_dut_zc");

  zb_set_long_address(g_ieee_addr_dut_zc);
  zb_set_pan_id(0x1aaa);
  zb_secur_setup_nwk_key((zb_uint8_t*) g_nwk_key, 0);

  zb_reg_test_set_common_channel_settings();
  zb_set_network_coordinator_role((1l << TEST_CHANNEL));
  zb_set_nvram_erase_at_start(ZB_TRUE);

  zb_set_installcode_policy(ZB_TRUE);

  zb_set_max_children(1);
  zb_aib_tcpol_set_authenticate_always(ZB_TRUE);

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
  zb_zdo_app_signal_hdr_t *sg_p;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

  TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  switch (sig)
  {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    {
      TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));
      if (status == 0)
      {
        ZB_SCHEDULE_CALLBACK(trigger_steering, 0);

        zb_secur_ic_add(g_ieee_addr_th_zr1, ZB_IC_TYPE_128, g_ic1, NULL);
        zb_secur_ic_add(g_ieee_addr_th_zed1, ZB_IC_TYPE_128, g_ic1, NULL);
      }
    }
    break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
      TRACE_MSG(TRACE_APP1, "signal: ZB_ZDO_SIGNAL_DEVICE_ANNCE, status %d", (FMT__D, status));

      if (status == 0)
      {
        zb_zdo_signal_device_annce_params_t *params =
          ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_annce_params_t);

        if (ZB_IEEE_ADDR_CMP(params->ieee_addr, g_ieee_addr_th_zed1))
        {
          if (g_test_step == TEST_ZED_LEAVE_WITH_REJOIN)
          {
            test_step_register(test_send_leave_delayed, 0, RTP_BDB_17_STEP_TIME_ZC);
            test_step_register(test_send_leave_delayed, 0, RTP_BDB_17_STEP_TIME_ZC);
            test_step_register(test_send_leave_delayed, 0, RTP_BDB_17_STEP_TIME_ZC);
            test_step_register(test_send_leave_delayed, 0, RTP_BDB_17_STEP_TIME_ZC);

            test_control_start(TEST_MODE, RTP_BDB_17_STEP_1_DELAY_ZC);
          }

          g_short_addr_th_zed1 = params->device_short_addr;
        }

        if (ZB_IEEE_ADDR_CMP(params->ieee_addr, g_ieee_addr_th_zr1))
        {
          g_short_addr_th_zr1 = params->device_short_addr;
        }
      }
      break; /* ZB_ZDO_SIGNAL_DEVICE_ANNCE */

    case ZB_ZDO_SIGNAL_DEVICE_UPDATE:
    {
      TRACE_MSG(TRACE_APP1, "signal: ZB_ZDO_SIGNAL_DEVICE_UPDATE, status %d", (FMT__D, status));
      if (status == 0)
      {
        zb_zdo_signal_device_update_params_t *params =
          ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_update_params_t);
        zb_trace_device_update_signal(params);
      }
    }
    break; /* ZB_ZDO_SIGNAL_DEVICE_UPDATE */

    case ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED:
    {
      TRACE_MSG(TRACE_APP1, "signal: ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED, status %d", (FMT__D, status));
      if (status == 0)
      {
        zb_zdo_signal_device_authorized_params_t *params =
          ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_authorized_params_t);
        zb_trace_device_authorized_signal(params);
      }
    }
    break; /* ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED */

    default:
      TRACE_MSG(TRACE_APP1, "Unknown signal %hd, status %d", (FMT__H_D, sig, status));
      break;
  }

  zb_buf_free(param);
}

static void trigger_steering(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
}


static void zb_trace_device_update_signal(zb_zdo_signal_device_update_params_t *params)
{
  TRACE_MSG(TRACE_APP3, "ZB_ZDO_SIGNAL_DEVICE_UPDATE", (FMT__0));
  TRACE_MSG(TRACE_APP3, "long_addr: " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(params->long_addr)));

  if (ZB_IEEE_ADDR_CMP(params->long_addr, g_ieee_addr_th_zed1))
  {
    TRACE_MSG(TRACE_APP3, "short_addr: TH_ZED, 0x%x", (FMT__D, params->short_addr));
  }
  else if (ZB_IEEE_ADDR_CMP(params->long_addr, g_ieee_addr_th_zr1))
  {
    TRACE_MSG(TRACE_APP3, "short_addr: TH_ZR, 0x%x", (FMT__D, params->short_addr));
  }
  else
  {
    TRACE_MSG(TRACE_APP3, "short_addr: UNKNOWN, 0x%x", (FMT__D, params->short_addr));
  }

  switch (params->status)
  {
    case ZB_STD_SEQ_SECURED_REJOIN:
      TRACE_MSG(TRACE_APP3, "status: 0x%hx - SECURED_REJOIN", (FMT__H, params->status));
      break;
    case ZB_STD_SEQ_UNSECURED_JOIN:
      TRACE_MSG(TRACE_APP3, "status: 0x%hx - UNSECURED_JOIN", (FMT__H, params->status));
      break;
    case ZB_DEVICE_LEFT:
      TRACE_MSG(TRACE_APP3, "status: 0x%hx - DEVICE_LEFT", (FMT__H, params->status));
      break;
    case ZB_STD_SEQ_UNSECURED_REJOIN:
      TRACE_MSG(TRACE_APP3, "status: 0x%hx - UNSECURED_REJOIN", (FMT__H, params->status));
      break;
    case ZB_HIGH_SEQ_SECURED_REJOIN:
      TRACE_MSG(TRACE_APP3, "status: 0x%hx - SECURED_REJOIN", (FMT__H, params->status));
      break;
    case ZB_HIGH_SEQ_UNSECURED_JOIN:
      TRACE_MSG(TRACE_APP3, "status: 0x%hx - UNSECURED_JOIN", (FMT__H, params->status));
      break;
    case ZB_HIGH_SEQ_UNSECURED_REJOIN:
      TRACE_MSG(TRACE_APP3, "status: 0x%hx - UNSECURED_REJOIN", (FMT__H, params->status));
      break;
    default:
      TRACE_MSG(TRACE_ERROR, "status: 0x%hx - INVALID STATUS", (FMT__H, params->status));
      break;
  }
}

static void zb_trace_device_authorized_signal(zb_zdo_signal_device_authorized_params_t *params)
{
  TRACE_MSG(TRACE_APP3, "ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED", (FMT__0));
  TRACE_MSG(TRACE_APP3, "long_addr: " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(params->long_addr)));

  if (ZB_IEEE_ADDR_CMP(params->long_addr, g_ieee_addr_th_zed1))
  {
    TRACE_MSG(TRACE_APP3, "short_addr: TH_ZED, 0x%x", (FMT__D, params->short_addr));
  }
  else if (ZB_IEEE_ADDR_CMP(params->long_addr, g_ieee_addr_th_zr1))
  {
    TRACE_MSG(TRACE_APP3, "short_addr: TH_ZR, 0x%x", (FMT__D, params->short_addr));
  }
  else
  {
    TRACE_MSG(TRACE_APP3, "short_addr: UNKNOWN, 0x%x", (FMT__D, params->short_addr));
  }

  switch (params->authorization_type)
  {
    case ZB_ZDO_AUTHORIZATION_TYPE_LEGACY:
      TRACE_MSG(TRACE_APP3, "auth_type: 0x%hx - LEGACY DEVICE", (FMT__H, params->authorization_type));
      switch (params->authorization_status)
      {
        case ZB_ZDO_LEGACY_DEVICE_AUTHORIZATION_SUCCESS:
          TRACE_MSG(TRACE_APP3, "auth_status: 0x%hx - SUCCESS", (FMT__H, params->authorization_status));
          break;
        case ZB_ZDO_LEGACY_DEVICE_AUTHORIZATION_FAILED:
          TRACE_MSG(TRACE_APP3, "auth_status: 0x%hx - FAILED", (FMT__H, params->authorization_status));
          break;
        default:
          TRACE_MSG(TRACE_APP3, "auth_status: 0x%hx - INVALID VALUE", (FMT__H, params->authorization_status));
          break;
      }
      break;

    case ZB_ZDO_AUTHORIZATION_TYPE_R21_TCLK:
      TRACE_MSG(TRACE_APP3, "auth_type: 0x%hx - R21 TCLK", (FMT__H, params->authorization_type));
      switch (params->authorization_status)
      {
        case ZB_ZDO_TCLK_AUTHORIZATION_SUCCESS:
          TRACE_MSG(TRACE_APP3, "auth_status: 0x%hx - SUCCESS", (FMT__H, params->authorization_status));
          break;
        case ZB_ZDO_TCLK_AUTHORIZATION_TIMEOUT:
          TRACE_MSG(TRACE_APP3, "auth_status: 0x%hx - TIMEOUT", (FMT__H, params->authorization_status));
          break;
        case ZB_ZDO_TCLK_AUTHORIZATION_FAILED:
          TRACE_MSG(TRACE_APP3, "auth_status: 0x%hx - FAILED", (FMT__H, params->authorization_status));
          break;
        default:
          TRACE_MSG(TRACE_APP3, "auth_status: 0x%hx - INVALID VALUE", (FMT__H, params->authorization_status));
          break;
      }
      break;

    default:
      TRACE_MSG(TRACE_APP3, "auth_type: 0x%hx - INVALID VALUE", (FMT__H, params->authorization_type));
      break;
  }
}

static void test_send_leave_delayed(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  zb_buf_get_out_delayed(test_send_leave);
}

static void test_send_leave(zb_uint8_t param)
{
  zb_zdo_mgmt_leave_param_t *req = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_leave_param_t);
  zb_bool_t continue_test = ZB_TRUE;

  TRACE_MSG(TRACE_APP1, ">>test_send_leave: buf_param = %d", (FMT__D, param));

  switch (g_test_step)
  {
    case TEST_ZED_LEAVE_WITH_REJOIN:
      req->dst_addr = g_short_addr_th_zed1;
      req->rejoin = 1;
      break;

    case TEST_ZED_LEAVE_WITHOUT_REJOIN:
      req->dst_addr = g_short_addr_th_zed1;
      req->rejoin = 0;
      break;

    case TEST_ZR1_LEAVE_WITH_REJOIN:
      req->dst_addr = g_short_addr_th_zr1;
      req->rejoin = 1;
      break;

    case TEST_ZR1_LEAVE_WITHOUT_REJOIN:
      req->dst_addr = g_short_addr_th_zr1;
      req->rejoin = 0;
      break;

    default:
      TRACE_MSG(TRACE_APP1, "test_send_leave: test fiished, g_test_step %d", (FMT__D, g_test_step));
      continue_test = ZB_FALSE;
      break;
  }

  if (continue_test)
  {
    g_test_step++;

    ZB_IEEE_ADDR_ZERO(req->device_address);
    req->remove_children = 0;
    zdo_mgmt_leave_req(param, NULL);
  }

  TRACE_MSG(TRACE_APP1, "<<test_send_leave", (FMT__0));
}

/*! @} */
