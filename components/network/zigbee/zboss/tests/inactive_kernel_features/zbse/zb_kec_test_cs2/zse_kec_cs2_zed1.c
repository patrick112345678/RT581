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
/* PURPOSE: ZED1
*/

#define ZB_TRACE_FILE_ID 40942
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "zboss_api.h"
#include "zb_se_zdkec.h"

//zb_ieee_addr_t g_ieee_addr = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
zb_ieee_addr_t g_ieee_addr = {0x12, 0x10, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a};

//static const zb_uint8_t g_key_nwk[16] = { 0x0b, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0, 0, 0, 0, 0, 0, 0, 0};
//static zb_uint8_t g_key_c[16] = { 0x45, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66};
//static zb_ieee_addr_t g_ieee_addr_c = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
//static zb_ieee_addr_t g_ieee_addr_c = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

static zb_uint8_t const_certificate[48] = {0x02, 0x06, 0x15, 0xE0, 0x7D, 0x30, 0xEC, 0xA2,
                                           0xDA, 0xD5, 0x80, 0x02, 0xE6, 0x67, 0xD9, 0x4B,
                                           0xC1, 0xB4, 0x22, 0x39, 0x83, 0x07, 0x00, 0x00,
                                           0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x54, 0x45,
                                           0x53, 0x54, 0x53, 0x45, 0x43, 0x41, 0x01, 0x09,
                                           0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                                          };

static zb_uint8_t const_ca_public_key[22] = {0x02, 0x00, 0xFD, 0xE8, 0xA7, 0xF3, 0xD1, 0x08,
                                             0x42, 0x24, 0x96, 0x2A, 0x4E, 0x7C, 0x54, 0xE6,
                                             0x9A, 0xC3, 0xF0, 0x4D, 0xA6, 0xB8
                                            };

static zb_uint8_t const_private_key[21] = {0x01, 0xE9, 0xDD, 0xB5, 0x58, 0x0C, 0xF7, 0x2E,
                                           0xCE, 0x7F, 0x21, 0x5F, 0x0A, 0xE5, 0x94, 0xE4,
                                           0x8D, 0xF3, 0xE7, 0xFE, 0xE8
                                          };

static zb_uint8_t const_ca_283_public_key[37] = {0x02, 0x07, 0xa4, 0x45, 0x02, 0x2d, 0x9f, 0x39,
                                                 0xf4, 0x9b, 0xdc, 0x38, 0x38, 0x00, 0x26, 0xa2,
                                                 0x7a, 0x9e, 0x0a, 0x17, 0x99, 0x31, 0x3a, 0xb2,
                                                 0x8c, 0x5c, 0x1a, 0x1c, 0x6b, 0x60, 0x51, 0x54,
                                                 0xdb, 0x1d, 0xff, 0x67, 0x52
                                                };

static zb_uint8_t const_private_283[36] = {0x00, 0xF2, 0x56, 0x1A, 0xDB, 0x39, 0xEF, 0x49,
                                           0xC1, 0xD6, 0x2E, 0xF5, 0x18, 0x6C, 0x6E, 0x0C,
                                           0x15, 0x8A, 0x5A, 0x45, 0xBF, 0xCE, 0x38, 0x66,
                                           0x09, 0x31, 0xAC, 0xC3, 0x69, 0x45, 0x92, 0xD5,
                                           0xAC, 0xDE, 0x90, 0x06
                                          };

static zb_uint8_t const_cert_283[74] = {0x00, 0x84, 0xA9, 0x33, 0xB3, 0x7F, 0x01, 0x8D,
                                        0xEC, 0x0D, 0x08, 0x11, 0x12, 0x13, 0x14, 0x15,
                                        0x16, 0x17, 0x18, 0x00, 0x52, 0x92, 0xA3, 0x8A,
                                        0xFF, 0xFF, 0xFF, 0xFF, 0x0A, 0x0B, 0x0C, 0x0D,
                                        0x0E, 0x0F, 0x10, 0x12, 0x88, 0x03, 0x07, 0x62,
                                        0x77, 0xE2, 0xF7, 0xE2, 0x25, 0x2B, 0x16, 0xA0,
                                        0xE9, 0x2B, 0x6E, 0x87, 0x71, 0xBB, 0x3F, 0x20,
                                        0x79, 0x46, 0xCB, 0xD4, 0xA4, 0x5D, 0x9A, 0x9D,
                                        0xF6, 0xED, 0xAB, 0x8C, 0x79, 0x6A, 0x48, 0xE8,
                                        0x9D, 0xEC
                                       };

/******************* Declare attributes ************************/
static zb_uint8_t g_remote_ep = 0;

#define KEC_CLIENT_ENDPOINT 10
/* Basic cluster attributes data */
zb_uint8_t g_attr_zcl_version  = ZB_ZCL_VERSION;
zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;
zb_uint16_t g_attr_kec_suite = ZB_KEC_SUPPORTED_CRYPTO_ATTR;

ZB_ZCL_DECLARE_KEC_ATTRIB_LIST(kec_attr_list, &g_attr_kec_suite);
/* Identify cluster attributes data */
//zb_uint16_t g_attr_identify_time = 0;
//ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_attr_identify_time);
/********************* Declare device **************************/
ZB_SE_DECLARE_KEC_CLUSTER_LIST(kec_clusters);

ZB_SE_DECLARE_KEC_EP(kec_ep, KEC_CLIENT_ENDPOINT, kec_clusters);

ZB_SE_DECLARE_KEC_CTX(kec_ctx, kec_ep);
/*
static void test_device_annce_cb(zb_zdo_device_annce_t *da)
{
  zb_ieee_addr_t ze_addr = TEST_ZE_ADDR;
  zb_ieee_addr_t zr_addr = TEST_ZR_ADDR;

  if (!ZB_IEEE_ADDR_CMP(da->ieee_addr, ze_addr) &&
      !ZB_IEEE_ADDR_CMP(da->ieee_addr, zr_addr))
  {
      TRACE_MSG(TRACE_ERROR, "Unknown device has joined!", (FMT__0));
  }
  // Use first joined device as destination for outgoing APS packets
  if (g_remote_addr == 0)
  {
    g_remote_addr = da->nwk_addr;
    if (PACKETS_FROM_ZC_NR != 0)
    {
      ZB_SCHEDULE_CALLBACK(zc_send_data_loop, 0);
    }
  }
}
*/
#if 0
static void match_desc_callback2(zb_uint8_t param)
{
    static zb_ushort_t cnt = 0;

    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_uint8_t *zdp_cmd = ZB_BUF_BEGIN(buf);
    zb_zdo_match_desc_resp_t *resp = (zb_zdo_match_desc_resp_t *)zdp_cmd;
    zb_uint8_t *match_list = (zb_uint8_t *)(resp + 1);
    zb_uint8_t g_error = 0;

    TRACE_MSG(TRACE_APS1, "match_desc_callback2 status %hd, addr 0x%x",
              (FMT__H, resp->status, resp->nwk_addr));
    //  if (resp->status != ZB_ZDP_STATUS_SUCCESS || resp->nwk_addr != zb_address_short_by_ieee((zb_uint8_t*) g_ieee_addr_c))
    //  {
    //    TRACE_MSG(TRACE_APS1, "Error incorrect status/addr", (FMT__0));
    //    g_error++;
    //  }
    /*
      asdu=Match_Descr_rsp(Status=0x00=Success, NWKAddrOfInterest=0x0000,
      MatchLength=0x01, MatchList=0x01)
    */
    TRACE_MSG(TRACE_APS1, "match_len %hd, list %hd ", (FMT__H_H, resp->match_len, *match_list));
    if (resp->match_len != 1 || *match_list != 119)
    {
        TRACE_MSG(TRACE_APS1, "Error incorrect match result", (FMT__0));
        g_error++;
    }

    TRACE_MSG(TRACE_APS1, "error counter %hd", (FMT__H, g_error));

    g_remote_ep = *match_list;

    if (g_error != 0)
    {
        TRACE_MSG(TRACE_APS1, "Test FAILED", (FMT__0));
        zb_free_buf(buf);
    }
    else
    {
        TRACE_MSG(TRACE_APS1, "Test PASSED", (FMT__0));

        //      ZB_SCHEDULE_ALARM(initiate_kec, param, 0*ZB_TIME_ONE_SECOND);
        zb_free_buf(buf);
        cnt++;
    }
}

static void match_desc_req2(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_zdo_match_desc_param_t *req;

    ZB_BUF_INITIAL_ALLOC(buf, sizeof(zb_zdo_match_desc_param_t) + (2 + 3) * sizeof(zb_uint16_t), req);

    req->nwk_addr = 0x0000;//zb_address_short_by_ieee((zb_uint8_t*) g_ieee_addr_c); //send to TC
    req->addr_of_interest = req->nwk_addr;
    req->profile_id = ZB_AF_SE_PROFILE_ID;
    req->num_in_clusters = 1;
    req->num_out_clusters = 0;
    req->cluster_list[0] = ZB_ZCL_CLUSTER_ID_KEY_ESTABLISHMENT;

    zb_zdo_match_desc_req(param, match_desc_callback2);
}

static void match_desc_req2_(zb_uint8_t param)
{
    ZVUNUSED(param);
    ZB_GET_OUT_BUF_DELAYED(match_desc_req2);
}
#endif

static void kec_callback(zb_uint8_t param)
{
    //  static zb_ushort_t cnt = 0;
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_uint8_t *zdp_cmd = ZB_BUF_BEGIN(buf);
    zb_zdo_match_desc_resp_t *resp = (zb_zdo_match_desc_resp_t *)zdp_cmd;
    TRACE_MSG(TRACE_ZSE1, "kec_callback status %hd", (FMT__H, resp->status));
    DUMP_TRAF("data:", zdp_cmd, 8, 0);
    if (resp->status != 0)
    {
        TRACE_MSG(TRACE_ZSE1, "kec connection failed", (FMT__0));
        zb_free_buf(buf);
    }
    else
    {
        TRACE_MSG(TRACE_ZSE1, "kec connection established, call match desc_req2", (FMT__0));
        //    ZB_SCHEDULE_ALARM(match_desc_req2, param, 1*ZB_TIME_ONE_SECOND);
    }
}

//static zb_uint32_t g_error = 0;
static void initiate_kec(zb_uint8_t param)
{
    //    ZVUNUSED(param);
    TRACE_MSG(TRACE_APP1, "Initiate KEC", (FMT__0));

    //  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    //  zb_zdo_match_desc_param_t *req;

    //  ZB_BUF_INITIAL_ALLOC(buf, sizeof(zb_zdo_match_desc_param_t) + (2 + 3) * sizeof(zb_uint16_t), req);

    //  zb_kec_load_keys(KEC_CS1, const_ca_public_key, const_certificate, const_private_key);
    zb_se_kec_initiate_key_establishment(param, KEC_CLIENT_ENDPOINT, g_remote_ep, kec_callback);
    //  ZB_SCHEDULE_ALARM(match_desc_req2_, 0, 1*ZB_TIME_ONE_SECOND);

    //  zb_zdo_match_desc_req(param, kec_callback);
}

static void match_desc_callback(zb_uint8_t param)
{
    static zb_ushort_t cnt = 0;

    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_uint8_t *zdp_cmd = ZB_BUF_BEGIN(buf);
    zb_zdo_match_desc_resp_t *resp = (zb_zdo_match_desc_resp_t *)zdp_cmd;
    zb_uint8_t *match_list = (zb_uint8_t *)(resp + 1);
    zb_uint8_t g_error = 0;

    TRACE_MSG(TRACE_APS1, "match_desc_callback status %hd, addr 0x%x",
              (FMT__H, resp->status, resp->nwk_addr));
    //  if (resp->status != ZB_ZDP_STATUS_SUCCESS || resp->nwk_addr != zb_address_short_by_ieee((zb_uint8_t*) g_ieee_addr_c))
    //  {
    //    TRACE_MSG(TRACE_APS1, "Error incorrect status/addr", (FMT__0));
    //    g_error++;
    //  }
    /*
      asdu=Match_Descr_rsp(Status=0x00=Success, NWKAddrOfInterest=0x0000,
      MatchLength=0x01, MatchList=0x01)
    */
    TRACE_MSG(TRACE_APS1, "match_len %hd, list %hd ", (FMT__H_H, resp->match_len, *match_list));
    if (resp->match_len != 1 || *match_list != 119)
    {
        TRACE_MSG(TRACE_APS1, "Error incorrect match result", (FMT__0));
        g_error++;
    }

    TRACE_MSG(TRACE_APS1, "error counter %hd", (FMT__H, g_error));

    g_remote_ep = *match_list;

    if (g_error != 0)
    {
        TRACE_MSG(TRACE_APS1, "Test FAILED", (FMT__0));
        zb_free_buf(buf);
    }
    else
    {
        ZB_SCHEDULE_ALARM(initiate_kec, param, 0 * ZB_TIME_ONE_SECOND);
        //      zb_free_buf(buf);
        cnt++;
    }
}


static void match_desc_req(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_zdo_match_desc_param_t *req;

    ZB_BUF_INITIAL_ALLOC(buf, sizeof(zb_zdo_match_desc_param_t) + (2 + 3) * sizeof(zb_uint16_t), req);

    /*
      NWKAddrOfInterest=0x0000 16-bit DUT ZC NWK address
      ProfileID=Profile of interest to match=0x0103
      NumInClusters=Number of input clusters to match=0x02,
      InClusterList=matching cluster list=0x54 0xe0
      NumOutClusters=return value=0x03
      OutClusterList=return value=0x1c 0x38 0xa8
    */

    req->nwk_addr = 0x0000;//zb_address_short_by_ieee((zb_uint8_t*) g_ieee_addr_c); //send to TC
    req->addr_of_interest = req->nwk_addr;
    req->profile_id = ZB_AF_SE_PROFILE_ID;
    req->num_in_clusters = 1;
    req->num_out_clusters = 0;
    req->cluster_list[0] = ZB_ZCL_CLUSTER_ID_KEY_ESTABLISHMENT;

    zb_zdo_match_desc_req(param, match_desc_callback);
}

MAIN()
{
    ARGV_UNUSED;

    ZB_SET_TRACE_ON();
    ZB_SET_TRAF_DUMP_ON();

    ZB_INIT("zse_zed1");

    zb_set_long_address(g_ieee_addr);
    zb_set_network_router_role(1l << 22); //ZB_DEFAULT_APS_CHANNEL_MASK);
    zb_set_nvram_erase_at_start(ZB_FALSE);

    //ZB_SET_TRACE_MASK(0xdfff);

    /* Set to 1 to force entire nvram erase. */
    //ZB_AIB().aps_nvram_erase_at_start = 1;

    //  zb_zdo_register_device_annce_cb(test_device_annce_cb);

    //  zb_kec_load_keys(KEC_CS1, const_ca_public_key, const_certificate, const_private_key);
#ifndef ZB_PRODUCTION_CONFIG
    if ( zb_secur_ic_str_set("966b 9f3e f98a ! e605 - 9708") == RET_ERROR )
    {
        TRACE_MSG(TRACE_ERROR, "install_code failed", (FMT__0));
        ZB_ASSERT(0);
        TRACE_DEINIT();
        MAIN_RETURN(1);
    }
#endif
    /* set ieee addr */
    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr);


    //  zb_secur_update_key_pair(g_ieee_addr_c, i_key, ZB_SECUR_UNIQUE_KEY, ZB_SECUR_PROVISIONAL_KEY);
    //  zb_secur_setup_preconfigured_key(i_key, 0);
    //  zb_secur_update_key_pair(g_ieee_addr_c, g_key_c, ZB_SECUR_UNIQUE_KEY, ZB_SECUR_VERIFIED_KEY);
    /****************** Register Device ********************************/
    ZB_AF_REGISTER_DEVICE_CTX(&kec_ctx);


    ZB_AIB().aps_insecure_join = ZB_FALSE;

    /* become an ED */
    ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ED;
    ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);

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
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
            TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
            //#ifndef ZB_PRODUCTION_CONFIG
            zb_se_load_ecc_cert(KEC_CS1, const_ca_public_key, const_certificate, const_private_key);
            zb_se_load_ecc_cert(KEC_CS2, const_ca_283_public_key, const_cert_283, const_private_283);
            //#endif
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);

            ZB_GET_OUT_BUF_DELAYED(match_desc_req);

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
        case ZB_ZDO_SIGNAL_LEAVE:
            TRACE_MSG(TRACE_APP1, "Device leave", (FMT__0));
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

