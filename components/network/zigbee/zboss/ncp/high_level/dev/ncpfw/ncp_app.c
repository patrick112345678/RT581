/* ZBOSS Zigbee software protocol stack
 *
 * Copyright (c) 2012-2021 DSR Corporation, Denver CO, USA.
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
/* PURPOSE: NCP FW application.
This is NCP side. Other side is now Python tester script at Host.
*/

#define ZB_TRACE_FILE_ID 14000
#include "zb_common.h"
#include "zb_ncp_internal.h"
#include "ncp/zb_ncp_ll_dev.h"
#include "ncp_hl_proto.h"
#include "zb_ncp.h"
#include "zb_aps.h"
#include "zboss_api_aps_interpan.h"

#ifndef NCP_MODE
#error This app is to be compiled as NCP application FW
#endif

static void ncp_fw_start(zb_uint8_t unused);
static void zboss_signal_exec(zb_uint8_t param);

static const zb_ieee_addr_t g_ieee_addr = {0x44, 0x33, 0x22, 0x11, 0x00, 0x50, 0x50, 0x50};

typedef struct
{
  zb_uint8_t call_type;
  zb_uint8_t param;
  zb_uint8_t buf;
} ncp_exec_q_ent_t;

#define NCP_TX_BUF_SIZE (1024*2)
#define NCP_TX_Q_SIZE (16)

ZB_RING_BUFFER_DECLARE(ncp_tx_space, zb_uint8_t, NCP_TX_BUF_SIZE);
ZB_RING_BUFFER_DECLARE(ncp_tx_q, zb_uint16_t, NCP_TX_Q_SIZE);
ZB_RING_BUFFER_DECLARE(ncp_exec_q, ncp_exec_q_ent_t, NCP_TX_Q_SIZE);

typedef struct
{
  zb_bool_t exec_blocked;
  zb_bool_t tx_in_progress;
  ncp_tx_space_t tx_space;
  ncp_tx_q_t tx_q;
  ncp_exec_q_t exec_q;
} ncp_tx_buf_t;

static ncp_tx_buf_t s_tx_q;

static void ncpfw_tx_ready(void);
static void ncp_enqueue_packet(void *data, zb_uint32_t len);
static void ncp_send_packet_from_q(void);
static void ncp_clear_tx_q(void);
static zb_bool_t ncp_is_tx_q_empty(void);
static void ncp_block_exec(void);
static void ncp_unblok_exec(void);
static zb_bool_t ncp_dequeue_buf(ncp_qcall_type_t *call_type, zb_uint8_t *param, zb_bufid_t *buf);
static zb_bool_t ncp_exec_enqueued(void);

#if defined ZB_ENABLE_SE_MIN_CONFIG || defined ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ

static void ncp_init_kec_subg_zcl(void);
static zb_ret_t ncp_sync_cluster_for_ep(zb_af_simple_desc_1_1_t *dsc, zb_uint16_t cluster);

#ifdef ZB_ENABLE_SE_MIN_CONFIG
/* Declare KE cluster */
/* We need SE, so declare KE cluster */

static void ncp_kec_srv_done_cb(zb_uint8_t ref, zb_uint16_t status);
static void ncp_kec_client_done_cb(zb_uint8_t param, zb_ret_t status);

/*
  Internals KE cluster processing.

  - in SE KE cluster must be at least on some endpoint
  - if Physical device endpoint declared, KEC must exist there
  - Physical device endpoint can be absent. In such case device has only one endpoint. KEC must be there.
  - KEC can be at any other endpoint as well
  - KEC at all endpoints are aliases to the same KEC
  - In NCP all clusters except KEC are at Host side
  Means, NCP must pass to Host all ZCL traffic except KEC _if_ KEC exists in that endpoint
  - NCP user declares endpoints by adding/updating Simple descriptors. That operafion does not declare ZCL data structures.

  Solution:
  - Declare cluster list consisting of KEC and endpoint containing KEC only
  - Define & fill device context (type zb_af_device_ctx_t) for ZB_MAX_EP_NUMBER entries for ep with KEC only - all sharing same KEC clusters list.
  - Use partial register device ctx: do not fill Simple desc
  - After registering device ctx set ep number to 0, so no ep descriptor will be found in ZCL data structures.
  - When user modifies simple desc adding KEC, update device context increasing ep_count and re-arrange pointers to have enabled ep on ep array begin

 */


/* Key Establishment cluster attributes */
static zb_zcl_kec_attrs_t s_kec_attrs;

ZB_ZCL_DECLARE_KEC_ATTR_LIST(kec_attr_list, s_kec_attrs);

#endif /* ZB_ENABLE_SE_MIN_CONFIG */

#define ZB_SE_DECLARE_KEC_PART_CLUSTER_LIST(_kec_attr_list)                           \
    ZB_ZCL_CLUSTER_DESC(                                                              \
      ZB_ZCL_CLUSTER_ID_KEY_ESTABLISHMENT,                                            \
      ZB_ZCL_ARRAY_SIZE(_kec_attr_list, zb_zcl_attr_t),                               \
      (_kec_attr_list),                                                               \
      ZB_ZCL_CLUSTER_SERVER_ROLE,                                                     \
      ZB_ZCL_MANUF_CODE_INVALID                                                       \
      ),                                                                              \
    ZB_ZCL_CLUSTER_DESC(                                                              \
      ZB_ZCL_CLUSTER_ID_KEY_ESTABLISHMENT,                                            \
      ZB_ZCL_ARRAY_SIZE(_kec_attr_list, zb_zcl_attr_t),                               \
      (_kec_attr_list),                                                               \
      ZB_ZCL_CLUSTER_CLIENT_ROLE,                                                     \
      ZB_ZCL_MANUF_CODE_INVALID                                                       \
      )                                                                               \

#define ZB_SE_DECLARE_SUBG_PART_CLUSTER_LIST                                          \
    ZB_ZCL_CLUSTER_DESC(                                                              \
      ZB_ZCL_CLUSTER_ID_SUB_GHZ,                                                      \
      0,                                                                              \
      NULL,                                                                           \
      ZB_ZCL_CLUSTER_CLIENT_ROLE,                                                     \
      ZB_ZCL_MANUF_CODE_INVALID                                                       \
      )                                                                               \

#define ZB_SE_DECLARE_KEC_SUBG_CLUSTER_LIST(cluster_list_name,                        \
                                            kec_attr_list)                            \
  static zb_zcl_cluster_desc_t cluster_list_name[] =                                  \
  {                                                                                   \
    ZB_SE_DECLARE_KEC_PART_CLUSTER_LIST(kec_attr_list),                               \
    ZB_SE_DECLARE_SUBG_PART_CLUSTER_LIST                                              \
  }

#define ZB_SE_DECLARE_KEC_ONLY_CLUSTER_LIST(cluster_list_name,                        \
                                            kec_attr_list)                            \
  static zb_zcl_cluster_desc_t cluster_list_name[] =                                  \
  {                                                                                   \
    ZB_SE_DECLARE_KEC_PART_CLUSTER_LIST(kec_attr_list)                                \
  }

#define ZB_SE_DECLARE_SUBG_ONLY_CLUSTER_LIST(cluster_list_name)                       \
  static zb_zcl_cluster_desc_t cluster_list_name[] =                                  \
  {                                                                                   \
    ZB_SE_DECLARE_SUBG_PART_CLUSTER_LIST                                              \
  }

#if defined ZB_ENABLE_SE_MIN_CONFIG && defined ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ

/* Clusters list consisting of KEC and SUBGHZ */
ZB_SE_DECLARE_KEC_SUBG_CLUSTER_LIST(s_ncp_clusters, kec_attr_list);

#elif defined ZB_ENABLE_SE_MIN_CONFIG

/* Clusters list consisting of KEC only */
ZB_SE_DECLARE_KEC_ONLY_CLUSTER_LIST(s_ncp_clusters, kec_attr_list);

#else /* defined ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ */

/* Clusters list consisting of SUBGHZ only */
ZB_SE_DECLARE_SUBG_ONLY_CLUSTER_LIST(s_ncp_clusters);

#endif

/* Common clusters suppport variables */
static zb_af_endpoint_desc_t s_ep_array[ZB_MAX_EP_NUMBER];
static zb_af_endpoint_desc_t *ep_list_name[ZB_MAX_EP_NUMBER];
static zb_af_device_ctx_t s_kec_device_ctx;
#endif /* defined ZB_ENABLE_SE_MIN_CONFIG || defined ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ */

#ifndef SNCP_MODE

/* Filter datasets which should not be loaded by NCP SoC at startup.
   A host reads and writes these datasets directly. */
static zb_bool_t nvram_filter(zb_uint16_t dataset_num)
{
  zb_bool_t ret;

  switch (dataset_num)
  {
    case ZB_NVRAM_ZCL_REPORTING_DATA:
    case ZB_NVRAM_HA_DATA:
    case ZB_NVRAM_HA_POLL_CONTROL_DATA:
    case ZB_NVRAM_APP_DATA1:
    case ZB_NVRAM_APP_DATA2:
    case ZB_NVRAM_APP_DATA3:
    case ZB_NVRAM_APP_DATA4:
      ret = ZB_FALSE;
      break;
    default:
      ret = ZB_TRUE;
      break;
  }

  return ret;
}

#endif /* !defined SNCP_MODE */

MAIN()
{
  ARGV_UNUSED;

#ifdef ZB_HAVE_CALIBRATION
  /* The result will be saved in NCP_CTX */
  ncp_perform_calibration();
#endif

#ifdef SNCP_MODE
#if defined ZB_TRAFFIC_DUMP_ON
  /* dump traffic is required even if trace is disabled */
  ZB_SET_TRAF_DUMP_ON();
#endif
#if defined(ZB_TRACE_LEVEL) && ZB_TRACE_LEVEL
  /* trace is enabled only if ZB_TRACE_LEVEL is not a zero */
  ZB_SET_TRACE_ON();
#endif /* ZB_TRACE_LEVEL */
#else
#if defined(ZB_TRACE_LEVEL) && ZB_TRACE_LEVEL
  ZB_SET_TRACE_ON();
  ZB_SET_TRAF_DUMP_ON();
#endif /* ZB_TRACE_LEVEL */
#endif /* SNCP_MODE */

  ZB_INIT("ncp_fw");

#ifdef ZB_NCP_ENABLE_OTA_CMD
  zb_osif_bootloader_report_successful_loading();
#endif /* ZB_NCP_ENABLE_OTA_CMD */

  /* set some reasonable and well visible default */
  zb_set_long_address(g_ieee_addr);

#if defined ZB_ENABLE_SE_MIN_CONFIG || defined ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ
  ncp_init_kec_subg_zcl();

#ifdef ZB_CERT_HACKS
  ZB_CERT_HACKS().allow_cbke_from_zc_ncp_dev = 1;
#endif

#endif /* defined ZB_ENABLE_SE_MIN_CONFIG || defined ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ */

  /* Temporary - debug stuff */
  /* zb_set_nvram_erase_at_start(ZB_TRUE);*/
  zb_set_nvram_erase_at_start(ZB_FALSE);
  /* TODO: implement "SE mode" NCP command to mandate installcodes */
//  ZB_BDB().bdb_join_uses_install_code_key = 1;

#ifdef ZB_PRODUCTION_CONFIG
#ifdef ZB_NCP_PRODUCTION_CONFIG_ON_HOST
  /* Disable production config loading on SoC as it will be requested and applied by the host later */
  zb_production_config_disable(ZB_TRUE);
#else
  /* Enable production config loading and applying on SoC */
  zb_production_config_disable(ZB_FALSE);
#endif /* ZB_NCP_PRODUCTION_CONFIG_ON_HOST */
#endif /* ZB_PRODUCTION_CONFIG */

#ifndef SNCP_MODE
  ZB_NVRAM().ds_filter_cb = nvram_filter;
#endif /* !defined SNCP_MODE */

  if (zboss_start_no_autostart() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
  }
  else
  {
    zboss_main_loop();
  }

#if defined(ZB_TRACE_LEVEL) && ZB_TRACE_LEVEL
  TRACE_DEINIT();
#endif /* ZB_TRACE_LEVEL */

  MAIN_RETURN(0);
}


#if defined ZB_ENABLE_SE_MIN_CONFIG || defined ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ
/**
   Hand-made processing of ZCL data structures for KEC and/or SubGhz Cluster.
 */
static void ncp_init_kec_subg_zcl(void)
{
  zb_uint_t i;

  /* use s_ep_array as a storage for ZCL ep descriptors */
  ZB_BZERO(s_ep_array, sizeof(s_ep_array));
  for (i = 0 ; i < ZB_MAX_EP_NUMBER ; ++i)
  {
    s_ep_array[i].profile_id = ZB_AF_SE_PROFILE_ID;
    s_ep_array[i].cluster_count = (zb_uint8_t)ZB_ARRAY_SIZE(s_ncp_clusters); /* KEC client and server for our ZCL */
    s_ep_array[i].cluster_desc_list = s_ncp_clusters;
    /* s_ep_array[i].simple_desc is to be assigned at simple desc update. */
    /* Initialize ep list array */
    ep_list_name[i] = &s_ep_array[i];
  }

  s_kec_device_ctx.ep_count = 0; /*0 eps are active initially*/
  s_kec_device_ctx.ep_desc_list = ep_list_name;

  /*   g_dev_ctx.kec_attrs.kec_suite = ZB_KEC_SUPPORTED_CRYPTO_ATTR; */

  /* NCP does not use SE logic so skip full SE init, but call KEC init. */
#ifdef ZB_ENABLE_SE_MIN_CONFIG
  zb_kec_init();
  zb_zcl_kec_init_client();
  zb_zcl_kec_init_server();
  ncp_set_kec_suite(ZB_KEC_SUPPORTED_CRYPTO_ATTR);
#endif

  /* Sure for NCP the only device ctx can contain KEC only */
  zb_zcl_register_device_ctx(&s_kec_device_ctx);
}

#ifdef ZB_ENABLE_SE_MIN_CONFIG
/**
   Set KEC CS at runtime
 */
void ncp_set_kec_suite(zb_uint16_t suite)
{
  s_kec_attrs.kec_suite = suite;
}
#endif

#if defined ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ
static void ncp_subg_cluster_init(void)
{
  /* TODO: Check the Role and select client- or server- init() */
  zb_zcl_subghz_init_client();
}
#endif


/**
   Check if new/updated simple desc has KEC/SubGHz cluster(s) and, if so, setup ZCL ep descriptor for it.
 */
static zb_ret_t ncp_sync_cluster_for_ep(zb_af_simple_desc_1_1_t *dsc, zb_uint16_t cluster)
{
  zb_uint_t i;
  zb_ret_t ret = RET_OK;
  zb_bool_t have_cluster = ZB_FALSE;
  zb_uint_t app_out_in_count;

  TRACE_MSG(TRACE_APP3, "ncp_sync_cluster_for_ep dsc %p cluster 0x%x ep %hu dev ep count %hu",
            (FMT__P_H_H_H, dsc, cluster, dsc->endpoint, s_kec_device_ctx.ep_count));

  app_out_in_count = ((zb_uint_t)dsc->app_input_cluster_count) + ((zb_uint_t)dsc->app_output_cluster_count);

  for (i = 0 ; i < app_out_in_count ; ++i)
  {
    if (dsc->app_cluster_list[i] == cluster)
    {
      TRACE_MSG(TRACE_APP3, "this dsc have cluster 0x%x at i %u", (FMT__H_D, cluster, i));
      have_cluster = ZB_TRUE;
      break;
    }
  }

  if (have_cluster)
  {
    for (i = 0 ; i < s_kec_device_ctx.ep_count ; ++i)
    {
      if (s_kec_device_ctx.ep_desc_list[i]->ep_id == dsc->endpoint)
      {
        TRACE_MSG(TRACE_APP3, "already 0x%x at ep %hu - do nothing", (FMT__H_H, cluster, dsc->endpoint));
        /* 'cluster' is already enabled for that ep */
        ret = RET_ALREADY_EXISTS;
        break;
      }
    }
    if (i == s_kec_device_ctx.ep_count)
    {
      /* SE build excludes GPPB, so we have no GP endpoint ==> all endpoints != ZDO are under user's control. */
      if (s_kec_device_ctx.ep_count >= ZB_MAX_EP_NUMBER)
      {
        TRACE_MSG(TRACE_APP3, "Too many ep enabled (%d) - can it ever happen?", (FMT__D, s_kec_device_ctx.ep_count));
        ret = RET_NO_MEMORY;
      }
      else
      {
        s_kec_device_ctx.ep_count++;
        s_kec_device_ctx.ep_desc_list[i]->ep_id = dsc->endpoint;
        s_kec_device_ctx.ep_desc_list[i]->simple_desc = dsc;
      }
    }
  }
  return ret;
}

zb_ret_t ncp_sync_kec_subg_for_ep(zb_af_simple_desc_1_1_t *dsc)
{
  zb_ret_t ret;
  zb_ret_t ret_kec;
  zb_ret_t ret_subg;

#ifdef ZB_ENABLE_SE_MIN_CONFIG
  ret_kec = ncp_sync_cluster_for_ep(dsc, ZB_ZCL_CLUSTER_ID_KEY_ESTABLISHMENT);
  if (ret_kec == RET_OK)
  {
    TRACE_MSG(TRACE_APP3, "Setup KEC at ep %hd", (FMT__H, dsc->endpoint));
    ncp_kec_tc_init();
  }
  if (ret_kec == RET_ALREADY_EXISTS)
  {
    ret_kec = RET_OK; /* Just ignore RET_ALREADY_EXISTS */
  }
#else
  ret_kec = RET_OK;
#endif
#ifdef ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ
  ret_subg = ncp_sync_cluster_for_ep(dsc, ZB_ZCL_CLUSTER_ID_SUB_GHZ);
  if (ret_subg == RET_OK)
  {
    TRACE_MSG(TRACE_APP3, "Setup SubGhz Cluster at ep %hd", (FMT__H, dsc->endpoint));
    ncp_subg_cluster_init();
  }
  if (ret_subg == RET_ALREADY_EXISTS)
  {
    ret_subg = RET_OK; /* Just ignore RET_ALREADY_EXISTS */
  }
#else
  ret_subg = RET_OK;
#endif
  /* ret must be RET_OK (0) or RET_NO_MEMORY */
  if ((ret_kec != RET_OK) || (ret_subg != RET_OK))
  {
    ret = RET_NO_MEMORY;
  }
  else
  {
    ret = RET_OK;
  }

  return ret;
}

#ifdef ZB_ENABLE_SE_MIN_CONFIG
static void ncp_kec_srv_done_cb(zb_uint8_t ref, zb_uint16_t status)
{
  TRACE_MSG(TRACE_ZDO2, "ncp_kec_srv_done_cb %hd %d", (FMT__H_D, ref, status));
  if (status == ((zb_uint16_t)RET_OK))
  {
    ncp_se_signal(SE_COMM_SIGNAL_JOINED_DEVICE_CBKE_OK, ref);
  }
  else
  {
    ncp_se_signal(SE_COMM_SIGNAL_JOINED_DEVICE_CBKE_FAILED, ref);
  }
}


static void ncp_kec_client_done_cb(zb_uint8_t param, zb_ret_t status)
{
  TRACE_MSG(TRACE_ZDO2, "ncp_kec_client_done_cb %hd %d", (FMT__H_D, param, status));
  if (status == RET_OK)
  {
    ncp_se_signal(SE_COMM_SIGNAL_CBKE_OK, param);
  }
  else
  {
    ncp_se_signal(SE_COMM_SIGNAL_CBKE_FAILED, param);
  }
}


void ncp_kec_tc_init(void)
{
  if (s_kec_device_ctx.ep_count > 0U)
  {
    if (ZB_IS_DEVICE_ZC())
    /*cstat !MISRAC2012-Rule-2.1_b */
    /** @mdr{00012,3} */
    {
#if defined ZB_SE_COMMISSIONING && defined ZB_COORDINATOR_ROLE
      se_minimal_tc_init();
      zse_kec_set_server_cb(ncp_kec_srv_done_cb);
#endif /* ZB_SE_COMMISSIONING && ZB_COORDINATOR_ROLE */
    }
    /*cstat !MISRAC2012-Rule-13.5 */
    /* After some investigation, the following violation of Rule 13.5 seems to be
     * a false positive. There are no side effect to 'ZB_IS_DEVICE_ZED()'. This
     * violation seems to be caused by the fact that 'ZB_IS_DEVICE_ZED()' is an
     * external macro, which cannot be analyzed by C-STAT. */
    else if (ZB_IS_DEVICE_ZR() || ZB_IS_DEVICE_ZED())
    {
      zse_kec_set_client_cb(ncp_kec_client_done_cb);
    }
    else
    {
      /* intentionally left blank */
    }
  }
}


zb_bool_t ncp_have_kec(void)
{
  return (zb_bool_t)(s_kec_device_ctx.ep_count > 0U);
}


zb_bool_t ncp_partner_lk_failed(zb_uint8_t param)
{
  if (ncp_partner_lk_inprogress())
  {
    zb_ret_t status = (param != 0U) ? ERROR_CODE(ERROR_CATEGORY_CBKE, ((zb_int_t)ZB_SE_KEY_ESTABLISHMENT_TERMINATE_NO_KE_EP)) : RET_TIMEOUT;
    ncp_hl_partner_lk_rsp(status);
  }
  if (param != 0U)
  {
    zb_buf_free(param);
  }
  return ZB_TRUE;
}
#endif /* ZB_ENABLE_SE_MIN_CONFIG */

#endif /* defined ZB_ENABLE_SE_MIN_CONFIG || defined ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ */


static void update_joined_status(void)
{
  TRACE_MSG(TRACE_APP3, "device type %d", (FMT__D, ZB_NIB().device_type));
  if (!ZB_EXTPANID_IS_ZERO(ZB_NIB_EXT_PAN_ID())
      /*cstat -MISRAC2012-Rule-13.5 */
      /* After some investigation, the following violation of Rule 13.5 seems to be
       * a false positive. There are no side effect to 'ZB_IS_DEVICE_ZC_OR_ZR()' or
       * 'zb_zdo_authenticated()'. This violation seems to be caused by the fact that both
       * 'ZB_IS_DEVICE_ZC_OR_ZR()' or 'zb_zdo_authenticated()' are external macros or functions,
       * which cannot be analyzed by C-STAT. */
      && ZB_IS_DEVICE_ZC_OR_ZR() && zb_zdo_authenticated())
  /*cstat -MISRAC2012-Rule-13.5 */
  {
    TRACE_MSG(TRACE_APP3, "Not ZED - joined", (FMT__0));
    ZB_SET_JOINED_STATUS(ZB_TRUE);
  }
  else
  {
    TRACE_MSG(TRACE_APP3, "ZED - not joined", (FMT__0));
    /* ZED must be rejoined after reset. Let Host do it explicitly. */
    ZB_SET_JOINED_STATUS(ZB_FALSE);
  }
}

/* Callback which will be called on startup procedure complete (successfull or not). */
void zboss_signal_handler(zb_uint8_t param)
{
  if (ncp_exec_blocked())
  {
    ncp_enqueue_buf(ZBOSS_SIGNAL, 0, param);
  }
  else
  {
    zboss_signal_exec(param);
  }
}

static void zboss_signal_exec(zb_uint8_t param)
{
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    zb_zdo_signal_leave_indication_params_t *leave_ind_params;
#ifndef SNCP_MODE
    zb_zdo_signal_device_authorized_params_t *dev_authorized_params;
    zb_zdo_signal_device_update_params_t *dev_update_params;
#endif
    zb_zdo_signal_leave_params_t *leave_params;
    zb_pan_id_conflict_info_t *info;

    switch(sig)
    {
      case ZB_ZDO_SIGNAL_SKIP_STARTUP:
        TRACE_MSG(TRACE_APP3, "SIGNAL_SKIP_STARTUP", (FMT__0));
        (void)ZB_SCHEDULE_APP_CALLBACK(ncp_fw_start, 0);
        break;

      case ZB_SIGNAL_NWK_INIT_DONE:
        TRACE_MSG(TRACE_APP3, "SIGNAL_NWK_INIT_DONE status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
        update_joined_status();

        /* DD: this logic doesn't make much sense for our NCP implementation:
            commissioning layer should decide what to do after a reboot, not NCP FW */
#if defined SNCP_MODE
        /* 8/23/2020 ES: to call router initiation routine when restoring from NVRAM
            TODO: add such logic for ZC (start TC)
        */
        if (ZB_IS_DEVICE_ZC_OR_ZR() && ZB_JOINED())
        {
          zb_bufid_t bufid = zb_buf_get_any();
          ZB_SCHEDULE_CALLBACK(zb_zdo_start_router, bufid);
          ncp_hl_mark_ffd_just_boot();
        }
#endif /* defined SNCP_MODE */
        /* Disable automatic PAN ID conflict resolution.
            Host decides what to do in this case. Do it here since nwk is reset during zboss_start */
        zb_enable_auto_pan_id_conflict_resolution(ZB_FALSE);
        ncp_hl_reset_notify_ncp(ZB_GET_APP_SIGNAL_STATUS(param));
        break;

      case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
        ncp_hl_rx_device_annce(ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_annce_params_t));
        break;

      case ZB_ZDO_SIGNAL_LEAVE:
        leave_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_leave_params_t);
        TRACE_MSG(TRACE_APP3, "ZB_ZDO_SIGNAL_LEAVE status %d leave_type %d", (FMT__D_D, ZB_GET_APP_SIGNAL_STATUS(param), leave_params->leave_type));
        /* handle Leave signal. leave_type can be a ZB_NWK_LEAVE_TYPE_RESET, ZB_NWK_LEAVE_TYPE_REJOIN */
        ncp_hl_nwk_leave_itself(leave_params);
        break;

      case ZB_ZDO_SIGNAL_LEAVE_INDICATION:
        TRACE_MSG(TRACE_APP3, "ZB_ZDO_SIGNAL_LEAVE_INDICATION status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
        leave_ind_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_leave_indication_params_t);
        ncp_hl_nwk_leave_ind(leave_ind_params);
        break;

      case ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED:
        TRACE_MSG(TRACE_APP3, "ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
#ifndef SNCP_MODE
        dev_authorized_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_authorized_params_t);
        ncp_hl_nwk_device_authorized_ind(dev_authorized_params);
#endif
        break;

      case ZB_ZDO_SIGNAL_DEVICE_UPDATE:
        TRACE_MSG(TRACE_APP3, "ZB_ZDO_SIGNAL_DEVICE_UPDATE status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
#ifndef SNCP_MODE
        dev_update_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_update_params_t);
        ncp_hl_nwk_device_update_ind(dev_update_params);
#endif
        break;

#ifdef ZB_ENABLE_SE_MIN_CONFIG
      case ZB_SE_SIGNAL_APS_KEY_READY:
        if (ncp_partner_lk_inprogress())
        {
          ncp_hl_partner_lk_rsp(RET_OK);
        }
        else
        {
          zb_uint8_t *peer_addr = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_uint8_t);
          ncp_hl_partner_lk_ind(peer_addr);
        }
        break;
#endif /* ZB_ENABLE_SE_MIN_CONFIG */

      case ZB_COMMON_SIGNAL_CAN_SLEEP:
      {
        zb_zdo_signal_can_sleep_params_t *can_sleep_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_can_sleep_params_t);

        ZVUNUSED(can_sleep_params);

        TRACE_MSG(TRACE_APP3, "Can sleep for %ld ms", (FMT__L, can_sleep_params->sleep_tmo));
        TRACE_MSG(TRACE_ERROR, "Can sleep for %ld ms", (FMT__L, can_sleep_params->sleep_tmo));
        if (!ncp_hl_is_manufacture_mode())
        {
          ZB_OSIF_NCP_TRANSPORT_PREPARE_TO_SLEEP();
#ifdef ZB_NSNG
          zb_sleep_now();
#endif
        }
        break;
      }

      case ZB_NWK_SIGNAL_PANID_CONFLICT_DETECTED:
        info = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_pan_id_conflict_info_t);
        ncp_hl_nwk_pan_id_conflict_ind(info);
        break;

      case ZB_ZDO_SIGNAL_DEFAULT_START:
#ifdef ZB_NCP_ENABLE_MANUFACTURE_CMD
        if (ncp_hl_is_manufacture_mode())
        {
          TRACE_MSG(TRACE_APP3, "MANUFACTURE MODE STARTED", (FMT__0));
          ncp_hl_manuf_init_cont(param);
          /* to avoid buf freeing below */
          param = 0;
        }
#endif /* ZB_NCP_ENABLE_MANUFACTURE_CMD */
        break;

#ifndef ZB_NCP_PRODUCTION_CONFIG_ON_HOST
      case ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY:
        if (zb_buf_len(param) > sizeof(zb_zdo_app_signal_hdr_t))
        {
          ncp_app_production_config_t *prod_cfg =
            ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, ncp_app_production_config_t);

          TRACE_MSG(TRACE_APP1, "Loading application production config", (FMT__0));

          ncp_hl_set_prod_config(prod_cfg);
        }
        break;
#endif /* ZB_NCP_PRODUCTION_CONFIG_ON_HOST */

#ifdef ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ
        case ZB_SIGNAL_SUBGHZ_SUSPEND:
          {
            zb_uint_t *susp_period = (zb_uint_t *)ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_uint_t);
            TRACE_MSG(TRACE_APP3, "ZB_SIGNAL_SUBGHZ_SUSPEND %d min", (FMT__D, *susp_period));
            ncp_hl_subghz_duty_cycle_msg((zb_uint8_t)*susp_period);
            break;
          }

        case ZB_SIGNAL_SUBGHZ_RESUME:
          TRACE_MSG(TRACE_APP3, "ZB_SIGNAL_SUBGHZ_RESUME", (FMT__0));
          ncp_hl_subghz_duty_cycle_msg(0U);
          break;
#endif

#if defined TC_SWAPOUT && !defined ZB_COORDINATOR_ONLY
      case ZB_TC_SWAPPED_SIGNAL:
        TRACE_MSG(TRACE_APP3, "ZB_TC_SWAPPED_SIGNAL", (FMT__0));
        ncp_hl_tc_swapped_signal(0);
        break;
#endif

#if 0
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
        /* Turn off link key exchange if legacy device support (<ZB3.0) is neeeded */
        zb_bdb_set_legacy_device_support(1);
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        ZB_SCHEDULE_APP_ALARM_CANCEL(toggle_bulbs, ZB_ALARM_ANY_PARAM);
        ZB_SCHEDULE_APP_ALARM(toggle_bulbs, 0, ZB_TIME_ONE_SECOND * 5);
        break;

      case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APP1, "Successfull steering", (FMT__0));
        break;


#endif
      default:
        TRACE_MSG(TRACE_APP1, "Unknown signal %d", (FMT__D, sig));
        break; /* MISRA Rule 16.3 requires switch cases to end with 'break' */
    }
  }
  else
  {
    switch (sig)
    {
      case ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY:
        TRACE_MSG(TRACE_ERROR, "No Production Config! status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
        break;
      default:
        TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
        break;
    }

  }

  if (param != 0U)
  {
    zb_buf_free(param);
  }
}


static void call_af_data_cb(zb_uint8_t param)
{
  (void)(*ZG->zdo.af_data_cb)(param);
}


zb_bool_t ncp_catch_zcl_packet(zb_uint8_t param, zb_zcl_parsed_hdr_t *cmd_info, zb_uint8_t zcl_parse_status)
{
  zb_bool_t ret = ZB_TRUE;

  ZVUNUSED(cmd_info);
  ZVUNUSED(zcl_parse_status);
  /* Pass to the Host all except KE cluster. */
  if (cmd_info->cluster_id != ZB_ZCL_CLUSTER_ID_KEY_ESTABLISHMENT
#ifdef ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ
      && cmd_info->cluster_id != ZB_ZCL_CLUSTER_ID_SUB_GHZ
#endif
      && cmd_info->profile_id != ZB_AF_GP_PROFILE_ID
      && (ZG->zdo.af_data_cb != NULL))
  {
    TRACE_MSG(TRACE_ZDO3, "Pass packet %d of len %d to the Host", (FMT__H_D, param, zb_buf_len(param)));
    zb_nwk_unlock_in(param);
    (void)ZB_SCHEDULE_APP_CALLBACK(call_af_data_cb, param);
  }
  else
  {
    ret = ZB_FALSE;
  }
  return ret;
}


static void ncp_fw_start(zb_uint8_t unused)
{
  ncp_ll_packet_received_cb_t cb = ncp_hl_proto_init();
  (void)unused;
  ncp_ll_proto_init(cb, ncpfw_tx_ready);
  ncp_clear_tx_q();

  /* Normal ZBOSS start begin: load NVRAM and production config (if exists), then NWK reset and load PIB from NIB. */
  /* TODO: implement an NCP command for setting a commissioning type,
           don't forget to call ncp_se_commissioning_force_link if needed */
#ifdef SNCP_MODE
  COMM_CTX().commissioning_type = ZB_COMMISSIONING_SE;
  zb_aib_tcpol_set_update_trust_center_link_keys_required(ZB_FALSE);
#else
  COMM_CTX().commissioning_type = ZB_COMMISSIONING_BDB;
  zb_aib_tcpol_set_update_trust_center_link_keys_required(ZB_TRUE);
#endif /* SNCP_MODE */
  ncp_comm_commissioning_force_link();

  zb_af_set_data_indication(ncp_hl_data_or_zdo_cmd_ind);
#ifdef ZB_ENABLE_INTER_PAN_NON_DEFAULT_CHANNEL
  zboss_enable_interpan_with_chan_change();
  zb_af_interpan_set_data_indication(ncp_hl_intrp_cmd_ind);
#endif /* ZB_ENABLE_INTER_PAN_NON_DEFAULT_CHANNEL */

  zboss_start_continue();
}


void ncp_send_packet(void *data, zb_uint32_t len)
{
  zb_bool_t need_block = ZB_TRUE;

  TRACE_MSG(TRACE_TRANSPORT3, "ncp_send_packet len %d", (FMT__D, len));
  if (ncp_is_tx_q_empty())
  {
    if (ncp_ll_send_packet(data, len) == RET_OK)
    {
      need_block = ZB_FALSE;
      s_tx_q.tx_in_progress = ZB_TRUE;
    }
  }
  if (need_block)
  {
    /* Block indications buffering it in our bufs */
    ncp_block_exec();
    /* Have no choice but enqueue that packet here */
    ncp_enqueue_packet(data, len);
  }
}


static void ncpfw_tx_ready(void)
{
  s_tx_q.tx_in_progress = ZB_FALSE;
  if (!ncp_is_tx_q_empty())
  {
    TRACE_MSG(TRACE_TRANSPORT3, "tx ready - send pkt from q", (FMT__0));
    ncp_send_packet_from_q();
  }
  else
  {
    TRACE_MSG(TRACE_TRANSPORT3, "tx ready - unblock exec", (FMT__0));
    ncp_unblok_exec();
  }
}


static void ncp_enqueue_packet(void *data, zb_uint32_t len)
{
  /* Use 2 ring buffers

     tx_space is big buffer with space for buffered packets.
     tx_q is a ring buffer with length of packets put into tx_space.
     There is one special case: when tail of tx_space is not enough to hold the packet, we skip it.
     Skip is marked as high bit in tx_q entry.
   */
  zb_uint_t avail = ZB_RING_BUFFER_AVAILABLE_CONTINUOUS_PORTION(&s_tx_q.tx_space);
  zb_uint16_t *hdr = ZB_RING_BUFFER_PUT_RESERVE(&s_tx_q.tx_q);
  zb_uint_t written = 0;
  if ((hdr != NULL) && avail >= len)
  {
    ZB_RING_BUFFER_FLUSH_PUT(&s_tx_q.tx_q);
    ZB_RING_BUFFER_BATCH_PUT(&s_tx_q.tx_space, data, len, written);
    if (written < len)
    {
      *hdr = (zb_uint16_t)(written | (1U<<15));
      /* Set high bit to mark entry to skip */
      TRACE_MSG(TRACE_TRANSPORT3, "Insert into tx buf gap of size %d", (FMT__D, written));
      hdr = ZB_RING_BUFFER_PUT_RESERVE(&s_tx_q.tx_q);
      if (hdr != NULL)
      {
        ZB_RING_BUFFER_BATCH_PUT(&s_tx_q.tx_space, data, len, written);
      }
    }
    if (hdr != NULL)
    {
      *hdr = (zb_uint16_t)written;
    }
    TRACE_MSG(TRACE_TRANSPORT3, "Buffered in tx buf %d (%d)", (FMT__D_D, len, written));
  }
  if (written < len)
  {
    TRACE_MSG(TRACE_ERROR, "Oops - no enough space in tx buffer (need %d)", (FMT__D, len));
  }
}


static void ncp_send_packet_from_q(void)
{
  zb_uint16_t *hdr = ZB_RING_BUFFER_PEEK(&s_tx_q.tx_q);
  if ((hdr != NULL)
      && (((*hdr) & (1U<<15)) != 0U))
  {
    zb_uint16_t skip = *hdr;
    skip &= ~(1U<<15);
    TRACE_MSG(TRACE_TRANSPORT3, "skip %d", (FMT__D, skip));
    ZB_RING_BUFFER_FLUSH_GET(&s_tx_q.tx_q);
    ZB_RING_BUFFER_FLUSH_GET_BATCH(&s_tx_q.tx_space, skip);
    hdr = ZB_RING_BUFFER_PEEK(&s_tx_q.tx_q);
  }
  if (hdr != NULL)
  {
    zb_uint8_t *ptr = ZB_RING_BUFFER_PEEK(&s_tx_q.tx_space);
    ZB_ASSERT(((*hdr) & (1U<<15)) == 0U);
    TRACE_MSG(TRACE_TRANSPORT3, "got %d bytes packet from tx rb", (FMT__D, *hdr));
    /* Not sure failure is ever possible, but let's check for successful send */
    if (ncp_ll_send_packet(ptr, *hdr) == RET_BUSY)
    {
      TRACE_MSG(TRACE_ERROR, "Oops - ncp_ll_send_packet return busy - will retry", (FMT__0));
    }
    else
    {
      ZB_RING_BUFFER_FLUSH_GET(&s_tx_q.tx_q);
      if (ZB_RING_BUFFER_IS_EMPTY(&s_tx_q.tx_q))
      {
        /* Prevent fragmentation in tx_space */
        ZB_RING_BUFFER_INIT(&s_tx_q.tx_space);
      }
      else
      {
        ZB_RING_BUFFER_FLUSH_GET_BATCH(&s_tx_q.tx_space, *hdr);
      }
    }
  }
}


static void ncp_clear_tx_q(void)
{
  ZB_RING_BUFFER_INIT(&s_tx_q.tx_space);
  ZB_RING_BUFFER_INIT(&s_tx_q.tx_q);
}


static zb_bool_t ncp_is_tx_q_empty(void)
{
  return (zb_bool_t)ZB_RING_BUFFER_IS_EMPTY(&s_tx_q.tx_q);
}


/**

Block commands execution until tx queue is free.

We have limited space in tx queue, so can bloc execution of commands.
Also enqueue indications.
 */
static void ncp_block_exec(void)
{
  TRACE_MSG(TRACE_TRANSPORT3, "ncp_block_exec", (FMT__0));
  s_tx_q.exec_blocked = ZB_TRUE;
}


static void ncp_unblok_exec(void)
{
  /* We try to exec enqueued command and unblock if nothing enqueued.
     That suppose that enqueued command MUST cause tx to the Host.
   */
  if (!ncp_exec_enqueued())
  {
    TRACE_MSG(TRACE_TRANSPORT3, "exec is unblocked", (FMT__0));
    s_tx_q.exec_blocked = ZB_FALSE;
  }
  else
  {
    TRACE_MSG(TRACE_TRANSPORT3, "executed enqueued cmd - exec is still blocked", (FMT__0));
  }
}


zb_bool_t ncp_exec_blocked(void)
{
  return s_tx_q.exec_blocked;
}


void ncp_enqueue_buf(ncp_qcall_type_t call_type, zb_uint8_t param, zb_bufid_t buf)
{
  ncp_exec_q_ent_t *ent = ZB_RING_BUFFER_PUT_RESERVE(&s_tx_q.exec_q);

  TRACE_MSG(TRACE_TRANSPORT3, "ncp_enqueue_buf call %d par %d buf %d", (FMT__D_D_D, call_type, param, buf));
  if (ent == NULL)
  {
    /* Free space by executing one entry */
    TRACE_MSG(TRACE_TRANSPORT3, "no space - try to execute", (FMT__0));
    (void)ncp_exec_enqueued();
    ent = ZB_RING_BUFFER_PUT_RESERVE(&s_tx_q.exec_q);
  }
  if (ent != NULL)
  {
    ent->call_type = call_type;
    ent->param = param;
    ent->buf = buf;
    ZB_RING_BUFFER_FLUSH_PUT(&s_tx_q.exec_q);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Oops - no enough space to enqueue exec buf param %d", (FMT__D, buf));
    if (buf != 0U)
    {
      /* FIXME: assert? reboot? */
      zb_buf_free(buf);
    }
  }
}


static zb_bool_t ncp_dequeue_buf(ncp_qcall_type_t *call_type, zb_uint8_t *param, zb_bufid_t *buf)
{
  ncp_exec_q_ent_t *ent = ZB_RING_BUFFER_PEEK(&s_tx_q.exec_q);
  if (ent != NULL)
  {
    *call_type = (ncp_qcall_type_t)ent->call_type;
    *param = ent->param;
    *buf = ent->buf;
    ZB_RING_BUFFER_FLUSH_GET(&s_tx_q.exec_q);
  }
  return (zb_bool_t)(ent != NULL);
}

static zb_bool_t ncp_exec_enqueued(void)
{
  ncp_qcall_type_t call_type;
  zb_uint8_t param;
  zb_bufid_t buf;

  /* May need to repeat because not all signal handlers really send something to the Host */
  do
  {
    if (ncp_dequeue_buf(&call_type, &param, &buf))
    {
      zb_bool_t sv_blocked = s_tx_q.exec_blocked;
      TRACE_MSG(TRACE_TRANSPORT3, "ncp_exec_enqueued call %d par %d buf %d", (FMT__D_D_D, call_type, param, buf));
      /* clear blocked flag. Else instead of execution we will enqueue again */
      s_tx_q.exec_blocked = ZB_FALSE;
      switch (call_type)
      {
        case ZBOSS_SIGNAL:
          zboss_signal_exec(buf);
          break;
        case NCP_SIGNAL:
          ncp_signal_exec((ncp_signal_t)param, buf);
          break;
        case NCP_SE_SIGNAL:
#ifdef ZB_ENABLE_SE_MIN_CONFIG
          ncp_se_signal_exec((zse_commissioning_signal_t)param, buf);
#endif /* ZB_ENABLE_SE_MIN_CONFIG */
          break;
        case NCP_DATA_OR_ZDO_IND:
          ncp_hl_data_or_zdo_ind_exec((zb_bool_t)param, buf);
          break;
        case NCP_DATA_REQ:
          (void)ZB_SCHEDULE_APP_CALLBACK(zb_apsde_data_request, buf);
          break;
        default:
          break;
      }
      s_tx_q.exec_blocked = sv_blocked;
      return (zb_bool_t)!ZB_RING_BUFFER_IS_EMPTY(&s_tx_q.exec_q);
    }
  }
  while (s_tx_q.tx_in_progress);
  return ZB_FALSE;
}
