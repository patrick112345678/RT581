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
/* PURPOSE: ZDO Discovery services - server side.
Mandatory calls onnly. Other calls will be implemented in some other project scope.
*/

#define ZB_TRACE_FILE_ID 2097
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_hash.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zdo_common.h"

/*! \addtogroup ZB_ZDO */
/*! @{ */

#ifdef ZB_APS_ENCRYPTION_PER_CLUSTER
#define SIMPLE_DESC_1_1_SIZE                                            \
  (sizeof(zb_af_simple_desc_1_1_t)                                      \
   - sizeof(zb_uint16_t)*2U /* Take into account app_cluster_list */     \
   - 1U)                     /* Take into account cluster_encryption field */
#else
#define SIMPLE_DESC_1_1_SIZE                                            \
  (sizeof(zb_af_simple_desc_1_1_t)                                      \
   - sizeof(zb_uint16_t)*2U) /* Take into account app_cluster_list */
#endif

void zdo_send_desc_resp(zb_uint8_t param)
{
    zb_uint16_t addr_of_interest;
    zb_apsde_data_indication_t *ind;
    zb_zdo_desc_resp_hdr_t *resp_hdr;
    zb_uint8_t *aps_body;
    zb_uint8_t tsn;
    zb_uint16_t resp_id = 0;

    /*
      2.4.4.1.3 Node_Desc_rsp
    */
    TRACE_MSG(TRACE_ZDO3, ">>zdo_send_desc_resp %hd", (FMT__H, param));

    if (zb_buf_len(param) < (sizeof(addr_of_interest) + sizeof(tsn)))
    {
        TRACE_MSG(TRACE_ZDO1, "Malformed node_desc_req/power_desc_req - drop packet", (FMT__0));
        zb_buf_free(param);
        return;
    }

    /*
      2.4.2.8 Transmission of ZDP Commands
      | Transaction sequence number (1byte) | Transaction data (variable) |
    */

    aps_body = zb_buf_begin(param);

    tsn = *aps_body;
    aps_body++;

    ZB_LETOH16(&addr_of_interest, aps_body);

    ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);
    resp_hdr = zb_buf_initial_alloc(param, sizeof(zb_zdo_desc_resp_hdr_t));

    resp_hdr->tsn = tsn;

    if (ZB_PIBCACHE_NETWORK_ADDRESS() == addr_of_interest)
    {
        zb_uint8_t *desc_body;

        resp_hdr->status = ZB_ZDP_STATUS_SUCCESS;
        switch (ind->clusterid)
        {
        case ZDO_NODE_DESC_REQ_CLID:
            desc_body = zb_buf_alloc_right(param, sizeof(zb_af_node_desc_t));
            /*cstat !MISRAC2012-Rule-11.3 */
            /** @mdr{00002,74} */
            zb_copy_node_desc((zb_af_node_desc_t *)desc_body, ZB_ZDO_NODE_DESC());
            resp_id = ZDO_NODE_DESC_RESP_CLID;
            break;

        case ZDO_POWER_DESC_REQ_CLID:
            desc_body = zb_buf_alloc_right(param, sizeof(zb_af_node_power_desc_t));
            /*cstat !MISRAC2012-Rule-11.3 */
            /** @mdr{00002,75} */
            zb_copy_power_desc((zb_af_node_power_desc_t *)desc_body, ZB_ZDO_NODE_POWER_DESC());
            resp_id = ZDO_POWER_DESC_RESP_CLID;
            break;

        default:
            TRACE_MSG(TRACE_ZDO3, "unknown cluster id %i", (FMT__D, ind->clusterid));
            ZB_ASSERT(0);
            break;
        }
    }
    else
    {
        /* descriptor is not sent in this case */
#ifdef ZB_COORDINATOR_ROLE
        /* search for child - not implemented */
        resp_hdr->status = ZB_ZDP_STATUS_DEVICE_NOT_FOUND;
        TRACE_MSG(TRACE_ZDO1, "Error,device cache is not impl", (FMT__0));
#else
        resp_hdr->status = ZB_ZDP_STATUS_INV_REQUESTTYPE;
        TRACE_MSG(TRACE_ZDO1, "Error,invalid addr", (FMT__0));
#endif
    }

    ZB_LETOH16((zb_uint8_t *)&resp_hdr->nwk_addr, (zb_uint8_t *)&addr_of_interest);
    zdo_send_resp_by_short(resp_id, param, ind->src_addr);

    TRACE_MSG(TRACE_ZDO3, "<<zdo_send_desc_resp", (FMT__0));
}


#ifdef ZB_FILTER_OUT_CLUSTERS
void zdo_add_cluster_filter(zdo_cluster_filter_t filter)
{
    zb_uint_t i;
    for (i = 0 ; i < ZDO_CTX().cluster_filters.n_filters ; ++i)
    {
        if (ZDO_CTX().cluster_filters.filters[i] == filter)
        {
            return;
        }
    }
    if (i < MAX_N_CLUSTER_FILTERS)
    {
        ZDO_CTX().cluster_filters.filters[i] = filter;
        ZDO_CTX().cluster_filters.n_filters++;
    }
}


zb_int_t zdo_check_cluster_filtered_out(zb_uint16_t clid)
{
    zb_uint_t i;
    for (i = 0 ; i < ZDO_CTX().cluster_filters.n_filters ; ++i)
    {
        if ((*ZDO_CTX().cluster_filters.filters[i])(clid) != 0)
        {
            return 1;
        }
    }
    return 0;
}

#endif  /* ZB_FILTER_OUT_CLUSTERS */

static zb_uint8_t get_app_input_cluster_count(zb_af_simple_desc_1_1_t *src_desc)
{
#ifndef ZB_FILTER_OUT_CLUSTERS
    return src_desc->app_input_cluster_count;
#else
    zb_uint8_t i;
    zb_uint8_t cnt = 0;

    for (i = 0; i < src_desc->app_input_cluster_count; i++)
    {
        if (ZDO_CLUSTER_FILTERED_OUT(src_desc->app_cluster_list[i]) == 0)
        {
            cnt++;
        }
    }
    return cnt;
#endif
}


static zb_uint8_t get_app_output_cluster_count(zb_af_simple_desc_1_1_t *src_desc)
{
#ifndef ZB_FILTER_OUT_CLUSTERS
    return src_desc->app_output_cluster_count;
#else
    zb_uint8_t i;
    zb_uint8_t cnt = 0;

    for (i = src_desc->app_input_cluster_count; i < src_desc->app_input_cluster_count + src_desc->app_output_cluster_count; i++)
    {
        if (ZDO_CLUSTER_FILTERED_OUT(src_desc->app_cluster_list[i]) == 0)
        {
            cnt++;
        }
    }
    return cnt;
#endif
}


void zdo_send_simple_desc_resp(zb_uint8_t param)
{
    zb_uint16_t addr_of_interest;
    zb_uint16_t src_addr;
    zb_apsde_data_indication_t *ind;
    zb_zdo_simple_desc_resp_hdr_t *resp_hdr;
    zb_uint8_t tsn;
    zb_uint8_t *desc_body;
    zb_af_simple_desc_1_1_t *src_desc = NULL;
    zb_uint8_t ep;
    zb_uindex_t i;

    /*
      2.4.4.1.5 Simple_Desc_rsp
    */
    TRACE_MSG(TRACE_ZDO1, ">>zdo_send_simple_desc_resp %hd", (FMT__H, param));
    {
        zb_uint8_t *aps_body;
        aps_body = zb_buf_begin(param);

        /*
          2.4.2.8 Transmission of ZDP Commands
          | Transaction sequence number (1byte) | Transaction data (variable) |
        */

        if (zb_buf_len(param) < sizeof(zb_zdo_simple_desc_req_t) + sizeof(zb_uint8_t))
        {
            /* ZDO payload for this request: tsn| nwk_addr_of_interest| endpoint */
            TRACE_MSG(TRACE_ZDO1, "Malformed simple_desc_req - drop packet", (FMT__0));
            zb_buf_free(param);
            return;
        }

        tsn = *aps_body;
        aps_body++;

        zb_get_next_letoh16(&addr_of_interest, &aps_body);
        ep = *aps_body;
    }
    ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);
    src_addr = ind->src_addr;
    resp_hdr = zb_buf_initial_alloc(param, sizeof(zb_zdo_simple_desc_resp_hdr_t));
    resp_hdr->tsn = tsn;
    resp_hdr->length = 0;

    if (ZB_PIBCACHE_NETWORK_ADDRESS() == addr_of_interest)
    {
        if (ep == 0U)
        {
            /*cstat !MISRAC2012-Rule-11.3 */
            /** @mdr{00002,76} */
            src_desc = (zb_af_simple_desc_1_1_t *)ZB_ZDO_SIMPLE_DESC();
        }
        else
        {
            for (i = 0; i < ZB_ZDO_SIMPLE_DESC_NUMBER(); i++)
            {
                if (ZB_ZDO_SIMPLE_DESC_LIST()[i]->endpoint == ep)
                {
                    src_desc = (zb_af_simple_desc_1_1_t *)ZB_ZDO_SIMPLE_DESC_LIST()[i];
                }
            }
        }

        if (src_desc != NULL)
        {
            zb_size_t resp_hdr_len = SIMPLE_DESC_1_1_SIZE;
            resp_hdr_len += ((zb_size_t)get_app_input_cluster_count(src_desc) + (zb_size_t)get_app_output_cluster_count(src_desc)) * sizeof(zb_uint16_t);
            resp_hdr->status = ZB_ZDP_STATUS_SUCCESS;
            resp_hdr->length = (zb_uint8_t)resp_hdr_len;
            desc_body = zb_buf_alloc_right(param, resp_hdr->length);
            /*cstat !MISRAC2012-Rule-11.3 */
            /** @mdr{00002,77} */
            zb_copy_simple_desc((zb_af_simple_desc_1_1_t *)desc_body, src_desc);
        }
        else
        {
            if (ep < ZB_ZCL_BROADCAST_ENDPOINT && ep > 0U)
            {
                /* From 2.4.4.2.5.1:
                 * If the endpoint field does not correspond to an active endpoint, the remote device
                 * shall set the Status field to NOT_ACTIVE, set the Length field to 0, and not include
                 * the SimpleDescriptor field.
                 */
                resp_hdr->status = ZB_ZDP_STATUS_NOT_ACTIVE;
            }
            else
            {
                /* From 2.4.4.2.5.1:
                 * If the 4169 endpoint field specified in the original Simple_Desc_req command does not
                 * fall within the correct range specified in Table 2.96 Fields of the Simple_Desc_req
                 * Command, the remote device shall set the Status field to INVALID_EP, set the Length
                 * field to 0 and not include the SimpleDescriptor field.
                 */
                resp_hdr->status = ZB_ZDP_STATUS_INVALID_EP;
            }
        }
    }
    else
    {
        /* descriptor is not sent in this case */
#ifdef ZB_COORDINATOR_ROLE
        /* search for child - not implemented */
        resp_hdr->status = ZB_ZDP_STATUS_DEVICE_NOT_FOUND;
        TRACE_MSG(TRACE_ZDO1, "Error,device cache is not impl", (FMT__0));
#else
        resp_hdr->status = ZB_ZDP_STATUS_INV_REQUESTTYPE;
        TRACE_MSG(TRACE_ZDO1, "Error,invalid addr", (FMT__0));
#endif
    }

    ZB_LETOH16((zb_uint8_t *)&resp_hdr->nwk_addr, (zb_uint8_t *)&addr_of_interest);
    TRACE_MSG(TRACE_ZDO3, "simple_desc length: %hd", (FMT__H, resp_hdr->length));
    zdo_send_resp_by_short(ZDO_SIMPLE_DESC_RESP_CLID, param, src_addr);

    TRACE_MSG(TRACE_ZDO1, "<<zdo_send_desc_resp", (FMT__0));
}

#ifdef ZB_FIXED_OPTIONAL_DESC_RESPONSES
void zb_zdo_send_complex_desc_resp(zb_uint8_t param)
{
    zb_zdo_complex_desc_resp_hdr_t *resp = NULL;
    zb_uint8_t tsn;
    zb_uint16_t addr_of_interest;

    zb_apsde_data_indication_t *ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);
    zb_uint16_t addr = ind->src_addr;

    if (zb_buf_len(param) < (sizeof(addr_of_interest) + sizeof(tsn)))
    {
        TRACE_MSG(TRACE_ZDO1, "Malformed Complex_Desc_req - drop packet", (FMT__0));
        zb_buf_free(param);
        return;
    }

    {
        zb_uint8_t *aps_body = zb_buf_begin(param);

        tsn = *aps_body;
        aps_body++;

        zb_get_next_letoh16(&addr_of_interest, &aps_body);
    }

    TRACE_MSG(TRACE_ZDO3, ">> zb_zdo_send_complex_desc_resp %hd", (FMT__H, param));

    resp = zb_buf_initial_alloc(param, sizeof(zb_zdo_complex_desc_resp_hdr_t));
    ZB_BZERO(resp, sizeof(zb_zdo_complex_desc_resp_hdr_t));
    resp->tsn = tsn;
    resp->status = ZB_ZDP_STATUS_NOT_SUPPORTED;
    resp->nwk_addr = addr_of_interest;
    resp->length = 0;

    zdo_send_resp_by_short(ZDO_COMPLEX_DESC_RESP_CLID, param, addr);
}


void zb_zdo_send_user_desc_resp(zb_uint8_t param)
{
    zb_zdo_user_desc_resp_hdr_t *resp = NULL;
    zb_uint8_t tsn;
    zb_uint16_t addr_of_interest;
    zb_uint8_t *aps_body;

    zb_apsde_data_indication_t *ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);
    zb_uint16_t addr = ind->src_addr;

    if (zb_buf_len(param) < (sizeof(addr_of_interest) + sizeof(tsn)))
    {
        TRACE_MSG(TRACE_ZDO1, "Malformed User_Desc_req - drop packet", (FMT__0));
        zb_buf_free(param);
        return;
    }

    aps_body = zb_buf_begin(param);

    tsn = *aps_body;
    aps_body++;

    zb_get_next_letoh16(&addr_of_interest, &aps_body);

    TRACE_MSG(TRACE_ZDO3, ">> zb_zdo_send_user_desc_resp %hd", (FMT__H, param));

    resp = zb_buf_initial_alloc(param, sizeof(zb_zdo_user_desc_resp_hdr_t));
    ZB_BZERO(resp, sizeof(zb_zdo_user_desc_resp_hdr_t));
    resp->tsn = tsn;
    resp->status = ZB_ZDP_STATUS_NOT_SUPPORTED;
    resp->nwk_addr = addr_of_interest;
    resp->length = 0;

    zdo_send_resp_by_short(ZDO_USER_DESC_RESP_CLID, param, addr);
}


#ifndef R23_DISABLE_DEPRECATED_ZDO_CMDS
void zb_zdo_send_user_desc_conf(zb_uint8_t param)
{
    zb_zdo_user_desc_conf_hdr_t *resp = NULL;
    zb_uint8_t tsn;
    zb_uint16_t addr_of_interest;
    zb_uint8_t *aps_body;

    zb_apsde_data_indication_t *ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);
    zb_uint16_t addr = ind->src_addr;

    if (zb_buf_len(param) < (sizeof(addr_of_interest) + sizeof(tsn)))
    {
        TRACE_MSG(TRACE_ZDO1, "Malformed User_Desc_set_req - drop packet", (FMT__0));
        zb_buf_free(param);
        return;
    }

    aps_body = zb_buf_begin(param);

    tsn = *aps_body;
    aps_body++;

    zb_get_next_letoh16(&addr_of_interest, &aps_body);

    TRACE_MSG(TRACE_ZDO3, ">> zb_zdo_send_user_desc_conf %hd", (FMT__H, param));

    resp = zb_buf_initial_alloc(param, sizeof(zb_zdo_user_desc_conf_hdr_t));
    ZB_BZERO(resp, sizeof(zb_zdo_user_desc_conf_hdr_t));
    resp->tsn = tsn;
    resp->status = ZB_ZDP_STATUS_NOT_SUPPORTED;
    resp->nwk_addr = addr_of_interest;

    zdo_send_resp_by_short(ZDO_USER_DESC_CONF_CLID, param, addr);
}
#endif /* R23_DISABLE_DEPRECATED_ZDO_CMDS */
#endif

/* convert payload from ZibBee ZDO fromat to ZBOSS ZDO format
   change order cluster count - cluster ids
   see @ref zb_zdo_simple_desc_resp_t and ZDO spec */
zb_ret_t zb_zdo_simple_desc_resp_convert_zboss(zb_uint8_t param)
{
    zb_zdo_simple_desc_resp_t *resp = (zb_zdo_simple_desc_resp_t *)zb_buf_begin(param);
    zb_ret_t ret = RET_IGNORE;

    zb_uint8_t in_cluster_cnt = 0;
    zb_uint8_t out_cluster_cnt = 0;
    zb_uint8_t simple_desc_len;
    zb_bool_t is_ill_formed = ZB_FALSE;
    zb_uint8_t min_simple_desc_size = (zb_uint8_t)SIMPLE_DESC_1_1_SIZE;

    TRACE_MSG(TRACE_ZDO1, "> zb_zdo_simple_desc_resp_convert_zboss param %hd", (FMT__H, param));

    simple_desc_len = resp->hdr.length;

    /* Check that declared simple descriptor length is equal to the actual simple
     * descriptor size received in the packet. */
    if (simple_desc_len != zb_buf_len(param) - sizeof(resp->hdr))
    {
        is_ill_formed = ZB_TRUE;
    }
    else if (simple_desc_len == 0U)
    {
        /* No payload (legal case) - pass response up */
        TRACE_MSG(TRACE_ZDO1, "no descriptor", (FMT__0));
        ret = RET_OK;
    }
    else
    {
        /* First check that simple descriptor in response has at least 8 bytes - minimum size
         * of descriptor */
        if (simple_desc_len < min_simple_desc_size)
        {
            is_ill_formed = ZB_TRUE;
        }

        /* simple_desc has at least all required fields + in_cluster_cnt field +
         * list of input clusters + out_cluster_cnt_field - check it first */
        if (!is_ill_formed)
        {
            in_cluster_cnt = resp->simple_desc.app_input_cluster_count;
            is_ill_formed = (zb_bool_t) (simple_desc_len < min_simple_desc_size +
                                         in_cluster_cnt * sizeof(zb_uint16_t));
        }

        /* check that simple_desc_len is equal to full size of simple descriptor in response */
        if (!is_ill_formed)
        {
            out_cluster_cnt = *( &(resp->simple_desc.app_output_cluster_count) +
                                 sizeof(zb_uint16_t) * in_cluster_cnt);
            is_ill_formed = (zb_bool_t) (simple_desc_len != min_simple_desc_size +
                                         in_cluster_cnt * sizeof(zb_uint16_t) +
                                         out_cluster_cnt * sizeof(zb_uint16_t));
        }
    }

    if (!is_ill_formed && (simple_desc_len != 0U))
    {
        /* Move input cluster list from ZCL packet (which is starts with 'app_output_cluster_count' in
         * ZBOSS structure layout) to input cluster list in ZBOSS structure. */
        ZB_MEMMOVE((zb_uint8_t *)resp->simple_desc.app_cluster_list, (zb_uint8_t *)&resp->simple_desc.app_output_cluster_count,
                   sizeof(zb_uint16_t)*in_cluster_cnt);

        resp->simple_desc.app_output_cluster_count = out_cluster_cnt;
        ret = RET_OK;
    }

    TRACE_MSG(TRACE_ZDO1, "< zb_zdo_simple_desc_resp_convert_zboss: ret %d", (FMT__D, ret));

    return ret;
}


void zb_copy_node_desc(zb_af_node_desc_t *dst_desc, zb_af_node_desc_t *src_desc)
{
    ZB_LETOH16((zb_uint8_t *)&dst_desc->node_desc_flags, (zb_uint8_t *)&src_desc->node_desc_flags);
    dst_desc->mac_capability_flags = src_desc->mac_capability_flags;
    ZB_LETOH16((zb_uint8_t *)&dst_desc->manufacturer_code, (zb_uint8_t *)&src_desc->manufacturer_code);
    dst_desc->max_buf_size = src_desc->max_buf_size;
    ZB_LETOH16((zb_uint8_t *)&dst_desc->max_incoming_transfer_size, (zb_uint8_t *)&src_desc->max_incoming_transfer_size);
    ZB_LETOH16((zb_uint8_t *)&dst_desc->server_mask, (zb_uint8_t *)&src_desc->server_mask);
    ZB_LETOH16((zb_uint8_t *)&dst_desc->max_outgoing_transfer_size, (zb_uint8_t *)&src_desc->max_outgoing_transfer_size);
    dst_desc->desc_capability_field = src_desc->desc_capability_field;
}

void zb_copy_power_desc(zb_af_node_power_desc_t *dst_desc, zb_af_node_power_desc_t *src_desc)
{
    ZB_LETOH16((zb_uint8_t *)&dst_desc->power_desc_flags, (zb_uint8_t *)&src_desc->power_desc_flags);
}

void zb_copy_simple_desc(zb_af_simple_desc_1_1_t *dst_desc, zb_af_simple_desc_1_1_t *src_desc)
{
    zb_uint8_t i, j;
    zb_uint8_t out_count;
    ZB_MEMCPY(dst_desc, src_desc, SIMPLE_DESC_1_1_SIZE);
    ZB_LETOH16_XOR(dst_desc->app_profile_id);
    ZB_LETOH16_XOR(dst_desc->app_device_id);
    dst_desc->app_input_cluster_count = get_app_input_cluster_count(src_desc);
    out_count = dst_desc->app_output_cluster_count = get_app_output_cluster_count(src_desc);
    TRACE_MSG(TRACE_ZDO1, "simple_desc outclusters count %hd", (FMT__H, out_count));
    TRACE_MSG(TRACE_ZDO1, "simple_desc inclusters count %hd", (FMT__H, dst_desc->app_input_cluster_count));

    j = 0;
    for (i = 0; i < src_desc->app_input_cluster_count; i++)
    {
        /*cstat !MISRAC2012-Rule-14.3_a */
        /** @mdr{00006,0} */
        if ((ZDO_CLUSTER_FILTERED_OUT(src_desc->app_cluster_list[i])) == 0)
        {
            ZB_LETOH16(((zb_uint8_t *)(&dst_desc->app_cluster_list[j]) - 1), (zb_uint8_t *)&src_desc->app_cluster_list[i]);
            j++;
        }
    }
    for (i = src_desc->app_input_cluster_count; i < src_desc->app_input_cluster_count + src_desc->app_output_cluster_count; i++)
    {
        /*cstat !MISRAC2012-Rule-14.3_a */
        /** @mdr{00006,1} */
        if ((ZDO_CLUSTER_FILTERED_OUT(src_desc->app_cluster_list[i])) == 0)
        {
            ZB_LETOH16(((zb_uint8_t *)&dst_desc->app_cluster_list[j]), ((zb_uint8_t *)&src_desc->app_cluster_list[i]));
            j++;
        }
    }
    ZB_MEMCPY((zb_uint8_t *)(dst_desc->app_cluster_list + dst_desc->app_input_cluster_count) - 1, &out_count, 1);

}

void zdo_device_nwk_addr_res(zb_uint8_t param, zb_uint8_t fc)
{
    zb_uint8_t status = ZB_ZDP_STATUS_SUCCESS;
    zb_apsde_data_indication_t ind;
    zb_zdo_nwk_addr_req_t req;
    zb_uint8_t *aps_body;
    zb_uint8_t tsn;
    zb_uint16_t nwk_addr = ZB_UNKNOWN_SHORT_ADDR;
    zb_zdo_nwk_addr_resp_head_t *resp;
    zb_ieee_addr_t remote_dev_ieee;

    TRACE_MSG(TRACE_ZDO3, "zdo_dev_nwk_addr_res %hd, fc %hd", (FMT__H_H, param, fc));
    TRACE_MSG(TRACE_ATM1, "Z< send network address response", (FMT__0));

    if (zb_buf_len(param) < sizeof(zb_zdo_nwk_addr_req_t) + sizeof(zb_uint8_t))
    {
        TRACE_MSG(TRACE_ZDO3, "No payload - drop", (FMT__0));
        zb_buf_free(param);
        return;
    }

    aps_body = zb_buf_begin(param);
    tsn = *aps_body;
    aps_body++;

    ZB_MEMCPY(&req, aps_body, sizeof(req));
    ZB_MEMCPY(remote_dev_ieee, req.ieee_addr, sizeof(zb_ieee_addr_t));

    ZB_MEMCPY(&ind, ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t), sizeof(ind));
    resp = zb_buf_initial_alloc(param, sizeof(zb_zdo_ieee_addr_resp_t));

    TRACE_MSG(TRACE_ZDO3, "request_type %hd, start_index %hd", (FMT__H_H, req.request_type, req.start_index));
    ZB_DUMP_IEEE_ADDR(req.ieee_addr);
    ZB_DUMP_IEEE_ADDR(ZB_PIBCACHE_EXTENDED_ADDRESS());

    if (ZB_64BIT_ADDR_CMP(ZB_PIBCACHE_EXTENDED_ADDRESS(), req.ieee_addr))
    {
        nwk_addr = ZB_PIBCACHE_NETWORK_ADDRESS();
        TRACE_MSG(TRACE_ZDO3, "local found 0x%x", (FMT__D, nwk_addr));
    }
    else
    {
        if (zb_nwk_get_nbr_dvc_type_by_ieee(req.ieee_addr) == ZB_NWK_DEVICE_TYPE_ED)
        {
            nwk_addr = zb_address_short_by_ieee(req.ieee_addr);
            TRACE_MSG(TRACE_ZDO3, "remote found 0x%x", (FMT__D, nwk_addr));
        }
    }

    if (nwk_addr == ZB_UNKNOWN_SHORT_ADDR)
    {
        if (ZB_APS_FC_GET_DELIVERY_MODE(fc) == ZB_APS_DELIVERY_UNICAST)
        {
            status = ZB_ZDP_STATUS_DEVICE_NOT_FOUND;
            /* If there is no match and the request was unicast, a NWK_addr_resp command shall be
               generated and sent back to the local device with the Status field set to
               DEVICE_NOT_FOUND, the IEEEAddrRemoteDev field set to the IEEE address of
               the request; the NWKAddrRemoteDev field set to the 0xFFFF
               (CCB 2112 from 08/08/2016). */
            /* bwk_addr == ZB_UNKNOWN_SHORT_ADDR is already -1 == 0xffff */
        }
        /* If there is no match and the command was received as a
           broadcast, the request shall be discarded and no response generated.
        */
        else  /* ZB_APS_FC_GET_DELIVERY_MODE(fc) == ZB_APS_DELIVERY_BROADCAST */
        {
            zb_buf_free(param);
            return;
        }
    }

    if ((status == (zb_uint8_t)ZB_ZDP_STATUS_SUCCESS) &&
            (req.request_type > ZB_ZDO_EXTENDED_RESPONSE))
    {
        status  = ZB_ZDP_STATUS_INV_REQUESTTYPE;
        /* Response to broadcast packet with error shall not be generated
        * (CCB 2111 from 08/08/2016). */
        /*
          If the RequestType is one of the reserved values _and the request was not
          sent to a broadcast address_, a NWK_addr_resp command shall be generated
          and sent back to the local device with the Status field set to
          INV_REQUESTTYPE;
         */
        if (ZB_APS_FC_GET_DELIVERY_MODE(fc) == ZB_APS_DELIVERY_BROADCAST)
        {
            zb_buf_free(param);
            return;
        }
    }
    else if ((status == (zb_uint8_t)ZB_ZDP_STATUS_SUCCESS) &&
             (req.request_type == ZB_ZDO_SINGLE_DEV_RESPONSE))
    {
        /* Nothing to add */
    }
    else if (status == (zb_uint8_t)ZB_ZDP_STATUS_SUCCESS
             && req.request_type == ZB_ZDO_EXTENDED_RESPONSE
             /*cstat !MISRAC2012-Rule-13.5 */
             /* After some investigation, the following violation of Rule 13.5 seems to be
              * a false positive. There are no side effect to 'ZB_IS_DEVICE_ZC_OR_ZR()'. This
              * violation seems to be caused by the fact that 'ZB_IS_DEVICE_ZC_OR_ZR()' is an
              * external macro, which cannot be analyzed by C-STAT. */
             && ZB_IS_DEVICE_ZC_OR_ZR())
    {
        zb_zdo_nwk_addr_resp_ext_t *resp_ext;

        /* If the RequestType was Extended response and the Remote Device is either the Zigbee
           coordinator or router, a NWK_addr_resp command shall be generated and sent back to
           the local device with the Status field set to SUCCESS, the IEEEAddrRemoteDev field
           set to the IEEE address of the device itself, and the NWKAddrRemoteDev
           field set to the NWK address of the device itself (2.4.3.1.1.2). */
        nwk_addr = ZB_PIBCACHE_NETWORK_ADDRESS();
        ZB_MEMCPY(remote_dev_ieee, ZB_PIBCACHE_EXTENDED_ADDRESS(), sizeof(zb_ieee_addr_t));

        resp_ext = zb_buf_alloc_right(param, sizeof(zb_zdo_nwk_addr_resp_ext_t));
        resp_ext->num_assoc_dev = zb_nwk_neighbor_get_ed_cnt();
        if (resp_ext->num_assoc_dev != 0U)
        {
            zb_zdo_nwk_addr_resp_ext2_t *resp_ext2;
            zb_size_t max_records_num;
            zb_uint8_t records_num;
            zb_uint8_t *ed_list_ptr;

            max_records_num = (ZB_ZDO_MAX_PAYLOAD_SIZE -
                               (sizeof(zb_zdo_nwk_addr_resp_head_t) +
                                sizeof(zb_zdo_nwk_addr_resp_ext_t) +
                                sizeof(zb_zdo_nwk_addr_resp_ext2_t))) / sizeof(zb_uint16_t);

            records_num = (resp_ext->num_assoc_dev > req.start_index) ?
                          (zb_uint8_t)(resp_ext->num_assoc_dev - req.start_index) : 0U;
            TRACE_MSG(TRACE_ZDO3, "max rec %hd, total %hd, start indx %hd",
                      (FMT__H_H_H, max_records_num, zb_nwk_neighbor_table_size(), req.start_index));

            records_num = (records_num < max_records_num) ? records_num : (zb_uint8_t)max_records_num;

            resp_ext2 = zb_buf_alloc_right(param, sizeof(zb_zdo_nwk_addr_resp_ext2_t) + records_num * sizeof(zb_uint16_t));
            resp_ext2->start_index = req.start_index;
            ed_list_ptr = (zb_uint8_t *)(resp_ext2 + 1);
            resp_ext->num_assoc_dev = zb_nwk_neighbor_get_ed_short_list(req.start_index, records_num, &ed_list_ptr);
        }
    }

    resp->tsn = tsn;
    resp->status = status;
    ZB_HTOLE64(resp->ieee_addr, remote_dev_ieee);
    ZB_HTOLE16((zb_uint8_t *)&resp->nwk_addr, (zb_uint8_t *)&nwk_addr);

    zdo_send_resp_by_short(ZDO_NWK_ADDR_RESP_CLID, param, ind.src_addr);
}


void zdo_device_ieee_addr_res(zb_uint8_t param, zb_uint8_t fc)
{
    zb_uint8_t status = ZB_ZDP_STATUS_SUCCESS;
    zb_apsde_data_indication_t ind;
    zb_zdo_ieee_addr_req_t req;
    zb_zdo_ieee_addr_resp_t *resp;
    zb_uint8_t *aps_body;
    zb_uint8_t tsn;
    zb_uint16_t nwk_addr;
    zb_ieee_addr_t remote_dev_ieee;

    TRACE_MSG(TRACE_ZDO3, "zdo_dev_ieee_addr_res %hd, fc %hd", (FMT__H_H, param, fc));
    TRACE_MSG(TRACE_ATM1, "Z< send extended address response", (FMT__0));

    if (zb_buf_len(param) < sizeof(zb_zdo_ieee_addr_req_t) + sizeof(zb_uint8_t))
    {
        TRACE_MSG(TRACE_ZDO3, "No payload - drop", (FMT__0));
        zb_buf_free(param);
        return;
    }

    ZB_MEMCPY(remote_dev_ieee, g_unknown_ieee_addr, sizeof(zb_ieee_addr_t));

    aps_body = zb_buf_begin(param);
    tsn = *aps_body;
    aps_body++;

    ZB_MEMCPY(&req, aps_body, sizeof(req));

    ZB_MEMCPY(&ind, ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t), sizeof(ind));
    ZB_HTOLE16((zb_uint8_t *)&nwk_addr, (zb_uint8_t *)&req.nwk_addr);

    resp = zb_buf_initial_alloc(param, sizeof(zb_zdo_ieee_addr_resp_t));

    if (ZB_PIBCACHE_NETWORK_ADDRESS() != nwk_addr
            /*cstat !MISRAC2012-Rule-13.5 */
            /* After some investigation, the following violation of Rule 13.5 seems to be a false
             * positive. There are no side effects to 'zb_nwk_get_nbr_dvc_type_by_short()'.
             * This violation seems to be caused by the fact that this function is an external
             * function, which cannot be analyzed by C-STAT. */
            && zb_nwk_get_nbr_dvc_type_by_short(nwk_addr) != ZB_NWK_DEVICE_TYPE_ED)
    {
        /* device not found */
        if (ZB_APS_FC_GET_DELIVERY_MODE(fc) == ZB_APS_DELIVERY_UNICAST)
        {
            status = ZB_ZDP_STATUS_DEVICE_NOT_FOUND;
        }
        else /* ZB_APS_FC_GET_DELIVERY_MODE(fc) == ZB_APS_DELIVERY_BROADCAST */
        {
            zb_buf_free(param);
            return;
        }
    }
    else
    {
        if (req.request_type > ZB_ZDO_EXTENDED_RESPONSE)
        {
            status = ZB_ZDP_STATUS_INV_REQUESTTYPE;
            ZB_MEMCPY(remote_dev_ieee, ZB_PIBCACHE_EXTENDED_ADDRESS(), sizeof(zb_ieee_addr_t));
        }
        else if (req.request_type == ZB_ZDO_SINGLE_DEV_RESPONSE
                 /*cstat !MISRAC2012-Rule-13.5 */
                 /* After some investigation, the following violation of Rule 13.5 seems to be
                  * a false positive. There are no side effect to 'ZB_IS_DEVICE_ZED()'. This
                  * violation seems to be caused by the fact that 'ZB_IS_DEVICE_ZED()' is an
                  * external macro, which cannot be analyzed by C-STAT. */
                 || (req.request_type == ZB_ZDO_EXTENDED_RESPONSE && ZB_IS_DEVICE_ZED()))
        {
            zb_ret_t ret;

            /* If the RequestType is single device response, an IEEE_addr_resp command shall
               be generated and sent back to the local device with the Status field set to
               SUCCESS, the IEEEAddrRemoteDev field set to the IEEE address of
               the discovered device (2.4.3.1.2.2). */
            ret = zb_address_ieee_by_short(nwk_addr, remote_dev_ieee);
            ZB_ASSERT(ret == RET_OK);
        }
        else if (req.request_type == ZB_ZDO_EXTENDED_RESPONSE
                 /*cstat !MISRAC2012-Rule-13.5 */
                 /* After some investigation, the following violation of Rule 13.5 seems to be
                  * a false positive. There are no side effect to 'ZB_IS_DEVICE_ZC_OR_ZR()'. This
                  * violation seems to be caused by the fact that 'ZB_IS_DEVICE_ZC_OR_ZR()' is an
                  * external macro, which cannot be analyzed by C-STAT. */
                 && ZB_IS_DEVICE_ZC_OR_ZR())
        {
            zb_zdo_ieee_addr_resp_ext_t *resp_ext;

            ZB_MEMCPY(remote_dev_ieee, ZB_PIBCACHE_EXTENDED_ADDRESS(), sizeof(zb_ieee_addr_t));
            resp_ext = zb_buf_alloc_right(param, sizeof(zb_zdo_ieee_addr_resp_ext_t));
            resp_ext->num_assoc_dev = zb_nwk_neighbor_get_ed_cnt();
            if (resp_ext->num_assoc_dev != 0U)
            {
                zb_zdo_ieee_addr_resp_ext2_t *resp_ext2;
                zb_size_t max_records_num;
                zb_uint8_t records_num;
                zb_uint8_t *ed_list_ptr;

                max_records_num = (ZB_ZDO_MAX_PAYLOAD_SIZE -
                                   (sizeof(zb_zdo_ieee_addr_resp_t) +
                                    sizeof(zb_zdo_ieee_addr_resp_ext_t) +
                                    sizeof(zb_zdo_ieee_addr_resp_ext2_t))) / sizeof(zb_uint16_t);

                records_num = (resp_ext->num_assoc_dev > req.start_index) ?
                              (zb_uint8_t)(resp_ext->num_assoc_dev - req.start_index) : 0U;
                TRACE_MSG(TRACE_ZDO3, "max rec %hd, total %hd, start indx %hd",
                          (FMT__H_H_H, max_records_num, zb_nwk_neighbor_table_size(), req.start_index));

                records_num = (records_num < max_records_num) ? records_num : (zb_uint8_t)max_records_num;

                resp_ext2 = zb_buf_alloc_right(param, sizeof(zb_zdo_ieee_addr_resp_ext2_t) + records_num * sizeof(zb_uint16_t));
                resp_ext2->start_index = req.start_index;
                ed_list_ptr = (zb_uint8_t *)(resp_ext2 + 1);
                resp_ext->num_assoc_dev = zb_nwk_neighbor_get_ed_short_list(req.start_index, records_num, &ed_list_ptr);
            }
        }
    }

    resp->tsn = tsn;
    resp->status = status;
    ZB_MEMCPY(resp->ieee_addr_remote_dev, remote_dev_ieee, sizeof(zb_ieee_addr_t));
    ZB_HTOLE16((zb_uint8_t *)&resp->nwk_addr_remote_dev, (zb_uint8_t *)&nwk_addr);

    zdo_send_resp_by_short(ZDO_IEEE_ADDR_RESP_CLID, param, ind.src_addr);
}

void zdo_active_ep_res(zb_uint8_t param)
{
    zb_uint8_t status = ZB_ZDP_STATUS_SUCCESS;
    zb_apsde_data_indication_t ind;
    zb_zdo_active_ep_req_t req;
    zb_uint8_t *aps_body;
    zb_uint8_t tsn;
    zb_uint16_t nwk_addr;

    TRACE_MSG(TRACE_ZDO3, "zdo_active_ep_res %hd", (FMT__H, param));
    /* TODO: rewrite it using zb_zdo_ep_resp_t */
    if (zb_buf_len(param) < sizeof(zb_zdo_active_ep_req_t) + sizeof(zb_uint8_t))
    {
        TRACE_MSG(TRACE_ZDO3, "No payload - drop", (FMT__0));
        zb_buf_free(param);
        return;
    }

    aps_body = zb_buf_begin(param);
    tsn = *aps_body;
    aps_body++;

    ZB_MEMCPY(&req, aps_body, sizeof(req));
    ZB_MEMCPY(&ind, ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t), sizeof(ind));
    ZB_HTOLE16((zb_uint8_t *)&nwk_addr, (zb_uint8_t *)&req.nwk_addr);

    TRACE_MSG(TRACE_ZDO3, "desc # %hd", (FMT__H, ZB_ZDO_SIMPLE_DESC_NUMBER()));


    if ((ZB_IS_DEVICE_ZED())
            && (ZB_PIBCACHE_NETWORK_ADDRESS() != nwk_addr))
    {
        status = ZB_ZDP_STATUS_INV_REQUESTTYPE;
    }
    else if ((status == (zb_uint8_t)ZB_ZDP_STATUS_SUCCESS) &&
             (ZB_PIBCACHE_NETWORK_ADDRESS() != req.nwk_addr))
    {
        status = ZB_ZDP_STATUS_DEVICE_NOT_FOUND;
    }
    else if (ZB_ZDO_SIMPLE_DESC_NUMBER() == 0U)
    {
        status = ZB_ZDP_STATUS_DEVICE_NOT_FOUND;
    }
    else
    {
        zb_uint8_t *ptr;
        zb_uint8_t  len;
        zb_uindex_t i;

        len  = 1U + 1U + 2U + 1U + ZB_ZDO_SIMPLE_DESC_NUMBER(); /* TSN + Table 2.94: Status + NWKAddrOfInterest + ActiveEPCount + ActiveEPList */
        ptr = zb_buf_initial_alloc(param, len);
        *ptr = tsn;
        ptr++;
        *ptr = status;
        ptr++;

        ptr = zb_put_next_htole16(ptr, nwk_addr);

        *ptr = ZB_ZDO_SIMPLE_DESC_NUMBER();
        ptr++;

        for (i = 0; i < ZB_ZDO_SIMPLE_DESC_NUMBER(); i++)
        {
            *ptr = ZB_ZDO_SIMPLE_DESC_LIST()[i]->endpoint;
            ptr++;
        }
    }
    if (status != 0U)
    {
        zb_uint8_t *ptr;
        zb_uint8_t  len;

        len  = 5; /* TSN + Table 2.94: Status + NWKAddrOfInterest + ActiveEPCount */
        ptr = zb_buf_initial_alloc(param, len);
        *ptr = tsn;
        ptr++;
        *ptr = status;
        ptr++;
        ptr = zb_put_next_htole16(ptr, req.nwk_addr);
        *ptr = 0;
        ptr++;
    }
    zdo_send_resp_by_short(ZDO_ACTIVE_EP_RESP_CLID, param, ind.src_addr);
}

/* adds value to sorted list, if the same value was not added before exists */
static zb_uint8_t add_ep_sorted(zb_uint8_t *ep_list, zb_uint8_t ep_num, zb_uint8_t endpoint)
{
    zb_uindex_t i;
    zb_uindex_t index;

    TRACE_MSG(TRACE_ZDO3, "add_ep_sorted ep_num %hd, ep %hd", (FMT__H_H, ep_num, endpoint));
    for (index = 0; index < ep_num; index++)
    {
        if (endpoint <= ep_list[index])
        {
            break;
        }
    }
    TRACE_MSG(TRACE_ZDO3, "index %hd, ep[i] %hd", (FMT__H_H, index, ep_list[index]));
    if (ep_list[index] != endpoint)
    {
        for (i = ep_num; i > index; i--)
        {
            ep_list[i] = ep_list[i - 1U];
        }
        ep_list[index] = endpoint;
        ep_num++;
    }
    TRACE_MSG(TRACE_ZDO3, "<< add_ep_sorted ret %hd", (FMT__H, ep_num));
    return ep_num;
}

zb_uint8_t *zb_copy_cluster_id(zb_uint8_t *cluster_dst, zb_uint8_t *cluster_src, zb_uint8_t cluster_num)
{
    zb_uindex_t i;
    for (i = 0; i < cluster_num; i++)
    {
        ZB_HTOLE16(cluster_dst + i * sizeof(zb_uint16_t), cluster_src + i * sizeof(zb_uint16_t));
    }
    return (zb_uint8_t *)(cluster_src + cluster_num * sizeof(zb_uint16_t));
}

#ifdef DSR_IOT_ECOSYSTEM
/* EES: Dirty workaround for old Centralite devices (leak, door, motion, etc.)
 *      This devices send broadcast Match Descriptor request
 *      for specific output clusters 0xfc0f and 0xfc46. If we don't send any response
 *      (obviously as unicast) than device send Leave after several attempts.
 *      Let's response for those requests.
 */
static zb_uint8_t dsr_iot_ecosystem_check_cpecific_match_descriptor_request(
    zb_uint8_t num_in_clusters,
    zb_uint8_t num_out_clusters,
    zb_uint16_t *cluster_id)
{
    zb_uint8_t ret = 0;

    if (num_in_clusters == 0 && num_out_clusters == 1 &&
            (cluster_id[num_in_clusters] == 0xfc0f ||
             cluster_id[num_in_clusters] == 0xfc46))
    {
        ret = 1;
    }
    return ret;
}

#endif  /* DSR_IOT_ECOSYSTEM */

void zdo_match_desc_res(zb_uint8_t param)
{
    zb_uint8_t status = ZB_ZDP_STATUS_SUCCESS;
    zb_apsde_data_indication_t *ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);
    zb_zdo_match_desc_param_t match_params;
    zb_uint16_t *cluster_id;
    zb_uint8_t *aps_body;
    zb_uint8_t tsn;
    /* Source address of the Match Desc Req originator */
    zb_uint16_t req_origin_addr;
    /* NWK destination address of the Match Desc Req */
    zb_uint16_t nwk_dst_addr;
    zb_zdo_match_desc_resp_t *match_resp;
    zb_uint8_t ep_num = 0;
    zb_uint8_t ep_list[ZB_MAX_EP_NUMBER];
    zb_uindex_t i, j, k;
    zb_bool_t drop_packet = ZB_FALSE;

    TRACE_MSG(TRACE_ZDO2, ">> zdo_match_desc_res %hd", (FMT__H, param));

    ZB_BZERO(ep_list, sizeof(ep_list));

    if (zb_buf_len(param) < 2U * sizeof(zb_uint16_t) + 3U * sizeof(zb_uint8_t))
    {
        /* Minimum payload is: tsn| nwk_addr_of_interest| profile_id| in_clusters_count| out_clusters_count */
        drop_packet = ZB_TRUE;
    }
    else
    {
        /* Check that request has input cluster list with size = 2*mun_in_clusters
           and 1 byte for num_out_clusters field.
           Also check that size of output_cluster_list = 2*num_out_clusters*/
        zb_uint8_t num_in_clusters, num_out_clusters;
        zb_size_t req_tail_size;
        zb_uint8_t *ptr = zb_buf_begin(param);

        num_in_clusters = *(ptr + 5);
        req_tail_size = zb_buf_len(param) - 6U;
        if (req_tail_size < num_in_clusters * sizeof(zb_uint16_t) + sizeof(zb_uint8_t))
        {
            drop_packet = ZB_TRUE;
        }

        num_out_clusters = *(ptr + 6 + num_in_clusters * sizeof(zb_uint16_t));
        req_tail_size = zb_buf_len(param) - 7U - num_in_clusters * sizeof(zb_uint16_t);
        if (req_tail_size < num_out_clusters * sizeof(zb_uint16_t))
        {
            drop_packet = ZB_TRUE;
        }
    }

    if (drop_packet)
    {
        TRACE_MSG(TRACE_ZDO3, "Malformed packet - drop", (FMT__0));
        zb_buf_free(param);
        return;
    }

    /* Parse incoming Match Descriptor Request command */
    aps_body = zb_buf_begin(param);
    tsn = *aps_body;
    aps_body++;

    ZB_HTOLE16((zb_uint8_t *)&match_params.addr_of_interest, aps_body);
    aps_body += sizeof(zb_uint16_t);
    ZB_HTOLE16((zb_uint8_t *)&match_params.profile_id, aps_body);
    aps_body += sizeof(zb_uint16_t);
    match_params.num_in_clusters = *aps_body;
    aps_body++;

    match_params.num_out_clusters = *(aps_body + match_params.num_in_clusters * sizeof(zb_uint16_t));

    req_origin_addr = ind->src_addr;
    nwk_dst_addr = ind->dst_addr;

    TRACE_MSG(TRACE_ZDO3, "addr_of_interest 0x%x, profile_id 0x%x, num in clust %hd, num out clust %hd",
              (FMT__D_D_H_H, match_params.addr_of_interest, match_params.profile_id,
               match_params.num_in_clusters, match_params.num_out_clusters));

    /*
       Buffer tail is used as a temporary storage for the clusters list.
       For me looks like quite dirty solution.
       Rewrite it!
     */
    /*
      cluster_id is really aligned to 2 here: every buffer is aligned, subtract
      multiple of 2 bytes from its tail.
      but aps_body does not.
    */
    {
        zb_size_t num_in_out_clusters = (zb_size_t)match_params.num_in_clusters;
        num_in_out_clusters += (zb_size_t)match_params.num_out_clusters;
        num_in_out_clusters *= sizeof(zb_uint16_t);
        cluster_id = zb_buf_get_tail(param, num_in_out_clusters);
    }

    if (match_params.num_in_clusters > 0U)
    {
        aps_body = zb_copy_cluster_id((zb_uint8_t *)cluster_id, aps_body, match_params.num_in_clusters);
        TRACE_MSG(TRACE_ZDO3, "1st in cluster_id 0x%x", (FMT__D, *cluster_id));
    }
    if (match_params.num_out_clusters > 0U)
    {
        aps_body++; /* num_out_clusters field */
        aps_body = zb_copy_cluster_id((zb_uint8_t *)(cluster_id + match_params.num_in_clusters),
                                      aps_body, match_params.num_out_clusters);
        TRACE_MSG(TRACE_ZDO3, "1st out cluster_id 0x%x", (FMT__D, cluster_id[match_params.num_in_clusters]));
    }
    ZVUNUSED(aps_body);

    /* R21: 2.4.4.2.7 Match_Desc_rsp */
    /* We are using simplified logic, as far as we have no support for
     * storing simple descriptor of remote device. It's optional and we
     * haven't implemented this (according to previous DA comment). At
     * this point, processing algorithm becomes shorter and quite simplier.
     */

    if ((ZB_PIBCACHE_NETWORK_ADDRESS() != match_params.addr_of_interest) &&
            (!(ZB_NWK_IS_ADDRESS_BROADCAST(match_params.addr_of_interest) )))
    {
        if (ZB_IS_DEVICE_ZED())
        {
            /* Althought the analysis of the broadcast NWK destination address
             * should be done in the step 2.a., it's moved to the bottom of
             * the function to keep the code consistent. (This is regaring
             * simplification noted above). */
            status = ZB_ZDP_STATUS_INV_REQUESTTYPE;
            TRACE_MSG(TRACE_ZDO3, "Match_desc: invalid requesttype", (FMT__0));
        }
#ifdef ZB_ROUTER_ROLE
        else
        {
            if (zb_nwk_get_nbr_rel_by_short(match_params.addr_of_interest) == ZB_NWK_RELATIONSHIP_CHILD)
            {
                status = ZB_ZDP_STATUS_NO_DESCRIPTOR;
                TRACE_MSG(TRACE_ZDO3, "Match_desc: request for our child, but no simple desc available", (FMT__0));
            }
            else
            {
                status = ZB_ZDP_STATUS_DEVICE_NOT_FOUND;
                TRACE_MSG(TRACE_ZDO3, "Match_desc: device not found", (FMT__0));
            }
        }
#endif
    }
    else
    {
        zb_bool_t found;

        TRACE_MSG(TRACE_ZDO3, "simple desc num %hd", (FMT__H, ZB_ZDO_SIMPLE_DESC_NUMBER()));
        for (i = 0; i < ZB_ZDO_SIMPLE_DESC_NUMBER(); i++)
        {
            found = ZB_FALSE;
            TRACE_MSG(TRACE_ZDO3, "app_profile_id %d", (FMT__D, ZB_ZDO_SIMPLE_DESC_LIST()[i]->app_profile_id));
            if (ZB_ZDO_SIMPLE_DESC_LIST()[i]->app_profile_id == match_params.profile_id
                    /* Wildcard profile id.
                       See 2.4.4.1.7.1:

                       If the profile ID field does not match exactly the remote device
                       shall check if the Profile ID of the Match_desc_req matches the
                       wildcard profile (0xFFFF) and the Profile ID of the Simple
                       Descriptor is within the Standard range (a public profile) as
                       dictated by document [B5]

                     */
                    || match_params.profile_id == ZB_AF_WILDCARD_PROFILE_ID)
            {
                TRACE_MSG(TRACE_ZDO3, "app in cluster count %hd", (FMT__H, ZB_ZDO_SIMPLE_DESC_LIST()[i]->app_input_cluster_count));
                for (j = 0; j < ZB_ZDO_SIMPLE_DESC_LIST()[i]->app_input_cluster_count && !found; j++)
                {
                    for (k = 0; k < match_params.num_in_clusters && !found; k++)
                    {
                        if ((ZDO_CLUSTER_FILTERED_OUT(cluster_id[k]) == 0)
                                && ZB_ZDO_SIMPLE_DESC_LIST()[i]->app_cluster_list[j] == cluster_id[k])
                        {
                            ep_num = add_ep_sorted(ep_list, ep_num, ZB_ZDO_SIMPLE_DESC_LIST()[i]->endpoint);
                            found = ZB_TRUE;
                        }
                    }
                }  /* for j .. app_input_cluster_count */

                TRACE_MSG(TRACE_ZDO3, "app out cluster count %hd", (FMT__H, ZB_ZDO_SIMPLE_DESC_LIST()[i]->app_output_cluster_count));
                for (j = 0; j < ZB_ZDO_SIMPLE_DESC_LIST()[i]->app_output_cluster_count && !found; j++)
                {
                    for (k = 0; k < match_params.num_out_clusters && !found; k++)
                    {
                        if ((ZDO_CLUSTER_FILTERED_OUT(cluster_id[match_params.num_in_clusters + k]) == 0)
                                &&
                                ZB_ZDO_SIMPLE_DESC_LIST()[i]->app_cluster_list[ZB_ZDO_SIMPLE_DESC_LIST()[i]->app_input_cluster_count + j] ==
                                cluster_id[match_params.num_in_clusters + k])
                        {
                            ep_num = add_ep_sorted(ep_list, ep_num, ZB_ZDO_SIMPLE_DESC_LIST()[i]->endpoint);
                            found = ZB_TRUE;
                        }
                    }
                } /* for j .. app_output_cluster_count */
            } /* if app_profile_id == profile_id */
        } /* for i .. simple_desc_number */
    }

    TRACE_MSG(TRACE_ZDO3, "ep_num %hd", (FMT__H, ep_num));
    if (ep_num == 0U && ZB_NWK_IS_ADDRESS_BROADCAST(nwk_dst_addr)
#ifdef DSR_IOT_ECOSYSTEM
            /* EES: Dirty workaround for old Centralite devices (leak, door, motion, etc.)
             *      This devices send broadcast Match Descriptor request
             *      for specific output cluster 0x0ffc. If we don't send any response (obviously
             *      as unicast) than device send Leave after several attempts.
             *      Let's response for those requests.
             */
            && !dsr_iot_ecosystem_check_cpecific_match_descriptor_request(match_params.num_in_clusters,
                    match_params.num_out_clusters,
                    cluster_id)
#endif  /* DSR_IOT_ECOSYSTEM */
       )
    {
        /* Done with this request */
        TRACE_MSG(TRACE_ZDO1, "Broadcast request, no match", (FMT__0));
        zb_buf_free(param);
    }
    else
    {
        /* Send response */
        match_resp = zb_buf_initial_alloc(param, sizeof(zb_zdo_match_desc_resp_t) + ep_num * sizeof(zb_uint8_t));
        if (ZB_NWK_IS_ADDRESS_BROADCAST(match_params.addr_of_interest))
        {
            ZB_HTOLE16_VAL((zb_uint8_t *)&match_resp->nwk_addr, ZB_PIBCACHE_NETWORK_ADDRESS());
        }
        else
        {
            ZB_HTOLE16_VAL((zb_uint8_t *)&match_resp->nwk_addr, match_params.addr_of_interest);
        }
        match_resp->tsn = tsn;
        match_resp->match_len = ep_num;
        ZB_MEMCPY((zb_uint8_t *)(match_resp + 1), ep_list, ep_num * sizeof(zb_uint8_t));

        /* R21, 2.4.4.2.7.1: Status should be success even if MatchLength == 0 */
        /* status = (ep_num == 0 && status == ZB_ZDP_STATUS_SUCCESS) ? (ZB_ZDP_STATUS_NO_DESCRIPTOR) : (status); */
        TRACE_MSG(TRACE_ZDO2, "status %hd", (FMT__H, status));

        match_resp->status = status;

        zdo_send_resp_by_short(ZDO_MATCH_DESC_RESP_CLID, param, req_origin_addr);
    }

    TRACE_MSG(TRACE_ZDO2, "<< zdo_match_desc_res", (FMT__0));
}

/*! @} */
