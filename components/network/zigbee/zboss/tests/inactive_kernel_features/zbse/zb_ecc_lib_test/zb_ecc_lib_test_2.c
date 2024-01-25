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
/* PURPOSE: ZBOSS ECC Library test - check how function works on test vectors
*/

#define ZB_TRACE_FILE_ID 40946
#include "zb_ecc_lib_test.h"
#include "zb_ecc.h"
#include "zb_ecc_internal.h"

#include "zb_debug.h"


/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifdef 1

#define PRV_LEN 36

#define ZB_ECC_PRIVATE_KEY_LEN(x) (x == sect163k1 ? 21 : 36)
#define ZB_ECC_PUBLIC_KEY_LEN(x) (x == sect163k1 ? 22 : 37)
/*See: 'C.6.1.2 Responder Data'*/
/*Original sequence:
0x02,0x02,0xF4,0xFA,0x2A,0x30,0x40,0x43,
0x3C,0x68,0x20,0x29,0x9D,0x18,0x2A,0x10,
0x42,0xE4,0x14,0x04,0xE3,0x37,0xC5,0x7F,
0x47,0x71,0x6B,0x42,0xDF,0xAF,0x97,0x0F,
0x15,0x80,0xA0,0x4C,0x9B
*/
zb_uint8_t _public_dev1[ZB_ECC_PUBLIC_KEY_LEN(sect283k1)] = {0x02,0x02,0xF4,0xFA,0x2A,0x30,0x40,0x43,
                                                             0x3C,0x68,0x20,0x29,0x9D,0x18,0x2A,0x10,
                                                             0x42,0xE4,0x14,0x04,0xE3,0x37,0xC5,0x7F,
                                                             0x47,0x71,0x6B,0x42,0xDF,0xAF,0x97,0x0F,
                                                             0x15,0x80,0xA0,0x4C,0x9B};

/*See: 'C.6.1.2 Responder Data'*/
/*Original sequence:
0x01,0x51,0xCD,0x0D,0xBC,0xB8,0x04,0x74,
0xBF,0x7A,0xC9,0xFE,0xEB,0xE3,0x9C,0x7A,
0x32,0xA6,0x35,0x18,0x93,0x8F,0xCA,0x97,
0x54,0xAA,0xE1,0x32,0xBC,0x9C,0x73,0xBE,
0x94,0xA7,0xE1,0xBE*/
zb_uint8_t private_dev1[ZB_ECC_PRIVATE_KEY_LEN(sect283k1)] = {0x01,0x51,0xCD,0x0D,0xBC,0xB8,0x04,0x74,
                                                              0xBF,0x7A,0xC9,0xFE,0xEB,0xE3,0x9C,0x7A,
                                                              0x32,0xA6,0x35,0x18,0x93,0x8F,0xCA,0x97,
                                                              0x54,0xAA,0xE1,0x32,0xBC,0x9C,0x73,0xBE,
                                                              0x94,0xA7,0xE1,0xBE};

/*See 'C.6.1.3 Initiator Data'*/
/*Original sequence:
0x03,0x03,0x0E,0x56,0xF7,0xAD,0xE8,0x66,
0xE7,0x63,0x72,0x76,0x4B,0xA2,0x0A,0x9F,
0xF1,0xFE,0x4C,0xAE,0x52,0x2F,0x94,0x83,
0x9E,0x70,0xF2,0xAD,0xFC,0x1C,0xA3,0xE9,
0x7F,0x4D,0xDC,0xAF,0x2E
*/
zb_uint8_t _public_dev2[ZB_ECC_PUBLIC_KEY_LEN(sect283k1)] = {0x03,0x03,0x0E,0x56,0xF7,0xAD,0xE8,0x66,
                                                             0xE7,0x63,0x72,0x76,0x4B,0xA2,0x0A,0x9F,
                                                             0xF1,0xFE,0x4C,0xAE,0x52,0x2F,0x94,0x83,
                                                             0x9E,0x70,0xF2,0xAD,0xFC,0x1C,0xA3,0xE9,
                                                             0x7F,0x4D,0xDC,0xAF,0x2E};

/*Original sequence:
0x00,0xF2,0x56,0x1A,0xDB,0x39,0xEF,0x49,
0xC1,0xD6,0x2E,0xF5,0x18,0x6C,0x6E,0x0C,
0x15,0x8A,0x5A,0x45,0xBF,0xCE,0x38,0x66,
0x09,0x31,0xAC,0xC3,0x69,0x45,0x92,0xD5,
0xAC,0xDE,0x90,0x06
*/
zb_uint8_t private_dev2[ZB_ECC_PRIVATE_KEY_LEN(sect283k1)] = {0x00,0xF2,0x56,0x1A,0xDB,0x39,0xEF,0x49,
                                                              0xC1,0xD6,0x2E,0xF5,0x18,0x6C,0x6E,0x0C,
                                                              0x15,0x8A,0x5A,0x45,0xBF,0xCE,0x38,0x66,
                                                              0x09,0x31,0xAC,0xC3,0x69,0x45,0x92,0xD5,
                                                              0xAC,0xDE,0x90,0x06};

/**
* Ephimerial public and private keys for device 1 (responder).
* Bytes order inversed. Constants took from 'C.6.3.4.1 Ephemeral Data'.
*/

/*Original sequence:
0x03,0xD4,0x8C,0x72,0x10,0xDD,0xBC,0xC4,0xFB,0x2E,0x5E,0x7A,0x0A,0xA1,0x6A,0x0D,
0xB8,0x95,0x40,0x82,0x0B,0x8D,0xC0,0x91,0xAB,0x52,0x1E,0xA8,0x24,0xAF,0xE1,0x17,
0xCA,0xDE,0x99,0x5B
*/
/*Ephimerial private key for device 1.*/
zb_uint8_t _eph_prv_key_dev1[ZB_ECC_PRIVATE_KEY_LEN(sect283k1)] = {0x03,0xD4,0x8C,0x72,0x10,0xDD,0xBC,0xC4,
                                                                   0xFB,0x2E,0x5E,0x7A,0x0A,0xA1,0x6A,0x0D,
                                                                   0xB8,0x95,0x40,0x82,0x0B,0x8D,0xC0,0x91,
                                                                   0xAB,0x52,0x1E,0xA8,0x24,0xAF,0xE1,0x17,
                                                                   0xCA,0xDE,0x99,0x5B};

/*Ephimerial public key for device 1 in compressed form.*/
/*Original sequence:
 0x03,0x00,0x9A,0x51,0x31,0xCF,0x5B,0x92,0xA0,0x16,0x37,0x8C,0x0F,0x7F,0x28,0x4E,
 0xCD,0x47,0xF9,0x40,0x10,0xF8,0x75,0xD4,0x3B,0xF1,0xE9,0xA6,0x54,0x74,0xAD,0xBF,
 0xC6,0x36,0x96,0xA9,0x30
*/
zb_uint8_t _x_eph_pub_key_dev1[ZB_ECC_PUBLIC_KEY_LEN(sect283k1)] = {0x03,0x00,0x9A,0x51,0x31,0xCF,0x5B,0x92,
                                                                    0xA0,0x16,0x37,0x8C,0x0F,0x7F,0x28,0x4E,
                                                                    0xCD,0x47,0xF9,0x40,0x10,0xF8,0x75,0xD4,
                                                                    0x3B,0xF1,0xE9,0xA6,0x54,0x74,0xAD,0xBF,
                                                                    0xC6,0x36,0x96,0xA9,0x30};
/*Original sequence:
0x00,0x13,0xD3,0x6D,0xE4,0xB1,0xEA,0x8E,
0x22,0x73,0x9C,0x38,0x13,0x70,0x82,0x3F,
0x40,0x4B,0xFF,0x88,0x62,0xB5,0x21,0xFE,
0xCA,0x98,0x71,0xFB,0x36,0x91,0x84,0x6D,
0x36,0x13,0x04,0xB4*/
zb_uint8_t _eph_prv_key_dev2[ZB_ECC_PRIVATE_KEY_LEN(sect283k1)] = {0x00,0x13,0xD3,0x6D,0xE4,0xB1,0xEA,0x8E,
                                                                   0x22,0x73,0x9C,0x38,0x13,0x70,0x82,0x3F,
                                                                   0x40,0x4B,0xFF,0x88,0x62,0xB5,0x21,0xFE,
                                                                   0xCA,0x98,0x71,0xFB,0x36,0x91,0x84,0x6D,
                                                                   0x36,0x13,0x04,0xB4};

/* Ephemeral public key for device 2 (initiator) in compressed form.
* Bytes order inversed. Constants took from 'C.6.3.3 Initiator Transform'.
*/
/*Original sequence:
0x03,0x05,0xF3,0x39,0x4E,0x15,0x68,0x06,0x60,0xEE,0xCA,0xA3,0x67,0x88,0xD9,0xB6,
0xF3,0x12,0xB9,0x71,0xCE,0x2C,0x96,0x17,0x57,0x0B,0xF7,0xDF,0xCD,0x21,0xC9,0x72,
0x01,0x77,0x62,0xC3,0x32
*/
zb_uint8_t _x_eph_pub_key_dev2[ZB_ECC_PUBLIC_KEY_LEN(sect283k1)] = {0x03,0x05,0xF3,0x39,0x4E,0x15,0x68,0x06,
                                                                    0x60,0xEE,0xCA,0xA3,0x67,0x88,0xD9,0xB6,
                                                                    0xF3,0x12,0xB9,0x71,0xCE,0x2C,0x96,0x17,
                                                                    0x57,0x0B,0xF7,0xDF,0xCD,0x21,0xC9,0x72,
                                                                    0x01,0x77,0x62,0xC3,0x32};


/*Given shared key. See 'C.6.3.4 Responder Transform'*/
/*Original sequence:
  04 F7 72 4A 9A 77 B2 1D   27 47 CC EF 68 A4 57 E4
  52 46 C4 BE 9F 66 FD 94   25 22 7B CB 2C C5 18 0E
  A9 CC CB 9A
  */
zb_uint8_t skey_giv[36] = {0x04,0xF7,0x72,0x4A,0x9A,0x77,0xB2,0x1D,
                           0x27,0x47,0xCC,0xEF,0x68,0xA4,0x57,0xE4,
                           0x52,0x46,0xC4,0xBE,0x9F,0x66,0xFD,0x94,
                           0x25,0x22,0x7B,0xCB,0x2C,0xC5,0x18,0x0E,
                           0xA9,0xCC,0xCB,0x9A};

zb_uint8_t _cert_dev1[74] = {0x00,0x26,0x22,0xA5,0x05,0xE8,0x93,0x8F,
                             0x27,0x0D,0x08,0x11,0x12,0x13,0x14,0x15,
                             0x16,0x17,0x18,0x00,0x52,0x92,0xA3,0x5B,
                             0xFF,0xFF,0xFF,0xFF,0x0A,0x0B,0x0C,0x0D,
                             0x0E,0x0F,0x10,0x11,0x88,0x03,0x03,0xB4,
                             0xE9,0xDC,0x54,0x3A,0x64,0x33,0x3C,0x98,
                             0x23,0x08,0x02,0x2B,0x54,0xE6,0x7E,0x2F,
                             0x15,0xF5,0x32,0x55,0x1B,0x0A,0x11,0xE2,
                             0xE2,0xC1,0xC1,0xD3,0x09,0x7A,0x43,0x24,
                             0xE7,0xED};

/*See 'C.6.1.3 Initiator Data'*/
/*Original sequence:
 00 84 A9 33 B3 7F 01 8D   EC 0D 08 11 12 13 14 15
 16 17 18 00 52 92 A3 8A   FF FF FF FF 0A 0B 0C 0D
 0E 0F 10 12 88 03 07 62   77 E2 F7 E2 25 2B 16 A0
 E9 2B 6E 87 71 BB 3F 20   79 46 CB D4 A4 5D 9A 9D
 F6 ED AB 8C 79 6A 48 E8   9D EC*/
 zb_uint8_t _cert_dev2[74] = {0x00,0x84,0xA9,0x33,0xB3,0x7F,0x01,0x8D,
                              0xEC,0x0D,0x08,0x11,0x12,0x13,0x14,0x15,
                              0x16,0x17,0x18,0x00,0x52,0x92,0xA3,0x8A,
                              0xFF,0xFF,0xFF,0xFF,0x0A,0x0B,0x0C,0x0D,
                              0x0E,0x0F,0x10,0x12,0x88,0x03,0x07,0x62,
                              0x77,0xE2,0xF7,0xE2,0x25,0x2B,0x16,0xA0,
                              0xE9,0x2B,0x6E,0x87,0x71,0xBB,0x3F,0x20,
                              0x79,0x46,0xCB,0xD4,0xA4,0x5D,0x9A,0x9D,
                              0xF6,0xED,0xAB,0x8C,0x79,0x6A,0x48,0xE8,
                              0x9D,0xEC};

zb_ecc_point_aff_t G283k1 = {{0x58492836,0xB0C2AC24,0x16876913,0x23C1567A,0x53CD265F,0x62F188E5,0x3F1A3B81,0x78CA4488,0x0503213F},
                             {0x77DD2259,0x4E341161,0xE4596236,0xE8184698,0xE87E45C0,0x07E5426F,0x8D90F95D,0x0F1C9E31,0x01CCDA38}};


/**
 * dump [size] bytes to screen
 * @param ptr  - array of [size] bytes to output
 * @param size - size in bytes of [ptr]
 */
void dump_bytes(zb_uint8_t *ptr, size_t size, zb_char_t *msg)
{
  printf("\n%s\n", msg);
  for(zb_uint8_t i=0; i < size; i++)
  {
    printf("%02X ", ptr[i]);
    if (15 == i % 16)
    {
      printf("\n");
    }
  }
  if (size % 16 > 0) printf("\n");
}


void zb_ecc_get_pseudo_random(zb_uint8_t *rnd, zb_uint8_t size)
{
  for(zb_uint8_t i = 0; i < size; i++)
  {
    rnd[i] = i + 1;
  }
}

void dump_dwords(zb_uint8_t *ptr, size_t size, zb_char_t *msg)
{
  zb_uint8_t len;

  len = (size % 4 > 0) ? size / 4 + 1: size / 4;
  printf("\n%s\n", msg);
  for(zb_uint8_t i=0; i < len; i++)
  {
    printf("%08X ", *((zb_uint32_t *)ptr + i));
    if (3 == i % 4)
    {
      printf("\n");
    }
  }
  if (len % 4 > 0) printf("\n");

}

void zb_ecc_point_addition(const zb_ecc_point_aff_t *P, const zb_ecc_point_aff_t *Q, zb_ecc_point_aff_t *R)
{
  zb_ecc_point_pro_t P_pro, R_pro;

  zb_ecc_affine_to_project(P, &P_pro);
  zb_ecc_mixed_addition(&P_pro, Q, &R_pro);
  zb_ecc_project_to_affine(&R_pro, R);
}

void g_point_computation(void)
{
  zb_ecc_point_aff_t G, G3, G5, G7, G9, G11, G13, G15;
  zb_ecc_point_pro_t R;
  zb_ecc_point_pro_t S;

  G = G283k1;

  zb_ecc_set_curve(sect283k1);
  /*G5 = - tG - G = - (t G + G)*/
  zb_ecc_affine_to_project(&G, &R);
  zb_ecc_point_frobenius(&R);
  zb_ecc_mixed_addition(&R, &G, &S);
  zb_ecc_project_to_affine(&S, &G5);
  zb_ecc_point_negation(&G5, &G5);

  dump_dwords((zb_uint8_t *)G5.x, 36, "G5.x");
  dump_dwords((zb_uint8_t *)G5.y, 36, "G5.y");

  /*G7 = - tG + G*/
  zb_ecc_point_negation(&G, &G3);
  zb_ecc_mixed_addition(&R, &G3, &S);
  zb_ecc_project_to_affine(&S, &G7);
  zb_ecc_point_negation(&G7, &G7);

  dump_dwords((zb_uint8_t *)G7.x, 36, "G7.x");
  dump_dwords((zb_uint8_t *)G7.y, 36, "G7.y");

  /*G3 = t^2 * G - G*/
  zb_ecc_point_frobenius(&R);
  zb_ecc_mixed_addition(&R, &G3, &S);
  zb_ecc_project_to_affine(&S, &G3);

  dump_dwords((zb_uint8_t *)G3.x, 36, "G3.x");
  dump_dwords((zb_uint8_t *)G3.y, 36, "G3.y");

  /*G11 = t^3 G + G3*/
  zb_ecc_point_frobenius(&R);
  zb_ecc_mixed_addition(&R, &G3, &S);
  zb_ecc_project_to_affine(&S, &G11);

  dump_dwords((zb_uint8_t *)G11.x, 36, "G11.x");
  dump_dwords((zb_uint8_t *)G11.y, 36, "G11.y");

  /*G13 = G - t^2 G5*/
  zb_ecc_mixed_addition(&R, &G5, &S);
  zb_ecc_project_to_affine(&S, &G13);

  dump_dwords((zb_uint8_t *)G13.x, 36, "G13.x");
  dump_dwords((zb_uint8_t *)G13.y, 36, "G13.y");

  /*G9 = - t^4 G - G7*/
  zb_ecc_point_frobenius(&R);
  zb_ecc_mixed_addition(&R, &G7, &S);
  zb_ecc_project_to_affine(&S, &G9);
  zb_ecc_point_negation(&G9, &G9);

  dump_dwords((zb_uint8_t *)G9.x, 36, "G9.x");
  dump_dwords((zb_uint8_t *)G9.y, 36, "G9.y");

  /*G15 = tG - G11*/
  zb_ecc_affine_to_project(&G, &R);
  zb_ecc_point_frobenius(&R);
  zb_ecc_point_negation(&G11,&G15);
  zb_ecc_mixed_addition(&R, &G15, &S);
  zb_ecc_project_to_affine(&S, &G15);

  dump_dwords((zb_uint8_t *)G15.x, 36, "G15.x");
  dump_dwords((zb_uint8_t *)G15.y, 36, "G15.y");
}


/**
 * Function for basic test of key generation, computing and comparing public keys.
 * @return  RET_OK in success, RET_ERROR in fail.
 */
zb_uint8_t base_test(void)
{

  zb_uint8_t pub_key[ZB_ECC_PUBLIC_KEY_LEN[sect283k1]];
  zb_uint8_t prv_key[ZB_ECC_PRIVATE_KEY_LEN[sect283k1]];
  zb_uint8_t rnd[ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES];
  zb_uint8_t buff[ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES];
  zb_ecc_point_aff_t point;

  zb_int8_t res;


  ZB_MEMSET(&point, 0, sizeof(zb_ecc_point_aff_t));
  zb_ecc_get_pseudo_random(rnd, PRV_LEN);
  zb_ecc_key_generation(rnd, prv_key, pub_key);

  // TRACE_MSG(TRACE_APP1, "Private key after generation", (FMT__0));
  // DUMP_TRAF("Private key-->", (zb_uint8_t *)key.d, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES, 0);
  //
  // TRACE_MSG(TRACE_APP1, "Public key after generation", (FMT__0));
  // DUMP_TRAF("Public key-->", (zb_uint8_t *)key.Qx, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES, 0);

  ZB_MEMSET(buff, 0, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);
  ZB_MEMCPY(buff, pub_key, ZB_ECC_PUBLIC_KEY_LEN[sect283k1]);
  zb_ecc_inverse_bytes(buff, ZB_ECC_PUBLIC_KEY_LEN[sect283k1]);
  if(RET_OK != (res = zb_ecc_point_decompression((zb_uint32_t *)buff, &point)))
  {
    TRACE_MSG(TRACE_APP1, "Have error in point decompression", (FMT__0));

    TRACE_MSG(TRACE_APP1, "Private key after generation", (FMT__0));
    DUMP_TRAF("Private key-->", prv_key, ZB_ECC_PUBLIC_KEY_LEN[sect283k1], 0);

    TRACE_MSG(TRACE_APP1, "Public key after generation", (FMT__0));
    DUMP_TRAF("Public key-->", pub_key, ZB_ECC_PRIVATE_KEY_LEN[sect283k1], 0);

    return RET_ERROR;
  }
  else
  {
    TRACE_MSG(TRACE_APP1, "Decompress public key successfully", (FMT__0));
  }

  if(ZB_PK_VALID != (res = zb_ecc_pk_validation(&point)))
  {
    TRACE_MSG(TRACE_APP1, "Have error in generated public key validation", (FMT__0));
    return RET_ERROR; //FIXME: need to implement pk_validation procedure for sect283k1 curve
  }
  else
  {
    TRACE_MSG(TRACE_APP1, "Generated public key validation is finished successfully", (FMT__0));
  }

  TRACE_MSG(TRACE_APP1, "Base test: Passed", (FMT__0));
  return RET_OK;
}


/**
 * Test public key generation procedure by given private key
 * @param  prv_key     pivate key
 * @param  exp_pub_key expected public key
 * @return             RET_OK by sucess, otherwise RET_ERROR
 */
zb_uint8_t test_key_generation(zb_uint8_t *prv_key, zb_uint8_t *exp_pub_key)
{
  zb_uint8_t buff[ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES];
  zb_uint8_t pub_key[ZB_ECC_PUBLIC_KEY_LEN[sect283k1]];
  zb_ecc_point_aff_t point;

  //zb_int8_t res;

  zb_ecc_key_generation(prv_key, prv_key, pub_key);

  if(0 != ZB_MEMCMP(pub_key, exp_pub_key, ZB_ECC_PUBLIC_KEY_LEN[sect283k1]))
  {
    TRACE_MSG(TRACE_APP1, "Calculated public key '!=' expected public key", (FMT__0));

    DUMP_TRAF("Calculated public key-->", pub_key, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES, 0);

    DUMP_TRAF("Expected public key:-->", exp_pub_key, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES, 0);

    return RET_ERROR;
  }
  else
  {
    TRACE_MSG(TRACE_APP1, "Calculated public key '==' expected public key", (FMT__0));
  }

  ZB_MEMSET(buff, 0, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);
  ZB_MEMCPY(buff, pub_key, ZB_ECC_PUBLIC_KEY_LEN[sect283k1]);
  zb_ecc_inverse_bytes(buff, ZB_ECC_PUBLIC_KEY_LEN[sect283k1]);
  if(RET_OK != zb_ecc_point_decompression((zb_uint32_t *)buff, &point))
  {
    TRACE_MSG(TRACE_APP1, "Have error in point_decompression", (FMT__0));
    return RET_ERROR;
  }
  else
  {
    TRACE_MSG(TRACE_APP1, "Decompress public key successfully", (FMT__0));
    // DUMP_TRAF("Calculated public key, X coordinate:-->", (zb_uint8_t *)&point.x, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES, 0);
    // DUMP_TRAF("Calculated public key, Y coordinate:-->", (zb_uint8_t *)&point.y, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES, 0);
  }

  if(ZB_PK_VALID != zb_ecc_pk_validation(&point))
  {
    TRACE_MSG(TRACE_APP1, "Have error in given public key validation", (FMT__0));
    /*FIXME: Fix public key validation procedure for sect283k1 curve*/
    return RET_ERROR;
  }
  else
  {
    TRACE_MSG(TRACE_APP1, "Given public key validation is finished successfully", (FMT__0));
  }

  return RET_OK;
}

/**
 * See 'C.5.2.1 Initiate Key Establishment Request'
 * @param  public_key_comp - non-initialized device 2(initiator) publik key
 * @return RET_OK in sucess, and RET_ERROR in fail.
 */
zb_uint8_t initiate_key_establishment_request(zb_uint8_t *exp_pub_key, zb_uint8_t *cert)
{
  /*See 'C.6.1.1 CA Public Key'*/
  /*In reverse order: 0x52,0x67,0xff,0x1d,0xdb,0x54,0x51,0x60,0x6b,0x1c,0x1c,
                      0x1a,0x5c,0x8c,0xb2,0x3 a,0x31,0x99,0x17,0x0a,0x9e,0x7a,0xa2,0x26,
                      0x00,0x38,0x38,0xdc,0x9b,0xf4,0x39,0x9f,0x2d,0x02,0x45,0xa4,0x07,0x02*/
/*Original sequence:
  02 07 A4 45 02 2D 9F 39
  f4 9B DC 38 38 00 26 A2
  7A 9E 0A 17 99 31 3A B2
  8C 5C 1A 1C 6B 60 51 54
  DB 1D FF 67 52
*/
  zb_uint8_t CA_pub_key[ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES] = {0x02,0x07,0xa4,0x45,0x02,0x2d,0x9f,0x39,
                                                            0xf4,0x9b,0xdc,0x38,0x38,0x00,0x26,0xa2,
                                                            0x7a,0x9e,0x0a,0x17,0x99,0x31,0x3a,0xb2,
                                                            0x8c,0x5c,0x1a,0x1c,0x6b,0x60,0x51,0x54,
                                                            0xdb,0x1d,0xff,0x67,0x52};

  zb_uint8_t x_pub_key_dev2[ZB_ECC_PUBLIC_KEY_LEN[sect283k1]];

  /**
   * CA_pub_key defined in 'C.5.1.1 CA Public Key'
   * _cert_dev2 defined in 'C.5.1.3 Initiator Data'
   * public_key_comp is extracted public key from implicit certificate for
   * initiator(device 2)
   */
  if(RET_OK != zb_ecc_ecqv_extraction(CA_pub_key, &cert[37], cert, x_pub_key_dev2))
  {
    TRACE_MSG(TRACE_APP1, "Have error in public key extraction", (FMT__0));
    return RET_ERROR;
  }
  else
  {
    TRACE_MSG(TRACE_APP1, "Public key extraction is finished successfully", (FMT__0));
  }

  if(0 != ZB_MEMCMP(x_pub_key_dev2, exp_pub_key, ZB_ECC_PUBLIC_KEY_LEN[sect283k1]))
  {
    TRACE_MSG(TRACE_APP1, "Extracted public key '!=' expected public key", (FMT__0));

    DUMP_TRAF("Extracted public key-->", (zb_uint8_t *)x_pub_key_dev2, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES, 0);
    DUMP_TRAF("Expected public key-->", (zb_uint8_t *)exp_pub_key, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES, 0);

    return RET_ERROR;
  }
  else
  {
    TRACE_MSG(TRACE_APP1, "Extracted public key '==' expected public key", (FMT__0));
  }

  return RET_OK;
}

/**
 * Generate shared secret for device 1(responder) between device1(initiator)
 * and device 2(responder). See 'C.5.3.4 Responder Transform', 'C.5.3.4.3 Detailed Steps'.
 * @param  x_pkey_dev2 - public key for device 2 in compressed form
 * @return             RET_OK in success, RET_ERROR in fail
 */
zb_int8_t generate_shared_secret(zb_uint8_t *prv_dev1, zb_uint8_t *eph_prv_dev1,
                                 zb_uint8_t *eph_pub_dev1, zb_uint8_t *pub_dev2, zb_uint8_t *eph_pub_dev2, zb_uint8_t *skey1)
{

  ZB_MEMSET(skey1, 0, 36);

  // /*Initialize key pairs for device 1(responder).*/
  // ZB_MEMCPY(key.d, prv_dev1, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES); /*Constants took from 'C.5.1.2 Responder Data'.*/
  // ZB_MEMCPY(key.Qx, pub_dev1, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES); /*Constants took from 'C.5.1.2 Responder Data'*/
  //
  // ZB_MEMCPY(eph_key.d, eph_prv_dev1, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);
  // ZB_MEMCPY(eph_key.Qx, eph_pub_dev1, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);

  if(RET_OK != zb_ecc_ecmqv(prv_dev1, eph_prv_dev1, eph_pub_dev1, pub_dev2, eph_pub_dev2, skey1))
  {
    TRACE_MSG(TRACE_APP1, "Shared key generation failed", (FMT__0));

    /*FIXME: within 36 use a macros*/
    DUMP_TRAF("Shared key between device 1 and device 2 is-->", skey1, 36, 0);
    DUMP_TRAF("Given shared key between device 1 and device 2 is-->", skey_giv, 36, 0);

    return RET_ERROR;
  }
  else
  {
    TRACE_MSG(TRACE_APP1, "Shared key generation is finished successfully", (FMT__0));
  }

  return RET_OK;
}


zb_ret_t confirm_key_response(zb_uint8_t *skey)
{

  zb_uint8_t mac_key[16] = {0xED,0x38,0x0A,0x00,0x29,0x66,0x00,0xFB,
                            0x6B,0x89,0x30,0x25,0xDE,0x5F,0xD1,0x37};
  /*zb_uint8_t mac_key[16];*/
  /*Original sequence from 'C.6.3.4 Responder Transform'
    0x02,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x12,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,
    0x11,0x03,0x05,0xF3,0x39,0x4E,0x15,0x68,0x06,0x60,0xEE,0xCA,0xA3,0x67,0x88,0xD9,
    0xB6,0xF3,0x12,0xB9,0x71,0xCE,0x2C,0x96,0x17,0x57,0x0B,0xF7,0xDF,0xCD,0x21,0xC9,
    0x72,0x01,0x77,0x62,0xC3,0x32,0x03,0x00,0x9A,0x51,0x31,0xCF,0x5B,0x92,0xA0,0x16,
    0x37,0x8C,0x0F,0x7F,0x28,0x4E,0xCD,0x47,0xF9,0x40,0x10,0xF8,0x75,0xD4,0x3B,0xF1,
    0xE9,0xA6,0x54,0x74,0xAD,0xBF,0xC6,0x36,0x96,0xA9,0x30
  */
  zb_uint8_t message[91] = {0x02,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,
                            0x12,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,
                            0x11,0x03,0x05,0xF3,0x39,0x4E,0x15,0x68,
                            0x06,0x60,0xEE,0xCA,0xA3,0x67,0x88,0xD9,
                            0xB6,0xF3,0x12,0xB9,0x71,0xCE,0x2C,0x96,
                            0x17,0x57,0x0B,0xF7,0xDF,0xCD,0x21,0xC9,
                            0x72,0x01,0x77,0x62,0xC3,0x32,0x03,0x00,
                            0x9A,0x51,0x31,0xCF,0x5B,0x92,0xA0,0x16,
                            0x37,0x8C,0x0F,0x7F,0x28,0x4E,0xCD,0x47,
                            0xF9,0x40,0x10,0xF8,0x75,0xD4,0x3B,0xF1,
                            0xE9,0xA6,0x54,0x74,0xAD,0xBF,0xC6,0x36,
                            0x96,0xA9,0x30};

  zb_uint8_t macu[16] = {0xBF,0x7E,0x1A,0x26,0xD4,0xEF,0x70,0x38,
                         0xB5,0x68,0x13,0xE4,0x65,0xA1,0x31,0xC9};
  zb_buf_t *hash_key = ZB_GET_ANY_BUF();
  zb_uint8_t *p;
  zb_uint8_t buff[40];
  zb_uint8_t id[4] = {0x00,0x00,0x00,0x01};
  zb_uint8_t ret_val = RET_ERROR;

  ZB_MEMCPY(buff, skey, 36);
  ZB_MEMCPY(buff + 36, id, 4);
  zb_sec_b6_hash(buff, 40, mac_key);
  // DUMP_TRAF("MacKey-->", (zb_uint8_t *)mac_key, 16, 0);
  // DUMP_TRAF("Message-->", (zb_uint8_t *)message, 91, 0);
  zb_ecc_key_hmac(mac_key, message, 91, hash_key);
  // DUMP_TRAF("MACV-->", (zb_uint8_t *)hash_key, 16, 0);

  p = ZB_BUF_BEGIN(hash_key);
  if(16 < ZB_BUF_LEN(hash_key))
  {
    if(0 == ZB_MEMCMP(p, macu, 16))
    {
      ret_val = RET_OK;
    }
    else
    {
      DUMP_TRAF("Calculated MAC(U) -->", p, 16, 0);
      DUMP_TRAF("Given MAC(U) -->", macu, 16, 0);
    }
  }
  else
  {
    TRACE_MSG(TRACE_APP1, "ZB_BUF_LEN less than 16 bytes", (FMT__0));
  }

  TRACE_MSG(TRACE_APP1, "Calculated MAC(U) '%s' given MAC(U)", (FMT__P, (RET_OK == ret_val) ? "==" : "!="));
  TRACE_MSG(TRACE_APP1, "MAC(U) comparison test: %s", (FMT__P, (RET_OK == ret_val) ? "Passed" : "Failed"));

  zb_free_buf(hash_key);

  return ret_val;
}


void zb_ecc_trace(zb_uint8_t curve_id, zb_uint32_t *a, zb_uint32_t *b)
{
  zb_uint32_t i, j;
  zb_uint32_t c[ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS];
  zb_uint32_t Num;
  zb_uint32_t BitLen;

  switch(curve_id)
  {
    case sect163k1:
      Num = 6;
      BitLen = 163;

      break;
    case sect283k1:
      Num = 9;  /*NOTE: previous Num = 10*/
      BitLen = 283;

      break;
    default:
      return;
  }

  ZB_MEMCPY(c, a, 4*Num);

  for(i = 1; i <= BitLen - 1; i++)
  {
    zb_ecc_modsq(c, c);
    //zb_ecc_modsq(curve_id, c, c);
    for(j = Num - 1; j != 0; j--)
    {
      *(c + j) ^= *(a + j);
    }
    *c ^= *a;
  }

  ZB_MEMCPY(b, c, 4*Num);
}

void check_trace(zb_uint8_t curve_id)
{
  zb_uint32_t i, word, shift;
  zb_uint32_t a[9], b[9];
  zb_uint16_t BitLen;
  zb_uint8_t Num;
  zb_uint8_t del = '{';

  switch(curve_id)
  {
    case sect163k1:
      Num = 6;
      BitLen = 163;

      break;
    case sect283k1:
      Num = 9;  /*NOTE: previous Num = 10*/
      BitLen = 283;

      break;
    default:
      return;
  }

  ZB_MEMSET(a, 0, 9*4);
  ZB_MEMSET(b, 0, 9*4);
  printf("Trace(z^i) = 1 for i in ");
  for(i=1; i <= BitLen; i++)
  {
    word = i / 32;
    shift = i % 32 - 1;
    *(a + word) = 1 << shift;
    //dump_bytes((zb_uint8_t *)a, 9*4, "z:");
    zb_ecc_trace(curve_id, a, b);
    if(*b & 0x01)
    {
      printf("%c %d", del, i - 1);
      del = ',';
    }
    //dump_bytes((zb_uint8_t *)b, 6*4, "z:");
    ZB_MEMSET(a, 0, 9*4);
    ZB_MEMSET(b, 0, 9*4);
  }
  printf("}\n");
}

zb_uint8_t extended_test(zb_uint8_t dev_id, zb_uint8_t *cert_dev1, zb_uint8_t *prv_dev1,
                         zb_uint8_t *pub_dev1, zb_uint8_t *eph_prv_dev1, zb_uint8_t *eph_pub_dev1,
                         zb_uint8_t *pub_dev2, zb_uint8_t *eph_pub_dev2, zb_uint8_t *skey1)
{
  TRACE_MSG(TRACE_ZCL1, "> Key generation for device %d", (FMT__H, dev_id));
  if(RET_OK != test_key_generation(prv_dev1, pub_dev1))
  {
    TRACE_MSG(TRACE_APP1, "Test: Failed", (FMT__0));
    return RET_ERROR;
  }

  TRACE_MSG(TRACE_APP1, "> Extract public key for device %d", (FMT__H, dev_id));
  if(RET_OK != initiate_key_establishment_request(pub_dev1, cert_dev1))
  {
    TRACE_MSG(TRACE_APP1, "Test: Failed", (FMT__0));
    return RET_ERROR;
  }

  TRACE_MSG(TRACE_APP1, "> Generate shared secret for device %d", (FMT__H, dev_id));
  if(RET_OK != generate_shared_secret(prv_dev1, eph_prv_dev1, eph_pub_dev1, pub_dev2, eph_pub_dev2, skey1))
  {
    TRACE_MSG(TRACE_APP1, "Test: Failed", (FMT__0));
    return RET_ERROR;
  }

  TRACE_MSG(TRACE_ZCL1, "> Start shared key comparison", (FMT__0));
  //FIXME:ZB_MEMCMP
  if(0 != ZB_MEMCMP(skey1, skey_giv, 36))
  {
    TRACE_MSG(TRACE_APP1, "Shared key for device %d '!=' given shared key", (FMT__H, dev_id));

    DUMP_TRAF("Shared key for device 1 -->", skey1, 36, 0);
    DUMP_TRAF("Given shared key -->", skey_giv, 36, 0);

    TRACE_MSG(TRACE_APP1, "Shared key comparison test: Failed", (FMT__0));
    return RET_ERROR;
  }

  TRACE_MSG(TRACE_APP1, "Extended test for device %d: Passed", (FMT__H, dev_id));

  return RET_OK;
}

MAIN()
{
  zb_uint8_t skey1[36];
  zb_uint8_t skey2[36];

  ARGV_UNUSED;

  ZB_INIT("zb_ecc_lib_test_2");

  // g_point_computation();
  // check_trace(sect283k1);

  zb_ecc_set_curve(sect283k1);
  if(RET_OK !=  base_test())
  {
    TRACE_MSG(TRACE_APP1, "Test: Failed", (FMT__0));
    return RET_ERROR;
  }

  if(RET_OK != extended_test(1, _cert_dev1, private_dev1, _public_dev1, _eph_prv_key_dev1, _x_eph_pub_key_dev1, _public_dev2, _x_eph_pub_key_dev2, skey1))
  {
    TRACE_MSG(TRACE_APP1, "Test for device 1: Failed", (FMT__0));
    return RET_ERROR;
  }

  if(RET_OK != extended_test(2, _cert_dev2, private_dev2, _public_dev2, _eph_prv_key_dev2, _x_eph_pub_key_dev2, _public_dev1, _x_eph_pub_key_dev1, skey2))
  {
    TRACE_MSG(TRACE_APP1, "Test for device 2: Failed", (FMT__0));
    return RET_ERROR;
  }

  if(RET_OK != confirm_key_response(skey_giv))
  {
    TRACE_MSG(TRACE_APP1, "Test: Failed", (FMT__0));

    return RET_ERROR;
  }

  TRACE_MSG(TRACE_APP1, "All tests: Passed", (FMT__0));
}


void zboss_signal_handler(zb_uint8_t param)
{
  ZVUNUSED(param);
}

#else

MAIN()
{
  ARGV_UNUSED;
/*  while (1)
  {
  }*/
}

#endif

/*! @} */
