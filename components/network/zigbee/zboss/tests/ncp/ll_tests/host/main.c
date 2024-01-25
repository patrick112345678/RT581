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

#define ZB_TRACE_FILE_ID 40122

#include <string.h>
#include <stdint.h>

#include <unistd.h>
#include <sys/time.h>

#define BUF_SIZE 1792

uint8_t rx_buf[BUF_SIZE];
uint32_t rx_buf_ind = 0;
uint8_t tx_buf[BUF_SIZE];

uint8_t send_flag = 1;
uint8_t callme_flag = 0;

useconds_t sleep_timeout = 0;

unsigned long int osif_transceiver_time_get()
{
  struct timeval tmv;

  gettimeofday(&tmv, NULL);
  return (tmv.tv_sec * (1000000) + tmv.tv_usec);
}

/* [ncp_host_ll_quant] */
void callme_cb(void)
{
  callme_flag = 1;
}

uint32_t received_bytes;
uint32_t alarm_timeout_ms;

void ll_quant(void)
{
  ncp_host_ll_quant(NULL, 0, rx_buf, BUF_SIZE, &received_bytes, &alarm_timeout_ms);
  rx_buf_ind += received_bytes;
  if (rx_buf_ind == 13)
  {
    rx_buf_ind = 0;
    send_flag = 1;
  }
  sleep_timeout = alarm_timeout_ms;
}

int main(void)
{
  ncp_host_ll_proto_init(callme_cb);

  while(1)
  {
    if (!callme_flag)
    {
      usleep(sleep_timeout);
    }
    else
    {
      callme_flag = 0;
    }
    if (send_flag)
    {
      memcpy(tx_buf, "Hello, World!", 13);
      ncp_host_ll_quant(tx_buf, 13, NULL, 0, &received_bytes, &alarm_timeout_ms);
      sleep_timeout = alarm_timeout_ms;
    }
    else
    {
      ll_quant();
    }
  }
}
/* [ncp_host_ll_quant] */
