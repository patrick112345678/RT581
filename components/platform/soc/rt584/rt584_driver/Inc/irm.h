/**************************************************************************//**
 * @file     irm.h
 * @version
 * @brief    IRM driver header file
 *
 * @copyright
 ******************************************************************************/
#ifndef __IRM_H__
#define __IRM_H__

#ifdef __cplusplus
extern "C"
{
#endif
typedef void (*irm_isr_t)(void);
/** @addtogroup Standard_Driver Standard Driver
  @{
*/

void IRM_test(void);
void irm_isr(void);
/** @addtogroup GPIO_EXPORTED_CONSTANTS GPIO Exported Constants
  @{
*/
#define IR_ENV_INI_HIGH (BIT0)
#define IR_ENV_INI_LOW (0)

#define IR_MODE_MASK ((~BIT1))
#define IR_OUT_MODE_MASK ((~(BIT2|BIT3)))

#define IR_BUFFER_MAX_LENGTH (2*8*32)//2(high+low)*8bits*32, i.e, 32bytes 
#define IR_BUFFER_FIRST_FILLIN_LENGTH (16)
#define IR_BUFFER_INT_LENGTH (4)// HALF OF IR_BUFFER_FIRST_FILLIN_LENGTH

#define IR_EVELOPE_HIGH (1)
#define IR_EVELOPE_LOW (0)

////// DEINE FOR NEC ///// 
#define NEC_CARRIER_LOW_CNT (1)
#define NEC_CARRIER_HIGH_CNT (0)
#define NEC_CARRIER_BASEMENT_CNT (281)

#define NEC_OFFSET (1)


#if 1
#define NEC_START_HIGHT (342+NEC_OFFSET)//9000us / 26.34375 us = 341.6
#define NEC_START_LOW (171+NEC_OFFSET)//4500us / 26.34375 us = 170.8
#define NEC_ONE_HIGHT (21+NEC_OFFSET)//560us / 26.34375 us = 21.25
#define NEC_ONE_LOW (64+NEC_OFFSET)//1690us / 26.34375 us = 64.1
#define NEC_ZERO_HIGHT (21+NEC_OFFSET)//560us / 26.34375 us = 21.25
#define NEC_ZERO_LOW (21+NEC_OFFSET)//560us / 26.34375 us = 21.25
#define NEC_END_BUST (21+NEC_OFFSET)//560us / 26.34375 us = 21.25
#define NEC_REPEAT_LOW (85+NEC_OFFSET)//2250us / 26.34375 us = 85.40
#define NEC_110MS (4176+NEC_OFFSET)//(4176)//110000us / 26.34375 us = 4175.5
#else
#define NEC_START_HIGHT (360+NEC_OFFSET)//9000us / 25 us = 360
#define NEC_START_LOW (144+NEC_OFFSET)//4500us / 25 us = 180
#define NEC_ONE_HIGHT (22+NEC_OFFSET)//560us / 25 us = 22.4
#define NEC_ONE_LOW (67+NEC_OFFSET)//1690us / 25 us = 67.6
#define NEC_ZERO_HIGHT (22+NEC_OFFSET)//560us / 25 us = 22.4
#define NEC_ZERO_LOW (22+NEC_OFFSET)//560us / 25 us = 22.4
#define NEC_END_BUST (22+NEC_OFFSET)//560us / 25 us = 22.4
#define NEC_REPEAT_LOW (90+NEC_OFFSET)//2250us / 25 us = 90
#define NEC_110MS (4400+NEC_OFFSET)//110000us / 25 us = 3520
#endif

////// DEINE FOR SIRC///// 
#define SIRC_CARRIER_LOW_CNT (1)
#define SIRC_CARRIER_HIGH_CNT (0)
#define SIRC_CARRIER_BASEMENT_CNT (267)
#define SIRC_OFFSET (-1)
#define SIRC_START_HIGHT (96+SIRC_OFFSET)//2400us / 25 us = 96
#define SIRC_START_LOW (24+SIRC_OFFSET)//600us / 25 us = 24
#define SIRC_ONE_HIGHT (48+SIRC_OFFSET)//1200us / 25 us = 48
#define SIRC_ONE_LOW (SIRC_START_LOW)
#define SIRC_ZERO_HIGHT (SIRC_START_LOW)//600us / 25 us = 24
#define SIRC_ZERO_LOW (SIRC_START_LOW)
#define SIRC_END_LOW (400+SIRC_START_LOW)//45000us+600us / 25 us = 1800+24

////// DEINE FOR RC6///// 
#define RC6_CARRIER_LOW_CNT (1)
#define RC6_CARRIER_HIGH_CNT (0)
#define RC6_CARRIER_BASEMENT_CNT (296)
#define RC6_OFFSET (0)
#define RC6_START_HIGHT (96+RC6_OFFSET)//2666us = t*6 = 1/36000*16*6
#define RC6_START_LOW (32+RC6_OFFSET)//889us = t*62 = 1/36000*16*2
#define RC6_TR_HIGHT (32+RC6_OFFSET)//889us = t*2 = 1/36000*16*2
#define RC6_TR_LOW (32+RC6_OFFSET)//889us = t*2 = 1/36000*16*2



#define RC6_ONE_HIGHT (16+RC6_OFFSET)//444us = t = 1/36000*16
#define RC6_ONE_LOW (RC6_ONE_HIGHT)//444us = t = 1/36000*16
#define RC6_ZERO_HIGHT (RC6_ONE_HIGHT)//444us = t = 1/36000*16
#define RC6_ZERO_LOW (RC6_ONE_HIGHT)//444us = t = 1/36000*16
#define RC6_END_LOW (RC6_START_HIGHT)//2666us = t*6 = 1/36000*16*6

#define ENV_START_INT 1
#define ENV_LAST_INT  2
#define FIFO_UFL_INT  4

#define IR_ENV_START_INT_EN 1
#define IR_ENV_LAST_INT_EN  2
#define IR_FIFO_UFL_INT_EN  4
#define IR_ALL_INT_EN (IR_ENV_START_INT_EN|IR_ENV_LAST_INT_EN|IR_FIFO_UFL_INT_EN)


typedef enum ir_mode
{
    NORMAL_MODE,
    AUTO_MODE
}IR_MODE_t;

typedef enum ir_out_mode
{
    ADD,
    OR,
    NAND,
    NOR
}IR_OUT_MODE_t;

typedef enum ir_potocol
{
    RC5,
    RC6,
    RC_MN,
    NEC,
    SIRC_12bits=5,
    SIRC_15bits=8    
}IR_POTOCOL_t;

typedef union 
{
    uint32_t u32;
    struct ir_fifo 
    {
        uint16_t  evn_cnt;
        uint8_t  evenlope:1;
        uint8_t  evn_int:1;
        uint8_t  env_last:1;
        uint16_t  reserve:13;
    } bf;
} IR_FIFO_t;



typedef struct ir_buffer
{
    IR_FIFO_t* buf ;
    uint16_t  bufsize_mask;
    volatile uint32_t wr_idx;
    volatile uint32_t rd_idx;
    uint32_t total_usage_cnt;
}IR_BUFFER_t;

typedef enum ir_state
{
    IDLE,
    TRANSMITTING,
    DONE
}IR_STATE_t;




/**
 * @brief IRM finish routine callback for user application.
 *
 * @param[in] statue IRM transfer status.
 *
 * @details    This callback function is still running in interrupt mode, so this function 
 *              should be as short as possible. It can NOT call any block function in this 
 *              callback service routine.
 * 
 *              This function will be called when IRM finished the transfer envelope, or there
 *              is no envelope in fifo during the transfer (error case).
 *
 */
typedef void (* irm_proc_cb_t)(uint32_t status);


/**
 * @brief 
 * @details
 *         
 */
typedef struct 
{    
    irm_proc_cb_t  irm_cb_func;              /*!< irm callback function  \hideinitializer */
    IR_MODE_t op_mode;                       /*!< irm opration mode  \hideinitializer */
    IR_OUT_MODE_t ir_out_mode;               /*!< irm output mode  \hideinitializer */
    uint32_t  irm_int_en;                    /*!< irm interrupt enable  \hideinitializer */
} irm_mode_t;


/**
 * @brief Set IRM initialize
 *
 * @param[in]  
 *          irm_cfg  irm config
 *
 * @retval
 *       STATUS_SUCCESS
 *       STATUS_INVALID_REQUEST   --- I2C master is not in idle mode.
 *
 * @details
 *       Call this function to initail IRM, the whole IRM 
 *       driver is interrupt-driven, all IRM response are processing
 *       in user "irm_cb_func" callback function. Please Notice the
 *       the function "irm_cb_func" is in ISR context, so finish the 
 *       task ASAP. You can NOT block the function!
 */
extern uint32_t IRM_open(irm_mode_t *irm_cfg);

/**
 * @brief Set IRM carrier
 *
 * @param[in]  
 *          carrier_high_cnt    
 *          carrier_low_cnt
 *          carrier_base_cnt
 *          
 *          carrier frequercy = irm_sclk(32M) / (carrier_base_cnt * (carrier_high_cnt + carrier_low_cnt + 2))
 *          duty cycle = (carrier_high_cnt + 1) / (carrier_high_cnt + carrier_low_cnt + 2)
 * @retval
 *
 * @details
 *       Call this function to setup IRM carrier frequency and duty cycle.
 */
extern void IR_Carrier_config(uint8_t carrier_high_cnt, uint8_t carrier_low_cnt, uint16_t carrier_base_cnt);

/**
 * @brief Set irm fifo first 
 *
 * @param[in]  
 *          
 * @retval
 *
 * @details
 *       Call this function to set irm fifo data.
 */
extern void IR_BUFFER_fill_in(void);

/**
 * @brief Set irm fifo 
 *
 * @param[in]  
 *          
 * @retval
 *
 * @details
 *       Call this function to set irm fifo data.
 */
extern void IR_BUFFER_repeat_fill_in(void);

/**
 *   @brief  Enable irm 
 * 
 *   @details
 *
 */
extern void IR_enable(void);

/**
 *   @brief  Disable irm
 * 
 *   @details
 *
 */
extern void IR_disable(void);

/**
 *   @brief  Start irm 
 * 
 *   @details
 *
 */
extern void IR_start(void);

/**
 *   @brief  Stop irm
 * 
 *   @details
 *
 */
extern void IR_stop(void);

/**
 * @brief Send NEC protocol 
 *
 * @param[in]  
 *          cmd         
 *          address     
 *          
 * @retval
 *
 * @details
 *       Call this function to send NEC protocol data.
 */
extern void IR_NEC_encoder(uint16_t cmd, uint16_t address);

/**
 * @brief Send NEC protocol repeat command 
 *
 * @param[in]  
 *          
 * @retval
 *
 * @details
 *       Call this function to send NEC protocol repeat command.
 */
extern void IR_NEC_repeat_encoder(void);


extern void IR_SIRC_encoder(uint8_t cmd, uint8_t address,uint8_t address_length);
extern void IR_RC6_encoder(uint16_t cmd, uint8_t address,uint8_t RC6_toggle);


#ifdef __cplusplus
}
#endif

#endif


