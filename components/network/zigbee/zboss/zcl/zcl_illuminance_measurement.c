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

#define ZB_TRACE_FILE_ID 79

#include "zb_common.h"

#if defined (ZB_ZCL_SUPPORT_CLUSTER_ILLUMINANCE_MEASUREMENT)

zb_ret_t check_value_illuminance_measurement_server(zb_uint16_t attr_id, zb_uint8_t endpoint, zb_uint8_t *value);

void zb_zcl_illuminance_measurement_init_server()
{
    zb_zcl_add_cluster_handlers(ZB_ZCL_CLUSTER_ID_ILLUMINANCE_MEASUREMENT,
                                ZB_ZCL_CLUSTER_SERVER_ROLE,
                                check_value_illuminance_measurement_server,
                                (zb_zcl_cluster_write_attr_hook_t)NULL,
                                (zb_zcl_cluster_handler_t)NULL);
}

void zb_zcl_illuminance_measurement_init_client()
{
    zb_zcl_add_cluster_handlers(ZB_ZCL_CLUSTER_ID_ILLUMINANCE_MEASUREMENT,
                                ZB_ZCL_CLUSTER_CLIENT_ROLE,
                                (zb_zcl_cluster_check_value_t)NULL,
                                (zb_zcl_cluster_write_attr_hook_t)NULL,
                                (zb_zcl_cluster_handler_t)NULL);
}

zb_ret_t check_value_illuminance_measurement_server(zb_uint16_t attr_id, zb_uint8_t endpoint, zb_uint8_t *value)
{
    zb_ret_t ret = RET_OK;

    TRACE_MSG(TRACE_ZCL1, "> check_value_illuminance_measurement", (FMT__0));

    switch ( attr_id )
    {
    case ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MEASURED_VALUE_ID:
    {
        zb_uint16_t val = ZB_ZCL_ATTR_GET16(value);

        if ( val == ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MEASURED_VALUE_TOO_LOW ||
                val == ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MEASURED_VALUE_INVALID)
        {
            ret = RET_OK;
        }
        else
        {
            zb_zcl_attr_t *attr_desc = zb_zcl_get_attr_desc_a(
                                           endpoint,
                                           ZB_ZCL_CLUSTER_ID_ILLUMINANCE_MEASUREMENT,
                                           ZB_ZCL_CLUSTER_SERVER_ROLE,
                                           ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MIN_MEASURED_VALUE_ID);

            ZB_ASSERT(attr_desc);

            ret = (ZB_ZCL_GET_ATTRIBUTE_VAL_16(attr_desc) ==
                   ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MIN_MEASURED_VALUE_UNDEFINED
                   || ZB_ZCL_GET_ATTRIBUTE_VAL_16(attr_desc) <= val)
                  ? RET_OK : RET_ERROR;

            if (ret != RET_OK)
            {
                attr_desc = zb_zcl_get_attr_desc_a(
                                endpoint,
                                ZB_ZCL_CLUSTER_ID_ILLUMINANCE_MEASUREMENT,
                                ZB_ZCL_CLUSTER_SERVER_ROLE,
                                ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MAX_MEASURED_VALUE_ID);

                ZB_ASSERT(attr_desc);

                ret = ZB_ZCL_GET_ATTRIBUTE_VAL_16(attr_desc) ==
                      ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MAX_MEASURED_VALUE_UNDEFINED ||
                      val <= ZB_ZCL_GET_ATTRIBUTE_VAL_16(attr_desc)
                      ? RET_OK : RET_ERROR;
            }
        }
    }
    break;

    case ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MIN_MEASURED_VALUE_ID:
        ret = (
                  ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MIN_MEASURED_VALUE_MIN_VALUE <= ZB_ZCL_ATTR_GET16(value) &&
                  (ZB_ZCL_ATTR_GET16(value) <= ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MIN_MEASURED_VALUE_MAX_VALUE) )
              ? RET_OK : RET_ERROR;

        if (ret != RET_OK)
        {
            zb_zcl_attr_t *attr_desc = zb_zcl_get_attr_desc_a(
                                           endpoint,
                                           ZB_ZCL_CLUSTER_ID_ILLUMINANCE_MEASUREMENT,
                                           ZB_ZCL_CLUSTER_SERVER_ROLE,
                                           ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MAX_MEASURED_VALUE_ID);

            ZB_ASSERT(attr_desc);

            ret = ZB_ZCL_GET_ATTRIBUTE_VAL_16(attr_desc) ==
                  ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MAX_MEASURED_VALUE_UNDEFINED ||
                  ZB_ZCL_ATTR_GET16(value) < ZB_ZCL_GET_ATTRIBUTE_VAL_16(attr_desc)
                  ? RET_OK : RET_ERROR;
        }
        break;

    case ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MAX_MEASURED_VALUE_ID:
        ret = ( (ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MAX_MEASURED_VALUE_MIN_VALUE <= ZB_ZCL_ATTR_GET16(value)) &&
                (ZB_ZCL_ATTR_GET16(value) <= ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MAX_MEASURED_VALUE_MAX_VALUE) )
              ? RET_OK : RET_ERROR;

        if (ret != RET_OK)
        {
            zb_zcl_attr_t *attr_desc = zb_zcl_get_attr_desc_a(
                                           endpoint,
                                           ZB_ZCL_CLUSTER_ID_ILLUMINANCE_MEASUREMENT,
                                           ZB_ZCL_CLUSTER_SERVER_ROLE,
                                           ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MIN_MEASURED_VALUE_ID);

            ZB_ASSERT(attr_desc);

            ret = (ZB_ZCL_GET_ATTRIBUTE_VAL_16(attr_desc) ==
                   ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MIN_MEASURED_VALUE_UNDEFINED
                   || ZB_ZCL_GET_ATTRIBUTE_VAL_16(attr_desc) < ZB_ZCL_ATTR_GET16(value))
                  ? RET_OK : RET_ERROR;
        }
        break;

    default:
        break;
    }

    TRACE_MSG(TRACE_ZCL1, "< check_value_illuminance_measurement ret %hd", (FMT__H, ret));
    return ret;
}
#endif  /* ZB_ZCL_SUPPORT_CLUSTER_ILLUMINANCE_MEASUREMENT */
