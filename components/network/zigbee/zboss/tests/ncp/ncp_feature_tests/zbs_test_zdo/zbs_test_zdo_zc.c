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
/* PURPOSE: ZC starts network and sends ZDO requests to a children
*/
#define ZB_TRACE_FILE_ID 41684
#include "zb_config.h"

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "../zbs_feature_tests.h"
#include "zb_trace.h"
#ifdef ZB_CONFIGURABLE_MEM
#include "zb_mem_config_max.h"
#endif


#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_zc_addr = TEST_ZC_ADDR;
static zb_ieee_addr_t g_ncp_addr = TEST_NCP_ADDR;
static zb_uint8_t gs_nwk_key[16] = TEST_NWK_KEY;
static zb_uint8_t g_ic1[16+2] = TEST_IC;

static zb_uint16_t g_remote_addr;

static void get_ieee_addr(zb_uint8_t param);

#define SUBGHZ_PAGE 1
#define SUBGHZ_CHANNEL 25

static void test_device_annce_cb(zb_zdo_device_annce_t *da)
{
  if (!ZB_IEEE_ADDR_CMP(da->ieee_addr, g_ncp_addr))
  {
      TRACE_MSG(TRACE_ERROR, "Unknown device has joined!", (FMT__0));
  }
  else
  {
    /* Use first joined device as destination for outgoing APS packets */
    if ((g_remote_addr == 0) || (g_remote_addr == da->nwk_addr))
    {
      g_remote_addr = da->nwk_addr;

      /* Once first leave is send, custom annce handler is not needed anymore.
       * Let the device rejoin the network.
       */
      zb_zdo_register_device_annce_cb(NULL);

      ZB_SCHEDULE_ALARM(get_ieee_addr, 0, 3*ZB_TIME_ONE_SECOND);
    }
  }
}

MAIN()
{
  ARGV_UNUSED;

#if !(defined KEIL || defined SDCC || defined ZB_IAR)
#ifdef ZB_NS_BUILD
  if ( argc < 3 )
  {
    printf("%s <read pipe path> <write pipe path>\n", argv[0]);
    return 0;
  }
#endif
#endif

  ZB_SET_TRAF_DUMP_ON();
  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zbs_test_zdo_zc");


#ifdef ZB_SECURITY
//  ZB_SET_NIB_SECURITY_LEVEL(0);
#endif

  zb_set_long_address(g_zc_addr);

  zb_set_pan_id(TEST_PAN_ID);

  zb_set_nvram_erase_at_start(ZB_TRUE);
  zb_production_config_disable(ZB_TRUE);
  ZDO_CTX().conf_attr.permit_join_duration = ZB_DEFAULT_PERMIT_JOINING_DURATION;

  zb_secur_setup_nwk_key(gs_nwk_key, 0);

#ifndef SUBGIG
  /* ZB_AIB().aps_channel_mask = (1L << CHANNEL); */
  zb_aib_channel_page_list_set_2_4GHz_mask(1L << CHANNEL);
#else
  {
  zb_channel_list_t channel_list;
  zb_channel_list_init(channel_list);

  zb_channel_page_list_set_mask(channel_list, SUBGHZ_PAGE, 1<<SUBGHZ_CHANNEL);
  zb_channel_page_list_copy(ZB_AIB().aps_channel_mask_list, channel_list);
  }
#endif

  zb_set_network_coordinator_role(1L << CHANNEL);

  ZB_SET_TRAF_DUMP_ON();
  zb_zdo_register_device_annce_cb(test_device_annce_cb);

  //ZB_BDB().bdb_join_uses_install_code_key = 1;
  ZB_TCPOL().require_installcodes = 1;

  TRACE_MSG(TRACE_APS1, "test test test ", (FMT__0));
  TRACE_MSG(TRACE_APS1, "test test test ", (FMT__0));
  TRACE_MSG(TRACE_APS1, "test test test ", (FMT__0));

  TRACE_MSG(TRACE_APS1, "test test test ", (FMT__0));
  TRACE_MSG(TRACE_APS1, "test test test ", (FMT__0));
  TRACE_MSG(TRACE_APS1, "test test test ", (FMT__0));

  if (zdo_dev_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
  }
  else
  {
    zdo_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}

static void send_mgmt_bind_req_cb(zb_uint8_t param)
{
  zb_buf_free(param);

  TRACE_MSG(TRACE_APP2, "<< send_mgmt_bind_req_cb", (FMT__0));
}

static void send_mgmt_bind_req(zb_uint8_t param)
{
  zb_zdo_mgmt_bind_param_t *req_param;

  TRACE_MSG(TRACE_APS1, "send_mgmt_bind_req param %d", (FMT__D, param));

  zb_buf_reuse(param);

  req_param = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_bind_param_t);

  req_param->start_index = 0;
  req_param->dst_addr = g_remote_addr;
  zb_zdo_mgmt_bind_req(param, send_mgmt_bind_req_cb);
}

static void send_mgmt_lqi_req_cb(zb_uint8_t param)
{
  ZB_SCHEDULE_ALARM(send_mgmt_bind_req, param, 1*ZB_TIME_ONE_SECOND);

  TRACE_MSG(TRACE_APP2, "<< send_mgmt_lqi_req_cb", (FMT__0));
}

static void send_mgmt_lqi_req(zb_uint8_t param)
{
  zb_zdo_mgmt_lqi_param_t *req_param;

  TRACE_MSG(TRACE_APS1, "send_mgmt_lqi_req param %d", (FMT__D, param));

  zb_buf_reuse(param);

  req_param = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_lqi_param_t);

  req_param->start_index = 0;
  req_param->dst_addr = g_remote_addr;
  zb_zdo_mgmt_lqi_req(param, send_mgmt_lqi_req_cb);
}

static void send_unbind_req_cb(zb_uint8_t param)
{
  ZB_SCHEDULE_ALARM(send_mgmt_lqi_req, param, 1*ZB_TIME_ONE_SECOND);

  TRACE_MSG(TRACE_APP2, "<< send_unbind_req_cb", (FMT__0));
}

static void send_unbind_req(zb_uint8_t param)
{
  zb_zdo_bind_req_param_t *req_param;

  TRACE_MSG(TRACE_APS1, "send_unbind_req param %d", (FMT__D, param));

  zb_buf_reuse(param);

  req_param = ZB_BUF_GET_PARAM(param, zb_zdo_bind_req_param_t);

  ZB_IEEE_ADDR_COPY(req_param->src_address, g_ncp_addr);
  req_param->src_endp = 1;
  req_param->cluster_id = ZB_ZCL_CLUSTER_ID_IDENTIFY;
  req_param->dst_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
  ZB_IEEE_ADDR_COPY(req_param->dst_address.addr_long, g_zc_addr);
  req_param->dst_endp = 1;
  req_param->req_dst_addr = g_remote_addr;

  zb_zdo_unbind_req(param, send_unbind_req_cb);
}

static void send_bind_req_cb(zb_uint8_t param)
{
  ZB_SCHEDULE_ALARM(send_unbind_req, param, 1*ZB_TIME_ONE_SECOND);

  TRACE_MSG(TRACE_APP2, "<< send_bind_req_cb", (FMT__0));
}

static void send_bind_req(zb_uint8_t param)
{
  zb_zdo_bind_req_param_t *req_param;

  TRACE_MSG(TRACE_APS1, "send_bind_req param %d", (FMT__D, param));

  zb_buf_reuse(param);

  req_param = ZB_BUF_GET_PARAM(param, zb_zdo_bind_req_param_t);

  ZB_IEEE_ADDR_COPY(req_param->src_address, g_ncp_addr);
  req_param->src_endp = 1;
  req_param->cluster_id = ZB_ZCL_CLUSTER_ID_IDENTIFY;
  req_param->dst_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
  ZB_IEEE_ADDR_COPY(req_param->dst_address.addr_long, g_zc_addr);
  req_param->dst_endp = 1;
  req_param->req_dst_addr = g_remote_addr;

  zb_zdo_bind_req(param, send_bind_req_cb);
}

static void send_match_desc_cb(zb_uint8_t param)
{
  ZB_SCHEDULE_ALARM(send_bind_req, param, 1*ZB_TIME_ONE_SECOND);

  TRACE_MSG(TRACE_APP2, "<< send_match_desc_cb", (FMT__0));
}

static void send_match_desc(zb_uint8_t param)
{
  zb_zdo_match_desc_param_t *req_param;

  TRACE_MSG(TRACE_APS1, "send_match_desc param %d", (FMT__D, param));

  zb_buf_reuse(param);

  req_param = zb_buf_initial_alloc(param, sizeof(zb_zdo_match_desc_param_t));

  req_param->nwk_addr = g_remote_addr;
  req_param->addr_of_interest = req_param->nwk_addr;
  req_param->profile_id = ZB_AF_SE_PROFILE_ID;
  req_param->num_in_clusters = 2;
  req_param->num_out_clusters = 1;
  req_param->cluster_list[0] = ZB_ZCL_CLUSTER_ID_BASIC;
  req_param->cluster_list[1] = ZB_ZCL_CLUSTER_ID_IDENTIFY;
  req_param->cluster_list[2] = ZB_ZCL_CLUSTER_ID_CALENDAR;

  zb_zdo_match_desc_req(param, send_match_desc_cb);
}

static void get_active_ep_cb(zb_uint8_t param)
{
  ZB_SCHEDULE_ALARM(send_match_desc, param, 1*ZB_TIME_ONE_SECOND);

  TRACE_MSG(TRACE_APP2, "<< get_active_ep_cb", (FMT__0));
}

static void get_active_ep(zb_uint8_t param)
{
  zb_zdo_active_ep_req_t *req_param;

  TRACE_MSG(TRACE_APS1, "get_active_ep param %d", (FMT__D, param));

  zb_buf_reuse(param);

  req_param = zb_buf_initial_alloc(param, sizeof(zb_zdo_active_ep_req_t));

  req_param->nwk_addr = g_remote_addr;
  zb_zdo_active_ep_req(param, get_active_ep_cb);
}

static void get_simple_desc_cb(zb_uint8_t param)
{
  ZB_SCHEDULE_ALARM(get_active_ep, param, 1*ZB_TIME_ONE_SECOND);

  TRACE_MSG(TRACE_APP2, "<< get_simple_desc_cb", (FMT__0));
}

static void get_simple_desc(zb_uint8_t param)
{
  zb_zdo_simple_desc_req_t *req_param;

  TRACE_MSG(TRACE_APS1, "get_simple_desc param %d", (FMT__D, param));

  zb_buf_reuse(param);

  req_param = zb_buf_initial_alloc(param, sizeof(zb_zdo_simple_desc_req_t));

  req_param->nwk_addr = g_remote_addr;
  req_param->endpoint = 1;
  zb_zdo_simple_desc_req(param, get_simple_desc_cb);
}

static void get_power_desc_cb(zb_uint8_t param)
{
  ZB_SCHEDULE_ALARM(get_simple_desc, param, 1*ZB_TIME_ONE_SECOND);

  TRACE_MSG(TRACE_APP2, "<< get_power_desc_cb", (FMT__0));
}

static void get_power_desc(zb_uint8_t param)
{
  zb_zdo_power_desc_req_t *req_param;

  TRACE_MSG(TRACE_APS1, "get_power_desc param %d", (FMT__D, param));

  zb_buf_reuse(param);

  req_param = zb_buf_initial_alloc(param, sizeof(zb_zdo_power_desc_req_t));

  req_param->nwk_addr = g_remote_addr;
  zb_zdo_power_desc_req(param, get_power_desc_cb);
}

static void get_node_desc_cb(zb_uint8_t param)
{
  ZB_SCHEDULE_ALARM(get_power_desc, param, 1*ZB_TIME_ONE_SECOND);

  TRACE_MSG(TRACE_APP2, "<< get_node_desc_cb", (FMT__0));
}

static void get_node_desc(zb_uint8_t param)
{
  zb_zdo_node_desc_req_t *req_param;

  TRACE_MSG(TRACE_APS1, "get_node_desc param %d", (FMT__D, param));

  zb_buf_reuse(param);

  req_param = zb_buf_initial_alloc(param, sizeof(zb_zdo_node_desc_req_t));

  req_param->nwk_addr = g_remote_addr;
  zb_zdo_node_desc_req(param, get_node_desc_cb);
}

static void get_nwk_addr_cb(zb_uint8_t param)
{
  ZB_SCHEDULE_ALARM(get_node_desc, param, 1*ZB_TIME_ONE_SECOND);

  TRACE_MSG(TRACE_APP2, "<< get_nwk_addr_cb", (FMT__0));
}

static void get_nwk_addr(zb_uint8_t param)
{
  zb_zdo_nwk_addr_req_param_t *req_param;

  TRACE_MSG(TRACE_APP1, ">> get_nwk_addr param %d", (FMT__D, param));

  zb_buf_reuse(param);

  req_param = ZB_BUF_GET_PARAM(param, zb_zdo_nwk_addr_req_param_t);

  req_param->dst_addr = ZB_NWK_BROADCAST_ALL_DEVICES;
  ZB_IEEE_ADDR_COPY(req_param->ieee_addr, g_ncp_addr);
  req_param->start_index = 0;
  req_param->request_type = 0x00;

  zb_zdo_nwk_addr_req(param, get_nwk_addr_cb);
}

static void get_ieee_addr_cb(zb_uint8_t param)
{
  ZB_SCHEDULE_ALARM(get_nwk_addr, param, 1*ZB_TIME_ONE_SECOND);

  TRACE_MSG(TRACE_APP2, "<< get_ieee_addr_cb", (FMT__0));
}

static void get_ieee_addr(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> get_ieee_addr param %d", (FMT__D, param));

  if (param == 0)
  {
    zb_buf_get_out_delayed(get_ieee_addr);
  }
  else
  {    
    zb_bufid_t buf = param;
    zb_zdo_ieee_addr_req_param_t *req_param;

    req_param = ZB_BUF_GET_PARAM(buf, zb_zdo_ieee_addr_req_param_t);

    req_param->nwk_addr = g_remote_addr;
    req_param->dst_addr = g_remote_addr;
    req_param->start_index = 0;
    req_param->request_type = 0;
    zb_zdo_ieee_addr_req(buf, get_ieee_addr_cb);
  }
}

void zb_zdo_startup_complete(zb_uint8_t param)
{
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

  TRACE_MSG(TRACE_APP1, "zboss_signal_handler: status %hd signal %hd",
            (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        /* No CBKE in that test */
        zb_aib_tcpol_set_update_trust_center_link_keys_required(ZB_FALSE);
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);

        zb_secur_ic_add(g_ncp_addr, ZB_IC_TYPE_128, g_ic1, NULL);

        TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
        break;
      default:
        break;
    }
  }
  else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
  {
    TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
  }

  if (param)
  {
    zb_buf_free(param);
  }
}

/*! @} */
