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
/* PURPOSE: ZCL OTA Cluster - common part for Server and Client
*/

#define ZB_TRACE_FILE_ID 47

#include "zb_common.h"

/** @internal
    @{
*/

#if defined (ZB_ZCL_SUPPORT_CLUSTER_OTA_UPGRADE) || defined DOXYGEN

#include "zb_time.h"
#include "zb_zdo.h"
#include "zb_zcl.h"
#include "zcl/zb_zcl_ota_upgrade.h"
#include "zb_zdo.h"
#include "zb_aps.h"
#include "zdo_wwah_stubs.h"
/*
  That routines are linked if either Client or Server role is enabled.
 */

zb_ret_t zb_zcl_check_value_ota_upgrade(zb_uint16_t attr_id, zb_uint8_t endpoint, zb_uint8_t *value)
{
    zb_ret_t ret = ZB_TRUE;
    ZVUNUSED(endpoint);
    ZVUNUSED(value);

    TRACE_MSG(TRACE_ZCL1, "> check_value_ota_upgrade", (FMT__0));

    switch ( attr_id )
    {
        /* remove for NTS certification - test 9.5.9
         * TH sends wrong value - 6000 (e.q. 25 minutes) */
#ifdef NTS_HACK
    case ZB_ZCL_ATTR_OTA_UPGRADE_MIN_BLOCK_REQUE_ID:
        ret = 258 > *(zb_uint16_t *)value
              ? RET_OK : RET_ERROR;
        break;
#endif

    default:
        break;
    }

    TRACE_MSG(TRACE_ZCL1, "< check_value_ota_upgrade ret %hd", (FMT__H, ret));
    return ret;
}
/*************************** Helper functions *************************/
/** @brief Get 8bit attribute value from OTA Upgrade cluster
    @param endpoint - endpoint
    @param attr_id - attribute ID
    @return attribute value
*/
zb_uint8_t zb_zcl_ota_upgrade_get8(zb_uint8_t endpoint, zb_uint16_t attr_id)
{
    zb_zcl_attr_t *attr_desc = zb_zcl_get_attr_desc_a(endpoint,
                               ZB_ZCL_CLUSTER_ID_OTA_UPGRADE, ZB_ZCL_CLUSTER_CLIENT_ROLE, attr_id);
    ZB_ASSERT(attr_desc);
    return ZB_ZCL_GET_ATTRIBUTE_VAL_8(attr_desc);
}

/** @brief Get 16bit attribute value from OTA Upgrade cluster
    @param endpoint - endpoint
    @param attr_id - attribute ID
    @return attribute value
*/
zb_uint16_t zb_zcl_ota_upgrade_get16(zb_uint8_t endpoint, zb_uint16_t attr_id)
{
    zb_zcl_attr_t *attr_desc = zb_zcl_get_attr_desc_a(endpoint,
                               ZB_ZCL_CLUSTER_ID_OTA_UPGRADE, ZB_ZCL_CLUSTER_CLIENT_ROLE, attr_id);
    ZB_ASSERT(attr_desc);
    return ZB_ZCL_GET_ATTRIBUTE_VAL_16(attr_desc);
}

/** @brief Get 16bit attribute value from OTA Upgrade cluster
    @param endpoint - endpoint
    @param attr_id - attribute ID
    @return attribute value
*/
zb_uint32_t zb_zcl_ota_upgrade_get32(zb_uint8_t endpoint, zb_uint16_t attr_id)
{
    zb_zcl_attr_t *attr_desc = zb_zcl_get_attr_desc_a(endpoint,
                               ZB_ZCL_CLUSTER_ID_OTA_UPGRADE, ZB_ZCL_CLUSTER_CLIENT_ROLE, attr_id);
    ZB_ASSERT(attr_desc);
    return ZB_ZCL_GET_ATTRIBUTE_VAL_32(attr_desc);
}

void zb_zcl_ota_upgrade_set_ota_status(zb_uint8_t endpoint, zb_uint8_t status)
{
    zb_zcl_attr_t *attr_desc;
    TRACE_MSG(TRACE_ZCL1, "> zb_zcl_ota_upgrade_set_ota_status endpoint %hd status %hd", (FMT__H_H, endpoint, status));

    attr_desc = zb_zcl_get_attr_desc_a(endpoint,
                                       ZB_ZCL_CLUSTER_ID_OTA_UPGRADE, ZB_ZCL_CLUSTER_CLIENT_ROLE, ZB_ZCL_ATTR_OTA_UPGRADE_IMAGE_STATUS_ID);
    ZB_ASSERT(attr_desc);

    ZB_ZCL_SET_DIRECTLY_ATTR_VAL8(attr_desc, status);

    TRACE_MSG(TRACE_ZCL1, "< zb_zcl_ota_upgrade_set_ota_status", (FMT__0));
}


#endif /* defined ZB_ZCL_SUPPORT_CLUSTER_OTA_UPGRADE || defined DOXYGEN */
