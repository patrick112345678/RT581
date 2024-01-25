/**
 * @file ot_flash.c
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief
 * @version 0.1
 * @date 2023-07-25
 *
 * @copyright Copyright (c) 2023
 *
 */

//=============================================================================
//                Include
//=============================================================================
#include <openthread/config.h>
#include "openthread-system.h"
#include "cm3_mcu.h"

//=============================================================================
//                Private Definitions of const value
//=============================================================================
#define PLATFORM_FLASH_PAGE_NUN 2

#define FLASH_PAGE_SIZE         0x1000
#define FLASH_PAGE_ERASE_SIZE   0x1000
#define FLASH_SWAP_SIZE (FLASH_PAGE_SIZE  *(PLATFORM_FLASH_PAGE_NUN/2))

#define FLASH_START_ADDRESS     0xF0000

#define FLASH_BASE_ADDR         0x90000000

//=============================================================================
//                Private ENUM
//=============================================================================

//=============================================================================
//                Private Struct
//=============================================================================

//=============================================================================
//                Private Global Variables
//=============================================================================
static uint32_t sFlashDataStart;
static uint32_t sFlashDataEnd;
static uint32_t sSwapSize = FLASH_SWAP_SIZE;


//=============================================================================
//                Functions
//=============================================================================
static inline uint32_t mapAddress(uint8_t aSwapIndex, uint32_t aOffset)
{
    uint32_t address;

    address = sFlashDataStart + aOffset;

    if (aSwapIndex)
    {
        address += sSwapSize;
    }

    return address;
}
/**
 * @brief
 *
 * @param aInstance
 */
void otPlatFlashInit(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    sFlashDataStart =  FLASH_START_ADDRESS;
    sFlashDataEnd = FLASH_START_ADDRESS + (FLASH_PAGE_SIZE * PLATFORM_FLASH_PAGE_NUN);
}

/**
 * @brief
 *
 * @param aInstance
 * @return uint32_t
 */
uint32_t otPlatFlashGetSwapSize(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return sSwapSize;
}

/**
 * @brief
 *
 * @param aInstance
 * @param aSwapIndex
 */
void otPlatFlashErase(otInstance *aInstance, uint8_t aSwapIndex)
{
    OT_UNUSED_VARIABLE(aInstance);
    while (flash_check_busy());
    flash_erase(FLASH_ERASE_SECTOR, FLASH_START_ADDRESS + (aSwapIndex * FLASH_PAGE_SIZE));
    while (flash_check_busy());
}

/**
 * @brief
 *
 * @param aInstance
 * @param aSwapIndex
 * @param aOffset
 * @param aData
 * @param aSize
 */
void otPlatFlashWrite(otInstance *aInstance, uint8_t aSwapIndex, uint32_t aOffset, const void *aData, uint32_t aSize)
{
    OT_UNUSED_VARIABLE(aInstance);

    uint32_t i;
    uint8_t *pDest = (uint8_t *)aData;

    for (i = 0; i < aSize; i++)
    {
        while (flash_check_busy());
        flash_write_byte(FLASH_START_ADDRESS + (aSwapIndex * FLASH_SWAP_SIZE) + aOffset + i, pDest[i]);
    }
}

/**
 * @brief
 *
 * @param aInstance
 * @param aSwapIndex
 * @param aOffset
 * @param aData
 * @param aSize
 */
void otPlatFlashRead(otInstance *aInstance, uint8_t aSwapIndex, uint32_t aOffset, void *aData, uint32_t aSize)
{
    OT_UNUSED_VARIABLE(aInstance);

    uint32_t i;
    uint8_t *pDest = (uint8_t *)aData;

    for (i = 0; i < aSize; i++)
    {
        while (flash_check_busy());
        pDest[i] = flash_read_byte((FLASH_START_ADDRESS + (aSwapIndex * FLASH_PAGE_SIZE)) + aOffset + i);
    }

}