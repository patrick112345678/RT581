/**
 * @file zb_nvram.c
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief 
 * @version 0.1
 * @date 2023-08-24
 * 
 * @copyright Copyright (c) 2023
 * 
 */
//=============================================================================
//                Include
//=============================================================================
#include "FreeRTOS.h"
#include "log.h"

#include "zb_common.h"
#include "zb_osif.h"
#include "zb_nvram.h"
//=============================================================================
//                Private Definitions of const value
//=============================================================================
#define ZB_TRACE_FILE_ID 8003
#define NVRAM_WRITE_SUCCESS 0
#define NVRAM_WRITE_ERROR -1
#define NVRAM_WRITE_WRONG_PARAM -2

#define PAGE_SIZE               (uint32_t)0x4000  /* Page size = 16KByte */
#define PAGE_COUNT  2

#define ERASE_BLOCK_SIZE (uint32_t)0x1000 
#define BLOCK_SIZE      sizeof(uint32_t)

#define ZB_ASSERT_IF_VAL_IS_NOT_ALIGNED_TO_4(val) \
  ZB_ASSERT((val) % 4 == 0)


//Zigbee nvram use 0xF4000 ~ 0xFC000 in mp_sector
#define NVRAM_START_ADDRESS  ((uint32_t)0xE0000) //((uint32_t)0x002777fc)  NVRAM start address:
                                                    //   from sector2 : after 16KByte of used

#define PAGE0_BASE_ADDRESS    ((uint32_t)(NVRAM_START_ADDRESS + 0x0000))

#define PAGE1_BASE_ADDRESS    ((uint32_t)(NVRAM_START_ADDRESS + 0x4000))

//=============================================================================
//                Private ENUM
//=============================================================================

//=============================================================================
//                Private Struct
//=============================================================================

//=============================================================================
//                Private Global Variables
//=============================================================================

//=============================================================================
//                Functions
//=============================================================================
void zb_osif_nvram_init(zb_char_t const *name)
{
    flash_set_read_pagesize();
}

zb_uint32_t zb_get_nvram_page_length()
{
    return PAGE_SIZE;
}

zb_uint8_t zb_get_nvram_page_count()
{
    return PAGE_COUNT;
}

zb_ret_t zb_osif_nvram_page_read_memory(uint32_t address, uint32_t buf )
{
    flash_read_page(buf, address);
    return RET_OK;
}

zb_ret_t zb_osif_nvram_read_memory(zb_uint32_t address, zb_uint32_t len, zb_uint8_t *buf)
{
    uint32_t read_block;
    /* read uint32_t blocks */
    ZB_ASSERT_IF_VAL_IS_NOT_ALIGNED_TO_4(len);

    //log_info(">> %s len %d address %lx", __func__, len, address);


    while (len >= BLOCK_SIZE)
    {
        read_block = flash_read_byte(address);
        read_block += flash_read_byte(address+1)<<8;
        read_block += flash_read_byte(address+2)<<16;
        read_block += flash_read_byte(address+3)<<24;

        ZB_MEMCPY(buf, &read_block, BLOCK_SIZE);

        address += BLOCK_SIZE;
        buf += BLOCK_SIZE;
        len -= BLOCK_SIZE;
    }

    //log_info("<< %s len %d address %lX",
    //        (__func__, len, address));
    return RET_OK;
}

zb_ret_t zb_osif_nvram_read(zb_uint8_t page, zb_uint32_t pos, zb_uint8_t *buf, zb_uint16_t len )
{
    zb_uint32_t address;

    if(page>=PAGE_COUNT)
    {
        return RET_PAGE_NOT_FOUND;
    }
    if(pos+len>=PAGE_SIZE)
    {
        return RET_INVALID_PARAMETER;
    }

    //log_info("zb_osif_nvram_read %hd pos %ld len %d", page, pos, len);

    address = (page ? PAGE1_BASE_ADDRESS : PAGE0_BASE_ADDRESS) + pos;
    ZB_OSIF_GLOBAL_LOCK();

    zb_osif_nvram_read_memory(address, len, buf);
    ZB_OSIF_GLOBAL_UNLOCK();    
    return RET_OK;
}

zb_ret_t zb_osif_nvram_write(zb_uint8_t page, zb_uint32_t pos, void *buf, zb_uint16_t len )
{
    zb_uint_t ret = 0;
    uint32_t address;
    int32_t write_status = NVRAM_WRITE_SUCCESS;

    if(page>=PAGE_COUNT)
    {
        return RET_PAGE_NOT_FOUND;
    }
    if(pos+len>=PAGE_SIZE)
    {
        return RET_INVALID_PARAMETER;
    }

    //log_info(">> zb_osif_nvram_write page %hd pos %ld len %d",
    //            (page, pos, len));

    address = (page ? PAGE1_BASE_ADDRESS : PAGE0_BASE_ADDRESS) + pos;

    /* write whole uint32_t blocks  */
    ZB_ASSERT_IF_VAL_IS_NOT_ALIGNED_TO_4(len);
    ZB_ASSERT_IF_VAL_IS_NOT_ALIGNED_TO_4(address);
    ZB_OSIF_GLOBAL_LOCK();
    if (len > 4)
    {
        do
        {
            while(flash_check_busy());
            flash_write_byte(address, *(uint8_t*)buf);
            while(flash_check_busy());
            flash_write_byte(address+1, *((uint8_t*)buf + 1));
            while(flash_check_busy());
            flash_write_byte(address+2, *((uint8_t*)buf + 2));
            while(flash_check_busy());
            flash_write_byte(address+3, *((uint8_t*)buf + 3));
            while(flash_check_busy());

            buf = (uint8_t*)buf + 4;
            address += 4;
            len -= 4;
        
        } while (len >= 4 && !write_status);
    }
    else
    {
        flash_write_byte(address, *(uint8_t*)buf);
        while(flash_check_busy());
        flash_write_byte(address+1, *((uint8_t*)buf + 1));
        while(flash_check_busy());
        flash_write_byte(address+2, *((uint8_t*)buf + 2));
        while(flash_check_busy());
        flash_write_byte(address+3, *((uint8_t*)buf + 3));
        while(flash_check_busy());

    }
    ZB_OSIF_GLOBAL_UNLOCK();


    //log_info("<< zb_osif_nvram_write status %d", (write_status));

    return write_status == NVRAM_WRITE_SUCCESS ? RET_OK : -ret;    
}


zb_ret_t zb_osif_nvram_erase_async(zb_uint8_t page)
{
    int32_t erase_status = NVRAM_WRITE_SUCCESS;
    uint32_t address;
    zb_uint8_t erased_blocks = 0;
    zb_uint_t ret = 0;

    if(page>=PAGE_COUNT)
    {
        return RET_PAGE_NOT_FOUND;
    }

    //log_info("zb_osif_nvram_erase_async page %hd", ( page));

    address = (page == 0) ? PAGE0_BASE_ADDRESS : PAGE1_BASE_ADDRESS;
    ZB_OSIF_GLOBAL_LOCK();
    while (erased_blocks < (PAGE_SIZE / ERASE_BLOCK_SIZE) && erase_status == NVRAM_WRITE_SUCCESS)
    {
        while(flash_check_busy());
        // JJ erase_status = FlashMainPageErase(address);
        erase_status = flash_erase(FLASH_ERASE_SECTOR, address);
        /*we use polling to check busy state.. */
        while(flash_check_busy());

        address += ERASE_BLOCK_SIZE;
        ++erased_blocks;
    }
    ZB_OSIF_GLOBAL_UNLOCK();
    /* Erase is actually sync, so now erase is donw so we can call erase done cb */
    ZB_SCHEDULE_CALLBACK(zb_nvram_erase_finished, page);
    return erase_status == NVRAM_WRITE_SUCCESS ? RET_OK : -ret;
}


void zb_osif_nvram_flush()
{
  /* empty */
}