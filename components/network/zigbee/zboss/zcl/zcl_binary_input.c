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
/* PURPOSE: Binary input cluster implementation.
*/



#define ZB_TRACE_FILE_ID 85

#include "zb_common.h"

#if defined (ZB_ZCL_SUPPORT_CLUSTER_BINARY_INPUT)

#include "zboss_api.h"
#include "zcl/zb_zcl_binary_input.h"

static zb_ret_t check_value_binary_input_server(zb_uint16_t attr_id, zb_uint8_t endpoint, zb_uint8_t *value);

void zb_zcl_binary_input_init_server(void)
{
    zb_zcl_add_cluster_handlers(ZB_ZCL_CLUSTER_ID_BINARY_INPUT,
                                ZB_ZCL_CLUSTER_SERVER_ROLE,
                                check_value_binary_input_server,
                                (zb_zcl_cluster_write_attr_hook_t)NULL,
                                (zb_zcl_cluster_handler_t)NULL);
}


void zb_zcl_binary_input_init_client(void)
{
    zb_zcl_add_cluster_handlers(ZB_ZCL_CLUSTER_ID_BINARY_INPUT,
                                ZB_ZCL_CLUSTER_CLIENT_ROLE,
                                (zb_zcl_cluster_check_value_t)NULL,
                                (zb_zcl_cluster_write_attr_hook_t)NULL,
                                (zb_zcl_cluster_handler_t)NULL);
}

static zb_ret_t check_value_binary_input_server(zb_uint16_t attr_id, zb_uint8_t endpoint, zb_uint8_t *value)
{
    zb_ret_t ret = RET_OK;
    ZVUNUSED(endpoint);

    switch (attr_id)
    {
    case ZB_ZCL_ATTR_BINARY_INPUT_OUT_OF_SERVICE_ID:
    case ZB_ZCL_ATTR_BINARY_INPUT_PRESENT_VALUE_ID:
        ret = ZB_ZCL_CHECK_BOOL_VALUE(*value) ? RET_OK : RET_ERROR;
        break;

    case ZB_ZCL_ATTR_BINARY_INPUT_STATUS_FLAG_ID:
        if (*value > ZB_ZCL_BINARY_INPUT_STATUS_FLAG_MAX_VALUE)
        {
            ret = RET_ERROR;
        }
        break;

    default:
        break;
    }

    TRACE_MSG(TRACE_ZCL1, "check_value_binary_input ret %hd", (FMT__H, ret));
    return ret;
}
#endif /* ZB_ZCL_SUPPORT_CLUSTER_BINARY_INPUT */
