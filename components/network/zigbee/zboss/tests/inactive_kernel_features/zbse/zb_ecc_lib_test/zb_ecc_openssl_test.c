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

#define ZB_TRACE_FILE_ID 40948
#include "zb_ecc_lib_test.h"
#include "zb_ecc.h"
#include "data.h"

#include "zb_debug.h"

#include <openssl/obj_mac.h>
#include <openssl/ec.h>
#include <openssl/bn.h>

/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifdef 1

void zb_ecc_get_random(zb_uint8_t *rnd)
{
  for (zb_uint8_t j = 0 ; j < ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES; j++)
  {
    *(rnd + j) = (zb_random() >> 4) & 0xff;
  }
}

zb_uint8_t key_generation_openssl(EC_GROUP *curve, zb_uint8_t *prv_key, zb_uint8_t *pub_key)
{
  BIGNUM *bn_prv_key;
  EC_POINT *ecp_pub_key;
  // BIGNUM *bn_pub_key;
  BN_CTX *ctx;
  // zb_uint8_t *output;

  zb_uint8_t len;

  if (NULL == (ctx = BN_CTX_new()))
  {
    // handleErrors("Can't generate context\n");
    return RET_ERROR;
  }

  if (NULL == (ecp_pub_key = EC_POINT_new(curve)))
  {
    return RET_ERROR;
  }

  if (NULL == (bn_prv_key = BN_bin2bn(prv_key, 36, NULL)))
  {
    // handleErrors("Can't set private key value\n");
    return RET_ERROR;
  }

  // if (NULL == (given_pub_bn = BN_bin2bn(pub, 22, NULL)))
  // {
  //   // handleErrors("Can't set public key value\n");
  //   return RET_ERROR;
  // }

  // if (1 != EC_POINT_set_affine_coordinates_GF2m(curve, generator, x, y, ctx))
  // {
  //   // handleErrors("Can't initialize a generator point\n");
  //   return RET_ERROR;
  // }

  /* use method to public key calculation from OpenSSL wiki */
  if (1 != EC_POINT_mul(curve, ecp_pub_key, bn_prv_key, NULL, NULL, ctx))
  {
    // handleErrors("Can't generate public key by private key\n");
    return RET_ERROR;
  }
  // printf("Pub key:\n");
  // output = EC_POINT_point2hex(curve, ecp_pub_key, POINT_CONVERSION_COMPRESSED, ctx);
  // printf("%s\n", output);

  len = EC_POINT_point2oct(curve, ecp_pub_key, POINT_CONVERSION_COMPRESSED, pub_key, 40, ctx);

  BN_free(bn_prv_key);
  EC_POINT_free(ecp_pub_key);
  BN_CTX_free(ctx);

  // BN_free(bn_pub_key);

  // OPENSSL_free(output);

  return RET_OK;
}

MAIN()
{

  zb_uint32_t j;
  zb_uint8_t curve_id;

  zb_uint8_t zb_ecc_prv_key[ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES];
  zb_uint8_t zb_ecc_pub_key[ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES];
  zb_uint8_t openssl_prv_key[ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES];
  zb_uint8_t openssl_pub_key[ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES];
  zb_uint32_t counter = 0;
  zb_uint8_t flag;

  EC_GROUP *curve;


  ARGV_UNUSED;

  if(NULL == (curve = EC_GROUP_new_by_curve_name(NID_sect283k1)))
    {
      return RET_ERROR;
      // handleErrors("Sect283k1 creation failed\n");
    }

  ZB_INIT("zb_ecc_openssl_test");

  curve_id = sect283k1;
  zb_ecc_set_curve(curve_id);
  for(j = 0; j <= 200000; j++)
  {
    if(j % 1000 == 0)
      printf("Iteration:%d, errors:%d\n", j, counter);


    // ZB_SET_TRACE_OFF();
    zb_ecc_get_random(zb_ecc_prv_key);

    ZB_MEMSET(zb_ecc_pub_key, 0xff, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);
    ZB_MEMSET(openssl_pub_key, 0xff, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);

    zb_ecc_key_generation(zb_ecc_prv_key, zb_ecc_prv_key, zb_ecc_pub_key);

    ZB_MEMCPY(openssl_prv_key, zb_ecc_prv_key, ZB_ECC_PRIVATE_KEY_LEN[curve_id]);

    key_generation_openssl(curve, openssl_prv_key, openssl_pub_key);

    if(0 != ZB_MEMCMP(zb_ecc_pub_key, openssl_pub_key, ZB_ECC_PUBLIC_KEY_LEN[curve_id]))
    {
      TRACE_MSG(TRACE_APP1, "zb_ecc public key'!=' openssl public key", (FMT__0));

      DUMP_TRAF("zb_ecc public key  -->", zb_ecc_pub_key, ZB_ECC_PUBLIC_KEY_LEN[curve_id], 0);
      DUMP_TRAF("openssl public key -->", openssl_pub_key, ZB_ECC_PUBLIC_KEY_LEN[curve_id], 0);

      TRACE_MSG(TRACE_APP1, "Public key comparison test: Failed", (FMT__0));

      flag = 1;
      counter++;
      // return RET_ERROR;
    }
    else
    {
      flag = 0;
      DUMP_TRAF("zb_ecc public key  -->", zb_ecc_pub_key, ZB_ECC_PUBLIC_KEY_LEN[curve_id], 0);
      DUMP_TRAF("openssl public key -->", openssl_pub_key, ZB_ECC_PUBLIC_KEY_LEN[curve_id], 0);

      TRACE_MSG(TRACE_APP1, "zb_ecc public key '==' openssl public key", (FMT__0));

      TRACE_MSG(TRACE_APP1, "Public key comparison test: Passed", (FMT__0));
    }

    if(0 != ZB_MEMCMP(zb_ecc_prv_key, openssl_prv_key, ZB_ECC_PRIVATE_KEY_LEN[curve_id]))
    {
      TRACE_MSG(TRACE_APP1, "zb_ecc private key'!=' openssl private key", (FMT__0));

      DUMP_TRAF("zb_ecc public key  -->", zb_ecc_prv_key, ZB_ECC_PRIVATE_KEY_LEN[curve_id], 0);
      DUMP_TRAF("openssl public key -->", openssl_prv_key, ZB_ECC_PRIVATE_KEY_LEN[curve_id], 0);

      TRACE_MSG(TRACE_APP1, "Private key comparison test: Failed", (FMT__0));

      flag = 1;
      counter++;
      // return RET_ERROR;
    }
    else
    {
      flag = 0;
      DUMP_TRAF("zb_ecc private key  -->", zb_ecc_prv_key, ZB_ECC_PRIVATE_KEY_LEN[curve_id], 0);
      DUMP_TRAF("openssl private key -->", openssl_prv_key, ZB_ECC_PRIVATE_KEY_LEN[curve_id], 0);

      TRACE_MSG(TRACE_APP1, "zb_ecc private key '==' openssl private key", (FMT__0));

      TRACE_MSG(TRACE_APP1, "Private key comparison test: Passed", (FMT__0));
    }

    ZB_SET_TRACE_ON();
    //TRACE_MSG(TRACE_APP1, "ERROR_STRING:%s", (FMT__0, ERROR_STRING));
    if(0 == flag)
    {
      TRACE_MSG(TRACE_APP1, ">>> Unit:[#%d], Status: Passed", (FMT__D, j));
    }
    else
    {
      TRACE_MSG(TRACE_APP1, ">>> Unit:[#%d], Status: Failed", (FMT__D, j));
    }
  }

  if(counter)
  {
    TRACE_MSG(TRACE_APP1, "Test: Failed, error(s):%d", (FMT__D, counter));
  }
  else
  {
    TRACE_MSG(TRACE_APP1, "Test: Passed", (FMT__0));
  }

  EC_GROUP_free(curve);

  return RET_OK;
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
