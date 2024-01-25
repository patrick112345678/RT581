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
/*  PURPOSE: Linux GPIO access implementation.
*/

#define ZB_TRACE_FILE_ID 35
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "linux_spi_internal.h"
#include "gpio_serve.h"
#include "linux_spi_internal_types.h"

#define CTRL_PIPE_MESSAGE ("1")

int ctrl_pipe_fd[2];

static gpio_conf_item_t gpio_evt_handlers[GPIO_MAX_PIN];

#ifndef SPI_NO_EXPORT
static void export_gpio(gpio_pin_t pin, spi_bool_t in);
#endif
static int s_gpio_read(gpio_pin_t pin, spi_bool_t silent);

#ifdef TEST_MISS_HOST_INT
static uint8_t test_miss_host_int(void);
#endif
#ifdef TEST_WRITE_WHEN_HOST_INT
extern void test_simultaneous_write(void);
static uint8_t test_write_when_host_int(void);
#endif

int gpio_in_init(gpio_pin_t pin, gpio_handler_t handler)
{
  uint8_t buf[1024];
  ssize_t read_ret = 0;
  int ret = 0;
  gpio_conf_item_t *cur_conf_item;

  if (pin >= GPIO_MAX_PIN)
  {
    return -1;
  }

  cur_conf_item = &gpio_evt_handlers[pin];

  cur_conf_item->pin = pin;
  cur_conf_item->last_value = (uint32_t) - 1;
  cur_conf_item->updated = SPI_FALSE;
  cur_conf_item->handler = handler;
  cur_conf_item->direction = GPIO_IN;
#ifndef RADIO_INTERRUPT_PIN
  snprintf(cur_conf_item->path, GPIO_MAX_FD_PATH_LEN,
		   "/sys/class/gpio/gpio%d/value", pin);
#else
  /* Sure the only IN pin is radio interrupt */
  strcpy(cur_conf_item->path, RADIO_INTERRUPT_PIN);
#endif

  /* export gpio */
#ifndef SPI_NO_EXPORT
  export_gpio(pin, SPI_TRUE);
#endif

  errno = 0;
  cur_conf_item->fd_value = open(cur_conf_item->path, O_RDONLY);
  if (cur_conf_item->fd_value <= 0)
  {
    printf("oper error %s", strerror(errno));
  }
  else
  {
    read_ret = read(cur_conf_item->fd_value, buf, 1024);
  }

  if (cur_conf_item->fd_value <= 0 || read_ret <= 0)
  {
    ret = -1;
  }

#ifdef SPI_ZBOSS_TRACE
  TRACE_MSG(TRACE_USB3, "gpio_in_init INT pin %d %s fd %d ret %d", (FMT__D_P_D_D, pin, cur_conf_item->path, cur_conf_item->fd_value, ret));
#endif

  return ret;
}

int gpio_out_init(gpio_pin_t pin, char initial_value)
{
  int ret = 0;
  gpio_conf_item_t *cur_conf_item;

  if (pin >= GPIO_MAX_PIN)
  {
    return -1;
  }

  cur_conf_item = &gpio_evt_handlers[pin];

  cur_conf_item->pin = pin;
  cur_conf_item->last_value = (uint32_t) - 1;
  cur_conf_item->updated = SPI_FALSE;
  cur_conf_item->handler = NULL;
  cur_conf_item->direction = GPIO_OUT;
#ifndef RADIO_RESET_PIN
  snprintf(cur_conf_item->path, GPIO_MAX_FD_PATH_LEN,
		   "/sys/class/gpio/gpio%d/value", pin);
#else
  strcpy(cur_conf_item->path, RADIO_RESET_PIN);
#endif

  /* export gpio */
#ifndef SPI_NO_EXPORT
  export_gpio(pin, SPI_FALSE);
#endif

  errno = 0;
  cur_conf_item->fd_value = open(cur_conf_item->path, O_WRONLY);
  if (cur_conf_item->fd_value <= 0)
  {
    printf("oper error %s", strerror(errno));
    ret = -1;
  }
  else
  {
    if (gpio_write(pin, initial_value) <= 0)
    {
      printf("gpio_write error %s", strerror(errno));
    }
  }

  return ret;
}

#ifndef SPI_NO_EXPORT
static void export_gpio(gpio_pin_t pin, spi_bool_t in)
{
  char s[128];
  FILE *f = fopen("/sys/class/gpio/export", "w+");

  if (f)
  {
    fprintf(f, "%d\n", pin);
    fclose(f);
    sprintf(s, "/sys/class/gpio/gpio%d/direction", pin);
    /* seems that we try to open file faster, than it will be created,
     * so at first run after reboot, we skip next steps,
     * that's why delay was added
     */
    usleep(100000);

    f = fopen(s, "w+");
    if (f)
    {
      fprintf(f, "%s\n", in ? "in" : "out");
      fclose(f);
    }
    if (f && in)
    {
      /* configure interrupts */
      sprintf(s, "/sys/class/gpio/gpio%d/edge", pin);
      f = fopen(s, "w+");
      if (f)
      {
        fprintf(f, HOST_INT_EDGE);
        fclose(f);
      }
    }
  }
}
#endif

static int s_gpio_read(gpio_pin_t pin, spi_bool_t silent)
{
  int ret = 0;
  char val = '0';

  (void)silent;
  lseek(gpio_evt_handlers[pin].fd_value, 0, SEEK_SET);
  read(gpio_evt_handlers[pin].fd_value, &val, sizeof(val));
  ret = val - '0';

#ifndef SPI_DEBUG
  if (!silent)
#endif
  {
#ifdef SPI_ZBOSS_TRACE
    if (ret == HOST_INT_VALUE)
    {
      TRACE_MSG(TRACE_USB3, "s_gpio_read pin %d fd %d ret %d - host INT", (FMT__D_D_D, pin, gpio_evt_handlers[pin].fd_value, ret));
    }
    else
    {
      TRACE_MSG(TRACE_USB3, "s_gpio_read pin %d fd %d ret %d - int off", (FMT__D_D_D, pin, gpio_evt_handlers[pin].fd_value, ret));
    }
#endif
#ifdef SPI_DEBUG
    printf("s_gpio_read << pin %d value %d\n", pin, ret);
#endif
  }
  return ret;
}

int gpio_write(gpio_pin_t pin, char value)
{
  int ret = 0;
  char val;

  lseek(gpio_evt_handlers[pin].fd_value, 0, SEEK_SET);
  if (value)
  {
    val = '1';
    ret = write(gpio_evt_handlers[pin].fd_value, &val, sizeof(val));
  }
  else
  {
    val = '0';
    ret = write(gpio_evt_handlers[pin].fd_value, &val, sizeof(val));
  }
  return ret;
}

void check_select(spi_bool_t wait)
{
  fd_set gpio_fdset_except;
  int max_fd = 0;
  int i;
  ssize_t size = 0;
  char ctrl_msg[sizeof(CTRL_PIPE_MESSAGE)];

  FD_ZERO(&gpio_fdset_except);
  for (i = 0; i < GPIO_MAX_PIN; ++i)
  {
    if ((gpio_evt_handlers[i].pin) && (gpio_evt_handlers[i].direction == GPIO_IN))
    {
      FD_SET(gpio_evt_handlers[i].fd_value, &gpio_fdset_except);
      if (max_fd < gpio_evt_handlers[i].fd_value)
      {
        max_fd = gpio_evt_handlers[i].fd_value;
      }
#ifdef SPI_ZBOSS_TRACE
      TRACE_MSG(TRACE_USB3, "fd_set fd %d max_fd %d wait %d", (FMT__D_D_D, gpio_evt_handlers[i].fd_value, max_fd, wait));
#endif
    }
  }
  FD_SET(CTRL_PIPE_READ_FD, &gpio_fdset_except);
  if (max_fd < CTRL_PIPE_READ_FD)
  {
    max_fd = CTRL_PIPE_READ_FD;
  }
  if (max_fd)
  {
    struct timeval sel_sl;
    int ret;

    sel_sl.tv_sec = wait ? 2 : 0;
    sel_sl.tv_usec = 0;

#ifdef SPI_ZBOSS_TRACE
    TRACE_MSG(TRACE_USB3, "before select wait %d t/o %d ", (FMT__D_D, wait, sel_sl.tv_sec));
#endif

    ret = select(max_fd + 1, NULL, NULL, &gpio_fdset_except, &sel_sl);
#ifdef SPI_ZBOSS_TRACE
    TRACE_MSG(TRACE_USB3, "select t/o %d max_fd %d ret %d", (FMT__D_D_D, sel_sl.tv_sec, max_fd, ret));
#endif

    if (ret > 0 && FD_ISSET(CTRL_PIPE_READ_FD, &gpio_fdset_except))
    {
      /* woken up via control pipe */
      size = read(CTRL_PIPE_READ_FD, ctrl_msg, 1);
      if (size < 0)
      {
        perror("can't read control pipe");
      }
      else
      {
        ret--;
      }
    }

    for (i = 0; i < GPIO_MAX_PIN && ret > 0; ++i)
    {
      if (gpio_evt_handlers[i].pin
          && FD_ISSET(gpio_evt_handlers[i].fd_value, &gpio_fdset_except))
      {
        ret--;
        if (s_gpio_read(gpio_evt_handlers[i].pin, SPI_TRUE) == HOST_INT_VALUE)
        {
#ifdef TEST_MISS_HOST_INT
          (test_miss_host_int()) ?  : gpio_evt_handlers[i].handler();
#elif defined(TEST_WRITE_WHEN_HOST_INT)
          (test_write_when_host_int()) ? test_simultaneous_write() : gpio_evt_handlers[i].handler();
#else
          /* Call host_interrupt_handler() */
          gpio_evt_handlers[i].handler();
#endif
        }
      }
    }
  }
}

spi_bool_t check_gpio(gpio_pin_t pin)
{
    int ret = 0;

    ret = s_gpio_read(pin, SPI_TRUE);
    if (ret == HOST_INT_VALUE)
    {
        return SPI_TRUE;
    }

    return SPI_FALSE;
}

void free_space_has_appeared(gpio_pin_t pin)
{
  ssize_t size = 0;

  if (check_gpio(pin))
  {
    size = write(CTRL_PIPE_WRITE_FD, CTRL_PIPE_MESSAGE, sizeof(CTRL_PIPE_MESSAGE));
    if (size != sizeof(CTRL_PIPE_MESSAGE))
    {
      perror("can't write control pipe");
      return;
    }
  }
}

#ifdef TEST_MISS_HOST_INT
static uint8_t test_miss_host_int(void)
{
  static uint32_t test_cnt = 0;

  if (test_cnt == TEST_MISS_HOST_INT_PERIOD)
  {
    test_cnt = 0; /* imitate missing host_int, so don't run handler */
    return 1;
  }
  else
  {
    test_cnt++;
    return 0;
  }
}
#endif

#ifdef TEST_WRITE_WHEN_HOST_INT
static uint8_t test_write_when_host_int(void)
{
  static uint32_t test_cnt = 0;

  if (test_cnt == TEST_WRITE_WHEN_HOST_INT_PERIOD)
  {
    test_cnt = 0; /* imitate simultaneous writing */
    return 1;
  }
  else
  {
    test_cnt++;
    return 0;
  }
}
#endif
