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
/*  PURPOSE: OTA protocol implementation - device side
*/

#define ZB_TRACE_FILE_ID 6668
#include "zb_macsplit_transport.h"


#if defined ZB_MACSPLIT_DEVICE && defined ZB_MACSPLIT_FW_UPGRADE

static void zb_ota_start_handler(zb_uint8_t *buf, zb_uint_t msg_len);
static void zb_ota_portion_handler(zb_uint8_t *buf, zb_uint_t msg_len);
static void zb_ota_check_fw_integrity_handler(zb_uint8_t *buf, zb_uint_t msg_len);
static zb_ota_protocol_error_type_t zb_ota_send_check_fw_integrity_response(zb_ota_protocol_error_type_t status);
void zb_ota_finished(void);


void zb_ota_handle_packet(zb_uint8_t *buf, zb_uint_t len)
{
    zb_ota_protocol_hdr_t *hdr = (zb_ota_protocol_hdr_t *)buf;
    zb_ota_protocol_msg_type_t type = (zb_ota_protocol_msg_type_t)hdr->msg_type;
    zb_uint_t msg_len = hdr->msg_len;
    zb_uint8_t *ptr = (zb_uint8_t *)(hdr + 1); /* skip OTA header to get OTA payload */

    TRACE_MSG(TRACE_MAC3, ">>zb_ota_handle_packet, size %d", (FMT__D, len));
    switch (type)
    {
    case OTA_START:
        TRACE_MSG(TRACE_MAC1, "OTA started", (FMT__0));
        zb_ota_start_handler(ptr, msg_len);
        break;
    case OTA_PORTION:
        zb_ota_portion_handler(ptr, msg_len);
        break;
    case OTA_CHECK_FW_INTEGRITY:
        TRACE_MSG(TRACE_ERROR, "OTA checking FW integrity", (FMT__0));
        zb_ota_check_fw_integrity_handler(ptr, msg_len);
        break;
    default:
        TRACE_MSG(TRACE_ERROR, "Unknown OTA message type!", (FMT__0));
        break;
    }
    TRACE_MSG(TRACE_MAC3, "<<zb_ota_handle_packet", (FMT__0));
}

static zb_ota_protocol_error_type_t zb_ota_send_start_response(zb_ota_protocol_error_type_t status)
{
    zb_bufid_t bufid;
    zb_uint8_t *ptr = NULL;
    zb_ota_protocol_hdr_t *hdr = NULL;
    zb_ota_protocol_msg_start_response_t *resp = NULL;

    TRACE_MSG(TRACE_MAC3, ">>zb_ota_send_start_response: status 0x%x", (FMT__D, status));
    bufid = zb_buf_get_out();
    if (bufid == 0)
    {
        TRACE_MSG(TRACE_ERROR, "zb_ota_send_start_response error: unable to allocate a zboss buffer", (FMT__0));
        return OTA_UNABLE_TO_ALLOCATE_ZBOSS_BUFFER;
    }

    ptr = zb_buf_initial_alloc(bufid, sizeof(zb_ota_protocol_hdr_t) + sizeof(zb_ota_protocol_msg_start_response_t));

    hdr = (zb_ota_protocol_hdr_t *) ptr;
    hdr->msg_type = OTA_START_RESPONSE;
    hdr->msg_len = sizeof(zb_ota_protocol_msg_start_response_t);

    resp = (zb_ota_protocol_msg_start_response_t *)(hdr + 1);
    resp->status = status;

    zb_macsplit_send_ota_msg(bufid);

    TRACE_MSG(TRACE_MAC3, "<<zb_ota_send_start_response", (FMT__0));
    return OTA_SUCCESS;
}

#ifdef ZB_MACSPLIT_OTA_ONLY_SIGNAL
static void zb_ota_only_signal_delay_handler(zb_uint8_t param)
{
    ZVUNUSED(param);
    bootloader_rebootAndInstall();
}
#endif

static void zb_ota_start_handler(zb_uint8_t *buf, zb_uint_t msg_len)
{
    zb_ota_protocol_msg_start_t *req = NULL;
    zb_uint8_t status_code = OTA_SUCCESS;

    ZVUNUSED(msg_len);
    TRACE_MSG(TRACE_MAC3, ">>zb_ota_start_handler", (FMT__0));
    if (OTA_CTX().ota_in_progress)
    {
        TRACE_MSG(TRACE_ERROR, "zb_ota_start_handler error: OTA is already in progress", (FMT__0));
        status_code = zb_ota_send_start_response(OTA_ALREADY_IN_PROGRESS);
        if (status_code != OTA_SUCCESS)
        {
            TRACE_MSG(TRACE_ERROR, "zb_ota_start_handler error: zb_ota_send_start_response failed with error_code %d", (FMT__D, status_code));
        }
    }
    else
    {
        req = (zb_ota_protocol_msg_start_t *)buf;
        if (!zb_osif_ota_fw_size_ok(req->image_length))
        {
            TRACE_MSG(TRACE_ERROR, "zb_ota_start_handler error: image size is incorrect", (FMT__D, req->image_length));
            status_code = zb_ota_send_start_response(OTA_INVALID_IMAGE_SIZE);
            if (status_code != OTA_SUCCESS)
            {
                TRACE_MSG(TRACE_ERROR, "zb_ota_start_handler error: zb_ota_send_start_response failed with error_code %d", (FMT__D, status_code));
            }
        }

        TRACE_MSG(TRACE_MAC3, "image size: %d", (FMT__D, req->image_length));
#ifdef ZB_MACSPLIT_OTA_ONLY_SIGNAL
        status_code = zb_ota_send_check_fw_integrity_response(OTA_SUCCESS);
        ZB_SCHEDULE_ALARM(zb_ota_only_signal_delay_handler, 0, ZB_TIME_ONE_SECOND);
#else
        /* TODO: check if initialization is correct */
        OTA_CTX().ota_in_progress = ZB_TRUE;
        OTA_CTX().image_size = req->image_length;
        OTA_CTX().dev = zb_osif_ota_open_storage();
        zb_osif_ota_mark_fw_absent();
        zb_osif_ota_erase_fw(OTA_CTX().dev, 0, OTA_CTX().image_size);

        status_code = zb_ota_send_start_response(OTA_SUCCESS);
#endif
        if (status_code != OTA_SUCCESS)
        {
            TRACE_MSG(TRACE_ERROR, "zb_ota_start_handler error: zb_ota_send_start_response failed with error_code %d", (FMT__D, status_code));
        }
    }
    TRACE_MSG(TRACE_MAC3, "<<zb_ota_start_handler", (FMT__0));
}

static zb_ota_protocol_error_type_t zb_ota_send_portion_response(zb_ota_protocol_error_type_t status, zb_uint32_t offset)
{
    zb_bufid_t bufid;
    zb_uint8_t *ptr = NULL;
    zb_ota_protocol_hdr_t *hdr = NULL;
    zb_ota_protocol_msg_portion_response_t *resp = NULL;

    TRACE_MSG(TRACE_MAC3, ">>zb_ota_send_portion_response, status: 0x%x offset: 0x%x", (FMT__D_D, status, offset));
    bufid = zb_buf_get_out();
    if (bufid == 0)
    {
        TRACE_MSG(TRACE_ERROR, "zb_ota_send_portion_response error: unable to allocate a zboss buffer", (FMT__0));
        return OTA_UNABLE_TO_ALLOCATE_ZBOSS_BUFFER;
    }

    ptr = zb_buf_initial_alloc(bufid, sizeof(zb_ota_protocol_hdr_t) + sizeof(zb_ota_protocol_msg_portion_response_t));

    hdr = (zb_ota_protocol_hdr_t *) ptr;
    hdr->msg_type = OTA_PORTION_RESPONSE;
    hdr->msg_len = sizeof(zb_ota_protocol_msg_portion_response_t);

    resp = (zb_ota_protocol_msg_portion_response_t *)(hdr + 1);
    resp->status = status;
    resp->offset = offset;

    zb_macsplit_send_ota_msg(bufid);

    TRACE_MSG(TRACE_MAC3, "<<zb_ota_send_portion_response", (FMT__0));
    return OTA_SUCCESS;
}

static void zb_ota_portion_handler(zb_uint8_t *buf, zb_uint_t msg_len)
{
    zb_ota_protocol_msg_portion_t *req = (zb_ota_protocol_msg_portion_t *)buf;
    zb_uint32_t offset = req->offset;
    zb_uint32_t portion_size = 0;
    zb_uint8_t *data = NULL;
    zb_uint8_t status_code = OTA_SUCCESS;

    TRACE_MSG(TRACE_MAC3, ">>zb_ota_portion_handler offset 0x%x", (FMT__D, offset));
    if (!OTA_CTX().ota_in_progress)
    {
        TRACE_MSG(TRACE_ERROR, "zb_ota_portion_handler error: OTA is not in progress", (FMT__0));
        status_code = zb_ota_send_portion_response(OTA_NOT_STARTED, offset);
        if (status_code != OTA_SUCCESS)
        {
            TRACE_MSG(TRACE_ERROR, "zb_ota_portion_handler error: zb_ota_send_portion_response failed with error_code %d", (FMT__D, status_code));
        }
    }
    else
    {
        portion_size = msg_len - sizeof(zb_ota_protocol_msg_portion_t);
        TRACE_MSG(TRACE_MAC3, "offset: 0x%x, portion_size: 0x%x", (FMT__D_D, offset, portion_size));
        data = (zb_uint8_t *)(req + 1);

        /* TODO: check for errors */
        zb_osif_ota_write(OTA_CTX().dev, data, offset, portion_size, OTA_CTX().image_size);

        status_code = zb_ota_send_portion_response(OTA_SUCCESS, offset);
        if (status_code != OTA_SUCCESS)
        {
            TRACE_MSG(TRACE_ERROR, "zb_ota_portion_handler error: zb_ota_send_portion_response failed with error_code %d", (FMT__D, status_code));
        }
    }
    TRACE_MSG(TRACE_MAC3, "<<zb_ota_portion_handler", (FMT__0));
}

static zb_ota_protocol_error_type_t zb_ota_send_check_fw_integrity_response(zb_ota_protocol_error_type_t status)
{
    zb_bufid_t bufid;
    zb_uint8_t *ptr = NULL;
    zb_ota_protocol_hdr_t *hdr = NULL;
    zb_ota_protocol_msg_start_response_t *resp = NULL;

    TRACE_MSG(TRACE_MAC3, ">>zb_ota_send_check_fw_integrity_response, status: 0x%x", (FMT__D, status));
    bufid = zb_buf_get_out();
    if (bufid == 0)
    {
        TRACE_MSG(TRACE_ERROR, "zb_ota_send_check_fw_integrity_response error: unable to allocate a zboss buffer", (FMT__0));
        return OTA_UNABLE_TO_ALLOCATE_ZBOSS_BUFFER;
    }

    ptr = zb_buf_initial_alloc(bufid, sizeof(zb_ota_protocol_hdr_t) + sizeof(zb_ota_protocol_msg_start_response_t));

    hdr = (zb_ota_protocol_hdr_t *) ptr;
    hdr->msg_type = OTA_CHECK_FW_INTEGRITY_RESPONSE;
    hdr->msg_len = sizeof(zb_ota_protocol_msg_portion_response_t);

    resp = (zb_ota_protocol_msg_start_response_t *)(hdr + 1);
    resp->status = status;

    zb_macsplit_send_ota_msg(bufid);

    TRACE_MSG(TRACE_MAC3, "<<zb_ota_send_check_fw_integrity_response", (FMT__0));
    return OTA_SUCCESS;
}

void zb_osif_ota_verify_integrity_done(zb_uint8_t verification_success)
{
    zb_uint8_t status_code = OTA_SUCCESS;

    TRACE_MSG(TRACE_MAC3, ">>zb_osif_ota_verify_integrity_done", (FMT__0));
    if (verification_success)
    {
        zb_ota_finished();
        status_code = zb_ota_send_check_fw_integrity_response(OTA_SUCCESS);
        if (status_code != OTA_SUCCESS)
        {
            TRACE_MSG(TRACE_ERROR, "zb_ota_check_fw_integrity_handler error: zb_ota_send_check_fw_integrity_response failed with error_code %d", (FMT__D, status_code));
        }
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "integrity check_failed!", (FMT__0));
        status_code = zb_ota_send_check_fw_integrity_response(OTA_INTEGRITY_CHECK_FAILED);
        if (status_code != OTA_SUCCESS)
        {
            TRACE_MSG(TRACE_ERROR, "zb_ota_check_fw_integrity_handler error: zb_ota_send_check_fw_integrity_response failed with error_code %d", (FMT__D, status_code));
        }
    }
    TRACE_MSG(TRACE_MAC3, "<<zb_osif_ota_verify_integrity_done", (FMT__0));
}


static void zb_ota_check_fw_integrity_handler(zb_uint8_t *buf, zb_uint_t msg_len)
{
    /*FIXME: buf is unused?*/
    zb_ota_protocol_msg_check_fw_integrity_t *req = NULL;
    zb_uint8_t status_code = OTA_SUCCESS;

    ZVUNUSED(req);
    ZVUNUSED(buf);
    ZVUNUSED(msg_len);
    TRACE_MSG(TRACE_MAC3, ">>zb_ota_check_fw_integrity_handler", (FMT__0));
    if (!OTA_CTX().ota_in_progress)
    {
        TRACE_MSG(TRACE_ERROR, "zb_ota_check_fw_integrity_handler error: OTA is not in progress", (FMT__0));
        status_code = zb_ota_send_check_fw_integrity_response(OTA_NOT_STARTED);
        if (status_code != OTA_SUCCESS)
        {
            TRACE_MSG(TRACE_ERROR, "zb_ota_check_fw_integrity_handler error: zb_ota_send_check_fw_integrity_response failed with error_code %d", (FMT__D, status_code));
        }
    }
    else
    {
        OTA_CTX().ota_in_progress = ZB_FALSE;
        zb_osif_ota_verify_integrity_async(OTA_CTX().dev, OTA_CTX().image_size);
    }
    TRACE_MSG(TRACE_MAC3, "<<zb_ota_check_fw_integrity_handler", (FMT__0));
}

void zb_ota_finished(void)
{
    TRACE_MSG(TRACE_ERROR, "OTA process finished successfully", (FMT__0));
    zb_osif_ota_mark_fw_ready(OTA_CTX().dev, OTA_CTX().image_size, 0);
}
#endif
