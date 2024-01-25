/**************************************************************************//**
 * @file     crypto_reg.h
 * @version  V1.00
 * @brief    CRYPTO ACCELEATOR register definition header file
 *
 * @copyright 
 *****************************************************************************/
#ifndef ___CRYPTO_REG_H__
#define ___CRYPTO_REG_H__



/**
   @addtogroup REGISTER Control Register
   @{
*/

/**
    @addtogroup   CRYPTO ACCELEATOR Controller
    Memory Mapped Structure for CRYPTO ACCELEATOR Controller
@{ */


typedef union crypto_ctrl_struct {
    
    struct crypto_ctrl_b {
        __IO uint32_t vlw_op_num     :8;
        __IO uint32_t vlw_sb_num     :5;
        __IO uint32_t reserved1      :3;
        __IO uint32_t en_crypto      :1;
        __IO uint32_t enable_sha     :1;
        __IO uint32_t reserved2      :6;
        __IO uint32_t crypto_done    :1;
        __IO uint32_t sha_done       :1;
        __IO uint32_t crypto_busy    :1;
        __IO uint32_t sha_busy       :1;
        __IO uint32_t reserved3      :3;
        __IO uint32_t clr_crypto_int :1;    
    } bit;
    
    uint32_t reg;
    
} crypto_ctrl_t;

typedef struct {
    __IO crypto_ctrl_t  CRYPTO_CFG;
    __IO uint32_t       SHA_DIGEST_BASE;         
    __IO uint32_t       SHA_K_BASE;
    __IO uint32_t       SHA_DMA_BASE;
    __IO uint32_t       SHA_DMA_LENGTH;
    __IO uint32_t       SHA_MISC_CFG;
} CRYPTO_T;


/**
    @addtogroup CRYPTO ACCELEATOR REGISTER BIT DEFINITIONS
    
@{ */



/**@}*/ /* end of CRYPTO ACCELEATOR REGISTER BIT DEFINITIONS */


/**@}*/ /* end of CRYPTO ACCELEATOR Controller */
/**@}*/ /* end of REGISTER group */



#endif
