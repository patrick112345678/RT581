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
/* PURPOSE: CCM* routines - to be used when HW is not available - that is, in Linux/ns-3
*/


/*! \addtogroup ZB_SECUR */
/*! @{ */


/* constants are hard-coded for Standard security */

#define ZB_TRACE_FILE_ID 2466
#include "zb_common.h"
#include "zb_types.h"
#include "zb_config.h"
#include "zb_secur.h"


static void zb_htobe16(void *dst, void *src)
{
    ZB_HTOBE16(dst, src);
}


#ifdef ZB_SOFT_SECURITY

static void xor16(zb_uint8_t *v1, zb_uint8_t *v2, zb_uint8_t *result);

void encrypt_trans(
    zb_ushort_t ccm_m,
    zb_uint8_t *key,
    zb_uint8_t *nonce,
    zb_uint8_t *string_m,
    zb_uint32_t string_m_len,
    zb_uint8_t *t,
    zb_uint8_t *encrypted);


zb_ret_t
zb_ccm_encrypt_n_auth(
    zb_uint8_t *key,
    zb_uint8_t *nonce,
    zb_uint8_t *string_a,
    zb_ushort_t string_a_len,
    zb_uint8_t *string_m,
    zb_uint32_t string_m_len,
    zb_bufid_t crypted_text)
{
    zb_uint8_t t[16];
    zb_uint8_t *c_text;

    TRACE_MSG(TRACE_SECUR3, ">> zb_ccm_encrypt_n_auth", (FMT__0));

    /* A.2.2Authentication Transformation */
    /* First ccm_m bytes of t now holds tag T */
    zb_ccm_auth_trans(ZB_CCM_M, key, nonce, string_a, string_a_len, string_m, string_m_len, t);

#ifdef ZB_CERTIFICATION_HACKS
    if (ZB_CERT_HACKS().ccm_check_cb)
    {
        zb_bool_t wrap_back = ZB_FALSE;
        /* User callback can change mic if application wants it.
         * See test bdb/cs-nfs-tc-01.
         */
        wrap_back = ZB_CERT_HACKS().ccm_check_cb(t, string_a, string_a_len);
        if (wrap_back == ZB_TRUE)
        {
            /* Application decide to stop calling this callback in future */
            ZB_CERT_HACKS().ccm_check_cb = NULL;
        }
    }
#endif

    c_text = zb_buf_initial_alloc(crypted_text, string_a_len + string_m_len + ZB_CCM_M);
    /* A.2.3 Encryption Transformation */
    encrypt_trans(ZB_CCM_M, key, nonce, string_m, string_m_len, t, c_text + string_a_len);
    ZB_MEMCPY(zb_buf_begin(crypted_text), string_a, string_a_len);

    TRACE_MSG(TRACE_SECUR3, "<< zb_ccm_encrypt_n_auth", (FMT__0));
    return RET_OK;
}


zb_ret_t
zb_ccm_decrypt_n_auth(
    zb_uint8_t *key,
    zb_uint8_t *nonce,
    zb_bufid_t buf,
    zb_ushort_t string_a_len,
    zb_uint32_t string_c_len)
{
    zb_uint8_t t[16];
    zb_uint8_t decrypted_text[ZB_IO_BUF_SIZE];
    zb_ushort_t decrypted_text_len;
    zb_uint8_t *beg = zb_buf_begin(buf);

    TRACE_MSG(TRACE_SECUR3, "zb_ccm_decrypt_n_auth key %p nonce %p buf %p a_len %hd c_len %hd",
              (FMT__P_P_P_H_H, key, nonce, buf, string_a_len, string_c_len));

    /* A.3.1Decryption Transformation */

    /*
      2 Form the padded message CiphertextData by right-concatenating the string C
      with the smallest non-negative number of all-zero octets such that the octet
      string CiphertextData has length divisible by 16.

      3 Use the encryption transformation in sub-clause A.2.3, with the data
      CipherTextData and the tag U as inputs.
     */
    encrypt_trans(ZB_CCM_M, key, nonce,
                  beg + string_a_len, /* string_c */
                  string_c_len - ZB_CCM_M,
                  /* 1 Parse the message c as C ||U, where the rightmost string U is an M-octet string.  */
                  beg + string_a_len + string_c_len - ZB_CCM_M, /* U */
                  decrypted_text);
    decrypted_text_len = string_c_len - ZB_CCM_M;

    /*
      1 Form the message AuthData using the input transformation in sub-
      clauseA.2.1, with the string a and the octet string m that was established in
      sub-clauseA.3.1 (step4) as inputs.

      2 Use the authentication transformation in sub-clauseA.2.2, with the message
      AuthData as input.
     */
    zb_ccm_auth_trans(ZB_CCM_M, key, nonce,
                      beg, /* string_a */
                      string_a_len, decrypted_text, decrypted_text_len, t);


    if (ZB_MEMCMP(decrypted_text + decrypted_text_len, t, ZB_CCM_M) == 0)
    {
        zb_buf_cut_right(buf, ZB_CCM_M);
        ZB_MEMCPY(beg + string_a_len, decrypted_text, decrypted_text_len);
        return RET_OK;
    }
    else
    {
        return RET_ERROR;
    }
}


/**
   Auth transformation from A.2.2

   Used in both IN and OUT time.

   @param t - (out) T from A.2.2 - procedure result - 16-butes byffer, its actual
            length is ccm_m.
 */
void zb_ccm_auth_trans(
    zb_ushort_t ccm_m,
    zb_uint8_t *key,
    zb_uint8_t *nonce,
    zb_uint8_t *string_a,
    zb_uint32_t string_a_len,
    zb_uint8_t *string_m,
    zb_uint32_t string_m_len,
    zb_uint8_t *t)
{
    zb_uint8_t *p;
    zb_ushort_t len;
    zb_uint8_t tmp[16];
    zb_uint8_t b[16];
    /* A.2.2Authentication Transformation */

    /*
      1 Form the 1-octet Flags field
       Flags = Reserved || Adata || M || L
      2 Form the 16-octet B0 field
       B0 = Flags || Nonce N || l(m)
    */

    b[0] = (1U << 6) | (ZB_CCM_L - 1U) | (zb_uint8_t)(((ccm_m - 2U) / 2U) << 3);
    ZB_MEMCPY(&b[1], nonce, ZB_CCM_NONCE_LEN);
    zb_htobe16(&b[ZB_CCM_NONCE_LEN + 1U], &string_m_len);
    ZB_CHK_ARR(b, 16);

    /* Execute CBC-MAC (CBC with zero initialization vector) */

    /* iteration 0: E(key, b ^ zero). result (X1) is in t. */
    zb_aes128(key, b, t);

    /* A.2.1Input Transformation */
    /*
      3 Form the padded message AddAuthData by right-concatenating the resulting
      string with the smallest non-negative number of all-zero octets such that the
      octet string AddAuthData has length divisible by 16

      Effectively done by memset(0)
     */
    /*
      1 Form the octet string representation L(a) of the length l(a) of the octet string a:
    */
    zb_htobe16(tmp, &string_a_len);
    /* 2 Right-concatenate the octet string L(a) and the octet string a itself: */
    if (string_a_len <= 16U - 2U)
    {
        ZB_BZERO(&tmp[2], 14);
        ZB_MEMCPY(&tmp[2], string_a, string_a_len);
        len = 0;
    }
    else
    {
        ZB_MEMCPY(&tmp[2], string_a, 14);
        len = string_a_len - 14U;
    }

    /* iteration 1: L(a) || a in tmp  */
    xor16(t, tmp, b);
    ZB_CHK_ARR(b, 16);
    zb_aes128(key, b, t);

    /* next iterations with a */
    p = string_a + 16 - 2;
    /* while no padding necessary */
    while (len >= 16U)
    {
        xor16(t, p, b);
        ZB_CHK_ARR(b, 16);
        zb_aes128(key, b, t);
        p += 16;
        len -= 16U;
    }
    if (len != 0U)
    {
        /* rest with padding */
        ZB_BZERO(tmp, 16);
        ZB_MEMCPY(tmp, p, len);
        xor16(t, tmp, b);
        ZB_CHK_ARR(b, 16);
        zb_aes128(key, b, t);
    }

    /* done with a, now process m */
    len = string_m_len;
    p = string_m;
    ZB_CHK_ARR(string_m, string_m_len);
    while (len >= 16U)
    {
        ZB_CHK_ARR(t, 16);
        ZB_CHK_ARR(p, 16);
        xor16(t, p, b);

        ZB_CHK_ARR(key, 16);
        ZB_CHK_ARR(b, 16);
        ZB_CHK_ARR(t, 16);

        zb_aes128(key, b, t);
        p += 16;
        len -= 16U;
    }
    if (len != 0U)
    {
        /* rest with padding */
        ZB_BZERO(tmp, 16);
        ZB_MEMCPY(tmp, p, len);
        xor16(t, tmp, b);
        zb_aes128(key, b, t);
    }

    /* return now. In first ccm_m bytes of t is T */
}


/**
   A.2.3Encryption Transformation
*/
void encrypt_trans(
    zb_ushort_t ccm_m,
    zb_uint8_t *key,
    zb_uint8_t *nonce,
    zb_uint8_t *string_m,
    zb_uint32_t string_m_len,
    zb_uint8_t *t,
    zb_uint8_t *encrypted)
{
    zb_uint8_t eai[16];
    zb_uint8_t ai[16];
    zb_uint16_t i;
    zb_ushort_t len;
    zb_uint8_t *p;
    zb_uint8_t *pe;


    TRACE_MSG(TRACE_SECUR3, ">> encrypt_trans ccm_m %hd key %p nonce %p string_m %p string_m_len %d t %p encrypted %p",
              (FMT__H_P_P_P_D_P_P, ccm_m, key, nonce, string_m, string_m_len, t, encrypted));


    p = string_m;
    pe = encrypted;
    len = string_m_len;
    i = 1;
    while (len >= 16U)
    {
        /* form Ai */
        ai[0] = (ZB_CCM_L - 1U);
        ZB_MEMCPY(&ai[1], nonce, ZB_CCM_NONCE_LEN);
        zb_htobe16(&ai[ZB_CCM_NONCE_LEN + 1U], &i);

        /* E(key, Ai) */
        zb_aes128(key, ai, eai);
        /* Ci =  E(key, Ai) ^ Mi */
        xor16(eai, p, pe);

        p += 16;
        pe += 16;
        len -= 16U;
        i++;
    }
    if (len != 0U)
    {
        /* process the rest with padding */

        /* form Ai */
        ai[0] = (ZB_CCM_L - 1U);
        ZB_MEMCPY(&ai[1], nonce, ZB_CCM_NONCE_LEN);
        zb_htobe16(&ai[ZB_CCM_NONCE_LEN + 1U], &i);

        /* E(key, Ai) */
        zb_aes128(key, ai, eai);
        /* Ci =  E(key, Ai) ^ Mi. Use ai as buffer */
        ZB_MEMSET(ai, 0, 16);
        ZB_MEMCPY(ai, p, len);
        xor16(eai, ai, pe);
        pe += len;
    }

    /* Now pe points to the encrypted data end, without padding */

    /* A0 */
    i = 0;
    ai[0] = (ZB_CCM_L - 1U);
    ZB_MEMCPY(&ai[1], nonce, ZB_CCM_NONCE_LEN);
    zb_htobe16(&ai[ZB_CCM_NONCE_LEN + 1U], &i);

    /* Define the 16-octet encryption block S0 by S0 = E(Key, A0) */
    /* store S0 in eai */
    zb_aes128(key, ai, eai);

    /*
      The encrypted authentication tag U is the result of XOR-ing the string
      and the authentication tag T.
      consisting of the leftmost M octets of S0
    */
    ZB_MEMSET(ai, 0, 16);
    ZB_MEMCPY(ai, t, ccm_m);
    xor16(eai, ai, pe);

    TRACE_MSG(TRACE_SECUR3, "<< encrypt_trans", (FMT__0));
}

static void xor16(zb_uint8_t *v1, zb_uint8_t *v2, zb_uint8_t *result)
{
    zb_uint_t i;
    for (i = 0 ; i < 16U ; ++i)
    {
        result[i] = v1[i] ^ v2[i];
    }
}

#endif  /* ZB_SOFT_SECURITY */

/*
  Some platforms have HW aes128 but no aes128 decryption.
  aes128 decrypt is necessary for ZLL only, so can use HW aes128 for HA and
  exclyde soft AES implementation.
 */
#if !defined ZB_HW_ZB_AES128 /* && defined ZB_SOFT_SECURITY */
#define NEED_SOFT_AES128
#endif

#if defined ZB_NEED_AES128_DEC && (!defined ZB_HW_ZB_AES128_DEC /* || defined ZB_SOFT_SECURITY */)
#define NEED_SOFT_AES128_DEC
#endif


// sw version of AES block still needed for hash functions
// Got from url: github.com/chrishulbert/crypto/blob/master/c/c_aes.c
// Original (c):

// Simple, thoroughly commented implementation of 128-bit AES / Rijndael using C
// Chris Hulbert - chris.hulbert@gmail.com - url: splinter.com.au/blog
// References:
// url: en.wikipedia.org/wiki/Advanced_Encryption_Standard
// url: en.wikipedia.org/wiki/Rijndael_key_schedule
// url: en.wikipeia.org/wiki/Rijndael_mix_columns
// url: en.wikipedia.org/wiki/Rijndael_S-box
// This code is public domain, or any OSI-approved license, your choice. No warranty.

// Here are all the lookup tables for the row shifts, rcon, s-boxes, and galois field multiplications
#ifdef NEED_SOFT_AES128
static const ZB_CODE zb_uint8_t shift_rows_table[] = {0, 5, 10, 15, 4, 9, 14, 3, 8, 13, 2, 7, 12, 1, 6, 11};
#endif
#ifdef NEED_SOFT_AES128_DEC
static const ZB_CODE zb_uint8_t shift_rows_table_inv[] = {0, 13, 10, 7, 4, 1, 14, 11, 8, 5, 2, 15, 12, 9, 6, 3};
#endif
#if defined NEED_SOFT_AES128_DEC || defined NEED_SOFT_AES128
static const ZB_CODE zb_uint8_t lookup_rcon[] = {0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a};
#endif
#if defined NEED_SOFT_AES128 || defined NEED_SOFT_AES128_DEC
static const ZB_CODE zb_uint8_t lookup_sbox[] = {0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76, 0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0, 0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15, 0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75, 0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84, 0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf, 0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8, 0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2, 0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73, 0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb, 0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79, 0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08, 0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a, 0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e, 0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf, 0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16};
#endif
#ifdef NEED_SOFT_AES128_DEC
static const ZB_CODE zb_uint8_t lookup_sbox_inv[] = {0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb, 0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb, 0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e, 0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25, 0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92, 0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84, 0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06, 0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b, 0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73, 0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e, 0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b, 0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4, 0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f, 0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef, 0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61, 0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d};
#endif
#ifdef NEED_SOFT_AES128
static const ZB_CODE zb_uint8_t lookup_g2 [] = {0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e, 0x10, 0x12, 0x14, 0x16, 0x18, 0x1a, 0x1c, 0x1e, 0x20, 0x22, 0x24, 0x26, 0x28, 0x2a, 0x2c, 0x2e, 0x30, 0x32, 0x34, 0x36, 0x38, 0x3a, 0x3c, 0x3e, 0x40, 0x42, 0x44, 0x46, 0x48, 0x4a, 0x4c, 0x4e, 0x50, 0x52, 0x54, 0x56, 0x58, 0x5a, 0x5c, 0x5e, 0x60, 0x62, 0x64, 0x66, 0x68, 0x6a, 0x6c, 0x6e, 0x70, 0x72, 0x74, 0x76, 0x78, 0x7a, 0x7c, 0x7e, 0x80, 0x82, 0x84, 0x86, 0x88, 0x8a, 0x8c, 0x8e, 0x90, 0x92, 0x94, 0x96, 0x98, 0x9a, 0x9c, 0x9e, 0xa0, 0xa2, 0xa4, 0xa6, 0xa8, 0xaa, 0xac, 0xae, 0xb0, 0xb2, 0xb4, 0xb6, 0xb8, 0xba, 0xbc, 0xbe, 0xc0, 0xc2, 0xc4, 0xc6, 0xc8, 0xca, 0xcc, 0xce, 0xd0, 0xd2, 0xd4, 0xd6, 0xd8, 0xda, 0xdc, 0xde, 0xe0, 0xe2, 0xe4, 0xe6, 0xe8, 0xea, 0xec, 0xee, 0xf0, 0xf2, 0xf4, 0xf6, 0xf8, 0xfa, 0xfc, 0xfe, 0x1b, 0x19, 0x1f, 0x1d, 0x13, 0x11, 0x17, 0x15, 0x0b, 0x09, 0x0f, 0x0d, 0x03, 0x01, 0x07, 0x05, 0x3b, 0x39, 0x3f, 0x3d, 0x33, 0x31, 0x37, 0x35, 0x2b, 0x29, 0x2f, 0x2d, 0x23, 0x21, 0x27, 0x25, 0x5b, 0x59, 0x5f, 0x5d, 0x53, 0x51, 0x57, 0x55, 0x4b, 0x49, 0x4f, 0x4d, 0x43, 0x41, 0x47, 0x45, 0x7b, 0x79, 0x7f, 0x7d, 0x73, 0x71, 0x77, 0x75, 0x6b, 0x69, 0x6f, 0x6d, 0x63, 0x61, 0x67, 0x65, 0x9b, 0x99, 0x9f, 0x9d, 0x93, 0x91, 0x97, 0x95, 0x8b, 0x89, 0x8f, 0x8d, 0x83, 0x81, 0x87, 0x85, 0xbb, 0xb9, 0xbf, 0xbd, 0xb3, 0xb1, 0xb7, 0xb5, 0xab, 0xa9, 0xaf, 0xad, 0xa3, 0xa1, 0xa7, 0xa5, 0xdb, 0xd9, 0xdf, 0xdd, 0xd3, 0xd1, 0xd7, 0xd5, 0xcb, 0xc9, 0xcf, 0xcd, 0xc3, 0xc1, 0xc7, 0xc5, 0xfb, 0xf9, 0xff, 0xfd, 0xf3, 0xf1, 0xf7, 0xf5, 0xeb, 0xe9, 0xef, 0xed, 0xe3, 0xe1, 0xe7, 0xe5};
static const ZB_CODE zb_uint8_t lookup_g3 [] = {0x00, 0x03, 0x06, 0x05, 0x0c, 0x0f, 0x0a, 0x09, 0x18, 0x1b, 0x1e, 0x1d, 0x14, 0x17, 0x12, 0x11, 0x30, 0x33, 0x36, 0x35, 0x3c, 0x3f, 0x3a, 0x39, 0x28, 0x2b, 0x2e, 0x2d, 0x24, 0x27, 0x22, 0x21, 0x60, 0x63, 0x66, 0x65, 0x6c, 0x6f, 0x6a, 0x69, 0x78, 0x7b, 0x7e, 0x7d, 0x74, 0x77, 0x72, 0x71, 0x50, 0x53, 0x56, 0x55, 0x5c, 0x5f, 0x5a, 0x59, 0x48, 0x4b, 0x4e, 0x4d, 0x44, 0x47, 0x42, 0x41, 0xc0, 0xc3, 0xc6, 0xc5, 0xcc, 0xcf, 0xca, 0xc9, 0xd8, 0xdb, 0xde, 0xdd, 0xd4, 0xd7, 0xd2, 0xd1, 0xf0, 0xf3, 0xf6, 0xf5, 0xfc, 0xff, 0xfa, 0xf9, 0xe8, 0xeb, 0xee, 0xed, 0xe4, 0xe7, 0xe2, 0xe1, 0xa0, 0xa3, 0xa6, 0xa5, 0xac, 0xaf, 0xaa, 0xa9, 0xb8, 0xbb, 0xbe, 0xbd, 0xb4, 0xb7, 0xb2, 0xb1, 0x90, 0x93, 0x96, 0x95, 0x9c, 0x9f, 0x9a, 0x99, 0x88, 0x8b, 0x8e, 0x8d, 0x84, 0x87, 0x82, 0x81, 0x9b, 0x98, 0x9d, 0x9e, 0x97, 0x94, 0x91, 0x92, 0x83, 0x80, 0x85, 0x86, 0x8f, 0x8c, 0x89, 0x8a, 0xab, 0xa8, 0xad, 0xae, 0xa7, 0xa4, 0xa1, 0xa2, 0xb3, 0xb0, 0xb5, 0xb6, 0xbf, 0xbc, 0xb9, 0xba, 0xfb, 0xf8, 0xfd, 0xfe, 0xf7, 0xf4, 0xf1, 0xf2, 0xe3, 0xe0, 0xe5, 0xe6, 0xef, 0xec, 0xe9, 0xea, 0xcb, 0xc8, 0xcd, 0xce, 0xc7, 0xc4, 0xc1, 0xc2, 0xd3, 0xd0, 0xd5, 0xd6, 0xdf, 0xdc, 0xd9, 0xda, 0x5b, 0x58, 0x5d, 0x5e, 0x57, 0x54, 0x51, 0x52, 0x43, 0x40, 0x45, 0x46, 0x4f, 0x4c, 0x49, 0x4a, 0x6b, 0x68, 0x6d, 0x6e, 0x67, 0x64, 0x61, 0x62, 0x73, 0x70, 0x75, 0x76, 0x7f, 0x7c, 0x79, 0x7a, 0x3b, 0x38, 0x3d, 0x3e, 0x37, 0x34, 0x31, 0x32, 0x23, 0x20, 0x25, 0x26, 0x2f, 0x2c, 0x29, 0x2a, 0x0b, 0x08, 0x0d, 0x0e, 0x07, 0x04, 0x01, 0x02, 0x13, 0x10, 0x15, 0x16, 0x1f, 0x1c, 0x19, 0x1a};
#endif
#ifdef NEED_SOFT_AES128_DEC
static const ZB_CODE zb_uint8_t lookup_g9  [] = {0x00, 0x09, 0x12, 0x1b, 0x24, 0x2d, 0x36, 0x3f, 0x48, 0x41, 0x5a, 0x53, 0x6c, 0x65, 0x7e, 0x77, 0x90, 0x99, 0x82, 0x8b, 0xb4, 0xbd, 0xa6, 0xaf, 0xd8, 0xd1, 0xca, 0xc3, 0xfc, 0xf5, 0xee, 0xe7, 0x3b, 0x32, 0x29, 0x20, 0x1f, 0x16, 0x0d, 0x04, 0x73, 0x7a, 0x61, 0x68, 0x57, 0x5e, 0x45, 0x4c, 0xab, 0xa2, 0xb9, 0xb0, 0x8f, 0x86, 0x9d, 0x94, 0xe3, 0xea, 0xf1, 0xf8, 0xc7, 0xce, 0xd5, 0xdc, 0x76, 0x7f, 0x64, 0x6d, 0x52, 0x5b, 0x40, 0x49, 0x3e, 0x37, 0x2c, 0x25, 0x1a, 0x13, 0x08, 0x01, 0xe6, 0xef, 0xf4, 0xfd, 0xc2, 0xcb, 0xd0, 0xd9, 0xae, 0xa7, 0xbc, 0xb5, 0x8a, 0x83, 0x98, 0x91, 0x4d, 0x44, 0x5f, 0x56, 0x69, 0x60, 0x7b, 0x72, 0x05, 0x0c, 0x17, 0x1e, 0x21, 0x28, 0x33, 0x3a, 0xdd, 0xd4, 0xcf, 0xc6, 0xf9, 0xf0, 0xeb, 0xe2, 0x95, 0x9c, 0x87, 0x8e, 0xb1, 0xb8, 0xa3, 0xaa, 0xec, 0xe5, 0xfe, 0xf7, 0xc8, 0xc1, 0xda, 0xd3, 0xa4, 0xad, 0xb6, 0xbf, 0x80, 0x89, 0x92, 0x9b, 0x7c, 0x75, 0x6e, 0x67, 0x58, 0x51, 0x4a, 0x43, 0x34, 0x3d, 0x26, 0x2f, 0x10, 0x19, 0x02, 0x0b, 0xd7, 0xde, 0xc5, 0xcc, 0xf3, 0xfa, 0xe1, 0xe8, 0x9f, 0x96, 0x8d, 0x84, 0xbb, 0xb2, 0xa9, 0xa0, 0x47, 0x4e, 0x55, 0x5c, 0x63, 0x6a, 0x71, 0x78, 0x0f, 0x06, 0x1d, 0x14, 0x2b, 0x22, 0x39, 0x30, 0x9a, 0x93, 0x88, 0x81, 0xbe, 0xb7, 0xac, 0xa5, 0xd2, 0xdb, 0xc0, 0xc9, 0xf6, 0xff, 0xe4, 0xed, 0x0a, 0x03, 0x18, 0x11, 0x2e, 0x27, 0x3c, 0x35, 0x42, 0x4b, 0x50, 0x59, 0x66, 0x6f, 0x74, 0x7d, 0xa1, 0xa8, 0xb3, 0xba, 0x85, 0x8c, 0x97, 0x9e, 0xe9, 0xe0, 0xfb, 0xf2, 0xcd, 0xc4, 0xdf, 0xd6, 0x31, 0x38, 0x23, 0x2a, 0x15, 0x1c, 0x07, 0x0e, 0x79, 0x70, 0x6b, 0x62, 0x5d, 0x54, 0x4f, 0x46};
static const ZB_CODE zb_uint8_t lookup_g11 [] = {0x00, 0x0b, 0x16, 0x1d, 0x2c, 0x27, 0x3a, 0x31, 0x58, 0x53, 0x4e, 0x45, 0x74, 0x7f, 0x62, 0x69, 0xb0, 0xbb, 0xa6, 0xad, 0x9c, 0x97, 0x8a, 0x81, 0xe8, 0xe3, 0xfe, 0xf5, 0xc4, 0xcf, 0xd2, 0xd9, 0x7b, 0x70, 0x6d, 0x66, 0x57, 0x5c, 0x41, 0x4a, 0x23, 0x28, 0x35, 0x3e, 0x0f, 0x04, 0x19, 0x12, 0xcb, 0xc0, 0xdd, 0xd6, 0xe7, 0xec, 0xf1, 0xfa, 0x93, 0x98, 0x85, 0x8e, 0xbf, 0xb4, 0xa9, 0xa2, 0xf6, 0xfd, 0xe0, 0xeb, 0xda, 0xd1, 0xcc, 0xc7, 0xae, 0xa5, 0xb8, 0xb3, 0x82, 0x89, 0x94, 0x9f, 0x46, 0x4d, 0x50, 0x5b, 0x6a, 0x61, 0x7c, 0x77, 0x1e, 0x15, 0x08, 0x03, 0x32, 0x39, 0x24, 0x2f, 0x8d, 0x86, 0x9b, 0x90, 0xa1, 0xaa, 0xb7, 0xbc, 0xd5, 0xde, 0xc3, 0xc8, 0xf9, 0xf2, 0xef, 0xe4, 0x3d, 0x36, 0x2b, 0x20, 0x11, 0x1a, 0x07, 0x0c, 0x65, 0x6e, 0x73, 0x78, 0x49, 0x42, 0x5f, 0x54, 0xf7, 0xfc, 0xe1, 0xea, 0xdb, 0xd0, 0xcd, 0xc6, 0xaf, 0xa4, 0xb9, 0xb2, 0x83, 0x88, 0x95, 0x9e, 0x47, 0x4c, 0x51, 0x5a, 0x6b, 0x60, 0x7d, 0x76, 0x1f, 0x14, 0x09, 0x02, 0x33, 0x38, 0x25, 0x2e, 0x8c, 0x87, 0x9a, 0x91, 0xa0, 0xab, 0xb6, 0xbd, 0xd4, 0xdf, 0xc2, 0xc9, 0xf8, 0xf3, 0xee, 0xe5, 0x3c, 0x37, 0x2a, 0x21, 0x10, 0x1b, 0x06, 0x0d, 0x64, 0x6f, 0x72, 0x79, 0x48, 0x43, 0x5e, 0x55, 0x01, 0x0a, 0x17, 0x1c, 0x2d, 0x26, 0x3b, 0x30, 0x59, 0x52, 0x4f, 0x44, 0x75, 0x7e, 0x63, 0x68, 0xb1, 0xba, 0xa7, 0xac, 0x9d, 0x96, 0x8b, 0x80, 0xe9, 0xe2, 0xff, 0xf4, 0xc5, 0xce, 0xd3, 0xd8, 0x7a, 0x71, 0x6c, 0x67, 0x56, 0x5d, 0x40, 0x4b, 0x22, 0x29, 0x34, 0x3f, 0x0e, 0x05, 0x18, 0x13, 0xca, 0xc1, 0xdc, 0xd7, 0xe6, 0xed, 0xf0, 0xfb, 0x92, 0x99, 0x84, 0x8f, 0xbe, 0xb5, 0xa8, 0xa3};
static const ZB_CODE zb_uint8_t lookup_g13 [] = {0x00, 0x0d, 0x1a, 0x17, 0x34, 0x39, 0x2e, 0x23, 0x68, 0x65, 0x72, 0x7f, 0x5c, 0x51, 0x46, 0x4b, 0xd0, 0xdd, 0xca, 0xc7, 0xe4, 0xe9, 0xfe, 0xf3, 0xb8, 0xb5, 0xa2, 0xaf, 0x8c, 0x81, 0x96, 0x9b, 0xbb, 0xb6, 0xa1, 0xac, 0x8f, 0x82, 0x95, 0x98, 0xd3, 0xde, 0xc9, 0xc4, 0xe7, 0xea, 0xfd, 0xf0, 0x6b, 0x66, 0x71, 0x7c, 0x5f, 0x52, 0x45, 0x48, 0x03, 0x0e, 0x19, 0x14, 0x37, 0x3a, 0x2d, 0x20, 0x6d, 0x60, 0x77, 0x7a, 0x59, 0x54, 0x43, 0x4e, 0x05, 0x08, 0x1f, 0x12, 0x31, 0x3c, 0x2b, 0x26, 0xbd, 0xb0, 0xa7, 0xaa, 0x89, 0x84, 0x93, 0x9e, 0xd5, 0xd8, 0xcf, 0xc2, 0xe1, 0xec, 0xfb, 0xf6, 0xd6, 0xdb, 0xcc, 0xc1, 0xe2, 0xef, 0xf8, 0xf5, 0xbe, 0xb3, 0xa4, 0xa9, 0x8a, 0x87, 0x90, 0x9d, 0x06, 0x0b, 0x1c, 0x11, 0x32, 0x3f, 0x28, 0x25, 0x6e, 0x63, 0x74, 0x79, 0x5a, 0x57, 0x40, 0x4d, 0xda, 0xd7, 0xc0, 0xcd, 0xee, 0xe3, 0xf4, 0xf9, 0xb2, 0xbf, 0xa8, 0xa5, 0x86, 0x8b, 0x9c, 0x91, 0x0a, 0x07, 0x10, 0x1d, 0x3e, 0x33, 0x24, 0x29, 0x62, 0x6f, 0x78, 0x75, 0x56, 0x5b, 0x4c, 0x41, 0x61, 0x6c, 0x7b, 0x76, 0x55, 0x58, 0x4f, 0x42, 0x09, 0x04, 0x13, 0x1e, 0x3d, 0x30, 0x27, 0x2a, 0xb1, 0xbc, 0xab, 0xa6, 0x85, 0x88, 0x9f, 0x92, 0xd9, 0xd4, 0xc3, 0xce, 0xed, 0xe0, 0xf7, 0xfa, 0xb7, 0xba, 0xad, 0xa0, 0x83, 0x8e, 0x99, 0x94, 0xdf, 0xd2, 0xc5, 0xc8, 0xeb, 0xe6, 0xf1, 0xfc, 0x67, 0x6a, 0x7d, 0x70, 0x53, 0x5e, 0x49, 0x44, 0x0f, 0x02, 0x15, 0x18, 0x3b, 0x36, 0x21, 0x2c, 0x0c, 0x01, 0x16, 0x1b, 0x38, 0x35, 0x22, 0x2f, 0x64, 0x69, 0x7e, 0x73, 0x50, 0x5d, 0x4a, 0x47, 0xdc, 0xd1, 0xc6, 0xcb, 0xe8, 0xe5, 0xf2, 0xff, 0xb4, 0xb9, 0xae, 0xa3, 0x80, 0x8d, 0x9a, 0x97};
static const ZB_CODE zb_uint8_t lookup_g14 [] = {0x00, 0x0e, 0x1c, 0x12, 0x38, 0x36, 0x24, 0x2a, 0x70, 0x7e, 0x6c, 0x62, 0x48, 0x46, 0x54, 0x5a, 0xe0, 0xee, 0xfc, 0xf2, 0xd8, 0xd6, 0xc4, 0xca, 0x90, 0x9e, 0x8c, 0x82, 0xa8, 0xa6, 0xb4, 0xba, 0xdb, 0xd5, 0xc7, 0xc9, 0xe3, 0xed, 0xff, 0xf1, 0xab, 0xa5, 0xb7, 0xb9, 0x93, 0x9d, 0x8f, 0x81, 0x3b, 0x35, 0x27, 0x29, 0x03, 0x0d, 0x1f, 0x11, 0x4b, 0x45, 0x57, 0x59, 0x73, 0x7d, 0x6f, 0x61, 0xad, 0xa3, 0xb1, 0xbf, 0x95, 0x9b, 0x89, 0x87, 0xdd, 0xd3, 0xc1, 0xcf, 0xe5, 0xeb, 0xf9, 0xf7, 0x4d, 0x43, 0x51, 0x5f, 0x75, 0x7b, 0x69, 0x67, 0x3d, 0x33, 0x21, 0x2f, 0x05, 0x0b, 0x19, 0x17, 0x76, 0x78, 0x6a, 0x64, 0x4e, 0x40, 0x52, 0x5c, 0x06, 0x08, 0x1a, 0x14, 0x3e, 0x30, 0x22, 0x2c, 0x96, 0x98, 0x8a, 0x84, 0xae, 0xa0, 0xb2, 0xbc, 0xe6, 0xe8, 0xfa, 0xf4, 0xde, 0xd0, 0xc2, 0xcc, 0x41, 0x4f, 0x5d, 0x53, 0x79, 0x77, 0x65, 0x6b, 0x31, 0x3f, 0x2d, 0x23, 0x09, 0x07, 0x15, 0x1b, 0xa1, 0xaf, 0xbd, 0xb3, 0x99, 0x97, 0x85, 0x8b, 0xd1, 0xdf, 0xcd, 0xc3, 0xe9, 0xe7, 0xf5, 0xfb, 0x9a, 0x94, 0x86, 0x88, 0xa2, 0xac, 0xbe, 0xb0, 0xea, 0xe4, 0xf6, 0xf8, 0xd2, 0xdc, 0xce, 0xc0, 0x7a, 0x74, 0x66, 0x68, 0x42, 0x4c, 0x5e, 0x50, 0x0a, 0x04, 0x16, 0x18, 0x32, 0x3c, 0x2e, 0x20, 0xec, 0xe2, 0xf0, 0xfe, 0xd4, 0xda, 0xc8, 0xc6, 0x9c, 0x92, 0x80, 0x8e, 0xa4, 0xaa, 0xb8, 0xb6, 0x0c, 0x02, 0x10, 0x1e, 0x34, 0x3a, 0x28, 0x26, 0x7c, 0x72, 0x60, 0x6e, 0x44, 0x4a, 0x58, 0x56, 0x37, 0x39, 0x2b, 0x25, 0x0f, 0x01, 0x13, 0x1d, 0x47, 0x49, 0x5b, 0x55, 0x7f, 0x71, 0x63, 0x6d, 0xd7, 0xd9, 0xcb, 0xc5, 0xef, 0xe1, 0xf3, 0xfd, 0xa7, 0xa9, 0xbb, 0xb5, 0x9f, 0x91, 0x83, 0x8d};
#endif


#if defined NEED_SOFT_AES128_DEC || defined NEED_SOFT_AES128
// Xor's all elements in a n zb_uint8_t array a by b
static void xor(zb_uint8_t *a, zb_uint8_t *b, zb_ushort_t n)
{
    zb_ushort_t i;
    for (i = 0; i < n; i++)
    {
        a[i] ^= b[i];
    }
}

// Xor the current cipher state by a specific round key
static void xor_round_key(zb_uint8_t *state, zb_uint8_t *keys, zb_ushort_t round)
{
    xor(state, keys + round * 16, 16);
}
#endif

#if defined NEED_SOFT_AES128_DEC || defined NEED_SOFT_AES128
// Apply and reverse the rijndael s-box to all elements in an array
// url: en.wikipedia.org/wiki/Rijndael_S-box
static void sub_bytes(zb_uint8_t *a, zb_ushort_t n)
{
    zb_ushort_t i;
    for (i = 0; i < n; i++)
    {
        zb_uint8_t v = a[i];
        a[i] = lookup_sbox[v];
    }
}
#endif

#ifdef NEED_SOFT_AES128_DEC
void sub_bytes_inv(zb_uint8_t *a, zb_ushort_t n)
{
    zb_ushort_t i;
    for (i = 0; i < n; i++)
    {
        a[i] = lookup_sbox_inv[a[i]];
    }
}
#endif


#if defined NEED_SOFT_AES128_DEC || defined NEED_SOFT_AES128
// Perform the core key schedule transform on 4 bytes, as part of the key expansion process
// url: en.wikipedia.org/wiki/Rijndael_key_schedule#Key_schedule_core
static void key_schedule_core(zb_uint8_t *a, zb_ushort_t i)
{
    zb_uint8_t temp = a[0]; // Rotate the output eight bits to the left
    a[0] = a[1];
    a[1] = a[2];
    a[2] = a[3];
    a[3] = temp;
    sub_bytes(a, 4); // Apply Rijndael's S-box on all four individual bytes in the output word
    a[0] ^= lookup_rcon[i]; // On just the first (leftmost) zb_uint8_t of the output word, perform the rcon operation with i
    // as the input, and exclusive or the rcon output with the first zb_uint8_t of the output word
}

// Expand the 16-zb_uint8_t key to 11 round keys (176 bytes)
// url: en.wikipedia.org/wiki/Rijndael_key_schedule#The_key_schedule
static void expand_key(zb_uint8_t *key, zb_uint8_t *keys)
{
    zb_ushort_t bytes = 16; // The count of how many bytes we've created so far
    zb_ushort_t i = 1; // The rcon iteration value i is set to 1
    zb_ushort_t j; // For repeating the second stage 3 times
    zb_uint8_t t[4]; // Temporary working area known as 't' in the Wiki article
    ZB_MEMCPY(keys, key, 16); // The first 16 bytes of the expanded key are simply the encryption key

    while (bytes < 176) // Until we have 176 bytes of expanded key, we do the following:
    {
        ZB_MEMCPY(t, keys + bytes - 4, 4); // We assign the value of the previous four bytes in the expanded key to t
        key_schedule_core(t, i); // We perform the key schedule core on t, with i as the rcon iteration value
        i++; // We increment i by 1
        xor(t, keys + bytes - 16, 4); // We exclusive-or t with the four-zb_uint8_t block 16 bytes before the new expanded key.
        ZB_MEMCPY(keys + bytes, t, 4); // This becomes the next 4 bytes in the expanded key
        bytes += 4; // Keep track of how many expanded key bytes we've added

        // We then do the following three times to create the next twelve bytes
        for (j = 0; j < 3; j++)
        {
            ZB_MEMCPY(t, keys + bytes - 4, 4); // We assign the value of the previous 4 bytes in the expanded key to t
            xor(t, keys + bytes - 16, 4); // We exclusive-or t with the four-zb_uint8_t block n bytes before
            ZB_MEMCPY(keys + bytes, t, 4); // This becomes the next 4 bytes in the expanded key
            bytes += 4; // Keep track of how many expanded key bytes we've added
        }
    }
}
#endif

#ifdef NEED_SOFT_AES128
// Apply / reverse the shift rows step on the 16 byte cipher state
// url: en.wikipedia.org/wiki/Advanced_Encryption_Standard#The_ShiftRows_step
static void shift_rows(zb_uint8_t *state)
{
    zb_ushort_t i;
    zb_uint8_t temp[16];
    ZB_MEMCPY(temp, state, 16);
    for (i = 0; i < 16; i++)
    {
        state[i] = temp[shift_rows_table[i]];
    }
}
#endif

#ifdef NEED_SOFT_AES128_DEC
void shift_rows_inv(zb_uint8_t *state)
{
    zb_ushort_t i;
    zb_uint8_t temp[16];
    ZB_MEMCPY(temp, state, 16);
    for (i = 0; i < 16; i++)
    {
        state[i] = temp[shift_rows_table_inv[i]];
    }
}
#endif

#ifdef NEED_SOFT_AES128
// Perform the mix columns matrix on one column of 4 bytes
// url: en.wikipedia.org/wiki/Rijndael_mix_columns
static void mix_col (zb_uint8_t *state)
{
    zb_uint8_t a0 = state[0];
    zb_uint8_t a1 = state[1];
    zb_uint8_t a2 = state[2];
    zb_uint8_t a3 = state[3];
    state[0] = lookup_g2[a0] ^ lookup_g3[a1] ^ a2 ^ a3;
    state[1] = lookup_g2[a1] ^ lookup_g3[a2] ^ a3 ^ a0;
    state[2] = lookup_g2[a2] ^ lookup_g3[a3] ^ a0 ^ a1;
    state[3] = lookup_g2[a3] ^ lookup_g3[a0] ^ a1 ^ a2;
}
#endif

#ifdef NEED_SOFT_AES128_DEC
void mix_col_inv (zb_uint8_t *state)
{
    zb_uint8_t a0 = state[0];
    zb_uint8_t a1 = state[1];
    zb_uint8_t a2 = state[2];
    zb_uint8_t a3 = state[3];
    state[0] = lookup_g14[a0] ^ lookup_g9[a3] ^ lookup_g13[a2] ^ lookup_g11[a1];
    state[1] = lookup_g14[a1] ^ lookup_g9[a0] ^ lookup_g13[a3] ^ lookup_g11[a2];
    state[2] = lookup_g14[a2] ^ lookup_g9[a1] ^ lookup_g13[a0] ^ lookup_g11[a3];
    state[3] = lookup_g14[a3] ^ lookup_g9[a2] ^ lookup_g13[a1] ^ lookup_g11[a0];
}
#endif

#ifdef NEED_SOFT_AES128
// Perform the mix columns matrix on each column of the 16 bytes
static void mix_cols (zb_uint8_t *state)
{
    mix_col(state);
    mix_col(state + 4);
    mix_col(state + 8);
    mix_col(state + 12);
}
#endif

#ifdef NEED_SOFT_AES128_DEC
void mix_cols_inv (zb_uint8_t *state)
{
    mix_col_inv(state);
    mix_col_inv(state + 4);
    mix_col_inv(state + 8);
    mix_col_inv(state + 12);
}
#endif

// Encrypt a single 128 bit block by a 128 bit key using AES
// url: en.wikipedia.org/wiki/Advanced_Encryption_Standard

#ifdef NEED_SOFT_AES128
void zb_aes128(zb_uint8_t *key, zb_uint8_t *msg, zb_uint8_t *c)
{
    zb_ushort_t i; // To count the rounds

    // Key expansion
    zb_uint8_t keys[176];
    expand_key(key, keys);

    // First Round
    ZB_MEMCPY(c, msg, 16);
    xor_round_key(c, keys, 0);

    // Middle rounds
    for (i = 0; i < 9; i++)
    {
        sub_bytes(c, 16);
        shift_rows(c);
        mix_cols(c);
        xor_round_key(c, keys, i + 1);
    }

    // Final Round
    sub_bytes(c, 16);
    shift_rows(c);
    xor_round_key(c, keys, 10);
}
#endif

#ifdef NEED_SOFT_AES128_DEC
void zb_aes128_dec(zb_uint8_t *key, zb_uint8_t *msg, zb_uint8_t *c)
{
    int i; // To count the rounds

    // Key expansion
    zb_uint8_t keys[176];
    expand_key(key, keys);

    // Reverse the final Round
    ZB_MEMCPY(c, msg, 16);
    xor_round_key(c, keys, 10);
    shift_rows_inv(c);
    sub_bytes_inv(c, 16);

    // Reverse the middle rounds
    for (i = 0; i < 9; i++)
    {
        xor_round_key(c, keys, 9 - i);
        mix_cols_inv(c);
        shift_rows_inv(c);
        sub_bytes_inv(c, 16);
    }

    // Reverse the first Round
    xor_round_key(c, keys, 0);
}
#endif

/**
  Hash function described in B.6.

  Used to generate key from the installcode and to generate APS key hash
  returns ZB_TRUE if blocks was processed and hash is calculated, returns ZB_FALSE if
  block size is greater that 268435455 bytes (0xFFFFFFFF bits).
 */
zb_bool_t zb_sec_b6_hash(zb_uint8_t *input, zb_uint32_t input_len, zb_uint8_t *output)
{
    zb_uint8_t cipher_in[ZBEE_SEC_CONST_BLOCKSIZE];
    zb_ushort_t  i, j;
    zb_uint8_t tmp_key[ZB_CCM_KEY_SIZE];
    zb_uint8_t tail_len;

    /* Clear the first hash block (Hash0). */
    ZB_BZERO(output, ZBEE_SEC_CONST_BLOCKSIZE);

    /* Note that the cryptographic hash function operates on bit strength of length less
       than 2^28 bits */
    if (input_len >= 0x10000000U)
    {
        return ZB_FALSE;
    }

    /* Create the subsequent hash blocks using the formula: Hash[i] = E(Hash[i-1], M[i]) XOR M[i]
     *
     */
    i = 0;
    j = 0;
    while (i < input_len)
    {
        cipher_in[j++] = input[i++];
        if (j >= ZBEE_SEC_CONST_BLOCKSIZE)
        {
            ZB_MEMCPY(tmp_key, output, ZB_CCM_KEY_SIZE);
            zb_aes128(tmp_key, cipher_in, output);

            for (j = 0; j < ZBEE_SEC_CONST_BLOCKSIZE; j++)
            {
                output[j] ^= cipher_in[j];
            }

            /* Reset j to start again at the beginning at the next block. */
            j = 0;
        }
    }
    /* Need to append the bit '1', followed by '0' padding long enough to end
     * the hash input on a block boundary. However, because 'n' is 16, and 'l'
     * will be a multiple of 8, the padding will be >= 7-bits, and we can just
     * append the byte 0x80.
     */
    cipher_in[j++] = 0x80;

    /* In case where input_len <= 0xFFFF bits add 16-bit represntation of input
       length (in bits) to string end. Otherwise append 32-bit input_len (in bits)
       plus two zero bytes. Representaion is a string (big-endian order). */
    if (input_len < 8192U)
    {
        tail_len = 2;
    }
    else
    {
        tail_len = 6;
    }

    /* Pad with '0' until the the current block is exactly 'n' bits from the end. */
    while (j != ((zb_uint_t)ZBEE_SEC_CONST_BLOCKSIZE - tail_len))
    {
        if (j >= ZBEE_SEC_CONST_BLOCKSIZE)
        {
            ZB_MEMCPY(tmp_key, output, ZB_CCM_KEY_SIZE);
            zb_aes128(tmp_key, cipher_in, output);
            for (j = 0; j < ZBEE_SEC_CONST_BLOCKSIZE; j++)
            {
                output[j] ^= cipher_in[j];
            }

            j = 0;
        }
        /* Pad the input with 0. */
        cipher_in[j++] = 0x00;
    }

    /* Add the 'n'-bit representation of 'l' to the end of the block. */
    /* Strict index condition for prevention MISRA array borders violation */
    if (j == ZBEE_SEC_CONST_BLOCKSIZE - 2U) /* tail_len == 2 */
    {
        zb_uint16_t blen;

        ZB_ASSERT(8U * input_len <= ZB_UINT16_MAX);
        blen = (zb_uint16_t)(8U * input_len);
        zb_htobe16(&cipher_in[j], &blen);
    }
    else if (j == ZBEE_SEC_CONST_BLOCKSIZE - 6U)/* tail_len == 6 */
    {
        zb_uint32_t blen = 8U * input_len;
        ZB_HTOBE32(&cipher_in[j], &blen);
        j += 4U;
        cipher_in[j++] = 0;
        cipher_in[j++] = 0;
    }
    else
    {
        return ZB_FALSE;
    }

    /* Process the last cipher block. */
    ZB_MEMCPY(tmp_key, output, ZB_CCM_KEY_SIZE);
    zb_aes128(tmp_key, cipher_in, output);
    for (j = 0; j < ZBEE_SEC_CONST_BLOCKSIZE; j++)
    {
        output[j] ^= cipher_in[j];
    }

    return ZB_TRUE;
} /* zb_sec_b6_hash */

#ifdef ZB_USE_OSIF_OTA_ROUTINES
/* TODO: No "hash in progress" check - implement if needed. */
typedef struct zb_b6_hash_iter_vars_s
{
    zb_uint32_t input_addr;
    zb_uint32_t input_len;
    zb_uint32_t len;
    zb_uint8_t hash_buf[ZBEE_SEC_CONST_BLOCKSIZE];
    void *dev;
} zb_b6_hash_iter_vars_t;

zb_b6_hash_iter_vars_t g_b6_hash_iter;

void zb_sec_b6_hash_iter(zb_uint8_t param);

zb_bool_t zb_sec_b6_hash_iter_start(void *dev, zb_uint32_t input_addr, zb_uint32_t input_len)
{
    TRACE_MSG(TRACE_SECUR3, "zb_sec_b6_hash_iter_start: addr %lx len %lx", (FMT__L_L, input_addr, input_len));

    /* Note that the cryptographic hash function operates on bit strength of length less
       than 2^32 bits */
    if (input_len >= 0x10000000)
    {
        return ZB_FALSE;
    }

    g_b6_hash_iter.input_addr = input_addr;
    g_b6_hash_iter.input_len = input_len;
    g_b6_hash_iter.len = input_len;
    g_b6_hash_iter.dev = dev;
    ZB_BZERO(g_b6_hash_iter.hash_buf, ZBEE_SEC_CONST_BLOCKSIZE);

    ZB_SCHEDULE_CALLBACK(zb_sec_b6_hash_iter, 0);

    return ZB_TRUE;
}

void zb_sec_b6_hash_iter(zb_uint8_t param)
{
    zb_uint8_t cipher_in[ZBEE_SEC_CONST_BLOCKSIZE];
    zb_ushort_t j;
    zb_uint8_t tmp_key[ZB_CCM_KEY_SIZE];

    ZVUNUSED(param);

    ZB_BZERO(cipher_in, ZBEE_SEC_CONST_BLOCKSIZE);
    if (g_b6_hash_iter.len > ZBEE_SEC_CONST_BLOCKSIZE)
    {
        TRACE_MSG(TRACE_SECUR3, "zb_sec_b6_hash_iter: block %hd", (FMT__H, g_b6_hash_iter.len));
        /* Get full block */
        zb_osif_ota_read(g_b6_hash_iter.dev, cipher_in, g_b6_hash_iter.input_addr, ZBEE_SEC_CONST_BLOCKSIZE);

        /* Create the subsequent hash blocks using the formula: Hash[i] = E(Hash[i-1], M[i]) XOR M[i]
         *
         */
        ZB_MEMCPY(tmp_key, g_b6_hash_iter.hash_buf, ZB_CCM_KEY_SIZE);
        zb_aes128(tmp_key, cipher_in, g_b6_hash_iter.hash_buf);
        for (j = 0; j < ZBEE_SEC_CONST_BLOCKSIZE; j++)
        {
            g_b6_hash_iter.hash_buf[j] ^= cipher_in[j];
        }

        TRACE_MSG(TRACE_SECUR3, "g_b6_hash_iter.hash_buf: " TRACE_FORMAT_128, (FMT__B, TRACE_ARG_128(g_b6_hash_iter.hash_buf)));
        /* Schedule next iteration */
        g_b6_hash_iter.len -= ZBEE_SEC_CONST_BLOCKSIZE;
        g_b6_hash_iter.input_addr += ZBEE_SEC_CONST_BLOCKSIZE;
        ZB_SCHEDULE_CALLBACK(zb_sec_b6_hash_iter, 0);
    }
    else
    {
        zb_uint8_t tail_len;

        TRACE_MSG(TRACE_SECUR3, "zb_sec_b6_hash_iter: last block %hd", (FMT__H, g_b6_hash_iter.len));
        /* Get last block */
        zb_osif_ota_read(g_b6_hash_iter.dev, cipher_in, g_b6_hash_iter.input_addr, g_b6_hash_iter.len);
        j = g_b6_hash_iter.len;

        /* ----------------------- */
        if (j >= ZBEE_SEC_CONST_BLOCKSIZE)
        {
            ZB_MEMCPY(tmp_key, g_b6_hash_iter.hash_buf, ZB_CCM_KEY_SIZE);
            zb_aes128(tmp_key, cipher_in, g_b6_hash_iter.hash_buf);

            for (j = 0; j < ZBEE_SEC_CONST_BLOCKSIZE; j++)
            {
                g_b6_hash_iter.hash_buf[j] ^= cipher_in[j];
            }
            /* Reset j to start again at the beginning at the next block. */
            j = 0;
        }

        /* Need to append the bit '1', followed by '0' padding long enough to end
         * the hash input on a block boundary. However, because 'n' is 16, and 'l'
         * will be a multiple of 8, the padding will be >= 7-bits, and we can just
         * append the byte 0x80.
         */
        cipher_in[j++] = 0x80;

        /* In case where input_len <= 0xFFFF bits add 16-bit represntation of input
           length (in bits) to string end. Otherwise append 32-bit input_len (in bits)
           plus two zero bytes. Representaion is a string (big-endian order). */
        if (g_b6_hash_iter.input_len < 8192)
        {
            tail_len = 2;
        }
        else
        {
            tail_len = 6;
        }

        /* Pad with '0' until the the current block is exactly 'n' bits from the end. */
        while (j != (zb_ushort_t)(ZBEE_SEC_CONST_BLOCKSIZE - tail_len))
        {
            if (j >= ZBEE_SEC_CONST_BLOCKSIZE)
            {
                ZB_MEMCPY(tmp_key, g_b6_hash_iter.hash_buf, ZB_CCM_KEY_SIZE);
                zb_aes128(tmp_key, cipher_in, g_b6_hash_iter.hash_buf);
                for (j = 0; j < ZBEE_SEC_CONST_BLOCKSIZE; j++)
                {
                    g_b6_hash_iter.hash_buf[j] ^= cipher_in[j];
                }
                j = 0;
            }
            /* Pad the input with 0. */
            cipher_in[j++] = 0x00;
        }

        /* Add the 'n'-bit representation of 'l' to the end of the block. */
        if (tail_len == 2)
        {
            zb_uint16_t blen = 8 * g_b6_hash_iter.input_len;
            zb_htobe16(&cipher_in[j], &blen);
        }
        else
        {
            zb_uint32_t blen = 8 * g_b6_hash_iter.input_len;
            ZB_HTOBE32(&cipher_in[j], &blen);
            j += 4;
            cipher_in[j++] = 0;
            cipher_in[j++] = 0;
        }

        /* Process the last cipher block. */
        ZB_MEMCPY(tmp_key, g_b6_hash_iter.hash_buf, ZB_CCM_KEY_SIZE);
        zb_aes128(tmp_key, cipher_in, g_b6_hash_iter.hash_buf);
        for (j = 0; j < ZBEE_SEC_CONST_BLOCKSIZE; j++)
        {
            g_b6_hash_iter.hash_buf[j] ^= cipher_in[j];
        }

        zb_sec_b6_hash_iter_done(g_b6_hash_iter.dev, g_b6_hash_iter.input_len, g_b6_hash_iter.hash_buf);
    }
} /* zb_sec_b6_hash_iter */
#endif /* ZB_USE_OSIF_OTA_ROUTINES */

/*FUNCTION:------------------------------------------------------
 *  NAME
 *      zbee_sec_key_hash
 *  DESCRIPTION
 *      Zigbee Keyed Hash Function. Described in Zigbee specification
 *      section B.1.4, and in FIPS Publication 198. Strictly speaking
 *      there is nothing about the Keyed Hash Function which restricts
 *      it to only a single byte input, but that's all Zigbee ever uses.
 *
 *      This function implements the hash function:
 *          Hash(Key, text) = H((Key XOR opad) || H((Key XOR ipad) || text));
 *          ipad = 0x36 repeated.
 *          opad = 0x5c repeated.
 *          H() = Zigbee Cryptographic Hash (B.1.3 and B.6).
 *
 *      The output of this function is an ep_alloced buffer containing
 *      the key-hashed output, and is garaunteed never to return NULL.
 *  PARAMETERS
 *      zb_uint8_t  *key    - Zigbee Security Key (must be ZBEE_SEC_CONST_KEYSIZE) in length.
 *      zb_uint8_t  input   -
 *  RETURNS
 *      zb_uint8_t*
 *---------------------------------------------------------------
 */
void zb_cmm_key_hash(zb_uint8_t *key, zb_uint8_t input, zb_uint8_t *hash_key)
{
    zb_uint8_t hash_in[2U * ZB_CCM_KEY_SIZE];
    zb_uint8_t hash_out[ZB_CCM_KEY_SIZE + 1U];
    zb_ushort_t i;
    zb_bool_t ret;
#define ipad  0x36U
#define opad  0x5CU

    ZVUNUSED(hash_key);

    for (i = 0; i < ZB_CCM_KEY_SIZE; i++)
    {
        /* Copy the key into hash_in and XOR with opad to form: (Key XOR opad) */
        hash_in[i] = key[i] ^ opad;
        /* Copy the Key into hash_out and XOR with ipad to form: (Key XOR ipad) */
        hash_out[i] = key[i] ^ ipad;
    }
    /* Append the input byte to form: (Key XOR ipad) || text. */
    hash_out[ZBEE_SEC_CONST_BLOCKSIZE] = input;
    /* Hash the contents of hash_out and append the contents to hash_in to
     * form: (Key XOR opad) || H((Key XOR ipad) || text).
     */
    ret = zb_sec_b6_hash(hash_out, ZB_CCM_KEY_SIZE + 1U, hash_in + ZB_CCM_KEY_SIZE);
    ZB_ASSERT(ret == ZB_TRUE);
    /* Hash the contents of hash_in to get the final result. */
    ret = zb_sec_b6_hash(hash_in, 2U * ZB_CCM_KEY_SIZE, hash_out);
    ZB_ASSERT(ret == ZB_TRUE);

    ZB_MEMCPY(hash_key, hash_out, ZB_CCM_KEY_SIZE);
} /* zbee_sec_key_hash */


/* Came here with merge from r22_test_harness/branches/ti1352p:46766. Looks like a copy-paste from some utility.
   Note that zb_sec_b6_hash() does the same thing.
   No C code calls this function. Python in CTH?

   To be killed after recheck.
 */
#ifdef ZB_TH_ENABLED
// AES MMO construction
void aes_mmo_key_hash(zb_uint8_t *M, zb_uint8_t *h)
{
    zb_uint32_t i, j;
    zb_uint16_t Mlen = 128;
    zb_uint8_t hash_j[16]; // 128-bit hash block
    zb_uint8_t msg_rest[32];  // 256-bit rest of message block
    zb_uint8_t *msg_ptr;
    zb_uint32_t t;         // total number of message blocks
    zb_uint32_t n_rest_blocks; // 1 or 2 blocks
    zb_uint32_t n_rest_bytes, n_msg_blocks;
    zb_uint8_t key128[16] = {0x00}; // key

    n_msg_blocks = (Mlen >> 7) + !!(Mlen & 0x7F);
    n_rest_bytes = ((Mlen & 0x7F) >> 3) + !!(Mlen & 0x07);
    n_rest_blocks = (n_rest_bytes <= 9) ? 1 : 2;

    t = n_msg_blocks + n_rest_blocks - (Mlen & 0x7F ? 1 : 0);

    memset(msg_rest, 0, sizeof(msg_rest));
    msg_ptr = msg_rest;
    msg_ptr[n_rest_bytes] = 0x80;

    if (n_rest_bytes)
    {
        /* Modify last block */
        memcpy(msg_ptr, M + (n_msg_blocks - 1) * 16, n_rest_bytes);
    }

    /* form last block */
    if (n_rest_blocks - 1)
    {
        msg_ptr += 16;
    }

    msg_ptr += 14;
    *msg_ptr++ = (zb_uint8_t)(Mlen >> 8);
    *msg_ptr = (zb_uint8_t)Mlen;

    for (j = 0, msg_ptr = M; j < t; ++j, msg_ptr += 16)
    {
        if (j == t - n_rest_blocks)
        {
            msg_ptr = msg_rest;
        }

        zb_aes128(key128, msg_ptr, hash_j);
        for (i = 0; i < 16; ++i)
        {
            hash_j[i] ^= msg_ptr[i];
        }
        memcpy(key128, hash_j, 16);
    }

    memcpy(h, hash_j, 16);
}
#endif

/*! @} */
