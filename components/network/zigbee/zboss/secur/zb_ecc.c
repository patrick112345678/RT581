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
/* PURPOSE: ZB Elliptic curve security routines
*/

#define ZB_TRACE_FILE_ID 2564
#include "zb_common.h"
#include "zb_ecc.h"
#include "zb_ecc_internal.h"
#include "zb_secur.h"

/* Constants to enable/disable async mode in the functions. */
#define ASYNC_ON (1u)
#define ASYNC_OFF (0u)

const zb_uint8_t ZB_ECC_PRIVATE_KEY_LEN[2] =
{
    21,  /* for sect163k1 */
    36  /* for sect283k1 */
};

const zb_uint8_t ZB_ECC_PUBLIC_KEY_LEN[2] =
{
    22,  /* for sect163k1 */
    37  /* for sect283k1 */
};

static const zb_uint8_t ZB_ECC_BITLEN[2] =
{
    20, /*163, for sect163k1 */
    35, /*283, for sect283k1 */
};

const zb_uint8_t ZB_ECC_PUB_KEY_LEN_IN_DWORDS[2] =
{
    6, /* for sect163k1*/
    9  /* for sect*/
};


/*DD: Set default values for sect163k1 curve*/
static zb_uint8_t g_ecc_prv_key_len = 21;
static zb_uint8_t g_ecc_pub_key_len = 32;
static zb_uint16_t g_ecc_bitlen = 163;
static zb_uint8_t g_ecc_pub_key_len_in_dwords = 6;
static zb_uint8_t g_curve_id = sect163k1;
static zb_ret_t g_last_error = RET_OK;


zb_ret_t zb_ecc_get_last_error(void)
{
    return g_last_error;
}


typedef struct main_api_vars_s
{
    zb_uint32_t *u32_pub;
    zb_ecc_point_aff_t *aff_point;
    zb_callback_t async_cb;  /*NOTE:this variable can be shared between functions of main API*/
    zb_uint8_t async_cb_param;  /*NOTE: this param isn't used in ecc library. We put it through calls and return back*/
    zb_ecc_point_pro_t *pro_point;
    zb_uint8_t *ret_p; /*NOTE: used for storing pointer for changing external data*/
    zb_uint8_t *u8p;
    zb_uint8_t *u8p1;
    zb_uint8_t *u8p2;
    zb_uint8_t *u8p3;
    zb_uint8_t *u8p4;
} main_api_vars_t;

static main_api_vars_t g_ctx;


/*Base point G*/
static zb_ecc_point_aff_t G[2] =
{
    {
        {0x5C94EEE8UL, 0xDE4E6D5EUL, 0xAA07D793UL, 0x7BBC11ACUL, 0xFE13C053UL, 0x2UL, 0x0UL, 0x0UL, 0x0UL, 0x0UL},
        {0xCCDAA3D9UL, 0x0536D538UL, 0x321F2E80UL, 0x5D38FF58UL, 0x89070FB0UL, 0x2UL, 0x0UL, 0x0UL, 0x0UL, 0x0UL}
    },
    /*0503213F 78CA4488 3F1A3B81 62F188E5 53CD265F 23C1567A
      16876913 B0C2AC24 58492836 01CCDA38 0F1C9E31 8D90F95D 07E5426F
      E87E45C0 E8184698 E4596236 4E341161 77DD2259*/
    {
        {0x58492836UL, 0xB0C2AC24UL, 0x16876913UL, 0x23C1567AUL, 0x53CD265FUL, 0x62F188E5UL, 0x3F1A3B81UL, 0x78CA4488UL, 0x0503213FUL, 0x0UL},
        {0x77DD2259UL, 0x4E341161UL, 0xE4596236UL, 0xE8184698UL, 0xE87E45C0UL, 0x07E5426FUL, 0x8D90F95DUL, 0x0F1C9E31UL, 0x01CCDA38UL, 0x0UL}
    }
};

/*Precomputation points*/
#ifdef ECC_ENABLE_FIXED_SCALARMUL
zb_ecc_point_aff_t G3[2]  = {{{0xCA10CE8CUL, 0xB3F21EBEUL, 0x9DED1294UL, 0x07BD695DUL, 0xB169733EUL, 0x7UL, 0x0UL, 0x0UL},
        {0x4A46E566UL, 0x40CF0940UL, 0x5596317AUL, 0xB3FA22D8UL, 0xEA6C3F7AUL, 0x2UL, 0x0UL, 0x0UL}
    },
    {   {0xEA608179, 0x30A02472, 0x9781805D, 0x2C239794, 0xC0BD48BA, 0x1D45FAAC, 0x603E8066, 0xCF6523BF, 0x0734B4B7},
        {0x2949DD7F, 0x1F035857, 0xA4CCE2A1, 0xB5BC3FEA, 0xA09C8C7D, 0x94F18DF3, 0x5EE72C8B, 0x2BB1A375, 0x05801D9D}
    }
};
zb_ecc_point_aff_t G5[2]  = {{{0xC68A1031UL, 0x8BC0046DUL, 0x536B5A45UL, 0x8FE46728UL, 0xC1428F6AUL, 0x4UL, 0x0UL, 0x0UL},
        {0xB8914834UL, 0xF440E165UL, 0xDD5562F1UL, 0x7DD8530CUL, 0x6187D5FFUL, 0x4UL, 0x0UL, 0x0UL}
    },
    {   {0xB278654F, 0x562A3A0E, 0x6C05664C, 0x31DDC3F4, 0x7627015B, 0xB5CB8734, 0x603106DB, 0xEB5693E8, 0x0585F122},
        {0x49F49DA5, 0x007D7862, 0xBD829CB1, 0xDAF58B54, 0x2CEE6C79, 0xCA7B9F9A, 0xEAA26F0E, 0x4A954CFD, 0x07B485DA}
    }
};
zb_ecc_point_aff_t G7[2]  = {{{0x29CE13DFUL, 0x92A33027UL, 0x4A538869UL, 0x6B74501DUL, 0x8343C0D1UL, 0x3UL, 0x0UL, 0x0UL},
        {0xCB2EC5AAUL, 0xCADD67F0UL, 0xB9A997F6UL, 0xF90D1502UL, 0x0D1B30B0UL, 0x3UL, 0x0UL, 0x0UL}
    },
    {   {0xF21AECCD, 0x65878B11, 0x684B9922, 0xDE7D9076, 0x0E2A19D9, 0x44DBDDF3, 0xE18AC0C5, 0x4CD518E6, 0x019FB980},
        {0xF3CC0666, 0x3D9427A2, 0x048F0891, 0x0E492568, 0xD582353C, 0xC9A3E5DA, 0xF15E9E2F, 0x077253A7, 0x060EC1AF}
    }
};
zb_ecc_point_aff_t G9[2]  = {{{0x57394F5CUL, 0x23D642E3UL, 0x02FF0DDFUL, 0x1993B9C9UL, 0xFC3309ABUL, 0x5UL, 0x0UL, 0x0UL},
        {0x07DD78C9UL, 0x8544EE5BUL, 0x89E8C5C9UL, 0x8B0253B6UL, 0xA6C19961UL, 0x4UL, 0x0UL, 0x0UL}
    },
    {   {0xF0D06CB4, 0xECB9EA3E, 0xD1D2D49D, 0x0FA091DC, 0xB26C70F6, 0xE92672D7, 0x6FCFAD7B, 0xB84BE904, 0x057861DF},
        {0xFF8354F6, 0x6D1629A4, 0xF84B3661, 0x222BB161, 0x5755DBE0, 0xE759EB72, 0x22498C05, 0x5EB27079, 0x05562AE1}
    }
};
zb_ecc_point_aff_t G11[2] = {{{0x5C534041UL, 0x86217FDEUL, 0xCE11A2B9UL, 0x5727F0B7UL, 0xC1722273UL, 0x1UL, 0x0UL, 0x0UL},
        {0x71A22148UL, 0x7B870045UL, 0x68F71342UL, 0x41B0C99DUL, 0x9BCB6731UL, 0x4UL, 0x0UL, 0x0UL}
    },
    {   {0xED8221B5, 0x7AB045E1, 0xFBEDE5BB, 0xECC7A10F, 0x09419933, 0x559DE582, 0x71745D56, 0xBB752F31, 0x0640B464},
        {0xACBB9230, 0xD7EF01EF, 0x4EDFEB1B, 0xDC6FAA20, 0x20CAC0A9, 0x9486BD16, 0xD99FA8CC, 0x745489DA, 0x04989F52}
    }
};
zb_ecc_point_aff_t G13[2] = {{{0x151346BBUL, 0xEC1470B5UL, 0xDAB99B8AUL, 0x180B46D0UL, 0xD51336A7UL, 0x6UL, 0x0UL, 0x0UL},
        {0xDDFB23FEUL, 0x2A08EDA0UL, 0x2A0721B1UL, 0x6A21A61AUL, 0x24037BDAUL, 0x7UL, 0x0UL, 0x0UL}
    },
    {   {0x5A897C60, 0xD13D4FF1, 0x6171C5A1, 0x00E84A19, 0xDEBAD606, 0x4CC818EE, 0xC17B510B, 0x68B5DEB4, 0x01EF0738},
        {0xF0B0F0A1, 0xF60FD96B, 0xD5B1B61B, 0x7F06FCAC, 0x5F468768, 0x6D1EA495, 0x81E0B78E, 0x89AE6254, 0x02F96C90}
    }
};
zb_ecc_point_aff_t G15[2] = {{{0x5FDE2A56UL, 0xC7EEF084UL, 0x7764A39AUL, 0xEEF24A99UL, 0xA595788AUL, 0x1UL, 0x0UL, 0x0UL},
        {0xC41F467AUL, 0x08922A85UL, 0x514F8790UL, 0xBF784D56UL, 0x95D86326UL, 0x5UL, 0x0UL, 0x0UL}
    },
    {   {0xDA83CC15, 0xF1205628, 0x50FFB298, 0x37726478, 0x45B8D399, 0xAA6D622D, 0x842343FD, 0x902D076F, 0x00F1AF02},
        {0x7E3B0DD1, 0x24D9296A, 0x5AFDBAE9, 0xF040FFAC, 0xE44FB8D0, 0x19C66656, 0x83D6EFD7, 0x2EB191C6, 0x04AF8457}
    }
};
#endif
/*Constants for partial reduction*/
/*NOTE:For sect163k1
s0 = 2579386439110731650419537, s0 = 0x22234 D1AD2426 73BDCB51
s1 = –755360064476226375461594, s1 = 0x9FF4 26B17BFC 40112ADA
s2 = 2*s1 = 0x13FE8 4D62F7F8 802255B4
s = s1 - s0 = 755360064476226375461594 - 2579386439110731650419537 =-1824026374634505274957943L , s = 0x18240 AAFBA82A 33ACA077
Vm = –4845466632539410776804317, Vm = 0x40211 45C1981B 33F14BDD*/
/*NOTE:For sect283k1. NOTE: the sign of variables s0, s1, s, s2, Vm considers in partial_mod function
s0 = -665981532109049041108795536001591469280025, s0 = 0x7A5 24D18280 550EC59E AD05080A BA9E0B19
s1 = 1155860054909136775192281072591609913945968, s1 = 0xD44 C4752086 E178BD07 87F8E327 DE5C2F70
s2 = 2*s1 = 0x1A89 88EA410D C2F17A0F 0FF1C64F BCB85EE0
s = s1 - s0 = 1155860054909136775192281072591609913945968 - ( - 665981532109049041108795536001591469280025) =  1821841587018185816301076608593201383225993,
s = 0x14E9 E946A307 368782A6 34FDEB32 98FA3A89
Vm = 7777244870872830999287791970962823977569917, Vm = 0x5947 44BE2A23 66880201 AEEB87E7 87A70E7D*/
/*NOTE:incresed length of variables s0,s1,s2,s,Vm*/
static zb_uint32_t ec_s0[2][5] = {{0x73BDCB51UL, 0xD1AD2426UL, 0x22234UL, 0x0UL, 0x0UL},
    {0xBA9E0B19UL, 0xAD05080AUL, 0x550EC59EUL, 0x24D18280UL, 0x07A5UL}
};         /*s0*/
static zb_uint32_t ec_s1[2][5] = {{0x40112ADAUL, 0x26B17BFCUL, 0x09FF4UL, 0x0UL, 0x0UL},
    {0xDE5C2F70UL, 0x87F8E327UL, 0xE178BD07UL, 0xC4752086UL, 0x0D44UL}
};         /*s1*/
static zb_uint32_t ec_s2[2][5] = {{0x802255B4UL, 0x4D62F7F8UL, 0x13FE8UL, 0x0UL, 0x0UL},
    {0xBCB85EE0UL, 0x0FF1C64FUL, 0xC2F17A0FUL, 0x88EA410DUL, 0x1A89UL}
};        /*s2=2*s1*/
static zb_uint32_t ec_s[2][5]  = {{0x33ACA077UL, 0xAAFBA82AUL, 0x18240UL, 0x0UL, 0x0UL},
    {0x98FA3A89UL, 0x34FDEB32UL, 0x368782A6UL, 0xE946A307UL, 0x14E9UL}
};         /*s=s1-s0*/
static zb_uint32_t ec_Vm[2][5] = {{0x33F14BDDUL, 0x45C1981BUL, 0x40211UL, 0x0UL, 0x0UL},
    {0x87A70E7DUL, 0xAEEB87E7UL, 0x66880201UL, 0x44BE2A23UL, 0x5947UL}
};        /*Vm=2^m+1-#E_a(F_2^m)*/

/*For the curve sect163k1 and sect283k1*/
static zb_uint32_t eccn[2][10] = {{0x99F8A5EFUL, 0xA2E0CC0DUL, 0x20108UL, 0x0UL, 0x0UL, 0x4UL, 0x0UL, 0x0UL, 0x0UL, 0x0UL},
    /*80 00000000 00000000 00000000 00069D5B B915BCD4 6EFB1AD5 F173ABDF*/
    /*{0xF173ABDFUL,0x6EFB1AD5UL,0xB915BCD4UL,0x69D5BUL,0x0UL,0x0UL,0x0UL,0x80UL}};*/
    /*01FFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFE9AE 2ED07577 265DFF7F 94451E06 1E163C61*/
    {0x1E163C61UL, 0x94451E06UL, 0x265DFF7FUL, 0x2ED07577UL, 0xFFFFE9AEUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0x01FFFFFFUL, 0x0UL}
};

/*NOTE:sect163k1:
eccu = 0x0x3fffffffffffffffffffdfef75d1f33f266075a120108e744e1df22e
eccu = (2^32)^(2 * (log_b(eccn) + 1)) / eccn = 2^384 / eccn
*/
static zb_uint32_t eccu[2][11] =
{
    {0x4E1DF22EUL, 0x20108E74UL, 0x266075A1UL, 0x75D1F33FUL, 0xFFFFDFEFUL, 0xFFFFFFFFUL, 0x3FFFFFFFUL, 0x0UL, 0x0UL, 0x0UL, 0x0UL},
    /*0x1ffffffffffffffffffffffffffffe58a911ba90cae441394a83a3150855e08327734f6*/
    /*NOTE: for sect233k1 eccu = 2^512 / eccn
    {0x327734F6UL,0x50855E08UL,0x94A83A31UL,0x0CAE4413UL,0x8A911BA9UL,0xFFFFFFE5UL,0xFFFFFFFFUL,0xFFFFFFFFUL,0x1FFFFFFUL}};
    */
    /*NOTE: for sect283k1 eccu = 2^576 / eccn
    eccu = 0x80000000000000000000000000000000000594744be2a2366880201aeeb87e787a70e7fe45
    */
    {0x70E7FE45UL, 0xB87E787AUL, 0x80201AEEUL, 0xE2A23668UL, 0x0594744BUL, 0x00000000UL, 0x00000000UL, 0x00000000UL, 0x00000000UL, 0x80UL, 0x0UL}
};


static zb_uint32_t shared_buf[7U * ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS];
#ifdef ECC_FAST_RANDOM_SCALARMUL
/*share memory with random scalar mul*/
static zb_uint32_t shared_buf1[20U * ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS];
#else
zb_uint32_t shared_buf1[12U * ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS];
#endif
static zb_uint32_t shared_buf2[5U * ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS];
static zb_uint32_t shared_buf3[2U * ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS];

#ifdef ECC_FAST_MODMUL
static zb_uint32_t modmul_table[15][ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS];  /*store the precomputation results*/
#endif

static zb_int8_t expansion_table[286];  /*NOTE: previous is 286 = 283 + 3*/

static zb_uint64_t T64_1;
static zb_uint64_t T64_2;
static zb_uint64_t T64_3;


/**
 * Function for elliptic curve crypto library initialization. You must call this function before
 * work with elliptic curve crypto library.
 * @param ecc_g_curve_id - id of elliptic curve crypto library(sect163k1 or sect283k1)
 */
void zb_ecc_set_curve(const zb_uint8_t ecc_curve_id)
{
    ZB_ASSERT((sect163k1 == ecc_curve_id) || (sect283k1 == ecc_curve_id));

    g_ecc_prv_key_len = ZB_ECC_PRIVATE_KEY_LEN[ecc_curve_id];
    g_ecc_pub_key_len = ZB_ECC_PUBLIC_KEY_LEN[ecc_curve_id];
    g_ecc_bitlen = (zb_uint16_t)ZB_ECC_BITLEN[ecc_curve_id] * 8U + 3U;
    g_ecc_pub_key_len_in_dwords = ZB_ECC_PUB_KEY_LEN_IN_DWORDS[ecc_curve_id];
    g_curve_id = ecc_curve_id;
}


/**
 * Simple implementation of bytes inversion.
 * @param ptr - pointer to block of bytes
 * @param len - length of bytes block
 */
void zb_ecc_inverse_bytes(zb_uint8_t *ptr, zb_uint32_t len)
{
    zb_uint8_t tmp;
    zb_uint32_t i;

    for (i = 0; i < len / 2U; i++)
    {
        tmp = ptr[i];
        ptr[i] = ptr[len - i - 1U];
        ptr[len - i - 1U] = tmp;
    }

}


/**
 * Function for copy len bytes of src to dst in reverse order
 * @param dst - pointer to destination
 * @param src - pointer to source
 * @param len - amount of bytes for copy
 */
static void copy_and_inverse(zb_uint8_t *dst, const zb_uint8_t *src, const zb_uint32_t len)
{
    zb_uint32_t i;
    /*NOTE:You must control that length of dst shoud be <= len and clear dst if it is necessary
    outside of this function*/

    for (i = 0; i <= len / 2U; i++)
    {
        dst[i] = src[len - i - 1U];
        dst[len - i - 1U] = src[i];
    }
}

/**
 * Zigbee Keyed Hash Function. Described in Zigbee specification
 * section B.1.4, and in FIPS Publication 198. Use MMO hash function
 * and authentication key to compute a MAC over the input data of specified
 * length.
 * This function implements the hash function:
 *     Hash(Key, text) = H((Key XOR opad) || H((Key XOR ipad) || text));
 *     ipad = 0x36 repeated.
 *     opad = 0x5c repeated.
 *     H() = Zigbee Cryptographic Hash (B.1.3 and B.6).
 *
 * The output of this function is an ep_alloced bufer containing
 * the key-hashed output, and is garaunteed never to return NULL.
 * @ZB_ECC_MAC_KEY_LEN is the same as the output size HMAClen of the HMAC
 * function shall have the same integer value as the
 * message digest parameter hashlen as specified in sub- clause C.4.2.2.6.
 *
 * @param mac_key    Zigbee Security Key (must be ZBEE_SEC_CONST_KEYSIZE) in length.
 * @param input      input data to compute MAC.
 * @param input_len  length of input data
 * @param hash       MAC computed over security key @key and given
 *                   input data @input of @len length.
 */
void zb_ecc_key_hmac(const zb_uint8_t *mac_key, const zb_uint8_t *input, const zb_uint16_t input_len, zb_bufid_t hash)
{
    /*zb_uint8_t  hash_in[2*ZB_ECC_MAC_KEY_LEN];*/
    zb_uint8_t *hash_in = (zb_uint8_t *)expansion_table;
    /*FIXME: need to check length of parameter @input*/

    zb_uint8_t *hash_out;
    zb_uint16_t i;

    hash_out = zb_buf_initial_alloc(hash, ZB_ECC_MAC_KEY_LEN + input_len);

    ZB_MEMSET(hash_in, 0, 2U * ZB_ECC_MAC_KEY_LEN);
    for (i = 0; i < ZB_ECC_MAC_KEY_LEN; i++)
    {
        /* Copy the key into hash_in and XOR with opad(0x5c) to form: (Key XOR opad) */
        hash_in[i] = mac_key[i] ^ 0x5CU;
        /* Copy the Key into hash_out and XOR with ipad(0x36) to form: (Key XOR ipad) */
        *(hash_out + i) = mac_key[i] ^ 0x36U;
    }
    /* Append the input byte to form: (Key XOR ipad) || text. */
    for (i = 0; i < input_len; i++)
    {
        *(hash_out + ZB_ECC_MAC_KEY_LEN + i) = input[i];
    }
    /* Hash the contents of hash_out and append the contents to hash_in to
     * form: (Key XOR opad) || H((Key XOR ipad) || text).
     */
    (void)zb_sec_b6_hash(hash_out, ZB_ECC_MAC_KEY_LEN + input_len, hash_in + ZB_ECC_MAC_KEY_LEN);
    /* Hash the contents of hash_in to get the final result. */
    (void)zb_sec_b6_hash(hash_in, 2UL * ZB_ECC_MAC_KEY_LEN, hash_out);
}


/**
 * Support fast arithmetic for two binary fields
 * GF(2^163): F_2[x]/(x^163 + x^7 + x^6 + x^3 + 1)
 * GF(2^233): F_2[x]/(x^233 + x^74 + 1)
 */


/**
 * Modular addition over GF(2^n) field
 * @param a        - the first summand
 * @param b        - the second summand
 * @param c        - result of a + b
 */
static void zb_ecc_modadd(const zb_uint32_t *a, const zb_uint32_t *b, zb_uint32_t *c)
{
    zb_uint8_t i;

    for (i = 0; i < g_ecc_pub_key_len_in_dwords; i++)
    {
        *(c + i) = *(a + i) ^ *(b + i);
    }
}

#ifndef ECC_FAST_MODMUL
/**
 * function for compute result of multiplication a on u
 * @param a  - polynom over GF(2^m) field
 * @param r  - result of a * u
 * @param u  - polynom over GF(2^m) field with order less than 4
 */
void zb_ecc_a_mul_u(const zb_uint32_t *a, zb_uint32_t *r, zb_uint8_t u)
{
    zb_uint8_t j;
    /*precomputation*/
    if (u == 1)
    {
        for (j = 0; j < g_ecc_pub_key_len_in_dwords; j++)
        {
            *(r + j) ^= *(a + j);  /*9 here is maximum length of private key of sect283k1 curve*/
        }
    }
    else
    {
        if (u & 1)
        {
            for (j = 0; j < g_ecc_pub_key_len_in_dwords; j++)
            {
                *(r + j) ^= *(a + j);
            }
            /*      *(r + 0) ^= *(a + 0);
                  *(r + 1) ^= *(a + 1);
                  *(r + 2) ^= *(a + 2);
                  *(r + 3) ^= *(a + 3);
                  *(r + 4) ^= *(a + 4);
                  *(r + 5) ^= *(a + 5);
                  *(r + 6) ^= *(a + 6);
                  *(r + 7) ^= *(a + 7);
                  *(r + 8) ^= *(a + 8);*/
        }
        if (u & 2)
        {
            *(r + 0) ^= *(a + 0) << 1;
            for (j = 1; j < g_ecc_pub_key_len_in_dwords; j++)
            {
                *(r + j) ^= ((*(a + j) << 1) | (*(a + j - 1) >> 31));
            }
            /**(r + 1) ^= ((*(a + 1) << 1) | (*(a + 1 - 1) >> 31));
            *(r + 2) ^= ((*(a + 2) << 1) | (*(a + 2 - 1) >> 31));
            *(r + 3) ^= ((*(a + 3) << 1) | (*(a + 3 - 1) >> 31));
            *(r + 4) ^= ((*(a + 4) << 1) | (*(a + 4 - 1) >> 31));
            *(r + 5) ^= ((*(a + 5) << 1) | (*(a + 5 - 1) >> 31));
            *(r + 6) ^= ((*(a + 6) << 1) | (*(a + 6 - 1) >> 31));
            *(r + 7) ^= ((*(a + 7) << 1) | (*(a + 7 - 1) >> 31));
            *(r + 8) ^= ((*(a + 8) << 1) | (*(a + 8 - 1) >> 31));*/
        }
        if (u & 4)
        {
            *(r + 0) ^= *(a + 0) << 2;
            for (j = 1; j < g_ecc_pub_key_len_in_dwords; j++)
            {
                *(r + j) ^= ((*(a + j) << 2) | (*(a + j - 1) >> 30));
            }
        }
        if (u & 8)
        {
            *(r + 0) ^= *(a + 0) << 3;
            for (j = 1; j < g_ecc_pub_key_len_in_dwords; j++)
            {
                *(r + j) ^= ((*(a + j) << 3) | (*(a + j - 1) >> 29));
            }
        }
    }
}
#endif

/* DD:arranged all global buffers to the top of the file, in the single place */

/**
 * Modular multiplication over GF(2^n)
 * @param a        - the first operand
 * @param b        - the second operand
 * @param c        - result of a * b over GF(2^n)
 */
static void zb_ecc_modmul(const zb_uint32_t *a, const zb_uint32_t *b, zb_uint32_t *c)
{
#ifdef ECC_FAST_MODMUL
    zb_uint32_t *p;
    zb_uint8_t k;
#endif
    /*zb_uint32_t R[2*ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS-1] = {0x0};*/  /*store the intermediate result*/
    zb_uint32_t *R = shared_buf3;
    zb_uint32_t u;
    zb_uint8_t i;
    zb_uint8_t j;

    /*NOTE: This algorithms looks as 'Algorithm 2.36 Left-to-right comb method with windows of width w'
     * of book 'Guide to Elliptic Curve Cryptography'
     */

    ZB_MEMSET(R, 0, 2U * ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);

#ifdef ECC_FAST_MODMUL
    /*precomputation*/
    ZB_MEMSET(modmul_table, 0, 15U * ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);
    p = modmul_table[0];

    ZB_MEMCPY(p, a, (zb_uint32_t)g_ecc_pub_key_len_in_dwords * 4U);


    /*NOTE: 9 here is maximum width of private key for sect283k1 curve*/
    for (j = g_ecc_pub_key_len_in_dwords - 1U; j != 0U; j--)
    {
        *(p + 9 + j)  = (*(p + j) << 1) | (*(p + j - 1) >> 31);   /*9 * 1*/
        *(p + 18 + j) = *(p + 9 + j) ^ *(p + j);   /*9 * 2*/
        *(p + 27 + j) = (*(p + j) << 2) | (*(p + j - 1) >> 30);   /*9 * 3*/
        *(p + 36 + j) = *(p + 27 + j) ^ *(p + j);   /*9 * 4*/
        *(p + 45 + j) = *(p + 27 + j) ^ *(p + 9 + j);   /*9 * 5*/
        *(p + 54 + j) = *(p + 45 + j) ^ *(p + j);   /*9 * 6*/
        *(p + 63 + j) = (*(p + j) << 3) | (*(p + j - 1) >> 29);   /*9 * 7*/
        *(p + 72 + j) = *(p + 63 + j) ^ *(p + j);   /*9 * 8*/
        *(p + 81 + j) = *(p + 63 + j) ^ *(p + 9 + j);   /*9 * 9*/
        *(p + 90 + j) = *(p + 81 + j) ^ *(p + j);   /*9 * 10*/
        *(p + 99 + j) = *(p + 63 + j) ^ *(p + 27 + j);   /*9 * 11*/
        *(p + 108 + j) = *(p + 99 + j) ^ *(p + j);   /*9 * 12*/
        *(p + 117 + j) = *(p + 99 + j) ^ *(p + 9 + j);   /*9 * 13*/
        *(p + 126 + j) = *(p + 117 + j) ^ *(p + j);   /*9 * 14*/
    }
    *(p + 9)  = *p << 1;
    *(p + 18) = *(p + 9) ^ *p;
    *(p + 27) = *p << 2;
    *(p + 36) = *(p + 27) ^ *p;
    *(p + 45) = *(p + 27) ^ *(p + 9);
    *(p + 54) = *(p + 45) ^ *p;
    *(p + 63) = *p << 3;
    *(p + 72) = *(p + 63) ^ *p;
    *(p + 81) = *(p + 63) ^ *(p + 9);
    *(p + 90) = *(p + 81) ^ *p;
    *(p + 99) = *(p + 63) ^ *(p + 27);
    *(p + 108) = *(p + 99) ^ *p;
    *(p + 117) = *(p + 99) ^ *(p + 9);
    *(p + 126) = *(p + 117) ^ *p;
#endif
    /* main loop*/
    for (i = 7; i != 0U; i--)
    {
        for (j = 0; j < g_ecc_pub_key_len_in_dwords; j++)
        {
            u = (*(b + j) >> (i * 4U)) & 0x0FU;
            if (u != 0U)
            {
#ifdef ECC_FAST_MODMUL
                u--;
                for (k = 0; k < g_ecc_pub_key_len_in_dwords; k++)
                {
                    *(R + j + k) ^= *(p + 9U * u + k); /* 9 here is maximum length of private key of sect283k1 curve*/
                }
#else
                zb_ecc_a_mul_u(a, (R + j), (zb_uint8_t)u);
#endif
            }
        }

        /*R <- x^4*R*/
        switch (g_curve_id)
            /*cstat !MISRAC2012-Rule-16.1*/
            /* Rule-16.1 The keyword return has an equivalent behaviour as the break */
        {
        case sect163k1:
        {
            if (i == 1U)
            {
                *(R + 10) = *(R + 9) >> 28;
            }
            for (j = 9U; j != 0U; j--)
            {
                R[j] = R[j] << 4 | R[j - 1U] >> 28;
            }

            break;
        }
        case sect283k1:
        {
            /*if ((i == 2) || (i == 1))
            {
              *(R + 17) = (*(R + 17) << 4) | (*(R + 16) >> 28);
            }*/
            for (j = 17U; j != 0U; j--)
            {
                R[j] = R[j] << 4 | R[j - 1U] >> 28;
            }

            break;
        }

        /*cstat !MISRAC2012-Rule-16.3*/
        /* Rule-16.3 The keyword return has an equivalent behaviour as the break */
        default:
            return;
        }
        *R = *R << 4;
    }

    /*last loop*/
    for (j = 0; j < g_ecc_pub_key_len_in_dwords; j++)
    {
        u = *(b + j) & 0x0FU;
        if (u != 0U)
        {
#ifdef ECC_FAST_MODMUL
            u--;
            for (k = 0; k < g_ecc_pub_key_len_in_dwords; k++)
            {
                *(R + j + k) ^= *(p + 9U * u + k);  /* 9 here is maximum length of private key of sect283k1 curve*/
            }
#else
            zb_ecc_a_mul_u(a, (R + j), (zb_uint8_t)u);
#endif
        }
    }

    /*fast modular reduction, one word at a time*/
    /*AEV:switch is not so efficient as if, changed to if it is possible*/
    if (sect163k1 == g_curve_id)
    {
        *(R + 4) ^= (*(R + 10) << 29);
        *(R + 5) ^= *(R + 10) ^ (*(R + 10) << 3) ^ (*(R + 10) << 4) ^ (*(R + 10) >> 3);

        for (j = 5; j != 1U; j--)
        {
            R[j] ^= R[j + 4U] >> 29 ^ R[j + 4U] >> 28;
            R[j - 1U] ^= R[j + 4U] ^ R[j + 4U] << 3 ^ R[j + 4U] << 4 ^ R[j + 4U] >> 3;
            R[j - 2U] ^= R[j + 4U] << 29;
        }

        u = *(R + 5) >> 3;
        *R ^= u ^ (u << 3) ^ (u << 6) ^ (u << 7);
        *(R + 1) ^= (u >> 25) ^ (u >> 26);

        c[g_ecc_pub_key_len_in_dwords - 1U] = R[5] & 0x07U;

    }
    else
    {
        ZB_ASSERT(sect283k1 == g_curve_id);
        for (j = 17; j >= 9U; j--)
        {
            R[j - 9U] ^= R[j] << 5 ^ R[j] << 10 ^ R[j] << 12 ^ R[j] << 17;
            R[j - 8U] ^= R[j] >> 27 ^ R[j] >> 22 ^ R[j] >> 20 ^ R[j] >> 15;
        }

        u = *(R + 8) >> 27;
        *R ^= u ^ (u << 5) ^ (u << 7) ^ (u << 12);

        c[g_ecc_pub_key_len_in_dwords - 1U] = R[8] & 0x07FFFFFFU;
    }

    /*store the reduction result*/
    /*DD: Use ZB_MEMCPY instead of explicit assignment in the loop*/
    ZB_MEMCPY(c, R, 4U * ((zb_uint32_t)g_ecc_pub_key_len_in_dwords - 1U));
    /*for (j = 0; j < g_ecc_pub_key_len_in_dwords - 1; j++)
    {
      *(c + j) = *(R + j);
    }*/
}

/*static void dump_bytes(zb_uint8_t *ptr, size_t size, zb_char_t *msg)
{
  printf("\n%s\n", msg);
  for (zb_uint8_t i=0; i < size; i++)
  {
    printf("%02X", ptr[size - i - 1]);
    if (3 == (size - i - 2) % 4)
      printf("  ");
    else
      printf(".");
  }
  printf("\n");
}*/


/*precomputation*/
#ifdef ECC_FAST_MODSQ
static zb_uint16_t moser_de_brujn[256] =
{
    0, 1, 4, 5, 16, 17, 20, 21, 64, 65, 68, 69, 80, 81, 84, 85, 256, 257, 260, 261, 272, 273, 276, 277, 320,
    321, 324, 325, 336, 337, 340, 341, 1024, 1025, 1028, 1029, 1040, 1041, 1044, 1045, 1088, 1089,
    1092, 1093, 1104, 1105, 1108, 1109, 1280, 1281, 1284, 1285, 1296, 1297, 1300, 1301, 1344, 1345,
    1348, 1349, 1360, 1361, 1364, 1365, 4096, 4097, 4100, 4101, 4112, 4113, 4116, 4117, 4160, 4161,
    4164, 4165, 4176, 4177, 4180, 4181, 4352, 4353, 4356, 4357, 4368, 4369, 4372, 4373, 4416, 4417,
    4420, 4421, 4432, 4433, 4436, 4437, 5120, 5121, 5124, 5125, 5136, 5137, 5140, 5141, 5184, 5185,
    5188, 5189, 5200, 5201, 5204, 5205, 5376, 5377, 5380, 5381, 5392, 5393, 5396, 5397, 5440, 5441,
    5444, 5445, 5456, 5457, 5460, 5461, 16384, 16385, 16388, 16389, 16400, 16401, 16404, 16405,
    16448, 16449, 16452, 16453, 16464, 16465, 16468, 16469, 16640, 16641, 16644, 16645, 16656,
    16657, 16660, 16661, 16704, 16705, 16708, 16709, 16720, 16721, 16724, 16725, 17408, 17409,
    17412, 17413, 17424, 17425, 17428, 17429, 17472, 17473, 17476, 17477, 17488, 17489, 17492,
    17493, 17664, 17665, 17668, 17669, 17680, 17681, 17684, 17685, 17728, 17729, 17732, 17733,
    17744, 17745, 17748, 17749, 20480, 20481, 20484, 20485, 20496, 20497, 20500, 20501, 20544,
    20545, 20548, 20549, 20560, 20561, 20564, 20565, 20736, 20737, 20740, 20741, 20752, 20753,
    20756, 20757, 20800, 20801, 20804, 20805, 20816, 20817, 20820, 20821, 21504, 21505, 21508,
    21509, 21520, 21521, 21524, 21525, 21568, 21569, 21572, 21573, 21584, 21585, 21588, 21589,
    21760, 21761, 21764, 21765, 21776, 21777, 21780, 21781, 21824, 21825, 21828, 21829, 21840,
    21841, 21844, 21845
};
#else
/*FIXME: it can be also optimized, becasue neighbors are differe 4*/
static zb_uint16_t moser_de_brujn[128] =
{
    0, 4, 16, 20, 64, 68, 80, 84, 256, 260, 272, 276, 320,
    324, 336, 340, 1024, 1028, 1040, 1044, 1088,
    1092, 1104, 1108, 1280, 1284, 1296, 1300, 1344,
    1348, 1360, 1364, 4096, 4100, 4112, 4116, 4160,
    4164, 4176, 4180, 4352, 4356, 4368, 4372, 4416,
    4420, 4432, 4436, 5120, 5124, 5136, 5140, 5184,
    5188, 5200, 5204, 5376, 5380, 5392, 5396, 5440,
    5444, 5456, 5460, 16384, 16388, 16400, 16404,
    16448, 16452, 16464, 16468, 16640, 16644, 16656,
    16660, 16704, 16708, 16720, 16724, 17408,
    17412, 17424, 17428, 17472, 17476, 17488, 17492,
    17664, 17668, 17680, 17684, 17728, 17732,
    17744, 17748, 20480, 20484, 20496, 20500, 20544,
    20548, 20560, 20564, 20736, 20740, 20752,
    20756, 20800, 20804, 20816, 20820, 21504, 21508,
    21520, 21524, 21568, 21572, 21584, 21588,
    21760, 21764, 21776, 21780, 21824, 21828, 21840, 21844
};
#endif


#define GET_T(x) ((((x) & 0x1)) ? (moser_de_brujn[(x) >> 1] + 1) : (moser_de_brujn[(x) >> 1]))


/**
 * Square function over GF(2^n) field
 * @param a        - value for squaring
 * @param b        - result of a^2 over GF(2^n) field
 */
void zb_ecc_modsq(const zb_uint32_t *a, zb_uint32_t *b)
{
    /*  zb_uint32_t R[2*ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS-1];*/
    zb_uint32_t *R = shared_buf3;  /*NOTE: length : 2 * ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES*/
    zb_uint8_t i, j;

    /*AEV: not efficient, changed switch to if*/
    if ((sect163k1 == g_curve_id ? IF1_163(a) : IF1_283(a)))
    {
        XIS1(b, (zb_uint32_t)g_ecc_pub_key_len_in_dwords);
    }

    ZB_MEMSET(R, 0, 76);
    /*compute R[2*g_ecc_pub_key_len_in_dwords-2]*/

    for (i = 0; i < g_ecc_pub_key_len_in_dwords - 1U; i++)
    {
#ifdef ECC_FAST_MODSQ
        *(R + 2U * i) = (zb_uint32_t) * (moser_de_brujn + (*(a + i) & 0xFFU))
                        | ((zb_uint32_t) * (moser_de_brujn + ((*(a + i) >> 8) & 0xFFU)) << 16);
        *(R + 2U * i + 1U) = (zb_uint32_t) * (moser_de_brujn + ((*(a + i) >> 16) & 0xFFU))
                             | ((zb_uint32_t) * (moser_de_brujn + ((*(a + i) >> 24) & 0xFFU)) << 16);
#else
        *(R + 2 * i) = (zb_uint32_t)(GET_T(*(a + i) & 0xff)) | ((zb_uint32_t)(GET_T((*(a + i) >> 8) & 0xff)) << 16);
        *(R + 2 * i + 1) = (zb_uint32_t)(GET_T((*(a + i) >> 16) & 0xff)) | ((zb_uint32_t)(GET_T((*(a + i) >> 24) & 0xff)) << 16);
#endif
    }

    /*AEV: not efficient, change switch to if*/
    /*fast modular reduction, one word at a time*/
    if (sect163k1 == g_curve_id)
    {
#ifdef ECC_FAST_MODSQ
        *(R + 10) = (zb_uint32_t) * (moser_de_brujn + (*(a + 5) & 0xFFU));
        /*dump_bytes((zb_uint8_t *)R, 45, "*(R + 10) = (zb_uint32_t)*(T + (*(a + 5) & 0xff))-->");*/
#else
        *(R + 10) = (zb_uint32_t)(GET_T(*(a + 5) & 0xff));
#endif
        *(R + 4) ^= (*(R + 10) << 29);
        /*dump_bytes((zb_uint8_t *)R, 45, "*(R + 4) ^= (*(R + 10) << 29)-->");*/
        *(R + 5) ^= *(R + 10) ^ (*(R + 10) << 3) ^ (*(R + 10) << 4) ^ (*(R + 10) >> 3);
        /*dump_bytes((zb_uint8_t *)R, 45, "*(R + 5) ^= *(R + 10) ^ (*(R + 10) << 3) ^ (*(R + 10) << 4) ^ (*(R + 10) >> 3)-->");*/

        for (j = 5; j != 1U; j--)
        {
            R[j] ^= R[j + 4U] >> 29 ^ R[j + 4U] >> 28;
            R[j - 1U] ^= R[j + 4U] ^ R[j + 4U] << 3 ^ R[j + 4U] << 4 ^ R[j + 4U] >> 3;
            R[j - 2U] ^= R[j + 4U] << 29;
        }

        *(R + 6) = *(R + 5) >> 3;
        *R ^= *(R + 6) ^ (*(R + 6) << 3) ^ (*(R + 6) << 6) ^ (*(R + 6) << 7);
        *(R + 1) ^= (*(R + 6) >> 25) ^ (*(R + 6) >> 26);

        *(b + 5) = *(R + 5) & 0x07U;
    }
    else /*if (sect283k1 == g_curve_id)*/
    {
#ifdef ECC_FAST_MODSQ
        /**(R + 14) = (zb_uint32_t)*(T + (*(a + 7) & 0xff)) | ((zb_uint32_t)*(T + ((*(a + 7) >> 8) & 0xff)) << 16);*/
        *(R + 17) = (zb_uint32_t) * (moser_de_brujn + ((*(a + 8) >> 16) & 0xFFU))
                    | ((zb_uint32_t) * (moser_de_brujn + ((*(a + 8) >> 24) & 0x07U)) << 16);
        *(R + 16) = (zb_uint32_t) * (moser_de_brujn + (*(a + 8) & 0xFFU))
                    | ((zb_uint32_t) * (moser_de_brujn + ((*(a + 8) >> 8) & 0xFFU)) << 16);
#else
        *(R + 17) = (zb_uint32_t)(GET_T((*(a + 8) >> 16) & 0xff)) | ((zb_uint32_t)(GET_T((*(a + 8) >> 24) & 0x7)) << 16);
        *(R + 16) = (zb_uint32_t)(GET_T(*(a + 8) & 0xff)) | ((zb_uint32_t)(GET_T((*(a + 8) >> 8) & 0xff)) << 16);
        /*dump_bytes((zb_uint8_t *)R, 76, "*(R + 16) = (zb_uint32_t)*(T + (*(a + 8) & 0xff)) | ((zb_uint32_t)*(T + ((*(a + 8) >> 8) & 0xff)) << 16)-->");*/
#endif
        for (j = 17; j >= 9U; j--)
        {
            R[j - 9U] ^= R[j] << 5 ^ R[j] << 10 ^ R[j] << 12 ^ R[j] << 17;
            R[j - 8U] ^= R[j] >> 27 ^ R[j] >> 22 ^ R[j] >> 20 ^ R[j] >> 15;
        }

        *(R + 9) = *(R + 8) >> 27;
        *R ^= *(R + 9) ^ (*(R + 9) << 5) ^ (*(R + 9) << 7) ^ (*(R + 9) << 12);
        *(b + 8) = *(R + 8) & 0x07FFFFFFU;
    }

    /*store the reduction result*/
    /*Use ZB_MEMCPY instead of explicit assignment in the loop*/
    ZB_MEMCPY(b, R, 4U * ((zb_uint32_t)g_ecc_pub_key_len_in_dwords - 1U));
    /*for (j = g_ecc_pub_key_len_in_dwords - 2; j != 0; j--)
    {
      *(b + j) = *(R + j);
    }
    *b = *R;*/
}
#undef GET_T


/**
 * Big integer comparison
 * @param  a - the first big integer
 * @param  b - the second big integer
 * @param  m - length of big integers a and b
 * @return   ZB_BI_EQUAL if a and b is equal, ZB_BI_LESS if a < b and ZB_BI_GREATER if a > b
 */
static zb_int8_t zb_ecc_compare(const zb_uint32_t *a, const zb_uint32_t *b, const zb_uint8_t len)
{
    zb_int8_t i;
    a += len - 1U;
    b += len - 1U;
    for (i = (zb_int8_t)len - 1; i >= 0; i--)
    {
        if (*(a) < * (b))
        {
            return ZB_BI_LESS;
        }
        if (*(a--) > *(b--))
        {
            return ZB_BI_GREATER;
        }
    }

    return ZB_BI_EQUAL;
}

/**
 * Modular inversion over GF(2^n) field
 * @param a        - parameter for modular inversion
 * @param b        - result of modular inversion
 */
static void zb_ecc_modinv(const zb_uint32_t *a, zb_uint32_t *b)
{
    /* zb_uint32_t g1[ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS];*/
    /*NOTE: we use shared_buf2 because zb_ecc_modinv is called only from zb_ecc_project_to_affine function
      which is also used shared_buf2 (ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES bytes)*/
    zb_uint32_t *g1 = shared_buf2 + ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS; /*NOTE: length : ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES*/
    /* zb_uint32_t g2[ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS];*/ /*FIXME: previous length 9 of g2 arrary generated infinity loop*/
    zb_uint32_t *g2 = shared_buf2 + 2U * ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS; /*NOTE: length : ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES. Length should be greater than 40 bytes!*/
    /* zb_uint32_t u[ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS];*/
    zb_uint32_t *u = shared_buf2 + 3U * ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS; /*NOTE: length : ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES*/
    /* zb_uint32_t v[ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS];*/  /*FIXME: previous length 9 of v arrary generated infinity loop in while ((v[0] & 0x1) == 0)*/
    zb_uint32_t *v = shared_buf2 + 4U * ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS; /*NOTE: length : ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES. Length should be greater than 40 bytes!*/
    zb_uint32_t uv;
    zb_uint8_t i;

    ZB_MEMSET(u, 0, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);
    ZB_MEMSET(v, 0, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);
    ZB_MEMSET(g1, 0, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);
    ZB_MEMSET(g2, 0, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);

    g1[0] = 0x1;
    g2[0] = 0x0;

    if (sect163k1 == g_curve_id)
    {
        ZB_MEMCPY(u, a, 4UL * g_ecc_pub_key_len_in_dwords);
        /*x^163 + x^7 + x^6 + x^3 + 1*/
        v[0] = 0xc9;
        v[5] = 0x8;

        uv = (zb_uint32_t)ZB_B2U((IFN1_163(u)) && (IFN1_163(v)));
    }
    else
    {
        ZB_ASSERT(sect283k1 == g_curve_id);
        ZB_MEMCPY(u, a, 4UL * g_ecc_pub_key_len_in_dwords);
        /*x^233 + x^74 + 1*/
        /*x^283 + x^12 + x^7 + x^5 + 1*/
        v[0] = 0x10A1;
        v[8] = 0x8000000; // 1 << 27
        uv = (zb_uint32_t)ZB_B2U((IFN1_283(u)) && (IFN1_283(v)));
    }

    while (uv != 0U)
    {
        while ((u[0] & 0x01U) == 0U)
        {
            for (i = 0; i < g_ecc_pub_key_len_in_dwords - 1U; i++)
            {
                *(u + i) = (*(u + i) >> 1) | (*(u + i + 1) << 31);
            }
            *(u + g_ecc_pub_key_len_in_dwords - 1) = *(u + g_ecc_pub_key_len_in_dwords - 1) >> 1;

            if ((g1[0] & 0x01U) != 0U)
            {
                if (sect163k1 == g_curve_id)
                {
                    /*x^163 + x^7 + x^6 + x^3 + 1*/
                    g1[0] ^= 0xC9U;
                    g1[5] ^= 0x08U;
                }
                else
                {
                    ZB_ASSERT(sect283k1 == g_curve_id);
                    /*x^283 + x^12 + x^7 + x^5 + 1*/
                    g1[0] ^= 0x10A1U;
                    g1[8] ^= 0x8000000U;
                }
            }

            for (i = 0; i < g_ecc_pub_key_len_in_dwords - 1U; i++)
            {
                *(g1 + i) = (*(g1 + i) >> 1) | (*(g1 + i + 1) << 31);
            }
            *(g1 + g_ecc_pub_key_len_in_dwords - 1) = *(g1 + g_ecc_pub_key_len_in_dwords - 1) >> 1;
        }

        while ((v[0] & 0x01U) == 0U)
        {
            for (i = 0; i < g_ecc_pub_key_len_in_dwords - 1U; i++)
            {
                *(v + i) = (*(v + i) >> 1) | (*(v + i + 1) << 31);
            }
            *(v + g_ecc_pub_key_len_in_dwords - 1) = *(v + g_ecc_pub_key_len_in_dwords - 1) >> 1;

            if ((g2[0] & 0x01U) != 0U)
            {
                if (sect163k1 == g_curve_id)
                {
                    /*x^163 + x^7 + x^6 + x^3 + 1*/
                    g2[0] ^= 0xC9U;
                    g2[5] ^= 0x08U;
                }
                else
                {
                    ZB_ASSERT(sect283k1 == g_curve_id);
                    /*x^283 + x^12 + x^7 + x^5 + 1*/
                    g2[0] ^= 0x10A1U;
                    g2[8] ^= 0x8000000U;
                }
            }

            for (i = 0; i < g_ecc_pub_key_len_in_dwords - 1U; i++)
            {
                *(g2 + i) = (*(g2 + i) >> 1) | (*(g2 + i + 1) << 31);
            }
            *(g2 + g_ecc_pub_key_len_in_dwords - 1) = *(g2 + g_ecc_pub_key_len_in_dwords - 1) >> 1;
        }

        if (ZB_BI_GREATER == zb_ecc_compare(u, v, g_ecc_pub_key_len_in_dwords))
        {
            for (i = 0; i < g_ecc_pub_key_len_in_dwords; i++)
            {
                *(u + i)  ^= *(v + i);
                *(g1 + i) ^= *(g2 + i);
            }
        }
        else
        {
            for (i = 0; i < g_ecc_pub_key_len_in_dwords; i++)
            {
                *(v + i)  ^= *(u + i);
                *(g2 + i) ^= *(g1 + i);
            }
        }

        if (sect163k1 == g_curve_id)
        {
            uv = (zb_uint32_t)ZB_B2U((IFN1_163(u)) && (IFN1_163(v)));
        }
        else
        {
            ZB_ASSERT(sect283k1 == g_curve_id);
            uv = (zb_uint32_t)ZB_B2U((IFN1_283(u)) && (IFN1_283(v)));
        }
    }

    if ((sect163k1 == g_curve_id) ? IF1_163(u) : IF1_283(u))
    {
        for (i = 0; i < g_ecc_pub_key_len_in_dwords; i++)
        {
            *(b + i) = *(g1 + i);
        }
    }
    else
    {
        for (i = 0; i < g_ecc_pub_key_len_in_dwords; i++)
        {
            *(b + i) = *(g2 + i);
        }
    }
}


/**
 * Big integer addition
 * @param  a        - the first summand
 * @param  b        - the second summand
 * @param  aplusb   - result of a + b
 * @return 1 if carry == 1, and 0 if carry == 0
 */
static zb_uint8_t zb_ecc_add(const zb_uint32_t *a, const zb_uint32_t *b, zb_uint32_t *aplusb)
{
    /* Code Fix:DD:fixed, make carry and T variables static to avoid memory allocation at stack. */
    zb_uint8_t i;
    static zb_uint32_t carry;
    static zb_uint64_t T;

    carry = 0;
    for (i = 0; i < g_ecc_pub_key_len_in_dwords; i++)
    {
        T = (zb_uint64_t) * (a + i) + *(b + i) + carry;
        *(aplusb + i) = (zb_uint32_t)T;
        carry = (zb_uint32_t)(T >> 32);
    }

    return (1U == carry) ? 1U : 0U;
}


/**
 * Big integer substraction
 * @param  a     - the first operand
 * @param  b     - the second operand
 * @param  asubb - result of a - b
 * @param  m     - length of a and b arrays
 * @return  0 if borrow != 0 and 1 if borrow == 0
 */
static zb_uint8_t zb_ecc_sub(const zb_uint32_t *a, const zb_uint32_t *b, const zb_uint8_t len, zb_uint32_t *asubb)
{
    /* DD: made borrow and T variables static to avoid memory allocation at stack. */
    zb_uint8_t i;
    static zb_uint32_t borrow;
    static zb_uint64_t T;

    /*FIXME: correct determine return constants*/
    borrow = 1;
    for (i = 0; i < len; i++)
    {
        T = (zb_uint64_t) * (a + i) + ~*(b + i) + borrow;
        *(asubb + i) = (zb_uint32_t)T;
        borrow = (zb_uint32_t)(T >> 32);
    }

    return (0U == borrow) ? 1U : 0U;
}


/**
 * Modular addition over GF(p) field
 * @param a        - the first summand
 * @param b        - the second summand
 * @param aplusb   - result of a + b operation
 */
static void zb_ecc_modpadd(const zb_uint32_t *a, const zb_uint32_t *b, zb_uint32_t *aplusb)
{

    if (1U == zb_ecc_add(a, b, aplusb))
    {
        (void)zb_ecc_sub(aplusb, eccn[g_curve_id], g_ecc_pub_key_len_in_dwords, aplusb);
        return;
    }
    else
    {
        while (zb_ecc_compare(aplusb, eccn[g_curve_id], g_ecc_pub_key_len_in_dwords) != ZB_BI_LESS)
        {
            (void)zb_ecc_sub(aplusb, eccn[g_curve_id], g_ecc_pub_key_len_in_dwords, aplusb);
        }
    }
}

/**
 * Modular multiplication over GF(p) with one operand 160 bit
 * @param a        - the first operand
 * @param b        - the second operand
 * @param amulb    - result of a * b over GF(p) field
 */
static void zb_ecc_modpmul160(const zb_uint32_t *a, const zb_uint32_t *b, zb_uint32_t *amulb)
{
    zb_uint8_t i;
    zb_uint8_t j;
    // zb_uint32_t c[ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS+4];
    zb_uint32_t *c = shared_buf1;
    // zb_uint32_t p[ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS+1];
    zb_uint32_t *p = shared_buf1 + ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS + 4;
    // zb_uint32_t q[ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS+1];
    zb_uint32_t *q = shared_buf1 + 2U * ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS + 5;
    /* DD: Moved T1, T2, T3 variables to globals and shared it between
    zb_ecc_modpmul160, zb_ecc_modpmul96, zb_ecc_partial_mod_sect283k1, zb_ecc_partial_mod functions.*/

    /*c = a * b*/
    T64_1 = (zb_uint64_t)(*a) * (*b);
    *c = (zb_uint32_t)T64_1;
    T64_1 = T64_1 >> 32;

    for (i = 1; i < g_ecc_pub_key_len_in_dwords + 5U; i++)
    {
        T64_3 = 0;
        for (j = (i < g_ecc_pub_key_len_in_dwords ? 0U : (i - g_ecc_pub_key_len_in_dwords + 1U)); j <= (i < 5U ? i : 4U); j++)
        {
            T64_2 = (zb_uint64_t)(*(a + j)) * (*(b + (i - j)));
            T64_1 += (zb_uint32_t)T64_2;
            T64_3 += (T64_2 >> 32);
        }
        *(c + i) = (zb_uint32_t)T64_1;
        T64_1 = (T64_1 >> 32) + T64_3;
    }

    /*Barrett Modular Reduction*/
    T64_1 = (zb_uint64_t)(*(c + g_ecc_pub_key_len_in_dwords - 1)) * (*(eccu[sect283k1]));
    T64_1 = T64_1 >> 32;

    for (i = g_ecc_pub_key_len_in_dwords; i < g_ecc_pub_key_len_in_dwords * 2U; i++)
    {
        T64_3 = 0;
        for (j = g_ecc_pub_key_len_in_dwords - 1U; j <= (i < (g_ecc_pub_key_len_in_dwords + 5U) ? i : (g_ecc_pub_key_len_in_dwords + 4U)); j++)
        {
            T64_2 = (zb_uint64_t)(*(c + j)) * (*(eccu[sect283k1] + (i - j)));
            T64_1 += (zb_uint32_t)T64_2;
            T64_3 += (T64_2 >> 32);
        }
        T64_1 = (T64_1 >> 32) + T64_3;
    }

    for (i = 0; i < 4U; i++)
    {
        T64_3 = 0;
        for (j = i + g_ecc_pub_key_len_in_dwords; j <= g_ecc_pub_key_len_in_dwords + 3U; j++)
        {
            T64_2 = (zb_uint64_t)(*(c + j)) * (*(eccu[sect283k1] + (g_ecc_pub_key_len_in_dwords * 2U + i - j)));
            T64_1 += (zb_uint32_t)T64_2;
            T64_3 += (T64_2 >> 32);
        }
        *(p + i) = (zb_uint32_t)T64_1;
        T64_1 = (T64_1 >> 32) + T64_3;
    }

    T64_2 = (zb_uint64_t) * (c + g_ecc_pub_key_len_in_dwords + 4) * (*(eccu[sect283k1] + g_ecc_pub_key_len_in_dwords));
    T64_1 += (zb_uint32_t)T64_2;
    *(p + 4) = (zb_uint32_t)T64_1;
    *(p + 5) = (zb_uint32_t)(T64_1 >> 32) + (zb_uint32_t)(T64_2 >> 32);

    /*q = p * eccn*/
    T64_1 = (zb_uint64_t)(*p) * (*(eccn[sect283k1]));
    *q = (zb_uint32_t)T64_1;
    T64_1 = T64_1 >> 32;

    for (i = 1; i <= 9U; i++)
    {
        T64_3 = 0;
        /* Refer to ZOI-501 for more details about loop control expression. */
        for (j = 0; j <= (i < 6U ? i : 5U); j++)
        {
            T64_2 = (zb_uint64_t)(*(p + j)) * (*(eccn[sect283k1] + (i - j)));
            T64_1 += (zb_uint32_t)T64_2;
            T64_3 += (T64_2 >> 32);
        }
        *(q + i) = (zb_uint32_t)T64_1;
        T64_1 = (T64_1 >> 32) + T64_3;
    }

    /*Use ZB_MEMCPY instead of explicit assignment in the loop*/
    ZB_MEMCPY(p, eccn[sect283k1], 4U * ((zb_uint32_t)g_ecc_pub_key_len_in_dwords + 1U));
    // for (i = 0; i <= g_ecc_pub_key_len_in_dwords; i++)
    // {
    //   *(p + i) = *(eccn[sect283k1] + i);
    // }
    //*p = *(eccn[sect283k1]);
    //*(p + g_ecc_pub_key_len_in_dwords) = 0;

    if (zb_ecc_sub(c, q, g_ecc_pub_key_len_in_dwords + 1U, q) == 1U)
    {
        /*Use ZB_MEMCPY instead of explicit assignment in the loop*/
        ZB_MEMCPY(amulb, q, 4U * (zb_uint32_t)g_ecc_pub_key_len_in_dwords);
        // for (i = g_ecc_pub_key_len_in_dwords - 1; i != 0; i--)
        // {
        //   *(amulb + i) = *(q + i);
        // }
        // *amulb = *q;

        return;
    }

    while (zb_ecc_compare(q, p, g_ecc_pub_key_len_in_dwords + 1U) != ZB_BI_LESS)
    {
        (void)zb_ecc_sub(q, p, g_ecc_pub_key_len_in_dwords + 1U, q);
    }
    /*Use ZB_MEMCPY instead of explicit assignment in the loop*/
    ZB_MEMCPY(amulb, q, 4U * (zb_uint32_t)g_ecc_pub_key_len_in_dwords);
    // for (i = g_ecc_pub_key_len_in_dwords - 1; i != 0; i--)
    // {
    //   *(amulb + i) = *(q + i);
    // }
    // *amulb = *q;
}


/**
 * Modular multiplication over GF(p) field with one operand 96-bit.
 * This function is only used for the curve sect163k1.
 * @param a     - the first operand
 * @param b     - the second operand
 * @param amulb - result of a * b over GF(p) field
 */
static void zb_ecc_modpmul96(const zb_uint32_t *a, const zb_uint32_t *b, zb_uint32_t *amulb)
{
    zb_uint32_t i, j;
    // zb_uint32_t c[8];
    // zb_uint32_t p[7];
    // zb_uint32_t q[7];
    zb_uint32_t *c = shared_buf1;
    zb_uint32_t *p = shared_buf1 + 8;
    zb_uint32_t *q = shared_buf1 + 15;

    /*c = a * b*/
    T64_1 = (zb_uint64_t)(*a) * (*b);
    *c = (zb_uint32_t)T64_1;
    T64_1 = T64_1 >> 32;

    for (i = 1; i < 7U; i++)
    {
        T64_3 = 0;
        for (j = (i < 6U ? 0U : (i - 5U)); j <= (i < 3U ? i : 2U); j++)
        {
            T64_2 = (zb_uint64_t)(*(a + j)) * (*(b + (i - j)));
            T64_1 += (zb_uint32_t)T64_2;
            T64_3 += (T64_2 >> 32);
        }
        *(c + i) = (zb_uint32_t)T64_1;
        T64_1 = (T64_1 >> 32) + T64_3;
    }

    *(c + 7) = (zb_uint32_t)T64_1 + (*(a + 2)) * (*(b + 5));

    /*Barrett Modular Reduction*/
    T64_1 = (zb_uint64_t) * (c + 5) * (*eccu[0]);
    T64_1 = T64_1 >> 32;

    for (i = 6; i < 12U; i++)
    {
        T64_3 = 0;
        for (j = 5; j <= (i < 8U ? i : 7U); j++)
        {
            T64_2 = (zb_uint64_t) * (c + j) * (*(eccu[0] + (i - j)));
            T64_1 += (zb_uint32_t)T64_2;
            T64_3 += (T64_2 >> 32);
        }
        T64_1 = (T64_1 >> 32) + T64_3;
    }

    T64_2 = (zb_uint64_t) * (c + 6) * (*(eccu[0] + 6));
    T64_1 += (zb_uint32_t)T64_2;
    T64_3 = (T64_2 >> 32);
    T64_2 = (zb_uint64_t) * (c + 7) * (*(eccu[0] + 5));
    T64_1 += (zb_uint32_t)T64_2;
    T64_3 += (T64_2 >> 32);
    *p = (zb_uint32_t)T64_1;
    T64_1 = (T64_1 >> 32) + T64_3;

    T64_2 = (zb_uint64_t) * (c + 7) * (*(eccu[0] + 6));
    T64_1 += (zb_uint32_t)T64_2;
    *(p + 1) = (zb_uint32_t)T64_1;
    *(p + 2) = (zb_uint32_t)(T64_1 >> 32) + (zb_uint32_t)(T64_2 >> 32);

    /*q = p * eccn*/
    T64_1 = (zb_uint64_t)(*p) * (*eccn[0]);
    *q = (zb_uint32_t)T64_1;
    T64_1 = T64_1 >> 32;

    for (i = 1; i < 5U; i++)
    {
        T64_3 = 0;
        for (j = (i < 3U ? 0U : (i - 2U)); j <= (i < 3U ? i : 2U); j++)
        {
            T64_2 = (zb_uint64_t) * (p + j) * (*(eccn[0] + (i - j)));
            T64_1 += (zb_uint32_t)T64_2;
            T64_3 += (T64_2 >> 32);
        }
        *(q + i) = (zb_uint32_t)T64_1;
        T64_1 = (T64_1 >> 32) + T64_3;
    }

    T64_2 = (zb_uint64_t)(*p) * (*(eccn[0] + 5));
    T64_1 += (zb_uint32_t)T64_2;
    *(q + 5) = (zb_uint32_t)T64_1;
    T64_1 = (T64_1 >> 32) + (T64_2 >> 32);

    *(q + 6) = (zb_uint32_t)T64_1 + (*(p + 1)) * (*(eccn[0] + 5));

    /*Use ZB_MEMCPY instead of explicit assignment in the loop*/
    ZB_MEMCPY(p, eccn, 28);
    // for (i = 5; i != 0; i--)
    // {
    //   *(p + i) = *(eccn[0] + i);
    // }
    // *p = *eccn[0];
    // *(p + 6) = 0;

    if (zb_ecc_sub(c, q, 7, q) == 1U)
    {
        /*Use ZB_MEMCPY instead of explicit assignment in the loop*/
        ZB_MEMCPY(amulb, q, 24);
        // for (i = 5; i != 0; i--)
        // {
        //   *(amulb + i) = *(q + i);
        // }
        // *amulb = *q;

        return;
    }

    while (zb_ecc_compare(q, p, 7) != ZB_BI_LESS)
    {
        (void)zb_ecc_sub(q, p, 7, q);
    }
    /*Use ZB_MEMCPY instead of explicit assignment in the loop*/
    ZB_MEMCPY(amulb, q, 24);
    // for (i = 5; i != 0; i--)
    // {
    //   *(amulb + i) = *(q + i);
    // }
    // *amulb = *q;
}


/**
 * Compute halftrace over GF(2^n)
 * @param a        - operand for halftrace computing
 * @param b        - result of computing halftrace
 */
static void zb_ecc_halftrace(const zb_uint32_t *a, zb_uint32_t *b)
{
    zb_uint16_t i;
    zb_uint8_t j;
    // zb_uint32_t c[ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS];
    /*NOTE: zb_ecc_halftrace function is only called from zb_ecc_point_decompression function*/
    zb_uint32_t *c = shared_buf1 + 2U * ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS;  /*NOTE: length: ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES*/

    ZB_MEMCPY(c, a, 4U * (zb_uint32_t)g_ecc_pub_key_len_in_dwords);

    for (i = 1; i <= (g_ecc_bitlen - 1U) / 2U; i++)
    {
        zb_ecc_modsq(c, c);
        zb_ecc_modsq(c, c);
        for (j = 0; j < g_ecc_pub_key_len_in_dwords; j++)
        {
            *(c + j) ^= *(a + j);
        }
    }

    ZB_MEMCPY(b, c, 4U * (zb_uint32_t)g_ecc_pub_key_len_in_dwords);
}


/**
 * Point compression
 * @param P        - pointer to elliptic curve point in affine coordinates
 * @param Pc       - pointer to elliptic curve point in compressed form. Result of function
 */
static void zb_ecc_point_compression(const zb_ecc_point_aff_t *P, zb_uint32_t *Pc)
{
    // zb_uint32_t z[ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS];
    zb_uint32_t *z = shared_buf1;
    zb_uint8_t Num;
    if (sect163k1 == g_curve_id)
    {
        Num = 6;
        if (IF0_163(P->x))
        {
            if (IF0_163(P->y))  /*P is infinity point*/
            {
                XIS0(Pc, 4U * (zb_uint32_t)Num);
                return;
            }
            *z = 0;
        }
    }
    else if (sect283k1 == g_curve_id)
    {
        Num = 10;
        if (IF0_283(P->x))
        {
            if (IF0_283(P->y))  /*P is infinity point*/
            {
                XIS0(Pc, 4U * (zb_uint32_t)Num);
                return;
            }
            *z = 0;
        }
    }
    else
    {
        return;
    }

    zb_ecc_modinv(P->x, z);
    zb_ecc_modmul(P->y, z, z);

    ZB_MEMCPY(Pc, P->x, 4U * (zb_uint32_t)Num);

    if (sect163k1 == g_curve_id)
    {
        *(Pc + 5) &= 0x07U;
        *(Pc + 5) |= (0U == (*z & 0x01U)) ? 0x0200U : 0x0300U;
    }
    else
    {
        ZB_ASSERT(sect283k1 == g_curve_id);
        *(Pc + 9) = (0U == (*z & 0x01U)) ? 0x02U : 0x03U;
    }
}


/**
 * Elliptic curve point decompression
 * @param  Pc       - pointer on compressed elliptic curve point
 * @param  P        - pointer on elliptic curve point in affine coordinates
 * @return RET_OK or RET_ERROR
 */
zb_ret_t zb_ecc_point_decompression(const zb_uint32_t *Pc, zb_ecc_point_aff_t *P)
{
    zb_uint8_t yp;
    zb_uint8_t Num;
    // zb_uint32_t beta[ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS];
    zb_uint32_t *beta = shared_buf1;  /*NOTE: length : ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES*/
    // zb_uint32_t z[ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS];
    zb_uint32_t *z = shared_buf1 + ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS;  /*NOTE: length : ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES*/

    //TRACE_MSG(TRACE_ZSE1, "> zb_ecc_point_decompression", (FMT__0));

    /*AEV: default case not needed because g_curve_id setted up by internal function*/
    if (sect163k1 == g_curve_id)
    {
        Num = 6;
        ZB_MEMCPY(z, Pc, 24);
        if ((*(z + 5) >> 8) == 0x02U)
        {
            yp = 0;
        }
        else if ((*(z + 5) >> 8) == 0x03U)
        {
            yp = 1;
        }
        else
        {
            /*        TRACE_MSG(TRACE_ZSE1, "> return error 1", (FMT__0));
                  DUMP_TRAF("Pc,6",z,Num*4,0);*/
            return RET_ERROR;
        }
        *(z + 5) &= 0xFFU;
    }
    else
    {
        ZB_ASSERT(sect283k1 == g_curve_id);
        Num = 10;
        ZB_MEMCPY(z, Pc, 40);

        if ((*(z + 9)) == 0x2U)
        {
            yp = 0;
        }
        else if ((*(z + 9)) == 0x3U)
        {
            yp = 1;
        }
        else
        {
            /*        TRACE_MSG(TRACE_ZSE1, "> return error 1", (FMT__0));
                  DUMP_TRAF("Pc,10",z,Num*4,0);*/
            return RET_ERROR;
        }
        /**(z + 9) &= 0xffff;*/
        *(z + 9) = 0;
    }

    if ((sect163k1 == g_curve_id ? IF0_163(Pc) : IF0_283(Pc)))
    {
        XIS0(P->x, 4U * (zb_uint32_t)Num);
        XIS1(P->y, 4U * (zb_uint32_t)Num);

        return RET_OK;
    }
    else
    {
        //dump_bytes((zb_uint8_t *)z, 40, "z:");
        ZB_MEMSET(beta, 0, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);
        //ZB_MEMSET(&P->y, 0, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES); /-- NOTE: it is necessary for some public keys --/

        zb_ecc_modinv(z, beta);
        zb_ecc_modsq(beta, beta);
        zb_ecc_modadd(z, beta, beta);
        //dump_bytes((zb_uint8_t *)beta, 40, "beta:");
        //printf("y=%d\n", yp);

        if (sect163k1 == g_curve_id)
        {
            *beta ^= 0x01U;
            if ((*beta & 0x01U) == ((*(beta + 4U) >> 29) & 0x01U))  /*trace(c) = c0 + c157*/
            {
                zb_ecc_halftrace(beta, beta);
                if ((*beta & 0x01U) == yp)
                {
                    zb_ecc_modmul(z, beta, P->y);
                }
                else
                {
                    *beta ^= 0x01U;
                    zb_ecc_modmul(z, beta, P->y);
                }
                ZB_MEMCPY(P->x, z, 24);

                return RET_OK;
            }
            else
            {
                //TRACE_MSG(TRACE_ZSE1, "> return error 2", (FMT__0));
                return RET_ERROR;
            }
        }
        else
        {
            ZB_ASSERT(sect283k1 == g_curve_id);
            //printf("c0=%d,c271=%d\n", (*beta & 0x1), ((*(beta + 8) >> 15) & 0x1));
            if ((*beta & 0x01U) == ((*(beta + 8U) >> 15) & 0x01U))  /*trace(c) = c0 + c271*/
            {
                zb_ecc_halftrace(beta, beta);
                if ((*beta & 0x1U) == yp)
                {
                    zb_ecc_modmul(z, beta, P->y);
                }
                else
                {
                    *beta ^= 0x1U;
                    zb_ecc_modmul(z, beta, P->y);
                }
                ZB_MEMCPY(P->x, z, 40);

                return RET_OK;
            }
            else
            {
                //TRACE_MSG(TRACE_ZSE1, "> return error 2", (FMT__0));
                return RET_ERROR;
            }
        }
    }
    return RET_OK; //FIXME: Or Return RET_ERROR
}


/**
 * Point negation
 * @param P        - pointer to elliptic curve point in affine coordinates
 * @param negP     - pointer to elliptic curve point in affine coordinates. Result of point negation.
 */
void zb_ecc_point_negation(const zb_ecc_point_aff_t *P, zb_ecc_point_aff_t *negP)
{
    zb_uint8_t i;

    if (((sect163k1 == g_curve_id) ? (IF0_163(P->x) && IF0_163(P->y)) : (IF0_283(P->x) && IF0_283(P->y))))
    {
        XIS0(negP->x, 4U * (zb_uint32_t)g_ecc_pub_key_len_in_dwords);
        XIS0(negP->y, 4U * (zb_uint32_t)g_ecc_pub_key_len_in_dwords);

        return;
    }

    // ZB_MEMCPY(negP->x, P->x, 4*g_ecc_pub_key_len_in_dwords);
    /*AEV:not efficient <= x-1 change to < x*/
    for (i = 0; i < g_ecc_pub_key_len_in_dwords; i++)
    {
        negP->y[i] = P->x[i] ^ P->y[i];
        negP->x[i] = P->x[i];
    }
}


/**
 * Point conversion from project coordinates to affine coordinates
 * @param Qp       - pointer to elliptic curve in project coordinates
 * @param Qa       - pointer to elliptic curve in affine coordinates
 */
void zb_ecc_project_to_affine(const zb_ecc_point_pro_t *Qp, zb_ecc_point_aff_t *Qa)
{
    // zb_uint32_t Zinv[ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS];
    zb_uint32_t *Zinv = shared_buf2;  /*NOTE: length : ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS*/

    if (((sect163k1 == g_curve_id) ? IF0_163(Qp->Z) : IF0_283(Qp->Z)))
    {
        XIS0(Qa->x, 4U * (zb_uint32_t)g_ecc_pub_key_len_in_dwords);
        XIS0(Qa->y, 4U * (zb_uint32_t)g_ecc_pub_key_len_in_dwords);

        return;
    }

    zb_ecc_modinv(Qp->Z, Zinv);
    zb_ecc_modmul(Qp->X, Zinv, Qa->x);
    zb_ecc_modsq(Zinv, Zinv);
    zb_ecc_modmul(Qp->Y, Zinv, Qa->y);
}


/**
 * Point doubling over GF(2^n) field
 * @param P        - pointer to elliptic curve point in projective coordinate
 * @param Q        - pointer to elliptic curve point in projective coordinate. Result of P point doubling
 */
void zb_ecc_point_doubling(const zb_ecc_point_pro_t *P, zb_ecc_point_pro_t *Q)
{
    // zb_uint32_t T1[ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS];
    /*NOTE: shared_buf2 is used because zb_ecc_point_doubling is called only from zb_ecc_mixed_addition*/
    zb_uint32_t *T1 = shared_buf2 + 3U * ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS; /*NOTE: length : ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES*/
    // zb_uint32_t T2[ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS];
    zb_uint32_t *T2 = shared_buf2 + 3U * ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS; /*NOTE: length : ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES*/

    if (((sect163k1 == g_curve_id) ? IF0_163(P->Z) : IF0_283(P->Z)))
    {
        /*If P = infinity then return infinity*/
        XIS1(Q->X, (zb_uint32_t)g_ecc_pub_key_len_in_dwords);
        XIS0(Q->Y, 4U * (zb_uint32_t)g_ecc_pub_key_len_in_dwords);
        XIS0(Q->Z, 4U * (zb_uint32_t)g_ecc_pub_key_len_in_dwords);

        return;
    }

    zb_ecc_modsq(P->Z, T1);
    zb_ecc_modsq(P->X, T2);
    zb_ecc_modmul(T1, T2, Q->Z);
    zb_ecc_modsq(T2, Q->X);
    zb_ecc_modsq(T1, T1);
    zb_ecc_modadd(Q->X, T1, Q->X);
    zb_ecc_modsq(P->Y, T2);
    if (g_curve_id == sect163k1)
    {
        zb_ecc_modadd(T2, Q->Z, T2);
    }
    zb_ecc_modadd(T1, T2, T2);
    zb_ecc_modmul(Q->X, T2, Q->Y);
    zb_ecc_modmul(T1, Q->Z, T2);
    zb_ecc_modadd(Q->Y, T2, Q->Y);
}


/**
 * Point conversion from affine coordinate to projective coordinate
 * @param Qa       - pointer to elliptic curve point in affine coordinate
 * @param Qp       - pointer to elliptic curve point in projective coordinate
 */
void zb_ecc_affine_to_project(const zb_ecc_point_aff_t *Qa, zb_ecc_point_pro_t *Qp)
{
    ZB_MEMCPY(Qp->X, Qa->x, 4U * (zb_uint32_t)g_ecc_pub_key_len_in_dwords);
    ZB_MEMCPY(Qp->Y, Qa->y, 4U * (zb_uint32_t)g_ecc_pub_key_len_in_dwords);
    XIS1(Qp->Z, (zb_uint32_t)g_ecc_pub_key_len_in_dwords);
}


/**
 * Points mixed addition over GF(2^n) field
 * @param P        - pointer to elliptic curve point in projective coordinate
 * @param Q        - pointer to elliptic curve point in affine coordinate
 * @param R        - pointer to elliptic curve point in projective coordinate. Result of P + Q
 */
void zb_ecc_mixed_addition(zb_ecc_point_pro_t *P, const zb_ecc_point_aff_t *Q, zb_ecc_point_pro_t *R)
{
    /*FIXME:P is not const*/
    /*FIXME: Use global variables*/
    // zb_uint32_t T1[ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS];
    // zb_uint32_t T2[ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS];
    // zb_uint32_t T3[ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS];
    zb_uint32_t *T1 = shared_buf2;  /*NOTE: length : ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES*/
    zb_uint32_t *T2 = shared_buf2 + ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS;  /*NOTE: length : ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES*/
    zb_uint32_t *T3 = shared_buf2 + 2U * ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS;  /*NOTE: length : ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES*/
    zb_uint8_t t1;
    zb_uint8_t t2;

    if (sect163k1 == g_curve_id)
    {
        t1 = ZB_B2U(IF0_163(Q->x) && IF0_163(Q->y));
        t2 = ZB_B2U(IF0_163(P->Z));
    }
    else
    {
        ZB_ASSERT(sect283k1 == g_curve_id);
        t1 = ZB_B2U(IF0_283(Q->x) && IF0_283(Q->y));
        t2 = ZB_B2U(IF0_283(P->Z));
    }

    /*If Q = infinity then return P*/
    if (t1 != 0U)
    {
        ZB_MEMCPY(R->X, P->X, 4U * (zb_uint32_t)g_ecc_pub_key_len_in_dwords);
        ZB_MEMCPY(R->Y, P->Y, 4U * (zb_uint32_t)g_ecc_pub_key_len_in_dwords);
        ZB_MEMCPY(R->Z, P->Z, 4U * (zb_uint32_t)g_ecc_pub_key_len_in_dwords);

        return;
    }

    /*If P = infinity then return (x2 : y2 : 1)*/
    if (t2 != 0U)
    {
        ZB_MEMCPY(R->X, Q->x, 4U * (zb_uint32_t)g_ecc_pub_key_len_in_dwords);
        ZB_MEMCPY(R->Y, Q->y, 4U * (zb_uint32_t)g_ecc_pub_key_len_in_dwords);
        XIS1(R->Z, (zb_uint32_t)g_ecc_pub_key_len_in_dwords);

        return;
    }

    // ZB_MEMSET(T1, 0, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);
    // ZB_MEMSET(T2, 0, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);
    // ZB_MEMSET(T3, 0, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);

    zb_ecc_modmul(P->Z, Q->x, T1);
    zb_ecc_modsq(P->Z, T2);
    zb_ecc_modadd(P->X, T1, R->X);
    zb_ecc_modmul(P->Z, R->X, T1);
    zb_ecc_modmul(T2, Q->y, T3);
    zb_ecc_modadd(P->Y, T3, R->Y);

    if (sect163k1 == g_curve_id)
    {
        t1 = ZB_B2U(IF0_163(R->X));
        t2 = ZB_B2U(IF0_163(R->Y));
    }
    else
    {
        ZB_ASSERT(sect283k1 == g_curve_id);

        t1 = ZB_B2U(IF0_283(R->X));
        t2 = ZB_B2U(IF0_283(R->Y));
    }

    if (t1 != 0U)
    {
        if (t2 != 0U)
        {
            zb_ecc_affine_to_project(Q, P);
            zb_ecc_point_doubling(P, R);

            return;
        }
        else
        {
            XIS1(R->X, (zb_uint32_t)g_ecc_pub_key_len_in_dwords);
            XIS0(R->Y, 4U * (zb_uint32_t)g_ecc_pub_key_len_in_dwords);
            XIS0(R->Z, 4U * (zb_uint32_t)g_ecc_pub_key_len_in_dwords);

            return;
        }
    }

    zb_ecc_modsq(T1, R->Z);
    zb_ecc_modmul(T1, R->Y, T3);
    if (g_curve_id == sect163k1)
    {
        zb_ecc_modadd(T1, T2, T1);
    }
    zb_ecc_modsq(R->X, T2);
    zb_ecc_modmul(T2, T1, R->X);
    zb_ecc_modsq(R->Y, T2);
    zb_ecc_modadd(R->X, T2, R->X);
    zb_ecc_modadd(R->X, T3, R->X);
    zb_ecc_modmul(Q->x, R->Z, T2);
    zb_ecc_modadd(T2, R->X, T2);
    zb_ecc_modsq(R->Z, T1);
    zb_ecc_modadd(T3, R->Z, T3);
    zb_ecc_modmul(T3, T2, R->Y);
    zb_ecc_modadd(Q->x, Q->y, T2);
    zb_ecc_modmul(T1, T2, T3);
    zb_ecc_modadd(R->Y, T3, R->Y);
}


/**
 * Frobenius mapping for elliptic curve points. Map (x,y) to (x^2, y^2)
 * @param P        - pointer to elliptic curve point in projective coordinates
 */
void zb_ecc_point_frobenius(zb_ecc_point_pro_t *P)
{
    if ((sect163k1 == g_curve_id ? IF0_163(P->Z) : IF0_283(P->Z)))
    {
        XIS1(P->X, (zb_uint32_t)g_ecc_pub_key_len_in_dwords);
        XIS0(P->Y, 4U * (zb_uint32_t)g_ecc_pub_key_len_in_dwords);

        return;
    }
    zb_ecc_modsq(P->X, P->X);
    zb_ecc_modsq(P->Y, P->Y);
    zb_ecc_modsq(P->Z, P->Z);
}


/**
 * Partial reduction
 * @param k        - Input integer for getting result of partial modular reduction.
 * @param r0       - integer. Result from representation r0 +r1 * tau = k partmod (tau^m - 1) / (tau - 1)
 * @param r1       - integer. Result from representation r0 +r1 * tau = k partmod (tau^m - 1) / (tau - 1)
 */
static void zb_ecc_partial_mod_sect283k1(const zb_uint32_t *k, zb_uint32_t *r0, zb_uint32_t *r1)
{
    // zb_uint32_t kp[5] = {0x0};
    zb_uint32_t *kp = shared_buf2;  /*NOTE: length : 5 dwords*/
    // zb_uint32_t gp[10] = {0x0};
    zb_uint32_t *gp = shared_buf2 + 5;  /*NOTE: length : 10 dwords*/
    // zb_uint32_t jp[6] = {0x0};  /-- NOTE: previous is 5 --/
    zb_uint32_t *jp = shared_buf2 + 15;  /*NOTE: length : 6 dwords*/
    // zb_uint32_t fp[10] = {0x0};  /-- NOTE: previous is 5 --/
    zb_uint32_t *fp = shared_buf2 + 21;  /*NOTE: length : 10 dwords*/
    /*FIXME: do we need to increase length of lambda1i and lambda0i arrays?*/
    // zb_uint32_t lambda0i[6];  /-- integer part of lambda0 --/ /-- NOTE:Previous value is 5 --/
    zb_uint32_t *lambda0i = shared_buf2 + 31;  /*NOTE: length : 6 dwords*/
    // zb_uint32_t lambda1i[6];  /-- integer part of lambda1 --/  /-- NOTE:Previous value is 5 --/
    zb_uint32_t *lambda1i = shared_buf2 + 37; /*NOTE: length : 6 dwords*/
    zb_int32_t lambda0f;  /*fraction part of lambda0*/
    zb_int32_t lambda1f;  /*fraction part of lambda1*/
    zb_uint32_t lambda_aux;
    zb_int8_t h0;
    zb_int8_t h1;
    zb_int32_t lambda;
    zb_uint8_t i, j;


    /*For sect233k1 k' = [k/2^(0 - 16 + (233 - 9) / 2)] = [k / 2^96]*/
    /*NOTE: For sect283k1 k' = [k/2^(0 - 9 + (283 - 9) / 2)] = [k / 2^128]*/
    ZB_MEMSET(kp, 0, 5U * ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES); /*NOTE: it is necessary step*/

    // u = 27;  /-- u = 283 mod 32 = 27 --/
    // v = 0x0;
    //q = 0;
    // w = 0x8000;  /-- NOTE: 2^143, placed in 5 dword, for sect233k1:  + 2^118 --/

    /*k' = [k/2^(a - C + (m - 9)/2)]*/
    for (i = 0; i < 5U; i++)
    {
        *(kp + i) = *(k + i + 4);
    }

    //dump_bytes((zb_uint8_t *)k, 40, "k:");
    //dump_bytes((zb_uint8_t *)kp, 5 * 4, "kp:");
    /*g' = s0 * k'*/
    T64_1 = (zb_uint64_t)(*(ec_s0[sect283k1])) * (*kp);
    *gp = (zb_uint32_t)T64_1;
    T64_1 = T64_1 >> 32;

    for (i = 1; i < 9U; i++)
    {
        T64_3 = 0;
        for (j = (i < 5U ? 0U : (i - 5U) + 1U); j <= (i < 5U ? i : 4U); j++)
        {
            // printf("i=%d,j=%d,i-j=%d\n", i, j, i - j);
            T64_2 = (zb_uint64_t)(*(ec_s0[sect283k1] + j)) * (*(kp + (i - j)));
            T64_1 += (zb_uint32_t)T64_2;
            T64_3 += (T64_2 >> 32);
        }
        *(gp + i) = (zb_uint32_t)T64_1;
        T64_1 = (T64_1 >> 32) + T64_3;
    }

    *(gp + 9) = (zb_uint32_t)T64_1;

    //dump_bytes((zb_uint8_t *)gp, 40, "gp1:");
    /* j'=Vm*[g'/2^m] */
    T64_1 = ((zb_uint64_t) * (gp + 8) >> 27) | (((zb_uint64_t) * (gp + 9) & 0xFFFFU) << 5);

    if (T64_1 != 0U)
    {
        T64_2 = T64_1 * (*(ec_Vm[sect283k1]));
        *jp = (zb_uint32_t)T64_2;
        T64_2 = T64_2 >> 32;

        T64_3 = T64_1 * (*(ec_Vm[sect283k1] + 1));
        T64_2 += (zb_uint32_t)T64_3;
        *(jp + 1) = (zb_uint32_t)T64_2;
        T64_2 = (T64_2 >> 32) + (T64_3 >> 32);

        T64_3 = T64_1 * (*(ec_Vm[sect283k1] + 2));
        T64_2 += (zb_uint32_t)T64_3;
        *(jp + 2) = (zb_uint32_t)T64_2;

        T64_2 = (T64_2 >> 32) + (T64_3 >> 32);

        T64_3 = T64_1 * (*(ec_Vm[sect283k1] + 3));
        T64_2 += (zb_uint32_t)T64_3;
        *(jp + 3) = (zb_uint32_t)T64_2;
        T64_2 = (T64_2 >> 32) + (T64_3 >> 32);

        T64_3 = T64_1 * (*(ec_Vm[sect283k1] + 4));
        T64_2 += (zb_uint32_t)T64_3;
        *(jp + 4) = (zb_uint32_t)T64_2;

        *(jp + 5) = (zb_uint32_t)(T64_2 >> 32) + (zb_uint32_t)(T64_3 >> 32);
    }

    //dump_bytes((zb_uint8_t *)jp, 6*4, "jp:");
    /*Rounding off in Z[t]*/
    /*NOTE: g'0 < 0 and j'0 < 0, lambda0 < 0*/
    T64_1 = (zb_uint64_t)(*gp) + (*jp); // ~(*jp) + 0x1;
    T64_2 = T64_1 >> 32;
    *gp = (zb_uint32_t)T64_1;

    T64_1 = T64_2 + (*(gp + 1)) + *(jp + 1);
    T64_2 = T64_1 >> 32;
    *(gp + 1) = (zb_uint32_t)T64_1;

    T64_1 = T64_2 + (*(gp + 2)) + *(jp + 2);
    T64_2 = T64_1 >> 32;
    *(gp + 2) = (zb_uint32_t)T64_1;

    T64_1 = T64_2 + (*(gp + 3)) + *(jp + 3);
    T64_2 = T64_1 >> 32;
    *(gp + 3) = (zb_uint32_t)T64_1;

    T64_1 = T64_2 + (*(gp + 4)) + *(jp + 4) + 0x8000U;  //NOTE: may be -w
    T64_2 = T64_1 >> 32;
    *(gp + 4) = (zb_uint32_t)T64_1;

    T64_1 = T64_2 + (*(gp + 5));
    T64_2 = T64_1 >> 32;
    *(gp + 5) = (zb_uint32_t)T64_1;

    T64_1 = T64_2 + (*(gp + 6));
    T64_2 = T64_1 >> 32;
    *(gp + 6) = (zb_uint32_t)T64_1;

    T64_1 = T64_2 + (*(gp + 7));
    T64_2 = T64_1 >> 32;
    *(gp + 7) = (zb_uint32_t)T64_1;

    T64_1 = T64_2 + (*(gp + 8));
    T64_2 = T64_1 >> 32;
    *(gp + 8) = (zb_uint32_t)T64_1;

    *(gp + 9) += (zb_uint32_t)T64_2;

    //dump_bytes((zb_uint8_t *)gp, 40, "gp2:");
    /*Obtain the fraction part of lambda0*/
    // lambda0f = (zb_int32_t)((*(gp + 4) >> 16) | ((*(gp + 5) & 0x3) << 16));
    lambda_aux = (*(gp + 4) >> 16) & 0x01FFU;
    /*the fraction of lambda0 is 9 bits*/  // NOTE: previous is 18 bit
    lambda0f = (zb_int32_t)lambda_aux;
    // v = 16 + 9;  // 9
    // w = 7; // 23

    /*Obtain the integer part of lambda0*/
    /*FIXME: lambda is 146 bits in maximum*/
    *(lambda0i + 5) = (*(gp + 9) >> 25) & 0x7FU;
    for (i = 0; i < 5U; i++)
    {
        *(lambda0i + i) = (*(gp + i + 4) >> 25) | (*(gp + i + 5) << 7);
    }

    // dump_bytes((zb_uint8_t *)lambda0i, 6*4, "lambda0i:");
    // dump_bytes((zb_uint8_t *)&lambda0f, 4, "lambda0f:");
    /*round(lambda0)*/
    /*For sect233k1 1/2 = 2^15 / 2^16 = 0x8000 / 0x10000*/
    /*NOTE:For sect283k1 1/2 = 2^8 / 2^9 = 0x100 / 0x200 */
    if (((zb_uint32_t)lambda0f & 0x0100U) == 0x0100U)
    {
        for (j = 0; j < 5U; j++)
        {
            if (*(lambda0i + j) == 0xffffffffUL)
            {
                *(lambda0i + j) = 0;
            }
            else
            {
                lambda0i[j]++;

                break;
            }
        }

        //lambda0f = 0x10000 - lambda0f;
        lambda0f = 0x200 - lambda0f;
    }
    /*FIXME: do we need to do this condition like for sect233k1?*/
    else
    {
        lambda0f = -lambda0f;
    }
    // dump_bytes((zb_uint8_t *)&lambda0f, 4, "lambda0f:");
    // v = 0x0;
    // w = 0x8000;  /-- + 2^143 --/

    /*g' = s1 * k'*/
    T64_1 = (zb_uint64_t)(*(ec_s1[sect283k1])) * (*kp);
    *gp = (zb_uint32_t)T64_1;
    T64_1 = T64_1 >> 32;

    for (i = 1; i < 9U; i++)
    {
        T64_3 = 0;
        for (j = (i < 5U ? 0U : (i - 5U) + 1U); j <= (i < 5U ? i : 4U); j++)
        {
            //printf("i=%d,j=%d,i-j=%d\n", i, j, i - j);
            T64_2 = (zb_uint64_t)(*(ec_s1[sect283k1] + j)) * (*(kp + (i - j)));
            T64_1 += (zb_uint32_t)T64_2;
            T64_3 += (T64_2 >> 32);
        }
        *(gp + i) = (zb_uint32_t)T64_1;
        T64_1 = (T64_1 >> 32) + T64_3;
    }

    *(gp + 9) = (zb_uint32_t)T64_1;

    //dump_bytes((zb_uint8_t *)gp, 40, "gp3:");
    /*j'=Vm*[g'/2^m], sign: +, 136 bits*/
    T64_1 = ((zb_uint64_t) * (gp + 8) >> 27) | (((zb_uint64_t) * (gp + 9) & 0xFFFFU) << 5);
    if (T64_1 != 0U)
    {
        T64_2 = T64_1 * (*(ec_Vm[sect283k1]));
        *jp = (zb_uint32_t)T64_2;
        T64_2 = T64_2 >> 32;

        T64_3 = T64_1 * (*(ec_Vm[sect283k1] + 1));
        T64_2 += (zb_uint32_t)T64_3;
        *(jp + 1) = (zb_uint32_t)T64_2;
        T64_2 = (T64_2 >> 32) + (T64_3 >> 32);

        T64_3 = T64_1 * (*(ec_Vm[sect283k1] + 2));
        T64_2 += (zb_uint32_t)T64_3;
        *(jp + 2) = (zb_uint32_t)T64_2;

        T64_2 = (T64_2 >> 32) + (T64_3 >> 32);

        T64_3 = T64_1 * (*(ec_Vm[sect283k1] + 3));
        T64_2 += (zb_uint32_t)T64_3;
        *(jp + 3) = (zb_uint32_t)T64_2;
        T64_2 = (T64_2 >> 32) + (T64_3 >> 32);

        T64_3 = T64_1 * (*(ec_Vm[sect283k1] + 4));
        T64_2 += (zb_uint32_t)T64_3;
        *(jp + 4) = (zb_uint32_t)T64_2;

        /*NOTE:jp + 4 enough for storing results*/
        *(jp + 5) = (zb_uint32_t)(T64_2 >> 32) + (zb_uint32_t)(T64_3 >> 32);
    }

    //dump_bytes((zb_uint8_t *)jp, 6*4, "jp1:");
    /*Rounding off in Z[t]*/
    /*NOTE: g'1 > 0 and j'1 > 0, lambda1 > 0 */
    T64_1 = (zb_uint64_t)(*gp) + *jp; //~(*jp) + 0x1;
    T64_2 = T64_1 >> 32;
    *gp = (zb_uint32_t)T64_1;

    T64_1 = T64_2 + (*(gp + 1)) + *(jp + 1);
    T64_2 = T64_1 >> 32;
    *(gp + 1) = (zb_uint32_t)T64_1;

    T64_1 = T64_2 + (*(gp + 2)) + *(jp + 2);
    T64_2 = T64_1 >> 32;
    *(gp + 2) = (zb_uint32_t)T64_1;

    T64_1 = T64_2 + (*(gp + 3)) + *(jp + 3);
    T64_2 = T64_1 >> 32;
    *(gp + 3) = (zb_uint32_t)T64_1;

    T64_1 = T64_2 + (*(gp + 4U)) + *(jp + 4U) + 0x8000U;
    T64_2 = T64_1 >> 32;
    *(gp + 4) = (zb_uint32_t)T64_1;

    T64_1 = T64_2 + (*(gp + 5));
    T64_2 = T64_1 >> 32;
    *(gp + 5) = (zb_uint32_t)T64_1;

    T64_1 = T64_2 + (*(gp + 6));
    T64_2 = T64_1 >> 32;
    *(gp + 6) = (zb_uint32_t)T64_1;

    T64_1 = T64_2 + (*(gp + 7));
    T64_2 = T64_1 >> 32;
    *(gp + 7) = (zb_uint32_t)T64_1;

    T64_1 = T64_2 + (*(gp + 8));
    T64_2 = T64_1 >> 32;
    *(gp + 8) = (zb_uint32_t)T64_1;

    *(gp + 9) += (zb_uint32_t)T64_2;

    /*Obtain the fraction part of lambda1*/
    lambda_aux = (*(gp + 4) >> 16) & 0x01FFU;
    /*the fraction of lambda0 is 9 bits*/  // NOTE: previous is 18 bit
    lambda1f = (zb_int32_t)lambda_aux;
    // v = 16 + 9;  // 9
    // w = 7; // 23

    //dump_bytes((zb_uint8_t *)lambda1i, 6*4, "lambda1i:");
    *(lambda1i + 5) =  (*(gp + 9) >> 25) & 0x7FU;
    for (i = 0; i < 5U; i++)
    {
        *(lambda1i + i) = (*(gp + i + 4) >> 25) | (*(gp + i + 5) << 7);
    }

    h0 = 0;
    h1 = 0;

    // dump_bytes((zb_uint8_t *)lambda1i, 6*4, "lambda1i:");
    // dump_bytes((zb_uint8_t *)&lambda1f, 4, "lambda1f:");
    /*round(lambda1)*/
    /*For sect233k1 1/2 = 2^15 / 2^16 = 0x8000 / 0x10000*/
    /*NOTE:For sect283k1 1/2 = 2^8 / 2^9 = 0x100 / 0x200 */

    //if ((lambda1f & 0x8000) == 0x8000)
    if (((zb_uint32_t)lambda1f & 0x0100U) == 0x0100U)
    {
        for (j = 0; j < 5U; j++)
        {
            if (*(lambda1i + j) == 0xffffffffUL)
            {
                *(lambda1i + j) = 0;
            }
            else
            {
                lambda1i[j]++;

                break;
            }
        }

        //lambda1f = 0x10000 - lambda1f;
        //lambda1f = 0x200 - lambda1f;
        lambda1f = lambda1f - 0x200;  //Because lambda1 for sect283k1 > 0
    }

    //dump_bytes((zb_uint8_t *)&lambda1f, 4, "lambda1f:");
    /*eta = 2*eta0 + mu * eta1 = 2 * eta0 - eta1*/
    lambda = 2 * lambda0f - lambda1f;

    if (lambda >= 0x200)
    {
        if ((lambda0f + 3 * lambda1f) < -0x200)
        {
            h1 = -1;
        }
        else
        {
            h0 = 1;
        }
    }
    else
    {
        if ((lambda0f - 4 * lambda1f) >= 0x400) /*0x400 = 2 * 0x200*/
        {
            h1 = -1;
        }
    }

    if (lambda < -0x200)
    {
        if ((lambda0f + 3 * lambda1f) >= 0x200)
        {
            h1 = 1;
        }
        else
        {
            h0 = -1;
        }
    }
    else
    {
        if ((lambda0f - 4 * lambda1f) < -0x400) /*0x400 = 2 * 0x200 */
        {
            h1 = 1;
        }
    }

    //printf("h0=%d,h1=%d\n",h0,h1);
    /*q0 = f0 + h0 < 0*/
    if (h0 != 0)
    {
        if (h0 == 1)
        {
            T64_1 = (zb_uint64_t)(*lambda0i) + 0xffffffffUL;
            T64_2 = T64_1 >> 32;
            *lambda0i = (zb_uint32_t)T64_1;

            T64_1 = T64_2 + (*(lambda0i + 1)) + 0xffffffffUL;
            T64_2 = T64_1 >> 32;
            *(lambda0i + 1) = (zb_uint32_t)T64_1;

            T64_1 = T64_2 + (*(lambda0i + 2)) + 0xffffffffUL;
            T64_2 = T64_1 >> 32;
            *(lambda0i + 2) = (zb_uint32_t)T64_1;

            T64_1 = T64_2 + (*(lambda0i + 3)) + 0xffffffffUL;
            T64_2 = T64_1 >> 32;
            *(lambda0i + 3) = (zb_uint32_t)T64_1;

            *(lambda0i + 4) += (zb_uint32_t)T64_2 + 0xffffffffUL;
        }
        else
        {
            for (j = 0; j < 5U; j++)
            {
                if (*(lambda0i + j) == 0xffffffffUL)
                {
                    *(lambda0i + j) = 0;
                }
                else
                {
                    lambda0i[j]++;

                    break;
                }
            }
        }
    }

    /*q1 = f1 + h1 > 0*/
    if (h1 != 0)
    {
        if (h1 == 1)
        {
            for (j = 0; j < 5U; j++)
            {
                if (*(lambda1i + j) == 0xffffffffUL)
                {
                    *(lambda1i + j) = 0;
                }
                else
                {
                    lambda1i[j]++;

                    break;
                }
            }
        }
        else
        {
            T64_1 = (zb_uint64_t)(*lambda1i) + 0xffffffffUL;
            T64_2 = T64_1 >> 32;
            *lambda1i = (zb_uint32_t)T64_1;

            T64_1 = T64_2 + (*(lambda1i + 1)) + 0xffffffffUL;
            T64_2 = T64_1 >> 32;
            *(lambda1i + 1) = (zb_uint32_t)T64_1;

            T64_1 = T64_2 + (*(lambda1i + 2)) + 0xffffffffUL;
            T64_2 = T64_1 >> 32;
            *(lambda1i + 2) = (zb_uint32_t)T64_1;

            T64_1 = T64_2 + (*(lambda1i + 3)) + 0xffffffffUL;
            T64_2 = T64_1 >> 32;
            *(lambda1i + 3) = (zb_uint32_t)T64_1;

            *(lambda1i + 4) += (zb_uint32_t)T64_2 + 0xffffffffUL;
        }
    }

    /*s * f0, f0 < 0, s > 0, fp < 0*/
    T64_1 = (zb_uint64_t)(*(ec_s[sect283k1])) * (*lambda0i);
    *fp = (zb_uint32_t)T64_1;
    T64_1 = T64_1 >> 32;

    for (i = 1; i < 5U; i++)
    {
        T64_3 = 0;
        for (j = 0; j <= i; j++)
        {
            // printf("i=%d,j=%d,i-j=%d\n", i, j, i - j);
            T64_2 = (zb_uint64_t)(*(ec_s[sect283k1] + j)) * (*(lambda0i + (i - j)));
            T64_1 += (zb_uint32_t)T64_2;
            T64_3 += (T64_2 >> 32);
        }
        *(fp + i) = (zb_uint32_t)T64_1;
        T64_1 = (T64_1 >> 32) + T64_3;
    }

    /*NOTE: follow computations are unnecessary because k[i] = fp[i], i >= 5*/
    // for (i = t; i <= 9; i++)
    // {
    //   T64_3 = 0;
    //   for (j = i - t + 1; j < t; j++)
    //   {
    //     // printf("i=%d,j=%d,i-j=%d\n", i, j, i - j);
    //     T64_2 = (zb_uint64_t)(*(ec_s[sect283k1] + j)) * (*(lambda0i + i - j));
    //     T64_1 += (zb_uint32_t)T64_2;
    //     T64_3 += (T64_2 >> 32);
    //   }
    //   *(fp + i) = (zb_uint32_t)T64_1;
    //   T64_1 = (T64_1 >> 32) + T64_3;
    // }

    //dump_bytes((zb_uint8_t *)fp, 40, "fp0:");
    /*s2 * f1 > 0, s2 > 0, f1 > 0, but -s2*f1 < 0*/
    T64_1 = (zb_uint64_t)(*(ec_s2[sect283k1])) * (*lambda1i);
    *gp = (zb_uint32_t)T64_1;
    T64_1 = T64_1 >> 32;

    for (i = 1; i < 5U; i++)
    {
        T64_3 = 0;
        for (j = 0; j <= i; j++)
        {
            // printf("i=%d,j=%d,i-j=%d\n", i, j, i - j);
            T64_2 = (zb_uint64_t)(*(ec_s2[sect283k1] + j)) * (*(lambda1i + (i - j)));
            T64_1 += (zb_uint32_t)T64_2;
            T64_3 += (T64_2 >> 32);
        }
        *(gp + i) = (zb_uint32_t)T64_1;
        T64_1 = (T64_1 >> 32) + T64_3;
    }

    /*NOTE: follow computations are unnecessary because k[i] = fp[i], i >= 5*/
    // for (i = t; i <= 9; i++)
    // {
    //   T64_3 = 0;
    //   for (j = i - t + 1; j < t; j++)
    //   {
    //     // printf("i=%d,j=%d,i-j=%d\n", i, j, i - j);
    //     T64_2 = (zb_uint64_t)(*(ec_s2[sect283k1] + j)) * (*(lambda1i + (i - j)));
    //     T64_1 += (zb_uint32_t)T64_2;
    //     T64_3 += (T64_2 >> 32);
    //   }
    //   *(gp + i) = (zb_uint32_t)T64_1;
    //   T64_1 = (T64_1 >> 32) + T64_3;
    // }

    /*r0 = (k + |fp|) - |gp|*/

    /*NOTE: for sect283k1: r0 = k - |fp| - |gp| = k - (|fp| + |gp|)*/
    T64_1 = (zb_uint64_t)(*fp) + (*gp);
    T64_2 = T64_1 >> 32;
    *fp = (zb_uint32_t)T64_1;

    T64_1 = T64_2 + (*(fp + 1)) + (*(gp + 1));
    T64_2 = T64_1 >> 32;
    *(fp + 1) = (zb_uint32_t)T64_1;

    T64_1 = T64_2 + (*(fp + 2)) + (*(gp + 2));
    T64_2 = T64_1 >> 32;
    *(fp + 2) = (zb_uint32_t)T64_1;

    T64_1 = T64_2 + (*(fp + 3)) + (*(gp + 3));
    T64_2 = T64_1 >> 32;
    *(fp + 3) = (zb_uint32_t)T64_1;

    T64_1 = T64_2 + (*(fp + 4)) + (*(gp + 4));
    T64_2 = T64_1 >> 32;
    *(fp + 4) = (zb_uint32_t)T64_1;
    /*NOTE: follow computations are unnecessary because k[i] = fp[i], i >= 5*/
    // T64_1 = T64_2 + (*(fp + 5)) + (*(gp + 5));
    // T64_2 = T64_1 >> 32;
    // *(fp + 5) = (zb_uint32_t)T64_1;
    //
    // T64_1 = T64_2 + (*(fp + 6)) + (*(gp + 6));
    // T64_2 = T64_1 >> 32;
    // *(fp + 6) = (zb_uint32_t)T64_1;
    //
    // T64_1 = T64_2 + (*(fp + 7)) + (*(gp + 7));
    // T64_2 = T64_1 >> 32;
    // *(fp + 7) = (zb_uint32_t)T64_1;
    //
    // T64_1 = T64_2 + (*(fp + 8)) + (*(gp + 8));
    // T64_2 = T64_1 >> 32;
    // *(fp + 8) = (zb_uint32_t)T64_1;
    //
    // *(fp + 9) = (zb_uint32_t)T64_2 + (*(fp + 9)) + (*(gp + 9));

    // dump_bytes((zb_uint8_t *)fp, 40, "fp:");
    // dump_bytes((zb_uint8_t *)k, 40, "k:");
    /*FIXME: k[10] somitemes includes a trash that changes result of comparing*/
    if (ZB_BI_GREATER == zb_ecc_compare(k, fp, 5))
    {
        T64_1 = (zb_uint64_t)(*k) + ~(*fp) + 0x01U;
        T64_2 = (zb_uint32_t)(T64_1 >> 32);
        *r0 = (zb_uint32_t)T64_1;
        /*FIXME: need to discuss what to prefer T64_2 or j*/
        T64_1 = T64_2 + (*(k + 1)) + ~(*(fp + 1));
        T64_2 = (zb_uint32_t)(T64_1 >> 32);
        *(r0 + 1) = (zb_uint32_t)T64_1;

        T64_1 = T64_2 + (*(k + 2)) + ~(*(fp + 2));
        T64_2 = (zb_uint32_t)(T64_1 >> 32);
        *(r0 + 2) = (zb_uint32_t)T64_1;

        T64_1 = T64_2 + (*(k + 3)) + ~(*(fp + 3));
        j = (zb_uint8_t)((T64_1 >> 32) & 0xFFU);
        *(r0 + 3) = (zb_uint32_t)T64_1;

        *(r0 + 4) = (*(k + 4)) + ~(*(fp + 4)) + j;

        *(r0 + 5) = 1;  //This flag means that r0 is positive
    }
    else if (ZB_BI_LESS == zb_ecc_compare(k, fp, 5))
    {
        T64_1 = (zb_uint64_t)(*fp) + ~(*k) + 0x01U;
        T64_2 = T64_1 >> 32;
        *r0 = (zb_uint32_t)T64_1;

        T64_1 = T64_2 + (*(fp + 1)) + ~(*(k + 1));
        T64_2 = (zb_uint32_t)(T64_1 >> 32);
        *(r0 + 1) = (zb_uint32_t)T64_1;

        T64_1 = T64_2 + (*(fp + 2)) + ~(*(k + 2));
        T64_2 = (zb_uint32_t)(T64_1 >> 32);
        *(r0 + 2) = (zb_uint32_t)T64_1;

        T64_1 = T64_2 + (*(fp + 3)) + ~(*(k + 3));
        j = (zb_uint8_t)((T64_1 >> 32) & 0xFFU);
        *(r0 + 3) = (zb_uint32_t)T64_1;

        *(r0 + 4) = (*(fp + 4)) + ~(*(k + 4)) + j;

        *(r0 + 5) = 2;  // This flag means that r0 is negative
    }
    else
    {
        for (i = 0; i < 6U; i++)
        {
            *(r0 + i) = 0;
        }
    }

    //dump_bytes((zb_uint8_t *)r0, 6*4, "r0:");
    /*|s0|*f1, -s0 * f1 > 0, fp > 0*/
    T64_1 = (zb_uint64_t)(*(ec_s0[sect283k1])) * (*lambda1i);
    *fp = (zb_uint32_t)T64_1;
    T64_1 = T64_1 >> 32;

    for (i = 1; i < 5U; i++)
    {
        T64_3 = 0;
        for (j = 0; j <= i; j++)
        {
            // printf("i=%d,j=%d,i-j=%d\n", i, j, i - j);
            T64_2 = (zb_uint64_t)(*(ec_s0[sect283k1] + j)) * (*(lambda1i + (i - j)));
            T64_1 += (zb_uint32_t)T64_2;
            T64_3 += (T64_2 >> 32);
        }
        *(fp + i) = (zb_uint32_t)T64_1;
        T64_1 = (T64_1 >> 32) + T64_3;
    }

    /*NOTE: follow computations are unnecessary because k[i] = fp[i], i >= 5*/
    // for (i = t; i <= 9; i++)
    // {
    //   T64_3 = 0;
    //   for (j = i - t + 1; j < t; j++)
    //   {
    //     // printf("i=%d,j=%d,i-j=%d\n", i, j, i - j);
    //     T64_2 = (zb_uint64_t)(*(ec_s0[sect283k1] + j)) * (*(lambda1i + (i - j)));
    //     T64_1 += (zb_uint32_t)T64_2;
    //     T64_3 += (T64_2 >> 32);
    //   }
    //   *(fp + i) = (zb_uint32_t)T64_1;
    //   T64_1 = (T64_1 >> 32) + T64_3;
    // }

    /*|s1|*f0 < 0, gp < 0*/
    T64_1 = (zb_uint64_t)(*(ec_s1[sect283k1])) * (*lambda0i);
    *gp = (zb_uint32_t)T64_1;
    T64_1 = T64_1 >> 32;

    for (i = 1; i < 5U; i++)
    {
        T64_3 = 0;
        for (j = 0; j <= i; j++)
        {
            // printf("i=%d,j=%d,i-j=%d\n", i, j, i - j);
            T64_2 = (zb_uint64_t)(*(ec_s1[sect283k1] + j)) * (*(lambda0i + (i - j)));
            T64_1 += (zb_uint32_t)T64_2;
            T64_3 += (T64_2 >> 32);
        }
        *(gp + i) = (zb_uint32_t)T64_1;
        T64_1 = (T64_1 >> 32) + T64_3;
    }

    /*NOTE: follow computations are unnecessary because k[i] = fp[i], i >= 5*/
    // for (i = t; i <= 9; i++)
    // {
    //   T64_3 = 0;
    //   for (j = i - t + 1; j < t; j++)
    //   {
    //     // printf("i=%d,j=%d,i-j=%d\n", i, j, i - j);
    //     T64_2 = (zb_uint64_t)(*(ec_s1[sect283k1] + j)) * (*(lambda0i + (i - j)));
    //     T64_1 += (zb_uint32_t)T64_2;
    //     T64_3 += (T64_2 >> 32);
    //   }
    //   *(gp + i) = (zb_uint32_t)T64_1;
    //   T64_1 = (T64_1 >> 32) + T64_3;
    // }

    /*r1 = fp - gp*/
    if (ZB_BI_GREATER == zb_ecc_compare(fp, gp, 5))
    {
        T64_1 = (zb_uint64_t)(*fp) + ~(*gp) + 0x1U;
        T64_2 = T64_1 >> 32;
        *r1 = (zb_uint32_t)T64_1;

        T64_1 = T64_2 + (*(fp + 1)) + ~(*(gp + 1));
        T64_2 = T64_1 >> 32;
        *(r1 + 1) = (zb_uint32_t)T64_1;

        T64_1 = T64_2 + (*(fp + 2)) + ~(*(gp + 2));
        T64_2 = T64_1 >> 32;
        *(r1 + 2) = (zb_uint32_t)T64_1;

        T64_1 = T64_2 + (*(fp + 3)) + ~(*(gp + 3));
        j = (zb_uint8_t)((T64_1 >> 32) & 0xFFU);
        *(r1 + 3) = (zb_uint32_t)T64_1;

        *(r1 + 4) = (*(fp + 4)) + ~(*(gp + 4)) + j;

        *(r1 + 5) = 1;  //This flag means that r1 is positive
    }
    else if (ZB_BI_LESS == zb_ecc_compare(fp, gp, 5))
    {
        T64_1 = (zb_uint64_t)(*gp) + ~(*fp) + 0x01U;
        T64_2 = T64_1 >> 32;
        *r1 = (zb_uint32_t)T64_1;

        T64_1 = T64_2 + (*(gp + 1)) + ~(*(fp + 1));
        T64_2 = T64_1 >> 32;
        *(r1 + 1) = (zb_uint32_t)T64_1;

        T64_1 = T64_2 + (*(gp + 2)) + ~(*(fp + 2));
        T64_2 = T64_1 >> 32;
        *(r1 + 2) = (zb_uint32_t)T64_1;

        T64_1 = T64_2 + (*(gp + 3)) + ~(*(fp + 3));
        j = (zb_uint8_t)((T64_1 >> 32) & 0xFFU);
        *(r1 + 3) = (zb_uint32_t)T64_1;

        *(r1 + 4) = (*(gp + 4)) + ~(*(fp + 4)) + j;

        *(r1 + 5) = 2;  //This flag means that r1 is negative
    }
    else
    {
        for (i = 0; i < 6U; i++)
        {
            *(r1 + i) = 0;
        }
    }
}


/**
 * Partial reduction
 * @param k        - Input integer for getting result of partial modular reduction.
 * @param r0       - integer. Result from representation r0 +r1 * tau = k partmod (tau^m - 1) / (tau - 1)
 * @param r1       - integer. Result from representation r0 +r1 * tau = k partmod (tau^m - 1) / (tau - 1)
 */
static void zb_ecc_partial_mod(const zb_uint32_t *k, zb_uint32_t *r0, zb_uint32_t *r1)
{
    /*FIXME: Do we need increase length of kp,jp,fp?*/
    // zb_uint32_t kp[6] = {0x0};  /-- NOTE:previous is 5 --/
    /*FIXME: decrease memory usage of shared_buf2*/
    zb_uint32_t *kp = shared_buf2;  /*NOTE: length : 5 dwords*/
    // zb_uint32_t gp[10] = {0x0};  /-- FIXME: ??? may be 9 or 10. NOTE: 9 is enough to avoid segmentation fault. --/
    zb_uint32_t *gp = shared_buf2 + 5;  /*NOTE: length : 10 dwords*/
    // zb_uint32_t jp[6] = {0x0};  /-- NOTE: previous is 5 --/
    zb_uint32_t *jp = shared_buf2 + 15;  /*NOTE: length : 6 dwords*/
    // zb_uint32_t fp[6] = {0x0};  /-- NOTE: previous is 5 --/
    zb_uint32_t *fp = shared_buf2 + 21;  /*NOTE: length : 10 dwords*/
    /*FIXME: do we need to increase length of lambda1i and lambda0i arrays?*/
    // zb_uint32_t lambda0i[5];  /-- integer part of lambda0 --/  /-- NOTE:Previous value is 4 --/
    zb_uint32_t *lambda0i = shared_buf2 + 31;  /*NOTE: length : 5 dwords*/
    // zb_uint32_t lambda1i[5];  /-- integer part of lambda1 --/  /-- NOTE:Previous value is 4 --/
    zb_uint32_t *lambda1i = shared_buf2 + 36; /*NOTE: length : 5 dwords*/
    zb_int32_t lambda0f;  /*fraction part of lambda0*/
    zb_int32_t lambda1f;  /*fraction part of lambda1*/
    zb_uint32_t lambda_aux;
    zb_int8_t h0;
    zb_int8_t h1;
    zb_int32_t lambda;
    // zb_uint32_t u, v, w;
    zb_uint8_t i, j;

    /*k' = [k/2^(a - C + (m - 9)/2)]*/
    /*NOTE:For sect163k1 k' = [k/2^(1 - 14 + (163 - 9) / 2)] = [k / 2^64]*/

    // u = 3;  /-- u = 163 mod 32 = 3 --/
    // v = 0x80000;  /-- + 2^83, NOTE: placed in 3rd dword --/
    // w = 0x0;

    ZB_MEMSET(kp, 0, 5U * ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);  /*NOTE: it is necessary step*/

    for (i = 0; i < 4U; i++)
    {
        *(kp + i) = *(k + i + 2);
    }

    /*g' = s0 * k'*/
    T64_1 = (zb_uint64_t)(*(ec_s0[sect163k1])) * (*kp);
    *gp = (zb_uint32_t)T64_1;
    T64_1 = T64_1 >> 32;

    for (i = 1; i < 5U; i++)
    {
        T64_3 = 0;
        for (j = (i <= 3U ? 0U : (i - 3U)); j <= (i < 3U ? i : 2U); j++)
        {
            T64_2 = (zb_uint64_t)(*(ec_s0[sect163k1] + j)) * (*(kp + (i - j)));
            T64_1 += (zb_uint32_t)T64_2;
            T64_3 += (T64_2 >> 32);
        }
        *(gp + i) = (zb_uint32_t)T64_1;
        T64_1 = (T64_1 >> 32) + T64_3;
    }

    *(gp + 5) = (zb_uint32_t)T64_1 + (*(ec_s0[sect163k1] + 2)) * (*(kp + 3));

    /* j'=Vm*[g'/2^m] */
    T64_1 = (zb_uint64_t)(*(gp + 5U)) >> 3;

    if (T64_1 != 0U)
    {
        T64_2 = T64_1 * (*(ec_Vm[sect163k1]));
        *jp = (zb_uint32_t)T64_2;
        T64_2 = T64_2 >> 32;

        T64_3 = T64_1 * (*(ec_Vm[sect163k1] + 1));
        T64_2 += (zb_uint32_t)T64_3;
        *(jp + 1) = (zb_uint32_t)T64_2;
        T64_2 = (T64_2 >> 32) + (T64_3 >> 32);

        T64_3 = T64_1 * (*(ec_Vm[sect163k1] + 2));
        T64_2 += (zb_uint32_t)T64_3;
        *(jp + 2) = (zb_uint32_t)T64_2;

        *(jp + 3) = (zb_uint32_t)(T64_2 >> 32) + (zb_uint32_t)(T64_3 >> 32);
    }

    /*Rounding off in Z[t]*/
    /*NOTE: g'0 > 0 and j'0 < 0, lambda0 > 0*/
    T64_1 = (zb_uint64_t)(*gp) + ~(*jp) + 0x01U;
    T64_2 = T64_1 >> 32;
    *gp = (zb_uint32_t)T64_1;

    T64_1 = T64_2 + (*(gp + 1)) + ~(*(jp + 1));
    T64_2 = T64_1 >> 32;
    *(gp + 1) = (zb_uint32_t)T64_1;

    T64_1 = T64_2 + (*(gp + 2U)) + ~(*(jp + 2U)) + 0x80000U;
    T64_2 = T64_1 >> 32;
    *(gp + 2) = (zb_uint32_t)T64_1;

    T64_1 = T64_2 + (*(gp + 3)) + ~(*(jp + 3));
    T64_2 = T64_1 >> 32;
    *(gp + 3) = (zb_uint32_t)T64_1;

    T64_1 = T64_2 + (*(gp + 4)) + 0xffffffffUL;
    T64_2 = T64_1 >> 32;
    *(gp + 4) = (zb_uint32_t)T64_1;
    *(gp + 5) += (zb_uint32_t)T64_2 + 0xffffffffUL;

    /*Obtain the fraction part of lambda0*/
    lambda_aux = *(gp + 2U) >> 20 | (*(gp + 3U) & 0x03U) << 12; /*the fraction of lambda0 is 14 bits*/
    ZB_ASSERT(lambda_aux <= (zb_uint32_t)ZB_INT32_MAX);
    lambda0f = (zb_int32_t)lambda_aux;
    // v = 2;
    // w = 30;

    /*Obtain the integer part of lambda0*/
    *(lambda0i + 2) = *(gp + 5) >> 2;
    *(lambda0i + 1) = (*(gp + 4) >> 2) | (*(gp + 5) << 30);
    *(lambda0i) = (*(gp + 3) >> 2) | (*(gp + 4) << 30);

    /*round(lambda0)*/
    if (((zb_uint32_t)lambda0f & 0x2000UL) == 0x2000UL)  /*NOTE: 1/2 = 2^13/2^14 = 0x2000/0x4000 = 1/2*/
    {
        for (j = 0; j < 3U; j++)
        {
            if (*(lambda0i + j) == 0xffffffffUL)
            {
                *(lambda0i + j) = 0;
            }
            else
            {
                lambda0i[j]++;

                break;
            }
        }
        lambda0f = lambda0f - 0x4000;
    }

    // v = 0x80000;  /-- + 2^83 --/
    // w = 0x0;

    /*g' = s1 * k'*/
    T64_1 = (zb_uint64_t)(*(ec_s1[sect163k1])) * (*kp);
    *gp = (zb_uint32_t)T64_1;
    T64_1 = T64_1 >> 32;

    for (i = 1; i < 5U; i++)
    {
        T64_3 = 0;
        for (j = (i <= 3U ? 0U : (i - 3U)); j <= (i < 3U ? i : 2U); j++)
        {
            T64_2 = (zb_uint64_t)(*(ec_s1[sect163k1] + j)) * (*(kp + (i - j)));
            T64_1 += (zb_uint32_t)T64_2;
            T64_3 += (T64_2 >> 32);
        }
        *(gp + i) = (zb_uint32_t)T64_1;
        T64_1 = (T64_1 >> 32) + T64_3;
    }

    *(gp + 5) = (zb_uint32_t)T64_1 + (*(ec_s1[sect163k1] + 2)) * (*(kp + 3));

    /*j'=Vm*[g'/2^m], sign: +, 136 bits*/
    T64_1 = (zb_uint64_t)(*(gp + 5U)) >> 3;

    if (T64_1 != 0U)
    {
        T64_2 = T64_1 * (*(ec_Vm[sect163k1]));
        *jp = (zb_uint32_t)T64_2;
        T64_2 = T64_2 >> 32;

        T64_3 = T64_1 * (*(ec_Vm[sect163k1] + 1));
        T64_2 += (zb_uint32_t)T64_3;
        *(jp + 1) = (zb_uint32_t)T64_2;
        T64_2 = (T64_2 >> 32) + (T64_3 >> 32);

        T64_3 = T64_1 * (*(ec_Vm[sect163k1] + 2));
        T64_2 += (zb_uint32_t)T64_3;
        *(jp + 2) = (zb_uint32_t)T64_2;

        *(jp + 3) = (zb_uint32_t)(T64_2 >> 32) + (zb_uint32_t)(T64_3 >> 32);
    }
    /*Rounding off in Z[t]*/
    /*NOTE: g'1 < 0 and j'1 > 0, lambda1 < 0 */
    T64_1 = (zb_uint64_t)(*gp) + ~(*jp) + 0x01U;
    T64_2 = T64_1 >> 32;
    *gp = (zb_uint32_t)T64_1;

    T64_1 = T64_2 + (*(gp + 1)) + ~(*(jp + 1));
    T64_2 = T64_1 >> 32;
    *(gp + 1) = (zb_uint32_t)T64_1;

    T64_1 = T64_2 + (*(gp + 2U)) + ~(*(jp + 2U)) + 0x80000U;
    T64_2 = T64_1 >> 32;
    *(gp + 2) = (zb_uint32_t)T64_1;

    T64_1 = T64_2 + (*(gp + 3)) + ~(*(jp + 3));
    T64_2 = T64_1 >> 32;
    *(gp + 3) = (zb_uint32_t)T64_1;

    T64_1 = T64_2 + (*(gp + 4)) + 0xffffffffUL;
    T64_2 = T64_1 >> 32;
    *(gp + 4) = (zb_uint32_t)T64_1;
    *(gp + 5) += (zb_uint32_t)T64_2 + 0xffffffffUL;

    /*Obtain the fraction part of lambda1*/
    lambda_aux = *(gp + 2) >> 20 | (*(gp + 3) & 0x03U) << 12; /*the faction of lambda0 is 14 bits*/
    ZB_ASSERT(lambda_aux <= (zb_uint32_t)ZB_INT32_MAX);
    lambda1f = (zb_int32_t)lambda_aux;
    // v = 2;
    // w = 30;

    /*Obtain the integer part of lambda1*/
    *(lambda1i + 2) = *(gp + 5) >> 2;
    *(lambda1i + 1) = (*(gp + 4) >> 2) | (*(gp + 5) << 30);
    *(lambda1i) = (*(gp + 3) >> 2) | (*(gp + 4) << 30);

    h0 = 0;
    h1 = 0;

    /*round(lambda1)*/
    if (((zb_uint32_t)lambda1f & 0x2000UL) == 0x2000UL)
    {
        for (j = 0; j < 3U; j++)
        {
            if (*(lambda1i + j) == 0xffffffffUL)
            {
                *(lambda1i + j) = 0;
            }
            else
            {
                lambda1i[j]++;

                break;
            }
        }
        lambda1f = 0x4000 - lambda1f;
    }
    else
    {
        lambda1f = -lambda1f;
    }

    /*eta=2*eta0+eta1*/
    lambda = 2 * lambda0f + lambda1f;

    if (lambda >= 0x4000)
    {
        if ((lambda0f - 3 * lambda1f) < -0x4000)
        {
            h1 = 1;
        }
        else
        {
            h0 = 1;
        }
    }
    else
    {
        if ((lambda0f + 4 * lambda1f) >= 0x8000)
        {
            h1 = 1;
        }
    }

    if (lambda < -0x4000)
    {
        if ((lambda0f - 3 * lambda1f) >= 0x4000)
        {
            h1 = -1;
        }
        else
        {
            h0 = -1;
        }
    }
    else
    {
        if ((lambda0f + 4 * lambda1f) < -0x8000)
        {
            h1 = -1;
        }
    }

    /*q0 = f0 + h0*/
    if (h0 != 0)
    {
        if (h0 == 1)
        {
            for (j = 0; j < 3U; j++)
            {
                if (*(lambda0i + j) == 0xffffffffUL)
                {
                    *(lambda0i + j) = 0;
                }
                else
                {
                    lambda0i[j]++;

                    break;
                }
            }
        }
        else
        {
            T64_1 = (zb_uint64_t)(*lambda0i) + 0xffffffffUL;
            T64_2 = T64_1 >> 32;
            *lambda0i = (zb_uint32_t)T64_1;

            T64_1 = T64_2 + (*(lambda0i + 1)) + 0xffffffffUL;
            T64_2 = T64_1 >> 32;
            *(lambda0i + 1) = (zb_uint32_t)T64_1;

            *(lambda0i + 2) += (zb_uint32_t)T64_2 + 0xffffffffUL;
        }
    }

    /*q1 = f1 + h1*/
    if (h1 != 0)
    {
        if (h1 == 1)
        {
            T64_1 = (zb_uint64_t)(*lambda1i) + 0xffffffffUL;
            T64_2 = T64_1 >> 32;
            *lambda1i = (zb_uint32_t)T64_1;

            T64_1 = T64_2 + (*(lambda1i + 1)) + 0xffffffffUL;
            T64_2 = T64_1 >> 32;
            *(lambda1i + 1) = (zb_uint32_t)T64_1;

            *(lambda1i + 2) += (zb_uint32_t)T64_2 + 0xffffffffUL;
        }
        else
        {
            for (j = 0; j < 3U; j++)
            {
                if (*(lambda1i + j) == 0xffffffffUL)
                {
                    *(lambda1i + j) = 0;
                }
                else
                {
                    lambda1i[j]++;

                    break;
                }
            }
        }
    }

    /*s * f0*/
    T64_1 = (zb_uint64_t)(*(ec_s[sect163k1])) * (*lambda0i);
    *fp = (zb_uint32_t)T64_1;
    T64_1 = T64_1 >> 32;

    for (i = 1; i < 3U; i++)
    {
        T64_3 = 0;
        for (j = 0; j <= i; j++)
        {
            T64_2 = (zb_uint64_t)(*(ec_s[sect163k1] + j)) * (*(lambda0i + (i - j)));
            T64_1 += (zb_uint32_t)T64_2;
            T64_3 += (T64_2 >> 32);
        }
        *(fp + i) = (zb_uint32_t)T64_1;
        T64_1 = (T64_1 >> 32) + T64_3;
    }

    T64_2 = (zb_uint64_t)(*(ec_s[sect163k1] + 1)) * (*(lambda0i + 2));
    T64_1 = T64_1 + (zb_uint32_t)T64_2;
    *(fp + 3) = (zb_uint32_t)T64_1 + (*(ec_s[sect163k1] + 2)) * (*(lambda0i + 1));

    /*s2 * f1*/
    T64_1 = (zb_uint64_t)(*(ec_s2[sect163k1])) * (*lambda1i);
    *gp = (zb_uint32_t)T64_1;
    T64_1 = T64_1 >> 32;

    for (i = 1; i < 3U; i++)
    {
        T64_3 = 0;
        for (j = 0; j <= i; j++)
        {
            T64_2 = (zb_uint64_t)(*(ec_s2[sect163k1] + j)) * (*(lambda1i + (i - j)));
            T64_1 += (zb_uint32_t)T64_2;
            T64_3 += (T64_2 >> 32);
        }
        *(gp + i) = (zb_uint32_t)T64_1;
        T64_1 = (T64_1 >> 32) + T64_3;
    }

    T64_2 = (zb_uint64_t)(*(ec_s2[sect163k1] + 1)) * (*(lambda1i + 2));
    T64_1 = T64_1 + (zb_uint32_t)T64_2;
    *(gp + 3) = (zb_uint32_t)T64_1 + (*(ec_s2[sect163k1] + 2)) * (*(lambda1i + 1));

    /*r0 = k - (|fp| + |gp|)*/
    T64_1 = (zb_uint64_t)(*fp) + (*gp);
    T64_2 = T64_1 >> 32;
    *fp = (zb_uint32_t)T64_1;

    T64_1 = T64_2 + (*(fp + 1)) + (*(gp + 1));
    T64_2 = T64_1 >> 32;
    *(fp + 1) = (zb_uint32_t)T64_1;

    T64_1 = T64_2 + (*(fp + 2)) + (*(gp + 2));
    T64_2 = T64_1 >> 32;
    *(fp + 2) = (zb_uint32_t)T64_1;

    *(fp + 3) = (zb_uint32_t)T64_2 + (*(fp + 3)) + (*(gp + 3));

    if (ZB_BI_GREATER == zb_ecc_compare(k, fp, 4))
    {
        T64_1 = (zb_uint64_t)(*k) + ~(*fp) + 0x01U;
        T64_2 = T64_1 >> 32;
        *r0 = (zb_uint32_t)T64_1;

        T64_1 = T64_2 + (*(k + 1)) + ~(*(fp + 1));
        /* ZOI-580: 'T64_1' is the result of the sum of 3 32-bit numbers. It will take no more than 34
         * bits for storing the result. So 'zb_uint8_t' variable 'j' is enough to store upper bits. */
        j = (zb_uint8_t)(T64_1 >> 32);
        *(r0 + 1) = (zb_uint32_t)T64_1;
        *(r0 + 2) = (*(k + 2)) + ~(*(fp + 2)) + j;
        *(r0 + 3) = 1;
    }
    else if (ZB_BI_LESS == zb_ecc_compare(k, fp, 4))
    {
        T64_1 = (zb_uint64_t)(*fp) + ~(*k) + 0x01U;
        T64_2 = T64_1 >> 32;
        *r0 = (zb_uint32_t)T64_1;

        T64_1 = T64_2 + (*(fp + 1)) + ~(*(k + 1));
        /* ZOI-580: 'T64_1' is the result of the sum of 3 32-bit numbers. It will take no more than 34
         * bits for storing the result. So 'zb_uint8_t' variable 'j' is enough to store upper bits. */
        j = (zb_uint8_t)(T64_1 >> 32);
        *(r0 + 1) = (zb_uint32_t)T64_1;
        *(r0 + 2) = (*(fp + 2)) + ~(*(k + 2)) + j;
        *(r0 + 3) = 2;
    }
    else
    {
        for (i = 0; i < 4U; i++)
        {
            *(r0 + i) = 0;
        }
    }

    /*|s0|*f1*/
    T64_1 = (zb_uint64_t)(*(ec_s0[sect163k1])) * (*lambda1i);
    *fp = (zb_uint32_t)T64_1;
    T64_1 = T64_1 >> 32;

    for (i = 1; i < 3U; i++)
    {
        T64_3 = 0;
        for (j = 0; j <= i; j++)
        {
            T64_2 = (zb_uint64_t)(*(ec_s0[sect163k1] + j)) * (*(lambda1i + (i - j)));
            T64_1 += (zb_uint32_t)T64_2;
            T64_3 += (T64_2 >> 32);
        }
        *(fp + i) = (zb_uint32_t)T64_1;
        T64_1 = (T64_1 >> 32) + T64_3;
    }

    T64_2 = (zb_uint64_t)(*(ec_s0[sect163k1] + 1)) * (*(lambda1i + 2));
    T64_1 = T64_1 + (zb_uint32_t)T64_2;
    *(fp + 3) = (zb_uint32_t)T64_1 + (*(ec_s0[sect163k1] + 2)) * (*(lambda1i + 1));

    /*|s1|*f0*/
    T64_1 = (zb_uint64_t)(*(ec_s1[sect163k1])) * (*lambda0i);
    *gp = (zb_uint32_t)T64_1;
    T64_1 = T64_1 >> 32;

    for (i = 1; i < 3U; i++)
    {
        T64_3 = 0;
        for (j = 0; j <= i; j++)
        {
            T64_2 = (zb_uint64_t)(*(ec_s1[sect163k1] + j)) * (*(lambda0i + (i - j)));
            T64_1 += (zb_uint32_t)T64_2;
            T64_3 += (T64_2 >> 32);
        }
        *(gp + i) = (zb_uint32_t)T64_1;
        T64_1 = (T64_1 >> 32) + T64_3;
    }

    T64_2 = (zb_uint64_t)(*(ec_s1[0] + sect163k1 * 4U + 1U)) * (*(lambda0i + 2));
    T64_1 = T64_1 + (zb_uint32_t)T64_2;
    *(gp + 3) = (zb_uint32_t)T64_1 + (*(ec_s1[0] + sect163k1 * 4U + 2U)) * (*(lambda0i + 1U));

    /*r1 = fp - gp*/
    if (ZB_BI_GREATER == zb_ecc_compare(fp, gp, 4))
    {
        T64_1 = (zb_uint64_t)(*fp) + ~(*gp) + 0x01U;
        T64_2 = T64_1 >> 32;
        *r1 = (zb_uint32_t)T64_1;

        T64_1 = T64_2 + (*(fp + 1)) + ~(*(gp + 1));
        j = (zb_uint8_t)(T64_1 >> 32);
        *(r1 + 1) = (zb_uint32_t)T64_1;
        *(r1 + 2) = (*(fp + 2)) + ~(*(gp + 2)) + j;
        *(r1 + 3) = 1;
    }
    else if (ZB_BI_LESS == zb_ecc_compare(fp, gp, 4))
    {
        T64_1 = (zb_uint64_t)(*gp) + ~(*fp) + 0x01U;
        T64_2 = T64_1 >> 32;
        *r1 = (zb_uint32_t)T64_1;

        T64_1 = T64_2 + (*(gp + 1)) + ~(*(fp + 1));
        j = (zb_uint8_t)(T64_1 >> 32);
        *(r1 + 1) = (zb_uint32_t)T64_1;
        *(r1 + 2) = (*(gp + 2)) + ~(*(fp + 2)) + j;
        *(r1 + 3) = 2;
    }
    else
    {
        for (i = 0; i < 4U; i++)
        {
            *(r1 + i) = 0;
        }
    }
}


/**
 * Width-5 TNAF expansion
 * @param r0       - value from representation r0 +r1 * tau = k partmod (tau^m - 1) / (tau - 1)
 * @param r1       - value from representation r0 +r1 * tau = k partmod (tau^m - 1) / (tau - 1)
 * @param u        - TNAF5 representation of r0 + r1 * tau
 */
static void zb_ecc_tnaf5_expansion(zb_uint32_t *r0, zb_uint32_t *r1, zb_int8_t *u)
{
    zb_int8_t t;  /*signed char*/
    zb_uint32_t i, c;
    zb_uint32_t r;
    zb_uint8_t j, v;
    zb_uint64_t T;

    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,37} */
    /* zb_ecc_point_pro_t is aligned by 4 */
    zb_ecc_point_pro_t *R = (zb_ecc_point_pro_t *)(shared_buf1 + 3U * ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS);
    // zb_uint32_t s0[5];  /-- NOTE: previous is 4 --/
    // zb_uint32_t s1[5];  /-- NOTE: previous is 4 --/
    // zb_uint32_t s2[5];  /-- NOTE: previous is 4 --/
    zb_uint32_t *s0 = R->X;  /*NOTE: previous is 4*/
    zb_uint32_t *s1 = R->Y;  /*NOTE: previous is 4*/
    zb_uint32_t *s2 = R->Z;  /*NOTE: previous is 4*/
    zb_int32_t flags0, flags0p, flags1, s, sbeta, sgamma;
    static zb_int8_t beta[8] = {1, -3, -1, 1, -3, -1, 1, 1};  /*FIXME: may be enough to use zb_int8_t instead of zb_int32_t*/
    static zb_int8_t gamma[2][8] = {{0, 1, 1, 1, 2, 2, 2, -3}, {0, -1, -1, -1, -2, -2, -2, 3}}; /*FIXME: may be enough to use zb_int8_t instead of zb_int32_t*/

    if (sect163k1 == g_curve_id)
    {
        //Num = 6;
        v = 3;  /*j = Num/2*/
    }
    else if (sect283k1 == g_curve_id)
    {
        //Num = 10;
        v = 5;  /*j = Num/2*/
    }
    else
    {
        return;
    }

    /*Use ZB_MEMCPY instead of explicit assignment in the loop*/
    ZB_MEMCPY(s0, r0, 4U * (zb_uint32_t)v);
    ZB_MEMCPY(s1, r1, 4U * (zb_uint32_t)v);
    // for (i = v - 1; i != 0; i--)
    // {
    //   *(s0 + i) = *(r0 + i);
    //   *(s1 + i) = *(r1 + i);
    // }
    // *s0 = *r0;
    // *s1 = *r1;

    if (*(r0 + v) == 2U)
    {
        flags0 = -1;
    }
    else if (*(r0 + v) == 1U)
    {
        flags0 = 1;
    }
    else
    {
        flags0 = 0;
    }

    if (*(r1 + v) == 2U)
    {
        flags1 = -1;
    }
    else if (*(r1 + v) == 1U)
    {
        flags1 = 1;
    }
    else
    {
        flags1 = 0;
    }

    i = 0;

    while (!(flags0 == 0 && flags1 == 0))
    {
        if ((*s0 & 0x01U) == 0x01U)  /*r0 is not equal to 0*/
        {
            /*compute r0+tw*r1*/
            if (flags0 > 0 && flags1 >= 0)
            {
                /*compute the lowest 32 bits of r0+tw*r1*/
                if (sect163k1 == g_curve_id)
                {
                    r = *s0 + 6U * (*s1);
                }
                else
                {
                    ZB_ASSERT(sect283k1 == g_curve_id);
                    r = *s0 + 26U * (*s1);
                }

                /*get the lowest 5 bits of r0+tw*r1*/
                r = r & 0x1fU;

                /*if (r & 0x10 > 0)*/
                if (r <= 16U)  /*if (r >= 0 && r <= 16)*/
                {
                    t = (zb_int8_t)r;
                }
                else
                {
                    t = (zb_int8_t)r - 32;
                }

                *(u + i) = t;
            }
            else if (flags0 < 0 && flags1 <= 0)
            {
                /*compute the lowest 32 bits of r0+tw*r1*/
                if (sect163k1 == g_curve_id)
                {
                    r = *s0 + 6U * (*s1);
                }
                else
                {
                    ZB_ASSERT(sect283k1 == g_curve_id);
                    r = *s0 + 26U * (*s1);
                }

                /*get the lowest 5 bits of r0+tw*r1*/
                r = r & 0x1FU;

                if (r <= 16U)  /*if (r >= 0 && r <= 16)*/
                {
                    t = -(zb_int8_t)r;
                }
                else
                {
                    t = 32 - (zb_int8_t)r;
                }

                *(u + i) = t;
            }
            else if (flags0 >= 0 && flags1 < 0)
            {
                /*compute the lowest 32 bits of r0-tw*r1=r0+(32+tw)*|r1|*/
                if (sect163k1 == g_curve_id)
                {
                    r = *s0 + 26U * (*s1);
                }
                else
                {
                    ZB_ASSERT(sect283k1 == g_curve_id);
                    r = *s0 + 6U * (*s1);
                }

                /*take out the lowest 5 bits of r0+(32+tw)*|r1|*/
                r = r & 0x1FU;

                if (r <= 16U)  /*if (r >= 0 && r <= 16)*/
                {
                    t = (zb_int8_t)r;
                }
                else
                {
                    t = (zb_int8_t)r - 32;
                }

                *(u + i) = t;
            }
            else  /*g_curve_ids0 <= 0 && g_curve_ids1 > 0*/
            {
                /*compute the lowest 32 bits of r0-tw*r1=-(|r0|+(32+tw)*r1)*/
                if (sect163k1 == g_curve_id)
                {
                    r = *s0 + 26U * (*s1);
                }
                else
                {
                    ZB_ASSERT(sect283k1 == g_curve_id);
                    r = *s0 + 6U * (*s1);
                }

                /*take out the lowest 5 bits of -(|r0|+(32+tw)*r1)*/
                r = r & 0x1FU;

                if (r <= 16U)  /*if (r >= 0 && r <= 16)*/
                {
                    t = -(zb_int8_t)r;
                }
                else
                {
                    t = 32 - (zb_int8_t)r;
                }

                *(u + i) = t;
            }

            if (t > 0)
            {
                s = 1;
            }
            else
            {
                s = -1;
                t = -t;
            }

            /*compute r0 = r0 - s*beta_u*/
            /* ZOI-163: The variable 't' can only take one of the following values {1, 3, 5, 7, 9, 11, 13,
             * 15} so 't - 1' can't be a negative number. Replaced '(t - 1) >> 1' with '(t - 1) / 2' as
             * both expressions are equivalent. */
            sbeta = s * (*(beta + ((t - 1) / 2)));
            if (sbeta > 0)
            {
                r = (zb_uint32_t)(sbeta);
            }
            else
            {
                r = (zb_uint32_t)(-sbeta);
            }

            if (flags0 == 0)
            {
                *s0 = r;
                if (sbeta > 0)
                {
                    flags0 = -1;
                }
                else
                {
                    flags0 = 1;
                }
            }
            else
            {
                if ((flags0 < 0 && sbeta > 0) || (flags0 > 0 && sbeta < 0))  /*addition*/
                {
                    T = (zb_uint64_t)(*s0) + r;
                    *s0 = (zb_uint32_t)T;
                    c = (zb_uint32_t)(T >> 32);
                    j = 1;
                    while (c != 0U && j < v)
                    {
                        T = (zb_uint64_t)(*(s0 + j)) + c;
                        *(s0 + j) = (zb_uint32_t)T;
                        c = (zb_uint32_t)(T >> 32);
                        j++;
                    }
                }
                else  /*subtraction*/
                {
                    if (sect163k1 == g_curve_id)
                    {
                        if (*(s0 + 2U) == 0U && *(s0 + 1U) == 0U && *s0 == r)
                        {
                            *s0 = 0;
                            flags0 = 0;
                        }
                        else if (*(s0 + 2U) == 0U && *(s0 + 1U) == 0U && *s0 < r)
                        {
                            *s0 = r - (*s0);
                            flags0 = -flags0;
                        }
                        else
                        {
                            T = (zb_uint64_t)(*s0) + ~r + 0x01U;
                            *s0 = (zb_uint32_t)T;
                            c = (zb_uint32_t)(T >> 32);
                            for (j = 1; j < 3U; j++)
                            {
                                T = (zb_uint64_t)(*(s0 + j)) + 0xffffffffUL + c;
                                *(s0 + j) = (zb_uint32_t)T;
                                c = (zb_uint32_t)(T >> 32);
                            }
                        }
                    }
                    else
                    {
                        ZB_ASSERT(sect283k1 == g_curve_id);
                        if (*(s0 + 4) == 0U && *(s0 + 3) == 0U && *(s0 + 2) == 0U && *(s0 + 1) == 0U && *s0 == r)
                        {
                            *s0 = 0;
                            flags0 = 0;
                        }
                        else if (*(s0 + 4) == 0U && *(s0 + 3) == 0U && *(s0 + 2) == 0U && *(s0 + 1) == 0U && *s0 < r)
                        {
                            *s0 = r - (*s0);
                            flags0 = -flags0;
                        }
                        else
                        {
                            T = (zb_uint64_t)(*s0) + ~r + 0x01U;
                            *s0 = (zb_uint32_t)T;
                            c = (zb_uint32_t)(T >> 32);
                            for (j = 1; j < v; j++)  // v = 5
                            {
                                T = (zb_uint64_t)(*(s0 + j)) + 0xffffffffUL + c;
                                *(s0 + j) = (zb_uint32_t)T;
                                c = (zb_uint32_t)(T >> 32);
                            }
                        }
                    }
                }
            }

            /*compute r1 = r1 - s*gamma_u*/
            /*NOTE: for curve sect283k1 g_curve_id = 1, and '+ g_curve_id * 8' means
              that we use second 8 elements of gamma. It is equal to gamma[1] + ((t - 1) >> 1)*/
            /*cstat !MISRAC2012-Rule-1.3_n */
            /* TODO: Check JIRA issue ZOI-163 */
            /* Replaced '(t - 1) >> 1' with '(t - 1) / 2' as both expressions are equivalent.
               see previous comment for 'sbeta' calculation. */
            sgamma = s * (*(gamma[0] + g_curve_id * 8U + ((t - 1) / 2)));
            if (sgamma >= 0)
            {
                r = (zb_uint32_t)(sgamma);
            }
            else
            {
                r = (zb_uint32_t)(-sgamma);
            }

            if (flags1 == 0)
            {
                *s1 = r;
                if (sgamma > 0)
                {
                    flags1 = -1;
                }
                if (sgamma < 0)
                {
                    flags1 = 1;
                }
            }
            else
            {
                if ((flags1 < 0 && sgamma > 0) || (flags1 > 0 && sgamma < 0)) /*addition*/
                {
                    T = (zb_uint64_t)(*s1) + r;
                    *s1 = (zb_uint32_t)T;
                    c = (zb_uint32_t)(T >> 32);
                    j = 1;
                    while (c != 0U && j < v)
                    {
                        T = (zb_uint64_t)(*(s1 + j)) + c;
                        *(s1 + j) = (zb_uint32_t)T;
                        c = (zb_uint32_t)(T >> 32);
                        j++;
                    }
                }
                else /*subtraction*/
                {
                    if (sect163k1 == g_curve_id)
                    {
                        if (*(s1 + 2) == 0U && *(s1 + 1) == 0U && *s1 == r)
                        {
                            *s1 = 0;
                            flags1 = 0;
                        }
                        else if (*(s1 + 2) == 0U && *(s1 + 1) == 0U && *s1 < r)
                        {
                            *s1 = r - (*s1);
                            flags1 = -flags1;
                        }
                        else
                        {
                            T = (zb_uint64_t)(*s1) + ~r + 0x01U;
                            *s1 = (zb_uint32_t)T;
                            c = (zb_uint32_t)(T >> 32);
                            for (j = 1; j < 3U; j++)
                            {
                                T = (zb_uint64_t)(*(s1 + j)) + 0xffffffffUL + c;
                                *(s1 + j) = (zb_uint32_t)T;
                                c = (zb_uint32_t)(T >> 32);
                            }
                        }
                    }
                    else
                    {
                        ZB_ASSERT(sect283k1 == g_curve_id);
                        if (*(s1 + 4) == 0U && *(s1 + 3) == 0U && *(s1 + 2) == 0U && *(s1 + 1) == 0U && *s1 == r)
                        {
                            *s1 = 0;
                            flags1 = 0;
                        }
                        else if (*(s1 + 4) == 0U && *(s1 + 3) == 0U && *(s1 + 2) == 0U && *(s1 + 1) == 0U && *s1 < r)
                        {
                            *s1 = r - (*s1);
                            flags1 = -flags1;
                        }
                        else
                        {
                            T = (zb_uint64_t)(*s1) + ~r + 0x01U;
                            *s1 = (zb_uint32_t)T;
                            c = (zb_uint32_t)(T >> 32);
                            for (j = 1; j < v; j++)  // v = 5
                            {
                                T = (zb_uint64_t)(*(s1 + j)) + 0xffffffffUL + c;
                                *(s1 + j) = (zb_uint32_t)T;
                                c = (zb_uint32_t)(T >> 32);
                            }
                        }
                    }
                }
            }
        }
        else
        {
            *(u + i) = 0;
        }

        /*compute r0/2*/
        for (j = 0; j < v - 1U; j++)
        {
            *(s2 + j) = (*(s0 + j) >> 1) | (*(s0 + j + 1) << 31);
        }
        *(s2 + (v - 1U)) = *(s0 + (v - 1U)) >> 1;
        flags0p = flags0;
        if (sect163k1 == g_curve_id)
        {
            /*r0 = r1 + r0/2*/
            if (flags0 == 0 || flags1 == 0)
            {
                if (flags0 == 0)
                {
                    *s0 = *s1;
                    *(s0 + 1) = *(s1 + 1);
                    *(s0 + 2) = *(s1 + 2);
                    flags0 = flags1;
                }
                else
                {
                    *s0 = *s2;
                    *(s0 + 1) = *(s2 + 1);
                    *(s0 + 2) = *(s2 + 2);
                }
            }
            else if (flags0 == flags1)  /*addition (g_curve_ids0 > 0 && g_curve_ids1 > 0) || (g_curve_ids0 < 0 && g_curve_ids1 < 0)*/
            {
                T = (zb_uint64_t)(*s1) + (*s2);
                *s0 = (zb_uint32_t)T;
                c = (zb_uint32_t)(T >> 32);
                T = (zb_uint64_t)(*(s1 + 1)) + (*(s2 + 1)) + c;
                *(s0 + 1) = (zb_uint32_t)T;
                c = (zb_uint32_t)(T >> 32);
                *(s0 + 2) = (*(s1 + 2)) + (*(s2 + 2)) + c;
            }
            else
            {
                if (ZB_BI_GREATER == zb_ecc_compare(s1, s2, 3))  /*r0 = |r1| - |r0|/2*/
                {
                    T = (zb_uint64_t)(*s1) + ~(*s2) + 0x01U;
                    *s0 = (zb_uint32_t)T;
                    c = (zb_uint32_t)(T >> 32);
                    T = (zb_uint64_t)(*(s1 + 1)) + ~(*(s2 + 1)) + c;
                    *(s0 + 1) = (zb_uint32_t)T;
                    c = (zb_uint32_t)(T >> 32);
                    *(s0 + 2) = (*(s1 + 2)) + ~(*(s2 + 2)) + c;
                    flags0 = flags1;
                }
                else if (ZB_BI_LESS == zb_ecc_compare(s1, s2, 3))  /*r0 = |r0|/2 - |r1|*/
                {
                    T = (zb_uint64_t)(*s2) + ~(*s1) + 0x01U;
                    *s0 = (zb_uint32_t)T;
                    c = (zb_uint32_t)(T >> 32);
                    T = (zb_uint64_t)(*(s2 + 1)) + ~(*(s1 + 1)) + c;
                    *(s0 + 1) = (zb_uint32_t)T;
                    c = (zb_uint32_t)(T >> 32);
                    *(s0 + 2) = (*(s2 + 2)) + ~(*(s1 + 2)) + c;
                }
                else  /*r0 = 0*/
                {
                    *s0 = 0;
                    *(s0 + 1) = 0;
                    *(s0 + 2) = 0;
                    flags0 = 0;
                }
            }
        }
        else
        {
            ZB_ASSERT(sect283k1 == g_curve_id);
            /*r0 = r1 - r0/2*/
            if (flags0 == 0 || flags1 == 0)
            {
                if (flags0 == 0)
                {
                    /*Use ZB_MEMCPY instead of explicit assignment in the loop*/
                    ZB_MEMCPY(s0, s1, 4U * (zb_uint32_t)v);
                    // for (j = v - 1; j != 0; j--)  // v = 5
                    // {
                    //   *(s0 + j) = *(s1 + j);
                    // }
                    // *s0 = *s1;
                    flags0 = flags1;
                }
                else
                {
                    /*Use ZB_MEMCPY instead of explicit assignment in the loop*/
                    ZB_MEMCPY(s0, s2, 4U * (zb_uint32_t)v);
                    // for (j = v - 1; j != 0; j--)  // v = 5
                    // {
                    //   *(s0 + j) = *(s2 + j);
                    // }
                    // *s0 = *s2;
                    flags0 = -flags0;
                }
            }
            else if (flags0 != flags1)  /*addition (g_curve_ids0 < 0 && g_curve_ids1 > 0) || (g_curve_ids0 > 0 && g_curve_ids1 < 0)*/
            {
                T = (zb_uint64_t)(*s1) + (*s2);
                *s0 = (zb_uint32_t)T;
                c = (zb_uint32_t)(T >> 32);

                T = (zb_uint64_t)(*(s1 + 1)) + (*(s2 + 1)) + c;
                *(s0 + 1) = (zb_uint32_t)T;
                c = (zb_uint32_t)(T >> 32);

                T = (zb_uint64_t)(*(s1 + 2)) + (*(s2 + 2)) + c;
                *(s0 + 2) = (zb_uint32_t)T;
                c = (zb_uint32_t)(T >> 32);

                T = (zb_uint64_t)(*(s1 + 3)) + (*(s2 + 3)) + c;
                *(s0 + 3) = (zb_uint32_t)T;
                c = (zb_uint32_t)(T >> 32);

                *(s0 + 4) = (*(s1 + 4)) + (*(s2 + 4)) + c;

                flags0 = flags1;
            }
            else
            {
                if (ZB_BI_GREATER == zb_ecc_compare(s1, s2, 5))  /*r0 = |r1| - |r0|/2*/
                {
                    T = (zb_uint64_t)(*s1) + ~(*s2) + 0x01U;
                    *s0 = (zb_uint32_t)T;
                    c = (zb_uint32_t)(T >> 32);

                    T = (zb_uint64_t)(*(s1 + 1)) + ~(*(s2 + 1)) + c;
                    *(s0 + 1) = (zb_uint32_t)T;
                    c = (zb_uint32_t)(T >> 32);

                    T = (zb_uint64_t)(*(s1 + 2)) + ~(*(s2 + 2)) + c;
                    *(s0 + 2) = (zb_uint32_t)T;
                    c = (zb_uint32_t)(T >> 32);

                    T = (zb_uint64_t)(*(s1 + 3)) + ~(*(s2 + 3)) + c;
                    *(s0 + 3) = (zb_uint32_t)T;
                    c = (zb_uint32_t)(T >> 32);

                    *(s0 + 4) = (*(s1 + 4)) + ~(*(s2 + 4)) + c;
                }
                else if (ZB_BI_LESS == zb_ecc_compare(s1, s2, 5))  /*r0 = |r0|/2 - |r1|*/
                {
                    T = (zb_uint64_t)(*s2) + ~(*s1) + 0x01U;
                    *s0 = (zb_uint32_t)T;
                    c = (zb_uint32_t)(T >> 32);
                    T = (zb_uint64_t)(*(s2 + 1)) + ~(*(s1 + 1)) + c;
                    *(s0 + 1) = (zb_uint32_t)T;
                    c = (zb_uint32_t)(T >> 32);

                    T = (zb_uint64_t)(*(s2 + 2)) + ~(*(s1 + 2)) + c;
                    *(s0 + 2) = (zb_uint32_t)T;
                    c = (zb_uint32_t)(T >> 32);

                    T = (zb_uint64_t)(*(s2 + 3)) + ~(*(s1 + 3)) + c;
                    *(s0 + 3) = (zb_uint32_t)T;
                    c = (zb_uint32_t)(T >> 32);

                    *(s0 + 4) = (*(s2 + 4)) + ~(*(s1 + 4)) + c;

                    flags0 = -flags0;
                }
                else  /*r0 = 0*/
                {
                    /*Use ZB_MEMSET instead of explicit assignment in the loop*/
                    ZB_MEMSET(s0, 0, 4U * (zb_uint32_t)v);
                    // for (j = v - 1; j != 0; j--)  // v = 5
                    // {
                    //   *(s0 + j) = 0;
                    // }
                    // *s0 = 0;
                    flags0 = 0;
                }
            }
        }

        /*r1 = -r0/2*/
        /*Use ZB_MEMCPY instead of explicit assignment in the loop*/
        ZB_MEMCPY(s1, s2, 4U * (zb_uint32_t)v);
        // for (j = v - 1; j != 0; j--)
        // {
        //   *(s1 + j) = *(s2 + j);
        // }
        // *s1 = *s2;
        flags1 = -flags0p;

        /*FIXME: need to check/compare i variable with the upper limit.*/
        // printf("i=%d\n", i);
        i++;
    }
}


#ifdef ECC_ENABLE_FIXED_SCALARMUL
/**
 * Scalar multiplication with the width-5 TNAF (fixed point)
 * @param k        - private key
 * @param Q        - public key. Result of multiplication (k * G, G is generator point)
 */
void zb_ecc_tnaf5_fixed_scalarmul(const zb_uint32_t *k, zb_ecc_point_aff_t *Q)
{
    // zb_int8_t a[286] = {0x0};  /*NOTE: previous is 286 = 283 + 3*/
    zb_uint32_t i;
    // zb_uint32_t lambda0[6];  /*NOTE: previous is 5*/
    // zb_uint32_t lambda1[6];  /*NOTE: previous is 5*/
    // zb_ecc_point_aff_t T;
    // zb_ecc_point_pro_t R;
    /* zb_ecc_point_aff_t is aligned by 4 */

    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,38} */
    /* zb_ecc_point_aff_t is aligned by 4 */
    zb_ecc_point_aff_t *T = (zb_ecc_point_aff_t *)shared_buf1;  /*NOTE:Memory of T is enough to store 'zb_ecc_point_pro_t' type*/
    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,39} */
    /* zb_ecc_point_pro_t is aligned by 4 */
    zb_ecc_point_pro_t *R = (zb_ecc_point_pro_t *)(shared_buf1 + 3 * ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS);

    if (sect163k1 == g_curve_id)
    {
        zb_ecc_partial_mod(k, (zb_uint32_t *)T->x, (zb_uint32_t *)T->y);
    }
    else if (sect283k1 == g_curve_id)
    {
        zb_ecc_partial_mod_sect283k1(k, (zb_uint32_t *)T->x, (zb_uint32_t *)T->y);
    }

    ZB_MEMSET(expansion_table, 0, 286); // NOTE: It is necessary step because this variable is used by two functions
    zb_ecc_tnaf5_expansion((zb_uint32_t *)T->x, (zb_uint32_t *)T->y, expansion_table);


    /*R = infinity*/
    XIS1(R->X, g_ecc_pub_key_len_in_dwords);
    XIS0(R->Y, 4 * g_ecc_pub_key_len_in_dwords);
    XIS0(R->Z, 4 * g_ecc_pub_key_len_in_dwords);

    for (i = g_ecc_bitlen + 3; i != 0; i--)
    {
        zb_ecc_point_frobenius(R);
        // if (a[i-1] != 0)
        // printf("a[%d-1] = %d\n", i, a[i-1]);
        switch (expansion_table[i - 1])
        {
        case 0:
            break;
        case 1:
            zb_ecc_mixed_addition(R, &G[g_curve_id], R);

            break;
        case -1:
            zb_ecc_point_negation(&G[g_curve_id], T);
            zb_ecc_mixed_addition(R, T, R);

            break;
        case 3:
            zb_ecc_mixed_addition(R, &G3[g_curve_id], R);

            break;
        case -3:
            zb_ecc_point_negation(&G3[g_curve_id], T);
            zb_ecc_mixed_addition(R, T, R);

            break;
        case 5:
            zb_ecc_mixed_addition(R, &G5[g_curve_id], R);

            break;
        case -5:
            zb_ecc_point_negation(&G5[g_curve_id], T);
            zb_ecc_mixed_addition(R, T, R);

            break;
        case 7:
            zb_ecc_mixed_addition(R, &G7[g_curve_id], R);

            break;
        case -7:
            zb_ecc_point_negation(&G7[g_curve_id], T);
            zb_ecc_mixed_addition(R, T, R);
            break;
        case 9:
            zb_ecc_mixed_addition(R, &G9[g_curve_id], R);
            break;
        case -9:
            zb_ecc_point_negation(&G9[g_curve_id], T);
            zb_ecc_mixed_addition(R, T, R);

            break;
        case 11:
            zb_ecc_mixed_addition(R, &G11[g_curve_id], R);

            break;
        case -11:
            zb_ecc_point_negation(&G11[g_curve_id], T);
            zb_ecc_mixed_addition(R, T, R);

            break;
        case 13:
            zb_ecc_mixed_addition(R, &G13[g_curve_id], R);

            break;
        case -13:
            zb_ecc_point_negation(&G13[g_curve_id], T);
            zb_ecc_mixed_addition(R, T, R);

            break;
        case 15:
            zb_ecc_mixed_addition(R, &G15[g_curve_id], R);

            break;
        case -15:
            zb_ecc_point_negation(&G15[g_curve_id], T);

            zb_ecc_mixed_addition(R, T, R);

            break;
        default:
            return;  /*Error*/
        }
    }
    zb_ecc_project_to_affine(R, Q);
}
#endif


#ifndef ECC_FAST_RANDOM_SCALARMUL
/**
 * Compute (u mod tau^5 )*P point according to TNAF5 algorithm
 * @param P    - base point for computations
 * @param num1 - number from set {+/-1, +/-3, +/-5, +/-7, +/-9, +/-11, +/-13, +/-15}
 * @param Q    - result of computing (u mod tau^5 )*P
 */
static void zb_ecc_compute_point(const zb_ecc_point_aff_t *P, const zb_int8_t num1, zb_ecc_point_aff_t *Q)
{
    /* zb_ecc_point_pro_t is aligned by 4 */
    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,40} */
    zb_ecc_point_pro_t *R = (zb_ecc_point_pro_t *)(shared_buf1 + 6 * ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS);
    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,41} */
    zb_ecc_point_pro_t *S = (zb_ecc_point_pro_t *)(shared_buf1 + 9 * ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS);
    zb_bool_t neg = num1 < 0;
    zb_uint8_t num = (neg) ? -num1 : num1;

    // zb_ecc_affine_to_project(P, &R);
    // zb_ecc_point_frobenius(&R);
    //printf("neg:%d, a[i -1]:%d\n", neg, num);
    // ZB_MEMSET(Q, 0, sizeof(zb_ecc_point_aff_t));
    if (sect163k1 == g_curve_id)
    {
        switch (num)
        {
        case 1:
            if (neg)
            {
                zb_ecc_point_negation(P, Q);
            }
            else
            {
                ZB_MEMCPY(Q, P, 2 * ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);
            }

            break;
        case 3:
            zb_ecc_affine_to_project(P, R);
            zb_ecc_point_frobenius(R);

            zb_ecc_point_negation(P, Q);
            zb_ecc_point_frobenius(R);
            zb_ecc_mixed_addition(R, Q, S);
            zb_ecc_project_to_affine(S, Q);

            if (neg)
            {
                zb_ecc_point_negation(Q, Q);
            }

            break;
        case 5:
            zb_ecc_affine_to_project(P, R);
            zb_ecc_point_frobenius(R);

            zb_ecc_point_negation(P, Q);
            zb_ecc_mixed_addition(R, Q, S);
            zb_ecc_project_to_affine(S, Q);

            if (neg)
            {
                zb_ecc_point_negation(Q, Q);
            }

            break;
        case 7:
            zb_ecc_affine_to_project(P, R);
            zb_ecc_point_frobenius(R);

            zb_ecc_mixed_addition(R, P, S);
            zb_ecc_project_to_affine(S, Q);

            if (neg)
            {
                zb_ecc_point_negation(Q, Q);
            }

            break;
        case 9:
            zb_ecc_affine_to_project(P, R);
            zb_ecc_point_frobenius(R);

            zb_ecc_mixed_addition(R, P, S);
            zb_ecc_project_to_affine(S, Q);

            zb_ecc_point_frobenius(R);
            zb_ecc_point_frobenius(R);
            zb_ecc_point_frobenius(R);

            zb_ecc_mixed_addition(R, Q, S);
            zb_ecc_project_to_affine(S, Q);

            if (!neg)
            {
                zb_ecc_point_negation(Q, Q);
            }

            break;
        case 11:
            zb_ecc_affine_to_project(P, R);
            zb_ecc_point_frobenius(R);

            zb_ecc_point_negation(P, Q);

            zb_ecc_point_frobenius(R);
            zb_ecc_mixed_addition(R, Q, S);
            zb_ecc_project_to_affine(S, Q);

            zb_ecc_point_frobenius(R);
            zb_ecc_point_negation(Q, Q);

            zb_ecc_mixed_addition(R, Q, S);
            zb_ecc_project_to_affine(S, Q);

            if (!neg)
            {
                zb_ecc_point_negation(Q, Q);
            }

            break;
        case 13:
            zb_ecc_affine_to_project(P, R);
            zb_ecc_point_frobenius(R);

            zb_ecc_point_negation(P, Q);
            zb_ecc_mixed_addition(R, Q, S);
            zb_ecc_project_to_affine(S, Q);

            zb_ecc_point_frobenius(R);
            zb_ecc_point_frobenius(R);

            zb_ecc_point_negation(Q, Q);
            zb_ecc_mixed_addition(R, Q, S);
            zb_ecc_project_to_affine(S, Q);

            if (!neg)
            {
                zb_ecc_point_negation(Q, Q);
            }


            break;
        case 15:
            zb_ecc_affine_to_project(P, R);
            zb_ecc_point_frobenius(R);

            zb_ecc_point_negation(P, Q);

            zb_ecc_point_frobenius(R);

            zb_ecc_mixed_addition(R, Q, S);
            zb_ecc_project_to_affine(S, Q);

            zb_ecc_point_frobenius(R);

            zb_ecc_point_negation(Q, Q);
            zb_ecc_mixed_addition(R, Q, S);

            zb_ecc_project_to_affine(S, Q);

            zb_ecc_point_negation(Q, Q);

            zb_ecc_point_frobenius(R);

            zb_ecc_affine_to_project(P, R);
            zb_ecc_point_frobenius(R);
            zb_ecc_mixed_addition(R, Q, S);
            zb_ecc_project_to_affine(S, Q);

            if (!neg)
            {
                zb_ecc_point_negation(Q, Q);
            }

            break;
        default:
            return;

        }
    }

    if (sect283k1 == g_curve_id)
    {
        switch (num)
        {
        case 1:
            if (neg)
            {
                zb_ecc_point_negation(P, Q);
            }
            else
            {
                ZB_MEMCPY(Q, P, 2 * ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);
            }

            break;
        case 3:
            zb_ecc_affine_to_project(P, R);
            zb_ecc_point_frobenius(R);

            zb_ecc_point_negation(P, Q);

            zb_ecc_point_frobenius(R);
            zb_ecc_mixed_addition(R, Q, S);
            zb_ecc_project_to_affine(S, Q);

            if (neg)
            {
                zb_ecc_point_negation(Q, Q);
            }

            break;
        case 5:
            zb_ecc_affine_to_project(P, R);
            zb_ecc_point_frobenius(R);

            zb_ecc_mixed_addition(R, P, S);
            zb_ecc_project_to_affine(S, Q);
            if (!neg)
            {
                zb_ecc_point_negation(Q, Q);
            }

            break;
        case 7:
            zb_ecc_affine_to_project(P, R);
            zb_ecc_point_frobenius(R);

            zb_ecc_point_negation(P, Q);
            zb_ecc_mixed_addition(R, Q, S);
            zb_ecc_project_to_affine(S, Q);
            if (!neg)
            {
                zb_ecc_point_negation(Q, Q);
            }

            break;
        case 9:
            zb_ecc_affine_to_project(P, R);
            zb_ecc_point_frobenius(R);

            zb_ecc_point_negation(P, Q);
            zb_ecc_mixed_addition(R, Q, S);
            zb_ecc_project_to_affine(S, Q);
            zb_ecc_point_negation(Q, Q);

            zb_ecc_point_frobenius(R);
            zb_ecc_point_frobenius(R);
            zb_ecc_point_frobenius(R);

            zb_ecc_mixed_addition(R, Q, S);
            zb_ecc_project_to_affine(S, Q);
            if (!neg)
            {
                zb_ecc_point_negation(Q, Q);
            }

            break;
        case 11:
            zb_ecc_affine_to_project(P, R);
            zb_ecc_point_frobenius(R);

            zb_ecc_point_negation(P, Q);

            zb_ecc_point_frobenius(R);

            zb_ecc_mixed_addition(R, Q, S);
            zb_ecc_project_to_affine(S, Q);

            zb_ecc_point_frobenius(R);

            zb_ecc_mixed_addition(R, Q, S);
            zb_ecc_project_to_affine(S, Q);
            if (neg)
            {
                zb_ecc_point_negation(Q, Q);
            }

            break;
        case 13:
            zb_ecc_affine_to_project(P, R);
            zb_ecc_point_frobenius(R);

            zb_ecc_mixed_addition(R, P, S);
            zb_ecc_project_to_affine(S, Q);
            zb_ecc_point_negation(Q, Q);

            zb_ecc_point_frobenius(R);

            zb_ecc_point_frobenius(R);

            zb_ecc_mixed_addition(R, Q, S);
            zb_ecc_project_to_affine(S, Q);

            if (neg)
            {
                zb_ecc_point_negation(Q, Q);
            }

            break;
        case 15:
            zb_ecc_affine_to_project(P, R);
            zb_ecc_point_frobenius(R);

            zb_ecc_point_negation(P, Q);

            zb_ecc_point_frobenius(R);

            zb_ecc_mixed_addition(R, Q, S);
            zb_ecc_project_to_affine(S, Q);

            zb_ecc_point_frobenius(R);

            zb_ecc_mixed_addition(R, Q, S);
            zb_ecc_project_to_affine(S, Q);

            zb_ecc_point_frobenius(R);

            zb_ecc_affine_to_project(P, R);

            zb_ecc_point_frobenius(R);

            zb_ecc_point_negation(Q, Q);
            zb_ecc_mixed_addition(R, Q, S);
            zb_ecc_project_to_affine(S, Q);
            if (neg)
            {
                zb_ecc_point_negation(Q, Q);
            }

            break;
        default:
            return;
        }
    }
    else
    {
        return;
    }
}
#endif


//FIXME: may be better to union follow variables via structure
static zb_uint16_t g_tnaf5_iter;
static zb_uint32_t *g_tnaf5_prv_key;
static zb_ecc_point_aff_t *tnaf5_P;
static zb_ecc_point_aff_t *tnaf5_res;
static zb_callback_t g_tnaf5_cb;

/*cstat !MISRAC2012-Rule-11.3 */
/** @mdr{00002,42} */
/* zb_ecc_point_pro_t is aligned by 4 */
static zb_ecc_point_pro_t *tnaf5_R = (zb_ecc_point_pro_t *)(shared_buf1 + 3 * ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS);
#ifdef ECC_FAST_RANDOM_SCALARMUL

/* zb_ecc_point_aff_t is aligned by 4 */
/*cstat !MISRAC2012-Rule-11.3 */
/** @mdr{00002,43} */
static zb_ecc_point_aff_t *tnaf5_P3 = (zb_ecc_point_aff_t *)(shared_buf1 + 6 * ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS);
/*cstat !MISRAC2012-Rule-11.3 */
/** @mdr{00002,44} */
static zb_ecc_point_aff_t *tnaf5_P5 = (zb_ecc_point_aff_t *)(shared_buf1 + 8 * ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS);
/*cstat !MISRAC2012-Rule-11.3 */
/** @mdr{00002,45} */
static zb_ecc_point_aff_t *tnaf5_P7 = (zb_ecc_point_aff_t *)(shared_buf1 + 10 * ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS);
/*cstat !MISRAC2012-Rule-11.3 */
/** @mdr{00002,46} */
static zb_ecc_point_aff_t *tnaf5_P9 = (zb_ecc_point_aff_t *)(shared_buf1 + 12 * ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS);
/*cstat !MISRAC2012-Rule-11.3 */
/** @mdr{00002,47} */
static zb_ecc_point_aff_t *tnaf5_P11 = (zb_ecc_point_aff_t *)(shared_buf1 + 14 * ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS);
/*cstat !MISRAC2012-Rule-11.3 */
/** @mdr{00002,48} */
static zb_ecc_point_aff_t *tnaf5_P13 = (zb_ecc_point_aff_t *)(shared_buf1 + 16 * ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS);
/*cstat !MISRAC2012-Rule-11.3 */
/** @mdr{00002,49} */
static zb_ecc_point_aff_t *tnaf5_P15 = (zb_ecc_point_aff_t *)(shared_buf1 + 18 * ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS);

/**
 * [zb_ecc_points_precomputation description]
 * @param P        [description]
 */
static void zb_ecc_points_precomputation(const zb_ecc_point_aff_t *P)
{
    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,50} */
    /* DD:Pointers doesn't change during execution */
    /* zb_ecc_point_pro_t is aligned by 4 */
    static zb_ecc_point_pro_t *S = (zb_ecc_point_pro_t *)shared_buf1;

    /*NOTE: move variables to global scope*/
    // zb_ecc_point_pro_t R;
    // zb_ecc_point_pro_t S;
    if (sect163k1 == g_curve_id)
    {
        //pre-computation
        zb_ecc_affine_to_project(P, tnaf5_R);
        zb_ecc_point_frobenius(tnaf5_R);
        zb_ecc_point_negation(P, tnaf5_P3);
        zb_ecc_mixed_addition(tnaf5_R, tnaf5_P3, S);
        zb_ecc_project_to_affine(S, tnaf5_P5);
        zb_ecc_mixed_addition(tnaf5_R, P, S);
        zb_ecc_project_to_affine(S, tnaf5_P7);
        zb_ecc_point_frobenius(tnaf5_R);
        zb_ecc_mixed_addition(tnaf5_R, tnaf5_P3, S);
        zb_ecc_project_to_affine(S, tnaf5_P3);
        zb_ecc_point_frobenius(tnaf5_R);
        zb_ecc_point_negation(tnaf5_P3, tnaf5_P11);
        zb_ecc_mixed_addition(tnaf5_R, tnaf5_P11, S);
        zb_ecc_project_to_affine(S, tnaf5_P11);
        zb_ecc_point_negation(tnaf5_P11, tnaf5_P11);
        zb_ecc_point_negation(tnaf5_P5, tnaf5_P13);
        zb_ecc_mixed_addition(tnaf5_R, tnaf5_P13, S);
        zb_ecc_project_to_affine(S, tnaf5_P13);
        zb_ecc_point_negation(tnaf5_P13, tnaf5_P13);
        zb_ecc_point_frobenius(tnaf5_R);
        zb_ecc_mixed_addition(tnaf5_R, tnaf5_P7, S);
        zb_ecc_project_to_affine(S, tnaf5_P9);
        zb_ecc_point_negation(tnaf5_P9, tnaf5_P9);
        zb_ecc_affine_to_project(P, tnaf5_R);
        zb_ecc_point_frobenius(tnaf5_R);
        zb_ecc_mixed_addition(tnaf5_R, tnaf5_P11, S);
        zb_ecc_project_to_affine(S, tnaf5_P15);
        zb_ecc_point_negation(tnaf5_P15, tnaf5_P15);
    }
    else
    {
        ZB_ASSERT(sect283k1 == g_curve_id);
        //pre-computation
        zb_ecc_affine_to_project(P, tnaf5_R);
        zb_ecc_point_frobenius(tnaf5_R);
        zb_ecc_mixed_addition(tnaf5_R, P, S);
        zb_ecc_project_to_affine(S, tnaf5_P5);
        zb_ecc_point_negation(tnaf5_P5, tnaf5_P5);
        zb_ecc_point_negation(P, tnaf5_P3);
        zb_ecc_mixed_addition(tnaf5_R, tnaf5_P3, S);
        zb_ecc_project_to_affine(S, tnaf5_P7);
        zb_ecc_point_negation(tnaf5_P7, tnaf5_P7);
        zb_ecc_point_frobenius(tnaf5_R);
        zb_ecc_mixed_addition(tnaf5_R, tnaf5_P3, S);
        zb_ecc_project_to_affine(S, tnaf5_P3);
        zb_ecc_point_frobenius(tnaf5_R);
        zb_ecc_mixed_addition(tnaf5_R, tnaf5_P3, S);
        zb_ecc_project_to_affine(S, tnaf5_P11);
        zb_ecc_mixed_addition(tnaf5_R, tnaf5_P5, S);
        zb_ecc_project_to_affine(S, tnaf5_P13);
        zb_ecc_point_frobenius(tnaf5_R);
        zb_ecc_mixed_addition(tnaf5_R, tnaf5_P7, S);
        zb_ecc_project_to_affine(S, tnaf5_P9);
        zb_ecc_point_negation(tnaf5_P9, tnaf5_P9);
        zb_ecc_affine_to_project(P, tnaf5_R);
        zb_ecc_point_frobenius(tnaf5_R);
        zb_ecc_point_negation(tnaf5_P11, tnaf5_P15);
        zb_ecc_mixed_addition(tnaf5_R, tnaf5_P15, S);
        zb_ecc_project_to_affine(S, tnaf5_P15);

        // //pre-computation
        // zb_ecc_affine_to_project(P, &R);
        // zb_ecc_point_frobenius(&R);
        // zb_ecc_mixed_addition(&R, P, &S);
        // zb_ecc_project_to_affine(&S, &P5);
        // zb_ecc_point_negation(&P5, &P5);
        //
        // zb_ecc_point_compression(&P5, P5x);
        // zb_ecc_point_negation(P, &P3);
        // zb_ecc_mixed_addition(&R, &P3, &S);
        // zb_ecc_project_to_affine(&S, &P7);
        // zb_ecc_point_negation(&P7, &P7);
        //     zb_ecc_point_compression(&P7, P7x);
        // zb_ecc_point_frobenius(&R);
        // zb_ecc_mixed_addition(&R, &P3, &S);
        // zb_ecc_project_to_affine(&S, &P3);
        //
        // zb_ecc_point_compression(&P3, P3x);
        //
        // zb_ecc_point_frobenius(&R);
        // zb_ecc_mixed_addition(&R, &P3, &S);
        // zb_ecc_project_to_affine(&S, &P11);
        //     zb_ecc_point_compression(&P11, P11x);
        // zb_ecc_mixed_addition(&R, &P5, &S);
        // zb_ecc_project_to_affine(&S, &P13);
        //     zb_ecc_point_compression(&P13, P13x);
        // zb_ecc_point_frobenius(&R);
        // zb_ecc_mixed_addition(&R, &P7, &S);
        // zb_ecc_project_to_affine(&S, &P9);
        // zb_ecc_point_negation(&P9, &P9);
        //     zb_ecc_point_compression(&P9, P9x);
        // zb_ecc_affine_to_project(P, &R);
        // zb_ecc_point_frobenius(&R);
        // zb_ecc_point_negation(&P11,&P15);
        // zb_ecc_mixed_addition(&R, &P15, &S);
        // zb_ecc_project_to_affine(&S, &P15);
        //     zb_ecc_point_compression(&P15, P15x);
    }
}
#else
zb_int8_t g_tnaf5_last = 0;
#endif


/**
 * Scalar multiplication with the width-5 TNAF (random point)
 * @param k        - multiplication constant
 * @param P        - multiplication elliptic curve point in affine coordiate
 * @param Q        - pointer to elliptic curve point in affine coordinate. Result of k * P multiplication.
 */
/*FIXME: need to make this function as inline!*/
static void zb_ecc_tnaf5_random_scalarmul_init(zb_uint32_t *k, zb_ecc_point_aff_t *P, zb_ecc_point_aff_t *Q)
{
    /*FIXME: need to store Q variable*/
    tnaf5_res = Q;
    g_tnaf5_iter = g_ecc_bitlen + 3U;
    tnaf5_P = P;
    g_tnaf5_prv_key = k;
}

void zb_ecc_tnaf5_random_scalarmul_iter(zb_uint8_t async_mode);

static void zb_ecc_tnaf5_random_scalarmul_start(zb_uint8_t async_mode)
{
    // ZVUNUSED(param);
    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,51} */
    /* zb_ecc_point_aff_t is aligned by 4 */
    static zb_ecc_point_aff_t *T = (zb_ecc_point_aff_t *)shared_buf1;  /*NOTE:Memory of T is enough to store 'zb_ecc_point_pro_t' type*/
#ifdef ECC_FAST_RANDOM_SCALARMUL
    /*NOTE:Be careful: this function and zb_ecc_points_precomputation function use
      the same buffers for T, R, P3-P15 variables*/
    zb_ecc_points_precomputation(tnaf5_P);
#endif

    if (sect163k1 == g_curve_id)
    {
        zb_ecc_partial_mod(g_tnaf5_prv_key, (zb_uint32_t *)T->x, (zb_uint32_t *)T->y);
    }
    else
    {
        ZB_ASSERT(sect283k1 == g_curve_id);
        zb_ecc_partial_mod_sect283k1(g_tnaf5_prv_key, (zb_uint32_t *)T->x, (zb_uint32_t *)T->y);
    }

    // zb_ecc_partial_mod(k, lambda0, lambda1);
    /*tau-adic expansion*/
    // zb_ecc_partial_mod(k, lambda0, lambda1);
    /*dump_bytes((zb_uint8_t *)lambda0, 6*4, "lambda0");
    dump_bytes((zb_uint8_t *)lambda1, 6*4, "lambda1");*/
    ZB_MEMSET(expansion_table, 0, 286); // NOTE: It is necessary step because this variable is used by two functions
    zb_ecc_tnaf5_expansion((zb_uint32_t *)T->x, (zb_uint32_t *)T->y, expansion_table);

    XIS1(tnaf5_R->X, (zb_uint32_t)(g_ecc_pub_key_len_in_dwords));
    XIS0(tnaf5_R->Y, 4U * (zb_uint32_t)g_ecc_pub_key_len_in_dwords);
    XIS0(tnaf5_R->Z, 4U * (zb_uint32_t)g_ecc_pub_key_len_in_dwords);

#ifdef ECC_SYNC_MODE_ENABLED
    /*cstat !MISRAC2012-Rule-14.3_* */
    /* C-STAT gives a warning that following condition is always 'false'.
     * zb_ecc_tnaf5_random_scalarmul_start() is actually called with both parameters ASYNC_ON and ASYNC_OFF.
     * - It is called directly from zb_ecc_tnaf5_random_scalarmul() with parameter ASYNC_OFF.
     * - It is called indirectly from zb_ecc_tnaf5_random_scalarmul_async() using ZB_SCHEDULE_CALLBACK()
     *   with parameter ASYNC_ON.
     */
    if (ASYNC_ON == async_mode)
#endif
    {
        ZB_SCHEDULE_CALLBACK(zb_ecc_tnaf5_random_scalarmul_iter, async_mode);
    }
}


void zb_ecc_tnaf5_random_scalarmul_iter(zb_uint8_t async_mode)
{
    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,52} */
    /* zb_ecc_point_aff_t is aligned by 4 */
    static zb_ecc_point_aff_t *s_T = (zb_ecc_point_aff_t *)shared_buf1;  /*NOTE:Memory of T is enough to store 'zb_ecc_point_pro_t' type*/
    // ZVUNUSED(param);
    /*tnaf5_R = infinity*/
    // for (g_tnaf5_iter = g_ecc_bitlen + 3; g_tnaf5_iter != 0; g_tnaf5_iter--)
    if (g_tnaf5_iter > 0U)
    {
        /*FIXME:Use Q instead of T*/
        /*NOTE and FIXME: we can use Q insteal of T*/
        zb_ecc_point_frobenius(tnaf5_R);

#ifndef ECC_FAST_RANDOM_SCALARMUL
        if (expansion_table[g_tnaf5_iter - 1] != 0)
        {
            // printf("i:%d, a[g_tnaf5_iter -1]:%d\n", i, a[g_tnaf5_iter - 1]);
            if (g_tnaf5_last == -expansion_table[g_tnaf5_iter - 1])
            {
                zb_ecc_point_negation(s_T, s_T);
            }
            else if (g_tnaf5_last != expansion_table[g_tnaf5_iter - 1])
            {
                zb_ecc_compute_point(tnaf5_P, expansion_table[g_tnaf5_iter - 1], s_T);
            }

            zb_ecc_mixed_addition(tnaf5_R, s_T, tnaf5_R);
            g_tnaf5_last = expansion_table[g_tnaf5_iter - 1];
        }
#else
        switch (expansion_table[g_tnaf5_iter - 1U])
            /*cstat !MISRAC2012-Rule-16.1*/
            /* Rule-16.1 The keyword return has an equivalent behaviour as the break */
        {
        case 0:
            break;
        case 1:
            zb_ecc_mixed_addition(tnaf5_R, tnaf5_P, tnaf5_R);

            break;
        case -1:
            zb_ecc_point_negation(tnaf5_P, s_T);
            zb_ecc_mixed_addition(tnaf5_R, s_T, tnaf5_R);

            break;
        case 3:
            // zb_ecc_point_decompression(g_curve_id, P3x, &s_T);

            zb_ecc_mixed_addition(tnaf5_R, tnaf5_P3, tnaf5_R);

            break;
        case -3:
            // zb_ecc_point_decompression(g_curve_id, P3x, &s_T);
            zb_ecc_point_negation(tnaf5_P3, s_T);
            zb_ecc_mixed_addition(tnaf5_R, s_T, tnaf5_R);

            break;
        case 5:
            // zb_ecc_point_decompression(g_curve_id, P5x, &s_T);
            zb_ecc_mixed_addition(tnaf5_R, tnaf5_P5, tnaf5_R);

            break;
        case -5:
            // zb_ecc_point_decompression(g_curve_id, P5x, &s_T);
            zb_ecc_point_negation(tnaf5_P5, s_T);
            zb_ecc_mixed_addition(tnaf5_R, s_T, tnaf5_R);

            break;
        case 7:
            // zb_ecc_point_decompression(g_curve_id, P7x, &s_T);
            zb_ecc_mixed_addition(tnaf5_R, tnaf5_P7, tnaf5_R);

            break;
        case -7:
            // zb_ecc_point_decompression(g_curve_id, P7x, &s_T);
            zb_ecc_point_negation(tnaf5_P7, s_T);
            zb_ecc_mixed_addition(tnaf5_R, s_T, tnaf5_R);

            break;
        case 9:
            // zb_ecc_point_decompression(g_curve_id, P9x, &s_T);
            zb_ecc_mixed_addition(tnaf5_R, tnaf5_P9, tnaf5_R);

            break;
        case -9:
            // zb_ecc_point_decompression(g_curve_id, P9x, &s_T);
            zb_ecc_point_negation(tnaf5_P9, s_T);
            zb_ecc_mixed_addition(tnaf5_R, s_T, tnaf5_R);

            break;
        case 11:
            // zb_ecc_point_decompression(g_curve_id, P11x, &s_T);
            zb_ecc_mixed_addition(tnaf5_R, tnaf5_P11, tnaf5_R);

            break;
        case -11:
            // zb_ecc_point_decompression(g_curve_id, P11x, &s_T);
            zb_ecc_point_negation(tnaf5_P11, s_T);
            zb_ecc_mixed_addition(tnaf5_R, s_T, tnaf5_R);

            break;
        case 13:
            // zb_ecc_point_decompression(g_curve_id, P13x, &s_T);
            zb_ecc_mixed_addition(tnaf5_R, tnaf5_P13, tnaf5_R);

            break;
        case -13:
            // zb_ecc_point_decompression(g_curve_id, P13x, &s_T);
            zb_ecc_point_negation(tnaf5_P13, s_T);
            zb_ecc_mixed_addition(tnaf5_R, s_T, tnaf5_R);

            break;
        case 15:
            // zb_ecc_point_decompression(g_curve_id, P15x, &s_T);
            zb_ecc_mixed_addition(tnaf5_R, tnaf5_P15, tnaf5_R);

            break;
        case -15:
            // zb_ecc_point_decompression(g_curve_id, P15x, &s_T);
            zb_ecc_point_negation(tnaf5_P15, s_T);
            zb_ecc_mixed_addition(tnaf5_R, s_T, tnaf5_R);

            break;

        /*cstat !MISRAC2012-Rule-16.3*/
        /* Rule-16.3 The keyword return has an equivalent behaviour as the break */
        default:
            return;  //Error
        }
#endif
        g_tnaf5_iter--;

#ifdef ECC_SYNC_MODE_ENABLED
        if (ASYNC_ON == async_mode)
#endif
        {
            ZB_SCHEDULE_CALLBACK(zb_ecc_tnaf5_random_scalarmul_iter, ASYNC_ON);
        }
    }
    else
    {
        zb_ecc_project_to_affine(tnaf5_R, tnaf5_res);
#ifdef ECC_SYNC_MODE_ENABLED
        if (ASYNC_ON == async_mode)
#endif
        {
            ZB_SCHEDULE_CALLBACK(g_tnaf5_cb, ASYNC_ON);
        }
    }
}


#ifdef ECC_SYNC_MODE_ENABLED
static void zb_ecc_tnaf5_random_scalarmul(zb_uint32_t *k, zb_ecc_point_aff_t *P, zb_ecc_point_aff_t *Q)
{
    zb_ecc_tnaf5_random_scalarmul_init(k, P, Q);
    zb_ecc_tnaf5_random_scalarmul_start(ASYNC_OFF);

    while (g_tnaf5_iter > 0U)
    {
        // zb_ecc_point_frobenius(tnaf5_R);
        zb_ecc_tnaf5_random_scalarmul_iter(ASYNC_OFF);
    }

    zb_ecc_tnaf5_random_scalarmul_iter(ASYNC_OFF); //last call
}
#endif


static void zb_ecc_tnaf5_random_scalarmul_async(zb_callback_t func, zb_uint32_t *k, zb_ecc_point_aff_t *P, zb_ecc_point_aff_t *Q)
{

    g_tnaf5_cb = func;

    zb_ecc_tnaf5_random_scalarmul_init(k, P, Q);

    ZB_SCHEDULE_CALLBACK(zb_ecc_tnaf5_random_scalarmul_start, ASYNC_ON);
}


/**
 * Public key validation
 * @param  Q        - public key in affine coordinates
 * @return ZB_PK_VALID if public key is valid, and ZB_PK_INVALID if public key is invalid
 */
/*FIXME: create public key validation function for compressed form in original order*/
zb_uint8_t zb_ecc_pk_validation(const zb_ecc_point_aff_t *Q)
{
    // zb_uint32_t S[ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS];
    zb_uint32_t *S = shared_buf1;
    // zb_uint32_t T[ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS];
    zb_uint32_t *T = shared_buf1 + ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS;

    if (sect163k1 == g_curve_id)
    {
        /*Q is infinity*/
        if (IF0_163(Q->x) && IF0_163(Q->y))
        {
            return ZB_PK_INVALID;
        }

        /*Check the degree of x_Q and y_Q*/
        if (((Q->x[5] >> 3) != 0x00U) || ((Q->y[5] >> 3) != 0x00U))
        {
            return ZB_PK_INVALID;
        }
    }
    else if (sect283k1 == g_curve_id)
    {
        /*Q is infinity*/
        if (IF0_283(Q->x) && IF0_283(Q->y))
        {
            return ZB_PK_INVALID;
        }

        /*Check the degree of x_Q and y_Q*/
        if (((Q->x[8] >> 27) != 0x00U) || ((Q->y[8] >> 27) != 0x00U))
        {
            return ZB_PK_INVALID;
        }
    }
    else
    {
        return ZB_PK_INVALID;
    }

    /*Compute x_Q^3 + 1*/
    zb_ecc_modsq(Q->x, S);
    zb_ecc_modmul(Q->x, S, T);
    if (g_curve_id == sect163k1)
    {
        zb_ecc_modadd(T, S, T);
    }

    T[0] = T[0] ^ 0x01U;

    /*Compute y_Q^2 + x_Qy_Q*/
    zb_ecc_modsq(Q->y, S);
    zb_ecc_modadd(T, S, T);
    zb_ecc_modmul(Q->x, Q->y, S);
    zb_ecc_modadd(T, S, T);
    if (sect163k1 == g_curve_id)
    {
        if (IFN0_163(T))
        {
            return ZB_PK_INVALID;
        }

        /*Check that 2Q is not infinity*/
        if (IF0_163(Q->x))  /*Q is a 2-torsion point*/
        {
            return ZB_PK_INVALID;
        }
    }
    else if (sect283k1 == g_curve_id)
    {
        if (IFN0_283(T))
        {
            return ZB_PK_INVALID;
        }

        /*Check that 2Q or 4Q is not infinity*/
        /*FIXME*/
        if ((IF0_283(Q->x)) || (IF1_283(Q->x)))  /*Q is a 2- or 4-torsion point*/
        {
            return ZB_PK_INVALID;
        }
    }
    else
    {
        return ZB_PK_INVALID;
    }

    return ZB_PK_VALID;
}


/**
 * The initial step of key generation procedure
 * @param rnd     - rnd  a random number (21-byte for sect163k1 and 36-byte for sect283k1)
 * @param prv_key - generated private key
 * @param pub_key - generated public
 */
static void zb_ecc_key_generation_start(const zb_uint8_t *rnd, zb_uint8_t *prv_key, zb_uint8_t *pub_key)
{

    g_last_error = RET_OK; /*Set new value*/

    // zb_uint32_t *c = (zb_uint32_t *)shared_buf;
    // zb_ecc_point_aff_t *Q = (zb_ecc_point_aff_t *)(shared_buf + 2 * ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES); //FIXME: why not 1 * ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES?

    // zb_uint8_t Qx[ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES];
    g_ctx.u32_pub = shared_buf; //NOTE: length is 1 * ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES
    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,53} */
    /* zb_ecc_point_aff_t is aligned by 4 */
    g_ctx.aff_point = (zb_ecc_point_aff_t *)(shared_buf + ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS);
    g_ctx.ret_p = pub_key;

    ZB_MEMSET((zb_uint8_t *)g_ctx.u32_pub, 0, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);
    if (sect163k1 == g_curve_id)
    {
        ZB_MEMCPY((zb_uint8_t *)g_ctx.u32_pub, rnd, g_ecc_prv_key_len);
        zb_ecc_inverse_bytes((zb_uint8_t *)g_ctx.u32_pub, g_ecc_prv_key_len);
        *(g_ctx.u32_pub + 5) = *(g_ctx.u32_pub + 5) & 0x07U;

        // *(c + 5) = (zb_uint32_t)(*(rnd + 20)) & 0x7;
    }
    else if (sect283k1 == g_curve_id)
    {
        ZB_MEMCPY((zb_uint8_t *)g_ctx.u32_pub, rnd, g_ecc_prv_key_len);
        zb_ecc_inverse_bytes((zb_uint8_t *)g_ctx.u32_pub, g_ecc_prv_key_len);
        *(g_ctx.u32_pub + 8) = *(g_ctx.u32_pub + 8) & 0x7FFFFFFU;

        //*(c + 8) = (zb_uint32_t)(*(rnd + 32)) | ((zb_uint32_t)(*(rnd + 33)) << 8) | ((zb_uint32_t)(*(rnd + 34)) << 16) | (((zb_uint32_t)(*(rnd + 35)) & 0x7 ) << 24);
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "g_curve_id can only be equal to sect163k1 or sect283k1", (FMT__0));
        ZB_ASSERT(0);
    }

    if (zb_ecc_compare(g_ctx.u32_pub, eccn[g_curve_id], g_ecc_pub_key_len_in_dwords) != ZB_BI_LESS)
    {
        (void)zb_ecc_sub(g_ctx.u32_pub, eccn[g_curve_id], g_ecc_pub_key_len_in_dwords, g_ctx.u32_pub);
    }

    ZB_MEMSET(g_ctx.aff_point, 0, sizeof(zb_ecc_point_aff_t));

    //XTOY(key->d, c, g_ecc_pub_key_len_in_dwords);
    ZB_MEMCPY(prv_key, (zb_uint8_t *)g_ctx.u32_pub, g_ecc_prv_key_len);
    zb_ecc_inverse_bytes(prv_key, g_ecc_prv_key_len);
}


/**
 * The last step of key generation procedure
 * @param  async_mode - ASYNC_ON or ASYNC_OFF. Enables/disables async mode.
 */
static void zb_ecc_key_generation_finish(zb_uint8_t async_mode)
{
    // dump_bytes((zb_uint8_t *)Q.x, 40, "Q.x:");
    /*NOTE: may be we need to create special type for compressed public key*/

    zb_ecc_point_compression(g_ctx.aff_point, g_ctx.u32_pub);
    ZB_MEMCPY(g_ctx.ret_p, (zb_uint8_t *)g_ctx.u32_pub, g_ecc_pub_key_len);
    zb_ecc_inverse_bytes(g_ctx.ret_p, g_ecc_pub_key_len);

    /*cstat !MISRAC2012-Rule-14.3_* */
    /* C-STAT gives a warning that following condition is always 'false'.
     * zb_ecc_key_generation_finish() is actually called with both parameters ASYNC_ON and ASYNC_OFF.
     * - It is called directly from zb_ecc_key_generation() with parameter ASYNC_OFF.
     * - It is called indirectly during asynchronous execution of zb_ecc_key_generation_async()
     *   through 'g_tnaf5_cb' function pointer with parameter ASYNC_ON.
     */
    if (ASYNC_ON == async_mode)
    {
        ZB_SCHEDULE_CALLBACK(g_ctx.async_cb, g_ctx.async_cb_param);
    }
}


#ifdef ECC_SYNC_MODE_ENABLED
/**
* Key pair generation procedure
* @param rnd     - rnd  a random number (21-byte for sect163k1 and 36-byte for sect283k1)
* @param prv_key - generated private key
* @param pub_key - generated public key
*/
void zb_ecc_key_generation(const zb_uint8_t *rnd, zb_uint8_t *prv_key, zb_uint8_t *pub_key)
{
    zb_ecc_key_generation_start(rnd, prv_key, pub_key);

#ifdef ECC_ENABLE_FIXED_SCALARMUL
    zb_ecc_tnaf5_fixed_scalarmul(g_ctx.u32_pub, g_ctx.aff_point);
#else
    zb_ecc_tnaf5_random_scalarmul(g_ctx.u32_pub, &G[g_curve_id], g_ctx.aff_point);
    // zb_ecc_tnaf5_random_scalarmul(c, &G[g_curve_id], Q);
#endif
    zb_ecc_key_generation_finish(ASYNC_OFF);  //run function in synchronous mode
}
#endif


/**
 * Key pair generation procedure. Asymc mode.
 * @param async_cb       - callback which will be executed after generation procedure.
 * @param async_cb_param - parameter which will be passed to @async_cb function. It is unchanged during execution
 * @param rnd            - rnd  a random number (21-byte for sect163k1 and 36-byte for sect283k1)
 * @param prv_key        - generated private key
 * @param pub_key        - generated public key
 */
void zb_ecc_key_generation_async(zb_callback_t async_cb, const zb_uint8_t async_cb_param, const zb_uint8_t *rnd, zb_uint8_t *prv_key, zb_uint8_t *pub_key)
{
    // zb_uint32_t c[ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS];
    g_ctx.async_cb = async_cb;
    g_ctx.async_cb_param = async_cb_param;

    zb_ecc_key_generation_start(rnd, prv_key, pub_key);

#ifdef ECC_ENABLE_FIXED_SCALARMUL
    zb_ecc_tnaf5_fixed_scalarmul(c, Q);
#else
    zb_ecc_tnaf5_random_scalarmul_async(zb_ecc_key_generation_finish, g_ctx.u32_pub, &G[g_curve_id], g_ctx.aff_point);
    // zb_ecc_tnaf5_random_scalarmul(c, &G[g_curve_id], Q);
#endif
}


void zb_ecc_ecqv_extraction_finish(zb_uint8_t async_mode);


/**
 * Store input parameters to context.
 *
 * @param QCAx    - the CA's public key in compressed form
 * @param pub_rec - reconstruction public key in original form
 * @param CertA   - an implicit certificate for user idA
 * @param QAx     - a user A's public key in compressed form
 */
static void zb_ecc_ecqv_extraction_init(zb_uint8_t *QCAx, zb_uint8_t *pub_rec, zb_uint8_t *CertA, zb_uint8_t *QAx)
{
    g_last_error = RET_OK;  /*Set default value*/

    g_ctx.u32_pub = shared_buf; //NOTE: length is 1 * ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES
    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,54} */
    /* zb_ecc_point_aff_t is aligned by 4 */
    g_ctx.aff_point = (zb_ecc_point_aff_t *)(shared_buf + 1U * ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS);  //NOTE: length is 2 * ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES
    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,55} */
    /* zb_ecc_point_pro_t is aligned by 4 */
    g_ctx.pro_point = (zb_ecc_point_pro_t *)(shared_buf + 3U * ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS);  //NOTE: length is 3 * ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES
    g_ctx.u8p = QCAx;  //FIXME: for storing CA's public key (or pointer to CA's public key), we can use g_ctx.pro_point, because it unused before finish function
    g_ctx.u8p1 = pub_rec;
    g_ctx.u8p2 = CertA;
    g_ctx.ret_p = QAx;
}


#define PUB_REC_KEY g_ctx.u8p1
#define CERT g_ctx.u8p2
/**
 * The initial step of ECQV scheme.
 * @param  async_mode - ASYNC_ON or ASYNC_OFF. Enables/disables async mode.
 */
static void zb_ecc_ecqv_extraction_start(zb_uint8_t async_mode)
{
    /*FIXME: set correct length of variable 's'*/
    /*NOTE:TNAF5_random_scalarmul function use more then 16 bytes of s.
      In some cases this cause invalid computation of QAx value
      (if value of ((uint32_t *)s + 4) or ((uint32_t *)s + 5) is not a zero).*/
    /*NOTE: s must be 40 bytes always(it must be sufficiently large for store
      compressed public key for both curves)*/
    /*XXX:Also we can use QAx input variable to store data instead of s*/
    // zb_uint8_t s[ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES];

    // static zb_ecc_point_aff_t *Q = (zb_ecc_point_aff_t *)(shared_buf + 2 * ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);
    // static zb_ecc_point_pro_t *P = (zb_ecc_point_pro_t *)(shared_buf + 4 * ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);

    /*zb_uint32_t e[N], BAx[N];*/
    // zb_uint32_t BAx[ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS];
    // zb_ecc_point_aff_t BA;
    // zb_ecc_point_aff_t QCA;
    // zb_ecc_point_aff_t QA;
    // zb_ecc_point_pro_t P;
    // zb_uint8_t buf[ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES];

    /*FIXME:Do we need to define separate constants for the follow values?*/
    // if ((*CertA != 0x2) && (*CertA != 0x3))
    // {
    //   return RET_ERROR;
    // }

    /*Convert BAS to a public key BA*/
    // ZB_MEMSET(BAx, 0, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);
    // ZB_MEMSET(&BA, 0xff, sizeof(BA));

    zb_uint8_t i;
    /*NOTE: max used length of g_ctx.u32_pub is ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES bytes*/
    ZB_MEMSET((zb_uint8_t *)g_ctx.u32_pub, 0x00, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES); /*NOTE: it is necessary step*/

    if (sect163k1 == g_curve_id)
    {
        *(g_ctx.u32_pub + 5) = ((zb_uint32_t) * PUB_REC_KEY << 8) | ((zb_uint32_t) * (PUB_REC_KEY + 1));
        for (i = 0; i < 5U; i++)
        {
            *(g_ctx.u32_pub + (4U - i)) = ((zb_uint32_t) * (PUB_REC_KEY + 4U * i + 2U) << 24)
                                          | ((zb_uint32_t) * (PUB_REC_KEY + 4U * i + 3U) << 16)
                                          | ((zb_uint32_t) * (PUB_REC_KEY + 4U * i + 4U) << 8)
                                          | ((zb_uint32_t) * (PUB_REC_KEY + 4U * i + 5U));
        }

        if (RET_ERROR == zb_ecc_point_decompression(g_ctx.u32_pub, g_ctx.aff_point))
        {
            g_last_error = RET_ERROR;
#ifdef ECC_SYNC_MODE_ENABLED
            /*cstat !MISRAC2012-Rule-14.3_* */
            /* C-STAT gives a warning that following condition is always 'false'.
             * zb_ecc_ecqv_extraction_start() is actually called with both parameters ASYNC_ON and ASYNC_OFF.
             * - It is called directly from zb_ecc_ecqv_extraction() with parameter ASYNC_OFF.
             * - It is called indirectly from zb_ecc_ecqv_extraction_async() using ZB_SCHEDULE_CALLBACK()
             *   with parameter ASYNC_ON.
             */
            if (ASYNC_ON == async_mode)
#endif
            {
                ZB_SCHEDULE_CALLBACK(g_ctx.async_cb, g_ctx.async_cb_param);
            }
            return;  //interrupt function
        }

        ZB_MEMSET((zb_uint8_t *)g_ctx.u32_pub, 0, g_ecc_pub_key_len);  /*NOTE: it is necessary*/
        /*Compute hash value of the certificate*/
        (void)zb_sec_b6_hash(CERT, ZB_ECC_SECT163_CERT_LEN, (zb_uint8_t *)g_ctx.u32_pub);  /*FIXME:functions return a bool values*/
    }
    else
    {
        ZB_ASSERT(sect283k1 == g_curve_id);
        ZB_MEMCPY((zb_uint8_t *)g_ctx.u32_pub, PUB_REC_KEY, 37);
        zb_ecc_inverse_bytes((zb_uint8_t *)g_ctx.u32_pub, 37);

        if (RET_ERROR == zb_ecc_point_decompression(g_ctx.u32_pub, g_ctx.aff_point))
        {
            g_last_error = RET_ERROR;
#ifdef ECC_SYNC_MODE_ENABLED
            /*cstat !MISRAC2012-Rule-14.3_* */
            /* C-STAT gives a warning that following condition is always 'false'.
             * zb_ecc_ecqv_extraction_start() is actually called with both parameters ASYNC_ON and ASYNC_OFF.
             * - It is called directly from zb_ecc_ecqv_extraction() with parameter ASYNC_OFF.
             * - It is called indirectly from zb_ecc_ecqv_extraction_async() using ZB_SCHEDULE_CALLBACK()
             *   with parameter ASYNC_ON.
             */
            if (ASYNC_ON == async_mode)
#endif
            {
                ZB_SCHEDULE_CALLBACK(g_ctx.async_cb, g_ctx.async_cb_param);
            }
            return;  //interrupt function
        }

        ZB_MEMSET((zb_uint8_t *)g_ctx.u32_pub, 0, g_ecc_pub_key_len);  /*NOTE: it is necessary*/
        // *(BAx + 8) = ((zb_uint32_t)*CertA << 16)| ((zb_uint32_t)*(CertA + 1) << 8) | ((zb_uint32_t)*(CertA + 2));
        // for (i = 0; i < 7; i++)
        // {
        //   *(BAx + (6 - i)) = ((zb_uint32_t)*(CertA+4*i+3) << 24) | ((zb_uint32_t)*(CertA+4*i+4) << 16) | ((zb_uint32_t)*(CertA+4*i+5) << 8) | ((zb_uint32_t)*(CertA+4*i+6));
        // }

        /*Compute hash value of the certificate*/
        (void)zb_sec_b6_hash(CERT, ZB_ECC_SECT283_CERT_LEN, (zb_uint8_t *)g_ctx.u32_pub); /*FIXME:functions return a bool values*/
    }

    /*Validate the public key BA*/
    /*FIXME:need to fix key_validation procedure*/
    if (ZB_PK_INVALID == zb_ecc_pk_validation(g_ctx.aff_point))
    {
        g_last_error = RET_ERROR;
#ifdef ECC_SYNC_MODE_ENABLED
        /*cstat !MISRAC2012-Rule-14.3_* */
        /* C-STAT gives a warning that following condition is always 'false'.
         * zb_ecc_ecqv_extraction_start() is actually called with both parameters ASYNC_ON and ASYNC_OFF.
         * - It is called directly from zb_ecc_ecqv_extraction() with parameter ASYNC_OFF.
         * - It is called indirectly from zb_ecc_ecqv_extraction_async() using ZB_SCHEDULE_CALLBACK()
         *   with parameter ASYNC_ON.
         */
        if (ASYNC_ON == async_mode)
#endif
        {
            ZB_SCHEDULE_CALLBACK(g_ctx.async_cb, g_ctx.async_cb_param);
        }
        return;  //interrupt function
    }

    /*ZB_MEMSET(e, 0, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);*/

    zb_ecc_inverse_bytes((zb_uint8_t *)g_ctx.u32_pub, ZB_ECC_HASH_LEN);
    /*FIXME: rewrite for effective inversion of s*/
    /*for (i = 0; i < 4; i++)
    {
       *(e + i) = ((zb_uint32_t)*(s+4*i) << 24) | ((zb_uint32_t)*(s+4*i+1) << 16) |  ((zb_uint32_t)*(s+4*i+1) << 8) |  ((zb_uint32_t)*(s+4*i+3));
       *(e + i + 4) = 0x0;
    }*/
    /*Compute QA = e*BA + QCA*/
    // ZB_MEMSET(&QA, 0xff, sizeof(zb_ecc_point_aff_t));
#ifdef ECC_SYNC_MODE_ENABLED
    /*cstat !MISRAC2012-Rule-14.3_* */
    /* C-STAT gives a warning that following condition is always 'true'.
     * zb_ecc_ecqv_extraction_start() is actually called with both parameters ASYNC_ON and ASYNC_OFF.
     * - It is called directly from zb_ecc_ecqv_extraction() with parameter ASYNC_OFF.
     * - It is called indirectly from zb_ecc_ecqv_extraction_async() using ZB_SCHEDULE_CALLBACK()
     *   with parameter ASYNC_ON.
     */
    if (ASYNC_OFF == async_mode)
    {
        zb_ecc_tnaf5_random_scalarmul(g_ctx.u32_pub, g_ctx.aff_point, g_ctx.aff_point);
    }
    else
#endif
    {
        zb_ecc_tnaf5_random_scalarmul_async(zb_ecc_ecqv_extraction_finish, g_ctx.u32_pub, g_ctx.aff_point, g_ctx.aff_point);
    }
}
#undef PUB_REC_KEY
#undef CERT


#define CA_PUB_KEY g_ctx.u8p
/**
 * The last step of ECQV scheme.
 * @param  async_mode - ASYNC_ON or ASYNC_OFF. Enables/disables async mode.
 */
void zb_ecc_ecqv_extraction_finish(zb_uint8_t async_mode)
{
    g_last_error = RET_ERROR;


    zb_ecc_affine_to_project(g_ctx.aff_point, g_ctx.pro_point);

    copy_and_inverse((zb_uint8_t *)g_ctx.u32_pub, CA_PUB_KEY, g_ecc_pub_key_len); //Copy and inverse CA's public key
    if (RET_OK == zb_ecc_point_decompression(g_ctx.u32_pub, g_ctx.aff_point))
    {
        zb_ecc_mixed_addition(g_ctx.pro_point, g_ctx.aff_point, g_ctx.pro_point);
        zb_ecc_project_to_affine(g_ctx.pro_point, g_ctx.aff_point);
        zb_ecc_point_compression(g_ctx.aff_point, g_ctx.u32_pub);

        copy_and_inverse(g_ctx.ret_p, (zb_uint8_t *)g_ctx.u32_pub, g_ecc_pub_key_len);

        g_last_error = RET_OK;
    }

#ifdef ECC_SYNC_MODE_ENABLED
    if (ASYNC_ON == async_mode)
#endif
    {
        ZB_SCHEDULE_CALLBACK(g_ctx.async_cb, g_ctx.async_cb_param);
    }
}
#undef CA_PUB_KEY


/**
 * Elliptic Curve Qu-Vanstone implicit certificate scheme. Public key extraction procedure. Async mode
 *
 * @param async_cb       - callback which will be executed after extraction procedure.
 * @param async_cb_param - parameter which will be passed to @async_cb function. It is unchanged during execution.
 * @param QCAx           - the CA's public key in compressed form
 * @param pub_rec        - reconstruction public key in original form
 * @param CertA          - an implicit certificate for user idA
 * @param QAx            - a user A's public key in compressed form
 * @return               - return RET_OK in success and return RET_ERROR in fail.
 */
void zb_ecc_ecqv_extraction_async(zb_callback_t async_cb, const zb_uint8_t async_cb_param, zb_uint8_t *QCAx, zb_uint8_t *pub_rec, zb_uint8_t *CertA, zb_uint8_t *QAx)
{
    g_ctx.async_cb = async_cb;
    g_ctx.async_cb_param = async_cb_param;

    zb_ecc_ecqv_extraction_init(QCAx, pub_rec, CertA, QAx);
    ZB_SCHEDULE_CALLBACK(zb_ecc_ecqv_extraction_start, ASYNC_ON);
}


#ifdef ECC_SYNC_MODE_ENABLED
/**
 * Elliptic Curve Qu-Vanstone implicit certificate scheme. Public key extraction procedure.
 *
 * @param QCAx           - the CA's public key in compressed form
 * @param pub_rec        - reconstruction public key in original form
 * @param CertA          - an implicit certificate for user idA
 * @param QAx            - a user A's public key in compressed form
 * @return               - return RET_OK in success and return RET_ERROR in fail.
 */
zb_ret_t zb_ecc_ecqv_extraction(zb_uint8_t *QCAx, zb_uint8_t *pub_rec, zb_uint8_t *CertA, zb_uint8_t *QAx)
{
    zb_ecc_ecqv_extraction_init(QCAx, pub_rec, CertA, QAx);
    zb_ecc_ecqv_extraction_start(ASYNC_OFF);

    if (RET_OK == g_last_error)
    {
        zb_ecc_ecqv_extraction_finish(ASYNC_OFF);
    }

    return g_last_error;
}
#endif

void zb_ecc_ecmqv_start(zb_uint8_t async_mode);

void zb_ecc_ecmqv_continue(zb_uint8_t async_mode);

void zb_ecc_ecmqv_finish(zb_uint8_t async_mode);


/**
 * Store input parameters to context
 *
 * @param prvA1    - A's private key
 * @param prvA2    - A's ephemeral private key
 * @param pubA2    - A's ephemeral public key
 * @param pubB1    - B's public key
 * @param pubB2    - B's ephemeral public key
 * @param skey     - a shared key between A and B
 */
static void zb_ecc_ecmqv_init(zb_uint8_t *prvA1, zb_uint8_t *prvA2,
                              zb_uint8_t *pubA2, zb_uint8_t *pubB1,
                              zb_uint8_t *pubB2, zb_uint8_t *skey)
{
    g_last_error = RET_OK;  /*Set default value*/

    g_ctx.u32_pub = shared_buf; //NOTE: length is 2 * ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES
    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,56} */
    /* zb_ecc_point_aff_t is aligned by 4 */
    g_ctx.aff_point = (zb_ecc_point_aff_t *)(shared_buf + 2U * ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS);  //NOTE: length is 2 * ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES
    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,57} */
    /* zb_ecc_point_pro_t is aligned by 4 */
    g_ctx.pro_point = (zb_ecc_point_pro_t *)(shared_buf + 4U * ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS);  //NOTE: length is 3 * ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES
    g_ctx.u8p = prvA1;
    g_ctx.u8p1 = prvA2;
    g_ctx.u8p2 = pubA2;
    g_ctx.u8p3 = pubB1;
    g_ctx.u8p4 = pubB2;
    g_ctx.ret_p = skey;
}

#define SET_ECMQV_CONSTANTS(t, mask, hbit) {if (sect163k1 == g_curve_id) {t = 2; mask = 0x3ffff; hbit = 0x40000;} else {t = 4; mask = 0x1fff; hbit = 0x2000;}}


#define Qbar g_ctx.u32_pub
#define pubB1 g_ctx.u8p3
#define pubB2 g_ctx.u8p4
/**
 * Initial step of ECMQV scheme.
 * @param  async_mode - ASYNC_ON or ASYNC_OFF. Enables/disables async mode.
 */
void zb_ecc_ecmqv_start(zb_uint8_t async_mode)
{
    zb_uint8_t i;
    zb_uint8_t t;
    zb_uint32_t mask, hbit;
    zb_uint32_t u32 = 0;
    // zb_uint32_t buf[ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS];
    zb_uint32_t *buf = shared_buf + ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS;  /*NOTE: length : ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES*/
    // static zb_ecc_point_aff_t *Q = (zb_ecc_point_aff_t *)(shared_buf + 2 * ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);
    // static zb_ecc_point_pro_t *P = (zb_ecc_point_pro_t *)(shared_buf + 4 * ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);
    /*
      TRACE_MSG(TRACE_ZSE3, "g_curve_id %hd len %hd", (FMT__H_H, g_curve_id, len));
      DUMP_TRAF("keyA1.d:",(zb_uint8_t *)&keyA1->d, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES,0);
      DUMP_TRAF("keyA1.Qx:",(zb_uint8_t *)&keyA1->Qx, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES,0);
      DUMP_TRAF("keyA2.d:",(zb_uint8_t *)&keyA2->d, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES,0);
      DUMP_TRAF("keyA2.Qx:",(zb_uint8_t *)&keyA2->Qx, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES,0);
      DUMP_TRAF("QB1x:",(zb_uint8_t *)QB1x, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES,0);
      DUMP_TRAF("QB2x:",(zb_uint8_t *)QB2x, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES,0);
    */
    ZB_MEMSET(Qbar, 0xff, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);

    SET_ECMQV_CONSTANTS(t, mask, hbit);

    //DUMP_TRAF("QA2bar:",(zb_uint8_t *)QA2bar, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES,0);
    //DUMP_TRAF("QB2bar:",(zb_uint8_t *)QB2bar, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES,0);

    /*Compute integers QA2bar and QB2bar from QA2 and QB2*/

    copy_and_inverse((zb_uint8_t *)buf, pubB2, g_ecc_pub_key_len);
    for (i = 0; i < g_ecc_pub_key_len_in_dwords; i++)
    {
        if (i > t)
        {
            u32 = 0;
        }
        if (t == i)
        {
            u32 = (*(buf + i) & mask) | hbit;
        }
        if (i < t)
        {
            u32 = *(buf + i);
        }

        *(Qbar + i) = u32;
    }

    /*Compute 4*s*(QB2 + QB2bar*QB1)*/
    ZB_MEMSET((zb_uint8_t *)buf, 0, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES); /*NOTE: it is necessary step. It is enough for pubB1 copying too.*/
    copy_and_inverse((zb_uint8_t *)buf, pubB2, g_ecc_pub_key_len);
    if (RET_ERROR == zb_ecc_point_decompression(buf, g_ctx.aff_point))
    {
        g_last_error = RET_ERROR;
#ifdef ECC_SYNC_MODE_ENABLED
        if (ASYNC_ON == async_mode)
#endif
        {
            ZB_SCHEDULE_CALLBACK(g_ctx.async_cb, g_ctx.async_cb_param);
        }
        return;  //interrupt function
    }

    zb_ecc_affine_to_project(g_ctx.aff_point, g_ctx.pro_point);

    // ZB_MEMSET((zb_uint8_t *)buf, 0, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);
    copy_and_inverse((zb_uint8_t *)buf, pubB1, g_ecc_pub_key_len);
    if (RET_ERROR == zb_ecc_point_decompression(buf, g_ctx.aff_point))
    {
        g_last_error = RET_ERROR;
#ifdef ECC_SYNC_MODE_ENABLED
        if (ASYNC_ON == async_mode)
#endif
        {
            ZB_SCHEDULE_CALLBACK(g_ctx.async_cb, g_ctx.async_cb_param);
        }
        return;  //interrupt function
    }

#ifdef ECC_SYNC_MODE_ENABLED
    if (ASYNC_OFF == async_mode)
    {
        zb_ecc_tnaf5_random_scalarmul(Qbar, g_ctx.aff_point, g_ctx.aff_point);
    }
    else
#endif
    {
        zb_ecc_tnaf5_random_scalarmul_async(zb_ecc_ecmqv_continue, Qbar, g_ctx.aff_point, g_ctx.aff_point);
    }
}
#undef pubB1
#undef pubB2


#define PRV_A1 g_ctx.u8p
#define PRV_A2 g_ctx.u8p1
#define PUB_A2 g_ctx.u8p2
/**
 * Intermediate step of ECMQV scheme.
 * @param  async_mode - ASYNC_ON or ASYNC_OFF. Enables/disables async mode.
 */
void zb_ecc_ecmqv_continue(zb_uint8_t async_mode)
{
    zb_uint8_t i;
    zb_uint8_t t;
    zb_uint32_t mask, hbit;
    zb_uint32_t u32 = 0;
    zb_uint32_t *buf = shared_buf + ZB_ECC_MAX_PUB_KEY_LEN_IN_DWORDS;

    SET_ECMQV_CONSTANTS(t, mask, hbit);

    zb_ecc_mixed_addition(g_ctx.pro_point, g_ctx.aff_point, g_ctx.pro_point);
    zb_ecc_project_to_affine(g_ctx.pro_point, g_ctx.aff_point);

    copy_and_inverse((zb_uint8_t *)buf, PUB_A2, g_ecc_pub_key_len);
    for (i = 0; i < g_ecc_pub_key_len_in_dwords; i++)
    {
        if (i > t)
        {
            u32 = 0;
        }
        if (t == i)
        {
            u32 = (*(buf + i) & mask) | hbit;
        }
        if (i < t)
        {
            u32 = *(buf + i);
        }

        *(Qbar + i) = u32;
    }


    /*Compute an integer s = dA2 + QA2bar*dA1 (mod n)*/
    ZB_MEMSET((zb_uint8_t *)buf, 0, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);
    copy_and_inverse((zb_uint8_t *)buf, PRV_A1, g_ecc_prv_key_len);
    if (g_curve_id == sect283k1)
    {
        zb_ecc_modpmul160(Qbar, buf, Qbar);
    }
    else
    {
        zb_ecc_modpmul96(Qbar, buf, Qbar);
    }
    //DUMP_TRAF("s1:",(zb_uint8_t *)s, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES,0);

    /*FIXME: 10 dword of keyA2->d is not a zero*/
    //dump_bytes(keyA2->d, 40, "KeyA2");

    ZB_MEMSET((zb_uint8_t *)buf, 0, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES);
    copy_and_inverse((zb_uint8_t *)buf, PRV_A2, g_ecc_prv_key_len);
    zb_ecc_modpadd(Qbar, buf, Qbar);

    //DUMP_TRAF("s2:",(zb_uint8_t *)s, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES,0);
    /*Compute 4*s*/
    if (sect163k1 == g_curve_id)
    {
        for (i = 5; i != 0U; i--)
        {
            *(Qbar + i) = (*(Qbar + i) << 1) | (*(Qbar + (i - 1U)) >> 31);
        }
        *Qbar = *Qbar << 1;
    }
    else
    {
        ZB_ASSERT(sect283k1 == g_curve_id);
        for (i = 8; i != 0U; i--)
        {
            *(Qbar + i) = (*(Qbar + i) << 2) | (*(Qbar + (i - 1U)) >> 30);
        }
        *Qbar = *Qbar << 2;
    }

    while (zb_ecc_compare(Qbar, eccn[g_curve_id], g_ecc_pub_key_len_in_dwords) != ZB_BI_LESS)
    {
        (void)zb_ecc_sub(Qbar, eccn[g_curve_id], g_ecc_pub_key_len_in_dwords, Qbar);
    }
    //DUMP_TRAF("s3:",(zb_uint8_t *)s, ZB_ECC_MAX_PUB_KEY_LEN_IN_BYTES,0);

    // zb_ecc_tnaf5_random_scalarmul(Qbar, Q, Q);
#ifdef ECC_SYNC_MODE_ENABLED
    if (ASYNC_OFF == async_mode)
    {
        zb_ecc_tnaf5_random_scalarmul(Qbar, g_ctx.aff_point, g_ctx.aff_point);
    }
    else
#endif
    {
        zb_ecc_tnaf5_random_scalarmul_async(zb_ecc_ecmqv_finish, Qbar, g_ctx.aff_point, g_ctx.aff_point);
    }
}
#undef Qbar
#undef PRV_A1
#undef PRV_A2
#undef PUB_A2


/**
 * Last step of ECMQV scheme.
 * @param  async_mode - ASYNC_ON or ASYNC_OFF. Enables/disables async mode.
 */
void zb_ecc_ecmqv_finish(zb_uint8_t async_mode)
{
    zb_uint8_t i;
    g_last_error = RET_ERROR;
    /*If P is infinity, return error*/
    if (sect163k1 == g_curve_id)
    {
        if (!(IF0_163(g_ctx.aff_point->x) && IF0_163(g_ctx.aff_point->y)))
        {
            *g_ctx.ret_p = (zb_uint8_t)(*(g_ctx.aff_point->x + 5));
            for (i = 1; i < 6U; i++)
            {
                /*x coordinate*/
                *(g_ctx.ret_p + 4U * (i - 1U) + 1U) = (zb_uint8_t)(*(g_ctx.aff_point->x + (5U - i)) >> 24);
                *(g_ctx.ret_p + 4U * (i - 1U) + 2U) = (zb_uint8_t)(*(g_ctx.aff_point->x + (5U - i)) >> 16);
                *(g_ctx.ret_p + 4U * (i - 1U) + 3U) = (zb_uint8_t)(*(g_ctx.aff_point->x + (5U - i)) >> 8);
                *(g_ctx.ret_p + 4U * (i - 1U) + 4U) = (zb_uint8_t)(*(g_ctx.aff_point->x + (5U - i)));
            }
            g_last_error = RET_OK;
        }
    }
    else
    {
        ZB_ASSERT(sect283k1 == g_curve_id);
        if (!(IF0_283(g_ctx.aff_point->x) && IF0_283(g_ctx.aff_point->y)))
        {
            *g_ctx.ret_p = (zb_uint8_t)(*(g_ctx.aff_point->x + 8) >> 24);
            *(g_ctx.ret_p + 1) = (zb_uint8_t)(*(g_ctx.aff_point->x + 8) >> 16);
            *(g_ctx.ret_p + 2) = (zb_uint8_t)(*(g_ctx.aff_point->x + 8) >> 8);
            *(g_ctx.ret_p + 3) = (zb_uint8_t) * (g_ctx.aff_point->x + 8);
            for (i = 1; i < 9U; i++)
            {
                /*x coordinate*/
                *(g_ctx.ret_p + 4U * i) = (zb_uint8_t)(*(g_ctx.aff_point->x + (8U - i)) >> 24);
                *(g_ctx.ret_p + 4U * i + 1U) = (zb_uint8_t)(*(g_ctx.aff_point->x + (8U - i)) >> 16);
                *(g_ctx.ret_p + 4U * i + 2U) = (zb_uint8_t)(*(g_ctx.aff_point->x + (8U - i)) >> 8);
                *(g_ctx.ret_p + 4U * i + 3U) = (zb_uint8_t)(*(g_ctx.aff_point->x + (8U - i)));
            }
            g_last_error = RET_OK;
        }
    }

#ifdef ECC_SYNC_MODE_ENABLED
    if (ASYNC_ON == async_mode)
#endif
    {
        ZB_SCHEDULE_CALLBACK(g_ctx.async_cb, g_ctx.async_cb_param);
    }
}


/**
 * Elliptic curve MQV scheme for shared secret generation. Async mode
 *
 * @param async_cb       - callback which will be executed after extraction procedure.
 * @param async_cb_param - parameter which will be passed to @async_cb function. It is unchanged during execution.
 * @param prv_A1          - A's private key
 * @param prv_A2          - A's ephemeral private key
 * @param pub_A2          - A's ephemeral public key
 * @param pub_B1          - B's public key
 * @param pub_B2          - B's ephemeral public key
 * @param skey           - a shared key between A and B
 * @return               - return RET_OK in success and return RET_ERROR in fail.
 */
void zb_ecc_ecmqv_async(zb_callback_t async_cb, const zb_uint8_t async_cb_param, zb_uint8_t *prv_A1,
                        zb_uint8_t *prv_A2, zb_uint8_t *pub_A2,
                        zb_uint8_t *pub_B1, zb_uint8_t *pub_B2, zb_uint8_t *skey)
{
    g_ctx.async_cb = async_cb;
    g_ctx.async_cb_param = async_cb_param;

    zb_ecc_ecmqv_init(prv_A1, prv_A2, pub_A2, pub_B1, pub_B2, skey);
    ZB_SCHEDULE_CALLBACK(zb_ecc_ecmqv_start, ASYNC_ON);
}


#ifdef ECC_SYNC_MODE_ENABLED
/**
 * Elliptic curve MQV scheme for shared secret generation
 *
 * @param prv_A1  - A's private key
 * @param prv_A2  - A's ephemeral private key
 * @param pub_A2  - A's ephemeral public key
 * @param pub_B1  - B's public key
 * @param pub_B2  - B's ephemeral public key
 * @param skey   - a shared key between A and B
 * @return       - return RET_OK in success and return RET_ERROR in fail.
 */
zb_ret_t zb_ecc_ecmqv(zb_uint8_t *prv_A1, zb_uint8_t *prv_A2,
                      zb_uint8_t *pub_A2, zb_uint8_t *pub_B1,
                      zb_uint8_t *pub_B2, zb_uint8_t *skey)
{
    zb_ecc_ecmqv_init(prv_A1, prv_A2, pub_A2, pub_B1, pub_B2, skey);
    zb_ecc_ecmqv_start(ASYNC_OFF);
    if (RET_OK == g_last_error)
    {
        zb_ecc_ecmqv_continue(ASYNC_OFF);
        if (RET_OK == g_last_error)
        {
            zb_ecc_ecmqv_finish(ASYNC_OFF);
        }
    }

    return g_last_error;
}
#endif

