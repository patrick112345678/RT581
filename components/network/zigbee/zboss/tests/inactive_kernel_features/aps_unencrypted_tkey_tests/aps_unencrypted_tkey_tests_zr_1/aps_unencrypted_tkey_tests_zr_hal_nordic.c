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
/* PURPOSE: APS Unencrypted Transport Key feature router HAL
*/

#define ZB_TRACE_FILE_ID 40957
#include "aps_unencrypted_tkey_tests_zr.h"
#include "aps_unencrypted_tkey_tests_zr_hal.h"
#include "nrf_gpio.h"
#include "boards.h"
#include "nrf_drv_gpiote.h"

#define NVRAM_ERASE_BUTTON 2

void button1_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
  ZVUNUSED(pin);
  ZVUNUSED(action);

  tests_aps_unencrypted_tkey_zr_button_pressed(LEAVE_NET_BUTTON);
}

void aps_unencrypted_tkey_zr_hal_gpio_init()
{
  nrf_drv_gpiote_in_config_t in_config = GPIOTE_RAW_CONFIG_IN_SENSE_TOGGLE(true);

  bsp_board_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS);

  nrf_drv_gpiote_init();

  in_config.pull = NRF_GPIO_PIN_PULLUP;
  nrf_drv_gpiote_in_init(BSP_BUTTON_1, &in_config, button1_handler);
  nrf_drv_gpiote_in_event_enable(BSP_BUTTON_1, true);

}

void aps_unencrypted_tkey_zr_hal_init()
{
  aps_unencrypted_tkey_zr_hal_gpio_init();
}

zb_bool_t aps_unencrypted_tkey_zc_hal_is_button_pressed()
{
  zb_bool_t ret = ZB_FALSE;
  ret = (zb_bool_t) bsp_board_button_state_get(NVRAM_ERASE_BUTTON);
  return ret;
}
