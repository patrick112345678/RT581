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
/* PURPOSE: 8051-specific random number generator
*/

#define ZB_TRACE_FILE_ID 554
#ifndef ZB_WINDOWS
#include "zb_common.h"
#else
#include "zb_types.h"
#include "zb_config.h"
#endif /* !ZB_WINDOWS */

#ifndef ZB_WINDOWS
static zb_uint32_t zb_last_random_value = 0;

static zb_uint32_t zb_last_random_value_jtr = 0;

/* Interrupt context. Moved here to build utils */
ZB_SDCC_XDATA zb_intr_globals_t g_izb;

zb_uint16_t zb_random()
{
  if (!zb_last_random_value)
  {
    zb_last_random_value = (zb_uint32_t)zb_random_seed();
  }
  zb_last_random_value = 1103515245l * zb_last_random_value + 12345;

  return (zb_last_random_value & 0xffff);
}


zb_uint16_t zb_random_jitter()
{
  if (!zb_last_random_value_jtr)
  {
    zb_last_random_value_jtr = (zb_uint32_t)zb_random_seed();
  }
  zb_last_random_value_jtr = 1103515245l * zb_last_random_value_jtr + 12345;

  return (zb_last_random_value_jtr & 0xffff);
}

void zb_set_last_random_value(zb_uint32_t val)
{
  zb_last_random_value = val;
}
#endif /* !ZB_WINDOWS */

#ifndef ZB_LITTLE_ENDIAN
void zb_htole32(zb_uint32_t ZB_XDATA *ptr, zb_uint32_t ZB_XDATA *val)
{
  ((zb_uint8_t *)(ptr))[3] = ((zb_uint8_t *)(val))[0],
  ((zb_uint8_t *)(ptr))[2] = ((zb_uint8_t *)(val))[1],
  ((zb_uint8_t *)(ptr))[1] = ((zb_uint8_t *)(val))[2],
  ((zb_uint8_t *)(ptr))[0] = ((zb_uint8_t *)(val))[3];
}
#else
void zb_htobe32(zb_uint32_t ZB_XDATA *ptr, zb_uint32_t ZB_XDATA *val)
{
  ((zb_uint8_t *)(ptr))[3] = ((zb_uint8_t *)(val))[0],
  ((zb_uint8_t *)(ptr))[2] = ((zb_uint8_t *)(val))[1],
  ((zb_uint8_t *)(ptr))[1] = ((zb_uint8_t *)(val))[2],
  ((zb_uint8_t *)(ptr))[0] = ((zb_uint8_t *)(val))[3];
}
#endif

#ifndef ZB_WINDOWS
#ifndef ZB_IAR
void zb_get_next_letoh16(zb_uint16_t *dst, zb_uint8_t **src)
{
  ZB_LETOH16(dst, *src);
  (*src) += 2;
}
#endif

void zb_put_next_htole16(zb_uint8_t **dst, zb_uint16_t val)
{
  ZB_HTOLE16(*dst, &val);
  (*dst) += 2;
}


void zb_bzero_short(char *s, zb_uint8_t n)
{
  while (n--)
  {
    s[n] = 0;
  }
}

/* Calculate Fowler-Noll-Vo hash for random number generator. */
zb_uint32_t zb_get_fnv_hash(zb_uint8_t *buffer)
{
  zb_uint32_t random_generator_seed = FNV_32_FNV0;

  while (*buffer)
  {
    random_generator_seed ^= (zb_uint32_t)*buffer++;
    random_generator_seed *= FNV_32_PRIME;
  }
  return random_generator_seed;
}

#endif /* !ZB_WINDOWS */
