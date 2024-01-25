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

#define ZB_TEST_NAME RTP_HA_12_TH_ZR

#define ZB_TRACE_FILE_ID 40440
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

#include "device_th.h"
#include "rtp_ha_12_common.h"
#include "../common/zb_reg_test_globals.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error ZB_USE_NVRAM is not compiled!
#endif


/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_uint8_t g_nwk_key[16] = ZB_REG_TEST_DEFAULT_NWK_KEY;
static zb_ieee_addr_t g_ieee_addr_dut_zc = IEEE_ADDR_DUT_ZC;
static zb_ieee_addr_t g_ieee_addr_th_zr = IEEE_ADDR_TH_ZR;

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(rtp_ha_12_th_zr_basic_attr_list, &attr_zcl_version, &attr_power_source);

/********************* Declare device **************************/
DECLARE_TH_CLUSTER_LIST(rtp_ha_12_th_zr_device_clusters,
                        rtp_ha_12_th_zr_basic_attr_list);

DECLARE_TH_EP(rtp_ha_12_th_zr_device_ep,
              TH_ENDPOINT,
              rtp_ha_12_th_zr_device_clusters);

DECLARE_TH_CTX(rtp_ha_12_th_zr_device_ctx, rtp_ha_12_th_zr_device_ep);

static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster);
static void trigger_fb_initiator(zb_uint8_t unused);
static void send_read_attr_req_delayed(zb_uint8_t unused);
static void send_read_attr_req(zb_uint8_t param);

/************************Main*************************************/
MAIN()
{
  ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
  ZB_SET_TRACE_LEVEL(4);
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_th_zr");


  zb_set_long_address(g_ieee_addr_th_zr);

  zb_reg_test_set_common_channel_settings();
  zb_set_network_router_role((1l << TEST_CHANNEL));
  zb_set_nvram_erase_at_start(ZB_TRUE);

  zb_secur_setup_nwk_key(g_nwk_key, 0);

  ZB_AF_REGISTER_DEVICE_CTX(&rtp_ha_12_th_zr_device_ctx);

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

/********************ZDO Startup*****************************/
ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  switch (sig)
  {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));
      if (status == 0)
      {
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);

        test_step_register(send_read_attr_req_delayed, 0, RTP_HA_12_STEP_1_TIME_ZR);

        test_control_start(TEST_MODE, RTP_HA_12_STEP_1_DELAY_ZR);
      }
      break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_STEERING:
      TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
      if (status == 0)
      {
        ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, ZB_TIME_ONE_SECOND);
      }
      break; /* ZB_BDB_SIGNAL_STEERING */

    case ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED:
      TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED, status %d", (FMT__D, status));
      break; /* ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}


static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster)
{
  TRACE_MSG(TRACE_ZCL1, "finding_binding_cb status %d addr " TRACE_FORMAT_64 " ep %hd cluster %d",
            (FMT__D_A_H_D, status, TRACE_ARG_64(addr), ep, cluster));
  return ZB_TRUE;
}

static void trigger_fb_initiator(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  zb_bdb_finding_binding_initiator(TH_ENDPOINT, finding_binding_cb);
}

static void send_read_attr_req_delayed(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  zb_buf_get_out_delayed(send_read_attr_req);
}

static void send_read_attr_req(zb_uint8_t param)
{
  zb_uint8_t *cmd_ptr;
  zb_uint16_t addr = zb_address_short_by_ieee(g_ieee_addr_dut_zc);

  TRACE_MSG(TRACE_ZCL1, ">>send_read_attr_req: param %h", (FMT__H, param));

  ZB_ZCL_GENERAL_INIT_READ_ATTR_REQ_A(param, cmd_ptr,
                                      ZB_ZCL_FRAME_DIRECTION_TO_SRV,
                                      ZB_ZCL_ENABLE_DEFAULT_RESPONSE);
  ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, ZB_ZCL_ATTR_BASIC_ZCL_VERSION_ID);
  ZB_ZCL_GENERAL_SEND_READ_ATTR_REQ(param, cmd_ptr,
                                   addr,
                                   ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT,
                                   DUT_ENDPOINT,
                                   TH_ENDPOINT,
                                   ZB_AF_HA_PROFILE_ID,
                                   ZB_ZCL_CLUSTER_ID_BASIC,
                                   NULL);

  TRACE_MSG(TRACE_ZCL1, "<<send_read_attr_req", (FMT__0));
}

/*! @} */
