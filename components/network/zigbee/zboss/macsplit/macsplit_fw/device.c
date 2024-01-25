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
/* PURPOSE: MAC split device application
*/

#define ZB_TRACE_FILE_ID 1
#include "zb_common.h"

#include "device.h"

#ifdef ZB_CONFIG_NRF52
#warning ZB_CONFIG_NRF52 enabled!
#include "sdk_common.h"
#include "nrfx_power_clock.h"
#endif

#ifdef ZB_PLATFORM_EFR32MG
#warning ZB_PLATFORM_EFR32MG enabled!
#include <application_properties.h>

const ApplicationProperties_t appProperties __attribute((used)) __attribute((section(".application_properties"))) =
{
    .magic = APPLICATION_PROPERTIES_MAGIC,
    .structVersion = APPLICATION_PROPERTIES_VERSION,
    .signatureType = APPLICATION_SIGNATURE_NONE,
    .signatureLocation = 0xFFFFFFFFU,
    .app = {
        .type = APPLICATION_TYPE_MCU,
        .version = MACSPLIT_APP_VERSION,
        .capabilities = 0UL,
        .productId = { 0U },
    },
    .cert = NULL,
    .longTokenSectionAddress = NULL,
};

#ifdef ZB_PLATFORM_EFR32MG_EBL
#include PLATFORM_HEADER
#include "hal/micro/micro.h"
#include "hal/micro/cortexm3/memmap.h"
#include "stack/include/ember-types.h"
// Pull in the SOFTWARE_VERSION and EMBER_BUILD_NUMBER from the stack
#include "stack/config/config.h"

// The startup files for IAR and GCC use different names for the vector table.
// We declare both here, but only actually use the one that's appropriate.
#ifdef __ICCARM__
#define VECTOR_TABLE  __vector_table
#elif defined(__GNUC__)
#define VECTOR_TABLE  __Vectors
#else
#error Do not know how to get the vector table for this compiler.
#endif
extern const HalVectorTableType VECTOR_TABLE[];
VAR_AT_SEGMENT(NO_STRIPPING const HalAppAddressTableType halAppAddressTable, __AAT__) =
{
    .baseTable = {
        .topOfStack = _CSTACK_SEGMENT_END,
        .resetVector = Reset_Handler,
        .nmiHandler = NMI_Handler,
        .hardFaultHandler = HardFault_Handler,
        .type = APP_ADDRESS_TABLE_TYPE,
        .version = AAT_VERSION,
        .vectorTable = VECTOR_TABLE
    },
    .platInfo = PLAT, // type of platform, defined in micro.h
    .microInfo = MICRO, // type of micro, defined in micro.h
    .phyInfo = PHY, // type of phy, defined in micro.h
    .aatSize = sizeof(HalAppAddressTableType),  // size of aat itself
    .softwareVersion = 0/*SOFTWARE_VERSION*/,
    .softwareBuild = 0/*EMBER_BUILD_NUMBER*/,
    .timestamp = 0, // Unix epoch time of .ebl file, filled in by ebl gen
    .imageInfo = "", // string, filled in by ebl generation
    .appProps = &appProperties, // ptr to app properties struct for Gecko bootloader
    .reserved = { 0 },
    .imageCrc = 0, // CRC over following pageRanges, filled in by ebl gen
    .pageRanges = { // Flash pages used by app, filled in by ebl gen
        { UNUSED_AAT_PAGE_NUMBER, UNUSED_AAT_PAGE_NUMBER },
        { UNUSED_AAT_PAGE_NUMBER, UNUSED_AAT_PAGE_NUMBER },
        { UNUSED_AAT_PAGE_NUMBER, UNUSED_AAT_PAGE_NUMBER },
        { UNUSED_AAT_PAGE_NUMBER, UNUSED_AAT_PAGE_NUMBER },
        { UNUSED_AAT_PAGE_NUMBER, UNUSED_AAT_PAGE_NUMBER },
        { UNUSED_AAT_PAGE_NUMBER, UNUSED_AAT_PAGE_NUMBER }
    },
    .simeeBottom = _SIMEE_SEGMENT_BEGIN,
    .customerApplicationVersion = MACSPLIT_APP_VERSION, // a version field for the customer
    .internalStorageBottom = _INTERNAL_STORAGE_SEGMENT_BEGIN,
    .imageStamp = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0XFF }, // image stamp (filled in by em3xx_convert)
    .familyInfo = FAMILY, // type of family, defined in micro.h
    .bootloaderReserved = { 0 }, // zero fill, previously was 0xFF fill
    .debugChannelBottom = _DEBUG_CHANNEL_SEGMENT_BEGIN,
    .noInitBottom = _NO_INIT_SEGMENT_BEGIN,
    .appRamTop = _BSS_SEGMENT_END, // NO LONGER USED! (set to __BSS__ for 3xx convert)
    .globalTop = _BSS_SEGMENT_END,
    .cstackTop = _CSTACK_SEGMENT_END,
    .initcTop = _DATA_INIT_SEGMENT_END,
    .codeTop = _TEXT_SEGMENT_END,
    .cstackBottom = _CSTACK_SEGMENT_BEGIN,
    .heapTop = _EMHEAP_SEGMENT_END,
    .simeeTop = _SIMEE_SEGMENT_END,
    .debugChannelTop = _DEBUG_CHANNEL_SEGMENT_END
};
#endif /* ZB_PLATFORM_EFR32MG_EBL */

#endif /* ZB_PLATFORM_EFR32MG */

void zboss_signal_handler(zb_uint8_t param)
{
    zb_zdo_app_signal_hdr_t *sg_p = NULL;
    /* Get application signal from the buffer */
    zb_zdo_app_signal_type_t sig =  zb_get_app_signal(param, &sg_p);

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_COMMON_SIGNAL_CAN_SLEEP:
        {
#if defined(ZB_USE_SLEEP)
            zb_sleep_now();
#endif
            break;
        }
        }
    }
    else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
    {
        TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
    }

    if (param)
    {
        zb_buf_free(param);
    }
}



void zb_ib_set_defaults(zb_char_t *rx_pipe)
{
    ZVUNUSED(rx_pipe);
}

void zb_nwk_init()
{
}

void zb_zcl_diagnostics_inc(zb_uint16_t attr_id, zb_uint8_t value)
{
    ZVUNUSED(attr_id);
    ZVUNUSED(value);
}

void zb_aps_init()
{
}

void zb_zdo_init()
{
}

void zb_zcl_init()
{
}

void zll_init()
{
}

void zb_nwk_unlock_in(zb_uint8_t param)
{
    ZVUNUSED(param);
}

zb_bool_t zb_zgp_will_skip_all_packets(void)
{
    return ZB_FALSE;
}

static zb_uint32_t get_device_version()
{
    return MACSPLIT_APP_VERSION;
}

MAIN()
{
    ARGV_UNUSED;

#if defined ZB_TRACE_LEVEL
    /* choose trace transport and trace level if it needs */
    ZB_SET_TRACE_TRANSPORT(ZB_TRACE_TRANSPORT_UART);
    ZB_SET_TRACE_LEVEL(ZB_TRACE_LEVEL);
    ZB_SET_TRACE_MASK(ZB_TRACE_MASK);
#endif

    zb_macsplit_set_cb_dev_version(get_device_version);

    ZB_INIT("macsplit_device");

    TRACE_MSG(TRACE_ERROR, "radio device version: %d", (FMT__D, MACSPLIT_APP_VERSION));

    while (!ZB_SCHEDULER_IS_STOP())
    {
        zb_sched_loop_iteration();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}
