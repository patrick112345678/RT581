
#ifndef __I2C_SLAVE_REG_H__
#define __I2C_SLAVE_REG_H__

#if defined ( __CC_ARM   )
#pragma anon_unions
#endif


/**
   @addtogroup REGISTER Control Register
   @{
*/

/**
    @addtogroup  I2C Slave Controller
    Memory Mapped Structure for I2C Master Controller
@{ */

typedef struct {
    __IO  uint32_t  RX_DATA;             /*0x00*/
    __IO  uint32_t  I2C_SLAVE_ADDR;      /*0x04*/
    __IO  uint32_t  I2C_INT_ENABLE;      /*0x08*/
    __IO  uint32_t  I2C_INT_STATUS;      /*0x0C*/
    __IO  uint32_t  I2C_TIMEOUT;         /*0x10*/
    __IO  uint32_t  I2C_SLAVE_ENABLE;    /*0x14*/
    __I   uint32_t  I2C_SLAVE_STATUS;    /*0x18*/
} I2C_SLAVE_T;

#define  WR_DATA     RX_DATA    

#define  I2C_SLAVE_MATCH_ADDR             (0x01)
#define  I2C_SLAVE_DATA_READY             (0x02)
#define  I2C_SLAVE_BUS_STOP               (0x04)
#define  I2C_SLAVE_BUS_TIMEOUT            (0x08)

#define  I2C_SLAVE_ALL_INT          (I2C_SLAVE_MATCH_ADDR | I2C_SLAVE_DATA_READY | I2C_SLAVE_BUS_STOP | I2C_SLAVE_BUS_TIMEOUT)

#define  I2C_SLAVE_READ_OP                (1<<1)

/**@}*/ /* end of GPIO Controller */
/**@}*/ /* end of REGISTER group */

#if defined ( __CC_ARM   )
#pragma no_anon_unions
#endif

#endif
