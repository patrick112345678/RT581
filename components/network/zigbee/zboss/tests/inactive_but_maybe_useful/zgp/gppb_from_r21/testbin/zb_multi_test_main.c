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


#define ZB_TRACE_FILE_ID 41390
#undef ZB_MULTI_TEST
#include "zb_common.h"

#ifdef UNIX
#include <string.h>
#include <ctype.h>
#else
#include "zb_console_monitor.h"
#include "zb_osif.h"
#endif

typedef struct zb_test_table_s
{
    char *test_name;
    void (*main_p)();
    void (*startup_complete_p)(zb_uint8_t param);
}
zb_test_table_t;

#ifdef ZB_ZGPD_ROLE
#ifdef ZB_CERTIFICATION_HACKS
#include "zgpd_th_tests_table.h"
#else
#include "zgpd_tests_table.h"
#endif
#else
#ifdef ZB_CERTIFICATION_HACKS
#include "zc_th_tests_table.h"
#else
#include "zc_tests_table.h"
#endif
#endif

#ifdef UNIX
static void get_test_name(int argc, char **argv, char *test_name);
#endif

static zb_int_t s_test_i;

zb_int_t g_argc;
char **g_argv;

MAIN()
{
    unsigned char test_name[60];
    zb_uint_t i;

    ZB_BZERO(test_name, sizeof(test_name));
#ifdef UNIX
    get_test_name(argc, argv, (char *)test_name);
#else
    zb_console_monitor_init();
    zb_console_monitor_get_cmd(test_name, sizeof(test_name));
#endif

    for (i = 0 ;
            i < ZB_ARRAY_SIZE(s_tests_table)
            && strcmp(s_tests_table[i].test_name, (char const *)test_name);
            ++i)
    {
    }
    if (i < ZB_ARRAY_SIZE(s_tests_table))
    {
        s_test_i = i;
        g_argc = argc;
        g_argv = argv;
        (*s_tests_table[i].main_p)();
    }
    else
    {
#ifdef UNIX
        printf("Oops! strange test name %s\n", test_name);
#endif
    }
}


void zb_zdo_startup_complete(zb_uint8_t param)
{
    (*s_tests_table[s_test_i].startup_complete_p)(param);
}


#ifdef UNIX
static void construct_test_name_component(char *name, char *testname);
#endif

#ifdef UNIX
static void get_test_name(int argc, char **argv, char *test_name)
{
    if (argc > 2
            && !strcmp(argv[1], "-t"))
    {
        strcpy(test_name, argv[2]);
    }
    else
    {
        char wd[512];

        getcwd(wd, sizeof(wd) - 1);
        /* Test name in the array converted to uppercase concatenation of dir name
         * and executable name, with - changed to _ */
        construct_test_name_component(wd, test_name);
        strcat(test_name, "_");
        construct_test_name_component(argv[0], test_name + strlen(test_name));
    }
}
#endif


#ifdef UNIX
static void construct_test_name_component(char *name, char *testname)
{
    char *p;

    p = strrchr(name, '/');
    if (p)
    {
        p++;
    }
    else
    {
        p = name;
    }
    while (*p)
    {
        if (*p == '-')
        {
            *testname++ = '_';
        }
        else
        {
            *testname++ = toupper(*p);
        }
        p++;
    }
}
#endif
