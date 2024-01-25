/**************************************************************************//**
 * @file     sysctrl.c
 * @version
 * @brief
 *
 * @copyright
*****************************************************************************/

#include "RT584_cm33.h"

uint32_t power_saving_mask_status = POWER_SAVING_NO_MASK;

void power_saving_mask(uint32_t mask)
{
    power_saving_mask_status |= mask;
}

void power_saving_unmask(uint32_t unmask)
{
    power_saving_mask_status &= (~unmask);
}

uint32_t get_power_saving_mask_status(void)
{
    return power_saving_mask_status;
}

void power_setting(void)
{
    
}

void sleep_mode_init(void)
{

}

void pin_set_mode(uint32_t pin_number, uint32_t mode)
{
    uint32_t reg, base, mask_offset, mask;

    /*RT584 mode become 4 bits.. so max mode is 15*/
    if ((pin_number>=32)||(mode >15)) {
        return;     /*Invalid setting mode.*/
    }

    base = MAP_BASE + (pin_number>>3)*4;
    mask_offset = (pin_number &0x7) <<2;
    mask = 0xF << mask_offset;

    enter_critical_section();
    /*pin mux setting is share resource.*/
    reg = *((volatile unsigned int *) base);
    reg = reg & ~mask;
    reg = reg | (mode << mask_offset);

    *((volatile unsigned int *)base) =  reg;

    leave_critical_section();

    return;
}

uint32_t pin_get_mode(uint32_t pin_number)
{
    uint32_t reg, base, mask_offset;

    if (pin_number>=32) {
        return 0xF;     /*Invalid setting mode.*/
    }

    base = MAP_BASE + (pin_number>>3)*4;
    mask_offset = (pin_number &0x7) <<2;

    reg = *((volatile unsigned int *) base);
    reg = reg >> mask_offset;
    reg = reg & 0x7;

    return reg;
}

/*
 * For multithread OS, and dynamic enable/disable peripheral environment,
 * Set this register has race condition issue.
 * so we need critical section protect.
 *
 */
void enable_perclk(uint32_t clock)
{
    if ((clock<UART0_CLK)||(clock >I2S_CLK)) {
        return;     /*Invalid setting mode.*/
    }

    enter_critical_section();
    SYSCTRL->SYS_CLK_CTRL |= (1<<clock) ;
    leave_critical_section();
}

/*
 * For multithread OS, and dynamic enable/disable peripheral environment,
 * Set this register has race condition issue,
 *  so we need critical section protect.
 *
 */
void disable_perclk(uint32_t clock)
{
    if ((clock<UART0_CLK)||(clock >I2S_CLK)) {
        return;     /*Invalid setting mode.*/
    }

    enter_critical_section();
    SYSCTRL->SYS_CLK_CTRL &= ~(1<<clock);
    leave_critical_section();
}


void set_sleep_option(uint32_t hclk_frozen_mode, uint32_t rtc_ds_wakeup_enable_mode) 
{
    uint32_t   mask=0;
    if (hclk_frozen_mode)
    {
        mask |= HCLK_FROZEN_ENABLE;       
    }
    if (rtc_ds_wakeup_enable_mode)
    {
        mask |= RTC_PCLK_DS_ENABLE;
    }
    enter_critical_section();      
    SYSCTRL->SYS_CLK_CTRL =  (SYSCTRL->SYS_CLK_CTRL & ~(HCLK_FROZEN_MASK|RTC_PCLK_DS_MASK)) | mask;
    leave_critical_section();
}

/*
 * For multithread OS, and dynamic enable/disable peripheral environment,
 * Set this register has race condition issue, (in fact, almost impossible)
 *  so we need critical section protect.
 *
 */
void pin_set_pullopt(uint32_t pin_number, uint32_t mode)
{
    uint32_t reg, base, mask_offset, mask;

    if ((pin_number>=32)||(mode >7))
    {
        return;     /*Invalid setting mode.*/
    }

    base = PULLOPT_BASE + (pin_number>>3)*4;
    mask_offset = (pin_number &0x7) <<2;
    mask = 0xF << mask_offset;
    
    enter_critical_section();
    /*pin mux setting is share resource.*/
    reg = *((volatile unsigned int *) base);
    reg = reg & ~mask;
    reg = reg | (mode << mask_offset);
 
    *((volatile unsigned int *)base) =  reg;
    
    leave_critical_section();

    return;
}

/*
 * For multithread OS, and dynamic enable/disable peripheral environment,
 * Set this register has race condition issue, (in fact, almost impossible)
 *  so we need critical section protect.
 *
 */
void pin_set_drvopt(uint32_t pin_number, uint32_t mode)
{
    uint32_t reg, base, mask_offset, mask;

    if ((pin_number>=32)||(mode >3))
    {
        return;     /*Invalid setting mode.*/
    }

    base = DRV_BASE + (pin_number>>4)*4;
    mask_offset = (pin_number &0xF) <<1;
    mask = 0x3 << mask_offset;
    
    enter_critical_section();
    /*pin mux setting is share resource.*/
    reg = *((volatile unsigned int *) base);
    reg = reg & ~mask;
    reg = reg | (mode << mask_offset);
 
    *((volatile unsigned int *)base) =  reg;
    
    leave_critical_section();

    return;
}

/*
 * For multithread OS, and dynamic enable/disable peripheral environment,
 * Set this register has race condition issue, so we need critical section protect.
 * In fact, it is almost impossible to dynamic change open drain.
 *
 */
void enable_pin_opendrain(uint32_t pin_number)
{
    uint32_t base, mask, reg;

    if (pin_number>=32)
    {
        return;     /*Invalid setting mode.*/
    }

    base = OD_BASE ;
    mask = 1<<pin_number ;

    enter_critical_section();
    /*pin mux setting is share resource.*/
    reg = *((volatile unsigned int *) base);
    reg = reg | mask;
    *((volatile unsigned int *)base) =  reg;

    leave_critical_section();

    return;
}

/*
 * For multithread OS, and dynamic enable/disable peripheral environment,
 * Set this register has race condition issue, so we need critical section protect.
 * In fact, it is almost impossible to dynamic change open drain.
 *
 */
void disable_pin_opendrain(uint32_t pin_number)
{
    uint32_t base, mask, reg;

    if (pin_number>=32)
    {
        return;     /*Invalid setting mode.*/
    }

    base = OD_BASE ;
    mask = ~(1<<pin_number);

    enter_critical_section();
    /*pin mux setting is share resource.*/
    reg = *((volatile unsigned int *) base);
    reg = reg & mask;
    *((volatile unsigned int *)base) =  reg;

    leave_critical_section();

    return;
}

uint32_t change_ahb_system_clk(uint32_t clk_mode)
{
    uint32_t i;

    if (clk_mode == SYS_CLK_32MHZ) {
        SYSCTRL->SYS_CLK_CTRL = (SYSCTRL->SYS_CLK_CTRL & ~HCLK_SEL_MASK) | HCLK_SEL_32M ;
        /*shall we disable PLL?*/
        SYSCTRL->SYS_CLK_CTRL = (SYSCTRL->SYS_CLK_CTRL & ~BASEBAND_PLL_MASK) ;

    } else if (clk_mode == SYS_CLK_48MHZ) {

        /*shall we check current CLK is 32M? if not we should change to 32M first.*/

        SYSCTRL->SYS_CLK_CTRL = (SYSCTRL->SYS_CLK_CTRL & ~BASEBAND_PLL_FREQ_MASK) | BASEBAND_PLL_48M;

        SYSCTRL->SYS_CLK_CTRL = (SYSCTRL->SYS_CLK_CTRL & ~BASEBAND_PLL_MASK) | BASEBAND_PLL_ENABLE;

        #if 0
        /* We need to wait 100us for PLL stable for 569 (if 32K clock is real 32K)
         * For First version 32K-clock is only 27K, so we need wait longer
         * if PLL_WAIT_PERIOD = 800  it will wait 125 us
         *    PLL_WAIT_PERIOD = 660  it will wait 103 us
         * If code is not in cache, it will take more time for preload data.
         */
        for(i=0; i<PLL_WAIT_PERIOD; i++)
        {
            __NOP();
        }
        #endif
        

        SYSCTRL->SYS_CLK_CTRL = (SYSCTRL->SYS_CLK_CTRL & ~HCLK_SEL_MASK) | HCLK_SEL_BASEBAND_PLL ;

    } else if (clk_mode == SYS_CLK_64MHZ) {

        /*Set PLL to 64MHZ*/
        SYSCTRL->SYS_CLK_CTRL = (SYSCTRL->SYS_CLK_CTRL & ~BASEBAND_PLL_FREQ_MASK) | BASEBAND_PLL_64M;

        SYSCTRL->SYS_CLK_CTRL = (SYSCTRL->SYS_CLK_CTRL & ~BASEBAND_PLL_MASK) | BASEBAND_PLL_ENABLE;

        #if 0
        /* We need to wait 100us for PLL stable for 569 (if 32K clock is real 32K)
         * For First version 32K-clock is only 27K, so we need wait longer
         * if PLL_WAIT_PERIOD = 800  it will wait 125 us
         *    PLL_WAIT_PERIOD = 660  it will wait 103 us
         * If code is not in cache, it will take more time for preload data.
         */
        for(i=0; i<PLL_WAIT_PERIOD; i++)
        {
            __NOP();
        }
        #endif

        SYSCTRL->SYS_CLK_CTRL = (SYSCTRL->SYS_CLK_CTRL & ~HCLK_SEL_MASK) | HCLK_SEL_BASEBAND_PLL ;
    } else {
        return STATUS_ERROR;
    }


    return STATUS_SUCCESS;
}

uint32_t get_ahb_system_clk(void)
{
    uint32_t clk_mode;

    if ((SYSCTRL->SYS_CLK_CTRL & HCLK_SEL_MASK) == HCLK_SEL_32M) {
        clk_mode = SYS_CLK_32MHZ;
    } else if ((SYSCTRL->SYS_CLK_CTRL & HCLK_SEL_MASK) == HCLK_SEL_BASEBAND_PLL) {
        if ((SYSCTRL->SYS_CLK_CTRL & BASEBAND_PLL_MASK) != BASEBAND_PLL_ENABLE) {
            //ASSERT();     /*???? ERROR*/
        }
        if ((SYSCTRL->SYS_CLK_CTRL & BASEBAND_PLL_FREQ_MASK) == BASEBAND_PLL_48M) {
            clk_mode = SYS_CLK_48MHZ;
        } else if ((SYSCTRL->SYS_CLK_CTRL & BASEBAND_PLL_FREQ_MASK) == BASEBAND_PLL_64M) {
            clk_mode = SYS_CLK_64MHZ;
        }
    } else {
        //ASSERT();     /*ERROR*/
    }

    return clk_mode;
}

void set_slow_clock_source(uint32_t mode)
{
    /*Slow clock selection.*/
    uint32_t  temp;
    
    if (mode>3)
        return;         /*Invalid mode*/
    
    enter_critical_section();
    
    temp  = SYSCTRL->SYS_CLK_CTRL & ~(SLOW_CLOCK_SEL_MASK) ;
    temp |= (mode<<SLOW_CLOCK_SEL_SHIFT);
    SYSCTRL->SYS_CLK_CTRL = temp;
    
    leave_critical_section();
}
