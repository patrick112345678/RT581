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
/* PURPOSE: Console Windows utility which reads serial port and writes trace and
traffic dump to files.
*/

#define DEBUG_RESYNC
#define DEBUG_DUMP_RAW
#define SKIP_OLD_OPTS

#include "win_com_dump_common.h"
#include "itm_trace.h"

//static void convert_dumpf(char *name);

static int open_serial(char *name);
static void close_serial();
static int port_read_os(serial_handle_t comf, void *buf, int n);
static int read_nordic_log(serial_handle_t comf, void *buf, int maxn, FILE *outf);
static void read_common_trace();

extern void read_itm_trace();

static unsigned char big_buf[1024];
static unsigned char buf[512/*256*/];

win_com_dump_ctx_t wcd_ctx = { 0 };

int
main(
    int argc,
    char **argv)
{
    int i = 1;

    if (argc < 4)
    {

#ifndef ZB_PLATFORM_LINUX_PC32
        /* Changed program to accept 2 options: for compiler and work mode */
        fprintf(stdout,
                "ZBOSS trace & traffic dump collecting utility.\n"
                "Usage: win_com_dump.exe "
#ifndef SKIP_OLD_OPTS
                "-U/-B/-i/-k/-I/-J/-O/-S"
#else
                "-U/-B/-N/-T"
#endif
                " com_port_or_file_name trace_file_name {dump_file_name} {-s script_file_name}\n"
#ifndef SKIP_OLD_OPTS
                "-U/-i/-I/-k/ define compiler and architecture. These can be combined with any of the other keys"
                "Parameters that define compiler or arch should be placed first"
                "To get raw logs from serial port: win_com_dump -O com_port_name {log_dump_file_name} {unused_file_name}\n"
#endif
                "Example:\n"
                "win_com_dump.exe -U \\\\.\\COM11 trace.log traf.dump\n\n"
#ifndef SKIP_OLD_OPTS
                "To parse logs from JTAG trace file: win_com_dump -J {jtag_file_name} {trace_file_name} {dump_file_name}\n"
                "Example: win_com_dump.exe -J logdump.swo trace.log traf.dump\n"
#endif
                "To parse binary logs from JTAG trace file  (if SDK compiled to trace via JTAG):\n"
                "win_com_dump -B {jtag_file_name} {trace_file_name} {dump_file_name}\n\n"
                "Convert traffic dump file into Wireshark's .pcap by running:\ndump_converter.exe traf.dump traf.pcap\n\n"
               );
#else
        fprintf(stdout, "Usage: lin_com_dump -i/-k/-I/-J/-O/-B/-S {serial_port_device_or_file_name} {trace_file_name} {dump_file_name}\n"
                "-i/-I/-k/ define compiler and architecture. These can be combined with any of the other keys"
                "Parameters that define compiler or arch should be placed first"
                "To get raw logs from serial port: lin_com_dump -O {serial_port_device_name} {log_dump_file_name} {unused_file_name}\n"
                "Example: lin_com_dump -I /dev/ttyS0 trace.log traf.dump\n\n"
                "To parse logs from JTAG trace file: lin_com_dump -J {jtag_file_name} {trace_file_name} {dump_file_name}\n"
                "To parse binary logs from JTAG trace file: lin_com_dump -B {jtag_file_name} {trace_file_name} {dump_file_name}\n"
                "Example: lin_com_dump -J logdump.swo trace.log traf.dump\n"
               );
#endif

        return -1;
    }


    /* Check for compiler/arch options */
    if (!strcmp(argv[i], "-i"))
    {
        i++;
        wcd_ctx.g_mode |= MODE_IAR51;
        fprintf(stdout, "Work for IAR/8051\n");
    }
    else if (!strcmp(argv[i], "-I"))
    {
        i++;
        wcd_ctx.g_mode |= MODE_IARARM;
        fprintf(stdout, "Work for IAR/ARM\n");
    }
    else if (!strcmp(argv[i], "-k"))
    {
        i++;
        wcd_ctx.g_mode |= MODE_KEIL;
        fprintf(stdout, "Work for Keil/8051\n");
    }


    /* Now choose working mode*/
    if (!strcmp(argv[i], "-S"))
    {
        i++;
        wcd_ctx.g_mode |= MODE_SDKLOGS;
        fprintf(stdout, "Convert SDK logs\n");
    }
    else if (!strcmp(argv[i], "-J"))
    {
        i++;
        wcd_ctx.g_mode |= MODE_JTAGLOGS;
        fprintf(stdout, "Convert JTAG/SWD logs\n");
    }
    else if (!strcmp(argv[i], "-B"))
    {
        i++;
        wcd_ctx.g_mode |= MODE_JTAGLOGSBIN;
        fprintf(stdout, "Convert binary JTAG/SWD logs\n");
    }
    else if (!strcmp(argv[i], "-U"))
    {
        i++;
        wcd_ctx.g_mode |= MODE_USARTLOGSBIN;
        fprintf(stdout, "Work for USART binary logs\n");
    }
    else if (!strcmp(argv[i], "-N"))
    {
        i++;
        wcd_ctx.g_mode |= MODE_USARTLOGSBIN | MODE_NORDIC_LOG;
        fprintf(stdout, "Work for Nordic text logs\n");
    }
    else if (!strcmp(argv[i], "-NN"))
    {
        i++;
        wcd_ctx.g_mode |= MODE_USARTLOGSBIN | MODE_NORDIC_LOG | MODE_NORDIC_LOG_115200;
        fprintf(stdout, "Work for Nordic text logs\n");
    }
    else if (!strcmp(argv[i], "-n"))
    {
        i++;
        wcd_ctx.g_mode |= MODE_NORDIC_LOG | MODE_JTAGLOGSBIN;
        fprintf(stdout, "Work for Nordic text logs from file\n");
    }
    else if (!strcmp(argv[i], "-"))
    {
        i++;
        wcd_ctx.g_mode |= MODE_STANDARD_INPUT | MODE_JTAGLOGSBIN;
        fprintf(stdout, "Work on logs from standard input\n");
    }
    else if (!strcmp(argv[i], "-R"))
    {
        i++;
        wcd_ctx.g_mode |= MODE_JTAGLOGSBIN | MODE_HEX;
        fprintf(stdout, "Work on logs from standard input\n");
    }

    else if (!strcmp(argv[i], "-u"))
    {
        i++;
        wcd_ctx.g_mode |= MODE_USARTLOGSBIN_SDK;
        fprintf(stdout, "Work for USART binary logs from file\n");
    }
    else if (!strcmp(argv[i], "-X"))
    {
        i++;
        wcd_ctx.g_mode |= MODE_USARTLOGSBIN_XAP;
        fprintf(stdout, "Work for XAP5 UART binary logs\n");
    }
    else if (!strcmp(argv[i], "-x"))
    {
        i++;
        wcd_ctx.g_mode |= MODE_SIFLOGSBIN_XAP;

        fprintf(stdout, "Work for XAP5 SIF binary logs\n");
    }
    else if (!strcmp(argv[i], "-O"))
    {
        i++;
        wcd_ctx.g_mode |= MODE_RAWSERIAL;
        fprintf(stdout, "Getting serial port raw logs\n");
    }
    else if (!strcmp(argv[i], "-T"))
    {
        i++;
        wcd_ctx.g_mode |= MODE_ITM;
        fprintf(stdout, "Convert ITM logs\n");
    }

#define TRACE_FILES_NUMBER 3
    if (argc > i + TRACE_FILES_NUMBER)
    {
        /* Check for additional options, as scripts */
        if (!strcmp(argv[i + TRACE_FILES_NUMBER], "-s"))
        {
            wcd_ctx.g_mode |= MODE_CONSOLE_SCRIPT;
            fprintf(stdout, "Using console scripting\n");
        }
    }

    if (wcd_ctx.g_mode & MODE_SDKLOGS)
    {
        FILE *parsef;
        char buf[512];
        wcd_ctx.tracef = fopen(argv[i], "rb");
        parsef = fopen(strcat(argv[i], "_p"), "wb");

        while (fgets(buf, sizeof(buf), wcd_ctx.tracef) != NULL)
        {
            printf("Parsing: %s", buf);
            parse_trace_line(buf, strlen(buf), parsef);
        }
        fclose(wcd_ctx.tracef);
        return 0;
    }

    if (!strcmp(argv[i], "-"))
    {
        wcd_ctx.g_mode |= MODE_STANDARD_INPUT;
        i++;
    }
    if (wcd_ctx.g_mode & MODE_STANDARD_INPUT)
    {
        wcd_ctx.comf = 0; /* stdin */
    }
    else
    {
        /* Open serial or source file */
        if (open_serial(argv[i]) == 0)
        {
            fprintf(stdout, "\nSerial/input file %s open ok\n", argv[i]);
        }
        else
        {
            fprintf(stdout, "Can't open serial/input file %s\n", argv[i]);
            return 1;
        }
        i++;
    }

    wcd_ctx.tracef = fopen(argv[i], "wb");
    if (!wcd_ctx.tracef)
    {
        fprintf(stdout, "Can't open trace file %s\n", argv[i]);
        return -1;
    }

    i++;
    wcd_ctx.dumpf = fopen(argv[i], "wb");
    if (!wcd_ctx.dumpf)
    {
        fprintf(stdout, "Can't open dump file %s\n", argv[3]);
        return -1;
    }

#ifdef DEBUG_DUMP_RAW
    sprintf((char *)big_buf, "%s.raw", argv[i]);
    wcd_ctx.fraw = fopen((char *)big_buf, "wb");
    if (!wcd_ctx.fraw)
    {
        fprintf(stdout, "Can't open raw file %s\n", big_buf);
        return -1;
    }
    fseek(wcd_ctx.fraw, 0l, SEEK_SET);
#endif

#ifdef DEBUG_RESYNC
    sprintf((char *)big_buf, "%s.dump", argv[i]);
    wcd_ctx.fdump = fopen((char *)big_buf, "wb");
    if (!wcd_ctx.fdump)
    {
        fprintf(stdout, "Can't open raw file %s\n", big_buf);
        return -1;
    }
    fseek(wcd_ctx.fdump, 0l, SEEK_SET);
#endif

    i++;
    if ((argc > i + 1) &&
            IS_SCRIPTING(wcd_ctx.g_mode))
    {
        i++;
        wcd_ctx.scriptf = fopen(argv[i], "rb");
        if (!wcd_ctx.scriptf)
        {
            fprintf(stdout, "Can't open script file %s\n", argv[i]);
            return -1;
        }
    }

    if (!(wcd_ctx.g_mode & MODE_ITM))
    {
        read_common_trace();
    }
    else
    {
        read_itm_trace();
    }

    fclose(wcd_ctx.tracef);
    fclose(wcd_ctx.dumpf);
#ifdef DEBUG_DUMP_RAW
    fclose(wcd_ctx.fraw);
#endif

    fprintf(stdout, "Exiting\n");
    close_serial();

    return 0;
}

static void read_common_trace()
{
    int eof = 0;
    int ret = 0;
    int rd = 0;
    int shift = 0;
    unsigned file_off = 0;

    /* to catch corruption */
    memset(big_buf, 0xeb, sizeof(big_buf));
    *big_buf = 0;
    while (!eof)
    {
        /* read sig + 2 header bytes */
        while (shift != 4)
        {
            ret = port_read(wcd_ctx.comf, buf + shift, 4 - shift, wcd_ctx.tracef);
            if (ret < 0)
            {
                eof = 1;
                break;
            }
#ifdef DEBUG_RESYNC
            {
                int ii;
                fprintf(wcd_ctx.fdump, "raw in: \n");
                for (ii = 0 ; ii < ret ; ++ii)
                {
                    fprintf(wcd_ctx.fdump, "%02x ", buf[ii + shift]);
                }
                fprintf(wcd_ctx.fdump, "\n");
            }
#endif
            shift += ret;
            file_off += ret;
        }

        /* sync with signature and header */

        /* First is signature, then header: 1 byte length, 1 byte type.
         */
        if (!(buf[0] == 0xde && buf[1] == 0xad &&
                /* Sure length is > 4 (header itself has 4 bytes - see zb_mac_transport_hdr_t). */
                buf[2] > 4 &&
                /* ZB_MAC_TRANSPORT_TYPE_DUMP | ZB_MAC_TRANSPORT_TYPE_TRACE, dump type can be with high bit
                 * set to 0x01 0x81. Dump type also can contain the used Zigbee channel.
                 * +0 = unkwnown channel
                 * +1 = 11th channel
                 * ...
                 * +16 = channel #26 */
                (buf[3] == 0 || buf[3] == 2 || (0x81 <= buf[3] && buf[3] <= 0x9b) || (0x01 <= buf[3] && buf[3] <= 0x14))))
        {
            /* resyncronize: skip first byte, read 1 more byte */
#ifdef DEBUG_RESYNC
            fprintf(wcd_ctx.fdump, "skip bad %02x\n", buf[0]);
#endif
            memmove(&buf[0], &buf[1], 3);
            shift = 3;
            /* go to 4-th byte read */
            continue;
        }
        if (wcd_ctx.g_mode & MODE_RAWSERIAL)
        {
            /* If getting raw logs from Serial port, write signature too */
            fwrite(buf, 1, 2, wcd_ctx.tracef);
            fflush(wcd_ctx.tracef);
        }

        /* skip signature */
#ifdef DEBUG_RESYNC
        fprintf(wcd_ctx.fdump, "skip sig %02x %02x\n", buf[0], buf[1]);
#endif
        memmove(&buf[0], &buf[2], 2);

        /* 2 bytes of header+body are already read */
        rd = 2;

        while (!eof && rd < buf[0])
        {
            ret = port_read(wcd_ctx.comf, buf + rd, buf[0] - rd, wcd_ctx.tracef);
            if (ret < 0)
            {
                eof = 1;
            }
            else
            {
#ifdef DEBUG_RESYNC
                {
                    int ii;
                    fprintf(wcd_ctx.fdump, "raw in: \n");
                    for (ii = 0 ; ii < ret ; ++ii)
                    {
                        fprintf(wcd_ctx.fdump, "%02x ", buf[ii + rd]);
                    }
                    fprintf(wcd_ctx.fdump, "\n");
                }
#endif
                rd += ret;
                file_off += ret;
            }
        }
        shift = 0;

#ifdef DEBUG_RESYNC
        {
            static int pkt_n = 0;
            int ii;
            fprintf(wcd_ctx.fdump, "pkt %d len %d %s\n", pkt_n, buf[0], buf[1] != 2 ? "TRAFFIC" : "");
            pkt_n++;
            for (ii = 0 ; ii < buf[0] ; ++ii)
            {
                fprintf(wcd_ctx.fdump, "%02x ", buf[ii]);
            }
            fprintf(wcd_ctx.fdump, "\n\n");
        }
#endif

        if (!eof)
        {
            if (!(wcd_ctx.g_mode & MODE_RAWSERIAL))
            {
                if (buf[1] == 0 &&
                        wcd_ctx.scriptf &&
                        IS_SCRIPTING(wcd_ctx.g_mode)) /* Console */
                {
                    if ('$' == buf[4])
                    {
                        unsigned char got_line;
                        unsigned int len;
                        unsigned int pos;

                        fprintf(stdout, "Received console invintation\n");

                        got_line = 0;
                        while (!(feof(wcd_ctx.scriptf) || got_line))
                        {
                            if (NULL == fgets((void *)big_buf, sizeof(big_buf), wcd_ctx.scriptf))
                            {
                                continue;
                            }

                            len = strlen((char *)big_buf);

                            /* Ignore empty and commented strings */
                            if (len == 0 || big_buf[0] == '#' ||
                                    (len == 2 && big_buf[0] == '\r' && big_buf[1] == '\n') ||
                                    (len == 1 && big_buf[1] == '\r'))
                            {

                                continue;
                            }

                            fprintf(stdout, "DEBUG: len=%d, command:%s\n", len, big_buf);
                            got_line = 1;
                            if (len == sizeof(big_buf))
                            {
                                len--;
                            }
                            big_buf[len] = 0xA;
                            len++;

                            pos = 0;

                            while (pos < len && !eof)
                            {
#ifndef ZB_PLATFORM_LINUX_PC32
                                Sleep(10);
                                if (!WriteFile(wcd_ctx.comf, (char *)(big_buf + pos), 1, (void *)&ret, 0) || ret == 0)
#else
                                ret = write(wcd_ctx.comf, big_buf + pos, 1);
                                if (ret == -1 || ret == 0)
#endif
                                {
                                    eof = 1;
                                }

                                pos++;
                            } /* while() */
                        }

                        *big_buf = 0;
                        if (got_line == 0)
                        {
                            fprintf(stdout, "Script file EOF, ignore\n");
                        }
                    }
                    else
                    {
                        fprintf(stdout, "Unexpected console invintation\n");
                    }
                }
                else if (buf[1] == 2) /* trace */
                {
                    buf[buf[0]] = 0;
                    if ((!IS_BINARY(wcd_ctx.g_mode)
                            && buf[buf[0] - 1] != '\n') || *big_buf != 0)
                    {
                        /* buffer can be fragmented. Text trace has \n at line end. */
                        strcat((char *)big_buf, (char *)buf + 4);
                    }
                    if (IS_BINARY(wcd_ctx.g_mode)
                            || buf[buf[0] - 1] == '\n')
                    {
                        if (*big_buf)
                        {
                            parse_trace_line((char *)big_buf, strlen((char *)big_buf), wcd_ctx.tracef);
                            memset(big_buf, 0xeb, sizeof(big_buf));
                            *big_buf = 0;
                        }
                        else
                        {
                            /* skip header, with timestamp */
                            if (IS_BINARY(wcd_ctx.g_mode))
                            {
                                /* if binary, get time from mac transport hdr */
                                parse_trace_line((char *)buf + 2, buf[0] - 2, wcd_ctx.tracef);
                            }
                            else
                            {
                                /* if text, skip mac transport hdr */
                                parse_trace_line((char *)buf + 4, buf[0] - 4, wcd_ctx.tracef);
                            }
                        }
                    }
                    fflush(wcd_ctx.tracef);
                }
                else
                {
                    /* dump */
                    fwrite(buf, 1, buf[0], wcd_ctx.dumpf);
                    fflush(wcd_ctx.dumpf);
                }
            }
        }
    } /* while */
}


#if 0
/**
   Try to convert dump file into pcap.

   It is not trivial to compile dump_converter, so use externally compiled one.
 */
static void convert_dumpf(char *name)
{
    char pcapname[256];
    char *p;

    strcpy(pcapname, name);
    p = strstr(pcapname, ".dump");
    if (p)
    {
        strcpy(p, ".pcap");
    }
    else
    {
        strcat(pcapname, ".pcap");
    }
    errno = 0;
    /* suppose have it in the path */

#ifndef ZB_PLATFORM_LINUX_PC32
    _execl("dump_converter.exe", "dump_converter.exe", name, pcapname, NULL);
#else
    execl("dump_converter", "dump_converter", name, pcapname, NULL);
#endif

    if (errno == ENOENT)
    {
        /* suppose we are at the sources tree root */

#ifndef ZB_PLATFORM_LINUX_PC32
        _execl("devtools\\dump_converter\\dump_converter.exe", "dump_converter.exe", name, pcapname, NULL);
#else
        execl("devtools/dump_converter/dump_converter", "dump_converter", name, pcapname, NULL);
#endif

    }
}
#endif


static int open_serial(char *name)
{
#ifndef ZB_PLATFORM_LINUX_PC32
    DCB dcb;
    DWORD BaudRate = BAUD_RATE;

    if (wcd_ctx.g_mode & MODE_USARTLOGSBIN_XAP)
    {
        BaudRate = XAP_BAUD_RATE;
    }
    if (wcd_ctx.g_mode & MODE_NORDIC_LOG && !(wcd_ctx.g_mode & MODE_NORDIC_LOG_115200))
    {
        BaudRate = NORDIC_BAUD_RATE;
    }
    if (wcd_ctx.g_mode & MODE_ITM)
    {
        BaudRate = BAUD_RATE_ITM;
    }
    memset(&dcb, 0, sizeof(DCB));

    do
    {
        char portname[512];
        if (strchr(name, '\\') || !IS_USING_SERIAL(wcd_ctx.g_mode))
        {
            strcpy(portname, name);
        }
        else
        {
            strcpy(portname, "\\\\.\\");
            strcat(portname, name);
        }
        wcd_ctx.comf = CreateFileA(portname, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (wcd_ctx.comf == INVALID_HANDLE_VALUE)
        {
            if (!IS_USING_SERIAL(wcd_ctx.g_mode))
            {
                fprintf(stdout, "Can't open serial port %s : %d\n", name, GetLastError());
                return -1;
            }
            else
            {
                /* When work with Cortex and use serial over USB, com port appears only
                 * after program start. retry opening. */
                fprintf(stdout, "\rRetrying open... ^C to cancel");
            }
        }
    } while (IS_USING_SERIAL(wcd_ctx.g_mode)
             && wcd_ctx.comf == INVALID_HANDLE_VALUE);

    if (IS_USING_SERIAL(wcd_ctx.g_mode))
    {
        GetCommState(wcd_ctx.comf, &dcb);
        dcb.BaudRate = BaudRate;
        dcb.DCBlength = sizeof(DCB);
        dcb.ByteSize = 8;
        dcb.StopBits = ONESTOPBIT;
        dcb.fOutxDsrFlow = 0;
        dcb.fDtrControl = DTR_CONTROL_DISABLE;
        dcb.fOutxCtsFlow = 0;
        dcb.fRtsControl = RTS_CONTROL_DISABLE;
        dcb.fInX = dcb.fOutX = 0;
        //  Set other stuff
        dcb.fBinary = TRUE;
        dcb.fParity = FALSE;
        dcb.fDsrSensitivity = 0;
        dcb.fTXContinueOnXoff = FALSE;
        dcb.fNull = 0;
        dcb.fAbortOnError = 0;
        dcb.Parity = NOPARITY;
        dcb.fErrorChar = 0;
        dcb.ErrorChar = 0;

        while (1)
        {
            if (!SetCommState(wcd_ctx.comf, &dcb))
            {
                if (!((wcd_ctx.g_mode & MODE_IARARM) || (wcd_ctx.g_mode & MODE_JTAGLOGSBIN)))
                {
                    fprintf(stdout, "SetCommState error %d\n", GetLastError());
                    return -1;
                }
                else
                {
                    fprintf(stdout, "\rRetrying SetCommState err %d... ^C to cancel", GetLastError());
                }
            }
            else
            {
                COMMTIMEOUTS timeouts;
                fprintf(stdout, "\nSetCommState ok, wcd_ctx.comf %x\n", wcd_ctx.comf);

                timeouts.ReadIntervalTimeout = 0;
                timeouts.ReadTotalTimeoutMultiplier = 0;
                timeouts.ReadTotalTimeoutConstant = 0;
                timeouts.WriteTotalTimeoutMultiplier = 0;
                timeouts.WriteTotalTimeoutConstant = 0;
                SetCommTimeouts(wcd_ctx.comf, &timeouts);

                break;
            }
        }
    }
#else
    int fcntl_return_code;
    struct termios term;

    do
    {
        wcd_ctx.comf = open(name, O_RDWR | O_NOCTTY/* | O_NONBLOCK*/);
        if (wcd_ctx.comf == -1)
        {
            if (!IS_USING_SERIAL(wcd_ctx.g_mode))
            {
                fprintf(stdout, "Can't open serial port %s : %d\n", name, GetLastError());
                return -1;
            }
            else
            {
                /* When work with Cortex and use serial over USB, com port appears only
                 * after program start. retry opening. */
                fprintf(stdout, "\rRetrying open... ^C to cancel");
            }
        }
    } while (IS_USING_SERIAL(wcd_ctx.g_mode) && wcd_ctx.comf == -1);

    if (IS_USING_SERIAL(wcd_ctx.g_mode) && isatty(wcd_ctx.comf))
    {
        if (tcgetattr(wcd_ctx.comf, &term) < 0)
        {
            fprintf(stdout, "\ntcgetattr error.\n");
            return -1;
        }
        cfmakeraw(&term);
        cfsetospeed(&term, B115200);
        cfsetispeed(&term, B115200);
        term.c_cflag &= ~(CSIZE | PARENB);
        term.c_cflag |= CS8 | CREAD | HUPCL | CLOCAL;
        if (tcsetattr(wcd_ctx.comf, TCSANOW, &term))
        {
            fprintf(stdout, "\ntcsetattr error.\n");
            return -1;
        }
        if ((fcntl_return_code = fcntl(wcd_ctx.comf, F_GETFL, 0)) == -1)
        {
            fprintf(stdout, "\nfcntl get error.\n");
            return -1;
        }
        fcntl_return_code &= ~O_NONBLOCK;
        if (fcntl(wcd_ctx.comf, F_SETFL, fcntl_return_code) == -1)
        {
            fprintf(stdout, "\nfcntl set error.\n");
            return -1;
        }
    }
#endif

    return 0;
}


static void close_serial()
{
#ifndef ZB_PLATFORM_LINUX_PC32
    CloseHandle(wcd_ctx.comf);
#else
    close(wcd_ctx.comf);
#endif
}


static int port_read_os(serial_handle_t comf, void *buf, int n)
{
    int ret = 0;
    int val = 0;
#ifndef ZB_PLATFORM_LINUX_PC32
    BOOL rc = ReadFile(comf, buf, n, (unsigned *)&ret, 0);
    if ((rc && ret == 0 && !IS_USING_SERIAL(wcd_ctx.g_mode))
            || (!rc && GetLastError() != ERROR_IO_PENDING))
    {
        fprintf(stdout, "oops - read error; rc %d ret %d err %d\n", rc, ret, GetLastError());
        ret = -1;
    }
#else
    do
    {
        errno = 0;
        ret = read(comf, buf, n);
    } while (ret == -1 && errno == EAGAIN);
    if (ret == -1 || ret == 0)
    {
        ret = -1;
    }
#endif
#ifdef DEBUG_DUMP_RAW
    if (ret > 0)
    {
        fwrite(buf, 1, ret, wcd_ctx.fraw);
        fflush(wcd_ctx.fraw);
    }
#endif
#if 0
    if (n > 0 && n < 5)
    {
        val = *((int *)buf);
        fprintf(stdout, "port_read_os(), val == %x, ret == %d\n", val, ret);
    }
#endif
    return ret;
}


static int port_read_hex(serial_handle_t comf, void *buf, int maxn);

int port_read(serial_handle_t comf, void *buf, int n, FILE *outf)
{
    if (!(wcd_ctx.g_mode & MODE_NORDIC_LOG))
    {
        if (!(wcd_ctx.g_mode & MODE_HEX))
        {
            return port_read_os(comf, buf, n);
        }
        else
        {
            return port_read_hex(comf, buf, n);
        }
    }
    else
    {
        return read_nordic_log(comf, buf, n, outf);
    }
}

static int read_nordic_log(serial_handle_t comf, void *buf, int maxn, FILE *outf)
{
    char prefix[] = "<info> zboss: ";
    static char bin_buffer[1024];
    static char line_buffer[1024];
    static char *line_buffer_p = line_buffer;
    static char bin_buffer_off = 0;
    static int bin_buffered = 0;
    static int line_buffered = 0;
    int n;
    char *nl;
    char *p;

    while (1)
    {
        /* Have something binry ready - return it. */
        if (bin_buffered > 0)
        {
            if (maxn > bin_buffered)
            {
                maxn = bin_buffered;
            }
            memcpy(buf, bin_buffer + bin_buffer_off, maxn);
            bin_buffered -= maxn;
            bin_buffer_off += maxn;
            if (bin_buffered == 0)
            {
                bin_buffer_off = 0;
            }
            return maxn;
        }

        nl = NULL;
        if (line_buffered)
        {
            nl = strchr(line_buffer_p, '\n');
        }
        if (!nl)
        {
            /* No nl - read more data. Normalize before read. */
            if (line_buffered == sizeof(line_buffer) - 1)
            {
                /* Buffer is full of some garbage. Skip it. */
                line_buffered = 0;
                line_buffer_p = line_buffer;
            }
            if (line_buffered)
            {
                memmove(line_buffer, line_buffer_p, line_buffered);
                line_buffer_p = line_buffer;
            }
            n = port_read_os(comf, line_buffer + line_buffered, sizeof(line_buffer) - line_buffered - 1);
            if (n < 0)
            {
                return n;
            }
            line_buffered += n;
            line_buffer[line_buffered] = 0;
        }

        if (nl)
        {
            /* string len, with trailing \n */
            unsigned v;
            int llen = nl - line_buffer_p + 1;

            if (llen > (int)strlen(prefix)
                    && !memcmp(prefix, line_buffer_p, strlen(prefix)))
            {
                /* skip trailer */
                p = strchr(line_buffer_p, '|');
                if (p)
                {
                    *p = 0;
                }
                line_buffer_p += strlen(prefix);
                while (*line_buffer_p
                        && sscanf(line_buffer_p, "%x%n", &v, &n) > 0)
                {
                    bin_buffer[bin_buffered] = v;
                    bin_buffered++;
                    line_buffer_p += n;
                }
            }
            else
            {
                /* Put into trace Nordic line as is. */
                fwrite(line_buffer_p, 1, nl - line_buffer_p + 1, outf);
            }
            line_buffer_p = nl + 1;
            line_buffered -= llen;
        } /* if nl */
    } /* while 1 */
}


static int port_read_hex(serial_handle_t comf, void *buf, int maxn)
{
    unsigned char *p = (unsigned char *)buf;
    static int state = 0;
    static unsigned char v = 0;
    int n;
    unsigned char c;

    while (maxn)
    {
        n = port_read_os(comf, &c, 1);
        if (n != 1)
        {
            break;
        }
        switch (state)
        {
        case 0:
            v = 0;
        /* FALLTHROUGH */
        case 1:
            if (c >= '0' && c <= '9')
            {
                v |= (c - '0') << ((!state) * 4);
                state++;
            }
            else
            {
                c = toupper(c);
                if (c >= 'A' && c <= 'F')
                {
                    v |= (c - 'A' + 10) << ((!state) * 4);
                    state++;
                }
                else
                {
                    state = 0;
                }
            }
            break;

        case 2:
            if (c == ' ')
            {
                *p = v;
                p++;
                maxn--;
            }
            state = 0;
            break;
        }
    }
    if (n <= 0 && (p - ((unsigned char *)buf)) == 0)
    {
        return -1;
    }
    else
    {
        return (p - ((unsigned char *)buf));
    }
}
