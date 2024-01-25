/**************************************************************************//**
 * @file     aux_comp_reg.h
 * @version  V1.00
 * @brief    AUX Comparator register definition header file
 *
 * @copyright
 *****************************************************************************/
#ifndef ___AUX_COMP_REG_H__
#define ___AUX_COMP_REG_H__

#if defined ( __CC_ARM   )
#pragma anon_unions
#endif

/**
   @addtogroup REGISTER Control Register
   @{
*/
//0x00
typedef union comp_ana_ctl_s {
    struct comp_ana_ctl_b { 
        uint32_t COMP_SELREF         : 1;
        uint32_t COMP_SELINPUT       : 1;        
        uint32_t COMP_PW             : 2;
        uint32_t COMP_SELHYS         : 2;
        uint32_t COMP_SWDIV          : 1;
        uint32_t COMP_PSRR           : 1;
        uint32_t COMP_VSEL           : 4;
        uint32_t COMP_REFSEL         : 4;        
        uint32_t COMP_CHSEL          : 4;
        uint32_t COMP_TC             : 1;
        uint32_t RESERVED1           : 3;
        uint32_t COMP_EN_START       : 2;
        uint32_t RESERVED2           : 6;
    } bit;
    uint32_t reg;
} comp_ana_ctl_t;

//0x04
typedef union comp_dig_ctl0_s {
    struct comp_dig_ctl0_b {       
        uint32_t COMP_EN_NM         : 1;
        uint32_t COMP_EN_SP         : 1;
        uint32_t COMP_EN_DS         : 1;
        uint32_t RESERVED1          : 1;
        uint32_t DEBOUNCE_EN        : 1;
        uint32_t RESERVED2          : 1;
        uint32_t DEBOUNCE_SEL       : 2;
        uint32_t COUNTER_MODE_EN    : 1;
        uint32_t RESERVED3          : 1;
        uint32_t COUNTER_MODE_EDGE  : 2;
        uint32_t DS_WAKEUP_EN       : 1;
        uint32_t DS_WAKEUP_POL      : 1;
        uint32_t RESERVED4          : 2;
        uint32_t COUNTER_TRIGGER_TH : 16;
    } bit;
    uint32_t reg;
} comp_dig_ctl0_t;

//0x08
typedef union comp_dig_ctl1_s {
    struct comp_dig_ctl1_b {       
        uint32_t EN_INTR_RISING         : 1;
        uint32_t EN_INTR_FALLING        : 1; 
        uint32_t EN_INTR_COUNTER        : 1;
        uint32_t RESERVED1              : 5;
        uint32_t CLR_INTR_RISING        : 1;
        uint32_t CLR_INTR_FALLING       : 1;
        uint32_t CLR_INTR_COUNTER       : 1; 
        uint32_t CLR_COUNTER            : 1;
        uint32_t RESERVED2              : 4;
        uint32_t COMP_SETTLE_TIME       : 4;
        uint32_t RESERVED3              : 12;
    } bit;
    uint32_t reg;
} comp_dig_ctl1_t;

//0x0c
typedef union comp_dig_ctl2_s {
    struct comp_dig_ctl2_b {       
        uint32_t STA_INTR_RISING        : 1;
        uint32_t STA_INTR_FALLING       : 1; 
        uint32_t STA_INTR_COUNTER       : 1;
        uint32_t RESERVED1              : 5;
        uint32_t COMP_OUT               : 1;
        uint32_t RESERVED2              : 7; 
        uint32_t COUNTER_CNT            : 16;
    } bit;
    uint32_t reg;
} comp_dig_ctl2_t;

/**
    @addtogroup  IRM Controller
    Memory Mapped Structure for IRM Controller
@{ */

typedef struct {
    __IO comp_ana_ctl_t       COMP_ANA_CTRL;       /*0x00*/
    __IO comp_dig_ctl0_t      COMP_DIG_CTRL0;      /*0x04*/
    __IO comp_dig_ctl1_t      COMP_DIG_CTRL1;      /*0x08*/
    __IO comp_dig_ctl2_t      COMP_DIG_CTRL2 ;     /*0x0C*/
} AUX_COMP_CTL_T;

/*register alias name*/




/**@}*/ /* end of GPIO Controller */
/**@}*/ /* end of REGISTER group */

#if defined ( __CC_ARM   )
#pragma no_anon_unions
#endif

#endif
