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
/* PURPOSE: Test sample with clusters Illuminance Measurement,
  Tempereture Measurement, Water Content Measurement,
  Occupancy Sensing, Electrical Measurement */

#define ZB_TRACE_FILE_ID 51371
#include "zboss_api.h"
#include "measurement_sensing_zr.h"

/* Insert that include before any code or declaration. */
#ifdef ZB_CONFIGURABLE_MEM
#include "zb_mem_config_max.h"
#endif

zb_uint16_t g_dst_addr = 0x00;

#define DST_ADDR g_dst_addr
#define DST_ADDR_MODE ZB_APS_ADDR_MODE_16_ENDP_PRESENT
#define DST_EP 5

/**
 * Global variables definitions
 */
zb_ieee_addr_t g_zc_addr = {0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb}; /* IEEE address of the
                                                                              * device */
/* Used endpoint */
#define ZB_SERVER_ENDPOINT          5
#define ZB_CLIENT_ENDPOINT          6

void test_loop(zb_bufid_t param);

/**
 * Declaring attributes for each cluster
 */

/* Illuminance Measurement cluster attributes: */
zb_uint16_t g_attr_ill_measured_value = ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MEASURED_VALUE_DEFAULT;
zb_uint16_t g_attr_ill_min_value = ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MIN_MEASURED_VALUE_MIN_VALUE; /* Missing default value */
zb_uint16_t g_attr_ill_max_value = ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MIN_MEASURED_VALUE_MAX_VALUE; /* Missing default value */
ZB_ZCL_DECLARE_ILLUMINANCE_MEASUREMENT_ATTRIB_LIST(illuminance_measurement_attr_list, &g_attr_ill_measured_value,
                                                  &g_attr_ill_min_value, &g_attr_ill_max_value);

/* Temperature Measurement cluster attributes: */
zb_uint16_t g_attr_temp_measured_value = ZB_ZCL_TEMP_MEASUREMENT_VALUE_DEFAULT_VALUE;
zb_uint16_t g_attr_temp_min_value = ZB_ZCL_TEMP_MEASUREMENT_MIN_VALUE_DEFAULT_VALUE;
zb_uint16_t g_attr_temp_max_value = ZB_ZCL_TEMP_MEASUREMENT_MAX_VALUE_DEFAULT_VALUE;
zb_uint16_t g_attr_temp_tolerance = 0; /* Missing default value */
ZB_ZCL_DECLARE_TEMP_MEASUREMENT_ATTRIB_LIST(temperature_measurement_attr_list, &g_attr_temp_measured_value,
                                            &g_attr_temp_min_value, &g_attr_temp_max_value, &g_attr_temp_tolerance);

/* Water Content Measurement cluster attributes */
zb_uint16_t g_attr_value = ZB_ZCL_REL_HUMIDITY_MEASUREMENT_VALUE_DEFAULT_VALUE;
zb_uint16_t g_attr_min_value = ZB_ZCL_REL_HUMIDITY_MEASUREMENT_MIN_VALUE_DEFAULT_VALUE;
zb_uint16_t g_attr_max_value = ZB_ZCL_REL_HUMIDITY_MEASUREMENT_MAX_VALUE_DEFAULT_VALUE;
ZB_ZCL_DECLARE_REL_HUMIDITY_MEASUREMENT_ATTRIB_LIST(water_measurement_attr_list, &g_attr_value,
                                                  &g_attr_min_value, &g_attr_max_value);

/* Occupancy Sensing cluster attributes */
zb_uint8_t g_attr_occupancy = 0;
zb_uint8_t g_attr_occupancy_sensor_type = 0;
zb_uint8_t g_attr_occupancy_sensor_type_bitmap = 0;
ZB_ZCL_DECLARE_OCCUPANCY_SENSING_ATTRIB_LIST(occupancy_sensing_attr_list, &g_attr_occupancy,
                                            &g_attr_occupancy_sensor_type, &g_attr_occupancy_sensor_type_bitmap);

/* Electrical Measurement cluster attributes */
zb_uint32_t g_attr_measurement_type = ZB_ZCL_ELECTRICAL_MEASUREMENT_MEASUREMENT_TYPE_DEFAULT_VALUE;
zb_uint16_t g_attr_dcpower = ZB_ZCL_ELECTRICAL_MEASUREMENT_DCPOWER_DEFAULT_VALUE;
ZB_ZCL_DECLARE_ELECTRICAL_MEASUREMENT_ATTRIB_LIST(electrical_measurent_attr_list, &g_attr_measurement_type, &g_attr_dcpower);

/* Declare cluster list for the device */
ZB_DECLARE_MEASUREMENT_SENSING_SERVER_CLUSTER_LIST(measurement_sensing_server_clusters, illuminance_measurement_attr_list,
                                                  temperature_measurement_attr_list, water_measurement_attr_list,
                                                  occupancy_sensing_attr_list, electrical_measurent_attr_list);

/* Declare server endpoint */
ZB_DECLARE_MEASUREMENT_SENSING_SERVER_EP(measurement_sensing_server_ep, ZB_SERVER_ENDPOINT, measurement_sensing_server_clusters);

ZB_DECLARE_MEASUREMENT_SENSING_CLIENT_CLUSTER_LIST(measurement_sensing_client_clusters);
/* Declare client endpoint */
ZB_DECLARE_MEASUREMENT_SENSING_CLIENT_EP(measurement_sensing_client_ep, ZB_CLIENT_ENDPOINT, measurement_sensing_client_clusters);

/* Declare application's device context for single-endpoint device */
ZB_DECLARE_MEASUREMENT_SENSING_CTX(measurement_sensing_output_ctx, measurement_sensing_server_ep, measurement_sensing_client_ep);


MAIN()
{
  ARGV_UNUSED;

  /* Trace enable */
  ZB_SET_TRACE_ON();
  /* Traffic dump enable*/
  ZB_SET_TRAF_DUMP_ON();

  /* Global ZBOSS initialization */
  ZB_INIT("measurement_sensing_zr");

  /* Set up defaults for the commissioning */
  zb_set_long_address(g_zc_addr);
  zb_set_network_router_role(1l<<22);

  zb_set_nvram_erase_at_start(ZB_FALSE);
  zb_set_max_children(1);

  /* [af_register_device_context] */
  /* Register device ZCL context */
  ZB_AF_REGISTER_DEVICE_CTX(&measurement_sensing_output_ctx);

  /* Initiate the stack start without starting the commissioning */
  if (zboss_start_no_autostart() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
  }
  else
  {
    /* Call the main loop */
    zboss_main_loop();
  }

  /* Deinitialize trace */
  TRACE_DEINIT();

  MAIN_RETURN(0);
}

/* Callback to handle the stack events */
void zboss_signal_handler(zb_uint8_t param)
{
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_SKIP_STARTUP:
        zboss_start_continue();
        break;

      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
        break;

      default:
        TRACE_MSG(TRACE_APP1, "Unknown signal %d", (FMT__D, (zb_uint16_t)sig));
    }
  }
  else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
  {
    TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d sig %d", (FMT__D_D, ZB_GET_APP_SIGNAL_STATUS(param), sig));
  }

  /* Free the buffer if it is not used */
  if (param)
  {
    zb_buf_free(param);
  }
}