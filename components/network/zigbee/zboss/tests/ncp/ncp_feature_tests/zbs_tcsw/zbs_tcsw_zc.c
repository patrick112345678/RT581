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
/* PURPOSE: TC Swap-out ZC device sample
*/

#define ZB_TRACE_FILE_ID 99

#include "zboss_api.h"
#include "zb_config.h"

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_mac.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zboss_tcswap.h"
#include "zb_secur.h"

#include "zbs_tcsw.h"

#ifndef TC_SWAPOUT
#warning TC_SWAPOUT is not defined. The purpose of this ZC is to test the feature "indicate TC swap-out" on NCP
#endif
#ifndef ZB_CERTIFICATION_HACKS
#warning ZB_CERTIFICATION_HACKS is not defined. ZC won`t work correctly
#endif

/** [ZBS_TCSW_ZC_DEV_DEFINE_PARAMS] */
#define ZBS_TCSW_ZC_DEV_ENDPOINT 1
#define ZBS_TCSW_ZC_DEV_CHANNEL_MASK (1L << 19) /* 19 channel. */

#define ZBS_TCSW_ZC_PERMIT_JOINING_DURATION_TIME 240
/** [ZBS_TCSW_ZC_DEV_DEFINE_PARAMS] */
#define ZBS_TCSW_ZC_KEEP_ALIVE_BASE 60  /* in minutes */
#define ZBS_TCSW_ZC_KEEP_ALIVE_JITTER 5 /* in minutes */
#define ZBS_TCSW_ZC_TIME_STATUS_INITIAL ((1 << ZB_ZCL_TIME_MASTER) | (1 << ZB_ZCL_TIME_MASTER_ZONE_DST) | (1 << ZB_ZCL_TIME_SUPERSEDING))

#define ZBS_TCSW_ZC_LOAD_CONTROL_EVENT_GROUP_TIMEOUT 120 * ZB_TIME_ONE_SECOND
#define ZBS_TCSW_ZC_LOAD_CONTROL_EVENT_TIMEOUT 5 * ZB_TIME_ONE_SECOND

#define ZBS_TCSW_PEER1_DEV_ADDR_INTERNAL          {0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb}
#define ZBS_TCSW_PEER2_DEV_ADDR_INTERNAL          {0x12, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a}
#define ZBS_TCSW_PEER3_DEV_ADDR_INTERNAL          {0x13, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a}

#define NCP_INSTALL_CODE               "966b9f3ef98ae605 9708"

static char ncp_installcode[] = NCP_INSTALL_CODE;

static zb_uint8_t gs_nwk_key[16] = {0x11, 0xaa, 0x22, 0xbb, 0x33, 0xcc, 0x44, 0xdd,
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                                   };

static zb_ieee_addr_t zc_tc_swapped_addr = ESI_DEV_SWAPPED_ADDR;

#ifndef ZB_TEST_DATA_SIZE
#define ZB_TEST_DATA_SIZE 10U
#endif

#define ZBS_TCSW_NVRAM_DATASET_SIZE 1024

static zb_uint8_t g_nvram_buf[ZBS_TCSW_NVRAM_DATASET_SIZE] = {0};
static zb_bool_t  tc_db_loaded = ZB_FALSE;

/** [DECLARE_CLUSTERS] */
/*********************  Clusters' attributes  **************************/

/** @struct zbs_tcsw_zc_dev_ctx_s
 *  @brief ZBS_TCSW_ZC device context
 */
typedef struct zbs_tcsw_zc_dev_ctx_s
{
    zb_zcl_basic_attrs_t basic_attrs;
    zb_zcl_kec_attrs_t kec_attrs;
    zb_zcl_time_attrs_t time_attrs;
} zbs_tcsw_zc_dev_ctx_t;

/* device context */
static zbs_tcsw_zc_dev_ctx_t g_dev_ctx;

/* Basic cluster attributes */
ZB_ZCL_DECLARE_BASIC_ATTR_LIST(basic_attr_list, g_dev_ctx.basic_attrs);

/* Key Establishment cluster attributes */
ZB_ZCL_DECLARE_KEC_ATTR_LIST(kec_attr_list, g_dev_ctx.kec_attrs);

/* Time cluster attributes */
ZB_ZCL_DECLARE_TIME_ATTR_LIST(time_attr_list, g_dev_ctx.time_attrs);
ZB_SE_DECLARE_CBKE_ZC_DEV_CLUSTER_LIST(zbs_tcsw_zc_dev_clusters,
                                       basic_attr_list,
                                       time_attr_list,
                                       kec_attr_list
                                      );

ZB_SE_DECLARE_CBKE_ZC_DEV_EP(zbs_tcsw_zc_dev_ep, ZBS_TCSW_ZC_DEV_ENDPOINT, zbs_tcsw_zc_dev_clusters);

ZB_SE_DECLARE_CBKE_ZC_DEV_CTX(zbs_tcsw_zc_dev_zcl_ctx, zbs_tcsw_zc_dev_ep);

/** [DECLARE_CLUSTERS] */

void zbs_tcsw_zc_dev_ctx_init();
void zbs_tcsw_zc_dev_clusters_attrs_init(zb_uint8_t param);
void zbs_tcsw_zc_dev_app_init(zb_uint8_t param);
static zb_uint8_t data_indication(zb_uint8_t param);
static void zc_send_data(zb_uint8_t param);
static zb_uint_t g_packets_sent;
static zb_uint16_t g_remote_addr;

/*********************  Device-specific functions  **************************/

/** Init device context. */
void zbs_tcsw_zc_dev_ctx_init()
{
    TRACE_MSG(TRACE_APP1, ">> zbs_tcsw_zc_dev_ctx_init", (FMT__0));

    ZB_MEMSET(&g_dev_ctx, 0, sizeof(g_dev_ctx));
    TRACE_MSG(TRACE_APP1, "<< zbs_tcsw_zc_dev_ctx_init", (FMT__0));
}

/** Init device ZCL attributes. */
void zbs_tcsw_zc_dev_clusters_attrs_init(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> zbs_tcsw_zc_dev_clusters_attrs_init", (FMT__0));
    ZVUNUSED(param);

    g_dev_ctx.basic_attrs.zcl_version = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
    g_dev_ctx.basic_attrs.power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

    g_dev_ctx.kec_attrs.kec_suite = ZB_KEC_SUPPORTED_CRYPTO_ATTR;

    g_dev_ctx.time_attrs.time = zb_get_utc_time();  /* We assume: that this time is trusted */
    g_dev_ctx.time_attrs.time_status = ZBS_TCSW_ZC_TIME_STATUS_INITIAL;
    g_dev_ctx.time_attrs.time_zone = ZB_ZCL_TIME_TIME_ZONE_DEFAULT_VALUE;
    g_dev_ctx.time_attrs.dst_start = ZB_ZCL_TIME_TIME_INVALID_VALUE;
    g_dev_ctx.time_attrs.dst_end = ZB_ZCL_TIME_TIME_INVALID_VALUE;
    g_dev_ctx.time_attrs.dst_shift = ZB_ZCL_TIME_DST_SHIFT_DEFAULT_VALUE;
    g_dev_ctx.time_attrs.standard_time = ZB_ZCL_TIME_TIME_INVALID_VALUE;
    g_dev_ctx.time_attrs.local_time = ZB_ZCL_TIME_TIME_INVALID_VALUE;
    g_dev_ctx.time_attrs.last_set_time = ZB_ZCL_TIME_LAST_SET_TIME_DEFAULT_VALUE;
    g_dev_ctx.time_attrs.valid_until_time = ZB_ZCL_TIME_VALID_UNTIL_TIME_DEFAULT_VALUE;

    TRACE_MSG(TRACE_APP1, "<< zbs_tcsw_zc_dev_clusters_attrs_init", (FMT__0));
}

static void zb_time_to_die(zb_uint8_t param)
{
    ZVUNUSED(param);

    TRACE_MSG(TRACE_APP1, "TIME TO DIE", (FMT__0));

    /* Note that this call keeps app1 datased with TC swap DB saved there */
    zb_nvram_clear();

    ZB_SCHEDULE_CALLBACK(zb_reset, 0);
}

/** Application callback for ZCL commands. Used for:
    - receiving application-specific values for commands (e.g. ZB_ZCL_PRICE_GET_CURRENT_PRICE_CB_ID)
    - providing received ZCL commands data to application (e.g. ZB_ZCL_DRLC_REPORT_EVENT_STATUS_CB_ID)
    Application may ignore callback id-s in which it is not interested.
 */
static void zbs_tcsw_zc_zcl_cmd_device_cb(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> zbs_tcsw_zc_zcl_cmd_device_cb(param=%hd, id=%d)",
              (FMT__H_D, param, ZB_ZCL_DEVICE_CMD_PARAM_CB_ID(param)));

    switch (ZB_ZCL_DEVICE_CMD_PARAM_CB_ID(param))
    {
    default:
        TRACE_MSG(TRACE_APP1, "Undefined callback was called, id=%d",
                  (FMT__D, ZB_ZCL_DEVICE_CMD_PARAM_CB_ID(param)));
        break;
    }

    TRACE_MSG(TRACE_APP1, "<< zbs_tcsw_zc_zcl_cmd_device_cb", (FMT__0));
}

static zb_ret_t zbs_tcsw_nvram_write_app_data(zb_uint8_t page, zb_uint32_t pos)
{
    zb_ret_t ret;

    TRACE_MSG(TRACE_APP1, "+zbs_tcsw_nvram_write_app_data page %hu pos %u", (FMT__H_D, page, pos));

    ret = zb_nvram_write_data(page, pos, g_nvram_buf, ZBS_TCSW_NVRAM_DATASET_SIZE);

    TRACE_MSG(TRACE_APP1, "-zbs_tcsw_nvram_write_app_data, ret %hd", (FMT__H, ret));

    return ret;
}

static void zbs_tcsw_nvram_read_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length)
{
    zb_ret_t ret;

    ZB_ASSERT(payload_length == ZBS_TCSW_NVRAM_DATASET_SIZE);

    ret = zb_nvram_read_data(page, pos, g_nvram_buf, ZBS_TCSW_NVRAM_DATASET_SIZE);

    TRACE_MSG(TRACE_APP1, "zbs_tcsw_nvram_read_app_data ret %d page %hu pos %u", (FMT__D_H_D, ret, page, pos));
    tc_db_loaded = ZB_TRUE;
}

static zb_uint16_t zbs_tcsw_get_nvram_data_size(void)
{
    TRACE_MSG(TRACE_APP1, "zbs_tcsw_get_nvram_data_size", (FMT__0));
    return ZBS_TCSW_NVRAM_DATASET_SIZE;
}

/* Application init: configure stack parameters (IEEE address, device role, channel mask etc),
 * register device context and ZCL cmd device callback), init application attributes/context. */
void zbs_tcsw_zc_dev_app_init(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> zbs_tcsw_zc_dev_app_init", (FMT__0));
    ZVUNUSED(param);

    ZB_AF_REGISTER_DEVICE_CTX(&zbs_tcsw_zc_dev_zcl_ctx);

    zb_set_nvram_erase_at_start(ZB_FALSE);

    /* device configuration */
    zbs_tcsw_zc_dev_ctx_init(); /* clear g_dev_ctx */
    zbs_tcsw_zc_dev_clusters_attrs_init(0);
    ZB_ZCL_REGISTER_DEVICE_CB(zbs_tcsw_zc_zcl_cmd_device_cb);

    zb_secur_setup_nwk_key(gs_nwk_key, 0);

    zb_se_set_network_coordinator_role(ZBS_TCSW_ZC_DEV_CHANNEL_MASK);

    zb_nvram_register_app1_read_cb(zbs_tcsw_nvram_read_app_data);
    zb_nvram_register_app1_write_cb(zbs_tcsw_nvram_write_app_data, zbs_tcsw_get_nvram_data_size);

    zb_set_max_children(1);

    TRACE_MSG(TRACE_APP1, "<< zbs_tcsw_zc_dev_app_init", (FMT__0));
}

MAIN()
{
    ARGV_UNUSED;

    ZB_SET_TRACE_ON();
    ZB_SET_TRAF_DUMP_ON();

    ZB_INIT("zbs_tcsw_zc");

    zb_set_pan_id(0x5043);

    zbs_tcsw_zc_dev_app_init(0);

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

static void l_nvram_write_dataset(zb_uint8_t param)
{
    zb_ret_t ret = zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);
    TRACE_MSG(TRACE_APP1, "zb_nvram_write_dataset() ret", (FMT__D, ret));
    ZVUNUSED(param);
}

static zb_ieee_addr_t peers_addr[] =
{
    ZBS_TCSW_PEER1_DEV_ADDR_INTERNAL,
    ZBS_TCSW_PEER2_DEV_ADDR_INTERNAL,
    ZBS_TCSW_PEER3_DEV_ADDR_INTERNAL
};

static void load_ic(void)
{
    zb_uindex_t i;

    for (i = 0; i < ZB_ARRAY_SIZE(peers_addr); i++)
    {
        zb_uint16_t keypair_i = zb_aps_keypair_get_index_by_addr(peers_addr[i], ZB_SECUR_PROVISIONAL_KEY);
        if (keypair_i == (zb_uint16_t) -1)
        {
            zb_secur_ic_str_add(peers_addr[i], ncp_installcode, NULL);
        }
    }
}

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
        case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
            TRACE_MSG(TRACE_APP1, "  ZB_ZDO_SIGNAL_DEVICE_ANNCE", (FMT__0));
            {
                zb_zdo_signal_device_annce_params_t *dev_annce_params;

                dev_annce_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_annce_params_t);
                g_remote_addr = dev_annce_params->device_short_addr;
                if (tc_db_loaded == ZB_FALSE)
                {
                    ZB_SCHEDULE_ALARM(zb_time_to_die, 0, 15 * ZB_TIME_ONE_SECOND);
                }
            }
            break;

        case ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY:
            TRACE_MSG(TRACE_APP1, "  ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY buf_len %u", (FMT__D, zb_buf_len(param)));
            break;

        case ZB_ZDO_SIGNAL_SKIP_STARTUP:
            TRACE_MSG(TRACE_APP1, "  ZB_ZDO_SIGNAL_SKIP_STARTUP tc_db_loaded %hu", (FMT__H, tc_db_loaded));

            if (tc_db_loaded)
            {
            }

            zboss_start_continue();
            break;

        case ZB_SIGNAL_DEVICE_FIRST_START:
        case ZB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_APP1, "Device STARTED OK: first_start %hd tc_db_loaded %hd",
                      (FMT__H_H, sig == ZB_SIGNAL_DEVICE_FIRST_START, tc_db_loaded));
            if (tc_db_loaded)
            {
#if defined TC_SWAPOUT && defined ZB_COORDINATOR_ROLE
                zb_ret_t ret = zb_tcsw_start_restore_db(g_nvram_buf, ZBS_TCSW_NVRAM_DATASET_SIZE, ZB_FALSE);
                TRACE_MSG(TRACE_APP1, "RESTORE TC DB, ret %hd", (FMT__H, ret));
#ifdef ZB_CERTIFICATION_HACKS
                ZG->cert_hacks.certificate_priority_from_certdb = 1;
#endif
#endif /* defined TC_SWAPOUT && defined ZB_COORDINATOR_ROLE*/
                zb_set_long_address(zc_tc_swapped_addr);
                TRACE_MSG(TRACE_APP1, "* Set new address " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(zc_tc_swapped_addr)));
#ifdef SE_CRYPTOSUITE_2
                zb_se_load_ecc_cert(KEC_CS2, ca_public_key_cs2, zbs_tcsw_zc_certificate_cs2, zbs_tcsw_zc_private_key_cs2);
#endif
            }

            load_ic(); /* Installcodes loaded only if no provisional key for device */

            zb_se_permit_joining(ZBS_TCSW_ZC_PERMIT_JOINING_DURATION_TIME);

            zb_af_set_data_indication(data_indication); /* Provide echo function */
            break;

        case ZB_SE_SIGNAL_SERVICE_DISCOVERY_BIND_INDICATION:
            TRACE_MSG(TRACE_APP1, "  ZB_SE_SIGNAL_SERVICE_DISCOVERY_BIND_INDICATION", (FMT__0));
            {
                zb_se_signal_service_discovery_bind_params_t *bind_params =
                    ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_se_signal_service_discovery_bind_params_t);

                TRACE_MSG(TRACE_APP1, "Bind indication signal: binded cluster 0x%x endpoint %d device " TRACE_FORMAT_64,
                          (FMT__D_D_A, bind_params->cluster_id, bind_params->endpoint, TRACE_ARG_64(bind_params->device_addr)));
            }
            break;
        case ZB_SE_TC_SIGNAL_CHILD_JOIN_CBKE:
            TRACE_MSG(TRACE_APP1, "  ZB_SE_TC_SIGNAL_CHILD_JOIN_CBKE", (FMT__0));
            {
                zb_uint8_t *remote_device_addr = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_uint8_t);

#ifdef DEBUG_EXPOSE_KEYS
                {
                    zb_uint8_t key[ZB_CCM_KEY_SIZE];
                    if (zb_se_debug_get_link_key_by_long(remote_device_addr, key) == RET_OK)
                    {
                        TRACE_MSG(TRACE_APP1, "Child " TRACE_FORMAT_64 " joined & established TCLK " TRACE_FORMAT_128,
                                  (FMT__A_B, TRACE_ARG_64(remote_device_addr), TRACE_ARG_128(key)));

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
#else
                TRACE_MSG(TRACE_APP1, "Child " TRACE_FORMAT_64 " joined & established TCLK",
                          (FMT__A, TRACE_ARG_64(remote_device_addr)));
#endif
                ZB_SCHEDULE_ALARM(zb_secur_trace_all_key_pairs, 0, 2 * ZB_TIME_ONE_SECOND);
            }
            break;
        case ZB_SE_TC_SIGNAL_CHILD_JOIN_NON_CBKE:
            TRACE_MSG(TRACE_APP1, "  ZB_SE_TC_SIGNAL_CHILD_JOIN_NON_CBKE", (FMT__0));
            {
                zb_ieee_addr_t *remote_device_addr = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_ieee_addr_t);
                TRACE_MSG(TRACE_APP1, "Child " TRACE_FORMAT_64 " joined & established non-CBKE TCLK",
                          (FMT__A, TRACE_ARG_64(remote_device_addr)));
            }
            break;
        case ZB_ZDO_SIGNAL_LEAVE_INDICATION:
            TRACE_MSG(TRACE_APP1, "  ZB_ZDO_SIGNAL_LEAVE_INDICATION", (FMT__0));
            {
                zb_zdo_signal_leave_indication_params_t *leave_ind_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_leave_indication_params_t);
                TRACE_MSG(TRACE_APP1, "leave indication, device " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(leave_ind_params->device_addr)));
            }
            break;

#if defined TC_SWAPOUT && defined ZB_COORDINATOR_ROLE
        case ZB_TCSWAP_DB_BACKUP_REQUIRED_SIGNAL:
            TRACE_MSG(TRACE_APP1, "  ZB_TCSWAP_DB_BACKUP_REQUIRED_SIGNAL", (FMT__0));
            {
                zb_ret_t ret = zb_tcsw_start_backup_db(g_nvram_buf, ZBS_TCSW_NVRAM_DATASET_SIZE);
                TRACE_MSG(TRACE_APP1, "BACKUP TC DB, ret %hd", (FMT__H, ret));
                ZB_SCHEDULE_CALLBACK(l_nvram_write_dataset, 0);
            }
            break;
#endif /* defined TC_SWAPOUT && defined ZB_COORDINATOR_ROLE */

        case ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED:
            TRACE_MSG(TRACE_APP1, "  ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED", (FMT__0));
            break;

        case ZB_ZDO_SIGNAL_DEVICE_UPDATE:
            TRACE_MSG(TRACE_APP1, "  ZB_ZDO_SIGNAL_DEVICE_UPDATE", (FMT__0));
            break;

        default:
            TRACE_MSG(TRACE_ERROR, "  SKIP SIGNAL %hd", (FMT__H, sig));
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

static zb_uint8_t data_indication(zb_uint8_t param)
{
    zb_apsde_data_indication_t *ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);

    if (ind->profileid == 0x0002)
    {
        TRACE_MSG(TRACE_APP3, "apsde_data_indication: param %hu len %u status 0x%x",
                  (FMT__H_D_D, param, (zb_uint_t)zb_buf_len(param), zb_buf_get_status(param)));

        ZB_SCHEDULE_CALLBACK(zc_send_data, param);
        return ZB_TRUE;               /* processed */
    }

    return ZB_FALSE;
}

static void zc_send_data(zb_uint8_t param)
{
    zb_apsde_data_req_t *req;
    zb_uint8_t *ptr = NULL;
    zb_ushort_t i;

    if (g_remote_addr != 0)
    {
        ptr = zb_buf_initial_alloc(param, ZB_TEST_DATA_SIZE);
        req = zb_buf_get_tail(param, sizeof(zb_apsde_data_req_t));
        req->tx_options = ZB_APSDE_TX_OPT_ACK_TX | ZB_APSDE_TX_OPT_SECURITY_ENABLED;
        req->dst_addr.addr_short = g_remote_addr;
        req->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
        req->radius = 1;
        req->profileid = 2;
        req->src_endpoint = 10;
        req->dst_endpoint = 10;
        req->clusterid = 0;
        g_packets_sent++;
        for (i = 0 ; i < ZB_TEST_DATA_SIZE - 2 ; ++i)
        {
            ptr[i] = i % 32 + '0';
        }
        /* place packet counter in last two bytes */
        ptr[i++] = (g_packets_sent >> 8) & 0xff;
        ptr[i]   = g_packets_sent & 0xff;

        TRACE_MSG(TRACE_APP3, "Sending apsde_data.request %hd pkts_sent %u", (FMT__H_D, param, g_packets_sent));
        ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, param);
    }
    else
    {
        zb_buf_free(param);
    }
}
