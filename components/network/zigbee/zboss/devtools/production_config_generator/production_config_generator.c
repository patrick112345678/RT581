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
/* PURPOSE:
*/
#include <unistd.h>
#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "zb_config.h"
#include "zboss_api.h"
#include "production_config_generator.h"
#include "production_config_interface.h"

#include "../application/samples/se/common/se_common.h"
#include "../ncp/high_level/dev/ncpfw/ncp_prod_config.h"

int verbose_flag = 0;
char *input_filename = "input.txt";
char *output_filename = "output.bin";
int help_flag = 0;

static zb_uint8_t g_binary_buffer[1024];
static int g_config_len = 0;
static int gs_application_type;

/* Function related to parsing */
static void parse_command_line_arguments(int argc, char **argv);
zb_ret_t scan_input_file(char *input_filename);
void write_binary_output(char *output_filename);
static zb_ret_t parse_input_line(char *line_buffer, char **key, char **value, char **section);

/* Production configuration logic */
void set_common_section_default_values(zb_production_config_t *common_section);
int get_application_section_size(int application_type);

/* Function that fillin production configuration */
zb_ret_t set_common_section_parameter(zb_production_config_t *common_section, char *key, char *value);
static zb_ret_t set_ias_zone_section_parameter(void *application_section, char *key, char *value);
static zb_ret_t set_smoke_device_section_parameter(void *application_section, char *key, char *value);

/* Little helper functions */
static void str_rstrip(char *str);
static void hex_string_to_binary(char *in_hex, unsigned char *out_bin, int max_len);
static int restrict_value(int value, int lower, int upper);

zb_ret_t set_cs1_cert_section_parameter(zb_uint8_t **cert_section,
                                               zb_uint8_t *options,
                                               char *key,
                                               char *value);

zb_ret_t set_cs2_cert_section_parameter(zb_uint8_t **cert_section,
                                               zb_uint8_t *options,
                                               char *key,
                                               char *value);

zb_ret_t set_generic_device_app_section_param(void *app_section,
                                              char *key,
                                              char *value);

zb_ret_t set_ncp_app_section_param(void *app_section, char *key, char *value);

zb_uint8_t gs_application_section_added;

/* This is where all application types should be registered */
application_config_type_t application_list[] =
{
  [GENERIC_DEVICE] = {"generic_device", sizeof(se_app_production_config_t),
                      set_generic_device_app_section_param},

#if defined IAS_ZONE_SENSOR_SUPPORT
  [IAS_ZONE_SENSOR] = {"ias_zone_sensor", sizeof(izs_production_config_t), set_ias_zone_section_parameter},
#endif
//  [SMOKE_DEVICE] = {"smoke_device", sizeof(sd_production_config_t), set_smoke_device_section_parameter},
//  [SMART_PLUG] = {"smart_plug", sizeof(sp_production_config_t), set_smart_plug_section_parameter},

  [NETWORK_COPROCESSOR] = {
    "network_coprocessor",
    sizeof(ncp_app_production_config_t),
    set_ncp_app_section_param
  },
};

#ifndef PROD_CONF_LIB
int main(int argc, char **argv)
{
  zb_ret_t ret = RET_OK;

  parse_command_line_arguments(argc, argv);
  if (help_flag)
  {
    printf("Available options:\n"
           "-h, --help   - print help message\n"
           "-i, --input  - for input filename\n"
           "-o, --output - for output filename\n"
           "--verbose    - turn on debug output\n\n"
           "Available device types:\n"
           "\tgeneric_device\n"
#if defined IAS_ZONE_SENSOR_SUPPORT
           "\tias_zone_sensor\n"
#endif
           "\tsmoke_device\n");
    return 0;
  }

  ret = scan_input_file(input_filename);
  if (ret == RET_OK)
  {
    write_binary_output(output_filename);
  }

  return 0;
}
#endif

static void parse_command_line_arguments(int argc, char **argv)
{
  static struct option options[] =
    {
      {"verbose", no_argument, &verbose_flag, 1},
      {"input", required_argument, NULL, 'i'},
      {"output", required_argument, NULL, 'o'},
      {"help", no_argument, &help_flag, 1},
    };

  char c = '\0';

  int option_index = 0;
  while ((c = getopt_long(argc, argv, "i:o:h", options, &option_index)) != -1)
  {
    switch (c)
    {
    case 0:
      break;
    case 'i':
      input_filename = optarg;
      break;
    case 'o':
      output_filename = optarg;
      break;
    case 'h':
      help_flag = 1;
      break;
    case '?':
      break;
    default:
      abort();
    }
  }
}

typedef enum zb_prod_cfg_section_e
{
  ZB_PROD_CFG_SECTION_NONE = 0,
  ZB_PROD_CFG_SECTION_COMMON,
  ZB_PROD_CFG_SECTION_CERT_CS1,
  ZB_PROD_CFG_SECTION_CERT_CS2,
  ZB_PROD_CFG_SECTION_APPLICATION,
} zb_prod_cfg_section_t;

#define ZB_PROD_CFG_COMMON_SECTION_NAME   "common"
#define ZB_PROD_CFG_CERT_CS1_SECTION_NAME "cs1_key_material"
#define ZB_PROD_CFG_CERT_CS2_SECTION_NAME "cs2_key_material"
#define ZB_PROD_CFG_APPLICATION_SECTION_NAME "application"

static void prod_cfg_update_section(char *section, zb_prod_cfg_section_t *current_section)
{
  if (section != NULL && current_section != NULL)
  {
    if (!strcmp(section, ZB_PROD_CFG_COMMON_SECTION_NAME))
    {
      *current_section = ZB_PROD_CFG_SECTION_COMMON;
    }
    else if (!strcmp(section, ZB_PROD_CFG_CERT_CS1_SECTION_NAME))
    {
      *current_section = ZB_PROD_CFG_SECTION_CERT_CS1;
    }
    else if (!strcmp(section, ZB_PROD_CFG_CERT_CS2_SECTION_NAME))
    {
      *current_section = ZB_PROD_CFG_SECTION_CERT_CS2;
    }
    else if (!strcmp(section, ZB_PROD_CFG_APPLICATION_SECTION_NAME))
    {
      *current_section = ZB_PROD_CFG_SECTION_APPLICATION;
    }
    else
    {
      printf("Skipping unknown section name:\t%s", section);
    }
  }
}


zb_ret_t scan_input_file(char *input_filename)
{
  zb_ret_t ret = RET_OK;
  zb_production_config_t *common_section = (zb_production_config_t*)g_binary_buffer;
  zb_prod_cfg_section_t current_section = ZB_PROD_CFG_SECTION_NONE;
  zb_uint8_t *cert_section = (zb_uint8_t *)(common_section + 1);
  zb_uint8_t *application_section;

  static char line_buffer[1024];
  FILE *fin = fopen(input_filename, "r");

  VERBOSE_PRINTF("Using input file %s\n", input_filename);

  if (fin == NULL)
  {
    printf("Could not open input file %s", input_filename);
    goto done;
  }

  /* All set up for work */
  set_common_section_default_values(common_section);
  g_config_len += sizeof(zb_production_config_t);

  /* Start reading parameters */
  while (fgets(line_buffer, sizeof(line_buffer), fin))
  {
    char *key;
    char *value;
    char *section;
    /* Here we have valid key and value - try and update them */
    zb_ret_t ret = RET_NOT_FOUND;

    // First, parse line and check if there's something useful
    {
      zb_ret_t parse_ret = parse_input_line(line_buffer, &key, &value, &section);

      if (parse_ret == RET_EMPTY)
      {
        continue;
      }
      else if (parse_ret == RET_ERROR)
      {
        printf("Skipping bad line:\t%s", line_buffer);
        continue;
      }
      else if (parse_ret == RET_AGAIN)
      {
        prod_cfg_update_section(section, &current_section);
        continue;
      }
    }

    switch(current_section)
    {
      case ZB_PROD_CFG_SECTION_COMMON:
        ret = set_common_section_parameter(common_section, key, value);
        break;

      case ZB_PROD_CFG_SECTION_CERT_CS1:
        ret = set_cs1_cert_section_parameter(&cert_section, &common_section->options, key, value);
        break;

      case ZB_PROD_CFG_SECTION_CERT_CS2:
        ret = set_cs2_cert_section_parameter(&cert_section, &common_section->options, key, value);
        break;

      case ZB_PROD_CFG_SECTION_APPLICATION:
        application_section = (zb_uint8_t *)(common_section) + g_config_len;
        if (application_list[gs_application_type].setter != NULL)
        {
          ret = application_list[gs_application_type].setter(application_section, key, value);
        }
        break;

      default:
        printf("Skipping unknown section %d\n", current_section);
        continue;
    }

    if (ret == RET_NOT_FOUND)
    {
      printf("Skipping unknown key-value pair in section %d: %s = %s\n", current_section, key, value);
    }
    else if (ret == RET_ERROR)
    {
      printf("Skipping malformed key-value pair in section %d: %s = %s\n", current_section, key, value);
    }
  }

  /* Fill out what we know without reading the rest of input file */
  if (gs_application_section_added)
  {
    g_config_len += get_application_section_size(gs_application_type);
  }

  VERBOSE_PRINTF("Config len is %d\n", g_config_len);

  common_section->hdr.len = g_config_len;
  /* Everything is filled, can now calculate crc */
  common_section->hdr.crc = zb_calculate_production_config_crc(g_binary_buffer, g_config_len);

  VERBOSE_PRINTF("Config CRC is 0x%x\n", common_section->hdr.crc);

done:
  fclose(fin);

  return ret;
}

void write_binary_output(char *output_filename)
{
  const unsigned char production_config_header[] = PRODUCTION_CONFIG_HEADER;

  FILE *fout = fopen(output_filename, "wb");

  /* First, write header by which stack will determine if production configuration is present */
  fwrite(production_config_header, sizeof(zb_uint8_t), sizeof(production_config_header), fout);

  /* Now, for the main part */
  fwrite(g_binary_buffer, sizeof(zb_uint8_t), g_config_len, fout);

  fclose(fout);
}

/* Parse line from input file (not including line with application name
 * @return RET_EMPTY - if line is empty or containts comment
 *         RET_OK - if line contains key-value pair.
 *                  In this case line_buffer is modified, key and value point to null-terminated
 *                  strings within line_buffer
 */
static zb_ret_t parse_input_line(char *line_buffer, char **key, char **value, char **section)
{
  char *sep;

  // Allow empty lines
  {
    char *p = line_buffer;
    while (*p != '\0' && isspace(*p))
    {
      p++;
    }
    if (*p == '\0')
    {
      return RET_EMPTY;
    }
  }

  // Allow comments
  if (line_buffer[0] == '#')
  {
    return RET_EMPTY;
  }
  else if (line_buffer[0] == '[')
  {
    sep = strchr(line_buffer, ']');
    if (sep != NULL)
    {
      *sep = '\0';
      *section = line_buffer+1;

      return RET_AGAIN;
    }
    else
    {
      return RET_ERROR;
    }
  }

  if ((sep = strchr(line_buffer, '=')) == NULL)
  {
    return RET_ERROR;
  }

  // Now there are two strings in the buffer
  *sep = '\0';
  *value = sep+1;

  *key = line_buffer;
  // strip all whitespace everywhere
  while (isspace(**key))
  {
    (*key)++;
  }
  str_rstrip(*key);


  while (isspace(**value))
  {
    (*value)++;
  }
  str_rstrip(*value);

  return RET_OK;
}


void set_common_section_default_values(zb_production_config_t *common_section)
{
  /* Version is not utilized at the moment */
  common_section->hdr.version = ZB_PRODUCTION_CONFIG_CURRENT_VERSION;

  common_section->aps_channel_mask_list[0] = ZB_TRANSCEIVER_ALL_CHANNELS_MASK;
  common_section->aps_channel_mask_list[1] = (28 << 27) | (0x7FFFFFF);
  common_section->aps_channel_mask_list[2] = (29 << 27) | (0x1F);
  common_section->aps_channel_mask_list[3] = (30 << 27) | (0x7FFFFFF);
  common_section->aps_channel_mask_list[4] = (31 << 27) | (0x7FFFFFF);

  /* Might not be optimal, but at least not risky and will be well-understood by every platform */
  memset(common_section->mac_tx_power, 0, sizeof(common_section->mac_tx_power));

  {
    /* Better this than nothing, processing all-zeroes extended address by Zigbee devices is weird */
    zb_64bit_addr_t addr[8] = {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}};
    memcpy(common_section->extended_address, addr, sizeof(common_section->extended_address));
  }

  memset(common_section->install_code, 0, sizeof(common_section->install_code));

  common_section->options = 0;
}

zb_ret_t parse_comma_separated_str(char **str, int *value)
{
  zb_ret_t ret = RET_ERROR;
  char *sep;

  sep = strchr(*str, ',');
  if (sep != NULL)
  {
    *sep = '\0';
    *value = strtol(*str, NULL, 10);
    *str = sep + 1;
    ret = RET_OK;
  }

  return ret;
}

/* Set parameters related to stack configuration: MAC address, channel mask etc */
zb_ret_t set_common_section_parameter(zb_production_config_t *common_section, char *key, char *value)
{
  if ( !strcmp(key, "device_type"))
  {
    zb_uint8_t index;

    for (index = 0; index < ZB_ARRAY_SIZE(application_list); index++)
    {
      if (!strcmp(value, application_list[index].name))
      {
        /* Found it! Enough for-looping */
        gs_application_type = index;
        break;
      }
    }

    if (index == ZB_ARRAY_SIZE(application_list))
    {
      printf("Could not find application type, abort");
      abort();
    }
  }
  else if (!strncmp(key, "aps_channel_mask_page", strlen("aps_channel_mask_page")))
  {
    char *channel_page;
    zb_uint8_t page_num;
    zb_uint32_t mask;

    channel_page = key+strlen("aps_channel_mask_page");
    page_num = strtol(channel_page, NULL, 10);
    mask = strtoul(value, NULL, 16);

    VERBOSE_PRINTF("Setting aps channel mask 0x%x to page %d\n", mask, page_num);

    if (page_num == 0)
    {
      common_section->aps_channel_mask_list[0] = (mask & ZB_TRANSCEIVER_ALL_CHANNELS_MASK);
    }
    else if (page_num == 28)
    {
      common_section->aps_channel_mask_list[1] = (28<<27) | (mask & 0x7ffffff);
    }
    else if (page_num == 29)
    {
      common_section->aps_channel_mask_list[2] = (29<<27) | (mask & 0x1f);
    }
    else if (page_num == 30)
    {
      common_section->aps_channel_mask_list[3] = (30<<27) | (mask & 0x7ffffff);
    }
    else if (page_num == 31)
    {
      common_section->aps_channel_mask_list[4] = (31<<27) | (mask & 0x7ffffff);
    }
    else
    {
      printf("Unknown channel page %d\n", page_num);
    }
  }

  else if (!strcmp(key, "extended_address"))
  {
    zb_64bit_addr_t addr;
    zb_ushort_t i;

    hex_string_to_binary(value, addr, 8);
    VERBOSE_PRINTF("Setting extended address to %s\n", value);
    for (i=0; i<sizeof(zb_64bit_addr_t); i++)
    {
      common_section->extended_address[i] = addr[sizeof(zb_64bit_addr_t)-1-i];
    }
    //memcpy(common_section->extended_address, &addr, 8);
  }

  else if (!strcmp(key, "default_mac_tx_power"))
  {
    zb_ushort_t i;
    zb_ushort_t j;
    int tx_power = strtol(value, NULL, 10);

    if (tx_power != ZB_INVALID_TX_POWER_VALUE)
    {
      tx_power = restrict_value(tx_power, TX_POWER_MIN_VALUE_DBM, TX_POWER_MAX_VALUE_DBM);
    }

    VERBOSE_PRINTF("Setting default tx power to %d\n", tx_power);
    for (i = 0; i < ZB_PROD_CFG_APS_CHANNEL_LIST_SIZE; i++)
    {
      for (j = 0; j < ZB_PROD_CFG_MAC_TX_POWER_CHANNEL_N; j++)
      {
        common_section->mac_tx_power[i][j] = tx_power;
      }
    }
  }

  else if (!strncmp(key, "default_mac_tx_power_page", strlen("default_mac_tx_power_page")))
  {
    zb_ushort_t j;
    zb_ushort_t idx;
    char *channel_page;
    zb_uint8_t page_num;
    int tx_power;

    channel_page = key+strlen("default_mac_tx_power_page");
    page_num = strtol(channel_page, NULL, 10);
    tx_power = strtol(value, NULL, 10);

    if (tx_power != ZB_INVALID_TX_POWER_VALUE)
    {
      tx_power = restrict_value(tx_power, TX_POWER_MIN_VALUE_DBM, TX_POWER_MAX_VALUE_DBM);
    }

    idx = (page_num) ? (page_num - 27) : (0);
    VERBOSE_PRINTF("Setting default tx power for channel %d idx %d to %d\n", page_num, idx, tx_power);

    if (idx < 5)
    {
      for (j = 0; j < ZB_PROD_CFG_MAC_TX_POWER_CHANNEL_N; j++)
      {
        common_section->mac_tx_power[idx][j] = tx_power;
      }
    }
    else
    {
      printf("Bad channel page %d\n", page_num);
      return RET_ERROR;
    }
  }

  else if (!strcmp(key, "mac_tx_power"))
  {
    int channel_num;
    int page_num;
    int tx_power;
    zb_uint8_t idx;

    if (RET_OK != parse_comma_separated_str(&value, &page_num))
    {
      printf("Bad mac_tx_power format: %s\n", value);
      return RET_ERROR;
    }

    if (RET_OK != parse_comma_separated_str(&value, &channel_num))
    {
      printf("Bad mac_tx_power format\n");
      return RET_ERROR;
    }

    idx = (page_num) ? (page_num - 27) : (0);
    VERBOSE_PRINTF("Setting mac tx power for channel_page %d idx %d channel_no %d\n", page_num, idx, channel_num);

    tx_power = strtol(value, NULL, 10);

    if (tx_power != ZB_INVALID_TX_POWER_VALUE)
    {
      tx_power = restrict_value(tx_power, TX_POWER_MIN_VALUE_DBM, TX_POWER_MAX_VALUE_DBM);
    }

    VERBOSE_PRINTF("Setting tx power to %d\n", tx_power);

    if (idx < 5)
    {
      if (channel_num >= 0 && channel_num < 27)
      {
        common_section->mac_tx_power[idx][channel_num] = tx_power;
      }
      else
      {
        printf("Bad channel page %d\n", page_num);
        return RET_ERROR;
      }
    }
    else
    {
      printf("Bad channel page %d\n", page_num);
      return RET_ERROR;
    }
  }

/*
 * Currently there is reserved array in production configuration to be able to set
 * tx power for each channel independently
 * For now, single tx power is set across whole channel mask
 */
#if defined OBSOLETE
#ifndef TX_POWER_FOR_SEPARATE_CHANNEL
  else if (!strcmp(key, "mac_tx_power"))
  {
    int tx_power = strtol(value, NULL, 10);

    if (tx_power != ZB_INVALID_TX_POWER_VALUE)
    {
      tx_power = restrict_value(tx_power, TX_POWER_MIN_VALUE_DBM, TX_POWER_MAX_VALUE_DBM);
    }

    VERBOSE_PRINTF("Setting tx power to %d\n", tx_power);
    for (size_t i = 0; i < sizeof(common_section->mac_tx_power); i++)
    {
      common_section->mac_tx_power[i] = tx_power;
    }
  }
#else
  else if (!strncmp(key, "mac_tx_power_", strlen("mac_tx_power_")))
  {
    char *channel_num = key + strlen("mac_tx_power_");
    int channel = strtol(channel_num, NULL, 10);
    int tx_power = strtol(value, NULL, 10);

    if (tx_power != ZB_INVALID_TX_POWER_VALUE)
    {
      tx_power = restrict_value(tx_power, TX_POWER_MIN_VALUE_DBM, TX_POWER_MAX_VALUE_DBM);
    }

    if (channel < MIN_CHANNEL_NUMBER || channel > MAX_CHANNEL_NUMBER)
    {
      printf("Bad channel number %d", channel);
      return RET_ERROR;
    }
    VERBOSE_PRINTF("Setting tx power on channel %d to %d\n", channel, tx_power);

    common_section->mac_tx_power[channel-11] = tx_power;
  }
#endif
#endif  /* OBSOLETE */

  else if (!strcmp(key, "installcode"))
  {
    zb_ic_types_t ic_type;
    zb_uint8_t ic_length;
    zb_uint16_t calculated_crc;
    zb_uint16_t read_crc;
    zb_uint8_t install_code[ZB_CCM_KEY_SIZE+2];

    /* Length in bytes: 1 byte == 2 char symbols; ic length: total_len - 2 bytes CRC */
    ic_length = strlen(value) / 2 - 2;

    switch (ic_length)
    {
      case 6:
        ic_type = ZB_IC_TYPE_48;
        break;

      case 8:
        ic_type = ZB_IC_TYPE_64;
        break;

      case 12:
        ic_type = ZB_IC_TYPE_96;
        break;

      case 16:
        ic_type = ZB_IC_TYPE_128;
        break;

      default:
        printf("Bad installcode(length %d): %s\n", ic_length, value);
        return RET_ERROR;
    }

    hex_string_to_binary(value, install_code, ic_length+2);

    calculated_crc = ~zb_crc16(install_code, 0xffff, ic_length);
    ZB_LETOH16(&read_crc, install_code+ic_length);

    VERBOSE_PRINTF("IC type: 0x%x\n", ic_type);
    VERBOSE_PRINTF("Calculated CRC: 0x%x Read CRC: 0x%x\n", calculated_crc, read_crc);
    if (calculated_crc != read_crc)
    {
      printf("Bad installcode CRC: %s\n", value);
      return RET_ERROR;
    }
    common_section->options |= ic_type;
    memcpy(common_section->install_code, install_code, ic_length+2);
    //ZB_HTOLE16(common_section->install_code + ZB_CCM_KEY_SIZE, &read_crc);

    VERBOSE_PRINTF("Setting installcode to %s\n", value);
  }

  else
  {
    return RET_NOT_FOUND;
  }

  return RET_OK;
}

int get_application_section_size(int application_type)
{
  /* Assumed valid application type */
  return application_list[application_type].app_config_block_size;
}


#if defined IAS_ZONE_SENSOR_SUPPORT
/* Set parameters specific to IAS zone application */
static zb_ret_t set_ias_zone_section_parameter(void *application_section, char *key, char *value)
{
  izs_production_config_t *izs_config = (izs_production_config_t*)application_section;

  if (!strcmp(key, "manufacturer_name"))
  {	
	if (strlen(value) > ZB_ZCL_STRING_CONST_SIZE(izs_config->manuf_name))
	{
		return RET_INVALID_PARAMETER;
	}
	else
	{
	  strncpy(izs_config->manuf_name, value, ZB_ZCL_STRING_CONST_SIZE(izs_config->manuf_name));
      VERBOSE_PRINTF("Setting manufacturer name to %s\n", value);
	}    
  }
  else if (!strcmp(key, "model_id"))
  {
	if (strlen(value) > ZB_ZCL_STRING_CONST_SIZE(izs_config->model_id))
	{
	  return RET_INVALID_PARAMETER;
	}
	else
	{
	  strncpy(izs_config->model_id, value, ZB_ZCL_STRING_CONST_SIZE(izs_config->model_id));
      VERBOSE_PRINTF("Setting model id to %s\n", value);
	}    
  }
  else
  {
	return RET_NOT_FOUND;
  }

  return RET_OK;
}


static zb_ret_t set_smoke_device_section_parameter(void *application_section, char *key, char *value)
{
  sd_production_config_t *sd_config = (sd_production_config_t*)application_section;
  sd_config->version = 0x01;

  if (!strcmp(key, "manufacturer_name"))
  {
	if (strlen(value) > ZB_ZCL_STRING_CONST_SIZE(sd_config->manuf_name))
	{
	  return RET_INVALID_PARAMETER;
	}
	else
	{
	  strncpy(sd_config->manuf_name, value, ZB_ZCL_STRING_CONST_SIZE(sd_config->manuf_name));
      VERBOSE_PRINTF("Setting manufacturer name to %s\n", value);
	}    
  }
  else if (!strcmp(key, "model_id"))
  {
	if (strlen(value) > ZB_ZCL_STRING_CONST_SIZE(sd_config->model_id))
	{
	  return RET_INVALID_PARAMETER;
	}
	else
	{
	  strncpy(sd_config->model_id, value, ZB_ZCL_STRING_CONST_SIZE(sd_config->model_id));
      VERBOSE_PRINTF("Setting model id to %s\n", value);
	}    
  }
  else if (!strcmp(key, "manufacturer_code"))
  {
    int manuf_code = strtoul(value, NULL, 16);
    manuf_code = restrict_value(manuf_code, 0, 0xFFFF);
    sd_config->manuf_code = manuf_code;
    VERBOSE_PRINTF("Setting manufacturer code to %x\n", manuf_code);
  }
  else
  {
    return RET_NOT_FOUND;
  }

  return RET_OK;
}
#endif

/* Set parameters specific to Smart Plug application */
static zb_ret_t set_smart_plug_section_parameter(void *application_section, char *key, char *value)
{
#if 0
  sp_production_config_t *sp_config = (sp_production_config_t*)application_section;
  sp_config->version = 0x02;

  if (!strcmp(key, "manufacturer_name"))
  {
	if (strlen(value) > ZB_ZCL_STRING_CONST_SIZE(sp_config->manuf_name))
	{
	  return RET_INVALID_PARAMETER;
	}
	else
	{
	  strncpy(sp_config->manuf_name, value, ZB_ZCL_STRING_CONST_SIZE(sp_config->manuf_name));
	  VERBOSE_PRINTF("Setting manufacturer name to %s\n", value);
	}
  }
  else if (!strcmp(key, "model_id"))
  {
	if (strlen(value) > ZB_ZCL_STRING_CONST_SIZE(sp_config->manuf_name))
	{
	  return RET_INVALID_PARAMETER;
	}
	else
	{
	  strncpy(sp_config->model_id, value, ZB_ZCL_STRING_CONST_SIZE(sp_config->model_id));
	  VERBOSE_PRINTF("Setting model id to %s\n", value);
	}    
  }
  else if (!strcmp(key, "manufacturer_code"))
  {
    int manuf_code = strtoul(value, NULL, 16);
    manuf_code = restrict_value(manuf_code, 0, 0xFFFF);
    sp_config->manuf_code = manuf_code;
    VERBOSE_PRINTF("Setting manufacturer code to %x\n", manuf_code);
  }
  else if (!strcmp(key, "overcurrent_ma"))
  {
    int overcurrent_ma = strtol(value, NULL, 10);
    overcurrent_ma = restrict_value(overcurrent_ma, 0, 0xFFFF);
    sp_config->overcurrent_ma = overcurrent_ma;
    VERBOSE_PRINTF("Setting overcurrent_ma to %d\n", overcurrent_ma);
  }
  else if (!strcmp(key, "overvoltage_dv"))
  {
    int overvoltage_dv = strtol(value, NULL, 10);
    overvoltage_dv = restrict_value(overvoltage_dv, 0, 0xFFFF);
    sp_config->overvoltage_dv = overvoltage_dv;
    VERBOSE_PRINTF("Setting overvoltage_dv to %d\n", overvoltage_dv);
  }
  else
  {
    return RET_NOT_FOUND;
  }
#endif
  return RET_OK;
}


/* Some little helper functions */

/* Remove whitespace from the end of the string */
static void str_rstrip(char *str)
{
  char *end = strchr(str, '\0');
  if (end == NULL)
  {
    return;
  }

  while (isspace(*(--end))) ;

  *(++end) = '\0';
}

/* Read hexadecimal string (no whitespaces) and convert to byte array */
static void hex_string_to_binary(char *in_hex, unsigned char *out_bin, int max_len)
{
  char *pos = in_hex;

  for(int i = 0; i < max_len; i++) {
    sscanf(pos, "%2hx", &out_bin[i]);
    pos += 2;
  }
}

static int restrict_value(int value, int lower, int upper)
{
  if (value > upper)
  {
    return upper;
  }
  else if (value < lower)
  {
    return lower;
  }
  return value;
}

/* Cripto Suite functions */
zb_ret_t set_cs1_cert_section_parameter(zb_uint8_t **cert_section,
                                               zb_uint8_t *options,
                                               char *key,
                                               char *value)
{
  static zb_uint8_t is_first = 1;
  zb_cs1_key_material_t *cs1_key_material = (zb_cs1_key_material_t *)(*cert_section + sizeof(zb_cs_key_material_header_t));

  if (is_first)
  {
    zb_cs_key_material_header_t *cs_hdr;
    is_first = 0;
    memset(cs1_key_material, 0, sizeof(zb_cs1_key_material_t));

    /* Mark prod_cfg contains certificates block */
    *options |= 0x80;

    cs_hdr = (zb_cs_key_material_header_t *)(*cert_section);
    cs_hdr->certificate_mask = 1<<0;

    g_config_len += sizeof(zb_cs_key_material_header_t) + sizeof(zb_cs1_key_material_t);
  }

  if (!strcmp(key, "publisher_public_key"))
  {
    zb_uint8_t ppk[ZB_CS1_PUBLISHER_PUBLIC_KEY_SIZE];
    size_t len;

    len = strlen(value)/2;
    if (len != ZB_CS1_PUBLISHER_PUBLIC_KEY_SIZE)
    {
      printf("Bad CS1 Publisher Public Key size\n");
      return RET_ERROR;
    }

    memset(ppk, 0, ZB_CS1_PUBLISHER_PUBLIC_KEY_SIZE);
    VERBOSE_PRINTF("Setting CS1 publisher public key to %s\n", value);
    hex_string_to_binary(value, ppk, ZB_CS1_PUBLISHER_PUBLIC_KEY_SIZE);

    memcpy(cs1_key_material->publisher_public_key, ppk, ZB_CS1_PUBLISHER_PUBLIC_KEY_SIZE);
  }
  else if (!strcmp(key, "certificate"))
  {
    zb_uint8_t cert[ZB_CS1_CERTIFICATE_SIZE];
    size_t len;

    len = strlen(value)/2;
    if (len != ZB_CS1_CERTIFICATE_SIZE)
    {
      printf("Bad CS1 Certificate size\n");
      return RET_ERROR;
    }

    memset(cert, 0, ZB_CS1_CERTIFICATE_SIZE);
    VERBOSE_PRINTF("Setting CS1 certificate to %s\n", value);
    hex_string_to_binary(value, cert, ZB_CS1_CERTIFICATE_SIZE);

    memcpy(cs1_key_material->certificate, cert, ZB_CS1_CERTIFICATE_SIZE);
  }
  else if (!strcmp(key, "private_key"))
  {
    zb_uint8_t pk[ZB_CS1_PRIVATE_KEY_SIZE];
    size_t len;

    len = strlen(value)/2;
    if (len != ZB_CS1_PRIVATE_KEY_SIZE)
    {
      printf("Bad CS1 Private Key size\n");
      return RET_ERROR;
    }

    memset(pk, 0, ZB_CS1_PRIVATE_KEY_SIZE);
    VERBOSE_PRINTF("Setting CS1 private key to %s\n", value);
    hex_string_to_binary(value, pk, ZB_CS1_PRIVATE_KEY_SIZE);

    memcpy(cs1_key_material->private_key, pk, ZB_CS1_PRIVATE_KEY_SIZE);
  }
  else
  {
    return RET_NOT_FOUND;
  }

  return RET_OK;
}


zb_ret_t set_cs2_cert_section_parameter(zb_uint8_t **cert_section,
                                               zb_uint8_t *options,
                                               char *key,
                                               char *value)
{
  static zb_uint8_t is_first = 1;
  zb_cs2_key_material_t *cs2_key_material = (zb_cs2_key_material_t *)(*cert_section);

  if (is_first)
  {
    zb_cs_key_material_header_t *cs_hdr;

    is_first = 0;

    cs_hdr = (zb_cs_key_material_header_t *)(*cert_section);
    cs_hdr->certificate_mask |= 1<<1;

    g_config_len += sizeof(zb_cs2_key_material_t);

    if (cs_hdr->certificate_mask & 0x01)
    {
      *cert_section += sizeof(zb_cs1_key_material_t) + sizeof(zb_cs_key_material_header_t);
      cs2_key_material = (zb_cs2_key_material_t *)(*cert_section);
    }
    else
    {
      g_config_len += sizeof(zb_cs_key_material_header_t);
      *cert_section += sizeof(zb_cs_key_material_header_t);
      cs2_key_material = (zb_cs2_key_material_t *)(*cert_section);
    }

    memset(cs2_key_material, 0, sizeof(zb_cs2_key_material_t));

    /* Mark prod_cfg contains certificates block */
    *options |= 0x80;
  }

  if (!strcmp(key, "publisher_public_key"))
  {
    zb_uint8_t ppk[ZB_CS2_PUBLISHER_PUBLIC_KEY_SIZE];
    size_t len;

    len = strlen(value)/2;
    if (len != ZB_CS2_PUBLISHER_PUBLIC_KEY_SIZE)
    {
      printf("Bad CS2 Pulisher Public Key size\n");
      return RET_ERROR;
    }

    memset(ppk, 0, ZB_CS2_PUBLISHER_PUBLIC_KEY_SIZE);
    VERBOSE_PRINTF("Setting CS2 publisher public key to %s\n", value);
    hex_string_to_binary(value, ppk, ZB_CS2_PUBLISHER_PUBLIC_KEY_SIZE);

    memcpy(cs2_key_material->publisher_public_key, ppk, ZB_CS2_PUBLISHER_PUBLIC_KEY_SIZE);
  }
  else if (!strcmp(key, "certificate"))
  {
    zb_uint8_t cert[ZB_CS2_CERTIFICATE_SIZE];
    size_t len;

    len = strlen(value)/2;
    if (len != ZB_CS2_CERTIFICATE_SIZE)
    {
      printf("Bad CS2 Certificate size\n");
      return RET_ERROR;
    }

    memset(cert, 0, ZB_CS2_CERTIFICATE_SIZE);
    VERBOSE_PRINTF("Setting CS2 certificate to %s\n", value);
    hex_string_to_binary(value, cert, ZB_CS2_CERTIFICATE_SIZE);

    memcpy(cs2_key_material->certificate, cert, ZB_CS2_CERTIFICATE_SIZE);
  }
  else if (!strcmp(key, "private_key"))
  {
    zb_uint8_t pk[ZB_CS2_PRIVATE_KEY_SIZE];
    size_t len;

    len = strlen(value)/2;
    if (len != ZB_CS2_PRIVATE_KEY_SIZE)
    {
      printf("Bad CS2 Private Key size\n");
      return RET_ERROR;
    }

    memset(pk, 0, ZB_CS2_PRIVATE_KEY_SIZE);
    VERBOSE_PRINTF("Setting CS2 private key to %s\n", value);
    hex_string_to_binary(value, pk, ZB_CS2_PRIVATE_KEY_SIZE);

    memcpy(cs2_key_material->private_key, pk, ZB_CS2_PRIVATE_KEY_SIZE);
  }
  else
  {
    return RET_NOT_FOUND;
  }

  return RET_OK;
}


zb_ret_t set_generic_device_app_section_param(void *app_section,
                                              char *key,
                                              char *value)
{
  se_app_production_config_t *se_app_config = (se_app_production_config_t *)app_section;

  se_app_config->version = SE_APP_PROD_CFG_CURRENT_VERSION;

  if (!strcmp(key, "manufacturer_name"))
  {
	if (strlen(value) > ZB_ZCL_STRING_CONST_SIZE(se_app_config->manuf_name))
	{
	  return RET_INVALID_PARAMETER;
	}
	else
	{
	  strncpy(se_app_config->manuf_name, value, ZB_ZCL_STRING_CONST_SIZE(se_app_config->manuf_name));
	  VERBOSE_PRINTF("Setting manufacturer name to %s\n", value);
	  gs_application_section_added = 1;
	}    
  }
  else if (!strcmp(key, "model_id"))
  {
	if (strlen(value) > ZB_ZCL_STRING_CONST_SIZE(se_app_config->model_id))
	{
	  return RET_INVALID_PARAMETER;
	}
	else
	{
	  strncpy(se_app_config->model_id, value, ZB_ZCL_STRING_CONST_SIZE(se_app_config->model_id));
	  VERBOSE_PRINTF("Setting manufacturer name to %s\n", value);
	  gs_application_section_added = 1;
	}
  }
  else if (!strcmp(key, "manufacturer_code"))
  {
    int manuf_code = strtoul(value, NULL, 16);
    manuf_code = restrict_value(manuf_code, 0, 0xFFFF);
    se_app_config->manuf_code = manuf_code;
    VERBOSE_PRINTF("Setting manufacturer code to: 0x%X\n", manuf_code);
    gs_application_section_added = 1;
  }
  else
  {
    return RET_NOT_FOUND;
  }

  return RET_OK;
}

static inline zb_bool_t decode_hex_digit(char chr, zb_uint8_t *val)
{
  zb_bool_t result = ZB_TRUE;

  if (chr >= '0' && chr <= '9')
  {
    *val = chr - '0';
  }
  else if (chr >= 'a' && chr <= 'f')
  {
    *val = 10 + (chr - 'a');
  }
  else if (chr >= 'A' && chr <= 'F')
  {
    *val = 10 + (chr - 'A');
  }
  else
  {
    result = ZB_FALSE;
  }

  return result;
}

static ssize_t decode_hex_string(const char *str, zb_uint8_t *data, size_t size)
{
  size_t pos = 0;
  zb_uint8_t lo, hi;

  while (*str && pos < size)
  {
    if (decode_hex_digit(str[0], &hi) &&
        decode_hex_digit(str[1], &lo))
    {
      data[pos] = (hi << 4) | lo;
      pos += 1;
      str += 2;
    }
    else
    {
      memset(data, 0, size);
      return -1;
    }
  }

  return pos;
}

zb_ret_t set_ncp_app_section_param(void *app_section, char *key, char *value)
{
  ncp_app_production_config_t *ncp_app_config = (ncp_app_production_config_t *)app_section;
  zb_ret_t ret = RET_NOT_FOUND;

  if (strcmp(key, "serial_number") == 0)
  {
    strncpy(ncp_app_config->serial_number, value,
        sizeof(ncp_app_config->serial_number));

    VERBOSE_PRINTF("Setting NCP serial number to: %s\n", value);

    ret = RET_OK;
  }
  else if (strcmp(key, "vendor_data") == 0)
  {
    ssize_t res = decode_hex_string(value, ncp_app_config->vendor_data,
        sizeof(ncp_app_config->vendor_data));
    if (res >= 0)
    {
      ncp_app_config->vendor_data_size = (zb_uint8_t)res;

      VERBOSE_PRINTF("Setting NCP vendor data to: %s\n", value);

      ret = RET_OK;
    }
    else
    {
      ret = RET_ERROR;
    }

  }

  gs_application_section_added |= (ret == RET_OK);

  return ret;
}

#ifdef PROD_CONF_LIB
zb_ret_t prepare_generic_binary_output(prod_cfg_param_t *prod_cfg_param)
{
  zb_ret_t ret = RET_OK;
  const unsigned char production_config_header[] = PRODUCTION_CONFIG_HEADER;
  zb_production_config_t *common_section = (zb_production_config_t*)(prod_cfg_param->g_binary_buffer+sizeof(production_config_header));
  uint8_t *cert_section = (uint8_t *)(common_section + 1);
  uint8_t *application_section;

  /* All set up for work */
  set_common_section_default_values(common_section);
  g_config_len += sizeof(zb_production_config_t);
  do
  {
    /* [common] */
    SET_AND_CHECK(set_common_section_parameter(common_section, "device_type", "generic_device"));
    SET_AND_CHECK(set_common_section_parameter(common_section, "extended_address", prod_cfg_param->ieee));
    SET_AND_CHECK(set_common_section_parameter(common_section, "aps_channel_mask_page0", prod_cfg_param->mask));
    SET_AND_CHECK(set_common_section_parameter(common_section, "aps_channel_mask_page28", "0f000"));
    SET_AND_CHECK(set_common_section_parameter(common_section, "aps_channel_mask_page29", "000010"));
    SET_AND_CHECK(set_common_section_parameter(common_section, "aps_channel_mask_page30", "7ffffff"));
    SET_AND_CHECK(set_common_section_parameter(common_section, "aps_channel_mask_page31", "7ffffff"));
    SET_AND_CHECK(set_common_section_parameter(common_section, "default_mac_tx_power", "-37"));
    SET_AND_CHECK(set_common_section_parameter(common_section, "default_mac_tx_power_page0", prod_cfg_param->power));
    SET_AND_CHECK(set_common_section_parameter(common_section, "installcode", "966b9f3ef98ae6059708"));
    /* [cs1_key_material] */
    SET_AND_CHECK(set_cs1_cert_section_parameter(&cert_section, &common_section->options, "publisher_public_key", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
    SET_AND_CHECK(set_cs1_cert_section_parameter(&cert_section, &common_section->options, "certificate", "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"));
    SET_AND_CHECK(set_cs1_cert_section_parameter(&cert_section, &common_section->options, "private_key", "cccccccccccccccccccccccccccccccccccccccccc"));
    /* [cs2_key_material] */
    SET_AND_CHECK(set_cs2_cert_section_parameter(&cert_section, &common_section->options, "publisher_public_key", "dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"));
    SET_AND_CHECK(set_cs2_cert_section_parameter(&cert_section, &common_section->options, "certificate", "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"));
    SET_AND_CHECK(set_cs2_cert_section_parameter(&cert_section, &common_section->options, "private_key", "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
    /* [application] */
    if (application_list[gs_application_type].setter != NULL)
    {
      application_section = (uint8_t *)(common_section) + g_config_len;
      SET_AND_CHECK(application_list[gs_application_type].setter(application_section, "manufacturer_name", "The_Manufacturer"));
      SET_AND_CHECK(application_list[gs_application_type].setter(application_section, "model_id", "Pretty_ID"));
      SET_AND_CHECK(application_list[gs_application_type].setter(application_section, "manufacturer_code", "0x1234"));
    }
  }
  while (0);
  /* Fill out what we know without reading the rest of input file */
  if (gs_application_section_added)
  {
    g_config_len += get_application_section_size(gs_application_type);
  }

  common_section->hdr.len = g_config_len;
  /* Everything is filled, can now calculate crc */
  common_section->hdr.crc = zb_calculate_production_config_crc((zb_uint8_t *)common_section, g_config_len);

  printf("Config CRC is 0x%x\n", common_section->hdr.crc);

  /* Copy the header to the beginning */
  ZB_MEMCPY(prod_cfg_param->g_binary_buffer, production_config_header, sizeof(production_config_header));
  g_config_len += sizeof(production_config_header);
  prod_cfg_param->g_config_len = g_config_len;

  printf("Config len is %d\n", prod_cfg_param->g_config_len);

  return ret;
}

static void binary_to_hex_string(unsigned char *in_bin, char *out_hex)
{
/* pointer to the first item (0 index) of the output array */
  char *ptr = &out_hex[0];
  int   i;

  for (i = sizeof(zb_64bit_addr_t) - 1; i >= 0; i--)
  {
    /* sprintf converts each byte to 2 chars hex string and a null byte, for example
     * 10 => "0A\0".
     *
     * These three chars would be added to the output array starting from
     * the ptr location, for example if ptr is pointing at 0 index then the hex chars
     * "0A\0" would be written as output[0] = '0', output[1] = 'A' and output[2] = '\0'.
     *
     * sprintf returns the number of chars written execluding the null byte, in our case
     * this would be 2. Then we move the ptr location two steps ahead so that the next
     * hex char would be written just after this one and overriding this one's null byte.
     *
     * We don't need to add a terminating null byte because it's already added from
     * the last hex string. */
    if (i)
    {
      ptr += sprintf(ptr, "%02x:", in_bin[i]);
    }
    else
    {
      ptr += sprintf(ptr, "%02x", in_bin[i]);
    }
  }
}

zb_ret_t parse_generic_binary_input(prod_cfg_param_t *prod_cfg_param)
{
  const unsigned char production_config_header[] = PRODUCTION_CONFIG_HEADER;
  zb_production_config_t *common_section = (zb_production_config_t*)(prod_cfg_param->g_binary_buffer+sizeof(production_config_header));

  if (prod_cfg_param->g_config_len < (sizeof(production_config_header) + sizeof(zb_production_config_t)))
  {
    return RET_ERROR;
  }

  binary_to_hex_string(common_section->extended_address, prod_cfg_param->ieee);
  sprintf(prod_cfg_param->power, "%d", common_section->mac_tx_power[0][0]);
  sprintf(prod_cfg_param->mask, "%08" PRIx32, common_section->aps_channel_mask_list[0]);
  return RET_OK;
}
#endif
