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
/* PURPOSE: DUT ZED
*/

#define ZB_TEST_NAME RTP_BDB_20_DUT_ZED
#define ZB_TRACE_FILE_ID 63996

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"

#include "rtp_bdb_20_common.h"
#include "../common/zb_reg_test_globals.h"


/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_ED_ROLE
#error ED role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error ZB_USE_NVRAM is not compiled!
#endif

typedef enum test_step_e
{
  TEST_STEP_0_INITIALIZATION = 0,
  TEST_STEP_1_STEERING_AFTER_SKIPPED_STARTUP_CONTINUE,
  TEST_STEP_2_STEERING_AFTER_SUCCESSFUL_STEERING,
  TEST_STEP_3_RESET_VIA_LOCAL_ACTION,
  TEST_STEP_4_STEERING_AFTER_RESET_VIA_LOCAL_ACTION,
  TEST_STEP_5_LEAVE_WITH_REJOIN,
  TEST_STEP_6_POWER_REBOOT_WITH_FOUND_NETWORK,
  TEST_STEP_7_POWER_REBOOT_WITH_LOST_NETWORK,
  TEST_STEP_8_REJOIN_BACKOFF,
  TEST_STEP_9_FINISH
} test_step_t;

typedef ZB_PACKED_PRE struct test_device_nvram_dataset_s
{
  zb_uint8_t test_step;
} ZB_PACKED_STRUCT test_device_nvram_dataset_t;

static zb_uint16_t test_get_nvram_data_size();
static zb_ret_t test_nvram_write_app_data(zb_uint8_t page, zb_uint32_t pos);
static void test_nvram_read_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length);
static void trigger_nvram_write_app_dataset(zb_uint8_t unused);

static void test_trigger_next_step(test_step_t new_test_step);
static void trigger_steering(zb_uint8_t steering_step);

static void local_leave_with_rejoin_cb(zb_uint8_t param);
static void trigger_local_leave_with_rejoin(zb_uint8_t param);

static zb_ieee_addr_t g_ieee_addr_dut_zed = IEEE_ADDR_DUT_ZED;
static test_step_t g_test_step = TEST_STEP_0_INITIALIZATION;

MAIN()
{
  ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP /*| TRACE_SUBSYSTEM_ZDO*/);
  ZB_SET_TRACE_LEVEL(4);

  ARGV_UNUSED;

  ZB_INIT("zdo_dut_zed");

  zb_set_long_address(g_ieee_addr_dut_zed);

  zb_reg_test_set_common_channel_settings();
  zb_set_network_ed_role((1l << TEST_CHANNEL));

  zb_set_nvram_erase_at_start(ZB_FALSE);

  zb_nvram_register_app1_write_cb(test_nvram_write_app_data, test_get_nvram_data_size);
  zb_nvram_register_app1_read_cb(test_nvram_read_app_data);

  zb_set_rx_on_when_idle(ZB_FALSE);

  if (zboss_start_no_autostart() != RET_OK)
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
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

  switch (sig)
  {
    /*
     * ZED starts without automatic initialization and does not try to find a network to join,
     * so it is a Factory New Device on first Power Boot and not factory new after Power Reboots,
     * because it stores information about active network in NVRAM
     */
    case ZB_ZDO_SIGNAL_SKIP_STARTUP:
      TRACE_MSG(TRACE_APP1, "signal: ZB_ZDO_SIGNAL_SKIP_STARTUP, status %hd, factory_new %hd",
        (FMT__H_H, status, zb_bdb_is_factory_new()));

      TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));
      ZB_ASSERT(status == RET_OK);

      switch (g_test_step)
      {
        /*
         * The ZED is started with clear NVRAM and without auto-startup procedure,
         * so check that it is Factory New and call the startup procedure
         */
        case TEST_STEP_0_INITIALIZATION:
          ZB_ASSERT(status == RET_OK);
          ZB_ASSERT(zb_bdb_is_factory_new());

          zboss_start_continue();

          break;

        /*
         * The ZED is started after Power Reboot, but without auto-startup procedure,
         * so check that it is Not Factory New and call the startup procedure
         */
        case TEST_STEP_6_POWER_REBOOT_WITH_FOUND_NETWORK:
        case TEST_STEP_7_POWER_REBOOT_WITH_LOST_NETWORK:
          ZB_ASSERT(status == RET_OK);
          ZB_ASSERT(!zb_bdb_is_factory_new());

          zboss_start_continue();

          break;

        default:
          ZB_ASSERT(ZB_FALSE);
          break;
      }

      break; /* ZB_ZDO_SIGNAL_SKIP_STARTUP */

    /*
     * Assume that ZED didn't find any networks to join during initialization,
     * so it continues to be a Factory New Device
     */
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      TRACE_MSG(TRACE_APP1, "signal: ZB_BDB_SIGNAL_DEVICE_FIRST_START, status %hd, factory_new %hd",
        (FMT__H_H, status, zb_bdb_is_factory_new()));

      ZB_ASSERT(g_test_step == TEST_STEP_0_INITIALIZATION);

      ZB_ASSERT(status == (zb_uint8_t)RET_ERROR);
      ZB_ASSERT(zb_bdb_is_factory_new());

      /* Assume that the cordinator is already started and trigger BDB Steering Commissioning */
      test_trigger_next_step(TEST_STEP_1_STEERING_AFTER_SKIPPED_STARTUP_CONTINUE);

      break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_DEVICE_REBOOT:
      TRACE_MSG(TRACE_APP1, "signal: ZB_BDB_SIGNAL_DEVICE_REBOOT, status %hd, factory_new %hd",
        (FMT__H_H, status, zb_bdb_is_factory_new()));

        switch (g_test_step)
        {
          /*
           * The ZED has left network with rejoin, so check that rejoin was successful and
           * that the device continues to be Not Factory New and then switch to waiting Power Reboot
           * without loosing the network
           */
          case TEST_STEP_5_LEAVE_WITH_REJOIN:
            ZB_ASSERT(status == RET_OK);
            ZB_ASSERT(!zb_bdb_is_factory_new());

            test_trigger_next_step(TEST_STEP_6_POWER_REBOOT_WITH_FOUND_NETWORK);

            break;

          /*
           * The ZED has been power rebooted, assume that network is found, so check that the device continues
           * to be Not Factory New, then switch to waiting Power Reboot with loosing the network
           */
          case TEST_STEP_6_POWER_REBOOT_WITH_FOUND_NETWORK:
            ZB_ASSERT(status == RET_OK);
            ZB_ASSERT(!zb_bdb_is_factory_new());

            test_trigger_next_step(TEST_STEP_7_POWER_REBOOT_WITH_LOST_NETWORK);

            break;

          /*
           * The ZED has been power rebooted, assume that network was not found, so check that the device continues
           * to be Not Factory New, then start rejoin back-off process and assume that the network will be found
           */
          case TEST_STEP_7_POWER_REBOOT_WITH_LOST_NETWORK:
            ZB_ASSERT(status == (zb_uint8_t)RET_ERROR);
            ZB_ASSERT(!zb_bdb_is_factory_new());

            test_trigger_next_step(TEST_STEP_8_REJOIN_BACKOFF);

            break;

          /*
           * The ZED has started Rejoin Back-Off process and one rejoin attempt was finished,
           * so check it's status and move to next back-off iteration or finish the test
           */
          case TEST_STEP_8_REJOIN_BACKOFF:
            if (status == RET_OK)
            {
              /*
               * Rejoin Back-Off is finished, the lost network is found, so check that
               * the device is Not Factory New and finish the test
               */
              ZB_ASSERT(!zb_bdb_is_factory_new());

              test_trigger_next_step(TEST_STEP_9_FINISH);
            }
            else
            {
              /*
               * Rejoin Back-Off iteration failed, the lost network is not found,
               * so check that the device is still Not Factory New and start new back-off iteration
               */
              ZB_ASSERT(!zb_bdb_is_factory_new());

              test_trigger_next_step(TEST_STEP_8_REJOIN_BACKOFF);
            }

            break;

          default:
            ZB_ASSERT(ZB_FALSE);
            break;
        }
      break; /* ZB_BDB_SIGNAL_DEVICE_REBOOT */

    case ZB_BDB_SIGNAL_STEERING:
      TRACE_MSG(TRACE_APP1, "signal: ZB_BDB_SIGNAL_STEERING, status %hd, factory_new %hd",
        (FMT__H_H, status, zb_bdb_is_factory_new()));

      switch (g_test_step)
      {
        /*
        * The ZED has joined to network, so it became a Not Factory New Device.
        * Try to trigget steering one more time and check result on the next step
        */
        case TEST_STEP_1_STEERING_AFTER_SKIPPED_STARTUP_CONTINUE:
          ZB_ASSERT(status == RET_OK);
          ZB_ASSERT(!zb_bdb_is_factory_new());

          test_trigger_next_step(TEST_STEP_2_STEERING_AFTER_SUCCESSFUL_STEERING);

          break;

        /*
        * The ZED tried to perform steering after joining,
        * check that it continues to be a Not Factory New Device and trigger full reset
        */
        case TEST_STEP_2_STEERING_AFTER_SUCCESSFUL_STEERING:
          ZB_ASSERT(status == RET_OK);
          ZB_ASSERT(!zb_bdb_is_factory_new());

          test_trigger_next_step(TEST_STEP_3_RESET_VIA_LOCAL_ACTION);

          break;

        /*
        * The ZED tried to perform steering after Reset Via Local Action,
        * check that it became a Not Factory New Device again and then try to perform Leave With Rejoin
        */
        case TEST_STEP_4_STEERING_AFTER_RESET_VIA_LOCAL_ACTION:
          ZB_ASSERT(status == RET_OK);
          ZB_ASSERT(!zb_bdb_is_factory_new());

          test_trigger_next_step(TEST_STEP_5_LEAVE_WITH_REJOIN);

          break;

        default:
          ZB_ASSERT(ZB_FALSE);
          break;
      }

      break; /* ZB_BDB_SIGNAL_STEERING */

    case ZB_ZDO_SIGNAL_LEAVE:
      {
        zb_zdo_signal_leave_params_t *leave_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_leave_params_t);

        TRACE_MSG(TRACE_APP1, "signal: ZB_ZDO_SIGNAL_LEAVE, status %hd, factory_new %hd",
          (FMT__H_H, status, zb_bdb_is_factory_new()));

        ZB_ASSERT(status == RET_OK);

        switch (g_test_step)
        {
          /*
           * The ZED was resetted via local action, so check that it became Factory New Device and
           * trigger steering to find a network again
           */
          case TEST_STEP_3_RESET_VIA_LOCAL_ACTION:
            ZB_ASSERT(leave_params->leave_type == ZB_NWK_LEAVE_TYPE_RESET);
            ZB_ASSERT(zb_bdb_is_factory_new());

            test_trigger_next_step(TEST_STEP_4_STEERING_AFTER_RESET_VIA_LOCAL_ACTION);
            break;

          case TEST_STEP_5_LEAVE_WITH_REJOIN:
            ZB_ASSERT(leave_params->leave_type == ZB_NWK_LEAVE_TYPE_REJOIN);
            ZB_ASSERT(!zb_bdb_is_factory_new());

            break;

          default:
            ZB_ASSERT(ZB_FALSE);
            break;
        }
      }

      break; /* ZB_ZDO_SIGNAL_LEAVE */

    case ZB_COMMON_SIGNAL_CAN_SLEEP:
      if (status == 0)
      {
        zb_sleep_now();
      }
      break; /* ZB_COMMON_SIGNAL_CAN_SLEEP */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal %hd, status %d", (FMT__H_D, sig, status));
      break;
  }

  zb_buf_free(param);
}


static void test_trigger_next_step(test_step_t new_test_step)
{
  TRACE_MSG(TRACE_APP1, ">> test_trigger_next_step %hd", (FMT__H, new_test_step));

  g_test_step = new_test_step;

  switch (new_test_step)
  {
    /*
     * The ZED is started. In assumption that it didn't find network to join
     * during auto-start process, trigger steering manually and assume that it will find network now.
     */
    case TEST_STEP_1_STEERING_AFTER_SKIPPED_STARTUP_CONTINUE:
      ZB_SCHEDULE_APP_ALARM(trigger_steering, 0, RTP_BDB_20_ZED_TEST_STEP_1_DELAY);
      break;

    /*
     * The manual steering is completed successfully. Trigger steering one more time
     * to confirm that Factory New status will not be changed.
     */
    case TEST_STEP_2_STEERING_AFTER_SUCCESSFUL_STEERING:
      ZB_SCHEDULE_APP_ALARM(trigger_steering, 0, RTP_BDB_20_ZED_TEST_STEP_2_DELAY);
      break;

    /*
     * The second manual steering is completed successfully. Trigger BDB Reset Via Local action now to confirm
     * that ZED will become Factory New.
     */
    case TEST_STEP_3_RESET_VIA_LOCAL_ACTION:
      ZB_SCHEDULE_APP_ALARM(zb_bdb_reset_via_local_action, 0, RTP_BDB_20_ZED_TEST_STEP_3_DELAY);
      break;

    /*
     * The Reset Via Local Action process is completed successfully.
     * Trigger steering, find network again it check that ZED will become Not Factory New.
     */
    case TEST_STEP_4_STEERING_AFTER_RESET_VIA_LOCAL_ACTION:
      ZB_SCHEDULE_APP_ALARM(trigger_steering, 0, RTP_BDB_20_ZED_TEST_STEP_4_DELAY);
      break;

    /*
     * The steering after Reset Via Local Action is completed successfully,
     * now try to perform Leave With Rejoin and check that ZED continued to be Not Factory New during
     * rejoin process and after it.
     */
    case TEST_STEP_5_LEAVE_WITH_REJOIN:
      ZB_SCHEDULE_APP_ALARM(trigger_local_leave_with_rejoin, 0, RTP_BDB_20_ZED_TEST_STEP_5_DELAY);
      break;

    /*
     * The device has left the network with rejoin, now just switch test step,
     * save test state to NVRAM and wait for the Power Reboot without loosing the network.
     * Then check that the device continued to be Not Factory New.
     */
    case TEST_STEP_6_POWER_REBOOT_WITH_FOUND_NETWORK:
      ZB_SCHEDULE_APP_CALLBACK(trigger_nvram_write_app_dataset, 0);

      break;

    /*
     * The device has rebooted without loosing the network, now just switch test step,
     * save test state to NVRAM and wait for the Power Reboot with loosing the network.
     * Then check that the device continued to be Not Factory New.
     */
    case TEST_STEP_7_POWER_REBOOT_WITH_LOST_NETWORK:
      ZB_SCHEDULE_APP_CALLBACK(trigger_nvram_write_app_dataset, 0);

      break;

    /*
     * The device has rebooted with loosing network, so start Rejoin Back-Off procedure or
     * test a new iteration after the first one failed and assume that the device will find the lost network.
     */
    case TEST_STEP_8_REJOIN_BACKOFF:
      if (zb_zdo_rejoin_backoff_is_running())
      {
#ifndef NCP_MODE_HOST
          ZB_SCHEDULE_APP_CALLBACK(zb_zdo_rejoin_backoff_continue, 0);
#else
          ZB_ASSERT(ZB_FALSE && "Rejoin back-off is not available on NCP devices");
#endif
      }
      else
      {
        zb_ret_t rejoin_start_status = zb_zdo_rejoin_backoff_start(ZB_FALSE);
        ZB_ASSERT(rejoin_start_status == RET_OK);
      }
      break;

    /*
     * The test has been successfully finished, so print success message.
     */
    case TEST_STEP_9_FINISH:
      TRACE_MSG(TRACE_APP1, "Test successfully finished", (FMT__0));
      break;

    default:
      ZB_ASSERT(0);
      break;
  }

  TRACE_MSG(TRACE_APP1, "<< test_trigger_next_step", (FMT__0));
}


static zb_uint16_t test_get_nvram_data_size()
{
  zb_uint16_t size;

  TRACE_MSG(TRACE_APP1, ">> test_get_nvram_data_size", (FMT__0));

  size = sizeof(test_device_nvram_dataset_t);

  TRACE_MSG(TRACE_APP1, "<< test_get_nvram_data_size, size %d", (FMT__D, size));

  return size;
}


static void test_nvram_read_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length)
{
  test_device_nvram_dataset_t ds;
  zb_ret_t ret;

  TRACE_MSG(TRACE_APP1, ">> test_nvram_read_app_data, page %hd, pos %d", (FMT__H_D, page, pos));

  ZB_ASSERT(payload_length == sizeof(ds));

  ret = zb_nvram_read_data(page, pos, (zb_uint8_t*)&ds, sizeof(ds));

  if (ret == RET_OK)
  {
    g_test_step = ds.test_step;

    TRACE_MSG(TRACE_APP1, "test_step: %hd", (FMT__H, ds.test_step));
  }

  TRACE_MSG(TRACE_APP1, "<< test_nvram_read_app_data ret %d", (FMT__D, ret));
}


static zb_ret_t test_nvram_write_app_data(zb_uint8_t page, zb_uint32_t pos)
{
  zb_ret_t ret;
  test_device_nvram_dataset_t ds;

  TRACE_MSG(TRACE_APP1, ">> test_nvram_write_app_data, page %hd, pos %d", (FMT__H_D, page, pos));

  ds.test_step = g_test_step;

  ret = zb_nvram_write_data(page, pos, (zb_uint8_t*)&ds, sizeof(ds));

  TRACE_MSG(TRACE_APP1, "<< test_nvram_write_app_data, ret %d", (FMT__D, ret));

  return ret;
}


static void trigger_nvram_write_app_dataset(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  TRACE_MSG(TRACE_APP1, ">> trigger_nvram_write_app_dataset", (FMT__0));

  zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);

  TRACE_MSG(TRACE_APP1, "<< trigger_nvram_write_app_dataset", (FMT__0));
}


static void trigger_steering(zb_uint8_t unused)
{
  zb_bool_t commissioning_start_status = ZB_FALSE;

  ZVUNUSED(unused);

  TRACE_MSG(TRACE_APP1, ">> trigger_steering", (FMT__0));

  commissioning_start_status = bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
  ZB_ASSERT(commissioning_start_status);

  TRACE_MSG(TRACE_APP1, "<< trigger_steering", (FMT__0));
}


static void local_leave_with_rejoin_cb(zb_uint8_t param)
{
  zb_zdo_mgmt_leave_res_t *resp = (zb_zdo_mgmt_leave_res_t *)zb_buf_begin(param);

  TRACE_MSG(TRACE_APP1, ">> local_leave_with_rejoin_cb, status %hd", (FMT__H, resp->status));

  ZB_ASSERT(resp->status == RET_OK);

  zb_buf_free(param);

  TRACE_MSG(TRACE_APP1, "<< trigger_local_leave_with_rejoin", (FMT__0));
}

static void trigger_local_leave_with_rejoin(zb_uint8_t param)
{
  zb_zdo_mgmt_leave_param_t *req_param;

  if (param == 0)
  {
    zb_buf_get_out_delayed(trigger_local_leave_with_rejoin);
    return;
  }

  TRACE_MSG(TRACE_APP1, ">> trigger_local_leave_with_rejoin", (FMT__0));

  req_param = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_leave_param_t);
  ZB_BZERO(req_param, sizeof(zb_zdo_mgmt_leave_param_t));

  req_param->dst_addr = ZB_PIBCACHE_NETWORK_ADDRESS();
  req_param->rejoin = ZB_TRUE;

  zdo_mgmt_leave_req(param, local_leave_with_rejoin_cb);

  TRACE_MSG(TRACE_APP1, "<< trigger_local_leave_with_rejoin", (FMT__0));
}


/*! @} */
