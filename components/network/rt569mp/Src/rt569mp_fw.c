/**************************************************************************//**
* @file     rt569mp_fw.c
* @version
* @brief    RF MCU FW initialization

******************************************************************************/

#include "cm3_mcu.h"
#include "rf_mcu.h"
#include "rf_mcu_chip.h"
#include "rf_common_init.h"



/**************************************************************************************************
 *    GLOBAL VARIABLES
 *************************************************************************************************/
#if (RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569MP)
/* Use top level CHIP_VERSION */

#if (CHIP_VERSION == RT58X_MPA)
#if (RF_FW_INCLUDE_PCI == TRUE)
#include "prg_pci_mp_asic_fw.h"
const uint32_t firmware_size_ruci = sizeof(firmware_program_ruci);
#else
const uint8_t firmware_program_ruci[] = {0};
const uint32_t firmware_size_ruci = 0;
#endif

#if (RF_FW_INCLUDE_BLE == TRUE)
#include "prg_ble_mp_asic_fw.h"
const uint32_t firmware_size_ble = sizeof(firmware_program_ble);
#else
const uint8_t firmware_program_ble[] = {0};
const uint32_t firmware_size_ble = 0;
#endif

#if (RF_FW_INCLUDE_MULTI_2P4G == TRUE)
#include "prg_multi_mp_asic_fw.h"
const uint32_t firmware_size_multi = sizeof(firmware_program_multi);
#else
const uint8_t firmware_program_multi[] = {0};
const uint32_t firmware_size_multi = 0;
#endif

#if (RF_CAL_TYPE & RF_CAL_PWR_ON_MODE)
#include "prg_rfk_mp_asic_fw.h"
const uint32_t firmware_size_rfk = sizeof(firmware_program_rfk);
#else
const uint8_t firmware_program_rfk[] = {0};
const uint32_t firmware_size_rfk = 0;
#endif

#elif (CHIP_VERSION == RT58X_MPB)
#if (RF_FW_INCLUDE_PCI == TRUE)
#include "prg_pci_mpb_asic_fw.h"
const uint32_t firmware_size_ruci = sizeof(firmware_program_ruci);
#else
const uint8_t firmware_program_ruci[] = {0};
const uint32_t firmware_size_ruci = 0;
#endif

#if (RF_FW_INCLUDE_BLE == TRUE)
#include "prg_ble_mpb_asic_fw.h"
const uint32_t firmware_size_ble = sizeof(firmware_program_ble);
#else
const uint8_t firmware_program_ble[] = {0};
const uint32_t firmware_size_ble = 0;
#endif

#if (RF_FW_INCLUDE_MULTI_2P4G == TRUE)
#include "prg_multi_mpb_asic_fw.h"
const uint32_t firmware_size_multi = sizeof(firmware_program_multi);
#else
const uint8_t firmware_program_multi[] = {0};
const uint32_t firmware_size_multi = 0;
#endif

#if (RF_FW_INCLUDE_INTERNAL_TEST == TRUE)
const uint8_t firmware_program_it[] = {0};
const uint32_t firmware_size_it = 0;
#if (RF_MCU_CONST_LOAD_SUPPORTED)
const uint8_t firmware_const_it[] = {0};
const uint32_t const_size_it = 0;
#endif
#endif


#if (RF_CAL_TYPE & RF_CAL_PWR_ON_MODE)
#include "prg_rfk_mpb_asic_fw.h"
const uint32_t firmware_size_rfk = sizeof(firmware_program_rfk);
#else
const uint8_t firmware_program_rfk[] = {0};
const uint32_t firmware_size_rfk = 0;
#endif

#else
#error "Error: Invalid RF chip version!!"
#endif

#endif

