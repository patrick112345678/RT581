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

#define ZB_TRACE_FILE_ID 40950
#include "zb_ecc_lib_test.h"
#include "zb_ecc.h"
#include "zb_ecc_internal.h"
#include "data.h"

#include "zb_debug.h"


/*! \addtogroup ZB_TESTS */
/*! @{ */

#if 1

zb_uint8_t ERROR_STRING[14];
zb_uint8_t pos = 0;
zb_uint8_t curve_id;

#define SET_SUCCESS() {if(pos < 13) ERROR_STRING[pos++] = '1';}
#define SET_FAILED() {if(pos < 13) ERROR_STRING[pos++] = '0';}
#define CLEAR_ERRORS() {ZB_MEMSET(ERROR_STRING, '0', 13); ERROR_STRING[13] = '\0'; pos = 0;}

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


void zb_ecc_get_random(zb_uint8_t *rnd)
{
  for (zb_uint8_t j = 0 ; j < ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES; j++)
  {
    *(rnd + j) = (zb_random() >> 4) & 0xff;
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

/**
 * Test public key generation procedure by given private key
 * @param  prv_key     pivate key
 * @param  exp_pub_key expected public key
 * @return             RET_OK by sucess, otherwise RET_ERROR
 */
zb_uint8_t test_key_generation(zb_uint8_t *prv_key, zb_uint8_t *exp_pub_key)
{
  zb_uint8_t pub_key[ZB_ECC_PUBLIC_KEY_LEN[curve_id]];
  zb_ecc_point_aff_t point;
  zb_uint32_t buff[ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS];

  ZB_MEMSET(&point, 0, sizeof(zb_ecc_point_aff_t));

  zb_ecc_key_generation(prv_key, prv_key, pub_key);

  if(0 != ZB_MEMCMP(pub_key, exp_pub_key, ZB_ECC_PUBLIC_KEY_LEN[curve_id]))
  {
    TRACE_MSG(TRACE_APP1, "Calculated public key '!=' expected public key", (FMT__0));

    DUMP_TRAF("Calculated public key-->", pub_key, ZB_ECC_PUBLIC_KEY_LEN[curve_id], 0);

    DUMP_TRAF("Expected public key-->", exp_pub_key, ZB_ECC_PUBLIC_KEY_LEN[curve_id], 0);

    SET_FAILED();
    //return RET_ERROR;
  }
  else
  {
    SET_SUCCESS();
    TRACE_MSG(TRACE_APP1, "Calculated public key '==' expected public key", (FMT__0));
  }

  ZB_MEMSET((zb_uint8_t *)buff, 0, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);
  ZB_MEMCPY((zb_uint8_t *)buff, pub_key, ZB_ECC_PUBLIC_KEY_LEN[curve_id]);
  zb_ecc_inverse_bytes((zb_uint8_t *)buff, ZB_ECC_PUBLIC_KEY_LEN[curve_id]);
  if(RET_OK != zb_ecc_point_decompression(buff, &point))
  {
    TRACE_MSG(TRACE_APP1, "Have error in point_decompression", (FMT__0));

    SET_FAILED();
    return RET_ERROR;
  }
  else
  {
    SET_SUCCESS();
    TRACE_MSG(TRACE_APP1, "Decompress public key successfully", (FMT__0));
    // DUMP_TRAF("Calculated public key, X coordinate:-->", (zb_uint8_t *)&point.x, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES, 0);
    // DUMP_TRAF("Calculated public key, Y coordinate:-->", (zb_uint8_t *)&point.y, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES, 0);
  }

  /*FIXME: If we have error or don't have error in previous step then pos will be have different values.*/
  if(ZB_PK_VALID != zb_ecc_pk_validation(&point))
  {
    TRACE_MSG(TRACE_APP1, "Have error in given public key validation", (FMT__0));

    SET_FAILED();
    return RET_ERROR;
  }
  else
  {
    SET_SUCCESS();
    TRACE_MSG(TRACE_APP1, "Given public key validation is finished successfully", (FMT__0));
  }

  return RET_OK;
}


/**
 * See 'C.5.2.1 Initiate Key Establishment Request'
 * @param  public_key_comp - non-initialized device 2(initiator) publik key
 * @return RET_OK in sucess, and RET_ERROR in fail.
 */
zb_uint8_t initiate_key_establishment_request(zb_uint8_t *exp_pub_key, zb_uint8_t *cert, zb_uint8_t *CA_pub_key)
{
  zb_uint8_t x_pub_key_dev2[ZB_ECC_PUBLIC_KEY_LEN[curve_id]];
  zb_uint8_t shift;

  switch (curve_id) {
    case sect163k1:
      shift = 0;
      break;
    case sect283k1:
      shift = 37;
      break;
    default:
      return RET_ERROR;
  }

  ZB_MEMSET(x_pub_key_dev2, 0, ZB_ECC_PUBLIC_KEY_LEN[curve_id]);

  /**
   * CA_pub_key defined in 'C.5.1.1 CA Public Key'
   * _cert_dev2 defined in 'C.5.1.3 Initiator Data'
   * public_key_comp is extracted public key from implicit certificate for
   * initiator(device 2)
   */
  if(RET_OK != zb_ecc_ecqv_extraction(CA_pub_key, (cert + shift), cert, x_pub_key_dev2))
  {
    TRACE_MSG(TRACE_APP1, "Have error in public key extraction", (FMT__0));

    SET_FAILED();
    return RET_ERROR;
  }
  else
  {
    SET_SUCCESS();
    TRACE_MSG(TRACE_APP1, "Public key extraction is finished successfully", (FMT__0));
  }

  if(0 != ZB_MEMCMP(x_pub_key_dev2, exp_pub_key, ZB_ECC_PUBLIC_KEY_LEN[curve_id]))
  {
    TRACE_MSG(TRACE_APP1, "Extracted public key '!=' expected public key", (FMT__0));

    DUMP_TRAF("Extracted public key-->", x_pub_key_dev2, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES, 0);
    DUMP_TRAF("Expected public key-->", exp_pub_key, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES, 0);

    SET_FAILED();
    return RET_ERROR;
  }
  else
  {
    SET_SUCCESS();
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
                                 zb_uint8_t *eph_pub_dev1, zb_uint8_t *pub_dev2,
                                 zb_uint8_t *eph_pub_dev2, zb_uint8_t *skey1)
{
  // zb_ecc_key_pair_t key;  /*Public and private keys received from CA for device 1(responder).*/
  // zb_ecc_key_pair_t eph_key;  /*Ephemeral public and private keys for device 1 (responder).*/
  //zb_uint8_t key_len;

  // switch (curve_id ) {
  //   case sect163k1:
  //     key_len = 21;
  //
  //     break;
  //   case sect283k1:
  //     key_len = 36;
  // }

  /*Initialize key pairs for device 1(responder).*/
  // ZB_MEMCPY(key.d, prv_dev1, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES); /*Constants took from 'C.5.1.2 Responder Data'.*/
  // ZB_MEMCPY(key.Qx, pub_dev1, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES); /*Constants took from 'C.5.1.2 Responder Data'*/
  //
  // ZB_MEMCPY(eph_key.d, eph_prv_dev1, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);
  // ZB_MEMCPY(eph_key.Qx, eph_pub_dev1, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);

  if(RET_OK != zb_ecc_ecmqv(prv_dev1, eph_prv_dev1, eph_pub_dev1, pub_dev2, eph_pub_dev2, skey1))
  {
    TRACE_MSG(TRACE_APP1, "Shared key generation failed", (FMT__0));

    /*FIXME: within 36 use a macros*/
    DUMP_TRAF("Shared key between device 1 and device 2 is-->", skey1, ZB_ECC_PRIVATE_KEY_LEN[curve_id], 0);

    SET_FAILED();
    return RET_ERROR;
  }
  else
  {
    SET_SUCCESS();
    TRACE_MSG(TRACE_APP1, "Shared key generation is finished successfully", (FMT__0));
  }

  return RET_OK;
}

zb_uint8_t extended_test(zb_uint8_t dev_id, zb_uint8_t *cert_dev1,
                         zb_uint8_t *CA_pub_key_dev1, zb_uint8_t *prv_dev1, zb_uint8_t *pub_dev1,
                         zb_uint8_t *eph_prv_dev1, zb_uint8_t *eph_pub_dev1, zb_uint8_t *pub_dev2,
                         zb_uint8_t *eph_pub_dev2, zb_uint8_t *skey1)
{

  TRACE_MSG(TRACE_ZCL1, "> Key generation for device %d", (FMT__H, dev_id));
  if(RET_OK != test_key_generation(prv_dev1, pub_dev1))
  {
    TRACE_MSG(TRACE_APP1, "Test: Failed", (FMT__0));
    //return RET_ERROR;
  }

  TRACE_MSG(TRACE_APP1, "> Extract public key for device %d", (FMT__H, dev_id));
  if(RET_OK != initiate_key_establishment_request(pub_dev1, cert_dev1, CA_pub_key_dev1))
  {
    TRACE_MSG(TRACE_APP1, "Test: Failed", (FMT__0));
    //return RET_ERROR;
  }

  TRACE_MSG(TRACE_APP1, "> Generate shared secret for device %d", (FMT__H, dev_id));
  if(RET_OK != generate_shared_secret(prv_dev1, eph_prv_dev1, eph_pub_dev1, pub_dev2, eph_pub_dev2, skey1))
  {
    TRACE_MSG(TRACE_APP1, "Test: Failed", (FMT__0));
    return RET_ERROR;
  }

  return RET_OK;
}

MAIN()
{

  zb_uint32_t i;
  // zb_uint8_t curve_id;

  zb_uint8_t eph_prv_dev1[ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES];
  zb_uint8_t eph_pub_dev1[ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES];
  zb_uint8_t eph_prv_dev2[ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES];
  zb_uint8_t eph_pub_dev2[ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES];
  //zb_uint8_t test_prv[40] = {0x81,0x7C,0x70,0xD3,0xA7,0x19,0xCD,0x6A,0x4C,0x16,0xC2,0x15,0xC7,0xE8,0x7A,0x03,0xC8,0xC0,0x86,0xEE,0x9A,0xC0,0x8F,0x91,0xFC,0x9D,0x77,0x13,0x5B,0xE4,0xC3,0xB3,0x76,0x18,0x71,0x01,0xFD,0x7F,0x00,0x00};
  //zb_uint8_t test_prv[40] = {0x81,0x7C,0x70,0xD3,0xA7,0x19,0xCD,0x6A,0x4C,0x16,0xC2,0x15,0xC7,0xE8,0x7A,0x03,0xC8,0xC0,0x86,0xEE,0x9A,0xC0,0x8F,0x91,0xFC,0x9D,0x77,0x13,0x5B,0xE4,0xC3,0xB3,0x76,0x18,0x71,0x01,0x00,0x00,0x00,0x00};
  zb_uint8_t skey1[36];
  zb_uint8_t skey2[36];
  zb_uint32_t counter = 0;


  ARGV_UNUSED;

  ZB_INIT("zb_ecc_main_lib_test");

  for(i = 0; i < TEST_COUNT; i++)
  {
    curve_id = (1 == suite_type[i]) ? sect163k1 : sect283k1;

    ZB_SET_TRACE_OFF();
    TRACE_MSG(TRACE_APP1, ">>>Test name: %s, Test [%hd], suite: %hd", (FMT__P_H_H, test_names[i], i, suite_type[i]));
    CLEAR_ERRORS();
    zb_ecc_get_random(eph_prv_dev1);
    zb_ecc_get_random(eph_prv_dev2);

    ZB_MEMSET(skey1, 0xff, 36);
    ZB_MEMSET(skey2, 0xff, 36);
    ZB_MEMSET(eph_pub_dev1, 0xff, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);
    ZB_MEMSET(eph_pub_dev2, 0xff, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);

    zb_ecc_set_curve(curve_id);

    zb_ecc_key_generation(eph_prv_dev1, eph_prv_dev1, eph_pub_dev1);
    zb_ecc_key_generation(eph_prv_dev2, eph_prv_dev2, eph_pub_dev2);


    if(RET_OK != extended_test(1, cert_dev1[i], CA_public_key_dev1[i], private_key_dev1[i], public_key_dev1[i], eph_prv_dev1, eph_pub_dev1, public_key_dev2[i], eph_pub_dev2, skey1))
    {
      TRACE_MSG(TRACE_APP1, "Test for device 1: Failed", (FMT__0));
      // return RET_ERROR;
    }

    if(RET_OK != extended_test(2, cert_dev2[i], CA_public_key_dev2[i], private_key_dev2[i], public_key_dev2[i], eph_prv_dev2, eph_pub_dev2, public_key_dev1[i], eph_pub_dev1, skey2))
    {
      TRACE_MSG(TRACE_APP1, "Test for device 2: Failed", (FMT__0));
      // return RET_ERROR;
    }

    TRACE_MSG(TRACE_ZCL1, "> Start shared key comparison", (FMT__0));
    if(0 != ZB_MEMCMP(skey1, skey2, ZB_ECC_PRIVATE_KEY_LEN[curve_id]))
    {
      TRACE_MSG(TRACE_APP1, "Shared key for device 1 '!=' shared key for device 2", (FMT__0));

      DUMP_TRAF("Shared key for device 1 -->", skey1, ZB_ECC_PRIVATE_KEY_LEN[curve_id], 0);
      DUMP_TRAF("Shared key for device 2 -->", skey2, ZB_ECC_PRIVATE_KEY_LEN[curve_id], 0);

      TRACE_MSG(TRACE_APP1, "Shared key comparison test: Failed", (FMT__0));

      SET_FAILED();
      // return RET_ERROR;
    }
    else
    {
      SET_SUCCESS();
      TRACE_MSG(TRACE_APP1, "Shared key for device 1 '==' shared key for device 2", (FMT__0));

      TRACE_MSG(TRACE_APP1, "Shared key comparison test: Passed", (FMT__0));
    }

    ZB_SET_TRACE_ON();
    //TRACE_MSG(TRACE_APP1, "ERROR_STRING:%s", (FMT__0, ERROR_STRING));
    if(0 == ZB_MEMCMP(ERROR_STRING, test_masks[i], 13))
    {
      TRACE_MSG(TRACE_APP1, ">>>Mask:%s, Unit:%s, [#%d], suite [%d], Status: Passed", (FMT__P_P_D_D, ERROR_STRING, test_names[i], i, suite_type[i]));
    }
    else
    {
      counter++;
      TRACE_MSG(TRACE_APP1, ">>>Mask:%s, Unit:%s, [#%d], suite [%d], Status: Failed", (FMT__P_P_D_D, ERROR_STRING, test_names[i], i, suite_type[i]));
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
