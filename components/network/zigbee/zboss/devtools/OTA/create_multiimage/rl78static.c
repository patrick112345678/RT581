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
    uint16_t crc;
    int ret;
    int c;
    int first = 1;
    FILE *f = fopen(argv[1], "r");
    ret = fread(&size, 1, sizeof(size), f);
    printf("#define RL78SIZE %d\n", size);
    printf("static const zb_uint8_t s_rl78img[RL78SIZE] = {");
    while (size && (c = getc(f)) != EOF)
    {
        size--;
        if (first)
        {
            first = 0;
        }
        else
        {
            printf(",");
        }
        printf("0x%02x", c & 0xff);
    }
    printf("};\n");
    ret = fread(&crc, 1, sizeof(crc), f);
    printf("#define RL78CRC 0x%04x\n", crc & 0xffff);
    return 0;
}
