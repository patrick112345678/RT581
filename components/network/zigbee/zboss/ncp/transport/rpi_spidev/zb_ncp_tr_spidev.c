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
/* PURPOSE: NCP transport for Linux SPIDEV, low level. Host side on Raspberry Pi.
*/

#define ZB_TRACE_FILE_ID 14008
#include "high_level/host-shared/ncp_common.h"
#include "transport/zb_ncp_tr.h"
#include "linux_spi_transport.h"

/*! \addtogroup NCP_TRANSPORT_API */
/*! @{ */

/** @brief NCP Linux SPIDEV transport implementation for Raspberry PI */
struct zbncp_transport_impl_s
{
  zbncp_transport_ops_t ops;    /**< Table of user-implemented transport operations */
  zbncp_transport_cb_t cb;      /**< Table of callbacks from low-level protocol */
};

/** @brief Internal name for spidev transport */
typedef struct zbncp_transport_impl_s ncp_tr_spidev_t;

/** @brief User callback on spidev initialization completion. */
static void linux_spi_init_complete(uint8_t unused);
/** @brief User callback on spidev sending completion. */
static void linux_spi_send_complete(uint8_t spi_status);
/** @brief User callback on spidev receving completion. */
static void linux_spi_recv_complete(uint8_t *buf, uint16_t len);

/** @brief User implementation of the initializing operation. */
static void ncp_tr_spidev_init(ncp_tr_spidev_t *tr, const zbncp_transport_cb_t *cb);
/** @brief User implementation of the sending operation. */
static void ncp_tr_spidev_send(ncp_tr_spidev_t *tr, zbncp_cmemref_t mem);
/** @brief User implementation of the receving operation. */
static void ncp_tr_spidev_recv(ncp_tr_spidev_t *tr, zbncp_memref_t mem);

/**
   @}
*/

static ncp_tr_spidev_t s_ncp_tr_spidev;

static void linux_spi_init_complete(uint8_t unused)
{
  /* [linux_spi_init_complete] */
  const ncp_tr_spidev_t *tr = &s_ncp_tr_spidev;

  if (tr->cb.init)
  {
    TRACE_MSG(TRACE_COMMON1, "calling ll init cb %p", (FMT__P, tr->cb.init));
    tr->cb.init(tr->cb.arg);
  }
  /* [linux_spi_init_complete] */

  (void) unused;
}

static void linux_spi_send_complete(uint8_t spi_status)
{
  const ncp_tr_spidev_t *tr = &s_ncp_tr_spidev;
  zbncp_tr_send_status_t status = ZBNCP_TR_SEND_STATUS_ERROR;

  switch (spi_status)
  {
    case SPI_SUCCESS:
      status = ZBNCP_TR_SEND_STATUS_SUCCESS;
      break;
    case SPI_BUSY:
      status = ZBNCP_TR_SEND_STATUS_BUSY;
      break;
    case SPI_FAIL:
      status = ZBNCP_TR_SEND_STATUS_ERROR;
      break;
  }

  /* [linux_spi_send_complete] */
  if (tr->cb.send)
  {
    TRACE_MSG(TRACE_COMMON3, "linux_spi_send_complete: calling ll send cb %p status %d", (FMT__P_D, tr->cb.send, (int)status));
    /* call libncp/zbncp_ll_impl.c:zbncp_ll_send_complete() */
    tr->cb.send(tr->cb.arg, status);
  }
  /* [linux_spi_send_complete] */
}

static void linux_spi_recv_complete(uint8_t *buf, uint16_t len)
{
  /* [linux_spi_recv_complete] */
  const ncp_tr_spidev_t *tr = &s_ncp_tr_spidev;

  if (tr->cb.recv)
  {
    TRACE_MSG(TRACE_COMMON3, "calling ll recv cb %p len %d", (FMT__P_D, tr->cb.recv, (int)len));
    tr->cb.recv(tr->cb.arg, len);
  }
  /* [linux_spi_recv_complete] */

  (void) buf;
}

/* [ncp_tr_spidev_init] */
static void ncp_tr_spidev_init(ncp_tr_spidev_t *tr, const zbncp_transport_cb_t *cb)
{
  TRACE_MSG(TRACE_COMMON2, ">ncp_tr_spidev_init - ZBOSS SPIDEV NCP transport starting", (FMT__0));

  tr->cb = *cb;

  linux_spi_init(linux_spi_init_complete);
  linux_spi_set_cb_send_data(linux_spi_send_complete);
  linux_spi_set_cb_recv_data(linux_spi_recv_complete);
  linux_spi_reset_ncp();

  TRACE_MSG(TRACE_COMMON2, "<ncp_tr_spidev_init", (FMT__0));
}
/* [ncp_tr_spidev_init] */


/* [ncp_tr_spidev_send] */
static void ncp_tr_spidev_send(ncp_tr_spidev_t *tr, zbncp_cmemref_t mem)
{
  TRACE_MSG(TRACE_COMMON3, ">ncp_tr_spidev_send data %p len %d", (FMT__P_D, mem.ptr, (int)mem.size));
  linux_spi_send_data((zb_uint8_t *)mem.ptr, (zb_ushort_t)mem.size);
  TRACE_MSG(TRACE_COMMON3, "<ncp_tr_spidev_send", (FMT__0));
  ZBNCP_UNUSED(tr);
}
/* [ncp_tr_spidev_send] */


/* [ncp_tr_spidev_recv] */
static void ncp_tr_spidev_recv(ncp_tr_spidev_t *tr, zbncp_memref_t mem)
{
  TRACE_MSG(TRACE_COMMON3, ">ncp_tr_spidev_recv data %p len %d", (FMT__P_D, mem.ptr, (int)mem.size));
  linux_spi_recv_data((zb_uint8_t *)mem.ptr, (zb_ushort_t)mem.size);
  TRACE_MSG(TRACE_COMMON3, "<ncp_tr_spidev_recv", (FMT__0));
  ZBNCP_UNUSED(tr);
}
/* [ncp_tr_spidev_recv] */


/* [ncp_host_transport_create] */
const zbncp_transport_ops_t *ncp_host_transport_create(void)
{
  s_ncp_tr_spidev.ops.impl = &s_ncp_tr_spidev;
  s_ncp_tr_spidev.ops.init = ncp_tr_spidev_init;
  s_ncp_tr_spidev.ops.send = ncp_tr_spidev_send;
  s_ncp_tr_spidev.ops.recv = ncp_tr_spidev_recv;
  return &s_ncp_tr_spidev.ops;
}
/* [ncp_host_transport_create] */
