/**************************************************************************//**
 * @file     dma_reg.h
 * @version
 * @brief    DMA Register defined
 *
 * @copyright
 ******************************************************************************/

#ifndef __RT_DMA_REG_H__
#define __RT_DMA_REG_H__

#if defined ( __CC_ARM   )
#pragma anon_unions
#endif


/**
   @addtogroup REGISTER Control Register
   @{
*/

/**
    @addtogroup  DMA
    Memory Mapped Structure for DMA
@{ */

typedef struct {
    __IO  uint32_t   DMA_SRC_ADR;            //0x0
    __IO  uint32_t   DMA_DEST_ADR;           //0x4
    __IO  uint32_t   DMA_BYTES_NUM;          //0x8
    __IO  uint32_t   DMA_CONTROL;            //0xC
    __IO  uint32_t   DMA_INT;                //0x10     
    __IO  uint32_t   DMA_PORT;               //0x14
} DMA_T;


/**
    @addtogroup DMA REGISTER BIT DEFINITIONS

@{ */

#define  DMA_START_ENABLE                (1<<0)
#define  DMA_BUSY                        (1<<8)

#define  DMA_CHL_INT_STATUS              (1<<0)
#define  DMA_CHL_WAKEUP_STATUS           (1<<1)

#define  DMA_CHL_INT_ENABLE              (1<<8)
#define  DMA_CHL_INT_WAKEUP              (1<<9)

#define  DMA_CHL_INT_CLEAR_CLR           (1<<16)
#define  DMA_CHL_INT_WAKEUP_CLR          (1<<17)

/**@}*/ /* end of DMA REGISTER BIT DEFINITIONS */

/**@}*/ /* end of DMA */

/**@}*/ /* end of REGISTER group */

#if defined ( __CC_ARM   )
#pragma no_anon_unions
#endif

#endif
