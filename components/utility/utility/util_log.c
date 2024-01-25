/**
 * @file util_log.c
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief
 * @version 0.1
 * @date 2023-08-01
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

#include "util_log.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ONE_LINE_MAX_NUM        (50)/* for print_buf log length */
#define MODULE_LOG_LOCK_LOCK    /* reserved */
#define MODULE_LOG_LOCK_UNLOCK  /* reserved */
static char log_buf[512];
int log_buf_out(const char *file, int line, const void *inbuf, int len, LOG_BUF_OUT_DATA_TYPE_T type)
{
    char *buf = (char *)inbuf;
    char *pbuffer = NULL;

    pbuffer = (char *)pvPortMalloc(sizeof(log_buf));
    if (pbuffer == NULL)
    {
        return -1;
    }
    int m = 0, n = 0;
    int j = 0, k = 0, tmp = 0;

    MODULE_LOG_LOCK_LOCK;

    tmp = (sizeof(log_buf)) / 3; /* 数组最大长度 */
    if ((ONE_LINE_MAX_NUM > tmp) || (len < 1))
    {
        MODULE_LOG_LOCK_UNLOCK;
        return -1;
    }

    m = len / ONE_LINE_MAX_NUM;
    n = len % ONE_LINE_MAX_NUM;
    if (n > 0)
    {
        m++;
    }


    if (n > 0)
    {
        for (k = 0; k < m; k++)
        {
            if ((k + 1) == m)
            {
                tmp = 0;
                for (j = 0; j < n; j++)
                {
                    switch (type)
                    {
                    case LOG_BUF_OUT_DATA_TYPE_INT8:
                    {
                        tmp += sprintf(pbuffer + tmp, "%3d ", (int8_t)buf[k * ONE_LINE_MAX_NUM + j]);
                    }
                    break;
                    case LOG_BUF_OUT_DATA_TYPE_UNT8:
                    {
                        tmp += sprintf(pbuffer + tmp, "%3u ", (uint8_t)buf[k * ONE_LINE_MAX_NUM + j]);
                    }
                    break;
                    case LOG_BUF_OUT_DATA_TYPE_HEX:
                    default:
                    {
                        tmp += sprintf(pbuffer + tmp, "%02x ", (uint8_t)buf[k * ONE_LINE_MAX_NUM + j]);
                    }
                    }
                }
                ulog_buf_pri(file, line, "%.*s\r\n", tmp, pbuffer);
            }
            else
            {
                tmp = 0;
                for (j = 0; j < ONE_LINE_MAX_NUM; j++)
                {
                    switch (type)
                    {
                    case LOG_BUF_OUT_DATA_TYPE_INT8:
                    {
                        tmp += sprintf(pbuffer + tmp, "%3d ", (int8_t)buf[k * ONE_LINE_MAX_NUM + j]);
                    }
                    break;
                    case LOG_BUF_OUT_DATA_TYPE_UNT8:
                    {
                        tmp += sprintf(pbuffer + tmp, "%3u ", (uint8_t)buf[k * ONE_LINE_MAX_NUM + j]);
                    }
                    break;
                    case LOG_BUF_OUT_DATA_TYPE_HEX:
                    default:
                    {
                        tmp += sprintf(pbuffer + tmp, "%02x ", (uint8_t)buf[k * ONE_LINE_MAX_NUM + j]);
                    }
                    }
                }
                ulog_buf_pri(file, line, "%.*s\r\n", tmp, pbuffer);
            }
        }
    }
    else
    {
        for (k = 0; k < m; k++)
        {
            tmp = 0;
            for (j = 0; j < ONE_LINE_MAX_NUM; j++)
            {
                switch (type)
                {
                case LOG_BUF_OUT_DATA_TYPE_INT8:
                {
                    tmp += sprintf(pbuffer + tmp, "%3d ", (int8_t)buf[k * ONE_LINE_MAX_NUM + j]);
                }
                break;
                case LOG_BUF_OUT_DATA_TYPE_UNT8:
                {
                    tmp += sprintf(pbuffer + tmp, "%3u ", (uint8_t)buf[k * ONE_LINE_MAX_NUM + j]);
                }
                break;
                case LOG_BUF_OUT_DATA_TYPE_HEX:
                default:
                {
                    tmp += sprintf(pbuffer + tmp, "%02x ", (uint8_t)buf[k * ONE_LINE_MAX_NUM + j]);
                }
                }
            }
            ulog_buf_pri(file, line, "%.*s\r\n", tmp, pbuffer);
        }
    }

    MODULE_LOG_LOCK_UNLOCK;

    vPortFree(pbuffer);

    return 0;
}

#if defined(__CC_ARM) || defined(__CLANG_ARM) || (defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050))
void rf_printk(const char *format, ...)
{
    va_list args;

    /* args point to the first variable parameter */
    va_start(args, format);

    /* You can add your code under here. */
    vprintf(format, args);

    va_end(args);
}
#else
extern void vprint(const char *fmt, va_list argp);

void rf_printk(const char *format, ...)
{
    va_list args;

    /* args point to the first variable parameter */
    va_start(args, format);

    /* You can add your code under here. */
    vprint(format, args);

    va_end(args);
}
#endif


#ifdef __cplusplus
}
#endif

