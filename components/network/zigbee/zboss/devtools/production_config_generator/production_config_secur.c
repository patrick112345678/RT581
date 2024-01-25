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
#include "zboss_api.h"

zb_uint32_t zb_crc32(zb_uint8_t *message, int len)
{
  int i, j;
  zb_uint32_t byte, crc, mask;

  crc = 0xFFFFFFFF;
  for (i = 0 ; i < len ; ++i)
  {
    byte = message[i];
    crc = crc ^ byte;
    for (j = 7; j >= 0; j--) {
      mask = -(crc & 1);
      crc = (crc >> 1) ^ (0xEDB88320 & mask);
    }
  }
  return ~crc;
}

#define ZB_PRODUCTION_CONFIG_SIZE 1024

zb_uint32_t zb_calculate_production_config_crc(zb_uint8_t *prod_cfg_ptr, zb_uint16_t prod_cfg_size)
{
  zb_uint8_t buf[ZB_PRODUCTION_CONFIG_SIZE - sizeof(zb_uint32_t)];
  zb_uint8_t *p = (zb_uint8_t*)&buf;
  zb_int16_t len = prod_cfg_size - sizeof(zb_uint32_t);

  memcpy(buf, prod_cfg_ptr + sizeof(zb_uint32_t), prod_cfg_size - sizeof(zb_uint32_t));

  return zb_crc32(p, len);
}

zb_uint16_t zb_crc16(zb_uint8_t *p, zb_uint16_t crc, zb_uint_t len)
{
  zb_uint8_t i;

  while (len--) {
    crc ^= *p++;
    for( i=0; i<8; i++ ) {
      crc = ( (crc & 0x0001) == 1 ) ? ( crc>>1 ) ^ 0x8408: crc >> 1;
  }
  }
  return crc;
  /* NOTE:AEV: do not inverse only for compatibility with apps,
  in crc-16/x-25 config, input crc must be fixed 0xffff and output must be inversed! */
}
