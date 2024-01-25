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
/* PURPOSE: Parse dump file for debug purposes
*/


#include <stdio.h>

static char s_x_tbl[] = "0123456789abcdef";

static void
dumpx(int v)
{
    putchar(s_x_tbl[((v) >> 4) & 0xf]);
    putchar(s_x_tbl[((v) & 0xf)]);
    putchar(' ');
}

int
main(int argc, char **argv)
{
    int c;
    int i;
    int l;
    int n = 1;

    (void)argv;
    (void)argc;

    while ((l = getchar()) != EOF)
    {
        printf("%d\n", n);
        n++;
        dumpx(l);
        for (i = 0; i < l - 1 && (c = getchar()) != EOF; ++i)
        {
            if (i % 16 == 0)
            {
                putchar('\n');
            }
            dumpx(c);
        }
        putchar('\n');
        putchar('\n');
    }
}
