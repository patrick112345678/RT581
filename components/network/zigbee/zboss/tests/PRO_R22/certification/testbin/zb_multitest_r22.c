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


#define ZB_TRACE_FILE_ID 40681
#undef ZB_MULTI_TEST

#include "zb_multitest.h"
#include "../common/zb_cert_test_globals.h"

/* it's needed to automatic build */
#define ZB_TEST_GROUP_NUM

#ifndef ZB_TEST_GROUP_NORDIC_R22_SPOTCHECK

#ifdef ZB_NSNG

#define ZB_TEST_GROUP_ZCP_R22_APS
#define ZB_TEST_GROUP_ZCP_R22_NWK
#define ZB_TEST_GROUP_ZCP_R22_PED
#define ZB_TEST_GROUP_ZCP_R22_PRO
#define ZB_TEST_GROUP_ZCP_R22_R20
#define ZB_TEST_GROUP_ZCP_R22_R21
#define ZB_TEST_GROUP_ZCP_R22_ZDO
#define ZB_TEST_GROUP_ZCP_R22_SEC
#define ZB_TEST_GROUP_ZCP_R22_BDB
#define ZB_TEST_GROUP_ZCP_R22_R22

#elif defined NRF52840_XXAA

#define ZB_TEST_GROUP_ZCP_R22_APS
#define ZB_TEST_GROUP_ZCP_R22_BDB
#define ZB_TEST_GROUP_ZCP_R22_NWK
#define ZB_TEST_GROUP_ZCP_R22_PED
#define ZB_TEST_GROUP_ZCP_R22_PRO
#define ZB_TEST_GROUP_ZCP_R22_R20
#define ZB_TEST_GROUP_ZCP_R22_R21
#define ZB_TEST_GROUP_ZCP_R22_R22
#define ZB_TEST_GROUP_ZCP_R22_SEC
#define ZB_TEST_GROUP_ZCP_R22_ZDO

#elif defined NRF52833_XXAA

#ifndef SOFTDEVICE_PRESENT

#define ZB_TEST_GROUP_ZCP_R22_APS
#define ZB_TEST_GROUP_ZCP_R22_BDB
#define ZB_TEST_GROUP_ZCP_R22_NWK
#define ZB_TEST_GROUP_ZCP_R22_PED
#define ZB_TEST_GROUP_ZCP_R22_PRO
#define ZB_TEST_GROUP_ZCP_R22_R20
#define ZB_TEST_GROUP_ZCP_R22_R21
#define ZB_TEST_GROUP_ZCP_R22_R22
#define ZB_TEST_GROUP_ZCP_R22_SEC
#define ZB_TEST_GROUP_ZCP_R22_ZDO

#else /* SOFTDEVICE_PRESENT */

#ifdef ZB_ED_ROLE

#define ZB_TEST_GROUP_ZCP_R22_APS
#define ZB_TEST_GROUP_ZCP_R22_BDB
#define ZB_TEST_GROUP_ZCP_R22_NWK
#define ZB_TEST_GROUP_ZCP_R22_PED
#define ZB_TEST_GROUP_ZCP_R22_PRO
#define ZB_TEST_GROUP_ZCP_R22_R20
#define ZB_TEST_GROUP_ZCP_R22_R21
#define ZB_TEST_GROUP_ZCP_R22_R22
#define ZB_TEST_GROUP_ZCP_R22_SEC
#define ZB_TEST_GROUP_ZCP_R22_ZDO

#else /* ZB_ED_ROLE */

#ifdef ZB_TEST_GROUP_1

#define ZB_TEST_GROUP_ZCP_R22_APS_DUT
#define ZB_TEST_GROUP_ZCP_R22_BDB_DUT
#define ZB_TEST_GROUP_ZCP_R22_NWK_DUT
#define ZB_TEST_GROUP_ZCP_R22_PED_DUT
#define ZB_TEST_GROUP_ZCP_R22_SEC_DUT
#define ZB_TEST_GROUP_ZCP_R22_ZDO_DUT

#elif defined ZB_TEST_GROUP_2

#define ZB_TEST_GROUP_ZCP_R22_PRO_DUT
#define ZB_TEST_GROUP_ZCP_R22_R20_DUT
#define ZB_TEST_GROUP_ZCP_R22_R21_DUT
#define ZB_TEST_GROUP_ZCP_R22_R22_DUT

#else /* ZB_TEST_GROUP_2 */

/* for manual build */
#define ZB_TEST_GROUP_ZCP_R22_APS_DUT
/* #define ZB_TEST_GROUP_ZCP_R22_BDB_DUT */
#define ZB_TEST_GROUP_ZCP_R22_NWK_DUT
#define ZB_TEST_GROUP_ZCP_R22_PED_DUT
#define ZB_TEST_GROUP_ZCP_R22_PRO_DUT
#define ZB_TEST_GROUP_ZCP_R22_R20_DUT
#define ZB_TEST_GROUP_ZCP_R22_R21_DUT
#define ZB_TEST_GROUP_ZCP_R22_R22_DUT
#define ZB_TEST_GROUP_ZCP_R22_SEC_DUT
#define ZB_TEST_GROUP_ZCP_R22_ZDO_DUT

#endif /* !ZB_TEST_GROUP_2 */

#endif /* ZB_ED_ROLE */

#endif /* SOFTDEVICE_PRESENT */

#elif defined ZB_TEST_GROUP_ALL

#define ZB_TEST_GROUP_ZCP_R22_APS
#define ZB_TEST_GROUP_ZCP_R22_NWK
#define ZB_TEST_GROUP_ZCP_R22_PED
#define ZB_TEST_GROUP_ZCP_R22_PRO
#define ZB_TEST_GROUP_ZCP_R22_R20
#define ZB_TEST_GROUP_ZCP_R22_R21
#define ZB_TEST_GROUP_ZCP_R22_ZDO
#define ZB_TEST_GROUP_ZCP_R22_SEC
#define ZB_TEST_GROUP_ZCP_R22_BDB
#define ZB_TEST_GROUP_ZCP_R22_R22

#endif /* ZB_TEST_GROUP_ALL */

#endif /* #ifndef ZB_TEST_GROUP_NORDIC_R22_SPOTCHECK */

#ifdef ZB_ED_ROLE
#include "zed_tests_table.h"
#else
#include "zc_tests_table.h"
#endif



void zb_multitest_init(void)
{
  zb_cert_test_set_init_globals();
}


const zb_test_table_t* zb_multitest_get_tests_table(void)
{
  return s_tests_table;
}


zb_uindex_t zb_multitest_get_tests_table_size(void)
{
  return ZB_ARRAY_SIZE(s_tests_table);
}

