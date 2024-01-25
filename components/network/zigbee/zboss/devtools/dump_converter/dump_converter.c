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
/* PURPOSE: convert *.dump file with traffic dump produced by ZBOSS' win_com_dump to *.pcap
file to be read by Wireshark.
*/

#ifdef WIN32
#ifndef __MINGW32__
#define __MINGW32__
#endif

#include "../../include/zb_common.h"
#include "dc_mingw.h"
//#include <winsock.h>
#else
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
//#include <sys/stat.h>
//#include <sys/socket.h>
//#include <netinet/udp.h>
//#include <netinet/in.h>
//#include <netinet/ip.h>
//#include <net/ethernet.h>
//#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <pcap/pcap.h>
#endif

/* Include minimum stuff to compile for the host Linux/win when current platform is different. */
#define ZB_CONFIG_H
#define UNIX
#include "zb_types.h"
#include "zb_mac_transport.h"

#define ZB_SYMBOL_DURATION_USEC   16
#define ZB_ABASE_SUPERFRAME_DURATION 960
#define ZB_BEACON_INTERVAL_USEC (ZB_SYMBOL_DURATION_USEC * ZB_ABASE_SUPERFRAME_DURATION)
#define ZB_TIME_BEACON_INTERVAL_TO_USEC(t) ((zb_uint64_t)ZB_BEACON_INTERVAL_USEC * (t))
#define ZB_MAX_TIME_VAL ZB_UINT32_MAX
#define ZB_HALF_MAX_TIME_VAL (ZB_MAX_TIME_VAL / 2)
typedef zb_uint32_t zb_time_t;
#define ZB_TIME_SUBTRACT(a, b) ((zb_time_t)((a) - (b)) < ZB_HALF_MAX_TIME_VAL ? (zb_time_t)((a) - (b)) : (zb_time_t)((b) - (a)))

#define IOBUF_SIZE 8192

#define ETH_AND_UDP_HDR_OFFSETS 0

#define ZBOSS_DUMP_HDR_LEN 11

typedef enum zb_dump_converter_mode_e
{
    DUMP_CONVERTER_MODE_NS = 0,
    DUMP_CONVERTER_MODE_N0 = 3,
    DUMP_CONVERTER_MODE_N1 = 1,
    DUMP_CONVERTER_MODE_N2 = 2,
    DUMP_CONVERTER_MODE_N3 = 4
} zb_dump_converter_mode_t;

static void create_eth_hdr(char *buf, int len, int is_w);
static void fcs_add(char *buf, int len);
static void parse_cmd(int argc, char **argv);

char *g_in_file;
char *g_out_file;
pcap_dumper_t *dumper = NULL;
pcap_t *pcap = NULL;
int uz2400regs_mode = 0;
zb_dump_converter_mode_t convert_mode;

static int handle_ns_packet(char *iobuf, zb_mac_transport_hdr_t *mac_hdr,
                            struct pcap_pkthdr *cap_hdr)
{
    iobuf[ETH_AND_UDP_HDR_OFFSETS] = mac_hdr->type + 1;
    iobuf[ETH_AND_UDP_HDR_OFFSETS + 1] = 0xDE;
    iobuf[ETH_AND_UDP_HDR_OFFSETS + 2] = 0x77;
    iobuf[ETH_AND_UDP_HDR_OFFSETS + 3] = 0xFF;

    return 1;
}

static int handle_n2_packet(char *iobuf, zb_mac_transport_hdr_t *mac_hdr,
                            struct pcap_pkthdr *cap_hdr)
{
    zb_dump_hdr_t dh;
    int i = 0;

    memcpy(&dh, iobuf, sizeof(dh));
    memmove(iobuf + ZBOSS_DUMP_HDR_LEN, iobuf + sizeof(dh), mac_hdr->len - sizeof(dh));
    cap_hdr->caplen = cap_hdr->len = mac_hdr->len - sizeof(dh) + ZBOSS_DUMP_HDR_LEN;

    iobuf[i++] = 'Z';
    iobuf[i++] = 'B';
    iobuf[i++] = 'O';
    iobuf[i++] = 'S';
    iobuf[i++] = 'S';
    /* direction */
    iobuf[i++] = (mac_hdr->type >> 7) & 1;
    iobuf[i++] = dh.channel;
    memcpy(&iobuf[i], &dh.trace_cnt, 4);
    i += 4;

    return 1;
}

static int handle_n3_packet(char *iobuf, zb_mac_transport_hdr_t *mac_hdr,
                            struct pcap_pkthdr *cap_hdr)
{
    zb_dump_hdr_v3_t dh;
    int i = 0;

    memcpy(&dh, iobuf, sizeof(dh));

    if (dh.version != 3)
    {
        /* Currently the latest version is 3 */
        return 0;
    }

    memmove(iobuf + ZBOSS_DUMP_HDR_LEN, iobuf + sizeof(dh), mac_hdr->len - sizeof(dh));
    cap_hdr->caplen = cap_hdr->len = mac_hdr->len - sizeof(dh) + ZBOSS_DUMP_HDR_LEN;

    iobuf[i++] = 'Z';
    iobuf[i++] = 'B';
    iobuf[i++] = 'O';
    iobuf[i++] = 'S';
    iobuf[i++] = 'S';
    /* direction & page */
    iobuf[i++] = ((mac_hdr->type >> 7) & 1) | ((dh.page << 1) & 0xFE);
    iobuf[i++] = dh.channel;
    memcpy(&iobuf[i], &dh.trace_cnt, 4);
    i += 4;

    return 1;
}

static int handle_n0_packet(char *iobuf, zb_mac_transport_hdr_t *mac_hdr,
                            struct pcap_pkthdr *cap_hdr)
{
    /* Same new Wireshark dissector, but old dump format:
       - FCS after packet
       - length byte before packet (including FCS)
       - Standard ZBOSS dump header containing information about
       channel in 'type' (sometimes wrong).
    */
    int i = 0;

    /* Cut FCS and put 'valid' bit. Zero LQI and RSSI: we have no
     * that info. */
    iobuf[mac_hdr->len - 2] = 0;
    iobuf[mac_hdr->len - 1] = 0x80;
    /* Cut length byte at begin and allloc space for dump header */
    memmove(iobuf + ZBOSS_DUMP_HDR_LEN, iobuf + 1, mac_hdr->len - 1);
    cap_hdr->caplen = cap_hdr->len = mac_hdr->len - 1 + ZBOSS_DUMP_HDR_LEN;

    iobuf[i++] = 'Z';
    iobuf[i++] = 'B';
    iobuf[i++] = 'O';
    iobuf[i++] = 'S';
    iobuf[i++] = 'S';
    /* direction: 7-th bit */
    iobuf[i++] = (mac_hdr->type >> 7) & 1;
    /* channel is valid for tx only. Zuev's code was:
       hdr.h.type |= (ZB_PIBCACHE_CURRENT_CHANNEL() - ZB_TRANSCEIVER_START_CHANNEL_NUMBER + 1);
       Try to unserstand what does it mean...
       type = 1 (ZB_MAC_TRANSPORT_TYPE_DUMP). Then type |= (channel -
       11 + 1).
       So, we can't distinguish between channels, say, 15 and 14: both
       will be shown as 15.
       Do we ever need to convert it?
    */
    iobuf[i] = 0;
    if (iobuf[i - 1])
    {
        /* tx - have a channel. Not always correct, but... */
        iobuf[i] = (mac_hdr->type & 0x7f) + 10;
    }
    i++;
    /* clear trace_cnt: no such info */
    memset(&iobuf[i], 0, 4);
    i += 4;

    return 1;
}

int main(int argc, char **argv)
{
    char iobuf[IOBUF_SIZE];
    char prev_reg_rx[3];
    FILE *in_f;
    FILE *out_f;
    int pkt_n = 0;
    int prev_len = 0;
    struct pcap_pkthdr cap_hdr;
    unsigned prev_timestamp = (unsigned)~0;
    unsigned running_time = 0;
    zb_mac_transport_hdr_t hdr;
    int last_conv_result;

    dumper = NULL;
    pcap = NULL;
    parse_cmd(argc, argv);
    in_f = fopen(g_in_file, "rb");
    if (!in_f)
    {
        fprintf(stderr, "Cannot read file %s. Error: %s.\n", g_in_file, strerror(errno));
        return -1;
    }
    out_f = fopen(g_out_file, "wb+");
    if (!out_f)
    {
        fprintf(stderr, "Cannot write to file %s. Error %s.\n", g_out_file, strerror(errno));
        return -1;
    }
    ftruncate(fileno(out_f), 0);
    fseek(out_f, 0, SEEK_SET);

    if (convert_mode == DUMP_CONVERTER_MODE_N1)
    {
        pcap = (pcap_t *)pcap_open_dead(DLT_IEEE802_15_4_NONASK_PHY, 4096);
    }
    else
    {
        pcap = (pcap_t *)pcap_open_dead(DLT_IEEE802_15_4, 4096);
    }

    if (!pcap)
    {
        fprintf(stderr, "pcap_fopen_offline error.\n");
    }
    else
    {

#ifdef WIN32
        dumper = (pcap_dumper_t *)pcap_dump_open(pcap, g_out_file);
#else
        dumper = pcap_dump_fopen(pcap, out_f);
#endif

    }
    if (!dumper && pcap)
    {
        fprintf(stderr, "pcap_dump_fopen error:\n%s\n", pcap_geterr(pcap));
    }
    if (!dumper)
    {
        return -1;
    }
    memset(&cap_hdr, 0, sizeof(cap_hdr));

    while (fread(&hdr, 1, 1, in_f) == 1)
    {
        int r;
        pkt_n++;
        if (hdr.len == 0)
        {
            fprintf(stderr, "Zero byte skipped at pkt #%d... Was it 0x0a to 0x0d0a translation?\n",
                    pkt_n);
            pkt_n--;
            continue;
        }

        // printf("Packet #%d: len %d pos %d\n", pkt_n ,hdr.len,ftell (in_f));

        if (fread(&hdr.type, sizeof(hdr) - 1, 1, in_f) != 1)
        {
            fprintf(stderr, "Packet #%d: error reading hdr.\n", pkt_n);
            return -1;
        }

        /* Handle time overflow */
        if (prev_timestamp == (unsigned)~0)
        {
            running_time = hdr.time;
        }
        else
        {
            running_time += ZB_TIME_SUBTRACT(hdr.time, prev_timestamp);
        }

        prev_timestamp = hdr.time;
        hdr.len -= sizeof(hdr);
        memset(iobuf, 0, sizeof(iobuf));

        if ((r = fread(iobuf + ETH_AND_UDP_HDR_OFFSETS, hdr.len, 1, in_f)) != 1)
        {
            fprintf(stderr, "Packet #%d: error reading %d bytes from the input file.\n", pkt_n,
                    hdr.len);
            return -1;
        }


        cap_hdr.caplen = cap_hdr.len = ETH_AND_UDP_HDR_OFFSETS + hdr.len;

        last_conv_result = 1;
        switch (convert_mode)
        {
        case DUMP_CONVERTER_MODE_NS:
            last_conv_result = handle_ns_packet(iobuf, &hdr, &cap_hdr);
            break;
        case DUMP_CONVERTER_MODE_N0:
            last_conv_result = handle_n0_packet(iobuf, &hdr, &cap_hdr);
            break;
        case DUMP_CONVERTER_MODE_N1:
            /*
              To exclude defining of our own constant in
              /usr/include/pcap/bpf.h let's mimic on IEEE 802.15.4 non-ASK PHY
              HW.
              It has following fields before MAC header:
              - 4 bytes hf_ieee802154_nonask_phy_preamble - let's put trace
              line here
              - hf_ieee802154_nonask_phy_sfd - let's put channel here
              - 7 bits of pht length
            */
            break;
        case DUMP_CONVERTER_MODE_N2:
            last_conv_result = handle_n2_packet(iobuf, &hdr, &cap_hdr);
            break;
        case DUMP_CONVERTER_MODE_N3:
            last_conv_result = handle_n3_packet(iobuf, &hdr, &cap_hdr);
            break;
        }

        if (!last_conv_result)
        {
            fprintf(stderr, "Packet #%d: conversion failed.\n", pkt_n);
            break;
        }

        {
            unsigned long long us = ZB_TIME_BEACON_INTERVAL_TO_USEC(running_time);
            cap_hdr.ts.tv_sec = us / 1000000ll;
            cap_hdr.ts.tv_usec = us % 1000000ll;
        }
        pcap_dump((u_char *)dumper, &cap_hdr, ((u_char *)iobuf));
    }
    pcap_dump_flush(dumper);
    pcap_dump_close(dumper);
    fclose(in_f);
    return 0;
}

static void usage(char **argv)
{
    printf("Usage: %s [-ns|n1|n2] dump_file pcap_file.\n", argv[0]);
    printf("-ns is old dump requiring Wireshark plugin which will not be incuded into official Wireshark\n");
    printf("-n0 is convert of old dump file (required old ns support in Wireshark) to be same as dump produced by -n2 (its support is already in Wireshark unstable)\n");
    printf("-n1 is mimic to IEEE802_15_4_NONASK_PHY Wireshark dissector. Preamble is ZBOSS trace #, SFD is ZBOSS channel, OUT can be identified by zero RSSI. Can use stock Wireshark\n");
    printf("-n2 is dump for new ZBOSS Wireshark dissector (already in Wireshark 2.2).\n");
    printf("-n3 (the default) is convert of ZBOSS format v3+.\n");
    exit(1);
}

static void parse_cmd(int argc, char **argv)
{
    int i = 1;

    convert_mode = DUMP_CONVERTER_MODE_N3;
    while (i < argc)
    {
        if (!strcmp(argv[i], "-ns"))
        {
            convert_mode = DUMP_CONVERTER_MODE_NS;
            i++;
            break;
        }
        if (!strcmp(argv[i], "-n0"))
        {
            convert_mode = DUMP_CONVERTER_MODE_N0;
            i++;
            break;
        }
        if (!strcmp(argv[i], "-n1"))
        {
            convert_mode = DUMP_CONVERTER_MODE_N1;
            i++;
            break;
        }
        if (!strcmp(argv[i], "-n2"))
        {
            convert_mode = DUMP_CONVERTER_MODE_N2;
            i++;
            break;
        }
        if (!strcmp(argv[i], "-n3"))
        {
            convert_mode = DUMP_CONVERTER_MODE_N3;
            i++;
            break;
        }

        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
        {
            usage(argv);
        }
        else
        {
            break;
        }
    }
    if (i < argc)
    {
        g_in_file = argv[i];
        i++;
    }
    else
    {
        usage(argv);
    }
    if (i < argc)
    {
        g_out_file = argv[i];
        i++;
    }
    else
    {
        usage(argv);
    }
}

static void fcs_add(char *buf, int len)
{
    static const unsigned short table[256] =
    {
        0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
        0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
        0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
        0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
        0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
        0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
        0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
        0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
        0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
        0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
        0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
        0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
        0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
        0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
        0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
        0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
        0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
        0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
        0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
        0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
        0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
        0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
        0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
        0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
        0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
        0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
        0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
        0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
        0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
        0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
        0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
        0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
    };
    unsigned short crc = 0;
    int i;
    unsigned char *p = (unsigned char *)buf;

    for (i = 0; i < len; ++i)
    {
        crc = table[(crc ^ *p++) & 0xff] ^ (crc >> 8);
    }
    memmove(p, &crc, 2);
    /* Little-endian only! */
}
