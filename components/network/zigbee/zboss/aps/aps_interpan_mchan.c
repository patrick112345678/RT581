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
/* PURPOSE: APS multichannel interpan
*/

#define ZB_TRACE_FILE_ID 2143
#include "zb_common.h"
#include "zb_types.h"
#include "zb_aps_interpan.h"
#include "zb_debug.h"
#include "zb_aps.h"
#if defined ZB_ENABLE_INTER_PAN_EXCHANGE

#ifdef ZB_ENABLE_INTER_PAN_NON_DEFAULT_CHANNEL

void zb_nwk_forward(zb_uint8_t param);
void nwk_call_nlde_data_confirm(zb_bufid_t param, zb_nwk_status_t status, zb_bool_t has_nwk_hdr);
static void zb_intrp_data_request_with_chan_change_invoke_user_callback(void);
static void zb_intrp_data_request_with_chan_change_continue(zb_bufid_t buffer);

/* Declare ring buffers for storing queued packets */
ZB_RING_BUFFER_DECLARE(zb_aps_lock_queue, zb_bufid_t, APS_LOCK_QUEUE_SIZE);
ZB_RING_BUFFER_DECLARE(zb_nwk_lock_queue, zb_bufid_t, NWK_INTRP_LOCK_QUEUE_SIZE);

typedef struct zb_aps_intrp_non_default_chan_s
{
    zb_bool_t intrp_busy;
    zb_bufid_t packet_buffer;
    zb_bufid_t worker_buffer;
    zb_uint8_t next_channel;
    zb_uint8_t default_page;
    zb_uint8_t default_channel;
    zb_channel_page_t channel_page_mask;
    zb_channel_page_t channel_status_page_mask;
    zb_uint32_t chan_wait;
    zb_callback_t cb;
    zb_aps_lock_queue_t lock_queue;
} zb_aps_intrp_non_default_chan_t;

typedef struct zb_nwk_intrp_non_default_chan_s
{
    zb_bool_t intrp_busy;
    zb_nwk_lock_queue_t lock_queue;
} zb_nwk_interp_non_default_chan_t;

/* Structure with pointers to interpan multichannel functions */
zb_aps_inter_pan_mchan_func_selector_t interpan_mchan_selector;

static zb_aps_intrp_non_default_chan_t aps_intrp_non_default_chan;
static zb_nwk_interp_non_default_chan_t nwk_intrp_non_default_chan;

/** @brief Function for scheduling calls to zb_apsde_data_request
  *        to send all packets that have been moved to a queue
  *        because of ongoing INTER-PAN communication.
  * @param param ignored
  */
static void zb_aps_schedule_data_requests_from_queue(zb_uint8_t param)
{
    ZVUNUSED(param);

    /* Clear intrp_busy flag */
    aps_intrp_non_default_chan.intrp_busy = ZB_FALSE;

    /* If lock queue not empty, get buffer and schedule sending the packet */
    if (!ZB_RING_BUFFER_IS_EMPTY(&aps_intrp_non_default_chan.lock_queue))
    {
        zb_bufid_t *lock_queue_entry = ZB_RING_BUFFER_GET(&aps_intrp_non_default_chan.lock_queue);
        ZB_RING_BUFFER_FLUSH_GET(&aps_intrp_non_default_chan.lock_queue);

        ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, *lock_queue_entry);
        /* Reschedule itself if lock queue is not yet empty */
        if (!ZB_RING_BUFFER_IS_EMPTY(&aps_intrp_non_default_chan.lock_queue))
        {
            ZB_SCHEDULE_CALLBACK(zb_aps_schedule_data_requests_from_queue, 0);
            return;
        }
    }

    /* At this point APS lock queue is empty, invoke user callback and return */
    zb_intrp_data_request_with_chan_change_invoke_user_callback();
    return;
}

/* Puts packet to lock queue if inter-pan at non default channel procedure is in progress.
 * If packet processing is delayed, packet is put to queue or dropped, returns RET_OK.
 * If packet processing should continue, returns RET_ERROR. */
static zb_ret_t zb_aps_put_packet_to_queue(zb_uint8_t param)
{
    if (aps_intrp_non_default_chan.intrp_busy == ZB_FALSE)
    {
        return RET_ERROR;
    }

    if (!ZB_RING_BUFFER_IS_FULL(&aps_intrp_non_default_chan.lock_queue))
    {
        ZB_RING_BUFFER_PUT(&aps_intrp_non_default_chan.lock_queue, param);
    }
    else
    {
        aps_send_fail_confirm(param, ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_INVALID_PARAMETER));
    }
    return RET_OK;
}

/* Function to schedule queued buffers, param is unused */
/** @brief Function for scheduling calls to zb_nwk_forward
  *        to send all packets that have been moved to a queue
  *        because of ongoing INTER-PAN communication.
  * @param param ignored
  */
static void zb_nwk_schedule_data_requests_from_queue(zb_uint8_t param)
{
    ZVUNUSED(param);

    /* Clear intrp_busy flag */
    nwk_intrp_non_default_chan.intrp_busy = ZB_FALSE;

    /* If entry was found, schedule sending the packet */
    if (!ZB_RING_BUFFER_IS_EMPTY(&nwk_intrp_non_default_chan.lock_queue))
    {
        zb_bufid_t *lock_queue_entry = ZB_RING_BUFFER_GET(&nwk_intrp_non_default_chan.lock_queue);
        ZB_RING_BUFFER_FLUSH_GET(&nwk_intrp_non_default_chan.lock_queue);

        ZB_SCHEDULE_CALLBACK(zb_nwk_forward, *lock_queue_entry);
        /* Reschedule itself if lock queue is not yet empty */
        if (!ZB_RING_BUFFER_IS_EMPTY(&nwk_intrp_non_default_chan.lock_queue))
        {
            ZB_SCHEDULE_CALLBACK(zb_nwk_schedule_data_requests_from_queue, 0);
            return;
        }
    }

    /* At this point NWK lock queue is empty, schedule queued aps data requests and return */
    zb_aps_schedule_data_requests_from_queue(0);
    return;
}

/* Puts packet to lock queue if inter-pan at non default channel procedure is in progress.
 * If packet processing is delayed, packet is put to queue or dropped, returns RET_OK.
 * If packet processing should continue, returns RET_ERROR. */
static zb_ret_t zb_nwk_put_packet_to_queue(zb_uint8_t param)
{
    if (nwk_intrp_non_default_chan.intrp_busy == ZB_FALSE)
    {
        return RET_ERROR;
    }

    if (!ZB_RING_BUFFER_IS_FULL(&nwk_intrp_non_default_chan.lock_queue))
    {
        ZB_RING_BUFFER_PUT(&nwk_intrp_non_default_chan.lock_queue, param);
    }
    else
    {
        /* No free slot in lock_queue, inform upper layer that sending this packet has failed */
        nwk_call_nlde_data_confirm(param, ZB_NWK_STATUS_INVALID_REQUEST, ZB_TRUE);
    }
    return RET_OK;
}

/* Function to send inter pan data request at multiple channels by temporarily switching channels */
static zb_ret_t zb_intrp_data_request_with_chan_change_func(zb_bufid_t buffer, zb_channel_page_t channel_page_mask, zb_uint32_t chan_wait_ms, zb_callback_t cb)
{
    if (buffer == 0U)
    {
        return RET_INVALID_PARAMETER;
    }
    if ((aps_intrp_non_default_chan.intrp_busy == ZB_TRUE)
            || (nwk_intrp_non_default_chan.intrp_busy == ZB_TRUE)
            || (!ZB_RING_BUFFER_IS_EMPTY(&aps_intrp_non_default_chan.lock_queue))
            || (!ZB_RING_BUFFER_IS_EMPTY(&nwk_intrp_non_default_chan.lock_queue)))
    {
        zb_buf_free(buffer);
        return RET_BUSY;
    }

    ZB_MEMSET(&aps_intrp_non_default_chan, 0, sizeof(aps_intrp_non_default_chan));
    ZB_MEMSET(&nwk_intrp_non_default_chan, 0, sizeof(nwk_intrp_non_default_chan));

    ZB_SCHEDULE_CALLBACK(zb_intrp_data_request_with_chan_change_continue, 0);

    aps_intrp_non_default_chan.intrp_busy = ZB_TRUE;
    nwk_intrp_non_default_chan.intrp_busy = ZB_TRUE;

    aps_intrp_non_default_chan.channel_page_mask = channel_page_mask;
    aps_intrp_non_default_chan.chan_wait = ZB_MILLISECONDS_TO_BEACON_INTERVAL(chan_wait_ms);
    ZB_CHANNEL_PAGE_SET_PAGE(aps_intrp_non_default_chan.channel_status_page_mask,
                             ZB_CHANNEL_PAGE_GET_PAGE(aps_intrp_non_default_chan.channel_page_mask));
    aps_intrp_non_default_chan.default_page = ZB_PIBCACHE_CURRENT_PAGE();
    aps_intrp_non_default_chan.default_channel = ZB_PIBCACHE_CURRENT_CHANNEL();
    aps_intrp_non_default_chan.packet_buffer = buffer;
    aps_intrp_non_default_chan.next_channel = 0;
    aps_intrp_non_default_chan.cb = cb;

    return RET_OK;
}

static void zb_intrp_data_request_with_chan_change_invoke_user_callback(void)
{
    /* Call user register callback to inform the app that procedure is done */
    if (aps_intrp_non_default_chan.cb != NULL)
    {
        /* Look for channels at which packet was successfully sent, if no channel found, set RET_ERROR, otherwise set RET_OK */
        zb_ret_t ret = (ZB_CHANNEL_PAGE_GET_MASK(aps_intrp_non_default_chan.channel_status_page_mask)) ? RET_OK : RET_ERROR;
        zb_uint8_t asdu_handle = ZB_BUF_GET_PARAM(aps_intrp_non_default_chan.packet_buffer, zb_intrp_data_req_t)->asdu_handle;
        zb_mchan_intrp_data_confirm_t *data_ptr = ZB_BUF_GET_PARAM(aps_intrp_non_default_chan.packet_buffer, zb_mchan_intrp_data_confirm_t);

        data_ptr->channel_status_page_mask = aps_intrp_non_default_chan.channel_status_page_mask;
        data_ptr->asdu_handle = asdu_handle;

        zb_buf_set_status(aps_intrp_non_default_chan.packet_buffer, ret);
        (*aps_intrp_non_default_chan.cb)(aps_intrp_non_default_chan.packet_buffer);
    }
    else
    {
        zb_buf_free(aps_intrp_non_default_chan.packet_buffer);
    }
}

static void zb_schedule_data_requests_from_queue(zb_bufid_t buffer)
{
    /* Free received buffer as it is no longer used */
    if (buffer != 0U)
    {
        zb_buf_free(buffer);
    }

    /* Worker buffer is freed, set it to 0 so we don't process others inter-pan data confirms */
    aps_intrp_non_default_chan.worker_buffer = 0;

    /* Unlock NWK queue at first and start scheduling queued buffers.
     * Once NWK queue is empty, APS queue will be unlocked
     * and buffers from APS queue will be scheduled */
    zb_nwk_schedule_data_requests_from_queue(0);
}

/* Return to default channel. After the channel is changed, schedule queued packets. */
static void zb_return_to_default_channel(zb_bufid_t buffer)
{
    if (ZB_PIBCACHE_CURRENT_CHANNEL() != aps_intrp_non_default_chan.default_channel)
    {
        /* Restore default channel in ZB_PIBCACHE and change it in the MAC PIB */
        ZB_PIBCACHE_CURRENT_CHANNEL() = aps_intrp_non_default_chan.default_channel;
        (void)zb_buf_reuse(buffer);
        zb_nwk_pib_set(buffer, ZB_PHY_PIB_CURRENT_CHANNEL,
                       &aps_intrp_non_default_chan.default_channel,
                       sizeof(aps_intrp_non_default_chan.default_channel),
                       zb_schedule_data_requests_from_queue);
    }
    else
    {
        zb_schedule_data_requests_from_queue(buffer);
    }
}

/* Return to default channel page and schedule changing the channel */
static void zb_return_to_default_channel_page(zb_bufid_t buffer)
{
    if (ZB_PIBCACHE_CURRENT_PAGE() != aps_intrp_non_default_chan.default_page)
    {
        /* Restore default channel page in ZB_PIBCACHE and change it in the MAC PIB */
        ZB_PIBCACHE_CURRENT_PAGE() = aps_intrp_non_default_chan.default_page;
        (void)zb_buf_reuse(buffer);
        zb_nwk_pib_set(buffer, ZB_PHY_PIB_CURRENT_PAGE,
                       &aps_intrp_non_default_chan.default_page,
                       sizeof(aps_intrp_non_default_chan.default_page),
                       zb_return_to_default_channel);
    }
    else
    {
        zb_return_to_default_channel(buffer);
    }
}

/* Function called when confirmation for inter-apn packet is received.
 * Returns RET_OK if function will process packet further,
 * returns RET_ERROR otherwise */
static zb_ret_t zb_intrp_data_request_with_chan_change_confirmed(zb_bufid_t buffer)
{
    /* Check if confirmation was received for packet sent with worker buffer.
     * If not, return immediately with RET_ERROR */
    if (aps_intrp_non_default_chan.worker_buffer != buffer)
    {
        return RET_ERROR;
    }
    /* If frame sent with status OK, mark channel as success and wait for required time
     * to receive any responses to the packet.
     * If failed, continue immediately */
    if (zb_buf_get_status(buffer) == RET_OK)
    {
        aps_intrp_non_default_chan.channel_status_page_mask |= (1 << aps_intrp_non_default_chan.next_channel);
        ZB_SCHEDULE_ALARM(zb_intrp_data_request_with_chan_change_continue,
                          buffer,
                          aps_intrp_non_default_chan.chan_wait);
    }
    else
    {
        ZB_SCHEDULE_CALLBACK(zb_intrp_data_request_with_chan_change_continue, buffer);
    }
    return RET_OK;
}

/* Function to prepare and schedule packet. Once packet is sent,
 * zb_intrp_data_request_with_chan_change_confirmed() is called to continue the procedure */
static void zb_intrp_data_request_with_chan_change_send(zb_bufid_t buffer)
{
    /* Prepare the packet to be sent */
    zb_buf_copy(buffer, aps_intrp_non_default_chan.packet_buffer);

    /* Schedule sending the frame */
    ZB_SCHEDULE_CALLBACK(zb_intrp_data_request, buffer);
}

/* Function to set next inter-pan channel to sent packet at */
static void zb_set_next_intrp_channel(zb_bufid_t buffer)
{
    if (ZB_PIBCACHE_CURRENT_CHANNEL() != aps_intrp_non_default_chan.next_channel)
    {
        /* Restore default channel in ZB_PIBCACHE and change it in the MAC PIB */
        ZB_PIBCACHE_CURRENT_CHANNEL() = aps_intrp_non_default_chan.next_channel;
        (void)zb_buf_reuse(buffer);
        zb_nwk_pib_set(buffer, ZB_PHY_PIB_CURRENT_CHANNEL,
                       &aps_intrp_non_default_chan.next_channel,
                       sizeof(aps_intrp_non_default_chan.next_channel),
                       zb_intrp_data_request_with_chan_change_send);
    }
    else
    {
        zb_intrp_data_request_with_chan_change_send(buffer);
    }
}

/* Function to set next inter-pan channel page */
static void zb_set_next_intrp_channel_page(zb_bufid_t buffer)
{
    zb_uint8_t channel_page_new = ZB_CHANNEL_PAGE_GET_PAGE(aps_intrp_non_default_chan.channel_page_mask);
    if (ZB_PIBCACHE_CURRENT_PAGE() != channel_page_new)
    {
        /* Set new channel page in ZB_PIBCACHE and change it in the MAC PIB */
        ZB_PIBCACHE_CURRENT_PAGE() = channel_page_new;
        (void)zb_buf_reuse(buffer);
        zb_nwk_pib_set(buffer, ZB_PHY_PIB_CURRENT_PAGE, &channel_page_new, sizeof(channel_page_new), zb_set_next_intrp_channel);
    }
    else
    {
        zb_set_next_intrp_channel(buffer);
    }
}

/* Function to look for next channel to send packet at and send it */
static void zb_intrp_data_request_with_chan_change_continue(zb_bufid_t buffer)
{
    zb_uint8_t channel;
    zb_uint8_t page_start_channel_number;
    zb_uint8_t page_max_channel_number;
    zb_ret_t ret = RET_OK;
    zb_uint32_t channel_mask = ZB_CHANNEL_PAGE_GET_MASK(aps_intrp_non_default_chan.channel_page_mask);

    zb_channel_page_get_start_channel_number(ZB_CHANNEL_PAGE_GET_PAGE(aps_intrp_non_default_chan.channel_page_mask),
            &page_start_channel_number);
    zb_channel_page_get_max_channel_number(ZB_CHANNEL_PAGE_GET_PAGE(aps_intrp_non_default_chan.channel_page_mask),
                                           &page_max_channel_number);

    /* Set channel initially to start channel for given page or previous channel number if is not zero */
    channel = ((aps_intrp_non_default_chan.next_channel)
               ? (aps_intrp_non_default_chan.next_channel)
               : page_start_channel_number);

    if (!buffer)
    {
        ret = zb_buf_get_out_delayed(zb_intrp_data_request_with_chan_change_continue);

        if (ret != RET_OK)
        {
            zb_return_to_default_channel_page(buffer);
            return;
        }
        return;
    }

    /* Store buffer id used for sending inter-pan packets, will be used later to determine
     * if this is our packet that is confirmed when zb_intrp_data_confirm() is called */
    aps_intrp_non_default_chan.worker_buffer = buffer;

    /* Continue with taken buffer, find next channel to send packet at */
    while (!(channel_mask & (1 << channel)) && (channel <= page_max_channel_number))
    {
        channel++;
    }

    /* If channel not found, return to default channel */
    if (channel > page_max_channel_number)
    {
        zb_return_to_default_channel_page(buffer);
        return;
    }
    else
    {
        aps_intrp_non_default_chan.next_channel = channel;
        /* Once the channel is selected, unset it in the channel mask
         * to prevent sending multiple packets at the same channel */
        aps_intrp_non_default_chan.channel_page_mask &= ~(1UL << channel);
    }

    /* Change to next channel page and channel, then send the packet */
    zb_set_next_intrp_channel_page(buffer);
}

/* Register inter-pan indication callback, called when inter-pan packet is received */
static void zb_af_interpan_set_data_indication_func(zb_af_inter_pan_handler_t cb)
{
    TRACE_MSG(TRACE_ZDO1, "set interpan_data_ind cb to %p, was %p", (FMT__P_P, cb, ZDO_CTX().af_inter_pan_data_cb));
    ZDO_CTX().af_inter_pan_data_cb = cb;
}

/* Function to send inter pan data request at multiple channels by temporarily switching channels.
 * Calls by pointer function to send actual frame only after the zboss_enable_interpan_with_chan_change() was called */
zb_ret_t zb_intrp_data_request_with_chan_change(zb_bufid_t buffer, zb_channel_page_t channel_page_mask, zb_uint32_t chan_wait_ms, zb_callback_t cb)
{
    if (ZB_APS_INTRP_MCHAN_FUNC_SELECTOR().intrp_data_req_with_chan_change != NULL)
    {
        return ZB_APS_INTRP_MCHAN_FUNC_SELECTOR().intrp_data_req_with_chan_change(buffer, channel_page_mask, chan_wait_ms, cb);
    }
    else
    {
        return RET_ERROR;
    }
}

/* Register inter-pan indication callback, called when inter-pan packet is received.
 * Calls by pointer function to register interpan indication callback only after
 * the zboss_enable_interpan_with_chan_change() was called */
void zb_af_interpan_set_data_indication(zb_af_inter_pan_handler_t cb)
{
    if (ZB_APS_INTRP_MCHAN_FUNC_SELECTOR().af_interpan_set_data_indication != NULL)
    {
        ZB_APS_INTRP_MCHAN_FUNC_SELECTOR().af_interpan_set_data_indication(cb);
    }
}

/* Enable interpan procedure */
void zboss_enable_interpan_with_chan_change(void)
{
    ZB_APS_INTRP_MCHAN_FUNC_SELECTOR().put_aps_packet_to_queue = zb_aps_put_packet_to_queue;
    ZB_APS_INTRP_MCHAN_FUNC_SELECTOR().put_nwk_packet_to_queue = zb_nwk_put_packet_to_queue;
    ZB_APS_INTRP_MCHAN_FUNC_SELECTOR().intrp_mchan_data_req_confirmed = zb_intrp_data_request_with_chan_change_confirmed;
    ZB_APS_INTRP_MCHAN_FUNC_SELECTOR().intrp_data_req_with_chan_change = zb_intrp_data_request_with_chan_change_func;
    ZB_APS_INTRP_MCHAN_FUNC_SELECTOR().af_interpan_set_data_indication = zb_af_interpan_set_data_indication_func;
}

#endif /* defined ZB_ENABLE_INTER_PAN_NON_DEFAULT_CHANNEL */
#endif /* defined ZB_ENABLE_INTER_PAN_EXCHANGE */
