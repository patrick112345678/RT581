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

#include <stdio.h>
#include <stdint.h>

int
main(
  int argc,
  char **argv)
{
  uint32_t osize, size;
  int16_t crc;
  int16_t rcrc;
  int c;
  int ret;
  FILE *f = fopen(argv[1], "r");
  ret = fread(&size, 1, sizeof(size), f);
  osize = size;
  crc = 0;
  while (size && (c = getc(f)) != EOF)
  {
    size--;
    crc -= c;
  }
  ret = fread(&rcrc, 1, sizeof(rcrc), f);
  if(rcrc != crc)
  {
    printf("crc mismatch: %x %x\n", crc, rcrc);
  }
  else
  {
    printf("size %d crc %x ok\n", osize, crc & 0xffff);
  }
  return 0;
}
