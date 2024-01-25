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
/* PURPOSE: Utility for single-imege OTA file creation for CSS v2
*/

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>

#include "types.h"
#include "AES128.h"
#include "AESMMO.h"


#define ZB_ZCL_OTA_UPGRADE_FILE_HEADER_FILE_ID          0x0BEEF11E
#define ZB_ZCL_OTA_UPGRADE_FILE_HEADER_FILE_VERSION     0x0100
#define ZB_ZCL_OTA_UPGRADE_FILE_HEADER_LENGTH           0x0038 /* sizeof(zb_zcl_ota_upgrade_file_header_t) */
#define ZB_ZCL_OTA_UPGRADE_FILE_HEADER_FC               0x0010 /* sizeof(zb_zcl_ota_upgrade_file_header_t) */
#define CSS_INIT_OTA_DEFAULT_IMAGE_TYPE				    0x0141
#define ZB_ZCL_OTA_UPGRADE_FILE_HEADER_STACK_PRO        0x0002
#define OTA_UPGRADE_TEST_IMAGE_SIZE                     0xFFFFFFFF

/* hash */
#define OTA_UPGRADE_TEST_FW_TAG_ID                      0x0000
#define OTA_UPGRADE_TEST_HASH_TAG_ID                    0x0003

#define OTA_UPGRADE_TEST_FW_LENGTH                      0x00000000
#define OTA_UPGRADE_TEST_HASH_LENGTH                    0x00000010

/* CR: 06/22/2015 [DT] */
#define USAGE_STRING        "Usage: {src_file} {dest_file} {file_version} {manufacturer_code} [{image_type}]"

/* Size of the buffer used to store and process program parameters */
#define ARG_BUF_SIZE                                    10

#pragma pack(1) 
typedef struct zb_zcl_ota_upgrade_file_header_s
{
  uint32 file_id;            /**< OTA upgrade file identifier*/
  uint16 header_version;     /**< OTA Header version */
  uint16 header_length;      /**< OTA Header length */
  uint16 fc;                 /**< OTA Header Field control */
  uint16 manufacturer_code;  /**< Manufacturer code */
  uint16 image_type;         /**< Image type */
  uint32 file_version;       /**< File version */
  uint16 stack_version;      /**< Zigbee Stack version */
  char   header_string[32];    /**< OTA Header string */
  uint32 total_image_size;   /**< Total Image size (including header) */

} zb_zcl_ota_upgrade_file_header_t;

#define ALIGNED_FW_LEN(len, al) \
    ((len % al == 0) ? al : 0)

#pragma pack(1) 

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
 *  @brief parse 4-char string to uint16
 *
 *  @param str string with number ('0x' omitted)
 */
uint16 hex_str_to_uint16(char *str)
{
  uint16 res = char_to_hex(str[3]);
  res += char_to_hex(str[2]) << 4;
  res += char_to_hex(str[1]) << 8;
  res += char_to_hex(str[0]) << 12;
  return res;
}
/* CR: 06/22/2015 [DT] End */


void print_hash(uint8 *hash)
{
  int i;

  printf("\nAES MMO hash:\n");
  for (i = 0; i < 16; ++i)
  {
    printf(" %x", hash[i]);
  }
}

int main(int argc, char* argv[])
{
  uint8 hash16[16] = {0}; // hash without blockchaining
  uint32 r_offset = 0;
  uint32 r_len = 0;
  FILE *r_file, *w_file;
  char buf[ARG_BUF_SIZE];
  uint8 *processed_image; // OTA header + processed image;

  uint16 fw_tag_id = OTA_UPGRADE_TEST_FW_TAG_ID;
  uint16 hash_tag_id = OTA_UPGRADE_TEST_HASH_TAG_ID;
  uint32 hash_container_length = OTA_UPGRADE_TEST_HASH_LENGTH; /* in q 2nd step we include the "last_block_hash" t AND the "full_hash" tf */

/* CR: 06/22/2015 [DT] Start */  
  zb_zcl_ota_upgrade_file_header_t ota_file = 
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
  };

  memset(buf, 0, sizeof(buf));

  if (argc < 5 || argc > 6)
  {
    printf("Wrong number of arguments supplied.\n" USAGE_STRING);
    return 1;
  }

  r_file = fopen(argv[1], "rb");
  if (!r_file)
  {
    printf("please specify input FW image\n");
    return 1;
  }

  /* 
   * Expected argv contents:
   * 1 -src file
   * 2 -dest file
   * 3 - file_version,
   * 4 - manufacturer code,
   * 5 - (optional) image type (default value CSS_INIT_OTA_DEFAULT_IMAGE_TYPE)
   */
  {
	  int ver[4];
	  if (sscanf(argv[3], "v%02x.%02x.%02x.%02x", &ver[0], &ver[1], &ver[2], &ver[3]) != 4)
	  {
		  printf("incorrect version format : use format vAA.BB.CC.DD i.s.o. %s\n", argv[3]);
    return 1;
  }
	  else
	  {
		  ota_file.file_version = (ver[0] << 24) | (ver[1] << 16) | (ver[2] << 8) | ver[3];
	  }
  }

  /* parse manufacturer code */
  memset(buf, 0, sizeof(buf));
  strncpy(buf, argv[4], sizeof(buf)-1);
  if (strlen(buf) != 6 || buf[0] != '0' || (buf[1] != 'x' && buf[1] != 'X'))
  {
    buf[ARG_BUF_SIZE-1] = '\0';
    printf("Invalid manufacturer code : use format 0x01AB instead of %s\n", buf);
    return 1;
  }
  ota_file.manufacturer_code = hex_str_to_uint16(buf + 2);

  /* check for image type */
  if (argc == 6)
  {
    memset(buf, 0, sizeof(buf));
    strncpy(buf, argv[5], sizeof(buf)-1);
    if (strlen(buf) != 6 || buf[0] != '0' || (buf[1] != 'x' && buf[1] != 'X'))
    {
      buf[ARG_BUF_SIZE-1] = '\0';
      printf("Invalid image type : use format 0x01AB instead of %s\n", buf);
      return 1;
    }
    ota_file.image_type = hex_str_to_uint16(buf + 2);
  }

  printf("setting version nbr = 0x%08X\n", ota_file.file_version);
  printf("setting manufacturer code = 0x%04X\n", ota_file.manufacturer_code);
  printf("setting image_type = 0x%04X\n", ota_file.image_type);
/* CR: 06/22/2015 [DT] End */
  fseek(r_file, 0, SEEK_END);
  r_offset = ftell(r_file);
  fseek(r_file, 0, SEEK_SET);


  ota_file.total_image_size = sizeof(ota_file);
  ota_file.total_image_size += 6; /* 2 bytes tag, 4 bytes length */
  ota_file.total_image_size += r_offset; /* actual fw size */
  ota_file.total_image_size += 22; /* 2bytes tag, 4bytes length, 16bytes "full_hash"*/

  processed_image = (uint8*) malloc((size_t) ota_file.total_image_size);
  if (processed_image == NULL)
  {
    printf("Not enough memory for storing image into host RAM, required size is %u\n", ota_file.total_image_size);
    fclose(r_file);
    return 1;
  }
  else
  {
    uint8 *ptr;
    uint32 encr_block_length;

    ptr = processed_image;
    memcpy(ptr, &ota_file, sizeof(ota_file));
    ptr += sizeof(ota_file);

    memcpy(ptr, &fw_tag_id, sizeof(fw_tag_id));
    ptr += sizeof(fw_tag_id);

    memcpy(ptr, &r_offset, sizeof(r_offset));
    ptr += sizeof(r_offset);

    fread(ptr, r_offset, 1, r_file);
    ptr += r_offset;

    encr_block_length = ota_file.total_image_size - 22;
    AES_MMO(processed_image, encr_block_length*8, hash16);

    memcpy(ptr, &hash_tag_id, sizeof(hash_tag_id));
    ptr += sizeof(hash_tag_id);

    memcpy(ptr, &hash_container_length, sizeof(hash_container_length));
    ptr += sizeof(hash_container_length);

    memcpy(ptr, hash16, 16);

    fclose(r_file);

    w_file = fopen(argv[2], "wb");
    fwrite(processed_image, ota_file.total_image_size, 1, w_file);
    fclose(w_file);
    free((void*)processed_image);

    printf("\nImage is written to file %s\n", argv[2]);
    print_hash(hash16);
  }

  return 0;
}












