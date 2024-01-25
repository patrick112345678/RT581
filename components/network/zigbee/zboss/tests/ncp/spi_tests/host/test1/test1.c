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

#define ZB_TRACE_FILE_ID 40123

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "ncp_transport.h"
#include "test1.h"

#define TX_BUF_SIZE 21

static uint8_t rx_buf[128];
/*static uint8_t tx_buf[] = "Obey your master 000";*/

static pthread_t send_thread_h;
/*static pthread_t receive_thread_h;*/

static void recv_cb(uint8_t *buf, uint16_t len);
static void send_cb(uint8_t params);

#ifdef TEST_SIMPLE_SEND
static void *send_thread(void *args);
#endif

static void init_cb(uint8_t param);

static void init_cb(uint8_t param)
{
    int32_t ret = 0;

    printf("SPI initialized successfully\n");

    ncp_transport_set_cb_recv_data(recv_cb);
    ncp_transport_set_cb_send_data(send_cb);
    ncp_transport_recv_data(rx_buf, SPI_MAX_SIZE);

#ifdef TEST_SIMPLE_SEND
    ret = pthread_create(&send_thread_h, NULL, send_thread, NULL);
    if (ret != 0)
    {
        perror("can't create send_thread");
        return;
    }
#endif

#if 0
    ret = pthread_create(&receive_thread_h, NULL, receive_thread, NULL);
    if (ret != 0)
    {
        perror("can't create receive_thread");
        return;
    }
#endif
}

#ifdef TEST_SIMPLE_SEND
static void *send_thread(void *args)
{
    uint32_t i = 0;
    uint8_t buf[TEST_SEND_SIZE];
    uint8_t data = 0;
#ifdef TEST_SEND_INCREMENTAL_SIZE
    uint8_t size = 2;
#else
    uint8_t size = TEST_SEND_SIZE;
#endif
    uint8_t pack_num = 0;
    uint32_t packs_cnt = 0;

    while (1)
    {
        if (packs_cnt == TEST_SIMPLE_SEND_PACKS_NUM)
        {
            printf("Sent packets: %d\n", packs_cnt);
            exit(0);
        }

        if (pack_num & 0x01)
        {
            data = TEST_STATS_ODD_BYTE;
        }
        else
        {
            data = TEST_STATS_EVEN_BYTE;
        }
        for (i = 2; i < size; i++)
        {
            buf[i] = data;
        }
        buf[0] = pack_num;
        buf[1] = size;
        pack_num++;
        packs_cnt++;

#ifdef TEST_SEND_INCREMENTAL_SIZE
        size++;
        if (size >= TEST_SEND_SIZE)
        {
            size = 2;
        }
#endif

        ncp_transport_send_data(buf, size);
        usleep(TEST_SIMPLE_SEND_PERIOD_US);
    }
    return NULL;
}
#endif

static void recv_cb(uint8_t *buf, uint16_t len)
{
    /*static uint8_t cnt = 0;*/

    /*printf("received bytes %d\n", len);*/
#ifdef TEST_STATS
    stats_add(buf, len);
#endif

#if 0
    printf("Rec %d bytes: %s\n", len, buf);
    tx_buf[TX_BUF_SIZE - 4] = (cnt / 100) % 10;
    tx_buf[TX_BUF_SIZE - 3] = (cnt / 10) % 10;
    tx_buf[TX_BUF_SIZE - 2] = cnt % 10;
    ncp_transport_send_data(tx_buf, 21);
    cnt++;
#endif

    ncp_transport_recv_data(rx_buf, SPI_MAX_SIZE);
}

static void send_cb(uint8_t params)
{
    printf("Message was sent with status %d\n", params);
#if 0
    ncp_transport_recv_data(rx_buf, 128);
#endif
}

int main(void)
{
    ncp_transport_init(init_cb);

    while (1)
    {
#ifdef TEST_STATS
        stats_print();
#endif
        usleep(TEST_STATS_PERIOD_US);
    }
    return 0;
}
