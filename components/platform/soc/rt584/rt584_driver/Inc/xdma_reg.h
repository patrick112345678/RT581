/**************************************************************************//**
 * @file     XDMA_reg.h
 * @version  V1.00
 * @brief    XDMA register definition header file
 *
 * @copyright
 *****************************************************************************/
#ifndef __XDMA_REG_H__
#define __XDMA_REG_H__

#if defined (__CC_ARM)
#pragma anon_unions
#endif



/**
    @addtogroup  WDT TIMER Controller
    Memory Mapped Structure for WDT TIMER Controller
@{ */

typedef struct {
    __IO uint32_t XDMA_CTL0;        /*0x00*/
    __O  uint32_t XDMA_CTL1;        /*0x04*/
    
} XDMA_T;

/**
    @addtogroup WDT TIMER REGISTER BIT DEFINITIONS

@{ */

// control register bit field definitions:

#define XDMA_ENABLE            (1UL<<0)
#define XDMA_RESET             (1UL<<0)

/**@}*/ /* end of WDT TIMER REGISTER BIT DEFINITIONS */

/**@}*/ /* end of WDT TIMER Controller */
/**@}*/ /* end of REGISTER group */

#if defined (__CC_ARM)
#pragma no_anon_unions
#endif

#endif

