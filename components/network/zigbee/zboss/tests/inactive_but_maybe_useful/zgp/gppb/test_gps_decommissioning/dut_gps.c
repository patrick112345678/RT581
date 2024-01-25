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
/* PURPOSE: DUT GPS
*/

#define ZB_TRACE_FILE_ID 41552
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_zcl.h"
#include "zgp/zgp_internal.h"
#include "zb_secur_api.h"


#include "zb_ha.h"

#include "test_config.h"
#include "../include/simple_combo_match.h"
#include "../include/simple_combo_zcl.h"
#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#define COMMISSIONING_TIMEOUT  0
#define DECOMMISSIONING_TIMEOUT 0


#if defined ZB_ENABLE_HA && defined ZB_ENABLE_ZGP_CLUSTER

#define ENDPOINT  10

static zb_ieee_addr_t g_zc_addr = DUT_GPS_IEEE_ADDR;
static zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);
zb_uint8_t g_key_nwk[] = { 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0, 0, 0, 0, 0, 0, 0, 0};
zb_uint8_t g_shared_key[] = { 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf };

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
zb_uint8_t g_attr_zcl_version  = ZB_ZCL_VERSION;
zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list, &g_attr_zcl_version, &g_attr_power_source);

/* Identify cluster attributes data */
zb_uint16_t g_attr_identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_attr_identify_time);

/********************* Declare device **************************/

ZB_HA_DECLARE_COMBO_CLUSTER_LIST( sample_clusters,
          basic_attr_list, identify_attr_list);

ZB_HA_DECLARE_COMBO_EP(sample_ep, ENDPOINT, sample_clusters);

ZB_HA_DECLARE_COMBO_CTX(sample_ctx, sample_ep);

void start_commissioning(zb_uint8_t param)
{
  ZVUNUSED(param);
  zb_zgps_start_commissioning(COMMISSIONING_TIMEOUT);
}


void commissioning_cb(zb_zgpd_id_t *zgpd_id, zb_zgp_comm_status_t result)
{
  TRACE_MSG(TRACE_SPECIAL2, ">> commissioning_cb result %d", (FMT__D, result));

  if (result == ZB_ZGP_ZGPD_DECOMMISSIONED)
  {
    TRACE_MSG(TRACE_APP1, "Decommissioning completed. ZGPD Src ID: 0x%08x",
        (FMT__D, zgpd_id->addr.src_id));
  }
  else if (result == ZB_ZGP_COMMISSIONING_COMPLETED)
  {
    TRACE_MSG(TRACE_APP1, "Commissioning completed. ZGPD Src ID: 0x%08x",
        (FMT__D, zgpd_id->addr.src_id));
  }

  TRACE_MSG(TRACE_APP1, "<< commissioning_cb ", (FMT__0));
}

//! [accept_comm]
void comm_req_cb(
    zb_zgpd_id_t  *zgpd_id,
    zb_uint8_t    device_id,
    zb_uint16_t   manuf_id,
    zb_uint16_t   manuf_model_id,
    zb_ieee_addr_t ieee_addr)
{
  TRACE_MSG(TRACE_APP1, ">> comm_req_cb zgpd_id %p, dev_id 0x%hx, manuf_id %d, manuf_model_id 0x%x",
      (FMT__P_H_D_H, zgpd_id, device_id, manuf_id, manuf_model_id));

  ZVUNUSED(manuf_id);
  ZVUNUSED(manuf_model_id);
  ZVUNUSED(ieee_addr);

  zb_zgps_accept_commissioning(ZB_TRUE);

  TRACE_MSG(TRACE_APP1, "<< comm_req_cb ", (FMT__0));
}
//! [accept_comm]

static zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);

MAIN()
{
  ARGV_UNUSED;

#if !(defined KEIL || defined SDCC || defined ZB_IAR)
#endif

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("dut_gps");


#ifdef ZB_ZGP_RUNTIME_WORK_MODE_WITH_PROXIES
  ZGP_CTX().enable_work_with_proxies = 1;
#endif

  zb_set_default_ffd_descriptor_values(ZB_COORDINATOR);

  /* let's always be coordinator */
  ZB_AIB().aps_designated_coordinator = 1;
  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_zc_addr);
  ZB_PIBCACHE_PAN_ID() = TEST_PAN_ID;

  ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);

  zb_set_default_ed_descriptor_values();

  /* use well-known key to simplify decrypt in Wireshark */
  zb_secur_setup_nwk_key(g_key_nwk, 0);
  ZB_ZGP_SET_MATCH_INFO(&g_zgps_match_info);
  ZB_ZGP_REGISTER_COMM_REQ_CB(comm_req_cb);
  ZB_ZGP_REGISTER_COMM_COMPLETED_CB(commissioning_cb);

  ZGP_GPS_SECURITY_LEVEL = ZB_ZGP_FILL_GPS_SECURITY_LEVEL(
                            ZB_ZGP_SEC_LEVEL_FULL_NO_ENC,
                            0,
                            ZB_ZGP_DEFAULT_SEC_LEVEL_INVOLVE_TC);

  ZB_MEMCPY(ZGP_GP_SHARED_SECURITY_KEY, g_shared_key, ZB_CCM_KEY_SIZE);

  /*
  ZB_ZGP_SEC_KEY_TYPE_NO_KEY             = 0x00,  // No key
  ZB_ZGP_SEC_KEY_TYPE_NWK                = 0x01,  // Zigbee NWK key
  ZB_ZGP_SEC_KEY_TYPE_GROUP              = 0x02,  // ZGPD group key
  ZB_ZGP_SEC_KEY_TYPE_GROUP_NWK_DERIVED  = 0x03,  // NWK-key derived ZGPD group key
  ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL    = 0x04,  // (Individual) out-of-the-box ZGPD key
  ZB_ZGP_SEC_KEY_TYPE_DERIVED_INDIVIDUAL = 0x07,  // Derived individual ZGPD key
  */

  ZGP_GP_SET_SHARED_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL);


  /****************** Register Device ********************************/
  ZB_AF_REGISTER_DEVICE_CTX(&sample_ctx);
  ZB_AF_SET_ENDPOINT_HANDLER(ENDPOINT, zcl_specific_cluster_cmd_handler);

  if (zdo_dev_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "th_gps_start failed", (FMT__0));
  }
  else
  {
    zcl_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}

static zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
  zb_buf_t *zcl_cmd_buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
  zb_zcl_parsed_hdr_t *cmd_info = ZB_GET_BUF_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);
  zb_bool_t processed = ZB_FALSE;

  TRACE_MSG(TRACE_ZCL1, "test_specific_cluster_cmd_handler %i", (FMT__H, param));
  ZB_ZCL_DEBUG_DUMP_HEADER(cmd_info);
  TRACE_MSG(TRACE_ZCL1, "payload size: %i", (FMT__D, ZB_BUF_LEN(zcl_cmd_buf)));

  switch (cmd_info->cluster_id)
  {
    case ZB_ZCL_CLUSTER_ID_ON_OFF:
      if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_SRV)
      {
        switch (cmd_info->cmd_id)
        {
          case ZB_ZCL_CMD_ON_OFF_ON_ID:
            TRACE_MSG(TRACE_ZCL1, "ON", (FMT__0));
            processed = ZB_TRUE;
            break;
          case ZB_ZCL_CMD_ON_OFF_OFF_ID:
            TRACE_MSG(TRACE_ZCL1, "OFF", (FMT__0));
            processed = ZB_TRUE;
            break;
          case ZB_ZCL_CMD_ON_OFF_TOGGLE_ID:
            TRACE_MSG(TRACE_ZCL1, "TOGGLE", (FMT__0));
            processed = ZB_TRUE;
            break;
        }
      }
      break;
  }

  if (processed)
  {
    zb_free_buf(ZB_BUF_FROM_REF(param));
  }

  return processed;
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);

  TRACE_MSG(TRACE_ZCL1, "> zb_zdo_startup_complete %hd", (FMT__H, param));

  if (buf->u.hdr.status == 0)
  {
    TRACE_MSG(TRACE_ZCL1, "DUT-GPS Device STARTED OK", (FMT__0));
    zb_zgps_start_commissioning(COMMISSIONING_TIMEOUT);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Error, Device start FAILED status %d",
              (FMT__D, (int)buf->u.hdr.status));
    zb_free_buf(buf);
  }

  TRACE_MSG(TRACE_ZCL1, "< zb_zdo_startup_complete", (FMT__0));
}

#else // defined ZB_ENABLE_HA && defined ZB_ENABLE_ZGP_CLUSTER

#include <stdio.h>
int main()
{
  printf(" HA and ZGP cluster is not supported\n");
  return 0;
}

#endif // defined ZB_ENABLE_HA
