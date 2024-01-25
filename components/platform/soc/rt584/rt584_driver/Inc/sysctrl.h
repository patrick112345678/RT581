/**************************************************************************//**
 * @file     sysctrl.h
 * @version
 * @brief    SYS CONTROL driver header file
 *
 * @copyright
 ******************************************************************************/
#ifndef ___SYSCTRL_H__
#define ___SYSCTRL_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "RT584_cm33.h"

/*Only Secure register can control these registers*/
#define MAP_BASE         ((uint32_t)SYSCTRL + 0x10)
#define PULLOPT_BASE     ((uint32_t)SYSCTRL + 0x20)
#define DRV_BASE         ((uint32_t)SYSCTRL + 0x30)
#define OD_BASE          ((uint32_t)SYSCTRL + 0x38)


/*Define AHB SYSTEM CLOCK mode*/
#define SYS_CLK_32MHZ    0
#define SYS_CLK_48MHZ    1
#define SYS_CLK_64MHZ    2




/*Because compiler code optimize, we should set PLL_WAIT_PERIOD as 4N */
#define PLL_WAIT_PERIOD      800


/*FPGA later 2023/02/14 is NEW_PINMUX*/

#define RT584_NEW_PINMUX  1

#ifdef RT584_NEW_PINMUX

/*RT584 NEW PINMUX QSPI0 DOES NOT SUPPORT CSN2 CSN3 */
/*RT584 EXT 32K is not set in pinmux, set it in SYC_CRTL register SYS_CLK_CTRL */

#define MODE_GPIO            0
#define MODE_QSPI0           1
#define MODE_QSPI1           2

#define MODE_IR_OUT          3

#define MODE_QSPI0_CSN1      3
#define MODE_QSPI1_CSN1      3



#define MODE_I2S0_SDI_PIN06        3
#define MODE_I2S0_SDO_PIN07        3

#define MODE_I2S0_MCLK_PIN30       3
#define MODE_I2S0_MCLK_PIN31       3

#define MODE_I2S0                  4

#define MODE_UART1                 5

#define MODE_UART2                 6

#define MODE_I2S0_MCLK_PIN04       6
#define MODE_I2S0_MCLK_PIN05       6

#define MODE_IR_MODE7              7

#define MODE_UART                  7            /*NOTICE: UART0 mode become 7*/

#define MODE_I2CM0                 8
#define MODE_I2CM1                 9

#define MODE_I2C_SLAVE             10

#define MODE_SWCLK_PIN10           10
#define MODE_SWDIO_PIN11           10

#define MODE_PWM0                  11
#define MODE_PWM1                  12
#define MODE_PWM2                  13
#define MODE_PWM3                  14
#define MODE_PWM4                  15

#else

/* NOTICE: We don't consider race condition issue here.
 * So System after power up, it should initialize the pin mode ASAP, and don't change anymore.
 *
 */
#define MODE_GPIO            0
#define MODE_QSPI0           1
#define MODE_I2C             4
#define MODE_UART            6

#define MODE_I2S             4
#define MODE_PWM0            1
#define MODE_PWM1            2
#define MODE_PWM2            3
#define MODE_PWM3            5
#define MODE_PWM4            7

#define MODE_QSPI1           5

#define MODE_EXT32K          5

/*NOTICE: The following setting only in GPIO0~GPIO3*/
#define MODE_QSPI0_CSN1       2
#define MODE_QSPI0_CSN2       3
#define MODE_QSPI0_CSN3       6

#endif

/*Driving through setting mode*/
#define MODE_PULL_NONE         0
#define MODE_PULLDOWN_10K      1
#define MODE_PULLDOWN_100K     2
#define MODE_PULLDOWN_1M       3
#define MODE_PULLUP_10K        5
#define MODE_PULLUP_100K       6
#define MODE_PULLUP_1M         7

#define POWER_SAVING_NO_MASK              0x00000000
#define POWER_SAVING_MASK_BIT_RESERVED0   0x00000001             /**< bit0 */
#define POWER_SAVING_MASK_BIT_RESERVED1   0x00000002             /**< bit1 */
#define POWER_SAVING_MASK_BIT_RESERVED2   0x00000004             /**< bit2 */
#define POWER_SAVING_MASK_BIT_RESERVED3   0x00000008             /**< bit3 */
#define POWER_SAVING_MASK_BIT_RESERVED4   0x00000010             /**< bit4 */
#define POWER_SAVING_MASK_BIT_RESERVED5   0x00000020             /**< bit5 */
#define POWER_SAVING_MASK_BIT_RESERVED6   0x00000040             /**< bit6 */
#define POWER_SAVING_MASK_BIT_RESERVED7   0x00000080             /**< bit7 */
#define POWER_SAVING_MASK_BIT_RESERVED8   0x00000100             /**< bit8 */
#define POWER_SAVING_MASK_BIT_RESERVED9   0x00000200             /**< bit9 */
#define POWER_SAVING_MASK_BIT_RESERVED10  0x00000400             /**< bit10 */
#define POWER_SAVING_MASK_BIT_RESERVED11  0x00000800             /**< bit11 */
#define POWER_SAVING_MASK_BIT_RESERVED12  0x00001000             /**< bit12 */
#define POWER_SAVING_MASK_BIT_RESERVED13  0x00002000             /**< bit13 */
#define POWER_SAVING_MASK_BIT_RESERVED14  0x00004000             /**< bit14 */
#define POWER_SAVING_MASK_BIT_RESERVED15  0x00008000             /**< bit15 */
#define POWER_SAVING_MASK_BIT_RESERVED16  0x00010000             /**< bit16 */
#define POWER_SAVING_MASK_BIT_RESERVED17  0x00020000             /**< bit17 */
#define POWER_SAVING_MASK_BIT_RESERVED18  0x00040000             /**< bit18 */
#define POWER_SAVING_MASK_BIT_RESERVED19  0x00080000             /**< bit19 */
#define POWER_SAVING_MASK_BIT_RESERVED20  0x00100000             /**< bit20 */
#define POWER_SAVING_MASK_BIT_RESERVED21  0x00200000             /**< bit21 */
#define POWER_SAVING_MASK_BIT_RESERVED22  0x00400000             /**< bit22 */
#define POWER_SAVING_MASK_BIT_RESERVED23  0x00800000             /**< bit23 */
#define POWER_SAVING_MASK_BIT_RESERVED24  0x01000000             /**< bit24 */
#define POWER_SAVING_MASK_BIT_RESERVED25  0x02000000             /**< bit25 */
#define POWER_SAVING_MASK_BIT_RESERVED26  0x04000000             /**< bit26 */
#define POWER_SAVING_MASK_BIT_RESERVED27  0x08000000             /**< bit27 */
#define POWER_SAVING_MASK_BIT_RESERVED28  0x10000000             /**< bit28 */
#define POWER_SAVING_MASK_BIT_RESERVED29  0x20000000             /**< bit29 */
#define POWER_SAVING_MASK_BIT_RESERVED30  0x40000000             /**< bit30 */
#define POWER_SAVING_MASK_BIT_RESERVED31  0x80000000             /**< bit31 */

/*Define used for pin_set_pullopt */
#define PULL_NONE           0
#define PULL_DOWN_10K       1
#define PULL_DOWN_100K      2
#define PULL_DOWN_1M        3
#define PULL_UP_10K         5
#define PULL_UP_100K        6
#define PULL_UP_1M          7

/*Define used for pin_set_drvopt */
#define DRV_4MA             0
#define DRV_10MA            1
#define DRV_14MA            2
#define DRV_20MA            3

extern void power_saving_mask(uint32_t mask);
extern void power_saving_unmask(uint32_t unmask);
extern uint32_t get_power_saving_mask_status(void);


/* the following code should be in secure mode only??
 * ?????????????????????
 */

extern void power_setting(void);
extern void sleep_mode_init(void);

extern uint32_t pin_get_mode(uint32_t pin_number);

extern void pin_set_mode(uint32_t pin_number, uint32_t mode);

extern void enable_perclk(uint32_t clock);

extern void disable_perclk(uint32_t clock);

extern void set_sleep_option(uint32_t hclk_frozen_mode, uint32_t rtc_ds_wakeup_enable_mode);
/* Set pin pull option. */
extern void pin_set_pullopt(uint32_t pin_number, uint32_t mode);

/* Set pin driver mode*/
extern void pin_set_drvopt(uint32_t pin_number, uint32_t mode);

/*opendrain pin is for I2C used.*/
extern void enable_pin_opendrain(uint32_t pin_number);

extern void disable_pin_opendrain(uint32_t pin_number);

/*
 * Change CPU AHB CLOCK, 
 *      return STATUS_SUCCESS(0) for change success.
 *      return STATUS_ERROR      for change fail.
 * 
 */
extern uint32_t change_ahb_system_clk(uint32_t clk_mode);

/*
 * Get CPU AHB CLOCK, 
 *      return SYS_CLK_32MHZ      for CPU AHB 32MHz clock.
 *      return SYS_CLK_48MHZ      for CPU AHB 48MHz clock.
 *      return SYS_CLK_64MHZ      for CPU AHB 64MHz clock.
 * 
 */
extern uint32_t get_ahb_system_clk(void);

/*
 * Select Slow clock source.
 * Available mode:
 *         SLOW_CLOCK_INTERNAL   --- default value. 
 *                  If system don't call this function, then slow clock source is from internal RCO by default.
 *         
 *          SLOW_CLOCK_FROM_GPIO ---
 *                 If system set this mode, system should use an external 32K source from GPIO.
 *                
 *
 */
extern void set_slow_clock_source(uint32_t mode);


/*
__STATIC_INLINE void set_lowpower_control(uint32_t value)
{
    SYSCTRL->SYS_LOWPOWER_CTRL = value;
}

__STATIC_INLINE uint32_t get_lowpower_control(void)
{
    return ((uint32_t)(SYSCTRL->SYS_LOWPOWER_CTRL));
}
*/
__STATIC_INLINE uint32_t get_clk_control(void)
{
    return ((uint32_t)(SYSCTRL->SYS_CLK_CTRL));
}


#ifdef __cplusplus
}
#endif

#endif
