/**************************************************************************//**
 * @file     flashctl.h
 * @version
 * @brief    FLASHCTL driver header file
 *
 * @copyright
 ******************************************************************************/
#ifndef ___FLASHCTL_H__
#define ___FLASHCTL_H__

#ifdef __cplusplus
extern "C"
{
#endif


/** @addtogroup Standard_Driver Standard Driver
  @{
*/

/** @addtogroup FLASH_Driver FLASH Driver
  @{
*/

/** @addtogroup FLASH_EXPORTED_CONSTANTS FLASH Exported Constants
  @{
*/

/**
 * @brief Erase flash size definitions.
 */

typedef enum {
    FLASH_ERASE_PAGE,       /*!< Erase Page size  \hideinitializer */
    FLASH_ERASE_32K,        /*!< Erase 32K block \hideinitializer */
    FLASH_ERASE_64K,        /*!< Erase 64K block \hideinitializer */
    FLASH_ERASE_SECURE,     /*!< Erase Secure page \hideinitializer */
} flash_erase_mode_t;

/**
 * @brief  Status Register definitions
 */

#define  FLASH_STATUS_RW1     1       /*!< Status Register 1 \hideinitializer */ 
#define  FLASH_STATUS_RW2     2       /*!< Status Register 2 \hideinitializer */
#define  FLASH_STATUS_RW3     4       /*!< Status Register 3 \hideinitializer */


/**
 * @brief Define Secure Register address.
 * Here assume there are 3 secure 512 pages, one page is 512 bytes. 
 *
 */
#define  FLASH_SECREG_R1_P0   ((1<<12))            /*!< Secure Page one, low 256 bytes. \hideinitializer */ 
#define  FLASH_SECREG_R1_P1   ((1<<12) | 0x100)    /*!< Secure Page one, high 256 bytes.  \hideinitializer */ 

#define  FLASH_SECREG_R2_P0   ((2<<12))            /*!< Secure Page two, low 256 bytes. \hideinitializer */ 
#define  FLASH_SECREG_R2_P1   ((2<<12) | 0x100)    /*!< Secure Page two, high 256 bytes. \hideinitializer */

#define  FLASH_SECREG_R3_P0   ((3<<12))            /*!< Secure Page three, low 256 bytes. \hideinitializer */
#define  FLASH_SECREG_R3_P1   ((3<<12) | 0x100)    /*!< Secure Page three, high 256 bytes. \hideinitializer */

/*@}*/ /* end of group FLASH_EXPORTED_CONSTANTS */

#define FLASH_TYPE_ID_MAKS      0xFFFF

/**
 * @brief Define Flash timing
 *
 * NOTICE: different flash has different timing. We should set correct flash timing according to flash type.
 */

#define GDLQ_FLASH_TDP            3       /*!< LQ type flash deep power down (CS# High To Deep Power-Down Mode) and the unit is us */
#define GDLQ_FLASH_TRES1         20       /*!< LQ type flash release deep power down (CS# High To Standby Mode Without Electronic Signature) and the unit is us */
#define GDLQ_FLASH_TSUS          20       /*!< LQ type flash suspend time (CS# High To Next Command After Suspend) and the unit is us */
#define GDLQ_FLASH_TRS          100       /*!< LQ type flash resume time (Latency Between Resume And Next Suspend) and the unit is us */

#define GDWQ_FLASH_TDP            3       /*!< WQ type flash deep power down (CS# High To Deep Power-Down Mode) and the unit is us */
#define GDWQ_FLASH_TRES1         30       /*!< WQ type flash release deep power down (CS# High To Standby Mode Without Electronic Signature) and the unit is us */
#define GDWQ_FLASH_TSUS          40       /*!< WQ type flash suspend time (CS# High To Next Command After Suspend) and the unit is us */
#define GDWQ_FLASH_TRS          100       /*!< WQ type flash resume time (Latency Between Resume And Next Suspend) and the unit is us */

#define PUYA_FLASH_TDP            3       /*!< PUYA type flash deep power down (CS# High To Deep Power-Down Mode) and the unit is us */
#define PUYA_FLASH_TRES1         30       /*!< PUYA type flash release deep power down (CS# High To Standby Mode Without Electronic Signature) and the unit is us */
#define PUYA_FLASH_TSUS          40       /*!< PUYA type flash suspend time (CS# High To Next Command After Suspend) and the unit is us */
#define PUYA_FLASH_TRS          100       /*!< PUYA type flash resume time (Latency Between Resume And Next Suspend) and the unit is us */



/** @addtogroup FLASH_EXPORTED_FUNCTIONS FLASH Exported Functions
  @{
*/


/**
 * @brief Flash config structure holding flash timing configuration for flash
 *       suspend/resume/deep-powerdown.
 * @details
 *       Please read Flash Datasheet for suspend/resume/powerdown timing.
 *       The unit of this data member is in HCLK clock, for example, 
 *       if CPU running in 32MHz, and flash deep power down 
 *       is 3 us, then deep_pd_timing is 3*32, since 32 clock is 1 us.
 *         
 */
typedef struct {
    uint16_t   deep_pd_timing;        /*!< flash deep power down T_dp*/
    uint16_t   deep_rpd_timing;       /*!< flash release deep power down T_rdp*/
    uint16_t   suspend_timing;        /*!< flash suspend time T_sus*/
    uint16_t   resume_timing;         /*!< flash resume time T_rs*/
} flash_timing_mode_t;

/**
 * @brief Structure for status read/write
 *
 * @details
 *        require_mode is bitwise read/write operation.
 *        For example, if reqquired_mode = (FLASH_STATUS_RW1 | FLASH_STATUS_RW2)
 *        then the flash driver will read/write status1 register and status2 
 *        register, depends on read/write API.
 * 
 */

typedef struct {
    uint8_t   require_mode;     /*!< bitwise mode to indicate read/write operation */
    uint8_t   status1;          /*!< flash status1 for read/write*/
    uint8_t   status2;          /*!< flash status2 for read/write*/
    uint8_t   status3;          /*!< flash status3 for read/write*/
} flash_status_t;



/**
 * @brief Set Flash timing
 *
 * @param[in]  timing_cfg  Pointer to the flash timing
 *
 * @details Flash controller will use flash timing to control flash to enter 
 *      deeply powerdown and release from deeply powerdown mode, and suspend/   
 *      resume time.  
 * 
 */
void flash_set_timing(flash_timing_mode_t *timing_cfg);


/**
 * @brief Get Flash model/type ID
 *
 * @param None
 *
 * @retval    flash model identifier.
 * 
 */

uint32_t flash_get_deviceinfo(void);


/**
 * @brief Get flash status register
 *
 * @param[in] status  The pointer of flash_status_t
 *
 * @retval    None
 *
 * @details  Read status will be save in status.status1, status.status2
 *       status.status3, depends on status.require_mode.
 *       require_mode is bitwise operation indicator, so it can read
 *       multiple status byte in one function call. 
 */
 
void flash_get_status_reg(flash_status_t *status);


/**
 * @brief Set flash status register
 *
 * @param[in] status  The pointer of flash_status_t
 *
 * @retval    None
 *
 * @details  Flash driver will write status of status.status1, status.status2
 *       status.status3 into flash, depends on status.require_mode.
 *       require_mode is bitwise operation indicator, so it can write
 *       multiple status byte in one function call. 
 *       Notice: PLEASE read flash datasheet for write status 
 *       setting information before you write. Some status bitfield is
 *       OTP bit, it just write once and can not erase!
 *       This function is BLOCK function. 
 */
 
void flash_set_status_reg(const flash_status_t *status);


/**
 * @brief Read flash page data
 *
 * @param[out] buf_addr  Specify the address for the read data of the page.
 *
 * @param[in]  read_page_addr Specify the page address in flash that required to read. 
 *
 * @retval    None
 *
 * @details  buf_addr must be 4-bytes alignment. One page is 256 bytes, so
 *           buf_addr must have 256 bytes for read data, otherwise some data 
 *           will be overwritten. Please notice that read_page_addr will be
 *           ignored the LSB 8 bits, that is LSB 8 bits of address is don't care.
 *           This function is BLOCK function.
 * 
 */
 
void flash_read_page(uint32_t buf_addr, uint32_t read_page_addr);

/**
 * @brief Read one byte from flash
 *
 *
 * @param[in]  read_addr Specify the byte address in flash that required to read. 
 *
 * @retval    The data in read_addr of flash
 *
 * @details  read one byte from flash address "read_addr".
 *           This function is BLOCK function.
 */
uint8_t flash_read_byte(uint32_t read_addr);

/*write/program flash page data*/
/**
 * @brief Write/Program flash page data
 *
 * @param[in] buf_addr  Specify the address of the write data.
 *
 * @param[in] write_page_addr Specify the page address in flash that to be written. 
 *
 * @retval    None
 *
 * @details  buf_addr must be 4-bytes alignment. One page is 256 bytes, so
 *           buf_addr must have 256 bytes for write data, if the data you 
 *           want to write is less than 256 bytes, please write "0xFF" for
 *           padding bytes. Please notice that write_page_addr will be
 *           ignored the LSB 8 bits, that is LSB 8 bits of address is don't care. 
 *           This function is BLOCK function. Please don't call this function in ISR. 
 */
void flash_write_page(uint32_t buf_addr, uint32_t write_page_addr);

/*program secure register data*/

/**
 * @brief Write/Program flash secure register
 *
 * @param[in] buf_addr  Specify the address of the write data into the page.
 *
 * @param[in] write_reg_addr Specify the secure register address in flash to write. 
 *
 * @retval    STATUS_INVALID_PARAM  -- write_reg_addr is invalid value.
 *            STATUS_SUCCESS --- write register opertation finish.
 *
 * @details  buf_addr must be 4-bytes alignment. One page is 256 bytes, so
 *           buf_addr must have 256 bytes for write data, if the data you 
 *           want to write is less than 256 bytes, please write "0xFF" for
 *           padding bytes. Please notice that write_page_addr must be
 *           256 bytes alignment, that is LSB 8 bits of address must be zero.
 *           This function is BLOCK function. 
 */
uint32_t flash_write_sec_register(uint32_t buf_addr, uint32_t write_reg_addr);

/*read secure register data*/

/**
 * @brief Read flash secure register
 *
 * @param[out] buf_addr  Specify the address of the read data to return.
 *
 * @param[in] read_reg_addr Specify the secure register address in flash to read. 
 *
 * @retval    STATUS_INVALID_PARAM  -- read_reg_addr is invalid value.
 *            STATUS_SUCCESS --- read register opertation finish.
 *
 * @details  buf_addr must be 4-bytes alignment. One page is 256 bytes, so
 *           buf_addr must have 256 bytes for read secure register data, 
 *           otherwise some data will be overwritten.
 *           This function is BLOCK function. 
 */
uint32_t flash_read_sec_register(uint32_t buf_addr, uint32_t read_reg_addr);

/*erase sector, block*/
/**
 * @brief Erase Sector/Blcok Function
 *
 * @param[in] mode  Specify the ease mode.
 *
 * @param[in] flash_addr Specify the start address in flash to be erased. 
 *
 * @retval    STATUS_INVALID_PARAM  -- mode is invalid value.
 *            STATUS_SUCCESS --- erase opertation start to processing.
 *
 * @details  This function is asynchronous NON-BLOCKING mode. That is,
 *      when this function returns, it does NOT mean the erase operation
 *      finish. It just means the erasing operation is in progressing.
 *      User should call function flash_check_busy to check erase finish or
 *      not. No other erase or program instruction can be called before 
 *      this erase function finish.
 *
 */
uint32_t flash_erase(flash_erase_mode_t mode, uint32_t flash_addr);

/*write one byte to flash address "write_addr"*/

/**
 * @brief Write/Program flash one byte data
 *
 * @param[in] write_addr  Specify the address of the flash to be written.
 *
 * @param[in] singlebyte  Specify the byte that to be written into the flash. 
 *
 * @retval    None
 *
 * @details  this function will write "singlebyte" data to flash address "write_addr".
 *           This function is block function.
 */
 
void flash_write_byte(uint32_t write_addr, uint8_t singlebyte);

/*check the page  crc*/
/**
 * @brief Check the page crc
 *
 * @param[in] read_page_addr  Specify the address of the flash to be verified.
 *
 * @retval    None
 *
 * @details  This function is asynchronous NON-BLOCKING mode. That is,
 *      when this function returns, it does NOT mean the operation
 *      finish. It just means the counting crc8 operation is in progressing.
 *      User should call function flash_check_busy to check verify finish or
 *      not. If the function counts CRC8 finish, user can call flash_get_crc
 *      to get the CRC8 value. then user can compare the return CRC8 value 
 *      with pre-caculate CRC8 value to verify write page data is correct or not.   
 *
 */
void flash_verify_page(uint32_t read_page_addr);

/*get flash unique ID. ID could be used for some random number generator. */

/**
 * @brief Get unique flash identifier. 
 *
 * @param[out] flash_id_buf_addr  Specify the address for return unique id. 
 *
 * @param[in] buf_length  Specify the length of unique id in byte it requirs to read.
 *
 * @retval    STATUS_INVALID_PARAM  -- if buf_length is zero.
 *			  STATUS_SUCCESS 
 *
 *
 * @details   flash_id_buf_addr must be 4-bytes alignment. Each flash has one unique
 *            identifier. The length of the uid depends on flash manufacture. Please
 *            check flash datasheet for this uid length. 
 *
 */
 
uint32_t flash_get_unique_id(uint32_t flash_id_buf_addr, uint32_t buf_length);


void flash_timing_init(uint32_t HCLK_MHZ);

/**
 * @brief Check Flash busy state
 *
 * @param  None
 *
 * @retval    0 --- flash is not busy. 
 *			  1 --- flash is in busy state.
 *
 * @details   Because some flash functions are non-blocking functions, 
 *        flash controller should provide this polling fucntion to help 
 *        user to know the flash request finished or not. 
 *
 */
 
/*check Flash command Finish*/
__STATIC_INLINE uint32_t flash_check_busy()
{
    return ((FLASH->START)&BUSYBIT)? 1:0;
}

/**
 * @brief Get flash read page verification CRC8 value
 *
 * @param     NONE
 *
 * @retval    CRC8  
 *
 *
 * @details   This function return CRC8 of the page that specify in previous
 *        funciton flash_verify_page(...).
 *
 */
 
/*check Flash CRC*/
__STATIC_INLINE uint32_t flash_get_crc()
{
    return (FLASH->CRC);
}

/*set Flash to 4bit SPI mode*/

/**
 * @brief Set flash opeation in 4bits SPI mode
 *
 * @param     NONE
 *
 * @retval    NONE 
 *
 * @details   When MCU reboots, the flash controller will operate in 1-bit SPI 
 *       mode by default. To speed up the read, user can call function flash_enable_qe()
 *       to enable controller to operate in 4-bits mode. Because most flash operations are
 *       read operations, this 4-bits mode is very important for code XIP execution and read.
 */

__STATIC_INLINE void flash_enable_qe(void)
{
    FLASH->CONTROL_SET = 0x3EF;     /*This setting is for MP IC*/
}

/* Set readpage size. Here we fix to read 256 bytes.
 * Do not change it*/
 
/**
 * @brief Set flash read page size 
 *
 * @param    NONE
 *
 * @retval   NONE
 *
 * @details  This function set read page length be 256 bytes by default.  
 *
 */

__STATIC_INLINE void flash_set_read_pagesize(void)
{
    FLASH->PAGE_READ_WORD = 0xFF;
}



#define GDWQ_ID                 0x65C8
#define GDLQ_ID                 0x60C8
#define PUYA_ID                 0x6085

/*@}*/ /* end of group FLASH_EXPORTED_CONSTANTS */

#ifdef __cplusplus
}
#endif

#endif
