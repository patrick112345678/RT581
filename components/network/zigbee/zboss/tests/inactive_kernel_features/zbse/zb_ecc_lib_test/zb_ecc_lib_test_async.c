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

#define ZB_TRACE_FILE_ID 40944
#include "zb_ecc_lib_test.h"
#include "zb_ecc.h"
#include "zb_ecc_internal.h"

#include "zb_debug.h"


/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifdef 1

#define PRV_LEN 21

#define ZB_ECC_PRIVATE_KEY_LEN(x) (x == sect163k1 ? 21 : 36)
#define ZB_ECC_PUBLIC_KEY_LEN(x) (x == sect163k1 ? 22 : 37)

zb_uint8_t _public_dev1[ZB_ECC_PUBLIC_KEY_LEN(sect163k1)] = {0x03, 0x02, 0x90, 0xa1, 0xf5, 0xc0, 0x8d, 0xad,
                                                             0x5f, 0x29, 0x45, 0xe3, 0x35, 0x62, 0x0c, 0x7a,
                                                             0x98, 0xfa, 0xc4, 0x66, 0x66, 0xa1
                                                            };

zb_uint8_t _public_dev2[ZB_ECC_PUBLIC_KEY_LEN(sect163k1)] = {0x03, 0x02, 0x5B, 0xBA, 0x38, 0xD0, 0xC7, 0xB5,
                                                             0x43, 0x6B, 0x68, 0xDF, 0x72, 0x8F, 0x09, 0x3E,
                                                             0x7A, 0x1D, 0x6C, 0x43, 0x7E, 0x6D
                                                            };

zb_uint8_t *public_keys[2] = {_public_dev1, _public_dev2};

zb_uint8_t private_dev1[ZB_ECC_PRIVATE_KEY_LEN(sect163k1)] = {0x00, 0xb8, 0xa9, 0x00, 0xfc, 0xad, 0xeb, 0xab,
                                                              0xbf, 0xa3, 0x83, 0xb5, 0x40, 0xfc, 0xe9, 0xed,
                                                              0x43, 0x83, 0x95, 0xea, 0xa7
                                                             };


zb_uint8_t private_dev2[ZB_ECC_PRIVATE_KEY_LEN(sect163k1)] = {0x01, 0xE9, 0xDD, 0xB5, 0x58, 0x0C, 0xF7, 0x2E,
                                                              0xCE, 0x7F, 0x21, 0x5F, 0x0A, 0xE5, 0x94, 0xE4,
                                                              0x8D, 0xF3, 0xE7, 0xFE, 0xE8
                                                             };

zb_uint8_t *private_keys[2] = {private_dev1, private_dev2};

/*03 D4 8C 72 10 DD BC C4 FB 2E 5E 7A 0A A1 6A 0D B8 95 40 82 0B*/
/*Ephimerial private key for device 1.*/
zb_uint8_t _eph_prv_key_dev1[ZB_ECC_PRIVATE_KEY_LEN(sect163k1)] = {0x03, 0xD4, 0x8C, 0x72, 0x10, 0xDD, 0xBC, 0xC4,
                                                                   0xFB, 0x2E, 0x5E, 0x7A, 0x0A, 0xA1, 0x6A, 0x0D,
                                                                   0xB8, 0x95, 0x40, 0x82, 0x0B
                                                                  };

/*Original sequence:0x00,0x13,0xD3,0x6D,0xE4,0xB1,0xEA,0x8E,0x22,0x73,0x9C,0x38,0x13,0x70,0x82,0x3F,0x40,0x4B,0xFF,0x88,0x62*/
/*Ephimerial private key for device 2.*/
zb_uint8_t _eph_prv_key_dev2[ZB_ECC_PRIVATE_KEY_LEN(sect163k1)] = {0x00, 0x13, 0xD3, 0x6D, 0xE4, 0xB1, 0xEA, 0x8E,
                                                                   0x22, 0x73, 0x9C, 0x38, 0x13, 0x70, 0x82, 0x3F,
                                                                   0x40, 0x4B, 0xFF, 0x88, 0x62
                                                                  };

zb_uint8_t *eph_prv_keys[2] = {_eph_prv_key_dev1, _eph_prv_key_dev2};

/* Ephemeral public key for device 1 (responder) in compressed form.
 * Bytes order inversed. Constants took from 'C.5.2.3 Ephemeral Data Request'.
 */
/*FIXME: Provide original sequence of Ephimerial key*/
/*Original sequence: 0x03,0x06,0xAB,0x52,0x06,0x22,0x01,0xD9,0x95,0xB8,0xB8,0x59,0x1F,0x3F,0x08,0x6A,0x3A,0x2E,0x21,0x4D,0x84,0x5E
*/
zb_uint8_t _x_eph_pub_key_dev1[ZB_ECC_PUBLIC_KEY_LEN(sect163k1)] = {0x03, 0x06, 0xAB, 0x52, 0x06, 0x22, 0x01, 0xD9,
                                                                    0x95, 0xB8, 0xB8, 0x59, 0x1F, 0x3F, 0x08, 0x6A,
                                                                    0x3A, 0x2E, 0x21, 0x4D, 0x84, 0x5E
                                                                   };
/**
 * Ephimerial public and private keys for device 2 (initiator).
 * Bytes order inversed. Constants took from 'C.5.3.4 Responder Transform'.
 */

/*Ephimerial public key for device 1 in compressed form.*/
/*Original sequence is: 0x03,0x00,0xE1,0x17,0xC8,0x6D,0x0E,0x7C,0xD1,0x28,0xB2,0xF3,0x4E,0x90,0x76,0xCF,0xF2,0x4A,0xF4,0x6D,0x72,0x88*/
zb_uint8_t _x_eph_pub_key_dev2[ZB_ECC_PUBLIC_KEY_LEN(sect163k1)] = {0x03, 0x00, 0xE1, 0x17, 0xC8, 0x6D, 0x0E, 0x7C,
                                                                    0xD1, 0x28, 0xB2, 0xF3, 0x4E, 0x90, 0x76, 0xCF,
                                                                    0xF2, 0x4A, 0xF4, 0x6D, 0x72, 0x88
                                                                   };

zb_uint8_t *eph_pub_keys[2] = {_x_eph_pub_key_dev1, _x_eph_pub_key_dev2};

zb_uint8_t _cert_dev1[48] = {0x03, 0x04, 0x5F, 0xDF, 0xC8, 0xD8, 0x5F, 0xFB,
                             0x8B, 0x39, 0x93, 0xCB, 0x72, 0xDD, 0xCA, 0xA5,
                             0x5F, 0x00, 0xB3, 0xE8, 0x7D, 0x6D, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x54, 0x45,
                             0x53, 0x54, 0x53, 0x45, 0x43, 0x41, 0x01, 0x09,
                             0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                            };

/*See 'C.5.1.3 Initiator Data'*/
zb_uint8_t _cert_dev2[ZB_ECC_MAX_CERT_LEN] = {0x02, 0x06, 0x15, 0xE0, 0x7D, 0x30, 0xEC, 0xA2,
                                              0xDA, 0xD5, 0x80, 0x02, 0xE6, 0x67, 0xD9, 0x4B,
                                              0xC1, 0xB4, 0x22, 0x39, 0x83, 0x07, 0x00, 0x00,
                                              0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x54, 0x45,
                                              0x53, 0x54, 0x53, 0x45, 0x43, 0x41, 0x01, 0x09,
                                              0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                                             };

zb_uint8_t *certs[2] = {_cert_dev1, _cert_dev2};

/*See 'C.5.1.1 CA Public Key'*/
/*original sequence {0x02,0x00,0xFD,0xE8,0xA7,0xF3,0xD1,0x08,0x42,0x24,0x96,0x2A,0x4E,0x7C,0x54,0xE6,0x9A,0xC3,0xF0,0x4D,0xA6,0xB8}*/
zb_uint8_t CA_pub_key[ZB_ECC_PUBLIC_KEY_LEN(sect163k1)] = {0x02, 0x00, 0xFD, 0xE8, 0xA7, 0xF3, 0xD1, 0x08,
                                                           0x42, 0x24, 0x96, 0x2A, 0x4E, 0x7C, 0x54, 0xE6,
                                                           0x9A, 0xC3, 0xF0, 0x4D, 0xA6, 0xB8
                                                          };


/*Given shared key. See 'C.5.3.4.3 Detailed Steps'*/
zb_uint8_t skey_giv[24] = {0x00, 0xE0, 0xD2, 0xC3, 0xCC, 0xD5, 0xC1, 0x06,
                           0xA8, 0x9C, 0x4F, 0x6C, 0xC2, 0x6A, 0x5F, 0x7E,
                           0xC9, 0xDF, 0x78, 0xA7, 0xBE, 0x00, 0x00, 0x00
                          };

zb_ecc_point_aff_t G163k1 = {{0x5C94EEE8UL, 0xDE4E6D5EUL, 0xAA07D793UL, 0x7BBC11ACUL, 0xFE13C053UL, 0x2UL, 0x0UL, 0x0UL},
    {0xCCDAA3D9UL, 0x0536D538UL, 0x321F2E80UL, 0x5D38FF58UL, 0x89070FB0UL, 0x2UL, 0x0UL, 0x0UL}
};

typedef struct test_ctx_s
{
    zb_uint8_t prv_key[ZB_ECC_PRIVATE_KEY_LEN(sect163k1)];
    zb_uint8_t pub_key[ZB_ECC_PUBLIC_KEY_LEN(sect163k1)];
    zb_ecc_point_aff_t point;
    zb_uint8_t buff[ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES];
    zb_uint8_t skey[ZB_ECC_PRIVATE_KEY_LEN(sect163k1)];
    // zb_uint8_t dev_ids_list[2] = {1, 2};
    zb_uint8_t dev_pairs[2][2]; //store indexes of public_keys, private_keys, eph_prv_keys and etc for different devices
    zb_uint8_t dev_pairs_len;
    zb_uint8_t dev_pair;
} test_ctx_t;

test_ctx_t test_ctx;

void test_finish(void);

void base_test_finish(zb_uint8_t param);

void test_key_generation(zb_uint8_t pararm);

void test_key_generation_finish(zb_uint8_t pararm);

void initiate_key_establishment_request_finish(zb_uint8_t param);

void generate_shared_secret(zb_uint8_t param);

void generate_shared_secret_finish(zb_uint8_t param);

void initiate_key_establishment_request(zb_uint8_t param);

/**
 * dump [size] bytes to screen
 * @param ptr  - array of [size] bytes to output
 * @param size - size in bytes of [ptr]
 */
void dump_bytes(zb_uint8_t *ptr, size_t size, zb_char_t *msg)
{
    printf("\n%s\n", msg);
    for (zb_uint8_t i = 0; i < size; i++)
    {
        printf("%02X ", ptr[i]);
        if (15 == i % 16)
        {
            printf("\n");
        }
    }
    if (size % 16 > 0)
    {
        printf("\n");
    }
}


void zb_ecc_get_pseudo_random(zb_uint8_t *rnd, zb_uint8_t size)
{
    for (zb_uint8_t i = 0; i < size; i++)
    {
        rnd[i] = i + 1;
    }
}


#define GET_DEV_ID(pos)  test_ctx.dev_pairs[test_ctx.dev_pair][pos]
#define NEXT_TEST() {if (test_ctx.dev_pair < test_ctx.dev_pairs_len - 1) test_ctx.dev_pair++; }


/**
 * Function for basic test of key generation, computing and comparing public keys.
 * @return  RET_OK in success, RET_ERROR in fail.
 */
void base_test(zb_uint8_t param)
{
    ZVUNUSED(param);
    ZB_MEMSET(&test_ctx.point, 0, sizeof(test_ctx.point));
    zb_ecc_get_pseudo_random(test_ctx.prv_key, PRV_LEN);
    zb_ecc_key_generation_async(base_test_finish, param, test_ctx.prv_key, test_ctx.prv_key, test_ctx.pub_key);

    // TRACE_MSG(TRACE_APP1, "Private key after generation", (FMT__0));
    // DUMP_TRAF("Private key-->", (zb_uint8_t *)key.d, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES, 0);
    //
    // TRACE_MSG(TRACE_APP1, "Public key after generation", (FMT__0));
    // DUMP_TRAF("Public key-->", (zb_uint8_t *)key.Qx, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES, 0);
}


void base_test_finish(zb_uint8_t param)
{

    ZVUNUSED(param);

    ZB_MEMCPY(test_ctx.buff, test_ctx.pub_key, ZB_ECC_PUBLIC_KEY_LEN[sect163k1]);
    zb_ecc_inverse_bytes(test_ctx.buff, ZB_ECC_PUBLIC_KEY_LEN[sect163k1]);

    if (RET_OK != zb_ecc_point_decompression((zb_uint32_t *)test_ctx.buff, &test_ctx.point))
    {
        TRACE_MSG(TRACE_APP1, "Have error in point decompression", (FMT__0));

        TRACE_MSG(TRACE_APP1, "Private key after generation", (FMT__0));
        DUMP_TRAF("Private key-->", test_ctx.prv_key, ZB_ECC_PRIVATE_KEY_LEN[sect163k1], 0);

        TRACE_MSG(TRACE_APP1, "Public key after generation", (FMT__0));
        DUMP_TRAF("Public key-->", test_ctx.pub_key, ZB_ECC_PUBLIC_KEY_LEN[sect163k1], 0);

        TRACE_MSG(TRACE_APP1, "Test: Failed", (FMT__0));
        ZB_ASSERT(0);
    }
    else
    {
        TRACE_MSG(TRACE_APP1, "Decompress public key successfully", (FMT__0));

        if (ZB_PK_VALID != (zb_ecc_pk_validation(&test_ctx.point)))
        {
            TRACE_MSG(TRACE_APP1, "Have error in generated public key validation", (FMT__0));

            TRACE_MSG(TRACE_APP1, "Test: Failed", (FMT__0));
            ZB_ASSERT(0);
        }
        else
        {
            TRACE_MSG(TRACE_APP1, "Generated public key validation is finished successfully", (FMT__0));
            TRACE_MSG(TRACE_APP1, "Base test: Passed", (FMT__0));
        }
    }
    //FIXME: Do we need to schedule the follow function?
    test_key_generation(0);
}


zb_ret_t confirm_key_response(zb_uint8_t *skey)
{
    zb_uint8_t mac_key[16] = {0x90, 0xF9, 0x67, 0xB2, 0x2C, 0x83, 0x57, 0xC1,
                              0x0C, 0x1C, 0x04, 0x78, 0x8D, 0xE9, 0xE8, 0x48
                             };
    /*zb_uint8_t mac_key[16];*/
    zb_uint8_t message[61] = {0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                              0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                              0x02, 0x03, 0x06, 0xab, 0x52, 0x06, 0x22, 0x01,
                              0xd9, 0x95, 0xb8, 0xb8, 0x59, 0x1f, 0x3f, 0x08,
                              0x6a, 0x3a, 0x2e, 0x21, 0x4d, 0x84, 0x5e, 0x03,
                              0x00, 0xe1, 0x17, 0xc8, 0x6d, 0x0e, 0x7c, 0xd1,
                              0x28, 0xb2, 0xf3, 0x4e, 0x90, 0x76, 0xcf, 0xf2,
                              0x4a, 0xf4, 0x6d, 0x72, 0x88
                             };

    zb_uint8_t macv[16] = {0x79, 0xD5, 0xF2, 0xAD, 0x1C, 0x31, 0xD4, 0xD1,
                           0xEE, 0x7C, 0xB7, 0x19, 0xAC, 0x68, 0x3C, 0x3C
                          };

    /*zb_uint8_t message[61] = {0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                              0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                              0x01,0x03,0x00,0xe1,0x17,0xc8,0x6d,0x0e,
                              0x7c,0xd1,0x28,0xb2,0xf3,0x4e,0x90,0x76,
                              0xcf,0xf2,0x4a,0xf4,0x6d,0x72,0x88,0x03,
                              0x06,0xab,0x52,0x06,0x22,0x01,0xd9,0x95,
                              0xb8,0xb8,0x59,0x1f,0x3f,0x08,0x6a,0x3a,
                              0x2e,0x21,0x4d,0x84,0x5e,0x88};
    */
    zb_buf_t *hash_key = ZB_GET_ANY_BUF();
    zb_uint8_t *p;
    zb_uint8_t buff[25];
    zb_uint8_t id[4] = {0x00, 0x00, 0x00, 0x01};
    zb_ret_t ret_val = RET_ERROR;

    ZB_MEMCPY(buff, skey, 21);
    ZB_MEMCPY(buff + 21, id, 4);
    zb_sec_b6_hash(buff, 25, mac_key);

    // DUMP_TRAF("MacKey-->", (zb_uint8_t *)mac_key, 16, 0);
    // DUMP_TRAF("Message-->", (zb_uint8_t *)message, 61, 0);
    zb_ecc_key_hmac(mac_key, message, 61, hash_key);
    // DUMP_TRAF("MACV-->", (zb_uint8_t *)hash_key, 16, 0);

    p = ZB_BUF_BEGIN(hash_key);
    if (16 < ZB_BUF_LEN(hash_key))
    {
        if (0 == ZB_MEMCMP(p, macv, 16))
        {
            ret_val = RET_OK;
        }
        else
        {
            DUMP_TRAF("Calculated MAC(V) -->", p, 16, 0);
            DUMP_TRAF("Given MAC(V) -->", macv, 16, 0);
        }
    }
    else
    {
        TRACE_MSG(TRACE_APP1, "ZB_BUF_LEN less than 16 bytes", (FMT__0));
    }

    TRACE_MSG(TRACE_APP1, "Calculated MAC(V) '%s' given MAC(V)", (FMT__P, (RET_OK == ret_val) ? "==" : "!="));
    TRACE_MSG(TRACE_APP1, "MAC(V) comparison test: %s", (FMT__P, (RET_OK == ret_val) ? "Passed" : "Failed"));

    zb_free_buf(hash_key);
    return ret_val;
}


void dump_dwords(zb_uint8_t *ptr, size_t size, zb_char_t *msg)
{
    zb_uint8_t len;

    len = (size % 4 > 0) ? size / 4 + 1 : size / 4;
    printf("\n%s\n", msg);

    for (zb_uint8_t i = 0; i < len; i++)
    {
        printf("%08X ", *((zb_uint32_t *)ptr + i));
        if (3 == i % 4)
        {
            printf("\n");
        }
    }
    if (len % 4 > 0)
    {
        printf("\n");
    }

}


void test_key_generation(zb_uint8_t param)
{
    // zb_uint8_t pub_key[ZB_ECC_PUBLIC_KEY_LEN[sect163k1]];
    // zb_uint8_t buff[ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES];
    // zb_ecc_point_aff_t point;

    ZVUNUSED(param);

    zb_ecc_key_generation_async(test_key_generation_finish, param,
                                private_keys[GET_DEV_ID(0)], private_keys[GET_DEV_ID(0)],
                                test_ctx.pub_key);
}


void test_key_generation_finish(zb_uint8_t param)
{
    ZVUNUSED(param);


    if (0 != ZB_MEMCMP(test_ctx.pub_key, public_keys[GET_DEV_ID(0)], ZB_ECC_PUBLIC_KEY_LEN[sect163k1]))
    {
        TRACE_MSG(TRACE_APP1, "Calculated public key '!=' expected public key", (FMT__0));

        DUMP_TRAF("Calculated public key-->", test_ctx.pub_key,  ZB_ECC_PUBLIC_KEY_LEN[sect163k1], 0);

        DUMP_TRAF("Expected public key:-->", public_keys[GET_DEV_ID(0)],  ZB_ECC_PUBLIC_KEY_LEN[sect163k1], 0);

        TRACE_MSG(TRACE_APP1, "Test: Failed", (FMT__0));
        ZB_ASSERT(0);
    }
    else
    {
        TRACE_MSG(TRACE_APP1, "Calculated public key '==' expected public key", (FMT__0));

        /*NOTE: do decompression function need to reverse bytes own?*/
        ZB_MEMSET(test_ctx.buff, 0, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);
        ZB_MEMCPY(test_ctx.buff, test_ctx.pub_key, ZB_ECC_PUBLIC_KEY_LEN[sect163k1]);
        zb_ecc_inverse_bytes(test_ctx.buff, ZB_ECC_PUBLIC_KEY_LEN[sect163k1]);
        if (RET_OK != zb_ecc_point_decompression((zb_uint32_t *)test_ctx.buff, &test_ctx.point))
        {
            TRACE_MSG(TRACE_APP1, "Have error in point_decompression", (FMT__0));

            TRACE_MSG(TRACE_APP1, "Test: Failed", (FMT__0));
            ZB_ASSERT(0);
        }
        else
        {
            TRACE_MSG(TRACE_APP1, "Decompress public key successfully", (FMT__0));
            // DUMP_TRAF("Calculated public key, X coordinate:-->", (zb_uint8_t *)&point.x, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES, 0);
            // DUMP_TRAF("Calculated public key, Y coordinate:-->", (zb_uint8_t *)&point.y, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES, 0);

            if (ZB_PK_VALID != zb_ecc_pk_validation(&test_ctx.point))
            {
                TRACE_MSG(TRACE_APP1, "Have error in given public key validation", (FMT__0));
                /*FIXME: Fix public key validation procedure for sect163k1 curve*/
                // return RET_ERROR;

                TRACE_MSG(TRACE_APP1, "Test: Failed", (FMT__0));
                ZB_ASSERT(0);
            }
            else
            {
                TRACE_MSG(TRACE_APP1, "Given public key validation is finished successfully", (FMT__0));
            }
        }
    }

    //FIXME: need to check res and make assert
    initiate_key_establishment_request(0);
    // return RET_OK;
}

/**
 * See 'C.5.2.1 Initiate Key Establishment Request'
 * @param  public_key_comp - non-initialized device 2(initiator) publik key
 * @return RET_OK in sucess, and RET_ERROR in fail.
 */
void initiate_key_establishment_request(zb_uint8_t param)
{
    ZVUNUSED(param);
    zb_ecc_ecqv_extraction_async(initiate_key_establishment_request_finish, param,
                                 CA_pub_key, certs[GET_DEV_ID(0)], certs[GET_DEV_ID(0)], test_ctx.pub_key);
}


void initiate_key_establishment_request_finish(zb_uint8_t param)
{
    if (RET_OK != param)
    {
        TRACE_MSG(TRACE_APP1, "Have error in public key extraction", (FMT__0));

        TRACE_MSG(TRACE_APP1, "Test: Failed", (FMT__0));
        ZB_ASSERT(0);
    }
    else
    {
        TRACE_MSG(TRACE_APP1, "Public key extraction is finished successfully", (FMT__0));

        if (0 != ZB_MEMCMP(test_ctx.pub_key, public_keys[GET_DEV_ID(0)], ZB_ECC_PUBLIC_KEY_LEN[sect163k1]))
        {
            TRACE_MSG(TRACE_APP1, "Extracted public key '!=' expected public key", (FMT__0));

            DUMP_TRAF("Extracted public key-->", test_ctx.pub_key, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES, 0);
            DUMP_TRAF("Expected public key-->", public_keys[GET_DEV_ID(0)], ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES, 0);

            TRACE_MSG(TRACE_APP1, "Test: Failed", (FMT__0));
            ZB_ASSERT(0);
        }
        else
        {
            TRACE_MSG(TRACE_APP1, "Extracted public key '==' expected public key", (FMT__0));
        }
    }

    generate_shared_secret(0);
    // return RET_OK;
}

/**
 * Generate shared secret for device 1(responder) between device1(initiator)
 * and device 2(responder). See 'C.5.3.4 Responder Transform', 'C.5.3.4.3 Detailed Steps'.
 * @param  x_pkey_dev1 - public key for device 1 in compressed form
 * @return             RET_OK in success, RET_ERROR in fail
 */
void generate_shared_secret(zb_uint8_t param)
{
    ZVUNUSED(param);
    // ZB_MEMSET(skey1, 0, 21);
    ZB_MEMSET(test_ctx.skey, 0, 21);
    zb_ecc_ecmqv_async(generate_shared_secret_finish, param, private_keys[GET_DEV_ID(0)],
                       eph_prv_keys[GET_DEV_ID(0)], eph_pub_keys[GET_DEV_ID(0)],
                       public_keys[GET_DEV_ID(1)], eph_pub_keys[GET_DEV_ID(1)], test_ctx.skey);
}


void generate_shared_secret_finish(zb_uint8_t param)
{
    if (RET_OK != param)
    {
        TRACE_MSG(TRACE_APP1, "Shared key generation failed", (FMT__0));

        /*FIXME: within 36 use a macros*/
        DUMP_TRAF("Shared key between device 1 and device 2 is-->", test_ctx.skey, 21, 0);
        DUMP_TRAF("Given shared key between device 1 and device 2 is-->", skey_giv, 21, 0);

        TRACE_MSG(TRACE_APP1, "Test: Failed", (FMT__0));
        ZB_ASSERT(0);
        // return RET_ERROR;
    }
    else
    {
        TRACE_MSG(TRACE_APP1, "Shared key generation is finished successfully", (FMT__0));

        TRACE_MSG(TRACE_ZCL1, "> Start shared key comparison", (FMT__0));
        if (0 != ZB_MEMCMP(test_ctx.skey, skey_giv, 21))
        {
            TRACE_MSG(TRACE_APP1, "Shared key for device %d '!=' given shared key", (FMT__H, GET_DEV_ID(0)));

            DUMP_TRAF("Shared key for device -->", test_ctx.skey, 21, 0);
            DUMP_TRAF("Given shared key -->", skey_giv, 21, 0);

            TRACE_MSG(TRACE_APP1, "Shared key comparison test: Failed", (FMT__0));

            TRACE_MSG(TRACE_APP1, "Test: Failed", (FMT__0));
            ZB_ASSERT(0);
            // return RET_ERROR;
        }
        else
        {
            TRACE_MSG(TRACE_APP1, "Shared key for device %d '==' given shared key", (FMT__H, GET_DEV_ID(0)));

            TRACE_MSG(TRACE_APP1, "Shared key comparison test: Passed", (FMT__0));
        }

        TRACE_MSG(TRACE_APP1, "Extended test for device %d: Passed", (FMT__H, GET_DEV_ID(0)));

        // return RET_OK;
    }

    if (test_ctx.dev_pair < test_ctx.dev_pairs_len - 1)
    {
        NEXT_TEST();
        test_key_generation(0);
    }
    else
    {
        // if(RET_OK != confirm_key_response(skey_giv))
        // {
        //   TRACE_MSG(TRACE_APP1, "Test: Failed", (FMT__0));
        //   return RET_ERROR;
        // }
        //
        TRACE_MSG(TRACE_APP1, "All tests: Passed", (FMT__0));
        ZB_ASSERT(0);
    }
}


MAIN()
{

    zb_uint8_t pairs[2][2] = {{0, 1}, {1, 0}};
    ZB_MEMCPY(test_ctx.dev_pairs, pairs, sizeof(pairs));
    test_ctx.dev_pairs_len = 2;
    test_ctx.dev_pair = 0;

    ARGV_UNUSED;
    ZB_INIT("zb_ecc_lib_test_async");

    // zb_sched_init();
    zb_ecc_set_curve(sect163k1);
    ZB_SCHEDULE_CALLBACK(base_test, 0);
    // zb_sched_loop_iteration();
    // zboss_main_loop();
    while (1) //FIXME: use flag to itterupt loop
    {
        zb_sched_loop_iteration();
    }

    TRACE_DEINIT();
    MAIN_RETURN(0);
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
