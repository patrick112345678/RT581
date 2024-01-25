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
/*  PURPOSE: Linux SPI access implementation.
*/

#define ZB_TRACE_FILE_ID 34
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "gpio_serve.h"
#include "linux_spi_internal.h"
#include "linux_spi_internal_types.h"
#include "low_level/zbncp_ll_pkt.h"

/* Time enough for Slave to set its SPI TX buffer */
#define SPI_USEC_BETWEEN_RX 100

//#define SPI_DEBUG
static void dump_data(const char *str, uint8_t *buf, size_t len);

static uint8_t mode = SPI_TRANSPORT_MODE;
#ifdef TEST_BITRATE
static uint32_t speed = TEST_BITRATE_VALUE;
#else
static uint32_t speed = SPI_TRANSPORT_BIT_RATE;
#endif
static uint8_t bits = 8;
static uint16_t delay = 0;

static spi_init_state_t spi_init_state = SPI_NOT_INITIALIZED;

static pthread_t send_thread_h;
static pthread_t receive_thread_h;
static pthread_mutex_t spi_mutex;
static pthread_mutex_t tx_ctx_mutex;
static pthread_mutex_t rx_ctx_mutex;
static sem_t *send_sem;

static linux_spi_ctx_t linux_spi_ctx;
static rx_spi_ctx_t rx_ctx;
static tx_spi_ctx_t tx_ctx;

static void *send_thread(void *args);
static void *receive_thread(void *args);
static int transfer(int fd, const uint8_t *tx_buf, size_t tx_len,
                    uint8_t *rx_buf, size_t rx_len);

static void spi_send_data_to_user(void);

static int32_t rx_ctx_init(void);
static void rx_swap_drv_bufs(void);
static void rx_swap_user_bufs(void);

static void host_interrupt_handler(void);

#ifdef TEST_WRONG_LEN
static uint8_t test_wrong_len(void);
#endif
#ifdef TEST_CORRUPTED_PACKET
static void test_corrupted_packet(void);
#endif

void linux_spi_init(spi_callback_t cb)
{
    int32_t spi_dev_fd = 0;
    int32_t ret = 0;

    if (spi_init_state == SPI_INITIALIZED)
    {
        return;
    }

    spi_init_state = SPI_INITIALIZED;

    spi_dev_fd = open(SPI_DEVICE, O_RDWR);
    if (spi_dev_fd < 0)
    {
        printf("%s\n", SPI_DEVICE);
        perror("can't open spi device");
        return;
    }

    /********************SPIDEV CONFIGURATION******************/
    ret = ioctl(spi_dev_fd, SPI_IOC_WR_MODE, &mode);
    if (-1 == ret)
    {
        perror("can't set spi mode");
        return;
    }
    ret = ioctl(spi_dev_fd, SPI_IOC_RD_MODE, &mode);
    if (-1 == ret)
    {
        perror("can't set spi mode");
        return;
    }

    ret = ioctl(spi_dev_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (-1 == ret)
    {
        perror("can't set bits per word");
        return;
    }
    ret = ioctl(spi_dev_fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
    if (-1 == ret)
    {
        perror("can't set bits per word");
        return;
    }

    ret = ioctl(spi_dev_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    if (-1 == ret)
    {
        perror("can't set max speed hz");
        return;
    }
    ret = ioctl(spi_dev_fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
    if (-1 == ret)
    {
        perror("can't set max speed hz");
        return;
    }

    /*****************SEMAPHORE********************************/
    send_sem = sem_open(SPI_SEND_DATA_SEM, O_CREAT, 0777, 0);
    if (send_sem == SEM_FAILED)
    {
        perror("can't init send_sem");
        return;
    }

    /*****************THREADS**********************************/
    ret = pthread_create(&send_thread_h, NULL, send_thread, NULL);
    if (ret != 0)
    {
        perror("can't create send_thread");
        return;
    }
    ret = pthread_create(&receive_thread_h, NULL, receive_thread, NULL);
    if (ret != 0)
    {
        perror("can't create receive_thread");
        return;
    }

    /*****************MUTEX*************************************/
    ret = pthread_mutex_init(&spi_mutex, NULL);
    if (ret != 0)
    {
        perror("can't create spi_mutex");
        return;
    }
    ret = pthread_mutex_init(&tx_ctx_mutex, NULL);
    if (ret != 0)
    {
        perror("can't create tx_ctx_mutex");
        return;
    }
    ret = pthread_mutex_init(&rx_ctx_mutex, NULL);
    if (ret != 0)
    {
        perror("can't create rx_ctx_mutex");
        return;
    }

    /********************GPIO*********************************/
    gpio_in_init(SPI_HOST_INT_PIN, host_interrupt_handler);

    gpio_out_init(SPI_RESET_PIN, SPI_RESET_DISABLED);

    if (pipe(ctrl_pipe_fd) < 0)
    {
        perror("can't create control pipe");
        return;
    }

    /*******************RX_CTX********************************/
    rx_ctx_init();

    linux_spi_ctx.spi_fd = spi_dev_fd;
    if (cb != NULL)
    {
        cb(0);
    }
}

void linux_spi_recv_data(uint8_t *buf, uint16_t len)
{
    recv_obj_t *recv_obj_ = NULL;

    if ((NULL == buf) || (0 == len))
    {
        printf("linux_spi_recv_data: check (NULL == buf) || (0 == len) failed\n");
        return;
    }

    if (spi_init_state != SPI_INITIALIZED)
    {
        printf("SPI not initialized yet\n");
        return;
    }
    if (!rx_ctx.is_user_requested)
    {
        rx_ctx.is_user_requested = SPI_TRUE;

        rx_ctx.user_buf = buf;
        rx_ctx.user_len = len;
        rx_ctx.user_offset = 0;

        recv_obj_ = &rx_ctx.recv_obj[rx_ctx.user_index];

        if (recv_obj_->remain)
        {
            spi_send_data_to_user();
        }
    }
}

void linux_spi_set_cb_recv_data(spi_recv_data_cb_t cb)
{
    linux_spi_ctx.recv_cb = cb;
}

void linux_spi_send_data(uint8_t *buf, uint16_t len)
{
    int32_t ret = 0;

    if (spi_init_state != SPI_INITIALIZED)
    {
        return;
    }

    /* don't want to block caller, better invoke callback with BUSY status
     */
    ret = pthread_mutex_trylock(&tx_ctx_mutex);
    if (0 == ret)
    {
        tx_ctx.buf = buf;
        tx_ctx.len = len;
        sem_post(send_sem);

        ret = pthread_mutex_unlock(&tx_ctx_mutex);
        if (ret != 0)
        {
            perror("linux_spi_send_data: pthread_mutex_unlock(&tx_ctx) error");
            return;
        }
    }
    else
    {
        if (linux_spi_ctx.send_cb != NULL)
        {
            linux_spi_ctx.send_cb(SPI_BUSY);
        }
    }
}

void linux_spi_set_cb_send_data(spi_callback_t cb)
{
    linux_spi_ctx.send_cb = cb;
}

static int32_t rx_ctx_init(void)
{
    int32_t i = 0;

    for (i = 0; i < SPI_RX_OBJS; i++)
    {
        rx_ctx.recv_obj[i].buf = (uint8_t * )malloc(SPI_MAX_SIZE);
        if (NULL == rx_ctx.recv_obj[i].buf)
        {
            printf("cannot allocate memory for rx_ctx.recv_obj\n");
            return -1;
        }
        rx_ctx.recv_obj[i].offset = 0;
        rx_ctx.recv_obj[i].remain = 0;
    }
    rx_ctx.drv_index = 0;
    rx_ctx.user_buf = NULL;
    rx_ctx.user_len = 0;
    rx_ctx.user_index = 0;
    rx_ctx.is_user_requested = SPI_FALSE;
    rx_ctx.empty_obj_cnt = SPI_RX_OBJS;

    return 0;
}

static void *send_thread(void *args)
{
    int32_t ret = 0;
    int32_t transfer_result = 0;

    (void)args;

    while (SPI_TRUE)
    {
        sem_wait(send_sem);
        ret = pthread_mutex_lock(&tx_ctx_mutex);
        if (ret != 0)
        {
            perror("send_thread: pthread_mutex_lock(&tx_ctx) error");
            return 0;
        }

        ret = pthread_mutex_lock(&spi_mutex);
        if (ret != 0)
        {
            perror("send_thread: pthread_mutex_lock(&spi_mutex) error");
            return 0;
        }

#ifdef TEST_CORRUPTED_PACKET
        test_corrupted_packet();
#endif

        dump_data("SEND DATA", tx_ctx.buf, tx_ctx.len);

        transfer_result = transfer(linux_spi_ctx.spi_fd, tx_ctx.buf,
                                   tx_ctx.len, NULL, 0);

        ret = pthread_mutex_unlock(&spi_mutex);
        if (ret != 0)
        {
            perror("send_thread: pthread_mutex_unlock(&spi_mutex) error");
            return 0;
        }
        /* free tx_ctx */
        tx_ctx.buf = NULL;
        tx_ctx.len = 0;
        ret = pthread_mutex_unlock(&tx_ctx_mutex);
        if (ret != 0)
        {
            perror("send_thread: pthread_mutex_unlock(&tx_ctx) error");
            return 0;
        }
        if (linux_spi_ctx.send_cb != NULL)
        {
            if (transfer_result < 0)
            {
                linux_spi_ctx.send_cb(SPI_FAIL);
            }
            else
            {
                linux_spi_ctx.send_cb(SPI_SUCCESS);
            }
        }
    }
}

static void *receive_thread(void *args)
{
    (void)args;

    while (SPI_TRUE)
    {
        check_select(SPI_TRUE);
    }

    return NULL;
}

static int transfer(int fd, const uint8_t *tx_buf, size_t tx_len,
                    uint8_t *rx_buf, size_t rx_len)
{
    int ret = 0;
    struct spi_ioc_transfer tr = {0};

    tr.tx_buf = (uintptr_t) tx_buf;
    tr.rx_buf = (uintptr_t) rx_buf;
    tr.len = (uint32_t)(tx_len != 0u ? tx_len : rx_len);
    tr.delay_usecs = delay;
    tr.speed_hz = speed;
    tr.bits_per_word = bits;

    ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 0)
    {
        perror("transfer: ioctl error");
    }

    return ret;
}

static void host_interrupt_handler(void)
{
    int32_t ret = 0;
    uint8_t *recv_buf = NULL;
    uint16_t len = 0;

    ret = pthread_mutex_lock(&spi_mutex);
    if (ret != 0)
    {
        perror("host_interrupt_handler: pthread_mutex_lock(&spi_mutex) error");
        return;
    }

    if (rx_ctx.empty_obj_cnt)
    {
        /* At Slave put entire packet into DMA buffer.  Master
           first reads its NCP header to define packet length and pot ~CS high causent end of tx at Slave.
           Master checks packet length and, if packet has Data section, read packet rest in the next transaction (~CS low).
           If packet is ACK/NACK having no Data section, single read transaction is used.
         */
        zbncp_ll_pkt_wo_body_t pkt_hdr;
        zbncp_uint8_t crc;
        ret = transfer(linux_spi_ctx.spi_fd, NULL, 0, (uint8_t *)&pkt_hdr, sizeof(pkt_hdr));
        dump_data("GET HDR", (uint8_t *)&pkt_hdr, sizeof(pkt_hdr));
        crc = zbncp_ll_hdr_calc_crc(&pkt_hdr.hdr);
        if (ret == sizeof(pkt_hdr)
                /* Skipping signature check here. Hope crc check is enough. */
                && pkt_hdr.hdr.crc == crc)
        {
            /* TODO: remove signature? */
            len = pkt_hdr.hdr.len + sizeof(pkt_hdr.sign);
        }
        else
        {

        }

        if ((len <= SPI_MAX_SIZE) && len)
        {
            recv_buf = rx_ctx.recv_obj[rx_ctx.drv_index].buf;
            /* Header is received. Do not need to receive it once
             * again. Slave sends packet rest */
            memcpy(recv_buf, &pkt_hdr, sizeof(pkt_hdr));

            if (len > sizeof(pkt_hdr))
            {
                /* have Data section - receive it from Slave */
                usleep(SPI_USEC_BETWEEN_RX);
                ret = transfer(linux_spi_ctx.spi_fd, NULL, 0, recv_buf + sizeof(pkt_hdr), len);
            }
            else
            {

            }
            rx_ctx.recv_obj[rx_ctx.drv_index].remain = len;

            dump_data("GET DATA", recv_buf, len);

            rx_swap_drv_bufs();

            if (rx_ctx.is_user_requested)
            {
                spi_send_data_to_user();
            }
        }
        else
        {
            printf("received len > SPI_MAX_SIZE or len == 0: %d\n", len);
        }
    }
    else
    {
#ifdef SPI_DEBUG
        printf("===========there is no free space in RX buffer\n");
#endif
    }
    ret = pthread_mutex_unlock(&spi_mutex);
    if (ret != 0)
    {
        perror("send_thread: pthread_mutex_unlock(&spi_mutex) error");
        return;
    }
}

static void rx_swap_drv_bufs(void)
{
    rx_ctx.empty_obj_cnt--;
    if ((rx_ctx.drv_index + 1) < SPI_RX_OBJS)
    {
        rx_ctx.drv_index++;
    }
    else
    {
        rx_ctx.drv_index = 0;
    }
}

static void rx_swap_user_bufs(void)
{
    recv_obj_t *recv_obj_ = NULL;

    recv_obj_ = &rx_ctx.recv_obj[rx_ctx.user_index];

    recv_obj_->remain = 0;
    recv_obj_->offset = 0;
    memset(recv_obj_->buf, 0, SPI_MAX_SIZE);

    rx_ctx.empty_obj_cnt++;

    if ((rx_ctx.user_index + 1) < SPI_RX_OBJS)
    {
        rx_ctx.user_index++;
    }
    else
    {
        rx_ctx.user_index = 0;
    }
}

static void spi_send_data_to_user(void)
{
    int32_t ret = 0;
    recv_obj_t *recv_obj_ = NULL;
    size_t remain_to_req_len = 0;

    ret = pthread_mutex_lock(&rx_ctx_mutex);
    if (ret != 0)
    {
        perror("spi_send_data_to_user: pthread_mutex_lock(&rx_ctx) error");
        return;
    }
    if (rx_ctx.is_user_requested)
    {

#ifdef SPI_DEBUG
        printf("===========USER REQUESTS: len = %d, buf = %p\n", rx_ctx.user_len, rx_ctx.user_buf);
#endif

        remain_to_req_len = rx_ctx.user_len - rx_ctx.user_offset;

        recv_obj_ = &rx_ctx.recv_obj[rx_ctx.user_index];

        if (recv_obj_->remain > remain_to_req_len)
        {
            memcpy(rx_ctx.user_buf + rx_ctx.user_offset, &recv_obj_->buf[recv_obj_->offset],
                   remain_to_req_len);
            recv_obj_->offset += remain_to_req_len;
            recv_obj_->remain -= remain_to_req_len;

            dump_data("COPY TO USER1", rx_ctx.user_buf, remain_to_req_len);

            rx_ctx.is_user_requested = SPI_FALSE;
            if (linux_spi_ctx.recv_cb != NULL)
            {
                linux_spi_ctx.recv_cb(rx_ctx.user_buf, (uint16_t)rx_ctx.user_len);
            }
        }
        else if (recv_obj_->remain == rx_ctx.user_len)
        {
            memcpy(rx_ctx.user_buf + rx_ctx.user_offset, &recv_obj_->buf[recv_obj_->offset],
                   remain_to_req_len);
            rx_swap_user_bufs();

            dump_data("COPY TO USER2", rx_ctx.user_buf, remain_to_req_len);

            rx_ctx.is_user_requested = SPI_FALSE;
            if (linux_spi_ctx.recv_cb != NULL)
            {
                linux_spi_ctx.recv_cb(rx_ctx.user_buf, (uint16_t)rx_ctx.user_len);
            }
        }
        else
        {
            memcpy(rx_ctx.user_buf + rx_ctx.user_offset, &recv_obj_->buf[recv_obj_->offset],
                   recv_obj_->remain);

            dump_data("COPY TO BUF3", rx_ctx.user_buf, recv_obj_->remain);

            rx_ctx.user_offset += recv_obj_->remain;
            rx_swap_user_bufs();
        }

    }
    ret = pthread_mutex_unlock(&rx_ctx_mutex);
    if (ret != 0)
    {
        perror("spi_send_data_to_user: pthread_mutex_unlock(&rx_ctx) error");
        return;
    }
}

void linux_spi_reset_ncp(void)
{
    ssize_t ret;

    ret = gpio_write(SPI_RESET_PIN, SPI_RESET_DISABLED);
    if (ret < 0)
    {
        perror("can't write reset pin");
    }
    usleep(1);
    ret = gpio_write(SPI_RESET_PIN, SPI_RESET_ACTIVE);
    if (ret < 0)
    {
        perror("can't write reset pin");
    }
    usleep(1);
    ret = gpio_write(SPI_RESET_PIN, SPI_RESET_DISABLED);
    if (ret < 0)
    {
        perror("can't write reset pin");
    }
}

#ifdef TEST_WRITE_WHEN_HOST_INT
static uint8_t tx_buf[TEST_WRITE_WHEN_HOST_INT_SIZE];

void test_simultaneous_write(void);

void test_simultaneous_write(void)
{
    uint32_t test_i = 0;

    for (test_i = 0; test_i < TEST_WRITE_WHEN_HOST_INT_SIZE; test_i++)
    {
        tx_buf[test_i] = 0xBF;
    }

    linux_spi_send_data(tx_buf, TEST_WRITE_WHEN_HOST_INT_SIZE);
}
#endif

#ifdef TEST_WRONG_LEN
static uint8_t test_wrong_len(void)
{
    static uint32_t test_cnt = 0;

    if (test_cnt == TEST_WRONG_LEN_PERIOD)
    {
        test_cnt = 0; /* imitate wrong len */
        return 1;
    }
    else
    {
        test_cnt++;
        return 0;
    }
}
#endif

#ifdef TEST_CORRUPTED_PACKET
static void test_corrupted_packet(void)
{
    static uint32_t test_cnt = 0;
    uint32_t test_i = 0;

    if (test_cnt == TEST_CORRUPTED_PACKET_PERIOD)
    {
        test_cnt = 0;
        for (test_i = 0; test_i < tx_ctx.len; test_i++)
        {
            if (test_i & 0x01)
            {
                tx_ctx.buf[test_i] = TEST_CORRUPTED_PACKET_ODD_BYTE;
            }
            else
            {
                tx_ctx.buf[test_i] = TEST_CORRUPTED_PACKET_EVEN_BYTE;
            }
        }
    }
    else
    {
        test_cnt++;
    }
}
#endif

static void dump_data(const char *str, uint8_t *buf, size_t len)
{
#ifdef SPI_DEBUG
    size_t test_cnt = 0;
    printf("===========%s: ", str);
    for (test_cnt = 0; test_cnt < len; test_cnt++)
    {
        printf("%x ", buf[test_cnt]);
    }
    printf("\n");
#else
    (void)str;
    (void)buf;
    (void)len;
#endif
}
