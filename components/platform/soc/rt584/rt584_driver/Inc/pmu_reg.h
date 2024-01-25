/**************************************************************************//**
 * @file     pmu_reg.h
 *****************************************************************************/

#ifndef __PMU_REG_H__
#define __PMU_REG_H__

#if defined (__CC_ARM)
#pragma anon_unions
#endif

/**
   @addtogroup REGISTER Control Register
   @{
*/

/**
    @addtogroup  PMU Controller
    Memory Mapped Structure for PMU Controller
@{ */

enum {
    PMU_CONFIG_VSEL_0V95      = 0,   /**< Vsel = 0.95V. */
    PMU_CONFIG_VSEL_1V00      = 1,   /**< Vsel = 1.00V. */
    PMU_CONFIG_VSEL_1V05      = 2,   /**< Vsel = 1.05V. */
    PMU_CONFIG_VSEL_1V10      = 3,   /**< Vsel = 1.10V. */
    PMU_CONFIG_VSEL_1V15      = 4,   /**< Vsel = 1.15V. */
    PMU_CONFIG_VSEL_1V20      = 5,   /**< Vsel = 1.20V. */
    PMU_CONFIG_VSEL_1V25      = 6,   /**< Vsel = 1.25V. */
    PMU_CONFIG_VSEL_1V30      = 7,   /**< Vsel = 1.30V. */
};

typedef union pmu_comp0_s {
    struct pmu_comp0_b {
        uint32_t AUX_COMP_SELREF        : 1;
        uint32_t AUX_COMP_INT_EN        : 1;
        uint32_t AUX_COMP_INT_POL       : 2;
        uint32_t AUX_COMP_EN_NM         : 1;
        uint32_t AUX_COMP_EN_SP         : 1;
        uint32_t AUX_COMP_EN_DS         : 1;
        uint32_t AUX_COMP_SELINPUT      : 1;
        uint32_t AUX_COMP_VSEL          : 4;
        uint32_t AUX_COMP_REFSEL        : 4;
        uint32_t AUX_COMP_CHSEL         : 4;
        uint32_t AUX_COMP_PW            : 2;
        uint32_t AUX_COMP_SELHYS        : 2;
        uint32_t AUX_COMP_SWDIV         : 1;
        uint32_t AUX_COMP_PSRR          : 1;
        uint32_t AUX_COMP_TC            : 1;
        uint32_t RESERVED               : 1;
        uint32_t AUX_COMP_DS_WAKEUP_EN  : 1;
        uint32_t AUX_COMP_DS_INV        : 1;
        uint32_t AUX_EN_START           : 2;
    } bit;
    uint32_t reg;
} pmu_comp0_t;

typedef union pmu_comp1_s {
    struct pmu_comp1_b {
        uint32_t AUX_COMP_INT_CLR : 1;
        uint32_t RESERVED1        : 31;
    } bit;
    uint32_t reg;
} pmu_comp1_t;

typedef union pmu_comp2_s {
    struct pmu_comp2_b {
        uint32_t AUX_COMP_INT_STA : 1;
        uint32_t AUX_COMP_OUT     : 1;
        uint32_t RESERVED1        : 14;
        uint32_t BBPLL_VTBIT      : 2;
        uint32_t BBPLL_TEMPBIT    : 2;
        uint32_t BBPLL_BANK_VCO   : 3;
        uint32_t RESERVED3        : 9;
    } bit;
    uint32_t reg;
} pmu_comp2_t;

typedef union pmu_xtal0_s {
    struct pmu_xtal0_b {
        uint32_t EN_XBUF            : 1;
        uint32_t EN_D_XTALIN        : 1;
        uint32_t SEL_XBUFLOAD       : 1;
        uint32_t BYP_XLDO           : 1;
        uint32_t AUTO_PWD_XBUF      : 1;
        uint32_t XSOC_EN_INJ        : 1;
        uint32_t XSOC_EN_VREF1      : 1;
        uint32_t XSOC_EN_AGC        : 1;
        uint32_t XSOC_CAP_MANU      : 1;
        uint32_t XSOC_CAP_CLK_DIV2  : 1;
        uint32_t XSOC_BIAS_HALF     : 1;
        uint32_t XSOC_IBOOST        : 1;
        uint32_t XSOC_INJ_TIME      : 2;
        uint32_t XSOC_AMPDET_TH     : 2;
        uint32_t XSOC_I_INJ         : 2;
        uint32_t XSOC_C_INJ         : 2;
        uint32_t XSOC_LPF_C         : 2;
        uint32_t XSOC_LPF_R         : 1;
        uint32_t XSOC_MODEL_XTAL_BO : 1;
        uint32_t PW_XTAL            : 3;
        uint32_t EN_CLOCKMODEM      : 1;
        uint32_t RESERVED1          : 2;
        uint32_t FORCE_RCO_32K_OFF  : 1;
        uint32_t FORCE_XOSC_32M_OFF : 1;
    } bit;
    uint32_t reg;
} pmu_xtal0_t;

typedef union pmu_xtal1_s {
    struct pmu_xtal1_b {
        uint32_t CFG_XTAL_SETTLE_TIME   : 8;
        uint32_t CFG_BYPASS_XTAL_TIME   : 1;
        uint32_t RESERVED1              : 3;
        uint32_t CFG_XTAL_FAST          : 1;
        uint32_t CFG_XTAL_FAST_AUTO     : 1;
        uint32_t CFG_EN_XTAL            : 1;
        uint32_t CFG_EN_XTAL_AUTO       : 1;
        uint32_t XOSC_CAP_INI           : 6;
        uint32_t RESERVED2              : 2;
        uint32_t XOSC_CAP_TARGET        : 1;
        uint32_t RESERVED3              : 2;
    } bit;
    uint32_t reg;
} pmu_xtal1_t;

//0x24 PMU_SOC_PMU_XTAL1
typedef union pmu_soc_pmu_xtal1_s {
    struct pmu_soc_pmu_xtal1_b {
        uint32_t XOSC_CAP_INI           : 6;
        uint32_t RESERVED1              : 2;
        uint32_t XOSC_CAP_TARGET        : 8;
        uint32_t CFG_XTAL_SETTLE_TIME   : 8;
        uint32_t CFG_XTAL_FAST          : 1;
        uint32_t CFG_XTAL_FAST_AUTO     : 1;
        uint32_t CFG_EN_XTAL            : 1;
        uint32_t CFG_EN_XTAL_AUTO       : 1;
        uint32_t CFG_BYPASS_XTAL_SETTLE : 1;
        uint32_t RESERVED2              : 3;
    } bit;
    uint32_t reg;
} pmu_soc_pmu_xtal1_t;


typedef union pmu_osc32k_s {
    struct pmu_osc32k_b {
        uint32_t TUNE_FINE_RCO_32K      : 8;
        uint32_t TUNE_COARSE_RCO_32K    : 2;
        uint32_t PW_BUF_RCO_32K         : 2;
        uint32_t PW_RCO_32K             : 3;
        uint32_t RCO_32K_SEL            : 1;
        uint32_t EN_XO_32K              : 1;
        uint32_t EN_XO_32K_FAST         : 1;
        uint32_t PW_BUF_XO_32K          : 2;
        uint32_t PW_XO_32K              : 3;
        uint32_t force_rco_32k_off      : 1;
        uint32_t RESERVED1              : 8;
    } bit;
    uint32_t reg;
} pmu_osc32k_t;

typedef union pmu_rvd0_s {
    struct pmu_rvd0_b {
        uint32_t CFG_MV_SETTLE_TIME     : 7;
        uint32_t RESERVED1              : 1;
        uint32_t CFG_PWEX_SETTLE_TIME   : 3;
        uint32_t RESERVED2              : 13;
        uint32_t PMU_RESERVED_0         : 8;
    } bit;
    uint32_t reg;
} pmu_rvd0_t;

typedef union pmu_cal32k_cfg0_s {
    struct pmu_cal32k_cfg0_b {
        uint32_t CFG_CAL32K_TARGET      : 18;
        uint32_t RESERVED1              : 6;
        uint32_t CFG_CAL32K_EN          : 1;
        uint32_t RESERVED2              : 7;
    } bit;
    uint32_t reg;
} pmu_cal32k_cfg0_t;

typedef union pmu_cal32k_cfg1_s {
    struct pmu_cal32k_cfg1_b {
        uint32_t CFG_CAL32K_LOCK_ERR      : 8;
        uint32_t CFG_CAL32K_AVG_COARSE    : 2;
        uint32_t CFG_CAL32K_AVG_FINE      : 2;
        uint32_t CFG_CAL32K_AVG_LOCK      : 2;
        uint32_t CFG_CAL32K_DLY           : 2;
        uint32_t CFG_CAL32K_FINE_GAIN     : 3;
        uint32_t CFG_CAL32K_SKIP_COARSE   : 1;
        uint32_t CFG_CAL32K_LOCK_GAIN     : 3;
        uint32_t CFG_CAL32K_BOUND_MODE    : 1;
        uint32_t CFG_CAL32K_TRACK_EM      : 1;
        uint32_t EN_CK_CAL32K             : 1;
        uint32_t RESERVED1                : 1;
        uint32_t CFG_32K_RC_SEL           : 1;
        uint32_t RESERVED2                : 4;
    } bit;
    uint32_t reg;
} pmu_cal32k_cfg1_t;

typedef union pmu_cal32k_result0_s {
    struct pmu_cal32k_result0_b {
        uint32_t EST_32K_RESULT         : 18;
        uint32_t RESERVED1              : 6;
        uint32_t EST_32K_RESULT_VALID   : 1;
        uint32_t RESERVED2              : 3;
        uint32_t CAL32K_BUSY            : 1;
        uint32_t CAL32K_LOCK            : 1;
        uint32_t CAL32K_TIMEOUT         : 1;
        uint32_t RESERVED3              : 1;
    } bit;
    uint32_t reg;
} pmu_cal32k_result0_t;

typedef union pmu_cal32k_result1_s {
    struct pmu_cal32k_result1_b {
        uint32_t TUNE_FINE_CAL32K       : 8;
        uint32_t TUNE_COARSE_CAL32K     : 2;
        uint32_t RESERVED1              : 22;
    } bit;
    uint32_t reg;
} pmu_cal32k_result1_t;

typedef union pmu_rfldo_s {
    struct pmu_rfldo_b {
        uint32_t RF_MLDO_VTUNE       : 4;
        uint32_t RF_MLDO_SIN_M       : 2;
        uint32_t RF_MLDO_BG_OS       : 2;
        uint32_t RF_MLDO_BG_OS_DIR   : 1;
        uint32_t RF_MLDO_LOUT        : 1;
        uint32_t RF_MLDO_BG_PN_SYNC  : 1;
        uint32_t RESERVED1           : 1;
        uint32_t RF_MLDO_IOC         : 3;
        uint32_t RESERVED2           : 1;
        uint32_t TS_VX               : 3;
        uint32_t RESERVED3           : 1;
        uint32_t TS_S                : 3;
        uint32_t RESERVED4           : 1;
        uint32_t TS_EN               : 1;
        uint32_t TS_RST              : 1;
        uint32_t TS_CLK_EN           : 1;
        uint32_t RESERVED5           : 1;
        uint32_t TS_CLK_SEL          : 2;
        uint32_t APMU_TEST           : 1;
    } bit;
    uint32_t reg;
} pmu_rfldo_t;

typedef union soc_pmu_rco1m_s {
    struct soc_pmu_rco1m_b {
        uint32_t TUNE_FINE_RCO_1M       : 7;
        uint32_t RESERVED1              : 1;
        uint32_t TUNE_COARSE_RCO_1M     : 4;
        uint32_t PW_RCO_1M              : 2;
        uint32_t TEST_RCO_1M            : 2;
        uint32_t EN_RCO_1M              : 1;
        uint32_t RESERVED3              : 16;
    } bit;
    uint32_t reg;
} soc_pmu_rco1m_t;

typedef union pmu_dcdc_vosel_s {
    struct pmu_dcdc_vosel_b {
        uint32_t DCDC_VOSEL_NORMAL   : 5;
        uint32_t RESERVED1           : 3;
        uint32_t DCDC_VOSEL_HEAVY    : 5;
        uint32_t RESERVED2           : 3;
        uint32_t DCDC_VOSEL_LIGHT    : 5;
        uint32_t RESERVED3           : 3;
        uint32_t DCDC_RUP_RATE       : 8;
    } bit;
    uint32_t reg;
} pmu_dcdc_vosel_t;

typedef union pmu_ldomv_vosel_s {
    struct pmu_ldomv_vosel_b {
        uint32_t LDOMV_VOSEL_NORMAL   : 5;
        uint32_t RESERVED1            : 3;
        uint32_t LDOMV_VOSEL_HEAVY    : 5;
        uint32_t RESERVED2            : 3;
        uint32_t LDOMV_VOSEL_LIGHT    : 5;
        uint32_t RESERVED3            : 3;
        uint32_t DCDC_RDN_RATE        : 8;
    } bit;
    uint32_t reg;
} pmu_ldomv_vosel_t;

typedef union pmu_sldo_vosel_s {
    struct pmu_sldo_vosel_b {
        uint32_t SLDO_VOSEL_NM      : 5;
        uint32_t RESERVED1          : 3;
        uint32_t SLDO_VOSEL_SP      : 5;
        uint32_t RESERVED2          : 3;
        uint32_t LLDO_VOSEL         : 4;
        uint32_t RESERVED3          : 10;
        uint32_t POR_VTH            : 2;
    } bit;
    uint32_t reg;
} pmu_sldo_vosel_t;


//0xBC SOC_PMU_TIMING
typedef union pmu_soc_pmu_timing_s {
    struct pmu_soc_pmu_timing_b {
        uint32_t CFG_LV_SETTLE_TIME         : 7;
        uint32_t CFG_BYPASS_LV_SETTLE       : 1;
        uint32_t CFG_MV_SETTLE_TIME         : 7;
        uint32_t CFG_BYPASS_MV_SETTLE       : 1;
        uint32_t CFG_PWRX_SETTLE_TIME       : 3;
        uint32_t RESERVED1                  : 9;
        uint32_t FORCE_DCDC_SOC_PMU         : 1;
        uint32_t FORCE_DCDC_SOC_Heavy_Tx    : 1;
        uint32_t FORCE_DCDC_SOC_Light_Rx    : 1;
        uint32_t RESERVED2                  : 1;        
    } bit;
    uint32_t reg;
} pmu_soc_pmu_timing_t;

typedef struct {
    __IO  pmu_comp0_t  PMU_COMP0;                           //offset: 0x00
    __IO  pmu_comp1_t  PMU_COMP1;                           //offset: 0x04
    __IO  pmu_comp2_t  PMU_COMP2;                           //offset: 0x08
    __IO  uint32_t  PMU_RESERVED0C;                         //offset: 0x0C
    __IO  pmu_xtal0_t  PMU_XTAL0;                           //offset: 0x10
    __IO  pmu_xtal1_t  PMU_XTAL1;                           //offset: 0x14
    __IO  uint32_t  PMU_RESERVED18;                         //offset: 0x18
    __IO  uint32_t  PMU_RESERVED1C;                         //offset: 0x1C
    __IO  uint32_t  PMU_RESERVED20;                         //offset: 0x20
    __IO  pmu_soc_pmu_xtal1_t  PMU_SOC_PMU_XTAL1;           //offset: 0x24
    __IO  uint32_t  PMU_RESERVED28;                         //offset: 0x28
    __IO  uint32_t  PMU_RESERVED2C;                         //offset: 0x2C
    __IO  uint32_t  PMU_RESERVED30;                         //offset: 0x30
    __IO  pmu_osc32k_t  PMU_OSC32K;                         //offset: 0x34
    __IO  uint32_t  PMU_RESERVED38;                         //offset: 0x38
    __IO  uint32_t  PMU_RESERVED3C;                         //offset: 0x3C
    __IO  pmu_rvd0_t  PMU_RVD0;                             //offset: 0x40
    __IO  uint32_t  PMU_RESERVED44;                         //offset: 0x44
    __IO  uint32_t  PMU_RESERVED48;                         //offset: 0x48
    __IO  uint32_t  PMU_RESERVED4C;                         //offset: 0x4C
    __IO  pmu_cal32k_cfg0_t  PMU_CAL32K_CFG0;               //offset: 0x50
    __IO  pmu_cal32k_cfg1_t  PMU_CAL32K_CFG1;               //offset: 0x54
    __IO  pmu_cal32k_result0_t  PMU_CAL32K_RESULT0;         //offset: 0x58
    __IO  pmu_cal32k_result1_t  PMU_CAL32K_RESULT1;         //offset: 0x5C
    __IO  uint32_t  PMU_RESERVED60;                         //offset: 0x60
    __IO  uint32_t  PMU_RESERVED64;                         //offset: 0x64
    __IO  uint32_t  PMU_RESERVED68;                         //offset: 0x68
    __IO  uint32_t  PMU_RESERVED6C;                         //offset: 0x6C
    __IO  uint32_t  PMU_RESERVED70;                         //offset: 0x70
    __IO  uint32_t  PMU_RESERVED74;                         //offset: 0x74
    __IO  uint32_t  PMU_RESERVED78;                         //offset: 0x78
    __IO  uint32_t  PMU_RESERVED7C;                         //offset: 0x7C
    __IO  uint32_t  PMU_RESERVED80;                         //offset: 0x80
    __IO  pmu_rfldo_t  PMU_RFLDO;                           //offset: 0x84
    __IO  uint32_t  PMU_RESERVED88;                         //offset: 0x88
    __IO  soc_pmu_rco1m_t  SOC_PMU_RCO1M;                   //offset: 0x8C
    __IO  pmu_dcdc_vosel_t  PMU_DCDC_VOSEL;                 //offset: 0x90
    __IO  pmu_ldomv_vosel_t  PMU_LDOMV_VOSEL;               //offset: 0x94
    __IO  uint32_t  PMU_RESERVED98;                         //offset: 0x98
    __IO  pmu_sldo_vosel_t  PMU_SLDO_VOSEL;                 //offset: 0x9C
    __IO  uint32_t  PMU_DCDC_NORMAL;                        //offset: 0xA0
    __IO  uint32_t  PMU_DCDC_HEAVY;                         //offset: 0xA4
    __IO  uint32_t  PMU_DCDC_LIGHT;                         //offset: 0xA8
    __IO  uint32_t  PMU_DCDC_RESERVED;                      //offset: 0xAC
    __IO  uint32_t  PMU_LDO_CTRL;                           //offset: 0xB0
    __IO  uint32_t  PMU_EN_CTRL;                            //offset: 0xB4
    __IO  uint32_t  PMU_BBPLL0;                             //offset: 0xB8
    __IO  pmu_soc_pmu_timing_t  PMU_SOC_PMU_TIMING;         //offset: 0xBC

} PMU_T;

/**
    @addtogroup PMU REGISTER BIT DEFINITIONS

@{ */

// control register bit field definitions:


/**@}*/ /* end of PMU REGISTER BIT DEFINITIONS */

/**@}*/ /* end of PMU Controller */
/**@}*/ /* end of REGISTER group */

#if defined (__CC_ARM)
#pragma no_anon_unions
#endif

#endif

