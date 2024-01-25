/* ZBOSS Zigbee software protocol stack
 *
 * Copyright (c) 2012-2020 DSR Corporation, Denver CO, USA.
 * http://www.dsr-zboss.com
 * http://www.dsr-corporation.com
 * All rights reserved.
 *
 * This is unpublished proprietary source code of DSR Corporation
 * The copyright notice does not evidence any actual or intended
 * publication of such source code.
 *
 * ZBOSS is a registered trademark of Data Storage Research LLC d/b/a DSR
 * Corporation
 *
 * Commercial Usage
 * Licensees holding valid DSR Commercial licenses may use
 * this file in accordance with the DSR Commercial License
 * Agreement provided with the Software or, alternatively, in accordance
 * with the terms contained in a written agreement between you and
 * DSR.
 */
/* PURPOSE: NVRAM implementation
*/

#define ZB_TRACE_FILE_ID 8003
#include "zb_common.h"
#include "zb_osif.h"
#include "zb_nvram.h"

//JJ #include "flash.h"
void RfMcu_InterruptEnableAll(void);
void RfMcu_InterruptDisableAll(void);

#define RADIO_INT_ENABLE1()  RfMcu_InterruptEnableAll()
#define RADIO_INT_DISABLE1() RfMcu_InterruptDisableAll()

static void delay1(void)
{
    {
        int x = 4000;
        while (x--)
        {
            __ASM("nop");
            __ASM("nop");
            __ASM("nop");
            __ASM("nop");
            __ASM("nop");
            __ASM("nop");
            __ASM("nop");
            __ASM("nop");
            __ASM("nop");
            __ASM("nop");
            __ASM("nop");
            __ASM("nop");
            __ASM("nop");
            __ASM("nop");
            __ASM("nop");
            __ASM("nop");
            __ASM("nop");
            __ASM("nop");
            __ASM("nop");
            __ASM("nop");
            __ASM("nop");
            __ASM("nop");
            __ASM("nop");
            __ASM("nop");
            __ASM("nop");
            __ASM("nop");
            __ASM("nop");
            __ASM("nop");
            __ASM("nop");
            __ASM("nop");
        }
    }
}

#define NVRAM_WRITE_SUCCESS 0
#define NVRAM_WRITE_ERROR -1
#define NVRAM_WRITE_WRONG_PARAM -2

#define PAGE_SIZE               (uint32_t)0x4000  /* Page size = 16KByte */
#define PAGE_COUNT  2

#define ERASE_BLOCK_SIZE (uint32_t)0x1000 //JJ Erase block size = 4K Byte  0x0800 /* Erase block size = 2KByte */
#define BLOCK_SIZE sizeof(uint32_t)

#define ZB_ASSERT_IF_VAL_IS_NOT_ALIGNED_TO_4(val) \
  ZB_ASSERT((val) % 4 == 0)

/* NVRAM start address in Flash */
/* NK: from 0x0023a438 - ffff space */
#define NVRAM_START_ADDRESS  ((uint32_t)0x00078000) //((uint32_t)0x002777fc)  NVRAM start address:
//   from sector2 : after 16KByte of used
//   Flash memory

#define PAGE0_BASE_ADDRESS    ((uint32_t)(NVRAM_START_ADDRESS + 0x0000))

#define PAGE1_BASE_ADDRESS    ((uint32_t)(NVRAM_START_ADDRESS + 0x4000))


/*! \addtogroup ZB_OSIF */
/*! @{ */

#if defined ZB_USE_NVRAM || defined DOXYGEN

extern void flash_set_timing(flash_timing_mode_t *timing_cfg);

void zb_osif_nvram_init(zb_char_t const *name)
{
    flash_timing_mode_t  flash_timing;
    flash_status_t  flash_status;

    ZVUNUSED(name);

    /*Here we assume CPU run in 48MHz, if CPU run in 64 this setting should changed*/
    flash_timing.deep_pd_timing = 144;
    flash_timing.deep_rpd_timing = 960;
    flash_timing.suspend_timing = 960;
    flash_timing.resume_timing = 4800;
    flash_set_timing(&flash_timing);

    /*enable flash QSPI mode.*/
    //flash_QSPI_enable();

    //dma_init();

    /*Here we default read page is 256 bytes.*/
    flash_set_read_pagesize();

    flash_status.require_mode = (FLASH_STATUS_RW1);
    flash_get_status_reg(&flash_status);
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

#if 1 //JJ
    uint32_t read_block;
    /* read uint32_t blocks */
    ZB_ASSERT_IF_VAL_IS_NOT_ALIGNED_TO_4(len);

    TRACE_MSG(TRACE_COMMON2, ">> zb_osif_nvram_read_memory len %d address %ld",
              (FMT__D_L, len, address));


    while (len >= BLOCK_SIZE)
    {
        read_block = flash_read_byte(address);
        read_block += flash_read_byte(address + 1) << 8;
        read_block += flash_read_byte(address + 2) << 16;
        read_block += flash_read_byte(address + 3) << 24;

        ZB_MEMCPY(buf, &read_block, BLOCK_SIZE);

        address += BLOCK_SIZE;
        buf += BLOCK_SIZE;
        len -= BLOCK_SIZE;
    }

    TRACE_MSG(TRACE_COMMON2, "<< zb_osif_nvram_read_memory len %d address %ld",
              (FMT__D_L, len, address));
    return RET_OK;
#endif
}


zb_ret_t zb_osif_nvram_read(zb_uint8_t page, zb_uint32_t pos, zb_uint8_t *buf, zb_uint16_t len )
{
#if 1 //JJ
    zb_uint32_t address;

    if (page >= PAGE_COUNT)
    {
        return RET_PAGE_NOT_FOUND;
    }
    if (pos + len >= PAGE_SIZE)
    {
        return RET_INVALID_PARAMETER;
    }

    TRACE_MSG(TRACE_COMMON2, "zb_osif_nvram_read %hd pos %ld len %d",
              (FMT__H_L_D, page, pos, len));

    address = (page ? PAGE1_BASE_ADDRESS : PAGE0_BASE_ADDRESS) + pos;
    //  ZB_OSIF_GLOBAL_LOCK();
    //  RADIO_INT_DISABLE1();
    zb_osif_nvram_read_memory(address, len, buf);
    //  RADIO_INT_ENABLE1();
    //  ZB_OSIF_GLOBAL_UNLOCK();

    return RET_OK;
#endif
}


zb_ret_t zb_osif_nvram_write(zb_uint8_t page, zb_uint32_t pos, void *buf, zb_uint16_t len )
{
#if 1 //JJ
    zb_uint_t ret = 0;
    uint32_t address;
    int32_t write_status = NVRAM_WRITE_SUCCESS;

    if (page >= PAGE_COUNT)
    {
        return RET_PAGE_NOT_FOUND;
    }
    if (pos + len >= PAGE_SIZE)
    {
        return RET_INVALID_PARAMETER;
    }

    TRACE_MSG(TRACE_COMMON2, ">> zb_osif_nvram_write page %hd pos %ld len %d",
              (FMT__H_L_D, page, pos, len));

    address = (page ? PAGE1_BASE_ADDRESS : PAGE0_BASE_ADDRESS) + pos;

    /* write whole uint32_t blocks  */
    ZB_ASSERT_IF_VAL_IS_NOT_ALIGNED_TO_4(len);
    ZB_ASSERT_IF_VAL_IS_NOT_ALIGNED_TO_4(address);
    //  ZB_OSIF_GLOBAL_LOCK();
    //  RADIO_INT_DISABLE1();
    if (len > 4)
    {
        do
        {

            flash_write_byte(address, *(uint8_t *)buf);
            while (flash_check_busy());
            flash_write_byte(address + 1, *((uint8_t *)buf + 1));
            while (flash_check_busy());
            flash_write_byte(address + 2, *((uint8_t *)buf + 2));
            while (flash_check_busy());
            flash_write_byte(address + 3, *((uint8_t *)buf + 3));
            while (flash_check_busy());

            buf = (uint8_t *)buf + 4;
            address += 4;
            len -= 4;

        } while (len >= 4 && !write_status);
    }
    else
    {
        //if (address == 0x0007827C) {
        //   TRACE_MSG(TRACE_COMMON2, ">> zb_osif_nvram_write page %hd pos %ld len %d",
        //        (FMT__H_L_D, page, pos, len));
        //  }
        //write_status = FlashMainPageProgram((uint32_t *)buf, address, len);

        flash_write_byte(address, *(uint8_t *)buf);
        while (flash_check_busy());
        flash_write_byte(address + 1, *((uint8_t *)buf + 1));
        while (flash_check_busy());
        flash_write_byte(address + 2, *((uint8_t *)buf + 2));
        while (flash_check_busy());
        flash_write_byte(address + 3, *((uint8_t *)buf + 3));
        while (flash_check_busy());

    }
    //  RADIO_INT_ENABLE1();
    //  ZB_OSIF_GLOBAL_UNLOCK();


    TRACE_MSG(TRACE_COMMON2, "<< zb_osif_nvram_write status %d", (FMT__D, write_status));

    return write_status == NVRAM_WRITE_SUCCESS ? RET_OK : -ret;
#endif
}


zb_ret_t zb_osif_nvram_erase_async(zb_uint8_t page)
{

    int32_t erase_status = NVRAM_WRITE_SUCCESS;
    uint32_t address;
    zb_uint8_t erased_blocks = 0;
    zb_uint_t ret = 0;

    if (page >= PAGE_COUNT)
    {
        return RET_PAGE_NOT_FOUND;
    }

    TRACE_MSG(TRACE_COMMON2, "zb_osif_nvram_erase_async page %hd", (FMT__H, page));

    address = (page == 0) ? PAGE0_BASE_ADDRESS : PAGE1_BASE_ADDRESS;
    ZB_OSIF_GLOBAL_LOCK();
    RADIO_INT_DISABLE1();
    while (erased_blocks < (PAGE_SIZE / ERASE_BLOCK_SIZE) && erase_status == NVRAM_WRITE_SUCCESS)
    {
        // JJ erase_status = FlashMainPageErase(address);
        erase_status = flash_erase(FLASH_ERASE_SECTOR, address);
        /*we use polling to check busy state.. */
        while (flash_check_busy());

        address += ERASE_BLOCK_SIZE;
        ++erased_blocks;
    }
    RADIO_INT_ENABLE1();
    ZB_OSIF_GLOBAL_UNLOCK();
    /* Erase is actually sync, so now erase is donw so we can call erase done cb */
    ZB_SCHEDULE_CALLBACK(zb_nvram_erase_finished, page);
    return erase_status == NVRAM_WRITE_SUCCESS ? RET_OK : -ret;

#if 0 //JJ
    int32_t erase_status = NVRAM_WRITE_SUCCESS;
    uint32_t address;
    zb_uint8_t erased_blocks = 0;
    zb_uint_t ret = 0;

    if (page >= PAGE_COUNT)
    {
        return RET_PAGE_NOT_FOUND;
    }

    TRACE_MSG(TRACE_COMMON2, "zb_osif_nvram_erase_async page %hd", (FMT__H, page));

    address = (page == 0) ? PAGE0_BASE_ADDRESS : PAGE1_BASE_ADDRESS;

    while (erased_blocks < (PAGE_SIZE / ERASE_BLOCK_SIZE) && erase_status == NVRAM_WRITE_SUCCESS)
    {
        erase_status = FlashMainPageErase(address);
        address += ERASE_BLOCK_SIZE;
        ++erased_blocks;
    }

    /* Erase is actually sync, so now erase is donw so we can call erase done cb */
    ZB_SCHEDULE_CALLBACK(zb_nvram_erase_finished, page);
    return erase_status == NVRAM_WRITE_SUCCESS ? RET_OK : -ret;
#endif
}


void zb_osif_nvram_flush()
{
    /* empty */
}
#endif  /* defined ZB_USE_NVRAM || defined DOXYGEN */

/*! @} */
