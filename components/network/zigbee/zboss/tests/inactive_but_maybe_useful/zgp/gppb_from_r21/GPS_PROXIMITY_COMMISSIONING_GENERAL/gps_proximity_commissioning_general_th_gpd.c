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
/* PURPOSE: TH GPD
*/

#define ZB_TEST_NAME GPS_PROXIMITY_COMMISSIONING_GENERAL_TH_GPD
#define ZB_TRACE_FILE_ID 41276
#include "zb_common.h"
#include "zgpd/zb_zgpd.h"
#include "test_config.h"
#include "../include/zgp_test_templates.h"

#ifdef ZB_CERTIFICATION_HACKS

/*============================================================================*/
/*                             DECLARATIONS                                   */
/*============================================================================*/

static zb_uint32_t    g_zgpd_srcId  = TEST_ZGPD_SRC_ID;
static zb_uint8_t     g_oob_key[16] = TEST_OOB_KEY;
static zb_uint8_t     g_key_nwk[]   = NWK_KEY;
static zb_ieee_addr_t g_zgpd_addr   = TH_GPD_IEEE_ADDR;

/* Switch to unidirectional commsissioning mode */
static void app_reinit_000_unidir();
static void app_reinit_010_unidir(zb_uint8_t ep);

/*============================================================================*/
/*                             FSM CORE                                       */
/*============================================================================*/
/*! Program states according to test scenario */
enum test_states_e
{
  TEST_STATE_INITIATE,
  /* STEP 1 */
  TEST_STATE_COMM_RIGHT_1,
  TEST_STATE_START_SHIFT_1,
  TEST_STATE_SMALL_WINDOW_1,
  TEST_STATE_GPDF_WRONG_FC_1,
  TEST_STATE_MEDIUM_WINDOW_1,
  /* STEP 2A */
  TEST_STATE_COMM_SEC_LVL_01_2A,
  TEST_STATE_LARGE_WINDOW_2A,
  /* STEP 2B */
  TEST_STATE_COMM_NO_KEY_2B,
  TEST_STATE_LARGE_WINDOW_2B,
  /* STEP 2C */
  TEST_STATE_COMM_KEY_NOT_ENCR_2C,
  TEST_STATE_LARGE_WINDOW_2C,
  /* STEP 2D */
  TEST_STATE_COMM_LOWER_SEC_LVL_2D,
  TEST_STATE_LARGE_WINDOW_2D,
  /* STEP 2E */
  TEST_STATE_COMM_WRONG_MIC_2E,
  TEST_STATE_LARGE_WINDOW_2E,
  /* STEP 3A */
  TEST_STATE_COMM_WITH_AUTOM_COMM_3A,
  TEST_STATE_LARGE_WINDOW_3A,
  /* STEP 3B */
  TEST_STATE_COMM_WRONG_FRAME_TYPE_3B,
  TEST_STATE_LARGE_WINDOW_3B,
  /* STEP 3C */
  TEST_STATE_COMM_WRONG_ZB_PROTO_VER_3C,
  TEST_STATE_LARGE_WINDOW_3C,
  /* STEP 3D */
  TEST_STATE_COMM_WRONG_APP_ID_3D,
  TEST_STATE_LARGE_WINDOW_3D,
  /* STEP 3E */
  TEST_STATE_COMM_WRONG_DIR_3E,
  TEST_STATE_LARGE_WINDOW_3E,
  /* STEP 4A */
  TEST_STATE_COMM_NULL_SRC_ID_4A,
  TEST_STATE_LARGE_WINDOW_4A,
  /* STEP 4B */
  TEST_STATE_COMM_NULL_GPD_IEEE_4B,
  TEST_STATE_LARGE_WINDOW_4B,
  /* STEP 5 */
  TEST_STATE_COMM_RIGHT_5,
  TEST_STATE_SMALL_WINDOW_5,
  TEST_STATE_COMM_RIGHT_IN_OPER_5,
  TEST_STATE_MEDIUM_WINDOW_5,
  /* STEP 6 */
  TEST_STATE_COMM_RIGHT_EXTRA_6,
  TEST_STATE_START_SHIFT_6,
  TEST_STATE_LARGE_WINDOW_6,
  /* STEP 7 */
  TEST_STATE_COMM_EP_FF_7,
  TEST_STATE_SMALL_WINDOW1_7,
  TEST_STATE_GPDF1_7,
  TEST_STATE_SMALL_WINDOW2_7,
  TEST_STATE_GPDF2_7,
  /* FINISH */
  TEST_STATE_FINISHED
};


ZB_ZGPD_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 1000)

ZB_ZGPD_DECLARE_COMMISSIONING_CALLBACK()

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
  ZB_ZGPD_CHACK_RESET_ALL();

  TRACE_MSG(TRACE_APP1, ">perform_next_state: state = %d",
            (FMT__D, TEST_DEVICE_CTX.test_state));

  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_START_SHIFT_1:
    case TEST_STATE_START_SHIFT_6:
      ZB_ZGPD_SET_PAUSE(TH_GPD_START_TEST_SHIFT);
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      break;
    case TEST_STATE_SMALL_WINDOW_1:
    case TEST_STATE_SMALL_WINDOW_5:
    case TEST_STATE_SMALL_WINDOW1_7:
    case TEST_STATE_SMALL_WINDOW2_7:
      ZB_ZGPD_SET_PAUSE(TH_GPD_SMALL_WINDOW);
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      break;
    case TEST_STATE_MEDIUM_WINDOW_1:
      ZB_ZGPD_SET_PAUSE(TH_GPD_MEDIUM_WINDOW);
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      break;
    case TEST_STATE_MEDIUM_WINDOW_5:
      ZB_ZGPD_SET_PAUSE(TH_GPD_MEDIUM_WINDOW + 2);
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      break;


    case TEST_STATE_LARGE_WINDOW_2A:
    case TEST_STATE_LARGE_WINDOW_2B:
    case TEST_STATE_LARGE_WINDOW_2C:
    case TEST_STATE_LARGE_WINDOW_2D:
    case TEST_STATE_LARGE_WINDOW_2E:
    case TEST_STATE_LARGE_WINDOW_3A:
    case TEST_STATE_LARGE_WINDOW_3B:
    case TEST_STATE_LARGE_WINDOW_3C:
    case TEST_STATE_LARGE_WINDOW_3D:
    case TEST_STATE_LARGE_WINDOW_3E:
    case TEST_STATE_LARGE_WINDOW_4A:
    case TEST_STATE_LARGE_WINDOW_4B:
    case TEST_STATE_LARGE_WINDOW_6:
      ZB_ZGPD_SET_PAUSE(TH_GPD_LARGE_WINDOW);
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      break;

    case TEST_STATE_COMM_RIGHT_1:
      zb_zgpd_start_commissioning(comm_cb);
      break;

    case TEST_STATE_COMM_SEC_LVL_01_2A:
      app_reinit_000_unidir();
      /* TODO: this don't works - extend hack */
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_COMM_SEC_LEVEL);
      ZGPD->ch_replace_comm_sec_level = ZB_ZGP_SEC_LEVEL_REDUCED;
      zb_zgpd_start_commissioning(comm_cb);
      break;
    case TEST_STATE_COMM_NO_KEY_2B:
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_SKP);
      ZGPD->ch_replace_security_key_present = 0;
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_SKE);
      ZGPD->ch_replace_security_key_encrypted = 0;
      zb_zgpd_start_commissioning(comm_cb);
      break;
    case TEST_STATE_COMM_KEY_NOT_ENCR_2C:
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_SKE);
      ZGPD->ch_replace_security_key_encrypted = 0;
      zb_zgpd_start_commissioning(comm_cb);
      break;
    case TEST_STATE_COMM_LOWER_SEC_LVL_2D:
      /* Just usual commissioning */
      zb_zgpd_start_commissioning(comm_cb);
      break;
    case TEST_STATE_COMM_WRONG_MIC_2E:
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_CORRUPT_COMM_MIC);
      zb_zgpd_start_commissioning(comm_cb);
      break;

    case TEST_STATE_COMM_WITH_AUTOM_COMM_3A:
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_AUTO_COMM);
      ZGPD->ch_replace_autocomm = 1;
      zb_zgpd_start_commissioning(comm_cb);
      break;
    case TEST_STATE_COMM_WRONG_FRAME_TYPE_3B:
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_FRAME_TYPE);
      ZGPD->ch_replace_frame_type = 0x03;
      zb_zgpd_start_commissioning(comm_cb);
      break;
    case TEST_STATE_COMM_WRONG_ZB_PROTO_VER_3C:
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_PROTO_VERSION);
      ZGPD->ch_replace_proto_version = 0x02;
      zb_zgpd_start_commissioning(comm_cb);
      break;
    case TEST_STATE_COMM_WRONG_APP_ID_3D:
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_EXTNWK_FC_FLAG);
      ZGPD->ch_replace_extnwk_flag = 1;
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_INSERT_EXTNWK_FC_DATA);
      ZGPD->ch_insert_extnwk_data = 1;
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_APPID);
      ZGPD->ch_replace_app_id= 0x03;
      zb_zgpd_start_commissioning(comm_cb);
      break;
    case TEST_STATE_COMM_WRONG_DIR_3E:
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_EXTNWK_FC_FLAG);
      ZGPD->ch_replace_extnwk_flag = 1;
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_INSERT_EXTNWK_FC_DATA);
      ZGPD->ch_insert_extnwk_data = 1;
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_DIRECTION);
      ZGPD->ch_replace_direction = 1;
      zb_zgpd_start_commissioning(comm_cb);
      break;

    case TEST_STATE_COMM_NULL_SRC_ID_4A:
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_SRCID);
      ZGPD->ch_replace_src_id = 0x00000000;
      zb_zgpd_start_commissioning(comm_cb);
      break;
    case TEST_STATE_COMM_NULL_GPD_IEEE_4B:
      app_reinit_010_unidir(TEST_ZGPD_EP_X);
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_BZERO_IEEE_ADDR);
      zb_zgpd_start_commissioning(comm_cb);
      break;

    case TEST_STATE_COMM_RIGHT_5:
    case TEST_STATE_COMM_RIGHT_IN_OPER_5:
      zb_zgpd_start_commissioning(comm_cb);
      break;

    case TEST_STATE_COMM_RIGHT_EXTRA_6:
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_INSERT_COMM_EXTRA_PLD);
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_COMM_OPT_RESERVED_SET);
      ZGPD->ch_comm_extra_payload = 5;
      ZGPD->ch_comm_extra_payload_start_byte = 0xE0;
      zb_zgpd_start_commissioning(comm_cb);
      break;

    case TEST_STATE_COMM_EP_FF_7:
      app_reinit_010_unidir(0xFF);
      zb_zgpd_start_commissioning(comm_cb);
      break;

    case TEST_STATE_FINISHED:
      TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
      break;

    default:
      ZB_SCHEDULE_ALARM(test_send_command, 0, ZB_TIME_ONE_SECOND);
  };

  TRACE_MSG(TRACE_APP1, "<perform_next_state", (FMT__0));
}

/*============================================================================*/
/*                            TEST IMPLEMENTATION                             */
/*============================================================================*/

static void make_gpdf(zb_buf_t *buf, zb_uint8_t **ptr)
{
  ZVUNUSED(buf);
  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_GPDF_WRONG_FC_1:
      ZGPD->security_frame_counter -= 2;
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      break;
    case TEST_STATE_GPDF1_7:
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_STORE_SCENE7);
      break;
    case TEST_STATE_GPDF2_7:
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_RECALL_SCENE7);
      break;
  }
}

static void app_reinit_000_unidir()
{
  ZB_ZGPD_INIT_ZGPD_CTX(ZB_ZGP_APP_ID_0000,
                        ZB_ZGPD_COMMISSIONING_UNIDIR,
                        ZB_ZGP_ON_OFF_SWITCH_DEV_ID);
  ZB_ZGPD_SET_SRC_ID(g_zgpd_srcId);

  ZB_ZGPD_SET_SECURITY_LEVEL(ZB_ZGP_SEC_LEVEL_FULL_NO_ENC);
  ZB_ZGPD_SET_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_NWK);
  ZB_ZGPD_SET_SECURITY_KEY(g_key_nwk);
  ZB_ZGPD_SET_OOB_KEY(g_oob_key);
}

static void app_reinit_010_unidir(zb_uint8_t ep)
{
  ZB_ZGPD_INIT_ZGPD_CTX(ZB_ZGP_APP_ID_0010,
                        ZB_ZGPD_COMMISSIONING_UNIDIR,
                        ZB_ZGP_ON_OFF_SWITCH_DEV_ID);

  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_zgpd_addr);
  ZB_IEEE_ADDR_COPY(&g_zgpd_ctx.id.addr.ieee_addr, &g_zgpd_addr);
  g_zgpd_ctx.id.endpoint = ep;

  ZB_ZGPD_SET_SECURITY_LEVEL(ZB_ZGP_SEC_LEVEL_FULL_NO_ENC);
  ZB_ZGPD_SET_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_NWK);
  ZB_ZGPD_SET_SECURITY_KEY(g_key_nwk);
  ZB_ZGPD_SET_OOB_KEY(g_oob_key);
}

/*============================================================================*/
/*                          STARTUP                                           */
/*============================================================================*/

static void zgp_custom_startup()
{
  #if ! (defined KEIL || defined ZB_PLATFORM_LINUX_ARM_2400)
#endif

/* Init device, load IB values from nvram or set it to default */

  ZB_INIT("th_gpd");


  ZB_ZGPD_INIT_ZGPD_CTX(ZB_ZGP_APP_ID_0000, ZB_ZGPD_COMMISSIONING_BIDIR, ZB_ZGP_ON_OFF_SWITCH_DEV_ID);

  ZB_ZGPD_SET_SRC_ID(g_zgpd_srcId);

  ZB_ZGPD_SET_SECURITY_LEVEL(ZB_ZGP_SEC_LEVEL_FULL_NO_ENC);
  ZB_ZGPD_SET_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_NWK);
  ZB_ZGPD_SET_SECURITY_KEY(g_key_nwk);
  ZB_ZGPD_SET_OOB_KEY(g_oob_key);

  ZGPD->channel = TEST_CHANNEL;
}

#endif /* ZB_CERTIFICATION_HACKS */

ZB_ZGPD_TH_DECLARE_STARTUP_PROCESS()
