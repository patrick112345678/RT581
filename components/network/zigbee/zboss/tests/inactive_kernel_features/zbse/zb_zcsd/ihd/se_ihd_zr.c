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

#define ZB_TRACE_FILE_ID 63639
#include "zboss_api.h"
#include "zb_led_button.h"

#include "../common/se_common.h"
#include "../common/se_indication.h"

#ifdef ENABLE_RUNTIME_APP_CONFIG
#include "../common/se_cert.h"
#include "../common/se_ic.h"
static zb_ieee_addr_t g_dev_addr = IHD_DEV_ADDR;
#ifdef ENABLE_PRECOMMISSIONED_REJOIN
static zb_ext_pan_id_t g_ext_pan_id = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
#endif
#endif

#if defined DEBUG && !defined DEBUG_EXPOSE_KEYS
#define DEBUG_EXPOSE_KEYS
#endif

/** [IHD_DEV_DEFINE_PARAMS] */
#define IHD_DEV_ENDPOINT 10
#define IHD_DEV_CHANNEL_MASK (1L << 22)
/** [IHD_DEV_DEFINE_PARAMS] */

/** Send GetCurrentPrice command every minute. */
#define IHD_PRICE_UPDATE_PERIOD (60 * ZB_TIME_ONE_SECOND)

#define IHD_MESSAGE_UPDATE_PERIOD (60 * ZB_TIME_ONE_SECOND)

#define IHD_CALENDAR_UPDATE_PERIOD (20 * ZB_TIME_ONE_SECOND)

#define IHD_APP_DATA_PERSISTING_TIMEOUT 60 * ZB_TIME_ONE_SECOND

#define IHD_CMD_RETRY_TIMEOUT (5 * ZB_TIME_ONE_SECOND)

/** Enable/disable sending GetScheduledPrices command.
 * This command will be sent instead of every second GetCurrentPrices command.
 */
#define IHD_ENABLE_GET_SCHEDULED_PRICES 1

#define IHD_ENABLE_GET_CALENDAR 1

#define IHD_DEV_ENT_NUMBER 10

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
    IHD_DEV_PRICE_GET_CURRENT_PRICE,
    IHD_DEV_PRICE_GET_SCHEDULED_PRICES,
    IHD_DEV_DRLC_GET_SCHEDULED_EVENTS,
    IHD_DEV_MESSAGING_GET_LAST_MESSAGE,
    IHD_DEV_CALENDAR_GET_CALENDAR,
    IHD_DEV_ENERGY_MANAGEMENT_SEND_MANAGE_EVENT
} ihd_dev_ent_pending_cmd_type_t;

typedef ZB_PACKED_PRE struct ihd_dev_ent_s
{
    zb_uint8_t used;
    zb_uint16_t cluster_id;
    zb_ieee_addr_t dev_addr;                        /**< device address */
    zb_uint8_t dev_ep;                          /**< endpoint */
    zb_uint8_t aps_key_established;
    zb_uint8_t pending_cmd;

    union
    {
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

    zb_uint8_t align[2];
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

/** @struct ihd_device_ctx_s
 *  @brief Ih-Home device context
 */
typedef struct ihd_dev_ctx_s
{
    zb_bool_t first_measurement_received;

    /* display device attributes */
    zb_zcl_basic_attrs_t basic_attrs;        /**< Basic cluster attributes  */
    zb_zcl_kec_attrs_t kec_attrs;            /**< Key Establishment cluster attributes */
    zb_zcl_drlc_client_attrs_t drlc_attrs;   /**< DRLC cluster client attributes */

    /* attributes of discovered devices */
    ihd_dev_nvram_data_t dev;
} ihd_dev_ctx_t;


/* device context */
static ihd_dev_ctx_t g_dev_ctx;


/* Basic cluster attributes */
ZB_ZCL_DECLARE_BASIC_ATTR_LIST(basic_attr_list, g_dev_ctx.basic_attrs);


/* Key Establishment cluster attributes */
ZB_ZCL_DECLARE_KEC_ATTR_LIST(kec_attr_list, g_dev_ctx.kec_attrs);

/* DRLC cluster attributes */
ZB_ZCL_DECLARE_DRLC_ATTR_LIST(drlc_attr_list, g_dev_ctx.drlc_attrs);

/*********************  Device declaration  **************************/

ZB_SE_DECLARE_IN_HOME_DISPLAY_CLUSTER_LIST(ihd_dev_clusters, basic_attr_list, kec_attr_list, drlc_attr_list);

ZB_SE_DECLARE_IN_HOME_DISPLAY_EP(ihd_dev_ep, IHD_DEV_ENDPOINT, ihd_dev_clusters);

ZB_SE_DECLARE_IN_HOME_DISPLAY_CTX(ihd_dev_zcl_ctx, ihd_dev_ep);

/** [DECLARE_CLUSTERS] */


static void ihd_dev_show_attr(zb_uint16_t attr_id, zb_uint8_t dev_idx);

static void ihd_dev_save_data(zb_uint8_t param);
void ihd_dev_nvram_read_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length);
zb_ret_t ihd_dev_nvram_write_app_data(zb_uint8_t page, zb_uint32_t pos);
zb_uint16_t ihd_dev_get_nvram_data_size();
void ihd_dev_disc_attr_resp_handler(zb_buf_t *cmd_buf, zb_zcl_parsed_hdr_t *cmd_info);

static zb_uint8_t ihd_dev_zcl_cmd_handler(zb_uint8_t param);
static void ihd_dev_ctx_init();
static void ihd_dev_clusters_attrs_init(zb_uint8_t param);
static void ihd_dev_app_init(zb_uint8_t param);
static void ihd_dev_read_metering_attrs(zb_buf_t *buf, zb_zcl_parsed_hdr_t *cmd_info);
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
            ZB_SCHEDULE_ALARM_CANCEL(ihd_dev_send_pending_cmd, i);
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
            //ZB_SCHEDULE_CALLBACK2(ihd_send_report_event_status, 0, dev_idx);
        }
    }
    return 0xff;
}


static void ihd_dev_read_formatting_attr(zb_uint8_t param, zb_uint16_t user_param);
static void ihd_dev_cmd_get_current_price(zb_uint8_t param, zb_uint16_t user_param);
static void ihd_dev_cmd_get_scheduled_events(zb_uint8_t param, zb_uint16_t user_param);
static void ihd_dev_cmd_get_last_message(zb_uint8_t param, zb_uint16_t user_param);
#if IHD_ENABLE_GET_CALENDAR
static void ihd_dev_cmd_get_calendar(zb_uint8_t param, zb_uint16_t user_param);
static void ihd_dev_cmd_get_day_profiles(zb_uint8_t param, zb_uint16_t user_param);
static void ihd_dev_cmd_get_week_profiles(zb_uint8_t param, zb_uint16_t user_param);
static void ihd_dev_cmd_get_seasons(zb_uint8_t param, zb_uint16_t user_param);
static void ihd_dev_cmd_get_special_days(zb_uint8_t param, zb_uint16_t user_param);
static void ihd_dev_cmd_get_calendar_cancellation(zb_uint8_t param, zb_uint16_t user_param);
#endif
static void ihd_dev_cmd_send_manage_event(zb_uint8_t param, zb_uint16_t user_param);
static void ihd_dev_cmd_get_scheduled_prices(zb_uint8_t param, zb_uint16_t user_param);
void ihd_dev_discover_metering_attrs(zb_uint8_t param, zb_uint16_t user_param);

/** Send pending command for device by index in device list. */
static void ihd_dev_send_pending_cmd(zb_uint8_t i)
{
    if (g_dev_ctx.dev.lst[i].used &&
            zb_se_has_valid_key_by_ieee(g_dev_ctx.dev.lst[i].dev_addr) &&
            g_dev_ctx.dev.lst[i].pending_cmd)
    {
        TRACE_MSG(TRACE_APP1, "ihd_dev_send_pending_cmd: idx %hd pending_cmd %hd", (FMT__H_H, i, g_dev_ctx.dev.lst[i].pending_cmd));
        switch (g_dev_ctx.dev.lst[i].pending_cmd)
        {
        case IHD_DEV_METERING_DISCOVER_ATTRS:
            ZB_GET_OUT_BUF_DELAYED2(ihd_dev_discover_metering_attrs, i);
            break;
        case IHD_DEV_METERING_READ_FORMATTING:
            ZB_GET_OUT_BUF_DELAYED2(ihd_dev_read_formatting_attr, i);
            break;
        case IHD_DEV_PRICE_GET_CURRENT_PRICE:
            ZB_GET_OUT_BUF_DELAYED2(ihd_dev_cmd_get_current_price, i);
            break;
        case IHD_DEV_PRICE_GET_SCHEDULED_PRICES:
            ZB_GET_OUT_BUF_DELAYED2(ihd_dev_cmd_get_scheduled_prices, i);
            break;
        case IHD_DEV_DRLC_GET_SCHEDULED_EVENTS:
            ZB_GET_OUT_BUF_DELAYED2(ihd_dev_cmd_get_scheduled_events, i);
            break;
        case IHD_DEV_MESSAGING_GET_LAST_MESSAGE:
            ZB_GET_OUT_BUF_DELAYED2(ihd_dev_cmd_get_last_message, i);
            break;
        case IHD_DEV_CALENDAR_GET_CALENDAR:
#if IHD_ENABLE_GET_CALENDAR
            ZB_GET_OUT_BUF_DELAYED2(ihd_dev_cmd_get_calendar, i);
#endif
            break;
        case IHD_DEV_ENERGY_MANAGEMENT_SEND_MANAGE_EVENT:
            ZB_GET_OUT_BUF_DELAYED2(ihd_dev_cmd_send_manage_event, i);
            break;
        default:
            /* Should not happen! */
            TRACE_MSG(TRACE_ERROR, "unknown pending_cmd!", (FMT__0));
            break;
        }
        g_dev_ctx.dev.lst[i].pending_cmd = IHD_DEV_NO_CMD;
    }
}

/** Handle "APS key established" event - force pending commands if device is found in device list. */
static void ihd_dev_aps_key_established(zb_ieee_addr_t addr)
{
    zb_uint8_t i = 0;

    TRACE_MSG(TRACE_APP1, "ihd_dev_aps_key_established: addr " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(addr)));

    while (i < ZB_ARRAY_SIZE(g_dev_ctx.dev.lst))
    {
        if (g_dev_ctx.dev.lst[i].used &&
                ZB_IEEE_ADDR_CMP(g_dev_ctx.dev.lst[i].dev_addr, addr))
        {
            ihd_dev_send_pending_cmd(i);
        }
        ++i;
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
    zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);
    ZB_SCHEDULE_ALARM(ihd_dev_save_data, 0, IHD_APP_DATA_PERSISTING_TIMEOUT);
}

/** Read remote device's stored data from NVRAM. */
void ihd_dev_read_nvram_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length)
{
    zb_ret_t ret;

    TRACE_MSG(TRACE_APP1, ">> ihd_dev_read_nvram_app_data: page %hd pos %d", (FMT__H_D, page, pos));

    ZB_ASSERT(payload_length == sizeof(ihd_dev_nvram_data_t));

    ret = zb_osif_nvram_read(page, pos, (zb_uint8_t *)&g_dev_ctx.dev, sizeof(ihd_dev_nvram_data_t));

    TRACE_MSG(TRACE_APP1, "<< ihd_dev_read_nvram_app_data: ret %d", (FMT__D, ret));
}

/** Application callback called on NVRAM writing operation (application dataset). */
zb_ret_t ihd_dev_write_nvram_app_data(zb_uint8_t page, zb_uint32_t pos)
{
    zb_ret_t ret;

    TRACE_MSG(TRACE_APP1, ">> ihd_dev_write_nvram_app_data: page %hd, pos %d", (FMT__H_D, page, pos));

    ret = zb_osif_nvram_write(page, pos, (zb_uint8_t *)&g_dev_ctx.dev, sizeof(ihd_dev_nvram_data_t));

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

    g_dev_ctx.kec_attrs.kec_suite = ZB_KEC_SUPPORTED_CRYPTO_ATTR;

    g_dev_ctx.drlc_attrs = ZB_ZCL_DECLARE_DRLC_ATTR_LIST_INIT;
    g_dev_ctx.drlc_attrs.device_class_value = ZB_ZCL_DRLC_DEVICE_CLASS_SMART_APPLIANCE;

    TRACE_MSG(TRACE_APP1, "<< ihd_dev_clusters_attrs_init", (FMT__0));
}

/** Handle received parameters for Price - Get Current Price command.
    Now trace received values. */
static void ihd_show_price_param(const zb_zcl_price_publish_price_payload_t *p)
{
    TRACE_MSG(TRACE_APP1, "PublishPrice:", (FMT__0));
    TRACE_MSG(TRACE_APP1, "provider=%d, issuer_event=%d",
              (FMT__D_D, p->provider_id, p->issuer_event_id));
    TRACE_MSG(TRACE_APP1, "price=%ld", (FMT__L, p->price));
}


#if IHD_ENABLE_GET_CALENDAR
/** Send Get Calendar command (ZCL Calendar cluster). */
/** [zb_zcl_calendar_send_cmd_get_calendar]  */
static void ihd_dev_cmd_get_calendar(zb_uint8_t param, zb_uint16_t user_param)
{
    TRACE_MSG(TRACE_APP1, ">> ihd_dev_cmd_get_calendar, param=%hd", (FMT__H, param));

    if (!param)
    {
        ZB_GET_OUT_BUF_DELAYED2(ihd_dev_cmd_get_calendar, user_param);
    }
    else
    {
        zb_zcl_calendar_get_calendar_payload_t pl =
        {
            .provider_id = IHD_PROVIDER_ID,
        };
        zb_zcl_calendar_send_cmd_get_calendar(param,
                                              (zb_addr_u *)&g_dev_ctx.dev.lst[user_param].dev_addr,
                                              ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                                              g_dev_ctx.dev.lst[user_param].dev_ep,
                                              IHD_DEV_ENDPOINT,
                                              &pl,
                                              NULL);

        g_dev_ctx.dev.lst[user_param].pending_cmd = IHD_DEV_CALENDAR_GET_CALENDAR;
        ZB_SCHEDULE_ALARM(ihd_dev_send_pending_cmd, user_param, IHD_CALENDAR_UPDATE_PERIOD);
    }

    TRACE_MSG(TRACE_APP1, "<< ihd_dev_cmd_get_calendar", (FMT__0));
}
/** [zb_zcl_calendar_send_cmd_get_calendar]  */

/** [zb_zcl_calendar_send_cmd_get_day_profiles] */
/** Send Get Day Profiles command. */
static void ihd_dev_cmd_get_day_profiles(zb_uint8_t param, zb_uint16_t user_param)
{
    TRACE_MSG(TRACE_APP1, ">> ihd_dev_cmd_get_day_profiles, param=%hd", (FMT__H, param));

    if (!param)
    {
        ZB_GET_OUT_BUF_DELAYED2(ihd_dev_cmd_get_day_profiles, user_param);
    }
    else
    {
        zb_zcl_calendar_get_day_profiles_payload_t pl =
        {
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
    TRACE_MSG(TRACE_APP1, ">> ihd_dev_cmd_get_week_profiles, param=%hd", (FMT__H, param));

    if (!param)
    {
        ZB_GET_OUT_BUF_DELAYED2(ihd_dev_cmd_get_week_profiles, user_param);
    }
    else
    {
        zb_zcl_calendar_get_week_profiles_payload_t pl =
        {
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
    TRACE_MSG(TRACE_APP1, ">> ihd_dev_cmd_get_seasons, param=%hd", (FMT__H, param));

    if (!param)
    {
        ZB_GET_OUT_BUF_DELAYED2(ihd_dev_cmd_get_seasons, user_param);
    }
    else
    {
        zb_zcl_calendar_get_seasons_payload_t pl =
        {
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
    TRACE_MSG(TRACE_APP1, ">> ihd_dev_cmd_get_special_days, param=%hd", (FMT__H, param));

    if (!param)
    {
        ZB_GET_OUT_BUF_DELAYED2(ihd_dev_cmd_get_special_days, user_param);
    }
    else
    {
        zb_zcl_calendar_get_special_days_payload_t pl =
        {
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
    TRACE_MSG(TRACE_APP1, ">> ihd_dev_cmd_get_calendar_cancellation, param=%hd", (FMT__H, param));

    if (!param)
    {
        ZB_GET_OUT_BUF_DELAYED2(ihd_dev_cmd_get_calendar_cancellation, user_param);
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

    ZB_SCHEDULE_CALLBACK2(ihd_dev_cmd_get_day_profiles, 0, ihd_dev_get_idx_by_cmd_info(in_cmd_info));

}

/** Handle Publish Week Profile command (ZCL Calendar cluster). */
static void ihd_dev_handle_publish_week_profile(const zb_zcl_calendar_publish_week_profile_payload_t *in,
        const zb_zcl_parsed_hdr_t *in_cmd_info)
{

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

    ZB_SCHEDULE_CALLBACK2(ihd_dev_cmd_get_seasons, 0, ihd_dev_get_idx_by_cmd_info(in_cmd_info));

}

static void ihd_dev_handle_publish_day_profile(const zb_zcl_calendar_publish_day_profile_payload_t *in,
        const zb_zcl_parsed_hdr_t *in_cmd_info)
{

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

    ZB_SCHEDULE_CALLBACK2(ihd_dev_cmd_get_week_profiles, 0, ihd_dev_get_idx_by_cmd_info(in_cmd_info));

}

static void ihd_dev_handle_publish_special_days(const zb_zcl_calendar_publish_special_days_payload_t *in,
        const zb_zcl_parsed_hdr_t *in_cmd_info)
{
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

    ZB_SCHEDULE_CALLBACK2(ihd_dev_cmd_get_calendar_cancellation, 0, ihd_dev_get_idx_by_cmd_info(in_cmd_info));
}

static void ihd_dev_handle_publish_seasons(const zb_zcl_calendar_publish_seasons_payload_t *in,
        const zb_zcl_parsed_hdr_t *in_cmd_info)
{
    TRACE_MSG(TRACE_APP1, ">> ihd_dev_handle_publish_seasons(in=%p)", (FMT__P, in));

    TRACE_MSG(TRACE_APP1, "in->provider_id = %lx", (FMT__D, in->provider_id));
    TRACE_MSG(TRACE_APP1, "in->issuer_event_id = %lx", (FMT__D, in->issuer_event_id));
    TRACE_MSG(TRACE_APP1, "in->issuer_calendar_id = %lx", (FMT__D, in->issuer_calendar_id));
    TRACE_MSG(TRACE_APP1, "in->command_index = %hx", (FMT__H, in->command_index));
    TRACE_MSG(TRACE_APP1, "in->total_number_of_commands = %hx", (FMT__H, in->total_number_of_commands));
    TRACE_MSG(TRACE_APP1, "in->season_entry = %p", (FMT__P, in->season_entry));
    TRACE_MSG(TRACE_APP1, "in->number_of_entries_in_this_command = %hx", (FMT__H, in->number_of_entries_in_this_command));

    TRACE_MSG(TRACE_APP1, "<< ihd_dev_handle_publish_seasons", (FMT__0));

    ZB_SCHEDULE_CALLBACK2(ihd_dev_cmd_get_special_days, 0, ihd_dev_get_idx_by_cmd_info(in_cmd_info));
}

/** Handle Cancel Calendar command (ZCL Calendar cluster). */
static void ihd_dev_handle_cancel_calendar(const zb_zcl_calendar_cancel_calendar_payload_t *in,
        const zb_zcl_parsed_hdr_t *in_cmd_info)
{
    ZVUNUSED(in_cmd_info);

    TRACE_MSG(TRACE_APP1, ">> handle_report_cancel_calendar(in=%p)", (FMT__P, in));

    TRACE_MSG(TRACE_APP1, "in->provider_id = %lx", (FMT__D, in->provider_id));
    TRACE_MSG(TRACE_APP1, "in->issuer_calendar_id = %lx", (FMT__D, in->issuer_calendar_id));
    TRACE_MSG(TRACE_APP1, "in->calendar_type = %hx", (FMT__H, in->calendar_type));

    TRACE_MSG(TRACE_APP1, ">> handle_report_cancel_calendar(in=%p)", (FMT__P, in));
}
#endif  /* IHD_ENABLE_GET_CALENDAR */


/** Get application parameters for DRLC - Load Control Event command.
    Now emulate some real values. */
static zb_ret_t ihd_handle_load_control_event(const zb_zcl_drlc_lce_payload_t *in,
        zb_zcl_drlc_report_event_status_payload_t *out)
{
    TRACE_MSG(TRACE_APP1, ">> handle_load_control_event(in=%p, out=%p) <<", (FMT__P_P, in, out));
    TRACE_MSG(TRACE_APP1, "in->issuer_event_id = %x", (FMT__D, in->issuer_event_id));
    TRACE_MSG(TRACE_APP1, "in->device_class = %d", (FMT__D, in->device_class));
    TRACE_MSG(TRACE_APP1, "in->utility_enrollment_group = %hd", (FMT__H, in->utility_enrollment_group));
    TRACE_MSG(TRACE_APP1, "in->start_time = %d", (FMT__D, in->start_time));
    TRACE_MSG(TRACE_APP1, "in->duration_in_minutes = %d", (FMT__D, in->duration_in_minutes));
    TRACE_MSG(TRACE_APP1, "in->criticality_level = %hd", (FMT__H, in->criticality_level));
    TRACE_MSG(TRACE_APP1, "in->event_control = %hd", (FMT__H, in->event_control));
    /*Here we must get events if present and pass them to client*/
    if (zb_get_utc_time() >= (in->start_time))
    {
        out->event_status = ZB_ZCL_DRLC_EVENT_EVENT_STARTED;     /* (M) */
    }
    else
    {
        out->event_status = ZB_ZCL_DRLC_EVENT_LCE_RECEIVED;     /* (M) */
    }
    out->issuer_event_id = in->issuer_event_id;             /* (M) */
    out->event_status_time = zb_get_utc_time();             /* (M) */
    out->criticality_level_applied = in->criticality_level; /* (M) */
    out->event_control = 0xbc;                                /* (M) */
    out->signature_type = ZB_ZCL_DRLC_LCE_NO_SIGNATURE;              /* (M) */

    return RET_OK;
}

/** Get application parameters for DRLC - Cancek Load Control Event command.
    Now emulate some real values. */
static void ihd_handle_cancel_load_control_event(const zb_zcl_drlc_cancel_lce_payload_t *in,
        zb_zcl_drlc_report_event_status_payload_t *out)
{
    TRACE_MSG(TRACE_APP1, ">> ihd_handle_cancel_load_control_event(in=%p, out=%p) <<", (FMT__P_P, in, out));
    TRACE_MSG(TRACE_APP1, "in->issuer_event_id = %x", (FMT__D, in->issuer_event_id));
    TRACE_MSG(TRACE_APP1, "in->device_class = %d", (FMT__D, in->device_class));
    TRACE_MSG(TRACE_APP1, "in->utility_enrollment_group = %hd", (FMT__H, in->utility_enrollment_group));
    TRACE_MSG(TRACE_APP1, "in->cancel_control = %hd", (FMT__H, in->cancel_control));
    TRACE_MSG(TRACE_APP1, "in->effective_time = %d (must be 0)", (FMT__D, in->effective_time));
    /*Here we must get events if present and pass them to client*/
    out->issuer_event_id = in->issuer_event_id;     /* (M) */
    out->event_status = ZB_ZCL_DRLC_EVENT_EVENT_CANCELLED;/*(M) */
    out->event_status_time = zb_get_utc_time();     /* (M) */
    out->criticality_level_applied = 0x22;          /* (M) */
    out->cooling_temperature_set_point_applied = 0xd;/*(O) */
    out->heating_temperature_set_point_applied = 0xc;/*(O) */
    out->average_load_adjustment_percentage_applied = 0xb;/*(O)*/
    out->duty_cycle_applied = 0xa;                  /* (O) */
    out->event_control = 0xbc;                      /* (M) */
    out->signature_type = ZB_ZCL_DRLC_LCE_NO_SIGNATURE;    /* (M) */
}

/** [ihd_dev_cmd_send_manage_event]  */
/** Send Energy Management Cluster command - Manage Event */
static void ihd_dev_cmd_send_manage_event(zb_uint8_t param, zb_uint16_t user_param)
{
    zb_zcl_energy_management_manage_event_payload_t payload;

    TRACE_MSG(TRACE_APP1, ">> ihd_dev_cmd_send_manage_event param %hd", (FMT__H, param));
    ZB_ASSERT(g_dev_ctx.dev.lst[user_param].used);
    ZB_BZERO(&payload, sizeof(payload));
    payload.issuer_event_id = 0x01;
    payload.device_class = ZB_ZCL_DRLC_DEVICE_CLASS_POOL_PUMP;
    payload.utility_enrollment_group = 0x88;
    payload.actions_required = 1 << ZB_ZCL_ENERGY_MANAGEMENT_ACTIONS_OPT_INTO_EVENT;

    ZB_ZCL_ENERGY_MANAGEMENT_SEND_CMD_MANAGE_EVENT(param,
            (zb_addr_u *)&g_dev_ctx.dev.lst[user_param].dev_addr,
            ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
            g_dev_ctx.dev.lst[user_param].dev_ep,
            IHD_DEV_ENDPOINT,
            &payload);

    TRACE_MSG(TRACE_APP1, "<< ihd_dev_cmd_send_manage_event", (FMT__0));
}
/** [ihd_dev_cmd_send_manage_event]  */

/** Handle received Energy MGMT Report Event Status command payload.
    Trace received values. */
static void ihd_handle_report_event_status(const zb_zcl_energy_management_report_event_status_payload_t *in,
        const zb_zcl_parsed_hdr_t *in_cmd_info)
{
    TRACE_MSG(TRACE_APP1, ">> handle_report_event_status(in=%p)", (FMT__P, in));
    TRACE_MSG(TRACE_APP1, "in->issuer_event_id = %lx", (FMT__D, in->issuer_event_id));
    TRACE_MSG(TRACE_APP1, "in->event_status = %lx", (FMT__H, in->event_status));
    TRACE_MSG(TRACE_APP1, "in->event_status_time = %lx", (FMT__D, in->event_status_time));
    TRACE_MSG(TRACE_APP1, "in->criticality_level_applied = %hx", (FMT__H, in->criticality_level_applied));
    TRACE_MSG(TRACE_APP1, "in->cooling_temperature_set_point_applied = %x", (FMT__D, in->cooling_temperature_set_point_applied));
    TRACE_MSG(TRACE_APP1, "in->heating_temperature_set_point_applied = %x", (FMT__D, in->heating_temperature_set_point_applied));
    TRACE_MSG(TRACE_APP1, "in->average_load_adjustment_percentage_applied = %hx", (FMT__H, in->average_load_adjustment_percentage_applied));
    TRACE_MSG(TRACE_APP1, "in->duty_cycle_applied = %hx", (FMT__H, in->duty_cycle_applied));
    TRACE_MSG(TRACE_APP1, "in->event_control = %hx", (FMT__H, in->event_control));

    if (ZB_ZCL_DRLC_EVENT_LCE_RECEIVED == in->event_status)
    {
        TRACE_MSG(TRACE_APP1, "Got ZB_ZCL_DRLC_EVENT_LCE_RECEIVED status from 0x%x", (FMT__D, ZB_ZCL_PARSED_HDR_SHORT_DATA(in_cmd_info).source.u.short_addr));
    }
    if (ZB_ZCL_DRLC_EVENT_REJECTED_UNDEFINED_EVENT == in->event_status)
    {
        TRACE_MSG(TRACE_APP1, "Got ZB_ZCL_DRLC_EVENT_REJECTED_UNDEFINED_EVENT status from 0x%x", (FMT__D, ZB_ZCL_PARSED_HDR_SHORT_DATA(in_cmd_info).source.u.short_addr));
    }
    if (ZB_ZCL_DRLC_EVENT_EVENT_STARTED == in->event_status)
    {
        TRACE_MSG(TRACE_APP1, "Got ZB_ZCL_DRLC_EVENT_EVENT_STARTED status from 0x%x", (FMT__D, ZB_ZCL_PARSED_HDR_SHORT_DATA(in_cmd_info).source.u.short_addr));
    }
    if (ZB_ZCL_DRLC_EVENT_EVENT_CANCELLED == in->event_status)
    {
        TRACE_MSG(TRACE_APP1, "Got ZB_ZCL_DRLC_EVENT_EVENT_CANCELLED status from 0x%x", (FMT__D, ZB_ZCL_PARSED_HDR_SHORT_DATA(in_cmd_info).source.u.short_addr));
    }
    TRACE_MSG(TRACE_APP1, "<< handle_report_event_status", (FMT__0));
}


/** [ihd_send_report_event_status]  */
/** Send DRLC - Report Event Status command. */
void ihd_send_report_event_status(zb_uint8_t param, zb_uint16_t user_param)
{
    TRACE_MSG(TRACE_APP1, ">> ihd_send_report_event_status param %hd", (FMT__H, param));
    if (!param)
    {
        ZB_GET_OUT_BUF_DELAYED2(ihd_send_report_event_status, user_param);
    }
    else
    {
        zb_zcl_drlc_report_event_status_payload_t payload = ZB_ZCL_DRLC_REPORT_EVENT_STATUS_PAYLOAD_INIT;

        ZB_ASSERT(g_dev_ctx.dev.lst[user_param].used);
        /*Here we must get events if present and pass them to client*/
        payload.issuer_event_id = 0x01;     /* (M) */
        payload.event_status = ZB_ZCL_DRLC_EVENT_REJECTED_UNDEFINED_EVENT;/*(M) */
        payload.event_status_time = zb_get_utc_time();     /* (M) */
        payload.criticality_level_applied = 0x22;          /* (M) */
        payload.event_control = 0xbc;                      /* (M) */
        payload.signature_type = ZB_ZCL_DRLC_LCE_NO_SIGNATURE;    /* (M) */

        ZB_ZCL_DRLC_SEND_CMD_REPORT_EVENT_STATUS(
            param,
            (zb_addr_u *)&g_dev_ctx.dev.lst[user_param].dev_addr,
            ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
            g_dev_ctx.dev.lst[user_param].dev_ep,
            IHD_DEV_ENDPOINT,
            &payload);
    }
    TRACE_MSG(TRACE_APP1, "<< ihd_send_report_event_status", (FMT__0));

}
/** [ihd_send_report_event_status]  */

/** [ihd_handle_cancel_all_load_control_events]  */
/** Handle received DRLC - Cancel All Load Control Events command. */
static void ihd_handle_cancel_all_load_control_events(zb_uint8_t cancel_control, const zb_zcl_parsed_hdr_t *in_cmd_info)
{
    zb_ieee_addr_t ieee_addr;
    zb_uint8_t dev_idx;

    if (zb_address_ieee_by_short(ZB_ZCL_PARSED_HDR_SHORT_DATA(in_cmd_info).source.u.short_addr, ieee_addr) == RET_OK)
    {
        dev_idx = ihd_dev_get_idx(ieee_addr,
                                  in_cmd_info->cluster_id,
                                  ZB_ZCL_PARSED_HDR_SHORT_DATA(in_cmd_info).src_endpoint);

        TRACE_MSG(TRACE_APP1, ">> ihd_handle_cancel_all_load_control_events(cancel_control=0x%hx) <<",
                  (FMT__H, cancel_control));
        if (dev_idx != 0xFF && ZB_ZCL_PARSED_HDR_SHORT_DATA(in_cmd_info).source.u.short_addr ==
                zb_address_short_by_ieee(g_dev_ctx.dev.lst[dev_idx].dev_addr))
        {
            TRACE_MSG(TRACE_APP1, "cancel all events from binded drlc, schedule event_status", (FMT__0));
            ZB_SCHEDULE_CALLBACK2(ihd_send_report_event_status, 0, dev_idx);
        }
    }
}
/** [ihd_handle_cancel_all_load_control_events]  */

/** Handle received DRLC - Cancel All Load Control Events command. */
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

/** Application device callback for ZCL commands. Used for:
    - receiving application-specific values for commands (e.g. ZB_ZCL_DRLC_LOAD_CONTROL_EVENT_CB_ID)
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
    case ZB_ZCL_DRLC_LOAD_CONTROL_EVENT_CB_ID:
    {
        TRACE_MSG(TRACE_APP1, "ZB_ZCL_DRLC_LOAD_CONTROL_EVENT_CB_ID", (FMT__0));
        ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = ihd_handle_load_control_event(
                ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_drlc_lce_payload_t),
                ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(param, zb_zcl_drlc_report_event_status_payload_t));
    }
    break;
    case ZB_ZCL_DRLC_CANCEL_LOAD_CONTROL_EVENT_CB_ID:
    {
        TRACE_MSG(TRACE_APP1, "ZB_ZCL_DRLC_CANCEL_LOAD_CONTROL_EVENT_CB_ID", (FMT__0));
        ihd_handle_cancel_load_control_event(
            ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_drlc_cancel_lce_payload_t),
            ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(param, zb_zcl_drlc_report_event_status_payload_t));
        ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
    }
    break;
    case ZB_ZCL_DRLC_CANCEL_ALL_LOAD_CONTROL_EVENTS_CB_ID:
    {
        TRACE_MSG(TRACE_APP1, "ZB_ZCL_DRLC_CANCEL_ALL_LOAD_CONTROL_EVENTS_CB_ID", (FMT__0));
        ihd_handle_cancel_all_load_control_events(*ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_uint8_t),
                ZB_ZCL_DEVICE_CMD_PARAM_CMD_INFO(param));
        ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
    }
    break;
    case ZB_ZCL_MESSAGING_DISPLAY_MSG_CB_ID:
    {
        TRACE_MSG(TRACE_APP1, "ZB_ZCL_MESSAGING_DISPLAY_MSG_CB_ID", (FMT__0));
        ihd_handle_display_msg(ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_messaging_display_message_payload_t));
        ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
    }
    break;
    case ZB_ZCL_ENERGY_MANAGEMENT_REPORT_EVENT_STATUS_CB_ID:
        TRACE_MSG(TRACE_APP1, "ZB_ZCL_ENERGY_MANAGEMENT_REPORT_EVENT_STATUS_CB_ID", (FMT__0));
        ihd_handle_report_event_status(ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param,
                                       zb_zcl_energy_management_report_event_status_payload_t),
                                       ZB_ZCL_DEVICE_CMD_PARAM_CMD_INFO(param));

        ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
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

    zb_osif_led_button_init();
    zb_nvram_register_app1_read_cb(ihd_dev_read_nvram_app_data);
    zb_nvram_register_app1_write_cb(ihd_dev_write_nvram_app_data, ihd_dev_get_nvram_data_size);

    if (zb_osif_button_state(BUTTON_LEFT) && zb_osif_button_state(BUTTON_RIGHT))
    {
        zb_se_start_nvram_erase_indication();
        zb_set_nvram_erase_at_start(ZB_TRUE);
        zb_se_stop_nvram_erase_indication();
    }

    /** [REGISTER_DEVICE_CTX] */
    ZB_AF_REGISTER_DEVICE_CTX(&ihd_dev_zcl_ctx);
    ZB_AF_SET_ENDPOINT_HANDLER(IHD_DEV_ENDPOINT, ihd_dev_zcl_cmd_handler);
    /** [REGISTER_DEVICE_CTX] */

    /* device configuration */
    ihd_dev_ctx_init();
    ihd_dev_clusters_attrs_init(0);
    zb_register_zboss_callback(ZB_ZCL_DEVICE_CB, SET_ZBOSS_CB(ihd_zcl_cmd_device_cb));
    ZB_ZCL_SET_REPORT_ATTR_CB(ihd_dev_reporting_cb);

    /* ZB configuration */
#ifdef ENABLE_RUNTIME_APP_CONFIG
    zb_set_long_address(g_dev_addr);
#ifdef ENABLE_PRECOMMISSIONED_REJOIN
    zb_set_use_extended_pan_id(&g_ext_pan_id);
#endif
#endif
    /** [IHD_DEV_SET_ROLE] */
    zb_se_set_network_router_role(IHD_DEV_CHANNEL_MASK);
    /** [IHD_DEV_SET_ROLE] */

#ifndef IHD_DEV_SUPPORT_MULTIPLE_COMMODITY
    zb_se_service_discovery_set_multiple_commodity_enabled(0);
#endif

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
        ZB_GET_OUT_BUF_DELAYED2(ihd_dev_cmd_get_scheduled_prices, user_param);
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
        ZB_SCHEDULE_ALARM(ihd_dev_send_pending_cmd, user_param, IHD_PRICE_UPDATE_PERIOD);
    }

    TRACE_MSG(TRACE_APP1, "<< ihd_dev_cmd_get_scheduled_prices", (FMT__0));
}
/** [CMD_GET_SCHEDULED_PRICES] */
#endif

/** [CMD_GET_CURRENT_PRICE] */
/** Send Price - Get Current Price command. */
static void ihd_dev_cmd_get_current_price(zb_uint8_t param, zb_uint16_t user_param)
{
    zb_uint8_t requestor_rx_on_when_idle = zb_get_rx_on_when_idle();

    TRACE_MSG(TRACE_APP1, ">> ihd_dev_cmd_get_current_price, param=%hd",
              (FMT__H, param));

    if (!param)
    {
        ZB_GET_OUT_BUF_DELAYED2(ihd_dev_cmd_get_current_price, user_param);
    }
    else
    {
        ZB_ZCL_PRICE_SEND_CMD_GET_CURRENT_PRICE(param,
                                                (zb_addr_u *)&g_dev_ctx.dev.lst[user_param].dev_addr,
                                                ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                                                g_dev_ctx.dev.lst[user_param].dev_ep,
                                                IHD_DEV_ENDPOINT,
                                                requestor_rx_on_when_idle);
#if IHD_ENABLE_GET_SCHEDULED_PRICES
        g_dev_ctx.dev.lst[user_param].pending_cmd = IHD_DEV_PRICE_GET_SCHEDULED_PRICES;
        ZB_SCHEDULE_ALARM(ihd_dev_send_pending_cmd, user_param, IHD_PRICE_UPDATE_PERIOD);
#else
        g_dev_ctx.dev.lst[user_param].pending_cmd = IHD_DEV_PRICE_GET_CURRENT_PRICE;
        ZB_SCHEDULE_ALARM(ihd_dev_send_pending_cmd, user_param, IHD_PRICE_UPDATE_PERIOD);
#endif
    }

    TRACE_MSG(TRACE_APP1, "<< ihd_dev_cmd_get_current_price", (FMT__0));
}
/** [CMD_GET_CURRENT_PRICE] */

/** [ihd_dev_cmd_get_scheduled_events]  */
/** Send DRLC - Get Scheduled Events command. */
static void ihd_dev_cmd_get_scheduled_events(zb_uint8_t param, zb_uint16_t user_param)
{
    zb_zcl_drlc_get_scheduled_events_payload_t payload = ZB_ZCL_DRLC_CMD_GET_SCHEDULED_EVENTS_PAYLOAD_INIT;

    TRACE_MSG(TRACE_APP1, ">> ihd_dev_cmd_get_scheduled_events, param=%hd", (FMT__H, param));

    if (!param)
    {
        ZB_GET_OUT_BUF_DELAYED2(ihd_dev_cmd_get_scheduled_events, user_param);
    }
    else
    {
        payload.start_time = zb_get_utc_time();         /* (M) */
        payload.number_of_events = 0;                   /* (M) */

        ZB_ZCL_DRLC_SEND_CMD_GET_SCHEDULED_EVENTS(param,
                (zb_addr_u *)&g_dev_ctx.dev.lst[user_param].dev_addr,
                ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                g_dev_ctx.dev.lst[user_param].dev_ep,
                IHD_DEV_ENDPOINT,
                &payload);
    }

    TRACE_MSG(TRACE_APP1, "<< ihd_dev_cmd_get_scheduled_events", (FMT__0));
}
/** [ihd_dev_cmd_get_scheduled_events]  */

/** [ihd_dev_cmd_get_last_message] */
/** Send Get Last Message command. */
static void ihd_dev_cmd_get_last_message(zb_uint8_t param, zb_uint16_t user_param)
{
    TRACE_MSG(TRACE_APP1, ">> ihd_dev_cmd_get_last_message, param=%hd", (FMT__H, param));

    if (!param)
    {
        ZB_GET_OUT_BUF_DELAYED2(ihd_dev_cmd_get_last_message, user_param);
    }
    else
    {
        zb_zcl_messaging_send_get_last_msg(param,
                                           (zb_addr_u *)&g_dev_ctx.dev.lst[user_param].dev_addr,
                                           ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                                           g_dev_ctx.dev.lst[user_param].dev_ep,
                                           IHD_DEV_ENDPOINT, NULL);
        g_dev_ctx.dev.lst[user_param].pending_cmd = IHD_DEV_MESSAGING_GET_LAST_MESSAGE;
        ZB_SCHEDULE_ALARM(ihd_dev_send_pending_cmd, user_param, IHD_MESSAGE_UPDATE_PERIOD);
    }

    TRACE_MSG(TRACE_APP1, "<< ihd_dev_cmd_get_last_message", (FMT__0));
}
/** [ihd_dev_cmd_get_last_message] */

/** Send Read Attributes for Metering cluster on remote device - get Unit Of Measure and Summation
 * Formatting values. */
static void ihd_dev_read_formatting_attr(zb_uint8_t param, zb_uint16_t user_param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_uint8_t *cmd_ptr = NULL;
    zb_ret_t ret = RET_OK;

    TRACE_MSG(TRACE_APP1, "ihd_dev_read_formatting_attr param %hd idx %d", (FMT__H_D, param, user_param));

    ZB_ASSERT(g_dev_ctx.dev.lst[user_param].used);
    /* query metering device about formatting attributes */
    ZB_ZCL_GENERAL_INIT_READ_ATTR_REQ(buf, cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);
    ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, ZB_ZCL_ATTR_METERING_UNIT_OF_MEASURE_ID);
    ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, ZB_ZCL_ATTR_METERING_SUMMATION_FORMATTING_ID);

    ret = ZB_ZCL_GENERAL_SEND_READ_ATTR_REQ(buf,
                                            cmd_ptr,
                                            g_dev_ctx.dev.lst[user_param].dev_addr,
                                            ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                                            g_dev_ctx.dev.lst[user_param].dev_ep,
                                            IHD_DEV_ENDPOINT,
                                            ZB_AF_SE_PROFILE_ID,
                                            ZB_ZCL_CLUSTER_ID_METERING,
                                            NULL);
    if (ret != RET_OK)
    {
        g_dev_ctx.dev.lst[user_param].pending_cmd = IHD_DEV_METERING_READ_FORMATTING;
        ZB_SCHEDULE_ALARM(ihd_dev_send_pending_cmd, user_param, IHD_CMD_RETRY_TIMEOUT);
    }
}


/** Process Read Attribute response from the Metering device and store results. */
static void ihd_dev_read_metering_attrs(zb_buf_t *buf, zb_zcl_parsed_hdr_t *cmd_info)
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
        ZB_SCHEDULE_CALLBACK(ihd_dev_save_data, 0);
        g_dev_ctx.first_measurement_received = ZB_TRUE;
    }

    TRACE_MSG(TRACE_APP1, "<< ihd_dev_reporting_cb", (FMT__0));
}


/*********************  SE In-Home Display ZR  **************************/

MAIN()
{
    ARGV_UNUSED;

    ZB_SET_TRACE_ON();
    ZB_SET_TRAF_DUMP_ON();

    ZB_INIT("display_device");

    ihd_dev_app_init(0);

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

/** Application callback for incoming ZCL packets, called before ZCL packet processing by ZBOSS.
 * Depending on returned cmd_processed value ZBOSS decides if it is needed to do further processing of
 * the packet or not.
 * This callback may be used to implement application-specific logic of ZCL packets processing.
 */
/** [COMMAND_HANDLER] */
zb_uint8_t ihd_dev_zcl_cmd_handler(zb_uint8_t param)
{
    zb_buf_t *zcl_cmd_buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
    zb_zcl_parsed_hdr_t *cmd_info = ZB_GET_BUF_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);
    zb_uint8_t cmd_processed = ZB_FALSE;

    TRACE_MSG(TRACE_ZCL1, "> ihd_dev_zcl_cmd_handler param %hd", (FMT__H, param));
    TRACE_MSG(TRACE_ZCL2, "f type %hd cluster_id 0x%x cmd_id %hd",
              (FMT__H_D_H, cmd_info->is_common_command, cmd_info->cluster_id, cmd_info->cmd_id));

    if (cmd_info->cluster_id == ZB_ZCL_CLUSTER_ID_METERING)
    {
        if (cmd_info->is_common_command &&
                cmd_info->cmd_id == ZB_ZCL_CMD_READ_ATTRIB_RESP)
        {
            ihd_dev_read_metering_attrs(zcl_cmd_buf, cmd_info);
            zb_free_buf(zcl_cmd_buf);
            cmd_processed = ZB_TRUE;
        }
        else if (cmd_info->is_common_command &&
                 cmd_info->cmd_id == ZB_ZCL_CMD_DISC_ATTRIB_RESP)
        {
            ihd_dev_disc_attr_resp_handler(zcl_cmd_buf, cmd_info);
            zb_free_buf(zcl_cmd_buf);
            cmd_processed = ZB_TRUE;
        }
    }

    TRACE_MSG(TRACE_ZCL1, "< ihd_dev_zcl_cmd_handler %hd", (FMT__H, cmd_processed));
    return cmd_processed;
}
/** [COMMAND_HANDLER] */

/** [AUTO_JOIN] */
/* Left button handler: start/stop join. */
static void ihd_dev_left_button_handler(zb_uint8_t param)
{
    if (!param)
    {
        /* Button is pressed, get buffer for outgoing command */
        ZB_GET_OUT_BUF_DELAYED(ihd_dev_left_button_handler);
    }
    else
    {
        if (zb_se_auto_join_start(param) == RET_OK)
        {
            TRACE_MSG(TRACE_APP1, "start auto_join", (FMT__0));
            zb_se_indicate_commissioning_started();
        }
        else
        {
            zb_se_auto_join_stop();
            zb_se_indicate_commissioning_stopped();
            TRACE_MSG(TRACE_APP1, "stop auto_join", (FMT__0));
            zb_free_buf(ZB_BUF_FROM_REF(param));
        }
    }
}
/** [AUTO_JOIN] */

/** Send Discover Attributes command (Metering cluster). */
void ihd_dev_discover_metering_attrs(zb_uint8_t param, zb_uint16_t user_param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);

    TRACE_MSG(TRACE_APP1, "ihd_dev_discover_metering_attrs param %hd", (FMT__H, param));
    ZB_ZCL_GENERAL_DISC_READ_ATTR_REQ(buf, ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                                      0, /* start attribute id */
                                      0xff, /* maximum attribute id-s */
                                      g_dev_ctx.dev.lst[user_param].dev_addr,
                                      ZB_APS_ADDR_MODE_64_ENDP_PRESENT,
                                      g_dev_ctx.dev.lst[user_param].dev_ep,
                                      IHD_DEV_ENDPOINT,
                                      ZB_AF_SE_PROFILE_ID,
                                      ZB_ZCL_CLUSTER_ID_METERING,
                                      NULL);
}

/** Handle Discover Attributes Response command. */
void ihd_dev_disc_attr_resp_handler(zb_buf_t *cmd_buf, zb_zcl_parsed_hdr_t *cmd_info)
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
        while (disc_attr_info)
        {
            TRACE_MSG(TRACE_APP1, "Id: 0x%x - Data Type: 0x%x",
                      (FMT__D_H, disc_attr_info->attr_id, disc_attr_info->data_type));
            ZB_ZCL_GENERAL_GET_NEXT_DISC_ATTR_RES(cmd_buf, disc_attr_info);
        }

        dev_idx = ihd_dev_get_idx(ieee_addr, cmd_info->cluster_id, ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint);
        if (dev_idx != 0xFF)
        {
            ZB_GET_OUT_BUF_DELAYED2(ihd_dev_read_formatting_attr, dev_idx);
        }
    }

    TRACE_MSG(TRACE_ZCL1, "<< disc_attr_resp_handler", (FMT__0));
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
        case ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY:
            if (ZB_BUF_LEN(ZB_BUF_FROM_REF(param)) > sizeof(zb_zdo_app_signal_hdr_t))
            {
                se_app_production_config_t *prod_cfg =
                    ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, se_app_production_config_t);

                TRACE_MSG(TRACE_APP1, "Loading application production config", (FMT__0));

                if (prod_cfg->version == SE_APP_PROD_CFG_CURRENT_VERSION)
                {
                    zb_set_node_descriptor_manufacturer_code(prod_cfg->manuf_code);
                }
            }
            break;

        /** [SIGNAL_HANDLER_GET_SIGNAL] */
        case ZB_ZDO_SIGNAL_SKIP_STARTUP:
#ifdef ENABLE_RUNTIME_APP_CONFIG
            zb_secur_ic_str_set(ihd_installcode);
#ifdef SE_CRYPTOSUITE_1
            zb_se_load_ecc_cert(KEC_CS1, ca_public_key_cs1, ihd_certificate_cs1, ihd_private_key_cs1);
#endif
#ifdef SE_CRYPTOSUITE_2
            zb_se_load_ecc_cert(KEC_CS2, ca_public_key_cs2, ihd_certificate_cs2, ihd_private_key_cs2);
#endif
#endif /* ENABLE_RUNTIME_APP_CONFIG */
            zboss_start_continue();
            break;

        case ZB_SE_SIGNAL_SKIP_JOIN:
            /* wait button click to start commissioning */
#ifdef ZB_USE_BUTTONS
            zb_button_register_handler(BUTTON_LEFT, 0, ihd_dev_left_button_handler);
#else
            ihd_dev_left_button_handler(0);
#endif
            break;

        case ZB_SIGNAL_DEVICE_FIRST_START:
        case ZB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_APP1, "Device STARTED OK: first_start %hd",
                      (FMT__H, sig == ZB_SIGNAL_DEVICE_FIRST_START));
            {
                zb_uint8_t key[ZB_CCM_KEY_SIZE];
                zb_se_debug_get_link_key(0, key);
                TRACE_MSG(TRACE_ERROR, "unique TCLK: " TRACE_FORMAT_128, (FMT__B, TRACE_ARG_128(key)));
                zb_se_debug_get_nwk_key(key);
                TRACE_MSG(TRACE_ERROR, "NWK key: " TRACE_FORMAT_128, (FMT__B, TRACE_ARG_128(key)));
                zb_se_debug_get_ic_key(key);
                TRACE_MSG(TRACE_ERROR, "TCLK by installcode: " TRACE_FORMAT_128, (FMT__B, TRACE_ARG_128(key)));
            }
            zb_se_indicate_default_start();
            break;

        /** [SIGNAL_HANDLER_CBKE_OK] */
        case ZB_SE_SIGNAL_CBKE_OK:
            TRACE_MSG(TRACE_APP1, "Key Establishment with Trust Center (CBKE) OK", (FMT__0));
#ifdef DEBUG_EXPOSE_KEYS
            {
                zb_uint16_t r_a_s = 0;
                zb_uint8_t key[ZB_CCM_KEY_SIZE];
                if (zb_se_debug_get_link_key(r_a_s, key) == RET_OK)
                {
                    TRACE_MSG(TRACE_APP1, "CBKE OK TCLK key " TRACE_FORMAT_128,
                              (FMT__B, TRACE_ARG_128(key)));
                    /** [SIGNAL_HANDLER_TC_SIGNAL_CHILD_JOIN] */

                    /* Note: that code broadcasts your TCLK! Use it only for debug purposes! Never keep it in production mode! */
                    zb_debug_bcast_key(NULL, key);
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
            break;
        /** [SIGNAL_HANDLER_CBKE_OK] */

        /** [SIGNAL_HANDLER_START_DISCOVERY] */
        case ZB_SE_SIGNAL_SERVICE_DISCOVERY_START:
            TRACE_MSG(TRACE_APP1, "Start Service Discovery", (FMT__0));
            zb_se_service_discovery_start(IHD_DEV_ENDPOINT);
            zb_se_indicate_service_discovery_started();
            break;
        /** [SIGNAL_HANDLER_START_DISCOVERY] */

        /** [SIGNAL_HANDLER_DO_BIND] */
        case ZB_SE_SIGNAL_SERVICE_DISCOVERY_DO_BIND:
        {
            zb_se_signal_service_discovery_bind_params_t *bind_params =
                ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_se_signal_service_discovery_bind_params_t);
            zb_uint8_t i;

            TRACE_MSG(TRACE_APP1, "can bind cluster 0x%x commodity_type %d remote_dev " TRACE_FORMAT_64,
                      (FMT__D_D_A, bind_params->cluster_id, bind_params->commodity_type,
                       TRACE_ARG_64(bind_params->device_addr)));
            /** [SIGNAL_HANDLER_DO_BIND] */

            /** [SIGNAL_HANDLER_BIND_DEV] */
            if (bind_params->cluster_id == ZB_ZCL_CLUSTER_ID_METERING)
            {
#ifdef IHD_DEV_SUPPORT_MULTIPLE_COMMODITY
                if (bind_params->commodity_type == ZB_ZCL_METERING_ELECTRIC_METERING
                        || bind_params->commodity_type == ZB_ZCL_METERING_GAS_METERING)
#endif
                {
                    i = ihd_dev_add_to_list(bind_params->device_addr,
                                            bind_params->cluster_id,
                                            bind_params->endpoint);

                    if (i != 0xFF)
                    {
#ifdef IHD_DEV_SUPPORT_MULTIPLE_COMMODITY
                        g_dev_ctx.dev.lst[i].u.metering.device_type = bind_params->commodity_type;
#endif
                        if (zb_se_has_valid_key_by_ieee(bind_params->device_addr))
                        {
                            ZB_GET_OUT_BUF_DELAYED2(ihd_dev_discover_metering_attrs, i);
                        }
                        else
                        {
                            g_dev_ctx.dev.lst[i].pending_cmd = IHD_DEV_METERING_DISCOVER_ATTRS;
                        }
                        zb_se_service_discovery_bind_req(param, bind_params->device_addr, bind_params->endpoint);
                        param = 0;
                    }
                }
            }
            else if (bind_params->cluster_id == ZB_ZCL_CLUSTER_ID_PRICE)
            {
#ifdef IHD_DEV_SUPPORT_MULTIPLE_COMMODITY
                if (bind_params->commodity_type == ZB_ZCL_METERING_ELECTRIC_METERING
                        || bind_params->commodity_type == ZB_ZCL_METERING_GAS_METERING)
#endif
                {
                    i = ihd_dev_add_to_list(bind_params->device_addr,
                                            bind_params->cluster_id,
                                            bind_params->endpoint);

                    if (i != 0xFF)
                    {
#ifdef IHD_DEV_SUPPORT_MULTIPLE_COMMODITY
                        g_dev_ctx.dev.lst[i].u.price.commodity_type = bind_params->commodity_type;
#endif
                        if (zb_se_has_valid_key_by_ieee(bind_params->device_addr))
                        {
                            ZB_GET_OUT_BUF_DELAYED2(ihd_dev_cmd_get_current_price, i);
                        }
                        else
                        {
                            g_dev_ctx.dev.lst[i].pending_cmd = IHD_DEV_PRICE_GET_CURRENT_PRICE;
                        }
                        zb_se_service_discovery_bind_req(param, bind_params->device_addr, bind_params->endpoint);
                        param = 0;
                    }
                }
            }
            else if (bind_params->cluster_id == ZB_ZCL_CLUSTER_ID_DRLC)
            {
                i = ihd_dev_add_to_list(bind_params->device_addr,
                                        bind_params->cluster_id,
                                        bind_params->endpoint);
                if (i != 0xFF)
                {
                    if (zb_se_has_valid_key_by_ieee(bind_params->device_addr))
                    {
                        ZB_GET_OUT_BUF_DELAYED2(ihd_dev_cmd_get_scheduled_events, i);
                    }
                    else
                    {
                        g_dev_ctx.dev.lst[i].pending_cmd = IHD_DEV_DRLC_GET_SCHEDULED_EVENTS;
                    }
                    zb_se_service_discovery_bind_req(param, bind_params->device_addr, bind_params->endpoint);
                    param = 0;
                }
            }
            else if (bind_params->cluster_id == ZB_ZCL_CLUSTER_ID_MESSAGING)
            {
                i = ihd_dev_add_to_list(bind_params->device_addr,
                                        bind_params->cluster_id,
                                        bind_params->endpoint);
                if (i != 0xFF)
                {
                    if (zb_se_has_valid_key_by_ieee(bind_params->device_addr))
                    {
                        ZB_GET_OUT_BUF_DELAYED2(ihd_dev_cmd_get_last_message, i);
                    }
                    else
                    {
                        g_dev_ctx.dev.lst[i].pending_cmd = IHD_DEV_MESSAGING_GET_LAST_MESSAGE;
                    }
                    zb_se_service_discovery_bind_req(param, bind_params->device_addr, bind_params->endpoint);
                    param = 0;
                }
            }
#if IHD_ENABLE_GET_CALENDAR
            else if (bind_params->cluster_id == ZB_ZCL_CLUSTER_ID_CALENDAR)
            {
                TRACE_MSG(TRACE_APP1, "Send bind request to Calendar Server", (FMT__0));
                i = ihd_dev_add_to_list(bind_params->device_addr,
                                        bind_params->cluster_id,
                                        bind_params->endpoint);
                if (i != 0xFF)
                {
                    if (zb_se_has_valid_key_by_ieee(bind_params->device_addr))
                    {
                        ZB_GET_OUT_BUF_DELAYED2(ihd_dev_cmd_get_calendar, i);
                    }
                    else
                    {
                        g_dev_ctx.dev.lst[i].pending_cmd = IHD_DEV_CALENDAR_GET_CALENDAR;
                    }
                    zb_se_service_discovery_bind_req(param, bind_params->device_addr, bind_params->endpoint);
                    param = 0;
                }
            }
#endif
            else if (bind_params->cluster_id == ZB_ZCL_CLUSTER_ID_ENERGY_MANAGEMENT)
            {
                i = ihd_dev_add_to_list(bind_params->device_addr,
                                        bind_params->cluster_id,
                                        bind_params->endpoint);
                if (i != 0xFF)
                {
                    TRACE_MSG(TRACE_APP1, "Send bind request to Energy Management cluster", (FMT__0));

                    if (zb_se_has_valid_key_by_ieee(bind_params->device_addr))
                    {
                        ZB_GET_OUT_BUF_DELAYED2(ihd_dev_cmd_send_manage_event, i);
                    }
                    else
                    {
                        g_dev_ctx.dev.lst[i].pending_cmd = IHD_DEV_ENERGY_MANAGEMENT_SEND_MANAGE_EVENT;
                    }
                    zb_se_service_discovery_bind_req(param, bind_params->device_addr, bind_params->endpoint);
                    param = 0;
                }
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

        /** [SIGNAL_HANDLER_DISCOVERY_OK] */
        case ZB_SE_SIGNAL_SERVICE_DISCOVERY_OK:
            TRACE_MSG(TRACE_APP1, "Service Discovery OK", (FMT__0));
            zb_se_indicate_commissioning_stopped();

            break;
        /** [SIGNAL_HANDLER_DISCOVERY_OK] */

        /** [SIGNAL_HANDLER_DISCOVERY_FAILED] */
        case ZB_SE_SIGNAL_SERVICE_DISCOVERY_FAILED:
            TRACE_MSG(TRACE_APP1, "Service Discovery failed", (FMT__0));
            zb_se_indicate_commissioning_fail();
            break;
        /** [SIGNAL_HANDLER_DISCOVERY_FAILED] */

        /**[SIGNAL_APS_KEY_READY]  */
        case ZB_SE_SIGNAL_APS_KEY_READY:
            /* NOTE: Place here Energy Management cluster call, because during handling
             * SIGNAL_HANDLER_DO_BIND for Energy Management cluster IHD don't
             * receive Transport key between itself and Energy Management cluster
             */
        {
            zb_ieee_addr_t *remote_device_addr = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_ieee_addr_t);

            TRACE_MSG(TRACE_APP1, "Partner link key established, remote device addr: " TRACE_FORMAT_64,
                      (FMT__A, TRACE_ARG_64(*remote_device_addr)));

            /**[SIGNAL_APS_KEY_READY]  */

            /**[SIGNAL_APS_KEY_READY_DEV]  */
            ihd_dev_aps_key_established(*remote_device_addr);
        }
        break;
        /**[SIGNAL_APS_KEY_READY_DEV]  */
        default:
            break;
        }
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
        zb_se_indicate_commissioning_fail();
    }

    if (param)
    {
        zb_free_buf(ZB_BUF_FROM_REF(param));
    }

    TRACE_MSG(TRACE_APP1, "<< zboss_signal_handler", (FMT__0));
}
/** [SIGNAL_HANDLER] */
