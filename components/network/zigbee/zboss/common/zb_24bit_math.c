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

#define ZB_TRACE_FILE_ID 12299


#include "zb_common.h"
#include "zb_types.h"

#ifdef ZB_UINT24_48_SUPPORT

#ifdef DEBUG

/* enpty lines to fix ZB_ASSERT_COMPILE_DECL implementation...
 */
ZB_ASSERT_COMPILE_DECL(sizeof(zb_uint24_t) == sizeof(zb_int24_t));
ZB_ASSERT_COMPILE_DECL(sizeof(zb_uint24_t) == ZB_24BIT_SIZE);

#endif /* DEBUG */


void zb_uint32_to_uint24(zb_uint32_t var, zb_uint24_t *res)
{
    res->low = (zb_uint16_t)var;
    res->high = (zb_uint8_t)(var >> 16);
}


/**
 * @f - first operand
 * @s - second operand
 * @r - result of operation
 */
zb_uint8_t zb_uint24_add(const zb_uint24_t *f, const zb_uint24_t *s, zb_uint24_t *r)
{
    zb_uint32_t temp = (zb_uint32_t)(f->low) + (zb_uint32_t)(s->low);

    temp += ((zb_uint32_t)f->high + (zb_uint32_t)s->high) << 16;

    if (((temp >> 24) & 1U) != 0U)
    {
        return ZB_MATH_OVERFLOW;
    }

    zb_uint32_to_uint24(temp, r);

    return ZB_MATH_OK;
}


zb_int32_t zb_int24_to_int32(const zb_int24_t *var)
{
    zb_uint32_t res = 0;

    res |= var->low;
    res |= ((zb_uint32_t)var->high << 16);

    if (((zb_uint16_t)var->high & 0x80U) != 0U)
    {
        res |= 0xFF000000U;
    }

    return (zb_int32_t)res;
}


void zb_int32_to_int24(zb_int32_t var, zb_int24_t *res)
{
    if ((var <= MAX_SIGNED_24BIT_VAL) && (var >= MIN_SIGNED_24BIT_VAL))
    {
        zb_uint24_t temp;

        temp.low = (zb_uint16_t)var;
        temp.high = (zb_uint8_t)((zb_uint32_t)var >> 16);

        res->low = temp.low;
        res->high = (zb_int8_t)temp.high;
    }
    else
    {
        /**
         * The variable type zb_int32_t can be out of the bounds
         */
        zb_uint32_t min_max_value = (zb_uint32_t)((var > MAX_SIGNED_24BIT_VAL) ? MAX_SIGNED_24BIT_VAL : MIN_SIGNED_24BIT_VAL);
        zb_uint8_t temp_high;

        res->low = (zb_uint16_t)min_max_value;

        temp_high = (zb_uint8_t)(min_max_value >> 16);
        res->high = (zb_int8_t)temp_high;
    }
}


/**
 * @f - first operand
 * @s - second operand
 * @r - result of operation
 */
zb_uint8_t zb_int24_add(const zb_int24_t *f, const zb_int24_t *s, zb_int24_t *r)
{

    zb_int32_t temp;

    temp = zb_int24_to_int32(f) + zb_int24_to_int32(s);

    if ((temp < MIN_SIGNED_24BIT_VAL) || (temp > MAX_SIGNED_24BIT_VAL))
    {
        return ZB_MATH_OVERFLOW;
    }

    zb_int32_to_int24(temp, r);

    return ZB_MATH_OK;
}


/**
 * @f - first operand
 * @s - second operand
 * @r - result of operation
 */
zb_uint8_t zb_int24_sub(const zb_int24_t *f, const zb_int24_t *s, zb_int24_t *r)
{

    zb_int32_t temp;

    /*TODO: check cases x1 - x2, (-x1) - (-x2), (-x1) - (x2), (x1) - (-x1) */
    temp = zb_int24_to_int32(f) - zb_int24_to_int32(s);

    if ((temp < MIN_SIGNED_24BIT_VAL) || (temp > MAX_SIGNED_24BIT_VAL))
    {
        return ZB_MATH_OVERFLOW;
    }

    zb_int32_to_int24(temp, r);

    return ZB_MATH_OK;
}


void zb_int32_to_uint24(zb_int32_t var, zb_uint24_t *res)
{
    if (var <= (zb_int32_t)MAX_UNSIGNED_24BIT_VAL && (var >= 0))
    {
        res->low = (zb_uint16_t)var;
        res->high = (zb_uint8_t)((zb_uint32_t)var >> 16);
    }
    else
    {
        /**
         * The variable type zb_int32_t can be out of the bounds
         */
        zb_uint32_t min_max_value = (var < 0) ? 0U : MAX_UNSIGNED_24BIT_VAL;
        zb_uint8_t temp_high;

        res->low = (zb_uint16_t)min_max_value;

        temp_high = (zb_uint8_t)(min_max_value >> 16);
        res->high = temp_high;
    }
}


zb_int32_t zb_uint24_to_int32(const zb_uint24_t *var)
{
    zb_uint32_t res = 0;

    res |= var->low;
    res |= ((zb_uint32_t)var->high << 16);

    return (zb_int32_t)res;
}


/**
 * @f - first operand
 * @s - second operand
 * @r - result of operation
 */
zb_uint8_t zb_uint24_sub(const zb_uint24_t *f, const zb_uint24_t *s, zb_uint24_t *r)
{
    zb_uint8_t ret = ZB_MATH_OK;
    zb_int32_t temp;
    zb_int32_t temp_f = (zb_int32_t)zb_uint24_to_int32(f);
    zb_int32_t temp_s = (zb_int32_t)zb_uint24_to_int32(s);

    if (temp_f >= temp_s)
    {
        temp = temp_f - temp_s;
        zb_int32_to_uint24(temp, r);
    }
    else
    {
        ret = ZB_MATH_OVERFLOW;
    }

    return ret;
}


/**
 * @f - first operand
 * @s - second operand
 * @r - result of operation
 */
zb_uint8_t zb_int24_neg(const zb_int24_t *f, zb_int24_t *r)
{
    zb_int32_t temp;

    zb_uint32_t u32_temp = (zb_uint32_t)zb_int24_to_int32(f);
    u32_temp = ~(u32_temp) + 1U;

    temp = (zb_int32_t)u32_temp;

    zb_int32_to_int24(temp, r);

    return ZB_MATH_OK;
}


/**
 * @f - first operand
 * @s - second operand
 * @r - result of operation
 */
zb_uint8_t zb_uint24_mul(const zb_uint24_t *f, const zb_uint24_t *s, zb_uint24_t *r)
{

    zb_uint32_t temp;

    temp = (zb_uint32_t)zb_uint24_to_int32(f) * (zb_uint32_t)zb_uint24_to_int32(s);

    if ((temp >> 24) != 0U)
    {
        return ZB_MATH_OVERFLOW;
    }

    zb_uint32_to_uint24(temp, r);

    return ZB_MATH_OK;
}


/**
 * @f - first operand
 * @s - second operand
 * @r - result of operation
 */
zb_uint8_t zb_int24_mul(const zb_int24_t *f, const zb_int24_t *s, zb_int24_t *r)
{

    zb_int32_t temp;

    temp = (zb_int32_t)zb_int24_to_int32(f) * (zb_int32_t)zb_int24_to_int32(s);

    if ((temp < MIN_SIGNED_24BIT_VAL) || (temp > MAX_SIGNED_24BIT_VAL))
    {
        return ZB_MATH_OVERFLOW;
    }

    zb_int32_to_int24(temp, r);

    return ZB_MATH_OK;
}


/**
 * @f - first operand
 * @s - second operand
 * @r - result of operation
 */
zb_uint8_t zb_uint24_div(const zb_uint24_t *f, const zb_uint24_t *s, zb_uint24_t *r)
{

    zb_int32_t temp;

    temp = (zb_int32_t)zb_uint24_to_int32(f) / (zb_int32_t)zb_uint24_to_int32(s);

    zb_int32_to_uint24(temp, r);

    return ZB_MATH_OK;
}


/**
 * @f - first operand
 * @s - second operand
 * @r - result of operation
 */
zb_uint8_t zb_int24_div(const zb_int24_t *f, const zb_int24_t *s, zb_int24_t *r)
{

    zb_int32_t temp;

    temp = (zb_int32_t)zb_int24_to_int32(f) / (zb_int32_t)zb_int24_to_int32(s);

    zb_int32_to_int24(temp, r);

    return ZB_MATH_OK;
}


/**
 * @f - first operand
 * @s - second operand
 * @r - result of operation
 */
zb_uint8_t zb_uint24_mod(const zb_uint24_t *f, const zb_uint24_t *s, zb_uint24_t *r)
{

    zb_uint32_t temp;

    temp = (zb_uint32_t)zb_uint24_to_int32(f) % (zb_uint32_t)zb_uint24_to_int32(s);

    zb_uint32_to_uint24(temp, r);

    return ZB_MATH_OK;
}


/**
 * @f - first operand
 * @s - second operand
 * @r - result of operation
 */
zb_uint8_t zb_int24_mod(const zb_int24_t *f, const zb_int24_t *s, zb_int24_t *r)
{

    zb_int32_t temp;

    temp = (zb_int32_t)zb_int24_to_int32(f) % (zb_int32_t)zb_int24_to_int32(s);

    zb_int32_to_int24(temp, r);

    return ZB_MATH_OK;
}

#endif /* ZB_UINT24_48_SUPPORT */
