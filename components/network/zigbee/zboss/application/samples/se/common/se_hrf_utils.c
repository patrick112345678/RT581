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
/* PURPOSE: human-readable formatting utility functions
*/

#define ZB_TRACE_FILE_ID 40024
#include "zboss_api.h"
#include "se_hrf_utils.h"


const zb_char_t *se_hrf_metering_device_type(zb_uint8_t val)
{
    switch (val)
    {
    case ZB_ZCL_METERING_ELECTRIC_METERING:
        return "Electric meter";

    case ZB_ZCL_METERING_GAS_METERING:
        return "Gas meter";

    case ZB_ZCL_METERING_WATER_METERING:
        return "Water meter";

    case ZB_ZCL_METERING_PRESSURE_METERING:
        return "Pressure meter";

    case ZB_ZCL_METERING_HEAT_METERING:
        return "Heat meter";

    case ZB_ZCL_METERING_COOLING_METERING:
        return "Cooling meter";

    case ZB_ZCL_METERING_MIRRORED_GAS_METERING:
        return "Gas meter (mirrored)";

    case ZB_ZCL_METERING_MIRRORED_WATER_METERING:
        return "Water meter (mirrored)";

    case ZB_ZCL_METERING_MIRRORED_PRESSURE_METERING:
        return "Pressure meter (mirrored)";

    case ZB_ZCL_METERING_MIRRORED_HEAT_METERING:
        return "Heat meter (mirrored)";

    case ZB_ZCL_METERING_MIRRORED_COOLING_METERING:
        return "Cooling meter (mirrored)";

    case ZB_ZCL_METERING_RESERVED:
    default:
        return "??";
    }
}


const zb_char_t *se_hrf_metering_unit_of_measure(zb_uint8_t val)
{

    switch (val)
    {
    case ZB_ZCL_METERING_UNIT_KW_KWH_BINARY:
    case ZB_ZCL_METERING_UNIT_KW_KWH_BCD:
        return "kWh";

    case ZB_ZCL_METERING_UNIT_M3_M3H_BINARY:
    case ZB_ZCL_METERING_UNIT_M3_M3H_BCD:
        return "m3";

    case ZB_ZCL_METERING_UNIT_FT3_FT3H_BINARY:
    case ZB_ZCL_METERING_UNIT_FT3_FT3H_BCD:
        return "ft3";

    case ZB_ZCL_METERING_UNIT_CCF_CCFH_BINARY:
    case ZB_ZCL_METERING_UNIT_CCF_CCFH_BCD:
        return "ccf";

    case ZB_ZCL_METERING_UNIT_USGL_USGLH_BINARY:
    case ZB_ZCL_METERING_UNIT_USGL_USGLH_BCD:
        return "gl (US)";

    case ZB_ZCL_METERING_UNIT_IMPGL_IMPGLH_BINARY:
    case ZB_ZCL_METERING_UNIT_IMPGL_IMPGLH_BCD:
        return "gl (IMP)";

    case ZB_ZCL_METERING_UNIT_BTU_BTUH_BINARY:
    case ZB_ZCL_METERING_UNIT_BTU_BTUH_BCD:
        return "BTU";

    case ZB_ZCL_METERING_UNIT_L_LH_BINARY:
    case ZB_ZCL_METERING_UNIT_L_LH_BCD:
        return "l";

    case ZB_ZCL_METERING_UNIT_KPAG_BINARY:
    case ZB_ZCL_METERING_UNIT_KPAG_BCD:
        return "kPA (gauge)";

    case ZB_ZCL_METERING_UNIT_KPAA_BINARY:
    case ZB_ZCL_METERING_UNIT_KPAA_BCD:
        return "kPA";

    case ZB_ZCL_METERING_UNIT_MCF_MCFH_BINARY:
    case ZB_ZCL_METERING_UNIT_MCF_MCFH_BCD:
        return "mcf";

    case ZB_ZCL_METERING_UNIT_UNITLESS_BINARY:
    case ZB_ZCL_METERING_UNIT_UNITLESS_BCD:
        return "units";

    case ZB_ZCL_METERING_UNIT_MJ_MJS_BINARY:
    case ZB_ZCL_METERING_UNIT_MJ_MJS_BCD:
        return "MJ";

    case ZB_ZCL_METERING_UNIT_BINARY_RESERVED:
    case ZB_ZCL_METERING_UNIT_BCD_RESERVED:
    default:
        return "??";
    }
}

zb_char_t *se_hrf_metering_format_uint48_value(zb_uint48_t value,
        zb_uint8_t fmt_left,
        zb_uint8_t fmt_right,
        zb_uint8_t fmt_suppr,
        zb_char_t *buf,
        zb_uint8_t buflen)
{
    zb_uint64_t val;
    zb_char_t *p;
    zb_uint8_t n, total;

    if (buflen < SE_HRF_UINT48_BUF_LEN)
    {
        return NULL;
    }

    val = (zb_uint64_t)value.low | (zb_uint64_t)value.high << 32;

    p = buf + SE_HRF_MAX_UINT48_DIGITS + 1;
    total = fmt_left + fmt_right;
    n = 0;
    *p-- = '\0';

    while (n != total)
    {
        /* print immediate digit (or leading zero) */
        *p-- = (val % 10) + '0';
        val /= 10;
        n++;

        /* put decimal point */
        if (n == fmt_right)
        {
            *p-- = '.';
        }

        /* stop printing if val is exhausted and no leading zeroes desired */
        if (val == 0 && fmt_suppr)
        {
            break;
        }
    }

    return ++p;
}

const char *se_hrf_format_currency(zb_uint16_t currency)
{
    if (currency == 840)
    {
        return "USD";
    }
    return "XST";
}

zb_char_t *se_hrf_format_price(zb_uint32_t raw_price,
                               zb_uint16_t currency,
                               zb_uint8_t  trailing_digit,
                               zb_uint8_t  unit,
                               zb_char_t  *buf,
                               zb_uint8_t  buflen)
{
    const zb_char_t *unit_str;
    const zb_char_t *value_str;
    const zb_char_t *currency_str;

    zb_uint8_t  max_uint32_len = 10;
    zb_uint8_t  total = max_uint32_len + trailing_digit + 1;

    zb_uint48_t u48_value = { .low = raw_price };
    zb_char_t   u48_buf[SE_HRF_PRICE_BUF_LEN] = {0};

    ZB_ASSERT(trailing_digit < max_uint32_len);

    unit_str = se_hrf_metering_unit_of_measure(unit);
    value_str = se_hrf_metering_format_uint48_value(u48_value, max_uint32_len, trailing_digit, 0, u48_buf, sizeof(u48_buf));
    currency_str = se_hrf_format_currency(currency);

    ZB_ASSERT(strlen(value_str) == (total));

    ZB_BZERO(buf, buflen);
    strcat(buf, value_str); /* 10 + 1 + trailing_digit */
    strcat(buf, " "); /* 1 */
    strcat(buf, currency_str); /* 3 */
    strcat(buf, "/"); /* 1 */
    strcat(buf, unit_str); /* 20 */

    return buf;
}
