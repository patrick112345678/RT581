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
/* PURPOSE: Light ZC sample HAL
*/

#define ZB_TRACE_FILE_ID 40014
#include "light_zc.h"
#include "light_zc_hal.h"

#include "em_cmu.h"
#include "em_gpio.h"
#include "gpiointerrupt.h"
#include "hal-config-board.h"


/* Private functions */
/* Stub function */
void light_zc_hal_device_started()
{
  return;
}

void light_zc_hal_gpio_init()
{
  // Enable GPIO clock.
  CMU_ClockEnable(cmuClock_GPIO, true);
  // Initialize GPIO interrupt.
  GPIOINT_Init();
  // Configure PB0 as input and enable interrupt.
  GPIO_PinModeSet(BSP_BUTTON0_PORT, BSP_BUTTON0_PIN, gpioModeInputPull, 1);
  GPIO_PinModeSet(BSP_BUTTON1_PORT, BSP_BUTTON1_PIN, gpioModeInputPull, 1);

  /* Configure pin as output */
  GPIO_PinModeSet(BSP_LED0_PORT, BSP_LED0_PIN, gpioModePushPull, 1);
  GPIO_PinModeSet(BSP_LED1_PORT, BSP_LED1_PIN, gpioModePushPull, 1);

  /* Configure pin as input */
  GPIO_PinOutClear(BSP_LED0_PORT, BSP_LED0_PIN);
  GPIO_PinOutClear(BSP_LED1_PORT, BSP_LED1_PIN);
}

/* Public interface */
void light_zc_hal_init()
{
   light_zc_hal_gpio_init();
  light_zc_hal_gpio_init();
  light_zc_hal_device_started();
}

zb_bool_t light_zc_hal_is_button_pressed(zb_uint8_t button_no)
{
  zb_uint32_t pin_read;
  if (button_no == 0)
  {
    pin_read = GPIO_PinInGet(BSP_BUTTON0_PORT, BSP_BUTTON0_PIN);
  }
  else if (button_no == 1)
  {
    pin_read = GPIO_PinInGet(BSP_BUTTON1_PORT, BSP_BUTTON1_PIN);
  }
  else
  {
    pin_read = 1;
  }
  return pin_read ? ZB_FALSE : ZB_TRUE;
}
