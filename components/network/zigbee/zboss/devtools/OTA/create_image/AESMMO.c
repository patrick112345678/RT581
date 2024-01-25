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
/* PURPOSE:
*/
#include "types.h"
#include "AESMMO.h"
#include "AES128.h"
#include <string.h>

static void copy_tag(uint8 *msg_block, uint32 Mlen);
static uint16 rev16(uint16 l);
static uint32 rev32(uint32 l);

// AES MMO construction
uint32 AES_MMO(uint8 *M, uint32 Mlen, uint8 *h)
{
    uint32 i, j;
    uint8 hash_j[16]; // 128-bit hash block
    uint8 msg_rest[32];  // 256-bit rest of message block
    uint8 *msg_ptr;
    uint32 t;         // total number of message blocks
    uint32 n_rest_blocks; // 1 or 2 blocks
    uint32 n_rest_bytes, n_msg_blocks;
    uint8 key128[16] = {0x00}; // key

    n_msg_blocks = (Mlen >> 7) + !!(Mlen & 0x7F);
    n_rest_bytes = ((Mlen & 0x7F) >> 3) + !!(Mlen & 0x07);
    if (Mlen < 0x10000)
    {
        n_rest_blocks = (n_rest_bytes <= 13) ? 1 : 2;
    }
    else
    {
        n_rest_blocks = (n_rest_bytes <= 9) ? 1 : 2;
    }

    t = n_msg_blocks + n_rest_blocks - (Mlen & 0x7F ? 1 : 0);

    {
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
        copy_tag(msg_ptr, Mlen);
    }

    for (j = 0, msg_ptr = M; j < t; ++j, msg_ptr += 16)
    {
        if (j == t - n_rest_blocks)
        {
            msg_ptr = msg_rest;
        }

        aes_block_enc(key128, msg_ptr, hash_j);
        for (i = 0; i < 16; ++i)
        {
            hash_j[i] ^= msg_ptr[i];
        }
        memcpy(key128, hash_j, 16);
    }

    memcpy(h, hash_j, 16);

    return 1;
}


static void copy_tag(uint8 *msg_block, uint32 Mlen)
{
    if (Mlen < 0x10000)
    {
        uint16 l_str = rev16((uint16)Mlen);
        memcpy(msg_block + 14, &l_str, 2);
    }
    else
    {
        uint32 l_str = rev32(Mlen);
        memcpy(msg_block + 10, &l_str, 4);
    }
}


static uint16 rev16(uint16 l)
{
    return (l >> 8) | (l << 8);
}

static uint32 rev32(uint32 l)
{
    return (l >> 24) | (l >> 8 & 0x0000ff00) | (l << 24) | (l << 8 & 0x00ff0000);
}
