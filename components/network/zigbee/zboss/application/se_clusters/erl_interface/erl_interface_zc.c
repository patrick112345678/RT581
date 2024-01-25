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
/* PURPOSE: ERL ZC device sample
*/

#define ZB_TRACE_FILE_ID 63707
#include "zboss_api.h"
#include "zb_erl_interface.h"
#include "erl_device_attrs.h"

/** [ERL_INTERFACE_DEV_DEFINE_PARAMS] */
#define ERL_INTERFACE_PHASE_METERING_EL_1_DEV_ENDPOINT 1
#define ERL_INTERFACE_PHASE_METERING_EL_2_DEV_ENDPOINT 2
#define ERL_INTERFACE_PHASE_METERING_EL_3_DEV_ENDPOINT 3
#define ERL_INTERFACE_DEV_ENDPOINT 4
#define ERL_INTERFACE_DEV_CHANNEL_MASK (1l<<21)

#define ERL_INTERFACE_DEV_MEASURE_TIMEOUT ZB_TIME_ONE_SECOND

/* SummationFormatting: XXXXXX.Y, do not suppress leading zeroes */
#define ERL_INTERFACE_DEV_SUMM_FMT_LEFT 8
#define ERL_INTERFACE_DEV_SUMM_FMT_RIGHT 3
#define ERL_INTERFACE_DEV_SUMM_FMT_SUPPR 0
#define ERL_INTERFACE_DEV_SUMMATION_FORMATTING \
  ZB_ZCL_METERING_FORMATTING_SET(ERL_INTERFACE_DEV_SUMM_FMT_SUPPR, \
                                 ERL_INTERFACE_DEV_SUMM_FMT_LEFT, \
                                 ERL_INTERFACE_DEV_SUMM_FMT_RIGHT)

#define ERL_INTERFACE_DEV_ADDR          {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa}
static zb_ieee_addr_t g_dev_addr = ERL_INTERFACE_DEV_ADDR;

/** [ERL_INTERFACE_DEV_DEFINE_PARAMS] */
#define ERL_INTERFACE_TIME_STATUS_INITIAL ((1 << ZB_ZCL_TIME_MASTER) | (1 << ZB_ZCL_TIME_MASTER_ZONE_DST))

#define ERL_INTERFACE_PRICE_ACK_TIMEOUT (1 * ZB_TIME_ONE_SECOND)
#define ERL_INTERFACE_PRICE_SCHEDULED_PRICES_TIMEOUT (3 * ZB_TIME_ONE_SECOND)


/** Disables PriceAck bit in 'price_control'.
 */
#define ERL_INTERFACE_DISABLE_PRICE_ACK 0

#define ERL_INTERFACE_ENABLE_CALENDAR_CLUSTER 1

/** [DECLARE_CLUSTERS] */
/*********************  Clusters' attributes  **************************/

/** @struct erl_interface_dev_price_attrs_s
 *  @brief Price cluster attributes
 */
typedef struct erl_interface_dev_price_attrs_s
{
  zb_uint8_t commodity_type;
} erl_interface_dev_price_attrs_t;

typedef struct erl_interface_dev_identify_attrs_s
{
  zb_uint16_t identify_time;
} erl_interface_dev_identify_attrs_t;


/** GetScheduledPrices states.
 */
typedef enum erl_interface_dev_get_sched_prices_state_e {
  ERL_INTERFACE_DEV_GET_SCHED_PRICES_STATE_INIT,
  ERL_INTERFACE_DEV_GET_SCHED_PRICES_STATE_WAIT_PRICE_ACK,
  ERL_INTERFACE_DEV_GET_SCHED_PRICES_STATE_FINI = ERL_INTERFACE_DEV_GET_SCHED_PRICES_STATE_INIT,
} erl_interface_dev_get_sched_prices_state_t;

typedef struct erl_interface_dev_price_ctx_t {
  zb_addr_u cli_addr;
  zb_aps_addr_mode_t cli_addr_mode;
  zb_uint8_t cli_ep;

  zb_uint8_t nr_events;
  zb_uint8_t curr_event;

  zb_zcl_price_publish_price_payload_t pp_pl;

  erl_interface_dev_get_sched_prices_state_t state;

  zb_bool_t price_ack_disabled;
} erl_interface_dev_price_ctx_t;

#define ERL_INTERFACE_DEV_PRICE_CTX() (g_dev_ctx.erl_interface.price_ctx)

typedef struct erl_interface_ctx_s
{
  zb_zcl_basic_attrs_t basic_attrs;
  zb_zcl_time_attrs_t time_attrs;
  erl_interface_metering_server_attrs_t metering_attrs;
  erl_interface_dev_price_attrs_t price_attrs;
  erl_interface_dev_identify_attrs_t identify_attrs;
  erl_calendar_server_attrs_t calendar_attrs;
  erl_meter_identification_server_attrs_t meter_identification_attrs;
  erl_el_measurement_server_attrs_t el_measurement_attrs;
  zb_addr_u calendar_client_address;
  zb_uint8_t calendar_client_ep;
  erl_interface_dev_price_ctx_t price_ctx;
} erl_interface_ctx_t;

typedef struct erl_interface_phase_metering_element_ctx_s
{
  zb_zcl_basic_attrs_t basic_attrs;
  erl_interface_dev_identify_attrs_t identify_attrs;
  zb_zcl_metering_attrs_t metering_attrs;
} erl_interface_phase_metering_element_ctx_t;

/** @struct erl_interface_dev_ctx_s
 *  @brief ERL device context
 */
typedef struct erl_interface_dev_ctx_s
{
  erl_interface_ctx_t erl_interface;
  erl_interface_phase_metering_element_ctx_t phase_metering_el[3];
} erl_interface_dev_ctx_t;

/* device context */
static erl_interface_dev_ctx_t g_dev_ctx;

ZB_ZCL_DECLARE_BASIC_ATTR_LIST(erl_interface_basic_attr_list, g_dev_ctx.erl_interface.basic_attrs);
ZB_ZCL_DECLARE_PRICE_SRV_ATTRIB_LIST(erl_interface_price_attr_list,
                                   &g_dev_ctx.erl_interface.price_attrs.commodity_type);
ZB_ZCL_DECLARE_TIME_ATTR_LIST(erl_interface_time_attr_list, g_dev_ctx.erl_interface.time_attrs);
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(erl_interface_identify_attr_list,
                                    &g_dev_ctx.erl_interface.identify_attrs.identify_time);
ERL_DECLARE_CALENDAR_ATTR_LIST(erl_interface_calendar_attr_list,
                                 g_dev_ctx.erl_interface.calendar_attrs);
ERL_DECLARE_METER_IDENTIFICATION_ATTR_LIST(erl_interface_meter_identification_attr_list,
                                             g_dev_ctx.erl_interface.meter_identification_attrs);
ERL_DECLARE_EL_MEASUREMENT_ATTR_LIST(erl_interface_el_measurement_attr_list,
                                     g_dev_ctx.erl_interface.el_measurement_attrs);
ZB_ZCL_DECLARE_DIAGNOSTICS_ATTRIB_LIST(erl_interface_diagnostics_attr_list);
ERL_INTERFACE_DECLARE_METERING_ATTR_LIST(erl_interface_metering_attr_list,
                                         g_dev_ctx.erl_interface.metering_attrs);

ZB_DECLARE_ERL_INTERFACE_DEV_CLUSTER_LIST(erl_interface_dev_clusters,
                                          erl_interface_basic_attr_list,
                                          erl_interface_identify_attr_list,
                                          erl_interface_time_attr_list,
                                          erl_interface_meter_identification_attr_list,
                                          erl_interface_el_measurement_attr_list,
                                          erl_interface_diagnostics_attr_list,
                                          erl_interface_price_attr_list,
                                          erl_interface_metering_attr_list,
                                          erl_interface_calendar_attr_list);

ZB_DECLARE_ERL_INTERFACE_DEV_EP(erl_interface_dev_ep, ERL_INTERFACE_DEV_ENDPOINT, erl_interface_dev_clusters);

ZB_ZCL_DECLARE_BASIC_ATTR_LIST(phase_metering_el1_basic_attr_list,
                               g_dev_ctx.phase_metering_el[0].basic_attrs);
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(phase_metering_el1_identify_attr_list,
                                    &g_dev_ctx.phase_metering_el[0].identify_attrs.identify_time);
ZB_ZCL_DECLARE_METERING_ATTR_LIST(phase_metering_el1_metering_attr_list,
                                  g_dev_ctx.phase_metering_el[0].metering_attrs);


ZB_DECLARE_ERL_INTERFACE_PHASE_METERING_ELEMENT_DEV_CLUSTER_LIST(
  phase_metering_el1_dev_clusters,
  phase_metering_el1_basic_attr_list,
  phase_metering_el1_identify_attr_list,
  phase_metering_el1_metering_attr_list);

ZB_DECLARE_ERL_INTERFACE_PHASE_METERING_ELEMENT_DEV_EP(phase_metering_el1_dev_ep,
                                ERL_INTERFACE_PHASE_METERING_EL_1_DEV_ENDPOINT,
                                phase_metering_el1_dev_clusters);

ZB_ZCL_DECLARE_BASIC_ATTR_LIST(phase_metering_el2_basic_attr_list,
                               g_dev_ctx.phase_metering_el[1].basic_attrs);
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(phase_metering_el2_identify_attr_list,
                                    &g_dev_ctx.phase_metering_el[1].identify_attrs.identify_time);
ZB_ZCL_DECLARE_METERING_ATTR_LIST(phase_metering_el2_metering_attr_list,
                                  g_dev_ctx.phase_metering_el[1].metering_attrs);


ZB_DECLARE_ERL_INTERFACE_PHASE_METERING_ELEMENT_DEV_CLUSTER_LIST(
  phase_metering_el2_dev_clusters,
  phase_metering_el2_basic_attr_list,
  phase_metering_el2_identify_attr_list,
  phase_metering_el2_metering_attr_list);

ZB_DECLARE_ERL_INTERFACE_PHASE_METERING_ELEMENT_DEV_EP(phase_metering_el2_dev_ep,
                                ERL_INTERFACE_PHASE_METERING_EL_2_DEV_ENDPOINT,
                                phase_metering_el2_dev_clusters);

ZB_ZCL_DECLARE_BASIC_ATTR_LIST(phase_metering_el3_basic_attr_list,
                               g_dev_ctx.phase_metering_el[2].basic_attrs);
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(phase_metering_el3_identify_attr_list,
                                    &g_dev_ctx.phase_metering_el[2].identify_attrs.identify_time);
ZB_ZCL_DECLARE_METERING_ATTR_LIST(phase_metering_el3_metering_attr_list,
                                  g_dev_ctx.phase_metering_el[2].metering_attrs);

ZB_DECLARE_ERL_INTERFACE_PHASE_METERING_ELEMENT_DEV_CLUSTER_LIST(
  phase_metering_el3_dev_clusters,
  phase_metering_el3_basic_attr_list,
  phase_metering_el3_identify_attr_list,
  phase_metering_el3_metering_attr_list);

ZB_DECLARE_ERL_INTERFACE_PHASE_METERING_ELEMENT_DEV_EP(phase_metering_el3_dev_ep,
                                ERL_INTERFACE_PHASE_METERING_EL_3_DEV_ENDPOINT,
                                phase_metering_el3_dev_clusters);

ZB_DECLARE_ERL_INTERFACE_DEV_CTX(erl_interface_dev_zcl_ctx,
                                 phase_metering_el1_dev_ep,
                                 phase_metering_el2_dev_ep,
                                 phase_metering_el3_dev_ep,
                                 erl_interface_dev_ep);

/** [DECLARE_CLUSTERS] */


void erl_interface_dev_ctx_init();
void erl_interface_dev_clusters_attrs_init(zb_uint8_t param);
void erl_interface_dev_app_init(zb_uint8_t param);

/*********************  Device-specific functions  **************************/

/** Init device context. */
void erl_interface_dev_ctx_init()
{
  TRACE_MSG(TRACE_APP1, ">> erl_interface_dev_ctx_init", (FMT__0));

  ZB_MEMSET(&g_dev_ctx, 0, sizeof(g_dev_ctx));
  ERL_INTERFACE_DEV_PRICE_CTX().price_ack_disabled = ERL_INTERFACE_DISABLE_PRICE_ACK;

  TRACE_MSG(TRACE_APP1, "<< erl_interface_dev_ctx_init", (FMT__0));
}

/** Init device ZCL attributes. */
void erl_interface_dev_clusters_attrs_init(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> erl_interface_dev_clusters_attrs_init", (FMT__0));
  ZVUNUSED(param);

  g_dev_ctx.erl_interface.basic_attrs.zcl_version = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
  g_dev_ctx.erl_interface.basic_attrs.power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

  g_dev_ctx.erl_interface.price_attrs.commodity_type = ZB_ZCL_METERING_ELECTRIC_METERING;

  g_dev_ctx.erl_interface.time_attrs.time = ZB_ZCL_TIME_TIME_INVALID_VALUE;
  g_dev_ctx.erl_interface.time_attrs.time_status = ERL_INTERFACE_TIME_STATUS_INITIAL;
  g_dev_ctx.erl_interface.time_attrs.time_zone = ZB_ZCL_TIME_TIME_ZONE_DEFAULT_VALUE;
  g_dev_ctx.erl_interface.time_attrs.dst_start = ZB_ZCL_TIME_TIME_INVALID_VALUE;
  g_dev_ctx.erl_interface.time_attrs.dst_end = ZB_ZCL_TIME_TIME_INVALID_VALUE;
  g_dev_ctx.erl_interface.time_attrs.dst_shift = ZB_ZCL_TIME_DST_SHIFT_DEFAULT_VALUE;
  g_dev_ctx.erl_interface.time_attrs.standard_time = ZB_ZCL_TIME_TIME_INVALID_VALUE;
  g_dev_ctx.erl_interface.time_attrs.local_time = ZB_ZCL_TIME_TIME_INVALID_VALUE;
  g_dev_ctx.erl_interface.time_attrs.last_set_time = ZB_ZCL_TIME_LAST_SET_TIME_DEFAULT_VALUE;
  g_dev_ctx.erl_interface.time_attrs.valid_until_time = ZB_ZCL_TIME_VALID_UNTIL_TIME_DEFAULT_VALUE;

  g_dev_ctx.erl_interface.calendar_attrs = ERL_CALENDAR_ATTR_LIST_INIT;
  g_dev_ctx.erl_interface.el_measurement_attrs = ERL_EL_MEASUREMENT_ATTR_LIST_INIT;


  g_dev_ctx.erl_interface.metering_attrs = ERL_INTERFACE_METERING_ATTR_LIST_INIT;
  g_dev_ctx.erl_interface.metering_attrs.status = ZB_ZCL_METERING_STATUS_DEFAULT_VALUE;
  g_dev_ctx.erl_interface.metering_attrs.unit_of_measure = ZB_ZCL_METERING_UNIT_KW_KWH_BINARY;
  g_dev_ctx.erl_interface.metering_attrs.summation_formatting =
    ERL_INTERFACE_DEV_SUMMATION_FORMATTING;
  g_dev_ctx.erl_interface.metering_attrs.device_type = ZB_ZCL_METERING_ELECTRIC_METERING;

  g_dev_ctx.phase_metering_el[0].metering_attrs.status = ZB_ZCL_METERING_STATUS_DEFAULT_VALUE;
  g_dev_ctx.phase_metering_el[0].metering_attrs.unit_of_measure =
    ZB_ZCL_METERING_UNIT_KW_KWH_BINARY;
  g_dev_ctx.phase_metering_el[0].metering_attrs.summation_formatting =
    ERL_INTERFACE_DEV_SUMMATION_FORMATTING;
  g_dev_ctx.phase_metering_el[0].metering_attrs.device_type =
    ZB_ZCL_METERING_ELECTRIC_METERING_ELEMENT_PHASE1;

  g_dev_ctx.phase_metering_el[1].metering_attrs.status = ZB_ZCL_METERING_STATUS_DEFAULT_VALUE;
  g_dev_ctx.phase_metering_el[1].metering_attrs.unit_of_measure =
    ZB_ZCL_METERING_UNIT_KW_KWH_BINARY;
  g_dev_ctx.phase_metering_el[1].metering_attrs.summation_formatting =
    ERL_INTERFACE_DEV_SUMMATION_FORMATTING;
  g_dev_ctx.phase_metering_el[1].metering_attrs.device_type =
    ZB_ZCL_METERING_ELECTRIC_METERING_ELEMENT_PHASE2;

    g_dev_ctx.phase_metering_el[2].metering_attrs.status = ZB_ZCL_METERING_STATUS_DEFAULT_VALUE;
  g_dev_ctx.phase_metering_el[2].metering_attrs.unit_of_measure =
    ZB_ZCL_METERING_UNIT_KW_KWH_BINARY;
  g_dev_ctx.phase_metering_el[2].metering_attrs.summation_formatting =
    ERL_INTERFACE_DEV_SUMMATION_FORMATTING;
  g_dev_ctx.phase_metering_el[2].metering_attrs.device_type =
    ZB_ZCL_METERING_ELECTRIC_METERING_ELEMENT_PHASE3;


  TRACE_MSG(TRACE_APP1, "<< erl_interface_dev_clusters_attrs_init", (FMT__0));
}

/** Get application parameters for Price - Get Current Price command.
    Now emulate some real values. */
static void erl_interface_get_price_param(zb_zcl_price_publish_price_payload_t *p)
{
  const char *rate_label = "B"; /* "BASE" */
  p->provider_id = 0x01; /* defined by device */
  ZB_ZCL_SET_STRING_VAL(p->rate_label, rate_label, strlen(rate_label));
  p->issuer_event_id = zb_get_utc_time(); /* always increasing */
  p->unit_of_measure = ZB_ZCL_METERING_UNIT_KW_KWH_BINARY;
  p->currency = 840; /* ISO USD */
  p->price_trailing_digit = 0x01; /* e.g, 222.3 */
  p->price_tier = 0x00; /* don't use price tiering */
  p->start_time = 0x00; /* "now" */
  p->duration_in_minutes = 0x05; /* 5 min */
  p->price = ZB_RANDOM_VALUE(24); /* X.Y$ per KW */

  if (ERL_INTERFACE_DEV_PRICE_CTX().price_ack_disabled)
  {
    /* Clear PriceAck bit in payload if theres is not need to recv PriceAck.
     */
    ERL_INTERFACE_DEV_PRICE_CTX().pp_pl.price_control &= 0xFE;
  }

}

/** [CMD_PUBLISH_PRICE] */
/* Send PublishPrice command and wait for APS ack (and PriceAck if it is
 * available)
 */
static void erl_interface_get_sched_prices_cb_send_pp(zb_uint8_t param);

/* Wait for PriceAck (or for timeout) and send next portion of PublishPrice
 * event.
 */
static void erl_interface_get_sched_prices_cb_on_sent(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> erl_interface_get_sched_prices_cb_on_sent, "
            "param=%hd, state=%hd",
            (FMT__H_H, param, ERL_INTERFACE_DEV_PRICE_CTX().state));
  ERL_INTERFACE_DEV_PRICE_CTX().state = ERL_INTERFACE_DEV_GET_SCHED_PRICES_STATE_FINI;
  ZB_SCHEDULE_APP_ALARM(erl_interface_get_sched_prices_cb_send_pp, 0, ERL_INTERFACE_PRICE_ACK_TIMEOUT);
  TRACE_MSG(TRACE_APP1, "<< erl_interface_get_sched_prices_cb_on_sent", (FMT__0));
}

static void erl_interface_get_sched_prices_cb_send_pp(zb_uint8_t param)
{
  if (!param)
  {
    zb_buf_get_out_delayed(erl_interface_get_sched_prices_cb_send_pp);
    return;
  }

  if ((ERL_INTERFACE_DEV_PRICE_CTX().state == ERL_INTERFACE_DEV_GET_SCHED_PRICES_STATE_WAIT_PRICE_ACK)&&
  (!ERL_INTERFACE_DEV_PRICE_CTX().price_ack_disabled))
  {
    TRACE_MSG(TRACE_APP1, "Ack not received, maybe reschedule (%hd)", (FMT__H, param));
  }
  ERL_INTERFACE_DEV_PRICE_CTX().curr_event++;

  if (ERL_INTERFACE_DEV_PRICE_CTX().nr_events != 0
      && ERL_INTERFACE_DEV_PRICE_CTX().curr_event == ERL_INTERFACE_DEV_PRICE_CTX().nr_events)
  {
    TRACE_MSG(TRACE_APP1, "The last packet was sent.", (FMT__0));
    ERL_INTERFACE_DEV_PRICE_CTX().state = ERL_INTERFACE_DEV_GET_SCHED_PRICES_STATE_FINI;
    zb_buf_free(param);
    return;
  }
  else
  {
    TRACE_MSG(TRACE_APP1, "Attempt to send packet %hd of %hd",
              (FMT__H_H, ERL_INTERFACE_DEV_PRICE_CTX().curr_event, ERL_INTERFACE_DEV_PRICE_CTX().nr_events));
    ERL_INTERFACE_DEV_PRICE_CTX().state = ERL_INTERFACE_DEV_GET_SCHED_PRICES_STATE_WAIT_PRICE_ACK;
  }

  erl_interface_get_price_param(&ERL_INTERFACE_DEV_PRICE_CTX().pp_pl);

  zb_zcl_price_send_cmd_publish_price(param,
   (zb_addr_u *) &ERL_INTERFACE_DEV_PRICE_CTX().cli_addr,
   ERL_INTERFACE_DEV_PRICE_CTX().cli_addr_mode,
   ERL_INTERFACE_DEV_PRICE_CTX().cli_ep,
   ERL_INTERFACE_DEV_ENDPOINT,
   &ERL_INTERFACE_DEV_PRICE_CTX().pp_pl,
   (ERL_INTERFACE_DEV_PRICE_CTX().price_ack_disabled)?erl_interface_get_sched_prices_cb_on_sent:NULL);

   if (!ERL_INTERFACE_DEV_PRICE_CTX().price_ack_disabled)
   {
     ZB_SCHEDULE_APP_ALARM(erl_interface_get_sched_prices_cb_send_pp, 0, ERL_INTERFACE_PRICE_ACK_TIMEOUT);
   }

}
/** [CMD_PUBLISH_PRICE] */

static void erl_interface_get_current_price_cb(zb_uint8_t param)
{
  ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;

  /* Print command input payload */
  TRACE_MSG(TRACE_APP1, "GetCurrentPrice Command Input Payload %hd", (FMT__H, *ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_price_get_current_price_payload_t)));

  /* Get Price parameters */
  erl_interface_get_price_param(ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(param, zb_zcl_price_publish_price_payload_t));
}

static void erl_interface_get_sched_prices_cb(zb_uint8_t param)
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

  /* NOTE: only one client device is supported for now. */
  ERL_INTERFACE_DEV_PRICE_CTX().cli_addr.addr_short = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr;
  ERL_INTERFACE_DEV_PRICE_CTX().cli_addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
  ERL_INTERFACE_DEV_PRICE_CTX().cli_ep = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint;

  ERL_INTERFACE_DEV_PRICE_CTX().nr_events = pl_in->number_of_events;
  ERL_INTERFACE_DEV_PRICE_CTX().curr_event = 0;

  ERL_INTERFACE_DEV_PRICE_CTX().pp_pl = ZB_ZCL_PRICE_PUBLISH_PRICE_PAYLOAD_INIT;

  ERL_INTERFACE_DEV_PRICE_CTX().curr_event = -1;

  zb_buf_get_out_delayed(erl_interface_get_sched_prices_cb_send_pp);
  ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
}



zb_zcl_price_publish_tier_labels_payload_t publish_tier_labels_pl;
#define MAX_NUMBER_OF_LABELS 3
zb_zcl_price_publish_tier_labels_sub_payload_t tier_labels_sub_pl[MAX_NUMBER_OF_LABELS];

zb_zcl_price_get_tier_labels_payload_t get_tier_labels_in_ctx;
zb_zcl_parsed_hdr_t get_tier_labels_cmd_info;


static void erl_interface_price_send_publish_tier_labels(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> erl_interface_price_send_publish_tier_labels", (FMT__0));

  if (!param)
  {
    zb_buf_get_out_delayed(erl_interface_price_send_publish_tier_labels);
  }
  else
  {
    char *tiers[MAX_NUMBER_OF_LABELS] = {"Tier1", "Tier2"};

    TRACE_MSG(TRACE_APP1, "in->issuer_tariff_id = %lx", (FMT__L, get_tier_labels_in_ctx.issuer_tariff_id));

    publish_tier_labels_pl.provider_id = 0x00000001;
    publish_tier_labels_pl.issuer_event_id = 0x00000002;
    publish_tier_labels_pl.issuer_tariff_id = get_tier_labels_in_ctx.issuer_tariff_id;
    publish_tier_labels_pl.command_index = 0;
    publish_tier_labels_pl.total_number_of_commands = 1;
    publish_tier_labels_pl.number_of_labels = 2;
    publish_tier_labels_pl.tier_labels = (zb_zcl_price_publish_tier_labels_sub_payload_t *)&tier_labels_sub_pl;

    for (zb_uint_t i = 0; i < publish_tier_labels_pl.number_of_labels; i++)
    {
      tier_labels_sub_pl[i].tier_id = (zb_uint8_t)i;
      ZB_ZCL_SET_STRING_VAL(tier_labels_sub_pl[i].tier_label, tiers[i], strlen(tiers[i]));
    }

    ZB_ZCL_PRICE_SEND_CMD_PUBLISH_TIER_LABELS(param,
                                              (zb_addr_u *) &ZB_ZCL_PARSED_HDR_SHORT_DATA(&get_tier_labels_cmd_info).source.u.short_addr,
                                              ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                              ZB_ZCL_PARSED_HDR_SHORT_DATA(&get_tier_labels_cmd_info).src_endpoint,
                                              ERL_INTERFACE_DEV_ENDPOINT,
                                              &publish_tier_labels_pl);
  }


  TRACE_MSG(TRACE_APP1, "<< erl_interface_price_send_publish_tier_labels", (FMT__0));
}


static void erl_interface_price_get_tier_labels(zb_uint8_t param)
{
  const zb_zcl_price_get_tier_labels_payload_t *pl_in = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_price_get_tier_labels_payload_t);
  const zb_zcl_parsed_hdr_t *cmd_info = ZB_ZCL_DEVICE_CMD_PARAM_CMD_INFO(param);

  TRACE_MSG(TRACE_APP1, ">> erl_interface_price_get_tier_labels", (FMT__0));

  ZB_MEMCPY(&get_tier_labels_in_ctx, pl_in, sizeof(zb_zcl_price_get_tier_labels_payload_t));
  ZB_MEMCPY(&get_tier_labels_cmd_info, cmd_info, sizeof(zb_zcl_parsed_hdr_t));

  zb_buf_get_out_delayed(erl_interface_price_send_publish_tier_labels);

  ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;

  TRACE_MSG(TRACE_APP1, "<< erl_interface_price_get_tier_labels", (FMT__0));
}


static void erl_interface_price_ack_cb(zb_uint8_t param)
{
  const zb_zcl_price_ack_payload_t *pl_in =
    ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_price_ack_payload_t);
  TRACE_MSG(TRACE_APP1, ">> erl_interface_price_ack_cb, "
            "issuer_event_id = %d, current_time=%d, provider_id = %d",
            (FMT__D_D_D, pl_in->issuer_event_id, pl_in->current_time,
             pl_in->provider_id));

  /* If we are in a middle of sending GetScheduledPrices responses. */
  if (ERL_INTERFACE_DEV_PRICE_CTX().state == ERL_INTERFACE_DEV_GET_SCHED_PRICES_STATE_WAIT_PRICE_ACK)
  {
    /* NOTE: only one client device is supported for now. */
    ERL_INTERFACE_DEV_PRICE_CTX().state = ERL_INTERFACE_DEV_GET_SCHED_PRICES_STATE_FINI;
    ZB_SCHEDULE_APP_ALARM_CANCEL(erl_interface_get_sched_prices_cb_send_pp, ZB_ALARM_ALL_CB);
    ZB_SCHEDULE_APP_ALARM(erl_interface_get_sched_prices_cb_send_pp, 0, ERL_INTERFACE_PRICE_SCHEDULED_PRICES_TIMEOUT);
  }

  ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
  TRACE_MSG(TRACE_APP1, "<< erl_interface_price_ack_cb", (FMT__0));
}

/** [zb_zcl_calendar_send_cmd_publish_calendar] */
void erl_interface_send_publish_calendar(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> erl_interface_send_publish_calendar param %hd", (FMT__H, param));
  if (!param)
  {
    zb_buf_get_out_delayed(erl_interface_send_publish_calendar);
  }
  else
  {
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
                                         &g_dev_ctx.erl_interface.calendar_client_address,
                                         ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                                         g_dev_ctx.erl_interface.calendar_client_ep,
                                         ERL_INTERFACE_DEV_ENDPOINT,
                                         &payload);
  }

  TRACE_MSG(TRACE_APP1, "<< erl_interface_send_publish_calendar", (FMT__0));
}
/** [zb_zcl_calendar_send_cmd_publish_calendar]  */


/** [zb_zcl_calendar_send_cmd_publish_week_profile] */
void erl_interface_send_publish_week_profile(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> erl_interface_send_publish_week_profile param %hd", (FMT__H, param));
  if (!param)
  {
    zb_buf_get_out_delayed(erl_interface_send_publish_week_profile);
  }
  else
  {
    zb_zcl_calendar_publish_week_profile_payload_t payload;

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
                                         &g_dev_ctx.erl_interface.calendar_client_address,
                                         ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                                         g_dev_ctx.erl_interface.calendar_client_ep,
                                         ERL_INTERFACE_DEV_ENDPOINT,
                                         &payload);
                                        }
                                        TRACE_MSG(TRACE_APP1, "<< erl_interface_send_publish_week_profile", (FMT__0));
}
/** [zb_zcl_calendar_send_cmd_publish_week_profile] */

/** [zb_zcl_calendar_send_cmd_cancel_calendar]  */
void erl_interface_send_cancel_calendar(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> erl_interface_send_cancel_calendar param %hd", (FMT__H, param));
  if (!param)
  {
    zb_buf_get_out_delayed(erl_interface_send_cancel_calendar);
  }
  else
  {
    zb_zcl_calendar_cancel_calendar_payload_t payload;
    /*Here we must get events if present and pass them to client*/

    payload.provider_id = 0x01;
    payload.issuer_calendar_id = zb_get_utc_time();
    payload.calendar_type = ZB_ZCL_CALENDAR_TYPE_DELIVERED_CALENDAR;

    ZB_ZCL_CALENDAR_SEND_CMD_CANCEL_CALENDAR(param,
                                        &g_dev_ctx.erl_interface.calendar_client_address,
                                        ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                                        g_dev_ctx.erl_interface.calendar_client_ep,
                                        ERL_INTERFACE_DEV_ENDPOINT,
                                        &payload);
  }

  TRACE_MSG(TRACE_APP1, "<< erl_interface_send_cancel_calendar", (FMT__0));
}
/** [zb_zcl_calendar_send_cmd_cancel_calendar]  */

#if ERL_INTERFACE_ENABLE_CALENDAR_CLUSTER
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

const char message[] = "Test display string";

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


#define ZB_ERL_MAX_INTERVAL_COUNT 5

zb_uint24_t zb_metering_intervals_storage[ZB_ERL_MAX_INTERVAL_COUNT];

static void handle_metering_get_profile(zb_uint8_t param)
{
  const zb_zcl_metering_get_profile_payload_t *pl_in = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_metering_get_profile_payload_t);
  zb_zcl_metering_get_profile_response_payload_t *pl_out = ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(param, zb_zcl_metering_get_profile_response_payload_t);
  zb_uint16_t low_value = 0x00;
  zb_uint_t i;

  TRACE_MSG(TRACE_ZCL1, "Received get_profile from display device", (FMT__0));

  TRACE_MSG(TRACE_APP1, "in->interval_channel = %hx", (FMT__H, pl_in->interval_channel));
  TRACE_MSG(TRACE_APP1, "in->end_time = %lx", (FMT__L, pl_in->end_time));
  TRACE_MSG(TRACE_APP1, "in->number_of_periods = %hx", (FMT__H, pl_in->number_of_periods));

  pl_out->end_time = pl_in->end_time;  /* This value doesn't have any special meaning in this sample*/
  pl_out->status = ZB_ZCL_METERING_SUCCESS;
  pl_out->profile_interval_period = ZB_ZCl_METERING_INTERVAL_PERIOD_DAILY;
  pl_out->number_of_periods_delivered = 3;

  for (i = 0; i < pl_out->number_of_periods_delivered; i++)
  {
    ZB_ASSIGN_UINT24(zb_metering_intervals_storage[i], 0x01, low_value);
    low_value++;
  }

  pl_out->intervals = zb_metering_intervals_storage;

  ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
}


static void handle_metering_request_fast_poll_mode(zb_uint8_t param)
{
  const zb_zcl_metering_request_fast_poll_mode_payload_t *pl_in = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_metering_request_fast_poll_mode_payload_t);
  zb_zcl_metering_request_fast_poll_mode_response_payload_t *pl_out = ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(param, zb_zcl_metering_request_fast_poll_mode_response_payload_t);

  TRACE_MSG(TRACE_ZCL1, "Received request_fast_poll_mode from display device", (FMT__0));

  TRACE_MSG(TRACE_APP1, "in->fast_poll_update_period = %hx", (FMT__H, pl_in->fast_poll_update_period));
  TRACE_MSG(TRACE_APP1, "in->duration_in_minutes = %x", (FMT__D, pl_in->duration_in_minutes));

  pl_out->applied_update_period_in_seconds = pl_in->fast_poll_update_period;
  pl_out->fast_poll_mode_end_time = zb_get_utc_time() + 100;

  ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
}

#define METERING_MAX_SAMPLES 5
zb_uint24_t erl_metering_samples_storage[METERING_MAX_SAMPLES];

static void handle_metering_get_sampled_data(zb_uint8_t param)
{
  const zb_zcl_metering_get_sampled_data_payload_t *pl_in = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_metering_get_sampled_data_payload_t);
  zb_zcl_metering_get_sampled_data_response_payload_t *pl_out = ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(param, zb_zcl_metering_get_sampled_data_response_payload_t);
  zb_uint16_t low_value = 0x00;

  TRACE_MSG(TRACE_ZCL1, "Received get_sampled_data from display device", (FMT__0));

  TRACE_MSG(TRACE_APP1, "in->sample_id = %hx", (FMT__H, pl_in->sample_id));
  TRACE_MSG(TRACE_APP1, "in->earliest_sample_time = %lx", (FMT__L, pl_in->earliest_sample_time));
  TRACE_MSG(TRACE_APP1, "in->sample_type = %hx", (FMT__H, pl_in->sample_type));
  TRACE_MSG(TRACE_APP1, "in->number_of_samples = %d", (FMT__D, pl_in->number_of_samples));

  pl_out->sample_id = pl_in->sample_id;
  pl_out->sample_start_time = zb_get_utc_time();
  pl_out->sample_type = pl_in->sample_type;
  pl_out->sample_request_interval = 20;
  pl_out->number_of_samples = (pl_in->number_of_samples < METERING_MAX_SAMPLES) ? pl_in->number_of_samples : METERING_MAX_SAMPLES;

  for (zb_uint_t i = 0; i < pl_out->number_of_samples; i++)
  {
    ZB_ASSIGN_UINT24(erl_metering_samples_storage[i], 0x02, low_value);
    low_value += 2;
  }

  pl_out->samples = erl_metering_samples_storage;

  ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
}


#define MAX_TIER_SUMMATION 3

zb_uint48_t tier_summation_storage[MAX_TIER_SUMMATION];

#define MAX_TIER_BLOCK_SUMMATION 3

zb_uint48_t tier_block_summation_storage[MAX_TIER_BLOCK_SUMMATION];

zb_zcl_metering_publish_snapshot_payload_t publish_snapshot_pl;

zb_zcl_metering_get_snapshot_payload_t get_snapshot_in_ctx;
zb_zcl_parsed_hdr_t get_snapshot_cmd_info;

static void handle_metering_send_publish_snapshot(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> handle_metering_send_publish_snapshot", (FMT__0));
  if (!param)
  {
    zb_buf_get_out_delayed(handle_metering_send_publish_snapshot);
  }
  else
  {
    TRACE_MSG(TRACE_APP1, "in->earliest_start_time = %hx", (FMT__H, get_snapshot_in_ctx.earliest_start_time));
    TRACE_MSG(TRACE_APP1, "in->latest_end_time = %lx", (FMT__L, get_snapshot_in_ctx.latest_end_time));
    TRACE_MSG(TRACE_APP1, "in->snapshot_offset = %hx", (FMT__H, get_snapshot_in_ctx.snapshot_offset));
    TRACE_MSG(TRACE_APP1, "in->snapshot_cause = %d", (FMT__D, get_snapshot_in_ctx.snapshot_cause));

    publish_snapshot_pl.snapshot_id = 0x00000001;
    publish_snapshot_pl.snapshot_time = zb_get_utc_time();
    publish_snapshot_pl.total_snapshots_found = 8;  /* For all snapshot types */
    publish_snapshot_pl.command_index = 0;
    publish_snapshot_pl.total_number_of_commands = 1;
    publish_snapshot_pl.snapshot_cause = ZB_ZCL_METERING_CAUSE_GENERAL;

    switch (get_snapshot_in_ctx.snapshot_offset)
    {
      case 0:
        publish_snapshot_pl.snapshot_payload_type = ZB_ZCL_METERING_TOU_DELIVERED_REGISTERS;
        break;
      case 1:
        publish_snapshot_pl.snapshot_payload_type = ZB_ZCL_METERING_TOU_RECEIVED_REGISTERS;
        break;
      case 2:
        publish_snapshot_pl.snapshot_payload_type = ZB_ZCL_METERING_BLOCK_TIER_DELIVERED;
        break;
      case 3:
        publish_snapshot_pl.snapshot_payload_type = ZB_ZCL_METERING_BLOCK_TIER_RECEIVED;
        break;
      case 4:
        publish_snapshot_pl.snapshot_payload_type = ZB_ZCL_METERING_TOU_DELIVERED_NO_BILLING;
        break;
      case 5:
        publish_snapshot_pl.snapshot_payload_type = ZB_ZCL_METERING_TOU_RECEIVED_NO_BILLING;
        break;
      case 6:
        publish_snapshot_pl.snapshot_payload_type = ZB_ZCL_METERING_BLOCK_TIER_DELIVERED_NO_BILLING;
        break;
      case 7:
        publish_snapshot_pl.snapshot_payload_type = ZB_ZCL_METERING_BLOCK_TIER_RECEIVED_NO_BILLING;
        break;
      default:
        publish_snapshot_pl.snapshot_payload_type = ZB_ZCL_METERING_DATA_UNAVAILABLE;
        break;
    }

    switch (publish_snapshot_pl.snapshot_payload_type)
    {
      case ZB_ZCL_METERING_TOU_DELIVERED_REGISTERS:
      {
        zb_zcl_metering_tou_delivered_payload_t *sub_pl = &(publish_snapshot_pl.snapshot_sub_payload.tou_delivered);

        TRACE_MSG(TRACE_ZCL1, "Snapshot payload type is ZB_ZCL_METERING_TOU_DELIVERED_REGISTERS", (FMT__0));

        ZB_ASSIGN_UINT48(sub_pl->current_summation_delivered, 0x0001, 0x00000001);
        sub_pl->bill_to_date_delivered = zb_get_utc_time();
        sub_pl->bill_to_date_time_stamp_delivered = 0x01020304;
        sub_pl->projected_bill_delivered = 0x01020304;
        sub_pl->projected_bill_time_stamp_delivered = 0x01020304;
        sub_pl->bill_delivered_trailing_digit = 0x20;
        sub_pl->number_of_tiers_in_use = 2;
        sub_pl->tier_summation = tier_summation_storage;

        for (zb_uint_t i = 0; i < sub_pl->number_of_tiers_in_use; i++)
        {
          ZB_ASSIGN_UINT48((*(sub_pl->tier_summation + i)), (zb_uint16_t)i, 0x00000001);
        }
      }
      break;
      case ZB_ZCL_METERING_TOU_RECEIVED_REGISTERS:
      {
        zb_zcl_metering_tou_received_payload_t *sub_pl = &(publish_snapshot_pl.snapshot_sub_payload.tou_received);

        TRACE_MSG(TRACE_ZCL1, "Snapshot payload type is ZB_ZCL_METERING_TOU_RECEIVED_REGISTERS", (FMT__0));

        ZB_ASSIGN_UINT48(sub_pl->current_summation_received, 0x0002, 0x00000001);
        sub_pl->bill_to_date_received = zb_get_utc_time();
        sub_pl->bill_to_date_time_stamp_received = 0x01020304;
        sub_pl->projected_bill_received = 0x01020304;
        sub_pl->projected_bill_time_stamp_received = 0x01020304;
        sub_pl->bill_received_trailing_digit = 0x02;
        sub_pl->number_of_tiers_in_use = 2;
        sub_pl->tier_summation = tier_summation_storage;

        for (zb_uint_t i = 0; i < sub_pl->number_of_tiers_in_use; i++)
        {
          ZB_ASSIGN_UINT48((*(sub_pl->tier_summation + i)), (zb_uint16_t)i, 0x00000002);
        }
      }
      break;
      case ZB_ZCL_METERING_BLOCK_TIER_DELIVERED:
      {
        zb_zcl_metering_block_delivered_payload_t *sub_pl = &(publish_snapshot_pl.snapshot_sub_payload.block_delivered);

        TRACE_MSG(TRACE_ZCL1, "Snapshot payload type is ZB_ZCL_METERING_BLOCK_TIER_DELIVERED", (FMT__0));

        ZB_ASSIGN_UINT48(sub_pl->current_summation_delivered, 0x0003, 0x00000001);
        sub_pl->bill_to_date_delivered = zb_get_utc_time();
        sub_pl->bill_to_date_time_stamp_delivered = 0x01020304;
        sub_pl->projected_bill_delivered = 0x01020304;
        sub_pl->projected_bill_time_stamp_delivered = 0x01020304;
        sub_pl->bill_delivered_trailing_digit = 0x02;
        sub_pl->number_of_tiers_in_use = 2;
        sub_pl->tier_summation = tier_summation_storage;

        for (zb_uint_t i = 0; i < sub_pl->number_of_tiers_in_use; i++)
        {
          ZB_ASSIGN_UINT48((*(sub_pl->tier_summation + i)), (zb_uint16_t)i, 0x00000003);
        }

        sub_pl->number_of_tiers_and_block_thresholds_in_use = 2;

        sub_pl->tier_block_summation = tier_block_summation_storage;
        for (zb_uint_t i = 0; i < sub_pl->number_of_tiers_and_block_thresholds_in_use; i++)
        {
          ZB_ASSIGN_UINT48((*(sub_pl->tier_block_summation + i)), (zb_uint16_t)i, 0x10000003);
        }
      }
      break;
      case ZB_ZCL_METERING_BLOCK_TIER_RECEIVED:
      {
        zb_zcl_metering_block_received_payload_t *sub_pl = &(publish_snapshot_pl.snapshot_sub_payload.block_received);

        TRACE_MSG(TRACE_ZCL1, "Snapshot payload type is ZB_ZCL_METERING_BLOCK_TIER_RECEIVED", (FMT__0));

        ZB_ASSIGN_UINT48(sub_pl->current_summation_received, 0x0004, 0x00000001);
        sub_pl->bill_to_date_received = zb_get_utc_time();
        sub_pl->bill_to_date_time_stamp_received = 0x01020304;
        sub_pl->projected_bill_received = 0x01020304;
        sub_pl->projected_bill_time_stamp_received = 0x01020304;
        sub_pl->bill_received_trailing_digit = 0x02;
        sub_pl->number_of_tiers_in_use = 2;
        sub_pl->tier_summation = tier_summation_storage;

        for (zb_uint_t i = 0; i < sub_pl->number_of_tiers_in_use; i++)
        {
          ZB_ASSIGN_UINT48((*(sub_pl->tier_summation + i)), (zb_uint16_t)i, 0x00000004);
        }

        sub_pl->number_of_tiers_and_block_thresholds_in_use = 2;

        sub_pl->tier_block_summation = tier_block_summation_storage;
        for (zb_uint_t i = 0; i < sub_pl->number_of_tiers_and_block_thresholds_in_use; i++)
        {
          ZB_ASSIGN_UINT48((*(sub_pl->tier_block_summation + i)), (zb_uint16_t)i, 0x10000004);
        }
      }
      break;
      case ZB_ZCL_METERING_TOU_DELIVERED_NO_BILLING:
      {
        zb_zcl_metering_tou_delivered_no_billing_payload_t *sub_pl = &(publish_snapshot_pl.snapshot_sub_payload.tou_delivered_no_billing);

        TRACE_MSG(TRACE_ZCL1, "Snapshot payload type is ZB_ZCL_METERING_TOU_DELIVERED_NO_BILLING", (FMT__0));

        ZB_ASSIGN_UINT48(sub_pl->current_summation_delivered, 0x0005, 0x00000001);
        sub_pl->number_of_tiers_in_use = 2;
        sub_pl->tier_summation = tier_summation_storage;

        for (zb_uint_t i = 0; i < sub_pl->number_of_tiers_in_use; i++)
        {
          ZB_ASSIGN_UINT48((*(sub_pl->tier_summation + i)), (zb_uint16_t)i, 0x00000005);
        }
      }
      break;
      case ZB_ZCL_METERING_TOU_RECEIVED_NO_BILLING:
      {
        zb_zcl_metering_tou_received_no_billing_payload_t *sub_pl = &(publish_snapshot_pl.snapshot_sub_payload.tou_received_no_billing);

        TRACE_MSG(TRACE_ZCL1, "Snapshot payload type is ZB_ZCL_METERING_TOU_RECEIVED_NO_BILLING", (FMT__0));

        ZB_ASSIGN_UINT48(sub_pl->current_summation_received, 0x0006, 0x00000001);
        sub_pl->number_of_tiers_in_use = 2;
        sub_pl->tier_summation = tier_summation_storage;

        for (zb_uint_t i = 0; i < sub_pl->number_of_tiers_in_use; i++)
        {
          ZB_ASSIGN_UINT48((*(sub_pl->tier_summation + i)), (zb_uint16_t)i, 0x00000006);
        }
      }
      break;
      case ZB_ZCL_METERING_BLOCK_TIER_DELIVERED_NO_BILLING:
      {
        zb_zcl_metering_block_tier_delivered_no_billing_payload_t *sub_pl = &(publish_snapshot_pl.snapshot_sub_payload.block_tier_delivered_no_billing);

        TRACE_MSG(TRACE_ZCL1, "Snapshot payload type is ZB_ZCL_METERING_BLOCK_TIER_DELIVERED_NO_BILLING", (FMT__0));

        ZB_ASSIGN_UINT48(sub_pl->current_summation_delivered, 0x0007, 0x00000001);
        sub_pl->number_of_tiers_in_use = 2;
        sub_pl->tier_summation = tier_summation_storage;

        for (zb_uint_t i = 0; i < sub_pl->number_of_tiers_in_use; i++)
        {
          ZB_ASSIGN_UINT48((*(sub_pl->tier_summation + i)), (zb_uint16_t)i, 0x00000007);
        }

        sub_pl->number_of_tiers_and_block_thresholds_in_use = 2;

        sub_pl->tier_block_summation = tier_block_summation_storage;
        for (zb_uint_t i = 0; i < sub_pl->number_of_tiers_and_block_thresholds_in_use; i++)
        {
          ZB_ASSIGN_UINT48((*(sub_pl->tier_block_summation + i)), (zb_uint16_t)i, 0x10000007);
        }
      }
      break;
      case ZB_ZCL_METERING_BLOCK_TIER_RECEIVED_NO_BILLING:
      {
        zb_zcl_metering_block_tier_received_no_billing_payload_t *sub_pl = &(publish_snapshot_pl.snapshot_sub_payload.block_tier_received_no_billing);

        TRACE_MSG(TRACE_ZCL1, "Snapshot payload type is ZB_ZCL_METERING_BLOCK_TIER_RECEIVED_NO_BILLING", (FMT__0));

        ZB_ASSIGN_UINT48(sub_pl->current_summation_received, 0x0008, 0x00000001);
        sub_pl->number_of_tiers_in_use = 2;
        sub_pl->tier_summation = tier_summation_storage;

        for (zb_uint_t i = 0; i < sub_pl->number_of_tiers_in_use; i++)
        {
          ZB_ASSIGN_UINT48((*(sub_pl->tier_summation + i)), (zb_uint16_t)i, 0x00000008);
        }

        sub_pl->number_of_tiers_and_block_thresholds_in_use = 2;

        sub_pl->tier_block_summation = tier_block_summation_storage;
        for (zb_uint_t i = 0; i < sub_pl->number_of_tiers_and_block_thresholds_in_use; i++)
        {
          ZB_ASSIGN_UINT48((*(sub_pl->tier_block_summation + i)), (zb_uint16_t)i, 0x10000008);
        }
      }
      break;
      case ZB_ZCL_METERING_DATA_UNAVAILABLE:
        TRACE_MSG(TRACE_ZCL1, "Snapshot payload type is ZB_ZCL_METERING_DATA_UNAVAILABLE", (FMT__0));
        break;
      default:
        TRACE_MSG(TRACE_ZCL1, "unsupported snapshot payload type", (FMT__0));
        break;
    }

    ZB_ZCL_METERING_SEND_CMD_PUBLISH_SNAPSHOT(param,
                                              (zb_addr_u *) &ZB_ZCL_PARSED_HDR_SHORT_DATA(&get_snapshot_cmd_info).source.u.short_addr,
                                              ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                              ZB_ZCL_PARSED_HDR_SHORT_DATA(&get_snapshot_cmd_info).src_endpoint,
                                              ERL_INTERFACE_DEV_ENDPOINT,
                                              &publish_snapshot_pl,
                                              NULL);
  }

  TRACE_MSG(TRACE_APP1, "<< handle_metering_send_publish_snapshot", (FMT__0));
}


static void handle_metering_get_snapshot(zb_uint8_t param)
{
  const zb_zcl_metering_get_snapshot_payload_t *pl_in = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_metering_get_snapshot_payload_t);
  const zb_zcl_parsed_hdr_t *cmd_info = ZB_ZCL_DEVICE_CMD_PARAM_CMD_INFO(param);

  TRACE_MSG(TRACE_ZCL1, "Received get_snapshot from display device", (FMT__0));

  ZB_MEMCPY(&get_snapshot_in_ctx, pl_in, sizeof(zb_zcl_metering_get_snapshot_payload_t));
  ZB_MEMCPY(&get_snapshot_cmd_info, cmd_info, sizeof(zb_zcl_parsed_hdr_t));

  zb_buf_get_out_delayed(handle_metering_send_publish_snapshot);

  ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
}


/** Application callback for ZCL commands. Used for:
    - receiving application-specific values for commands (e.g. ZB_ZCL_PRICE_GET_CURRENT_PRICE_CB_ID)
    - providing received ZCL commands data to application
    Application may ignore callback id-s in which it is not interested.
 */
static void erl_interface_zcl_cmd_device_cb(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> erl_interface_zcl_cmd_device_cb(param=%hd, id=%d)",
            (FMT__H_D, param, ZB_ZCL_DEVICE_CMD_PARAM_CB_ID(param)));

  switch (ZB_ZCL_DEVICE_CMD_PARAM_CB_ID(param))
  {
    case ZB_ZCL_PRICE_GET_CURRENT_PRICE_CB_ID:
      /* 04/03/2018 EE CR:MINOR Why braces? Here and below. */
      {
        TRACE_MSG(TRACE_APP1, "ZB_ZCL_PRICE_GET_CURRENT_PRICE_CB_ID", (FMT__0));
        erl_interface_get_current_price_cb(param);
      }
      break;
    case ZB_ZCL_PRICE_GET_SCHEDULED_PRICES_CB_ID:
      {
        TRACE_MSG(TRACE_APP1, "ZB_ZCL_PRICE_GET_SCHEDULED_PRICES_CB_ID", (FMT__0));
        erl_interface_get_sched_prices_cb(param);
      }
      break;
    case ZB_ZCL_PRICE_PRICE_ACK_CB_ID:
      {
        TRACE_MSG(TRACE_APP1, "ZB_ZCL_PRICE_PRICE_ACK_CB_ID", (FMT__0));
        erl_interface_price_ack_cb(param);
      }
      break;
    case ZB_ZCL_PRICE_GET_TIER_LABELS_CB_ID:
      {
        TRACE_MSG(TRACE_APP1, "ZB_ZCL_PRICE_GET_TIER_LABELS_CB_ID", (FMT__0));
        erl_interface_price_get_tier_labels(param);
      }
      break;
    case ZB_ZCL_MESSAGING_GET_LAST_MSG_CB_ID:
    {
        TRACE_MSG(TRACE_APP1, "ZB_ZCL_MESSAGING_GET_LAST_MSG_CB_ID", (FMT__0));
        ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
        handle_get_last_message(ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(param, zb_zcl_messaging_get_last_message_response_t));
    }
    break;
#if ERL_INTERFACE_ENABLE_CALENDAR_CLUSTER
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
    case ZB_ZCL_METERING_GET_PROFILE_CB_ID:
    {
      TRACE_MSG(TRACE_APP1, "ZB_ZCL_METERING_GET_PROFILE_CB_ID", (FMT__0));

      handle_metering_get_profile(param);
    }
    break;
    case ZB_ZCL_METERING_REQUEST_FAST_POLL_MODE_CB_ID:
    {
      TRACE_MSG(TRACE_APP1, "ZB_ZCL_METERING_REQUEST_FAST_POLL_MODE_CB_ID", (FMT__0));

      handle_metering_request_fast_poll_mode(param);
    }
    break;
    case ZB_ZCL_METERING_GET_SAMPLED_DATA_CB_ID:
    {
      TRACE_MSG(TRACE_APP1, "ZB_ZCL_METERING_GET_SAMPLED_DATA_CB_ID", (FMT__0));

      handle_metering_get_sampled_data(param);
    }
    break;
    case ZB_ZCL_METERING_GET_SNAPSHOT_CB_ID:
    {
      TRACE_MSG(TRACE_APP1, "ZB_ZCL_METERING_GET_SNAPSHOT_CB_ID", (FMT__0));

      handle_metering_get_snapshot(param);
    }
    break;
#endif
    default:
      TRACE_MSG(TRACE_APP1, "Undefined callback was called, id=%d",
                (FMT__D, ZB_ZCL_DEVICE_CMD_PARAM_CB_ID(param)));
      break;
  }

  TRACE_MSG(TRACE_APP1, "<< erl_interface_zcl_cmd_device_cb", (FMT__0));
}

/** [ERL_INTERFACE_DEV_INIT] */
/* Application init: configure stack parameters (IEEE address, device role, channel mask etc),
 * register device context and ZCL cmd device callback), init application attributes/context. */
void erl_interface_dev_app_init(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> erl_interface_dev_app_init", (FMT__0));
/** [ERL_INTERFACE_DEV_INIT] */
  ZVUNUSED(param);

  /** [REGISTER_DEVICE_CTX] */
  ZB_AF_REGISTER_DEVICE_CTX(&erl_interface_dev_zcl_ctx);
  /** [REGISTER_DEVICE_CTX] */

  /* device configuration */
  erl_interface_dev_ctx_init();
  erl_interface_dev_clusters_attrs_init(0);
  ZB_ZCL_REGISTER_DEVICE_CB(erl_interface_zcl_cmd_device_cb);
  /*zb_register_zboss_callback(ZB_ZCL_DEVICE_CB, SET_ZBOSS_CB(erl_interface_zcl_cmd_device_cb));*/

  /* ZB configuration */
  zb_set_long_address(g_dev_addr);
/** [ERL_INTERFACE_DEV_SET_ROLE] */
  zb_set_network_coordinator_role(ERL_INTERFACE_DEV_CHANNEL_MASK);
/** [ERL_INTERFACE_DEV_SET_ROLE] */
  zb_set_max_children(2);

  TRACE_MSG(TRACE_APP1, "<< erl_interface_dev_app_init", (FMT__0));
}

static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster)
{
  TRACE_MSG(TRACE_ZCL1, "finding_binding_cb status %d addr " TRACE_FORMAT_64 " ep %hd cluster %d",
            (FMT__D_A_H_D, status, TRACE_ARG_64(addr), ep, cluster));

  if (status == ZB_BDB_COMM_BIND_ASK_USER)
  {
    if (cluster == ZB_ZCL_CLUSTER_ID_CALENDAR)
    {
      ZB_IEEE_ADDR_COPY(g_dev_ctx.erl_interface.calendar_client_address.addr_long, addr);
      g_dev_ctx.erl_interface.calendar_client_ep = ep;
      ZB_SCHEDULE_APP_ALARM_CANCEL(erl_interface_send_publish_calendar, ZB_ALARM_ANY_PARAM);
      zb_buf_get_out_delayed(erl_interface_send_publish_calendar);
    }
    if (cluster == ZB_ZCL_CLUSTER_ID_PRICE)
    {
      ZB_IEEE_ADDR_COPY(ERL_INTERFACE_DEV_PRICE_CTX().cli_addr.addr_long, addr);
      ERL_INTERFACE_DEV_PRICE_CTX().cli_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
      ERL_INTERFACE_DEV_PRICE_CTX().cli_ep = ep;
    }
  }

  return ZB_TRUE;
}

void erl_interface_finding_binding_initiator(zb_uint8_t param)
{
  ZVUNUSED(param);
  zb_bdb_finding_binding_initiator(ERL_INTERFACE_DEV_ENDPOINT, finding_binding_cb);
}

/** Generate and accumulate measurements - emulate real device. */
static void erl_interface_dev_measure(zb_uint8_t param)
{
  zb_uint48_t instantaneous_consumption;
  zb_uint32_t measure;
  zb_uint48_t new_curr_summ_delivered;
  zb_uint8_t new_metering_status;

  ZVUNUSED(param);

  measure = (zb_uint32_t)ZB_RANDOM_VALUE(7);
  zb_uint64_to_uint48((zb_uint64_t)measure, &instantaneous_consumption);
  new_curr_summ_delivered = g_dev_ctx.erl_interface.metering_attrs.curr_summ_delivered;

  zb_uint48_add(&new_curr_summ_delivered, &instantaneous_consumption, &new_curr_summ_delivered);

  ZB_ZCL_SET_ATTRIBUTE(ERL_INTERFACE_DEV_ENDPOINT,
                       ZB_ZCL_CLUSTER_ID_METERING,
                       ZB_ZCL_CLUSTER_SERVER_ROLE,
                       ZB_ZCL_ATTR_METERING_CURRENT_SUMMATION_DELIVERED_ID,
                       (zb_uint8_t *)&new_curr_summ_delivered,
                       ZB_FALSE);

  new_metering_status = (1L << measure);

  ZB_ZCL_SET_ATTRIBUTE(ERL_INTERFACE_DEV_ENDPOINT,
                       ZB_ZCL_CLUSTER_ID_METERING,
                       ZB_ZCL_CLUSTER_SERVER_ROLE,
                       ZB_ZCL_ATTR_METERING_STATUS_ID,
                       (zb_uint8_t *)&new_metering_status,
                       ZB_FALSE);

  TRACE_MSG(TRACE_APP1, "instantaneous gas consumption: %ld %ld",
            (FMT__L_L, instantaneous_consumption.high, instantaneous_consumption.low));

  ZB_SCHEDULE_APP_ALARM(erl_interface_dev_measure, 0, ERL_INTERFACE_DEV_MEASURE_TIMEOUT);
}

void erl_interface_dev_default_reporting_init(zb_uint8_t param)
{
  zb_zcl_reporting_info_t rep_info;

  TRACE_MSG(TRACE_APP1, ">> metering_dev_default_reporting_init", (FMT__0));
  ZVUNUSED(param);

  ZB_BZERO(&rep_info, sizeof(zb_zcl_reporting_info_t));
  rep_info.direction = ZB_ZCL_CONFIGURE_REPORTING_SEND_REPORT;
  rep_info.ep = ERL_INTERFACE_DEV_ENDPOINT;
  rep_info.cluster_id = ZB_ZCL_CLUSTER_ID_METERING;
  rep_info.cluster_role = ZB_ZCL_CLUSTER_SERVER_ROLE;
  rep_info.dst.profile_id = ZB_AF_HA_PROFILE_ID;

  rep_info.u.send_info.min_interval = 30/* ZB_METERING_DATA_UPDATE_RATE */;
  rep_info.u.send_info.max_interval = 60;
  rep_info.u.send_info.def_min_interval = 30/* ZB_METERING_DATA_UPDATE_RATE */;
  rep_info.u.send_info.def_max_interval = 60;
  rep_info.u.send_info.delta.u32 = 0;

  rep_info.attr_id = ZB_ZCL_ATTR_METERING_CURRENT_SUMMATION_DELIVERED_ID;
  zb_zcl_put_reporting_info(&rep_info, ZB_FALSE);

  rep_info.attr_id = ZB_ZCL_ATTR_METERING_STATUS_ID;
  zb_zcl_put_reporting_info(&rep_info, ZB_FALSE);

  TRACE_MSG(TRACE_APP1, "<< metering_dev_default_reporting_init", (FMT__0));
}

/*********************  ERL ZC  **************************/

MAIN()
{
  ARGV_UNUSED;

  ZB_SET_TRACE_ON();
  ZB_SET_TRAF_DUMP_ON();

  ZB_INIT("erl_interface_device");

  erl_interface_dev_app_init(0);

  if (zboss_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zboss_start() failed", (FMT__0));
  }
  else
  {
    zboss_main_loop();
  }

  TRACE_DEINIT();
  MAIN_RETURN(0);
}

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
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APP1, "Device STARTED OK: first_start %hd",
                  (FMT__H, sig == ZB_BDB_SIGNAL_DEVICE_FIRST_START));
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        erl_interface_dev_default_reporting_init(0);
        erl_interface_dev_measure(0);
        break;

      case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APP1, "Successfull steering, start f&b target", (FMT__0));
        zb_bdb_finding_binding_target(ERL_INTERFACE_DEV_ENDPOINT);
        break;

      case ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED:
        TRACE_MSG(TRACE_APP1, "Finding&binding target done, start initiator", (FMT__0));
        ZB_SCHEDULE_APP_ALARM(erl_interface_finding_binding_initiator, 0, 10 * ZB_TIME_ONE_SECOND);
        break;

      case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:
        TRACE_MSG(TRACE_APP1, "Finding&binding initiator done", (FMT__0));
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
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_APP1, "<< zboss_signal_handler", (FMT__0));
}
/** [SIGNAL_HANDLER] */
