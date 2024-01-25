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
/*  PURPOSE: OTA protocol implementation - host side
*/

#define ZB_TRACE_FILE_ID 6669
#include "zb_macsplit_transport.h"


#if defined ZB_MACSPLIT_HOST
#if defined ZB_MACSPLIT_FW_UPGRADE

static zb_ota_protocol_error_t zb_ota_start_response_handler(zb_uint8_t *buf, zb_uint_t msg_len);
static zb_ota_protocol_error_t zb_ota_portion_response_handler(zb_uint8_t *buf, zb_uint_t msg_len);
static zb_ota_protocol_error_t zb_ota_check_fw_integrity_response_handler(zb_uint8_t *buf, zb_uint_t msg_len);

void zb_ota_finished(void);

void zb_ota_handle_packet(zb_uint8_t *buf, zb_uint_t len)
{
    zb_ota_protocol_hdr_t *hdr = (zb_ota_protocol_hdr_t *)buf;
    zb_ota_protocol_msg_type_t type = hdr->msg_type;
    zb_uint8_t msg_len = hdr->msg_len;
    zb_ota_protocol_error_t error;
    zb_ota_protocol_error_t *error_ptr;
    zb_bufid_t bufid;
    zb_uint8_t *ptr = (zb_uint8_t *) (hdr + 1); /* skip OTA header to get OTA payload */

    TRACE_MSG(TRACE_MAC3, ">>zb_ota_handle_packet: len %d", (FMT__D, len));
    switch (type)
    {
    case OTA_START_RESPONSE:
        error = zb_ota_start_response_handler(ptr, msg_len);
        break;
    case OTA_PORTION_RESPONSE:
        error = zb_ota_portion_response_handler(ptr, msg_len);
        break;
    case OTA_CHECK_FW_INTEGRITY_RESPONSE:
        error = zb_ota_check_fw_integrity_response_handler(ptr, msg_len);
        break;
    default:
        TRACE_MSG(TRACE_ERROR, "Unknown OTA message type!", (FMT__0));
        error.error_type = OTA_UNSUPPORTED_MESSAGE_RECEIVED;
        error.is_host = ZB_TRUE;
        break;
    }

    if (error.error_type != OTA_SUCCESS)
    {
        bufid = zb_buf_get(ZB_TRUE, 0);

        if (bufid == 0)
        {
            TRACE_MSG(TRACE_ERROR, "can't allocate buffer for OTA protocol error indication", (FMT__0));
            return;
        }

        error_ptr = (zb_ota_protocol_error_t *)zb_app_signal_pack(bufid,
                    ZB_MACSPLIT_DEVICE_FW_UPGRADE_EVENT,
                    RET_OK,
                    sizeof(zb_ota_protocol_error_t));
        ZB_MEMCPY(error_ptr, &error, sizeof(zb_ota_protocol_error_t));
        ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete, bufid);
    }

    TRACE_MSG(TRACE_MAC3, "<<zb_ota_handle_packet", (FMT__0));
}

zb_ota_protocol_error_type_t zb_ota_start(zb_ota_protocol_gain_cb_t gain_cb, zb_uint32_t len)
{
    zb_bufid_t bufid;
    zb_uint8_t *ptr = NULL;
    zb_ota_protocol_hdr_t *hdr = NULL;
    zb_ota_protocol_msg_start_t *req = NULL;

    TRACE_MSG(TRACE_MAC3, ">>zb_ota_start: image size %d", (FMT__D, len));
    if (OTA_CTX().ota_in_progress)
    {
        TRACE_MSG(TRACE_ERROR, "zb_ota_start error: OTA already in progress", (FMT__0));
        return OTA_ALREADY_IN_PROGRESS;
    }
    bufid = zb_buf_get_out();
    if (bufid == 0)
    {
        TRACE_MSG(TRACE_ERROR, "zb_ota_start error: unable to allocate a zboss buffer", (FMT__0));
        return OTA_UNABLE_TO_ALLOCATE_ZBOSS_BUFFER;
    }

    OTA_CTX().ota_in_progress = ZB_TRUE;

    /* Maybe, some checks for the buffer and the length here? */
    OTA_CTX().gain_cb = gain_cb;
    OTA_CTX().image_size = len;
    OTA_CTX().fw_offset = 0;

    ptr = zb_buf_initial_alloc(bufid, sizeof(zb_ota_protocol_hdr_t) + sizeof(zb_ota_protocol_msg_start_t));
    hdr = (zb_ota_protocol_hdr_t *)ptr;
    hdr->msg_type = OTA_START;
    hdr->msg_len = sizeof(zb_ota_protocol_msg_start_t);

    req = (zb_ota_protocol_msg_start_t *)(hdr + 1);
    req->image_length = len;

    zb_macsplit_send_ota_msg(bufid);

    TRACE_MSG(TRACE_MAC3, "<<zb_ota_start", (FMT__0));
    return OTA_SUCCESS;
}

zb_ota_protocol_error_type_t zb_ota_send_portion(zb_uint32_t offset, zb_uint16_t length)
{
    zb_bufid_t bufid;
    zb_uint16_t portion_size = 0;
    zb_uint8_t *ptr = NULL;
    zb_ota_protocol_hdr_t *msg_hdr = NULL;
    zb_ota_protocol_msg_portion_t *msg_body = NULL;
    zb_uint8_t ota_buf[length];

    TRACE_MSG(TRACE_MAC3, ">>zb_ota_send_portion: offset 0x%x length 0x%x", (FMT__D, offset, length));

    ZB_BZERO(ota_buf, length);

    if (!OTA_CTX().ota_in_progress)
    {
        TRACE_MSG(TRACE_ERROR, "zb_ota_send_portion error: OTA is not started", (FMT__0));
        return OTA_NOT_STARTED;
    }

    if (offset >= OTA_CTX().image_size)
    {
        TRACE_MSG(TRACE_ERROR, "zb_ota_send_portion error: invalid offset %d, firmware size is %d", (FMT__D_D, offset, OTA_CTX().image_size));
        return OTA_INVALID_OFFSET;
    }

    bufid = zb_buf_get_out();
    if (bufid == 0)
    {
        TRACE_MSG(TRACE_ERROR, "zb_ota_send_portion error: unable to allocate a zboss buffer", (FMT__0));
        return OTA_UNABLE_TO_ALLOCATE_ZBOSS_BUFFER;
    }

    if (offset + length >= OTA_CTX().image_size)
    {
        /* if it is the last portion */
        portion_size = OTA_CTX().image_size - offset;
    }
    else
    {
        portion_size = length;
    }

    if (OTA_CTX().gain_cb(ota_buf, offset, &portion_size) != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zb_ota_send_portion error: failed to fetch portion", (FMT__0));
        return OTA_GAIN_PORTION_FAILED;
    }

    ptr = zb_buf_initial_alloc(bufid, portion_size + sizeof(zb_ota_protocol_hdr_t) + sizeof(zb_ota_protocol_msg_portion_t));

    msg_hdr = (zb_ota_protocol_hdr_t *)ptr;
    msg_hdr->msg_type = OTA_PORTION;
    msg_hdr->msg_len = sizeof(zb_ota_protocol_msg_portion_t) + portion_size;

    msg_body = (zb_ota_protocol_msg_portion_t *)(msg_hdr + 1);
    msg_body->offset = offset;

    ptr = (zb_uint8_t *)(msg_body + 1);

    ZB_MEMCPY(ptr, ota_buf, portion_size);

    zb_macsplit_send_ota_msg(bufid);

    TRACE_MSG(TRACE_MAC3, "<<zb_ota_send_portion", (FMT__0));
    return OTA_SUCCESS;
}

zb_ota_protocol_error_type_t zb_ota_send_check_fw_integrity(void)
{
    zb_bufid_t bufid;
    zb_uint8_t *ptr = NULL;
    zb_ota_protocol_hdr_t *hdr = NULL;

    TRACE_MSG(TRACE_MAC3, ">>zb_ota_send_check_fw_integrity", (FMT__0));
    if (!OTA_CTX().ota_in_progress)
    {
        TRACE_MSG(TRACE_ERROR, "zb_ota_send_check_fw_integrity error: OTA is not started", (FMT__0));
        return OTA_NOT_STARTED;
    }

    bufid = zb_buf_get_out();
    if (bufid == 0)
    {
        TRACE_MSG(TRACE_ERROR, "zb_ota_send_check_fw_integrity error: unable to allocate a zboss buffer", (FMT__0));
        return OTA_UNABLE_TO_ALLOCATE_ZBOSS_BUFFER;
    }

    ptr = zb_buf_initial_alloc(bufid, sizeof(zb_ota_protocol_hdr_t) + sizeof(zb_ota_protocol_msg_check_fw_integrity_t));
    hdr = (zb_ota_protocol_hdr_t *)ptr;
    hdr->msg_type = OTA_CHECK_FW_INTEGRITY;
    hdr->msg_len = sizeof(zb_ota_protocol_msg_check_fw_integrity_t);

    zb_macsplit_send_ota_msg(bufid);

    TRACE_MSG(TRACE_MAC3, "<<zb_ota_send_check_fw_integrity", (FMT__0));
    return OTA_SUCCESS;
}

zb_ota_protocol_error_t zb_ota_start_response_handler(zb_uint8_t *buf, zb_uint_t msg_len)
{
    zb_ota_protocol_msg_start_response_t *resp = (zb_ota_protocol_msg_start_response_t *)buf;
    zb_ota_protocol_error_t error;
    error.is_host = ZB_TRUE;
    error.error_type = OTA_SUCCESS;

    ZVUNUSED(msg_len);

    TRACE_MSG(TRACE_MAC3, ">>zb_ota_start_response_handler", (FMT__0));
    switch (resp->status)
    {
    case OTA_SUCCESS:
    {
        error.error_type = zb_ota_send_portion(OTA_CTX().fw_offset, OTA_FW_PORTION_SIZE);
        if (error.error_type != OTA_SUCCESS)
        {
            TRACE_MSG(TRACE_ERROR, "zb_ota_start_response_handler: send_portion failed, error code %d",
                      (FMT__D, error.error_type));
        }
    }
    break;
    default:
        error.is_host = ZB_FALSE;
        error.error_type = resp->status;
        TRACE_MSG(TRACE_ERROR, "zb_ota_start_response_handler: error code %d", (FMT__D, resp->status));
        break;
    }
    TRACE_MSG(TRACE_MAC3, "<<zb_ota_start_response_handler", (FMT__0));
    return error;
}

zb_ota_protocol_error_t zb_ota_portion_response_handler(zb_uint8_t *buf, zb_uint_t msg_len)
{
    zb_ota_protocol_msg_portion_response_t *resp = (zb_ota_protocol_msg_portion_response_t *)buf;
    zb_ota_protocol_error_t error;
    error.is_host = ZB_TRUE;
    error.error_type = OTA_SUCCESS;

    TRACE_MSG(TRACE_MAC3, ">>zb_ota_portion_response_handler", (FMT__0));

    ZVUNUSED(msg_len);

    switch (resp->status)
    {
    case OTA_SUCCESS:
        OTA_CTX().fw_offset += OTA_FW_PORTION_SIZE;
        if (OTA_CTX().fw_offset > OTA_CTX().image_size)
        {
            OTA_CTX().fw_offset = OTA_CTX().image_size;
        }

        if (OTA_CTX().fw_offset == OTA_CTX().image_size)
        {
            error.error_type = zb_ota_send_check_fw_integrity();
            if (error.error_type != OTA_SUCCESS)
            {
                TRACE_MSG(TRACE_ERROR, "zb_ota_portion_response_handler: send_check_fw_integrity failed, error code %d",
                          (FMT__D, error.error_type));
            }
        }
        else
        {
            error.error_type = zb_ota_send_portion(OTA_CTX().fw_offset, OTA_FW_PORTION_SIZE);
            if (error.error_type != OTA_SUCCESS)
            {
                TRACE_MSG(TRACE_ERROR, "zb_ota_portion_response_handler: send_portion failed, error code %d",
                          (FMT__D, error.error_type));
            }
        }
        break;
    default:
        error.error_type = resp->status;
        error.is_host = ZB_FALSE;
        TRACE_MSG(TRACE_ERROR, "zb_ota_portion_response_handler: error code %d", (FMT__D, resp->status));
        break;
    }
    TRACE_MSG(TRACE_MAC3, "<<zb_ota_portion_response_handler", (FMT__0));
    return error;
}

static zb_ota_protocol_error_t zb_ota_check_fw_integrity_response_handler(zb_uint8_t *buf, zb_uint_t msg_len)
{
    zb_ota_protocol_error_t error;
    zb_ota_protocol_msg_check_fw_integrity_response_t *resp =
        (zb_ota_protocol_msg_check_fw_integrity_response_t *)buf;
    error.is_host = ZB_TRUE;
    error.error_type = OTA_SUCCESS;

    TRACE_MSG(TRACE_MAC3, ">>zb_ota_check_fw_integrity_response_handler", (FMT__0));

    ZVUNUSED(msg_len);

    switch (resp->status)
    {
    case OTA_SUCCESS:
        OTA_CTX().ota_in_progress = ZB_FALSE;
        zb_ota_finished();
        break;
    default:
        error.is_host = ZB_FALSE;
        error.error_type = resp->status;
        TRACE_MSG(TRACE_ERROR, "zb_ota_check_fw_integrity_response_handler: error code %d", (FMT__D, resp->status));
        break;
    }
    TRACE_MSG(TRACE_MAC3, "<<zb_ota_check_fw_integrity_response_handler", (FMT__0));
    return error;
}

void zb_ota_finished(void)
{
    zb_bufid_t bufid;
    zb_ota_protocol_error_t *error;

    TRACE_MSG(TRACE_INFO1, "OTA process finished successfully", (FMT__0));

    bufid = zb_buf_get(ZB_TRUE, 0);

    if (bufid == 0)
    {
        TRACE_MSG(TRACE_ERROR, "can't allocate buffer for indication OTA finished", (FMT__0));
        return;
    }

    error = (zb_ota_protocol_error_t *)zb_app_signal_pack(bufid,
            ZB_MACSPLIT_DEVICE_FW_UPGRADE_EVENT,
            RET_OK,
            sizeof(zb_ota_protocol_error_t));
    error->is_host = ZB_TRUE;
    error->error_type = OTA_SUCCESS;
    ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete, bufid);
}

#endif  /* ZB_MACSPLIT_FW_UPGRADE */
#endif
