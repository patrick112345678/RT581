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
/* PURPOSE: In-Home Display ZR device sample
*/

#define ZB_TRACE_FILE_ID 40033
#include "zboss_api.h"
#include "zb_in_home_display.h"

/** [IHD_DEV_DEFINE_PARAMS] */
#define IHD_DEV_ENDPOINT 10
#define IHD_DEV_CHANNEL_MASK (1l<<21)
/** [IHD_DEV_DEFINE_PARAMS] */

#define IHD_DEV_ADDR          {0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb}
static zb_ieee_addr_t g_dev_addr = IHD_DEV_ADDR;

/** Send GetCurrentPrice command every minute. */
#define IHD_PRICE_UPDATE_PERIOD (60 * ZB_TIME_ONE_SECOND)
#define IHD_MESSAGE_UPDATE_PERIOD (70 * ZB_TIME_ONE_SECOND)
#define IHD_CALENDAR_UPDATE_PERIOD (80 * ZB_TIME_ONE_SECOND)
#define IHD_CALENDAR_CMD_PERIOD (15 * ZB_TIME_ONE_SECOND)
#define IHD_METERING_PERIOD (5 * ZB_TIME_ONE_SECOND)

#define IHD_APP_DATA_PERSISTING_TIMEOUT 60 * ZB_TIME_ONE_SECOND

#define IHD_CMD_RETRY_TIMEOUT (5 * ZB_TIME_ONE_SECOND)

/** Enable/disable sending GetScheduledPrices command.
 * This command will be sent instead of every second GetCurrentPrices command.
 */
#define IHD_ENABLE_GET_SCHEDULED_PRICES 1

#define IHD_ENABLE_GET_CALENDAR 1

#define IHD_DEV_ENT_NUMBER 15

#define IHD_PROVIDER_ID 0x1E

typedef enum ihd_dev_ent_type_e
{
  IHD_DEV_FREE_ENT = 0x00,
  IHD_DEV_COMMON_ENT,
  IHD_DEV_METERING_ENT,
  IHD_DEV_PRICE_ENT
} ihd_dev_ent_type_t;

typedef enum ihd_dev_ent_pending_cmd_type_e
{
  IHD_DEV_NO_CMD = 0x00,
  IHD_DEV_METERING_DISCOVER_ATTRS,
  IHD_DEV_METERING_READ_FORMATTING,
  IHD_DEV_METERING_GET_PROFILE,
  IHD_DEV_METERING_GET_SAMPLED_DATA,
  IHD_DEV_METERING_REQUEST_FAST_POLL_MODE,
  IHD_DEV_METERING_GET_SNAPSHOT,
  IHD_DEV_PRICE_GET_CURRENT_PRICE,
  IHD_DEV_PRICE_GET_SCHEDULED_PRICES,
  IHD_DEV_PRICE_GET_TIER_LABELS,
  IHD_DEV_MESSAGING_GET_LAST_MESSAGE,
  IHD_DEV_CALENDAR_GET_CALENDAR,
  IHD_DEV_CALENDAR_GET_DAY_PROFILES,
  IHD_DEV_CALENDAR_GET_SEASONS,
  IHD_DEV_CALENDAR_GET_WEEK_PROFILES,
  IHD_DEV_CALENDAR_GET_CALENDAR_CANCELLATION,
  IHD_DEV_CALENDAR_GET_SPECIAL_DAYS
} ihd_dev_ent_pending_cmd_type_t;

typedef ZB_PACKED_PRE struct ihd_dev_ent_s
{
  zb_uint8_t used;
  zb_uint16_t cluster_id;
  zb_ieee_addr_t dev_addr;                        /**< device address */
  zb_uint8_t dev_ep;                          /**< endpoint */
  zb_uint8_t pending_cmd;

  union {
    ZB_PACKED_PRE struct ihd_dev_metering_ent_s
    {
      zb_uint48_t curr_summ_delivered;          /**< CurrentSummationDelivered attribute value */
      zb_uint8_t device_type;                   /**< MeteringDeviceType attribute value */
      zb_uint8_t unit_of_measure;               /**< UnitOfMeasure attribute value */
      /* CurrentSummationDelivered parsed values */
      zb_uint8_t summ_fmt_left;                 /**< number of digits left to the decimal point */
      zb_uint8_t summ_fmt_right;                /**< number of digits right to the decimal point */
      zb_uint8_t summ_fmt_suppr;                /**< flags whether to suppress leading zeroes */
      zb_uint8_t status;                        /**< Status attribute value */
    } ZB_PACKED_STRUCT metering;
    ZB_PACKED_PRE struct ihd_dev_price_ent_s
    {
      zb_uint8_t commodity_type;
    } ZB_PACKED_STRUCT price;
  } u;

  zb_uint8_t align[3];
} ZB_PACKED_STRUCT ihd_dev_ent_t;

/** @struct ihd_dev_nvram_data_s
 *  @brief info about metering device stored in NVRAM
 */
typedef ZB_PACKED_PRE struct ihd_dev_nvram_data_s
{
  ihd_dev_ent_t lst[IHD_DEV_ENT_NUMBER];
} ZB_PACKED_STRUCT ihd_dev_nvram_data_t;

ZB_ASSERT_IF_NOT_ALIGNED_TO_4(ihd_dev_nvram_data_t);
ZB_ASSERT_IF_NOT_ALIGNED_TO_4(ihd_dev_ent_t);


/** [DECLARE_CLUSTERS] */
/*********************  Clusters' attributes  **************************/

typedef struct ihd_dev_identify_attrs_s
{
  zb_uint16_t identify_time;
} ihd_dev_identify_attrs_t;

/** @struct ihd_device_ctx_s
 *  @brief Ih-Home device context
 */
typedef struct ihd_dev_ctx_s
{
  zb_bool_t first_measurement_received;

  /* display device attributes */
  zb_zcl_basic_attrs_t basic_attrs;        /**< Basic cluster attributes  */
  ihd_dev_identify_attrs_t identify_attrs;

  /* attributes of discovered devices */
  ihd_dev_nvram_data_t dev;
} ihd_dev_ctx_t;


/* device context */
static ihd_dev_ctx_t g_dev_ctx;


/* Basic cluster attributes */
ZB_ZCL_DECLARE_BASIC_ATTR_LIST(basic_attr_list, g_dev_ctx.basic_attrs);

/* Identify cluster attributes */
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_dev_ctx.identify_attrs.identify_time);

/*********************  Device declaration  **************************/

ZB_DECLARE_IN_HOME_DISPLAY_CLUSTER_LIST(ihd_dev_clusters, basic_attr_list,
                                           identify_attr_list);

ZB_DECLARE_IN_HOME_DISPLAY_EP(ihd_dev_ep, IHD_DEV_ENDPOINT, ihd_dev_clusters);

ZB_DECLARE_IN_HOME_DISPLAY_CTX(ihd_dev_zcl_ctx, ihd_dev_ep);

/** [DECLARE_CLUSTERS] */


static void ihd_dev_show_attr(zb_uint16_t attr_id, zb_uint8_t dev_idx);

static void ihd_dev_save_data(zb_uint8_t param);
void ihd_dev_nvram_read_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length);
zb_ret_t ihd_dev_nvram_write_app_data(zb_uint8_t page, zb_uint32_t pos);
zb_uint16_t ihd_dev_get_nvram_data_size();
void ihd_dev_disc_attr_resp_handler(zb_bufid_t cmd_buf, zb_zcl_parsed_hdr_t *cmd_info);

static zb_uint8_t ihd_dev_zcl_cmd_handler(zb_uint8_t param);
static void ihd_dev_ctx_init();
static void ihd_dev_clusters_attrs_init(zb_uint8_t param);
static void ihd_dev_app_init(zb_uint8_t param);
static void ihd_dev_read_metering_attrs(zb_bufid_t buf, zb_zcl_parsed_hdr_t *cmd_info);
static void ihd_dev_metering_get_profile(zb_uint8_t param, zb_uint16_t user_param);
static void ihd_dev_metering_get_sampled_data(zb_uint8_t param, zb_uint16_t user_param);
static void ihd_dev_metering_request_fast_poll_mode(zb_uint8_t param, zb_uint16_t user_param);
static void ihd_dev_metering_get_snapshot(zb_uint8_t param, zb_uint16_t user_param);
static void ihd_dev_reporting_cb(zb_zcl_addr_t *addr, zb_uint8_t ep, zb_uint16_t cluster_id,
                       zb_uint16_t attr_id, zb_uint8_t attr_type, zb_uint8_t *value);

/*********************  Debug functions  **************************/
void dump_traf_str(zb_uint8_t *buf, zb_ushort_t len)
{
  zb_ushort_t i;

  ZVUNUSED(buf);
  ZVUNUSED(len);

  for (i = 0; i < len; i++)
  {
    TRACE_MSG(TRACE_APP1, "'%c'", (FMT__C, (buf[i])));
  }
}
/*********************  Device-specific functions  **************************/

static void ihd_dev_send_pending_cmd(zb_uint8_t i);

/** Add remote device's address, cluster id and endpoint to IHD device list. */
static zb_uint8_t ihd_dev_add_to_list(zb_ieee_addr_t addr, zb_uint16_t cluster_id, zb_uint8_t endpoint)
{
  zb_uint8_t i = 0;

  TRACE_MSG(TRACE_APP1, "ihd_dev_add_to_list: cluster_id 0x%x endpoint %hd", (FMT__D_H, cluster_id, endpoint));
  TRACE_MSG(TRACE_APP1, "addr " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(addr)));
  while (i < ZB_ARRAY_SIZE(g_dev_ctx.dev.lst))
  {
    if (g_dev_ctx.dev.lst[i].used &&
        g_dev_ctx.dev.lst[i].cluster_id == cluster_id &&
        endpoint == g_dev_ctx.dev.lst[i].dev_ep &&
        ZB_IEEE_ADDR_CMP(g_dev_ctx.dev.lst[i].dev_addr, addr))
    {
      /* record already exists, clear all pending cmds for this entry */
      ZB_SCHEDULE_APP_ALARM_CANCEL(ihd_dev_send_pending_cmd, i);
      g_dev_ctx.dev.lst[i].pending_cmd = IHD_DEV_NO_CMD;
      return i;
    }
    if (!g_dev_ctx.dev.lst[i].used)
    {
      /* add new entry */
      TRACE_MSG(TRACE_APP1, "add to idx %hd", (FMT__H, i));
      ZB_BZERO(&g_dev_ctx.dev.lst[i], sizeof(ihd_dev_ent_t));
      ZB_IEEE_ADDR_COPY(g_dev_ctx.dev.lst[i].dev_addr, addr);
      g_dev_ctx.dev.lst[i].dev_ep = endpoint;
      g_dev_ctx.dev.lst[i].cluster_id = cluster_id;
      g_dev_ctx.dev.lst[i].used = 1;
      return i;
    }
    ++i;
  }
  TRACE_MSG(TRACE_APP1, "no free space for ent", (FMT__0));
  return 0xFF;
}

/** Get index in device list by address, cluster id and endpoint. */
static zb_uint8_t ihd_dev_get_idx(zb_ieee_addr_t addr, zb_uint16_t cluster_id, zb_uint8_t endpoint)
{
  zb_uint8_t i = 0;

  TRACE_MSG(TRACE_APP1, "ihd_dev_get_idx: cluster_id 0x%x endpoint %hd", (FMT__D_H, cluster_id, endpoint));
  TRACE_MSG(TRACE_APP1, "addr " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(addr)));
  while (i < ZB_ARRAY_SIZE(g_dev_ctx.dev.lst))
  {
    if (g_dev_ctx.dev.lst[i].used &&
        g_dev_ctx.dev.lst[i].cluster_id == cluster_id &&
        endpoint == g_dev_ctx.dev.lst[i].dev_ep &&
        ZB_IEEE_ADDR_CMP(g_dev_ctx.dev.lst[i].dev_addr, addr))
    {
      TRACE_MSG(TRACE_APP1, "found on idx %hd", (FMT__H, i));
      return i;
    }
    ++i;
  }
  TRACE_MSG(TRACE_APP1, "ent not found", (FMT__H, i));
  return 0xFF;
}


static zb_uint8_t ihd_dev_get_idx_by_cmd_info(const zb_zcl_parsed_hdr_t *in_cmd_info)
{
  zb_ieee_addr_t ieee_addr;
  zb_uint8_t dev_idx;

  if (zb_address_ieee_by_short(ZB_ZCL_PARSED_HDR_SHORT_DATA(in_cmd_info).source.u.short_addr, ieee_addr) == RET_OK)
  {
    dev_idx = ihd_dev_get_idx(ieee_addr,
                              in_cmd_info->cluster_id,
                              ZB_ZCL_PARSED_HDR_SHORT_DATA(in_cmd_info).src_endpoint);

    if (dev_idx != 0xFF && ZB_ZCL_PARSED_HDR_SHORT_DATA(in_cmd_info).source.u.short_addr ==
        zb_address_short_by_ieee(g_dev_ctx.dev.lst[dev_idx].dev_addr))
    {
      return dev_idx;
    }
  }
  return 0xff;
}


static void ihd_dev_read_formatting_attr(zb_uint8_t param, zb_uint16_t user_param);
static void ihd_dev_cmd_get_current_price(zb_uint8_t param, zb_uint16_t user_param);
static void ihd_dev_cmd_get_tier_labels(zb_uint8_t param, zb_uint16_t user_param);
static void ihd_dev_cmd_get_last_message(zb_uint8_t param, zb_uint16_t user_param);
#if IHD_ENABLE_GET_CALENDAR
static void ihd_dev_cmd_get_calendar(zb_uint8_t param, zb_uint16_t user_param);
static void ihd_dev_cmd_get_day_profiles(zb_uint8_t param, zb_uint16_t user_param);
static void ihd_dev_cmd_get_week_profiles(zb_uint8_t param, zb_uint16_t user_param);
static void ihd_dev_cmd_get_seasons(zb_uint8_t param, zb_uint16_t user_param);
static void ihd_dev_cmd_get_special_days(zb_uint8_t param, zb_uint16_t user_param);
static void ihd_dev_cmd_get_calendar_cancellation(zb_uint8_t param, zb_uint16_t user_param);
#endif
static void ihd_dev_cmd_get_scheduled_prices(zb_uint8_t param, zb_uint16_t user_param);
void ihd_dev_discover_metering_attrs(zb_uint8_t param, zb_uint16_t user_param);

/** Send pending command for device by index in device list. */
static void ihd_dev_send_pending_cmd(zb_uint8_t i)
{
  if (g_dev_ctx.dev.lst[i].used &&
      g_dev_ctx.dev.lst[i].pending_cmd)
  {
    TRACE_MSG(TRACE_APP1, "ihd_dev_send_pending_cmd: idx %hd pending_cmd %hd", (FMT__H_H, i, g_dev_ctx.dev.lst[i].pending_cmd));
    switch (g_dev_ctx.dev.lst[i].pending_cmd)
    {
      case IHD_DEV_METERING_DISCOVER_ATTRS:
        zb_buf_get_out_delayed_ext(ihd_dev_discover_metering_attrs, i, 0);
        break;
      case IHD_DEV_METERING_READ_FORMATTING:
        zb_buf_get_out_delayed_ext(ihd_dev_read_formatting_attr, i, 0);
        break;
      case IHD_DEV_METERING_GET_PROFILE:
        zb_buf_get_out_delayed_ext(ihd_dev_metering_get_profile, i, 0);
        break;
      case IHD_DEV_METERING_REQUEST_FAST_POLL_MODE:
        zb_buf_get_out_delayed_ext(ihd_dev_metering_request_fast_poll_mode, i, 0);
        break;
      case IHD_DEV_METERING_GET_SAMPLED_DATA:
        zb_buf_get_out_delayed_ext(ihd_dev_metering_get_sampled_data, i, 0);
        break;
      case IHD_DEV_METERING_GET_SNAPSHOT:
        zb_buf_get_out_delayed_ext(ihd_dev_metering_get_snapshot, i, 0);
        break;
      case IHD_DEV_PRICE_GET_CURRENT_PRICE:
        zb_buf_get_out_delayed_ext(ihd_dev_cmd_get_current_price, i, 0);
        break;
      case IHD_DEV_PRICE_GET_SCHEDULED_PRICES:
        zb_buf_get_out_delayed_ext(ihd_dev_cmd_get_scheduled_prices, i, 0);
        break;
      case IHD_DEV_PRICE_GET_TIER_LABELS:
        zb_buf_get_out_delayed_ext(ihd_dev_cmd_get_tier_labels, i, 0);
        break;
      case IHD_DEV_MESSAGING_GET_LAST_MESSAGE:
        zb_buf_get_out_delayed_ext(ihd_dev_cmd_get_last_message, i, 0);
        break;
      case IHD_DEV_CALENDAR_GET_CALENDAR:
#if IHD_ENABLE_GET_CALENDAR
        zb_buf_get_out_delayed_ext(ihd_dev_cmd_get_calendar, i, 0);
#endif
        break;
      case IHD_DEV_CALENDAR_GET_DAY_PROFILES:
        zb_buf_get_out_delayed_ext(ihd_dev_cmd_get_day_profiles, i, 0);
        break;
      case IHD_DEV_CALENDAR_GET_SEASONS:
        zb_buf_get_out_delayed_ext(ihd_dev_cmd_get_seasons, i, 0);
        break;
      case IHD_DEV_CALENDAR_GET_WEEK_PROFILES:
        zb_buf_get_out_delayed_ext(ihd_dev_cmd_get_week_profiles, i, 0);
        break;
      case IHD_DEV_CALENDAR_GET_CALENDAR_CANCELLATION:
        zb_buf_get_out_delayed_ext(ihd_dev_cmd_get_calendar_cancellation, i, 0);
        break;
      case IHD_DEV_CALENDAR_GET_SPECIAL_DAYS:
        zb_buf_get_out_delayed_ext(ihd_dev_cmd_get_special_days, i, 0);
        break;
      default:
        /* Should not happen! */
        TRACE_MSG(TRACE_ERROR, "unknown pending_cmd!", (FMT__0));
        break;
    }
    g_dev_ctx.dev.lst[i].pending_cmd = IHD_DEV_NO_CMD;
  }
}

/** Trace remote device's attributes by attribute id. */
static void ihd_dev_show_attr(zb_uint16_t attr_id, zb_uint8_t dev_idx)
{
  switch (attr_id)
  {
    case ZB_ZCL_ATTR_METERING_UNIT_OF_MEASURE_ID:
      TRACE_MSG(TRACE_APP1, "Unit of measure: 0x%x",
                (FMT__D, g_dev_ctx.dev.lst[dev_idx].u.metering.unit_of_measure));
      break;

    case ZB_ZCL_ATTR_METERING_SUMMATION_FORMATTING_ID:
      TRACE_MSG(TRACE_APP1, "Summation formatting: %d.%d, suppress leading zeroes: %d",
                (FMT__D_D_D, g_dev_ctx.dev.lst[dev_idx].u.metering.summ_fmt_left,
                 g_dev_ctx.dev.lst[dev_idx].u.metering.summ_fmt_right,
                 g_dev_ctx.dev.lst[dev_idx].u.metering.summ_fmt_suppr));
      break;

    case ZB_ZCL_ATTR_METERING_CURRENT_SUMMATION_DELIVERED_ID:
    {
      TRACE_MSG(TRACE_APP1, "Current Summation Delivered: %ld %ld",
                (FMT__L_L, g_dev_ctx.dev.lst[dev_idx].u.metering.curr_summ_delivered.high,
                 g_dev_ctx.dev.lst[dev_idx].u.metering.curr_summ_delivered.low));
    }
    break;

    case ZB_ZCL_ATTR_METERING_STATUS_ID:
      TRACE_MSG(TRACE_APP1, "Device Status 0x%x", (FMT__D, g_dev_ctx.dev.lst[dev_idx].u.metering.status));

      switch (g_dev_ctx.dev.lst[dev_idx].u.metering.device_type)
      {
        case ZB_ZCL_METERING_ELECTRIC_METERING:
          if (g_dev_ctx.dev.lst[dev_idx].u.metering.status & ZB_ZCL_METERING_ELECTRICITY_CHECK_METER)
          {
            TRACE_MSG(TRACE_APP1, "   - check meter", (FMT__0));
          }
          if (g_dev_ctx.dev.lst[dev_idx].u.metering.status & ZB_ZCL_METERING_ELECTRICITY_LOW_BATTERY)
          {
            TRACE_MSG(TRACE_APP1, "   - low battery", (FMT__0));
          }
          if (g_dev_ctx.dev.lst[dev_idx].u.metering.status & ZB_ZCL_METERING_ELECTRICITY_TAMPER_DETECT)
          {
            TRACE_MSG(TRACE_APP1, "   - tamper detect", (FMT__0));
          }
          if (g_dev_ctx.dev.lst[dev_idx].u.metering.status & ZB_ZCL_METERING_ELECTRICITY_POWER_FAILURE)
          {
            TRACE_MSG(TRACE_APP1, "   - power failure", (FMT__0));
          }
          if (g_dev_ctx.dev.lst[dev_idx].u.metering.status & ZB_ZCL_METERING_ELECTRICITY_POWER_QUALITY)
          {
            TRACE_MSG(TRACE_APP1, "   - power quality", (FMT__0));
          }
          if (g_dev_ctx.dev.lst[dev_idx].u.metering.status & ZB_ZCL_METERING_ELECTRICITY_LEAK_DETECT)
          {
            TRACE_MSG(TRACE_APP1, "   - leak detect", (FMT__0));
          }
          if (g_dev_ctx.dev.lst[dev_idx].u.metering.status & ZB_ZCL_METERING_ELECTRICITY_SERVICE_DISCONNECT_OPEN)
          {
            TRACE_MSG(TRACE_APP1, "   - service disconnect open", (FMT__0));
          }
          break;
        case ZB_ZCL_METERING_GAS_METERING:
          if (g_dev_ctx.dev.lst[dev_idx].u.metering.status & ZB_ZCL_METERING_GAS_CHECK_METER)
          {
            TRACE_MSG(TRACE_APP1, "   - check meter", (FMT__0));
          }
          if (g_dev_ctx.dev.lst[dev_idx].u.metering.status & ZB_ZCL_METERING_GAS_LOW_BATTERY)
          {
            TRACE_MSG(TRACE_APP1, "   - low battery", (FMT__0));
          }
          if (g_dev_ctx.dev.lst[dev_idx].u.metering.status & ZB_ZCL_METERING_GAS_TAMPER_DETECT)
          {
            TRACE_MSG(TRACE_APP1, "   - tamper detect", (FMT__0));
          }
          if (g_dev_ctx.dev.lst[dev_idx].u.metering.status & ZB_ZCL_METERING_GAS_LOW_PRESSURE)
          {
            TRACE_MSG(TRACE_APP1, "   - low pressure", (FMT__0));
          }
          if (g_dev_ctx.dev.lst[dev_idx].u.metering.status & ZB_ZCL_METERING_GAS_LEAK_DETECT)
          {
            TRACE_MSG(TRACE_APP1, "   - leak detect", (FMT__0));
          }
          if (g_dev_ctx.dev.lst[dev_idx].u.metering.status & ZB_ZCL_METERING_GAS_SERVICE_DISCONNECT)
          {
            TRACE_MSG(TRACE_APP1, "   - service disconnect", (FMT__0));
          }
          if (g_dev_ctx.dev.lst[dev_idx].u.metering.status & ZB_ZCL_METERING_GAS_REVERSE_FLOW)
          {
            TRACE_MSG(TRACE_APP1, "   - reverse flow", (FMT__0));
          }
          break;

        default:
          TRACE_MSG(TRACE_APP1, " !! detailed report is not supported for this device type", (FMT__0));
      }

      break;

    default:
      break;
  }
}


/** Save remote device's received data into NVRAM. */
static void ihd_dev_save_data(zb_uint8_t param)
{
  ZVUNUSED(param);
  /* If we fail, trace is given and assertion is triggered */
  (void)zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);
  ZB_SCHEDULE_APP_ALARM(ihd_dev_save_data, 0, IHD_APP_DATA_PERSISTING_TIMEOUT);
}

/** Read remote device's stored data from NVRAM. */
void ihd_dev_read_nvram_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length)
{
  zb_ret_t ret;

  TRACE_MSG(TRACE_APP1, ">> ihd_dev_read_nvram_app_data: page %hd pos %d", (FMT__H_D, page, pos));

  ZB_ASSERT(payload_length == sizeof(ihd_dev_nvram_data_t));

  /* If we fail, trace is given and assertion is triggered */
  ret = zb_nvram_read_data(page, pos, (zb_uint8_t *)&g_dev_ctx.dev, sizeof(ihd_dev_nvram_data_t));

  TRACE_MSG(TRACE_APP1, "<< ihd_dev_read_nvram_app_data: ret %d", (FMT__D, ret));
}

/** Application callback called on NVRAM writing operation (application dataset). */
zb_ret_t ihd_dev_write_nvram_app_data(zb_uint8_t page, zb_uint32_t pos)
{
  zb_ret_t ret;

  TRACE_MSG(TRACE_APP1, ">> ihd_dev_write_nvram_app_data: page %hd, pos %d", (FMT__H_D, page, pos));

  /* If we fail, trace is given and assertion is triggered */
  ret = zb_nvram_write_data(page, pos, (zb_uint8_t *)&g_dev_ctx.dev, sizeof(ihd_dev_nvram_data_t));

  TRACE_MSG(TRACE_APP1, "<< ihd_dev_write_nvram_app_data: ret %d", (FMT__D, ret));

  return ret;
}

/** Get NVRAM application dataset size. */
zb_uint16_t ihd_dev_get_nvram_data_size()
{
  TRACE_MSG(TRACE_APP1, "ihd_dev_get_nvram_data_size: ret %hd", (FMT__H, sizeof(ihd_dev_nvram_data_t)));
  return sizeof(ihd_dev_nvram_data_t);
}

/** Init device context. */
static void ihd_dev_ctx_init()
{
  TRACE_MSG(TRACE_APP1, ">> ihd_dev_ctx_init", (FMT__0));

  ZB_MEMSET(&g_dev_ctx, 0, sizeof(g_dev_ctx));

  TRACE_MSG(TRACE_APP1, "<< ihd_dev_ctx_init", (FMT__0));
}

/** Init device ZCL attributes. */
static void ihd_dev_clusters_attrs_init(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> ihd_dev_clusters_attrs_init", (FMT__0));
  ZVUNUSED(param);

  g_dev_ctx.basic_attrs.zcl_version = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
  g_dev_ctx.basic_attrs.power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

  TRACE_MSG(TRACE_APP1, "<< ihd_dev_clusters_attrs_init", (FMT__0));
}

/** Handle received parameters for Price - Get Current Price command.
    Now trace received values. */
static void ihd_show_price_param(const zb_zcl_price_publish_price_payload_t *p)
{
  TRACE_MSG(TRACE_APP1,"PublishPrice:", (FMT__0));
  TRACE_MSG(TRACE_APP1, "provider=%d, issuer_event=%d",
            (FMT__D_D, p->provider_id, p->issuer_event_id));
  TRACE_MSG(TRACE_APP1, "price=%ld", (FMT__L, p->price));
}


#if IHD_ENABLE_GET_CALENDAR
/** Send Get Calendar command (ZCL Calendar cluster). */
/** [zb_zcl_calendar_send_cmd_get_calendar]  */
static void ihd_dev_cmd_get_calendar(zb_uint8_t param, zb_uint16_t user_param)
{
  TRACE_MSG(TRACE_APP1, ">> ihd_dev_cmd_get_calendar, param=%hd",(FMT__H, param));

  if (!param)
  {
    zb_buf_get_out_delayed_ext(ihd_dev_cmd_get_calendar, user_param, 0);
  }
  else
  {
    zb_zcl_calendar_get_calendar_payload_t pl = {
      .provider_id = IHD_PROVIDER_ID,
    };
    zb_zcl_calendar_send_cmd_get_calendar(param,
                                     (zb_addr_u *)&g_dev_ctx.dev.lst[user_param].dev_addr,
                                     ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                                     g_dev_ctx.dev.lst[user_param].dev_ep,
                                     IHD_DEV_ENDPOINT,
                                     &pl,
                                     NULL);
  }

  TRACE_MSG(TRACE_APP1, "<< ihd_dev_cmd_get_calendar", (FMT__0));
}
/** [zb_zcl_calendar_send_cmd_get_calendar]  */

/** [zb_zcl_calendar_send_cmd_get_day_profiles] */
/** Send Get Day Profiles command. */
static void ihd_dev_cmd_get_day_profiles(zb_uint8_t param, zb_uint16_t user_param)
{
  TRACE_MSG(TRACE_APP1, ">> ihd_dev_cmd_get_day_profiles, param=%hd",(FMT__H, param));

  if (!param)
  {
    zb_buf_get_out_delayed_ext(ihd_dev_cmd_get_day_profiles, user_param, 0);
  }
  else
  {
    zb_zcl_calendar_get_day_profiles_payload_t pl = {
      .provider_id = 0xABCD5678,
      .issuer_calendar_id = 0x12345678,
      .start_day_id = 1,
      .number_of_days = 2
    };
    zb_zcl_calendar_send_cmd_get_day_profiles(param,
                                     (zb_addr_u *)&g_dev_ctx.dev.lst[user_param].dev_addr,
                                     ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                                     g_dev_ctx.dev.lst[user_param].dev_ep,
                                     IHD_DEV_ENDPOINT,
                                     &pl,
                                     NULL);

  }

  TRACE_MSG(TRACE_APP1, "<< ihd_dev_cmd_get_day_profiles", (FMT__0));
}
/** [zb_zcl_calendar_send_cmd_get_day_profiles] */

/** [zb_zcl_calendar_send_cmd_get_week_profiles]  */
/** Send Get Week Profiles command. */
static void ihd_dev_cmd_get_week_profiles(zb_uint8_t param, zb_uint16_t user_param)
{
  TRACE_MSG(TRACE_APP1, ">> ihd_dev_cmd_get_week_profiles, param=%hd",(FMT__H, param));

  if (!param)
  {
    zb_buf_get_out_delayed_ext(ihd_dev_cmd_get_week_profiles, user_param, 0);
  }
  else
  {
    zb_zcl_calendar_get_week_profiles_payload_t pl = {
      .provider_id = 0xABCD5678,
      .issuer_calendar_id = 0x12345678,
      .start_week_id = 1,
      .number_of_weeks = 5
    };
    zb_zcl_calendar_send_cmd_get_week_profiles(param,
                                     (zb_addr_u *)&g_dev_ctx.dev.lst[user_param].dev_addr,
                                     ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                                     g_dev_ctx.dev.lst[user_param].dev_ep,
                                     IHD_DEV_ENDPOINT,
                                     &pl,
                                     NULL);

  }

  TRACE_MSG(TRACE_APP1, "<< ihd_dev_cmd_get_week_profiles", (FMT__0));
}
/** [zb_zcl_calendar_send_cmd_get_week_profiles]  */

/** [zb_zcl_calendar_send_cmd_get_seasons]  */
/** Send Get Seasons command. */
static void ihd_dev_cmd_get_seasons(zb_uint8_t param, zb_uint16_t user_param)
{
  TRACE_MSG(TRACE_APP1, ">> ihd_dev_cmd_get_seasons, param=%hd",(FMT__H, param));

  if (!param)
  {
    zb_buf_get_out_delayed_ext(ihd_dev_cmd_get_seasons, user_param, 0);
  }
  else
  {
    zb_zcl_calendar_get_seasons_payload_t pl = {
      .provider_id = 0xABCD5678,
      .issuer_calendar_id = 0x12345678
    };
    zb_zcl_calendar_send_cmd_get_seasons(param,
                                     (zb_addr_u *)&g_dev_ctx.dev.lst[user_param].dev_addr,
                                     ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                                     g_dev_ctx.dev.lst[user_param].dev_ep,
                                     IHD_DEV_ENDPOINT,
                                     &pl,
                                     NULL);

  }

  TRACE_MSG(TRACE_APP1, "<< ihd_dev_cmd_get_seasons", (FMT__0));
}
/** [zb_zcl_calendar_send_cmd_get_seasons]  */

/** [zb_zcl_calendar_send_cmd_get_special_days] */
/** Send Get Special Days command. */
static void ihd_dev_cmd_get_special_days(zb_uint8_t param, zb_uint16_t user_param)
{
  TRACE_MSG(TRACE_APP1, ">> ihd_dev_cmd_get_special_days, param=%hd",(FMT__H, param));

  if (!param)
  {
    zb_buf_get_out_delayed_ext(ihd_dev_cmd_get_special_days, user_param, 0);
  }
  else
  {
    zb_zcl_calendar_get_special_days_payload_t pl = {
      .start_time = zb_get_utc_time(),
      .number_of_events = 1,
      .calendar_type = ZB_ZCL_CALENDAR_TYPE_FRIENDLY_CREDIT_CALENDAR,
      .provider_id = 0xABCD5678,
      .issuer_calendar_id = 0x12345678
    };
    zb_zcl_calendar_send_cmd_get_special_days(param,
                                     (zb_addr_u *)&g_dev_ctx.dev.lst[user_param].dev_addr,
                                     ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                                     g_dev_ctx.dev.lst[user_param].dev_ep,
                                     IHD_DEV_ENDPOINT,
                                     &pl,
                                     NULL);

  }

  TRACE_MSG(TRACE_APP1, "<< ihd_dev_cmd_get_special_days", (FMT__0));
}
/** [zb_zcl_calendar_send_cmd_get_special_days] */

/** [zb_zcl_calendar_send_cmd_get_calendar_cancellation]  */
/** Send Get Calendar Cancellation command. */
static void ihd_dev_cmd_get_calendar_cancellation(zb_uint8_t param, zb_uint16_t user_param)
{
  TRACE_MSG(TRACE_APP1, ">> ihd_dev_cmd_get_calendar_cancellation, param=%hd",(FMT__H, param));

  if (!param)
  {
    zb_buf_get_out_delayed_ext(ihd_dev_cmd_get_calendar_cancellation, user_param, 0);
  }
  else
  {

    zb_zcl_calendar_send_cmd_get_calendar_cancellation(param,
                                     (zb_addr_u *)&g_dev_ctx.dev.lst[user_param].dev_addr,
                                     ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                                     g_dev_ctx.dev.lst[user_param].dev_ep,
                                     IHD_DEV_ENDPOINT,
                                     NULL);

  }

  TRACE_MSG(TRACE_APP1, "<< ihd_dev_cmd_get_calendar_cancellation", (FMT__0));
}
/** [zb_zcl_calendar_send_cmd_get_calendar_cancellation]  */

/** Handle Publish Calendar command (ZCL Calendar cluster). */
static void ihd_dev_handle_publish_calendar(const zb_zcl_calendar_publish_calendar_payload_t *in,
                                                 const zb_zcl_parsed_hdr_t *in_cmd_info)
{
  zb_uint8_t dev_idx = ihd_dev_get_idx_by_cmd_info(in_cmd_info);

  TRACE_MSG(TRACE_APP1, ">> handle_publish_calendar(in=%p)", (FMT__P, in));

  TRACE_MSG(TRACE_APP1, "in->provider_id = %lx", (FMT__D, in->provider_id));
  TRACE_MSG(TRACE_APP1, "in->issuer_event_id = %lx", (FMT__D, in->issuer_event_id));
  TRACE_MSG(TRACE_APP1, "in->issuer_calendar_id = %lx", (FMT__D, in->issuer_calendar_id));
  TRACE_MSG(TRACE_APP1, "in->start_time = %d", (FMT__D, in->start_time));
  TRACE_MSG(TRACE_APP1, "in->calendar_type = %hx", (FMT__H, in->calendar_type));
  TRACE_MSG(TRACE_APP1, "in->calendar_time_reference = %hx", (FMT__H, in->calendar_time_reference));
  TRACE_MSG(TRACE_APP1, "in->calendar_name = %p", (FMT__P, in->calendar_name));
  TRACE_MSG(TRACE_APP1, "in->number_of_seasons = %hx", (FMT__H, in->number_of_seasons));
  TRACE_MSG(TRACE_APP1, "in->number_of_week_profiles = %hx", (FMT__H, in->number_of_week_profiles));
  TRACE_MSG(TRACE_APP1, "in->number_of_day_profiles = %hx", (FMT__H, in->number_of_day_profiles));

  TRACE_MSG(TRACE_APP1, "<< handle_publish_calendar", (FMT__0));

  g_dev_ctx.dev.lst[dev_idx].pending_cmd = IHD_DEV_CALENDAR_GET_DAY_PROFILES;
  ZB_SCHEDULE_APP_ALARM(ihd_dev_send_pending_cmd, dev_idx, IHD_CALENDAR_CMD_PERIOD);

}

/** Handle Publish Week Profile command (ZCL Calendar cluster). */
static void ihd_dev_handle_publish_week_profile(const zb_zcl_calendar_publish_week_profile_payload_t *in,
                                                     const zb_zcl_parsed_hdr_t *in_cmd_info)
{
  zb_uint8_t dev_idx = ihd_dev_get_idx_by_cmd_info(in_cmd_info);

  TRACE_MSG(TRACE_APP1, ">> handle_publish_week_profile(in=%p)", (FMT__P, in));

  TRACE_MSG(TRACE_APP1, "in->provider_id = %lx", (FMT__D, in->provider_id));
  TRACE_MSG(TRACE_APP1, "in->issuer_event_id = %lx", (FMT__D, in->issuer_event_id));
  TRACE_MSG(TRACE_APP1, "in->issuer_calendar_id = %lx", (FMT__D, in->issuer_calendar_id));
  TRACE_MSG(TRACE_APP1, "in->week_id = %hx", (FMT__H, in->week_id));
  TRACE_MSG(TRACE_APP1, "in->day_id_ref_monday = %hx", (FMT__H, in->day_id_ref_monday));
  TRACE_MSG(TRACE_APP1, "in->day_id_ref_tuesday = %hx", (FMT__H, in->day_id_ref_tuesday));
  TRACE_MSG(TRACE_APP1, "in->day_id_ref_wednesday = %hx", (FMT__H, in->day_id_ref_wednesday));
  TRACE_MSG(TRACE_APP1, "in->day_id_ref_thursday = %hx", (FMT__H, in->day_id_ref_thursday));
  TRACE_MSG(TRACE_APP1, "in->day_id_ref_friday = %hx", (FMT__H, in->day_id_ref_friday));
  TRACE_MSG(TRACE_APP1, "in->day_id_ref_saturday = %hx", (FMT__H, in->day_id_ref_saturday));
  TRACE_MSG(TRACE_APP1, "in->day_id_ref_sunday = %hx", (FMT__H, in->day_id_ref_sunday));

  TRACE_MSG(TRACE_APP1, "<< handle_publish_week_profile", (FMT__0));

  g_dev_ctx.dev.lst[dev_idx].pending_cmd = IHD_DEV_CALENDAR_GET_SEASONS;
  ZB_SCHEDULE_APP_ALARM(ihd_dev_send_pending_cmd, dev_idx, IHD_CALENDAR_CMD_PERIOD);
}

static void ihd_dev_handle_publish_day_profile(const zb_zcl_calendar_publish_day_profile_payload_t *in,
  const zb_zcl_parsed_hdr_t *in_cmd_info)
{
  zb_uint8_t dev_idx = ihd_dev_get_idx_by_cmd_info(in_cmd_info);

TRACE_MSG(TRACE_APP1, ">> handle_publish_day_profile(in=%p)", (FMT__P, in));

TRACE_MSG(TRACE_APP1, "in->provider_id = %lx", (FMT__D, in->provider_id));
TRACE_MSG(TRACE_APP1, "in->issuer_event_id = %lx", (FMT__D, in->issuer_event_id));
TRACE_MSG(TRACE_APP1, "in->issuer_calendar_id = %lx", (FMT__D, in->issuer_calendar_id));
TRACE_MSG(TRACE_APP1, "in->day_id = %hx", (FMT__H, in->day_id));
TRACE_MSG(TRACE_APP1, "in->total_number_of_schedule_entries = %hx", (FMT__H, in->total_number_of_schedule_entries));
TRACE_MSG(TRACE_APP1, "in->command_index = %hx", (FMT__H, in->command_index));
TRACE_MSG(TRACE_APP1, "in->total_number_of_commands = %hx", (FMT__H, in->total_number_of_commands));
TRACE_MSG(TRACE_APP1, "in->calendar_type = %hx", (FMT__H, in->calendar_type));
TRACE_MSG(TRACE_APP1, "in->day_schedule_entries = %p", (FMT__P, in->day_schedule_entries));
TRACE_MSG(TRACE_APP1, "in->number_of_entries_in_this_command = %hx", (FMT__H, in->number_of_entries_in_this_command));

TRACE_MSG(TRACE_APP1, "<< handle_publish_day_profile", (FMT__0));

  g_dev_ctx.dev.lst[dev_idx].pending_cmd = IHD_DEV_CALENDAR_GET_WEEK_PROFILES;
  ZB_SCHEDULE_APP_ALARM(ihd_dev_send_pending_cmd, dev_idx, IHD_CALENDAR_CMD_PERIOD);
}

static void ihd_dev_handle_publish_special_days(const zb_zcl_calendar_publish_special_days_payload_t *in,
  const zb_zcl_parsed_hdr_t *in_cmd_info)
{
  zb_uint8_t dev_idx = ihd_dev_get_idx_by_cmd_info(in_cmd_info);
  TRACE_MSG(TRACE_APP1, ">> ihd_dev_handle_publish_special_days(in=%p)", (FMT__P, in));

  TRACE_MSG(TRACE_APP1, "in->provider_id = %lx", (FMT__D, in->provider_id));
  TRACE_MSG(TRACE_APP1, "in->issuer_event_id = %lx", (FMT__D, in->issuer_event_id));
  TRACE_MSG(TRACE_APP1, "in->issuer_calendar_id = %lx", (FMT__D, in->issuer_calendar_id));
  TRACE_MSG(TRACE_APP1, "in->start_time = %lx", (FMT__D, in->start_time));
  TRACE_MSG(TRACE_APP1, "in->calendar_type = %hx", (FMT__H, in->calendar_type));
  TRACE_MSG(TRACE_APP1, "in->total_number_of_special_days %hx", (FMT__H, in->total_number_of_special_days));
  TRACE_MSG(TRACE_APP1, "in->command_index = %hx", (FMT__H, in->command_index));
  TRACE_MSG(TRACE_APP1, "in->total_number_of_commands = %hx", (FMT__H, in->total_number_of_commands));
  TRACE_MSG(TRACE_APP1, "in->special_day_entry = %p", (FMT__P, in->special_day_entry));
  TRACE_MSG(TRACE_APP1, "in->number_of_entries_in_this_command = %hx", (FMT__H, in->number_of_entries_in_this_command));

  TRACE_MSG(TRACE_APP1, "<< ihd_dev_handle_publish_special_days", (FMT__0));

  g_dev_ctx.dev.lst[dev_idx].pending_cmd = IHD_DEV_CALENDAR_GET_CALENDAR_CANCELLATION;
  ZB_SCHEDULE_APP_ALARM(ihd_dev_send_pending_cmd, dev_idx, IHD_CALENDAR_CMD_PERIOD);
}

static void ihd_dev_handle_publish_seasons(const zb_zcl_calendar_publish_seasons_payload_t *in,
  const zb_zcl_parsed_hdr_t *in_cmd_info)
{
  zb_uint8_t dev_idx = ihd_dev_get_idx_by_cmd_info(in_cmd_info);
  TRACE_MSG(TRACE_APP1, ">> ihd_dev_handle_publish_seasons(in=%p)", (FMT__P, in));

  TRACE_MSG(TRACE_APP1, "in->provider_id = %lx", (FMT__D, in->provider_id));
  TRACE_MSG(TRACE_APP1, "in->issuer_event_id = %lx", (FMT__D, in->issuer_event_id));
  TRACE_MSG(TRACE_APP1, "in->issuer_calendar_id = %lx", (FMT__D, in->issuer_calendar_id));
  TRACE_MSG(TRACE_APP1, "in->command_index = %hx", (FMT__H, in->command_index));
  TRACE_MSG(TRACE_APP1, "in->total_number_of_commands = %hx", (FMT__H, in->total_number_of_commands));
  TRACE_MSG(TRACE_APP1, "in->season_entry = %p", (FMT__P, in->season_entry));
  TRACE_MSG(TRACE_APP1, "in->number_of_entries_in_this_command = %hx", (FMT__H, in->number_of_entries_in_this_command));

  TRACE_MSG(TRACE_APP1, "<< ihd_dev_handle_publish_seasons", (FMT__0));

  g_dev_ctx.dev.lst[dev_idx].pending_cmd = IHD_DEV_CALENDAR_GET_SPECIAL_DAYS;
  ZB_SCHEDULE_APP_ALARM(ihd_dev_send_pending_cmd, dev_idx, IHD_CALENDAR_CMD_PERIOD);
}

/** Handle Cancel Calendar command (ZCL Calendar cluster). */
static void ihd_dev_handle_cancel_calendar(const zb_zcl_calendar_cancel_calendar_payload_t *in,
                                                const zb_zcl_parsed_hdr_t *in_cmd_info)
{
  zb_uint8_t dev_idx = ihd_dev_get_idx_by_cmd_info(in_cmd_info);
  ZVUNUSED(in_cmd_info);

  TRACE_MSG(TRACE_APP1, ">> handle_report_cancel_calendar(in=%p)", (FMT__P, in));

  TRACE_MSG(TRACE_APP1, "in->provider_id = %lx", (FMT__D, in->provider_id));
  TRACE_MSG(TRACE_APP1, "in->issuer_calendar_id = %lx", (FMT__D, in->issuer_calendar_id));
  TRACE_MSG(TRACE_APP1, "in->calendar_type = %hx", (FMT__H, in->calendar_type));

  TRACE_MSG(TRACE_APP1, ">> handle_report_cancel_calendar(in=%p)", (FMT__P, in));

  g_dev_ctx.dev.lst[dev_idx].pending_cmd = IHD_DEV_CALENDAR_GET_CALENDAR;
  ZB_SCHEDULE_APP_ALARM(ihd_dev_send_pending_cmd, dev_idx, IHD_CALENDAR_UPDATE_PERIOD);
}
#endif  /* IHD_ENABLE_GET_CALENDAR */


void ihd_handle_display_msg(const zb_zcl_messaging_display_message_payload_t *in)
{
  TRACE_MSG(TRACE_APP1, ">> ihd_handle_display_msg(in=%p)", (FMT__P, in));
  TRACE_MSG(TRACE_APP1, "in->message_id = %lx", (FMT__L, in->message_id));
  TRACE_MSG(TRACE_APP1, "in->message_control = %hx", (FMT__H, in->message_control));
  TRACE_MSG(TRACE_APP1, "in->start_time = %lx", (FMT__L, in->start_time));
  TRACE_MSG(TRACE_APP1, "in->duration_in_minutes = %x", (FMT__D, in->duration_in_minutes));
  TRACE_MSG(TRACE_APP1, "in->extended_message_control = %hx", (FMT__H, in->extended_message_control));

  TRACE_MSG(TRACE_APP1, "Got message: len = %hd", (FMT__H, in->message_len));
  dump_traf_str(in->message, in->message_len);

  TRACE_MSG(TRACE_ZCL1, "message ptr:%p", (FMT__P, in->message));
}


zb_uint8_t prev_get_snapshot_offset = 0x00;

void ihd_dev_handle_publish_snapshot(zb_uint8_t param)
{
  const zb_zcl_metering_publish_snapshot_payload_t *pl_in = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_metering_publish_snapshot_payload_t);
  zb_uint16_t dev_idx;

  TRACE_MSG(TRACE_APP1, ">> ihd_dev_handle_publish_snapshot(in=%p)", (FMT__P, pl_in));

  TRACE_MSG(TRACE_APP1, "pl_in->snapshot_id = %lx", (FMT__L, pl_in->snapshot_id));
  TRACE_MSG(TRACE_APP1, "pl_in->snapshot_time = %lx", (FMT__L, pl_in->snapshot_time));
  TRACE_MSG(TRACE_APP1, "pl_in->total_snapshots_found = %d", (FMT__D, pl_in->total_snapshots_found));
  TRACE_MSG(TRACE_APP1, "pl_in->command_index = %d", (FMT__D, pl_in->command_index));
  TRACE_MSG(TRACE_APP1, "pl_in->total_number_of_commands = %d", (FMT__D, pl_in->total_number_of_commands));
  TRACE_MSG(TRACE_APP1, "pl_in->snapshot_cause = %lx", (FMT__L, pl_in->snapshot_cause));
  TRACE_MSG(TRACE_APP1, "pl_in->snapshot_payload_type = %hx", (FMT__H, pl_in->snapshot_payload_type));

  switch (pl_in->snapshot_payload_type)
  {
    case ZB_ZCL_METERING_TOU_DELIVERED_REGISTERS:
    {
      const zb_zcl_metering_tou_delivered_payload_t *sub_pl = &(pl_in->snapshot_sub_payload.tou_delivered);

      TRACE_MSG(TRACE_ZCL1, "Snapshot payload type is ZB_ZCL_METERING_TOU_DELIVERED_REGISTERS", (FMT__0));

      TRACE_MSG(TRACE_APP1, "sub_pl->current_summation_delivered = %lx", (FMT__L, zb_uint48_to_int64(&sub_pl->current_summation_delivered)));
      TRACE_MSG(TRACE_APP1, "sub_pl->bill_to_date_delivered = %lx", (FMT__L, sub_pl->bill_to_date_delivered));
      TRACE_MSG(TRACE_APP1, "sub_pl->bill_to_date_time_stamp_delivered = %lx", (FMT__L, sub_pl->bill_to_date_time_stamp_delivered));
      TRACE_MSG(TRACE_APP1, "sub_pl->projected_bill_delivered = %lx", (FMT__L, sub_pl->projected_bill_delivered));
      TRACE_MSG(TRACE_APP1, "sub_pl->projected_bill_time_stamp_delivered = %lx", (FMT__L, sub_pl->projected_bill_time_stamp_delivered));
      TRACE_MSG(TRACE_APP1, "sub_pl->bill_delivered_trailing_digit = %d", (FMT__D, sub_pl->bill_delivered_trailing_digit));
      TRACE_MSG(TRACE_APP1, "sub_pl->number_of_tiers_in_use = %d", (FMT__D, sub_pl->number_of_tiers_in_use));

      for (zb_uint_t i = 0; i < sub_pl->number_of_tiers_in_use; i++)
      {
        TRACE_MSG(TRACE_APP1, "sub_pl->tier_summation[%d] = %lx", (FMT__D_L, i, zb_uint48_to_int64(&sub_pl->tier_summation[i])));
      }
    }
    break;
    case ZB_ZCL_METERING_TOU_RECEIVED_REGISTERS:
    {
      const zb_zcl_metering_tou_received_payload_t *sub_pl = &(pl_in->snapshot_sub_payload.tou_received);

      TRACE_MSG(TRACE_ZCL1, "Snapshot payload type is ZB_ZCL_METERING_TOU_RECEIVED_REGISTERS", (FMT__0));

      TRACE_MSG(TRACE_APP1, "sub_pl->current_summation_received = %lx", (FMT__L, zb_uint48_to_int64(&sub_pl->current_summation_received)));
      TRACE_MSG(TRACE_APP1, "sub_pl->bill_to_date_received = %lx", (FMT__L, sub_pl->bill_to_date_received));
      TRACE_MSG(TRACE_APP1, "sub_pl->bill_to_date_time_stamp_received = %lx", (FMT__L, sub_pl->bill_to_date_time_stamp_received));
      TRACE_MSG(TRACE_APP1, "sub_pl->projected_bill_received = %lx", (FMT__L, sub_pl->projected_bill_received));
      TRACE_MSG(TRACE_APP1, "sub_pl->projected_bill_time_stamp_received = %lx", (FMT__L, sub_pl->projected_bill_time_stamp_received));
      TRACE_MSG(TRACE_APP1, "sub_pl->bill_received_trailing_digit = %d", (FMT__D, sub_pl->bill_received_trailing_digit));
      TRACE_MSG(TRACE_APP1, "sub_pl->number_of_tiers_in_use = %d", (FMT__D, sub_pl->number_of_tiers_in_use));

      for (zb_uint_t i = 0; i < sub_pl->number_of_tiers_in_use; i++)
      {
        TRACE_MSG(TRACE_APP1, "sub_pl->tier_summation[%d] = %lx", (FMT__D_L, i, zb_uint48_to_int64(&sub_pl->tier_summation[i])));
      }
    }
    break;
    case ZB_ZCL_METERING_BLOCK_TIER_DELIVERED:
    {
      const zb_zcl_metering_block_delivered_payload_t *sub_pl = &(pl_in->snapshot_sub_payload.block_delivered);

      TRACE_MSG(TRACE_ZCL1, "Snapshot payload type is ZB_ZCL_METERING_BLOCK_TIER_DELIVERED", (FMT__0));

      TRACE_MSG(TRACE_APP1, "sub_pl->current_summation_delivered = %lx", (FMT__L, zb_uint48_to_int64(&sub_pl->current_summation_delivered)));
      TRACE_MSG(TRACE_APP1, "sub_pl->bill_to_date_delivered = %lx", (FMT__L, sub_pl->bill_to_date_delivered));
      TRACE_MSG(TRACE_APP1, "sub_pl->bill_to_date_time_stamp_delivered = %lx", (FMT__L, sub_pl->bill_to_date_time_stamp_delivered));
      TRACE_MSG(TRACE_APP1, "sub_pl->projected_bill_delivered = %lx", (FMT__L, sub_pl->projected_bill_delivered));
      TRACE_MSG(TRACE_APP1, "sub_pl->projected_bill_time_stamp_delivered = %lx", (FMT__L, sub_pl->projected_bill_time_stamp_delivered));
      TRACE_MSG(TRACE_APP1, "sub_pl->bill_delivered_trailing_digit = %d", (FMT__D, sub_pl->bill_delivered_trailing_digit));
      TRACE_MSG(TRACE_APP1, "sub_pl->number_of_tiers_in_use = %d", (FMT__D, sub_pl->number_of_tiers_in_use));

      for (zb_uint_t i = 0; i < sub_pl->number_of_tiers_in_use; i++)
      {
        TRACE_MSG(TRACE_APP1, "sub_pl->tier_summation[%d] = %lx", (FMT__D_L, i, zb_uint48_to_int64(&sub_pl->tier_summation[i])));
      }

      TRACE_MSG(TRACE_APP1, "sub_pl->number_of_tiers_and_block_thresholds_in_use = %d",
                (FMT__D, sub_pl->number_of_tiers_and_block_thresholds_in_use));


      for (zb_uint_t i = 0; i < sub_pl->number_of_tiers_and_block_thresholds_in_use; i++)
      {
        TRACE_MSG(TRACE_APP1, "sub_pl->tier_block_summation[%d] = %lx", (FMT__D_L, i, zb_uint48_to_int64(&sub_pl->tier_block_summation[i])));
      }
    }
    break;
    case ZB_ZCL_METERING_BLOCK_TIER_RECEIVED:
    {
      const zb_zcl_metering_block_received_payload_t *sub_pl = &(pl_in->snapshot_sub_payload.block_received);

      TRACE_MSG(TRACE_ZCL1, "Snapshot payload type is ZB_ZCL_METERING_BLOCK_TIER_RECEIVED", (FMT__0));

      TRACE_MSG(TRACE_APP1, "sub_pl->current_summation_received = %lx", (FMT__L, zb_uint48_to_int64(&sub_pl->current_summation_received)));
      TRACE_MSG(TRACE_APP1, "sub_pl->bill_to_date_received = %lx", (FMT__L, sub_pl->bill_to_date_received));
      TRACE_MSG(TRACE_APP1, "sub_pl->bill_to_date_time_stamp_received = %lx", (FMT__L, sub_pl->bill_to_date_time_stamp_received));
      TRACE_MSG(TRACE_APP1, "sub_pl->projected_bill_received = %lx", (FMT__L, sub_pl->projected_bill_received));
      TRACE_MSG(TRACE_APP1, "sub_pl->projected_bill_time_stamp_received = %lx", (FMT__L, sub_pl->projected_bill_time_stamp_received));
      TRACE_MSG(TRACE_APP1, "sub_pl->bill_received_trailing_digit = %d", (FMT__D, sub_pl->bill_received_trailing_digit));
      TRACE_MSG(TRACE_APP1, "sub_pl->number_of_tiers_in_use = %d", (FMT__D, sub_pl->number_of_tiers_in_use));

      for (zb_uint_t i = 0; i < sub_pl->number_of_tiers_in_use; i++)
      {
        TRACE_MSG(TRACE_APP1, "sub_pl->tier_summation[%d] = %lx", (FMT__D_L, i, zb_uint48_to_int64(&sub_pl->tier_summation[i])));
      }

      TRACE_MSG(TRACE_APP1, "sub_pl->number_of_tiers_and_block_thresholds_in_use = %d",
                (FMT__D, sub_pl->number_of_tiers_and_block_thresholds_in_use));


      for (zb_uint_t i = 0; i < sub_pl->number_of_tiers_and_block_thresholds_in_use; i++)
      {
        TRACE_MSG(TRACE_APP1, "sub_pl->tier_block_summation[%d] = %lx", (FMT__D_L, i, zb_uint48_to_int64(&sub_pl->tier_block_summation[i])));
      }
    }
    break;
    case ZB_ZCL_METERING_TOU_DELIVERED_NO_BILLING:
    {
      const zb_zcl_metering_tou_delivered_no_billing_payload_t *sub_pl = &(pl_in->snapshot_sub_payload.tou_delivered_no_billing);

      TRACE_MSG(TRACE_ZCL1, "Snapshot payload type is ZB_ZCL_METERING_TOU_DELIVERED_NO_BILLING", (FMT__0));

      TRACE_MSG(TRACE_APP1, "sub_pl->current_summation_delivered = %lx", (FMT__L, zb_uint48_to_int64(&sub_pl->current_summation_delivered)));
      TRACE_MSG(TRACE_APP1, "sub_pl->number_of_tiers_in_use = %d", (FMT__D, sub_pl->number_of_tiers_in_use));

      for (zb_uint_t i = 0; i < sub_pl->number_of_tiers_in_use; i++)
      {
        TRACE_MSG(TRACE_APP1, "sub_pl->tier_summation[%d] = %lx", (FMT__D_L, i, zb_uint48_to_int64(&sub_pl->tier_summation[i])));
      }
    }
    break;
    case ZB_ZCL_METERING_TOU_RECEIVED_NO_BILLING:
    {
      const zb_zcl_metering_tou_received_no_billing_payload_t *sub_pl = &(pl_in->snapshot_sub_payload.tou_received_no_billing);

      TRACE_MSG(TRACE_ZCL1, "Snapshot payload type is ZB_ZCL_METERING_TOU_RECEIVED_NO_BILLING", (FMT__0));

      TRACE_MSG(TRACE_APP1, "sub_pl->current_summation_received = %lx",
                (FMT__L, zb_uint48_to_int64(&sub_pl->current_summation_received)));
      TRACE_MSG(TRACE_APP1, "sub_pl->number_of_tiers_in_use = %d",
                (FMT__D, sub_pl->number_of_tiers_in_use));

      for (zb_uint_t i = 0; i < sub_pl->number_of_tiers_in_use; i++)
      {
        TRACE_MSG(TRACE_APP1, "sub_pl->tier_summation[%d] = %lx", (FMT__D_L, i, zb_uint48_to_int64(&sub_pl->tier_summation[i])));
      }
    }
    break;
    case ZB_ZCL_METERING_BLOCK_TIER_DELIVERED_NO_BILLING:
    {
      const zb_zcl_metering_block_tier_delivered_no_billing_payload_t *sub_pl = &(pl_in->snapshot_sub_payload.block_tier_delivered_no_billing);

      TRACE_MSG(TRACE_ZCL1, "Snapshot payload type is ZB_ZCL_METERING_BLOCK_TIER_DELIVERED_NO_BILLING", (FMT__0));

      TRACE_MSG(TRACE_APP1, "sub_pl->current_summation_delivered = %lx", (FMT__L, zb_uint48_to_int64(&sub_pl->current_summation_delivered)));
      TRACE_MSG(TRACE_APP1, "sub_pl->number_of_tiers_in_use = %d", (FMT__D, sub_pl->number_of_tiers_in_use));

      for (zb_uint_t i = 0; i < sub_pl->number_of_tiers_in_use; i++)
      {
        TRACE_MSG(TRACE_APP1, "sub_pl->tier_summation[%d] = %lx", (FMT__D_L, i, zb_uint48_to_int64(&sub_pl->tier_summation[i])));
      }

      TRACE_MSG(TRACE_APP1, "sub_pl->number_of_tiers_and_block_thresholds_in_use = %d",
                (FMT__D, sub_pl->number_of_tiers_and_block_thresholds_in_use));

      for (zb_uint_t i = 0; i < sub_pl->number_of_tiers_and_block_thresholds_in_use; i++)
      {
        TRACE_MSG(TRACE_APP1, "sub_pl->tier_block_summation[%d] = %lx", (FMT__D_L, i, zb_uint48_to_int64(&sub_pl->tier_block_summation[i])));
      }
    }
    break;
    case ZB_ZCL_METERING_BLOCK_TIER_RECEIVED_NO_BILLING:
    {
      const zb_zcl_metering_block_tier_received_no_billing_payload_t *sub_pl = &(pl_in->snapshot_sub_payload.block_tier_received_no_billing);

      TRACE_MSG(TRACE_ZCL1, "Snapshot payload type is ZB_ZCL_METERING_BLOCK_TIER_RECEIVED_NO_BILLING", (FMT__0));

      TRACE_MSG(TRACE_APP1, "sub_pl->current_summation_received = %lx", (FMT__L, zb_uint48_to_int64(&sub_pl->current_summation_received)));
      TRACE_MSG(TRACE_APP1, "sub_pl->number_of_tiers_in_use = %d", (FMT__D, sub_pl->number_of_tiers_in_use));

      for (zb_uint_t i = 0; i < sub_pl->number_of_tiers_in_use; i++)
      {
        TRACE_MSG(TRACE_APP1, "sub_pl->tier_summation[%d] = %lx", (FMT__D_L, i, zb_uint48_to_int64(&sub_pl->tier_summation[i])));
      }

      TRACE_MSG(TRACE_APP1, "sub_pl->number_of_tiers_and_block_thresholds_in_use = %d",
                (FMT__D, sub_pl->number_of_tiers_and_block_thresholds_in_use));

      for (zb_uint_t i = 0; i < sub_pl->number_of_tiers_and_block_thresholds_in_use; i++)
      {
        TRACE_MSG(TRACE_APP1, "sub_pl->tier_block_summation[%d] = %lx", (FMT__D_L, i, zb_uint48_to_int64(&sub_pl->tier_block_summation[i])));
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

  dev_idx = ihd_dev_get_idx_by_cmd_info(ZB_ZCL_DEVICE_CMD_PARAM_CMD_INFO(param));

  if (prev_get_snapshot_offset < pl_in->total_snapshots_found)
  {
    prev_get_snapshot_offset++;

    /* Need to get the next snapshot from server */
    g_dev_ctx.dev.lst[dev_idx].pending_cmd = IHD_DEV_METERING_GET_SNAPSHOT;
  }
  else
  {
    prev_get_snapshot_offset = 0;
    g_dev_ctx.dev.lst[dev_idx].pending_cmd = IHD_DEV_METERING_GET_PROFILE;
  }

  ZB_SCHEDULE_APP_ALARM(ihd_dev_send_pending_cmd, dev_idx, IHD_METERING_PERIOD);

  ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
}


void ihd_dev_handle_publish_tier_labels(zb_uint8_t param)
{
  const zb_zcl_price_publish_tier_labels_payload_t *pl_in = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_price_publish_tier_labels_payload_t);
  zb_zcl_price_publish_tier_labels_sub_payload_t *sub_pl;
  TRACE_MSG(TRACE_APP1, ">> ihd_dev_handle_publish_tier_labels(in=%p)", (FMT__P, pl_in));

  TRACE_MSG(TRACE_APP1, "in->provider_id = %lx", (FMT__L, pl_in->provider_id));
  TRACE_MSG(TRACE_APP1, "in->issuer_event_id = %lx", (FMT__L, pl_in->issuer_event_id));
  TRACE_MSG(TRACE_APP1, "in->issuer_tariff_id = %lx", (FMT__L, pl_in->issuer_tariff_id));
  TRACE_MSG(TRACE_APP1, "in->command_index = %d", (FMT__D, pl_in->command_index));
  TRACE_MSG(TRACE_APP1, "in->total_number_of_commands = %d", (FMT__D, pl_in->total_number_of_commands));
  TRACE_MSG(TRACE_APP1, "in->number_of_labels = %d", (FMT__D, pl_in->number_of_labels));

  sub_pl = pl_in->tier_labels;
  for (zb_uint_t i = 0; i < pl_in->number_of_labels; i++)
  {
    TRACE_MSG(TRACE_APP1, "in->tier_labels.tier_id[%d] = %d", (FMT__D_D, i, sub_pl->tier_id));

    TRACE_MSG(TRACE_APP1, "in->tier_labels.tier_label[%d] length = %d", (FMT__D_D, i, sub_pl->tier_label[0]));
    dump_traf_str(&sub_pl->tier_label[1], sub_pl->tier_label[0]);

    sub_pl = (zb_zcl_price_publish_tier_labels_sub_payload_t *)((zb_uint8_t *)sub_pl + sub_pl->tier_label[0] + 2);
  }

  ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
}


void ihd_dev_handle_get_profile_response(zb_uint8_t param)
{
  const zb_zcl_metering_get_profile_response_payload_t *pl_in = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_metering_get_profile_response_payload_t);

  TRACE_MSG(TRACE_APP1, ">> ihd_dev_handle_get_profile_response(in=%p)", (FMT__P, pl_in));

  TRACE_MSG(TRACE_APP1, "in->end_time = %lx", (FMT__L, pl_in->end_time));
  TRACE_MSG(TRACE_APP1, "in->status = %hx", (FMT__H, pl_in->status));
  TRACE_MSG(TRACE_APP1, "in->profile_interval_period = %hx", (FMT__H, pl_in->profile_interval_period));
  TRACE_MSG(TRACE_APP1, "in->number_of_periods_delivered = %x", (FMT__D, pl_in->number_of_periods_delivered));

  for (zb_uint_t i = 0; i < pl_in->number_of_periods_delivered; i++)
  {
    TRACE_MSG(TRACE_APP1, "in->intervals[%lx] = %lx", (FMT__L_L, i, zb_uint24_to_int32(&pl_in->intervals[i])));
  }

  ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
}


void ihd_dev_handle_request_fast_poll_mode_response(zb_uint8_t param)
{
  const zb_zcl_metering_request_fast_poll_mode_response_payload_t *pl_in = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_metering_request_fast_poll_mode_response_payload_t);

  TRACE_MSG(TRACE_APP1, ">> ihd_dev_handle_request_fast_poll_mode_response(in=%p)", (FMT__P, pl_in));

  TRACE_MSG(TRACE_APP1, "in->applied_update_period_in_seconds = %x", (FMT__H, pl_in->applied_update_period_in_seconds));
  TRACE_MSG(TRACE_APP1, "in->fast_poll_mode_end_time = %lx", (FMT__L, pl_in->fast_poll_mode_end_time));

  ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
}


void ihd_dev_handle_get_sampled_data_response(zb_uint8_t param)
{
  const zb_zcl_metering_get_sampled_data_response_payload_t *pl_in = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_metering_get_sampled_data_response_payload_t);

  TRACE_MSG(TRACE_APP1, ">> ihd_dev_handle_get_sampled_data_response(in=%p)", (FMT__P, pl_in));

  TRACE_MSG(TRACE_APP1, "in->sample_id = %lx", (FMT__L, pl_in->sample_id));
  TRACE_MSG(TRACE_APP1, "in->sample_start_time = %lx", (FMT__L, pl_in->sample_start_time));
  TRACE_MSG(TRACE_APP1, "in->sample_type = %hx", (FMT__H, pl_in->sample_type));
  TRACE_MSG(TRACE_APP1, "in->sample_request_interval = %x", (FMT__D, pl_in->sample_request_interval));
  TRACE_MSG(TRACE_APP1, "in->number_of_samples = %x", (FMT__D, pl_in->number_of_samples));

  for (zb_uint_t i = 0; i < pl_in->number_of_samples; i++)
  {
    TRACE_MSG(TRACE_APP1, "in->samples[%d] = %lx", (FMT__D_L, i, zb_uint24_to_int32(&pl_in->samples[i])));
  }

  ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
}


/** Application device callback for ZCL commands. Used for:
    - receiving application-specific values for commands (e.g. ZB_ZCL_MESSAGING_DISPLAY_MSG_CB_ID)
    - providing received ZCL commands data to application (e.g. ZB_ZCL_PRICE_PUBLISH_PRICE_CB_ID)
    Application may ignore callback id-s in which it is not interested.
 */
static void ihd_zcl_cmd_device_cb(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> ihd_zcl_cmd_device_cb param %hd id %d", (FMT__H_D,
    param, ZB_ZCL_DEVICE_CMD_PARAM_CB_ID(param)));

  switch (ZB_ZCL_DEVICE_CMD_PARAM_CB_ID(param))
  {
    case ZB_ZCL_PRICE_PUBLISH_PRICE_CB_ID:
    {
      TRACE_MSG(TRACE_APP1, "ZB_ZCL_PRICE_PUBLISH_PRICE_CB_ID", (FMT__0));
      ihd_show_price_param(ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_price_publish_price_payload_t));
      ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
    }
    break;
    case ZB_ZCL_PRICE_PUBLISH_TIER_LABELS_CB_ID:
    {
      TRACE_MSG(TRACE_APP1, "ZB_ZCL_PRICE_PUBLISH_PRICE_CB_ID", (FMT__0));
      ihd_dev_handle_publish_tier_labels(param);
    }
    break;
    case ZB_ZCL_MESSAGING_DISPLAY_MSG_CB_ID:
    {
      TRACE_MSG(TRACE_APP1, "ZB_ZCL_MESSAGING_DISPLAY_MSG_CB_ID", (FMT__0));
      ihd_handle_display_msg(ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_messaging_display_message_payload_t));
      ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
    }
    break;
    case ZB_ZCL_CALENDAR_PUBLISH_CALENDAR_CB_ID:
      TRACE_MSG(TRACE_APP1, "ZB_ZCL_CALENDAR_PUBLISH_CALENDAR_CB_ID", (FMT__0));
      ihd_dev_handle_publish_calendar(ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param,
                                                                     zb_zcl_calendar_publish_calendar_payload_t),
                                      ZB_ZCL_DEVICE_CMD_PARAM_CMD_INFO(param));
      ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
      break;
    case ZB_ZCL_CALENDAR_PUBLISH_WEEK_PROFILE_CB_ID:
      TRACE_MSG(TRACE_APP1, "ZB_ZCL_CALENDAR_PUBLISH_WEEK_PROFILE_CB_ID", (FMT__0));
      ihd_dev_handle_publish_week_profile(ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param,
                                                                         zb_zcl_calendar_publish_week_profile_payload_t),
                                          ZB_ZCL_DEVICE_CMD_PARAM_CMD_INFO(param));
      ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
      break;
    case ZB_ZCL_CALENDAR_PUBLISH_DAY_PROFILE_CB_ID:
      TRACE_MSG(TRACE_APP1, "ZB_ZCL_CALENDAR_PUBLISH_DAY_PROFILE_CB_ID", (FMT__0));
      ihd_dev_handle_publish_day_profile(ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param,
                                                                        zb_zcl_calendar_publish_day_profile_payload_t),
                                        ZB_ZCL_DEVICE_CMD_PARAM_CMD_INFO(param));
      ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
      break;
    case ZB_ZCL_CALENDAR_PUBLISH_SEASONS_CB_ID:
      TRACE_MSG(TRACE_APP1, "ZB_ZCL_CALENDAR_PUBLISH_SEASONS_CB_ID", (FMT__0));
      ihd_dev_handle_publish_seasons(ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param,
                                                                         zb_zcl_calendar_publish_seasons_payload_t),
                                          ZB_ZCL_DEVICE_CMD_PARAM_CMD_INFO(param));
      ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
      break;
    case ZB_ZCL_CALENDAR_PUBLISH_SPECIAL_DAYS_CB_ID:
      TRACE_MSG(TRACE_APP1, "ZB_ZCL_CALENDAR_PUBLISH_SPECIAL_DAYS_CB_ID", (FMT__0));
      ihd_dev_handle_publish_special_days(ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param,
                                                                         zb_zcl_calendar_publish_special_days_payload_t),
                                          ZB_ZCL_DEVICE_CMD_PARAM_CMD_INFO(param));
      ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
      break;
    case ZB_ZCL_CALENDAR_CANCEL_CALENDAR_CB_ID:
      TRACE_MSG(TRACE_APP1, "ZB_ZCL_CALENDAR_CANCEL_CALENDAR_CB_ID", (FMT__0));
      ihd_dev_handle_cancel_calendar(ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param,
                                                                    zb_zcl_calendar_cancel_calendar_payload_t),
                                     ZB_ZCL_DEVICE_CMD_PARAM_CMD_INFO(param));
      ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
      break;
    case ZB_ZCL_METERING_GET_PROFILE_RESPONSE_CB_ID:
    {
      TRACE_MSG(TRACE_APP1, "ZB_ZCL_METERING_GET_PROFILE_RESPONSE_CB_ID", (FMT__0));
      ihd_dev_handle_get_profile_response(param);
    }
    break;
    case ZB_ZCL_METERING_REQUEST_FAST_POLL_MODE_RESPONSE_CB_ID:
    {
      TRACE_MSG(TRACE_APP1, "ZB_ZCL_METERING_REQUEST_FAST_POLL_MODE_RESPONSE_CB_ID", (FMT__0));
      ihd_dev_handle_request_fast_poll_mode_response(param);
    }
    break;
    case ZB_ZCL_METERING_GET_SAMPLED_DATA_RESPONSE_CB_ID:
    {
      TRACE_MSG(TRACE_APP1, "ZB_ZCL_METERING_GET_SAMPLED_DATA_RESPONSE_CB_ID", (FMT__0));
      ihd_dev_handle_get_sampled_data_response(param);
    }
    break;
    case ZB_ZCL_METERING_PUBLISH_SNAPSHOT_CB_ID:
    {
      TRACE_MSG(TRACE_APP1, "ZB_ZCL_METERING_PUBLISH_SNAPSHOT_CB_ID", (FMT__0));
      ihd_dev_handle_publish_snapshot(param);
    }
    break;
    default:
      TRACE_MSG(TRACE_APP1, "Undefined callback was called, id=%d",
                (FMT__D, ZB_ZCL_DEVICE_CMD_PARAM_CB_ID(param)));
      break;
  }
  TRACE_MSG(TRACE_APP1, ">> ihd_zcl_cmd_device_cb param", (FMT__0));
}

/** [IHD_DEV_INIT] */
/* Application init: configure stack parameters (IEEE address, device role, channel mask etc),
 * register device context and ZCL cmd device callback), init application attributes/context. */
static void ihd_dev_app_init(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> ihd_dev_app_init", (FMT__0));
/** [IHD_DEV_INIT] */
  ZVUNUSED(param);

  zb_nvram_register_app1_read_cb(ihd_dev_read_nvram_app_data);
  zb_nvram_register_app1_write_cb(ihd_dev_write_nvram_app_data, ihd_dev_get_nvram_data_size);

  /** [REGISTER_DEVICE_CTX] */
  ZB_AF_REGISTER_DEVICE_CTX(&ihd_dev_zcl_ctx);
  ZB_AF_SET_ENDPOINT_HANDLER(IHD_DEV_ENDPOINT, ihd_dev_zcl_cmd_handler);
  /** [REGISTER_DEVICE_CTX] */

  /* device configuration */
  ihd_dev_ctx_init();
  ihd_dev_clusters_attrs_init(0);
  ZB_ZCL_REGISTER_DEVICE_CB(ihd_zcl_cmd_device_cb);
  /*zb_register_zboss_callback(ZB_ZCL_DEVICE_CB, SET_ZBOSS_CB(ihd_zcl_cmd_device_cb));*/
  ZB_ZCL_SET_REPORT_ATTR_CB(ihd_dev_reporting_cb);

  /* ZB configuration */
  zb_set_long_address(g_dev_addr);
/** [IHD_DEV_SET_ROLE] */
  zb_set_network_router_role(IHD_DEV_CHANNEL_MASK);
/** [IHD_DEV_SET_ROLE] */

  TRACE_MSG(TRACE_APP1, "<< ihd_dev_app_init", (FMT__0));
}

#if IHD_ENABLE_GET_SCHEDULED_PRICES
/** [CMD_GET_SCHEDULED_PRICES] */
/** Send Price - Get Scheduled Prices command. */
static void ihd_dev_cmd_get_scheduled_prices(zb_uint8_t param, zb_uint16_t user_param)
{
  TRACE_MSG(TRACE_APP1, ">> ihd_dev_cmd_get_scheduled_prices, param=%hd",
            (FMT__H, param));

  if (!param)
  {
    zb_buf_get_out_delayed_ext(ihd_dev_cmd_get_scheduled_prices, user_param, 0);
  }
  else
  {
    zb_zcl_price_get_scheduled_prices_payload_t pl = ZB_ZCL_PRICE_GET_SCHEDULED_PRICES_PAYLOAD_INIT;
    pl.number_of_events = 5;
    pl.start_time = 0;
    ZB_ZCL_PRICE_SEND_CMD_GET_SCHEDULED_PRICES(param,
                                            (zb_addr_u *)&g_dev_ctx.dev.lst[user_param].dev_addr,
                                            ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                                            g_dev_ctx.dev.lst[user_param].dev_ep,
                                            IHD_DEV_ENDPOINT,
                                            &pl);
    g_dev_ctx.dev.lst[user_param].pending_cmd = IHD_DEV_PRICE_GET_CURRENT_PRICE;
    ZB_SCHEDULE_APP_ALARM(ihd_dev_send_pending_cmd, user_param, IHD_PRICE_UPDATE_PERIOD);
  }

  TRACE_MSG(TRACE_APP1, "<< ihd_dev_cmd_get_scheduled_prices", (FMT__0));
}
/** [CMD_GET_SCHEDULED_PRICES] */
#endif


static void ihd_dev_cmd_get_tier_labels(zb_uint8_t param, zb_uint16_t user_param)
{
  zb_zcl_price_get_tier_labels_payload_t pl;

  TRACE_MSG(TRACE_APP1, ">> ihd_dev_cmd_get_tier_labels, param=%hd",
            (FMT__H, param));

  if (!param)
  {
    zb_buf_get_out_delayed_ext(ihd_dev_cmd_get_tier_labels, user_param, 0);
  }
  else
  {

    pl.issuer_tariff_id = 0x0005;
    ZB_ZCL_PRICE_SEND_CMD_GET_TIER_LABELS(param,
                                          (zb_addr_u *)&g_dev_ctx.dev.lst[user_param].dev_addr,
                                          ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                                          g_dev_ctx.dev.lst[user_param].dev_ep,
                                          IHD_DEV_ENDPOINT,
                                          &pl);
#if IHD_ENABLE_GET_SCHEDULED_PRICES
    g_dev_ctx.dev.lst[user_param].pending_cmd = IHD_DEV_PRICE_GET_SCHEDULED_PRICES;
    ZB_SCHEDULE_APP_ALARM(ihd_dev_send_pending_cmd, user_param, IHD_PRICE_UPDATE_PERIOD);
#else
    g_dev_ctx.dev.lst[user_param].pending_cmd = IHD_DEV_PRICE_GET_CURRENT_PRICE;
    ZB_SCHEDULE_APP_ALARM(ihd_dev_send_pending_cmd, user_param, IHD_PRICE_UPDATE_PERIOD);
#endif
  }

  TRACE_MSG(TRACE_APP1, "<< ihd_dev_cmd_get_tier_labels", (FMT__0));
}


/** [CMD_GET_CURRENT_PRICE] */
/** Send Price - Get Current Price command. */
static void ihd_dev_cmd_get_current_price(zb_uint8_t param, zb_uint16_t user_param)
{
  zb_uint8_t requestor_rx_on_when_idle = zb_get_rx_on_when_idle();

  TRACE_MSG(TRACE_APP1, ">> ihd_dev_cmd_get_current_price, param=%hd",
            (FMT__H, param));

  if (!param)
  {
    zb_buf_get_out_delayed_ext(ihd_dev_cmd_get_current_price, user_param, 0);
  }
  else
  {
    ZB_ZCL_PRICE_SEND_CMD_GET_CURRENT_PRICE(param,
                                            (zb_addr_u *)&g_dev_ctx.dev.lst[user_param].dev_addr,
                                            ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                                            g_dev_ctx.dev.lst[user_param].dev_ep,
                                            IHD_DEV_ENDPOINT,
                                            requestor_rx_on_when_idle);

    g_dev_ctx.dev.lst[user_param].pending_cmd = IHD_DEV_PRICE_GET_TIER_LABELS;
    ZB_SCHEDULE_APP_ALARM(ihd_dev_send_pending_cmd, user_param, IHD_PRICE_UPDATE_PERIOD);
  }

  TRACE_MSG(TRACE_APP1, "<< ihd_dev_cmd_get_current_price", (FMT__0));
}
/** [CMD_GET_CURRENT_PRICE] */

/** [ihd_dev_cmd_get_last_message] */
/** Send Get Last Message command. */
static void ihd_dev_cmd_get_last_message(zb_uint8_t param, zb_uint16_t user_param)
{
  TRACE_MSG(TRACE_APP1, ">> ihd_dev_cmd_get_last_message, param=%hd",(FMT__H, param));

  if (!param)
  {
    zb_buf_get_out_delayed_ext(ihd_dev_cmd_get_last_message, user_param, 0);
  }
  else
  {
    zb_zcl_messaging_send_get_last_msg(param,
      (zb_addr_u *)&g_dev_ctx.dev.lst[user_param].dev_addr,
      ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
      g_dev_ctx.dev.lst[user_param].dev_ep,
      IHD_DEV_ENDPOINT,
      NULL);
    g_dev_ctx.dev.lst[user_param].pending_cmd = IHD_DEV_MESSAGING_GET_LAST_MESSAGE;
    ZB_SCHEDULE_APP_ALARM(ihd_dev_send_pending_cmd, user_param, IHD_MESSAGE_UPDATE_PERIOD);
  }

  TRACE_MSG(TRACE_APP1, "<< ihd_dev_cmd_get_last_message", (FMT__0));
}
/** [ihd_dev_cmd_get_last_message] */

/** Send Read Attributes for Metering cluster on remote device - get Unit Of Measure and Summation
 * Formatting values. */
static void ihd_dev_read_formatting_attr(zb_uint8_t param, zb_uint16_t user_param)
{
  zb_bufid_t buf = param;
  zb_uint8_t *cmd_ptr = NULL;
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_APP1, "ihd_dev_read_formatting_attr param %hd idx %d", (FMT__H_D, param, user_param));

  ZB_ASSERT(g_dev_ctx.dev.lst[user_param].used);
  /* query metering device about formatting attributes */
  ZB_ZCL_GENERAL_INIT_READ_ATTR_REQ(buf, cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);
  ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, ZB_ZCL_ATTR_METERING_UNIT_OF_MEASURE_ID);
  ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, ZB_ZCL_ATTR_METERING_SUMMATION_FORMATTING_ID);

  ZB_ZCL_GENERAL_SEND_READ_ATTR_REQ(buf,
                                    cmd_ptr,
                                    g_dev_ctx.dev.lst[user_param].dev_addr,
                                    ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                                    g_dev_ctx.dev.lst[user_param].dev_ep,
                                    IHD_DEV_ENDPOINT,
                                    ZB_AF_HA_PROFILE_ID,
                                    ZB_ZCL_CLUSTER_ID_METERING,
                                    NULL);
  if (ret != RET_OK)
  {
    g_dev_ctx.dev.lst[user_param].pending_cmd = IHD_DEV_METERING_READ_FORMATTING;
    ZB_SCHEDULE_APP_ALARM(ihd_dev_send_pending_cmd, user_param, IHD_CMD_RETRY_TIMEOUT);
  }
}


/** Process Read Attribute response from the Metering device and store results. */
static void ihd_dev_read_metering_attrs(zb_bufid_t buf, zb_zcl_parsed_hdr_t *cmd_info)
{
  zb_zcl_read_attr_res_t *resp;
  zb_uint8_t dev_idx;
  zb_ieee_addr_t ieee_addr;

  TRACE_MSG(TRACE_APP1, ">> ihd_dev_read_metering_attrs", (FMT__0));
  if (zb_address_ieee_by_short(ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr, ieee_addr) == RET_OK)
  {
    dev_idx = ihd_dev_get_idx(ieee_addr,
                              ZB_ZCL_CLUSTER_ID_METERING,
                              ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint);

    if (dev_idx != 0xFF)
    {
      ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(buf, resp);

      while (resp)
      {
        if (resp->status != ZB_ZCL_STATUS_SUCCESS)
        {
          TRACE_MSG(TRACE_ERROR, "attribute read error: attr 0x%x, status %d", (FMT__D_D, resp->attr_id, resp->status));
          ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(buf, resp);
          continue;
        }

        switch (resp->attr_id)
        {
          case ZB_ZCL_ATTR_METERING_UNIT_OF_MEASURE_ID:
            g_dev_ctx.dev.lst[dev_idx].u.metering.unit_of_measure = resp->attr_value[0];
            ihd_dev_show_attr(ZB_ZCL_ATTR_METERING_UNIT_OF_MEASURE_ID, dev_idx);
            break;

          case ZB_ZCL_ATTR_METERING_SUMMATION_FORMATTING_ID:
            g_dev_ctx.dev.lst[dev_idx].u.metering.summ_fmt_left =  ZB_ZCL_METERING_FORMATTING_LEFT(resp->attr_value[0]);
            g_dev_ctx.dev.lst[dev_idx].u.metering.summ_fmt_right = ZB_ZCL_METERING_FORMATTING_RIGHT(resp->attr_value[0]);
            g_dev_ctx.dev.lst[dev_idx].u.metering.summ_fmt_suppr = ZB_ZCL_METERING_FORMATTING_SUPPRESS_ZERO(resp->attr_value[0]);
            ihd_dev_show_attr(ZB_ZCL_ATTR_METERING_SUMMATION_FORMATTING_ID, dev_idx);
            break;

          default:
            TRACE_MSG(TRACE_ERROR, "invalid attribute %d", (FMT__D, resp->attr_id));
            break;
        }

        ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(buf, resp);
      }
    }
  }

  TRACE_MSG(TRACE_APP1, "<< ihd_dev_read_metering_attrs", (FMT__0));
}

/** Process Metering device's reports (display measurements). */
static void ihd_dev_reporting_cb(zb_zcl_addr_t *addr, zb_uint8_t ep, zb_uint16_t cluster_id,
                       zb_uint16_t attr_id, zb_uint8_t attr_type, zb_uint8_t *value)
{
  zb_ieee_addr_t dev_ieee_addr;
  zb_uint8_t dev_idx;
  ZVUNUSED(attr_type);

  TRACE_MSG(TRACE_APP1, ">> ihd_dev_reporting_cb", (FMT__0));

  if (addr->addr_type == ZB_ZCL_ADDR_TYPE_SHORT)
  {
    if (zb_address_ieee_by_short(addr->u.short_addr, dev_ieee_addr) != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "invalid reporting: unknown dev %d", (FMT__D, addr->u.short_addr));
      return;
    }
  }
  else if (addr->addr_type == ZB_ZCL_ADDR_TYPE_IEEE)
  {
    ZB_IEEE_ADDR_COPY(dev_ieee_addr, addr->u.ieee_addr);
  }

  dev_idx = ihd_dev_get_idx(dev_ieee_addr, cluster_id, ep);
  if (dev_idx == 0xFF)
  {
    TRACE_MSG(TRACE_ERROR, "unexpected report from addr " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(dev_ieee_addr)));
    return;
  }

  if (cluster_id != ZB_ZCL_CLUSTER_ID_METERING
      || (attr_id != ZB_ZCL_ATTR_METERING_CURRENT_SUMMATION_DELIVERED_ID
          && attr_id != ZB_ZCL_ATTR_METERING_STATUS_ID))
  {
    TRACE_MSG(TRACE_ERROR, "invalid reporting: cluster %d, attr %d", (FMT__D_D, cluster_id, attr_id));
    return;
  }

  switch (attr_id)
  {
    case ZB_ZCL_ATTR_METERING_CURRENT_SUMMATION_DELIVERED_ID:
      ZB_MEMCPY(&g_dev_ctx.dev.lst[dev_idx].u.metering.curr_summ_delivered, value, sizeof(g_dev_ctx.dev.lst[dev_idx].u.metering.curr_summ_delivered));
      ihd_dev_show_attr(ZB_ZCL_ATTR_METERING_CURRENT_SUMMATION_DELIVERED_ID, dev_idx);
      break;

    case ZB_ZCL_ATTR_METERING_STATUS_ID:
      g_dev_ctx.dev.lst[dev_idx].u.metering.status = *value;
      ihd_dev_show_attr(ZB_ZCL_ATTR_METERING_STATUS_ID, dev_idx);
      break;

    default:
      break;
  }

  if (g_dev_ctx.first_measurement_received == ZB_FALSE)
  {
    ZB_SCHEDULE_APP_CALLBACK(ihd_dev_save_data, 0);
    g_dev_ctx.first_measurement_received = ZB_TRUE;
  }

  TRACE_MSG(TRACE_APP1, "<< ihd_dev_reporting_cb", (FMT__0));
}


/********************* In-Home Display ZR  **************************/

MAIN()
{
  ARGV_UNUSED;

  ZB_SET_TRACE_ON();
  ZB_SET_TRAF_DUMP_ON();

  ZB_INIT("display_device");

  ihd_dev_app_init(0);

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


static void ihd_dev_metering_get_snapshot(zb_uint8_t param, zb_uint16_t user_param)
{
  if (user_param != 0xFF)
  {
    zb_zcl_metering_get_snapshot_payload_t pl;

    TRACE_MSG(TRACE_APP1, ">> ihd_dev_metering_get_snapshot", (FMT__0));

    pl.earliest_start_time = zb_get_utc_time() - 100;
    pl.latest_end_time = zb_get_utc_time() + 100;
    /* An offset of zero (0x00) indicates that the first snapshot satisfying the selection
     * criteria should be returned, 0x01 the second, and so on. */
    pl.snapshot_offset = prev_get_snapshot_offset;
    pl.snapshot_cause = ZB_ZCL_METERING_CAUSE_GENERAL;

    ZB_ZCL_METERING_SEND_CMD_GET_SNAPSHOT(param,
                                          (zb_addr_u *)&g_dev_ctx.dev.lst[user_param].dev_addr,
                                          ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                                          g_dev_ctx.dev.lst[user_param].dev_ep,
                                          IHD_DEV_ENDPOINT,
                                          &pl, NULL);

    TRACE_MSG(TRACE_APP1, "<< ihd_dev_metering_get_snapshot", (FMT__0));
  }
  else
  {
    zb_buf_free(param);
  }

}


static void ihd_dev_metering_get_sampled_data(zb_uint8_t param, zb_uint16_t user_param)
{
  if (user_param != 0xFF)
  {
    zb_zcl_metering_get_sampled_data_payload_t pl;

    TRACE_MSG(TRACE_APP1, ">> ihd_dev_metering_get_sampled_data", (FMT__0));

    pl.sample_id = 0x0001;
    pl.earliest_sample_time = zb_get_utc_time() - 100;
    pl.sample_type = ZB_ZCL_METERING_SAMPLE_TYPE_CONSUMPTION_DELIVERED;
    pl.number_of_samples = 3;

    ZB_ZCL_METERING_SEND_CMD_GET_SAMPLED_DATA(param,
                                              (zb_addr_u *)&g_dev_ctx.dev.lst[user_param].dev_addr,
                                              ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                                              g_dev_ctx.dev.lst[user_param].dev_ep,
                                              IHD_DEV_ENDPOINT,
                                              &pl, NULL);

    TRACE_MSG(TRACE_APP1, "<< ihd_dev_metering_get_sampled_data", (FMT__0));

    g_dev_ctx.dev.lst[user_param].pending_cmd = IHD_DEV_METERING_GET_SNAPSHOT;
    ZB_SCHEDULE_APP_ALARM(ihd_dev_send_pending_cmd, user_param, IHD_METERING_PERIOD);
  }
  else
  {
    zb_buf_free(param);
  }

}


static void ihd_dev_metering_request_fast_poll_mode(zb_uint8_t param, zb_uint16_t user_param)
{
  if (user_param != 0xFF)
  {
    zb_zcl_metering_request_fast_poll_mode_payload_t pl;

    TRACE_MSG(TRACE_APP1, ">> ihd_dev_metering_request_fast_poll_mode", (FMT__0));

    pl.fast_poll_update_period = 20;
    pl.duration_in_minutes = 10;

    ZB_ZCL_METERING_SEND_CMD_REQUEST_FAST_POLL_MODE(param,
                                                    (zb_addr_u *)&g_dev_ctx.dev.lst[user_param].dev_addr,
                                                    ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                                                    g_dev_ctx.dev.lst[user_param].dev_ep,
                                                    IHD_DEV_ENDPOINT,
                                                    &pl, NULL);

    TRACE_MSG(TRACE_APP1, "<< ihd_dev_metering_request_fast_poll_mode", (FMT__0));

    g_dev_ctx.dev.lst[user_param].pending_cmd = IHD_DEV_METERING_GET_SAMPLED_DATA;
    ZB_SCHEDULE_APP_ALARM(ihd_dev_send_pending_cmd, user_param, IHD_METERING_PERIOD);
  }
  else
  {
    zb_buf_free(param);
  }

}


static void ihd_dev_metering_get_profile(zb_uint8_t param, zb_uint16_t user_param)
{
  if (user_param != 0xFF)
  {
    zb_zcl_metering_get_profile_payload_t pl;

    TRACE_MSG(TRACE_APP1, ">> ihd_dev_metering_get_profile", (FMT__0));

    pl.interval_channel = ZB_ZCL_METERING_CONSUMPTION_DELIVERED;
    pl.end_time = zb_get_utc_time();
    pl.number_of_periods = 1;

    ZB_ZCL_METERING_SEND_CMD_GET_PROFILE(param,
                                         (zb_addr_u *)&g_dev_ctx.dev.lst[user_param].dev_addr,
                                         ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                                         g_dev_ctx.dev.lst[user_param].dev_ep,
                                         IHD_DEV_ENDPOINT,
                                         &pl, NULL);

    TRACE_MSG(TRACE_APP1, "<< ihd_dev_metering_get_profile", (FMT__0));

    g_dev_ctx.dev.lst[user_param].pending_cmd = IHD_DEV_METERING_REQUEST_FAST_POLL_MODE;
    ZB_SCHEDULE_APP_ALARM(ihd_dev_send_pending_cmd, user_param, IHD_METERING_PERIOD);
  }
  else
  {
    zb_buf_free(param);
  }

}


/** Application callback for incoming ZCL packets, called before ZCL packet processing by ZBOSS.
 * Depending on returned cmd_processed value ZBOSS decides if it is needed to do further processing of
 * the packet or not.
 * This callback may be used to implement application-specific logic of ZCL packets processing.
 */
/** [COMMAND_HANDLER] */
zb_uint8_t ihd_dev_zcl_cmd_handler(zb_uint8_t param)
{

  zb_bufid_t zcl_cmd_buf = param;
  zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);

  //zb_zcl_parsed_hdr_t cmd_info;

  //ZB_ZCL_COPY_PARSED_HEADER(param, &cmd_info);

  zb_uint8_t cmd_processed = ZB_FALSE;
  zb_uint16_t dev_idx;

  TRACE_MSG(TRACE_ZCL1, "> ihd_dev_zcl_cmd_handler param %hd", (FMT__H, param));
  TRACE_MSG(TRACE_ZCL2, "f type %hd cluster_id 0x%x cmd_id %hd",
            (FMT__H_D_H, cmd_info->is_common_command, cmd_info->cluster_id, cmd_info->cmd_id));

  if (cmd_info->cluster_id == ZB_ZCL_CLUSTER_ID_METERING)
  {
    if (cmd_info->is_common_command &&
        cmd_info->cmd_id == ZB_ZCL_CMD_READ_ATTRIB_RESP)
    {
      ihd_dev_read_metering_attrs(zcl_cmd_buf, cmd_info);
      zb_buf_free(zcl_cmd_buf);
      cmd_processed = ZB_TRUE;
    }
    else if (cmd_info->is_common_command &&
             cmd_info->cmd_id == ZB_ZCL_CMD_DISC_ATTRIB_RESP)
    {
      ihd_dev_disc_attr_resp_handler(zcl_cmd_buf, cmd_info);

      dev_idx = ihd_dev_get_idx_by_cmd_info(cmd_info);
      g_dev_ctx.dev.lst[dev_idx].pending_cmd = IHD_DEV_METERING_GET_PROFILE;
      ZB_SCHEDULE_APP_ALARM(ihd_dev_send_pending_cmd, dev_idx, IHD_METERING_PERIOD);

      zb_buf_free(zcl_cmd_buf);
      cmd_processed = ZB_TRUE;
    }
  }

  TRACE_MSG(TRACE_ZCL1, "< ihd_dev_zcl_cmd_handler %hd", (FMT__H, cmd_processed));
  return cmd_processed;
}
/** [COMMAND_HANDLER] */

/** Send Discover Attributes command (Metering cluster). */
void ihd_dev_discover_metering_attrs(zb_uint8_t param, zb_uint16_t user_param)
{
  TRACE_MSG(TRACE_APP1, "ihd_dev_discover_metering_attrs param %hd", (FMT__H, param));
  ZB_ZCL_GENERAL_DISC_READ_ATTR_REQ(param, ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                                    0, /* start attribute id */
                                    0xff, /* maximum attribute id-s */
                                    g_dev_ctx.dev.lst[user_param].dev_addr,
                                    ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                                    g_dev_ctx.dev.lst[user_param].dev_ep,
                                    IHD_DEV_ENDPOINT,
                                    ZB_AF_HA_PROFILE_ID,
                                    ZB_ZCL_CLUSTER_ID_METERING,
                                    NULL);
}

/** Handle Discover Attributes Response command. */
void ihd_dev_disc_attr_resp_handler(zb_bufid_t cmd_buf, zb_zcl_parsed_hdr_t *cmd_info)
{
  zb_zcl_disc_attr_info_t *disc_attr_info;
  zb_uint8_t complete;
  zb_uint8_t dev_idx;
  zb_ieee_addr_t ieee_addr;

  TRACE_MSG(TRACE_ZCL1, ">> ihd_dev_disc_attr_resp_handler", (FMT__0));

  if (zb_address_ieee_by_short(ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr, ieee_addr) == RET_OK)
  {
    ZB_ZCL_GENERAL_GET_COMPLETE_DISC_RES(cmd_buf, complete);

    ZB_ZCL_GENERAL_GET_NEXT_DISC_ATTR_RES(cmd_buf, disc_attr_info);
    while(disc_attr_info)
    {
      TRACE_MSG(TRACE_APP1, "Id: 0x%x - Data Type: 0x%x",
                (FMT__D_H, disc_attr_info->attr_id, disc_attr_info->data_type));
      ZB_ZCL_GENERAL_GET_NEXT_DISC_ATTR_RES(cmd_buf, disc_attr_info);
    }

    dev_idx = ihd_dev_get_idx(ieee_addr, cmd_info->cluster_id, ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint);
    if (dev_idx != 0xFF)
    {
      zb_buf_get_out_delayed_ext(ihd_dev_read_formatting_attr, dev_idx, 0);
    }
  }

  TRACE_MSG(TRACE_ZCL1, "<< disc_attr_resp_handler", (FMT__0));
}

/* Perform local operation - leave network */
void ihd_leave_nwk(zb_uint8_t param)
{
  zb_bufid_t buf = param;
  zb_zdo_mgmt_leave_param_t *req_param;

  req_param = ZB_BUF_GET_PARAM(buf, zb_zdo_mgmt_leave_param_t);
  ZB_BZERO(req_param, sizeof(zb_zdo_mgmt_leave_param_t));

  /* Set dst_addr == local address for local leave */
  req_param->dst_addr = ZB_PIBCACHE_NETWORK_ADDRESS();
  zdo_mgmt_leave_req(param, NULL);
}

void ihd_leave_nwk_delayed(zb_uint8_t param)
{
  ZVUNUSED(param);
  zb_buf_get_out_delayed(ihd_leave_nwk);
}

static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster)
{
  zb_uint8_t i;

  TRACE_MSG(TRACE_ZCL1, "finding_binding_cb status %d addr " TRACE_FORMAT_64 " ep %hd cluster %d",
            (FMT__D_A_H_D, status, TRACE_ARG_64(addr), ep, cluster));

  if (status == ZB_BDB_COMM_BIND_ASK_USER)
  {
    i = ihd_dev_add_to_list(addr, cluster, ep);

    if (i != 0xFF)
    {
      zb_time_t alarm_timeout = 0;
      if (cluster == ZB_ZCL_CLUSTER_ID_METERING)
      {
        g_dev_ctx.dev.lst[i].pending_cmd = IHD_DEV_METERING_DISCOVER_ATTRS;
        alarm_timeout = IHD_MESSAGE_UPDATE_PERIOD;
      }
      else if (cluster == ZB_ZCL_CLUSTER_ID_PRICE)
      {
        g_dev_ctx.dev.lst[i].pending_cmd = IHD_DEV_PRICE_GET_CURRENT_PRICE;
        alarm_timeout = IHD_PRICE_UPDATE_PERIOD;
      }
      else if (cluster == ZB_ZCL_CLUSTER_ID_MESSAGING)
      {
        g_dev_ctx.dev.lst[i].pending_cmd = IHD_DEV_MESSAGING_GET_LAST_MESSAGE;
        alarm_timeout = IHD_MESSAGE_UPDATE_PERIOD;
      }
      else if (cluster == ZB_ZCL_CLUSTER_ID_CALENDAR)
      {
        g_dev_ctx.dev.lst[i].pending_cmd = IHD_DEV_CALENDAR_GET_CALENDAR;
        alarm_timeout = IHD_CALENDAR_UPDATE_PERIOD;
      }
      ZB_SCHEDULE_APP_ALARM(ihd_dev_send_pending_cmd, i, alarm_timeout);
    }
  }

  return ZB_TRUE;
}

void ihd_finding_binding_target(zb_uint8_t param)
{
  ZVUNUSED(param);
  zb_bdb_finding_binding_target(IHD_DEV_ENDPOINT);
}

void ihd_finding_binding_initiator(zb_uint8_t param)
{
  ZVUNUSED(param);
  zb_bdb_finding_binding_initiator(IHD_DEV_ENDPOINT, finding_binding_cb);
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

  TRACE_MSG(TRACE_APP1, ">> zboss_signal_handler: param %hd signal %hd status %hd",
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
        break;

      case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APP1, "Successfull steering, start f&b initiator", (FMT__0));
        zb_bdb_finding_binding_initiator(IHD_DEV_ENDPOINT, finding_binding_cb);
        ZB_SCHEDULE_APP_ALARM(ihd_finding_binding_target, 0, (ZB_BDBC_MIN_COMMISSIONING_TIME_S) * ZB_TIME_ONE_SECOND);
        break;

      case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:
        TRACE_MSG(TRACE_APP1, "Finding&binding initiator done", (FMT__0));
        break;

      case ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED:
        TRACE_MSG(TRACE_APP1, "Finding&binding target done", (FMT__0));
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
    TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
  }

  if (param)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_APP1, "<< zboss_signal_handler", (FMT__0));
}
/** [SIGNAL_HANDLER] */
