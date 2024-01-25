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
/*  PURPOSE: Green Power Cluster helper
*/

#define ZB_TRACE_FILE_ID 22106

#include "zb_common.h"

#if defined ZB_ENABLE_ZGP_CLUSTER
#include "zgp/zgp_internal.h"

/**
 * @brief Perform send general zcl read attributes command for ZGP clister
 *
 * @param buf_ref        [in]  Buffer reference
 * @param dst_addr       [in]  Destination address
 * @param dst_addr_mode  [in]  Destination address mode
 * @param attr_ids       [in]  Attribute IDs list
 * @param attr_cnt       [in]  Attribute IDs list size
 * @param def_resp       [in]  Enable ZCL default response if TRUE
 * @param cb             [in]  Call callback if needed after sending request
 *
 */
void zgp_cluster_read_attrs(zb_uint8_t     buf_ref,
                            zb_uint16_t    dst_addr,
                            zb_uint8_t     dst_addr_mode,
                            zb_uint16_t   *attr_ids,
                            zb_uint8_t     attr_cnt,
                            zb_uint8_t     dir,
                            zb_uint8_t     def_resp,
                            zb_callback_t  cb)
{
    zb_uint8_t *cmd_ptr;
    zb_uint8_t  i;

    ZB_ASSERT(attr_cnt >= 1);

    ZB_ZCL_GENERAL_INIT_READ_ATTR_REQ_A(buf_ref, cmd_ptr, dir, def_resp);

    for (i = 0; i < attr_cnt; i++)
    {
        ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, attr_ids[i]);
    }

    ZB_ZCL_GENERAL_SEND_READ_ATTR_REQ(buf_ref, cmd_ptr, dst_addr, dst_addr_mode, ZGP_ENDPOINT,
                                      ZGP_ENDPOINT, ZB_AF_GP_PROFILE_ID,
                                      ZB_ZCL_CLUSTER_ID_GREEN_POWER, cb);
}

void zgp_cluster_read_attr(zb_uint8_t     buf_ref,
                           zb_uint16_t    dst_addr,
                           zb_uint8_t     dst_addr_mode,
                           zb_uint16_t    attr_id,
                           zb_uint8_t     dir,
                           zb_uint8_t     def_resp,
                           zb_callback_t  cb)
{
    zgp_cluster_read_attrs(buf_ref, dst_addr, dst_addr_mode, &attr_id, 1, dir, def_resp, cb);
}

void zgp_cluster_write_attr(zb_uint8_t     buf_ref,
                            zb_uint16_t    dst_addr,
                            zb_uint8_t     dst_addr_mode,
                            zb_uint16_t    attr_id,
                            zb_uint8_t     attr_type,
                            zb_uint8_t    *attr_val,
                            zb_uint8_t     dir,
                            zb_uint8_t     def_resp,
                            zb_callback_t  cb)
{
    zb_uint8_t *cmd_ptr;

    ZB_ZCL_GENERAL_INIT_WRITE_ATTR_REQ_A(buf_ref, cmd_ptr, dir, def_resp);

    ZB_ZCL_GENERAL_ADD_VALUE_WRITE_ATTR_REQ(cmd_ptr, attr_id, attr_type, attr_val);

    ZB_ZCL_GENERAL_SEND_WRITE_ATTR_REQ(buf_ref, cmd_ptr, dst_addr, dst_addr_mode, ZGP_ENDPOINT,
                                       ZGP_ENDPOINT, ZB_AF_GP_PROFILE_ID,
                                       ZB_ZCL_CLUSTER_ID_GREEN_POWER, cb);
}

#endif  /* ZB_ENABLE_ZGP_CLUSTER */
