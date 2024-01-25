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
/* PURPOSE: Dimmable light sample HAL
*/

#define ZB_TRACE_FILE_ID 41674
#include "bulb.h"
#include "bulb_hal.h"
#include "boards.h"
#include "app_pwm.h"

#define BULB_LED_POVER BSP_BOARD_LED_0
#define BULB_LED_CONNECT BSP_BOARD_LED_1
#define BULB_LED_LIGHT BSP_BOARD_LED_2
#define BULB_BUTTON_2 1

void pwm_ready_callback(uint32_t pwm_id);    // PWM callback function

APP_PWM_INSTANCE(PWM1,2);                   // Create the instance "PWM1" using TIMER2.

static volatile bool ready_flag;            // A flag indicating PWM status.
app_pwm_config_t pwm1_cfg;

/* Private functions */
void bulb_hal_device_started()
{
  bsp_board_led_on(BULB_LED_POVER);
/*
  app_pwm_config_t pwm1_cfg = APP_PWM_DEFAULT_CONFIG_1CH(5000L, BSP_LED_2);
  app_pwm_init(&PWM1,&pwm1_cfg,pwm_ready_callback);
  app_pwm_enable(&PWM1);
  while (app_pwm_channel_duty_set(&PWM1, 0, 0x100) == NRF_ERROR_BUSY);
*/
}

void pwm_ready_callback(uint32_t pwm_id)    // PWM callback function
{
  ZVUNUSED(pwm_id);
}


void bulb_hal_gpio_init()
{
  bsp_board_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS);
}

void bulb_hal_pwm_init()
{
  app_pwm_config_t pwm1_cfg = APP_PWM_DEFAULT_CONFIG_1CH(5000L, BSP_LED_2);
  app_pwm_init(&PWM1,&pwm1_cfg,pwm_ready_callback);
  app_pwm_enable(&PWM1);
}

void bulb_hal_set_pwm(zb_uint8_t level)
{
  while (app_pwm_channel_duty_set(&PWM1, 0, level) == NRF_ERROR_BUSY);
}

/* Public interface */
void bulb_hal_init()
{
  bulb_hal_gpio_init();

  bulb_hal_pwm_init();

  bulb_hal_device_started();
}

void bulb_hal_set_level(zb_uint8_t level)
{
  bulb_hal_set_pwm(level);
}

void bulb_hal_set_on_off(zb_bool_t on)
{
  if (on)
  {
    bsp_board_led_on(BULB_LED_LIGHT);
  }
  else
  {
    bulb_hal_set_pwm(0);
    bsp_board_led_off(BULB_LED_LIGHT);
  }
}

void bulb_hal_set_connect(zb_bool_t on)
{
  if (on)
  {
    bsp_board_led_on(BULB_LED_CONNECT);
  }
  else
  {
    bsp_board_led_off(BULB_LED_CONNECT);
  }
}

zb_bool_t bulb_hal_is_button_pressed()
{
  zb_bool_t ret = ZB_FALSE;
  ret = (zb_bool_t) bsp_board_button_state_get(BULB_BUTTON_2);
  return ret;
}
