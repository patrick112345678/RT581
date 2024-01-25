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
/* PURPOSE: Header file for led bulb sample application for HA
*/

#ifndef BULB_H
#define BULB_H 1

#include "zboss_api.h"

/* Bulb IEEE address */
#define BULB_IEEE_ADDRESS {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
/* major app version */
#define BULB_FIRMWARE_MAJOR       1
/* Application manufacturer code */
#define BULB_FIRMWARE_MANUF_CODE  2
/* BULB app revision (minor ver) */
#define BULB_FIRMWARE_MINOR       0
/* Default channel */
#define BULB_DEFAULT_APS_CHANNEL_MASK (1l<<21)
/* Used endpoint number */
#define HA_DIMMABLE_LIGHT_ENDPOINT 10

/* Handler for specific ZCL commands */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);

/* Light Bulb initialization */
void bulb_device_app_init(zb_uint8_t param);
/* Initialization of global device context */
void bulb_app_ctx_init();
/* Initialization of HA attributes */
void bulb_clusters_attr_init(zb_uint8_t param);
/* Device user application callback */
void test_device_cb(zb_uint8_t param);
/* Set default value all attribute" callback */
void zcl_reset_to_defaults_cb(zb_uint8_t param);

/* Update current level value in the global context */
zb_ret_t level_control_set_level(zb_uint8_t new_level);
/* Start identifying for hardware */
void bulb_do_identify(zb_uint8_t param);

/* [app_nvram_cb_proto] */
/* Application callback for writing application data to NVRAM */
zb_ret_t bulb_nvram_write_app_data(zb_uint8_t page, zb_uint32_t pos);
/* Application callback for reading application data from NVRAM */
void bulb_nvram_read_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length);
/* Application callback to determine application NVRAM stored dataset size */
zb_uint16_t bulb_get_nvram_data_size();
/* [app_nvram_cb_proto] */

/* attributes of Basic cluster */
typedef struct bulb_device_basic_attr_s
{
    zb_uint8_t zcl_version;
    zb_uint8_t app_version;
    zb_uint8_t stack_version;
    zb_uint8_t hw_version;
    zb_char_t mf_name[32];
    zb_char_t model_id[32];
    zb_char_t date_code[16];
    zb_uint8_t power_source;
    zb_char_t location_id[17];
    zb_uint8_t ph_env;
    zb_char_t sw_build_id[3];
}
bulb_device_basic_attr_t;

/* Basic cluster attributes data */
#define BULB_INIT_BASIC_STACK_VERSION     BULB_FIRMWARE_MAJOR           /* define major app version */
#define BULB_INIT_BASIC_HW_VERSION        BULB_FIRMWARE_MANUF_CODE      /* define manufacturer code */
#define BULB_INIT_BASIC_APP_VERSION       BULB_FIRMWARE_MINOR           /* define f/w revision */

#define BULB_INIT_BASIC_MANUF_NAME    "DSR"

/* Note: instead of the 1st space string len will be set */
#define BULB_INIT_BASIC_MODEL_ID     "Dim_Light"
#define BULB_INIT_BASIC_DATE_CODE    "2016-05-25"
#define BULB_INIT_BASIC_LOCATION_ID  "CH"
#define BULB_INIT_BASIC_PH_ENV                            0

/* attributes of Identify cluster */
typedef struct bulb_device_identify_attr_s
{
    zb_uint16_t identify_time;
}
bulb_device_identify_attr_t;

/* attributes of ON/Off cluster */
typedef struct bulb_device_on_off_attr_s
{
    zb_bool_t on_off;
}
bulb_device_on_off_attr_t;

/* attributes of Level Control cluster */
typedef struct bulb_device_level_control_attr_s
{
    zb_uint8_t current_level;
    zb_uint16_t remaining_time;
}
bulb_device_level_control_attr_t;

/* Global device context */
typedef struct bulb_device_ctx_s
{
    bulb_device_basic_attr_t basic_attr;
    bulb_device_identify_attr_t identify_attr;
    bulb_device_on_off_attr_t on_off_attr;
    bulb_device_level_control_attr_t level_control_attr;
}
bulb_device_ctx_t;

/* [app_nvram_define_dataset] */
/* Application dataset for persisting into nvram */
typedef ZB_PACKED_PRE struct bulb_device_nvram_dataset_s
{
    zb_uint8_t onoff_state;
    zb_uint8_t current_level;
    /* Size of the structure in bytes must be divisible by 4 */
    zb_uint8_t reserved[2];
} ZB_PACKED_STRUCT
bulb_device_nvram_dataset_t;
/* [app_nvram_define_dataset] */

#endif /* BULB_H */
