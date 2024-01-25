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
/* PURPOSE: General IAS zone device: battery monitoring API
*/

#define ZB_TRACE_FILE_ID 63294
#include "izs_device.h"

/*** Global variables declaration ***/
IZS_DECLARE_AVG_DATA_VAR(izs_voltage_data_t, g_loaded_battery_voltage);
IZS_DECLARE_AVG_DATA_VAR(izs_voltage_data_t, g_unloaded_battery_voltage);

/*
  Radio module battery volume measurement

  Battery volume for loaded and unloaded radio is read using:
  void izs_read_loaded_battery_voltage()
  void izs_read_unloaded_battery_voltage()
  Read voltage is not returned to a caller, but it is stored in the
  internal array. Once per IZS_BATTERY_MONITORING_ARRAY_SIZE read
  operations average value is calculated for loaded and unloaded
  voltage. In that time Power Configuration attribute value is updated
  and all checks related to IAS Zone status are performed.
*/

static void radio_battery_check_avg(void)
{
    TRACE_MSG(TRACE_BATTERY1, ">> radio_battery_check_avg", (FMT__0));

    TRACE_MSG(TRACE_BATTERY1, "avg Loaded Battery value %d", (FMT__D, g_loaded_battery_voltage.avg_val));
    /* save to Power configuration cluster loaded battery value */
    izs_set_battery_voltage(g_loaded_battery_voltage.avg_val);

    if (g_unloaded_battery_voltage.avg_valid)
    {
        TRACE_MSG(TRACE_BATTERY1, "avg Unloaded Battery value %d",
                  (FMT__D, g_unloaded_battery_voltage.avg_val));

        izs_check_battery_defect(
            g_loaded_battery_voltage.avg_val, g_unloaded_battery_voltage.avg_val);
    }

    TRACE_MSG(TRACE_BATTERY1, "<< radio_battery_check_avg", (FMT__0));
}

/************* loaded measurements *****************/

void izs_read_loaded_battery_voltage(zb_uint8_t param)
{
    zb_uint16_t voltage = 0; // avoid warning
    zb_uint16_t adjusted_voltage = 0;
    zb_bool_t valid;

    TRACE_MSG(TRACE_BATTERY1, ">> radio_read_loaded_battery_voltage", (FMT__0));

    ZVUNUSED(param);

    valid = (zb_bool_t)IZS_GET_LOADED_BATTERY_VOLT(&voltage);

    if (valid)
    {
        /* Assume voltage is in mV (1V == 1000 mV). */
        adjusted_voltage = IZS_CONVERT_BATTERY_VOLT(voltage);

        TRACE_MSG(TRACE_BATTERY1, "adjusted_voltage %d", (FMT__D, adjusted_voltage));

        if (g_device_ctx.pwr_cfg_attr.voltage == IZS_INIT_BATTERY_VOLTAGE)
        {
            g_loaded_battery_voltage.avg_val = adjusted_voltage;
        }

        IZS_AVG_VAL_PUT_VALUE(&g_loaded_battery_voltage, adjusted_voltage);

        if (g_loaded_battery_voltage.avg_valid == ZB_TRUE)
        {
            radio_battery_check_avg();
            g_loaded_battery_voltage.avg_valid = ZB_FALSE;
        }

        if (g_device_ctx.pwr_cfg_attr.voltage == IZS_INIT_BATTERY_VOLTAGE)
        {
            /* If avg value was not calculated yet - set just measured value */
            izs_set_battery_voltage(adjusted_voltage);
        }
    }
    TRACE_MSG(TRACE_BATTERY1, "<< radio_read_loaded_battery_voltage", (FMT__0));
}

/************* unloaded measurements *****************/

void izs_read_unloaded_battery_voltage(zb_uint8_t param)
{
    zb_uint16_t voltage = 0;
    zb_uint16_t adjusted_voltage = 0;
    zb_bool_t valid;

    TRACE_MSG(TRACE_BATTERY1, ">> izs_read_unloaded_battery_voltage", (FMT__0));

    ZVUNUSED(param);
    valid = (zb_bool_t)IZS_GET_UNLOADED_BATTERY_VOLT(&voltage);

    if (valid)
    {
        /* Assume voltage is in mV (1V == 1000 mV). */
        adjusted_voltage = IZS_CONVERT_BATTERY_VOLT(voltage);

        TRACE_MSG(TRACE_BATTERY1, "adjusted_voltage %d", (FMT__D, adjusted_voltage));
        IZS_AVG_VAL_PUT_VALUE(&g_unloaded_battery_voltage, adjusted_voltage);

        /* Average value is handled in radio_battery_check_avg(), it
         * is called while Loaded battery volume is read */
    }

    TRACE_MSG(TRACE_BATTERY1, "<< izs_read_unloaded_battery_voltage", (FMT__0));
}

zb_bool_t izs_voltage_under_threshold(zb_uint16_t voltage, zb_uint16_t threshold, zb_bool_t is_alarm_set)
{
    zb_bool_t ret = ZB_FALSE;

    TRACE_MSG(TRACE_ZCL1, "> izs_voltage_under_threshold voltage %hd threshold %hd alarm_set %hd",
              (FMT__D_D_H, voltage, threshold, is_alarm_set));

    if (threshold != 0)
    {
        /* Implement comparison taking into account hysteresis
         *
         * if alarm set then alarm will be clean when voltage cross high level
         * if alarm not set then alarm will be set when voltage cross low level */
        if (is_alarm_set)
        {
            ret = (voltage <= threshold + IZS_BATTERY_THRESHOLD_DELTA) ? ZB_TRUE : ZB_FALSE;
        }
        else
        {
            ret = (voltage < threshold) ? ZB_TRUE : ZB_FALSE;
        }
    }

    TRACE_MSG(TRACE_ZCL1, "< izs_voltage_under_threshold ret %hd",
              (FMT__H, ret));
    return ret;
}

static void izs_check_voltage_value(zb_uint16_t voltage)
{
    zb_uint8_t conv_voltage;

    TRACE_MSG(TRACE_ZCL1, "> izs_check_voltage_value voltage %d",
              (FMT__D, voltage));

    /* ZCL value is in 0.1V */
    conv_voltage = (zb_uint8_t)((IZS_CONVERT_BATTERY_VOLT(voltage) + 50) / 100); /* + 50 to apply
                                                                                * rounding */
    if (conv_voltage != g_device_ctx.pwr_cfg_attr.voltage)
    {
        /* set battery voltage value */
        ZB_ZCL_SET_ATTRIBUTE(
            IZS_DEVICE_ENDPOINT,
            ZB_ZCL_CLUSTER_ID_POWER_CONFIG,
            ZB_ZCL_CLUSTER_SERVER_ROLE,
            ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_VOLTAGE_ID,
            (zb_uint8_t *)&conv_voltage,
            ZB_FALSE /* do not check access */
        );
    }

    TRACE_MSG(TRACE_ZCL1, "< izs_check_voltage_value", (FMT__0));
}

void izs_set_battery_voltage(zb_uint16_t voltage)
{
    zb_uint32_t alarm_state;

    TRACE_MSG(TRACE_BATTERY1, "> izs_set_battery_voltage voltage %d", (FMT__H, voltage));

    /* write attribute directly, set alarm bits and send alarm if necessary  */
    izs_check_voltage_value(voltage);

    alarm_state = g_device_ctx.pwr_cfg_attr.alarm_state;

    if (alarm_state & ZB_ZCL_POWER_CONFIG_BATTERY_ALARM_STATE_SOURCE1_VOLTAGE3)
    {
        TRACE_MSG(TRACE_BATTERY1, "battery alarm state: min threshold", (FMT__0));
        g_device_ctx.battery_low = ZB_TRUE;
    }
    else
    {
        g_device_ctx.battery_low = ZB_FALSE;
    }

    izs_update_ias_zone_status(ZB_ZCL_IAS_ZONE_ZONE_STATUS_BATTERY, g_device_ctx.battery_low);

    TRACE_MSG(TRACE_BATTERY1, "< izs_set_battery_voltage", (FMT__0));
}


void izs_check_battery_defect(zb_uint16_t avg_loaded, zb_uint16_t avg_unloaded)
{
    zb_uint16_t delta;
    zb_uint16_t zone_status;

    TRACE_MSG(TRACE_BATTERY1, "> izs_check_battery_defect loaded val %d unloaded val %d",
              (FMT__D_D, avg_loaded, avg_unloaded));

    /* The unloaded battery voltage should be higher than loaded battery voltage.
       If not the case, the unloaded battert voltage needs to be updated. */
    if (avg_unloaded < avg_loaded)
    {
        return;
    }

    delta = ZB_ABS(avg_unloaded - avg_loaded);

    zone_status = IZS_DEVICE_GET_ZONE_STATUS();

    if (delta > IZS_BATTERY_DEFECT_DELTA)
    {
        TRACE_MSG(TRACE_BATTERY1, "battery_defect detected!", (FMT__0));
        zone_status |= ZB_ZCL_IAS_ZONE_ZONE_STATUS_BATTERY_DEFECT;
    }
    else
    {
        zone_status &= ~ZB_ZCL_IAS_ZONE_ZONE_STATUS_BATTERY_DEFECT;
    }

    if (zone_status != IZS_DEVICE_GET_ZONE_STATUS())
    {
        /* Send notification only in case zone status is changed */
        izs_ias_zone_queue_put(zone_status);
        izs_send_notification();
    }

    TRACE_MSG(TRACE_BATTERY1, "< izs_check_battery_defect", (FMT__0));
}
