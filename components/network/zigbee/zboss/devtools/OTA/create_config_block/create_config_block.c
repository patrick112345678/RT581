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
/* PURPOSE: Create config block tool.
*/

#include <ctype.h>
#include <stdio.h>

#include "zb_types.h"
#include "zb_errors.h"
#include "zb_osif.h"

/* Global context. */
FILE *binary_file;

#define INPUT_FILE "config.txt"
#define OUTPUT_FILE "config_block.bin"

#define MAX_CTN_STR_SIZE    16

#define MAX_FILE_STRING_LENGTH 128

zb_ret_t parse_input_file(ZB_CONST zb_char_t *filename);


MAIN()
{
    /* TODO Use argv, argc. */

    printf("create_config_block tool started.\n");

    binary_file = fopen(OUTPUT_FILE, "wb");

    printf("input file parser started.\n");
    if (parse_input_file(INPUT_FILE) == RET_OK)
    {
        printf("SUCCESS.\n");
    }
    else
    {
        printf("FAILURE.\n");
    }
    printf("input file parser stopped.\n");

    printf("create_config_block tool stopped.\n");

    fclose(binary_file);

    MAIN_RETURN(0);
}

int char_to_hex(char c)
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }
    c = toupper(c);
    if (c >= 'A' && c <= 'F')
    {
        return c - 'A' + 10;
    }
    return -1;
}

/**
 *  @brief parse 4-char string to uint16
 *
 *  @param str string with number ('0x' omitted)
 */
zb_uint16_t hex_str_to_uint16(char *str)
{
    zb_uint_t v;
    sscanf(str, "%x", &v);
    return (zb_uint16_t)v;
}

zb_uint8_t hex_str_to_uint8(char *str)
{
    zb_uint8_t res = char_to_hex(str[1]);
    res += char_to_hex(str[0]) << 4;
    return res;
}

zb_ret_t parse_input_file(ZB_CONST zb_char_t *filename)
{
    FILE *input_file;
    zb_bool_t dataset_is_valid = ZB_FALSE;
    zb_char_t current_line[MAX_FILE_STRING_LENGTH];
    zb_ret_t ret = RET_ERROR;
    zb_uint32_t datasets_counter = 0;
    zb_uint8_t pos = 0;
    zb_uint16_t uint16_val = 0;
    zb_uint8_t uint8_val = 0;
    zb_uint8_t char_buf[MAX_CTN_STR_SIZE + 1] = {0};
    zb_uint8_t *data_buffer;
    zb_uint32_t data_length = 0;
    zb_uint8_t i = 0;
    zb_uint32_t cfg_block_pattern = 0xdeadf00d;

    input_file = fopen(filename, "r");

    if (input_file == NULL)
    {
        ret = RET_ERROR;
        printf("%s not found\n", filename);
        return ret;
    }

    do
    {
        fgets(current_line, MAX_FILE_STRING_LENGTH, input_file);

        if (!strncmp(current_line, "config_version=0x", 17))
        {
            /* printf("%s", current_line); */
            uint16_val = hex_str_to_uint16(current_line + 17);
            uint16_val = (uint16_val & 0x00FF);
            data_buffer = (zb_uint8_t *)&uint16_val;
            data_length = sizeof(uint16_val);
        }
        else if (!strncmp(current_line, "hw_config=0x", 12))
        {
            uint16_val = hex_str_to_uint16(current_line + 12);
            data_buffer = (zb_uint8_t *)&uint16_val;
            data_length = sizeof(uint16_val);
        }
        else if (!strncmp(current_line, "ctn=", 4))
        {
            int ctn_length = strlen(current_line + 4) - 1;
            strncpy(char_buf, current_line + 4, MAX_CTN_STR_SIZE);
            char_buf[ctn_length] = 0;
            data_buffer = char_buf;
            data_length = MAX_CTN_STR_SIZE;
        }
        else if (!strncmp(current_line, "customer_id=0x", 14))
        {
            uint16_val = hex_str_to_uint16(current_line + 14);
            data_buffer = (zb_uint8_t *)&uint16_val;
            data_length = sizeof(uint16_val);
        }
        else if (!strncmp(current_line, "power_level=0x", 14))
        {
            uint8_val = hex_str_to_uint8(current_line + 14);
            data_buffer = &uint8_val;
            data_length = sizeof(uint8_val);
        }
        else if (!strncmp(current_line, "protocol_id=0x", 14))
        {
            uint8_val = hex_str_to_uint8(current_line + 14);
            data_buffer = &uint8_val;
            data_length = sizeof(uint8_val);
        }
        else if (!strncmp(current_line, "aux_features=0x", 15))
        {
            uint8_val = hex_str_to_uint8(current_line + 15);
            data_buffer = &uint8_val;
            data_length = sizeof(uint8_val);
        }
        else if (!strncmp(current_line, "lockout=0x", 10))
        {
            uint8_val = hex_str_to_uint8(current_line + 10);
            data_buffer = &uint8_val;
            data_length = sizeof(uint8_val);
        }
        else if (!strncmp(current_line, "pir=0x", 6))
        {
            uint8_val = hex_str_to_uint8(current_line + 6);
            data_buffer = &uint8_val;
            data_length = sizeof(uint8_val);
        }
        else if (!strncmp(current_line, "mw=0x", 5))
        {
            uint8_val = hex_str_to_uint8(current_line + 5);
            data_buffer = &uint8_val;
            data_length = sizeof(uint8_val);
        }
        else if (!strncmp(current_line, "temperature=0x", 14))
        {
            uint8_val = hex_str_to_uint8(current_line + 14);
            data_buffer = &uint8_val;
            data_length = sizeof(uint8_val);
        }
        else if (!strncmp(current_line, "ambient=0x", 10))
        {
            uint8_val = hex_str_to_uint8(current_line + 10);
            data_buffer = &uint8_val;
            data_length = sizeof(uint8_val);
        }
        else if (!strncmp(current_line, "rel_humidity=0x", 15))
        {
            uint8_val = hex_str_to_uint8(current_line + 15);
            data_buffer = &uint8_val;
            data_length = sizeof(uint8_val);
        }
        else if (!strncmp(current_line, "pressure=0x", 11))
        {
            uint8_val = hex_str_to_uint8(current_line + 11);
            data_buffer = &uint8_val;
            data_length = sizeof(uint8_val);
        }
        else if (!strncmp(current_line, "vibration=0x", 12))
        {
            uint8_val = hex_str_to_uint8(current_line + 12);
            data_buffer = &uint8_val;
            data_length = sizeof(uint8_val);
        }
        else if (!strncmp(current_line, "tilt=0x", 7))
        {
            uint8_val = hex_str_to_uint8(current_line + 7);
            data_buffer = &uint8_val;
            data_length = sizeof(uint8_val);
            /* Last block */
            ret = RET_OK;
        }

        if (data_length)
        {
            fwrite(data_buffer, sizeof(zb_uint8_t), data_length, binary_file);
            data_length = 0;
        }
    } while (ret != RET_OK && !feof(binary_file));

    /* Add 6 reserved bytes */
    for (i = 0; i < 6; ++i)
    {
        fwrite(&i, sizeof(zb_uint8_t), sizeof(i), binary_file);
    }
    fclose(input_file);
    return ret;
}
