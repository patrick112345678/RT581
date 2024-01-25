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
/* PURPOSE: TH-GPP/TH-TOOL
*/

#define ZB_TEST_NAME GPS_PROXIMITY_COMMISSIONING_GENERAL_TH_TOOL
#define ZB_TRACE_FILE_ID 41275

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_zcl.h"
#include "zb_secur_api.h"
#include "zb_ha.h"
#include "test_config.h"

#ifdef ZB_ENABLE_HA
#include "../include/zgp_test_templates.h"

/*============================================================================*/
/*                             DECLARATIONS                                   */
/*============================================================================*/

static zb_ieee_addr_t g_th_tool_addr = TH_TOOL_IEEE_ADDR;
static zb_uint8_t g_key_nwk[] = NWK_KEY;
static zb_uint16_t g_dut_addr;

static void dev_annce_cb(zb_zdo_device_annce_t *da);
static void gp_pairing_handler_cb(zb_uint8_t buf_ref);
static void gp_proxy_comm_mode_handler_cb(zb_uint8_t buf_ref);

/*============================================================================*/
/*                             FSM CORE                                       */
/*============================================================================*/

/*! Program states according to test scenario */
enum test_states_e
{
  TEST_STATE_INITIAL,
  /* Initial */
  TEST_STATE_WAIT_DUT_STARTUP,
  /* STEP 1 */
  TEST_STATE_WAIT_PAIRING_1,
  TEST_STATE_READ_OUT_DUT_SINK_TABLE1_1,
  TEST_STATE_MEDIUM_WINDOW_1,
  TEST_STATE_READ_OUT_DUT_SINK_TABLE2_1,
  /* STEP 2A */
  TEST_STATE_WAIT_COMM_MODE_ENTER_2A,
  TEST_STATE_MEDIUM_WINDOW_2A,
  TEST_STATE_READ_OUT_DUT_SINK_TABLE_2A,
  /* STEP 2B */
  TEST_STATE_WAIT_COMM_MODE_ENTER_2B,
  TEST_STATE_MEDIUM_WINDOW_2B,
  TEST_STATE_READ_OUT_DUT_SINK_TABLE_2B,
  /* STEP 2C */
  TEST_STATE_WAIT_COMM_MODE_ENTER_2C,
  TEST_STATE_MEDIUM_WINDOW_2C,
  TEST_STATE_READ_OUT_DUT_SINK_TABLE_2C,
  /* STEP 2D */
  TEST_STATE_WAIT_COMM_MODE_ENTER_2D,
  TEST_STATE_MEDIUM_WINDOW_2D,
  TEST_STATE_READ_OUT_DUT_SINK_TABLE_2D,
  /* STEP 2E */
  TEST_STATE_WAIT_COMM_MODE_ENTER_2E,
  TEST_STATE_MEDIUM_WINDOW_2E,
  TEST_STATE_READ_OUT_DUT_SINK_TABLE_2E,
  /* STEP 3A */
  TEST_STATE_WAIT_COMM_MODE_ENTER_3A,
  TEST_STATE_MEDIUM_WINDOW_3A,
  TEST_STATE_READ_OUT_DUT_SINK_TABLE_3A,
  /* STEP 3B */
  TEST_STATE_WAIT_COMM_MODE_ENTER_3B,
  TEST_STATE_MEDIUM_WINDOW_3B,
  TEST_STATE_READ_OUT_DUT_SINK_TABLE_3B,
  /* STEP 3C */
  TEST_STATE_WAIT_COMM_MODE_ENTER_3C,
  TEST_STATE_MEDIUM_WINDOW_3C,
  TEST_STATE_READ_OUT_DUT_SINK_TABLE_3C,
  /* STEP 3D */
  TEST_STATE_WAIT_COMM_MODE_ENTER_3D,
  TEST_STATE_MEDIUM_WINDOW_3D,
  TEST_STATE_READ_OUT_DUT_SINK_TABLE_3D,
  /* STEP 3E */
  TEST_STATE_WAIT_COMM_MODE_ENTER_3E,
  TEST_STATE_MEDIUM_WINDOW_3E,
  TEST_STATE_READ_OUT_DUT_SINK_TABLE_3E,
  /* STEP 4A */
  TEST_STATE_WAIT_COMM_MODE_ENTER_4A,
  TEST_STATE_MEDIUM_WINDOW_4A,
  TEST_STATE_READ_OUT_DUT_SINK_TABLE_4A,
  /* STEP 4B */
  TEST_STATE_WAIT_COMM_MODE_ENTER_4B,
  TEST_STATE_MEDIUM_WINDOW_4B,
  TEST_STATE_READ_OUT_DUT_SINK_TABLE_4B,
  /* STEP 5 */
  TEST_STATE_WAIT_PAIRING_5,
  TEST_STATE_MEDIUM_WINDOW_5,
  TEST_STATE_READ_OUT_DUT_SINK_TABLE_5,
  /* STEP 6 */
  TEST_STATE_WAIT_PAIRING_6,
  TEST_STATE_MEDIUM_WINDOW_6,
  TEST_STATE_READ_OUT_DUT_SINK_TABLE_6,
  /* STEP 7 */
  TEST_STATE_WAIT_PAIRING_7,
  TEST_STATE_SMALL_WINDOW_7,
  TEST_STATE_READ_OUT_DUT_SINK_TABLE1_7,
  TEST_STATE_MEDIUM_WINDOW_7,
  TEST_STATE_READ_OUT_DUT_SINK_TABLE2_7,
  /* FINISH */
  TEST_STATE_FINISHED
};

ZB_ZGPC_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 1000)

static void send_zcl(zb_uint8_t buf_ref, zb_callback_t cb)
{
  ZVUNUSED(cb);

  TRACE_MSG(TRACE_APP1, ">send_zcl: test_state = %d", (FMT__D, TEST_DEVICE_CTX.test_state));

  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE1_1:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE2_1:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_2A:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_2B:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_2C:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_2D:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_2E:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_3A:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_3B:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_3C:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_3D:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_3E:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_4A:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_4B:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_5:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_6:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE1_7:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE2_7:
      zgp_cluster_read_attr(buf_ref, g_dut_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                            ZB_ZCL_ATTR_GPS_SINK_TABLE_ID,
                            ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
      break;
  }

  TRACE_MSG(TRACE_APP1, "<send_zcl", (FMT__0));
}

static void perform_next_state(zb_uint8_t param)
{
  ZVUNUSED(param);
  if (TEST_DEVICE_CTX.pause)
  {
    ZB_SCHEDULE_ALARM(perform_next_state, 0,
                      ZB_TIME_ONE_SECOND*TEST_DEVICE_CTX.pause);
    TEST_DEVICE_CTX.pause = 0;
    return;
  }

  TEST_DEVICE_CTX.test_state++;

  TRACE_MSG(TRACE_APP1, ">perform_next_state: test_state = %d",
            (FMT__D, TEST_DEVICE_CTX.test_state));

  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_MEDIUM_WINDOW_1:
    case TEST_STATE_MEDIUM_WINDOW_2A:
    case TEST_STATE_MEDIUM_WINDOW_2B:
    case TEST_STATE_MEDIUM_WINDOW_2C:
    case TEST_STATE_MEDIUM_WINDOW_2D:
    case TEST_STATE_MEDIUM_WINDOW_2E:
    case TEST_STATE_MEDIUM_WINDOW_3A:
    case TEST_STATE_MEDIUM_WINDOW_3B:
    case TEST_STATE_MEDIUM_WINDOW_3C:
    case TEST_STATE_MEDIUM_WINDOW_3D:
    case TEST_STATE_MEDIUM_WINDOW_3E:
    case TEST_STATE_MEDIUM_WINDOW_4A:
    case TEST_STATE_MEDIUM_WINDOW_4B:
    case TEST_STATE_MEDIUM_WINDOW_5:
    case TEST_STATE_MEDIUM_WINDOW_6:
    case TEST_STATE_MEDIUM_WINDOW_7:
      ZB_ZGPC_SET_PAUSE(TH_TOOL_MEDIUM_WINDOW);
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      break;

    case TEST_STATE_SMALL_WINDOW_7:
      ZB_ZGPC_SET_PAUSE(TH_TOOL_SMALL_WINDOW);
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      break;

    case TEST_STATE_WAIT_DUT_STARTUP:
    case TEST_STATE_WAIT_PAIRING_1:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_2A:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_2B:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_2C:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_2D:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_2E:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_3A:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_3B:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_3C:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_3D:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_3E:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_4A:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_4B:
    case TEST_STATE_WAIT_PAIRING_5:
    case TEST_STATE_WAIT_PAIRING_6:
    case TEST_STATE_WAIT_PAIRING_7:
      /* Transition to next state should be performed in handlers */
      break;

    case TEST_STATE_FINISHED:
      TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
      break;
    default:
    {
      ZB_SCHEDULE_ALARM(test_send_command, 0, ZB_TIME_ONE_SECOND);
    }
  }

  TRACE_MSG(TRACE_APP1, "<perform_next_state", (FMT__0));
}

/*============================================================================*/
/*                            TEST IMPLEMENTATION                             */
/*============================================================================*/

static void dev_annce_cb(zb_zdo_device_annce_t *da)
{
  zb_ieee_addr_t dut_ieee = DUT_GPS_IEEE_ADDR;
  TRACE_MSG(TRACE_APP1, "dev_annce_cb: ieee = " TRACE_FORMAT_64 " NWK = 0x%x",
            (FMT__A_H, TRACE_ARG_64(da->ieee_addr), da->nwk_addr));

  if (ZB_IEEE_ADDR_CMP(dut_ieee, da->ieee_addr) == ZB_TRUE)
  {
    g_dut_addr = da->nwk_addr;
    if (TEST_DEVICE_CTX.test_state == TEST_STATE_WAIT_DUT_STARTUP)
    {
      PERFORM_NEXT_STATE(0);
    }
  }
}

static void gp_pairing_handler_cb(zb_uint8_t buf_ref)
{
  TRACE_MSG(TRACE_APP1, ">gp_pairing_handler_cb: buf_ref = %d, test_state = %d",
            (FMT__D_D, buf_ref, TEST_DEVICE_CTX.test_state));

  ZVUNUSED(buf_ref);
  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_WAIT_PAIRING_1:
    case TEST_STATE_WAIT_PAIRING_5:
    case TEST_STATE_WAIT_PAIRING_6:
    case TEST_STATE_WAIT_PAIRING_7:
      PERFORM_NEXT_STATE(0);
      break;
  }

  TRACE_MSG(TRACE_APP1, "<gp_pairing_handler_cb", (FMT__0));
}

static void gp_proxy_comm_mode_handler_cb(zb_uint8_t buf_ref)
{
  TRACE_MSG(TRACE_APP1, ">gp_proxy_comm_mode_handler_cb: buf_ref = %d, test_state = %d",
            (FMT__D_D, buf_ref, TEST_DEVICE_CTX.test_state));

  ZVUNUSED(buf_ref);
  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_WAIT_COMM_MODE_ENTER_2A:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_2B:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_2C:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_2D:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_2E:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_3A:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_3B:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_3C:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_3D:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_3E:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_4A:
    case TEST_STATE_WAIT_COMM_MODE_ENTER_4B:
      PERFORM_NEXT_STATE(0);
      break;
  }

  TRACE_MSG(TRACE_APP1, "<gp_proxy_comm_mode_handler_cb", (FMT__0));
}

/*============================================================================*/
/*                          STARTUP                                           */
/*============================================================================*/

static void zgpc_custom_startup()
{
/* Init device, load IB values from nvram or set it to default */
  ZB_INIT("th_tool");

  /* let's always be coordinator */
  ZB_AIB().aps_designated_coordinator = 1;
  ZB_AIB().aps_channel_mask = (1<<TEST_CHANNEL);
  zb_set_default_ffd_descriptor_values(ZB_COORDINATOR);
  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_th_tool_addr);
  ZB_PIBCACHE_PAN_ID() = TEST_PAN_ID;
  ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);
  zb_secur_setup_nwk_key(g_key_nwk, 0);
  ZB_NIB_SET_USE_MULTICAST(ZB_FALSE);
  /* Must use NVRAM for ZGP */
  ZB_AIB().aps_use_nvram = 1;

  /* Need to block GPDF recv directly */
#ifdef ZB_ZGP_SKIP_GPDF_ON_NWK_LAYER
  ZG->nwk.skip_gpdf = 1;
#endif
#ifdef ZB_ZGP_RUNTIME_WORK_MODE_WITH_PROXIES
  ZGP_CTX().enable_work_with_proxies = 1;
#endif
#ifdef ZB_CERTIFICATION_HACKS
  ZB_CERT_HACKS().ccm_check_cb = NULL;
#endif

  ZGP_GPS_SECURITY_LEVEL = ZB_ZGP_FILL_GPS_SECURITY_LEVEL(
                             ZB_ZGP_SEC_LEVEL_FULL_NO_ENC,
                             ZB_ZGP_DEFAULT_SEC_LEVEL_PROTECTION_WITH_GP_LINK_KEY,
                             ZB_ZGP_DEFAULT_SEC_LEVEL_INVOLVE_TC);

  ZGP_GP_SET_SHARED_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_NO_KEY);
  ZGP_CTX().device_role = ZGP_DEVICE_COMMISSIONING_TOOL;

  zb_zdo_register_device_annce_cb(dev_annce_cb);
  TEST_DEVICE_CTX.gp_pairing_hndlr_cb = gp_pairing_handler_cb;
  TEST_DEVICE_CTX.gp_prx_comm_mode_hndlr_cb = gp_proxy_comm_mode_handler_cb;
}

ZB_ZGPC_DECLARE_STARTUP_PROCESS()

#else // defined ZB_ENABLE_HA

#include <stdio.h>
MAIN()
{
  ARGV_UNUSED;

  printf("HA profile should be enabled in zb_config.h\n");

  MAIN_RETURN(1);
}

#endif
