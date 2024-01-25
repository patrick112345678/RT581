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
/* PURPOSE: ZBS_TEST_ROUTING_FEATURES_ZC ZC device sample
*/

#define ZB_TRACE_FILE_ID 40026

#include "zboss_api.h"
//#include "zboss_api.h"
#include "zb_config.h"

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_mac.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "zbs_test_routing_features.h"

#ifdef ENABLE_RUNTIME_APP_CONFIG
static zb_ieee_addr_t g_dev_addr = ZBS_TEST_ROUTING_FEATURES_ZC_DEV_ADDR;
#endif

/** [ZBS_TEST_ROUTING_FEATURES_ZC_DEV_DEFINE_PARAMS] */
#define ZBS_TEST_ROUTING_FEATURES_ZC_DEV_ENDPOINT 1
#define ZBS_TEST_ROUTING_FEATURES_ZC_DEV_CHANNEL_MASK (1L << 19) /* 19 channel. */

#define ZBS_TEST_ROUTING_FEATURES_ZC_PERMIT_JOINING_DURATION_TIME 240
/** [ZBS_TEST_ROUTING_FEATURES_ZC_DEV_DEFINE_PARAMS] */
#define ZBS_TEST_ROUTING_FEATURES_ZC_KEEP_ALIVE_BASE 60  /* in minutes */
#define ZBS_TEST_ROUTING_FEATURES_ZC_KEEP_ALIVE_JITTER 5 /* in minutes */
#define ZBS_TEST_ROUTING_FEATURES_ZC_TIME_STATUS_INITIAL ((1 << ZB_ZCL_TIME_MASTER) | (1 << ZB_ZCL_TIME_MASTER_ZONE_DST) | (1 << ZB_ZCL_TIME_SUPERSEDING))

#define ZBS_TEST_ROUTING_FEATURES_ZC_LOAD_CONTROL_EVENT_GROUP_TIMEOUT 120 * ZB_TIME_ONE_SECOND
#define ZBS_TEST_ROUTING_FEATURES_ZC_LOAD_CONTROL_EVENT_TIMEOUT 5 * ZB_TIME_ONE_SECOND

#define DEV_ADDR_SPEC_CS1              {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define DEV_ADDR_SPEC_CS2              {0x12, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a}

#define ZBS_TEST_ROUTING_FEATURES_ZR_DEV_ADDR_INTERNAL          {0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb}
#define EL_METERING_DEV_ADDR_INTERNAL  {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11}

#define NCP_INSTALL_CODE               "966b9f3ef98ae605 9708"

zb_ieee_addr_t dev_addr_spec_cs1 = DEV_ADDR_SPEC_CS1;
zb_ieee_addr_t dev_addr_spec_cs2 = DEV_ADDR_SPEC_CS2;

zb_ieee_addr_t zbs_test_routing_features_zr_dev_addr_internal = ZBS_TEST_ROUTING_FEATURES_ZR_DEV_ADDR_INTERNAL;
zb_ieee_addr_t el_metering_dev_addr_internal = EL_METERING_DEV_ADDR_INTERNAL;

char ncp_installcode[]= NCP_INSTALL_CODE;

static zb_uint8_t gs_nwk_key[16] = {0x11, 0xaa, 0x22, 0xbb, 0x33, 0xcc, 0x44, 0xdd,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

const zb_uint16_t g_assigned_short_addr = 0x0001;
const zb_uint16_t g_invisible_short_addr = 0x0002;

/** [DECLARE_CLUSTERS] */
/*********************  Clusters' attributes  **************************/

/** @struct zbs_test_routing_features_zc_dev_ctx_s
 *  @brief ZBS_TEST_ROUTING_FEATURES_ZC device context
 */
typedef struct zbs_test_routing_features_zc_dev_ctx_s
{
  zb_zcl_basic_attrs_t basic_attrs;
  zb_zcl_kec_attrs_t kec_attrs;
  zb_zcl_time_attrs_t time_attrs;
} zbs_test_routing_features_zc_dev_ctx_t;

/* device context */
static zbs_test_routing_features_zc_dev_ctx_t g_dev_ctx;

/* Basic cluster attributes */
ZB_ZCL_DECLARE_BASIC_ATTR_LIST(basic_attr_list, g_dev_ctx.basic_attrs);

/* Key Establishment cluster attributes */
ZB_ZCL_DECLARE_KEC_ATTR_LIST(kec_attr_list, g_dev_ctx.kec_attrs);

/* Time cluster attributes */
ZB_ZCL_DECLARE_TIME_ATTR_LIST(time_attr_list, g_dev_ctx.time_attrs);
ZB_SE_DECLARE_CBKE_ZC_DEV_CLUSTER_LIST(zbs_test_routing_features_zc_dev_clusters,
                                       basic_attr_list,
                                       time_attr_list,
                                       kec_attr_list
                                       );

ZB_SE_DECLARE_CBKE_ZC_DEV_EP(zbs_test_routing_features_zc_dev_ep, ZBS_TEST_ROUTING_FEATURES_ZC_DEV_ENDPOINT, zbs_test_routing_features_zc_dev_clusters);

ZB_SE_DECLARE_CBKE_ZC_DEV_CTX(zbs_test_routing_features_zc_dev_zcl_ctx, zbs_test_routing_features_zc_dev_ep);

/** [DECLARE_CLUSTERS] */


void zbs_test_routing_features_zc_dev_ctx_init();
void zbs_test_routing_features_zc_dev_clusters_attrs_init(zb_uint8_t param);
void zbs_test_routing_features_zc_dev_app_init(zb_uint8_t param);

/*********************  Device-specific functions  **************************/

/** Init device context. */
void zbs_test_routing_features_zc_dev_ctx_init()
{
  TRACE_MSG(TRACE_APP1, ">> zbs_test_routing_features_zc_dev_ctx_init", (FMT__0));

  ZB_MEMSET(&g_dev_ctx, 0, sizeof(g_dev_ctx));
  TRACE_MSG(TRACE_APP1, "<< zbs_test_routing_features_zc_dev_ctx_init", (FMT__0));
}

/** Init device ZCL attributes. */
void zbs_test_routing_features_zc_dev_clusters_attrs_init(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> zbs_test_routing_features_zc_dev_clusters_attrs_init", (FMT__0));
  ZVUNUSED(param);

  g_dev_ctx.basic_attrs.zcl_version = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
  g_dev_ctx.basic_attrs.power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

  g_dev_ctx.kec_attrs.kec_suite = ZB_KEC_SUPPORTED_CRYPTO_ATTR;

  g_dev_ctx.time_attrs.time = zb_get_utc_time();  /* We assume: that this time is trusted */
  g_dev_ctx.time_attrs.time_status = ZBS_TEST_ROUTING_FEATURES_ZC_TIME_STATUS_INITIAL;
  g_dev_ctx.time_attrs.time_zone = ZB_ZCL_TIME_TIME_ZONE_DEFAULT_VALUE;
  g_dev_ctx.time_attrs.dst_start = ZB_ZCL_TIME_TIME_INVALID_VALUE;
  g_dev_ctx.time_attrs.dst_end = ZB_ZCL_TIME_TIME_INVALID_VALUE;
  g_dev_ctx.time_attrs.dst_shift = ZB_ZCL_TIME_DST_SHIFT_DEFAULT_VALUE;
  g_dev_ctx.time_attrs.standard_time = ZB_ZCL_TIME_TIME_INVALID_VALUE;
  g_dev_ctx.time_attrs.local_time = ZB_ZCL_TIME_TIME_INVALID_VALUE;
  g_dev_ctx.time_attrs.last_set_time = ZB_ZCL_TIME_LAST_SET_TIME_DEFAULT_VALUE;
  g_dev_ctx.time_attrs.valid_until_time = ZB_ZCL_TIME_VALID_UNTIL_TIME_DEFAULT_VALUE;

  TRACE_MSG(TRACE_APP1, "<< zbs_test_routing_features_zc_dev_clusters_attrs_init", (FMT__0));
}

/** Application callback for ZCL commands. Used for:
    - receiving application-specific values for commands (e.g. ZB_ZCL_PRICE_GET_CURRENT_PRICE_CB_ID)
    - providing received ZCL commands data to application (e.g. ZB_ZCL_DRLC_REPORT_EVENT_STATUS_CB_ID)
    Application may ignore callback id-s in which it is not interested.
 */
static void zbs_test_routing_features_zc_zcl_cmd_device_cb(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> zbs_test_routing_features_zc_zcl_cmd_device_cb(param=%hd, id=%d)",
            (FMT__H_D, param, ZB_ZCL_DEVICE_CMD_PARAM_CB_ID(param)));

  switch (ZB_ZCL_DEVICE_CMD_PARAM_CB_ID(param))
  {
    default:
      TRACE_MSG(TRACE_APP1, "Undefined callback was called, id=%d",
                (FMT__D, ZB_ZCL_DEVICE_CMD_PARAM_CB_ID(param)));
      break;
  }

  TRACE_MSG(TRACE_APP1, "<< zbs_test_routing_features_zc_zcl_cmd_device_cb", (FMT__0));
}

zb_uint16_t addr_assignment(zb_ieee_addr_t ieee_addr)
{
  return g_assigned_short_addr;
}

/** [ZBS_TEST_ROUTING_FEATURES_ZC_DEV_INIT] */
/* Application init: configure stack parameters (IEEE address, device role, channel mask etc),
 * register device context and ZCL cmd device callback), init application attributes/context. */
void zbs_test_routing_features_zc_dev_app_init(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> zbs_test_routing_features_zc_dev_app_init", (FMT__0));
/** [ZBS_TEST_ROUTING_FEATURES_ZC_DEV_INIT] */
  ZVUNUSED(param);

  /** [REGISTER_DEVICE_CTX] */
  ZB_AF_REGISTER_DEVICE_CTX(&zbs_test_routing_features_zc_dev_zcl_ctx);
  /** [REGISTER_DEVICE_CTX] */

  /* disable buttons usage for automatic testing */
  /* reset nvram every time when device starts */
  {
     zb_set_nvram_erase_at_start(ZB_TRUE);
  }

  /* device configuration */
  zbs_test_routing_features_zc_dev_ctx_init();
  zbs_test_routing_features_zc_dev_clusters_attrs_init(0);
  ZB_ZCL_REGISTER_DEVICE_CB(zbs_test_routing_features_zc_zcl_cmd_device_cb);

  zb_secur_setup_nwk_key(gs_nwk_key, 0);

  /* ZB configuration */
#ifdef ENABLE_RUNTIME_APP_CONFIG
  zb_set_long_address(g_dev_addr);
#endif

/** [ZBS_TEST_ROUTING_FEATURES_ZC_DEV_SET_ROLE] */
  zb_se_set_network_coordinator_role(ZBS_TEST_ROUTING_FEATURES_ZC_DEV_CHANNEL_MASK);
/** [ZBS_TEST_ROUTING_FEATURES_ZC_DEV_SET_ROLE] */

  zb_set_max_children(2);
  zb_nwk_set_address_assignment_cb(addr_assignment);
  mac_add_invisible_short(g_invisible_short_addr);

  TRACE_MSG(TRACE_APP1, "<< zbs_test_routing_features_zc_dev_app_init", (FMT__0));
}


/*********************  SE ZBS_TEST_ROUTING_FEATURES_ZC ZC  **************************/

/** [PERMIT_JOINING] */
/* Left button handler: permit join for 240 sec (ZBS_TEST_ROUTING_FEATURES_ZC_PERMIT_JOINING_DURATION_TIME). */
static void zbs_test_routing_features_zc_dev_left_button_handler(zb_uint8_t param)
{
  ZVUNUSED(param);
  zb_se_permit_joining(ZBS_TEST_ROUTING_FEATURES_ZC_PERMIT_JOINING_DURATION_TIME);
}

MAIN()
{
  ARGV_UNUSED;

  ZB_SET_TRACE_ON();
  ZB_SET_TRAF_DUMP_OFF();

  ZB_INIT("zbs_test_routing_features_zc");

  zb_set_pan_id(0x5043);

  zbs_test_routing_features_zc_dev_app_init(0);

  if (zboss_start_no_autostart() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zboss_start_no_autostart() failed", (FMT__0));
  }
  else
  {
    zboss_main_loop();
  }

  TRACE_DEINIT();
  MAIN_RETURN(0);
}

static void zbs_test_routing_features_zc_permit_join(zb_uint8_t param)
{
  ZVUNUSED(param);
  zb_se_permit_joining(ZBS_TEST_ROUTING_FEATURES_ZC_PERMIT_JOINING_DURATION_TIME);
}

/** [PERMIT_JOINING] */

/** Application signal handler. Used for informing application about important ZBOSS
    events/states (device started first time/rebooted, key establishment completed etc).
    Application may ignore signals in which it is not interested.
*/
/** [SIGNAL_HANDLER] */
/** [SIGNAL_HANDLER_GET_SIGNAL] */
void zboss_signal_handler(zb_uint8_t param)
{
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

  TRACE_MSG(TRACE_APP1, ">> zboss_signal_handler %hd sig %hd status %hd",
            (FMT__H_H_H, param, sig, ZB_GET_APP_SIGNAL_STATUS(param)));

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch (sig)
    {
/*      case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
        ZB_SCHEDULE_APP_CALLBACK(zbs_test_routing_features_zc_permit_join_after_dev_annce, param);
        param = 0;
        break;
*/
      case ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY:
        if (zb_buf_len(param) > sizeof(zb_zdo_app_signal_hdr_t))
        {
          se_app_production_config_t *prod_cfg =
            ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, se_app_production_config_t);

          TRACE_MSG(TRACE_APP1, "Loading application production config", (FMT__0));

          if (prod_cfg->version == SE_APP_PROD_CFG_CURRENT_VERSION)
          {
            zb_set_node_descriptor_manufacturer_code_req(prod_cfg->manuf_code, NULL);
          }
        }
        break;

/** [SIGNAL_HANDLER_GET_SIGNAL] */
      case ZB_ZDO_SIGNAL_SKIP_STARTUP:
        zb_secur_ic_str_add(dev_addr_spec_cs1, NCP_INSTALL_CODE, NULL);
        zb_secur_ic_str_add(dev_addr_spec_cs2, ncp_installcode, NULL);

        zb_secur_ic_str_add(zbs_test_routing_features_zr_dev_addr_internal, ncp_installcode, NULL);
        zb_secur_ic_str_add(el_metering_dev_addr_internal, ncp_installcode, NULL);
#ifdef ENABLE_RUNTIME_APP_CONFIG
/** [SIGNAL_HANDLER_LOAD_CERT] */
#ifdef SE_CRYPTOSUITE_1
        zb_se_load_ecc_cert(KEC_CS1, ca_public_key_cs1, zbs_test_routing_features_zc_certificate_cs1, zbs_test_routing_features_zc_private_key_cs1);
#endif
#ifdef SE_CRYPTOSUITE_2
        zb_se_load_ecc_cert(KEC_CS2, ca_public_key_cs2, zbs_test_routing_features_zc_certificate_cs2, zbs_test_routing_features_zc_private_key_cs2);
#endif
/** [SIGNAL_HANDLER_LOAD_CERT] */
#endif /* ENABLE_RUNTIME_APP_CONFIG */
        zboss_start_continue();
        break;

      case ZB_SIGNAL_DEVICE_FIRST_START:
      case ZB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APP1, "Device STARTED OK: first_start %hd",
                  (FMT__H, sig == ZB_SIGNAL_DEVICE_FIRST_START));
        //zbs_test_routing_features_zc_dev_left_button_handler(0);
        zbs_test_routing_features_zc_permit_join(0);
        break;

/** [SIGNAL_HANDLER_BIND_INDICATION] */
        case ZB_SE_SIGNAL_SERVICE_DISCOVERY_BIND_INDICATION:
        {
          zb_se_signal_service_discovery_bind_params_t *bind_params =
            ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_se_signal_service_discovery_bind_params_t);

          TRACE_MSG(TRACE_APP1, "Bind indication signal: binded cluster 0x%x endpoint %d device " TRACE_FORMAT_64,
                    (FMT__D_D_A, bind_params->cluster_id, bind_params->endpoint, TRACE_ARG_64(bind_params->device_addr)));
/** [SIGNAL_HANDLER_BIND_INDICATION] */
        }
        break;
/** [SIGNAL_HANDLER_TC_SIGNAL_CHILD_JOIN] */
      case ZB_SE_TC_SIGNAL_CHILD_JOIN_CBKE:
      {
        zb_uint8_t *remote_device_addr = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_uint8_t);
        TRACE_MSG(TRACE_APP1, "Child " TRACE_FORMAT_64 " joined & established TCLK",
                    (FMT__A, TRACE_ARG_64(remote_device_addr)));
#ifdef DEBUG_EXPOSE_KEYS
        {
          zb_uint8_t key[ZB_CCM_KEY_SIZE];
          if (zb_se_debug_get_link_key_by_long(remote_device_addr, key) == RET_OK)
          {
            TRACE_MSG(TRACE_APP1, "Child " TRACE_FORMAT_64 " joined & established TCLK " TRACE_FORMAT_128,
                      (FMT__A_B, TRACE_ARG_64(remote_device_addr), TRACE_ARG_128(key)));
/** [SIGNAL_HANDLER_TC_SIGNAL_CHILD_JOIN] */

            /* Note: that code broadcasts your TCLK! Use it only for debug purposes! Never keep it in production mode! */
            zb_debug_bcast_key(remote_device_addr, key);
          }
          if (zb_se_debug_get_nwk_key(key) == RET_OK)
          {
            TRACE_MSG(TRACE_APP1, "Current NWK key " TRACE_FORMAT_128,
                      (FMT__B, TRACE_ARG_128(key)));
            /* Note: that code broadcasts your NWK key! Use it only for debug purposes! Never keep it in production mode! */
            zb_debug_bcast_key(NULL, key);
          }
        }
#endif
        zb_ieee_addr_t *zb_ieee_addr = (zb_ieee_addr_t *)remote_device_addr;
        if (zb_address_short_by_ieee(*zb_ieee_addr) == g_assigned_short_addr)
        {
          zbs_test_routing_features_zc_permit_join(0);
          //ZB_SCHEDULE_APP_CALLBACK(zbs_test_routing_features_zc_dev_left_button_handler, 0);
          //ZB_SCHEDULE_APP_ALARM(zbs_test_routing_features_zc_dev_left_button_handler, 0, ZB_TIME_ONE_SECOND);
        }
      }
      break;
      case ZB_SE_TC_SIGNAL_CHILD_JOIN_NON_CBKE:
      {
        zb_ieee_addr_t *remote_device_addr = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_ieee_addr_t);
        TRACE_MSG(TRACE_APP1, "Child " TRACE_FORMAT_64 " joined & established non-CBKE TCLK",
                  (FMT__A, TRACE_ARG_64(remote_device_addr)));
      }
      break;
      case ZB_SIGNAL_SUBGHZ_SUSPEND:
        TRACE_MSG(TRACE_APP1, "sub-ghz. TX suspended", (FMT__0));
        break;
      case ZB_SIGNAL_SUBGHZ_RESUME:
        TRACE_MSG(TRACE_APP1, "sub-ghz. TX resumed", (FMT__0));
        break;

      case ZB_ZDO_SIGNAL_LEAVE_INDICATION:
      {
        zb_zdo_signal_leave_indication_params_t *leave_ind_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_leave_indication_params_t);
        TRACE_MSG(TRACE_APP1, "leave indication, device " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(leave_ind_params->device_addr)));
      }
      break;

      default:
        TRACE_MSG(TRACE_ERROR, "skip signal %hd", (FMT__H, sig));
        break;
    }
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
  }

  if (param)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_APP1, "<< zboss_signal_handler", (FMT__0));
}
/** [SIGNAL_HANDLER] */
