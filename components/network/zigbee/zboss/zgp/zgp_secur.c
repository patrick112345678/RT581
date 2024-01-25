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
/* PURPOSE: ZGP security utilities
*/

#define ZB_TRACE_FILE_ID 2103
#include "zb_common.h"

#if defined ZB_ENABLE_ZGP_SECUR && defined ZB_ENABLE_ZGP
#include "zb_secur.h"
#include "zgp/zgp_internal.h"

zb_uint8_t form_secur_header(
    zb_gpdf_info_t *gpdf_info,
    zb_bufid_t      buf,
    zb_uint8_t     **hdr_to_secure)
{
    zb_uint8_t ret;
    zb_uint8_t *hdr_ptr;
    zb_uint8_t sec_level = ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl);

    TRACE_MSG(TRACE_SECUR2, ">> form_secur_header, gpdf_info %p, buf %hd, hdr_to_secure %p",
              (FMT__P_H_P, gpdf_info, buf, hdr_to_secure));
    TRACE_MSG(TRACE_SECUR2, "GPDF sec_level  %hd", (FMT__H, sec_level));

    ret = sizeof(gpdf_info->nwk_frame_ctl) +
          sizeof(gpdf_info->nwk_ext_frame_ctl) +
          ((gpdf_info->zgpd_id.app_id == ZB_ZGP_APP_ID_0000) ?
           sizeof(gpdf_info->zgpd_id.addr.src_id) :
           ((gpdf_info->zgpd_id.app_id == ZB_ZGP_APP_ID_0010) ?
            sizeof(gpdf_info->zgpd_id.endpoint) : 0));

    if (sec_level == ZB_ZGP_SEC_LEVEL_REDUCED)
    {
        TRACE_MSG(TRACE_SECUR2, "gpdf_info->mac_addr_flds_len %d", (FMT__D, gpdf_info->mac_addr_flds_len));
        ret += sizeof(gpdf_info->mac_seq_num) + gpdf_info->mac_addr_flds_len;
    }
    else
    {
        ret += sizeof(gpdf_info->sec_frame_counter);
    }

    TRACE_MSG(TRACE_SECUR2, "ret %d", (FMT__D, ret));

    hdr_ptr = zb_buf_begin(buf);

    if (sec_level == ZB_ZGP_SEC_LEVEL_REDUCED)
    {
        hdr_ptr = zb_buf_alloc_left(buf, gpdf_info->mac_addr_flds_len + sizeof(gpdf_info->mac_seq_num));

        *hdr_ptr = gpdf_info->sec_frame_counter & 0xFF;

        ZB_MEMCPY(hdr_ptr + 1, &gpdf_info->mac_addr_flds, gpdf_info->mac_addr_flds_len);
        zb_buf_cut_left(buf, gpdf_info->mac_addr_flds_len + sizeof(gpdf_info->mac_seq_num));
    }

    *hdr_to_secure = hdr_ptr;

    TRACE_MSG(TRACE_SECUR2, "<< form_secur_header, ret %d", (FMT__D, ret));

    return ret;
}


static void construct_aes_nonce(
    zb_bool_t    from_gpd,
    zb_zgpd_id_t *zgpd_id,
    zb_uint32_t  frame_counter,
    zb_zgp_aes_nonce_t *res_nonce)
{
    TRACE_MSG(TRACE_SECUR2, ">> construct_aes_nonce, from_gpd %hd, zgpd_id %p, frame_counter %hd, "
              "res_nonce %p", (FMT__H_P_H_P, from_gpd, zgpd_id, frame_counter, res_nonce));

    ZB_BZERO(res_nonce, sizeof(zb_zgp_aes_nonce_t));

    if (zgpd_id->app_id == ZB_ZGP_APP_ID_0000)
    {
        zb_uint32_t src_id_le;

        ZB_HTOLE32((zb_uint8_t *)&src_id_le, (zb_uint8_t *)&zgpd_id->addr.src_id);

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

    ZB_HTOLE32((zb_uint8_t *)&res_nonce->frame_counter, (zb_uint8_t *)&frame_counter);

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


zb_ret_t zb_zgp_protect_frame(
    zb_gpdf_info_t *gpdf_info,
    zb_uint8_t *key,
    zb_bufid_t packet)
{
    zb_ret_t ret = RET_OK;
    zb_uint8_t sec_level = ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl);
    zb_uint8_t *payload = (zb_uint8_t *)zb_buf_begin(packet) + gpdf_info->nwk_hdr_len;
    zb_int8_t pld_len = zb_buf_len(packet) - gpdf_info->nwk_hdr_len;
    zb_int8_t hdr_len;
    zb_uint8_t *header;
    zb_zgp_aes_nonce_t nonce;
    zb_uint8_t *ptr;
    //zb_uint8_t sl = sec_level;

    TRACE_MSG(TRACE_SECUR2, ">> zb_zgp_protect_frame, gpdf_info %p, key %p, sec_level %d, packet %p",
              (FMT__P_P_D_P, gpdf_info, key, sec_level, packet));
    TRACE_MSG(TRACE_ZGP3, "key: " TRACE_FORMAT_128, (FMT__A_A, TRACE_ARG_128(key)));

    if (sec_level == ZB_ZGP_SEC_LEVEL_NO_SECURITY)
    {
        TRACE_MSG(TRACE_SECUR2, "No security required, return", (FMT__0));
        TRACE_MSG(TRACE_SECUR2, "<< zb_zgp_protect_frame, ret %hd", (FMT__H, RET_OK));
        return RET_OK;
    }

    hdr_len = form_secur_header(gpdf_info, packet, &header);

    construct_aes_nonce(
        (zb_bool_t)!ZB_GPDF_EXT_NFC_GET_DIRECTION(gpdf_info->nwk_ext_frame_ctl),
        &gpdf_info->zgpd_id,
        gpdf_info->sec_frame_counter,
        &nonce);

    /*  #ifdef ZB_CERTIFICATION_HACKS
          sl = ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_REPLACE_SEC_LEVEL)?ZGPD->ch_replace_sec_level:sec_level;
      #endif*/
    //  if( sec_level>ZB_ZGP_SEC_LEVEL_FULL_NO_ENC )
    {
        if (sec_level == ZB_ZGP_SEC_LEVEL_FULL_WITH_ENC)
        {
            ret = zb_ccm_encrypt_n_auth(key, (zb_uint8_t *)&nonce, header, hdr_len, payload, pld_len,
                                        SEC_CTX().encryption_buf2);

            ptr = zb_buf_alloc_right(packet, ZB_CCM_M);
            ZVUNUSED(ptr);

            /* Allocation may shift start of the payload, so don't use payload var */

            ZB_MEMCPY((zb_uint8_t *)zb_buf_begin(packet) + gpdf_info->nwk_hdr_len,
                      (zb_uint8_t *)zb_buf_begin(SEC_CTX().encryption_buf2)
                      + hdr_len, pld_len + ZB_CCM_M);
        }
        else
        {
            zb_uint16_t mic_length;

            hdr_len += pld_len;

            ret = zb_ccm_encrypt_n_auth(key, (zb_uint8_t *)&nonce, header, hdr_len, NULL, 0,
                                        SEC_CTX().encryption_buf2);

            mic_length  = (sec_level > ZB_ZGP_SEC_LEVEL_REDUCED) ? ZB_CCM_M : 2;

            ptr = zb_buf_alloc_right(packet, mic_length);

            ZB_MEMCPY(ptr, (zb_uint8_t *)zb_buf_begin(SEC_CTX().encryption_buf2) + hdr_len, mic_length);
        }
    }

    TRACE_MSG(TRACE_SECUR2, "<< zb_zgp_protect_frame, ret %hd", (FMT__H, ret));

    return ret;
}

#ifdef ZB_ENABLE_ZGP_TEST_HARNESS
zb_ret_t zb_zgp_protect_out_gpdf(zb_uint8_t               buf_ref,
                                 zb_outgoing_gpdf_info_t *gpdf_info,
                                 zb_uint8_t              *key,
                                 zb_uint8_t               nwk_hdr_len)
{
    zb_ret_t   ret = RET_OK;
    zb_uint8_t sec_level = ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl);

    TRACE_MSG(TRACE_SECUR2, ">> zb_zgp_protect_out_gpdf", (FMT__0));
    TRACE_MSG(TRACE_SECUR3, "key: " TRACE_FORMAT_128, (FMT__A_A, TRACE_ARG_128(key)));

    ZB_ASSERT(sec_level != ZB_ZGP_SEC_LEVEL_REDUCED);

    if (sec_level == ZB_ZGP_SEC_LEVEL_NO_SECURITY)
    {
        TRACE_MSG(TRACE_SECUR2, "No security required", (FMT__0));
    }
    else
    {
        zb_zgpd_id_t        zgpd_id;
        zb_zgp_aes_nonce_t  nonce;
        zb_uint8_t         *hdr_ptr = zb_buf_begin(param);
        zb_uint8_t         *ptr;
        zgp_tbl_ent_t       ent;

        ZB_MAKE_ZGPD_ID(zgpd_id,
                        (ZB_GPDF_NFC_GET_FRAME_TYPE(gpdf_info->nwk_frame_ctl) != ZGP_FRAME_TYPE_MAINTENANCE ? ZB_GPDF_EXT_NFC_GET_APP_ID(gpdf_info->nwk_ext_frame_ctl) : ZB_ZGP_APP_ID_0000),
                        gpdf_info->endpoint,
                        gpdf_info->addr);

        if (key == NULL)
        {
            ret = zgp_any_table_read(&zgpd_id, &ent);

            if (ret == RET_OK)
            {
                key = ent.zgpd_key;
            }
        }

        if (key)
        {
            construct_aes_nonce(
                (zb_bool_t)!ZB_GPDF_EXT_NFC_GET_DIRECTION(gpdf_info->nwk_ext_frame_ctl),
                &zgpd_id,
                gpdf_info->sec_frame_counter,
                &nonce);

            if (sec_level == ZB_ZGP_SEC_LEVEL_FULL_WITH_ENC)
            {
                zb_uint8_t *payload = hdr_ptr + nwk_hdr_len;

                ret = zb_ccm_encrypt_n_auth(key, (zb_uint8_t *)&nonce, hdr_ptr, nwk_hdr_len,
                                            payload, gpdf_info->payload_len,
                                            SEC_CTX().encryption_buf2);
                ZB_BUF_ALLOC_RIGHT(buf, ZB_CCM_M, ptr);

                /* Allocation may shift start of the payload, so don't use payload var */
                ZB_MEMCPY(zb_buf_begin(param) + nwk_hdr_len,
                          zb_buf_begin(SEC_CTX().encryption_buf2) + nwk_hdr_len,
                          gpdf_info->payload_len + ZB_CCM_M);
            }
            else
            {
                nwk_hdr_len += gpdf_info->payload_len;
                ret = zb_ccm_encrypt_n_auth(key, (zb_uint8_t *)&nonce, hdr_ptr, nwk_hdr_len, NULL, 0,
                                            SEC_CTX().encryption_buf2);
                ZB_BUF_ALLOC_RIGHT(buf, ZB_CCM_M, ptr);
                ZB_MEMCPY(ptr, zb_buf_begin(SEC_CTX().encryption_buf2) + nwk_hdr_len, ZB_CCM_M);
            }
        }
    }

    TRACE_MSG(TRACE_SECUR2, "<< zb_zgp_protect_out_gpdf, ret %hd", (FMT__H, ret));

    return ret;
}
#endif  /* ZB_ENABLE_ZGP_TEST_HARNESS */

/**
   Security processesing of the received GPDF.

   Implemented according to A.1.5.3.5 Incoming frames decryption and
   authentication check

*/
zb_ret_t zb_zgp_decrypt_and_auth(zb_uint8_t param)
{
    zb_bufid_t packet = param;
    zb_gpdf_info_t *gpdf_info = ZB_BUF_GET_PARAM(packet, zb_gpdf_info_t);
    zb_ret_t ret = RET_OK;
    zb_uint8_t sec_level = ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl);
    zb_uint16_t mic_length = (sec_level > ZB_ZGP_SEC_LEVEL_REDUCED) ? ZB_CCM_M : 2;
    zb_int8_t pld_len = zb_buf_len(packet) - gpdf_info->nwk_hdr_len - mic_length;
    zb_int8_t hdr_len = 0;
    zb_uint8_t *hdr = zb_buf_begin(packet);
    zb_zgp_aes_nonce_t nonce;

    TRACE_MSG(TRACE_SECUR2, ">> zb_zgp_decrypt_and_auth param %hd", (FMT__H, param));

    /* Determine the security level, as described in A.1.5.2.2, and perform initialization, as described in A.1.5.3.3.  */

    ZB_ASSERT(sec_level > ZB_ZGP_SEC_LEVEL_NO_SECURITY);
    hdr_len = form_secur_header(gpdf_info, packet, &hdr);

    construct_aes_nonce(
        (zb_bool_t)!ZB_GPDF_EXT_NFC_GET_DIRECTION(gpdf_info->nwk_ext_frame_ctl),
        &gpdf_info->zgpd_id,
        gpdf_info->sec_frame_counter,
        &nonce);

    TRACE_MSG(TRACE_ZGP3, "key: " TRACE_FORMAT_128, (FMT__A_A, TRACE_ARG_128(gpdf_info->key)));

    if (pld_len < 0)
    {
        TRACE_MSG(TRACE_ZGP1, "pld len incorrect: %d, drop packet", (FMT__D, pld_len));
        return RET_OUT_OF_RANGE;
    }

    if (sec_level == ZB_ZGP_SEC_LEVEL_FULL_WITH_ENC)
    {
        /* From ZGP spec, A.1.5.4.4:
         * If decryption is required (SecurityLevel 0b11), proceed with CCM* as specified in
         * A.2.3 of Zigbee core spec, by using
         * PlaintextData = encrypted GPD CommandID || encrypted GPD Command Payload from the
         * received GPDF.
         */
        ret = zb_ccm_decrypt_n_auth(gpdf_info->key, (zb_uint8_t *)&nonce,
                                    packet,
                                    hdr_len,
                                    pld_len + ZB_CCM_M
                                   );

        ZB_MEMCPY(&gpdf_info->mic, hdr + hdr_len + pld_len, mic_length);

        if (ret == RET_OK)
        {
            /* update zgpd_cmd_id! */
            gpdf_info->zgpd_cmd_id = *((zb_uint8_t *)zb_buf_begin(packet) + hdr_len);
            TRACE_MSG(TRACE_SECUR3, "zgpd_cmd_id: 0x%02x", (FMT__H, gpdf_info->zgpd_cmd_id));
        }
        else
        {
            TRACE_MSG(TRACE_SECUR1, "MIC authentication failed", (FMT__0));
        }
    }
    else
    {
        ret = zb_ccm_encrypt_n_auth(gpdf_info->key, (zb_uint8_t *)&nonce,
                                    hdr, hdr_len + pld_len, NULL, 0, SEC_CTX().encryption_buf2);

        if (ret == RET_OK)
        {
            if (TRACE_ENABLED(TRACE_SECUR3))
            {
                zb_uint32_t mic1;
                zb_uint32_t mic2;

                ZB_MEMCPY(&mic1, hdr + hdr_len + pld_len, sizeof(mic1));
                ZB_MEMCPY(&mic2, (zb_uint8_t *)zb_buf_begin(SEC_CTX().encryption_buf2) + hdr_len + pld_len, sizeof(mic2));

                TRACE_MSG(TRACE_SECUR3, "MIC1[0x%02x], MIC2[0x%02x]", (FMT__D_D, mic1, mic2));
            }

            ZB_MEMCPY(&gpdf_info->mic, hdr + hdr_len + pld_len, mic_length);
            if (!ZB_MEMCMP(
                        hdr + hdr_len + pld_len, /* MIC in the packet */
                        (zb_uint8_t *)zb_buf_begin(SEC_CTX().encryption_buf2) + hdr_len + pld_len, /* calculated MIC */
                        mic_length))
            {
                ret = RET_OK;
                zb_buf_cut_right(packet, mic_length);
            }
            else
            {
                ret = RET_ERROR;
                TRACE_MSG(TRACE_SECUR1, "MIC authentication failed", (FMT__0));
            }
        }
    }

    TRACE_MSG(TRACE_SECUR2, "<< zb_zgp_decrypt_and_auth, ret %hd", (FMT__H, ret));

    return ret;
}


void zb_zgp_protect_gpd_key(
    zb_bool_t from_gpd,
    zb_zgpd_id_t *zgpd_id,
    zb_uint8_t *key_to_encrypt,
    zb_uint8_t *key_encrypt_with,
    zb_uint8_t *crypted_key,
    zb_uint32_t security_frame_counter,
    zb_uint8_t *mic)
{
    zb_zgp_aes_nonce_t nonce;
    zb_uint32_t nonce_fc;
    zb_uint8_t header[4];

    TRACE_MSG(TRACE_SECUR2, ">> zb_zgp_protect_gpd_key from_gpd %hd zgpd_id %p "
              "key_to_encrypt %p key_encrypt_with %p crypted_key %p mic %p",
              (FMT__H_P_P_P_P_P, from_gpd, zgpd_id, key_to_encrypt, key_encrypt_with, crypted_key, mic));

    /* Constructing header */
    if (zgpd_id->app_id == ZB_ZGP_APP_ID_0000)
    {
        ZB_HTOLE32(header, (zb_uint8_t *)&zgpd_id->addr.src_id);
    }
    else
    {
        ZB_MEMCPY(header, zgpd_id->addr.ieee_addr, 4);
    }

    /* Constructing nonce */
    if (from_gpd)
    {
        if (zgpd_id->app_id == ZB_ZGP_APP_ID_0000)
        {
            nonce_fc = zgpd_id->addr.src_id;
        }
        else
        {
            ZB_LETOH32(&nonce_fc, zgpd_id->addr.ieee_addr);
        }
    }
    else
    {
        nonce_fc = security_frame_counter + 1;
        TRACE_MSG(TRACE_SECUR2, "INFRA:Key is sent to ZGPD.nonce_fc=0x%08x", (FMT__L, nonce_fc));
    }

    construct_aes_nonce(from_gpd, zgpd_id, nonce_fc, &nonce);

    /* Ignore ret code */
    zb_ccm_encrypt_n_auth(key_encrypt_with, (zb_uint8_t *)&nonce,
                          header, 4, key_to_encrypt, ZB_CCM_KEY_SIZE, SEC_CTX().encryption_buf2);

    ZB_MEMCPY(crypted_key, (zb_uint8_t *)zb_buf_begin(SEC_CTX().encryption_buf2) + 4, ZB_CCM_KEY_SIZE);
    ZB_MEMCPY(mic, (zb_uint8_t *)zb_buf_begin(SEC_CTX().encryption_buf2) + 4 + ZB_CCM_KEY_SIZE, ZB_CCM_M);

    TRACE_MSG(TRACE_SECUR2, "<< zb_zgp_protect_gpd_key", (FMT__0));
}


zb_ret_t zb_zgp_decrypt_n_auth_gpd_key(
    zb_bool_t from_gpd,
    zb_zgpd_id_t *zgpd_id,
    zb_uint8_t *key_decrypt_with,
    zb_uint8_t *crypted_key,
    zb_uint32_t security_frame_counter,
    zb_uint8_t *mic,
    zb_uint8_t *plain_key)
{
    zb_ret_t ret;
    zb_zgp_aes_nonce_t nonce;
    zb_uint32_t nonce_fc;
    zb_uint8_t header[4];

    TRACE_MSG(TRACE_SECUR2, ">> zb_zgp_decrypt_n_auth_gpd_key from_gpd %hd zgpd_id %p "
              "key_decrypt_with %p crypted_key %p mic %p, plain_key %p",
              (FMT__H_P_P_P_P_P, from_gpd, zgpd_id, key_decrypt_with, crypted_key, mic, plain_key));

    /* Constructing header */
    if (zgpd_id->app_id == ZB_ZGP_APP_ID_0000)
    {
        ZB_HTOLE32(header, (zb_uint8_t *)&zgpd_id->addr.src_id);
    }
    else
    {
        ZB_MEMCPY(header, zgpd_id->addr.ieee_addr, 4);
    }

    /* Constructing nonce */
    if (from_gpd)
    {
        TRACE_MSG(TRACE_ZGP3, "Packet from GPD, use src_id or ieee_addr", (FMT__0));
        if (zgpd_id->app_id == ZB_ZGP_APP_ID_0000)
        {
            nonce_fc = zgpd_id->addr.src_id;
        }
        else
        {
            ZB_LETOH32(&nonce_fc, zgpd_id->addr.ieee_addr);
        }
    }
    else
    {
        nonce_fc = security_frame_counter;
    }

    construct_aes_nonce(from_gpd, zgpd_id, nonce_fc, &nonce);

    {
        zb_bufid_t  buf = SEC_CTX().encryption_buf2;
        zb_uint8_t *ptr;

        zb_buf_reuse(buf);
        ptr = zb_buf_alloc_left(buf, /*header*/4 + /*crypted_key*/ZB_CCM_KEY_SIZE + /*mic*/ZB_CCM_M);
        /* Construct buffer for ccm_decrypt_n_auth */
        ZB_MEMCPY(ptr, header, 4);
        ptr += 4;
        ZB_MEMCPY(ptr, crypted_key, ZB_CCM_KEY_SIZE);
        ptr += ZB_CCM_KEY_SIZE;
        ZB_MEMCPY(ptr, mic, ZB_CCM_M);
    }

    ret = zb_ccm_decrypt_n_auth(key_decrypt_with, (zb_uint8_t *)&nonce,
                                SEC_CTX().encryption_buf2, 4, ZB_CCM_KEY_SIZE + ZB_CCM_M );

    ZB_MEMCPY(plain_key, (zb_uint8_t *)zb_buf_begin(SEC_CTX().encryption_buf2) + 4, ZB_CCM_KEY_SIZE);

    TRACE_MSG(TRACE_SECUR2, "<< zb_zgp_decrypt_n_auth_gpd_key, ret %hd", (FMT__H, ret));

    return ret;
}

/**
 *  ZigBee Keyed Hash Function.
 *
 * Described in ZigBee specification
 *      section B.1.4, and in FIPS Publication 198. Same as zb_cmm_key_hash
 *      but supports up to 8-bytes input strings, that's Zigbee GreenPower uses.
 *
 *      This function implements the hash function:
 *          Hash(Key, text) = H((Key XOR opad) || H((Key XOR ipad) || text));
 *          ipad = 0x36 repeated.
 *          opad = 0x5c repeated.
 *          H() = Zigbee Cryptographic Hash (B.1.3 and B.6).
 *
 *      The output of this function is an ep_alloced buffer containing
 *      the key-hashed output, and is garaunteed never to return NULL.
 *
 *      @param  *key    - ZigBee Security Key (must be ZBEE_SEC_CONST_KEYSIZE) in length.
 *      @param  input   - text
 *      @param  input_len - text len
 *      @return hask_key
 *---------------------------------------------------------------
 */
void zb_zgp_key_hash(zb_uint8_t *key, zb_uint8_t *input, zb_uint8_t input_len, zb_uint8_t *hash_key)
{
    zb_uint8_t  hash_in[2 * ZB_CCM_KEY_SIZE];
    zb_uint8_t  hash_out[ZB_CCM_KEY_SIZE + 8]; /*as 8 is the maximum text message in current specification (ieee address)*/
    zb_ushort_t i;
#define ipad  0x36
#define opad  0x5c

    ZVUNUSED(hash_key);

    for (i = 0; i < ZB_CCM_KEY_SIZE; i++)
    {
        /* Copy the key into hash_in and XOR with opad to form: (Key XOR opad) */
        hash_in[i] = key[i] ^ opad;
        /* Copy the Key into hash_out and XOR with ipad to form: (Key XOR ipad) */
        hash_out[i] = key[i] ^ ipad;
        /* Append the input byte to form: (Key XOR ipad) || text. */
        if ( i < input_len )
        {
            hash_out[ZB_CCM_KEY_SIZE + i] = input[i];
        }
    }
    /* Hash the contents of hash_out and append the contents to hash_in to
     * form: (Key XOR opad) || H((Key XOR ipad) || text).
     */
    zb_sec_b6_hash(hash_out, ZB_CCM_KEY_SIZE + input_len, hash_in + ZB_CCM_KEY_SIZE);
    /* Hash the contents of hash_in to get the final result. */
    zb_sec_b6_hash(hash_in, 2 * ZB_CCM_KEY_SIZE, hash_out);
    ZB_MEMCPY(hash_key, hash_out, ZB_CCM_KEY_SIZE);
} /* zb_zgp_key_hash */

#ifndef ZB_ZGPD_ROLE

/*FUNCTION:------------------------------------------------------
 *  NAME
 *      zb_zgp_key_gen
 *  DESCRIPTION
 *      Zigbee Key Generator Function. See A.3.7.1.2 Zigbee GP specification
 *      Generate key according to given type as in Table 48.
 *      Used by sink to provide keys for GP devices.
 *
 *      The output of this function is a key.
 *      the key-hashed output, and is garaunteed never to return NULL.
 *  PARAMETERS
 *      enum zb_zgp_security_key_type_e security_key_type - Key type according enum zb_zgp_security_key_type_e (zb_zgp.h)
 *      zb_zgpd_id_t *zgpd_id - zgpd_if of GPD
 *      zb_uint8_t  *oob    - GPD out-of-the-box key (must be ZBEE_SEC_CONST_KEYSIZE) in length.
 *      zb_uint8_t  *key    - Output Key (must be ZBEE_SEC_CONST_KEYSIZE) in length.
 *  RETURNS
 *      zb_uint8_t  status: RET_OK or RET_ERROR
 *
 *  NOTES   zgpd_id needed only if ZB_ZGP_SEC_KEY_TYPE_DERIVED_INDIVIDUAL
 *          oob   required only if ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL
 *---------------------------------------------------------------
 */
zb_ret_t zb_zgp_key_gen(enum zb_zgp_security_key_type_e security_key_type, zb_zgpd_id_t *zgpd_id, zb_uint8_t *oob, zb_uint8_t *key)
{
    zb_int_t ret = RET_OK;
    zb_uint8_t *src_key = secur_nwk_key_by_seq(ZB_NIB().active_key_seq_number);
    TRACE_MSG(TRACE_ZGP3, "src_key: " TRACE_FORMAT_128, (FMT__A_A, TRACE_ARG_128(src_key)));
    TRACE_MSG(TRACE_ZGP3, ">>> zb_zgp_zgpd_key_gen", (FMT__0));
    switch (security_key_type)
    {
    case ZB_ZGP_SEC_KEY_TYPE_NO_KEY:
    {
        TRACE_MSG(TRACE_ZGP3, "ZB_ZGP_SEC_KEY_TYPE_NO_KEY, copy OOB key:" TRACE_FORMAT_128, (FMT__A_A, TRACE_ARG_128(src_key)));
        ZB_MEMCPY(key, oob, ZB_CCM_KEY_SIZE);
    }
    break;
    case ZB_ZGP_SEC_KEY_TYPE_NWK:
    {
        TRACE_MSG(TRACE_ZGP3, "ZB_ZGP_SEC_KEY_TYPE_NWK, copy NWK key:" TRACE_FORMAT_128, (FMT__A_A, TRACE_ARG_128(src_key)));
        ZB_MEMCPY(key, src_key, ZB_CCM_KEY_SIZE);
    }
    break;
    case ZB_ZGP_SEC_KEY_TYPE_GROUP:
    {
        TRACE_MSG(TRACE_ZGP3, "ZB_ZGP_SEC_KEY_TYPE_GROUP, copy gp_shared_security_key", (FMT__0));
        ZB_MEMCPY(key, ZGP_CTXC().cluster.gp_shared_security_key, ZB_CCM_KEY_SIZE);
    }
    break;
    case ZB_ZGP_SEC_KEY_TYPE_GROUP_NWK_DERIVED:
    {
        zb_uint8_t inp[3] = {90, 71, 80}; /*ZGP*/
        TRACE_MSG(TRACE_ZGP3, "ZB_ZGP_SEC_KEY_TYPE_GROUP_NWK_DERIVED, derive from NWK key", (FMT__0));
        zb_zgp_key_hash(src_key, inp, 3, key);
    }
    break;
    case ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL:
    {
        /*out-of-the-box key*/
        TRACE_MSG(TRACE_ZGP3, "ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL, copy oob key", (FMT__0));
        ZB_MEMCPY(key, oob, ZB_CCM_KEY_SIZE);
    }
    break;
    case ZB_ZGP_SEC_KEY_TYPE_DERIVED_INDIVIDUAL:
    {
        /* see A.3.7.1.2.2   Individual GPD key derivation
          The HMAC keyed hash function, as defined in [16],
          is used to derive the individual GPD key.
          KGPD ID= HMAC(K, ID)16
          whereby the block size  B, the length of the key K and the output size t (of the individual key KGPD ID)
                    are all 128 bit/16 octets;
                  the Matyas-Meyer-Oseas hash function, as defined in [1] section B.6, is used as the hash function H;
                  the ID is:
                    *for GPD using ApplicationID = 0b010, i.e. identified by IEEE address: 8B GPD IEEE address
                    is used as the text input, in little endian order
                    (e.g. 0x11 0xff 0xee 0xdd 0xcc 0xbb 0xaa 0x00 for IEEE address 00:aa:bb:cc:dd:ee:ff:11);
                    the Endpoint field SHALL NOT be used;
                    *for GPD using ApplicationID = 0b000, i.e. identified by SrcID: 4B GPD SrcID is used as the
                    text input, in little endian order (e.g. 0x21 0x43 0x65 0x87 for SrcID=0x87654321);
                  the GPD group key (0x010) as stored in the  gpSharedSecurityKey
                    attribute (see sec. A.3.3.3.2) is used as the key K.
          Implementation of key derivation is only mandatory for the sink;
            the proxies receive the correct key in the GP Pairing command. */
        zb_uint8_t id[8];
        zb_uint8_t id_size = 0;
        TRACE_MSG(TRACE_ZGP3, "ZB_ZGP_SEC_KEY_TYPE_DERIVED_INDIVIDUAL, derive from shared group key", (FMT__0));
        ZB_ASSERT(ZGP_CTXC().cluster.gp_shared_security_key_type == ZB_ZGP_SEC_KEY_TYPE_DERIVED_INDIVIDUAL);
        switch (zgpd_id->app_id)
        {
        case ZB_ZGP_APP_ID_0010:
        {
            ZB_MEMCPY(id, &zgpd_id->addr.ieee_addr, 8);
            ZB_ZCL_FIX_ENDIAN(id, ZB_ZCL_ATTR_TYPE_IEEE_ADDR);
            id_size = 8;
        }
        break;
        case ZB_ZGP_APP_ID_0000:
        {
            ZB_MEMCPY(id, (zb_uint8_t *)&zgpd_id->addr.src_id, 4);
            ZB_ZCL_FIX_ENDIAN(id, ZB_ZCL_ATTR_TYPE_U32);
            id_size = 4;
        }
        break;
        default:
            ZB_ASSERT(0);
            break;
        }
        ZB_ASSERT(id_size > 0);
        {
            zb_uint8_t tmp_key[ZB_CCM_KEY_SIZE];
            ZB_MEMCPY(tmp_key, ZGP_CTXC().cluster.gp_shared_security_key, ZB_CCM_KEY_SIZE);
            TRACE_MSG(TRACE_ZGP3, "gp_shared_security_key: " TRACE_FORMAT_128, (FMT__A_A, TRACE_ARG_128(tmp_key)));
            {
                int i;
                for (i = 0; i < id_size; i++)
                {
                    TRACE_MSG(TRACE_ZGP3, "ID[%d]:%hx", (FMT__D_H, i, id[i]));
                }
            }
            zb_zgp_key_hash(tmp_key, id, id_size, key);
        }
    }
    break;
    default:
    {
        TRACE_MSG(TRACE_ZGP3, "ERROR: ZB_ZGP_SEC_KEY_TYPE=0x%hx NOT SUPPORTED", (FMT__H, security_key_type));
        ret = RET_ERROR;
    }
    break;
    }
    TRACE_MSG(TRACE_ZGP3, "<< zb_zgp_zgpd_key_gen, ret:%d", (FMT__D, ret));
    return ret;
}

#endif /* !ZB_ZGPD_ROLE */

#endif /* ZB_ENABLE_ZGP_SECUR */
