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

#define ZB_TRACE_FILE_ID 36


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

#include "linux_spi/linux_spi_transport.h"

void send_data_cb(uint8_t param);
void recv_data_cb(uint8_t* buffer, uint16_t len);

#define RECV_BUF_SIZE 100

uint8_t read_flag = 1;

uint8_t buf[RECV_BUF_SIZE];

int main()
{
  linux_spi_init(NULL);

  linux_spi_set_cb_send_data(send_data_cb);
  linux_spi_set_cb_recv_data(recv_data_cb);

  while(1)
  {
    if (read_flag)
    {
      read_flag = 0;
      linux_spi_send_data("\rhello, world!", 14);
      linux_spi_recv_data(buf, 13);
    }
  }

  return 0;
}


void send_data_cb(uint8_t param)
{
  if (param == SPI_SUCCESS)
  {
    printf("sent successfully\n");
  }
  else
  {
    printf("failed to send\n");
  }
}

void recv_data_cb(uint8_t* buffer, uint16_t len)
{
    int i = 0;
    printf("received successfully\n");
    for (i = 0; i < len; i++)
    {
      printf("%x ", buffer[i]);
    }
    printf("\n");
    read_flag = 1;
}
