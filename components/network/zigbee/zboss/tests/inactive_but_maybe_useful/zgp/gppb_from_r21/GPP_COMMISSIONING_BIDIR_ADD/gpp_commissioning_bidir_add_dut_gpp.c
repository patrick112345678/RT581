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
/* PURPOSE: DUT-GPP
*/

#define ZB_TEST_NAME GPP_COMMISSIONING_BIDIR_ADD_DUT_GPP
#define ZB_TRACE_FILE_ID 41460

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

static zb_ieee_addr_t g_dut_gpp_addr = DUT_GPP_IEEE_ADDR;

/*============================================================================*/
/*                             FSM CORE                                       */
/*============================================================================*/

/*! Program states according to test scenario */
enum test_states_e
{
  TEST_STATE_INITIAL,
  /* STEP 1A */
#ifndef TH_SKIP_STEPS_1_2
  TEST_STATE_CLEAN_PROXY_TABLE_1A,
  TEST_STATE_WINDOW_1A,
#endif
  /* STEP 3*/
#ifndef TH_SKIP_STEP_3
  TEST_STATE_CLEAN_PROXY_TABLE_3,
  TEST_STATE_WINDOW_3,
#endif
  /* STEP 4 */
#ifndef TH_SKIP_STEP_4
  TEST_STATE_CLEAN_PROXY_TABLE_4,
  TEST_STATE_WINDOW_4,
#endif
  /* STEP 5 */
#ifndef TH_SKIP_STEP_7
  TEST_STATE_CLEAN_PROXY_TABLE_5,
  TEST_STATE_WINDOW_5,
#endif
  /* STEP 6 - SKIPPED*/
  /* STEP 7A */
#ifndef TH_SKIP_STEP_7
  TEST_STATE_CLEAN_PROXY_TABLE_7A,
  TEST_STATE_WINDOW_7A,
#endif
  /* STEP 8A */
#ifndef TH_SKIP_STEP_8A
  TEST_STATE_CLEAN_PROXY_TABLE_8A,
  TEST_STATE_WINDOW_8A,
#endif
  /* STEP 8B */
#ifndef TH_SKIP_STEP_8B
  TEST_STATE_CLEAN_PROXY_TABLE_8B,
  TEST_STATE_WINDOW_8B,
#endif
  /* STEP 8C */
#ifndef TH_SKIP_STEP_8C
  TEST_STATE_CLEAN_PROXY_TABLE_8C,
  TEST_STATE_WINDOW_8C,
#endif
  /* STEP 9A */
  TEST_STATE_CLEAN_PROXY_TABLE_9A,
  TEST_STATE_WINDOW_9A,
  /* STEP 9B */
  TEST_STATE_CLEAN_PROXY_TABLE_9B,
  TEST_STATE_WINDOW_9B,
  /* FINISH */
  TEST_STATE_FINISHED
};

ZB_ZGPC_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 1000)

static void send_zcl(zb_uint8_t buf_ref, zb_callback_t cb)
{
  ZVUNUSED(buf_ref);
  ZVUNUSED(cb);
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

  TRACE_MSG(TRACE_APP1,
            "DUT-GPP: perform_next_state: test_state = %d",
            (FMT__D, TEST_DEVICE_CTX.test_state));

  switch (TEST_DEVICE_CTX.test_state)
  {
#ifndef TH_SKIP_STEPS_1_2
    case TEST_STATE_CLEAN_PROXY_TABLE_1A:
#endif
#ifndef TH_SKIP_STEP_3
    case TEST_STATE_CLEAN_PROXY_TABLE_3:
#endif
#ifndef TH_SKIP_STEP_4
    case TEST_STATE_CLEAN_PROXY_TABLE_4:
#endif
#ifndef TH_SKIP_STEP_5
    case TEST_STATE_CLEAN_PROXY_TABLE_5:
#endif
#ifndef TH_SKIP_STEP_7
    case TEST_STATE_CLEAN_PROXY_TABLE_7A:
#endif
#ifndef TH_SKIP_STEP_8A
    case TEST_STATE_CLEAN_PROXY_TABLE_8A:
#endif
#ifndef TH_SKIP_STEP_8B
    case TEST_STATE_CLEAN_PROXY_TABLE_8B:
#endif
#ifndef TH_SKIP_STEP_8C
    case TEST_STATE_CLEAN_PROXY_TABLE_8C:
#endif
    case TEST_STATE_CLEAN_PROXY_TABLE_9A:
    case TEST_STATE_CLEAN_PROXY_TABLE_9B:
      zgp_tbl_clear();
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      break;

#ifndef TH_SKIP_STEPS_1_2
    case TEST_STATE_WINDOW_1A:
      ZB_ZGPC_SET_PAUSE(DUT_GPP_WINDOW_1A);
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      break;
#endif
#ifndef TH_SKIP_STEP_3
    case TEST_STATE_WINDOW_3:
      ZB_ZGPC_SET_PAUSE(DUT_GPP_WINDOW_3);
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      break;
#endif
#ifndef TH_SKIP_STEP_4
    case TEST_STATE_WINDOW_4:
      ZB_ZGPC_SET_PAUSE(DUT_GPP_WINDOW_4);
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      break;
#endif
#ifndef TH_SKIP_STEP_5
    case TEST_STATE_WINDOW_5:
      ZB_ZGPC_SET_PAUSE(DUT_GPP_WINDOW_5);
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      break;
#endif
#ifndef TH_SKIP_STEP_7
    case TEST_STATE_WINDOW_7A:
      ZB_ZGPC_SET_PAUSE(DUT_GPP_WINDOW_7A);
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      break;
#endif
#ifndef TH_SKIP_STEP_8A
    case TEST_STATE_WINDOW_8A:
      ZB_ZGPC_SET_PAUSE(DUT_GPP_WINDOW_8A);
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      break;
#endif
#ifndef TH_SKIP_STEP_8B
    case TEST_STATE_WINDOW_8B:
      ZB_ZGPC_SET_PAUSE(DUT_GPP_WINDOW_8B);
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      break;
#endif
#ifndef TH_SKIP_STEP_8C
    case TEST_STATE_WINDOW_8C:
      ZB_ZGPC_SET_PAUSE(DUT_GPP_WINDOW_8C);
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      break;
#endif
    case TEST_STATE_WINDOW_9A:
      ZB_ZGPC_SET_PAUSE(DUT_GPP_WINDOW_9A);
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      break;
    case TEST_STATE_WINDOW_9B:
      ZB_ZGPC_SET_PAUSE(DUT_GPP_WINDOW_9B);
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      break;

    case TEST_STATE_FINISHED:
      TRACE_MSG(TRACE_APP1, "DUT-GPP: Test finished. Status: OK", (FMT__0));
      break;
    default:
    {
      ZB_SCHEDULE_ALARM(test_send_command, 0, ZB_TIME_ONE_SECOND);
    }
  }

}

/*============================================================================*/
/*                            TEST IMPLEMENTATION                             */
/*============================================================================*/

/*============================================================================*/
/*                            STARTUP                                         */
/*============================================================================*/

static void zgpc_custom_startup()
{
  /* Init device, load IB values from nvram or set it to default */
  ZB_SET_TRAF_DUMP_ON();
  ZB_INIT("dut_gpp");

  /* let's always be coordinator */
  ZB_AIB().aps_channel_mask = (1<<TEST_CHANNEL);
  zb_set_default_ffd_descriptor_values(ZB_ROUTER);
  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_dut_gpp_addr);
  /* Must use NVRAM for ZGP */
  ZB_AIB().aps_use_nvram = 1;

  /* Need to block GPDF recv directly */
#ifdef ZB_ZGP_SKIP_GPDF_ON_NWK_LAYER
  ZG->nwk.skip_gpdf = 0;
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

  ZGP_GP_SET_SHARED_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_NWK);
  ZGP_CTX().device_role = ZGP_DEVICE_PROXY_BASIC;
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
