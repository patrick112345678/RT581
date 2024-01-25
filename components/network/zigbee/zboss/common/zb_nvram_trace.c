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
/* PURPOSE: trace into nvram
*/

#define ZB_TRACE_FILE_ID 3221
#include "zboss_api_core.h"

#ifdef ZB_NET_TRACE

#include "zb_net_trace.h"
#include "zb_nvram.h"

/*! @cond DOXYGEN_DEBUG_SECTION */
/*! @addtogroup ZB_TRACE */
/*! @{ */

#define ZB_NVRAM_TRACE_FLASH_ADDR 0x08004000
#define ZB_NVRAM_TRACE_FLASH_SECTOR_SIZE 0x4000
#define ZB_NVRAM_TRACE_INVALID_LENGTH 0xFFFF

/* Wrappers for FLASH operations */
static zb_ret_t zb_nvram_trace_flash_write(zb_uint32_t address, zb_uint8_t *buf, zb_uint16_t len);
static zb_ret_t zb_nvram_trace_flash_read(zb_uint32_t address, zb_uint8_t *buf, zb_uint16_t len);

typedef struct zb_nvram_trace_ctx_s
{
  zb_uint32_t write_pos;
  zb_nvram_trace_size_t trace_size;
} zb_nvram_trace_ctx_t;

static zb_nvram_trace_ctx_t gs_nvram_trace_ctx;

/**
   Initialize nvram trace context.

   To be used at boot after crash.

 */
void zb_nvram_trace_init()
{
  zb_osif_nvram_init(NULL);
  ZB_BZERO(&gs_nvram_trace_ctx, sizeof(zb_nvram_trace_ctx_t));
}

/**
   Get size of crash/trace data stored in nvram

   To be used at boot after crash.

   @return data size or 0 if no stored trace
 */
zb_nvram_trace_size_t zb_nvram_trace_data_size()
{
  zb_nvram_trace_flash_read(ZB_NVRAM_TRACE_FLASH_ADDR, (zb_uint8_t*)&gs_nvram_trace_ctx.trace_size,
                            sizeof(zb_uint16_t));

  if (gs_nvram_trace_ctx.trace_size == ZB_NVRAM_TRACE_INVALID_LENGTH)
  {
    gs_nvram_trace_ctx.trace_size = 0;
  }

  return gs_nvram_trace_ctx.trace_size;
}


/**
   Get trace data from nvram to the RAM buffer.

   Must get exactly same number of bytes which zb_nvram_trace_data_size()
   returned.
   To be used at boot after crash.

   @param buf - data buffer
 */
void zb_nvram_trace_get(zb_uint8_t *buf)
{
  zb_nvram_trace_flash_read(ZB_NVRAM_TRACE_FLASH_ADDR+sizeof(zb_nvram_trace_size_t), buf,
                            gs_nvram_trace_ctx.trace_size);
}


/**
   Put portion of trace data from RAM to nvram

   Must get exactly same number of bytes which zb_nvram_trace_data_size()
   returned.
   To be used when crash detected.

   @param off - offset in nvram.
   @param buf - data buffer
   @param size - data size
 */
void zb_nvram_trace_put(zb_uint_t off, zb_uint8_t *buf, zb_uint_t size)
{
  if (gs_nvram_trace_ctx.write_pos >=
      ZB_NVRAM_TRACE_FLASH_SECTOR_SIZE + sizeof(zb_nvram_trace_size_t))
  {
    /* No free space */
    return;
  }

  if (size > (ZB_NVRAM_TRACE_FLASH_SECTOR_SIZE + sizeof(zb_nvram_trace_size_t)
              - gs_nvram_trace_ctx.write_pos))
  {
    size = ZB_NVRAM_TRACE_FLASH_SECTOR_SIZE + sizeof(zb_nvram_trace_size_t) - gs_nvram_trace_ctx.write_pos;
  }

  zb_nvram_trace_flash_write(ZB_NVRAM_TRACE_FLASH_ADDR+off, buf, size);
}

/**
   Erase nvram trace sector to store new data

   To be used when crash detected.
 */
zb_ret_t zb_nvram_trace_flash_erase()
{
  zb_ret_t ret;

  gs_nvram_trace_ctx.write_pos = 0;
  gs_nvram_trace_ctx.trace_size = 0;
  ret = zb_osif_erase_nvram_trace_sector();

  return ret;
}

static zb_ret_t zb_nvram_trace_flash_write(zb_uint32_t address, zb_uint8_t *buf, zb_uint16_t len)
{
  zb_ret_t ret;

  gs_nvram_trace_ctx.write_pos += len;
  ret = zb_osif_nvram_write_memory(address, len, buf);

  return ret;
}

static zb_ret_t zb_nvram_trace_flash_read(zb_uint32_t address, zb_uint8_t *buf, zb_uint16_t len)
{
  zb_ret_t ret;
  ret = zb_osif_nvram_read_memory(address, len, buf);

  return ret;
}
/*! @} */
/*! @endcond */ /* DOXYGEN_DEBUG_SECTION */

#endif  /* ZB_NET_TRACE */
