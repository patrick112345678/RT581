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
/* PURPOSE: 8051-specific random number generator
*/

#define ZB_TRACE_FILE_ID 123
#include "zboss_api_core.h"
#include "zb_magic_macros.h"
#include "zb_hash.h"

static zb_uint32_t zb_last_random_value_jtr = 0;

#ifndef ZB_CONTEXTS_IN_SEPARATE_FILE

#include "zb_common.h"
#include "zb_common_u.h"

/* Interrupt context. Moved here to build utils */
#ifdef ZB_CONFIG_FIX_G_IZB
/* fixing address the g_izb in memory,
otherwise the addresses of the SPI buffers may changed in different builds */
__attribute__((section (".zb_global"))) zb_intr_globals_t g_izb;
#else
zb_intr_globals_t g_izb;
#endif
#endif /* ZB_CONTEXTS_IN_SEPARATE_FILE */

zb_uint32_t zb_random()
{
    return zb_random_seed();
}

zb_uint32_t zb_random_val(zb_uint32_t max_value)
{
    zb_uint32_t ret = (max_value == 0U) ? 0U : (zb_random() / (ZB_RAND_MAX / max_value));
    ZB_ASSERT(ret <= max_value);
    return ret;
}

zb_uint32_t zb_random_jitter()
{
    if (zb_last_random_value_jtr == 0U)
    {
        zb_last_random_value_jtr = zb_random_seed();
    }
    zb_last_random_value_jtr = 1103515245UL * zb_last_random_value_jtr + 12345UL;

    return zb_last_random_value_jtr;
}

#ifndef ZB_LITTLE_ENDIAN
void zb_htole16(zb_uint8_t ZB_XDATA *ptr, zb_uint8_t ZB_XDATA *val)
{
    ptr[1] = val[0];
    ptr[0] = val[1];
}
void zb_htole32(zb_uint8_t ZB_XDATA *ptr, zb_uint8_t ZB_XDATA *val)
{
    ptr[3] = val[0],
             ptr[2] = val[1],
                      ptr[1] = val[2],
                               ptr[0] = val[3];
}
#else
void zb_htobe32(zb_uint8_t ZB_XDATA *ptr, zb_uint8_t ZB_XDATA *val)
{
    ((zb_uint8_t *)(ptr))[3] = ((zb_uint8_t *)(val))[0],
                               ((zb_uint8_t *)(ptr))[2] = ((zb_uint8_t *)(val))[1],
                                       ((zb_uint8_t *)(ptr))[1] = ((zb_uint8_t *)(val))[2],
                                               ((zb_uint8_t *)(ptr))[0] = ((zb_uint8_t *)(val))[3];
}
#endif


#ifndef ZB_IAR
void zb_get_next_letoh16(zb_uint16_t *dst, zb_uint8_t **src)
{
    ZB_LETOH16(dst, *src);
    (*src) += 2;
}
#endif

void *zb_put_next_htole16(zb_uint8_t *dst, zb_uint16_t val)
{
    ZB_HTOLE16(dst, &val);
    return (dst + 2);
}

void *zb_put_next_2_htole16(zb_uint8_t *dst, zb_uint16_t val1, zb_uint16_t val2)
{
    ZB_HTOLE16(dst, &val1);
    ZB_HTOLE16(dst + 2, &val2);
    return (void *)(dst + 4);
}

void *zb_put_next_2_htole32(zb_uint8_t *dst, zb_uint32_t val1, zb_uint32_t val2)
{
    ZB_HTOLE32(dst, &val1);
    ZB_HTOLE32((dst + 4), &val2);
    return (void *)(dst + 8);
}

void *zb_put_next_htole32(zb_uint8_t *dst, zb_uint32_t val1)
{
    ZB_HTOLE32(dst, &val1);
    return (void *)(dst + 4);
}

#if 0
#define FNV1_32_INIT ((zb_uint32_t)0x811c9dc5)

zb_uint32_t
zb_fnv_32a_uint16(zb_uint16_t v)
{
    zb_uint32_t hval = FNV1_32_INIT;

    /* xor the bottom with the current octet */
    hval ^= (zb_uint32_t)(v & 0xff);
    /* multiply by the 32 bit FNV magic prime mod 2^32 */
    hval += (hval << 1) + (hval << 4) + (hval << 7) + (hval << 8) + (hval << 24);
    v >>= 8;
    hval ^= (zb_uint32_t)(v);
    /* multiply by the 32 bit FNV magic prime mod 2^32 */
    hval += (hval << 1) + (hval << 4) + (hval << 7) + (hval << 8) + (hval << 24);
    return hval;
}
#endif

/* Calculate Fowler-Noll-Vo hash for random number generator. */

zb_uint32_t zb_get_fnv_hash(zb_uint8_t *buffer)
{
    zb_uint32_t random_generator_seed = FNV_32_FNV0;

    while (*buffer != 0U)
    {
        random_generator_seed ^= (zb_uint32_t) * buffer++;
        random_generator_seed *= FNV_32_PRIME;
    }
    return random_generator_seed;
}

#if 0
zb_uint8_t zb_crc8(zb_uint8_t *p, zb_uint8_t crc, zb_uint_t len)
{
    static ZB_CODE ZB_CONST zb_uint16_t table[256] =
    {
        0xea, 0xd4, 0x96, 0xa8, 0x12, 0x2c, 0x6e, 0x50, 0x7f, 0x41, 0x03, 0x3d,
        0x87, 0xb9, 0xfb, 0xc5, 0xa5, 0x9b, 0xd9, 0xe7, 0x5d, 0x63, 0x21, 0x1f,
        0x30, 0x0e, 0x4c, 0x72, 0xc8, 0xf6, 0xb4, 0x8a, 0x74, 0x4a, 0x08, 0x36,
        0x8c, 0xb2, 0xf0, 0xce, 0xe1, 0xdf, 0x9d, 0xa3, 0x19, 0x27, 0x65, 0x5b,
        0x3b, 0x05, 0x47, 0x79, 0xc3, 0xfd, 0xbf, 0x81, 0xae, 0x90, 0xd2, 0xec,
        0x56, 0x68, 0x2a, 0x14, 0xb3, 0x8d, 0xcf, 0xf1, 0x4b, 0x75, 0x37, 0x09,
        0x26, 0x18, 0x5a, 0x64, 0xde, 0xe0, 0xa2, 0x9c, 0xfc, 0xc2, 0x80, 0xbe,
        0x04, 0x3a, 0x78, 0x46, 0x69, 0x57, 0x15, 0x2b, 0x91, 0xaf, 0xed, 0xd3,
        0x2d, 0x13, 0x51, 0x6f, 0xd5, 0xeb, 0xa9, 0x97, 0xb8, 0x86, 0xc4, 0xfa,
        0x40, 0x7e, 0x3c, 0x02, 0x62, 0x5c, 0x1e, 0x20, 0x9a, 0xa4, 0xe6, 0xd8,
        0xf7, 0xc9, 0x8b, 0xb5, 0x0f, 0x31, 0x73, 0x4d, 0x58, 0x66, 0x24, 0x1a,
        0xa0, 0x9e, 0xdc, 0xe2, 0xcd, 0xf3, 0xb1, 0x8f, 0x35, 0x0b, 0x49, 0x77,
        0x17, 0x29, 0x6b, 0x55, 0xef, 0xd1, 0x93, 0xad, 0x82, 0xbc, 0xfe, 0xc0,
        0x7a, 0x44, 0x06, 0x38, 0xc6, 0xf8, 0xba, 0x84, 0x3e, 0x00, 0x42, 0x7c,
        0x53, 0x6d, 0x2f, 0x11, 0xab, 0x95, 0xd7, 0xe9, 0x89, 0xb7, 0xf5, 0xcb,
        0x71, 0x4f, 0x0d, 0x33, 0x1c, 0x22, 0x60, 0x5e, 0xe4, 0xda, 0x98, 0xa6,
        0x01, 0x3f, 0x7d, 0x43, 0xf9, 0xc7, 0x85, 0xbb, 0x94, 0xaa, 0xe8, 0xd6,
        0x6c, 0x52, 0x10, 0x2e, 0x4e, 0x70, 0x32, 0x0c, 0xb6, 0x88, 0xca, 0xf4,
        0xdb, 0xe5, 0xa7, 0x99, 0x23, 0x1d, 0x5f, 0x61, 0x9f, 0xa1, 0xe3, 0xdd,
        0x67, 0x59, 0x1b, 0x25, 0x0a, 0x34, 0x76, 0x48, 0xf2, 0xcc, 0x8e, 0xb0,
        0xd0, 0xee, 0xac, 0x92, 0x28, 0x16, 0x54, 0x6a, 0x45, 0x7b, 0x39, 0x07,
        0xbd, 0x83, 0xc1, 0xff
    };
    crc &= 0xff;
    while (len--)
    {
        crc = table[crc ^ *p++];
    }
    return crc;
}
#endif

/* zb_crc8_slow() is an equivalent bit-wise implementation of crc8() that does not
   need a table, and which can be used to generate crc8_table[]. Entry k in the
   table is the CRC-8 of the single byte k, with an initial crc value of zero.
   0xb2 is the bit reflection of 0x4d, the polynomial coefficients below x^8. */
zb_uint8_t zb_crc8(zb_uint8_t *p, zb_uint8_t crc, zb_uint_t len)
{
    zb_uint8_t k;

    if (p == NULL)
    {
        return 0;
    }

    crc = ~crc & 0xFFU;
    while ((len--) != 0U)
    {
        crc ^= *p++;
        for (k = 0U; k < 8U; k++)
        {
            crc = ZB_U2B(crc & 1U) ? (crc >> 1) ^ 0xB2U : crc >> 1;
        }
    }
    return crc ^ 0xFFU;
}

zb_uint16_t zb_crc16(zb_uint8_t *p, zb_uint16_t crc, zb_uint_t len)
{
    zb_uint8_t i;

    while ((len--) != 0U)
    {
        crc ^= *p++;
        for ( i = 0U; i < 8U; i++ )
        {
            crc = ( (crc & 0x0001U) == 1U ) ? ( crc >> 1 ) ^ 0x8408U : crc >> 1;
        }
    }
    return crc; //AEV: do not inverse only for compatibility with apps,
    //in crc-16/x-25 config, input crc must be fixed 0xffff and output must be inversed!
}


zb_uint32_t zb_crc32(zb_uint8_t *message, int len)
{
    int i, j;
    zb_uint32_t byte, crc, mask;

    crc = 0xFFFFFFFFUL;
    for (i = 0 ; i < len ; ++i)
    {
        byte = message[i];
        crc = crc ^ byte;
        for (j = 7; j >= 0; j--)
        {
            mask = ZB_BIT_IS_SET(crc, ZB_BITS1(0)) ? ~(0U) : 0U;
            crc = (crc >> 1) ^ (0xEDB88320UL & mask);
        }
    }
    return ~crc;
}

zb_uint32_t zb_crc32_next(zb_uint32_t prev_crc, zb_uint8_t *message, int len)
{
    int i, j;
    zb_uint32_t byte, crc, mask;

    crc = ~prev_crc;
    for (i = 0 ; i < len ; ++i)
    {
        byte = message[i];
        crc = crc ^ byte;
        for (j = 7; j >= 0; j--)
        {
            mask = ZB_BIT_IS_SET(crc, ZB_BITS1(0)) ? ~(0U) : 0U;
            crc = (crc >> 1) ^ (0xEDB88320UL & mask);
        }
    }

    return ~crc;
}

zb_uint32_t zb_crc32_next_v2(zb_uint32_t prev_crc, zb_uint8_t *message, zb_uint16_t len)
{
#define ZB_CRC32_POLY 0x04C11DB7U
    zb_uint32_t crc, c, i;

    crc = (ZB_U2B(prev_crc)) ? (~prev_crc) : 0U;
    while (len > 0U)
    {
        c  =  (((crc ^ (*message)) & 0xFFU) << 24);
        for (i = 8U; i > 0U; --i)
        {
            c = ZB_U2B(c & 0x80000000U) ? (c << 1) ^ ZB_CRC32_POLY : (c << 1);
        }
        crc = ( crc >> 8 ) ^ c;
        ++message;
        --len;
    }

    return ~crc;
}

zb_uint32_t zb_64bit_hash(zb_uint8_t *data)
{
    zb_uint_t i;
    zb_uint32_t hash = ZB_HASH_MAGIC_VAL;
    for (i = 0 ; i < sizeof(zb_64bit_addr_t) ; ++i)
    {
        hash = ZB_HASH_FUNC_STEP(hash, data[i]);
    }
    return (hash & ZB_INT_MASK);
}

#ifndef ZB_CONFIG_NRF52
void zb_bzero(void *p, zb_uint_t size)
{
    ZB_MEMSET(p, 0, size);
}

void zb_bzero_2(void *p)
{
    ZB_MEMSET(p, 0, 2);
}
#endif


void zb_bzero_volatile(volatile void *s, zb_uint_t size)
{
    volatile zb_uint8_t *p = s;
    while (size-- > 0U)
    {
        p[size] = 0;
    }
}


void zb_memcpy8(zb_uint8_t *ptr, zb_uint8_t *src)
{
    zb_uint_t i;
    for (i = 0U; i < 8U; ++i)
    {
        *ptr ++ = *src++;
    }
}

/**
 * Calculates number of '1' in 16bit bitmask
 */
zb_uint8_t zb_bit_cnt16(zb_uint16_t a)
{
    static const zb_uint8_t bitc[16] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};
    return bitc[ a & 0x0FU ] + bitc[ (a >> 4) & 0x0FU ] + bitc[ (a >> 8) & 0x0FU ] + bitc[ (a >> 12) & 0x0FU ];
}


void zb_inverse_bytes(zb_uint8_t *ptr, zb_uint32_t len)
{
    zb_uint8_t tmp;
    zb_uint32_t i;

    for (i = 0U; i < len / 2U; i++)
    {
        tmp = *(ptr + i);
        *(ptr + i) = *(ptr + len - i - 1);
        *(ptr + len - i - 1) = tmp;
    }
}


zb_uint_t zb_high_bit_number(zb_uint32_t mask)
{
    return MAGIC_LOG2_32(mask);
}


zb_uint_t zb_low_bit_number(zb_uint32_t mask)
{
    zb_uint32_t v = MAGIC_LEAST_SIGNIFICANT_BIT_MASK(mask);
    return zb_high_bit_number(v);
}
