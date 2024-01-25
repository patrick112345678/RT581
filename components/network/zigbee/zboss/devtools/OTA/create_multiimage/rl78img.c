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
*/


#include <stdio.h>
#include <stdint.h>

int
main(
    int argc,
    char **argv)
{
    uint32_t size;
    int16_t crc;
    int c;
    int ret;
    FILE *f = fopen(argv[1], "r");
    FILE *of = fopen(argv[2], "w");
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    size = (size + 1023) / 1024 * 1024;
    fseek(f, 0, SEEK_SET);
    fwrite(&size, 1, sizeof(size), of);
    crc = 0;
    while (size)
    {
        size--;
        c = getc(f);
        crc -= (c & 0xff);
        fputc(c, of);
    }
    ret = fwrite(&crc, 1, sizeof(crc), of);
    fclose(of);
    fclose(f);
}
