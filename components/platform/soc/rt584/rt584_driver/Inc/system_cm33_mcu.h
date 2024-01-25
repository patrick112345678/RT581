
/** @defgroup    Sytem_RT584_cm33_mcu
 *  @ingroup     peripheral_group
 *  @breif 
 *  @{
 *  @details   System Initialization header file for RT584-CM3 device based on CMSIS-CORE
*/
#ifndef _SYSTEM_RT584_CM33_MCU_H_
#define _SYSTEM_RT584_CM33_MCU_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
  \brief Exception / Interrupt Handler Function Prototype
*/
typedef void(*VECTOR_TABLE_Type)(void);

extern uint32_t SystemCoreClock;    /*!< Processor Clock Frequency            */

/**
 * Initialize the system
 *
 * @param  none
 * @return none
 *
 * @brief  Setup the microcontroller system.
 *         Initialize the System and update the SystemCoreClock variable.
 */
extern void SystemInit(void);

/**
 * Update SystemCoreClock variable
 *
 * @param  none
 * @return none
 *
 * @brief  Updates the SystemCoreClock with current core Clock
 *         retrieved from cpu registers.
 */
extern void SystemCoreClockUpdate(void);

/*@}*/ /* end of peripheral_group Sytem_RT584_cm33_mcu Driver */

#ifdef __cplusplus
}
#endif

#endif /* end of _SYSTEM_RT584_CM33_MCU_H_ */
