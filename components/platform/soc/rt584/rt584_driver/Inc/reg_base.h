/**************************************************************************//**
 * @file     reg_base.h
 * @version
 * @brief    Register Base defined
 *
 * @copyright
 ******************************************************************************/

#ifndef ___REG_BASE_H__
#define ___REG_BASE_H__

#if defined ( __CC_ARM   )
#pragma anon_unions
#endif
/* ================================================================================ */
/* ================            Peripheral Secure define            ================ */
/* ================================================================================ */
#define SYSCTRL_SECURE_EN                    1
#define GPIO_SECURE_EN                       1
#define RTC_SECURE_EN                        1
#define DPD_SECURE_EN                        1
#define SOC_PMU_SECURE_EN                    1
#define FLASHCTRL_SECURE_EN                  1
#define TIMER0_SECURE_EN                     1
#define TIMER1_SECURE_EN                     1
#define TIMER2_SECURE_EN                     1
#define TIMER32K0_SECURE_EN                  1
#define TIMER32K1_SECURE_EN                  1

#define WDT_SECURE_EN                        1
#define UART0_SECURE_EN                      1
#define UART1_SECURE_EN                      1
#define PWM0_SECURE_EN                       1
#define PWM1_SECURE_EN                       1
#define PWM2_SECURE_EN                       1
#define PWM3_SECURE_EN                       1
#define PWM4_SECURE_EN                       1
#define I2C_SLAVE_SECURE_EN                  1
#define COMM_SUBSYSTEM_AHB_SECURE_EN         1
#define BOD_COMP_SECURE_EN                   1
#define AUX_COMP_SECURE_EN                   1

#define QSPI0_SECURE_EN                      1
#define QSPI1_SECURE_EN                      1
#define TRNG_SECURE_EN                       1
#define IRM_SECURE_EN                        1
#define UART2_SECURE_EN                      1
#define XDMA_SECURE_EN                       1
#define DMA0_SECURE_EN                       1
#define DMA1_SECURE_EN                       1
#define I2C_MASTER0_SECURE_EN                1
#define I2C_MASTER1_SECURE_EN                1
#define I2S0_SECURE_EN                       1
#define SADC_SECURE_EN                       1

#define SW_IRQ0_SECURE_EN                    1
#define SW_IRQ1_SECURE_EN                    1

#define CRYPTO_SECURE_EN                     1

/* ================================================================================ */
/* ================              Peripheral memory map             ================ */
/* ================================================================================ */

#define SRAM_BASE                       (0x20000000UL)  /*!< (SRAM      ) Base Address */
#define PERIPH_BASE                     (0x40000000UL)  /*!< (Peripheral) Base Address */
#define PERIPH_SECURE_OFFSET            (0x10000000UL)  /*!< (SECURE Peripheral) Offset */

#define SYSCTRL_BASE                    (PERIPH_BASE)
#define GPIO_BASE                       (PERIPH_BASE+0x1000UL)
#define SEC_CTRL_SECBASE                (PERIPH_BASE+0x3000UL+PERIPH_SECURE_OFFSET)
#define RTC_BASE                        (PERIPH_BASE+0x4000UL)
#define DPD_BASE                        (PERIPH_BASE+0x5000UL)
#define SOC_PMU_BASE                    (PERIPH_BASE+0x6000UL)
#define FLASHCTRL_BASE                  (PERIPH_BASE+0x9000UL)
#define TIMER0_BASE                     (PERIPH_BASE+0xA000UL)
#define TIMER1_BASE                     (PERIPH_BASE+0xB000UL)
#define TIMER2_BASE                     (PERIPH_BASE+0xC000UL)
#define TIMER32K0_BASE                  (PERIPH_BASE+0xD000UL)
#define TIMER32K1_BASE                  (PERIPH_BASE+0xE000UL)

#define WDT_BASE                        (PERIPH_BASE+0x10000UL)
#define UART0_BASE                      (PERIPH_BASE+0x12000UL)
#define UART1_BASE                      (PERIPH_BASE+0x13000UL)
#define PWM0_BASE                       (PERIPH_BASE+0x16000UL)
#define PWM1_BASE                       (PERIPH_BASE+0x16100UL)
#define PWM2_BASE                       (PERIPH_BASE+0x16200UL)
#define PWM3_BASE                       (PERIPH_BASE+0x16300UL)
#define PWM4_BASE                       (PERIPH_BASE+0x16400UL)
#define I2C_SLAVE_BASE                  (PERIPH_BASE+0x18000UL)
#define COMM_SUBSYSTEM_AHB_BASE         (PERIPH_BASE+0x1A000UL)
#define BOD_COMP_BASE                   (PERIPH_BASE+0x1D000UL)
#define AUX_COMP_BASE                   (PERIPH_BASE+0x1E000UL)

#define QSPI0_BASE                      (PERIPH_BASE+0x20000UL)
#define QSPI1_BASE                      (PERIPH_BASE+0x21000UL)
#define TRNG_BASE                       (PERIPH_BASE+0x23000UL)
#define IRM_BASE                        (PERIPH_BASE+0x24000UL)
#define UART2_BASE                      (PERIPH_BASE+0x25000UL)
#define XDMA_BASE                       (PERIPH_BASE+0x28000UL)
#define DMA0_BASE                       (PERIPH_BASE+0x29000UL)
#define DMA1_BASE                       (PERIPH_BASE+0x2A000UL)
#define I2C_MASTER0_BASE                (PERIPH_BASE+0x2B000UL)
#define I2C_MASTER1_BASE                (PERIPH_BASE+0x2C000UL)
#define I2S0_BASE                       (PERIPH_BASE+0x2D000UL)
#define SADC_BASE                       (PERIPH_BASE+0x2F000UL)

#define SW_IRQ0_BASE                    (PERIPH_BASE+0x30000UL)
#define SW_IRQ1_BASE                    (PERIPH_BASE+0x31000UL)

#define CRYPTO_BASE                     (PERIPH_BASE+0x40000UL)


#if defined ( __CC_ARM   )
#pragma no_anon_unions
#endif

#endif
