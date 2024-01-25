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
/* PURPOSE: 24 bit variable mathematics
*/

#define ZB_TRACE_FILE_ID 1843


#include "zb_common.h"
#include "zb_types.h"

#ifdef ZB_UINT24_48_SUPPORT

#ifdef DEBUG

/* enpty lines to fix ZB_ASSERT_COMPILE_DECL implementation...
 */

ZB_ASSERT_COMPILE_DECL(sizeof(zb_uint48_t) == sizeof(zb_int48_t));
ZB_ASSERT_COMPILE_DECL(sizeof(zb_uint48_t) == ZB_48BIT_SIZE);

#endif /* DEBUG */


void zb_uint64_to_uint48(zb_uint64_t var, zb_uint48_t *res)
{
    res->low = (zb_uint32_t)var;
    res->high = (zb_uint16_t)(var >> 32);
}


/**
 * @f - first operand
 * @s - second operand
 * @r - result of operation
 */
zb_uint8_t zb_uint48_add(const zb_uint48_t *f, const zb_uint48_t *s, zb_uint48_t *r)
{
    zb_uint64_t temp = (zb_uint64_t)(f->low) + (zb_uint64_t)(s->low);

    temp += ((zb_uint64_t)f->high + (zb_uint64_t)s->high) << 32;

    if (((temp >> 48) & 1U) != 0U)
    {
        return ZB_MATH_OVERFLOW;
    }

    zb_uint64_to_uint48(temp, r);

    return ZB_MATH_OK;
}


zb_int64_t zb_int48_to_int64(const zb_int48_t *var)
{
    zb_uint64_t res = 0;

    res |= var->low;
    res |= ((zb_uint64_t)(var->high) << 32);

    if (((zb_uint16_t)var->high & 0x8000U) != 0U)
    {
        res |= 0xFFFF000000000000U;
    }

    return (zb_int64_t)res;
}


void zb_int64_to_int48(zb_int64_t var, zb_int48_t *res)
{
    if ((var <= MAX_SIGNED_48BIT_VAL) && (var >= MIN_SIGNED_48BIT_VAL))
    {
        zb_uint48_t temp;

        temp.low = (zb_uint32_t)var;
        temp.high = (zb_uint16_t)((zb_uint64_t)var >> 32);

        res->low = temp.low;
        res->high = (zb_int16_t)temp.high;
    }
    else
    {
        /**
         * The variable type zb_int64_t can be out of the bounds
         */
        zb_uint64_t min_max_value = (zb_uint64_t)((var > MAX_SIGNED_48BIT_VAL) ? MAX_SIGNED_48BIT_VAL : MIN_SIGNED_48BIT_VAL);
        zb_uint16_t temp_high;

        res->low = (zb_uint32_t)min_max_value;

        temp_high = (zb_uint16_t)(min_max_value >> 32);
        res->high = (zb_int16_t)temp_high;
    }
}


/**
 * @f - first operand
 * @s - second operand
 * @r - result of operation
 */
zb_uint8_t zb_int48_add(const zb_int48_t *f, const zb_int48_t *s, zb_int48_t *r)
{

    zb_int64_t temp;

    temp = zb_int48_to_int64(f) + zb_int48_to_int64(s);

    if ((temp < MIN_SIGNED_48BIT_VAL) || (temp > MAX_SIGNED_48BIT_VAL))
    {
        return ZB_MATH_OVERFLOW;
    }

    zb_int64_to_int48(temp, r);

    return ZB_MATH_OK;
}


/**
 * @f - first operand
 * @s - second operand
 * @r - result of operation
 */
zb_uint8_t zb_int48_sub(const zb_int48_t *f, const zb_int48_t *s, zb_int48_t *r)
{

    zb_int64_t temp;

    temp = zb_int48_to_int64(f) - zb_int48_to_int64(s);

    if ((temp < MIN_SIGNED_48BIT_VAL) || (temp > MAX_SIGNED_48BIT_VAL))
    {
        return ZB_MATH_OVERFLOW;
    }

    zb_int64_to_int48(temp, r);

    return ZB_MATH_OK;
}


void zb_int64_to_uint48(zb_int64_t var, zb_uint48_t *res)
{
    if (var <= (zb_int64_t)MAX_UNSIGNED_48BIT_VAL && (var >= 0))
    {
        res->low = (zb_uint32_t)var;
        res->high = (zb_uint16_t)((zb_uint64_t)var >> 32);
    }
    else
    {
        /**
         * The variable type zb_int64_t can be out of the bounds
         */
        zb_uint64_t min_max_value = (var < 0) ? 0U : MAX_UNSIGNED_48BIT_VAL;
        zb_uint16_t temp_high;

        res->low = (zb_uint32_t)min_max_value;

        temp_high = (zb_uint16_t)(min_max_value >> 32);
        res->high = temp_high;
    }
}


zb_int64_t zb_uint48_to_int64(const zb_uint48_t *var)
{
    zb_uint64_t res = 0;

    res |= var->low;
    res |= ((zb_uint64_t)var->high << 32);

    return (zb_int64_t)res;
}


/**
 * @f - first operand
 * @s - second operand
 * @r - result of operation
 */
zb_uint8_t zb_uint48_sub(const zb_uint48_t *f, const zb_uint48_t *s, zb_uint48_t *r)
{
    zb_uint8_t ret = ZB_MATH_OK;
    zb_int64_t temp;
    zb_int64_t temp_f = (zb_int64_t)zb_uint48_to_int64(f);
    zb_int64_t temp_s = (zb_int64_t)zb_uint48_to_int64(s);

    if (temp_f >= temp_s)
    {
        temp = temp_f - temp_s;
        zb_int64_to_uint48(temp, r);
    }
    else
    {
        ret = ZB_MATH_OVERFLOW;
    }

    return ret;
}


zb_uint8_t zb_int48_neg(const zb_int48_t *f, zb_int48_t *r)
{
    zb_int64_t temp;

    zb_uint64_t u64_temp = (zb_uint64_t)zb_int48_to_int64(f);
    u64_temp = ~(u64_temp) + 1U;

    temp = (zb_int64_t)u64_temp;

    zb_int64_to_int48(temp, r);

    return ZB_MATH_OK;
}


/**
 * @f - first operand
 * @s - second operand
 * @r - result of operation
 */
zb_uint8_t zb_uint48_mul(const zb_uint48_t *f, const zb_uint48_t *s, zb_uint48_t *r)
{

    zb_uint64_t temp;

    temp = (zb_uint64_t)zb_uint48_to_int64(f) * (zb_uint64_t)zb_uint48_to_int64(s);

    if ((temp >> 48) != 0U)
    {
        return ZB_MATH_OVERFLOW;
    }

    zb_uint64_to_uint48(temp, r);

    return ZB_MATH_OK;
}


/**
 * @f - first operand
 * @s - second operand
 * @r - result of operation
 */
zb_uint8_t zb_int48_mul(const zb_int48_t *f, const zb_int48_t *s, zb_int48_t *r)
{

    zb_int64_t temp;

    temp = (zb_int64_t)zb_int48_to_int64(f) * (zb_int64_t)zb_int48_to_int64(s);

    if ((temp < MIN_SIGNED_48BIT_VAL) || (temp > MAX_SIGNED_48BIT_VAL))
    {
        return ZB_MATH_OVERFLOW;
    }

    zb_int64_to_int48(temp, r);

    return ZB_MATH_OK;
}


/**
 * @f - first operand
 * @s - second operand
 * @r - result of operation
 */
zb_uint8_t zb_uint48_div(const zb_uint48_t *f, const zb_uint48_t *s, zb_uint48_t *r)
{

    zb_int64_t temp;

    temp = (zb_int64_t)zb_uint48_to_int64(f) / (zb_int64_t)zb_uint48_to_int64(s);

    zb_int64_to_uint48(temp, r);

    return ZB_MATH_OK;
}


/**
 * @f - first operand
 * @s - second operand
 * @r - result of operation
 */
zb_uint8_t zb_int48_div(const zb_int48_t *f, const zb_int48_t *s, zb_int48_t *r)
{

    zb_int64_t temp;

    temp = (zb_int64_t)zb_int48_to_int64(f) / (zb_int64_t)zb_int48_to_int64(s);

    zb_int64_to_int48(temp, r);

    return ZB_MATH_OK;
}


/**
 * @f - first operand
 * @s - second operand
 * @r - result of operation
 */
zb_uint8_t zb_uint48_mod(const zb_uint48_t *f, const zb_uint48_t *s, zb_uint48_t *r)
{

    zb_uint64_t temp;

    temp = (zb_uint64_t)zb_uint48_to_int64(f) % (zb_uint64_t)zb_uint48_to_int64(s);

    zb_uint64_to_uint48(temp, r);

    return ZB_MATH_OK;
}


/**
 * @f - first operand
 * @s - second operand
 * @r - result of operation
 */
zb_uint8_t zb_int48_mod(const zb_int48_t *f, const zb_int48_t *s, zb_int48_t *r)
{

    zb_int64_t temp;

    temp = (zb_int64_t)zb_int48_to_int64(f) % (zb_int64_t)zb_int48_to_int64(s);

    zb_int64_to_int48(temp, r);

    return ZB_MATH_OK;
}

#endif /* ZB_UINT24_48_SUPPORT */
