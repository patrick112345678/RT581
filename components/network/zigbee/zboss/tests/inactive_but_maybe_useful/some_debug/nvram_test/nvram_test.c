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
/* PURPOSE: ESI ZC device sample
*/

#define ZB_TRACE_FILE_ID 60597

#include "zboss_api.h"
#include "zb_led_button.h"

#include "../../samples/se/common/se_common.h"
#include "../../samples/se/common/se_indication.h"

#define ESI_USE_ALL_PRICE_CLUSTER_ATTRIBUTES 1

#if ESI_USE_ALL_PRICE_CLUSTER_ATTRIBUTES
#include "../../samples/se/common/price_srv_attr_sets.h"
#endif

#ifdef ENABLE_RUNTIME_APP_CONFIG
#include "../../samples/se/common/se_cert.h"
static zb_ieee_addr_t g_dev_addr = ESI_DEV_ADDR;
#ifdef ENABLE_PRECOMMISSIONED_REJOIN
static zb_ext_pan_id_t g_ext_pan_id = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
#endif
#endif

#if defined DEBUG && !defined DEBUG_EXPOSE_KEYS
#define DEBUG_EXPOSE_KEYS
#endif

#include "../../samples/se/common/se_ic.h"
#include "nvram_test_device.h"

/** [ESI_DEV_DEFINE_PARAMS] */
#define ESI_DEV_ENDPOINT 1
#define ESI_DEV_CHANNEL_MASK (1L << 22) /* 22 channel. */

/* For MM testing */
#if defined ZB_SUBGHZ_BAND_ENABLED
#define ESI_DEV_CHANNEL_PAGE ZB_CHANNEL_PAGE28_SUB_GHZ
#define ESI_DEV_CHANNEL_MASK1 (1L << 1) /* 1 channel page 28. */
#else
#define ESI_DEV_CHANNEL_PAGE ZB_CHANNEL_PAGE0_2_4_GHZ
#define ESI_DEV_CHANNEL_MASK1 ESI_DEV_CHANNEL_MASK
#endif

#define ESI_PERMIT_JOINING_DURATION_TIME 240
/** [ESI_DEV_DEFINE_PARAMS] */
#define ESI_KEEP_ALIVE_BASE 60  /* in minutes */
#define ESI_KEEP_ALIVE_JITTER 5 /* in minutes */
#define ESI_TIME_STATUS_INITIAL ((1 << ZB_ZCL_TIME_MASTER) | (1 << ZB_ZCL_TIME_MASTER_ZONE_DST))

#define ESI_LOAD_CONTROL_EVENT_GROUP_TIMEOUT 120 * ZB_TIME_ONE_SECOND
#define ESI_LOAD_CONTROL_EVENT_TIMEOUT 5 * ZB_TIME_ONE_SECOND

#define ESI_PRICE_ACK_TIMEOUT (1 * ZB_TIME_ONE_SECOND)

#define ESI_TUNNELING_CLOSE_TUNNEL_TIMEOUT 10 /* in sec */
#define ESI_TUNNELING_PAYLOAD_SIZE 10


/** Disables PriceAck bit in 'price_control'.
 * Can be used for testing device behaviour with clients which
 * don't support SE 1.1 and later standards.
 */
#define ESI_DISABLE_PRICE_ACK 0

/* IA:TODO: WIP */
#define ESI_ENABLE_CALENDAR_CLUSTER 1

/** [DECLARE_CLUSTERS] */
/*********************  Clusters' attributes  **************************/

/** @struct esi_dev_price_attrs_s
 *  @brief Price cluster attributes
 */
typedef struct esi_dev_price_attrs_s
{
  zb_uint8_t commodity_type;
} esi_dev_price_attrs_t;

/** GetScheduledPrices states.
 */
typedef enum esi_dev_get_sched_prices_state_e {
  ESI_DEV_GET_SCHED_PRICES_STATE_INIT,
  ESI_DEV_GET_SCHED_PRICES_STATE_WAIT_PRICE_ACK,
  ESI_DEV_GET_SCHED_PRICES_STATE_FINI = ESI_DEV_GET_SCHED_PRICES_STATE_INIT,
} esi_dev_get_sched_prices_state_t;

typedef struct esi_dev_price_ctx_t {
  zb_addr_u cli_addr;
  zb_aps_addr_mode_t cli_addr_mode;
  zb_uint8_t cli_ep;

  zb_uint8_t nr_events;
  zb_uint8_t curr_event;

  zb_zcl_price_publish_price_payload_t pp_pl;

  esi_dev_get_sched_prices_state_t state;

  zb_bool_t price_ack_disabled;
} esi_dev_price_ctx_t;

#define ESI_DEV_PRICE_CTX() (g_dev_ctx.price_ctx)

/** @struct esi_dev_ctx_s
 *  @brief ESI device context
 */
typedef struct esi_dev_ctx_s
{
  zb_zcl_basic_attrs_t basic_attrs;
  zb_zcl_kec_attrs_t kec_attrs;
  zb_zcl_time_attrs_t time_attrs;
  zb_zcl_keep_alive_attrs_t keep_alive_attrs;
#ifdef ZB_SUBGHZ_BAND_ENABLED
  zb_zcl_sub_ghz_attrs_t sub_ghz_attrs;
#endif
  zb_zcl_tunneling_attrs_t tunneling_attrs;
  esi_dev_price_attrs_t price_attrs;
  zb_addr_u drlc_client_address;
  zb_uint8_t drlc_client_ep;
  zb_addr_u calendar_client_address;
  zb_uint8_t calendar_client_ep;
  esi_dev_price_ctx_t price_ctx;
} esi_dev_ctx_t;

/* device context */
static esi_dev_ctx_t g_dev_ctx;

/* Basic cluster attributes */
ZB_ZCL_DECLARE_BASIC_ATTR_LIST(basic_attr_list, g_dev_ctx.basic_attrs);

/* Key Establishment cluster attributes */
ZB_ZCL_DECLARE_KEC_ATTR_LIST(kec_attr_list, g_dev_ctx.kec_attrs);

/* Price cluster attributes */
#if ESI_USE_ALL_PRICE_CLUSTER_ATTRIBUTES
/* NOTE:
 * 1. There are 998 attributes.
 * 2. (sizeof(price_attr_values) + sizeof(price_attr_list)) == 17304 bytes
 */
zb_zcl_price_attr_values_t price_attr_values = ZB_ZCL_PRICE_ATTR_VALUES_INIT;
ZB_ZCL_DECLARE_PRICE_SRV_ALL_ATTR_LIST(price_attr_list, &price_attr_values);
#else
ZB_ZCL_DECLARE_PRICE_SRV_ATTR_LIST(price_attr_list, &g_dev_ctx.price_attrs.commodity_type);
#endif

/* Time cluster attributes */
ZB_ZCL_DECLARE_TIME_ATTR_LIST(time_attr_list, g_dev_ctx.time_attrs);

/* Keep-Alive cluster attributes */
ZB_ZCL_DECLARE_KEEP_ALIVE_ATTR_LIST(keep_alive_attr_list, g_dev_ctx.keep_alive_attrs);

#ifdef ZB_SUBGHZ_BAND_ENABLED
  ZB_ZCL_DECLARE_SUBGHZ_SRV_ATTR_LIST(sub_ghz_attr_list, g_dev_ctx.sub_ghz_attrs);
#else
#define sub_ghz_attr_list 0
#endif

/* Tunneling cluster attributes */
ZB_ZCL_DECLARE_TUNNELING_ATTR_LIST(tunneling_attr_list, g_dev_ctx.tunneling_attrs);

/*********************  Device declaration  **************************/

ZB_SE_DECLARE_NVRAM_TEST_INTERFACE_DEV_CLUSTER_LIST(esi_dev_clusters,
                                                        basic_attr_list,
                                                        kec_attr_list,
                                                        price_attr_list,
                                                        time_attr_list,
                                                        keep_alive_attr_list,
                                                        tunneling_attr_list,
                                                        sub_ghz_attr_list
                                                      );

ZB_SE_DECLARE_NVRAM_TEST_INTERFACE_DEV_EP(esi_dev_ep, ESI_DEV_ENDPOINT, esi_dev_clusters);

ZB_SE_DECLARE_NVRAM_TEST_INTERFACE_DEV_CTX(esi_dev_zcl_ctx, esi_dev_ep);

/** [DECLARE_CLUSTERS] */


void esi_dev_ctx_init();
void esi_dev_clusters_attrs_init(zb_uint8_t param);
void esi_dev_app_init(zb_uint8_t param);

/** Example of MDU list, fill it if you want to use MDU cluster */
zb_ieee_addr_t g_mdu[] = {/*{0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33},
                            {0xbb,0xbb,0xbb,0xbb,0xbb,0xbb,0xbb,0xbb},*/
                            {0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa}
};

/*********************  Testing functions          **************************/
typedef struct {
  zb_uint16_t cluster_id;
  zb_uint8_t dst_endp;
  zb_ieee_addr_t ieee_addr;
} nvram_testing_bind_ctx_t;

typedef struct configure_reporting_params_s
{
  zb_uint16_t interval_left, interval_right;
  zb_uint16_t attr_id;
  zb_uint8_t attr_type;
  zb_uint16_t cluster_id;
} configure_reporting_params_t;

static void nvram_testing_action_bind(zb_uint8_t param);
static void nvram_testing_action_rejoin(zb_uint8_t param);
static void nvram_testing_action_configure_reporting(zb_uint8_t param);
static void se_send_configure_reporting(zb_uint8_t param,
                                             configure_reporting_params_t *config_params,
                                             zb_callback_t cb);

#define ZB_NVRAM_TEST_RANDOM_IEEE_ADDR_COUNT 10

static zb_bool_t gs_ongoing_rejoin = ZB_FALSE;
static zb_uint16_t gs_zed_short_addr;
static zb_ieee_addr_t gs_zed_ieee_addr;
static zb_ieee_addr_t gs_random_ieee_addrs[ZB_NVRAM_TEST_RANDOM_IEEE_ADDR_COUNT];
static zb_bool_t gs_random_ieee_addrs_generated = ZB_FALSE;

zb_callback_t nvram_testing_actions[] = {
  nvram_testing_action_bind,
  nvram_testing_action_rejoin,
  nvram_testing_action_configure_reporting
};

nvram_testing_bind_ctx_t nvram_testing_bind_ctx;

static void generate_random_ieee_addrs()
{
  zb_int_t i, j;
  for (i = 0; i < ZB_NVRAM_TEST_RANDOM_IEEE_ADDR_COUNT; i++)
  {
    for (j = 0; j < 8; j++)
    {
      gs_random_ieee_addrs[i][j] = zb_random() % 256;
    }
  }
}

static void schedule_random_test_action(zb_uint8_t param)
{
  zb_uint8_t actions_total = sizeof(nvram_testing_actions) / sizeof(nvram_testing_actions[0]);
  zb_uint8_t rnd  = zb_random() % actions_total;

  TRACE_MSG(TRACE_APP1, "scheduled next: %hd, total: %hd", (FMT__H_H, rnd, actions_total));
  ZB_SCHEDULE_ALARM(nvram_testing_actions[rnd], param, ZB_TIME_ONE_SECOND * 4);
}

static void fill_bind_req_from_ctx(zb_zdo_bind_req_param_t *req)
{
  ZB_MEMCPY(&req->src_address, gs_zed_ieee_addr, sizeof(zb_ieee_addr_t));
  req->src_endp = 1;
  req->cluster_id = nvram_testing_bind_ctx.cluster_id;
  req->dst_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
  ZB_MEMCPY(&req->dst_address.addr_long,
            (zb_uint8_t*)&nvram_testing_bind_ctx.ieee_addr,
            sizeof(zb_ieee_addr_t));
  req->dst_endp = nvram_testing_bind_ctx.dst_endp;
  req->req_dst_addr = gs_zed_short_addr;
}

static void nvram_testing_action_unbind_callback(zb_uint8_t param)
{
  zb_ret_t ret = RET_OK;
  zb_buf_t *buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);

  zb_zdo_bind_resp_t *bind_resp = (zb_zdo_bind_resp_t*)ZB_BUF_BEGIN(buf);

  TRACE_MSG(TRACE_APP1, "nvram_testing_action_unbind_callback %hd (status %hd)", (FMT__H_H, ret, bind_resp->status));
  if (bind_resp->status == ZB_ZDP_STATUS_SUCCESS)
  {
    /* unbind ok, now switch to another random task */
    schedule_random_test_action(param);
  }
}

static void nvram_testing_action_bind_callback(zb_uint8_t param)
{
  zb_ret_t ret = RET_OK;
  zb_buf_t *buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);

  zb_zdo_bind_resp_t *bind_resp = (zb_zdo_bind_resp_t*)ZB_BUF_BEGIN(buf);

  TRACE_MSG(TRACE_APP1, "nvram_testing_action_bind_callback %hd (status %hd)", (FMT__H_H, ret, bind_resp->status));
  if (bind_resp->status == ZB_ZDP_STATUS_SUCCESS || bind_resp->status == ZB_ZDP_STATUS_TIMEOUT)
  {
    // bind ok
    zb_zdo_bind_req_param_t *req;
    req = ZB_GET_BUF_PARAM(buf, zb_zdo_bind_req_param_t);
    fill_bind_req_from_ctx(req);

    zb_zdo_unbind_req(param, nvram_testing_action_unbind_callback);
  }
}


static void nvram_testing_action_bind(zb_uint8_t param)
{
  zb_zdo_bind_req_param_t *req;
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);

  TRACE_MSG(TRACE_APP1, "nvram_testing_action_bind %hd", (FMT__H, param));

  nvram_testing_bind_ctx.cluster_id = ZB_ZCL_CLUSTER_ID_KEEP_ALIVE;
  nvram_testing_bind_ctx.dst_endp = zb_random() % 50 + 1;

  if (!gs_random_ieee_addrs_generated)
  {
    generate_random_ieee_addrs();
    gs_random_ieee_addrs_generated = ZB_TRUE;
  }

  ZB_IEEE_ADDR_COPY(
    nvram_testing_bind_ctx.ieee_addr,
    gs_random_ieee_addrs[zb_random() % ZB_NVRAM_TEST_RANDOM_IEEE_ADDR_COUNT]);

  req = ZB_GET_BUF_PARAM(buf, zb_zdo_bind_req_param_t);
  fill_bind_req_from_ctx(req);

  zb_zdo_bind_req(param, nvram_testing_action_bind_callback);
}

static void nvram_testing_action_rejoin_callback(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, "nvram_testing_action_rejoin_callback %hd", (FMT__H, param));
  zb_free_buf(ZB_BUF_FROM_REF(param));
//  schedule_random_test_action(param);
}

static void nvram_testing_action_rejoin(zb_uint8_t param)
{
  zb_zdo_mgmt_leave_param_t *leave_req;
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);

  TRACE_MSG(TRACE_APP1, "nvram_testing_action_rejoin (param %hd) ", (FMT__H, param));

  gs_ongoing_rejoin = ZB_TRUE;

  leave_req = ZB_GET_BUF_PARAM(buf, zb_zdo_mgmt_leave_param_t);
  leave_req->dst_addr = gs_zed_short_addr;
  ZB_MEMSET(leave_req->device_address, 0, sizeof(leave_req->device_address));
  leave_req->remove_children = ZB_FALSE;
  leave_req->rejoin = ZB_TRUE;
  zdo_mgmt_leave_req(param, nvram_testing_action_rejoin_callback);
}

static void nvram_testing_action_configure_reporting(zb_uint8_t param)
{
  configure_reporting_params_t configure_params;

  TRACE_MSG(TRACE_APP1, ">> nvram_testing_action_configure_reporting param %hd", (FMT__H, param));

  configure_params.interval_left = zb_random() % 50;
  configure_params.interval_right = 60 + zb_random() % 50;
  configure_params.attr_id = ZB_ZCL_ATTR_METERING_STATUS_ID;
  configure_params.attr_type = ZB_ZCL_ATTR_TYPE_8BITMAP;
  configure_params.cluster_id = ZB_ZCL_CLUSTER_ID_METERING;

  se_send_configure_reporting(param, &configure_params, schedule_random_test_action);
}


static void se_send_configure_reporting(zb_uint8_t param,
                                      configure_reporting_params_t *config_params,
                                      zb_callback_t cb)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_uint8_t *cmd_ptr;
  zb_uint8_t delta[8];

  TRACE_MSG(TRACE_APP1, ">> se_send_configure_reporting param %hd", (FMT__H, param));

  ZB_ZCL_GENERAL_INIT_CONFIGURE_REPORTING_SRV_REQ(buf,
                                                  cmd_ptr,
                                                  ZB_ZCL_ENABLE_DEFAULT_RESPONSE);

  ZB_MEMSET(delta, 0, sizeof(delta));

  ZB_ZCL_GENERAL_ADD_SEND_REPORT_CONFIGURE_REPORTING_REQ(
    cmd_ptr,
    config_params->attr_id,
    config_params->attr_type,
    config_params->interval_left,
    config_params->interval_right,
    (zb_uint8_t *)&delta);

  ZB_ZCL_GENERAL_SEND_CONFIGURE_REPORTING_REQ(
    buf,
    cmd_ptr,
    gs_zed_short_addr,
    ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
    1,
    ESI_DEV_ENDPOINT,
    ZB_AF_SE_PROFILE_ID,
    config_params->cluster_id,
    cb);

  TRACE_MSG(TRACE_APP2, "<< configure_reporting", (FMT__0));
}

static void child_nwk_addr_resp(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_zdo_nwk_addr_resp_head_t *resp = (zb_zdo_nwk_addr_resp_head_t*) ZB_BUF_BEGIN(buf);

  TRACE_MSG(TRACE_APP1, ">>child_nwk_addr_resp, param = %d, status = %d",
            (FMT__D_D, param, resp->status));

  if (resp->status == ZB_ZDP_STATUS_SUCCESS)
  {
    configure_reporting_params_t configure_params;

    ZB_LETOH16(&gs_zed_short_addr, &resp->nwk_addr);

    TRACE_MSG(TRACE_APP1, "ret short %d", (FMT__D, gs_zed_short_addr));

    configure_params.interval_left = 10;
    configure_params.interval_right = 60;
    configure_params.attr_id = ZB_ZCL_ATTR_METERING_CURRENT_SUMMATION_DELIVERED_ID;
    configure_params.attr_type = ZB_ZCL_ATTR_TYPE_U48;
    configure_params.cluster_id = ZB_ZCL_CLUSTER_ID_METERING;

    se_send_configure_reporting(param, &configure_params, schedule_random_test_action);
  }
  else
  {
    zb_free_buf(buf);
  }

  TRACE_MSG(TRACE_APP1, "<<child_nwk_addr_resp", (FMT__0));
}

static void child_nwk_addr_req(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_zdo_nwk_addr_req_param_t *req_param = ZB_GET_BUF_PARAM(buf, zb_zdo_nwk_addr_req_param_t);

  req_param->dst_addr = 0xFFFF;
  req_param->start_index = 0;
  req_param->request_type = ZB_ZDO_SINGLE_DEVICE_RESP;
  ZB_IEEE_ADDR_COPY(req_param->ieee_addr, gs_zed_ieee_addr);
  zb_zdo_nwk_addr_req(param, child_nwk_addr_resp);

  TRACE_MSG(TRACE_APP1, "<<child_nwk_addr_req", (FMT__0));
}




/*********************  Device-specific functions  **************************/

/** Init device context. */
void esi_dev_ctx_init()
{
  TRACE_MSG(TRACE_APP1, ">> esi_dev_ctx_init", (FMT__0));

  ZB_MEMSET(&g_dev_ctx, 0, sizeof(g_dev_ctx));
  ESI_DEV_PRICE_CTX().price_ack_disabled = ESI_DISABLE_PRICE_ACK;

  TRACE_MSG(TRACE_APP1, "<< esi_dev_ctx_init", (FMT__0));
}

/** Init device ZCL attributes. */
void esi_dev_clusters_attrs_init(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> esi_dev_clusters_attrs_init", (FMT__0));
  ZVUNUSED(param);

  g_dev_ctx.basic_attrs.zcl_version = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
  g_dev_ctx.basic_attrs.power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

  g_dev_ctx.kec_attrs.kec_suite = ZB_KEC_SUPPORTED_CRYPTO_ATTR;

  g_dev_ctx.price_attrs.commodity_type = ZB_ZCL_METERING_ELECTRIC_METERING;

  g_dev_ctx.time_attrs.time = ZB_ZCL_TIME_TIME_INVALID_VALUE;
  g_dev_ctx.time_attrs.time_status = ESI_TIME_STATUS_INITIAL;
  g_dev_ctx.time_attrs.time_zone = ZB_ZCL_TIME_TIME_ZONE_DEFAULT_VALUE;
  g_dev_ctx.time_attrs.dst_start = ZB_ZCL_TIME_TIME_INVALID_VALUE;
  g_dev_ctx.time_attrs.dst_end = ZB_ZCL_TIME_TIME_INVALID_VALUE;
  g_dev_ctx.time_attrs.dst_shift = ZB_ZCL_TIME_DST_SHIFT_DEFAULT_VALUE;
  g_dev_ctx.time_attrs.standard_time = ZB_ZCL_TIME_TIME_INVALID_VALUE;
  g_dev_ctx.time_attrs.local_time = ZB_ZCL_TIME_TIME_INVALID_VALUE;
  g_dev_ctx.time_attrs.last_set_time = ZB_ZCL_TIME_LAST_SET_TIME_DEFAULT_VALUE;
  g_dev_ctx.time_attrs.valid_until_time = ZB_ZCL_TIME_VALID_UNTIL_TIME_DEFAULT_VALUE;

  g_dev_ctx.keep_alive_attrs.base = ESI_KEEP_ALIVE_BASE;
  g_dev_ctx.keep_alive_attrs.jitter = ESI_KEEP_ALIVE_JITTER;

  g_dev_ctx.tunneling_attrs.close_tunnel_timeout = ESI_TUNNELING_CLOSE_TUNNEL_TIMEOUT;

  TRACE_MSG(TRACE_APP1, "<< esi_dev_clusters_attrs_init", (FMT__0));
}


/** [esi_tunneling_tx_resp] */
void esi_tunneling_tx_resp(zb_uint8_t param, zb_uint16_t user_param)
{
  zb_uint8_t tunneling_data[ESI_TUNNELING_PAYLOAD_SIZE];
  zb_uint8_t i;

  for (i = 0; i < ZB_ARRAY_SIZE(tunneling_data); ++i)
  {
    tunneling_data[i] = i + 10;
  }

  if (ZB_ZCL_TUNNELING_SERVER_SEND_TRANSFER_DATA(param,
                                          ESI_DEV_ENDPOINT,
                                          ZB_AF_SE_PROFILE_ID,
                                          ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                                          NULL, /* no callback */
                                          user_param,
                                          ZB_ARRAY_SIZE(tunneling_data) * sizeof(zb_uint8_t),
                                          tunneling_data) != RET_OK)
  {
    ZB_FREE_BUF_BY_REF(param);
  }
}
/** [esi_tunneling_tx_resp] */



const char message[] = "The display string, The quick brown fox jumps over the lazy dog";

/** [handle_get_last_message] */
static void handle_get_last_message(zb_zcl_messaging_get_last_message_response_t *resp)
{
  resp->resp.display_message.message = (zb_uint8_t *)message;
  resp->resp.display_message.message_len = sizeof(message)-1;

  resp->resp_type = ZB_ZCL_MESSAGING_RESPONSE_TYPE_NORMAL;
  resp->resp.display_message.duration_in_minutes = 0;
  resp->resp.display_message.extended_message_control = 0;
  resp->resp.display_message.start_time = 0;
  resp->resp.display_message.message_id = zb_get_utc_time();
  resp->resp.display_message.message_control = 0;
}
/** [handle_get_last_message] */

/** Application callback for ZCL commands. Used for:
    - receiving application-specific values for commands (e.g. ZB_ZCL_PRICE_GET_CURRENT_PRICE_CB_ID)
    - providing received ZCL commands data to application (e.g. ZB_ZCL_DRLC_REPORT_EVENT_STATUS_CB_ID)
    Application may ignore callback id-s in which it is not interested.
 */
static void esi_zcl_cmd_device_cb(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> esi_zcl_cmd_device_cb(param=%hd, id=%d)",
            (FMT__H_D, param, ZB_ZCL_DEVICE_CMD_PARAM_CB_ID(param)));

  switch (ZB_ZCL_DEVICE_CMD_PARAM_CB_ID(param))
  {
    case ZB_ZCL_TUNNELING_REQUEST_TUNNEL_CB_ID:
    {
      const zb_zcl_tunneling_request_tunnel_t *req =
        ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_tunneling_request_tunnel_t);
      zb_uint8_t *tunnel_status = ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(param, zb_uint8_t);

      TRACE_MSG(TRACE_APP1, "ZB_ZCL_TUNNELING_REQUEST_TUNNEL_CB_ID", (FMT__0));

      /* Check protocol, flow control etc. */
      if (req->protocol_id != ZB_ZCL_TUNNELING_PROTOCOL_MANUFACTURER_DEFINED)
      {
        *tunnel_status = ZB_ZCL_TUNNELING_STATUS_PROTOCOL_NOT_SUPPORTED;
      }
      else if (req->flow_control_support != ZB_FALSE)
      {
        *tunnel_status = ZB_ZCL_TUNNELING_STATUS_FLOW_CONTROL_NOT_SUPPORTED;
      }
      else
      {
        *tunnel_status = ZB_ZCL_TUNNELING_STATUS_SUCCESS;
      }
      ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
    }
    break;
    case ZB_ZCL_TUNNELING_TRANSFER_DATA_CLI_CB_ID:
    {
      const zb_zcl_tunneling_transfer_data_payload_t *tr_data =
        ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_tunneling_transfer_data_payload_t);
      TRACE_MSG(TRACE_APP1, "ZB_ZCL_TUNNELING_TRANSFER_DATA_CLI_CB_ID", (FMT__0));
      DUMP_TRAF("recv:", tr_data->tun_data, tr_data->data_size, 0);
      ZB_GET_OUT_BUF_DELAYED2(esi_tunneling_tx_resp, tr_data->hdr.tunnel_id);
      ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
    }
    break;
    case ZB_ZCL_TUNNELING_CLOSE_TUNNEL_CB_ID:
    {
      const zb_zcl_tunneling_close_tunnel_t *close_tunnel =
        ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_tunneling_close_tunnel_t);
      TRACE_MSG(TRACE_APP1, "ZB_ZCL_TUNNELING_CLOSE_TUNNEL_CB_ID: tunnel_id %d",
                (FMT__D, close_tunnel->tunnel_id));
      ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
    }
    break;
    case ZB_ZCL_MESSAGING_GET_LAST_MSG_CB_ID:
    {
        TRACE_MSG(TRACE_APP1, "ZB_ZCL_MESSAGING_GET_LAST_MSG_CB_ID", (FMT__0));
        ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
        handle_get_last_message(ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(param, zb_zcl_messaging_get_last_message_response_t));
    }
    break;
    default:
      TRACE_MSG(TRACE_APP1, "Undefined callback was called, id=%d",
                (FMT__D, ZB_ZCL_DEVICE_CMD_PARAM_CB_ID(param)));
      break;
  }

  TRACE_MSG(TRACE_APP1, "<< esi_zcl_cmd_device_cb", (FMT__0));
}

/** [ESI_DEV_INIT] */
/* Application init: configure stack parameters (IEEE address, device role, channel mask etc),
 * register device context and ZCL cmd device callback), init application attributes/context. */
void esi_dev_app_init(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> esi_dev_app_init", (FMT__0));
/** [ESI_DEV_INIT] */
  ZVUNUSED(param);

  /** [REGISTER_DEVICE_CTX] */
  ZB_AF_REGISTER_DEVICE_CTX(&esi_dev_zcl_ctx);
  /** [REGISTER_DEVICE_CTX] */

  zb_osif_led_button_init();

  /* reset nvram when both buttons are pressed */
  if (zb_osif_button_state(BUTTON_LEFT) && zb_osif_button_state(BUTTON_RIGHT))
  {
     zb_se_start_nvram_erase_indication();
     zb_set_nvram_erase_at_start(ZB_TRUE);
     zb_se_stop_nvram_erase_indication();
  }

  /* device configuration */
  esi_dev_ctx_init();
  esi_dev_clusters_attrs_init(0);
  zb_register_zboss_callback(ZB_ZCL_DEVICE_CB, SET_ZBOSS_CB(esi_zcl_cmd_device_cb));

  /* ZB configuration */
#ifdef ENABLE_RUNTIME_APP_CONFIG
  zb_set_long_address(g_dev_addr);
#ifdef ENABLE_PRECOMMISSIONED_REJOIN
  zb_set_use_extended_pan_id(&g_ext_pan_id);
#endif
#endif
#if defined ZB_SUBGHZ_BAND_ENABLED
  {
    zb_channel_list_t channel_list;
    zb_channel_list_init(channel_list);
    zb_channel_list_add(channel_list, ESI_DEV_CHANNEL_PAGE, ESI_DEV_CHANNEL_MASK1);
    TRACE_MSG(TRACE_APP1, "ZC in MM mode start: page %d mask 0x%x", (FMT__D_D, ESI_DEV_CHANNEL_PAGE, ESI_DEV_CHANNEL_MASK1));
    zb_se_set_network_coordinator_role_select_device(channel_list);
  }
#else
/** [ESI_DEV_SET_ROLE] */
  zb_se_set_network_coordinator_role(ESI_DEV_CHANNEL_MASK);
/** [ESI_DEV_SET_ROLE] */
#endif
#ifdef ZB_SE_BDB_MIXED
  zb_se_set_bdb_mode_enabled(1);
#endif
  TRACE_MSG(TRACE_APP1, "<< esi_dev_app_init", (FMT__0));
}

/*********************  SE ESI ZC  **************************/

MAIN()
{
  ARGV_UNUSED;

  ZB_SET_TRACE_ON();
  ZB_SET_TRAF_DUMP_OFF();

  ZB_INIT("nvram_test_device");

  esi_dev_app_init(0);

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


/** [PERMIT_JOINING] */
/* Left button handler: permit join for 240 sec (ESI_PERMIT_JOINING_DURATION_TIME). */
static void esi_dev_left_button_handler(zb_uint8_t param)
{
  ZVUNUSED(param);

  zb_se_indicate_permit_joining_on(ESI_PERMIT_JOINING_DURATION_TIME);
  zb_se_permit_joining(ESI_PERMIT_JOINING_DURATION_TIME);
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
/** [SIGNAL_HANDLER_GET_SIGNAL] */
      case ZB_ZDO_SIGNAL_SKIP_STARTUP:
        zb_secur_ic_str_add(ihd_dev_addr, ihd_installcode, NULL);
        zb_secur_ic_str_add(el_metering_dev_addr, el_metering_installcode, NULL);
        zb_secur_ic_str_add(gas_metering_dev_addr, gas_metering_installcode, NULL);
        zb_secur_ic_str_add(pct_dev_addr, pct_installcode, NULL);
#ifdef ADDITIONAL_DEV_ADDR
        zb_secur_ic_str_add(add_dev_addr, add_installcode, NULL);
#endif
#ifdef ENABLE_RUNTIME_APP_CONFIG
/** [SIGNAL_HANDLER_LOAD_CERT] */
#ifdef SE_CRYPTOSUITE_1
        zb_se_load_ecc_cert(KEC_CS1, ca_public_key_cs1, esi_certificate_cs1, esi_private_key_cs1);
#endif
#ifdef SE_CRYPTOSUITE_2
        zb_se_load_ecc_cert(KEC_CS2, ca_public_key_cs2, esi_certificate_cs2, esi_private_key_cs2);
#endif
/** [SIGNAL_HANDLER_LOAD_CERT] */
#endif /* ENABLE_RUNTIME_APP_CONFIG */
        zboss_start_continue();
        break;

      case ZB_SIGNAL_DEVICE_FIRST_START:
      case ZB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APP1, "Device STARTED OK: first_start %hd",
                  (FMT__H, sig == ZB_SIGNAL_DEVICE_FIRST_START));
#ifdef ZB_USE_BUTTONS
        zb_button_register_handler(BUTTON_LEFT, 0, esi_dev_left_button_handler);
#else
        esi_dev_left_button_handler(0);
#endif
        break;

/** [SIGNAL_HANDLER_DISCOVERY_OK] */
      case ZB_SE_SIGNAL_SERVICE_DISCOVERY_OK:
        TRACE_MSG(TRACE_APP1, "Service Discovery OK", (FMT__0));
        zb_se_indicate_commissioning_stopped();

        ZB_GET_OUT_BUF_DELAYED(child_nwk_addr_req);
        break;
/** [SIGNAL_HANDLER_DISCOVERY_OK] */

/** [SIGNAL_HANDLER_DO_BIND] */
      case ZB_SE_SIGNAL_SERVICE_DISCOVERY_DO_BIND:
      {
        zb_se_signal_service_discovery_bind_params_t *bind_params =
          ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_se_signal_service_discovery_bind_params_t);

        TRACE_MSG(TRACE_APP1, "can bind cluster 0x%x commodity_type %d remote_dev " TRACE_FORMAT_64,
                  (FMT__D_D_A, bind_params->cluster_id, bind_params->commodity_type,
                   TRACE_ARG_64(bind_params->device_addr)));

        if (bind_params->cluster_id == ZB_ZCL_CLUSTER_ID_METERING
           || bind_params->cluster_id == ZB_ZCL_CLUSTER_ID_DRLC
           || bind_params->cluster_id == ZB_ZCL_CLUSTER_ID_MESSAGING
           || bind_params->cluster_id == ZB_ZCL_CLUSTER_ID_PREPAYMENT)
        {
          zb_se_service_discovery_bind_req(param, bind_params->device_addr, bind_params->endpoint);
          param = 0;
        }

      }
      break;
/** [SIGNAL_HANDLER_BIND_DEV] */

/** [SIGNAL_HANDLER_BIND_OK] */
      case ZB_SE_SIGNAL_SERVICE_DISCOVERY_BIND_OK:
      {
        zb_uint16_t *addr = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_uint16_t);
        TRACE_MSG(TRACE_APP1, "Bind dev 0x%x OK", (FMT__D, *addr));
        break;
      }
/** [SIGNAL_HANDLER_BIND_OK] */

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
/** [ZB_ZDO_SIGNAL_DEVICE_ANNCE] */
      case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
        {
          if (gs_ongoing_rejoin)
          {
            gs_ongoing_rejoin = ZB_FALSE;
            TRACE_MSG(TRACE_APP1, "Device annce; rejoin completed ", (FMT__0));
            ZB_GET_OUT_BUF_DELAYED(schedule_random_test_action);
          }
        }
        break;
/** [SIGNAL_HANDLER_TC_SIGNAL_CHILD_JOIN] */
      case ZB_SE_TC_SIGNAL_CHILD_JOIN_CBKE:
        {
          zb_ieee_addr_t *remote_device_addr = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_ieee_addr_t);
          TRACE_MSG(TRACE_APP1, "Child " TRACE_FORMAT_64 " joined & established TCLK",
                    (FMT__A, TRACE_ARG_64(remote_device_addr)));
#ifdef DEBUG_EXPOSE_KEYS
          {
            zb_uint8_t key[ZB_CCM_KEY_SIZE];
            if (zb_se_debug_get_link_key_by_long(*remote_device_addr, key) == RET_OK)
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

          TRACE_MSG(TRACE_APP1, "Start Service Discovery", (FMT__0));
          zb_se_service_discovery_start(ESI_DEV_ENDPOINT);
          zb_se_indicate_service_discovery_started();

          ZB_IEEE_ADDR_COPY(gs_zed_ieee_addr, remote_device_addr);
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
    zb_free_buf(ZB_BUF_FROM_REF(param));
  }

  TRACE_MSG(TRACE_APP1, "<< zboss_signal_handler", (FMT__0));
}
/** [SIGNAL_HANDLER] */
