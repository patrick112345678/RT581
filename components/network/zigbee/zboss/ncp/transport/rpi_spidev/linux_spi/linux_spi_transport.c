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
#define ZB_TRACE_FILE_ID 14011
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

#include "linux_spi_internal.h"
#include "gpio_serve.h"
#include "linux_spi_internal_types.h"
#include "zbncp_ll_pkt.h"
#include "ncp_common.h"

/* Time enough for Slave to set its SPI TX buffer */
#define SPI_USEC_BETWEEN_RX 100
//#define SPI_USEC_BETWEEN_RX 10
#define NCP_LL_PKT_HAVE_SIGNATURE

#define NCP_LL_CS_DELAY_DATA_START

#ifdef NCP_LL_CS_DELAY_DATA_START
#define NCP_LL_CS_DELAY_US  (800u)
#endif


static void dump_data(const char * str, uint8_t *buf, size_t len);

static uint8_t mode = SPI_TRANSPORT_MODE;
#ifdef TEST_BITRATE
static uint32_t speed = TEST_BITRATE_VALUE;
#else
static uint32_t speed = SPI_TRANSPORT_BIT_RATE;
#endif
static uint8_t bits = 8;
static uint16_t delay = 0;

static spi_init_state_t s_spi_init_state = SPI_NOT_INITIALIZED;

static pthread_t s_io_thread_h;
static pthread_t s_intr_thread_h;
static pthread_mutex_t s_spi_mutex;
static pthread_cond_t s_spi_cond;

static linux_spi_ctx_t linux_spi_ctx;
static rx_spi_ctx_t rx_ctx;
static tx_spi_ctx_t tx_ctx;
static int s_slave_intr_counter;

static void *io_thread(void *args);
static void *intr_thread(void *args);
static int spi_transfer(int fd, const uint8_t *tx_buf, size_t tx_len,
                        uint8_t *rx_buf, size_t rx_len);

static int spi_send_data_to_user(void);
static void call_rx_cb(void);

static int32_t rx_ctx_init(void);
static void rx_swap_drv_bufs(void);
static void rx_swap_user_bufs(void);

static void host_interrupt_handler(void);
static void spi_do_send(uint8_t *tx_buf, size_t tx_len);
static void spi_do_recv(void);

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

  if (s_spi_init_state == SPI_INITIALIZED)
  {
    return;
  }
  s_spi_init_state = SPI_INITIALIZED;

  spi_dev_fd = open(SPI_DEVICE, O_RDWR);
  if (spi_dev_fd < 0)
  {
    TRACE_MSG(TRACE_ERROR, "can't open spi device %d", (FMT__D, errno));
    return;
  }

#ifdef SPI_MANUAL_CS
  /* Manual chip select if don't want to put CS High between NCP hdr and body recv. */
  /* Note: manial CS is not compatible with pre-defined RESET GPIO pin names (all our non-RPI setups).
     So, currently manual CS is for RPi only */
  mode |= SPI_NO_CS;
  gpio_out_init(SPI_CS_PIN, 1);
#endif
  /********************SPIDEV CONFIGURATION******************/
  ret = ioctl(spi_dev_fd, SPI_IOC_WR_MODE, &mode);
  if (-1 == ret)
  {
    TRACE_MSG(TRACE_ERROR, "can't set spi mode %d", (FMT__D, errno));
    return;
  }
  ret = ioctl(spi_dev_fd, SPI_IOC_RD_MODE, &mode);
  if (-1 == ret)
  {
    TRACE_MSG(TRACE_ERROR, "can't set spi mode %d", (FMT__D, errno));
    return;
  }

  ret = ioctl(spi_dev_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
  if (-1 == ret)
  {
    TRACE_MSG(TRACE_ERROR, "can't set bits per word %d", (FMT__D, errno));
    return;
  }
  ret = ioctl(spi_dev_fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
  if (-1 == ret)
  {
    TRACE_MSG(TRACE_ERROR, "can't set bits per word %d", (FMT__D, errno));
    return;
  }

  ret = ioctl(spi_dev_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
  if (-1 == ret)
  {
    TRACE_MSG(TRACE_ERROR, "can't set max speed %d", (FMT__D, errno));
    return;
  }
  ret = ioctl(spi_dev_fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
  if (-1 == ret)
  {
    TRACE_MSG(TRACE_ERROR, "can't set max speed %d", (FMT__D, errno));
    return;
  }

  /*****************MUTEX*************************************/
  ret = pthread_mutex_init(&s_spi_mutex, NULL);
  if (ret != 0)
  {
    TRACE_MSG(TRACE_ERROR, "can't create mutex %d", (FMT__D, errno));
    return;
  }
  ret = pthread_cond_init(&s_spi_cond, NULL);
  if (ret != 0)
  {
    TRACE_MSG(TRACE_ERROR, "can't create cond %d", (FMT__D, errno));
    return;
  }

  /*****************THREADS**********************************/
  ret = pthread_create(&s_io_thread_h, NULL, io_thread, NULL);
  if (ret != 0)
  {
    TRACE_MSG(TRACE_ERROR, "can't create io thread %d", (FMT__D, errno));
    return;
  }
  ret = pthread_create(&s_intr_thread_h, NULL, intr_thread, NULL);
  if (ret != 0)
  {
    TRACE_MSG(TRACE_ERROR, "can't create intr thread %d", (FMT__D, errno));
    return;
  }


  /********************GPIO*********************************/
  gpio_in_init(SPI_HOST_INT_PIN, host_interrupt_handler);

  gpio_out_init(SPI_RESET_PIN, SPI_RESET_DISABLED);

  /*******************RX_CTX********************************/
  rx_ctx_init();

  linux_spi_ctx.spi_fd = spi_dev_fd;
  if (cb != NULL)
  {
    /* call cb on init complete */
    cb(0);
  }
}


#ifdef SPI_MANUAL_CS
static void spi_cs_high()
{
  gpio_write(SPI_CS_PIN, 1);
}

static void spi_cs_low()
{
  gpio_write(SPI_CS_PIN, 0);
}
#endif


/**
   Entry point: provide a buffer for data rx.
 */
void linux_spi_recv_data(uint8_t *buf, uint16_t len)
{
  int call_cb = 0;
  ZB_ASSERT((NULL != buf) && (0 != len));
  ZB_ASSERT(s_spi_init_state == SPI_INITIALIZED);

  pthread_mutex_lock(&s_spi_mutex);
  ZB_ASSERT(!rx_ctx.is_user_requested);
  rx_ctx.user_buf = buf;
  rx_ctx.user_len = len;
  rx_ctx.user_offset = 0;
  rx_ctx.is_user_requested = SPI_TRUE;

  if (rx_ctx.recv_obj[rx_ctx.user_index].remain)
  {
    TRACE_MSG(TRACE_USB3, "linux_spi_recv_data %p %d - rx to user immediately", (FMT__P_D, buf, len));
    call_cb = spi_send_data_to_user();
  }
  else
  {
    TRACE_MSG(TRACE_USB3, "linux_spi_recv_data %p %d - wait", (FMT__P_D, buf, len));
  }
  pthread_mutex_unlock(&s_spi_mutex);
  /* Call upper level callback not holding mutex */
  if(call_cb)
  {
    call_rx_cb();
  }
}


void linux_spi_set_cb_recv_data(spi_recv_data_cb_t cb)
{
  TRACE_MSG(TRACE_USB3, "linux_spi_set_cb_recv_data %p", (FMT__P, cb));
  linux_spi_ctx.recv_cb = cb;
}


/**
   Entry point of the whole transport level.

   Called from ncp_tr_spidev_send()
 */
void linux_spi_send_data(uint8_t *buf, uint16_t len)
{
  int ret = 0;

  pthread_mutex_lock(&s_spi_mutex);

  if (s_spi_init_state != SPI_INITIALIZED)
  {
    ret = SPI_FAIL;
  }
  if (ret == 0 && tx_ctx.len != 0)
  {
    TRACE_MSG(TRACE_ERROR, "oops: call to linux_spi_send_data(%p, %d) when tx is not done - len %d", (FMT__P_D_D, buf, len, tx_ctx.len));
    ret = SPI_BUSY;
    /* WARNING: this code removes the previous packet and it won't be sent!
       This works only in conjunction with the NCP proto which has retransmission logic.
       It is needed for the following reason:
       Under high load a host can be so slow that it is not able to send a packet over SPI in time,
       NCP tries to retransmit a packet, gets SPI_BUSY status and tries to retransmit again without any delay:
       it is effectively a busy loop.
       If we clear tx_ctx here, NCP will not get an error status on an every other retransmission attempt
       so there is no busy loop anymore.
    */
    tx_ctx.buf = 0;
    tx_ctx.len = 0;
  }
  if (0 == ret)
  {
    tx_ctx.buf = buf;
    tx_ctx.len = len;
    TRACE_MSG(TRACE_USB3, "linux_spi_send_data %p %d - wake io", (FMT__P_D, buf, len));
    pthread_cond_signal(&s_spi_cond);
  }
  else
  {
    TRACE_MSG(TRACE_USB3, "linux_spi_send_data %p %d ret %d", (FMT__P_D_D, buf, len));
  }

  pthread_mutex_unlock(&s_spi_mutex);

  if (ret != 0
      && linux_spi_ctx.send_cb != NULL)
  {
    linux_spi_ctx.send_cb((uint8_t)ret);
  }
}

void linux_spi_set_cb_send_data(spi_callback_t cb)
{
  /* usuallu ncp/zb_ncp_tr_spidev.c:linux_spi_send_complete() */
  linux_spi_ctx.send_cb = cb;
}

static int32_t rx_ctx_init(void)
{
  int32_t i = 0;

  for(i = 0; i < SPI_RX_OBJS; i++)
  {
    rx_ctx.recv_obj[i].buf = (uint8_t* )malloc(SPI_MAX_SIZE);
    if (NULL == rx_ctx.recv_obj[i].buf)
    {
      TRACE_MSG(TRACE_ERROR, "cannot allocate memory for rx_ctx.recv_obj %d", (FMT__D, errno));
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

/**
   SPI I/o thread.

   sleeps on a condvar.
   Can be woken up either by linux_spi_send_data() when need tx or by
   intr_thread()->check_select()->host_interrupt_handler() when git Slave intr so have smth to recv.

   Do recv or send or both.
 */
static void *io_thread(void *args)
{
  int need_rx;
  int can_pass_to_user;
  size_t tx_len;
  uint8_t *tx_buf;

  (void)args;
  TRACE_MSG(TRACE_USB1, "io_thread started", (FMT__0));

  while (1)
  {
    pthread_mutex_lock(&s_spi_mutex);
    while (1)
    {
      need_rx = s_slave_intr_counter;
      if (s_slave_intr_counter)
      {
        s_slave_intr_counter--;
      }
      tx_len = tx_ctx.len;
      tx_buf = tx_ctx.buf;

      can_pass_to_user = (rx_ctx.recv_obj[rx_ctx.user_index].remain && rx_ctx.is_user_requested);
      TRACE_MSG(TRACE_USB3, "io_thread need_rx %d tx_len %d can_pass_to_user %d", (FMT__D_D_D, need_rx, tx_len, can_pass_to_user));
      if (need_rx
          || tx_len
          || can_pass_to_user)
      {
        /* BREAK LOOP */
        break;
      }
      pthread_cond_wait(&s_spi_cond, &s_spi_mutex);
      TRACE_MSG(TRACE_USB3, "io_thread ret from cond_wait", (FMT__0));
    }
    pthread_mutex_unlock(&s_spi_mutex);


    /* If still have something received to pass to user, do it now. */
    if (can_pass_to_user)
    {
      TRACE_MSG(TRACE_USB3, "io_thread: can pass smth to user", (FMT__0));
      /* fill buf and call linux_spi_ctx.recv_cb - tr->cb.send(tr->cb.arg, status) - zbncp_ll_recv_complete - ncp_ll_call_me */
      /* TODO: in ncp_ll_call_me write to pipe only if we are now waiting inide poll. */
      if (spi_send_data_to_user())
      {
        call_rx_cb();
      }
    }
    if (need_rx)
    {
      TRACE_MSG(TRACE_USB3, "io_thread: Slave intr - do rx; pending intr cnt %d", (FMT__D, s_slave_intr_counter));
      spi_do_recv();
    }
    if (tx_len)
    {
      TRACE_MSG(TRACE_USB3, "io_thread: Have smth to send - do tx len %d", (FMT__D, tx_len));
      spi_do_send(tx_buf, tx_len);
    }
  }
  return NULL;
}


static void spi_do_send(uint8_t *tx_buf, size_t tx_len)
{
  int transfer_result;

#ifdef TEST_CORRUPTED_PACKET
  test_corrupted_packet();
#endif

  dump_data("SEND DATA", tx_buf, tx_len);

#ifdef SPI_MANUAL_CS
  spi_cs_low();
#endif

#ifdef NCP_LL_CS_DELAY_DATA_START
  usleep(NCP_LL_CS_DELAY_US);
#endif

  transfer_result = spi_transfer(linux_spi_ctx.spi_fd, tx_buf,
                                 tx_len, NULL, 0);

#ifdef SPI_MANUAL_CS
  spi_cs_high();
#endif

/* free tx_ctx */
  pthread_mutex_lock(&s_spi_mutex);
  tx_ctx.buf = NULL;
  tx_ctx.len = 0;
  pthread_mutex_unlock(&s_spi_mutex);

  if (linux_spi_ctx.send_cb != NULL)
  {
    if (transfer_result < 0)
    {
      TRACE_MSG(TRACE_ERROR, "spi tx failed %d", (FMT__D, transfer_result));
      linux_spi_ctx.send_cb(SPI_FAIL);
    }
    else
    {
      linux_spi_ctx.send_cb(SPI_SUCCESS);
    }
  }
}


static void *intr_thread(void *args)
{
  (void)args;

  TRACE_MSG(TRACE_USB1, "intr_thread started", (FMT__0));

  while (1)
  {
    /* block in select() waiting for GPIO intr from host. Call host_interrupt_handler when got intr. */
    check_select(1);
  }

  return NULL;
}


static int spi_transfer(int fd, const uint8_t *tx_buf, size_t tx_len,
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
    TRACE_MSG(TRACE_ERROR, "spi_transfer: ioctl error %d", (FMT__D, errno));
  }

  return ret;
}

/* Called when gpio_serve.c detects device interrupt in check_select(). check_select() is called from rx thread. */
static void host_interrupt_handler(void)
{
  pthread_mutex_lock(&s_spi_mutex);
  s_slave_intr_counter++;
  TRACE_MSG(TRACE_USB3, "host_interrupt_handler s_slave_intr_counter %d - signal cond", (FMT__D, s_slave_intr_counter));
  pthread_cond_signal(&s_spi_cond);
  pthread_mutex_unlock(&s_spi_mutex);
}


static void spi_do_recv(void)
{
  int32_t ret = 0;
  uint8_t* recv_buf = NULL;
  uint16_t len = 0;

  if (rx_ctx.empty_obj_cnt)
  {
    /* At Slave put entire packet into DMA buffer.  Master
       first reads its NCP header to define packet length and pot ~CS high causent end of tx at Slave.
       Master checks packet length and, if packet has Data section, read packet rest in the next transaction (~CS low).
       If packet is ACK/NACK having no Data section, single read transaction is used.
     */
    zbncp_ll_pkt_wo_body_t pkt_hdr;
    zbncp_uint8_t crc;

#ifdef SPI_MANUAL_CS
    spi_cs_low();
#endif

    ret = spi_transfer(linux_spi_ctx.spi_fd, NULL, 0, (uint8_t*)&pkt_hdr, sizeof(pkt_hdr));
    dump_data("rx HDR", (uint8_t*)&pkt_hdr, sizeof(pkt_hdr));
    crc = zbncp_ll_hdr_calc_crc(&pkt_hdr.hdr);
    if (ret == sizeof(pkt_hdr)
        /* Skipping signature check here. Hope crc check is enough. */
        && pkt_hdr.hdr.crc == crc)
    {
      len = pkt_hdr.hdr.len;
#ifdef NCP_LL_PKT_HAVE_SIGNATURE
      len++;
      len++;
#endif
      TRACE_MSG(TRACE_USB3, "read packet header: packet len %d", (FMT__D, len));
    }
    else
    {
      TRACE_MSG(TRACE_USB3, "transfer ret %d, crc 0x%hx does not match 0x%x", (FMT__D_H_H, ret, crc, pkt_hdr.hdr.crc));
    }

    if ((len <= SPI_MAX_SIZE) && len)
    {
      recv_buf = rx_ctx.recv_obj[rx_ctx.drv_index].buf;
      /* Header is received. Do not need to receive it once
       * again. Slave sends the packet rest. It is true for TI platform as well
       * (but simpler at TI where DMA buffer need not
       * be reloaded) */
      ZB_MEMCPY(recv_buf, &pkt_hdr, sizeof(pkt_hdr));

      if (len > sizeof(pkt_hdr))
      {
#if defined SPI_USEC_BETWEEN_RX && !defined SPI_MANUAL_CS
        /* have Data section - receive it from Slave */
        /* Give to Slave a chance to fill its dma buffers. */
        /* Do not use sinchronization using interrupt line because it is much slower. */
        /* In case of manual CS DMA reload at Slave is not necessary, so no sleep there. */
        usleep(SPI_USEC_BETWEEN_RX);
#endif
        /* receive packet rest (body) */
        ret = spi_transfer(linux_spi_ctx.spi_fd, NULL, 0, recv_buf + sizeof(pkt_hdr), len - sizeof(pkt_hdr));
        TRACE_MSG(TRACE_USB3, "received Body from Slave [%d] %hx %hx %hx %hx",
                  (FMT__D_H_H_H_H, len,
                   recv_buf[sizeof(pkt_hdr)], recv_buf[sizeof(pkt_hdr)+1], recv_buf[sizeof(pkt_hdr)+2], recv_buf[sizeof(pkt_hdr)+3]));
        dump_data("rx HDR+DATA", recv_buf, len);
      }
      else
      {
        TRACE_MSG(TRACE_USB3, "packet consists of hdr only", (FMT__0));
      }

      rx_ctx.recv_obj[rx_ctx.drv_index].remain = len;

      rx_swap_drv_bufs();

      if (rx_ctx.is_user_requested)
      {
        if (spi_send_data_to_user())
        {
          call_rx_cb();
        }
      }
    }
    else
    {
      TRACE_MSG(TRACE_ERROR, "received len > SPI_MAX_SIZE or len == 0 / bad crc: len %d", (FMT__D, len));
    }

#ifdef SPI_MANUAL_CS
      spi_cs_high();
#endif
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "===========there is no free space in RX buffer", (FMT__0));
  }
}


static void rx_swap_drv_bufs(void)
{
  ZB_ASSERT(rx_ctx.empty_obj_cnt);
  rx_ctx.empty_obj_cnt--;
  rx_ctx.drv_index = (uint8_t)((rx_ctx.drv_index + 1) % SPI_RX_OBJS);
}


static void rx_swap_user_bufs(void)
{
  recv_obj_t* recv_obj_ = NULL;

  recv_obj_ = &rx_ctx.recv_obj[rx_ctx.user_index];

  recv_obj_->remain = 0;
  recv_obj_->offset = 0;
  memset(recv_obj_->buf, 0, SPI_MAX_SIZE);

  rx_ctx.empty_obj_cnt++;

  /* use 0xff mask to shut up RPI compiler. Can't overflow when doing % 3 ! */
  rx_ctx.user_index = ((rx_ctx.user_index + 1u) % SPI_RX_OBJS) & 0xffu;
}


static int spi_send_data_to_user(void)
{
  recv_obj_t* recv_obj_ = NULL;
  size_t remain_to_req_len = 0;
  int call_cb = 0;

  remain_to_req_len = rx_ctx.user_len - rx_ctx.user_offset;

  recv_obj_ = &rx_ctx.recv_obj[rx_ctx.user_index];

  if (recv_obj_->remain > remain_to_req_len)
  {
    memcpy(rx_ctx.user_buf+rx_ctx.user_offset, &recv_obj_->buf[recv_obj_->offset],
           remain_to_req_len);
    recv_obj_->offset += remain_to_req_len;
    recv_obj_->remain -= remain_to_req_len;

    dump_data("COPY TO USER1", rx_ctx.user_buf, remain_to_req_len);

    rx_ctx.is_user_requested = SPI_FALSE;
    call_cb = 1;
  }
  else if (recv_obj_->remain == rx_ctx.user_len)
  {
    memcpy(rx_ctx.user_buf+rx_ctx.user_offset, &recv_obj_->buf[recv_obj_->offset],
           remain_to_req_len);
    rx_swap_user_bufs();

    dump_data("COPY TO USER2", rx_ctx.user_buf, remain_to_req_len);

    rx_ctx.is_user_requested = SPI_FALSE;
    call_cb = 1;
  }
  else
  {
    memcpy(rx_ctx.user_buf+rx_ctx.user_offset, &recv_obj_->buf[recv_obj_->offset],
           recv_obj_->remain);

    dump_data("COPY TO BUF3", rx_ctx.user_buf, recv_obj_->remain);

    rx_ctx.user_offset += recv_obj_->remain;
    rx_swap_user_bufs();
  }

  return call_cb;
}


static void call_rx_cb(void)
{
  if (linux_spi_ctx.recv_cb != NULL)
  {
    TRACE_MSG(TRACE_USB3, "call rx cb ( %p %d )", (FMT__P_D, rx_ctx.user_buf, rx_ctx.user_len));
    linux_spi_ctx.recv_cb(rx_ctx.user_buf, (uint16_t)rx_ctx.user_len);
  }
  else
  {
    TRACE_MSG(TRACE_USB3, "user rx cb is NULL", (FMT__0));
  }
}

zb_ret_t zb_linux_turn_off_radio()
{
  ssize_t ret;

  TRACE_MSG(TRACE_ERROR, "zb_linux_turn_off_radio", (FMT__0));

  ret = gpio_write(SPI_RESET_PIN, SPI_RESET_ACTIVE);

  if (ret < 0)
  {
    return ZBNCP_RET_ERROR;
  }
  return ZBNCP_RET_OK;
}

void linux_spi_reset_ncp(void)
{
  ssize_t ret;

#ifdef DONT_RESET_SSLAVE
  return;
#endif
  ret = gpio_write(SPI_RESET_PIN, SPI_RESET_DISABLED);
  if (ret < 0)
  {
    TRACE_MSG(TRACE_ERROR, "can't write reset pin %d", (FMT__D, errno));
  }
  usleep(1);
  ret = gpio_write(SPI_RESET_PIN, SPI_RESET_ACTIVE);
  if (ret < 0)
  {
    TRACE_MSG(TRACE_ERROR, "can't write reset pin %d", (FMT__D, errno));
  }
  usleep(1);
  ret = gpio_write(SPI_RESET_PIN, SPI_RESET_DISABLED);
  if (ret < 0)
  {
    TRACE_MSG(TRACE_ERROR, "can't write reset pin %d", (FMT__D, errno));
  }
}

#ifdef TEST_WRITE_WHEN_HOST_INT
static uint8_t tx_buf[TEST_WRITE_WHEN_HOST_INT_SIZE];

void test_simultaneous_write(void);

void test_simultaneous_write(void)
{
  uint32_t test_i = 0;

  for(test_i = 0; test_i < TEST_WRITE_WHEN_HOST_INT_SIZE; test_i++)
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
    for(test_i = 0; test_i < tx_ctx.len; test_i++)
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

static void dump_data(const char * str, uint8_t *buf, size_t len)
{
  ZVUNUSED(str);
  ZVUNUSED(buf);
  ZVUNUSED(len);
  if (TRACE_ENABLED(TRACE_USB3))
  {
    TRACE_MSG(TRACE_USB3, "USB dump %s len %d", (FMT__P_D, str, len));
#ifdef DEBUG
    dump_usb_traf(buf, len);
#endif
  }
}
