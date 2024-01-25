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
/*  PURPOSE: Genereic memory copying/moving functions implementation.
*/

#define ZB_TRACE_FILE_ID 30
#include "zbncp_mem.h"
#include "zbncp_debug.h"

ZBNCP_DBG_STATIC_ASSERT(sizeof(void*) <= sizeof(zbncp_uintptr_t))

#if !ZBNCP_USE_STDMEM

static void zbncp_mem_copy_forward(void *dest, const void *src, zbncp_size_t size)
{
  zbncp_size_t i;
  char *pd = (char *)dest;
  const char *ps = (const char *)src;
  for (i = 0u; i < size; ++i)
  {
    pd[i] = ps[i];
  }
}

static void zbncp_mem_copy_backward(void *dest, const void *src, zbncp_size_t size)
{
  zbncp_size_t i;
  char *pd = (char *)dest;
  const char *ps = (const char *)src;
  if (size != 0u)
  {
    for (i = 0u; i < size; ++i)
    {
      zbncp_size_t ri = size - i - 1u;
      pd[ri] = ps[ri];
    }
  }
}

void zbncp_mem_copy(void *dest, const void *src, zbncp_size_t size)
{
  zbncp_mem_copy_forward(dest, src, size);
}

void zbncp_mem_move(void *dest, const void *src, zbncp_size_t size)
{
  /* MISRA-C 2012 Rule 18.3 prohibits to use relational operators
   * (>, >=, <, <=) to objects of pointer type. We need to detect
   * memory copying direction so we cast pointers to appropriate-
   * sized integers and compare them. */
  zbncp_uintptr_t pd = (zbncp_uintptr_t)(char *)dest;
  zbncp_uintptr_t ps = (zbncp_uintptr_t)(const char *)src;

  if (pd < ps)
  {
    zbncp_mem_copy_forward(dest, src, size);
  }
  else
  {
    zbncp_mem_copy_backward(dest, src, size);
  }
}

void zbncp_mem_fill(void *mem, zbncp_size_t size, zbncp_uint8_t val)
{
  zbncp_size_t i;
  char *p = (char *)mem;
  for (i = 0u; i < size; ++i)
  {
    p[i] = (char)val;
  }
}

#endif /* !ZBNCP_USE_STDMEM */
