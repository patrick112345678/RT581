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
/* PURPOSE: Simple GPPB for GP device
*/

#define ZB_TEST_NAME GPS_TUNNELING_COMMISSIONED_GROUPCAST_TH_GPP
#define ZB_TRACE_FILE_ID 41325

#include "test_config.h"

#include "../include/zgp_test_templates.h"

#ifdef ZB_CERTIFICATION_HACKS

static zb_ieee_addr_t g_th_gpp_addr = TH_GPP_IEEE_ADDR;
static zb_uint8_t g_key_nwk[] = TEST_NWK_KEY;
static zb_uint8_t g_zgpd_key[] = TEST_OOB_KEY;

/*! Program states according to test scenario */
enum test_states_e
{
  TEST_STATE_INITIAL,
  TEST_STATE_SEND_PAIRING_CONFIGURATION,
  TEST_STATE_FINISHED
};

ZB_ZGPC_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 3000)

static void send_pairing_configuration_cb(zb_uint8_t buf_ref)
{
  ZGP_CTX().device_role = ZGP_DEVICE_PROXY_BASIC;
  zb_free_buf(ZB_BUF_FROM_REF(buf_ref));
  perform_next_state(0);
}

static void send_pairing_configuration(zb_uint8_t buf_ref)
{
  zb_zgp_sink_tbl_ent_t ent;
  zb_zgp_cluster_list_t cll;
  zb_uint8_t            actions;
  zb_uint8_t            app_info;
  zb_uint8_t            num_paired_endpoints;
  zb_uint8_t            num_gpd_commands;

  actions = ZB_ZGP_FILL_GP_PAIRING_CONF_ACTIONS(ZGP_PAIRING_CONF_REPLACE,
                                                ZGP_PAIRING_CONF_SEND_PAIRING);

  app_info = ZB_ZGP_FILL_GP_PAIRING_CONF_APP_INFO(ZB_ZGP_GP_PAIRING_CONF_APP_INFO_MANUF_ID_NO_PRESENT,
                                                  ZB_ZGP_GP_PAIRING_CONF_APP_INFO_MODEL_ID_NO_PRESENT,
                                                  ZB_ZGP_GP_PAIRING_CONF_APP_INFO_GPD_CMDS_NO_PRESENT,
                                                  ZB_ZGP_GP_PAIRING_CONF_APP_INFO_CLSTS_NO_PRESENT);

  ZB_BZERO(&ent, sizeof(zb_zgp_sink_tbl_ent_t));

#define ZGPD_SEQ_NUM_CAP 1
#define ZGPD_RX_ON_CAP 0
#define ZGPD_FIX_LOC 1
#define ZGPD_USE_ASSIGNED_ALIAS 0
#define ZGPD_USE_SECURITY 1

  ent.options = ZGP_TBL_SINK_FILL_OPTIONS(
    ZB_ZGP_APP_ID_0000,
    //ZGP_COMMUNICATION_MODE_LIGHTWEIGHT_UNICAST,
    //ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED,
    ZGP_COMMUNICATION_MODE_GROUPCAST_PRECOMMISSIONED,
    ZGPD_SEQ_NUM_CAP,
    ZGPD_RX_ON_CAP,
    ZGPD_FIX_LOC,
    ZGPD_USE_ASSIGNED_ALIAS,
    ZGPD_USE_SECURITY);
  ent.sec_options = ZGP_TBL_FILL_SEC_OPTIONS(ZB_ZGP_SEC_LEVEL_FULL_NO_ENC,
                                             ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL);

  ZB_MEMSET(ent.u.sink.sgrp, 0xff, sizeof(ent.u.sink.sgrp));
  ent.zgpd_id.src_id = TEST_ZGPD_SRC_ID;
  ent.u.sink.device_id = ZB_ZGP_ON_OFF_SWITCH_DEV_ID;
  ent.u.sink.match_dev_tbl_idx = 0xff;
//  ent.zgpd_assigned_alias = 0x7777;

  ent.u.sink.sgrp[0].sink_group = TEST_ZGPD_GROUP;
  ent.u.sink.sgrp[0].alias = 0xb00d;//0xffff;

  num_paired_endpoints = 0;

  num_gpd_commands = 0;

  cll.server_cl_num=0;
  cll.client_cl_num=0;

  ZB_MEMCPY(ent.zgpd_key, g_zgpd_key, ZB_CCM_KEY_SIZE);

  zgp_cluster_send_pairing_configuration(buf_ref,
                                         0x0000,
                                         ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                         actions,
                                         &ent,
                                         num_paired_endpoints,
                                         NULL,
                                         app_info,
                                         0,
                                         0,
                                         num_gpd_commands,
                                         NULL,
                                         &cll,
                                         send_pairing_configuration_cb);
}

static void send_zcl(zb_uint8_t buf_ref, zb_callback_t cb)
{
  ZVUNUSED(cb);

  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_SEND_PAIRING_CONFIGURATION:
      send_pairing_configuration(buf_ref);
      TEST_DEVICE_CTX.skip_next_state = NEXT_STATE_SKIP_AFTER_CMD;
      break;
  };
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

  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_FINISHED:
      if (TEST_DEVICE_CTX.err_cnt)
      {
        TRACE_MSG(TRACE_APP1, "Test finished. Status: ERROR[%hd]", (FMT__H, TEST_DEVICE_CTX.err_cnt));
      }
      else
      {
        TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
      }
      break;
    default:
    {
      if (param)
      {
        zb_free_buf(ZB_BUF_FROM_REF(param));
      }
      ZB_SCHEDULE_ALARM(test_send_command, 0, (zb_time_t)(0.1f * ZB_TIME_ONE_SECOND));
    }
  }
}

static void zgpc_custom_startup()
{
/* Init device, load IB values from nvram or set it to default */
  ZB_INIT("th_gpp");

  ZB_AIB().aps_channel_mask = (1<<TEST_CHANNEL);
  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_th_gpp_addr);
  ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);

  zb_set_default_ed_descriptor_values();

  zb_secur_setup_nwk_key(g_key_nwk, 0);

  ZB_NIB_SET_USE_MULTICAST(ZB_FALSE);

  ZB_AIB().aps_use_nvram = 1;

  ZGP_GP_SET_SHARED_SECURITY_KEY_TYPE(TEST_KEY_TYPE);

  ZGP_CTX().device_role = ZGP_DEVICE_COMMISSIONING_TOOL;
}

#endif /* ZB_CERTIFICATION_HACKS */

ZB_ZGPC_TH_DECLARE_STARTUP_PROCESS()
