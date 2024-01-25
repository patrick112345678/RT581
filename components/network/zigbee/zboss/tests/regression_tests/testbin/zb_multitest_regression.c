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
/*  PURPOSE: Main module for multi-test FW
*/

#undef ZB_MULTI_TEST
#define ZB_TRACE_FILE_ID 40361

#include "zb_multitest.h"
#include "../common/zb_reg_test_globals.h"

#if defined ZB_NSNG || defined NRF52840_XXAA

#define ZB_REG_TEST_GROUP_APS
#define ZB_REG_TEST_GROUP_BDB
#define ZB_REG_TEST_GROUP_COM
#define ZB_REG_TEST_GROUP_OTA
#define ZB_REG_TEST_GROUP_ZCL
#define ZB_REG_TEST_GROUP_ZDO
#define ZB_REG_TEST_GROUP_MAC
#define ZB_REG_TEST_GROUP_SEC
#define ZB_REG_TEST_GROUP_HA
#define ZB_REG_TEST_GROUP_NWK

#define ZB_REG_TEST_GROUP_MEMCONF
#define ZB_REG_TEST_GROUP_OSIF
#define ZB_REG_TEST_GROUP_INT
#define ZB_REG_TEST_GROUP_PRODCONF
#define ZB_REG_TEST_GROUP_NVRAM

#else
#error Platform does not supported!
#endif  /* ZB_NSNG || NRF52840_XXAA */

#ifdef ZB_ED_ROLE
#include "zed_tests_table.h"
#else
#include "zc_tests_table.h"
#endif

#if defined ZB_REG_TEST_GROUP_MEMCONF
#include "zb_mem_config_med.h"
#endif  /* ZB_REG_TEST_GROUP_MEMCONF */

#ifdef NCP_MODE_HOST
#define ZB_TEST_DISABLE_TAG_RTP_WWAH
#endif

void zb_multitest_init(void)
{
  zb_reg_test_set_init_globals();
}


const zb_test_table_t* zb_multitest_get_tests_table(void)
{
  return s_tests_table;
}


zb_uindex_t zb_multitest_get_tests_table_size(void)
{
  return ZB_ARRAY_SIZE(s_tests_table);
}
