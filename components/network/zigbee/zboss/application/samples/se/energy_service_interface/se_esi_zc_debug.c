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

#define ZB_TRACE_FILE_ID 40028

#include "zboss_api.h"
#include "zb_led_button.h"

#include "../common/se_common.h"
#include "../common/se_indication.h"

#define ESI_USE_ALL_PRICE_CLUSTER_ATTRIBUTES 1

#if ESI_USE_ALL_PRICE_CLUSTER_ATTRIBUTES
#include "../common/price_srv_attr_sets.h"
#endif

#ifdef ENABLE_RUNTIME_APP_CONFIG
#include "../common/se_cert.h"
static zb_ieee_addr_t g_dev_addr = ESI_DEV_ADDR;
#ifdef ENABLE_PRECOMMISSIONED_REJOIN
static zb_ext_pan_id_t g_ext_pan_id = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
#endif
#endif

#include "../common/se_ic.h"

/** [ESI_DEV_DEFINE_PARAMS] */
#define ESI_DEV_ENDPOINT 1
#define ESI_DEV_CHANNEL_MASK (1L << 22) /* 22 channel. */

/* For MM testing */
#ifdef ZB_SUBGHZ_BAND_ENABLED
#define ESI_DEV_CHANNEL_PAGE_I 1
#define ESI_DEV_CHANNEL_MASK1 (1L << 1) /* 1 channel page 28. */
#else
#define ESI_DEV_CHANNEL_PAGE_I 0
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
  ESI_DEV_GET_SCHED_PRICES_STATE_WAIT_APS_ACK,
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

#define ESI_DEV_PRICE_CTX() (&g_dev_ctx.price_ctx)

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
/* FIXME:
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

ZB_SE_DECLARE_ENERGY_SERVICE_INTERFACE_DEV_CLUSTER_LIST(esi_dev_clusters,
                                                        basic_attr_list,
                                                        kec_attr_list,
                                                        price_attr_list,
                                                        time_attr_list,
                                                        keep_alive_attr_list,
                                                        tunneling_attr_list,
                                                        sub_ghz_attr_list
                                                      );

ZB_SE_DECLARE_ENERGY_SERVICE_INTERFACE_DEV_EP(esi_dev_ep, ESI_DEV_ENDPOINT, esi_dev_clusters);

ZB_SE_DECLARE_ENERGY_SERVICE_INTERFACE_DEV_CTX(esi_dev_zcl_ctx, esi_dev_ep);

/** [DECLARE_CLUSTERS] */


void esi_dev_ctx_init();
void esi_dev_clusters_attrs_init(zb_uint8_t param);
void esi_dev_app_init(zb_uint8_t param);

zb_ieee_addr_t g_mdu[] = {//{0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33},
                          //{0xbb,0xbb,0xbb,0xbb,0xbb,0xbb,0xbb,0xbb},
                          {0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa}
//                          ,{5,5,5,5,5,5,5,5},{6,6,6,6,6,6,6,6},{3,3,3,3,3,3,3,3},
//                          {7,7,7,7,7,7,7,7},{8,8,8,8,8,8,8,8},{9,9,9,9,9,9,9,9},

};
/*********************  Device-specific functions  **************************/

/** Init device context. */
void esi_dev_ctx_init()
{
  TRACE_MSG(TRACE_APP1, ">> esi_dev_ctx_init", (FMT__0));

  ZB_MEMSET(&g_dev_ctx, 0, sizeof(g_dev_ctx));
  ESI_DEV_PRICE_CTX()->price_ack_disabled = ESI_DISABLE_PRICE_ACK;

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

  g_dev_ctx.price_attrs.commodity_type = ZB_ZCL_METERING_DEVICE_TYPE_ELECTRIC_METERING;

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

/** Get application parameters for Price - Get Current Price command.
    Now emulate some real values. */
static void esi_get_price_param(zb_zcl_price_publish_price_payload_t *p)
{
  const char *rate_label = "B"; /* "BASE" */
  p->provider_id = 0x01; /* defined by device */
  ZB_ZCL_SET_STRING_VAL(p->rate_label, rate_label, strlen(rate_label));
  p->issuer_event_id = zb_get_utc_time(); /* always increasing */
  p->unit_of_measure = ZB_ZCL_METERING_UNIT_OF_MEASURE_KW_KWH_BINARY;
  p->currency = 840; /* ISO USD */
  p->price_trailing_digit = 0x01; /* e.g, 222.3 */
  p->price_tier = 0x00; /* don't use price tiering */
  p->start_time = 0x00; /* "now" */
  p->duration_in_minutes = 0x05; /* 5 min */
  p->price = ZB_RANDOM_VALUE(24); /* X.Y$ per KW */

  if (ESI_DEV_PRICE_CTX()->price_ack_disabled)
  {
    /* Clear PriceAck bit in payload if theres is not need to recv PriceAck.
     * It can be used to emulate devices with SE version < 1.1.
     */
    ESI_DEV_PRICE_CTX()->pp_pl.price_control &= 0xFE;
  }

}

/* Send PublishPrice command and wait for APS ack (and PriceAck if it is
 * available)
 */
static void esi_get_sched_prices_cb_send_pp(zb_uint8_t param);

/* Wait for PriceAck (or for timeout) and send next portion of PublishPrice
 * event.
 */
static void esi_get_sched_prices_cb_on_sent(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> esi_get_sched_prices_cb_on_sent, "
            "param=%hd, state=%hd",
            (FMT__H_H, param, ESI_DEV_PRICE_CTX()->state));
  ZB_ASSERT(ESI_DEV_PRICE_CTX()->state == ESI_DEV_GET_SCHED_PRICES_STATE_WAIT_APS_ACK);
  ESI_DEV_PRICE_CTX()->state = ESI_DEV_GET_SCHED_PRICES_STATE_WAIT_PRICE_ACK;
  ZB_SCHEDULE_APP_ALARM(esi_get_sched_prices_cb_send_pp, param, ESI_PRICE_ACK_TIMEOUT);
  TRACE_MSG(TRACE_APP1, "<< esi_get_sched_prices_cb_on_sent", (FMT__0));
}

static void esi_get_sched_prices_cb_send_pp(zb_uint8_t param)
{
  if (!param)
  {
    ZB_GET_OUT_BUF_DELAYED(esi_get_sched_prices_cb_send_pp);
    return;
  }

  ESI_DEV_PRICE_CTX()->curr_event++;

  if (ESI_DEV_PRICE_CTX()->nr_events != 0
      && ESI_DEV_PRICE_CTX()->curr_event == ESI_DEV_PRICE_CTX()->nr_events)
  {
    TRACE_MSG(TRACE_APP1, "The last packet was sent.", (FMT__0));
    ESI_DEV_PRICE_CTX()->state = ESI_DEV_GET_SCHED_PRICES_STATE_FINI;
    ZB_FREE_BUF_BY_REF(param);
    /* FIXME: should we send Default Response ? */
    return;
  }
  else
  {
    TRACE_MSG(TRACE_APP1, "Attempt to send packet %hd of %hd",
              (FMT__H_H, ESI_DEV_PRICE_CTX()->curr_event, ESI_DEV_PRICE_CTX()->nr_events));
    ESI_DEV_PRICE_CTX()->state = ESI_DEV_GET_SCHED_PRICES_STATE_WAIT_APS_ACK;
  }

  esi_get_price_param(&ESI_DEV_PRICE_CTX()->pp_pl);

  ZB_ZCL_PRICE_SEND_CMD_PUBLISH_PRICE(param,
   (zb_addr_u *) &ESI_DEV_PRICE_CTX()->cli_addr,
   ESI_DEV_PRICE_CTX()->cli_addr_mode,
   ESI_DEV_PRICE_CTX()->cli_ep,
   ESI_DEV_ENDPOINT,
   &ESI_DEV_PRICE_CTX()->pp_pl, esi_get_sched_prices_cb_on_sent);
}

static void esi_get_current_price_cb(zb_uint8_t param)
{
  ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;

  /* Print command input payload */
  TRACE_MSG(TRACE_APP1, "GetCurrentPrice Command Input Payload %hd", (FMT__H, *ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_price_get_current_price_payload_t)));

  /* Get Price parameters */
  esi_get_price_param(ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(param, zb_zcl_price_publish_price_payload_t));
}

static void esi_get_sched_prices_cb(zb_uint8_t param)
{
  const zb_zcl_price_get_scheduled_prices_payload_t *pl_in =
    ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_price_get_scheduled_prices_payload_t);
  const zb_zcl_parsed_hdr_t                         *cmd_info =
    ZB_ZCL_DEVICE_CMD_PARAM_CMD_INFO(param);


  if (ZB_RANDOM_VALUE(1))
  {
    /* Occasionally, we don't have any events. */
    ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_NOT_FOUND;
    return;
  }

  /* FIXME: only one client device is supported for now. */
  /* FIXME: use long address */
  ESI_DEV_PRICE_CTX()->cli_addr.addr_short = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr;
  ESI_DEV_PRICE_CTX()->cli_addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
  ESI_DEV_PRICE_CTX()->cli_ep = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint;

  ESI_DEV_PRICE_CTX()->nr_events = pl_in->number_of_events;
  ESI_DEV_PRICE_CTX()->curr_event = 0;

  ESI_DEV_PRICE_CTX()->pp_pl = ZB_ZCL_PRICE_PUBLISH_PRICE_PAYLOAD_INIT;

  ESI_DEV_PRICE_CTX()->curr_event = -1;

  ZB_GET_OUT_BUF_DELAYED(esi_get_sched_prices_cb_send_pp);
  ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
}

static void esi_price_ack_cb(zb_uint8_t param)
{
  const zb_zcl_price_ack_payload_t *pl_in =
    ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_price_ack_payload_t);
  TRACE_MSG(TRACE_APP1, ">> esi_price_ack_cb, "
            "issuer_event_id = %d, current_time=%d, provider_id = %d",
            (FMT__D_D_D, pl_in->issuer_event_id, pl_in->current_time,
             pl_in->provider_id));

  /* If we are in a middle of sending GetScheduledPrices responses. */
  if (ESI_DEV_PRICE_CTX()->state == ESI_DEV_GET_SCHED_PRICES_STATE_WAIT_PRICE_ACK)
  {
    /* FIXME: only one client device is supported for now. */
    ZB_SCHEDULE_APP_ALARM_CANCEL(esi_get_sched_prices_cb_send_pp, ZB_ALARM_ALL_CB);
    esi_get_sched_prices_cb_send_pp(0);
  }

  ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
  TRACE_MSG(TRACE_APP1, "<< esi_price_ack_cb", (FMT__0));
}


/** Handle received DRLC Report Event Status command payload.
    Now trace received values. */
static void handle_report_event_status(const zb_zcl_drlc_report_event_status_payload_t *in,
                                            const zb_zcl_parsed_hdr_t *in_cmd_info)
{
  TRACE_MSG(TRACE_APP1, ">> handle_report_event_status(in=%p) <<", (FMT__P, in));
  TRACE_MSG(TRACE_APP1, "in->issuer_event_id = %d", (FMT__D, in->issuer_event_id));
  TRACE_MSG(TRACE_APP1, "in->event_status = %hd", (FMT__H, in->event_status));
  TRACE_MSG(TRACE_APP1, "in->event_status_time = %d", (FMT__D, in->event_status_time));
  TRACE_MSG(TRACE_APP1, "in->criticality_level_applied = %hd", (FMT__H, in->criticality_level_applied));
  TRACE_MSG(TRACE_APP1, "in->cooling_temperature_set_point_applied = %d", (FMT__D, in->cooling_temperature_set_point_applied));
  TRACE_MSG(TRACE_APP1, "in->heating_temperature_set_point_applied = %d", (FMT__D, in->heating_temperature_set_point_applied));
  TRACE_MSG(TRACE_APP1, "in->average_load_adjustment_percentage_applied = %hd", (FMT__H, in->average_load_adjustment_percentage_applied));
  TRACE_MSG(TRACE_APP1, "in->duty_cycle_applied = %hd", (FMT__H, in->duty_cycle_applied));
  TRACE_MSG(TRACE_APP1, "in->event_control = %d", (FMT__D, in->event_control));
  TRACE_MSG(TRACE_APP1, "in->signature_type = %d", (FMT__D, in->signature_type));
  if(DRLC_EVENT_STATUS_LCE_RECEIVED == in->event_status)
  {
    TRACE_MSG(TRACE_APP1, "Got DRLC_EVENT_STATUS_LCE_RECEIVED status from 0x%x", (FMT__D, ZB_ZCL_PARSED_HDR_SHORT_DATA(in_cmd_info).source.u.short_addr));
  }
  if(DRLC_EVENT_STATUS_REJECTED_UNDEFINED_EVENT == in->event_status)
  {
    TRACE_MSG(TRACE_APP1, "Got DRLC_EVENT_STATUS_REJECTED_UNDEFINED_EVENT status from 0x%x", (FMT__D, ZB_ZCL_PARSED_HDR_SHORT_DATA(in_cmd_info).source.u.short_addr));
  }
  if(DRLC_EVENT_STATUS_EVENT_STARTED == in->event_status)
  {
    TRACE_MSG(TRACE_APP1, "Got DRLC_EVENT_STATUS_EVENT_STARTED status from 0x%x", (FMT__D, ZB_ZCL_PARSED_HDR_SHORT_DATA(in_cmd_info).source.u.short_addr));
  }
  if(DRLC_EVENT_STATUS_EVENT_CANCELLED == in->event_status)
  {
    TRACE_MSG(TRACE_APP1, "Got DRLC_EVENT_STATUS_EVENT_CANCELLED status from 0x%x", (FMT__D, ZB_ZCL_PARSED_HDR_SHORT_DATA(in_cmd_info).source.u.short_addr));
  }
}

/** [handle_get_scheduled_events]  */
/** Get application parameters for DRLC - Get Scheduled Events command.
    Now emulate some real values. */
static void handle_get_scheduled_events(
                                const zb_zcl_drlc_get_scheduled_events_payload_t *in,
                                zb_zcl_drlc_lce_payload_t *out)
{
  TRACE_MSG(TRACE_APP1, ">> handle_get_scheduled_events(in=%p, out=%p) <<", (FMT__P_P, in,out));
  TRACE_MSG(TRACE_APP1, "in->start_time = %d", (FMT__D, in->start_time));
  TRACE_MSG(TRACE_APP1, "in->number_of_events(0-no limit) = %hd", (FMT__H, in->number_of_events));
  TRACE_MSG(TRACE_APP1, "in->issuer_event_id = %x", (FMT__D, in->issuer_event_id));
  /*Here we must get events if present and pass them to client*/
  out->issuer_event_id = 0x01;
  out->device_class = DRLC_DEVICE_CLASS_SMART_APPLIANCE;
  out->utility_enrollment_group = 0x88;
  out->start_time = zb_get_utc_time()+3;
  out->duration_in_minutes = 1;
  out->criticality_level = 0x56;
  out->event_control = 0xab;
}
/** [handle_get_scheduled_events]  */

static zb_ret_t handle_get_mdu_events( const zb_zcl_mdu_pairing_request_t *in,
                                       zb_zcl_mdu_pairing_response_t *out)
{
  TRACE_MSG(TRACE_APP1, ">> handle_get_mdu_events(in=%p, out=%p) <<", (FMT__P_P, in,out));
  TRACE_MSG(TRACE_APP1, "in->lpi_version = %d", (FMT__D, in->lpi_version));
  TRACE_MSG(TRACE_APP1, "in->eui64 = " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(in->eui64)));
  /*Here we must get mdu table if present and pass them to client*/
  out->lpi_version = in->lpi_version+1;
  out->total_number_of_devices = sizeof(g_mdu)/sizeof(zb_ieee_addr_t);
  TRACE_MSG(TRACE_APP1, "out->total_number_of_devices = %d", (FMT__D, out->total_number_of_devices));
  //zb_uint8_t     command_index;
  //zb_uint8_t     total_number_of_commands;
  out->eui64=g_mdu;
  return RET_OK;
}

static void esi_dev_cmd_send_lce_event1(zb_uint8_t param);

/** [esi_dev_cmd_send_lce_event3]  */
/** Send DRLC - Cancel All Load Control Events command. */
static void esi_dev_cmd_send_lce_event3(zb_uint8_t param)
{
  zb_uint8_t cancel_control;
  if (!param)
  {
    ZB_GET_OUT_BUF_DELAYED(esi_dev_cmd_send_lce_event3);
  }
  else
  {
    cancel_control = 0;
    ZB_ZCL_DRLC_SEND_CMD_CANCEL_ALL_LCE(param,
                                        &g_dev_ctx.drlc_client_address,
                                        ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                                        g_dev_ctx.drlc_client_ep,
                                        ESI_DEV_ENDPOINT,
                                        &cancel_control);
    ZB_SCHEDULE_APP_ALARM(esi_dev_cmd_send_lce_event1, 0, ESI_LOAD_CONTROL_EVENT_GROUP_TIMEOUT);
  }
}
/** [esi_dev_cmd_send_lce_event3]  */

/** [esi_dev_cmd_send_lce_event2] */
/** Send DRLC - Cancel Load Control Event command. */
static void esi_dev_cmd_send_lce_event2(zb_uint8_t param)
{
  zb_zcl_drlc_cancel_lce_payload_t payload;
  if (!param)
  {
    ZB_GET_OUT_BUF_DELAYED(esi_dev_cmd_send_lce_event2);
  }
  else
  {
    payload.issuer_event_id = 0x01;
    payload.device_class = DRLC_DEVICE_CLASS_POOL_PUMP;
    payload.utility_enrollment_group = 0x88;
    payload.cancel_control = 0;
    payload.effective_time = 0; /* 0 means now */
    /* Note: effective_time: This field is deprecated; a Cancel Load Control command shall
     * now take immediate effect. A value of 0x00000000 shall be used in all Cancel Load Control
     * commands. */
    ZB_ZCL_DRLC_SEND_CMD_CANCEL_LCE(param,
                                    &g_dev_ctx.drlc_client_address,
                                    ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                                    g_dev_ctx.drlc_client_ep,
                                    ESI_DEV_ENDPOINT,
                                    &payload);
    ZB_SCHEDULE_APP_ALARM(esi_dev_cmd_send_lce_event3, 0, ESI_LOAD_CONTROL_EVENT_TIMEOUT);
  }
}
/** [esi_dev_cmd_send_lce_event2] */

/** [esi_dev_cmd_send_lce_event1]  */
/** Send DRLC - Load Control Event command.
    Emulate real device behavior - send Load Control Event, then after 5 sec
    (ESI_LOAD_CONTROL_EVENT_TIMEOUT) - Cancel Load Control Event, then after 5 sec - Cancel All
    Load Control Events.
    Repeat every 60 sec (ESI_LOAD_CONTROL_EVENT_GROUP_TIMEOUT).
*/
static void esi_dev_cmd_send_lce_event1(zb_uint8_t param)
{
  zb_zcl_drlc_lce_payload_t payload = ZB_ZCL_DRLC_LCE_PAYLOAD_INIT;
  if (!param)
  {
    ZB_GET_OUT_BUF_DELAYED(esi_dev_cmd_send_lce_event1);
  }
  else
  {
    payload.issuer_event_id = 0x01;
    payload.device_class = DRLC_DEVICE_CLASS_POOL_PUMP;
    payload.utility_enrollment_group = 0x88;
    payload.start_time = zb_get_utc_time();
    payload.duration_in_minutes = 1;
    payload.criticality_level = 0x56;
    payload.event_control = 0xab;
    ZB_ZCL_DRLC_SEND_CMD_LOAD_CONTROL_EVENT(param,
                                            &g_dev_ctx.drlc_client_address,
                                            ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                                            g_dev_ctx.drlc_client_ep,
                                            ESI_DEV_ENDPOINT,
                                            &payload);
    ZB_SCHEDULE_APP_ALARM(esi_dev_cmd_send_lce_event2, 0, ESI_LOAD_CONTROL_EVENT_TIMEOUT);
  }
}
/** [esi_dev_cmd_send_lce_event1]  */

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

/** [zb_zcl_calendar_send_cmd_publish_calendar] */
void esi_send_publish_calendar(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> esi_send_publish_calendar param %hd", (FMT__H, param));
  if (!param)
  {
    ZB_GET_OUT_BUF_DELAYED(esi_send_publish_calendar);
  }
  else
  {
    /* FIXME: default values for this payload are not determined */
    zb_zcl_calendar_publish_calendar_payload_t payload = ZB_ZCL_CALENDAR_PUBLISH_CALENDAR_PL_INIT;
    /*Here we must get events if present and pass them to client*/

    const char name[] = "New Calendar";  /* length is 12 + \0. There is enough space for store it. */

    payload.provider_id = 0x01;
    payload.issuer_event_id = zb_get_utc_time();  /* always increasing */
    payload.issuer_calendar_id = 0x02;
    payload.start_time = 0;
    payload.calendar_type = ZB_ZCL_CALENDAR_TYPE_DELIVERED_CALENDAR;
    payload.calendar_time_reference = 0x00;  /* UTC time */

    ZB_ZCL_STRING_CLEAR(payload.calendar_name);
    ZB_ZCL_STATIC_STRING_APPEND_C_STR(payload.calendar_name, name);

    payload.number_of_seasons = 0x00;        /* no Season defined */
    payload.number_of_week_profiles = 0x00;  /* no Week Profile defined */
    payload.number_of_day_profiles = 1;

    ZB_ZCL_CALENDAR_SEND_CMD_PUBLISH_CALENDAR(param,
                                         &g_dev_ctx.calendar_client_address,
                                         ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                                         g_dev_ctx.calendar_client_ep,
                                         ESI_DEV_ENDPOINT,
                                         &payload);
  }

  TRACE_MSG(TRACE_APP1, "<< esi_send_publish_calendar", (FMT__0));
}
/** [zb_zcl_calendar_send_cmd_publish_calendar]  */


/** [zb_zcl_calendar_send_cmd_publish_week_profile] */
void esi_send_publish_week_profile(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> esi_send_publish_week_profile param %hd", (FMT__H, param));
  if (!param)
  {
    ZB_GET_OUT_BUF_DELAYED(esi_send_publish_week_profile);
  }
  else
  {
    zb_zcl_calendar_publish_week_profile_payload_t payload; /* FIXME: = ZB_ZCL_ENERGY_MANAGEMENT_REPORT_EVENT_STATUS_PAYLOAD_INIT; */

    payload.provider_id = 0x01;
    payload.issuer_event_id = zb_get_utc_time();
    payload.issuer_calendar_id = 0x02;
    payload.week_id = 1;

    payload.day_id_ref_monday = 0x0;
    payload.day_id_ref_tuesday = 0x0;
    payload.day_id_ref_wednesday = 0x0;
    payload.day_id_ref_thursday = 0x0;
    payload.day_id_ref_friday = 0x0;
    payload.day_id_ref_saturday = 0x0;
    payload.day_id_ref_sunday = 0x0;

    ZB_ZCL_CALENDAR_SEND_CMD_PUBLISH_WEEK_PROFILE(param,
                                         &g_dev_ctx.calendar_client_address,
                                         ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                                         g_dev_ctx.calendar_client_ep,
                                         ESI_DEV_ENDPOINT,
                                         &payload);
                                        }
                                        TRACE_MSG(TRACE_APP1, "<< esi_send_publish_week_profile", (FMT__0));
}
/** [zb_zcl_calendar_send_cmd_publish_week_profile] */


/** [zb_zcl_calendar_send_cmd_publish_day_profile] */
void esi_send_publish_day_profile(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> esi_send_publish_day_profile param %hd", (FMT__H, param));
  if (!param)
  {
    ZB_GET_OUT_BUF_DELAYED(esi_send_publish_day_profile);
  }
  else
  {
    zb_zcl_calendar_publish_day_profile_payload_t payload = ZB_ZCL_CALENDAR_PUBLISH_DAY_PROFILE_PL_INIT;

    payload.provider_id = 0x01;
    payload.issuer_event_id = zb_get_utc_time();
    payload.issuer_calendar_id = 0x02;
    payload.day_id = 1;
    payload.total_number_of_schedule_entries = 0;
    payload.command_index = 0;
    payload.total_number_of_commands = 1;
    payload.calendar_type = ZB_ZCL_CALENDAR_TYPE_FRIENDLY_CREDIT_CALENDAR;
    payload.day_schedule_entries = NULL;
    payload.number_of_entries_in_this_command = 0;
/*
    ZB_ZCL_CALENDAR_SEND_CMD_PUBLISH_DAY_PROFILE(param,
                                         &g_dev_ctx.calendar_client_address,
                                         ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                                         g_dev_ctx.calendar_client_ep,
                                         ESI_DEV_ENDPOINT,
                                         &payload);*/
  }

  TRACE_MSG(TRACE_APP1, "<< esi_send_publish_day_profile", (FMT__0));
}
/** [zb_zcl_calendar_send_cmd_publish_day_profile] */

/** [zb_zcl_calendar_send_cmd_cancel_calendar]  */
void esi_send_cancel_calendar(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> esi_send_cancel_calendar param %hd", (FMT__H, param));
  if (!param)
  {
    ZB_GET_OUT_BUF_DELAYED(esi_send_cancel_calendar);
  }
  else
  {
    zb_zcl_calendar_cancel_calendar_payload_t payload; /*FIXME: = ZB_ZCL_CALENDAR_CANCEL_CALENDAR_INIT; */
    /*Here we must get events if present and pass them to client*/

    payload.provider_id = 0x01;
    payload.issuer_calendar_id = zb_get_utc_time();
    payload.calendar_type = ZB_ZCL_CALENDAR_TYPE_DELIVERED_CALENDAR;

    ZB_ZCL_CALENDAR_SEND_CMD_CANCEL_CALENDAR(param,
                                        &g_dev_ctx.calendar_client_address,
                                        ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                                        g_dev_ctx.calendar_client_ep,
                                        ESI_DEV_ENDPOINT,
                                        &payload);
  }

  TRACE_MSG(TRACE_APP1, "<< esi_send_cancel_calendar", (FMT__0));
}
/** [zb_zcl_calendar_send_cmd_cancel_calendar]  */

#if ESI_ENABLE_CALENDAR_CLUSTER
/** [handle_get_calendar] */
static void handle_get_calendar(zb_uint8_t param)
{
  const zb_zcl_calendar_get_calendar_payload_t *pl_in = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_calendar_get_calendar_payload_t);
  zb_zcl_calendar_publish_calendar_payload_t *pl_out = ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(param, zb_zcl_calendar_publish_calendar_payload_t);
  const char name[] = "SomeCalendar";

  TRACE_MSG(TRACE_ZCL1, "Received get calendar from %d", (FMT__D, pl_in->provider_id));

  pl_out->provider_id = pl_in->provider_id;
  pl_out->issuer_event_id = pl_in->min_issuer_event_id+1;  /* always increasing */
  pl_out->issuer_calendar_id = 0x02;
  pl_out->start_time = pl_in->earliest_start_time+1;
  pl_out->calendar_type = pl_in->calendar_type;

  ZB_ZCL_STRING_CLEAR(pl_out->calendar_name);
  ZB_ZCL_STATIC_STRING_APPEND_C_STR(pl_out->calendar_name, name);

  ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
}
/** [handle_get_calendar] */

/** [handle_get_day_profiles]  */
static void handle_get_day_profiles(zb_uint8_t param)
{
  const zb_zcl_calendar_get_day_profiles_payload_t *pl_in = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_calendar_get_day_profiles_payload_t);
  zb_zcl_calendar_publish_day_profile_payload_t *pl_out = ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(param, zb_zcl_calendar_publish_day_profile_payload_t);

  TRACE_MSG(TRACE_ZCL1, "Received get_day_profiles from %d", (FMT__D, pl_in->provider_id));

  pl_out->provider_id = 0x01;
  pl_out->issuer_event_id = zb_get_utc_time();
  pl_out->issuer_calendar_id = 0x02;
  pl_out->day_id = 1;
  pl_out->total_number_of_schedule_entries = 0;
  pl_out->command_index = 0;
  pl_out->total_number_of_commands = 1;
  pl_out->calendar_type = ZB_ZCL_CALENDAR_TYPE_FRIENDLY_CREDIT_CALENDAR;
  pl_out->day_schedule_entries = NULL;
  pl_out->number_of_entries_in_this_command = 0;

  ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
}
/** [handle_get_day_profiles]  */

/** [handle_get_week_profiles] */
static void handle_get_week_profiles(zb_uint8_t param)
{
  const zb_zcl_calendar_get_week_profiles_payload_t *pl_in = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_calendar_get_week_profiles_payload_t);
  zb_zcl_calendar_publish_week_profile_payload_t *pl_out = ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(param, zb_zcl_calendar_publish_week_profile_payload_t);

  TRACE_MSG(TRACE_ZCL1, "Received get_week_profiles from %d", (FMT__D, pl_in->provider_id));

  pl_out->provider_id = pl_in->provider_id;
  pl_out->issuer_event_id = zb_get_utc_time();
  pl_out->issuer_calendar_id = pl_in->issuer_calendar_id;
  pl_out->week_id = pl_in->start_week_id;
  pl_out->day_id_ref_monday = 1;
  pl_out->day_id_ref_tuesday = 1;
  pl_out->day_id_ref_wednesday = 1;
  pl_out->day_id_ref_thursday = 1;
  pl_out->day_id_ref_friday = 1;
  pl_out->day_id_ref_saturday = 2;
  pl_out->day_id_ref_sunday = 2;

  ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
}
/** [handle_get_week_profiles] */

/** [handle_get_seasons]  */
static void handle_get_seasons(zb_uint8_t param)
{
  const zb_zcl_calendar_get_seasons_payload_t *pl_in = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_calendar_get_seasons_payload_t);
  zb_zcl_calendar_publish_seasons_payload_t *pl_out = ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(param, zb_zcl_calendar_publish_seasons_payload_t);

  TRACE_MSG(TRACE_ZCL1, "Received get_seasons from %d", (FMT__D, pl_in->provider_id));

  pl_out->provider_id = pl_in->provider_id;
  pl_out->issuer_event_id = zb_get_utc_time();
  pl_out->issuer_calendar_id = pl_in->issuer_calendar_id;
  pl_out->command_index = 0;
  pl_out->total_number_of_commands = 1;
  pl_out->season_entry = NULL;
  pl_out->number_of_entries_in_this_command = 0;

  ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
}
/** [handle_get_seasons]  */

/** [get_special_days]  */
static void handle_get_special_days(zb_uint8_t param)
{
  const zb_zcl_calendar_get_special_days_payload_t *pl_in = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_calendar_get_special_days_payload_t);
  zb_zcl_calendar_publish_special_days_payload_t *pl_out = ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(param, zb_zcl_calendar_publish_special_days_payload_t);

  TRACE_MSG(TRACE_ZCL1, "Received get_special_days from %d", (FMT__D, pl_in->provider_id));

  pl_out->provider_id = pl_in->provider_id;
  pl_out->issuer_event_id = 0xaa;
  pl_out->issuer_calendar_id = pl_in->issuer_calendar_id;
  pl_out->start_time = zb_get_utc_time();
  pl_out->calendar_type = pl_in->calendar_type;
  pl_out->total_number_of_special_days = 0;
  pl_out->command_index = 0;
  pl_out->total_number_of_commands = 1;
  pl_out->special_day_entry = NULL;
  pl_out->number_of_entries_in_this_command = 0;

  ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
}
/** [get_special_days]  */

static void handle_get_calendar_cancellation(zb_uint8_t param)
{
  zb_zcl_calendar_cancel_calendar_payload_t *pl_out = ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(param, zb_zcl_calendar_cancel_calendar_payload_t);

  TRACE_MSG(TRACE_ZCL1, "Received get_calendar_cancellation from device", (FMT__0));

  pl_out->provider_id = 0x01;
  pl_out->issuer_calendar_id = zb_get_utc_time();
  pl_out->calendar_type = ZB_ZCL_CALENDAR_TYPE_DELIVERED_CALENDAR;

   ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
}
#endif


static void handle_get_last_message(zb_zcl_messaging_get_last_message_response_t *resp)
{
  const char message[] = "The display string, random value = ";
  zb_uint8_t rand_val = ZB_RANDOM_VALUE(9);

  ZB_ZCL_STRING_CLEAR(resp->resp.display_message.message);
  ZB_ZCL_STATIC_STRING_APPEND_C_STR(resp->resp.display_message.message, message);
  ZB_ZCL_STATIC_STRING_APPEND_CHAR(resp->resp.display_message.message, '0' + rand_val);

  resp->resp_type = ZB_ZCL_MESSAGING_RESPONSE_TYPE_NORMAL;
  resp->resp.display_message.duration_in_minutes = 0;
  resp->resp.display_message.extended_message_control = 0;
  resp->resp.display_message.start_time = 0;
  resp->resp.display_message.message_id = zb_get_utc_time();
  resp->resp.display_message.message_control = 0;
}

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
    case ZB_ZCL_PRICE_GET_CURRENT_PRICE_CB_ID:
      {
        TRACE_MSG(TRACE_APP1, "ZB_ZCL_PRICE_GET_CURRENT_PRICE_CB_ID", (FMT__0));
        esi_get_current_price_cb(param);
      }
      break;
    case ZB_ZCL_PRICE_GET_SCHEDULED_PRICES_CB_ID:
      {
        TRACE_MSG(TRACE_APP1, "ZB_ZCL_PRICE_GET_SCHEDULED_PRICES_CB_ID", (FMT__0));
        esi_get_sched_prices_cb(param);
      }
      break;
    case ZB_ZCL_PRICE_PRICE_ACK_CB_ID:
      {
        TRACE_MSG(TRACE_APP1, "ZB_ZCL_PRICE_PRICE_ACK_CB_ID", (FMT__0));
        esi_price_ack_cb(param);
      }
      break;
    case ZB_ZCL_DRLC_REPORT_EVENT_STATUS_CB_ID:
      {
        TRACE_MSG(TRACE_APP1, "ZB_ZCL_DRLC_REPORT_EVENT_STATUS_CB_ID", (FMT__0));
        ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
        handle_report_event_status(
          ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param,
                                         zb_zcl_drlc_report_event_status_payload_t),
          ZB_ZCL_DEVICE_CMD_PARAM_CMD_INFO(param));
      }
      break;
    case ZB_ZCL_DRLC_GET_SCHEDULED_EVENTS_CB_ID:
      {
        TRACE_MSG(TRACE_APP1, "ZB_ZCL_DRLC_GET_SCHEDULED_EVENTS_CB_ID", (FMT__0));
        ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
        handle_get_scheduled_events(
          ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param,zb_zcl_drlc_get_scheduled_events_payload_t),
          ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(param,zb_zcl_drlc_lce_payload_t));
      }
      break;
    case ZB_ZCL_MDU_PAIRING_REQUEST_CB_ID:
    {
      TRACE_MSG(TRACE_APP1, "ZB_ZCL_MDU_PAIRING_REQUEST_CB_ID", (FMT__0));
      ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = handle_get_mdu_events(
        ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param,zb_zcl_mdu_pairing_request_t),
        ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(param,zb_zcl_mdu_pairing_response_t));
    }
    break;
    case ZB_ZCL_TUNNELING_REQUEST_TUNNEL_CB_ID:
    {
      const zb_zcl_tunneling_request_tunnel_t *req =
        ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_tunneling_request_tunnel_t);
      zb_uint8_t *tunnel_status = ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(param, zb_uint8_t);

      TRACE_MSG(TRACE_APP1, "ZB_ZCL_TUNNELING_REQUEST_TUNNEL_CB_ID", (FMT__0));

      /* Check protocol, flow control etc. */
      if (req->protocol_id != ZB_ZCL_TUNNELING_PROTOCOL_MANUFACTURER_DEFINED)
      {
        *tunnel_status = ZB_ZCL_TUNNELING_TUNNEL_STATUS_PROTOCOL_NOT_SUPPORTED;
      }
      else if (req->flow_control_support != ZB_FALSE)
      {
        *tunnel_status = ZB_ZCL_TUNNELING_TUNNEL_STATUS_FLOW_CONTROL_NOT_SUPPORTED;
      }
      else
      {
        *tunnel_status = ZB_ZCL_TUNNELING_TUNNEL_STATUS_SUCCESS;
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
#if ESI_ENABLE_CALENDAR_CLUSTER
    case ZB_ZCL_CALENDAR_GET_CALENDAR_CB_ID:
    {
        TRACE_MSG(TRACE_APP1, "ZB_ZCL_CALENDAR_GET_CALENDAR_CB_ID", (FMT__0));
        handle_get_calendar(param);
    }
    break;
    case ZB_ZCL_CALENDAR_GET_DAY_PROFILES_CB_ID:
    {
        TRACE_MSG(TRACE_APP1, "ZB_ZCL_CALENDAR_GET_DAY_PROFILES_CB_ID", (FMT__0));
        handle_get_day_profiles(param);
    }
    break;
    case ZB_ZCL_CALENDAR_GET_WEEK_PROFILES_CB_ID:
    {
        TRACE_MSG(TRACE_APP1, "ZB_ZCL_CALENDAR_GET_WEEK_PROFILES_CB_ID", (FMT__0));
        handle_get_week_profiles(param);
    }
    break;
    case ZB_ZCL_CALENDAR_GET_SEASONS_CB_ID:
    {
        TRACE_MSG(TRACE_APP1, "ZB_ZCL_CALENDAR_GET_SEASONS_CB_ID", (FMT__0));
        handle_get_seasons(param);
    }
    break;
    case ZB_ZCL_CALENDAR_GET_SPECIAL_DAYS_CB_ID:
    {
        TRACE_MSG(TRACE_APP1, "ZB_ZCL_CALENDAR_GET_SPECIAL_DAYS_CB_ID", (FMT__0));
        handle_get_special_days(param);
    }
    break;
    case ZB_ZCL_CALENDAR_GET_CALENDAR_CANCELLATION_CB_ID:
    {
        TRACE_MSG(TRACE_APP1, "ZB_ZCL_CALENDAR_GET_CALENDAR_CANCELLATION_CB_ID", (FMT__0));
        handle_get_calendar_cancellation(param);
    }
    break;
#endif
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
  ZB_ZCL_REGISTER_DEVICE_CB(esi_zcl_cmd_device_cb);

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
  zb_channel_page_list_set_mask(channel_list, ESI_DEV_CHANNEL_PAGE_I, ESI_DEV_CHANNEL_MASK1);
  TRACE_MSG(TRACE_APP1, "ZC in MM mode start: page %d mask 0x%x", (FMT__D_D, ESI_DEV_CHANNEL_PAGE_I, ESI_DEV_CHANNEL_MASK1));
  zb_se_set_network_coordinator_role_select_device(channel_list);
}
#else
/** [ESI_DEV_SET_ROLE] */
zb_se_set_network_coordinator_role(ESI_DEV_CHANNEL_MASK);
/** [ESI_DEV_SET_ROLE] */
#endif
  TRACE_MSG(TRACE_APP1, "<< esi_dev_app_init", (FMT__0));
}

/*********************  SE ESI ZC  **************************/

MAIN()
{
  ARGV_UNUSED;

  ZB_SET_TRACE_ON();
  ZB_SET_TRAF_DUMP_ON();

  ZB_INIT("esi_device");

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

#ifdef TEST_DUTY_C
void zb_mac_duty_cycle_bump_buckets();

void tst_bump_buckets_alarm(zb_uint8_t param)
{
  ZVUNUSED(param);
  zb_mac_duty_cycle_bump_buckets();
  ZB_SCHEDULE_APP_ALARM(tst_bump_buckets_alarm, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(1000 * 2));
}

#endif


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
#ifdef TEST_DUTY_C
        {
          void zb_mac_test_dec_th();
          zb_mac_test_dec_th();
          ZB_SCHEDULE_APP_ALARM(tst_bump_buckets_alarm, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(1000 * 2));
        }
#endif
        break;

/** [SIGNAL_HANDLER_BIND_INDICATION] */
        case ZB_SE_SIGNAL_SERVICE_DISCOVERY_BIND_INDICATION:
        {
          zb_se_signal_service_discovery_bind_params_t *bind_params =
            ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_se_signal_service_discovery_bind_params_t);

          TRACE_MSG(TRACE_APP1, "Bind indication signal: binded cluster 0x%x endpoint %d device " TRACE_FORMAT_64,
                    (FMT__D_D_A, bind_params->cluster_id, bind_params->endpoint, TRACE_ARG_64(bind_params->device_addr)));
/** [SIGNAL_HANDLER_BIND_INDICATION] */

          if (bind_params->cluster_id == ZB_ZCL_CLUSTER_ID_DRLC)
          {
            ZB_IEEE_ADDR_COPY(g_dev_ctx.drlc_client_address.addr_long,bind_params->device_addr);
            g_dev_ctx.drlc_client_ep = bind_params->endpoint;
            TRACE_MSG(TRACE_APP1,
              "DRLC device bound to out service: remote addr " TRACE_FORMAT_64 " ep %hd",
               (FMT__A_D, TRACE_ARG_64(g_dev_ctx.drlc_client_address.addr_long),
                g_dev_ctx.drlc_client_ep));

            ZB_SCHEDULE_APP_ALARM_CANCEL(esi_dev_cmd_send_lce_event1, ZB_ALARM_ANY_PARAM);
            ZB_SCHEDULE_APP_CALLBACK(esi_dev_cmd_send_lce_event1, param);
            param = 0;
          }
#if ESI_ENABLE_CALENDAR_CLUSTER
          else if (bind_params->cluster_id == ZB_ZCL_CLUSTER_ID_CALENDAR)
          {
            /* We send unsolicited command to client, so we store client data*/

            ZB_IEEE_ADDR_COPY(g_dev_ctx.calendar_client_address.addr_long,
                              bind_params->device_addr);
            g_dev_ctx.calendar_client_ep = bind_params->endpoint;

            TRACE_MSG(TRACE_APP1,
                     "Calendar device bound to out service: remote addr " TRACE_FORMAT_64 " ep %hd",
                      (FMT__A_D, TRACE_ARG_64(g_dev_ctx.calendar_client_address.addr_long),
                       g_dev_ctx.calendar_client_ep));

            /* FIXME: APS_KEY_READY is not received */
            ZB_SCHEDULE_APP_ALARM_CANCEL(esi_send_publish_calendar, ZB_ALARM_ANY_PARAM);
            ZB_SCHEDULE_APP_CALLBACK(esi_send_publish_calendar, param);
            param = 0;
          }
#endif /* ESI_ENABLE_CALENDAR_CLUSTER */
        }
        break;

/** [SIGNAL_HANDLER_TC_SIGNAL_CHILD_JOIN] */
      case ZB_SE_TC_SIGNAL_CHILD_JOIN_CBKE:
        {
          zb_uint8_t *remote_device_addr = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_uint8_t);
          TRACE_MSG(TRACE_APP1, "Child " TRACE_FORMAT_64 " joined & established CBKE TCLK",
                    (FMT__A, TRACE_ARG_64(remote_device_addr)));
#ifdef DEBUG
          {
            zb_uint8_t key[ZB_CCM_KEY_SIZE];
            if (zb_se_debug_get_link_key_by_long(remote_device_addr, key) == RET_OK)
            {
              TRACE_MSG(TRACE_APP1, "Child " TRACE_FORMAT_64 " joined & established CBKE TCLK " TRACE_FORMAT_128,
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

#if ESI_ENABLE_CALENDAR_CLUSTER
          TRACE_MSG(TRACE_APP1, "Child joined & established TCLK", (FMT__0));
          /* TODO: Do we need to implement the same approach as used in IHD? */
          if (ZB_IEEE_ADDR_CMP(g_dev_ctx.calendar_client_address.addr_long, remote_device_addr))
          {
            TRACE_MSG(TRACE_APP1, "Calendar cluster's partner link initiated", (FMT__0));

            /* FIXME: need to send this command after some intervals */
            ZB_SCHEDULE_APP_ALARM_CANCEL(esi_send_publish_calendar, ZB_ALARM_ANY_PARAM);
            ZB_SCHEDULE_APP_CALLBACK(esi_send_publish_calendar, param);
          }
#endif /* ESI_ENABLE_CALENDAR_CLUSTER */

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
    zb_free_buf(ZB_BUF_FROM_REF(param));
  }

  TRACE_MSG(TRACE_APP1, "<< zboss_signal_handler", (FMT__0));
}
/** [SIGNAL_HANDLER] */
