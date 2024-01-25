/**************************************************************************//**
 * @file     timer_reg.h
 * @version  V1.00
 * @brief    timer definition header file
 *
 * @copyright
 *****************************************************************************/
#ifndef __TIMER_REG_H__
#define __TIMER_REG_H__

#if defined (__CC_ARM)
#pragma anon_unions
#endif

/**
   @addtogroup REGISTER Control Register
   @{
*/

/**
    @addtogroup  TIMER Controller
    Memory Mapped Structure for TIMER Controller
@{ */

typedef union timern_ctrl_s {
    struct timern_ctrl_b {       
        uint32_t UP_CONUNT                  : 1;
        uint32_t ONE_SHOT_EN                : 1;        
        uint32_t PRESCALE                   : 3;
        uint32_t INT_ENABLE                 : 1;
        uint32_t MODE                       : 1;
        uint32_t EN                         : 1;
        
        uint32_t INT_STATUS                 : 1;
        uint32_t TIMER_ENABLE_STATUS        : 1;
        uint32_t CH0_CAPTURE_INT_STATUS     : 1;
        uint32_t CH1_CAPTURE_INT_STATUS     : 1;
        uint32_t RESERVED1                  : 4;
        
        uint32_t CH0_CAPTURE_EDGE           : 1;
        uint32_t CH1_CAPTURE_EDGE           : 1;
        uint32_t CH0_DEGLICH_EN             : 1;
        uint32_t CH1_DEGLICH_EN             : 1;
        uint32_t CH0_CAPTURE_INT_EN         : 1;
        uint32_t CH1_CAPTURE_INT_EN         : 1;
        uint32_t RESERVED2                  : 10;
    } bit;
    uint32_t reg;
} timern_ctrl_t;

typedef union timern_cap_clr_s {
    struct timern_cap_clr_b {       
        uint32_t CH0_CAPTURE_INT_CLEAR      : 1;
        uint32_t CH1_CAPTURE_INT_CLEAR      : 1;        
        uint32_t RESERVED1                  : 30;
    } bit;
    uint32_t reg;
} timern_cap_clr_t;

typedef union timern_cap_en_s {
    struct timern_cap_en_b {       
        uint32_t CH0_CAPTURE_EN     : 1;
        uint32_t CH1_CAPTURE_EN     : 1;
        uint32_t TIMER_PWM_EN       : 1;
        uint32_t RESERVED1          : 29;
    } bit;
    uint32_t reg;
} timern_cap_en_t;

typedef union timern_cap_io_sel_s {
    struct timern_cap_io_sel_b {       
        uint32_t CH0_CAPTURE_IO_SEL     : 5;
        uint32_t RESERVED1              : 3;
        uint32_t CH1_CAPTURE_IO_SEL     : 5;        
        uint32_t RESERVED2              : 19;
    } bit;
    uint32_t reg;
} timern_cap_io_sel_t;

typedef union timern_pha_s {
    struct timern_pha_b {       
        uint32_t PHA                    : 1;
        uint32_t RESERVED1              : 31;
    } bit;
    uint32_t reg;
} timern_pha_t;


typedef union timer32kn_ctrl_s {
    struct timer32kn_ctrl_b {       
        uint32_t UP_CONUNT              : 1;
        uint32_t ONE_SHOT_EN            : 1;        
        uint32_t PRESCALE               : 3;
        uint32_t INT_ENABLE             : 1;
        uint32_t MODE                   : 1;
        uint32_t EN                     : 1;
        uint32_t INT_STATUS             : 1;
        uint32_t TIMER_ENABLE_STATUS    : 1;
        uint32_t RESERVED2              : 22;
    } bit;
    uint32_t reg;
} timer32kn_ctrl_t;

typedef union timer32kn_repdly_s {
    struct timer32kn_repdly_b {
        uint32_t INT_REPEAT_DELAY       : 16;
        uint32_t INT_REPEAT_DELAY_EN    : 1;
        uint32_t RESERVED               : 15;
    } bit;
    uint32_t reg;
} timer32kn_repdly_t;

typedef struct {
    __IO uint32_t LOAD;                         /*0x00*/
    __IO uint32_t VALUE;                        /*0x04*/
    __IO timern_ctrl_t CONTROL;                 /*0x08*/
    __IO uint32_t CLEAR;                        /*0x0C*/    
    __O  timern_cap_clr_t CAPTURE_CLEAR;        /*0x10*/
    __I  uint32_t CH0_CAP_VALUE;                /*0x14*/
    __I  uint32_t CH1_CAP_VALUE;                /*0x18*/
    __IO uint32_t PRESCALE;                     /*0x1C*/
    __IO uint32_t EXPRIED_VALUE;                /*0x20*/
    __IO timern_cap_en_t CAP_EN;                /*0x24*/
    __IO timern_cap_io_sel_t CAP_IO_SEL;        /*0x28*/
    __IO uint32_t THD;                          /*0x2C*/
    __IO timern_pha_t PHA;                      /*0x30*/
    
} timern_t;


typedef struct {
    __IO uint32_t LOAD;                         /*0x00*/
    __IO uint32_t VALUE;                        /*0x04*/
    __IO timer32kn_ctrl_t CONTROL;              /*0x08*/
    __IO uint32_t CLEAR;                        /*0x0C*/    
    __O  timer32kn_repdly_t REPEAT_DELAY;       /*0x10*/
    __IO uint32_t PRESCALE;                     /*0x14*/
    __IO uint32_t EXPRIED_VALUE;                /*0x18*/
    
} timer32kn_t;

/**
    @addtogroup TIMER REGISTER BIT DEFINITIONS

@{ */

// control register bit field definitions:

#define TIMER_INT_STATUS        0x0100

#define TIMER_DISABLE_TIMER     0x0000
#define TIMER_ENABLE_TIMER      0x0080

#define TIMER_MODE_PERODIC      0x0040


/**@}*/ /* end of TIMER REGISTER BIT DEFINITIONS */

/**@}*/ /* end of TIMER Controller */
/**@}*/ /* end of REGISTER group */

#if defined (__CC_ARM)
#pragma no_anon_unions
#endif

#endif

