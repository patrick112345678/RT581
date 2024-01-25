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
/* PURPOSE: APS Unencrypted Transport Key feature coordinator HAL
*/

#define ZB_TRACE_FILE_ID 40955
#include "aps_unencrypted_tkey_tests_zc.h"
#include "aps_unencrypted_tkey_tests_zc_hal.h"
#include "nrf_gpio.h"
#include "boards.h"
#include "nrf_drv_gpiote.h"

#define NVRAM_ERASE_BUTTON 2
#define APS_UNENCRYPTED_TKEY_DEVICE_JOINED_1 BSP_BOARD_LED_0
#define APS_UNENCRYPTED_TKEY_DEVICE_JOINED_2 BSP_BOARD_LED_1
#define APS_UNENCRYPTED_TKEY_DEVICE_JOINED_3 BSP_BOARD_LED_2
#define APS_UNENCRYPTED_TKEY_DEVICE_JOINED_4 BSP_BOARD_LED_2

zb_uint8_t aps_unencrypted_tkey_zc_device_joined_indication()
{
  zb_uint8_t i;
  zb_uint32_t leds[] = {BSP_BOARD_LED_0, BSP_BOARD_LED_1, BSP_BOARD_LED_2, BSP_BOARD_LED_3};
  for (i=0; i<LEDS_NUMBER && bsp_board_led_state_get(leds[i]); i++);
  if (i<LEDS_NUMBER)
  {
    bsp_board_led_on(leds[i]);
  }
  else
  {
    i = 0xFF;
  }
  return i;
}

void aps_unencrypted_tkey_zc_device_leaved_indication(zb_uint8_t led_idx)
{
  zb_uint32_t leds[] = {BSP_BOARD_LED_0, BSP_BOARD_LED_1, BSP_BOARD_LED_2, BSP_BOARD_LED_3};
  if (led_idx >= LEDS_NUMBER)
  {
    ZB_ASSERT(0);
  }
  else
  {
    bsp_board_led_off(leds[led_idx]);
  }
}

void button1_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
  ZVUNUSED(pin);
  ZVUNUSED(action);

  aps_unencrypted_tkey_tests_zc_button_pressed(OPEN_NET_BUTTON);
}

void button2_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
  ZVUNUSED(pin);
  ZVUNUSED(action);

  aps_unencrypted_tkey_tests_zc_button_pressed(APS_TKEY_SECURITY_BUTTON);
}

void button3_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
  ZVUNUSED(pin);
  ZVUNUSED(action);

  aps_unencrypted_tkey_tests_zc_button_pressed(ZDO_LEAVE_BUTTON);
}

void button4_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
  ZVUNUSED(pin);
  ZVUNUSED(action);

  aps_unencrypted_tkey_tests_zc_button_pressed(CLOSE_NET_BUTTON);
}

void aps_unencrypted_tkey_zc_hal_gpio_init()
{
  nrf_drv_gpiote_in_config_t in_config = GPIOTE_RAW_CONFIG_IN_SENSE_TOGGLE(true);
  
  bsp_board_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS);
  
  nrf_drv_gpiote_init();

  in_config.pull = NRF_GPIO_PIN_PULLUP;
  nrf_drv_gpiote_in_init(BSP_BUTTON_0, &in_config, button1_handler);
  nrf_drv_gpiote_in_event_enable(BSP_BUTTON_0, true);
  
  in_config.pull = NRF_GPIO_PIN_PULLUP;
  nrf_drv_gpiote_in_init(BSP_BUTTON_1, &in_config, button2_handler);
  nrf_drv_gpiote_in_event_enable(BSP_BUTTON_1, true);
  
  in_config.pull = NRF_GPIO_PIN_PULLUP;
  nrf_drv_gpiote_in_init(BSP_BUTTON_2, &in_config, button3_handler);
  nrf_drv_gpiote_in_event_enable(BSP_BUTTON_2, true);
  
  in_config.pull = NRF_GPIO_PIN_PULLUP;
  nrf_drv_gpiote_in_init(BSP_BUTTON_3, &in_config, button4_handler);
  nrf_drv_gpiote_in_event_enable(BSP_BUTTON_3, true);
}

/* Public interface */
void aps_unencrypted_tkey_zc_hal_init()
{
  volatile zb_uint16_t i = 0;
  for (i = 16000; i>0; i--)
  {
   aps_unencrypted_tkey_zc_hal_gpio_init();
  }
  aps_unencrypted_tkey_zc_hal_gpio_init();
}

zb_bool_t aps_unencrypted_tkey_zc_hal_is_button_pressed()
{
  zb_bool_t ret = ZB_FALSE;
  ret = (zb_bool_t) bsp_board_button_state_get(NVRAM_ERASE_BUTTON);
  return ret;
}
