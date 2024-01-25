/**
 * Copyright (c) 2020 - 2020, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/* PURPOSE: NCP device LL test
*/

#define ZB_TRACE_FILE_ID 15943

#include "zb_common.h"
#include "zb_types.h"

#include "zb_ncp_tr.h"
#include "low_level/zbncp_tr_impl.h"

#include "ll_dev_tests.h"

#define PACKET_TO_MASTER          "HELLO from slave 000"
#define PACKET_TO_MASTER_SIZE     (21)
#define ZB_NCP_TRANSPORT_MAX_SIZE (TEST_SEND_SIZE + 2)


typedef struct
{
    zb_uint8_t pack_num;
    zb_uint8_t size;
    zb_uint8_t payload[TEST_SEND_SIZE - 2];
} test_msg_t;


static volatile zb_uint8_t cnt = 0;
static volatile zb_uint8_t send_cnt = 0;
static zb_uint8_t rx_message[ZB_NCP_TRANSPORT_MAX_SIZE];
static zbncp_memref_t rx_message_mem =
{
    .ptr = rx_message,
    .size = ZB_NCP_TRANSPORT_MAX_SIZE
};

static void test_ncp_receive_cb(zbncp_tr_cb_arg_t arg, zbncp_size_t size);
static void test_ncp_send_cb(zbncp_tr_cb_arg_t arg, zbncp_tr_send_status_t param);
static void test_ncp_init_cb(zbncp_tr_cb_arg_t arg);

static zbncp_transport_t ncpdev_transport;
static zbncp_transport_cb_t test_callbacks =
{
    .init = test_ncp_init_cb,
    .send = test_ncp_send_cb,
    .recv = test_ncp_receive_cb,
    .arg = NULL
};


#ifdef TEST_SIMPLE_SEND
static zb_uint8_t pack_num = 0;
static zb_uint8_t size = TEST_SEND_SIZE;
#endif


#ifdef TEST_SIMPLE_SEND
static void test_simple_send(void);
static void simple_send_cb(zb_uint8_t param);
static void simple_send(zb_uint8_t param);


static void test_ncp_send_cb(zbncp_tr_cb_arg_t arg, zbncp_tr_send_status_t param)
{
    ZVUNUSED(arg);
    ZVUNUSED(param);
    TRACE_MSG(TRACE_APP1, "Send status: %d", (FMT__D, param));
}

#ifdef TEST_SEND_BINARY_MSG
static zb_ret_t verify_bin_test_msg(test_msg_t *test_msg, zb_uint8_t size)
{
    zb_uint8_t data = 0;
    zb_uint8_t i = 0;

    if (test_msg->size != size)
    {
        return RET_ERROR;
    }

    data = (test_msg->pack_num & 0x01 ? TEST_STATS_ODD_BYTE : TEST_STATS_EVEN_BYTE);
    for (i = 0; i < test_msg->size - 2; i++)
    {
        if (test_msg->payload[i] != data)
        {
            return RET_ERROR;
        }
    }

    return RET_OK;
}

static void create_bin_test_msg(zb_uint8_t pack_num, test_msg_t *test_msg, zb_uint8_t size)
{
    zb_uint8_t data = 0;

    test_msg->pack_num = pack_num;
    test_msg->size = size;

    data = (pack_num & 0x01 ? TEST_STATS_ODD_BYTE : TEST_STATS_EVEN_BYTE);
    ZB_MEMSET(test_msg->payload, data, size - 2);
}

#else /* TEST_SEND_BINARY_MSG */

static void create_txt_test_msg(zb_uint8_t pack_num, test_msg_t *test_msg, zb_uint8_t size)
{
    zb_uindex_t start_idx = 0;
    zb_uint8_t temp_buf[] = PACKET_TO_MASTER;
    zb_uint8_t *p_txbuf = (zb_uint8_t *)test_msg;

    temp_buf[PACKET_TO_MASTER_SIZE - 4] = ((pack_num / 100) % 10) + '0';
    temp_buf[PACKET_TO_MASTER_SIZE - 3] = ((pack_num / 10) % 10) + '0';
    temp_buf[PACKET_TO_MASTER_SIZE - 2] = (pack_num % 10) + '0';

    ZB_MEMSET(p_txbuf, '-', size);

    start_idx = (size >= PACKET_TO_MASTER_SIZE ? 0 : (PACKET_TO_MASTER_SIZE - size));
    ZB_MEMCPY(&p_txbuf[size - (PACKET_TO_MASTER_SIZE - start_idx)],
              &temp_buf[start_idx],
              PACKET_TO_MASTER_SIZE - start_idx);
}

#endif /* TEST_SEND_BINARY_MSG */
#endif /* TEST_SIMPLE_SEND */

static void test_ncp_receive_cb(zbncp_tr_cb_arg_t arg, zbncp_size_t size)
{
    ZVUNUSED(arg);

#ifdef TEST_STATS
    stats_add(rx_message, size);
#endif /* TEST_STATS */

#ifdef TEST_SEND_BINARY_MSG
    if (verify_bin_test_msg((test_msg_t *)rx_message, size) != RET_OK)
    {
        TRACE_MSG(TRACE_APP1, "Received incorrect payload after sending %d packet", (FMT__D, pack_num));
        ZB_ABORT();
    }
    if (((test_msg_t *)rx_message)->pack_num == TEST_SIMPLE_SEND_PACKS_NUM - 1)
    {
        TRACE_MSG(TRACE_APP1, "Received packs: %d", (FMT__D, ((test_msg_t *)rx_message)->pack_num + 1));
        TRACE_MSG(TRACE_APP1, "TEST PASSED", (FMT__0));
    }
#endif /* TEST_SEND_BINARY_MSG */
    zbncp_transport_recv(&ncpdev_transport, rx_message_mem);

#ifdef TEST_SIMPLE_SEND
    simple_send_cb(0);
#endif
}


static void test_ncp_init_cb(zbncp_tr_cb_arg_t arg)
{
    ZVUNUSED(arg);
    zbncp_transport_recv(&ncpdev_transport, rx_message_mem);

#ifdef TEST_SIMPLE_SEND
    test_simple_send();
#endif /* TEST_SIMPLE_SEND */

#ifdef TEST_STATS
    ZB_SCHEDULE_ALARM(stats_print, 0,
                      ZB_MILLISECONDS_TO_BEACON_INTERVAL(TEST_STATS_PERIOD_MS));
#endif /* TEST_STATS */
}

#ifdef TEST_SIMPLE_SEND
static void simple_send_cb(zb_uint8_t param)
{
    ZVUNUSED(param);

    if (pack_num < TEST_SIMPLE_SEND_PACKS_NUM)
    {
        ZB_SCHEDULE_ALARM(simple_send, 0,
                          ZB_MILLISECONDS_TO_BEACON_INTERVAL(TEST_SIMPLE_SEND_PERIOD_MS));
    }
    else
    {
        TRACE_MSG(TRACE_APP1, "Sent packs: %d", (FMT__D, pack_num));
    }
}

static void simple_send(zb_uint8_t param)
{
    static test_msg_t msg;
    zbncp_cmemref_t mem;

    ZVUNUSED(param);

#ifdef TEST_SEND_INCREMENTAL_SIZE
    size++;
    if (size >= TEST_SEND_SIZE)
    {
        size = 5;
    }
#endif /* TEST_SEND_INCREMENTAL_SIZE */

#ifdef TEST_SEND_BINARY_MSG
    create_bin_test_msg(pack_num, &msg, size);
#else /* TEST_SEND_BINARY_MSG */
    create_txt_test_msg(pack_num, &msg, size);
#endif /* TEST_SEND_BINARY_MSG */

    mem.ptr = &msg;
    mem.size = size;
    zbncp_transport_send(&ncpdev_transport, mem);
    pack_num++;
}

static void test_simple_send(void)
{
    pack_num = 0;
    size = TEST_SEND_SIZE;
    ZB_SCHEDULE_ALARM(simple_send, 0,
                      ZB_MILLISECONDS_TO_BEACON_INTERVAL(TEST_SIMPLE_SEND_PERIOD_MS));
}

#endif /* TEST_SIMPLE_SEND */

MAIN()
{
    ARGV_UNUSED;

#if ZB_TRACE_LEVEL
    ZB_SET_TRACE_ON();
    ZB_SET_TRAF_DUMP_OFF();
#endif /* ZB_TRACE_LEVEL */

    ZB_INIT("NCP device transport test");

    zbncp_transport_construct(&ncpdev_transport, ncp_dev_transport_create());
    zbncp_transport_init(&ncpdev_transport, &test_callbacks);

    zboss_main_loop();

    TRACE_DEINIT();
    MAIN_RETURN(0);
}

void zboss_signal_handler(zb_uint8_t param)
{
    if (param)
    {
        zb_buf_free(param);
    }

}
