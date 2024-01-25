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
/* PURPOSE: ZC
*/

#define ZB_TRACE_FILE_ID 40943
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "zboss_api_se.h"
#include "zb_se_tckec.h"

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

//zb_ieee_addr_t g_ieee_addr = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
zb_ieee_addr_t g_ieee_addr = {0x11, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a};
/* [zb_secur_setup_preconfigured_key_1] */
static const zb_uint8_t g_key_nwk[16] = { 0x0b, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0, 0, 0, 0, 0, 0, 0, 0};
/* [zb_secur_setup_preconfigured_key_1] */

//static const zb_ieee_addr_t g_ieee_addr1 = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
//static const zb_uint8_t g_key1[16] = { 0x12, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33};
//static zb_ieee_addr_t g_ieee_addr2 = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static zb_ieee_addr_t g_ieee_addr2 = {0x12, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a};
//static const zb_uint8_t g_key2[16] = { 0x45, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66};

static char g_installcode[]= "966b 9f3e f98a !sp@m! e605 - 9708";

#ifndef ZB_PRODUCTION_CONFIG
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
/******************* Declare attributes ************************/

#define KEC_SERVER_ENDPOINT 0x77
/* Basic cluster attributes data */
zb_uint8_t g_attr_zcl_version  = ZB_ZCL_VERSION;
zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;
zb_uint16_t g_attr_kec_suite = ZB_KEC_SUPPORTED_CRYPTO_ATTR;

ZB_ZCL_DECLARE_KEC_ATTRIB_LIST(kec_attr_list, &g_attr_kec_suite);
/* Identify cluster attributes data */
zb_uint16_t g_attr_identify_time = 0;
//ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_attr_identify_time);
/********************* Declare device **************************/
ZB_SE_DECLARE_KEC_CLUSTER_LIST(kec_clusters);

ZB_SE_DECLARE_KEC_EP(kec_ep, KEC_SERVER_ENDPOINT, kec_clusters);

ZB_SE_DECLARE_KEC_CTX(kec_ctx, kec_ep);

//light_control_ctx_t g_device_ctx;
/* Read hexadecimal string (no whitespaces) and convert to byte array
 * Note: no validation whatsoever, tread carefully with input files */
#if 0
static void hex_string_to_binary(char *in_hex, unsigned char *out_bin, int max_len)
{
  char *pos = in_hex;
  for(int i = 0; i < max_len; i++) {
    sscanf(pos, "%2hx", &out_bin[i]);
    pos += 2;
  }
}
#endif

MAIN()
{
  ARGV_UNUSED;
  ZB_SET_TRACE_ON();
  ZB_SET_TRAF_DUMP_ON();

//  ZB_PIBCACHE_PAN_ID() = 0x1aaa;

  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("zse_zc");

  zb_set_long_address(g_ieee_addr);
//zb_set_network_ed_role(LIGHT_CONTROL_CHANNEL_MASK);
  zb_set_network_coordinator_role(1l<<22);
  zb_set_nvram_erase_at_start(ZB_FALSE);
  zb_secur_setup_nwk_key((zb_uint8_t*) g_key_nwk, 0);
  /****************** Register Device ********************************/
  ZB_AF_REGISTER_DEVICE_CTX(&kec_ctx);

  //ZB_SET_TRACE_MASK(0xdffF);
  /* let's always be coordinator */
  //zb_set_channel
//  ZB_AIB().aps_designated_coordinator = 1;
  /* set ieee addr */
//  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr);

//  ZB_AF_SET_ENDPOINT_HANDLER(KEC_SERVER_ENDPOINT, zcl_specific_cluster_cmd_handler);

//#ifdef APS_FRAME_SECURITY
//  zb_secur_update_key_pair((zb_uint8_t*) g_ieee_addr1, (zb_uint8_t*) g_key1, ZB_SECUR_UNIQUE_KEY, ZB_SECUR_VERIFIED_KEY);

//  if ( zb_kec_install_code_to_key("966b 9f3e f98a ! e605 - 9708",i_key) == RET_ERROR )
//  {
//
//    TRACE_MSG(TRACE_ERROR, "install_code failed", (FMT__0));
//    ZB_ASSERT(0);
//    TRACE_DEINIT();
//    MAIN_RETURN(1);
//  }
//  zb_secur_update_key_pair((zb_uint8_t*)g_ieee_addr2, i_key, ZB_SECUR_UNIQUE_KEY, ZB_SECUR_PROVISIONAL_KEY);
//  zb_secur_update_key_pair((zb_uint8_t*) g_ieee_addr2, (zb_uint8_t*) g_key2, ZB_SECUR_UNIQUE_KEY, ZB_SECUR_VERIFIED_KEY);
//#endif

//  zb_kec_load_keys(KEC_CS1, const_ca_public_key, const_certificate, const_private_key);

  /* only ZR1 is visible for ZC */
//  MAC_ADD_VISIBLE_LONG((zb_uint8_t*) g_ieee_addr1);
//  ZB_NIB().max_children = 1;

  ZB_AIB().aps_insecure_join = ZB_FALSE;

  /* accept only one child */
//  ZB_NIB().max_children = 3;

  /* configure join duration */
//  ZDO_CTX().conf_attr.permit_join_duration = 10;
//  if ( zdo_dev_start() != RET_OK )

  if (zboss_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
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
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));

        if ( zb_secur_ic_str_add(g_ieee_addr2, g_installcode, NULL) != RET_OK )
        {
          TRACE_MSG(TRACE_ERROR, "install_code failed", (FMT__0));
          ZB_ASSERT(0);
        }
#ifndef ZB_PRODUCTION_CONFIG
        zb_se_load_ecc_cert(KEC_CS1, const_ca_public_key, const_certificate, const_private_key);
        zb_se_load_ecc_cert(KEC_CS2, const_ca_283_public_key, const_cert_283, const_private_283);
#endif
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        break;
      case ZB_NWK_SIGNAL_DEVICE_ASSOCIATED:
        {
        zb_nwk_signal_device_associated_params_t *dev_assoc_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_nwk_signal_device_associated_params_t);
        TRACE_MSG(TRACE_APP1, "Device associated: " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(dev_assoc_params->device_addr)));
        }
        break;
      case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
        {
        zb_zdo_signal_device_annce_params_t *dev_annce_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_annce_params_t);
        TRACE_MSG(TRACE_APP1, "Device annce: 0x%04X", (FMT__D, dev_annce_params->device_short_addr));
        }
        break;
      default:
        TRACE_MSG(TRACE_APP1, "Unknown signal: %d", (FMT__D, sig));
//      ZB_ASSERT(0);
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
}
