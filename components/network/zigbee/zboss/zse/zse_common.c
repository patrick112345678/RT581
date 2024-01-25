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
/*  PURPOSE: Smart Energy common functions
*/

#define ZB_TRACE_FILE_ID 3896

#include "zb_common.h"

#ifdef ZB_ENABLE_SE_MIN_CONFIG

#include "zb_se.h"
#include "zboss_api_se.h"
#include "zb_aps.h"
#include "zcl/zb_zcl_common.h"
#include "zcl/zb_zcl_commands.h"

#define UNSPECIFIED_PAYLOAD_SIZE 0xff

zb_uint8_t zse_specific_cluster_cmd_handler(zb_uint8_t param);

zb_zse_ctx_t g_zse_ctx;


#ifdef ZSE_CLUSTER_TEST

/**
 * @brief Setup specific application zcl command handler for test porpose
 *
 * @param handler [in]  Specific application zcl command handler pointer
 */
void zse_cluster_set_app_zcl_cmd_handler(zgp_cluster_app_zcl_cmd_handler_t handler)
{
  zse_cluster_app_zcl_cmd_handler = handler;
}

#endif


void zb_zse_init()
{
  TRACE_MSG(TRACE_ZSE2, ">> zb_zse_init", (FMT__0));
#ifdef ZB_SE_ENABLE_KEC_CLUSTER
  zb_kec_init();
#endif

#if defined ZB_ENABLE_SE_SAS
  zb_se_init_sas();
#endif /* defined ZB_ENABLE_SE_SAS */

#if defined(ZB_SE_ENABLE_SERVICE_DISCOVERY_PROCESSING)
  zb_se_service_discovery_reset_dev();
  ZSE_CTXC().service_disc.match_desc_tsn = ZB_ZDO_INVALID_TSN;
  //ZSE_CTXC().service_disc.multiple_commodity_enabled = 1;
  ZSE_SERVICE_DISCOVERY_SET_MULTIPLE_COMMODITY();
#endif

#if defined(ZB_SE_ENABLE_STEADY_STATE_PROCESSING)
  ZSE_CTXC().steady_state.poll_method = ZSE_TC_POLL_NOT_SUPPORTED;
  ZSE_CTXC().steady_state.tsn = ZB_ZDO_INVALID_TSN;
#endif

#ifdef ZB_ZCL_SUPPORT_CLUSTER_TIME
  ZSE_CTXC().time_server.server_auth_level = ZB_ZCL_TIME_SERVER_NOT_CHOSEN;
  ZSE_CTXC().second_time_server.server_auth_level = ZB_ZCL_TIME_SERVER_NOT_CHOSEN;
  ZSE_CTXC().time_server.server_short_addr = 0xffff;
  ZSE_CTXC().second_time_server.server_short_addr = 0xffff;
#endif  /* ZB_ZCL_SUPPORT_CLUSTER_TIME */

  TRACE_MSG(TRACE_ZSE2, "<< zb_zse_init", (FMT__0));
}


#endif /* ZB_ENABLE_SE_MIN_CONFIG */
