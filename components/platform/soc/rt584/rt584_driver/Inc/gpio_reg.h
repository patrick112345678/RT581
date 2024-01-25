/**************************************************************************//**
 * @file     gpio_reg.h
 * @version  V1.00
 * @brief    GPIO register definition header file
 *
 * @copyright
 *****************************************************************************/
#ifndef ___GPIO_REG_H__
#define ___GPIO_REG_H__

#if defined ( __CC_ARM   )
#pragma anon_unions
#endif

/**
   @addtogroup REGISTER Control Register
   @{
*/

/**
    @addtogroup  GPIO Controller
    Memory Mapped Structure for GPIO Controller
@{ */

typedef struct {
    
    union {                        /*0x00*/
        __IM uint32_t STATE;
        __OM uint32_t OUTPUT_HIGH;
    };
    
    union {                        /*0x04*/
        __IM uint32_t INT_STATUS;
        __OM uint32_t OUTPUT_LOW;
    };
                               
    __IOM uint32_t OUTPUT_EN;      /*0x08, read for output state*/
    
    union {                        /*0x0C*/
        __IM uint32_t OUTPUT_STATE;
        __OM uint32_t INPUT_EN;
    };    
    
    union {                        /*0x10*/
        __IO  uint32_t INT_ENABLE_STATUS;
        __OM  uint32_t ENABLE_INT;        
    };
    
    __OM  uint32_t DISABLE_INT;    /*0x14*/    
    __IOM uint32_t EDGE_TRIG;      /*0x18, read for the state of edge trigger enable*/
    __IOM uint32_t LEVEL_TRIG;     /*0x1C, read for the state of level enable*/
    __IOM uint32_t POSTITIVE;      /*0x20*/
    __IOM uint32_t NEGATIVE;       /*0x24*/
    __IOM uint32_t BOTH_EDGE_EN;   /*0x28*/
    
    __OM  uint32_t BOTH_EDGE_CLR;  /*0x2C*/
    __OM  uint32_t EDGE_INT_CLR;   /*0x30*/
    __IOM uint32_t LOOPBACK;       /*0x34 Enable Loopback*/
    __IOM uint32_t ENABLE_INPUT;   /*0x38 Enable Input*/
    __OM  uint32_t DISABLE_INPUT;  /*0x3C Disable input*/
    
    __IOM uint32_t DEBOUCE_EN;     /*0x40 Enable Debounce*/
    __OM  uint32_t DEBOUCE_DIS;    /*0x44 Disable Debounce*/
    __IOM uint32_t DEBOUNCE_TIME;  /*0x48 De-Bounce time selection*/
    
    __IOM uint32_t Reserve1;       /*0x4C */
    
    __IOM uint32_t DS_WAKEUP_EN;   /*0x50*/
    __OM  uint32_t DS_WAKEUP_CLR;  /*0x54*/
    __IOM uint32_t DS_WAKEUP_INV;  /*0x58, write 1 to set corresponding GPIO wakeup polarity low to high
                                           0: for high to low (default is 0)*/
                                           
    __OM  uint32_t DS_WAKEUP_INV_CLR;  /*0x5C, write 1 to set corresponding GPIO wakeup polarity of
                                        high to low in DeepSleep*/
                                            
} GPIO_T;


/**@}*/ /* end of GPIO Controller */
/**@}*/ /* end of REGISTER group */

#if defined ( __CC_ARM   )
#pragma no_anon_unions
#endif

#endif
