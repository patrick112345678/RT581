/**************************************************************************//**
 * @file     flashctl.c
 * @version
 * @brief
 *
 * @copyright
*****************************************************************************/


/****************************************************************************
 * IMPORTANT: PLEASE READ THIS NOTICE BEFORE YOU CHANGE ANY DESIGN　CODING
 * SOME FLASH OPERATION will take very long time... like erase or write..
 * we don't want to the whole function to be block in non-sense waiting block
 * so for time-wasting functions. we using async non-block define... 
 * It means user should check each function finish before a new instruction!
 *
 */

#include "RT584_cm33.h"

#define FLASH_UNLOCK_PATTER   0x52414254

uint32_t flash_get_deviceinfo(void)
{
    return (FLASH->FLASH_INFO);
}

/*
* flash_read_page :
 *   read one page. One page is 256 bytes, so buf_addr should have 256 bytes available.
 *
 */
void flash_read_page(uint32_t buf_addr, uint32_t read_page_addr)
{
    FLASH->COMMAND =  CMD_READPAGE;
    FLASH->FLASH_ADDR = read_page_addr;
    FLASH->MEM_ADDR  = buf_addr;
    FLASH->PATTERN = FLASH_UNLOCK_PATTER;
    FLASH->START = STARTBIT;
    
    /*Notice: user should check tgus function finish before a new instruction */
    return;
}

uint8_t flash_read_byte(uint32_t read_addr)
{    
    FLASH->COMMAND =  CMD_READBYTE;
    FLASH->FLASH_ADDR = read_addr;

    FLASH->PATTERN = FLASH_UNLOCK_PATTER;
    FLASH->START = STARTBIT;

    while(flash_check_busy());

    return (FLASH->FLASH_DATA >> 8);
}

uint32_t flash_erase(flash_erase_mode_t mode, uint32_t flash_addr)
{
    if(mode > FLASH_ERASE_SECURE)
        return  STATUS_INVALID_PARAM;

    FLASH->FLASH_ADDR = flash_addr;

    /* For Safety reason, we don't implement
     * erase chip command here. */

    switch(mode) {
    case FLASH_ERASE_PAGE:
        FLASH->COMMAND =  CMD_ERASESECTOR;
        break;

    case FLASH_ERASE_32K:
        FLASH->COMMAND =  CMD_ERASE_BL32K;
        break;

    case FLASH_ERASE_64K:
        FLASH->COMMAND =  CMD_ERASE_BL64K;
        break;

    case FLASH_ERASE_SECURE:
        /*This is special command for erase secure register*/
        FLASH->COMMAND = CMD_ERASE_SEC_PAGE;
        break;

    default:
        return STATUS_INVALID_PARAM;
    }

    FLASH->PATTERN = FLASH_UNLOCK_PATTER;
    FLASH->START = STARTBIT;

    return STATUS_SUCCESS;
}

void flash_set_timing(flash_timing_mode_t *timing_cfg)
{
    FLASH->DPD = timing_cfg->deep_pd_timing;
    FLASH->RDPD = timing_cfg->deep_rpd_timing;
    FLASH->SUSPEND = timing_cfg->suspend_timing;
    FLASH->RESUME  = timing_cfg->resume_timing;
    return;
}

void flash_write_page(uint32_t buf_addr, uint32_t write_page_addr)
{
    FLASH->COMMAND =  CMD_WRITEPAGE;
    FLASH->FLASH_ADDR = write_page_addr;
    FLASH->MEM_ADDR  = buf_addr;
    FLASH->PATTERN = FLASH_UNLOCK_PATTER;
    FLASH->START = STARTBIT;

    return;
}

void flash_verify_page(uint32_t read_page_addr)
{

    FLASH->COMMAND =  CMD_READVERIFY;
    FLASH->FLASH_ADDR = read_page_addr;
    FLASH->PATTERN = FLASH_UNLOCK_PATTER;
    FLASH->START = STARTBIT;

    return;
}

void flash_write_byte(uint32_t write_addr, uint8_t singlebyte)
{

    FLASH->COMMAND =  CMD_WRITEBYTE;
    FLASH->FLASH_ADDR = write_addr;
    FLASH->FLASH_DATA = singlebyte;

    FLASH->PATTERN = FLASH_UNLOCK_PATTER;
    FLASH->START = STARTBIT;

    return;
}

/*get Flash status register*/
void flash_get_status_reg(flash_status_t *status)
{
    if ((status->require_mode)&FLASH_STATUS_RW1) {
        FLASH->COMMAND =  CMD_READ_STATUS1;
        FLASH->PATTERN = FLASH_UNLOCK_PATTER;
        FLASH->START = STARTBIT;

         /*this check_busy is very short... it just send command then to receive data*/
        while(flash_check_busy());   
        status->status1 = (uint8_t)((FLASH->FLASH_DATA) >> 8);
    }

    if(status->require_mode&FLASH_STATUS_RW2) {
        FLASH->COMMAND =  CMD_READ_STATUS2;
        FLASH->PATTERN = FLASH_UNLOCK_PATTER;
        FLASH->START = STARTBIT;

        while(flash_check_busy());
        status->status2 = (uint8_t)((FLASH->FLASH_DATA) >> 8);
    }

    if(status->require_mode&FLASH_STATUS_RW3) {
        FLASH->COMMAND =  CMD_READ_STATUS3;
        FLASH->PATTERN = FLASH_UNLOCK_PATTER;
        FLASH->START = STARTBIT;

        while(flash_check_busy());
        status->status3 = (uint8_t)((FLASH->FLASH_DATA) >> 8);
    }

}

/*set Flash status register*/
void flash_set_status_reg(const flash_status_t *status)
{
    if ((status->require_mode)&FLASH_STATUS_RW1) {
        FLASH->COMMAND =  CMD_WRITE_STATUS1;
        FLASH->STATUS  = (status->status1);
        FLASH->PATTERN = FLASH_UNLOCK_PATTER;
        FLASH->START = STARTBIT;

        while(flash_check_busy());

    }

    if(status->require_mode&FLASH_STATUS_RW2) {
        FLASH->COMMAND =  CMD_WRITE_STATUS2;
        FLASH->STATUS  = (uint32_t) ((status->status2) << 8);
        FLASH->PATTERN = FLASH_UNLOCK_PATTER;
        FLASH->START = STARTBIT;

        while(flash_check_busy());
    }

    if(status->require_mode&FLASH_STATUS_RW3) {
        FLASH->COMMAND =  CMD_WRITE_STATUS3;
        FLASH->STATUS  = (uint32_t) ((status->status3) << 16);
        FLASH->PATTERN = FLASH_UNLOCK_PATTER;
        FLASH->START = STARTBIT;

        while(flash_check_busy());
    }

}

/* program secure page data
 * Note: write_page_addr must be alignment
 */
uint32_t flash_write_sec_register(uint32_t buf_addr, uint32_t write_reg_addr)
{
    uint32_t  addr;
    /*first we should check write_reg_addr*/
    addr = write_reg_addr>>12;

    if((addr<1) ||(addr>3)|| (write_reg_addr & 0xFF)) {
        /*only support 3 secureity register.*/
        /*We need secure register write to be 256 bytes alignment*/
        return STATUS_INVALID_PARAM;
    }

    FLASH->COMMAND =  CMD_WRITE_SEC_PAGE;
    FLASH->FLASH_ADDR = write_reg_addr;
    FLASH->MEM_ADDR  = buf_addr;
    FLASH->PATTERN = FLASH_UNLOCK_PATTER;
    FLASH->START = STARTBIT;

    return STATUS_SUCCESS;
}

/* read secure register data.
 * Note: read_page_addr must be alignment
 */
uint32_t flash_read_sec_register(uint32_t buf_addr, uint32_t read_reg_addr)
{
    uint32_t  addr;
    /*first we should check read_reg_addr*/
    addr = read_reg_addr>>12;

    if((addr<1) ||(addr>3)|| (read_reg_addr & 0xFF)) {
        /*We need secure register read to be 256 bytes alignment*/
        return STATUS_INVALID_PARAM;
    }

    FLASH->COMMAND =  CMD_READ_SEC_PAGE;
    FLASH->FLASH_ADDR = read_reg_addr;
    FLASH->MEM_ADDR  = buf_addr;
    FLASH->PATTERN = FLASH_UNLOCK_PATTER;
    FLASH->START = STARTBIT;

    return STATUS_SUCCESS;
}


/* read flash unique ID
 *  flash ID is 128bits/16 bytes. 
 *  if buf_length <16, it will return required length data only.
 *  if buf_length >16, it will return 16 bytes only.
 *  if buf_length = 0 , this function will return STATUS_INVALID_PARAM
 *
 */
uint32_t flash_get_unique_id(uint32_t flash_id_buf_addr, uint32_t buf_length)
{
    uint32_t  i;
    uint8_t  temp[16], *ptr;
    
    /*
     * Notice: we don't check flash_id_buf_addr value here..
     * it should be correct address in SRAM!
     */
    if (buf_length==0) {
        return STATUS_INVALID_PARAM;
    } else if (buf_length >16) {
        buf_length = 16;
    }

    FLASH->COMMAND =  CMD_READUID;
    FLASH->PAGE_READ_WORD = 0xF;
    FLASH->MEM_ADDR  = (uint32_t) temp;
    FLASH->PATTERN = FLASH_UNLOCK_PATTER;
    FLASH->START = STARTBIT;
    
    ptr = (uint8_t *) flash_id_buf_addr;    /*set address*/
    
    while(flash_check_busy());
    
    FLASH->PAGE_READ_WORD = 0xFF;   /*restore read one page length by default*/
        
    /*move unique number from stack to assign buffer*/
    for(i=0; i<buf_length; i++) {
        ptr[i] = temp[i];
    }
        
    return STATUS_SUCCESS;
}

void flash_timing_init(uint32_t HCLK_MHZ)
{
    uint32_t   sys_clk;
    uint16_t   tdp, tres, tsus, trs, flash_type_id;

    flash_timing_mode_t  flash_timing;
    /*change AHB clock also need change flash timing.*/
    flash_type_id = flash_get_deviceinfo() & FLASH_TYPE_ID_MAKS;
    
    sys_clk = HCLK_MHZ;          /* FIXME: in FPGA, assume 32M.
                                  * In ASIC... change this setting
                                  */

    /*check flash type to adjust flash timing*/
    if (flash_type_id == GDWQ_ID)
    {
        tdp  = GDWQ_FLASH_TDP;
        tres = GDWQ_FLASH_TRES1;
        tsus = GDWQ_FLASH_TSUS;
        trs  = GDWQ_FLASH_TRS;
    }

    if (flash_type_id == GDLQ_ID)
    {
        tdp  = GDLQ_FLASH_TDP;
        tres = GDLQ_FLASH_TRES1;
        tsus = GDLQ_FLASH_TSUS;
        trs  = GDLQ_FLASH_TRS;
    }

    if (flash_type_id == PUYA_ID)
    {
        tdp  = PUYA_FLASH_TDP;
        tres = PUYA_FLASH_TRES1;
        tsus = PUYA_FLASH_TSUS;
        trs  = PUYA_FLASH_TRS;
    }

    flash_timing.deep_pd_timing = tdp * sys_clk + 2;
    flash_timing.deep_rpd_timing = tres * sys_clk + 2;
    flash_timing.suspend_timing = tsus * sys_clk + 2;
    flash_timing.resume_timing = trs * sys_clk + 2;

    flash_set_timing(&flash_timing);
}

