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
/* PURPOSE: TH-GPCT/TH-TOOL
*/

#define ZB_TEST_NAME GPS_REMOTE_SINK_COMMISSIONING_CONTROL_TH_GPP

#define ZB_TRACE_FILE_ID 63518
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_zcl.h"
#include "zb_secur_api.h"
#include "zb_ha.h"
#include "test_config.h"
#include "zgp/zgp_internal.h"

#ifdef ZB_ENABLE_HA
#include "../include/zgp_test_templates.h"

/*============================================================================*/
/*                             DECLARATIONS                                   */
/*============================================================================*/

static zb_ieee_addr_t g_th_gpp_addr = TH_GPCB_IEEE_ADDR;
static zb_uint8_t g_key_nwk[] = NWK_KEY;
static zb_uint8_t g_oob_key[] = TEST_OOB_KEY;
static zb_uint16_t g_dut_addr;

static void dev_annce_cb(zb_zdo_device_annce_t *da);
static void send_comm_cb(zb_uint8_t buf_ref);
static void send_sink_comm_mode(zb_uint8_t buf_ref);

static void send_commissioning_frame(zb_uint8_t buf_ref);
static void fill_gpdf_info(zb_gpdf_info_t *gpdf_info, zb_uint32_t use_frame_cnt, zb_uint16_t options);
static void prepare_commissioning_command(zb_uint8_t buf_ref, zb_uint32_t use_frame_cnt);

static void schedule_delay(zb_uint32_t timeout);

static void encrypt_payload(zb_gpdf_info_t *gpdf_info, zb_buf_t *packet);
static void construct_aes_nonce(
    zb_bool_t from_gpd,
    zb_zgpd_id_t *zgpd_id,
    zb_uint32_t frame_counter,
    zb_zgp_aes_nonce_t *res_nonce);


/*============================================================================*/
/*                             FSM CORE                                       */
/*============================================================================*/

/*! Program states according to test scenario */
enum test_states_e
{
    TEST_STATE_INITIAL,
    TEST_STATE_OPEN_NETWORK,
    TEST_STATE_WAIT_DUT_JOINING,
    /* STEP 1 */
    TEST_STATE_SEND_SINK_COMM_MODE_1,
    TEST_STATE_SEND_COMMISSIONING_1,
    TEST_STATE_DELAY_1,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_1,
    /* STEP 2 */
    TEST_STATE_START_DELAY_ON_STEP_2,
    TEST_STATE_SEND_SINK_COMM_MODE_2,
    TEST_STATE_SEND_COMMISSIONING_2,
    TEST_STATE_DELAY_2,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_2,
    /* STEP 3 */
    TEST_STATE_START_DELAY_ON_STEP_3,
    TEST_STATE_SEND_SINK_COMM_MODE_3,
    TEST_STATE_SEND_COMMISSIONING_3,
    TEST_STATE_DELAY_3,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_3,
    /* STEP 4 - SKIPPED */
    /* STEP 5 */
    TEST_STATE_START_DELAY_ON_STEP_5,
    TEST_STATE_SEND_SINK_COMM_MODE1_5,
    TEST_STATE_DELAY1_5,
    TEST_STATE_SEND_SINK_COMM_MODE2_5,
    TEST_STATE_DELAY2_5,
    TEST_STATE_SEND_COMMISSIONING_5,
    TEST_STATE_DELAY3_5,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_5,
    /* STEP 6 */
    TEST_STATE_START_DELAY_ON_STEP_6,
    TEST_STATE_SEND_SINK_COMM_MODE_6,
    TEST_STATE_DELAY_6,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_6,
    /* STEP 7 */
    TEST_STATE_START_DELAY_ON_STEP_7,
    TEST_STATE_SEND_SINK_COMM_MODE_7,
    TEST_STATE_DELAY_7,
    TEST_STATE_READ_OUT_DUT_SINK_TABLE_7,
    /* FINISH */
    TEST_STATE_FINISHED
};

ZB_ZGPC_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 1000)

static void send_zcl(zb_uint8_t buf_ref, zb_callback_t cb)
{
    TRACE_MSG(TRACE_APP1, ">send_zcl: test_state = %d", (FMT__D, TEST_DEVICE_CTX.test_state));
    ZVUNUSED(cb);

    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_1:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_2:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_3:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_5:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_6:
    case TEST_STATE_READ_OUT_DUT_SINK_TABLE_7:
    {
        zgp_cluster_send_gp_sink_table_request(buf_ref, g_dut_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                               ZGP_REQUEST_TABLE_ENTRIES_BY_INDEX << 3, NULL, 0, cb);
        break;
    }

    case TEST_STATE_SEND_SINK_COMM_MODE_1:
    case TEST_STATE_SEND_SINK_COMM_MODE_2:
    case TEST_STATE_SEND_SINK_COMM_MODE_3:
    case TEST_STATE_SEND_SINK_COMM_MODE1_5:
    case TEST_STATE_SEND_SINK_COMM_MODE2_5:
    case TEST_STATE_SEND_SINK_COMM_MODE_6:
    case TEST_STATE_SEND_SINK_COMM_MODE_7:
    {
        send_sink_comm_mode(buf_ref);
        break;
    }
    }

    TRACE_MSG(TRACE_APP1, "<send_zcl", (FMT__0));
}

static void perform_next_state(zb_uint8_t param)
{
    if (param)
    {
        zb_free_buf(ZB_BUF_FROM_REF(param));
    }

    if (TEST_DEVICE_CTX.pause)
    {
        ZB_SCHEDULE_ALARM(perform_next_state, 0,
                          ZB_TIME_ONE_SECOND * TEST_DEVICE_CTX.pause);
        TEST_DEVICE_CTX.pause = 0;
        return;
    }


    TEST_DEVICE_CTX.test_state++;

    TRACE_MSG(TRACE_APP1, ">perform_next_state: test_state = %d",
              (FMT__D, TEST_DEVICE_CTX.test_state));

    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_OPEN_NETWORK:
    {
        TRACE_MSG(TRACE_APP1, "Open network for joining.", (FMT__0));
        zb_bdb_set_legacy_device_support(1);
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        break;
    }

    case TEST_STATE_WAIT_DUT_JOINING:
    {
        TRACE_MSG(TRACE_APP1, "Wait for DUT joining.", (FMT__0));
        break;
    }

    case TEST_STATE_START_DELAY_ON_STEP_2:
    case TEST_STATE_START_DELAY_ON_STEP_3:
    case TEST_STATE_START_DELAY_ON_STEP_5:
    case TEST_STATE_START_DELAY_ON_STEP_6:
    case TEST_STATE_START_DELAY_ON_STEP_7:
    {
        schedule_delay(TEST_PARAM_TH_GPCT_START_DELAY);
        break;
    }
    case TEST_STATE_DELAY_1:
    case TEST_STATE_DELAY_2:
    case TEST_STATE_DELAY_3:
    case TEST_STATE_DELAY1_5:
    case TEST_STATE_DELAY2_5:
    case TEST_STATE_DELAY3_5:
    case TEST_STATE_DELAY_6:
    case TEST_STATE_DELAY_7:
    {
        schedule_delay(TEST_PARAM_TH_GPCT_SHORT_DELAY);
        break;
    }

    case TEST_STATE_SEND_COMMISSIONING_1:
    case TEST_STATE_SEND_COMMISSIONING_2:
    case TEST_STATE_SEND_COMMISSIONING_3:
    case TEST_STATE_SEND_COMMISSIONING_5:
    {
        ZB_GET_OUT_BUF_DELAYED(send_commissioning_frame);
        break;
    }

    case TEST_STATE_FINISHED:
    {
        TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
        break;
    }

    default:
    {
        ZB_SCHEDULE_ALARM(test_send_command, 0, ZB_TIME_ONE_SECOND);
    }
    }

    TRACE_MSG(TRACE_APP1, "<perform_next_state", (FMT__0));
}

/*============================================================================*/
/*                            TEST IMPLEMENTATION                             */
/*============================================================================*/

static void dev_annce_cb(zb_zdo_device_annce_t *da)
{
    zb_ieee_addr_t th_ieee = DUT_GPS_IEEE_ADDR;
    TRACE_MSG(TRACE_APP1, "dev_annce_cb: ieee = " TRACE_FORMAT_64 " NWK = 0x%x",
              (FMT__A_H, TRACE_ARG_64(da->ieee_addr), da->nwk_addr));

    if (ZB_IEEE_ADDR_CMP(th_ieee, da->ieee_addr) == ZB_TRUE)
    {
        if (TEST_DEVICE_CTX.test_state == TEST_STATE_WAIT_DUT_JOINING)
        {
            g_dut_addr = da->nwk_addr;
            ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
        }
    }
}

static void send_comm_cb(zb_uint8_t buf_ref)
{
    TRACE_MSG(TRACE_ZCL1, ">> send_simk_comm_mode %hd", (FMT__H, buf_ref));
    zb_free_buf(ZB_BUF_FROM_REF(buf_ref));
    ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
    TRACE_MSG(TRACE_ZCL1, "<< send_simk_comm_mode", (FMT__0));
}

static void send_sink_comm_mode(zb_uint8_t buf_ref)
{
    zb_buf_t     *buf = ZB_BUF_FROM_REF(buf_ref);
    zb_uint8_t   *ptr;
    zb_uint16_t  gpm_security_addr = 0xffff;
    zb_uint16_t  gpm_pairing_addr = 0xffff;
    zb_uint8_t   options = 0x09; /* Action = Enter, Involve Proxies */
    zb_uint8_t   endpoint = 0xFF;

    TRACE_MSG(TRACE_ZCL1, ">> send_simk_comm_mode %hd", (FMT__H, buf_ref));

    switch (TEST_DEVICE_CTX.test_state)
    {
    case TEST_STATE_SEND_SINK_COMM_MODE_1:
    {
        options = 0x01; /* Action = Enter */
        break;
    }
    case TEST_STATE_SEND_SINK_COMM_MODE_3:
    {
        options = 0x01; /* Action = Enter */
        endpoint = DUT_ENDPOINT;
        break;
    }
    case TEST_STATE_SEND_SINK_COMM_MODE2_5:
    {
        options = 0x08; /* Action = Exit, Involve Proxies */
        break;
    }
    case TEST_STATE_SEND_SINK_COMM_MODE_6:
    {
        endpoint = TH_INVALID_EP;
        break;
    }
    case TEST_STATE_SEND_SINK_COMM_MODE_7:
    {
        options = 0x0F; /* Action = Enter, Involve GPM in Security and Pairing, Involve Proxies */
        gpm_security_addr = 0x1111;
        gpm_pairing_addr = 0x2222;
        break;
    }
    }

    ptr = ZB_ZCL_START_PACKET_REQ(buf)
          ZB_ZCL_CONSTRUCT_SPECIFIC_COMMAND_REQ_FRAME_CONTROL(ptr, ZB_ZCL_DISABLE_DEFAULT_RESPONSE)
          ZB_ZCL_CONSTRUCT_COMMAND_HEADER_REQ(ptr, ZB_ZCL_GET_SEQ_NUM(), ZGP_SERVER_CMD_GP_SINK_COMMISSIONING_MODE);

    ZB_ZCL_PACKET_PUT_DATA8(ptr, options);

    /* The GPM address for security. */
    ZB_ZCL_PACKET_PUT_DATA16(ptr, &gpm_security_addr);
    /* The GPM address for pairing. */
    ZB_ZCL_PACKET_PUT_DATA16(ptr, &gpm_pairing_addr);

    ZB_ZCL_PACKET_PUT_DATA8(ptr, endpoint);

    ZB_ZCL_FINISH_PACKET(buf, ptr)
    ZB_ZCL_SEND_COMMAND_SHORT(buf, g_dut_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, ZGP_ENDPOINT,
                              ZGP_ENDPOINT, ZB_AF_GP_PROFILE_ID,
                              ZB_ZCL_CLUSTER_ID_GREEN_POWER, NULL);

    TRACE_MSG(TRACE_ZCL1, "<< send_simk_comm_mode", (FMT__0));
}


static void send_commissioning_frame(zb_uint8_t buf_ref)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(buf_ref);
    zb_uint16_t options = 0;
    zb_gpdf_info_t *gpdf_info = ZB_GET_BUF_PARAM(buf, zb_gpdf_info_t);

    TRACE_MSG(TRACE_APP1, ">send_commissioning_frame: param = %d, test_state = %d",
              (FMT__D_D, buf_ref, TEST_DEVICE_CTX.test_state));

    options = ZB_ZGP_FILL_COMM_NOTIFICATION_OPTIONS(ZB_ZGP_APP_ID_0000,
              0, /* RxAfterTX */
              ZB_ZGP_SEC_LEVEL_FULL_NO_ENC,
              ZB_ZGP_SEC_KEY_TYPE_NWK,
              0, /* Security processing not failed */
              0, /* not BidirCap */
              1 /* ProxyInfo Present */);
    fill_gpdf_info(gpdf_info, 0, options);
    prepare_commissioning_command(buf_ref, 0);

    /* Encrypt frame */
    {
        zb_uint8_t *ptr;
        ZB_BUF_ALLOC_LEFT(buf, 1, ptr);
        *ptr = gpdf_info->zgpd_cmd_id;

        encrypt_payload(gpdf_info, buf);
        gpdf_info->zgpd_cmd_id = *ptr;
        ZB_BUF_CUT_LEFT(buf, 1, ptr);
    }

    zb_zgp_cluster_gp_comm_notification_req(buf_ref,
                                            0 /* do not use alias */, 0x0000, 0x00,
                                            g_dut_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                            options,
                                            send_comm_cb);

    TRACE_MSG(TRACE_APP1, "<send_gp_comm_notify", (FMT__0));
}

static void fill_gpdf_info(zb_gpdf_info_t *gpdf_info, zb_uint32_t use_frame_cnt, zb_uint16_t options)
{
    zb_uint8_t frame_type = ZGP_FRAME_TYPE_DATA;

    TRACE_MSG(TRACE_APP1, ">fill_gpd_info", (FMT__0));

    ZB_BZERO(gpdf_info, sizeof(*gpdf_info));
    /* Always excellent */
    gpdf_info->rssi = 0x3b;
    gpdf_info->lqi = 0x3;

    gpdf_info->zgpd_id.app_id = ZB_ZGP_APP_ID_0000;
    gpdf_info->zgpd_id.addr.src_id = TEST_ZGPD_SRC_ID;

    gpdf_info->sec_frame_counter = use_frame_cnt;
    gpdf_info->zgpd_cmd_id = ZB_GPDF_CMD_COMMISSIONING;

    TRACE_MSG(TRACE_APP2, "FILL_GPDF: options = 0x%x", (FMT__H, options));

    ZB_GPDF_NWK_FRAME_CTL_EXT(gpdf_info->nwk_ext_frame_ctl,
                              ZB_ZGP_COMM_NOTIF_OPT_GET_APP_ID(options),
                              ZB_ZGP_GP_COMM_NOTIF_OPT_GET_SEC_LVL(options),
                              /* Security Key field: */
                              ZB_FALSE,
                              ZB_ZGP_GP_COMM_NOTIF_OPT_GET_RX_AFTER_TX(options),
                              ZGP_FRAME_DIR_FROM_ZGPD /* Dir: always from ZGPD */);

    ZB_GPDF_NWK_FRAME_CONTROL(gpdf_info->nwk_frame_ctl,
                              frame_type,
                              0, /* Auto-Commissioning */
                              ZB_TRUE /* frame_ext */);

    TRACE_MSG(TRACE_APP2, "FILL_GPDF: nwk_opt = 0x%x, nwk_ext_opt = 0x%x",
              (FMT__H_H, gpdf_info->nwk_frame_ctl, gpdf_info->nwk_ext_frame_ctl));

    TRACE_MSG(TRACE_APP1, "<fill_gpd_info", (FMT__0));
}

static void prepare_commissioning_command(zb_uint8_t buf_ref, zb_uint32_t use_frame_cnt)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(buf_ref);
    zb_uint8_t *ptr = ZB_BUF_BEGIN(buf);
    zb_uint8_t opt, ext_opt;
    zb_uint8_t encrypted_key[ZB_CCM_KEY_SIZE];
    zb_uint8_t mic[ZB_CCM_M];
    zb_gpdf_info_t *gpdf_info = ZB_GET_BUF_PARAM(buf, zb_gpdf_info_t);
    zb_uint8_t secur_lvl;
    zb_uint8_t s_k_r, s_k_p, s_k_e;
    zb_uint8_t comm_pld_len = 3;

    TRACE_MSG(TRACE_APP1, ">prepare_commissioning_command: param = %d", (FMT__D, buf_ref));

    s_k_r = 1;
    s_k_p = 1;
    s_k_e = 1;
    secur_lvl = ZB_ZGP_SEC_LEVEL_FULL_NO_ENC;

    /* SN, Key Request and ext options */
    opt = ZB_GPDF_COMM_OPT_FLD(1, 0, 0, 0, s_k_r, 0, 1);
    /* key present, encrypted and frame counter also present */
    ext_opt = ZB_GPDF_COMM_EXT_OPT_FLD(secur_lvl,
                                       ZB_ZGP_SEC_KEY_TYPE_NWK,
                                       s_k_p, s_k_p && s_k_e, s_k_p);
    if (s_k_p && s_k_e)
    {
        zb_zgpd_id_t id;
        zb_uint8_t link_key[ZB_CCM_KEY_SIZE] = ZB_STANDARD_TC_KEY;

        id = gpdf_info->zgpd_id;
        zb_zgp_protect_gpd_key(ZB_TRUE, &id, g_oob_key, link_key, encrypted_key, gpdf_info->sec_frame_counter, mic);
    }

    comm_pld_len += (s_k_p) ? ((s_k_e) ? 24 : 20) : 0;
    ZB_BUF_INITIAL_ALLOC(buf, comm_pld_len, ptr);
    *ptr++ = ZB_ZGP_ON_OFF_SWITCH_DEV_ID;
    *ptr++ = opt;
    *ptr++ = ext_opt;
    if (s_k_p)
    {
        if (s_k_e)
        {
            ZB_MEMCPY(ptr, encrypted_key, ZB_CCM_KEY_SIZE);
            ptr += ZB_CCM_KEY_SIZE;
            ZB_MEMCPY(ptr, mic, ZB_CCM_M);
            ptr += ZB_CCM_M;
        }
        else
        {
            ZB_MEMCPY(ptr, g_oob_key, ZB_CCM_KEY_SIZE);
            ptr += ZB_CCM_KEY_SIZE;
        }
        zb_put_next_htole32(ptr, use_frame_cnt);
        ptr += sizeof(use_frame_cnt);
    }

    TRACE_MSG(TRACE_APP1, "<prepare_commissioning_command", (FMT__0));
}


static void schedule_delay(zb_uint32_t timeout)
{
    ZB_ZGPC_SET_PAUSE(timeout);
    ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
}


static void encrypt_payload(zb_gpdf_info_t *gpdf_info, zb_buf_t *packet)
{
    zb_uint8_t *payload;
    zb_int8_t pld_len;
    zb_uint8_t sec_level = ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl);
    zb_zgp_aes_nonce_t nonce;
    zb_uint8_t pseudo_header[16];
    zb_uint8_t *ptr = pseudo_header;
    zb_uint8_t hdr_len = 0;

    construct_aes_nonce(
        (zb_bool_t)!ZB_GPDF_EXT_NFC_GET_DIRECTION(gpdf_info->nwk_ext_frame_ctl),
        &gpdf_info->zgpd_id,
        gpdf_info->sec_frame_counter,
        &nonce);

    /* Form Header */
    ZB_BZERO(pseudo_header, 16);
    *ptr++ = gpdf_info->nwk_frame_ctl;
    ++hdr_len;
    if (gpdf_info->nwk_ext_frame_ctl)
    {
        *ptr++ = gpdf_info->nwk_ext_frame_ctl;
        ++hdr_len;
    }
    if (gpdf_info->zgpd_id.app_id == ZB_ZGP_APP_ID_0000)
    {
        ptr = zb_put_next_htole32(ptr, gpdf_info->zgpd_id.addr.src_id);
        hdr_len += sizeof(gpdf_info->zgpd_id.addr.src_id);
    }
    else if (gpdf_info->zgpd_id.app_id == ZB_ZGP_APP_ID_0010)
    {
        *ptr++ = gpdf_info->zgpd_id.endpoint;
        ++hdr_len;
    }

    zb_put_next_htole32(ptr, gpdf_info->sec_frame_counter);
    hdr_len += sizeof(gpdf_info->sec_frame_counter);

    payload = ZB_BUF_BEGIN(packet);
    pld_len = ZB_BUF_LEN(packet);

    TRACE_MSG(TRACE_ERROR, "ENCRYPT: dump payload", (FMT__0));
    dump_traf(payload, pld_len);
    TRACE_MSG(TRACE_ERROR, "ENCRYPT: dump header", (FMT__0));
    dump_traf(pseudo_header, hdr_len);
    TRACE_MSG(TRACE_ERROR, "ENCRYPT: dump nonce", (FMT__0));
    dump_traf((zb_uint8_t *) &nonce, sizeof(nonce));

    if (sec_level == ZB_ZGP_SEC_LEVEL_FULL_WITH_ENC)
    {
        zb_ccm_encrypt_n_auth(g_key_nwk, (zb_uint8_t *)&nonce,
                              pseudo_header, hdr_len,
                              payload, pld_len,
                              SEC_CTX().encryption_buf2);

        ZB_MEMCPY(ZB_BUF_BEGIN(packet),
                  ZB_BUF_BEGIN(SEC_CTX().encryption_buf2) + hdr_len,
                  pld_len);
        ZB_MEMCPY(gpdf_info->mic,
                  ZB_BUF_BEGIN(SEC_CTX().encryption_buf2) + hdr_len + pld_len,
                  4);
    }
    else
    {
        hdr_len += pld_len;
        zb_ccm_encrypt_n_auth(g_key_nwk, (zb_uint8_t *)&nonce,
                              pseudo_header, hdr_len,
                              NULL, 0,
                              SEC_CTX().encryption_buf2);

        ZB_MEMCPY(gpdf_info->mic,
                  ZB_BUF_BEGIN(SEC_CTX().encryption_buf2) + hdr_len,
                  ZB_CCM_M);
    }

    TRACE_MSG(TRACE_ERROR, "ENCRYPT: mic = 0x%x", (FMT__L, gpdf_info->mic));
}

static void construct_aes_nonce(
    zb_bool_t    from_gpd,
    zb_zgpd_id_t *zgpd_id,
    zb_uint32_t  frame_counter,
    zb_zgp_aes_nonce_t *res_nonce)
{
    TRACE_MSG(TRACE_SECUR2, ">> construct_aes_nonce, from_gpd %hd, zgpd_id %p, frame_counter %d, "
              "res_nonce %p", (FMT__H_P_D, from_gpd, zgpd_id, frame_counter, res_nonce));

    ZB_BZERO(res_nonce, sizeof(zb_zgp_aes_nonce_t));

    if (zgpd_id->app_id == ZB_ZGP_APP_ID_0000)
    {
        zb_uint32_t src_id_le;

        ZB_HTOLE32(&src_id_le, &zgpd_id->addr.src_id);

        if (from_gpd)
        {
            res_nonce->src_addr.splitted_addr[0] = src_id_le;
        }
        else
        {
            res_nonce->src_addr.splitted_addr[0] = 0;
        }

        res_nonce->src_addr.splitted_addr[1] = src_id_le;
    }
    else
    {
        /* zgpd_id->addr.ieee_addr is stored as LE, so res_nonce->src_addr will be LE also */
        ZB_64BIT_ADDR_COPY(&res_nonce->src_addr.ieee_addr, &zgpd_id->addr.ieee_addr);
    }

    ZB_HTOLE32(&res_nonce->frame_counter, &frame_counter);

    if ((zgpd_id->app_id == ZB_ZGP_APP_ID_0010) && (!from_gpd))
    {
        res_nonce->security_control = 0xA3;
    }
    else
    {
        res_nonce->security_control = 0x05;
    }

    TRACE_MSG(TRACE_SECUR2, "<< construct_aes_nonce", (FMT__0));
}

/*============================================================================*/
/*                          STARTUP                                           */
/*============================================================================*/

static void zgpc_custom_startup()
{
    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("th_gpct");

    zb_set_long_address(g_th_gpp_addr);
    zb_set_network_coordinator_role(1l << TEST_CHANNEL);
    zb_secur_setup_nwk_key(g_key_nwk, 0);
    ZB_NIB_SET_USE_MULTICAST(ZB_FALSE);

    /* Need to block GPDF recv directly */
#ifdef ZB_ZGP_SKIP_GPDF_ON_NWK_LAYER
    ZG->nwk.skip_gpdf = 0;
#endif
#ifdef ZB_ZGP_RUNTIME_WORK_MODE_WITH_PROXIES
    ZGP_CTX().enable_work_with_proxies = 1;
#endif
#ifdef ZB_CERTIFICATION_HACKS
    ZB_CERT_HACKS().ccm_check_cb = NULL;
#endif

    ZGP_CTX().device_role = ZGP_DEVICE_COMMISSIONING_TOOL;
    //ZGP_CTX().device_role = ZGP_DEVICE_PROXY_BASIC;
    ZGP_GP_SHARED_SECURITY_KEY_TYPE = ZB_ZGP_SEC_KEY_TYPE_NWK;
    zb_zdo_register_device_annce_cb(dev_annce_cb);
}

ZB_ZGPC_DECLARE_STARTUP_PROCESS()

#else // defined ZB_ENABLE_HA

#include <stdio.h>
MAIN()
{
    ARGV_UNUSED;

    printf("HA profile should be enabled in zb_config.h\n");

    MAIN_RETURN(1);
}

#endif
