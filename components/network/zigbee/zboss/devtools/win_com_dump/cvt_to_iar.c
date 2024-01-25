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
/* PURPOSE: Count sizes of trace arguments for IAR
*/

#include <stdio.h>
#include <string.h>

int
main(
    int argc,
    char **argv)
{
    char s[256];
    (void)argv;
    (void)argc;
    while (gets(s))
    {
        int len = 0;
        char *p = strstr(s, "FMT__");
        if (p)
        {
            p += 5;
            while (*p != ' ' && *p != '\t')
            {
                switch (*p)
                {
                case 'H':
                case 'D':
                case 'C':
                    len += 2;
                    break;
                case 'P':
                    len += 2;           /* maybe, 3? */
                    break;
                case 'A':
                    len += 8;
                    break;
                case 'L':
                    len += 4;
                    break;
                }
                p += 2;
            }
            p = strrchr(s, ',');
            p[1] = 0;
            printf("%s %d\n", s, len);
        }
        else
        {
            puts(s);
        }
    }
    return 0;
}

