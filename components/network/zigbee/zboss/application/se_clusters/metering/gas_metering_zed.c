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
/* PURPOSE: Gas metering ZED device sample
*/

#define ZB_TRACE_FILE_ID 40006

#include "zboss_api.h"
#include "zb_metering_load_control.h"

#define GAS_METERING_DEV_ADDR {0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22}
static zb_ieee_addr_t g_dev_addr = GAS_METERING_DEV_ADDR;

#define MLC_PHYSICAL_DEV_ENDPOINT 1
#define MLC_METERING_DEV_ENDPOINT 2
#define MLC_LOAD_CONTROL_DEV_ENDPOINT 3

#define METERING_DEV_CHANNEL_MASK (1L << 22)

/* SummationFormatting: XXXXXX.Y, do not suppress leading zeroes */
#define METERING_DEV_SUMM_FMT_LEFT 8
#define METERING_DEV_SUMM_FMT_RIGHT 3
#define METERING_DEV_SUMM_FMT_SUPPR 0
#define METERING_DEV_SUMMATION_FORMATTING \
  ZB_ZCL_METERING_FORMATTING_SET(METERING_DEV_SUMM_FMT_SUPPR, METERING_DEV_SUMM_FMT_LEFT, METERING_DEV_SUMM_FMT_RIGHT)
#define METERING_DEV_MEASURE_TIMEOUT ZB_TIME_ONE_SECOND
#define METERING_DEV_APP_DATA_PERSISTING_TIMEOUT 60 * ZB_TIME_ONE_SECOND


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

typedef struct mlc_dev_identify_attrs_s
{
    zb_uint16_t identify_time;
} mlc_dev_identify_attrs_t;

/** @struct metering_dev_ctx_s
 *  @brief Metering device context
 */
typedef struct metering_dev_ctx_s
{
    zb_bool_t first_measurement_done;

    /* metering device attributes */
    zb_zcl_basic_attrs_t basic_attrs;            /**< Basic cluster attributes */
    zb_zcl_metering_attrs_t metering_attrs;      /**< Metering cluster attributes */
    zb_zcl_drlc_client_attrs_t drlc_attrs;   /**< DRLC cluster client attributes */
    mlc_dev_identify_attrs_t identify_attrs;
} metering_dev_ctx_t;

/* device context */
static metering_dev_ctx_t g_dev_ctx;

void metering_dev_ctx_init();
void metering_dev_clusters_attrs_init(zb_uint8_t param);
void metering_dev_default_reporting_init(zb_uint8_t param);
void metering_dev_app_init(zb_uint8_t param);

static void metering_dev_measure(zb_uint8_t param);


/** [COMMON_DECLARATION] */

/*********************  Clusters' attributes  **************************/

/* Basic cluster attributes */
ZB_ZCL_DECLARE_BASIC_ATTR_LIST(basic_attr_list, g_dev_ctx.basic_attrs);

/* Identify cluster attributes */
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_dev_ctx.identify_attrs.identify_time);

/* Metering cluster attributes */
ZB_ZCL_DECLARE_METERING_ATTR_LIST(metering_attr_list, g_dev_ctx.metering_attrs);

/* DRLC cluster attributes */
ZB_ZCL_DECLARE_DRLC_ATTR_LIST(drlc_attr_list, g_dev_ctx.drlc_attrs);

/*********************  Device declaration  **************************/


ZB_DECLARE_MLC_PHYSICAL_DEVICE_CLUSTER_LIST(physical_dev_clusters,
        basic_attr_list);
ZB_DECLARE_MLC_PHYSICAL_DEVICE_EP(physical_dev_ep, MLC_PHYSICAL_DEV_ENDPOINT,
                                  physical_dev_clusters);

ZB_DECLARE_MLC_METERING_DEVICE_CLUSTER_LIST(metering_dev_clusters,
        metering_attr_list,
        identify_attr_list);
ZB_DECLARE_MLC_METERING_DEVICE_EP(metering_dev_ep, MLC_METERING_DEV_ENDPOINT,
                                  metering_dev_clusters);

ZB_DECLARE_MLC_LOAD_CONTROL_DEVICE_CLUSTER_LIST(load_control_dev_clusters,
        drlc_attr_list);
ZB_DECLARE_MLC_LOAD_CONTROL_DEVICE_EP(load_control_dev_ep, MLC_LOAD_CONTROL_DEV_ENDPOINT,
                                      load_control_dev_clusters);

ZBOSS_DECLARE_DEVICE_CTX_3_EP(metering_dev_zcl_ctx, physical_dev_ep, metering_dev_ep, load_control_dev_ep);

/** [COMMON_DECLARATION] */


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
        if (g_dev_ctx.metering_attrs.status & ZB_ZCL_METERING_GAS_CHECK_METER)
        {
            TRACE_MSG(TRACE_APP1, "   - check meter", (FMT__0));
        }
        if (g_dev_ctx.metering_attrs.status & ZB_ZCL_METERING_GAS_LOW_BATTERY)
        {
            TRACE_MSG(TRACE_APP1, "   - low battery", (FMT__0));
        }
        if (g_dev_ctx.metering_attrs.status & ZB_ZCL_METERING_GAS_TAMPER_DETECT)
        {
            TRACE_MSG(TRACE_APP1, "   - tamper detect", (FMT__0));
        }
        if (g_dev_ctx.metering_attrs.status & ZB_ZCL_METERING_GAS_LOW_PRESSURE)
        {
            TRACE_MSG(TRACE_APP1, "   - low pressure", (FMT__0));
        }
        if (g_dev_ctx.metering_attrs.status & ZB_ZCL_METERING_GAS_LEAK_DETECT)
        {
            TRACE_MSG(TRACE_APP1, "   - leak detect", (FMT__0));
        }
        if (g_dev_ctx.metering_attrs.status & ZB_ZCL_METERING_GAS_SERVICE_DISCONNECT)
        {
            TRACE_MSG(TRACE_APP1, "   - service disconnect", (FMT__0));
        }
        if (g_dev_ctx.metering_attrs.status & ZB_ZCL_METERING_GAS_REVERSE_FLOW)
        {
            TRACE_MSG(TRACE_APP1, "   - reverse flow", (FMT__0));
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
    /* if we fail, trace is given and assertion is triggered */
    (void)zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);
    ZB_SCHEDULE_APP_ALARM(metering_dev_save_data, 0, METERING_DEV_APP_DATA_PERSISTING_TIMEOUT);
}

/** Read Metering device's stored data from NVRAM. */
void metering_dev_read_nvram_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length)
{
    metering_dev_nvram_data_t data;
    zb_ret_t ret;

    TRACE_MSG(TRACE_APP1, ">> metering_dev_read_nvram_app_data: page %hd pos %d", (FMT__H_D, page, pos));

    ZB_ASSERT(payload_length == sizeof(data));

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

    g_dev_ctx.metering_attrs.status = ZB_ZCL_METERING_STATUS_DEFAULT_VALUE;
    g_dev_ctx.metering_attrs.unit_of_measure = ZB_ZCL_METERING_UNIT_M3_M3H_BINARY;
    g_dev_ctx.metering_attrs.summation_formatting = METERING_DEV_SUMMATION_FORMATTING;
    g_dev_ctx.metering_attrs.device_type = ZB_ZCL_METERING_GAS_METERING;

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
    rep_info.ep = MLC_METERING_DEV_ENDPOINT;
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

/** Application init: configure stack parameters (IEEE address, device role, channel mask etc),
 * register device context and ZCL cmd device callback), init application attributes/context. */
void metering_dev_app_init(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> metering_dev_app_init", (FMT__0));
    ZVUNUSED(param);

    /** [REGISTER] */
    zb_nvram_register_app1_read_cb(metering_dev_read_nvram_app_data);
    zb_nvram_register_app1_write_cb(metering_dev_write_nvram_app_data, metering_dev_get_nvram_data_size);

    ZB_AF_REGISTER_DEVICE_CTX(&metering_dev_zcl_ctx);
    /** [REGISTER] */

    /* device configuration */
    metering_dev_ctx_init();
    metering_dev_clusters_attrs_init(0);

    /* ZB configuration */
    zb_set_long_address(g_dev_addr);

    zb_set_network_ed_role(METERING_DEV_CHANNEL_MASK);

    /* Act as Sleepy End Device */
    zb_set_rx_on_when_idle(ZB_FALSE);

    TRACE_MSG(TRACE_APP1, "<< metering_dev_app_init", (FMT__0));
}


/** Generate and accumulate measurements - emulate real device. */
static void metering_dev_measure(zb_uint8_t param)
{
    zb_uint48_t instantaneous_consumption;
    zb_uint32_t measure;
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

    measure = (zb_uint32_t)ZB_RANDOM_VALUE(7);
    zb_uint64_to_uint48((zb_uint64_t)measure, &instantaneous_consumption);
    new_curr_summ_delivered = g_dev_ctx.metering_attrs.curr_summ_delivered;

    zb_uint48_add(&new_curr_summ_delivered, &instantaneous_consumption, &new_curr_summ_delivered);

    ZB_ZCL_SET_ATTRIBUTE(MLC_METERING_DEV_ENDPOINT,
                         ZB_ZCL_CLUSTER_ID_METERING,
                         ZB_ZCL_CLUSTER_SERVER_ROLE,
                         ZB_ZCL_ATTR_METERING_CURRENT_SUMMATION_DELIVERED_ID,
                         (zb_uint8_t *)&new_curr_summ_delivered,
                         ZB_FALSE);

    new_metering_status = (1L << measure);

    ZB_ZCL_SET_ATTRIBUTE(MLC_METERING_DEV_ENDPOINT,
                         ZB_ZCL_CLUSTER_ID_METERING,
                         ZB_ZCL_CLUSTER_SERVER_ROLE,
                         ZB_ZCL_ATTR_METERING_STATUS_ID,
                         (zb_uint8_t *)&new_metering_status,
                         ZB_FALSE);

    TRACE_MSG(TRACE_APP1, "instantaneous gas consumption: %ld %ld",
              (FMT__L_L, instantaneous_consumption.high, instantaneous_consumption.low));

    metering_dev_show_attr(ZB_ZCL_ATTR_METERING_CURRENT_SUMMATION_DELIVERED_ID);
    metering_dev_show_attr(ZB_ZCL_ATTR_METERING_STATUS_ID);

    if (g_dev_ctx.first_measurement_done == ZB_FALSE)
    {
        ZB_SCHEDULE_APP_CALLBACK(metering_dev_save_data, 0);
        g_dev_ctx.first_measurement_done = ZB_TRUE;
    }

    ZB_SCHEDULE_APP_ALARM(metering_dev_measure, 0, METERING_DEV_MEASURE_TIMEOUT);
}


/*********************  SE Gas Metering ZED  **************************/

MAIN()
{
    ARGV_UNUSED;

    ZB_SET_TRACE_ON();
    ZB_SET_TRAF_DUMP_OFF();

    ZB_INIT("gas_device");

    metering_dev_app_init(0);

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

static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster)
{
    TRACE_MSG(TRACE_ZCL1, "finding_binding_cb status %d addr " TRACE_FORMAT_64 " ep %hd cluster %d",
              (FMT__D_A_H_D, status, TRACE_ARG_64(addr), ep, cluster));

    return ZB_TRUE;
}

void mlc_finding_binding_initiator_metering(zb_uint8_t param)
{
    ZVUNUSED(param);
    zb_bdb_finding_binding_initiator(MLC_METERING_DEV_ENDPOINT, finding_binding_cb);
}

void mlc_finding_binding_target_metering(zb_uint8_t param)
{
    ZVUNUSED(param);
    zb_bdb_finding_binding_target(MLC_METERING_DEV_ENDPOINT);
}

/** Application signal handler. Used for informing applica
tion about important ZBOSS
    events/states (device started first time/rebooted, key establishment completed etc).
    Application may ignore signals in which it is not interested.
*/
void zboss_signal_handler(zb_uint8_t param)
{
    zb_zdo_app_signal_hdr_t *sg_p = NULL;
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_APP1, "Device STARTED OK: first_start %hd",
                      (FMT__H, sig == ZB_BDB_SIGNAL_DEVICE_FIRST_START));
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
            metering_dev_default_reporting_init(0);
            metering_dev_measure(0);
            break;

        case ZB_BDB_SIGNAL_STEERING:
            TRACE_MSG(TRACE_APP1, "Successfull steering, start f&b initiator", (FMT__0));
            ZB_SCHEDULE_APP_ALARM(mlc_finding_binding_initiator_metering, 0, 10 * ZB_TIME_ONE_SECOND);
            ZB_SCHEDULE_APP_ALARM(mlc_finding_binding_initiator_metering, 0, (ZB_BDBC_MIN_COMMISSIONING_TIME_S + 15) * ZB_TIME_ONE_SECOND);
            ZB_SCHEDULE_APP_ALARM(mlc_finding_binding_target_metering, 0, (ZB_BDBC_MIN_COMMISSIONING_TIME_S * 2) * ZB_TIME_ONE_SECOND);
            break;

        case ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED:
            TRACE_MSG(TRACE_APP1, "Finding&binding target done", (FMT__0));
            break;

        case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:
            TRACE_MSG(TRACE_APP1, "Finding&binding initiator done", (FMT__0));
            break;

        default:
            TRACE_MSG(TRACE_APP1, "zboss_signal_handler: skip sig %hd status %hd",
                      (FMT__H_H, sig, ZB_GET_APP_SIGNAL_STATUS(param)));
            break;
        }
    }
    else
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY:
            TRACE_MSG(TRACE_ERROR, "Production confing is not ready...", (FMT__0));
            break;
        default:
            TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
            break;
        }
    }

    if (param)
    {
        zb_buf_free(param);
    }

}
