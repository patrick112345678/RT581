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

#define ZB_TEST_NAME RTP_PRODCONF_01_DUT_ZED

#define ZB_TRACE_FILE_ID 40337
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"
#include "zb_mac_globals.h"

#include "rtp_prodconf_01_common.h"
#include "../common/zb_reg_test_globals.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_ED_ROLE
#error ED role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error ZB_USE_NVRAM is not compiled!
#endif

#define TRACE_FORMAT_144 "%02hx:%02hx:%02hx:%02hx:%02hx:%02hx:%02hx:%02hx:%02hx:%02hx:%02hx:%02hx:%02hx:%02hx:%02hx:%02hx:%02hx:%02hx"

#define TRACE_ARG_144(a) (zb_uint8_t)((a)[0]),(zb_uint8_t)((a)[1]),(zb_uint8_t)((a)[2]),(zb_uint8_t)((a)[3]),(zb_uint8_t)((a)[4]),(zb_uint8_t)((a)[5]),(zb_uint8_t)((a)[6]),(zb_uint8_t)((a)[7]),(zb_uint8_t)((a)[8]),(zb_uint8_t)((a)[9]),(zb_uint8_t)((a)[10]),(zb_uint8_t)((a)[11]),(zb_uint8_t)((a)[12]),(zb_uint8_t)((a)[13]),(zb_uint8_t)((a)[14]),(zb_uint8_t)((a)[15]),(zb_uint8_t)((a)[16]),(zb_uint8_t)((a)[17])

static void test_ic();
static void test_extended_address();
static void test_aps_channel_mask();

#define MANUFACTURER_NAME "The_MF"
#define MODEL_ID "Pretty_ID"
#define MANUFACTURER_CODE 0X1234


static void trigger_steering(zb_uint8_t unused);
static void test_production_config_read_correct(zb_uint8_t param);

static zb_ieee_addr_t g_ieee_addr_th_zc = IEEE_ADDR_TH_ZC;

MAIN()
{
  ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
  ZB_SET_TRACE_LEVEL(4);
  ARGV_UNUSED;

  ZB_INIT("zdo_dut_zed");

  zb_reg_test_set_common_channel_settings();
  zb_set_network_ed_role((1l << TEST_CHANNEL));
  zb_set_nvram_erase_at_start(ZB_TRUE);

  /* Device should be able to join only to test's TH ZC, but not to other coordinators */
  MAC_ADD_VISIBLE_LONG(g_ieee_addr_th_zc);

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
  zb_zdo_app_signal_hdr_t *sig_hdr = NULL;
  zb_zdo_app_signal_t sig = zb_get_app_signal(param, &sig_hdr);

  TRACE_MSG(TRACE_APS1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  TRACE_MSG(TRACE_APP1, "zboss_signal_handler: status %hd signal %hd",
                  (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));

  switch (sig)
  {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));
      break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_ZDO_SIGNAL_LEAVE:
      TRACE_MSG(TRACE_APS1, "signal: ZB_ZDO_SIGNAL_LEAVE, status %d", (FMT__D, status));
      if (status == 0)
      {
        ZB_SCHEDULE_CALLBACK(trigger_steering, 0);
      }
      break; /* ZB_ZDO_SIGNAL_LEAVE */

    case ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY:
      {
        test_step_register(test_production_config_read_correct, param, RTP_PRODCONG_01_STEP_1_TIME_ZED);

        test_control_start(TEST_MODE, RTP_PRODCONG_01_STEP_1_DELAY_ZED);
      }
      break;
    case ZB_COMMON_SIGNAL_CAN_SLEEP:
      TRACE_MSG(TRACE_APS1, "signal: ZB_COMMON_SIGNAL_CAN_SLEEP, status %d", (FMT__D, status));
      if (status == 0)
      {
        zb_sleep_now();
      }
      break; /* ZB_COMMON_SIGNAL_CAN_SLEEP */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  if (sig != ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
  {
      zb_buf_free(param);
  }
}

static void test_production_config_read_correct(zb_uint8_t param)
{
  zb_uint16_t manufacturer = 0;
  zb_uint8_t expected_mf_name[ZB_ZCL_STRING_CONST_SIZE(MANUFACTURER_NAME) + 1] = {0};
  zb_uint8_t prod_mf_name[ZB_ZCL_STRING_CONST_SIZE(MANUFACTURER_NAME) + 1] = {0};
  zb_uint8_t expected_model_id[ZB_ZCL_STRING_CONST_SIZE(MODEL_ID) + 1] = {0};
  zb_uint8_t prod_model_id[ZB_ZCL_STRING_CONST_SIZE(MODEL_ID) + 1] = {0};

  zb_zdo_app_signal_hdr_t *sig_hdr = NULL;
  se_app_production_config_t *prod_cfg = NULL;

  ZB_ZCL_SET_STRING_VAL(expected_mf_name, MANUFACTURER_NAME, ZB_ZCL_STRING_CONST_SIZE(MANUFACTURER_NAME));
  ZB_ZCL_SET_STRING_VAL(expected_model_id, MODEL_ID, ZB_ZCL_STRING_CONST_SIZE(MODEL_ID));

  zb_get_app_signal(param, &sig_hdr);

/* application data should be in the buf */
  if (zb_buf_len(param) > sizeof(zb_zdo_app_signal_hdr_t))
  {
    prod_cfg = ZB_ZDO_SIGNAL_GET_PARAMS(sig_hdr, se_app_production_config_t);
    TRACE_MSG(TRACE_APP1, "Loading application production config", (FMT__0));
    TRACE_MSG(TRACE_ERROR, "Version is %hd", (FMT__H, prod_cfg->version));
    if (prod_cfg->version >= 1)
    {
      ZB_ZCL_SET_STRING_VAL(prod_mf_name, prod_cfg->manuf_name,
                            strlen(prod_cfg->manuf_name));
      ZB_ZCL_SET_STRING_VAL(prod_model_id, prod_cfg->model_id,
                            strlen(prod_cfg->model_id));
      manufacturer = prod_cfg->manuf_code;

      if (ZB_MEMCMP(prod_mf_name, expected_mf_name, sizeof(prod_mf_name)) == 0)
      {
        TRACE_MSG(TRACE_APP1, "Manufacturer name - test OK", (FMT__0));
      }
      if (ZB_MEMCMP(prod_model_id, expected_model_id, sizeof(prod_model_id)) == 0)
      {
        TRACE_MSG(TRACE_APP1, "Model id - test OK", (FMT__0));
      }

      if (manufacturer == MANUFACTURER_CODE)
      {
        TRACE_MSG(TRACE_APP1, "Manufacturer code - test OK", (FMT__0));
      }
    }

    if (prod_cfg->version >= 2)
    {
      /*
       * For now test checks application prodconfig for se_device.
       * For other application configs shall be added other tests.
       */
#if 0
    g_dev_ctx.overcurrent_ma = prod_cfg->overcurrent_ma;
    g_dev_ctx.overvoltage_dv = prod_cfg->overvoltage_dv;
#endif
    }
    test_ic();
    test_extended_address();
    test_aps_channel_mask();
  }
  zb_buf_free(param);
}

static void test_ic()
{
  zb_uindex_t i;
  zb_uint8_t ic_type;
  zb_uint8_t *installcode;

  installcode = zb_secur_ic_get_from_client_storage(&ic_type);

  for (i = 0; i < 16; i += 4)
  {
    TRACE_MSG(TRACE_APP1, "Install code: %hx:%hx:%hx:%hx", (FMT__H_H_H_H,
                                                          installcode[i],
                                                          installcode[i + 1],
                                                          installcode[i + 2],
                                                          installcode[i + 3]));
  }

  TRACE_MSG(TRACE_APP1, "Install code: %hx:%hx", (FMT__H_H,
                                                  installcode[16],
                                                  installcode[17]));
}

static void test_extended_address()
{
  zb_ieee_addr_t extended_addr = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
  zb_get_long_address(extended_addr);

  TRACE_MSG(TRACE_APP1, "IEEE 64-bit address: " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(extended_addr)));
}

static void test_aps_channel_mask()
{
  zb_channel_list_t channel_list;
  zb_uint32_t bdb_primary_channel_set = zb_get_bdb_primary_channel_set();

  zb_aib_get_channel_page_list(channel_list);

  TRACE_MSG(TRACE_APP1, "Channel mask is : %lx", (FMT__L, channel_list[ZB_CHANNEL_LIST_PAGE0_IDX]));
#ifdef ZB_BDB_MODE
/* set unconditional even if not in bdb mode. Later may not use it. */
  TRACE_MSG(TRACE_APP1, "BDB Primary channel mask: %lx", (FMT__L, bdb_primary_channel_set));
#endif
#ifdef ZB_SUBGHZ_BAND_ENABLED
  {
    zb_uint_t i;
    for (i = 1; i < ZB_CHANNEL_PAGES_NUM; i++)
    {
      zb_uint32_t channel_mask = channel_list[i];
      TRACE_MSG(TRACE_ZDO1, "aps_mask [%d] %08lx", (FMT__D_L, i, channel_mask));
    }
  }

#endif  /* ZB_SUBGHZ_BAND_ENABLED */
}

static void trigger_steering(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
}

/*! @} */
