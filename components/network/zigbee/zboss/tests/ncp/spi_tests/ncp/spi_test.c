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

#define ZB_TRACE_FILE_ID 41677

#include "zb_common.h"
#include "zb_types.h"

#include "zb_ncp_tr.h"

#include "spi_test.h"

#define PACKET_TO_MASTER      "HELLO from slave 000"
#define PACKET_TO_MASTER_SIZE (21)
#define ZB_NCP_TRANSPORT_MAX_SIZE 256

static volatile zb_uint8_t cnt = 0;
static volatile zb_uint8_t send_cnt = 0;
static zb_uint8_t tx_message[128] = {PACKET_TO_MASTER};
static zb_uint8_t rx_message[ZB_NCP_TRANSPORT_MAX_SIZE];
static zbncp_memref_t rx_message_mem = {
  .ptr = rx_message,
  .size = ZB_NCP_TRANSPORT_MAX_SIZE
};

static void test_ncp_receive_cb(zbncp_tr_cb_arg_t arg, zbncp_size_t size);
static void test_ncp_send_cb(zbncp_tr_cb_arg_t arg, zbncp_tr_send_status_t param);
static void test_ncp_init_cb(zbncp_tr_cb_arg_t arg);

static const zbncp_transport_ops_t * spi_transport = NULL;
static zbncp_transport_cb_t test_callbacks = {
  .init = test_ncp_init_cb,
  .send = test_ncp_send_cb,
  .recv = test_ncp_receive_cb,
  .arg = NULL
};

#ifdef TEST_SIMPLE_SEND
static void simple_send_cb(zb_uint8_t param);
#endif

#if 0
static inline test_send_message(void)
{
  zbncp_cmemref_t tx_mem;
  static zb_uint8_t cnt_max = 0;
  zb_uint8_t temp_buf[] = PACKET_TO_MASTER;
  zb_uindex_t i = 0;

  for(i = 0; i < cnt_max; i++)
  {
    tx_message[i] = '-';
  }
  for(i = cnt_max; i < cnt_max+PACKET_TO_MASTER_SIZE; i++)
  {
    tx_message[i] = temp_buf[i-cnt_max];
  }
  tx_message[cnt_max+PACKET_TO_MASTER_SIZE-4] = ((send_cnt/100) % 10) + '0';
  tx_message[cnt_max+PACKET_TO_MASTER_SIZE-3] = ((send_cnt/10) % 10) + '0';
  tx_message[cnt_max+PACKET_TO_MASTER_SIZE-2] = (send_cnt % 10) + '0';

  tx_mem.ptr = tx_message;
  //tx_mem.size = sizeof(tx_message));
  tx_mem.size = cnt_max + PACKET_TO_MASTER_SIZE;
  spi_transport->send(spi_transport->impl, tx_mem);

  send_cnt++;
  if (cnt_max >= 100)
  {
    cnt_max = 0;
  }
  else
  {
    cnt_max++;
  }
}
#endif

static void test_ncp_receive_cb(zbncp_tr_cb_arg_t arg, zbncp_size_t size)
{
#ifdef TEST_STATS
  stats_add(rx_message, size);
#endif  

  spi_transport->recv(spi_transport->impl, rx_message_mem);
}

static void test_ncp_send_cb(zbncp_tr_cb_arg_t arg, zbncp_tr_send_status_t param)
{
  TRACE_MSG(TRACE_APP1, "Send status: %d", (FMT__D, param));
}

static void test_ncp_init_cb(zbncp_tr_cb_arg_t arg)
{
  spi_transport->recv(spi_transport->impl, rx_message_mem);
#ifdef TEST_SIMPLE_SEND  
  ZB_SCHEDULE_ALARM(simple_send_cb, 0,
                        ZB_MILLISECONDS_TO_BEACON_INTERVAL(TEST_SIMPLE_SEND_PERIOD_MS));
#endif                

#ifdef TEST_STATS
  ZB_SCHEDULE_ALARM(stats_print, 0,
                        ZB_MILLISECONDS_TO_BEACON_INTERVAL(TEST_STATS_PERIOD_MS));
#endif 
}

#ifdef TEST_SIMPLE_SEND  
static void simple_send_cb(zb_uint8_t param)
{
  zbncp_cmemref_t tx_mem;
  zb_uindex_t i = 0;
  zb_uint8_t buf[TEST_SEND_SIZE];
  zb_uint8_t data = 0;
#ifdef TEST_SEND_INCREMENTAL_SIZE
  static zb_uint8_t size = 2;
#else
  zb_uint8_t size = TEST_SEND_SIZE;
#endif    
  static zb_uint8_t pack_num = 0;
  static zb_uint32_t packs_cnt = 0;

  if (pack_num & 0x01)
  {
    data = TEST_STATS_ODD_BYTE;
  }
  else 
  {
    data = TEST_STATS_EVEN_BYTE;
  }
  for(i = 2; i < size; i++)
  {
    buf[i] = data;
  }
  buf[0] = pack_num;
  buf[1] = size;
  pack_num++;
  packs_cnt++;

  tx_mem.ptr = buf;
  tx_mem.size = size;
  spi_transport->send(spi_transport->impl, tx_mem);

#ifdef TEST_SEND_INCREMENTAL_SIZE     
    size++;
    if (size >= TEST_SEND_SIZE)
    {
      size = 2;
    }
#endif

  if (packs_cnt < TEST_SIMPLE_SEND_PACKS_NUM)
  {
    ZB_SCHEDULE_ALARM(simple_send_cb, 0,
                        ZB_MILLISECONDS_TO_BEACON_INTERVAL(TEST_SIMPLE_SEND_PERIOD_MS));
  }
  else
  {
    TRACE_MSG(TRACE_APP1, "Sent packs: %d", (FMT__D, packs_cnt));
  }
}
#endif   

MAIN()
{
  ARGV_UNUSED;

  ZB_SET_TRACE_ON();
  ZB_SET_TRAF_DUMP_OFF();

  ZB_INIT("spi test");

  spi_transport = ncp_dev_transport_create();
  spi_transport->init(spi_transport->impl, &test_callbacks);

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
