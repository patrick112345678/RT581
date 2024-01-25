/**
 * @file log.c
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief 
 * @version 0.1
 * @date 2023-08-01
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <stdio.h>
#include <string.h>

#ifdef CONFIG_CLI_CMD_ENABLE
#include <cli.h>
#include "shell.h"
#endif

#include "log_type.h"
#include "log_cfg.h"


#include <util_log.h>

#if LOG_TEST
#include <log_testc.h>
#endif

/* every line log's buffer */
static char log_buf[LOG_LINE_BUF_SIZE] = { 0 };

#ifdef CONFIG_CLI_CMD_ENABLE
static int findch_sum(const char *str, char ch)
{
    int i = 0;
    int len = strlen(str);
    int ret = 0;

    for (i = 0; i < len; i++) {
        if (str[i] == ch) {
            ret++;
        }
    }

    return ret;
}

static int set_level(int argc, char **argv)
{
    extern char _ld_rf_static_logcomponent_code_start;
    extern char _ld_rf_static_logcomponent_code_end;
    extern char _ld_rf_static_logfile_code_start;
    extern char _ld_rf_static_logfile_code_end;
    extern char _ld_rf_static_logpri_code_start;
    extern char _ld_rf_static_logpri_code_end;

    log_info_t *start;
    log_info_t *end;
    log_info_t *info;

    int          left;
    int          right;
    uint32_t     mid;
    int          cmp_val;
    log_level_t level;

    int ch_sum;

    if (argc != 3) {
        __log_printf("arg error.\r\n");
        return -1;
    }

    /* get component level */
    if (0 == strcmp(argv[1], "all")) {
        level = LOG_LEVEL_ALL;
    } else if (0 == strcmp(argv[1], "debug")) {
        level = LOG_LEVEL_DEBUG;
    } else if (0 == strcmp(argv[1], "info")) {
        level = LOG_LEVEL_INFO;
    } else if (0 == strcmp(argv[1], "warn")) {
        level = LOG_LEVEL_WARN;
    } else if (0 == strcmp(argv[1], "error")) {
        level = LOG_LEVEL_ERROR;
    } else if (0 == strcmp(argv[1], "assert")) {
        level = LOG_LEVEL_ASSERT;
    } else if (0 == strcmp(argv[1], "never")) {
        level = LOG_LEVEL_NEVER;
    } else {
        __log_printf("input level = %s not support.\r\n", argv[1]);
        return -1;
    }

    /* component_name.file_name.pri_name */
    ch_sum = findch_sum(argv[2], '.');

    if (0 == ch_sum) {
        start = (log_info_t *)(&_ld_rf_static_logcomponent_code_start);
        end = (log_info_t *)(&_ld_rf_static_logcomponent_code_end);
    } else if (1 == ch_sum) {
        start = (log_info_t *)(&_ld_rf_static_logfile_code_start);
        end = (log_info_t *)(&_ld_rf_static_logfile_code_end);
    } else if (2 == ch_sum) {
        start = (log_info_t *)(&_ld_rf_static_logpri_code_start);
        end = (log_info_t *)(&_ld_rf_static_logpri_code_end);
    } else {
        __log_printf("input name = %s not support.\r\n", argv[2]);
        return -1;
    }

    left  = 0;
    right = end - start - 1;
    while (left <= right) {
        mid = (left + right) >> 1;
        cmp_val = strcmp(start[mid].name, argv[2]);
        if (cmp_val < 0) {
            left = mid + 1;
        } else if (cmp_val > 0) {
            right = mid - 1;
        } else {
            info = &start[mid];
            break;
        }
    }

    if (left > right) {
        __log_printf("input name = %s not find.\r\n", argv[2]);
        return -1;
    }

    /* set level */
    *(info->level) = level;
    __log_printf("set %s = %d\r\n", info->name, *(info->level));
    return 0;
}

static int cmd_log_set_level(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    if (0 != set_level(argc, argv)) {
        __log_printf("set log error.\r\n");
    }
    return 0;
}

static int cmd_log_info_dump(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    log_info_t *info_c, *info_f, *info_p;
    char         name_buf[LOG_NAMELEN_MAX] = {0};
    
    extern char _ld_rf_static_logcomponent_code_start;
    extern char _ld_rf_static_logcomponent_code_end;
    extern char _ld_rf_static_logfile_code_start;
    extern char _ld_rf_static_logfile_code_end;
    extern char _ld_rf_static_logpri_code_start;
    extern char _ld_rf_static_logpri_code_end;

    __log_printf("log code1 = %p - %p\r\n", &_ld_rf_static_logcomponent_code_start, &_ld_rf_static_logcomponent_code_end);
    __log_printf("log code2 = %p - %p\r\n", &_ld_rf_static_logfile_code_start, &_ld_rf_static_logfile_code_end);
    __log_printf("log code3 = %p - %p\r\n", &_ld_rf_static_logpri_code_start, &_ld_rf_static_logpri_code_end);

    for (info_c = (log_info_t *)&_ld_rf_static_logcomponent_code_start;
         (uint32_t)info_c < (uint32_t)&_ld_rf_static_logcomponent_code_end; info_c++) {
        if (strlen(info_c->name) > LOG_NAMELEN_MAX) {
            __log_printf("name too long.\r\n");
            return 0;
        }
        if (name_buf[0] != 0) {
            if (0 == strcmp(name_buf, info_c->name)) {
#if LOG_DUMP_DEDUPLICATE
              continue;
#endif              
            } else {
                memset(name_buf, 0, strlen(name_buf));
            }
        }
        
        __log_printf("[%-48s] = [%d]\r\n", info_c->name, *(info_c->level));
        
        if ((name_buf[0] != 0) && (strcmp(info_c->name, name_buf) == 0)) {
            continue;
        }
        strcpy(name_buf, info_c->name);

        for (info_f = (log_info_t *)&_ld_rf_static_logfile_code_start;
            (uint32_t)info_f < (uint32_t)&_ld_rf_static_logfile_code_end; info_f++) {

            if (strstr(info_f->name, info_c->name) == info_f->name) {

                __log_printf("[%-48s] = [%d]\r\n", info_f->name, *(info_f->level));

                for (info_p = (log_info_t *)&_ld_rf_static_logpri_code_start;
                    (uint32_t)info_p < (uint32_t)&_ld_rf_static_logpri_code_end; info_p++) {

                    if (strstr(info_p->name, info_f->name) == info_p->name) {
                        __log_printf("[%-48s] = [%d]\r\n", info_p->name, *(info_p->level));
                    }
                }
            }
        }
    }
    return 0;
}

#if LOG_TEST
static int cmd_log_test(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    log_testc_init();
    return 0;
}
#endif
#endif
static void log_set_poweron_softlevel(void)
{
    log_info_t *info;

    extern char _ld_rf_static_logcomponent_code_start;
    extern char _ld_rf_static_logcomponent_code_end;
    extern char _ld_rf_static_logfile_code_start;
    extern char _ld_rf_static_logfile_code_end;
    extern char _ld_rf_static_logpri_code_start;
    extern char _ld_rf_static_logpri_code_end;

    __log_printf("\r\nlog init set power on level %d, %d, %d.\r\n", LOG_POWERON_SOFTLEVEL_COMPONENT,
            LOG_POWERON_SOFTLEVEL_FILE, LOG_POWERON_SOFTLEVEL_PRI);
    for ( info = (log_info_t *)&_ld_rf_static_logcomponent_code_start;
          (uint32_t)info < (uint32_t)&_ld_rf_static_logcomponent_code_end; info++ ) {
        *(info->level) = LOG_POWERON_SOFTLEVEL_COMPONENT;
    }

    for ( info = (log_info_t *)&_ld_rf_static_logfile_code_start;
          (uint32_t)info < (uint32_t)&_ld_rf_static_logfile_code_end; info++ ) {
        *(info->level) = LOG_POWERON_SOFTLEVEL_FILE;
    }

    for ( info = (log_info_t *)&_ld_rf_static_logpri_code_start;
          (uint32_t)info < (uint32_t)&_ld_rf_static_logpri_code_end; info++ ) {
        *(info->level) = LOG_POWERON_SOFTLEVEL_PRI;
    }
}

/**
 * output log port interface
 *
 * @param log output of log
 * @param size log size
 */
void log_port_output(const char *log, size_t size) {
    /* output to terminal */
    __log_printf("%.*s", size, log);
    //TODO output to flash
}

/**
 * another copy string function
 *
 * @param cur_len current copied log length, max size is LOG_LINE_BUF_SIZE
 * @param dst destination
 * @param src source
 *
 * @return copied length
 */
size_t log_strcpy(size_t cur_len, char *dst, const char *src) {
    const char *src_old = src;

    if ((!dst) || (!src)) {
        __log_printf("assert.\r\n");
        return 0;
    }

    while (*src != 0) {
        /* make sure destination has enough space */
        if (cur_len++ < LOG_LINE_BUF_SIZE) {
            *dst++ = *src++;
        } else {
            break;
        }
    }
    return src - src_old;
}
/**
 * dump the hex format data to log
 *
 * @param name name for hex object, it will show on log header
 * @param width hex number for every line, such as: 16, 32
 * @param buf hex buffer
 * @param size buffer size
 */
void log_hexdump_out(const char *name, uint8_t width, const uint8_t *buf, uint16_t size)
{
#define __is_print(ch)       ((unsigned int)((ch) - ' ') < 127u - ' ')

    uint16_t i, j;
    uint16_t log_len = 0;
    char dump_string[8] = {0};
    int fmt_result;

    /* lock output */

    for (i = 0; i < size; i += width) {
        /* package header */
        fmt_result = snprintf(log_buf, LOG_LINE_BUF_SIZE, "[%s]: %04X-%04X: ", name, i, i + width - 1);
        /* calculate log length */
        if ((fmt_result > -1) && (fmt_result <= LOG_LINE_BUF_SIZE)) {
            log_len = fmt_result;
        } else {
            log_len = LOG_LINE_BUF_SIZE;
        }
        /* dump hex */
        for (j = 0; j < width; j++) {
            if (i + j < size) {
                snprintf(dump_string, sizeof(dump_string), "%02X ", buf[i + j]);
            } else {
                strncpy(dump_string, "   ", sizeof(dump_string));
            }
            log_len += log_strcpy(log_len, log_buf + log_len, dump_string);
            if ((j + 1) % 8 == 0) {
                log_len += log_strcpy(log_len, log_buf + log_len, " ");
            }
        }
        log_len += log_strcpy(log_len, log_buf + log_len, "  ");
        /* dump char for hex */
        for (j = 0; j < width; j++) {
            if (i + j < size) {
                snprintf(dump_string, sizeof(dump_string), "%c", __is_print(buf[i + j]) ? buf[i + j] : '.');
                log_len += log_strcpy(log_len, log_buf + log_len, dump_string);
            }
        }
        /* overflow check and reserve some space for newline sign */
        if (log_len + strlen("\r\n") > LOG_LINE_BUF_SIZE) {
            log_len = LOG_LINE_BUF_SIZE - strlen("\r\n");
        }
        /* package newline sign */
        log_len += log_strcpy(log_len, log_buf + log_len, "\r\n");

        /* do log output */
        log_port_output(log_buf, log_len);
    }

    /* unlock output */
}

void log_init(void)
{
    log_set_poweron_softlevel();
}

#ifdef CONFIG_CLI_CMD_ENABLE
const sh_cmd_t cmds_user[] STATIC_CLI_CMD_ATTRIBUTE = {
    { 
        .pCmd_name    = "logset", 
        .pDescription = "log pri set level", 
        .cmd_exec     = cmd_log_set_level
    },
    { 
        .pCmd_name    = "logdump", 
        .pDescription = "log info dump", 
        .cmd_exec     = cmd_log_info_dump
    },
#if LOG_TEST
    { 
        .pCmd_name    = "logtest", 
        .pDescription = "log test", 
        .cmd_exec     = cmd_log_test
    },
#endif
};
#endif

