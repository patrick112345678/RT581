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
/* PURPOSE: ZCL WWAH cluster specific commands handling
*/

#define ZB_TRACE_FILE_ID 12082

#include "zb_common.h"

#if (defined ZB_ZCL_ENABLE_WWAH_SERVER || defined ZB_ZCL_ENABLE_WWAH_CLIENT)

void zb_zcl_wwah_set_wwah_behavior(zb_uint8_t behavior)
{
    if (behavior < ZB_ZCL_WWAH_BEHAVIOR_RESERVED)
    {
        WWAH_CTX().wwah_behavior = behavior;
    }
}

/*
 * Check if WWAH client (TC) behaviour is enabled
 * @return ZB_TRUE if WWAH client (TC) behaviour is enabled and WWAH client endpoint is exist
 *         ZB_FALSE otherwise */

zb_bool_t zb_is_wwah_client()
{
    zb_uint8_t wwah_endpoint = get_endpoint_by_cluster(ZB_ZCL_CLUSTER_ID_WWAH,
                               ZB_ZCL_CLUSTER_CLIENT_ROLE);
    return (zb_bool_t)(((zb_zcl_wwah_behavior_t)WWAH_CTX().wwah_behavior == ZB_ZCL_WWAH_BEHAVIOR_CLIENT) && wwah_endpoint);
}

/*
 * Check if WWAH server behaviour is enabled
 * @return ZB_TRUE if WWAH server behaviour is enabled and WWAH server endpoint is exist
 *         ZB_FALSE otherwise */

zb_bool_t zb_is_wwah_server()
{
    zb_uint8_t wwah_endpoint = get_endpoint_by_cluster(ZB_ZCL_CLUSTER_ID_WWAH,
                               ZB_ZCL_CLUSTER_SERVER_ROLE);
    return (zb_bool_t)(((zb_zcl_wwah_behavior_t)WWAH_CTX().wwah_behavior == ZB_ZCL_WWAH_BEHAVIOR_SERVER) && wwah_endpoint);
}

#endif
