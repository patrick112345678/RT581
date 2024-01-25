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

#define ZB_TRACE_FILE_ID 40130
#include "zboss_api.h"
#include "zb_wwah_door_lock.h"
#include "zcl/zb_zcl_wwah.h"

static void dl_event_queue_init(void);
static void dl_event_queue_put(zb_uint16_t operation_event_code);
static void dl_door_lock_release_element_from_queue(void);
static dl_door_lock_event_t *dl_door_lock_get_element_from_queue(void);
static void dl_send_notification_direct(zb_bufid_t param, zb_uint16_t operation_event_code);
static void dl_send_event_notification_cb(zb_bufid_t bufid);
static void dl_resend_notification(zb_bufid_t param);
static void dl_try_send_notification(zb_bufid_t param);

void dl_send_notification(zb_bufid_t param, zb_uint16_t operation_event_code)
{
    TRACE_MSG(TRACE_APP1, ">> dl_send_notification %hd, event %hd", (FMT__H_H, param, operation_event_code));

    if (wwah_attr.wwah_app_event_retry_enabled)
    {
        AER_CTX().pending_buf = param;
        dl_event_queue_put(operation_event_code);

        AER_CTX().failed_attempts = 0;
        dl_try_send_notification(param);
    }
    else
    {
        dl_send_notification_direct(param, operation_event_code);
    }
}

static void dl_send_notification_direct(zb_bufid_t param, zb_uint16_t operation_event_code)
{
    /* [zcl_construct_specific_frame_header] */
    zb_uint8_t *ptr = ZB_ZCL_START_PACKET(param);

    ZB_ZCL_CONSTRUCT_SPECIFIC_COMMAND_RES_FRAME_CONTROL(ptr);
    ZB_ZCL_CONSTRUCT_COMMAND_HEADER(ptr, ZB_ZCL_GET_SEQ_NUM(), ZB_ZCL_CMD_DOOR_LOCK_OPERATION_EVENT_NOTIFICATION_ID);
    /* [zcl_construct_specific_frame_header] */
    ZB_ZCL_PACKET_PUT_DATA8(ptr, ZB_ZCL_DOOR_LOCK_OPERATION_EVENT_SOURCE_RF);
    ZB_ZCL_PACKET_PUT_DATA8(ptr, operation_event_code);
    ZB_ZCL_PACKET_PUT_DATA16_VAL(ptr, 1);
    ZB_ZCL_PACKET_PUT_DATA8(ptr, 1);
    ZB_ZCL_PACKET_PUT_DATA32_VAL(ptr, ZB_TIME_ADD(g_dev_ctx.last_obtained_time, ZB_TIME_SUBTRACT(zb_get_utc_time(), g_dev_ctx.obtained_at)));
    ZB_ZCL_FINISH_N_SEND_PACKET_NEW(param, ptr,
                                    g_dev_ctx.door_lock_client_addr,
                                    ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT,
                                    g_dev_ctx.door_lock_client_endpoint,
                                    WWAH_DOOR_LOCK_EP,
                                    ZB_AF_HA_PROFILE_ID,
                                    ZB_ZCL_CLUSTER_ID_DOOR_LOCK,
                                    dl_send_event_notification_cb, ZB_TRUE, ZB_FALSE, 0);

    TRACE_MSG(TRACE_APP1, ">> dl_send_notification_direct", (FMT__0));
}

void dl_enable_event_retry(const zb_zcl_wwah_enable_wwah_app_event_retry_algorithm_t *params)
{
    dl_event_queue_init();

    FIRST_BACKOFF_TIME() = params->first_backoff_time_in_seconds;
    COMMON_RATIO() = params->backoff_sequence_common_ratio;
    MAX_BACKOFF_TIME() = params->max_backoff_time_in_seconds;
    MAX_REDELIVERY_ATTEMPTS() = params->max_re_delivery_attempts;

    dl_write_app_data(0);
}

void dl_disable_event_retry(void)
{
    dl_event_queue_init();
}

static void dl_event_queue_init(void)
{
    TRACE_MSG(TRACE_APP1, ">> dl_event_queue_init", (FMT__0));

    AER_CTX().pending_buf = 0;
    ZB_RING_BUFFER_INIT(&AER_CTX().door_lock_queue);

    TRACE_MSG(TRACE_APP1, ">> dl_event_queue_init", (FMT__0));
}

static void dl_event_queue_put(zb_uint16_t operation_event_code)
{
    dl_door_lock_event_t dl_info;

    TRACE_MSG(TRACE_APP1, ">> dl_event_queue_put", (FMT__0));

    dl_info.operation_event_code = operation_event_code;
    dl_info.timestamp = ZB_TIME_QUARTERECONDS( ZB_TIMER_GET() );

    if (!ZB_RING_BUFFER_IS_FULL(&AER_CTX().door_lock_queue))
    {
        ZB_RING_BUFFER_PUT_PTR(&AER_CTX().door_lock_queue, &dl_info);
    }
    else
    {
        TRACE_MSG(TRACE_APP1, "buffer is full, reuse last", (FMT__0));
        ZB_RING_BUFFER_PUT_REUSE_LAST(&AER_CTX().door_lock_queue, &dl_info);
    }

    TRACE_MSG(TRACE_APP1, "<< dl_event_queue_put", (FMT__0));
}

static void dl_door_lock_release_element_from_queue(void)
{
    dl_door_lock_event_t *buf_element;

    TRACE_MSG(TRACE_APP1, ">> dl_door_lock_release_element_from_queue", (FMT__0));

    buf_element = ZB_RING_BUFFER_PEEK(&AER_CTX().door_lock_queue);
    if (buf_element)
    {
        ZB_RING_BUFFER_FLUSH_GET(&AER_CTX().door_lock_queue);
    }

    TRACE_MSG(TRACE_APP1, "<< dl_door_lock_release_element_from_queue", (FMT__0));
}


/* get item from queue, but not delete it in queue. Queue item will
 * be released when command is successfully sent */
static dl_door_lock_event_t *dl_door_lock_get_element_from_queue(void)
{
    dl_door_lock_event_t *buf_element;

    TRACE_MSG(TRACE_APP1, ">> dl_door_lock_get_element_from_queue", (FMT__0));

    buf_element = ZB_RING_BUFFER_PEEK(&AER_CTX().door_lock_queue);

    TRACE_MSG(TRACE_APP1, "<< dl_door_lock_get_element_from_queue", (FMT__0));
    return buf_element;
}

static void dl_send_event_notification_cb(zb_bufid_t bufid)
{
    zb_zcl_command_send_status_t *cmd_send_status = ZB_BUF_GET_PARAM(bufid, zb_zcl_command_send_status_t);

    TRACE_MSG(TRACE_APP1, ">> dl_send_event_notification_cb %d", (FMT__H, bufid));

    if (!wwah_attr.wwah_app_event_retry_enabled)
    {
        TRACE_MSG(TRACE_APP1, "wwah app event retry is disabled", (FMT__0));
        zb_buf_free(bufid);
        return;
    }

    if (AER_CTX().pending_buf != bufid)
    {
        TRACE_MSG(TRACE_APP1, "a new event occurred while no ack for previous event received", (FMT__0));
        zb_buf_free(bufid);
        return;
    }

    if (cmd_send_status->status == RET_OK)
    {
        TRACE_MSG(TRACE_APP2, "notification sent OK", (FMT__0));

        dl_door_lock_release_element_from_queue();
        AER_CTX().failed_attempts = 0;

        dl_try_send_notification(bufid);
    }
    else
    {
        TRACE_MSG(TRACE_APP2, "notification don't sent", (FMT__0));

        AER_CTX().failed_attempts++;
        if (dl_wwah_app_event_retry_should_discard_event(AER_CTX().failed_attempts))
        {
            TRACE_MSG(TRACE_APP2, "max retry count reached; drop the event", (FMT__0));
            dl_door_lock_release_element_from_queue();
        }

        if (!ZB_RING_BUFFER_IS_EMPTY(&AER_CTX().door_lock_queue))
        {
            zb_uint_t timeout = dl_wwah_app_event_retry_get_next_timeout(AER_CTX().failed_attempts);
            TRACE_MSG(TRACE_APP2, "retry after %d sec.", (FMT__D, timeout));
            ZB_SCHEDULE_APP_ALARM(dl_resend_notification, bufid, ZB_MILLISECONDS_TO_BEACON_INTERVAL(timeout * 1000));
        }
        else
        {
            zb_buf_free(bufid);
        }
    }


    TRACE_MSG(TRACE_APP1, "<< dl_send_event_notification_cb", (FMT__0));
}

static void dl_resend_notification(zb_bufid_t param)
{
    TRACE_MSG(TRACE_APP1, ">> dl_resend_notification %hd", (FMT__H, param));

    /* NOTE: pure ZCL packet is in the buffer */
    ZB_ZCL_CUT_HEADER(param);
    dl_try_send_notification(param);

    TRACE_MSG(TRACE_APP1, "<< izs_resend_notification", (FMT__0));
}

static void dl_try_send_notification(zb_bufid_t param)
{
    dl_door_lock_event_t *dl_info = dl_door_lock_get_element_from_queue();

    TRACE_MSG(TRACE_APP1, ">> dl_try_send_notification", (FMT__0));

    if (dl_info)
    {
        dl_send_notification_direct(param, dl_info->operation_event_code);
    }
    else
    {
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_APP1, "<< dl_try_send_notification", (FMT__0));
}
