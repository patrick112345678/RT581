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
/*  PURPOSE: NCP utility functions implementation.
*/

#define ZB_TRACE_FILE_ID 33
#include "zbncp_utils.h"
#include "zbncp_mem.h"
#include "zbncp_debug.h"

/*
 * CRC polynomials and algorithms are selected to provide
 * compatibility with existing ZBOSS macsplit protocol
 * (e.g. in the CRC-16 implementation an intial and output
 * values are not inverted).
 * This implementation is not optimal.
 */

#define ZBNCP_CRC8_GEN_POLY   0xb2u
#define ZBNCP_CRC16_GEN_POLY  0x8408u

#define ZBNCP_LOWEST_BIT(x)   ((x) & 1u)

zbncp_uint8_t zbncp_crc8(zbncp_uint8_t init, const void *data, zbncp_size_t size)
{
    const zbncp_uint8_t *p = (const zbncp_uint8_t *) data;
    zbncp_uint8_t crc = (zbncp_uint8_t) ~init;
    zbncp_size_t i;
    zbncp_size_t bit;

    for (i = 0u; i < size; ++i)
    {
        crc ^= p[i];
        for (bit = 0u; bit < 8u; ++bit)
        {
            zbncp_uint8_t bitval = (crc & 1u);
            zbncp_uint8_t mask = (zbncp_uint8_t) (-(zbncp_int16_t)bitval);
            /* Lowest bit was converted to a mask: 0 -> 0x00, 1 -> 0xFF */
            crc = (crc >> 1) ^ (ZBNCP_CRC8_GEN_POLY & mask);
        }
    }
    crc = (zbncp_uint8_t) ~crc;

    ZBNCP_DBG_TRACE("CRC8 init %#02x data %#p size %zu -> crc %#02x", init, data, size, crc);

    return crc;
}

zbncp_uint16_t zbncp_crc16(zbncp_uint16_t init, const void *data, zbncp_size_t size)
{
    const zbncp_uint8_t *p = (const zbncp_uint8_t *) data;
    zbncp_uint16_t crc = init;
    zbncp_size_t i;
    zbncp_size_t bit;

    for (i = 0u; i < size; ++i)
    {
        zbncp_uint16_t byte = p[i];
        crc ^= byte;
        for (bit = 0u; bit < 8u; ++bit)
        {
            zbncp_uint16_t bitval = (crc & 1u);
            zbncp_uint16_t mask = ((zbncp_uint16_t) - (zbncp_int32_t)bitval);
            /* Lowest bit was converted to a mask: 0 -> 0x0000, 1 -> 0xFFFF */
            crc = (crc >> 1) ^ (ZBNCP_CRC16_GEN_POLY & mask);
        }
    }

    ZBNCP_DBG_TRACE("CRC16 init %#04x data %#p size %zu --> crc %#04x", init, data, size, crc);

    return crc;
}
