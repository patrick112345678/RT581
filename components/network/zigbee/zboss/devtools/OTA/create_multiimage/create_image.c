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
/* PURPOSE: Utility for multi-component OTA image creation
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#ifndef ZB_WINDOWS
#include "zb_common.h"
#else
#include "zb_types.h"
#endif /* !ZB_WINDOWS */

#define CSS_OTA_ENCRYPTION
#define CSS_MULTIIMAGE_UPGRADE

#ifndef ZB_WINDOWS
#include "css_device.h"
#endif /* !ZB_WINDOWS */
#include "css_ota.h"

#ifndef CSS_OTA_ENCRYPTION

#include "zb_osif.h"
#include "AES128.h"
#include "AESMMO.h"

#else

void ota_create_nonce(zb_uint8_t nonce[16], zb_uint32_t file_version, zb_uint32_t image_tag);
void ota_auth_crc(zb_uint8_t *data, zb_uint32_t size, zb_uint32_t *crc_p);
void css_ota_encrypt_n_auth(
    zb_uint8_t *key,
    zb_uint8_t *nonce,
    zb_uint8_t *string_a,
    zb_uint_t string_a_len,
    zb_uint8_t *string_m,
    zb_uint_t string_m_len,
    zb_uint8_t *crypted_text);


#endif  /* !CSS_OTA_ENCRYPTION */

#define  BLOCK_SIZE 16


#define ZB_ZCL_OTA_UPGRADE_FILE_HEADER_FILE_ID          0x0BEEF11E
#define ZB_ZCL_OTA_UPGRADE_FILE_HEADER_FILE_VERSION     0x0100
#define ZB_ZCL_OTA_UPGRADE_FILE_HEADER_LENGTH           0x0038 /* sizeof(zb_zcl_ota_upgrade_file_header_t) */
#define ZB_ZCL_OTA_UPGRADE_FILE_HEADER_FC               ((1<<0/*security*/)|(1<<2/*hw versions present*/)|0x0010/*hmm*/)
#define CSS_INIT_OTA_DEFAULT_IMAGE_TYPE         0x0141
#define ZB_ZCL_OTA_UPGRADE_FILE_HEADER_STACK_PRO        0x0002
#define OTA_UPGRADE_TEST_IMAGE_SIZE                     0xFFFFFFFF

#define MAIN_IMAGE_TAG 0

/* CR: 06/22/2015 [DT] */
#define USAGE_STRING        "Usage: {file_version} {manufacturer_code} {image_type} {radio app version} {sensor app version}  {'rl78only'|'rfonly'|cfgonly'}\n\
Example: create_image.exe v2421 0x1133 0x0401 0x1234 0x5678"

/* Size of the buffer used to store and process program parameters */
#define ARG_BUF_SIZE                                    10

#define CRC_SIZE                                    sizeof(uint32)

#define FINAL_IMAGE_NAME "write.bin"
#define MULTIIMAGE_NAME "multiimage.bin"
#define BMS_HEADER_SUBELEMENT_TAG 0xFB00
#define RADIO_FW_IMAGE_NAME "radio_firmware"
#define RADIO_FW_IMAGE_TAG 0xFB01
#define SENSOR_FW_IMAGE_NAME "sensor_firmware"
#define SENSOR_FW_IMAGE_TAG 0xFB02
#define CONFIG_BLOCK_IMAGE_NAME "config_block"
#define CONFIG_BLOCK_IMAGE_TAG 0xFB03


static zb_uint32_t calc_file_size(FILE *r_file);

#pragma pack(1)

#ifndef CSS_OTA_ENCRYPTION
zb_ret_t add_image_with_crc(char *r_filename, char *w_filename, zb_uint16_t tag_id)
{
    /* Hash is always 128-bits. */
    zb_uint8_t tf[16] = {0}; // hash with blockchaining
    zb_uint32_t mlen = BLOCK_SIZE;
    zb_uint8_t msg[BLOCK_SIZE];
    zb_uint32_t r_offset = 0;
    zb_uint32_t r_len = 0;
    FILE *r_file, *w_file;
    zb_uint32_t crc_value = 0;

    memset(&tf, 0, sizeof(tf));
    memset(&msg, 0, sizeof(msg));

    r_file = fopen(r_filename, "rb");
    if (!r_file)
    {
        printf("%s not found \n", r_filename);
        return RET_ERROR;
    }
    w_file = fopen(w_filename, "ab");
    if (!w_file)
    {
        printf("%s failed fopen \n", w_filename);
        return RET_ERROR;
    }

    r_offset = calc_file_size(r_file);
    printf("%s file : img_size is %d \n", r_filename, r_offset);

    /* Write tag_id and length before copying image (if it is sub-element). */
    if (tag_id != MAIN_IMAGE_TAG)
    {
        zb_uint32_t len = r_offset + CRC_SIZE;
        fwrite(&tag_id, sizeof(tag_id), 1, w_file);
        fwrite(&len, sizeof(len), 1, w_file);
    }

    while (!feof(r_file))
    {
        if (r_offset < BLOCK_SIZE + r_len)
        {
            mlen = r_offset - r_len;
        }
        else
        {
            mlen = BLOCK_SIZE;
        }

        if (mlen == 0)
        {
            break;
        }

        fread(msg, mlen, 1, r_file);

        /* NK:FIXME: why mlen*4, why WITH_BLOCK_CHAINING? */
        AES_MMO(msg, mlen * 4, tf, WITH_BLOCK_CHAINING);

        fwrite(msg, mlen, 1, w_file);
        memset(&msg, 0, sizeof(msg));
        r_len += mlen;

        if (feof(r_file))
        {
            break;
        }
    }
    fflush(w_file);

    fwrite(&tf, CRC_SIZE, 1, w_file); // add 4-byte CRC

    fclose(r_file);
    fclose(w_file);

    return RET_OK;
}
#endif


#ifdef CSS_OTA_ENCRYPTION
zb_ret_t add_image_with_encr(char *r_filename, char *w_filename, zb_uint32_t file_version, zb_uint16_t tag_id)
{
    zb_uint8_t nonce[16];
    zb_uint32_t size = 0;
    FILE *r_file, *w_file;
    css_ota_section_hdr_t section_hdr;
    zb_uint8_t *data;
    zb_uint8_t *encrypted;

    ota_create_nonce(nonce, file_version, (zb_uint32_t)tag_id);

    r_file = fopen(r_filename, "rb");
    if (!r_file)
    {
        printf("%s not found. Omitting corresponding section \n", r_filename);
        return RET_OK;
    }
    w_file = fopen(w_filename, "ab");
    if (!w_file)
    {
        printf("%s failed fopen \n", w_filename);
        return RET_ERROR;
    }

    size = calc_file_size(r_file);
    printf("%s file : img_size is %d \n", r_filename, size);

    section_hdr.tag_id = tag_id;
    section_hdr.length = size + CSS_OTA_CCM_M;
    fwrite(&section_hdr, sizeof(section_hdr), 1, w_file);

    data = malloc(section_hdr.length);
    encrypted = malloc(section_hdr.length);
    fread(data, size, 1, r_file);

#ifdef VERBOSE
    {
        int i;
        printf("size %d section tag %x length %d nonce ", size, section_hdr.tag_id, section_hdr.length);
        for (i = 0 ; i < 13 ; ++i)
        {
            printf("%02X ", nonce[i]);
        }
        printf("A[6] ");
        for (i = 0 ; i < 6 ; ++i)
        {
            printf("%02X ", ((zb_uint8_t *)&section_hdr)[i]);
        }
    }
#endif
    css_ota_encrypt_n_auth(OTA_ENCRYPT_KEY, nonce,
                           (zb_uint8_t *)&section_hdr, sizeof(section_hdr), /* a */
                           data, size, encrypted);

    fwrite(encrypted, section_hdr.length, 1, w_file);

    fclose(r_file);
    fclose(w_file);
    free(data);
    free(encrypted);

    return RET_OK;
}
#endif


/* CR: 06/22/2015 [DT] Start */
/* Convert given char to matching hex digit.
 * return -1 if char is invalid.
 */
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
 * @brief parse version number
 *
 * @param str string of format "vABCD"
 */
zb_uint32_t parse_version(char *str)
{
    /* Skip first char 'v' */
    zb_uint32_t res;
    sscanf(str + 1, "%x", &res);
    return res;
}

/**
 *  @brief parse 4-char string to uint16
 *
 *  @param str string with number ('0x' omitted)
 */
zb_uint16_t hex_str_to_uint16(char *str)
{
    zb_uint16_t res = char_to_hex(str[3]);
    res += char_to_hex(str[2]) << 4;
    res += char_to_hex(str[1]) << 8;
    res += char_to_hex(str[0]) << 12;
    return res;
}
/* CR: 06/22/2015 [DT] End */

static zb_uint32_t calc_file_size(FILE *r_file)
{
    zb_uint32_t r_offset;
    fseek(r_file, 0L, SEEK_END);
    r_offset = ftell(r_file);
    fseek(r_file, 0L, SEEK_SET);
    return r_offset;
}


int main(int argc, char *argv[])
{
    zb_ret_t ret = RET_OK;
    zb_uint8_t t[16] = {0}; // hash (with blockchaining)
    zb_uint8_t msg[BLOCK_SIZE];
#ifndef CSS_OTA_ENCRYPTION
    zb_uint32_t mlen = BLOCK_SIZE;
    zb_uint32_t r_offset = 0;
    zb_uint32_t r_len = 0;
#endif
    FILE *w_file, *multiimage_file, *config_file;
    char buf[ARG_BUF_SIZE];
    int sections_present[3] = {1, 1, 1};

    css_ota_file_hdr_t ota_header =
    {
        ZB_ZCL_OTA_UPGRADE_FILE_HEADER_FILE_ID,         // OTA upgrade file identifier
        ZB_ZCL_OTA_UPGRADE_FILE_HEADER_FILE_VERSION,    // OTA Header version
        ZB_ZCL_OTA_UPGRADE_FILE_HEADER_LENGTH,          // OTA Header length
        ZB_ZCL_OTA_UPGRADE_FILE_HEADER_FC,              // OTA Header Field control
        0,                                              // Manufacturer code
        CSS_INIT_OTA_DEFAULT_IMAGE_TYPE,                // Image type
        0,                                              // File version
        ZB_ZCL_OTA_UPGRADE_FILE_HEADER_STACK_PRO,       // Zigbee Stack version
        // OTA Header string
        {
            0x54, 0x68, 0x65, 0x20, 0x6c, 0x61, 0x74, 0x65,   0x73, 0x74, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x67,
            0x72, 0x65, 0x61, 0x74, 0x65, 0x73, 0x74, 0x20,   0x75, 0x70, 0x67, 0x72, 0x61, 0x64, 0x65, 0x2e,
        },
        OTA_UPGRADE_TEST_IMAGE_SIZE,                    // dummy : Total Image size (including header)
        0,   /**< Security credential version */
        0,   /**< Minimum hardware version */
        65535    /**< Maximum hardware version: for now fields are present, but no
            * real control*/
    };

    memset(t, 0, sizeof(t));
    memset(msg, 0, sizeof(msg));
    memset(buf, 0, sizeof(buf));

    /*
     * Expected argv contents:
     * 1 - file_version,
     * 2 - manufacturer code,
     * 3 - image type (default value CSS_INIT_OTA_DEFAULT_IMAGE_TYPE)
     * 4 - radio app version
     * 5 - sensor app version
     */
    if (argc < 6)
    {
        printf("Wrong number of arguments supplied.\n" USAGE_STRING);
        return 1;
    }
    if (argc == 7
            && !strcmp(argv[6], "rl78only"))
    {
        memset(sections_present, 0, sizeof(sections_present));
        sections_present[1] = 1;
    }
    if (argc == 7
            && !strcmp(argv[6], "rfonly"))
    {
        memset(sections_present, 0, sizeof(sections_present));
        sections_present[0] = 1;
    }
    if (argc == 7
            && !strcmp(argv[6], "cfgonly"))
    {
        memset(sections_present, 0, sizeof(sections_present));
        sections_present[2] = 1;
    }
    buf[ARG_BUF_SIZE - 1] = '\0';
    strncpy(buf, argv[1], sizeof(buf) - 1);
    if (strlen(buf) != 9 || buf[0] != 'v')
    {
        buf[ARG_BUF_SIZE - 1] = '\0';
        printf("incorrect version format : use format v01ABCDEF instead of %s\n", buf);
        ret = RET_ERROR;
    }
    if (ret == RET_OK)
    {
        ota_header.std.m.file_version = parse_version(buf);

        /* parse manufacturer code */
        memset(buf, 0, sizeof(buf));
        strncpy(buf, argv[2], sizeof(buf) - 1);
        if (strlen(buf) != 6 || buf[0] != '0' || (buf[1] != 'x' && buf[1] != 'X'))
        {
            buf[ARG_BUF_SIZE - 1] = '\0';
            printf("Invalid manufacturer code : use format 0x01AB instead of %s\n", buf);
            ret = RET_ERROR;
        }
    }
    if (ret == RET_OK)
    {
        ota_header.std.m.manufacturer_code = hex_str_to_uint16(buf + 2);

        /* image type */
        memset(buf, 0, sizeof(buf));
        strncpy(buf, argv[3], sizeof(buf) - 1);
        if (strlen(buf) != 6 || buf[0] != '0' || (buf[1] != 'x' && buf[1] != 'X'))
        {
            buf[ARG_BUF_SIZE - 1] = '\0';
            printf("Invalid image type : use format 0x01AB instead of %s\n", buf);
            ret = RET_ERROR;
        }
    }
    if (ret == RET_OK)
    {
        ota_header.std.m.image_type = hex_str_to_uint16(buf + 2);

        /* radio app version */
        memset(buf, 0, sizeof(buf));
        strncpy(buf, argv[4], sizeof(buf) - 1);
        if (strlen(buf) != 6 || buf[0] != '0' || (buf[1] != 'x' && buf[1] != 'X'))
        {
            buf[ARG_BUF_SIZE - 1] = '\0';
            printf("Invalid image type : use format 0x01AB instead of %s\n", buf);
            ret = RET_ERROR;
        }
    }
    if (ret == RET_OK)
    {
        ota_header.bms.section_versions[CSS_OTA_RF_IMAGE_I] = hex_str_to_uint16(buf + 2);

        /* sensor app version */
        memset(buf, 0, sizeof(buf));
        strncpy(buf, argv[5], sizeof(buf) - 1);
        if (strlen(buf) != 6 || buf[0] != '0' || (buf[1] != 'x' && buf[1] != 'X'))
        {
            buf[ARG_BUF_SIZE - 1] = '\0';
            printf("Invalid image type : use format 0x01AB instead of %s\n", buf);
            ret = RET_ERROR;
        }
    }
    if (ret == RET_OK)
    {
        ota_header.bms.section_versions[CSS_OTA_SENSOR_IMAGE_I] = hex_str_to_uint16(buf + 2);

        config_file = fopen(CONFIG_BLOCK_IMAGE_NAME, "rb");
        if (config_file && calc_file_size(config_file) != 0)
        {
            zb_uint16_t cfg_version;

            fseek(config_file, 0L, SEEK_SET);
            fread(&cfg_version, 1, 1, config_file);
            cfg_version = (cfg_version & 0x00FF);
            ota_header.bms.section_versions[CSS_OTA_CONFIG_IMAGE_I] = cfg_version;
            ota_header.std.m.file_version = (ota_header.std.m.file_version & 0xFF00FFFF) | (cfg_version << 16);
            fclose(config_file);
        }

        printf("setting version number = 0x%08X\n", ota_header.std.m.file_version);
        printf("setting manufacturer code = 0x%04X\n", ota_header.std.m.manufacturer_code);
        printf("setting image_type = 0x%04X\n", ota_header.std.m.image_type);

        /* It is supposed that Configuration block is "usual" image (as anothers). Use special utility to
         * prepare Configuration block. */
        /* NK:TODO: Prepare this utility. Use precommissioning tool from lcgw. */
        /* Firstly prepare and paste together sub-elements, then prepare OTA and BMS headers, then
         * combine it. */

        /* Just erase MULTIIMAGE_NAME file if it is exists. */
        w_file = fopen(MULTIIMAGE_NAME, "wb");
        if (!w_file)
        {
            printf("%s failed fopen \n", MULTIIMAGE_NAME);
            ret = RET_ERROR;
        }
    }
    if (ret == RET_OK)
    {
        fflush(w_file);
        fclose(w_file);
    }

#ifdef CSS_OTA_ENCRYPTION
    if (sections_present[0])
    {
        ret = add_image_with_encr(RADIO_FW_IMAGE_NAME, MULTIIMAGE_NAME, ota_header.std.m.file_version, RADIO_FW_IMAGE_TAG);
    }
    if (ret == RET_OK && sections_present[1])
    {
        ret = add_image_with_encr(SENSOR_FW_IMAGE_NAME, MULTIIMAGE_NAME, ota_header.std.m.file_version, SENSOR_FW_IMAGE_TAG);
    }
    if (ret == RET_OK && sections_present[2])
    {
        ret = add_image_with_encr(CONFIG_BLOCK_IMAGE_NAME, MULTIIMAGE_NAME, ota_header.std.m.file_version, CONFIG_BLOCK_IMAGE_TAG);
    }
#else
    ret = add_image_with_crc(RADIO_FW_IMAGE_NAME, MULTIIMAGE_NAME, RADIO_FW_IMAGE_TAG);
    if (ret == RET_OK)
    {
        ret = add_image_with_crc(SENSOR_FW_IMAGE_NAME, MULTIIMAGE_NAME, SENSOR_FW_IMAGE_TAG);
    }
    if (ret == RET_OK)
    {
        ret = add_image_with_crc(CONFIG_BLOCK_IMAGE_NAME, MULTIIMAGE_NAME, CONFIG_BLOCK_IMAGE_TAG);
    }
#endif

    if (ret == RET_OK)
    {
        /* Prepare OTA header */
        multiimage_file = fopen(MULTIIMAGE_NAME, "rb");
        if (!multiimage_file)
        {
            printf("%s failed fopen \n", MULTIIMAGE_NAME);
            ret = RET_ERROR;
        }
        if (ret == RET_OK)
        {
            ota_header.std.m.total_image_size = sizeof(ota_header);
            ota_header.std.m.total_image_size += calc_file_size(multiimage_file); /* actual fw size */
            fclose(multiimage_file);
            ota_header.std.m.header_length = sizeof(ota_header);

            /* Prepare BMS header */
            ota_header.bms.tag_id = BMS_HEADER_SUBELEMENT_TAG;
            /* Q: Should we include CRC size in Length (check all cases)?
               A: YES. */
            ota_header.bms.length = sizeof(ota_header.bms) - sizeof(ota_header.bms.tag_id) - sizeof(ota_header.bms.length);

            /* Fill OTA header CRC in BMS header and BMS header CRC */
#ifdef CSS_OTA_ENCRYPTION
            /* Use same CCM* for 32-bit CRC. Reason: simplify client side, decrease its
             * code size. */
            ota_auth_crc((zb_uint8_t *)&ota_header, sizeof(ota_header.std), &ota_header.bms.fh_crc);
            ota_auth_crc((zb_uint8_t *)&ota_header.bms, sizeof(ota_header.bms) - sizeof(ota_header.bms.bms_section_crc), &ota_header.bms.bms_section_crc);
#else
            memset(&t, 0, sizeof(t));
            AES_MMO((zb_uint8_t *)&ota_header, sizeof(ota_header), t, WITH_BLOCK_CHAINING);
            memcpy(&(ota_header.bms.ota_header_crc), t, CRC_SIZE);

            memset(&t, 0, sizeof(t));
            AES_MMO((zb_uint8_t *)&ota_header.bms.ota_header_crc, BMS_HEADER_REST_SIZE - CRC_SIZE, t, WITH_BLOCK_CHAINING);
            memcpy(&(ota_header.bms.crc), t, CRC_SIZE);
#endif
            /* Write OTA and BMS headers */
            w_file = fopen(FINAL_IMAGE_NAME, "wb");
            if (!w_file)
            {
                printf("%s failed fopen \n", FINAL_IMAGE_NAME);
                ret = RET_ERROR;
            }
        }
        if (ret == RET_OK)
        {
            fwrite(&ota_header, sizeof(ota_header), 1, w_file);

            /* Build final image */
#ifndef CSS_OTA_ENCRYPTION
            fclose(w_file);
            ret = add_image_with_crc(MULTIIMAGE_NAME, FINAL_IMAGE_NAME, MAIN_IMAGE_TAG);
#else
            if (ret == RET_OK)
            {
                /* OTA and BMS headers are here, now copy from the temporary file we just
                 * created. I don't like all that copy-copy, but let's don;t touch it for now... */
                char buf[1024];
                int n;
                FILE *r_file = fopen(MULTIIMAGE_NAME, "rb");
                if (!r_file)
                {
                    printf("%s failed fopen \n", MULTIIMAGE_NAME);
                    ret = RET_ERROR;
                }
                if (ret == RET_OK)
                {
                    while ((n = fread(buf, 1, sizeof(buf), r_file)) > 0)
                    {
                        fwrite(buf, 1, n, w_file);
                    }
                }
                fclose(w_file);
            }
#endif
        }
    }

    return 0;
}
