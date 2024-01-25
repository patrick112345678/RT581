#include "rt584_cm33.h"

#if defined (__ARM_FEATURE_CMSE) &&  (__ARM_FEATURE_CMSE == 3U)
    #include "partition_rt584.h"
#endif

/*----------------------------------------------------------------------------
  Define clocks
 *----------------------------------------------------------------------------*/
#define  XTAL            (32000000UL)     /* Oscillator frequency */

#define  SYSTEM_CLOCK    (XTAL)

/*----------------------------------------------------------------------------
  Exception / Interrupt Vector table
 *----------------------------------------------------------------------------*/
extern const VECTOR_TABLE_Type __VECTOR_TABLE[64];

/*----------------------------------------------------------------------------
  System Core Clock Variable
 *----------------------------------------------------------------------------*/
uint32_t SystemCoreClock = SYSTEM_CLOCK;  /* System Core Clock Frequency */

/*----------------------------------------------------------------------------
  System Core Clock update function
 *----------------------------------------------------------------------------*/
void SystemCoreClockUpdate (void)
{
    SystemCoreClock = SYSTEM_CLOCK;
}

/*----------------------------------------------------------------------------
  System initialization function
 *----------------------------------------------------------------------------*/
void SystemInit (void)
{    
#if defined (__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)
//  uint32_t blk_cfg, blk_max, blk_size, blk_cnt;    
#endif

#if defined (__VTOR_PRESENT) && (__VTOR_PRESENT == 1U)
    SCB->VTOR = (uint32_t) &(__VECTOR_TABLE[0]);
#endif

#if defined (__FPU_USED) && (__FPU_USED == 1U)
    /* Coprocessor Access Control Register. It's banked for secure state and non-seure state */
    SCB->CPACR |= ((3U << 10U*2U) |           /* enable CP10 Full Access */
                   (3U << 11U*2U)  );         /* enable CP11 Full Access */
                   
    /*Notice: CPACR Secure state address 0xE000ED88.  CPACR_NS is 0xE002ED88
     *   Secure software can also define whether non-secure software 
     *   can access ecah of the coprocessor using a register called NSACR 
     *   Non-secure Access Control Register.
     */
                   
#endif


#if defined (__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)

/*
 * in FPGA, it is hard to route 48MHz... current version could be 44.28 MHz
 * You can use sys_tick to verify the AHB clock
 * Of course, you can set FPGA_48MHZ in C/C++(AC6) option
 */
//#define FPGA_48MHZ  1   


    /* Enable BusFault, UsageFault, MemManageFault and SecureFault to ease diagnostic */
    SCB->SHCSR |= (SCB_SHCSR_USGFAULTENA_Msk  |
                 SCB_SHCSR_BUSFAULTENA_Msk  |
                 SCB_SHCSR_MEMFAULTENA_Msk  |
                 SCB_SHCSR_SECUREFAULTENA_Msk);

    /* BFSR register setting to enable precise errors */
    SCB->CFSR |= SCB_CFSR_PRECISERR_Msk;

    TZ_SAU_Setup();
    

#ifdef FPGA_48MHZ
    /*Set PLL to about 48MHz..*/
    /* FIXME: NOTICE: in real ASIC, we should wait PLL stable... 
     * in FPGA, we don't need to wait PLL stable*/

    SYSCTRL->SYS_CLK_CTRL = (SYSCTRL->SYS_CLK_CTRL & ~HCLK_SEL_MASK) | HCLK_SEL_32M;            //set pll to 32Mhz
    SYSCTRL->SYS_CLK_CTRL = (SYSCTRL->SYS_CLK_CTRL & ~BASEBAND_PLL_MASK);                       //baseband pll disable
    
    SYSCTRL->SYS_CLK_CTRL = (SYSCTRL->SYS_CLK_CTRL & ~BASEBAND_PLL_FREQ_MASK) | BASEBAND_PLL_48M;
    SYSCTRL->SYS_CLK_CTRL = (SYSCTRL->SYS_CLK_CTRL & ~HCLK_SEL_MASK) | HCLK_SEL_BASEBAND_PLL;        /*config BASEBAND_PLL_ENABLE*/
    
    /*set flash timing.*/
    flash_timing_init(48);

#else

    SYSCTRL->SYS_CLK_CTRL = (SYSCTRL->SYS_CLK_CTRL & ~HCLK_SEL_MASK) | HCLK_SEL_32M;  
    SYSCTRL->SYS_CLK_CTRL = (SYSCTRL->SYS_CLK_CTRL & ~BASEBAND_PLL_MASK); 
    
    SYSCTRL->SYS_CLK_CTRL = (SYSCTRL->SYS_CLK_CTRL & ~PERCLK_SEL) | PERCLK_SEL_32M;
    
    /*set flash timing.*/
    flash_timing_init(32);

#endif

    /*enable flash 4 bits mode*/
    flash_enable_qe();
                   
#endif

    SystemCoreClock = SYSTEM_CLOCK;
}

#if  (__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)
/*This interrupt must be secure world.*/

/*Debug used*/
void Sec_Ctrl_Handler(void)
{
    uint32_t status;

    status = SEC_CTRL->SEC_INT_STATUS.reg;    
    SEC_CTRL->SEC_INT_CLR.reg = status;
    status = SEC_CTRL->SEC_INT_STATUS.reg;         /*ensure the clear.*/    
}

#endif

