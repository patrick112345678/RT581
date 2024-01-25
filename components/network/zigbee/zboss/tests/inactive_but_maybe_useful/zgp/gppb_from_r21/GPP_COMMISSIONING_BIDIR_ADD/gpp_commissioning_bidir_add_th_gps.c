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
/* PURPOSE: TH DUT
*/

#define ZB_TEST_NAME GPP_COMMISSIONING_BIDIR_ADD_TH_GPS
#define ZB_TRACE_FILE_ID 41459

#include "../include/zgp_test_templates.h"
#include "test_config.h"

/*============================================================================*/
/*                             DECLARATIONS                                   */
/*============================================================================*/

#define ENDPOINT 10

static zb_ieee_addr_t g_th_gps_addr = TH_GPS_IEEE_ADDR;
static zb_uint16_t g_dut_addr;
static zb_uint8_t g_key_nwk[] = NWK_KEY;

static void dev_annce_cb(zb_zdo_device_annce_t *da);
static zb_bool_t gpd_comm_cb(zb_zgpd_id_t *zgpd_id, zb_zgp_comm_status_t result);
static void enter_comm_mode();

static void send_channel_config(zb_uint8_t param);
static void send_comm_reply(zb_uint8_t param);

/*============================================================================*/
/*                             FSM CORE                                       */
/*============================================================================*/
/*! Program states according to test scenario */
enum test_states_e
{
  TEST_STATE_INITIAL,
  /* Initial */
  TEST_STATE_WAIT_DUT_STARTUP,
  /* STEPS 1 and 2 */
#ifndef TH_SKIP_STEPS_1_2
  TEST_STATE_COMM_MODE_ENTER_1A,
  TEST_STATE_SEND_CHANNEL_CONFIG_RESP_1A,
  TEST_STATE_WAIT_PAIRING_2D,
  TEST_STATE_READ_OUT_DUT_PROXY_TABLE_2D,
#endif
  /* STEP 3 */
#ifndef TH_SKIP_STEP_3
  TEST_STATE_COMM_MODE_ENTER_3,
#endif
  /* STEP 4 */
#ifndef TH_SKIP_STEP_4
  TEST_STATE_COMM_MODE_ENTER_4,
#endif
  /* STEP 5 */
#ifndef TH_SKIP_STEP_4
  TEST_STATE_COMM_MODE_ENTER_5,
#endif
  /* STEP 7A */
#ifndef TH_SKIP_STEP_7
  TEST_STATE_COMM_MODE_ENTER_7A,
  TEST_STATE_SEND_CHANNEL_CONFIG_RESP1_7A,
  TEST_STATE_SEND_CHANNEL_CONFIG_RESP2_7A,
  /* STEP 7B */
  TEST_STATE_SEND_CHANNEL_CONFIG_RESP1_7B,
  TEST_STATE_SEND_CHANNEL_CONFIG_RESP2_7B,
  /* STEP 7C */
  TEST_STATE_SEND_COMM_REPLY_RESP1_7C,
  TEST_STATE_SEND_COMM_REPLY_RESP2_7C,
#endif
  /* STEP 8A */
#ifndef TH_SKIP_STEP_8A
  TEST_STATE_COMM_MODE_ENTER_8A,
  TEST_STATE_SEND_CHANNEL_CONFIG_RESP_8A,
#endif
  /* STEP 8B */
#ifndef TH_SKIP_STEP_8B
  TEST_STATE_COMM_MODE_ENTER_8B,
  TEST_STATE_SEND_CHANNEL_CONFIG_RESP_8B,
#endif
  /* STEP 8C */
#ifndef TH_SKIP_STEP_8C
  TEST_STATE_COMM_MODE_ENTER_8C,
  TEST_STATE_SEND_CHANNEL_CONFIG_RESP_8C,
#endif
  /* STEP 9A */
#ifndef TH_SKIP_STEP_9A
  TEST_STATE_COMM_MODE_ENTER_9A,
  TEST_STATE_SEND_COMM_REPLY_RESP_9A,
#endif
  /* STEP 9B */
  TEST_STATE_COMM_MODE_ENTER_9B,
  TEST_STATE_SEND_COMM_REPLY_RESP_9B,
  /* FINISH */
  TEST_STATE_FINISHED
};

ZB_ZGPC_DECLARE_ZCL_ON_OFF_TOGGLE_TEST_TEMPLATE(TEST_DEVICE_CTX, ENDPOINT, 4000)

static void send_zcl(zb_uint8_t buf_ref, zb_callback_t cb)
{
#ifndef TH_SKIP_STEP_3
  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_READ_OUT_DUT_PROXY_TABLE_2D:
      zgp_cluster_read_attr(buf_ref, g_dut_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                            ZB_ZCL_ATTR_GPP_PROXY_TABLE_ID,
                            ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
      ZB_ZGPC_SET_PAUSE(TH_GPS_PAUSE_BEFORE_STEP_3);
      break;
  }
#endif
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

  TRACE_MSG(TRACE_APP1,
            "TH-GPS: perform_next_state: state = %d",
            (FMT__D, TEST_DEVICE_CTX.test_state));

  switch (TEST_DEVICE_CTX.test_state)
  {
#ifndef TH_SKIP_STEPS_1_2
    case TEST_STATE_WAIT_PAIRING_2D:
      /* Just wait signal from gp_pairing_cb */
      ZB_ZGPC_SET_PAUSE(4);
      break;

    case TEST_STATE_COMM_MODE_ENTER_1A:
      {
        enter_comm_mode();
        ZB_ZGPC_SET_PAUSE(2);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      }
      break;
    case TEST_STATE_SEND_CHANNEL_CONFIG_RESP_1A:
      {
        ZB_GET_OUT_BUF_DELAYED(send_channel_config);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
      }
#endif
#ifndef TH_SKIP_STEP_3
    case TEST_STATE_COMM_MODE_ENTER_3:
      {
        ZB_ZGPC_SET_PAUSE(TH_GPS_PAUSE_BEFORE_STEP_4);
        ZGP_GPS_COMMISSIONING_EXIT_MODE = ZGP_COMMISSIONING_EXIT_MODE_ON_COMMISSIONING_WINDOW_EXPIRATION;
        ZGP_GPS_COMMISSIONING_WINDOW = TH_GPS_COMM_WINDOW_3;
        enter_comm_mode();
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      }
      break;
#endif
#ifndef TH_SKIP_STEP_4
    case TEST_STATE_COMM_MODE_ENTER_4:
      {
        ZB_ZGPC_SET_PAUSE(TH_GPS_PAUSE_BEFORE_STEP_5);
        ZGP_GPS_COMMISSIONING_WINDOW = TH_GPS_COMM_WINDOW_4;
        enter_comm_mode();
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      }
      break;
#endif
#ifndef TH_SKIP_STEP_5
    case TEST_STATE_COMM_MODE_ENTER_5:
      {
        ZB_ZGPC_SET_PAUSE(TH_GPS_PAUSE_BEFORE_STEP_7A);
        ZGP_GPS_COMMISSIONING_WINDOW = TH_GPS_COMM_WINDOW_5;
        enter_comm_mode();
        ZB_ZGPC_SET_PAUSE(10);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      }
      break;
#endif

#ifndef TH_SKIP_STEP_7
    case TEST_STATE_COMM_MODE_ENTER_7A:
      {
        ZB_ZGPC_SET_PAUSE(TH_GPS_PAUSE_BEFORE_CH_CFG1_7A);
        ZGP_GPS_COMMISSIONING_WINDOW = TH_GPS_COMM_WINDOW_7A;
        enter_comm_mode();
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      }
      break;
    case TEST_STATE_SEND_CHANNEL_CONFIG_RESP1_7A:
      {
        ZB_ZGPC_SET_PAUSE(TH_GPS_PAUSE_BEFORE_CH_CFG2_7A);
        ZB_GET_OUT_BUF_DELAYED(send_channel_config);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
      }
    case TEST_STATE_SEND_CHANNEL_CONFIG_RESP2_7A:
      {
        ZB_ZGPC_SET_PAUSE(TH_GPS_PAUSE_BEFORE_CH_CFG1_7B);
        ZB_GET_OUT_BUF_DELAYED(send_channel_config);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
      }

    case TEST_STATE_SEND_CHANNEL_CONFIG_RESP1_7B:
      {
        ZB_ZGPC_SET_PAUSE(TH_GPS_PAUSE_BEFORE_CH_CFG2_7B);
        ZB_GET_OUT_BUF_DELAYED(send_channel_config);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
      }
    case TEST_STATE_SEND_CHANNEL_CONFIG_RESP2_7B:
      {
        ZB_ZGPC_SET_PAUSE(TH_GPS_PAUSE_BEFORE_COMM_REPLY1_7C);
        ZB_GET_OUT_BUF_DELAYED(send_channel_config);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
      }
    case TEST_STATE_SEND_COMM_REPLY_RESP1_7C:
      {
        ZB_ZGPC_SET_PAUSE(TH_GPS_PAUSE_BEFORE_COMM_REPLY2_7C);
        ZB_GET_OUT_BUF_DELAYED(send_comm_reply);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
      }
    case TEST_STATE_SEND_COMM_REPLY_RESP2_7C:
      {
        ZB_ZGPC_SET_PAUSE(TH_GPS_PAUSE_BEFORE_STEP_8A);
        ZB_GET_OUT_BUF_DELAYED(send_comm_reply);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
      }
#endif

#ifndef TH_SKIP_STEP_8A
    case TEST_STATE_COMM_MODE_ENTER_8A:
      {
        ZB_ZGPC_SET_PAUSE(TH_GPS_PAUSE_BEFORE_CH_CFG_8A);
        ZGP_GPS_COMMISSIONING_WINDOW = TH_GPS_COMM_WINDOW_8A;
        enter_comm_mode();
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
      }
    case TEST_STATE_SEND_CHANNEL_CONFIG_RESP_8A:
      {
        ZB_ZGPC_SET_PAUSE(TH_GPS_PAUSE_BEFORE_STEP_8B);
        ZB_GET_OUT_BUF_DELAYED(send_channel_config);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
      }
#endif

#ifndef TH_SKIP_STEP_8B
    case TEST_STATE_COMM_MODE_ENTER_8B:
      {
        ZB_ZGPC_SET_PAUSE(TH_GPS_PAUSE_BEFORE_CH_CFG_8B);
        ZGP_GPS_COMMISSIONING_WINDOW = TH_GPS_COMM_WINDOW_8B;
        enter_comm_mode();
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
      }
    case TEST_STATE_SEND_CHANNEL_CONFIG_RESP_8B:
      {
        ZB_ZGPC_SET_PAUSE(TH_GPS_PAUSE_BEFORE_STEP_8C);
        ZB_GET_OUT_BUF_DELAYED(send_channel_config);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
      }
#endif

#ifndef TH_SKIP_STEP_8C
    case TEST_STATE_COMM_MODE_ENTER_8C:
      {
        ZB_ZGPC_SET_PAUSE(TH_GPS_PAUSE_BEFORE_CH_CFG_8C);
        ZGP_GPS_COMMISSIONING_WINDOW = TH_GPS_COMM_WINDOW_8C;
        enter_comm_mode();
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
      }
    case TEST_STATE_SEND_CHANNEL_CONFIG_RESP_8C:
      {
        ZB_ZGPC_SET_PAUSE(TH_GPS_PAUSE_BEFORE_STEP_9A);
        ZB_GET_OUT_BUF_DELAYED(send_channel_config);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
      }
#endif

#ifndef TH_SKIP_STEP_9A
    case TEST_STATE_COMM_MODE_ENTER_9A:
      {
        ZB_ZGPC_SET_PAUSE(TH_GPS_PAUSE_BEFORE_COMM_REPLY_9A);
        ZGP_GPS_COMMISSIONING_WINDOW = TH_GPS_COMM_WINDOW_9A;
        enter_comm_mode();
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
      }
    case TEST_STATE_SEND_COMM_REPLY_RESP_9A:
      {
        ZB_ZGPC_SET_PAUSE(TH_GPS_PAUSE_BEFORE_STEP_9B);
        ZB_GET_OUT_BUF_DELAYED(send_comm_reply);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
      }
#endif

    case TEST_STATE_COMM_MODE_ENTER_9B:
      {
        ZB_ZGPC_SET_PAUSE(TH_GPS_PAUSE_BEFORE_COMM_REPLY_9B);
        ZGP_GPS_COMMISSIONING_WINDOW = TH_GPS_COMM_WINDOW_9B;
        enter_comm_mode();
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
      }
    case TEST_STATE_SEND_COMM_REPLY_RESP_9B:
      {
        ZB_GET_OUT_BUF_DELAYED(send_comm_reply);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
      }

    case TEST_STATE_FINISHED:
      TRACE_MSG(TRACE_APP1, "TH-GPS: Test finished. Status: OK", (FMT__0));
      break;
    default:
      ZB_SCHEDULE_ALARM(test_send_command, 0, ZB_TIME_ONE_SECOND);
  }

  if (param)
  {
    zb_free_buf(ZB_BUF_FROM_REF(param));
  }
}

/*============================================================================*/
/*                            TEST IMPLEMENTATION                             */
/*============================================================================*/

static void dev_annce_cb(zb_zdo_device_annce_t *da)
{
  zb_ieee_addr_t dut_ieee = DUT_GPP_IEEE_ADDR;
  TRACE_MSG(TRACE_APP1, "dev_annce_cb: ieee = " TRACE_FORMAT_64 " NWK = 0x%x",
            (FMT__A_H, TRACE_ARG_64(da->ieee_addr), da->nwk_addr));

  if (ZB_IEEE_ADDR_CMP(dut_ieee, da->ieee_addr) == ZB_TRUE)
  {
    g_dut_addr = da->nwk_addr;
    if (TEST_DEVICE_CTX.test_state == TEST_STATE_WAIT_DUT_STARTUP)
    {
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
    }
  }
}

static zb_bool_t gpd_comm_cb(zb_zgpd_id_t *zgpd_id, zb_zgp_comm_status_t result)
{
  zb_bool_t ret = ZB_FALSE;

  ZVUNUSED(zgpd_id);

  TRACE_MSG(TRACE_APP1,
            "TH-GPS: gpd_comm_cb: state = %d",
            (FMT__D, TEST_DEVICE_CTX.test_state));

#ifndef TH_SKIP_STEPS_1_2
  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_WAIT_PAIRING_2D:
      if (result == ZB_ZGP_COMMISSIONING_COMPLETED)
      {
        ret = ZB_TRUE;
      }
      break;
  }
#endif

  return ret;
}

static void enter_comm_mode()
{
  ZGP_CTX().sink_comm_mode_opt &= ~0x0002;
  zb_zgps_start_commissioning(TEST_GPS_GET_COMMISSIONING_WINDOW() *
                              ZB_TIME_ONE_SECOND);
#ifdef ZB_USE_BUTTONS
  zb_led_blink_on(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif
}


static void send_channel_config(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_zgp_gp_response_t *resp = ZB_GET_BUF_PARAM(buf, zb_zgp_gp_response_t);
  zb_uint16_t temp_master_short_addr;
  zb_uint8_t temp_master_tx_channel;

  TRACE_MSG(TRACE_APP1,
            "TH-GPS: send_channel_config: state = %d",
            (FMT__D, TEST_DEVICE_CTX.test_state));

  ZB_BZERO(resp, sizeof(*resp));

  temp_master_short_addr = g_dut_addr;
  temp_master_tx_channel = TEST_CHANNEL;

  switch (TEST_DEVICE_CTX.test_state)
  {
#ifndef TH_SKIP_STEPS_1_2
    case TEST_STATE_SEND_CHANNEL_CONFIG_RESP_1A:
      {
        temp_master_tx_channel = TEST_CHANNEL + 5;
      }
      break;
#endif

#ifndef TH_SKIP_STEP_7
    case TEST_STATE_SEND_CHANNEL_CONFIG_RESP2_7A:
      {
        temp_master_short_addr = ~g_dut_addr;
      }
      break;
    case TEST_STATE_SEND_CHANNEL_CONFIG_RESP2_7B:
      {
        resp->zgpd_addr.src_id = zb_random() | (zb_random() << 2);
        temp_master_short_addr = ~g_dut_addr;
      }
      break;
#endif

#ifndef TH_SKIP_STEP_8A
    case TEST_STATE_SEND_CHANNEL_CONFIG_RESP_8A:
    {
      temp_master_tx_channel = TH_GPS_TMP_MASTER_TX_CHANNEL_X;
      break;
    }
#endif
#ifndef TH_SKIP_STEP_8B
    case TEST_STATE_SEND_CHANNEL_CONFIG_RESP_8B:
    {
      temp_master_tx_channel = TH_GPS_TMP_MASTER_TX_CHANNEL_X;
      break;
    }
#endif
#ifndef TH_SKIP_STEP_8C
    case TEST_STATE_SEND_CHANNEL_CONFIG_RESP_8C:
    {
      temp_master_tx_channel = TH_GPS_TMP_MASTER_TX_CHANNEL_X;
      break;
    }
#endif
  }

  resp->temp_master_addr = temp_master_short_addr;
  resp->temp_master_tx_chnl = temp_master_tx_channel - ZB_ZGPD_FIRST_CH;
  resp->gpd_cmd_id = ZB_GPDF_CMD_CHANNEL_CONFIGURATION;
  resp->payload[0] = 1;
#ifndef TH_SKIP_STEPS_1_2
  if (TEST_DEVICE_CTX.test_state == TEST_STATE_SEND_CHANNEL_CONFIG_RESP_1A)
  {
    resp->payload[1] = (temp_master_tx_channel - ZB_ZGPD_FIRST_CH) | (1<<4);
  }
  else
#endif
  {
    resp->payload[1] = (temp_master_tx_channel - ZB_ZGPD_FIRST_CH);
  }

  zb_zgp_cluster_gp_response_send(param,
                                  ZB_NWK_BROADCAST_ALL_DEVICES,
                                  ZB_ADDR_16BIT_DEV_OR_BROADCAST,
                                  NULL);
}

static void send_comm_reply(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_zgp_gp_response_t *resp = ZB_GET_BUF_PARAM(buf, zb_zgp_gp_response_t);
  zb_uint16_t temp_master_short_addr;
  zb_uint8_t temp_master_tx_channel;
  zb_zgpd_addr_t dest_gpd;

  TRACE_MSG(TRACE_APP1,
            "TH-GPS: send_comm_reply: state = %d",
            (FMT__D, TEST_DEVICE_CTX.test_state));

  ZB_BZERO(resp, sizeof(*resp));
  ZB_BZERO(&dest_gpd, sizeof(dest_gpd));

  temp_master_short_addr = g_dut_addr;
  temp_master_tx_channel = TEST_CHANNEL;
  dest_gpd.src_id = TEST_ZGPD_SRC_ID;

  switch (TEST_DEVICE_CTX.test_state)
  {
#ifndef TH_SKIP_STEP_7
    case TEST_STATE_SEND_COMM_REPLY_RESP2_7C:
      {
        temp_master_short_addr = ~g_dut_addr;
      }
      break;
#endif
    case TEST_STATE_SEND_COMM_REPLY_RESP_9B:
      {
        static zb_ieee_addr_t g_zgpd_addr = TH_GPD_IEEE_ADDR;

        ZB_IEEE_ADDR_COPY(dest_gpd.ieee_addr, g_zgpd_addr);
        resp->endpoint = TEST_ZGPD_EP_X;
        resp->options = ZB_ZGP_APP_ID_0010;
      }
      break;
  }

  resp->temp_master_addr = temp_master_short_addr;
  resp->temp_master_tx_chnl = temp_master_tx_channel - ZB_ZGPD_FIRST_CH;
  resp->zgpd_addr = dest_gpd;
  resp->gpd_cmd_id = ZB_GPDF_CMD_COMMISSIONING_REPLY;

  switch (TEST_DEVICE_CTX.test_state)
  {
#ifndef TH_SKIP_STEP_7
    case TEST_STATE_SEND_COMM_REPLY_RESP1_7C:
    case TEST_STATE_SEND_COMM_REPLY_RESP2_7C:
      {
        resp->payload[0] = 1;
        resp->payload[1] = 0x00;
      }
      break;
#endif
#ifndef TH_SKIP_STEP_9A
    case TEST_STATE_SEND_COMM_REPLY_RESP_9A:
      {
        zb_uint8_t i, filler;

        resp->payload[0] = 64;
        resp->payload[1] = 0x00;
        for (i = 2, filler = 0xA0;
             i < MAX_ZGP_CLUSTER_GPDF_PAYLOAD_SIZE + 1;
             ++i, ++filler)
        {
          resp->payload[i] = filler;
        }
      }
      break;
#endif
    case TEST_STATE_SEND_COMM_REPLY_RESP_9B:
      {
        zb_uint8_t i, filler;

        resp->payload[0] = 59;
        resp->payload[1] = 0x00;
        for (i = 2, filler = 0xA0;
             i < MAX_ZGP_CLUSTER_GPDF_PAYLOAD_SIZE + 1;
             ++i, ++filler)
        {
          resp->payload[i] = filler;
        }
      }
      break;
  }

  zb_zgp_cluster_gp_response_send(param,
                                  ZB_NWK_BROADCAST_ALL_DEVICES,
                                  ZB_ADDR_16BIT_DEV_OR_BROADCAST,
                                  NULL);
}

/*============================================================================*/
/*                          STARTUP                                           */
/*============================================================================*/

static void zgpc_custom_startup()
{
  /* Init device, load IB values from nvram or set it to default */
  ZB_SET_TRAF_DUMP_ON();
  ZB_INIT("th_gps");

  ZB_AIB().aps_designated_coordinator = 1;
  ZB_AIB().aps_channel_mask = (1<<TEST_CHANNEL);
  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_th_gps_addr);
  zb_set_default_ffd_descriptor_values(ZB_COORDINATOR);
  zb_secur_setup_nwk_key(g_key_nwk, 0);
  ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);
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

  ZGP_GPS_COMMUNICATION_MODE = ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED;
  ZGP_GPS_COMMISSIONING_EXIT_MODE = ZGP_COMMISSIONING_EXIT_MODE_ON_PAIRING_SUCCESS;
  ZGP_GPS_COMMISSIONING_WINDOW = 0;


  ZGP_GPS_SECURITY_LEVEL = ZB_ZGP_FILL_GPS_SECURITY_LEVEL(
                             ZB_ZGP_SEC_LEVEL_FULL_NO_ENC,
                             ZB_ZGP_DEFAULT_SEC_LEVEL_PROTECTION_WITH_GP_LINK_KEY,
                             ZB_ZGP_DEFAULT_SEC_LEVEL_INVOLVE_TC);

  ZGP_GP_SHARED_SECURITY_KEY_TYPE = ZB_ZGP_SEC_KEY_TYPE_NWK;
  zb_zdo_register_device_annce_cb(dev_annce_cb);
  ZGP_CTX().device_role = ZGP_DEVICE_COMBO_BASIC;
  TEST_DEVICE_CTX.custom_comm_cb = gpd_comm_cb;
}

ZB_ZGPC_DECLARE_STARTUP_PROCESS()
