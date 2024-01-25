/**************************************************************************//**
 * @file     flashds.c
 * @version
 * @brief
 *
 * @copyright
*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "cm3_mcu.h"

static ds_info_t  dsinfo;

uint32_t ds_initinal(uint32_t start_addr, uint32_t end_addr, uint32_t size)
{
    uint8_t page_size = 0;

    if ((end_addr > DS_END_ADDRESS) || (start_addr < DS_START_ADDRESS))
    {
        dsinfo.invaild = DS_INVAILD;
        return STATUS_INVALID_PARAM;
    }

    page_size = (end_addr - start_addr) / size;

    if (page_size != DS_PAGE_MAX)
    {
        dsinfo.invaild = DS_INVAILD;
        return STATUS_INVALID_PARAM;
    }

    dsinfo.start_address = start_addr;
    dsinfo.end_address = end_addr;
    dsinfo.page_size = size;
    dsinfo.page_number = ((start_addr - end_addr) / size) >> 1;

    ds_pagex_check();

    return STATUS_SUCCESS;
}


uint32_t ds_pagex_check()
{
    uint32_t page1_satatus, page2_satatus, satatus = STATUS_SUCCESS;
    uint32_t ds_page1_addr = dsinfo.start_address;
    uint32_t ds_page2_addr = (dsinfo.start_address + dsinfo.page_size);
    uint32_t ds_page1_sn, ds_page2_sn;
    ds_search_t ds_search_pagex;
    ds_t    *dspack1, *dspack2;


    ds_search_pagex.address = ds_page1_addr;
    ds_search_pagex.offset = DS_START_OFFSET;
    ds_search_pagex.type = DS_VAILD_TYPE;

    page1_satatus = ds_erase_check(&ds_search_pagex);

    ds_search_pagex.address = ds_page2_addr;
    page2_satatus = ds_erase_check(&ds_search_pagex);


    if ((page1_satatus == STATUS_SUCCESS) && (page2_satatus == STATUS_SUCCESS))                   //Page1 and Page 2 All 0xFF
    {
        dsinfo.current_page = DS_PAGE_1;
        dsinfo.current_offset = DS_START_OFFSET;
        dsinfo.current_address = ds_page1_addr;
        dsinfo.current_sn = DS_SERAIL_NUMBER_START;
    }
    else if ((page1_satatus != STATUS_SUCCESS) && (page2_satatus == STATUS_SUCCESS))              //Page2 All 0xFF,Page1 has record data
    {
        satatus = ds_page_vaild_address(ds_page1_addr, dspack1);                                                //check page 1 data

        if (satatus == STATUS_INVALID_PARAM)                                                                                            //Page1 invaild data
        {
            dsinfo.current_page = DS_PAGE_2;
            dsinfo.current_offset = DS_START_OFFSET;
            dsinfo.current_address = ds_page2_addr;
            ds_migration(DS_PAGE_1);
            ds_migration_erase(DS_PAGE_1);
        }
    }
    else if ((page1_satatus == STATUS_SUCCESS) && (page2_satatus != STATUS_SUCCESS))                  //Page1 All 0xFF, Page2 has record data
    {
        satatus = ds_page_vaild_address(ds_page2_addr, dspack2);                                                //check page 1 data

        if (satatus == STATUS_INVALID_PARAM)                                                                                            //Page2 invaild data
        {
            dsinfo.current_page = DS_PAGE_1;
            dsinfo.current_offset = DS_START_OFFSET;
            dsinfo.current_address = ds_page1_addr;
            ds_migration(DS_PAGE_2);
            ds_migration_erase(DS_PAGE_2);
        }
    }
    else
    {
        page1_satatus = ds_page_vaild_address(ds_page1_addr, dspack1);
        ds_page1_sn = dsinfo.current_sn;

        page2_satatus = ds_page_vaild_address(ds_page2_addr, dspack2);
        ds_page2_sn = dsinfo.current_sn;

        if ((page1_satatus == STATUS_SUCCESS) && (page2_satatus == STATUS_SUCCESS)) //all page data is okay
        {
            if (ds_page1_sn > ds_page2_sn)
            {
                ds_migration_erase(DS_PAGE_2);
                ds_migration(DS_PAGE_1);
                ds_migration_erase(DS_PAGE_1);
                ds_page_vaild_address(ds_page2_addr, dspack2);
            }
            else
            {
                ds_migration_erase(DS_PAGE_1);
                ds_migration(DS_PAGE_2);
                ds_migration_erase(DS_PAGE_2);
                ds_page_vaild_address(ds_page1_addr, dspack1);
            }

        }
        else if ((page1_satatus == STATUS_INVALID_PARAM) && (page2_satatus == STATUS_SUCCESS))
        {
            ds_migration_erase(DS_PAGE_1);
            dsinfo.current_page = DS_PAGE_2;
        }
        else if ((page1_satatus == STATUS_SUCCESS) && (page2_satatus == STATUS_INVALID_PARAM))
        {
            ds_migration_erase(DS_PAGE_2);
            dsinfo.current_page = DS_PAGE_1;
        }
        else if ((page1_satatus == STATUS_INVALID_PARAM) && (page2_satatus == STATUS_INVALID_PARAM))
        {
            ds_migration_erase(DS_PAGE_1);
            ds_migration_erase(DS_PAGE_2);
            dsinfo.current_page = DS_PAGE_1;
            dsinfo.current_offset = DS_START_OFFSET;
            dsinfo.current_address = ds_page1_addr;
            dsinfo.current_sn = 1;
        }
    }

    return STATUS_SUCCESS;
}


uint32_t ds_erase_check(ds_search_t *ds_search)
{
    uint32_t i, j;
    uint8_t dsbufP1[DS_DATA_CHECK_SIZE];

    for (i = 0; i < dsinfo.page_size; i += DS_DATA_CHECK_SIZE)
    {
        flash_read_page((uint32_t)dsbufP1, (ds_search->address + i));

        while (flash_check_busy());

        for (j = 0; j < DS_DATA_CHECK_SIZE; j++)
        {
            if (dsbufP1[j] != DS_VAILD_TYPE)
            {
                ds_search->offset = j;
                ds_search->address = (ds_search->address + i);

                return STATUS_ERROR;
            }
        }
    }

    return STATUS_SUCCESS;
}


uint32_t ds_page_vaild_address(uint32_t ds_start_address, ds_t *dspacktmp)
{
    uint32_t ds_status;
    ds_search_t ds_vaild_search;

    ds_vaild_search.address = ds_start_address;
    ds_vaild_search.offset = DS_START_OFFSET;
    ds_vaild_search.type = DS_VAILD_TYPE;
    ds_vaild_search.flag = DS_VAILD_TYPE_SEARCH;

    ds_status = ds_vaild_address_search(&ds_vaild_search, dspacktmp);

    if (ds_status == STATUS_SUCCESS)
    {

        dsinfo.current_page = (((dsinfo.current_address - dsinfo.start_address) / dsinfo.page_size));
    }

    return ds_status;
}


uint32_t ds_vaild_address_search(ds_search_t *dssearch, ds_t *dspacktmp)
{

    uint32_t    ds_status, read_type_search;
    uint32_t    maxaddress, address;
    ds_t dspack;

    dspack = (*(ds_t *)(dssearch->address + dssearch->offset));

    if (dspack.type == DS_VAILD_TYPE)
    {
        return STATUS_INVALID_PARAM;
    }

    read_type_search = 0;

    do
    {
        dspack = (*(ds_t *)(dssearch->address + dssearch->offset));

        ds_status = ds_vaild(dssearch, &dspack);

        if (ds_status == STATUS_SUCCESS && dssearch->type == DS_VAILD_TYPE)
        {
            break;
        }

        if (ds_status == STATUS_TIMEOUT && dssearch->type != DS_VAILD_TYPE)
        {
            if ((dspack.type == DS_VAILD_TYPE) && (read_type_search != DS_READ_TYPE_SEARCH))
            {
                ds_status = STATUS_INVALID_REQUEST;
            }
            else
            {
                ds_status = STATUS_SUCCESS;
            }
            break;
        }
        else if (ds_status == DS_READ_STATUS_SUCCESS && dssearch->type != DS_VAILD_TYPE)
        {
            if (dspack.type == DS_VAILD_TYPE)
            {
                ds_status = STATUS_INVALID_REQUEST;
            }
            else
            {
                read_type_search = DS_READ_TYPE_SEARCH;
                memcpy(dspacktmp, (ds_t *)&dspack, sizeof(ds_t));
            }
        }

        dssearch->offset += DS_HEADER_OFFSET + dspack.len + DS_TAIL_OFFSET;

        address =   (dssearch->address + dssearch->offset);
        maxaddress = dssearch->address + dsinfo.page_size;

        if ((address) > (maxaddress))
        {
            ds_status = STATUS_INVALID_PARAM;
            break;
        }

    } while (ds_status != STATUS_SUCCESS);

    return ds_status;
}



uint32_t ds_vaild(ds_search_t *dssearch, ds_t *ds_get_vaild)
{
    static uint8_t          crc_result, crc;
    uint16_t    magic;
    uint32_t  address;
    ds_t dspack;
    ds_rw_t dscrc;
    address = (dssearch->address + dssearch->offset);

    dspack = (*(ds_t *)(dssearch->address + dssearch->offset));


    if (dspack.type == DS_VAILD_TYPE)
    {
        if (dssearch->flag == DS_VAILD_TYPE_SEARCH)
        {
            dsinfo.current_address = dssearch->address + dssearch->offset;
            dsinfo.current_offset = dssearch->offset;
            return STATUS_SUCCESS;
        }

        return STATUS_TIMEOUT;
    }
    else if (dssearch->type == dspack.type)
    {
        ds_get_vaild->type = dspack.type;
        ds_get_vaild->len = dspack.len;
        ds_get_vaild->sn = dspack.sn;
        address += DS_HEADER_OFFSET;

        ds_get_vaild->buf_addr = address;

        address += dspack.len;
        crc = flash_read_byte(address);

        address += sizeof(uint8_t);
        magic = flash_read_byte(address);

        address += sizeof(uint8_t);
        magic |= flash_read_byte(address) << 8;


        dscrc.type = ds_get_vaild->type;
        dscrc.address = ds_get_vaild->buf_addr;
        dscrc.len = ds_get_vaild->len;
        crc_result = ds_cal_crc(&dscrc, dspack.sn);

        if (crc_result != crc)
        {
            return STATUS_INVALID_PARAM;
        }

        if (dssearch->flag == DS_VAILD_TYPE_SEARCH)
        {
            if (dspack.type >= dsinfo.type_max)
            {
                dsinfo.type_max = dspack.type;
            }

            if (dspack.sn >= dsinfo.current_sn)
            {
                dsinfo.current_sn = dspack.sn;
            }
        }

        ds_get_vaild->crc = crc;
        ds_get_vaild->magic = magic;


        return DS_READ_STATUS_SUCCESS;
    }
    else if (dssearch->type == DS_VAILD_TYPE)
    {

        ds_get_vaild->type = dspack.type;
        ds_get_vaild->len = dspack.len;
        ds_get_vaild->sn = dspack.sn;
        address += DS_HEADER_OFFSET;

        ds_get_vaild->buf_addr = address;

        address += dspack.len;
        crc = flash_read_byte(address);

        address += sizeof(uint8_t);
        magic = flash_read_byte(address);

        address += sizeof(uint8_t);
        magic |= flash_read_byte(address) << 8;

        dscrc.type = ds_get_vaild->type;
        dscrc.address = ds_get_vaild->buf_addr;
        dscrc.len = ds_get_vaild->len;
        crc_result = ds_cal_crc(&dscrc, dspack.sn);


        if (crc_result != crc)
        {
            return STATUS_INVALID_PARAM;
        }

        if (dssearch->flag == DS_VAILD_TYPE_SEARCH)
        {
            if (dspack.type >= dsinfo.type_max)
            {
                dsinfo.type_max = dspack.type;
            }

            if (dspack.sn >= dsinfo.current_sn)
            {
                dsinfo.current_sn = dspack.sn;
            }
        }

        ds_get_vaild->crc = dspack.crc;
        ds_get_vaild->magic = dspack.magic;
    }

    return STATUS_INVALID_REQUEST;
}


uint32_t ds_get_current_page()
{
    return  dsinfo.current_page;
}


uint32_t ds_set_current_page(uint8_t set_page)
{

    if ((set_page > dsinfo.page_number))
    {
        return STATUS_INVALID_PARAM;
    }

    dsinfo.current_page = set_page;
    dsinfo.current_address = dsinfo.start_address + dsinfo.page_size * set_page;

    return STATUS_SUCCESS;
}

uint32_t ds_get_current_page_offset()
{
    return dsinfo.current_offset;
}

uint32_t ds_set_current_page_offset(uint32_t set_page_offset)
{

    uint32_t dsaddress, endaddress;

    dsaddress = dsinfo.current_address + set_page_offset;

    endaddress = dsinfo.start_address + dsinfo.page_size * (dsinfo.current_page + 1);

    if ((dsaddress > endaddress))
    {
        return STATUS_INVALID_PARAM;
    }

    dsinfo.current_address += set_page_offset;

    return STATUS_SUCCESS;
}

uint32_t ds_read(ds_rw_t *ds_read)
{
    uint32_t ds_status;
    uint32_t address;
    ds_search_t dssearch;
    ds_t                dspacktmp;

    if (dsinfo.invaild == DS_INVAILD)
    {
        return STATUS_INVALID_PARAM;
    }

    if (ds_get_current_page() == DS_PAGE_1)
    {
        address = dsinfo.start_address;
    }
    else
    {
        address = dsinfo.start_address + dsinfo.page_size * dsinfo.current_page;
    }

    dssearch.type = ds_read->type;
    dssearch.address = address;
    dssearch.offset = DS_START_OFFSET;
    dssearch.flag = DS_READ_TYPE_SEARCH;

    ds_status = ds_vaild_address_search(&dssearch, &dspacktmp);

    if (ds_status == STATUS_SUCCESS)
    {
        ds_read->address = dspacktmp.buf_addr;
        ds_read->len = dspacktmp.len;
    }
    else
    {
        ds_read->address = 0;
        ds_read->len = 0;
    }

    return ds_status;
}


uint32_t ds_write(ds_rw_t *ds_write)
{

    uint32_t i;
    uint32_t dswraddress, dsmaxaddress;
    uint8_t  bufTemp, crc_result, magic_number;

    if (dsinfo.invaild == DS_INVAILD)
    {
        return STATUS_INVALID_PARAM;
    }

    if ((ds_write->type == DS_INVAILD_A_TYPE) || (ds_write->type == DS_INVAILD_B_TYPE) )
    {
        return STATUS_INVALID_PARAM;
    }


    dsmaxaddress = dsinfo.start_address + (dsinfo.page_size << dsinfo.current_page);
    dswraddress = dsinfo.current_address + DS_HEADER_OFFSET + DS_TAIL_OFFSET + ds_write->len;

    if ((dswraddress > dsmaxaddress))
    {
        //data migration
        if (ds_get_current_page() == 1)
        {
            ds_migration_erase(DS_PAGE_1);
            ds_migration(DS_PAGE_2);
            dsinfo.current_page = DS_PAGE_1;
        }
        else
        {
            ds_migration_erase(DS_PAGE_2);
            ds_migration(DS_PAGE_1);
            dsinfo.current_page = DS_PAGE_2;
        }

        dsinfo.current_address = dsinfo.migration_address;
    }

    dswraddress = dsinfo.current_address;


    flash_write_byte(dswraddress, ds_write->type);
    while (flash_check_busy()) {;}
    dswraddress += 1;
    flash_write_byte(dswraddress, ds_write->len);
    while (flash_check_busy()) {;}
    dswraddress += 1;
    flash_write_byte(dswraddress, ds_write->len >> 8);
    while (flash_check_busy()) {;}
    dswraddress += 1;

    if (dsinfo.current_sn == DS_SERAIL_NUMBER_MAX)
    {
        dsinfo.current_sn = DS_SERAIL_NUMBER_START;
    }

    flash_write_byte(dswraddress, dsinfo.current_sn);
    while (flash_check_busy()) {;}
    dswraddress += 1;
    flash_write_byte(dswraddress, dsinfo.current_sn >> 8);
    while (flash_check_busy()) {;}
    dswraddress += 1;
    flash_write_byte(dswraddress, dsinfo.current_sn >> 16);
    while (flash_check_busy()) {;}
    dswraddress += 1;
    flash_write_byte(dswraddress, dsinfo.current_sn >> 24);
    while (flash_check_busy()) {;}
    dswraddress += 1;

    for (i = 0; i < ds_write->len; i++)
    {
        bufTemp = (*(uint32_t *)(ds_write->address + i));
        flash_write_byte(dswraddress, bufTemp);
        while (flash_check_busy()) {;}
        dswraddress += 1;
    }

    crc_result = ds_cal_crc(ds_write, dsinfo.current_sn);
    flash_write_byte(dswraddress, crc_result);
    while (flash_check_busy()) {;}
    dswraddress += 1;

    magic_number = DS_MAGIC_NUMBER;
    flash_write_byte(dswraddress, magic_number);
    while (flash_check_busy()) {;}
    dswraddress += 1;
    flash_write_byte(dswraddress, magic_number >> 8);
    while (flash_check_busy()) {;}
    dswraddress += 1;

    dsinfo.current_address = dswraddress;
    dsinfo.current_offset = dswraddress;
    dsinfo.current_sn += 1;

    return STATUS_SUCCESS;

}

uint32_t ds_migration(ds_page_t page)
{

    uint32_t        ds_status;
    uint32_t        address, end_address, i;
    uint8_t         update_flag;
    ds_search_t dssearch;
    ds_rw_t         dswr;
    ds_t                dspack;
    ds_t                ds_migration_after;
    ds_t                ds_migration_befor;

    if (dsinfo.invaild == 1)
    {
        return STATUS_INVALID_PARAM;
    }

    if (page == DS_PAGE_1)
    {
        address = dsinfo.start_address;
        end_address = dsinfo.start_address + dsinfo.page_size;
        dsinfo.migration_address = dsinfo.start_address + dsinfo.page_size;
    }
    else
    {
        address = dsinfo.start_address + dsinfo.page_size;
        end_address = dsinfo.start_address + dsinfo.page_size * (page + 1);
        dsinfo.migration_address = dsinfo.start_address;
    }

    update_flag = 0;

    for (i = 1; i <= dsinfo.type_max; i++)
    {
        dssearch.address = address;
        dssearch.offset = DS_START_OFFSET;
        dssearch.type = (uint8_t)i;
        dssearch.flag = DS_READ_TYPE_SEARCH;

        do
        {

            dspack = (*(ds_t *)(dssearch.address + dssearch.offset));

            ds_status = ds_vaild(&dssearch, &dspack);

            if (ds_status == DS_READ_STATUS_SUCCESS && dssearch.type != DS_VAILD_TYPE)
            {
                if (update_flag == 0)
                {
                    memcpy((ds_t *)&ds_migration_befor, (ds_t *)&dspack, sizeof(ds_t));
                    update_flag = 1;
                }
                else
                {
                    memcpy((ds_t *)&ds_migration_after, (ds_t *)&dspack, sizeof(ds_t));

                    if ((ds_migration_after.sn) > (ds_migration_befor.sn))
                    {
                        memcpy((ds_t *)&ds_migration_befor, (ds_t *)&ds_migration_after, sizeof(ds_t));
                        update_flag = 2;
                    }
                }
            }

            dssearch.offset += DS_HEADER_OFFSET + dspack.len + DS_TAIL_OFFSET;

            if ((dssearch.address + dssearch.offset) > end_address)
            {
                break;
            }

        } while (ds_status != STATUS_SUCCESS);

        if (update_flag != 0)
        {
            dswr.type = ds_migration_befor.type;
            dswr.len = ds_migration_befor.len;
            dswr.address = ds_migration_befor.buf_addr;
            ds_migration_write(page, &dswr);
            update_flag = 0;
        }
    }

    return STATUS_SUCCESS;
}
uint32_t ds_migration_write(ds_page_t page, ds_rw_t *ds_write)
{

    uint32_t i;
    uint32_t dswraddress;
    uint8_t  bufTemp, crc_result, magic_number;

    if (dsinfo.invaild == DS_INVAILD)
    {
        return STATUS_INVALID_PARAM;
    }

    if ((ds_write->type == DS_INVAILD_A_TYPE) || (ds_write->type == DS_INVAILD_B_TYPE))
    {
        return STATUS_INVALID_REQUEST;
    }

    dswraddress = dsinfo.migration_address;

    crc_result = ds_cal_crc(ds_write, dsinfo.current_sn);
    magic_number = DS_MAGIC_NUMBER;

    flash_write_byte(dswraddress, ds_write->type);
    while (flash_check_busy()) {;}
    dswraddress += 1;
    flash_write_byte(dswraddress, ds_write->len);
    while (flash_check_busy()) {;}
    dswraddress += 1;
    flash_write_byte(dswraddress, ds_write->len >> 8);
    while (flash_check_busy()) {;}
    dswraddress += 1;

    if (dsinfo.current_sn == DS_SERAIL_NUMBER_MAX)
    {
        dsinfo.current_sn = DS_SERAIL_NUMBER_START;
    }

    flash_write_byte(dswraddress, dsinfo.current_sn);
    while (flash_check_busy()) {;}
    dswraddress += 1;
    flash_write_byte(dswraddress, dsinfo.current_sn >> 8);
    while (flash_check_busy()) {;}
    dswraddress += 1;
    flash_write_byte(dswraddress, dsinfo.current_sn >> 16);
    while (flash_check_busy()) {;}
    dswraddress += 1;
    flash_write_byte(dswraddress, dsinfo.current_sn >> 24);
    while (flash_check_busy()) {;}
    dswraddress += 1;

    for (i = 0; i < ds_write->len; i++)
    {
        bufTemp = (*(uint32_t *)(ds_write->address + i));
        flash_write_byte(dswraddress, bufTemp);
        while (flash_check_busy()) {;}
        dswraddress += 1;
    }

    crc_result = ds_cal_crc(ds_write, dsinfo.current_sn);
    flash_write_byte(dswraddress, crc_result);
    while (flash_check_busy()) {;}
    dswraddress += 1;

    magic_number = DS_MAGIC_NUMBER;
    flash_write_byte(dswraddress, magic_number);
    while (flash_check_busy()) {;}
    dswraddress += 1;
    flash_write_byte(dswraddress, magic_number >> 8);
    while (flash_check_busy()) {;}
    dswraddress += 1;

    dsinfo.migration_address = dswraddress;
    dsinfo.current_sn += 1;

    return STATUS_SUCCESS;
}

uint32_t ds_migration_erase(ds_page_t page)
{
    uint32_t i, end_addr;
    uint32_t  start_addr;

    if ((dsinfo.invaild == DS_INVAILD) || (page > DS_PAGE_2))
    {
        return STATUS_INVALID_PARAM;
    }

    end_addr = dsinfo.start_address + (dsinfo.page_size << page);
    start_addr = dsinfo.start_address + (dsinfo.page_size * page);

    for (i = start_addr; i < end_addr; i += DATA_SHEET_ERASE_SIZE)
    {
        flash_erase(FLASH_ERASE_SECTOR, i);
    }

    return STATUS_SUCCESS;
}

uint32_t ds_reset_to_default()
{
    uint32_t i, end_addr;
    uint32_t  start_addr;

    if (dsinfo.invaild == DS_INVAILD)
    {
        return STATUS_INVALID_PARAM;
    }

    end_addr = dsinfo.end_address;
    start_addr = dsinfo.start_address;

    for (i = start_addr; i < end_addr; i += DATA_SHEET_ERASE_SIZE)
    {
        flash_erase(FLASH_ERASE_SECTOR, i);
    }

    dsinfo.current_page = DS_PAGE_1;
    dsinfo.current_offset = DS_START_OFFSET;
    dsinfo.current_address = start_addr;
    dsinfo.current_sn = DS_SERAIL_NUMBER_START;

    return STATUS_SUCCESS;
}

uint8_t ds_cal_crc(ds_rw_t *ds_crc, uint32_t sn)
{
    uint8_t value = 0, crc_result = 0;
    uint32_t i = 0;


    crc_result ^= (ds_crc->type);
    crc_result ^= (ds_crc->len & 0xFF);
    crc_result ^= (ds_crc->len >> 8);

    crc_result ^= (sn & 0xFF);
    crc_result ^= (sn >> 8);
    crc_result ^= (sn >> 16);
    crc_result ^= (sn >> 24);

    for (i = 0; i < ds_crc->len; i++)
    {
        value = (*(uint32_t *)(ds_crc->address + i));
        crc_result ^= value;
    }

    return crc_result;
}

