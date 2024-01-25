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
/* PURPOSE: ZDO response validation
*/

#define ZB_TRACE_FILE_ID 63

#ifndef ZB_LITE_NO_ZDO_RESPONSE_VALIDATION

#include "zb_common.h"
#include "zboss_api_zdo.h"

static zb_bool_t zdo_validate_nwk_addr_resp(zb_bufid_t buf)
{
    zb_zdo_nwk_addr_resp_head_t *addr_resp;
    zb_zdo_nwk_addr_resp_ext_t *ext_num;
    zb_size_t expected_len;
    zb_bool_t ret;

    expected_len = sizeof(*addr_resp);

    if (zb_buf_len(buf) == expected_len)
    {
        ret = ZB_TRUE;
    }
    else if (zb_buf_len(buf) >= (expected_len + sizeof(*ext_num)))
    {
        expected_len += sizeof(*ext_num);

        addr_resp = (zb_zdo_nwk_addr_resp_head_t *)zb_buf_begin(buf);
        /*cstat !MISRAC2012-Rule-11.3 */
        /** @mdr{00002,88} */
        ext_num = (zb_zdo_nwk_addr_resp_ext_t *)(addr_resp + 1);
        if (ext_num->num_assoc_dev > 0U)
        {
            expected_len += sizeof(zb_zdo_nwk_addr_resp_ext2_t)
                            + sizeof(zb_uint16_t) * ext_num->num_assoc_dev;
            ret = (zb_buf_len(buf) >= expected_len) ? ZB_TRUE : ZB_FALSE;
        }
        else
        {
            ret = ZB_TRUE;
        }
    }
    else
    {
        ret = ZB_FALSE;
    }

    return ret;
}

/* Validate fixed-size descriptors: Node_Desc_rsp, Power_Desc_rsp */
static zb_bool_t zdo_validate_desc_resp(zb_bufid_t buf, zb_uint8_t size)
{
    zb_zdo_desc_resp_hdr_t *hdr;
    zb_bool_t ret = ZB_FALSE;

    hdr = (zb_zdo_desc_resp_hdr_t *)(zb_buf_begin(buf));
    if (zb_buf_len(buf) >= sizeof(zb_zdo_desc_resp_hdr_t))
    {
        if (hdr->status == (zb_zdp_status_t)ZB_ZDP_STATUS_SUCCESS)
        {
            ret = (sizeof(zb_zdo_desc_resp_hdr_t) + zb_buf_len(buf)) >= size
                  ? ZB_TRUE : ZB_FALSE;
        }
        else
        {
            ret = ZB_TRUE;
        }
    }

    return ret;
}

/**
 * Simple_Desc_rsp, Active_EP_rsp, Match_Desc_rsp, Complex_Desc_rsp, User_Desc_rsp
 * share the same response format structure.
 */
static zb_bool_t zdo_validate_var_desc_resp(zb_bufid_t buf)
{
    zb_zdo_simple_desc_resp_hdr_t *hdr;
    zb_size_t expected_len = 0;
    zb_bool_t ret = ZB_FALSE;

    expected_len += sizeof(*hdr);
    hdr = (zb_zdo_simple_desc_resp_hdr_t *)(zb_buf_begin(buf));
    if (zb_buf_len(buf) >= expected_len)
    {
        expected_len += hdr->length;
        ret = zb_buf_len(buf) >= expected_len ? ZB_TRUE : ZB_FALSE;
    }

    return ret;
}


static zb_bool_t zdo_validate_parent_annce_resp(zb_bufid_t buf)
{
    zb_zdo_parent_annce_rsp_hdr_t *resp;
    zb_bool_t ret = ZB_FALSE;
    zb_size_t expected_len = 0;

    expected_len += sizeof(*resp);
    resp = (zb_zdo_parent_annce_rsp_hdr_t *) zb_buf_begin(buf);

    if (zb_buf_len(buf) >= expected_len)
    {
        expected_len += resp->num_of_children * sizeof(zb_ieee_addr_t);
        ret = zb_buf_len(buf) >= expected_len ? ZB_TRUE : ZB_FALSE;
    }

    return ret;
}

/**
 * Mgmt_Lqi_rsp, Mgmt_Rtg_rsp share the same structure
 * The only difference between them is the size of individual variable part elements
 */
static zb_bool_t zdo_validate_mgmt_common_resp(zb_bufid_t buf, zb_uint8_t element_size)
{
    zb_zdo_mgmt_lqi_resp_t *resp;
    zb_size_t expected_len = 0;
    zb_bool_t ret = ZB_FALSE;

    expected_len += sizeof(*resp);
    resp = (zb_zdo_mgmt_lqi_resp_t *)zb_buf_begin(buf);
    if (zb_buf_len(buf) >= expected_len)
    {
        expected_len += (zb_size_t)resp->neighbor_table_list_count * (zb_size_t)element_size;
        ret = zb_buf_len(buf) >= expected_len ? ZB_TRUE : ZB_FALSE;
    }

    return ret;
}

/**
 * See ZB spec 2.4.4.4.4
 */
static zb_bool_t zdo_validate_mgmt_bind_resp(zb_bufid_t buf)
{
    zb_zdo_mgmt_lqi_resp_t *resp;
    zb_zdo_binding_table_record_t *rec;
    zb_size_t expected_len, permanent_rec_len;
    zb_uint32_t i;
    zb_bool_t   ret = ZB_FALSE;

    /* first check Bind resp cmd header */
    expected_len = sizeof(zb_zdo_mgmt_lqi_resp_t);
    if (zb_buf_len(buf) >= expected_len)
    {
        /* check binding table records */
        resp = (zb_zdo_mgmt_lqi_resp_t *)zb_buf_begin(buf);
        if (resp->neighbor_table_list_count != 0U)
        {
            /* calc permanent binding table record fields size:
             * src_address, src_endp, cluster_id, dst_addr_mode.
             */
            permanent_rec_len = sizeof(zb_ieee_addr_t) + sizeof(zb_uint8_t) +
                                sizeof(zb_uint16_t) + sizeof(zb_uint8_t);

            /* check all binding table records */
            for (i = 0; i < resp->neighbor_table_list_count; ++i)
            {
                /* previously check permanent part from binding table record */
                if (zb_buf_len(buf) >= expected_len + permanent_rec_len)
                {
                    /*cstat !MISRAC2012-Rule-11.3 */
                    /** @mdr{00002,89} */
                    rec = (zb_zdo_binding_table_record_t *)((zb_uint8_t *)zb_buf_begin(buf) + expected_len);
                    if (rec->dst_addr_mode == ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT)
                    {
                        /* plus short address */
                        expected_len += permanent_rec_len + sizeof(zb_uint16_t);
                        ret = zb_buf_len(buf) >= expected_len ? ZB_TRUE : ZB_FALSE;
                    }
                    else if (rec->dst_addr_mode == ZB_APS_ADDR_MODE_64_ENDP_PRESENT)
                    {
                        /* plus long address and dst endpoint */
                        expected_len += permanent_rec_len + sizeof(zb_ieee_addr_t) + sizeof(zb_uint8_t);
                        ret = zb_buf_len(buf) >= expected_len ? ZB_TRUE : ZB_FALSE;
                    }
                    else
                    {
                        /* unexpected address mode */
                        ret = ZB_FALSE;
                    }

                    if (ret != ZB_TRUE)
                    {
                        break;
                    }
                }
                else
                {
                    ret = ZB_FALSE;
                    break;
                }
            }
        }
        else
        {
            ret = ZB_TRUE;
        }
    }

    return ret;
}

static zb_bool_t zdo_validate_mgmt_nwk_update_notify(zb_bufid_t buf)
{
    zb_zdo_mgmt_nwk_update_notify_hdr_t *resp;
    zb_size_t expected_len = 0;
    zb_bool_t ret = ZB_FALSE;

    expected_len += sizeof(*resp);
    resp = (zb_zdo_mgmt_nwk_update_notify_hdr_t *)zb_buf_begin(buf);
    if (zb_buf_len(buf) >= expected_len)
    {
        expected_len += resp->scanned_channels_list_count * sizeof(zb_uint8_t);
        ret = zb_buf_len(buf) >= expected_len ? ZB_TRUE : ZB_FALSE;
    }

    return ret;
}

zb_bool_t zb_zdo_validate_reponse(zb_bufid_t buf, zb_uint16_t cluster_id)
{
    zb_bool_t ret;

    TRACE_MSG(TRACE_ZDO3, ">> zb_zdo_validate_reponse (cluster 0x%x)", (FMT__D, cluster_id));

    switch (cluster_id)
    {
    case ZDO_NWK_ADDR_RESP_CLID:
        ret = zdo_validate_nwk_addr_resp(buf);
        break;

    case ZDO_IEEE_ADDR_RESP_CLID:
        ret = zdo_validate_nwk_addr_resp(buf);
        break;

    case ZDO_NODE_DESC_RESP_CLID:
        ret = zdo_validate_desc_resp(buf, (zb_uint8_t)sizeof(zb_zdo_node_desc_resp_t));
        break;

    case ZDO_POWER_DESC_RESP_CLID:
        ret = zdo_validate_desc_resp(buf, (zb_uint8_t)sizeof(zb_af_node_power_desc_t));
        break;

    case ZDO_SIMPLE_DESC_RESP_CLID:
        ret = zdo_validate_var_desc_resp(buf);
        break;

    case ZDO_ACTIVE_EP_RESP_CLID:
        ret = zdo_validate_var_desc_resp(buf);
        break;

    case ZDO_MATCH_DESC_RESP_CLID:
        ret = zdo_validate_var_desc_resp(buf);
        break;

    case ZDO_COMPLEX_DESC_RESP_CLID:
        ret = zdo_validate_var_desc_resp(buf);
        /* complex descriptors do not seem to be handled by ZBOSS */
        break;

    case ZDO_USER_DESC_RESP_CLID:
        ret = zdo_validate_var_desc_resp(buf);
        /* user descriptors do not seem to be handled by ZBOSS */
        break;

    case ZDO_USER_DESC_CONF_CLID:
        ret = (zb_bool_t)(zb_buf_len(buf) >= sizeof(zb_zdo_desc_resp_hdr_t));
        break;

#ifndef ZB_LITE_NO_ZDO_SYSTEM_SERVER_DISCOVERY
    case ZDO_SYSTEM_SERVER_DISCOVERY_RESP_CLID:
        ret = (zb_bool_t)(zb_buf_len(buf) >= sizeof(zb_zdo_system_server_discovery_resp_t));
        break;
#endif

    case ZDO_PARENT_ANNCE_RESP_CLID:
        ret = zdo_validate_parent_annce_resp(buf);
        break;

#ifndef R23_DISABLE_DEPRECATED_ZDO_CMDS
    case ZDO_END_DEVICE_BIND_RESP_CLID:
        ret = (zb_bool_t)(zb_buf_len(buf) >= sizeof(zb_zdo_end_device_bind_resp_t));
        break;
#endif /* R23_DISABLE_DEPRECATED_ZDO_CMDS */

    case ZDO_UNBIND_RESP_CLID:
    /* fall through */
    case ZDO_BIND_RESP_CLID:
        ret = (zb_bool_t)(zb_buf_len(buf) >= sizeof(zb_zdo_bind_resp_t));
        break;

    case ZDO_MGMT_LQI_RESP_CLID:
        ret = zdo_validate_mgmt_common_resp(buf, (zb_uint8_t)sizeof(zb_zdo_neighbor_table_record_t));
        break;

#if !(defined ZB_LITE_NO_ZDO_MGMT_RTG || defined R23_DISABLE_DEPRECATED_ZDO_CMDS)
    case ZDO_MGMT_RTG_RESP_CLID:
        ret = zdo_validate_mgmt_common_resp(buf, (zb_uint8_t)sizeof(zb_zdo_routing_table_record_t));
        break;
#endif

    case ZDO_MGMT_BIND_RESP_CLID:
        ret = zdo_validate_mgmt_bind_resp(buf);
        break;

    case ZDO_MGMT_LEAVE_RESP_CLID:
        ret = (zb_bool_t)(zb_buf_len(buf) >= sizeof(zb_zdo_mgmt_leave_res_t));
        break;

    case ZDO_MGMT_PERMIT_JOINING_RESP_CLID:
        ret = (zb_bool_t)(zb_buf_len(buf) >= sizeof(zb_zdo_mgmt_permit_joining_resp_t));
        break;

    case ZDO_MGMT_NWK_UPDATE_NOTIFY_CLID:
#ifdef ZB_MGMT_NWK_ENHANCED_UPDATE_ENABLED
    /* fall through, they are of the same structure */
    case ZDO_MGMT_NWK_ENHANCED_UPDATE_NOTIFY_CLID:
#endif
        ret = zdo_validate_mgmt_nwk_update_notify(buf);
        break;

#ifdef ZB_MGMT_NWK_ENHANCED_UPDATE_ENABLED
    case ZDO_MGMT_NWK_UNSOLICITED_ENHANCED_UPDATE_NOTIFY_CLID:
        ret = (zb_bool_t)(zb_buf_len(buf) >= sizeof(zb_zdo_mgmt_nwk_unsol_enh_update_notify_t));
        break;
#endif

    case ZDO_MGMT_NWK_IEEE_JOINING_LIST_RESP_CLID:
        /* TODO: after merge */
        ret = ZB_TRUE;
        break;

    /* TODO: Also, NWK Enhanced and NWK unsolicited enhanced */
    default:
        /* Unknown clusted id*/
        ret = ZB_FALSE;
        break;
    }

    TRACE_MSG(TRACE_ZDO3, "<< zb_zdo_validate_reponse (ret %hd)", (FMT__H, ret));

    return ret;
}

#endif /* ZB_LITE_NO_ZDO_RESPONSE_VALIDATION */
