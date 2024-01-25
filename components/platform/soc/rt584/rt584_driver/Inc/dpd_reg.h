/**************************************************************************//**
 * @file     dpd_reg.h
 * @version  V1.00
 * @brief    CRYPTO ACCELEATOR register definition header file
 *
 * @copyright 
 *****************************************************************************/
#ifndef ___DPD_REG_H__
#define ___DPD_REG_H__


/**
   @addtogroup REGISTER Control Register
   @{
*/


/**
   @addtogroup DPD_RST_CAUSE
   @{
*/

typedef union dpd_rst_cause_s {
    struct dpd_rst_cause_b {
        uint32_t RST_CAUSE_POR          : 1;
        uint32_t RST_CAUSE_EXT          : 1;
        uint32_t RST_CAUSE_dpd          : 1;
        uint32_t RST_CAUSE_ds           : 1;
        uint32_t RST_CAUSE_wdt          : 1;
        uint32_t RST_CAUSE_soft         : 1;
        uint32_t RST_CAUSE_lock         : 1;
        uint32_t RESERVED               : 1;
        uint32_t BOOT_STATUS            : 3;
        uint32_t RESERVE2               : 21;
    } bit;
    uint32_t reg;
} dpd_rst_cause_t;


/**
   @addtogroup DPD_CMD
   @{
*/

typedef union dpd_cmd_s {
    struct dpd_cmd_b {
        uint32_t CLR_RST_CAUSE          : 1;
        uint32_t RESERVED               : 15;
        uint32_t DPD_FLASH_DPD_EN       : 1;
        uint32_t RESERVED2              : 7;
        uint32_t EN_UVL                 : 1;
        uint32_t UVL_OUT_VALID          : 1;
        uint32_t UVL_VTH                : 2;
        uint32_t RESERVED3              : 3;
        uint32_t UVH_DISABLE            : 1;
    } bit;
    uint32_t reg;
} dpd_cmd_t;


/**
    @addtogroup   DPD Controller
    Memory Mapped Structure for Deep Power Down
@{ */


typedef struct {
    __IO dpd_rst_cause_t        DPD_RST_CAUSE;
    __IO dpd_cmd_t              DPD_CMD;
    __IO uint32_t               DPD_GPIO_EN;
    __IO uint32_t               DPD_GPIO_INV;
    __IO uint32_t               DPD_RET0_REG;
    __IO uint32_t               DPD_RET1_REG;
    __IO uint32_t               DPD_RET2_REG;
    __IO uint32_t               DPD_RET3_REG;             
} DPD_T;

/*DPD_RET3_REG will be used for system.*/

#define CLEAR_RESET_CAUSE           1

/**
    @addtogroup Deep Power Down REGISTER BIT DEFINITIONS
    
@{ */

#define  RESET_CAUSE_POR        (1<<0)
#define  RESET_CAUSE_EXT        (1<<1)
#define  RESET_CAUSE_DPD        (1<<2)
#define  RESET_CAUSE_DS         (1<<3)
#define  RESET_CAUSE_WDT        (1<<4)
#define  RESET_CAUSE_SOFT       (1<<5)
#define  RESET_CAUSE_LOCK       (1<<6)


/**@}*/ /* end of Deep Power Down REGISTER BIT DEFINITIONS */


/**@}*/ /* end of DPD Controller */
/**@}*/ /* end of REGISTER group */

#endif
