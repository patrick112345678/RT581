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
/* PURPOSE: ZED Programmable Controlled Thermostat (PCT) device sample
*/

#define ZB_TRACE_FILE_ID 40018

#include "zboss_api.h"
#include "zb_led_button.h"

#include "../common/se_common.h"
#include "../common/se_indication.h"

#include "se/zb_se_pct_device.h"

/* Additional settings if the production config usage is disabled in the stack */
#ifdef ENABLE_RUNTIME_APP_CONFIG
#include "../common/se_cert.h"
#include "../common/se_ic.h"
static zb_ieee_addr_t g_dev_addr = PCT_DEV_ADDR;
#endif

/** [PCT_DEV_DEFINE_PARAMS] */
/* Used endpoint */
#define PCT_DEV_ENDPOINT 10

/* Channel masks for the device for the SubGHz mode and for the usual one */
#define PCT_DEV_CHANNEL_MASK1 (1L << 22)
#define PCT_DEV_CHANNEL_MASK2 (1L << 1)
#define PCT_DEV_CHANNEL_PAGE_1 ZB_CHANNEL_PAGE0_2_4_GHZ
#define PCT_DEV_CHANNEL_PAGE_2 ZB_CHANNEL_PAGE28_SUB_GHZ
/** [PCT_DEV_DEFINE_PARAMS] */

/* Send GetCurrentPrice command every minute. */
#define PCT_PRICE_UPDATE_PERIOD (60 * ZB_TIME_ONE_SECOND)

#define PCT_MESSAGE_UPDATE_PERIOD (60 * ZB_TIME_ONE_SECOND)

#define PCT_APP_DATA_PERSISTING_TIMEOUT 60 * ZB_TIME_ONE_SECOND

#define PCT_CMD_RETRY_TIMEOUT (5 * ZB_TIME_ONE_SECOND)

/** Enable/disable sending GetScheduledPrices command.
 * This command will be sent instead of every second GetCurrentPrices command.
 */
#define PCT_ENABLE_GET_SCHEDULED_PRICES 1


/**
 * Declaration of Zigbee device data structures
 */

/** @struct pct_dev_nvram_data_s
 *  @brief info about metering device stored in NVRAM
 */
typedef ZB_PACKED_PRE struct pct_dev_nvram_data_s
{
    zb_uint48_t curr_summ_delivered;          /**< CurrentSummationDelivered attribute value */

    zb_uint16_t addr;                         /**< device address */
    zb_uint8_t ep;                            /**< endpoint */

    zb_addr_u price_addr;                         /**< device address */
    zb_uint8_t price_ep;                            /**< endpoint */

    zb_addr_u drlc_addr;                         /**< device address */
    zb_uint8_t drlc_ep;                            /**< endpoint */

    zb_addr_u messaging_addr;
    zb_uint8_t messaging_ep;

    zb_addr_u mdu_addr;
    zb_uint8_t mdu_ep;
    zb_ieee_addr_t mdu_buf[10];
    zb_uint8_t device_type;                   /**< MeteringDeviceType attribute value */
    zb_uint8_t unit_of_measure;               /**< UnitOfMeasure attribute value */

    /* CurrentSummationDelivered parsed values */
    zb_uint8_t summ_fmt_left;                 /**< number of digits left to the decimal point */
    zb_uint8_t summ_fmt_right;                /**< number of digits right to the decimal point */
    zb_uint8_t summ_fmt_suppr;                /**< flags whether to suppress leading zeroes */

    zb_uint8_t status;                        /**< Status attribute value */

    zb_uint8_t align[1];
} ZB_PACKED_STRUCT pct_dev_nvram_data_t;

ZB_ASSERT_IF_NOT_ALIGNED_TO_4(pct_dev_nvram_data_t);

/** @struct pct_device_ctx_s
 *  @brief In-Home device context
 */
typedef struct pct_dev_ctx_s
{
    zb_bool_t first_measurement_received;

    /* thermostat device attributes */
    zb_zcl_basic_attrs_t basic_attrs;
    zb_zcl_kec_attrs_t kec_attrs;
    /* attributes of discovered metering device */
    pct_dev_nvram_data_t thermostat;
    zb_zcl_energy_management_attr_t energy_mgmt_attrs;
    zb_addr_u energy_mgmt_client_addr;
    zb_uint8_t energy_mgmt_client_ep;
} pct_dev_ctx_t;

/* Device context */
static pct_dev_ctx_t g_dev_ctx;


/** [DECLARE_CLUSTERS] */
/**
 * Declaring attributes for each cluster
 */

/* Basic cluster attributes */
ZB_ZCL_DECLARE_BASIC_ATTR_LIST(basic_attr_list, g_dev_ctx.basic_attrs);

/* Key Establishment cluster attributes */
ZB_ZCL_DECLARE_KEC_ATTR_LIST(kec_attr_list, g_dev_ctx.kec_attrs);

/* Energy Management cluster attributes */
ZB_ZCL_DECLARE_ENERGY_MANAGEMENT_ATTR_LIST(energy_mgmt_attr_list, g_dev_ctx.energy_mgmt_attrs);

/* Declare SE cluster list for the device */
ZB_SE_DECLARE_PCT_CLUSTER_LIST(pct_dev_clusters,
                               basic_attr_list,
                               kec_attr_list,
                               energy_mgmt_attr_list);

/* Declare endpoint */
ZB_SE_DECLARE_PCT_EP(pct_dev_ep, PCT_DEV_ENDPOINT, pct_dev_clusters);

/* Declare device context */
ZB_SE_DECLARE_PCT_CTX(pct_dev_zcl_ctx, pct_dev_ep);
/** [DECLARE_CLUSTERS] */

/**
 * Standard set of callbacks for application-specific NVRAM
 */
/* Reads dataset from application-specific NVRAM */
void pct_dev_read_nvram_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length);
/* Application callback called on NVRAM dataset writing operation */
zb_ret_t pct_dev_write_nvram_app_data(zb_uint8_t page, zb_uint32_t pos);
/* Returns application NVRAM dataset size */
zb_uint16_t pct_dev_get_nvram_data_size();

/* Application callback for incoming ZCL packets, called before ZCL packet processing by ZBOSS.
 * Depending on returned cmd_processed value ZBOSS decides if it is needed to do further processing of
 * the packet or not.
 * This callback may be used to implement application-specific logic of ZCL packets processing. */
static zb_uint8_t pct_dev_zcl_cmd_handler(zb_uint8_t param);
/* Initialize device global context */
static void pct_dev_ctx_init();
/* Initialize declared ZCL clusters attributes */
static void pct_dev_clusters_attrs_init(zb_uint8_t param);
/* Application init:
 * configure stack parameters (IEEE address, device role, channel mask etc),
 * register device context and ZCL cmd device callback),
 * init application attributes/context. */
static void pct_dev_app_init(zb_uint8_t param);
/* The callback being called on receive attribute report */
static void pct_dev_reporting_cb(zb_zcl_addr_t *addr, zb_uint8_t ep, zb_uint16_t cluster_id,
                                 zb_uint16_t attr_id, zb_uint8_t attr_type, zb_uint8_t *value);

void pct_dev_read_nvram_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length)
{
    zb_ret_t ret;

    TRACE_MSG(TRACE_APP1, ">> pct_dev_read_nvram_app_data: page %hd pos %d", (FMT__H_D, page, pos));

    ZB_ASSERT(payload_length == sizeof(g_dev_ctx.thermostat));

    /* If we fail, trace is given and assertion is triggered */
    ret = zb_nvram_read_data(page, pos, (zb_uint8_t *)&g_dev_ctx.thermostat, sizeof(g_dev_ctx.thermostat));

    if (ret == RET_OK)
    {
        TRACE_MSG(TRACE_APP1, "Loaded thermostat device info: addr %d, endpoint %d",
                  (FMT__D_D, g_dev_ctx.thermostat.addr, g_dev_ctx.thermostat.ep));
    }

    TRACE_MSG(TRACE_APP1, "<< pct_dev_read_nvram_app_data: ret %d", (FMT__D, ret));
}

zb_ret_t pct_dev_write_nvram_app_data(zb_uint8_t page, zb_uint32_t pos)
{
    zb_ret_t ret;

    TRACE_MSG(TRACE_APP1, ">> pct_dev_write_nvram_app_data: page %hd, pos %d", (FMT__H_D, page, pos));

    /* If we fail, trace is given and assertion is triggered */
    ret = zb_nvram_write_data(page, pos, (zb_uint8_t *)&g_dev_ctx.thermostat, sizeof(g_dev_ctx.thermostat));

    TRACE_MSG(TRACE_APP1, "<< pct_dev_write_nvram_app_data: ret %d", (FMT__D, ret));

    return ret;
}

zb_uint16_t pct_dev_get_nvram_data_size()
{
    TRACE_MSG(TRACE_APP1, "pct_dev_get_nvram_data_size: ret %hd", (FMT__H, sizeof(pct_dev_nvram_data_t)));
    return sizeof(pct_dev_nvram_data_t);
}

static void pct_dev_ctx_init()
{
    TRACE_MSG(TRACE_APP1, ">> pct_dev_ctx_init", (FMT__0));

    ZB_MEMSET(&g_dev_ctx, 0, sizeof(g_dev_ctx));

    TRACE_MSG(TRACE_APP1, "<< pct_dev_ctx_init", (FMT__0));
}

static void pct_dev_clusters_attrs_init(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> pct_dev_clusters_attrs_init", (FMT__0));
    ZVUNUSED(param);

    g_dev_ctx.basic_attrs.zcl_version = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
    g_dev_ctx.basic_attrs.power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

    g_dev_ctx.kec_attrs.kec_suite = ZB_KEC_SUPPORTED_CRYPTO_ATTR;

    g_dev_ctx.energy_mgmt_attrs = ZB_ZCL_DECLARE_ENERGY_MANAGEMENT_ATTR_LIST_INIT;

    TRACE_MSG(TRACE_APP1, "<< pct_dev_clusters_attrs_init", (FMT__0));
}

void pct_handle_display_msg(const zb_zcl_messaging_display_message_payload_t *pl)
{
    const zb_uint8_t *str = pl->message;
    zb_uint8_t i;

    TRACE_MSG(TRACE_APP1, "Got message: len = %hd", (FMT__H, ZB_ZCL_GET_STRING_LENGTH(str)));

    for (i = 8; i <= ZB_ZCL_GET_STRING_LENGTH(str); i += 8)
    {
        TRACE_MSG(TRACE_APP1, "Message content[%hd-%hd]", (FMT__H_H, i - 8, i));
        /* 2017/08/22 NK:MEDIUM Looks not very good)
         * I suggest to use function similar to DUMP_TRAF, which prints in char format instead of
         * hex. Maybe add format parameter to DUMP_TRAF - %hd/%xd/%c. */
        TRACE_MSG(TRACE_APP1, " = '%c%c%c%c%c%c%c%c'", (FMT__NC(8),
                  ZB_ZCL_GET_STRING_CHAR_AT(str, i - 8),
                  ZB_ZCL_GET_STRING_CHAR_AT(str, i - 7),
                  ZB_ZCL_GET_STRING_CHAR_AT(str, i - 6),
                  ZB_ZCL_GET_STRING_CHAR_AT(str, i - 5),
                  ZB_ZCL_GET_STRING_CHAR_AT(str, i - 4),
                  ZB_ZCL_GET_STRING_CHAR_AT(str, i - 3),
                  ZB_ZCL_GET_STRING_CHAR_AT(str, i - 2),
                  ZB_ZCL_GET_STRING_CHAR_AT(str, i - 1)));
    }

    for (i -= 8; i < ZB_ZCL_GET_STRING_LENGTH(str); i++)
    {
        TRACE_MSG(TRACE_APP1, "rest byte[i] = '%c'", (FMT__C, (ZB_ZCL_GET_STRING_CHAR_AT(str, i))));
    }
}

/* Handler for MDU diag. */
static void pct_handle_mdu_pairing_response( const zb_zcl_mdu_pairing_response_t *in )
{
    zb_uint8_t i = 0;
    zb_ieee_addr_t *eui64 = (zb_ieee_addr_t *)in->eui64;

    TRACE_MSG(TRACE_APP1, ">> handle_get_mdu_pairing_response(in=%p) <<", (FMT__P, in));
    TRACE_MSG(TRACE_APP1, "in->lpi_version = %d", (FMT__D, in->lpi_version));
    TRACE_MSG(TRACE_APP1, "in->total_number_of_devices = %hd", (FMT__H, in->total_number_of_devices));
    TRACE_MSG(TRACE_APP1, "in->command_index = %hd", (FMT__H, in->command_index));
    TRACE_MSG(TRACE_APP1, "in->total_number_of_commands = %hd", (FMT__H, in->total_number_of_commands));
    TRACE_MSG(TRACE_APP1, "in->num_dev_cmd = %hd (number of device EUI's passed in this command)", (FMT__H, in->num_dev_cmd));

    for (; i < in->num_dev_cmd; i++)
    {
        TRACE_MSG(TRACE_APP1, "in->eui64[%hd] = " TRACE_FORMAT_64, (FMT__H_A, i, TRACE_ARG_64(*eui64)));
        eui64++;
    }

}

/** [handle_manage_event] */
/* Handles received Energy Management Cluster's Mange Event command
 * Prints received values to trace */
static zb_ret_t handle_manage_event(const zb_zcl_energy_management_manage_event_payload_t *in,
                                    const zb_zcl_parsed_hdr_t *in_cmd_info,
                                    zb_zcl_energy_management_report_event_status_payload_t *out)
{
    TRACE_MSG(TRACE_APP1, ">> handle_manage_event(in=%p) <<", (FMT__P, in));
    TRACE_MSG(TRACE_APP1, "in->issuer_event_id = %08x", (FMT__D, in->issuer_event_id));
    TRACE_MSG(TRACE_APP1, "in->device_class = %02x", (FMT__D, in->device_class));
    TRACE_MSG(TRACE_APP1, "in->utility_enrollment_group = %02x", (FMT__H, in->utility_enrollment_group));
    TRACE_MSG(TRACE_APP1, "in->actions_required = %02x", (FMT__H, in->actions_required));

    if ((in->actions_required) & (1 << ZB_ZCL_ENERGY_MANAGEMENT_ACTIONS_DISABLE_DUTY_CYCLING))
    {
        TRACE_MSG(TRACE_APP1, "Got ZB_ZCL_ENERGY_MANAGEMENT_ACTIONS_DISABLE_DUTY_CYCLING action from 0x%x",
                  (FMT__D, ZB_ZCL_PARSED_HDR_SHORT_DATA(in_cmd_info).source.u.short_addr));
    }

    if ((in->actions_required) & (1 << ZB_ZCL_ENERGY_MANAGEMENT_ACTIONS_ENABLE_DUTY_CYCLING))
    {
        TRACE_MSG(TRACE_APP1, "Got ZB_ZCL_ENERGY_MANAGEMENT_ACTIONS_ENABLE_DUTY_CYCLING action from 0x%x",
                  (FMT__D, ZB_ZCL_PARSED_HDR_SHORT_DATA(in_cmd_info).source.u.short_addr));
    }

    if ((in->actions_required) & (1 << ZB_ZCL_ENERGY_MANAGEMENT_ACTIONS_EVENT_OPT_OUT))
    {
        TRACE_MSG(TRACE_APP1, "Got ZB_ZCL_ENERGY_MANAGEMENT_ACTIONS_EVENT_OPT_OUT action from 0x%x",
                  (FMT__D, ZB_ZCL_PARSED_HDR_SHORT_DATA(in_cmd_info).source.u.short_addr));
    }

    if ((in->actions_required) & (1 << ZB_ZCL_ENERGY_MANAGEMENT_ACTIONS_OPT_INTO_EVENT))
    {
        TRACE_MSG(TRACE_APP1, "Got ZB_ZCL_ENERGY_MANAGEMENT_ACTIONS_OPT_INTO_EVENT action from 0x%x",
                  (FMT__D, ZB_ZCL_PARSED_HDR_SHORT_DATA(in_cmd_info).source.u.short_addr));
    }

    /** If the Manage Event command is valid, a Report Event Status command will be
     * returned regardless of whether any bits of
     * zb_zcl_energy_management_manage_event_payload_t->actions_required are set.
     */
    /*Here we must get events if present and pass them to client*/
    out->issuer_event_id = in->issuer_event_id;                             /* (M) */
    out->event_status = ZB_ZCL_DRLC_EVENT_REJECTED_UNDEFINED_EVENT;  /* (M) */
    out->event_status_time = zb_get_utc_time();                             /* (M) */
    out->criticality_level_applied = 0x22;                                  /* (M) */
    out->event_control = 0xbc;                                              /* (M) */

    TRACE_MSG(TRACE_APP1, "<< handle_manage_event", (FMT__0));
    return RET_OK;
}
/** [handle_manage_event] */


/* Application device callback for ZCL commands. Used for:
 *  - receiving application-specific values for commands (e.g. ZB_ZCL_DRLC_LOAD_CONTROL_EVENT_CB_ID)
 *  - providing received ZCL commands data to application (e.g. ZB_ZCL_PRICE_PUBLISH_PRICE_CB_ID)
 *  Application may ignore callback id-s in which it is not interested. */
static void pct_zcl_cmd_device_cb(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> pct_zcl_cmd_device_cb param %hd id %d", (FMT__H_D,
              param, ZB_ZCL_DEVICE_CMD_PARAM_CB_ID(param)));

    switch (ZB_ZCL_DEVICE_CMD_PARAM_CB_ID(param))
    {
    case ZB_ZCL_MDU_PAIRING_RESPONSE_CB_ID:
    {
        TRACE_MSG(TRACE_APP1, "ZB_ZCL_MDU_PAIRING_RESPONSE_CB_ID", (FMT__0));
        pct_handle_mdu_pairing_response(
            ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_mdu_pairing_response_t));
        ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
    }
    break;
    case ZB_ZCL_MESSAGING_DISPLAY_MSG_CB_ID:
    {
        TRACE_MSG(TRACE_APP1, "ZB_ZCL_MESSAGING_DISPLAY_MSG_CB_ID", (FMT__0));
        pct_handle_display_msg(ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_messaging_display_message_payload_t));
        ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
    }
    break;
    case ZB_ZCL_ENERGY_MANAGEMENT_MANAGE_EVENT_CB_ID:
    {
        TRACE_MSG(TRACE_APP1, "ZB_ZCL_ENERGY_MANAGEMENT_MANAGE_EVENT_CB_ID", (FMT__0));
        handle_manage_event(ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param,
                            zb_zcl_energy_management_manage_event_payload_t),
                            ZB_ZCL_DEVICE_CMD_PARAM_CMD_INFO(param),
                            ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(param,
                                    zb_zcl_energy_management_report_event_status_payload_t));
        ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) = RET_OK;
    }
    break;
    default:
        TRACE_MSG(TRACE_APP1, "Undefined callback was called, id=%d",
                  (FMT__D, ZB_ZCL_DEVICE_CMD_PARAM_CB_ID(param)));
        break;
    }
    TRACE_MSG(TRACE_APP1, ">> pct_zcl_cmd_device_cb param", (FMT__0));
}

/** [PCT_DEV_INIT] */
static void pct_dev_app_init(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> pct_dev_app_init", (FMT__0));
    /** [PCT_DEV_INIT] */
    ZVUNUSED(param);

    /* Inits buttons if there are any
     * Not valid for running on Network Simulator */
    zb_osif_led_button_init();

    /* Register callbacks for application-specific NVRAM */
    zb_nvram_register_app1_read_cb(pct_dev_read_nvram_app_data);
    zb_nvram_register_app1_write_cb(pct_dev_write_nvram_app_data, pct_dev_get_nvram_data_size);

    /* Reset NVRAM when both buttons are pressed
     * When running on the Network Simulator - both functions always return false */
    if (zb_osif_button_state(BUTTON_LEFT) && zb_osif_button_state(BUTTON_RIGHT))
    {
        zb_se_start_nvram_erase_indication();
        zb_set_nvram_erase_at_start(ZB_TRUE);
        zb_se_stop_nvram_erase_indication();
    }

    /** [REGISTER_DEVICE_CTX] */
    /* Register device ZCL context */
    ZB_AF_REGISTER_DEVICE_CTX(&pct_dev_zcl_ctx);
    /* Register cluster commands handler for a specific endpoint */
    ZB_AF_SET_ENDPOINT_HANDLER(PCT_DEV_ENDPOINT, pct_dev_zcl_cmd_handler);
    /** [REGISTER_DEVICE_CTX] */

    /* Initialization of global device context */
    pct_dev_ctx_init();
    /* Initialization of declared ZCL clusters attributes */
    pct_dev_clusters_attrs_init(0);
    /* Sets the Device user application callback */
    ZB_ZCL_REGISTER_DEVICE_CB(pct_zcl_cmd_device_cb);
    /* Sets the callback being called on receive attribute report */
    ZB_ZCL_SET_REPORT_ATTR_CB(pct_dev_reporting_cb);

    /* Set up device's role in a Zigbee network and used channel */
#ifdef ENABLE_RUNTIME_APP_CONFIG
    zb_set_long_address(g_dev_addr);
#endif
#if defined ZB_SUBGHZ_BAND_ENABLED
    {
        zb_channel_list_t channel_list;
        zb_channel_list_init(channel_list);
        zb_channel_list_add(channel_list, PCT_DEV_CHANNEL_PAGE_1, PCT_DEV_CHANNEL_MASK1);
        zb_channel_list_add(channel_list, PCT_DEV_CHANNEL_PAGE_2, PCT_DEV_CHANNEL_MASK2);

        TRACE_MSG(TRACE_APP1, "ZED in MM mode start: page %d mask 0x%x page %d mask 0x%x",
                  (FMT__D_D_D_D, PCT_DEV_CHANNEL_PAGE_1, PCT_DEV_CHANNEL_MASK1, PCT_DEV_CHANNEL_PAGE_2, PCT_DEV_CHANNEL_MASK2));
        zb_se_set_network_ed_role_select_device(channel_list);
    }
#else
    /** [PCT_DEV_SET_ROLE] */
    zb_se_set_network_ed_role(PCT_DEV_CHANNEL_MASK1);
    /** [PCT_DEV_SET_ROLE] */
#endif

    /* Define that device will act as Sleepy End Device */
    zb_set_rx_on_when_idle(ZB_FALSE);

    TRACE_MSG(TRACE_APP1, "<< pct_dev_app_init", (FMT__0));
}

static void pct_dev_reporting_cb(zb_zcl_addr_t *addr, zb_uint8_t ep, zb_uint16_t cluster_id,
                                 zb_uint16_t attr_id, zb_uint8_t attr_type, zb_uint8_t *value)
{
    //  zb_uint16_t dev_short_addr = ZB_UNKNOWN_SHORT_ADDR;
    ZVUNUSED(attr_type);
    ZVUNUSED(addr);
    ZVUNUSED(ep);
    ZVUNUSED(cluster_id);
    ZVUNUSED(attr_id);
    ZVUNUSED(value);

    TRACE_MSG(TRACE_APP1, ">> pct_dev_reporting_cb", (FMT__0));


    TRACE_MSG(TRACE_APP1, "<< pct_dev_reporting_cb", (FMT__0));
}

MAIN()
{
    ARGV_UNUSED;

    /* Enable trace */
    ZB_SET_TRACE_ON();
    /* Disable traffic dump */
    ZB_SET_TRAF_DUMP_OFF();

    /* Global ZBOSS initialization */
    ZB_INIT("thermostat_device");

    /* Application initialization */
    pct_dev_app_init(0);

    /* Initiate the stack start without starting the commissioning */
    if (zboss_start_no_autostart() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zboss_start_no_autostart() failed", (FMT__0));
    }
    else
    {
        /* Call the main loop */
        zboss_main_loop();
    }

    /* Deinitialize trace */
    TRACE_DEINIT();

    MAIN_RETURN(0);
}


/** [COMMAND_HANDLER] */
zb_uint8_t pct_dev_zcl_cmd_handler(zb_uint8_t param)
{
    zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);
    zb_uint8_t cmd_processed = ZB_FALSE;
    ZVUNUSED(cmd_info);

    TRACE_MSG(TRACE_ZCL1, "> pct_dev_zcl_cmd_handler", (FMT__0));

    TRACE_MSG(TRACE_ZCL1, "< pct_dev_zcl_cmd_handler %hd", (FMT__H, cmd_processed));
    return cmd_processed;
}
/** [COMMAND_HANDLER] */

/** [AUTO_JOIN] */
/* Left button handler: start/stop join. */
static void pct_dev_left_button_handler(zb_uint8_t param)
{
    if (!param)
    {
        /* Button is pressed, get buffer for outgoing command */
        zb_buf_get_out_delayed(pct_dev_left_button_handler);
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
            zb_buf_free(param);
        }
    }
}
/** [AUTO_JOIN] */


/** [SIGNAL_HANDLER] */
/** [SIGNAL_HANDLER_GET_SIGNAL] */
/*  Application signal handler. Used for informing application about important ZBOSS
 *  events/states (device started first time/rebooted, key establishment completed etc).
 *  Application may ignore signals in which it is not interested.
 */
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
#ifdef ENABLE_RUNTIME_APP_CONFIG
            zb_secur_ic_str_set(pct_installcode);
#ifdef SE_CRYPTOSUITE_1
            zb_se_load_ecc_cert(KEC_CS1, ca_public_key_cs1, pct_certificate_cs1, pct_private_key_cs1);
#endif
#ifdef SE_CRYPTOSUITE_2
            zb_se_load_ecc_cert(KEC_CS2, ca_public_key_cs2, pct_certificate_cs2, pct_private_key_cs2);
#endif
#endif /* ENABLE_RUNTIME_APP_CONFIG */
            zboss_start_continue();
            break;

        case ZB_SE_SIGNAL_SKIP_JOIN:
            /* wait button click to start commissioning */
#ifdef ZB_USE_BUTTONS
            zb_button_register_handler(BUTTON_LEFT, 0, pct_dev_left_button_handler);
#else
            pct_dev_left_button_handler(0);
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
            break;
        /** [SIGNAL_HANDLER_CBKE_OK] */

        /** [SIGNAL_HANDLER_START_DISCOVERY] */
        case ZB_SE_SIGNAL_SERVICE_DISCOVERY_START:
            TRACE_MSG(TRACE_APP1, "Start Service Discovery", (FMT__0));
            zb_se_service_discovery_start(PCT_DEV_ENDPOINT);
            zb_se_indicate_service_discovery_started();
            break;
        /** [SIGNAL_HANDLER_START_DISCOVERY] */

        /** [SIGNAL_HANDLER_DO_BIND] */
        case ZB_SE_SIGNAL_SERVICE_DISCOVERY_DO_BIND:
        {
            zb_se_signal_service_discovery_bind_params_t *bind_params =
                ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_se_signal_service_discovery_bind_params_t);

            TRACE_MSG(TRACE_APP1, "can bind cluster %d commodity_type %d remote_dev " TRACE_FORMAT_64,
                      (FMT__D_D_A, bind_params->cluster_id, bind_params->commodity_type,
                       TRACE_ARG_64(bind_params->device_addr)));

            if (bind_params->cluster_id == ZB_ZCL_CLUSTER_ID_PREPAYMENT)
            {
                zb_se_service_discovery_bind_req(param, bind_params->device_addr, bind_params->endpoint);
                param = 0;
            }
        }
            /** [SIGNAL_HANDLER_DO_BIND] */

        break;
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
            zb_se_indicate_commissioning_fail(0,  /* erase nvram */
                                              0); /* reboot */
            break;
        /** [SIGNAL_HANDLER_DISCOVERY_FAILED] */

        /** [SIGNAL_HANDLER_BIND_INDICATION] */
        case ZB_SE_SIGNAL_SERVICE_DISCOVERY_BIND_INDICATION:
        {
            zb_se_signal_service_discovery_bind_params_t *bind_params =
                ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_se_signal_service_discovery_bind_params_t);

            TRACE_MSG(TRACE_APP1, "Bind indication signal: binded cluster 0x%x endpoint %d device " TRACE_FORMAT_64,
                      (FMT__D_D_A, bind_params->cluster_id, bind_params->endpoint, TRACE_ARG_64(bind_params->device_addr)));
            /** [SIGNAL_HANDLER_BIND_INDICATION] */

            if (bind_params->cluster_id == ZB_ZCL_CLUSTER_ID_ENERGY_MANAGEMENT)
            {
                ZB_IEEE_ADDR_COPY(g_dev_ctx.energy_mgmt_client_addr.addr_long, bind_params->device_addr);
                g_dev_ctx.energy_mgmt_client_ep = bind_params->endpoint;

                TRACE_MSG(TRACE_APP1, "Energy Management device bound to out service: remote addr " TRACE_FORMAT_64 " ep %hd",
                          (FMT__A_D, TRACE_ARG_64(g_dev_ctx.energy_mgmt_client_addr.addr_long),
                           g_dev_ctx.energy_mgmt_client_ep));
            }
        }
        break;
        /**[SIGNAL_APS_KEY_READY]  */
        case ZB_SE_SIGNAL_APS_KEY_READY:
        {
            zb_ieee_addr_t *remote_device_addr = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_ieee_addr_t);

            TRACE_MSG(TRACE_APP1, "Partner link key initiated, remote device addr: " TRACE_FORMAT_64,
                      (FMT__A, TRACE_ARG_64(remote_device_addr)));

            /**[SIGNAL_APS_KEY_READY]  */

            /**[SIGNAL_APS_KEY_READY_DEV]  */
        }
        break;
        /**[SIGNAL_APS_KEY_READY_DEV]  */

        case ZB_COMMON_SIGNAL_CAN_SLEEP:
            /* skip*/
            break;

        default:
            break;
        }
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
        zb_se_indicate_commissioning_fail(0,  /* erase nvram */
                                          0); /* reboot */
    }

    if (param)
    {
        zb_buf_free(param);
    }

}
/** [SIGNAL_HANDLER] */
