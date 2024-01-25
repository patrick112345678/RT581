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
/* PURPOSE: Test Profile 2 implementation
*/

#define ZB_TRACE_FILE_ID 2090
#include "zb_common.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_test_profile.h"
#ifdef TEST_BLE_STRESS
#include "util_log.h"
#endif
#ifdef ZB_TEST_PROFILE
#include "../nwk/nwk_internal.h"

/* WARNING: DO NOT MERGE! Temporary changes for APS fragmentation debug! */

/*! @cond internals_doc */
/*! \addtogroup zb_af */
/*! @{ */

#ifdef USE_COUNTER_RES_REQ
zb_uint16_t counted_packets_counter = 0;
#endif
static void tp_buffer_test_request_handler(zb_uint8_t param);
static void tp_buffer_test_response_handler(zb_uint8_t param);
static void tp_buffer_test_response(zb_uint8_t param, zb_uint16_t cluster);
#ifdef USE_COUNTER_RES_REQ
static void tp_buffer_test_reset_counter(void);
static void tp_buffer_test_counter_response(zb_uint8_t param);
#endif

#ifdef TEST_APS_FRAGMENTATION
void tp_buffer_counted_packets_ind(zb_uint8_t param);
#endif

#ifdef TEST_BLE_STRESS
/*thsi cmd use private cluster id and the cmd structure same as zb_buffer_test_req_t*/
void tp_buffer_test_cmd(zb_uint8_t param, zb_uint8_t cmd);
static void tp_buffer_test_cmd_handler(zb_uint8_t param);
#endif /*TEST_BLE_STRESS*/

void zb_test_profile_indication(zb_uint8_t param)
{
    zb_apsde_data_indication_t *ind = (zb_apsde_data_indication_t *)ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);
    zb_uint8_t skip_free_buf = 1;

    TRACE_MSG(TRACE_ZDO1, "zb_test_profile_indication %hd clu 0x%hx", (FMT__H_H, param, ind->clusterid));

    if ((ind->clusterid  == TP_BUFFER_TEST_REQUEST_CLID)
            || (ind->clusterid  == TP_BUFFER_TEST_REQUEST_CLID2))
    {
        TRACE_MSG(TRACE_ZDO1, "BUFFER_TEST_REQUEST", (FMT__0));
        tp_buffer_test_request_handler(param);
    }
#ifdef USE_RESP_FOR_COUNTED_BUFS
    else if (ind->clusterid  == TP_TRANSMIT_COUNTED_PACKETS_REQ_CLID)
    {
        TRACE_MSG(TRACE_ZDO1, "BUFFER_TEST_COUNTED_REQUEST", (FMT__0));
        tp_buffer_test_request_handler(param);
    }
#endif
    else if (ind->clusterid  == TP_BUFFER_TEST_RESPONSE_CLID )
    {
        TRACE_MSG(TRACE_ZDO1, "BUFFER_TEST_RESPONSE", (FMT__0));
        tp_buffer_test_response_handler(param);
    }
    else if (ind->clusterid  == TP_TRANSMIT_COUNTED_PACKETS_RES_CLID)
    {
        TRACE_MSG(TRACE_ZDO1, "BUFFER_TEST_COUNTED_RESPONSE (drop)", (FMT__0));
        skip_free_buf = 0;
    }
    else if (ind->clusterid  == TP_RESET_PACKET_COUNT_CLID)
    {
        TRACE_MSG(TRACE_ZDO1, "BUFFER_TEST_RESET_COUNTER", (FMT__0));
        tp_buffer_test_reset_counter();
        skip_free_buf = 0;
    }
    else if (ind->clusterid  == TP_RETRIEVE_PACKET_COUNT_CLID)
    {
        TRACE_MSG(TRACE_ZDO1, "BUFFER_TEST_RETRIVE_PACKET_COUNT", (FMT__0));
        tp_buffer_test_counter_response(param);
    }
    else if (ind->clusterid  == TP_PACKET_COUNT_RESPONSE_CLID)
    {
        TRACE_MSG(TRACE_ZDO1, "BUFFER_TEST_RETRIVE_PACKET_COUNT_RESPONSE (drop)", (FMT__0));
        skip_free_buf = 0;
    }
    else
    {
        TRACE_MSG(TRACE_ZDO1, "unhandl clu %hd - drop", (FMT__H, ind->clusterid));
        skip_free_buf = 0;
    }


    if (!skip_free_buf)
    {
        zb_buf_free(param);
    }
}

void tp_start_send_counted_packet(zb_uint8_t param)
{
    ZVUNUSED(param);
    zb_buf_get_out_delayed(tp_send_counted_packet);
}

void tp_send_counted_packet(zb_uint8_t param)
{
    zb_tp_transmit_counted_packets_req_t *req;
    zb_ushort_t i = 0;

    TRACE_MSG(TRACE_ZDO3, "send_counted_packet pack num %d, counter %d",
              (FMT__D_D, ZG->zdo.test_prof_ctx.tp_counted_packets.params.packets_number, ZG->zdo.test_prof_ctx.tp_counted_packets.counter));

    if (ZG->zdo.test_prof_ctx.tp_counted_packets.params.packets_number > ZG->zdo.test_prof_ctx.tp_counted_packets.counter)
    {
        req = zb_buf_initial_alloc(param,
                                   sizeof(zb_tp_transmit_counted_packets_req_t) - sizeof(zb_uint8_t) + ZG->zdo.test_prof_ctx.tp_counted_packets.params.len);

        req->len = ZG->zdo.test_prof_ctx.tp_counted_packets.params.len; /* data length */
        req->counter = ZG->zdo.test_prof_ctx.tp_counted_packets.counter++;
        for (i = 0; i < ZG->zdo.test_prof_ctx.tp_counted_packets.params.len; ++i)
        {
            req->req_data[i] = i;
        }
        TRACE_MSG(TRACE_APS3, "radius %hd", (FMT__H, ZG->zdo.test_prof_ctx.tp_counted_packets.params.radius));
        tp_send_req_by_short(TP_TRANSMIT_COUNTED_PACKETS_REQ_CLID,
                             param,
                             ZB_TEST_PROFILE_ID,
                             ZG->zdo.test_prof_ctx.tp_counted_packets.params.dst_addr,
                             ZG->zdo.test_prof_ctx.tp_counted_packets.params.addr_mode,
                             /* ZB_TEST_PROFILE_EP */ ZG->zdo.test_prof_ctx.tp_counted_packets.params.src_ep,
                             /* ZB_TEST_PROFILE_EP */ ZG->zdo.test_prof_ctx.tp_counted_packets.params.dst_ep,
                             /* ZB_APSDE_TX_OPT_ACK_TX */ 0,
                             ZG->zdo.test_prof_ctx.tp_counted_packets.params.radius ? ZG->zdo.test_prof_ctx.tp_counted_packets.params.radius : MAX_NWK_RADIUS);

        ZB_SCHEDULE_ALARM(tp_start_send_counted_packet, 0,
                          ZB_MILLISECONDS_TO_BEACON_INTERVAL(ZG->zdo.test_prof_ctx.tp_counted_packets.params.idle_time));
    }
    else
    {
        ZB_SCHEDULE_CALLBACK(ZG->zdo.test_prof_ctx.tp_counted_packets.user_cb, param);
        zb_buf_free(param);
    }
}

/* Rafael added */
void tp_stop_counted_packet(void)
{
    ZB_SCHEDULE_ALARM_CANCEL(tp_start_send_counted_packet, ZB_ALARM_ANY_PARAM);
}

/*! [tp_send_counted_packet] */
void zb_tp_transmit_counted_packets_req(zb_uint8_t param, zb_callback_t cb)
{
    zb_tp_transmit_counted_packets_param_t *params = ZB_BUF_GET_PARAM(param, zb_tp_transmit_counted_packets_param_t);

    /* Rafael added, avoid buffer full */
    tp_stop_counted_packet();

    TRACE_MSG(TRACE_ZDO3, "transmit_counted_packets_req param %hd", (FMT__H, param));

    ZB_MEMCPY(&ZG->zdo.test_prof_ctx.tp_counted_packets.params, params, sizeof(zb_tp_transmit_counted_packets_param_t));
    ZG->zdo.test_prof_ctx.tp_counted_packets.user_cb = cb;
    ZG->zdo.test_prof_ctx.tp_counted_packets.counter = 0;
    TRACE_MSG(TRACE_APS3, "radius %hd", (FMT__H, ZG->zdo.test_prof_ctx.tp_counted_packets.params.radius));
    tp_send_counted_packet(param);
}
/*! [tp_send_counted_packet] */


/* Added for test tp_r21_bv-25 */
void tp_start_send_counted_packet_ext(zb_uint8_t param)
{
    ZVUNUSED(param);
    zb_buf_get_out_delayed(tp_send_counted_packet_ext);
}

void tp_send_counted_packet_ext(zb_uint8_t param)
{
    zb_tp_transmit_counted_packets_req_t *req;
    zb_ushort_t i = 0;

    TRACE_MSG(TRACE_ZDO3, "send_counted_packet_ext pack num %d, counter %d",
              (FMT__D_D, ZG->zdo.test_prof_ctx.tp_counted_packets.params.packets_number,
               ZG->zdo.test_prof_ctx.tp_counted_packets.counter));

    if (ZG->zdo.test_prof_ctx.tp_counted_packets.params.packets_number >
            ZG->zdo.test_prof_ctx.tp_counted_packets.counter)
    {
        req = zb_buf_initial_alloc(param,
                                   sizeof(zb_tp_transmit_counted_packets_req_t) -
                                   sizeof(zb_uint8_t) + ZG->zdo.test_prof_ctx.tp_counted_packets.params.len);

        req->len = ZG->zdo.test_prof_ctx.tp_counted_packets.params.len; /* data length */
        req->counter = ZG->zdo.test_prof_ctx.tp_counted_packets.counter++;

        for (i = 0; i < ZG->zdo.test_prof_ctx.tp_counted_packets.params.len; ++i)
        {
            req->req_data[i] = i;
        }

        tp_send_req_by_short_ext(TP_TRANSMIT_COUNTED_PACKETS_REQ_CLID,
                                 param,
                                 ZB_TEST_PROFILE_ID,
                                 ZG->zdo.test_prof_ctx.tp_counted_packets.params.dst_addr,
                                 ZG->zdo.test_prof_ctx.tp_counted_packets.params.addr_mode,
                                 ZG->zdo.test_prof_ctx.tp_counted_packets.params.src_ep,
                                 ZG->zdo.test_prof_ctx.tp_counted_packets.params.dst_ep,
                                 ZB_APSDE_TX_OPT_ACK_TX,
                                 MAX_NWK_RADIUS);

        ZB_SCHEDULE_ALARM(tp_start_send_counted_packet_ext, 0,
                          ZB_MILLISECONDS_TO_BEACON_INTERVAL(ZG->zdo.test_prof_ctx.tp_counted_packets.params.idle_time));
    }
    else
    {
        ZB_SCHEDULE_CALLBACK(ZG->zdo.test_prof_ctx.tp_counted_packets.user_cb, param);
    }
}


void zb_tp_transmit_counted_packets_req_ext(zb_uint8_t param, zb_callback_t cb)
{
    zb_tp_transmit_counted_packets_param_t *params = ZB_BUF_GET_PARAM(param,
            zb_tp_transmit_counted_packets_param_t);

    TRACE_MSG(TRACE_ZDO3, "transmit_counted_packets_req_ext param %hd", (FMT__H, param));

    ZB_MEMCPY(&ZG->zdo.test_prof_ctx.tp_counted_packets.params,
              params, sizeof(zb_tp_transmit_counted_packets_param_t));
    ZG->zdo.test_prof_ctx.tp_counted_packets.user_cb = cb;
    ZG->zdo.test_prof_ctx.tp_counted_packets.counter = 0;

    tp_send_counted_packet_ext(param);
}


void tp_send_req_by_short_ext(zb_uint16_t command_id, zb_uint8_t param,
                              zb_uint16_t profile_id, zb_uint16_t addr,
                              zb_uint8_t addr_mode, zb_uint8_t src_ep,
                              zb_uint8_t dst_ep, zb_uint8_t tx_options,
                              zb_uint8_t radius)
{
    zb_apsde_data_req_t *dreq = ZB_BUF_GET_PARAM(param, zb_apsde_data_req_t);

    TRACE_MSG(TRACE_ZDO2, "tp_send_req_by_short_ext param %hd", (FMT__H, param));
    ZB_BZERO(dreq, sizeof(*dreq));

    dreq->profileid = profile_id;
    dreq->clusterid = command_id;
    dreq->dst_endpoint = dst_ep;
    dreq->src_endpoint = src_ep;
    dreq->dst_addr.addr_short = addr;
    dreq->tx_options = tx_options;
    /* see: ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT,
     *      ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
     *      ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT
     */
    dreq->addr_mode = addr_mode;
    dreq->radius = radius;

    ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, param);
}


void tp_send_req_by_short(zb_uint16_t command_id, zb_uint8_t param, zb_uint16_t profile_id, zb_uint16_t addr, zb_uint8_t addr_mode, zb_uint8_t src_ep, zb_uint8_t dst_ep, zb_uint8_t tx_options, zb_uint8_t radius)
{
    zb_apsde_data_req_t *dreq = ZB_BUF_GET_PARAM(param, zb_apsde_data_req_t);

    TRACE_MSG(TRACE_ZDO2, "tp_send_req_by_short param %hd", (FMT__H, param));
    ZB_BZERO(dreq, sizeof(*dreq));

    dreq->profileid = profile_id;
    dreq->clusterid = command_id;
    dreq->dst_endpoint = dst_ep;
    dreq->src_endpoint = src_ep;
    dreq->dst_addr.addr_short = addr;
    dreq->tx_options = tx_options;

    if (!IS_DISTRIBUTED_SECURITY())
    {
        /* Check for the existence of aps-keys */
        zb_ieee_addr_t dst_ieee_addr;
        zb_aps_device_key_pair_set_t *aps_key = NULL;
        if (addr_mode == ZB_APS_ADDR_MODE_16_ENDP_PRESENT
                && zb_address_ieee_by_short(addr, dst_ieee_addr) == RET_OK
                && !ZB_IEEE_ADDR_IS_UNKNOWN(dst_ieee_addr))
        {
            aps_key = zb_secur_get_link_key_pair_set(dst_ieee_addr, ZB_TRUE);
            if (aps_key)
            {
                dreq->tx_options |= ZB_APSDE_TX_OPT_SECURITY_ENABLED;
            }
        }
    }
    /* see: ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT */
    dreq->addr_mode = addr_mode;

    if (dreq->addr_mode == ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT
            || dreq->addr_mode == ZB_APS_ADDR_MODE_16_ENDP_PRESENT)
    {
        dreq->radius = radius;
    }

    ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, param);
}

void tp_send_req_by_EP(zb_uint16_t command_id, zb_uint8_t param, zb_uint8_t src_ep, zb_uint8_t dst_ep, zb_uint8_t tx_options)
{
    zb_apsde_data_req_t *dreq = ZB_BUF_GET_PARAM(param, zb_apsde_data_req_t);

    TRACE_MSG(TRACE_ZDO3, "> tp_send_req_by_EP param %hd, src_ep %hd, dst_ep %hd", (FMT__H_H_H, param, src_ep, dst_ep));
    ZB_BZERO(dreq, sizeof(*dreq));

    dreq->profileid = ZB_TEST_PROFILE_ID;
    dreq->clusterid = command_id;
    dreq->dst_endpoint = dst_ep;
    dreq->src_endpoint = src_ep;
    dreq->tx_options = tx_options;
    dreq->addr_mode = ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT;
    ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, param);

    TRACE_MSG(TRACE_ZDO3, "< tp_send_req_by_EP", (FMT__0));
}

#ifdef TEST_BLE_STRESS
/*thsi cmd use private cluster id and the cmd structure same as zb_buffer_test_req_t*/
void tp_buffer_test_cmd(zb_uint8_t param, zb_uint8_t cmd)
{
    zb_buffer_test_req_param_t *req_param = ZB_BUF_GET_PARAM(param, zb_buffer_test_req_param_t);
    zb_buffer_test_req_t *req;

    req = zb_buf_initial_alloc(param, sizeof(zb_buffer_test_req_t) + req_param->len - 1);
    req->len = 1;
    /* #AT */
    req->req_data[0] = cmd;

    tp_send_req_by_short(TP_BUFFER_TEST_CMD_CLID, param, req_param->profile_id, req_param->dst_addr, req_param->addr_mode, req_param->src_ep, req_param->dst_ep, 0, req_param->radius);
}

void buffer_test_req_timeout(zb_uint8_t param)
{
    ZVUNUSED(param);
    TRACE_MSG(TRACE_ZDO2, "buffer_test_req_timeout", (FMT__0));
    if (ZG->zdo.test_prof_ctx.zb_tp_buffer_test_request.user_cb)
    {
        ZB_SCHEDULE_CALLBACK2(ZG->zdo.test_prof_ctx.zb_tp_buffer_test_request.user_cb, ZB_TP_BUFFER_TEST_FAIL, NULL);
        ZG->zdo.test_prof_ctx.zb_tp_buffer_test_request.user_cb = NULL;
    }
}

/* 6.7  Buffer test request, CID=0x1c */
void zb_tp_buffer_test_request(zb_uint8_t param, zb_callback2_t cb)
{
    if (param == 0)
    {
        TRACE_MSG(TRACE_ERROR, "BUFID = 0, Buffer test not sent.", (FMT__0));
        ZG->zdo.test_prof_ctx.zb_tp_buffer_test_request.user_cb = cb;
        TRACE_MSG(TRACE_ZDO3, "sched buffer_test_req_timeout, timeout %d", (FMT__D, 10 * ZB_TIME_ONE_SECOND));
        /* use callback to determine that response is not received */
        //ZB_SCHEDULE_ALARM(buffer_test_req_timeout, 0,  30 * ZB_TIME_ONE_SECOND);
        ZB_SCHEDULE_ALARM(buffer_test_req_timeout, 0,  1 * ZB_TIME_ONE_SECOND);
    }
    else
    {
        zb_buffer_test_req_param_t *req_param = ZB_BUF_GET_PARAM(param, zb_buffer_test_req_param_t);
        zb_buffer_test_req_t *req;
        int i = 0;

        TRACE_MSG(TRACE_ZDO2, "zb_tp_buffer_test_request param %hd, len %hd", (FMT__H_H, param, req_param->len));
        req = zb_buf_initial_alloc(param, sizeof(zb_buffer_test_req_t) + req_param->len - 1);
        req->len = req_param->len;
        /* #AT */

        req->req_data[0] = req_param->seq_num;
        for (i = 1; i < req_param->len; ++i)
        {
            req->req_data[i] = i;
        }

        ZG->zdo.test_prof_ctx.zb_tp_buffer_test_request.user_cb = cb;
        TRACE_MSG(TRACE_ZDO3, "sched buffer_test_req_timeout, timeout %d", (FMT__D, 10 * ZB_TIME_ONE_SECOND));
        /* use callback to determine that response is not received */
        //ZB_SCHEDULE_ALARM(buffer_test_req_timeout, 0,  30 * ZB_TIME_ONE_SECOND);
        ZB_SCHEDULE_ALARM(buffer_test_req_timeout, 0,  1 * ZB_TIME_ONE_SECOND);

        tp_send_req_by_short(TP_BUFFER_TEST_REQUEST_CLID, param, req_param->profile_id, req_param->dst_addr, req_param->addr_mode, req_param->src_ep, req_param->dst_ep, 0, req_param->radius);
    }
}

/* 6.7  Buffer test request, CID=0x1c */
void zb_tp_buffer_test_request_EP(zb_uint8_t param, zb_callback2_t cb)
{
    zb_buffer_test_req_param_EP_t *req_param = ZB_BUF_GET_PARAM(param, zb_buffer_test_req_param_EP_t);
    zb_buffer_test_req_t *req;
    zb_uindex_t i;

    TRACE_MSG(TRACE_ZDO3, "> zb_tp_buffer_test_request_EP param %hd, len %hd", (FMT__H_H, param, req_param->len));
    req = zb_buf_initial_alloc(param, sizeof(zb_buffer_test_req_t) + (req_param->len - 1) * sizeof(zb_uint8_t));
    req->len = req_param->len;
    for (i = 0; i < req_param->len; ++i)
    {
        req->req_data[i] = i;
    }

    ZG->zdo.test_prof_ctx.zb_tp_buffer_test_request.user_cb = cb;
    TRACE_MSG(TRACE_ZDO3, "zb_tp_buffer_test_request_EP, timeout %d", (FMT__D, 30 * ZB_TIME_ONE_SECOND));
    /* use callback to determine that response is not received */
    ZB_SCHEDULE_ALARM(buffer_test_req_timeout, 0, 30 * ZB_TIME_ONE_SECOND);

    TRACE_MSG(TRACE_ZDO3, "request_EP src ep %hd, dst ep %hd", (FMT__H_H, req_param->src_ep, req_param->dst_ep));
    tp_send_req_by_EP(TP_BUFFER_TEST_REQUEST_CLID, param, req_param->src_ep, req_param->dst_ep, 0);

    TRACE_MSG(TRACE_ZDO3, "< zb_tp_buffer_test_request_EP", (FMT__0));
}

static void tp_buffer_test_request_handler(zb_uint8_t param)
{
    static zb_uint8_t  g_zb_stress_rx_index = 0;
    static zb_uint32_t g_zb_stress_wrong_count = 0;   // received wrong data count
    static zb_uint32_t g_zb_stress_duplicate_count = 0;   // received wrong data count
    zb_uint8_t *data = 0, i, seq;
    zb_buffer_test_response_param_t *test_param;
    zb_apsde_data_indication_t ind;
    zb_uint8_t len = 0;

    TRACE_MSG(TRACE_ZDO3, "tp_buffer_test_request_handler %hd", (FMT__D, param));

    ZB_MEMCPY(&ind, ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t), sizeof(ind));

    /* Get payload size, do this in the beginning, later buffer will be overwritten */
    len = *((zb_uint8_t *)(zb_buf_begin(param)));

    test_param = ZB_BUF_GET_PARAM(param, zb_buffer_test_response_param_t);
    test_param->src_ep = ind.dst_endpoint;
    test_param->dst_ep = ind.src_endpoint;
    test_param->dst_addr = ind.src_addr;
    test_param->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
    test_param->len = len;
    test_param->status = ZB_TP_BUFFER_TEST_OK;

    TRACE_MSG(TRACE_ZDO3, "src addr 0x%x, dst addr 0x%x src ept %hd, dst ept %hd payload len = %hd",
              (FMT__H_H_H_H_H, ind.src_addr, ind.dst_addr, ind.src_endpoint, ind.dst_endpoint, len));

    data = ((zb_uint8_t *)(zb_buf_begin(param)));
    seq = data[1];
    if (seq != g_zb_stress_rx_index)
    {
        if (((seq + 1) == g_zb_stress_rx_index) || ((seq == 255) && (g_zb_stress_rx_index == 0)))
        {
            g_zb_stress_duplicate_count++;
            info("\nDups[%d]=%d\n", seq, g_zb_stress_rx_index);
        }
        else
        {
            info("\nErrs[%d]=%d\n", seq, g_zb_stress_rx_index);
            g_zb_stress_rx_index = seq + 1;
            g_zb_stress_wrong_count++;

        }
    }
    else
    {
        g_zb_stress_rx_index++;
    }

    for (i = 1; i < data[0]; i++)
    {
        if (*(data + 1 + i) != i)
        {
            info("\nPkt seq = %d, ErrD [%d] = %d\n", g_zb_stress_rx_index, i, *(data + 1 + i));
            g_zb_stress_wrong_count++;
        }
    }

    if (g_zb_stress_wrong_count == 0)
    {
        info_color(LOG_RED, ".");
    }
    else
    {
        info_color(LOG_RED, "%d", g_zb_stress_wrong_count);
    }
    info_color(LOG_CYAN, "%d", g_zb_stress_duplicate_count);


    /* When send request to the group which includes us (test aps/bv-16-i), we
     * pass here our own packet. Do not talk to myself!  */
    if (test_param->dst_addr == ZB_PIBCACHE_NETWORK_ADDRESS())
    {
        TRACE_MSG(TRACE_ZDO1, "do not andwer to myself", (FMT__0));
        zb_buf_free(param);
    }
    else
    {
#ifdef USE_RESP_FOR_COUNTED_BUFS
        TRACE_MSG(TRACE_ZDO3, "tp_buffer_test_request_handler: cluster = 0x%x;", (FMT__H, ind.clusterid));
        if (ind.clusterid == TP_TRANSMIT_COUNTED_PACKETS_REQ_CLID)
        {
            /* Note: according to the Test Profile 2,
               6.2    Transmit counted packets, CID=0x0001,
               we must not answer on counted_packets request! */
            zb_buf_free(param);
            counted_packets_counter++;
            TRACE_MSG(TRACE_ZDO3, "counted packets %d", (FMT__D, counted_packets_counter));
        }
        else
#endif
        {
            tp_buffer_test_response(param, TP_BUFFER_TEST_RESPONSE_CLID);
        }
    }
}


void tp_buffer_test_response(zb_uint8_t param, zb_uint16_t cluster)
{
    zb_buffer_test_response_t *test_resp;
    zb_buffer_test_response_param_t *test_param;
    zb_uint8_t *aps_body, *data;
    zb_uindex_t i;
    zb_uint8_t send_len;

    data = ((zb_uint8_t *)(zb_buf_begin(param)));

    TRACE_MSG(TRACE_ZDO3, "tp_buffer_test_response %x", (FMT__D, param));
    test_param = ZB_BUF_GET_PARAM(param, zb_buffer_test_response_param_t);
    send_len = (test_param->status == ZB_TP_BUFFER_TEST_OK) ? test_param->len : 0;
    TRACE_MSG(TRACE_ZDO3, "len %hd, status %hd, send_len %hd", (FMT__H_H_H, test_param->len, test_param->status, send_len));
    test_resp = zb_buf_initial_alloc(param, sizeof(zb_buffer_test_response_t) + send_len);
    TRACE_MSG(TRACE_ZDO3, "src ept resp %hd, dst ept resp%hd", (FMT__H_H, test_param->src_ep, test_param->dst_ep));
    test_resp->len = test_param->len;
    test_resp->status = test_param->status;

    if (test_param->status == ZB_TP_BUFFER_TEST_OK)
    {
        aps_body = (zb_uint8_t *)(test_resp + 1);
        for (i = 0; i < test_param->len; i++)
        {
            if (i == 0)
            {
                *aps_body = data[1];
            }
            else
            {
                *aps_body = i;
            }
            aps_body++;
        }
    }

    TRACE_MSG(TRACE_ATM1, "Z< send buffer test response, dst_addr = 0x%04x", (FMT__D, test_param->dst_addr));
    tp_send_req_by_short(cluster, param, ZB_TEST_PROFILE_ID, test_param->dst_addr, test_param->addr_mode, test_param->src_ep, test_param->dst_ep,
                         0, MAX_NWK_RADIUS);
}

static void tp_buffer_test_response_handler(zb_uint8_t param)
{
    zb_buffer_test_response_t *test_resp;
    zb_uint8_t *data;

    TRACE_MSG(TRACE_ZDO2, "tp_buffer_test_response_handler %x", (FMT__D, param));

    test_resp = (zb_buffer_test_response_t *)zb_buf_begin(param);

    data = (zb_uint8_t *)test_resp;
    /* response on test profile 2 buffer request at payload beginig exists length and status fields */
    if (zb_buf_len(param) >= sizeof(zb_buffer_test_response_t))
    {
        TRACE_MSG(TRACE_ZDO2, "tp: status %hd, len %hd", (FMT__H_H, test_resp->status, test_resp->len));
        ZB_SCHEDULE_ALARM_CANCEL(buffer_test_req_timeout, 0);
        if (ZG->zdo.test_prof_ctx.zb_tp_buffer_test_request.user_cb)
        {
            //ZB_SCHEDULE_CALLBACK(ZG->zdo.test_prof_ctx.zb_tp_buffer_test_request.user_cb, test_resp->status);
            ZB_SCHEDULE_CALLBACK2(ZG->zdo.test_prof_ctx.zb_tp_buffer_test_request.user_cb, test_resp->status, data[2]);
        }
        ZG->zdo.test_prof_ctx.zb_tp_buffer_test_request.user_cb = NULL;
    }
    else
    {
        TRACE_MSG(TRACE_ZDO2, "tp: status %hd", (FMT__H, test_resp->status));
    }
    zb_buf_free(param);
}

#else
void buffer_test_req_timeout(zb_uint8_t param)
{
    ZVUNUSED(param);
    TRACE_MSG(TRACE_ZDO2, "buffer_test_req_timeout", (FMT__0));
    if (ZG->zdo.test_prof_ctx.zb_tp_buffer_test_request.user_cb)
    {
        ZB_SCHEDULE_CALLBACK(ZG->zdo.test_prof_ctx.zb_tp_buffer_test_request.user_cb, ZB_TP_BUFFER_TEST_FAIL);
        ZG->zdo.test_prof_ctx.zb_tp_buffer_test_request.user_cb = NULL;
    }
}

/* 6.7  Buffer test request, CID=0x1c */
void zb_tp_buffer_test_request(zb_uint8_t param, zb_callback_t cb)
{
    if (param == 0)
    {
        TRACE_MSG(TRACE_ERROR, "BUFID = 0, Buffer test not sent.", (FMT__0));
        ZG->zdo.test_prof_ctx.zb_tp_buffer_test_request.user_cb = cb;
        TRACE_MSG(TRACE_ZDO3, "sched buffer_test_req_timeout, timeout %d", (FMT__D, 10 * ZB_TIME_ONE_SECOND));
        /* use callback to determine that response is not received */
        //ZB_SCHEDULE_ALARM(buffer_test_req_timeout, 0,  30 * ZB_TIME_ONE_SECOND);
        //ZB_SCHEDULE_ALARM(buffer_test_req_timeout, 0,  8 * ZB_TIME_ONE_SECOND);
    }
    else
    {
        zb_buffer_test_req_param_t *req_param = ZB_BUF_GET_PARAM(param, zb_buffer_test_req_param_t);
        zb_buffer_test_req_t *req;
        int i = 0;

        TRACE_MSG(TRACE_ZDO2, "zb_tp_buffer_test_request param %hd, len %hd", (FMT__H_H, param, req_param->len));
        req = zb_buf_initial_alloc(param, sizeof(zb_buffer_test_req_t) + req_param->len - 1);
        req->len = req_param->len;
        /* #AT */

        for (i = 0; i < req_param->len; ++i)
        {
            req->req_data[i] = i;
        }

        ZG->zdo.test_prof_ctx.zb_tp_buffer_test_request.user_cb = cb;
        TRACE_MSG(TRACE_ZDO3, "sched buffer_test_req_timeout, timeout %d", (FMT__D, 10 * ZB_TIME_ONE_SECOND));
        /* use callback to determine that response is not received */
        //ZB_SCHEDULE_ALARM(buffer_test_req_timeout, 0,  30 * ZB_TIME_ONE_SECOND);
        //ZB_SCHEDULE_ALARM(buffer_test_req_timeout, 0,  8 * ZB_TIME_ONE_SECOND);

        tp_send_req_by_short(TP_BUFFER_TEST_REQUEST_CLID, param,
                             req_param->profile_id, req_param->dst_addr, req_param->addr_mode,
                             req_param->src_ep, req_param->dst_ep,
                             ZB_APSDE_TX_OPT_ACK_TX, req_param->radius);
    }
}

/* 6.7  Buffer test request, CID=0x1c */
void zb_tp_buffer_test_request_EP(zb_uint8_t param, zb_callback_t cb)
{
    zb_buffer_test_req_param_EP_t *req_param = ZB_BUF_GET_PARAM(param, zb_buffer_test_req_param_EP_t);
    zb_buffer_test_req_t *req;
    zb_uindex_t i;

    TRACE_MSG(TRACE_ZDO3, "> zb_tp_buffer_test_request_EP param %hd, len %hd", (FMT__H_H, param, req_param->len));
    req = zb_buf_initial_alloc(param, sizeof(zb_buffer_test_req_t) + (req_param->len - 1) * sizeof(zb_uint8_t));
    req->len = req_param->len;
    for (i = 0; i < req_param->len; ++i)
    {
        req->req_data[i] = i;
    }

    ZG->zdo.test_prof_ctx.zb_tp_buffer_test_request.user_cb = cb;
    TRACE_MSG(TRACE_ZDO3, "zb_tp_buffer_test_request_EP, timeout %d", (FMT__D, 30 * ZB_TIME_ONE_SECOND));
    /* use callback to determine that response is not received */
    ZB_SCHEDULE_ALARM(buffer_test_req_timeout, 0, 30 * ZB_TIME_ONE_SECOND);

    TRACE_MSG(TRACE_ZDO3, "request_EP src ep %hd, dst ep %hd", (FMT__H_H, req_param->src_ep, req_param->dst_ep));
    tp_send_req_by_EP(TP_BUFFER_TEST_REQUEST_CLID, param, req_param->src_ep, req_param->dst_ep, 0);

    TRACE_MSG(TRACE_ZDO3, "< zb_tp_buffer_test_request_EP", (FMT__0));
}

static void tp_buffer_test_request_handler(zb_uint8_t param)
{
    zb_buffer_test_response_param_t *test_param;
    zb_apsde_data_indication_t ind;
    zb_uint8_t len = 0;

    TRACE_MSG(TRACE_ZDO3, "tp_buffer_test_request_handler %hd", (FMT__D, param));

    ZB_MEMCPY(&ind, ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t), sizeof(ind));

    /* Get payload size, do this in the beginning, later buffer will be overwritten */
    len = *((zb_uint8_t *)(zb_buf_begin(param)));

    test_param = ZB_BUF_GET_PARAM(param, zb_buffer_test_response_param_t);
    test_param->src_ep = ind.dst_endpoint;
    test_param->dst_ep = ind.src_endpoint;
    test_param->dst_addr = ind.src_addr;
    test_param->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
    test_param->len = len;
    test_param->status = ZB_TP_BUFFER_TEST_OK;

    TRACE_MSG(TRACE_ZDO3, "src addr 0x%x, dst addr 0x%x src ept %hd, dst ept %hd payload len = %hd",
              (FMT__H_H_H_H_H, ind.src_addr, ind.dst_addr, ind.src_endpoint, ind.dst_endpoint, len));


    /* When send request to the group which includes us (test aps/bv-16-i), we
     * pass here our own packet. Do not talk to myself!  */
    if (test_param->dst_addr == ZB_PIBCACHE_NETWORK_ADDRESS())
    {
        TRACE_MSG(TRACE_ZDO1, "do not andwer to myself", (FMT__0));
        zb_buf_free(param);
    }
    else
    {
#ifdef USE_RESP_FOR_COUNTED_BUFS
        TRACE_MSG(TRACE_ZDO3, "tp_buffer_test_request_handler: cluster = 0x%x;", (FMT__H, ind.clusterid));
        if (ind.clusterid == TP_TRANSMIT_COUNTED_PACKETS_REQ_CLID)
        {
            /* Note: according to the Test Profile 2,
               6.2    Transmit counted packets, CID=0x0001,
               we must not answer on counted_packets request! */
            zb_buf_free(param);
            counted_packets_counter++;
            TRACE_MSG(TRACE_ZDO3, "counted packets %d", (FMT__D, counted_packets_counter));
        }
        else
#endif
        {
            tp_buffer_test_response(param, TP_BUFFER_TEST_RESPONSE_CLID);
        }
    }
}


void tp_buffer_test_response(zb_uint8_t param, zb_uint16_t cluster)
{
    zb_buffer_test_response_t *test_resp;
    zb_buffer_test_response_param_t *test_param;
    zb_uint8_t *aps_body;
    zb_uindex_t i;
    zb_uint8_t send_len;

    TRACE_MSG(TRACE_ZDO3, "tp_buffer_test_response %x", (FMT__D, param));
    test_param = ZB_BUF_GET_PARAM(param, zb_buffer_test_response_param_t);
    send_len = (test_param->status == ZB_TP_BUFFER_TEST_OK) ? test_param->len : 0;
    TRACE_MSG(TRACE_ZDO3, "len %hd, status %hd, send_len %hd", (FMT__H_H_H, test_param->len, test_param->status, send_len));
    test_resp = zb_buf_initial_alloc(param, sizeof(zb_buffer_test_response_t) + send_len);
    TRACE_MSG(TRACE_ZDO3, "src ept resp %hd, dst ept resp%hd", (FMT__H_H, test_param->src_ep, test_param->dst_ep));
    test_resp->len = test_param->len;
    test_resp->status = test_param->status;
    if (test_param->status == ZB_TP_BUFFER_TEST_OK)
    {
        aps_body = (zb_uint8_t *)(test_resp + 1);
        for (i = 0; i < test_param->len; i++)
        {
            *aps_body = i;
            aps_body++;
        }
    }

    TRACE_MSG(TRACE_ATM1, "Z< send buffer test response, dst_addr = 0x%04x", (FMT__D, test_param->dst_addr));
    tp_send_req_by_short(cluster, param, ZB_TEST_PROFILE_ID, test_param->dst_addr, test_param->addr_mode, test_param->src_ep, test_param->dst_ep,
                         ZB_APSDE_TX_OPT_ACK_TX, MAX_NWK_RADIUS);
}

static void tp_buffer_test_response_handler(zb_uint8_t param)
{
    zb_buffer_test_response_t *test_resp;

    TRACE_MSG(TRACE_ZDO2, "tp_buffer_test_response_handler %x", (FMT__D, param));

    test_resp = (zb_buffer_test_response_t *)zb_buf_begin(param);
    /* response on test profile 2 buffer request at payload beginig exists length and status fields */
    if (zb_buf_len(param) >= sizeof(zb_buffer_test_response_t))
    {
        TRACE_MSG(TRACE_ZDO2, "tp: status %hd, len %hd", (FMT__H_H, test_resp->status, test_resp->len));
        ZB_SCHEDULE_ALARM_CANCEL(buffer_test_req_timeout, 0);
        if (ZG->zdo.test_prof_ctx.zb_tp_buffer_test_request.user_cb)
        {
            ZB_SCHEDULE_CALLBACK(ZG->zdo.test_prof_ctx.zb_tp_buffer_test_request.user_cb, test_resp->status);
        }
        ZG->zdo.test_prof_ctx.zb_tp_buffer_test_request.user_cb = NULL;
    }
    else
    {
        TRACE_MSG(TRACE_ZDO2, "tp: status %hd", (FMT__H, test_resp->status));
    }
    zb_buf_free(param);
}

#endif /*#ifdef TEST_BLE_STRESS*/
void tp_packet_ack(zb_uint8_t param)
{
    zb_aps_hdr_t aps_hdr;
    zb_buffer_test_response_param_t *test_param;
    zb_buffer_test_response_t *test_resp;

    TRACE_MSG(TRACE_APS3, ">>tp_packet_ack: param %hd", (FMT__H, param));

    zb_aps_hdr_parse(param, &aps_hdr, ZB_FALSE);
    ZB_APS_HDR_CUT(param);

#ifdef USE_RESP_FOR_COUNTED_BUFS
    if (aps_hdr.clusterid == TP_TRANSMIT_COUNTED_PACKETS_RES_CLID)
    {
        if (zb_buf_get_status(param) == 0)
        {
            TRACE_MSG(TRACE_APS3, "buffer test ack ok", (FMT__0));
            zb_buf_free(param);
        }
        else
        {
            TRACE_MSG(TRACE_APS3, "buffer test ack failed", (FMT__0));

            test_resp = (zb_buffer_test_response_t *)zb_buf_begin(param);
            test_param = ZB_BUF_GET_PARAM(param, zb_buffer_test_response_param_t);
            test_param->len = test_resp->len;
            test_param->status = ZB_TP_BUFFER_TEST_FAIL;
            test_param->dst_addr = aps_hdr.src_addr;
            test_param->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
            tp_buffer_test_response(param, TP_TRANSMIT_COUNTED_PACKETS_RES_CLID);
        }
    }
    else
#endif
        if (aps_hdr.clusterid == TP_BUFFER_TEST_RESPONSE_CLID)
        {
            if (zb_buf_get_status(param) == 0)
            {
                TRACE_MSG(TRACE_APS3, "buffer test ack ok", (FMT__0));
                zb_buf_free(param);
            }
            else
            {
                TRACE_MSG(TRACE_APS3, "buffer test ack failed", (FMT__0));

                test_resp = (zb_buffer_test_response_t *)zb_buf_begin(param);
                test_param = ZB_BUF_GET_PARAM(param, zb_buffer_test_response_param_t);
                test_param->len = test_resp->len;
                test_param->status = ZB_TP_BUFFER_TEST_FAIL;
                test_param->dst_addr = aps_hdr.src_addr;
                test_param->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
                tp_buffer_test_response(param, TP_BUFFER_TEST_RESPONSE_CLID);
            }
        }
        else
        {
            TRACE_MSG(TRACE_APS3, "unknown cluster id %x", (FMT__D, aps_hdr.clusterid));
            zb_buf_free(param);
        }
    TRACE_MSG(TRACE_APS3, "<< tp_packet_ack", (FMT__0));
}



#ifdef USE_COUNTER_RES_REQ
static void tp_buffer_test_reset_counter()
{
    TRACE_MSG(TRACE_APS3, ">>tp_buffer_test_reset_counter", (FMT__0));
    counted_packets_counter = 0;
    TRACE_MSG(TRACE_APS3, ">>tp_buffer_test_reset_counter: counter = 0;", (FMT__0));
}

static void tp_buffer_test_counter_response(zb_uint8_t param)
{
    zb_apsde_data_indication_t *ind = (zb_apsde_data_indication_t *)ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);
    zb_uint16_t *test_resp = NULL;

    TRACE_MSG(TRACE_ZDO3, "tp_buffer_test_counter_response %hd", (FMT__D, param));

    /*! [tp_send_req_by_short] */
    test_resp = zb_buf_initial_alloc(param, sizeof(zb_uint16_t));
    *test_resp = counted_packets_counter;
    tp_send_req_by_short(TP_PACKET_COUNT_RESPONSE_CLID,     /* cluster id*/
                         param,                             /* buf ind */
                         ZB_TEST_PROFILE_ID,
                         ind->src_addr,                     /* dst addr */
                         ZB_APS_ADDR_MODE_16_ENDP_PRESENT,  /* addr mode */
                         ind->dst_endpoint,                 /* src ep */
                         ind->src_endpoint,                 /* dst ep */
                         ZB_APSDE_TX_OPT_ACK_TX,            /* rx opt */
                         MAX_NWK_RADIUS);
    /*! [tp_send_req_by_short] */
}

void zb_tp_retrive_packet_count(zb_uint8_t param)
{
    zb_tp_transmit_counted_packets_param_t *params = ZB_BUF_GET_PARAM(param, zb_tp_transmit_counted_packets_param_t);
    TRACE_MSG(TRACE_ZDO3, ">>zb_tp_retrive_packet_count", (FMT__0));
    tp_send_req_by_short(TP_RETRIEVE_PACKET_COUNT_CLID,
                         param,
                         ZB_TEST_PROFILE_ID,
                         params->dst_addr,
                         ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                         params->src_ep,
                         params->dst_ep,
                         /* ZB_APSDE_TX_OPT_ACK_TX */ 0,
                         MAX_NWK_RADIUS);
    TRACE_MSG(TRACE_ZDO3, "<<zb_tp_retrive_packet_count", (FMT__0));
}

#endif

void zb_tp_device_announce(zb_uint8_t param)
{
    zb_bool_t secure = ZB_FALSE;

    TRACE_MSG(TRACE_ZDO3, ">>zb_tp_device_announce", (FMT__0));
#ifdef ZB_SECURE
    secure = (zb_bool_t)(nldereq->security_enable
                         && ZG->aps.authenticated && ZB_NIB().secure_all_frames
                         && ZB_NIB_SECURITY_LEVEL());
#endif
    nwk_alloc_and_fill_hdr(param,
                           ZB_PIBCACHE_NETWORK_ADDRESS(),
                           ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE,
                           ZB_FALSE,
                           secure,
                           ZB_TRUE, ZB_FALSE);
    TRACE_MSG(TRACE_ZDO3, "scheduling device_annce %hd", (FMT__H, param));
    ZB_SCHEDULE_CALLBACK(zdo_send_device_annce, param);
    TRACE_MSG(TRACE_ZDO3, "<<zb_tp_device_announce", (FMT__0));
}

#if defined ZB_PRO_STACK && defined ZB_TEST_CUSTOM_LINK_STATUS && defined ZB_ROUTER_ROLE
zb_bool_t zb_tp_send_link_status_command(zb_uint8_t param, zb_uint16_t short_addr)
{
    zb_nwk_hdr_t *nwhdr = NULL;
    zb_address_ieee_ref_t ref_p;
    zb_uint8_t count = 0;
    zb_uint8_t max_count;
    zb_uint8_t *dt;
    zb_uint8_t *status_cmd;
    zb_bool_t secure = ZB_FALSE;
    zb_uint16_t its_short_addr = ZB_PIBCACHE_NETWORK_ADDRESS();


    TRACE_MSG(TRACE_NWK1, ">> zb_tp_send_link_status_command param %hd",
              (FMT__H, param));

    if ( (zb_nwk_neighbor_table_size() > 0)
            && (zb_address_by_sorted_table_index(ZG->nwk.handle.send_link_status_index, &ref_p) == RET_OK)
       )
    {
        secure = (zb_bool_t)(ZB_NIB_SECURITY_LEVEL());
        ZB_PIBCACHE_NETWORK_ADDRESS() = short_addr;
        nwhdr = nwk_alloc_and_fill_hdr(param,
                                       short_addr,
                                       ZB_NWK_BROADCAST_ROUTER_COORDINATOR,
                                       ZB_FALSE,
                                       secure,
                                       ZB_TRUE, ZB_FALSE);
        ZB_PIBCACHE_NETWORK_ADDRESS() = its_short_addr;
        nwhdr->radius = 1;

        status_cmd = (zb_uint8_t *)nwk_alloc_and_fill_cmd(param, ZB_NWK_CMD_LINK_STATUS, sizeof(zb_uint8_t));
        *status_cmd = 0;
        //max_count = ((MAX_PHY_FRM_SIZE - (zb_buf_len(buf)+ZB_BUF_OFFSET(buf))) / ZB_LINK_STATUS_SIZE);
        max_count = (MAX_PHY_FRM_SIZE - zb_buf_len(param)) / ZB_LINK_STATUS_SIZE;

        dt = zb_buf_alloc_right(param, max_count * ZB_LINK_STATUS_SIZE);

        count = zb_nwk_prepare_link_status_command(dt, max_count);

        TRACE_MSG(TRACE_NWK1, "max_count %d count %d", (FMT__H_H, max_count, count));
    }

    if (count > 0)
    {
        ZB_NWK_LS_SET_COUNT(status_cmd,  count);
        (void)zb_buf_cut_right(param, (max_count - count)*ZB_LINK_STATUS_SIZE);
        ZB_NWK_LS_SET_FIRST_FRAME( status_cmd, 1);
        ZB_NWK_LS_SET_LAST_FRAME( status_cmd, 1);

        /* *ZB_BUF_GET_PARAM(param, zb_uint8_t) */
        ZB_BUF_GET_PARAM(param, zb_apsde_data_ind_params_t)->handle = ZB_NWK_INTERNAL_NSDU_HANDLE;
        ZB_SCHEDULE_CALLBACK(zb_nwk_forward, param);
    }
    else
    {
        TRACE_MSG(TRACE_ZDO1, "zb_tp_send_link_status_command: unable to send command try again later", (FMT__0));
        return ZB_FALSE;
    }
    TRACE_MSG(TRACE_ZDO1, "<< zb_tp_send_link_status_command", (FMT__0));
    return ZB_TRUE;
}

#endif


#endif /* ZB_TEST_PROFILE */

/*! @} */
/*! @endcond */
