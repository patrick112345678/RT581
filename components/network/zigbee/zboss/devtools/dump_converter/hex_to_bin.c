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
/* PURPOSE: Convert hex dump to the binary dump

This utility get hex dump file skipping empty lines and comments (lines starged
with #) and outputs binary file.

To be used to create dump files by hands, initially - for wireshark plugin debug.

See dump1.txt for text file example.

Usage: ./hex_to_bin < dump1.txt >dump1.bin

*/


#include <stdio.h>

int
main(int argc, char **argv)
{
    char *p;
    char s[512];
    int n;
    unsigned v;

    (void)argc;
    (void)argv;

    while (fgets(s, sizeof(s), stdin))
    {
        if (*s == '#' || *s == 0)
        {
            continue;
        }
        p = s;
        while (sscanf(p, "%2x%n", &v, &n) == 1)
        {
            p += n;
            putchar(v);
        }
    }
}
