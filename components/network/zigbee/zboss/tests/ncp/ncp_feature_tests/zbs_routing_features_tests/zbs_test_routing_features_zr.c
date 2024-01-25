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

#define ZB_TRACE_FILE_ID 40032

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

/* Enable this define to start as BDB device */
/* #define ZBS_TEST_ROUTING_FEATURES_ZR_START_IN_BDB_MODE */

/* This defines are enabled in case production config is disabled */
#ifdef ENABLE_RUNTIME_APP_CONFIG
static zb_ieee_addr_t g_dev_addr = ZBS_TEST_ROUTING_FEATURES_ZR_DEV_ADDR;
#endif

const zb_uint16_t g_invisible_short_addr = 0x0002;

/** [ZBS_TEST_ROUTING_FEATURES_ZR_DEV_DEFINE_PARAMS] */
/* Current endpoint */
#define ZBS_TEST_ROUTING_FEATURES_ZR_DEV_ENDPOINT 10
/* Used channel */
#define ZBS_TEST_ROUTING_FEATURES_ZR_DEV_CHANNEL_MASK (1L << 19)
/** [ZBS_TEST_ROUTING_FEATURES_ZR_DEV_DEFINE_PARAMS] */

#define ZBS_TEST_ROUTING_FEATURES_ZR_APP_DATA_PERSISTING_TIMEOUT 50 * ZB_TIME_ONE_SECOND
#define ZBS_TEST_ROUTING_FEATURES_ZR_DEV_ENT_NUMBER 10

/**
 * Declaration of Zigbee device data structures
 */
typedef ZB_PACKED_PRE struct zbs_test_routing_features_zr_dev_ent_s
{
    zb_uint8_t used;
    zb_uint16_t cluster_id;
    zb_ieee_addr_t dev_addr;                        /**< device address */
    zb_uint8_t dev_ep;                          /**< endpoint */
    zb_uint8_t aps_key_established;
    zb_uint8_t align[3]; /* how much*/
} ZB_PACKED_STRUCT zbs_test_routing_features_zr_dev_ent_t;

/** @struct zbs_test_routing_features_zr_dev_nvram_data_s
 *  @brief info about metering device stored in NVRAM
 */
typedef ZB_PACKED_PRE struct zbs_test_routing_features_zr_dev_nvram_data_s
{
    zbs_test_routing_features_zr_dev_ent_t lst[ZBS_TEST_ROUTING_FEATURES_ZR_DEV_ENT_NUMBER];
} ZB_PACKED_STRUCT zbs_test_routing_features_zr_dev_nvram_data_t;

ZB_ASSERT_IF_NOT_ALIGNED_TO_4(zbs_test_routing_features_zr_dev_nvram_data_t);
ZB_ASSERT_IF_NOT_ALIGNED_TO_4(zbs_test_routing_features_zr_dev_ent_t);

/** @struct zbs_test_routing_features_zr_device_ctx_s
 *  @brief In-Home device context
 */
typedef struct zbs_test_routing_features_zr_dev_ctx_s
{
    zb_zcl_basic_attrs_t basic_attrs;        /**< Basic cluster attributes  */
    zb_zcl_kec_attrs_t kec_attrs;            /**< Key Establishment cluster attributes */
    /* attributes of discovered devices */
    zbs_test_routing_features_zr_dev_nvram_data_t dev;
    zb_uint32_t network_time;
} zbs_test_routing_features_zr_dev_ctx_t;


/* Device context */
static zbs_test_routing_features_zr_dev_ctx_t g_dev_ctx;

/** [DECLARE_CLUSTERS] */
/**
 * Declaring attributes for each cluster
 */

/* Basic cluster attributes */
ZB_ZCL_DECLARE_BASIC_ATTR_LIST(basic_attr_list, g_dev_ctx.basic_attrs);

/* Key Establishment cluster attributes */
ZB_ZCL_DECLARE_KEC_ATTR_LIST(kec_attr_list, g_dev_ctx.kec_attrs);
ZB_SE_DECLARE_CBKE_ZR_DEV_CLUSTER_LIST(zbs_test_routing_features_zr_dev_clusters,
                                       basic_attr_list,
                                       kec_attr_list);
/* Declare endpoint */
ZB_SE_DECLARE_CBKE_ZR_DEV_EP(zbs_test_routing_features_zr_dev_ep, ZBS_TEST_ROUTING_FEATURES_ZR_DEV_ENDPOINT, zbs_test_routing_features_zr_dev_clusters);

/* Declare SE application's device context for single-endpoint device */
ZB_SE_DECLARE_CBKE_ZR_DEV_CTX(zbs_test_routing_features_zr_dev_zcl_ctx, zbs_test_routing_features_zr_dev_ep);
/** [DECLARE_CLUSTERS] */

static void test_partner_lk(zb_uint8_t param);

/* Saves received from another devices data into NVRAM */
static void zbs_test_routing_features_zr_dev_save_data(zb_uint8_t param);

/**
 * Standard set of callbacks for application-specific NVRAM
 */
/* Reads dataset from application-specific NVRAM */
void zbs_test_routing_features_zr_dev_read_nvram_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length);
/* Application callback called on NVRAM dataset writing operation */
zb_ret_t zbs_test_routing_features_zr_dev_write_nvram_app_data(zb_uint8_t page, zb_uint32_t pos);
/* Returns application NVRAM dataset size */
zb_uint16_t zbs_test_routing_features_zr_dev_get_nvram_data_size();

/* Handler for Discover Attributes Response command */
void zbs_test_routing_features_zr_dev_disc_attr_resp_handler(zb_bufid_t cmd_buf, zb_zcl_parsed_hdr_t *cmd_info);

/** Application callback for incoming ZCL packets, called before ZCL packet processing by ZBOSS.
 * Depending on returned cmd_processed value ZBOSS decides if it is needed to do further processing of
 * the packet or not.
 * This callback may be used to implement application-specific logic of ZCL packets processing.
 */
static zb_uint8_t zbs_test_routing_features_zr_dev_zcl_cmd_handler(zb_uint8_t param);
/* Initialize device global context */
static void zbs_test_routing_features_zr_dev_ctx_init();
/* Initialize declared ZCL clusters attributes */
static void zbs_test_routing_features_zr_dev_clusters_attrs_init(zb_uint8_t param);
/* Application init:
 * configure stack parameters (IEEE address, device role, channel mask etc),
 * register device context and ZCL cmd device callback),
 * init application attributes/context. */
static void zbs_test_routing_features_zr_dev_app_init(zb_uint8_t param);

/* Handles "APS key established" event - forces pending commands if device is found in device list. */
static void zbs_test_routing_features_zr_dev_aps_key_established(zb_ieee_addr_t addr)
{
    zb_uint8_t i = 0;

    TRACE_MSG(TRACE_APP1, "zbs_test_routing_features_zr_dev_aps_key_established: addr " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(addr)));

    while (i < ZB_ARRAY_SIZE(g_dev_ctx.dev.lst))
    {
        if (g_dev_ctx.dev.lst[i].used &&
                ZB_IEEE_ADDR_CMP(g_dev_ctx.dev.lst[i].dev_addr, addr))
        {
        }
        ++i;
    }
}

static void zbs_test_routing_features_zr_dev_save_data(zb_uint8_t param)
{
    ZVUNUSED(param);
    zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);
    ZB_SCHEDULE_APP_ALARM(zbs_test_routing_features_zr_dev_save_data, 0, ZBS_TEST_ROUTING_FEATURES_ZR_APP_DATA_PERSISTING_TIMEOUT);
}

void zbs_test_routing_features_zr_dev_read_nvram_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length)
{
    zb_ret_t ret;

    TRACE_MSG(TRACE_APP1, ">> zbs_test_routing_features_zr_dev_read_nvram_app_data: page %hd pos %d", (FMT__H_D, page, pos));

    ZB_ASSERT(payload_length == sizeof(zbs_test_routing_features_zr_dev_nvram_data_t));

    ret = zb_osif_nvram_read(page, pos, (zb_uint8_t *)&g_dev_ctx.dev, sizeof(zbs_test_routing_features_zr_dev_nvram_data_t));

    TRACE_MSG(TRACE_APP1, "<< zbs_test_routing_features_zr_dev_read_nvram_app_data: ret %d", (FMT__D, ret));
}

zb_ret_t zbs_test_routing_features_zr_dev_write_nvram_app_data(zb_uint8_t page, zb_uint32_t pos)
{
    zb_ret_t ret;

    TRACE_MSG(TRACE_APP1, ">> zbs_test_routing_features_zr_dev_write_nvram_app_data: page %hd, pos %d", (FMT__H_D, page, pos));

    ret = zb_osif_nvram_write(page, pos, (zb_uint8_t *)&g_dev_ctx.dev, sizeof(zbs_test_routing_features_zr_dev_nvram_data_t));

    TRACE_MSG(TRACE_APP1, "<< zbs_test_routing_features_zr_dev_write_nvram_app_data: ret %d", (FMT__D, ret));

    return ret;
}

zb_uint16_t zbs_test_routing_features_zr_dev_get_nvram_data_size()
{
    TRACE_MSG(TRACE_APP1, "zbs_test_routing_features_zr_dev_get_nvram_data_size: ret %hd", (FMT__H, sizeof(zbs_test_routing_features_zr_dev_nvram_data_t)));
    return sizeof(zbs_test_routing_features_zr_dev_nvram_data_t);
}

static void zbs_test_routing_features_zr_dev_ctx_init()
{
    TRACE_MSG(TRACE_APP1, ">> zbs_test_routing_features_zr_dev_ctx_init", (FMT__0));

    ZB_MEMSET(&g_dev_ctx, 0, sizeof(g_dev_ctx));

    g_dev_ctx.network_time = ZB_ZCL_TIME_TIME_INVALID_VALUE;

    TRACE_MSG(TRACE_APP1, "<< zbs_test_routing_features_zr_dev_ctx_init", (FMT__0));
}

static void zbs_test_routing_features_zr_dev_clusters_attrs_init(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> zbs_test_routing_features_zr_dev_clusters_attrs_init", (FMT__0));
    ZVUNUSED(param);

    g_dev_ctx.basic_attrs.zcl_version = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
    g_dev_ctx.basic_attrs.power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

    g_dev_ctx.kec_attrs.kec_suite = ZB_KEC_SUPPORTED_CRYPTO_ATTR;
    TRACE_MSG(TRACE_APP1, "<< zbs_test_routing_features_zr_dev_clusters_attrs_init", (FMT__0));
}

/** Application device callback for ZCL commands. Used for:
    - receiving application-specific values for commands (e.g. ZB_ZCL_DRLC_LOAD_CONTROL_EVENT_CB_ID)
    - providing received ZCL commands data to application (e.g. ZB_ZCL_PRICE_PUBLISH_PRICE_CB_ID)
    Application may ignore callback id-s in which it is not interested.
 */
static void zbs_test_routing_features_zr_zcl_cmd_device_cb(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> zbs_test_routing_features_zr_zcl_cmd_device_cb param %hd id %d", (FMT__H_D,
              param, ZB_ZCL_DEVICE_CMD_PARAM_CB_ID(param)));

    switch (ZB_ZCL_DEVICE_CMD_PARAM_CB_ID(param))
    {
    default:
        TRACE_MSG(TRACE_APP1, "Undefined callback was called, id=%d",
                  (FMT__D, ZB_ZCL_DEVICE_CMD_PARAM_CB_ID(param)));
        break;
    }
    TRACE_MSG(TRACE_APP1, ">> zbs_test_routing_features_zr_zcl_cmd_device_cb param", (FMT__0));
}

zb_uint16_t addr_assignment(zb_ieee_addr_t ieee_addr);

/** [ZBS_TEST_ROUTING_FEATURES_ZR_DEV_INIT] */
static void zbs_test_routing_features_zr_dev_app_init(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> zbs_test_routing_features_zr_dev_app_init", (FMT__0));
    /** [ZBS_TEST_ROUTING_FEATURES_ZR_DEV_INIT] */
    ZVUNUSED(param);

    /* Inits buttons if there are any
    * Not valid for running on Network Simulator */
    zb_osif_led_button_init();

    /* Register callbacks for application-specific NVRAM */
    zb_nvram_register_app1_read_cb(zbs_test_routing_features_zr_dev_read_nvram_app_data);
    zb_nvram_register_app1_write_cb(zbs_test_routing_features_zr_dev_write_nvram_app_data, zbs_test_routing_features_zr_dev_get_nvram_data_size);

    /* Reset NVRAM when both buttons are pressed
     * When running on the Network Simulator - both functions always return false */
    /* disable buttons usage for automatic testing */
    /* reset nvram every time when device starts */
    {
        zb_set_nvram_erase_at_start(ZB_TRUE);
    }

    /** [REGISTER_DEVICE_CTX] */
    /* Register device ZCL context */
    ZB_AF_REGISTER_DEVICE_CTX(&zbs_test_routing_features_zr_dev_zcl_ctx);
    /* Register cluster commands handler for a specific endpoint */
    ZB_AF_SET_ENDPOINT_HANDLER(ZBS_TEST_ROUTING_FEATURES_ZR_DEV_ENDPOINT, zbs_test_routing_features_zr_dev_zcl_cmd_handler);
    /** [REGISTER_DEVICE_CTX] */

    /* Initialization of global device context */
    zbs_test_routing_features_zr_dev_ctx_init();
    /* Initialization of declared ZCL clusters attributes */
    zbs_test_routing_features_zr_dev_clusters_attrs_init(0);
    /* Sets the Device user application callback */
    ZB_ZCL_REGISTER_DEVICE_CB(zbs_test_routing_features_zr_zcl_cmd_device_cb);
    /* ZB configuration for the case, when production configuration is disabled */
#ifdef ENABLE_RUNTIME_APP_CONFIG
    zb_set_long_address(g_dev_addr);
#endif

    /** [ZBS_TEST_ROUTING_FEATURES_ZR_DEV_SET_ROLE] */
    /* Set up device role in a Zigbee network */
    zb_se_set_network_router_role(ZBS_TEST_ROUTING_FEATURES_ZR_DEV_CHANNEL_MASK);
    /** [ZBS_TEST_ROUTING_FEATURES_ZR_DEV_SET_ROLE] */

#ifndef ZBS_TEST_ROUTING_FEATURES_ZR_DEV_SUPPORT_MULTIPLE_COMMODITY
    zb_se_service_discovery_set_multiple_commodity_enabled(0);
#endif

    zb_set_max_children(1);
    zb_nwk_set_address_assignment_cb(addr_assignment);

    TRACE_MSG(TRACE_APP1, "<< zbs_test_routing_features_zr_dev_app_init", (FMT__0));
}

zb_uint16_t addr_assignment(zb_ieee_addr_t ieee_addr)
{
    return g_invisible_short_addr;
}

MAIN()
{
    ARGV_UNUSED;

    /* Enable trace */
    ZB_SET_TRACE_ON();
    /* Disable traffic dump */
    ZB_SET_TRAF_DUMP_ON();

    /* Global ZBOSS initialization */
#ifdef ZBS_TEST_ROUTING_FEATURES_ZR_START_IN_BDB_MODE
    ZB_INIT("zbs_test_routing_features_zr_bdb");
#else
    ZB_INIT("zbs_test_routing_features_zr");
#endif

    zb_set_nvram_erase_at_start(ZB_TRUE);
    /* Application initialization */
    zbs_test_routing_features_zr_dev_app_init(0);

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
zb_uint8_t zbs_test_routing_features_zr_dev_zcl_cmd_handler(zb_uint8_t param)
{
    zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);
    zb_uint8_t cmd_processed = ZB_FALSE;

    TRACE_MSG(TRACE_ZCL1, "> zbs_test_routing_features_zr_dev_zcl_cmd_handler param %hd", (FMT__H, param));
    TRACE_MSG(TRACE_ZCL2, "f type %hd cluster_id 0x%x cmd_id %hd",
              (FMT__H_D_H, cmd_info->is_common_command, cmd_info->cluster_id, cmd_info->cmd_id));
    TRACE_MSG(TRACE_ZCL1, "< zbs_test_routing_features_zr_dev_zcl_cmd_handler %hd", (FMT__H, cmd_processed));
    return cmd_processed;
}

/** [COMMAND_HANDLER] */

/** [AUTO_JOIN] */
/* Left button handler: start/stop join. */
static void zbs_test_routing_features_zr_dev_left_button_handler(zb_uint8_t param)
{
    if (!param)
    {
        /* Button is pressed, get buffer for outgoing command */
        zb_buf_get_out_delayed(zbs_test_routing_features_zr_dev_left_button_handler);
    }
    else
    {
        if (zb_se_auto_join_start(param) == RET_OK)
        {
            TRACE_MSG(TRACE_APP1, "start auto_join", (FMT__0));
        }
        else
        {
            zb_se_auto_join_stop();
            TRACE_MSG(TRACE_APP1, "stop auto_join", (FMT__0));
            zb_buf_free(param);
        }
    }
}


/** [AUTO_JOIN] */
/** [SIGNAL_HANDLER] */
/** [SIGNAL_HANDLER_GET_SIGNAL] */
/* Application signal handler. Used for informing application about important ZBOSS
 * events/states (device started first time/rebooted, key establishment completed etc).
 * Application may ignore signals in which it is not interested. */
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
            zb_secur_ic_str_set(zbs_test_routing_features_zr_installcode);
#ifdef SE_CRYPTOSUITE_1
            zb_se_load_ecc_cert(KEC_CS1, ca_public_key_cs1, zbs_test_routing_features_zr_certificate_cs1, zbs_test_routing_features_zr_private_key_cs1);
#endif
#ifdef SE_CRYPTOSUITE_2
            zb_se_load_ecc_cert(KEC_CS2, ca_public_key_cs2, zbs_test_routing_features_zr_certificate_cs2, zbs_test_routing_features_zr_private_key_cs2);
#endif
#endif /* ENABLE_RUNTIME_APP_CONFIG */
            zboss_start_continue();
            break;

        case ZB_SE_SIGNAL_SKIP_JOIN:
            /* wait button click to start commissioning */
            /* disable buttons usage for automatic testing */
            /* since USE_BUTTONS is defined by default for ti13xx will use its handler directly */
            zbs_test_routing_features_zr_dev_left_button_handler(0);
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
            break;

        /** [SIGNAL_HANDLER_CBKE_OK] */
        case ZB_SE_SIGNAL_CBKE_OK:
            TRACE_MSG(TRACE_APP1, "Key Establishment with Trust Center (CBKE) OK", (FMT__0));
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
            break;
        /** [SIGNAL_HANDLER_CBKE_OK] */

        /** [SIGNAL_HANDLER_START_DISCOVERY] */
        case ZB_SE_SIGNAL_SERVICE_DISCOVERY_START:
            TRACE_MSG(TRACE_APP1, "Start Service Discovery", (FMT__0));
            zb_se_service_discovery_start(ZBS_TEST_ROUTING_FEATURES_ZR_DEV_ENDPOINT);
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

            break;
        /** [SIGNAL_HANDLER_DISCOVERY_OK] */

        /** [SIGNAL_HANDLER_DISCOVERY_FAILED] */
        case ZB_SE_SIGNAL_SERVICE_DISCOVERY_FAILED:
            TRACE_MSG(TRACE_APP1, "Service Discovery failed", (FMT__0));
            break;
        /** [SIGNAL_HANDLER_DISCOVERY_FAILED] */

        /**[SIGNAL_APS_KEY_READY]  */
        case ZB_SE_SIGNAL_APS_KEY_READY:
            /* NOTE: Place here Energy Management cluster call, because during handling
             * SIGNAL_HANDLER_DO_BIND for Energy Management cluster ZBS_TEST_ROUTING_FEATURES_ZR don't
             * receive Transport key between itself and Energy Management cluster
             */
        {
            static int n_sig = 0;
            zb_ieee_addr_t *remote_device_addr = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_ieee_addr_t);

            TRACE_MSG(TRACE_APP1, "Partner link key established, remote device addr: " TRACE_FORMAT_64,
                      (FMT__A, TRACE_ARG_64(*remote_device_addr)));

            /**[SIGNAL_APS_KEY_READY]  */

            /**[SIGNAL_APS_KEY_READY_DEV]  */
            zbs_test_routing_features_zr_dev_aps_key_established(*remote_device_addr);
            if (n_sig == 0)
            {
                /* To test both directions, initiate same procedure from our side */
                n_sig++;
                ZB_SCHEDULE_APP_CALLBACK(test_partner_lk, 0);
            }
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
        /* if (ZB_GET_APP_SIGNAL_STATUS(param) == (zb_uint16_t)RET_CONNECTION_LOST) */
        if (sig != ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
        {
            /* TODO: reboot */
        }
    }

    if (param)
    {
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_APP1, "<< zboss_signal_handler", (FMT__0));
}
/** [SIGNAL_HANDLER] */



/* When testing with NCP test, we have no clusters implemented, so
 * can't discover a Host thus will not establish partner LK
 * normally. So do it manually. */

#define PEER_DEV_ADDR {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11}

static void start_partner_lk(zb_uint8_t param);

static void test_partner_lk(zb_uint8_t param)
{
    if (param == 0)
    {
        zb_buf_get_out_delayed(test_partner_lk);
    }
    else
    {
        zb_ieee_addr_t zbs_test_routing_features_zr_addr = PEER_DEV_ADDR;
        zb_zdo_nwk_addr_req_param_t *req = zb_buf_get_tail(param, sizeof(zb_zdo_nwk_addr_req_param_t));

        req->dst_addr = ZB_NWK_BROADCAST_ALL_DEVICES;
        ZB_IEEE_ADDR_COPY(req->ieee_addr, zbs_test_routing_features_zr_addr);
        req->start_index = 0;
        req->request_type = 0;

        TRACE_MSG(TRACE_APP2, "bulb_nwk_addr_req: param %hd ieee "TRACE_FORMAT_64, (FMT__H_A, param, TRACE_ARG_64(req->ieee_addr)));
        zb_zdo_nwk_addr_req(param, start_partner_lk);
    }
}


static void start_partner_lk(zb_uint8_t param)
{
    zb_zdo_nwk_addr_resp_head_t *resp = (zb_zdo_nwk_addr_resp_head_t *)zb_buf_begin(param);
    zb_uint16_t nwk_addr;

    TRACE_MSG(TRACE_APP2, "nwk addr resp status %hd, nwk addr %d", (FMT__H_D, resp->status, resp->nwk_addr));

    if (resp->status == ZB_ZDP_STATUS_SUCCESS)
    {
        ZB_LETOH16(&nwk_addr, &resp->nwk_addr);
        zb_se_start_aps_key_establishment(param, nwk_addr);
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Failed to get nwk addr of ZBS_TEST_ROUTING_FEATURES_ZR", (FMT__0));
        zb_buf_free(param);
    }
}
