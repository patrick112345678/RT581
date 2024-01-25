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

Parse format macros (like #define FMT__H_H_L_H) and outputs strings to
substitute to zb_trace.h.

TRACE_ARG_SIZE(n_h, n_d, n_l, n_p, n_a)

Got line like #define FMT__H_A_A_H_H                            TRACE_ARG_SIZE(3,0,0,0,2)

*/


#include <stdio.h>
#include <string.h>

int
main(
    int argc,
    char **argv)
{
    char s[150];
    char def[80];
    char *p;
    int size;
    int n[5];

    while (fgets(s, sizeof(s), stdin))
    {
        memset(n, 0, sizeof(n));
        sscanf(s, "%*s%s", def);
        p = def + 5;
        size = 0;
        while (*p)
        {
            switch (*p)
            {
            case 'P':
                n[3]++;
                break;
            case 'A':
                n[4]++;
                break;
            case 'H':
            case 'C':
                n[0]++;
                break;
            case 'D':
                n[1]++;
                break;
            case 'L':
                n[2]++;
                break;
            }
            p++;
        }
        printf("#define %-*s\tTRACE_ARG_SIZE(%d,%d,%d,%d,%d)\n", 40, def, n[0], n[1], n[2], n[3], n[4]);
    }
}

