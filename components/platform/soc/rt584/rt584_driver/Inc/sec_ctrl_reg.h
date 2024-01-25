/**************************************************************************//**
 * @file     
 * @version  V1.00
 * @brief    RafaelMicro Secure Control register definition header file
 *
 * @copyright (C) 2023 Rafael Microelectronics Inc. All rights reserved.
 *****************************************************************************/
#ifndef __RT_SEC_CONTROL_REG_H__
#define __RT_SEC_CONTROL_REG_H__

/**
   @addtogroup REGISTER Control Register
   @{
*/

typedef union sec_int_en_ctrl_s {
    struct sec_int_en_ctrl_b {       
        uint32_t SEC_EN_ROM_ERR_INT                  : 1;
        uint32_t SEC_EN_FLASH_ERR_INT                : 1;        
        uint32_t SEC_EN_RAM_ERR_INT                  : 1;
        uint32_t SEC_EN_PERI_ERR_INT                 : 1;
        uint32_t SEC_EN_CRYPTO_ERR_INT               : 1;
        uint32_t SEC_EN_PUF_ERR_INT                  : 1;
        uint32_t RESERVED1                           : 26;
    } bit;
    uint32_t reg;
} sec_int_en_ctrl_t;


typedef union sec_int_clr_ctrl_s {
    struct sec_int_clr_ctrl_b {       
        uint32_t SEC_CLR_ROM_ERR_INT                  : 1;
        uint32_t SEC_CLR_FLASH_ERR_INT                : 1;        
        uint32_t SEC_CLR_RAM_ERR_INT                  : 1;
        uint32_t SEC_CLR_PERI_ERR_INT                 : 1;
        uint32_t SEC_CLR_CRYPTO_ERR_INT               : 1;
        uint32_t SEC_CLR_PUF_ERR_INT                  : 1;
        uint32_t RESERVED1                            : 26;
    } bit;
    uint32_t reg;
} sec_int_clr_ctrl_t;


typedef union sec_int_status_ctrl_s {
    struct sec_int_status_ctrl_b {       
        uint32_t SEC_STATUS_ROM_ERR_INT                  : 1;
        uint32_t SEC_STATUS_FLASH_ERR_INT                : 1;        
        uint32_t SEC_STATUS_RAM_ERR_INT                  : 1;
        uint32_t SEC_STATUS_PERI_ERR_INT                 : 1;
        uint32_t SEC_STATUS_CRYPTO_ERR_INT               : 1;
        uint32_t SEC_STATUS_PUF_ERR_INT                  : 1;
        uint32_t RESERVED1                               : 26;
    } bit;
    uint32_t reg;
} sec_int_status_ctrl_t;


/**
    @addtogroup  RafaelMicro Secure Controller Register    
    Memory Mapped Structure for Secure Controller
@{ */

typedef struct {
    __IO uint32_t SEC_FLASH_SEC_SIZE;     /*0x00*/
    
    __IO uint32_t SEC_FLASH_NSC_START;    /*0x04*/
    __IO uint32_t SEC_FLASH_NSC_STOP;     /*0x08*/
    
    __IO uint32_t SEC_FLASH_NS_STOP;      /*0x0C*/
    
    __IO uint32_t SEC_RAM_SEC_SIZE;       /*0x10*/
    
    __IO uint32_t SEC_RAM_NSC_START;      /*0x14*/
    __IO uint32_t SEC_RAM_NSC_STOP;       /*0x18*/
    __IO uint32_t RESV2;                  /*0x1C*/
    
    __IO uint32_t SEC_PERI_ATTR0;         /*0x20*/
    __IO uint32_t SEC_PERI_ATTR1;         /*0x24*/
    __IO uint32_t SEC_PERI_ATTR2;         /*0x28*/    
    __IO uint32_t SEC_IDAU_CTRL;          /*0x2C*/
    
    __IO sec_int_en_ctrl_t SEC_INT_EN;             /*0x30*/
    
    __IO sec_int_clr_ctrl_t SEC_INT_CLR;            /*0x34*/
    __IO sec_int_status_ctrl_t SEC_INT_STATUS;         /*0x38*/    
    __IO uint32_t RESV1;                  /*0x3C*/
    
    __IO uint32_t SEC_MCU_DEBUG;          /*0x40*/
    __IO uint32_t SEC_LOCK_MCU_CTRL;      /*0x44*/    
} SEC_CTRL_T;


#define ENABLE_IDAU_SEC_CTRL               1
#define DISABLE_IDAU_SEC_CTRL              0


#define SEC_ROM_ERR_INT                 (1<<0)
#define SEC_FLASH_ERR_INT               (1<<1)
#define SEC_RAM_ERR_INT                 (1<<2)
#define SEC_PERI_ERR_INT                (1<<3)
#define SEC_CRYPTO_ERR_INT              (1<<4)


#define SEC_DBGEN                       (1)
#define SEC_SPIDEN                      (2)
#define SEC_NIDEN                       (4)
#define SEC_SPNIDEN                     (8)


#define SEC_LOCK_SVTAIRCR               (1<<0)
#define SEC_LOCK_NSVTOR                 (1<<1)
#define SEC_LOCK_SMPU                   (1<<1)
#define SEC_LOCK_NSMPU                  (1<<3)
#define SEC_LOCK_SAU                    (1<<4)


/*attribute setting*/

/*For REG SEC_PERI_ATTR0*/

#define SEC_IDAU_SYS_CTRL               (1UL << 0)
#define SEC_IDAU_GPIO                   (1UL << 1)

#define SEC_IDAU_SEC_CTRL               (1UL << 3)
#define SEC_IDAU_RTC                    (1UL << 4)
#define SEC_IDAU_DPD_CTRL               (1UL << 5)
#define SEC_IDAU_SOC_PMU                (1UL << 6)

#define SEC_IDAU_CACHECTRL              (1UL << 8)
#define SEC_IDAU_FLASH_CTRL             (1UL << 9)

#define SEC_IDAU_TIMER0                 (1UL << 10)
#define SEC_IDAU_TIMER1                 (1UL << 11)
#define SEC_IDAU_TIMER2                 (1UL << 12)

#define SEC_IDAU_TIMER32K0              (1UL << 13)
#define SEC_IDAU_TIMER32K1              (1UL << 14)

#define SEC_IDAU_WDT                    (1UL << 16)

#define SEC_IDAU_UART0                  (1UL << 18)
#define SEC_IDAU_UART1                  (1UL << 19)

#define SEC_IDAU_PWM                    (1UL << 22)

#define SEC_IDAU_I2C_SLAVE              (1UL << 24)

#define SEC_IDAU_COMM_AHB               (1UL << 26)

/*For REG SEC_PERI_ATTR1*/
#define SEC_IDAU_QSPI0                  (1UL << 0)
#define SEC_IDAU_QSPI1                  (1UL << 1)

#define SEC_IDAU_TRNG                   (1UL << 3)
#define SEC_IDAU_UART2                  (1UL << 5)

#define SEC_IDAU_OTP                    (1UL << 7)
#define SEC_IDAU_XDMA                   (1UL << 8)

#define SEC_IDAU_DMA0                   (1UL << 9)
#define SEC_IDAU_DMA1                   (1UL << 10)

#define SEC_IDAU_I2C_MASTER0            (1UL << 11)
#define SEC_IDAU_I2C_MASTER1            (1UL << 12)

#define SEC_IDAU_I2S0_MASTER            (1UL << 13)

#define SEC_IDAU_SADC                   (1UL << 15)
#define SEC_IDAU_SW_IRQ0                (1UL << 16)
#define SEC_IDAU_SW_IRQ1                (1UL << 17)

#define SEC_IDAU_AES_CCM                (1UL << 18)

/*For REG SEC_PERI_ATTR2*/
#define SEC_IDAU_CRYPTO                 (1UL << 0)

#endif

