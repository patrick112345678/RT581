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
/* PURPOSE: ZGP stub functionality: direct communication with ZGPD
*/

#define ZB_TRACE_FILE_ID 2106

#include "zb_common.h"

#ifdef ZB_ENABLE_ZGP
#ifdef ZB_ENABLE_ZGP_DIRECT
/** @addtogroup zgp_stub_int */
/** @{ */
#include "zboss_api_zgp.h"
#include "zgp/zgp_internal.h"
#include "zb_mac.h"
#include "zb_osif.h"

#ifndef ZB_ZGP_CHECK_NON_SUCCESS_CONFIRM_STATUS
#define ZB_ZGP_CHECK_NON_SUCCESS_CONFIRM_STATUS(status) (1)
#endif


/**
 * @brief Fill GPDF NWK header for outgoing broadcast message
 *
 * Broadcast ZGP messages can be sent in the following ways:
 *   - using maintenance frame
 *   - using data frame with ZB_ZGP_SRC_ID_ALL Src ID destination with App ID 0000
 *
 * Security processing is not performed for maintenance frames.
 * Data frames with ZB_ZGP_SRC_ID_ALL can be secured, but it's not obvious in which
 * cases. So they are not secured in current implementation, too.
 *
 * @param buf         [in,out] Buffer with GPDF payload. NWK GPDF header will be added before
 *                             payload in buffer data
 * @param frame_type  [in]     Type of the GPDF frame (@ref enum zb_gpdf_frame_type_e)
 *
 * @return            Number of bytes written to the NWK header
 */
static zb_uint8_t fill_broadcast_gpdf_nwk_hdr(zb_bufid_t buf, zb_uint8_t frame_type)
{
    zb_uint8_t *ptr;

    zb_uint8_t src_id_fld_size = ZGPD_SRC_ID_SIZE(
                                     ZB_ZGP_APP_ID_0000,
                                     frame_type);

    zb_uint8_t ext_present = (frame_type == ZGP_FRAME_TYPE_DATA ? 1 : 0);

    zb_uint8_t zgp_nwk_hdr_len =
        1 //NWK frame control
        + ext_present //Extended NWK frame control
        + src_id_fld_size;

    TRACE_MSG(TRACE_ZGP1, ">> fill_broadcast_gpdf_nwk_hdr buf %p, frame_type %hd",
              (FMT__P_H, buf, frame_type));

    ptr = zb_buf_alloc_left(buf, zgp_nwk_hdr_len);
    ZB_GPDF_NWK_FRAME_CONTROL(*ptr,
                              frame_type,
                              0, /* auto-commissioning */
                              ext_present); /* Extended NWK frame control */

    if (ext_present)
    {
        ptr++;

        ZB_GPDF_NWK_FRAME_CTL_EXT(*ptr,
                                  ZB_ZGP_APP_ID_0000,
                                  0,  /* security level */
                                  0,  /* security key */
                                  0,
                                  ZGP_FRAME_DIR_TO_ZGPD);
        ptr++;

        if (src_id_fld_size > 0)
        {
            zb_uint32_t broadcast_src_id = ZB_ZGP_SRC_ID_ALL;
            ZB_HTOLE32(ptr, &broadcast_src_id);
            ptr += 4;
            ZVUNUSED(ptr);
        }
    }

    TRACE_MSG(TRACE_ZGP1, "<< fill_broadcast_gpdf_nwk_hdr, ret %hd", (FMT__H, zgp_nwk_hdr_len));

    return zgp_nwk_hdr_len;
}

/**
 * @brief Fill GPDF NWK header for outgoing unicast message
 *
 * NWK header is filled based on GPD frame that triggered its sending.
 *
 * @param buf        [in,out] Buffer with GPDF payload. NWK GPDF header will be added before
 *                             payload in buffer data
 * @param recv_gpdf  [in]     GPDF info about received GPDF, that triggered transmission
 *
 * @return            Number of bytes written to the NWK header
 */



static zb_uint8_t fill_unicast_gpdf_nwk_hdr(zb_bufid_t buf, zb_gpdf_info_t *recv_gpdf)
{
    zb_uint8_t *ptr;
    zb_uint8_t frame_type = ZGP_FRAME_TYPE_DATA;
    zb_uint8_t sec_level = ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(recv_gpdf->nwk_ext_frame_ctl);

    zb_uint8_t src_id_fld_size = ZGPD_SRC_ID_SIZE(
                                     ZB_GPDF_EXT_NFC_GET_APP_ID(recv_gpdf->nwk_ext_frame_ctl),
                                     frame_type);

    zb_uint8_t zgp_nwk_hdr_len =
        1 //NWK frame control
        + 1 //Extended NWK frame control
        + src_id_fld_size
        + ((ZB_GPDF_EXT_NFC_GET_APP_ID(recv_gpdf->nwk_ext_frame_ctl) == ZB_ZGP_APP_ID_0010) ? 1 : 0)
        + GPDF_SECURITY_FRAME_COUNTER_SIZE(sec_level);



    TRACE_MSG(TRACE_ZGP1, ">> fill_unicast_gpdf_nwk_hdr buf %p, recv_gpdf %p",
              (FMT__P_P, buf, recv_gpdf));

    ptr = zb_buf_alloc_left(buf, zgp_nwk_hdr_len);

    ZB_GPDF_NWK_FRAME_CONTROL(*ptr,
                              frame_type,
                              0, /* auto-commissioning */
                              1); /* Extended NWK frame control */
    ptr++;

    ZB_GPDF_NWK_FRAME_CTL_EXT(*ptr,
                              ZB_GPDF_EXT_NFC_GET_APP_ID(recv_gpdf->nwk_ext_frame_ctl),
                              sec_level,
                              ZB_GPDF_EXT_NFC_GET_SEC_KEY(recv_gpdf->nwk_ext_frame_ctl),
                              0,
                              ZGP_FRAME_DIR_TO_ZGPD);
    ptr++;

    if (src_id_fld_size > 0)
    {
        ZB_HTOLE32(ptr, (zb_uint8_t *)&recv_gpdf->zgpd_id.addr.src_id);
        ptr += 4;
    }

    /* [AEV] fix incorrect frame generating when APPID 010 start*/
    if (ZB_GPDF_EXT_NFC_GET_APP_ID(recv_gpdf->nwk_ext_frame_ctl) == ZB_ZGP_APP_ID_0010)
    {
        *ptr = recv_gpdf->zgpd_id.endpoint;
        ptr++;
    }

    if (sec_level > ZB_ZGP_SEC_LEVEL_REDUCED)
    {
        ZB_HTOLE32(ptr, (zb_uint8_t *)&recv_gpdf->sec_frame_counter);
    }

    TRACE_MSG(TRACE_ZGP1, "<< fill_unicast_gpdf_nwk_hdr, ret %hd", (FMT__H, zgp_nwk_hdr_len));

    return zgp_nwk_hdr_len;
}

static void cgp_data_req(zb_uint8_t param)
{
    zb_cgp_data_req_t          cgp_d_r;
    zb_mcps_data_req_params_t *mac_data_req;

    TRACE_MSG(TRACE_ZGP3, ">> cgp_data_req param %hd", (FMT__H, param));

    ZB_MEMCPY(&cgp_d_r, ZB_BUF_GET_PARAM(param, zb_cgp_data_req_t), sizeof(zb_cgp_data_req_t));

    mac_data_req = ZB_BUF_GET_PARAM(param, zb_mcps_data_req_params_t);
    ZB_BZERO(mac_data_req, sizeof(zb_mcps_data_req_params_t));

    mac_data_req->src_addr_mode = cgp_d_r.src_addr_mode;
    ZB_64BIT_ADDR_COPY(mac_data_req->src_addr.addr_long, &cgp_d_r.src_addr);

    mac_data_req->dst_addr_mode = cgp_d_r.dst_addr_mode;
    ZB_64BIT_ADDR_COPY(mac_data_req->dst_addr.addr_long, &cgp_d_r.dst_addr);
    mac_data_req->dst_pan_id = cgp_d_r.dst_pan_id;

    ZB_64BIT_ADDR_COPY(mac_data_req->src_addr.addr_long, &cgp_d_r.src_addr);

    if ((cgp_d_r.tx_options & ZB_CGP_DATA_REQ_USE_MAC_ACK_BIT) &&
            (mac_data_req->dst_addr_mode == ZB_ADDR_64BIT_DEV))
    {
        mac_data_req->tx_options |= MAC_TX_OPTION_ACKNOWLEDGED_BIT;
    }

    if (!(cgp_d_r.tx_options & ZB_CGP_DATA_REQ_USE_CSMA_CA_BIT))
    {
        /* Don't use CSMA/CA for GreenPower frames (see ZGP spec, A.1.5.2.2) */
        mac_data_req->tx_options |= MAC_TX_OPTION_NO_CSMA_CA;
        /* Let MAC decide when to send */
        mac_data_req->src_addr.tx_at = cgp_d_r.recv_timestamp + ZB_GPD_TX_OFFSET_US;
    }

    mac_data_req->msdu_handle = cgp_d_r.handle;

    zb_mcps_data_request(param);

    TRACE_MSG(TRACE_ZGP3, "<< cgp_data_req", (FMT__0));
}

#ifdef ZB_ENABLE_ZGP_TEST_HARNESS
static zb_uint8_t fill_gpdf_nwk_hdr(zb_bufid_t buf, zb_outgoing_gpdf_info_t *gpdf_info)
{
    zb_uint8_t *ptr;
    zb_uint8_t  zgp_nwk_hdr_len = 1;  /* NWK frame control at least */
    zb_uint8_t  src_addr_size = 0;
    zb_uint8_t  endpoint_size = 0;
    zb_uint8_t  sec_frame_counter_size = 0;

    TRACE_MSG(TRACE_ZGP2, ">> fill_gpdf_nwk_hdr", (FMT__0));

    zgp_nwk_hdr_len += (ZB_GPDF_NFC_GET_NFC_EXT(gpdf_info->nwk_frame_ctl) == 1) ? 1 : 0;

    if (ZB_GPDF_NFC_GET_FRAME_TYPE(gpdf_info->nwk_frame_ctl) != ZGP_FRAME_TYPE_MAINTENANCE)
    {
        if (ZB_GPDF_EXT_NFC_GET_APP_ID(gpdf_info->nwk_ext_frame_ctl) == ZB_ZGP_APP_ID_0000)
        {
            src_addr_size = 4;
            zgp_nwk_hdr_len += src_addr_size;
        }
        else if (ZB_GPDF_EXT_NFC_GET_APP_ID(gpdf_info->nwk_ext_frame_ctl) == ZB_ZGP_APP_ID_0010)
        {
            endpoint_size = 1;
            zgp_nwk_hdr_len += endpoint_size;
        }

        if (ZB_GPDF_NFC_GET_NFC_EXT(gpdf_info->nwk_frame_ctl))
        {
            zb_uint8_t sec_lvl = ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl);

            sec_frame_counter_size = GPDF_SECURITY_FRAME_COUNTER_SIZE(sec_lvl);
            zgp_nwk_hdr_len += sec_frame_counter_size;
        }
    }

    ZB_BUF_ALLOC_LEFT(buf, zgp_nwk_hdr_len, ptr);

    ZGP_GPDF_NWK_PUT_FCTL(ptr, gpdf_info->nwk_frame_ctl);

    if (ZB_GPDF_NFC_GET_NFC_EXT(gpdf_info->nwk_frame_ctl))
    {
        ZGP_GPDF_NWK_PUT_EFCTL(ptr, gpdf_info->nwk_ext_frame_ctl);
    }

    if (src_addr_size)
    {
        ZGP_GPDF_NWK_PUT_SRC_ADDR(ptr, &gpdf_info->addr.src_id);
    }

    if (endpoint_size)
    {
        ZGP_GPDF_NWK_PUT_ENDP(ptr, gpdf_info->endpoint);
    }

    if (sec_frame_counter_size)
    {
        ZGP_GPDF_NWK_PUT_SFCNT(ptr, &gpdf_info->sec_frame_counter);
    }

    TRACE_MSG(TRACE_ZGP2, "<< fill_gpdf_nwk_hdr %hd", (FMT__H, zgp_nwk_hdr_len));
    return zgp_nwk_hdr_len;
}

static void zgp_send_gpdf_on_tx_channel(zb_uint8_t buf_ref);

static void zgp_send_gpdf_pib_channel_cb(zb_uint8_t buf_ref)
{
    zb_mlme_get_confirm_t   *cfm = (zb_mlme_get_confirm_t *)zb_buf_begin(param);
    zb_outgoing_gpdf_info_t *gpdf_info = &ZGP_CTXC().out_gpdf_info;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_send_gpdf_pib_channel_cb %hd", (FMT__H, buf_ref));

    ZB_ASSERT(cfm->pib_attr == ZB_PHY_PIB_CURRENT_CHANNEL);

    if (cfm->status == MAC_SUCCESS)
    {
        switch (gpdf_info->state)
        {
        case ZB_OUTGOING_GPDF_STATE_GET_OPER_CHANNEL:
            ZGP_OUT_GPDF_INFO_SET_OPER_CHANNEL(gpdf_info->channel_info,
                                               *(zb_uint8_t *)(cfm + 1));

            if (ZGP_OUT_GPDF_INFO_GET_OPER_CHANNEL(gpdf_info->channel_info) !=
                    ZGP_OUT_GPDF_INFO_GET_TEMP_CHANNEL(gpdf_info->channel_info))
            {
                gpdf_info->state = ZB_OUTGOING_GPDF_STATE_SET_TEMP_CHANNEL;
                gpdf_info->channel = ZGP_OUT_GPDF_INFO_GET_TEMP_CHANNEL(gpdf_info->channel_info);
                zb_nwk_pib_set(buf_ref, ZB_PHY_PIB_CURRENT_CHANNEL,
                               (void *)&gpdf_info->channel, sizeof(zb_uint8_t),
                               zgp_send_gpdf_pib_channel_cb);
            }
            else
            {
                zgp_send_gpdf_on_tx_channel(buf_ref);
            }
            break;
        case ZB_OUTGOING_GPDF_STATE_SET_TEMP_CHANNEL:
            zgp_send_gpdf_on_tx_channel(buf_ref);
            break;
        case ZB_OUTGOING_GPDF_STATE_SET_OPER_CHANNEL:
            if (gpdf_info->cb)
            {
                (*gpdf_info->cb)(buf_ref, ZB_OUTGOING_GPDF_STATUS_SUCCESS);
            }
            else
            {
                zb_buf_free(param);
            }
            gpdf_info->buf_ref = 0;
            break;
        default:
            ZB_ASSERT(0);
        };
    }
    else
    {
        if (gpdf_info->cb)
        {
            (*gpdf_info->cb)(buf_ref, ZB_OUTGOING_GPDF_STATUS_PIB_ERROR);
        }
        else
        {
            zb_buf_free(param);
        }
        gpdf_info->buf_ref = 0;
    }

    TRACE_MSG(TRACE_ZGP2, "<< zgp_send_gpdf_pib_channel_cb", (FMT__0));
}

static void zgp_send_gpdf_on_tx_channel_cfm(zb_uint8_t buf_ref)
{
    zb_outgoing_gpdf_info_t *gpdf_info = &ZGP_CTXC().out_gpdf_info;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_send_gpdf_on_tx_channel_cfm %hd", (FMT__H, buf_ref));

    if (ZGP_OUT_GPDF_INFO_GET_OPER_CHANNEL(gpdf_info->channel_info) !=
            ZGP_OUT_GPDF_INFO_GET_TEMP_CHANNEL(gpdf_info->channel_info))
    {
        gpdf_info->state = ZB_OUTGOING_GPDF_STATE_SET_OPER_CHANNEL;
        gpdf_info->channel = ZGP_OUT_GPDF_INFO_GET_OPER_CHANNEL(gpdf_info->channel_info);
        zb_nwk_pib_set(buf_ref, ZB_PHY_PIB_CURRENT_CHANNEL,
                       (void *)&gpdf_info->channel, sizeof(zb_uint8_t),
                       zgp_send_gpdf_pib_channel_cb);
    }
    else
    {
        if (gpdf_info->cb)
        {
            (*gpdf_info->cb)(buf_ref, ZB_OUTGOING_GPDF_STATUS_SUCCESS);
            gpdf_info->buf_ref = 0;
        }
        else
        {
            zb_free_buf(ZB_BUF_FROM_REF(buf_ref));
        }
    }

    TRACE_MSG(TRACE_ZGP2, "<< zgp_send_gpdf_on_tx_channel_cfm", (FMT__0));
}

static void zgp_send_gpdf_on_tx_channel(zb_uint8_t buf_ref)
{
    zb_uint8_t                *ptr;
    zb_outgoing_gpdf_info_t   *gpdf_info = &ZGP_CTXC().out_gpdf_info;
    zb_mcps_data_req_params_t *mac_data_req;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_send_gpdf_on_tx_channel %hd", (FMT__H, buf_ref));

    {
        zb_uint8_t nwk_hdr_len;

        ZB_BUF_INITIAL_ALLOC(buf, gpdf_info->payload_len, ptr);
        ZB_MEMCPY(ptr, gpdf_info->payload, gpdf_info->payload_len);

        nwk_hdr_len = fill_gpdf_nwk_hdr(buf, gpdf_info);

        if (zb_zgp_protect_out_gpdf(buf_ref, gpdf_info, NULL, nwk_hdr_len) != RET_OK)
        {
            if (gpdf_info->cb)
            {
                (*gpdf_info->cb)(buf_ref, ZB_OUTGOING_GPDF_STATUS_ENC_ERROR);
            }
            else
            {
                zb_buf_free(param);
            }
            gpdf_info->buf_ref = 0;
        }
    }

    mac_data_req = ZB_BUF_GET_PARAM(param, zb_mcps_data_req_params_t);
    ZB_BZERO(mac_data_req, sizeof(zb_mcps_data_req_params_t));

    mac_data_req->src_addr_mode = ZB_ADDR_NO_ADDR;

    if ((ZB_GPDF_EXT_NFC_GET_APP_ID(gpdf_info->nwk_ext_frame_ctl) == ZB_ZGP_APP_ID_0010))
    {
        mac_data_req->dst_addr_mode = ZB_ADDR_64BIT_DEV;
        ZB_64BIT_ADDR_COPY(mac_data_req->dst_addr.addr_long, gpdf_info->addr.ieee_addr);
    }
    else
    {
        mac_data_req->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        mac_data_req->dst_addr.addr_short = 0xffff;
    }

    mac_data_req->dst_pan_id = 0xffff;

    if ((gpdf_info->tx_options & ZB_CGP_DATA_REQ_USE_MAC_ACK_BIT) &&
            (mac_data_req->dst_addr_mode == ZB_ADDR_64BIT_DEV))
    {
        mac_data_req->tx_options |= MAC_TX_OPTION_ACKNOWLEDGED_BIT;
    }

    mac_data_req->tx_options |= MAC_TX_OPTION_NO_CSMA_CA;
    mac_data_req->msdu_handle = ZB_MAC_DIRECT_GPDF_MSDU_HANDLE;

    zb_mcps_data_request(buf_ref);

    TRACE_MSG(TRACE_ZGP2, "<< zgp_send_gpdf_on_tx_channel", (FMT__0));
}

zb_ret_t zgp_send_gpdf(zb_uint8_t buf_ref, zb_outgoing_gpdf_info_t *gpdf_info)
{
    zb_ret_t ret = RET_ERROR;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_send_gpdf %hd", (FMT__H, buf_ref));

#ifdef ZB_MAC_OK_TO_SEND_ZGP
    if ((gpdf_info->tx_options & MAC_TX_OPTION_NO_CSMA_CA) &&
            ZB_MAC_OK_TO_SEND_ZGP()))
#endif
    {
        ZGP_OUT_GPDF_INFO_SET_TEMP_CHANNEL(gpdf_info->channel_info, gpdf_info->channel);
        gpdf_info->state = ZB_OUTGOING_GPDF_STATE_GET_OPER_CHANNEL;
        gpdf_info->channel = 0;
        gpdf_info->buf_ref = buf_ref;
        ZB_MEMCPY(&ZGP_CTXC().out_gpdf_info, gpdf_info, sizeof(zb_outgoing_gpdf_info_t));
        zb_nwk_pib_get(buf_ref, ZB_PHY_PIB_CURRENT_CHANNEL, zgp_send_gpdf_pib_channel_cb);
        ret = RET_OK;
    }

    TRACE_MSG(TRACE_ZGP2, "<< zgp_send_gpdf", (FMT__0));

    return ret;
}
#endif  /* ZB_ENABLE_ZGP_TEST_HARNESS */

/* TODO: pass needed parameters instead of gpdf_info. */
static void prepare_buf_for_cpg_data_req(zb_bufid_t buf, zb_gp_data_req_t *gp_data_req,
        zb_uint8_t tx_p_i_pos, zb_gpdf_info_t *gpdf_info)
{
    zb_uint8_t         cmd_id;
    zb_uint8_t         payload_len;
    zb_uint8_t        *payload;
    zb_uint8_t         gp_d_r_tx_options;
    zb_zgp_tx_q_ent_t *tx_q_ent;
    zb_zgp_tx_pinfo_t *tx_p_i_ent;
    zb_uint8_t        *ptr;
    zb_cgp_data_req_t  cgp_req;

    TRACE_MSG(TRACE_ZGP3, ">> prepare_buf_for_cpg_data_req buf %p gp_data_req %p pos %d gpdf_info %p",
              (FMT__P_P_D_P, buf, gp_data_req, tx_p_i_pos, gpdf_info));

#ifndef ZB_ZGP_IMMED_TX
    ZVUNUSED(gp_data_req);
#endif

    ZB_ASSERT((!gp_data_req) == (tx_p_i_pos < ZB_ZGP_TX_QUEUE_SIZE));
#ifdef ZB_ZGP_IMMED_TX
    ZB_ASSERT(!!gp_data_req ==
              ((tx_p_i_pos >= ZB_ZGP_TX_QUEUE_SIZE) && (tx_p_i_pos < ZB_ZGP_TX_PACKET_INFO_COUNT)));
#else
    ZB_ASSERT(!gp_data_req);
#endif

    tx_p_i_ent = &ZGP_CTXC().tx_packet_info_queue.queue[tx_p_i_pos];

#ifdef ZB_ZGP_IMMED_TX
    if (gp_data_req)
    {
        cmd_id            = gp_data_req->cmd_id;
        payload_len       = gp_data_req->payload_len;
        payload           = gp_data_req->pld;
        gp_d_r_tx_options = gp_data_req->tx_options;
    }
    else
#endif
    {
        tx_q_ent = &ZGP_CTXC().tx_queue.queue[tx_p_i_pos];
        cmd_id            = tx_q_ent->cmd_id;
        payload_len       = tx_q_ent->payload_len;
        payload           = tx_q_ent->pld;
        gp_d_r_tx_options = tx_q_ent->tx_options;
    }

    /* init cgp_data_req */
    ZB_BZERO(&cgp_req, sizeof(zb_cgp_data_req_t));
    if (gp_d_r_tx_options & ZB_GP_DATA_REQ_USE_CSMA_CA_BIT)
    {
        cgp_req.tx_options |= ZB_CGP_DATA_REQ_USE_CSMA_CA_BIT;
    }
    if (gp_d_r_tx_options & ZB_GP_DATA_REQ_USE_MAC_ACK_BIT)
    {
        cgp_req.tx_options |= ZB_CGP_DATA_REQ_USE_MAC_ACK_BIT;
    }

    if ((ZB_GPDF_EXT_NFC_GET_APP_ID(gpdf_info->nwk_ext_frame_ctl) != ZB_ZGP_APP_ID_0000)
            || (gp_d_r_tx_options & ZB_GP_DATA_REQ_USE_MAC_ACK_BIT))
    {
        cgp_req.dst_addr_mode = ZB_ADDR_64BIT_DEV;
        cgp_req.dst_pan_id = 0xffff;//gpdf_info->mac_addr_flds.comb.dst_pan_id;

        ZB_64BIT_ADDR_COPY(gpdf_info->mac_addr_flds.l.addr,
                           &gpdf_info->zgpd_id.addr);
        ZB_64BIT_ADDR_COPY(&cgp_req.dst_addr, &gpdf_info->zgpd_id.addr);

        gpdf_info->mac_addr_flds_len = sizeof(gpdf_info->mac_addr_flds.l);
    }
    else
    {
        cgp_req.dst_addr_mode       = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        cgp_req.dst_addr.addr_short = gpdf_info->mac_addr_flds.s.dst_addr = ZB_NWK_BROADCAST_ALL_DEVICES;
        cgp_req.dst_pan_id          = gpdf_info->mac_addr_flds.s.dst_pan_id = ZB_BROADCAST_PAN_ID;
        gpdf_info->mac_addr_flds_len = sizeof(gpdf_info->mac_addr_flds.s);
    }
    cgp_req.src_addr_mode = ZB_ADDR_NO_ADDR;
    cgp_req.buf_ref       = ZGP_CTXC().tx_packet_info_queue.queue[tx_p_i_pos].buf_ref;
    cgp_req.handle        = ZB_MAC_DIRECT_GPDF_MSDU_HANDLE;

    /* fill buffer with command id and payload */
    tx_p_i_ent->buf_ref = buf;
    ptr = zb_buf_initial_alloc(buf, payload_len + 1);
    *ptr++ = cmd_id;
    ZB_MEMCPY(ptr, payload, payload_len);

    ZB_GPDF_EXT_NFC_SET_DIRECTION(gpdf_info->nwk_ext_frame_ctl, ZGP_FRAME_DIR_TO_ZGPD);

    if ((ZB_GP_DATA_REQ_FRAME_TYPE(gp_d_r_tx_options) == ZGP_FRAME_TYPE_MAINTENANCE)
            || ((ZB_GPDF_EXT_NFC_GET_APP_ID(gpdf_info->zgpd_id.app_id) == ZB_ZGP_APP_ID_0000)
                && (gpdf_info->zgpd_id.addr.src_id == ZB_ZGP_SRC_ID_ALL)))
    {
        gpdf_info->nwk_hdr_len = fill_broadcast_gpdf_nwk_hdr(buf,
                                 ZB_GP_DATA_REQ_FRAME_TYPE(gp_d_r_tx_options));
    }
    else
    {
        gpdf_info->nwk_hdr_len = fill_unicast_gpdf_nwk_hdr(buf, gpdf_info);
    }

    if (ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl))
    {
        zb_ret_t      kret = RET_NOT_FOUND;
        zgp_tbl_ent_t ent;
        zb_uint8_t    tgskt = 0;

        kret = zgp_any_table_read(&gpdf_info->zgpd_id, &ent);

        if (kret != RET_OK)
        {
            ZB_ASSERT(0);
        }

        if (ent.is_sink)
        {
            tgskt = ZGP_TBL_SINK_GET_SEC_KEY_TYPE(&ent);
        }
        else
        {
            tgskt = ZGP_TBL_PROXY_GET_SEC_KEY_TYPE(&ent);
        }
        zgp_key_recovery(&ent, (zb_bool_t)ZGP_KEY_TYPE_IS_INDIVIDUAL(tgskt),
                         gpdf_info->key, &gpdf_info->key_type);

        /* Ignore ret code */
        zb_zgp_protect_frame(gpdf_info, gpdf_info->key, buf);
    }

    cgp_req.recv_timestamp = gpdf_info->recv_timestamp;
    /* copy cgp_data_req to buf param */
    ZB_MEMCPY(ZB_BUF_GET_PARAM(buf, zb_cgp_data_req_t), &cgp_req, sizeof(zb_cgp_data_req_t));

    TRACE_MSG(TRACE_ZGP3, "<< prepare_buf_for_cpg_data_req", (FMT__0));
}

#ifdef ZB_ENABLE_ZGP_SINK
static void zb_gp_app_data_cfm(zb_uint8_t param)
{
    zb_gp_data_cfm_t *cfm;
    zb_uint8_t       *app_data_ptr;
    zb_int16_t        status;
    zb_uint8_t        len;
    zb_mac_mhr_t      mhr;
    zb_uint8_t       *cmd_ptr;
    zb_gpdf_info_t    gpdf_info;
    zb_gp_data_req_t  req;

    TRACE_MSG(TRACE_ZGP2, ">> zb_gp_app_cfm, param %hd", (FMT__H, param));

    cfm     = ZB_BUF_GET_PARAM(param, zb_gp_data_cfm_t);
    status  = zb_buf_get_status(param);
    cmd_ptr = zb_buf_begin(param);

    if (status >= ZB_ZGP_STATUS_ENTRY_REPLACED && status <= ZB_ZGP_STATUS_TX_QUEUE_FULL)
    {
        app_data_ptr = cmd_ptr + sizeof(zb_gp_data_req_t) - sizeof(req.pld);
    }
    else
    {
        /* Parse MAC header */
        len = zb_parse_mhr(&mhr, param);
        cmd_ptr += len;
        /* Parse ZGP NWK header */
        len = zgp_parse_gpdf_nwk_hdr(cmd_ptr, zb_buf_len(param), &gpdf_info);
        cmd_ptr += len;
        /* Store command id */
        cfm->cmd_id = *(zb_uint8_t *)cmd_ptr;
        app_data_ptr = cmd_ptr + 1;
    }

    ZGP_CTXC().app_cfm_cb(cfm->cmd_id, status, app_data_ptr, cfm->zgpd_id, cfm->handle);

    TRACE_MSG(TRACE_ZGP2, "<< zb_gp_app_cfm, param %hd", (FMT__H, param));
}
#endif  /* ZB_ENABLE_ZGP_SINK */

static void zb_dgp_data_ind_continue(zb_uint8_t param);

static void zb_gp_data_transmission_finished(zb_uint8_t param)
{
    zb_gp_data_cfm_t *cfm = ZB_BUF_GET_PARAM(param, zb_gp_data_cfm_t);
    zb_zgpd_id_t *zgpd_id = cfm->zgpd_id;
    zb_uint8_t handle     = cfm->handle;
    zb_uint8_t req_handle = 0;
    zb_uint8_t status     = zb_buf_get_status(param);

    TRACE_MSG(TRACE_ZGP2, "ZGP packet transmission finished: status 0x%x", (FMT__H, status));

    /* If sending ZGP packet is failed we can't leave packet in queue to resend,
     * because is_sent flag in queue entry is set to TRUE and this entry will
     * never resent again */

    if (handle == ZB_ZGP_HANDLE_ADD_CHANNEL_CONFIG)
    {
        req_handle = ZB_ZGP_HANDLE_REMOVE_CHANNEL_CONFIG;
    }
    else if (handle == ZB_ZGP_HANDLE_ADD_COMMISSIONING_REPLY)
    {
        req_handle = ZB_ZGP_HANDLE_REMOVE_COMMISSIONING_REPLY;
    }
    else
    {
        req_handle = ZB_ZGP_HANDLE_DEFAULT_HANDLE;
    }

    /* Remove entry from ZGP TX queue */
    zgp_clean_zgpd_info_from_queue(param, zgpd_id, req_handle);

    /* Update ZGP commissioning state */
    if (handle == ZB_ZGP_HANDLE_ADD_COMMISSIONING_REPLY)
    {
        ZB_ZGP_SET_COMM_STATE(status == MAC_SUCCESS ?
                              ZGP_COMM_STATE_COMMISSIONING_REPLY_SENT :
                              ZGP_COMM_STATE_CHANNEL_CONFIG_SENT);
    }
    else if (handle == ZB_ZGP_HANDLE_ADD_CHANNEL_CONFIG)
    {
        ZB_ZGP_SET_COMM_STATE(status == MAC_SUCCESS ?
                              ZGP_COMM_STATE_CHAN_CFG_SENT_RET_CHANNEL :
                              ZGP_COMM_STATE_CHAN_CFG_FAILED_RET_CHANNEL);
    }
}

void zb_gp_data_cfm(zb_uint8_t param)
{
    zb_gp_data_cfm_t *cfm = ZB_BUF_GET_PARAM(param, zb_gp_data_cfm_t);

    TRACE_MSG(TRACE_ZGP2, ">> zb_gp_data_cfm, param %hd", (FMT__H, param));

    TRACE_MSG(TRACE_ZGP2, "cfm->status 0x%hx, cfm->handle %hd",
              (FMT__H_H, zb_buf_get_status(param), cfm->handle));

#ifdef ZB_ENABLE_ZGP_SINK
    if (cfm->handle >= ZB_ZGP_HANDLE_APP_DATA)
    {
        if (ZGP_CTXC().app_cfm_cb != NULL)
        {
            zb_gp_app_data_cfm(param);
        }
    }
#endif  /* ZB_ENABLE_ZGP_SINK */

    switch (zb_buf_get_status(param))
    {
    case ZB_ZGP_STATUS_ENTRY_ADDED:
    case ZB_ZGP_STATUS_ENTRY_REPLACED:
    {
    }
    break;

    case ZB_ZGP_STATUS_TX_QUEUE_FULL:
    {
        TRACE_MSG(TRACE_ZGP1, "ZGP transmission queue is full: handle %hd, ZGP_CTXC().comm_data.state %hd",
                  (FMT__H_H, cfm->handle, ZGP_CTXC().comm_data.state));

        /* If there was place for Channel configuration packet in TX queue,
         * then it should be for commissioning reply for sure */
        ZB_ASSERT(cfm->handle != ZB_ZGP_HANDLE_ADD_COMMISSIONING_REPLY);

        if (cfm->handle == ZB_ZGP_HANDLE_ADD_CHANNEL_CONFIG)
        {
            /* No place in TX queue. Maybe some entries in TX queue will be expired,
             * so wait for next ZGPD channel req on operational channel */
            ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_CHAN_CFG_FAILED_RET_CHANNEL);

            zgp_back_to_oper_channel(param);
            param = 0;
        }
    }
    break;

    case ZB_ZGP_STATUS_ENTRY_EXPIRED:
    {
        TRACE_MSG(TRACE_ZGP1, "ZGP transmission entry expired: handle %hd, ZGP_CTXC().comm_data.state %hd",
                  (FMT__H_H, cfm->handle, ZGP_CTXC().comm_data.state));

        zb_gp_data_transmission_finished(param);
        param = 0;
    }
    break;

    case ZB_ZGP_STATUS_ENTRY_REMOVED:
    {
        if (cfm->handle == ZB_ZGP_HANDLE_REMOVE_CHANNEL_CONFIG)
        {
            zgp_back_to_oper_channel(param);
            param = 0;
        }
        else if (cfm->handle == ZB_ZGP_HANDLE_REMOVE_COMMISSIONING_REPLY)
        {

        }
        else if (cfm->handle == ZB_ZGP_HANDLE_REMOVE_AFTER_FAILED_COMM)
        {
            ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_COMMISSIONING_FINALIZING);

            zgp_back_to_oper_channel(param);
            param = 0;
        }
        else
        {
            ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_IDLE);
        }
    }
    break;

    case MAC_SUCCESS:
    {
        zb_gp_data_transmission_finished(param);
        param = 0;
    }
    break;

    default:
    {
        if (ZB_ZGP_CHECK_NON_SUCCESS_CONFIRM_STATUS(zb_buf_get_status(param)))
        {
            zb_gp_data_transmission_finished(param);
            param = 0;
        }
    }
    break;
    }

    if (param)
    {
        TRACE_MSG(TRACE_ZGP2, "No need additional action, drop the buffer %hd", (FMT__H, param));
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_ZGP2, "<< zb_gp_data_cfm", (FMT__0));
}

static void notify_about_expired_entry(zb_uint8_t param)
{
    zb_gp_data_cfm_t  *cfm = ZB_BUF_GET_PARAM(param, zb_gp_data_cfm_t);
    zb_uint8_t         tx_p_i_pos;
    zb_zgp_tx_pinfo_t *tx_p_i_ent;

    TRACE_MSG(TRACE_ZGP2, ">> notify_about_expired_entry param %hd", (FMT__H, param));

    tx_p_i_pos = zb_zgp_tx_q_find_expired_ent_pos(&ZGP_CTXC().tx_packet_info_queue,
                 &ZGP_CTXC().tx_queue);
    ZB_ASSERT(tx_p_i_pos != 0xFF);

    tx_p_i_ent        = &ZGP_CTXC().tx_packet_info_queue.queue[tx_p_i_pos];
    cfm->handle       = tx_p_i_ent->handle;
    /* TODO: Don't use a pointer to zgpd_id in zb_gp_data_cfm_t.
     * Restore original data request parameters before call data confirm */
    cfm->zgpd_id      = &tx_p_i_ent->zgpd_id;
    zb_buf_set_status(param, ZB_ZGP_STATUS_ENTRY_EXPIRED);
    zb_gp_data_cfm(param);

    TRACE_MSG(TRACE_ZGP2, "<< notify_about_expired_entry", (FMT__0));
}

void zb_zgp_tx_q_entry_expired(zb_uint8_t param)
{
    zb_zgp_tx_q_ent_t *ent;

    TRACE_MSG(TRACE_ZGP2, ">> zb_zgp_tx_q_entry_expired param %hd", (FMT__H, param));

    ZB_ASSERT(param < ZB_ZGP_TX_QUEUE_SIZE);

    ent = &ZGP_CTXC().tx_queue.queue[param];
    ent->is_expired = ZB_TRUE;
    zb_buf_get_out_delayed(notify_about_expired_entry);

    TRACE_MSG(TRACE_ZGP2, "<< zb_zgp_tx_q_entry_expired", (FMT__0));
}

#ifdef ZB_ZGP_IMMED_TX
static zb_uint32_t direct_backch_out_frame_counter_update(zb_zgpd_id_t *zgpd_id);

/*
 * This function should fill not gpdf_info, but actual packet contents on NWK layer.
 * cgp_data_req() should be refactored to accept packet with NWK layer filled.
 */
static void fill_gpdf_info_for_immed_send(zb_gpdf_info_t *gpdf_info, zb_zgpd_id_t *zgpd_id)
{
    ZB_BZERO(gpdf_info, sizeof(zb_gpdf_info_t));

    gpdf_info->mac_addr_flds.comb.dst_pan_id = ZB_NWK_BROADCAST_ALL_DEVICES;
    ZB_GPDF_NWK_FRAME_CONTROL(gpdf_info->nwk_frame_ctl, ZGP_FRAME_TYPE_DATA, ZB_FALSE, ZB_TRUE);
    ZB_GPDF_NWK_FRAME_CTL_EXT(gpdf_info->nwk_ext_frame_ctl, ZB_ZGP_APP_ID_0000,
                              ZB_ZGP_SEC_LEVEL_FULL_WITH_ENC, ZB_TRUE, ZB_FALSE, ZB_ZGP_SEC_LEVEL_NO_SECURITY);
    gpdf_info->sec_frame_counter = direct_backch_out_frame_counter_update(zgpd_id);
    ZB_GPDF_EXT_NFC_SET_RX_AFTER_TX(gpdf_info->nwk_ext_frame_ctl, 1);
    ZB_MEMCPY(&gpdf_info->zgpd_id, zgpd_id, sizeof(zb_zgpd_id_t));
}
#endif /* ZB_ZGP_IMMED_TX */

static void init_zgp_tx_pinfo_ent(zb_gp_data_req_t *req, zb_zgp_tx_pinfo_t *tx_p_i_ent)
{
    TRACE_MSG(TRACE_ZGP3, ">> init_zgp_tx_pinfo_ent req %p tx_p_i_ent %p",
              (FMT__P_P, req, tx_p_i_ent));

    ZB_ASSERT(req);
    ZB_ASSERT(tx_p_i_ent);

    tx_p_i_ent->handle = req->handle;
    ZB_MEMCPY(&tx_p_i_ent->zgpd_id, &req->zgpd_id, sizeof(zb_zgpd_id_t));
    TRACE_MSG(TRACE_ZGP3, "<< init_zgp_tx_pinfo_ent", (FMT__0));
}

static void gp_data_req_send_cnf(zb_uint8_t cnf_param, zb_zgpd_id_t *zgpd_id,
                                 zb_zgp_status_t status)
{
    zb_gp_data_req_t *req;
    zb_gp_data_cfm_t *cfm;

    TRACE_MSG(TRACE_ZGP3, ">> gp_data_req_send_cnf cnf_param, %hd zgpd_id %p status %hd",
              (FMT__H_P_H, cnf_param, zgpd_id, status));

    ZB_ASSERT(cnf_param);

    req = (zb_gp_data_req_t *)zb_buf_begin(cnf_param);

    /* Prepare and send confirmation */
    cfm = ZB_BUF_GET_PARAM(cnf_param, zb_gp_data_cfm_t);
    cfm->handle = req->handle;
    cfm->cmd_id = req->cmd_id;
    cfm->zgpd_id = zgpd_id;
    zb_buf_set_status(cnf_param, status);
    ZB_SCHEDULE_CALLBACK(zb_gp_data_cfm, cnf_param);

    TRACE_MSG(TRACE_ZGP3, "<< gp_data_req_send_cnf", (FMT__0));
}

#ifdef ZB_ZGP_IMMED_TX
static zb_uint8_t locate_direct_packet(zb_zgpd_id_t *id, zb_zgp_status_t *status)
{
    zb_uint8_t tx_p_i_pos;

    TRACE_MSG(TRACE_ZGP3, ">> locate_direct_packet id %p", (FMT__P, id));

    tx_p_i_pos = zb_zgp_tx_packet_info_q_grab_free_ent_pos(&ZGP_CTXC().tx_packet_info_queue,
                 ZB_ZGP_TX_PACKET_INFO_IMMED_PACKETS);
    *status = (tx_p_i_pos == 0xFF) ? ZB_ZGP_STATUS_TX_QUEUE_FULL : ZB_ZGP_STATUS_ENTRY_ADDED;

    TRACE_MSG(TRACE_ZGP3, "<< locate_direct_packet tx_p_i_pos %hd status %hd",
              (FMT__P_H, tx_p_i_pos, status));

    return tx_p_i_pos;
}
#endif  /* ZB_ZGP_IMMED_TX */

static zb_uint8_t locate_pending_packet(zb_zgpd_id_t *id, zb_zgp_status_t *status)
{
    zb_zgp_tx_packet_info_q_t *tx_p_i_q;
    zb_uint8_t                 tx_p_i_pos;

    TRACE_MSG(TRACE_ZGP3, ">> locate_pending_packet id %p", (FMT__P, id));

    /*
     * ZGP spec, A.1.5.2.1:
     * If the gpTxQueue already has an entry for the GPD ID (i.e. GPD
     * SrcID/GPD IEEE address) in the GP-DATA.request, the previous GPDF is
     * overwritten and GP-DATA.confirmation with the Status ENTRY_REPLACED is
     * provided to the GPEP. If the gpTxQueue has no previous entries for this
     * GPD SrcID/GPD IEEE address and it has empty entries, the GPDF is added to
     * the gpTxQueue and GP-DATA.confirmation with the Status ENTRY_ADDED is
     * provided to the GPEP. If the gpTxQueue has no previous entries for this
     * GPD SrcID/GPD IEEE address and it is full, the dGP stub returns GP-
     * DATA.confirm with the Status set to QUEUE_FULL.
     */

    tx_p_i_q = &ZGP_CTXC().tx_packet_info_queue;
    tx_p_i_pos = zb_zgp_tx_packet_info_q_find_pos(tx_p_i_q, id, ZB_ZGP_TX_PACKET_INFO_PENDING_PACKETS);

    if (tx_p_i_pos == 0xFF)
    {
        tx_p_i_pos = zb_zgp_tx_packet_info_q_grab_free_ent_pos(tx_p_i_q,
                     ZB_ZGP_TX_PACKET_INFO_PENDING_PACKETS);
        *status = (tx_p_i_pos == 0xFF) ? ZB_ZGP_STATUS_TX_QUEUE_FULL : ZB_ZGP_STATUS_ENTRY_ADDED;
    }
    else
    {
        *status = ZB_ZGP_STATUS_ENTRY_REPLACED;
    }

    TRACE_MSG(TRACE_ZGP3, "<< locate_pending_packet tx_p_i_pos %hd status %hd",
              (FMT__H_H, tx_p_i_pos, *status));

    return tx_p_i_pos;
}

static void gp_data_req_add_to_q(zb_gp_data_req_t *req, zb_uint8_t cnf_param)
{
    zb_uint8_t         tx_p_i_pos;
    zb_zgp_status_t    status;
    zb_zgp_tx_pinfo_t *tx_p_i_ent;
    zb_zgp_tx_q_ent_t *tx_q_ent;

    TRACE_MSG(TRACE_ZGP2, ">> gp_data_req_add_to_q req %p cnf_param %hd", (FMT__P_H, req, cnf_param));

    tx_p_i_pos = locate_pending_packet(&req->zgpd_id, &status);

    if (tx_p_i_pos != 0xFF)
    {
        tx_p_i_ent  = &ZGP_CTXC().tx_packet_info_queue.queue[tx_p_i_pos];
        tx_q_ent    = &ZGP_CTXC().tx_queue.queue[tx_p_i_pos];

        init_zgp_tx_pinfo_ent(req, tx_p_i_ent);

        tx_q_ent->tx_options        = req->tx_options;
        tx_q_ent->cmd_id            = req->cmd_id;
        tx_q_ent->payload_len       = req->payload_len;
        ZB_MEMCPY(&tx_q_ent->pld, &req->pld, req->payload_len);
        tx_q_ent->is_expired        = tx_q_ent->sent = 0;

        while (zb_schedule_alarm_cancel(zb_zgp_tx_q_entry_expired, tx_p_i_pos, NULL) == RET_OK)
        {
        }

        if (req->tx_q_ent_lifetime != ZB_GP_TX_QUEUE_ENTRY_LIFETIME_INF)
        {
            ZB_SCHEDULE_ALARM(zb_zgp_tx_q_entry_expired, tx_p_i_pos, req->tx_q_ent_lifetime);
        }
    }

    /* Prepare and send confirmation */
    gp_data_req_send_cnf(cnf_param, &req->zgpd_id, status);

    TRACE_MSG(TRACE_ZGP2, "<< gp_data_req_add_to_q req", (FMT__0));
}

#ifdef ZB_ZGP_IMMED_TX
static void gp_data_req_immed_send(zb_uint8_t param)
{
    zb_buf_t                  *buf;
    zb_zgp_status_t            status;
    zb_zgp_tx_packet_info_q_t *tx_p_i_q;
    zb_uint8_t                 tx_p_i_pos;
    zb_zgp_tx_pinfo_t         *tx_p_i_ent;
    zb_zgpd_id_t              *zgpd_id;
    zb_gp_data_req_t          *req;
    zb_gpdf_info_t             gpdf_info;

    TRACE_MSG(TRACE_ZGP2, ">> gp_data_req_immed_send param %hd", (FMT__H, param));

    ZB_ASSERT(param);

    req = (zb_gp_data_req_t *)zb_buf_begin(param);

    tx_p_i_q = &ZGP_CTXC().tx_packet_info_queue;

    zgpd_id = &req->zgpd_id;
    tx_p_i_pos = locate_direct_packet(zgpd_id, &status);

    if (tx_p_i_pos != 0xFF)
    {
        tx_p_i_ent = &tx_p_i_q->queue[tx_p_i_pos];

        init_zgp_tx_pinfo_ent(req, tx_p_i_ent);
        fill_gpdf_info_for_immed_send(&gpdf_info, zgpd_id);

        prepare_buf_for_cpg_data_req(buf, req, tx_p_i_pos, &gpdf_info);
        ZB_SCHEDULE_CALLBACK(cgp_data_req, param);
    }
    else
    {
        gp_data_req_send_cnf(param, zgpd_id, status);
    }

    TRACE_MSG(TRACE_ZGP2, "<< gp_data_req_immed_send", (FMT__0));
}
#endif  /* ZB_ZGP_IMMED_TX */

#if !defined ZB_ALIEN_ZGP_STUB
void zb_gp_data_req(zb_uint8_t param)
{
    zb_gp_data_req_t          *req;
    zb_uint8_t                 tx_p_i_pos;
#ifdef ZB_ZGP_IMMED_TX
    zb_uint8_t                 tx_options;
    zb_time_t                  lifetime;
#endif  /* ZB_ZGP_IMMED_TX */
    zb_zgp_tx_packet_info_q_t *tx_p_i_q;

    TRACE_MSG(TRACE_ZGP2, ">> zb_gp_data_req param %hd", (FMT__H, param));

    ZB_ASSERT(param);

    req = (zb_gp_data_req_t *)zb_buf_begin(param);

    if (req->action == ZB_GP_DATA_REQ_ACTION_ADD_GPDF)
    {
#ifdef ZB_ZGP_IMMED_TX
        tx_options = req->tx_options;
        lifetime   = req->tx_q_ent_lifetime;

        ZVUNUSED(lifetime);

        ZB_ASSERT((!(tx_options & ZB_GP_DATA_REQ_USE_GP_TX_QUEUE)) ==
                  (lifetime == ZB_GP_TX_QUEUE_ENTRY_LIFETIME_NONE));

        if (!(tx_options & ZB_GP_DATA_REQ_USE_GP_TX_QUEUE))
        {
            gp_data_req_immed_send(param);
        }
        else
#endif  /* ZB_ZGP_IMMED_TX */
        {
            gp_data_req_add_to_q(req, param);
        }
    }
    else /* ZB_GP_DATA_REQ_ACTION_REMOVE_GPDF */
    {
        tx_p_i_q = &ZGP_CTXC().tx_packet_info_queue;
        tx_p_i_pos = zb_zgp_tx_packet_info_q_find_pos(tx_p_i_q, &req->zgpd_id,
                     ZB_ZGP_TX_PACKET_INFO_ALL_PACKETS);

        if (tx_p_i_pos != 0xFF)
        {
            zb_zgp_tx_packet_info_q_delete_ent(tx_p_i_q, tx_p_i_pos);
            ZB_SCHEDULE_ALARM_CANCEL(zb_zgp_tx_q_entry_expired, tx_p_i_pos);
        }

        gp_data_req_send_cnf(param, &req->zgpd_id, ZB_ZGP_STATUS_ENTRY_REMOVED);
    }

    TRACE_MSG(TRACE_ZGP2, "<< zb_gp_data_req", (FMT__0));
}
#endif  /* !defined ZB_ALIEN_ZGP_STUB */

#ifdef ZB_ZGP_IMMED_TX
static zb_uint32_t direct_backch_out_frame_counter_update(zb_zgpd_id_t *zgpd_id)
{
    zb_zgp_sink_tbl_ent_t  ent;
    zb_uint32_t           *cnt;

    TRACE_MSG(TRACE_ZGP2, ">> direct_backch_out_frame_counter_update", (FMT__0));

    if (zgp_sink_table_read(zgpd_id, &ent) == RET_OK)
    {
        cnt = &ZGP_CTXC().immed_tx_frame_counter;

        (*cnt)++;
        if (*cnt == 0)
        {
            *cnt = 1;
        }

        if (*cnt == ent.security_counter)
        {
            (*cnt)++;
            if (*cnt == 0)
            {
                *cnt = 1;
            }
        }

#ifdef ZB_USE_NVRAM
        if (*cnt % ZB_GP_IMMED_TX_FRAME_COUNTER_UPDATE_INTERVAL == 0)
        {
            /* If we fail, trace is given and assertion is triggered */
            (void)zb_nvram_write_dataset(ZB_NVRAM_DATASET_GP_SINKT);
        }
#endif  /* ZB_USE_NVRAM */
    }
    else
    {
        ZB_ASSERT(0);
    }

    TRACE_MSG(TRACE_ZGP2, "<< direct_backch_out_frame_counter_update %d", (FMT__D, *cnt));
    return *cnt;
}
#endif  /* ZB_ZGP_IMMED_TX */

/** @} */ /* zgp_stub_int */

#if !defined ZB_ALIEN_ZGP_STUB
/**
 * @brief Search and send GPDF from Tx queue
 *
 * @return ZB_TRUE if matching entry in Tx Queue was found for incoming GPDF \n
 *         ZB_FALSE otherwise
 */
zb_uint8_t zb_zgp_try_bidir_tx(zb_uint8_t param)
{
    zb_gpdf_info_t    *gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);
    zb_uint8_t         tx_pos;
    zb_zgp_tx_q_ent_t *tx_ent = NULL;
    zb_uint8_t         ret = ZB_FALSE;

    TRACE_MSG(TRACE_ZGP3, ">> zb_zgp_try_bidir_tx param %hd", (FMT__H, param));

    tx_pos = zb_zgp_tx_q_find_ent_pos_for_send(&ZGP_CTXC().tx_packet_info_queue,
             &ZGP_CTXC().tx_queue, &gpdf_info->zgpd_id);

    if (tx_pos != 0xFF)
    {
        zb_bufid_t outgoing_buf = 0;

        ZB_ASSERT(tx_pos < ZB_ZGP_TX_QUEUE_SIZE);
        ZB_ASSERT(ZB_CHECK_BIT_IN_BIT_VECTOR(ZGP_CTXC().tx_packet_info_queue.used_mask, tx_pos));
        ZB_ASSERT(ZB_GPDF_EXT_NFC_GET_RX_AFTER_TX(gpdf_info->nwk_ext_frame_ctl));

        tx_ent = &ZGP_CTXC().tx_queue.queue[tx_pos];

        TRACE_MSG(TRACE_ZGP3, "Got TX tx_pos %hd, tx_options %hd",
                  (FMT__H_H, tx_pos, tx_ent->tx_options));

#ifdef ZB_MAC_OK_TO_SEND_ZGP
        if ((tx_ent->tx_options & MAC_TX_OPTION_NO_CSMA_CA) &&
                ZB_MAC_OK_TO_SEND_ZGP())
#endif
        {
            /* Synchronous allocate: called directly from MAC, can't block. If no
             * buffers, we are unlucky. */
            outgoing_buf = zb_buf_get_out();
        }

        if (outgoing_buf)
        {
            zb_gpdf_info_t     out_gpdf_info;
            zb_zgp_tx_pinfo_t *tx_pi = &ZGP_CTXC().tx_packet_info_queue.queue[tx_pos];

            TRACE_MSG(TRACE_ZGP3, "allocated out buf %hd", (FMT__H, outgoing_buf));

            tx_ent->sent = 1;

            ZB_MEMCPY(&out_gpdf_info, gpdf_info, sizeof(zb_gpdf_info_t));
            ZB_MEMCPY(&out_gpdf_info.zgpd_id, &tx_pi->zgpd_id, sizeof(zb_zgpd_id_t));

            prepare_buf_for_cpg_data_req(outgoing_buf, NULL, tx_pos, &out_gpdf_info);

#ifndef MAC_AUTO_DELAY_IN_MAC_GP_SEND
            {
                zb_time_t          t  = 0;
                zb_time_t          t2 = 0;

                /* Old code: sleep here, not in MAC. Moved to be after
                 * prepare_buf_for_cpg_data_req which can spend significant time inside encryption. */
                t = osif_transceiver_time_get();
                TRACE_MSG(TRACE_ZGP3, "t recv %ld now %ld",
                          (FMT__L_L, out_gpdf_info.recv_timestamp, t));
                t2 = ZB_TRANSCEIVER_TIME_SUBTRACT(t, out_gpdf_info.recv_timestamp);

                if (ZB_GPD_TX_OFFSET_US > t2)
                {
                    /* Synchronous wait for RX enable at GPD */
                    TRACE_MSG(TRACE_ZGP3, "sync sleep: %d us",
                              (FMT__D, (ZB_GPD_TX_OFFSET_US - t2)));

                    osif_sleep_using_transc_timer(ZB_GPD_TX_OFFSET_US - t2);
                    TRACE_MSG(TRACE_ZGP3, "sync sleep done", (FMT__0));
                }
                else
                {
                    TRACE_MSG(TRACE_ERROR, "No need sync sleep: !(%d us > %d us)",
                              (FMT__D_D, t2, ZB_GPD_TX_OFFSET_US));
                }
            }
#endif

            cgp_data_req(tx_pi->buf_ref);
            ret = ZB_TRUE;
        }
    }

    TRACE_MSG(TRACE_ZGP3, "<< zb_zgp_try_bidir_tx, ret %hd", (FMT__H, ret));

    return ret;
}
#endif  /* !defined ZB_ALIEN_ZGP_STUB */

static zb_bool_t check_mac_hdr(zb_mac_mhr_t *mhr)
{
    zb_bool_t ret = ZB_TRUE;

    switch (ZB_FCF_GET_DST_ADDRESSING_MODE(mhr->frame_control))
    {
    case ZB_ADDR_16BIT_DEV_OR_BROADCAST:
    {
        ret = (zb_bool_t)(mhr->dst_addr.addr_short == ZB_NWK_BROADCAST_ALL_DEVICES);
    }
    break;

    case ZB_ADDR_64BIT_DEV:
    {
        ret = (zb_bool_t)ZB_64BIT_ADDR_CMP(
                  mhr->dst_addr.addr_long,
                  ZB_PIBCACHE_EXTENDED_ADDRESS());
    }
    break;

    default:
        ret = ZB_FALSE;
    }

    ret = (zb_bool_t)(ret &&
                      (mhr->dst_pan_id == ZB_BROADCAST_PAN_ID
                       || mhr->dst_pan_id == ZB_PIBCACHE_PAN_ID()));

    return ret;
}

void zb_gp_mcps_data_indication(zb_uint8_t param)
{
    zb_gpdf_info_t *gpdf_info;
    zb_mac_mhr_t    mhr;
    zb_ushort_t     mhr_len;
    zb_uint8_t     *dummy;
    zb_uint8_t      lqi;
    zb_int8_t       rssi;
    zb_time_t       recv_timestamp;

    TRACE_MSG(TRACE_ZGP3, ">> zb_gp_mcps_data_indication param %hd", (FMT__H, param));

    /* Suppose MAC already done duplicates filtering based on MAC sequence number. */
    recv_timestamp = *ZB_BUF_GET_PARAM(param, zb_time_t);
    lqi = ZB_MAC_GET_LQI(param);
    rssi = ZB_MAC_GET_RSSI(param);

    mhr_len = zb_parse_mhr(&mhr, param);
    ZB_MAC_CUT_HDR(param, mhr_len, dummy);
    ZVUNUSED(dummy);

    if (!check_mac_hdr(&mhr)
#ifdef ZB_RAF_INVALID_PACKET_PREVENT
            || ((zb_buf_len(param) + sizeof(zb_gpdf_info_t)) > ZB_IO_BUF_SIZE)
#endif
       )
    {

#ifdef ZB_RAF_INVALID_PACKET_PREVENT
        if ((zb_buf_len(param) + sizeof(zb_gpdf_info_t)) > ZB_IO_BUF_SIZE)
        {
            TRACE_MSG(TRACE_ERROR, "Z< (D) ZGP lpkt", (FMT__0));
        }
#endif

#ifdef ZB_RAF_INVALID_PACKET_PREVENT
        if (!check_mac_hdr(&mhr))
        {
            TRACE_MSG(TRACE_ERROR, "Some check of GPDF MAC fields failed. Drop packet", (FMT__0));
        }
#else
        TRACE_MSG(TRACE_ERROR, "Some check of GPDF MAC fields failed. Drop packet", (FMT__0));
#endif
        zb_buf_free(param);
        param = 0;
    }
    if (param)
    {
        gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);

        ZB_BZERO(gpdf_info, sizeof(zb_gpdf_info_t));

        /* Save source address and seq_num. Only long address may be needed */
        if (ZB_FCF_GET_SRC_ADDRESSING_MODE(mhr.frame_control) == ZB_ADDR_64BIT_DEV)
        {
            ZB_64BIT_ADDR_COPY(&gpdf_info->zgpd_id.addr, &mhr.src_addr);
        }

        if (ZB_FCF_GET_PANID_COMPRESSION_BIT(mhr.frame_control))
        {
            gpdf_info->mac_addr_flds_len = mhr_len - sizeof(mhr.frame_control) - sizeof(mhr.seq_number);
            ZB_MEMCPY(&gpdf_info->mac_addr_flds.in,
                      (zb_uint8_t *)zb_buf_begin(param) - (gpdf_info->mac_addr_flds_len),
                      gpdf_info->mac_addr_flds_len);
        }
        else
        {
            gpdf_info->mac_addr_flds_len = mhr_len - sizeof(mhr.frame_control) - sizeof(mhr.seq_number) - sizeof(mhr.src_pan_id);
            gpdf_info->mac_addr_flds.comb.dst_addr = mhr.dst_addr.addr_short;
            gpdf_info->mac_addr_flds.comb.dst_pan_id = mhr.dst_pan_id;

            if (ZB_FCF_GET_SRC_ADDRESSING_MODE(mhr.frame_control) == ZB_ADDR_64BIT_DEV)
            {
                ZB_64BIT_ADDR_COPY(&gpdf_info->mac_addr_flds.comb.src_addr, &mhr.src_addr);
            }
        }

        if (gpdf_info->mac_addr_flds_len > sizeof(gpdf_info->mac_addr_flds.in))
        {
            TRACE_MSG(TRACE_ZGP1, "Unexpected MAC addressing fields len %d",
                      (FMT__H, gpdf_info->mac_addr_flds_len));
            zb_buf_free(param);
            param = 0;
        }
    }

    if (param)
    {
        gpdf_info->mac_seq_num = mhr.seq_number;

        gpdf_info->nwk_hdr_len = zgp_parse_gpdf_nwk_hdr(
                                     zb_buf_begin(param), zb_buf_len(param), gpdf_info);

        if (gpdf_info->nwk_hdr_len == 0)
        {
            TRACE_MSG(TRACE_ZGP1, "Can't parse incoming GPDF. Drop it", (FMT__0));
            zb_buf_free(param);
            param = 0;
        }
    }

    if (param)
    {
        /* It's correct only in case when payload not encrypted! */
        gpdf_info->zgpd_cmd_id = *((zb_uint8_t *)zb_buf_begin(param) + gpdf_info->nwk_hdr_len);
        gpdf_info->lqi = lqi;
        gpdf_info->rssi = rssi;
        gpdf_info->recv_timestamp = recv_timestamp;
        gpdf_info->rx_directly = 1;

        zb_dgp_data_ind(param);
    }

    TRACE_MSG(TRACE_ZGP3, "<< zb_gp_mcps_data_indication", (FMT__0));
}

/**
 * @brief cGP-DATA.confirm handler
 *
 * @param param [in] Buffer with confirmation
 *
 * @see ZGP spec, A.1.3.1.1
   */
void zb_cgp_data_cfm(zb_uint8_t param)
{
    zb_uint8_t         tx_p_i_pos;
    zb_zgp_tx_pinfo_t *tx_p_i_ent;
    zb_gp_data_cfm_t  *cfm = ZB_BUF_GET_PARAM(param, zb_gp_data_cfm_t);

    TRACE_MSG(TRACE_ZGP2, ">> zb_cgp_data_cfm param %hd", (FMT__H, param));

#ifdef ZB_ENABLE_ZGP_TEST_HARNESS
    if (ZGP_CTXC().out_gpdf_info.buf_ref != 0 && ZGP_CTXC().out_gpdf_info.buf_ref == param)
    {
        zgp_send_gpdf_on_tx_channel_cfm(param);
    }
    else
#endif  /* ZB_ENABLE_ZGP_TEST_HARNESS */
    {
        tx_p_i_pos = zb_zgp_tx_q_find_ent_pos_for_cfm(&ZGP_CTXC().tx_packet_info_queue, param);

        if (tx_p_i_pos != 0xFF)
        {
            tx_p_i_ent = &ZGP_CTXC().tx_packet_info_queue.queue[tx_p_i_pos];

            cfm->handle = tx_p_i_ent->handle;
            cfm->zgpd_id = &tx_p_i_ent->zgpd_id;

            tx_p_i_ent->buf_ref = 0;

            ZB_SCHEDULE_CALLBACK(zb_gp_data_cfm, param);
        }
        else
        {
            TRACE_MSG(TRACE_ZGP2, "param %hd not found in tx_queue", (FMT__H, param));
            zb_buf_free(param);
        }
    }

    TRACE_MSG(TRACE_ZGP2, "<< zb_cgp_data_cfm", (FMT__0));
}
#endif  /* ZB_ENABLE_ZGP_DIRECT */

/**
 * @brief Perform freshness check for incoming GPDF
 *
 * Freshness check should be applied only to frames from known ZGPDs
 * (i.e. information about ZGPD is stored in Sink or Proxy table).
 *
 * @param gpdf_info [in]  Information about incoming GPDF
 *
 * @return ZB_TRUE if freshness check succeed \n
 *         ZB_FALSE otherwise
 */
static zb_bool_t gpdf_is_fresh(zb_gpdf_info_t *gpdf_info, zgp_tbl_ent_t *ent)
{
    zb_bool_t   ret = ZB_FALSE;
    zb_uint32_t stored_zgpd_dup_counter = (zb_uint32_t)~0;

    ZB_ASSERT(ent);
    TRACE_MSG(TRACE_ZGP2, ">> gpdf_is_fresh gpdf_info %p is_sink %d", (FMT__P_H, gpdf_info, ent->is_sink));

    ZVUNUSED(ent);

#ifdef ZB_ENABLE_ZGP_SINK
    if (ent->is_sink)
    {
        stored_zgpd_dup_counter = zgp_sink_table_get_security_counter(&gpdf_info->zgpd_id);
        TRACE_MSG(TRACE_ZGP3, "Got dup counter %ld from the Sink table ", (FMT__L, stored_zgpd_dup_counter));
    }
#endif  /* ZB_ENABLE_ZGP_SINK */
#ifdef ZB_ENABLE_ZGP_PROXY
#ifdef ZB_ENABLE_ZGP_SINK
    else
#endif  /* ZB_ENABLE_ZGP_SINK */
    {
        stored_zgpd_dup_counter = zgp_proxy_table_get_security_counter(&gpdf_info->zgpd_id);
        TRACE_MSG(TRACE_ZGP3, "Got dup counter %ld from the Proxy table ", (FMT__L, stored_zgpd_dup_counter));
    }
#endif  /* ZB_ENABLE_ZGP_PROXY */

    if (stored_zgpd_dup_counter != (zb_uint32_t)~0)
    {
        if (ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl) == 0)
        {
            /* ZGP spec, A.3.6.1.3:
             *
             * If the GPD command used SecurityLevel 0b00, the filtering of reply GPD
             * commands depends of the MAC sequence number capabilities of a particular
             * GPD. For random sequence numbers, any number that passes the duplicate
             * filter is accepted. For incremental numbers, the received MAC sequence
             * number must be higher than the last sequence number stored in the
             * Proxy/Sink table, taking into account counter roll over, see A.3.6.1.3.1.
             */
#if defined ZB_ENABLE_ZGP_PROXY && defined ZB_CERTIFICATION_HACKS
            /* 08/29/2016 [AEV] Deprecated ZB CERT HACK */
            if (ZGP_CERT_HACKS().gp_proxy_ignore_duplicate_gp_frames)
            {
                TRACE_MSG(TRACE_ZGP3, "Forced skip freshness check!", (FMT__0));
                ret = ZB_TRUE;
            }
            else
#endif  /* defined ZB_ENABLE_ZGP_PROXY && defined ZB_CERTIFICATION_HACKS */
                /* ZGP spec,Revision 15,Version candidate 1.0,December 3rd, 2015, A.3.6.1.3:
                 * If the GPD command used SecurityLevel 0b00, any number that passes the duplicate filter is accepted.
                 */
                ret = ZB_TRUE;
            /* 08/29/2016 [AEV] Minor: remove old spec. */
            /*   if (ZGP_TBL_GET_SEQ_NUM_CAP(ent))
                  {
                    ZB_ZGP_CHECK_MAC_SEQ_NUM_FRESHNESS(
                      stored_zgpd_dup_counter,
                      gpdf_info->mac_seq_num, ret);
                  }
                  else *//* random MAC sequence number is used */
            /*    {
                    ret = ZB_TRUE;
                  }
            */
        }
        else /* security level > 0 */
        {
            /* If the GPD command used SecurityLevel 0b01, 0b10 or 0b11, then the
             * filtering of duplicate messages is performed based on the GPD security
             * frame counter, stored in the Proxy/Sink Table entry for this GPD. The
             * received GPD security frame counter must be higher than the value stored
             * in the Proxy/Sink Table; roll over shall not be supported.
             */

            ret = (zb_bool_t)(gpdf_info->sec_frame_counter > stored_zgpd_dup_counter);
        }
    }
    else
    {
        ret = ZB_TRUE;
    }

    TRACE_MSG(TRACE_ZGP2, "<< gpdf_is_fresh ret %d", (FMT__D, ret));

    return ret;
}

/* Described in A.3.7.3.4  Incoming frames: key recovery */
zb_ret_t zgp_key_recovery(zgp_tbl_ent_t *ent, zb_bool_t individual, zb_uint8_t *key, zb_uint8_t *key_type)
{
    zb_ret_t ret = RET_NOT_FOUND;

    TRACE_MSG(TRACE_ZGP1, ">> zgp_key_recovery ent: %hd, individual: %hd", (FMT__H_H, (ent ? 1 : 0), individual));

    if (individual)
    {
        if (ent)
        {
            *key_type = ZGP_TBL_GET_SEC_KEY_TYPE(ent);
            /*
              If SecurityLevel is 0b00 or if the SecurityKeyType has value 0b011 (GPD group
              key), 0b001 (NWK key) or 0b111 (derived individual GPD key), the GPDkey
              parameter MAY be omitted and the key MAY be stored in the gpSharedSecurityKey
              parameter instead. If SecurityLevel has value other than 0b00 and the
              SecurityKeyType has value 0b111 (derived individual GPD key), the GPDkey
              parameter MAY be omitted and the key MAY calculated on the fly, based on the
              value stored in the gpSharedSecurityKey parameter.
            */
            switch (ZGP_TBL_GET_SEC_KEY_TYPE(ent))
            {
            case ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL:
            {
                /*
                  And the KeyType sub-field of the Sink/Proxy entry has the value 0b100:
                  use the GPD key stored in the Sink/Proxy Table entry for this GPD,
                  if none is stored: return DROP_FRAME.
                */
                if (ZGP_TBL_GET_SEC_PRESENT(ent) && !ZB_CCM_KEY_IS_ZERO(ent->zgpd_key))
                {
                    ZB_MEMCPY(key, ent->zgpd_key, ZB_CCM_KEY_SIZE);
                    ret = RET_OK;
                }
                break;
            }
            case ZB_ZGP_SEC_KEY_TYPE_DERIVED_INDIVIDUAL:
            {
                if (ZGP_TBL_GET_SEC_PRESENT(ent) && !ZB_CCM_KEY_IS_ZERO(ent->zgpd_key))
                {
                    /*
                      And the KeyType sub-field of the Sink/Proxy entry has the value 0b111:
                      use the GPD key stored in the Sink/Proxy Table entry for this GPD */
                    ZB_MEMCPY(key, ent->zgpd_key, ZB_CCM_KEY_SIZE);
                    ret = RET_OK;
                }
                else
                {
                    zb_zgpd_id_t zgpd_id;

                    /* Or if none stored in the Sink/Proxy Table entry: the individual key, derived from the
                     * gpSharedSecurityKey.
                     */
                    /* derive from ZGP_CTXC().cluster.gp_shared_security_key */
                    ZB_MAKE_ZGPD_ID(zgpd_id,
                                    ZGP_TBL_GET_APP_ID(ent),
                                    ent->endpoint,
                                    ent->zgpd_id);
                    zb_zgp_key_gen(ZB_ZGP_SEC_KEY_TYPE_DERIVED_INDIVIDUAL, &zgpd_id, 0, key);
                    ret = RET_OK;
                }
                break;
            }
            };
        }
    } /* if (individual) */
    else  /* else shared */
    {
        /*  If the KeyType field of the GP-SEC-request had the value of 0b0:
         */
        if (ent && (ZGP_TBL_GET_SEC_PRESENT(ent)))
        {
            *key_type = ZGP_TBL_GET_SEC_KEY_TYPE(ent);
            ZB_MEMCPY(key, ent->zgpd_key, ZB_CCM_KEY_SIZE);
            ret = RET_OK;
        }
        else
        {
            *key_type = ZGP_GP_SHARED_SECURITY_KEY_TYPE;

            switch (*key_type)
            {
            case ZB_ZGP_SEC_KEY_TYPE_NO_KEY:
                break;
            case ZB_ZGP_SEC_KEY_TYPE_NWK:
                /*
                  And the KeyType sub-field of the Sink/Proxy entry has the value 0b001:
                  use the GPD key stored in the gpSharedSecurityKey, if the gpSharedSecurityKeyType = 0b001,
                  or the key from the Key field of the nwkSecurityMaterialSet NIB parameter.
                  else: return DROP_FRAME.
                */
                if (ZGP_GP_SHARED_SECURITY_KEY_TYPE == ZB_ZGP_SEC_KEY_TYPE_NWK)
                {
                    ZB_MEMCPY(key, secur_nwk_key_by_seq(ZB_NIB().active_key_seq_number), ZB_CCM_KEY_SIZE);
                    ret = RET_OK;
                }
                break;
            case ZB_ZGP_SEC_KEY_TYPE_GROUP:
                /* And the KeyType sub-field of the Sink/Proxy entry has the value 0b010: use the GPD key
                   stored in the gpSharedSecurityKey, if the gpSharedSecurityKeyType = 0b010, else: return
                   DROP_FRAME.
                */
                if (ZGP_GP_SHARED_SECURITY_KEY_TYPE == ZB_ZGP_SEC_KEY_TYPE_GROUP)
                {
                    ZB_MEMCPY(key, ZGP_CTXC().cluster.gp_shared_security_key, ZB_CCM_KEY_SIZE);
                    ret = RET_OK;
                }
                break;
            case ZB_ZGP_SEC_KEY_TYPE_GROUP_NWK_DERIVED:
                /*
                  And the KeyType sub-field of the Sink/Proxy entry has the value 0b011:
                  use the GPD key stored in the gpSharedSecurityKey, if the gpSharedSecurityKeyType = 0b011,
                  or the key derived from the gpSharedSecurityKey,
                  else: return DROP_FRAME.
                */
                if (ZGP_GP_SHARED_SECURITY_KEY_TYPE == ZB_ZGP_SEC_KEY_TYPE_GROUP_NWK_DERIVED)
                {
                    ZB_MEMCPY(key, ZGP_CTXC().cluster.gp_shared_security_key, ZB_CCM_KEY_SIZE);
                    ret = RET_OK;
                }
                break;
            };
        }
    }
    TRACE_MSG(TRACE_ZGP1, "<< zgp_key_recovery ret = %d", (FMT__H, ret));
    return ret;
}

static void restore_frame_counter(zb_gpdf_info_t *gpdf_info)
{
    TRACE_MSG(TRACE_ZGP1, ">> restore_frame_counter", (FMT__0));

#ifdef ZB_ENABLE_ZGP_SINK
    zgp_sink_table_restore_security_counter(&gpdf_info->zgpd_id);
#endif  /* ZB_ENABLE_ZGP_SINK */
#ifdef ZB_ENABLE_ZGP_PROXY
    zgp_proxy_table_restore_security_counter(&gpdf_info->zgpd_id);
#endif  /* ZB_ENABLE_ZGP_PROXY */

    TRACE_MSG(TRACE_ZGP1, "<< restore_frame_counter", (FMT__0));
}

static void store_frame_counter(zb_gpdf_info_t *gpdf_info, zb_bool_t have_sink, zb_bool_t have_proxy)
{
    TRACE_MSG(TRACE_ZGP1, ">> store_frame_counter have_sink[%hd], have_proxy[%hd]", (FMT__H_H, have_sink, have_proxy));

    /* A.3.7.3.1 If the device is a combo, i.e. has both sink and proxy functionality, the Sink Table
     * SHALL be consult-ed first, see sec. A.3.7.3.3 A.3.7.3.2. Whenever the security-related
     * parameters in a Sink Table entry for a particular GPD are updated, the changes SHALL be
     * automatically propagated to the Proxy Table.
     */
#ifdef ZB_ENABLE_ZGP_SINK
    if (have_sink)
    {
        zgp_sink_table_set_security_counter(&gpdf_info->zgpd_id, gpdf_info->sec_frame_counter);
    }
#endif  /* ZB_ENABLE_ZGP_SINK */
#ifdef ZB_ENABLE_ZGP_PROXY
    if (have_proxy)
    {
        zgp_proxy_table_set_security_counter(&gpdf_info->zgpd_id, gpdf_info->sec_frame_counter);
    }
#endif  /* ZB_ENABLE_ZGP_PROXY */
    TRACE_MSG(TRACE_ZGP1, "<< store_frame_counter", (FMT__0));
}

static zb_bool_t zb_gp_sec_request_security_check(zb_gpdf_info_t *gpdf_info, zgp_tbl_ent_t *ent)
{
    zb_bool_t ret = ZB_FALSE;

    if (ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl) == ZGP_TBL_GET_SEC_LEVEL(ent))
    {
        if (ZB_GPDF_EXT_NFC_GET_SEC_KEY(gpdf_info->nwk_ext_frame_ctl) == ZGP_KEY_TYPE_IS_INDIVIDUAL(ZGP_TBL_GET_SEC_KEY_TYPE(ent)))
        {
            ret = ZB_TRUE;
        }
        else
        {
            TRACE_MSG(TRACE_ZGP3, "The frame has sec_key %hd while key type is %hd (is individual %hd). Drop.",
                      (FMT__H_H_H, ZB_GPDF_EXT_NFC_GET_SEC_KEY(gpdf_info->nwk_ext_frame_ctl), ZGP_TBL_GET_SEC_KEY_TYPE(ent), ZGP_KEY_TYPE_IS_INDIVIDUAL(ZGP_TBL_GET_SEC_KEY_TYPE(ent))));
        }
    }
    else
    {
        TRACE_MSG(TRACE_ZGP3, "The frame has sec_level %hd while sec_level is %hd. Drop.",
                  (FMT__H_H, ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl), ZGP_TBL_GET_SEC_LEVEL(ent)));
    }
    return ret;
}

static gp_sec_resp_t zb_gp_sec_request_match_endpoint_and_key_recovery(zb_gpdf_info_t *gpdf_info, zgp_tbl_ent_t *ent)
{
    gp_sec_resp_t ret = GP_SEC_RESPONSE_DROP_FRAME;
    zb_bool_t     key_recovery = ZB_TRUE;

    /* If the checks are successful, the sink/proxy checks if the Endpoint parameter of the
     * GP-SEC.request matches that in the Sink/Proxy Table entry. If yes, the sink/proxy generates
     * GP-SEC.response, with the Status MATCH, and includes the key, the key type and the
     * frame counter as processed here.
     */
    if (ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl) != ZB_ZGP_SEC_LEVEL_NO_SECURITY)
    {
        if (zgp_key_recovery(ent, (zb_bool_t)ZB_GPDF_EXT_NFC_GET_SEC_KEY(gpdf_info->nwk_ext_frame_ctl),
                             gpdf_info->key, &gpdf_info->key_type) != RET_OK)
        {
            key_recovery = ZB_FALSE;
        }
    }
    if (GP_ENPOINT_MATCH(gpdf_info->zgpd_id.endpoint, ent->endpoint))
    {
        if (key_recovery)
        {
            TRACE_MSG(TRACE_ZGP3, "Sink %hd: return MATCH", (FMT__H, ent->is_sink));
            ret = GP_SEC_RESPONSE_MATCH;
        }
        else
        {
            TRACE_MSG(TRACE_ZGP3, "Sink %hd: key recovery failed. Drop.", (FMT__H, ent->is_sink));
        }
    }
    else
    {
        /* If not, the sink/proxy generates GP-SEC.response with the Status TX_THEN_DROP and
         * includes the key, the key type and the frame counter as processed here; if the sink/proxy
         * does not support bidirectional communication it MAY return the Status DROP instead.
         */
        TRACE_MSG(TRACE_ZGP3, "Sink %hd: GPDF endpoint %hd != table endpoint %hd. Return TX_THEN_DROP",
                  (FMT__H_H_H, ent->is_sink, gpdf_info->zgpd_id.endpoint, ent->endpoint));
        ret = GP_SEC_RESPONSE_TX_THEN_DROP;
    }
    return ret;
}

static void zgp_dup_aging_alarm(zb_uint8_t param)
{
    ZVUNUSED(param);
    ZGP_CTXC().comm_data.any_packet_received = 0;
}

/**
 * @brief Check GPDF for duplicate
 *
 * @param gpdf_info  [in] Pointer to info of incoming GPDF
 * @param have_sink  [in] ZB_TRUE if device have sink table entry for this gpd_id, ZB_FALSE otherwise
 * @param have_proxy [in] ZB_TRUE if device have proxy table entry for this gpd_id, ZB_FALSE otherwise
 *
 * @see ZGP spec, A.3.6.1.2
 */
static zb_bool_t check_gpdf_for_dup(zb_gpdf_info_t *gpdf_info, zb_bool_t have_sink, zb_bool_t have_proxy)
{
    zb_bool_t   is_duplicate = ZB_FALSE;
    zb_uint32_t stored_zgpd_dup_counter = (zb_uint32_t)~0;

    TRACE_MSG(TRACE_ZGP3, ">> check_gpdf_for_dup: [have_sink: %hd], [have_proxy: %hd], incoming frame_counter: %ld",
              (FMT__H_H_L, have_sink, have_proxy, ZB_GPDF_INFO_GET_DUP_COUNTER(gpdf_info)));

#ifdef ZB_ENABLE_ZGP_SINK
    if (have_sink)
    {
        stored_zgpd_dup_counter = zgp_sink_table_get_dup_counter(&gpdf_info->zgpd_id);
        TRACE_MSG(TRACE_ZGP3, "Got dup counter %ld from the Sink table ",
                  (FMT__L, stored_zgpd_dup_counter));
    }
    else
    {
        if (ZGP_CTXC().sink_mode == ZB_ZGP_COMMISSIONING_MODE)
        {
            if (ZGP_CTXC().comm_data.any_packet_received)
            {
                stored_zgpd_dup_counter = ZGP_CTXC().comm_data.comm_dup_counter;
                TRACE_MSG(TRACE_ZGP3, "Got dup counter %ld from comm_dup_counter for sink commissioning mode",
                          (FMT__L, stored_zgpd_dup_counter));
            }
        }
        else
        {
            TRACE_MSG(TRACE_ZGP3, "Sink is in operational mode and not have sink table entry, drop GPDF further",
                      (FMT__0));
        }
    }
    is_duplicate = (zb_bool_t)(stored_zgpd_dup_counter != (zb_uint32_t)~0
                               && gpdf_info->sec_frame_counter == stored_zgpd_dup_counter);
#endif /* ZB_ENABLE_ZGP_SINK */
#ifdef ZB_ENABLE_ZGP_PROXY
    if (!is_duplicate)
    {
        if (have_proxy)
        {
            stored_zgpd_dup_counter = zgp_proxy_table_get_dup_counter(&gpdf_info->zgpd_id);
            TRACE_MSG(TRACE_ZGP3, "Got dup counter %ld from the Proxy table ",
                      (FMT__L, stored_zgpd_dup_counter));
        }
        else
        {
            if (ZGP_CTXC().proxy_mode == ZB_ZGP_COMMISSIONING_MODE)
            {
                if (ZGP_CTXC().comm_data.any_packet_received)
                {
                    stored_zgpd_dup_counter = ZGP_CTXC().comm_data.comm_dup_counter;
                    TRACE_MSG(TRACE_ZGP3, "Got dup counter %ld from comm_dup_counter for proxy commissioning mode",
                              (FMT__L, stored_zgpd_dup_counter));
                }
            }
        }
        is_duplicate = (zb_bool_t)(stored_zgpd_dup_counter != (zb_uint32_t)~0
                                   && gpdf_info->sec_frame_counter == stored_zgpd_dup_counter);
    }
#endif /* ZB_ENABLE_ZGP_PROXY */

    if (!is_duplicate)
    {
#ifdef ZB_ENABLE_ZGP_SINK
        if (ZGP_CTXC().sink_mode == ZB_ZGP_COMMISSIONING_MODE)
        {
            ZGP_CTXC().comm_data.comm_dup_counter = ZB_GPDF_INFO_GET_DUP_COUNTER((gpdf_info));
        }
#endif  /* ZB_ENABLE_ZGP_SINK */
#ifdef ZB_ENABLE_ZGP_PROXY
        if (ZGP_CTXC().proxy_mode == ZB_ZGP_COMMISSIONING_MODE)
        {
            ZGP_CTXC().comm_data.comm_dup_counter = ZB_GPDF_INFO_GET_DUP_COUNTER((gpdf_info));
        }
#endif  /* ZB_ENABLE_ZGP_PROXY */
        if (ZGP_CTXC().comm_data.any_packet_received)
        {
            ZB_SCHEDULE_ALARM_CANCEL(zgp_dup_aging_alarm, 0);
        }
        ZGP_CTXC().comm_data.any_packet_received = 1;
        ZB_SCHEDULE_ALARM(zgp_dup_aging_alarm, 0, ZB_ZGP_DUPLICATE_TIMEOUT);
    }

    TRACE_MSG(TRACE_ZGP3, "<< check_gpdf_for_dup: [is_duplicate: %hd]",
              (FMT__H, is_duplicate));
    return is_duplicate;
}

/**
Described in A.1.3.4 GP-SEC.request, A.1.3.5 GP-SEC.response,
A.3.7.3 Security operation
A.3.7.3.1 Incoming frames
A.3.5.2.3 Operation of GP Proxy Basic and proxy side of GP Combo Basic (?)
 */
static gp_sec_resp_t zb_gp_sec_request(zb_uint8_t param)
{
    gp_sec_resp_t   ret = GP_SEC_RESPONSE_ILLEGAL;
    zb_gpdf_info_t *gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);
#ifdef ZB_ENABLE_ZGP_SINK
    zgp_tbl_ent_t   s_ent;  //sink entry
#endif  /* ZB_ENABLE_ZGP_SINK */
#ifdef ZB_ENABLE_ZGP_PROXY
    zgp_tbl_ent_t   p_ent;  //proxy_entry
#endif  /* ZB_ENABLE_ZGP_PROXY */
    zb_bool_t       have_sink = ZB_FALSE;
    zb_bool_t       have_proxy = ZB_FALSE;

    TRACE_MSG(TRACE_ZGP1, ">> zb_gp_sec_request param %hd", (FMT__H, param));

    /* If I am combo, check for Sink table first */
#ifdef ZB_ENABLE_ZGP_SINK
    /* A.3.7.3.2 Sink */
    /* The sink (i.e. GPT+ and combo) checks if it has a Sink Table entry for this GPD.  */
    have_sink = (zb_bool_t)(zgp_sink_table_read(&gpdf_info->zgpd_id, &s_ent) == RET_OK);
#endif /* ZB_ENABLE_ZGP_SINK */
#ifdef ZB_ENABLE_ZGP_PROXY
    {
        have_proxy = (zb_bool_t)(zgp_proxy_table_read(&gpdf_info->zgpd_id, &p_ent) == RET_OK);
    }
#endif /* ZB_ENABLE_ZGP_PROXY */
    if (check_gpdf_for_dup(gpdf_info, have_sink, have_proxy) == ZB_TRUE)
    {
        TRACE_MSG(TRACE_ZGP3, "Duplicate! Drop ", (FMT__0));
        ret = GP_SEC_RESPONSE_DROP_FRAME;
        TRACE_MSG(TRACE_ZGP1, "<< zb_gp_sec_request ret %hd", (FMT__H, ret));
        return ret;
    }
#ifdef ZB_ENABLE_ZGP_SINK
    /* A.3.7.3.2 Sink */
#ifdef ZB_ENABLE_ZGP_PROXY
    if (!have_sink && ZGP_IS_SINK_IN_OPERATIONAL_MODE())
    {
        /* If there no Sink Table entry for this GPD and the sink is in operational mode, and the sink
         * is a combo, it SHALL act a described in A.3.7.3.3.
         */
        /* we process it below */
    }
    else
#endif  /* ZB_ENABLE_ZGP_PROXY */
    {
        ret = GP_SEC_RESPONSE_DROP_FRAME;

        if (!have_sink)
        {
            if (ZGP_IS_SINK_IN_COMMISSIONING_MODE())
            {
                if (ZB_GPDF_EXT_NFC_GET_SEC_KEY(gpdf_info->nwk_ext_frame_ctl) == 1)
                {
                    /* If there is no Sink Table entry for this GPD and the sink is in commissioning mode and
                     * the KeyType as indicated in GP-SEC.request was 0b1, the sink generates GP-SEC.response,
                     * with the Status DROP_FRAME.
                     */
                    TRACE_MSG(TRACE_ZGP3, "Sink in commissioning mode and not have a sink entry. KeyType = 0b1! Drop", (FMT__0));
                }
                else
                {
                    if (ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl) != ZB_ZGP_SEC_LEVEL_NO_SECURITY)
                    {
                        /* If there no Sink Table entry for this GPD and the sink is in commissioning mode and the
                         * KeyType as indicated in GP-SEC.request was 0b0, the sink fetches the shared key. If
                         * there is none, sink generates GP-SEC.response, with the Status DROP_FRAME. If there is,
                         * the sink generates GP-SEC.response, with the Status MATCH, and includes the key, the
                         * key type and the frame counter as processed here.
                         */
                        /* Described in A.3.7.3.4  Incoming frames: key recovery */
                        if (zgp_key_recovery(NULL, ZB_FALSE, gpdf_info->key, &gpdf_info->key_type) != RET_OK)
                        {
                            TRACE_MSG(TRACE_ZGP3, "Sink: no appropriate shared key. Drop.", (FMT__0));
                        }
                        else
                        {
                            TRACE_MSG(TRACE_ZGP3, "Sink: got shared key. Return MATCH.", (FMT__0));
                            ret = GP_SEC_RESPONSE_MATCH;
                        }
                    }
                    else
                    {
                        TRACE_MSG(TRACE_ZGP3, "Sink: frame is unsecured. Return MATCH.", (FMT__0));
                        ret = GP_SEC_RESPONSE_MATCH;
                    }
                }
            }
            else
            {
                TRACE_MSG(TRACE_ZGP3, "Sink in operational mode and not have a sink entry. Drop.", (FMT__0));
            }
        }
        else
        {
            /* If there is a Sink Table entry for this GPD  (note: if ApplicationID = 0b010, the Sink
             * Table entry may contain a different value of the Endpoint parameter than that supplied by
             * GP-SEC.request), the Sink checks the freshness of the frame and whether the SecurityLevel
             * and SecurityKeyType from the GP-SEC.request match those from the Sink Table entry; for
             * SecurityKeyType mapping Table 12 is to be used.
             */
            if (gpdf_is_fresh(gpdf_info, &s_ent))
            {
                if (zb_gp_sec_request_security_check(gpdf_info, &s_ent))
                {
                    ret = zb_gp_sec_request_match_endpoint_and_key_recovery(gpdf_info, &s_ent);

                    if (ret != GP_SEC_RESPONSE_DROP_FRAME)
                    {
                        store_frame_counter(gpdf_info, have_sink, have_proxy);
                    }
                }
                else
                {
                    if (ZGP_IS_SINK_IN_COMMISSIONING_MODE() &&
                            (ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl) == ZB_ZGP_SEC_LEVEL_NO_SECURITY) &&
                            ((gpdf_info->zgpd_cmd_id == ZB_GPDF_CMD_COMMISSIONING) || (gpdf_info->zgpd_cmd_id == ZB_GPDF_CMD_APPLICATION_DESCR)))
                    {
                        TRACE_MSG(TRACE_ZGP3, "Sink: unsecured commissioning frame. Return MATCH.", (FMT__0));
                        store_frame_counter(gpdf_info, have_sink, have_proxy);
                        ret = GP_SEC_RESPONSE_MATCH;
                    }
                }
            }
            else
            {
                TRACE_MSG(TRACE_ZGP3, "Sink: the frame is not fresh. Drop.", (FMT__0));
            }
        }
    }
#endif /* ZB_ENABLE_ZGP_SINK */

    if (ret == GP_SEC_RESPONSE_ILLEGAL)
    {
        ret = GP_SEC_RESPONSE_DROP_FRAME;

#ifdef ZB_ENABLE_ZGP_PROXY
        /* A.3.7.3.3 Proxy */
        if (have_proxy && ZGP_TBL_GET_ACTIVE(&p_ent))
        {
            /* If the proxy has an active entry  (note: if ApplicationID = 0b010, the Proxy Table entry
             * may contain a different value of the Endpoint parameter than that supplied by
             * GP-SEC.request), the proxy checks the freshness of the frame and whether the SecurityLevel
             * and SecurityKeyType from the GP-SEC.request match those from the Proxy Table entry; for
             * SecurityKeyType mapping Table 12 is to be used.
             */
            if (gpdf_is_fresh(gpdf_info, &p_ent))
            {
                if (zb_gp_sec_request_security_check(gpdf_info, &p_ent))
                {
                    ret = zb_gp_sec_request_match_endpoint_and_key_recovery(gpdf_info, &p_ent);

                    if (ret != GP_SEC_RESPONSE_DROP_FRAME)
                    {
                        store_frame_counter(gpdf_info, have_sink, have_proxy);
                    }
                }
                else
                {
                    if (ZGP_IS_PROXY_IN_COMMISSIONING_MODE() &&
                            ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl) == ZB_ZGP_SEC_LEVEL_NO_SECURITY &&
                            gpdf_info->zgpd_cmd_id == ZB_GPDF_CMD_COMMISSIONING)
                    {
                        TRACE_MSG(TRACE_ZGP3, "Proxy: unsecured commissioning frame. Return MATCH.", (FMT__0));
                        //store_frame_counter(gpdf_info, have_sink, have_proxy);
                        ret = GP_SEC_RESPONSE_TX_THEN_PASS_COMMISSIONING_UNPROCESSED;
                    }
                    else
                    {
                        /* If any of those checks fails, and the proxy is in the commissioning mode, the proxy
                         * generates GP-SEC.response, with the Status PASS_UNPROCESSED.
                         */
                        if (ZGP_IS_PROXY_IN_COMMISSIONING_MODE())
                        {
                            ret = GP_SEC_RESPONSE_PASS_UNPROCESSED;
                            store_frame_counter(gpdf_info, have_sink, have_proxy);
                            TRACE_MSG(TRACE_ZGP3, "Proxy in commissioning: security check failed. Pass unprocessed.", (FMT__0));
                        }
                        else
                        {
                            TRACE_MSG(TRACE_ZGP3, "Proxy in operational: security check failed. Drop.", (FMT__0));
                        }
                    }
                }
            }
            else
            {
                TRACE_MSG(TRACE_ZGP3, "Proxy: the frame is not fresh. Drop.", (FMT__0));
            }
        }
        else
        {
            /* If the proxy has an inactive entry and is in operational mode, it updates the SearchCounter
             * and generates GP-SEC.response, with the Status DROP_FRAME.
             */
            if (ZGP_IS_PROXY_IN_OPERATIONAL_MODE())
            {
                TRACE_MSG(TRACE_ZGP3, "Proxy in operational: not have entry or entry is inactive. Drop.", (FMT__0));
            }
            else
            {
                if (ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl) != ZB_ZGP_SEC_LEVEL_NO_SECURITY)
                {
                    if (zgp_key_recovery((have_proxy && ZGP_TBL_GET_ACTIVE(&p_ent) ? &p_ent : NULL),
                                         (zb_bool_t)ZB_GPDF_EXT_NFC_GET_SEC_KEY(gpdf_info->nwk_ext_frame_ctl),
                                         gpdf_info->key, &gpdf_info->key_type) == RET_OK)
                    {
                        /* If (i) the proxy has an inactive entry and is in commissioning mode or if there is no
                         * Proxy Table entry for this GPD and (ii) the KeyType as indicated in GP-SEC.request was
                         * 0b0, the proxy fetches the shared key.
                         */
                        /* Possible specification mean that here we need use GP-SEC.response with the Status MATCH */
                        TRACE_MSG(TRACE_ZGP3, "Proxy: got shared key. Return MATCH.", (FMT__0));
                        ret = GP_SEC_RESPONSE_MATCH;
                    }
                    else
                    {
                        /* If the key type was 0b1 or the key type was 0b0 and there is no shared key, proxy
                         * generates GP-SEC.response, with the Status PASS_UNPROCESSED.
                         */
                        ret = GP_SEC_RESPONSE_PASS_UNPROCESSED;
                        TRACE_MSG(TRACE_ZGP3, "Proxy in commissioning: not have entry or entry is inactive. Pass unprocessed.", (FMT__0));
                    }
                }
                else
                {
                    TRACE_MSG(TRACE_ZGP3, "Proxy: unsecured frame. Return MATCH.", (FMT__0));
                    ret = GP_SEC_RESPONSE_MATCH;
                }
            }
        }
#endif  /* ZB_ENABLE_ZGP_PROXY */
    }

    TRACE_MSG(TRACE_ZGP1, "<< zb_gp_sec_request ret %hd", (FMT__H, ret));
    return ret;
}

static zb_bool_t zb_dgp_data_ind_simple_check_data_frame(zb_gpdf_info_t *gpdf_info)
{
    zb_bool_t ret = ZB_FALSE;

    TRACE_MSG(TRACE_ZGP2, ">> zb_dgp_data_ind_simple_check_data_frame", (FMT__0));

    /*
      A.1.5.1.1 GPDF reception
      On receipt of a GPDF, the GP stub SHALL filter out (silently drop) frames with ApplicationID value other than 0b000, 0b010 and 0b001, frames with Direction sub-field of the Extended NWK Frame Control field set to 0b1, and duplicate frames.
      Frames with ApplicationID 0b000 and 0b010 SHALL be passed up, using dGP-DATA.indication.
    */
    if ((!(gpdf_info->zgpd_id.app_id == ZB_ZGP_APP_ID_0000
            || gpdf_info->zgpd_id.app_id == ZB_ZGP_APP_ID_0010)
            || ZB_GPDF_EXT_NFC_GET_DIRECTION(gpdf_info->nwk_ext_frame_ctl) != ZGP_FRAME_DIR_FROM_ZGPD))
    {
        TRACE_MSG(TRACE_ZGP1, "Bad app id or direction - drop frame", (FMT__0));
        return ret;
    }

#ifdef ZB_ENABLE_ZGP_SINK
    if (ZGP_IS_SINK_IN_COMMISSIONING_MODE() &&
            ZB_GPDF_NFC_GET_FRAME_TYPE(gpdf_info->nwk_frame_ctl) != ZGP_FRAME_TYPE_MAINTENANCE)
    {
        if (ZGP_CTXC().comm_data.zgpd_id.app_id != ZB_ZGP_APP_ID_0000 ||
                (ZGP_CTXC().comm_data.zgpd_id.addr.src_id != 0 &&
                 ZGP_CTXC().comm_data.zgpd_id.addr.src_id != ZB_ZGP_SRC_ID_ALL))
        {
            if (!ZB_ZGPD_IDS_ARE_EQUAL(&gpdf_info->zgpd_id, &ZGP_CTXC().comm_data.zgpd_id))
            {
                TRACE_MSG(TRACE_ZGP2, "Sink in commissioning mode, receive gpdf from other GPD, drop frame",
                          (FMT__0));
                return ret;
            }
        }
        else
        {
            if (ZB_GPDF_NFC_GET_AUTO_COMMISSIONING(gpdf_info->nwk_frame_ctl) ||
                    gpdf_info->zgpd_cmd_id == ZB_GPDF_CMD_COMMISSIONING ||
                    gpdf_info->zgpd_cmd_id == ZB_GPDF_CMD_APPLICATION_DESCR)
            {
                ZGP_CTXC().comm_data.zgpd_id = gpdf_info->zgpd_id;
            }
            else
            {
                TRACE_MSG(TRACE_ZGP2, "Sink in commissioning mode, receive not commissioning gpdf, drop frame",
                          (FMT__0));
                return ret;
            }
        }
    }
#endif  /* ZB_ENABLE_ZGP_SINK */

    if (((ZB_GPDF_NFC_GET_FRAME_TYPE(gpdf_info->nwk_frame_ctl) != ZGP_FRAME_TYPE_MAINTENANCE &&
            gpdf_info->zgpd_id.app_id == ZB_ZGP_APP_ID_0000 && gpdf_info->zgpd_id.addr.src_id == 0) ||
            (gpdf_info->zgpd_id.app_id == ZB_ZGP_APP_ID_0010 && ZB_IEEE_ADDR_IS_ZERO(gpdf_info->zgpd_id.addr.ieee_addr))))
    {
        TRACE_MSG(TRACE_ZGP1, "Source 0 - drop frame", (FMT__0));
    }
    else if (ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl) == ZB_ZGP_SEC_LEVEL_REDUCED)
    {
        /*
          If the SecurityLevel is not supported (incl. SecurityLevel = 0b01) , the dGP
          stub SHALL silently drop the frame
        */
        TRACE_MSG(TRACE_ZGP3, "Unsupported security level - drop frame", (FMT__0));
    }
    /*
      If SecurityLevel is supported and has the value of 0b00  or 0b10, and GPD
      CommandID has the value from the range 0xf0-0xff, the GPDF is silently
      dropped.
    */
    else if ((ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl) == ZB_ZGP_SEC_LEVEL_NO_SECURITY ||
              ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl) == ZB_ZGP_SEC_LEVEL_FULL_NO_ENC) &&
             gpdf_info->zgpd_cmd_id >= ZB_GPDF_CMD_COMMISSIONING_REPLY)
    {
        TRACE_MSG(TRACE_ZGP3, "CommandID has the value from the range 0xf0-0xff - drop frame", (FMT__0));
    }
    else if (ZB_GPDF_NFC_GET_AUTO_COMMISSIONING(gpdf_info->nwk_frame_ctl) &&
             ZB_GPDF_NFC_GET_FRAME_TYPE(gpdf_info->nwk_frame_ctl) == ZGP_FRAME_TYPE_DATA &&
             (ZB_GPDF_EXT_NFC_GET_RX_AFTER_TX(gpdf_info->nwk_ext_frame_ctl) ||
              ((ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl) == ZB_ZGP_SEC_LEVEL_NO_SECURITY ||
                ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl) == ZB_ZGP_SEC_LEVEL_FULL_NO_ENC) &&
               gpdf_info->zgpd_cmd_id >= ZB_GPDF_CMD_COMMISSIONING)))
    {
        /*
          If RxAfterTx sub-field was set to 0b1 in a Data GPDF (i.e. with GPD CommandID
          other than 0xE0) with Auto-Commissioning sub-field set to 0b1: silently drop the
          frame.
        */
        ZB_INIT_ZGPD_ID(&ZGP_CTXC().comm_data.zgpd_id);
        TRACE_MSG(TRACE_ZGP3, "Drop frame with Autocommissioning and RxAfterTx", (FMT__0));
    }
    else
    {
        ret = ZB_TRUE;
    }

    TRACE_MSG(TRACE_ZGP2, "<< zb_dgp_data_ind_simple_check_data_frame %hd", (FMT__H, ret));
    return ret;
}

static void zb_dgp_data_ind_continue(zb_uint8_t param)
{
    zb_gpdf_info_t *gpdf_info       = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);
    zb_uint8_t      gpd_cmd_payload = 0;

    TRACE_MSG(TRACE_ZGP2, ">> zb_dgp_data_ind_continue %hd", (FMT__H, param));

    if (gpdf_info->nwk_hdr_len)
    {
        zb_buf_cut_left(param, gpdf_info->nwk_hdr_len + sizeof(gpdf_info->zgpd_cmd_id));

        if (gpdf_info->status == GP_DATA_IND_STATUS_UNPROCESSED &&
                (ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl) != ZB_ZGP_SEC_LEVEL_NO_SECURITY))
        {
            zb_uint8_t *ptr = (zb_uint8_t *)zb_buf_begin(param);

            ZB_ASSERT(zb_buf_len(param) >= sizeof(gpdf_info->mic));

            ptr += (zb_buf_len(param) - sizeof(gpdf_info->mic));
            ZB_LETOH32(&gpdf_info->mic, ptr);
            //remove mic from payload
            zb_buf_cut_right(param, sizeof(gpdf_info->mic));
        }
        else
        {
            /* MIC, if present, has already removed during decryption, so don't remove it here */
        }
    }

    gpd_cmd_payload = zb_buf_len(param);

    /* Check maximum GPD command payload */
    if (gpd_cmd_payload <= ((gpdf_info->zgpd_id.app_id == ZB_ZGP_APP_ID_0000) ?
                            ZB_ZGP_MAX_GPDF_CMD_PAYLOAD_APP_ID_0000 : ZB_ZGP_MAX_GPDF_CMD_PAYLOAD_APP_ID_0010))
    {
        TRACE_MSG(TRACE_ZGP1, "GPD command payload: %hd", (FMT__H, gpd_cmd_payload));
#if defined ZB_ENABLE_ZGP_SINK && defined ZB_ENABLE_ZGP_PROXY
        ZB_SCHEDULE_CALLBACK(zb_gp_data_indication, param);
#else
#ifdef ZB_ENABLE_ZGP_SINK
        ZB_SCHEDULE_CALLBACK(zb_gp_sink_data_indication, param);
#else  /* ZB_ENABLE_ZGP_SINK */
        ZB_SCHEDULE_CALLBACK(zb_gp_proxy_data_indication, param);
#endif  /* ZB_ENABLE_ZGP_PROXY */
#endif  /* defined ZB_ENABLE_ZGP_SINK && defined ZB_ENABLE_ZGP_PROXY */
    }
    else
    {
        TRACE_MSG(TRACE_ZGP1, "Corrupted buffer or too big GPD command payload: %hd, DROP", (FMT__H, gpd_cmd_payload));
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_ZGP2, "<< zb_dgp_data_ind_continue", (FMT__0));
}

/**
 * @brief dGP-DATA.indication handler
 *
 * @param param [in] Buffer with incoming GPDF
 *
 * Was: old ZGP spec, A.1.3.1.1
 * Now implemented according to GPPB spec A.1.5.2.2  GPDF reception
 */
void zb_dgp_data_ind(zb_uint8_t param)
{
    zb_gpdf_info_t *gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);

    TRACE_MSG(TRACE_ZGP2, ">> zb_dgp_data_ind param %hd, buf_len: %hd", (FMT__H_H, param, zb_buf_len(param)));
    ZB_DUMP_GPDF_INFO(gpdf_info);

    /* See A.1.5.2.2  GPDF reception */
    switch (ZB_GPDF_NFC_GET_FRAME_TYPE(gpdf_info->nwk_frame_ctl))
    {
    case ZGP_FRAME_TYPE_MAINTENANCE:
    {
        TRACE_MSG(TRACE_ZGP3, "ZGP_FRAME_TYPE_MAINTENANCE", (FMT__0));

        if (check_gpdf_for_dup(gpdf_info, ZB_FALSE, ZB_FALSE) == ZB_TRUE)
        {
            TRACE_MSG(TRACE_ZGP3, "Duplicate! Drop ", (FMT__0));
            zb_buf_free(param);
            param = 0;
        }
        else if (gpdf_info->zgpd_cmd_id >= ZB_GPDF_CMD_COMMISSIONING_REPLY)
        {
            /* If the received GPD CommandID had a value from the range 0xf0-0xff, the dGP SHALL silently drop it. */
            TRACE_MSG(TRACE_ZGP3, "cmd id %hd - drop frame", (FMT__H, gpdf_info->zgpd_cmd_id));
            zb_buf_free(param);
            param = 0;
        }
#ifdef ZB_ENABLE_ZGP_DIRECT
        else if (ZB_GPDF_NFC_GET_NFC_EXT(gpdf_info->nwk_frame_ctl) && gpdf_info->rx_directly)
        {
            TRACE_MSG(TRACE_ZGP3, "Maintenance Frame with NWK Ext frame control field!", (FMT__0));
            zb_buf_free(param);
            param = 0;
        }
        else
        {
            /*
              If the received frame was of type Maintenance frame (0b01), and the GPD
              CommandID of the received GPDF does NOT have a value from the range 0xf0-0xff,
              then the dGP stub SHALL schedule transmission of the GPDF ...
            */
            /* In a Maintenance FrameType, the Auto-Commissioning sub-field, if set to 0b0, indicates
             * that the GPD will enter the receive mode gpdRxOffset ms after completion of this GPDF
             * transmission, for at least gpdMinRxWindow.  If the value of this sub-field is 0b1, then
             * the GPD will not enter the receive mode after sending this particular GPDF.
             */
            if (!ZB_GPDF_NFC_GET_AUTO_COMMISSIONING(gpdf_info->nwk_frame_ctl) &&
                    zb_zgp_try_bidir_tx(param))
            {
                TRACE_MSG(TRACE_ZGP3, "bidir tx success", (FMT__0));
            }

            /* If the TempMaster receives any other GPDF than Channel Request GPDF on
             * TransmitChannel, including a Commissioning GPDF or Success GPDF, it SHALL silently drop
             * it.
             */
            if (ZGP_CTXC().comm_data.is_work_on_temp_channel)
            {
                TRACE_MSG(TRACE_ZGP3, "Work on temp channel! Drop frame", (FMT__0));

                zb_buf_free(param);
                param = 0;
            }
        }
#endif  /* ZB_ENABLE_ZGP_DIRECT */

        if (param)
        {
            if (gpdf_info->zgpd_cmd_id == ZB_GPDF_CMD_CHANNEL_REQUEST &&
                    !(ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_IDLE ||
                      ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_CHANNEL_CONFIG_SENT))
            {
                TRACE_MSG(TRACE_ZGP3, "Channel request is processing. Drop new request", (FMT__0));
                zb_buf_free(param);
                param = 0;
            }
            else
            {
                /* so, pass up the packet */
                gpdf_info->status = GP_DATA_IND_STATUS_NO_SECURITY;
            }
        }
    }
    break;
    case ZGP_FRAME_TYPE_DATA:
    {
        gp_sec_resp_t sec_resp;

        /* Data frame (0b00)  */
        TRACE_MSG(TRACE_ZGP3, "ZGP_FRAME_TYPE_DATA", (FMT__0));

#ifdef ZB_ENABLE_ZGP_DIRECT
        if (ZGP_CTXC().comm_data.is_work_on_temp_channel)
        {
            /* If the TempMaster receives any other GPDF than Channel Request GPDF on TransmitChannel,
             * including a Commissioning GPDF or Success GPDF, it SHALL silently drop it.
             */

            TRACE_MSG(TRACE_ZGP3, "Work on temp channel! Drop ", (FMT__0));

            zb_buf_free(param);
            param = 0;
        }
#endif  /* ZB_ENABLE_ZGP_DIRECT */

        if (param && zb_dgp_data_ind_simple_check_data_frame(gpdf_info) == ZB_FALSE)
        {
            zb_buf_free(param);
            param = 0;
        }

        if (param)
        {
            sec_resp = zb_gp_sec_request(param);
            switch (sec_resp)
            {
            case GP_SEC_RESPONSE_DROP_FRAME:
                TRACE_MSG(TRACE_ZGP3, "Data frame; gp_seq_request return drop frame", (FMT__0));
                zb_buf_free(param);
                param = 0;
                break;
            case GP_SEC_RESPONSE_PASS_UNPROCESSED:
                TRACE_MSG(TRACE_ZGP3, "Data frame; gp_seq_request return PASS_UNPROCESSED - pass unproceeded frame up", (FMT__0));
                gpdf_info->status = GP_DATA_IND_STATUS_UNPROCESSED;
                break;
            case GP_SEC_RESPONSE_TX_THEN_PASS_COMMISSIONING_UNPROCESSED:
                TRACE_MSG(TRACE_ZGP3, "Commissioning frame; gp_seq_request return PASS_COMMISSIONING_UNPROCESSED - pass unproceeded frame up", (FMT__0));
#ifdef ZB_ENABLE_ZGP_DIRECT
                if (gpdf_info->rx_directly &&
                        !ZB_GPDF_NFC_GET_AUTO_COMMISSIONING(gpdf_info->nwk_frame_ctl) &&
                        ZB_GPDF_EXT_NFC_GET_RX_AFTER_TX(gpdf_info->nwk_ext_frame_ctl) &&
                        zb_zgp_try_bidir_tx(param))
                {
                    TRACE_MSG(TRACE_ZGP3, "bidir tx success", (FMT__0));
                }
#endif  /* ZB_ENABLE_ZGP_DIRECT */
                gpdf_info->status = GP_DATA_IND_STATUS_COMMISSIONING_UNPROCESSED;
                break;
            case GP_SEC_RESPONSE_TX_THEN_DROP:
            case GP_SEC_RESPONSE_MATCH:
            {
                /* unsecure */
                if (ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl) == ZB_ZGP_SEC_LEVEL_NO_SECURITY)
                {
                    gpdf_info->status = GP_DATA_IND_STATUS_NO_SECURITY;
                    TRACE_MSG(TRACE_ZGP3, "status = GP_DATA_IND_STATUS_NO_SECURITY", (FMT__0));
                }
                else
                {
#ifdef ZB_GP_ZGP_STUB_SECURITY
                    /* alien zgp stub should manually decrypts messages because has internal ZGPD entry
                       with security material */
                    if (gpdf_info->rx_directly)
                    {
                        gpdf_info->status = GP_DATA_IND_STATUS_SECURITY_SUCCESS;
                    }
                    else
#endif /* ZB_GP_ZGP_STUB_SECURITY */
                        if (gpdf_info->rx_directly ||
                                (!gpdf_info->rx_directly && gpdf_info->nwk_hdr_len &&
                                 gpdf_info->status == GP_DATA_IND_STATUS_AUTH_FAILURE))
                        {
                            zb_ret_t ret = zb_zgp_decrypt_and_auth(param);

                            if (ret == RET_OUT_OF_RANGE)
                            {
                                restore_frame_counter(gpdf_info);
                                TRACE_MSG(TRACE_ZGP3, "mailformed packet - drop frame", (FMT__0));
                                zb_buf_free(param);
                                param = 0;
                            }
                            else if (ret != RET_OK)
                            {
                                /* If security processing fails, the dGP stub indicates that with
                                 * GP-DATA.indication carrying the corresponding Status value and
                                 * stops any further processing of this frame.  */
                                restore_frame_counter(gpdf_info);
                                TRACE_MSG(TRACE_ZGP3, "decrypt or auth failure, drop frame", (FMT__0));
                                zb_buf_free(param);
                                param = 0;
                            }
                            else
                            {
                                gpdf_info->status = GP_DATA_IND_STATUS_SECURITY_SUCCESS;
                                /*
                                  If security processing is successful, and the SecurityLevel was
                                  0b11, the dGP stub checks the plaintext value of the GPD
                                  CommandID. If it has the value from the range 0xf0-0xff, the GPDF is
                                  silently dropped.

                                  Note: zb_zgp_decrypt_and_auth updates gpdf_info->zgpd_cmd_id.
                                */
                                if (ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl) == ZB_ZGP_SEC_LEVEL_FULL_WITH_ENC)
                                {
                                    if (gpdf_info->zgpd_cmd_id >= ZB_GPDF_CMD_COMMISSIONING_REPLY)
                                    {
                                        restore_frame_counter(gpdf_info);
                                        TRACE_MSG(TRACE_ZGP3, "cmd id %hd for data frame - drop frame", (FMT__H, gpdf_info->zgpd_cmd_id));
                                        zb_buf_free(param);
                                        param = 0;
                                    }
                                    else if (ZB_GPDF_NFC_GET_AUTO_COMMISSIONING(gpdf_info->nwk_frame_ctl) &&
                                             gpdf_info->zgpd_cmd_id >= ZB_GPDF_CMD_COMMISSIONING)
                                    {
                                        restore_frame_counter(gpdf_info);
                                        TRACE_MSG(TRACE_ZGP3, "in data frame auto-commissioning flag with commissioning command %hd - drop frame",
                                                  (FMT__H, gpdf_info->zgpd_cmd_id));
                                        zb_buf_free(param);
                                        param = 0;
                                    }
                                }
                            }
                        }  /* recv directly or tunneled with auth_failure */
                }  /* security frame */

                if (param)
                {
                    /* If security processing was successful, and the GPD CommandID is not from the 0xf0
                     * - 0xff range,  the dGP stub checks if the RxAfterTx sub-field of the Extended
                     * NWK Frame Control field of the re-ceived GPDF was set to 0b1. If yes, it searches
                     * the gpTxQueue for an entry...
                     */
#ifdef ZB_ENABLE_ZGP_DIRECT
                    if (gpdf_info->rx_directly &&
                            !ZB_GPDF_NFC_GET_AUTO_COMMISSIONING(gpdf_info->nwk_frame_ctl) &&
                            ZB_GPDF_EXT_NFC_GET_RX_AFTER_TX(gpdf_info->nwk_ext_frame_ctl) &&
                            zb_zgp_try_bidir_tx(param))
                    {
                        TRACE_MSG(TRACE_ZGP3, "bidir tx success", (FMT__0));
                    }
#endif  /* ZB_ENABLE_ZGP_DIRECT */

                    /* Otherwise, if the Status of the GP-SEC.response was MATCH, and if no matching
                     * entry is found in the gpTxQueue, the GP stub indicates reception of the GPDF to
                     * the next higher layer, by calling GP-DATA.indication. If SecurityLevel was 0b00,
                     * the dGP calls GP-DATA.indication with the Status NO_SECURITY; if SecurityLevel
                     * was 0b10 - 0b11, the dGP calls GP-DATA.indication with the Status
                     * SECURITY_SUCCESS.
                     */
                    if (sec_resp == GP_SEC_RESPONSE_TX_THEN_DROP)
                    {
                        TRACE_MSG(TRACE_ZGP3, "SEC-RESPONSE:TX_THEN_DROP - drop frame", (FMT__0));
                        zb_buf_free(param);
                        param = 0;
                    }
                }
            }
            break;
            default:
                TRACE_MSG(TRACE_ZGP3, "DEFAULT sec_resp: %hd", (FMT__H, sec_resp));
                ZB_ASSERT(0);
            }; /* switch GP-SEC.response */
        }
    }
    break;
    default:
        /* drop bad frame type */
        TRACE_MSG(TRACE_ZGP3, "drop BAD Frame type: 0x%02x", (FMT__H, ZB_GPDF_NFC_GET_FRAME_TYPE(gpdf_info->nwk_frame_ctl)));
        zb_buf_free(param);
        param = 0;
    }; /* switch frame type */

    if (param)
    {
        /* continue data indication process */
        zb_dgp_data_ind_continue(param);
    }

    TRACE_MSG(TRACE_ZGP2, "<< zb_dgp_data_ind", (FMT__0));
}

#endif  /* #ifdef ZB_ENABLE_ZGP  */
