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
/*  PURPOSE: MAC split transport layer implementation
*/

#define ZB_TRACE_FILE_ID 6667
#include "zb_common.h"

#ifdef ZB_MACSPLIT

#include "zb_macsplit_transport.h"
#include "zb_bufpool.h"

/* 12/11/2017 EE CR:MINOR Add prefix to SIGNATURE_FIRST_BYTE */
static zb_uint8_t signature[] = { SIGNATURE_FIRST_BYTE, SIGNATURE_SECOND_BYTE };
#define SIGNATURE signature

zb_macsplit_transport_context_t g_macsplit;

#if !defined ZB_PLATFORM_LINUX
static void zb_macsplit_transport_recv_byte_from_osif(zb_uint8_t byte);
#if !defined ZB_MACSPLIT_HOST && !defined ZB_MACSPLIT_DEVICE_ONETIME_BOOT_INDICATION
static zb_bool_t boot_ind_already_scheduled = ZB_FALSE;
#endif /* !ZB_MACSPLIT_HOST && !ZB_MACSPLIT_DEVICE_ONETIME_BOOT_INDICATION*/
#endif /* !defined ZB_PLATFORM_LINUX */

static void zb_macsplit_transport_recv_byte(zb_uint8_t byte);
static void zb_macsplit_transport_send_data_with_type_internal(void);
static void zb_macsplit_transport_send_packet(zb_bool_t retransmit);
static void zb_macsplit_transport_stop_recv_packet_timer(void);
static void zb_macsplit_transport_stop_wait_for_ack_timer(void);
static void zb_macsplit_reset_tx_queue(void);
static void zb_macsplit_transport_handle_packet(void);
static void zb_macsplit_transport_handle_macsplit_packet(void);
static void zb_macsplit_transport_handle_trace_packet(void);

static void zb_macsplit_transport_stop_wait_for_ack_timer(void);

extern void zb_trace_put_bytes(zb_uint16_t file_id, zb_uint8_t *buf, zb_short_t len);

/* TODO: implement keep-alive mechanism for handling fail of Host or Device
 */

/**
 * Initialization functions
 */
void zb_macsplit_transport_init(void)
{
    TRACE_MSG(TRACE_MAC1, ">zb_macsplit_transport_init", (FMT__0 ));

#ifdef ZB_MACSPLIT_FW_UPGRADE
    zb_ota_protocol_init();
#endif

    zb_macsplit_transport_reinit();

#if !defined ZB_PLATFORM_LINUX

#if defined ZB_MACSPLIT_USE_IO_BUFFERS
    ZB_RING_BUFFER_INIT(&MACSPLIT_CTX().rx_buffer);
#endif /* ZB_MACSPLIT_USE_IO_BUFFERS */

#ifdef ZB_MACSPLIT_TRANSPORT_SERIAL
    zb_osif_serial_transport_init();
#if defined ZB_MACSPLIT_USE_IO_BUFFERS
    ZB_RING_BUFFER_INIT(&MACSPLIT_CTX().tx_buffer);
    zb_osif_set_user_io_buffer((zb_byte_array_t *)&MACSPLIT_CTX().tx_buffer, ZB_MACSPLIT_TRANSPORT_BUFFER_CAPACITY);
#endif
    zb_osif_set_uart_byte_received_cb(zb_macsplit_transport_recv_byte_from_osif);
#endif

#if defined ZB_MACSPLIT_DEVICE && (defined ZB_SERIAL_FOR_TRACE || defined ZB_TRACE_OVER_USART)
    MACSPLIT_CTX().trace_buffer_data = 0;
#endif

#ifdef ZB_MACSPLIT_TRANSPORT_USERIAL
    zb_osif_userial_init();
    zb_osif_set_userial_byte_received_cb(zb_macsplit_transport_recv_byte_from_osif);
#endif

#ifdef ZB_MACSPLIT_TRANSPORT_SPI
    zb_osif_spi_init();
    zb_osif_spi_set_byte_received_cb(zb_macsplit_transport_recv_byte_from_osif);
#endif

#else  /* ZB_PLATFORM_LINUX */
    zb_linux_transport_init();
#endif /* !defined ZB_PLATFORM_LINUX */

    MACSPLIT_CTX().operation_buf = zb_buf_get_out();

#if defined ZB_MACSPLIT_DEVICE
    macsplit_indicate_boot();
#endif

#if defined ZB_MACSPLIT_HOST
#if defined ZB_MACSPLIT_RESET_DEVICE_AT_START
    {
        /* Let's use operation_buf to reset the device */
        zb_mlme_dev_reset(MACSPLIT_CTX().operation_buf);
    }
#else
    zb_macsplit_mlme_mark_radio_reset();
#endif
#endif /* defined ZB_MACSPLIT_HOST */

    TRACE_MSG(TRACE_MAC1, "<zb_macsplit_transport_init", (FMT__0 ));
}

#if defined ZB_MACSPLIT_TRACE_DUMP_TO_FILE
/* Default implementation of macsplit trace file path provider function */
ZB_WEAK_PRE void zb_macsplit_trace_dump_file_path_get(char *file_name) ZB_WEAK;
void zb_macsplit_trace_dump_file_path_get(char *file_name)
{
    ZB_FILE_PATH_GET_WITH_POSTFIX(ZB_FILE_PATH_BASE_RAMFS_TRACE_LOGS,
                                  ZB_TMP_FILE_PATH_PREFIX, "macsplit_device.trace", file_name);
}
#endif /* defined ZB_MACSPLIT_TRACE_DUMP_TO_FILE */

void zb_macsplit_transport_reinit()
{
    TRACE_MSG(TRACE_MAC1, ">zb_macsplit_transport_reinit", (FMT__0));

    /* fill ack packet */
    MACSPLIT_CTX().ack_pkt.flags.is_ack = ZB_TRUE;
    MACSPLIT_CTX().ack_pkt.type         = ZB_MAC_TRANSPORT_TYPE_MAC_SPLIT_DATA;
    MACSPLIT_CTX().ack_pkt.len          = sizeof(MACSPLIT_CTX().ack_pkt);
    MACSPLIT_CTX().last_rx_pkt_number   = 0xFF;
    MACSPLIT_CTX().transport_type = ZB_MACSPLIT_TRANSPORT_TYPE;
    MACSPLIT_CTX().curr_pkt_number = 0;

#if defined ZB_MACSPLIT_TRACE_DUMP_TO_FILE
    {
        char file_name[ZB_MAX_FILE_PATH_SIZE] = {0};

        zb_macsplit_trace_dump_file_path_get(file_name);

        MACSPLIT_CTX().trace_file = zb_osif_file_open(file_name, "wb");

        if (MACSPLIT_CTX().trace_file == NULL)
        {
            TRACE_MSG(TRACE_ERROR, "Unable to open trace dump file", (FMT__0));
            ZB_ASSERT(0);
        }
    }
#endif

    zb_macsplit_transport_stop_recv_packet_timer();
    zb_macsplit_transport_stop_wait_for_ack_timer();

#if defined ZB_MACSPLIT_USE_IO_BUFFERS
    ZB_RING_BUFFER_INIT(&MACSPLIT_CTX().rx_buffer);
#else
    ZB_RING_BUFFER_INIT(&ZB_IOCTX().in_buffer);
#endif  /* ZB_MACSPLIT_USE_IO_BUFFERS */

    zb_macsplit_reset_tx_queue();
    TRACE_MSG(TRACE_MAC1, "<zb_macsplit_transport_reinit", (FMT__0));
}


static zb_bool_t zb_macsplit_transport_is_open(void)
{
#ifdef ZB_PLATFORM_LINUX
    return zb_linux_transport_is_open();
#else

#ifdef ZB_MACSPLIT_TRANSPORT_USERIAL
    return zb_osif_userial_is_open();
#else
    return ZB_TRUE;
#endif /* ZB_MACSPLIT_TRANSPORT_USERIAL */

#endif /* ZB_PLATFORM_LINUX */
}

static zb_uint8_t zb_macsplit_pkt_payload_size(zb_macsplit_packet_t *pkt)
{
    ZB_ASSERT(pkt);

    /* No known data types with payload apart from MAC_SPLIT_DATA for now */
    if (pkt->hdr.type != ZB_MAC_TRANSPORT_TYPE_MAC_SPLIT_DATA
            && pkt->hdr.type != ZB_MAC_TRANSPORT_TYPE_OTA_PROTOCOL)
    {
        return 0;
    }

    return pkt->hdr.len - /* payload is included into length calculation */
           sizeof(pkt->hdr) - /* but without macsplit transport header */
           sizeof(pkt->body.msdu_handle) - /* and without msdu_handle */
           sizeof(pkt->body.call_type) - /* and without call type */
           sizeof(pkt->body.crc); /* and without crc of course */
}

void zb_macsplit_set_cb_recv_data(recv_data_cb_t cb)
{
    MACSPLIT_CTX().recv_data_cb = cb;
}


/**
 * Functions for debug purposes
 */
static void zb_macsplit_transport_dump_transport_hdr(zb_macsplit_packet_t *pkt, zb_uint8_t tx)
{
    ZVUNUSED(pkt);

    if (!pkt->hdr.flags.is_ack)
    {
        TRACE_MSG(TRACE_MAC3, "TRANSPORT HDR tx %hd len %hd msduh %hd type %hd crc %hd packet_number %hd call_type %hd",
                  (FMT__H_H_H_H_H_H_H, tx, pkt->hdr.len, pkt->body.msdu_handle, pkt->hdr.type, pkt->hdr.crc, pkt->hdr.flags.packet_number, pkt->body.call_type));
    }
    else
    {
        TRACE_MSG(TRACE_MAC3, "tx %hd len %hd ACK packet_number %hd should_retransmit %hd", (FMT__H_H_H_H, tx, pkt->hdr.len, pkt->hdr.flags.packet_number, pkt->hdr.flags.should_retransmit));
    }
}


static void zb_macsplit_reset_tx_queue(void)
{
    MACSPLIT_CTX().retransmit_count   = 0;
    MACSPLIT_CTX().is_waiting_for_ack = ZB_FALSE;

    /* flush ring buffer */
    while (!ZB_RING_BUFFER_IS_EMPTY(&MACSPLIT_CTX().tx_queue))
    {
        zb_bufid_t param = *ZB_RING_BUFFER_GET(&MACSPLIT_CTX().tx_queue);
        if (param != 0)
        {
            zb_buf_free(param);
        }
        ZB_RING_BUFFER_FLUSH_GET(&MACSPLIT_CTX().tx_queue);
    }
}

#if !defined ZB_MACSPLIT_HOST && !defined ZB_MACSPLIT_DEVICE_ONETIME_BOOT_INDICATION
static void zb_macsplit_transport_boot_indication_call_timeout(zb_bufid_t param)
{
    ZVUNUSED(param);
    TRACE_MSG(TRACE_ERROR, "zb_macsplit_transport_boot_indication_call_timeout", (FMT__0));
    zb_reset(0);
}
#endif /* !defined ZB_MACSPLIT_HOST && !defined ZB_MACSPLIT_DEVICE_ONETIME_BOOT_INDICATION */

/**
 * Functions for handle exceptions
 */
static void zb_macsplit_transport_wait_for_ack_timeout(zb_bufid_t param)
{
    ZVUNUSED(param);

    if (MACSPLIT_CTX().is_waiting_for_ack)
    {
        if ((MACSPLIT_CTX().retransmit_count < ZB_MACSPLIT_RETRANSMIT_MAX_COUNT)
#ifndef ZB_MACSPLIT_HOST
                && (MACSPLIT_CTX().tx_pkt.body.call_type != ZB_TRANSPORT_CALL_TYPE_DEVICE_BOOT)
#endif  /* !defined ZB_MACSPLIT_HOST */
           )
        {
            TRACE_MSG(TRACE_INFO1, "No ack received, retransmit. Info: packet number %hd, retransmit count %hd ", (FMT__H_H, MACSPLIT_CTX().tx_pkt.hdr.flags.packet_number, MACSPLIT_CTX().retransmit_count));

            MACSPLIT_CTX().retransmit_count++;
            zb_macsplit_transport_send_packet(ZB_TRUE);
        }
        else
        {
#ifndef ZB_MACSPLIT_HOST
            /* At MCU reboot after N tx attempts
             * except BOOT call, try to retransmit it more times */
            if (MACSPLIT_CTX().tx_pkt.body.call_type == ZB_TRANSPORT_CALL_TYPE_DEVICE_BOOT)
            {
                TRACE_MSG(TRACE_INFO1, "No ack received, retransmit. Info: packet number %hd, retransmit count %hd ", (FMT__H_H, MACSPLIT_CTX().tx_pkt.hdr.flags.packet_number, MACSPLIT_CTX().retransmit_count));
#ifndef ZB_MACSPLIT_DEVICE_ONETIME_BOOT_INDICATION
                if (!boot_ind_already_scheduled)
                {
                    ZB_SCHEDULE_ALARM(zb_macsplit_transport_boot_indication_call_timeout,
                                      0, ZB_TIME_ONE_SECOND * ZB_MACSPLIT_BOOT_INDICATION_RETRANSMIT_TIMEOUT);
                    boot_ind_already_scheduled = ZB_TRUE;
                }
                zb_macsplit_transport_send_packet(ZB_TRUE);
#endif /* !defined ZB_MACSPLIT_DEVICE_ONETIME_BOOT_INDICATION */
            }
            else
#endif /* !defined ZB_MACSPLIT_HOST */
            {
                TRACE_MSG(TRACE_ERROR, "MACSPLIT: Can't transmit data - reset", (FMT__0));
                zb_reset(0);
            }
        }
    }
}

static void zb_macsplit_transport_start_wait_for_ack_timer(void)
{
    MACSPLIT_CTX().is_waiting_for_ack = ZB_TRUE;
    ZB_SCHEDULE_ALARM(zb_macsplit_transport_wait_for_ack_timeout, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(ZB_MACSPLIT_RETRANSMIT_TIMEOUT));
}

static void zb_macsplit_transport_stop_wait_for_ack_timer(void)
{
    MACSPLIT_CTX().retransmit_count   = 0;
    MACSPLIT_CTX().is_waiting_for_ack = ZB_FALSE;
    ZB_SCHEDULE_ALARM_CANCEL(zb_macsplit_transport_wait_for_ack_timeout, ZB_ALARM_ANY_PARAM);
#if !defined ZB_MACSPLIT_HOST && !defined ZB_MACSPLIT_DEVICE_ONETIME_BOOT_INDICATION
    if (boot_ind_already_scheduled)
    {
        ZB_SCHEDULE_ALARM_CANCEL(zb_macsplit_transport_boot_indication_call_timeout, ZB_ALARM_ANY_PARAM);
        boot_ind_already_scheduled = ZB_FALSE;
    }
#endif /* !defined ZB_MACSPLIT_HOST && !defined ZB_MACSPLIT_DEVICE_ONETIME_BOOT_INDICATION */
}

static void zb_macsplit_transport_recv_packet_timeout(zb_bufid_t param)
{
    ZVUNUSED(param);

    TRACE_MSG(TRACE_INFO1, "Reset recv transport state", (FMT__0));

    MACSPLIT_CTX().received_bytes  = 0;
    MACSPLIT_CTX().transport_state = RECEIVING_SIGNATURE;

#if defined ZB_MACSPLIT_USE_IO_BUFFERS
    ZB_RING_BUFFER_INIT(&MACSPLIT_CTX().rx_buffer);
#endif
}

static void zb_macsplit_transport_start_recv_packet_timer(void)
{
    ZB_SCHEDULE_ALARM(zb_macsplit_transport_recv_packet_timeout, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(ZB_MACSPLIT_RECV_DATA_TIMEOUT));
}

static void zb_macsplit_transport_stop_recv_packet_timer(void)
{
    MACSPLIT_CTX().received_bytes  = 0;
    MACSPLIT_CTX().transport_state = RECEIVING_SIGNATURE;
    ZB_SCHEDULE_ALARM_CANCEL(zb_macsplit_transport_recv_packet_timeout, ZB_ALARM_ANY_PARAM);
}

#if defined ZB_MACSPLIT_HOST

static zb_bool_t zb_macsplit_transport_needs_confirm_timer(zb_transport_call_type_t call_type, zb_bufid_t param)
{
    zb_bool_t ret = ZB_FALSE;
    switch (call_type)
    {
    case ZB_TRANSPORT_CALL_TYPE_HOST_MCPS_DATA_REQUEST:
        ret = ZB_TRUE;
        break;
    case ZB_TRANSPORT_CALL_TYPE_HOST_MLME_SET_REQUEST:
        ret = ZB_TRUE;
        break;
    case ZB_TRANSPORT_CALL_TYPE_HOST_MLME_GET_REQUEST:
        ret = ZB_TRUE;
        break;
    }

    if (ret == ZB_TRUE)
    {
        /* The issue was found when the Host was losing incoming packets from the radio.
           In case when the Host lost MAC-split ACK packet, Data Confirm packet may be sent
           to the Host BEFORE sending MAC-split ACK for the previous Data request packet.
           This leads to canceling the confirm alarm timer BEFORE it actually started.
           Finally, the host will retransmit this Data Request packet (because it didn't receive the ACK)
           and after receiving the ACK, the host will start the alarm timer, but it won't be canceled because
           this Data Confirm was already processed. The assertion condition will be failed in this case.
           Now we'd like to prevent such crashes to improve the system robustness.
         */
        if (ZB_CHECK_BIT_IN_BIT_VECTOR(MACSPLIT_CTX().specific_ctx.recv_cfm, param))
        {
            TRACE_MSG(TRACE_INFO1, "Warning: confirmation is received before a request ACKed (buf: %hd)", (FMT__H, param));
            ret = ZB_FALSE;
        }
    }

    return ret;
}

static void zb_macsplit_transport_confirm_timeout(zb_bufid_t param)
{
    ZB_ASSERT(param != 0);

    TRACE_MSG(TRACE_ERROR, "Receive %hd confirm timeout expired, buf len %hd", (FMT__H_H, param, zb_buf_len(param)));

    /* Can't do anything meaningful here */
    ZB_ASSERT(0);
}

static void zb_macsplit_transport_start_confirm_timer(zb_bufid_t param)
{
    if (zb_schedule_alarm(zb_macsplit_transport_confirm_timeout, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(ZB_MACSPLIT_CONFIRM_TIMEOUT)) != RET_OK)
    {
        TRACE_MSG(TRACE_INFO1, "Can't schedule the confirm packet alarm timeout: buf %hd", (FMT__H, param));
    }
}

static void zb_macsplit_transport_stop_confirm_timer(zb_bufid_t param)
{
    if (zb_schedule_alarm_cancel(zb_macsplit_transport_confirm_timeout, param, NULL) != RET_OK)
    {
        TRACE_MSG(TRACE_INFO1, "Can't cancel the confirm packet alarm timeout: buf %hd", (FMT__H, param));
    }
}
#endif  /* #if defined ZB_MACSPLIT_HOST */

/**
 * Functions for sending data
 */
void zb_macsplit_transport_send_data_with_type(zb_bufid_t param, zb_transport_call_type_t call_type)
{
    MACSPLIT_CTX().tx_calls_table[param] = call_type;

    if (call_type == ZB_TRANSPORT_CALL_TYPE_DEVICE_MCPS_DATA_CONFIRM)
    {
        /* We put data confirms at the beginning of the tx queue.
           Reason: these packets do not require the host to allocate a new buffer.
           We are thus decreasing the probability of OOM on both sides. */

        zb_uint8_t *ptr = ZB_RING_BUFFER_PUT_HEAD_RESERVE(&MACSPLIT_CTX().tx_queue);
        ZB_ASSERT(ptr != NULL);

        *ptr = param;
        /* The order of data.confirm's would be inverted, but it is not much of a problem:
           macsplit is very fast + host will always ack those packets immediately
         */
        ZB_RING_BUFFER_FLUSH_PUT_HEAD(&MACSPLIT_CTX().tx_queue);
    }
    else
    {
        ZB_RING_BUFFER_PUT(&MACSPLIT_CTX().tx_queue, param);
    }

    zb_macsplit_transport_send_data_with_type_internal();
}

void zb_macsplit_send_ota_msg(zb_bufid_t param)
{
    /* FIXME: dirty hack - 0xFF indicates that it is ZB_MAC_TRANSPORT_TYPE_OTA_PROTOCOL packet */
    zb_macsplit_transport_send_data_with_type(param, 0xFF);
}

void zb_macsplit_transport_send_data(zb_bufid_t param)
{
    zb_macsplit_transport_send_data_with_type(param, ZB_TRANSPORT_CALL_TYPE_NO_TYPE);
}

static zb_bool_t zb_macsplit_transport_is_body_present(zb_macsplit_packet_t *pkt)
{
    return (zb_bool_t)(pkt->hdr.flags.is_ack == ZB_FALSE);
}

static void zb_macsplit_transport_write_to_inner_buffer(zb_uint8_t *buf, zb_uint8_t len, zb_uint8_t *ind)
{
    ZB_MEMCPY(&MACSPLIT_CTX().tx_inner_buffer[*ind], buf, len);
    *ind += len;
}

static zb_uint8_t zb_macsplit_transport_transfer_packet_preparation(zb_macsplit_packet_t *pkt)
{
    zb_uint8_t len = 0;

    ZB_ASSERT(pkt->hdr.len >= sizeof(pkt->hdr));

    zb_macsplit_transport_write_to_inner_buffer(SIGNATURE, SIGNATURE_SIZE, &len);
    if (pkt->hdr.type == ZB_MAC_TRANSPORT_TYPE_MAC_SPLIT_DATA
            || pkt->hdr.type == ZB_MAC_TRANSPORT_TYPE_OTA_PROTOCOL)
    {
        zb_macsplit_transport_write_to_inner_buffer((zb_uint8_t *)&pkt->hdr, sizeof(pkt->hdr), &len);
        if (zb_macsplit_transport_is_body_present(pkt))
        {
            zb_macsplit_transport_write_to_inner_buffer((zb_uint8_t *)&pkt->body,
                    pkt->hdr.len - sizeof(pkt->hdr) - ZB_TRANSPORT_BODY_CRC_SIZE,
                    &len);
            zb_macsplit_transport_write_to_inner_buffer((zb_uint8_t *)&pkt->body.crc, ZB_TRANSPORT_BODY_CRC_SIZE, &len);
        }
    }
    else
    {
        /* MG: that code doesn't seem to be used in current implementation as trace/dump sending is implementted in 'common/trace'.
           FIXME: we have to send trace/dump via the same buffer to avoid mixing up packet parts!
         */

        /* write len, type and time (assuming that zb_macsplit_transport_hdr_t is always same size as zb_mac_transport_hdr_t) */
        zb_macsplit_transport_write_to_inner_buffer((zb_uint8_t *)&pkt->hdr,
                sizeof(pkt->hdr),
                &len);
        /* write body */
        zb_macsplit_transport_write_to_inner_buffer((zb_uint8_t *)&pkt->body,
                pkt->hdr.len - sizeof(pkt->hdr),
                &len);
    }

    return len;
}

static void zb_macsplit_transport_transfer_packet(zb_macsplit_packet_t *pkt, zb_bool_t retransmit)
{
    zb_uint8_t len = 0;

    ZVUNUSED(retransmit);

    /* prepare the packet for transfer */
    len = zb_macsplit_transport_transfer_packet_preparation(pkt);
    ZB_ASSERT(len);

    /* 12/11/2017 EE CR:MAJOR use "feature" define hare. Rationale: platforms other than Geniatech can communicate to Telink */
#if defined ZB_TRANSPORT_LINUX_SPI
    if (pkt->hdr.flags.is_ack)
    {
        zb_linux_spi_write_ack(MACSPLIT_CTX().tx_inner_buffer, len);
    }
    else
    {
        zb_linux_spi_write_packet(MACSPLIT_CTX().tx_inner_buffer, len);
    }

#elif defined ZB_PLATFORM_LINUX
    zb_linux_transport_send_data(MACSPLIT_CTX().tx_inner_buffer, len);
#else  /* not Linux */

#ifdef ZB_MACSPLIT_TRANSPORT_SERIAL
    zb_osif_serial_transport_put_bytes(MACSPLIT_CTX().tx_inner_buffer, len);
#endif

#ifdef ZB_MACSPLIT_TRANSPORT_USERIAL
    zb_osif_userial_put_bytes(MACSPLIT_CTX().tx_inner_buffer, len);
#endif

#ifdef ZB_MACSPLIT_TRANSPORT_SPI
    if (zb_osif_spi_send_data(MACSPLIT_CTX().tx_inner_buffer, len, retransmit) == ZB_FALSE)
    {
        ZB_ASSERT(0);
    }
#endif

#endif
}

static void zb_macsplit_transport_send_packet(zb_bool_t retransmit)
{
    ZB_ASSERT(MACSPLIT_CTX().tx_pkt.hdr.len >= sizeof(MACSPLIT_CTX().tx_pkt.hdr));

    if (zb_macsplit_transport_is_open())
    {

        if (TRACE_ENABLED(TRACE_MAC3))
        {
            zb_macsplit_transport_dump_transport_hdr(&MACSPLIT_CTX().tx_pkt, 1);
        }
        zb_macsplit_transport_transfer_packet(&MACSPLIT_CTX().tx_pkt, retransmit);
        zb_macsplit_transport_start_wait_for_ack_timer();

        TRACE_MSG(TRACE_MAC2, "Sent packet: packet num %hd, resend %hd", (FMT__H_H, MACSPLIT_CTX().tx_pkt.hdr.flags.packet_number, retransmit));
    }
    else
    {
        /* try to handle races between our processing and USB close in interrupt handler. */
        TRACE_MSG(TRACE_MAC1, "Oops - zb_macsplit_transport_send_packet while port is closed", (FMT__0));
    }
}

static void zb_macsplit_transport_send_ack_internal(zb_uint8_t packet_number, zb_bool_t should_retransmit)
{
    MACSPLIT_CTX().ack_pkt.len = sizeof(MACSPLIT_CTX().ack_pkt);
    MACSPLIT_CTX().ack_pkt.flags.should_retransmit = should_retransmit;
    MACSPLIT_CTX().ack_pkt.flags.packet_number     = packet_number;
    MACSPLIT_CTX().ack_pkt.crc = 0;
    MACSPLIT_CTX().ack_pkt.crc = zb_crc8((zb_uint8_t *)&MACSPLIT_CTX().ack_pkt,
                                         ZB_CRC8_INITIAL_VALUE,
                                         sizeof(MACSPLIT_CTX().ack_pkt) - sizeof(MACSPLIT_CTX().ack_pkt.crc));

    if (TRACE_ENABLED(TRACE_MAC3))
    {
        zb_macsplit_transport_dump_transport_hdr((zb_macsplit_packet_t *)&MACSPLIT_CTX().ack_pkt, 1);
    }

    /* ACK is not retransmittable */
    zb_macsplit_transport_transfer_packet((zb_macsplit_packet_t *)&MACSPLIT_CTX().ack_pkt, ZB_FALSE);

    TRACE_MSG(TRACE_MAC2, "Sent packet: ack num %hd", (FMT__H, MACSPLIT_CTX().ack_pkt.flags.packet_number));
}

static void zb_macsplit_transport_send_ack(zb_uint8_t packet_number)
{
    zb_macsplit_transport_send_ack_internal(packet_number, ZB_FALSE);
}

static void zb_macsplit_transport_send_not_ack(zb_uint8_t packet_number)
{
    zb_macsplit_transport_send_ack_internal(packet_number, ZB_TRUE);
}

static zb_ret_t zb_macsplit_transport_fill_tx_packet(zb_bufid_t param)
{
    zb_bufid_t buf = param;
    zb_ret_t ret = RET_OK;
    zb_bufid_t saved_param = param;
    zb_uint8_t packet_type = ZB_MAC_TRANSPORT_TYPE_MAC_SPLIT_DATA;

    /* fill header */

    /* dirty hack: if call_type is 0xFF it is ZB_MAC_TRANSPORT_TYPE_OTA_PROTOCOL packet type */
    if (MACSPLIT_CTX().tx_calls_table[param] == 0xFF)
    {
        packet_type = ZB_MAC_TRANSPORT_TYPE_OTA_PROTOCOL;
    }
    else
    {
        /* fill call type (it's part of body now) */
        MACSPLIT_CTX().tx_pkt.body.call_type = MACSPLIT_CTX().tx_calls_table[param];
    }
    MACSPLIT_CTX().tx_pkt.hdr.flags.packet_number = MACSPLIT_CTX().curr_pkt_number;

    /* Packet_number is 2 bits while curr_pkt_number is 1 byte. Handle overflow. */
    MACSPLIT_CTX().curr_pkt_number++;
    if (MACSPLIT_CTX().curr_pkt_number > ZB_TRANSPORT_MAX_PACKET_NUMBER)
    {
        /* Use packet number 0 only once, to send boot
        * report/boot request. Later use only values 1,2,3. Rationale: prevent
        * throwing out boot indication/request as a dup. */
        MACSPLIT_CTX().curr_pkt_number = 1;
    }
    MACSPLIT_CTX().tx_pkt.hdr.type                = packet_type;
    MACSPLIT_CTX().tx_pkt.hdr.flags.is_ack        = ZB_FALSE;

    /* specific processing for MACSplit Host and Device */
    if (packet_type == ZB_MAC_TRANSPORT_TYPE_MAC_SPLIT_DATA)
    {
#if defined ZB_MACSPLIT_HOST
        /* send bufid to device. Need for correct handling confirm data */
        MACSPLIT_CTX().tx_pkt.body.msdu_handle = param;
        ZB_CLR_BIT_IN_BIT_VECTOR(MACSPLIT_CTX().specific_ctx.recv_cfm, param);

        if (MACSPLIT_CTX().tx_pkt.body.call_type == ZB_TRANSPORT_CALL_TYPE_HOST_MCPS_DATA_REQUEST)
        {
            /* prevent to freeing buffer */
            param = 0;

            if (zb_buf_flags_get(buf) & ZB_BUF_SECUR_ALL_ENCR)
            {
                /* need to secure frame */
                ret = zb_macsplit_transport_secure_frame(buf);
                if (ret == RET_OK)
                {
                    /* copy data request param to encryption buf */
                    ZB_MEMCPY(ZB_BUF_GET_PARAM(SEC_CTX().encryption_buf, zb_mcps_data_req_params_t),
                              ZB_BUF_GET_PARAM(buf, zb_mcps_data_req_params_t),
                              sizeof(zb_mcps_data_req_params_t));

                    /* further use encryption buf */
                    buf = SEC_CTX().encryption_buf;
                }
                else
                {
                    TRACE_MSG(TRACE_INFO1, "Error %d securing frame! Drop it!", (FMT__D, ret));

                    /* free buf */
                    zb_buf_free(buf);
                }
            }
        }
        else if (MACSPLIT_CTX().tx_pkt.body.call_type == ZB_TRANSPORT_CALL_TYPE_HOST_MLME_SET_REQUEST)
        {
            zb_mlme_set_request_t *set_req = (zb_mlme_set_request_t *)zb_buf_begin(buf);

            MACSPLIT_CTX().specific_ctx.confirm_cb[param] = set_req->confirm_cb_u.cb;
            /* prevent to freeing buffer */
            param = 0;
        }
        else if (MACSPLIT_CTX().tx_pkt.body.call_type == ZB_TRANSPORT_CALL_TYPE_HOST_MLME_GET_REQUEST)
        {
            zb_mlme_get_request_t *get_req = (zb_mlme_get_request_t *)zb_buf_begin(buf);

            MACSPLIT_CTX().specific_ctx.confirm_cb[param] = get_req->confirm_cb_u.cb;
            /* prevent to freeing buffer */
            param = 0;
        }
#elif defined ZB_MACSPLIT_DEVICE
        /* return bufid number to host */
        if (MACSPLIT_CTX().tx_pkt.body.call_type != ZB_TRANSPORT_CALL_TYPE_DEVICE_MCPS_DATA_INDICATION)
        {
            MACSPLIT_CTX().tx_pkt.body.msdu_handle = MACSPLIT_CTX().specific_ctx.msdu_handles[param];
            TRACE_MSG(TRACE_MAC3, "tx msdu_handle: mac param %hd for host %hd", (FMT__H_H, param, MACSPLIT_CTX().specific_ctx.msdu_handles[param]));
        }
        else
        {
            MACSPLIT_CTX().tx_pkt.body.msdu_handle = 0;
        }
#endif /* ZB_MACSPLIT_HOST */
    }

    if (ret == RET_OK)
    {
        zb_uint8_t serialized_size = zb_buf_serialize(buf, MACSPLIT_CTX().tx_pkt.body.data);
        /* Len field includes everything from header and body, so to calculate it, we need: */
        MACSPLIT_CTX().tx_pkt.hdr.len = sizeof(MACSPLIT_CTX().tx_pkt.hdr) +
                                        sizeof(MACSPLIT_CTX().rx_pkt.body.msdu_handle) +
                                        sizeof(MACSPLIT_CTX().rx_pkt.body.call_type) +
                                        serialized_size +
                                        ZB_TRANSPORT_BODY_CRC_SIZE;
        MACSPLIT_CTX().tx_pkt.hdr.crc = zb_crc8((zb_uint8_t *)&MACSPLIT_CTX().tx_pkt.hdr,
                                                ZB_CRC8_INITIAL_VALUE,
                                                sizeof(MACSPLIT_CTX().tx_pkt.hdr) - sizeof(MACSPLIT_CTX().tx_pkt.hdr.crc));
        MACSPLIT_CTX().tx_pkt.body.crc = zb_crc16((zb_uint8_t *)&MACSPLIT_CTX().tx_pkt.body,
                                         ZB_CRC16_INITIAL_VALUE,
                                         MACSPLIT_CTX().tx_pkt.hdr.len - sizeof(MACSPLIT_CTX().tx_pkt.hdr) - ZB_TRANSPORT_BODY_CRC_SIZE);
        TRACE_MSG(TRACE_MAC2, "tx mac param %hd : body crc 0x%x", (FMT__H_D, saved_param, MACSPLIT_CTX().tx_pkt.body.crc));
    }

    if (param != 0)
    {
        zb_buf_free(param);
    }

    return ret;
}

static void zb_macsplit_transport_send_data_with_type_internal(void)
{
    zb_bufid_t param = 0;
    zb_ret_t   ret = RET_OK;


    ZB_ASSERT(!ZB_RING_BUFFER_IS_EMPTY(&MACSPLIT_CTX().tx_queue));

    if (!MACSPLIT_CTX().is_waiting_for_ack
#ifdef ZB_MACSPLIT_HOST
            && !MACSPLIT_CTX().forced_device_reset
#endif
       )
    {
        param = *ZB_RING_BUFFER_GET(&MACSPLIT_CTX().tx_queue);
        ZB_RING_BUFFER_FLUSH_GET(&MACSPLIT_CTX().tx_queue);

        ret = zb_macsplit_transport_fill_tx_packet(param);
        if (ret == RET_OK)
        {
            zb_macsplit_transport_send_packet(ZB_FALSE);
        }
        else
        {
            TRACE_MSG(TRACE_INFO1, "fill tx packet failed", (FMT__0));
        }
    }
    else
    {
        TRACE_MSG(TRACE_MAC1, "Request queued", (FMT__0));
    }
}


/**
 * Functions for handle received data
 */
static zb_bool_t zb_macsplit_transport_check_hdr(void)
{
    zb_bool_t ret = ZB_TRUE;

    if (MACSPLIT_CTX().rx_pkt.hdr.type == ZB_MAC_TRANSPORT_TYPE_MAC_SPLIT_DATA
            || MACSPLIT_CTX().rx_pkt.hdr.type == ZB_MAC_TRANSPORT_TYPE_OTA_PROTOCOL)
    {
        if (MACSPLIT_CTX().rx_pkt.hdr.crc == zb_crc8((zb_uint8_t *)&MACSPLIT_CTX().rx_pkt.hdr,
                ZB_CRC8_INITIAL_VALUE,
                sizeof(MACSPLIT_CTX().rx_pkt.hdr) - sizeof(MACSPLIT_CTX().rx_pkt.hdr.crc)))
        {
            ret = ZB_TRUE;
            if (TRACE_ENABLED(TRACE_MAC3))
            {
                zb_macsplit_transport_dump_transport_hdr((zb_macsplit_packet_t *)&MACSPLIT_CTX().rx_pkt.hdr, 0);
            }
        }
        else
        {
            /* 12/12/2017 EE CR:MINOR For the future: try to implement
               re-syncronization by rolling back rx pointer in the ring buffer to the
               point "first signature byte + 1", or any byte after it if "first
               signature byte + 1" is already overwritten.
               Be sure that ring buffer contains at least a header, else fall back to sending NACK.
               Remember the fact of started re-synchronization.

               Next set machine state to "receiving signature", 0 bytes received.

               When system is about to block waiting for serial traffic due to
               absence of data in the ring buffer, if "resynchronize attempt" flag is
               set, go to "receiving signature" and send NACK.

               Same for body check.
            */
            /* not acknowledged, need retransmit */
            TRACE_MSG(TRACE_INFO1, "Bad crc in hdr. Need retransmit.", (FMT__0));
            zb_macsplit_transport_send_not_ack(MACSPLIT_CTX().rx_pkt.hdr.flags.packet_number);
            ret = ZB_FALSE;
        }
    }
    return ret;
}

static zb_bool_t zb_macsplit_transport_check_body(void)
{
    zb_bool_t   ret;
    zb_uint16_t crc;
    zb_uint16_t received_crc;

    received_crc = MACSPLIT_CTX().rx_pkt.body.crc;
    MACSPLIT_CTX().rx_pkt.body.crc = 0;
    crc = zb_crc16((zb_uint8_t *)&MACSPLIT_CTX().rx_pkt.body,
                   ZB_CRC16_INITIAL_VALUE,
                   MACSPLIT_CTX().rx_pkt.hdr.len - sizeof(MACSPLIT_CTX().rx_pkt.hdr) - ZB_TRANSPORT_BODY_CRC_SIZE);

    if (crc == received_crc)
    {
        ret = ZB_TRUE;
    }
    else
    {
        TRACE_MSG(TRACE_INFO1, "Bad crc in body. Need retransmit", (FMT__0));
        /* not acknowledged, need retransmit */
        zb_macsplit_transport_send_not_ack(MACSPLIT_CTX().rx_pkt.hdr.flags.packet_number);
        ret = ZB_FALSE;
    }

    return ret;
}
static zb_bufid_t zb_macsplit_transport_get_and_fill_rx_packet(void)
{
    zb_bufid_t bufid;
#ifdef ZB_MACSPLIT_HOST
    bufid = zb_buf_get(ZB_TRUE, 0);
#else
    /* In experimenting with "out of buffers" at MAC side let's always use OUT for a MAC. */
    bufid = zb_buf_get_out();
#endif

    if (bufid == 0)
    {
        /* 12/12/2017 EE CR:MINOR For the future: implement handling of the packet after OOM situation ended. To do it:
           - set flag 'rx packet handle pending'
           - call GET_BUFF_DELAYED here
           - when received next packet over serial, clear flag 'rx packet handle pending'
           - when buffer is allocated & callback called, check 'rx packet handle pending' flag.
            - if flag is cleared, free buffer and return
            - if flag is still set, handle the packet.
         */
        return 0;
    }
    zb_buf_deserialize(bufid, MACSPLIT_CTX().rx_pkt.body.data, zb_macsplit_pkt_payload_size(&MACSPLIT_CTX().rx_pkt));

#if defined ZB_MACSPLIT_DEVICE
    /* store bufid number from host */
    MACSPLIT_CTX().specific_ctx.msdu_handles[bufid] = MACSPLIT_CTX().rx_pkt.body.msdu_handle;
    TRACE_MSG(TRACE_MAC3, "msdu_handles: mac param %hd for host %hd", (FMT__H_H, bufid, MACSPLIT_CTX().rx_pkt.body.msdu_handle));
#endif /* ZB_MACSPLIT_DEVICE */

    return bufid;
}

static void zb_macsplit_transport_handle_ack(void)
{
    ZB_ASSERT(MACSPLIT_CTX().rx_pkt.hdr.flags.is_ack);

    TRACE_MSG(TRACE_MAC2, "Recv packet: ack num %hd", (FMT__H, MACSPLIT_CTX().rx_pkt.hdr.flags.packet_number));

    if (MACSPLIT_CTX().is_waiting_for_ack)
    {
        if (MACSPLIT_CTX().rx_pkt.hdr.flags.packet_number ==
                MACSPLIT_CTX().tx_pkt.hdr.flags.packet_number)
        {
            zb_macsplit_transport_stop_wait_for_ack_timer();
            if (MACSPLIT_CTX().rx_pkt.hdr.flags.should_retransmit)
            {
                TRACE_MSG(TRACE_MAC1, "Retransmit requested, send packet", (FMT__0));
                zb_macsplit_transport_send_packet(ZB_FALSE);
            }
            else
            {
#ifdef ZB_MACSPLIT_HOST
                /* We are starting a confirm timeout timer only after we are sure that
                   the other side has successfully received the frame */
                if (zb_macsplit_transport_needs_confirm_timer(MACSPLIT_CTX().tx_pkt.body.call_type, MACSPLIT_CTX().tx_pkt.body.msdu_handle))
                {
                    zb_macsplit_transport_start_confirm_timer(MACSPLIT_CTX().tx_pkt.body.msdu_handle);
                }
#endif

                /* check ring buffer */
                if (!ZB_RING_BUFFER_IS_EMPTY(&MACSPLIT_CTX().tx_queue))
                {
                    zb_macsplit_transport_send_data_with_type_internal();
                }
#ifdef ZB_MACSPLIT_HANDLE_DATA_BY_APP
                else if (MACSPLIT_CTX().handle_data_by_app_after_last_ack)
                {
                    MACSPLIT_CTX().handle_data_by_app = 1;

                    if (zb_macsplit_transport_needs_confirm_timer(MACSPLIT_CTX().tx_pkt.body.call_type, MACSPLIT_CTX().tx_pkt.body.msdu_handle))
                    {
                        zb_macsplit_transport_stop_confirm_timer(MACSPLIT_CTX().tx_pkt.body.msdu_handle);
                    }
                }
#endif
            }
        }
        else
        {
            TRACE_MSG(TRACE_MAC1, "Ack with wrong packet number, retransmit req %d", (FMT__D, MACSPLIT_CTX().rx_pkt.hdr.flags.should_retransmit));
        }

    }
    else
    {
        TRACE_MSG(TRACE_INFO1, "Unexpected Ack!", (FMT__0));
    }
}

void zb_macsplit_push_tx_queue(void)
{
    if (!ZB_RING_BUFFER_IS_EMPTY(&MACSPLIT_CTX().tx_queue))
    {
        zb_macsplit_transport_send_data_with_type_internal();
    }
}

/* 12/12/2017 EE CR:MINOR Better move that routine to zb_macsplit.c: this is not a transport. */
#if defined ZB_MACSPLIT_HOST
void zb_macsplit_transport_handle_device_boot_call(zb_bufid_t param)
{
    ZB_SCHEDULE_CALLBACK(zb_mlme_dev_reset_conf, param);
}

void zb_macsplit_transport_handle_data_confirm_call(zb_bufid_t param)
{
    zb_bufid_t msdu_handle = MACSPLIT_CTX().rx_pkt.body.msdu_handle;
    zb_mcps_data_confirm_params_t *confirm_params;
    zb_uint8_t confirm_offset;
    TRACE_MSG(TRACE_MAC2, ">handle_data_confirm for req bufid: %hd", (FMT__H, msdu_handle));

    ZB_ASSERT(msdu_handle);
    /* We must not have a valid temporary buffer. Reuse existing buffer instead.
       Just received buffer is in MACSPLIT_CTX().rx_pkt.body.data
     */
    ZB_ASSERT(param == 0xffu);

    ZB_SET_BIT_IN_BIT_VECTOR(MACSPLIT_CTX().specific_ctx.recv_cfm, msdu_handle);

    /* cancel alarm */
    zb_macsplit_transport_stop_confirm_timer(msdu_handle);

    /* Need to align data offset size to 4 because ZB_BUF_GET_PARAM do this. */
    confirm_offset = sizeof(zb_mcps_data_confirm_params_t);
    if (confirm_offset % ZB_BUF_ALLOC_ALIGN)
    {
        confirm_offset = ((confirm_offset + ZB_BUF_ALLOC_ALIGN - 1U) / ZB_BUF_ALLOC_ALIGN) * ZB_BUF_ALLOC_ALIGN;
    }

    confirm_params = ZB_BUF_GET_PARAM(msdu_handle, zb_mcps_data_confirm_params_t);
    ZB_MEMCPY(confirm_params,
              &MACSPLIT_CTX().rx_pkt.body.data[zb_macsplit_pkt_payload_size(&MACSPLIT_CTX().rx_pkt) - confirm_offset],
              sizeof(zb_mcps_data_confirm_params_t));

    ZB_SCHEDULE_CALLBACK(zb_mcps_data_confirm, msdu_handle);
    TRACE_MSG(TRACE_MAC2, "<handle_data_confirm", (FMT__0));
}

void zb_macsplit_transport_handle_set_get_confirm_call(zb_bufid_t param)
{
    zb_bufid_t msdu_handle = MACSPLIT_CTX().rx_pkt.body.msdu_handle;

    TRACE_MSG(TRACE_MAC2, ">handle_set_get_confirm for req bufid: %hd", (FMT__H, msdu_handle));

    ZB_ASSERT(msdu_handle);

    ZB_SET_BIT_IN_BIT_VECTOR(MACSPLIT_CTX().specific_ctx.recv_cfm, msdu_handle);

    /* cancel alarm */
    zb_macsplit_transport_stop_confirm_timer(msdu_handle);

    if (MACSPLIT_CTX().specific_ctx.confirm_cb[msdu_handle])
    {
        /* Copy param buffer to macsplit context */
        ZB_MEMCPY(zb_buf_initial_alloc(msdu_handle, zb_buf_len(param)),
                  zb_buf_begin(param),
                  zb_buf_len(param));
        ZB_SCHEDULE_CALLBACK(MACSPLIT_CTX().specific_ctx.confirm_cb[msdu_handle], msdu_handle);
    }
    else
    {
        TRACE_MSG(TRACE_MAC1, "no requested confirmation callback for %hd, free", (FMT__H, msdu_handle));
        zb_buf_free(msdu_handle);
    }


    /* unlock buf */
    zb_buf_free(param);

    TRACE_MSG(TRACE_MAC2, "<handle_set_get_confirm", (FMT__0));
}
#endif /* ZB_MACSPLIT_HOST */

static void zb_macsplit_save_last_rx_packet_num()
{
    MACSPLIT_CTX().last_rx_pkt_number = MACSPLIT_CTX().rx_pkt.hdr.flags.packet_number;
    TRACE_MSG(TRACE_MAC3, "new last_rx_pkt_number %hd", (FMT__H, MACSPLIT_CTX().last_rx_pkt_number));
}

static void zb_macsplit_transport_handle_packet(void)
{
    switch (MACSPLIT_CTX().rx_pkt.hdr.type)
    {
    case ZB_MAC_TRANSPORT_TYPE_MAC_SPLIT_DATA: /* FALLTHROUGH */
    case ZB_MAC_TRANSPORT_TYPE_OTA_PROTOCOL:
        TRACE_MSG(TRACE_MAC1, "Data packet %hd received", (FMT__H, MACSPLIT_CTX().rx_pkt.hdr.flags.packet_number));
        zb_macsplit_transport_handle_macsplit_packet();
        break;
    case ZB_MAC_TRANSPORT_TYPE_TRACE:
#if TRACE_ENABLED(TRACE_MAC1)
    {
        zb_uint16_t body_data = 0;
        ZB_MEMCPY(&body_data, &MACSPLIT_CTX().rx_pkt.body.data, sizeof(body_data));
        TRACE_MSG(TRACE_MAC1, "Trace packet received, id %d", (FMT__D, body_data));
    }
#endif /* TRACE_ENABLED(TRACE_MAC1) */
    zb_macsplit_transport_handle_trace_packet();
    break;

    case ZB_MAC_TRANSPORT_TYPE_DUMP:
        TRACE_MSG(TRACE_MAC1, "<Traffic dump packet received", (FMT__0));
        break;
    case (ZB_MAC_TRANSPORT_TYPE_DUMP | 0x80):
        TRACE_MSG(TRACE_MAC1, ">Traffic dump packet received", (FMT__0));
        break;


    default:
        TRACE_MSG(TRACE_MAC1, "Unknown packet received, type %hd", (FMT__H, MACSPLIT_CTX().rx_pkt.hdr.type));
        break;
    }
}

static void zb_macsplit_transport_handle_macsplit_packet(void)
{
    zb_bufid_t param;
    zb_bool_t  should_return = ZB_FALSE;

    TRACE_MSG(TRACE_MAC2, "Recv packet: packet num %hd", (FMT__H, MACSPLIT_CTX().rx_pkt.hdr.flags.packet_number));

    if (!zb_macsplit_transport_is_open())
    {
        should_return = ZB_TRUE;
    }

    if (MACSPLIT_CTX().rx_pkt.hdr.type != ZB_MAC_TRANSPORT_TYPE_MAC_SPLIT_DATA
            && MACSPLIT_CTX().rx_pkt.hdr.type != ZB_MAC_TRANSPORT_TYPE_OTA_PROTOCOL)
    {
        TRACE_MSG(TRACE_MAC3, "specific transport type: %hd", (FMT__H, MACSPLIT_CTX().rx_pkt.hdr.type));
        should_return = ZB_TRUE;
    }
    /* handle duplicates */
    if (!should_return && !MACSPLIT_CTX().rx_pkt.hdr.flags.is_ack)
    {
        if (MACSPLIT_CTX().rx_pkt.hdr.flags.packet_number ==
                MACSPLIT_CTX().last_rx_pkt_number)
        {
            zb_macsplit_transport_send_ack(MACSPLIT_CTX().rx_pkt.hdr.flags.packet_number);
            should_return = ZB_TRUE;
            TRACE_MSG(TRACE_INFO1, "Got duplicate %hd, discard it", (FMT__H, (zb_uint8_t)MACSPLIT_CTX().last_rx_pkt_number));
        }
    }

#if defined ZB_MACSPLIT_FW_UPGRADE
    if (!should_return && MACSPLIT_CTX().rx_pkt.hdr.type == ZB_MAC_TRANSPORT_TYPE_OTA_PROTOCOL)
    {
        zb_uint8_t *body_ptr;
        zb_uint8_t buf_hdr_size;

        zb_macsplit_save_last_rx_packet_num();
        /* send ack */
        zb_macsplit_transport_send_ack(MACSPLIT_CTX().rx_pkt.hdr.flags.packet_number);
        body_ptr = zb_buf_partial_deserialize(MACSPLIT_CTX().rx_pkt.body.data, &buf_hdr_size);
        zb_ota_handle_packet(body_ptr, MACSPLIT_CTX().rx_pkt.hdr.len - sizeof(MACSPLIT_CTX().rx_pkt.hdr) - buf_hdr_size - ZB_TRANSPORT_BODY_CRC_SIZE);
        return;
    }
#endif

    /* handle calls */
    if (!should_return)
    {
        if (!zb_macsplit_call_is_conf(MACSPLIT_CTX().rx_pkt.body.call_type))
        {
            param = zb_macsplit_transport_get_and_fill_rx_packet();
        }
        else
        {
            param = 0xffu;
        }
        if (param)
        {
            zb_macsplit_save_last_rx_packet_num();
            /* send ack */
            zb_macsplit_transport_send_ack(MACSPLIT_CTX().rx_pkt.hdr.flags.packet_number);
            zb_macsplit_handle_call(param, MACSPLIT_CTX().rx_pkt.body.call_type);
        }
        else
        {
            /* If could not read, do not send ack, do not move last_rx_pkt_number. Maybe, after retransmit will be lucky. */
            TRACE_MSG(TRACE_INFO1, "Can't allocate buffer for received packet!", (FMT__0));
            should_return = ZB_TRUE;
            //      zb_reset(0);
        }
    }
}

static void zb_macsplit_transport_handle_trace_packet(void)
{
    zb_uint8_t rx_buf[255];
    zb_uint8_t len;
    zb_uint_t  body_size;

    body_size = MACSPLIT_CTX().rx_pkt.hdr.len - sizeof(MACSPLIT_CTX().rx_pkt.hdr);
    len = sizeof(signature) + sizeof(MACSPLIT_CTX().rx_pkt.hdr) + body_size;
    /* signature */
    ZB_MEMCPY(rx_buf, (zb_uint8_t *)&SIGNATURE, sizeof(signature));
    /* header */
    ZB_MEMCPY(rx_buf + 2, (zb_uint8_t *)&MACSPLIT_CTX().rx_pkt.hdr, sizeof(MACSPLIT_CTX().rx_pkt.hdr));
    /* body */
    ZB_MEMCPY(rx_buf + 2 + sizeof(MACSPLIT_CTX().rx_pkt.hdr), (zb_uint8_t *)&MACSPLIT_CTX().rx_pkt.body.data, body_size);

#if defined ZB_MACSPLIT_TRACE_DUMP_TO_FILE
    zb_osif_file_write(MACSPLIT_CTX().trace_file, rx_buf, len);
    zb_osif_file_flush(MACSPLIT_CTX().trace_file);
#endif
#ifdef ZB_BINARY_TRACE
    zb_trace_put_bytes(0, rx_buf, len);
#else
    if (MACSPLIT_CTX().device_trace_cb)
    {
        (void)(*MACSPLIT_CTX().device_trace_cb)(rx_buf, len);
    }
#endif
}


static void zb_macsplit_transport_recv_signature(zb_uint8_t byte)
{
    if (byte == SIGNATURE[MACSPLIT_CTX().received_bytes])
    {
        MACSPLIT_CTX().received_bytes++;
    }
    else
    {
        TRACE_MSG(TRACE_INFO1, "Wrong signature, %x", (FMT__H, byte));
        MACSPLIT_CTX().received_bytes = 0;
    }

    if (MACSPLIT_CTX().received_bytes == SIGNATURE_SIZE)
    {
        MACSPLIT_CTX().received_bytes  = 0;
        MACSPLIT_CTX().transport_state = RECEIVING_LENGTH;

        /* waiting for full packet */
        zb_macsplit_transport_start_recv_packet_timer();
    }
}

static void zb_macsplit_transport_recv_length(zb_uint8_t byte)
{
    if (byte < sizeof(MACSPLIT_CTX().rx_pkt.hdr) ||
            byte > sizeof(MACSPLIT_CTX().rx_pkt.hdr) + ZB_TRANSPORT_BODY_CRC_SIZE + ZB_TRANSPORT_DATA_SIZE)
    {
        TRACE_MSG(TRACE_INFO1, "Invalid packet length, drop packet", (FMT__0));
        zb_macsplit_transport_stop_recv_packet_timer();
    }
    else
    {
        MACSPLIT_CTX().rx_pkt.hdr.len  = byte;
        MACSPLIT_CTX().transport_state = RECEIVING_TYPE;
    }
}

static void zb_macsplit_transport_recv_msdu_handle(zb_uint8_t byte)
{
    MACSPLIT_CTX().rx_pkt.body.msdu_handle = byte;
    MACSPLIT_CTX().received_bytes = 0;
    MACSPLIT_CTX().transport_state = RECEIVING_CALL_TYPE;
}

static void zb_macsplit_transport_recv_call_type(zb_uint8_t byte)
{
    /* call type consists of 2 bytes now */

    ((zb_uint8_t *)&MACSPLIT_CTX().rx_pkt.body.call_type)[MACSPLIT_CTX().received_bytes] = byte;
    MACSPLIT_CTX().received_bytes++;

    if (MACSPLIT_CTX().received_bytes == 2)
    {
        MACSPLIT_CTX().received_bytes = 0;
        MACSPLIT_CTX().transport_state = RECEIVING_BODY_DATA;
    }
}

static void zb_macsplit_transport_recv_type(zb_uint8_t byte)
{
    MACSPLIT_CTX().rx_pkt.hdr.type = byte;

    switch (MACSPLIT_CTX().rx_pkt.hdr.type)
    {
    case ZB_MAC_TRANSPORT_TYPE_MAC_SPLIT_DATA:
    case ZB_MAC_TRANSPORT_TYPE_OTA_PROTOCOL:
        MACSPLIT_CTX().transport_state = RECEIVING_FLAGS;
        break;

    default:
        MACSPLIT_CTX().transport_state = RECEIVING_TIME;
        break;
    }
}

static void zb_macsplit_transport_recv_time(zb_uint8_t byte)
{
    /* Assume that zb_macsplit_transport_hdr_t is always same size as zb_mac_transport_hdr_t,
     * so writing 'time' bytes to the same place as in zb_mac_transport_hdr_t */

    ((zb_uint8_t *)&MACSPLIT_CTX().rx_pkt.hdr.flags)[MACSPLIT_CTX().received_bytes] = byte;
    MACSPLIT_CTX().received_bytes++;

    if (MACSPLIT_CTX().received_bytes == 2)
    {
        MACSPLIT_CTX().received_bytes = 0;
        MACSPLIT_CTX().transport_state = RECEIVING_BODY_DATA;
    }
}

static void zb_macsplit_transport_recv_flags(zb_uint8_t byte)
{
    *((zb_uint8_t *)&MACSPLIT_CTX().rx_pkt.hdr.flags) = byte;
    MACSPLIT_CTX().transport_state = RECEIVING_HDR_CRC;
}

static void zb_macsplit_transport_recv_hdr_crc(zb_uint8_t byte)
{
    MACSPLIT_CTX().rx_pkt.hdr.crc = byte;

    if (zb_macsplit_transport_check_hdr())
    {
        /* TODO: improve ack handling logics (for ack in packets) */
        if (MACSPLIT_CTX().rx_pkt.hdr.flags.is_ack)
        {
            zb_macsplit_transport_stop_recv_packet_timer();
            zb_macsplit_transport_handle_ack();
        }
        else
        {
            MACSPLIT_CTX().received_bytes  = 0;
            MACSPLIT_CTX().transport_state = RECEIVING_MSDU_HANDLE;
        }
    }
    else
    {
        zb_macsplit_transport_stop_recv_packet_timer();
    }
}


static void zb_macsplit_transport_recv_body_data(zb_uint8_t byte)
{
    switch (MACSPLIT_CTX().rx_pkt.hdr.type)
    {
    case ZB_MAC_TRANSPORT_TYPE_MAC_SPLIT_DATA: /* FALLTHROUGH */
    case ZB_MAC_TRANSPORT_TYPE_OTA_PROTOCOL:
    {
        ((zb_uint8_t *)&MACSPLIT_CTX().rx_pkt.body.data)[MACSPLIT_CTX().received_bytes] = byte;

        if (&MACSPLIT_CTX().rx_pkt.body.data[MACSPLIT_CTX().received_bytes] >
                ((zb_uint8_t *)&MACSPLIT_CTX().rx_pkt.hdr) + MACSPLIT_CTX().rx_pkt.hdr.len
                || &MACSPLIT_CTX().rx_pkt.body.data[MACSPLIT_CTX().received_bytes] >=
                (zb_uint8_t *)&MACSPLIT_CTX().tx_pkt)
        {
            /* something went wrong */
            ZB_ASSERT(0);
        }

        MACSPLIT_CTX().received_bytes++;

        if (MACSPLIT_CTX().received_bytes == zb_macsplit_pkt_payload_size(&MACSPLIT_CTX().rx_pkt))
        {
            MACSPLIT_CTX().received_bytes = 0;
            MACSPLIT_CTX().transport_state = RECEIVING_BODY_CRC;
        }
        break;
    }

    case ZB_MAC_TRANSPORT_TYPE_DUMP: /* FALLTHROGH */
    case ZB_MAC_TRANSPORT_TYPE_TRACE:

        ((zb_uint8_t *)&MACSPLIT_CTX().rx_pkt.body.data)[MACSPLIT_CTX().received_bytes] = byte;

        if (MACSPLIT_CTX().received_bytes >= MACSPLIT_CTX().rx_pkt.hdr.len
                || MACSPLIT_CTX().received_bytes >= sizeof(MACSPLIT_CTX().rx_pkt.body.data))
        {
            /* something went wrong */
            ZB_ASSERT(0);
        }

        MACSPLIT_CTX().received_bytes++;

        if (MACSPLIT_CTX().received_bytes ==
                MACSPLIT_CTX().rx_pkt.hdr.len - sizeof(MACSPLIT_CTX().rx_pkt.hdr))
        {
            zb_macsplit_transport_stop_recv_packet_timer();
            zb_macsplit_transport_handle_packet();
        }
        break;

    default:
    {
        ((zb_uint8_t *)&MACSPLIT_CTX().rx_pkt.body.data)[MACSPLIT_CTX().received_bytes] = byte;

        if (MACSPLIT_CTX().received_bytes >= MACSPLIT_CTX().rx_pkt.hdr.len
                || MACSPLIT_CTX().received_bytes >= sizeof(MACSPLIT_CTX().rx_pkt.body))
        {
            /* something went wrong */
            if (TRACE_ENABLED(TRACE_MAC3))
            {
                zb_macsplit_transport_dump_transport_hdr(&MACSPLIT_CTX().rx_pkt, 0);
            }
            ZB_ASSERT(0);
        }

        MACSPLIT_CTX().received_bytes++;

        if (MACSPLIT_CTX().received_bytes ==
                MACSPLIT_CTX().rx_pkt.hdr.len - sizeof(MACSPLIT_CTX().rx_pkt.hdr))
        {
            zb_macsplit_transport_stop_recv_packet_timer();
            zb_macsplit_transport_handle_packet();
        }

        break;
    }
    }
}

static void zb_macsplit_transport_recv_body_crc(zb_uint8_t byte)
{
    ((zb_uint8_t *)&MACSPLIT_CTX().rx_pkt.body.crc)[MACSPLIT_CTX().received_bytes] = byte;
    MACSPLIT_CTX().received_bytes++;

    if (MACSPLIT_CTX().received_bytes == ZB_TRANSPORT_BODY_CRC_SIZE)
    {
        zb_macsplit_transport_stop_recv_packet_timer();

        if (zb_macsplit_transport_check_body())
        {
            zb_macsplit_transport_handle_packet();
        }
    }
}

static void zb_macsplit_transport_recv_byte(zb_uint8_t byte)
{
    /* TRACE_MSG(TRACE_MAC3, "zb_macsplit_transport_recv_byte: %hd", (FMT__H, byte)); */

    switch (MACSPLIT_CTX().transport_state)
    {
    case RECEIVING_SIGNATURE:
        zb_macsplit_transport_recv_signature(byte);
        break;

    /* header/common */
    case RECEIVING_LENGTH:
        zb_macsplit_transport_recv_length(byte);
        break;

    case RECEIVING_TYPE:
        zb_macsplit_transport_recv_type(byte);
        break;

    /* header/mac_data */
    case RECEIVING_FLAGS:
        zb_macsplit_transport_recv_flags(byte);
        break;

    case RECEIVING_HDR_CRC:
        zb_macsplit_transport_recv_hdr_crc(byte);
        break;

    /* header/trace */
    case RECEIVING_TIME:
        zb_macsplit_transport_recv_time(byte);
        break;

    /* body/mac_data */
    case RECEIVING_MSDU_HANDLE:
        zb_macsplit_transport_recv_msdu_handle(byte);
        break;

    case RECEIVING_CALL_TYPE:
        zb_macsplit_transport_recv_call_type(byte);
        break;

    /* body/common */
    case RECEIVING_BODY_DATA:
        zb_macsplit_transport_recv_body_data(byte);
        break;
    case RECEIVING_BODY_CRC:
        zb_macsplit_transport_recv_body_crc(byte);
        break;
    }
}

#if defined ZB_PLATFORM_LINUX
void zb_linux_transport_handle_data(void)
{
    zb_uint8_t byte;

    while (zb_linux_transport_is_data_available())
    {
        byte = zb_linux_transport_get_next_byte();
#ifdef ZB_MACSPLIT_HANDLE_DATA_BY_APP
        if (MACSPLIT_CTX().handle_data_by_app)
        {
            ZB_ASSERT(MACSPLIT_CTX().handle_data_by_app_cb);
            (*MACSPLIT_CTX().handle_data_by_app_cb)(byte);
        }
        else
        {
            zb_macsplit_transport_recv_byte(byte);
        }
#else
        zb_macsplit_transport_recv_byte(byte);
#endif
    }
}

#else

static void zb_macsplit_transport_recv_byte_from_osif(zb_uint8_t byte)
{
#if defined ZB_MACSPLIT_USE_IO_BUFFERS
    ZB_ASSERT(!ZB_RING_BUFFER_IS_FULL(&MACSPLIT_CTX().rx_buffer));
    ZB_RING_BUFFER_PUT(&MACSPLIT_CTX().rx_buffer, byte);
#else
    zb_macsplit_transport_recv_byte(byte);
#endif /* !defined ZB_MACSPLIT_USE_IO_BUFFERS */
}

#endif /* ZB_PLATFORM_LINUX */

#if defined ZB_MACSPLIT_USE_IO_BUFFERS
void zb_macsplit_transport_recv_bytes(void)
{
#ifdef ZB_MACSPLIT_TRANSPORT_USERIAL
    zb_osif_userial_poll();
#endif
    while (!ZB_RING_BUFFER_IS_EMPTY(&MACSPLIT_CTX().rx_buffer))
    {
        zb_uint8_t *ptr;
        zb_uint_t   size;
        zb_uint_t   i;

        ZB_SERIAL_INT_DISABLE();
        ptr = (zb_uint8_t *)ZB_RING_BUFFER_GET_BATCH(&MACSPLIT_CTX().rx_buffer, size);
        ZB_SERIAL_INT_ENABLE();

        for (i = 0 ; i < size ; ++i)
        {
            zb_macsplit_transport_recv_byte(ptr[i]);
        }

        ZB_SERIAL_INT_DISABLE();
        ZB_RING_BUFFER_FLUSH_GET_BATCH(&MACSPLIT_CTX().rx_buffer, size);
        ZB_SERIAL_INT_ENABLE();
    }
}
#endif /* defined ZB_MACSPLIT_USE_IO_BUFFERS */

#if defined ZB_MACSPLIT_DEVICE
void zb_macsplit_transport_put_trace_bytes(zb_uint8_t *buf, zb_short_t len)
{
#if defined ZB_TRACE_OVER_MACSPLIT
    zb_short_t sz;
    sz = (len > (zb_short_t)(sizeof(MACSPLIT_CTX().trace_buffer) - MACSPLIT_CTX().trace_buffer_data) ?
          (zb_short_t)(sizeof(MACSPLIT_CTX().trace_buffer) - MACSPLIT_CTX().trace_buffer_data) : len);

    memcpy(MACSPLIT_CTX().trace_buffer + MACSPLIT_CTX().trace_buffer_data, buf, sz);

    MACSPLIT_CTX().trace_buffer_data += sz;
#endif /* defined ZB_TRACE_OVER_MACSPLIT */
}

void zb_macsplit_transport_flush_trace(void)
{
#if defined ZB_TRACE_OVER_MACSPLIT
    if (!MACSPLIT_CTX().trace_buffer_data)
    {
        return;
    }

    zb_macsplit_transport_send_trace(MACSPLIT_CTX().trace_buffer, MACSPLIT_CTX().trace_buffer_data);

    MACSPLIT_CTX().trace_buffer_data = 0;
#endif /* defined ZB_TRACE_OVER_MACSPLIT */
}

void zb_macsplit_transport_send_trace(zb_uint8_t *buf, zb_short_t len)
{

#ifdef ZB_MACSPLIT_TRANSPORT_SERIAL

    zb_osif_serial_transport_put_bytes(buf, len);

#elif defined ZB_MACSPLIT_TRANSPORT_USERIAL

    zb_osif_userial_put_bytes(buf, len);

#elif defined ZB_MACSPLIT_TRANSPORT_SPI

    zb_osif_spi_send_trace_dump(buf, len);

#else
#error correct transport is not defined
#endif

}

#endif /* ZB_MACSPLIT_DEVICE */

#endif /* ZB_MACSPLIT */
