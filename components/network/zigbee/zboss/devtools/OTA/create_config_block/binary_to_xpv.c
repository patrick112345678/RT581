/* ZBOSS Zigbee software protocol stack
 *
 * Copyright (c) 2012-2020 DSR Corporation, Denver CO, USA.
 * www.dsr-zboss.com
 * www.dsr-corporation.com
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
/* PURPOSE: Binary to XPV tool.
*/

#include <ctype.h>
#include <stdio.h>

#include "zb_types.h"
#include "zb_errors.h"
#include "zb_osif.h"

void print_to_xpv(zb_uint8_t *data_buffer, zb_uint32_t data_length);
zb_ret_t parse_input_file(ZB_CONST zb_char_t *filename);

FILE *xpv_file;
zb_uint32_t block_address = 0;
char xpv_filename[80];

#define MAX_FILE_STRING_LENGTH 128
#define BLOCK_SIZE 16

MAIN()
{
    if (argc < 3)
    {
        fprintf(stdout,
                "ZBOSS binary to XPV convert utility.\n"
                "Usage: binary_to_xpv.exe <binary file> <start address>\n"
                "Example: binary_to_xpv.exe input.bin 0x3C000\n\n"
               );
        return 1;
    }

    strcpy(xpv_filename, argv[1]);
    strcat(xpv_filename, ".xpv");
    xpv_file = fopen(xpv_filename, "wb");

    if (sscanf(argv[2], "0x%x", &block_address) != 1)
    {
        fprintf(stdout,
                "can not parse address from %s\n",
                argv[2]
               );
        return 1;
    }
    fprintf(stdout,
            "output file %s start address %06X\n",
            xpv_filename, block_address
           );

    if (parse_input_file(argv[1]) == RET_OK)
    {
        printf("Success.\n");
    }
    else
    {
        printf("Failure.\n");
    }

    fclose(xpv_file);

    MAIN_RETURN(0);
}

zb_uint32_t calc_file_size(FILE *r_file)
{
    zb_uint32_t r_offset;
    fseek(r_file, 0L, SEEK_END);
    r_offset = ftell(r_file);
    fseek(r_file, 0L, SEEK_SET);
    return r_offset;
}


zb_ret_t parse_input_file(ZB_CONST zb_char_t *filename)
{
    FILE *input_file;
    zb_bool_t dataset_is_valid = ZB_FALSE;
    zb_ret_t ret = RET_OK;
    zb_uint32_t datasets_counter = 0;
    zb_uint8_t pos = 0;
    zb_uint16_t uint16_val = 0;
    zb_uint8_t uint8_val = 0;
    zb_uint8_t buf[BLOCK_SIZE];
    zb_uint16_t len = 0;
    zb_uint32_t r_size = 0;
    zb_uint32_t r_pos = 0;
    zb_uint32_t block_len = BLOCK_SIZE;
    //zb_uint32_t cfg_block_pattern = 0xdeadf00d;

    input_file = fopen(filename, "rb");

    if (input_file == NULL)
    {
        ret = RET_ERROR;
        printf("%s not found\n", filename);
        return ret;
    }

    r_size = calc_file_size(input_file);

    //print_to_xpv((zb_uint8_t*)&cfg_block_pattern, sizeof(cfg_block_pattern));

    //while (!feof(input_file))
    while (r_pos < r_size)
    {
        if (r_size < BLOCK_SIZE + r_pos)
        {
            block_len = r_size - r_pos;
        }
        else
        {
            block_len = BLOCK_SIZE;
        }

        if (block_len == 0)
        {
            break;
        }

        if (ret = fread(buf, sizeof(zb_uint8_t), block_len, input_file) != block_len)
        {
            printf("fread: read %d expected %d\n", ret, block_len);
        }
        print_to_xpv(buf, block_len);

        r_pos += block_len;
    }

    fflush(xpv_file);
    fclose(input_file);
    return ret;
}

void print_to_xpv(zb_uint8_t *data_buffer, zb_uint32_t data_length)
{
    while (data_length > 0)
    {
        fprintf(xpv_file, "@%06X   %02X\n", block_address, data_buffer[0]);
        ++data_buffer;
        --data_length;
        ++block_address;
    }
}
