/**************************************************************************//**
 * @file     sysfun.c
 * @version
 * @brief    System function implement
 *
 * @copyright
 ******************************************************************************/

#include "RT584_cm33.h"

static int critical_counter=0;

/*   For CM33, there are banked PRIMASK registers
 * for secure world and non-secure world.
 * the code will disable/enable irq
 * depends on the secure/non-secure state.
 *
 */

void enter_critical_section(void)
{
    __disable_irq();
    critical_counter++;
}

void leave_critical_section(void)
{
    critical_counter--;

#ifdef  DEBUG
    if (critical_counter <0)
    {   /*Serious Error */
        while(1);
    }
#endif

    if (critical_counter == 0) {
        __enable_irq();
    }
}

/*
 *      version_check is help function to check 
 *   software project setting is the same as hardware IC version.
 *   If software project define "SUPPORT_IC_VERSION" is 
 *   not matched with hardware IC version, this functio will return 0, otherwise 1.
 *     
 */
uint32_t version_check(void)
{
#if 0
    /*not support yet*/
    uint32_t   version_info , ret=1, chip_id, chip_rev;
    
    version_info =  SYSCTRL_S->CHIP_INFO ;
    
    chip_id =  (version_info >>8) &0xFF;
    chip_rev = (version_info >>4) &0x0F;
    
    if (chip_id!=0x84) {
        return 0;
    }

    return ret;

#else

    return 1;

#endif    

}
