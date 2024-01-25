/**************************************************************************//**
 * @file     i2c_slave.h
 * @version
 * @brief    I2C slave driver header file
 *
 * @copyright
 ******************************************************************************/
#ifndef __I2C_SLAVE_DRV_H__
#define __I2C_SLAVE_DRV_H__

#include "RT584_cm33.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef void (*i2c_slave_proc_cb_t)(uint32_t status);

#define I2C_SLAVE_STATUS_ADDR_MATCH          (1<<0)
#define I2C_SLAVE_STATUS_DATA_READY          (1<<1)
#define I2C_SLAVE_STATUS_STOP                (1<<2)
#define I2C_SLAVE_STATUS_TIMEOUT             (1<<3)

#define I2C_SLAVE_STATUS_READ                (1<<4)
#define I2C_SLAVE_STATUS_WRITE               (1<<5)


/**
 * @brief 
 * @details
 *         
 */
typedef struct 
{    
    i2c_slave_proc_cb_t  i2c_slave_cb_func;  /*!< i2c slave callback function \hideinitializer */
    uint8_t  i2c_bus_timeout_enable;         /*!< i2c bus timeout enable \hideinitializer */
    uint8_t  i2c_bus_timeout;                /*!< i2c bus timeout value  \hideinitializer */
    uint8_t  i2c_slave_addr;                 /*!< i2c slave 7 bits only  \hideinitializer */
    uint8_t  resv;
} i2c_slave_mode_t;


/**
 * @brief Set I2C slave initialize
 *
 * @param[in]  
 *
 * @retval
 *       STATUS_SUCCESS
 *       STATUS_INVALID_REQUEST   --- I2C master is not in idle mode.
 *
 * @details
 *       Call this function to initail I2C slave, the whole I2C slave 
 *       driver is interrupt-driven, all i2c slave response are processing
 *       in user "i2c_slave_cb_func" callback function. Please Notice the
 *       the function "i2c_slave_cb_func" is in ISR context, so finish the 
 *       task ASAP. You can NOT block the function!
 */

extern uint32_t i2c_slave_open(i2c_slave_mode_t *i2c_slave_client);


/**
 * @brief Close I2C slave 
 *
 * @param[in]  
 *
 * @retval
 *       STATUS_SUCCESS
 *       STATUS_INVALID_REQUEST   
 *
 * @details
 *       This function is disable I2C slave client function.       
 *
 */
 
extern uint32_t i2c_slave_close(void);

/**
 *   @brief  Get one byte from i2c master
 * 
 *   @details
 *   Notice: I2C read or write status is from I2C Master viewpoint...
 *   That is, when I2C slave get a write flag, slave should read a byte to RX_DATA
 *   Call this function when I2C slave receive a "write flag" Data Ready interrupt 
 *
 */

__STATIC_INLINE uint8_t I2C_SLAVE_READ_BYTE(void)
{
    return (I2C_SLAVE->RX_DATA & 0xFF);
}

/**
 *   @brief  Send one byte to i2c master
 * 
 *   @details
 *   Notice: I2C read or write status is from I2C Master viewpoint...
 *   That is, when I2C slave get a read flag, slave should write a byte to TX_DATA
 *   Call this function when I2C slave receive a "read flag" Data Ready interrupt 
 *
 */

__STATIC_INLINE void I2C_SLAVE_WRITE_BYTE(uint8_t data)
{
    I2C_SLAVE->WR_DATA = data;
}


#ifdef __cplusplus
}
#endif

#endif

