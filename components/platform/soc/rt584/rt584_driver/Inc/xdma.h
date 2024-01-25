/**************************************************************************//**
 * @file     XDMA.h
 * @version
 * @brief    XDMA driver header file
 *
 * @copyright
 ******************************************************************************/
#ifndef __XDMA_H__
#define __XDMA_H__

#include "RT584_cm33.h"

#ifdef __cplusplus
extern "C"
{
#endif


__STATIC_INLINE void xdma_enable(void)
{
  XDMA->XDMA_CTL0 |=XDMA_ENABLE;
}

__STATIC_INLINE void xdma_disable(void)
{
  XDMA->XDMA_CTL0 &=~XDMA_ENABLE;
}

__STATIC_INLINE void xdma_reset(void)
{
   XDMA->XDMA_CTL1 |=XDMA_RESET;
}

#ifdef __cplusplus
}
#endif

#endif

