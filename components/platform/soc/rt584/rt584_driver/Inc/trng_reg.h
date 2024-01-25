/**************************************************************************//**
 * @file     trng_reg.h
 * @version  V1.00
 * @brief    true random number generator register definition header file
 *
 * @copyright 
 *****************************************************************************/
#ifndef ___RT584_TRNG_REG_H__
#define ___RT584_TRNG_REG_H__


typedef struct trng_ctrl_struct {
        
    __IO uint32_t       TRNG0;
    __IO uint32_t       TRNG1;    
    __I  uint32_t       TRNG2;
    __IO uint32_t       TRNG3;

} TRNG_T;

#define  TRNG_ENABLE          (1UL << 0)
#define  TRNG_INTR_CLEAR      (1UL << 1)

#define  TRNG_SEL             (1UL << 0)
#define  TRNG_INTR_ENABLE     (1UL << 1)

#define  TRNG_BUSY            (1UL << 0)
#define  TRNG_INTR_STATUS     (1UL << 1)

#endif
