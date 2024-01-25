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
/* PURPOSE: Parse trace file
*/

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dump_common.h"
#include "win_com_dump_common.h"

#ifndef ZB_PLATFORM_LINUX_PC32
#include <windows.h>
#else
#include <dirent.h>
#include <errno.h>
#include <glob.h>
#include <stdbool.h>
#include "lin_com_dump.h"
#endif

#define FILE_NAME_LEN 256
#define N_FILES 300
#define N_TRACE_LINES_PER_FILE 500

typedef struct cache_line_s
{
    int line1;
    int line2;
    char *fmt;
} cache_line_t;

typedef struct file_cache_s
{
    char file_name[FILE_NAME_LEN];
    int id;
    int n_lines;
    cache_line_t lines[N_TRACE_LINES_PER_FILE];
} file_cache_t;


int n_files;
file_cache_t files[N_FILES];

static int get_file_cache(char *name, file_cache_t **fc);
static int get_format(file_cache_t *fc, int line, char **fmt);
static void parse_dump(char *format, char *s, FILE *f);
static int get_bytes(char *s, unsigned char *buf, int n_bytes);
static int get_bytes_be(char *s, unsigned char *buf, int n_bytes);
static void load_file(FILE *f, file_cache_t *fc);
static FILE *open_source_file(char *name);
static FILE *open_source_recursive(char *name, char *root);
static void parse_trace_line_bin(char *str, int len, FILE *outf);
static void parse_trace_line_text(char *str, FILE *outf);

static FILE *open_source_file_byid(int id, char *name);
static FILE *open_source_recursive_byid(int id, char *name, char *root);
static FILE *seek_id_in_c_file(char *name, int id);
static int get_file_cache_byid(int id, file_cache_t **fc);

extern win_com_dump_ctx_t wcd_ctx;

void parse_trace_line(char *str, int line_len, FILE *outf)
{
    if (IS_BINARY(wcd_ctx.g_mode))
    {
        parse_trace_line_bin(str, line_len, outf);
    }
    else
    {
        parse_trace_line_text(str, outf);
    }
}


static void parse_trace_line_text(char *str, FILE *outf)
{
    char file[FILE_NAME_LEN];
    int line;
    int off;
    file_cache_t *fc;
    char *fmt;
    char *p;

    sscanf(str, "%*d%*d%*[ ]%s%n", file, &off);
    p = strrchr(file, ':');
    if (p)
    {
        *p = 0;
        p++;
        sscanf(p, "%d", &line);
    }
    if (get_file_cache(file, &fc))
    {
        fputs(str, outf);
        return;
    }
    if (get_format(fc, line, &fmt))
    {
        fputs(str, outf);
        return;
    }

    //fputs(str, outf);
    fwrite(str, off + 1, 1, outf);
    parse_dump(fmt, str + off + 1, outf);
    fflush(outf);

}


static void parse_trace_line_bin(char *str, int len, FILE *outf)
{
    unsigned short counter;
    unsigned short time_be;
    unsigned long long time_us;
    unsigned short file_id;
    unsigned short line;
    int extra_tab = 0;
    //  int len_steps = 0;

    file_cache_t *fc;
    char *fmt;

    /* See zb_trace_msg_port in zb_serial_trace_bin.c */

    /* counter time fileid line args */
    memcpy(&time_be, str, 2);     /* get time from mac transport hdr */
    memcpy(&counter, str + 2, 2);
    memcpy(&file_id, str + 4, 2);
    memcpy(&line, str + 6, 2);


    time_us = time_be * 15360;
    fprintf(outf, "% 4d\t% 4d/%d.%06d\t",
            counter, time_be,
            (unsigned int)(time_us / 1000000),
            (unsigned int)(time_us % 1000000));

    if (get_file_cache_byid(file_id, &fc))
    {
        fprintf(outf, "%d:", file_id);
    }
    else
    {
        fprintf(outf, "%s:", fc->file_name);
        if (strlen(fc->file_name) + 1 + log10(line) < 15)
        {
            extra_tab = 1;
        }
    }
    fprintf(outf, "%d\t", line);
    if (extra_tab)
    {
        fprintf(outf, "\t");
    }

    if (fc && !get_format(fc, line, &fmt))
    {
        parse_dump(fmt, str + 8, outf);
    }
    else
    {
        int i;

        /* if no format, do raw dump */
        for (i = 8 ; i < len ; ++i)
        {
            fprintf(outf, "%02x ", ((unsigned)str[i]) & 0xff);
        }
        fprintf(outf, "\n");
    }
    fflush(outf);
}


static int get_file_cache(char *name, file_cache_t **fc)
{
    int i;
    FILE *f;

    for (i = 0 ; i < n_files ; ++i)
    {
        if (!strcmp(name, files[i].file_name))
        {
            *fc = &files[i];
            return 0;
        }
    }
    *fc = &files[i];
    f = open_source_file(name);
    if (f)
    {
        strcpy((*fc)->file_name, name);
        load_file(f, &files[i]);
        fclose(f);
        n_files++;
        return 0;
    }
    else
    {
        return -1;
    }
}


static int get_file_cache_byid(int id, file_cache_t **fc)
{
    int i;
    FILE *f;

    for (i = 0 ; i < n_files ; ++i)
    {
        if (files[i].id == id)
        {
            *fc = &files[i];
            return 0;
        }
    }
    *fc = &files[i];

    f = open_source_file_byid(id, files[i].file_name);
    if (f)
    {
        load_file(f, &files[i]);

#ifndef ZB_PLATFORM_LINUX_PC32
        fclose(f);
#endif

        files[i].id = id;
        n_files++;
        return 0;
    }
    else
    {
        return -1;
    }
}


static int get_format(file_cache_t *fc, int line, char **fmt)
{
    int i;
    for (i = 0 ; i < fc->n_lines ; ++i)
    {
        if (fc->lines[i].line1 <= line && fc->lines[i].line2 >= line)
        {
            *fmt = fc->lines[i].fmt;
            return 0;
        }
    }
    return -1;
}


/* trace function which does not use printf() */

#define PRINTU_MACRO(v, f)                      \
{                                               \
  char s[10];                                   \
  int i = 10;                                   \
  do                                            \
  {                                             \
    s[--i] = '0' + (v) % 10;                    \
    v /= 10;                                    \
  }                                             \
  while (v);                                    \
  while (i < 10)                                \
  {                                             \
    putc(s[i], f);                              \
    i++;                                        \
  }                                             \
}

static char s_x_tbl[] = "0123456789abcdef";

static void printx2(unsigned char v, FILE *f)
{
    putc(s_x_tbl[((v) >> 4) & 0xf], f);
    putc(s_x_tbl[((v) & 0xf)], f);
}

#define PRINTX(v, f)                            \
{                                               \
  char s[10];                                   \
  int i = 10;                                   \
  do                                            \
  {                                             \
    s[--i] = s_x_tbl[(v) & 0xf];                \
    (v) >>= 4;                                  \
  }                                             \
  while (v);                                    \
  while (i < 10)                                \
  {                                             \
    putc(s[i], f);                              \
    i++;                                        \
  }                                             \
}

/**
   Output trace message.

   @param file_name - source file name
   @param line_number - source file line
   @param mask - layers mask of the current message. Do trace if mask&ZB_TRACE_MASK != 0
   @param level - message trace level. Do trace if level <= ZB_TRACE_LEVEL
   @param format - printf-like format string
 */

static void parse_dump(char *format, char *s, FILE *f)
{
    if (!format || !(*format))
    {
        putc('\n', f);
    }
    else
    {
        /*
          Implement printf-like formatting.

          Understands %u, %d, %i, %c, %x, %s, %p, %ld, %lu, %lx.
          Not so complex as SDCC's printf_large but in differs with its printf_small
          understands %p. Do not understand %o - we never use it.
        */
        for ( ; *format ; format++)
        {
            int l;
            int h;
            int u = 0;
            if (*format != '%')
            {
                putc(*format, f);
                continue;
            }
            format++;
            l = (*format == 'l');
            h = (*format == 'h');
            if (l || h)
            {
                format++;
            }
            if (IS_IARARM(wcd_ctx.g_mode))
            {
                /* In case of Cortex-M4 all integers are 4-bytes length */
                l = 1;
            }
            if (TWO_BYTES_H(wcd_ctx.g_mode))
            {
                /* At XAP and 8051 %h is passed as 2 bytes - same as %d */
                /* CR: 04/23/2015 [EE] Sure at 8051/Keil %h is passed as 1
                 * byte. */
                /* CR: 04/23/2015 [DT] Sorry, I meant 8051/IAR combination here
                 * zb_minimal_vararg_t is 2 bytes for IAR and 1 for KEIL
                 */
                h = 0;
            }
            switch (*format)
            {
            case 'u':
                u = 1;
            /* FALLTHROUGH */
            case 'd':
            case 'i':
                if (l)
                {
                    unsigned int v;
                    s += get_bytes_be(s, (unsigned char *)&v, 4);
                    if (!u && (int)v < 0)
                    {
                        putc('-', f);
                        v = -((int)v);
                    }
                    PRINTU_MACRO(v, f);
                }
                else if (h)
                {
                    unsigned char v;
                    s += get_bytes(s, &v, 1);
                    if (!u && (signed char)v < 0)
                    {
                        putc('-', f);
                        v = -((signed char)v);
                    }
                    PRINTU_MACRO(v, f);
                }
                else
                {
                    unsigned short v;
                    s += get_bytes_be(s, (unsigned char *)&v, 2);
                    if (!u && (short)v < 0)
                    {
                        putc('-', f);
                        v = -((short)v);
                    }
                    PRINTU_MACRO(v, f);
                }
                continue;
            case 'x':
                if (l)
                {
                    unsigned int v;
                    s += get_bytes_be(s, (unsigned char *)&v, 4);
                    PRINTX(v, f);
                }
                else if (h)
                {
                    unsigned char v;
                    s += get_bytes(s, &v, 1);
                    PRINTX(v, f);
                }
                else
                {
                    unsigned short v;
                    s += get_bytes_be(s, (unsigned char *)&v, 2);
                    PRINTX(v, f);
                }
                continue;
            case 'c':
            {
                unsigned char v;
                s += get_bytes(s, &v, 1);
                putc(v, f);
                continue ;
            }
            case 'p':
            {
                unsigned char b[4];
                /* Needed to be able to work with multiple options */
                if (wcd_ctx.g_mode & MODE_KEIL)
                {
                    s += get_bytes(s, b, 3);
                    printx2(b[0], f);
                    printx2(b[1], f);
                    printx2(b[2], f);
                }
                else if ((wcd_ctx.g_mode & MODE_IAR51)
                         /* we use XAP near - 2 bytes ptrs */
                         || IS_XAP(wcd_ctx.g_mode))
                {
                    /* IAR/8051 uses 2-bytes pointers */
                    s += get_bytes(s, b, 2);
                    printx2(b[0], f);
                    printx2(b[1], f);
                }
                else if (IS_IARARM(wcd_ctx.g_mode))
                {
                    /* IAR/ARM uses 4-bytes pointers, little endian */
                    s += get_bytes(s, b, 4);
                    printx2(b[3], f);
                    printx2(b[2], f);
                    printx2(b[1], f);
                    printx2(b[0], f);
                }
                else
                {
                    /* any other, including XAP FAR - 4-bytes pointers. TODO: XAP NEAR */
                    s += get_bytes(s, b, 4);
                    printx2(b[3], f);
                    printx2(b[2], f);
                    printx2(b[1], f);
                    printx2(b[0], f);
                }
            }
            continue;

            case 'A':
            {
                int i;
                unsigned char b[8];
                s += get_bytes(s, b, 8);
                if (IS_IARARM(wcd_ctx.g_mode) || IS_XAP(wcd_ctx.g_mode))
                {
                    for (i = 7 ; i >= 0 ; --i)
                    {
                        printx2(b[i], f);
                        if (i != 0)
                        {
                            putc(':', f);
                        }
                    }
                }
                else
                {
                    for (i = 0 ; i < 8 ; ++i)
                    {
                        printx2(b[i], f);
                        if (i < 7)
                        {
                            putc(':', f);
                        }
                    }
                }
                continue ;
            }

            case 'B':
            {
                int i;
                unsigned char b[16];
                s += get_bytes(s, b, 16);
                for (i = 0 ; i < 16 ; ++i)
                {
                    printx2(b[i], f);
                    if (i < 15)
                    {
                        putc(':', f);
                    }
                }

                continue ;
            }

            case 's':
            {
                fprintf(f, "%%s is not supported!");
                continue;
            }
            }
        }
        if (format[-1] != '\n')
        {
            putc('\n', f);
        }
    }
}


static int get_bytes(char *s, unsigned char *buf, int n_bytes)
{
    int i;

    if (IS_BINARY(wcd_ctx.g_mode))
    {
        memcpy(buf, s, n_bytes);
        return n_bytes;
    }
    else
    {
        for (i = 0 ; i < n_bytes ; ++i)
        {
            int v;
            sscanf(s, "%02x", &v);
            buf[i] = v;
            s += 2;
        }

        /* IAR casts bytes to 16-bit integer when pass it as varargs. Also, IAR is
         * little-endian, so can safely skip second byte. Note that for text
         * representation 1 binary byte == 2 text bytes */
        return (n_bytes + ((wcd_ctx.g_mode & MODE_IAR51) && (n_bytes == 1))) * 2;
    }
}


static int get_bytes_be(char *s, unsigned char *buf, int n_bytes)
{
    if (IS_BE(wcd_ctx.g_mode))
    {
        int i = n_bytes - 1;
        while (i >= 0)
        {
            int v;
            sscanf(s, "%02x", &v);
            buf[i] = v;
            i--;
            s += 2;
        }

        return n_bytes * 2;
    }
    else
    {
        /* IAR is little-endian while Keil is big-endian */
        return get_bytes(s, buf, n_bytes);
    }
}



static void load_file(FILE *f, file_cache_t *fc)
{
    char buf[512];
    int n_lines = 0;
    int line = 0;
    char *p;
    char *p_end;
    char *p_end2;
    char *p_o;
    int skip;
    int trace_msg = 0;

    rewind(f);
    while (fgets(buf, sizeof(buf), f))
    {
        line++;
        if (strstr(buf, "TRACE_MSG"))
        {
            trace_msg = 1;
            fc->lines[n_lines].line1 = line;
        }
        if (trace_msg
                && ((p = strchr(buf, '\"'))
                    || (p = strstr(buf, "TRACE_FORMAT_"))))
        {
            trace_msg = 0;
            p_end2 = strstr(p + 1, "TRACE_FORMAT_");
            while (p_end2)
            {
                p_end = strstr(p_end2 + 1, "TRACE_FORMAT_");
                if (p_end)
                {
                    p_end2 = p_end;
                }
                else
                {
                    break;
                }
            }

            p_end = strrchr(buf, '\"');

            if (!p_end || p_end == p || (p_end && p_end2 && p_end2 > p_end))
            {
                if (!p_end2)
                {
                    continue;
                }
                p_end = p_end2 + strlen("TRACE_FORMAT_64");
                if (!strncmp(p_end2, "TRACE_FORMAT_128", 16))
                {
                    p_end++;
                }
            }
            p++;

            fc->lines[n_lines].fmt = (char *)malloc(p_end - p + 1);

            p_o = fc->lines[n_lines].fmt;
            skip = 0;
            while (p != p_end)
            {
                if (!strncmp(p, "TRACE_FORMAT_64", 15))
                {
                    *p_o++ = '%';
                    *p_o++ = 'A';
                    p += 15;
                    skip = !skip;
                    continue;
                }
                if (!strncmp(p, "TRACE_FORMAT_128", 16))
                {
                    *p_o++ = '%';
                    *p_o++ = 'B';
                    p += 16;
                    skip = !skip;
                    continue;
                }
                if (*p == '\"')
                {
                    skip = !skip;
                }
                if (!skip)
                {
                    *p_o++ = *p;
                }
                p++;
            }
            *p_o = 0;

            if (!strchr(p_end, ';'))
            {
                while (!strchr(buf, ';'))
                {
                    if (!fgets(buf, sizeof(buf), f))
                    {
                        break;
                    }
                    line++;
                }
            }
            fc->lines[n_lines].line2 = line;

            n_lines++;
        }
        fc->n_lines = n_lines;
    }

    fclose(f);
}


/**
   Try to open source file.

   If name has no path and file can't be open, search in subdirectories.
 */
static FILE *open_source_file(char *name)
{
    FILE *f;
    if (!strchr(name, '/') && !strchr(name, '\\'))
    {
        /* name without path - search in subdirs */

#ifndef ZB_PLATFORM_LINUX_PC32
        f = open_source_recursive(name, ".");
#else
        f = open_source_recursive(name, "");
#endif

    }
    else
    {
        f = fopen(name, "r");
    }
    return f;
}


static FILE *open_source_recursive(char *name, char *root)
{
#ifndef ZB_PLATFORM_LINUX_PC32
    HANDLE h;
    WIN32_FIND_DATAA find_data;
    BOOL valid = TRUE;
#else
    glob_t p_glob;
    unsigned glob_index;
#endif

    char pattern[512];
    FILE *f;

    strcpy(pattern, root);

#ifndef ZB_PLATFORM_LINUX_PC32
    strcat(pattern, "\\");
#endif

    strcat(pattern, name);
    f = fopen(pattern, "r");

    if (f)
    {
        return f;
    }

    strcpy(pattern, root);

#ifndef ZB_PLATFORM_LINUX_PC32
    strcat(pattern, "\\*.*");
    for (h = FindFirstFileA(pattern, &find_data);
            f == NULL && h != INVALID_HANDLE_VALUE && valid ;
            valid = FindNextFileA(h, &find_data))
    {
        if (strcmp(find_data.cFileName, ".svn")
                && strcmp(find_data.cFileName, ".")
                && strcmp(find_data.cFileName, "..")
                && strcmp(find_data.cFileName, "mk")
                && find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            strcpy(pattern, root);
            strcat(pattern, "\\");
            strcat(pattern, find_data.cFileName);
            f = open_source_recursive(name, pattern);
        }
    }
    if (h != INVALID_HANDLE_VALUE)
    {
        FindClose(h);
    }
#else
    strcat(pattern, "*");
    glob(pattern, GLOB_MARK, NULL, &p_glob);
    for (glob_index = 0; glob_index < p_glob.gl_pathc; ++glob_index)
    {
        if (f == NULL &&
                strcmp(p_glob.gl_pathv[glob_index], "..") &&
                strcmp(p_glob.gl_pathv[glob_index], ".") &&
                strcmp(p_glob.gl_pathv[glob_index], ".svn") &&
                strcmp(p_glob.gl_pathv[glob_index], "mk") &&
                !strcmp(&p_glob.gl_pathv[glob_index][strlen(p_glob.gl_pathv[glob_index]) - 1], "/"))
        {
            strcpy(pattern, root);
            strcat(pattern, p_glob.gl_pathv[glob_index]);
            f = open_source_recursive(name, pattern);
        }
    }
    globfree(&p_glob);
#endif

    return f;
}


/**
   Try to open source file by its id.
 */
static FILE *open_source_file_byid(int id, char *name)
{
    FILE *f;

#ifndef ZB_PLATFORM_LINUX_PC32
    f = open_source_recursive_byid(id, name, ".");
#else
    f = open_source_recursive_byid(id, name, "");
#endif

    return f;
}


static FILE *open_source_recursive_byid(int id, char *name, char *root)
{
#ifndef ZB_PLATFORM_LINUX_PC32
    HANDLE h = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAA find_data;
#else
    char path[512];
    glob_t p_glob;
    unsigned glob_index;
#endif

    char pattern[512];
    FILE *f = NULL;
#ifndef ZB_PLATFORM_LINUX_PC32
    BOOL valid = TRUE;
#endif

    strcpy(pattern, root);

#ifndef ZB_PLATFORM_LINUX_PC32
    strcat(pattern, "\\*.c");

    for (h = FindFirstFileA(pattern, &find_data);
            f == NULL && h != INVALID_HANDLE_VALUE && valid ;
            valid = FindNextFileA(h, &find_data))
    {
        if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            char path[512];

            strcpy(path, root);
            strcat(path, "\\");
            strcat(path, find_data.cFileName);

            f = seek_id_in_c_file(path, id);
            if (f)
            {
                strcpy(name, find_data.cFileName);
                break;
            }
        }
    }
    if (h != INVALID_HANDLE_VALUE)
    {
        FindClose(h);
        h = INVALID_HANDLE_VALUE;
    }

    if (!f)
    {
        strcpy(pattern, root);
        strcat(pattern, "\\*.*");

        for (h = FindFirstFileA(pattern, &find_data), valid = TRUE;
                f == NULL && h != INVALID_HANDLE_VALUE && valid ;
                valid = FindNextFileA(h, &find_data))
        {
            if (strcmp(find_data.cFileName, ".svn")
                    && strcmp(find_data.cFileName, ".")
                    && strcmp(find_data.cFileName, "..")
                    && strcmp(find_data.cFileName, "mk")
                    && find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                strcpy(pattern, root);
                strcat(pattern, "\\");
                strcat(pattern, find_data.cFileName);
                f = open_source_recursive_byid(id, name, pattern);
            }
        }
    }
    if (h != INVALID_HANDLE_VALUE)
    {
        FindClose(h);
    }
#else
    strcat(pattern, "*.c");
    glob(pattern, GLOB_MARK, NULL, &p_glob);
    for (glob_index = 0; glob_index < p_glob.gl_pathc; ++glob_index)
    {
        if (strcmp(&p_glob.gl_pathv[glob_index][strlen(p_glob.gl_pathv[glob_index]) - 1], "/"))
        {
            strcpy(path, root);
            strcat(path, p_glob.gl_pathv[glob_index]);
            f = seek_id_in_c_file(p_glob.gl_pathv[glob_index], id);
            if (f)
            {
                strcpy(name, p_glob.gl_pathv[glob_index]);
                break;
            }
        }
    }
    globfree(&p_glob);
    if (!f)
    {
        strcpy(pattern, root);
        strcat(pattern, "*");
        glob(pattern, GLOB_MARK, NULL, &p_glob);
        for (glob_index = 0; glob_index < p_glob.gl_pathc; ++glob_index)
        {
            if (f == NULL &&
                    strcmp(p_glob.gl_pathv[glob_index], "..") &&
                    strcmp(p_glob.gl_pathv[glob_index], ".") &&
                    strcmp(p_glob.gl_pathv[glob_index], ".svn") &&
                    strcmp(p_glob.gl_pathv[glob_index], "mk") &&
                    !strcmp(&p_glob.gl_pathv[glob_index][strlen(p_glob.gl_pathv[glob_index]) - 1], "/"))
            {
                strcpy(pattern, p_glob.gl_pathv[glob_index]);
                f = open_source_recursive_byid(id, name, pattern);
            }
        }
        globfree(&p_glob);
    }
#endif

    return f;
}


static FILE *seek_id_in_c_file(char *name, int id)
{
    int iid;
    FILE *f = fopen(name, "r");
    char buf[512];
    char *p;

    while (f && fgets(buf, sizeof(buf), f))
    {
        if ((p = strstr(buf, "ZB_TRACE_FILE_ID")) != NULL)
        {
            iid = 0;
            sscanf(p + strlen("ZB_TRACE_FILE_ID"), "%d", &iid);
            if (iid == id)
            {
                break;
            }
            fclose(f);
            f = NULL;
        }
        /* ZB_TRACE_FILE_ID must be before first #include, so not need to check more */
        else if (strstr(buf, "#include"))
        {
            fclose(f);
            f = NULL;
        }
    }
    if (f && feof(f))
    {
        fclose(f);
        f = NULL;
    }
    return f;
}
