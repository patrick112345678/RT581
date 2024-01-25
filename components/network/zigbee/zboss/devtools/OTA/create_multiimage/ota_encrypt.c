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
/* PURPOSE: Routines for encryption of OTA image
*/

#include <stdio.h>
#include <string.h>
#ifndef ZB_WINDOWS
#include "zb_common.h"
#else
#include "zb_types.h"
#include "zb_osif.h"
#endif /* !ZB_WINDOWS */

#define CSS_MULTIIMAGE_UPGRADE
#ifndef ZB_WINDOWS
#include "css_device.h"
#endif /* !ZB_WINDOWS */
#include "css_ota.h"
#include "secur_aes128.h"

#define PUT_BE_L_OCTET_LEN(L_len, buf, i)       \
{                                               \
  if (L_len == 2)                               \
  {                                             \
    zb_uint16_t _i = (zb_uint16_t)i;                         \
    ZB_HTOBE16((buf), &_i);                     \
  }                                             \
  else                          /* 6 */         \
  {                                             \
    zb_uint32_t _i = i;                         \
    (buf)[0] = 0xff;                            \
    (buf)[1] = 0xfe;                            \
    ZB_HTOBE32(((buf)+2), &_i);                 \
  }                                             \
}

static void xor16(zb_uint8_t *v1, zb_uint8_t *v2, zb_uint8_t *result);

/**
   Auth transformation from A.2.2

   Used in both IN and OUT time.

   @param t - (out) T from A.2.2 - procedure result - 16-bytes buffer
 */
static void css_auth_trans(
    zb_uint8_t *key,
    zb_uint8_t *nonce,
    zb_uint8_t *string_a,
    zb_uint16_t string_a_len,
    zb_uint8_t *string_m,
    zb_uint_t string_m_len,
    zb_uint8_t *t)
{
    zb_uint8_t *p;
    zb_uint_t len;
    zb_uint8_t tmp[16];
    zb_uint8_t b[16];
    zb_uint_t L_len;
    zb_uint_t nonce_len;
    /* A.2.2Authentication Transformation */

    if (string_m_len >= (1l << 16))
    {
        L_len = 6;
    }
    else
    {
        L_len = 2;
    }
    nonce_len = 15 - L_len;

    /*
      1 Form the 1-octet Flags field
       Flags = Reserved || Adata || M || L
      2 Form the 16-octet B0 field
       B0 = Flags || Nonce N || l(m)
    */

    b[0] = (1 << 6) | (L_len - 1) | (((CSS_OTA_CCM_M - 2) / 2) << 3);
    ZB_MEMCPY(&b[1], nonce, nonce_len);
    PUT_BE_L_OCTET_LEN(L_len, &b[nonce_len + 1], string_m_len);

    /* Execute CBC-MAC (CBC with zero initialization vector) */

    /* iteration 0: E(key, b ^ zero). result (X1) is in t. */
    zb_aes128(key, b, t);

    /* A.2.1Input Transformation */
    /*
      3 Form the padded message AddAuthData by right-concatenating the resulting
      string with the smallest non-negative number of all-zero octets such that the
      octet string AddAuthData has length divisible by 16
     */
    /*
      1 Form the octet string representation L(a) of the length l(a) of the octet string a:
    */
    /* a is short, so sure L = 2 here */
    ZB_HTOBE16(tmp, &string_a_len);
    /* 2 Right-concatenate the octet string L(a) and the octet string a itself: */
    /* sure a < 14 */
    ZB_MEMSET(&tmp[2], 0, 14);
    ZB_MEMCPY(&tmp[2], string_a, string_a_len);

    /* iteration 1: L(a) || a in tmp  */
    xor16(t, tmp, b);
    zb_aes128(key, b, t);

    /* done with a, now process m */
    len = string_m_len;
    p = string_m;
    while (len >= 16)
    {
        xor16(t, p, b);

        zb_aes128(key, b, t);
        p += 16;
        len -= 16;
    }
    if (len)
    {
        /* rest with padding */
        ZB_MEMSET(tmp, 0, 16);
        ZB_MEMCPY(tmp, p, len);
        xor16(t, tmp, b);
        zb_aes128(key, b, t);
    }

    /* return now. In first ccm_m bytes of t is T */
}


/**
   A.2.3Encryption Transformation
*/
void css_encrypt_trans(
    zb_uint8_t *key,
    zb_uint8_t *nonce,
    zb_uint8_t *string_m,
    zb_uint32_t string_m_len,
    zb_uint8_t *t,
    zb_uint8_t *encrypted)
{
    zb_uint8_t eai[16];
    zb_uint8_t ai[16];
    zb_uint32_t i;
    zb_ushort_t len;
    zb_uint8_t *p;
    zb_uint8_t *pe;
    zb_uint_t L_len;
    zb_uint_t nonce_len;

    if (string_m_len >= (1l << 16))
    {
        L_len = 6;
    }
    else
    {
        L_len = 2;
    }
    nonce_len = 15 - L_len;


    p = string_m;
    pe = encrypted;
    len = string_m_len;
    i = 1;
    while (len >= 16)
    {
        /* form Ai */
        ai[0] = (L_len - 1);
        ZB_MEMCPY(&ai[1], nonce, nonce_len);
        PUT_BE_L_OCTET_LEN(L_len, &ai[nonce_len + 1], i);

        /* E(key, Ai) */
        zb_aes128(key, ai, eai);
        /* Ci =  E(key, Ai) ^ Mi */
        xor16(eai, p, pe);

        p += 16;
        pe += 16;
        len -= 16;
        i++;
    }
    if (len)
    {
        /* process the rest with padding */

        /* form Ai */
        ai[0] = (L_len - 1);
        ZB_MEMCPY(&ai[1], nonce, nonce_len);
        PUT_BE_L_OCTET_LEN(L_len, &ai[nonce_len + 1], i);

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
    ai[0] = (L_len - 1);
    ZB_MEMCPY(&ai[1], nonce, nonce_len);
    PUT_BE_L_OCTET_LEN(L_len, &ai[nonce_len + 1], i);

    /* Define the 16-octet encryption block S0 by S0 = E(Key, A0) */
    /* store S0 in eai */
    zb_aes128(key, ai, eai);

    /*
      The encrypted authentication tag U is the result of XOR-ing the string
      and the authentication tag T.
      consisting of the leftmost M octets of S0
    */
    ZB_MEMSET(ai, 0, 16);
    ZB_MEMCPY(ai, t, CSS_OTA_CCM_M);
    xor16(eai, ai, pe);
}

static void xor16(zb_uint8_t *v1, zb_uint8_t *v2, zb_uint8_t *result)
{
    zb_ushort_t i;
    for (i = 0 ; i < 16 ; ++i)
    {
        result[i] = v1[i] ^ v2[i];
    }
}


void
css_ota_encrypt_n_auth(
    zb_uint8_t *key,
    zb_uint8_t *nonce,
    zb_uint8_t *string_a,
    zb_uint_t string_a_len,
    zb_uint8_t *string_m,
    zb_uint_t string_m_len,
    zb_uint8_t *crypted_text)
{
    zb_uint8_t t[16];

    /* A.2.2Authentication Transformation */
    /* First ccm_m bytes of t now holds tag T */
    css_auth_trans(key, nonce, string_a, string_a_len, string_m, string_m_len, t);

#ifdef VERBOSE
    {
        int i;
        printf("MIC ");
        for (i = 0 ; i < 16 ; ++i)
        {
            printf("%02X ", t[i]);
        }
        printf("\n");
    }
#endif

    /* A.2.3Encryption Transformation */
    css_encrypt_trans(key, nonce, string_m, string_m_len, t, crypted_text);
}

void ota_create_nonce(zb_uint8_t nonce[16], zb_uint32_t file_version, zb_uint32_t image_tag)
{
    memset(nonce, 0, 16);
    ZB_HTOBE32(nonce, &file_version);
    ZB_HTOBE32(nonce + 4, &image_tag);
}


void ota_auth_crc(zb_uint8_t *data, zb_uint32_t size, zb_uint32_t *crc_p)
{
    zb_uint8_t key[16];
    zb_uint8_t nonce[16];
    zb_uint8_t t[16];
    memset(key, 0, sizeof(key));
    memset(nonce, 0, sizeof(nonce));
    css_auth_trans(key, nonce, (zb_uint8_t *)"", 0, data, size, t);
    memcpy(crc_p, t, 4);
}

