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
/* PURPOSE: Metering ZC device sample
*/

#define ZB_TRACE_FILE_ID 40180

#include "zboss_api.h"
#include "../common/se_hrf_utils.h"

#define METERING_DEV_ENDPOINT 1
#define METERING_DEV_CHANNEL_MASK (1L << 22)

/* SummationFormatting: XXXXXX.Y, do not suppress leading zeroes */
#define METERING_DEV_SUMM_FMT_LEFT 6
#define METERING_DEV_SUMM_FMT_RIGHT 1
#define METERING_DEV_SUMM_FMT_SUPPR 0
#define METERING_DEV_SUMMATION_FORMATTING \
  ZB_ZCL_METERING_FORMATTING_SET(METERING_DEV_SUMM_FMT_SUPPR, METERING_DEV_SUMM_FMT_LEFT, METERING_DEV_SUMM_FMT_RIGHT)

static char g_installcode2[] = "966b9f3ef98ae605 9708";
static char g_installcode3[] = "966b9f3ef98ae605 9708";

#define SE_CRYPTOSUITE_1

zb_uint8_t g_key[16] = { 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0, 0, 0, 0, 0, 0, 0, 0};

#ifdef SE_CRYPTOSUITE_1
#define METERING_DEV_ADDR     {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}

static zb_ieee_addr_t g_ieee_addr2 = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static zb_ieee_addr_t g_ieee_addr3 = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};

static zb_uint8_t const_ca_public_key[22] = {0x02,0x00,0xFD,0xE8,0xA7,0xF3,0xD1,0x08,
                                             0x42,0x24,0x96,0x2A,0x4E,0x7C,0x54,0xE6,
                                             0x9A,0xC3,0xF0,0x4D,0xA6,0xB8};

static zb_uint8_t const_certificate[48] = {0x03,0x04,0x5F,0xDF,0xC8,0xD8,0x5F,0xFB,
                                           0x8B,0x39,0x93,0xCB,0x72,0xDD,0xCA,0xA5,
                                           0x5F,0x00,0xB3,0xE8,0x7D,0x6D,0x00,0x00,
                                           0x00,0x00,0x00,0x00,0x00,0x01,0x54,0x45,
                                           0x53,0x54,0x53,0x45,0x43,0x41,0x01,0x09,
                                           0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x00};

static zb_uint8_t const_private_key[21] = {0x00,0xB8,0xA9,0x00,0xFC,0xAD,0xEB,0xAB,
                                           0xBF,0xA3,0x83,0xB5,0x40,0xFC,0xE9,0xED,
                                           0x43,0x83,0x95,0xEA,0xA7};

#endif

#ifdef SE_CRYPTOSUITE_2
#define METERING_DEV_ADDR   {0x11, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a}
static zb_ieee_addr_t g_ieee_addr2 = {0x12, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a};
static zb_uint8_t const_ca_283_public_key[37] = {0x02,0x07,0xa4,0x45,0x02,0x2d,0x9f,0x39,
                                           0xf4,0x9b,0xdc,0x38,0x38,0x00,0x26,0xa2,
                                           0x7a,0x9e,0x0a,0x17,0x99,0x31,0x3a,0xb2,
                                           0x8c,0x5c,0x1a,0x1c,0x6b,0x60,0x51,0x54,
                                           0xdb,0x1d,0xff,0x67,0x52};

static zb_uint8_t const_private_283[36] = {0x01,0x51,0xCD,0x0D,0xBC,0xB8,0x04,0x74,
0xBF,0x7A,0xC9,0xFE,0xEB,0xE3,0x9C,0x7A,
0x32,0xA6,0x35,0x18,0x93,0x8F,0xCA,0x97,
0x54,0xAA,0xE1,0x32,0xBC,0x9C,0x73,0xBE,
0x94,0xA7,0xE1,0xBE};

static zb_uint8_t const_cert_283[74] = {0x00,0x26,0x22,0xA5,0x05,0xE8,0x93,0x8F,
                             0x27,0x0D,0x08,0x11,0x12,0x13,0x14,0x15,
                             0x16,0x17,0x18,0x00,0x52,0x92,0xA3,0x5B,
                             0xFF,0xFF,0xFF,0xFF,0x0A,0x0B,0x0C,0x0D,
                             0x0E,0x0F,0x10,0x11,0x88,0x03,0x03,0xB4,
                             0xE9,0xDC,0x54,0x3A,0x64,0x33,0x3C,0x98,
                             0x23,0x08,0x02,0x2B,0x54,0xE6,0x7E,0x2F,
                             0x15,0xF5,0x32,0x55,0x1B,0x0A,0x11,0xE2,
                             0xE2,0xC1,0xC1,0xD3,0x09,0x7A,0x43,0x24,
                             0xE7,0xED};

#endif

static zb_ieee_addr_t g_dev_addr = METERING_DEV_ADDR;

/** @struct metering_dev_nvram_data_s
 *  @brief metering device info that should be stored in NVRAM
 */
typedef ZB_PACKED_PRE struct metering_dev_nvram_data_s
{
  zb_uint48_t curr_summ_delivered;          /**< CurrentSummationDelivered attribute value */
  zb_uint8_t status;                        /**< Status attribute value */
  zb_uint8_t aling[1];
} ZB_PACKED_STRUCT metering_dev_nvram_data_t;


/** @struct metering_dev_basic_attrs_s
 *  @brief Basic cluster attributes
 */
typedef struct metering_dev_basic_attrs_s
{
  zb_uint8_t zcl_version;
  zb_uint8_t power_source;
} metering_dev_basic_attrs_t;


/** @struct metering_dev_kec_attrs_s
 *  @brief Key Establishment cluster attributes
 */
typedef struct metering_dev_kec_attrs_s
{
  zb_uint16_t kec_suite;
} metering_dev_kec_attrs_t;


/** @struct metering_dev_identify_attrs_s
 *  @brief Identify cluster attributes
 */
typedef struct metering_dev_identify_attrs_s
{
  zb_uint16_t identify_time;
} metering_dev_identify_attrs_t;


/** @struct metering_dev_metering_attrs_s
 *  @brief Metering cluster attributes
 */
typedef struct metering_dev_metering_attrs_s
{
  zb_uint48_t curr_summ_delivered;
  zb_uint8_t status;
  zb_uint8_t unit_of_measure;
  zb_uint8_t summation_formatting;
  zb_uint8_t device_type;
} metering_dev_metering_attrs_t;


/** @struct metering_dev_ctx_s
 *  @brief Metering device context
 */
typedef struct metering_dev_ctx_s
{
  zb_bool_t first_measurement_done;

  /* metering device attributes */
  metering_dev_basic_attrs_t basic_attrs;          /**< Basic cluster attributes */
  metering_dev_kec_attrs_t kec_attrs;              /**< Key Establishement cluster attributes */
  metering_dev_identify_attrs_t identify_attrs;    /**< Identify cluster attributes */
  metering_dev_metering_attrs_t metering_attrs;    /**< Metering cluster attributes */
} metering_dev_ctx_t;


/* device context */
static metering_dev_ctx_t g_dev_ctx;

void metering_dev_ctx_init();
void metering_dev_clusters_attrs_init(zb_uint8_t param);
void metering_dev_default_reporting_init(zb_uint8_t param);
void metering_dev_app_init(zb_uint8_t param);

static void metering_dev_main(zb_uint8_t param);


/** [COMMON_DECLARATION] */

/*********************  Clusters' attributes  **************************/

/* Basic cluster attributes */
ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list,
                                 &g_dev_ctx.basic_attrs.zcl_version,
                                 &g_dev_ctx.basic_attrs.power_source);


/* Key Establishment cluster attributes */
ZB_ZCL_DECLARE_KEC_ATTRIB_LIST(kec_attr_list, &g_dev_ctx.kec_attrs.kec_suite);


/* Identify cluster attributes */
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_dev_ctx.identify_attrs.identify_time);


/* Metering cluster attributes */
ZB_ZCL_DECLARE_METERING_ATTRIB_LIST(metering_attr_list,
                                    &g_dev_ctx.metering_attrs.curr_summ_delivered,
                                    &g_dev_ctx.metering_attrs.status,
                                    &g_dev_ctx.metering_attrs.unit_of_measure,
                                    &g_dev_ctx.metering_attrs.summation_formatting,
                                    &g_dev_ctx.metering_attrs.device_type);

/*********************  Device declaration  **************************/

ZB_SE_DECLARE_METERING_DEV_CLUSTER_LIST(metering_dev_clusters,
                                        basic_attr_list,
                                        kec_attr_list,
                                        identify_attr_list,
                                        metering_attr_list);

ZB_SE_DECLARE_METERING_DEV_EP(metering_dev_ep, METERING_DEV_ENDPOINT, metering_dev_clusters);

ZB_SE_DECLARE_METERING_DEV_CTX(metering_dev_zcl_ctx, metering_dev_ep);

/** [COMMON_DECLARATION] */


/*********************  Device-specific functions  **************************/

static void metering_dev_show_attr(zb_uint16_t attr_id)
{
  zb_char_t buf[SE_HRF_UINT48_BUF_LEN];
  zb_char_t *p;

  switch (attr_id)
  {
    case ZB_ZCL_ATTR_METERING_CURRENT_SUMMATION_DELIVERED_ID:
      p = se_hrf_metering_format_uint48_value(g_dev_ctx.metering_attrs.curr_summ_delivered,
                                              METERING_DEV_SUMM_FMT_LEFT,
                                              METERING_DEV_SUMM_FMT_RIGHT,
                                              METERING_DEV_SUMM_FMT_SUPPR,
                                              buf, SE_HRF_UINT48_BUF_LEN);
      TRACE_MSG(TRACE_APP1, "Current Summation Delivered: %s %s",
                (FMT__A_A, p, se_hrf_metering_unit_of_measure(g_dev_ctx.metering_attrs.unit_of_measure)));
      break;

    case ZB_ZCL_ATTR_METERING_STATUS_ID:
      TRACE_MSG(TRACE_APP1, "Device Status: 0x%x", (FMT__D, g_dev_ctx.metering_attrs.status));
      TRACE_MSG(TRACE_APP1, "   - check meter: %d",
                (FMT__D, g_dev_ctx.metering_attrs.status & ZB_ZCL_METERING_ELECTRICITY_CHECK_METER));
      TRACE_MSG(TRACE_APP1, "   - low battery: %d",
                (FMT__D, g_dev_ctx.metering_attrs.status & ZB_ZCL_METERING_ELECTRICITY_LOW_BATTERY));
      TRACE_MSG(TRACE_APP1, "   - tamper detect: %d",
                (FMT__D, g_dev_ctx.metering_attrs.status & ZB_ZCL_METERING_ELECTRICITY_TAMPER_DETECT));
      TRACE_MSG(TRACE_APP1, "   - power failure: %d",
                (FMT__D, g_dev_ctx.metering_attrs.status & ZB_ZCL_METERING_ELECTRICITY_POWER_FAILURE));
      TRACE_MSG(TRACE_APP1, "   - power quality: %d",
                (FMT__D, g_dev_ctx.metering_attrs.status & ZB_ZCL_METERING_ELECTRICITY_POWER_QUALITY));
      TRACE_MSG(TRACE_APP1, "   - leak detect: %d",
                (FMT__D, g_dev_ctx.metering_attrs.status & ZB_ZCL_METERING_ELECTRICITY_LEAK_DETECT));
      TRACE_MSG(TRACE_APP1, "   - service disconnect open: %d",
                (FMT__D, g_dev_ctx.metering_attrs.status & ZB_ZCL_METERING_ELECTRICITY_SERVICE_DISCONNECT_OPEN));
      break;

    default:
      break;
  }
}


/* save metering device's info into NVRAM */
static void metering_dev_save_data(zb_uint8_t param)
{
  ZVUNUSED(param);
  /* If we fail, trace is given and assertion is triggered */
  (void)zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);
  ZB_SCHEDULE_APP_ALARM(metering_dev_save_data, 0, 60 * ZB_TIME_ONE_SECOND);
}


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


zb_uint16_t metering_dev_get_nvram_data_size()
{
  TRACE_MSG(TRACE_APP1, "metering_dev_get_nvram_data_size: ret %hd", (FMT__H, sizeof(metering_dev_nvram_data_t)));
  return sizeof(metering_dev_nvram_data_t);
}


void metering_dev_ctx_init()
{
  TRACE_MSG(TRACE_APP1, ">> metering_dev_ctx_init", (FMT__0));

  ZB_MEMSET(&g_dev_ctx, 0, sizeof(g_dev_ctx));

  TRACE_MSG(TRACE_APP1, "<< metering_dev_ctx_init", (FMT__0));
}


void metering_dev_clusters_attrs_init(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> metering_dev_clusters_attrs_init", (FMT__0));
  ZVUNUSED(param);

  g_dev_ctx.basic_attrs.zcl_version = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
  g_dev_ctx.basic_attrs.power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

  g_dev_ctx.kec_attrs.kec_suite = ZB_KEC_SUPPORTED_CRYPTO_ATTR;

  g_dev_ctx.identify_attrs.identify_time = 0;

  g_dev_ctx.metering_attrs.status = ZB_ZCL_METERING_STATUS_DEFAULT_VALUE;
  g_dev_ctx.metering_attrs.unit_of_measure = ZB_ZCL_METERING_UNIT_OF_MEASURE_KW_KWH_BINARY;
  g_dev_ctx.metering_attrs.summation_formatting = METERING_DEV_SUMMATION_FORMATTING;
  g_dev_ctx.metering_attrs.device_type = ZB_ZCL_METERING_DEVICE_TYPE_ELECTRIC_METERING;

  TRACE_MSG(TRACE_APP1, "<< metering_dev_clusters_attrs_init", (FMT__0));
}


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
  zb_zcl_put_default_reporting_info(&rep_info);

  rep_info.attr_id = ZB_ZCL_ATTR_METERING_STATUS_ID;
  zb_zcl_put_default_reporting_info(&rep_info);

  TRACE_MSG(TRACE_APP1, "<< metering_dev_default_reporting_init", (FMT__0));
}

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
  zb_set_network_coordinator_role_se(METERING_DEV_CHANNEL_MASK);
#if 1//#ifdef DEBUG
  zb_secur_setup_nwk_key(g_key, 0);
#endif

  TRACE_MSG(TRACE_APP1, "<< metering_dev_app_init", (FMT__0));
}


/* generate and accumulate measurements */
static void metering_dev_main(zb_uint8_t param)
{
  zb_int24_t instantaneous_consumption;
  zb_int32_t measure;
  zb_uint64_t val;

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
  measure = (zb_int32_t)ZB_RANDOM_VALUE(9);
  ZB_INT24_FROM_INT32(instantaneous_consumption, measure);
  ZB_UINT48_ADD_INT24(g_dev_ctx.metering_attrs.curr_summ_delivered, instantaneous_consumption);

  /* add some random device status */
  switch (measure)
  {
    case 3:
    case 5:
    case 8:
      g_dev_ctx.metering_attrs.status = ZB_ZCL_METERING_ELECTRICITY_POWER_QUALITY;
      break;

    default:
      g_dev_ctx.metering_attrs.status = 0;
      break;
  }

  val = (zb_uint64_t)instantaneous_consumption.low |
        (zb_uint64_t)instantaneous_consumption.high << 16;
  TRACE_MSG(TRACE_APP1, "instantaneous electricity consumption: 0.%llu %s",
            (FMT__D_A, val, se_hrf_metering_unit_of_measure(g_dev_ctx.metering_attrs.unit_of_measure)));

  metering_dev_show_attr(ZB_ZCL_ATTR_METERING_CURRENT_SUMMATION_DELIVERED_ID);
  metering_dev_show_attr(ZB_ZCL_ATTR_METERING_STATUS_ID);

  if (g_dev_ctx.first_measurement_done == ZB_FALSE)
  {
    ZB_SCHEDULE_APP_CALLBACK(metering_dev_save_data, 0);
    g_dev_ctx.first_measurement_done = ZB_TRUE;
  }

  ZB_SCHEDULE_APP_ALARM(metering_dev_main, 0, ZB_TIME_ONE_SECOND);
}


/*********************  SE Metering ZC  **************************/

MAIN()
{
  ARGV_UNUSED;

  /* for debug purposes */
  ZB_SET_TRACE_ON();
  ZB_SET_TRAF_DUMP_ON();

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


void zboss_signal_handler(zb_uint8_t param)
{
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  TRACE_MSG(TRACE_APP1, ">> zboss_signal_handler %h sig %hd status %hd",
            (FMT__H_H_H, param, sig, ZB_GET_APP_SIGNAL_STATUS(param)));

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch (sig)
    {
      case ZB_ZDO_SIGNAL_SKIP_STARTUP:
        zb_secur_ic_str_add(g_ieee_addr2, g_installcode2, NULL);
        zb_secur_ic_str_add(g_ieee_addr3, g_installcode3, NULL);
#ifdef SE_CRYPTOSUITE_1
        zse_load_ecc_cert(KEC_CS1, const_ca_public_key, const_certificate, const_private_key);
#endif
#ifdef SE_CRYPTOSUITE_2
        zse_load_ecc_cert(KEC_CS2, const_ca_283_public_key, const_cert_283, const_private_283);
#endif

        zboss_start_continue();
        break;

      case ZB_SIGNAL_DEVICE_FIRST_START:
      case ZB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APP1, "Device STARTED OK: first_start %hd",
                  (FMT__H, sig == ZB_SIGNAL_DEVICE_FIRST_START));

        metering_dev_default_reporting_init(0);
        ZB_SCHEDULE_APP_CALLBACK(metering_dev_main, 0);
        ZB_SCHEDULE_APP_ALARM(metering_dev_save_data, 0, 60 * ZB_TIME_ONE_SECOND);
        zb_se_permit_joining(500);
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
    zb_free_buf(ZB_BUF_FROM_REF(param));
  }

  TRACE_MSG(TRACE_APP1, "<< zboss_signal_handler", (FMT__0));
}
