/**************************************************************************//**
 * @file    i2cm_reg.h
 * @version
 * @brief    I2C MASTER Register defined
 *
 * @copyright
 ******************************************************************************/

#ifndef ___I2CM_NEWIP_REG_H__
#define ___I2CM_NEWIP_REG_H__

#if defined ( __CC_ARM   )
#pragma anon_unions
#endif


/**
   @addtogroup REGISTER Control Register
   @{
*/

/**
    @addtogroup  I2C Master Controller
    Memory Mapped Structure for I2C Master Controller
@{ */

typedef struct {
    __IO  uint32_t  CONTROL;
    __IO  uint32_t  TAR;
    __IO  uint32_t  BUF;
    __I   uint32_t  INT_STATUS;
    __IO  uint32_t  INT_ENABLE;
    __I   uint32_t  INT_RAW_STATUS;
    __IO  uint32_t  INT_CLEAR;
    __IO  uint32_t  SLCK_GEN;
} I2C_MASTER_T;


/**
    @addtogroup I2C REGISTER BIT DEFINITIONS

@{ */

#define I2CM_CONTROL_ENABLE                    0x01
#define I2CM_CONTROL_RESTART                   0x02
#define I2CM_CONTROL_STOP_EN                   0x04
#define I2CM_CONTROL_BUS_CLEAR                 0x08
#define I2CM_CONTROL_FIFO_CLEAR                0x10

// interrupt mask register bit definitions
#define I2CM_INT_RX_UNDER                      0x0001
#define I2CM_INT_RX_OVER                       0x0002
#define I2CM_INT_RX_FULL                       0x0004
#define I2CM_INT_RX_FINISH                     0x0008
#define I2CM_INT_TX_OVER                       0x0010
#define I2CM_INT_TX_EMPTY                      0x0020
#define I2CM_INT_ADDR_NACK                     0x0040
#define I2CM_INT_WRITE_NACK                    0x0080
#define I2CM_INT_LOSTARB                       0x0100
#define I2CM_INT_IDLE                          0x0200

#define I2CM_INT_MASK_ALL                      0x03FF

#define I2CM_FIFO_NUM                          16




/**@}*/ /* end of I2C REGISTER BIT DEFINITIONS */

/**@}*/ /* end of I2C Master Controller */

/**@}*/ /* end of REGISTER group */

#if defined ( __CC_ARM   )
#pragma no_anon_unions
#endif

#endif
