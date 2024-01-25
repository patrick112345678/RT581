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
/* PURPOSE: Metering ZED device sample
*/

#define ZB_TRACE_FILE_ID 40004

#include "zboss_api.h"
#include "zb_led_button.h"

#include "../common/se_common.h"
#include "../common/se_indication.h"

#include "se/zb_se_metering_device.h"

#ifdef ENABLE_RUNTIME_APP_CONFIG
#include "../common/se_cert.h"
#include "../common/se_ic.h"
static zb_ieee_addr_t g_dev_addr = EL_METERING_DEV_ADDR;
#ifdef ENABLE_PRECOMMISSIONED_REJOIN
static zb_ext_pan_id_t g_ext_pan_id = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
#endif
#endif

void metering_dev_request_tunnel(zb_uint8_t param);
void metering_tunneling_tx(zb_uint8_t param);
void metering_tunneling_close_tunnel(zb_uint8_t param);

/** [METERING_DEV_DEFINE_PARAMS] */
#define METERING_DEV_ENDPOINT 1
#define METERING_DEV_CHANNEL_MASK1 (1L << 22)
#define METERING_DEV_CHANNEL_MASK2 (1L << 1)
#define METERING_DEV_CHANNEL_PAGE_1 ZB_CHANNEL_PAGE0_2_4_GHZ
#define METERING_DEV_CHANNEL_PAGE_2 ZB_CHANNEL_PAGE28_SUB_GHZ
/** [METERING_DEV_DEFINE_PARAMS] */

/* SummationFormatting: XXXXXX.Y, do not suppress leading zeroes */
#define METERING_DEV_SUMM_FMT_LEFT 6
#define METERING_DEV_SUMM_FMT_RIGHT 1
#define METERING_DEV_SUMM_FMT_SUPPR 0
#define METERING_DEV_SUMMATION_FORMATTING \
  ZB_ZCL_METERING_FORMATTING_SET(METERING_DEV_SUMM_FMT_SUPPR, METERING_DEV_SUMM_FMT_LEFT, METERING_DEV_SUMM_FMT_RIGHT)
#define METERING_DEV_MEASURE_TIMEOUT ZB_TIME_ONE_SECOND
#define METERING_DEV_APP_DATA_PERSISTING_TIMEOUT 60 * ZB_TIME_ONE_SECOND

#define METERING_TUNNELING_PAYLOAD_SIZE 10
#define METERING_TUNNELING_MANUFACTURER_CODE 0xabcd

/** @struct metering_dev_nvram_data_s
 *  @brief metering device info that should be stored in NVRAM
 */
typedef ZB_PACKED_PRE struct metering_dev_nvram_data_s
{
    zb_uint48_t curr_summ_delivered;          /**< CurrentSummationDelivered attribute value */
    zb_uint8_t status;                        /**< Status attribute value */
    zb_uint8_t aling[1];
} ZB_PACKED_STRUCT metering_dev_nvram_data_t;

ZB_ASSERT_IF_NOT_ALIGNED_TO_4(metering_dev_nvram_data_t);

/** [DECLARE_CLUSTERS] */
/*********************  Clusters' attributes  **************************/

typedef struct metering_dev_tunneling_data_s
{
    zb_uint16_t tunnel_id;
    zb_uint16_t srv_addr;
    zb_uint8_t srv_ep;
} metering_dev_tunneling_data_t;

/** @struct metering_dev_ctx_s
 *  @brief Metering device context
 */
typedef struct metering_dev_ctx_s
{
    zb_bool_t first_measurement_done;

    /* metering device attributes */
    zb_zcl_basic_attrs_t basic_attrs;          /**< Basic cluster attributes */
    zb_zcl_kec_attrs_t kec_attrs;              /**< Key Establishement cluster attributes */
    zb_zcl_identify_attrs_t identify_attrs;    /**< Identify cluster attributes */
    zb_zcl_metering_attrs_t metering_attrs;    /**< Metering cluster attributes */
    metering_dev_tunneling_data_t metering_tunneling;
    zb_zcl_prepayment_attrs_t prepayment_attrs;
} metering_dev_ctx_t;

/* Device context */
static metering_dev_ctx_t g_dev_ctx;

static zb_int_t s_tx_suspended;

/* Basic cluster attributes */
ZB_ZCL_DECLARE_BASIC_ATTR_LIST(basic_attr_list, g_dev_ctx.basic_attrs);

/* Key Establishment cluster attributes */
ZB_ZCL_DECLARE_KEC_ATTR_LIST(kec_attr_list, g_dev_ctx.kec_attrs);

/* Identify cluster attributes */
ZB_ZCL_DECLARE_IDENTIFY_ATTR_LIST(identify_attr_list, g_dev_ctx.identify_attrs);

/* Metering cluster attributes */
ZB_ZCL_DECLARE_METERING_ATTR_LIST(metering_attr_list, g_dev_ctx.metering_attrs);

/* Metering cluster attributes */
ZB_ZCL_DECLARE_PREPAYMENT_ATTR_LIST(prepayment_attr_list, g_dev_ctx.prepayment_attrs);

/*********************  Device declaration  **************************/

ZB_SE_DECLARE_METERING_DEV_CLUSTER_LIST(metering_dev_clusters,
                                        basic_attr_list,
                                        kec_attr_list,
                                        identify_attr_list,
                                        metering_attr_list,
                                        prepayment_attr_list);
ZB_SE_DECLARE_METERING_DEV_EP(metering_dev_ep, METERING_DEV_ENDPOINT, metering_dev_clusters);

ZB_SE_DECLARE_METERING_DEV_CTX(metering_dev_zcl_ctx, metering_dev_ep);

/** [DECLARE_CLUSTERS] */


/*********************  Device-specific functions  **************************/

/** Trace Metering attributes by attribute id. */
static void metering_dev_show_attr(zb_uint16_t attr_id)
{
    switch (attr_id)
    {
    case ZB_ZCL_ATTR_METERING_CURRENT_SUMMATION_DELIVERED_ID:
    {
        TRACE_MSG(TRACE_APP1, "Current Summation Delivered: %ld %ld",
                  (FMT__L_L, g_dev_ctx.metering_attrs.curr_summ_delivered.high, g_dev_ctx.metering_attrs.curr_summ_delivered.low));
    }
    break;

    case ZB_ZCL_ATTR_METERING_STATUS_ID:
        TRACE_MSG(TRACE_APP1, "Device Status: 0x%x", (FMT__D, g_dev_ctx.metering_attrs.status));
        if (g_dev_ctx.metering_attrs.status & ZB_ZCL_METERING_ELECTRICITY_CHECK_METER)
        {
            TRACE_MSG(TRACE_APP1, "   - check meter", (FMT__0));
        }
        if (g_dev_ctx.metering_attrs.status & ZB_ZCL_METERING_ELECTRICITY_LOW_BATTERY)
        {
            TRACE_MSG(TRACE_APP1, "   - low battery", (FMT__0));
        }
        if (g_dev_ctx.metering_attrs.status & ZB_ZCL_METERING_ELECTRICITY_TAMPER_DETECT)
        {
            TRACE_MSG(TRACE_APP1, "   - tamper detect", (FMT__0));
        }
        if (g_dev_ctx.metering_attrs.status & ZB_ZCL_METERING_ELECTRICITY_POWER_FAILURE)
        {
            TRACE_MSG(TRACE_APP1, "   - power failure", (FMT__0));
        }
        if (g_dev_ctx.metering_attrs.status & ZB_ZCL_METERING_ELECTRICITY_POWER_QUALITY)
        {
            TRACE_MSG(TRACE_APP1, "   - power quality", (FMT__0));
        }
        if (g_dev_ctx.metering_attrs.status & ZB_ZCL_METERING_ELECTRICITY_LEAK_DETECT)
        {
            TRACE_MSG(TRACE_APP1, "   - leak detect", (FMT__0));
        }
        if (g_dev_ctx.metering_attrs.status & ZB_ZCL_METERING_ELECTRICITY_SERVICE_DISCONNECT_OPEN)
        {
            TRACE_MSG(TRACE_APP1, "   - service disconnect open", (FMT__0));
        }
        break;

    default:
        break;
    }
}


/** Save Metering device's data into NVRAM. */
static void metering_dev_save_data(zb_uint8_t param)
{
    ZVUNUSED(param);
    /* If we fail, trace is given and assertion is triggered */
    (void)zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);
    ZB_SCHEDULE_APP_ALARM(metering_dev_save_data, 0, METERING_DEV_APP_DATA_PERSISTING_TIMEOUT);
}

/** Read Metering device's stored data from NVRAM. */
void metering_dev_read_nvram_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length)
{
    metering_dev_nvram_data_t data;
    zb_ret_t ret;

    ZB_ASSERT(payload_length == sizeof(data));

    TRACE_MSG(TRACE_APP1, ">> metering_dev_read_nvram_app_data: page %hd pos %d", (FMT__H_D, page, pos));

    /* If we fail, trace is given and assertion is triggered */
    ret = zb_nvram_read_data(page, pos, (zb_uint8_t *)&data, sizeof(data));

    if (ret == RET_OK)
    {
        ZB_MEMCPY(&g_dev_ctx.metering_attrs.curr_summ_delivered, &data.curr_summ_delivered, sizeof(data.curr_summ_delivered));
        g_dev_ctx.metering_attrs.status = data.status;

        TRACE_MSG(TRACE_APP1, "Loaded metering device info", (FMT__0));
        metering_dev_show_attr(ZB_ZCL_ATTR_METERING_CURRENT_SUMMATION_DELIVERED_ID);
        metering_dev_show_attr(ZB_ZCL_ATTR_METERING_STATUS_ID);
    }

    TRACE_MSG(TRACE_APP1, "<< metering_dev_read_nvram_app_data: ret %d", (FMT__D, ret));
}

/** Application callback called on NVRAM writing operation (application dataset). */
zb_ret_t metering_dev_write_nvram_app_data(zb_uint8_t page, zb_uint32_t pos)
{
    metering_dev_nvram_data_t data;
    zb_ret_t ret;

    TRACE_MSG(TRACE_APP1, ">> metering_dev_write_nvram_app_data: page %hd, pos %d", (FMT__H_D, page, pos));

    ZB_MEMCPY(&data.curr_summ_delivered, &g_dev_ctx.metering_attrs.curr_summ_delivered, sizeof(data.curr_summ_delivered));
    data.status = g_dev_ctx.metering_attrs.status;

    /* If we fail, trace is given and assertion is triggered */
    ret = zb_nvram_write_data(page, pos, (zb_uint8_t *)&data, sizeof(data));

    TRACE_MSG(TRACE_APP1, "<< metering_dev_write_nvram_app_data: ret %d", (FMT__D, ret));

    return ret;
}

/** Get NVRAM application dataset size. */
zb_uint16_t metering_dev_get_nvram_data_size()
{
    TRACE_MSG(TRACE_APP1, "metering_dev_get_nvram_data_size: ret %hd", (FMT__H, sizeof(metering_dev_nvram_data_t)));
    return sizeof(metering_dev_nvram_data_t);
}

/** Init device context. */
void metering_dev_ctx_init()
{
    TRACE_MSG(TRACE_APP1, ">> metering_dev_ctx_init", (FMT__0));

    ZB_MEMSET(&g_dev_ctx, 0, sizeof(g_dev_ctx));

    TRACE_MSG(TRACE_APP1, "<< metering_dev_ctx_init", (FMT__0));
}

/** Init device ZCL attributes. */
void metering_dev_clusters_attrs_init(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> metering_dev_clusters_attrs_init", (FMT__0));
    ZVUNUSED(param);

    g_dev_ctx.basic_attrs.zcl_version = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
    g_dev_ctx.basic_attrs.power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

    g_dev_ctx.kec_attrs.kec_suite = ZB_KEC_SUPPORTED_CRYPTO_ATTR;

    g_dev_ctx.identify_attrs.identify_time = 0;

    g_dev_ctx.metering_attrs.status = ZB_ZCL_METERING_STATUS_DEFAULT_VALUE;
    g_dev_ctx.metering_attrs.unit_of_measure = ZB_ZCL_METERING_UNIT_KW_KWH_BINARY;
    g_dev_ctx.metering_attrs.summation_formatting = METERING_DEV_SUMMATION_FORMATTING;
    g_dev_ctx.metering_attrs.device_type = ZB_ZCL_METERING_ELECTRIC_METERING;

    TRACE_MSG(TRACE_APP1, "<< metering_dev_clusters_attrs_init", (FMT__0));
}

/** Init default reporting configuration of ZCL attributes (Metering - Current Summation Delivered
    and Status).
*/
void metering_dev_default_reporting_init(zb_uint8_t param)
{
    zb_zcl_reporting_info_t rep_info;

    TRACE_MSG(TRACE_APP1, ">> metering_dev_default_reporting_init", (FMT__0));
    ZVUNUSED(param);

    ZB_BZERO(&rep_info, sizeof(zb_zcl_reporting_info_t));
    rep_info.direction = ZB_ZCL_CONFIGURE_REPORTING_SEND_REPORT;
    rep_info.ep = METERING_DEV_ENDPOINT;
    rep_info.cluster_id = ZB_ZCL_CLUSTER_ID_METERING;
    rep_info.cluster_role = ZB_ZCL_CLUSTER_SERVER_ROLE;
    rep_info.dst.profile_id = ZB_AF_SE_PROFILE_ID;

    rep_info.u.send_info.min_interval = ZB_SE_METERING_DATA_UPDATE_RATE;
    rep_info.u.send_info.max_interval = ZB_ZCL_MAX_REPORTING_INTERVAL_DEFAULT;
    rep_info.u.send_info.def_min_interval = ZB_SE_METERING_DATA_UPDATE_RATE;
    rep_info.u.send_info.def_max_interval = ZB_ZCL_MAX_REPORTING_INTERVAL_DEFAULT;
    rep_info.u.send_info.delta.u32 = 0;

    rep_info.attr_id = ZB_ZCL_ATTR_METERING_CURRENT_SUMMATION_DELIVERED_ID;
    /* Note: that call writes to nvram! Call it only after device started. */
    zb_zcl_put_default_reporting_info(&rep_info);

    rep_info.attr_id = ZB_ZCL_ATTR_METERING_STATUS_ID;
    zb_zcl_put_default_reporting_info(&rep_info);

    TRACE_MSG(TRACE_APP1, "<< metering_dev_default_reporting_init", (FMT__0));
}

/** Application device callback for ZCL commands. Used for:
    - receiving application-specific values for commands (e.g. ZB_ZCL_DRLC_LOAD_CONTROL_EVENT_CB_ID)
    - providing received ZCL commands data to application (e.g. ZB_ZCL_PRICE_PUBLISH_PRICE_CB_ID)
    Application may ignore callback id-s in which it is not interested.
 */
static void metering_zcl_cmd_device_cb(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> metering_zcl_cmd_device_cb param %hd id %d", (FMT__H_D,
              param, ZB_ZCL_DEVICE_CMD_PARAM_CB_ID(param)));

    switch (ZB_ZCL_DEVICE_CMD_PARAM_CB_ID(param))
    {
    case ZB_ZCL_TUNNELING_REQUEST_TUNNEL_RESPONSE_CB_ID:
    {
        const zb_zcl_tunneling_request_tunnel_response_t *resp =
            ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_tunneling_request_tunnel_response_t);

        TRACE_MSG(TRACE_APP1, "ZB_ZCL_TUNNELING_REQUEST_TUNNEL_RESPONSE_CB_ID", (FMT__0));
        if (resp->tunnel_status == ZB_ZCL_TUNNELING_STATUS_SUCCESS)
        {
            /* Send tunneling data */
            g_dev_ctx.metering_tunneling.tunnel_id = resp->tunnel_id;
            ZB_GET_OUT_BUF_DELAYED(metering_tunneling_tx);
        }
        else
        {
            /* Forget tunneling srv */
            g_dev_ctx.metering_tunneling.srv_addr = ZB_UNKNOWN_SHORT_ADDR;
            g_dev_ctx.metering_tunneling.srv_ep = 0;
        }
    }
    break;

    case ZB_ZCL_TUNNELING_TRANSFER_DATA_SRV_CB_ID:
    {
        const zb_zcl_tunneling_transfer_data_payload_t *tr_data =
            ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_tunneling_transfer_data_payload_t);
        TRACE_MSG(TRACE_APP1, "ZB_ZCL_TUNNELING_TRANSFER_DATA_SRV_CB_ID", (FMT__0));
        DUMP_TRAF("recv:", tr_data->tun_data, tr_data->data_size, 0);
        ZB_GET_OUT_BUF_DELAYED(metering_tunneling_close_tunnel);
        ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
    }
    break;

    case ZB_ZCL_TUNNELING_TRANSFER_DATA_ERROR_SRV_CB_ID:
    {
        const zb_zcl_tunneling_transfer_data_error_t *tr_error =
            ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_tunneling_transfer_data_error_t);
        TRACE_MSG(TRACE_APP1, "ZB_ZCL_TUNNELING_TRANSFER_DATA_ERROR_SRV_CB_ID: reason %hd", (FMT__H, tr_error->transfer_data_status));
        if (tr_error->transfer_data_status == ZB_ZCL_TUNNELING_TRANSFER_DATA_STATUS_DATA_OVERFLOW)
        {
            ZB_GET_OUT_BUF_DELAYED(metering_tunneling_close_tunnel);
        }
        ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
    }
    break;

    default:
        TRACE_MSG(TRACE_APP1, "Undefined callback was called, id=%d",
                  (FMT__D, ZB_ZCL_DEVICE_CMD_PARAM_CB_ID(param)));
        break;
    }
    TRACE_MSG(TRACE_APP1, ">> metering_zcl_cmd_device_cb param", (FMT__0));
}

/** [METERING_DEV_INIT] */
/* Application init: configure stack parameters (IEEE address, device role, channel mask etc),
 * register device context and ZCL cmd device callback), init application attributes/context. */
void metering_dev_app_init(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> metering_dev_app_init", (FMT__0));
    /** [METERING_DEV_INIT] */
    ZVUNUSED(param);

    zb_osif_led_button_init();
    zb_nvram_register_app1_read_cb(metering_dev_read_nvram_app_data);
    zb_nvram_register_app1_write_cb(metering_dev_write_nvram_app_data, metering_dev_get_nvram_data_size);

    if (zb_osif_button_state(BUTTON_LEFT) && zb_osif_button_state(BUTTON_RIGHT))
    {
        zb_se_start_nvram_erase_indication();
        zb_set_nvram_erase_at_start(ZB_TRUE);
        zb_se_stop_nvram_erase_indication();
    }
    /** [REGISTER_DEVICE_CTX] */
    ZB_AF_REGISTER_DEVICE_CTX(&metering_dev_zcl_ctx);
    /** [REGISTER_DEVICE_CTX] */

    /* device configuration */
    metering_dev_ctx_init();
    metering_dev_clusters_attrs_init(0);
    zb_register_zboss_callback(ZB_ZCL_DEVICE_CB, SET_ZBOSS_CB(metering_zcl_cmd_device_cb));

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
        zb_channel_list_add(channel_list, METERING_DEV_CHANNEL_PAGE_1, METERING_DEV_CHANNEL_MASK1);
        zb_channel_list_add(channel_list, METERING_DEV_CHANNEL_PAGE_2, METERING_DEV_CHANNEL_MASK2);
        TRACE_MSG(TRACE_APP1, "ZED in MM mode start: page %d mask 0x%x page %d mask 0x%x",
                  (FMT__D_D_D_D, METERING_DEV_CHANNEL_PAGE_1, METERING_DEV_CHANNEL_MASK1, METERING_DEV_CHANNEL_PAGE_2, METERING_DEV_CHANNEL_MASK2));
        zb_se_set_network_ed_role_select_device(channel_list);
    }
#else
    /** [METERING_DEV_SET_ROLE] */
    zb_se_set_network_ed_role(METERING_DEV_CHANNEL_MASK1);
#endif
    /* Act as Sleepy End Device */
    zb_set_rx_on_when_idle(ZB_FALSE);
    /** [METERING_DEV_SET_ROLE] */

    TRACE_MSG(TRACE_APP1, "<< metering_dev_app_init", (FMT__0));
}


/** Generate and accumulate measurements - emulate real device. */
static void metering_dev_measure(zb_uint8_t param)
{
    zb_int24_t instantaneous_consumption;
    zb_int32_t measure;
    zb_uint48_t new_curr_summ_delivered;
    zb_uint8_t new_metering_status;

    ZVUNUSED(param);

    /* Update InstantaneousDemand and CurrentSummationDelivered every time we are here
     * (for now it is once a second).
     *
     * ZSE spec, subclause D.3.2.2.1.1
     *    CurrentSummationDelivered represents the most recent summed value of Energy, Gas,
     *    or Water delivered and consumed in the premises. CurrentSummationDelivered is updated
     *    continuously as new measurements are made.
     *
     * ZSE spec, subclause D.3.2.2.5.1:
     *    InstantaneousDemand is updated continuously as new measurements are made.
     *    The frequency of updates to this field is specific to the metering device, but
     *    should be within the range of once every second to once every 5 seconds.
     */
    measure = (zb_int32_t)ZB_RANDOM_VALUE(7);
    ZB_INT24_FROM_INT32(instantaneous_consumption, measure);
    new_curr_summ_delivered = g_dev_ctx.metering_attrs.curr_summ_delivered;
    ZB_UINT48_ADD_INT24(new_curr_summ_delivered, instantaneous_consumption);

    TRACE_MSG(TRACE_APP1, "instantaneous electricity consumption: %ld %ld",
              (FMT__L_L, instantaneous_consumption.high, instantaneous_consumption.low));

    ZB_ZCL_SET_ATTRIBUTE(METERING_DEV_ENDPOINT,
                         ZB_ZCL_CLUSTER_ID_METERING,
                         ZB_ZCL_CLUSTER_SERVER_ROLE,
                         ZB_ZCL_ATTR_METERING_CURRENT_SUMMATION_DELIVERED_ID,
                         (zb_uint8_t *)&new_curr_summ_delivered,
                         ZB_FALSE);

    new_metering_status = (1L << measure);

    ZB_ZCL_SET_ATTRIBUTE(METERING_DEV_ENDPOINT,
                         ZB_ZCL_CLUSTER_ID_METERING,
                         ZB_ZCL_CLUSTER_SERVER_ROLE,
                         ZB_ZCL_ATTR_METERING_STATUS_ID,
                         (zb_uint8_t *)&new_metering_status,
                         ZB_FALSE);

    metering_dev_show_attr(ZB_ZCL_ATTR_METERING_CURRENT_SUMMATION_DELIVERED_ID);
    metering_dev_show_attr(ZB_ZCL_ATTR_METERING_STATUS_ID);

    if (g_dev_ctx.first_measurement_done == ZB_FALSE)
    {
        ZB_SCHEDULE_APP_CALLBACK(metering_dev_save_data, 0);
        g_dev_ctx.first_measurement_done = ZB_TRUE;
    }

    ZB_SCHEDULE_APP_ALARM(metering_dev_measure, 0, METERING_DEV_MEASURE_TIMEOUT);
}


/*********************  SE Electricity Metering ZED  **************************/

MAIN()
{
    ARGV_UNUSED;

    ZB_SET_TRACE_ON();
    ZB_SET_TRAF_DUMP_OFF();

    ZB_INIT("metering_device");

    metering_dev_app_init(0);

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

/** Left button handler: start/stop join. */
static void metering_dev_left_button_handler(zb_uint8_t param)
{
    if (!param)
    {
        /* Button is pressed, get buffer for outgoing command */
        ZB_GET_OUT_BUF_DELAYED(metering_dev_left_button_handler);
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

/** [metering_dev_request_tunnel] */
void metering_dev_request_tunnel(zb_uint8_t param)
{
    zb_ret_t status;
    ZB_ZCL_TUNNELING_SEND_REQUEST_TUNNEL(param,
                                         g_dev_ctx.metering_tunneling.srv_addr,
                                         ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                         g_dev_ctx.metering_tunneling.srv_ep,
                                         METERING_DEV_ENDPOINT,
                                         ZB_AF_SE_PROFILE_ID,
                                         ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                                         NULL, /* no callback */
                                         ZB_ZCL_TUNNELING_PROTOCOL_MANUFACTURER_DEFINED,
                                         METERING_TUNNELING_MANUFACTURER_CODE, /* manufacturer_code */
                                         ZB_FALSE, /* no flow control */
                                         ZB_ZCL_TUNNELING_MAX_INCOMING_TRANSFER_SIZE, status);
}
/** [metering_dev_request_tunnel] */

void metering_tunneling_tx(zb_uint8_t param)
{
    zb_uint8_t tunneling_data[METERING_TUNNELING_PAYLOAD_SIZE];
    zb_uint8_t i;

    TRACE_MSG(TRACE_APP1, "metering_tunneling_tx param %hd", (FMT__H, param));

    for (i = 0; i < ZB_ARRAY_SIZE(tunneling_data); ++i)
    {
        tunneling_data[i] = i;
    }

    if (ZB_ZCL_TUNNELING_CLIENT_SEND_TRANSFER_DATA(param,
            METERING_DEV_ENDPOINT,
            ZB_AF_SE_PROFILE_ID,
            ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
            NULL, /* no callback */
            g_dev_ctx.metering_tunneling.tunnel_id,
            ZB_ARRAY_SIZE(tunneling_data) * sizeof(zb_uint8_t),
            tunneling_data) != RET_OK)
    {
        TRACE_MSG(TRACE_APP1, "error sending tunneling tx!", (FMT__H, param));
        ZB_FREE_BUF_BY_REF(param);
    }
}

/** [metering_tunneling_close_tunnel] */
void metering_tunneling_close_tunnel(zb_uint8_t param)
{
    ZB_ZCL_TUNNELING_SEND_CLOSE_TUNNEL(param,
                                       g_dev_ctx.metering_tunneling.srv_addr,
                                       ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                       g_dev_ctx.metering_tunneling.srv_ep,
                                       METERING_DEV_ENDPOINT,
                                       ZB_AF_SE_PROFILE_ID,
                                       ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                                       NULL, /* no callback */
                                       g_dev_ctx.metering_tunneling.tunnel_id);
}
/** [metering_tunneling_close_tunnel] */



/** Application signal handler. Used for informing application about important ZBOSS
    events/states (device started first time/rebooted, key establishment completed etc).
    Application may ignore signals in which it is not interested.
*/
/** [SIGNAL_HANDLER] */
void zboss_signal_handler(zb_uint8_t param)
{
    zb_zdo_app_signal_hdr_t *sg_p = NULL;
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

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

        case ZB_ZDO_SIGNAL_SKIP_STARTUP:
            TRACE_MSG(TRACE_APP1, "ZB_ZDO_SIGNAL_SKIP_STARTUP: boot, not started yet", (FMT__0));
            /* This call is here because reporting configuration is saved to nvram, so it must be called after nvram loaded. */
            metering_dev_default_reporting_init(0);
#ifdef ENABLE_RUNTIME_APP_CONFIG
            zb_secur_ic_str_set(el_metering_installcode);
#ifdef SE_CRYPTOSUITE_1
            zb_se_load_ecc_cert(KEC_CS1, ca_public_key_cs1, el_metering_certificate_cs1, el_metering_private_key_cs1);
#endif
#ifdef SE_CRYPTOSUITE_2
            zb_se_load_ecc_cert(KEC_CS2, ca_public_key_cs2, el_metering_certificate_cs2, el_metering_private_key_cs2);
#endif
#endif /* ENABLE_RUNTIME_APP_CONFIG */
            zboss_start_continue();
            break;

        case ZB_SE_SIGNAL_SKIP_JOIN:
            /* wait button click to start commissioning */
            TRACE_MSG(TRACE_APP1, "ZB_SE_SIGNAL_SKIP_JOIN", (FMT__0));
#ifdef ZB_USE_BUTTONS
            zb_button_register_handler(BUTTON_LEFT, 0, metering_dev_left_button_handler);
#else
            metering_dev_left_button_handler(0);
#endif
            break;

        case ZB_SIGNAL_DEVICE_FIRST_START:
        case ZB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_APP1, "Device STARTED OK: first_start %hd",
                      (FMT__H, sig == ZB_SIGNAL_DEVICE_FIRST_START));
            {
                zb_uint8_t key[ZB_CCM_KEY_SIZE];
                zb_se_debug_get_link_key(0, key);
                TRACE_MSG(TRACE_ERROR, "TCLK: " TRACE_FORMAT_128, (FMT__B, TRACE_ARG_128(key)));
                zb_se_debug_get_nwk_key(key);
                TRACE_MSG(TRACE_ERROR, "NWK key: " TRACE_FORMAT_128, (FMT__B, TRACE_ARG_128(key)));
                zb_se_debug_get_ic_key(key);
                TRACE_MSG(TRACE_ERROR, "TCLK by installcode: " TRACE_FORMAT_128, (FMT__B, TRACE_ARG_128(key)));
            }
            ZB_SCHEDULE_APP_ALARM_CANCEL(metering_dev_measure, ZB_ALARM_ANY_PARAM);
            ZB_SCHEDULE_APP_CALLBACK(metering_dev_measure, 0);
            zb_se_indicate_default_start();
            break;

        case ZB_SE_SIGNAL_CBKE_OK:
            TRACE_MSG(TRACE_APP1, "ZB_SE_SIGNAL_CBKE_OK: Key Establishment with Trust Center (CBKE) OK", (FMT__0));
            break;

        case ZB_SE_SIGNAL_SERVICE_DISCOVERY_START:
            TRACE_MSG(TRACE_APP1, "Start Service Discovery", (FMT__0));
            zb_se_service_discovery_start(METERING_DEV_ENDPOINT);
            zb_se_indicate_service_discovery_started();
            break;

        case ZB_SE_SIGNAL_SERVICE_DISCOVERY_DO_BIND:
        {
            zb_se_signal_service_discovery_bind_params_t *bind_params =
                ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_se_signal_service_discovery_bind_params_t);

            TRACE_MSG(TRACE_APP1, "can bind cluster %d commodity_type %d remote_dev " TRACE_FORMAT_64,
                      (FMT__D_D_A, bind_params->cluster_id, bind_params->commodity_type,
                       TRACE_ARG_64(bind_params->device_addr)));

            if (bind_params->cluster_id == ZB_ZCL_CLUSTER_ID_TUNNELING)
            {
                /* Do not bind, but request tunnel to this dev */
                g_dev_ctx.metering_tunneling.srv_addr = zb_address_short_by_ieee(bind_params->device_addr);
                g_dev_ctx.metering_tunneling.srv_ep = bind_params->endpoint;
                ZB_SCHEDULE_APP_CALLBACK(metering_dev_request_tunnel, param);
                param = 0;
            }
        }
        break;

        case ZB_SE_SIGNAL_SERVICE_DISCOVERY_BIND_OK:
        {
            zb_uint16_t *addr = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_uint16_t);
            TRACE_MSG(TRACE_APP1, "Bind dev 0x%x OK", (FMT__D, *addr));
            break;
        }

        case ZB_SE_SIGNAL_SERVICE_DISCOVERY_OK:
            TRACE_MSG(TRACE_APP1, "Service Discovery OK", (FMT__0));
            zb_se_indicate_commissioning_stopped();
            break;

        case ZB_SE_SIGNAL_SERVICE_DISCOVERY_FAILED:
            TRACE_MSG(TRACE_APP1, "Service Discovery failed", (FMT__0));
            zb_se_indicate_commissioning_fail(0,  /* erase nvram */
                                              0); /* reboot */
            break;

        case ZB_SIGNAL_SUBGHZ_SUSPEND:
            TRACE_MSG(TRACE_APP1, "sub-ghz. TX suspended", (FMT__0));
            s_tx_suspended = 1;
            break;
        case ZB_SIGNAL_SUBGHZ_RESUME:
            TRACE_MSG(TRACE_APP1, "sub-ghz. TX resumed", (FMT__0));
            s_tx_suspended = 0;
            break;

        case ZB_COMMON_SIGNAL_CAN_SLEEP:
            /* skip*/
            break;

        default:
            TRACE_MSG(TRACE_APP1, "zboss_signal_handler: skip sig %hd status %hd",
                      (FMT__H_H_H, param, sig, ZB_GET_APP_SIGNAL_STATUS(param)));
            break;
        }
    }
    else
    {
        zb_se_indicate_commissioning_fail(0,  /* erase nvram */
                                          0); /* reboot */
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
    }

    if (param)
    {
        zb_free_buf(ZB_BUF_FROM_REF(param));
    }

}
/** [SIGNAL_HANDLER] */
